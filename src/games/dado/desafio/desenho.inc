// ---------------------------------------------------------------------------
// Desenho
// ---------------------------------------------------------------------------
// Moeda de ouro: nao e ficha. Disco dourado com borda escura e um brilho.
static void moeda_ic(int x, int y)
{
    gfx_disco(x + 1, y + 2, 7, C_SOMBRA);
    gfx_disco(x, y, 7, C_LATAO_ESC);
    gfx_disco(x, y, 6, C_OURO);
    gfx_disco(x, y, 4, gfx_mistura(C_OURO, C_LATAO_ESC, 30));
    gfx_disco(x - 2, y - 2, 1, gfx_mistura(C_OURO, C_BRANCO, 60));
}

static void moedas_c(int cx, int y, int valor, const char *sufixo, uint16_t c)
{
    char t[48];
    snprintf(t, sizeof t, "%d%s", valor, sufixo);
    int w = gfx_largura(t, 1) + 20, x = cx - w / 2;
    moeda_ic(x + 7, y + 11);
    txt(x + 20, y, t, c, true);
}

// Aro de pontinhos em volta de (x, y).
static void aro_circulo(int x, int y, int r, uint16_t c, int n)
{
    for (int i = 0; i < n; i++) {
        float a = i * 6.2832f / n;
        gfx_rect(x + (int)(cosf(a) * r) - 1, y + (int)(sinf(a) * r) - 1, 2, 2, c);
    }
}

static void pontilhado(int x0, int y0, int x1, int y1, uint16_t c, int esp, int passo)
{
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0);
    int n = (int)(sqrtf(dx * dx + dy * dy) / passo);
    for (int i = 0; i <= n; i++) {
        int x = x0 + (int)(dx * i / (n ? n : 1)), y = y0 + (int)(dy * i / (n ? n : 1));
        gfx_rect(x - esp / 2, y - esp / 2, esp, esp, c);
    }
}

static const char *nome_dif(int d);
static uint16_t cor_dif(int d);
// voltar: a tecla do botao "<" (0: sem botao).
static void faixa_run(int voltar)
{
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    int x0 = voltar ? botao_voltar(voltar) : 18;
    txt(x0, ty, "MODO DESAFIANTE", C_LATAO, true);
    int fim = x0 + gfx_largura("MODO DESAFIANTE", 1);
    if (run.dificuldade) {
        txt(fim + 10, ty, nome_dif(run.dificuldade), cor_dif(run.dificuldade), true);
        fim += 10 + gfx_largura(nome_dif(run.dificuldade), 1);
    }
    char s[48];
    snprintf(s, sizeof s, "%s  ·  %d de %d", SALAO[run.andar], run.andar + 1, DES_ANDARES);
    int cx = GFX_W / 2, meio = gfx_largura(s, 1) / 2;
    if (cx - meio < fim + 18) cx = fim + 18 + meio;
    txt_c(cx, ty, s, COR_SALAO[run.andar], true);
    snprintf(s, sizeof s, "%d", run.moedas);
    int x = GFX_W - 18 - gfx_largura(s, 1);
    txt(x, ty, s, C_OURO, true);
    moeda_ic(x - 12, FAIXA_H / 2);
    // A sua bolsa, junto das suas moedas: no mapa, clicar abre.
    snprintf(s, sizeof s, "%d dados", run.n_col);
    int wb = gfx_largura(s, 1), xb = x - 34 - wb;
    bool clica = fase == F_MAPA, em = clica && mouse_em(xb - 22, 6, wb + 28, FAIXA_H - 12);
    icone_bolsa(xb - 12, FAIXA_H / 2);
    txt(xb, ty, s, em ? C_OURO : C_MARFIM, true);
    if (clica) zona_clique(xb - 22, 6, wb + 28, FAIXA_H - 12, NULL, 0, KEY_TAB);
}

static void rodape_run(const char *s)
{
    int yb = GFX_H - FAIXA_H;
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
    teclas_c(GFX_W / 2, yb + (FAIXA_H - TECLA_H) / 2 + 1, s, C_TEXTO_M);
}

// ---- O mapa -------------------------------------------------------------
static const int MAPA_X[DES_PASSOS] = { 136, 224, 312, 400, 488, 580 };
static const int MAPA_Y[3] = { 92, 142, 192 };
#define MAPA_X0 56
enum { V_FEITO, V_ABERTO, V_LONGE, V_PERDIDO };

