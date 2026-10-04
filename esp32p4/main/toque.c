// Sequencia de partida do GSL3680, a mesma das configuracoes que funcionam
// nas tres revisoes do JC8012P4A1: confere que o chip responde, limpa,
// reinicia, carrega o firmware palavra por palavra, liga e confere que ele
// esta rodando (0x5A5A5A5A no registrador 0xB0). Depois de ligado, nao se
// reinicia de novo: em algumas placas isso apaga o firmware recem-carregado.
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "placa.h"
#include "toque.h"
#include "toque_fw.h"

static const char *TAG = "toque";
static i2c_master_dev_handle_t dev;
static bool pronto;

static esp_err_t escreve(uint8_t reg, const uint8_t *d, int n)
{
    uint8_t b[8];
    b[0] = reg;
    memcpy(b + 1, d, (size_t)n);
    return i2c_master_transmit(dev, b, n + 1, 50);
}

static esp_err_t escreve1(uint8_t reg, uint8_t v) { return escreve(reg, &v, 1); }

static esp_err_t le(uint8_t reg, uint8_t *d, int n)
{
    return i2c_master_transmit_receive(dev, &reg, 1, d, (size_t)n, 50);
}

static void espera(int ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static void pulso_reset(int baixo_ms, int alto_ms)
{
    gpio_set_level(PIN_TOQUE_RST, 0);
    espera(baixo_ms);
    gpio_set_level(PIN_TOQUE_RST, 1);
    espera(alto_ms);
}

bool toque_inicia(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOQUE_ADDR,
        .scl_speed_hz = I2C_HZ,
    };
    if (i2c_master_bus_add_device(bus, &dc, &dev) != ESP_OK) return false;
    gpio_config_t r = { .pin_bit_mask = (1ULL << PIN_TOQUE_RST) | (1ULL << PIN_TOQUE_INT), .mode = GPIO_MODE_OUTPUT };
    gpio_config(&r);
    // Como no driver do fabricante: INT em baixo enquanto o chip sai do reset.
    gpio_set_level(PIN_TOQUE_INT, 0);
    pulso_reset(10, 10);
    gpio_config_t i = { .pin_bit_mask = 1ULL << PIN_TOQUE_INT, .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&i);
    espera(50);

    // O chip responde? Escreve um padrao e le de volta.
    uint8_t b[4], teste[4] = { 0x12, 0x34, 0x56, 0x00 };
    if (le(0xF0, b, 4) != ESP_OK) {
        ESP_LOGE(TAG, "o GSL3680 nao respondeu no I2C (0x%02X)", TOQUE_ADDR);
        return false;
    }
    espera(20);
    escreve(0xF0, teste, 4);
    espera(20);
    le(0xF0, b, 4);
    if (b[0] != 0x12) ESP_LOGW(TAG, "leitura de teste estranha: %02X", b[0]);

    // Limpa os registradores e reinicia.
    static const uint8_t LIMPA[4][2] = { { 0xE0, 0x88 }, { 0x88, 0x01 }, { 0xE4, 0x04 }, { 0xE0, 0x00 } };
    for (int k = 0; k < 4; k++) { escreve1(LIMPA[k][0], LIMPA[k][1]); espera(20); }
    pulso_reset(20, 20);
    escreve1(0xE0, 0x88);
    espera(10);
    escreve1(0xE4, 0x04);
    espera(10);
    static const uint8_t ZEROS[4] = { 0 };
    escreve(0xBC, ZEROS, 4);
    espera(10);

    // Firmware: pagina por pagina, de 4 em 4 bytes.
    int erros = 0;
    for (int blk = 0; blk < TOQUE_FW_BLOCOS; blk++) {
        if (escreve1(0xF0, TOQUE_FW[blk][0]) != ESP_OK) erros++;
        for (int k = 0; k < 32; k++)
            if (escreve((uint8_t)(k * 4), &TOQUE_FW[blk][4 + k * 4], 4) != ESP_OK) erros++;
    }
    if (erros) ESP_LOGW(TAG, "%d escritas do firmware falharam", erros);

    escreve1(0xE0, 0x00);                  // roda
    espera(40);
    if (le(0xB0, b, 4) == ESP_OK && b[0] == 0x5A && b[1] == 0x5A && b[2] == 0x5A && b[3] == 0x5A)
        ESP_LOGI(TAG, "GSL3680 rodando");
    else
        ESP_LOGW(TAG, "GSL3680 nao confirmou (0xB0 = %02X %02X %02X %02X); seguindo assim mesmo",
                 b[0], b[1], b[2], b[3]);
    pronto = true;
    return true;
}

bool toque_ok(void) { return pronto; }

bool toque_le(int *x, int *y)
{
    if (!pronto) return false;
    uint8_t b[8];
    if (le(0x80, b, 8) != ESP_OK) return false;
    int dedos = b[0] & 0x0F;
    if (!dedos || dedos > 5) return false;
    // Primeiro ponto: y nos bytes 4-5, x nos 6-7 (o alto do 7 e o numero do dedo).
    *y = ((b[5] << 8) | b[4]) & 0x0FFF;
    *x = ((b[7] & 0x0F) << 8) | b[6];
    return true;
}
