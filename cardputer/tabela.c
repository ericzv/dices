#include <string.h>
#include <stdio.h>
#include "gfx.h"
#include "tabela.h"

#define COL_ROTULO 1
#define COL_QTD    29
#define COL_FIM    39

void tabela_desenha(int row_ini, int row_fim, const tabela_linha_t *l, int n,
                    int topo, uint32_t total, bool ver_valor)
{
    uint32_t max = 1;
    for (int i = 0; i < n; i++) if (l[i].qtd > max) max = l[i].qtd;

    int visiveis = row_fim - row_ini + 1;
    for (int i = 0; i < visiveis; i++) {
        int k = topo + i;
        if (k >= n) break;
        int row = row_ini + i;
        const tabela_linha_t *x = &l[k];

        // Barra proporcional ao fundo: da a forma da serie sem esconder o numero.
        int w = (int)((uint64_t)x->qtd * GFX_W / max);
        gfx_faixa_px(row, w, x->destaque ? COL_BARRA2 : COL_BARRA);
        if (x->destaque) gfx_stripe(row, COL_ACENTO);

        // 14 colunas ate o numero; o rotulo ja vem cortado na origem
        char rot[15];
        strncpy(rot, x->rotulo, sizeof rot - 1);
        rot[sizeof rot - 1] = 0;
        gfx_at(COL_ROTULO, row, rot, x->destaque ? COL_ACENTO : COL_TEXTO,
               x->destaque);

        if (ver_valor) {
            gfx_right(COL_FIM, row, fmt_reais(x->cent), COL_CIANO, false);
        } else {
            gfx_right(COL_QTD, row, fmt_int(x->qtd), COL_TEXTO, false);
            if (total)
                gfx_rightf(COL_FIM, row, COL_TEXTO3, false, "%d%%",
                           (int)((uint64_t)x->qtd * 100 / total));
        }
    }
}
