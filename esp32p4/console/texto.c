#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "texto.h"
#include "sistema.h"
#include "fontes_ttf.h"
#include "ui/gfx.h"
#include "ui/fonte_tela.h"
#include "../../desktop/fonte_ttf.h"      // a Jersey 10 do PC (FONTE_TTF)

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb_truetype.h"
#pragma GCC diagnostic pop

typedef struct {
    stbtt_fontinfo info;
    float asc_unid;                      // ascendente em unidades da fonte
    bool ok;
} fonte_t;
static fonte_t fontes[FONTE_QTD];

// ---------------------------------------------------------------------------
// Letras ja rasterizadas, por fonte, tamanho (em quartos de pixel) e codigo.
// Os bitmaps vivem numa arena; quando ela enche, tudo e jogado fora de uma vez.
// ---------------------------------------------------------------------------
typedef struct {
    uint32_t chave;                      // 0: vaga
    int16_t w, h, xo, yo;                // tamanho e posicao em relacao a linha de base
    float avanco;
    uint8_t *alfa;
} glifo_t;

#define N_GLIFOS   4096
#define ARENA_TAM  (3 * 1024 * 1024)
static glifo_t *glifos;
static uint8_t *arena;
static size_t arena_usada;
static int n_glifos;

static void esvazia(void)
{
    memset(glifos, 0, sizeof(glifo_t) * N_GLIFOS);
    arena_usada = 0;
    n_glifos = 0;
}

void texto_inicia(void)
{
    if (glifos) return;
    glifos = sis_aloca(sizeof(glifo_t) * N_GLIFOS);
    arena = sis_aloca(ARENA_TAM);
    esvazia();
    const unsigned char *dados[FONTE_QTD] = { FONTE_INTER, FONTE_INTER_NEGRITO, FONTE_INTER_PRETO, FONTE_TTF };
    for (int f = 0; f < FONTE_QTD; f++) {
        fonte_t *ft = &fontes[f];
        ft->ok = stbtt_InitFont(&ft->info, dados[f], stbtt_GetFontOffsetForIndex(dados[f], 0)) != 0;
        if (!ft->ok) continue;
        int asc, desc, gap;
        stbtt_GetFontVMetrics(&ft->info, &asc, &desc, &gap);
        ft->asc_unid = (float)asc;
    }
}

static const glifo_t *glifo(int f, float px, int cp)
{
    int tam4 = (int)(px * 4 + 0.5f);
    if (tam4 < 4) tam4 = 4;
    if (tam4 > 4095) tam4 = 4095;
    uint32_t chave = ((uint32_t)(f + 1) << 28) | ((uint32_t)tam4 << 16) | (uint32_t)(cp & 0xFFFF);
    uint32_t i = (chave * 2654435761u) >> 20;          // 12 bits
    for (;;) {
        glifo_t *g = &glifos[i];
        if (g->chave == chave) return g;
        if (!g->chave) break;
        i = (i + 1) & (N_GLIFOS - 1);
    }
    fonte_t *ft = &fontes[f];
    float s = stbtt_ScaleForPixelHeight(&ft->info, tam4 / 4.0f);
    int gi = stbtt_FindGlyphIndex(&ft->info, cp);
    if (!gi && cp != ' ') gi = stbtt_FindGlyphIndex(&ft->info, '?');
    int x0, y0, x1, y1, av, lsb;
    stbtt_GetGlyphBitmapBox(&ft->info, gi, s, s, &x0, &y0, &x1, &y1);
    stbtt_GetGlyphHMetrics(&ft->info, gi, &av, &lsb);
    size_t tam = (size_t)(x1 - x0) * (size_t)(y1 - y0);
    if (n_glifos >= N_GLIFOS * 3 / 4 || arena_usada + tam > ARENA_TAM) {
        esvazia();
        return glifo(f, px, cp);
    }
    // depois de esvaziar, a vaga pode ter mudado
    i = (chave * 2654435761u) >> 20;
    while (glifos[i].chave) i = (i + 1) & (N_GLIFOS - 1);
    glifo_t *g = &glifos[i];
    g->chave = chave;
    g->w = (int16_t)(x1 - x0);
    g->h = (int16_t)(y1 - y0);
    g->xo = (int16_t)x0;
    g->yo = (int16_t)y0;
    g->avanco = av * s;
    g->alfa = arena + arena_usada;
    arena_usada += (tam + 3) & ~(size_t)3;
    if (tam) stbtt_MakeGlyphBitmap(&ft->info, g->alfa, g->w, g->h, g->w, s, s, gi);
    n_glifos++;
    return g;
}

static void desenha_glifo(const glifo_t *g, int x, int y, cor_t c, int alfa)
{
    tela_t *t = d_alvo_atual();
    if (!t || !g->w) return;
    int cx0, cy0, cx1, cy1;
    d_recorte_atual(&cx0, &cy0, &cx1, &cy1);
    int gx = x + g->xo, gy = y + g->yo;
    int i0 = gx < cx0 ? cx0 - gx : 0, i1 = gx + g->w > cx1 ? cx1 - gx : g->w;
    int j0 = gy < cy0 ? cy0 - gy : 0, j1 = gy + g->h > cy1 ? cy1 - gy : g->h;
    for (int j = j0; j < j1; j++) {
        const uint8_t *a = g->alfa + j * g->w;
        uint16_t *p = t->px + (gy + j) * t->w + gx;
        for (int i = i0; i < i1; i++) {
            int v = a[i];
            if (!v) continue;
            if (alfa < 255) v = v * alfa / 255;
            p[i] = v >= 255 ? c : d_mistura(p[i], c, v);
        }
    }
}

