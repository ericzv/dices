// Acesso ao pacote de dados gravado na particao "tabnet".
//
// O pacote e mapeado direto no espaco de enderecamento com esp_partition_mmap,
// entao os registros abaixo sao lidos como structs na flash, sem copia para a
// RAM e sem sistema de arquivos. Ver FORMAT.md.
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PACK_MAGIC "TBN2"
#define PACK_VERSAO 2

enum {
    SEC_AMB_PROC = 0, SEC_AMB_GRUPO, SEC_CIR, SEC_GRUPO_LOCAL,
    SEC_AMB_FACT, SEC_VVPP_ESTAB, SEC_VVPP_FACT, SEC_VVPP_META,
    SEC_STR, SEC_N
};

typedef struct __attribute__((packed)) {
    char     magic[4];
    uint16_t versao;
    uint16_t n_secoes;
    uint32_t crc32;
    uint32_t corpo_len;
    uint16_t ext_ano, ext_mes, ext_dia;
    uint16_t amb_per_min, amb_per_max;
    uint16_t vvpp_per_min;
    uint16_t _rsv;
    uint16_t vvpp_per_max;
    struct __attribute__((packed)) { uint32_t off, len; } sec[SEC_N];
    uint8_t  _rsv2[24];       // completa 128 B; o corpo comeca logo apos
} pack_hdr_t;
_Static_assert(sizeof(pack_hdr_t) == 128, "cabecalho deve ter 128 bytes");

typedef struct __attribute__((packed)) {
    uint32_t codigo;          // SIGTAP sem pontuacao
    uint16_t s_nome, s_abrev;
    uint32_t valor_cent;
    uint32_t _rsv;
} proc_t;

typedef struct __attribute__((packed)) {
    uint16_t s_nome, s_abrev;
    uint32_t mask;            // bits dos indices de procedimento
    uint32_t _rsv;
} grupo_proc_t;

typedef struct __attribute__((packed)) {
    uint16_t codigo;          // 42001..42017
    uint16_t s_nome, s_abrev;
    uint16_t flags;
} cir_t;

typedef struct __attribute__((packed)) {
    uint16_t s_nome, s_abrev;
    uint32_t mask;            // bits dos indices de CIR
    uint16_t flags;           // bit0 = destaque
    uint16_t _pad;
} grupo_local_t;

#define GL_DESTAQUE 0x1

typedef struct __attribute__((packed)) {
    uint8_t  base;            // 0 atendimento, 1 residencia
    uint8_t  proc;
    uint8_t  cir;
    uint8_t  per_rel;         // relativo a amb_per_min
    uint32_t qtd;
} amb_fact_t;

typedef struct __attribute__((packed)) {
    uint32_t cnes;
    uint16_t s_rotulo, s_cidade;
    uint32_t _rsv;
} estab_t;

typedef struct __attribute__((packed)) {
    uint8_t  cir;
    uint8_t  estab;
    uint8_t  per_rel;         // relativo a vvpp_per_min
    uint8_t  _pad;
    uint32_t qtd;
} vvpp_fact_t;

typedef struct __attribute__((packed)) {
    uint16_t s_nome, s_abrev;
    uint32_t mask_cobertura;  // CIRs realmente presentes na extracao
    uint32_t valor_cent;
    uint8_t  estimado;
    uint8_t  _pad;
    uint16_t _pad2;
} vvpp_meta_t;

typedef struct {
    bool ok;
    const char *erro;
    const pack_hdr_t *h;
    const proc_t        *proc;   int n_proc;
    const grupo_proc_t  *gproc;  int n_gproc;
    const cir_t         *cir;    int n_cir;
    const grupo_local_t *glocal; int n_glocal;
    const amb_fact_t    *amb;    int n_amb;
    const estab_t       *estab;  int n_estab;
    const vvpp_fact_t   *vvpp;   int n_vvpp;
    const vvpp_meta_t   *meta;
    const char          *str;
} pack_t;

extern pack_t P;

bool        pack_init(void);
const char *pack_str(uint16_t off);
