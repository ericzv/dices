// ---------------------------------------------------------------------------
// A loja da run, depois de cada mesa vencida: a vitrine (2 dados, 3 com a
// Cartola), o pacote (dados embrulhados de um grupo), o adesivo (vai num lado
// de um dado da bolsa), trocar a vitrine e tirar um dado da bolsa. Cada acao
// responde na hora, com movimento: a vitrine cai do alto quando a loja abre
// (e de novo quando troca, girando); o dado comprado voa ate a bolsa da
// faixa, e o carimbo de vendido bate no cartao; sem moedas, o preco treme; o
// dado tirado treme, racha e estoura em lascas; o pacote cresce no meio da
// mesa, sacode, rasga e solta tres cartas; o adesivo desce sobre o lado
// escolhido e assenta com um estalo.
// ---------------------------------------------------------------------------
#define LJ_Y      118                    // o alto dos cartoes
#define LJ_CH     118
#define LJ_SEC    14                     // entre a vitrine, o pacote e o adesivo, e os servicos
#define LJ_SW     170                    // os servicos, a direita
#define LJ_SH     55
#define LJ_DESC_Y 250                    // o que o item em foco faz
#define LJ_VOO_T  0.5f                   // o dado comprado ate a bolsa
#define LJ_TIRA_T 0.32f                  // o dado que sai: tremendo ate estourar
#define LJ_LASCAS 40

// O pacote aberto: cresce ate o meio, sacode, rasga e as cartas voam.
#define PAC_CRESCE 0.22f
#define PAC_RASGA  0.62f                 // o papel rasga
#define PAC_VOA    0.30f                 // cada carta ate o lugar
#define PAC_PRONTO (PAC_RASGA + 0.16f + PAC_VOA)   // a ultima pousou: escolhe
#define PAC_FIM    0.8f                  // depois de escolher (ou pular), ate voltar a loja
#define PAC_CW     112                   // as cartas do pacote aberto
#define PAC_PASSO  134
#define PAC_CX     (GFX_W / 2)
#define PAC_CY     166

// Colando o adesivo: ele desce sobre o lado escolhido e assenta.
#define LJ_COLA_BATE 0.22f               // encosta no dado (o estalo)
#define LJ_COLA_FIM  1.0f                // depois disso, a loja volta
#define LJ_LADO_Y    142                 // a primeira fileira de lados
#define LJ_LADO_DY   70                  // entre uma fileira e a outra

static bool  lj_tirando;                 // escolhendo o dado que sai
static float t_lj_entra = 9;             // desde que a vitrine (re)apareceu
static float t_lj_abriu = 9;             // desde que a loja abriu (os servicos entram uma vez)
static uint8_t lj_caiu;                  // cartoes que ja bateram na mesa (o som, uma vez)
static float t_lj_nega[VITRINE_MAX + 4]; // > 0: o item treme (sem moedas, sem lugar)
static float t_lj_vendido[VITRINE_MAX];  // desde a compra: o carimbo bate
static char  lj_aviso[64];               // por que nao deu
static float t_lj_aviso;
typedef struct { peca_t p; float x0, y0, t; bool vivo; } lj_voo_t;
static lj_voo_t lj_voo[VITRINE_MAX + 1]; // o ultimo e o do pacote
static peca_t lj_tirado;                 // o dado que sai, enquanto treme
static int   lj_tira_i = -1, lj_tira_x, lj_tira_y;
static float t_lj_tira, t_lj_volta;      // tremendo; depois do estouro, ate voltar a loja
typedef struct { float x, y, vx, vy, t, vida; uint16_t cor; uint8_t tam; } lasca_t;
static lasca_t lasca[LJ_LASCAS];
static int n_lasca;
typedef struct { float x, y, t; char s[16]; uint16_t cor; } lj_flut_t;
static lj_flut_t lj_flut[6];
static bool  lj_pac;                     // o pacote aberto: escolhendo 1 dos 3
static float t_pac;                      // desde que comecou a abrir
static float t_pac_fim;                  // > 0: escolheu (ou pulou); as outras somem
static int   pac_levado;                 // a carta levada (-1: pulou)
static uint8_t pac_caiu;                 // cartas que ja pousaram (o som, uma vez)
static uint8_t pac_bateu;                // as batidas do que tem dentro, sacudindo
static float t_pac_carimbo;              // o carimbo de aberto no cartao do pacote
static int   lj_colando;                 // colando o adesivo: 1 escolhendo o dado, 2 o lado
static int   lj_cola_dado;               // o dado escolhido (posicao na bolsa)
static int   lj_cola_lado;               // o lado onde colou (a animacao)
static float t_lj_cola;                  // > 0: colou; o adesivo assenta e a loja volta
static float t_lj_ad_carimbo;            // o carimbo de colado no cartao do adesivo

// Os itens, na ordem das setas: a vitrine, o pacote, o adesivo e os dois servicos.
static int lj_n_itens(void) { return n_vitrine() + 4; }
static int lj_item_pacote(void) { return n_vitrine(); }
static int lj_item_adesivo(void) { return n_vitrine() + 1; }
static int lj_item_troca(void) { return n_vitrine() + 2; }
static int lj_item_tira(void) { return n_vitrine() + 3; }
// Com a Cartola, tres cartoes na vitrine: todos um pouco mais estreitos.
static int lj_cw(void) { return n_vitrine() > 2 ? 78 : 88; }
static int lj_vao(void) { return n_vitrine() > 2 ? 6 : 8; }
static int lj_x0(void)
{
    int nv = n_vitrine(), total = nv * lj_cw() + (nv - 1) * lj_vao() + LJ_SEC + 2 * lj_cw() + lj_vao() + LJ_SEC + LJ_SW;
    return (MESA_X0 + MESA_X1) / 2 - total / 2;
}
// O meio do cartao i: os da vitrine e, depois deles, o do pacote e o do adesivo.
static int lj_card_x(int i)
{
    int nv = n_vitrine(), x = lj_x0() + lj_cw() / 2 + i * (lj_cw() + lj_vao());
    return i < nv ? x : x - lj_vao() + LJ_SEC;
}
static int lj_sx(void) { return lj_card_x(lj_item_adesivo()) + lj_cw() / 2 + LJ_SEC; }
static int lj_serv_y(int s) { return LJ_Y + s * (LJ_SH + 8); }
static int pac_carta_x(int i) { return PAC_CX + (i - 1) * PAC_PASSO; }

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
    t_pac_carimbo = 0;
    lj_colando = 0;
    t_lj_cola = t_lj_ad_carimbo = 0;
    // Saiu com o pacote aberto: volta nele, rasgado, com as mesmas cartas.
    lj_pac = run.pac_estado == PAC_ABERTO;
    t_pac = PAC_RASGA;
    t_pac_fim = 0;
    pac_levado = -1;
    pac_caiu = 0;
    pac_bateu = 0xFF;
    if (lj_pac) cursor = 0;
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
    if (item >= 0 && item < VITRINE_MAX + 4) t_lj_nega[item] = 0.4f;
    snprintf(lj_aviso, sizeof lj_aviso, "%s", porque);
    t_lj_aviso = 1.6f;
    som_toca_tom(SOM_TIQUE, -7);
}

