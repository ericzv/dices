#include <stdlib.h>
#include "../hal/display.h"
#include "gfx.h"

// Tabela de todas as 65536 cores RGB565 para a cor da paleta mais proxima.
static uint16_t lut[65536];
static bool tem_paleta;

static void rgb888(uint16_t c, int *r, int *g, int *b)
{
    *r = ((c >> 11) & 31) * 255 / 31;
    *g = ((c >> 5) & 63) * 255 / 63;
    *b = (c & 31) * 255 / 31;
}

void gfx_paleta(const uint16_t *cores, int n)
{
    int pr[64], pg[64], pb[64];
    if (n > 64) n = 64;
    for (int i = 0; i < n; i++) rgb888(cores[i], &pr[i], &pg[i], &pb[i]);
    for (int c = 0; c < 65536; c++) {
        int r, g, b, melhor = 0;
        long dmin = -1;
        rgb888((uint16_t)c, &r, &g, &b);
        for (int i = 0; i < n; i++) {
            int rm = (r + pr[i]) / 2, dr = r - pr[i], dg = g - pg[i], db = b - pb[i];
            long d = (long)(512 + rm) * dr * dr / 256 + 4L * dg * dg + (long)(767 - rm) * db * db / 256;
            if (dmin < 0 || d < dmin) { dmin = d; melhor = i; }
        }
        lut[c] = cores[melhor];
    }
    tem_paleta = n > 0;
}

uint16_t gfx_na_paleta(uint16_t c) { return tem_paleta ? lut[c] : c; }

void gfx_quantiza(void)
{
    if (!tem_paleta) return;
    uint16_t *fb = display_fb();
    for (int i = 0; i < GFX_W * GFX_H; i++) fb[i] = lut[fb[i]];
}

static uint16_t *desvio;
void gfx_desvia(uint16_t *buf) { desvio = buf; }
#define FB (desvio ? desvio : display_fb())

void gfx_clear(uint16_t c)
{
    uint16_t *p = FB;
    for (int i = 0; i < GFX_W * GFX_H; i++) p[i] = c;
}

static bool recorta(int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > GFX_W) *w = GFX_W - *x;
    if (*y + *h > GFX_H) *h = GFX_H - *y;
    return *w > 0 && *h > 0;
}

void gfx_rect(int x, int y, int w, int h, uint16_t c)
{
    if (!recorta(&x, &y, &w, &h)) return;
    uint16_t *p = FB;
    for (int j = 0; j < h; j++) {
        uint16_t *row = p + (y + j) * GFX_W + x;
        for (int i = 0; i < w; i++) row[i] = c;
    }
}

