// DADO EM CASA - duelo de apostas com dados, para dois jogadores.
//
// Cada rodada, os dois montam uma FILA de quatro dados e jogam um por um.
// Depois de cada dado: parar ou arriscar o proximo. O primeiro sempre vale; os
// seguintes precisam ser IGUAIS OU MAIORES que o ultimo dado que vale. Se sair
// menor, o dado novo e o ultimo que valia sao anulados juntos. Anulado tudo, o
// proximo dado recomeca a fila sozinho.
//
//   * A ORDEM da fila decide o risco: dado grande na frente levanta a barra.
//   * QUEM VENCEU A ULTIMA RODADA JOGA PRIMEIRO - no escuro. Quem joga depois
//     sabe o alvo. A vantagem de informacao vai para quem perdeu.
//   * A APOSTA acontece entre os dois turnos, com a pontuacao do primeiro na
//     mesa.
//
// Os poderes que mexem no rival resolvem no fim, um de cada vez e animados:
// um traco de quem ataca ate o dado atingido, o dado roubado atravessando a
// mesa. Primeiro os poderes de quem jogou primeiro; um dado anulado por um
// ataque nao dispara o proprio poder depois.
//
// Versao PC: mesa de 640x360 (a do Cardputer, 240x135, esta em
// cardputer/dado.c). Os textos sao UTF-8, com acentos.
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_timer.h"
#include "../ui/gfx.h"
#include "../hal/keyboard.h"
#include "../hal/som.h"
#include "../hal/display.h"
#include "../hal/salva.h"
#include "jogos.h"

#ifndef DADO_VERSAO                  // o CMake passa o commit; sem ele, "dev"
#define DADO_VERSAO "dev"
#endif

// Regras e medidas da mesa
#define ANTE        5                    // a entrada do comeco; depois sobe (ante_da())
#define FICHAS_INI  100
#define MAX_RODADAS 15
#define MAX_COL     48             // espaco para a colecao (o Desafiante nao tem limite)
#define LIM_COL     24             // dados que um jogador pode ter na partida normal
#define MAO         8              // espaco da mao (compra 7; 8 com a Bolsa Funda)
#define MAO_NORMAL  7              // quantos compra da bolsa por rodada
#define N_FILA      4              // a fila leva de 1 a 4

// Mesa de 640x360: faixa do rival em cima, a sua embaixo, feltro no meio.
#define FAIXA_H    44
#define MESA_X0    14
#define MESA_X1    626
#define MESA_Y0    (FAIXA_H + 8)
#define MESA_Y1    (GFX_H - FAIXA_H - 8)
#define LINHA_CIMA  98
#define LINHA_BAIXO 262
#define CENTRO     180
#define R_DADO     18                       // raio de um dado na mesa
#define SLOT_X(k)  (218 + 68 * (k))
// Dados roubados, a esquerda da fila e um pouco menores: cabem os quatro
// (centros 170, 136, 102, 68) entre a borda da mesa e o primeiro lugar.
#define R_ROUBO    15
#define ROUBO_X(n) (170 - 34 * (n))
#define PLACAR_X   540
#define POTE_X     586
#define TXT_H      FONTE_ALT                // altura de uma linha de texto

