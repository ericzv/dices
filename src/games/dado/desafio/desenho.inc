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

// A mesma moeda em qualquer tamanho (a que acabou de entrar na fileira pula).
static void moeda_r(int x, int y, float r)
{
    int R = (int)(r + 0.5f);
    if (R < 1) return;
    gfx_disco(x + 1, y + 2, R, C_SOMBRA);
    gfx_disco(x, y, R, C_LATAO_ESC);
    gfx_disco(x, y, R - 1, C_OURO);
    if (R > 3) gfx_disco(x, y, R - 3, gfx_mistura(C_OURO, C_LATAO_ESC, 30));
    gfx_disco(x - R / 3, y - R / 3, R > 5 ? 1 : 0, gfx_mistura(C_OURO, C_BRANCO, 60));
}

// A mola de saida: passa um pouco do lugar e volta (0 -> 1).
static float sai_mola(float q)
{
    if (q <= 0) return 0;
    if (q >= 1) return 1;
    float u = q - 1;
    return 1 + 2.70158f * u * u * u + 1.70158f * u * u;
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
    faixa_madeira(0);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    int x0 = voltar ? botao_voltar(voltar) : 18;
    txt(x0, ty, "MODO DESAFIANTE", C_LATAO, true);
    int fim = x0 + gfx_largura("MODO DESAFIANTE", 1);
    if (run.dificuldade) {
        txt(fim + 10, ty, nome_dif(run.dificuldade), cor_dif(run.dificuldade), true);
        fim += 10 + gfx_largura(nome_dif(run.dificuldade), 1);
    }
    char s[48];
    if (run.andar >= DES_ANDARES) snprintf(s, sizeof s, "%s  ·  final", SALAO[run.andar]);
    else snprintf(s, sizeof s, "%s  ·  %d de %d", SALAO[run.andar], run.andar + 1, DES_ANDARES);
    int cx = GFX_W / 2, meio = gfx_largura(s, 1) / 2;
    if (cx - meio < fim + 18) cx = fim + 18 + meio;
    txt_c(cx, ty, s, COR_SALAO[run.andar], true);
    // As moedas contam (andamento.inc): subindo, o numero acende; gastando, escurece.
    snprintf(s, sizeof s, "%d", moedas_vis());
    int x = GFX_W - 18 - gfx_largura(s, 1);
    uint16_t cm = C_OURO;
    if (moedas_ok && moedas_cont.pulo > 0)
        cm = gfx_mistura(C_OURO, moedas_cont.sinal > 0 ? C_BRANCO : C_VINHO_CLR, (int)(moedas_cont.pulo * 70));
    txt(x, ty - (moedas_ok && moedas_cont.pulo > 0.4f ? 1 : 0), s, cm, true);
    moeda_ic(x - 12, FAIXA_H / 2);
    // A sua bolsa, junto das suas moedas: no mapa, clicar abre. O dado
    // comprado so conta quando chega nela, e ela pula.
    snprintf(s, sizeof s, "%d dados", run.n_col - bolsa_voando);
    int wb = gfx_largura(s, 1), xb = x - 34 - wb;
    bool clica = fase == F_MAPA, pop = !clica && fase != F_RBOLSA && bolsa_pop < 0;
    bool em = (clica || pop) && mouse_em(xb - 22, 6, wb + 28, FAIXA_H - 12);
    int pulo = t_bolsa_pulo > 0 ? (int)(sinf(t_bolsa_pulo * 3.1416f) * 4) : 0;
    faixa_bolsa_x = xb - 12;
    faixa_bolsa_y = FAIXA_H / 2;
    icone_da_run(xb - 12, FAIXA_H / 2 - pulo);
    txt(xb, ty - (pulo > 1), s, t_bolsa_pulo > 0.3f ? C_OURO : (em ? C_OURO : C_MARFIM), true);
    if (clica) zona_clique(xb - 22, 6, wb + 28, FAIXA_H - 12, NULL, 0, KEY_TAB);
    else if (fase != F_RBOLSA) {                 // nas outras telas, o pop-up (bolsa.inc)
        zona_clique(xb - 22, 6, wb + 28, FAIXA_H - 12, NULL, BOLSA_RUN, TECLA_BOLSA);
        bolsas_vistas |= 1u << BOLSA_RUN;
    }
}

static void rodape_run(const char *s)
{
    int yb = GFX_H - FAIXA_H;
    faixa_madeira(yb);
    teclas_c(GFX_W / 2, yb + (FAIXA_H - TECLA_H) / 2 + 1, s, C_TEXTO_M);
}

// ---- O salao -------------------------------------------------------------
// O nome no cartao: na fonte fina, ou na miuda se nao couber.
static void nome_no_cartao(int cx, int y, const char *s, uint16_t c, int w)
{
    if (gfx_largura(s, GFX_FINO) <= w) txt_c(cx, y, s, c, false);
    else gfx_texto(cx - gfx_largura(s, GFX_MIUDO) / 2, y + 2, s, c, GFX_MIUDO, false);
}

// As quatro mesas do salao em fila, da esquerda para a direita: o retrato de
// quem senta em cada uma, a dificuldade e quanto ela paga. As vencidas
// apagam, com o visto; a da vez pulsa, e a ficha do jogador anda ate ela.
static const int SALAO_X[DES_PASSOS] = { 104, 248, 392, 536 };
static const int SALAO_X3[3] = { 150, 320, 490 };   // o salao de tres mesas
#define SALAO_BASE 139                   // onde os retratos assentam
#define SALAO_LINHA 112                  // a altura do caminho entre eles

