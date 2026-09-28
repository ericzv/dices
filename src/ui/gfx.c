#include <stdlib.h>
#include "../hal/display.h"
#include "gfx.h"
#include "fonte_tela.h"

static gfx_texto_fn nitido;
void gfx_texto_nitido(gfx_texto_fn fn) { nitido = fn; }
gfx_texto_fn gfx_texto_nitido_atual(void) { return nitido; }

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

static void glifo(int x, int y, int ch, uint16_t c, int e)
{
    const uint8_t *g = FONTE[ch - FONTE_PRIM];
    for (int j = 0; j < FONTE_ALT; j++) {
        uint8_t b = g[j];
        for (int i = 0; b; i++, b <<= 1)
            if (b & 0x80) {
                if (e == 1) gfx_pixel(x + i, y + j, c);
                else gfx_rect(x + i * e, y + j * e, e, e, c);
            }
    }
}

int gfx_texto(int x, int y, const char *s, uint16_t c, int e, bool negrito)
{
    if (nitido) { nitido(x, y, s, c, e, negrito); return gfx_largura(s, e); }
    int x0 = x;
    while (*s) {
        int ch = gfx_proximo_car(&s);
        glifo(x, y, ch, c, e);
        if (negrito) glifo(x + 1, y, ch, c, e);
        x += FONTE_LARG * e;
    }
    return x - x0;
}

int gfx_largura(const char *s, int e)
{
    int n = 0;
    if (nitido) {                                 // soma os avancos da Jersey 10
        while (*s) n += FT_AVANCO[gfx_proximo_car(&s) - FONTE_PRIM];
        return (n * e + 32) / 64;
    }
    while (*s) { gfx_proximo_car(&s); n++; }
    return n * FONTE_LARG * e;
}