// Lascas que voam de (x, y) e caem: n delas, nas tres cores.
static void lj_lascas_de(int x, int y, const uint16_t cores[3], int n, float forca)
{
    n_lasca = 0;
    for (int i = 0; i < n && n_lasca < LJ_LASCAS; i++) {
        lasca_t *l = &lasca[n_lasca++];
        float a = frnd(0, 6.2832f), v = frnd(60, 190) * forca;
        l->x = (float)x + cosf(a) * 6; l->y = (float)y + sinf(a) * 6;
        l->vx = cosf(a) * v; l->vy = sinf(a) * v - 90;
        l->t = 0; l->vida = frnd(0.45f, 0.85f);
        l->cor = cores[i % 3];
        l->tam = (uint8_t)(i % 3 == 2 ? 2 : 3 + (i % 2));
    }
}

static void lj_tira_confirma(void);
static void lj_cola_estala(void);
static void lj_estoura(void)
{
    // As lascas: a cor do dado, o marfim do corpo e o po.
    uint16_t cores[3] = { cor_dado(lj_tirado.tipo), C_MARFIM_S, C_TEXTO_M };
    lj_lascas_de(lj_tira_x, lj_tira_y, cores, 22, 1);
}

// O papel rasga: pedacos do embrulho e da fita voam do meio da mesa,
// maiores que as lascas de um dado.
static void pac_rasga(void)
{
    const uint16_t *p = PAPEL_GRUPO[run.pacote < GR_N ? run.pacote : 0];
    uint16_t cores[3] = { p[1], p[0], C_MARFIM_S };
    lj_lascas_de(PAC_CX, PAC_CY, cores, LJ_LASCAS, 1.6f);
    for (int i = 0; i < n_lasca; i++) lasca[i].tam = (uint8_t)(lasca[i].tam + 2);
    som_toca(SOM_PREMIO);
}

// O dado (vitrine ou pacote) voa ate a bolsa da faixa.
static void lj_voa(int slot, peca_t p, int x, int y)
{
    lj_voo[slot] = (lj_voo_t){ p, (float)x, (float)y, 0, true };
    bolsa_voando++;
}

