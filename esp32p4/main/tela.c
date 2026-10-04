#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/ppa.h"
#include "placa.h"
#include "tela.h"
#include "painel_jd9365.h"
#include "desenho.h"

static const char *TAG = "tela";

static esp_lcd_panel_handle_t painel;
static esp_lcd_panel_io_handle_t io;
static void *fb[2];
static int atras = 1;                     // o quadro do painel em que se desenha agora
static uint16_t *quadro;                  // a tela deitada do console
static ppa_client_handle_t ppa;
static SemaphoreHandle_t fim_varredura;
static ppa_srm_rotation_angle_t giro = GIRO_NORMAL;
static uint8_t id_painel[3];

// O driver avisa quando o painel terminou um quadro e ja vai ler o proximo.
static IRAM_ATTR bool quadro_pronto(esp_lcd_panel_handle_t p, esp_lcd_dpi_panel_event_data_t *e, void *ctx)
{
    BaseType_t acorda = pdFALSE;
    xSemaphoreGiveFromISR(fim_varredura, &acorda);
    return acorda == pdTRUE;
}

static void luz_inicia(void)
{
    ledc_timer_config_t t = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 20000,                 // acima do audivel: o conversor nao chia
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));
    ledc_channel_config_t c = {
        .gpio_num = PIN_LCD_LUZ,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

void tela_brilho(int pct)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    // A luz nao e linear para o olho: o comeco da escala precisa de mais passos.
    uint32_t duty = (uint32_t)(1023 * pct * pct / 10000);
    if (pct > 0 && duty < 12) duty = 12;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

static esp_err_t comando(uint8_t cmd, uint8_t valor)
{
    return esp_lcd_panel_io_tx_param(io, cmd, &valor, 1);
}

esp_err_t tela_inicia(int versao)
{
    luz_inicia();
    fim_varredura = xSemaphoreCreateBinary();

    // A PHY do MIPI-DSI e alimentada por um LDO do proprio chip.
    esp_ldo_channel_handle_t ldo = NULL;
    esp_ldo_channel_config_t lc = { .chan_id = LDO_DSI_CANAL, .voltage_mv = LDO_DSI_MV };
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&lc, &ldo), TAG, "LDO do DSI");

    // phy_clk_src fica em zero: o IDF escolhe o relogio que serve ao chip. O
    // valor fixo "DEFAULT" derruba o ESP32-P4 v3 na partida da tela.
    esp_lcd_dsi_bus_handle_t bus;
    esp_lcd_dsi_bus_config_t bc = {
        .bus_id = 0,
        .num_data_lanes = 2,
        .lane_bit_rate_mbps = versao == 1 ? 1000 : 1500,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bc, &bus), TAG, "barramento DSI");
    esp_lcd_dbi_io_config_t dc = { .virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8 };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(bus, &dc, &io), TAG, "comandos DSI");

    esp_lcd_dpi_panel_config_t pc = {
        .virtual_channel = 0,
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = versao == 1 ? 60 : 70,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = 2,
        .video_timing = {
            .h_size = LCD_W,
            .v_size = LCD_H,
            .hsync_pulse_width = 20,
            .hsync_back_porch = 20,
            .hsync_front_porch = 40,
            .vsync_pulse_width = 4,
            .vsync_back_porch = versao == 1 ? 8 : 10,
            .vsync_front_porch = 20,
        },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_dpi(bus, &pc, &painel), TAG, "painel DPI");

    // Reinicio por hardware e a sequencia do fabricante para esta tela.
    gpio_config_t g = { .pin_bit_mask = 1ULL << PIN_LCD_RST, .mode = GPIO_MODE_OUTPUT };
    gpio_config(&g);
    gpio_set_level(PIN_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(PIN_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    if (esp_lcd_panel_io_rx_param(io, 0x04, id_painel, 3) == ESP_OK)
        ESP_LOGI(TAG, "JD9365 respondeu: %02X %02X %02X", id_painel[0], id_painel[1], id_painel[2]);
    else
        ESP_LOGW(TAG, "o JD9365 nao respondeu a leitura do ID");

    comando(0xE0, 0x00);                  // pagina do usuario
    comando(0x3A, 0x55);                  // RGB565
    const uint8_t (*seq)[2] = versao == 1 ? JD9365_V1 : JD9365_V2;
    int n = versao == 1 ? (int)(sizeof JD9365_V1 / 2) : (int)(sizeof JD9365_V2 / 2);
    for (int i = 0; i < n; i++) {
        esp_err_t e = comando(seq[i][0], seq[i][1]);
        if (e != ESP_OK) {
            ESP_LOGE(TAG, "comando %d (0x%02X) falhou", i, seq[i][0]);
            return e;
        }
    }
    comando(0x11, 0x00);                  // sai do sono
    vTaskDelay(pdMS_TO_TICKS(120));
    comando(0x29, 0x00);                  // liga a imagem
    vTaskDelay(pdMS_TO_TICKS(20));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(painel), TAG, "inicio do DPI");
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(painel, 2, &fb[0], &fb[1]), TAG, "quadros");
    esp_lcd_dpi_panel_event_callbacks_t cb = { .on_frame_buf_complete = quadro_pronto };
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_register_event_callbacks(painel, &cb, NULL), TAG, "aviso de quadro");

    quadro = heap_caps_aligned_calloc(128, TELA_W * TELA_H, 2, MALLOC_CAP_SPIRAM);
    if (!quadro) return ESP_ERR_NO_MEM;

    ppa_client_config_t cc = { .oper_type = PPA_OPERATION_SRM };
    ESP_RETURN_ON_ERROR(ppa_register_client(&cc, &ppa), TAG, "PPA");
    ESP_LOGI(TAG, "tela pronta: painel V%d, %d MHz", versao, versao == 1 ? 60 : 70);
    return ESP_OK;
}

uint16_t *tela_quadro(void) { return quadro; }
void tela_gira(bool g) { giro = g ? GIRO_INVERTIDO : GIRO_NORMAL; }
void tela_id(uint8_t id[3]) { memcpy(id, id_painel, 3); }

void tela_mostra(void)
{
    if (!quadro || !ppa) return;
    void *alvo = fb[atras];
    ppa_srm_oper_config_t op = {
        .in = {
            .buffer = quadro,
            .pic_w = TELA_W, .pic_h = TELA_H,
            .block_w = TELA_W, .block_h = TELA_H,
            .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
        },
        .out = {
            .buffer = alvo,
            .buffer_size = LCD_W * LCD_H * 2,
            .pic_w = LCD_W, .pic_h = LCD_H,
            .srm_cm = PPA_SRM_COLOR_MODE_RGB565,
        },
        .rotation_angle = giro,
        .scale_x = 1.0f,
        .scale_y = 1.0f,
        .mode = PPA_TRANS_MODE_BLOCKING,
    };
    if (ppa_do_scale_rotate_mirror(ppa, &op) != ESP_OK) return;
    // O painel passa a ler este quadro no fim da varredura atual; ate la, o
    // outro ainda esta na tela e nao pode ser mexido.
    esp_lcd_panel_draw_bitmap(painel, 0, 0, LCD_W, LCD_H, alvo);
    xSemaphoreTake(fim_varredura, 0);
    xSemaphoreTake(fim_varredura, pdMS_TO_TICKS(50));
    atras ^= 1;
}
