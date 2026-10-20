// ---------------------------------------------------------------------------
// A loja da run, depois de cada mesa vencida: a vitrine (2 dados, 3 com a
// Cartola), trocar a vitrine e tirar um dado da bolsa. Cada acao responde na
// hora, com movimento: a vitrine cai do alto quando a loja abre (e de novo
// quando troca, girando); o dado comprado voa ate a bolsa da faixa, e o
// carimbo de vendido bate no cartao; sem moedas, o preco treme; o dado tirado
// treme, racha e estoura em lascas.
// ---------------------------------------------------------------------------
#define LJ_Y      118                    // o alto dos cartoes
#define LJ_CW     104                    // cartao da vitrine
#define LJ_CH     118
#define LJ_PASSO  116
#define LJ_VIT_CX 190                    // o meio da vitrine
#define LJ_SX     402                    // os servicos, a direita
#define LJ_SW     212
#define LJ_SH     55
#define LJ_DESC_Y 250                    // o que o item em foco faz
#define LJ_VOO_T  0.5f                   // o dado comprado ate a bolsa
#define LJ_TIRA_T 0.32f                  // o dado que sai: tremendo ate estourar
#define LJ_LASCAS 40

static bool  lj_tirando;                 // escolhendo o dado que sai
static float t_lj_entra = 9;             // desde que a vitrine (re)apareceu
static float t_lj_abriu = 9;             // desde que a loja abriu (os servicos entram uma vez)
static uint8_t lj_caiu;                  // cartoes que ja bateram na mesa (o som, uma vez)
static float t_lj_nega[VITRINE_MAX + 2]; // > 0: o item treme (sem moedas, sem lugar)
static float t_lj_vendido[VITRINE_MAX];  // desde a compra: o carimbo bate
static char  lj_aviso[64];               // por que nao deu
static float t_lj_aviso;
typedef struct { peca_t p; float x0, y0, t; bool vivo; } lj_voo_t;
static lj_voo_t lj_voo[VITRINE_MAX];
static peca_t lj_tirado;                 // o dado que sai, enquanto treme
static int   lj_tira_i = -1, lj_tira_x, lj_tira_y;
static float t_lj_tira, t_lj_volta;      // tremendo; depois do estouro, ate voltar a loja
typedef struct { float x, y, vx, vy, t, vida; uint16_t cor; uint8_t tam; } lasca_t;
static lasca_t lasca[LJ_LASCAS];
static int n_lasca;
typedef struct { float x, y, t; char s[16]; uint16_t cor; } lj_flut_t;
static lj_flut_t lj_flut[6];

static int lj_n_itens(void) { return n_vitrine() + 2; }
static int lj_card_x(int i) { int nv = n_vitrine(); return LJ_VIT_CX + (int)((i - (nv - 1) / 2.0f) * LJ_PASSO); }
static int lj_serv_y(int s) { return LJ_Y + s * (LJ_SH + 8); }

static void loja_abriu(void)
{
    lj_tirando = false;
    t_lj_entra = t_lj_abriu = 0;
    lj_caiu = 0;
    lj_tira_i = -1;
    t_lj_aviso = 0;
    t_lj_tira = t_lj_volta = 0;
    n_lasca = 0;
    bolsa_voando = 0;
    memset(t_lj_nega, 0, sizeof t_lj_nega);
    memset(t_lj_vendido, 0, sizeof t_lj_vendido);
    memset(lj_voo, 0, sizeof lj_voo);
    memset(lj_flut, 0, sizeof lj_flut);
}

static void lj_flutua(float x, float y, uint16_t cor, const char *fmt, int v)
{
    int i = 0;
    while (i < 5 && lj_flut[i].t > 0) i++;
    lj_flut[i].x = x; lj_flut[i].y = y; lj_flut[i].t = 0.001f; lj_flut[i].cor = cor;
    snprintf(lj_flut[i].s, sizeof lj_flut[i].s, fmt, v);
}

// Nao deu: o item treme, o preco pisca e a linha de baixo diz por que.
static void lj_nega(int item, const char *porque)
{
    if (item >= 0 && item < VITRINE_MAX + 2) t_lj_nega[item] = 0.4f;
    snprintf(lj_aviso, sizeof lj_aviso, "%s", porque);
    t_lj_aviso = 1.6f;
    som_toca_tom(SOM_TIQUE, -7);
}