static void loja_passo(float dt)
{
    if (fase != F_RLOJA) return;
    t_lj_entra += dt;
    t_lj_abriu += dt;
    if (t_lj_aviso > 0) t_lj_aviso -= dt;
    for (int i = 0; i < VITRINE_MAX + 4; i++) if (t_lj_nega[i] > 0) t_lj_nega[i] -= dt;
    for (int i = 0; i < VITRINE_MAX; i++) if (t_lj_vendido[i] > 0) t_lj_vendido[i] += dt;
    if (t_pac_carimbo > 0) t_pac_carimbo += dt;
    if (t_lj_ad_carimbo > 0) t_lj_ad_carimbo += dt;
    // Os cartoes batendo na mesa, um depois do outro (o pacote e o adesivo por ultimo).
    for (int i = 0; i <= lj_item_adesivo(); i++) {
        float q = (t_lj_entra - i * 0.08f) / 0.34f;
        if (i >= n_vitrine()) q = (t_lj_abriu - i * 0.08f) / 0.34f;
        if (q >= 0.75f && !(lj_caiu >> i & 1)) { lj_caiu |= (uint8_t)(1 << i); som_toca_tom(SOM_BATE_DADO, i * 2); }
    }
    // O adesivo colado: desce, assenta com um estalo, e a loja volta.
    if (t_lj_cola > 0) {
        float antes = t_lj_cola;
        t_lj_cola += dt;
        if (antes < LJ_COLA_BATE && t_lj_cola >= LJ_COLA_BATE) lj_cola_estala();
        if (t_lj_cola >= LJ_COLA_FIM) {
            t_lj_cola = 0;
            lj_colando = 0;
            cursor = lj_item_adesivo();
            t_lj_ad_carimbo = 0.001f;
        }
    }
    // O dado comprado chegando na bolsa.
    for (int i = 0; i <= VITRINE_MAX; i++) {
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
    // O pacote: o que tem dentro bate enquanto ele sacode; rasga; as cartas
    // pousam uma a uma; escolhido, volta a loja.
    if (lj_pac) {
        float antes = t_pac;
        t_pac += dt;
        static const float BATIDA[3] = { 0.30f, 0.43f, 0.53f };
        for (int k = 0; k < 3; k++)
            if (antes < BATIDA[k] && t_pac >= BATIDA[k] && !(pac_bateu >> k & 1)) {
                pac_bateu |= (uint8_t)(1 << k);
                som_toca_tom(SOM_BATE_DADO, -6 + 2 * k);
            }
        if (antes < PAC_RASGA && t_pac >= PAC_RASGA) pac_rasga();
        for (int i = 0; i < PAC_OPCOES; i++) {
            float q = (t_pac - PAC_RASGA - i * 0.08f) / PAC_VOA;
            if (q >= 0.8f && !(pac_caiu >> i & 1)) { pac_caiu |= (uint8_t)(1 << i); som_toca_tom(SOM_BATE_DADO, 2 + 3 * i); }
        }
        if (t_pac_fim > 0 && (t_pac_fim += dt) >= PAC_FIM) {
            lj_pac = false;
            t_pac_fim = 0;
            cursor = lj_item_pacote();
            t_pac_carimbo = 0.001f;
        }
    }
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
        cursor = lj_item_tira();
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
    lj_voa(i, run.vitrine[i], lj_card_x(i), LJ_Y + 40);
    lj_flutua((float)lj_card_x(i), (float)(LJ_Y + LJ_CH - 18), C_VINHO_CLR, "-%d", pr);
    mola_chuta();
    som_toca(SOM_FICHA);
}

// Compra o pacote: paga e ele abre na hora (fica salvo aberto: sair e voltar
// mostra as mesmas cartas).
static void loja_pacote(void)
{
    int it = lj_item_pacote(), pr = preco_pacote();
    if (run.pacote >= GR_N) { lj_nega(it, "Nenhum pacote nesta loja."); return; }
    if (run.pac_estado != PAC_VENDA) { lj_nega(it, "O pacote desta loja já foi aberto."); return; }
    if (run.moedas < pr) { lj_nega(it, "Moedas insuficientes."); return; }
    if (run.n_col >= MAX_COL) { lj_nega(it, "A bolsa está cheia."); return; }
    run.moedas -= pr;
    run.pac_estado = PAC_ABERTO;
    des_salva();
    lj_pac = true;
    t_pac = 0;
    t_pac_fim = 0;
    pac_levado = -1;
    pac_caiu = 0;
    pac_bateu = 0;
    cursor = 1;                                  // a carta do meio
    som_toca(SOM_FICHA);
}

// Leva a carta i do pacote (ou pula, i < 0): o pacote acaba nesta loja.
static void pacote_escolhe(int i)
{
    if (!lj_pac || t_pac_fim > 0 || run.pac_estado != PAC_ABERTO) return;
    if (i >= 0 && i < PAC_OPCOES) {
        if (run.n_col >= MAX_COL) { lj_nega(-1, "A bolsa está cheia."); return; }
        run.col[run.n_col++] = run.pac_op[i];
        lj_voa(VITRINE_MAX, run.pac_op[i], pac_carta_x(i), LJ_Y + 40);
        mola_chuta();
        som_toca(SOM_VALIDO);
    } else som_toca(SOM_CORREU);
    pac_levado = i;
    run.pac_estado = PAC_USADO;
    des_salva();
    t_pac_fim = 0.001f;
}

// ---- O adesivo ---------------------------------------------------------------
// Algum dado da bolsa ainda tem lado livre?
static bool lj_algum_lado(void)
{
    for (int i = 0; i < run.n_col; i++) if (ad_cabe(&run.col[i])) return true;
    return false;
}

// ENTER no adesivo: escolhe o dado (paga so ao colar; voltar nao custa nada).
static void loja_adesivo(void)
{
    int it = lj_item_adesivo(), a = run.adesivo;
    if (a <= AD_NADA || a >= AD_N) { lj_nega(it, "Nenhum adesivo nesta loja."); return; }
    if (run.ad_vendido) { lj_nega(it, "O adesivo desta loja já foi colado."); return; }
    if (run.moedas < preco_adesivo(a)) { lj_nega(it, "Moedas insuficientes."); return; }
    if (!lj_algum_lado()) { lj_nega(it, "Nenhum dado da bolsa tem lado livre."); return; }
    lj_colando = 1;
    cursor = 0;
    while (cursor < run.n_col - 1 && !ad_cabe(&run.col[cursor])) cursor++;
    som_toca(SOM_TIQUE);
}

// O dado i: abre os lados dele (o cursor no primeiro livre).
static void cola_escolhe_dado(int i)
{
    if (i < 0 || i >= run.n_col) return;
    if (!ad_cabe(&run.col[i])) { lj_nega(-1, "Todos os lados desse dado já têm adesivo."); return; }
    lj_cola_dado = i;
    lj_colando = 2;
    int t = run.col[i].tipo;
    cursor = 0;
    while (cursor < ad_n_lados(t) - 1 && ad_no_lado(&run.col[i], ad_lado_min(t) + cursor)) cursor++;
    som_toca(SOM_TIQUE);
}

// Cola no lado c (contando do primeiro): paga, fica salvo, e ele desce sobre o lado.
static void cola_escolhe_lado(int c)
{
    if (lj_colando != 2 || t_lj_cola > 0 || lj_cola_dado < 0 || lj_cola_dado >= run.n_col) return;
    peca_t *p = &run.col[lj_cola_dado];
    int lado = ad_lado_min(p->tipo) + c, a = run.adesivo, pr = preco_adesivo(a);
    if (c < 0 || lado > ad_lado_max(p->tipo)) return;
    if (ad_no_lado(p, lado)) { lj_nega(-1, "Esse lado já tem adesivo."); return; }
    if (run.moedas < pr || run.ad_vendido) return;
    run.moedas -= pr;
    ad_cola(p, lado, a);
    run.ad_vendido = 1;
    des_salva();
    lj_cola_lado = lado;
    t_lj_cola = 0.001f;
    som_toca(SOM_FICHA);
}

static void loja_troca(void)
{
    int it = lj_item_troca(), pr = preco_troca();
    if (run.moedas < pr) { lj_nega(it, "Moedas insuficientes."); return; }
    run.moedas -= pr;
    if (run.trocas < 255) run.trocas++;
    monta_vitrine();
    des_salva();
    t_lj_entra = 0;                              // a vitrine nova cai, girando
    lj_caiu &= (uint8_t)(3 << n_vitrine());      // (o pacote e o adesivo ficam)
    memset(t_lj_vendido, 0, sizeof t_lj_vendido);
    lj_flutua((float)(lj_sx() + LJ_SW - 26), (float)(lj_serv_y(0) - 4), C_VINHO_CLR, "-%d", pr);
    som_toca(SOM_FICHA);
}

static void loja_tira_abre(void)
{
    int it = lj_item_tira();
    if (run.n_col <= BOLSA_MIN) { lj_nega(it, "A bolsa já está no mínimo."); return; }
    if (run.moedas < preco_remove()) { lj_nega(it, "Moedas insuficientes."); return; }
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

// O pacote aberto: as setas passam pelas tres cartas e pelo "pular"; ENTER
// leva (ou pula). Antes das cartas pousarem, ENTER adianta a animacao.
static bool pacote_tecla(int k, int e)
{
    if (t_pac_fim > 0) return true;
    if (t_pac < PAC_PRONTO) {
        if (k == KEY_ENTER) t_pac = t_pac < PAC_RASGA ? PAC_RASGA - 0.01f : PAC_PRONTO;
        return true;
    }
    if (e) { cursor = (cursor + e + PAC_OPCOES + 1) % (PAC_OPCOES + 1); som_toca(SOM_TIQUE); return true; }
    if (k == KEY_BKSP || k == 'x' || k == KEY_TAB) { cursor = PAC_OPCOES; som_toca(SOM_TIQUE); return true; }
    if (k >= '1' && k < '1' + PAC_OPCOES) { cursor = k - '1'; pacote_escolhe(cursor); return true; }
    if (k == KEY_ENTER) pacote_escolhe(cursor < PAC_OPCOES ? cursor : -1);
    return true;
}

// Colando: as setas passam pelos dados da bolsa (depois, pelos lados); ENTER
// escolhe; BACKSPACE volta um passo.
static int lj_lados_por(int n) { return n <= 12 ? n : 10; }
static bool cola_tecla(int k, int e)
{
    if (t_lj_cola > 0) return true;
    bool volta = k == KEY_BKSP || k == 'x' || k == KEY_TAB;
    if (lj_colando == 1) {
        int por = grade_lj_tira(run.n_col).por;
        if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= por) cursor -= por; return true; }
        if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + por < run.n_col) cursor += por; return true; }
        if (e) { cursor = (cursor + e + run.n_col) % run.n_col; som_toca(SOM_TIQUE); return true; }
        if (volta) { lj_colando = 0; cursor = lj_item_adesivo(); return true; }
        if (k == KEY_ENTER) cola_escolhe_dado(cursor);
        return true;
    }
    int n = ad_n_lados(run.col[lj_cola_dado].tipo), por = lj_lados_por(n);
    if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= por) cursor -= por; return true; }
    if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + por < n) cursor += por; return true; }
    if (e) { cursor = (cursor + e + n) % n; som_toca(SOM_TIQUE); return true; }
    if (volta) { lj_colando = 1; cursor = lj_cola_dado; return true; }
    if (k == KEY_ENTER) cola_escolhe_lado(cursor);
    return true;
}

