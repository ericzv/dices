// Arena das IAs: uma IA contra a outra, em partidas inteiras no formato do
// Modo Desafiante (8 rodadas, 50 fichas, a entrada que sobe), sem trunfos nem
// amuletos, com bolsas do Desafiante. Nao e teste de passar ou falhar (fica
// fora do ctest): imprime numeros para comparar IAs.
//
//   arena <pares> <nivelA> <nivelB> [semente] [bolsas]
//
// Cada par sao duas partidas com as mesmas bolsas e a mesma semente (os
// mesmos sorteios da bolsa e os mesmos lances enquanto as jogadas forem as
// mesmas): numa, A senta no lugar 0 e B no 1; na outra, trocam. Assim cada IA
// joga uma vez com cada bolsa, e uma vez de cada lado de quem abre.
//   nivel: 0 a simples, 1 a atenta, 2 a afiada, 3 a mestre; 11 a 15, a afiada
//     apostando de um jeito fixo (escolhe_aposta); 30 a 40, a mestre com a
//     parte franca da aposta de 0 a 100% (30: o equilibrio puro; o jogo usa 38).
//   bolsas: 0 misturado (padrao); 1 so as bolsas de saida do jogador (com
//     dados da vitrine); 2 so as dos oponentes (monta_oponente, com chefes);
//     3 a bolsa inicial dos dois lados (os oito dados de sempre).
//
// Saida: uma linha PAR por par (para somar varias execucoes em paralelo com
// tools/arena_soma.py) e o resumo com o intervalo de confianca de 95%, por
// pares. E o tempo de cada IA, por fase: o de cada quadro em que ela pensou
// (o que trava a tela, se for grande) e o de cada decisao inteira (a mestre
// reparte a fila por varios quadros).
//
// No ambiente:
//   ESFORCO=0  as IAs pensam pouco (o jogo usa 1).
//   LOG=arq    uma linha por rodada (R: a rodada, quem abriu, quem levou - 2
//              empate -, se alguem correu, as fichas de cada lugar depois dela,
//              o pote que ficou) e uma FIM por partida.
//   LAB=1      o laboratorio da aposta (lab_mede): em vez de partidas, mede so
//              a aposta, em fichas, sem a sorte dos lances. Com FICHAS=1 a
//              mestre aposta pelas fichas da rodada, e nao pela partida (e o
//              que da para comparar em fichas).
//   LAB=2      imprime as maos que quem abriu imagina (linhas TIPOS), para a curva.
//   LAB=3      a chance da mao pela regra simples contra a da fila planejada
//              (linhas CAL: de onde sai MS_ACHA).
//   LAB=4      a fila: a da afiada contra a da mestre, na mesma situacao.
//   LAB=5      o tempo de cada parte da aposta de quem abriu (linhas PARTES).
//   (os LAB jogam as partidas com ESFORCO=0, so para chegar nas situacoes.)
//
//   arena curva <arquivo com linhas TIPOS>
//       refaz a curva da partida (MS_CURVA de ia.inc) e imprime a tabela:
//         LAB=2 ESFORCO=0 arena 60 2 2 77 | grep TIPOS > tipos.txt
//         arena curva tipos.txt
#include "arena_comum.h"

static int jeito_jog[2];                 // o jeito de apostar de cada lugar (escolhe_aposta); 0: o da IA

// A IA de um lugar, pelo numero pedido (veja o comeco).
static void senta(int lugar, int niv)
{
    jeito_jog[lugar] = 0;
    ms_franca_jog[lugar] = MS_FRANCA;
    if (niv >= 30 && niv <= 40) { nivel_jog[lugar] = 3; ms_franca_jog[lugar] = (float)(niv - 30) / 10.0f; }
    else if (niv >= 10) { nivel_jog[lugar] = 2; jeito_jog[lugar] = niv - 10; }
    else nivel_jog[lugar] = niv;
}

