// Saida de som: I2S para o codec ES8311 (mono) e o amplificador NS4150, que
// liga o alto-falante. Uma tarefa no segundo nucleo pede amostras ao
// misturador (som_p4.c) e as entrega ao I2S, 44,1 kHz.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"

typedef void (*audio_mistura_fn)(int16_t *saida, int n);   // n amostras mono

bool audio_inicia(i2c_master_bus_handle_t bus, audio_mistura_fn mistura);
void audio_volume(int pct);              // volume do codec, 0..100 (0: mudo)