static bool loja_tecla(int k, int e)
{
    if (lj_pac) return pacote_tecla(k, e);
    if (lj_colando) return cola_tecla(k, e);
    if (lj_tirando) {
        if (t_lj_tira > 0 || t_lj_volta > 0) return true;      // estourando
        int por = grade_lj_tira(run.n_col).por;
        if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= por) cursor -= por; return true; }
        if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + por < run.n_col) cursor += por; return true; }
        if (e) { cursor = (cursor + e + run.n_col) % run.n_col; som_toca(SOM_TIQUE); return true; }
        if (k == KEY_BKSP || k == 'x' || k == KEY_TAB) { lj_tirando = false; cursor = lj_item_tira(); return true; }
        if (k == KEY_ENTER) loja_tira(cursor);
        return true;
    }
    int nv = n_vitrine(), nc = lj_n_itens();
    if (e) { cursor = (cursor + e + nc) % nc; som_toca(SOM_TIQUE); return true; }
    if (k >= '1' && k < '1' + nc) cursor = k - '1';
    if (k == KEY_ENTER || (k >= '1' && k < '1' + nc)) {
        if (cursor < nv) loja_compra(cursor);
        else if (cursor == lj_item_pacote()) loja_pacote();
        else if (cursor == lj_item_adesivo()) loja_adesivo();
        else if (cursor == lj_item_troca()) loja_troca();
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

// O carimbo (vendido, aberto): bate grande e assenta, torto.
static void lj_carimbo(int cx, int cy, float t, const char *s)
{
    float q = t / 0.14f;
    if (q > 1) q = 1;
    int cresce = (int)((1 - q) * 14);
    int w = gfx_largura(s, 1) + 16 + 2 * cresce, h = TXT_H + 6 + cresce;
    int x = cx - w / 2, y = cy - h / 2;
    for (int k = 0; k < 2; k++) gfx_moldura(x + k * 2 - 1, y + k - 1, w, h, 2, k ? C_VINHO_CLR : C_SOMBRA);
    gfx_rect_alfa(x + 2, y + 2, w - 4, h - 4, C_SOMBRA, 120);
    txt_c(cx + 1, y + 3 + cresce / 2, s, C_VINHO_CLR, true);
}

static void lj_cartao_dado(int i)
{
    int x = lj_card_x(i), cw = lj_cw();
    float q = (t_lj_entra - i * 0.08f) / 0.34f;
    if (q <= 0) return;
    int y = LJ_Y - (int)((1 - sai_mola(q)) * 80);
    peca_t pc = run.vitrine[i];
    int pr = preco_run(pc.tipo);
    bool ja = run.levou[i], cur = i == cursor && !lj_tirando, da = run.moedas >= pr;
    float nega = t_lj_nega[i];
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 2 * (nega / 0.4f)) : 0;
    ui_cartao(x - cw / 2 + dx, y, cw, LJ_CH, cur, cor_rar(pc.tipo));
    float giro = q < 1 ? (1 - q) * 7 : 0;           // caindo, o dado gira
    float bob = cur && !ja ? sinf(t_fase * 4) * 3 - mola_px() : 0;
    if (!ja || t_lj_vendido[i] < 0.12f) {
        if (cur && !ja) brilho_passa = mola_brilho();
        marca_nivel = pc.fixo;
        desenha_dado((float)(x + dx), y + 40 + bob, 24, pc.tipo, 0, giro, pc.fixo, 0, false);
        marca_nivel = -1;
    }
    nome_no_cartao(x + dx, y + 68, TIPO[pc.tipo].nome, cur ? cor_dado(pc.tipo) : C_MARFIM_S, cw - 8);
    const char *rar = RARIDADE[TIPO[pc.tipo].raridade];
    gfx_texto(x + dx - gfx_largura(rar, GFX_MIUDO) / 2, y + 85, rar, cor_rar(pc.tipo), GFX_MIUDO, false);
    if (ja) {
        gfx_rect_alfa(x - cw / 2 + 1, y + 1, cw - 2, LJ_CH - 2, C_SOMBRA, 110);
        lj_carimbo(x, y + LJ_CH - 22, t_lj_vendido[i] > 0 ? t_lj_vendido[i] : 1, "VENDIDO");   // ja vendido: assentado
    } else lj_preco(x, y + LJ_CH - 24, pr, da, nega);
    zona_clique(x - cw / 2, LJ_Y, cw, LJ_CH, &cursor, i, KEY_ENTER);
}

// O simbolo do grupo, na etiqueta do pacote: barras que sobem (Numeros), a
// lamina (Ataque), o escudo (Defesa), dois aros presos (Combo). k: o tamanho
// de cada pixel.
static void glifo_grupo(int cx, int cy, int g, uint16_t c, int k)
{
    switch (g) {
    case GR_NUMEROS:
        gfx_rect(cx - 4 * k, cy + 1 * k, 2 * k, 3 * k, c);
        gfx_rect(cx - 1 * k, cy - 1 * k, 2 * k, 5 * k, c);
        gfx_rect(cx + 2 * k, cy - 3 * k, 2 * k, 7 * k, c);
        break;
    case GR_ATAQUE:
        gfx_linha_grossa(cx - 3 * k, cy + 3 * k, cx + 3 * k, cy - 3 * k, 2 * k, c);
        gfx_linha_grossa(cx - 4 * k, cy, cx, cy + 4 * k, k, c);   // a guarda
        break;
    case GR_DEFESA:
        gfx_rect(cx - 3 * k, cy - 3 * k, 7 * k, 4 * k, c);
        gfx_rect(cx - 2 * k, cy + 1 * k, 5 * k, 2 * k, c);
        gfx_rect(cx - 1 * k, cy + 3 * k, 3 * k, 1 * k, c);
        break;
    default: {                                   // dois aros entrelacados
        uint16_t fundo = C_MARFIM_S;
        gfx_disco(cx - 2 * k, cy, 3 * k, c);
        gfx_disco(cx - 2 * k, cy, k + (k > 1), fundo);
        gfx_disco(cx + 2 * k, cy, 3 * k, c);
        gfx_disco(cx + 2 * k, cy, k + (k > 1), fundo);
        break;
    }
    }
}

// O pacote: uma caixa embrulhada no papel do grupo, com a fita marfim, o
// laco em cima e a etiqueta com o simbolo. e: a escala (1 na loja); rasgado:
// a tampa arrancada e a caixa vazia.
static void desenha_pacote(int cx, int cy, float e, int g, bool rasgado)
{
    const uint16_t *pp = PAPEL_GRUPO[g >= 0 && g < GR_N ? g : 0];
    uint16_t luz = pp[0], papel = pp[1], sombra = pp[2];
    int w = (int)(44 * e), h = (int)(36 * e), x = cx - w / 2, y = cy - h / 2 + (int)(4 * e);
    int tampa = (int)(9 * e), fw = (int)(5 * e) < 3 ? 3 : (int)(5 * e), k = e >= 1.6f ? 2 : 1;
    gfx_rect(x + 3, y + h, w - 3, (int)(3 * e), C_SOMBRA);               // a sombra na mesa
    gfx_rect(x, y, w, h, C_BARRA);
    gfx_rect(x + 1, y + 1, w - 2, h - 2, papel);
    gfx_rect(x + w - 1 - (int)(6 * e), y + 1, (int)(6 * e), h - 2, sombra);   // o lado na sombra
    if (rasgado) {
        // A boca aberta, escura, com a borda do papel em dentes.
        gfx_rect(x + 2, y + 2, w - 4, tampa + 2, C_BARRA);
        for (int i = 0; i < w - 4; i += 3 * k) gfx_rect(x + 2 + i, y + tampa + 2, 2 * k, (i / (3 * k)) % 2 ? 2 * k : k, luz);
        gfx_rect(cx - fw / 2, y + tampa + 4, fw, h - tampa - 6, gfx_mistura(C_MARFIM_S, C_EBANO, 35));
        return;
    }
    gfx_rect(x + 1, y + 1, w - 2, tampa, luz);                         // a tampa
    gfx_rect(x + 1, y + 1 + tampa, w - 2, k, sombra);
    // Dois riscos de dobra no papel.
    gfx_rect(x + (int)(6 * e), y + tampa + (int)(6 * e), (int)(8 * e), k, luz);
    gfx_rect(x + w - (int)(18 * e), y + h - (int)(8 * e), (int)(7 * e), k, luz);
    // A fita: em pe e deitada, com um fio de luz.
    int fy = y + (int)(h * 0.58f);
    gfx_rect(cx - fw / 2, y + 1, fw, h - 2, C_MARFIM_S);
    gfx_rect(x + 1, fy, w - 2, fw, C_MARFIM_S);
    gfx_rect(cx - fw / 2, y + 1, k, h - 2, C_MARFIM);
    gfx_rect(x + 1, fy, w - 2, k, C_MARFIM);
    // O laco de borboleta: duas asas que abrem do no e duas pontas caidas.
    int bw = (int)(10 * e), bh = (int)(6 * e), ly = y - (int)(2 * e);
    for (int s = -1; s <= 1; s += 2) {
        gfx_linha_grossa(cx, ly, cx + s * (bw - 2 * k), ly + bh + (int)(3 * e), fw - 1, C_BARRA);
        gfx_linha_grossa(cx, ly, cx + s * (bw - 2 * k), ly + bh + (int)(3 * e), fw - 3 > 0 ? fw - 3 : 1, C_MARFIM_S);
        for (int d = 0; d <= bw; d++) {
            int hh = bh * d / bw;
            gfx_rect(cx + s * d - (s < 0), ly - hh - 1, 1, 2 * hh + 3, C_BARRA);
        }
        for (int d = 1; d < bw; d++) {
            int hh = bh * d / bw;
            gfx_rect(cx + s * d - (s < 0), ly - hh, 1, 2 * hh + 1, d > bw - 3 * k ? C_MARFIM : C_MARFIM_S);
        }
    }
    gfx_rect(cx - fw / 2 - 1, ly - fw / 2 - 1, fw + 2, fw + 2, C_BARRA);
    gfx_rect(cx - fw / 2, ly - fw / 2, fw, fw, C_MARFIM);
    // A etiqueta, presa na fita, com o simbolo do grupo.
    int tw = (int)(16 * e), th = (int)(13 * e), tx = x + w - tw + (int)(4 * e), ty = fy + fw - 1;
    gfx_rect(tx, ty, tw, th, C_BARRA);
    gfx_rect(tx + 1, ty + 1, tw - 2, th - 2, C_MARFIM_S);
    glifo_grupo(tx + tw / 2, ty + th / 2, g, papel, k);
}