// Uma mesa do mapa como ficha de cassino: a cor diz a dificuldade, os pontos
// no meio tambem (1, 2 ou 3); a loja e uma moeda; o chefe, uma ficha maior
// com aro dourado.
static void mesa_no(int x, int y, int tipo, int vis, bool cur)
{
    int r = tipo == NO_CHEFE ? 20 : 15;
    uint16_t c = cor_no(tipo);
    int apaga = vis == V_PERDIDO ? 72 : (vis == V_LONGE ? 38 : (vis == V_FEITO ? 50 : 0));
    uint16_t base = gfx_mistura(c, C_FELTRO, apaga);
    uint16_t marca = gfx_mistura(C_MARFIM, C_FELTRO, apaga);
    if (cur) {
        int rr = r + 6 + (int)(sinf(t_fase * 6) * 1.5f);
        gfx_disco(x, y, rr, C_BRANCO);
        gfx_disco(x, y, rr - 2, C_FELTRO_ESC);
    } else if (vis == V_ABERTO) {
        aro_circulo(x, y, r + 5, gfx_mistura(c, C_FELTRO, 30), 32);
    }
    gfx_disco(x + 1, y + 3, r, C_SOMBRA);
    if (tipo == NO_EVENTO) {                     // ficha lisa com uma interrogacao
        gfx_disco(x, y, r, base);
        for (int i = 0; i < 8; i++) {                // listras da borda, como as fichas
            float a = i * 0.7854f + 0.39f;
            gfx_rect(x + (int)(cosf(a) * (r - 3)) - 1, y + (int)(sinf(a) * (r - 3)) - 1, 3, 3, marca);
        }
        gfx_disco(x, y, r - 6, gfx_mistura(base, C_BARRA, 18));
        txt_c(x, y - 9, "?", marca, true);
    } else if (tipo == NO_LOJA) {                // moeda grande
        gfx_disco(x, y, r, gfx_mistura(C_LATAO_ESC, C_FELTRO, apaga));
        gfx_disco(x, y, r - 2, base);
        gfx_disco(x, y, r - 6, gfx_mistura(base, C_LATAO_ESC, 30));
        gfx_disco(x - 4, y - 4, 2, gfx_mistura(base, C_BRANCO, 55));
        txt_c(x, y - 10, "$", gfx_mistura(C_LATAO_ESC, C_FELTRO, apaga), true);
    } else {
        gfx_disco(x, y, r, base);
        for (int i = 0; i < 8; i++) {                // listras da borda
            float a = i * 0.7854f + 0.39f;
            gfx_rect(x + (int)(cosf(a) * (r - 3)) - 1, y + (int)(sinf(a) * (r - 3)) - 1, 3, 3, marca);
        }
        gfx_disco(x, y, r - 6, gfx_mistura(base, C_BARRA, 30));
        int n = tipo == NO_FACIL ? 1 : (tipo == NO_MEDIO ? 2 : 3);
        for (int k = 0; k < n; k++)
            gfx_disco(x + (int)((k - (n - 1) / 2.0f) * 6), y, tipo == NO_CHEFE ? 2 : 1, marca);
        if (tipo == NO_CHEFE) aro_circulo(x, y, r + 3, gfx_mistura(C_OURO, C_FELTRO, apaga), 40);
    }
    if (vis == V_FEITO) aro_circulo(x, y, r + 3, C_OURO, 28);
}

static int vis_de(int p, int l)
{
    if (p < run.passo) return run.caminho[p] == l ? V_FEITO : V_PERDIDO;
    if (p == run.passo) return escolhivel(l) ? V_ABERTO : V_PERDIDO;
    return V_LONGE;
}

static const char *const NIVEL_IA[] = { "IA simples", "IA atenta", "IA afiada" };
static const char *const PREMIO_TIER[] = { "Prêmio: dados comuns", "Prêmio: dados variados",
                                           "Prêmio: dados raros", "Prêmio: os melhores dados" };

// O quadro de baixo do mapa, em colunas separadas por filetes: quem e (com
// as etiquetas e o premio), o trunfo (nome e regra) e a bolsa dele, com os
// dados agrupados por tipo.
#define INFO_Y  220
#define INFO_H  86
static void info_divisa(int x)
{
    gfx_rect(x, INFO_Y + 8, 1, INFO_H - 16, gfx_mistura(C_PLACA, C_LATAO_ESC, 60));
}

