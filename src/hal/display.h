// Tela, versao PC: so o framebuffer RGB565 de LCD_W x LCD_H, que a janela
// amplia. O firmware tem aqui tambem o init e o flush do ST7789.
#pragma once
#include <stdint.h>

uint16_t *display_fb(void);
