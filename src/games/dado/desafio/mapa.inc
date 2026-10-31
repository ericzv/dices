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

static int rola_r(int n) { return 1 + sorte_r(n); }
static peca_t peca_r(int t)
{
    peca_t p = { .tipo = (uint8_t)t, .fixo = 0 };
    if (t == D_QUEBRADO) p.fixo = (uint8_t)(1 + sorte_r(8));
    if (t == D_ADESIVADO || t == D_MAIS_ADESIVADO) adesiva(&p, rola_r, true);   // os adesivos dele, dos abertos
    return p;
}

// A loja nao repete: cada dado e cada adesivo aparece a venda uma vez so na
// run (a vitrine, ao abrir ou trocar, e o adesivo da loja). Os pacotes
// repetem. Se os abertos acabarem, volta a sortear entre todos. O que esta
// a venda vira "visto" quando sai (trocar a vitrine, a loja seguinte): assim
// a mesma loja, sorteada de novo, mostra o mesmo.
static bool dado_visto(int t) { return run.vistos[t >> 3] >> (t & 7) & 1; }
static void marca_visto(int t) { if (t >= 0 && t < D_N) run.vistos[t >> 3] |= (uint8_t)(1u << (t & 7)); }
static bool adesivo_visto(int a) { return run.ad_vistos[a >> 3] >> (a & 7) & 1; }
static void marca_adesivo_visto(int a) { if (a > AD_NADA && a < AD_N) run.ad_vistos[a >> 3] |= (uint8_t)(1u << (a & 7)); }

// Um dado da vitrine: pela raridade, entre os abertos que ainda nao
// estiveram a venda na run.
static int sorteia_vitrine(const float w[4])
{
    for (int todos = 0; todos < 2; todos++) {
        float total = 0;
        for (int t = 0; t < D_N; t++)
            if (!so_inicial(t) && dado_liberado(t) && (todos || !dado_visto(t))) total += w[TIPO[t].raridade];
        if (total <= 0) continue;
        float x = (float)(rnd_r() % 1000000u) / 1000000.0f * total;
        for (int t = 0; t < D_N; t++) {
            if (so_inicial(t) || !dado_liberado(t) || (!todos && dado_visto(t))) continue;
            x -= w[TIPO[t].raridade];
            if (x < 0) return t;
        }
    }
    return D_D8;
}

// Tres (ou n) dados diferentes entre si, que a vitrine ainda nao mostrou.
static void sorteia_distintos(peca_t *out, int n, const float w[4])
{
    for (int i = 0; i < n; i++) {
        bool rep;
        int tent = 0;
        do {
            out[i] = peca_r(sorteia_vitrine(w));
            rep = false;
            for (int k = 0; k < i; k++) if (out[k].tipo == out[i].tipo) rep = true;
        } while (rep && ++tent < 50);
    }
}

// ---------------------------------------------------------------------------
// A loja: depois de cada mesa vencida, com as poucas moedas que ela rendeu.
// A venda: 2 dados e 1 adesivo, ou 1 dado e 2 adesivos (meio a meio), e 1
// pacote. A Cartola poe 1 dado a mais; a Lata de Biscoito, 1 a menos.
// Trocar a loja (tudo: dados, adesivos e o pacote) custa 1, 2, 3... na mesma
// loja; tirar um dado da bolsa custa 3, 5, 7... na run inteira. O Cupom tira
// 1 de cada preco (nunca abaixo de 1).
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
// Os dados a venda: 2 (1 na loja de dois adesivos), +1 com a Cartola, -1 com a Lata.
static int n_vitrine(void)
{
    int n = (run.dois_adesivos ? 1 : 2) + ((run_amuletos() >> A_CARTOLA & 1) ? 1 : 0) - (run.bolsa == B_LATA);
    return n < 0 ? 0 : n > VITRINE_MAX ? VITRINE_MAX : n;
}
// Os adesivos a venda: 1, ou 2 na loja de dois adesivos.
static int n_adesivos_loja(void) { return run.dois_adesivos ? 2 : 1; }
static int ad_loja(int k) { return k == 0 ? run.adesivo : k == 1 && run.dois_adesivos ? run.adesivo2 : AD_NADA; }
static bool ad_loja_vendido(int k) { return k == 0 ? run.ad_vendido : run.ad_vendido2; }

