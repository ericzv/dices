// O icone do jogo desenhado pelo proprio jogo, no estilo da mesa: o feltro
// pontilhado com a vinheta, a moldura de latao em rampa e os dados com o
// contorno, o aro em rampa e a sombra pontilhada. Cada tamanho e desenhado
// no tamanho dele, pixel a pixel (nada de reduzir um grande).
//
// Escreve, para cada tamanho N pedido, icone_N.rgba (N x N x 4) no diretorio
// dado. Quem chama e o tools/gen_icone.py, que compila e roda este arquivo.
#include <stdio.h>
#include <stdlib.h>
#include "games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
int64_t esp_timer_get_time(void) { return 1; }
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
void som_chefe(bool on) { (void)on; }
void som_volume(int c, int n) { (void)c; (void)n; }
int som_volume_de(int c) { (void)c; return SOM_NIVEIS; }
#include "../tests/salva_mem.h"

#define OX 64                                            // onde o icone e desenhado na tela
#define OY 32

// Um d6 pequeno, reto, pixel a pixel (16 a 32 px, onde o desenho da mesa nao
// cabe): a sombra pontilhada, o contorno escuro, o aro de latao em rampa, o
// corpo com a luz pontilhada de cima e os pontos da face (3 ou 5) de lado p,
// centrados (lado - p tem de ser par).
static bool d6_dentro(int x, int y, int lado)
{
    float c = (lado - 1) / 2.0f;
    int cx = x < c ? x : lado - 1 - x, cy = y < c ? y : lado - 1 - y;
    return x >= 0 && y >= 0 && x < lado && y < lado && cx + cy >= 1;   // o canto aparado
}

static void d6_pixel(int x0, int y0, int lado, int p, int mat, int face, int so)
{
    for (int s = so; s >= 1; s--)                         // a sombra: cheia por dentro, pontilhada na beira
        for (int y = 0; y < lado; y++)
            for (int x = 0; x < lado; x++)
                if (d6_dentro(x, y, lado) && (s < so || so == 1 || bayer(x0 + x + s, y0 + y + s) < 0.5f))
                    gfx_pixel(x0 + x + s, y0 + y + s, C_SOMBRA);
    uint16_t lat[4];
    rampa_de(C_LATAO, lat);
    const uint16_t *m = MATERIAL[mat].tons;
    float c = (lado - 1) / 2.0f;
    for (int y = 0; y < lado; y++)
        for (int x = 0; x < lado; x++) {
            if (!d6_dentro(x, y, lado)) continue;
            int X = x0 + x, Y = y0 + y;
            if (!d6_dentro(x - 1, y, lado) || !d6_dentro(x + 1, y, lado) ||
                !d6_dentro(x, y - 1, lado) || !d6_dentro(x, y + 1, lado))
                gfx_pixel(X, Y, C_BARRA);
            else if (!d6_dentro(x - 2, y, lado) || !d6_dentro(x + 2, y, lado) ||
                     !d6_dentro(x, y - 2, lado) || !d6_dentro(x, y + 2, lado))
                gfx_pixel(X, Y, tom_rampa(lat, luz_de(x - c, y - c), X, Y));
            else {
                float luz = -((x - c) + (y - c)) / lado * 1.6f;
                int k = (int)floorf(luz + bayer(X, Y)) + 2;
                gfx_pixel(X, Y, m[k < 0 ? 0 : k > 4 ? 4 : k]);
            }
        }
    int a = 3, b = lado - 3 - p, meio = (lado - p) / 2;
    int cinco[5][2] = { { a, a }, { b, a }, { meio, meio }, { a, b }, { b, b } };
    int tres[3][2] = { { a, a }, { meio, meio }, { b, b } };
    int (*pts)[2] = face == 5 ? cinco : tres;
    for (int i = 0; i < face; i++) gfx_rect(x0 + pts[i][0], y0 + pts[i][1], p, p, MATERIAL[mat].tinta);
}

