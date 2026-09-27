#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../board.h"
#include "../hal/display.h"
#include "font6x12.h"
#include "icones.h"
#include "gfx.h"

#define FB display_fb()


void gfx_clear(uint16_t c)
{
    uint16_t *p = FB;
    for (int i = 0; i < LCD_W * LCD_H; i++) p[i] = c;
}

void gfx_rect(int x, int y, int w, int h, uint16_t c)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_W) w = LCD_W - x;
    if (y + h > LCD_H) h = LCD_H - y;
    if (w <= 0 || h <= 0) return;
    uint16_t *p = FB;
    for (int j = 0; j < h; j++) {
        uint16_t *row = p + (y + j) * LCD_W + x;
        for (int i = 0; i < w; i++) row[i] = c;
    }
}

void gfx_divisoria(int row, uint16_t c)
{
    gfx_rect(0, GY(row) + FONT_H / 2, LCD_W, 1, c);
}

void gfx_faixa(int row, uint16_t c)
{
    gfx_rect(0, GY(row) - 1, LCD_W, FONT_H, c);
}

void gfx_faixa_px(int row, int w_px, uint16_t c)
{
    if (w_px > 0) gfx_rect(0, GY(row) - 1, w_px, FONT_H, c);
}

void gfx_faixas(int row_ini, int row_fim, uint16_t c)
{
    gfx_rect(0, GY(row_ini) - 1, LCD_W, (row_fim - row_ini + 1) * FONT_H, c);
}

void gfx_stripe(int row, uint16_t c)
{
    gfx_rect(0, GY(row) - 1, 2, FONT_H, c);
}

static int draw_char(int x, int y, uint8_t ch, uint16_t fg, uint16_t bg, bool bold)
{
    if (ch < FONT_FIRST) ch = '?';
    const uint8_t *g = bold ? font6x12_bold[ch - FONT_FIRST]
                            : font6x12_regular[ch - FONT_FIRST];
    uint16_t *p = FB;
    for (int j = 0; j < FONT_H; j++) {
        int yy = y + j;
        if (yy < 0 || yy >= LCD_H) continue;
        uint8_t bits = g[j];
        uint16_t *row = p + yy * LCD_W;
        for (int i = 0; i < FONT_W; i++) {
            int xx = x + i;
            if (xx < 0 || xx >= LCD_W) continue;
            if (bits & (0x20 >> i)) row[xx] = fg;
            else if (bg != COL_NONE) row[xx] = bg;
        }
    }
    return FONT_W;
}

int gfx_text(int x, int y, const char *s, uint16_t fg, uint16_t bg, bool bold)
{
    int x0 = x;
    for (; *s; s++) x += draw_char(x, y, (uint8_t)*s, fg, bg, bold);
    return x - x0;
}

int gfx_text_escala(int x, int y, const char *s, int e, uint16_t c)
{
    int x0 = x;
    for (; *s; s++) {
        uint8_t ch = (uint8_t)*s;
        if (ch < FONT_FIRST) ch = '?';
        const uint8_t *g = font6x12_bold[ch - FONT_FIRST];
        for (int j = 0; j < FONT_H; j++)
            for (int i = 0; i < FONT_W; i++)
                if (g[j] & (0x20 >> i))
                    gfx_rect(x + i * e, y + j * e, e, e, c);
        x += FONT_W * e;
    }
    return x - x0;
}

void gfx_at(int col, int row, const char *s, uint16_t fg, bool bold)
{
    gfx_text(GX(col), GY(row), s, fg, COL_NONE, bold);
}

void gfx_atf(int col, int row, uint16_t fg, bool bold, const char *fmt, ...)
{
    char buf[GRID_COLS * 2 + 1];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    gfx_at(col, row, buf, fg, bold);
}

void gfx_right(int col_fim, int row, const char *s, uint16_t fg, bool bold)
{
    int n = strlen(s);
    gfx_at(col_fim - n + 1, row, s, fg, bold);
}

void gfx_rightf(int col_fim, int row, uint16_t fg, bool bold, const char *fmt, ...)
{
    char buf[GRID_COLS * 2 + 1];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    gfx_right(col_fim, row, buf, fg, bold);
}

void gfx_centro(int row, const char *s, uint16_t fg, bool bold)
{
    int n = strlen(s);
    gfx_at((GRID_COLS - n) / 2, row, s, fg, bold);
}

// --- formatacao ------------------------------------------------------------

const char *fmt_int(uint64_t v)
{
    static char b[8][28];
    static int n;
    char *out = b[n = (n + 1) & 7];
    char tmp[24];
    int len = snprintf(tmp, sizeof tmp, "%llu", (unsigned long long)v);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (i && (len - i) % 3 == 0) out[j++] = '.';
        out[j++] = tmp[i];
    }
    out[j] = 0;
    return out;
}

