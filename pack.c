#include <string.h>
#include "esp_partition.h"
#include "esp_log.h"
#include "pack.h"

static const char *TAG = "pack";
pack_t P;

// CRC32 (IEEE) proprio, para casar bit a bit com o zlib.crc32 do build_pack.py
// sem depender da convencao da funcao da ROM.
static uint32_t crc32_le(const uint8_t *d, size_t n)
{
    uint32_t c = 0xFFFFFFFF;
    for (size_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(c & 1));
    }
    return ~c;
}

const char *pack_str(uint16_t off)
{
    if (!P.ok || !P.str) return "";
    return P.str + off;
}

static const void *sec(const uint8_t *base, int i, int reg, int *n)
{
    const pack_hdr_t *h = (const pack_hdr_t *)base;
    *n = reg ? (int)(h->sec[i].len / reg) : 0;
    return base + h->sec[i].off;
}

bool pack_init(void)
{
    memset(&P, 0, sizeof P);

    const esp_partition_t *part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "tabnet");
    if (!part) { P.erro = "particao tabnet ausente"; return false; }

    const void *map;
    esp_partition_mmap_handle_t mh;
    if (esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA,
                           &map, &mh) != ESP_OK) {
        P.erro = "falha ao mapear a particao";
        return false;
    }

    const uint8_t *b = map;
    const pack_hdr_t *h = (const pack_hdr_t *)b;

    if (memcmp(h->magic, PACK_MAGIC, 4) != 0) {
        P.erro = "sem dados gravados";
        return false;
    }
    if (h->versao != PACK_VERSAO) { P.erro = "versao de formato diferente"; return false; }
    if (sizeof(pack_hdr_t) + h->corpo_len > part->size) {
        P.erro = "pacote maior que a particao"; return false;
    }
    uint32_t crc = crc32_le(b + sizeof(pack_hdr_t), h->corpo_len);
    if (crc != h->crc32) { P.erro = "CRC nao confere"; return false; }

    P.h      = h;
    P.proc   = sec(b, SEC_AMB_PROC,    sizeof(proc_t),        &P.n_proc);
    P.gproc  = sec(b, SEC_AMB_GRUPO,   sizeof(grupo_proc_t),  &P.n_gproc);
    P.cir    = sec(b, SEC_CIR,         sizeof(cir_t),         &P.n_cir);
    P.glocal = sec(b, SEC_GRUPO_LOCAL, sizeof(grupo_local_t), &P.n_glocal);
    P.amb    = sec(b, SEC_AMB_FACT,    sizeof(amb_fact_t),    &P.n_amb);
    P.estab  = sec(b, SEC_VVPP_ESTAB,  sizeof(estab_t),       &P.n_estab);
    P.vvpp   = sec(b, SEC_VVPP_FACT,   sizeof(vvpp_fact_t),   &P.n_vvpp);
    int dummy;
    P.meta   = sec(b, SEC_VVPP_META,   sizeof(vvpp_meta_t),   &dummy);
    P.str    = (const char *)(b + h->sec[SEC_STR].off);
    P.ok     = true;

    ESP_LOGI(TAG, "AMB %d fatos, VVPP %d fatos, extracao %04d-%02d-%02d",
             P.n_amb, P.n_vvpp, h->ext_ano, h->ext_mes, h->ext_dia);
    return true;
}
