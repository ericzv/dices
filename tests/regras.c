// Teste das regras dos dados, com jogadas montadas a mao: cada caso poe dados
// e valores exatos na fila e confere o que a regra da fila e o fim da rodada
// fazem. Inclui o dado.c para alcancar as funcoes internas.
#include <stdio.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
int64_t esp_timer_get_time(void) { return 1; }
void som_toca(int s) { (void)s; }
void som_toca_tom(int s, int t) { (void)s; (void)t; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }
void som_rufo_moeda(void) {}
void som_giro(bool on) { (void)on; }
void som_suspense(bool on) { (void)on; }

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
    if (k >= N_FILA) return;
    f->tipo[k] = (uint8_t)t;
    f->n = k + 1;
    f->cru[k] = f->valor[k] = v;
    f->lancados = k + 1;
    aplica_regra(j, k);
}

// Como joga, mas o dado cai pela regra completa (Ventania inclusive) e os
// lances que ela refaz saem de 'r'.
static int sempre_dois(int lados) { (void)lados; return 2; }
static int lances_sr[2], li_sr;           // os dois lances do Sorte/Reves, em ordem
static int rola_sr(int lados) { (void)lados; return lances_sr[li_sr++ % 2]; }
static int sempre_cinco(int lados) { return lados < 5 ? lados : 5; }
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

    // Semente: seus dados somam 14 ou mais, +7.
    limpa();
    joga(0, D_SEMENTE, 2); joga(0, D_D6, 5); joga(0, D_D8, 7);
    d = fim();
    CONFERE(d.total[0] == 14 + 7, "Semente com 14: esperava 21, deu %d", d.total[0]);
    limpa();
    joga(0, D_SEMENTE, 2); joga(0, D_D6, 5); joga(0, D_D8, 6);
    d = fim();
    CONFERE(d.total[0] == 13, "Semente com 13: esperava 13, deu %d", d.total[0]);

    // Trevo: 4 vale 10; 1 a 3, zero.
    limpa();
    joga(0, D_TREVO, 4);
    d = fim();
    CONFERE(d.total[0] == 10, "Trevo 4: esperava 10, deu %d", d.total[0]);
    limpa();
    joga(0, D_TREVO, 3);
    d = fim();
    CONFERE(d.total[0] == 0, "Trevo 3: esperava 0, deu %d", d.total[0]);

    // Par: igual ao ultimo que vale, +6.
    limpa();
    joga(0, D_D4, 3); joga(0, D_PAR, 3);
    d = fim();
    CONFERE(d.total[0] == 3 + 3 + 6, "Par: esperava 12, deu %d", d.total[0]);

    // Pirata: com 7 ou 8 tira 4 do oponente, sem ficar com eles; com 6, nao.
    limpa();
    joga(0, D_PIRATA, 7);
    joga(1, D_D8, 8);
    d = fim();
    CONFERE(d.total[0] == 7 && d.total[1] == 4, "Pirata 7: esperava 7 x 4, deu %d x %d", d.total[0], d.total[1]);
    limpa();
    joga(0, D_PIRATA, 6);
    joga(1, D_D8, 8);
    d = fim();
    CONFERE(d.total[0] == 6 && d.total[1] == 8, "Pirata 6: esperava 6 x 8, deu %d x %d", d.total[0], d.total[1]);

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

    // So dois iguais: +4.
    limpa();
    joga(0, D_JACKPOT, 2);
    joga(0, D_D6, 2);
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.total[0] == 9 + 4, "Jackpot com dois iguais: esperava 13, deu %d", d.total[0]);
    // Nenhum igual: nada.
    limpa();
    joga(0, D_JACKPOT, 2);
    joga(0, D_D6, 3);
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.total[0] == 10, "Jackpot sem iguais: esperava 10, deu %d", d.total[0]);

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
    // Com o Tudo ou Nada 2 junto: +5 em cada dado, antes dos multiplicadores.
    limpa();
    joga(0, D_TUDO_NADA, 2);
    joga(0, D_FARTURA, 2);
    joga(0, D_DOBRO, 2);
    joga(0, D_D4, 2);
    d = fim();
    // Cada um vale 2 + 5 = 7; Fartura x2 em todos (sao todos 2); o d4 ainda x2 do Dobro: x4.
    CONFERE(d.total[0] == 14 + 14 + 14 + 28, "Tudo ou Nada 2 com multiplicadores: esperava 70, deu %d", d.total[0]);

    // ---- Tudo ou Nada: 2 da +5 em cada dado seu; 1 tira 5 de cada ----------
    limpa();
    joga(0, D_TUDO_NADA, 2); joga(0, D_D6, 4);
    d = fim();
    CONFERE(d.total[0] == 7 + 9, "Tudo ou Nada 2: esperava 16, deu %d", d.total[0]);
    limpa();
    joga(0, D_TUDO_NADA, 1); joga(0, D_D12, 10); joga(0, D_D20, 15);
    d = fim();
    CONFERE(d.total[0] == -4 + 5 + 10, "Tudo ou Nada 1: esperava 11, deu %d", d.total[0]);
    limpa();
    joga(0, D_D20, 18); joga(0, D_D20, 19);
    joga(0, D_TUDO_NADA, 1);                           // cai com o 19; ainda tira 5 do 18
    d = fim();
    CONFERE(d.est[0][2] == V_ANULADO && d.est[0][0] == V_VALIDO && d.total[0] == 18 - 5,
            "Tudo ou Nada 1 caido: esperava 13, deu %d", d.total[0]);

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
        CONFERE(TIPO[D_LASTRO].raridade == RAR_LENDA, "Lastro devia ser lendario");
        CONFERE(TIPO[D_D12].raridade == RAR_RARO && TIPO[D_D20].raridade == RAR_LENDA, "D12 raro e D20 lenda");
        // Risco com barra 9: 4, 5, 6, 7 e 8 caem = 5 em 9.
        limpa();
        joga(0, D_D12, 9);
        F[0].tipo[1] = D_LASTRO; F[0].n = 2;
        CONFERE(risco(0) == 500 / 9, "Lastro com barra 9: risco %d, esperado %d", risco(0), 500 / 9);
    }

    // Todo dado que a loja vende tem preco.
    for (int i = 0; i < (int)sizeof VITRINE; i++)
        CONFERE(TIPO[VITRINE[i]].preco > 0, "%s esta na loja sem preco", TIPO[VITRINE[i]].nome);

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

        // Ferreiro: nao pontua; com 3 ou 4, +6 no dado seguinte se os dois valem.
        limpa();
        joga(0, D_FERREIRO, 3); joga(0, D_D6, 5);
        d = fim();
        CONFERE(d.total[0] == 11, "Ferreiro: esperava 0 + 5 + 6 = 11, deu %d", d.total[0]);
        // Com 1 ou 2 nao forja nada.
        limpa();
        joga(0, D_FERREIRO, 2); joga(0, D_D6, 5);
        d = fim();
        CONFERE(d.total[0] == 5 && !F[0].tag[1][0], "Ferreiro 2: esperava 5, deu %d", d.total[0]);
        // O seguinte caiu: nada de +4, e o Ferreiro sozinho vale 0.
        limpa();
        joga(0, D_FERREIRO, 3); joga(0, D_D6, 5); joga(0, D_D4, 1);
        d = fim();
        CONFERE(d.total[0] == 0, "Ferreiro sem o seguinte: esperava 0, deu %d", d.total[0]);
        // Ferreiro, Dobro e D6: o Dobro ganha +6 (3 + 6) e o D6 vale x2 (12): 21.
        limpa();
        joga(0, D_FERREIRO, 3); joga(0, D_DOBRO, 3); joga(0, D_D6, 6);
        d = fim();
        CONFERE(d.total[0] == 21, "Ferreiro + Dobro: esperava 21, deu %d", d.total[0]);

        // Cobrador: +4 se o oponente comecou a rodada com mais fichas.
        limpa();
        fichas_ini[0] = 50; fichas_ini[1] = 80;
        joga(0, D_COBRADOR, 3);
        d = fim();
        CONFERE(d.total[0] == 7, "Cobrador cobrando: esperava 7, deu %d", d.total[0]);
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

        // Solitario: +6 se nao ha dado valendo logo antes nem logo depois.
        limpa();
        joga(0, D_D6, 5); joga(0, D_D4, 1); joga(0, D_SOLITARIO, 2);
        d = fim();
        CONFERE(d.total[0] == 2 + 6, "Solitario sem vizinhos: esperava 8, deu %d", d.total[0]);
        limpa();
        joga(0, D_D6, 1); joga(0, D_SOLITARIO, 2);
        d = fim();
        CONFERE(d.total[0] == 3, "Solitario com vizinho: esperava 3, deu %d", d.total[0]);
        // Primeiro da fila; o segundo cai com o terceiro: sem vizinhos, +6.
        limpa();
        joga(0, D_SOLITARIO, 2); joga(0, D_D4, 3); joga(0, D_D6, 1);
        d = fim();
        CONFERE(d.total[0] == 2 + 6, "Solitario na ponta: esperava 8, deu %d", d.total[0]);
        // Um dado valendo mais longe (nao vizinho) nao atrapalha.
        limpa();
        joga(0, D_D4, 1); joga(0, D_D4, 3); joga(0, D_D4, 2); joga(0, D_SOLITARIO, 4);
        d = fim();
        CONFERE(d.total[0] == 1 + 4 + 6, "Solitario com dado longe: esperava 11, deu %d", d.total[0]);

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
        // Trinca: dado que tirar 3 ganha +3.
        limpa(); amul[0] = 1u << A_TRINCA;
        joga(0, D_D6, 3); joga(0, D_D8, 5);
        d = fim();
        CONFERE(d.total[0] == 3 + 3 + 5, "Trinca: esperava 11, deu %d", d.total[0]);
        // Prensa com Trinca: o 1 vira 3 e ainda ganha +3.
        limpa(); amul[0] = 1u << A_PRENSA | 1u << A_TRINCA;
        joga(0, D_D6, 1);
        d = fim();
        CONFERE(d.total[0] == 6, "Prensa com Trinca: esperava 6, deu %d", d.total[0]);
        // Piramide: x2 nos de 4 lados.
        limpa(); amul[0] = 1u << A_PIRAMIDE;
        joga(0, D_D4, 3); joga(0, D_D6, 5);
        d = fim();
        CONFERE(d.total[0] == 6 + 5, "Piramide: esperava 11, deu %d", d.total[0]);
        // Abre-Alas: x2 no primeiro da sequencia (so se ele terminar valendo).
        limpa(); amul[0] = 1u << A_ABRE_ALAS;
        joga(0, D_D6, 2); joga(0, D_D6, 5);
        d = fim();
        CONFERE(d.total[0] == 4 + 5, "Abre-Alas: esperava 9, deu %d", d.total[0]);
        limpa(); joga(0, D_D6, 5); joga(0, D_D6, 2);
        d = fim();
        CONFERE(d.total[0] == 0, "Abre-Alas caido nao vale: %d", d.total[0]);
        // Corrimao: so 1 abaixo nao cai; 2 abaixo, cai.
        limpa(); amul[0] = 1u << A_CORRIMAO; am_disparou = 0;
        joga(0, D_D6, 5); joga(0, D_D6, 4);
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO && (am_disparou >> A_CORRIMAO & 1),
                "Corrimao: 5 e 4 deviam ficar");
        joga(0, D_D6, 2);
        CONFERE(F[0].est[1] == V_ANULADO && F[0].est[2] == V_ANULADO && F[0].est[0] == V_VALIDO,
                "Corrimao: 4 e 2 deviam cair");
        limpa(); joga(0, D_D6, 5); F[0].n = 2; F[0].tipo[1] = D_D6;   // o proximo, um D6: so 1, 2 e 3 derrubam
        CONFERE(risco(0) == 50, "Corrimao: risco depois do 5 devia ser 50, deu %d", risco(0));
        // Saideira: na ultima rodada, os poderes do fim agem duas vezes.
        int rod = rodada;
        limpa(); amul[0] = 1u << A_SAIDEIRA; rodada = DES_RODADAS - 1;
        joga(0, D_D6, 2); joga(0, D_CAPITAO, 3);
        d = fim();
        CONFERE(d.total[0] == 2 + 3 + 2, "Saideira antes da ultima: esperava 7, deu %d", d.total[0]);
        rodada = DES_RODADAS;
        d = fim();
        CONFERE(d.total[0] == 2 + 3 + 2 + 2, "Saideira: esperava 9, deu %d", d.total[0]);
        bool aviso = false;
        for (int i = 0; i < n_ev; i++) aviso |= ev[i].tipo == EV_AVISO && (ev[i].am >> A_SAIDEIRA & 1);
        CONFERE(aviso, "Saideira: sem o aviso de tudo de novo");
        limpa(); amul[0] = 1u << A_SAIDEIRA | 1u << A_ABRE_ALAS;     // x2 de novo: x4
        joga(0, D_D6, 4);
        d = fim();
        CONFERE(d.total[0] == 16, "Saideira com Abre-Alas: esperava 16, deu %d", d.total[0]);
        limpa(); amul[0] = 1u << A_SAIDEIRA;                         // nada para repetir: nem o aviso
        joga(0, D_D6, 4);
        d = fim();
        CONFERE(d.total[0] == 4 && n_ev == 0, "Saideira sem poderes: %d pontos, %d eventos", d.total[0], n_ev);
        rodada = rod;
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
        // Com 2 tambem volta; com 3, nao.
        limpa();
        joga_com(0, D_BUMERANGUE, 2, sempre_cinco);
        CONFERE(F[0].valor[0] == 5 && !strcmp(F[0].tag[0], "2>5"), "Bumerangue 2: devia voltar e tirar 5 (%d)", F[0].valor[0]);
        limpa();
        joga_com(0, D_BUMERANGUE, 3, sempre_cinco);
        CONFERE(F[0].valor[0] == 3 && !F[0].tag[0][0], "Bumerangue 3: nao devia voltar (%d)", F[0].valor[0]);
        // Capitao: +1 por dado seu valendo, ele incluido.
        limpa();
        joga(0, D_D6, 2); joga(0, D_D4, 3); joga(0, D_CAPITAO, 5);
        d = fim();
        CONFERE(d.total[0] == 2 + 3 + 5 + 3, "Capitao: esperava 13, deu %d", d.total[0]);
        limpa();
        joga(0, D_D6, 5); joga(0, D_D4, 2); joga(0, D_CAPITAO, 6);
        d = fim();
        CONFERE(d.total[0] == 6 + 1, "Capitao sozinho: esperava 7, deu %d", d.total[0]);
        // Viuva Negra: x2 se for o unico dado seu valendo.
        limpa();
        joga(0, D_D6, 5); joga(0, D_D4, 2); joga(0, D_VIUVA, 7);
        d = fim();
        CONFERE(d.total[0] == 14, "Viuva sozinha: esperava 14, deu %d", d.total[0]);
        limpa();
        joga(0, D_D4, 2); joga(0, D_VIUVA, 7);
        d = fim();
        CONFERE(d.total[0] == 9, "Viuva acompanhada: esperava 9, deu %d", d.total[0]);
        limpa();
        joga(0, D_VIUVA, 3);
        joga(1, D_CARRASCO, 4); joga(1, D_D6, 5);              // o Carrasco anula o maior: ela
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.total[0] == 0, "Viuva anulada pelo Carrasco: esperava 0, deu %d", d.total[0]);
        // O Carrasco do oponente anula o outro: ela fica sozinha e dobra.
        limpa();
        joga(0, D_VIUVA, 3); joga(0, D_D8, 7);
        joga(1, D_CARRASCO, 4);
        d = fim();
        CONFERE(d.total[0] == 6, "Viuva que ficou sozinha: esperava 6, deu %d", d.total[0]);
        limpa();
        joga(0, D_D6, 2); joga(0, D_D8, 7); joga(0, D_CAPITAO, 3);   // caiu: nao da nada
        d = fim();
        CONFERE(d.total[0] == 2, "Capitao anulado: esperava 2, deu %d", d.total[0]);
        // Berserker: valendo, anula um outro dado seu ao acaso.
        limpa();
        joga(0, D_D6, 3); joga(0, D_BERSERKER, 15);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.total[0] == 15, "Berserker: esperava 15, deu %d", d.total[0]);
        limpa();
        joga(0, D_D4, 2); joga(0, D_D6, 4); joga(0, D_BERSERKER, 12);
        d = fim();
        CONFERE((d.est[0][0] == V_ANULADO) + (d.est[0][1] == V_ANULADO) == 1 && d.est[0][2] == V_VALIDO,
                "Berserker: devia anular so um dos outros");
        limpa();
        joga(0, D_ESCUDO, 3); joga(0, D_BERSERKER, 15);
        d = fim();
        CONFERE(d.total[0] == 18, "Berserker com Escudo: esperava 18, deu %d", d.total[0]);
        limpa();
        joga(0, D_BERSERKER, 17);
        d = fim();
        CONFERE(d.total[0] == 17, "Berserker sozinho: esperava 17, deu %d", d.total[0]);
        limpa();
        joga(0, D_D6, 2); joga(0, D_D12, 9); joga(0, D_BERSERKER, 5);   // caiu: nao anula
        d = fim();
        CONFERE(d.est[0][1] == V_ANULADO && d.est[0][2] == V_ANULADO && d.est[0][0] == V_VALIDO && d.total[0] == 2,
                "Berserker anulado nao devia agir (%d)", d.total[0]);
        limpa();
        joga(0, D_FENIX, 4); joga(0, D_BERSERKER, 10);
        d = fim();
        CONFERE(d.total[0] == 14, "Berserker + Fenix: a Fenix renasce, esperava 14, deu %d", d.total[0]);
        // Autofagia: valendo, anula o outro dado seu de menor numero (no
        // empate, o primeiro); Escudo resiste; caida, nao age.
        limpa();
        joga(0, D_D4, 2); joga(0, D_D8, 5); joga(0, D_AUTOFAGIA, 9);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.est[0][1] == V_VALIDO && d.total[0] == 5 + 9,
                "Autofagia: devia comer o 2 (deu %d)", d.total[0]);
        CONFERE(poder_armado(0, 2) == 2, "Autofagia com alvo devia brilhar contra o dono");
        limpa();
        joga(0, D_D4, 3); joga(0, D_D6, 3); joga(0, D_AUTOFAGIA, 4);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.est[0][1] == V_VALIDO, "Autofagia no empate: devia comer o primeiro");
        limpa();
        joga(0, D_ESCUDO, 2); joga(0, D_D8, 5); joga(0, D_AUTOFAGIA, 9);
        d = fim();
        CONFERE(d.est[0][0] == V_VALIDO && d.est[0][1] == V_ANULADO, "Autofagia: o Escudo resiste, vai o 5");
        limpa();
        joga(0, D_AUTOFAGIA, 7);
        d = fim();
        CONFERE(d.total[0] == 7, "Autofagia sozinha: esperava 7, deu %d", d.total[0]);
        limpa();
        joga(0, D_D4, 2); joga(0, D_D12, 9); joga(0, D_AUTOFAGIA, 3);
        d = fim();
        CONFERE(F[0].est[2] == V_ANULADO && d.est[0][0] == V_VALIDO && d.total[0] == 2,
                "Autofagia caida nao devia agir (deu %d)", d.total[0]);
        // Come de verdade: com 8 dados em jogo, o comido sai da bolsa e ela
        // guarda +1 (ate +3); com 7, ou ja com +3, so anula.
        {
            jogador_t guarda = J[0];
            J[0].n_col = 8;
            for (int i = 0; i < 8; i++) { J[0].col[i] = (peca_t){ D_D6, 0 }; J[0].onde[i] = NA_BOLSA; }
            J[0].col[2] = (peca_t){ D_AUTOFAGIA, 0 };
            limpa();
            joga(0, D_D4, 2); joga(0, D_D8, 5); joga(0, D_AUTOFAGIA, 9);
            F[0].idx[0] = 0; F[0].idx[1] = 1; F[0].idx[2] = 2;
            d = fim();
            CONFERE(d.come[0] == 1 && d.come_de[0] == 3, "Autofagia com 8 em jogo devia comer o 2");
            autofagia_come(0, &d);
            CONFERE(J[0].onde[0] == COMIDO && J[0].col[2].fixo == 1 && em_jogo(&J[0]) == 7,
                    "Autofagia: o comido devia sair da bolsa e ela guardar +1");
            d = fim();
            CONFERE(d.come[0] == 0 && d.est[0][0] == V_ANULADO, "Autofagia com 7 em jogo: so anula");
            J[0].onde[0] = NA_BOLSA;
            J[0].col[2].fixo = AUTO_MAX;
            d = fim();
            CONFERE(d.come[0] == 0, "Autofagia ja com +3: so anula");
            limpa();
            F[0].fixo[0] = 9;
            joga(0, D_AUTOFAGIA, 4);
            d = fim();
            CONFERE(d.total[0] == 4 + AUTO_MAX, "Autofagia: o bonus para em +3 (deu %d)", d.total[0]);
            J[0] = guarda;
        }
        // ---- Os dados das builds ------------------------------------------
        // Eco: o mesmo do primeiro que vale (ate 6); pode cair se a fila subiu.
        limpa();
        joga(0, D_D6, 4); joga(0, D_ECO, 1);
        CONFERE(F[0].valor[1] == 4 && F[0].est[1] == V_VALIDO && !strcmp(F[0].tag[1], "=4"), "Eco devia repetir o 4");
        limpa();
        joga(0, D_D4, 3); joga(0, D_D6, 5); joga(0, D_ECO, 6);
        CONFERE(F[0].valor[2] == 3 && F[0].est[2] == V_ANULADO && F[0].est[1] == V_ANULADO, "Eco 3 depois do 5 devia cair");
        limpa();
        joga(0, D_D8, 7); joga(0, D_ECO, 2);
        CONFERE(F[0].valor[1] == 2, "Eco: acima de 6, rola normal");
        limpa();
        joga(0, D_ECO, 5);
        CONFERE(F[0].valor[0] == 5, "Eco primeiro: rola normal");
        // Domino: o seguinte com o mesmo numero, +3.
        limpa();
        joga(0, D_DOMINO, 3); joga(0, D_D6, 3);
        d = fim();
        CONFERE(d.total[0] == 3 + 3 + 3, "Domino: esperava 9, deu %d", d.total[0]);
        limpa();
        joga(0, D_DOMINO, 3); joga(0, D_D6, 4);
        d = fim();
        CONFERE(d.total[0] == 7, "Domino sem par: esperava 7, deu %d", d.total[0]);
        // Ancora: o seguinte nao cai nem e roubado; menor, cai ela.
        limpa();
        joga(0, D_D4, 2); joga(0, D_ANCORA, 3); joga(0, D_D6, 1);
        d = fim();
        CONFERE(F[0].ancora[2] && F[0].est[2] == V_VALIDO && F[0].est[1] == V_ANULADO && d.total[0] == 3,
                "Ancora: o 1 preso devia ficar e ela cair (deu %d)", d.total[0]);
        limpa();
        joga(0, D_ANCORA, 2); joga(0, D_D8, 7);
        joga(1, D_CARRASCO, 4);
        d = fim();
        CONFERE(d.est[0][1] == V_VALIDO && d.est[0][0] == V_ANULADO, "Ancora: o Carrasco devia pular o 7 preso");
        limpa();
        joga(0, D_ANCORA, 4); joga(0, D_D4, 4);
        joga(1, D_CARIDOSO, 3);
        d = fim();
        CONFERE(d.est[0][1] == V_VALIDO && d.est[0][0] == V_ROUBADO, "Ancora: o preso nao e roubado");
        // Martir: caido, +2 em cada um que vale.
        limpa();
        joga(0, D_D4, 2); joga(0, D_D8, 6); joga(0, D_MARTIR, 3); joga(0, D_D8, 7);
        d = fim();
        CONFERE(d.est[0][2] == V_ANULADO && d.total[0] == 2 + 7 + 4, "Martir caido: esperava 13, deu %d", d.total[0]);
        CONFERE(poder_armado(0, 2) == 1, "Martir caido devia brilhar");
        limpa();
        joga(0, D_MARTIR, 3);
        d = fim();
        CONFERE(d.total[0] == 3, "Martir de pe: esperava 3, deu %d", d.total[0]);
        // Puxa-fila: primeiro, e os quatro valendo, +5.
        limpa();
        joga(0, D_PUXA_FILA, 1); joga(0, D_D4, 2); joga(0, D_D6, 3); joga(0, D_D8, 4);
        d = fim();
        CONFERE(d.total[0] == 1 + 2 + 3 + 4 + 5, "Puxa-fila: esperava 15, deu %d", d.total[0]);
        limpa();
        joga(0, D_PUXA_FILA, 1); joga(0, D_D4, 2); joga(0, D_D6, 3);
        d = fim();
        CONFERE(d.total[0] == 6, "Puxa-fila com tres: esperava 6, deu %d", d.total[0]);
        // Coveiro: com 5 ou 6, +1 por anulado na mesa.
        limpa();
        joga(0, D_D6, 5); joga(0, D_D4, 2); joga(0, D_COVEIRO, 6);
        joga(1, D_D6, 4); joga(1, D_D4, 1);
        d = fim();
        CONFERE(d.total[0] == 6 + 4, "Coveiro 6 com 4 anulados: esperava 10, deu %d", d.total[0]);
        limpa();
        joga(0, D_D6, 5); joga(0, D_D4, 2); joga(0, D_COVEIRO, 4);
        d = fim();
        CONFERE(d.total[0] == 4, "Coveiro 4: esperava 4, deu %d", d.total[0]);
        // Hexa: +1 por d6 seu valendo, ele incluido.
        limpa();
        joga(0, D_D6, 2); joga(0, D_HEXA, 3); joga(0, D_D4, 4);
        d = fim();
        CONFERE(d.total[0] == 2 + 3 + 4 + 2, "Hexa: esperava 11, deu %d", d.total[0]);
        // Ourico: cada dado seu atacado custa 4 ao oponente.
        limpa();
        joga(0, D_OURICO, 3); joga(0, D_D8, 7);
        joga(1, D_CARRASCO, 4);
        d = fim();
        CONFERE(d.est[0][1] == V_ANULADO && d.total[0] == 3 && d.total[1] == 0, "Ourico: esperava 3 x 0, deu %d x %d",
                d.total[0], d.total[1]);
        // Calibre: o seguinte tira 1 a mais, ate o maximo dele.
        limpa();
        joga(0, D_CALIBRE, 2); joga_com(0, D_LASER, 4, sempre_dois);
        CONFERE(F[0].valor[1] == 5 && F[0].calibre[1] && poder_armado(0, 1) == 1, "Calibre: o Laser 4 devia virar 5");
        limpa();
        joga(0, D_CALIBRE, 3); joga_com(0, D_D4, 4, sempre_dois);
        CONFERE(F[0].valor[1] == 4 && !F[0].calibre[1], "Calibre: o 4 do d4 ja e o maximo");
        limpa();
        joga(0, D_CALIBRE, 1); joga_com(0, D_TREVO, 3, sempre_dois);
        d = fim();
        CONFERE(d.total[0] == 1 + 10, "Calibre + Trevo: o 3 vira 4 e vale 10 (deu %d)", d.total[0]);
        // Sabotador: com 5 ou 6, o maior do oponente vale metade.
        limpa();
        joga(0, D_SABOTADOR, 6);
        joga(1, D_D8, 7); joga(1, D_D12, 9);
        d = fim();
        CONFERE(d.total[1] == 7 + 4 && d.total[0] == 6, "Sabotador: esperava 6 x 11, deu %d x %d", d.total[0], d.total[1]);
        limpa();
        joga(0, D_SABOTADOR, 4);
        joga(1, D_D12, 9);
        d = fim();
        CONFERE(d.total[1] == 9, "Sabotador 4: esperava 9, deu %d", d.total[1]);
        // Azarao: comecou atras em fichas, x2.
        limpa();
        fichas_ini[0] = 30; fichas_ini[1] = 50;
        joga(0, D_AZARAO, 3);
        d = fim();
        CONFERE(d.total[0] == 6, "Azarao atras: esperava 6, deu %d", d.total[0]);
        fichas_ini[0] = fichas_ini[1] = 0;
        limpa();
        joga(0, D_AZARAO, 3);
        d = fim();
        CONFERE(d.total[0] == 3, "Azarao empatado: esperava 3, deu %d", d.total[0]);
        // Ampulheta: +1 a cada 3 rodadas jogadas, ate +4.
        {
            int rod = rodada;
            rodada = 7; limpa(); joga(0, D_AMPULHETA, 4); d = fim();
            CONFERE(d.total[0] == 4 + 2, "Ampulheta na 7a: esperava 6, deu %d", d.total[0]);
            rodada = 15; limpa(); joga(0, D_AMPULHETA, 4); d = fim();
            CONFERE(d.total[0] == 4 + 4, "Ampulheta na 15a: esperava 8, deu %d", d.total[0]);
            rodada = 1; limpa(); joga(0, D_AMPULHETA, 4); d = fim();
            CONFERE(d.total[0] == 4, "Ampulheta na 1a: esperava 4, deu %d", d.total[0]);
            rodada = rod;
        }
        // Agiota: valendo, 2 fichas do oponente no fim.
        limpa();
        joga(0, D_AGIOTA, 2);
        d = fim();
        CONFERE(d.agiota[0] == 2 && d.agiota[1] == 0 && d.total[0] == 2, "Agiota: devia cobrar 2 fichas");
        limpa();
        joga(0, D_D6, 5); joga(0, D_AGIOTA, 2);
        d = fim();
        CONFERE(d.agiota[0] == 0, "Agiota caido nao cobra");
        // Medusa: valendo, a fila vira Quebrados com o que tirou (os caidos
        // tambem; os roubados, nao).
        {
            jogador_t guarda = J[0];
            limpa();
            joga(0, D_D6, 4); joga(0, D_D8, 2); joga(0, D_MEDUSA, 9);
            F[0].idx[0] = 0; F[0].idx[1] = 1; F[0].idx[2] = 2;
            J[0].n_col = 3;
            J[0].col[0] = (peca_t){ D_D6, 0 }; J[0].col[1] = (peca_t){ D_D8, 0 }; J[0].col[2] = (peca_t){ D_MEDUSA, 0 };
            d = fim();
            CONFERE(d.medusa[0] && !d.medusa[1] && d.total[0] == 9, "Medusa valendo devia agir (total %d)", d.total[0]);
            medusa_petrifica(0, &d);
            CONFERE(J[0].col[0].tipo == D_QUEBRADO && J[0].col[0].fixo == 4 && J[0].col[1].tipo == D_QUEBRADO
                    && J[0].col[1].fixo == 2 && J[0].col[2].tipo == D_QUEBRADO && J[0].col[2].fixo == 9,
                    "Medusa: a fila devia virar Quebrados 4, 2 e 9");
            limpa();
            joga(0, D_D12, 9); joga(0, D_MEDUSA, 3);
            d = fim();
            CONFERE(!d.medusa[0], "Medusa caida nao age");
            // O Sorte/Reves negativo vira pedra negativa, e ela sai com esse numero.
            limpa();
            joga(0, D_SORTE_REVES, -3); joga(0, D_MEDUSA, 2);
            F[0].idx[0] = 0; F[0].idx[1] = 1;
            J[0].n_col = 2;
            J[0].col[0] = (peca_t){ D_SORTE_REVES, 0 }; J[0].col[1] = (peca_t){ D_MEDUSA, 0 };
            d = fim();
            medusa_petrifica(0, &d);
            CONFERE(J[0].col[0].tipo == D_QUEBRADO && (int8_t)J[0].col[0].fixo == -3,
                    "Medusa num Reves -3: devia virar Quebrado -3 (deu %d)", (int8_t)J[0].col[0].fixo);
            int menor = 0;
            CONFERE(sorteia_valor(D_QUEBRADO, J[0].col[0].fixo, sempre_dois, &menor) == -3, "Quebrado -3 sai -3");
            // Jogada: na partida rapida, se desfaz como todo Quebrado; no
            // Desafiante, se parte e deixa o Dado Misterioso.
            int modo0 = modo;
            for (int m = 0; m < 2; m++) {
                modo = m ? M_DESAFIO : M_UM;
                J[0].n_col = N_FILA + 2;
                for (int i = 0; i < J[0].n_col; i++) { J[0].col[i] = (peca_t){ D_D6, 0 }; J[0].onde[i] = NA_MAO; }
                J[0].col[3] = (peca_t){ D_QUEBRADO, (uint8_t)(int8_t)-3 };
                limpa();
                memset(vis_queima, 0, sizeof vis_queima);
                joga(0, D_QUEBRADO, -3);
                F[0].idx[0] = 3; F[0].fixo[0] = J[0].col[3].fixo;
                recolhe_quebrados();
                if (m) CONFERE(J[0].n_col == N_FILA + 2 && J[0].col[3].tipo == D_MISTERIOSO && F[0].quebra[0],
                               "Desafiante: a pedra negativa devia deixar o Dado Misterioso");
                else CONFERE(J[0].n_col == N_FILA + 1 && J[0].col[3].tipo == D_D6, "rapida: o Quebrado -3 se desfaz");
            }
            modo = modo0;
            t_misterio = 0;
            J[0] = guarda;
        }
        // Rancor: +1 por rodada perdida na partida, valendo.
        {
            int p0 = J[0].perdidas;
            J[0].perdidas = 3;
            limpa(); joga(0, D_RANCOR, 2); d = fim();
            CONFERE(d.total[0] == 2 + 3, "Rancor com 3 perdidas: esperava 5, deu %d", d.total[0]);
            limpa(); joga(0, D_D8, 6); joga(0, D_RANCOR, 2); d = fim();
            CONFERE(d.total[0] == 0, "Rancor caido nao soma (deu %d)", d.total[0]);
            J[0].perdidas = 0;
            limpa(); joga(0, D_RANCOR, 2); d = fim();
            CONFERE(d.total[0] == 2 && !poder_armado(0, 0), "Rancor sem rodada perdida: so o 2 (deu %d)", d.total[0]);
            J[0].perdidas = p0;
        }
        // Esperanca: comecou a rodada com 15 fichas ou menos, x2.
        {
            int f0 = fichas_ini[0], f1 = fichas_ini[1];
            fichas_ini[0] = 15; fichas_ini[1] = 60;
            limpa(); joga(0, D_D4, 2); joga(0, D_ESPERANCA, 5); d = fim();
            CONFERE(d.total[0] == 2 + 10, "Esperanca com 15 fichas: esperava 12, deu %d", d.total[0]);
            fichas_ini[0] = 16;
            limpa(); joga(0, D_D4, 2); joga(0, D_ESPERANCA, 5); d = fim();
            CONFERE(d.total[0] == 2 + 5, "Esperanca com 16 fichas: esperava 7, deu %d", d.total[0]);
            fichas_ini[0] = f0; fichas_ini[1] = f1;
        }
        // Ponte: nunca cai, e o seguinte compara com o de antes dela.
        limpa(); joga(0, D_D8, 6); joga(0, D_PONTE, 1);
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO, "Ponte com 1 depois do 6: os dois ficam");
        F[0].n = 3; F[0].tipo[2] = D_D6;
        CONFERE(risco(0) == 83, "depois da Ponte, o D6 compara com o 6: risco 83, deu %d", risco(0));
        joga(0, D_D6, 4);
        CONFERE(F[0].est[0] == V_ANULADO && F[0].est[1] == V_VALIDO && F[0].est[2] == V_ANULADO,
                "4 depois de 6 e Ponte: caem o 6 e o 4, a Ponte fica");
        limpa(); joga(0, D_D4, 2); joga(0, D_PONTE, 4); joga(0, D_D6, 3);
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO && F[0].est[2] == V_VALIDO,
                "a Ponte com 4 nao sobe a regua: o 3 compara com o 2");
        limpa(); joga(0, D_PONTE, 3); joga(0, D_D2, 1);
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO, "Ponte primeira: o seguinte vale como o primeiro");
        limpa(); joga(0, D_D6, 5); joga(0, D_PONTE, 2); joga(0, D_GEMEO, 1);
        CONFERE(F[0].valor[2] == 5 && F[0].est[2] == V_VALIDO, "Gemeo depois da Ponte copia o de antes dela");
        // Pote Quente: +1 a cada 10 fichas no pote, valendo.
        {
            int pt = pote;
            pote = 60;
            limpa(); joga(0, D_POTE_QUENTE, 5); d = fim();
            CONFERE(d.total[0] == 5 + 2, "Pote Quente com 60 no pote: esperava 7, deu %d", d.total[0]);
            pote = 24;
            limpa(); joga(0, D_POTE_QUENTE, 5); d = fim();
            CONFERE(d.total[0] == 5, "Pote Quente com 24 no pote: esperava 5, deu %d", d.total[0]);
            pote = pt;
        }
        // Garimpeiro: valendo, os outros dados que valem voltam para a bolsa;
        // ele mesmo, os que cairam e os que ficaram na mao, nao.
        {
            jogador_t guarda = J[0];
            J[0].n_col = 9;
            for (int i = 0; i < 9; i++) { J[0].col[i] = (peca_t){ D_D6, 0 }; J[0].onde[i] = i < 7 ? NA_MAO : FORA; }
            limpa(); joga(0, D_D6, 5); joga(0, D_D4, 3); joga(0, D_GARIMPEIRO, 4); joga(0, D_D8, 7);
            F[0].idx[0] = 0; F[0].idx[1] = 3; F[0].idx[2] = 5; F[0].idx[3] = 1;
            d = fim();
            CONFERE(d.garimpo[0] == 1 << 3, "Garimpeiro: so o 7 que vale devia voltar (mascara %d)", d.garimpo[0]);
            garimpa(0, &d);
            CONFERE(J[0].onde[1] == NA_BOLSA && J[0].onde[5] == NA_MAO && J[0].onde[0] == NA_MAO && J[0].onde[3] == NA_MAO
                    && J[0].onde[7] == FORA, "Garimpeiro: so os outros que valem voltam para a bolsa");
            limpa(); joga(0, D_GARIMPEIRO, 4); d = fim();
            CONFERE(!d.garimpo[0] && !poder_armado(0, 0), "Garimpeiro sozinho nao devolve nada");
            limpa(); joga(0, D_D8, 6); joga(0, D_GARIMPEIRO, 4); d = fim();
            CONFERE(!d.garimpo[0], "Garimpeiro caido nao age");
            J[0] = guarda;
        }
        // Os dados novos: Previsivel, Oculto, Sacrificio, Vazio, Media, Vampiro,
        // Colecionador, Pacto e Assombracao.
        {
            int menor = 0;
            CONFERE(sorteia_valor(D_PREVISIVEL, 0, sempre_dois, &menor) == 1 && sorteia_valor(D_PREVISIVEL, 4, sempre_dois, &menor) == 5
                    && sorteia_valor(D_PREVISIVEL, 11, sempre_dois, &menor) == 12 && sorteia_valor(D_PREVISIVEL, 20, sempre_dois, &menor) == 12,
                    "Previsivel: 1, 2, 3... ate 12");
            limpa(); joga(0, D_D6, 4); F[0].n = 2; F[0].tipo[1] = D_PREVISIVEL; F[0].fixo[1] = 2;
            CONFERE(risco(0) == 100, "Previsivel: o 3 depois de um 4 cai com certeza (%d)", risco(0));
            F[0].fixo[1] = 3;
            CONFERE(risco(0) == 0, "Previsivel: o 4 depois de um 4 vale com certeza (%d)", risco(0));
            // Oculto: escondido nao cai nem derruba; no fim aparece e a sequencia e refeita.
            limpa(); joga(0, D_D8, 5); joga_com(0, D_OCULTO, 3, sempre_dois);
            CONFERE(F[0].escondido[1] && F[0].est[1] == V_ESPERA && F[0].est[0] == V_VALIDO && F[0].pontos == 5,
                    "Oculto: escondido devia ficar fora da sequencia");
            joga(0, D_D6, 6);
            CONFERE(F[0].est[2] == V_VALIDO, "Oculto: o seguinte compara com o de antes dele");
            desfecho_t pv;
            calcula_desfecho(&pv, false);
            CONFERE(pv.total[0] == 11 && F[0].escondido[1], "Oculto: a previa nao sabe o numero (%d)", pv.total[0]);
            d = fim();
            CONFERE(!F[0].escondido[1] && F[0].est[0] == V_ANULADO && F[0].est[1] == V_ANULADO && d.total[0] == 6,
                    "Oculto: o 3 aparece, derruba o 5 e o 6 recomeca (total %d)", d.total[0]);
            int revela = 0;
            for (int i = 0; i < n_ev; i++) revela += ev[i].tipo == EV_REVELA;
            CONFERE(revela == 1, "Oculto: devia ter o evento de revelar");
            CONFERE(melhor_lance(0, &(peca_t){ 0 }), "melhor lance com o Oculto ja aberto");
            limpa(); joga(0, D_D4, 2); joga_com(0, D_OCULTO, 8, sempre_dois);
            {
                peca_t cp;
                CONFERE(melhor_lance(0, &cp) && cp.tipo == D_D4, "Coringa nao copia o Oculto escondido");
            }
            // Sacrificio: nao pontua; valendo, x2 no menor que pontua.
            limpa(); joga(0, D_D4, 2); joga(0, D_SACRIFICIO, 3); joga(0, D_D8, 5); d = fim();
            CONFERE(d.total[0] == 4 + 0 + 5, "Sacrificio: esperava 9, deu %d", d.total[0]);
            // Vazio: vale 0, nunca cai, e baixa a barra.
            limpa(); joga(0, D_D12, 10); joga(0, D_VAZIO, 0); joga(0, D_D4, 2); d = fim();
            CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO && F[0].est[2] == V_VALIDO && d.total[0] == 12,
                    "Vazio: esperava 12, deu %d", d.total[0]);
            limpa(); joga(0, D_D12, 10); F[0].n = 2; F[0].tipo[1] = D_VAZIO;
            CONFERE(risco(0) == 0 && sorteia_valor(D_VAZIO, 0, sempre_dois, &menor) == 0, "Vazio nunca cai e vale 0");
            // Vazio nao pode ser anulado: nem pela sequencia nem por dado nenhum.
            limpa(); joga(0, D_VAZIO, 0); joga(1, D_LASER, 6); d = fim();
            CONFERE(d.est[0][0] == V_VALIDO, "Vazio anulado pelo Laser");
            limpa(); joga(0, D_D6, 4); joga(0, D_VAZIO, 0); joga(0, D_SANGUESSUGA, 3);
            CONFERE(F[0].est[1] == V_VALIDO && F[0].est[2] == V_VALIDO, "Vazio sugado pela Sanguessuga");
            limpa(); joga(0, D_VAZIO, 0); joga(0, D_SORTE_REVES, -3);
            CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_ANULADO, "Vazio derrubado por um negativo depois dele");
            limpa(); joga(0, D_EGOISTA, 5); joga(0, D_VAZIO, 0);
            CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO, "Vazio anulado pelo Egoista");
            limpa(); joga(0, D_D6, 3); joga(0, D_VAZIO, 0); joga(0, D_MALDITO, 5);
            CONFERE(F[0].est[0] == V_ANULADO && F[0].est[1] == V_VALIDO, "Vazio anulado pelo Agouro");
            CONFERE(nao_anulavel_t(D_VAZIO) && !intocavel_t(D_VAZIO), "Vazio: nao anulavel, mas pode ser roubado");
            // Media: + o total dividido por 4.
            limpa(); joga(0, D_D6, 4); joga(0, D_MEDIA, 4); joga(0, D_D8, 8); d = fim();
            CONFERE(d.total[0] == 16 + 4, "Media: esperava 20, deu %d", d.total[0]);
            // Vampiro: valendo, -1 em cada dado do oponente que vale.
            limpa(); joga(1, D_D6, 3); joga(1, D_D6, 5); joga(0, D_VAMPIRO, 1); d = fim();
            CONFERE(d.total[1] == 2 + 4 && d.total[0] == 1, "Vampiro: oponente devia ficar com 6, ficou %d", d.total[1]);
            limpa(); joga(1, D_D6, 3); joga(0, D_D4, 4); joga(0, D_VAMPIRO, 2); d = fim();
            CONFERE(d.total[1] == 3, "Vampiro caido nao age");
            // Colecionador: com 6, + metade dos dados que voce tem.
            for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
            int tem = em_jogo(&J[0]);
            limpa(); joga(0, D_COLECIONADOR, 6); d = fim();
            CONFERE(d.total[0] == 6 + tem / 2, "Colecionador com 6: esperava %d, deu %d", 6 + tem / 2, d.total[0]);
            limpa(); joga(0, D_COLECIONADOR, 5); d = fim();
            CONFERE(d.total[0] == 5, "Colecionador sem 6 nao ganha nada");
            // Pacto: lancado, um Fantasma na sua bolsa; Assombracao valendo, na do oponente.
            int c0 = J[0].n_col, c1 = J[1].n_col;
            limpa(); joga(0, D_D8, 7); joga(0, D_PACTO, 2);           // o Pacto caiu: cobra do mesmo jeito
            recolhe_quebrados();
            CONFERE(J[0].n_col == c0 + 1 && J[0].col[c0].tipo == D_FANTASMA && J[1].n_col == c1, "Pacto: um Fantasma na sua bolsa");
            limpa(); joga(0, D_ASSOMBRACAO, 4);
            recolhe_quebrados();
            CONFERE(J[1].n_col == c1 + 1 && J[1].col[c1].tipo == D_FANTASMA, "Assombracao valendo: Fantasma na bolsa dele");
            limpa(); joga(0, D_D6, 6); joga(0, D_ASSOMBRACAO, 2);
            recolhe_quebrados();
            CONFERE(J[1].n_col == c1 + 1, "Assombracao caida nao assombra");
            some_fantasmas();
            for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
            // Ficha Velha: valendo, conta o que tirou; anulada (caida ou atacada), +4 no fim.
            limpa(); joga(0, D_FICHA_VELHA, 2); d = fim();
            CONFERE(d.total[0] == 2, "Ficha Velha valendo: esperava 2, deu %d", d.total[0]);
            limpa(); joga(0, D_D6, 5); joga(0, D_FICHA_VELHA, 1); d = fim();
            CONFERE(F[0].est[0] == V_ANULADO && F[0].est[1] == V_ANULADO && d.total[0] == FICHA_VELHA_PTS,
                    "Ficha Velha caida: os dois anulados e +%d, deu %d", FICHA_VELHA_PTS, d.total[0]);
            CONFERE(poder_armado(0, 1) == 1, "Ficha Velha anulada: o poder fica armado");
            limpa(); joga(0, D_FICHA_VELHA, 2); joga(1, D_LASER, 6); d = fim();
            CONFERE(d.est[0][0] == V_ANULADO && d.total[0] == FICHA_VELHA_PTS,
                    "Ficha Velha atacada pelo Laser: +%d, deu %d", FICHA_VELHA_PTS, d.total[0]);
            // Sanguessuga: ao pousar anula o seu de antes, nunca cai, e nada a anula.
            limpa(); joga(0, D_D6, 5); joga(0, D_SANGUESSUGA, 3);
            CONFERE(F[0].est[0] == V_ANULADO && F[0].est[1] == V_VALIDO && ver_tipo == 4,
                    "Sanguessuga menor: suga o 5 e fica (est %d %d)", F[0].est[0], F[0].est[1]);
            joga(0, D_D6, 4);                                       // compara com ela (3): vale
            joga(0, D_D4, 1);                                       // menor que o 4: cai so o 4 e ele
            d = fim();
            CONFERE(F[0].est[1] == V_VALIDO && F[0].est[2] == V_ANULADO && F[0].est[3] == V_ANULADO && d.total[0] == 3,
                    "Sanguessuga: a seguir a sequencia parte dela (total %d)", d.total[0]);
            limpa(); joga(0, D_D6, 2); joga(0, D_SANGUESSUGA, 7); joga(0, D_D4, 3); d = fim();
            CONFERE(F[0].est[0] == V_ANULADO && F[0].est[1] == V_VALIDO && F[0].est[2] == V_ANULADO && d.total[0] == 7,
                    "Sanguessuga maior tambem suga, e o 3 cai sem derruba-la (total %d)", d.total[0]);
            limpa(); joga(0, D_SANGUESSUGA, 6); d = fim();
            CONFERE(F[0].est[0] == V_VALIDO && d.total[0] == 6, "Sanguessuga sozinha so vale");
            limpa(); joga(0, D_ESCUDO, 4); joga(0, D_SANGUESSUGA, 2); d = fim();
            CONFERE(F[0].est[0] == V_VALIDO && d.total[0] == 6, "o Escudo resiste a Sanguessuga (total %d)", d.total[0]);
            limpa(); joga(0, D_SANGUESSUGA, 8); joga(1, D_LASER, 6); d = fim();
            CONFERE(d.est[0][0] == V_VALIDO && d.total[0] == 8, "o Laser nao anula a Sanguessuga");
            limpa(); joga(0, D_D8, 6); F[0].n = 2; F[0].tipo[1] = D_SANGUESSUGA;
            CONFERE(risco(0) == 0, "Sanguessuga nunca cai: risco 0");
            limpa(); joga(0, D_D8, 6); joga(0, D_SANGUESSUGA, 2); joga(0, D_MALDITO, 3);   // o Agouro zera a sequencia
            CONFERE(F[0].est[1] == V_VALIDO, "o Agouro nao derruba a Sanguessuga");
            // Liberacao: no maximo LIB_DADOS por nivel, e todos os novos abrem em algum nivel.
            static const int NOVOS[] = { D_PREVISIVEL, D_OCULTO, D_SACRIFICIO, D_VAZIO, D_MEDIA, D_VAMPIRO,
                                         D_COLECIONADOR, D_PACTO, D_ASSOMBRACAO, D_FICHA_VELHA, D_SANGUESSUGA };
            for (unsigned i = 0; i < sizeof NOVOS / sizeof NOVOS[0]; i++)
                CONFERE(nivel_do_dado(NOVOS[i]) >= 1 && nivel_do_dado(NOVOS[i]) <= N_NIVEIS && TEXTO_CAT[NOVOS[i]] && TEXTO_MEDIO[NOVOS[i]],
                        "%s: nivel %d", TIPO[NOVOS[i]].nome, nivel_do_dado(NOVOS[i]));
            limpa();
        }
        // O Dado Misterioso e oculto: um d20 sem poder, fora da colecao, do
        // premio, da loja e da liberacao.
        CONFERE(TIPO[D_MISTERIOSO].lados == 20 && TIPO[D_MISTERIOSO].efeito == EF_NADA, "Misterioso: um d20 comum");
        CONFERE(peso_dado(D_MISTERIOSO) == 0 && !dado_liberado(D_MISTERIOSO), "Misterioso nao sai em sorteio");
        {
            int vis = 0;
            for (int t = 0; t < D_N; t++) vis += !oculto(t);
            CONFERE(vis == N_CATALOGO, "a colecao devia ter %d dados (tem %d)", vis, N_CATALOGO);
            for (int i = 0; i < N_CATALOGO; i++) CONFERE(cat_tipo(i) != D_MISTERIOSO, "Misterioso na colecao");
            for (int i = 0; i < n_basicos(); i++) CONFERE(basico(i) != D_MISTERIOSO, "Misterioso na partida rapida");
        }
        // Gemeo: tira o mesmo do anterior que vale; sozinho, fica com o seu.
        limpa();
        joga(0, D_D6, 4); joga(0, D_GEMEO, 2);
        d = fim();
        CONFERE(F[0].valor[1] == 4 && d.total[0] == 8, "Gemeo: esperava 8, deu %d", d.total[0]);
        limpa();
        joga(0, D_GEMEO, 5);
        d = fim();
        CONFERE(d.total[0] == 5, "Gemeo sozinho: esperava 5, deu %d", d.total[0]);
        // So ate 6 (e um d6): depois de um 9, fica com o que rolou (e pode cair).
        limpa();
        joga(0, D_D8, 6); joga(0, D_GEMEO, 2);
        CONFERE(F[0].valor[1] == 6 && F[0].est[1] == V_VALIDO, "Gemeo devia copiar o 6");
        limpa();
        joga(0, D_D12, 9); joga(0, D_GEMEO, 3);
        CONFERE(F[0].valor[1] == 3 && F[0].est[1] == V_ANULADO && F[0].est[0] == V_ANULADO,
                "Gemeo depois de 9: devia ficar com o 3 e cair (valor %d)", F[0].valor[1]);
        // Checkpoint valendo: +3 em cada dado seu valendo lancado depois dele.
        // Caido, nao age.
        limpa();
        joga(0, D_D4, 2); joga(0, D_CHECKPOINT, 3); joga(0, D_D6, 4); joga(0, D_D8, 6);
        d = fim();
        CONFERE(d.total[0] == 2 + 3 + 4 + 6 + 6, "Checkpoint: esperava 21, deu %d", d.total[0]);
        CONFERE(poder_armado(0, 1) == 1, "Checkpoint com dados depois devia brilhar");
        limpa();
        joga(0, D_D6, 5); joga(0, D_CHECKPOINT, 6); joga(0, D_D4, 2); joga(0, D_D8, 7);
        d = fim();
        CONFERE(F[0].est[1] == V_ANULADO && d.total[0] == 5 + 7,
                "Checkpoint caido nao age: esperava 12, deu %d", d.total[0]);
        CONFERE(poder_armado(0, 1) == 0, "Checkpoint caido nao devia brilhar");
        limpa();
        joga(0, D_CHECKPOINT, 6);
        d = fim();
        CONFERE(d.total[0] == 6, "Checkpoint sozinho: esperava 6, deu %d", d.total[0]);
        // Sorte/Reves: um d12 soma, um d8 tira; o dado vale a diferenca, ate
        // negativa, e o seguinte so precisa passar dela.
        {
            int menor;
            lances_sr[0] = 12; lances_sr[1] = 5; li_sr = 0;
            int x = sorteia_valor(D_SORTE_REVES, 0, rola_sr, &menor);
            CONFERE(x == 7 && menor == 5, "Sorte/Reves 12 e 5: esperava 7 (reves 5), deu %d (%d)", x, menor);
        }
        limpa();
        joga(0, D_D4, 3);
        F[0].descarte[1] = 5; joga(0, D_SORTE_REVES, 7);
        CONFERE(F[0].est[1] == V_VALIDO && !strcmp(F[0].tag[1], "12-5"), "Sorte/Reves: etiqueta %s", F[0].tag[1]);
        limpa();
        F[0].descarte[0] = 8; joga(0, D_SORTE_REVES, -6);
        joga(0, D_D4, 1);
        d = fim();
        CONFERE(F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO && d.bruto[0] == -5 && d.total[0] == 0,
                "Sorte/Reves negativo: esperava bruto -5 e total 0, deu %d e %d", d.bruto[0], d.total[0]);
        limpa();
        joga(0, D_D6, 5);
        F[0].tipo[1] = D_SORTE_REVES; F[0].n = 2;
        CONFERE(risco(0) == 70, "Sorte/Reves depois de um 5: esperava 70%% de cair, deu %d", risco(0));
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
        // Trono: +4 se for o 4o da fila e os quatro terminarem valendo.
        limpa();
        joga(0, D_D4, 1); joga(0, D_D6, 3); joga(0, D_D8, 5); joga(0, D_TRONO, 10);
        d = fim();
        CONFERE(d.total[0] == 1 + 3 + 5 + 10 + 4, "Trono em 4o: esperava 23, deu %d", d.total[0]);
        CONFERE(poder_armado(0, 3) == 1, "Trono em 4o com todos valendo devia brilhar");
        limpa();
        joga(0, D_D6, 3); joga(0, D_TRONO, 10);                    // so dois dados
        d = fim();
        CONFERE(d.total[0] == 13 && poder_armado(0, 1) == 0, "Trono em 2o: esperava 13, deu %d", d.total[0]);
        limpa();
        joga(0, D_D6, 5); joga(0, D_D4, 2); joga(0, D_D8, 6); joga(0, D_TRONO, 8);   // o 5 e o 2 caem
        d = fim();
        CONFERE(d.total[0] == 14 && poder_armado(0, 3) == 0, "Trono com dado caido: esperava 14, deu %d", d.total[0]);
        limpa();
        joga(0, D_TRONO, 8); joga(0, D_D12, 9);
        d = fim();
        CONFERE(d.total[0] == 8 + 9, "Trono no meio: esperava 17, deu %d", d.total[0]);
        // Nenhum deles entra no sorteio das partidas basicas.
        for (int t = 0; t < D_N; t++) if (so_desafio(t)) CONFERE(peso_dado(t) == 0, "dado %d na partida basica", t);
        // Fora do Desafiante, o premio sobe de raridade com as rodadas: 1 a 5,
        // comuns e incomuns; 6 a 10, incomuns e raros; 11 em diante, raros e
        // lendarios. A loja do comeco: so comuns e incomuns.
        int rod_antes = rodada;
        rodada = 3;
        CONFERE(peso_dado(D_VIDRO) > 0 && peso_dado(D_JACKPOT) > 0 && peso_dado(D_CARIDOSO) == 0 && peso_dado(D_D20) == 0,
                "rodada 3: so comuns e incomuns");
        rodada = 8;
        CONFERE(peso_dado(D_VIDRO) == 0 && peso_dado(D_JACKPOT) > 0 && peso_dado(D_CARIDOSO) > 0 && peso_dado(D_D20) == 0,
                "rodada 8: so incomuns e raros");
        rodada = 12;
        CONFERE(peso_dado(D_JACKPOT) == 0 && peso_dado(D_CARIDOSO) > 0 && peso_dado(D_D20) > 0, "rodada 12: so raros e lendarios");
        rodada = rod_antes;
        for (unsigned i = 0; i < sizeof VITRINE; i++)
            CONFERE(TIPO[VITRINE[i]].raridade <= RAR_INCOMUM, "a loja do comeco vende o raro %s", TIPO[VITRINE[i]].nome);
    }

    // ---- Perfis: gravar e ler; precos da dificuldade e do Cupom ----------
    {
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[1].nome, sizeof perfis[1].nome, "LILI");
        perfis[1].avatar = AV_NOMALISA; perfis[1].nivel = 5; perfis[1].venceu[4] = 4; perfis[1].runs = 300;
        perfil_atual = 1;
        perfis_salva();
        perfil_t copia[N_PERFIS];
        memcpy(copia, perfis, sizeof copia);
        memset(perfis, 0, sizeof perfis); perfil_atual = -1; perfis_lidos = false;
        perfis_le();
        CONFERE(!memcmp(copia, perfis, sizeof copia) && perfil_atual == 1, "perfis nao voltaram iguais");
        CONFERE(!strcmp(chave_run(), "desafio2"), "perfil 2 grava em %s", chave_run());
        CONFERE(dado_liberado(D_FARTURA) && !dado_liberado(D_ESPELHO) && amuleto_liberado(A_COROA)
                && !amuleto_liberado(A_CUPOM), "o que abre com o nivel 5");
        CONFERE(!dado_liberado(D_CAPITAO) && !dado_liberado(D_CAVALO) && !dado_liberado(D_BERSERKER),
                "Capitao, Cavalo e Berserker fechados no nivel 5");
        perfis[1].nivel = 8;
        CONFERE(dado_liberado(D_CAPITAO) && !dado_liberado(D_CAVALO), "Capitao abre no 8, o Cavalo nao");
        perfis[1].nivel = 9;
        CONFERE(dado_liberado(D_CAVALO) && !dado_liberado(D_BERSERKER), "Cavalo no 9; o Berserker so no 10");
        perfis[1].nivel = 10;
        CONFERE(dado_liberado(D_BERSERKER) && dado_liberado(D_MEDUSA), "Berserker e Medusa no 10");
        perfis[1].nivel = 0;                         // no Normal, so os simples
        CONFERE(dado_liberado(D_PAR) && dado_liberado(D_ESCUDO) && !dado_liberado(D_LASER) && !dado_liberado(D_ESPELHO)
                && !dado_liberado(D_OCULTO), "no Normal so os dados simples");
        for (int n = 1; n <= N_NIVEIS; n++) {        // um amuleto por nivel
            int c = 0;
            for (int a = 0; a < A_N; a++) c += NIVEL_AMULETO[a] == n;
            CONFERE(c == 1, "nivel %d abre %d amuletos", n, c);
        }
        for (int n = 1; n <= N_NIVEIS; n++) {        // a tela da dificuldade mostra ate LIB_DADOS
            int c = 0;
            for (int t = 0; t < D_N; t++) c += nivel_do_dado(t) == n && marco_do_dado(t) < 0;
            CONFERE(c <= LIB_DADOS, "nivel %d abre %d dados (cabem %d)", n, c, LIB_DADOS);
        }
        perfis[1].nivel = 8;
        // Os fortes de antes: Explosivo, Egoista e Tudo ou Nada ja abriram no nivel 8; o
        // Agouro nao; D20 e Lastro so com o chefe de cada salao vencido.
        CONFERE(dado_liberado(D_EXPLOSIVO) && dado_liberado(D_EGOISTA) && dado_liberado(D_TUDO_NADA)
                && !dado_liberado(D_MALDITO) && !dado_liberado(D_D20) && !dado_liberado(D_LASTRO),
                "dados fortes no nivel 8");
        perfis[1].marcos = 1;
        CONFERE(dado_liberado(D_D20) && !dado_liberado(D_LASTRO), "D20 com o Crupie vencido");
        {
            float w[4] = { 0, 0, 1, 1 };             // so raros e lendarios
            for (int i = 0; i < 300; i++) {
                int t = sorteia_raridade(w);
                CONFERE(dado_liberado(t), "sorteou dado fechado: %s", TIPO[t].nome);
            }
        }
        // O perfil dos 5 niveis volta convertido: o nivel n vira 2n.
        {
            perfil_v_t v[N_PERFIS];
            static char buf[8 + 2 * sizeof v + 1];
            memset(v, 0, sizeof v);
            snprintf(v[2].nome, sizeof v[2].nome, "VELHA");
            v[2].avatar = AV_BERRO; v[2].nivel = 3; v[2].venceu[2] = 7; v[2].venceu[5] = 1; v[2].runs = 42; v[2].marcos = 3;
            memcpy(buf, "PER32", 5);
            hex_de(v, sizeof v, buf + 5);
            salva_grava("perfis", buf);
            perfis_lidos = false; perfis_le();
            CONFERE(perfil_atual == 2 && !strcmp(perfis[2].nome, "VELHA") && perfis[2].avatar == AV_BERRO
                    && perfis[2].nivel == 6 && perfis[2].venceu[4] == 7 && perfis[2].venceu[10] == 1
                    && !perfis[2].venceu[2] && perfis[2].runs == 42 && perfis[2].marcos == 3,
                    "perfil de antes: nivel %d, venceu[4] %d", perfis[2].nivel, perfis[2].venceu[4]);
        }
        // Salvamento estragado: nenhum perfil, sem quebrar.
        salva_grava("perfis", "PER4zz");
        perfis_lidos = false; perfis_le();
        CONFERE(perfil_atual == -1 && !perfis[0].nome[0] && perfil()->nivel == 0, "perfis estragados");
        memset(&run, 0, sizeof run);
        run.dificuldade = 3;
        CONFERE(preco_run(D_D8) == 50 && preco_am() == 87, "Nivel 3: precos %d e %d", preco_run(D_D8), preco_am());
        run_poe_amuleto(A_CUPOM);
        CONFERE(run_amuletos() >> A_CUPOM & 1 && run.amuletos_hi, "amuleto 16+ nao entrou na parte alta");
        CONFERE(preco_run(D_D8) == 37, "Nivel 3 com Cupom: %d", preco_run(D_D8));
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
        // Selo de Divida: o primeiro chefe deixa um incomum, a escolher; o Selo
        // cobra mais um, sorteado entre os comuns e os incomuns.
        run_poe_amuletos(1u << A_SELO | 1u << A_PEDAGIO);
        run.tier = 3; run.andar = 0;
        abre_premio_run();
        des_premio_feito();
        CONFERE(fase == F_RAMULETO && run.estado == R_AMULETO && run.am_volta == AV_CHEFE && !run.am_novo,
                "chefe: sem amuleto (fase %d)", fase);
        for (int i = 0; i < 3; i++)
            CONFERE(run.am_oferta[i] < A_N && RAR_AMULETO[run.am_oferta[i]] == AMR_INCOMUM,
                    "o primeiro chefe devia deixar incomuns: %d", run.am_oferta[i]);
        int esc = run.am_oferta[cursor];
        amuleto_escolhido(cursor);
        int sel = run.am_oferta[1];
        CONFERE(fase == F_RAMULETO && run.am_volta == AV_SELO && cursor == 1 && sel < A_N
                && run.am_oferta[0] == 0xFF && run.am_oferta[2] == 0xFF
                && (RAR_AMULETO[sel] == AMR_COMUM || RAR_AMULETO[sel] == AMR_INCOMUM) && (run_amuletos() >> esc & 1),
                "Selo: o segundo amuleto (fase %d, volta %d, %d)", fase, run.am_volta, sel);
        run_t r2;
        CONFERE(des_carrega(&r2) && r2.estado == R_AMULETO && r2.am_volta == AV_SELO, "Selo: o segundo nao ficou salvo");
        amuleto_escolhido(1);
        CONFERE(am_conta(run_amuletos()) == 4 && (run_amuletos() >> sel & 1) && !run.am_selo && fase == F_MAPA,
                "Selo: depois do segundo, o mapa (fase %d, %d amuletos)", fase, am_conta(run_amuletos()));
        // O segundo chefe e o Barao da Cobertura deixam raros; o Barao antes dela, nada.
        run.tier = 3; run.andar = 1;
        CONFERE(chefe_deixa_amuleto(), "o segundo chefe devia deixar amuleto");
        oferece_amuleto(AV_CHEFE);
        for (int i = 0; i < 3; i++)
            CONFERE(run.am_oferta[i] < A_N && RAR_AMULETO[run.am_oferta[i]] == AMR_RARO, "o segundo chefe: raros");
        run.andar = 2; run.dificuldade = N_NIVEIS - 1;
        CONFERE(!chefe_deixa_amuleto(), "o Barao antes da Cobertura nao deixa amuleto");
        run.dificuldade = N_NIVEIS;
        CONFERE(chefe_deixa_amuleto(), "o Barao da Cobertura deixa amuleto");
        run.andar = 3;
        CONFERE(!chefe_deixa_amuleto(), "o Coringa fecha a run: sem amuleto");
        run.dificuldade = 0; run.andar = 0;
        // Cinco lugares: o sexto pede troca. Voltar devolve a escolha; trocar tira o escolhido.
        run_poe_amuletos(1u << A_BOLSA | 1u << A_OURO | 1u << A_PENA | 1u << A_IMA | 1u << A_COFRE);
        oferece_amuleto(AV_CHEFE);
        cursor = 2;
        int novo = run.am_oferta[2];
        amuleto_escolhido(2);
        CONFERE(run.am_novo == novo + 1 && fase == F_RAMULETO && am_conta(run_amuletos()) == 5 && run.estado == R_AMULETO,
                "troca: o novo devia esperar (am_novo %d)", run.am_novo);
        CONFERE(des_carrega(&r2) && r2.am_novo == novo + 1, "troca: nao ficou salva");
        amuleto_desiste();
        CONFERE(!run.am_novo && fase == F_RAMULETO && cursor == 2, "troca: voltar devia devolver a escolha");
        amuleto_escolhido(2);
        amuleto_troca(A_OURO);
        CONFERE(am_conta(run_amuletos()) == 5 && !(run_amuletos() >> A_OURO & 1) && (run_amuletos() >> novo & 1)
                && !run.am_novo && fase == F_MAPA, "troca: o Ouro devia sair e o %d entrar", novo);
        // Na loja, sem lugar: so paga se trocar.
        run.estado = R_LOJA; run.andar = 0; run.moedas = 500;
        monta_loja_run();
        int al = run.am_loja;
        CONFERE(al < A_N && RAR_AMULETO[al] == AMR_COMUM, "a loja do Salao Verde vende comuns: %d", al);
        abre_loja_run();
        ganha_amuleto(al, AV_LOJA);
        CONFERE(run.am_novo == al + 1 && run.moedas == 500 && fase == F_RAMULETO, "loja cheia: devia pedir troca");
        amuleto_desiste();
        CONFERE(!run.am_novo && run.moedas == 500 && !run.am_levou && fase == F_RLOJA, "loja: voltar sem pagar");
        ganha_amuleto(al, AV_LOJA);
        amuleto_troca(-1);
        CONFERE(run.moedas == 500 && !run.am_levou && fase == F_RLOJA && !(run_amuletos() >> al & 1),
                "loja: deixar o novo nao paga");
        ganha_amuleto(al, AV_LOJA);
        amuleto_troca(A_BOLSA);
        CONFERE(run.moedas == 500 - preco_am() && run.am_levou && (run_amuletos() >> al & 1)
                && !(run_amuletos() >> A_BOLSA & 1) && am_conta(run_amuletos()) == 5, "loja: trocar paga");
        run.andar = 1; monta_loja_run();
        CONFERE(run.am_loja >= A_N || RAR_AMULETO[run.am_loja] == AMR_INCOMUM, "a loja do Salao Vermelho: incomuns");
        run.andar = 2; monta_loja_run();
        CONFERE(run.am_loja >= A_N || RAR_AMULETO[run.am_loja] == AMR_RARO, "a loja do Salao Dourado: raros");
        // Todos tem raridade; os que voltaram, nas que foram escolhidas.
        for (int a = 0; a < A_N; a++) CONFERE(amuleto_no_jogo(a), "amuleto %d sem raridade", a);
        CONFERE(RAR_AMULETO[A_FERRADURA] == AMR_RARO && RAR_AMULETO[A_COROA] == AMR_INCOMUM
                && RAR_AMULETO[A_PRENSA] == AMR_INCOMUM && RAR_AMULETO[A_CUPOM] == AMR_COMUM
                && RAR_AMULETO[A_TACA] == AMR_COMUM && RAR_AMULETO[A_CORINGA] == AMR_COMUM
                && RAR_AMULETO[A_DUPLA] == AMR_COMUM, "raridade dos que voltaram");
        // Os de nivel so saem depois de abertos: no Normal, nem a Ferradura nem a Dupla.
        run_poe_amuletos(0);
        for (int i = 0; i < 300; i++) {
            uint8_t a;
            sorteia_amuletos(&a, 1, AMR(AMR_COMUM) | AMR(AMR_INCOMUM) | AMR(AMR_RARO));
            CONFERE(a < A_N && NIVEL_AMULETO[a] <= perfil()->nivel, "sorteou amuleto fechado: %d", a);
        }
        // Os que sairam do jogo nao saem em sorteio nenhum.
        run_poe_amuletos(0);
        for (int i = 0; i < 200; i++) {
            uint8_t a;
            sorteia_amuletos(&a, 1, AMR(AMR_COMUM) | AMR(AMR_INCOMUM) | AMR(AMR_RARO));
            CONFERE(a < A_N && amuleto_no_jogo(a), "sorteou amuleto fora do jogo: %d", a);
        }
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
        // Depois do Barao: no ultimo nivel, o salao do Coringa (a ultima loja e
        // ele); nas outras dificuldades, a run acaba ali.
        for (int dif = N_NIVEIS - 1; dif <= N_NIVEIS; dif++) {
            run.dificuldade = (uint8_t)dif;
            run.andar = DES_ANDARES - 1; run.passo = DES_PASSOS - 1;
            fase = F_MAPA; des_venceu_run = false;
            avanca_passo();
            if (dif < N_NIVEIS) CONFERE(fase == F_RFIM && des_venceu_run, "dificuldade %d: a run devia acabar no Barao", dif);
            else {
                CONFERE(run.andar == DES_ANDAR_FINAL && run.passo == 0 && run.no[0][1].tipo == NO_LOJA
                        && run.no[1][1].tipo == NO_CHEFE && run.no[1][1].perso == PERSO_CORINGA && fase == F_MAPA,
                        "Desafiante 5: devia abrir o salao do Coringa (andar %d, fase %d)", run.andar, fase);
                run_t rr2;
                CONFERE(des_carrega(&rr2) && rr2.andar == DES_ANDAR_FINAL, "o salao do Coringa devia ficar salvo");
                run.no[1][1].semente = 4242;
                run.passo = 1; run.caminho[1] = 1;
                des_comeca_partida();
                CONFERE(reg_op == REG_CORINGA && nivel_jog[1] == 2,
                        "Coringa: trunfo %d, nivel %d", reg_op, nivel_jog[1]);
                int fracos = 0, bons = 0;              // a bolsa dele: sem dados de saida, so do tema
                for (int i = 0; i < J[1].n_col; i++) {
                    int t = J[1].col[i].tipo;
                    fracos += t == D_D2 || t == D_D4 || t == D_D6;
                    bons += t == D_D20 || t == D_ESCUDO || t == D_LASER;
                }
                CONFERE(fracos == 0 && bons >= 3, "Coringa: bolsa com %d dados fracos e %d bons", fracos, bons);
                fase = F_MAPA;
                avanca_passo();
                CONFERE(fase == F_RFIM && des_venceu_run, "vencer o Coringa devia fechar a run");
            }
        }
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
            run.dificuldade = 6;
            CONFERE(tam_mao(0) == 7, "Crupie no 6o andar ainda nao tira dado seu: %d", tam_mao(0));
            run.dificuldade = 7;
            CONFERE(tam_mao(0) == 6, "Crupie no 7o andar: voce compra 6, comprou %d", tam_mao(0));
            // Barao no 4o andar: +3 por dado seu anulado e x2 no primeiro dele.
            fala_perso = PERSO_CHEFE0 + 2; run.dificuldade = 4;
            limpa(); rodada = 5;
            joga(1, D_D6, 2); joga(1, D_D6, 5);
            joga(0, D_D6, 5); joga(0, D_D6, 2);          // cai: os dois anulados
            d = fim();
            CONFERE(d.total[1] == 4 + 5 + 6, "Barao virado: esperava 15, deu %d", d.total[1]);
            // No 9o andar, x2 tambem no segundo (o terceiro fica como saiu).
            run.dificuldade = 9;
            limpa(); rodada = 5;
            joga(1, D_D6, 2); joga(1, D_D6, 5); joga(1, D_D6, 6);
            d = fim();
            CONFERE(d.total[1] == 4 + 10 + 6, "Barao no 9o andar: esperava 20, deu %d", d.total[1]);
            // No 3o andar o Barao ainda nao vira.
            run.dificuldade = 3;
            limpa(); rodada = 5; joga(1, D_D6, 2); d = fim();
            CONFERE(d.total[1] == 2, "Barao sem virada no 3o andar: %d", d.total[1]);
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
        // Os trunfos novos: Revanche, Cara ou coroa, Velorio, Juros, Bruxaria e o Coringa.
        {
            desfecho_t d;
            // A Revanche do Rival e o amuleto Desforra: a mesma regra.
            reg_op = REG_NADA; rodada = 2;
            amul[1] = 1u << A_DESFORRA;
            J[1].perdeu_ultima = true;
            limpa(); joga(1, D_D6, 3); joga(1, D_D8, 5); d = fim();
            CONFERE(d.total[1] == 3 + 5 + 2, "Revanche depois de perder: esperava 10, deu %d", d.total[1]);
            J[1].perdeu_ultima = false;
            limpa(); joga(1, D_D6, 3); d = fim();
            CONFERE(d.total[1] == 3, "Revanche depois de vencer: esperava 3, deu %d", d.total[1]);
            amul[1] = 0; amul[0] = 1u << A_DESFORRA;
            J[0].perdeu_ultima = true;
            limpa(); joga(0, D_D6, 3); joga(0, D_D8, 5); d = fim();
            CONFERE(d.total[0] == 3 + 5 + 2, "Desforra depois de perder: esperava 10, deu %d", d.total[0]);
            J[0].perdeu_ultima = false; amul[0] = 0;
            reg_op = REG_MALUCO;
            maluco_lado = 2; limpa(); joga(1, D_D6, 3); joga(1, D_D8, 5); d = fim();
            CONFERE(d.total[1] == 3 + 5 + 4, "Maluco com cara: esperava 12, deu %d", d.total[1]);
            maluco_lado = -1; limpa(); joga(1, D_D6, 3); d = fim();
            CONFERE(d.total[1] == 2, "Maluco com coroa: esperava 2, deu %d", d.total[1]);
            reg_op = REG_NADA; amul[1] = 1u << A_CRISANTEMO;   // o Velorio do Coveiro
            limpa(); joga(1, D_D8, 6); joga(1, D_D4, 2); joga(1, D_D6, 5); d = fim();
            CONFERE(d.total[1] == 5 + 2, "Velorio: 2 caidos, esperava 7, deu %d", d.total[1]);
            amul[1] = 0; amul[0] = 1u << A_CRISANTEMO;         // e o seu Crisantemo
            limpa(); joga(0, D_D8, 6); joga(0, D_D4, 2); joga(0, D_D6, 5); d = fim();
            CONFERE(d.total[0] == 5 + 2, "Crisantemo: 2 caidos, esperava 7, deu %d", d.total[0]);
            amul[0] = 0;
            reg_op = REG_JUROS;
            entrada(30, 40, 0);
            CONFERE(J[0].fichas == 30 - 5 - 2 && pote == 12 && juros_rodada == 2, "Juros atras: %d fichas, pote %d", J[0].fichas, pote);
            entrada(40, 30, 0);
            CONFERE(J[0].fichas == 35 && pote == 10 && juros_rodada == 0, "Juros na frente: %d fichas", J[0].fichas);
            // Bruxaria: rodada vencida por ela, um Fantasma na sua bolsa.
            reg_op = REG_NADA; amul[1] = 1u << A_MAU_OLHADO;
            for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
            int antes = J[0].n_col;
            limpa();
            memset(&resolvido, 0, sizeof resolvido);
            resolvido.total[0] = 3; resolvido.total[1] = 9;
            J[0].fichas = J[1].fichas = 40; pote = 10;
            conclui_rodada();
            CONFERE(J[0].n_col == antes + 1 && J[0].col[antes].tipo == D_FANTASMA && J[0].onde[antes] == NA_BOLSA
                    && J[0].perdidas >= 1 && J[0].perdeu_ultima && fantasma_para == 0,
                    "Bruxaria: devia por um Fantasma na sua bolsa");
            // O seu Mau-Olhado: rodada vencida por voce, um Fantasma na bolsa dele.
            amul[1] = 0; amul[0] = 1u << A_MAU_OLHADO;
            int antes1 = J[1].n_col;
            limpa();
            memset(&resolvido, 0, sizeof resolvido);
            resolvido.total[0] = 9; resolvido.total[1] = 3;
            J[0].fichas = J[1].fichas = 40; pote = 10;
            conclui_rodada();
            CONFERE(J[1].n_col == antes1 + 1 && J[1].col[antes1].tipo == D_FANTASMA && fantasma_para == 1
                    && J[0].n_col == antes + 1, "Mau-Olhado: devia por um Fantasma na bolsa dele");
            amul[0] = 0;
            int menor = 0;
            CONFERE(sorteia_valor(D_FANTASMA, 1, sempre_dois, &menor) == 1, "Fantasma vale sempre 1");
            limpa(); joga(0, D_D6, 4); F[0].n = 2; F[0].tipo[1] = D_FANTASMA; F[0].fixo[1] = 1;
            CONFERE(risco(0) == 100, "Fantasma depois de um 4 cai com certeza (%d)", risco(0));
            some_fantasmas();
            CONFERE(J[0].n_col == antes, "fim da partida: o Fantasma some");
            CONFERE(peso_dado(D_FANTASMA) == 0 && !dado_liberado(D_FANTASMA), "Fantasma nao sai em sorteio");
            // O Coringa: monta 3; o quarto e a copia do seu dado que tirou mais
            // no seu turno (empate: o ultimo).
            reg_op = REG_CORINGA;
            CONFERE(max_fila(1) == 3 && max_fila(0) == N_FILA, "Coringa monta 3");
            peca_t c;
            limpa(); joga(0, D_D8, 6); joga(0, D_D20, 9); joga(0, D_D12, 9);
            CONFERE(melhor_lance(0, &c) && c.tipo == D_D12, "Coringa: empate no 9 devia copiar o ultimo (D12), copiou %d", c.tipo);
            limpa(); joga(0, D_D12, 10); joga(0, D_D4, 3);
            CONFERE(melhor_lance(0, &c) && c.tipo == D_D12, "Coringa: o 10 caiu, mas foi o maior que voce tirou");
            // Voce ja jogou: o quarto e a copia, na hora.
            limpa(); primeiro = 0; joga(0, D_LASTRO, 8); F[0].fixo[0] = 3;
            F[1].n = 3; F[1].tipo[0] = F[1].tipo[1] = F[1].tipo[2] = D_D6;
            coringa_completa(1);
            CONFERE(F[1].n == 4 && !F[1].carta && F[1].tipo[3] == D_LASTRO && F[1].fixo[3] == 3 && F[1].idx[3] == -1,
                    "Coringa: a copia devia ser o quarto");
            // Ele abre: o quarto e a carta, em suspensao. Posta, nao cai.
            limpa(); primeiro = 1;
            F[1].n = 3; F[1].tipo[0] = F[1].tipo[1] = F[1].tipo[2] = D_D6;
            coringa_completa(1);
            CONFERE(F[1].n == 4 && F[1].carta && !carta_posta(1), "Coringa abrindo: o quarto devia ser a carta");
            joga(1, D_D6, 5); joga(1, D_D6, 5); joga(1, D_D6, 6);
            sim_lanca(1);
            CONFERE(carta_posta(1) && F[1].lancados == 4 && F[1].est[3] == V_ESPERA && F[1].est[2] == V_VALIDO
                    && F[1].pontos == 16, "Coringa: a carta posta devia esperar sem cair (est %d, pontos %d)", F[1].est[3], F[1].pontos);
            // Voce joga um 2 e um 9 no D20: a carta vira o D20 e e lancada.
            joga(0, D_D4, 2); joga(0, D_D20, 9);
            CONFERE(vira_carta(1) && !F[1].carta && F[1].tipo[3] == D_D20 && F[1].lancados == 3,
                    "Coringa: a carta devia virar o seu D20");
            joga(1, D_D20, 4);
            CONFERE(F[1].est[3] == V_ANULADO && F[1].est[2] == V_ANULADO, "a copia que tira menos cai, como qualquer dado");
            // Sem nada seu lancado, a carta sai da fila.
            limpa(); F[1].n = 4; F[1].carta = true; F[1].lancados = 4;
            CONFERE(!vira_carta(1) && F[1].n == 3 && !F[1].carta, "Coringa: sem o que copiar, a carta sai");
            // Da 5a rodada, dado seu anulado e destruido (antes, nao).
            for (int r5 = 4; r5 <= 5; r5++) {
                for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
                int antes_c = J[0].n_col;
                limpa(); rodada = r5;
                joga(0, D_D6, 5); joga(0, D_D6, 2); joga(0, D_D8, 7);
                F[0].idx[0] = 0; F[0].idx[1] = 1; F[0].idx[2] = 2;
                recolhe_quebrados();
                int esperado = r5 >= VIRADA_RODADA ? antes_c - 2 : antes_c;
                CONFERE(J[0].n_col == esperado, "Coringa na rodada %d: bolsa com %d, esperava %d", r5, J[0].n_col, esperado);
            }
            reg_op = REG_NADA; rodada = 1;
            for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
        }
        // Trunfos aos poucos, por dificuldade: nenhum comum no Normal; todos no 5.
        {
            int andar0 = run.andar, dif0 = run.dificuldade;
            run.dificuldade = 0;
            CONFERE(!trunfo_ativo(2) && !trunfo_ativo(16) && trunfo_ativo(PERSO_CHEFE0), "Normal: so os chefes");
            int antes = 0;
            for (int d = 1; d <= N_NIVEIS; d++) {
                run.dificuldade = (uint8_t)d;
                int com = 0;
                for (int p2 = 0; p2 < N_PERSO; p2++) com += !perso_chefe(p2) && p2 != PERSO_CORINGA && trunfo_ativo(p2);
                CONFERE(com > antes, "nivel %d: mais oponentes com trunfo (%d, antes %d)", d, com, antes);
                antes = com;
            }
            run.dificuldade = 1;
            CONFERE(trunfo_ativo(2) && !trunfo_ativo(16) && !trunfo_ativo(17), "Nivel 1: o Apostador sim, o Rival nao");
            run.dificuldade = N_NIVEIS;
            int todos = 1;
            for (int p2 = 0; p2 < N_PERSO; p2++) todos &= trunfo_ativo(p2);
            CONFERE(todos, "Nivel 5: todos com trunfo");
            run.dificuldade = (uint8_t)dif0;
            // O nucleo: o Rival sempre traz o Eco (e o Gemeo a partir do 2o salao),
            // quando a dificuldade ja abriu os dois.
            int pa0 = perfil_atual;
            bool pl0 = perfis_lidos;
            perfil_t p0 = perfis[0];
            memset(&perfis[0], 0, sizeof perfis[0]);
            snprintf(perfis[0].nome, sizeof perfis[0].nome, "T");
            perfis[0].nivel = N_NIVEIS; perfil_atual = 0; perfis_lidos = true;
            no_t no = { 0 };
            no.tipo = NO_FACIL; no.perso = 16; no.semente = 99;
            for (int a = 0; a < 3; a++) {
                peca_t col[MAX_COL];
                run.andar = (uint8_t)a;
                int n = monta_oponente(&no, a, col), eco = 0, gemeo = 0;
                for (int i = 0; i < n; i++) { eco += col[i].tipo == D_ECO; gemeo += col[i].tipo == D_GEMEO; }
                CONFERE(eco >= 1 && (a == 0 || gemeo >= 1), "nucleo do Rival no salao %d: eco %d gemeo %d", a, eco, gemeo);
            }
            // No Normal, a mesa facil do primeiro salao: nada fechado e no maximo
            // dois tipos de dado especial, para qualquer oponente.
            perfis[0].nivel = 0; run.dificuldade = 0; run.andar = 0; run.passo = 3;
            for (int pp = 0; pp < N_PERSO; pp++) {
                if (perso_chefe(pp) || pp == PERSO_CORINGA) continue;
                no.perso = (uint8_t)pp; no.tipo = NO_FACIL;
                for (int sd = 1; sd <= 5; sd++) {
                    peca_t col[MAX_COL];
                    no.semente = (uint32_t)(sd * 7919 + pp);
                    int n = monta_oponente(&no, 0, col), tipos = 0, fechado = -1;
                    bool viu[D_N] = { false };
                    for (int i = 0; i < n; i++) {
                        int t = col[i].tipo;
                        if (!dado_liberado(t) && fechado < 0) fechado = t;
                        if (!so_numero(t) && !viu[t]) { viu[t] = true; tipos++; }
                    }
                    CONFERE(fechado < 0 && tipos <= 2, "%s no Normal: %d tipos especiais, fechado %d", PERSO[pp].nome, tipos, fechado);
                }
            }
            perfis[0] = p0; perfil_atual = pa0; perfis_lidos = pl0;
            run.dificuldade = (uint8_t)dif0;
            run.andar = (uint8_t)andar0;
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
        joga(1, D_CARRASCO, 4); joga(1, D_INVEJOSO, 5);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.est[0][2] == V_ANULADO && d.est[0][1] == V_VALIDO,
                "Seguranca com dois ataques");
        // Valendo no fim, o Seguranca ganha +2.
        limpa();
        joga(0, D_D4, 1); joga(0, D_SEGURANCA, 3);
        d = fim();
        CONFERE(d.total[0] == 1 + 3 + 2, "Seguranca valendo: esperava 6, deu %d", d.total[0]);
        // Sem Seguranca, o Carrasco anula o maior, como sempre.
        limpa();
        joga(0, D_D4, 2); joga(0, D_D8, 7);
        joga(1, D_CARRASCO, 4);
        d = fim();
        CONFERE(d.est[0][1] == V_ANULADO && d.total[0] == 2, "Carrasco sem Seguranca");
    }

    // ---- Fim do turno: o que nao foi lancado volta para a bolsa -----------
    {
        int cpu0 = cpu;
        cpu = 0;
        entrada(100, 100, 0);
        int j = vez = primeiro;
        jogador_t *p = &J[j];
        int bolsa0 = na_bolsa(p), mao0 = p->n_mao;
        for (int i = 0; i < 4; i++) poe_na_fila(i);
        int c1 = F[j].idx[1], c3 = F[j].idx[3];
        F[j].lancados = 1;                        // lancou um e parou
        F[j].est[0] = V_VALIDO;
        termina_turno();
        CONFERE(F[j].n == 1 && na_bolsa(p) == bolsa0 + 3 && p->n_mao == mao0 - 3,
                "devolve: fila %d, bolsa %d (era %d), mao %d (era %d)", F[j].n, na_bolsa(p), bolsa0, p->n_mao, mao0);
        CONFERE(p->onde[c1] == NA_BOLSA && p->onde[c3] == NA_BOLSA && p->onde[F[j].idx[0]] == NA_MAO,
                "devolve: os nao lancados deviam estar na bolsa");
        bool na_mao = false;
        for (int i = 0; i < p->n_mao; i++) na_mao |= p->mao[i] == c1 || p->mao[i] == c3;
        CONFERE(!na_mao, "devolve: os nao lancados ainda na mao");
        cpu = cpu0;
    }

    // ---- Acumulador: o bonus guardado vale so dentro de uma partida da run ---
    {
        int modo_antes = modo;
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[0].nome, sizeof perfis[0].nome, "T");
        perfil_atual = 0; perfis_lidos = true;
        des_dif = 0;
        des_nova_run();
        amuleto_escolhido(0);
        run.col[run.n_col++] = (peca_t){ D_ACUMULADOR, 4 };    // de uma partida antes
        int a = run.n_col - 1;
        des_comeca_partida();
        CONFERE(J[0].col[a].tipo == D_ACUMULADOR && J[0].col[a].fixo == 0,
                "Acumulador comecou a partida com +%d guardado", J[0].col[a].fixo);
        J[0].col[a].fixo = 3;                                   // guardou nesta
        venc_partida = 0;
        des_resultado();
        CONFERE(run.col[a].tipo == D_ACUMULADOR && run.col[a].fixo == 0,
                "Acumulador voltou para a run com +%d", run.col[a].fixo);
        modo = modo_antes;
    }

    // ---- O brilho: o poder armado pelo que o dado tirou ---------------------
    {
        limpa();
        joga(0, D_LASER, 5); joga(0, D_LASER, 6);
        CONFERE(poder_armado(0, 0) == 1 && poder_armado(0, 1) == 1, "Laser 5 e 6 armados");
        limpa();
        joga(0, D_LASER, 4);
        CONFERE(poder_armado(0, 0) == 0, "Laser 4 nao arma");
        limpa();
        joga(0, D_CARRASCO, 1);
        CONFERE(poder_armado(0, 0) == 2, "Carrasco 1: armado contra o dono");
        limpa();
        joga(0, D_MISERICORDIOSO, 2); joga(0, D_D4, 3); joga(0, D_D6, 4); joga(0, D_TRONO, 7);
        CONFERE(poder_armado(0, 0) == 1 && poder_armado(0, 3) == 1, "Misericordioso valendo e Trono em 4o");
        limpa();
        joga(0, D_D4, 3); joga(0, D_TRONO, 7);
        CONFERE(poder_armado(0, 1) == 0, "Trono fora do 4o nao brilha");
        limpa();
        joga(0, D_D8, 6); joga(0, D_FENIX, 2);            // cai com o 6: renasce no fim
        CONFERE(F[0].est[1] == V_ANULADO && poder_armado(0, 1) == 1, "Fenix anulada brilha");
        limpa();
        joga(0, D_CARIDOSO, 2);
        CONFERE(poder_armado(0, 0) == 0, "Caridoso 2 nao arma");
    }

    // ---- Cavalo de Troia: anulado ou roubado pelo oponente, anula os dele ---
    {
        desfecho_t d;
        // O Carrasco anula o Cavalo: caem o Carrasco e o D8 de quem atacou.
        limpa();
        joga(0, D_D4, 2); joga(0, D_CAVALO, 6);
        joga(1, D_CARRASCO, 4); joga(1, D_D8, 7);
        d = fim();
        CONFERE(d.est[0][1] == V_ANULADO && d.est[1][0] == V_ANULADO && d.est[1][1] == V_ANULADO
                && d.total[1] == 0 && d.total[0] == 2, "Cavalo x Carrasco: %d a %d", d.total[0], d.total[1]);
        // Laser e Invejoso tambem abrem o Cavalo.
        limpa();
        joga(0, D_CAVALO, 3);
        joga(1, D_LASER, 6); joga(1, D_D8, 8);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.total[1] == 0, "Cavalo x Laser: %d", d.total[1]);
        limpa();
        joga(0, D_CAVALO, 7);
        joga(1, D_INVEJOSO, 5); joga(1, D_D8, 7);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.total[1] == 0, "Cavalo x Invejoso: %d", d.total[1]);
        // Escudo e Teimoso resistem.
        limpa();
        joga(0, D_CAVALO, 6);
        joga(1, D_CARRASCO, 4); joga(1, D_ESCUDO, 5); joga(1, D_TEIMOSO, 6);
        d = fim();
        CONFERE(d.est[1][0] == V_ANULADO && d.total[1] == 11, "Cavalo: Escudo e Teimoso deviam ficar (%d)", d.total[1]);
        // Roubado: quem roubou perde os dados e o que roubou na rodada.
        limpa();
        joga(0, D_D4, 1); joga(0, D_CAVALO, 5); joga(0, D_D8, 6);
        joga(1, D_CARIDOSO, 3); joga(1, D_CARIDOSO, 4);
        d = fim();
        CONFERE(d.est[0][0] == V_ROUBADO && d.est[0][1] == V_ROUBADO && d.total[1] == 0 && d.total[0] == 6,
                "Cavalo roubado: %d a %d", d.total[0], d.total[1]);
        // O Seguranca leva o ataque no lugar: o Cavalo nem e tocado.
        limpa();
        joga(0, D_SEGURANCA, 2); joga(0, D_CAVALO, 7);
        joga(1, D_CARRASCO, 4);
        d = fim();
        CONFERE(d.est[0][1] == V_VALIDO && d.total[1] == 4 && d.total[0] == 7, "Cavalo com Seguranca: %d", d.total[1]);
        // Anulado pelo proprio dono (Berserker) ou pela queda: nada.
        limpa();
        joga(0, D_CAVALO, 3); joga(0, D_BERSERKER, 15);
        joga(1, D_D6, 5);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.total[1] == 5, "Cavalo anulado pelo Berserker abriu: %d", d.total[1]);
        // Com a Couraca, quem atacou nao perde nada.
        int modo0 = modo;
        modo = M_DESAFIO;
        limpa(); amul[0] = 0; amul[1] = 1 << A_COURACA;
        joga(0, D_CAVALO, 6);
        joga(1, D_CARRASCO, 4); joga(1, D_D8, 7);
        d = fim();
        CONFERE(d.est[0][0] == V_ANULADO && d.total[1] == 11, "Cavalo x Couraca: %d", d.total[1]);
        amul[1] = 0;
        modo = modo0;
        // Os eventos: o ataque, e o Cavalo se abrindo com os dois dados marcados.
        limpa();
        joga(0, D_CAVALO, 6);
        joga(1, D_CARRASCO, 4); joga(1, D_D8, 7);
        d = fim();
        CONFERE(n_ev == 2 && ev[1].tipo == EV_CAVALO && ev[1].mascara == 3 && ev[1].alvo_j == 1,
                "Cavalo: eventos (%d)", n_ev);
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

    // ---- Invejoso: 5 ou 6 anula o maior do oponente; 1 ou 2, os seus dois
    // menores (ele incluido); 3 ou 4, nada ---------------------------------
    limpa();
    primeiro = 1;
    joga(1, D_INVEJOSO, 5);
    joga(0, D_D8, 7);
    joga(0, D_D8, 8);
    d = fim();
    CONFERE(d.est[0][1] == V_ANULADO && d.est[0][0] == V_VALIDO, "Invejoso 5 devia anular o 8 do oponente");
    CONFERE(d.est[1][0] == V_VALIDO && d.total[1] == 5 && d.total[0] == 7, "Invejoso 5: %d x %d", d.total[1], d.total[0]);
    limpa();
    joga(1, D_D4, 1); joga(1, D_INVEJOSO, 2); joga(1, D_D6, 3);
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.est[1][0] == V_ANULADO && d.est[1][1] == V_ANULADO && d.est[1][2] == V_VALIDO && d.est[0][0] == V_VALIDO,
            "Invejoso 2 devia anular o 1 e a si mesmo");
    CONFERE(poder_armado(1, 1) == 2, "Invejoso 2: armado contra o dono");
    limpa();
    joga(1, D_INVEJOSO, 4);            // com 3 ou 4 nao age
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.est[0][0] == V_VALIDO && d.est[1][0] == V_VALIDO, "Invejoso com 4 nao devia anular nada");

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
    CONFERE(d.est[0][0] == V_ANULADO && d.est[0][1] == V_VALIDO && d.est[0][2] == V_VALIDO,
            "Carrasco com 1 valendo e o menor: devia anular a si mesmo");

    // ---- Fichas: comecou a rodada na frente, +4 ---------------------------
    limpa(); fichas_ini[0] = 40; fichas_ini[1] = 30;
    joga(0, D_FICHAS, 3);
    d = fim();
    CONFERE(d.total[0] == 7, "Fichas na frente: esperava 7, deu %d", d.total[0]);
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
    limpa();
    joga(0, D_FOGO, 1); joga(0, D_D6, 2); joga(0, D_D8, 6);
    d = fim();
    CONFERE(d.total[0] == 1 + 2 + 6 && d.queima[0][2], "Fogo com 1: sem +10, e o 6 queima (deu %d)", d.total[0]);

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

    // ---- Ventania: rola de novo o primeiro dado e refaz a sequencia --------
    limpa();
    joga(0, D_D6, 3); joga(0, D_D8, 6);
    joga_com(0, D_VENTANIA, 7, sempre_dois);
    CONFERE(F[0].valor[0] == 2 && F[0].valor[1] == 6, "Ventania devia rolar o primeiro (e nao o de antes)");
    CONFERE(F[0].valor[2] == 7 && F[0].est[2] == V_VALIDO, "Ventania nao rola a si mesmo");
    d = fim();
    CONFERE(d.total[0] == 2 + 6 + 7, "Ventania: esperava 15, deu %d", d.total[0]);
    // O primeiro rolado de novo pode fazer o par voltar a valer.
    limpa();
    joga(0, D_D6, 5);
    joga(0, D_D8, 3);                  // 3 < 5: caem os dois
    joga_com(0, D_VENTANIA, 8, sempre_dois);
    CONFERE(F[0].valor[0] == 2 && F[0].est[0] == V_VALIDO && F[0].est[1] == V_VALIDO,
            "Ventania: 2 e 3 deviam valer de novo");
    d = fim();
    CONFERE(d.total[0] == 2 + 3 + 8, "Ventania refeita: esperava 13, deu %d", d.total[0]);
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
    joga(0, D_INVEJOSO, 5);
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

    // Hora da verdade: o ultimo dado da rodada e decisivo quando a face muda
    // quem leva o pote. Quem joga primeiro, um dado do meio da fila, um jogo
    // ja decidido ou quase certo nao contam. Imaginar as faces nao mexe na
    // mesa nem no sorteio da partida.
    modo = M_UM; rodada = 3; pote = 20; J[0].fichas = J[1].fichas = 50;
    limpa();
    primeiro = 1;
    joga(1, D_D6, 3); joga(1, D_D8, 5); joga(1, D_D8, 6);         // ele: 14
    joga(0, D_D4, 4); joga(0, D_D6, 5);                            // voce: 9; o 6 vence, o 5 empata
    F[0].tipo[2] = D_D6; F[0].n = 3;
    sem_antes = semente;
    memcpy(f_antes, F, sizeof F);
    CONFERE(dado_decisivo(0, 2) == 1, "Hora: o D6 que vence com 6 devia decidir a rodada");
    CONFERE(semente == sem_antes, "Hora: imaginar as faces mexeu no sorteio da partida");
    CONFERE(!memcmp(f_antes, F, sizeof F), "Hora: imaginar as faces mexeu na mesa");
    CONFERE(dado_decisivo(1, 2) == 0, "Hora: quem joga primeiro nao decide a rodada");
    rodada = MAX_RODADAS;                                          // a ultima: decide a partida
    CONFERE(dado_decisivo(0, 2) == 2, "Hora: na ultima rodada devia decidir a partida");
    rodada = 3;
    F[0].tipo[3] = D_D6; F[0].n = 4;
    CONFERE(dado_decisivo(0, 2) == 0, "Hora: um dado do meio da fila nao e o ultimo");
    limpa();
    primeiro = 1;
    joga(1, D_D6, 4); joga(1, D_D6, 5);                            // ele: 9
    joga(0, D_D8, 6); joga(0, D_D12, 7); joga(0, D_D8, 7);        // voce: 20
    F[0].tipo[3] = D_D6; F[0].n = 4;
    CONFERE(dado_decisivo(0, 3) == 0, "Hora: com 20 contra 9 nao ha o que decidir");
    limpa();
    primeiro = 1;
    joga(1, D_D12, 9); joga(1, D_D12, 9); joga(1, D_D12, 10);     // ele: 28
    joga(0, D_D4, 4); joga(0, D_D6, 5);                            // voce: 9; so o 20 vence
    F[0].tipo[2] = D_D20; F[0].n = 3;
    CONFERE(dado_decisivo(0, 2) == 0, "Hora: 1 chance em 20 nao devia prender a mesa");

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
