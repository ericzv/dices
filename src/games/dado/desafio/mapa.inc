// ---------------------------------------------------------------------------
// Dados sorteados por raridade
// ---------------------------------------------------------------------------
// Pesos por dado de cada raridade [comum, incomum, raro, lenda]. Os dados
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

// Premio depois de vencer: mesas faceis dao mais dados comuns; dificeis e
// chefes, mais raros. [tier] = facil, medio, dificil, chefe.
static const float PESO_PREMIO[4][4] = {
    { 6, 2.0f, 0.4f, 0.0f },
    { 3, 3.0f, 1.2f, 0.3f },
    { 1, 2.0f, 2.5f, 1.5f },
    { 0, 1.0f, 3.0f, 4.0f },
};

// Loja: caro de proposito, para a bolsa nao encher. Com as 50 moedas do
// comeco, da para um dado incomum ou um comum.
// A dificuldade Prata encarece a loja; o Cupom barateia.
static int preco_ajusta(int p)
{
    if (run.dificuldade >= 2) p = p * 5 / 4;
    if (run_amuletos() >> A_CUPOM & 1) p = p * 3 / 4;
    return p;
}
static int preco_run(int t)
{
    static const int P[4] = { 25, 40, 60, 90 };
    return preco_ajusta(P[TIPO[t].raridade]);
}
static int preco_am(void) { return preco_ajusta(AM_PRECO); }

static void monta_loja_run(void)
{
    rs = run.semente ^ (0x51ED27u * (uint32_t)(run.andar * 8 + run.passo + 1));
    rnd_r();
    float w[4] = { 3, 4, 2.0f * (1 + 0.3f * run.andar), 0.4f * (1 + 0.5f * run.andar) };
    sorteia_distintos(run.loja, N_LOJA, w);
    memset(run.levou, 0, sizeof run.levou);
    sorteia_amuletos(&run.am_loja, 1);
    run.am_levou = 0;
}

// ---------------------------------------------------------------------------
// O mapa de um salao
// ---------------------------------------------------------------------------
static bool existe(int p, int l) { return p >= 0 && p < DES_PASSOS && l >= 0 && l < 3 && run.no[p][l].tipo != NO_VAZIO; }

static void gera_andar(void)
{
    rs = run.semente ^ (0x9E3779B9u * (uint32_t)(run.andar + 1));
    rnd_r(); rnd_r();
    for (int p = 0; p < DES_PASSOS; p++)
        for (int l = 0; l < 3; l++) run.no[p][l].tipo = NO_VAZIO;
    uint32_t usados = 0;
    int lojas = 0, ult_loja = -9, eventos = 0;
    uint32_t evt_usados = run.lapidou ? 1u << EVT_LAPIDADOR : 0;
    for (int p = 0; p < DES_PASSOS - 1; p++) {
        int ls[3], n;
        if (sorte_r(100) < 45) { n = 3; ls[0] = 0; ls[1] = 1; ls[2] = 2; }
        else {
            static const int PAR[3][2] = { { 0, 1 }, { 1, 2 }, { 0, 2 } };
            int q = sorte_r(3);
            n = 2; ls[0] = PAR[q][0]; ls[1] = PAR[q][1];
        }
        for (int i = 0; i < n; i++) {
            no_t *no = &run.no[p][ls[i]];
            // O salao esquenta: a primeira mesa nunca e dificil; perto do
            // chefe, as dificeis aparecem mais.
            static const uint8_t FACIL[DES_PASSOS - 1] = { 50, 40, 30, 25, 20 };
            static const uint8_t ATE_MEDIO[DES_PASSOS - 1] = { 100, 80, 70, 65, 60 };
            int x = sorte_r(100);
            no->tipo = x < FACIL[p] ? NO_FACIL : (x < ATE_MEDIO[p] ? NO_MEDIO : NO_DIFICIL);
        }
        // Ate duas lojas por salao, nunca na primeira mesa nem em passos
        // seguidos.
        if (lojas < 2 && p >= 1 && p - ult_loja >= 2 && sorte_r(100) < (lojas ? 30 : (p == 1 ? 35 : 50))) {
            run.no[p][ls[sorte_r(n)]].tipo = NO_LOJA;
            lojas++;
            ult_loja = p;
        }
        // Eventos: um ou dois por salao, da segunda mesa ate a antepenultima
        // (nunca logo antes do chefe), cada um uma vez so.
        bool ultima_chance = p == DES_PASSOS - 3 && eventos == 0;
        if (eventos < 2 && p >= 1 && p <= DES_PASSOS - 3 && (ultima_chance || sorte_r(100) < 40)) {
            int livres[3], nl = 0;
            for (int i = 0; i < n; i++) if (run.no[p][ls[i]].tipo != NO_LOJA) livres[nl++] = ls[i];
            int opc[EVT_N], no_ = 0;
            for (int e = 0; e < EVT_N; e++) if (!(evt_usados >> e & 1)) opc[no_++] = e;
            if (nl && no_) {
                no_t *no = &run.no[p][livres[sorte_r(nl)]];
                int e = opc[sorte_r(no_)];
                no->tipo = NO_EVENTO;
                no->perso = (uint8_t)e;
                evt_usados |= 1u << e;
                eventos++;
            }
        }
        for (int i = 0; i < n; i++) {
            no_t *no = &run.no[p][ls[i]];
            no->semente = rnd_r() | 1;
            if (no->tipo == NO_LOJA || no->tipo == NO_EVENTO) continue;
            int pe, tent = 0;
            // O Novato nao senta nas mesas dificeis.
            static const uint8_t COMUNS[] = { 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13, 14, 15 };
            do pe = COMUNS[sorte_r((int)sizeof COMUNS)];
            while (((usados >> pe & 1) || (pe == 0 && no->tipo == NO_DIFICIL)) && ++tent < 20);
            usados |= 1u << pe;
            no->perso = (uint8_t)pe;
            no->nivel = NIVEL_NO[run.andar][no->tipo];
        }
    }
    no_t *chefe = &run.no[DES_PASSOS - 1][1];
    chefe->tipo = NO_CHEFE;
    chefe->perso = (uint8_t)(PERSO_CHEFE0 + run.andar);
    chefe->nivel = NIVEL_NO[run.andar][3];
    chefe->semente = rnd_r() | 1;

    // Ligacoes: cada mesa leva as do passo seguinte na mesma linha ou ao lado.
    // Toda mesa tem saida, e toda mesa do passo seguinte tem entrada.
    for (int p = 0; p < DES_PASSOS - 1; p++) {
        for (int l = 0; l < 3; l++) {
            if (!existe(p, l)) continue;
            uint8_t liga = 0;
            for (int m = 0; m < 3; m++)
                if (existe(p + 1, m) && abs(l - m) <= 1) liga |= (uint8_t)(1 << m);
            if (!liga) {                     // nenhuma ao lado: a mais proxima
                int melhor = -1;
                for (int m = 0; m < 3; m++)
                    if (existe(p + 1, m) && (melhor < 0 || abs(l - m) < abs(l - melhor))) melhor = m;
                if (melhor >= 0) liga = (uint8_t)(1 << melhor);
            }
            run.no[p][l].liga = liga;
        }
        for (int m = 0; m < 3; m++) {
            if (!existe(p + 1, m)) continue;
            bool entra = false;
            for (int l = 0; l < 3; l++) if (existe(p, l) && (run.no[p][l].liga >> m & 1)) entra = true;
            if (entra) continue;
            int melhor = -1;
            for (int l = 0; l < 3; l++)
                if (existe(p, l) && (melhor < 0 || abs(l - m) < abs(melhor - m))) melhor = l;
            if (melhor >= 0) run.no[p][melhor].liga |= (uint8_t)(1 << m);
        }
    }
    for (int p = 0; p < DES_PASSOS; p++) run.caminho[p] = -1;
}

