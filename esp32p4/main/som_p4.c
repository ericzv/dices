// O som do jogo no aparelho: o mesmo contrato do PC (hal/som.h, mais a
// trilha), com os sons ja prontos em sons.bin (ferramentas/gera_sons.c) e um
// misturador simples que a tarefa de audio chama no segundo nucleo.
//
// O comportamento segue desktop/som.c: quatro copias de cada efeito podem
// tocar juntas, os baques variam um pouco de tom, o rufar e o giro entram e
// saem com rampa e abaixam a trilha enquanto tocam.
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "hal/som.h"
#include "som_p4.h"

static const char *TAG = "som";

extern const uint8_t sons_bin_inicio[] asm("_binary_sons_bin_start");

#define N_TRECHOS (SOM_N + 3)
#define T_RUFO    SOM_N
#define T_GIRO    (SOM_N + 1)
#define T_TRILHA  (SOM_N + 2)
#define VOZES_POR 4
#define N_VOZES   16

static int16_t *pcm[N_TRECHOS];
static uint32_t tam[N_TRECHOS];
static bool pronto, falhou, ligado = true, musica = true;

typedef struct {
    int s;                                 // qual efeito (-1: livre)
    uint32_t pos, passo;                   // 16.16
    uint32_t ordem;                        // para achar a mais velha
} voz_t;
static voz_t vozes[N_VOZES];
static uint32_t contador;
static portMUX_TYPE trava = portMUX_INITIALIZER_UNLOCKED;

// O que o laco principal decide e o misturador le (volumes em 1/256).
static volatile int vol_trilha, vol_rufo, vol_giro;
static volatile bool toca_rufo, toca_giro;
static uint32_t pos_rufo, pos_giro, pos_trilha;
static volatile bool reinicia_rufo, reinicia_giro;

// Estado do lado do jogo (como no PC).
static bool rufando, girando;
static float f_rufo, f_giro, duck = 1;

// ---------------------------------------------------------------------------
// sons.bin
// ---------------------------------------------------------------------------
static const int16_t PASSOS[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80,
    88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635,
    13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};
