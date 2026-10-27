// ---------------------------------------------------------------------------
// Dados sorteados por raridade
// ---------------------------------------------------------------------------
// Pesos por dado de cada raridade [comum, incomum, raro, lendario]. Os dados
// iniciais (moeda, D4, D6) nunca saem, como no premio da partida normal.
static int sorteia_raridade(const float w[4])
{
    float total = 0;
    for (int t = 0; t < D_N; t++) if (!so_inicial(t) && dado_liberado(t)) total += w[TIPO[t].raridade];
    float x = (float)(rnd_r() % 1000000u) / 1000000.0f * total;
    for (int t = 0; t < D_N; t++) {
        if (so_inicial(t) || !dado_liberado(t)) continue;
        x -= w[TIPO[t].raridade];
        if (x < 0) return t;
    }
    return D_D8;
}

static peca_t peca_r(int t)
{
    peca_t p = { .tipo = (uint8_t)t, .fixo = 0 };
    if (t == D_QUEBRADO) p.fixo = (uint8_t)(1 + sorte_r(8));
    return p;
}

// Tres (ou n) dados diferentes entre si.
static void sorteia_distintos(peca_t *out, int n, const float w[4])
{
    for (int i = 0; i < n; i++) {
        bool rep;
        int tent = 0;
        do {
            out[i] = peca_r(sorteia_raridade(w));
            rep = false;
            for (int k = 0; k < i; k++) if (out[k].tipo == out[i].tipo) rep = true;
        } while (rep && ++tent < 50);
    }
}

// ---------------------------------------------------------------------------
// A loja: depois de cada mesa vencida, com as poucas moedas que ela rendeu.
// A vitrine mostra 2 dados (3 com a Cartola); trocar a vitrine custa 1, 2, 3...
// na mesma loja; tirar um dado da bolsa custa 3, 5, 7... na run inteira. O
// Cupom tira 1 de cada preco (nunca abaixo de 1).
// ---------------------------------------------------------------------------
#define BOLSA_MIN 5                      // a bolsa nao fica com menos que isto
static int preco_ajusta(int p)
{
    if (run_amuletos() >> A_CUPOM & 1) p--;
    return p < 1 ? 1 : p;
}
static int preco_run(int t)
{
    static const int P[4] = { 3, 4, 6, 9 };
    return preco_ajusta(P[TIPO[t].raridade]);
}
static int preco_troca(void)  { return preco_ajusta(1 + run.trocas); }
static int preco_remove(void) { return preco_ajusta(3 + 2 * run.remocoes); }
static int n_vitrine(void)    { return (run_amuletos() >> A_CARTOLA & 1) ? 3 : 2; }

// A vitrine sai da semente da run, da loja e de quantas vezes ja trocou:
// sair e voltar mostra a mesma. Os saloes de cima trazem mais raros.
static const float PESO_VITRINE[DES_ANDARES][4] = {
    { 3, 4, 2.0f, 0.4f }, { 1.5f, 4, 3.5f, 0.8f }, { 0.5f, 3, 4.0f, 1.5f } };
static const float *peso_do_salao(void) { return PESO_VITRINE[run.andar < DES_ANDARES ? run.andar : DES_ANDARES - 1]; }

static void monta_vitrine(void)
{
    rs = run.semente ^ (0x51ED27u * (uint32_t)(run.lojas * 16 + run.trocas + 1));
    rnd_r();
    sorteia_distintos(run.vitrine, VITRINE_MAX, peso_do_salao());
    memset(run.levou, 0, sizeof run.levou);
}

