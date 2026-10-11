// A bateria (conector CN1 da placa, carregador IP5306): a tensao chega ao
// GPIO52 por um divisor de 68k/100k. So da uma ideia da carga.
#pragma once
#include <stdbool.h>

void bateria_inicia(void);
void bateria_passo(void);                 // no laco principal: le de tempos em tempos
// false: sem bateria (ou ainda sem leitura). mv: a tensao da bateria.
bool bateria_le(int *pct, int *mv, bool *carregando);
