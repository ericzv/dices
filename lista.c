#include <string.h>
#include <ctype.h>
#include "../hal/keyboard.h"
#include "gfx.h"
#include "lista.h"

#define VISIVEIS 8          // linhas 2..9

static bool casa(const char *texto, const char *filtro)
{
    if (!*filtro) return true;
    for (const char *p = texto; *p; p++) {
        const char *a = p, *b = filtro;
        while (*b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
        if (!*b) return true;
    }
    return false;
}

void lista_abre(lista_t *L, const char *titulo, int n,
                lista_rotulo_fn rotulo, void *ctx, int sel_inicial)
{
    memset(L, 0, sizeof *L);
    L->titulo = titulo;
    L->n = n;
    L->rotulo = rotulo;
    L->ctx = ctx;
    lista_refiltra(L);
    for (int i = 0; i < L->nf; i++)
        if (L->idx[i] == sel_inicial) { L->sel = i; break; }
    lista_ajusta(L);
}

void lista_refiltra(lista_t *L)
{
    L->nf = 0;
    for (int i = 0; i < L->n && L->nf < LISTA_MAX; i++) {
        bool destaque = false;
        const char *r = L->rotulo(L->ctx, i, &destaque);
        if (casa(r, L->filtro)) L->idx[L->nf++] = i;
    }
    if (L->sel >= L->nf) L->sel = L->nf ? L->nf - 1 : 0;
    lista_ajusta(L);
}

void lista_ajusta(lista_t *L)
{
    if (L->sel < L->topo) L->topo = L->sel;
    if (L->sel >= L->topo + VISIVEIS) L->topo = L->sel - VISIVEIS + 1;
    if (L->topo < 0) L->topo = 0;
}

int lista_tecla(lista_t *L, const key_event_t *ev)
{
    switch (ev->key) {
    case KEY_UP:    if (L->sel > 0) L->sel--;        lista_ajusta(L); return LISTA_NADA;
    case KEY_DOWN:  if (L->sel + 1 < L->nf) L->sel++; lista_ajusta(L); return LISTA_NADA;
    case KEY_ESC:   return LISTA_CANCELA;
    case KEY_ENTER: return L->nf ? LISTA_ESCOLHE : LISTA_NADA;
    case KEY_BKSP: {
        int n = strlen(L->filtro);
        if (n) L->filtro[n - 1] = 0;
        lista_refiltra(L);
        return LISTA_NADA;
    }
    default:
        if (ev->key >= 0x20 && ev->key < 0x7F) {
            int n = strlen(L->filtro);
            if (n < (int)sizeof L->filtro - 1) {
                L->filtro[n] = (char)ev->key;
                L->filtro[n + 1] = 0;
                lista_refiltra(L);
            }
        }
        return LISTA_NADA;
    }
}

int lista_escolhido(const lista_t *L)
{
    return L->nf ? L->idx[L->sel] : -1;
}

void lista_desenha(const lista_t *L)
{
    gfx_clear(COL_FUNDO);
    gfx_faixa(0, COL_PAINEL2);
    gfx_at(1, 0, L->titulo, COL_ACENTO, true);
    gfx_rightf(GRID_COLS - 2, 0, COL_TEXTO3, false, "%d", L->nf);

    for (int i = 0; i < VISIVEIS; i++) {
        int k = L->topo + i;
        if (k >= L->nf) break;
        int row = 2 + i;
        bool destaque = false;
        const char *r = L->rotulo(L->ctx, L->idx[k], &destaque);
        bool cur = (k == L->sel);
        if (cur) {
            gfx_faixa(row, COL_PAINEL2);
            gfx_stripe(row, COL_ACENTO);
        }
        gfx_at(2, row, r, cur ? COL_TEXTO : (destaque ? COL_ACENTO : COL_TEXTO2),
               destaque);
    }

    gfx_faixa(10, COL_PAINEL2);
    if (L->filtro[0])
        gfx_atf(1, 10, COL_ACENTO, false, "/%s", L->filtro);
    else
        gfx_at(1, 10, "digite para filtrar", COL_TEXTO3, false);
    gfx_right(GRID_COLS - 2, 10, "ENTER  ESC", COL_TEXTO3, false);
}
