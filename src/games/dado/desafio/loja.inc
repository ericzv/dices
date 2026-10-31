// ---------------------------------------------------------------------------
// A loja da run, depois de cada mesa vencida: os dados (2, ou 1 quando sao
// 2 adesivos; +1 com a Cartola), os adesivos (vao num lado de um dado da
// bolsa), o pacote (de dados, no nivel Bronze a Diamante, embrulhado em
// caixa; ou de adesivos, num pacotinho de figurinhas), trocar a loja e tirar
// um dado da bolsa. Cada acao responde na hora, com movimento: os cartoes
// caem do alto quando a loja abre (e de novo quando troca, girando); o dado
// comprado voa ate a bolsa da faixa, e o carimbo de vendido bate no cartao;
// sem moedas, o preco treme; o dado tirado treme, racha e estoura em lascas;
// o pacote cresce no meio da mesa, sacode, rasga e solta tres cartas; o
// adesivo desce sobre o lado escolhido e assenta com um estalo.
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
#define LJ_LADO_Y    158                 // a primeira fileira de lados
#define LJ_LADO_DY   70                  // entre uma fileira e a outra

static bool  lj_tirando;                 // escolhendo o dado que sai
static float t_lj_entra = 9;             // desde que a vitrine (re)apareceu
static float t_lj_abriu = 9;             // desde que a loja abriu (os servicos entram uma vez)
static uint8_t lj_caiu;                  // cartoes que ja bateram na mesa (o som, uma vez)
#define LJ_ITENS_MAX (VITRINE_MAX + AD_LOJA_MAX + 3)   // os dados, os adesivos, o pacote e os dois servicos
static float t_lj_nega[LJ_ITENS_MAX];    // > 0: o item treme (sem moedas, sem lugar)
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
static float t_lj_ad_carimbo[AD_LOJA_MAX];   // o carimbo de colado no cartao de cada adesivo
static int   lj_ad_k;                    // o adesivo da loja que esta sendo colado (0 ou 1)
// O segredo do Adesivado: a colagem que encheu os seis lados. Ele ja virou
// o Mais Adesivado na bolsa (e salvo); a tela mostra o de antes (lj_vira_de)
// recebendo o adesivo e, depois, virando (t_lj_vira).
static bool  lj_vira;
static peca_t lj_vira_de;
static float t_lj_vira;                  // > 0: virando
#define LJ_VIRA_T     3.0f
#define LJ_VIRA_TROCA 1.0f                // o estouro: dali em diante, o Mais Adesivado
// Onde o adesivo vai: na loja, na bolsa da run, comprado; no premio do modo de
// 2 jogadores (cola_premio), na bolsa de quem venceu a rodada, de graca.
// Do pacote de adesivos (cola_pac: a carta), tambem de graca: o pacote ja foi pago.
static bool  cola_premio;
static int   cola_premio_ad;              // o adesivo ganho
static int   cola_premio_slot;            // de qual cartao do premio ele veio
static int   cola_pac = -1;               // colando a carta do pacote de adesivos (-1: nao)
static peca_t *cola_col(void) { return cola_premio ? J[vez].col : run.col; }
static int   cola_n(void) { return cola_premio ? J[vez].n_col : run.n_col; }
static int   cola_ad(void) { return cola_premio ? cola_premio_ad : cola_pac >= 0 ? pac_ad(cola_pac) : ad_loja(lj_ad_k); }
static int   cola_dono(void) { return cola_premio ? vez : 0; }

// Os itens, na ordem das setas: os dados, os adesivos, o pacote e os dois servicos.
static int lj_item_adesivo(int k) { return n_vitrine() + k; }
static int lj_item_pacote(void) { return n_vitrine() + n_adesivos_loja(); }
static int lj_item_troca(void) { return lj_item_pacote() + 1; }
static int lj_item_tira(void) { return lj_item_pacote() + 2; }
static int lj_n_itens(void) { return lj_item_pacote() + 3; }
static int lj_n_cartoes(void) { return lj_item_pacote() + 1; }
// Cinco cartoes (a Cartola): todos um pouco mais estreitos; tres, mais largos.
static int lj_cw(void) { int n = lj_n_cartoes(); return n >= 5 ? 76 : n == 4 ? 86 : 92; }
static int lj_vao(void) { return lj_n_cartoes() >= 5 ? 6 : 8; }
// O grupo do cartao i: 0 os dados, 1 os adesivos, 2 o pacote. Entre cartoes do
// mesmo grupo vai o vao; entre grupos, LJ_SEC.
static int lj_grupo(int i) { return i < n_vitrine() ? 0 : i < lj_item_pacote() ? 1 : 2; }
static int lj_x0(void)
{
    int nc = lj_n_cartoes(), grupos = (n_vitrine() > 0) + 2;
    int total = nc * lj_cw() + (nc - grupos) * lj_vao() + (grupos - 1) * LJ_SEC + LJ_SEC + LJ_SW;
    return (MESA_X0 + MESA_X1) / 2 - total / 2;
}
static int lj_card_x(int i)
{
    int x = lj_x0() + lj_cw() / 2;
    for (int k = 1; k <= i; k++) x += lj_cw() + (lj_grupo(k) != lj_grupo(k - 1) ? LJ_SEC : lj_vao());
    return x;
}
static int lj_sx(void) { return lj_card_x(lj_item_pacote()) + lj_cw() / 2 + LJ_SEC; }
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
    cola_premio = false;
    cola_pac = -1;
    t_lj_cola = 0;
    memset(t_lj_ad_carimbo, 0, sizeof t_lj_ad_carimbo);
    lj_vira = false;
    t_lj_vira = 0;
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
    if (item >= 0 && item < LJ_ITENS_MAX) t_lj_nega[item] = 0.4f;
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