static void lj_tira_confirma(void);
static void lj_estoura(void)
{
    // As lascas: a cor do dado, o marfim do corpo e o po.
    uint16_t cores[3] = { cor_dado(lj_tirado.tipo), C_MARFIM_S, C_TEXTO_M };
    n_lasca = 0;
    for (int i = 0; i < 22 && n_lasca < LJ_LASCAS; i++) {
        lasca_t *l = &lasca[n_lasca++];
        float a = frnd(0, 6.2832f), v = frnd(60, 190);
        l->x = (float)lj_tira_x + cosf(a) * 6; l->y = (float)lj_tira_y + sinf(a) * 6;
        l->vx = cosf(a) * v; l->vy = sinf(a) * v - 90;
        l->t = 0; l->vida = frnd(0.45f, 0.85f);
        l->cor = cores[i % 3];
        l->tam = (uint8_t)(i % 3 == 2 ? 2 : 3 + (i % 2));
    }
}

static void loja_passo(float dt)
{
    if (fase != F_RLOJA) return;
    t_lj_entra += dt;
    t_lj_abriu += dt;
    if (t_lj_aviso > 0) t_lj_aviso -= dt;
    for (int i = 0; i < VITRINE_MAX + 2; i++) if (t_lj_nega[i] > 0) t_lj_nega[i] -= dt;
    for (int i = 0; i < VITRINE_MAX; i++) if (t_lj_vendido[i] > 0) t_lj_vendido[i] += dt;
    // Os cartoes batendo na mesa, um depois do outro.
    for (int i = 0; i < n_vitrine(); i++) {
        float q = (t_lj_entra - i * 0.08f) / 0.34f;
        if (q >= 0.75f && !(lj_caiu >> i & 1)) { lj_caiu |= (uint8_t)(1 << i); som_toca_tom(SOM_BATE_DADO, i * 2); }
    }
    // O dado comprado chegando na bolsa.
    for (int i = 0; i < VITRINE_MAX; i++) {
        if (!lj_voo[i].vivo) continue;
        lj_voo[i].t += dt;
        if (lj_voo[i].t >= LJ_VOO_T) {
            lj_voo[i].vivo = false;
            if (bolsa_voando > 0) bolsa_voando--;
            t_bolsa_pulo = 1;
            som_toca(SOM_BOLSA);
        }
    }
    for (int i = 0; i < 6; i++) if (lj_flut[i].t > 0 && (lj_flut[i].t += dt) > 0.9f) lj_flut[i].t = 0;
    // O dado que sai: treme, estoura (sai da bolsa nessa hora), as lascas
    // caem, e a loja volta.
    if (t_lj_tira > 0) {
        t_lj_tira += dt;
        if (t_lj_tira >= LJ_TIRA_T) {            // estoura: so agora sai da bolsa
            t_lj_tira = 0;
            lj_tira_confirma();
            lj_estoura();
            som_toca(SOM_VIDRO);
            t_lj_volta = 0.001f;
        }
    } else if (t_lj_volta > 0 && (t_lj_volta += dt) > 0.75f) {
        t_lj_volta = 0;
        lj_tirando = false;
        cursor = n_vitrine() + 1;
    }
    for (int i = 0; i < n_lasca; i++) {
        lasca_t *l = &lasca[i];
        l->t += dt;
        l->x += l->vx * dt; l->y += l->vy * dt;
        l->vy += 520 * dt;
        l->vx *= 1 - 1.5f * dt;
    }
    while (n_lasca && lasca[n_lasca - 1].t > lasca[n_lasca - 1].vida) n_lasca--;
}

// ---- As acoes ---------------------------------------------------------------
static void loja_compra(int i)
{
    int pr = preco_run(run.vitrine[i].tipo);
    if (run.levou[i]) { lj_nega(i, "Esse já foi para a sua bolsa."); return; }
    if (run.moedas < pr) { lj_nega(i, "Moedas insuficientes."); return; }
    if (run.n_col >= MAX_COL) { lj_nega(i, "A bolsa está cheia."); return; }
    run.moedas -= pr;
    run.col[run.n_col++] = run.vitrine[i];
    run.levou[i] = 1;
    des_salva();
    t_lj_vendido[i] = 0.001f;
    lj_voo[i] = (lj_voo_t){ run.vitrine[i], (float)lj_card_x(i), (float)(LJ_Y + 40), 0, true };
    bolsa_voando++;
    lj_flutua((float)lj_card_x(i), (float)(LJ_Y + LJ_CH - 18), C_VINHO_CLR, "-%d", pr);
    mola_chuta();
    som_toca(SOM_FICHA);
}

