// ---------------------------------------------------------------------------
// Perfis e dificuldades
// ---------------------------------------------------------------------------
static const char *const NOME_DIF[N_NIVEIS + 1] = { "Normal", "Desafiante - Nível 1", "Desafiante - Nível 2",
    "Desafiante - Nível 3", "Desafiante - Nível 4", "Desafiante - Nível 5", "Desafiante - Nível 6",
    "Desafiante - Nível 7", "Desafiante - Nível 8", "Desafiante - Nível 9", "Desafiante - Nível 10" };
static const char *const NOME_DIF_CURTO[N_NIVEIS + 1] = { "Normal", "Nível 1", "Nível 2", "Nível 3", "Nível 4",
    "Nível 5", "Nível 6", "Nível 7", "Nível 8", "Nível 9", "Nível 10" };
// Cada dificuldade e um andar do cassino, mais chique a cada um: a mesa
// mostra o andar nos acabamentos (veja pinta_mesa).
static const char *const ANDAR_DIF[N_NIVEIS + 1] = { "Térreo", "1º Andar", "2º Andar", "3º Andar", "4º Andar",
    "5º Andar", "6º Andar", "7º Andar", "8º Andar", "9º Andar", "Cobertura" };
static const uint16_t COR_DIF[N_NIVEIS + 1] = { C_MARFIM, C_BRONZE, RGB(210, 120, 80), C_PRATA, RGB(110, 190, 150),
    C_OURO, RGB(232, 150, 60), RGB(222, 70, 92), RGB(176, 110, 210), RGB(90, 140, 230), RGB(150, 220, 240) };
// O que cada dificuldade acrescenta (vale junto com as de baixo): uma coisa
// por nivel, para a subida ser aos poucos.
static const char *const PENA_DIF[N_NIVEIS + 1] = {
    "Os salões como sempre; só os chefes têm trunfo.",
    "O Crupiê passa a virar na 5ª rodada.",
    "A Dona também vira na 5ª rodada.",
    "Mais oponentes trazem o trunfo.",
    "O Barão também vira na 5ª rodada.",
    "As mesas difíceis pensam mais.",
    "Oponentes trazem 1 dado raro a mais.",
    "Viradas do Crupiê e da Dona mais fortes.",
    "Oponentes trazem 2 dados raros a mais.",
    "Barão vira mais forte; chefes pensam mais.",
    "Os oponentes ganham um amuleto.",
};

static const char *nome_dif(int d) { return NOME_DIF_CURTO[d <= N_NIVEIS ? d : 0]; }

// O Coringa da dificuldade d, numa linha (0 se ele ainda nao aparece): do
// Nivel 5, depois do Barao, e mais forte a cada nivel.
static int coringa_txt(int d, char *s, int n)
{
    if (d < CORINGA_DESDE) { s[0] = 0; return 0; }
    if (d == CORINGA_DESDE) return snprintf(s, (size_t)n, "O Coringa espera depois do Barão, com %d dados.", coringa_bolsa(d));
    return snprintf(s, (size_t)n, "Coringa mais forte: %d dados%s.", coringa_bolsa(d),
                    d >= N_NIVEIS ? ", um amuleto e seu dado anulado some"
                    : d == CORINGA_SOME ? ", IA afiada; da 5ª rodada, seu dado anulado some"
                    : d > CORINGA_SOME ? "; da 5ª rodada, seu dado anulado some" : "");
}

static uint16_t cor_dif(int d) { return COR_DIF[d <= N_NIVEIS ? d : 0]; }

static int  pf_cur;                      // card em foco na tela de perfis
static bool pf_apagando;                 // pedindo confirmacao para apagar
static int  pf_avatar;                   // tela de criar perfil
static char pf_nome[11];
static int  dif_cur;
static bool  dif_pop;                    // o pop-up dos desbloqueios do andar escolhido (o bau)
static float dif_pop_t;
static bool  dif_escolhe;                // "Trocar de andar": o predio acende; as setas ou o clique escolhem o andar
static int   dif_antes;                  // o andar de antes do "Trocar de andar" (ESC volta a ele)
// O botao em foco: as setas passam por eles e ENTER aperta. O "Iniciar a
// run" e o 0, o de sempre; o de cima e o ultimo dos dois de cima visitado.
enum { DIF_INICIAR, DIF_DESBLOQ, DIF_TROCA };
static int   dif_foco, dif_foco_cima = DIF_DESBLOQ;
#define TECLA_ANDAR   0x230                // o clique num andar aberto, no predio ou na botoeira (+ o andar): so escolhe
static int   negado_d = -1;                // o andar trancado que acabaram de apertar (predio.inc)
static float negado_t = -9;

