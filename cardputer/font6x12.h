#pragma once
#include <stdint.h>
#define FONT_W       6
#define FONT_H       12
#define FONT_FIRST   0x20
#define FONT_LAST    0xFF
#define FONT_GLYPHS  (FONT_LAST - FONT_FIRST + 1)

extern const uint8_t font6x12_regular[FONT_GLYPHS][FONT_H];
extern const uint8_t font6x12_bold[FONT_GLYPHS][FONT_H];
