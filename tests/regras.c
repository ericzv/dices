// Teste das regras dos dados, com jogadas montadas a mao: cada caso poe dados
// e valores exatos na fila e confere o que a regra da fila e o fim da rodada
// fazem. Inclui o dado.c para alcancar as funcoes internas.
#include <stdio.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
int64_t esp_timer_get_time(void) { return 1; }
void som_toca(int s) { (void)s; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }
void som_giro(bool on) { (void)on; }

// Salvamento do Modo Desafiante na memoria (sem arquivo nos testes).
#include "salva_mem.h"

static int falhas;

// Posicao (na lista do evento) de um dado do tipo t; -1 se nao ha.
static int acha_na_lista(int t);
#define CONFERE(cond, ...) do { if (!(cond)) { printf("FALHOU %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); putchar('\n'); falhas++; } } while (0)

static void limpa(void)
{
    memset(F, 0, sizeof F);
    primeiro = 0;
    n_ev = 0;
}

// Poe o dado de tipo t com o valor v no fim da fila de j e aplica a regra.
static void joga(int j, int t, int v)
{
    fila_t *f = &F[j];
    int k = f->lancados;
    f->tipo[k] = (uint8_t)t;
    f->n = k + 1;
    f->cru[k] = f->valor[k] = v;
    f->lancados = k + 1;
    aplica_regra(j, k);
}

// Como joga, mas o dado cai pela regra completa (Ventania inclusive) e os
// lances que ela refaz saem de 'r'.
static int sempre_dois(int lados) { (void)lados; return 2; }
static void joga_com(int j, int t, int v, int (*r)(int))
{
    fila_t *f = &F[j];
    int k = f->lancados;
    f->tipo[k] = (uint8_t)t;
    f->n = k + 1;
    f->cru[k] = f->valor[k] = v;
    f->lancados = k + 1;
    pousa(j, k, r);
}

static desfecho_t fim(void)
{
    desfecho_t d;
    calcula_desfecho(&d, true);
    return d;
}

// Mesa de apostas limpa: A (jogador 0) joga primeiro e abre a aposta.
static void aposta_zera(int a, int b)
{
    J[0].fichas = a; J[1].fichas = b;
    pote = 0; a_pagar = 0; passes = 0; aumentou = false;
    primeiro = vez = 0;
    fase = F_APOSTA;
}

// Comeco de rodada com estas fichas e este pote parado na mesa.
static void entrada(int a, int b, int p)
{
    for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
    J[0].fichas = a; J[1].fichas = b; pote = p;
    rodada = 1;
    fase = F_RESULTADO;
    nova_rodada();
}

static uint32_t teste_estado = 12345;
static uint32_t teste_rnd(void)
{
    teste_estado ^= teste_estado << 13; teste_estado ^= teste_estado >> 17; teste_estado ^= teste_estado << 5;
    return teste_estado;
}

