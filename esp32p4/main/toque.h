// Toque capacitivo GSL3680 (Silead), no I2C da placa. O chip nao guarda
// programa: a cada partida o firmware dele (toque_fw.h) e carregado na RAM.
// As coordenadas saem cruas; quem as leva para a tela e a calibracao do
// console, que descobre a orientacao e a escala na primeira vez.
#pragma once
#include <stdbool.h>
#include "driver/i2c_master.h"

bool toque_inicia(i2c_master_bus_handle_t bus);
bool toque_ok(void);
bool toque_le(int *x, int *y);           // true com um dedo na tela (o primeiro, se houver mais)