// ---------------------------------------------------------------------------
// Os pacotes: dados embrulhados, um por loja, de um grupo. So se ve o que tem
// dentro depois de abrir (3 dados do grupo, sorteados pela raridade do
// salao, e leva 1), por isso custa menos que a vitrine. Os grupos sao largos
// de proposito: um grupo pequeno deixaria escolher o dado sem depender da
// sorte. So entra na loja o grupo que ja tem pelo menos 3 dados abertos.
// ---------------------------------------------------------------------------
enum { GR_NUMEROS, GR_ATAQUE, GR_DEFESA, GR_COMBO, GR_N, GR_NENHUM = 0xFF };
// O pacote de adesivos: metade das lojas, no lugar do de dados. As cartas
// guardam o adesivo em pac_op[i].ad[0] (a run salva fica como era).
#define GR_ADESIVOS GR_N
enum { PAC_VENDA, PAC_ABERTO, PAC_USADO };
#define PRECO_PACOTE 4
static const char *const NOME_GRUPO[GR_N + 1] = { "Números", "Ataque", "Defesa", "Combo", "Adesivos" };
static const char *const DESC_GRUPO[GR_N + 1] = {
    "Números grandes e dados que pontuam muito (ou nada) sozinhos.",
    "Dados que tiram pontos, anulam ou roubam do oponente.",
    "Dados que não são anulados, protegem a sequência ou ganham quando são anulados.",
    "Dados que ganham com os outros dados da sua sequência.",
    "Adesivos para colar num lado de um dado seu." };
// A cor de cada grupo (texto e moldura) e o papel do embrulho: a luz da
// tampa, a frente e o lado na sombra, todos da paleta.
static const uint16_t PAPEL_GRUPO[GR_N + 1][3] = {
    { RGB(228, 194, 106), RGB(190, 156, 86), RGB(150, 122, 62) },     // ouro
    { RGB(208, 98, 108), RGB(190, 56, 60), RGB(118, 30, 38) },        // vermelho
    { RGB(96, 134, 196), RGB(56, 86, 150), RGB(26, 34, 62) },         // azul
    { RGB(150, 112, 160), RGB(102, 74, 138), RGB(54, 36, 74) },       // roxo
    { RGB(130, 204, 112), RGB(72, 150, 138), RGB(26, 34, 62) } };     // os adesivos: verde-agua
static uint16_t cor_grupo(int g)
{
    static const uint16_t C[GR_N + 1] = { C_OURO, C_VINHO_CLR, RGB(96, 134, 196), RGB(150, 112, 160), RGB(130, 204, 112) };
    return g >= 0 && g <= GR_ADESIVOS ? C[g] : C_LATAO;
}
static bool tem_pacote(void) { return run.pacote <= GR_ADESIVOS; }
static bool pacote_de_adesivos(void) { return run.pacote == GR_ADESIVOS; }
static int pac_ad(int i) { return i >= 0 && i < PAC_OPCOES && run.pac_op[i].ad[0] < AD_N ? run.pac_op[i].ad[0] : AD_NADA; }

static int grupo_do_dado(int t)
{
    switch (t) {
    case D_D8: case D_D12: case D_D20: case D_LASTRO: case D_VICIADO: case D_EXPLOSIVO: case D_DESAFIANTE:
    case D_ACUMULADOR: case D_PREVISIVEL: case D_PACTO: case D_EGOISTA: case D_BERSERKER: case D_AUTOFAGIA:
    case D_BUMERANGUE: case D_VIDRO: case D_QUEBRADO: case D_RANCOR: case D_AMPULHETA: case D_POTE_QUENTE:
    case D_ESPERANCA: case D_TREVO: case D_TUDO_NADA: case D_SORTE_REVES: case D_MALDITO: case D_FOGO:
    case D_FURIA: case D_ONDA: case D_PATINHOS: case D_PRESENTE: case D_PANETONE: case D_PISCA: case D_MEIA:
    case D_REVOLVER: case D_ASTRO:
        return GR_NUMEROS;
    case D_CARRASCO: case D_MARTELO: case D_PIRATA: case D_GATUNO: case D_INVEJOSO: case D_CARIDOSO:
    case D_MALDICAO: case D_LASER: case D_SABOTADOR: case D_VAMPIRO: case D_ASSOMBRACAO: case D_CAVALO:
    case D_OURICO: case D_AGIOTA: case D_ESPELHO: case D_COVEIRO: case D_BONECO:
        return GR_ATAQUE;
    case D_ESCUDO: case D_TEIMOSO: case D_SENTINELA: case D_SEGURANCA: case D_ANCORA: case D_PONTE:
    case D_FENIX: case D_MARTIR: case D_MISERICORDIOSO: case D_FICHA_VELHA: case D_SANGUESSUGA: case D_VAZIO:
    case D_OCULTO: case D_VIUVA: case D_AZARAO: case D_FICHAS: case D_COBRADOR:
        return GR_DEFESA;
    case D_DOBRO: case D_PAR: case D_FARTURA: case D_SORTE: case D_SEMENTE: case D_JACKPOT: case D_COPIADOR:
    case D_VENTANIA: case D_SOLITARIO: case D_FERREIRO: case D_GEMEO: case D_PRISMA: case D_CAPITAO:
    case D_CHECKPOINT: case D_ECO: case D_DOMINO: case D_PUXA_FILA: case D_HEXA: case D_CALIBRE:
    case D_SACRIFICIO: case D_MEDIA: case D_COLECIONADOR: case D_GARIMPEIRO: case D_TRONO: case D_MEDUSA:
    case D_TRENO: case D_JINGLE: case D_ESTRELA: case D_RENA:
        return GR_COMBO;
    default:
        return GR_NENHUM;                        // os de saida e os que nao saem em sorteio
    }
}

