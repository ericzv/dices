// Som do PC, todo sintetizado na partida: nenhum arquivo de audio acompanha
// o executavel.
//
//  - Trilha: bossa de 16 compassos (piano eletrico, contrabaixo, vibrafone,
//    vassourinha e shaker), composta aqui mesmo e tocada em loop.
//  - Rufar: caixinha em crescendo enquanto o dado rola; abaixa a trilha.
//  - Efeitos: baques, fichas, sinos; todos com um pouco de sala.
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "hal/som.h"
#include "plataforma.h"

#define TAXA  44100
#define VOZES 4                              // copias de cada efeito, para sobrepor
#define PI2   6.2831853f

static Sound base[SOM_N], voz[SOM_N][VOZES];
static int   prox[SOM_N];
static Sound rufo;
static Music trilha;
static uint8_t *trilha_wav;
static bool  pronto, falhou, ligado = true, musica = true;
static bool  rufando;
static float vol_rufo, duck = 1;             // duck: quanto a trilha abaixa

static float frand(void) { return (float)rand() / (float)RAND_MAX * 2 - 1; }
static float mtof(float m) { return 440.0f * powf(2.0f, (m - 69) / 12.0f); }

// ---------------------------------------------------------------------------
// Filtros e reverb
// ---------------------------------------------------------------------------
typedef struct { float y; } pb_t;             // passa-baixa de um polo
static float pb(pb_t *f, float x, float k) { f->y += k * (x - f->y); return f->y; }

// Reverb de Schroeder: quatro pentes em paralelo e dois passa-tudo em serie.
#define RV_MAX 4096
typedef struct {
    float pente[4][RV_MAX], pt[2][1024], amort[4];
    int   n_pente[4], n_pt[2], i_pente[4], i_pt[2];
    float fb;
} reverb_t;

static void reverb_ini(reverb_t *r, float tamanho, float fb, int lado)
{
    static const int P[4] = { 1557, 1617, 1491, 1422 }, A[2] = { 225, 556 };
    memset(r, 0, sizeof *r);
    for (int i = 0; i < 4; i++) r->n_pente[i] = (int)(P[i] * tamanho) + lado * 23;
    for (int i = 0; i < 2; i++) r->n_pt[i] = A[i] + lado * 7;
    r->fb = fb;
}

static float reverb(reverb_t *r, float x)
{
    float s = 0;
    for (int i = 0; i < 4; i++) {
        float *b = &r->pente[i][r->i_pente[i]];
        float y = *b;
        r->amort[i] += 0.3f * (y - r->amort[i]);          // cauda mais escura
        *b = x + r->amort[i] * r->fb;
        if (++r->i_pente[i] >= r->n_pente[i]) r->i_pente[i] = 0;
        s += y;
    }
    s *= 0.25f;
    for (int i = 0; i < 2; i++) {
        float *b = &r->pt[i][r->i_pt[i]];
        float y = *b;
        *b = s + y * 0.5f;
        s = y - s * 0.5f;
        if (++r->i_pt[i] >= r->n_pt[i]) r->i_pt[i] = 0;
    }
    return s;
}

// ---------------------------------------------------------------------------
// Efeitos: tons e ruidos somados num buffer mono
// ---------------------------------------------------------------------------
#define MAX_EF (TAXA * 3)
static float ef[MAX_EF];

enum { SENO, TRIANGULO, SINO };

// Tom de f0 a f1 Hz com ataque curto e queda exponencial ('queda': tempo em
// que cai a 1/e). SINO soma parciais inarmonicos, como um sininho.
static void tom(float t0, float dur, float f0, float f1, int forma, float vol, float queda)
{
    int i0 = (int)(t0 * TAXA), n = (int)(dur * TAXA);
    float fase = 0;
    for (int i = 0; i < n && i0 + i < MAX_EF; i++) {
        float t = (float)i / TAXA, q = (float)i / n;
        float f = f0 * powf(f1 / f0, q);
        fase += f / TAXA;
        float env = (t < 0.003f ? t / 0.003f : 1.0f) * expf(-t / queda);
        env *= q > 0.9f ? (1 - q) * 10 : 1;
        float x;
        switch (forma) {
        case TRIANGULO: { float p = fase - floorf(fase); x = 4 * fabsf(p - 0.5f) - 1; break; }
        case SINO:
            x = sinf(PI2 * fase) + 0.45f * sinf(PI2 * fase * 2.76f) * expf(-t / (queda * 0.4f))
              + 0.25f * sinf(PI2 * fase * 5.40f) * expf(-t / (queda * 0.2f));
            x *= 0.6f;
            break;
        default: x = sinf(PI2 * fase); break;
        }
        ef[i0 + i] += vol * env * x;
    }
}

