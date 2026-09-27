// Efeitos sonoros do PC, sintetizados na partida: nenhum arquivo de audio
// acompanha o executavel. Cada som e uma soma de tons com envelope curto e,
// nos baques, um pouco de ruido abafado.
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "hal/som.h"
#include "plataforma.h"

#define TAXA   44100
#define VOZES  4                 // copias de cada som, para sobrepor
#define MAX_AM (TAXA * 6 / 5)   // amostras do som mais longo: 1,2 s

enum { SENO, TRIANGULO, QUADRADA };

static Sound base[SOM_N], voz[SOM_N][VOZES];
static int   prox[SOM_N];
static bool  pronto, falhou, ligado = true;
static float buf[MAX_AM];
static int   n_buf;

static float onda(int forma, float fase)
{
    float x = fase - floorf(fase);
    switch (forma) {
    case TRIANGULO: return 4.0f * fabsf(x - 0.5f) - 1.0f;
    case QUADRADA:  return x < 0.5f ? 0.6f : -0.6f;
    default:        return sinf(6.2831853f * x);
    }
}

// Tom de f0 a f1 Hz (glissando exponencial), com ataque curto e queda
// exponencial: 'queda' e o tempo em que cai a 1/e.
static void tom(float t0, float dur, float f0, float f1, int forma, float vol, float queda)
{
    int i0 = (int)(t0 * TAXA), n = (int)(dur * TAXA);
    float fase = 0;
    for (int i = 0; i < n && i0 + i < n_buf; i++) {
        float t = (float)i / TAXA, q = (float)i / n;
        float f = f0 * powf(f1 / f0, q);
        fase += f / TAXA;
        float env = (t < 0.004f ? t / 0.004f : 1.0f) * expf(-t / queda);
        env *= q > 0.9f ? (1 - q) * 10 : 1;          // sem estalo no fim
        buf[i0 + i] += vol * env * onda(forma, fase);
    }
}

// Ruido passado num passa-baixa de um polo: 'brilho' perto de 1 e chiado,
// perto de 0 e um baque surdo.
static void ruido(float t0, float dur, float vol, float queda, float brilho)
{
    int i0 = (int)(t0 * TAXA), n = (int)(dur * TAXA);
    float y = 0;
    for (int i = 0; i < n && i0 + i < n_buf; i++) {
        float t = (float)i / TAXA;
        float x = (float)rand() / RAND_MAX * 2 - 1;
        y += brilho * (x - y);
        buf[i0 + i] += vol * y * expf(-t / queda);
    }
}

