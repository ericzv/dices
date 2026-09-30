// Interface dos jogos com a plataforma. O mesmo dado.c roda no Cardputer e
// no PC: quem chama estas funcoes e o laco principal de cada um.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../hal/keyboard.h"

// DADO EM CASA
void dado_inicia(int n_partidas);        // n_partidas decide quem abre
void dado_passo(float dt);               // avanca a animacao dt segundos
void dado_desenha(void);                 // pinta o quadro inteiro no framebuffer
void dado_tecla(const key_event_t *ev);
void dado_mouse(int x, int y, int acao);   // acao: 0 moveu, 1 clique, 2 clique direito
int  dado_vencedor(void);                // -1 em jogo, 0/1 vencedor, 2 empate

// Modo online. Os dois computadores rodam a mesma partida com a mesma
// semente; so viajam as teclas de quem esta na vez de decidir.
// A plataforma avisa o jogo:
void dado_online_comeca(uint32_t semente, int eu);   // eu: 0 cria a sala, 1 entra
void dado_rede_tecla(int k);                         // tecla que o rival apertou
bool dado_digitando(void);                           // campo de texto aberto: letras sao dele
// e o jogo pede a plataforma (cada uma implementa as suas):
enum { REDE_PARADA, REDE_CONECTANDO, REDE_ESPERANDO, REDE_JOGANDO, REDE_ERRO };
bool rede_disponivel(void);              // false: esta versao nao tem online
void rede_cria(void);                    // abre uma sala nova
void rede_entra(const char *codigo);     // entra na sala do amigo
void rede_envia_tecla(int k);
void rede_sai(void);
int  rede_estado(void);
const char *rede_codigo(void);           // o codigo da sala aberta
const char *rede_erro(void);             // por que parou, quando REDE_ERRO

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
void dado_dbg_tutorial(int pag);         // abre o tutorial na pagina pag
int  dado_dbg_n_tipos(void);             // quantos tipos de dado existem
void dado_dbg_ia_esforco(int e);         // 0: IA pensa pouco (simulador rapido)