// Ruido filtrado: passa-baixa em 'grave' e passa-alta em 'agudo' (0..1).
static void ruido(float t0, float dur, float vol, float queda, float grave, float agudo)
{
    int i0 = (int)(t0 * TAXA), n = (int)(dur * TAXA);
    pb_t a = { 0 }, b = { 0 };
    for (int i = 0; i < n && i0 + i < MAX_EF; i++) {
        float t = (float)i / TAXA;
        float x = pb(&a, frand(), grave);
        x -= pb(&b, x, agudo);
        float env = (t < 0.002f ? t / 0.002f : 1.0f) * expf(-t / queda);
        ef[i0 + i] += vol * env * x;
    }
}

// Estalo de ficha de barro: clique agudo com corpo curto.
static void clique(float t, float f, float vol)
{
    ruido(t, 0.03f, vol * 1.4f, 0.004f, 0.9f, 0.35f);
    tom(t, 0.05f, f, f * 0.94f, SENO, vol * 0.5f, 0.010f);
}

static void compoe(int s)
{
    switch (s) {
    case SOM_TIQUE:                              // madeira fina
        tom(0, 0.04f, 1900, 1750, SENO, 0.16f, 0.007f);
        ruido(0, 0.02f, 0.10f, 0.003f, 0.8f, 0.3f);
        break;
    case SOM_BATE:                               // o dado bate no trilho de madeira
        ruido(0, 0.05f, 1.2f, 0.008f, 0.45f, 0.05f);
        tom(0, 0.07f, 740, 520, SENO, 0.30f, 0.014f);
        tom(0, 0.05f, 1880, 1600, SENO, 0.08f, 0.006f);
        break;
    case SOM_POUSA:                              // assenta no feltro: baque abafado
        ruido(0, 0.10f, 1.3f, 0.020f, 0.10f, 0.004f);
        tom(0, 0.12f, 150, 95, SENO, 0.60f, 0.035f);
        break;
    case SOM_VALIDO: {                           // dois sininhos subindo
        static const float N[3] = { 79, 83, 86 };            // G5 B5 D6
        for (int i = 0; i < 3; i++)
            tom(i * 0.055f, 0.9f, mtof(N[i]), mtof(N[i]), SINO, 0.20f, 0.30f);
        break;
    }
    case SOM_ANULA:                              // dois dados caem: baque e descida
        ruido(0, 0.12f, 0.9f, 0.03f, 0.08f, 0.003f);
        tom(0.00f, 0.40f, 220, 110, TRIANGULO, 0.28f, 0.16f);
        tom(0.09f, 0.45f, 207, 98, TRIANGULO, 0.22f, 0.18f);
        break;
    case SOM_FICHA:                              // fichas se ajeitando na pilha
        clique(0.000f, 3300, 0.35f);
        clique(0.045f, 2900, 0.28f);
        clique(0.075f, 3100, 0.18f);
        break;
    case SOM_EFEITO: {                           // brilho: arpejo de sininhos
        static const float N[4] = { 81, 85, 88, 93 };
        for (int i = 0; i < 4; i++)
            tom(i * 0.045f, 0.7f, mtof(N[i]), mtof(N[i]), SINO, 0.13f, 0.20f);
        ruido(0, 0.35f, 0.10f, 0.12f, 0.9f, 0.6f);
        break;
    }
    case SOM_VITORIA: {                          // acorde subindo e fichas caindo no monte
        static const float N[5] = { 72, 76, 79, 84, 88 };
        for (int i = 0; i < 5; i++)
            tom(i * 0.07f, 1.4f, mtof(N[i]), mtof(N[i]), SINO, 0.16f, i == 4 ? 0.6f : 0.35f);
        for (int i = 0; i < 7; i++)
            clique(0.30f + i * 0.05f + frand() * 0.01f, 2800 + frand() * 500, 0.22f - i * 0.02f);
        break;
    }
    case SOM_CORREU:                             // desiste: duas notas descendo
        tom(0.00f, 0.35f, mtof(67), mtof(67), TRIANGULO, 0.22f, 0.14f);
        tom(0.20f, 0.70f, mtof(62), mtof(61), TRIANGULO, 0.22f, 0.25f);
        break;
    case SOM_ABERTURA: {                         // vinheta: arpejo com sino no alto
        static const float N[5] = { 65, 69, 72, 76, 81 };
        for (int i = 0; i < 5; i++)
            tom(i * 0.09f, 1.6f, mtof(N[i]), mtof(N[i]), SINO, 0.15f, i == 4 ? 0.8f : 0.35f);
        break;
    }
    }
}