// O tempo, por nivel da IA e por fase (0 fila, 1 lancar, 2 aposta): de cada
// quadro em que ela pensou e de cada decisao inteira.
#define N_NIV 4
typedef struct { double soma, pior; long n; } tempo_t;
static tempo_t t_quadro[N_NIV][3], t_decisao[N_NIV][3];
static void tempo_junta(tempo_t *t, double dt) { t->soma += dt; t->n++; if (dt > t->pior) t->pior = dt; }
// Jeitos de apostar para medir a aposta da IA (nivel 10 + jeito: joga como a
// afiada, mas aposta assim): 1 nunca aposta e sempre paga; 2 sempre 25 e
// sempre dobra; 3 a IA, mas nunca corre; 4 nunca aposta e corre de tudo; 5
// sempre 25 quando ninguem apostou, e a IA no resto.
static int escolhe_aposta(int jeito)
{
    int op[4], n = opcoes(op), quer = OP_PASSAR;
    ia_nivel = 2;
    ia_ousadia = 0;
    switch (jeito) {
    case 1: quer = a_pagar ? OP_PAGAR : OP_PASSAR; break;
    case 2: quer = a_pagar ? OP_AUMENTAR : OP_APOSTAR25; break;
    case 3: quer = op[ia_aposta()]; if (quer == OP_CORRER) quer = OP_PAGAR; break;
    case 4: quer = a_pagar ? OP_CORRER : OP_PASSAR; break;
    case 5: quer = a_pagar ? op[ia_aposta()] : OP_APOSTAR25; break;
    }
    for (int i = 0; i < n; i++) if (op[i] == quer) return i;
    for (int i = 0; i < n; i++) if (op[i] == OP_PAGAR || op[i] == OP_PASSAR) return i;
    return 0;
}

