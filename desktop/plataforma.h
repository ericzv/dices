// O que o laco do PC oferece alem do contrato hal/ do Cardputer.
#pragma once
#include <stdbool.h>

void som_inicia(void);       // depois de InitAudioDevice
void som_encerra(void);      // antes de CloseAudioDevice
void som_atualiza(float dt); // todo quadro: trilha em stream e volumes
// A carga sob demanda (som.c): os sons que a cena pode pedir agora, que o
// jogo conta (dado_sons_precisos) - SOM_N marcas dos efeitos, SOM_ROLAS dos
// lacos do acabamento, e a moeda rolando, o giro da roleta e o motor do
// elevador.
void som_precisa(const unsigned char *som, const unsigned char *rola, bool moeda, bool roleta, bool elevador);
void som_folga(bool on);     // a tela esta coberta (a capa do site): a carga pode usar mais tempo por quadro
// Para o medidor de quadros: KB de som decodificado, quantos sons foram
// decodificados na hora de tocar e quantos ainda esperam na fila.
void som_medida(int *kb, int *na_hora, int *na_fila);