// Volume de pico de cada efeito, ja com a sala: o que toca muito fica baixo.
static const float PICO[SOM_N] = {
    [SOM_TIQUE] = 0.20f, [SOM_BATE] = 0.36f, [SOM_POUSA] = 0.45f, [SOM_VALIDO] = 0.40f,
    [SOM_ANULA] = 0.42f, [SOM_FICHA] = 0.38f, [SOM_EFEITO] = 0.32f, [SOM_VITORIA] = 0.45f,
    [SOM_CORREU] = 0.34f, [SOM_ABERTURA] = 0.40f,
};

// Compoe o efeito s, poe sala, normaliza e devolve quantas amostras valem.
static int renderiza_efeito(int s, float *saida)
{
    static reverb_t rv;
    memset(ef, 0, sizeof ef);
    compoe(s);
    reverb_ini(&rv, 0.5f, 0.70f, 0);
    float sala = (s == SOM_TIQUE || s == SOM_BATE) ? 0.12f : 0.35f, pico = 1e-6f;
    for (int i = 0; i < MAX_EF; i++) {
        saida[i] = ef[i] + sala * reverb(&rv, ef[i]);
        pico = fmaxf(pico, fabsf(saida[i]));
    }
    int fim = 1;
    for (int i = 0; i < MAX_EF; i++) {
        saida[i] *= PICO[s] / pico;
        if (fabsf(saida[i]) > 3e-4f) fim = i + 1;
    }
    int rampa = TAXA / 50;                       // 20 ms de saida suave
    for (int i = 0; i < rampa && fim - 1 - i >= 0; i++) saida[fim - 1 - i] *= (float)i / rampa;
    return fim;
}

static Sound monta_efeito(int s)
{
    static float saida[MAX_EF];
    static short pcm[MAX_EF];
    int n = renderiza_efeito(s, saida);
    for (int i = 0; i < n; i++) pcm[i] = (short)(saida[i] * 32000);
    Wave w = { .frameCount = (unsigned)n, .sampleRate = TAXA, .sampleSize = 16,
               .channels = 1, .data = pcm };
    return LoadSoundFromWave(w);
}

// Rufar: caixinha tocada com vassourinha, golpes alternados de cada mao,
// crescendo nos primeiros dois segundos e depois firme.
#define N_RUFO (TAXA * 8)
static void renderiza_rufo(float *x)
{
    const int n = N_RUFO;
    float golpe = 1.0f / 22;                      // 22 golpes por segundo
    for (int g = 0; g * golpe < 8; g++) {
        float t0 = g * golpe + (g ? frand() * 0.004f : 0);  // nunca antes do zero
        float cresce = fminf(1.0f, 0.18f + 0.82f * (t0 / 2.2f));
        float v = cresce * (g % 2 ? 0.85f : 1.0f) * (0.9f + 0.1f * frand());
        int i0 = (int)(t0 * TAXA);
        pb_t a = { 0 }, b = { 0 };
        for (int i = 0; i < TAXA / 10 && i0 + i < n; i++) {
            float t = (float)i / TAXA;
            float r = pb(&a, frand(), 0.55f);
            r -= pb(&b, r, 0.08f);                 // tira o grave: pele, nao bumbo
            float corpo = sinf(PI2 * 190 * t) * expf(-t / 0.012f) * 0.5f;
            x[i0 + i] += v * (r * expf(-t / 0.035f) + corpo) * 0.32f;
        }
    }
    reverb_t *rv = malloc(sizeof *rv);
    reverb_ini(rv, 0.6f, 0.72f, 0);
    for (int i = 0; i < n; i++) {
        float y = x[i] + 0.25f * reverb(rv, x[i]);
        x[i] = y > 1 ? 1 : (y < -1 ? -1 : y);
    }
    free(rv);
}

