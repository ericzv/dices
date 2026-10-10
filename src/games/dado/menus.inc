// ===========================================================================
// Menus: a abertura, as faixas das telas fora da partida e o online
// ===========================================================================
// As opcoes da lista do menu em uso.
static int menu_opcoes(const char **op)
{
    switch (menu_tela) {
    case MT_UM:      op[0] = "Partida rápida"; op[1] = "Modo Desafiante"; return 2;
    case MT_NIVEL:   op[0] = "Fácil"; op[1] = "Médio"; op[2] = "Difícil"; return 3;
    case MT_DESAFIO: op[0] = "Continuar a run"; op[1] = "Nova run"; return 2;
    default:
        op[0] = "1 jogador"; op[1] = "2 jogadores"; op[2] = "Online";
        op[3] = "Dados da casa"; op[4] = "Como jogar";
        return MN_N;
    }
}

static void desenha_abertura(void)
{
    desenha_lance();
    if (!abertura_pronta) return;
    // O titulo entra letra a letra, sem pressa.
    const char *titulo = "DADO EM CASA";
    int n = (int)(t_fase / 0.05f);
    if (n > 12) n = 12;
    char parcial[16];
    memcpy(parcial, titulo, (size_t)n);
    parcial[n] = 0;
    int x0 = GFX_W / 2 - gfx_largura(titulo, 3) / 2;
    gfx_texto(x0 + 3, 70 + 3, parcial, C_SOMBRA, 3, true);
    gfx_texto(x0, 70, parcial, C_LATAO, 3, true);
    if (t_fase < 0.75f) return;
    if (menu_tela) botao_voltar(KEY_ESC);
    gfx_rect(GFX_W / 2 - 150, 134, 300, 2, C_LATAO_ESC);
    static const char *const SUB[] = { "Criado por ERICZ", "1 jogador", "Partida rápida · escolha o nível",
                                       "Modo Desafiante" };
    txt_c(GFX_W / 2, 140, SUB[menu_tela], menu_tela ? C_LATAO : C_TEXTO_M, menu_tela != 0);

    const char *op[MN_N];
    int n_op = menu_opcoes(op);
    for (int i = 0; i < n_op; i++) {
        int yb = 170 + i * 26;
        bool cur = i == menu_cur;
        ui_botao(GFX_W / 2 - 110, yb, 220, 24, op[i], cur ? C_LATAO : 0, 0);
        zona_clique(GFX_W / 2 - 110, yb, 220, 24, &menu_cur, i, KEY_ENTER);
    }
    // Nos submenus, uma linha explica a opcao em foco.
    char dica[96] = "";
    if (menu_tela == MT_UM)
        snprintf(dica, sizeof dica, "%s", menu_cur == 0 ? "15 rodadas contra o computador."
                                                          : "Suba os 3 salões do cassino. Perdeu, acabou.");
    else if (menu_tela == MT_NIVEL) {
        static const char *const NIV[] = { "Joga por regras simples.",
                                           "Pensa, mas erra de vez em quando.",
                                           "Calcula cada jogada." };
        snprintf(dica, sizeof dica, "%s", NIV[menu_cur]);
    } else if (menu_tela == MT_DESAFIO) {
        if (menu_cur == 0) des_resumo(dica, sizeof dica);
        else snprintf(dica, sizeof dica, "A run guardada será perdida.");
    }
    if (dica[0]) txt_c(GFX_W / 2, 170 + n_op * 26 + 8, dica, menu_cur == 1 && menu_tela == MT_DESAFIO
                                                          ? C_VINHO_CLR : C_MARFIM_S, false);
    teclas_c(GFX_W / 2, GFX_H - FAIXA_H + 14, menu_tela
             ? "setas escolhem  ·  ENTER confirma  ·  ESC volta"
             : "M som  ·  N música  ·  F11 tela cheia  ·  - e + tamanho da mesa",
             C_TEXTO_M);
    // A versao, miuda no canto da mesa: diz qual jogo esta aberto.
    char ver[32];
    snprintf(ver, sizeof ver, "versão %s", DADO_VERSAO);
    gfx_texto(MESA_X1 - 12 - gfx_largura(ver, GFX_MIUDO), MESA_Y1 - 18, ver,
              gfx_mistura(C_TEXTO_M, C_FELTRO, 35), GFX_MIUDO, false);
}