// O papel rasga: pedacos do embrulho (ou da folha metalizada do pacotinho
// de figurinhas) voam do meio da mesa, maiores que as lascas de um dado.
static void pac_rasga(void)
{
    const uint16_t *p = PAPEL_PACOTE[tem_pacote() ? run.pacote : 0];
    uint16_t cores[3] = { p[1], p[0], pacote_de_adesivos() ? C_PRATA : C_MARFIM_S };
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

// O adesivo colado: desce, assenta com um estalo, e a loja volta (o premio:
// segue para a proxima rodada). As lascas voam e caem. Se ele encheu o
// Adesivado, antes de voltar o Adesivado vira o Mais Adesivado.
static void lj_lascas_passo(float dt);
static void lj_vira_estoura(void);
// Devolve true quando a tela da colagem acabou de vez (o premio segue a partida).
static bool cola_termina(void)
{
    lj_colando = 0;
    if (cola_premio) { cola_premio = false; nova_rodada(); return true; }
    if (cola_pac >= 0) {                         // o pacote de adesivos: acabou, volta a loja
        cola_pac = -1;
        lj_pac = false;
        cursor = lj_item_pacote();
        t_pac_carimbo = 0.001f;
        return true;
    }
    cursor = lj_item_adesivo(lj_ad_k);
    t_lj_ad_carimbo[lj_ad_k] = 0.001f;
    return false;
}
static void cola_passo(float dt)
{
    if (t_lj_cola > 0) {
        float antes = t_lj_cola;
        t_lj_cola += dt;
        if (antes < LJ_COLA_BATE && t_lj_cola >= LJ_COLA_BATE) lj_cola_estala();
        if (t_lj_cola >= LJ_COLA_FIM) {
            t_lj_cola = 0;
            if (lj_vira) { t_lj_vira = 0.001f; som_toca(SOM_SUSPENSE); }   // encheu: treme antes de virar
            else if (cola_termina()) return;
        }
    }
    if (t_lj_vira > 0) {
        float antes = t_lj_vira;
        t_lj_vira += dt;
        if (antes < LJ_VIRA_TROCA && t_lj_vira >= LJ_VIRA_TROCA) lj_vira_estoura();
        if (t_lj_vira >= LJ_VIRA_T) {
            t_lj_vira = 0;
            lj_vira = false;
            if (cola_termina()) return;
        }
    }
    if (cola_premio) {
        if (t_lj_aviso > 0) t_lj_aviso -= dt;
        lj_lascas_passo(dt);
    }
}

static void loja_passo(float dt)
{
    if (fase != F_RLOJA) return;
    t_lj_entra += dt;
    t_lj_abriu += dt;
    if (t_lj_aviso > 0) t_lj_aviso -= dt;
    for (int i = 0; i < LJ_ITENS_MAX; i++) if (t_lj_nega[i] > 0) t_lj_nega[i] -= dt;
    for (int i = 0; i < VITRINE_MAX; i++) if (t_lj_vendido[i] > 0) t_lj_vendido[i] += dt;
    if (t_pac_carimbo > 0) t_pac_carimbo += dt;
    for (int k = 0; k < AD_LOJA_MAX; k++) if (t_lj_ad_carimbo[k] > 0) t_lj_ad_carimbo[k] += dt;
    // Os cartoes batendo na mesa, um depois do outro (o pacote por ultimo).
    for (int i = 0; i <= lj_item_pacote(); i++) {
        float q = ui_entra_q(t_lj_entra, i);            // a cascata padrao (interface.inc)
        if (q >= 0.75f && !(lj_caiu >> i & 1)) { lj_caiu |= (uint8_t)(1 << i); som_toca_tom(SOM_BATE_DADO, i * 2); }
    }
    cola_passo(dt);
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
    lj_lascas_passo(dt);
}

static void lj_lascas_passo(float dt)
{
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

// Algum dado da bolsa tem algum lado livre? (o pacote de adesivos)
static bool lj_algum_lado_livre(void)
{
    for (int i = 0; i < run.n_col; i++) if (ad_cabe(&run.col[i])) return true;
    return false;
}

// Compra o pacote: paga e ele abre na hora (fica salvo aberto: sair e voltar
// mostra as mesmas cartas).
static void loja_pacote(void)
{
    int it = lj_item_pacote(), pr = preco_pacote();
    if (!tem_pacote()) { lj_nega(it, "Nenhum pacote nesta loja."); return; }
    if (run.pac_estado != PAC_VENDA) { lj_nega(it, "O pacote desta loja já foi aberto."); return; }
    if (run.moedas < pr) { lj_nega(it, "Moedas insuficientes."); return; }
    if (pacote_de_adesivos() ? !lj_algum_lado_livre() : run.n_col >= MAX_COL) {
        lj_nega(it, pacote_de_adesivos() ? "Nenhum dado da bolsa tem lado livre." : "A bolsa está cheia.");
        return;
    }
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
    if (i >= 0 && i < PAC_OPCOES && pacote_de_adesivos()) {   // o adesivo: escolhe o dado e o lado
        if (!pac_ad(i)) return;
        cola_pac = i;
        lj_colando = 1;
        t_lj_cola = 0;
        cursor = 0;
        while (cursor < run.n_col - 1 && !ad_cabe_o(&run.col[cursor], pac_ad(i))) cursor++;
        som_toca(SOM_TIQUE);
        return;
    }
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

// ---- Os adesivos ---------------------------------------------------------------
// Algum dado da bolsa ainda tem lado livre para o adesivo a?
static bool lj_algum_lado(int a)
{
    for (int i = 0; i < run.n_col; i++) if (ad_cabe_o(&run.col[i], a)) return true;
    return false;
}

// ENTER no adesivo k: escolhe o dado (paga so ao colar; voltar nao custa nada).
static void loja_adesivo(int k)
{
    int it = lj_item_adesivo(k), a = ad_loja(k);
    if (a <= AD_NADA || a >= AD_N) { lj_nega(it, "Nenhum adesivo nesta loja."); return; }
    if (ad_loja_vendido(k)) { lj_nega(it, "Esse adesivo já foi colado."); return; }
    if (run.moedas < preco_adesivo(a)) { lj_nega(it, "Moedas insuficientes."); return; }
    if (!lj_algum_lado(a)) { lj_nega(it, "Nenhum dado da bolsa tem lado livre."); return; }
    lj_ad_k = k;
    lj_colando = 1;
    cursor = 0;
    while (cursor < run.n_col - 1 && !ad_cabe_o(&run.col[cursor], a)) cursor++;
    som_toca(SOM_TIQUE);
}

// O dado i: abre os lados dele (o cursor no primeiro livre).
static void cola_escolhe_dado(int i)
{
    peca_t *col = cola_col();
    if (i < 0 || i >= cola_n()) return;
    if (!ad_cabe(&col[i])) { lj_nega(-1, "Todos os lados desse dado já têm adesivo."); return; }
    if (!ad_cabe_o(&col[i], cola_ad())) { lj_nega(-1, "Esse dado já tem dois Chumbos, o máximo."); return; }
    lj_cola_dado = i;
    lj_colando = 2;
    int t = col[i].tipo;
    cursor = 0;
    while (cursor < ad_n_lados(t) - 1 && ad_no_lado(&col[i], ad_lado_min(t) + cursor)) cursor++;
    som_toca(SOM_TIQUE);
}

// Cola no lado c (contando do primeiro): paga, fica salvo, e ele desce sobre o lado.
static void cola_escolhe_lado(int c)
{
    if (lj_colando != 2 || t_lj_cola > 0 || lj_cola_dado < 0 || lj_cola_dado >= cola_n()) return;
    peca_t *p = &cola_col()[lj_cola_dado];
    int lado = ad_lado_min(p->tipo) + c, a = cola_ad(), pr = preco_adesivo(a);
    if (c < 0 || lado > ad_lado_max(p->tipo)) return;
    if (ad_no_lado(p, lado)) { lj_nega(-1, "Esse lado já tem adesivo."); return; }
    if (cola_premio) ad_cola(p, lado, a);       // o premio: de graca, so ate o fim da partida
    else if (cola_pac >= 0) {                   // o pacote ja pago: cola e o pacote acaba
        ad_cola(p, lado, a);
        pac_levado = cola_pac;
        run.pac_estado = PAC_USADO;
        des_salva();
    } else {
        if (run.moedas < pr || ad_loja_vendido(lj_ad_k)) return;
        run.moedas -= pr;
        ad_cola(p, lado, a);
        if (lj_ad_k) run.ad_vendido2 = 1;
        else run.ad_vendido = 1;
        des_salva();
    }
    lj_cola_lado = lado;
    t_lj_cola = 0.001f;
    som_toca(SOM_FICHA);
    lj_vira_de = *p;                             // o segredo: os seis lados cheios
    lj_vira = vira_mais_adesivado(p);
    if (lj_vira && !cola_premio) des_salva();
}

// Trocar a loja: tudo de novo - os dados, os adesivos (quantos, inclusive) e o pacote.
static void loja_troca(void)
{
    int it = lj_item_troca(), pr = preco_troca();
    if (run.moedas < pr) { lj_nega(it, "Moedas insuficientes."); return; }
    run.moedas -= pr;
    loja_vista();                                // o que sai nao volta na run
    if (run.trocas < 255) run.trocas++;
    sorteia_loja();
    des_salva();
    t_lj_entra = 0;                              // os cartoes novos caem, girando
    lj_caiu = 0;
    memset(t_lj_vendido, 0, sizeof t_lj_vendido);
    memset(t_lj_ad_carimbo, 0, sizeof t_lj_ad_carimbo);
    t_pac_carimbo = 0;
    cursor = lj_item_troca();                    // (a quantidade de cartoes pode mudar)
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
static void lj_pos_grade(int i, int n, int *x, int *y)
{
    grade_t g = grade_lj_tira(n);
    int lin = i / g.por, col = i % g.por;
    int na_linha = (n - lin * g.por) < g.por ? n - lin * g.por : g.por;
    *x = GFX_W / 2 + (int)((col - (na_linha - 1) / 2.0f) * 46 * g.f);
    *y = LJ_TIRA_Y + (int)((lin + 0.5f) * 52 * g.f);
}
static void lj_pos_tira(int i, int *x, int *y) { lj_pos_grade(i, run.n_col, x, y); }

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
    if (run.moedas >= PORCO_ABRE) abre_bolsa(B_PORCO);   // guardou: abre o Porquinho
    bolsa_voando = 0;
    if (run.estado == R_LOJA_INI) { run.estado = R_MAPA; des_salva(); abre_mapa(); t_luz = LUZ_T; }   // o primeiro salao acende
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
    if (t_lj_cola > 0 || t_lj_vira > 0) return true;
    bool volta = k == KEY_BKSP || k == 'x' || k == KEY_TAB;
    if (lj_colando == 1) {
        int nc = cola_n(), por = grade_lj_tira(nc).por;
        if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= por) cursor -= por; return true; }
        if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + por < nc) cursor += por; return true; }
        if (e) { cursor = (cursor + e + nc) % nc; som_toca(SOM_TIQUE); return true; }
        if (volta) {                              // volta aos cartoes (o premio: ao do adesivo; o pacote: as cartas)
            lj_colando = 0;
            cursor = cola_premio ? cola_premio_slot : cola_pac >= 0 ? cola_pac : lj_item_adesivo(lj_ad_k);
            cola_premio = false;
            cola_pac = -1;
            return true;
        }
        if (k == KEY_ENTER) cola_escolhe_dado(cursor);
        return true;
    }
    int n = ad_n_lados(cola_col()[lj_cola_dado].tipo), por = lj_lados_por(n);
    if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= por) cursor -= por; return true; }
    if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + por < n) cursor += por; return true; }
    if (e) { cursor = (cursor + e + n) % n; som_toca(SOM_TIQUE); return true; }
    if (volta) { lj_colando = 1; cursor = lj_cola_dado; return true; }
    if (k == KEY_ENTER) cola_escolhe_lado(cursor);
    return true;
}

