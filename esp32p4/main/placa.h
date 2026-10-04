// Guition JC8012P4A1: ESP32-P4 + ESP32-C6, tela de 10,1" 800x1280 (JD9365,
// MIPI-DSI de 2 vias), toque GSL3680, codec ES8311 com amplificador NS4150,
// relogio RX8025T. Pinos conforme o esquema e o BSP do fabricante.
#pragma once

// ---- Tela ------------------------------------------------------------------
#define PIN_LCD_RST        27
#define PIN_LCD_LUZ        23      // PWM da luz de fundo (MP3202), ativo em alto
#define LCD_W              800     // o painel e em pe: 800 de largura...
#define LCD_H              1280    // ...por 1280 de altura
#define LDO_DSI_CANAL      3       // VDD_MIPI_DPHY: 2,5 V do LDO interno
#define LDO_DSI_MV         2500

// O console desenha deitado (1280x800) e a PPA gira um quarto de volta para o
// painel. Se a imagem aparecer de cabeca para baixo, os Ajustes giram 180
// graus; aqui so se escolhe qual dos dois lados e o "normal".
#define GIRO_NORMAL        PPA_SRM_ROTATION_ANGLE_270
#define GIRO_INVERTIDO     PPA_SRM_ROTATION_ANGLE_90

// ---- I2C (toque, codec, relogio) --------------------------------------------
#define PIN_I2C_SDA        7
#define PIN_I2C_SCL        8
#define I2C_HZ             400000

#define PIN_TOQUE_RST      22
#define PIN_TOQUE_INT      21
#define TOQUE_ADDR         0x40     // GSL3680

#define ES8311_ADDR        0x18
#define RTC_ADDR           0x32     // RX8025T

// ---- Som (I2S para o ES8311) ------------------------------------------------
#define PIN_I2S_MCLK       13
#define PIN_I2S_BCLK       12
#define PIN_I2S_WS         10
#define PIN_I2S_DOUT       9        // para o DSDIN do codec
#define PIN_AMP            20       // PA_CTRL do NS4150: alto liga

// ---- Outros -----------------------------------------------------------------
#define PIN_BOOT           35       // botao BOOT: faz o papel do botao "PS"