static int salao_x(int p)
{
    if (run.andar == DES_ANDAR_FINAL) return GFX_W / 2;
    return mesas_do_salao() == 3 && p < 3 ? SALAO_X3[p] : SALAO_X[p];
}
static int salao_esc(int p) { return run.no[p].tipo == NO_CHEFE ? 3 : 2; }
static int salao_topo(int p) { return SALAO_BASE - RT_H * salao_esc(p); }

// O quadro de baixo, em colunas separadas por filetes: quem e (com as
// etiquetas e quanto paga), o trunfo (nome e regra) e a bolsa dele, com os
// dados agrupados por tipo.
static const char *const NIVEL_IA[] = { "IA simples", "IA atenta", "IA afiada" };
#define INFO_Y  220
#define INFO_H  86
static void info_divisa(int x)
{
    gfx_rect(x, INFO_Y + 8, 1, INFO_H - 16, gfx_mistura(C_PLACA, C_LATAO_ESC, 60));
}

static void info_mesa(int p)
{
    if (p < 0 || p >= mesas_do_salao()) return;
    const no_t *no = &run.no[p];
    const int x0 = 28, w0 = GFX_W - 56, y0 = INFO_Y;
    painel(y0, INFO_H, w0);
    int cy = y0 + 6;
    const perso_t *pe = &PERSO[no->perso];

    // Retrato.
    int rx = x0 + 8, ry = y0 + 10, esc = 2, nl = 27;
    gfx_rect(rx - 2, ry - 2, RT_W * esc + 4, 58, no->tipo == NO_CHEFE ? C_OURO : C_LATAO_ESC);
    gfx_rect(rx, ry, RT_W * esc, 54, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(rx, ry, esc, no->perso, t_fase + no->perso * 1.3f, false, 0, nl);

    // Quem e: nome, etiquetas e quanto paga.
    int xa = rx + RT_W * esc + 10, wa = 140;
    gfx_texto(xa, cy, pe->nome, cor_no(no->tipo), 1, true);
    int xt = etiqueta_caixa(xa, cy + 21, NOME_NO[no->tipo], cor_no(no->tipo));
    etiqueta_caixa(xt, cy + 21, NIVEL_IA[nivel_mesa(no)], C_ACO);
    if (no->tipo == NO_CHEFE && virada_de(no->perso)) {   // a virada: os detalhes na dica dela
        int xv = etiqueta_caixa(xa, cy + 42, "Virada", C_VINHO_CLR);
        dica_zona(xa, cy + 42, xv - xa, 20, DICA_TRUNFO, no->perso);
    } else {
        int vale = moedas_mesa(p);
        bool paga = vale && !(run.andar == DES_ANDARES - 1 && no->tipo == NO_CHEFE && !com_coringa());
        char v[32];
        if (paga) {
            snprintf(v, sizeof v, "vale %d", vale);
            moeda_ic(xa + 6, cy + 54);
            txt(xa + 16, cy + 44, v, C_OURO, false);
        } else txt(xa, cy + 44, "a última mesa da run", C_TEXTO_M, false);
    }
    info_divisa(xa + wa);

    // O trunfo: nome em destaque e a regra, quebrada em linhas. Sem trunfo
    // nesta dificuldade, a coluna nem aparece: a bolsa vem para o lugar.
    int xb = xa + wa + 12, wb = 168, xc = xb;
    if (trunfo_ativo(no->perso)) {
        txt(xb, cy, pe->trunfo, C_ROSA, true);
        quebra_linhas(xb, cy + 22, wb, 3, regra_perso(no->perso), C_MARFIM_S);
        info_divisa(xb + wb + 6);
        xc = xb + wb + 18;
    }

    // A bolsa dele, agrupada por tipo: o mouse em cima diz o que o dado faz.
    int wc = x0 + w0 - 10 - xc;
    peca_t col[MAX_COL];
    int passo = run.passo;
    run.passo = (uint8_t)p;                      // a bolsa de cada mesa sai do passo dela
    int n = monta_oponente(no, run.andar, col);
    run.passo = (uint8_t)passo;
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
        peca_t pc = { .tipo = (uint8_t)tipos[k], .fixo = 0 };
        peca_parada((float)(gx + 10), (float)gy, 10, pc, 0);
        if (q[0]) gfx_texto(gx + 22, gy - 12, q, C_MARFIM_S, GFX_FINO, false);
        dica_zona(gx - 2, gy - 12, w, 24, DICA_DADO, tipos[k]);
        gx += w;
    }
}

// A ficha do jogador: na cor dele, com o aro de marfim.
static void ficha_jogador(int x, int y)
{
    gfx_disco(x + 1, y + 2, 8, C_SOMBRA);
    gfx_disco(x, y, 8, C_LATAO_ESC);
    gfx_disco(x, y, 7, C_LATAO);
    for (int i = 0; i < 6; i++) {
        float a = i * 1.0472f + 0.5f;
        gfx_rect(x + (int)(cosf(a) * 5.5f) - 1, y + (int)(sinf(a) * 5.5f) - 1, 2, 2, C_MARFIM);
    }
    gfx_disco(x, y, 3, C_OURO);
    gfx_pixel(x - 1, y - 1, C_BRANCO);
}