static void loja_troca(void)
{
    int nv = n_vitrine(), pr = preco_troca();
    if (run.moedas < pr) { lj_nega(nv, "Moedas insuficientes."); return; }
    run.moedas -= pr;
    if (run.trocas < 255) run.trocas++;
    monta_vitrine();
    des_salva();
    t_lj_entra = 0;                              // a vitrine nova cai, girando
    lj_caiu = 0;
    memset(t_lj_vendido, 0, sizeof t_lj_vendido);
    lj_flutua(LJ_SX + LJ_SW - 26, (float)(lj_serv_y(0) - 4), C_VINHO_CLR, "-%d", pr);
    som_toca(SOM_FICHA);
}

static void loja_tira_abre(void)
{
    int nv = n_vitrine();
    if (run.n_col <= BOLSA_MIN) { lj_nega(nv + 1, "A bolsa já está no mínimo."); return; }
    if (run.moedas < preco_remove()) { lj_nega(nv + 1, "Moedas insuficientes."); return; }
    lj_tirando = true;
    cursor = 0;
    som_toca(SOM_TIQUE);
}

#define LJ_TIRA_Y 112                     // o alto da grade da bolsa, tirando
static grade_t grade_lj_tira(int n) { return grade_cabe(n, 12, 52, 120); }
static void lj_pos_tira(int i, int *x, int *y)
{
    grade_t g = grade_lj_tira(run.n_col);
    int lin = i / g.por, col = i % g.por;
    int na_linha = (run.n_col - lin * g.por) < g.por ? run.n_col - lin * g.por : g.por;
    *x = GFX_W / 2 + (int)((col - (na_linha - 1) / 2.0f) * 46 * g.f);
    *y = LJ_TIRA_Y + (int)((lin + 0.5f) * 52 * g.f);
}

// O dado escolhido sai de vez: paga, sai da bolsa e fica salvo.
static void lj_tira_confirma(void)
{
    int i = lj_tira_i, pr = preco_remove();
    lj_tira_i = -1;
    if (i < 0 || i >= run.n_col || run.n_col <= BOLSA_MIN || run.moedas < pr) return;
    run.moedas -= pr;
    for (int m = i; m + 1 < run.n_col; m++) run.col[m] = run.col[m + 1];
    run.n_col--;
    if (run.remocoes < 255) run.remocoes++;
    des_salva();
    lj_flutua((float)lj_tira_x, (float)(lj_tira_y - 30), C_VINHO_CLR, "-%d", pr);
}

// O dado i vai sair: treme e racha (lj_tira_confirma, quando estoura).
static void loja_tira(int i)
{
    if (i < 0 || i >= run.n_col || run.n_col <= BOLSA_MIN || run.moedas < preco_remove()) return;
    lj_tira_i = i;
    lj_pos_tira(i, &lj_tira_x, &lj_tira_y);
    lj_tirado = run.col[i];
    t_lj_tira = 0.001f;
    som_toca(SOM_ANULA);
}

static void loja_sai(void)
{
    bolsa_voando = 0;
    if (run.estado == R_LOJA_INI) { run.estado = R_MAPA; des_salva(); abre_mapa(); }
    else avanca_passo();
}

static bool loja_tecla(int k, int e)
{
    if (lj_tirando) {
        if (t_lj_tira > 0 || t_lj_volta > 0) return true;      // estourando
        int por = grade_lj_tira(run.n_col).por;
        if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= por) cursor -= por; return true; }
        if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + por < run.n_col) cursor += por; return true; }
        if (e) { cursor = (cursor + e + run.n_col) % run.n_col; som_toca(SOM_TIQUE); return true; }
        if (k == KEY_BKSP || k == 'x' || k == KEY_TAB) { lj_tirando = false; cursor = n_vitrine() + 1; return true; }
        if (k == KEY_ENTER) loja_tira(cursor);
        return true;
    }
    int nv = n_vitrine(), nc = lj_n_itens();
    if (e) { cursor = (cursor + e + nc) % nc; som_toca(SOM_TIQUE); return true; }
    if (k >= '1' && k < '1' + nc) cursor = k - '1';
    if (k == KEY_ENTER || (k >= '1' && k < '1' + nc)) {
        if (cursor < nv) loja_compra(cursor);
        else if (cursor == nv) loja_troca();
        else loja_tira_abre();
        return true;
    }
    if (k == KEY_TAB || k == 'x') loja_sai();
    return true;
}