static bool loja_tecla(int k, int e)
{
    if (lj_colando) return cola_tecla(k, e);    // (o pacote de adesivos tambem cola)
    if (lj_pac) return pacote_tecla(k, e);
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
        else if (cursor < lj_item_pacote()) loja_adesivo(cursor - nv);
        else if (cursor == lj_item_pacote()) loja_pacote();
        else if (cursor == lj_item_troca()) loja_troca();
        else loja_tira_abre();
        return true;
    }
    if (k == 'q' && porco_inteiro()) porco_quebra();
    if (k == KEY_TAB || k == 'x') loja_sai();
    return true;
}

// ---- O desenho --------------------------------------------------------------
// O Porquinho, ao lado das moedas: o que tem dentro e, inteiro, Q (ou o
// clique) quebra. Quebrando, os cacos voam e as moedas pulam para as suas.
static void lj_porquinho(void)
{
    if (run.bolsa != B_PORCO) return;
    int cx = MESA_X0 + 108, cy = MESA_Y0 + 24;
    char s[24];
    if (!run.porco_quebrou) {
        bool em = mouse_em(cx - 20, cy - 18, 70, 40);
        int pula = em ? (int)(sinf(t_relogio * 9) * 1.5f) : 0;
        icone_bolsa_em(B_PORCO, cx, cy + pula, 2, 1, t_relogio, 1, 0);
        snprintf(s, sizeof s, "%d", run.porco);
        gfx_texto(cx + 19, cy - 16, s, C_ROSA, 1, true);
        gfx_texto(cx + 19, cy - 2, run.porco ? "Q quebra" : "vazio", em ? C_OURO : C_TEXTO_M, GFX_FINO, false);
        if (run.porco) zona_clique(cx - 20, cy - 18, 70, 40, NULL, 0, 'q');
        return;
    }
    if (t_porco <= 0) return;
    float t = t_porco;                           // os cacos e as moedas
    for (int i = 0; i < 10; i++) {
        float a = i * 0.628f + 0.3f, v = 26 + (i % 3) * 9;
        int x = cx + (int)(cosf(a) * v * t), y = cy + (int)(-sinf(a) * v * t + 70 * t * t);
        gfx_rect(x, y, 3, 2, i % 2 ? C_ROSA : C_VINHO_CLR);
    }
    for (int i = 0; i < porco_saiu && i < 12; i++) {   // as moedas voam para o contador
        float q = (t - i * 0.05f) / 0.45f;
        if (q <= 0 || q >= 1) continue;
        int x = cx + (int)((MESA_X0 + 30 - cx) * q), y = cy - (int)(sinf(q * 3.1416f) * 22) + (int)((MESA_Y0 + 24 - cy) * q);
        moeda_r(x, y, ICONE_P);
    }
    if (t < 0.9f) {
        snprintf(s, sizeof s, "+%d", porco_saiu);
        gfx_texto(cx + 6, cy - 22 - (int)(t * 14), s, C_AMARELO, 1, true);
    }
}

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
    ui_escurece(x + 2, y + 2, w - 4, h - 4, 120);
    txt_c(cx + 1, y + 3 + cresce / 2, s, C_VINHO_CLR, true);
}

static void lj_cartao_dado(int i)
{
    int x = lj_card_x(i), cw = lj_cw();
    float q = ui_entra_q(t_lj_entra, i);            // a cascata padrao (interface.inc)
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
    ui_selo_c(x + dx, y + 84, RARIDADE[TIPO[pc.tipo].raridade], cor_rar(pc.tipo), GFX_MIUDO);
    if (ja) {
        ui_escurece(x - cw / 2 + 1, y + 1, cw - 2, LJ_CH - 2, 110);
        lj_carimbo(x, y + LJ_CH - 22, t_lj_vendido[i] > 0 ? t_lj_vendido[i] : 1, "VENDIDO");   // ja vendido: assentado
    } else lj_preco(x, y + LJ_CH - 24, pr, da, nega);
    zona_clique(x - cw / 2, LJ_Y, cw, LJ_CH, &cursor, i, KEY_ENTER);
}

// O nivel, na etiqueta da caixa: 1 a 4 pedrinhas (Bronze a Diamante), cada
// uma um losango com a luz em cima. k: o tamanho de cada pixel.
static void glifo_nivel(int cx, int cy, int nv, uint16_t c, int k)
{
    static const int8_t POS[PN_N][4][2] = {
        { { 0, 0 } }, { { -2, 0 }, { 2, 0 } }, { { -3, 1 }, { 0, -2 }, { 3, 1 } },
        { { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } } };
    uint16_t r[4];
    rampa_de(c, r);
    for (int i = 0; i <= nv && i < 4; i++) {
        int x = cx + POS[nv][i][0] * k, y = cy + POS[nv][i][1] * k;
        gfx_rect(x - k, y, 3 * k, k, r[2]);
        gfx_rect(x, y - k, k, 3 * k, r[2]);
        gfx_rect(x, y - k, k, k, r[3]);                  // a luz em cima
        gfx_rect(x, y + k, k, k, r[1]);                  // e a sombra embaixo
    }
}

// Um pedaco do pacote em rampa, no acabamento dos dados: a luz vem de cima,
// a esquerda, da caixa inteira (bx, by, bw, bh); mais clareia (a tampa) ou
// escurece (o lado na sombra), e cada tom passa ao vizinho pontilhado.
static void pac_rampa(int x, int y, int w, int h, const uint16_t *r, float mais, int bx, int by, int bw, int bh)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            float u = (float)(x + i - bx) / bw, v = (float)(y + j - by) / bh;
            gfx_pixel(x + i, y + j, tom_rampa(r, 0.75f - 0.35f * (u + v) + mais, x + i, y + j));
        }
}