static void info_mesa(int l)
{
    if (!existe(run.passo, l)) return;
    const no_t *no = &run.no[run.passo][l];
    const int x0 = 28, w0 = GFX_W - 56, y0 = INFO_Y;
    painel(y0, INFO_H, w0);
    int cx = x0 + 14, cy = y0 + 6;
    if (no->tipo == NO_EVENTO || no->tipo == NO_LOJA) {
        bool eh_evento = no->tipo == NO_EVENTO;
        const evt_t *e = &EVT[no->perso < EVT_N ? no->perso : 0];
        txt(cx, cy, eh_evento ? "EVENTO" : "LOJA", cor_no(no->tipo), true);
        if (eh_evento) txt(cx, cy + 18, e->nome, C_MARFIM, true);
        txt(cx, cy + 38, "Não é uma partida.", C_TEXTO_M, false);
        info_divisa(x0 + 170);
        if (eh_evento) quebra_linhas(x0 + 184, cy, w0 - 200, 3, e->resumo, C_MARFIM_S);
        else {
            quebra_linhas(x0 + 184, cy, w0 - 200, 2, "Troque moedas de ouro por dados para a sua bolsa.", C_MARFIM_S);
            moedas_c(x0 + 184 + 60, cy + 44, run.moedas, " moedas para gastar", C_OURO);
        }
        return;
    }
    const perso_t *pe = &PERSO[no->perso];
    int tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
    if ((run_amuletos() >> A_CARTOLA & 1) && tier < 3) tier++;             // Cartola: um premio acima

    // Retrato.
    int rx = x0 + 8, ry = y0 + 10, esc = 2, nl = 27;
    gfx_rect(rx - 2, ry - 2, RT_W * esc + 4, 58, no->tipo == NO_CHEFE ? C_OURO : C_LATAO_ESC);
    gfx_rect(rx, ry, RT_W * esc, 54, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(rx, ry, esc, no->perso, t_fase + no->perso * 1.3f, false, 0, nl);

    // Quem e: nome, etiquetas e o premio.
    int xa = rx + RT_W * esc + 10, wa = 140;
    gfx_texto(xa, cy, pe->nome, cor_no(no->tipo), 1, true);
    int xt = etiqueta_caixa(xa, cy + 21, NOME_NO[no->tipo], cor_no(no->tipo));
    xt = etiqueta_caixa(xt, cy + 21, NIVEL_IA[nivel_mesa(no)], C_ACO);
    if (no->tipo == NO_CHEFE && virada_de(no->perso)) {   // a virada: os detalhes na dica dela
        int xv = etiqueta_caixa(xa, cy + 42, "Virada", C_VINHO_CLR);
        dica_zona(xa, cy + 42, xv - xa, 20, DICA_TRUNFO, no->perso);
    } else txt(xa, cy + 46, PREMIO_TIER[tier], C_TEXTO_M, false);
    info_divisa(xa + wa);

    // O trunfo: nome em destaque e a regra, quebrada em linhas.
    int xb = xa + wa + 12, wb = 168;
    txt(xb, cy, pe->trunfo, C_ROSA, true);
    quebra_linhas(xb, cy + 22, wb, 3, pe->regra, C_MARFIM_S);
    info_divisa(xb + wb + 6);

    // A bolsa dele, agrupada por tipo: o mouse em cima diz o que o dado faz.
    int xc = xb + wb + 18, wc = x0 + w0 - 10 - xc;
    peca_t col[MAX_COL];
    int n = monta_oponente(no, run.andar, col);
    char s[24];
    snprintf(s, sizeof s, "Bolsa · %d dados", n);
    txt(xc, cy + 4, s, C_TEXTO_M, false);
    int tipos[MAX_COL], conta[MAX_COL], nt = 0;
    for (int i = 0; i < n; i++) {
        int k = 0;
        while (k < nt && tipos[k] != col[i].tipo) k++;
        if (k == nt) { tipos[nt] = col[i].tipo; conta[nt++] = 0; }
        conta[k]++;
    }
    int gx = xc, gy = cy + 33;
    for (int k = 0; k < nt; k++) {
        char q[16] = "";
        if (conta[k] > 1) snprintf(q, sizeof q, "×%d", conta[k]);
        int w = 22 + (q[0] ? gfx_largura(q, GFX_FINO) + 1 : 0) + 6;
        if (gx + w > xc + wc) { gx = xc; gy += 25; }
        if (gy > y0 + INFO_H - 10) break;
        // O dado parado, como na mao: forma, aro e quantos lados.
        peca_t pc = { (uint8_t)tipos[k], 0 };
        peca_parada((float)(gx + 10), (float)gy, 10, pc, 0);
        if (q[0]) gfx_texto(gx + 22, gy - 12, q, C_MARFIM_S, GFX_FINO, false);
        dica_zona(gx - 2, gy - 12, w, 24, DICA_DADO, tipos[k]);
        gx += w;
    }
}

static void desenha_mapa(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 50);
    faixa_run(KEY_ESC);
    rodape_run("setas escolhem  ·  ENTER senta à mesa  ·  TAB bolsa  ·  ESC menu");

    for (int p = 0; p < DES_PASSOS; p++) {
        char s[24];
        if (p < DES_PASSOS - 1) snprintf(s, sizeof s, "mesa %d", p + 1);
        else snprintf(s, sizeof s, "chefe");
        uint16_t c = p == run.passo ? COR_SALAO[run.andar] : (p < run.passo ? C_FELTRO2 : C_TEXTO_M);
        txt_c(MAPA_X[p], 52, s, c, false);
    }
    // Entrada do salao e as ligacoes.
    int ey = MAPA_Y[1];
    for (int l = 0; l < 3; l++) {
        if (!existe(0, l)) continue;
        bool feito = run.passo > 0 && run.caminho[0] == l;
        uint16_t c = feito ? C_OURO : (run.passo == 0 ? C_TEXTO_M : C_FELTRO2);
        pontilhado(MAPA_X0, ey, MAPA_X[0], MAPA_Y[l], c, feito ? 3 : 2, 6);
    }
    for (int p = 0; p < DES_PASSOS - 1; p++)
        for (int l = 0; l < 3; l++) {
            if (!existe(p, l)) continue;
            for (int m = 0; m < 3; m++) {
                if (!(run.no[p][l].liga >> m & 1) || !existe(p + 1, m)) continue;
                bool feito = p + 1 < run.passo && run.caminho[p] == l && run.caminho[p + 1] == m;
                bool aberto = p + 1 == run.passo && run.caminho[p] == l;
                uint16_t c = feito ? C_OURO : (aberto ? C_TEXTO_M : C_FELTRO2);
                pontilhado(MAPA_X[p], MAPA_Y[l], MAPA_X[p + 1], MAPA_Y[m], c, feito ? 3 : 2, 6);
            }
        }
    gfx_disco(MAPA_X0, ey, 6, C_LATAO_ESC);
    gfx_disco(MAPA_X0, ey, 4, C_LATAO);

    for (int p = 0; p < DES_PASSOS; p++)
        for (int l = 0; l < 3; l++) {
            if (!existe(p, l)) continue;
            const no_t *no = &run.no[p][l];
            int vis = vis_de(p, l);
            bool cur = p == run.passo && l == des_cur;
            mesa_no(MAPA_X[p], MAPA_Y[l], no->tipo, vis, cur);
            int r = no->tipo == NO_CHEFE ? 20 : 15;
            if (vis == V_ABERTO) zona_clique(MAPA_X[p] - r - 6, MAPA_Y[l] - r - 6, 2 * r + 12, 2 * r + 12,
                                             &des_cur, l, KEY_ENTER);
        }
    info_mesa(des_cur);
    // Os amuletos da run, no canto de cima.
    for (int a = 0, n = 0; a < A_N; a++)
        if (run_amuletos() >> a & 1) {
            int ax = 30 + (n % 4) * 19, ay = 66 + (n / 4) * 19;
            desenha_amuleto(ax, ay, 8, a, false);
            dica_zona(ax - 9, ay - 9, 18, 18, DICA_AMULETO, a);
            n++;
        }
}

