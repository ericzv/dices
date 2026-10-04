#include <math.h>
#include <string.h>
#include "desenho.h"

static tela_t *alvo;
static int rx0, ry0, rx1, ry1;              // recorte: [rx0, rx1) x [ry0, ry1)

void d_alvo(tela_t *t) { alvo = t; d_recorte_tudo(); }
tela_t *d_alvo_atual(void) { return alvo; }

void d_recorte_tudo(void)
{
    rx0 = ry0 = 0;
    rx1 = alvo ? alvo->w : 0;
    ry1 = alvo ? alvo->h : 0;
}

void d_recorte_atual(int *x0, int *y0, int *x1, int *y1)
{
    *x0 = rx0; *y0 = ry0; *x1 = rx1; *y1 = ry1;
}

void d_recorte(int x, int y, int w, int h)
{
    d_recorte_tudo();
    if (x > rx0) rx0 = x;
    if (y > ry0) ry0 = y;
    if (x + w < rx1) rx1 = x + w;
    if (y + h < ry1) ry1 = y + h;
    if (rx1 < rx0) rx1 = rx0;
    if (ry1 < ry0) ry1 = ry0;
}

// Mistura em RGB565 sem desmontar o pixel: verde vai para a metade de cima
// da palavra, e cada campo ganha folga para a multiplicacao por 0..32.
static inline uint32_t expande(uint32_t c) { return (c | (c << 16)) & 0x07E0F81Fu; }
static inline uint16_t contrai(uint32_t v) { v &= 0x07E0F81Fu; return (uint16_t)(v | (v >> 16)); }
#define ARREDONDA 0x02008010u              // meio degrau em cada campo, antes do >> 5

// Pontilhado 4x4: espalha o erro dos 32 niveis de alfa e dos 5/6 bits do
// RGB565, para degrade nao virar faixas.
static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };

cor_t d_mistura(cor_t a, cor_t b, int alfa)
{
    if (alfa <= 0) return a;
    if (alfa >= 255) return b;
    uint32_t k = (uint32_t)(alfa + 4) >> 3;
    return contrai((expande(a) * (32 - k) + expande(b) * k + ARREDONDA) >> 5);
}

cor_t d_clareia(cor_t c, int pct)
{
    if (pct >= 0) return d_mistura(c, 0xFFFF, pct * 255 / 100);
    return d_mistura(c, 0x0000, -pct * 255 / 100);
}

static bool recorta(int *x, int *y, int *w, int *h)
{
    if (!alvo) return false;
    if (*x < rx0) { *w -= rx0 - *x; *x = rx0; }
    if (*y < ry0) { *h -= ry0 - *y; *y = ry0; }
    if (*x + *w > rx1) *w = rx1 - *x;
    if (*y + *h > ry1) *h = ry1 - *y;
    return *w > 0 && *h > 0;
}

// Enche n pixels com a mesma cor, de dois em dois quando da.
static void preenche(uint16_t *p, int n, cor_t c)
{
    if (n > 0 && ((uintptr_t)p & 2)) { *p++ = c; n--; }
    uint32_t dupla = c | ((uint32_t)c << 16), *q = (uint32_t *)p;
    for (int i = 0; i < n / 2; i++) q[i] = dupla;
    if (n & 1) p[n - 1] = c;
}

// Mistura n pixels com a mesma cor e o mesmo alfa (0..32).
static void mistura_linha(uint16_t *p, int n, cor_t c, uint32_t k)
{
    uint32_t vbk = expande(c) * k + ARREDONDA, ik = 32 - k;
    for (int i = 0; i < n; i++) p[i] = contrai((expande(p[i]) * ik + vbk) >> 5);
}

void d_limpa(cor_t c)
{
    if (alvo) d_ret(0, 0, alvo->w, alvo->h, c);
}