// O cartao do pacote na loja: o embrulho (sacudindo de leve, de tempos em
// tempos, como se o que tem dentro se mexesse), o grupo e o preco.
static void lj_cartao_pacote(void)
{
    int it = lj_item_pacote(), x = lj_card_x(it), cw = lj_cw();
    float q = (t_lj_abriu - it * 0.08f) / 0.34f;
    if (q <= 0) return;
    int y = LJ_Y - (int)((1 - sai_mola(q)) * 80);
    int g = run.pacote;
    bool tem = g < GR_N, usado = run.pac_estado != PAC_VENDA, cur = it == cursor && !lj_tirando;
    float nega = t_lj_nega[it];
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 2 * (nega / 0.4f)) : 0;
    ui_cartao(x - cw / 2 + dx, y, cw, LJ_CH, cur, tem ? cor_grupo(g) : C_LATAO);
    zona_clique(x - cw / 2, LJ_Y, cw, LJ_CH, &cursor, it, KEY_ENTER);
    if (!tem) {
        txt_c(x, y + 50, "sem pacote", C_TEXTO_M, false);
        return;
    }
    float ciclo = fmodf(t_relogio + 0.7f, 2.6f);                         // a cada 2,6 s, um tremelique
    int mexe = !usado && ciclo < 0.3f ? (int)(sinf(ciclo * 60) * 2 * (1 - ciclo / 0.3f)) : 0;
    float bob = cur && !usado ? sinf(t_fase * 4) * 3 - mola_px() : 0;
    desenha_pacote(x + dx + mexe, y + 38 + (int)bob, cw > 96 ? 1.0f : 0.9f, g, usado);
    txt_c(x + dx, y + 62, "Pacote", cur ? C_MARFIM : C_MARFIM_S, false);
    txt_c(x + dx, y + 75, NOME_GRUPO[g], cor_grupo(g), true);
    if (usado) {
        gfx_rect_alfa(x - cw / 2 + 1, y + 1, cw - 2, LJ_CH - 2, C_SOMBRA, 110);
        lj_carimbo(x, y + LJ_CH - 17, t_pac_carimbo > 0 ? t_pac_carimbo : 1, "ABERTO");
    } else lj_preco(x, y + LJ_CH - 24, preco_pacote(), run.moedas >= preco_pacote(), nega);
}

// O cartao do adesivo: o adesivo da loja (de tempos em tempos a ponta descola
// um pouco, como quem quer sair), o nome e o preco.
static void lj_cartao_adesivo(void)
{
    int it = lj_item_adesivo(), x = lj_card_x(it), cw = lj_cw();
    float q = (t_lj_abriu - it * 0.08f) / 0.34f;
    if (q <= 0) return;
    int y = LJ_Y - (int)((1 - sai_mola(q)) * 80);
    int a = run.adesivo;
    bool tem = a > AD_NADA && a < AD_N, usado = run.ad_vendido, cur = it == cursor && !lj_tirando;
    float nega = t_lj_nega[it];
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 2 * (nega / 0.4f)) : 0;
    ui_cartao(x - cw / 2 + dx, y, cw, LJ_CH, cur, tem ? ADESIVO[a].luz : C_LATAO);
    zona_clique(x - cw / 2, LJ_Y, cw, LJ_CH, &cursor, it, KEY_ENTER);
    if (!tem) {
        txt_c(x, y + 50, "sem adesivo", C_TEXTO_M, false);
        return;
    }
    float ciclo = fmodf(t_relogio + 1.9f, 3.1f);
    float descola = !usado && ciclo < 0.5f ? sinf(ciclo / 0.5f * 3.1416f) : 0;
    float bob = cur && !usado ? sinf(t_fase * 4) * 3 - mola_px() : 0;
    ad_solto_d(x + dx, y + 36 + (int)bob, a, 3, descola);
    txt_c(x + dx, y + 62, "Adesivo", cur ? C_MARFIM : C_MARFIM_S, false);
    const char *nome = ADESIVO[a].nome;
    if (gfx_largura(nome, 1) <= cw - 8) txt_c(x + dx, y + 75, nome, ADESIVO[a].luz, true);
    else nome_no_cartao(x + dx, y + 78, nome, ADESIVO[a].luz, cw - 6);
    if (usado) {
        gfx_rect_alfa(x - cw / 2 + 1, y + 1, cw - 2, LJ_CH - 2, C_SOMBRA, 110);
        lj_carimbo(x, y + LJ_CH - 17, t_lj_ad_carimbo > 0 ? t_lj_ad_carimbo : 1, "COLADO");
    } else lj_preco(x, y + LJ_CH - 24, preco_adesivo(a), run.moedas >= preco_adesivo(a), nega);
}

