// Teste da IA mestre (o nivel 3 de ia.inc): joga partidas inteiras, mestre
// contra mestre e mestre contra a afiada, com bolsas do Desafiante e bolsas
// de dados de todos os tipos, e confere a cada decisao dela que:
//   - a jogada e legal (a fila so com dados da mao, sem repetir e sem passar
//     do tamanho; a aposta, uma das opcoes da vez; lancar so com dado na fila);
//   - pensar nao mexe em nada do jogo: a mesa, as bolsas, as fichas e os
//     sorteios (o da partida e o da run) ficam como estavam;
//   - a fila pensada em pedacos (um por quadro) e a mesma pensada de uma vez;
//   - a mesma partida da o mesmo resultado com ela pensando a mais ou nao;
//   - cada quadro em que ela pensa cabe no tempo.
// E que os niveis de baixo nao passam pela mestre.
//
//   ia_mestre [partidas] [esforco] [limite em ms de um quadro]
//
// esforco: 0 pensa pouco (rapido, o padrao); 1 como no jogo (e o que mede o
// tempo de verdade).
#include "arena_comum.h"

static int falhas;
#define CONFERE(c, ...) do { if (!(c)) { printf("FALHOU: "); printf(__VA_ARGS__); putchar('\n'); falhas++; } } while (0)

// O que pensar nao pode mudar.
typedef struct {
    jogador_t j[2];
    fila_t f[2];
    int pote, a_pagar, passes, fase, vez, primeiro, rodada, cursor;
    bool aumentou;
    uint32_t semente, rs;
    char veredito[sizeof veredito];
} retrato_t;
static void retrata(retrato_t *r)
{
    memset(r, 0, sizeof *r);
    memcpy(r->j, J, sizeof J);
    memcpy(r->f, F, sizeof F);
    r->pote = pote; r->a_pagar = a_pagar; r->passes = passes; r->fase = fase; r->vez = vez;
    r->primeiro = primeiro; r->rodada = rodada; r->cursor = cursor; r->aumentou = aumentou;
    r->semente = semente; r->rs = rs;
    memcpy(r->veredito, veredito, sizeof veredito);
}

static long n_fila, n_lanca, n_aposta, n_fatias;

