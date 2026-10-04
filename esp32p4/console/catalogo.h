// Os jogos instalados no console. Cada jogo entra com um adaptador (como
// jogo_dado.c): o que mostrar no menu e como rodar.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "desenho.h"

typedef struct {
    const char *id;              // espaco do salvamento ("dado"): ate 15 letras
    const char *nome;            // "Dado em Casa"
    const char *descricao;       // duas ou tres linhas para o menu
    const char *etiquetas[3];    // chips curtos: "1 ou 2 jogadores", ...

    void (*capa)(tela_t *t);                 // arte de fundo do menu, na tela inteira
    void (*icone)(tela_t *t);                // arte do quadrado da fileira (t->w x t->h)
    void (*logo)(int x, int y, int alfa);    // o nome do jogo no estilo dele

    void (*abre)(void);                      // comeca do zero
    void (*fecha)(void);                     // larga a partida (o que ele guarda, fica)
    void (*passo)(float dt);
    const uint16_t *(*desenha)(int *w, int *h);   // um quadro; devolve o framebuffer dele
    void (*toque)(int x, int y, int acao);   // em pixels do jogo: 0 moveu, 1 clique, 2 voltar
    void (*tecla)(int k);                    // teclado da tela (hal/keyboard.h)
    bool (*digitando)(void);                 // pede o teclado da tela
    bool (*detalhe)(void);                   // segurar o dedo: "mostra tudo sobre isto"

    // Opcoes do jogo que aparecem nos Ajustes (liga/desliga).
    int n_opcoes;
    const char *opcoes[4];
    bool (*opcao_le)(int i);
    void (*opcao_troca)(int i);

    // Textos do jogo trocados na tela ({original, novo}; novo NULL some).
    // Servem para as dicas de teclado virarem dicas de toque.
    const char *const (*trocas)[2];
    int n_trocas;
} jogo_t;

extern const jogo_t *const CATALOGO[];
extern const int N_CATALOGO;