// A mesa l do passo atual pode ser escolhida?
static bool escolhivel(int l)
{
    if (!existe(run.passo, l)) return false;
    if (run.passo == 0) return true;
    int de = run.caminho[run.passo - 1];
    return de >= 0 && (run.no[run.passo - 1][de].liga >> l & 1);
}

// ---------------------------------------------------------------------------
// Os oponentes: dados melhores e menos dados fracos a cada salao e dificuldade
// ---------------------------------------------------------------------------
static int monta_oponente(const no_t *no, int andar, peca_t *col)
{
    rs = no->semente;
    rnd_r();
    int tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
    // Os oito de saida, do mais forte ao mais fraco; os fracos saem primeiro.
    static const uint8_t INICIO[] = { D_D6, D_D6, D_D6, D_D6, D_D4, D_D4, D_D2, D_D2 };
    int tira = andar * 2 + (tier >= 2 ? 1 : 0);
    if (tira > 6) tira = 6;
    int n = 0;
    for (int i = 0; i < 8 - tira; i++) col[n++] = peca_r(INICIO[i]);
    int extras = 1 + andar * 2 + (tier == 3 ? 1 : tier);
    // As duas primeiras mesas do salao aquecem: menos dados especiais.
    if (tier < 3 && run.passo < 2) extras -= 2 - run.passo;
    if (extras < 0) extras = 0;
    static const float BASE[4][4] = {
        { 6, 3, 1.0f, 0.0f }, { 3, 4, 2.0f, 0.2f }, { 1, 3, 4.0f, 0.6f }, { 1, 3, 3.5f, 1.0f } };
    float w[4] = { BASE[tier][0] / (1 + andar), BASE[tier][1],
                   BASE[tier][2] * (1 + 0.4f * andar), BASE[tier][3] * (1 + 0.8f * andar) };
    const perso_t *pe = &PERSO[no->perso];
    int n_fav = 0;
    while (n_fav < 5 && pe->fav[n_fav] != 0xFF) n_fav++;
    for (int i = 0; i < extras && n < MAX_COL; i++) {
        int t = (n_fav && sorte_r(100) < 45) ? pe->fav[sorte_r(n_fav)] : sorteia_raridade(w);
        col[n++] = peca_r(t);
    }
    if (run.dificuldade >= 3) {                  // Ouro: dois dados raros a mais
        static const float RARO[4] = { 0, 0.5f, 3, 1.5f };
        for (int i = 0; i < 2 && n < MAX_COL; i++) col[n++] = peca_r(sorteia_raridade(RARO));
    }
    // Recheio: dados simples (ou de efeito fraco) ate um tamanho minimo de
    // bolsa. Sem isso, um oponente de poucos dados poe os seus dois ou tres
    // dados bons na mao quase toda rodada.
    static const uint8_t RECHEIO[] = { D_D6, D_D8, D_D4, D_D8, D_QUEBRADO, D_TEIMOSO, D_PAR,
                                       D_TREVO, D_SORTE, D_SEMENTE, D_JACKPOT, D_COPIADOR,
                                       D_SENTINELA, D_COBRADOR, D_VIDRO, D_SOLITARIO };
    int alvo = 13 + andar * 2;
    while (n < alvo && n < MAX_COL) col[n++] = peca_r(RECHEIO[sorte_r((int)sizeof RECHEIO)]);
    return n;
}