static Sound monta_rufo(void)
{
    float *x = calloc(N_RUFO, sizeof *x);
    short *pcm = malloc(N_RUFO * sizeof *pcm);
    renderiza_rufo(x);
    for (int i = 0; i < N_RUFO; i++) pcm[i] = (short)(x[i] * 30000);
    Wave w = { .frameCount = N_RUFO, .sampleRate = TAXA, .sampleSize = 16,
               .channels = 1, .data = pcm };
    Sound s = LoadSoundFromWave(w);
    free(x); free(pcm);
    return s;
}

// ---------------------------------------------------------------------------
// Trilha
// ---------------------------------------------------------------------------
#define BPM      100.0f
#define BATIDA   (60.0f / BPM)
#define COMPASSOS 16
#define N_TRILHA ((int)(COMPASSOS * 4 * BATIDA * TAXA))

static float *tl, *tr;                       // canais esquerdo e direito

// Soma uma amostra com panorama (-1 esquerda, +1 direita), dando a volta no
// fim para o loop emendar sem costura.
static void poe(int i, float x, float pan)
{
    i %= N_TRILHA;
    if (i < 0) i += N_TRILHA;
    tl[i] += x * (1 - pan) * 0.5f;
    tr[i] += x * (1 + pan) * 0.5f;
}

enum { EP, VIBRA, BAIXO };

static void nota(int inst, float bat, float dur, float midi, float vel, float pan)
{
    float f = mtof(midi);
    int i0 = (int)(bat * BATIDA * TAXA);
    float soa = dur * BATIDA;
    float cauda = inst == BAIXO ? 0.06f : (inst == EP ? 0.25f : 0.9f);
    int n = (int)((soa + cauda) * TAXA);
    float fa = 0, fm = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / TAXA;
        float solta = t > soa ? expf(-(t - soa) / (cauda * 0.3f)) : 1;
        float x;
        switch (inst) {
        case EP: {                               // piano eletrico: FM de razao 1
            float ind = 1.6f * expf(-t / 0.25f) + 0.35f;
            fm += f / TAXA; fa += f * 1.003f / TAXA;
            x = sinf(PI2 * fa + ind * sinf(PI2 * fm)) * expf(-t / 1.4f);
            x *= t < 0.004f ? t / 0.004f : 1;
            break;
        }
        case VIBRA: {                            // vibrafone: seno + parcial 4x, tremolo
            fa += f / TAXA;
            float trem = 1 - 0.22f * (0.5f + 0.5f * sinf(PI2 * 5.2f * t));
            x = (sinf(PI2 * fa) + 0.22f * sinf(PI2 * fa * 4) * expf(-t / 0.15f))
              * expf(-t / 1.3f) * trem;
            x *= t < 0.002f ? t / 0.002f : 1;
            break;
        }
        default: {                               // contrabaixo: dedilhado redondo
            fa += f / TAXA;
            x = (sinf(PI2 * fa) + 0.35f * sinf(PI2 * fa * 2) * expf(-t / 0.15f)
                 + 0.12f * sinf(PI2 * fa * 3) * expf(-t / 0.08f)) * expf(-t / 0.9f);
            x *= t < 0.008f ? t / 0.008f : 1;
            break;
        }
        }
        poe(i0 + i, vel * x * solta, pan);
    }
}

static void bumbo(float bat, float vel)
{
    int i0 = (int)(bat * BATIDA * TAXA);
    float fase = 0;
    for (int i = 0; i < TAXA / 5; i++) {
        float t = (float)i / TAXA;
        fase += (48 + 50 * expf(-t / 0.03f)) / TAXA;
        poe(i0 + i, vel * sinf(PI2 * fase) * expf(-t / 0.10f), 0);
    }
}