void d_ret(int x, int y, int w, int h, cor_t c)
{
    if (!recorta(&x, &y, &w, &h)) return;
    for (int j = 0; j < h; j++) preenche(alvo->px + (y + j) * alvo->w + x, w, c);
}

void d_ret_alfa(int x, int y, int w, int h, cor_t c, int alfa)
{
    if (alfa >= 255) { d_ret(x, y, w, h, c); return; }
    if (alfa <= 0 || !recorta(&x, &y, &w, &h)) return;
    uint32_t k = (uint32_t)(alfa + 4) >> 3;
    if (!k) return;
    for (int j = 0; j < h; j++) mistura_linha(alvo->px + (y + j) * alvo->w + x, w, c, k);
}

void d_pixel_alfa(int x, int y, cor_t c, int alfa)
{
    if (!alvo || x < rx0 || y < ry0 || x >= rx1 || y >= ry1 || alfa <= 0) return;
    uint16_t *p = alvo->px + y * alvo->w + x;
    *p = d_mistura(*p, c, alfa);
}

void d_degrade_v(int x, int y, int w, int h, cor_t c0, cor_t c1)
{
    int y0 = y, h0 = h;
    if (!recorta(&x, &y, &w, &h)) return;
    int r0 = (c0 >> 11) * 255 / 31, g0 = ((c0 >> 5) & 63) * 255 / 63, b0 = (c0 & 31) * 255 / 31;
    int r1 = (c1 >> 11) * 255 / 31, g1 = ((c1 >> 5) & 63) * 255 / 63, b1 = (c1 & 31) * 255 / 31;
    for (int j = 0; j < h; j++) {
        int yy = y + j, q = h0 > 1 ? (yy - y0) * 1024 / (h0 - 1) : 0;
        int r = r0 + (r1 - r0) * q / 1024, g = g0 + (g1 - g0) * q / 1024, b = b0 + (b1 - b0) * q / 1024;
        cor_t quatro[4];
        for (int i = 0; i < 4; i++) quatro[i] = d_cor_pontilhada(r, g, b, i, yy);
        uint16_t *p = alvo->px + yy * alvo->w + x;
        for (int i = 0; i < w; i++) p[i] = quatro[(x + i) & 3];
    }
}

void d_degrade_v_alfa(int x, int y, int w, int h, cor_t c, int a0, int a1)
{
    int y0 = y, h0 = h;
    if (!recorta(&x, &y, &w, &h)) return;
    uint32_t vb = expande(c);
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        int a = h0 > 1 ? a0 + (a1 - a0) * (yy - y0) / (h0 - 1) : a0;
        uint32_t base = (uint32_t)a * 512 / 255;           // alfa em 1/16 de degrau
        uint16_t *p = alvo->px + yy * alvo->w + x;
        for (int i = 0; i < w; i++) {
            uint32_t k = (base + BAYER[yy & 3][(x + i) & 3]) >> 4;
            if (!k) continue;
            p[i] = k >= 32 ? c : contrai((expande(p[i]) * (32 - k) + vb * k + ARREDONDA) >> 5);
        }
    }
}

void d_degrade_h_alfa(int x, int y, int w, int h, cor_t c, int a0, int a1)
{
    int x0 = x, w0 = w;
    if (!recorta(&x, &y, &w, &h)) return;
    uint32_t vb = expande(c);
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        uint16_t *p = alvo->px + yy * alvo->w + x;
        for (int i = 0; i < w; i++) {
            int a = w0 > 1 ? a0 + (a1 - a0) * (x + i - x0) / (w0 - 1) : a0;
            uint32_t k = ((uint32_t)a * 512 / 255 + BAYER[yy & 3][(x + i) & 3]) >> 4;
            if (!k) continue;
            p[i] = k >= 32 ? c : contrai((expande(p[i]) * (32 - k) + vb * k + ARREDONDA) >> 5);
        }
    }
}

