// Teste do modo online: duas copias do jogo em dois processos (fork), ligadas
// por canos como os dois navegadores pelo servidor. Uma anda a 60 quadros por
// segundo; a outra varia de 30 a 144, como um monitor diferente. Cada lado
// aperta teclas ao acaso; as da vez viajam para o outro, como no navegador.
// Os marcos da partida (rede_marco: o estado das regras a cada rodada e a
// cada turno) dos dois lados tem que ser os mesmos, na mesma ordem, e a
// partida tem que terminar nos dois. E o que garante que o placar de um
// computador e o placar do outro.
//
//   online [salas] [quadros]
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
static int64_t relogio = 11;
int64_t esp_timer_get_time(void) { return relogio; }
void som_toca(int s) { (void)s; }
void som_toca_tom(int s, int t) { (void)s; (void)t; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }
void som_rufo_moeda(void) {}
void som_giro(bool on) { (void)on; }
void som_suspense(bool on) { (void)on; }
void som_cadeia(bool on) { (void)on; }
void som_chefe(int q, bool t) { (void)q; (void)t; }
static int volume_[SOM_CANAIS] = { SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS };
void som_volume(int c, int n) { if (c >= 0 && c < SOM_CANAIS) volume_[c] = n; }
int som_volume_de(int c) { return c >= 0 && c < SOM_CANAIS ? volume_[c] : 0; }
#include "salva_mem.h"

// A rede: as teclas vao pelo cano; os marcos ficam anotados.
#define MAX_MARCOS 4096
typedef struct { int32_t i; uint32_t h; } marco_t;
static marco_t marcos[MAX_MARCOS];
static int n_marcos, fd_in = -1, fd_out = -1, fim_f0, fim_f1;   // as fichas no ultimo marco
bool rede_disponivel(void) { return true; }
void rede_cria(void) {}
void rede_entra(const char *c) { (void)c; }
void rede_envia_tecla(int k)
{
    int32_t v = k;
    if (write(fd_out, &v, 4) != 4) { /* o outro ja terminou */ }
}
void rede_sai(void) {}
int rede_estado(void) { return REDE_JOGANDO; }
const char *rede_codigo(void) { return "TEST"; }
const char *rede_erro(void) { return ""; }
void rede_marco(int i, uint32_t h)
{
    if (n_marcos < MAX_MARCOS) marcos[n_marcos++] = (marco_t){ i, h };
    fim_f0 = J[0].fichas; fim_f1 = J[1].fichas;
}
const char *rede_convite(void) { return ""; }
void rede_copia_convite(void) {}
int rede_convite_copiado(void) { return 0; }

static uint32_t r32(uint32_t *s) { *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s; }

// Um lado da sala: joga ate a partida acabar (a revanche comecar).
static bool joga(int eu, uint32_t sem, int quadros)
{
    natal_forcado = 0;                       // a mesma epoca em qualquer dia do ano
    dado_online_comeca(sem, eu);
    uint32_t rs = 0x9E3779B9u ^ sem ^ (uint32_t)eu * 7919u;
    for (int q = 0; q < quadros; q++) {
        int32_t v;
        while (read(fd_in, &v, 4) == 4) dado_rede_tecla(v);
        if (r32(&rs) % 9 == 0) {             // de vez em quando, uma tecla ao acaso
            static const int K[] = { KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_LEFT, KEY_RIGHT,
                                     KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_TAB, KEY_BKSP, KEY_ENTER };
            key_event_t e = { 0 };
            e.key = K[r32(&rs) % 12];
            dado_tecla(&e);
        }
        dado_desenha();
        float dt = eu == 0 ? 1 / 60.0f : 1 / 144.0f + (r32(&rs) % 1000) / 1000.0f * (1 / 30.0f - 1 / 144.0f);
        dado_passo(dt);
        relogio += (int64_t)(dt * 1e6f);
#ifdef DESSINCRONIZA
        if (eu == 1 && q == 3000) rnd();     // so para ver o teste falhar: um sorteio a mais de um lado
#endif
        if (partidas >= 1) return true;      // a partida acabou e a revanche comecou
    }
    return false;
}