// Ruido filtrado curto: vassourinha na caixa (grave) ou shaker (agudo).
static void chiado(float bat, float dur, float vel, float grave, float agudo, float pan)
{
    int i0 = (int)(bat * BATIDA * TAXA), n = (int)(dur * TAXA * 4);
    pb_t a = { 0 }, b = { 0 };
    for (int i = 0; i < n; i++) {
        float t = (float)i / TAXA;
        float r = pb(&a, frand(), grave);
        r -= pb(&b, r, agudo);
        float env = (t < 0.004f ? t / 0.004f : 1) * expf(-t / dur);
        poe(i0 + i, vel * r * env, pan);
    }
}

typedef struct { int raiz, tipo; } acorde_t;
enum { MAJ7, M7, DOM7, M6 };
static const int INTERV[4][4] = { { 0, 4, 7, 11 }, { 0, 3, 7, 10 }, { 0, 4, 7, 10 }, { 0, 3, 7, 9 } };

// Dois acordes por compasso (o segundo repete o primeiro quando nao muda).
// Fa maior: I vi ii V, com um desvio para Bb e Bbm6 no meio.
static const acorde_t HARMONIA[COMPASSOS][2] = {
    { { 41, MAJ7 }, { 41, MAJ7 } }, { { 38, M7 }, { 38, M7 } },
    { { 43, M7 }, { 43, M7 } },     { { 36, DOM7 }, { 36, DOM7 } },
    { { 41, MAJ7 }, { 41, MAJ7 } }, { { 38, M7 }, { 38, M7 } },
    { { 43, M7 }, { 36, DOM7 } },   { { 41, MAJ7 }, { 41, MAJ7 } },
    { { 46, MAJ7 }, { 46, MAJ7 } }, { { 46, M6 }, { 46, M6 } },
    { { 45, M7 }, { 45, M7 } },     { { 38, DOM7 }, { 38, DOM7 } },
    { { 43, M7 }, { 43, M7 } },     { { 36, DOM7 }, { 36, DOM7 } },
    { { 41, MAJ7 }, { 41, MAJ7 } }, { { 43, M7 }, { 36, DOM7 } },
};

// Melodia do vibrafone: { compasso, batida, duracao, nota MIDI }.
static const float MELODIA[][4] = {
    {0,0,1.5f,69},{0,1.5f,.5f,72},{0,2,2,76},
    {1,0,1,74},{1,1,1,72},{1,2,2,69},
    {2,0,1.5f,70},{2,1.5f,.5f,74},{2,2,1.5f,77},{2,3.5f,.5f,76},
    {3,0,1,76},{3,1,.5f,74},{3,1.5f,.5f,72},{3,2,2,70},
    {4,0,1,69},{4,1,1,72},{4,2,1,77},{4,3,1,76},
    {5,0,1.5f,77},{5,1.5f,.5f,76},{5,2,2,74},
    {6,0,1,74},{6,1,1,70},{6,2,1,76},{6,3,1,79},
    {7,0,3,77},
    {8,0,1.5f,74},{8,1.5f,.5f,77},{8,2,2,81},
    {9,0,1,79},{9,1,.5f,77},{9,1.5f,.5f,73},{9,2,2,77},
    {10,0,1.5f,76},{10,1.5f,.5f,72},{10,2,1,79},{10,3,1,76},
    {11,0,1,78},{11,1,1,81},{11,2,1,84},{11,3,1,81},
    {12,0,1.5f,82},{12,1.5f,.5f,81},{12,2,1,79},{12,3,1,74},
    {13,0,1,76},{13,1,1,79},{13,2,1,82},{13,3,.5f,76},{13,3.5f,.5f,79},
    {14,0,1.5f,81},{14,1.5f,.5f,79},{14,2,2,77},
    {15,0,1,76},{15,1,1,72},
};