// Uma parte do estilo, pequena: o feltro e a mesa num retalho, o material
// num D6. lin: 0 feltro, 1 material, 2 mesa.
static void estilo_mini(int lin, int k, int cx, int cy, int r)
{
    if (lin == 1) {
        uint8_t antes = material_j[0];
        material_j[0] = (uint8_t)k;
        desenha_dado((float)cx, (float)cy, (float)r, D_D6, 0, 0.18f, 5, 2, false);
        material_j[0] = antes;
        return;
    }
    uint16_t c = lin == 0 ? FELTROS[k].tons[k == FT_GALAXIA ? 3 : 4] : k == MS_NOGUEIRA ? C_MADEIRA : MESAS[k].madeira[1];
    uint16_t aro = lin == 0 ? C_LATAO_ESC : k == MS_NOGUEIRA ? C_OURO : k == MS_LED ? LED_TOM[5] : MESAS[k].metal;
    int x = cx - r, y = cy - r * 2 / 3, w = r * 2, h = r * 4 / 3;
    gfx_rect(x, y, w, h, c);
    if (lin == 0) retalho_enfeite(k, x, y, w, h);
    if (lin != 0 || k != FT_NATAL) ui_contorno(x, y, w, h, aro);
}

// As partes do estilo de um andar (linha da tela de estilo e a opcao).
static int estilo_partes(int andar, int *lin, int *k)
{
    int n = 0;
    for (int i = 0; i < FT_N; i++)  if (ABRE_FELTRO[i] == andar)   { lin[n] = 0; k[n++] = i; }
    for (int i = 0; i < MAT_N; i++) if (ABRE_MATERIAL[i] == andar) { lin[n] = 1; k[n++] = i; }
    for (int i = 0; i < MS_N; i++)  if (ABRE_MESA[i] == andar)     { lin[n] = 2; k[n++] = i; }
    return n;
}
static void estilo_parte_nome(int lin, int k, char *s, int n)
{
    if (lin == 0) snprintf(s, (size_t)n, "feltro %s", FELTROS[k].nome);
    else if (lin == 1) snprintf(s, (size_t)n, "dados de %s", MATERIAL[k].nome);
    else snprintf(s, (size_t)n, "mesa %s", MESAS[k].nome);
}

static void des_abre_perfis(void)
{
    perfis_le();
    pf_cur = perfil_atual >= 0 ? perfil_atual : 0;
    pf_apagando = false;
    t_fase = 0;
    fase = F_PERFIS;
}

// Nova run: com dificuldades abertas, pergunta qual antes; depois, a bolsa.
static void abre_bolsas(void);           // bolsa_escolha.inc
static void des_escolhe_nova(void)
{
    if (perfil()->nivel > 0) {
        dif_cur = perfil()->nivel;               // o ultimo andar aberto, ja escolhido
        dif_pop = dif_escolhe = false;
        dif_foco = DIF_INICIAR;                  // e o "Iniciar a run" em foco
        t_fase = 0;
        fase = F_RDIFIC;
        return;
    }
    des_dif = 0;
    abre_bolsas();
}

static void des_entra_perfil(int i)
{
    perfil_atual = i;
    perfis_salva();
    estilo_do_perfil();                          // o feltro, a mesa e os dados dele
    if (des_tem_run()) {                         // continuar ou comecar outra
        menu_tela = MT_DESAFIO;
        menu_cur = 0;
        t_fase = 1.0f;
        fase = F_ABERTURA;
        return;
    }
    des_escolhe_nova();
}

static int avatar_retrato(void) { return perfil_atual >= 0 && perfis[perfil_atual].nome[0] ? RETRATO_AVATAR + perfil()->avatar : -1; }

