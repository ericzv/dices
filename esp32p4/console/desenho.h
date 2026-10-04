// Desenho do console na tela inteira (1280x800, RGB565): retangulos, cantos
// arredondados, circulos e linhas com borda suavizada, degrades e copias de
// imagem. O jogo continua desenhando no seu proprio quadro (640x360, ui/gfx);
// isto aqui e o desenho do menu, dos ajustes e das barras por cima do jogo.
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define TELA_W 1280
#define TELA_H 800

typedef uint16_t cor_t;
#define COR(r, g, b) ((cor_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

// Uma imagem RGB565 de w x h, linha a linha, sem folga entre as linhas.
typedef struct {
    uint16_t *px;
    int w, h;
} tela_t;

void d_alvo(tela_t *t);                          // onde o desenho vai parar
tela_t *d_alvo_atual(void);
void d_recorte(int x, int y, int w, int h);      // so desenha dentro disto
void d_recorte_tudo(void);
void d_recorte_atual(int *x0, int *y0, int *x1, int *y1);

cor_t d_mistura(cor_t fundo, cor_t c, int alfa); // alfa 0..255: quanto de c
cor_t d_clareia(cor_t c, int pct);               // pct > 0 clareia, < 0 escurece

void d_limpa(cor_t c);
void d_ret(int x, int y, int w, int h, cor_t c);
void d_ret_alfa(int x, int y, int w, int h, cor_t c, int alfa);
void d_degrade_v(int x, int y, int w, int h, cor_t c0, cor_t c1);
void d_degrade_v_alfa(int x, int y, int w, int h, cor_t c, int a0, int a1);
void d_degrade_h_alfa(int x, int y, int w, int h, cor_t c, int a0, int a1);
void d_pixel_alfa(int x, int y, cor_t c, int alfa);
cor_t d_cor_pontilhada(int r, int g, int b, int x, int y);   // 8 bits por canal, com pontilhado

// Formas com a borda suavizada (coordenadas em ponto flutuante).
void d_arred(float x, float y, float w, float h, float r, cor_t c, int alfa);
void d_arred_borda(float x, float y, float w, float h, float r, float esp, cor_t c, int alfa);
void d_circulo(float cx, float cy, float r, cor_t c, int alfa);
void d_anel(float cx, float cy, float r, float esp, cor_t c, int alfa);
void d_linha(float x0, float y0, float x1, float y1, float esp, cor_t c, int alfa);
void d_poligono(const float *xy, int n, cor_t c, int alfa);   // convexo, ate 40 pontos (x, y)

// Copias de imagem: 1:1, ampliada k vezes sem filtro, ou esticada.
void d_copia(const uint16_t *src, int sw, int sh, int dx, int dy);
void d_amplia(const uint16_t *src, int sw, int sh, int dx, int dy, int k);
void d_estica(const uint16_t *src, int sw, int sh, int dx, int dy, int dw, int dh);
