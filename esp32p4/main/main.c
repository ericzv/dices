// Console de jogos para o Guition JC8012P4A1 (ESP32-P4, tela de 10,1"). Liga
// a tela, o toque, o som e o relogio, e roda o console (esp32p4/console), que
// abre os jogos. O primeiro jogo e o Dado em Casa, o mesmo src/games/dado.c
// do PC.
//
// Botao BOOT: nos primeiros 8 segundos depois de ligar, troca a versao do
// painel (V1 <-> V2) e reinicia; para quando a tela fica preta ou listrada.
// Depois disso, faz o papel do botao "PS": abre a central de controle.
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_app_desc.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "sdkconfig.h"
#include "placa.h"
#include "tela.h"
#include "toque.h"
#include "audio.h"
#include "som_p4.h"
#include "relogio.h"
#include "console.h"
#include "sistema.h"
#include "desenho.h"
#include "ui/gfx.h"
#include "hal/display.h"

static const char *TAG = "console";

static uint16_t *fb_jogo;
static int painel_versao, brilho = 80;
static bool luz_acesa;

uint16_t *display_fb(void) { return fb_jogo; }

// ---------------------------------------------------------------------------
// O que o console pede ao aparelho (sistema.h)
// ---------------------------------------------------------------------------
void *sis_aloca(size_t n) { return heap_caps_calloc(1, n, MALLOC_CAP_SPIRAM); }
void sis_brilho(int pct) { brilho = pct; if (luz_acesa) tela_brilho(pct); }
void sis_volume(int pct) { audio_volume(pct); }
void sis_gira(bool g) { tela_gira(g); }
int  sis_toque_estado(void) { return toque_ok() ? 1 : 0; }
bool sis_hora(int *h, int *m) { return relogio_le(h, m); }
void sis_acerta_hora(int h, int m) { relogio_acerta(h, m); }
int  sis_painel(void) { return painel_versao; }
void sis_reinicia(void) { esp_restart(); }

static void grava_painel(int v)
{
    nvs_handle_t h;
    if (nvs_open("sistema", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h, "painel", (uint8_t)v);
    nvs_commit(h);
    nvs_close(h);
}

void sis_troca_painel(int v)
{
    grava_painel(v);
    esp_restart();
}

// Sem nada guardado: as placas com ESP32-P4 v3 vem todas com a tela nova (V2);
// as de antes, na maioria, com a primeira (V1).
static int painel_escolhido(void)
{
    nvs_handle_t h;
    uint8_t v = 0;
    if (nvs_open("sistema", NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, "painel", &v);
        nvs_close(h);
    }
    if (v == 1 || v == 2) return v;
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    return ci.revision >= 300 ? 2 : 1;
}

int sis_info(char *buf, int n)
{
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    uint8_t id[3];
    tela_id(id);
    return snprintf(buf, (size_t)n,
                    "Placa: Guition JC8012P4A1\n"
                    "Chip: ESP32-P4 v%d.%d, %d MHz\n"
                    "Memória livre: %.1f MB\n"
                    "Tela: JD9365, painel V%d (id %02X %02X %02X)\n"
                    "Toque: GSL3680 %s\n"
                    "Firmware: %s\n",
                    ci.revision / 100, ci.revision % 100, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
                    heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1048576.0, painel_versao, id[0], id[1], id[2],
                    toque_ok() ? "respondendo" : "sem resposta", esp_app_get_description()->version);
}

// ---------------------------------------------------------------------------
// Botao BOOT nos primeiros segundos: troca o painel. Fica de olho desde o
// comeco, antes mesmo da tela (que e justamente o que pode estar errado).
// ---------------------------------------------------------------------------
static void vigia_boot(void *arg)
{
    bool antes = gpio_get_level(PIN_BOOT) == 0;
    while (esp_timer_get_time() < 8000000) {
        bool agora = gpio_get_level(PIN_BOOT) == 0;
        if (agora && !antes) {
            int outro = painel_versao == 1 ? 2 : 1;
            ESP_LOGW(TAG, "BOOT apertado: trocando para o painel V%d", outro);
            grava_painel(outro);
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();
        }
        antes = agora;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    painel_versao = painel_escolhido();
    ESP_LOGI(TAG, "ESP32-P4 v%d.%d, %u KB de PSRAM livres, painel V%d", ci.revision / 100, ci.revision % 100,
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024), painel_versao);

    gpio_config_t b = { .pin_bit_mask = 1ULL << PIN_BOOT, .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&b);
    xTaskCreate(vigia_boot, "boot", 4096, NULL, 5, NULL);

    fb_jogo = heap_caps_calloc(GFX_W * GFX_H, 2, MALLOC_CAP_SPIRAM);
    if (tela_inicia(painel_versao) != ESP_OK)
        ESP_LOGE(TAG, "a tela nao subiu; com o painel errado ela fica preta: aperte BOOT ate 8 s depois de ligar");

    i2c_master_bus_handle_t i2c;
    i2c_master_bus_config_t bc = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bc, &i2c));
    if (!toque_inicia(i2c)) ESP_LOGE(TAG, "toque sem resposta");
    som_carrega();
    if (!audio_inicia(i2c, som_mistura)) ESP_LOGE(TAG, "som sem resposta");
    relogio_inicia(i2c);

    tela_t tela = { tela_quadro(), TELA_W, TELA_H };
    console_inicia();
    ESP_LOGI(TAG, "pronto, %u KB de PSRAM livres", (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));

    int64_t antes = esp_timer_get_time();
    bool boot_antes = true;
    for (;;) {
        int64_t agora = esp_timer_get_time();
        float dt = (agora - antes) / 1e6f;
        antes = agora;
        if (dt > 0.05f) dt = 0.05f;

        int x = 0, y = 0;
        bool dedo = toque_le(&x, &y);
        console_toque(dedo, x, y);

        bool boot = gpio_get_level(PIN_BOOT) == 0;
        if (boot && !boot_antes && agora > 8000000) console_botao();
        boot_antes = boot;

        console_passo(dt);
        som_atualiza(dt);
        if (tela.px && console_desenha(&tela)) {
            tela_mostra();
            if (!luz_acesa) {                // so acende depois da primeira imagem
                luz_acesa = true;
                tela_brilho(brilho);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(12));
        }
    }
}
