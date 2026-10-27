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
    icone_bolsa(xb - 12, FAIXA_H / 2 - pulo);
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
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
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
    gfx_rect(x - w / 2 - 2, y - 2, w + 4, h + 4, moldura);
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
    if (!feita) gfx_texto(x - gfx_largura(tag, GFX_MIUDO) / 2, SALAO_BASE + 26, tag, cn, GFX_MIUDO, false);
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
static void desenha_fim_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run(0);
    int n = des_venceu_run ? nov_quantas() : 0;
    painel(CENTRO - 84, n ? 186 : 168, 500);
    char s[96];
    if (des_venceu_run) {
        txt_sombra_c(GFX_W / 2, CENTRO - 74, "VITÓRIA", C_OURO, 3);
        txt_c(GFX_W / 2, CENTRO - 6, "Você venceu O BARÃO e fechou o cassino.", C_MARFIM, true);
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
    if (n) nov_linha_fim(CENTRO + 46);          // o que abriu vem depois, card a card
    teclas_c(GFX_W / 2, CENTRO + (n ? 76 : 58), n ? "ENTER mostra as novidades"
                                                  : des_venceu_run ? "ENTER segue" : "ENTER volta ao menu", C_MARFIM_S);
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
    // O oponente, grande, a esquerda, com o que disse no fim.
    if (fala_perso >= 0) {
        int esc = 3, rx = 22, ry = CENTRO - 70;
        bool chefe = perso_chefe(fala_perso);
        gfx_rect(rx - 2, ry - 2, RT_W * esc + 4, RT_H * esc + 4, chefe ? C_OURO : C_LATAO_ESC);
        gfx_rect(rx, ry, RT_W * esc, RT_H * esc, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
        retrato(rx, ry, esc, fala_perso, t_fase + fala_perso * 1.3f, fala_digitando(), 0, RT_H);
        dica_zona(rx, ry, RT_W * esc, RT_H * esc, DICA_TRUNFO, fala_perso);
        if (fala_txt[0]) txt_c(rx + RT_W * esc / 2, ry + RT_H * esc + 6, PERSO[fala_perso].nome, C_ROSA, true);
    }
    int alto = venc_partida == 0 && des_marco >= 0 ? 168 : 150;
    painel(CENTRO - 70, alto, 460);
    char s[80];
    if (venc_partida == 0) {
        txt_sombra_c(GFX_W / 2, CENTRO - 58, "VOCÊ VENCEU", C_LATAO, 3);
        saque_fileira(CENTRO + 4);
        const char *dica = fim_da_run() ? "ENTER segue" : des_marco >= 0 ? "ENTER mostra a novidade"
                         : chefe_deixa_amuleto() ? "ENTER: o chefe deixa um amuleto" : "ENTER vai à loja";
        int yd = CENTRO + 56;
        if (des_marco >= 0) {                      // primeira vitoria contra este chefe: o card vem depois
            txt_c(GFX_W / 2, CENTRO + 46, "Abriu uma novidade para as próximas runs", C_OURO, true);
            yd = CENTRO + 72;
        }
        teclas_c(GFX_W / 2, yd - (saque_pronto && t_saque < 0.15f ? 2 : 0), dica, saque_pronto ? C_MARFIM : C_TEXTO_M);
    } else {
        txt_sombra_c(GFX_W / 2, CENTRO - 58, venc_partida == 2 ? "EMPATE" : "DERROTA", C_VINHO_CLR, 3);
        txt_c(GFX_W / 2, CENTRO + 6, des_saiu ? "Você saiu da mesa: conta como derrota, e a run termina."
                                     : venc_partida == 2 ? "Empate não passa de mesa: a run termina aqui."
                                                         : "A run termina aqui.", C_MARFIM, true);
        snprintf(s, sizeof s, "%s %d  x  %d %s", J[0].nome, J[0].fichas, J[1].fichas, J[1].nome);
        txt_c(GFX_W / 2, CENTRO + 32, s, C_TEXTO_M, false);
        teclas_c(GFX_W / 2, CENTRO + 58, "ENTER segue", C_MARFIM_S);
    }
    if (fala_perso >= 0 && fala_txt[0]) {                 // a ultima palavra dele, letra por letra
        char q[80];
        snprintf(q, sizeof q, "\"%s\"", fala_txt);
        int x0 = GFX_W / 2 - gfx_largura(q, fonte_de(false)) / 2;
        const char *dito = fala_dita_txt();
        snprintf(q, sizeof q, "\"%s%s", dito, strlen(dito) == strlen(fala_txt) ? "\"" : "");
        txt(x0, CENTRO - 70 + alto + 8, q, C_ROSA, false);
    }
}