// Faixas de cima e de baixo das telas fora da partida.
static void faixas_menu(const char *titulo, const char *dir, const char *rodape)
{
    int ty = (FAIXA_H - TXT_H) / 2 - 2, yb = GFX_H - FAIXA_H;
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    txt(botao_voltar(KEY_ESC), ty, titulo, C_LATAO, true);
    if (dir) txt(GFX_W - 18 - gfx_largura(dir, 1), ty, dir, C_TEXTO_M, false);
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
    teclas_c(GFX_W / 2, yb + (FAIXA_H - TECLA_H) / 2 + 1, rodape, C_TEXTO_M);
}

// Botao grande de menu, centrado.
static void botao_menu(int y, int w, const char *rot, bool cur)
{
    ui_botao(GFX_W / 2 - w / 2, y, w, 26, rot, cur ? C_LATAO : 0, 0);
}

// Quatro casas com as letras do codigo da sala.
static void casas_codigo(int y, const char *cod, bool cursor_pisca)
{
    for (int i = 0; i < 4; i++) {
        int x = GFX_W / 2 - 110 + i * 56;
        ui_painel(x, y, 48, 56);
        char c[2] = { cod[i] ? cod[i] : 0, 0 };
        if (c[0]) gfx_texto(x + 24 - gfx_largura(c, 2) / 2, y + 6, c, C_MARFIM, 2, true);
        else if (cursor_pisca && i == (int)strlen(cod) && (int)(t_fase * 2) % 2 == 0)
            gfx_rect(x + 14, y + 44, 20, 3, C_OURO);
    }
}

#define URL_WEB "ericzv.github.io/dices"

static void desenha_online(void)
{
    faixas_menu("ONLINE", NULL, "ESC volta ao menu");
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    if (!rede_disponivel()) {
        txt_c2(GFX_W / 2, 84, "Jogar online", C_LATAO);
        txt_c(GFX_W / 2, 140, "O modo online funciona na versão do jogo para o navegador:", C_MARFIM, false);
        txt_sombra_c(GFX_W / 2, 176, URL_WEB, C_OURO, 2);
        txt_c(GFX_W / 2, 236, "Abra esse endereço, escolha Online e crie uma sala.", C_MARFIM_S, false);
        teclas_c(GFX_W / 2, 272, "ENTER volta", C_TEXTO_M);
        return;
    }
    int st = rede_estado();
    txt_c2(GFX_W / 2, 64, "Jogar online", C_LATAO);
    if (st == REDE_ERRO) {
        txt_c(GFX_W / 2, 130, "Não deu para conectar:", C_VINHO_CLR, true);
        txt_c(GFX_W / 2, 156, rede_erro(), C_MARFIM, false);
        teclas_c(GFX_W / 2, 202, "ENTER tenta de novo  ·  ESC volta", C_TEXTO_M);
    } else if (st == REDE_CONECTANDO) {
        char t[32];
        snprintf(t, sizeof t, "conectando%.*s", (int)(t_fase * 3) % 4, "...");
        txt_c(GFX_W / 2, 150, t, C_MARFIM, true);
    } else if (st == REDE_ESPERANDO) {
        txt_c(GFX_W / 2, 116, "Sala criada! Passe este código para o seu amigo:", C_MARFIM, true);
        casas_codigo(146, rede_codigo(), false);
        char t[48];
        snprintf(t, sizeof t, "esperando o amigo entrar%.*s", (int)(t_fase * 3) % 4, "...");
        txt_c(GFX_W / 2, 222, t, C_OURO, false);
        txt_c(GFX_W / 2, 250, "Ele abre o jogo, escolhe Online > Entrar com código e digita o código.",
              C_TEXTO_M, false);
    } else if (digitando) {
        txt_c(GFX_W / 2, 116, "Digite o código da sala do seu amigo:", C_MARFIM, true);
        casas_codigo(146, codigo_dig, true);
        txt_c(GFX_W / 2, 222, strlen(codigo_dig) == 4 ? "ENTER entra na sala" : "4 letras ou números",
              strlen(codigo_dig) == 4 ? C_OURO : C_TEXTO_M, false);
        txt_c(GFX_W / 2, 250, "BACKSPACE apaga  ·  ESC volta", C_TEXTO_M, false);
    } else {
        txt_c(GFX_W / 2, 112, "Cada um no seu computador: um cria a sala, o outro entra com o código.",
              C_MARFIM_S, false);
        botao_menu(150, 240, "Criar sala", online_cur == 0);
        botao_menu(186, 240, "Entrar com código", online_cur == 1);
        zona_clique(GFX_W / 2 - 120, 150, 240, 26, &online_cur, 0, KEY_ENTER);
        zona_clique(GFX_W / 2 - 120, 186, 240, 26, &online_cur, 1, KEY_ENTER);
        txt_c(GFX_W / 2, 238, "Quem cria a sala joga primeiro na loja.", C_TEXTO_M, false);
    }
}
