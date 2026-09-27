// Cardputer ADV (K132-Adv) - pinos conforme o PinMap oficial da M5Stack.
// https://docs.m5stack.com/en/core/Cardputer-Adv
#pragma once

// ---- LCD ST7789V2 240x135 -------------------------------------------------
#define PIN_LCD_BL    38   // tambem alimenta o RGB LED (PWR_EN) no Stamp-S3A
#define PIN_LCD_RST   33
#define PIN_LCD_DC    34   // "RS" no pinmap
#define PIN_LCD_MOSI  35   // "DAT"
#define PIN_LCD_SCK   36
#define PIN_LCD_CS    37

#define LCD_W         240
#define LCD_H         135
#define LCD_HZ        (40 * 1000 * 1000)

// O painel e um 135x240 girado. Se a imagem sair deslocada ou espelhada,
// e aqui que se ajusta - nada mais no codigo depende disso.
#define LCD_SWAP_XY   true
#define LCD_MIRROR_X  true
#define LCD_MIRROR_Y  false
#define LCD_GAP_X     40
#define LCD_GAP_Y     53
#define LCD_INVERT    true

// Ordem dos componentes no painel. Se vermelho e azul aparecerem trocados,
// e esta linha que muda.
#define LCD_ORDEM_BGR false

// O ESP32 e little-endian e o ST7789 espera o byte alto primeiro, entao cada
// pixel de 16 bits sai trocado se nada for feito. O sintoma e caracteristico:
// cinzas viram cores saturadas - branco fica verde, cinza medio fica violeta.
// A troca e aplicada uma vez por chamada de desenho, nao por pixel.
#define LCD_SWAP_BYTES true

// ---- I2C interno (teclado TCA8418, codec ES8311, IMU BMI270) --------------
#define PIN_I2C_SDA   8
#define PIN_I2C_SCL   9
#define I2C_HZ        (400 * 1000)

#define PIN_KBD_INT   11
#define TCA8418_ADDR  0x34

// ---- outros ---------------------------------------------------------------
#define PIN_IR_TX     44
#define PIN_BAT_ADC   10   // divisor 2:1
#define PIN_SD_CS     12
#define PIN_SD_MOSI   14
#define PIN_SD_CLK    40
#define PIN_SD_MISO   39
