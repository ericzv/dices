// ===========================================================================
// Menus: a abertura, as faixas das telas fora da partida e o online
// ===========================================================================
static float sai_mola(float q);          // desafio/desenho.inc

// As opcoes da lista do menu em uso.
static int menu_opcoes(const char **op)
{
    if (menu_tela == MT_DESAFIO) { op[0] = "Continuar"; op[1] = "Nova run"; return 2; }
    op[0] = "1 jogador"; op[1] = "2 jogadores"; op[2] = "Online";
    op[3] = "Dados da casa"; op[4] = "Tutorial";
    return MN_N;
}

// Cada opcao e uma ficha de cassino: a cor dela e o desenho do meio.
enum { ICO_UM, ICO_DOIS, ICO_ONLINE, ICO_DADO, ICO_TUTORIAL, ICO_CONTINUA, ICO_NOVA };
static void menu_ficha_de(int i, uint16_t *cor, int *ico)
{
    static const uint8_t ICO[MN_N] = { ICO_UM, ICO_DOIS, ICO_ONLINE, ICO_DADO, ICO_TUTORIAL };
    const uint16_t COR[MN_N] = { C_VINHO, C_AZUL, C_VIOLETA, C_EBANO_B, C_TERRA };
    if (menu_tela == MT_DESAFIO) { *cor = i ? C_VINHO : C_AZUL; *ico = i ? ICO_NOVA : ICO_CONTINUA; return; }
    *cor = COR[i]; *ico = ICO[i];
}
// O tom de cada ficha quando o cursor chega nela: a escala sobe da esquerda
// para a direita (pentatonica: nenhuma vizinha desafina).
static const int8_t TOM_FICHA[MN_N] = { 0, 2, 4, 7, 9 };

#define FICHA_R   29                     // o raio das fichas do menu
#define FICHA_Y   214                    // o centro delas
#define MENU_CLIQUE_T 0.24f              // a escolhida assentando, antes da acao

// Uma pessoa (cabeca e ombros) para os icones de jogador, na cor c.
static void icone_pessoa(int x, int y, uint16_t c)
{
    gfx_disco(x, y - 5, 4, c);
    for (int j = 0; j < 7; j++) {                  // os ombros: a metade de cima de um disco
        int w = (int)sqrtf((float)(64 - (j - 8) * (j - 8)));
        gfx_rect(x - w, y + j, 2 * w + 1, 1, c);
    }
}

// O desenho do meio da ficha, na cor da tinta, sobre o marfim do centro.
static void icone_menu(int x, int y, int ico, uint16_t tinta)
{
    uint16_t fundo = C_MARFIM;
    switch (ico) {
    case ICO_UM: icone_pessoa(x, y, tinta); break;
    case ICO_DOIS:
        icone_pessoa(x - 5, y - 1, gfx_mistura(tinta, fundo, 45));
        icone_pessoa(x + 5, y + 1, tinta);
        break;
    case ICO_ONLINE:                                  // o globo, com meridianos e o equador
        gfx_disco(x, y, 8, tinta);
        gfx_disco(x, y, 7, fundo);
        gfx_rect(x - 7, y, 15, 1, tinta);
        gfx_rect(x, y - 7, 1, 15, tinta);
        gfx_rect(x - 4, y - 5, 1, 11, tinta); gfx_rect(x + 4, y - 5, 1, 11, tinta);
        gfx_rect(x - 6, y - 4, 13, 1, gfx_mistura(tinta, fundo, 50));
        gfx_rect(x - 6, y + 4, 13, 1, gfx_mistura(tinta, fundo, 50));
        break;
    case ICO_DADO:                                    // um dado mostrando 3
        gfx_rect(x - 7, y - 7, 15, 15, tinta);
        gfx_rect(x - 8, y - 6, 1, 13, tinta); gfx_rect(x + 8, y - 6, 1, 13, tinta);
        gfx_rect(x - 6, y - 8, 13, 1, tinta); gfx_rect(x - 6, y + 8, 13, 1, tinta);
        for (int k = -1; k <= 1; k++) gfx_rect(x + k * 4 - 1, y + k * 4 - 1, 3, 3, fundo);
        break;
    case ICO_TUTORIAL: {                              // o ponto de interrogacao, em pixels
        static const char *const Q[12] = { "..#####..", ".##...##.", "##.....##", "##.....##", ".......##",
                                           "......##.", ".....##..", "....##...", "....##...", ".........",
                                           "....##...", "....##..." };
        for (int j = 0; j < 12; j++)
            for (int i = 0; i < 9; i++)
                if (Q[j][i] == '#') gfx_rect(x - 4 + i, y - 6 + j, 1, 1, tinta);
        break;
    }
    case ICO_CONTINUA: {                              // seguir: o triangulo de tocar
        float vx[3] = { (float)(x - 4), (float)(x - 4), (float)(x + 7) };
        float vy[3] = { (float)(y - 7), (float)(y + 7), (float)y };
        poligono(vx, vy, 3, tinta);
        break;
    }
    default:                                          // uma run nova: o mais
        gfx_rect(x - 7, y - 2, 15, 5, tinta);
        gfx_rect(x - 2, y - 7, 5, 15, tinta);
        break;
    }
}

