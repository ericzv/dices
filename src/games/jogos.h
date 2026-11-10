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
void dado_mouse(int x, int y, int acao);   // acao: 0 moveu, 1 clique, 2 clique direito, 3 toque
bool dado_mouse_clicavel(void);           // mouse sobre um botao ou opcao
// Controles: com que se joga agora (as dicas de tecla da tela mostram os
// botoes dele) - 0 teclado e mouse, 1 controle do Xbox (e parecidos), 2 do
// PlayStation. E, no modo 2 jogadores, quem escolhe agora (0 ou 1; -1:
// qualquer um), para cada controle so jogar na sua vez.
enum { CONTROLE_NENHUM, CONTROLE_XBOX, CONTROLE_PS };
void dado_controle(int estilo);
int  dado_vez_do_controle(void);
// O salao em volta da mesa, para a margem da janela: px tem w x h pixels do
// jogo, com o quadro de GFX_W x GFX_H em (ox, oy).
void dado_salao(uint16_t *px, int w, int h, int ox, int oy);
int  dado_vencedor(void);                // -1 em jogo, 0/1 vencedor, 2 empate

// Modo online. Os dois computadores rodam a mesma partida com a mesma
// semente; so viajam as teclas de quem esta na vez de decidir.
// A plataforma avisa o jogo:
void dado_online_comeca(uint32_t semente, int eu);   // eu: 0 cria a sala, 1 entra
int  dado_rede_tecla(int k);                         // tecla que o rival apertou
void dado_online_abre(void);                         // abre a tela do online (quem chega por um convite)
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
// Um marco da partida: o estado das regras num instante que os dois lados
// atravessam igual (indice 1, 2, 3...). A rede compara com o do rival.
void rede_marco(int indice, uint32_t hash);
const char *rede_convite(void);          // o link de convite da sala aberta ("": sem link)
void rede_copia_convite(void);           // copia o link (no celular, abre o compartilhar)
int  rede_convite_copiado(void);         // 1 copiou, -1 nao deu, 0 ainda nada

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
