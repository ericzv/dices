// O console: tela de inicio com os jogos (no jeito do PS5), ajustes, a barra
// e a central de controle por cima do jogo, o teclado da tela e a primeira
// configuracao (calibrar o toque, acertar o lado da imagem).
//
// A plataforma chama, a cada quadro: console_toque com o que o painel de toque
// leu (em coordenadas cruas, as do controlador), console_passo e
// console_desenha. O console desenha numa tela de 1280x800 deitada; a
// plataforma gira para o painel.
#pragma once
#include <stdbool.h>
#include "desenho.h"

void console_inicia(void);
void console_toque(bool tocando, int x, int y);   // x, y crus do controlador de toque
void console_botao(void);                         // botao fisico: faz o papel do "PS"
void console_passo(float dt);
bool console_desenha(tela_t *t);                  // true: a tela mudou e deve ir para o painel

// Para o simulador e os testes: pula a primeira configuracao com o toque ja
// na escala da tela (cru = tela).
void console_toque_identidade(void);