// Proximo codigo de uma string UTF-8 (ate U+FFFF).
static int proximo(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    int c = *p++;
    if (c >= 0xE0 && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80) {
        c = ((c & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
        p += 2;
        while ((*p & 0xC0) == 0x80) p++;
    } else if (c >= 0xC0 && (p[0] & 0xC0) == 0x80) {
        c = ((c & 0x1F) << 6) | (p[0] & 0x3F);
        p++;
    }
    *s = (const char *)p;
    return c;
}

static float ascendente(int f, float px)
{
    fonte_t *ft = &fontes[f];
    return ft->asc_unid * stbtt_ScaleForPixelHeight(&ft->info, px);
}

int texto_largura(int f, float px, const char *s)
{
    if (f < 0 || f >= FONTE_QTD || !fontes[f].ok || !glifos) return 0;
    float w = 0;
    while (*s) w += glifo(f, px, proximo(&s))->avanco;
    return (int)(w + 0.5f);
}

int texto(int f, float px, int x, int y, const char *s, cor_t c, int alfa)
{
    if (f < 0 || f >= FONTE_QTD || !fontes[f].ok || !glifos) return 0;
    int base = y + (int)(ascendente(f, px) + 0.5f);
    float pena = (float)x;
    while (*s) {
        int cp = proximo(&s);
        if (cp == '\n') break;
        const glifo_t *g = glifo(f, px, cp);
        desenha_glifo(g, (int)(pena + 0.5f), base, c, alfa);
        pena += g->avanco;
    }
    return (int)(pena - x + 0.5f);
}

int texto_c(int f, float px, int cx, int y, const char *s, cor_t c, int alfa)
{
    int w = texto_largura(f, px, s);
    return texto(f, px, cx - w / 2, y, s, c, alfa);
}

int texto_d(int f, float px, int xd, int y, const char *s, cor_t c, int alfa)
{
    int w = texto_largura(f, px, s);
    return texto(f, px, xd - w, y, s, c, alfa);
}

int texto_quebra(int f, float px, int x, int y, int larg, float entrelinha, const char *s, cor_t c, int alfa)
{
    char linha[256], palavra[96], tenta[360];
    int nl = 0, y0 = y;
    linha[0] = 0;
    while (*s) {
        int np = 0;
        while (*s && *s != ' ' && *s != '\n' && np < (int)sizeof palavra - 1) palavra[np++] = *s++;
        palavra[np] = 0;
        snprintf(tenta, sizeof tenta, "%s%s%s", linha, nl ? " " : "", palavra);
        if (nl && texto_largura(f, px, tenta) > larg) {
            texto(f, px, x, y, linha, c, alfa);
            y += (int)(px * entrelinha);
            snprintf(linha, sizeof linha, "%s", palavra);
        } else {
            snprintf(linha, sizeof linha, "%.255s", tenta);
        }
        nl = (int)strlen(linha);
        if (*s == '\n') {
            texto(f, px, x, y, linha, c, alfa);
            y += (int)(px * entrelinha);
            linha[0] = 0;
            nl = 0;
        }
        if (*s) s++;
    }
    if (nl) {
        texto(f, px, x, y, linha, c, alfa);
        y += (int)(px * entrelinha);
    }
    return y - y0;
}

// ---------------------------------------------------------------------------
// Texto do jogo, como no PC (desktop/main.c): cada letra no avanco que o jogo
// mediu, o negrito repetido um tico ao lado.
// ---------------------------------------------------------------------------
typedef struct { int16_t x, y; uint8_t esc; bool negrito; uint16_t cor; int ini; } pedido_t;
static pedido_t pedidos[768];
static int n_pedidos, usado;
static char letras[32768];

static const char *const (*trocas)[2];
static int n_trocas;

void texto_jogo_trocas(const char *const (*t)[2], int n) { trocas = t; n_trocas = n; }

static void recebe(int x, int y, const char *s, uint16_t c, int esc, bool negrito)
{
    for (int i = 0; i < n_trocas; i++)
        if (!strcmp(s, trocas[i][0])) {
            const char *novo = trocas[i][1];
            if (!novo) return;
            x += (gfx_largura(s, esc) - gfx_largura(novo, esc)) / 2;    // mesmo centro
            s = novo;
            break;
        }
    int n = (int)strlen(s) + 1;
    if (n_pedidos >= (int)(sizeof pedidos / sizeof pedidos[0]) || usado + n > (int)sizeof letras) return;
    pedidos[n_pedidos++] = (pedido_t){ (int16_t)x, (int16_t)y, (uint8_t)esc, negrito, c, usado };
    memcpy(letras + usado, s, (size_t)n);
    usado += n;
}

void texto_jogo_registra(void) { gfx_texto_nitido(recebe); }
void texto_jogo_limpa(void) { n_pedidos = usado = 0; }

void texto_jogo_desenha(int ox, int oy, int k)
{
    if (!fontes[FONTE_JOGO].ok) return;
    for (int i = 0; i < n_pedidos; i++) {
        const pedido_t *t = &pedidos[i];
        float e = t->esc == GFX_MIUDO ? GFX_MIUDO_ESC : (float)t->esc;
        float tam = FT_EM * e * k;
        float x = ox + t->x * (float)k;
        int base = oy + t->y * k + (int)(ascendente(FONTE_JOGO, tam) + 0.5f);
        const char *s = letras + t->ini;
        while (*s) {
            int cp = gfx_proximo_car(&s);
            const glifo_t *g = glifo(FONTE_JOGO, tam, cp);
            desenha_glifo(g, (int)(x + 0.5f), base, t->cor, 255);
            if (t->negrito) desenha_glifo(g, (int)(x + k * 0.45f * e + 0.5f), base, t->cor, 255);
            x += FT_AVANCO[cp - FONTE_PRIM] * e * k / 64.0f;
        }
    }
}