// ---- Loja ---------------------------------------------------------------
static void desenha_loja_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    faixa_run(0);
    bool ini = run.estado == R_LOJA_INI;
    txt_c2(GFX_W / 2, MESA_Y0 + 2, ini ? "LOJA DA ENTRADA" : "LOJA DO SALÃO", C_OURO);
    if (ini) txt_c(GFX_W / 2, MESA_Y0 + 44, "A run começa: gaste bem, a próxima loja pode demorar.", C_MARFIM_S, false);
    moedas_c(GFX_W / 2, MESA_Y0 + (ini ? 66 : 54), run.moedas, " moedas", C_OURO);
    bool tem_am_loja = run.am_loja < A_N;
    int nc = N_LOJA + (tem_am_loja ? 1 : 0);
    for (int i = 0; i < nc; i++) {
        int x = GFX_W / 2 + (int)((i - (nc - 1) / 2.0f) * (nc > N_LOJA ? 96 : 108)), yb = 142;
        int wc = nc > N_LOJA ? 88 : 96;
        bool am = i == N_LOJA;
        int pr = am ? preco_am() : preco_run(run.loja[i].tipo);
        bool ja = am ? run.am_levou : run.levou[i], cur = i == cursor, da = run.moedas >= pr;
        uint16_t cr = am ? C_OURO : cor_rar(run.loja[i].tipo);
        ui_cartao(x - wc / 2, yb, wc, 92, cur, cr);
        float bob = cur && !ja ? sinf(t_fase * 4) * 3 : 0;
        if (am) desenha_amuleto(x, yb + 32 + (int)bob, 20, run.am_loja, false);
        else peca_parada((float)x, yb + 32 + bob, 22, run.loja[i], 0);
        if (ja) {
            txt_c(x, yb + 64, "comprado", C_TEXTO_M, false);
            gfx_rect_alfa(x - wc / 2 + 1, yb + 1, wc - 2, 90, C_SOMBRA, 120);
        } else {
            moedas_c(x, yb + 64, pr, "", da ? C_OURO : C_VINHO_CLR);
        }
        zona_clique(x - wc / 2, yb, wc, 92, &cursor, i, KEY_ENTER);
    }
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "sair da loja", KEY_TAB);
    if (cursor == N_LOJA && tem_am_loja) {
        const amuleto_t *a = &AMULETO[run.am_loja];
        txt_c(GFX_W / 2, 240, a->nome, a->cor, true);
        txt_c(GFX_W / 2, 240 + TXT_H + 2, a->desc, C_MARFIM_S, false);
        if (!run.am_levou && run.moedas < preco_am()) txt_c(GFX_W / 2, 284, "moedas insuficientes", C_VINHO_CLR, true);
    } else if (cursor < N_LOJA) {
        dado_e_poder(240, run.loja[cursor].tipo, true);
        if (!run.levou[cursor] && run.moedas < preco_run(run.loja[cursor].tipo))
            txt_c(GFX_W / 2, 284, "moedas insuficientes", C_VINHO_CLR, true);
    }
    rodape_run("setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB sai da loja");
}