// Os servicos: trocar a vitrine (as setas girando) e tirar um dado (o dado rachado).
static void lj_servico(int s)
{
    int item = lj_item_troca() + s, y0 = lj_serv_y(s), sx = lj_sx();
    float q = (t_lj_abriu - 0.12f - s * 0.08f) / 0.3f;
    if (q <= 0) return;
    int x0 = sx + (int)((1 - sai_mola(q)) * 120);
    bool cur = item == cursor && !lj_tirando;
    float nega = t_lj_nega[item];
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 2 * (nega / 0.4f)) : 0;
    bool pode = s == 0 || run.n_col > BOLSA_MIN;
    ui_cartao(x0 + dx, y0, LJ_SW, LJ_SH, cur, s == 0 ? C_LATAO : C_VINHO_CLR);
    int ix = x0 + dx + 28, iy = y0 + LJ_SH / 2;
    if (s == 0) {                                 // dois dados e a seta em volta
        float gira = cur ? t_fase * 2.5f : 0;
        peca_parada((float)(ix - 6), (float)(iy - 3), 9, (peca_t){ .tipo = D_D6, .fixo = 0 }, 0);
        peca_parada((float)(ix + 7), (float)(iy + 5), 8, (peca_t){ .tipo = D_D4, .fixo = 0 }, 0);
        for (int k = 0; k < 18; k++) {
            float a = gira + k * 0.29f;
            gfx_rect(ix + (int)(cosf(a) * 19), iy + (int)(sinf(a) * 19), 2, 2, k > 14 ? C_OURO : C_LATAO);
        }
    } else {                                      // um dado com a rachadura
        peca_parada((float)ix, (float)iy, 13, (peca_t){ .tipo = D_D6, .fixo = 0 }, 0);
        gfx_linha_grossa(ix - 8, iy - 10, ix - 1, iy - 2, 2, C_BARRA);
        gfx_linha_grossa(ix - 1, iy - 2, ix - 5, iy + 4, 2, C_BARRA);
        gfx_linha_grossa(ix - 5, iy + 4, ix + 4, iy + 11, 2, C_BARRA);
        if (!pode) gfx_rect_alfa(ix - 16, iy - 16, 32, 32, C_SOMBRA, 120);
    }
    const char *nome = s == 0 ? "Trocar a vitrine" : "Tirar um dado";
    txt(x0 + dx + 54, y0 + 8, nome, cur ? C_MARFIM : C_MARFIM_S, true);
    char sub[48];
    if (s == 0) snprintf(sub, sizeof sub, "outros dados");
    else if (pode) snprintf(sub, sizeof sub, "bolsa: %d dados", run.n_col - bolsa_voando);
    else snprintf(sub, sizeof sub, "bolsa no mínimo (%d)", BOLSA_MIN);
    gfx_texto(x0 + dx + 54, y0 + 30, sub, C_TEXTO_M, GFX_FINO, false);
    int pr = s == 0 ? preco_troca() : preco_remove();
    if (pode) lj_preco(x0 + LJ_SW - 22, y0 + 27, pr, run.moedas >= pr, nega);
    zona_clique(sx, y0, LJ_SW, LJ_SH, &cursor, item, KEY_ENTER);
}