static void acorde_ep(float bat, float dur, acorde_t a, float vel)
{
    for (int k = 1; k < 4; k++) {                // sem a raiz: o baixo cuida dela
        int m = a.raiz + INTERV[a.tipo][k];
        while (m < 55) m += 12;
        while (m >= 67) m -= 12;
        nota(EP, bat, dur, (float)m, vel, -0.35f);
    }
    int nona = a.raiz + 14;                      // a nona da o tempero de bossa
    while (nona < 60) nona += 12;
    while (nona >= 72) nona -= 12;
    nota(EP, bat, dur, (float)nona, vel * 0.6f, -0.35f);
}

static void compoe_trilha(void)
{
    for (int c = 0; c < COMPASSOS; c++) {
        float b0 = c * 4.0f;
        const acorde_t *h = HARMONIA[c];
        bool troca = h[0].raiz != h[1].raiz || h[0].tipo != h[1].tipo;
        acorde_t prox_ac = HARMONIA[(c + 1) % COMPASSOS][0];

        // Piano eletrico: batida de bossa, sincopada.
        acorde_ep(b0 + 0.0f, 1.25f, h[0], 0.09f);
        acorde_ep(b0 + 1.5f, 0.5f, h[0], 0.07f);
        acorde_ep(b0 + 2.5f, 1.25f, h[1], 0.08f);

        // Contrabaixo: raiz, quinta, e uma aproximacao para o proximo acorde.
        int r0 = h[0].raiz, r1 = h[1].raiz;
        while (r0 > 45) r0 -= 12;
        while (r1 > 45) r1 -= 12;
        nota(BAIXO, b0 + 0.0f, 1.4f, (float)r0, 0.32f, 0);
        nota(BAIXO, b0 + 1.5f, 0.45f, (float)(r0 + 7), 0.24f, 0);
        nota(BAIXO, b0 + 2.0f, 1.4f, (float)(troca ? r1 : r0 + 7), 0.28f, 0);
        int alvo = prox_ac.raiz;
        while (alvo > 45) alvo -= 12;
        nota(BAIXO, b0 + 3.5f, 0.45f, (float)(alvo - 1), 0.22f, 0);

        // Bateria de vassourinha: bumbo leve, caixa no 2 e no 4, shaker nas colcheias.
        bumbo(b0 + 0.0f, 0.30f);
        bumbo(b0 + 2.5f, 0.18f);
        chiado(b0 + 1.0f, 0.07f, 0.045f, 0.35f, 0.05f, -0.3f);
        chiado(b0 + 3.0f, 0.07f, 0.045f, 0.35f, 0.05f, -0.3f);
        for (int e = 0; e < 8; e++)
            chiado(b0 + e * 0.5f, 0.018f, e % 2 ? 0.028f : 0.016f, 0.95f, 0.5f, 0.45f);
    }
    for (unsigned i = 0; i < sizeof MELODIA / sizeof MELODIA[0]; i++) {
        const float *m = MELODIA[i];
        nota(VIBRA, m[0] * 4 + m[1], m[2], m[3], 0.12f, 0.3f);
    }
}

// Sala: o reverb roda duas voltas no loop para a cauda do fim cair no
// comeco, e a segunda volta e a que fica.
static void sala_trilha(void)
{
    reverb_t *rl = malloc(sizeof *rl), *rr = malloc(sizeof *rr);
    reverb_ini(rl, 1.0f, 0.80f, 0);
    reverb_ini(rr, 1.0f, 0.80f, 1);
    float *wl = malloc(N_TRILHA * sizeof *wl), *wr = malloc(N_TRILHA * sizeof *wr);
    for (int volta = 0; volta < 2; volta++)
        for (int i = 0; i < N_TRILHA; i++) {
            float m = (tl[i] + tr[i]) * 0.5f;
            wl[i] = reverb(rl, m);
            wr[i] = reverb(rr, m);
        }
    for (int i = 0; i < N_TRILHA; i++) { tl[i] += 0.30f * wl[i]; tr[i] += 0.30f * wr[i]; }
    free(wl); free(wr); free(rl); free(rr);
}

static void escreve_u32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void escreve_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