int main(void)
{
    desfecho_t d;

    // Espinhoso: com 3 ou 4 tira 4 do oponente (e vale para o dono).
    limpa();
    joga(0, D_MARTELO, 4);
    joga(1, D_D6, 6);
    d = fim();
    CONFERE(d.total[1] == 2, "Espinhoso 4: oponente devia ficar com 2, ficou %d", d.total[1]);
    CONFERE(d.total[0] == 4, "Espinhoso 4: dono devia fazer 4, fez %d", d.total[0]);
    limpa();
    joga(0, D_MARTELO, 3);
    joga(1, D_D6, 6);
    d = fim();
    CONFERE(d.total[1] == 2 && d.total[0] == 3, "Espinhoso 3: tira 4 (%d x %d)", d.total[0], d.total[1]);
    limpa();
    joga(0, D_MARTELO, 2);
    joga(1, D_D6, 6);
    d = fim();
    CONFERE(d.total[1] == 6 && d.total[0] == 2, "Espinhoso 2: nao tira nada (%d x %d)", d.total[0], d.total[1]);

    // Maldito: soma para o dono e tira o mesmo do oponente.
    limpa();
    joga(0, D_MALDICAO, 3);
    joga(1, D_D8, 8);
    d = fim();
    CONFERE(d.total[0] == 3 && d.total[1] == 5, "Maldito 3: esperava 3 x 5, deu %d x %d", d.total[0], d.total[1]);

    // Pirata: com 6+ tira 4 do oponente, sem ficar com eles.
    limpa();
    joga(0, D_PIRATA, 7);
    joga(1, D_D8, 8);
    d = fim();
    CONFERE(d.total[0] == 7 && d.total[1] == 4, "Pirata 7: esperava 7 x 4, deu %d x %d", d.total[0], d.total[1]);

    // Agouro: 7 ou menos derruba a sequencia; 8, nao.
    limpa();
    joga(0, D_D6, 5);
    joga(0, D_MALDITO, 7);
    CONFERE(F[0].est[0] == V_ANULADO && F[0].est[1] == V_ANULADO, "Agouro 7 devia anular a sequencia");
    limpa();
    joga(0, D_D6, 5);
    joga(0, D_MALDITO, 8);
    CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO, "Agouro 8 nao devia anular");

    // Copiador 4: cada outro 4 ganha +2.
    limpa();
    joga(0, D_COPIADOR, 4);
    joga(0, D_D6, 4);
    joga(0, D_D8, 4);
    joga(0, D_D8, 7);
    d = fim();
    CONFERE(d.total[0] == 4 + 6 + 6 + 7, "Copiador: esperava 23, deu %d", d.total[0]);

    // Copiador anulado nao da nada.
    limpa();
    joga(0, D_D8, 5);
    joga(0, D_COPIADOR, 3);          // menor: anula os dois
    joga(0, D_D6, 3);
    d = fim();
    CONFERE(d.total[0] == 3, "Copiador anulado: esperava 3, deu %d", d.total[0]);

    // Jackpot: tres iguais (sem contar ele) com o Jackpot valendo: +10.
    limpa();
    joga(0, D_JACKPOT, 1);
    joga(0, D_D6, 5);
    joga(0, D_D6, 5);
    joga(0, D_D8, 5);
    d = fim();
    CONFERE(d.total[0] == 1 + 15 + 10, "Jackpot com trinca: esperava 26, deu %d", d.total[0]);
    bool viu = false;
    for (int i = 0; i < n_ev; i++) if (ev[i].tipo == EV_JACKPOT) viu = true;
    CONFERE(viu, "Jackpot com trinca: faltou o evento");

    // Jackpot contando ele mesmo na trinca.
    limpa();
    joga(0, D_D4, 3);
    joga(0, D_JACKPOT, 3);
    joga(0, D_D6, 3);
    d = fim();
    CONFERE(d.total[0] == 9 + 10, "Jackpot na trinca: esperava 19, deu %d", d.total[0]);

    // Sem trinca, nada.
    limpa();
    joga(0, D_JACKPOT, 2);
    joga(0, D_D6, 2);
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.total[0] == 9, "Jackpot sem trinca: esperava 9, deu %d", d.total[0]);

    // Trinca sem Jackpot, nada.
    limpa();
    joga(0, D_D6, 4);
    joga(0, D_D6, 4);
    joga(0, D_D6, 4);
    d = fim();
    CONFERE(d.total[0] == 12, "Trinca sem Jackpot: esperava 12, deu %d", d.total[0]);

    // Egoista: anula os anteriores ao chegar e os que vierem depois; o Escudo resiste.
    limpa();
    joga(0, D_D6, 2);
    joga(0, D_ESCUDO, 3);
    joga(0, D_EGOISTA, 9);
    CONFERE(F[0].est[0] == V_ANULADO, "Egoista: devia anular o d6 anterior");
    CONFERE(F[0].est[1] == V_VALIDO, "Egoista: o Escudo devia resistir");
    CONFERE(F[0].est[2] == V_VALIDO, "Egoista: devia valer");
    joga(0, D_D12, 11);
    CONFERE(F[0].est[3] == V_ANULADO, "Egoista: devia anular o dado de depois");
    d = fim();
    CONFERE(d.total[0] == 3 + 9, "Egoista: esperava 12, deu %d", d.total[0]);

    // Egoista nao cai na queda da fila.
    limpa();
    joga(0, D_D12, 12);
    joga(0, D_EGOISTA, 1);
    CONFERE(F[0].est[1] == V_VALIDO, "Egoista 1 depois de 12: nao devia cair");
    CONFERE(F[0].est[0] == V_ANULADO, "Egoista 1 depois de 12: o 12 devia ser anulado por ele");

    // O Egoista e anulavel: o Carrasco com 4 derruba ele.
    limpa();
    joga(1, D_EGOISTA, 8);
    joga(0, D_CARRASCO, 4);
    d = fim();
    CONFERE(d.est[1][0] == V_ANULADO, "Carrasco devia anular o Egoista");
    CONFERE(d.total[1] == 0, "Egoista atacado: esperava 0, deu %d", d.total[1]);
    // E cai na queda da sequencia diante de um Teimoso maior (que fica).
    limpa();
    joga(0, D_TEIMOSO, 6);
    joga(0, D_EGOISTA, 3);
    CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_ANULADO, "Egoista 3 depois do Teimoso 6 devia cair sozinho");

    // E o risco depois dele e total (a IA usa isso para parar).
    limpa();
    joga(0, D_EGOISTA, 6);
    F[0].tipo[1] = D_D20; F[0].n = 2;
    CONFERE(risco(0) == 100, "risco depois do Egoista devia ser 100, deu %d", risco(0));

    // Moeda da sorte: 1 da +1 nos impares (ela inclusive), 2 da +1 nos pares.
    limpa();
    joga(0, D_SORTE, 1);
    joga(0, D_D6, 3);
    joga(0, D_D6, 4);
    d = fim();
    CONFERE(d.total[0] == (1 + 1) + (3 + 1) + 4, "Moeda 1: esperava 10, deu %d", d.total[0]);
    limpa();
    joga(0, D_SORTE, 2);
    joga(0, D_D6, 3);
    joga(0, D_D6, 4);
    d = fim();
    CONFERE(d.total[0] == (2 + 1) + 3 + (4 + 1), "Moeda 2: esperava 11, deu %d", d.total[0]);

    // Viciado: o veredito conta os dois lances.
    limpa();
    F[0].descarte[0] = 2;
    joga(0, D_VICIADO, 5);
    CONFERE(strstr(veredito, "5 e 2") != NULL, "Viciado: veredito devia citar os dois lances: %s", veredito);

    // ---- Apostas -------------------------------------------------------
    // Dobrar completo: A aposta 10, B cobre e sobe 10, A paga 10.
    aposta_zera(100, 100);
    executa(OP_APOSTAR10);
    executa(OP_AUMENTAR);
    CONFERE(a_pagar == 10, "dobrar completo: A devia pagar 10, paga %d", a_pagar);
    CONFERE(J[1].fichas == 80 && pote == 30, "dobrar completo: B %d, pote %d", J[1].fichas, pote);
    executa(OP_PAGAR);
    CONFERE(J[0].fichas == 80 && J[1].fichas == 80 && pote == 40,
            "dobrar completo: A %d B %d pote %d", J[0].fichas, J[1].fichas, pote);

    // Dobrar parcial: B tem 30 contra a aposta de 25; cobre 25 e sobe so 5.
    aposta_zera(100, 30);
    executa(OP_APOSTAR25);
    int op[4], n_op = opcoes(op);
    char rot[24] = "";
    for (int i = 0; i < n_op; i++) if (op[i] == OP_AUMENTAR) rotulo_op(op[i], rot, sizeof rot);
    CONFERE(!strcmp(rot, "subir 5"), "dobrar parcial: rotulo devia ser 'subir 5', foi '%s'", rot);
    executa(OP_AUMENTAR);
    CONFERE(J[1].fichas == 0, "dobrar parcial: B devia ficar com 0, ficou %d", J[1].fichas);
    CONFERE(a_pagar == 5, "dobrar parcial: A devia pagar 5, paga %d", a_pagar);
    executa(OP_PAGAR);
    CONFERE(J[0].fichas == 70 && pote == 60, "dobrar parcial: A %d pote %d", J[0].fichas, pote);

    // Limitado pelo rival: A aposta 25 e fica com 3; B so pode subir 3.
    aposta_zera(28, 100);
    executa(OP_APOSTAR25);
    executa(OP_AUMENTAR);
    CONFERE(a_pagar == 3 && J[1].fichas == 72, "limite do rival: a pagar %d, B %d", a_pagar, J[1].fichas);
    executa(OP_PAGAR);
    CONFERE(J[0].fichas == 0 && pote == 56, "limite do rival: A %d pote %d", J[0].fichas, pote);

    // Sem nada para subir alem de cobrir, nao ha opcao de dobrar.
    aposta_zera(100, 25);
    executa(OP_APOSTAR25);
    n_op = opcoes(op);
    bool tem_dobrar = false;
    for (int i = 0; i < n_op; i++) tem_dobrar |= op[i] == OP_AUMENTAR;
    CONFERE(!tem_dobrar, "B com fichas so para cobrir nao devia poder dobrar");

    // Nunca fica saldo negativo, e o pote e sempre o que saiu dos dois.
    for (int a = 0; a <= 60; a += 3)
        for (int b = 0; b <= 60; b += 4)
            for (int abre = 0; abre < 2; abre++) {
                aposta_zera(a, b);
                executa(abre ? OP_APOSTAR25 : OP_APOSTAR10);
                if (fase != F_APOSTA) continue;
                n_op = opcoes(op);
                for (int i = 0; i < n_op; i++) if (op[i] == OP_AUMENTAR) { executa(OP_AUMENTAR); break; }
                if (fase == F_APOSTA && a_pagar) executa(OP_PAGAR);
                CONFERE(J[0].fichas >= 0 && J[1].fichas >= 0, "saldo negativo com %d x %d", a, b);
                CONFERE(J[0].fichas + J[1].fichas + pote == a + b, "fichas sumiram com %d x %d", a, b);
                CONFERE(a - J[0].fichas == b - J[1].fichas, "contribuicoes diferentes com %d x %d: %d e %d",
                        a, b, a - J[0].fichas, b - J[1].fichas);
            }

    // ---- Entrada da rodada ----------------------------------------------
    entrada(3, 100, 0);
    CONFERE(fase == F_FIM && venc_partida == 1, "so A sem entrada: B devia vencer (%d)", venc_partida);
    entrada(100, 4, 0);
    CONFERE(fase == F_FIM && venc_partida == 0, "so B sem entrada: A devia vencer (%d)", venc_partida);
    entrada(4, 2, 194);
    CONFERE(fase == F_FIM && venc_partida == 0, "os dois sem entrada: vence quem tem mais (%d)", venc_partida);
    entrada(1, 3, 196);
    CONFERE(fase == F_FIM && venc_partida == 1, "os dois sem entrada: vence quem tem mais (%d)", venc_partida);
    entrada(0, 0, 200);
    CONFERE(fase == F_FIM && venc_partida == 2, "os dois zerados: devia empatar (%d)", venc_partida);

    // ---- Multiplicadores se somam -------------------------------------
    // Dobro antes: x2. Dobro e Fartura no mesmo dado de 2: x2 + x2 = x4.
    limpa();
    joga(0, D_DOBRO, 1);
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.total[0] == 1 + 10, "Dobro: esperava 11, deu %d", d.total[0]);
    limpa();
    joga(0, D_FARTURA, 1);
    joga(0, D_DOBRO, 1);
    joga(0, D_D4, 2);
    d = fim();
    // Fartura 1 (x2 = 2), Dobro 1 (x2 = 2), d4 2: Dobro + Fartura = x4 = 8.
    CONFERE(d.total[0] == 2 + 2 + 8, "Dobro + Fartura: esperava 12, deu %d", d.total[0]);
    // Com o Tudo ou Nada 2 junto: x2 + x2 + x2 = x6 (soma, nao x8).
    limpa();
    joga(0, D_TUDO_NADA, 2);
    joga(0, D_FARTURA, 2);
    joga(0, D_DOBRO, 2);
    joga(0, D_D4, 2);
    d = fim();
    // TN 2 (x2+x2 = 8), Fartura 2 (x2 tn + x2 far = 8), Dobro 2 (x2 x2 = 8), d4 2 (x6 = 12).
    CONFERE(d.total[0] == 8 + 8 + 8 + 12, "tres multiplicadores: esperava 36, deu %d", d.total[0]);

    // ---- Agouro: Teimoso e Escudo seguram, e a mensagem diz isso -------
    limpa();
    joga(0, D_TEIMOSO, 6);
    joga(0, D_MALDITO, 1);
    d = fim();
    CONFERE(d.total[0] == 6, "Agouro x Teimoso: o Teimoso devia ficar (6), deu %d", d.total[0]);
    CONFERE(strstr(veredito, "Teimoso") != NULL, "Agouro x Teimoso: veredito devia citar o Teimoso: %s", veredito);
    limpa();
    joga(0, D_D6, 5);
    joga(0, D_MALDITO, 3);
    CONFERE(strstr(veredito, "zerou a sequência") != NULL, "Agouro: veredito '%s'", veredito);

    // ---- O resultado pago e o do motor, e os eventos chegam nele --------
    for (int caso = 0; caso < 4000; caso++) {
        limpa();
        for (int j = 0; j < 2; j++) {
            int n = 1 + (int)(teste_rnd() % N_FILA);
            for (int k = 0; k < n; k++) {
                int t = (int)(teste_rnd() % D_N);
                int v = 1 + (int)(teste_rnd() % TIPO[t].lados);
                F[j].fixo[F[j].lancados] = (uint8_t)v;
                joga(j, t, v);
            }
        }
        J[0].fichas = J[1].fichas = 100; pote = 10;
        desfecho();
        for (int i = 0; i < n_ev; i++) aplica_evento(i);
        for (int j = 0; j < 2; j++)
            CONFERE(total_visivel(j) == resolvido.total[j],
                    "caso %d: eventos mostram %d, o motor paga %d (jogador %d)",
                    caso, total_visivel(j), resolvido.total[j], j);
        if (fase == F_EFEITOS) conclui_rodada();
        for (int j = 0; j < 2; j++)
            CONFERE(F[j].total == resolvido.total[j], "caso %d: pagou %d, motor %d", caso, F[j].total, resolvido.total[j]);
        if (falhas > 20) break;
    }

    // ---- Lastro: 4 a 12, cada um 1 em 9 ---------------------------------
    {
        int cont[13] = { 0 }, n = 90000, menor;
        semente = 0x1234567u;                    // o sorteio da partida, semeado
        for (int i = 0; i < n; i++) {
            int v = sorteia_valor(D_LASTRO, 0, rola, &menor);
            CONFERE(v >= 4 && v <= 12, "Lastro tirou %d", v);
            if (v >= 4 && v <= 12) cont[v]++;
        }
        for (int v = 4; v <= 12; v++)
            CONFERE(cont[v] > n / 9 * 0.94 && cont[v] < n / 9 * 1.06,
                    "Lastro: o %d saiu %d vezes em %d (esperado ~%d)", v, cont[v], n, n / 9);
        CONFERE(TIPO[D_LASTRO].raridade == RAR_RARO, "Lastro devia ser raro");
        // Risco com barra 9: 4, 5, 6, 7 e 8 caem = 5 em 9.
        limpa();
        joga(0, D_D12, 9);
        F[0].tipo[1] = D_LASTRO; F[0].n = 2;
        CONFERE(risco(0) == 500 / 9, "Lastro com barra 9: risco %d, esperado %d", risco(0), 500 / 9);
    }

    // ---- Dados novos: Sentinela, Vidro, Ferreiro, Cobrador, ... -----------
    {
        desfecho_t d;
        // Sentinela cai sozinha: o 6 de antes continua valendo.
        limpa();
        joga(0, D_D8, 6); joga(0, D_SENTINELA, 2);
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_ANULADO, "Sentinela: o de antes devia ficar");
        d = fim();
        CONFERE(d.total[0] == 6, "Sentinela: esperava 6, deu %d", d.total[0]);
        // Sentinela que passa a barra vale normal.
        limpa();
        joga(0, D_D6, 2); joga(0, D_SENTINELA, 3);
        d = fim();
        CONFERE(d.total[0] == 5, "Sentinela valendo: esperava 5, deu %d", d.total[0]);
        // Dado comum que cai sobre a Sentinela derruba os dois, como sempre.
        limpa();
        joga(0, D_SENTINELA, 4); joga(0, D_D4, 1);
        CONFERE(F[0].est[0] == V_ANULADO && F[0].est[1] == V_ANULADO, "queda sobre a Sentinela: os dois caem");

        // Vidro com 1: se estilhaca e derruba o de antes, como na queda.
        limpa();
        joga(0, D_D6, 5); joga(0, D_VIDRO, 1);
        CONFERE(F[0].est[0] == V_ANULADO && F[0].est[1] == V_ANULADO, "Vidro 1: devia anular ele e o anterior");
        joga(0, D_D8, 6);
        d = fim();
        CONFERE(d.total[0] == 6, "Vidro 1: esperava 6, deu %d", d.total[0]);
        // O Teimoso de antes segura o Vidro quebrado.
        limpa();
        joga(0, D_TEIMOSO, 5); joga(0, D_VIDRO, 1);
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_ANULADO, "Vidro 1 sobre o Teimoso: o Teimoso fica");
        // Vidro com outro numero e um D8 comum.
        limpa();
        joga(0, D_VIDRO, 6);
        d = fim();
        CONFERE(d.total[0] == 6, "Vidro 6: esperava 6, deu %d", d.total[0]);
        // O Vidro quebrado sai da colecao no fim da rodada.
        limpa();
        colecao_inicial(&J[0]); colecao_inicial(&J[1]);
        J[0].col[J[0].n_col] = nova_peca(D_VIDRO); J[0].onde[J[0].n_col++] = NA_MAO;
        int nc = J[0].n_col;
        joga(0, D_VIDRO, 1);
        F[0].idx[0] = (int8_t)(nc - 1);
        memset(vis_queima, 0, sizeof vis_queima);
        recolhe_quebrados();
        bool ainda = false;
        for (int i = 0; i < J[0].n_col; i++) if (J[0].col[i].tipo == D_VIDRO) ainda = true;
        CONFERE(J[0].n_col == nc - 1 && !ainda, "Vidro quebrado devia sair da bolsa (%d dados)", J[0].n_col);
        // A Ventania nao conserta o vidro quebrado.
        limpa();
        joga(0, D_VIDRO, 1);
        joga_com(0, D_VENTANIA, 9, sempre_dois);
        CONFERE(F[0].valor[0] == 1 && F[0].est[0] == V_ANULADO, "Ventania rolou o Vidro quebrado");

        // Ferreiro: nao pontua; +4 no dado seguinte se os dois valem.
        limpa();
        joga(0, D_FERREIRO, 3); joga(0, D_D6, 5);
        d = fim();
        CONFERE(d.total[0] == 9, "Ferreiro: esperava 0 + 5 + 4 = 9, deu %d", d.total[0]);
        // O seguinte caiu: nada de +4, e o Ferreiro sozinho vale 0.
        limpa();
        joga(0, D_FERREIRO, 3); joga(0, D_D6, 5); joga(0, D_D4, 1);
        d = fim();
        CONFERE(d.total[0] == 0, "Ferreiro sem o seguinte: esperava 0, deu %d", d.total[0]);
        // Ferreiro, Dobro e D6: o Dobro ganha +4 (3 + 4) e o D6 vale x2 (12): 19.
        limpa();
        joga(0, D_FERREIRO, 2); joga(0, D_DOBRO, 3); joga(0, D_D6, 6);
        d = fim();
        CONFERE(d.total[0] == 19, "Ferreiro + Dobro: esperava 19, deu %d", d.total[0]);

        // Cobrador: +2 se o oponente comecou a rodada com mais fichas.
        limpa();
        fichas_ini[0] = 50; fichas_ini[1] = 80;
        joga(0, D_COBRADOR, 3);
        d = fim();
        CONFERE(d.total[0] == 5, "Cobrador cobrando: esperava 5, deu %d", d.total[0]);
        limpa();
        fichas_ini[0] = 80; fichas_ini[1] = 50;
        joga(0, D_COBRADOR, 3);
        d = fim();
        CONFERE(d.total[0] == 3, "Cobrador sem cobrar: esperava 3, deu %d", d.total[0]);
        fichas_ini[0] = fichas_ini[1] = 0;

        // Misericordioso: +2 por dado seu anulado.
        limpa();
        joga(0, D_D8, 6); joga(0, D_D4, 2); joga(0, D_MISERICORDIOSO, 3);
        d = fim();
        CONFERE(d.total[0] == 3 + 4, "Misericordioso: esperava 7, deu %d", d.total[0]);
        // Anulado, nao da nada.
        limpa();
        joga(0, D_D8, 6); joga(0, D_D4, 2); joga(0, D_MISERICORDIOSO, 3); joga(0, D_D4, 1);
        d = fim();
        CONFERE(d.total[0] == 0, "Misericordioso anulado: esperava 0, deu %d", d.total[0]);

        // Solitario: +4 se nao ha dado valendo logo antes nem logo depois.
        limpa();
        joga(0, D_D6, 5); joga(0, D_D4, 1); joga(0, D_SOLITARIO, 2);
        d = fim();
        CONFERE(d.total[0] == 2 + 4, "Solitario sem vizinhos: esperava 6, deu %d", d.total[0]);
        limpa();
        joga(0, D_D6, 1); joga(0, D_SOLITARIO, 2);
        d = fim();
        CONFERE(d.total[0] == 3, "Solitario com vizinho: esperava 3, deu %d", d.total[0]);
        // Primeiro da fila; o segundo cai com o terceiro: sem vizinhos, +4.
        limpa();
        joga(0, D_SOLITARIO, 2); joga(0, D_D4, 3); joga(0, D_D6, 1);
        d = fim();
        CONFERE(d.total[0] == 2 + 4, "Solitario na ponta: esperava 6, deu %d", d.total[0]);
        // Um dado valendo mais longe (nao vizinho) nao atrapalha.
        limpa();
        joga(0, D_D4, 1); joga(0, D_D4, 3); joga(0, D_D4, 2); joga(0, D_SOLITARIO, 4);
        d = fim();
        CONFERE(d.total[0] == 1 + 4 + 4, "Solitario com dado longe: esperava 9, deu %d", d.total[0]);

        // Desafiante: +4 se for o maior dado da rodada (empate nao conta).
        limpa();
        joga(0, D_DESAFIANTE, 7); joga(1, D_D6, 5);
        d = fim();
        CONFERE(d.total[0] == 11, "Desafiante maior: esperava 11, deu %d", d.total[0]);
        limpa();
        joga(0, D_DESAFIANTE, 7); joga(1, D_D8, 7);
        d = fim();
        CONFERE(d.total[0] == 7, "Desafiante empatado: esperava 7, deu %d", d.total[0]);
        // Um dado maior, mas anulado, nao conta.
        limpa();
        joga(0, D_DESAFIANTE, 7); joga(1, D_D20, 15); joga(1, D_D4, 2);
        d = fim();
        CONFERE(d.total[0] == 11, "Desafiante com o maior anulado: esperava 11, deu %d", d.total[0]);

        // Acumulador: o bonus guardado vale junto.
        limpa();
        F[0].fixo[0] = 3;
        joga(0, D_ACUMULADOR, 5);
        d = fim();
        CONFERE(d.total[0] == 8, "Acumulador +3: esperava 8, deu %d", d.total[0]);
        limpa();
        F[0].fixo[0] = 9;                       // nunca passa de +6
        joga(0, D_ACUMULADOR, 5);
        d = fim();
        CONFERE(d.total[0] == 11, "Acumulador no teto: esperava 11, deu %d", d.total[0]);
    }

    // ---- Desafiante: salvamento de antes dos amuletos ainda carrega -------
    {
        int modo_antes = modo;
        des_nova_run();
        CONFERE(fase == F_RAMULETO && run.estado == R_AMULETO, "run nova devia comecar escolhendo amuleto");
        amuleto_escolhido(0);
        CONFERE(am_conta(run_amuletos()) == 1 && fase == F_RLOJA, "amuleto inicial: %x, fase %d", run_amuletos(), fase);
        run.moedas = 77;
        des_salva();
        run_t r;
        CONFERE(des_carrega(&r) && r.moedas == 77 && r.amuletos == run.amuletos, "salvamento novo nao volta");
        salva_apaga(chave_run());
        modo = modo_antes;
        // Fora do Desafiante, amuleto nenhum vale.
        amul[0] = 0xFFF;
        modo = M_UM;
        CONFERE(!tem_am(0, A_LUVA) && tam_mao(0) == 7 && max_fila(0) == N_FILA, "amuleto valendo fora do Desafiante");
        amul[0] = 0;
        modo = modo_antes;
    }

    // ---- Amuletos no motor (so com modo = Desafiante) ---------------------
    {
        int modo_antes = modo;
        desfecho_t d;
        modo = M_DESAFIO;
        // Luva de Couro: o primeiro nao cai.
        limpa(); amul[0] = 1 << A_LUVA; F[0].n = 2;
        joga(0, D_D6, 5); joga(0, D_D4, 2);
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_ANULADO, "Luva: o primeiro devia ficar");
        // Rede: o ultimo da sequencia cai sozinho.
        limpa(); amul[0] = 1 << A_REDE; F[0].n = 3;
        joga(0, D_D4, 2); joga(0, D_D6, 5); joga(0, D_D4, 1);
        CONFERE(F[0].est[1] == V_VALIDO && F[0].est[2] == V_ANULADO, "Rede: o de antes do ultimo devia ficar");
        // Sem amuleto, cai como sempre.
        limpa(); amul[0] = 0; F[0].n = 2;
        joga(0, D_D6, 5); joga(0, D_D4, 2);
        CONFERE(F[0].est[0] == V_ANULADO, "sem Luva, o primeiro cai");
        // Dado de Ouro: numero maximo +2. Peso Pena: 2 e 4 lados +1.
        limpa(); amul[0] = 1 << A_OURO;
        joga(0, D_D6, 6);
        d = fim();
        CONFERE(d.total[0] == 8, "Dado de Ouro: esperava 8, deu %d", d.total[0]);
        limpa(); amul[0] = 1 << A_PENA | 1 << A_OURO;
        joga(0, D_D4, 4);
        d = fim();
        CONFERE(d.total[0] == 4 + 2 + 1, "Ouro + Pena num D4 com 4: esperava 7, deu %d", d.total[0]);
        // O oponente nunca tem amuleto.
        limpa(); amul[0] = 1 << A_OURO;
        joga(1, D_D6, 6);
        d = fim();
        CONFERE(d.total[1] == 6, "amuleto valeu para o oponente: %d", d.total[1]);
        // Ima: o Caridoso do oponente nao rouba.
        limpa(); amul[0] = 1 << A_IMA; primeiro = 0;
        joga(0, D_D4, 2); joga(1, D_CARIDOSO, 3);
        d = fim();
        CONFERE(d.est[0][0] == V_VALIDO && d.total[0] == 2, "Ima: o dado foi roubado");
        // Couraca: o Carrasco do oponente nao anula.
        limpa(); amul[0] = 1 << A_COURACA;
        joga(0, D_D6, 5); joga(1, D_CARRASCO, 4);
        d = fim();
        CONFERE(d.est[0][0] == V_VALIDO && d.total[0] == 5, "Couraca: o dado foi anulado");
        // Contrato Sujo: sequencia de 3; Bolsa Funda e Saco Furado: tamanho da mao.
        amul[0] = 1 << A_CONTRATO;
        CONFERE(max_fila(0) == 3 && max_fila(1) == N_FILA, "Contrato: sequencia de %d", max_fila(0));
        amul[0] = 1 << A_BOLSA | 1 << A_FURADO;
        CONFERE(tam_mao(0) == 8 && tam_mao(1) == 6, "Bolsa/Saco: maos %d e %d", tam_mao(0), tam_mao(1));
        amul[0] = 0;
        modo = modo_antes;
    }

    // ---- Amuletos que as dificuldades abrem -------------------------------
    {
        int modo_antes = modo;
        desfecho_t d;
        modo = M_DESAFIO;
        // Ferradura: na primeira queda da rodada, o de antes fica; o novo cai.
        limpa(); amul[0] = 1u << A_FERRADURA; ferr_k[0] = -1; am_disparou = 0;
        joga(0, D_D6, 5); joga(0, D_D4, 2);
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_ANULADO && (am_disparou >> A_FERRADURA & 1),
                "Ferradura: o 5 devia ficar e o 2 cair");
        ferr_k[0] = 1;                                   // a primeira queda ja pousou
        joga(0, D_D6, 6); joga(0, D_D4, 1);
        CONFERE(F[0].est[2] == V_ANULADO && F[0].est[3] == V_ANULADO, "Ferradura: a segunda queda devia cair");
        ferr_k[0] = -1;
        // Dado Pesado: +1 nos de 6 lados.
        limpa(); amul[0] = 1u << A_PESADO;
        joga(0, D_D6, 4); joga(0, D_D8, 4);
        d = fim();
        CONFERE(d.total[0] == 5 + 4, "Dado Pesado: esperava 9, deu %d", d.total[0]);
        // Coroa do Azarao: +1 so com menos fichas no comeco da rodada.
        limpa(); amul[0] = 1u << A_COROA; fichas_ini[0] = 10; fichas_ini[1] = 20;
        joga(0, D_D6, 3);
        d = fim();
        CONFERE(d.total[0] == 4, "Coroa atras no placar: esperava 4, deu %d", d.total[0]);
        limpa(); fichas_ini[0] = 30;
        joga(0, D_D6, 3);
        d = fim();
        CONFERE(d.total[0] == 3, "Coroa na frente: esperava 3, deu %d", d.total[0]);
        fichas_ini[0] = fichas_ini[1] = 0;
        // Prensa: o 1 vale 3.
        limpa(); amul[0] = 1u << A_PRENSA;
        joga(0, D_D6, 1);
        d = fim();
        CONFERE(F[0].valor[0] == 3 && d.total[0] == 3, "Prensa: esperava 3, deu %d", d.total[0]);
        // Coringa: o primeiro da sequencia vale +2.
        limpa(); amul[0] = 1u << A_CORINGA;
        joga(0, D_D6, 4); joga(0, D_D8, 5);
        d = fim();
        CONFERE(d.total[0] == 6 + 5, "Coringa: esperava 11, deu %d", d.total[0]);
        // Contrato Sujo: +2 em cada dado que vale; a sequencia leva 3.
        limpa(); amul[0] = 1u << A_CONTRATO;
        joga(0, D_D6, 3); joga(0, D_D8, 5);
        d = fim();
        CONFERE(d.total[0] == 5 + 7 && max_fila(0) == 3, "Contrato: esperava 12 e fila de 3, deu %d", d.total[0]);
        // Dupla: +2 em cada um dos que repetem o numero.
        limpa(); amul[0] = 1u << A_DUPLA;
        joga(0, D_D6, 3); joga(0, D_D8, 3); joga(0, D_D8, 5);
        d = fim();
        CONFERE(d.total[0] == 3 + 3 + 5 + 4, "Dupla: esperava 15, deu %d", d.total[0]);
        // Relogio de Bolso: atras em fichas, o oponente abre a rodada.
        amul[0] = 1u << A_RELOGIO;
        for (int i = 0; i < 4; i++) { entrada(30, 40, 0); CONFERE(primeiro == 1, "Relogio: quem abre e %d", primeiro); }
        primeiro = 0; entrada(40, 40, 0);
        CONFERE(primeiro == 0, "Relogio sem estar atras: quem abre e %d", primeiro);
        // Fora do Desafiante, nenhum deles vale.
        modo = M_UM; amul[0] = 0xFFFFFFFFu;
        limpa(); joga(0, D_D6, 1); joga(0, D_D6, 4);
        d = fim();
        CONFERE(d.total[0] == 5, "amuleto novo valendo fora do Desafiante: %d", d.total[0]);
        amul[0] = 0;
        modo = modo_antes;
    }

    // ---- Dados que as dificuldades abrem ----------------------------------
    {
        desfecho_t d;
        // Bumerangue: tirou 1, rola de novo uma vez.
        limpa();
        joga_com(0, D_BUMERANGUE, 1, sempre_dois);
        d = fim();
        CONFERE(F[0].valor[0] == 2 && d.total[0] == 2, "Bumerangue: esperava 2, deu %d", d.total[0]);
        // Gemeo: tira o mesmo do anterior que vale; sozinho, fica com o seu.
        limpa();
        joga(0, D_D6, 4); joga(0, D_GEMEO, 2);
        d = fim();
        CONFERE(F[0].valor[1] == 4 && d.total[0] == 8, "Gemeo: esperava 8, deu %d", d.total[0]);
        limpa();
        joga(0, D_GEMEO, 5);
        d = fim();
        CONFERE(d.total[0] == 5, "Gemeo sozinho: esperava 5, deu %d", d.total[0]);
        // Prisma: +1 por tipo diferente valendo.
        limpa();
        joga(0, D_D6, 2); joga(0, D_D4, 3); joga(0, D_PRISMA, 5);
        d = fim();
        CONFERE(d.total[0] == 2 + 3 + 5 + 3, "Prisma: esperava 13, deu %d", d.total[0]);
        // Fenix: anulado pela queda, renasce no fim.
        limpa();
        joga(0, D_D6, 5); joga(0, D_FENIX, 2);
        d = fim();
        CONFERE(d.est[0][1] == V_VALIDO && d.est[0][0] == V_ANULADO && d.total[0] == 2,
                "Fenix: esperava so ela (2), deu %d", d.total[0]);
        // Trono: x2 se for o ultimo jogado.
        limpa();
        joga(0, D_D6, 3); joga(0, D_TRONO, 8);
        d = fim();
        CONFERE(d.total[0] == 3 + 16, "Trono por ultimo: esperava 19, deu %d", d.total[0]);
        limpa();
        joga(0, D_TRONO, 8); joga(0, D_D12, 9);
        d = fim();
        CONFERE(d.total[0] == 8 + 9, "Trono no meio: esperava 17, deu %d", d.total[0]);
        // Nenhum deles entra no sorteio das partidas basicas.
        for (int t = D_BASICOS; t < D_N; t++) CONFERE(peso_dado(t) == 0 && so_desafio(t), "dado %d na partida basica", t);
    }

    // ---- Perfis: gravar e ler; precos da dificuldade e do Cupom ----------
    {
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[1].nome, sizeof perfis[1].nome, "LILI");
        perfis[1].avatar = AV_NOMALISA; perfis[1].nivel = 3; perfis[1].venceu[2] = 4; perfis[1].runs = 300;
        perfil_atual = 1;
        perfis_salva();
        perfil_t copia[N_PERFIS];
        memcpy(copia, perfis, sizeof copia);
        memset(perfis, 0, sizeof perfis); perfil_atual = -1; perfis_lidos = false;
        perfis_le();
        CONFERE(!memcmp(copia, perfis, sizeof copia) && perfil_atual == 1, "perfis nao voltaram iguais");
        CONFERE(!strcmp(chave_run(), "desafio2"), "perfil 2 grava em %s", chave_run());
        CONFERE(dado_liberado(D_PRISMA) && !dado_liberado(D_FENIX) && amuleto_liberado(A_CUPOM)
                && !amuleto_liberado(A_PRENSA), "o que abre com o nivel 3");
        // Os fortes de antes: Explosivo, Egoista e Tudo ou Nada ja abriram no nivel 3; o
        // Agouro nao; D20 e Lastro so com o chefe de cada salao vencido.
        CONFERE(dado_liberado(D_EXPLOSIVO) && dado_liberado(D_EGOISTA) && dado_liberado(D_TUDO_NADA)
                && !dado_liberado(D_MALDITO) && !dado_liberado(D_D20) && !dado_liberado(D_LASTRO),
                "dados fortes no nivel 3");
        perfis[1].marcos = 1;
        CONFERE(dado_liberado(D_D20) && !dado_liberado(D_LASTRO), "D20 com o Crupie vencido");
        {
            float w[4] = { 0, 0, 1, 1 };             // so raros e lendarios
            for (int i = 0; i < 300; i++) {
                int t = sorteia_raridade(w);
                CONFERE(dado_liberado(t), "sorteou dado fechado: %s", TIPO[t].nome);
            }
        }
        // Salvamento estragado: nenhum perfil, sem quebrar.
        salva_grava("perfis", "PER3zz");
        perfis_lidos = false; perfis_le();
        CONFERE(perfil_atual == -1 && !perfis[0].nome[0] && perfil()->nivel == 0, "perfis estragados");
        memset(&run, 0, sizeof run);
        run.dificuldade = 2;
        CONFERE(preco_run(D_D8) == 31 && preco_am() == 87, "Prata: precos %d e %d", preco_run(D_D8), preco_am());
        run_poe_amuleto(A_CUPOM);
        CONFERE(run_amuletos() >> A_CUPOM & 1 && run.amuletos_hi, "amuleto 16+ nao entrou na parte alta");
        CONFERE(preco_run(D_D8) == 23, "Prata com Cupom: %d", preco_run(D_D8));
        memset(&run, 0, sizeof run);
        salva_limpa_tudo();
        perfil_atual = -1;
    }

    // ---- Selo de Divida e Pedagio -----------------------------------------
    {
        int modo_antes = modo;
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[0].nome, sizeof perfis[0].nome, "T");
        perfil_atual = 0; perfis_lidos = true;
        des_dif = 0;
        des_nova_run();
        amuleto_escolhido(0);
        run.passo = DES_PASSOS - 1; run.caminho[run.passo] = 1; run.tier = 3;
        // Pedagio: o chefe comeca com 10 fichas a menos.
        des_comeca_partida();
        int sem = J[1].fichas;
        CONFERE(perfis[0].partidas == 0, "partida contada antes de acabar");
        run_poe_amuleto(A_PEDAGIO);
        des_comeca_partida();
        CONFERE(J[1].fichas == sem - 10, "Pedagio: %d, sem ele %d", J[1].fichas, sem);
        venc_partida = 0;
        des_resultado();
        CONFERE(perfis[0].partidas == 1, "partidas do perfil: %d", perfis[0].partidas);
        CONFERE((perfis[0].marcos & 1) && des_marco == D_D20 && dado_liberado(D_D20) && !dado_liberado(D_LASTRO),
                "vencer o Crupie devia abrir o D20 (marcos %d)", perfis[0].marcos);
        // Selo de Divida: o chefe da dois premios, depois o amuleto.
        run_poe_amuleto(A_SELO);
        run.tier = 3;
        abre_premio_run();
        des_premio_feito();
        CONFERE(fase == F_PREMIO && run.premio2 == 1 && run.estado == R_PREMIO, "Selo: sem segundo premio (fase %d)", fase);
        des_premio_feito();
        CONFERE(run.premio2 == 0 && fase == F_RAMULETO, "Selo: depois do segundo, o amuleto (fase %d)", fase);
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        perfil_atual = -1;
        modo = modo_antes;
    }

    // ---- Eventos do mapa ----------------------------------------------------
    {
        int modo_antes = modo;
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[0].nome, sizeof perfis[0].nome, "T");
        perfis[0].nivel = 5;
        perfil_atual = 0; perfis_lidos = true;
        des_dif = 0;
        des_nova_run();
        amuleto_escolhido(0);
        #define POE_EVENTO(e) do { run.passo = 1; run.caminho[1] = 0; run.no[1][0].tipo = NO_EVENTO; \
            run.no[1][0].perso = (uint8_t)(e); run.no[1][0].semente = 777u; entra_evento(); } while (0)
        // Lapidador: D6 vira D8 por 100 moedas; depois nao aparece mais.
        run.moedas = 150;
        POE_EVENTO(EVT_LAPIDADOR);
        CONFERE(fase == F_REVENTO && run.estado == R_EVENTO, "evento nao abriu (fase %d)", fase);
        run_t r;
        CONFERE(des_carrega(&r) && r.estado == R_EVENTO, "evento nao ficou salvo");
        ev_escolhe(EA_LAPIDA);
        CONFERE(ev_passo == 1 && acha_na_lista(D_D6) >= 0 && acha_na_lista(D_D12) < 0, "Lapidador: lista errada");
        int i6 = ev_lista[acha_na_lista(D_D6)];
        ev_aplica_dado(i6);
        CONFERE(run.col[i6].tipo == D_D8 && run.moedas == 50 && run.lapidou && run.passo == 2 && run.estado == R_MAPA,
                "Lapidador: tipo %d moedas %d", run.col[i6].tipo, run.moedas);
        for (int a = 0; a < DES_ANDARES; a++) {
            run.andar = (uint8_t)a; run.semente += 77;
            gera_andar();
            for (int p2 = 0; p2 < DES_PASSOS; p2++)
                for (int l = 0; l < 3; l++)
                    CONFERE(!(run.no[p2][l].tipo == NO_EVENTO && run.no[p2][l].perso == EVT_LAPIDADOR),
                            "Lapidador apareceu de novo");
        }
        run.andar = 0;
        // Capela: tira 2, nunca abaixo do minimo.
        int antes = run.n_col;
        POE_EVENTO(EVT_CAPELA);
        ev_escolhe(EA_CAPELA);
        ev_aplica_dado(ev_lista[0]);
        CONFERE(ev_passo == 1 && run.n_col == antes - 1 && run.tirados == 1, "Capela: primeira");
        des_continua();                                      // sair e voltar no meio: continua
        CONFERE(fase == F_REVENTO && ev_id == EVT_CAPELA && run.tirados == 1, "Capela: voltar no meio");
        ev_escolhe(EA_CAPELA);
        ev_aplica_dado(ev_lista[0]);
        CONFERE(ev_passo == 2 && run.n_col == antes - 2, "Capela: segunda");
        // Copista: nao copia lendario; copia por 70.
        run.col[run.n_col++] = (peca_t){ D_EGOISTA, 0 };
        run.moedas = 100;
        POE_EVENTO(EVT_COPISTA);
        ev_escolhe(EA_COPIA);
        for (int k = 0; k < ev_n; k++) CONFERE(TIPO[run.col[ev_lista[k]].tipo].raridade < RAR_LENDA, "Copista: lendario na lista");
        antes = run.n_col;
        int tc = run.col[ev_lista[0]].tipo;
        ev_aplica_dado(ev_lista[0]);
        CONFERE(run.n_col == antes + 1 && run.col[run.n_col - 1].tipo == tc && run.moedas == 30, "Copista");
        // Trocador: sobe uma raridade.
        POE_EVENTO(EVT_TROCADOR);
        ev_escolhe(EA_TROCA);
        int it = ev_lista[acha_na_lista(D_D4)];
        ev_aplica_dado(it);
        CONFERE(TIPO[run.col[it].tipo].raridade == RAR_INCOMUM, "Trocador: virou raridade %d", TIPO[run.col[it].tipo].raridade);
        // Dado Esquecido: arriscar da raro ou Quebrado.
        for (int k = 0; k < 6; k++) {
            POE_EVENTO(EVT_ESQUECIDO);
            run.no[1][0].semente = 1000u + (uint32_t)k; ev_sem = run.no[1][0].semente;
            antes = run.n_col;
            ev_escolhe(EA_ARRISCA);
            int t2 = run.col[run.n_col - 1].tipo;
            CONFERE(run.n_col == antes + 1 && (t2 == D_QUEBRADO || TIPO[t2].raridade == RAR_RARO), "Esquecido: %d", t2);
            run.n_col--;
        }
        // Fonte: 100 moedas; o amuleto sai ou nao, e o passo anda.
        run.moedas = 100;
        int am = am_conta(run_amuletos());
        POE_EVENTO(EVT_FONTE);
        ev_escolhe(EA_FONTE2);
        CONFERE(run.moedas == 0 && ev_passo == 2 && am_conta(run_amuletos()) - am == (ev_mostra == 2), "Fonte");
        // Caca-niquel: 15 por puxada ate dar tres iguais; o mesmo sorteio na volta.
        int puxadas[2];
        for (int vez2 = 0; vez2 < 2; vez2++) {
            run.moedas = 15 * 60;
            POE_EVENTO(EVT_NIQUEL);
            while (ev_passo != 2 && run.puxadas < 60) { ev_escolhe(EA_PUXA); t_fase += 2; ev_atualiza(); }
            puxadas[vez2] = run.puxadas;
            int premio = ev_rolos[0] == RL_MOEDAS ? EVT_PREMIO_NIQUEL : 0;
            CONFERE(ev_passo == 2 && ev_rolos[0] == ev_rolos[1] && ev_rolos[1] == ev_rolos[2]
                    && run.moedas == 15 * 60 - 15 * run.puxadas + premio, "Caca-niquel: moedas %d", run.moedas);
        }
        CONFERE(puxadas[0] == puxadas[1], "Caca-niquel: sorteio mudou (%d, %d)", puxadas[0], puxadas[1]);
        // Garcom: o proximo oponente com 10 fichas a menos, uma vez so.
        run.moedas = 50;
        POE_EVENTO(EVT_GARCOM);
        ev_escolhe(EA_GARCOM);
        CONFERE(run.garcom && run.moedas == 0, "Garcom: pagamento");
        run.no[2][0].tipo = NO_FACIL; run.no[2][0].perso = 1; run.no[2][0].semente = 99;
        run.passo = 2; run.caminho[2] = 0;
        des_comeca_partida();
        int com = J[1].fichas;
        venc_partida = 0;
        des_resultado();
        CONFERE(!run.garcom, "Garcom: devia valer uma partida so");
        run.passo = 2; run.caminho[2] = 0;
        des_comeca_partida();
        CONFERE(J[1].fichas == com + 10, "Garcom: %d com, %d sem", com, J[1].fichas);
        #undef POE_EVENTO
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        perfil_atual = -1;
        modo = modo_antes;
    }

    // ---- Trunfos proprios dos oponentes do Desafiante ----------------------
    {
        int modo_antes = modo;
        modo = M_DESAFIO;
        for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
        // O Crupie leva o empate.
        limpa(); reg_op = REG_CASA;
        memset(&resolvido, 0, sizeof resolvido);
        resolvido.total[0] = resolvido.total[1] = 7;
        J[0].fichas = J[1].fichas = 40; pote = 10;
        conclui_rodada();
        CONFERE(venc_mao == 1 && J[1].fichas == 50 && pote == 0, "Crupie: empate devia ser dele (%d, %d)", venc_mao, J[1].fichas);
        // O Ladrao, vencendo, leva mais 2 fichas suas.
        limpa(); reg_op = REG_BATEDOR;
        memset(&resolvido, 0, sizeof resolvido);
        resolvido.total[0] = 3; resolvido.total[1] = 9;
        J[0].fichas = J[1].fichas = 40; pote = 10;
        conclui_rodada();
        CONFERE(J[0].fichas == 38 && J[1].fichas == 52, "Ladrao: fichas %d x %d", J[0].fichas, J[1].fichas);
        // O Novato, depois de perder uma rodada, compra 6.
        limpa(); reg_op = REG_NERVOSO;
        memset(&resolvido, 0, sizeof resolvido);
        resolvido.total[0] = 9; resolvido.total[1] = 3;
        J[0].fichas = J[1].fichas = 40; pote = 10;
        conclui_rodada();
        CONFERE(op_nervoso && tam_mao(1) == 6 && tam_mao(0) == 7, "Novato: maos %d e %d", tam_mao(0), tam_mao(1));
        // A Detetive joga sempre por ultimo (o Relogio de Bolso do jogador vale mais).
        limpa(); reg_op = REG_ULTIMA; amul[0] = 0;
        for (int i = 0; i < 4; i++) { primeiro = 1; entrada(40, 30, 0); CONFERE(primeiro == 0, "Detetive atras: quem abre e %d", primeiro); }
        primeiro = 1; entrada(30, 40, 0);
        CONFERE(primeiro == 1, "Detetive na frente nao muda quem abre: %d", primeiro);
        amul[0] = 1u << A_RELOGIO;
        entrada(30, 40, 0);
        CONFERE(primeiro == 1, "Detetive com Relogio: quem abre e %d", primeiro);
        amul[0] = 0;
        // As viradas dos chefes, da 5a rodada em diante, pela dificuldade.
        {
            desfecho_t d;
            int modo_v = modo, perso_v = fala_perso;
            modo = M_DESAFIO; reg_op = REG_NADA;
            // Crupie no 1o andar: +1 em cada dado dele (antes da 5a, nada).
            fala_perso = PERSO_CHEFE0; run.dificuldade = 1;
            limpa(); rodada = 4; joga(1, D_D6, 3); d = fim();
            CONFERE(d.total[1] == 3, "Crupie antes da virada: %d", d.total[1]);
            limpa(); rodada = 5; joga(1, D_D6, 3); joga(1, D_D6, 4); d = fim();
            CONFERE(d.total[1] == 4 + 5, "Crupie virado: esperava 9, deu %d", d.total[1]);
            CONFERE(tam_mao(0) == 7, "Crupie no 1o andar nao tira dado seu: %d", tam_mao(0));
            run.dificuldade = 4;
            CONFERE(tam_mao(0) == 6, "Crupie no 4o andar: voce compra 6, comprou %d", tam_mao(0));
            // Barao no 3o andar: +3 por dado seu anulado e x2 no primeiro dele.
            fala_perso = PERSO_CHEFE0 + 2; run.dificuldade = 3;
            limpa(); rodada = 5;
            joga(1, D_D6, 2); joga(1, D_D6, 5);
            joga(0, D_D6, 5); joga(0, D_D6, 2);          // cai: os dois anulados
            d = fim();
            CONFERE(d.total[1] == 4 + 5 + 6, "Barao virado: esperava 15, deu %d", d.total[1]);
            // Na Cobertura, x2 tambem no segundo (o terceiro fica como saiu).
            run.dificuldade = 5;
            limpa(); rodada = 5;
            joga(1, D_D6, 2); joga(1, D_D6, 5); joga(1, D_D6, 6);
            d = fim();
            CONFERE(d.total[1] == 4 + 10 + 6, "Barao na Cobertura: esperava 20, deu %d", d.total[1]);
            // No 2o andar o Barao ainda nao vira.
            run.dificuldade = 2;
            limpa(); rodada = 5; joga(1, D_D6, 2); d = fim();
            CONFERE(d.total[1] == 2, "Barao sem virada no 2o andar: %d", d.total[1]);
            run.dificuldade = 0; fala_perso = perso_v; modo = modo_v; rodada = 1;
        }
        // A Maratonista: +1 nos dados dela da 5a rodada em diante.
        {
            desfecho_t d;
            reg_op = REG_FOLEGO;
            limpa(); rodada = 4;
            joga(1, D_D6, 3);
            d = fim();
            CONFERE(d.total[1] == 3, "Maratonista na 4a rodada: %d", d.total[1]);
            limpa(); rodada = 5;
            joga(1, D_D6, 3); joga(0, D_D6, 3);
            d = fim();
            CONFERE(d.total[1] == 4 && d.total[0] == 3, "Maratonista na 5a rodada: %d x %d", d.total[1], d.total[0]);
            // O Caubói: +1 no primeiro da sequência dele.
            reg_op = REG_SAQUE; rodada = 1;
            limpa();
            joga(1, D_D6, 4); joga(1, D_D8, 5);
            d = fim();
            CONFERE(d.total[1] == 4 + 1 + 5, "Caubói: esperava 10, deu %d", d.total[1]);
            // O Ilusionista (Dupla) vale para o oponente.
            reg_op = REG_NADA; amul[1] = 1u << A_DUPLA;
            limpa();
            joga(1, D_D6, 3); joga(1, D_D8, 3);
            d = fim();
            CONFERE(d.total[1] == 3 + 3 + 4, "Ilusionista: esperava 10, deu %d", d.total[1]);
            amul[1] = 0;
        }
        // Fora do Desafiante, nada disso vale.
        modo = M_UM;
        reg_op = REG_ULTIMA;
        CONFERE(!tem_reg(REG_ULTIMA), "Detetive valendo fora do Desafiante");
        reg_op = REG_NERVOSO;
        CONFERE(tam_mao(1) == 7 && !tem_reg(REG_NERVOSO), "trunfo valendo fora do Desafiante");
        reg_op = REG_NADA; op_nervoso = false;
        modo = modo_antes;
    }

    // ---- Seguranca: leva no lugar a anulacao ou o roubo do oponente --------
    {
        desfecho_t d;
        limpa();
        joga(0, D_SEGURANCA, 2); joga(0, D_D8, 7);
        joga(1, D_CARRASCO, 4);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.est[0][1] == V_VALIDO && d.total[0] == 7,
                "Seguranca x Carrasco: esperava so o 7 (deu %d)", d.total[0]);
        limpa();
        joga(0, D_D4, 1); joga(0, D_SEGURANCA, 3);
        joga(1, D_CARIDOSO, 3);
        d = fim();
        CONFERE(d.est[0][0] == V_VALIDO && d.est[0][1] == V_ROUBADO, "Seguranca x Caridoso: devia ir o Seguranca");
        // Dois ataques: o primeiro leva o Seguranca, o segundo pega o alvo de sempre.
        limpa();
        joga(0, D_SEGURANCA, 2); joga(0, D_D8, 5); joga(0, D_D12, 9);
        joga(1, D_CARRASCO, 4); joga(1, D_INVEJOSO, 6);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.est[0][2] == V_ANULADO && d.est[0][1] == V_VALIDO,
                "Seguranca com dois ataques");
        // Sem Seguranca, o Carrasco anula o maior, como sempre.
        limpa();
        joga(0, D_D4, 2); joga(0, D_D8, 7);
        joga(1, D_CARRASCO, 4);
        d = fim();
        CONFERE(d.est[0][1] == V_ANULADO && d.total[0] == 2, "Carrasco sem Seguranca");
    }

    // ---- Previa: o total com os bonus pode ser negativo ------------------
    limpa();
    primeiro = 1;
    joga(1, D_MARTELO, 4);              // Espinhoso do rival: -4
    joga(0, D_D4, 1);
    {
        desfecho_t pv;
        calcula_desfecho(&pv, false);
        CONFERE(pv.bruto[0] == -3 && pv.total[0] == 0, "previa negativa: bruto %d total %d", pv.bruto[0], pv.total[0]);
    }

    // ---- Dobro: so o dado logo depois dele; anulado, o x2 se perde -----
    limpa();
    joga(0, D_DOBRO, 3);
    joga(0, D_D8, 7);
    joga(0, D_D6, 5);                  // 5 < 7: anula o 7 e o 5
    joga(0, D_D12, 9);                 // o 9 nao herda o x2
    d = fim();
    CONFERE(d.total[0] == 3 + 9, "Dobro com o seguinte anulado: esperava 12, deu %d", d.total[0]);
    CONFERE(!F[0].dobra[3], "Dobro: o 9 nao devia ganhar a etiqueta x2");
    limpa();
    joga(0, D_DOBRO, 2);
    joga(0, D_D8, 6);
    d = fim();
    CONFERE(d.total[0] == 2 + 12, "Dobro com o seguinte valendo: esperava 14, deu %d", d.total[0]);

    // ---- Dobro so dobra se tirar de 1 a 3 --------------------------------
    limpa();
    joga(0, D_DOBRO, 5);
    joga(0, D_D8, 6);
    d = fim();
    CONFERE(d.total[0] == 5 + 6, "Dobro com 5: nao devia dobrar (11), deu %d", d.total[0]);

    // ---- Invejoso: anula o maior do oponente e depois o proprio maior ----
    limpa();
    primeiro = 1;
    joga(1, D_INVEJOSO, 4);            // sozinho: ele e o maior dele
    joga(0, D_D8, 7);
    joga(0, D_D8, 8);
    d = fim();
    CONFERE(d.est[0][1] == V_ANULADO, "Invejoso devia anular o 8 do oponente");
    CONFERE(d.est[1][0] == V_ANULADO, "Invejoso sozinho devia anular a si mesmo");
    CONFERE(d.total[1] == 0 && d.total[0] == 7, "Invejoso sozinho: %d x %d", d.total[1], d.total[0]);
    limpa();
    joga(1, D_INVEJOSO, 4);
    joga(1, D_D8, 7);                  // o maior dele e o 7: o Invejoso fica
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.est[1][1] == V_ANULADO && d.est[1][0] == V_VALIDO, "Invejoso devia anular o 7, e nao a si");
    limpa();
    joga(1, D_INVEJOSO, 3);            // com 3 nao age
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.est[0][0] == V_VALIDO && d.est[1][0] == V_VALIDO, "Invejoso com 3 nao devia anular nada");

    // ---- Carrasco com 1: anula o menor da propria sequencia ---------------
    limpa();
    joga(0, D_D6, 3); joga(0, D_D6, 5);
    joga(0, D_CARRASCO, 1);            // cai (1 < 5) e ainda anula o menor que sobrou
    joga(1, D_D6, 4);
    d = fim();
    CONFERE(d.est[0][0] == V_ANULADO && d.est[1][0] == V_VALIDO, "Carrasco com 1 devia anular o seu 3");
    limpa();
    joga(0, D_CARRASCO, 1); joga(0, D_D6, 2); joga(0, D_D6, 6);
    d = fim();
    CONFERE(d.est[0][1] == V_ANULADO && d.est[0][2] == V_VALIDO && d.est[0][0] == V_VALIDO,
            "Carrasco com 1 valendo devia anular o 2 (fora ele)");

    // ---- Fichas: comecou a rodada na frente, +2 ---------------------------
    limpa(); fichas_ini[0] = 40; fichas_ini[1] = 30;
    joga(0, D_FICHAS, 3);
    d = fim();
    CONFERE(d.total[0] == 5, "Fichas na frente: esperava 5, deu %d", d.total[0]);
    limpa(); fichas_ini[0] = 30; fichas_ini[1] = 40;
    joga(0, D_FICHAS, 3);
    d = fim();
    CONFERE(d.total[0] == 3, "Fichas atras: esperava 3, deu %d", d.total[0]);
    fichas_ini[0] = fichas_ini[1] = 0;

    // ---- Ficha de Fogo: +10 e o maior queima ------------------------------
    limpa();
    joga(0, D_FOGO, 2); joga(0, D_D6, 2); joga(0, D_D8, 6);
    d = fim();
    CONFERE(d.total[0] == 2 + 2 + 6 + 10 && d.queima[0][2], "Fogo: esperava 20 e o 6 queimando, deu %d", d.total[0]);

    // ---- Laser: com 6, anula um dado anulavel do oponente -----------------
    limpa();
    joga(1, D_D6, 2);
    joga(1, D_TEIMOSO, 4);
    joga(1, D_D8, 7);
    joga(0, D_LASER, 6);
    d = fim();
    {
        int anul = 0;
        for (int i = 0; i < 3; i++) anul += d.est[1][i] == V_ANULADO;
        CONFERE(anul == 1 && d.est[1][1] == V_VALIDO, "Laser devia anular um dado (e nao o Teimoso)");
        desfecho_t d2;
        calcula_desfecho(&d2, false);
        CONFERE(!memcmp(d.est, d2.est, sizeof d.est), "Laser: previa e desfecho deviam escolher o mesmo alvo");
    }
    limpa();
    joga(1, D_D8, 7);
    joga(0, D_LASER, 4);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Laser com 4 nao devia anular");
    limpa();
    joga(1, D_D8, 7);
    joga(0, D_LASER, 5);
    d = fim();
    CONFERE(d.est[1][0] == V_ANULADO, "Laser com 5 devia anular");
    limpa();
    joga(1, D_D8, 7);
    joga(0, D_D6, 6);
    joga(0, D_LASER, 5);               // caiu: anulado, nao atira
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Laser anulado nao devia atirar");

    // ---- Ventania: rola de novo os de antes e refaz a sequencia -------------
    limpa();
    joga(0, D_D6, 5);
    joga(0, D_D4, 2);                  // 2 < 5: caem os dois
    joga(0, D_TUDO_NADA, 1);           // rodada zerada
    CONFERE(F[0].zerada, "Tudo ou Nada 1 devia zerar");
    joga_com(0, D_VENTANIA, 9, sempre_dois);
    CONFERE(F[0].valor[0] == 2 && F[0].valor[1] == 2 && F[0].valor[2] == 2, "Ventania devia rolar de novo os 3");
    CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO, "Ventania: 2 e 2 deviam voltar a valer");
    CONFERE(!F[0].zerada, "Ventania: o Tudo ou Nada 2 nao zera mais");
    CONFERE(F[0].valor[3] == 9 && F[0].est[3] == V_VALIDO, "Ventania nao rola a si mesmo");
    d = fim();
    CONFERE(d.total[0] == (2 + 2 + 2 + 9) * 2, "Ventania + Tudo ou Nada 2: esperava 30, deu %d", d.total[0]);
    // Refeita, a sequencia pode cair: 6 depois de... o Quebrado nao muda.
    limpa();
    F[0].fixo[0] = 8;
    joga(0, D_QUEBRADO, 8);
    joga_com(0, D_VENTANIA, 3, sempre_dois);    // 3 < 8: cai junto com o Quebrado
    CONFERE(F[0].valor[0] == 8 && F[0].est[0] == V_ANULADO && F[0].est[1] == V_ANULADO,
            "Ventania: Quebrado nao muda; 3 depois de 8 cai");

    // ---- Nao anulaveis (Escudo, Teimoso, Egoista) e intocavel (Escudo) --
    limpa();
    joga(0, D_TEIMOSO, 3);
    joga(0, D_EGOISTA, 7);
    CONFERE(F[0].est[0] == V_VALIDO, "Egoista nao devia anular o Teimoso");
    limpa();
    joga(1, D_TEIMOSO, 6);
    joga(1, D_D4, 2);                  // cai: o Teimoso segura, o 2 cai
    joga(0, D_CARRASCO, 4);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Carrasco nao devia anular o Teimoso");
    limpa();
    joga(1, D_EGOISTA, 9);
    joga(0, D_INVEJOSO, 4);
    d = fim();
    CONFERE(d.est[1][0] == V_ANULADO, "Invejoso devia anular o Egoista (agora anulavel)");
    // Roubar pode: o Egoista e o Teimoso sao roubaveis; o Escudo, nao.
    limpa();
    joga(1, D_EGOISTA, 5);
    joga(0, D_CARIDOSO, 3);
    d = fim();
    CONFERE(d.est[1][0] == V_ROUBADO, "Caridoso devia roubar o Egoista");
    limpa();
    joga(1, D_TEIMOSO, 5);
    joga(0, D_CARIDOSO, 3);
    d = fim();
    CONFERE(d.est[1][0] == V_ROUBADO, "Caridoso devia roubar o Teimoso");
    limpa();
    joga(1, D_TEIMOSO, 5);
    joga(0, D_CARIDOSO, 1);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Caridoso com 1 nao devia roubar");
    limpa();
    joga(1, D_ESCUDO, 5);
    joga(0, D_CARIDOSO, 2);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Escudo e intocavel: nao devia ser roubado");

    // ---- Gatuno: so rouba se terminar valendo ---------------------------
    limpa();
    joga(1, D_D6, 4);
    joga(0, D_D2, 1);
    joga(0, D_GATUNO, 3);
    d = fim();
    CONFERE(d.est[1][0] == V_ROUBADO, "Gatuno valendo com um 1 seu devia roubar");
    limpa();
    joga(1, D_D6, 4);
    joga(0, D_D8, 5);
    joga(0, D_GATUNO, 1);              // 1 < 5: cai junto com o 5
    CONFERE(F[0].est[1] == V_ANULADO, "Gatuno 1 depois de 5 devia cair");
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Gatuno anulado nao devia roubar");

    // ---- Espelho: vira sempre o maior do rival ---------------------------
    limpa();
    primeiro = 1;
    joga(1, D_D8, 7);
    joga(1, D_D4, 1);                  // cai: sobra... nada, os dois caem
    joga(1, D_D12, 9);
    joga(0, D_ESPELHO, 2);
    CONFERE(F[0].valor[0] == 9, "Espelho do segundo devia virar o 9 do rival, virou %d", F[0].valor[0]);
    limpa();
    primeiro = 1;
    joga(1, D_D2, 1);
    joga(0, D_ESPELHO, 2);
    CONFERE(F[0].valor[0] == 1, "Espelho vira sempre o maior do rival, ate menor: devia ser 1, e %d", F[0].valor[0]);
    // Quem abre: o Espelho espera, nao cai, e vira no fim.
    limpa();
    primeiro = 0;
    joga(0, D_D6, 5);
    joga(0, D_ESPELHO, 1);
    CONFERE(F[0].est[1] == V_VALIDO && F[0].espera[1], "Espelho de quem abre devia esperar valendo");
    joga(1, D_D6, 3);
    joga(1, D_D12, 11);
    {
        desfecho_t pv;
        calcula_desfecho(&pv, false);
        CONFERE(pv.total[0] == 5 + 11, "previa com Espelho esperando: esperava 16, deu %d", pv.total[0]);
        CONFERE(F[0].valor[1] == 1 && F[0].espera[1], "a previa nao devia mexer no Espelho");
    }
    d = fim();
    CONFERE(d.total[0] == 16 && F[0].valor[1] == 11 && !F[0].espera[1],
            "Espelho no fim: total %d, valor %d", d.total[0], F[0].valor[1]);
    bool viu_esp = false;
    for (int i = 0; i < n_ev; i++) viu_esp |= ev[i].tipo == EV_ESPELHO;
    CONFERE(viu_esp, "Espelho no fim: faltou o evento");

    // ---- Espelho esperando: so no fim, ja virado, a queda vale -----------
    // 2 -> 3 -> Espelho 1 (espera). Se virar 2, cai com o 3; se virar 5, fica.
    for (int caso = 0; caso < 2; caso++) {
        limpa();
        primeiro = 0;
        joga(0, D_D4, 2);
        joga(0, D_D4, 3);
        joga(0, D_ESPELHO, 1);
        CONFERE(F[0].est[1] == V_VALIDO && F[0].est[2] == V_VALIDO, "Espelho esperando nao anula nem e anulado");
        joga(1, D_D8, caso == 0 ? 2 : 5);
        desfecho_t pv;
        calcula_desfecho(&pv, false);
        CONFERE(F[0].espera[2] && F[0].valor[2] == 1, "a previa nao devia mexer na mesa");
        d = fim();
        if (caso == 0)
            CONFERE(d.est[0][1] == V_ANULADO && d.est[0][2] == V_ANULADO && d.total[0] == 2,
                    "Espelho vira 2: 3 e Espelho caem, fica 2 (deu %d)", d.total[0]);
        else
            CONFERE(d.est[0][1] == V_VALIDO && d.est[0][2] == V_VALIDO && d.total[0] == 10,
                    "Espelho vira 5: nada cai, 2+3+5 (deu %d)", d.total[0]);
        CONFERE(pv.total[0] == d.total[0], "Espelho: previa %d, desfecho %d", pv.total[0], d.total[0]);
    }

    // ---- Premio: quem ganha no correr tambem escolhe; no fim, nao ha ------
    for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
    J[0].fichas = J[1].fichas = 80;
    rodada = 3;
    aposta_zera(80, 80);
    executa(OP_APOSTAR10);
    executa(OP_CORRER);                // B corre: A leva o pote
    segue_do_resultado();
    CONFERE(fase == F_PREMIO && vez == 0, "quem leva no correr devia escolher premio (fase %d vez %d)", fase, vez);
    rodada = MAX_RODADAS;
    venc_mao = 1;
    J[0].fichas = J[1].fichas = 80;
    fase = F_RESULTADO;
    segue_do_resultado();
    CONFERE(fase == F_FIM, "ultima rodada: sem premio, a partida acaba (fase %d)", fase);
    rodada = 5;
    venc_mao = 0;
    J[0].fichas = 150; J[1].fichas = 3;
    fase = F_RESULTADO;
    segue_do_resultado();
    CONFERE(fase == F_FIM && venc_partida == 0, "rival sem fichas: sem premio, acaba (fase %d)", fase);

    // ---- IA: o Dobro nunca vai por ultimo na sequencia -------------------
    {
        static const int T[7] = { D_DOBRO, D_D6, D_D8, D_D4, D_D2, D_D12, D_D20 };
        for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
        J[0].n_col = 0;
        for (int i = 0; i < 7; i++) {
            J[0].col[i] = (peca_t){ (uint8_t)T[i], 0 };
            J[0].onde[i] = NA_MAO;
            J[0].mao[i] = (int8_t)i;
            J[0].n_col++;
        }
        J[0].n_mao = 7;
        memset(F, 0, sizeof F);
        primeiro = vez = 0;
        rodada = 2;
        ia_esforco = 0;
        for (int niv = 1; niv <= 2; niv++) {
            ia_nivel = niv;
            plano_rodada = -1;
            ia_planeja();
            CONFERE(plano_n >= 2 && J[0].col[J[0].mao[plano[plano_n - 1]]].tipo != D_DOBRO,
                    "IA nivel %d pos o Dobro por ultimo", niv);
        }
        peca_t mao[7];
        for (int i = 0; i < 7; i++) mao[i] = J[0].col[i];
        mao[5].tipo = D_D2; mao[6].tipo = D_D2;          // o Dobro entre os de maior media
        int ordem[N_FILA], q = fila_simples(mao, 7, ordem);
        CONFERE(q >= 2 && mao[ordem[q - 1]].tipo != D_DOBRO, "IA facil pos o Dobro por ultimo");
        ia_esforco = 1;
        ia_nivel = 2;
    }

    // ---- IA: nao troca empate certo por derrota certa --------------------
    // O rival abriu e fez 6. A IA tem um 6 valendo e o proximo e um Quebrado
    // com 1 gravado: lancar anula o 6 e o 1. Parar empata.
    limpa();
    primeiro = 1;
    joga(1, D_D6, 6);
    joga(0, D_D6, 6);
    F[0].tipo[1] = D_QUEBRADO; F[0].fixo[1] = 1; F[0].n = 2;
    vez = 0;
    CONFERE(!ia_continua(), "IA: com empate na mao, nao devia lancar o Quebrado 1");
    // Perdendo, lanca: parar perde com certeza.
    limpa();
    primeiro = 1;
    joga(1, D_D6, 6);
    joga(0, D_D4, 2);
    F[0].tipo[1] = D_D8; F[0].n = 2;
    vez = 0;
    CONFERE(ia_continua(), "IA: perdendo, devia lancar mais um");
    // Vencendo, para.
    limpa();
    primeiro = 1;
    joga(1, D_D6, 3);
    joga(0, D_D6, 5);
    F[0].tipo[1] = D_D6; F[0].n = 2;
    vez = 0;
    CONFERE(!ia_continua(), "IA: vencendo, devia parar");
    // Pensar nao mexe na mesa nem no sorteio da partida.
    uint32_t sem_antes = semente;
    fila_t f_antes[2];
    memcpy(f_antes, F, sizeof F);
    (void)ia_continua();
    CONFERE(semente == sem_antes, "IA: pensar mexeu no sorteio da partida");
    CONFERE(!memcmp(f_antes, F, sizeof F), "IA: pensar mexeu na mesa");

    if (falhas) { printf("%d falhas\n", falhas); return 1; }
    puts("regras ok");
    return 0;
}

static int acha_na_lista(int t)
{
    int a = -1;
    for (int i = 0; i < ev_n; i++) if (run.col[ev_lista[i]].tipo == t) a = i;
    return a;
}
