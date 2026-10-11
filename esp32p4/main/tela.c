#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_timer.h"
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
static uint16_t *quadro;                  // a tela deitada do console
static ppa_client_handle_t ppa;
static SemaphoreHandle_t fim_varredura;
static ppa_srm_rotation_angle_t giro = GIRO_NORMAL;
static uint8_t id_painel[3];

// Tres quadros no painel, usados em roda: um na tela, um mandado esperando o
// fim da varredura e um livre para montar o proximo. Assim montar um quadro
// nunca espera a varredura (com dois, cada quadro esperava ate 17 ms).
#define N_QUADROS 3
static void *fb[N_QUADROS];
static int ultimo;                        // o mandado por ultimo (o 0 esta na tela no comeco)
static uint32_t enviado[N_QUADROS];       // a contagem de varreduras quando cada um foi mandado
static volatile uint32_t varreduras;
static SemaphoreHandle_t trava;           // um quadro montado por vez (a PPA ou o outro nucleo)

// O giro da mesa feito pela CPU (tela_mostra_jogo) tem de bater com o da PPA.
// confere_giro descobre na partida para que lado a PPA gira com GIRO_NORMAL.
static int horario_normal = -1;           // 1 horario, 0 anti-horario, -1 nao se sabe
static bool invertido;

// O driver avisa quando o painel terminou um quadro e ja vai ler o proximo.
static IRAM_ATTR bool quadro_pronto(esp_lcd_panel_handle_t p, esp_lcd_dpi_panel_event_data_t *e, void *ctx)
{
    BaseType_t acorda = pdFALSE;
    varreduras++;
    xSemaphoreGiveFromISR(fim_varredura, &acorda);
    return acorda == pdTRUE;
}

// O proximo quadro da roda, quando o painel com certeza ja saiu dele: duas
// varreduras depois de mandado o quadro seguinte a ele, o painel esta nesse
// seguinte ou num mais novo. Com a trava.
static int proximo_quadro(void)
{
    int x = (ultimo + 1) % N_QUADROS, y = (x + 1) % N_QUADROS;
    int64_t limite = esp_timer_get_time() + 100000;           // painel parado: nao trava o console
    while ((int32_t)(varreduras - enviado[y]) < 2 && esp_timer_get_time() < limite)
        xSemaphoreTake(fim_varredura, pdMS_TO_TICKS(20));
    return x;
}

// O painel passa a ler este quadro no fim da varredura atual.
static void envia(int x)
{
    esp_lcd_panel_draw_bitmap(painel, 0, 0, LCD_W, LCD_H, fb[x]);
    enviado[x] = varreduras;
    ultimo = x;
}

// ---------------------------------------------------------------------------
// A mesa do jogo direto no quadro do painel, sem passar pela tela deitada do
// console nem pela PPA: a CPU amplia e gira de uma vez, no segundo nucleo,
// enquanto o primeiro ja desenha o quadro seguinte do jogo.
// ---------------------------------------------------------------------------
static bool gira_horario(void) { return invertido ? !horario_normal : horario_normal; }

static inline uint32_t cor_de(const uint16_t *lut, uint16_t c) { return lut ? lut[c] : c; }