// ---- O desenho --------------------------------------------------------------
// O preco: a moeda e o numero; tremendo e vermelho quando nao deu.
static void lj_preco(int cx, int y, int pr, bool da, float nega)
{
    char s[12];
    snprintf(s, sizeof s, "%d", pr);
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 3 * (nega / 0.4f) + 0.5f) : 0;
    uint16_t c = nega > 0 && (int)(nega * 14) % 2 ? C_BRANCO : (da ? C_OURO : C_VINHO_CLR);
    int w = gfx_largura(s, 1) + 18, x = cx - w / 2 + dx;
    moeda_ic(x + 7, y + 11);
    txt(x + 18, y, s, c, true);
}

// O carimbo de vendido: bate grande e assenta, torto.
static void lj_carimbo(int cx, int cy, float t)
{
    float q = t / 0.14f;
    if (q > 1) q = 1;
    int cresce = (int)((1 - q) * 14);
    const char *s = "VENDIDO";
    int w = gfx_largura(s, 1) + 16 + 2 * cresce, h = TXT_H + 6 + cresce;
    int x = cx - w / 2, y = cy - h / 2;
    for (int k = 0; k < 2; k++) gfx_moldura(x + k * 2 - 1, y + k - 1, w, h, 2, k ? C_VINHO_CLR : C_SOMBRA);
    gfx_rect_alfa(x + 2, y + 2, w - 4, h - 4, C_SOMBRA, 120);
    txt_c(cx + 1, y + 3 + cresce / 2, s, C_VINHO_CLR, true);
}

static void lj_cartao_dado(int i)
{
    int x = lj_card_x(i);
    float q = (t_lj_entra - i * 0.08f) / 0.34f;
    if (q <= 0) return;
    int y = LJ_Y - (int)((1 - sai_mola(q)) * 80);
    peca_t pc = run.vitrine[i];
    int pr = preco_run(pc.tipo);
    bool ja = run.levou[i], cur = i == cursor && !lj_tirando, da = run.moedas >= pr;
    float nega = t_lj_nega[i];
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 2 * (nega / 0.4f)) : 0;
    ui_cartao(x - LJ_CW / 2 + dx, y, LJ_CW, LJ_CH, cur, cor_rar(pc.tipo));
    float giro = q < 1 ? (1 - q) * 7 : 0;           // caindo, o dado gira
    float bob = cur && !ja ? sinf(t_fase * 4) * 3 - mola_px() : 0;
    if (!ja || t_lj_vendido[i] < 0.12f) {
        if (cur && !ja) brilho_passa = mola_brilho();
        marca_nivel = pc.fixo;
        desenha_dado((float)(x + dx), y + 40 + bob, 24, pc.tipo, 0, giro, pc.fixo, 0, false);
        marca_nivel = -1;
    }
    nome_no_cartao(x + dx, y + 68, TIPO[pc.tipo].nome, cur ? cor_dado(pc.tipo) : C_MARFIM_S, LJ_CW - 8);
    const char *rar = RARIDADE[TIPO[pc.tipo].raridade];
    gfx_texto(x + dx - gfx_largura(rar, GFX_MIUDO) / 2, y + 85, rar, cor_rar(pc.tipo), GFX_MIUDO, false);
    if (ja) {
        gfx_rect_alfa(x - LJ_CW / 2 + 1, y + 1, LJ_CW - 2, LJ_CH - 2, C_SOMBRA, 110);
        lj_carimbo(x, y + LJ_CH - 22, t_lj_vendido[i] > 0 ? t_lj_vendido[i] : 1);   // ja vendido: assentado
    } else lj_preco(x, y + LJ_CH - 24, pr, da, nega);
    zona_clique(x - LJ_CW / 2, LJ_Y, LJ_CW, LJ_CH, &cursor, i, KEY_ENTER);
}