// ---- A bolsa ------------------------------------------------------------
#define GRADE_B 12
static void desenha_bolsa_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run(KEY_TAB);
    txt_c2(GFX_W / 2, MESA_Y0 + 2, "SUA BOLSA", C_LATAO);
    char s[64];
    snprintf(s, sizeof s, "%d dados", run.n_col);
    txt_c(GFX_W / 2, MESA_Y0 + 44, s, C_MARFIM_S, false);
    for (int i = 0; i < run.n_col; i++) {
        int lin = i / GRADE_B, col = i % GRADE_B;
        int na_linha = (run.n_col - lin * GRADE_B) < GRADE_B ? run.n_col - lin * GRADE_B : GRADE_B;
        int x = GFX_W / 2 + (int)((col - (na_linha - 1) / 2.0f) * 44);
        int y = 138 + lin * 40;
        if (i == cursor) ui_selecao(x - 20, y - 20, 40, 40, C_OURO);
        peca_parada((float)x, (float)y, 15, run.col[i], 0);
        if (i != cursor) marca_rar(x, y + 20, run.col[i].tipo);
        zona_clique(x - 20, y - 20, 40, 40, &cursor, i, 'i');   // clique abre os detalhes
    }
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar ao mapa", KEY_TAB);
    // Os amuletos da run, nas laterais: seis de cada lado.
    int na = 0;
    for (int a = 0; a < A_N; a++) {
        if (!(run_amuletos() >> a & 1)) continue;
        int x = na < 6 ? 34 : GFX_W - 34, y = 128 + (na % 6) * 26;
        int idx = run.n_col + na;
        if (idx == cursor) gfx_disco(x, y, 14, C_LATAO);
        desenha_amuleto(x, y, 11, a, false);
        zona_clique(x - 12, y - 12, 24, 24, &cursor, idx, 0);
        if (idx == cursor) {
            txt_c(GFX_W / 2, 262, AMULETO[a].nome, AMULETO[a].cor, true);
            txt_c(GFX_W / 2, 262 + TXT_H + 2, AMULETO[a].desc, C_MARFIM_S, false);
        }
        na++;
    }
    if (run.n_col && cursor < run.n_col) dado_e_poder(262, run.col[cursor].tipo, true);
    rodape_run("setas escolhem  ·  I detalhes  ·  TAB volta ao mapa");
}