static void compoe(int s)
{
    switch (s) {
    case SOM_TIQUE:
        tom(0, 0.03f, 2300, 2100, SENO, 0.18f, 0.008f);
        break;
    case SOM_BATE:                                   // madeira no feltro
        ruido(0, 0.05f, 0.9f, 0.010f, 0.35f);
        tom(0, 0.06f, 620, 420, SENO, 0.35f, 0.015f);
        break;
    case SOM_POUSA:                                  // o dado assenta
        ruido(0, 0.08f, 0.9f, 0.018f, 0.12f);
        tom(0, 0.10f, 190, 120, SENO, 0.55f, 0.030f);
        break;
    case SOM_VALIDO:
        tom(0.00f, 0.25f, 880.0f, 880.0f, SENO, 0.28f, 0.08f);
        tom(0.00f, 0.25f, 1760.0f, 1760.0f, SENO, 0.06f, 0.05f);
        tom(0.07f, 0.35f, 1318.5f, 1318.5f, SENO, 0.28f, 0.10f);
        tom(0.07f, 0.35f, 2637.0f, 2637.0f, SENO, 0.05f, 0.06f);
        break;
    case SOM_ANULA:
        tom(0, 0.32f, 330, 150, TRIANGULO, 0.30f, 0.14f);
        tom(0, 0.32f, 311, 142, QUADRADA, 0.07f, 0.12f);
        break;
    case SOM_FICHA:                                  // fichas de barro batendo
        for (int i = 0; i < 2; i++) {
            float t = i * 0.045f;
            ruido(t, 0.03f, 0.35f, 0.006f, 0.8f);
            tom(t, 0.04f, 3100 - i * 350, 2900 - i * 350, SENO, 0.22f, 0.012f);
        }
        break;
    case SOM_EFEITO: {
        static const float N[3] = { 1318.5f, 1661.2f, 1975.5f };
        for (int i = 0; i < 3; i++)
            tom(i * 0.05f, 0.30f, N[i], N[i], TRIANGULO, 0.16f, 0.09f);
        break;
    }
    case SOM_VITORIA: {
        static const float N[4] = { 523.3f, 659.3f, 784.0f, 1046.5f };
        for (int i = 0; i < 4; i++)
            tom(i * 0.09f, i == 3 ? 0.70f : 0.30f, N[i], N[i], TRIANGULO, 0.22f,
                i == 3 ? 0.25f : 0.10f);
        tom(0.27f, 0.70f, 2093.0f, 2093.0f, SENO, 0.05f, 0.20f);
        break;
    }
    case SOM_CORREU:
        tom(0.00f, 0.22f, 392.0f, 392.0f, TRIANGULO, 0.26f, 0.09f);
        tom(0.16f, 0.40f, 293.7f, 277.2f, TRIANGULO, 0.26f, 0.16f);
        break;
    case SOM_ABERTURA: {
        static const float N[4] = { 392.0f, 523.3f, 659.3f, 784.0f };
        for (int i = 0; i < 4; i++)
            tom(i * 0.08f, i == 3 ? 0.90f : 0.35f, N[i], N[i], TRIANGULO, 0.20f,
                i == 3 ? 0.35f : 0.12f);
        tom(0.24f, 0.90f, 1568.0f, 1568.0f, SENO, 0.04f, 0.30f);
        break;
    }
    }
}

// Ultima amostra audivel: o som termina ali, sem cauda de silencio.
static int fim_do_som(void)
{
    int n = n_buf;
    while (n > 1 && fabsf(buf[n - 1]) < 1e-4f) n--;
    return n;
}

void som_inicia(void)
{
    if (!IsAudioDeviceReady()) { falhou = true; ligado = false; return; }
    srand(12345);                                     // o mesmo ruido sempre
    static short pcm[MAX_AM];
    for (int s = 0; s < SOM_N; s++) {
        n_buf = MAX_AM;
        memset(buf, 0, sizeof buf);
        compoe(s);
        int n = fim_do_som();
        for (int i = 0; i < n; i++) {
            float x = buf[i];
            x = x > 1 ? 1 : (x < -1 ? -1 : x);
            pcm[i] = (short)(x * 32000);
        }
        Wave w = { .frameCount = (unsigned)n, .sampleRate = TAXA, .sampleSize = 16,
                   .channels = 1, .data = pcm };
        base[s] = LoadSoundFromWave(w);
        for (int v = 0; v < VOZES; v++) voz[s][v] = LoadSoundAlias(base[s]);
    }
    pronto = true;
}

void som_encerra(void)
{
    if (!pronto) return;
    for (int s = 0; s < SOM_N; s++) {
        for (int v = 0; v < VOZES; v++) UnloadSoundAlias(voz[s][v]);
        UnloadSound(base[s]);
    }
    pronto = false;
}

void som_toca(int s)
{
    if (!pronto || !ligado || s < 0 || s >= SOM_N) return;
    Sound v = voz[s][prox[s]++ % VOZES];
    // Baques nunca soam iguais: variam um pouco de tom.
    if (s == SOM_BATE || s == SOM_POUSA)
        SetSoundPitch(v, 0.9f + 0.2f * (float)rand() / RAND_MAX);
    PlaySound(v);
}

bool som_liga(bool on)  { ligado = on && pronto; return ligado; }
bool som_ligado(void)   { return ligado; }
bool som_falhou(void)   { return falhou; }
