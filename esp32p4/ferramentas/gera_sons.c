// Gera esp32p4/main/sons.bin a partir das gravacoes do jogo do PC
// (desktop/sons/*.ogg): cada arquivo vira um trecho em IMA ADPCM, com o
// mesmo nome. O firmware toca direto desse formato, sem abrir na memoria.
//
// Por que nao o OGG direto: abrir Vorbis no ESP32-P4 custaria uns 30% de um
// nucleo so para a trilha, e abrir todos os efeitos na partida levaria
// dezenas de segundos. O ADPCM ocupa mais flash (4 bits por amostra), mas o
// aparelho le quase de graca.
//
// Formato (tudo little-endian):
//   "DSOM", u16 versao (2), u16 n, u32 taxa (44100)
//   n entradas: char nome[24] (sem o .ogg), u32 amostras, u32 deslocamento
//   os trechos: IMA ADPCM de 4 bits, mono, cada um comecando do zero
// Os sons em estereo (a trilha) chegam somados num canal so: o alto-falante
// do aparelho e um so.
//
// Ao lado do sons.bin sai o sons.lista (nome e tamanho de cada .ogg), que o
// main/CMakeLists.txt compara com desktop/sons/ para avisar quando o som do
// PC mudou e o sons.bin ficou velho.
//
// Para rodar: o alvo sons_p4 do simulador (simulador/CMakeLists.txt), que
// compila isto com o stb_vorbis do raylib e passa todos os .ogg:
//   cmake -S esp32p4/simulador -B build-console
//   cmake --build build-console --target sons_p4
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "stb_vorbis.c"
#pragma GCC diagnostic pop

#define TAXA 44100
#define NOME_TAM 24

typedef struct {
    char nome[NOME_TAM];
    long ogg;                            // bytes do arquivo original
    int16_t *pcm;
    uint32_t amostras, bytes;
    uint8_t *cod;
} trecho_t;

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

// ---------------------------------------------------------------------------
// Um .ogg: abre, soma os canais e, se precisar, leva para 44100 Hz.
// ---------------------------------------------------------------------------
static const char *base(const char *p)
{
    const char *b = p;
    for (const char *q = p; *q; q++)
        if (*q == '/' || *q == '\\') b = q + 1;
    return b;
}

static int le_ogg(const char *arq, trecho_t *t)
{
    const char *b = base(arq);
    size_t n = strlen(b);
    if (n < 5 || strcmp(b + n - 4, ".ogg") || n - 4 >= NOME_TAM) {
        fprintf(stderr, "%s: nome inesperado\n", arq);
        return 0;
    }
    memset(t, 0, sizeof *t);
    memcpy(t->nome, b, n - 4);

    FILE *f = fopen(arq, "rb");
    if (!f) { perror(arq); return 0; }
    fseek(f, 0, SEEK_END);
    t->ogg = ftell(f);
    fclose(f);

    int canais = 0, taxa = 0;
    short *x = NULL;
    int q = stb_vorbis_decode_filename(arq, &canais, &taxa, &x);
    if (q <= 0 || canais < 1) {
        fprintf(stderr, "%s: nao abriu\n", arq);
        return 0;
    }
    double passo = (double)taxa / TAXA;
    uint32_t m = (uint32_t)(q / passo);
    t->pcm = malloc((size_t)m * 2 + 2);
    for (uint32_t i = 0; i < m; i++) {
        double p = i * passo;
        int k = (int)p;
        double fr = p - k;
        double s = 0;
        for (int c = 0; c < canais; c++) {
            double a = x[(size_t)k * canais + c];
            double b2 = k + 1 < q ? x[(size_t)(k + 1) * canais + c] : a;
            s += a + (b2 - a) * fr;
        }
        s /= canais;
        t->pcm[i] = (int16_t)(s > 32767 ? 32767 : s < -32768 ? -32768 : s);
    }
    free(x);
    t->amostras = m;
    t->cod = malloc(m / 2 + 1);
    t->bytes = codifica(t->pcm, m, t->cod);
    return 1;
}

static int compara(const void *a, const void *b)
{
    return strcmp(base(*(const char *const *)a), base(*(const char *const *)b));
}

static void u16(FILE *f, uint16_t v) { fputc(v & 255, f); fputc(v >> 8, f); }
static void u32(FILE *f, uint32_t v) { for (int i = 0; i < 4; i++) fputc((v >> (8 * i)) & 255, f); }

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "uso: gera_sons saida/sons.bin desktop/sons/*.ogg\n");
        return 1;
    }
    const char *saida = argv[1];
    int n = argc - 2;
    const char **arqs = malloc(sizeof(char *) * (size_t)n);
    for (int i = 0; i < n; i++) arqs[i] = argv[i + 2];
    qsort(arqs, (size_t)n, sizeof(char *), compara);       // a mesma ordem do CMake

    trecho_t *t = calloc((size_t)n, sizeof *t);
    for (int i = 0; i < n; i++)
        if (!le_ogg(arqs[i], &t[i])) return 1;

    FILE *f = fopen(saida, "wb");
    if (!f) { perror(saida); return 1; }
    fwrite("DSOM", 1, 4, f);
    u16(f, 2);
    u16(f, (uint16_t)n);
    u32(f, TAXA);
    uint32_t desl = 0, am = 0;
    for (int i = 0; i < n; i++) {
        fwrite(t[i].nome, 1, NOME_TAM, f);
        u32(f, t[i].amostras);
        u32(f, desl);
        desl += t[i].bytes;
        am += t[i].amostras;
    }
    for (int i = 0; i < n; i++) fwrite(t[i].cod, 1, t[i].bytes, f);
    fclose(f);

    // A lista, para o build do firmware saber se o sons.bin esta em dia.
    char lista[4096];
    snprintf(lista, sizeof lista, "%s", saida);
    char *ponto = strrchr(lista, '.');
    if (ponto && ponto > base(lista)) strcpy(ponto, ".lista");
    else strcat(lista, ".lista");
    FILE *l = fopen(lista, "wb");
    if (!l) { perror(lista); return 1; }
    for (int i = 0; i < n; i++) fprintf(l, "%s %ld\n", t[i].nome, t[i].ogg);
    fclose(l);

    printf("%s: %d sons, %.1f s de audio, %u KB\n", saida, n, am / (double)TAXA, (unsigned)(desl / 1024));
    return 0;
}