// ---- O laboratorio da aposta ----
// Em cada aposta que aparece nas partidas, mede so a aposta: com a mesma mesa
// e a mesma mao de quem joga depois, cada IA aposta no lugar de quem abriu e
// de quem joga depois, e a conta fecha na media dos dados (a chance de
// verdade da mao, medida com 300 jogadas da fila planejada), sem a sorte dos
// lances. O numero: a diferenca de fichas (quem abriu menos o outro) depois
// da rodada.
static int lab_ligado, lab_pol[2], lab_rodada;
static double lab_p;
static double lab_ev(ms_ap_t s, int pr)
{
    J[pr].fichas = s.f[0]; J[outro(pr)].fichas = s.f[1];
    pote = s.pote; a_pagar = s.a_pagar; passes = s.passes; aumentou = s.aumentou;
    vez = s.vez ? outro(pr) : pr;
    float q[OP_CORRER + 1] = { 0 }, t = 0;
    ia_nivel = lab_pol[s.vez];
    ia_ousadia = 0;
    if (ia_nivel >= 3) {
        mestre_aposta();
        for (int a = 0; a < ms_ult_n; a++) if (ms_ult_q[a] >= 0.04f) { q[ms_ult_ops[a]] = ms_ult_q[a]; t += ms_ult_q[a]; }
    } else {
        int op[4];
        opcoes(op);
        q[op[ia_aposta()]] = 1;
        t = 1;
    }
    double ev = 0;
    for (int o = 0; o <= OP_CORRER; o++) {
        if (q[o] <= 0) continue;
        ms_ap_t s2 = s;
        int fim = ms_faz(&s2, o);
        double v;
        if (fim == 2) v = s.vez == 0 ? s2.f[0] - s2.f[1] - s2.pote : s2.f[0] + s2.pote - s2.f[1];
        else if (fim == 1) v = (1 - lab_p) * (s2.f[0] + s2.pote - s2.f[1]) + lab_p * (s2.f[0] - s2.f[1] - s2.pote);
        else v = lab_ev(s2, pr);
        ev += q[o] / t * v;
    }
    return ev;
}
typedef struct { double s, s2; long n; } soma_t;
static void junta(soma_t *a, double x) { a->s += x; a->s2 += x * x; a->n++; }
static soma_t lab_abre, lab_resp, lab_33, lab_22, lab_chance, lab_plano;
static void lab_mede(void)
{
    int pr = primeiro, b = outro(pr), esf = ia_esforco;
    if (chiclete_na_mao(b) >= 0) return;
    ms_ap_t s = { .f = { J[pr].fichas, J[b].fichas }, .pote = pote };
    ia_esforco = 1;
    vez = b;
    ia_nivel = 2;
    plano_rodada = -1;
    float est = ia_chance(), chance = 0;         // a fila de quem joga depois, planejada pela afiada
    mesa_t m;
    guarda_mesa(&m);
    avalia_fila_c(b, plano, plano_n, 300, 0x1AB0u + (uint32_t)rodada * 131u, &m, &chance);
    volta_mesa(&m);
    lab_p = chance;
    double ev[2][2] = { { 0 } };
    for (int a = 0; a < 2 && lab_ligado == 1; a++)
        for (int c = 0; c < 2; c++) {
            lab_pol[0] = 2 + a; lab_pol[1] = 2 + c;
            ev[a][c] = lab_ev(s, pr) - (s.f[0] - s.f[1]);
        }
    if (lab_ligado == 3) {                       // a mao de verdade pela regra simples (como ms_tipos a ve) contra a planejada
        uint8_t onde0[MAX_COL];
        memcpy(onde0, J[b].onde, sizeof onde0);
        for (int i = 0; i < J[b].n_col; i++) if (J[b].onde[i] == NA_BOLSA) J[b].onde[i] = FORA;
        float sq = 0;
        guarda_mesa(&m);
        for (int a = 0; a < 300; a++) {
            volta_mesa(&m);
            semeia_ia(0x77u, 0);
            fila_imaginada(b, false);
            semeia_ia(0x1234u + (uint32_t)rodada, a);
            sim_turno(b);
            sq += placar(b);
        }
        volta_mesa(&m);
        memcpy(J[b].onde, onde0, sizeof onde0);
        printf("CAL %.3f %.3f %d\n", sq / 300, chance, J[b].n_mao);
    }
    if (lab_ligado == 5) {                       // o tempo de cada parte da aposta de quem abriu
        float pk[MS_TIPOS + 1], w[MS_TIPOS + 1];
        ms_maos.marca = 0;
        double t0 = agora_ms();
        float crua = ms_tipos(b, pk);
        double t1 = agora_ms();
        for (int k = 0; k < MS_TIPOS; k++) w[k] = 1.0f / MS_TIPOS;
        ms_resolve(&s, pk, w, MS_TIPOS, 0, MS_FRANCA, crua);
        double t2 = agora_ms();
        printf("PARTES tipos %.2f resolve %.2f\n", t1 - t0, t2 - t1);
    }
    if (lab_ligado == 2) {                       // so as maos que quem abriu imagina, para a curva
        float pk[MS_TIPOS + 1];
        ms_tipos(b, pk);
        printf("TIPOS");
        for (int k = 0; k < MS_TIPOS; k++) printf(" %.3f", pk[k]);
        printf("\n");
    }
    junta(&lab_22, ev[0][0]);
    junta(&lab_abre, ev[1][0] - ev[0][0]);       // a mestre abrindo, contra a afiada: o que ganha a mais que a afiada no lugar dela
    junta(&lab_resp, ev[0][0] - ev[0][1]);       // a mestre jogando depois
    junta(&lab_33, ev[1][1]);
    junta(&lab_chance, chance);
    junta(&lab_plano, est);
    J[pr].fichas = s.f[0]; J[b].fichas = s.f[1];
    pote = s.pote; a_pagar = 0; passes = 0; aumentou = false;
    vez = pr;
    ia_esforco = esf;
}

// LAB=4: a fila. Na mesma situacao (quem abre, antes de montar; quem joga
// depois, na aposta), a afiada e a mestre planejam, e cada fila joga as
// mesmas 400 rodadas de mentira novas: a chance de vencer a rodada de cada
// uma, e o tempo de cada plano.
static soma_t fila_dif[2], fila_chance[2], fila_t2[2], fila_t3[2];
static void lab_fila(int quem)
{
    int j = vez, esf = ia_esforco;
    if (F[j].n || chiclete_na_mao(j) >= 0) return;
    ia_esforco = 1;
    int8_t f2[N_FILA], f3[N_FILA];
    int n2, n3;
    float c2 = 0, c3 = 0;
    mesa_t m;
    ia_nivel = 2; plano_rodada = -1;
    double t0 = agora_ms();
    ia_planeja();
    double t1 = agora_ms();
    n2 = plano_n; memcpy(f2, plano, N_FILA);
    ia_nivel = 3; plano_rodada = -1; ms_pl.marca = 0;
    ia_planeja();
    double t2 = agora_ms();
    n3 = plano_n; memcpy(f3, plano, N_FILA);
    guarda_mesa(&m);
    uint32_t base = 0x51F1A0u + (uint32_t)rodada * 977u;
    if (n2 > 0 && n3 > 0) {
        avalia_fila_c(j, f2, n2, 400, base, &m, &c2);
        avalia_fila_c(j, f3, n3, 400, base, &m, &c3);
        volta_mesa(&m);
        junta(&fila_dif[quem], c3 - c2);
        junta(&fila_chance[quem], c2);
        junta(&fila_t2[quem], t1 - t0);
        junta(&fila_t3[quem], t2 - t1);
    }
    plano_rodada = -1; ms_pl.marca = 0;
    ia_esforco = esf;
}

