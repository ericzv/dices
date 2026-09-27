#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "font6x12.h"

// Grade de texto: 240/6 = 40 colunas, 11 linhas de 12 px (1 px de margem).
#define GRID_COLS 40
#define GRID_ROWS 11
#define GX(c) ((c) * FONT_W)
#define GY(r) (1 + (r) * FONT_H)
#define GFX_W (GRID_COLS * FONT_W)   // 240
#define GFX_H 135

// Paleta (RGB565): branco sobre preto, tres cinzas de estrutura, tres niveis
// de texto e um unico acento dessaturado. Nenhuma cor primaria.
#define COL_FUNDO    0x0000   // #000000  fundo
#define COL_PAINEL   0x0862   // #0D0F11  painel discreto
#define COL_PAINEL2  0x18E4   // #191C20  cabecalho, rodape, linha selecionada
#define COL_LINHA    0x2966   // #2B2F35  divisorias
#define COL_TEXTO    0xEF5D   // #E9EBED  texto principal
#define COL_TEXTO2   0x9D15   // #9BA1A8  texto secundario
#define COL_TEXTO3   0x5B0D   // #5B6169  rotulos e dicas
#define COL_ACENTO   0x8D99   // #8FB3CC  azul-aco discreto: titulos, selecao, destaque
#define COL_CIANO    0xEF5D   // #E9EBED  valores: branco, sem segunda cor
#define COL_BARRA    0x1904   // #1B2026  barra proporcional
#define COL_BARRA2   0x2987   // #2B333C  barra da linha em destaque
#define COL_ALERTA   0x8D99   // #8FB3CC  erro usa o mesmo acento; nenhuma cor primaria na UI
// Jogos: um par frio/quente dessaturado, legivel a distancia sem virar circo.
#define COL_P1       0x85BB   // #86B5D9  jogador 1
#define COL_P2       0xDD4D   // #D9A86A  jogador 2
#define COL_TERRA    0x2985   // #2B322C  massa do terreno
#define COL_CROSTA   0x5B4B   // #5D6A5D  crosta do terreno

#define COL_NONE     0x0001   // sentinela: nao pinta o fundo do glifo

void gfx_clear(uint16_t c);
void gfx_rect(int x, int y, int w, int h, uint16_t c);

// Blocos alinhados a grade de texto
void gfx_faixa(int row, uint16_t c);                 // linha inteira
void gfx_faixa_px(int row, int w_px, uint16_t c);    // parte da linha, em px
void gfx_faixas(int row_ini, int row_fim, uint16_t c);
void gfx_stripe(int row, uint16_t c);                // filete de 2 px a esquerda
void gfx_divisoria(int row, uint16_t c);             // 1 px no meio da linha

int  gfx_text(int x, int y, const char *s, uint16_t fg, uint16_t bg, bool bold);
void gfx_at(int col, int row, const char *s, uint16_t fg, bool bold);
void gfx_atf(int col, int row, uint16_t fg, bool bold, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));
void gfx_right(int col_fim, int row, const char *s, uint16_t fg, bool bold);
void gfx_rightf(int col_fim, int row, uint16_t fg, bool bold, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));
void gfx_centro(int row, const char *s, uint16_t fg, bool bold);

const char *fmt_int(uint64_t v);
const char *fmt_reais(uint64_t cent);
const char *fmt_periodo(uint16_t p);
const char *fmt_mes_curto(uint16_t p);
const char *fmt_media(uint32_t total, int meses);

// Desenha um icone 28x28 com alfa de 2 bits, misturando fg sobre bg.
void gfx_icone(int x, int y, const uint8_t *ic, uint16_t fg, uint16_t bg);

// Pilha de 14x7 px com nivel preenchido. pct < 0 desenha um traco (sem leitura).
void gfx_bateria(int x, int y, int pct, uint16_t fg, uint16_t baixo);

// Primitivas de pixel usadas pelos jogos.
// Desenha texto ampliando cada pixel do glifo por 'escala'.
int  gfx_text_escala(int x, int y, const char *s, int escala, uint16_t c);

void gfx_pixel(int x, int y, uint16_t c);
void gfx_linha(int x0, int y0, int x1, int y1, uint16_t c);
void gfx_disco(int cx, int cy, int r, uint16_t c);
