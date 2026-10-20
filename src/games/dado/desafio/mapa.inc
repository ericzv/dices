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
    peca_t p = { (uint8_t)t, 0 };
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
static void monta_vitrine(void)
{
    rs = run.semente ^ (0x51ED27u * (uint32_t)(run.lojas * 16 + run.trocas + 1));
    rnd_r();
    static const float PESO_VITRINE[DES_ANDARES][4] = {
        { 3, 4, 2.0f, 0.4f }, { 1.5f, 4, 3.5f, 0.8f }, { 0.5f, 3, 4.0f, 1.5f } };
    const float *w = PESO_VITRINE[run.andar < DES_ANDARES ? run.andar : DES_ANDARES - 1];
    sorteia_distintos(run.vitrine, VITRINE_MAX, w);
    memset(run.levou, 0, sizeof run.levou);
}

static void monta_loja_run(void)
{
    if (run.lojas < 255) run.lojas++;
    run.trocas = 0;
    monta_vitrine();
}

// ---------------------------------------------------------------------------
// As mesas de um salao: tres oponentes, do mais facil ao mais dificil, e o
// chefe. O salao inteiro aparece antes de comecar.
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
    chefe->semente = rnd_r() | 1;
}

// O trunfo do oponente p ja vale nesta dificuldade? Os comuns trazem o seu
// a partir de um nivel (nenhum no Normal); os chefes, sempre.
static bool trunfo_ativo(int p) { return p >= 0 && p < N_PERSO && run.dificuldade >= PERSO[p].andar_trunfo; }

// Quantas mesas o salao tem (o do Coringa, uma).
static int mesas_do_salao(void)
{
    int n = 0;
    while (n < DES_PASSOS && run.no[n].tipo != NO_VAZIO) n++;
    return n;
}

static void gera_andar(void)
{
    rs = run.semente ^ (0x9E3779B9u * (uint32_t)(run.andar + 1));
    rnd_r(); rnd_r();
    uint32_t usados = 0;
    for (int p = 0; p < DES_PASSOS - 1; p++) {
        no_t *no = &run.no[p];
        no->tipo = (uint8_t)p;                   // NO_FACIL, NO_MEDIO, NO_DIFICIL
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
    no_t *chefe = &run.no[DES_PASSOS - 1];
    chefe->tipo = NO_CHEFE;
    chefe->perso = (uint8_t)(PERSO_CHEFE0 + run.andar);
    chefe->nivel = NIVEL_NO[run.andar][3];
    chefe->semente = rnd_r() | 1;
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
        col[k] = (peca_t){ (uint8_t)(l == 2 ? D_D2 : l == 4 ? D_D4 : l == 6 ? D_D6 : D_D8), 0 };
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
    int tira = andar + (tier >= 2 ? 1 : 0);
    if (tira > 6) tira = 6;
    if (coringa) tira = 8;
    int n = 0;
    for (int i = 0; i < 8 - tira; i++) col[n++] = peca_r(INICIO[i]);
    // Dados especiais a mais, [salao][1o, 2o, 3o oponente, chefe]: sobem
    // devagar (o jogador cresce aos poucos, comprando na loja).
    static const uint8_t EXTRAS[DES_ANDARES + 1][4] = { { 0, 0, 0, 1 }, { 1, 1, 2, 2 }, { 2, 2, 3, 3 }, { 8, 8, 8, 8 } };
    int extras = EXTRAS[andar][tier];
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
    // So entra o que a sua dificuldade ja abriu (o Coringa, no ultimo nivel,
    // traz o que quiser): o nucleo e o tema fechados ficam de fora.
    for (int i = 0; i < 3 && i <= andar && pe->nucleo[i] && n < MAX_COL; i++) {
        if (!coringa && !dado_liberado(pe->nucleo[i])) continue;
        col[n++] = peca_r(pe->nucleo[i]);
        if (i > 0 && extras > 0) extras--;
    }
    for (int i = 0; i < extras && n < MAX_COL; i++) {
        int t = (n_fav && (coringa || sorte_r(100) < 45)) ? pe->fav[sorte_r(n_fav)] : sorteia_raridade(w);
        if (!coringa && !dado_liberado(t)) t = sorteia_raridade(w);
        col[n++] = peca_r(t);
    }
    if (run.dificuldade >= 6) {                  // dados raros a mais: um do Nivel 6, dois do 8
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
    // No primeiro salao, so dados de numero, e fracos: o jogador ainda e a
    // bolsa do comeco.
    static const uint8_t RECHEIO_SIMPLES[] = { D_D4, D_D4, D_D2, D_D6 };
    // No segundo, numeros e especiais fracos, meio a meio.
    static const uint8_t RECHEIO_MEIO[] = { D_D4, D_D6, D_D4, D_D8, D_D2, D_D6, D_TEIMOSO, D_PAR, D_TREVO,
                                            D_SENTINELA, D_COBRADOR, D_SOLITARIO };
    int alvo = 12 + andar * 2;
    while (n < alvo && n < MAX_COL) {
        int t = coringa ? RECHEIO_CORINGA[sorte_r((int)sizeof RECHEIO_CORINGA)]
              : andar == 0 ? RECHEIO_SIMPLES[sorte_r((int)sizeof RECHEIO_SIMPLES)]
              : andar == 1 ? RECHEIO_MEIO[sorte_r((int)sizeof RECHEIO_MEIO)]
              : RECHEIO[sorte_r((int)sizeof RECHEIO)];
        col[n++] = peca_r(coringa || dado_liberado(t) ? t : D_D6);
    }
    if (!coringa) n = poucos_tipos(col, n, pe, 2 + (tier < 3 ? tier : 3) + andar + run.dificuldade / 4);
    return n;
}
