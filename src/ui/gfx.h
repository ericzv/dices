// Desenho em pixels no framebuffer RGB565 do PC: 640x360, ampliado em escala
// inteira pela janela (3x num monitor Full HD).
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "fonte.h"

#define GFX_W 640
#define GFX_H 360

#define RGB(r, g, b) (uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

void gfx_clear(uint16_t c);
void gfx_rect(int x, int y, int w, int h, uint16_t c);
void gfx_rect_alfa(int x, int y, int w, int h, uint16_t c, int alfa);  // alfa 0..255
void gfx_moldura(int x, int y, int w, int h, int esp, uint16_t c);
void gfx_pixel(int x, int y, uint16_t c);
void gfx_linha(int x0, int y0, int x1, int y1, uint16_t c);
void gfx_linha_grossa(int x0, int y0, int x1, int y1, int esp, uint16_t c);
void gfx_disco(int cx, int cy, int r, uint16_t c);
uint16_t gfx_mistura(uint16_t a, uint16_t b, int pct);   // pct 0 = a, 100 = b

// Texto em UTF-8 (acentos do Latin-1), em pixels no proprio framebuffer, no
// mesmo grid da mesa. 'esc' amplia cada pixel da fonte (1 = Jersey 10 no
// tamanho nativo, linha de 20 px; 2 = o dobro). esc = GFX_MIUDO (0): a fonte
// miuda (Tiny5), linha de 12 px. esc = GFX_FINO (-1): a fonte fina do texto
// corrido (DotGothic16), linha de 20 px. 'negrito' poe uma sombra de um pixel
// embaixo. Devolve a largura em pixels.
#define GFX_MIUDO 0
#define GFX_FINO  (-1)
int gfx_texto(int x, int y, const char *s, uint16_t c, int esc, bool negrito);
int gfx_largura(const char *s, int esc);

// Paleta fechada: depois de gfx_paleta, gfx_quantiza troca cada pixel do
// quadro pela cor mais proxima da paleta (pela distancia "redmean", que pesa
// as cores como o olho). Assim misturas, transparencias e sombras caem
// sempre em cores da paleta. Sem paleta, gfx_quantiza nao faz nada.
void gfx_paleta(const uint16_t *cores, int n);
void gfx_quantiza(void);
uint16_t gfx_na_paleta(uint16_t c);

// Desvia todo o desenho para outro buffer de GFX_W x GFX_H (NULL volta a tela).
void gfx_desvia(uint16_t *buf);
// Proximo caractere de uma string UTF-8, como codigo Latin-1.
int gfx_proximo_car(const char **s);
#define GFX_ALT_TEXTO(esc) (FONTE_ALT * (esc))