// Uma ficha do menu, vista de cima. sobe: 0..1 (o cursor ergue a ficha e a
// sombra fica para tras); giro: o angulo das listras da borda; clique: 0..1
// enquanto a escolhida assenta (afunda, gira e solta um anel), -1 fora.
static void ficha_menu(int cx, int cy, int i, float sobe, float giro, bool foco, float clique)
{
    uint16_t cor;
    int ico;
    menu_ficha_de(i, &cor, &ico);
    int r = FICHA_R;
    float aperta = clique >= 0 ? sinf(fminf(clique, 1) * 3.1416f) : 0;
    int y = cy - (int)lroundf(sobe * 5) + (int)lroundf(aperta * 3);
    if (clique >= 0) {                                // o anel que sai da ficha assentando
        int ra = r + 3 + (int)(clique * 14);
        uint16_t ca = gfx_mistura(C_OURO, C_FELTRO, (int)(clique * 100));
        for (int k = 0; k < 40; k++) {
            float a = k * 0.157f;
            gfx_rect(cx + (int)(cosf(a) * ra), y + (int)(sinf(a) * ra), 1, 1, ca);
        }
    }
    ficha_sombra(cx + 2, cy + 3, r - (int)lroundf(sobe * 2));      // a sombra fica no feltro
    if (foco) ui_cantos(cx - r, y - r, 2 * r + 1, 2 * r + 1, C_OURO);
    ficha_disco(cx, y, r - 1, r - 1, cor);
    // As listras da borda: seis placas de marfim, que giram com a ficha.
    for (int k = 0; k < 6; k++) {
        float a = giro + k * 1.0472f, d = 0.16f;
        float vx[4] = { cx + cosf(a - d) * (r - 7), cx + cosf(a - d) * (r - 1.5f),
                        cx + cosf(a + d) * (r - 1.5f), cx + cosf(a + d) * (r - 7) };
        float vy[4] = { y + sinf(a - d) * (r - 7), y + sinf(a - d) * (r - 1.5f),
                        y + sinf(a + d) * (r - 1.5f), y + sinf(a + d) * (r - 7) };
        poligono(vx, vy, 4, C_MARFIM_S);
    }
    uint16_t rc[4];
    rampa_de(cor, rc);
    gfx_disco(cx, y, r - 8, rc[1]);                                    // o friso de dentro, um tom abaixo
    ficha_disco_em(cx, y, r - 9, r - 9, cor, false, cx, y, r - 1);
    for (int k = 0; k < 18; k++) {                    // o pontilhado em volta do centro
        float a = giro * 0.5f + k * 0.349f;
        gfx_rect(cx + (int)lroundf(cosf(a) * (r - 11)), y + (int)lroundf(sinf(a) * (r - 11)), 1, 1,
                 gfx_mistura(cor, C_MARFIM, 55));
    }
    ficha_disco_em(cx, y, r - 13, r - 13, C_MARFIM, false, cx, y, r - 13);   // o centro de marfim, com o desenho
    gfx_rect(cx - 6, y - (r - 13) + 2, 4, 1, C_BRANCO);
    icone_menu(cx, y, ico, gfx_mistura(cor, C_BARRA, 25));
}

// Onde fica a ficha i entre n.
static int ficha_x(int i, int n)
{
    int passo = n > 2 ? 104 : 150;
    return GFX_W / 2 + (2 * i - (n - 1)) * passo / 2;
}