// ---- Fim da run ---------------------------------------------------------
static void desenha_liberou(void);
static void desenha_fim_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run(0);
    bool abriu = des_venceu_run && des_liberou > 0;
    painel(CENTRO - 84, abriu ? 208 : 168, 500);
    char s[96];
    if (des_venceu_run) {
        txt_sombra_c(GFX_W / 2, CENTRO - 74, "VITÓRIA", C_OURO, 3);
        txt_c(GFX_W / 2, CENTRO - 6, "Você venceu O BARÃO e fechou o cassino.", C_MARFIM, true);
        desenha_liberou();
    } else {
        txt_sombra_c(GFX_W / 2, CENTRO - 74, "FIM DA RUN", C_VINHO_CLR, 3);
        if (des_perdeu_para >= 0)
            snprintf(s, sizeof s, "%s venceu você no %s.", PERSO[des_perdeu_para].nome, SALAO[des_perdeu_andar]);
        else snprintf(s, sizeof s, "A casa ficou com a mesa.");
        txt_c(GFX_W / 2, CENTRO - 6, s, C_MARFIM, true);
    }
    snprintf(s, sizeof s, "%d %s  ·  %d dados na bolsa  ·  %d moedas", run.vitorias,
             run.vitorias == 1 ? "partida vencida" : "partidas vencidas", run.n_col, run.moedas);
    txt_c(GFX_W / 2, CENTRO + 22, s, C_TEXTO_M, false);
    teclas_c(GFX_W / 2, CENTRO + (abriu ? 98 : 58), des_venceu_run ? "ENTER segue" : "ENTER volta ao menu", C_MARFIM_S);
}

// Os creditos: so depois de vencer uma run, de qualquer andar. Cada linha
// entra um instante depois da anterior.
static void desenha_creditos(void)
{
    static const struct { const char *s; int tipo; } L[] = {
        { "Dado em Casa, criado por ERICZ", 0 },
        { "Sons", 1 },
        { "Dados, feltro e madeira: dice-box-threejs, de 3D Dice (MIT)", 2 },
        { "Teclado: jRhodes3d, um Rhodes de 1977 gravado por J. Learman", 2 },
        { "Baixo e sax: Karoryfer, coleção tonejs-instruments (CC BY 3.0)", 2 },
        { "Vibrafone, celesta e outros: FluidR3 GM, de Frank Wen (CC BY 3.0)", 2 },
        { "Fontes", 1 },
        { "Jersey 10, Tiny5 e Bitcount Prop Single (SIL Open Font License)", 2 },
        { "Obrigado por jogar.", 3 },
    };
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run(0);
    const int y0 = MESA_Y0 + 6;                                 // cabe todo dentro do feltro
    painel(y0, MESA_Y1 - 6 - y0, 520);
    txt_sombra_c(GFX_W / 2, y0 + 8, "CRÉDITOS", C_OURO, 2);
    int y = y0 + 48;
    for (int i = 0; i < (int)(sizeof L / sizeof L[0]); i++) {
        if (L[i].tipo == 1 || L[i].tipo == 3) y += 4;          // folga antes de cada bloco
        if (t_fase > 0.2f + i * 0.08f) {
            switch (L[i].tipo) {
            case 0: txt_c(GFX_W / 2, y, L[i].s, C_MARFIM, true);  break;
            case 1: txt_c(GFX_W / 2, y, L[i].s, C_LATAO, true);   break;
            case 2: txt_c(GFX_W / 2, y, L[i].s, C_TEXTO_M, false); break;
            default: txt_c(GFX_W / 2, y, L[i].s, C_OURO, true);   break;
            }
        }
        y += L[i].tipo == 2 ? LINHA_FINA : L[i].tipo == 1 ? 18 : 20;
    }
    teclas_c(GFX_W / 2, y + 10, "ENTER volta ao menu", C_MARFIM_S);
}

