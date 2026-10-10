// Efeitos sonoros. No PC sao gravacoes embutidas no executavel
// (desktop/som.c, desktop/sons/); no Cardputer saem do codec ES8311.
#pragma once
#include <stdbool.h>

enum {
    SOM_TIQUE,       // cursor andou
    SOM_BATE,        // dado bateu na borda de madeira
    SOM_POUSA,       // dado parou
    SOM_VALIDO,      // dado entrou valendo
    SOM_ANULA,       // dado anulado
    SOM_FICHA,       // fichas na mesa
    SOM_EFEITO,      // poder resolvendo no fim da rodada
    SOM_VITORIA,     // alguem levou o pote
    SOM_CORREU,      // alguem correu da aposta
    SOM_ABERTURA,    // titulo entrando
    SOM_JACKPOT,     // tres iguais com o Jackpot na mesa
    SOM_LASER,       // o raio do Laser
    SOM_TIRA,        // pontos saindo do placar (Espinhoso, Pirata, Maldito)
    SOM_ROUBA,       // um dado levado para o outro lado
    SOM_FOGO,        // a Ficha de Fogo queimando
    SOM_ZERA,        // Tudo ou Nada: a rodada vale zero
    SOM_VENTO,       // a Ventania rolando os dados de novo
    SOM_PARTIDA,     // comeca a partida
    SOM_FIM,         // a partida acabou: vitoria
    SOM_DERROTA,     // a partida acabou: derrota
    SOM_TURNO,       // um jogador parou: fim do turno
    SOM_BOLSA,       // dados saindo da bolsa: comeca uma rodada
    SOM_EMPATE,      // rodada empatada: o pote fica
    SOM_SLOT,        // um rolo da roleta do premio parou
    SOM_PREMIO,      // a roleta parou toda: o premio saiu
    SOM_VIDRO,       // o dado de Vidro se estilhacando
    SOM_BIGORNA,     // a marreta do Ferreiro na bigorna
    SOM_VALIDO2,     // dado valendo na 2a, 3a e 4a posicao: o mesmo arpejo,
    SOM_VALIDO3,     // cada vez um pouco mais agudo; o 4o fecha a fila
    SOM_VALIDO4,
    SOM_BATE_DADO,   // dado bateu em outro dado
    SOM_PASSA,       // o mouse passou por cima de algo que se escolhe
    SOM_MOEDA_BATE,  // uma moeda bateu na borda ou num dado
    SOM_MOEDA_POUSA, // uma moeda parou na mesa
    SOM_N
};

void som_toca(int som);
bool som_liga(bool on);      // devolve se ficou ligado
bool som_ligado(void);
bool som_falhou(void);       // o aparelho nao tem saida de audio
void som_rufo(bool on);      // o dado rolando no feltro, enquanto rola (false para tambem a moeda)
void som_rufo_moeda(void);   // no lugar do som_rufo(true) quando quem rola e uma moeda
void som_giro(bool on);      // arpejo de caca-niquel enquanto a roleta gira