// A decisao que a mestre tomaria agora, pedida direto (como o ia_passo pede):
// confere que e legal e que o jogo ficou como estava.
static void confere_decisao(int partida)
{
    retrato_t antes, depois;
    int j = vez;
    retrata(&antes);
    ia_nivel = 3;
    ia_ousadia = 0;
    // (O sorteio da IA e a aposta que ela lembra voltam depois: o blefe da
    // afiada sai do sorteio da IA, e a partida conferida tem de ser a mesma.)
    uint32_t sem_ia = semente_ia;
    int apostou[2] = { ms_apostou[0], ms_apostou[1] };
    if (fase == F_ORDEM && F[j].n > (F[j].grudado ? 1 : 0)) {
        // A fila ja comecada: o proximo dado e o do plano.
        int i = ia_proximo_da_fila();
        CONFERE(i == (F[j].n < plano_n ? plano[F[j].n] : -1), "partida %d: o proximo da fila (%d) nao e o do plano", partida, i);
    } else if (fase == F_ORDEM) {
        // Em pedacos pequenos, e depois de uma vez: a mesma fila.
        int8_t em_pedacos[N_FILA];
        plano_rodada = -1;
        ms_pl.marca = 0;
        long voltas = 0;
        while (!ms_fatia(37)) {
            n_fatias++;
            retrato_t meio;
            retrata(&meio);
            CONFERE(!memcmp(&antes, &meio, sizeof antes), "partida %d rodada %d: um pedaco do plano mexeu no jogo", partida, rodada);
            if (++voltas > 2000000) { CONFERE(false, "partida %d: o plano em pedacos nao acaba", partida); break; }
        }
        int n_ped = plano_n;
        float p_ped = plano_p;
        memcpy(em_pedacos, plano, N_FILA);
        plano_rodada = -1;
        ms_pl.marca = 0;
        int i = ia_proximo_da_fila();
        CONFERE(plano_n == n_ped && p_ped == plano_p && !memcmp(em_pedacos, plano, (size_t)plano_n),
                "partida %d rodada %d: a fila em pedacos (%d dados) nao e a de uma vez (%d)", partida, rodada, n_ped, plano_n);
        CONFERE(plano_n <= max_fila(j), "partida %d: fila de %d dados (cabem %d)", partida, plano_n, max_fila(j));
        CONFERE(plano_n > 0 || J[j].n_mao == 0, "partida %d rodada %d: fila vazia com %d dados na mao", partida, rodada, J[j].n_mao);
        for (int a = 0; a < plano_n; a++) {
            CONFERE(plano[a] >= 0 && plano[a] < J[j].n_mao, "partida %d: a fila pede o lugar %d da mao de %d", partida, plano[a], J[j].n_mao);
            for (int b = 0; b < a; b++) CONFERE(plano[a] != plano[b], "partida %d: a fila repete o lugar %d da mao", partida, plano[a]);
        }
        if (F[j].grudado && plano_n > 0)
            CONFERE(J[j].mao[plano[0]] == F[j].idx[0], "partida %d: o dado grudado nao abre a fila", partida);
        CONFERE(i == (F[j].n < plano_n ? plano[F[j].n] : -1), "partida %d: o proximo da fila (%d) nao e o do plano", partida, i);
        n_fila++;
    } else if (fase == F_JOGA) {
        bool segue = mestre_continua();
        CONFERE(!segue || F[j].lancados < F[j].n, "partida %d: quer lancar sem dado na fila", partida);
        CONFERE(segue || F[j].lancados > 0, "partida %d: quer parar sem lancar nenhum", partida);
        n_lanca++;
    } else if (fase == F_APOSTA) {
        int op[4], n = opcoes(op), i = mestre_aposta();
        CONFERE(i >= 0 && i < n, "partida %d: aposta fora das opcoes (%d de %d)", partida, i, n);
        float t = 0;
        for (int a = 0; a < ms_ult_n; a++) {
            bool tem = false;
            for (int b = 0; b < n; b++) tem |= op[b] == ms_ult_ops[a];
            CONFERE(tem, "partida %d: o plano da aposta tem um lance que nao esta nas opcoes", partida);
            CONFERE(ms_ult_q[a] >= 0 && ms_ult_q[a] <= 1.001f, "partida %d: chance %f no plano da aposta", partida, ms_ult_q[a]);
            t += ms_ult_q[a];
        }
        CONFERE(t > 0.99f && t < 1.01f, "partida %d: as chances do plano da aposta somam %f", partida, t);
        n_aposta++;
    }
    semente_ia = sem_ia;
    ms_apostou[0] = apostou[0]; ms_apostou[1] = apostou[1];
    retrata(&depois);
    CONFERE(!memcmp(&antes, &depois, sizeof antes), "partida %d rodada %d fase %d: pensar mexeu no jogo%s", partida, rodada, fase,
            antes.semente != depois.semente ? " (no sorteio da partida)" : "");
}

typedef struct { int venc, fichas[2], rodadas; uint32_t semente; } fim_t;
static double t_pior, t_soma;
static long t_n;

// Uma partida ate o fim. confere: a cada decisao da mestre, pede a decisao
// direto antes (confere_decisao); mede: o tempo de cada quadro em que ela pensa.
static fim_t joga(int partida, bool confere, bool mede)
{
    long passos = 0;
    int conferida = -1;
    while (fase != F_FIM) {
        if (++passos > 3000000) { CONFERE(false, "partida %d parada na fase %d", partida, fase); break; }
        relogio += 16667;
        if (fase == F_RESULTADO) { trata_tecla(KEY_ENTER); continue; }
        bool dela = ia_na_vez() && nivel_jog[vez & 1] >= 3 && t_anuncio <= 0;
        int chave = ((rodada * 8 + fase) * 2 + vez) * 64 + F[vez].n * 8 + F[vez].lancados + a_pagar * 4096 + passes * 2048;
        if (confere && dela && chave != conferida) { conferida = chave; confere_decisao(partida); }
        cpu = 3;
        double t0 = mede && dela ? agora_ms() : 0;
        dado_passo(1.0f / 60);
        if (mede && dela) {
            double dt = agora_ms() - t0;
            if (dt > 0.02) { t_soma += dt; t_n++; }
            if (dt > t_pior) t_pior = dt;
        }
        for (int j = 0; j < 2; j++) CONFERE(J[j].fichas >= 0, "partida %d: lugar %d com %d fichas", partida, j, J[j].fichas);
        CONFERE(pote >= 0, "partida %d: pote %d", partida, pote);
    }
    return (fim_t){ venc_partida, { J[0].fichas, J[1].fichas }, rodada, semente };
}