// O pacote de dados: uma caixa embrulhada no papel do nivel (bronze, prata,
// ouro ou diamante), com a fita, o laco em cima e a etiqueta com as
// pedrinhas do nivel - tudo em rampa, com contorno escuro e a sombra
// pontilhada na mesa, como os dados. A de diamante cintila. e: a escala (1
// na loja); rasgado: a tampa arrancada e a caixa vazia.
static const uint16_t FITA_NIVEL[PN_N] = { RGB(238, 230, 212), RGB(96, 134, 196), RGB(190, 56, 60), RGB(150, 112, 160) };
static void desenha_figurinhas(int cx, int cy, float e, bool rasgado);
static void desenha_pacote(int cx, int cy, float e, int g, bool rasgado)
{
    if (g == PAC_ADESIVOS) { desenha_figurinhas(cx, cy, e, rasgado); return; }
    if (g < 0 || g >= PN_N) g = PN_BRONZE;
    const uint16_t *pp = PAPEL_PACOTE[g];
    uint16_t rp[4], rl[4], rm[4], rt[4];
    rampa_de(pp[1], rp);                                                 // o papel
    rampa_de(pp[0], rl);                                                 // e a tampa, no tom claro dele
    rampa_de(FITA_NIVEL[g], rm);                                         // a fita
    rampa_de(C_MARFIM, rt);                                              // a etiqueta
    int w = (int)(44 * e), h = (int)(36 * e), x = cx - w / 2, y = cy - h / 2 + (int)(4 * e);
    int tampa = (int)(9 * e), fw = (int)(5 * e) < 3 ? 3 : (int)(5 * e), k = e >= 1.6f ? 2 : 1;
    int lado = (int)(6 * e);
    ui_sombra(x, y, w, h);                                               // a sombra na mesa
    ui_contorno(x, y, w, h, C_BARRA);
    gfx_rect(x + 1, y, w - 2, 1, C_BARRA); gfx_rect(x, y + 1, 1, h - 2, C_BARRA);
    pac_rampa(x + 1, y + 1, w - 2, h - 2, rp, 0, x, y, w, h);
    pac_rampa(x + w - 1 - lado, y + 1, lado, h - 2, rp, -0.3f, x, y, w, h);   // o lado na sombra
    gfx_rect(x + w - 1 - lado, y + 1, 1, h - 2, rp[0]);                  // a quina
    if (rasgado) {
        // A boca aberta, escura, com a borda do papel em dentes acesos.
        gfx_rect(x + 2, y + 2, w - 4, tampa + 2, C_BARRA);
        for (int i = 0; i < w - 4; i += 3 * k) gfx_rect(x + 2 + i, y + tampa + 2, 2 * k, (i / (3 * k)) % 2 ? 2 * k : k, rp[3]);
        pac_rampa(cx - fw / 2, y + tampa + 4, fw, h - tampa - 6, rm, -0.7f, x, y, w, h);
        return;
    }
    pac_rampa(x + 1, y + 1, w - 2, tampa, rl, 0, x, y, w, h);           // a tampa
    gfx_rect(x + 2, y + 1, w - 4, 1, rl[3]);                             // a luz na beira de cima
    gfx_rect(x + 1, y + 1 + tampa, w - 2, k, rp[0]);                     // e a sombra embaixo dela
    // Dois riscos de dobra no papel.
    gfx_rect(x + (int)(6 * e), y + tampa + (int)(6 * e), (int)(8 * e), k, rp[3]);
    gfx_rect(x + w - (int)(18 * e), y + h - (int)(8 * e), (int)(7 * e), k, rp[2]);
    // A fita: em pe e deitada, em rampa marfim, com um fio de luz.
    int fy = y + (int)(h * 0.58f);
    pac_rampa(cx - fw / 2, y + 1, fw, h - 2, rm, 0, x, y, w, h);
    pac_rampa(x + 1, fy, w - 2, fw, rm, 0, x, y, w, h);
    gfx_rect(cx - fw / 2, y + 1, k, h - 2, rm[3]);
    gfx_rect(x + 1, fy, w - 2, k, rm[3]);
    gfx_rect(cx + fw / 2, y + 1, 1, h - 2, rm[0]);                       // a fita faz sombra no papel
    gfx_rect(x + 1, fy + fw, w - 2, 1, rm[0]);
    // O laco de borboleta: duas asas que abrem do no e duas pontas caidas; a
    // asa da esquerda pega a luz, a da direita fica na sombra.
    int bw = (int)(10 * e), bh = (int)(6 * e), ly = y - (int)(2 * e);
    for (int sd = -1; sd <= 1; sd += 2) {
        gfx_linha_grossa(cx, ly, cx + sd * (bw - 2 * k), ly + bh + (int)(3 * e), fw - 1, C_BARRA);
        gfx_linha_grossa(cx, ly, cx + sd * (bw - 2 * k), ly + bh + (int)(3 * e), fw - 3 > 0 ? fw - 3 : 1, sd < 0 ? rm[2] : rm[1]);
        for (int d = 0; d <= bw; d++) {
            int hh = bh * d / bw;
            gfx_rect(cx + sd * d - (sd < 0), ly - hh - 1, 1, 2 * hh + 3, C_BARRA);
        }
        for (int d = 1; d < bw; d++) {
            int hh = bh * d / bw;
            for (int q = -hh; q <= hh; q++) {
                int px = cx + sd * d - (sd < 0), py = ly + q;
                float l = (sd < 0 ? 0.55f : -0.35f) - 0.5f * q / (bh + 1) + (d > bw - 3 * k ? 0.35f : 0);
                gfx_pixel(px, py, tom_rampa(rm, l, px, py));
            }
        }
    }
    gfx_rect(cx - fw / 2 - 1, ly - fw / 2 - 1, fw + 2, fw + 2, C_BARRA);
    pac_rampa(cx - fw / 2, ly - fw / 2, fw, fw, rm, 0.6f, x, y, w, h);  // o no
    // A etiqueta, presa na fita, com as pedrinhas do nivel.
    int tw = (int)(16 * e), th = (int)(13 * e), tx = x + w - tw + (int)(4 * e), ty = fy + fw - 1;
    ui_sombra(tx, ty, tw, th);
    gfx_rect(tx, ty, tw, th, C_BARRA);
    pac_rampa(tx + 1, ty + 1, tw - 2, th - 2, rt, 0.2f, tx, ty, tw, th);
    glifo_nivel(tx + tw / 2, ty + th / 2, g, pp[2], k);
    // O diamante cintila: tres brilhos que acendem um de cada vez.
    if (g == PN_DIAMANTE)
        for (int b = 0; b < 3; b++) {
            float f = fmodf(t_relogio * 0.9f + b * 0.37f, 1.0f);
            if (f > 0.22f) continue;
            static const float BX[3] = { 0.18f, 0.62f, 0.33f }, BY[3] = { 0.30f, 0.22f, 0.80f };
            int sx = x + (int)(w * BX[b]), sy = y + (int)(h * BY[b]), r = f < 0.11f ? 2 * k : k;
            gfx_rect(sx - r, sy, 2 * r + 1, 1, C_BRANCO);
            gfx_rect(sx, sy - r, 1, 2 * r + 1, C_BRANCO);
        }
}