// O dado comprado no ar: sobe em arco, gira e encolhe ate a bolsa da faixa,
// deixando um rastro de brilhos.
static void lj_voos(void)
{
    for (int i = 0; i <= VITRINE_MAX; i++) {
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

// ---- Colando o adesivo --------------------------------------------------------
// O lugar do lado c (contando do primeiro) na grade dos lados: ate 12 numa
// fileira; o d20, duas de 10.
static void lj_lado_pos(int c, int n, int *x, int *y)
{
    int por = lj_lados_por(n), lin = c / por, col = c % por;
    int na_linha = n - lin * por < por ? n - lin * por : por;
    *x = GFX_W / 2 + 40 + (int)((col - (na_linha - 1) / 2.0f) * 44);
    *y = LJ_LADO_Y + lin * LJ_LADO_DY;
}

// O estalo: o adesivo encostou no lado. Lascas de vinil e um som seco.
static void lj_cola_estala(void)
{
    int a = run.adesivo, n = ad_n_lados(run.col[lj_cola_dado].tipo), x, y;
    lj_lado_pos(lj_cola_lado - ad_lado_min(run.col[lj_cola_dado].tipo), n, &x, &y);
    uint16_t cores[3] = { ADESIVO[a].fundo == AD_SEM_FUNDO ? ADESIVO[a].luz : ADESIVO[a].fundo, C_BRANCO, ADESIVO[a].luz };
    lj_lascas_de(x, y, cores, 18, 0.8f);
    som_toca(som_cola());
    mola_chuta();
}

// O titulo das duas telas: o adesivo, o nome e o que ele faz.
static void lj_cola_titulo(const char *pergunta)
{
    int a = run.adesivo;
    char s[160];
    txt_c2(GFX_W / 2, MESA_Y0 + 2, "COLAR O ADESIVO", ADESIVO[a].luz);
    snprintf(s, sizeof s, "%s: %s %s  ·  %s", ADESIVO[a].nome, ADESIVO[a].l1, ADESIVO[a].l2, pergunta);
    txt_c(GFX_W / 2, MESA_Y0 + 40, s, C_MARFIM_S, false);
}

// 1: qual dado. Os sem lado livre ficam apagados.
static void lj_cola_dados(void)
{
    char s[64];
    lj_cola_titulo("em qual dado?");
    grade_t g = grade_lj_tira(run.n_col);
    float f = g.f;
    int sel = (int)(22 * f), meio = (int)(16 * f), a = run.adesivo;
    for (int i = 0; i < run.n_col; i++) {
        int x, y;
        lj_pos_tira(i, &x, &y);
        bool cur = i == cursor, cabe = ad_cabe(&run.col[i]);
        if (cur) ui_selecao(x - sel, y - sel, 2 * sel, 2 * sel, ADESIVO[a].luz);
        if (cur && cabe) brilho_passa = mola_brilho();
        peca_parada((float)x, (float)(y - (cur ? mola_px() : 0)), 17 * f, run.col[i], 0);
        if (!cabe) gfx_rect_alfa(x - sel, y - sel, 2 * sel, 2 * sel, C_SOMBRA, 150);
        if (cur) gfx_rect(x - meio, y + (int)(23 * f), 2 * meio, 3, ADESIVO[a].luz);
        else marca_rar(x, y + (int)(23 * f), run.col[i].tipo);
        zona_clique(x - sel, y - sel, 2 * sel, 2 * sel, &cursor, i, KEY_ENTER);
    }
    int linhas = (run.n_col + g.por - 1) / g.por, ty = LJ_TIRA_Y + (int)(linhas * 52 * f) + 22;
    if (ty < 236) ty = 236;
    const peca_t *pc = &run.col[cursor < run.n_col ? cursor : 0];
    int livres = ad_n_lados(pc->tipo) - ad_quantos(pc);
    txt_c(GFX_W / 2, ty, TIPO[pc->tipo].nome, cor_dado(pc->tipo), true);
    if (livres) snprintf(s, sizeof s, "%d %s de %d", livres, livres == 1 ? "lado livre" : "lados livres", ad_n_lados(pc->tipo));
    else snprintf(s, sizeof s, "todos os lados já têm adesivo");
    txt_c(GFX_W / 2, ty + 22, s, livres ? C_MARFIM_S : C_VINHO_CLR, false);
    if (t_lj_aviso > 0) txt_c(GFX_W / 2, ty + 22 + TXT_H, lj_aviso, C_VINHO_CLR, true);
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar", KEY_BKSP);
    rodape_run("setas escolhem  ·  ENTER escolhe o dado  ·  I detalhes  ·  BACKSPACE volta");
}

// 2: qual lado. O dado inteiro, um lado ao lado do outro, com os adesivos que
// ja tem; o novo paira sobre o lado da vez e, escolhido, desce e assenta.
static void lj_cola_lados(void)
{
    const peca_t *pc = &run.col[lj_cola_dado];
    int t = pc->tipo, n = ad_n_lados(t), a = run.adesivo;
    char s[96];
    snprintf(s, sizeof s, "em qual lado do %s?", TIPO[t].nome);
    lj_cola_titulo(s);
    peca_parada(64, LJ_LADO_Y, 22, *pc, 0);                // o dado, com os selinhos
    txt_c(64, LJ_LADO_Y + 32, TIPO[t].nome, cor_dado(t), true);
    for (int c = 0; c < n; c++) {
        int lado = ad_lado_min(t) + c, x, y, ja = ad_no_lado(pc, lado);
        lj_lado_pos(c, n, &x, &y);
        bool cur = c == cursor, colando = t_lj_cola > 0 && lado == lj_cola_lado;
        float pulo = cur && !t_lj_cola ? mola_px() : 0;
        if (colando && t_lj_cola >= LJ_COLA_BATE) {      // assentou: o lado afunda e volta
            float q = (t_lj_cola - LJ_COLA_BATE) / 0.25f;
            if (q < 1) pulo = -sinf(q * 3.1416f) * 4;
        }
        if (cur) ui_selecao(x - 22, y - 22, 44, 44, ADESIVO[a].luz);
        int mostra = lado + (ja == AD_REMENDO ? 2 : 0);
        if (colando && t_lj_cola >= LJ_COLA_BATE) {
            ja = a;
            mostra = lado + (a == AD_REMENDO ? 2 : 0);
            ad_face_aceso = t_lj_cola < LJ_COLA_BATE + 0.5f && (int)(t_lj_cola * 14) % 2 == 0;
        }
        ad_face = ja;
        desenha_dado((float)x, y - pulo, 17, t, 0, 0, mostra, 2, false);
        // O numero do lado embaixo, quando o dado nao o mostra (o Vazio, o Remendo).
        if (mostra != lado || t == D_VAZIO) {
            snprintf(s, sizeof s, "%d", lado);
            gfx_texto(x - gfx_largura(s, GFX_MIUDO) / 2, y + 20, s, C_TEXTO_M, GFX_MIUDO, false);
        }
        zona_clique(x - 21, y - 21, 42, 42, &cursor, c, KEY_ENTER);
    }
    // O adesivo novo: paira sobre o lado da vez; colando, desce ate ele.
    if (cursor >= 0 && cursor < n && (!t_lj_cola || t_lj_cola < LJ_COLA_BATE)) {
        int x, y;
        lj_lado_pos(t_lj_cola > 0 ? lj_cola_lado - ad_lado_min(t) : cursor, n, &x, &y);
        bool livre = !ad_no_lado(pc, ad_lado_min(t) + cursor);
        float desce = t_lj_cola > 0 ? t_lj_cola / LJ_COLA_BATE : 0;
        int hy = y - 40 + (int)(sinf(t_fase * 3) * 2) + (int)(desce * desce * 40);
        if (livre || t_lj_cola > 0) ad_solto_d(x + 2, hy, a, desce > 0.5f ? 1 : 2, 0.4f * (1 - desce));
        else gfx_texto(x - gfx_largura("ocupado", GFX_MIUDO) / 2, y - 36, "ocupado", C_VINHO_CLR, GFX_MIUDO, false);
    }
    lj_lascas();
    int lado = ad_lado_min(t) + (cursor >= 0 && cursor < n ? cursor : 0), ja = ad_no_lado(pc, lado);
    int ty = LJ_LADO_Y + ((n + lj_lados_por(n) - 1) / lj_lados_por(n) - 1) * LJ_LADO_DY + 34;
    if (t_lj_cola > 0) {
        snprintf(s, sizeof s, "%s no lado %d", ADESIVO[a].nome, lj_cola_lado);
        txt_c(GFX_W / 2, ty, s, ADESIVO[a].luz, true);
    } else if (ja) {
        snprintf(s, sizeof s, "O lado %d já tem o %s", lado, ADESIVO[ja].nome);
        txt_c(GFX_W / 2, ty, s, C_VINHO_CLR, true);
        snprintf(s, sizeof s, "%s %s", ADESIVO[ja].l1, ADESIVO[ja].l2);
        txt_c(GFX_W / 2, ty + TXT_H - 1, s, C_MARFIM_S, false);
    } else {
        snprintf(s, sizeof s, "Lado %d  ·  custa %d %s", lado, preco_adesivo(a), preco_adesivo(a) == 1 ? "moeda" : "moedas");
        txt_c(GFX_W / 2, ty, s, C_OURO, true);
    }
    if (t_lj_aviso > 0) txt_c(GFX_W / 2, ty + 2 * TXT_H, lj_aviso, C_VINHO_CLR, true);
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar", KEY_BKSP);
    rodape_run("setas escolhem  ·  ENTER cola  ·  BACKSPACE volta ao dado");
}

// ---- O pacote aberto ----------------------------------------------------------
// Uma carta do pacote: sai do meio girando e pequena, pousa no lugar; a
// escolhida sobe e voa (o voo e o de lj_voos); as outras encolhem e somem.
static void pac_carta(int i)
{
    float q = (t_pac - PAC_RASGA - i * 0.08f) / PAC_VOA;
    if (q <= 0) return;
    float m = sai_mola(q), mq = q > 1 ? 1 : q;
    int x1 = pac_carta_x(i), y1 = LJ_Y;
    float esc = 0.35f + 0.65f * mq;
    if (t_pac_fim > 0) {
        if (i == pac_levado) return;                     // ja esta no ar, a caminho da bolsa
        float f = t_pac_fim / 0.3f;
        if (f >= 1) return;
        esc *= 1 - f * f;
    }
    int cx = (int)(PAC_CX + (x1 - PAC_CX) * m), cy = (int)(PAC_CY + (y1 + LJ_CH / 2 - PAC_CY) * m);
    int w = (int)(PAC_CW * esc), h = (int)(LJ_CH * esc), x = cx - w / 2, y = cy - h / 2;
    if (w < 8 || h < 8) return;
    peca_t pc = run.pac_op[i];
    bool pronto = t_pac >= PAC_PRONTO && t_pac_fim <= 0, cur = pronto && i == cursor;
    ui_cartao(x, y, w, h, cur, cor_rar(pc.tipo));
    float giro = q < 1 ? (1 - q) * 9 : 0;
    float bob = cur ? sinf(t_fase * 4) * 3 - mola_px() : 0;
    if (cur) brilho_passa = mola_brilho();
    marca_nivel = pc.fixo;
    desenha_dado((float)cx, y + 40 * esc + bob, 26 * esc, pc.tipo, 0, giro, pc.fixo, 0, false);
    marca_nivel = -1;
    if (esc > 0.9f) {
        nome_no_cartao(cx, y + 72, TIPO[pc.tipo].nome, cur ? cor_dado(pc.tipo) : C_MARFIM_S, w - 8);
        const char *rar = RARIDADE[TIPO[pc.tipo].raridade];
        gfx_texto(cx - gfx_largura(rar, GFX_MIUDO) / 2, y + 90, rar, cor_rar(pc.tipo), GFX_MIUDO, false);
    }
    if (pronto) zona_clique(x1 - PAC_CW / 2, y1, PAC_CW, LJ_CH, &cursor, i, KEY_ENTER);
}

// O rasgo: raios que saem do meio e um aro que abre, no primeiro instante.
static void pac_clarao(void)
{
    float q = (t_pac - PAC_RASGA) / 0.24f;
    if (q < 0 || q >= 1) return;
    uint16_t c = gfx_mistura(C_BRANCO, cor_grupo(run.pacote), (int)(q * 70));
    for (int k = 0; k < 12; k++) {
        float a = k * 0.5236f + 0.26f, r0 = 18 + q * 30, r1 = r0 + 22 * (1 - q) + 6;
        gfx_linha_grossa(PAC_CX + (int)(cosf(a) * r0), PAC_CY + (int)(sinf(a) * r0),
                         PAC_CX + (int)(cosf(a) * r1), PAC_CY + (int)(sinf(a) * r1), q < 0.5f ? 3 : 2, c);
    }
    int r = (int)(14 + q * 60);
    for (int k = 0; k < 36; k++) {
        float a = k * 0.1745f;
        gfx_rect(PAC_CX + (int)(cosf(a) * r) - 1, PAC_CY + (int)(sinf(a) * r) - 1, 2, 2, c);
    }
}

static void lj_desenha_pacote(void)
{
    int g = run.pacote;
    char s[64];
    snprintf(s, sizeof s, "PACOTE DE %s", g == GR_NUMEROS ? "NÚMEROS" : g == GR_ATAQUE ? "ATAQUE" : g == GR_DEFESA ? "DEFESA" : "COMBO");
    txt_c2(GFX_W / 2, MESA_Y0 + 2, s, cor_grupo(g));
    txt_c(GFX_W / 2, MESA_Y0 + 36, t_pac < PAC_RASGA ? DESC_GRUPO[g] : "Escolha 1 dos 3 dados  ·  ou pule o pacote",
          C_MARFIM_S, false);
    if (t_pac < PAC_RASGA) {
        // Cresce do cartao da loja ate o meio da mesa e sacode cada vez mais.
        float qc = t_pac / PAC_CRESCE;
        if (qc > 1) qc = 1;
        float e = 0.9f + 1.4f * sai_mola(qc);
        int x0 = lj_card_x(lj_item_pacote()), y0 = LJ_Y + 38;
        int cx = (int)(x0 + (PAC_CX - x0) * (qc * qc * (3 - 2 * qc))), cy = (int)(y0 + (PAC_CY - y0) * qc);
        float qs = (t_pac - PAC_CRESCE) / (PAC_RASGA - PAC_CRESCE);
        if (qs > 0) {
            cx += (int)(sinf(t_pac * 55) * (1 + 5 * qs));
            cy += (int)(cosf(t_pac * 41) * 2 * qs);
            e += 0.12f * qs;
        }
        desenha_pacote(cx, cy, e, g, false);
    } else {
        pac_clarao();
        for (int i = 0; i < PAC_OPCOES; i++) pac_carta(i);
    }
    lj_lascas();
    // O que a carta em foco faz; ou o que e pular.
    bool pronto = t_pac >= PAC_PRONTO && t_pac_fim <= 0;
    if (pronto && cursor < PAC_OPCOES) dado_e_poder(LJ_DESC_Y + 6, run.pac_op[cursor].tipo, true);
    else if (pronto) {
        txt_c(GFX_W / 2, LJ_DESC_Y + 6, "Pular o pacote", C_LATAO, true);
        txt_c(GFX_W / 2, LJ_DESC_Y + 5 + TXT_H, "Fica sem nenhum dos três. As moedas não voltam.", C_MARFIM_S, false);
    }
    if (t_lj_aviso > 0) txt_c(GFX_W / 2, LJ_DESC_Y + 6 + 2 * TXT_H, lj_aviso, C_VINHO_CLR, true);
    if (pronto) {
        int w = gfx_largura("pular", 1) + 20, h = TXT_H + 4, x = MESA_X1 - 10 - w, y = MESA_Y0 + 8;
        ui_botao(x, y, w, h, "pular", 0, cursor == PAC_OPCOES ? UI_FOCO : 0);
        zona_clique(x, y, w, h, &cursor, PAC_OPCOES, KEY_ENTER);
    }
    lj_voos();
    lj_flutuantes();
    rodape_run(pronto ? "setas escolhem  ·  ENTER leva  ·  I detalhes  ·  BACKSPACE pular"
                      : "ENTER abre de uma vez");
}

static void desenha_loja_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    faixa_run(0);
    if (lj_tirando) { lj_desenha_tirando(); return; }
    if (lj_pac) { lj_desenha_pacote(); return; }
    if (lj_colando == 1) { lj_cola_dados(); return; }
    if (lj_colando == 2) { lj_cola_lados(); return; }
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

    int nv = n_vitrine(), cw = lj_cw();
    gfx_texto(lj_card_x(0) - cw / 2 + 4, LJ_Y - 11, "VITRINE", C_TEXTO_M, GFX_MIUDO, false);
    gfx_texto(lj_card_x(nv) - cw / 2 + 4, LJ_Y - 11, "PACOTE", C_TEXTO_M, GFX_MIUDO, false);
    gfx_texto(lj_card_x(nv + 1) - cw / 2 + 4, LJ_Y - 11, "ADESIVO", C_TEXTO_M, GFX_MIUDO, false);
    gfx_texto(lj_sx() + 4, LJ_Y - 11, "SERVIÇOS", C_TEXTO_M, GFX_MIUDO, false);
    for (int i = 0; i < nv; i++) lj_cartao_dado(i);
    lj_cartao_pacote();
    lj_cartao_adesivo();
    for (int k = 0; k < 2; k++) lj_servico(k);

    // O que o item em foco faz (ou por que nao deu).
    if (cursor < nv) dado_e_poder(LJ_DESC_Y, run.vitrine[cursor].tipo, true);
    else if (cursor == nv) {
        int g = run.pacote;
        if (g >= GR_N) txt_c(GFX_W / 2, LJ_DESC_Y, "Sem pacote nesta loja", C_LATAO, true);
        else {
            snprintf(s, sizeof s, "Pacote de %s", NOME_GRUPO[g]);
            txt_c(GFX_W / 2, LJ_DESC_Y, s, cor_grupo(g), true);
            txt_c(GFX_W / 2, LJ_DESC_Y + TXT_H - 1, run.pac_estado == PAC_VENDA ? DESC_GRUPO[g]
                  : "Já aberto. A próxima loja traz outro pacote.", C_MARFIM_S, false);
            if (run.pac_estado == PAC_VENDA && t_lj_aviso <= 0)
                txt_c(GFX_W / 2, LJ_DESC_Y + 2 * TXT_H - 3, "Abre na hora: 3 dados do grupo, leva 1.", C_TEXTO_M, false);
        }
    } else if (cursor == lj_item_adesivo()) {
        int a = run.adesivo;
        if (a <= AD_NADA || a >= AD_N) txt_c(GFX_W / 2, LJ_DESC_Y, "Sem adesivo nesta loja", C_LATAO, true);
        else {
            snprintf(s, sizeof s, "Adesivo %s", ADESIVO[a].nome);
            txt_c(GFX_W / 2, LJ_DESC_Y, s, ADESIVO[a].luz, true);
            snprintf(s, sizeof s, "%s %s", ADESIVO[a].l1, ADESIVO[a].l2);
            txt_c(GFX_W / 2, LJ_DESC_Y + TXT_H - 1, run.ad_vendido ? "Já colado. A próxima loja traz outro adesivo." : s,
                  C_MARFIM_S, false);
            if (!run.ad_vendido && t_lj_aviso <= 0)
                txt_c(GFX_W / 2, LJ_DESC_Y + 2 * TXT_H - 3, "Vai num lado de um dado da sua bolsa. Age quando o dado tira esse lado.",
                      C_TEXTO_M, false);
        }
    } else if (cursor == lj_item_troca()) {
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