// Dentro do quadrado de cantos redondos de lado n e raio rc, encolhido de i?
static bool dentro(int x, int y, int n, int rc, int i)
{
    int a = i, b = n - 1 - i, r = rc - i;
    if (r < 1) return x >= a && x <= b && y >= a && y <= b;
    int cx = x < a + r ? a + r : x > b - r ? b - r : x, cy = y < a + r ? a + r : y > b - r ? b - r : y;
    float dx = x - cx, dy = y - cy;
    return x >= a && x <= b && y >= a && y <= b && dx * dx + dy * dy <= (r + 0.35f) * (r + 0.35f);
}

static void icone(int n)
{
    gfx_clear(C_BARRA);
    int rc = n / 6 > 2 ? n / 6 : 2;                       // o raio dos cantos
    int moldura = n >= 48 ? 3 : n >= 24 ? 2 : 1;           // o latao, em pixels
    int borda = n >= 24;                                   // o contorno de fora e o vao (no de 16 nao cabem)
    uint16_t lat[4];
    rampa_de(C_LATAO, lat);
    // A moldura de latao, em rampa (a luz de cima, a esquerda), e o feltro
    // pontilhado: claro no meio, escurecendo para as bordas.
    const uint16_t TOM[3] = { C_FELTRO2, C_FELTRO, C_FELTRO_MEIO };
    float c = (n - 1) / 2.0f;
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            int X = OX + x, Y = OY + y;
            if (!dentro(x, y, n, rc, 0)) continue;
            if (!dentro(x, y, n, rc, borda)) { gfx_pixel(X, Y, C_BARRA); continue; }   // o contorno de fora
            if (!dentro(x, y, n, rc, borda + moldura)) {
                gfx_pixel(X, Y, tom_rampa(lat, luz_de(x - c, y - c), X, Y));
                continue;
            }
            if (!dentro(x, y, n, rc, 2 * borda + moldura)) { gfx_pixel(X, Y, C_SOMBRA); continue; }   // o vao entre o latao e o feltro
            float dx = (x - c) / c, dy = (y - c) / c;
            float v = (dx * dx + dy * dy) * 1.6f;
            int i = (int)v;
            if (i >= 2) i = 2;
            else if (v - i > bayer(X, Y)) i++;
            gfx_pixel(X, Y, TOM[i]);
        }
    // Os dados: o de ebano atras, o de marfim na frente, como na mesa.
    material_j[0] = MAT_MARFIM;
    material_j[1] = MAT_EBANO;
    if (n > 32) {                                         // os dados retos, como parados na mesa
        float k = n / 64.0f;
        desenha_dado(OX + (int)(41 * k), OY + (int)(23 * k), (int)(12 * k), D_D6, 1, 0, 3, 2, false);
        desenha_dado(OX + (int)(25 * k), OY + (int)(39 * k), (int)(14 * k), D_D6, 0, 0, 5, 2, false);
    } else if (n >= 32) {                                 // os dois, feitos a mao
        d6_pixel(OX + 14, OY + 5, 13, 1, MAT_EBANO, 3, 2);
        d6_pixel(OX + 5, OY + 12, 14, 2, MAT_MARFIM, 5, 2);
    } else if (n >= 24) {                                 // um so, o de marfim
        d6_pixel(OX + 5, OY + 5, 14, 2, MAT_MARFIM, 5, 1);
    } else d6_pixel(OX + 2, OY + 2, 11, 1, MAT_MARFIM, 5, 1);
    gfx_quantiza();
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "uso: icone_pixel DIR N...\n"); return 1; }
    aplica_estilo(FT_VERDE, MS_NOGUEIRA);
    for (int a = 2; a < argc; a++) {
        int n = atoi(argv[a]);
        if (n < 8 || n > 256) continue;
        icone(n);
        char path[512];
        snprintf(path, sizeof path, "%s/icone_%d.rgba", argv[1], n);
        FILE *f = fopen(path, "wb");
        if (!f) { perror(path); return 1; }
        int rc = n / 6 > 2 ? n / 6 : 2;
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++) {
                uint16_t c = fb[(OY + y) * GFX_W + OX + x];
                uint8_t px[4] = { (uint8_t)(((c >> 11) & 31) * 255 / 31), (uint8_t)(((c >> 5) & 63) * 255 / 63),
                                  (uint8_t)((c & 31) * 255 / 31), (uint8_t)(dentro(x, y, n, rc, 0) ? 255 : 0) };
                fwrite(px, 1, 4, f);
            }
        fclose(f);
    }
    return 0;
}
