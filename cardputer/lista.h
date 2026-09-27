#pragma once
#include <stdbool.h>
#include "../hal/keyboard.h"

#define LISTA_MAX 64
enum { LISTA_NADA = 0, LISTA_ESCOLHE, LISTA_CANCELA };

// Devolve o rotulo do item i; pode marcar *destaque para exibir em negrito.
typedef const char *(*lista_rotulo_fn)(void *ctx, int i, bool *destaque);

typedef struct {
    const char      *titulo;
    int              n;
    lista_rotulo_fn  rotulo;
    void            *ctx;
    int              sel, topo;
    char             filtro[16];
    int              nf;
    int              idx[LISTA_MAX];
} lista_t;

void lista_abre(lista_t *L, const char *titulo, int n,
                lista_rotulo_fn rotulo, void *ctx, int sel_inicial);
void lista_refiltra(lista_t *L);
void lista_ajusta(lista_t *L);
int  lista_tecla(lista_t *L, const key_event_t *ev);
int  lista_escolhido(const lista_t *L);
void lista_desenha(const lista_t *L);