static const int8_t INDICES[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

static void decodifica(const uint8_t *d, uint32_t n, int16_t *x)
{
    int pred = 0, idx = 0;
    for (uint32_t i = 0; i < n; i++) {
        int nib = i & 1 ? d[i / 2] >> 4 : d[i / 2] & 15;
        int passo = PASSOS[idx], v = passo >> 3;
        if (nib & 4) v += passo;
        if (nib & 2) v += passo >> 1;
        if (nib & 1) v += passo >> 2;
        pred += nib & 8 ? -v : v;
        if (pred > 32767) pred = 32767;
        if (pred < -32768) pred = -32768;
        idx += INDICES[nib];
        if (idx < 0) idx = 0;
        if (idx > 88) idx = 88;
        x[i] = (int16_t)pred;
    }
}

static uint32_t u32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

bool som_carrega(void)
{
    const uint8_t *d = sons_bin_inicio;
    int n = d[6] | d[7] << 8;
    if (memcmp(d, "DSOM", 4) || (d[4] | d[5] << 8) != 1 || n != N_TRECHOS) {
        ESP_LOGE(TAG, "sons.bin nao confere (esperava %d sons, veio %d): rode ferramentas/gera_sons", N_TRECHOS, n);
        falhou = true;
        return false;
    }
    const uint8_t *dados = d + 12 + 8 * n;
    size_t total = 0;
    for (int i = 0; i < n; i++) {
        tam[i] = u32(d + 12 + 4 * i);
        pcm[i] = heap_caps_malloc(tam[i] * 2, MALLOC_CAP_SPIRAM);
        if (!pcm[i]) { falhou = true; return false; }
        decodifica(dados, tam[i], pcm[i]);
        dados += u32(d + 12 + 4 * n + 4 * i);
        total += tam[i] * 2;
    }
    for (int v = 0; v < N_VOZES; v++) vozes[v].s = -1;
    ESP_LOGI(TAG, "%d sons prontos (%u KB)", n, (unsigned)(total / 1024));
    pronto = true;
    return true;
}

// ---------------------------------------------------------------------------
// Misturador (tarefa de audio)
// ---------------------------------------------------------------------------
void som_mistura(int16_t *saida, int n)
{
    if (!pronto) { memset(saida, 0, (size_t)n * 2); return; }
    static int32_t acc[512];
    if (n > 512) n = 512;
    memset(acc, 0, (size_t)n * 4);

    int vt = vol_trilha;
    const int16_t *t = pcm[T_TRILHA];
    for (int i = 0; i < n; i++) {
        if (vt) acc[i] += t[pos_trilha] * vt >> 8;
        if (++pos_trilha >= tam[T_TRILHA]) pos_trilha = 0;
    }
    if (reinicia_rufo) { pos_rufo = 0; reinicia_rufo = false; }
    if (reinicia_giro) { pos_giro = 0; reinicia_giro = false; }
    int vr = vol_rufo, vg = vol_giro;
    if (toca_rufo)
        for (int i = 0; i < n && pos_rufo < tam[T_RUFO]; i++) acc[i] += pcm[T_RUFO][pos_rufo++] * vr >> 8;
    if (toca_giro)
        for (int i = 0; i < n && pos_giro < tam[T_GIRO]; i++) acc[i] += pcm[T_GIRO][pos_giro++] * vg >> 8;

    voz_t local[N_VOZES];
    portENTER_CRITICAL(&trava);
    memcpy(local, vozes, sizeof local);
    portEXIT_CRITICAL(&trava);
    for (int v = 0; v < N_VOZES; v++) {
        voz_t *z = &local[v];
        if (z->s < 0) continue;
        const int16_t *a = pcm[z->s];
        uint32_t fim = tam[z->s] << 16;
        for (int i = 0; i < n && z->pos < fim; i++) {
            uint32_t k = z->pos >> 16, f = z->pos & 0xFFFF;
            int x = a[k];
            if (f && k + 1 < tam[z->s]) x += (int)(((int64_t)(a[k + 1] - x) * f) >> 16);
            acc[i] += x;
            z->pos += z->passo;
        }
        if (z->pos >= fim) z->s = -1;
    }
    portENTER_CRITICAL(&trava);
    for (int v = 0; v < N_VOZES; v++)
        if (vozes[v].ordem == local[v].ordem && vozes[v].s >= 0) {    // ninguem trocou a voz no meio
            vozes[v].pos = local[v].pos;
            if (local[v].s < 0) vozes[v].s = -1;
        }
    portEXIT_CRITICAL(&trava);

    for (int i = 0; i < n; i++) {
        int32_t x = acc[i];
        saida[i] = (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x);
    }
}

// ---------------------------------------------------------------------------
// hal/som.h
// ---------------------------------------------------------------------------
void som_toca(int s)
{
    if (!pronto || !ligado || s < 0 || s >= SOM_N) return;
    uint32_t passo = 1u << 16;
    if (s == SOM_BATE || s == SOM_POUSA || s == SOM_FICHA)          // baques nunca soam iguais
        passo = (uint32_t)((0.9f + 0.2f * (float)rand() / (float)RAND_MAX) * 65536);
    portENTER_CRITICAL(&trava);
    int iguais = 0, velha_igual = -1, livre = -1, velha = 0;
    for (int v = 0; v < N_VOZES; v++) {
        voz_t *z = &vozes[v];
        if (z->s == s) {
            iguais++;
            if (velha_igual < 0 || z->ordem < vozes[velha_igual].ordem) velha_igual = v;
        }
        if (z->s < 0 && livre < 0) livre = v;
        if (z->ordem < vozes[velha].ordem) velha = v;
    }
    int v = iguais >= VOZES_POR ? velha_igual : livre >= 0 ? livre : velha;
    vozes[v] = (voz_t){ .s = s, .pos = 0, .passo = passo, .ordem = ++contador };
    portEXIT_CRITICAL(&trava);
}

bool som_liga(bool on) { ligado = on && pronto; return ligado; }
bool som_ligado(void) { return ligado; }
bool som_falhou(void) { return falhou; }

void som_rufo(bool on)
{
    if (!pronto) return;
    if (on && !rufando) { f_rufo = 0; vol_rufo = 0; reinicia_rufo = true; toca_rufo = true; }
    rufando = on;
}

void som_giro(bool on)
{
    if (!pronto) return;
    if (on && !girando) { f_giro = 0; vol_giro = 0; reinicia_giro = true; toca_giro = true; }
    girando = on;
}

bool som_musica(bool on) { musica = on; return musica; }
bool som_musica_ligada(void) { return musica; }

// Uma vez por quadro, como o som_atualiza do PC.
void som_atualiza(float dt)
{
    if (!pronto) return;
    float alvo = rufando && ligado ? 1 : 0;
    f_rufo += (alvo - f_rufo) * fminf(1, dt / (rufando ? 0.06f : 0.12f));
    vol_rufo = (int)(f_rufo * 0.8f * 256);
    if (!rufando && f_rufo < 0.01f) toca_rufo = false;
    float ag = girando && ligado ? 1 : 0;
    f_giro += (ag - f_giro) * fminf(1, dt / (girando ? 0.02f : 0.08f));
    vol_giro = (int)(f_giro * 256);
    if (!girando && f_giro < 0.01f) toca_giro = false;
    float d = rufando || girando ? 0.45f : 1;
    duck += (d - duck) * fminf(1, dt / 0.25f);
    vol_trilha = (int)(0.35f * duck * (ligado && musica ? 1 : 0) * 256);
}
