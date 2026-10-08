// GERADO POR tools/gen_fonte.py - NAO EDITAR A MAO
// As fontes de pixels do jogo, Latin-1, no grid de 640x360:
//   FONTE  Jersey 10 (texto normal): linha de 20 px, base na linha 15.
//   MIUDA  Tiny5 (texto miudo): linha de 12 px, base na linha 9.
// Cada linha de um glifo e um mapa de bits (bit 0 = coluna 0).
#pragma once
#include <stdint.h>

#define FONTE_ALT    20
#define FONTE_BASE   15
#define MIUDA_ALT    12
#define MIUDA_BASE   9
#define FONTE_PRIM   0x20
#define FONTE_N      (0x100 - FONTE_PRIM)

extern const uint8_t  FONTE_AV[FONTE_N];
extern const uint16_t FONTE[FONTE_N][FONTE_ALT];
extern const uint8_t  MIUDA_AV[FONTE_N];
extern const uint16_t MIUDA[FONTE_N][MIUDA_ALT];
