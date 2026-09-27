// Interface dos jogos com a plataforma. O mesmo dado.c roda no Cardputer e
// no PC: quem chama estas funcoes e o laco principal de cada um.
#pragma once
#include <stdbool.h>
#include "../hal/keyboard.h"

// DADO EM CASA
void dado_inicia(int n_partidas);        // n_partidas decide quem abre
void dado_passo(float dt);               // avanca a animacao dt segundos
void dado_desenha(void);                 // pinta o quadro inteiro no framebuffer
void dado_tecla(const key_event_t *ev);
int  dado_vencedor(void);                // -1 em jogo, 0/1 vencedor, 2 empate

// Espiadas no estado, para os testes de host.
int  dado_dbg_fase(void);
int  dado_dbg_vez(void);
int  dado_dbg_primeiro(void);
int  dado_dbg_fichas(int j);
int  dado_dbg_pool(int j);
int  dado_dbg_tipo(int j, int i);
int  dado_dbg_col(int j);
int  dado_dbg_bolsa_n(int j);
int  dado_dbg_lados(int t);
int  dado_dbg_pontos(int j);
int  dado_dbg_total(int j);
int  dado_dbg_lancados(int j);
int  dado_dbg_risco(int j);
int  dado_dbg_rodada(void);
int  dado_dbg_pote(void);
int  dado_dbg_venc_mao(void);
int  dado_dbg_pronta(void);
int  dado_dbg_ver(void);
const char *dado_dbg_veredito(void);
int  dado_dbg_n_ev(void);
int  dado_dbg_sorteia(void);
const char *dado_dbg_cat_nome(void);
int  dado_dbg_ev_tipo(void);
int  dado_dbg_margem(void);
void dado_dbg_bolsa(int j, const int *tipos, int n);
void dado_dbg_cpu(int mascara);          // bit j: jogador j e do aparelho