static bool no_grupo(int t, int g) { return !so_inicial(t) && dado_liberado(t) && grupo_do_dado(t) == g; }
static int dados_no_grupo(int g)
{
    int n = 0;
    for (int t = 0; t < D_N; t++) n += no_grupo(t, g);
    return n;
}
static int preco_pacote(void) { return preco_ajusta(PRECO_PACOTE); }

// Um dado do grupo g, pela raridade (w), que ainda nao esta em out[0..n).
static peca_t sorteia_do_grupo(int g, const float w[4], const peca_t *out, int n)
{
    float total = 0;
    for (int t = 0; t < D_N; t++) {
        bool ja = false;
        for (int k = 0; k < n; k++) ja |= out[k].tipo == t;
        if (no_grupo(t, g) && !ja) total += w[TIPO[t].raridade];
    }
    float x = (float)(rnd_r() % 1000000u) / 1000000.0f * total;
    for (int t = 0; t < D_N; t++) {
        bool ja = false;
        for (int k = 0; k < n; k++) ja |= out[k].tipo == t;
        if (!no_grupo(t, g) || ja) continue;
        x -= w[TIPO[t].raridade];
        if (x < 0) return peca_r(t);
    }
    for (int t = D_N - 1; t >= 0; t--) if (no_grupo(t, g)) return peca_r(t);
    return peca_r(D_D8);
}

// O pacote desta loja: o grupo, ao acaso entre os que tem dados bastantes,
// e o que vem dentro (da semente: sair e voltar mostra o mesmo).
static int sorteia_adesivo_loja(const uint8_t *fora, int n);
static int adesivos_abertos(void);
static void monta_pacote(void)
{
    rs = run.semente ^ (0xBA6E1u * (uint32_t)(run.lojas + 1));
    rnd_r();
    int pode[GR_N], n = 0;
    for (int g = 0; g < GR_N; g++) if (dados_no_grupo(g) >= PAC_OPCOES) pode[n++] = g;
    run.pac_estado = PAC_VENDA;
    memset(run.pac_op, 0, sizeof run.pac_op);
    // Metade das lojas traz o pacote de adesivos (com 3 abertos, pelo menos).
    if (adesivos_abertos() >= PAC_OPCOES && (!n || sorte_r(2))) {
        run.pacote = GR_ADESIVOS;
        uint8_t ja[PAC_OPCOES];
        for (int i = 0; i < PAC_OPCOES; i++) run.pac_op[i].ad[0] = ja[i] = (uint8_t)sorteia_adesivo_loja(ja, i);
        return;
    }
    if (!n) { run.pacote = GR_NENHUM; run.pac_estado = PAC_USADO; return; }
    run.pacote = (uint8_t)pode[sorte_r(n)];
    for (int i = 0; i < PAC_OPCOES; i++) run.pac_op[i] = sorteia_do_grupo(run.pacote, peso_do_salao(), run.pac_op, i);
}