// A vitrine sai da semente da run, da loja e de quantas vezes ja trocou:
// sair e voltar mostra a mesma. Os saloes de cima trazem mais raros.
static const float PESO_VITRINE[DES_ANDARES][4] = {
    { 3, 4, 2.0f, 0.4f }, { 1.5f, 4, 3.5f, 0.8f }, { 0.5f, 3, 4.0f, 1.5f } };
static const float *peso_do_salao(void) { return PESO_VITRINE[run.andar < DES_ANDARES ? run.andar : DES_ANDARES - 1]; }
// A semente de cada parte da loja (a vitrine, os adesivos, o pacote): a da
// run, a da loja e a das trocas.
static void semente_loja(uint32_t parte)
{
    rs = run.semente ^ (parte * (uint32_t)(run.lojas * 16 + run.trocas + 1));
    rnd_r();
}

// A loja que sai (trocada, ou a loja acabou): os dados e os adesivos dela
// nao voltam na run.
static void loja_vista(void)
{
    for (int i = 0; i < n_vitrine(); i++) if (!so_inicial(run.vitrine[i].tipo)) marca_visto(run.vitrine[i].tipo);
    for (int k = 0; k < n_adesivos_loja(); k++) marca_adesivo_visto(ad_loja(k));
}
static void monta_vitrine(void)
{
    semente_loja(0x51ED27u);
    sorteia_distintos(run.vitrine, VITRINE_MAX, peso_do_salao());
    memset(run.levou, 0, sizeof run.levou);
}

// ---------------------------------------------------------------------------
// Os pacotes: um por loja, de dados ou de adesivos (meio a meio). So se ve o
// que tem dentro depois de abrir: 3, e leva 1. O de adesivos sorteia como o
// adesivo da loja. O de dados, 3 dados quaisquer entre os abertos, pela
// raridade do nivel do pacote - Bronze, Prata, Ouro ou Diamante -, e o nivel
// sai da mesa: a cada oponente vencido na run, os pacotes melhores aparecem
// mais.
// ---------------------------------------------------------------------------
enum { PN_BRONZE, PN_PRATA, PN_OURO, PN_DIAMANTE, PN_N };
#define PAC_ADESIVOS PN_N                // o pacote de adesivos (as cartas guardam o adesivo em pac_op[i].ad[0])
#define PAC_NENHUM   0xFF
enum { PAC_VENDA, PAC_ABERTO, PAC_USADO };
#define PRECO_PAC_ADESIVOS 4
static const uint8_t PRECO_NIVEL[PN_N] = { 3, 5, 8, 12 };
static const char *const NOME_PACOTE[PN_N + 1] = { "Bronze", "Prata", "Ouro", "Diamante", "Adesivos" };
// A chance (%) de cada raridade dentro de cada nivel [comum, incomum, raro, lendario].
static const uint8_t RAR_NIVEL[PN_N][4] = { { 75, 20, 5, 0 }, { 20, 65, 15, 0 }, { 0, 25, 65, 10 }, { 0, 0, 65, 35 } };
static const char *const DESC_NIVEL[PN_N] = { "3 dados ao acaso, quase todos comuns.", "3 dados ao acaso, quase todos incomuns.",
                                              "3 dados ao acaso, quase todos raros; às vezes um lendário.",
                                              "3 dados ao acaso, raros e lendários." };
// A chance (%) de cada nivel [Bronze, Prata, Ouro, Diamante] na loja depois do
// 1o, 2o... 8o oponente vencido na run (a da entrada, como a do 1o).
#define PN_MESAS 8
static const uint8_t CHANCE_NIVEL[PN_MESAS][PN_N] = {
    { 75, 20, 4, 1 }, { 62, 26, 9, 3 }, { 48, 30, 15, 7 }, { 34, 30, 22, 14 },
    { 22, 27, 28, 23 }, { 12, 22, 31, 35 }, { 5, 15, 32, 48 }, { 0, 10, 30, 60 } };
