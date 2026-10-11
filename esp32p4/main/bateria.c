// Carga da bateria pela tensao, como da: o JC8012P4A1 nao tem medidor de
// carga, so o divisor do BAT+ (68k/100k) no GPIO52. A tensao cai uns 70 mV
// quando a luz da tela acende e varia com o uso, entao a leitura e alisada e
// a porcentagem e so uma ideia (uns 10% para mais ou para menos).
//
// Sem sensor de "na tomada": com a bateria a 4,15 V ou mais, ela esta
// carregando (ou cheia na tomada).
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "placa.h"
#include "bateria.h"

static const char *TAG = "bateria";

static adc_oneshot_unit_handle_t adc;
static adc_cali_handle_t cali;
static adc_channel_t canal;
static int64_t proxima;
static float mv_liso = -1;                // a tensao da bateria, alisada
static int faltas;                        // leituras seguidas sem bateria

void bateria_inicia(void)
{
    adc_unit_t unidade;
    if (adc_oneshot_io_to_channel(PIN_BATERIA, &unidade, &canal) != ESP_OK) {
        ESP_LOGW(TAG, "o GPIO%d nao tem ADC", PIN_BATERIA);
        return;
    }
    adc_oneshot_unit_init_cfg_t uc = { .unit_id = unidade };
    if (adc_oneshot_new_unit(&uc, &adc) != ESP_OK) { adc = NULL; return; }
    adc_oneshot_chan_cfg_t cc = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    adc_oneshot_config_channel(adc, canal, &cc);
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cf = { .unit_id = unidade, .chan = canal, .atten = ADC_ATTEN_DB_12,
                                           .bitwidth = ADC_BITWIDTH_DEFAULT };
    if (adc_cali_create_scheme_curve_fitting(&cf, &cali) != ESP_OK) cali = NULL;
#endif
}

static int compara(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }

// A tensao no pino, em mV: a mediana de 9 leituras.
static int le_pino(void)
{
    int v[9], n = 0;
    for (int i = 0; i < 9; i++) {
        int bruto, mv;
        if (adc_oneshot_read(adc, canal, &bruto) != ESP_OK) continue;
        if (cali && adc_cali_raw_to_voltage(cali, bruto, &mv) == ESP_OK) v[n++] = mv;
        else v[n++] = bruto * 3100 / 4095;                         // sem calibracao: aproximado
    }
    if (!n) return -1;
    qsort(v, (size_t)n, sizeof v[0], compara);
    return v[n / 2];
}

void bateria_passo(void)
{
    if (!adc) return;
    int64_t agora = esp_timer_get_time();
    if (agora < proxima) return;
    proxima = agora + (mv_liso < 0 ? 300000 : 3000000);           // de 3 em 3 s
    int pino = le_pino();
    if (pino < 0) return;
    float mv = pino * (168.0f / 100.0f);                           // o divisor: (68k + 100k) / 100k
    if (mv < 2500) {                                               // nada no conector
        if (++faltas >= 3) mv_liso = -1;
        return;
    }
    faltas = 0;
    mv_liso = mv_liso < 0 ? mv : mv_liso + (mv - mv_liso) / 8;
}

// A curva de descarga de uma celula de litio, em pontos.
static int porcentagem(int mv)
{
    static const int MV[] = { 3300, 3600, 3700, 3800, 3950, 4100, 4200 };
    static const int PCT[] = { 0, 10, 25, 50, 75, 95, 100 };
    if (mv <= MV[0]) return 0;
    for (int i = 1; i < 7; i++)
        if (mv < MV[i]) return PCT[i - 1] + (PCT[i] - PCT[i - 1]) * (mv - MV[i - 1]) / (MV[i] - MV[i - 1]);
    return 100;
}

bool bateria_le(int *pct, int *mv, bool *carregando)
{
    *mv = mv_liso < 0 ? 0 : (int)mv_liso;
    *pct = porcentagem(*mv);
    *carregando = *mv >= 4150;
    return mv_liso >= 0;
}