// O adesivo da loja (adesivos.inc): um por loja, entre os que o perfil ja
// abriu (o Crupie abre o de cada nivel), sorteado pela raridade com os pesos
// da vitrine: no primeiro salao, os lendarios quase nao saem. Trocar a
// vitrine nao muda o adesivo.
static bool adesivo_liberado(int a) { return a > AD_NADA && a < AD_N && chefe_abriu(ADESIVO[a].nivel - 1, CH_CRUPIE); }
static int preco_adesivo(int a) { return preco_ajusta(a > AD_NADA && a < AD_N ? ADESIVO[a].preco : 0); }
static int adesivos_abertos(void)
{
    int n = 0;
    for (int a = AD_NADA + 1; a < AD_N; a++) n += adesivo_liberado(a);
    return n;
}
// Um adesivo aberto, pela raridade com os pesos do salao, fora os n de 'fora'.
static int sorteia_adesivo_loja(const uint8_t *fora, int n)
{
    const float *w = peso_do_salao();
    float total = 0;
    for (int a = AD_NADA + 1; a < AD_N; a++) {
        bool ja = false;
        for (int k = 0; k < n; k++) ja |= fora[k] == a;
        if (adesivo_liberado(a) && !ja) total += w[RAR_ADESIVO[a]];
    }
    float x = (float)(rnd_r() % 1000000u) / 1000000.0f * total;
    int r = AD_NADA;
    for (int a = AD_NADA + 1; a < AD_N && total > 0; a++) {
        bool ja = false;
        for (int k = 0; k < n; k++) ja |= fora[k] == a;
        if (!adesivo_liberado(a) || ja) continue;
        r = a;
        if ((x -= w[RAR_ADESIVO[a]]) < 0) break;
    }
    return r;
}
static void monta_adesivo(void)
{
    rs = run.semente ^ (0xAD5E1u * (uint32_t)(run.lojas + 1));
    rnd_r();
    run.adesivo = (uint8_t)sorteia_adesivo_loja(NULL, 0);
    run.ad_vendido = 0;
}

static void monta_loja_run(void)
{
    if (run.lojas < 255) run.lojas++;
    run.trocas = 0;
    monta_vitrine();
    monta_pacote();
    monta_adesivo();
}

// ---------------------------------------------------------------------------
// As mesas de um salao: dois oponentes, um facil e um dificil, e o chefe.
// O salao inteiro aparece antes de comecar.
// ---------------------------------------------------------------------------
// O salao do Coringa: so ele.
static void gera_salao_final(void)
{
    rs = run.semente ^ 0xC0121A5Au;
    rnd_r();
    for (int p = 0; p < DES_PASSOS; p++) run.no[p].tipo = NO_VAZIO;
    no_t *chefe = &run.no[0];
    chefe->tipo = NO_CHEFE;
    chefe->perso = PERSO_CORINGA;
    chefe->nivel = NIVEL_NO[DES_ANDAR_FINAL][3];
    if (run.dificuldade < CORINGA_SOME) chefe->nivel = 1;    // no 5 e no 6 ele ainda pensa menos
    chefe->semente = rnd_r() | 1;
}

// O trunfo do oponente p ja vale nesta dificuldade? Os comuns trazem o seu
// a partir de um nivel (nenhum no Normal); os chefes, sempre.
static bool trunfo_ativo(int p) { return p >= 0 && p < N_PERSO && run.dificuldade >= PERSO[p].andar_trunfo; }

// A regra do trunfo, como vale na run: a do Coringa cresce no Nivel 7.
static const char *regra_perso(int p)
{
    if (p == PERSO_CORINGA && run.dificuldade >= CORINGA_SOME)
        return "Seu dado que tirou mais vira o 4º dele (\"?\" se ele abre). Da 5ª rodada, seu dado anulado some.";
    return PERSO[p].regra;
}

// O Coringa vem depois do Barao nesta run?
static bool com_coringa(void) { return run.dificuldade >= CORINGA_DESDE; }
// Quantos dados o Coringa traz na dificuldade d: 13 no Nivel 5, um a mais por nivel.
static int coringa_bolsa(int d) { return 13 + (d > N_NIVEIS ? N_NIVEIS : d) - CORINGA_DESDE; }