// O pacote de adesivos: um pacotinho de figurinhas. O envelope de folha
// metalizada, estufado no meio, com as pontas seladas em serrilha prateada,
// o picote do lado, o brilho da folha passando na diagonal e, na frente, a
// estampa: o sol amarelo com um adesivo descolando a ponta (e, grande, a
// faixa ADESIVOS). e: a escala (1 na loja); rasgado: a ponta de cima
// arrancada num rasgo torto, a boca escura e os adesivos aparecendo.
#define FIG_W 38
#define FIG_H 50
static int fig_topo(int i, int k, int sel, bool rasgado)
{
    if (rasgado) return sel + (int)((i * 7919u + (i / k) * 104729u) % (unsigned)(2 * k + 1));   // o rasgo
    return (i / k) % 2 ? k : 0;                                                                 // a serrilha
}
static bool fig_dentro(int i, int j, int w, int h, int k, int sel, int seb, bool rasgado)
{
    if (i < 0 || i >= w || j < fig_topo(i, k, sel, rasgado) || j > h - 1 - ((i / k) % 2 ? k : 0)) return false;
    // O picote: um V na beira da direita, logo abaixo da solda de cima.
    int v = j - sel - k;
    if (!rasgado && i >= w - 2 * k && v >= -k && v <= k && (w - 1 - i) < 2 * k - abs(v)) return false;
    (void)seb;
    return true;
}
static void desenha_figurinhas(int cx, int cy, float e, bool rasgado)
{
    int k = e >= 1.6f ? 2 : 1, w = (int)(FIG_W * e), h = (int)(FIG_H * e);
    int x0 = cx - w / 2, y0 = cy - h / 2, sel = (int)(5 * e), seb = (int)(4 * e);
    const uint16_t *pp = PAPEL_PACOTE[PAC_ADESIVOS];
    uint16_t rf[4], rs_[4], ra[4], rr[4];
    rampa_de(pp[1], rf);                                             // a folha
    rampa_de(C_PRATA, rs_);                                          // a solda prateada
    rampa_de(C_AMARELO, ra);                                         // o sol da estampa
    rampa_de(C_ROSA, rr);                                            // a faixa
    ui_sombra(x0, y0 + (rasgado ? sel : 0), w, h - (rasgado ? sel : 0));
    // O contorno: cada pixel de fora encostado num de dentro.
    for (int j = -1; j <= h; j++)
        for (int i = -1; i <= w; i++)
            if (!fig_dentro(i, j, w, h, k, sel, seb, rasgado)
                && (fig_dentro(i - 1, j, w, h, k, sel, seb, rasgado) || fig_dentro(i + 1, j, w, h, k, sel, seb, rasgado)
                    || fig_dentro(i, j - 1, w, h, k, sel, seb, rasgado) || fig_dentro(i, j + 1, w, h, k, sel, seb, rasgado)))
                gfx_pixel(x0 + i, y0 + j, C_BARRA);
    // A folha: a luz de cima a esquerda, estufada no meio (os lados mais
    // escuros), e o brilho que passa na diagonal; as soldas, frisadas.
    float faixa = 44 * e, passa = fmodf(t_relogio * 16 * e, faixa);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            if (!fig_dentro(i, j, w, h, k, sel, seb, rasgado)) continue;
            int px = x0 + i, py = y0 + j;
            if (j < sel || j > h - 1 - seb) {                          // a solda: frisos em pe
                bool friso = (i / k) % 2;
                gfx_pixel(px, py, j == sel - 1 || j == h - seb ? rs_[1] : friso ? rs_[3] : rs_[2]);
                continue;
            }
            float u = (float)i / w, v = (float)j / h, m = (i - w / 2.0f) / (w / 2.0f);
            float luz = 0.62f - 0.45f * (u + v) - 0.95f * m * m;
            float d = fmodf(i + j * 0.8f - passa + 4 * faixa, faixa);
            if (d < 2 * e) luz += 0.45f;
            else if (d < 5 * e) luz += 0.2f;
            gfx_pixel(px, py, tom_rampa(rf, luz, px, py));
        }
    if (rasgado) {
        // A boca aberta e escura, e as pontas de dois adesivos saindo dela.
        int by = y0 + sel + 2 * k;
        gfx_rect(x0 + 2 * k, by, w - 4 * k, 2 * k + (int)e, C_BARRA);
        static const uint16_t PONTA[2] = { RGB(226, 164, 168), RGB(246, 222, 132) };
        for (int p = 0; p < 2; p++) {
            int px = cx + (p ? 3 : -9) * (int)e, pw = (int)(7 * e);
            gfx_rect(px - 1, by - (int)(5 * e) - 1, pw + 2, (int)(6 * e) + 1, C_BARRA);
            gfx_rect(px, by - (int)(5 * e), pw, (int)(5 * e), PONTA[p]);
            gfx_rect(px, by - (int)(5 * e), pw, k, C_BRANCO);
        }
        return;
    }
    // A faixa do nome, logo abaixo da solda de cima: ADESIVOS quando o pacote
    // esta grande; pequeno, so os pontinhos das letras.
    int fy = y0 + sel + (int)(2 * e), fh = (int)(7 * e) / 2 * 2;
    if (fh < 4) fh = 4;
    gfx_rect(x0 + k, fy - 1, w - 2 * k, fh + 2, C_BARRA);
    for (int j = 0; j < fh; j++)
        gfx_rect(x0 + k, fy + j, w - 2 * k, 1, j < k ? rr[3] : j >= fh - k ? rr[1] : rr[2]);
    if (e >= 1.6f) {
        int tw = gfx_largura("ADESIVOS", GFX_MIUDO);
        gfx_texto(cx - tw / 2, fy + fh / 2 - 4, "ADESIVOS", RGB(118, 30, 38), GFX_MIUDO, false);
    } else
        for (int i = x0 + 3 * k; i < x0 + w - 3 * k; i += 2) if ((i * 5 + 3) % 7 < 5) gfx_pixel(i, fy + fh / 2, C_BRANCO);
    // A estampa: o sol de doze pontas, em rampa, com contorno.
    int topo_sol = fy + fh + (int)(2 * e), R = (int)(11 * e);
    int sx = cx, sy = topo_sol + (y0 + h - seb - topo_sol) / 2;
    for (int j = -R - 1; j <= R + 1; j++)
        for (int i = -R - 1; i <= R + 1; i++) {
            float d = sqrtf((float)(i * i + j * j)), a = atan2f((float)j, (float)i);
            float lim = R * (0.80f + 0.20f * cosf(12 * a));
            int px = sx + i, py = sy + j;
            if (d <= lim) gfx_pixel(px, py, tom_rampa(ra, 0.5f - 0.5f * (i + j) / R, px, py));
            else if (d <= lim + 1.0f) gfx_pixel(px, py, ra[0]);
        }
    // O adesivo no meio do sol, com a ponta de baixo descolando.
    int q = (int)(4.5f * e), qx = sx - q, qy = sy - q;
    gfx_rect(qx - 1, qy - 1, 2 * q + 2, 2 * q + 2, C_BARRA);
    gfx_rect(qx, qy, 2 * q, 2 * q, C_BRANCO);
    gfx_rect(qx, qy, 2 * q, k, C_MARFIM);
    for (int t = 0; t < q; t++)                                      // a ponta dobrada
        gfx_rect(qx + 2 * q - 1 - t, qy + 2 * q - q + t, t + 1, 1, t == q - 1 ? C_BARRA : C_MARFIM_S);
    gfx_rect(sx - k, sy - k, 2 * k, 2 * k, RGB(190, 56, 60));       // o coracaozinho estampado
}

// O cartao do pacote na loja: a caixa do nivel (ou o pacotinho de
// figurinhas), sacudindo de leve de tempos em tempos, como se o que tem
// dentro se mexesse; o nivel e o preco.
static void lj_cartao_pacote(void)
{
    int it = lj_item_pacote(), x = lj_card_x(it), cw = lj_cw();
    float q = ui_entra_q(t_lj_entra, it);
    if (q <= 0) return;
    int y = LJ_Y - (int)((1 - sai_mola(q)) * 80);
    int g = run.pacote;
    bool tem = tem_pacote(), usado = run.pac_estado != PAC_VENDA, cur = it == cursor && !lj_tirando;
    float nega = t_lj_nega[it];
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 2 * (nega / 0.4f)) : 0;
    ui_cartao(x - cw / 2 + dx, y, cw, LJ_CH, cur, tem ? cor_pacote(g) : C_LATAO);
    zona_clique(x - cw / 2, LJ_Y, cw, LJ_CH, &cursor, it, KEY_ENTER);
    if (!tem) {
        txt_c(x, y + 50, "sem pacote", C_TEXTO_M, false);
        return;
    }
    float ciclo = fmodf(t_relogio + 0.7f, 2.6f);                         // a cada 2,6 s, um tremelique
    int mexe = !usado && ciclo < 0.3f ? (int)(sinf(ciclo * 60) * 2 * (1 - ciclo / 0.3f)) : 0;
    float bob = cur && !usado ? sinf(t_fase * 4) * 3 - mola_px() : 0;
    if (pacote_de_adesivos()) desenha_figurinhas(x + dx + mexe, y + 34 + (int)bob, 0.84f, usado);
    else desenha_pacote(x + dx + mexe, y + 38 + (int)bob, 0.9f, g, usado);
    txt_c(x + dx, y + 62, "Pacote", cur ? C_MARFIM : C_MARFIM_S, false);
    txt_c(x + dx, y + 75, NOME_PACOTE[g], cor_pacote(g), true);
    if (usado) {
        ui_escurece(x - cw / 2 + 1, y + 1, cw - 2, LJ_CH - 2, 110);
        lj_carimbo(x, y + LJ_CH - 17, t_pac_carimbo > 0 ? t_pac_carimbo : 1, "ABERTO");
    } else lj_preco(x, y + LJ_CH - 24, preco_pacote(), run.moedas >= preco_pacote(), nega);
}

// O cartao do adesivo k: o adesivo (de tempos em tempos a ponta descola um
// pouco, como quem quer sair), o nome e o preco.
static void lj_cartao_adesivo(int k)
{
    int it = lj_item_adesivo(k), x = lj_card_x(it), cw = lj_cw();
    float q = ui_entra_q(t_lj_entra, it);
    if (q <= 0) return;
    int y = LJ_Y - (int)((1 - sai_mola(q)) * 80);
    int a = ad_loja(k);
    bool tem = a > AD_NADA && a < AD_N, usado = ad_loja_vendido(k), cur = it == cursor && !lj_tirando;
    float nega = t_lj_nega[it];
    int dx = nega > 0 ? (int)(sinf(nega * 70) * 2 * (nega / 0.4f)) : 0;
    ui_cartao(x - cw / 2 + dx, y, cw, LJ_CH, cur, tem ? ADESIVO[a].luz : C_LATAO);
    zona_clique(x - cw / 2, LJ_Y, cw, LJ_CH, &cursor, it, KEY_ENTER);
    if (!tem) {
        txt_c(x, y + 50, "sem adesivo", C_TEXTO_M, false);
        return;
    }
    float ciclo = fmodf(t_relogio + 1.9f + 1.3f * k, 3.1f);
    float descola = !usado && ciclo < 0.5f ? sinf(ciclo / 0.5f * 3.1416f) : 0;
    float bob = cur && !usado ? sinf(t_fase * 4) * 3 - mola_px() : 0;
    ad_solto_d(x + dx, y + 36 + (int)bob, a, 3, descola);
    txt_c(x + dx, y + 62, "Adesivo", cur ? C_MARFIM : C_MARFIM_S, false);
    const char *nome = ADESIVO[a].nome;
    if (gfx_largura(nome, 1) <= cw - 8) txt_c(x + dx, y + 75, nome, ADESIVO[a].luz, true);
    else nome_no_cartao(x + dx, y + 78, nome, ADESIVO[a].luz, cw - 6);
    if (usado) {
        ui_escurece(x - cw / 2 + 1, y + 1, cw - 2, LJ_CH - 2, 110);
        lj_carimbo(x, y + LJ_CH - 17, t_lj_ad_carimbo[k] > 0 ? t_lj_ad_carimbo[k] : 1, "COLADO");
    } else lj_preco(x, y + LJ_CH - 24, preco_adesivo(a), run.moedas >= preco_adesivo(a), nega);
}