// Tela do fim de uma partida da run (por cima da mesa, como a normal).
static void desenha_fim_partida_run(void)
{
    // O oponente, grande, a esquerda, com o que disse no fim.
    if (fala_perso >= 0) {
        int esc = 3, rx = 22, ry = CENTRO - 70;
        bool chefe = perso_chefe(fala_perso);
        gfx_rect(rx - 2, ry - 2, RT_W * esc + 4, RT_H * esc + 4, chefe ? C_OURO : C_LATAO_ESC);
        gfx_rect(rx, ry, RT_W * esc, RT_H * esc, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
        retrato(rx, ry, esc, fala_perso, t_fase + fala_perso * 1.3f, t_fala > 0.3f, 0, RT_H);
        dica_zona(rx, ry, RT_W * esc, RT_H * esc, DICA_TRUNFO, fala_perso);
        if (fala_txt[0]) txt_c(rx + RT_W * esc / 2, ry + RT_H * esc + 6, PERSO[fala_perso].nome, C_ROSA, true);
    }
    painel(CENTRO - 70, 150, 460);
    char s[80];
    if (venc_partida == 0) {
        txt_sombra_c(GFX_W / 2, CENTRO - 58, "VOCÊ VENCEU", C_LATAO, 3);
        snprintf(s, sizeof s, "%d fichas viram %d moedas de ouro", run.ganho, run.ganho);
        if (run.juros) {
            char j2[24];
            snprintf(j2, sizeof j2, "  ·  Cofre +%d", run.juros);
            strncat(s, j2, sizeof s - strlen(s) - 1);
        }
        if (run.taca) {
            char j2[24];
            snprintf(j2, sizeof j2, "  ·  Taça +%d", run.taca);
            strncat(s, j2, sizeof s - strlen(s) - 1);
        }
        moedas_c(GFX_W / 2, CENTRO + 6, run.ganho + run.juros + run.taca, " moedas", C_OURO);
        txt_c(GFX_W / 2, CENTRO + 32, s, C_TEXTO_M, false);
        if (des_marco >= 0) {                      // primeira vitoria contra este chefe
            char m[64];
            snprintf(m, sizeof m, "Abriu o %s para as próximas runs", TIPO[des_marco].nome);
            int w = gfx_largura(m, 1);
            desenha_dado((float)(GFX_W / 2 - w / 2 - 12), (float)(CENTRO + 56), 8, des_marco, 0, 0, 0, 0, false);
            txt(GFX_W / 2 - w / 2 + 2, CENTRO + 50, m, C_OURO, true);
            teclas_c(GFX_W / 2, CENTRO + 72, "ENTER escolhe o prêmio", C_MARFIM_S);
        } else {
            teclas_c(GFX_W / 2, CENTRO + 58, "ENTER escolhe o prêmio", C_MARFIM_S);
        }
    } else {
        txt_sombra_c(GFX_W / 2, CENTRO - 58, venc_partida == 2 ? "EMPATE" : "DERROTA", C_VINHO_CLR, 3);
        txt_c(GFX_W / 2, CENTRO + 6, venc_partida == 2 ? "Empate não passa de mesa: a run termina aqui."
                                                       : "A run termina aqui.", C_MARFIM, true);
        snprintf(s, sizeof s, "%s %d  x  %d %s", J[0].nome, J[0].fichas, J[1].fichas, J[1].nome);
        txt_c(GFX_W / 2, CENTRO + 32, s, C_TEXTO_M, false);
        teclas_c(GFX_W / 2, CENTRO + 58, "ENTER segue", C_MARFIM_S);
    }
    if (fala_perso >= 0 && fala_txt[0]) {                 // a ultima palavra dele
        char q[80];
        snprintf(q, sizeof q, "\"%s\"", fala_txt);
        txt_c(GFX_W / 2, CENTRO + 88, q, C_ROSA, false);
    }
}