// Uma mesa do salao: o retrato na moldura, o nome, a dificuldade e quanto paga.
static void salao_mesa(int p)
{
    const no_t *no = &run.no[p];
    int x = salao_x(p), esc = salao_esc(p), w = RT_W * esc, h = RT_H * esc, y = salao_topo(p);
    bool feita = p < run.passo, da_vez = p == run.passo, chefe = no->tipo == NO_CHEFE;
    uint16_t moldura = chefe ? C_OURO : (feita ? C_FELTRO2 : C_LATAO_ESC);
    if (da_vez) {                                // a da vez: um aro que pulsa
        int b = 4 + (int)(sinf(t_fase * 5) * 1.5f + 1.5f);
        gfx_rect(x - w / 2 - b, y - b, w + 2 * b, h + 2 * b, gfx_mistura(COR_SALAO[run.andar], C_FELTRO, 35));
    }
    if (p == des_cur) ui_selecao(x - w / 2 - 9, y - 9, w + 18, h + 74, C_OURO);
    uint16_t rm[4];                              // a moldura: metal em rampa, com contorno
    ui_rampa(moldura, feita, rm);
    ui_contorno(x - w / 2 - 3, y - 3, w + 6, h + 6, C_BARRA);
    ui_aro(x - w / 2 - 2, y - 2, w + 4, h + 4, 2, rm, UI_VIVO);
    gfx_rect(x - w / 2, y, w, h, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(x - w / 2, y, esc, no->perso, t_fase + no->perso * 1.3f, false, 0, RT_H);
    if (feita) {                                 // vencida: apaga e leva o visto
        gfx_rect_alfa(x - w / 2, y, w, h, C_SOMBRA, 150);
        gfx_linha_grossa(x - 9, y + h / 2, x - 2, y + h / 2 + 8, 4, C_OURO);
        gfx_linha_grossa(x - 2, y + h / 2 + 8, x + 12, y + h / 2 - 10, 4, C_OURO);
    }
    uint16_t cn = feita ? C_TEXTO_M : cor_no(no->tipo);
    nome_no_cartao(x, SALAO_BASE + 6, PERSO[no->perso].nome, cn, 130);
    const char *tag = NOME_NO[no->tipo];
    if (!feita) gfx_texto(x - gfx_largura(tag, GFX_FINO) / 2, SALAO_BASE + 22, tag, cn, GFX_FINO, false);
    // Quanto paga: a moeda e o numero (o fim da run nao paga).
    int vale = moedas_mesa(p);
    if (run.andar == DES_ANDARES - 1 && chefe && !com_coringa()) vale = 0;
    char v[16];
    if (feita) snprintf(v, sizeof v, "vencida");
    else if (vale) snprintf(v, sizeof v, "+%d", vale);
    else snprintf(v, sizeof v, "final");
    int wv = gfx_largura(v, 1) + (vale && !feita ? 14 : 0), vx = x - wv / 2;
    if (vale && !feita) { moeda_ic(vx + 6, SALAO_BASE + 52); vx += 14; }
    txt(vx, SALAO_BASE + (feita ? 26 : 42), v, feita ? C_TEXTO_M : (vale ? C_OURO : C_ROSA), !feita);
    zona_clique(x - w / 2 - 8, y - 8, w + 16, h + 72, &des_cur, p, da_vez ? KEY_ENTER : 0);
}

static void desenha_mapa(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 50);
    faixa_run(KEY_ESC);
    rodape_run("setas veem as mesas  ·  ENTER senta à mesa  ·  TAB bolsa  ·  ESC menu");
    int n = mesas_do_salao();
    // O caminho entre as mesas: dourado no que ja foi andado.
    for (int p = 0; p + 1 < n; p++) {
        int x0 = salao_x(p) + RT_W * salao_esc(p) / 2 + 6, x1 = salao_x(p + 1) - RT_W * salao_esc(p + 1) / 2 - 6;
        bool feito = p + 1 <= run.passo;
        pontilhado(x0, SALAO_LINHA, x1, SALAO_LINHA, feito ? C_OURO : C_TEXTO_M, feito ? 3 : 2, 6);
    }
    for (int p = 0; p < n; p++) salao_mesa(p);
    // A ficha do jogador: no caminho, logo antes da mesa da vez; vindo da
    // vencida, anda pelo caminho em dois pulos.
    int px = salao_x(run.passo) - RT_W * salao_esc(run.passo) / 2 - 14, py = SALAO_LINHA;
    if (t_mapa_anda > 0 && run.passo > 0) {
        float q = 1 - t_mapa_anda / 0.7f;
        if (q < 0) q = 0;
        int ax = salao_x(run.passo - 1) + RT_W * salao_esc(run.passo - 1) / 2 + 14;
        float s = q * q * (3 - 2 * q);
        px = ax + (int)((px - ax) * s);
        py -= (int)(fabsf(sinf(q * 6.2832f)) * 16);
    } else py += (int)(sinf(t_fase * 4) * 2) - 1;
    ficha_jogador(px, py);
    info_mesa(des_cur);
    // Os amuletos da run, no canto de cima.
    for (int a = 0, k = 0; a < A_N; a++)
        if (run_amuletos() >> a & 1) {
            int ax = 30 + k * 19, ay = 66;
            desenha_amuleto(ax, ay, 8, a, false);
            dica_zona(ax - 9, ay - 9, 18, 18, DICA_AMULETO, a);
            k++;
        }
}

