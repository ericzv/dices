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
static int desvio_min;                    // o relogio acertado: minutos somados a hora do PC

void *sis_aloca(size_t n) { return calloc(1, n); }
void sis_brilho(int pct) { pc_brilho = pct; }
void sis_volume(int pct) { pc_volume = pct; pc_volume_mudou(pct); }
void sis_gira(bool g) { pc_gira = g; }
int  sis_toque_estado(void) { return 1; }
int  sis_painel(void) { return 0; }
void sis_troca_painel(int v) { (void)v; }
void sis_reinicia(void) { }

bool sis_hora(int *h, int *m)
{
    time_t t = time(NULL);
    struct tm *l = localtime(&t);
    int min = (l->tm_hour * 60 + l->tm_min + desvio_min) % 1440;
    if (min < 0) min += 1440;
    *h = min / 60;
    *m = min % 60;
    return true;
}

void sis_acerta_hora(int h, int m)
{
    int ah, am;
    desvio_min = 0;
    sis_hora(&ah, &am);
    desvio_min = (h * 60 + m) - (ah * 60 + am);
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