typedef struct { int rod[2], correu[2], empates; } placar_t;   // por lugar: rodadas vencidas e vezes que correu
static FILE *reg_arq;                    // LOG=arquivo

// Joga a partida ate o fim; devolve o vencedor (0, 1; 2 empate).
static int joga(placar_t *pl)
{
    long passos = 0, dec_chave = -1;
    double dec_t = 0;
    int dec_niv = 0, dec_f = 0;
    memset(pl, 0, sizeof *pl);
    lab_rodada = 0;
    while (fase != F_FIM) {
        if (++passos > 3000000) { printf("partida parada na fase %d\n", fase); exit(1); }
        relogio += 16667;
        if (fase == F_RESULTADO) {
            if (reg_arq) fprintf(reg_arq, "R %d %d %d %d %d %d %d\n", rodada, primeiro, venc_mao >= 0 ? venc_mao : correu_venc,
                                 venc_mao < 0, J[0].fichas, J[1].fichas, pote);
            if (venc_mao == 0 || venc_mao == 1) pl->rod[venc_mao]++;
            else if (venc_mao == 2) pl->empates++;
            else if (correu_venc >= 0) { pl->rod[correu_venc]++; pl->correu[outro(correu_venc)]++; }
            trata_tecla(KEY_ENTER);
            continue;
        }
        if (lab_ligado == 4) {
            if (fase == F_ORDEM && vez == primeiro && !F[vez].n && lab_rodada != rodada) { lab_rodada = rodada; lab_fila(0); }
            if (fase == F_APOSTA && vez != primeiro && lab_rodada != -rodada) { lab_rodada = -rodada; lab_fila(1); }
        } else if (lab_ligado && fase == F_APOSTA && vez == primeiro && !a_pagar && !passes && lab_rodada != rodada) {
            lab_rodada = rodada;
            lab_mede();
        }
        if (fase == F_APOSTA && jeito_jog[vez & 1] && t_anuncio <= 0) {
            cursor = escolhe_aposta(jeito_jog[vez & 1]);
            trata_tecla(KEY_ENTER);
            continue;
        }
        int f = fase == F_ORDEM ? 0 : fase == F_JOGA ? 1 : fase == F_APOSTA ? 2 : -1;
        bool decide = f >= 0 && ia_na_vez();
        int niv = nivel_jog[vez & 1];
        // A decisao e a mesma enquanto nada disto muda (a fila inteira conta como uma).
        long chave = ((((long)rodada * 8 + fase) * 2 + vez) * 8 + (f == 0 ? 0 : F[vez].lancados)) * 64 + a_pagar + passes * 32;
        if (chave != dec_chave) {
            if (dec_t > 0.02) tempo_junta(&t_decisao[dec_niv][dec_f], dec_t);
            dec_chave = chave; dec_t = 0; dec_niv = niv < N_NIV ? niv : 0; dec_f = f < 0 ? 0 : f;
        }
        cpu = 3;
        double t0 = decide ? agora_ms() : 0;
        dado_passo(1.0f / 60);
        if (decide) {
            double dt = agora_ms() - t0;
            if (dt > 0.02 && niv >= 0 && niv < N_NIV) {      // so os passos em que ela pensou
                tempo_junta(&t_quadro[niv][f], dt);
                dec_t += dt;
            }
        }
    }
    if (reg_arq) fprintf(reg_arq, "FIM %d %d %d\n", venc_partida, J[0].fichas, J[1].fichas);
    return venc_partida;
}

