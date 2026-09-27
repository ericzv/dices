// Substituto do esp_timer.h do ESP-IDF: o dado.c so o usa para a semente.
#pragma once
#include <stdint.h>

int64_t esp_timer_get_time(void);        // microssegundos