// Os servicos: trocar a vitrine (as setas girando) e tirar um dado (o dado rachado).
static void lj_servico(int s)
{
    int nv = n_vitrine(), item = nv + s, y0 = lj_serv_y(s);
    float q = (t_lj_abriu - 0.12f - s * 0.08f) / 0.3f;
    if (q <= 0) return;
    int x0 = LJ_SX + (int)((1 - sai_mola(q)) * 120);
    bool cur = item == cursor && !lj_tirando;
    float nega = t_lj_nega[item];
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 2 * (nega / 0.4f)) : 0;
    bool pode = s == 0 || run.n_col > BOLSA_MIN;
    ui_cartao(x0 + dx, y0, LJ_SW, LJ_SH, cur, s == 0 ? C_LATAO : C_VINHO_CLR);
    int ix = x0 + dx + 28, iy = y0 + LJ_SH / 2;
    if (s == 0) {                                 // dois dados e a seta em volta
        float gira = cur ? t_fase * 2.5f : 0;
        peca_parada((float)(ix - 6), (float)(iy - 3), 9, (peca_t){ D_D6, 0 }, 0);
        peca_parada((float)(ix + 7), (float)(iy + 5), 8, (peca_t){ D_D4, 0 }, 0);
        for (int k = 0; k < 18; k++) {
            float a = gira + k * 0.29f;
            gfx_rect(ix + (int)(cosf(a) * 19), iy + (int)(sinf(a) * 19), 2, 2, k > 14 ? C_OURO : C_LATAO);
        }
    } else {                                      // um dado com a rachadura
        peca_parada((float)ix, (float)iy, 13, (peca_t){ D_D6, 0 }, 0);
        gfx_linha_grossa(ix - 8, iy - 10, ix - 1, iy - 2, 2, C_BARRA);
        gfx_linha_grossa(ix - 1, iy - 2, ix - 5, iy + 4, 2, C_BARRA);
        gfx_linha_grossa(ix - 5, iy + 4, ix + 4, iy + 11, 2, C_BARRA);
        if (!pode) gfx_rect_alfa(ix - 16, iy - 16, 32, 32, C_SOMBRA, 120);
    }
    const char *nome = s == 0 ? "Trocar a vitrine" : "Tirar um dado";
    txt(x0 + dx + 56, y0 + 8, nome, cur ? C_MARFIM : C_MARFIM_S, true);
    char sub[48];
    if (s == 0) snprintf(sub, sizeof sub, "outros dados na vitrine");
    else if (pode) snprintf(sub, sizeof sub, "bolsa: %d dados", run.n_col - bolsa_voando);
    else snprintf(sub, sizeof sub, "bolsa no mínimo (%d)", BOLSA_MIN);
    gfx_texto(x0 + dx + 56, y0 + 30, sub, C_TEXTO_M, GFX_FINO, false);
    int pr = s == 0 ? preco_troca() : preco_remove();
    if (pode) lj_preco(x0 + LJ_SW - 26, y0 + 15, pr, run.moedas >= pr, nega);
    zona_clique(LJ_SX, y0, LJ_SW, LJ_SH, &cursor, item, KEY_ENTER);
}

// O dado comprado no ar: sobe em arco, gira e encolhe ate a bolsa da faixa,
// deixando um rastro de brilhos.
static void lj_voos(void)
{
    for (int i = 0; i < VITRINE_MAX; i++) {
        if (!lj_voo[i].vivo) continue;
        float q = lj_voo[i].t / LJ_VOO_T, s = q * q * (3 - 2 * q);
        float bx = (float)faixa_bolsa_x, by = (float)faixa_bolsa_y;
        for (int k = 4; k >= 0; k--) {
            float qk = s - k * 0.05f;
            if (qk < 0) continue;
            float x = lj_voo[i].x0 + (bx - lj_voo[i].x0) * qk;
            float y = lj_voo[i].y0 + (by - lj_voo[i].y0) * qk - 70 * 4 * qk * (1 - qk);
            if (k) gfx_rect((int)x - 1, (int)y - 1, 3 - (k > 2), 3 - (k > 2), k % 2 ? C_OURO : C_AMARELO);
            else desenha_dado(x, y, 24 - 15 * s, lj_voo[i].p.tipo, 0, q * 9, lj_voo[i].p.fixo, 0, false);
        }
    }
}

