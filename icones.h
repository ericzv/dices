#pragma once
#include <stdint.h>

// Icones 28x28 com 4 niveis de alfa (2 bits por pixel), gerados por
// tools/gen_icons.py. 7 bytes por linha, 28 linhas.
#define ICONE_W     28
#define ICONE_H     28
#define ICONE_STRIDE 7
#define ICONE_BYTES (ICONE_STRIDE * ICONE_H)

extern const uint8_t icone_olho[ICONE_BYTES];
extern const uint8_t icone_cruz[ICONE_BYTES];
extern const uint8_t icone_controle[ICONE_BYTES];
extern const uint8_t icone_picareta[ICONE_BYTES];
extern const uint8_t icone_encruzilhada[ICONE_BYTES];
extern const uint8_t icone_letra_a[ICONE_BYTES];
extern const uint8_t icone_dado[ICONE_BYTES];
extern const uint8_t icone_engrenagem[ICONE_BYTES];