// O retangulo (sx, sy, w, h) de 'src' (linhas de sw pixels), ampliado k vezes
// e passado pela paleta 'lut' (se houver), vai para (X0, Y0) da tela deitada,
// ja girado para o quadro do painel 'dst'. Em blocos de 64x32, para a cache
// dar conta da leitura em coluna e da escrita em linha.
static void gira_bloco(uint16_t *dst, const uint16_t *src, int sw, int sx, int sy, int w, int h, int k,
                       int X0, int Y0, const uint16_t *lut)
{
    bool hor = gira_horario();
    for (int tj = 0; tj < h; tj += 32) {
        int jn = h - tj < 32 ? h - tj : 32;
        int Y = Y0 + tj * k;
        for (int ti = 0; ti < w; ti += 64) {
            int in = w - ti < 64 ? w - ti : 64;
            for (int i = ti; i < ti + in; i++) {
                const uint16_t *s = src + (sy + tj) * sw + sx + i;
                for (int r = 0; r < k; r++) {
                    int X = X0 + i * k + r;
                    uint16_t *lin = dst + (hor ? X : TELA_W - 1 - X) * LCD_W;
                    if (k == 2 && !(Y & 1)) {
                        // Duas linhas da tela deitada viram dois pixels vizinhos: 32 bits.
                        if (hor) {
                            uint32_t *d = (uint32_t *)(lin + LCD_W - 2 - Y);
                            for (int j = 0; j < jn; j++, d--) { uint32_t c = cor_de(lut, s[j * sw]); *d = c | c << 16; }
                        } else {
                            uint32_t *d = (uint32_t *)(lin + Y);
                            for (int j = 0; j < jn; j++, d++) { uint32_t c = cor_de(lut, s[j * sw]); *d = c | c << 16; }
                        }
                    } else {
                        for (int j = 0; j < jn; j++) {
                            uint16_t c = (uint16_t)cor_de(lut, s[j * sw]);
                            for (int q = 0; q < k; q++) {
                                int Yq = Y + j * k + q;
                                lin[hor ? LCD_W - 1 - Yq : Yq] = c;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Para que lado a PPA gira com GIRO_NORMAL? Gira uma imagem pequena com cada
// pixel diferente e compara: 1 horario, 0 anti-horario, -1 nenhum dos dois.
static int confere_giro(void)
{
    enum { W = 16, H = 8 };
    uint16_t *a = heap_caps_aligned_calloc(128, W * H, 2, MALLOC_CAP_SPIRAM);
    uint16_t *b = heap_caps_aligned_calloc(128, W * H, 2, MALLOC_CAP_SPIRAM);
    int r = -1;
    if (a && b) {
        for (int i = 0; i < W * H; i++) a[i] = (uint16_t)(0x0841 * (i % 31) + i / 31 + 1);
        ppa_srm_oper_config_t op = {
            .in = { .buffer = a, .pic_w = W, .pic_h = H, .block_w = W, .block_h = H,
                    .srm_cm = PPA_SRM_COLOR_MODE_RGB565 },
            .out = { .buffer = b, .buffer_size = W * H * 2, .pic_w = H, .pic_h = W,
                     .srm_cm = PPA_SRM_COLOR_MODE_RGB565 },
            .rotation_angle = GIRO_NORMAL,
            .scale_x = 1.0f, .scale_y = 1.0f,
            .mode = PPA_TRANS_MODE_BLOCKING,
        };
        if (ppa_do_scale_rotate_mirror(ppa, &op) == ESP_OK) {
            bool h = true, ah = true;
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++) {
                    uint16_t c = a[y * W + x];
                    if (b[x * H + (H - 1 - y)] != c) h = false;      // horario: (x, y) -> (H-1-y, x)
                    if (b[(W - 1 - x) * H + y] != c) ah = false;     // anti-horario: (x, y) -> (y, W-1-x)
                }
            r = h ? 1 : ah ? 0 : -1;
        }
    }
    heap_caps_free(a);
    heap_caps_free(b);
    ESP_LOGI(TAG, "a PPA gira %s", r == 1 ? "no sentido horario" : r == 0 ? "no sentido anti-horario" : "de um jeito estranho");
    return r;
}

typedef struct {
    const uint16_t *px, *lut;
    int w, h, k, x, y;
    SemaphoreHandle_t livre;
} pedido_t;
static QueueHandle_t fila;
static int64_t tempo_giro;
static int n_quadros;

static void apresenta(void *arg)
{
    pedido_t p;
    int64_t descanso = 0;
    for (;;) {
        if (xQueueReceive(fila, &p, portMAX_DELAY) != pdTRUE) continue;
        if (esp_timer_get_time() - descanso > 200000) {      // a tarefa ociosa deste nucleo tambem roda
            vTaskDelay(1);
            descanso = esp_timer_get_time();
        }
        xSemaphoreTake(trava, portMAX_DELAY);
        int64_t t0 = esp_timer_get_time();
        int q = proximo_quadro();
        uint16_t *d = fb[q];
        int gw = p.w * p.k, gh = p.h * p.k;
        gira_bloco(d, p.px, p.w, 0, 0, p.w, p.h, p.k, p.x, p.y, p.lut);
        if (p.livre) xSemaphoreGive(p.livre);                  // o quadro do jogo ja pode ser redesenhado
        // O resto da tela deitada (a barra do sistema) vem do quadro do console.
        if (p.y > 0) gira_bloco(d, quadro, TELA_W, 0, 0, TELA_W, p.y, 1, 0, 0, NULL);
        if (p.y + gh < TELA_H)
            gira_bloco(d, quadro, TELA_W, 0, p.y + gh, TELA_W, TELA_H - p.y - gh, 1, 0, p.y + gh, NULL);
        if (p.x > 0) gira_bloco(d, quadro, TELA_W, 0, p.y, p.x, gh, 1, 0, p.y, NULL);
        if (p.x + gw < TELA_W) gira_bloco(d, quadro, TELA_W, p.x + gw, p.y, TELA_W - p.x - gw, gh, 1, p.x + gw, p.y, NULL);
        envia(q);
        tempo_giro += esp_timer_get_time() - t0;
        n_quadros++;
        xSemaphoreGive(trava);
    }
}

bool tela_mostra_jogo(const uint16_t *px, int w, int h, int k, int x, int y, const uint16_t *lut,
                      SemaphoreHandle_t livre)
{
    if (!fila || !quadro || k < 1 || x < 0 || y < 0 || x + w * k > TELA_W || y + h * k > TELA_H) return false;
    pedido_t p = { px, lut, w, h, k, x, y, livre };
    xQueueSend(fila, &p, portMAX_DELAY);
    return true;
}

void tela_tempos(int *quadros, float *ms_giro)
{
    *quadros = n_quadros;
    *ms_giro = n_quadros ? tempo_giro / 1000.0f / n_quadros : 0;
    n_quadros = 0;
    tempo_giro = 0;
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
    gpio_hold_dis(PIN_LCD_LUZ);          // presos em baixo se o aparelho foi desligado
    gpio_hold_dis(PIN_LCD_RST);
    luz_inicia();
    fim_varredura = xSemaphoreCreateBinary();
    trava = xSemaphoreCreateMutex();

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
        .num_fbs = N_QUADROS,
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
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(painel, N_QUADROS, &fb[0], &fb[1], &fb[2]), TAG, "quadros");
    // O fim de cada varredura: no IDF ate o 5.5.4 o aviso se chama
    // on_refresh_done; do 5.5.5 em diante, on_frame_buf_complete, no mesmo
    // lugar (os dois nomes dividem o campo). O nome antigo serve nos dois.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    esp_lcd_dpi_panel_event_callbacks_t cb = { .on_refresh_done = quadro_pronto };
#pragma GCC diagnostic pop
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_register_event_callbacks(painel, &cb, NULL), TAG, "aviso de quadro");

    quadro = heap_caps_aligned_calloc(128, TELA_W * TELA_H, 2, MALLOC_CAP_SPIRAM);
    if (!quadro) return ESP_ERR_NO_MEM;

    ppa_client_config_t cc = { .oper_type = PPA_OPERATION_SRM };
    ESP_RETURN_ON_ERROR(ppa_register_client(&cc, &ppa), TAG, "PPA");
    horario_normal = confere_giro();
    if (horario_normal >= 0) {
        fila = xQueueCreate(1, sizeof(pedido_t));
        xTaskCreatePinnedToCore(apresenta, "tela", 4096, NULL, 6, NULL, 1);
    } else {
        ESP_LOGW(TAG, "o giro da PPA nao bateu com nenhum dos dois sentidos: a mesa vai pelo caminho lento");
    }
    ESP_LOGI(TAG, "tela pronta: painel V%d, %d MHz, %d quadros", versao, versao == 1 ? 60 : 70, N_QUADROS);
    return ESP_OK;
}

uint16_t *tela_quadro(void) { return quadro; }
void tela_gira(bool g)
{
    giro = g ? GIRO_INVERTIDO : GIRO_NORMAL;
    invertido = g;
}
void tela_id(uint8_t id[3]) { memcpy(id, id_painel, 3); }

void tela_mostra(void)
{
    if (!quadro || !ppa) return;
    xSemaphoreTake(trava, portMAX_DELAY);
    int x = proximo_quadro();
    void *alvo = fb[x];
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
    if (ppa_do_scale_rotate_mirror(ppa, &op) == ESP_OK) envia(x);
    xSemaphoreGive(trava);
}

void tela_desliga(void)
{
    tela_brilho(0);
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    if (io) {
        esp_lcd_panel_io_tx_param(io, 0x28, NULL, 0);           // imagem desligada
        vTaskDelay(pdMS_TO_TICKS(20));
        esp_lcd_panel_io_tx_param(io, 0x10, NULL, 0);           // painel dormindo
        vTaskDelay(pdMS_TO_TICKS(120));
    }
    // A luz e o reset do painel ficam em baixo durante o sono.
    gpio_config_t g = { .pin_bit_mask = (1ULL << PIN_LCD_LUZ) | (1ULL << PIN_LCD_RST), .mode = GPIO_MODE_OUTPUT };
    gpio_config(&g);
    gpio_set_level(PIN_LCD_LUZ, 0);
    gpio_set_level(PIN_LCD_RST, 0);
    gpio_hold_en(PIN_LCD_LUZ);
    gpio_hold_en(PIN_LCD_RST);
}
