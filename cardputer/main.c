#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "board.h"
#include "hal/display.h"
#include "hal/keyboard.h"
#include "hal/bateria.h"
#include "ui/gfx.h"
#include "data/pack.h"
#include "data/lexico.h"
#include "data/historia.h"
#include "apps/app.h"
#include "apps/estado.h"

static const char *TAG = "tabnet";

#define PILHA_MAX 4
static const app_t *pilha[PILHA_MAX];
static int topo;
static bool sujo = true;

void app_marca_sujo(void) { sujo = true; }

void app_vai(const app_t *a)
{
    if (topo + 1 >= PILHA_MAX) return;
    pilha[++topo] = a;
    if (a->entra) a->entra();
    sujo = true;
}

void app_volta(void)
{
    if (topo > 0) topo--;
    sujo = true;
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    display_init();
    keyboard_init();
    bateria_init();

    if (!pack_init())
        ESP_LOGE(TAG, "pacote invalido: %s", P.erro ? P.erro : "?");
    lexico_init();
    historia_init();
    estado_carrega();

    pilha[0] = &app_menu;
    topo = 0;
    if (app_menu.entra) app_menu.entra();

    key_event_t ev;
    while (1) {
        bool teve = false;
        // Drena o FIFO do TCA8418 (ate 10 eventos) antes de redesenhar.
        for (int i = 0; i < 10 && keyboard_read(&ev); i++) {
            pilha[topo]->tecla(&ev);
            teve = true;
        }
        if (pilha[topo]->tick) pilha[topo]->tick();
        if (sujo || teve) {
            pilha[topo]->desenha();
            display_flush();
            sujo = false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