// Centavos -> "R$ 1,2 mi" / "R$ 384 mil" / "R$ 1.284"
const char *fmt_reais(uint64_t cent)
{
    static char b[4][28];
    static int n;
    char *out = b[n = (n + 1) & 3];
    uint64_t r = cent / 100;
    if (r >= 1000000ULL)
        snprintf(out, 28, "R$ %llu,%llu mi", (unsigned long long)(r / 1000000),
                 (unsigned long long)((r / 100000) % 10));
    else if (r >= 10000ULL)
        snprintf(out, 28, "R$ %llu mil", (unsigned long long)(r / 1000));
    else
        snprintf(out, 28, "R$ %s", fmt_int(r));
    return out;
}

const char *fmt_periodo(uint16_t p)
{
    static char b[4][10];
    static int n;
    char *out = b[n = (n + 1) & 3];
    snprintf(out, 10, "%02d/%04d", p % 12 + 1, p / 12);
    return out;
}

const char *fmt_mes_curto(uint16_t p)
{
    static const char *M[] = {"jan","fev","mar","abr","mai","jun",
                              "jul","ago","set","out","nov","dez"};
    static char b[8][8];
    static int n;
    char *out = b[n = (n + 1) & 7];
    snprintf(out, 8, "%s/%02d", M[p % 12], (p / 12) % 100);
    return out;
}

// Media mensal com uma casa decimal, sem ponto flutuante.
const char *fmt_media(uint32_t total, int meses)
{
    static char b[2][16];
    static int n;
    char *out = b[n = (n + 1) & 1];
    if (meses <= 0) { out[0] = 0; return out; }
    uint32_t d = (total * 10u) / (uint32_t)meses;
    snprintf(out, 16, "%u,%u", (unsigned)(d / 10), (unsigned)(d % 10));
    return out;
}

// --- icones ----------------------------------------------------------------

static uint16_t mistura(uint16_t fg, uint16_t bg, int a)
{
    int fr = (fg >> 11) & 0x1F, fv = (fg >> 5) & 0x3F, fa = fg & 0x1F;
    int br = (bg >> 11) & 0x1F, bv = (bg >> 5) & 0x3F, ba = bg & 0x1F;
    int r = br + (fr - br) * a / 3;
    int v = bv + (fv - bv) * a / 3;
    int z = ba + (fa - ba) * a / 3;
    return (uint16_t)((r << 11) | (v << 5) | z);
}

void gfx_icone(int x, int y, const uint8_t *ic, uint16_t fg, uint16_t bg)
{
    uint16_t *p = FB;
    for (int j = 0; j < ICONE_H; j++) {
        int yy = y + j;
        if (yy < 0 || yy >= LCD_H) continue;
        const uint8_t *linha = ic + j * ICONE_STRIDE;
        for (int i = 0; i < ICONE_W; i++) {
            int a = (linha[i >> 2] >> (6 - 2 * (i & 3))) & 3;
            if (!a) continue;
            int xx = x + i;
            if (xx < 0 || xx >= LCD_W) continue;
            p[yy * LCD_W + xx] = mistura(fg, bg, a);
        }
    }
}

// --- bateria ---------------------------------------------------------------

void gfx_bateria(int x, int y, int pct, uint16_t fg, uint16_t baixo)
{
    uint16_t c = (pct >= 0 && pct <= 20) ? baixo : fg;
    gfx_rect(x, y, 12, 7, c);
    gfx_rect(x + 1, y + 1, 10, 5, COL_FUNDO);
    gfx_rect(x + 12, y + 2, 2, 3, c);
    if (pct < 0) {
        gfx_rect(x + 3, y + 3, 6, 1, c);
        return;
    }
    int w = (pct * 10 + 50) / 100;
    if (w > 0) gfx_rect(x + 1, y + 1, w, 5, c);
}

// --- primitivas para os jogos ----------------------------------------------

void gfx_pixel(int x, int y, uint16_t c)
{
    if (x < 0 || y < 0 || x >= LCD_W || y >= LCD_H) return;
    FB[y * LCD_W + x] = c;
}

void gfx_linha(int x0, int y0, int x1, int y1, uint16_t c)
{
    int dx = x1 - x0, dy = y1 - y0;
    int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    int err = dx - dy;
    for (;;) {
        gfx_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void gfx_disco(int cx, int cy, int r, uint16_t c)
{
    for (int dy = -r; dy <= r; dy++) {
        int w = 0;
        while ((w + 1) * (w + 1) + dy * dy <= r * r) w++;
        if (w) gfx_rect(cx - w, cy + dy, w * 2 + 1, 1, c);
    }
}