// ---- A bolsa ------------------------------------------------------------
// 12 por linha, tres linhas antes de encolher.
#define BOLSA_Y 118                  // o alto da grade
static grade_t grade_bolsa(int n) { return grade_cabe(n, 12, 40, 120); }
static void desenha_bolsa_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run(KEY_TAB);
    txt_c2(GFX_W / 2, MESA_Y0 + 2, "SUA BOLSA", C_LATAO);
    char s[64];
    snprintf(s, sizeof s, "%d dados", run.n_col);
    txt_c(GFX_W / 2, MESA_Y0 + 44, s, C_MARFIM_S, false);
    grade_t g = grade_bolsa(run.n_col);
    float f = g.f;
    int sel = (int)(20 * f);
    for (int i = 0; i < run.n_col; i++) {
        int lin = i / g.por, col = i % g.por;
        int na_linha = (run.n_col - lin * g.por) < g.por ? run.n_col - lin * g.por : g.por;
        int x = GFX_W / 2 + (int)((col - (na_linha - 1) / 2.0f) * 44 * f);
        int y = BOLSA_Y + (int)((lin + 0.5f) * 40 * f);
        if (i == cursor) ui_selecao(x - sel, y - sel, 2 * sel, 2 * sel, C_OURO);
        if (i == cursor) brilho_passa = mola_brilho();
        peca_parada((float)x, (float)(y - (i == cursor ? mola_px() : 0)), 15 * f, run.col[i], 0);
        if (i != cursor) marca_rar(x, y + sel, run.col[i].tipo);
        zona_clique(x - sel, y - sel, 2 * sel, 2 * sel, &cursor, i, 'i');   // clique abre os detalhes
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
    if (run.n_col && cursor < run.n_col) {
        bool tem = ad_quantos(&run.col[cursor]) > 0;      // com adesivos: uma linha deles embaixo
        dado_e_poder(tem ? 252 : 262, run.col[cursor].tipo, true);
        if (tem) ad_linha(GFX_W / 2, 294, &run.col[cursor]);
    }
    rodape_run("setas escolhem  ·  I detalhes  ·  TAB volta ao mapa");
}

// ---- Fim da run ---------------------------------------------------------
static int nov_quantas(void);             // as novidades que a run abriu (novidades.inc)
static void nov_linha_fim(int y);
// ---- O fim da run ----------------------------------------------------------
// A bolsa em leque: os dados num arco aberto para cima, sobrepostos como
// cartas na mao, cada um saindo do meio no seu tempo (t). O dado do meio fica
// em (cx, topo) e os das pontas caem; o arco abre o que a bolsa pede, ate
// abrir de todo no raio R. col: as pecas, com os adesivos; sem elas, so os
// tipos (a foto do historico). Ate LEQUE_MAX; os outros viram "+N" na ponta.
#define LEQUE_MAX 21
static float leque_abre(int n, int R, float r)          // o meio-arco, em radianos
{
    int m = n < LEQUE_MAX ? n : LEQUE_MAX;
    return m > 1 ? fminf(1.1f, (m - 1) * 1.7f * r / (2 * R)) : 0;
}
static int leque_cai(int n, int R, float r) { return (int)(0.45f * R * (1 - cosf(leque_abre(n, R, r)))); }
static void leque_da_bolsa(int cx, int topo, int R, float r, const peca_t *col, const uint8_t *tipos, int n, float t)
{
    int m = n < LEQUE_MAX ? n : LEQUE_MAX;
    float abre = leque_abre(n, R, r);
    for (int i = 0; i < m; i++) {
        float q = (t - i * 0.04f) / 0.28f;
        if (q <= 0) continue;
        if (q > 1) q = 1;
        float e = q * q * (3 - 2 * q), a = m > 1 ? -abre + 2 * abre * i / (m - 1) : 0;
        peca_t pc = col ? col[i] : (peca_t){ .tipo = tipos[i] };
        float x = cx + sinf(a) * R * e, y = topo + (1 - cosf(a)) * R * 0.45f * e;
        peca_parada(x, y, r, pc, 0);
        if (q >= 1) dica_peca((int)(x - r), (int)(y - r), (int)(2 * r), (int)(2 * r), &pc);   // o de cima ganha a dica
    }
    if (n > m && t > m * 0.04f + 0.2f) {
        char s[12];
        snprintf(s, sizeof s, "+%d", n - m);
        gfx_texto(cx + (int)(sinf(abre) * R + r) + 4, topo + leque_cai(n, R, r) - 4, s, C_TEXTO_M, GFX_FINO, false);
    }
}
static void amuletos_fileira(int cx, int y, amul_t am, int r)
{
    int na = am_conta(am), x = cx - (na - 1) * (r + 4);
    for (int a = 0; a < A_N; a++) {
        if (!(am >> a & 1)) continue;
        desenha_amuleto(x, y, r, a, false);
        dica_zona(x - r, y - r, 2 * r, 2 * r, DICA_AMULETO, a);
        x += 2 * r + 8;
    }
}