static void mostra(const char *nome, const soma_t *a, double escala, const char *un)
{
    double m = a->s / (double)a->n, v = a->s2 / (double)a->n - m * m;
    double e = a->n > 1 ? 1.96 * sqrt(v / (double)(a->n - 1)) : 0;
    printf("%-34s %7.2f%s  IC95 [%7.2f, %7.2f]\n", nome, m * escala, un, (m - e) * escala, (m + e) * escala);
}

// ---- A curva da partida ----
// De tras para a frente: a ultima rodada primeiro (depois dela so importa quem
// fica na frente); em cada rodada antes, o valor da aposta resolvida (o
// equilibrio puro) para quem abre, na media das mesas do arquivo (linhas
// TIPOS, de LAB=2), com a curva da rodada seguinte no fim. Imprime a tabela
// MS_CURVA de ia.inc.
static uint16_t curva_nova[DES_RODADAS][MS_CURVA_N];
static void faz_curva(const char *arq)
{
    static float lib[2000][MS_TIPOS];
    int n = 0;
    char linha[2048];
    FILE *f = fopen(arq, "r");
    if (!f) { printf("nao abriu %s\n", arq); exit(1); }
    while (n < 2000 && fgets(linha, sizeof linha, f)) {
        if (strncmp(linha, "TIPOS", 5)) continue;
        char *c = linha + 5;
        int k = 0;
        for (; k < MS_TIPOS; k++) { char *e; lib[n][k] = strtof(c, &e); if (e == c) break; c = e; }
        if (k == MS_TIPOS) n++;
    }
    fclose(f);
    if (!n) { printf("nenhuma linha TIPOS em %s\n", arq); exit(1); }
    modo = M_DESAFIO;
    ms_curva = (const uint16_t (*)[MS_CURVA_N])curva_nova;
    float w[MS_TIPOS];
    for (int k = 0; k < MS_TIPOS; k++) w[k] = 1.0f / MS_TIPOS;
    for (int r = DES_RODADAS; r >= 1; r--) {
        rodada = r;
        int ante = ante_da(r);
        for (int i = 0; i < MS_CURVA_N; i++) {
            int d = -100 + 4 * i, fa = (100 + d) / 2, fb = (100 - d) / 2;
            double v = 0;
            if (fa < ante || fb < ante) v = fa < ante && fb < ante ? (d > 0 ? 1 : d < 0 ? 0 : 0.5) : fa < ante ? 0 : 1;
            else {
                ms_ap_t s = { .f = { fa - ante, fb - ante }, .pote = 2 * ante };
                for (int q = 0; q < n; q++) v += ms_resolve(&s, lib[q], w, MS_TIPOS, 0, 0.0f, 0.5f);   // o equilibrio puro, sem a parte franca
                v /= n;
            }
            curva_nova[r - 1][i] = (uint16_t)(v * 10000 + 0.5);
        }
    }
    for (int r = 0; r < DES_RODADAS; r++) {
        printf("    {");
        for (int i = 0; i < MS_CURVA_N; i++) printf(" %d,", curva_nova[r][i]);
        printf(" },\n");
    }
}