// Cor de 8 bits por canal levada ao RGB565 com pontilhado (para degrades
// calculados pixel a pixel, como o feltro da capa).
cor_t d_cor_pontilhada(int r, int g, int b, int x, int y)
{
    int d = BAYER[y & 3][x & 3];
    r += (d * 8) / 16 - 4;
    g += (d * 4) / 16 - 2;
    b += (d * 8) / 16 - 4;
    r = r < 0 ? 0 : r > 255 ? 255 : r;
    g = g < 0 ? 0 : g > 255 ? 255 : g;
    b = b < 0 ? 0 : b > 255 ? 255 : b;
    return COR(r, g, b);
}

// ---------------------------------------------------------------------------
// Formas suavizadas. Cada pixel ganha a parte da cor que a forma cobre,
// calculada pela distancia do centro do pixel ate a borda.
// ---------------------------------------------------------------------------

// Distancia ate uma caixa de meia-largura hx, meia-altura hy e cantos de raio r
// (negativa dentro).
static inline float dist_caixa(float px, float py, float hx, float hy, float r)
{
    float qx = fabsf(px) - hx, qy = fabsf(py) - hy;
    float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
    float fora = (ox > 0 && oy > 0) ? sqrtf(ox * ox + oy * oy) : ox + oy;
    float dentro = qx > qy ? qx : qy;
    if (dentro > 0) dentro = 0;
    return fora + dentro - r;
}

static inline void pinta(uint16_t *p, cor_t c, int alfa, float cobre)
{
    if (cobre <= 0) return;
    int a = cobre >= 1 ? alfa : (int)(alfa * cobre);
    if (a >= 255) *p = c;
    else if (a > 0) *p = d_mistura(*p, c, a);
}

