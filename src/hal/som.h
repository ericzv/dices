// Efeitos sonoros. No PC sao sintetizados na partida (desktop/som.c); no
// Cardputer saem do codec ES8311.
#pragma once
#include <stdbool.h>

enum {
    SOM_TIQUE,       // cursor andou
    SOM_BATE,        // dado bateu na borda ou em outro dado
    SOM_POUSA,       // dado parou
    SOM_VALIDO,      // dado entrou valendo
    SOM_ANULA,       // dado anulado
    SOM_FICHA,       // fichas na mesa
    SOM_EFEITO,      // poder resolvendo no fim da rodada
    SOM_VITORIA,     // alguem levou o pote
    SOM_CORREU,      // alguem correu da aposta
    SOM_ABERTURA,    // titulo entrando
    SOM_N
};

void som_toca(int som);
bool som_liga(bool on);      // devolve se ficou ligado
bool som_ligado(void);
bool som_falhou(void);       // o aparelho nao tem saida de audio
