// Desenha as telas no PC usando o MESMO codigo de desenho do firmware e salva
// PNGs. Inclui os .c dos apps para alcancar as funcoes estaticas de desenho.
//   cc -Imain tools/preview.c main/ui/*.c main/data/query.c -o build/preview
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "board.h"
#include "data/query.h"
#include "apps/estado.h"
#include "apps/app.h"

static uint16_t FBUF[240 * 135];
uint16_t *display_fb(void) { return FBUF; }

pack_t P;
estado_t E;
const char *pack_str(uint16_t off) { return P.str + off; }
void estado_salva(void) { }
void app_vai(const app_t *a) { (void)a; }
void app_volta(void) { }
void app_marca_sujo(void) { }

static const void *sec(const uint8_t *b, int i, int reg, int *n)
{
    const pack_hdr_t *h = (const pack_hdr_t *)b;
    *n = reg ? (int)(h->sec[i].len / reg) : 0;
    return b + h->sec[i].off;
}

static void carrega(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    struct stat st; stat(path, &st);
    uint8_t *b = malloc(st.st_size);
    if (fread(b, 1, st.st_size, f) != (size_t)st.st_size) exit(1);
    fclose(f);
    const pack_hdr_t *h = (const pack_hdr_t *)b;
    P.h = h;
    P.proc   = sec(b, SEC_AMB_PROC,    sizeof(proc_t),        &P.n_proc);
    P.gproc  = sec(b, SEC_AMB_GRUPO,   sizeof(grupo_proc_t),  &P.n_gproc);
    P.cir    = sec(b, SEC_CIR,         sizeof(cir_t),         &P.n_cir);
    P.glocal = sec(b, SEC_GRUPO_LOCAL, sizeof(grupo_local_t), &P.n_glocal);
    P.amb    = sec(b, SEC_AMB_FACT,    sizeof(amb_fact_t),    &P.n_amb);
    P.estab  = sec(b, SEC_VVPP_ESTAB,  sizeof(estab_t),       &P.n_estab);
    P.vvpp   = sec(b, SEC_VVPP_FACT,   sizeof(vvpp_fact_t),   &P.n_vvpp);
    int d;
    P.meta   = sec(b, SEC_VVPP_META,   sizeof(vvpp_meta_t),   &d);
    P.str    = (const char *)(b + h->sec[SEC_STR].off);
    P.ok     = true;
}

static void dump(const char *nome)
{
    char path[64];
    snprintf(path, sizeof path, "build/%s.ppm", nome);
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n240 135\n255\n");
    for (int i = 0; i < 240 * 135; i++) {
        uint16_t c = FBUF[i];
        uint8_t r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
        uint8_t g = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
        uint8_t b = (uint8_t)((c & 0x1F) * 255 / 31);
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
}

#include "apps/amb.c"

int main(void)
{
    carrega("build/tabnet.pack");

    // --- menu -------------------------------------------------------------
    extern const app_t app_menu;
    app_menu.desenha();
    dump("01_menu");

    // --- formulario ambulatorial -----------------------------------------
    E.amb_proc = 0;      // grupo "Laser retina"
    E.amb_base = 0;      // atendimento
    E.amb_local = 1;     // Macro Sul
    E.amb_per = 2;       // 2025 + 2026
    entra();
    campo = C_CONSULTA;
    desenha();
    dump("02_form");

    campo = C_LOCAL;
    desenha();
    dump("03_form_sel");

    // --- resultado por local ---------------------------------------------
    roda();
    tela = TELA_RESULT;
    desenha();
    dump("04_result_local");

    // --- resultado mes a mes ---------------------------------------------
    eixo = EIXO_MES;
    roda();
    desenha();
    dump("05_result_mes");

    // --- mes a mes em valor ----------------------------------------------
    ver_valor = true;
    desenha();
    dump("06_result_mes_valor");

    // --- SC inteiro por local, com rolagem -------------------------------
    ver_valor = false;
    eixo = EIXO_LOCAL;
    E.amb_local = 0;     // SC
    E.amb_proc = P.n_gproc;  // Foto isolada
    roda();
    desenha();
    dump("07_result_sc");

    // --- seletor de local -------------------------------------------------
    abre_selecao(C_LOCAL);
    lista_desenha(&L);
    dump("08_seletor");

    puts("PPMs em build/");
    return 0;
}