typedef struct { pid_t pid; int res; } lado_t;
static lado_t abre_lado(int eu, uint32_t sem, int quadros, int cano_in, int cano_out, int fecha1, int fecha2)
{
    int res[2];
    if (pipe(res)) { perror("pipe"); exit(2); }
    fflush(stdout);
    pid_t p = fork();
    if (p < 0) { perror("fork"); exit(2); }
    if (p == 0) {
        signal(SIGPIPE, SIG_IGN);
        close(res[0]); close(fecha1); close(fecha2);
        fd_in = cano_in; fd_out = cano_out;
        fcntl(fd_in, F_SETFL, O_NONBLOCK);
        bool fim = joga(eu, sem, quadros);
        int32_t cab[4] = { fim, n_marcos, fim_f0, fim_f1 };
        if (write(res[1], cab, sizeof cab) != sizeof cab) _exit(3);
        if (n_marcos && write(res[1], marcos, sizeof(marco_t) * (size_t)n_marcos) != (ssize_t)(sizeof(marco_t) * (size_t)n_marcos))
            _exit(3);
        _exit(0);
    }
    close(res[1]);
    return (lado_t){ p, res[0] };
}

static bool le_tudo(int fd, void *buf, size_t n)
{
    char *p = buf;
    while (n) {
        ssize_t k = read(fd, p, n);
        if (k <= 0) return false;
        p += k; n -= (size_t)k;
    }
    return true;
}

typedef struct { int32_t fim, n, f0, f1; marco_t m[MAX_MARCOS]; } resultado_t;
static resultado_t res_lado[2];

int main(int argc, char **argv)
{
    static const uint32_t SEMENTES[] = { 7, 2026, 31337, 424242 };
    int salas = argc > 1 ? atoi(argv[1]) : 2, quadros = argc > 2 ? atoi(argv[2]) : 150000;
    if (salas < 1) salas = 1;
    if (salas > 4) salas = 4;
    int falhas = 0;
    lado_t L[4][2];
    for (int s = 0; s < salas; s++) {                 // as salas rodam ao mesmo tempo
        int a2b[2], b2a[2];
        if (pipe(a2b) || pipe(b2a)) { perror("pipe"); return 2; }
        L[s][0] = abre_lado(0, SEMENTES[s], quadros, b2a[0], a2b[1], a2b[0], b2a[1]);
        L[s][1] = abre_lado(1, SEMENTES[s], quadros, a2b[0], b2a[1], b2a[0], a2b[1]);
        close(a2b[0]); close(a2b[1]); close(b2a[0]); close(b2a[1]);
    }
    for (int s = 0; s < salas; s++) {
        for (int j = 0; j < 2; j++) {
            resultado_t *r = &res_lado[j];
            memset(r, 0, sizeof *r);
            if (!le_tudo(L[s][j].res, r, sizeof(int32_t) * 4) || r->n < 0 || r->n > MAX_MARCOS
                || (r->n && !le_tudo(L[s][j].res, r->m, sizeof(marco_t) * (size_t)r->n))) r->fim = -1;
            close(L[s][j].res);
            waitpid(L[s][j].pid, NULL, 0);
        }
        resultado_t *a = &res_lado[0], *b = &res_lado[1];
        int igual = 0;
        while (igual < a->n && igual < b->n && a->m[igual].i == b->m[igual].i && a->m[igual].h == b->m[igual].h) igual++;
        bool ok = a->fim == 1 && b->fim == 1 && a->n == b->n && igual == a->n && a->n >= 3
               && a->f0 == b->f0 && a->f1 == b->f1;
        printf("sala %u: %s  marcos %d/%d (iguais ate o %d), fichas %d x %d / %d x %d%s\n", SEMENTES[s],
               ok ? "ok   " : "FALHOU", a->n, b->n, igual, a->f0, a->f1, b->f0, b->f1,
               a->fim == 1 && b->fim == 1 ? "" : "  (a partida nao terminou nos dois)");
        if (!ok) falhas++;
    }
    if (falhas) { printf("\n%d sala(s) com os dois lados diferentes\n", falhas); return 1; }
    printf("\nos dois lados iguais em %d sala(s)\n", salas);
    return 0;
}
