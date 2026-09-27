// Roda o motor de consulta no PC, contra o mesmo tabnet.pack que vai para a
// flash. Serve para conferir o layout dos structs e os totais antes de gravar.
//   cc -I../main tools/host_test.c main/data/query.c -o build/host_test
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "data/query.h"

static const char *fmt_per(uint16_t p)
{
    static char b[4][10]; static int n;
    char *o = b[n = (n + 1) & 3];
    snprintf(o, 10, "%02d/%04d", p % 12 + 1, p / 12);
    return o;
}

static uint32_t crc32_le(const uint8_t *d, size_t n)
{
    uint32_t c = 0xFFFFFFFF;
    for (size_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320 & -(c & 1));
    }
    return ~c;
}

pack_t P;
const char *pack_str(uint16_t off) { return P.str + off; }

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
    if (fread(b, 1, st.st_size, f) != (size_t)st.st_size) { puts("leitura curta"); exit(1); }
    fclose(f);

    const pack_hdr_t *h = (const pack_hdr_t *)b;
    if (memcmp(h->magic, PACK_MAGIC, 4)) { puts("magic errado"); exit(1); }
    if (h->versao != PACK_VERSAO) { puts("versao errada"); exit(1); }
    if (sizeof(pack_hdr_t) + h->corpo_len != (size_t)st.st_size) {
        printf("tamanho inconsistente: hdr %zu + corpo %u != arquivo %ld\n",
               sizeof(pack_hdr_t), h->corpo_len, (long)st.st_size);
        exit(1);
    }
    if (crc32_le(b + sizeof(pack_hdr_t), h->corpo_len) != h->crc32) {
        puts("CRC NAO CONFERE"); exit(1);
    }
    puts("CRC ok, tamanho ok");

    P.h      = h;
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

static int idx_proc(const char *abrev)
{
    for (int i = 0; i < P.n_proc; i++)
        if (!strcmp(pack_str(P.proc[i].s_abrev), abrev)) return i;
    fprintf(stderr, "procedimento %s nao existe\n", abrev);
    exit(1);
}

static int idx_grupo_local(const char *abrev)
{
    for (int i = 0; i < P.n_glocal; i++)
        if (!strcmp(pack_str(P.glocal[i].s_abrev), abrev)) return i;
    fprintf(stderr, "grupo de local %s nao existe\n", abrev);
    exit(1);
}

static uint32_t soma(const char *proc, int base, const char *local, int ano)
{
    amb_q_t q = {
        .base = base,
        .proc_mask = 1u << idx_proc(proc),
        .cir_mask = P.glocal[idx_grupo_local(local)].mask,
        .per_ini = ano * 12, .per_fim = ano * 12 + 11,
    };
    resultado_t r;
    query_amb(&q, EIXO_LOCAL, &r);
    return r.total;
}

int main(int argc, char **argv)
{
    carrega(argc > 1 ? argv[1] : "build/tabnet.pack");

    printf("tamanhos: hdr=%zu proc=%zu grupo=%zu cir=%zu glocal=%zu "
           "amb=%zu estab=%zu vvpp=%zu meta=%zu\n",
           sizeof(pack_hdr_t), sizeof(proc_t), sizeof(grupo_proc_t),
           sizeof(cir_t), sizeof(grupo_local_t), sizeof(amb_fact_t),
           sizeof(estab_t), sizeof(vvpp_fact_t), sizeof(vvpp_meta_t));
    printf("secoes: proc=%d grupo=%d cir=%d glocal=%d amb=%d estab=%d vvpp=%d\n",
           P.n_proc, P.n_gproc, P.n_cir, P.n_glocal, P.n_amb, P.n_estab, P.n_vvpp);
    printf("extracao %02d/%02d/%04d   AMB %s a %s\n\n",
           P.h->ext_dia, P.h->ext_mes, P.h->ext_ano,
           fmt_per(P.h->amb_per_min), fmt_per(P.h->amb_per_max));

    // --- totais por procedimento/base/ano (para bater com o build_pack) -----
    puts("TOTAIS AMB (SC)");
    for (int i = 0; i < P.n_proc; i++) {
        const char *a = pack_str(P.proc[i].s_abrev);
        for (int base = 0; base < 2; base++)
            printf("  %-13s %-11s 2025 %8u   2026 %8u\n", a,
                   base ? "residencia" : "atendimento",
                   soma(a, base, "SC", 2025), soma(a, base, "SC", 2026));
    }

    // --- consulta completa: laser em retina, Macro Sul, 2025+2026 ----------
    puts("\nLASER RETINA | atendimento | Macro Sul | 2025+2026 | por local");
    amb_q_t q = {
        .base = 0,
        .proc_mask = P.gproc[0].mask,
        .cir_mask = P.glocal[idx_grupo_local("Macro Sul")].mask,
        .per_ini = 2025 * 12, .per_fim = 2026 * 12 + 11,
    };
    resultado_t r;
    query_amb(&q, EIXO_LOCAL, &r);
    printf("  total %u  (%.1f%% de SC)  R$ %llu\n", r.total,
           100.0 * r.total / (r.total_uf ? r.total_uf : 1),
           (unsigned long long)(r.total_cent / 100));
    for (int i = 0; i < r.n; i++)
        printf("    %-14s %6u  %3d%%\n", pack_str(P.cir[r.b[i].chave].s_abrev),
               r.b[i].qtd, (int)(100ULL * r.b[i].qtd / r.total));

    // --- VVPP: Macro Sul, serie toda, por estabelecimento ------------------
    puts("\nVVPP | Macro Sul | serie toda | por estabelecimento");
    vvpp_q_t vq = {
        .cir_mask = P.meta->mask_cobertura,
        .per_ini = P.h->vvpp_per_min, .per_fim = P.h->vvpp_per_max,
    };
    resultado_t vr;
    query_vvpp(&vq, EIXO_ESTAB, &vr);
    int meses = P.h->vvpp_per_max - P.h->vvpp_per_min + 1;
    printf("  total %u AIH em %d meses (%.1f/mes)  R$ %llu estimado\n",
           vr.total, meses, (float)vr.total / meses,
           (unsigned long long)(vr.total_cent / 100));
    for (int i = 0; i < vr.n; i++)
        printf("    %-16s %5u  %3d%%\n", pack_str(P.estab[vr.b[i].chave].s_rotulo),
               vr.b[i].qtd, (int)(100ULL * vr.b[i].qtd / vr.total));

    puts("\nVVPP | por residencia");
    query_vvpp(&vq, EIXO_VCIR, &vr);
    for (int i = 0; i < vr.n; i++)
        printf("    %-16s %5u  %3d%%\n", pack_str(P.cir[vr.b[i].chave].s_abrev),
               vr.b[i].qtd, (int)(100ULL * vr.b[i].qtd / vr.total));

    // --- serie mensal -------------------------------------------------------
    puts("\nVVPP | serie mensal (Macro Sul)");
    resultado_t vm;
    query_vvpp(&vq, EIXO_VMES, &vm);
    uint32_t serie[64];
    int n = query_serie(&vm, vq.per_ini, vq.per_fim, serie, 64);
    for (int i = 0; i < n; i++) {
        printf("    %s %3u ", fmt_per(vq.per_ini + i), serie[i]);
        for (unsigned j = 0; j < serie[i]; j++) putchar('#');
        putchar('\n');
    }
    return 0;
}
