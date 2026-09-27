// Fonte de pixels do PC: DotGothic16, 8x20, Latin-1 (gerada por tools/gen_fonte.py).
#pragma once
#include <stdint.h>

#define FONTE_LARG   8
#define FONTE_ALT    20
#define FONTE_PRIM   0x20
#define FONTE_N      (0x100 - FONTE_PRIM)

extern const uint8_t FONTE[FONTE_N][FONTE_ALT];