// A trilha pronta vira um WAV em memoria, que o raylib toca em stream.
static void monta_trilha(void)
{
    tl = calloc(N_TRILHA, sizeof *tl);
    tr = calloc(N_TRILHA, sizeof *tr);
    srand(2024);
    compoe_trilha();
    sala_trilha();
    float pico = 1e-6f;
    for (int i = 0; i < N_TRILHA; i++) pico = fmaxf(pico, fmaxf(fabsf(tl[i]), fabsf(tr[i])));
    float g = 0.9f / pico;

    uint32_t dados = (uint32_t)N_TRILHA * 4;
    trilha_wav = malloc(44 + dados);
    uint8_t *h = trilha_wav;
    memcpy(h, "RIFF", 4); escreve_u32(h + 4, 36 + dados); memcpy(h + 8, "WAVEfmt ", 8);
    escreve_u32(h + 16, 16); escreve_u16(h + 20, 1); escreve_u16(h + 22, 2);
    escreve_u32(h + 24, TAXA); escreve_u32(h + 28, TAXA * 4); escreve_u16(h + 32, 4);
    escreve_u16(h + 34, 16); memcpy(h + 36, "data", 4); escreve_u32(h + 40, dados);
    for (int i = 0; i < N_TRILHA; i++) {
        escreve_u16(h + 44 + i * 4, (uint16_t)(int16_t)(tl[i] * g * 32000));
        escreve_u16(h + 46 + i * 4, (uint16_t)(int16_t)(tr[i] * g * 32000));
    }
    free(tl); free(tr);
    trilha = LoadMusicStreamFromMemory(".wav", trilha_wav, (int)(44 + dados));
    trilha.looping = true;
}

// ---------------------------------------------------------------------------
// Interface
// ---------------------------------------------------------------------------
void som_inicia(void)
{
    if (!IsAudioDeviceReady()) { falhou = true; ligado = false; return; }
    srand(12345);                                // o mesmo som sempre
    for (int s = 0; s < SOM_N; s++) {
        base[s] = monta_efeito(s);
        for (int v = 0; v < VOZES; v++) voz[s][v] = LoadSoundAlias(base[s]);
    }
    rufo = monta_rufo();
    monta_trilha();
    SetMusicVolume(trilha, 0.35f);
    PlayMusicStream(trilha);
    pronto = true;
}

void som_encerra(void)
{
    if (!pronto) return;
    for (int s = 0; s < SOM_N; s++) {
        for (int v = 0; v < VOZES; v++) UnloadSoundAlias(voz[s][v]);
        UnloadSound(base[s]);
    }
    UnloadSound(rufo);
    UnloadMusicStream(trilha);
    free(trilha_wav);
    pronto = false;
}

void som_atualiza(float dt)
{
    if (!pronto) return;
    // O rufar entra em 60 ms e sai em 120 ms; a trilha abaixa enquanto ele toca.
    float alvo = rufando && ligado ? 1 : 0;
    vol_rufo += (alvo - vol_rufo) * fminf(1, dt / (rufando ? 0.06f : 0.12f));
    if (IsSoundPlaying(rufo)) {
        SetSoundVolume(rufo, vol_rufo * 0.8f);
        if (!rufando && vol_rufo < 0.01f) StopSound(rufo);
    }
    float d = rufando ? 0.45f : 1;
    duck += (d - duck) * fminf(1, dt / 0.25f);
    SetMusicVolume(trilha, 0.35f * duck * (ligado && musica ? 1 : 0));
    UpdateMusicStream(trilha);
}

void som_toca(int s)
{
    if (!pronto || !ligado || s < 0 || s >= SOM_N) return;
    Sound v = voz[s][prox[s]++ % VOZES];
    // Baques nunca soam iguais: variam um pouco de tom.
    if (s == SOM_BATE || s == SOM_POUSA || s == SOM_FICHA)
        SetSoundPitch(v, 0.9f + 0.2f * (float)rand() / (float)RAND_MAX);
    PlaySound(v);
}

void som_rufo(bool on)
{
    if (!pronto) return;
    if (on && !rufando) {
        vol_rufo = 0;
        SetSoundVolume(rufo, 0);
        PlaySound(rufo);
    }
    rufando = on;
}

bool som_liga(bool on)  { ligado = on && pronto; return ligado; }
bool som_ligado(void)   { return ligado; }
bool som_falhou(void)   { return falhou; }

bool som_musica(bool on) { musica = on; return musica; }
bool som_musica_ligada(void) { return musica; }