int main(int argc, char **argv)
{
    if (argc > 2 && !strcmp(argv[1], "curva")) { faz_curva(argv[2]); return 0; }
    #define ARG(i, d) (argc > (i) ? atoi(argv[i]) : (d))
    int pares = ARG(1, 20), nivA = ARG(2, 2), nivB = ARG(3, 2), jeito = ARG(5, 0);
    uint32_t sem = (uint32_t)ARG(4, 1);
    ia_esforco = getenv("ESFORCO") ? atoi(getenv("ESFORCO")) : 1;
    natal_forcado = 0;
    if (getenv("LOG")) reg_arq = fopen(getenv("LOG"), "w");
    lab_ligado = getenv("LAB") ? atoi(getenv("LAB")) : 0;
    if (lab_ligado) ia_esforco = 0;
    if (getenv("FICHAS") && atoi(getenv("FICHAS"))) ms_curva = NULL;
    est = sem * 2654435761u | 1;
    for (int i = 0; i < 8; i++) rnd_t();
    perfil_teste();
    soma_t partidas = { 0 }, rodadas = { 0 }, fichas = { 0 };
    long nr = 0, correuA = 0, correuB = 0, emp = 0, npart = 0;
    for (int p = 0; p < pares; p++) {
        peca_t b0[MAX_COL], b1[MAX_COL];
        int n0, n1, q = jeito ? jeito : 1 + (int)(rnd_t() % 2u);
        if (q == 3) { n0 = bolsa_de_saida(b0); n1 = bolsa_de_saida(b1); }
        else if (q == 1) { n0 = bolsa_jogador(b0); n1 = (rnd_t() & 1) ? bolsa_jogador(b1) : bolsa_oponente(b1); }
        else { n0 = bolsa_oponente(b0); n1 = bolsa_oponente(b1); }
        uint32_t s = rnd_t();
        double vA = 0, rA = 0, rT = 0, fA = 0;
        for (int lado = 0; lado < 2; lado++) {   // lado: o lugar em que A senta
            relogio = 7919LL * (p + 1) * 1000003LL + s;
            comeca(b0, n0, b1, n1, 2, 2, s);
            senta(lado, nivA);
            senta(outro(lado), nivB);
            placar_t pl;
            int w = joga(&pl);
            vA += w == 2 ? 0.5 : w == lado ? 1 : 0;
            rA += pl.rod[lado] + 0.5 * pl.empates;
            rT += pl.rod[0] + pl.rod[1] + pl.empates;
            fA += J[lado].fichas - J[outro(lado)].fichas;
            correuA += pl.correu[lado]; correuB += pl.correu[outro(lado)];
            emp += pl.empates;
            npart++;
        }
        nr += (long)rT;
        junta(&partidas, vA / 2);
        junta(&rodadas, rA / rT);
        junta(&fichas, fA / 2);
        if (!lab_ligado) printf("PAR %.1f %.1f %.0f %.0f\n", vA, rA, rT, fA);
        fflush(stdout);
    }
    if (lab_ligado == 4) {
        for (int q = 0; q < 2; q++) {
            printf("FILA de quem %s: %ld situacoes; a da afiada vence %.3f\n", q ? "joga depois" : "abre", fila_dif[q].n,
                   fila_chance[q].s / (double)fila_chance[q].n);
            mostra("  a da mestre, a mais (pontos)", &fila_dif[q], 100, " ");
            printf("  tempo do plano: afiada %.1f ms, mestre %.1f ms\n", fila_t2[q].s / (double)fila_t2[q].n, fila_t3[q].s / (double)fila_t3[q].n);
        }
        return 0;
    }
    if (lab_ligado) {
        printf("LAB %ld apostas; a chance de quem joga depois: %.3f (a afiada estima %.3f)\n", lab_22.n,
               lab_chance.s / (double)lab_chance.n, lab_plano.s / (double)lab_plano.n);
        mostra("afiada x afiada: quem abre leva", &lab_22, 1, " ");
        mostra("mestre abrindo: fichas a mais", &lab_abre, 1, " ");
        mostra("mestre respondendo: fichas a mais", &lab_resp, 1, " ");
        mostra("mestre x mestre: quem abre leva", &lab_33, 1, " ");
        return 0;
    }
    printf("ARENA nivel %d (A) contra nivel %d (B): %ld partidas (%d pares), %ld rodadas, esforco %d\n",
           nivA, nivB, npart, pares, nr, ia_esforco);
    mostra("partidas vencidas por A", &partidas, 100, "%");
    mostra("rodadas vencidas por A", &rodadas, 100, "%");
    mostra("saldo de fichas de A por partida", &fichas, 1, " ");
    printf("correu: A %.2f, B %.2f por partida; empates de rodada %.2f\n", (double)correuA / (double)npart,
           (double)correuB / (double)npart, (double)emp / (double)npart);
    static const char *const FASE[3] = { "fila", "lancar", "aposta" };
    for (int d = 0; d < 2; d++)
        for (int n = 0; n < N_NIV; n++)
            for (int f = 0; f < 3; f++) {
                const tempo_t *t = d ? &t_decisao[n][f] : &t_quadro[n][f];
                if (t->n) printf("TEMPO %s nivel %d %-7s n %7ld  medio %7.2f ms  pior %7.2f ms\n", d ? "decisao" : "quadro", n,
                                 FASE[f], t->n, t->soma / (double)t->n, t->pior);
            }
    return 0;
}
