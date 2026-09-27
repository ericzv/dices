#pragma once
#include <stdlib.h>
#include "pack.h"

#define MAX_BUCKETS 256

typedef enum { EIXO_LOCAL, EIXO_MES, EIXO_PROC } eixo_t;
typedef enum { EIXO_ESTAB, EIXO_VMES, EIXO_VCIR } eixo_vvpp_t;

typedef struct {
    uint16_t chave;      // indice de CIR / periodo absoluto / indice de proc
    uint32_t qtd;
    uint64_t cent;
} bucket_t;

typedef struct {
    int      n;
    bucket_t b[MAX_BUCKETS];
    uint32_t total;
    uint64_t total_cent;
    uint32_t total_uf;        // mesmo recorte sem o filtro de local
    uint64_t total_uf_cent;
    uint16_t per_min, per_max;
    int      n_meses_vistos;
} resultado_t;

typedef struct {
    uint8_t  base;            // 0 atendimento, 1 residencia
    uint32_t proc_mask;
    uint32_t cir_mask;
    uint16_t per_ini, per_fim;
} amb_q_t;

typedef struct {
    uint32_t cir_mask;
    uint16_t per_ini, per_fim;
} vvpp_q_t;

void query_amb(const amb_q_t *q, eixo_t eixo, resultado_t *r);
void query_vvpp(const vvpp_q_t *q, eixo_vvpp_t eixo, resultado_t *r);
int  query_serie(const resultado_t *r_por_mes, uint16_t ini, uint16_t fim,
                 uint32_t *out, int max);
int  query_pico(const uint32_t *v, int n);