static void lj_flutuantes(void)
{
    for (int i = 0; i < 6; i++) {
        if (lj_flut[i].t <= 0) continue;
        float q = lj_flut[i].t / 0.9f;
        uint16_t c = q < 0.7f ? lj_flut[i].cor : gfx_mistura(lj_flut[i].cor, C_FELTRO, (int)((q - 0.7f) / 0.3f * 80));
        txt_sombra_c((int)lj_flut[i].x, (int)(lj_flut[i].y - q * 26), lj_flut[i].s, c, 1);
    }
}

static void lj_lascas(void)
{
    for (int i = 0; i < n_lasca; i++) {
        const lasca_t *l = &lasca[i];
        if (l->t > l->vida) continue;
        uint16_t c = l->t > l->vida * 0.7f ? gfx_mistura(l->cor, C_FELTRO, 55) : l->cor;
        gfx_rect((int)l->x, (int)l->y, l->tam, l->tam, c);
    }
}

// A proxima mesa, para comprar pensando nela.
static void lj_proxima(char *s, int n)
{
    int p = run.estado == R_LOJA_INI ? run.passo : run.passo + 1;
    if (run.estado != R_LOJA_INI && p >= mesas_do_salao()) {
        int a = run.andar + 1;
        if (a == DES_ANDAR_FINAL && run.dificuldade >= N_NIVEIS) snprintf(s, (size_t)n, "A seguir: O CORINGA");
        else if (a < DES_ANDARES) snprintf(s, (size_t)n, "A seguir: o %s", SALAO[a]);
        else s[0] = 0;
        return;
    }
    const no_t *no = &run.no[p];
    snprintf(s, (size_t)n, "Próxima mesa: %s  ·  vale %d %s", PERSO[no->perso].nome,
             MOEDAS_MESA[run.andar][p], MOEDAS_MESA[run.andar][p] == 1 ? "moeda" : "moedas");
}

static void lj_desenha_tirando(void)
{
    char s[64];
    txt_c2(GFX_W / 2, MESA_Y0 + 2, "TIRAR UM DADO", C_VINHO_CLR);
    snprintf(s, sizeof s, "Qual sai para sempre?  ·  custa %d %s", preco_remove(), preco_remove() == 1 ? "moeda" : "moedas");
    txt_c(GFX_W / 2, MESA_Y0 + 40, s, C_MARFIM_S, false);
    grade_t g = grade_lj_tira(run.n_col);
    float f = g.f;
    int sel = (int)(22 * f), meio = (int)(16 * f);
    bool estourou = t_lj_volta > 0;
    for (int i = 0; i < run.n_col; i++) {
        int x, y;
        lj_pos_tira(i, &x, &y);
        if (i == lj_tira_i && t_lj_tira > 0) continue;  // esse treme por cima
        bool cur = i == cursor && t_lj_tira <= 0 && !estourou;
        if (cur) ui_selecao(x - sel, y - sel, 2 * sel, 2 * sel, C_VINHO_CLR);
        if (cur) brilho_passa = mola_brilho();
        peca_parada((float)x, (float)(y - (cur ? mola_px() : 0)), 17 * f, run.col[i], 0);
        if (!cur) marca_rar(x, y + (int)(23 * f), run.col[i].tipo);
        else gfx_rect(x - meio, y + (int)(23 * f), 2 * meio, 3, C_VINHO_CLR);
        zona_clique(x - sel, y - sel, 2 * sel, 2 * sel, &cursor, i, KEY_ENTER);
    }
    if (t_lj_tira > 0) {                         // o que sai: treme cada vez mais e racha
        float q = t_lj_tira / LJ_TIRA_T;
        int dx = (int)(sinf(t_lj_tira * 90) * (1 + 3 * q)), x = lj_tira_x + dx, y = lj_tira_y;
        ui_selecao(x - sel, y - sel, 2 * sel, 2 * sel, C_VINHO_CLR);
        peca_parada((float)x, (float)y, 17 * f, lj_tirado, 0);
        if (q > 0.45f) {
            int r = (int)(15 * f);
            gfx_linha_grossa(x - r, y - r / 2, x - 2, y, 2, C_BARRA);
            gfx_linha_grossa(x - 2, y, x + 3, y - r, 2, C_BARRA);
            gfx_linha_grossa(x - 2, y, x + r / 2, y + r, 2, C_BARRA);
        }
    }
    lj_lascas();
    int tc = estourou || t_lj_tira > 0 ? lj_tirado.tipo : run.col[cursor < run.n_col ? cursor : 0].tipo;
    int linhas = (run.n_col + g.por - 1) / g.por, ty = LJ_TIRA_Y + (int)(linhas * 52 * f) + 22;
    if (ty < 236) ty = 236;
    txt_c(GFX_W / 2, ty, TIPO[tc].nome, cor_dado(tc), true);
    txt_c(GFX_W / 2, ty + 22, estourou || t_lj_tira > 0 ? "saiu da bolsa para sempre" : TEXTO_MEDIO[tc],
          estourou || t_lj_tira > 0 ? C_VINHO_CLR : C_MARFIM_S, false);
    lj_flutuantes();
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar", KEY_BKSP);
    rodape_run("setas escolhem  ·  ENTER tira para sempre  ·  I detalhes  ·  BACKSPACE volta");
}

