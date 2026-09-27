#pragma once
#include <stdint.h>
#include <stdbool.h>

// Linha de resultado: rotulo a esquerda, quantidade e uma terceira coluna
// (percentual ou valor), com uma barra proporcional preenchendo o fundo.
#define TABELA_ROTULO 16

typedef struct {
    // Copia, nao ponteiro: fmt_mes_curto() e amigos devolvem buffers rotativos
    // que seriam sobrescritos antes do desenho.
    char        rotulo[TABELA_ROTULO];
    uint32_t    qtd;
    uint64_t    cent;
    bool        destaque;
} tabela_linha_t;

// Desenha ate (row_fim - row_ini + 1) linhas a partir de 'topo'.
void tabela_desenha(int row_ini, int row_fim, const tabela_linha_t *l, int n,
                    int topo, uint32_t total, bool ver_valor);
