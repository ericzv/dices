// hal/salva.h na memoria flash (NVS): cada jogo tem o seu espaco (o id do
// catalogo) e cada chave guarda um texto. As chaves do NVS tem no maximo 15
// letras; as do jogo ("perfis", "desafio2", "vel"...) cabem.
#include <stdlib.h>
#include <string.h>
#include "nvs.h"
#include "esp_log.h"
#include "hal/salva.h"
#include "sistema.h"

static char espaco[16] = "console";

void sis_espaco(const char *nome)
{
    strncpy(espaco, nome, sizeof espaco - 1);
    espaco[sizeof espaco - 1] = 0;
}

bool salva_grava(const char *chave, const char *texto)
{
    nvs_handle_t h;
    if (nvs_open(espaco, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t e = nvs_set_blob(h, chave, texto, strlen(texto) + 1);
    if (e == ESP_OK) e = nvs_commit(h);
    nvs_close(h);
    if (e != ESP_OK) ESP_LOGW("salva", "nao gravou %s/%s (%s)", espaco, chave, esp_err_to_name(e));
    return e == ESP_OK;
}

int salva_le(const char *chave, char *buf, int n)
{
    buf[0] = 0;
    nvs_handle_t h;
    if (nvs_open(espaco, NVS_READONLY, &h) != ESP_OK) return 0;
    size_t tam = 0;
    int lidos = 0;
    if (nvs_get_blob(h, chave, NULL, &tam) == ESP_OK && tam > 0) {
        if ((int)tam <= n) {
            if (nvs_get_blob(h, chave, buf, &tam) == ESP_OK) lidos = (int)tam - 1;
        } else {
            char *tmp = malloc(tam);              // maior que o buffer: corta
            if (tmp && nvs_get_blob(h, chave, tmp, &tam) == ESP_OK) {
                memcpy(buf, tmp, (size_t)n - 1);
                buf[n - 1] = 0;
                lidos = n - 1;
            }
            free(tmp);
        }
    }
    nvs_close(h);
    if (lidos < 0) lidos = 0;
    buf[lidos] = 0;
    return lidos;
}

void salva_apaga(const char *chave)
{
    nvs_handle_t h;
    if (nvs_open(espaco, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_erase_key(h, chave);
    nvs_commit(h);
    nvs_close(h);
}
