// O "aparelho" do simulador: o que o console e o jogo pedem a plataforma,
// feito no PC. O salvamento vai para arquivos numa pasta (ou fica so na
// memoria, se nenhuma pasta for dada).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sistema.h"
#include "sistema_pc.h"
#include "ui/gfx.h"
#include "hal/display.h"
#include "hal/salva.h"

static uint16_t fb_jogo[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb_jogo; }

int64_t esp_timer_get_time(void)
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (int64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000;
}

int pc_brilho = 80, pc_volume = 70;
bool pc_gira;
static long desvio_s;                     // o relogio acertado: segundos somados a hora do PC
static bool acertado;

void *sis_aloca(size_t n) { return calloc(1, n); }
void sis_brilho(int pct) { pc_brilho = pct; }
void sis_volume(int pct) { pc_volume = pct; pc_volume_mudou(pct); }
void sis_gira(bool g) { pc_gira = g; }
int  sis_toque_estado(void) { return 1; }
void sis_desliga(void) { printf("desligado\n"); exit(0); }
int  sis_painel(void) { return 0; }
bool sis_tela_pode_trocar(void) { return true; }
void sis_tela_troca(void) { printf("trocaria a versao da tela\n"); }
bool sis_tela_em_teste(void) { return getenv("TELA_EM_TESTE") != NULL; }
void sis_tela_mantem(void) { }
void sis_tela_desfaz(void) { printf("voltaria a tela anterior\n"); }
void sis_reinicia(void) { }
// Uma bateria de mentira, para ver o indicador.
bool sis_bateria(int *pct, int *mv, bool *carregando)
{
    *pct = 76;
    *mv = 3890;
    *carregando = false;
    return true;
}
// No PC o console amplia a mesa no proprio quadro.
bool sis_mostra_jogo(const uint16_t *px, int w, int h, int k, int x, int y, const uint16_t *lut)
{
    (void)px; (void)w; (void)h; (void)k; (void)x; (void)y; (void)lut;
    return false;
}

// O relogio do PC, com o que se acertou nos Ajustes somado. RELOGIO_SEM_DATA
// faz de conta que o aparelho ainda nao sabe o dia.
bool sis_data_hora(sis_data_t *d)
{
    time_t t = time(NULL) + desvio_s;
    struct tm *l = localtime(&t);
    bool sem_data = !acertado && getenv("RELOGIO_SEM_DATA");
    *d = (sis_data_t){ sem_data ? 0 : l->tm_year + 1900, l->tm_mon + 1, l->tm_mday, l->tm_hour, l->tm_min };
    return true;
}

void sis_acerta_data_hora(const sis_data_t *d)
{
    time_t agora = time(NULL);
    struct tm t = *localtime(&agora);
    t.tm_year = d->ano - 1900;
    t.tm_mon = d->mes - 1;
    t.tm_mday = d->dia;
    t.tm_hour = d->h;
    t.tm_min = d->m;
    t.tm_isdst = -1;
    desvio_s = (long)(mktime(&t) - agora);
    acertado = true;
}

int sis_info(char *buf, int n)
{
    return snprintf(buf, (size_t)n,
                    "Placa: simulador no PC\n"
                    "Tela: 1280 x 800 (o mouse faz o dedo)\n"
                    "Esc: botão de início  ·  F12: foto da tela\n");
}

// ---------------------------------------------------------------------------
// Salvamento: "<pasta>/<espaco>.<chave>.sav", ou na memoria
// ---------------------------------------------------------------------------
static char espaco[24] = "console";
static const char *pasta;
static struct { char chave[48]; char *texto; } mem[64];

void sis_espaco(const char *nome) { snprintf(espaco, sizeof espaco, "%s", nome); }
void pc_pasta_salva(const char *p) { pasta = p; }

static void nome_de(const char *chave, char *out, size_t n) { snprintf(out, n, "%s.%s", espaco, chave); }

bool salva_grava(const char *chave, const char *texto)
{
    char k[48];
    nome_de(chave, k, sizeof k);
    if (pasta) {
        char p[512];
        snprintf(p, sizeof p, "%s/%s.sav", pasta, k);
        FILE *f = fopen(p, "wb");
        if (!f) return false;
        fputs(texto, f);
        fclose(f);
        return true;
    }
    int livre = -1;
    for (int i = 0; i < 64; i++) {
        if (!strcmp(mem[i].chave, k)) { free(mem[i].texto); mem[i].texto = strdup(texto); return true; }
        if (!mem[i].chave[0] && livre < 0) livre = i;
    }
    if (livre < 0) return false;
    snprintf(mem[livre].chave, sizeof mem[livre].chave, "%s", k);
    mem[livre].texto = strdup(texto);
    return true;
}

int salva_le(const char *chave, char *buf, int n)
{
    char k[48];
    nome_de(chave, k, sizeof k);
    buf[0] = 0;
    if (pasta) {
        char p[512];
        snprintf(p, sizeof p, "%s/%s.sav", pasta, k);
        FILE *f = fopen(p, "rb");
        if (!f) return 0;
        int l = (int)fread(buf, 1, (size_t)n - 1, f);
        fclose(f);
        buf[l] = 0;
        return l;
    }
    for (int i = 0; i < 64; i++)
        if (!strcmp(mem[i].chave, k)) {
            snprintf(buf, (size_t)n, "%s", mem[i].texto);
            return (int)strlen(buf);
        }
    return 0;
}

void salva_apaga(const char *chave)
{
    char k[48];
    nome_de(chave, k, sizeof k);
    if (pasta) {
        char p[512];
        snprintf(p, sizeof p, "%s/%s.sav", pasta, k);
        remove(p);
        return;
    }
    for (int i = 0; i < 64; i++)
        if (!strcmp(mem[i].chave, k)) { free(mem[i].texto); mem[i].texto = NULL; mem[i].chave[0] = 0; }
}