void d_arred(float x, float y, float w, float h, float r, cor_t c, int alfa)
{
    if (!alvo || w <= 0 || h <= 0 || alfa <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    float cx = x + w / 2, cy = y + h / 2, hx = w / 2 - r, hy = h / 2 - r;
    int x0 = (int)floorf(x), y0 = (int)floorf(y), x1 = (int)ceilf(x + w), y1 = (int)ceilf(y + h);
    if (x0 < rx0) x0 = rx0;
    if (y0 < ry0) y0 = ry0;
    if (x1 > rx1) x1 = rx1;
    if (y1 > ry1) y1 = ry1;
    if (x0 >= x1 || y0 >= y1) return;
    // Faixa do meio de cada linha: ali a cobertura so depende da altura.
    int xa = (int)ceilf(cx - hx), xb = (int)floorf(cx + hx - 1) + 1;
    if (xa < x0) xa = x0;
    if (xb > x1) xb = x1;
    for (int j = y0; j < y1; j++) {
        uint16_t *lin = alvo->px + j * alvo->w;
        float py = j + 0.5f - cy, qy = fabsf(py) - hy;
        float dm = (qy > -0.5f ? qy : -0.5f) - r, cm = 0.5f - dm;
        int ini = x0, fim = x1;
        if (xb > xa) {
            if (cm > 0) {
                int a = cm >= 1 ? alfa : (int)(alfa * cm);
                if (a >= 255) preenche(lin + xa, xb - xa, c);
                else if (a > 0) mistura_linha(lin + xa, xb - xa, c, (uint32_t)(a + 4) >> 3);
            }
            for (int i = ini; i < xa; i++)
                pinta(lin + i, c, alfa, 0.5f - dist_caixa(i + 0.5f - cx, py, hx, hy, r));
            ini = xb;
        }
        for (int i = ini; i < fim; i++)
            pinta(lin + i, c, alfa, 0.5f - dist_caixa(i + 0.5f - cx, py, hx, hy, r));
    }
}

void d_arred_borda(float x, float y, float w, float h, float r, float esp, cor_t c, int alfa)
{
    if (!alvo || w <= 0 || h <= 0 || alfa <= 0 || esp <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    float cx = x + w / 2, cy = y + h / 2, hx = w / 2 - r, hy = h / 2 - r;
    float ri = r - esp;
    float hxi = hx, hyi = hy;
    if (ri < 0) { hxi += ri; hyi += ri; ri = 0; }
    int x0 = (int)floorf(x), y0 = (int)floorf(y), x1 = (int)ceilf(x + w), y1 = (int)ceilf(y + h);
    if (x0 < rx0) x0 = rx0;
    if (y0 < ry0) y0 = ry0;
    if (x1 > rx1) x1 = rx1;
    if (y1 > ry1) y1 = ry1;
    // O miolo, bem dentro da borda de dentro, fica como esta.
    int mx0 = (int)ceilf(x + esp + 1), mx1 = (int)floorf(x + w - esp - 1);
    int my0 = (int)ceilf(y + esp + r + 1), my1 = (int)floorf(y + h - esp - r - 1);
    for (int j = y0; j < y1; j++) {
        uint16_t *lin = alvo->px + j * alvo->w;
        float py = j + 0.5f - cy;
        bool pula = j >= my0 && j < my1;
        for (int i = x0; i < x1; i++) {
            if (pula && i >= mx0 && i < mx1) { i = mx1 - 1; continue; }
            float px = i + 0.5f - cx;
            float fora = 0.5f - dist_caixa(px, py, hx, hy, r);
            if (fora <= 0) continue;
            if (fora > 1) fora = 1;
            float dentro = 0.5f - dist_caixa(px, py, hxi, hyi, ri);
            if (dentro < 0) dentro = 0;
            if (dentro > 1) dentro = 1;
            pinta(lin + i, c, alfa, fora - dentro);
        }
    }
}

void d_circulo(float cx, float cy, float r, cor_t c, int alfa)
{
    d_arred(cx - r, cy - r, 2 * r, 2 * r, r, c, alfa);
}

void d_anel(float cx, float cy, float r, float esp, cor_t c, int alfa)
{
    d_arred_borda(cx - r, cy - r, 2 * r, 2 * r, r, esp, c, alfa);
}

void d_linha(float ax, float ay, float bx, float by, float esp, cor_t c, int alfa)
{
    if (!alvo || alfa <= 0) return;
    float m = esp / 2;
    int x0 = (int)floorf(fminf(ax, bx) - m - 1), x1 = (int)ceilf(fmaxf(ax, bx) + m + 1);
    int y0 = (int)floorf(fminf(ay, by) - m - 1), y1 = (int)ceilf(fmaxf(ay, by) + m + 1);
    if (x0 < rx0) x0 = rx0;
    if (y0 < ry0) y0 = ry0;
    if (x1 > rx1) x1 = rx1;
    if (y1 > ry1) y1 = ry1;
    float dx = bx - ax, dy = by - ay, l2 = dx * dx + dy * dy;
    for (int j = y0; j < y1; j++)
        for (int i = x0; i < x1; i++) {
            float px = i + 0.5f - ax, py = j + 0.5f - ay;
            float t = l2 > 0 ? (px * dx + py * dy) / l2 : 0;
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            float ex = px - t * dx, ey = py - t * dy;
            pinta(alvo->px + j * alvo->w + i, c, alfa, 0.5f - (sqrtf(ex * ex + ey * ey) - m));
        }
}

// Poligono convexo: a distancia a borda e a maior das distancias as retas
// dos lados (exata nos lados, um pouco arredondada nas pontas).
void d_poligono(const float *xy, int n, cor_t c, int alfa)
{
    if (!alvo || n < 3 || alfa <= 0) return;
    float area = 0, mnx = xy[0], mxx = xy[0], mny = xy[1], mxy = xy[1];
    for (int k = 0; k < n; k++) {
        int l = (k + 1) % n;
        area += xy[2 * k] * xy[2 * l + 1] - xy[2 * l] * xy[2 * k + 1];
        mnx = fminf(mnx, xy[2 * k]); mxx = fmaxf(mxx, xy[2 * k]);
        mny = fminf(mny, xy[2 * k + 1]); mxy = fmaxf(mxy, xy[2 * k + 1]);
    }
    float sinal = area > 0 ? 1.0f : -1.0f;
    float nx[40], ny[40], nc[40];
    if (n > 40) n = 40;
    for (int k = 0; k < n; k++) {
        int l = (k + 1) % n;
        float ex = xy[2 * l] - xy[2 * k], ey = xy[2 * l + 1] - xy[2 * k + 1];
        float len = sqrtf(ex * ex + ey * ey);
        if (len <= 0) len = 1;
        // normal para fora
        nx[k] = sinal * ey / len;
        ny[k] = -sinal * ex / len;
        nc[k] = -(nx[k] * xy[2 * k] + ny[k] * xy[2 * k + 1]);
    }
    int x0 = (int)floorf(mnx) - 1, x1 = (int)ceilf(mxx) + 1, y0 = (int)floorf(mny) - 1, y1 = (int)ceilf(mxy) + 1;
    if (x0 < rx0) x0 = rx0;
    if (y0 < ry0) y0 = ry0;
    if (x1 > rx1) x1 = rx1;
    if (y1 > ry1) y1 = ry1;
    for (int j = y0; j < y1; j++)
        for (int i = x0; i < x1; i++) {
            float px = i + 0.5f, py = j + 0.5f, d = -1e9f;
            for (int k = 0; k < n; k++) {
                float e = nx[k] * px + ny[k] * py + nc[k];
                if (e > d) d = e;
            }
            pinta(alvo->px + j * alvo->w + i, c, alfa, 0.5f - d);
        }
}

// ---------------------------------------------------------------------------
// Copias
// ---------------------------------------------------------------------------
void d_copia(const uint16_t *src, int sw, int sh, int dx, int dy)
{
    int x = dx, y = dy, w = sw, h = sh;
    if (!recorta(&x, &y, &w, &h)) return;
    for (int j = 0; j < h; j++)
        memcpy(alvo->px + (y + j) * alvo->w + x, src + (y - dy + j) * sw + (x - dx), (size_t)w * 2);
}

void d_amplia(const uint16_t *src, int sw, int sh, int dx, int dy, int k)
{
    if (!alvo || k < 1) return;
    // Caso comum (o jogo inteiro em 2x, sem recorte): duas linhas por linha,
    // cada pixel virando uma dupla de 32 bits.
    if (k == 2 && dx >= rx0 && dy >= ry0 && dx + sw * 2 <= rx1 && dy + sh * 2 <= ry1 && !((uintptr_t)(alvo->px + dy * alvo->w + dx) & 3)
        && !(alvo->w & 1)) {
        for (int j = 0; j < sh; j++) {
            const uint16_t *s = src + j * sw;
            uint32_t *d = (uint32_t *)(alvo->px + (dy + 2 * j) * alvo->w + dx);
            for (int i = 0; i < sw; i++) d[i] = s[i] | ((uint32_t)s[i] << 16);
            memcpy(alvo->px + (dy + 2 * j + 1) * alvo->w + dx, d, (size_t)sw * 4);
        }
        return;
    }
    d_estica(src, sw, sh, dx, dy, sw * k, sh * k);
}

void d_estica(const uint16_t *src, int sw, int sh, int dx, int dy, int dw, int dh)
{
    int x = dx, y = dy, w = dw, h = dh;
    if (dw <= 0 || dh <= 0 || !recorta(&x, &y, &w, &h)) return;
    for (int j = 0; j < h; j++) {
        const uint16_t *s = src + (int)((int64_t)(y - dy + j) * sh / dh) * sw;
        uint16_t *d = alvo->px + (y + j) * alvo->w + x;
        for (int i = 0; i < w; i++) d[i] = s[(int64_t)(x - dx + i) * sw / dw];
    }
}