uint16_t gfx_mistura(uint16_t a, uint16_t b, int pct)
{
    int ra = (a >> 11) & 31, ga = (a >> 5) & 63, ba = a & 31;
    int rb = (b >> 11) & 31, gb = (b >> 5) & 63, bb = b & 31;
    int r = ra + (rb - ra) * pct / 100, g = ga + (gb - ga) * pct / 100;
    int bl = ba + (bb - ba) * pct / 100;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

void gfx_rect_alfa(int x, int y, int w, int h, uint16_t c, int alfa)
{
    if (!recorta(&x, &y, &w, &h)) return;
    int pct = alfa * 100 / 255;
    uint16_t *p = FB;
    for (int j = 0; j < h; j++) {
        uint16_t *row = p + (y + j) * GFX_W + x;
        for (int i = 0; i < w; i++) row[i] = gfx_mistura(row[i], c, pct);
    }
}

void gfx_moldura(int x, int y, int w, int h, int e, uint16_t c)
{
    gfx_rect(x, y, w, e, c);
    gfx_rect(x, y + h - e, w, e, c);
    gfx_rect(x, y, e, h, c);
    gfx_rect(x + w - e, y, e, h, c);
}

void gfx_pixel(int x, int y, uint16_t c)
{
    if (x < 0 || y < 0 || x >= GFX_W || y >= GFX_H) return;
    FB[y * GFX_W + x] = c;
}

void gfx_linha(int x0, int y0, int x1, int y1, uint16_t c)
{
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = x1 < x0 ? -1 : 1, sy = y1 < y0 ? -1 : 1;
    int err = dx - dy;
    for (;;) {
        gfx_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// Linha de 'esp' pixels: um quadrado arrastado ao longo dela.
void gfx_linha_grossa(int x0, int y0, int x1, int y1, int esp, uint16_t c)
{
    if (esp <= 1) { gfx_linha(x0, y0, x1, y1, c); return; }
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = x1 < x0 ? -1 : 1, sy = y1 < y0 ? -1 : 1;
    int err = dx - dy, m = esp / 2;
    for (;;) {
        gfx_rect(x0 - m, y0 - m, esp, esp, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void gfx_disco(int cx, int cy, int r, uint16_t c)
{
    if (r <= 0) { gfx_pixel(cx, cy, c); return; }
    for (int dy = -r; dy <= r; dy++) {
        int w = 0;
        while ((w + 1) * (w + 1) + dy * dy <= r * r + r) w++;
        gfx_rect(cx - w, cy + dy, w * 2 + 1, 1, c);
    }
}

// Proximo caractere de uma string UTF-8, como codigo Latin-1. O que a
// fonte nao tem vira '?'.
int gfx_proximo_car(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    int c = *p++;
    if (c >= 0xC0 && (*p & 0xC0) == 0x80) {
        int cp = ((c & 0x1F) << 6) | (*p++ & 0x3F);
        if (c >= 0xE0) { while ((*p & 0xC0) == 0x80) p++; cp = '?'; }
        c = cp;
    }
    *s = (const char *)p;
    return c >= FONTE_PRIM && c <= 0xFF ? c : '?';
}

// Um glifo de uma das fontes de pixels, ampliado 'e' vezes (cada pixel da
// fonte vira um bloco de e x e pixels do jogo: sempre no grid da mesa).
static void glifo(int x, int y, const uint16_t *g, int alt, uint16_t c, int e)
{
    for (int j = 0; j < alt; j++) {
        uint16_t b = g[j];
        for (int i = 0; b; i++, b >>= 1)
            if (b & 1) {
                if (e == 1) gfx_pixel(x + i, y + j, c);
                else gfx_rect(x + i * e, y + j * e, e, e, c);
            }
    }
}

// Sombra do texto em destaque: um pixel abaixo, quase preta. Na Jersey, que ja
// e encorpada, o destaque fica por conta dela (engrossar junta as letras).
#define SOMBRA_TEXTO 0x0841

int gfx_texto(int x, int y, const char *s, uint16_t c, int e, bool negrito)
{
    int x0 = x;
    while (*s) {
        int ch = gfx_proximo_car(&s) - FONTE_PRIM;
        if (e == GFX_FINO) {
            glifo(x, y, FINO[ch], FINO_ALT, c, 1);
            x += FINO_AV[ch];
        } else if (e == GFX_MIUDO) {
            // A miuda (Tiny5) desce um pixel: a base dela fica onde ficaria a
            // de um texto normal encolhido.
            if (negrito) glifo(x, y + 2, MIUDA[ch], MIUDA_ALT, SOMBRA_TEXTO, 1);
            glifo(x, y + 1, MIUDA[ch], MIUDA_ALT, c, 1);
            x += MIUDA_AV[ch];
        } else {
            if (negrito) glifo(x, y + e, FONTE[ch], FONTE_ALT, SOMBRA_TEXTO, e);
            glifo(x, y, FONTE[ch], FONTE_ALT, c, e);
            x += FONTE_AV[ch] * e;
        }
    }
    return x - x0;
}

int gfx_largura(const char *s, int e)
{
    int n = 0;
    while (*s) {
        int ch = gfx_proximo_car(&s) - FONTE_PRIM;
        n += e == GFX_FINO ? FINO_AV[ch] : e == GFX_MIUDO ? MIUDA_AV[ch] : FONTE_AV[ch] * e;
    }
    return n;
}