// O caminho da run: cada salao com o nome na cor dele e as mesas em fila -
// as vencidas com o visto de ouro, a que parou a run com o X vinho, as que
// ficaram por jogar so o aro apagado. cx: o meio da fila.
static void caminho_run(int cx, int y, int ate_andar, int ate_passo, bool perdeu)
{
    int ns = com_coringa() || ate_andar >= DES_ANDAR_FINAL ? DES_ANDARES + 1 : DES_ANDARES;
    int gw[DES_ANDARES + 1], tot = 0;
    for (int a = 0; a < ns; a++) {
        int nm = a == DES_ANDAR_FINAL ? 1 : 3, wn = gfx_largura(SALAO[a], GFX_FINO);
        gw[a] = wn > nm * 18 ? wn : nm * 18;
        tot += gw[a] + (a ? 28 : 0);
    }
    int x = cx - tot / 2;
    for (int a = 0; a < ns; a++) {
        int nm = a == DES_ANDAR_FINAL ? 1 : 3, gx = x + gw[a] / 2;
        gfx_texto(gx - gfx_largura(SALAO[a], GFX_FINO) / 2, y, SALAO[a], a <= ate_andar ? COR_SALAO[a] : C_TEXTO_M, GFX_FINO, false);
        for (int k = 0; k < nm; k++) {
            int mx = gx + (k - (nm - 1) / 2.0f) * 18, my = y + 26;
            bool feita = a < ate_andar || (a == ate_andar && k < ate_passo) || (!perdeu && a == ate_andar);
            bool parou = perdeu && a == ate_andar && k == ate_passo;
            if (feita) {
                gfx_disco(mx, my, 6, C_BARRA); gfx_disco(mx, my, 5, C_OURO);
                gfx_linha(mx - 3, my, mx - 1, my + 2, C_BARRA); gfx_linha(mx - 1, my + 2, mx + 3, my - 2, C_BARRA);
            } else if (parou) {
                gfx_disco(mx, my, 6, C_BARRA); gfx_disco(mx, my, 5, C_VINHO);
                gfx_linha(mx - 2, my - 2, mx + 2, my + 2, C_ROSA); gfx_linha(mx - 2, my + 2, mx + 2, my - 2, C_ROSA);
            } else {
                gfx_disco(mx, my, 6, C_FELTRO2); gfx_disco(mx, my, 4, C_FELTRO_ESC);
            }
        }
        if (a + 1 < ns)                               // o caminho ate o proximo salao
            for (int d = x + gw[a] + 4; d < x + gw[a] + 24; d += 4) gfx_rect(d, y + 26, 2, 2, a < ate_andar ? C_OURO : C_FELTRO2);
        x += gw[a] + 28;
    }
}

// A foto da run campea: uma polaroide de moldura marfim com a mesa dentro -
// a bolsa em leque e os amuletos - e a legenda escrita embaixo. A mesma foto
// vai para o historico (so com os tipos dos dados).
#define FOTO_BORDA 6
#define FOTO_BAIXO 20                        // a tira de baixo, a da legenda
typedef struct { int cx, topo, R, yam; float r, ram; } foto_arranjo_t;
// Onde o leque e os amuletos ficam numa area de w x h: no meio dela.
static foto_arranjo_t foto_arranjo(int x, int y, int w, int h, int n, bool amuletos)
{
    foto_arranjo_t f = { .cx = x + w / 2, .R = (int)(w * 0.46f), .r = (w > 220 ? 12 : 10) + (h > 160 ? 2 : 0),
                         .ram = w > 220 ? 8 : 7 };
    int cai = leque_cai(n, f.R, f.r), em_cima = (int)f.r, embaixo = (int)(f.r + 8 + 2 * f.ram);
    if (!amuletos) embaixo = (int)f.r;
    if (cai + (int)f.r > embaixo) embaixo = cai + (int)f.r;
    f.topo = y + h / 2 - (em_cima + embaixo) / 2 + em_cima;
    f.yam = f.topo + (int)(f.r + 8 + f.ram);
    return f;
}
static void foto_da_run(int x, int y, int w, int h, const peca_t *col, const uint8_t *tipos, int n, amul_t am,
                        const char *legenda, float t)
{
    int fx = x + FOTO_BORDA, fy = y + FOTO_BORDA, fw = w - 2 * FOTO_BORDA, fh = h - FOTO_BORDA - FOTO_BAIXO;
    uint16_t rm[4];
    rampa_de(C_MARFIM, rm);
    ui_sombra(x, y, w, h);
    for (int j = 0; j < h; j++)                       // o papel, com a luz de cima
        for (int i = 0; i < w; i++)
            gfx_pixel(x + i, y + j, tom_rampa(rm, 0.9f - 0.6f * (i + j) / (float)(w + h), x + i, y + j));
    ui_contorno(x, y, w, h, C_BARRA);
    for (int j = 0; j < fh; j++)                      // a mesa na foto: o feltro, mais escuro na beira
        for (int i = 0; i < fw; i++) {
            int d = i < j ? i : j, d2 = fw - 1 - i < fh - 1 - j ? fw - 1 - i : fh - 1 - j;
            if (d2 < d) d = d2;
            float q = d < 14 ? 1 - d / 14.0f : 0;
            gfx_pixel(fx + i, fy + j, bayer(fx + i, fy + j) < q ? C_FELTRO_ESC : C_FELTRO);
        }
    ui_contorno(fx - 1, fy - 1, fw + 2, fh + 2, C_BARRA);
    foto_arranjo_t f = foto_arranjo(fx, fy, fw, fh, n, am != 0);
    leque_da_bolsa(f.cx, f.topo, f.R, f.r, col, tipos, n, t);
    amuletos_fileira(f.cx, f.yam, am, (int)f.ram);
    gfx_texto(x + w / 2 - gfx_largura(legenda, GFX_FINO) / 2, y + h - FOTO_BAIXO - 1, legenda, C_EBANO, GFX_FINO, false);
}