static void desenha_loja_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    faixa_run(0);
    if (lj_tirando) { lj_desenha_tirando(); return; }
    bool ini = run.estado == R_LOJA_INI;
    txt_c2(GFX_W / 2, MESA_Y0 + 2, ini ? "LOJA DA ENTRADA" : "LOJA", C_OURO);
    char s[96];
    lj_proxima(s, sizeof s);
    txt_c(GFX_W / 2, MESA_Y0 + 36, s, C_MARFIM_S, false);
    // As moedas, grandes, no canto: contam quando gasta.
    snprintf(s, sizeof s, "%d", moedas_vis());
    uint16_t cm = moedas_ok && moedas_cont.pulo > 0
                ? gfx_mistura(C_OURO, moedas_cont.sinal > 0 ? C_BRANCO : C_VINHO_CLR, (int)(moedas_cont.pulo * 70)) : C_OURO;
    moeda_ic(MESA_X0 + 30, MESA_Y0 + 24);
    gfx_texto(MESA_X0 + 42, MESA_Y0 + 12 - (moedas_ok && moedas_cont.pulo > 0.4f ? 1 : 0), s, cm, 1, true);
    gfx_texto(MESA_X0 + 22, MESA_Y0 + 36, "moedas", C_TEXTO_M, GFX_MIUDO, false);

    gfx_texto(LJ_VIT_CX - 116 * n_vitrine() / 2 + 4, LJ_Y - 11, "VITRINE", C_TEXTO_M, GFX_MIUDO, false);
    gfx_texto(LJ_SX + 4, LJ_Y - 11, "SERVIÇOS", C_TEXTO_M, GFX_MIUDO, false);
    for (int i = 0; i < n_vitrine(); i++) lj_cartao_dado(i);
    for (int k = 0; k < 2; k++) lj_servico(k);

    // O que o item em foco faz (ou por que nao deu).
    int nv = n_vitrine();
    if (cursor < nv) dado_e_poder(LJ_DESC_Y, run.vitrine[cursor].tipo, true);
    else if (cursor == nv) {
        txt_c(GFX_W / 2, LJ_DESC_Y, "Trocar a vitrine", C_LATAO, true);
        txt_c(GFX_W / 2, LJ_DESC_Y + TXT_H - 1, "Sorteia outros dados para a vitrine. Cada troca nesta loja custa 1 a mais.",
              C_MARFIM_S, false);
    } else {
        txt_c(GFX_W / 2, LJ_DESC_Y, "Tirar um dado da bolsa", C_VINHO_CLR, true);
        txt_c(GFX_W / 2, LJ_DESC_Y + TXT_H - 1, "Bolsa menor: os seus melhores dados vêm mais vezes. Cada um tirado na run custa 2 a mais.",
              C_MARFIM_S, false);
    }
    if (t_lj_aviso > 0) txt_c(GFX_W / 2, LJ_DESC_Y + 2 * TXT_H, lj_aviso, C_VINHO_CLR, true);
    lj_voos();
    lj_flutuantes();
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "seguir »", KEY_TAB);
    rodape_run("setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB segue");
}