// As fichas andam no passo: sobem e descem devagar, a do cursor gira, a
// escolhida assenta e, no fim, a acao acontece. Cada ficha que o cursor
// alcanca toca um tom.
static void menu_executa(int i);
static void menu_passo(float dt)
{
    const char *op[MN_N];
    int n = menu_opcoes(op);
    if (menu_tela != menu_tela_ant) { menu_tela_ant = menu_tela; menu_ant = menu_cur; }
    if (menu_cur != menu_ant) {
        if (menu_ant >= 0 && menu_cur < n) som_toca_tom(SOM_MENU_PASSA, TOM_FICHA[menu_cur]);
        menu_ant = menu_cur;
    }
    float a = fminf(1, dt * 14);
    for (int i = 0; i < MN_N; i++) {
        bool foco = i == menu_cur && i < n;
        menu_sobe[i] += ((foco ? 1.0f : 0.0f) - menu_sobe[i]) * a;
        if (foco) menu_giro[i] += dt * 0.9f;
    }
    if (t_menu_clique < 0) return;
    t_menu_clique += dt;
    float q = t_menu_clique / MENU_CLIQUE_T;
    menu_giro[menu_escolhida] += dt * 14 * fmaxf(0, 1 - q);   // gira rapido e para, como a ficha deitando
    if (q >= 1) { t_menu_clique = -1; menu_executa(menu_escolhida); }
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
    txt_c(GFX_W / 2, 140, menu_tela ? "Modo Desafiante" : "Criado por ERICZ", menu_tela ? C_LATAO : C_TEXTO_M,
          menu_tela != 0);

    // As fichas entram uma depois da outra, caindo de leve no feltro.
    const char *op[MN_N];
    int n_op = menu_opcoes(op);
    for (int i = 0; i < n_op; i++) {
        float q = (t_fase - 0.75f - i * 0.06f) / 0.25f;
        if (q <= 0) continue;
        int cx = ficha_x(i, n_op), cy = FICHA_Y - (q < 1 ? (int)((1 - sai_mola(q)) * 10) : 0);
        bool cur = i == menu_cur;
        ficha_menu(cx, cy, i, menu_sobe[i], menu_giro[i] + i * 0.4f, cur,
                   t_menu_clique >= 0 && i == menu_escolhida ? t_menu_clique / MENU_CLIQUE_T : -1);
        int ly = FICHA_Y + FICHA_R + 8;
        txt_c(cx, ly, op[i], cur ? C_OURO : C_MARFIM_S, cur);
        zona_clique(cx - FICHA_R - 8, FICHA_Y - FICHA_R - 6, 2 * FICHA_R + 16, 2 * FICHA_R + 30, &menu_cur, i, KEY_ENTER);
    }
    // Embaixo, uma linha diz o que a ficha em foco abre.
    char dica[96] = "";
    if (menu_tela == MT_DESAFIO) {
        if (menu_cur == 0) des_resumo(dica, sizeof dica);
        else snprintf(dica, sizeof dica, "A run guardada será perdida.");
    } else {
        static const char *const DICA[MN_N] = {
            "Modo Desafiante: uma run pelos salões do cassino.", "Dois na mesma mesa, 15 rodadas.",
            "Contra um amigo, pela internet.", "Todos os dados e o que cada um faz.", "As regras, passo a passo." };
        snprintf(dica, sizeof dica, "%s", DICA[menu_cur]);
    }
    txt_c(GFX_W / 2, FICHA_Y + FICHA_R + 30, dica,
          menu_cur == 1 && menu_tela == MT_DESAFIO ? C_VINHO_CLR : C_TEXTO_M, false);
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
    faixa_madeira(0);
    txt(botao_voltar(KEY_ESC), ty, titulo, C_LATAO, true);
    if (dir) txt(GFX_W - 18 - gfx_largura(dir, 1), ty, dir, C_TEXTO_M, false);
    faixa_madeira(yb);
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
        txt_c2(GFX_W / 2, 84, "JOGAR ONLINE", C_LATAO);
        txt_c(GFX_W / 2, 140, "O modo online funciona na versão do jogo para o navegador:", C_MARFIM, false);
        txt_sombra_c(GFX_W / 2, 176, URL_WEB, C_OURO, 2);
        txt_c(GFX_W / 2, 236, "Abra esse endereço, escolha Online e crie uma sala.", C_MARFIM_S, false);
        teclas_c(GFX_W / 2, 272, "ENTER volta", C_TEXTO_M);
        return;
    }
    int st = rede_estado();
    txt_c2(GFX_W / 2, 64, "JOGAR ONLINE", C_LATAO);
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
