// Exporta os sons do jogo para WAV, sem abrir a placa de som: serve para
// ouvir a trilha e os efeitos fora do jogo e conferir niveis.
//   exporta_som DIR
#include <stdio.h>
#include "../desktop/som.c"

static void grava(const char *dir, const char *nome, const float *l, const float *r, int n)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.wav", dir, nome);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    int ch = r ? 2 : 1;
    uint8_t h[44];
    uint32_t dados = (uint32_t)n * 2 * ch;
    memcpy(h, "RIFF", 4); escreve_u32(h + 4, 36 + dados); memcpy(h + 8, "WAVEfmt ", 8);
    escreve_u32(h + 16, 16); escreve_u16(h + 20, 1); escreve_u16(h + 22, (uint16_t)ch);
    escreve_u32(h + 24, TAXA); escreve_u32(h + 28, TAXA * 2 * ch); escreve_u16(h + 32, (uint16_t)(2 * ch));
    escreve_u16(h + 34, 16); memcpy(h + 36, "data", 4); escreve_u32(h + 40, dados);
    fwrite(h, 1, 44, f);
    float pico = 0;
    for (int i = 0; i < n; i++)
        for (int c = 0; c < ch; c++) {
            float x = c ? r[i] : l[i];
            pico = fmaxf(pico, fabsf(x));
            x = x > 1 ? 1 : (x < -1 ? -1 : x);
            int16_t v = (int16_t)(x * 32000);
            fwrite(&v, 2, 1, f);
        }
    fclose(f);
    printf("%-10s %6.2f s  pico %.2f\n", nome, (double)n / TAXA, pico);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    static const char *NOME[SOM_N] = { "tique", "bate", "pousa", "valido", "anula",
                                       "ficha", "efeito", "vitoria", "correu", "abertura" };
    srand(12345);
    static float saida[MAX_EF];
    for (int s = 0; s < SOM_N; s++) grava(dir, NOME[s], saida, NULL, renderiza_efeito(s, saida));
    float *r = calloc(N_RUFO, sizeof *r);
    renderiza_rufo(r);
    grava(dir, "rufo", r, NULL, N_RUFO);
    tl = calloc(N_TRILHA, sizeof *tl);
    tr = calloc(N_TRILHA, sizeof *tr);
    srand(2024);
    compoe_trilha();
    sala_trilha();
    float pico = 1e-6f;
    for (int i = 0; i < N_TRILHA; i++) pico = fmaxf(pico, fmaxf(fabsf(tl[i]), fabsf(tr[i])));
    for (int i = 0; i < N_TRILHA; i++) { tl[i] *= 0.9f / pico; tr[i] *= 0.9f / pico; }
    grava(dir, "trilha", tl, tr, N_TRILHA);
    return 0;
}
