#pragma once
#include <stdbool.h>
#include "driver/i2c_master.h"

bool relogio_inicia(i2c_master_bus_handle_t bus);
bool relogio_le(int *h, int *m);
void relogio_acerta(int h, int m);
