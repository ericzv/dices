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

// Texto em UTF-8 (acentos do Latin-1). 'esc' amplia cada pixel da fonte;
// 'negrito' repete o glifo um pixel ao lado. Devolve a largura em pixels.
int gfx_texto(int x, int y, const char *s, uint16_t c, int esc, bool negrito);
int gfx_largura(const char *s, int esc);

// Texto nitido: a plataforma que sabe desenhar texto na resolucao da tela
// (o PC e o navegador) registra aqui quem recebe cada texto; o jogo entao so
// avisa onde ele vai, e mede as larguras pela fonte Jersey 10. Sem registro
// (testes sem janela), o texto e desenhado em pixels no framebuffer.
typedef void (*gfx_texto_fn)(int x, int y, const char *s, uint16_t c, int esc, bool negrito);
void gfx_texto_nitido(gfx_texto_fn fn);
gfx_texto_fn gfx_texto_nitido_atual(void);

// Desvia todo o desenho para outro buffer de GFX_W x GFX_H (NULL volta a tela).
void gfx_desvia(uint16_t *buf);
// Proximo caractere de uma string UTF-8, como codigo Latin-1.
int gfx_proximo_car(const char **s);
#define GFX_ALT_TEXTO(esc) (FONTE_ALT * (esc))