int main(int argc, char **argv)
{
    int partidas = argc > 1 ? atoi(argv[1]) : 12;
    ia_esforco = argc > 2 ? atoi(argv[2]) : 0;
    double limite = argc > 3 ? atof(argv[3]) : 250;
    natal_forcado = 0;
    perfil_teste();

    // Os niveis de baixo nao passam pela mestre: uma partida da afiada contra
    // a atenta nao toca em nada dela.
    {
        peca_t b0[MAX_COL], b1[MAX_COL];
        int n0 = bolsa_jogador(b0), n1 = bolsa_oponente(b1);
        comeca(b0, n0, b1, n1, 2, 1, 12345);
        joga(-1, false, false);
        CONFERE(ms_pl.marca == 0 && ms_ult_n == 0, "a afiada e a atenta passaram pela mestre");
    }

    for (int p = 0; p < partidas && falhas < 20; p++) {
        peca_t b0[MAX_COL], b1[MAX_COL];
        int n0, n1;
        if (p % 2) { n0 = bolsa_ao_acaso(b0); n1 = bolsa_ao_acaso(b1); }
        else { n0 = bolsa_jogador(b0); n1 = bolsa_oponente(b1); }
        uint32_t s = rnd_t();
        int niv0 = p % 3 == 1 ? 2 : 3, niv1 = p % 3 == 2 ? 2 : 3;   // mestre x mestre, afiada x mestre, mestre x afiada
        // Pensando a mais (a decisao pedida direto antes de cada gesto) e sem
        // pensar a mais: a mesma partida, lance a lance.
        relogio = 7919LL * (p + 1) * 1000003LL;
        comeca(b0, n0, b1, n1, niv0, niv1, s);
        fim_t a = joga(p, true, false);
        relogio = 7919LL * (p + 1) * 1000003LL;
        comeca(b0, n0, b1, n1, niv0, niv1, s);
        fim_t b = joga(p, false, true);
        CONFERE(a.venc == b.venc && a.fichas[0] == b.fichas[0] && a.fichas[1] == b.fichas[1] && a.rodadas == b.rodadas
                && a.semente == b.semente,
                "partida %d: pensando a mais deu %d x %d em %d rodadas; sem, %d x %d em %d", p, a.fichas[0], a.fichas[1],
                a.rodadas, b.fichas[0], b.fichas[1], b.rodadas);
    }
    CONFERE(n_fila > 0 && n_lanca > 0 && n_aposta > 0 && n_fatias > 0, "a mestre nao decidiu de tudo (%ld filas, %ld lances, %ld apostas)",
            n_fila, n_lanca, n_aposta);
    CONFERE(t_pior <= limite, "um quadro da mestre levou %.1f ms (limite %.0f)", t_pior, limite);
    printf("%d partidas: %ld filas, %ld lances e %ld apostas da mestre conferidos (%ld pedacos de plano)\n", partidas, n_fila,
           n_lanca, n_aposta, n_fatias);
    printf("tempo de um quadro em que a mestre pensa (esforco %d): medio %.2f ms, pior %.2f ms\n", ia_esforco,
           t_n ? t_soma / (double)t_n : 0.0, t_pior);
    if (falhas) { printf("%d falhas\n", falhas); return 1; }
    printf("tudo certo\n");
    return 0;
}
