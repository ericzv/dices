// Relogio RX8025T (Epson), com a bateria CR1220 do suporte da placa. Sem a
// bateria, ou na primeira vez, ele acorda com a marca de "perdeu a hora"
// (VLF): ai o console nao mostra relogio ate alguem acertar nos Ajustes.
//
// Ao ligar, a data e a hora dele vao para o relogio do sistema (time(),
// localtime()), que e o que o jogo le: a epoca do Natal, o dia da run
// campea. Sem fuso: o RX8025T guarda a hora local e o sistema fica em UTC,
// entao localtime() devolve exatamente o que foi acertado.
//
// O byte livre do RX8025T (0x07) marca que a data tambem foi acertada: as
// versoes antigas deste firmware so acertavam a hora e deixavam la um 1 de
// janeiro qualquer, que o jogo tomaria pelo Reveillon. Sem a marca, o sistema
// fica com a hora certa num dia de 1970, que o jogo le como "sem data".
#define MARCA_DATA 0xDA
#include <sys/time.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "placa.h"
#include "relogio.h"

static i2c_master_dev_handle_t dev;
static bool ok, valido, posto, data_posta;   // posto: o relogio do sistema tem a hora certa

static int bcd(uint8_t v) { return (v >> 4) * 10 + (v & 15); }
static uint8_t para_bcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static void poe_no_sistema(struct tm *t)
{
    t->tm_isdst = 0;
    struct timeval tv = { .tv_sec = mktime(t) };
    settimeofday(&tv, NULL);
    posto = true;
}

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
    // segundos, minutos, horas, dia da semana, dia, mes, ano (BCD, 20xx), o byte livre
    uint8_t b[8] = { 0 };
    reg = 0x00;
    if (valido && i2c_master_transmit_receive(dev, &reg, 1, b, 8, 50) == ESP_OK) {
        struct tm t = {
            .tm_sec = bcd(b[0] & 0x7F), .tm_min = bcd(b[1] & 0x7F), .tm_hour = bcd(b[2] & 0x3F),
            .tm_mday = bcd(b[4] & 0x3F), .tm_mon = bcd(b[5] & 0x1F) - 1, .tm_year = bcd(b[6]) + 100,
        };
        if (t.tm_sec < 60 && t.tm_min < 60 && t.tm_hour < 24 && t.tm_mday >= 1 && t.tm_mday <= 31 &&
            t.tm_mon >= 0 && t.tm_mon < 12) {
            data_posta = b[7] == MARCA_DATA;
            if (!data_posta) {
                t.tm_mday = 1;
                t.tm_mon = 0;
                t.tm_year = 70;
            }
            poe_no_sistema(&t);
        } else {
            valido = false;
        }
    }
    if (data_posta)
        ESP_LOGI("relogio", "RX8025T: %02d/%02d/20%02d %02d:%02d", bcd(b[4] & 0x3F), bcd(b[5] & 0x1F), bcd(b[6]),
                 bcd(b[2] & 0x3F), bcd(b[1] & 0x7F));
    else
        ESP_LOGI("relogio", "RX8025T %s", posto ? "com a hora, sem a data (falta acertar)" : "sem hora (falta acertar)");
    return true;
}

bool relogio_le(struct tm *t, bool *com_data)
{
    if (!posto) return false;
    time_t agora = time(NULL);
    localtime_r(&agora, t);
    *com_data = data_posta;
    return true;
}

void relogio_acerta(struct tm *t)
{
    t->tm_sec = 0;
    poe_no_sistema(t);                       // normaliza e acha o dia da semana
    data_posta = true;
    if (!ok) return;                         // sem o RX8025T: so ate desligar
    uint8_t b[9] = {
        0x00, 0x00, para_bcd(t->tm_min), para_bcd(t->tm_hour), (uint8_t)(1 << t->tm_wday),
        para_bcd(t->tm_mday), para_bcd(t->tm_mon + 1), para_bcd(t->tm_year % 100), MARCA_DATA,
    };
    i2c_master_transmit(dev, b, sizeof b, 50);
    uint8_t f[2] = { 0x0E, 0x00 };           // limpa VLF
    i2c_master_transmit(dev, f, 2, 50);
    valido = true;
}
