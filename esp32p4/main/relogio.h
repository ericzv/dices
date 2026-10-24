#pragma once
#include <stdbool.h>
#include <time.h>
#include "driver/i2c_master.h"

bool relogio_inicia(i2c_master_bus_handle_t bus);   // e poe a hora dele no relogio do sistema
bool relogio_le(struct tm *t, bool *com_data);      // false: nunca foi acertado
void relogio_acerta(struct tm *t);                  // o RX8025T e o relogio do sistema
