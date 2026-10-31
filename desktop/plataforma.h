// O que o laco do PC oferece alem do contrato hal/ do Cardputer.
#pragma once
#include <stdbool.h>

void som_inicia(void);       // depois de InitAudioDevice
void som_encerra(void);      // antes de CloseAudioDevice
void som_atualiza(float dt); // todo quadro: trilha em stream e volumes
