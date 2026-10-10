// Texto nitido na tela inteira: as fontes TrueType sao rasterizadas na hora
// (stb_truetype), no tamanho pedido, e cada letra fica guardada para o
// proximo quadro. O console escreve com o Inter; o nome do jogo e as letras da
// capa usam a Jersey 10, a fonte do jogo (que desenha o proprio texto em
// pixels, no quadro dele).
#pragma once
#include <stdbool.h>
#include "desenho.h"

enum { FONTE_TEXTO, FONTE_NEGRITO, FONTE_PRETO, FONTE_JOGO, FONTE_QTD };

void texto_inicia(void);
// px: altura da linha em pixels. y: o alto da linha (nao a linha de base).
int  texto(int fonte, float px, int x, int y, const char *s, cor_t c, int alfa);
int  texto_c(int fonte, float px, int cx, int y, const char *s, cor_t c, int alfa);
int  texto_d(int fonte, float px, int xd, int y, const char *s, cor_t c, int alfa);
int  texto_largura(int fonte, float px, const char *s);
// Quebra em palavras dentro de 'larg'; devolve a altura usada.
int  texto_quebra(int fonte, float px, int x, int y, int larg, float entrelinha, const char *s,
                  cor_t c, int alfa);