// O resto mora em dado/, um arquivo por assunto, na ordem em que um usa o
// outro. Tudo vira uma so unidade de compilacao: as funcoes sao static, e
// os testes incluem este arquivo para alcancar as de dentro.
#include "dado/cores.inc"         // a paleta fechada e as cores com nome
#include "dado/dados.inc"         // os tipos de dado, seus poderes, textos e precos
#include "dado/estado.inc"        // o estado da partida inteira
#include "dado/util.inc"          // sorteio, quem e quem na mesa, texto centrado
#include "dado/interface.inc"     // paineis, botoes, numeros flutuantes e o mouse
#include "dado/desenho_dado.inc"  // o desenho de cada dado
#include "dado/fila.inc"          // a regra da fila: o que vale, o que anula
#include "dado/desfecho.inc"      // os poderes que mexem no rival, em sequencia
#include "dado/partida.inc"       // comeco, rodada, resultado e fim
#include "dado/aposta.inc"        // a aposta entre os dois turnos
#include "dado/premio.inc"        // o sorteio e a roleta do premio
#include "dado/ia.inc"            // o aparelho como adversario
#include "dado/online.inc"        // o rival pela rede
#include "dado/passo.inc"         // o passo de tempo
#include "dado/mesa.inc"          // o desenho da mesa
#include "dado/telas.inc"         // as telas da partida
#include "dado/catalogo.inc"      // os dados da casa
#include "dado/menus.inc"         // a abertura e os menus
#include "dado/tutorial.inc"      // como jogar
#include "dado/avisos.inc"        // os avisos sobre a mesa
#include "dado/desafio.inc"       // o Modo Desafiante
#include "dado/quadro.inc"        // o quadro de cada fase
#include "dado/teclado.inc"       // teclado e mouse

int dado_vencedor(void) { return fase == F_FIM ? venc_partida : -1; }

// Para o simulador de host.
int  dado_dbg_fase(void)     { return fase; }
int  dado_dbg_vez(void)      { return vez; }
int  dado_dbg_primeiro(void) { return primeiro; }
int  dado_dbg_fichas(int j)  { return J[j].fichas; }
int  dado_dbg_pool(int j)    { return J[j].n_mao; }
int  dado_dbg_tipo(int j, int i) { return J[j].col[J[j].mao[i]].tipo; }
int  dado_dbg_col(int j)     { return J[j].n_col; }
int  dado_dbg_bolsa_n(int j) { return na_bolsa(&J[j]); }
int  dado_dbg_lados(int t)   { return TIPO[t].lados; }
int  dado_dbg_pontos(int j)  { return F[j].pontos; }
int  dado_dbg_total(int j)   { return F[j].total; }
int  dado_dbg_lancados(int j){ return F[j].lancados; }
int  dado_dbg_risco(int j)   { return risco(j); }
int  dado_dbg_rodada(void)   { return rodada; }
int  dado_dbg_pote(void)     { return pote; }
int  dado_dbg_venc_mao(void) { return venc_mao; }
int  dado_dbg_pronta(void)   { return abertura_pronta; }
int  dado_dbg_ver(void)      { return ver_tipo; }
const char *dado_dbg_veredito(void) { return veredito; }
int  dado_dbg_n_ev(void)     { return n_ev; }
int  dado_dbg_sorteia(void)  { return sorteia_tipo(); }
const char *dado_dbg_cat_nome(void) { return cat_sec == 0 ? TIPO[cat_tipo(cat_i)].nome : "amuleto"; }
int  dado_dbg_ev_tipo(void)  { return ev_i < n_ev ? ev[ev_i].tipo : -1; }
void dado_dbg_cpu(int mascara) { cpu = mascara; }
int  dado_dbg_n_tipos(void) { return D_BASICOS; }   // os da partida comum
void dado_dbg_ia_esforco(int e) { ia_esforco = e; }
void dado_dbg_tutorial(int pag) { tut_pag = pag; t_fase = 2.5f; fase = F_TUTORIAL; }
int  dado_dbg_margem(void)
{
    desfecho_t d;
    calcula_desfecho(&d, false);
    return d.total[vez] - d.total[outro(vez)];
}
// Troca a colecao e refaz a compra (para montar cenas de teste).
void dado_dbg_bolsa(int j, const int *tipos, int n)
{
    J[j].n_col = 0;
    for (int i = 0; i < n && i < MAX_COL; i++) {
        J[j].col[J[j].n_col] = nova_peca(tipos[i]);
        J[j].onde[J[j].n_col++] = NA_BOLSA;
    }
    J[j].n_mao = 0;
    compra(&J[j]);
}
