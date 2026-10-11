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
#include "esp_sleep.h"
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
#include "bateria.h"
#include "console.h"
#include "sistema.h"
#include "desenho.h"
#include "ui/gfx.h"
#include "hal/display.h"

static const char *TAG = "console";

static int painel_versao, brilho = 80;
static bool luz_acesa;

// O quadro do jogo (640x360) em dobro: o jogo desenha num enquanto o segundo
// nucleo amplia e gira o outro para a tela (tela_mostra_jogo).
static uint16_t *fb_jogo[2];
static int fb_vez;                       // o que o jogo desenha agora
static SemaphoreHandle_t fb_livre[2];    // dado quando o segundo nucleo largou aquele quadro
static struct { const uint16_t *px, *lut; int w, h, k, x, y; } pedido;

uint16_t *display_fb(void) { return fb_jogo[fb_vez]; }

bool sis_mostra_jogo(const uint16_t *px, int w, int h, int k, int x, int y, const uint16_t *lut)
{
    pedido.px = px; pedido.lut = lut;
    pedido.w = w; pedido.h = h; pedido.k = k; pedido.x = x; pedido.y = y;
    return true;
}

// Manda o quadro pronto: o jogo direto (se o console pediu) ou a tela deitada
// do console inteira.
static void mostra(void)
{
    const uint16_t *px = pedido.px;
    pedido.px = NULL;
    if (px) {
        int g = px == fb_jogo[0] ? 0 : px == fb_jogo[1] ? 1 : -1;
        if (g >= 0 && tela_mostra_jogo(px, pedido.w, pedido.h, pedido.k, pedido.x, pedido.y, pedido.lut,
                                       fb_livre[g])) {
            fb_vez = g ^ 1;                                    // o proximo vai no outro quadro,
            xSemaphoreTake(fb_livre[fb_vez], portMAX_DELAY);   // assim que o segundo nucleo largar
            return;
        }
        // Sem o caminho rapido: a mesa ampliada pela CPU na tela deitada.
        tela_t t = { tela_quadro(), TELA_W, TELA_H };
        d_alvo(&t);
        d_recorte_tudo();
        d_amplia_lut(px, pedido.w, pedido.h, pedido.x, pedido.y, pedido.k, pedido.lut);
    }
    tela_mostra();
}

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
void sis_reinicia(void) { esp_restart(); }
bool sis_bateria(int *pct, int *mv, bool *carregando) { return bateria_le(pct, mv, carregando); }

// Nao ha chave de liga e desliga: apaga a luz devagar, desliga o
// alto-falante e a tela e poe o ESP32-P4 no sono profundo, sem nada que o
// acorde (o BOOT nao esta entre os pinos que acordam o chip). Liga de novo
// pelo botao RST, ou tirando e pondo a energia.
void sis_desliga(void)
{
    ESP_LOGW(TAG, "desligando; para ligar de novo, aperte o RST da placa");
    for (int b = brilho; b > 0; b -= 6) {
        tela_brilho(b);
        vTaskDelay(pdMS_TO_TICKS(12));
    }
    audio_desliga();
    tela_desliga();
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_deep_sleep_start();
}

// A versao do painel fica em "tela" (e a anterior em "tela_ant"). Trocar e
// um teste ("teste" = 1): o console pergunta se a imagem ficou boa e, sem
// resposta em 15 s, volta a anterior. Assim uma troca errada nunca deixa a
// tela presa. A chave antiga ("painel") vinha de um botao dos ajustes que
// saiu: e apagada.
static bool em_teste;

