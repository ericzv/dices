// O ES8311 escravo do I2S, com o MCLK vindo do ESP32-P4 (256 x 44,1 kHz), so
// a parte do DAC. Registradores conforme o driver es8311 do esp_codec_dev
// (Espressif, Apache 2.0).
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "placa.h"
#include "audio.h"

static const char *TAG = "audio";
#define TAXA       44100
#define BLOCO      256

static i2c_master_dev_handle_t codec;
static i2s_chan_handle_t tx;
static audio_mistura_fn misturador;
static bool codec_ok;

static esp_err_t escreve(uint8_t reg, uint8_t v)
{
    uint8_t b[2] = { reg, v };
    return i2c_master_transmit(codec, b, 2, 50);
}

static uint8_t le(uint8_t reg)
{
    uint8_t v = 0;
    i2c_master_transmit_receive(codec, &reg, 1, &v, 1, 50);
    return v;
}

static bool es8311_inicia(void)
{
    esp_err_t e = ESP_OK;
    e |= escreve(0x44, 0x08);              // imunidade a ruido no I2C (duas vezes: a primeira as vezes falha)
    e |= escreve(0x44, 0x08);
    e |= escreve(0x01, 0x30);
    e |= escreve(0x02, 0x00);
    e |= escreve(0x03, 0x10);
    e |= escreve(0x16, 0x24);
    e |= escreve(0x04, 0x10);
    e |= escreve(0x05, 0x00);
    e |= escreve(0x0B, 0x00);
    e |= escreve(0x0C, 0x00);
    e |= escreve(0x10, 0x1F);
    e |= escreve(0x11, 0x7F);
    e |= escreve(0x00, 0x80);              // escravo
    e |= escreve(0x01, 0x3F);              // relogio do pino MCLK, sem inverter
    e |= escreve(0x06, le(0x06) & ~0x20);
    e |= escreve(0x13, 0x10);
    e |= escreve(0x1B, 0x0A);
    e |= escreve(0x1C, 0x6A);
    e |= escreve(0x44, 0x58);
    // Formato: I2S, 16 bits.
    e |= escreve(0x09, (le(0x09) | 0x0C) & 0xFC);
    e |= escreve(0x0A, (le(0x0A) | 0x0C) & 0xFC);
    // Relogios para MCLK = 11,2896 MHz e 44,1 kHz (tabela do esp_codec_dev).
    e |= escreve(0x02, le(0x02) & 0x07);   // pre_div 1, pre_mult 1
    e |= escreve(0x05, 0x00);              // adc_div 1, dac_div 1
    e |= escreve(0x03, (le(0x03) & 0x80) | 0x10);
    e |= escreve(0x04, (le(0x04) & 0x80) | 0x10);
    e |= escreve(0x07, le(0x07) & 0xC0);
    e |= escreve(0x08, 0xFF);
    e |= escreve(0x06, (le(0x06) & 0xE0) | 0x03);   // bclk_div 4
    // Liga o DAC.
    e |= escreve(0x00, 0x80);
    e |= escreve(0x01, 0x3F);
    e |= escreve(0x09, le(0x09) & 0xBF);
    e |= escreve(0x0A, le(0x0A) & 0xBF);
    e |= escreve(0x17, 0xBF);
    e |= escreve(0x0E, 0x02);
    e |= escreve(0x12, 0x00);
    e |= escreve(0x14, 0x1A);
    e |= escreve(0x0D, 0x01);
    e |= escreve(0x15, 0x40);
    e |= escreve(0x37, 0x08);
    e |= escreve(0x45, 0x00);
    e |= escreve(0x31, le(0x31) & 0x9F);   // sem mudo
    return e == ESP_OK;
}

void audio_volume(int pct)
{
    if (!codec_ok) return;
    // 0 dB no maximo, -45 dB no 1%; 0 cala. Passos de 0,5 dB a partir de 0xBF (0 dB).
    int reg = 0;
    if (pct > 0) {
        if (pct > 100) pct = 100;
        float db = -45.0f * (1.0f - pct / 100.0f);
        reg = 0xBF + (int)(db * 2);
    }
    escreve(0x32, (uint8_t)reg);
}

static void tarefa(void *arg)
{
    static int16_t mono[BLOCO], estereo[BLOCO * 2];
    for (;;) {
        misturador(mono, BLOCO);
        for (int i = 0; i < BLOCO; i++) estereo[2 * i] = estereo[2 * i + 1] = mono[i];
        size_t escritos;
        i2s_channel_write(tx, estereo, sizeof estereo, &escritos, portMAX_DELAY);
    }
}

bool audio_inicia(i2c_master_bus_handle_t bus, audio_mistura_fn mistura)
{
    misturador = mistura;
    i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    cc.dma_desc_num = 6;
    cc.dma_frame_num = BLOCO;
    cc.auto_clear = true;
    if (i2s_new_channel(&cc, &tx, NULL) != ESP_OK) return false;
    i2s_std_config_t sc = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(TAXA),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = PIN_I2S_MCLK,
            .bclk = PIN_I2S_BCLK,
            .ws = PIN_I2S_WS,
            .dout = PIN_I2S_DOUT,
            .din = I2S_GPIO_UNUSED,
        },
    };
    if (i2s_channel_init_std_mode(tx, &sc) != ESP_OK || i2s_channel_enable(tx) != ESP_OK) return false;

    // Com o MCLK ja correndo, o codec.
    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ES8311_ADDR,
        .scl_speed_hz = I2C_HZ,
    };
    if (i2c_master_bus_add_device(bus, &dc, &codec) == ESP_OK) {
        codec_ok = es8311_inicia();
        if (!codec_ok) ESP_LOGE(TAG, "o ES8311 nao respondeu");
    }
    gpio_config_t g = { .pin_bit_mask = 1ULL << PIN_AMP, .mode = GPIO_MODE_OUTPUT };
    gpio_config(&g);
    gpio_set_level(PIN_AMP, codec_ok);

    xTaskCreatePinnedToCore(tarefa, "som", 4096, NULL, 18, NULL, 1);
    return codec_ok;
}
