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

static int falhas;
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

static desfecho_t fim(void)
{
    desfecho_t d;
    calcula_desfecho(&d, true);
    return d;
}

int main(void)
{
    desfecho_t d;

    // Espinhoso: tira do rival o valor que tirou, e ainda soma para o dono.
    limpa();
    joga(0, D_MARTELO, 3);
    joga(1, D_D6, 6);
    d = fim();
    CONFERE(d.total[1] == 3, "Espinhoso 3: rival devia ficar com 3, ficou %d", d.total[1]);
    CONFERE(d.total[0] == 3, "Espinhoso 3: dono devia fazer 3, fez %d", d.total[0]);

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

    // Nem por ataque: o Carrasco com 4 procura outro alvo.
    limpa();
    joga(1, D_EGOISTA, 8);
    joga(0, D_CARRASCO, 4);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Carrasco nao devia anular o Egoista");
    CONFERE(d.total[1] == 8, "Egoista atacado: esperava 8, deu %d", d.total[1]);

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

    if (falhas) { printf("%d falhas\n", falhas); return 1; }
    puts("regras ok");
    return 0;
}