static void moldura_retrato(int x, int y, int esc, int av, float t, bool cur)
{
    uint16_t r[4];                               // a moldura de latao em rampa, como a dos retratos do mapa
    ui_rampa(C_LATAO, !cur, r);
    ui_contorno(x - 4, y - 4, RT_W * esc + 8, RT_H * esc + 8, C_BARRA);
    ui_aro(x - 3, y - 3, RT_W * esc + 6, RT_H * esc + 6, 2, r, UI_VIVO);
    gfx_rect(x - 1, y - 1, RT_W * esc + 2, RT_H * esc + 2, C_BARRA);
    gfx_rect(x, y, RT_W * esc, RT_H * esc, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(x, y, esc, RETRATO_AVATAR + av, t, false, 0, RT_H);
}

static void faixa_titulo(const char *s)
{
    faixa_madeira(0);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(botao_voltar(KEY_ESC), ty, "MODO DESAFIANTE", C_LATAO, true);
    txt_c(GFX_W / 2, ty, s, C_MARFIM, true);
}

static void desenha_perfis(void)
{
    ui_escurece(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, 110);
    faixa_titulo("Quem vai jogar?");
    for (int i = 0; i < N_PERFIS; i++) {
        const perfil_t *p = &perfis[i];
        int w = 170, h = 190, x = GFX_W / 2 + (i - 1) * 186 - w / 2, y = 66;
        bool cur = i == pf_cur;
        ui_cartao(x, y, w, h, cur, 0);
        zona_clique(x, y, w, h, &pf_cur, i, KEY_ENTER);
        if (!p->nome[0]) {
            int cx = x + w / 2, cy = y + 84;
            uint16_t c = cur ? C_OURO : C_TEXTO_M;
            gfx_rect(cx - 14, cy - 2, 28, 4, c);
            gfx_rect(cx - 2, cy - 14, 4, 28, c);
            txt_c(cx, y + 120, "novo perfil", c, true);
            continue;
        }
        int esc = 3, rx = x + (w - RT_W * esc) / 2, ry = y + 14;
        moldura_retrato(rx, ry, esc, p->avatar, t_fase + i * 2.1f, cur);
        txt_c(x + w / 2, ry + RT_H * esc + 8, p->nome, cur ? C_OURO : C_MARFIM, true);
        char s[48];
        int nv = p->nivel;
        txt_c(x + w / 2, ry + RT_H * esc + 26, NOME_DIF[nv], nv ? COR_DIF[nv] : C_MARFIM_S, false);
        snprintf(s, sizeof s, "%d %s", p->partidas, p->partidas == 1 ? "partida" : "partidas");
        txt_c(x + w / 2, ry + RT_H * esc + 42, s, C_TEXTO_M, false);
        int ganhou = 0;
        for (int d = 0; d <= N_NIVEIS; d++) ganhou += p->venceu[d];
        if (ganhou) {
            snprintf(s, sizeof s, "Runs vencidas: %d", ganhou);
            txt_c(x + w / 2, ry + RT_H * esc + 58, s, C_OURO, false);
        }
    }
    if (pf_apagando && perfis[pf_cur].nome[0]) {
        char s[80];
        snprintf(s, sizeof s, "Apagar %s e a run guardada?", perfis[pf_cur].nome);
        txt_c(GFX_W / 2, 266, s, C_VINHO_CLR, true);
        rodape_run("ENTER ou X apaga  ·  ESC deixa como está");
    } else {
        if (perfis[pf_cur].nome[0]) {
            int bx = GFX_W / 2 + (pf_cur - 1) * 186 - 104;
            ui_botao(bx, 264, 64, 22, "estilo", 0, 0);
            zona_clique(bx, 264, 64, 22, NULL, 0, 'e');
            ui_botao(bx + 70, 264, 80, 22, "histórico", 0, 0);
            zona_clique(bx + 70, 264, 80, 22, NULL, 0, 'h');
            ui_botao(bx + 156, 264, 60, 22, "apagar", C_VINHO, 0);
            zona_clique(bx + 156, 264, 60, 22, NULL, 0, 'x');
        }
        rodape_run(controle_estilo   // no controle: o X (I) e o Y (TAB)
                   ? "setas escolhem  ·  ENTER entra  ·  I estilo  ·  TAB histórico  ·  X apaga  ·  ESC volta"
                   : "setas escolhem  ·  ENTER entra  ·  E estilo  ·  H histórico  ·  X apaga  ·  ESC volta");
    }
}

static void desenha_cria_perfil(void)
{
    ui_escurece(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, 110);
    faixa_titulo("Novo perfil");
    // Com o controle, o teclado na tela entra embaixo do nome: o retrato
    // fica um pouco menor para caber.
    bool tt = tt_ativo();
    int esc = tt ? 3 : 4, rx = GFX_W / 2 - RT_W * esc / 2, ry = tt ? 52 : 58;
    moldura_retrato(rx, ry, esc, pf_avatar, t_fase, true);
    if (tt && tt_lin < 0) ui_cantos(rx - 6, ry - 6, RT_W * esc + 12, RT_H * esc + 12, C_OURO);   // o foco no retrato
    // Setas para trocar o retrato.
    for (int lado = -1; lado <= 1; lado += 2) {
        int cx = GFX_W / 2 + lado * (RT_W * esc / 2 + 34), cy = ry + RT_H * esc / 2;
        float o[6] = { (float)(cx + lado * 8), (float)cy, (float)(cx - lado * 6), (float)(cy - 10),
                       (float)(cx - lado * 6), (float)(cy + 10) };
        float vx[3] = { o[0], o[2], o[4] }, vy[3] = { o[1], o[3], o[5] };
        poligono(vx, vy, 3, C_LATAO);
        zona_clique(cx - 16, cy - 18, 32, 36, NULL, 0, lado < 0 ? KEY_LEFT : KEY_RIGHT);
    }
    int y = ry + RT_H * esc + (tt ? 4 : 8);
    txt_c(GFX_W / 2, y, NOME_AVATAR[pf_avatar], C_OURO, true);
    // O nome, digitado.
    int bw = 180, bx = GFX_W / 2 - bw / 2, by = y + (tt ? 19 : 22);
    gfx_rect(bx + 1, by + 1, bw - 2, 18, C_PLACA);
    ui_contorno(bx, by, bw, 20, C_LATAO);
    char s[16];
    bool pisca = fmodf(t_fase, 1.0f) < 0.55f;
    snprintf(s, sizeof s, "%s%s", pf_nome, pisca && strlen(pf_nome) < 10 ? "_" : " ");
    if (pf_nome[0] || pisca) txt_c(GFX_W / 2, by + 2, s, C_MARFIM, true);
    if (!pf_nome[0] && !pisca) txt_c(GFX_W / 2, by + 2, "seu nome", C_TEXTO_M, false);
    // Botao de criar.
    int ok = pf_nome[0] != 0;
    if (tt) {                                    // o teclado na tela, e o criar embaixo dele
        tt_desenha(by + 26, true);
        int cy = by + 26 + TT_ALTO + 6;
        ui_botao(GFX_W / 2 - 50, cy, 100, 22, "criar", C_LATAO, !ok ? UI_DESLIGADO : tt_lin == TT_LINHAS ? UI_FOCO : 0);
        rodape_run("setas escolhem  ·  ENTER escreve  ·  BACKSPACE apaga  ·  TAB cria  ·  ESC volta");
        return;
    }
    ui_botao(GFX_W / 2 - 50, by + 28, 100, 24, "criar", C_LATAO, ok ? 0 : UI_DESLIGADO);
    zona_clique(GFX_W / 2 - 50, by + 30, 100, 20, NULL, 0, KEY_ENTER);
    rodape_run("setas trocam o retrato  ·  digite o nome  ·  ENTER cria  ·  ESC volta");
}

// O que os chefes do andar d abrem, chefe a chefe: aceso o que ja abriu,
// apagado (com o nome do chefe apagado) o que falta. A lista vai de x0 a x1,
// do ly para baixo, sem passar de y_max (as linhas chegam mais perto se nao
// couber: a letra maior, num andar que abre muito).
static void lista_desbloqueios(int d, int x0, int ly, int x1, int y_max)
{
    bool conhece_coringa = chefe_abriu(CORINGA_DESDE, CH_CORINGA);   // ja venceu o Coringa uma vez
    static const char *const CURTO[CH_N] = { "Crupiê", "Dona", "Barão", "Coringa" };
    const char *curto_c[CH_N] = { CURTO[0], CURTO[1], dezembro ? "Noel" : CURTO[2], conhece_coringa ? CURTO[3] : "???" };
    int col = 60;                                    // a coluna dos nomes: cabe o maior (com a letra maior tambem)
    for (int c = 0; c < chefes_de(d); c++) {
        int w = gfx_largura(curto_c[c], GFX_FINO) + 14;
        if (w > col) col = w;
    }
    // Duas passadas: a primeira so mede; se a lista nao cabe no painel (a
    // letra maior, num nivel que abre muito), as linhas chegam mais perto.
    int passo = 19, ly0 = ly;
    for (int medindo = 1; medindo >= 0; medindo--) {
        int linhas = 0, prox = ly0 + 9;
        ly = ly0;
        for (int c = 0; c < chefes_de(d); c++) {
            novidade_t it[PEDACO_MAX];
            int ni = pedaco(d, c, it);
            if (!ni) continue;
            bool ab = chefe_abriu(d, c);
            int ix = x0 + col, iy = prox;
            linhas++;
            if (!medindo) gfx_texto(x0, iy - 10, curto_c[c], ab ? C_OURO : C_TEXTO_M, GFX_FINO, false);
            for (int i = 0; i < ni; i++) {               // em fluxo: cada um com a largura do seu nome
                int v = it[i].v, lin[4], k[4], np = it[i].k == NV_ESTILO ? estilo_partes(v, lin, k) : 1;
                for (int q = 0; q < np; q++) {
                    char nome[40];
                    switch (it[i].k) {
                    case NV_DIF:     snprintf(nome, sizeof nome, "%s", NOME_DIF_CURTO[v]); break;
                    case NV_AMULETO: snprintf(nome, sizeof nome, "%s", AMULETO[v].nome); break;
                    case NV_ADESIVO: snprintf(nome, sizeof nome, "%s", ADESIVO[v].nome); break;
                    case NV_DADO:    snprintf(nome, sizeof nome, "%s", TIPO[v].nome); break;
                    default:         estilo_parte_nome(lin[q], k[q], nome, sizeof nome); break;
                    }
                    int wi = 13 + gfx_largura(nome, GFX_FINO) + 12;
                    if (ix + wi > x1) { ix = x0 + col; iy += passo; linhas++; }
                    uint16_t cn = C_TEXTO_M;
                    if (medindo) { ix += wi; continue; }
                    switch (it[i].k) {
                    case NV_DIF: {
                        char nn[4];
                        snprintf(nn, sizeof nn, "%d", v);
                        gfx_disco(ix, iy, 8, ab ? COR_DIF[v] : C_FELTRO2);
                        gfx_disco(ix, iy, 6, C_FELTRO_ESC);
                        gfx_texto(ix - gfx_largura(nn, GFX_MIUDO) / 2, iy - 6, nn, ab ? COR_DIF[v] : C_TEXTO_M, GFX_MIUDO, false);
                        if (ab) cn = COR_DIF[v];
                        break;
                    }
                    case NV_AMULETO:
                        desenha_amuleto(ix, iy, 8, v, !ab);
                        if (ab) { cn = AMULETO[v].cor; dica_zona(ix - 10, iy - 9, wi, 18, DICA_AMULETO, v); }
                        break;
                    case NV_ADESIVO:
                        ad_solto_d(ix, iy, v, 1, 0);
                        if (ab) { cn = ADESIVO[v].luz; dica_zona(ix - 10, iy - 9, wi, 18, DICA_ADESIVO, v); }
                        break;
                    case NV_DADO:
                        desenha_dado((float)ix, (float)iy, 8, v, 0, 0, 0, 0, !ab);
                        if (ab) { cn = cor_dado(v); dica_zona(ix - 10, iy - 9, wi, 18, DICA_DADO, v); }
                        break;
                    default:
                        estilo_mini(lin[q], k[q], ix, iy, 7);
                        if (!ab) acinzenta(ix - 8, iy - 8, 17, 17, 70);
                        if (ab) cn = C_MARFIM_S;
                        break;
                    }
                    gfx_texto(ix + 12, iy - 10, nome, cn, GFX_FINO, false);
                    ix += wi;
                }
            }
            ly = iy + 10;
            prox = iy + passo;
        }
        int sobra = ly - y_max;
        if (medindo && sobra > 0 && linhas > 1) {
            passo -= (sobra + linhas - 2) / (linhas - 1);
            if (passo < 15) passo = 15;
        }
    }
}

// Quantos itens o andar d abre (as partes de um estilo contam uma a uma) e
// quantos ja abriram.
static void conta_desbloqueios(int d, int *abertos, int *total)
{
    *abertos = *total = 0;
    for (int c = 0; c < chefes_de(d); c++) {
        novidade_t it[PEDACO_MAX];
        int ni = pedaco(d, c, it);
        for (int i = 0; i < ni; i++) {
            int lin[4], k[4], np = it[i].k == NV_ESTILO ? estilo_partes(it[i].v, lin, k) : 1;
            *total += np;
            if (chefe_abriu(d, c)) *abertos += np;
        }
    }
}

// O bau dos desbloqueios: 16 x 13, madeira com as cintas e a fechadura de
// latao.
static void icone_bau(int x, int y)
{
    static const char *const L[13] = {
        "..BBBBBBBBBBBB..", ".BWWWWWWWWWWWWB.", "BWwwwwwwwwwwwwWB", "BwwwwwwwwwwwwwwB",
        "BGGGGGGBBGGGGGGB", "BgggggBOOBgggggB", "BwwwwwBOkBwwwwwB", "BwwwwwBOkBwwwwwB",
        "BwwwwwwBBwwwwwwB", "BdwwwwwwwwwwwwdB", "BGddddddddddddGB", "BGGddddddddddGGB",
        ".BBBBBBBBBBBBBB." };
    for (int j = 0; j < 13; j++)
        for (int i = 0; i < 16; i++) {
            uint16_t c;
            switch (L[j][i]) {
            case 'B': c = C_BARRA; break;
            case 'W': c = C_TERRA; break;
            case 'w': c = RGB(120, 78, 50); break;
            case 'd': c = RGB(78, 46, 30); break;
            case 'G': c = C_OURO; break;
            case 'g': c = C_LATAO; break;
            case 'O': c = C_AMARELO; break;
            case 'k': c = C_EBANO; break;
            default: continue;
            }
            gfx_pixel(x + i, y + j, c);
        }
}

// O botao de comecar a run: o feltro verde da mesa (os tons do feltro Verde,
// que valem em qualquer mesa) numa moldura de latao com rebites, e a letra
// branca (Jersey 2x) com o contorno escuro. Em foco (o de sempre), as
// cantoneiras de ouro respiram em volta e o ENTER afunda ele.
static void botao_iniciar(int x, int y, int w, int h, const char *s, bool foco)
{
    bool em = mouse_em(x, y, w, h);
    bool baixo = t_aperto > 0 && aperto_x >= x && aperto_x < x + w && aperto_y >= y && aperto_y < y + h;
    if (foco) { foco_bx = x + w / 2; foco_by = y + h / 2; }          // o ENTER afunda ele
    int d = baixo ? 2 : 0;
    if (!baixo) ui_sombra(x, y, w, h);
    uint16_t ouro[4];
    rampa_de(em ? C_AMARELO : C_OURO, ouro);                       // o mouse em cima: o latao acende
    static const float K[4] = { 3, 2, 1, 0 };
    ui_aro(x, y + d, w, h - d, 4, ouro, K);                         // a moldura de latao
    ui_contorno(x, y + d, w, h - d, C_BARRA);
    int fx = x + 4, fy = y + d + 4, fw = w - 8, fh = h - d - 8;
    ui_contorno(fx - 1, fy - 1, fw + 2, fh + 2, C_LATAO_ESC);      // o filete escuro entre o latao e o feltro
    // O feltro: o tom do meio do Verde, com os fios (pontos soltos um tom
    // acima e um abaixo) e a luz em cima, como o pano da mesa.
    const uint16_t *fv = FELTROS[FT_VERDE].tons;
    for (int j = 0; j < fh; j++)
        for (int i = 0; i < fw; i++) {
            uint32_t hh = hash2(fx + i, fy + j) % 23;
            uint16_t c = hh == 0 ? fv[5] : hh == 1 ? fv[2] : fv[4];
            if (j < 2 && bayer(fx + i, fy + j) < 0.75f - j * 0.3f) c = fv[5];          // a luz na beira de cima
            if (j >= fh - 3 && bayer(fx + i, fy + j) < (j - (fh - 4)) / 3.0f) c = fv[2];   // e a sombra embaixo
            gfx_pixel(fx + i, fy + j, c);
        }
    for (int k = 0; k < 4; k++) {                                    // os rebites de ouro nos cantos da moldura
        int rx = k & 1 ? x + w - 4 : x + 1, ry = y + d + (k & 2 ? h - d - 4 : 1);
        gfx_rect(rx, ry, 3, 3, C_LATAO_ESC);
        gfx_pixel(rx + 1, ry + 1, C_AMARELO);
    }
    int tw = gfx_largura(s, 2), tx = fx + (fw - tw) / 2, ty = fy + fh / 2 - 21;
    static const int8_t CONT[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {1,-1}, {-1,1}, {1,1} };
    gfx_texto(tx + 1, ty + 2, s, C_BARRA, 2, false);                // a sombra
    for (int k = 0; k < 8; k++) gfx_texto(tx + CONT[k][0], ty + CONT[k][1], s, C_BARRA, 2, false);
    gfx_texto(tx, ty, s, C_BRANCO, 2, false);
    if (foco && !baixo) ui_cantos(x - 1, y - 1, w + 2, h + 2, C_OURO);
    zona_clique(x, y, w, h, &dif_foco, DIF_INICIAR, KEY_ENTER);      // no toque, pede o segundo toque
}

// Um dos dois botoes de cima do painel (os desbloqueios, trocar de andar): a face de
// sempre, o bau a esquerda (icone 0; 1: sem icone, o rotulo no meio), o
// rotulo e um extra miudo (a conta do bau).
static void botao_dif(int x, int y, int w, int h, int icone, const char *rot, const char *extra, bool aceso, int qual)
{
    int d;
    uint16_t c = ui_botao_face(x, y, w, h, aceso ? C_LATAO : 0, dif_foco == qual ? UI_FOCO : 0, &d);
    int ix = x + 9, cy = y + d + (h - 4) / 2;
    if (icone == 0) { icone_bau(ix, cy - 7); ix += 22; }
    else ix = x + (w - gfx_largura(rot, 1)) / 2;
    int tw = gfx_texto(ix, cy - TXT_H / 2 - 1, rot, c, 1, false);
    if (extra) gfx_texto(ix + tw + 6, cy - TXT_H / 2 + 1, extra, aceso ? C_BARRA : C_TEXTO_M, GFX_FINO, false);
    zona_clique(x, y, w, h, &dif_foco, qual, KEY_ENTER);            // o mouse em cima poe o foco; o clique aperta
    zona_sem_confirmar();                                            // nao comeca nada: um toque basta
}

// O pop-up dos desbloqueios do andar d: o que cada chefe dele abre. Fecha
// com ENTER, ESC, BACKSPACE (o B), I (o X do controle), o botao ou um
// clique em qualquer lugar; as setas nao fazem nada.
static void desenha_desbloqueios(int d)
{
    n_zonas = 0;                                                     // nada por baixo responde ao clique
    n_dicas = 0;
    foco_bx = -1;                                                    // e o ENTER que fecha nao afunda o "Iniciar"
    ui_escurece(0, 0, GFX_W, GFX_H, 150);
    zona_clique(0, 0, GFX_W, GFX_H, NULL, 0, KEY_BKSP);              // o clique em qualquer lugar fecha, como nos outros pop-ups
    int w = 520, h = 254, x = (GFX_W - w) / 2, y = 50 + ui_entra(t_relogio - dif_pop_t, 0, UI_ENTRA_PLACA);
    ui_painel(x, y, w, h);
    icone_bau(x + 14, y + 13);
    char s[64];
    snprintf(s, sizeof s, "Desbloqueios do %s", ANDAR_DIF[d]);
    int wt = gfx_texto(x + 38, y + 6, s, C_OURO, 1, true);
    txt(x + 38 + wt + 10, y + 6, NOME_DIF_CURTO[d], COR_DIF[d], false);
    int ab, tot;
    conta_desbloqueios(d, &ab, &tot);
    snprintf(s, sizeof s, "%d de %d abertos", ab, tot);
    gfx_texto(x + w - 14 - gfx_largura(s, GFX_FINO), y + 8, s, ab == tot && tot ? C_OURO : C_TEXTO_M, GFX_FINO, false);
    gfx_texto(x + 14, y + 32, !tot ? "Este andar não abre nada novo." : ab == tot ? "Tudo já aberto neste andar:" : "Vença os chefes deste andar para abrir:",
              C_TEXTO_M, GFX_FINO, false);
    lista_desbloqueios(d, x + 26, y + 56, x + w - 8, y + h - 44);
    int bw = 100, bh = 26, bx = x + w - 14 - bw, by = y + h - 12 - bh;
    ui_botao(bx, by, bw, bh, "Fechar", 0, mouse_em(bx, by, bw, bh) ? UI_FOCO : 0);
    zona_clique(bx, by, bw, bh, NULL, 0, KEY_BKSP);
    faixa_madeira(GFX_H - FAIXA_H);                                  // o rodape limpo, sem as teclas
}

static void predio_dificuldade(int sel, int aberto);   // predio.inc: o predio e a botoeira
static void desenha_dificuldade(void)
{
    ui_escurece(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, 110);
    faixa_titulo("Dificuldade da run");
    // O predio do cassino com todos os andares; os que ainda nao abriram
    // ficam apagados e trancados. A tela abre no ultimo andar aberto: e nele
    // que quase todo mundo joga, para subir. Os tres botoes sao um menu: as
    // setas passam por eles e ENTER aperta o que esta em foco (o "Iniciar a
    // run", de comeco). O andar so muda depois de "Trocar de andar": pelas
    // setas ou pelo clique num andar (no predio ou na botoeira).
    int nv = perfil()->nivel < N_NIVEIS ? perfil()->nivel : N_NIVEIS;
    int d = dif_cur < 0 ? 0 : dif_cur > nv ? nv : dif_cur;
    predio_dificuldade(d, nv);
    // O que vale no andar escolhido, curto: o que muda nos oponentes fica num
    // "inimigos mais fortes" (o detalhe se descobre jogando); do Nivel 5, que
    // a run nao acaba no Barao; e as moedas do comeco. O que os chefes abrem
    // fica no bau (o pop-up).
    int px = 236, py = 54, pw = 378, ph = 254;
    ui_painel(px, py, pw, ph);
    int wt = gfx_texto(px + 12, py + 8, NOME_DIF[d], COR_DIF[d], 1, true);
    txt(px + 12 + wt + 12, py + 8, ANDAR_DIF[d], C_TEXTO_M, false);
    int ly = py + 38;
    if (d > 0) {
        txt(px + 12, ly, "+", COR_DIF[d], true);
        txt(px + 24, ly, "Inimigos mais fortes.", C_MARFIM, false);
    } else txt(px + 24, ly, PENA_DIF[0], C_MARFIM, false);
    ly += 21;
    if (d >= CORINGA_DESDE) {                        // a run segue depois do Barao (sem dizer quem)
        char s[64];
        snprintf(s, sizeof s, "Um novo chefe surge após %s.", dezembro ? "o Papai Noel" : "o Barão");
        txt(px + 12, ly, "+", C_REAL, true);
        txt(px + 24, ly, s, C_REAL, false);
        ly += 21;
    }
    {
        char m[64];                                  // o mesmo comeco em todo andar
        if (negado_d > nv && t_relogio - negado_t < 1.2f) {   // apertaram um trancado: por que nao abre
            snprintf(m, sizeof m, "Vença uma run no %s para abrir.", NOME_DIF_CURTO[nv]);
            txt(px + 24, ly, m, C_VERMELHO, false);
        } else {
            snprintf(m, sizeof m, "Você começa com %d moedas.", moedas_iniciais(d));
            txt(px + 24, ly, m, C_VERDE, false);
        }
    }
    // Os dois botoes pequenos: o bau (o que este andar desbloqueia) e trocar
    // de andar; embaixo, o grande, que comeca a run.
    int ab, tot;
    conta_desbloqueios(d, &ab, &tot);
    char cnt[16];
    snprintf(cnt, sizeof cnt, "%d/%d", ab, tot);
    int bh_g = 44, bw_g = 250, bw = (pw - 24 - 10) / 2, bh = 30, by = py + ph - 14 - bh_g - 32 - bh;   // o vao cabe o "toque de novo"
    botao_dif(px + 12, by, bw, bh, 0, "Desbloqueios", cnt, false, DIF_DESBLOQ);
    botao_dif(px + 12 + bw + 10, by, bw, bh, 1, dif_escolhe ? "Escolha o andar" : "Trocar de andar", NULL, dif_escolhe, DIF_TROCA);
    botao_iniciar(px + (pw - bw_g) / 2, py + ph - 14 - bh_g, bw_g, bh_g, "Iniciar a run", dif_foco == DIF_INICIAR);
    if (dif_pop) { desenha_desbloqueios(d); return; }
    faixa_madeira(GFX_H - FAIXA_H);                                  // o rodape limpo, sem as teclas
}