// Os servicos: trocar a vitrine (as setas girando) e tirar um dado (o dado rachado).
static void lj_servico(int s)
{
    int item = lj_item_troca() + s, y0 = lj_serv_y(s), sx = lj_sx();
    float q = ui_entra_q(t_lj_abriu - 0.12f, s);
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
        if (!pode) ui_escurece(ix - 16, iy - 16, 32, 32, 120);
    }
    const char *nome = s == 0 ? "Trocar a loja" : "Tirar um dado";
    txt(x0 + dx + 54, y0 + 8, nome, cur ? C_MARFIM : C_MARFIM_S, true);
    char sub[48];
    if (s == 0) snprintf(sub, sizeof sub, "tudo novo");
    else if (pode) snprintf(sub, sizeof sub, "bolsa: %d dados", run.n_col - bolsa_voando);
    else snprintf(sub, sizeof sub, "bolsa no mínimo (%d)", BOLSA_MIN);
    gfx_texto(x0 + dx + 54, y0 + 30, sub, C_TEXTO_M, fonte_que_cabe(sub, GFX_FINO, LJ_SW - 54 - 30), false);
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
        if (a == DES_ANDAR_FINAL && com_coringa()) snprintf(s, (size_t)n, "A seguir: O CORINGA");
        else if (a < DES_ANDARES) snprintf(s, (size_t)n, "A seguir: o %s", SALAO[a]);
        else s[0] = 0;
        return;
    }
    const no_t *no = &run.no[p];
    snprintf(s, (size_t)n, "Próxima mesa: %s  ·  vale %d %s", PERSO[no->perso].nome,
             moedas_mesa(p), moedas_mesa(p) == 1 ? "moeda" : "moedas");
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
    int a = cola_ad(), t = cola_col()[lj_cola_dado].tipo, n = ad_n_lados(t), x, y;
    lj_lado_pos(lj_cola_lado - ad_lado_min(t), n, &x, &y);
    uint16_t cores[3] = { ADESIVO[a].fundo == AD_SEM_FUNDO ? ADESIVO[a].luz : ADESIVO[a].fundo, C_BRANCO, ADESIVO[a].luz };
    lj_lascas_de(x, y, cores, 18, 0.8f);
    som_toca(som_cola());
    mola_chuta();
}

// O segredo: o Adesivado cheio estoura em lascas de todos os adesivos dele e
// volta Mais Adesivado, com confete.
static void lj_vira_estoura(void)
{
    for (int l = 1; l <= 6; l++) {
        int a = ad_no_lado(&lj_vira_de, l);
        uint16_t cores[3] = { ADESIVO[a].fundo == AD_SEM_FUNDO ? ADESIVO[a].luz : ADESIVO[a].fundo, C_BRANCO, ADESIVO[a].luz };
        lj_lascas_de(64, LJ_LADO_Y, cores, 8, 1.4f);
    }
    solta_confete(70);
    som_toca(SOM_NOVIDADE_LENDA);
    mola_chuta();
}

// O titulo das duas telas: o adesivo, o nome e o que ele faz.
static void lj_cola_titulo(const char *pergunta)
{
    int a = cola_ad();
    char s[160];
    txt_c2(GFX_W / 2, MESA_Y0 + 2, cola_premio ? "COLAR O ADESIVO GANHO" : "COLAR O ADESIVO", ADESIVO[a].luz);
    snprintf(s, sizeof s, "%s: %s %s  ·  %s", ADESIVO[a].nome, ADESIVO[a].l1, ADESIVO[a].l2, pergunta);
    txt_c(GFX_W / 2, MESA_Y0 + 40, s, C_MARFIM_S, false);
}

// O rodape: o da run na loja; no premio, dentro da mesa, como o do premio.
static void cola_rodape(const char *s)
{
    if (cola_premio) teclas_c(GFX_W / 2, 288, s, C_TEXTO_M);
    else rodape_run(s);
}

// 1: qual dado. Os sem lado livre ficam apagados.
static void lj_cola_dados(void)
{
    char s[64];
    lj_cola_titulo("em qual dado?");
    const peca_t *col = cola_col();
    int nc = cola_n();
    grade_t g = grade_lj_tira(nc);
    float f = g.f;
    int sel = (int)(22 * f), meio = (int)(16 * f), a = cola_ad();
    for (int i = 0; i < nc; i++) {
        int x, y;
        lj_pos_grade(i, nc, &x, &y);
        bool cur = i == cursor, cabe = ad_cabe_o(&col[i], a);
        if (cur) ui_selecao(x - sel, y - sel, 2 * sel, 2 * sel, ADESIVO[a].luz);
        if (cur && cabe) brilho_passa = mola_brilho();
        peca_parada((float)x, (float)(y - (cur ? mola_px() : 0)), 17 * f, col[i], cola_dono());
        if (!cabe) ui_escurece(x - sel, y - sel, 2 * sel, 2 * sel, 150);
        if (cur) gfx_rect(x - meio, y + (int)(23 * f), 2 * meio, 3, ADESIVO[a].luz);
        else marca_rar(x, y + (int)(23 * f), col[i].tipo);
        zona_clique(x - sel, y - sel, 2 * sel, 2 * sel, &cursor, i, KEY_ENTER);
    }
    int linhas = (nc + g.por - 1) / g.por, ty = LJ_TIRA_Y + (int)(linhas * 52 * f) + 22;
    if (ty < 236) ty = 236;
    const peca_t *pc = &col[cursor < nc ? cursor : 0];
    int livres = ad_n_lados(pc->tipo) - ad_quantos(pc);
    txt_c(GFX_W / 2, ty, TIPO[pc->tipo].nome, cor_dado(pc->tipo), true);
    if (livres) snprintf(s, sizeof s, "%d %s de %d", livres, livres == 1 ? "lado livre" : "lados livres", ad_n_lados(pc->tipo));
    else snprintf(s, sizeof s, "todos os lados já têm adesivo");
    if (livres && !ad_cabe_o(pc, a)) { snprintf(s, sizeof s, "já tem dois Chumbos, o máximo"); livres = 0; }
    txt_c(GFX_W / 2, ty + 22, s, livres ? C_MARFIM_S : C_VINHO_CLR, false);
    if (t_lj_aviso > 0) txt_c(GFX_W / 2, ty + 22 + TXT_H, lj_aviso, C_VINHO_CLR, true);
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar", KEY_BKSP);
    cola_rodape(cola_premio ? "setas escolhem  ·  ENTER escolhe o dado  ·  BACKSPACE volta"
                            : "setas escolhem  ·  ENTER escolhe o dado  ·  I detalhes  ·  BACKSPACE volta");
}