static const uint8_t *chance_nivel(void)
{
    int m = run.vitorias < 1 ? 0 : run.vitorias > PN_MESAS ? PN_MESAS - 1 : run.vitorias - 1;
    return CHANCE_NIVEL[m];
}
// A cor de cada pacote (o nome, a moldura do cartao) e o papel do embrulho:
// a luz da tampa, a frente e o lado na sombra, todos da paleta.
static const uint16_t PAPEL_PACOTE[PN_N + 1][3] = {
    { RGB(200, 142, 104), RGB(156, 106, 64), RGB(120, 78, 50) },      // bronze
    { RGB(184, 194, 208), RGB(140, 150, 164), RGB(104, 112, 124) },   // prata
    { RGB(246, 222, 132), RGB(228, 194, 106), RGB(150, 122, 62) },    // ouro
    { RGB(252, 248, 236), RGB(168, 204, 230), RGB(96, 134, 196) },    // diamante
    { RGB(168, 204, 230), RGB(96, 182, 198), RGB(72, 150, 138) } };   // os adesivos: a folha metalizada
static uint16_t cor_pacote(int g)
{
    static const uint16_t C[PN_N + 1] = { RGB(238, 162, 74), C_PRATA, C_OURO, RGB(168, 204, 230), RGB(96, 182, 198) };
    return g >= 0 && g <= PAC_ADESIVOS ? C[g] : C_LATAO;
}
static bool tem_pacote(void) { return run.pacote <= PAC_ADESIVOS; }
static bool pacote_de_adesivos(void) { return run.pacote == PAC_ADESIVOS; }
static int pac_ad(int i) { return i >= 0 && i < PAC_OPCOES && run.pac_op[i].ad[0] < AD_N ? run.pac_op[i].ad[0] : AD_NADA; }

// Os dados que um pacote pode trazer: os que a vitrine sorteia.
static bool no_pacote(int t) { return !so_inicial(t) && dado_liberado(t); }
// A Lata de Biscoito: o pacote de adesivos, como o adesivo, 1 mais barato.
static int preco_pacote(void)
{
    if (!tem_pacote()) return 0;
    return preco_ajusta(pacote_de_adesivos() ? PRECO_PAC_ADESIVOS - (run.bolsa == B_LATA) : PRECO_NIVEL[run.pacote]);
}

// Um dado do pacote de nivel nv que ainda nao esta em out[0..n): primeiro a
// raridade, pela chance do nivel (entre as que tem dado), depois um dado
// dela, todos com a mesma chance.
static peca_t sorteia_do_nivel(int nv, const peca_t *out, int n)
{
    int quantos[4] = { 0, 0, 0, 0 };
    for (int t = 0; t < D_N; t++) {
        bool ja = false;
        for (int k = 0; k < n; k++) ja |= out[k].tipo == t;
        if (no_pacote(t) && !ja) quantos[TIPO[t].raridade]++;
    }
    int total = 0;
    for (int r = 0; r < 4; r++) if (quantos[r]) total += RAR_NIVEL[nv][r];
    int rar = -1;
    if (total > 0) {
        int x = sorte_r(total);
        for (int r = 0; r < 4 && rar < 0; r++)
            if (quantos[r] && (x -= RAR_NIVEL[nv][r]) < 0) rar = r;
    }
    if (rar < 0)                                 // o nivel nao tem nenhuma das raridades dele: a mais perto que tiver
        for (int d = 0; d < 4 && rar < 0; d++)
            for (int r = 0; r < 4 && rar < 0; r++)
                if (quantos[r] && RAR_NIVEL[nv][r] == 0 && abs(r - (nv < 2 ? 0 : 3)) == d) rar = r;
    if (rar < 0) return peca_r(D_D8);
    int q = sorte_r(quantos[rar]);
    for (int t = 0; t < D_N; t++) {
        bool ja = false;
        for (int k = 0; k < n; k++) ja |= out[k].tipo == t;
        if (no_pacote(t) && !ja && TIPO[t].raridade == rar && q-- == 0) return peca_r(t);
    }
    return peca_r(D_D8);
}