// Quantas mesas o salao tem (o do Coringa, uma).
static int mesas_do_salao(void)
{
    int n = 0;
    while (n < DES_PASSOS && run.no[n].tipo != NO_VAZIO) n++;
    return n;
}
// Quantas moedas a mesa p do salao vale (o salao do Coringa, nenhuma).
static int moedas_mesa(int p)
{
    if (run.andar >= DES_ANDARES || p < 0 || p >= DES_PASSOS || run.no[p].tipo > NO_CHEFE) return 0;
    return MOEDAS_MESA[run.andar][run.no[p].tipo];
}

static void gera_andar(void)
{
    le_epoca();                                  // dezembro: o Papai Noel e os de Natal
    rs = run.semente ^ (0x9E3779B9u * (uint32_t)(run.andar + 1));
    rnd_r(); rnd_r();
    uint32_t usados = 0;
    for (int p = 0; p < DES_PASSOS; p++) run.no[p].tipo = NO_VAZIO;
    for (int p = 0; p < DES_MESAS - 1; p++) {
        no_t *no = &run.no[p];
        no->tipo = (uint8_t)(p == 0 ? NO_FACIL : NO_DIFICIL);
        no->semente = rnd_r() | 1;
        int pe, tent = 0;
        // O Novato nao senta na mesa dificil.
        static const uint8_t COMUNS[] = { 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
        do pe = COMUNS[sorte_r((int)sizeof COMUNS)];
        while (((usados >> pe & 1) || (pe == 0 && no->tipo == NO_DIFICIL)) && ++tent < 20);
        usados |= 1u << pe;
        no->perso = (uint8_t)pe;
        no->nivel = NIVEL_NO[run.andar][no->tipo];
    }
    no_t *chefe = &run.no[DES_MESAS - 1];
    chefe->tipo = NO_CHEFE;
    chefe->perso = (uint8_t)(PERSO_CHEFE0 + run.andar);
    chefe->nivel = NIVEL_NO[run.andar][3];
    chefe->semente = rnd_r() | 1;
    // Em dezembro: um oponente de Natal em cada salao, numa das tres mesas, e
    // o Papai Noel no lugar do Barao.
    if (dezembro) {
        static const uint8_t DE_NATAL[DES_ANDARES] = { PERSO_RENA, PERSO_DUENDE, PERSO_RANZINZA };
        run.no[sorte_r(DES_MESAS - 1)].perso = DE_NATAL[run.andar < DES_ANDARES ? run.andar : 0];
        if (run.andar == 2) chefe->perso = PERSO_NOEL;
    }
}

// ---------------------------------------------------------------------------
// Os oponentes: dados melhores e menos dados fracos a cada salao e dificuldade
// ---------------------------------------------------------------------------
// Dado de numero, sem poder: o que um especial vira quando sobra.
static bool so_numero(int t) { return t == D_D2 || t == D_D4 || t == D_D6 || t == D_D8 || t == D_D12 || t == D_D20; }

// Poucos tipos diferentes: um oponente simples leva so uns poucos dados
// especiais diferentes - o nucleo primeiro, depois os que mais se repetem;
// os outros viram dados de numero do mesmo tamanho (os grandes, d8). Assim
// da para entender a mesa dele de uma olhada.
static int poucos_tipos(peca_t *col, int n, const perso_t *pe, int lim)
{
    bool fica[D_N] = { false };
    int nf = 0;
    for (int i = 0; i < 3 && pe->nucleo[i]; i++)
        for (int k = 0; k < n; k++)
            if (col[k].tipo == pe->nucleo[i] && !fica[col[k].tipo]) { fica[col[k].tipo] = true; nf++; }
    while (nf < lim) {
        int b = -1, nb = 0;
        for (int k = 0; k < n; k++) {
            int t = col[k].tipo, c = 0;
            if (so_numero(t) || fica[t]) continue;
            for (int i = 0; i < n; i++) c += col[i].tipo == t;
            if (c > nb) { nb = c; b = t; }
        }
        if (b < 0) break;
        fica[b] = true;
        nf++;
    }
    for (int k = 0; k < n; k++) {
        int t = col[k].tipo, l = TIPO[t].lados;
        if (so_numero(t) || fica[t]) continue;
        col[k] = (peca_t){ .tipo = (uint8_t)(l == 2 ? D_D2 : l == 4 ? D_D4 : l == 6 ? D_D6 : D_D8), .fixo = 0 };
    }
    return n;
}

static int monta_oponente(const no_t *no, int andar, peca_t *col)
{
    rs = no->semente;
    rnd_r();
    int tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
    // Os oito de saida, do mais forte ao mais fraco; os fracos saem primeiro.
    static const uint8_t INICIO[] = { D_D6, D_D6, D_D6, D_D6, D_D4, D_D4, D_D2, D_D2 };
    // O Coringa nao tem poder nenhum alem da copia: a forca dele e a bolsa.
    // Sem os dados de saida, toda do tema, recheada de dados bons.
    bool coringa = no->perso == PERSO_CORINGA;
    bool noel = no->perso == PERSO_NOEL;         // a bolsa do Papai Noel: so do tema (Trenos e sinos)
    // Ele cresce do Nivel 5 a Cobertura (g de 0 a 5): mais dados do tema e
    // uma bolsa maior. So na Cobertura ele traz o que quiser; antes, o que
    // voce ainda nao abriu vira D12.
    int g = coringa ? run.dificuldade - CORINGA_DESDE : 0;
    g = g < 0 ? 0 : g > 5 ? 5 : g;
    bool tudo = coringa && run.dificuldade >= N_NIVEIS;
    int tira = andar + (tier >= 2 ? 1 : 0);
    if (tira > 6) tira = 6;
    if (coringa) tira = 8;
    int n = 0;
    // No primeiro salao a bolsa e de dados de numero, mais fraca que a do
    // jogador e um pouco mais forte a cada mesa: [mesa] = { D2, D4, D6 }.
    static const uint8_t SAIDA_S1[4][3] = { { 1, 10, 0 }, { 0, 10, 1 }, { 0, 8, 3 }, { 0, 10, 1 } };
    if (andar == 0 && !coringa) {
        static const uint8_t LADOS[3] = { D_D2, D_D4, D_D6 };
        for (int k = 2; k >= 0; k--)
            for (int i = 0; i < SAIDA_S1[tier][k]; i++) col[n++] = peca_r(LADOS[k]);
    } else for (int i = 0; i < 8 - tira; i++) col[n++] = peca_r(INICIO[i]);
    // Dados especiais a mais, [salao][1o, 2o, 3o oponente, chefe]: sobem
    // devagar (o jogador cresce aos poucos, comprando na loja).
    static const uint8_t EXTRAS[DES_ANDARES + 1][4] = { { 0, 0, 0, 1 }, { 1, 1, 2, 0 }, { 3, 3, 4, 4 }, { 8, 8, 8, 8 } };
    int extras = coringa ? 3 + g : EXTRAS[andar][tier];
    static const float BASE[4][4] = {
        { 6, 3, 1.0f, 0.0f }, { 3, 4, 2.0f, 0.2f }, { 1, 3, 4.0f, 0.6f }, { 1, 3, 3.5f, 1.0f } };
    float w[4] = { BASE[tier][0] / (1 + andar), BASE[tier][1],
                   BASE[tier][2] * (1 + 0.4f * andar), BASE[tier][3] * (1 + 0.8f * andar) };
    const perso_t *pe = &PERSO[no->perso];
    int n_fav = 0;
    while (n_fav < 8 && pe->fav[n_fav] != 0xFF) n_fav++;
    // O nucleo: os dados que sao a cara dele, sempre na bolsa - um no primeiro
    // salao, dois no segundo, tres dali em diante. Cada um toma o lugar de um
    // dos sorteados (o primeiro vem de graca: ninguem fica sem o seu).
    // So entra o que a sua dificuldade ja abriu (o Coringa, na Cobertura,
    // traz o que quiser): o nucleo e o tema fechados ficam de fora.
    for (int i = 0; i < 3 && i <= andar && pe->nucleo[i] && n < MAX_COL; i++) {
        if (!tudo && !dado_liberado(pe->nucleo[i])) continue;
        col[n++] = peca_r(pe->nucleo[i]);
        if (i > 0 && extras > 0) extras--;
    }
    for (int i = 0; i < extras && n < MAX_COL; i++) {
        int t = (n_fav && (coringa || sorte_r(100) < (noel ? 70 : 45))) ? pe->fav[sorte_r(n_fav)] : sorteia_raridade(w);
        if (!tudo && !dado_liberado(t)) t = coringa ? D_D12 : sorteia_raridade(w);
        col[n++] = peca_r(t);
    }
    if (run.dificuldade >= 6 && andar >= 1) {   // do 2o salao: dados raros a mais, um do Nivel 6, dois do 8
        static const float RARO[4] = { 0, 0.5f, 3, 1.5f };
        int raros = run.dificuldade >= 8 ? 2 : 1;
        for (int i = 0; i < raros && n < MAX_COL; i++) col[n++] = peca_r(sorteia_raridade(RARO));
    }
    // Recheio: dados simples (ou de efeito fraco) ate um tamanho minimo de
    // bolsa. Sem isso, um oponente de poucos dados poe os seus dois ou tres
    // dados bons na mao quase toda rodada.
    static const uint8_t RECHEIO[] = { D_D6, D_D8, D_D4, D_D8, D_QUEBRADO, D_TEIMOSO, D_PAR,
                                       D_TREVO, D_SORTE, D_SEMENTE, D_JACKPOT, D_COPIADOR,
                                       D_SENTINELA, D_COBRADOR, D_VIDRO, D_SOLITARIO };
    static const uint8_t RECHEIO_CORINGA[] = { D_D20, D_ESCUDO, D_LASER, D_CAPITAO, D_TEIMOSO, D_FOGO };
    static const uint8_t RECHEIO_CORINGA_LEVE[] = { D_D12, D_D20, D_D8, D_ESCUDO, D_TEIMOSO, D_D12 };   // no 5 e no 6
    // No primeiro salao, so dados de numero, e fracos: o jogador ainda e a
    // bolsa do comeco.
    static const uint8_t RECHEIO_SIMPLES[] = { D_D4, D_D4, D_D2, D_D6 };
    // No segundo, numeros e especiais fracos, meio a meio.
    static const uint8_t RECHEIO_MEIO[] = { D_D4, D_D6, D_D4, D_D8, D_D2, D_D6, D_TEIMOSO, D_PAR, D_TREVO,
                                            D_SENTINELA, D_COBRADOR, D_SOLITARIO };
    int alvo = coringa ? coringa_bolsa(CORINGA_DESDE + g) : 12 + andar * 2;
    while (n < alvo && n < MAX_COL) {
        int t = coringa && g < CORINGA_SOME - CORINGA_DESDE ? RECHEIO_CORINGA_LEVE[sorte_r((int)sizeof RECHEIO_CORINGA_LEVE)]
              : coringa ? RECHEIO_CORINGA[sorte_r((int)sizeof RECHEIO_CORINGA)]
              : andar == 0 ? RECHEIO_SIMPLES[sorte_r((int)sizeof RECHEIO_SIMPLES)]
              : andar == 1 ? RECHEIO_MEIO[sorte_r((int)sizeof RECHEIO_MEIO)]
              : RECHEIO[sorte_r((int)sizeof RECHEIO)];
        col[n++] = peca_r(tudo || dado_liberado(t) ? t : coringa ? D_D12 : D_D6);
    }
    if (!coringa) n = poucos_tipos(col, n, pe, 2 + (tier < 3 ? tier : 3) + andar + run.dificuldade / 4);
    // O Revolver dele ja chega com balas no tambor (o seu vem vazio da loja).
    for (int i = 0; i < n; i++) if (col[i].tipo == D_REVOLVER) col[i].fixo = (uint8_t)sorte_r(REVOLVER_BALAS);
    return n;
}