// 2: qual lado. O dado inteiro, um lado ao lado do outro, com os adesivos que
// ja tem; o novo paira sobre o lado da vez e, escolhido, desce e assenta.
static void lj_cola_lados(void)
{
    // Enchendo o Adesivado: ate o estouro, o de antes (com o adesivo novo).
    bool antes = lj_vira && t_lj_vira < LJ_VIRA_TROCA;
    const peca_t *pc = antes ? &lj_vira_de : &cola_col()[lj_cola_dado];
    int t = pc->tipo, n = ad_n_lados(t), a = cola_ad();
    char s[96];
    snprintf(s, sizeof s, "em qual lado do %s?", TIPO[lj_vira ? D_ADESIVADO : t].nome);
    lj_cola_titulo(s);
    float vx = 0, vr = 22;                                  // virando: treme cada vez mais; virou, cresce e assenta
    if (lj_vira && t_lj_vira > 0 && t_lj_vira < LJ_VIRA_TROCA) vx = sinf(t_lj_vira * 70) * 4 * t_lj_vira / LJ_VIRA_TROCA;
    if (lj_vira && t_lj_vira >= LJ_VIRA_TROCA) {
        float q = (t_lj_vira - LJ_VIRA_TROCA) / 0.5f;
        vr = 22 + (q < 1 ? sinf(q * 3.1416f) * 12 : 0);
        if (q < 1) brilho_passa = q;
    }
    if (lj_vira && t_lj_vira >= LJ_VIRA_TROCA) brilho_poder(64 + vx, LJ_LADO_Y, vr, t, 0, true);
    peca_parada(64 + vx, LJ_LADO_Y, vr, *pc, cola_dono());      // o dado, com os selinhos
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
        if (cur && !lj_vira) ui_selecao(x - 22, y - 22, 44, 44, ADESIVO[a].luz);
        int mostra = lado + (ja == AD_REMENDO ? 2 : 0);
        if (colando && t_lj_cola >= LJ_COLA_BATE) {
            ja = a;
            mostra = lado + (a == AD_REMENDO ? 2 : 0);
            ad_face_aceso = t_lj_cola < LJ_COLA_BATE + 0.5f && (int)(t_lj_cola * 14) % 2 == 0;
        }
        if (lj_vira && t_lj_vira > 0)                // virando: os adesivos acendem um depois do outro
            ad_face_aceso = (int)(t_lj_vira * 10 + c * 3) % 4 == 0 || (t_lj_vira >= LJ_VIRA_TROCA && t_lj_vira < LJ_VIRA_TROCA + 0.4f);
        if (lj_vira && t_lj_vira > 0 && t_lj_vira < LJ_VIRA_TROCA) pulo += sinf(t_lj_vira * 50 + c) * 2 * t_lj_vira / LJ_VIRA_TROCA;
        if (lj_vira && t_lj_vira >= LJ_VIRA_TROCA) {    // os lados novos caem um depois do outro
            float q = (t_lj_vira - LJ_VIRA_TROCA - c * 0.07f) / 0.3f;
            if (q < 0) { ad_face_aceso = false; continue; }
            if (q < 1) pulo += (1 - q) * (1 - q) * 30;
        }
        ad_face = ja;
        desenha_dado((float)x, y - pulo, 17, t, cola_dono(), 0, mostra, 2, false);
        // O numero do lado embaixo, quando o dado nao o mostra (o Vazio, o Remendo).
        if (mostra != lado || t == D_VAZIO) {
            snprintf(s, sizeof s, "%d", lado);
            gfx_texto(x - gfx_largura(s, GFX_FINO) / 2, y + 15, s, C_TEXTO_M, GFX_FINO, false);
        }
        zona_clique(x - 21, y - 21, 42, 42, &cursor, c, KEY_ENTER);
    }
    // O adesivo novo: paira sobre o lado da vez; colando, desce ate ele.
    if (cursor >= 0 && cursor < n && !lj_vira && (!t_lj_cola || t_lj_cola < LJ_COLA_BATE)) {
        int x, y;
        lj_lado_pos(t_lj_cola > 0 ? lj_cola_lado - ad_lado_min(t) : cursor, n, &x, &y);
        bool livre = !ad_no_lado(pc, ad_lado_min(t) + cursor);
        float desce = t_lj_cola > 0 ? t_lj_cola / LJ_COLA_BATE : 0;
        int hy = y - 36 + (int)(sinf(t_fase * 3) * 2) + (int)(desce * desce * 36);
        if (livre || t_lj_cola > 0) ad_solto_d(x + 2, hy, a, desce > 0.5f ? 1 : 2, 0.4f * (1 - desce));
        else gfx_texto(x - gfx_largura("ocupado", GFX_FINO) / 2, y - 42, "ocupado", C_VINHO_CLR, GFX_FINO, false);
    }
    lj_lascas();
    int lado = ad_lado_min(t) + (cursor >= 0 && cursor < n ? cursor : 0), ja = ad_no_lado(pc, lado);
    int ty = LJ_LADO_Y + ((n + lj_lados_por(n) - 1) / lj_lados_por(n) - 1) * LJ_LADO_DY + 34;
    if (lj_vira && t_lj_vira > 0) {                       // o segredo: o letreiro
        if (t_lj_vira < LJ_VIRA_TROCA) {
            txt_c(GFX_W / 2, ty, "Os seis lados cheios... não cabe mais nada!", (int)(t_lj_vira * 8) % 2 ? C_OURO : C_AMARELO, true);
        } else {
            float q = (t_lj_vira - LJ_VIRA_TROCA) / 0.35f;
            int sobe = q < 1 ? (int)((1 - q) * (1 - q) * 14) : 0;
            txt_c2(GFX_W / 2, ty - 8 + sobe, "MAIS ADESIVADO!", (int)(t_lj_vira * 6) % 2 ? C_OURO : C_AMARELO);
            if (q > 0.6f) txt_c(GFX_W / 2, ty + 34, "o Adesivado virou um d8, com um adesivo em cada lado", C_MARFIM_S, false);
        }
    } else if (t_lj_cola > 0) {
        snprintf(s, sizeof s, "%s no lado %d", ADESIVO[a].nome, lj_cola_lado);
        txt_c(GFX_W / 2, ty, s, ADESIVO[a].luz, true);
    } else if (ja) {
        snprintf(s, sizeof s, "O lado %d já tem o %s", lado, ADESIVO[ja].nome);
        txt_c(GFX_W / 2, ty, s, C_VINHO_CLR, true);
        snprintf(s, sizeof s, "%s %s", ADESIVO[ja].l1, ADESIVO[ja].l2);
        txt_c(GFX_W / 2, ty + TXT_H - 1, s, C_MARFIM_S, false);
    } else if (cola_pac >= 0) {
        snprintf(s, sizeof s, "Lado %d  ·  do pacote", lado);
        txt_c(GFX_W / 2, ty, s, C_OURO, true);
    } else if (cola_premio) {
        snprintf(s, sizeof s, "Lado %d  ·  prêmio: fica até o fim da partida", lado);
        txt_c(GFX_W / 2, ty, s, C_OURO, true);
    } else {
        snprintf(s, sizeof s, "Lado %d  ·  custa %d %s", lado, preco_adesivo(a), preco_adesivo(a) == 1 ? "moeda" : "moedas");
        txt_c(GFX_W / 2, ty, s, C_OURO, true);
    }
    if (t_lj_aviso > 0) txt_c(GFX_W / 2, ty + 2 * TXT_H, lj_aviso, C_VINHO_CLR, true);
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar", KEY_BKSP);
    cola_rodape("setas escolhem  ·  ENTER cola  ·  BACKSPACE volta ao dado");
}

// ---- O adesivo do premio (modo de 2 jogadores) -------------------------------
// As mesmas duas telas, na bolsa de quem venceu a rodada, sem custo; colado,
// a partida segue para a proxima rodada.
static bool premio_colando(void) { return cola_premio && lj_colando; }
static bool premio_cabe(int a)
{
    for (int c = 0; c < J[vez].n_col; c++) if (ad_cabe_o(&J[vez].col[c], a)) return true;
    return false;
}
static bool premio_cola_comeca(int i)
{
    if (i < 0 || i >= 3 || !oferta_ad[i] || !premio_cabe(oferta_ad[i])) return false;
    cola_premio = true;
    cola_premio_ad = oferta_ad[i];
    cola_premio_slot = i;
    lj_colando = 1;
    t_lj_cola = t_lj_aviso = t_lj_vira = 0;
    lj_vira = false;
    n_lasca = 0;
    cursor = 0;
    while (cursor < J[vez].n_col - 1 && !ad_cabe_o(&J[vez].col[cursor], cola_premio_ad)) cursor++;
    som_toca(SOM_TIQUE);
    return true;
}
static void premio_cola_desenha(void)
{
    if (lj_colando == 1) lj_cola_dados();
    else lj_cola_lados();
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
    int a = pacote_de_adesivos() ? pac_ad(i) : AD_NADA;
    if (a) {                                         // a carta do adesivo: ele solto, o nome e a raridade
        uint16_t cr = COR_RAR[RAR_ADESIVO[a]];
        ui_cartao(x, y, w, h, cur, cr);
        float bob = cur ? sinf(t_fase * 4) * 3 - mola_px() : 0;
        float descola = cur ? 0.25f + 0.15f * sinf(t_fase * 4) : 0;
        int s = esc > 0.9f ? 4 : esc > 0.6f ? 3 : 2;
        ad_solto_d(cx, (int)(y + 40 * esc + bob), a, s, descola);
        if (esc > 0.9f) {
            nome_no_cartao(cx, y + 72, ADESIVO[a].nome, cur ? ADESIVO[a].luz : C_MARFIM_S, w - 8);
            ui_selo_c(cx, y + 89, RARIDADE[RAR_ADESIVO[a]], cr, GFX_MIUDO);
        }
        if (pronto) zona_clique(x1 - PAC_CW / 2, y1, PAC_CW, LJ_CH, &cursor, i, KEY_ENTER);
        return;
    }
    ui_cartao(x, y, w, h, cur, cor_rar(pc.tipo));
    float giro = q < 1 ? (1 - q) * 9 : 0;
    float bob = cur ? sinf(t_fase * 4) * 3 - mola_px() : 0;
    if (cur) brilho_passa = mola_brilho();
    marca_nivel = pc.fixo;
    desenha_dado((float)cx, y + 40 * esc + bob, 26 * esc, pc.tipo, 0, giro, pc.fixo, 0, false);
    marca_nivel = -1;
    if (esc > 0.9f) {
        nome_no_cartao(cx, y + 72, TIPO[pc.tipo].nome, cur ? cor_dado(pc.tipo) : C_MARFIM_S, w - 8);
        ui_selo_c(cx, y + 89, RARIDADE[TIPO[pc.tipo].raridade], cor_rar(pc.tipo), GFX_MIUDO);
    }
    if (pronto) zona_clique(x1 - PAC_CW / 2, y1, PAC_CW, LJ_CH, &cursor, i, KEY_ENTER);
}