static uint32_t hoje(void);                   // historico.inc
// A run perdida, num resumo: quem a parou (com a fala dele), o caminho pelos
// saloes, a bolsa em leque, os amuletos e, na fina, os numeros.
static void desenha_run_perdida(void)
{
    char s[96];
    txt_sombra_c(GFX_W / 2, MESA_Y0 + 2, "FIM DA RUN", C_VINHO_CLR, 2);
    int p = des_perdeu_para;
    if (p >= 0) {
        int esc = 3, w = RT_W * esc, h = RT_H * esc, x = 34, y = MESA_Y0 + 52 + ui_entra(t_fase, 0, UI_ENTRA_CARTAO);
        uint16_t r[4];
        ui_rampa(perso_chefe(p) ? C_OURO : C_LATAO, false, r);
        ui_sombra(x - 3, y - 3, w + 6, h + 6);
        ui_contorno(x - 3, y - 3, w + 6, h + 6, C_BARRA);
        ui_aro(x - 2, y - 2, w + 4, h + 4, 2, r, UI_VIVO);
        gfx_rect(x, y, w, h, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
        retrato(x, y, esc, p, t_fase + p * 1.3f, t_fase > 0.5f && t_fase < 1.4f, 0, RT_H);
        txt_c(x + w / 2, y + h + 4, PERSO[p].nome, C_ROSA, true);
        if (t_fase > 0.45f) {                         // a fala de quem ganhou, num balao
            float q = sai_mola(fminf(1, (t_fase - 0.45f) / 0.2f));
            const char *f = PERSO[p].fala[FA_GANHA_P][0];
            balao_em(x + w / 2, y + h + 26, f, f, q);
        }
    }
    int cx = 404;                                     // o resto, a direita do retrato
    caminho_run(cx, MESA_Y0 + 50, des_perdeu_andar, run.passo, true);
    int topo = MESA_Y0 + 106;                         // o leque: as pontas nao descem ate o balao
    leque_da_bolsa(cx, topo, 190, 13, run.col, NULL, run.n_col, t_fase - 0.3f);
    if (t_fase > 0.9f) amuletos_fileira(cx, topo + leque_cai(run.n_col, 190, 13) + 34, run_amuletos(), 9);
    snprintf(s, sizeof s, "%d %s  ·  %d dados na bolsa  ·  %d moedas", run.vitorias,
             run.vitorias == 1 ? "partida vencida" : "partidas vencidas", run.n_col, run.moedas);
    gfx_texto(cx - gfx_largura(s, GFX_FINO) / 2, MESA_Y1 - 50, s, C_TEXTO_M, GFX_FINO, false);
    teclas_c(GFX_W / 2, MESA_Y1 - 20, "ENTER volta ao menu", C_MARFIM_S);
}

// A run vencida: a bolsa campea posta na mesa, o flash e a foto, que fica
// guardada no historico.
#define FOTO_FLASH 1.5f
static void foto_clique(float antes, float agora)          // o clique da maquina, no flash
{
    if (des_venceu_run && antes < FOTO_FLASH && agora >= FOTO_FLASH) som_toca(SOM_TIQUE);
}
static void desenha_run_campea(int n)
{
    char s[96], leg[64];
    txt_sombra_c(GFX_W / 2, MESA_Y0 + 2, "VITÓRIA", C_OURO, 2);
    txt_c(GFX_W / 2, MESA_Y0 + 42, run.andar >= DES_ANDAR_FINAL ? "Você venceu O CORINGA e fechou o cassino."
                                                                 : "Você venceu O BARÃO e fechou o cassino.", C_MARFIM, true);
    uint32_t d = hoje();
    snprintf(leg, sizeof leg, "Run campeã  ·  %s  ·  %02u/%02u/%04u", nome_dif(run.dificuldade),
             (unsigned)(d % 100), (unsigned)(d / 100 % 100), (unsigned)(d / 10000));
    int fw = 340, fh = 150, fx = GFX_W / 2 - fw / 2, fy = MESA_Y0 + 60;
    if (t_fase < FOTO_FLASH) {                        // a bolsa vai sendo posta na mesa, ja onde a foto pega
        foto_arranjo_t f = foto_arranjo(fx + FOTO_BORDA, fy + FOTO_BORDA, fw - 2 * FOTO_BORDA,
                                        fh - FOTO_BORDA - FOTO_BAIXO, run.n_col, run_amuletos() != 0);
        leque_da_bolsa(f.cx, f.topo, f.R, f.r, run.col, NULL, run.n_col, t_fase);
        if (t_fase > 1.0f) amuletos_fileira(f.cx, f.yam, run_amuletos(), (int)f.ram);
    } else {
        foto_da_run(fx, fy, fw, fh, run.col, NULL, run.n_col, run_amuletos(), leg, 9);
        float q = (t_fase - FOTO_FLASH) / 0.35f;      // o flash: a tela clareia e volta
        if (q < 1) gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_BRANCO, (int)(220 * (1 - q)));
        else txt_c(GFX_W / 2, fy + fh + 4, "a foto fica no seu histórico", C_TEXTO_M, false);
    }
    if (n) nov_linha_fim(MESA_Y1 - 40);               // o que abriu vem depois, card a card
    snprintf(s, sizeof s, "%s", n ? "ENTER mostra as novidades" : "ENTER segue");
    teclas_c(GFX_W / 2, MESA_Y1 - 20, s, C_MARFIM_S);
}

