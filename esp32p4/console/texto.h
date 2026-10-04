// Texto nitido na tela inteira: as fontes TrueType sao rasterizadas na hora
// (stb_truetype), no tamanho pedido, e cada letra fica guardada para o
// proximo quadro. O console escreve com o Inter; o texto do jogo vem pelo
// gfx_texto_nitido (o mesmo caminho do PC) e sai na Jersey 10, ampliado
// junto com a mesa.
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

// Texto do jogo: o jogo avisa onde vai cada texto (em pixels do jogo); depois
// de ampliar a mesa, o console desenha tudo k vezes maior a partir de (ox, oy).
void texto_jogo_registra(void);
void texto_jogo_trocas(const char *const (*trocas)[2], int n);   // ver catalogo.h
void texto_jogo_limpa(void);
void texto_jogo_desenha(int ox, int oy, int k);