// O pacote desta loja: de adesivos (com 3 abertos, pelo menos) ou de dados,
// meio a meio; o de dados, no nivel sorteado pela mesa. O que vem dentro sai
// da semente: sair e voltar mostra o mesmo.
static int sorteia_adesivo_loja(const uint8_t *fora, int n, bool so_novos);
static int adesivos_abertos(void);
static void monta_pacote(void)
{
    semente_loja(0xBA6E1u);
    run.pac_estado = PAC_VENDA;
    memset(run.pac_op, 0, sizeof run.pac_op);
    if (adesivos_abertos() >= PAC_OPCOES && sorte_r(2)) {
        run.pacote = PAC_ADESIVOS;
        uint8_t ja[PAC_OPCOES];
        for (int i = 0; i < PAC_OPCOES; i++) run.pac_op[i].ad[0] = ja[i] = (uint8_t)sorteia_adesivo_loja(ja, i, false);
        return;
    }
    const uint8_t *ch = chance_nivel();
    int x = sorte_r(100), nv = 0;
    while (nv < PN_N - 1 && (x -= ch[nv]) >= 0) nv++;
    run.pacote = (uint8_t)nv;
    for (int i = 0; i < PAC_OPCOES; i++) run.pac_op[i] = sorteia_do_nivel(nv, run.pac_op, i);
}

// Os adesivos da loja (adesivos.inc): entre os que o perfil ja abriu (o
// Crupie abre o de cada nivel), sorteados pela raridade com os pesos da
// vitrine: no primeiro salao, os lendarios quase nao saem.
static bool adesivo_liberado(int a) { return a > AD_NADA && a < AD_N && chefe_abriu(ADESIVO[a].nivel - 1, CH_CRUPIE); }
static int preco_adesivo(int a) { return preco_ajusta(a > AD_NADA && a < AD_N ? ADESIVO[a].preco - (run.bolsa == B_LATA) : 0); }
static int adesivos_abertos(void)
{
    int n = 0;
    for (int a = AD_NADA + 1; a < AD_N; a++) n += adesivo_liberado(a);
    return n;
}
// Um adesivo aberto, pela raridade com os pesos do salao, fora os n de 'fora'
// (so_novos: e os que ja estiveram a venda na run, enquanto houver outros).
static int sorteia_adesivo_loja(const uint8_t *fora, int n, bool so_novos)
{
    const float *w = peso_do_salao();
    for (int todos = !so_novos; todos < 2; todos++) {
        float total = 0;
        for (int a = AD_NADA + 1; a < AD_N; a++) {
            bool ja = !todos && adesivo_visto(a);
            for (int k = 0; k < n; k++) ja |= fora[k] == a;
            if (adesivo_liberado(a) && !ja) total += w[RAR_ADESIVO[a]];
        }
        if (total <= 0) continue;
        float x = (float)(rnd_r() % 1000000u) / 1000000.0f * total;
        int r = AD_NADA;
        for (int a = AD_NADA + 1; a < AD_N; a++) {
            bool ja = !todos && adesivo_visto(a);
            for (int k = 0; k < n; k++) ja |= fora[k] == a;
            if (!adesivo_liberado(a) || ja) continue;
            r = a;
            if ((x -= w[RAR_ADESIVO[a]]) < 0) break;
        }
        return r;
    }
    return AD_NADA;
}
// Os adesivos desta loja: meio a meio, um ou dois (e entao 1 dado a menos).
static void monta_adesivos(void)
{
    semente_loja(0xAD5E1u);
    run.dois_adesivos = (uint8_t)(adesivos_abertos() >= 2 && sorte_r(2));
    uint8_t ja[1];
    run.adesivo = ja[0] = (uint8_t)sorteia_adesivo_loja(NULL, 0, true);
    run.adesivo2 = run.dois_adesivos ? (uint8_t)sorteia_adesivo_loja(ja, 1, true) : AD_NADA;
    run.ad_vendido = run.ad_vendido2 = 0;
}

// A loja inteira: na abertura e a cada troca.
static void sorteia_loja(void)
{
    monta_adesivos();                            // (decide quantos dados a vitrine mostra)
    monta_vitrine();
    monta_pacote();
}
static void monta_loja_run(void)
{
    if (run.lojas) loja_vista();                 // o que a loja de antes mostrou nao volta
    if (run.lojas < 255) run.lojas++;
    run.trocas = 0;
    sorteia_loja();
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