// O rasgo: raios que saem do meio e um aro que abre, no primeiro instante.
static void pac_clarao(void)
{
    float q = (t_pac - PAC_RASGA) / 0.24f;
    if (q < 0 || q >= 1) return;
    uint16_t c = gfx_mistura(C_BRANCO, cor_pacote(run.pacote), (int)(q * 70));
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
    static const char *const MAI[PN_N + 1] = { "BRONZE", "PRATA", "OURO", "DIAMANTE", "ADESIVOS" };
    int g = tem_pacote() ? run.pacote : PN_BRONZE;
    char s[64];
    snprintf(s, sizeof s, "PACOTE DE %s", MAI[g]);
    txt_c2(GFX_W / 2, MESA_Y0 + 2, s, cor_pacote(g));
    txt_c(GFX_W / 2, MESA_Y0 + 36, t_pac < PAC_RASGA ? (g == PAC_ADESIVOS ? "3 adesivos ao acaso, entre os abertos." : DESC_NIVEL[g])
          : g == PAC_ADESIVOS ? "Escolha 1 dos 3 adesivos e cole num dado  ·  ou pule o pacote"
          : "Escolha 1 dos 3 dados  ·  ou pule o pacote", C_MARFIM_S, false);
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
        desenha_pacote(cx, cy, g == PAC_ADESIVOS ? e * 0.82f : e, g, false);
    } else {
        pac_clarao();
        for (int i = 0; i < PAC_OPCOES; i++) pac_carta(i);
    }
    lj_lascas();
    // O que a carta em foco faz; ou o que e pular.
    bool pronto = t_pac >= PAC_PRONTO && t_pac_fim <= 0;
    if (pronto && cursor < PAC_OPCOES && pacote_de_adesivos()) {   // o adesivo em foco: o que ele faz
        int a = pac_ad(cursor);
        char l[112];
        snprintf(l, sizeof l, "Adesivo %s", ADESIVO[a].nome);
        txt_c(GFX_W / 2, LJ_DESC_Y + 6, l, ADESIVO[a].luz, true);
        snprintf(l, sizeof l, "%s %s", ADESIVO[a].l1, ADESIVO[a].l2);
        txt_c(GFX_W / 2, LJ_DESC_Y + 5 + TXT_H, l, C_MARFIM_S, false);
    } else if (pronto && cursor < PAC_OPCOES) dado_e_poder(LJ_DESC_Y + 6, run.pac_op[cursor].tipo, true);
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
    ui_escurece(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, 90);
    faixa_run(0);
    if (lj_tirando) { lj_desenha_tirando(); return; }
    if (lj_colando == 1) { lj_cola_dados(); return; }
    if (lj_colando == 2) { lj_cola_lados(); return; }
    if (lj_pac) { lj_desenha_pacote(); return; }
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
    gfx_texto(MESA_X0 + 18, MESA_Y0 + 30, "moedas", C_TEXTO_M, GFX_FINO, false);
    lj_porquinho();

    int nv = n_vitrine(), na = n_adesivos_loja(), cw = lj_cw();
    // Os rotulos das secoes ficam na fina mesmo com a letra maior: a linha
    // da proxima mesa, logo acima, ja cresceu.
    if (nv) gfx_texto(lj_card_x(0) - cw / 2 + 4, LJ_Y - 20, "DADOS", C_TEXTO_M, GFX_FINO_FIXO, false);
    gfx_texto(lj_card_x(nv) - cw / 2 + 4, LJ_Y - 20, na > 1 ? "ADESIVOS" : "ADESIVO", C_TEXTO_M, GFX_FINO_FIXO, false);
    gfx_texto(lj_card_x(lj_item_pacote()) - cw / 2 + 4, LJ_Y - 20, "PACOTE", C_TEXTO_M, GFX_FINO_FIXO, false);
    gfx_texto(lj_sx() + 4, LJ_Y - 20, "SERVIÇOS", C_TEXTO_M, GFX_FINO_FIXO, false);
    for (int i = 0; i < nv; i++) lj_cartao_dado(i);
    for (int k = 0; k < na; k++) lj_cartao_adesivo(k);
    lj_cartao_pacote();
    for (int k = 0; k < 2; k++) lj_servico(k);

    // O que o item em foco faz (ou por que nao deu).
    if (cursor < nv) dado_e_poder(LJ_DESC_Y, run.vitrine[cursor].tipo, true);
    else if (cursor == lj_item_pacote()) {
        int g = run.pacote;
        if (!tem_pacote()) txt_c(GFX_W / 2, LJ_DESC_Y, "Sem pacote nesta loja", C_LATAO, true);
        else {
            snprintf(s, sizeof s, "Pacote de %s", NOME_PACOTE[g]);
            txt_c(GFX_W / 2, LJ_DESC_Y, s, cor_pacote(g), true);
            txt_c(GFX_W / 2, LJ_DESC_Y + TXT_H - 1, run.pac_estado != PAC_VENDA ? "Já aberto. Troque a loja ou espere a próxima para outro pacote."
                  : g == PAC_ADESIVOS ? "3 adesivos ao acaso: abre na hora, você leva 1 e já cola num dado." : DESC_NIVEL[g],
                  C_MARFIM_S, false);
            if (run.pac_estado == PAC_VENDA && t_lj_aviso <= 0 && g < PN_N) {
                // As chances do nivel, so as raridades que ele traz.
                static const char *const RAR_PAC[4] = { "comum", "incomum", "raro", "lendário" };
                char l[96] = "Abre na hora e você leva 1:  ";
                for (int r = 0, prim = 1; r < 4; r++) {
                    if (!RAR_NIVEL[g][r]) continue;
                    size_t n = strlen(l);
                    snprintf(l + n, sizeof l - n, "%s%s %d%%", prim ? "" : "  ·  ", RAR_PAC[r], RAR_NIVEL[g][r]);
                    prim = 0;
                }
                txt_c(GFX_W / 2, LJ_DESC_Y + 2 * TXT_H - 3, l, C_TEXTO_M, false);
            }
        }
    } else if (cursor < lj_item_pacote()) {
        int k = cursor - nv, a = ad_loja(k);
        if (a <= AD_NADA || a >= AD_N) txt_c(GFX_W / 2, LJ_DESC_Y, "Sem adesivo nesta loja", C_LATAO, true);
        else {
            snprintf(s, sizeof s, "Adesivo %s", ADESIVO[a].nome);
            txt_c(GFX_W / 2, LJ_DESC_Y, s, ADESIVO[a].luz, true);
            snprintf(s, sizeof s, "%s %s", ADESIVO[a].l1, ADESIVO[a].l2);
            txt_c(GFX_W / 2, LJ_DESC_Y + TXT_H - 1, ad_loja_vendido(k) ? "Já colado. Troque a loja ou espere a próxima para outros." : s,
                  C_MARFIM_S, false);
            if (!ad_loja_vendido(k) && t_lj_aviso <= 0)
                txt_c(GFX_W / 2, LJ_DESC_Y + 2 * TXT_H - 3, "Vai num lado de um dado da sua bolsa. Age quando o dado tira esse lado.",
                      C_TEXTO_M, false);
        }
    } else if (cursor == lj_item_troca()) {
        txt_c(GFX_W / 2, LJ_DESC_Y, "Trocar a loja", C_LATAO, true);
        txt_c(GFX_W / 2, LJ_DESC_Y + TXT_H - 1, "Sorteia tudo de novo: os dados, os adesivos e o pacote. Cada troca nesta loja custa 1 a mais.",
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
