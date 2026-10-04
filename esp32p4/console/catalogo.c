// Os jogos do console, na ordem da fileira do menu. Um jogo novo entra aqui
// (e com o seu adaptador, como jogo_dado.c).
#include "catalogo.h"

extern const jogo_t JOGO_DADO;

const jogo_t *const CATALOGO[] = { &JOGO_DADO };
const int N_CATALOGO = (int)(sizeof CATALOGO / sizeof CATALOGO[0]);