static void desenha_fim_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    faixa_run(0);
    if (des_venceu_run) desenha_run_campea(nov_quantas());
    else desenha_run_perdida();
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

// As moedas da vitoria, em fileira: entram uma a uma (andamento.inc), a
// ultima pulando, com um brilho. Embaixo de cada grupo, de onde veio (a mesa,
// o Colar, o Cofre, a Taca); embaixo de tudo, a bolsa de moedas contando.
static void saque_fileira(int y)
{
    int tot = saque_total();
    if (!tot) {                                  // a ultima mesa da run nao paga: nao ha mais loja
        txt_c(GFX_W / 2, y - 6, "A última mesa da run.", C_MARFIM, true);
        return;
    }
    int sp = tot > 22 ? 14 : 17, gap = 10;
    int grupos[5] = { run.ganho, run.colar, run.juros, run.taca, run.dourado };
    int ng = (run.colar > 0) + (run.juros > 0) + (run.taca > 0) + (run.dourado > 0);
    int larg = tot * sp + ng * gap, x = GFX_W / 2 - larg / 2 + sp / 2, i = 0;
    static const char *const ROT[5] = { "mesa", "Colar", "Cofre", "Taça", "Dourado" };
    uint16_t cor_rot[5] = { C_OURO, AMULETO[A_COLAR].cor, AMULETO[A_COFRE].cor, AMULETO[A_TACA].cor, ADESIVO[AD_DOURADO].luz };
    for (int g = 0; g < 5; g++) {
        if (!grupos[g]) continue;
        int x0 = x;
        for (int k = 0; k < grupos[g]; k++, i++, x += sp) {
            if (i >= saque_n) {                      // ainda nao entrou: so o lugar
                gfx_disco(x, y, 2, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
                continue;
            }
            float r = 7;
            if (i == saque_n - 1 && !saque_pronto) {
                float q = t_saque / 0.13f;
                r = 7 * (q < 1 ? sai_mola(q) * 1.15f : 1);
                if (q < 1)                           // o brilho da que entrou
                    for (int b = 0; b < 4; b++) {
                        float a = b * 1.5708f + 0.78f;
                        int d = 9 + (int)(q * 6);
                        gfx_rect(x + (int)(cosf(a) * d), y + (int)(sinf(a) * d), 2, 2, C_AMARELO);
                    }
            }
            moeda_r(x, y - (i == saque_n - 1 && !saque_pronto ? 2 : 0), r);
        }
        const char *rot = ROT[g];
        int cx = (x0 + x - sp) / 2;
        bool aceso = i - grupos[g] < saque_n;          // o primeiro do grupo ja entrou
        gfx_texto(cx - gfx_largura(rot, GFX_MIUDO) / 2, y + 10, rot, aceso ? cor_rot[g] : C_FELTRO2, GFX_MIUDO, false);
        x += gap;
    }
    // A bolsa de moedas: o que tinha mais o que ja entrou.
    char s[40];
    snprintf(s, sizeof s, "+%d", saque_n);
    int na_bolsa = run.moedas - tot + saque_n;
    char b[40];
    snprintf(b, sizeof b, "%d moedas na bolsa", na_bolsa);
    int wb = gfx_largura(b, fonte_de(false)), ws = gfx_largura(s, 1);
    int bx = GFX_W / 2 - (ws + 30 + wb) / 2;
    txt(bx, y + 22, s, saque_pronto ? C_AMARELO : C_OURO, true);
    moeda_ic(bx + ws + 14, y + 33);
    txt(bx + ws + 26, y + 24, b, C_MARFIM_S, false);
}

// Tela do fim de uma partida da run (por cima da mesa, como a normal).
static void desenha_fim_partida_run(void)
{
    int C0 = CENTRO + ui_entra(t_fase, 0, 2 * UI_ENTRA_PLACA);   // a placa entra (interface.inc)
    // O oponente, grande, a esquerda, com o que disse no fim.
    if (fala_perso >= 0) {
        int esc = 3, rx = 22, ry = C0 - 70;
        bool chefe = perso_chefe(fala_perso);
        gfx_rect(rx - 2, ry - 2, RT_W * esc + 4, RT_H * esc + 4, chefe ? C_OURO : C_LATAO_ESC);
        gfx_rect(rx, ry, RT_W * esc, RT_H * esc, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
        retrato(rx, ry, esc, fala_perso, t_fase + fala_perso * 1.3f, fala_digitando(), 0, RT_H);
        dica_zona(rx, ry, RT_W * esc, RT_H * esc, DICA_TRUNFO, fala_perso);
        if (fala_txt[0]) txt_c(rx + RT_W * esc / 2, ry + RT_H * esc + 6, PERSO[fala_perso].nome, C_ROSA, true);
    }
    int alto = venc_partida == 0 && des_marco >= 0 ? 168 : 150;
    painel(C0 - 70, alto, 460);
    placa_placar(C0 - 70);                                          // o placar final, acima do painel
    if (venc_partida == 0) {
        txt_sombra_c(GFX_W / 2, C0 - 58, "VOCÊ VENCEU", C_LATAO, 3);
        saque_fileira(C0 + 4);
        const char *dica = fim_da_run() ? "ENTER segue" : des_marco >= 0 ? "ENTER mostra a novidade"
                         : chefe_deixa_amuleto() ? "ENTER: o chefe deixa um amuleto" : "ENTER vai à loja";
        int yd = C0 + 56;
        if (des_marco >= 0) {                      // primeira vitoria contra este chefe: o card vem depois
            txt_c(GFX_W / 2, C0 + 46, "Abriu uma novidade para as próximas runs", C_OURO, true);
            yd = C0 + 72;
        }
        teclas_c(GFX_W / 2, yd - (saque_pronto && t_saque < 0.15f ? 2 : 0), dica, saque_pronto ? C_MARFIM : C_TEXTO_M);
    } else {
        txt_sombra_c(GFX_W / 2, C0 - 58, venc_partida == 2 ? "EMPATE" : "DERROTA", C_VINHO_CLR, 3);
        txt_c(GFX_W / 2, C0 + 6, des_saiu ? "Você saiu da mesa: conta como derrota, e a run termina."
                                     : venc_partida == 2 ? "Empate não passa de mesa: a run termina aqui."
                                                         : "A run termina aqui.", C_MARFIM, true);
        teclas_c(GFX_W / 2, C0 + 46, "ENTER segue", C_MARFIM_S);
    }
    if (fala_perso >= 0 && fala_txt[0]) {                 // a ultima palavra dele, letra por letra
        char q[80];
        snprintf(q, sizeof q, "\"%s\"", fala_txt);
        int x0 = GFX_W / 2 - gfx_largura(q, fonte_de(false)) / 2;
        const char *dito = fala_dita_txt();
        snprintf(q, sizeof q, "\"%s%s", dito, strlen(dito) == strlen(fala_txt) ? "\"" : "");
        txt(x0, C0 - 70 + alto + 8, q, C_ROSA, false);
    }
}

// A cena do Modo Desafiante no tutorial: as tres mesas de um salao em fila,
// com o caminho pontilhado, e a sua ficha andando de uma para a outra; a
// mesa que fica para tras leva o visto.
static void tut_cena_desafio(int cx, int cy, float t)
{
    static int P[3] = { -1, -1, -1 };
    static const char *const NOME[3] = { "O COVEIRO", "O LADRÃO", "O CRUPIÊ" };
    static const char *const TAG[3] = { "Fácil", "Difícil", "Chefe" };
    for (int i = 0; i < 3; i++)
        if (P[i] < 0) for (int p = 0; p < N_PERSO; p++) if (!strcmp(PERSO[p].nome, NOME[i])) P[i] = p;
    float c = fmodf(t, 5.2f);
    // Onde a ficha esta: parada numa mesa ou andando para a proxima.
    float pos = c < 1.3f ? 0 : c < 1.9f ? (c - 1.3f) / 0.6f : c < 3.1f ? 1 : c < 3.7f ? 1 + (c - 3.1f) / 0.6f : 2;
    int w = RT_W * 2, h = RT_H * 2, y = cy - 50;
    for (int i = 0; i + 1 < 3; i++)                   // o caminho
        for (int x = cx - 150 + i * 150 + w / 2 + 6; x < cx - 150 + (i + 1) * 150 - w / 2 - 6; x += 6)
            gfx_rect(x, y + h / 2, 2, 2, x < cx - 150 + pos * 150 ? C_OURO : C_FELTRO2);
    for (int i = 0; i < 3; i++) {
        int x = cx - 150 + i * 150;
        bool chefe = i == 2, feita = pos > i + 0.3f;
        uint16_t r[4];
        ui_rampa(chefe ? C_OURO : C_LATAO_ESC, feita, r);
        ui_contorno(x - w / 2 - 3, y - 3, w + 6, h + 6, C_BARRA);
        ui_aro(x - w / 2 - 2, y - 2, w + 4, h + 4, 2, r, UI_VIVO);
        gfx_rect(x - w / 2, y, w, h, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
        if (P[i] >= 0) retrato(x - w / 2, y, 2, P[i], t + i * 1.3f, false, 0, RT_H);
        if (feita) {                                  // vencida: apaga e leva o visto
            gfx_rect_alfa(x - w / 2, y, w, h, C_SOMBRA, 150);
            gfx_linha_grossa(x - 9, y + h / 2, x - 2, y + h / 2 + 8, 4, C_OURO);
            gfx_linha_grossa(x - 2, y + h / 2 + 8, x + 12, y + h / 2 - 10, 4, C_OURO);
        }
        gfx_texto(x - gfx_largura(TAG[i], GFX_FINO) / 2, y + h + 2, TAG[i], feita ? C_TEXTO_M : cor_no(i == 0 ? NO_FACIL : i == 1 ? NO_DIFICIL : NO_CHEFE),
                  GFX_FINO, false);
    }
    int fx = cx - 150 + (int)(pos * 150), pulo = pos != (int)pos ? (int)(sinf((pos - (int)pos) * 3.1416f) * 8) : 0;
    ficha(fx - w / 2 - 12, y + h / 2 - pulo, C_LATAO);   // a sua ficha, na frente da mesa da vez
}