static void grava_tela(uint8_t v, uint8_t ant, bool teste)
{
    nvs_handle_t h;
    if (nvs_open("sistema", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h, "tela", v);
    nvs_set_u8(h, "tela_ant", ant);
    if (teste) nvs_set_u8(h, "teste", 1);
    else nvs_erase_key(h, "teste");
    nvs_commit(h);
    nvs_close(h);
}

int  sis_painel(void) { return painel_versao; }
bool sis_tela_em_teste(void) { return em_teste; }

void sis_tela_troca(void)
{
    int outro = painel_versao == 1 ? 2 : 1;
    ESP_LOGW(TAG, "trocando para o painel V%d (em teste)", outro);
    grava_tela((uint8_t)outro, (uint8_t)painel_versao, true);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
}

void sis_tela_mantem(void)
{
    em_teste = false;
    grava_tela((uint8_t)painel_versao, (uint8_t)painel_versao, false);
    ESP_LOGI(TAG, "painel V%d mantido", painel_versao);
}

void sis_tela_desfaz(void)
{
    nvs_handle_t h;
    uint8_t ant = 0;
    if (nvs_open("sistema", NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, "tela_ant", &ant);
        nvs_close(h);
    }
    if (ant != 1 && ant != 2) ant = painel_versao == 1 ? 2 : 1;
    ESP_LOGW(TAG, "sem confirmacao: voltando ao painel V%d", ant);
    grava_tela(ant, ant, false);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
}

// Sem nada guardado: as placas com ESP32-P4 v3 vem todas com a tela nova (V2);
// as de antes, na maioria, com a primeira (V1).
static int painel_escolhido(void)
{
    nvs_handle_t h;
    uint8_t v = 0, t = 0;
    if (nvs_open("sistema", NVS_READWRITE, &h) == ESP_OK) {
        if (nvs_erase_key(h, "painel") == ESP_OK) nvs_commit(h);
        nvs_get_u8(h, "tela", &v);
        nvs_get_u8(h, "teste", &t);
        nvs_close(h);
    }
    em_teste = t == 1;
    if (v == 1 || v == 2) return v;
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    return ci.revision >= 300 ? 2 : 1;
}

// Quatro linhas: e o que cabe no cartao "Sobre" dos ajustes.
int sis_info(char *buf, int n)
{
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    uint8_t id[3];
    tela_id(id);
    char bat[64];
    int pct, mv;
    bool carga;
    if (bateria_le(&pct, &mv, &carga))
        snprintf(bat, sizeof bat, "Bateria: %d%% (%.2f V%s)", pct, mv / 1000.0, carga ? ", carregando" : "");
    else
        snprintf(bat, sizeof bat, "Bateria: não encontrada");
    return snprintf(buf, (size_t)n,
                    "ESP32-P4 v%d.%d, %d MHz  ·  %.1f MB livres\n"
                    "Tela: painel V%d (JD9365 %02X %02X %02X)  ·  toque %s\n"
                    "%s\n"
                    "Firmware: %s\n",
                    ci.revision / 100, ci.revision % 100, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
                    heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1048576.0, painel_versao, id[0], id[1], id[2],
                    toque_ok() ? "ok" : "sem resposta", bat, esp_app_get_description()->version);
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
        if (agora && !antes) sis_tela_troca();
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

    for (int i = 0; i < 2; i++) {
        fb_jogo[i] = heap_caps_calloc(GFX_W * GFX_H, 2, MALLOC_CAP_SPIRAM);
        fb_livre[i] = xSemaphoreCreateBinary();
    }
    xSemaphoreGive(fb_livre[1]);               // o 0 e do jogo; o 1 esta livre
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
    bateria_inicia();

    tela_t tela = { tela_quadro(), TELA_W, TELA_H };
    console_inicia();
    ESP_LOGI(TAG, "pronto, %u KB de PSRAM livres", (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));

    int64_t antes = esp_timer_get_time(), log_em = antes + 5000000, t_quadro = 0, t_envio = 0, descanso = antes;
    int n_quadros = 0;
    bool boot_antes = true;
    for (;;) {
        int64_t agora = esp_timer_get_time();
        float dt = (agora - antes) / 1e6f;
        antes = agora;
        if (dt > 0.05f) dt = 0.05f;
        bateria_passo();

        // A cada 5 s, no log: quantos quadros por segundo e onde vai o tempo.
        if (agora >= log_em) {
            int giros;
            float ms_giro;
            tela_tempos(&giros, &ms_giro);
            float n = n_quadros ? (float)n_quadros : 1;
            ESP_LOGI(TAG, "%.1f quadros/s | desenho %.1f ms | envio %.1f ms | mesa no 2o nucleo %.1f ms (%d) | "
                     "PSRAM livre %u KB", n_quadros / 5.0f, t_quadro / 1000.0f / n, t_envio / 1000.0f / n, ms_giro,
                     giros, (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
            n_quadros = 0;
            t_quadro = t_envio = 0;
            log_em = agora + 5000000;
        }

        int x = 0, y = 0;
        bool dedo = toque_le(&x, &y);
        console_toque(dedo, x, y);

        bool boot = gpio_get_level(PIN_BOOT) == 0;
        if (boot && !boot_antes && agora > 8000000) console_botao();
        boot_antes = boot;

        console_passo(dt);
        som_atualiza(dt);
        int64_t t0 = esp_timer_get_time();
        if (tela.px && console_desenha(&tela)) {
            int64_t t1 = esp_timer_get_time();
            mostra();
            t_quadro += t1 - t0;
            t_envio += esp_timer_get_time() - t1;
            n_quadros++;
            // Desenhando sem parar, a tarefa ociosa deste nucleo nunca roda e o
            // watchdog reclama: um descanso de 1 ms a cada 200 ms.
            if (agora - descanso > 200000) {
                vTaskDelay(1);
                descanso = agora;
            }
            if (!luz_acesa) {                // so acende depois da primeira imagem
                luz_acesa = true;
                tela_brilho(brilho);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(12));
        }
    }
}
