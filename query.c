// Motor de consulta. Com menos de 3 mil fatos mapeados em flash, a varredura
// linear completa leva dezenas de microssegundos - nao ha indice nem cache.
#include <string.h>
#include "query.h"

static int cmp_qtd_desc(const void *a, const void *b)
{
    const bucket_t *x = a, *y = b;
    if (x->qtd != y->qtd) return x->qtd > y->qtd ? -1 : 1;
    return x->chave < y->chave ? -1 : 1;
}

static int cmp_chave_asc(const void *a, const void *b)
{
    const bucket_t *x = a, *y = b;
    return x->chave < y->chave ? -1 : (x->chave > y->chave);
}

static bucket_t *acha(resultado_t *r, uint16_t chave)
{
    for (int i = 0; i < r->n; i++)
        if (r->b[i].chave == chave) return &r->b[i];
    if (r->n >= MAX_BUCKETS) return NULL;
    bucket_t *b = &r->b[r->n++];
    b->chave = chave;
    b->qtd = 0;
    b->cent = 0;
    return b;
}

void query_amb(const amb_q_t *q, eixo_t eixo, resultado_t *r)
{
    memset(r, 0, sizeof *r);
    uint16_t base_per = P.h->amb_per_min;

    for (int i = 0; i < P.n_amb; i++) {
        const amb_fact_t *f = &P.amb[i];
        if (f->base != q->base) continue;
        if (!(q->proc_mask & (1u << f->proc))) continue;
        uint16_t per = base_per + f->per_rel;
        if (per < q->per_ini || per > q->per_fim) continue;

        uint64_t cent = (uint64_t)f->qtd * P.proc[f->proc].valor_cent;

        // Denominador do percentual: o mesmo recorte no estado inteiro.
        r->total_uf += f->qtd;
        r->total_uf_cent += cent;

        if (!(q->cir_mask & (1u << f->cir))) continue;

        r->total += f->qtd;
        r->total_cent += cent;
        if (!r->n_meses_vistos || per < r->per_min) r->per_min = per;
        if (!r->n_meses_vistos || per > r->per_max) r->per_max = per;
        r->n_meses_vistos++;

        uint16_t chave = eixo == EIXO_LOCAL ? f->cir
                       : eixo == EIXO_MES   ? per
                                            : f->proc;
        bucket_t *b = acha(r, chave);
        if (!b) continue;
        b->qtd += f->qtd;
        b->cent += cent;
    }
    qsort(r->b, r->n, sizeof r->b[0],
          eixo == EIXO_MES ? cmp_chave_asc : cmp_qtd_desc);
}

void query_vvpp(const vvpp_q_t *q, eixo_vvpp_t eixo, resultado_t *r)
{
    memset(r, 0, sizeof *r);
    uint16_t base_per = P.h->vvpp_per_min;
    uint32_t valor = P.meta->valor_cent;

    for (int i = 0; i < P.n_vvpp; i++) {
        const vvpp_fact_t *f = &P.vvpp[i];
        uint16_t per = base_per + f->per_rel;
        if (per < q->per_ini || per > q->per_fim) continue;

        uint64_t cent = (uint64_t)f->qtd * valor;
        r->total_uf += f->qtd;                       // total da cobertura
        r->total_uf_cent += cent;

        if (!(q->cir_mask & (1u << f->cir))) continue;

        r->total += f->qtd;
        r->total_cent += cent;
        if (!r->n_meses_vistos || per < r->per_min) r->per_min = per;
        if (!r->n_meses_vistos || per > r->per_max) r->per_max = per;
        r->n_meses_vistos++;

        uint16_t chave = eixo == EIXO_ESTAB ? f->estab
                       : eixo == EIXO_VMES  ? per
                                            : f->cir;
        bucket_t *b = acha(r, chave);
        if (!b) continue;
        b->qtd += f->qtd;
        b->cent += cent;
    }
    qsort(r->b, r->n, sizeof r->b[0],
          eixo == EIXO_VMES ? cmp_chave_asc : cmp_qtd_desc);
}

// Serie mensal continua (com zeros) para o grafico de barras.
int query_serie(const resultado_t *r_por_mes, uint16_t ini, uint16_t fim,
                uint32_t *out, int max)
{
    int n = fim - ini + 1;
    if (n > max) n = max;
    if (n < 0) n = 0;
    memset(out, 0, n * sizeof *out);
    for (int i = 0; i < r_por_mes->n; i++) {
        int k = r_por_mes->b[i].chave - ini;
        if (k >= 0 && k < n) out[k] = r_por_mes->b[i].qtd;
    }
    return n;
}

int query_pico(const uint32_t *v, int n)
{
    int melhor = 0;
    for (int i = 1; i < n; i++) if (v[i] > v[melhor]) melhor = i;
    return melhor;
}
