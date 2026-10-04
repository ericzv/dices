// Relogio RX8025T (Epson), com a bateria CR1220 do suporte da placa. Sem a
// bateria, ou na primeira vez, ele acorda com a marca de "perdeu a hora"
// (VLF): ai o console nao mostra relogio ate alguem acertar nos Ajustes.
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "placa.h"
#include "relogio.h"

static i2c_master_dev_handle_t dev;
static bool ok, valido;

static int bcd(uint8_t v) { return (v >> 4) * 10 + (v & 15); }
static uint8_t para_bcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

bool relogio_inicia(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = RTC_ADDR,
        .scl_speed_hz = I2C_HZ,
    };
    if (i2c_master_bus_add_device(bus, &dc, &dev) != ESP_OK) return false;
    uint8_t reg = 0x0E, flag = 0;
    if (i2c_master_transmit_receive(dev, &reg, 1, &flag, 1, 50) != ESP_OK) {
        ESP_LOGW("relogio", "RX8025T nao respondeu");
        return false;
    }
    ok = true;
    valido = !(flag & 0x02);                // VLF: a hora se perdeu
    ESP_LOGI("relogio", "RX8025T %s", valido ? "com a hora certa" : "sem hora (falta acertar)");
    return true;
}

bool relogio_le(int *h, int *m)
{
    if (!ok || !valido) return false;
    uint8_t reg = 0x00, b[3];
    if (i2c_master_transmit_receive(dev, &reg, 1, b, 3, 50) != ESP_OK) return false;
    *m = bcd(b[1] & 0x7F);
    *h = bcd(b[2] & 0x3F);
    return *h < 24 && *m < 60;
}

void relogio_acerta(int h, int m)
{
    if (!ok) return;
    // segundos, minutos, horas; se a data nunca foi posta, um dia qualquer valido.
    uint8_t b[8] = { 0x00, 0x00, para_bcd(m), para_bcd(h), 0x01, 0x01, 0x01, 0x26 };
    i2c_master_transmit(dev, b, valido ? 4 : 8, 50);
    uint8_t f[2] = { 0x0E, 0x00 };           // limpa VLF
    i2c_master_transmit(dev, f, 2, 50);
    valido = true;
}
