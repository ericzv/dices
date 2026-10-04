// Gera esp32p4/main/sons.bin: os efeitos, o rufar, o giro da roleta e a
// trilha do Dado em Casa, sintetizados pelo mesmo desktop/som.c do PC.
//
// No PC o som e composto na hora em que o jogo abre. No ESP32-P4 isso levaria
// uns 30 s de processador e mais memoria do que sobra (so a trilha, em ponto
// flutuante, ocupa 27 MB enquanto e mixada). Entao a sintese roda aqui, no
// computador, uma vez; o firmware so descompacta.
//
// Formato (tudo little-endian):
//   "DSOM", u16 versao (1), u16 n, u32 taxa
//   u32 amostras[n], u32 bytes[n]
//   n trechos em IMA ADPCM de 4 bits, mono, cada um comecando em 0
// Ordem: os SOM_N efeitos de hal/som.h, o rufar, o giro e a trilha (os dois
// canais da trilha somados: o alto-falante do aparelho e um so).
//
// Compilar e rodar (da raiz do repositorio), numa linha so:
//   cc -O2 -Iesp32p4/ferramentas -Isrc -Idesktop esp32p4/ferramentas/gera_sons.c
//      desktop/som.c -lm -o /tmp/gera_sons && /tmp/gera_sons esp32p4/main/sons.bin
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "hal/som.h"

void som_inicia(void);

#define MAX_TRECHOS (SOM_N + 3)
static int16_t *trecho[MAX_TRECHOS];
static uint32_t amostras[MAX_TRECHOS];
static int n_trechos;

bool IsAudioDeviceReady(void) { return true; }

Sound LoadSoundFromWave(Wave w)
{
    Sound s = { n_trechos, w.frameCount };
    if (n_trechos >= MAX_TRECHOS || w.channels != 1 || w.sampleSize != 16) {
        fprintf(stderr, "som inesperado (%u canais, %u bits)\n", w.channels, w.sampleSize);
        exit(1);
    }
    trecho[n_trechos] = malloc(w.frameCount * 2);
    memcpy(trecho[n_trechos], w.data, w.frameCount * 2);
    amostras[n_trechos++] = w.frameCount;
    return s;
}

// A trilha chega como um WAV estereo de 16 bits em memoria.
Music LoadMusicStreamFromMemory(const char *tipo, const unsigned char *d, int n)
{
    (void)tipo;
    Music m = { n_trechos, 0, true };
    uint32_t bytes = (uint32_t)d[40] | (uint32_t)d[41] << 8 | (uint32_t)d[42] << 16 | (uint32_t)d[43] << 24;
    if (n < 44 || memcmp(d, "RIFF", 4) || d[22] != 2 || bytes + 44 > (uint32_t)n) {
        fprintf(stderr, "trilha inesperada\n");
        exit(1);
    }
    uint32_t q = bytes / 4;
    int16_t *mono = malloc(q * 2);
    for (uint32_t i = 0; i < q; i++) {
        const unsigned char *p = d + 44 + i * 4;
        int l = (int16_t)(p[0] | p[1] << 8), r = (int16_t)(p[2] | p[3] << 8);
        mono[i] = (int16_t)((l + r) / 2);
    }
    trecho[n_trechos] = mono;
    amostras[n_trechos++] = q;
    m.frameCount = q;
    return m;
}

Sound LoadSoundAlias(Sound s) { return s; }
void  UnloadSoundAlias(Sound s) { (void)s; }
void  UnloadSound(Sound s) { (void)s; }
void  UnloadMusicStream(Music m) { (void)m; }
void  PlayMusicStream(Music m) { (void)m; }
void  UpdateMusicStream(Music m) { (void)m; }
void  SetMusicVolume(Music m, float v) { (void)m; (void)v; }
bool  IsSoundPlaying(Sound s) { (void)s; return false; }
void  SetSoundVolume(Sound s, float v) { (void)s; (void)v; }
void  SetSoundPitch(Sound s, float p) { (void)s; (void)p; }
void  PlaySound(Sound s) { (void)s; }
void  StopSound(Sound s) { (void)s; }

// ---------------------------------------------------------------------------
// IMA ADPCM
// ---------------------------------------------------------------------------
static const int16_t PASSOS[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80,
    88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635,
    13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};
static const int8_t INDICES[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

static uint32_t codifica(const int16_t *x, uint32_t n, uint8_t *saida)
{
    int pred = 0, idx = 0;
    memset(saida, 0, (n + 1) / 2);
    for (uint32_t i = 0; i < n; i++) {
        int passo = PASSOS[idx], dif = x[i] - pred, nib = 0;
        if (dif < 0) { nib = 8; dif = -dif; }
        int v = passo >> 3;
        if (dif >= passo) { nib |= 4; dif -= passo; v += passo; }
        passo >>= 1;
        if (dif >= passo) { nib |= 2; dif -= passo; v += passo; }
        passo >>= 1;
        if (dif >= passo) { nib |= 1; v += passo; }
        pred += nib & 8 ? -v : v;
        if (pred > 32767) pred = 32767;
        if (pred < -32768) pred = -32768;
        idx += INDICES[nib];
        if (idx < 0) idx = 0;
        if (idx > 88) idx = 88;
        saida[i / 2] |= (uint8_t)(i & 1 ? nib << 4 : nib);
    }
    return (n + 1) / 2;
}

static void u16(FILE *f, uint16_t v) { fputc(v & 255, f); fputc(v >> 8, f); }
static void u32(FILE *f, uint32_t v) { for (int i = 0; i < 4; i++) fputc((v >> (8 * i)) & 255, f); }

int main(int argc, char **argv)
{
    const char *saida = argc > 1 ? argv[1] : "sons.bin";
    som_inicia();
    if (n_trechos != SOM_N + 3) {
        fprintf(stderr, "esperava %d sons, vieram %d\n", SOM_N + 3, n_trechos);
        return 1;
    }
    uint8_t *cod[MAX_TRECHOS];
    uint32_t bytes[MAX_TRECHOS], total = 0;
    for (int i = 0; i < n_trechos; i++) {
        cod[i] = malloc(amostras[i] / 2 + 1);
        bytes[i] = codifica(trecho[i], amostras[i], cod[i]);
        total += bytes[i];
    }
    FILE *f = fopen(saida, "wb");
    if (!f) { perror(saida); return 1; }
    fwrite("DSOM", 1, 4, f);
    u16(f, 1);
    u16(f, (uint16_t)n_trechos);
    u32(f, 44100);
    for (int i = 0; i < n_trechos; i++) u32(f, amostras[i]);
    for (int i = 0; i < n_trechos; i++) u32(f, bytes[i]);
    for (int i = 0; i < n_trechos; i++) fwrite(cod[i], 1, bytes[i], f);
    fclose(f);
    uint32_t am = 0;
    for (int i = 0; i < n_trechos; i++) am += amostras[i];
    printf("%s: %d sons, %.1f s de audio, %u KB (%.1f MB descompactado)\n", saida, n_trechos, am / 44100.0,
           (unsigned)(total / 1024), am * 2 / 1048576.0);
    return 0;
}
