// DADO EM CASA - duelo de apostas com dados, para dois jogadores.
//
// Cada rodada, os dois montam uma FILA de quatro dados e jogam um por um.
// Depois de cada dado: parar ou arriscar o proximo. O primeiro sempre vale; os
// seguintes precisam ser IGUAIS OU MAIORES que o ultimo dado que vale. Se sair
// menor, o dado novo e o ultimo que valia sao anulados juntos. Anulado tudo, o
// proximo dado recomeca a fila sozinho.
//
//   * A ORDEM da fila decide o risco: dado grande na frente levanta a barra.
//   * QUEM VENCEU A ULTIMA RODADA JOGA PRIMEIRO - no escuro. Quem joga depois
//     sabe o alvo. A vantagem de informacao vai para quem perdeu.
//   * A APOSTA acontece entre os dois turnos, com a pontuacao do primeiro na
//     mesa.
//
// Os poderes que mexem no rival resolvem no fim, um de cada vez e animados:
// um traco de quem ataca ate o dado atingido, o dado roubado atravessando a
// mesa. Primeiro os poderes de quem jogou primeiro; um dado anulado por um
// ataque nao dispara o proprio poder depois.
//
// Versao PC: mesa de 640x360 (a do Cardputer, 240x135, esta em
// cardputer/dado.c). Os textos sao UTF-8, com acentos.
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_timer.h"
#include "../ui/gfx.h"
#include "../hal/keyboard.h"
#include "../hal/som.h"
#include "../hal/display.h"
#include "../hal/salva.h"
#include "jogos.h"

#ifndef DADO_VERSAO                  // o CMake passa o commit; sem ele, "dev"
#define DADO_VERSAO "dev"
#endif

#define C_FELTRO     RGB(18, 49, 36)
#define C_FELTRO2    RGB(24, 62, 46)
#define C_FELTRO_ESC RGB(11, 33, 24)
#define C_FELTRO_MEIO RGB(15, 41, 30)
#define C_LATAO      RGB(190, 156, 86)
#define C_LATAO_ESC  RGB(106, 86, 44)
#define C_BARRA      RGB(14, 14, 12)
#define C_MARFIM     RGB(238, 230, 212)
#define C_MARFIM_S   RGB(204, 194, 172)
#define C_EBANO      RGB(36, 32, 29)
#define C_EBANO_B    RGB(62, 56, 50)
#define C_ROSA       RGB(226, 164, 168)
#define C_VINHO      RGB(150, 62, 80)
#define C_VINHO_CLR  RGB(208, 98, 108)
#define C_AGUA       RGB(72, 150, 138)
#define C_VIOLETA    RGB(102, 74, 138)
#define C_ACO        RGB(140, 150, 164)
#define C_OURO       RGB(228, 194, 106)
#define C_TERRA      RGB(156, 106, 64)
#define C_SOMBRA     RGB(8, 24, 17)
#define C_VERDE      RGB(88, 152, 84)        // Moeda Semente
#define C_FOGO       RGB(214, 112, 44)       // Ficha de Fogo: lado 1
#define C_BRASA      RGB(190, 56, 60)        // lado 2
#define C_AMARELO    RGB(246, 222, 132)
#define C_CINZA      RGB(128, 120, 112)      // cinza do que queimou
#define C_LIMA       RGB(130, 204, 112)       // Invejoso: verde de inveja
#define C_ROXO       RGB(102, 74, 138)        // Maldito
#define C_PRATA      RGB(184, 194, 208)      // Espelho
#define C_OSSO       RGB(204, 194, 172)      // chifres do Teimoso
#define C_CORACAO    RGB(190, 56, 60)
#define C_SALMAO     RGB(226, 164, 168)      // faixa de imunidade (pastel)
#define C_TREVO      RGB(88, 152, 84)
#define C_MARFIM_D   RGB(238, 230, 212)      // corpo dos dados de ERIC: off-white
#define C_AZUL       RGB(56, 86, 150)       // ficha da Moeda da sorte: lado 1
#define C_VERMELHO   RGB(190, 56, 60)        // lado 2
#define C_TEXTO_M    RGB(170, 190, 178)
#define C_BRANCO     RGB(252, 248, 236)
#define C_CIANO      RGB(96, 182, 198)       // Copiador
#define C_CHUMBO     RGB(70, 76, 88)        // gota de chumbo do Viciado
#define C_REAL       RGB(150, 112, 160)       // purpura real
#define C_EGO        RGB(150, 112, 160)      // Egoista: ameixa, sem gritar
#define C_LASER      RGB(130, 204, 112)       // raio do Laser
#define C_VENTO      RGB(168, 204, 230)      // Ventania
#define C_CARVALHO   RGB(156, 106, 64)       // Cavalo de Troia: madeira do cavalo
#define C_SANGUE     RGB(150, 62, 80)        // Berserker
#define C_TEIA       RGB(140, 150, 164)      // Viuva Negra: aro e fios da teia
#define C_HALO       RGB(184, 194, 208)      // Misericordioso: aureola
#define C_NOITE      RGB(96, 134, 196)       // Solitario: azul de noite
#define C_BRONZE     RGB(156, 106, 64)       // Sentinela: aro
#define C_PEDRA      RGB(166, 154, 132)      // Sentinela: pedra de muralha
#define C_FERRO      RGB(104, 112, 124)       // Ferreiro: bigorna
#define C_COBRE      RGB(156, 106, 64)       // Cobrador
#define C_CARMIM     RGB(190, 56, 60)        // Desafiante
#define C_VIDRO      RGB(168, 204, 230)      // Vidro
#define C_AMBAR      RGB(238, 162, 74)       // Acumulador
#define C_MADEIRA    RGB(78, 46, 30)
#define C_MADEIRA_ESC RGB(56, 32, 20)
#define C_MADEIRA_CLR RGB(120, 78, 50)

// ===========================================================================
// A paleta: 52 cores, em familias de tons vizinhos. O quadro inteiro sai so
// nelas (gfx_quantiza no fim de dado_desenha): misturas, sombras e
// transparencias caem sempre numa cor daqui. Cor nova so entra se couber numa
// familia.
// ===========================================================================
static const uint16_t PALETA[] = {
    // escuros quentes: barras, ebano, pardo
    RGB(14, 14, 12), RGB(24, 22, 20), RGB(36, 32, 29), RGB(62, 56, 50), RGB(96, 88, 80), RGB(128, 120, 112),
    // cinzas frios: chumbo, aco, prata
    RGB(70, 76, 88), RGB(104, 112, 124), RGB(140, 150, 164), RGB(184, 194, 208),
    // marfim
    RGB(252, 248, 236), RGB(238, 230, 212), RGB(204, 194, 172), RGB(166, 154, 132),
    // feltro, do mais escuro ao realce, e o texto discreto (PAL_FELTRO)
    RGB(8, 24, 17), RGB(11, 33, 24), RGB(15, 41, 30), RGB(18, 49, 36), RGB(24, 62, 46), RGB(40, 84, 62),
    RGB(170, 190, 178),
    // verdes
    RGB(88, 152, 84), RGB(130, 204, 112),
    // madeira
    RGB(56, 32, 20), RGB(78, 46, 30), RGB(120, 78, 50), RGB(156, 106, 64),
    // latao e ouro
    RGB(106, 86, 44), RGB(150, 122, 62), RGB(190, 156, 86), RGB(228, 194, 106), RGB(246, 222, 132),
    // vinho e vermelho
    RGB(66, 18, 24), RGB(118, 30, 38), RGB(150, 62, 80), RGB(190, 56, 60), RGB(208, 98, 108), RGB(226, 164, 168),
    // fogo
    RGB(214, 112, 44), RGB(238, 162, 74),
    // azuis
    RGB(26, 34, 62), RGB(56, 86, 150), RGB(96, 134, 196), RGB(168, 204, 230),
    // agua
    RGB(72, 150, 138), RGB(96, 182, 198),
    // roxos
    RGB(54, 36, 74), RGB(102, 74, 138), RGB(150, 112, 160), RGB(206, 180, 218),
    // pele (PAL_PELE): so os retratos
    RGB(240, 200, 164), RGB(200, 142, 104),
};
#define PAL_FELTRO 14                    // onde comeca cada familia em PALETA
#define PAL_PELE   50

#define ANTE        5                    // a entrada do comeco; depois sobe (ante_da())
#define FICHAS_INI  100
#define MAX_RODADAS 15
#define MAX_COL     48             // espaco para a colecao (o Desafiante nao tem limite)
#define LIM_COL     24             // dados que um jogador pode ter na partida normal
#define MAO         8              // espaco da mao (compra 7; 8 com a Bolsa Funda)
#define MAO_NORMAL  7              // quantos compra da bolsa por rodada
#define N_FILA      4              // a fila leva de 1 a 4

// Mesa de 640x360: faixa do rival em cima, a sua embaixo, feltro no meio.
#define FAIXA_H    44
#define MESA_X0    14
#define MESA_X1    626
#define MESA_Y0    (FAIXA_H + 8)
#define MESA_Y1    (GFX_H - FAIXA_H - 8)
#define LINHA_CIMA  98
#define LINHA_BAIXO 262
#define CENTRO     180
#define R_DADO     18                       // raio de um dado na mesa
#define SLOT_X(k)  (218 + 68 * (k))
// Dados roubados, a esquerda da fila e um pouco menores: cabem os quatro
// (centros 170, 136, 102, 68) entre a borda da mesa e o primeiro lugar.
#define R_ROUBO    15
#define ROUBO_X(n) (170 - 34 * (n))
#define PLACAR_X   540
#define POTE_X     586
#define TXT_H      FONTE_ALT                // altura de uma linha de texto

// ===========================================================================
// Os dados
// ===========================================================================
enum { EF_NADA, EF_DOBRO, EF_VICIADO, EF_LASTRO, EF_PAR, EF_ESCUDO,
       EF_CARRASCO, EF_EXPLODE, EF_MARTELO, EF_PIRATA, EF_ESPELHO, EF_FICHAS,
       EF_TUDO_NADA, EF_MALDITO, EF_GATUNO, EF_QUEBRADO, EF_INVEJOSO, EF_CARIDOSO,
       EF_FARTURA, EF_SORTE, EF_TEIMOSO, EF_SEMENTE, EF_FOGO, EF_MALDICAO, EF_TREVO,
       EF_JACKPOT, EF_COPIADOR, EF_EGOISTA, EF_LASER, EF_VENTANIA,
       EF_MISERICORDIA, EF_SOLITARIO, EF_SENTINELA, EF_FERREIRO, EF_COBRADOR,
       EF_DESAFIANTE, EF_VIDRO, EF_ACUMULADOR, EF_SEGURANCA,
       EF_BUMERANGUE, EF_GEMEO, EF_PRISMA, EF_FENIX, EF_TRONO,
       EF_CAVALO, EF_BERSERKER, EF_VIUVA };

enum { RAR_COMUM, RAR_INCOMUM, RAR_RARO, RAR_LENDA };

typedef struct {
    const char *nome;
    uint8_t lados, efeito, raridade;
    const char *desc;
} tipo_t;

enum { D_D2, D_D4, D_D6, D_D8, D_D12, D_D20,
       D_DOBRO, D_VICIADO, D_LASTRO, D_PAR, D_ESCUDO,
       D_CARRASCO, D_EXPLOSIVO, D_MARTELO, D_PIRATA, D_ESPELHO, D_FICHAS,
       D_TUDO_NADA, D_MALDITO, D_GATUNO, D_QUEBRADO, D_INVEJOSO, D_CARIDOSO,
       D_FARTURA, D_SORTE,
       // D_MALDITO e o d20 que se chama Agouro na tela; o "Maldito" que a tela
       // mostra e o d4 D_MALDICAO, que tira pontos do rival.
       D_TEIMOSO, D_SEMENTE, D_FOGO, D_MALDICAO, D_TREVO,
       D_JACKPOT, D_COPIADOR, D_EGOISTA, D_LASER, D_VENTANIA,
       // Sempre no fim da lista: os numeros dos tipos vao no salvamento.
       D_MISERICORDIOSO, D_SOLITARIO, D_SENTINELA, D_FERREIRO, D_COBRADOR,
       D_DESAFIANTE, D_VIDRO, D_ACUMULADOR, D_SEGURANCA,
       // Daqui em diante, so no Modo Desafiante, liberados pelas dificuldades.
       D_BUMERANGUE, D_GEMEO, D_PRISMA, D_FENIX, D_TRONO,
       D_CAVALO, D_BERSERKER, D_VIUVA, D_N };
#define D_BASICOS D_BUMERANGUE           // tipos da partida rapida, 2 jogadores e online
static bool so_desafio(int t) { return t >= D_BASICOS; }

// "Fica" = termina a rodada valendo, sem ter sido anulado.
static const tipo_t TIPO[D_N] = {
    { "Moeda",      2,  EF_NADA,      RAR_COMUM,   "1 ou 2: ótima para abrir a sequência" },
    { "D4",         4,  EF_NADA,      RAR_COMUM,   "quatro lados, de 1 a 4" },
    { "D6",         6,  EF_NADA,      RAR_COMUM,   "seis lados, de 1 a 6" },
    { "D8",         8,  EF_NADA,      RAR_COMUM,   "oito lados, de 1 a 8" },
    { "D12",       12,  EF_NADA,      RAR_INCOMUM, "doze lados, de 1 a 12" },
    { "D20",       20,  EF_NADA,      RAR_RARO,    "vinte lados, de 1 a 20" },
    { "Dobro",      6,  EF_DOBRO,     RAR_INCOMUM, "1 a 3: o dado seguinte vale x2" },
    { "Viciado",    6,  EF_VICIADO,   RAR_INCOMUM, "rola 2 vezes e fica com o maior" },
    { "Lastro",    12,  EF_LASTRO,    RAR_RARO,    "sempre de 4 a 12" },
    { "Par",        4,  EF_PAR,       RAR_INCOMUM, "igual ao anterior: +8" },
    { "Escudo",     6,  EF_ESCUDO,    RAR_INCOMUM, "não pode ser anulado nem roubado" },
    { "Carrasco",   4,  EF_CARRASCO,  RAR_RARO,    "4: anula o maior do rival · 1: anula o seu menor" },
    { "Explosivo",  8,  EF_EXPLODE,   RAR_RARO,    "no 8, rola de novo e soma" },
    { "Espinhoso",  4,  EF_MARTELO,   RAR_RARO,    "com 3 ou 4: tira 4 do oponente" },
    { "Pirata",     8,  EF_PIRATA,    RAR_RARO,    "com 6 ou mais: tira 4 do oponente" },
    { "Espelho",    6,  EF_ESPELHO,   RAR_RARO,    "1 ou 2: vira o maior do oponente" },
    { "Fichas",     4,  EF_FICHAS,    RAR_COMUM,   "com mais fichas que o oponente: +2" },
    { "Tudo ou Nada",2,  EF_TUDO_NADA, RAR_LENDA,   "2: tudo vale x2 · 1: rodada vale 0" },
    { "Agouro",    20,  EF_MALDITO,   RAR_LENDA,   "7 ou menos: anula a sua sequência" },
    { "Gatuno",     6,  EF_GATUNO,    RAR_RARO,    "com um 1 na sua sequência: rouba" },
    { "Quebrado",   8,  EF_QUEBRADO,  RAR_COMUM,   "número fixo; quebra ao ser jogado" },
    { "Invejoso",   6,  EF_INVEJOSO,  RAR_RARO,    "com 4+: anula o maior do oponente e o seu" },
    { "Caridoso",   4,  EF_CARIDOSO,  RAR_RARO,    "com 3 ou 4: rouba o menor do oponente" },
    { "Fartura",    4,  EF_FARTURA,   RAR_INCOMUM, "seus dados com 1 ou 2 valem x2" },
    { "Moeda sorte",2,  EF_SORTE,     RAR_INCOMUM, "1: ímpares +1 · 2: pares +1" },
    { "Teimoso",    6,  EF_TEIMOSO,   RAR_INCOMUM,   "não pode ser anulado" },
    { "Semente",    2,  EF_SEMENTE,   RAR_INCOMUM, "seus dados somam 20+: +10" },
    { "Ficha Fogo", 2,  EF_FOGO,      RAR_RARO,    "valendo: +10, e o seu maior queima" },
    { "Maldito",    4,  EF_MALDICAO,  RAR_RARO,    "vale e tira o mesmo do oponente" },
    { "Trevo",      4,  EF_TREVO,     RAR_INCOMUM, "4 vale 8 · 1 a 3 vale 0" },
    { "Jackpot",    4,  EF_JACKPOT,   RAR_INCOMUM, "três dados iguais: +10" },
    { "Copiador",   6,  EF_COPIADOR,  RAR_INCOMUM, "cada dado igual a ele: +2" },
    { "Egoísta",   12,  EF_EGOISTA,   RAR_RARO,    "anula os seus outros dados" },
    { "Laser",       6,  EF_LASER,     RAR_RARO,    "com 5 ou 6: anula um dado do oponente" },
    { "Ventania",    8,  EF_VENTANIA,  RAR_RARO,    "rola de novo o dado de antes dele" },
    { "Misericordioso",4, EF_MISERICORDIA, RAR_INCOMUM, "valendo: +2 por dado seu anulado" },
    { "Solitário",  4,  EF_SOLITARIO, RAR_COMUM,   "sem vizinho valendo: +4" },
    { "Sentinela",  4,  EF_SENTINELA, RAR_COMUM,   "se cair, cai sozinho" },
    { "Ferreiro",   4,  EF_FERREIRO,  RAR_INCOMUM, "não pontua; 3 ou 4: +6 no seguinte" },
    { "Cobrador",   4,  EF_COBRADOR,  RAR_COMUM,   "oponente com mais fichas: +2" },
    { "Desafiante", 8,  EF_DESAFIANTE, RAR_INCOMUM, "maior dado da rodada: +4" },
    { "Vidro",      8,  EF_VIDRO,     RAR_COMUM,   "tirou 1: quebra e anula o anterior" },
    { "Acumulador", 8,  EF_ACUMULADOR, RAR_INCOMUM, "+1 a cada lance, até +6" },
    { "Segurança",  4,  EF_SEGURANCA, RAR_INCOMUM, "anulação ou roubo do oponente cai nele" },
    { "Bumerangue", 6,  EF_BUMERANGUE, RAR_COMUM,  "1 ou 2: volta e rola de novo" },
    { "Gêmeo",      6,  EF_GEMEO,     RAR_INCOMUM, "tira o mesmo do anterior que vale" },
    { "Prisma",     8,  EF_PRISMA,    RAR_INCOMUM, "+1 por tipo de dado seu valendo" },
    { "Fênix",      6,  EF_FENIX,     RAR_RARO,    "anulado: renasce no fim" },
    { "Trono",     12,  EF_TRONO,     RAR_LENDA,   "último que você jogou, valendo: x2" },
    { "Cavalo de Troia", 8, EF_CAVALO, RAR_RARO,  "valendo: +1 por dado seu valendo" },
    { "Berserker", 20,  EF_BERSERKER, RAR_LENDA,   "valendo: anula um dado seu ao acaso" },
    { "Viúva Negra", 8, EF_VIUVA,     RAR_RARO,    "único dado seu valendo: x2" },
};

// Tres niveis de texto por dado, sempre com a mesma regra: o curto (TIPO.desc)
// vai no painel da fila e do jogo; o medio, numa linha, no premio, na loja e na
// escolha do que sai; o completo, na colecao e na tecla I.
static const char *TEXTO_MEDIO[D_N] = {
    [D_D2]        = "Uma moeda: 1 ou 2. Deixa a barra baixa para os próximos.",
    [D_D4]        = "De 1 a 4. Vale pouco, mas é bom para abrir a sequência.",
    [D_D6]        = "O dado comum da casa: de 1 a 6.",
    [D_D8]        = "De 1 a 8. Vale mais, mas levanta a barra da sequência.",
    [D_D12]       = "De 1 a 12. Forte no fim da sequência, arriscado no começo.",
    [D_D20]       = "De 1 a 20. O maior da mesa: guarde para o fim da sequência.",
    [D_DOBRO]     = "Tirando 1 a 3, o dado logo depois dele vale x2 no fim.",
    [D_VICIADO]   = "Rola duas vezes e fica com o maior dos dois resultados.",
    [D_LASTRO]    = "Doze lados, mas só cai de 4 a 12, todos com a mesma chance.",
    [D_PAR]       = "Se tirar o mesmo número do último dado que vale: +8.",
    [D_ESCUDO]    = "Nada o anula e nenhum ataque o alcança, nem roubo.",
    [D_CARRASCO]  = "Valendo com 4, anula o maior dado do oponente. Tirando 1, anula o seu menor.",
    [D_EXPLOSIVO] = "Se tirar 8, rola de novo e soma (até 3 vezes seguidas).",
    [D_MARTELO]   = "Se ficar valendo com 3 ou 4, tira 4 pontos do oponente.",
    [D_PIRATA]    = "Se ficar valendo com 6 ou mais, tira 4 pontos do oponente.",
    [D_ESPELHO]   = "Se tirar 1 ou 2, vira o número do maior dado do oponente.",
    [D_FICHAS]    = "Se ficar valendo e você começou a rodada com mais fichas que o oponente: +2.",
    [D_TUDO_NADA] = "Tirou 2: todos os seus dados valem x2. Tirou 1: sua rodada vale 0.",
    [D_MALDITO]   = "Se tirar 7 ou menos, anula a sua sequência inteira, e ele junto.",
    [D_GATUNO]    = "Se ficar valendo e houver um 1 na sua sequência, rouba o menor dado do oponente.",
    [D_QUEBRADO]  = "Sem sorteio: vale o número gravado nele. Quebra ao ser jogado.",
    [D_INVEJOSO]  = "Valendo com 4 ou mais, anula o maior do oponente e depois o seu maior.",
    [D_CARIDOSO]  = "Se ficar valendo com 3 ou 4, rouba o menor dado do oponente.",
    [D_FARTURA]   = "Se ficar valendo, seus dados que tiraram 1 ou 2 valem x2 no fim.",
    [D_SORTE]     = "Tirou 1: +1 em cada dado ímpar seu. Tirou 2: +1 em cada dado par.",
    [D_TEIMOSO]   = "Nada o anula, nem a queda da sequência. Mas pode ser roubado.",
    [D_SEMENTE]   = "Se ficar valendo e seus dados somarem 20 ou mais no fim: +10.",
    [D_FOGO]      = "Se ficar valendo: +10 pontos, e o seu maior dado queima e sai.",
    [D_MALDICAO]  = "Se ficar valendo, soma para você e tira o mesmo do oponente.",
    [D_TREVO]     = "Tirou 4: SORTE, vale 8. Tirou 1, 2 ou 3: vale 0.",
    [D_JACKPOT]   = "Se ficar valendo e três dados seus mostrarem o mesmo número: +10.",
    [D_COPIADOR]  = "Se ficar valendo, cada outro dado seu com o mesmo número ganha +2.",
    [D_EGOISTA]   = "Anula todos os seus outros dados, menos a si mesmo. Pode ser anulado.",
    [D_LASER]     = "Se ficar valendo com 5 ou 6, anula um dado do oponente, ao acaso.",
    [D_VENTANIA]  = "Ao cair, rola de novo o seu dado lançado logo antes dele.",
    [D_MISERICORDIOSO] = "Se ficar valendo, ganha +2 por dado seu anulado na rodada.",
    [D_SOLITARIO] = "Valendo, sem dado valendo logo antes nem logo depois dele: +4.",
    [D_SENTINELA] = "Se cair na sequência, cai sozinho: o dado de antes continua valendo.",
    [D_FERREIRO]  = "Não pontua. Tirando 3 ou 4, o dado logo depois ganha +6 se os dois valerem.",
    [D_COBRADOR]  = "Se o oponente começou a rodada com mais fichas: +2.",
    [D_DESAFIANTE] = "Se for o maior dado da rodada, dos dois lados: +4 no fim.",
    [D_VIDRO]     = "Se tirar 1, se estilhaça: anula ele e o anterior, e some da bolsa.",
    [D_ACUMULADOR] = "Cada lance soma +1 de bônus a ele, para sempre (até +6).",
    [D_SEGURANCA]  = "Valendo, leva no lugar dos outros a anulação ou o roubo do oponente.",
    [D_BUMERANGUE] = "Se tirar 1 ou 2, volta e rola de novo, uma vez.",
    [D_GEMEO]     = "Tira o mesmo número do último dado seu que vale (sem um antes, rola normal).",
    [D_PRISMA]    = "Se ficar valendo, +1 no fim para cada tipo diferente de dado seu valendo.",
    [D_FENIX]     = "Se terminar anulado, renasce no fim valendo o número que tirou.",
    [D_TRONO]     = "Se for o último dado que você jogou na rodada e ficar valendo: x2.",
    [D_CAVALO]    = "Se ficar valendo, +1 no fim por dado seu valendo, ele incluído.",
    [D_BERSERKER] = "Se ficar valendo, anula no fim um outro dado seu, ao acaso.",
    [D_VIUVA]     = "Se terminar como o único dado seu valendo, vale x2 no fim.",
};

static const char *TEXTO_CAT[D_N] = {
    [D_D2]        = "Uma moeda: sai 1 ou 2. Como primeiro dado da sequência, deixa a barra baixa para os que vêm depois.",
    [D_D4]        = "Quatro lados, de 1 a 4. Vale pouco, mas é bom para abrir a sequência.",
    [D_D6]        = "Seis lados, de 1 a 6. O dado comum da casa.",
    [D_D8]        = "Oito lados, de 1 a 8. Vale mais que o D6, mas levanta a barra para os próximos dados.",
    [D_D12]       = "Doze lados, de 1 a 12. Forte no fim da sequência, arriscado no começo: depois dele, é difícil não cair.",
    [D_D20]       = "Vinte lados, de 1 a 20. O maior da mesa: melhor como último dado da sequência.",
    [D_DOBRO]     = "Seis lados. Se tirar de 1 a 3, no fim da rodada o dado logo depois dele na sequência vale x2, se os dois estiverem valendo. Tirando 4 a 6, não dobra nada. Se esse dado for anulado, o x2 se perde: não passa para o próximo. Multiplicadores se somam: dois x2 no mesmo dado dão x4; três, x6.",
    [D_VICIADO]   = "Seis lados. Rola duas vezes e fica com o maior resultado. O menor aparece na mesa e é descartado.",
    [D_LASTRO]    = "Doze lados, mas só cai de 4 a 12, cada número com a mesma chance (1 em 9).",
    [D_PAR]       = "Quatro lados. Se tirar o mesmo número do último dado que vale na sequência, ganha +8. Igual não cai; menor cai, como qualquer dado.",
    [D_ESCUDO]    = "Seis lados. Nada anula este dado: nem a queda da sequência, nem o Agouro, nem o Egoísta, nem ataques. E ninguém pode roubá-lo.",
    [D_CARRASCO]  = "Quatro lados. Se terminar valendo com 4, anula o maior dado do oponente. Se tirar 1, anula o menor dado da sua própria sequência (fora ele). Escudo e Teimoso resistem à anulação.",
    [D_EXPLOSIVO] = "Oito lados. Tirando 8, rola de novo e soma ao que já tinha. Pode explodir até 3 vezes seguidas.",
    [D_MARTELO]   = "Quatro lados, coberto de espinhos. Se terminar valendo com 3 ou 4, tira 4 pontos do oponente. Com 1 ou 2, é um D4 comum.",
    [D_PIRATA]    = "Oito lados. Se terminar valendo com 6 ou mais, tira 4 pontos do oponente. Os pontos não passam para você.",
    [D_ESPELHO]   = "Seis lados. Se tirar 1 ou 2, vira o número do maior dado do oponente. Se o oponente ainda não jogou, espera valendo e vira no fim da rodada.",
    [D_FICHAS]    = "Quatro lados. Se terminar valendo e você começou a rodada com mais fichas que o oponente, vale +2. É o contrário do Cobrador.",
    [D_TUDO_NADA] = "Uma moeda de risco. Tirando 2, no fim todos os seus dados que valem ganham x2. Tirando 1, a sua rodada inteira vale zero, sem volta.",
    [D_MALDITO]   = "Vinte lados. Tirando 8 ou mais, é um D20 forte. Tirando 7 ou menos, anula a sua sequência inteira, e ele junto. Escudo e Teimoso resistem.",
    [D_GATUNO]    = "Seis lados. Se terminar valendo e algum dado da sua sequência tiver tirado 1, rouba o menor dado do oponente: os pontos dele passam para você. Se for anulado, não rouba.",
    [D_QUEBRADO]  = "Oito lados, mas sem sorteio: vale sempre o número gravado nele. Depois de jogado, quebra e sai da sua bolsa.",
    [D_INVEJOSO]  = "Seis lados. Se terminar valendo com 4 ou mais, anula o maior dado do oponente e depois o maior da sua própria sequência (ele mesmo, se for o maior). Escudo e Teimoso resistem à anulação.",
    [D_CARIDOSO]  = "Quatro lados. Se terminar valendo com 3 ou 4, rouba o menor dado do oponente: os pontos dele passam para você. Só o Escudo não pode ser roubado.",
    [D_FARTURA]   = "Quatro lados. Se terminar valendo, no fim da rodada todos os seus dados que tiraram 1 ou 2 valem x2, ele também.",
    [D_SORTE]     = "Ficha azul e vermelha. Se terminar valendo: tirando 1 (azul), cada dado ímpar seu ganha +1; tirando 2 (vermelha), cada dado par ganha +1. Ela conta a si mesma.",
    [D_TEIMOSO]   = "Seis lados. Nada o anula: nem a queda da sequência, nem o Agouro, nem o Egoísta, nem ataques. Mas pode ser roubado.",
    [D_SEMENTE]   = "Moeda verde. Se terminar valendo e, no fim, os seus dados somarem 20 ou mais (com bônus e roubos), ganha +10.",
    [D_FOGO]      = "Ficha em chamas. Se terminar valendo, dá +10 pontos no fim, e o seu maior dado vira cinza e sai da sua bolsa para sempre.",
    [D_MALDICAO]  = "Quatro lados. Se terminar valendo, soma para você e tira do placar do oponente o mesmo número que tirou.",
    [D_TREVO]     = "Quatro folhas. Tirando 4: SORTE, vale 8. Tirando 1, 2 ou 3, vale zero. Na queda da sequência conta o número que saiu.",
    [D_JACKPOT]   = "Caça-níquel de quatro lados. Se terminar valendo e três dados seus que valem mostrarem o mesmo número (ele pode ser um deles): JACKPOT, +10 no fim.",
    [D_COPIADOR]  = "Seis lados. Se terminar valendo, no fim cada outro dado seu que mostrar o mesmo número ganha +2. Iguais nunca caem na sequência.",
    [D_EGOISTA]   = "Doze lados e uma coroa. Ao cair, anula todos os seus outros dados da rodada, antes e depois dele, menos a si mesmo. Ele pode ser anulado: pela queda da sequência, por ataques, pelo Agouro. Escudo e Teimoso resistem.",
    [D_LASER]     = "Seis lados, com um emissor no topo. Se terminar valendo com 5 ou 6, um raio verde anula um dado do oponente escolhido ao acaso. Escudo e Teimoso resistem à anulação.",
    [D_VENTANIA]  = "Oito lados de vento. Ao cair, rola de novo o seu dado lançado logo antes dele (nunca a si mesmo nem os outros). A sequência é refeita: dados podem cair ou voltar a valer.",
    [D_MISERICORDIOSO] = "Quatro lados. Se terminar valendo, ganha +2 no fim para cada dado seu anulado na rodada.",
    [D_SOLITARIO] = "Quatro lados. Se terminar valendo e os dados vizinhos dele na sequência (o de antes e o de depois) não estiverem valendo, ganha +4 no fim. Na ponta da sequência, só conta o vizinho que existe.",
    [D_SENTINELA] = "Quatro lados. Se tirar menos que o último dado que vale, só ele cai: o dado de antes continua valendo.",
    [D_FERREIRO] = "Quatro lados. Não pontua, mas conta na queda da sequência com o número que tirou. Se tirar 3 ou 4, o dado logo depois dele ganha +6 no fim, se os dois terminarem valendo.",
    [D_COBRADOR] = "Quatro lados. Vale +2 se o oponente começou a rodada com mais fichas que você.",
    [D_DESAFIANTE] = "Oito lados. Se terminar valendo e for maior que todos os outros dados que valem na rodada, dos dois jogadores, ganha +4 no fim. Empate não conta.",
    [D_VIDRO] = "Oito lados. Tirando 1, se estilhaça: é anulado, anula também o último dado seu que valia (como na queda) e sai da sua bolsa para sempre.",
    [D_ACUMULADOR] = "Oito lados. Cada vez que é lançado, guarda +1 de bônus, até +6. O bônus fica com ele de rodada em rodada e soma ao número que tirou, se ele terminar valendo.",
    [D_SEGURANCA]  = "Quatro lados. Se o oponente anular ou roubar um dado seu (Carrasco, Laser, Invejoso, Caridoso, Gatuno) e o Segurança estiver valendo, quem é anulado ou roubado é o Segurança, e o outro dado fica.",
    [D_BUMERANGUE] = "Seis lados. Se tirar 1 ou 2, volta e rola de novo uma vez; fica o segundo número.",
    [D_GEMEO]     = "Seis lados. Tira sempre o mesmo número do último dado seu que vale, e por isso nunca cai. Sem um dado valendo antes dele, rola normal.",
    [D_PRISMA]    = "Oito lados. Se terminar valendo, ganha +1 no fim para cada tipo diferente de dado seu que vale na rodada, ele incluído.",
    [D_FENIX]     = "Seis lados. Se terminar anulado (pela queda, por ataque, pelo Agouro ou pelo Egoísta), renasce no fim e vale o número que tirou. Roubado, não volta.",
    [D_TRONO]     = "Doze lados. Se for o último dado que você jogou na rodada e terminar valendo, vale x2 no fim.",
    [D_CAVALO]    = "Oito lados. Se terminar valendo, ganha +1 no fim para cada dado seu que vale na rodada, ele incluído.",
    [D_BERSERKER] = "Vinte lados. Se terminar valendo, no fim anula um outro dado seu que vale, escolhido ao acaso. Escudo e Teimoso resistem.",
    [D_VIUVA]     = "Oito lados. Se terminar a rodada como o único dado seu que vale, vale x2 no fim. Soma com outros multiplicadores.",
};

static const char *RARIDADE[] = { "comum", "incomum", "raro", "lenda" };
// Cor de cada raridade: cinza, verde, azul e dourado.
static const uint16_t COR_RAR[] = { RGB(168, 164, 150), RGB(104, 186, 110),
                                    RGB(96, 146, 226), RGB(236, 170, 64) };
static uint16_t cor_rar(int tipo) { return COR_RAR[TIPO[tipo].raridade]; }

// "moeda", "D4" ... "D20": o tipo, sempre escrito do mesmo jeito.
static const char *nome_tipo(int tipo)
{
    switch (TIPO[tipo].lados) {
    case 2:  return "moeda";
    case 4:  return "D4";
    case 6:  return "D6";
    case 8:  return "D8";
    case 12: return "D12";
    default: return "D20";
    }
}

static bool igual_sem_caixa(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if ((*a | 32) != (*b | 32)) return false;
    return *a == *b;
}

// "D8 · raro", na cor da raridade.
static void classe(int tipo, char *s, int n)
{
    // O dado comum ja se chama pelo tipo ("D6"): a etiqueta diz so a raridade.
    if (igual_sem_caixa(TIPO[tipo].nome, nome_tipo(tipo))) snprintf(s, (size_t)n, "%s", RARIDADE[TIPO[tipo].raridade]);
    else snprintf(s, (size_t)n, "%s · %s", nome_tipo(tipo), RARIDADE[TIPO[tipo].raridade]);
}

// O catalogo mostra os dados do mais comum ao mais raro.
static int cat_tipo(int i)
{
    for (int r = RAR_COMUM; r <= RAR_LENDA; r++)
        for (int t = 0; t < D_N; t++)
            if (TIPO[t].raridade == r && i-- == 0) return t;
    return 0;
}

static const uint8_t VITRINE[] = { D_D8, D_D12, D_PAR, D_ESCUDO,
                                   D_DOBRO, D_VICIADO, D_LASTRO, D_QUEBRADO,
                                   D_FARTURA, D_SORTE, D_TEIMOSO, D_SEMENTE, D_TREVO,
                                   D_JACKPOT, D_COPIADOR, D_MISERICORDIOSO, D_SOLITARIO,
                                   D_SENTINELA, D_FERREIRO, D_COBRADOR, D_DESAFIANTE,
                                   D_VIDRO, D_ACUMULADOR, D_SEGURANCA };
// Precos medidos com partidas IA x IA (2000 por dado, com duas copias na
// bolsa): quem rende mais custa mais.
static int preco_loja(int t)
{
    switch (t) {
    case D_D2:       return 4;
    case D_D4:       return 5;
    case D_D8:       return 9;
    case D_QUEBRADO: return 9;
    case D_SENTINELA: return 6;
    case D_COBRADOR: return 6;
    case D_VIDRO:    return 7;
    case D_SEGURANCA: return 7;
    case D_TREVO:    return 8;
    case D_SEMENTE:  return 9;
    case D_JACKPOT:  return 9;
    case D_SOLITARIO: return 9;
    case D_PAR:      return 11;
    case D_COPIADOR: return 11;
    case D_TEIMOSO:  return 11;
    case D_FERREIRO: return 10;
    case D_DESAFIANTE: return 13;
    case D_MISERICORDIOSO: return 13;
    case D_SORTE:    return 14;
    case D_FARTURA:  return 14;
    case D_FICHAS:   return 6;
    case D_TUDO_NADA: return 14;
    case D_EGOISTA:  return 14;
    case D_ESCUDO:   return 15;
    case D_MARTELO: return 15;
    case D_DOBRO:    return 16;
    case D_VICIADO:  return 16;
    case D_ACUMULADOR: return 16;
    case D_EXPLOSIVO: return 16;
    case D_LASER:    return 16;
    case D_CARRASCO: return 17;
    case D_D12:      return 20;
    case D_GATUNO:   return 20;
    case D_ESPELHO:  return 20;
    case D_VENTANIA: return 10;
    case D_FOGO:     return 22;
    case D_MALDITO:  return 24;
    case D_D20:      return 26;
    case D_LASTRO:   return 26;
    default:         return 18;
    }
}

static uint16_t cor_aro(int tipo)
{
    switch (TIPO[tipo].efeito) {
    case EF_NADA:      return C_LATAO;
    case EF_CARRASCO: case EF_MARTELO: case EF_PIRATA: case EF_GATUNO:
    case EF_INVEJOSO: case EF_CARIDOSO:
        return C_VINHO;
    case EF_TUDO_NADA: case EF_MALDITO: return C_VIOLETA;
    case EF_ESCUDO:    return C_ACO;
    case EF_FICHAS:    return C_OURO;
    case EF_QUEBRADO:  return C_TERRA;
    case EF_SORTE:     return C_OURO;
    case EF_TEIMOSO:   return C_ACO;
    case EF_SEMENTE:   return C_VERDE;
    case EF_FOGO:      return C_FOGO;
    case EF_MALDICAO:  return C_VINHO;
    case EF_TREVO:     return C_TREVO;
    case EF_JACKPOT:   return C_OURO;
    case EF_VICIADO:   return C_VERMELHO;
    case EF_COPIADOR:  return C_CIANO;
    case EF_EGOISTA:   return C_EGO;
    case EF_LASER:     return C_LASER;
    case EF_VENTANIA:  return C_VENTO;
    case EF_MISERICORDIA: return C_HALO;
    case EF_SOLITARIO: return C_NOITE;
    case EF_SENTINELA: return C_BRONZE;
    case EF_FERREIRO:  return C_FERRO;
    case EF_COBRADOR:  return C_COBRE;
    case EF_DESAFIANTE: return C_CARMIM;
    case EF_VIDRO:     return C_VIDRO;
    case EF_ACUMULADOR: return C_AMBAR;
    case EF_SEGURANCA: return RGB(90, 120, 170);
    case EF_BUMERANGUE: return C_MADEIRA_CLR;
    case EF_GEMEO:     return C_CIANO;
    case EF_PRISMA:    return RGB(176, 132, 220);
    case EF_FENIX:     return C_FOGO;
    case EF_TRONO:     return C_OURO;
    case EF_CAVALO:    return C_CARVALHO;
    case EF_BERSERKER: return C_SANGUE;
    case EF_VIUVA:     return C_TEIA;
    default:           return C_AGUA;
    }
}

// ===========================================================================
// Estado
// ===========================================================================
// Uma peca da bolsa: o tipo, e para o Quebrado o numero gravado nele.
typedef struct { uint8_t tipo, fixo; } peca_t;

// Cada dado do jogador esta num de tres lugares.
enum { NA_BOLSA, NA_MAO, FORA };

typedef struct {
    const char *nome;
    uint16_t cor;
    int fichas;
    peca_t col[MAX_COL];         // todos os dados que ele tem
    uint8_t onde[MAX_COL];
    int n_col;
    int8_t mao[MAO];             // a mao desta rodada: indices em col
    int n_mao;
    bool embaralhou;             // a bolsa esvaziou e tudo voltou nesta compra
} jogador_t;

enum { V_ESPERA, V_VALIDO, V_ANULADO, V_ROUBADO };

typedef struct {
    int8_t  idx[N_FILA];         // posicoes em col, na ordem escolhida
    uint8_t tipo[N_FILA], fixo[N_FILA];
    int     n;
    int     lancados;
    int     cru[N_FILA];
    int     valor[N_FILA];
    uint8_t est[N_FILA];
    uint8_t dobra[N_FILA];       // so para a etiqueta: o x2 vale no fim
    int8_t  bonus[N_FILA];
    bool    quebra[N_FILA];
    bool    cinza[N_FILA];           // queimado pela Ficha de Fogo
    uint8_t descarte[N_FILA];        // Viciado: o menor dos dois lances
    bool    espera[N_FILA];          // Espelho que so vira no fim (o rival nao tinha jogado)
    char    tag[N_FILA][14];
    bool    zerada;                  // Tudo ou Nada tirou 1: a rodada vale 0
    int     pontos;
    int     total;
} fila_t;

typedef struct {
    float x, y, vx, vy, giro, spin;
    float ox, oy;
    uint8_t dono, slot, tipo;
    int valor, face;
    float t_face, pouso;
    bool parado;
    bool fantasma;                   // o segundo lance do Viciado, que e descartado
} voo_t;

// Eventos do desfecho, tocados um a um.
enum { EV_ANULA, EV_ROUBA, EV_TIRA, EV_PIRATA, EV_BONUS, EV_MULT, EV_FOGO, EV_SEMENTE,
       EV_ZERA, EV_JACKPOT, EV_ESPELHO,
       EV_EXTRA,                     // bonus fixo do fim (Misericordioso, Solitario, Desafiante)
       EV_RENASCE,                   // a Fenix volta a valer
       EV_CHEFE };                   // a virada do Barao: pontos por queda sua
typedef struct {
    uint8_t tipo, de_j, de_k, alvo_j, alvo_k;
    int valor;
    uint8_t mascara;                 // dados da fila atingidos (BONUS, MULT)
    int8_t  por_dado[N_FILA];        // bonus ou multiplicador de cada um
    char txt[48];
} evento_t;
#define MAX_EV 32

typedef struct { uint8_t tipo, fixo, de_j; int valor; } roubado_t;

enum { F_ORDEM, F_APOSTA, F_ROLANDO, F_ARRUMA, F_RESULTADO, F_PREMIO, F_FIM,
       F_LOJA, F_ABERTURA, F_JOGA, F_VEREDITO, F_EFEITOS, F_CATALOGO,
       F_ONLINE, F_TUTORIAL,
       F_MAPA, F_RLOJA, F_RBOLSA, F_RFIM, F_RAMULETO,       // Modo Desafiante
       F_PERFIS, F_PCRIA, F_RDIFIC, F_REVENTO };

static jogador_t J[2];
static fila_t F[2];
static voo_t voo[2];
static int  n_voo;
static int  fase, vez, primeiro, rodada, pote, cursor, partidas;
static int  a_pagar, passes;
static bool aumentou;
static float t_fase, t_som;
static int  venc_mao, venc_partida;
static int  correu_venc = -1;        // quem levou o pote porque o outro correu
static peca_t oferta[3];
static char aviso[64], veredito[64], nota[48];
static int  ver_tipo;
static int  anulou_a, anulou_b;
static int  fichas_ini[2];           // fichas de cada um quando a rodada comecou (Cobrador)
static int  resistiu;                // dado que segurou a queda (treme)
static uint8_t anulou_masc;          // dados que o Egoista acabou de anular
static uint32_t semente;

static evento_t ev[MAX_EV];
static int  n_ev, ev_i;
static uint8_t vis_est[2][N_FILA];
static roubado_t vis_roubo[2][4];
static int  vis_nroubo[2], vis_delta[2];
static int8_t vis_bonus[2][N_FILA], vis_mult[2][N_FILA];
static uint8_t vis_queima[2][N_FILA];
static uint8_t vis_espelho[2][N_FILA];
static uint8_t est_pre_espelho[2][N_FILA];   // a sequencia antes do Espelho virar (para a animacao)   // Espelho que ja virou na conta, mas cuja vez de mostrar nao chegou
static int  vis_semente[2];
static uint8_t vis_zerada[2];

#define N_LOJA 5
static peca_t loja[N_LOJA];
static bool levou[2][N_LOJA];
static int  loja_primeiro;
static bool abertura_pronta;
static int  menu_cur;                // opcao do menu de abertura
// Qual lista o menu mostra: a principal, "1 jogador", o nivel da partida
// rapida ou a escolha da run do Modo Desafiante.
enum { MT_PRINCIPAL, MT_UM, MT_NIVEL, MT_DESAFIO };
static int  menu_tela;
static float sacode;                 // 1 quando o cursor do menu anda: os dados chacoalham
enum { MN_UM, MN_DOIS, MN_ONLINE, MN_DADOS, MN_TUTORIAL, MN_N };

// Modos: contra o computador, dois no mesmo teclado, ou online.
enum { M_UM, M_DOIS, M_ONLINE, M_DESAFIO };
static int  modo;
// Quantos dados cabem na colecao: 24 na partida normal; sem limite pratico
// no Modo Desafiante.
static int lim_col(void) { return modo == M_DESAFIO ? MAX_COL : LIM_COL; }

// Amuletos: so no Modo Desafiante, so de quem joga (o oponente nunca tem).
// A run guarda quais tem; a partida copia para amul[0].
enum { A_LUVA, A_REDE, A_BOLSA, A_OURO, A_PENA, A_CONTRATO, A_IMA, A_COURACA,
       A_COFRE, A_RESERVA, A_POTE, A_FURADO,
       // Liberados pelas dificuldades (dois por nivel).
       A_FERRADURA, A_PESADO, A_RELOGIO, A_CARTOLA, A_COROA, A_CUPOM,
       A_PRENSA, A_TACA, A_CORINGA, A_DUPLA, A_SELO, A_PEDAGIO, A_N };
#define A_BASICOS A_FERRADURA
static uint32_t amul[2];
static uint32_t am_disparou;         // amuletos que acabaram de agir (a faixa acende)
static int8_t ferr_k[2] = { -1, -1 };  // dado que a Ferradura salvou nesta rodada
static int des_rod_vencidas;         // rodadas que voce venceu na partida (Taca)
static float am_brilho[A_N];
static bool tem_am(int j, int a) { return modo == M_DESAFIO && (amul[j & 1] >> a & 1); }
// Trunfos dos oponentes do Desafiante que nao sao amuletos (o resto deles
// usa amul[1]): Nervoso (Novato), Batedor de carteira (Ladrao), A casa
// sempre ganha (Crupie).
enum { REG_NADA, REG_NERVOSO, REG_BATEDOR, REG_CASA, REG_ULTIMA, REG_FOLEGO, REG_SAQUE };
static uint8_t reg_op;
static bool op_nervoso;              // o Novato perdeu a ultima rodada
static bool tem_reg(int r) { return modo == M_DESAFIO && reg_op == r; }
// Falas do oponente (desafio.inc): o que acabou de acontecer.
enum { FA_INICIO, FA_CAI, FA_GANHA_R, FA_PERDE_R, FA_GANHA_P, FA_PERDE_P, FA_N };
static void fala_evento(int cat);
static char  fala_txt[64];
static float t_fala;                     // > 0: o balao do oponente esta na mesa
// Velocidade: no rapido, animacoes e a vez do computador andam em dobro
// (menos no online, onde os dois lados precisam andar juntos). Fica guardada.
#define VEL_RAPIDO 2.0f
static bool rapido, rapido_lido;
static float t_aviso_vel;                // > 0: o aviso da velocidade esta na mesa
// Elevador do Desafiante: ao comecar a run, um mostrador sobe ate o andar
// da dificuldade (> 0: quanto falta da animacao).
#define ELEV_T     1.9f
#define ELEV_CHEGA 1.25f                 // quando o ponteiro chega
static float t_elev;
static int   fala_perso = -1;            // o oponente da partida do Desafiante
// A virada dos chefes: da 5a rodada em diante, o chefe muda o jogo. Cada um
// tem a sua, que aparece num andar e fica mais forte num andar acima:
//   Crupie (1o andar): +1 em cada dado dele; do 4o, voce compra 1 a menos.
//   Dona do Salao (2o): o aluguel, 3 fichas suas no fim de cada rodada; 5 do 4o.
//   Barao (3o): +3 para ele por dado seu anulado e x2 no primeiro dado dele;
//   na Cobertura, x2 tambem no segundo.
#define PERSO_CHEFE0  8                  // os chefes, na lista dos oponentes
#ifndef VIRADA_BARAO
#define VIRADA_BARAO  3                  // pontos do Barao virado por dado seu anulado
#endif
#ifndef VIRADA_RODADA
#define VIRADA_RODADA 5                  // (os testes de balanco desligam com 99)
#endif
static int   perso_virado = -1;          // o chefe que ja virou nesta partida
static float t_virada;                   // > 0: o anuncio da virada na tela
static int des_dificuldade(void);
// A virada que o oponente perso traz na dificuldade da run: 0 nenhuma, 1, 2.
static int virada_de(int perso)
{
    int c = perso - PERSO_CHEFE0;
    if (c < 0 || c > 2) return 0;
    static const int8_t ABRE[3] = { 1, 2, 3 }, FORTE[3] = { 4, 4, 5 };
    int d = des_dificuldade();
    return d >= FORTE[c] ? 2 : d >= ABRE[c] ? 1 : 0;
}
// Com que forca o chefe c (0 Crupie, 1 Dona, 2 Barao) esta virado agora.
static int virada_forca(int c)
{
    if (modo != M_DESAFIO || fala_perso != PERSO_CHEFE0 + c || rodada < VIRADA_RODADA) return 0;
    return virada_de(PERSO_CHEFE0 + c);
}
static void am_acende(int a) { am_brilho[a] = 1.2f; }
// Quantos dados cabem na sequencia de j (Contrato Sujo: 3).
static int max_fila(int j) { return tem_am(j, A_CONTRATO) ? 3 : N_FILA; }

// Modo Desafiante: o resto mora em desafio.inc, incluido mais abaixo.
#define DES_RODADAS 8                // rodadas de cada partida da run
#define DES_EXTRAS  3                // rodadas extras se terminar empatado

// A entrada sobe ao longo da partida, para que abrir vantagem e so correr de
// tudo nao baste: na partida de 15 rodadas, 5 fichas da 1a a 5a, 10 da 6a a
// 10a e 15 da 11a em diante; no Desafiante (8 rodadas), 5 da 1a a 4a, 10 da
// 5a a 7a e 15 da 8a em diante (as extras tambem).
static int ante_da(int r)
{
#ifdef ENTRADA_FIXA
    (void)r; return ANTE;                // (os testes de balanco comparam com a antiga)
#else
    if (modo == M_DESAFIO) return r <= 4 ? ANTE : r <= 7 ? 2 * ANTE : 3 * ANTE;
    return r <= 5 ? ANTE : r <= 10 ? 2 * ANTE : 3 * ANTE;
#endif
}
static float t_aviso_entrada;            // > 0: o aviso de que a entrada subiu
#define ENTRADA_AVISO_T 2.6f
static void des_resultado(void);
static void des_premio_feito(void);
static int  des_foco(void);
static int  remoto;                  // bit j: jogador j joga do outro computador
static uint32_t semente_online;
#define FILA_REDE 256
static uint16_t rede_q[FILA_REDE];   // teclas do rival ainda nao aplicadas
static int  rede_ini, rede_fim;
static bool digitando;               // lobby online: digitando o codigo da sala
static char codigo_dig[8];
static int  online_cur;
static float t_sair = -1;            // ESC apertado uma vez: confirma com outro
static int  tut_pag;                 // pagina do tutorial
static int  cpu;                     // bit j: jogador j e do aparelho
static float t_ia;                   // espera da IA antes de cada gesto
static int  ia_fase = -1, ia_vez = -1;
static int8_t plano[N_FILA];         // a fila que a IA decidiu montar (posicoes na mao)
static int  plano_n, plano_rodada = -1, plano_vez = -1;
static float plano_p;                // chance de vencer com o plano
static int  detalhe = -1, detalhe_fase;   // dado aberto pela tecla I (-1: nenhum)
static int  cat_i, cat_a;         // dado e amuleto em exibicao na colecao
static int  cat_sec;              // secao da colecao: 0 dados, 1 amuletos
static bool removendo;               // premio trocado por tirar um dado
static float t_tira;                 // animacao do dado saindo (0 = parado)
static int  tira_i;
// Roleta do premio: os tres rolos giram e travam um a um no dado que ja
// foi sorteado. t_slot: tempo desde que abriu (-1: sem roleta).
static float t_slot = -1, slot_t0, slot_dt;
typedef struct { float x, y, vx, vy, giro, vg; uint16_t cor; uint8_t w, h; } confete_t;
#define MAX_CONFETE 140
static confete_t confete[MAX_CONFETE];
static int n_confete;

// ===========================================================================
// Utilidades
// ===========================================================================
static uint32_t rnd(void)
{
    semente ^= semente << 13;
    semente ^= semente >> 17;
    semente ^= semente << 5;
    return semente;
}
static int sorte(int n) { return n > 0 ? (int)(rnd() % (uint32_t)n) : 0; }
// Sorteio so do que se ve (faces piscando, trajetoria do dado): separado do
// sorteio das regras, que no modo online precisa andar igual nos dois lados.
static uint32_t semente_v = 88172645u;
static uint32_t rnd_v(void)
{
    semente_v ^= semente_v << 13; semente_v ^= semente_v >> 17; semente_v ^= semente_v << 5;
    return semente_v;
}
static float frnd(float a, float b) { return a + (b - a) * (float)(rnd_v() % 10000) / 10000.0f; }
static int rola_v(int lados) { return 1 + (int)(rnd_v() % (uint32_t)lados); }
// Face que pisca enquanto o dado rola: so as que ele pode mostrar.
static int face_v(int tipo, int valor)
{
    if (tipo == D_QUEBRADO) return valor;
    if (tipo == D_LASTRO)   return 3 + rola_v(9);
    return rola_v(TIPO[tipo].lados);
}
static int outro(int j) { return j ^ 1; }
static int rola(int lados) { return 1 + sorte(lados); }
static int baixo;                        // quem aparece embaixo: o jogador local
static int zona(int j) { return j == baixo ? LINHA_BAIXO : LINHA_CIMA; }
static int tipo_de(int j, int k) { return F[j].tipo[k]; }

static peca_t nova_peca(int tipo)
{
    peca_t p = { (uint8_t)tipo, 0 };
    if (tipo == D_QUEBRADO) p.fixo = (uint8_t)rola(8);
    return p;
}

// Texto centrado em cx; y e o topo da linha.
static void txt_c(int cx, int y, const char *s, uint16_t c, bool bold)
{
    gfx_texto(cx - gfx_largura(s, 1) / 2, y, s, c, 1, bold);
}

static void txt_c2(int cx, int y, const char *s, uint16_t c)
{
    gfx_texto(cx - gfx_largura(s, 2) / 2, y, s, c, 2, true);
}

static void txt(int x, int y, const char *s, uint16_t c, bool bold)
{
    gfx_texto(x, y, s, c, 1, bold);
}

// Texto com sombra de um pixel: legivel sobre o feltro trabalhado.
static void txt_sombra_c(int cx, int y, const char *s, uint16_t c, int esc)
{
    int x = cx - gfx_largura(s, esc) / 2;
    gfx_texto(x + esc, y + esc, s, RGB(8, 26, 19), esc, true);
    gfx_texto(x, y, s, c, esc, true);
}

// ===========================================================================
// Kit de interface: as mesmas tres pecas em todo o jogo.
//   ui_painel  placa de ebano com filete de latao (avisos sobre a mesa)
//   ui_cartao  cartao de feltro escuro (loja, premio, perfis, listas)
//   ui_botao   botao com espessura: face, brilho em cima, borda de baixo
// Cantos aparados de um pixel; contorno quase preto; sombra dura embaixo.
// ===========================================================================
#define C_PLACA     RGB(24, 22, 20)              // fundo das placas
#define C_PLACA_LUZ RGB(36, 32, 29)

// Contorno de um pixel com os quatro cantos aparados.
static void ui_contorno(int x, int y, int w, int h, uint16_t c)
{
    gfx_rect(x + 1, y, w - 2, 1, c);
    gfx_rect(x + 1, y + h - 1, w - 2, 1, c);
    gfx_rect(x, y + 1, 1, h - 2, c);
    gfx_rect(x + w - 1, y + 1, 1, h - 2, c);
}

static void ui_painel(int x, int y, int w, int h)
{
    gfx_rect(x + 2, y + 3, w, h, C_SOMBRA);                  // sombra dura
    gfx_rect(x + 1, y + 1, w - 2, h - 2, C_PLACA);
    gfx_rect(x + 2, y + 1, w - 4, 1, C_PLACA_LUZ);           // luz na beira de cima
    ui_contorno(x, y, w, h, C_BARRA);
    ui_contorno(x + 2, y + 2, w - 4, h - 4, C_LATAO_ESC);    // filete de latao
}

// 'cor' (0: ouro) e o filete do cartao em foco; fora de foco ele fica apagado.
static void ui_cartao(int x, int y, int w, int h, bool foco, uint16_t cor)
{
    if (!cor) cor = C_OURO;
    gfx_rect(x + 2, y + 3, w, h, C_SOMBRA);
    gfx_rect(x + 1, y + 1, w - 2, h - 2, foco ? C_FELTRO : C_FELTRO_ESC);
    ui_contorno(x, y, w, h, C_BARRA);
    ui_contorno(x + 1, y + 1, w - 2, h - 2, foco ? cor : gfx_mistura(cor, C_FELTRO_ESC, 55));
    if (foco) ui_contorno(x + 2, y + 2, w - 4, h - 4, gfx_mistura(cor, C_BARRA, 45));
}

// Selecao de uma peca numa grade: fundo levemente aceso e contorno na cor.
static void ui_selecao(int x, int y, int w, int h, uint16_t cor)
{
    gfx_rect(x + 1, y + 1, w - 2, h - 2, gfx_mistura(C_FELTRO, cor, 22));
    ui_contorno(x, y, w, h, cor);
}

// Etiqueta pequena (raridade, protecao): fundo escuro tingido e contorno na cor.
static void ui_etiqueta(int x, int y, int w, int h, uint16_t cor)
{
    gfx_rect(x + 1, y + 1, w - 2, h - 2, gfx_mistura(C_PLACA, cor, 22));
    ui_contorno(x, y, w, h, cor);
}

// Botao: estado 0 normal, 1 em foco, 2 desligado. 'tinta' e a cor da face
// (0: ebano). O rotulo vai centrado na face.
#define UI_FOCO 1
#define UI_DESLIGADO 2
static void ui_botao(int x, int y, int w, int h, const char *rot, uint16_t tinta, int estado)
{
    uint16_t face = estado == UI_DESLIGADO ? C_EBANO : tinta ? tinta : C_EBANO_B;
    if (estado == UI_FOCO) face = gfx_na_paleta(gfx_mistura(face, C_BRANCO, 18));
    uint16_t borda = gfx_mistura(face, C_BARRA, 45), luz = gfx_mistura(face, C_BRANCO, 30);
    gfx_rect(x + 1, y + 2, w - 2, h - 1, C_SOMBRA);                 // sombra no feltro
    gfx_rect(x + 1, y + 1, w - 2, h - 2, borda);                    // a espessura
    gfx_rect(x + 1, y + 1, w - 2, h - 4, face);                     // a face
    if (estado != UI_DESLIGADO) gfx_rect(x + 2, y + 1, w - 4, 1, luz);
    ui_contorno(x, y, w, h, estado == UI_FOCO ? C_OURO : C_BARRA);
    uint16_t c = estado == UI_DESLIGADO ? C_EBANO_B : estado == UI_FOCO ? C_BRANCO : C_MARFIM;
    // Face clara (latao, ouro): letra escura.
    int lum = ((face >> 11) & 31) * 2 + ((face >> 5) & 63) + (face & 31);
    if (lum > 85 && estado != UI_DESLIGADO) c = C_BARRA;
    int tw = gfx_largura(rot, 1);
    gfx_texto(x + (w - tw) / 2, y + (h - 4 - TXT_H) / 2 + 1, rot, c, 1, false);
}

// ===========================================================================
// Desenho de dados: poligono por numero de lados
// ===========================================================================
static void poligono(const float *vx, const float *vy, int n, uint16_t cor)
{
    float ymin = vy[0], ymax = vy[0];
    for (int i = 1; i < n; i++) {
        if (vy[i] < ymin) ymin = vy[i];
        if (vy[i] > ymax) ymax = vy[i];
    }
    for (int y = (int)ceilf(ymin); y <= (int)floorf(ymax); y++) {
        float xs[16];
        int nx = 0;
        for (int i = 0; i < n && nx < 16; i++) {
            int j = (i + 1) % n;
            float ya = vy[i], yb = vy[j];
            if ((ya <= y && yb > y) || (yb <= y && ya > y))
                xs[nx++] = vx[i] + (y - ya) / (yb - ya) * (vx[j] - vx[i]);
        }
        for (int a = 1; a < nx; a++)
            for (int b = a; b > 0 && xs[b] < xs[b - 1]; b--) {
                float t = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = t;
            }
        for (int k = 0; k + 1 < nx; k += 2) {
            int x0 = (int)ceilf(xs[k]), x1 = (int)floorf(xs[k + 1]);
            if (x1 >= x0) gfx_rect(x0, y, x1 - x0 + 1, 1, cor);
        }
    }
}

// ---------------------------------------------------------------------------
// Texturas: um padrao discreto no corpo de cada dado, que gira com ele
// ---------------------------------------------------------------------------
enum { T_LISO, T_GRAO, T_LINHAS, T_METAL, T_ONDAS, T_MARMORE, T_PONTOS, T_MADEIRA, T_TIJOLO };

static int textura_de(int tipo)
{
    switch (tipo) {
    case D_LASTRO: case D_ESCUDO: case D_TEIMOSO: case D_LASER: return T_METAL;
    case D_VENTANIA:                                       return T_ONDAS;
    case D_DOBRO: case D_CARRASCO: case D_GATUNO: case D_JACKPOT: return T_LINHAS;
    case D_PAR: case D_FICHAS: case D_INVEJOSO: case D_TREVO: return T_ONDAS;
    case D_ESPELHO: case D_MALDICAO: case D_EGOISTA:       return T_MARMORE;
    case D_EXPLOSIVO: case D_MARTELO: case D_CARIDOSO:
    case D_COPIADOR: case D_VICIADO:                       return T_PONTOS;
    case D_PIRATA:                                         return T_MADEIRA;
    case D_SENTINELA:                                      return T_TIJOLO;
    case D_FERREIRO:                                       return T_METAL;
    case D_MISERICORDIOSO:                                 return T_MARMORE;
    case D_DESAFIANTE:                                     return T_LINHAS;
    case D_ACUMULADOR:                                     return T_PONTOS;
    case D_SEGURANCA:                                      return T_METAL;
    case D_VIDRO: case D_SOLITARIO:                        return T_LISO;
    case D_BUMERANGUE:                                     return T_MADEIRA;
    case D_GEMEO: case D_PRISMA:                           return T_LISO;
    case D_FENIX:                                          return T_ONDAS;
    case D_TRONO:                                          return T_MARMORE;
    case D_CAVALO:                                         return T_MADEIRA;
    case D_BERSERKER:                                      return T_GRAO;
    case D_VIUVA:                                          return T_LISO;   // a teia e o desenho
    case D_MALDITO:                                        return T_LISO;   // ja e manchado
    default:                                               return T_GRAO;   // marfim com grao
    }
}

static uint32_t hash2(int x, int y)
{
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

// -1 escurece, +1 clareia, 0 deixa: o desenho do padrao no ponto (u, v) do
// dado, em pixels, com s = escala (1 num dado da mesa).
static int padrao(int tex, float u, float v, float s)
{
    switch (tex) {
    case T_GRAO: {
        uint32_t h = hash2((int)floorf(u / s), (int)floorf(v / s));
        return h % 19 == 0 ? -1 : (h % 29 == 0 ? 1 : 0);
    }
    case T_LINHAS:  return ((int)floorf((u + v) / (3 * s)) & 3) == 0 ? -1 : 0;
    case T_METAL:   return ((int)floorf(v / (2 * s)) & 1) ? 1 : 0;
    case T_ONDAS:   return sinf(v / (2.4f * s) + sinf(u / (5 * s)) * 1.8f) > 0.8f ? 1 : 0;
    case T_MARMORE: return fabsf(sinf(u / (6 * s) + 2.2f * sinf(v / (7 * s)))) < 0.1f ? -1 : 0;
    case T_PONTOS: {
        float a = fmodf(u / s + 100, 5), b = fmodf(v / s + 100, 5);
        return a < 1.2f && b < 1.2f ? -1 : 0;
    }
    case T_MADEIRA: return sinf((u * 0.25f + sinf(v * 0.12f / s) * 2.5f) / s) > 0.7f ? -1 : 0;
    case T_TIJOLO: {                              // pedras de muralha: fiadas desencontradas
        float a = u / s + 100, b = v / s + 100;
        int fiada = (int)floorf(b / 6);
        float desloc = (fiada & 1) * 5.5f;
        if (fmodf(b, 6) < 1.1f) return -1;          // argamassa entre as fiadas
        if (fmodf(a + desloc, 11) < 1.1f) return -1;   // juntas entre as pedras
        return hash2((int)floorf((a + desloc) / 11), fiada) % 3 == 0 ? 1 : 0;
    }
    default:        return 0;
    }
}

// Poligono como o poligono(), mas com o padrao do dado por cima da cor base.
static void poligono_tex(const float *vx, const float *vy, int n, uint16_t cor, int tex,
                         float cx, float cy, float ang, float r, int dono)
{
    if (tex == T_LISO || r < 12) { poligono(vx, vy, n, cor); return; }
    uint16_t esc = gfx_mistura(cor, C_SOMBRA, dono == 0 ? 12 : 18);
    uint16_t cla = gfx_mistura(cor, C_BRANCO, dono == 0 ? 14 : 12);
    float ca = cosf(-ang), sa = sinf(-ang), s = r / 22;
    float ymin = vy[0], ymax = vy[0];
    for (int i = 1; i < n; i++) { if (vy[i] < ymin) ymin = vy[i]; if (vy[i] > ymax) ymax = vy[i]; }
    for (int y = (int)ceilf(ymin); y <= (int)floorf(ymax); y++) {
        float xs[16];
        int nx = 0;
        for (int i = 0; i < n && nx < 16; i++) {
            int j = (i + 1) % n;
            float ya = vy[i], yb = vy[j];
            if ((ya <= y && yb > y) || (yb <= y && ya > y))
                xs[nx++] = vx[i] + (y - ya) / (yb - ya) * (vx[j] - vx[i]);
        }
        for (int a = 1; a < nx; a++)
            for (int b = a; b > 0 && xs[b] < xs[b - 1]; b--) { float t = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = t; }
        for (int k = 0; k + 1 < nx; k += 2)
            for (int x = (int)ceilf(xs[k]); x <= (int)floorf(xs[k + 1]); x++) {
                float dx = x - cx, dy = y - cy;
                int p = padrao(tex, dx * ca - dy * sa, dx * sa + dy * ca, s);
                gfx_pixel(x, y, p < 0 ? esc : (p > 0 ? cla : cor));
            }
    }
}

// A forma diz quantos lados o dado tem antes de qualquer numero:
// moeda redonda, d4 triangulo, d6 quadrado de canto aparado, d8 losango,
// d12 pentagono, d20 hexagono.
static int forma(int lados, float cx, float cy, float r, float ang,
                 float *vx, float *vy)
{
    int n = 0;
    switch (lados) {
    case 4:
        for (int i = 0; i < 3; i++) {
            float a = ang - 1.5708f + i * 2.0944f;
            vx[n] = cx + cosf(a) * r * 1.2f; vy[n] = cy + sinf(a) * r * 1.2f + r * 0.2f; n++;
        }
        break;
    case 6:
        for (int i = 0; i < 4; i++) {
            float a = ang + 0.7854f + i * 1.5708f;
            for (int k = -1; k <= 1; k += 2) {
                float b = a + k * 0.24f;
                vx[n] = cx + cosf(b) * r * 1.08f; vy[n] = cy + sinf(b) * r * 1.08f; n++;
            }
        }
        break;
    case 8:
        for (int i = 0; i < 4; i++) {
            float a = ang + i * 1.5708f;
            vx[n] = cx + cosf(a) * r * 1.18f; vy[n] = cy + sinf(a) * r * 1.18f; n++;
        }
        break;
    case 12:
        for (int i = 0; i < 5; i++) {
            float a = ang - 1.5708f + i * 1.2566f;
            vx[n] = cx + cosf(a) * r * 1.08f; vy[n] = cy + sinf(a) * r * 1.08f; n++;
        }
        break;
    default:   // 20
        for (int i = 0; i < 6; i++) {
            float a = ang + i * 1.0472f;
            vx[n] = cx + cosf(a) * r * 1.08f; vy[n] = cy + sinf(a) * r * 1.08f; n++;
        }
        break;
    }
    return n;
}

static void pips(int cx, int cy, int v, int r, uint16_t c)
{
    int o = r * 5 / 11;
    static const int8_t P[7][6][2] = {
        {{0}}, {{0,0}}, {{-1,-1},{1,1}}, {{-1,-1},{0,0},{1,1}},
        {{-1,-1},{1,-1},{-1,1},{1,1}}, {{-1,-1},{1,-1},{0,0},{-1,1},{1,1}},
        {{-1,-1},{1,-1},{-1,0},{1,0},{-1,1},{1,1}},
    };
    if (v < 1 || v > 6) return;
    int pr = r / 7 > 1 ? r / 7 : 1;
    for (int i = 0; i < v; i++) {
        int x = cx + P[v][i][0] * o, y = cy + P[v][i][1] * o;
        if (r < 12) { gfx_rect(x - 1, y - 1, 3, 3, c); continue; }
        gfx_disco(x, y, pr, c);
    }
}

// modo 0: parado na mao, mostra quantos lados tem.
// modo 1: rolando, mostra uma face qualquer.
// modo 2: pousado, mostra o valor final (e riscado se zerou).
// ---------------------------------------------------------------------------
// Marcas de cada dado especial. Cada uma conta o efeito: o Dobro tem um dado
// atras, o Martelo tem cabeca de martelo, o Gatuno usa mascara. Todas sao
// desenhadas em coordenadas do dado e giram junto com ele. Nenhuma cobre o
// valor: vao fora do contorno ou na borda do corpo.
// ---------------------------------------------------------------------------
static float mca, msa, mcx, mcy;            // rotacao da marca em curso
static int MX(float ox, float oy) { return (int)lroundf(mcx + ox * mca - oy * msa); }
static int MY(float ox, float oy) { return (int)lroundf(mcy + ox * msa + oy * mca); }

// Poligono em coordenadas do dado (pares ox, oy), girado com ele.
static void marca_poli(const float *o, int n, uint16_t c)
{
    float vx[16], vy[16];
    for (int i = 0; i < n; i++) {
        vx[i] = mcx + o[2 * i] * mca - o[2 * i + 1] * msa;
        vy[i] = mcy + o[2 * i] * msa + o[2 * i + 1] * mca;
    }
    poligono(vx, vy, n, c);
}

// Retangulo girado: centro (ox, oy), meia-largura hw, meia-altura hh.
static void marca_ret(float ox, float oy, float hw, float hh, uint16_t c)
{
    float o[8] = { ox - hw, oy - hh, ox + hw, oy - hh, ox + hw, oy + hh, ox - hw, oy + hh };
    marca_poli(o, 4, c);
}

static uint16_t mistura(uint16_t a, uint16_t b, int pct)
{
    int ra = (a >> 11) & 31, ga = (a >> 5) & 63, ba = a & 31;
    int rb = (b >> 11) & 31, gb = (b >> 5) & 63, bb = b & 31;
    int r = ra + (rb - ra) * pct / 100, g = ga + (gb - ga) * pct / 100, bl = ba + (bb - ba) * pct / 100;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

// O corpo de um dado sempre numa cor da paleta, escolhida entre as que servem
// a um dado: nem pele (dos retratos) nem feltro (da mesa).
static uint16_t cor_de_dado(uint16_t c)
{
    int r = ((c >> 11) & 31) * 255 / 31, g = ((c >> 5) & 63) * 255 / 63, b = (c & 31) * 255 / 31;
    uint16_t melhor = c;
    long dmin = -1;
    for (unsigned i = 0; i < sizeof PALETA / sizeof PALETA[0]; i++) {
        uint16_t p = PALETA[i];
        if (i >= PAL_PELE || (i >= PAL_FELTRO && i < PAL_FELTRO + 7)) continue;
        int pr = ((p >> 11) & 31) * 255 / 31, pg = ((p >> 5) & 63) * 255 / 63, pb = (p & 31) * 255 / 31;
        int rm = (r + pr) / 2, dr = r - pr, dg = g - pg, db = b - pb;
        long d = (long)(512 + rm) * dr * dr / 256 + 4L * dg * dg + (long)(767 - rm) * db * db / 256;
        if (dmin < 0 || d < dmin) { dmin = d; melhor = p; }
    }
    return melhor;
}

// Cor de corpo e aro proprios de alguns dados. O tom vem misturado com o
// marfim ou o ebano do dono: claro para ERIC, escuro para LILI, sempre.
static void cores_do_tipo(int tipo, int dono, uint16_t *corpo, uint16_t *aro, uint16_t *tinta)
{
    int pct = 0;
    uint16_t tom = 0;
    switch (tipo) {
    case D_INVEJOSO: tom = C_LIMA;   pct = 38; *aro = C_LIMA;   break;
    case D_MALDICAO: tom = C_ROXO;   pct = 36; *aro = C_ROXO;   break;
    case D_MALDITO:  tom = C_VIOLETA; pct = 34; break;          // Agouro
    case D_ESPELHO:  tom = C_PRATA;  pct = 55; *aro = C_PRATA;  break;
    case D_FICHAS:   tom = C_OURO;   pct = 42; break;
    case D_D2:       tom = C_OURO;   pct = 58; break;           // moeda dourada
    case D_TREVO:    tom = C_TREVO;  pct = 34; break;
    case D_JACKPOT:  tom = C_OURO;   pct = 16; break;
    case D_COPIADOR: tom = C_CIANO;  pct = 28; break;
    case D_EGOISTA:  tom = C_EGO;    pct = 20; break;
    case D_LASER:    tom = C_CHUMBO; pct = 34; *aro = C_LASER;  break;
    case D_VENTANIA: tom = C_VENTO;  pct = 40; *aro = C_VENTO;  break;
    case D_VICIADO:  *tinta = C_VERMELHO; break;               // pips de cassino
    case D_MISERICORDIOSO: tom = C_HALO; pct = 34; break;
    case D_SOLITARIO: tom = C_NOITE; pct = 30; *aro = C_NOITE; break;
    case D_SENTINELA: tom = C_PEDRA; pct = 46; break;
    case D_FERREIRO: tom = C_FERRO;  pct = 32; break;
    case D_COBRADOR: tom = C_COBRE;  pct = 22; break;
    case D_DESAFIANTE: tom = C_CARMIM; pct = 20; break;
    case D_VIDRO:    tom = C_VIDRO;  pct = 52; *aro = C_VIDRO; break;
    case D_ACUMULADOR: tom = C_AMBAR; pct = 20; break;
    case D_SEGURANCA: tom = RGB(90, 120, 170); pct = 24; break;
    case D_BUMERANGUE: tom = C_MADEIRA_CLR; pct = 18; break;
    case D_GEMEO:    tom = C_CIANO;  pct = 16; break;
    case D_PRISMA:   tom = RGB(176, 132, 220); pct = 26; *aro = RGB(176, 132, 220); break;
    case D_FENIX:    tom = C_FOGO;   pct = 18; break;
    case D_TRONO:    tom = C_OURO;   pct = 14; break;
    case D_CAVALO:   tom = C_CARVALHO; pct = 30; *aro = C_CARVALHO; break;
    case D_BERSERKER: *aro = C_SANGUE; break;
    case D_VIUVA:                                 // preta para os dois, com a tinta clara
        *corpo = dono == 0 ? RGB(46, 46, 52) : RGB(26, 26, 30);
        *tinta = RGB(214, 214, 220);
        break;
    default: break;
    }
    if (pct) *corpo = cor_de_dado(mistura(*corpo, tom, dono == 0 ? pct : pct - 12));
}

// Atras do corpo: as copias do Dobro e do Par.
static void marca_atras(int tipo, int lados, float r, uint16_t corpo, uint16_t aro, float ang)
{
    float vx[12], vy[12];
    if (tipo == D_DOBRO) {                        // um segundo dado, deslocado
        int n = forma(lados, MX(r * 0.42f, -r * 0.42f), MY(r * 0.42f, -r * 0.42f), r + 1, ang, vx, vy);
        poligono(vx, vy, n, aro);
        n = forma(lados, MX(r * 0.42f, -r * 0.42f), MY(r * 0.42f, -r * 0.42f), r - 1, ang, vx, vy);
        poligono(vx, vy, n, mistura(corpo, C_SOMBRA, 35));
    }
    if (tipo == D_COPIADOR) {                     // a copia fantasma, atras e a esquerda
        float ox = -r * 0.34f, oy = -r * 0.26f;
        int n = forma(lados, MX(ox, oy), MY(ox, oy), r * 0.92f + 1, ang, vx, vy);
        poligono(vx, vy, n, gfx_mistura(C_CIANO, C_SOMBRA, 25));
        n = forma(lados, MX(ox, oy), MY(ox, oy), r * 0.92f - 1, ang, vx, vy);
        poligono(vx, vy, n, gfx_mistura(corpo, C_SOMBRA, 45));
    }
    if (tipo == D_COBRADOR) {                     // uma moeda grande atras do dado
        int x = MX(r * 0.8f, -r * 0.52f), y = MY(r * 0.8f, -r * 0.52f), R = (int)(r * 0.56f);
        gfx_disco(x, y, R + 1, C_LATAO_ESC);
        gfx_disco(x, y, R, C_OURO);
        gfx_disco(x, y, R - (R > 8 ? 3 : 2), C_LATAO);
        gfx_disco(x, y, R - (R > 8 ? 4 : 3), C_OURO);
        gfx_disco(x - R / 3, y - R / 3, R > 8 ? 2 : 1, C_BRANCO);
    }
    if (tipo == D_PAR) {                          // gemeo pequeno encostado
        float rr = r * 0.55f;
        int n = forma(lados, MX(-r * 0.95f, r * 0.35f), MY(-r * 0.95f, r * 0.35f), rr + 1, ang, vx, vy);
        poligono(vx, vy, n, aro);
        n = forma(lados, MX(-r * 0.95f, r * 0.35f), MY(-r * 0.95f, r * 0.35f), rr - 1, ang, vx, vy);
        poligono(vx, vy, n, corpo);
    }
}

// Na frente: tudo o que se prende ao dado sem cobrir o valor.
// Espinho: triangulo com base no ponto (bx, by) e ponta na direcao (nx, ny).
static void espinho(float bx, float by, float nx, float ny, float comp, float meia, uint16_t c)
{
    float o[6] = { bx - ny * meia, by + nx * meia, bx + nx * comp, by + ny * comp,
                   bx + ny * meia, by - nx * meia };
    marca_poli(o, 3, c);
}

// Espessura dos tracos das marcas: 1 px num dado pequeno, mais num grande.
static int esp_marca(float t) { return t < 12 ? 1 : (int)(t / 11); }

// Quantos pontos o Acumulador guarda: quem desenha a peca avisa (-1: nao sabe).
static int marca_nivel = -1;

// Na frente: tudo o que se prende ao dado. O valor vem depois, por cima.
static void marca_frente(int tipo, float r, int dono, uint16_t corpo)
{
    float t = r < 5 ? 5 : r;                      // escala minima legivel
    uint16_t escuro = dono == 0 ? C_BARRA : C_MARFIM_S;
    switch (tipo) {
    case D_LASTRO:                                // base pesada
        marca_ret(0, t * 1.02f, t * 0.85f, t * 0.16f + 1, C_LATAO_ESC);
        marca_ret(0, t * 0.9f, t * 0.85f, 0.6f, C_LATAO);
        break;
    case D_TEIMOSO:                               // uma cinta: nao anula na queda
        marca_ret(0, 0, t * 0.8f, t * 0.13f + 0.6f, C_SALMAO);
        break;
    case D_ESCUDO: {                              // um brasao: nada o anula
        // (Nada em X: o X e de dado anulado.)
        float e = t * 0.66f, f = e - esp_marca(t) - 0.6f;
        float o[10] = { -e, -e * 0.85f, e, -e * 0.85f, e, e * 0.1f, 0, e * 1.05f, -e, e * 0.1f };
        float i[10] = { -f, -f * 0.85f - (e - f) * 0.15f, f, -f * 0.85f - (e - f) * 0.15f,
                        f, f * 0.1f, 0, f * 1.05f, -f, f * 0.1f };
        marca_poli(o, 5, C_SALMAO);
        marca_poli(i, 5, mistura(corpo, C_SALMAO, 30));
        break;
    }
    case D_VICIADO: {                             // gota de chumbo pendurada no canto
        float cx = 1.02f * t, cy = 1.02f * t, gr = 0.24f * t + 0.8f;
        float o[6] = { 0.62f * t, 0.78f * t, 0.78f * t, 0.62f * t, cx + gr * 0.2f, cy + gr * 0.2f };
        marca_poli(o, 3, C_CHUMBO);
        gfx_disco(MX(cx, cy), MY(cx, cy), (int)gr, C_CHUMBO);
        gfx_disco(MX(cx - gr * 0.35f, cy - gr * 0.35f), MY(cx - gr * 0.35f, cy - gr * 0.35f),
                  (int)(gr * 0.3f), C_ACO);                      // brilho do metal
        break;
    }
    case D_ESPELHO:                               // reflexo na diagonal
        gfx_linha_grossa(MX(-0.75f * t, -0.2f * t), MY(-0.75f * t, -0.2f * t),
                         MX(-0.2f * t, -0.75f * t), MY(-0.2f * t, -0.75f * t),
                         esp_marca(t) + 1, C_MARFIM);
        break;
    case D_EXPLOSIVO:                             // faiscas saindo das faces
        for (int i = 0; i < 4; i++) {
            float a = 0.785f + i * 1.5708f, ca = cosf(a), sa = sinf(a);
            float b = 0.8f * t, p = 1.35f * t, w = 0.22f * t + 0.6f;
            float o[6] = { ca * b - sa * w, sa * b + ca * w, ca * p, sa * p, ca * b + sa * w, sa * b - ca * w };
            marca_poli(o, 3, C_AMARELO);
        }
        break;
    case D_FARTURA: {                             // graos caindo da base
        static const float G[5][2] = { {-0.7f,1.05f}, {-0.3f,1.25f}, {0.1f,1.08f}, {0.5f,1.28f}, {0.8f,1.05f} };
        for (int i = 0; i < 5; i++) marca_ret(G[i][0] * t, G[i][1] * t, 0.8f, 0.8f, C_OURO);
        break;
    }
    case D_CARRASCO: {                            // capuz escuro na ponta
        float o[6] = { 0, -1.0f * t, -0.42f * t, -0.3f * t, 0.42f * t, -0.3f * t };
        marca_poli(o, 3, dono == 0 ? C_BARRA : C_VINHO);
        break;
    }
    case D_MARTELO: {                             // Espinhoso: espinhos no contorno todo
        float R = t + 1;
        float V[3][2];
        for (int i = 0; i < 3; i++) {
            float a = -1.5708f + i * 2.0944f;
            V[i][0] = cosf(a) * R * 1.2f;
            V[i][1] = sinf(a) * R * 1.2f + t * 0.2f;
        }
        float c_x = 0, c_y = t * 0.2f;
        for (int i = 0; i < 3; i++) {
            float ax = V[i][0], ay = V[i][1], bx = V[(i + 1) % 3][0], by = V[(i + 1) % 3][1];
            float ex = bx - ax, ey = by - ay, el = sqrtf(ex * ex + ey * ey);
            float nx = ey / el, ny = -ex / el;
            float mx = (ax + bx) / 2 - c_x, my = (ay + by) / 2 - c_y;
            if (nx * mx + ny * my < 0) { nx = -nx; ny = -ny; }
            for (int k = 1; k <= 3; k++) {
                float q = k / 4.0f;
                espinho(ax + ex * q, ay + ey * q, nx, ny, 0.34f * t + 1, 0.12f * t + 0.6f, C_VINHO_CLR);
            }
            float vl = sqrtf((ax - c_x) * (ax - c_x) + (ay - c_y) * (ay - c_y));
            espinho(ax, ay, (ax - c_x) / vl, (ay - c_y) / vl, 0.38f * t + 1, 0.12f * t + 0.6f, C_VINHO_CLR);
        }
        break;
    }
    case D_PIRATA: {                              // tapa-olho, tira e brinco
        gfx_linha_grossa(MX(-1.0f * t, 0.22f * t), MY(-1.0f * t, 0.22f * t),
                         MX(0.22f * t, -1.0f * t), MY(0.22f * t, -1.0f * t),
                         esp_marca(t) + 1, escuro);
        gfx_disco(MX(-0.36f * t, -0.36f * t), MY(-0.36f * t, -0.36f * t), (int)(0.27f * t + 0.8f), escuro);
        gfx_disco(MX(1.28f * t, 0.12f * t), MY(1.28f * t, 0.12f * t), (int)(0.1f * t + 0.8f), C_OURO);
        break;
    }
    case D_GATUNO: {                              // mascara de ladrao, com o no solto
        float y = -0.8f * t, h = 0.16f * t + 0.6f;
        marca_ret(-0.05f * t, y, 1.2f * t, h, escuro);
        marca_ret(-0.36f * t, y, 0.14f * t + 0.4f, 0.09f * t + 0.4f, corpo);
        marca_ret(0.36f * t, y, 0.14f * t + 0.4f, 0.09f * t + 0.4f, corpo);
        gfx_linha_grossa(MX(1.12f * t, y), MY(1.12f * t, y),
                         MX(1.48f * t, y + 0.38f * t), MY(1.48f * t, y + 0.38f * t), esp_marca(t) + 1, escuro);
        gfx_linha_grossa(MX(1.12f * t, y), MY(1.12f * t, y),
                         MX(1.28f * t, y + 0.55f * t), MY(1.28f * t, y + 0.55f * t), esp_marca(t) + 1, escuro);
        break;
    }
    case D_INVEJOSO:                              // olhos colados, olhando de lado
        for (int i = 0; i < 2; i++) {
            float ox = (i ? 0.34f : -0.3f) * t, oy = -1.0f * t;
            marca_ret(ox, oy, 0.26f * t + 0.6f, 0.2f * t + 0.5f, C_MARFIM);
            marca_ret(ox + 0.12f * t, oy, 0.13f * t + 0.5f, 0.13f * t + 0.5f, C_BARRA);
        }
        break;
    case D_CARIDOSO: {                            // coracao na ponta
        float h = t * 0.2f + 0.8f;
        marca_ret(-h, -1.2f * t, h * 0.9f, h * 0.8f, C_CORACAO);
        marca_ret(h, -1.2f * t, h * 0.9f, h * 0.8f, C_CORACAO);
        float o[6] = { -2 * h, -1.2f * t, 2 * h, -1.2f * t, 0, -1.2f * t + 2.4f * h };
        marca_poli(o, 3, C_CORACAO);
        break;
    }
    case D_MALDICAO: {                            // fumaca subindo da ponta
        static const float F[3][3] = { {0.1f,-1.2f,0.2f}, {-0.22f,-1.45f,0.16f}, {0.16f,-1.68f,0.12f} };
        for (int i = 0; i < 3; i++)
            gfx_disco(MX(F[i][0] * t, F[i][1] * t), MY(F[i][0] * t, F[i][1] * t),
                      (int)(F[i][2] * t + 1), i ? C_CINZA : C_ROXO);
        break;
    }
    case D_MALDITO: {                             // Agouro: corpo manchado
        uint16_t mancha = dono == 0 ? RGB(92, 60, 132) : RGB(38, 24, 58);
        uint16_t clara  = dono == 0 ? RGB(204, 186, 232) : RGB(146, 118, 186);
        static const float M[7][4] = {
            {-0.52f,-0.36f,0.2f,0}, {0.46f,-0.46f,0.16f,1}, {0.56f,0.34f,0.18f,0},
            {-0.3f,0.58f,0.15f,1}, {-0.66f,0.2f,0.12f,1}, {0.14f,-0.72f,0.12f,0}, {0.22f,0.7f,0.11f,0} };
        for (int i = 0; i < 7; i++)
            gfx_disco(MX(M[i][0] * t, M[i][1] * t), MY(M[i][0] * t, M[i][1] * t),
                      (int)(M[i][2] * t + 0.6f), M[i][3] ? clara : mancha);
        break;
    }
    case D_JACKPOT: {                             // tres caixinhas com um 7 vermelho
        float y = 1.16f * t, h = 0.19f * t + 0.8f, passo = 0.46f * t + 1;
        marca_ret(0, y, passo * 1.5f + h * 0.3f, h + 1.2f, C_LATAO_ESC);
        int e = t < 14 ? 1 : 2;
        for (int i = -1; i <= 1; i++) {
            float bx = i * passo;
            marca_ret(bx, y, h, h, C_MARFIM);
            // o 7: traco de cima e a perna em diagonal
            gfx_linha_grossa(MX(bx - h * 0.55f, y - h * 0.6f), MY(bx - h * 0.55f, y - h * 0.6f),
                             MX(bx + h * 0.55f, y - h * 0.6f), MY(bx + h * 0.55f, y - h * 0.6f), e, C_VERMELHO);
            gfx_linha_grossa(MX(bx + h * 0.55f, y - h * 0.6f), MY(bx + h * 0.55f, y - h * 0.6f),
                             MX(bx - h * 0.15f, y + h * 0.7f), MY(bx - h * 0.15f, y + h * 0.7f), e, C_VERMELHO);
        }
        break;
    }
    case D_COPIADOR: {                            // icone de copiar no canto
        float c = 1.02f * t, h = 0.2f * t + 0.8f;
        marca_ret(c + 0.12f * t, -c + 0.12f * t, h, h, C_CIANO);
        marca_ret(c + 0.12f * t, -c + 0.12f * t, h - 1, h - 1, C_BARRA);
        marca_ret(c, -c, h, h, C_CIANO);
        marca_ret(c, -c, h - 1, h - 1, C_BRANCO);
        break;
    }
    case D_EGOISTA: {                             // coroa pequena, baixa, pousada na ponta de cima
        uint16_t ouro = gfx_mistura(C_OURO, C_LATAO_ESC, 30);
        float y = -1.04f * t, w = 0.3f * t + 0.5f, h = 0.32f * t + 0.6f;
        float o[14] = { -w, y, w, y, w, y - h, w * 0.5f, y - h * 0.55f, 0, y - h * 1.05f,
                        -w * 0.5f, y - h * 0.55f, -w, y - h };
        marca_poli(o, 7, ouro);
        marca_ret(0, y - 0.04f * t, w, 0.04f * t + 0.5f, C_LATAO_ESC);          // o aro da coroa
        if (t >= 12) gfx_pixel(MX(0, y - h * 0.5f), MY(0, y - h * 0.5f), C_VINHO);   // uma pedra so
        break;
    }
    case D_TREVO: {                               // trevo de quatro folhas na ponta
        // Quatro folhas na diagonal, separadas por um vao no meio.
        float cy0 = -1.4f * t, d = 0.24f * t + 0.4f;
        int fr = (int)(0.16f * t + 0.8f);
        uint16_t folha = RGB(96, 196, 112);
        gfx_linha_grossa(MX(0, cy0), MY(0, cy0), MX(0.12f * t, cy0 + 0.55f * t),
                         MY(0.12f * t, cy0 + 0.55f * t), esp_marca(t), folha);
        gfx_disco(MX(-d, cy0 - d), MY(-d, cy0 - d), fr, folha);
        gfx_disco(MX(d, cy0 - d), MY(d, cy0 - d), fr, folha);
        gfx_disco(MX(-d, cy0 + d), MY(-d, cy0 + d), fr, folha);
        gfx_disco(MX(d, cy0 + d), MY(d, cy0 + d), fr, folha);
        break;
    }
    case D_LASER: {                               // emissor no topo, lente verde e um raio curto
        marca_ret(0, -1.0f * t, 0.3f * t + 0.6f, 0.17f * t + 0.6f, C_CHUMBO);
        marca_ret(0, -1.0f * t, 0.3f * t - 0.4f, 0.17f * t - 0.4f, escuro);
        gfx_disco(MX(0, -1.0f * t), MY(0, -1.0f * t), (int)(0.1f * t + 0.8f), C_LASER);
        gfx_linha_grossa(MX(0, -1.2f * t), MY(0, -1.2f * t), MX(0, -1.62f * t), MY(0, -1.62f * t),
                         esp_marca(t), C_LASER);
        break;
    }
    case D_VENTANIA: {                            // tres rajadas de vento saindo pela esquerda
        static const float V[3][3] = { { -0.55f, 1.0f, 0.9f }, { 0.0f, 1.25f, 1.0f }, { 0.55f, 0.95f, 0.8f } };
        int e = esp_marca(t);
        for (int i = 0; i < 3; i++) {
            float y = V[i][0] * t, x0 = -V[i][1] * t, x1 = x0 - V[i][2] * t * 0.6f;
            gfx_linha_grossa(MX(x0, y), MY(x0, y), MX(x1, y), MY(x1, y), e, C_BRANCO);
            // o caracol na ponta da rajada
            gfx_linha_grossa(MX(x1, y), MY(x1, y), MX(x1 - 0.12f * t, y - 0.14f * t),
                             MY(x1 - 0.12f * t, y - 0.14f * t), e, C_BRANCO);
            gfx_linha_grossa(MX(x1 - 0.12f * t, y - 0.14f * t), MY(x1 - 0.12f * t, y - 0.14f * t),
                             MX(x1 + 0.02f * t, y - 0.26f * t), MY(x1 + 0.02f * t, y - 0.26f * t), e, C_BRANCO);
        }
        break;
    }
    case D_MISERICORDIOSO: {                      // aureola flutuando sobre a ponta
        float cy0 = -1.5f * t, rx = 0.5f * t + 1, ry = 0.16f * t + 1;
        int e = t < 14 ? 1 : 2;
        for (int i = 0; i < 32; i++) {
            float a = i * 0.19635f;
            int px = MX(cosf(a) * rx, cy0 + sinf(a) * ry), py = MY(cosf(a) * rx, cy0 + sinf(a) * ry);
            gfx_rect(px, py, e, e, sinf(a) < 0 ? C_BRANCO : C_HALO);
        }
        break;
    }
    case D_SOLITARIO: {                           // estrela de quatro pontas no centro, atras do numero
        float cy0 = 0.08f * t, a = 0.64f * t + 1, b = 0.17f * t + 0.6f;
        uint16_t c = gfx_mistura(corpo, C_OURO, dono == 0 ? 60 : 50);
        float v[8] = { 0, cy0 - a, b, cy0, 0, cy0 + a, -b, cy0 };
        float h[8] = { -a, cy0, 0, cy0 - b, a, cy0, 0, cy0 + b };
        marca_poli(v, 4, c);
        marca_poli(h, 4, c);
        break;
    }
    case D_FERREIRO: {                            // bigorna pequena dentro do dado, embaixo do numero
        uint16_t c = dono == 0 ? gfx_mistura(C_FERRO, C_BARRA, 35) : C_FERRO;
        float y = 0.44f * t, h = 0.07f * t + 0.5f;
        float mesa_b[8] = { -0.46f * t, y, 0.4f * t, y, 0.36f * t, y + 2 * h, -0.4f * t, y + 2 * h };
        marca_poli(mesa_b, 4, c);
        float chifre[6] = { -0.46f * t, y, -0.72f * t, y + 0.2f * h, -0.46f * t, y + 1.4f * h };
        marca_poli(chifre, 3, c);
        marca_ret(0, y + 2.6f * h, 0.12f * t + 0.5f, 0.8f * h, c);          // cintura
        marca_ret(0, y + 3.6f * h, 0.3f * t + 0.5f, 0.6f * h, c);           // pe
        break;
    }
    case D_DESAFIANTE: {                          // flamula de desafio fincada na ponta
        float y0 = -1.12f * t, y1 = -1.95f * t;
        gfx_linha_grossa(MX(0, y0), MY(0, y0), MX(0, y1), MY(0, y1), esp_marca(t), C_LATAO);
        float o[6] = { 0, y1, 0.62f * t + 1, y1 + 0.2f * t, 0, y1 + 0.42f * t + 1 };
        marca_poli(o, 3, C_CARMIM);
        gfx_disco(MX(0, y1), MY(0, y1), (int)(0.06f * t + 0.8f), C_OURO);
        break;
    }
    case D_VIDRO: {                               // dois reflexos na borda de cima
        int e = esp_marca(t);
        gfx_linha_grossa(MX(-0.62f * t, -0.12f * t), MY(-0.62f * t, -0.12f * t),
                         MX(-0.12f * t, -0.62f * t), MY(-0.12f * t, -0.62f * t), e, C_BRANCO);
        gfx_linha_grossa(MX(-0.62f * t, 0.12f * t), MY(-0.62f * t, 0.12f * t),
                         MX(-0.42f * t, -0.08f * t), MY(-0.42f * t, -0.08f * t), e, C_BRANCO);
        break;
    }
    case D_SEGURANCA: {                           // cadeado embaixo do numero, no centro (longe dos cantos)
        uint16_t c = dono == 0 ? RGB(70, 96, 140) : RGB(150, 176, 220);
        float y = 0.62f * t, w = 0.2f * t + 0.6f, h = 0.15f * t + 0.6f;
        int e = esp_marca(t);
        int ax = MX(0, y - h), ay = MY(0, y - h);
        for (int i = 0; i <= 12; i++) {                                      // a alca
            float a = 3.1416f + i * 0.2618f;
            int px = ax + (int)(cosf(a) * (w * 0.7f)), py = ay + (int)(sinf(a) * (w * 0.75f));
            gfx_rect(px, py, e, e, c);
        }
        marca_ret(0, y, w, h, c);                                            // o corpo
        gfx_rect(MX(0, y), MY(0, y), 1, 1 + e, corpo);                       // o buraco da chave
        break;
    }
    case D_ACUMULADOR: {                          // seis casas em volta do numero: as acesas sao o bonus
        int n = marca_nivel < 0 ? 0 : (marca_nivel > 6 ? 6 : marca_nivel);
        static const float P[6][2] = { {0,-0.74f}, {0.56f,-0.3f}, {0.56f,0.3f}, {0,0.74f}, {-0.56f,0.3f}, {-0.56f,-0.3f} };
        int pr = (int)(0.08f * t + 0.8f);
        for (int i = 0; i < 6; i++) {
            int x = MX(P[i][0] * t, P[i][1] * t), y = MY(P[i][0] * t, P[i][1] * t);
            gfx_disco(x, y, pr + 1, gfx_mistura(corpo, C_BARRA, 30));
            gfx_disco(x, y, pr, i < n ? C_AMBAR : gfx_mistura(corpo, C_BARRA, 12));
        }
        break;
    }
    case D_BUMERANGUE: {                          // bumerangue inclinado, no alto (onde os pontos nao vao)
        uint16_t m = dono == 0 ? C_MADEIRA_CLR : RGB(176, 124, 80);
        int e = esp_marca(t) + 1;
        float ex = -0.06f * t, ey = -0.8f * t;
        gfx_linha_grossa(MX(ex, ey), MY(ex, ey), MX(-0.3f * t, -0.5f * t), MY(-0.3f * t, -0.5f * t), e, m);
        gfx_linha_grossa(MX(ex, ey), MY(ex, ey), MX(0.32f * t, -0.62f * t), MY(0.32f * t, -0.62f * t), e, m);
        gfx_disco(MX(ex, ey), MY(ex, ey), e / 2 + 1, gfx_mistura(m, C_BRANCO, 25));
        break;
    }
    case D_GEMEO: {                               // duas contas iguais, ligadas, no alto
        float y = -0.74f * t, d = 0.2f * t + 0.6f;
        int pr = (int)(0.1f * t + 1.0f);
        uint16_t c = dono == 0 ? gfx_mistura(C_CIANO, C_BARRA, 35) : C_CIANO;
        gfx_linha_grossa(MX(-d, y), MY(-d, y), MX(d, y), MY(d, y), esp_marca(t), c);
        gfx_disco(MX(-d, y), MY(-d, y), pr, c);
        gfx_disco(MX(d, y), MY(d, y), pr, c);
        break;
    }
    case D_PRISMA: {                              // tres faixas de cor rente a borda de baixo, a esquerda
        static const uint16_t COR[3] = { RGB(220, 70, 70), RGB(230, 200, 70), RGB(80, 140, 230) };
        int e = esp_marca(t);
        for (int i = 0; i < 3; i++) {
            float o = (0.5f + i * 0.09f) * t;          // distancia da borda para dentro
            float a = 1.0f * t - o;
            gfx_linha_grossa(MX(-a * 0.92f, 0.08f * t), MY(-a * 0.92f, 0.08f * t),
                             MX(-0.08f * t, a * 0.92f), MY(-0.08f * t, a * 0.92f), e, COR[i]);
        }
        break;
    }
    case D_FENIX: {                               // chama com duas asas, embaixo, no centro
        float y = 0.6f * t, h = 0.26f * t + 0.6f;
        float asa_e[6] = { -0.06f * t, y + 0.04f * t, -0.34f * t, y - 0.2f * t, -0.2f * t, y + 0.12f * t };
        float asa_d[6] = { 0.06f * t, y + 0.04f * t, 0.34f * t, y - 0.2f * t, 0.2f * t, y + 0.12f * t };
        marca_poli(asa_e, 3, C_FOGO);
        marca_poli(asa_d, 3, C_FOGO);
        float ch[8] = { 0, y - h, 0.13f * t + 0.5f, y, 0, y + h * 0.7f, -0.13f * t - 0.5f, y };
        marca_poli(ch, 4, C_FOGO);
        float mi[8] = { 0, y - h * 0.4f, 0.06f * t + 0.4f, y + 0.02f * t, 0, y + h * 0.4f, -0.06f * t - 0.4f, y + 0.02f * t };
        marca_poli(mi, 4, C_AMARELO);
        break;
    }
    case D_TRONO: {                               // trono dourado embaixo do numero
        uint16_t c = dono == 0 ? gfx_mistura(C_OURO, C_BARRA, 20) : C_OURO;
        uint16_t vel = C_VINHO_CLR;
        float y = 0.36f * t, w = 0.26f * t + 0.5f;
        float enc[10] = { -w, y + 0.34f * t, -w, y + 0.06f * t, 0, y - 0.06f * t, w, y + 0.06f * t, w, y + 0.34f * t };
        marca_poli(enc, 5, c);                                             // encosto de ponta
        marca_ret(0, y + 0.2f * t, w * 0.55f, 0.1f * t + 0.3f, vel);      // almofada
        marca_ret(0, y + 0.4f * t, w + 0.1f * t, 0.06f * t + 0.5f, c);   // assento
        marca_ret(-w, y + 0.52f * t, 0.05f * t + 0.5f, 0.08f * t + 0.5f, c);   // pes
        marca_ret(w, y + 0.52f * t, 0.05f * t + 0.5f, 0.08f * t + 0.5f, c);
        break;
    }
    case D_CAVALO: {                              // crina discreta na aresta de cima, a direita
        // Tufos pontudos que nascem na aresta e caem para dentro do dado.
        uint16_t c = dono == 0 ? gfx_mistura(corpo, C_MADEIRA, 42) : gfx_mistura(corpo, C_CARVALHO, 45);
        float R = 1.02f * t;                          // a aresta, rente ao aro por dentro
        for (int i = 0; i < 7; i++) {
            float q0 = 0.14f + i * 0.1f, q1 = q0 + 0.13f;     // do alto para a ponta da direita
            float ax = R * q0, ay = -R * (1 - q0), bx = R * q1, by = -R * (1 - q1);
            float L = (i % 3 == 1 ? 0.3f : 0.22f) * t;     // tufos de tamanhos diferentes
            float px = (ax + bx) / 2 - 0.8f * L, py = (ay + by) / 2 + 0.45f * L;
            float o[6] = { ax, ay, bx, by, px, py };
            marca_poli(o, 3, c);
        }
        break;
    }
    case D_VIUVA: {                               // teia cinza por dentro
        uint16_t fio = gfx_mistura(corpo, C_TEIA, dono == 0 ? 45 : 50);
        uint16_t fio2 = gfx_mistura(corpo, C_TEIA, 28);
        float R = 0.9f * t;                           // ate onde a teia vai (dentro do aro)
        float px[8], py[8];
        for (int i = 0; i < 8; i++) {                 // raios: aos cantos e ao meio das arestas
            float a = i * 0.7854f, rr = (i & 1) ? R * 0.62f : R;
            px[i] = cosf(a) * rr; py[i] = sinf(a) * rr;
            gfx_linha(MX(0, 0), MY(0, 0), MX(px[i], py[i]), MY(px[i], py[i]), fio);
        }
        for (int c = 1; c <= 3; c++) {                // tres voltas, cedendo entre os raios
            float q = c / 3.4f;
            for (int i = 0; i < 8; i++) {
                int b = (i + 1) % 8;
                float ax = px[i] * q, ay = py[i] * q, bx = px[b] * q, by = py[b] * q;
                float mx = (ax + bx) * 0.44f, my = (ay + by) * 0.44f;
                gfx_linha(MX(ax, ay), MY(ax, ay), MX(mx, my), MY(mx, my), c == 3 ? fio2 : fio);
                gfx_linha(MX(mx, my), MY(mx, my), MX(bx, by), MY(bx, by), c == 3 ? fio2 : fio);
            }
        }
        break;
    }
    case D_BERSERKER: {                           // respingos de sangue seco e um elmo pequeno
        uint16_t sangue = gfx_mistura(corpo, RGB(104, 34, 30), dono == 0 ? 48 : 52);
        // Uma mancha maior embaixo, a esquerda, com gotas soltas; outra menor no alto.
        static const float M[9][3] = { { -0.52f, 0.42f, 0.15f }, { -0.38f, 0.5f, 0.11f }, { -0.62f, 0.3f, 0.09f },
                                       { -0.44f, 0.32f, 0.07f }, { -0.2f, 0.62f, 0.05f }, { -0.72f, 0.12f, 0.04f },
                                       { 0.56f, -0.36f, 0.08f }, { 0.64f, -0.26f, 0.05f }, { 0.42f, -0.5f, 0.035f } };
        for (int i = 0; i < 9; i++) {
            float rr = M[i][2] * t;
            if (rr < 0.6f) rr = 0.6f;
            gfx_disco(MX(M[i][0] * t, M[i][1] * t), MY(M[i][0] * t, M[i][1] * t), (int)(rr + 0.3f), sangue);
        }
        uint16_t osso = gfx_mistura(C_OSSO, C_CINZA, 40), ferro = gfx_mistura(C_ACO, C_CINZA, 45);
        float y = -0.93f * t;                     // a aresta de cima do hexagono
        for (int s = -1; s <= 1; s += 2) {        // chifres curtos, saindo dos cantos
            float o[10] = { s * 0.42f * t, y + 0.02f * t, s * 0.6f * t, y - 0.02f * t, s * 0.7f * t, y - 0.2f * t,
                            s * 0.62f * t, y - 0.14f * t, s * 0.44f * t, y - 0.1f * t };
            marca_poli(o, 5, osso);
        }
        marca_ret(0, y, 0.5f * t + 0.5f, 0.06f * t + 0.5f, ferro);   // a faixa do elmo
        break;
    }
    default:
        break;
    }
}

static float sombra_alt;                 // altura do dado no ar, em pixels

// Numero centrado no dado: grande nos dados grandes.
static void numero(float cx, float cy, float r, const char *s, uint16_t c)
{
    int e = r >= 34 ? 2 : 1;
    gfx_texto((int)cx - gfx_largura(s, e) / 2, (int)cy - 12 * e, s, c, e, true);
}

// Fechado no catalogo: o dado fica apagado como o anulado, mas sem o X.
static bool so_apagado;

// X grosso em vinho sobre o dado anulado.
static void risca(float cx, float cy, float r)
{
    if (so_apagado) return;
    int e = r < 12 ? 2 : (int)(r / 6);
    gfx_linha_grossa((int)(cx - r), (int)(cy - r), (int)(cx + r), (int)(cy + r), e, C_VINHO_CLR);
    gfx_linha_grossa((int)(cx - r), (int)(cy + r), (int)(cx + r), (int)(cy - r), e, C_VINHO_CLR);
}

// Borda de moeda serrilhada: aro com dentinhos em volta.
static void serrilha(float cx, float cy, int R, uint16_t aro, uint16_t dente)
{
    gfx_disco((int)cx, (int)cy, R, aro);
    int n = R < 14 ? 12 : 20;
    for (int i = 0; i < n; i++) {
        float a = i * 6.2832f / n;
        gfx_pixel((int)(cx + cosf(a) * (R - 1)), (int)(cy + sinf(a) * (R - 1)), dente);
        if (R >= 14) gfx_pixel((int)(cx + cosf(a) * (R - 2)), (int)(cy + sinf(a) * (R - 2)), dente);
    }
}

// Tudo ou Nada: moeda de ouro. Na mao, yin-yang de marfim e ebano (pode dar
// tudo, pode dar nada); caida, o lado que saiu: TUDO (2) marfim com brilhos
// dourados, NADA (1) ebano rachado.
static void tudo_ou_nada(float cx, float cy, float r, float ang, int valor, bool zerado)
{
    int R = (int)(r + (r < 12 ? 1 : r / 10)), Ri = R - (R < 14 ? 2 : 4);
    serrilha(cx, cy, R, zerado ? C_FELTRO2 : C_OURO, zerado ? C_FELTRO_ESC : C_LATAO_ESC);
    if (valor < 0) {                                   // yin-yang, girando com a moeda
        float ca = cosf(-ang), sa = sinf(-ang), h = Ri / 2.0f;
        for (int dy = -Ri; dy <= Ri; dy++)
            for (int dx = -Ri; dx <= Ri; dx++) {
                if (dx * dx + dy * dy > Ri * Ri) continue;
                float u = dx * ca - dy * sa, v = dx * sa + dy * ca;
                float dc = u * u + (v + h) * (v + h), db = u * u + (v - h) * (v - h);
                bool claro = dc <= h * h ? true : db <= h * h ? false : u < 0;
                float olho = h * 0.3f;
                if (u * u + (v + h) * (v + h) <= olho * olho) claro = false;
                if (u * u + (v - h) * (v - h) <= olho * olho) claro = true;
                gfx_pixel((int)cx + dx, (int)cy + dy, claro ? (zerado ? C_MARFIM_S : C_MARFIM)
                                                            : (zerado ? C_EBANO_B : C_EBANO));
            }
        return;
    }
    bool tudo = valor == 2;
    uint16_t face = zerado ? C_EBANO_B : tudo ? C_MARFIM : C_EBANO;
    gfx_disco((int)cx, (int)cy, Ri, face);
    if (!zerado && r >= 12) {
        // anel fino por dentro: dourado no TUDO, violeta no NADA
        for (int i = 0; i < 48; i++) {
            float a = i * 0.1309f;
            gfx_pixel((int)(cx + cosf(a) * (Ri - 3)), (int)(cy + sinf(a) * (Ri - 3)), tudo ? C_OURO : C_VIOLETA);
        }
        if (tudo)                                       // brilhos dourados
            for (int i = 0; i < 4; i++) {
                float a = ang + 0.6f + i * 1.5708f;
                int bx = (int)(cx + cosf(a) * Ri * 0.62f), by = (int)(cy + sinf(a) * Ri * 0.62f);
                gfx_rect(bx - 2, by, 5, 1, C_OURO);
                gfx_rect(bx, by - 2, 1, 5, C_OURO);
            }
        else {                                          // rachadura
            int e = esp_marca(r);
            gfx_linha_grossa((int)(cx + Ri * 0.15f), (int)(cy + Ri * 0.85f), (int)(cx + Ri * 0.35f), (int)(cy + Ri * 0.55f), e, C_EBANO_B);
            gfx_linha_grossa((int)(cx + Ri * 0.35f), (int)(cy + Ri * 0.55f), (int)(cx + Ri * 0.65f), (int)(cy + Ri * 0.62f), e, C_EBANO_B);
        }
    }
    char n[2] = { (char)('0' + valor), 0 };
    numero(cx, cy, r, n, zerado ? C_MARFIM_S : tudo ? C_REAL : C_MARFIM);
    if (zerado) risca(cx, cy, r);
}

static void desenha_dado(float cx, float cy, float r, int tipo, int dono,
                         float ang, int valor, int modo, bool zerado)
{
    int lados = TIPO[tipo].lados;
    uint16_t corpo = dono == 0 ? C_MARFIM_D : C_EBANO;
    uint16_t tinta = dono == 0 ? C_EBANO : C_MARFIM;
    uint16_t aro = cor_aro(tipo);
    cores_do_tipo(tipo, dono, &corpo, &aro, &tinta);
    if (zerado) {                        // anulado: o dado fica, apagado
        corpo = dono == 0 ? C_MARFIM_S : C_EBANO_B;
        tinta = dono == 0 ? C_EBANO_B : C_MARFIM_S;
        aro = C_FELTRO2;
    }
    mca = cosf(ang); msa = sinf(ang); mcx = cx; mcy = cy;
    float ea = r < 12 ? 1 : r / 10;      // espessura do aro
    int so = r < 12 ? 2 : (int)(r / 6);  // deslocamento da sombra

    int alt = (int)sombra_alt;                    // dado no ar: sombra mais longe
    gfx_disco((int)cx + so + alt, (int)cy + so + 1 + alt, (int)(r - alt * 0.2f), C_SOMBRA);   // sombra no feltro

    // Fichas de cassino: Moeda da sorte (azul 1, vermelho 2), Semente (verde,
    // com broto) e Ficha de Fogo (laranja e brasa, com chama). Na mao, o
    // centro mostra o simbolo; jogada, o valor.
    if (tipo == D_SORTE || tipo == D_SEMENTE || tipo == D_FOGO) {
        int R = (int)(r + ea);
        float u = r / 9;                             // unidade do desenho
        uint16_t f1, f2, marca;
        if (tipo == D_SORTE)        { f1 = C_AZUL;  f2 = C_VERMELHO; marca = C_MARFIM; }
        else if (tipo == D_SEMENTE) { f1 = C_VERDE; f2 = C_VERDE;    marca = C_MARFIM; }
        else                        { f1 = C_FOGO;  f2 = C_BRASA;    marca = C_AMARELO; }
        uint16_t face = modo == 0 ? (tipo == D_SORTE ? C_MARFIM_S : f1) : (valor == 1 ? f1 : f2);
        gfx_disco((int)cx, (int)cy, R, gfx_mistura(face, C_BARRA, 25));
        gfx_disco((int)cx, (int)cy - 1, R - 1, face);
        for (int i = 0; i < 8; i++) {                // marcas da borda
            float a = ang + i * 0.785f;
            int mx = (int)(cx + cosf(a) * (R - 2 * u)), my = (int)(cy + sinf(a) * (R - 2 * u));
            uint16_t c = (modo == 0 && tipo == D_SORTE) ? (i % 2 ? C_AZUL : C_VERMELHO) : marca;
            gfx_disco(mx, my, (int)(u + 0.5f), c);
        }
        gfx_disco((int)cx, (int)cy, (int)(R - 4 * u), (modo == 0 && tipo == D_SORTE) ? C_MARFIM : face);
        if (modo != 0) {
            char s2[4] = { (char)('0' + valor), 0 };
            numero(cx, cy, r, s2, C_MARFIM);
        } else if (tipo == D_SORTE) {
            numero(cx, cy, r, "$", C_OURO);
        } else if (tipo == D_SEMENTE) {              // broto
            int U = (int)(u + 0.5f);
            gfx_rect((int)cx, (int)(cy - 3 * u), U, 7 * U, C_MARFIM);
            gfx_rect((int)(cx - 3 * u), (int)(cy - 2 * u), 3 * U, 2 * U, C_MARFIM);
            gfx_rect((int)(cx + u), (int)(cy - 4 * u), 3 * U, 2 * U, C_MARFIM);
        } else {                                     // chama
            int U = (int)(u + 0.5f);
            gfx_rect((int)cx, (int)(cy - 5 * u), U, U, C_AMARELO);
            gfx_rect((int)(cx - u), (int)(cy - 4 * u), 3 * U, 2 * U, C_AMARELO);
            gfx_rect((int)(cx - 2 * u), (int)(cy - 2 * u), 5 * U, 4 * U, C_AMARELO);
            gfx_rect((int)(cx - u), (int)(cy + 2 * u), 3 * U, U, C_AMARELO);
            gfx_rect((int)cx, (int)(cy - u), U, 2 * U, C_BRASA);
        }
        if (zerado) risca(cx, cy, r);
        return;
    }

    if (tipo == D_TUDO_NADA) {
        tudo_ou_nada(cx, cy, r, ang, modo == 0 ? -1 : valor, zerado);
        return;
    } else if (lados == 2) {
        gfx_disco((int)cx, (int)cy, (int)(r + ea), aro);
        gfx_disco((int)cx, (int)cy, (int)(r - ea), gfx_mistura(corpo, C_BRANCO, 22));
        gfx_disco((int)cx + 1, (int)cy + 1, (int)(r - ea) - 1, corpo);
    } else {
        if (!zerado) marca_atras(tipo, lados, r, corpo, aro, ang);
        float vx[12], vy[12];
        int n = forma(lados, cx, cy, r + ea, ang, vx, vy);
        poligono(vx, vy, n, aro);
        // Brilho no canto de cima: o corpo claro por baixo, deslocado.
        n = forma(lados, cx, cy, r - ea, ang, vx, vy);
        poligono(vx, vy, n, zerado ? corpo : gfx_mistura(corpo, C_BRANCO, dono == 0 ? 35 : 18));
        if (r >= 12) {
            n = forma(lados, cx + ea * 0.8f, cy + ea * 0.8f, r - ea * 1.6f, ang, vx, vy);
            poligono_tex(vx, vy, n, corpo, zerado ? T_LISO : textura_de(tipo), cx, cy, ang, r, dono);
        }
    }
    if (!zerado) marca_frente(tipo, r, dono, corpo);

    char s[12];
    // Vidro que tirou 1: estilhacado, com rachaduras saindo do impacto.
    bool partido = tipo == D_VIDRO && modo == 2 && valor == 1 && zerado;
    if (partido) {
        int e = esp_marca(r);
        static const float RA[6][2] = { {-0.9f,-0.3f}, {-0.2f,-0.95f}, {0.7f,-0.6f}, {0.95f,0.25f}, {0.2f,0.9f}, {-0.6f,0.7f} };
        float ix = cx + r * 0.15f, iy = cy - r * 0.1f;
        for (int i = 0; i < 6; i++) {
            float mx = ix + RA[i][0] * r * 0.5f + (i % 2 ? 2 : -2), my = iy + RA[i][1] * r * 0.5f;
            gfx_linha_grossa((int)ix, (int)iy, (int)mx, (int)my, e, C_VIDRO);
            gfx_linha_grossa((int)mx, (int)my, (int)(ix + RA[i][0] * r), (int)(iy + RA[i][1] * r), e, C_VIDRO);
        }
        numero(cx, cy, r, "1", tinta);
        return;
    }
    if (tipo == D_QUEBRADO) {                    // rachadura em zigue-zague
        uint16_t rc = dono == 0 ? C_EBANO_B : C_MARFIM_S;
        int e = esp_marca(r);
        gfx_linha_grossa((int)(cx - r * 0.7f), (int)(cy - r * 0.6f), (int)(cx - r * 0.2f), (int)(cy - r * 0.1f), e, rc);
        gfx_linha_grossa((int)(cx - r * 0.2f), (int)(cy - r * 0.1f), (int)(cx + r * 0.1f), (int)(cy - r * 0.4f), e, rc);
    }
    if (modo == 0) {
        // Parado na mao mostra quantos lados tem - ou, no Quebrado, o numero
        // gravado nele, que e o que ele vai valer.
        if (tipo == D_QUEBRADO && valor) {
            snprintf(s, sizeof s, "%d", valor);
            numero(cx, cy, r, s, tinta);
            return;
        }
        // A tinta do dado a meio caminho do corpo: le-se bem, mas nao se
        // confunde com um valor tirado, que vem na tinta cheia.
        snprintf(s, sizeof s, "%d", lados);
        numero(cx, cy, r, s, gfx_mistura(tinta, corpo, 38));
        return;
    }
    if (valor < 0) return;               // so o corpo: o catalogo escreve grande
    if (lados == 6 && TIPO[tipo].efeito != EF_ESPELHO && valor >= 1 && valor <= 6
        && (!zerado || so_apagado)) {
        pips((int)cx, (int)cy, valor, (int)r, tinta);
    } else {
        snprintf(s, sizeof s, "%d", valor);
        numero(cx, cy, r, s, tinta);
    }
    if (zerado) risca(cx, cy, r);
}


// Moeda girando no ar: a arte de verdade da ficha (desenhada num rascunho)
// colada achatada na horizontal, pela largura aparente |escx|. Quando ela
// passa de lado, aparece a espessura.
#define CHAVE 0xF81F                                   // magenta: fundo do rascunho
static uint16_t rascunho[GFX_W * GFX_H];

static void moeda_girando_arte(float cx, float cy, float r, int tipo, int dono, float escx, int face)
{
    int caixa = (int)(r * 1.5f) + 4, x0 = (int)cx - caixa, y0 = (int)cy - caixa;
    for (int y = y0; y <= y0 + 2 * caixa; y++)
        for (int x = x0; x <= x0 + 2 * caixa; x++)
            if (x >= 0 && y >= 0 && x < GFX_W && y < GFX_H) rascunho[y * GFX_W + x] = CHAVE;
    gfx_desvia(rascunho);                              // o numero vai junto, achatado
    desenha_dado(cx, cy, r, tipo, dono, 0, face, 2, false);
    gfx_desvia(NULL);
    float e = fabsf(escx);
    if (e < 0.06f) e = 0.06f;
    uint16_t *fb = display_fb();
    for (int y = y0; y <= y0 + 2 * caixa; y++) {
        if (y < 0 || y >= GFX_H) continue;
        for (int x = (int)(cx - caixa * e); x <= (int)(cx + caixa * e); x++) {
            if (x < 0 || x >= GFX_W) continue;
            int sx = (int)lroundf(cx + (x - cx) / e);
            if (sx < x0 || sx > x0 + 2 * caixa || sx < 0 || sx >= GFX_W) continue;
            uint16_t c = rascunho[y * GFX_W + sx];
            if (c != CHAVE) fb[y * GFX_W + x] = c;
        }
    }
    if (fabsf(escx) < 0.35f)                           // a borda da moeda, de lado
        gfx_rect((int)cx - 1, (int)(cy - r), 3, (int)(2 * r), gfx_mistura(cor_aro(tipo), C_SOMBRA, 30));
}

static void estala(float forca)
{
    if (forca > 100 && t_fase - t_som > 0.03f) { som_toca(SOM_BATE); t_som = t_fase; }
}

// Um passo de fisica para todos os dados no ar. Devolve true quando todos
// pararam. Serve ao lancamento e a abertura.
static bool fisica(float dt)
{
    bool todos = true;
    for (int i = 0; i < n_voo; i++) {
        voo_t *v = &voo[i];
        if (v->pouso > 0) v->pouso -= dt;
        if (v->parado) continue;
        todos = false;
        v->x += v->vx * dt;
        v->y += v->vy * dt;
        v->giro += v->spin * dt;
        float atrito = 1.0f - 2.4f * dt;
        v->vx *= atrito; v->vy *= atrito; v->spin *= atrito;

        const float r = R_DADO + 2;
        if (v->x < MESA_X0 + r) { v->x = MESA_X0 + r; estala(fabsf(v->vx)); v->vx = -v->vx * 0.62f; v->spin = -v->spin; }
        if (v->x > MESA_X1 - r) { v->x = MESA_X1 - r; estala(fabsf(v->vx)); v->vx = -v->vx * 0.62f; v->spin = -v->spin; }
        if (v->y < MESA_Y0 + r) { v->y = MESA_Y0 + r; estala(fabsf(v->vy)); v->vy = -v->vy * 0.62f; }
        if (v->y > MESA_Y1 - r) { v->y = MESA_Y1 - r; estala(fabsf(v->vy)); v->vy = -v->vy * 0.62f; }

        float vel = sqrtf(v->vx * v->vx + v->vy * v->vy);
        v->t_face -= dt;
        if (v->t_face <= 0) {
            v->face = face_v(v->tipo, v->valor);
            v->t_face = 0.04f + (1.0f - vel / 680.0f) * 0.12f;
        }
        if (vel < 36 || t_fase > 6.0f) {
            v->parado = true;
            v->pouso = 0.18f;
            v->face = v->valor;
            som_toca(SOM_POUSA);
        }
    }
    // Dados se empurram quando se encontram.
    for (int a = 0; a < n_voo; a++)
        for (int b = a + 1; b < n_voo; b++) {
            voo_t *p = &voo[a], *q = &voo[b];
            float dx = q->x - p->x, dy = q->y - p->y;
            float d = sqrtf(dx * dx + dy * dy);
            const float D = 2 * R_DADO + 4;
            if (d >= D || d < 0.01f) continue;
            float nx = dx / d, ny = dy / d, meio = (D - d) / 2;
            p->x -= nx * meio; p->y -= ny * meio;
            q->x += nx * meio; q->y += ny * meio;
            float rv = (q->vx - p->vx) * nx + (q->vy - p->vy) * ny;
            if (rv < 0) {
                float imp = -rv * 0.8f;
                estala(imp);
                p->vx -= imp * nx; p->vy -= imp * ny;
                q->vx += imp * nx; q->vy += imp * ny;
                if (p->parado || q->parado) { p->parado = q->parado = false; }
            }
        }
    return todos;
}

// A mesa: trilho de madeira com filete de latao e o feltro, mais escuro nas
// bordas. Nao muda nunca: e pintada uma vez e copiada a cada quadro.
static uint16_t mesa_pronta[GFX_W * GFX_H];
static bool mesa_feita;

static uint32_t ruido_xy(int x, int y)
{
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Os andares do cassino: no Desafiante, cada dificuldade e um andar mais
// chique, e a mesa mostra isso nos acabamentos (0 = terreo, a mesa de sempre).
static int mesa_andar, mesa_andar_feito = -1;
#define C_RUBI     RGB(190, 56, 60)
#define C_DIAMANTE RGB(168, 204, 230)

// Tacha de metal: um botao com brilho em cima e sombra embaixo.
static void tacha(int x, int y, uint16_t m)
{
    gfx_disco(x + 1, y + 1, 2, C_SOMBRA);
    gfx_disco(x, y, 2, m);
    gfx_pixel(x - 1, y - 1, gfx_mistura(m, C_BRANCO, 55));
    gfx_pixel(x + 1, y + 1, gfx_mistura(m, C_SOMBRA, 45));
}

// Pedra lapidada em losango, com aro de ouro e um ponto de luz.
static void pedra(int x, int y, uint16_t g)
{
    for (int d = -4; d <= 4; d++) {
        int w = 4 - (d < 0 ? -d : d);
        gfx_rect(x - w, y + d, 2 * w + 1, 1, C_OURO);
    }
    for (int d = -3; d <= 3; d++) {
        int w = 3 - (d < 0 ? -d : d);
        gfx_rect(x - w, y + d, 2 * w + 1, 1, d < 0 ? gfx_mistura(g, C_BRANCO, 25) : g);
    }
    gfx_pixel(x - 1, y - 2, C_BRANCO);
}

// Filete fino no feltro, a `in` pixels da borda, com os cantos recortados
// para dentro (um quarto de circulo) nos andares de ouro em diante.
static void filete(int in, uint16_t c, bool recorte)
{
    int a = MESA_X0 + in, b = MESA_X1 - 1 - in, t = MESA_Y0 + in, u = MESA_Y1 - 1 - in;
    int r = recorte ? 7 : 0;
    gfx_rect(a + r, t, b - a - 2 * r + 1, 1, c);
    gfx_rect(a + r, u, b - a - 2 * r + 1, 1, c);
    gfx_rect(a, t + r, 1, u - t - 2 * r + 1, c);
    gfx_rect(b, t + r, 1, u - t - 2 * r + 1, c);
    if (!recorte) return;
    for (int i = 0; i <= 16; i++) {                     // quarto de circulo para dentro
        float q = i * 0.09817f;                         // 0 a 90 graus
        int dx = (int)(cosf(q) * r + 0.5f), dy = (int)(sinf(q) * r + 0.5f);
        gfx_pixel(a + dx, t + dy, c);
        gfx_pixel(b - dx, t + dy, c);
        gfx_pixel(a + dx, u - dy, c);
        gfx_pixel(b - dx, u - dy, c);
    }
}

static void pinta_mesa(void)
{
    int lv = mesa_andar;
    gfx_clear(C_BARRA);
    int x0 = 4, y0 = MESA_Y0 - 8, x1 = GFX_W - 4, y1 = MESA_Y1 + 8;
    // Do andar do rubi em diante, a madeira e mogno.
    uint16_t mad = lv >= 4 ? gfx_mistura(C_MADEIRA, RGB(92, 26, 30), 45) : C_MADEIRA;
    gfx_rect(x0, y0, x1 - x0, y1 - y0, mad);
    for (int y = y0; y < y1; y += 2)                    // veios da madeira
        for (int x = x0; x < x1; x++)
            if (ruido_xy(x / 7, y) % 5 == 0) gfx_pixel(x, y, gfx_mistura(mad, C_SOMBRA, 30));
    gfx_rect(x0, y0, x1 - x0, 1, C_MADEIRA_CLR);
    // O metal do andar: latao no terreo, bronze, prata, e ouro dali para cima.
    uint16_t metal = lv == 1 ? C_BRONZE : lv == 2 ? C_PRATA : lv >= 3 ? C_OURO : C_LATAO;
    gfx_moldura(MESA_X0 - 4, MESA_Y0 - 4, MESA_X1 - MESA_X0 + 8, MESA_Y1 - MESA_Y0 + 8, 2,
                lv >= 3 ? C_OURO : C_LATAO);
    gfx_moldura(MESA_X0 - 2, MESA_Y0 - 2, MESA_X1 - MESA_X0 + 4, MESA_Y1 - MESA_Y0 + 4, 2, C_SOMBRA);
    float cx = (MESA_X0 + MESA_X1) / 2.0f, cy = (MESA_Y0 + MESA_Y1) / 2.0f;
    float ax = (MESA_X1 - MESA_X0) / 2.0f, ay = (MESA_Y1 - MESA_Y0) / 2.0f;
    // O feltro escurece para as bordas em tres tons da paleta, com pontilhado
    // ordenado (Bayer 4x4) na passagem de um para o outro, e uma fibra ou
    // outra mais clara ou mais escura.
    static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
    static const uint16_t TOM[3] = { C_FELTRO, C_FELTRO_MEIO, C_FELTRO_ESC };
    for (int y = MESA_Y0; y < MESA_Y1; y++)
        for (int x = MESA_X0; x < MESA_X1; x++) {
            float dx = (x - cx) / ax, dy = (y - cy) / ay;
            float v = (dx * dx * 0.6f + dy * dy) * 1.5f - 0.15f;
            if (v < 0) v = 0;
            int i = (int)v;
            if (i >= 2) i = 2;
            else if ((v - i) * 16 > BAYER[y & 3][x & 3]) i++;
            uint16_t c = TOM[i];
            uint32_t h = ruido_xy(x, y) % 23;
            if (h == 0) c = i == 0 ? C_FELTRO2 : TOM[i - 1];
            else if (h == 1 && i < 2) c = TOM[i + 1];
            gfx_pixel(x, y, c);
        }
    // Linha de aposta: uma elipse tracejada ao redor do centro da mesa.
    uint16_t linha = lv >= 3 ? gfx_mistura(C_FELTRO2, C_OURO, 30) : C_FELTRO2;
    for (int i = 0; i < 360; i++) {
        if ((i / 4) % 2) continue;
        float a = i * 0.017453f;
        gfx_rect((int)(cx + cosf(a) * 250) , (int)(cy + sinf(a) * 52), 2, 1, linha);
    }
    if (lv <= 0) return;

    // Acabamentos do andar, discretos: so nas bordas.
    if (lv >= 2) filete(6, gfx_mistura(C_FELTRO, metal, 38), lv >= 3);
    if (lv >= 5) filete(9, gfx_mistura(C_FELTRO, metal, 22), true);
    int fx0 = MESA_X0 - 3, fx1 = MESA_X1 + 2, fy0 = MESA_Y0 - 3, fy1 = MESA_Y1 + 2;
    int my = (MESA_Y0 + MESA_Y1) / 2;
    // Nos cantos da moldura: tachas; no rubi e no diamante, pedras.
    uint16_t gema = lv == 4 ? C_RUBI : C_DIAMANTE;
    const int CX[4] = { fx0, fx1, fx0, fx1 }, CY[4] = { fy0, fy0, fy1, fy1 };
    for (int i = 0; i < 4; i++) {
        if (lv >= 4) pedra(CX[i], CY[i], gema);
        else tacha(CX[i], CY[i], metal);
    }
    // No meio das bordas (prata em diante): mais tachas; no rubi e no
    // diamante, uma pedra no meio de cima e de baixo.
    if (lv >= 2) {
        for (int k = 1; k <= 3; k++) {
            int x = MESA_X0 + (MESA_X1 - MESA_X0) * k / 4;
            if (lv >= 4 && k == 2) { pedra(x, fy0, gema); pedra(x, fy1, gema); continue; }
            tacha(x, fy0, metal); tacha(x, fy1, metal);
        }
        if (lv >= 4) { pedra(fx0, my, gema); pedra(fx1, my, gema); }
        else { tacha(fx0, my, metal); tacha(fx1, my, metal); }
    }
}

static int des_dificuldade(void);

static void mesa(void)
{
    mesa_andar = modo == M_DESAFIO ? des_dificuldade() : 0;
    if (mesa_andar != mesa_andar_feito) { mesa_feita = false; mesa_andar_feito = mesa_andar; }
    if (!mesa_feita) {
        pinta_mesa();
        memcpy(mesa_pronta, display_fb(), sizeof mesa_pronta);
        mesa_feita = true;
    }
    memcpy(display_fb(), mesa_pronta, sizeof mesa_pronta);
}

static void ficha(int x, int y, uint16_t c)
{
    gfx_disco(x + 1, y + 2, 7, C_SOMBRA);
    gfx_disco(x, y, 7, c);
    for (int i = 0; i < 6; i++) {                        // listras da borda
        float a = i * 1.0472f;
        gfx_rect(x + (int)(cosf(a) * 5.5f) - 1, y + (int)(sinf(a) * 5.5f) - 1, 2, 2, C_MARFIM);
    }
    gfx_disco(x, y, 3, gfx_mistura(c, C_BARRA, 30));
}

// ===========================================================================
// A regra da fila
// ===========================================================================
static int ultimo_valido(const fila_t *f, const uint8_t *est, int antes)
{
    for (int k = antes - 1; k >= 0; k--)
        if (est[k] == V_VALIDO) return k;
    return -1;
}

// Soma dos validos. O Tudo ou Nada valido com 2 dobra tudo no fim.
// O que o dado vale no placar. So o Trevo difere do que saiu: 4 vira 8, o
// resto vira 0. A queda da fila sempre compara o que saiu (F.valor).
static int valor_pontos(int j, int k)
{
    if (F[j].tipo[k] == D_TREVO) return F[j].valor[k] == 4 ? 8 : 0;
    if (F[j].tipo[k] == D_FERREIRO) return 0;
    return F[j].valor[k];
}

// Nao anulavel: nada o anula - nem a queda da fila, nem o Agouro, nem o
// Egoista, nem Carrasco ou Invejoso. Sao o Escudo, o Teimoso e o Egoista.
// Intocavel: alem disso, nenhum ataque o alcanca (nao pode ser roubado).
// So o Escudo.
static bool nao_anulavel_t(int t)
{
    int ef = TIPO[t].efeito;
    return ef == EF_ESCUDO || ef == EF_TEIMOSO;
}
static bool intocavel_t(int t) { return TIPO[t].efeito == EF_ESCUDO; }
static bool firme(int j, int k) { return nao_anulavel_t(tipo_de(j, k)); }
// Quem nao cai quando o Egoista chega: os nao anulaveis e outro Egoista.
static bool resiste_egoista(int t) { return nao_anulavel_t(t) || TIPO[t].efeito == EF_EGOISTA; }

// O maior dado do rival de j, para o Espelho: entre os que valem; se nenhum
// vale, o maior que ele jogou. 0 se o rival ainda nao jogou nada.
static int maior_do_rival(int j)
{
    const fila_t *r = &F[outro(j)];
    int vale = 0, qualquer = 0;
    for (int i = 0; i < r->lancados; i++) {
        if (r->valor[i] > qualquer) qualquer = r->valor[i];
        if (r->est[i] == V_VALIDO && r->valor[i] > vale) vale = r->valor[i];
    }
    return vale ? vale : qualquer;
}
// Algum Egoista valendo antes do dado k?
static int egoista_antes(int j, int k)
{
    for (int i = 0; i < k; i++)
        if (F[j].est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_EGOISTA) return i;
    return -1;
}
// O Maldito entra na fila, mas o valor dele nao soma para quem o jogou.
// Todo dado que vale soma para quem o jogou (o Maldito tambem, agora).
static bool pontua(int j, int k) { (void)j; (void)k; return true; }

// Soma ao vivo: valor mais bonus de Par. Multiplicadores e Moeda da sorte
// so entram no fim da rodada.
static int soma(int j, const uint8_t *est)
{
    const fila_t *f = &F[j];
    if (f->zerada) return 0;
    int t = 0;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO && pontua(j, k)) t += valor_pontos(j, k) + f->bonus[k];
    return t;
}


static void anula_fila(int j)
{
    for (int k = 0; k < F[j].lancados; k++)
        if (F[j].est[k] == V_VALIDO && !firme(j, k)) F[j].est[k] = V_ANULADO;
}

// Copiador valendo antes de k, com o mesmo numero que k tirou?
static bool copia_antes(int j, int k)
{
    for (int i = 0; i < k; i++)
        if (F[j].est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_COPIADOR
            && F[j].valor[i] == F[j].valor[k]) return true;
    return false;
}

// Tres ou mais dados valendo com o mesmo numero. Devolve o numero (0 se nao
// ha trinca) e, em masc, quais dados formam a trinca.
static int trinca(int j, const uint8_t *est, uint8_t *masc)
{
    for (int a = 0; a < F[j].lancados; a++) {
        if (est[a] != V_VALIDO) continue;
        uint8_t m = 0;
        for (int b = 0; b < F[j].lancados; b++)
            if (est[b] == V_VALIDO && F[j].valor[b] == F[j].valor[a]) m |= (uint8_t)(1 << b);
        int n = 0;
        for (uint8_t x = m; x; x >>= 1) n += x & 1;
        if (n >= 3) { if (masc) *masc = m; return F[j].valor[a]; }
    }
    return 0;
}

static bool tem_jackpot(int j, const uint8_t *est)
{
    for (int k = 0; k < F[j].lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_JACKPOT) return true;
    return false;
}

// Aplica a regra ao dado k que acabou de pousar. O veredito e curto: diz o
// que aconteceu, nao o porque em detalhe.
static void aplica_regra(int j, int k)
{
    if (k < 0 || k >= N_FILA) return;
    fila_t *f = &F[j];
    int t = tipo_de(j, k), ef = TIPO[t].efeito;
    int l = ultimo_valido(f, f->est, k);
    int v = f->valor[k];
    anulou_a = anulou_b = -1;
    anulou_masc = 0;
    resistiu = -1;
    bool espelhou = false, espera = false;

    // Espelho caido com 1 ou 2 vira sempre o maior dado do rival. Se o rival
    // ainda nao jogou, espera: vira no calculo do fim da rodada, e ate la
    // vale como esta, sem cair.
    if (ef == EF_ESPELHO && v <= 2) {
        int alvo = maior_do_rival(j);
        if (alvo > 0) {
            v = f->valor[k] = alvo;
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v);
            espelhou = true;
        } else {
            espera = true;
            f->espera[k] = true;
            snprintf(f->tag[k], sizeof f->tag[k], "=?");
        }
    }

    // Gemeo: tira o mesmo do ultimo dado que vale. Prensa (amuleto): 1 vira 3.
    if (ef == EF_GEMEO && l >= 0) {
        v = f->valor[k] = f->valor[l];
        snprintf(f->tag[k], sizeof f->tag[k], "=%d", v);
    }
    if (v == 1 && tem_am(j, A_PRENSA) && ef != EF_VIDRO && ef != EF_TUDO_NADA && ef != EF_ESPELHO
        && ef != EF_TREVO && ef != EF_BUMERANGUE) {
        v = f->valor[k] = 3;
        snprintf(f->tag[k], sizeof f->tag[k], "=3");
        am_disparou |= 1u << A_PRENSA;
    }

    // Vidro com 1: se estilhaca na hora. Fica anulado e derruba o dado de
    // antes, como na queda; no fim da rodada sai da bolsa.
    if (ef == EF_VIDRO && v == 1) {
        f->est[k] = V_ANULADO;
        anulou_b = k;
        if (l >= 0 && !firme(j, l)) {             // e derruba o anterior, como na queda
            f->est[l] = V_ANULADO;
            anulou_a = l;
            snprintf(veredito, sizeof veredito, "o Vidro se estilhaçou e anulou o %d", f->valor[l]);
        } else {
            if (l >= 0) resistiu = l;
            snprintf(veredito, sizeof veredito, "o Vidro tirou 1 e se estilhaçou");
        }
        ver_tipo = 1;
        f->pontos = soma(j, f->est);
        return;
    }

    // Egoista na fila: nenhum dado novo vale ao lado dele (menos o Escudo).
    int ego = egoista_antes(j, k);
    if (ego >= 0 && !resiste_egoista(t)) {
        f->est[k] = V_ANULADO;
        anulou_b = k;
        resistiu = ego;
        snprintf(veredito, sizeof veredito, "o Egoísta não divide a mesa: %d anulado", v);
        ver_tipo = 4;
        f->pontos = soma(j, f->est);
        return;
    }
    // Egoista chegando: anula todos os outros que valiam, e vale sozinho.
    if (ef == EF_EGOISTA) {
        uint8_t *est = f->est;
        est[k] = V_VALIDO;
        int n = 0;
        for (int i = 0; i < k; i++)
            if (est[i] == V_VALIDO && !resiste_egoista(f->tipo[i])) {
                est[i] = V_ANULADO;
                anulou_masc |= (uint8_t)(1 << i);
                n++;
            }
        if (n) snprintf(veredito, sizeof veredito, "Egoísta tirou %d e anulou %d dado%s", v, n, n > 1 ? "s" : "");
        else   snprintf(veredito, sizeof veredito, "Egoísta vale %d, sozinho", v);
        ver_tipo = n ? 4 : 0;
        // Ele nao se anula, mas pode cair: se sobrou antes dele um dado que
        // resiste (Escudo, Teimoso) maior que ele, a queda da sequencia vale.
        int l2 = ultimo_valido(f, est, k);
        if (l2 >= 0 && v < f->valor[l2]) {
            bool esc_l = firme(j, l2);
            if (!esc_l) { est[l2] = V_ANULADO; anulou_a = l2; }
            est[k] = V_ANULADO;
            anulou_b = k;
            resistiu = esc_l ? l2 : -1;
            snprintf(veredito, sizeof veredito, "%d < %d: o Egoísta cai", v, f->valor[l2]);
            ver_tipo = esc_l ? 2 : 1;
        }
        f->pontos = soma(j, f->est);
        return;
    }

    bool condenou = ef == EF_TUDO_NADA && v == 1 && !f->zerada;
    if (ef == EF_TUDO_NADA && v == 1) f->zerada = true;
    if (ef == EF_MALDITO && v <= 7) {          // Agouro: 7 ou menos derruba a sequencia
        anula_fila(j);
        f->est[k] = V_ANULADO;
        anulou_a = k;
        // Escudo, Teimoso e Egoista seguram: a mensagem nao diz que zerou tudo.
        int segura = -1;
        for (int i = 0; i < k; i++) if (f->est[i] == V_VALIDO) { segura = i; break; }
        if (segura >= 0) {
            resistiu = segura;
            snprintf(veredito, sizeof veredito, "%s zerou a sequência; %s segura", TIPO[t].nome,
                     TIPO[tipo_de(j, segura)].nome);
        } else
            snprintf(veredito, sizeof veredito, "%s tirou %d e zerou a sequência", TIPO[t].nome, v);
        ver_tipo = 3;
        f->pontos = soma(j, f->est);
        return;
    }

    if (!espera && l >= 0 && v < f->valor[l]) {
        // A Sentinela cai sozinha: o dado de antes fica de pe. A Luva de
        // Couro segura o primeiro da sequencia; a Rede, o de antes do ultimo.
        bool sentinela = ef == EF_SENTINELA;
        bool luva = l == 0 && tem_am(j, A_LUVA) && !firme(j, l);
        bool rede = k == f->n - 1 && tem_am(j, A_REDE) && !firme(j, l) && !sentinela && !luva;
        // Ferradura: na primeira queda da rodada, o dado de antes fica.
        bool ferr = tem_am(j, A_FERRADURA) && (ferr_k[j] < 0 || ferr_k[j] == k) && !firme(j, l)
                    && !sentinela && !luva && !rede;
        if (luva) am_disparou |= 1u << A_LUVA;
        if (rede) am_disparou |= 1u << A_REDE;
        if (ferr) am_disparou |= 1u << A_FERRADURA;
        bool esc_l = firme(j, l) || sentinela || luva || rede || ferr, esc_k = firme(j, k);
        if (!esc_l) f->est[l] = V_ANULADO;
        f->est[k] = esc_k ? V_VALIDO : V_ANULADO;
        anulou_a = esc_l ? -1 : l;
        anulou_b = esc_k ? -1 : k;
        resistiu = esc_l ? l : (esc_k ? k : -1);
        if (ferr && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Ferradura segura o %d", v, f->valor[l], f->valor[l]);
        else if (sentinela && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Sentinela cai sozinha", v, f->valor[l]);
        else if (luva && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Luva de Couro segura o %d", v, f->valor[l], f->valor[l]);
        else if (rede && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Rede segura o %d", v, f->valor[l], f->valor[l]);
        else if (esc_l) snprintf(veredito, sizeof veredito, "%d < %d: %s segura", v, f->valor[l], TIPO[tipo_de(j, l)].nome);
        else if (esc_k) snprintf(veredito, sizeof veredito, "%d < %d: %s fica, %d cai", v, f->valor[l], TIPO[t].nome, f->valor[l]);
        else            snprintf(veredito, sizeof veredito, "%d é menor que %d: anulou os dois", v, f->valor[l]);
        ver_tipo = (esc_l || esc_k) ? 2 : 1;
    } else {
        f->est[k] = V_VALIDO;
        if (ef == EF_PAR && l >= 0 && v == f->valor[l]) {
            f->bonus[k] = 8;
            snprintf(f->tag[k], sizeof f->tag[k], "+8");
        }
        // Dobro: so o dado logo depois dele na sequencia (com o Dobro valendo).
        if (l >= 0 && l == k - 1 && TIPO[tipo_de(j, l)].efeito == EF_DOBRO && f->valor[l] <= 3) {
            f->dobra[k] = 1;
            snprintf(f->tag[k], sizeof f->tag[k], "x2");
        }
        // Ferreiro com 3 ou 4 valendo logo antes: +6 no fim, se os dois ficarem.
        if (l >= 0 && l == k - 1 && TIPO[tipo_de(j, l)].efeito == EF_FERREIRO && f->valor[l] >= 3)
            snprintf(f->tag[k], sizeof f->tag[k], "+6");
        if ((ef == EF_COBRADOR && fichas_ini[outro(j)] > fichas_ini[j])     // atras: o Cobrador cobra
            || (ef == EF_FICHAS && fichas_ini[j] > fichas_ini[outro(j)])) {   // na frente: o Fichas rende
            f->bonus[k] = 2;
            snprintf(f->tag[k], sizeof f->tag[k], "$+2");
        }
        if (ef == EF_ACUMULADOR && f->fixo[k]) {
            f->bonus[k] = (int8_t)(f->fixo[k] < 6 ? f->fixo[k] : 6);
            snprintf(f->tag[k], sizeof f->tag[k], "+%d", f->bonus[k]);
        }
        if (ef == EF_FERREIRO) snprintf(f->tag[k], sizeof f->tag[k], "=0");
        // Amuletos: Dado de Ouro (+2 no numero maximo), Peso Pena (+1 nos de
        // 2 e 4 lados).
        int am_b = 0;
        if (tem_am(j, A_OURO) && v == TIPO[t].lados && ef != EF_ESPELHO) { am_b += 2; am_disparou |= 1u << A_OURO; }
        if (tem_am(j, A_PENA) && (TIPO[t].lados == 2 || TIPO[t].lados == 4)) { am_b += 1; am_disparou |= 1u << A_PENA; }
        if (tem_am(j, A_PESADO) && TIPO[t].lados == 6) { am_b += 1; am_disparou |= 1u << A_PESADO; }
        if (tem_am(j, A_CONTRATO)) { am_b += 2; am_disparou |= 1u << A_CONTRATO; }   // Contrato Sujo
        if (tem_am(j, A_COROA) && fichas_ini[j] < fichas_ini[outro(j)]) { am_b += 1; am_disparou |= 1u << A_COROA; }
        if (j == 1 && rodada >= 5 && tem_reg(REG_FOLEGO)) am_b += 1;              // a Maratonista
        if (j == 1 && k == 0 && tem_reg(REG_SAQUE)) am_b += 1;                    // o Caubói
        if (j == 1 && virada_forca(0)) am_b += 1;                                 // a casa tem vantagem
        if (k == 0 && tem_am(j, A_CORINGA)) { am_b += 2; am_disparou |= 1u << A_CORINGA; }
        if (am_b) {
            f->bonus[k] = (int8_t)(f->bonus[k] + am_b);
            snprintf(f->tag[k], sizeof f->tag[k], "+%d", f->bonus[k]);
        }
        int vale = valor_pontos(j, k) + f->bonus[k];
        if (ef == EF_TREVO) {
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v == 4 ? 8 : 0);
            if (v == 4) snprintf(veredito, sizeof veredito, "SORTE! o Trevo vale 8");
            else        snprintf(veredito, sizeof veredito, "Trevo tirou %d: vale 0", v);
        } else if (ef == EF_MALDICAO) {
            snprintf(veredito, sizeof veredito, "Maldito vale %d e tira %d do oponente", v, v);
        } else if (ef == EF_COPIADOR) {
            snprintf(veredito, sizeof veredito, "Copiador tirou %d: cada %d seu ganha +2", v, v);
        } else if (copia_antes(j, k)) {
            snprintf(f->tag[k], sizeof f->tag[k], "+2");
            snprintf(veredito, sizeof veredito, "vale %d, igual ao Copiador: +2", vale);
        } else if (ef == EF_FERREIRO) {
            if (v >= 3) snprintf(veredito, sizeof veredito, "Ferreiro: não pontua, forja +6 no próximo");
            else        snprintf(veredito, sizeof veredito, "Ferreiro tirou %d: não pontua nem forja", v);
        } else if (ef == EF_COBRADOR && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Cobrador cobra: vale %d (+2)", vale);
        } else if (ef == EF_FICHAS && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Fichas rende: vale %d (+2)", vale);
        } else if (ef == EF_ACUMULADOR && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Acumulador vale %d (+%d guardado)", vale, f->bonus[k]);
        } else if (espelhou)          snprintf(veredito, sizeof veredito, "Espelho vira %d e vale", v);
        else if (espera)       snprintf(veredito, sizeof veredito, "Espelho espera: no fim vira o maior do oponente");
        else if (f->dobra[k])  snprintf(veredito, sizeof veredito, "vale %d, x2 no fim", vale);
        else if (f->bonus[k])  snprintf(veredito, sizeof veredito, "vale %d (Par)", vale);
        else if (l < 0 && k)   snprintf(veredito, sizeof veredito, "vale %d: começa sequência nova", vale);
        else                   snprintf(veredito, sizeof veredito, "vale %d", vale);
        ver_tipo = 0;
    }
    if (condenou) {                              // o aviso vale mais que o resto
        snprintf(veredito, sizeof veredito, "Tudo ou Nada deu 1: a rodada vale 0");
        ver_tipo = 3;
    } else if (ef == EF_VICIADO && f->descarte[k] && ver_tipo == 0 && !f->tag[k][0]) {
        snprintf(veredito, sizeof veredito, "Viciado: %d e %d, fica o %d", v, f->descarte[k], v);
    } else if (f->est[k] == V_VALIDO && trinca(j, f->est, NULL) && tem_jackpot(j, f->est)) {
        snprintf(veredito, sizeof veredito, "três iguais: JACKPOT no fim!");
        ver_tipo = 0;
    }
    f->pontos = soma(j, f->est);
}

static int sorteia_valor(int t, int fixo, int (*r)(int), int *menor);

// Refaz a sequencia de j do comeco ate o dado n (sem ele), com os valores que
// estao la: a regra da queda, o Egoista, o Tudo ou Nada... tudo de novo.
// Serve a Ventania (que rolou o de antes) e ao Espelho que virou no fim.
static void refaz_sequencia(int j, int n)
{
    fila_t *f = &F[j];
    for (int i = 0; i < n; i++) {
        f->est[i] = V_ESPERA;
        f->bonus[i] = 0;
        f->dobra[i] = 0;
        f->tag[i][0] = 0;
    }
    f->zerada = false;
    int lanc = f->lancados;
    for (int i = 0; i < n; i++) { f->lancados = i + 1; aplica_regra(j, i); }
    f->lancados = lanc;
}

// Um dado acabou de cair na posicao k. O Ventania, antes da regra dele, rola
// de novo o dado lancado logo antes dele e refaz a sequencia do comeco com o
// valor novo (dados podem cair ou voltar a valer). Nao rola a si mesmo nem os
// outros. 'r' e o sorteio: o da partida ou o da IA imaginando jogadas.
static void pousa(int j, int k, int (*r)(int))
{
    fila_t *f = &F[j];
    bool voltou = false;
    int tirou = f->valor[k];
    if (f->tipo[k] == D_BUMERANGUE && tirou <= 2) {          // volta e rola de novo, uma vez
        f->cru[k] = f->valor[k] = r(6);
        voltou = true;
    }
    if (f->tipo[k] != D_VENTANIA || k == 0) {
        aplica_regra(j, k);
        if (voltou && ver_tipo == 0) snprintf(veredito, sizeof veredito, "o Bumerangue voltou: %d", f->valor[k]);
        if (voltou) snprintf(f->tag[k], sizeof f->tag[k], "%d>%d", tirou, f->valor[k]);
        return;
    }
    int i = k - 1;
    if (!(f->tipo[i] == D_VIDRO && f->cru[i] == 1)) {           // vidro quebrado nao volta
        int menor;
        f->cru[i] = f->valor[i] = sorteia_valor(f->tipo[i], f->fixo[i], r, &menor);
        f->descarte[i] = (uint8_t)menor;
        f->espera[i] = false;
    }
    refaz_sequencia(j, k);
    aplica_regra(j, k);
    if (f->est[k] == V_VALIDO && ver_tipo == 0)
        snprintf(veredito, sizeof veredito, "Ventania! o dado de antes rolou: %d", f->valor[i]);
}

// ===========================================================================
// Desfecho: poderes que mexem no rival, em sequencia
// ===========================================================================
typedef struct {
    uint8_t est[2][N_FILA];
    int8_t  bonus[2][N_FILA];        // Moeda da sorte
    int8_t  mult[2][N_FILA];         // soma dos multiplicadores (0 = nenhum)
    uint8_t queima[2][N_FILA];       // alvos da Ficha de Fogo
    int semente[2];
    uint8_t zerada[2];
    int roubo_val[2];
    int delta[2];
    int total[2];
    int bruto[2];                    // o total antes de travar em 0 (a previa mostra negativo)
} desfecho_t;

static int contribui(int j, int k)
{
    if (!pontua(j, k)) return 0;
    return (valor_pontos(j, k) + F[j].bonus[k]) * (F[j].dobra[k] ? 2 : 1);
}

// Alvo de um ataque na fila de j. maior = true: o maior, para ANULAR (Carrasco,
// Invejoso) - os nao anulaveis ficam de fora. maior = false: o menor, para
// ROUBAR (Caridoso, Gatuno) - so o intocavel fica de fora.
static int alvo_de(const uint8_t *est, int j, bool maior, int exceto)
{
    int a = -1;
    if (!maior && tem_am(j, A_IMA)) return -1;      // Ima: ninguem rouba
    for (int i = 0; i < F[j].lancados; i++) {
        if (i == exceto || est[i] != V_VALIDO) continue;
        int t = tipo_de(j, i);
        if (maior ? nao_anulavel_t(t) : intocavel_t(t)) continue;
        if (a < 0 || (maior ? F[j].valor[i] > F[j].valor[a] : F[j].valor[i] < F[j].valor[a]))
            a = i;
    }
    return a;
}

// Seguranca: um ataque ao dado a da fila de j cai no Seguranca dele, se houver
// um valendo (e nao for o proprio alvo).
static int desvia_seguranca(const uint8_t *est, int j, int a)
{
    if (a < 0) return a;
    for (int i = 0; i < F[j].lancados; i++)
        if (i != a && est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_SEGURANCA) return i;
    return a;
}

static void evento(bool gera, int tipo, int dj, int dk, int aj, int ak, int valor,
                   const char *fmt, ...) __attribute__((format(printf, 8, 9)));
static void evento(bool gera, int tipo, int dj, int dk, int aj, int ak, int valor,
                   const char *fmt, ...)
{
    if (!gera || n_ev >= MAX_EV) return;
    evento_t *e = &ev[n_ev++];
    memset(e, 0, sizeof *e);
    e->tipo = (uint8_t)tipo; e->de_j = (uint8_t)dj; e->de_k = (uint8_t)dk;
    e->alvo_j = (uint8_t)aj; e->alvo_k = (uint8_t)ak; e->valor = valor;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(e->txt, sizeof e->txt, fmt, ap);
    va_end(ap);
}

// Pontos de uma fila dados o estado, os bonus e os multiplicadores do fim.
// Multiplicadores se SOMAM (x2, x3, x2 = x7) e valem sobre valor + bonus.
static int pontos_fila(int j, const uint8_t *est, const int8_t *bonus, const int8_t *mult)
{
    int t = 0;
    for (int k = 0; k < F[j].lancados; k++) {
        if (est[k] != V_VALIDO || !pontua(j, k)) continue;
        int base = valor_pontos(j, k) + F[j].bonus[k] + (bonus ? bonus[k] : 0);
        int m = (mult && mult[k] > 0) ? mult[k] : 1;
        t += base * m;
    }
    return t;
}

// Efeitos do fim sobre a propria fila: Moeda da sorte, depois os
// multiplicadores. Decididos sobre o estado depois dos ataques.
static void efeitos_do_fim(desfecho_t *d, int j, bool gera)
{
    const fila_t *f = &F[j];
    const uint8_t *est = d->est[j];

    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_SORTE) continue;
        int face = f->valor[k];
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        for (int i = 0; i < f->lancados; i++) {
            if (est[i] != V_VALIDO || !pontua(j, i)) continue;
            bool impar = f->valor[i] % 2 == 1;
            if (face == 1 && impar)  { d->bonus[j][i] += 1; mask |= 1 << i; por[i] = 1; }
            if (face == 2 && !impar) { d->bonus[j][i] += 1; mask |= 1 << i; por[i] = 1; }
        }
        if (mask && gera && n_ev < MAX_EV) {
            evento_t *e = &ev[n_ev++];
            memset(e, 0, sizeof *e);
            e->tipo = EV_BONUS; e->de_j = (uint8_t)j; e->de_k = (uint8_t)k;
            e->mascara = mask;
            memcpy(e->por_dado, por, sizeof por);
            snprintf(e->txt, sizeof e->txt, face == 1 ? "Moeda 1: +1 nos ímpares"
                                                      : "Moeda 2: +1 nos pares");
        }
    }

    // Copiador: +2 em cada outro dado seu com o mesmo numero que ele.
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_COPIADOR) continue;
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        int nl = f->lancados < N_FILA ? f->lancados : N_FILA;
        for (int i = 0; i < nl; i++) {
            if (i == k || est[i] != V_VALIDO || !pontua(j, i) || f->valor[i] != f->valor[k]) continue;
            d->bonus[j][i] += 2; mask |= (uint8_t)(1 << i); por[i] = 2;
        }
        if (mask && gera && n_ev < MAX_EV) {
            evento_t *e = &ev[n_ev++];
            memset(e, 0, sizeof *e);
            e->tipo = EV_BONUS; e->de_j = (uint8_t)j; e->de_k = (uint8_t)k;
            e->mascara = mask;
            memcpy(e->por_dado, por, sizeof por);
            snprintf(e->txt, sizeof e->txt, "Copiador %d: +2 em cada %d", f->valor[k], f->valor[k]);
        }
    }

    // Dupla (amuleto): +2 em cada dado seu que repete o numero de outro seu.
    if (tem_am(j, A_DUPLA)) {
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        int nl = f->lancados < N_FILA ? f->lancados : N_FILA;
        for (int k = 0; k < nl; k++) {
            if (est[k] != V_VALIDO || !pontua(j, k)) continue;
            for (int i = 0; i < nl; i++)
                if (i != k && est[i] == V_VALIDO && f->valor[i] == f->valor[k]) {
                    d->bonus[j][k] += 2; mask |= (uint8_t)(1 << k); por[k] = 2;
                    break;
                }
        }
        if (mask && gera && n_ev < MAX_EV) {
            evento_t *e = &ev[n_ev++];
            memset(e, 0, sizeof *e);
            e->tipo = EV_BONUS; e->de_j = (uint8_t)j; e->de_k = 0xFF;
            e->mascara = mask;
            memcpy(e->por_dado, por, sizeof por);
            snprintf(e->txt, sizeof e->txt, "Dupla: +2 nos números repetidos");
        }
    }

    // Ferreiro com 3 ou 4: +6 no dado logo depois dele, se os dois terminam valendo.
    for (int k = 0; k + 1 < f->lancados && k + 1 < N_FILA; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_FERREIRO || f->valor[k] < 3) continue;
        if (est[k + 1] != V_VALIDO || !pontua(j, k + 1)) continue;
        d->bonus[j][k + 1] += 6;
        if (gera && n_ev < MAX_EV) {
            evento_t *e = &ev[n_ev++];
            memset(e, 0, sizeof *e);
            e->tipo = EV_BONUS; e->de_j = (uint8_t)j; e->de_k = (uint8_t)k;
            e->mascara = (uint8_t)(1 << (k + 1));
            e->por_dado[k + 1] = 6;
            snprintf(e->txt, sizeof e->txt, "Ferreiro forja +6 no %d", f->valor[k + 1]);
        }
    }

    // Multiplicadores: Dobro (no dado valido logo acima), Fartura (nos de
    // valor 1 e 2), Tudo ou Nada com 2 (em todos).
    bool fartura = false, tudo = false;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO) continue;
        int ef = TIPO[tipo_de(j, k)].efeito;
        if (ef == EF_FARTURA) fartura = true;
        if (ef == EF_TUDO_NADA && f->valor[k] == 2) tudo = true;
    }
    int n_far = 0;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_FARTURA) n_far++;
    // Ficha de Fogo: +10, e o maior dado seu que pontua (sem contar ela)
    // queima no fim. Duas fichas queimam dois dados diferentes.
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_FOGO) continue;
        int a = -1;
        for (int i = 0; i < f->lancados; i++) {
            if (i == k || est[i] != V_VALIDO || !pontua(j, i) || d->queima[j][i]) continue;
            if (TIPO[tipo_de(j, i)].efeito == EF_FOGO) continue;
            if (a < 0 || f->valor[i] > f->valor[a]) a = i;
        }
        if (a < 0) continue;
        d->queima[j][a] = 1;
        d->semente[j] += 10;
        evento(gera, EV_FOGO, j, k, j, a, 10, "Fogo: +10, e o %d vira cinza", f->valor[a]);
    }

    uint8_t mask = 0;
    int sozinhos = 0;                            // dados valendo: a Viuva quer ser a unica
    for (int k = 0; k < f->lancados; k++) sozinhos += est[k] == V_VALIDO;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || !pontua(j, k)) continue;
        int m = 0;
        // Dobro: vale so para o dado logo depois dele, e so se os dois
        // terminarem valendo. Anulado o seguinte, o x2 se perde; nao passa
        // adiante para o proximo que vale.
        // E so se o Dobro tirou de 1 a 3.
        if (k > 0 && est[k - 1] == V_VALIDO && TIPO[tipo_de(j, k - 1)].efeito == EF_DOBRO
            && f->valor[k - 1] <= 3) m += 2;
        if (fartura && f->valor[k] <= 2) m += 2 * n_far;
        if (tudo) m += 2;
        if (TIPO[tipo_de(j, k)].efeito == EF_TRONO && k == f->lancados - 1) m += 2;   // Trono
        if (TIPO[tipo_de(j, k)].efeito == EF_VIUVA && sozinhos == 1) m += 2;          // Viuva Negra sozinha
        if (j == 1 && virada_forca(2) && (k == 0 || (virada_forca(2) == 2 && k == 1)))
            m += 2;                              // o Barao virado: x2 no primeiro (e no segundo, na Cobertura)
        if (m) { d->mult[j][k] = (int8_t)m; mask |= 1 << k; }
    }
    if (mask && gera && n_ev < MAX_EV) {
        evento_t *e = &ev[n_ev++];
        memset(e, 0, sizeof *e);
        e->tipo = EV_MULT; e->de_j = (uint8_t)j; e->de_k = 0xFF;
        e->mascara = mask;
        memcpy(e->por_dado, d->mult[j], N_FILA);
        int antes = pontos_fila(j, d->est[j], d->bonus[j], NULL);
        int depois = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]);
        snprintf(e->txt, sizeof e->txt, "multiplicadores: %d vira %d", antes, depois);
    }

    // Semente: se os dados deste jogador (com bonus, multiplicadores e os que
    // roubou) somam 20 ou mais, +10 no fim - fora de qualquer multiplicador.
    int dados = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j];
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_SEMENTE) continue;
        if (dados < 20) continue;
        d->semente[j] += 10;
        evento(gera, EV_SEMENTE, j, k, j, 0, 10, "Semente: %d pontos, brota +10", dados);
    }

    // A virada do Barao: +2 para ele por dado seu anulado na rodada.
    if (j == 1 && virada_forca(2)) {
        int caidos = 0;
        for (int k = 0; k < F[0].lancados; k++) if (d->est[0][k] == V_ANULADO) caidos++;
        if (caidos) {
            d->semente[1] += VIRADA_BARAO * caidos;
            evento(gera, EV_CHEFE, 1, 0xFF, 1, 0, VIRADA_BARAO * caidos, "%s: %d dado%s seu%s caiu, +%d", J[1].nome,
                   caidos, caidos > 1 ? "s" : "", caidos > 1 ? "s" : "", VIRADA_BARAO * caidos);
        }
    }
    // Bonus fixos, fora dos multiplicadores (no mesmo lugar da Semente):
    // Misericordioso, Solitario, Prisma, Desafiante e Cavalo de Troia.
    int anulados = 0, valendo = 0;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] == V_ANULADO) anulados++;
        if (est[k] == V_VALIDO) valendo++;
    }
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO) continue;
        int ef = TIPO[tipo_de(j, k)].efeito;
        if (ef == EF_MISERICORDIA && anulados) {
            d->semente[j] += 2 * anulados;
            evento(gera, EV_EXTRA, j, k, j, 0, 2 * anulados, "Misericordioso: %d anulado%s, +%d",
                   anulados, anulados > 1 ? "s" : "", 2 * anulados);
        } else if (ef == EF_SOLITARIO && !(k > 0 && est[k - 1] == V_VALIDO)
                   && !(k + 1 < f->lancados && est[k + 1] == V_VALIDO)) {
            d->semente[j] += 4;
            evento(gera, EV_EXTRA, j, k, j, 0, 4, "Solitário sem vizinhos: +4");
        } else if (ef == EF_PRISMA) {
            uint64_t tipos = 0;
            for (int i = 0; i < f->lancados; i++)
                if (est[i] == V_VALIDO) tipos |= 1ull << tipo_de(j, i);
            int nt = 0;
            for (; tipos; tipos >>= 1) nt += (int)(tipos & 1);
            d->semente[j] += nt;
            evento(gera, EV_EXTRA, j, k, j, 0, nt, "Prisma: %d tipo%s, +%d", nt, nt > 1 ? "s" : "", nt);
        } else if (ef == EF_CAVALO) {
            d->semente[j] += valendo;
            evento(gera, EV_EXTRA, j, k, j, 0, valendo, "Cavalo de Troia: %d dado%s, +%d",
                   valendo, valendo > 1 ? "s" : "", valendo);
        } else if (ef == EF_DESAFIANTE) {
            bool maior = true;
            for (int q = 0; q < 2 && maior; q++)
                for (int i = 0; i < F[q].lancados; i++)
                    if (!(q == j && i == k) && d->est[q][i] == V_VALIDO && F[q].valor[i] >= f->valor[k]) {
                        maior = false;
                        break;
                    }
            if (maior) {
                d->semente[j] += 4;
                evento(gera, EV_EXTRA, j, k, j, 0, 4, "Desafiante: o %d é o maior, +4", f->valor[k]);
            }
        }
    }

    // Jackpot: tres dados valendo com o mesmo numero, +10 fora dos
    // multiplicadores (soma no mesmo bonus fixo da Semente).
    uint8_t trio = 0;
    int num = trinca(j, est, &trio);
    for (int k = 0; num && k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_JACKPOT) continue;
        d->semente[j] += 10;
        if (gera && n_ev < MAX_EV) {
            evento(gera, EV_JACKPOT, j, k, j, 0, 10, "JACKPOT! três %d: +10", num);
            ev[n_ev - 1].mascara = trio;
        }
    }
}

// Funcao pura sobre copias: serve ao desfecho de verdade (gerando eventos) e
// a previa do botao "parar" de quem joga em segundo.
static void calcula_desfecho(desfecho_t *d, bool gera)
{
    memset(d, 0, sizeof *d);
    if (gera) n_ev = 0;

    // Espelho que esperava o oponente (com "?"): vira agora o maior dado dele,
    // e a sequencia do dono e refeita com o numero novo - so agora ele pode
    // cair ou derrubar outro. Enquanto esperava, nao anulava nem era anulado.
    // No desfecho de verdade fica assim; na previa, a mesa volta como estava.
    static fila_t f_antes[2];
    static char ver_antes[sizeof veredito];
    int vt = ver_tipo, aa = anulou_a, ab = anulou_b, res = resistiu;
    uint8_t am = anulou_masc;
    bool virou = false;
    memcpy(f_antes, F, sizeof F);
    memcpy(ver_antes, veredito, sizeof veredito);
    for (int j = 0; j < 2; j++) {
        int alvo = maior_do_rival(j), virados = 0;
        uint8_t quais = 0;
        for (int k = 0; k < F[j].lancados; k++) {
            if (!F[j].espera[k] || F[j].est[k] != V_VALIDO || alvo <= 0) continue;
            F[j].valor[k] = alvo;
            F[j].espera[k] = false;
            quais |= (uint8_t)(1 << k);
            virados++;
            evento(gera, EV_ESPELHO, j, k, outro(j), 0, alvo, "Espelho vira o %d de %s", alvo, J[outro(j)].nome);
        }
        if (!virados) continue;
        if (gera) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        refaz_sequencia(j, F[j].lancados);
        for (int k = 0; k < F[j].lancados; k++)
            if (quais >> k & 1) snprintf(F[j].tag[k], sizeof F[j].tag[k], "=%d", alvo);
        virou = true;
    }
    memcpy(veredito, ver_antes, sizeof veredito);
    ver_tipo = vt; anulou_a = aa; anulou_b = ab; resistiu = res; anulou_masc = am;
    for (int j = 0; j < 2; j++) memcpy(d->est[j], F[j].est, N_FILA);

    int ordem[2] = { primeiro, outro(primeiro) };
    for (int o = 0; o < 2; o++) {
        int j = ordem[o], r = outro(j);
        bool tem_um = false;
        for (int k = 0; k < F[j].lancados; k++) if (F[j].cru[k] == 1) tem_um = true;

        for (int k = 0; k < F[j].lancados; k++) {
            int ef = TIPO[tipo_de(j, k)].efeito, v = F[j].valor[k];
            if (d->est[j][k] == V_ROUBADO) continue;
            // Carrasco que tirou 1 (valendo ou nao): anula o menor dado de quem
            // o lancou, fora ele mesmo.
            if (ef == EF_CARRASCO && F[j].cru[k] == 1) {
                int b = -1;
                for (int i = 0; i < F[j].lancados; i++)
                    if (i != k && d->est[j][i] == V_VALIDO && !nao_anulavel_t(tipo_de(j, i))
                        && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
                if (b >= 0) {
                    d->est[j][b] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                           "Carrasco tirou 1: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
                }
            }
            // Todo poder so age se o dado terminar valendo. O Gatuno, alem
            // disso, precisa de algum 1 tirado por um dado seu.
            if (d->est[j][k] != V_VALIDO || (ef == EF_GATUNO && !tem_um)) continue;
            const char *nome = TIPO[tipo_de(j, k)].nome;

            // Berserker: anula um outro dado de quem o lancou, ao acaso - um
            // acaso que so depende da mesa, como o do Laser.
            if (ef == EF_BERSERKER) {
                int alvos[N_FILA], n = 0;
                for (int i = 0; i < F[j].lancados; i++)
                    if (i != k && d->est[j][i] == V_VALIDO && !nao_anulavel_t(tipo_de(j, i))) alvos[n++] = i;
                if (n) {
                    uint32_t h = (uint32_t)(j * 5 + k);
                    for (int i = 0; i < F[j].lancados; i++) h = h * 29u + (uint32_t)F[j].valor[i];
                    for (int i = 0; i < F[r].lancados; i++) h = h * 13u + (uint32_t)F[r].valor[i];
                    int b = alvos[hash2((int)h, k + 23) % (uint32_t)n];
                    d->est[j][b] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                           "Berserker: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
                }
                continue;
            }

            bool couraca = tem_am(r, A_COURACA);         // os ataques dele nao anulam
            if (couraca && gera && ((ef == EF_CARRASCO && v == 4) || (ef == EF_LASER && v >= 5) || (ef == EF_INVEJOSO && v >= 4)))
                am_acende(A_COURACA);
            if (gera && tem_am(r, A_IMA) && (ef == EF_CARIDOSO || ef == EF_GATUNO)) am_acende(A_IMA);
            if (ef == EF_CARRASCO && v == 4) {
                int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1), a = desvia_seguranca(d->est[r], r, a0);
                if (a >= 0) {
                    d->est[r][a] = V_ANULADO;
                    if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                        "Carrasco: o Segurança de %s leva no lugar", J[r].nome);
                    else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Carrasco anula o %d de %s", F[r].valor[a], J[r].nome);
                }
            } else if (ef == EF_LASER && v >= 5) {
                // Um dado anulavel do oponente, ao acaso - mas um acaso que so
                // depende da mesa, para a previa e o desfecho darem o mesmo.
                int alvos[N_FILA], n = 0;
                for (int i = 0; i < F[r].lancados && !couraca; i++)
                    if (d->est[r][i] == V_VALIDO && !nao_anulavel_t(tipo_de(r, i))) alvos[n++] = i;
                if (n) {
                    uint32_t h = (uint32_t)(j * 7 + k);
                    for (int i = 0; i < F[r].lancados; i++) h = h * 31u + (uint32_t)F[r].valor[i];
                    for (int i = 0; i < F[j].lancados; i++) h = h * 17u + (uint32_t)F[j].valor[i];
                    int a0 = alvos[hash2((int)h, k + 11) % (uint32_t)n], a = desvia_seguranca(d->est[r], r, a0);
                    if (a >= 0) {
                        d->est[r][a] = V_ANULADO;
                        if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                            "Laser: o Segurança de %s leva no lugar", J[r].nome);
                        else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                    "Laser anula o %d de %s", F[r].valor[a], J[r].nome);
                    }
                }
            } else if (ef == EF_INVEJOSO && v >= 4) {   // Invejoso: com 4 ou mais
                int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1), a = desvia_seguranca(d->est[r], r, a0);
                // Depois de anular o do oponente, anula o maior anulavel da
                // propria sequencia - ele mesmo, se for o maior.
                int b;
                if (a >= 0) {
                    d->est[r][a] = V_ANULADO;
                    if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                        "Invejoso: o Segurança de %s leva no lugar", J[r].nome);
                    else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Invejoso anula o %d de %s", F[r].valor[a], J[r].nome);
                }
                b = alvo_de(d->est[j], j, true, -1);
                if (b >= 0) {
                    d->est[j][b] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                           "e anula o %d do próprio %s", F[j].valor[b], J[j].nome);
                }
            } else if (ef == EF_MARTELO && v >= 3) {     // Espinhoso: com 3 ou 4
                d->delta[r] -= 4;
                evento(gera, EV_TIRA, j, k, r, 0, 4, "Espinhoso: -4 para %s", J[r].nome);
            } else if (ef == EF_MALDICAO) {              // vale para o dono e tira do oponente
                d->delta[r] -= v;
                evento(gera, EV_TIRA, j, k, r, 0, v, "Maldito: -%d para %s", v, J[r].nome);
            } else if (ef == EF_PIRATA && v >= 6) {      // so tira; nao fica com os pontos
                d->delta[r] -= 4;
                evento(gera, EV_TIRA, j, k, r, 0, 4, "Pirata tira 4 de %s", J[r].nome);
            } else if ((ef == EF_CARIDOSO && v >= 3) || ef == EF_GATUNO) {   // Caridoso: com 3 ou 4
                int a0 = alvo_de(d->est[r], r, false, -1), a = desvia_seguranca(d->est[r], r, a0);
                if (a >= 0) {
                    int vale = contribui(r, a);
                    d->est[r][a] = V_ROUBADO;
                    d->roubo_val[j] += vale;
                    if (a != a0) evento(gera, EV_ROUBA, j, k, r, a, vale,
                                        "%s: o Segurança de %s vai no lugar (%d)", nome, J[r].nome, vale);
                    else evento(gera, EV_ROUBA, j, k, r, a, vale,
                                "%s rouba o %d de %s", nome, vale, J[r].nome);
                }
            }
        }
    }
    // Fenix: anulada, renasce valendo o que tirou (roubada, nao).
    for (int o = 0; o < 2; o++) {
        int j = ordem[o];
        for (int k = 0; k < F[j].lancados; k++)
            if (d->est[j][k] == V_ANULADO && TIPO[tipo_de(j, k)].efeito == EF_FENIX) {
                d->est[j][k] = V_VALIDO;
                evento(gera, EV_RENASCE, j, k, j, k, F[j].valor[k], "Fênix renasce valendo %d", F[j].valor[k]);
            }
    }
    for (int o = 0; o < 2; o++) efeitos_do_fim(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) {
        int j = ordem[o];
        if (!F[j].zerada) continue;
        d->zerada[j] = 1;
        int k = -1;
        for (int i = 0; i < F[j].lancados; i++)
            if (TIPO[tipo_de(j, i)].efeito == EF_TUDO_NADA && F[j].cru[i] == 1) { k = i; break; }
        evento(gera, EV_ZERA, j, k < 0 ? 0 : k, j, 0, 0, "Tudo ou Nada: a rodada vale 0");
    }
    for (int j = 0; j < 2; j++) {
        int t = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j]
              + d->delta[j] + d->semente[j];
        d->bruto[j] = d->zerada[j] ? 0 : t;
        d->total[j] = (t < 0 || d->zerada[j]) ? 0 : t;
    }
    if (!gera && virou) memcpy(F, f_antes, sizeof F);   // previa: o Espelho segue esperando
}

// Pontuacao que a mesa mostra durante e depois da animacao dos eventos.
static int total_visivel(int j)
{
    if (vis_zerada[j]) return 0;
    int t = pontos_fila(j, vis_est[j], vis_bonus[j], vis_mult[j]) + vis_delta[j] + vis_semente[j];
    for (int i = 0; i < vis_nroubo[j]; i++) t += vis_roubo[j][i].valor;
    // Espelho cujo evento ainda nao tocou: na tela, ainda com o que tirou.
    // (Esses eventos vem antes de todos, sem bonus nem multiplicador ainda.)
    for (int k = 0; k < F[j].lancados; k++)
        if (vis_espelho[j][k] && vis_est[j][k] == V_VALIDO) t -= F[j].valor[k] - F[j].cru[k];
    return t < 0 ? 0 : t;
}

// Probabilidade de o proximo dado cair. Nao aparece mais na tela; fica para
// o simulador medir o jogo.
static int risco(int j)
{
    const fila_t *f = &F[j];
    int k = f->lancados;
    if (k >= f->n) return 0;
    int t = tipo_de(j, k), lados = TIPO[t].lados, ef = TIPO[t].efeito;
    int l = ultimo_valido(f, f->est, k);
    int L = l >= 0 ? f->valor[l] : 0;
    if (ef == EF_QUEBRADO) return f->fixo[k] < L ? 100 : 0;
    if (egoista_antes(j, k) >= 0 && !resiste_egoista(t)) return 100;   // cai com certeza
    if (ef == EF_EGOISTA) {                  // so cai diante de quem resiste a ele
        int m = 0;
        for (int i = 0; i < k; i++)
            if (f->est[i] == V_VALIDO && resiste_egoista(tipo_de(j, i)) && f->valor[i] > m) m = f->valor[i];
        int ruins = 0;
        for (int a = 1; a <= lados; a++) ruins += a < m;
        return ruins * 100 / lados;
    }
    int ruins = 0, total = 0;
    if (ef == EF_VICIADO) {
        for (int a = 1; a <= lados; a++)
            for (int b = 1; b <= lados; b++) { total++; if ((a > b ? a : b) < L) ruins++; }
        return ruins * 100 / total;
    }
    for (int a = ef == EF_LASTRO ? 4 : 1; a <= lados; a++) {   // Lastro: so 4 a 12
        int v = a;
        total++;
        if (ef == EF_EXPLODE && v == lados) continue;
        if (ef == EF_ESPELHO && v <= 2) continue;
        if ((ef == EF_MALDITO && v <= 7) || (ef == EF_TUDO_NADA && v == 1) || (ef == EF_VIDRO && v == 1)) { ruins++; continue; }
        if (v < L) ruins++;
    }
    return ruins * 100 / total;
}

// ===========================================================================
// Partida e rodada
// ===========================================================================
// Oito dados de saida: com a mao de 7, a compra ja pesa desde a 1a rodada.
static void colecao_inicial(jogador_t *p)
{
    static const uint8_t INICIO[] = { D_D6, D_D6, D_D6, D_D6, D_D4, D_D4, D_D2, D_D2 };
    p->n_col = 0;
    for (unsigned i = 0; i < sizeof INICIO; i++) {
        p->col[p->n_col] = nova_peca(INICIO[i]);
        p->onde[p->n_col++] = NA_BOLSA;
    }
}

static int na_bolsa(const jogador_t *p)
{
    int n = 0;
    for (int i = 0; i < p->n_col; i++) if (p->onde[i] == NA_BOLSA) n++;
    return n;
}

// A mao da rodada anterior sai da mesa.
static void descarta_mao(jogador_t *p)
{
    for (int i = 0; i < p->n_col; i++) if (p->onde[i] == NA_MAO) p->onde[i] = FORA;
    p->n_mao = 0;
}

// Compra ate 7 da bolsa, ao acaso. Se a bolsa esvazia no meio, tudo o que
// esta fora volta para ela - menos o que ja esta na mao - e a compra segue.
// Quantos j compra por rodada: 7; 8 com a Bolsa Funda; 6 se o oponente tem
// o Saco Furado.
static int tam_mao(int j)
{
    int n = MAO_NORMAL;
    if (tem_am(j, A_BOLSA)) n++;
    if (tem_am(j ^ 1, A_FURADO)) n--;
    if (j == 1 && op_nervoso && tem_reg(REG_NERVOSO)) n--;
    if (j == 0 && virada_forca(0) == 2) n--;            // o Crupie forte: voce compra 1 a menos
    return n;
}

static void compra(jogador_t *p)
{
    p->embaralhou = false;
    int quer = tam_mao((int)(p - J));
    while (p->n_mao < quer) {
        int n = na_bolsa(p);
        if (!n) {
            bool voltou = false;
            for (int i = 0; i < p->n_col; i++)
                if (p->onde[i] == FORA) { p->onde[i] = NA_BOLSA; voltou = true; }
            if (!voltou) break;                        // todos ja estao na mao
            p->embaralhou = true;
            n = na_bolsa(p);
        }
        int alvo = sorte(n);
        for (int i = 0; i < p->n_col; i++)
            if (p->onde[i] == NA_BOLSA && alvo-- == 0) {
                p->onde[i] = NA_MAO;
                p->mao[p->n_mao++] = (int8_t)i;
                break;
            }
    }
}

static void encerra(int vencedor)
{
    venc_partida = vencedor;
    fase = F_FIM;
    // Contra o computador (ou online), perder tem o seu som; com duas pessoas
    // no mesmo teclado, alguem sempre ganhou.
    bool perdeu = modo != M_DOIS && vencedor != baixo;
    som_toca(vencedor == 2 && modo == M_DOIS ? SOM_EMPATE : (perdeu ? SOM_DERROTA : SOM_FIM));
    if (modo == M_DESAFIO) {
        fala_evento(vencedor == 1 ? FA_GANHA_P : FA_PERDE_P);
        des_resultado();                      // a run anota na hora
    }
}

static void des_virou(void);

static void nova_rodada(void)
{
    rodada++;
    plano_rodada = -1;                       // a IA monta outra fila
    ferr_k[0] = ferr_k[1] = -1;
    // A Detetive joga por ultimo na primeira rodada e quando esta atras em fichas.
    if (tem_reg(REG_ULTIMA) && (rodada == 1 || J[1].fichas < J[0].fichas)) primeiro = 0;
    // Relogio de Bolso: na primeira rodada e quando voce esta atras em fichas,
    // o oponente abre (vale mais).
    if (tem_am(0, A_RELOGIO) && (rodada == 1 || J[0].fichas < J[1].fichas)) primeiro = 1;
    for (int j = 0; j < 2; j++) fichas_ini[j] = J[j].fichas;   // para o Cobrador
    // No Desafiante sao 8 rodadas; empatado no fim, ate 3 rodadas extras.
    int max_r = modo == M_DESAFIO ? DES_RODADAS : MAX_RODADAS;
    bool extra = modo == M_DESAFIO && J[0].fichas == J[1].fichas && rodada <= max_r + DES_EXTRAS;
    if (rodada > max_r && !extra) {
        encerra(J[0].fichas == J[1].fichas ? 2 : (J[0].fichas > J[1].fichas ? 0 : 1));
        return;
    }
    // Quem nao paga a entrada perde. Se nenhum dos dois paga, vence quem tem
    // mais fichas (o pote parado na mesa se dividiria igual e nao muda isso);
    // com fichas iguais, empate.
    int ante = ante_da(rodada);
    bool sem0 = J[0].fichas < ante, sem1 = J[1].fichas < ante;
    if (sem0 && sem1) {
        encerra(J[0].fichas == J[1].fichas ? 2 : (J[0].fichas > J[1].fichas ? 0 : 1));
        return;
    }
    if (sem0 || sem1) { encerra(sem0 ? 1 : 0); return; }
    if (rodada == VIRADA_RODADA && fala_perso >= PERSO_CHEFE0 && fala_perso < PERSO_CHEFE0 + 3
        && virada_forca(fala_perso - PERSO_CHEFE0))
        des_virou();                             // o chefe vira: anuncio, retrato e fala
    if (rodada > 1 && ante > ante_da(rodada - 1)) {     // subiu: avisa, com faixa e fichas
        t_aviso_entrada = ENTRADA_AVISO_T;
        som_toca(SOM_FICHA);
    }
    for (int j = 0; j < 2; j++) {
        J[j].fichas -= ante;
        memset(&F[j], 0, sizeof F[j]);
        descarta_mao(&J[j]);
        compra(&J[j]);
    }
    pote += 2 * ante;
    n_voo = 0;
    n_ev = ev_i = 0;
    memset(vis_nroubo, 0, sizeof vis_nroubo);
    memset(vis_delta, 0, sizeof vis_delta);
    memset(vis_bonus, 0, sizeof vis_bonus);
    memset(vis_mult, 0, sizeof vis_mult);
    memset(vis_queima, 0, sizeof vis_queima);
    memset(vis_espelho, 0, sizeof vis_espelho);
    memset(vis_semente, 0, sizeof vis_semente);
    memset(vis_zerada, 0, sizeof vis_zerada);
    aviso[0] = veredito[0] = nota[0] = 0;
    vez = primeiro;
    cursor = 0;
    t_fase = 0;
    fase = F_ORDEM;
    som_toca(rodada == 1 ? SOM_PARTIDA : SOM_BOLSA);   // comeca a partida, ou mais uma rodada
}

// Nomes de cada modo: quem joga aqui e sempre VOCE (ou JOGADOR 1 e 2).
static void poe_nomes(void)
{
    switch (modo) {
    case M_UM:     J[0].nome = "VOCÊ";      J[1].nome = "COMPUTADOR"; break;
    case M_DOIS:   J[0].nome = "JOGADOR 1"; J[1].nome = "JOGADOR 2";  break;
    default:       J[baixo].nome = "VOCÊ";  J[outro(baixo)].nome = "OPONENTE"; break;
    }
}

static void abre_loja(void);

void dado_inicia(int n_partidas)
{
    static bool paleta_pronta;
    if (!paleta_pronta) { gfx_paleta(PALETA, (int)(sizeof PALETA / sizeof PALETA[0])); paleta_pronta = true; }
    if (!rapido_lido) {                  // a velocidade escolhida da ultima vez
        char b[8];
        rapido = salva_le("vel", b, sizeof b) > 0 && b[0] == '1';
        rapido_lido = true;
    }
    partidas = n_partidas;
    semente_v = (uint32_t)esp_timer_get_time() | 1;
    // Online, os dois lados sorteiam igual: a semente vem da sala.
    if (modo == M_ONLINE) semente = (semente_online ^ (uint32_t)n_partidas * 0x9E3779B9u) | 1;
    else                  semente = (uint32_t)esp_timer_get_time() | 1;
    som_rufo(false);
    for (int i = 0; i < 8; i++) rnd();
    memset(J, 0, sizeof J);
    J[0].cor = C_LATAO;
    J[1].cor = C_ROSA;
    poe_nomes();
    for (int j = 0; j < 2; j++) {
        J[j].fichas = FICHAS_INI;
        colecao_inicial(&J[j]);
    }
    rodada = 0;
    pote = 0;
    primeiro = partidas % 2;
    venc_partida = -1;
    correu_venc = -1;
    menu_tela = MT_PRINCIPAL;

    uint8_t v[sizeof VITRINE];
    memcpy(v, VITRINE, sizeof v);
    for (int i = (int)sizeof v - 1; i > 0; i--) {
        int j = sorte(i + 1);
        uint8_t t = v[i]; v[i] = v[j]; v[j] = t;
    }
    for (int i = 0; i < N_LOJA; i++) loja[i] = nova_peca(v[i]);
    memset(levou, 0, sizeof levou);
    loja_primeiro = primeiro;

    n_voo = 2;
    memset(voo, 0, sizeof voo);
    voo[0] = (voo_t){ .x = 60, .y = 270, .vx = 650, .vy = -180, .spin = 11,
                      .dono = 0, .tipo = D_D6, .valor = 6, .face = 1 };
    voo[1] = (voo_t){ .x = 580, .y = 245, .vx = -650, .vy = -110, .spin = -11,
                      .dono = 1, .tipo = D_D6, .valor = 6, .face = 3 };
    abertura_pronta = false;
    t_fase = 0;
    fase = F_ABERTURA;
    t_sair = -1;
    if (modo == M_ONLINE) abre_loja();     // online, a revanche vai direto a loja
}

static void abre_loja(void)
{
    vez = loja_primeiro;
    cursor = 0;
    n_voo = 0;
    fase = F_LOJA;
}

// O que o dado de tipo t tira, com os poderes que mexem no sorteio. 'r' e
// quem sorteia: o da partida ou o da IA quando ela imagina jogadas. No
// Viciado, 'menor' recebe o lance descartado.
static int sorteia_valor(int t, int fixo, int (*r)(int), int *menor)
{
    int lados = TIPO[t].lados;
    int x = r(lados);
    *menor = 0;
    switch (TIPO[t].efeito) {
    case EF_VICIADO: {                       // dois lances; fica o maior
        int y = r(lados);
        *menor = x < y ? x : y;
        if (y > x) x = y;
        break;
    }
    case EF_LASTRO:  x = 3 + r(9); break;    // 4 a 12, 1 em 9 cada
    case EF_EXPLODE: {
        int n = x, g = 0;
        while (n == lados && g++ < 3) { n = r(lados); x += n; }
        break;
    }
    case EF_QUEBRADO: x = fixo; break;       // sem sorteio
    default: break;
    }
    return x;
}

static void lanca_proximo(void)
{
    fila_t *f = &F[vez];
    int k = f->lancados;
    int t = tipo_de(vez, k), lados = TIPO[t].lados;

    // Acumulador: cada lance guarda +1 nele, para sempre (ate +6).
    if (t == D_ACUMULADOR && f->idx[k] >= 0 && f->idx[k] < J[vez].n_col) {
        peca_t *pc = &J[vez].col[f->idx[k]];
        if (pc->fixo < 6) pc->fixo++;
        f->fixo[k] = pc->fixo;
    }
    int menor;
    int x = sorteia_valor(t, f->fixo[k], rola, &menor);
    f->cru[k] = x;
    f->valor[k] = x;

    voo_t *v = &voo[0];
    memset(v, 0, sizeof *v);
    v->dono = (uint8_t)vez;
    v->slot = (uint8_t)k;
    v->tipo = (uint8_t)t;
    v->valor = x;
    v->x = GFX_W / 2 + frnd(-110, 110);
    v->y = vez == baixo ? MESA_Y1 - R_DADO - 4 : MESA_Y0 + R_DADO + 4;
    v->vx = frnd(-300, 300);
    v->vy = (vez == baixo ? -1 : 1) * frnd(420, 600);
    v->spin = frnd(-14, 14);
    v->face = face_v(t, x);
    n_voo = 1;
    f->descarte[k] = (uint8_t)menor;
    // O Viciado rola de verdade dois dados: o do menor valor e um fantasma
    // que some quando o maior vai para a fila.
    if (menor) {
        voo_t *w = &voo[1];
        *w = *v;
        w->fantasma = true;
        w->valor = menor;
        w->x = v->x + (v->x < GFX_W / 2 ? 60 : -60);
        w->vx = -v->vx * 0.8f + frnd(-60, 60);
        w->vy = v->vy * frnd(0.8f, 1.0f);
        w->spin = -v->spin;
        w->face = rola_v(lados);
        n_voo = 2;
    }
    som_rufo(true);                          // tensao ate o dado assentar
    t_fase = 0;
    fase = F_ROLANDO;
}

static void comeca_segundo(void)
{
    vez = outro(primeiro);
    cursor = 0;
    t_fase = 0;
    fase = F_ORDEM;
}

// Quebrados jogados saem da bolsa de quem os tinha. A bolsa nunca cai abaixo
// de quatro: se cairia, o Quebrado vira um d4 comum.
// Quebrados jogados saem da colecao de quem os tinha. A colecao nunca cai
// abaixo de quatro: se cairia, o Quebrado vira um d4 comum.
// Dados que saem do jogo ao fim da rodada: o Quebrado que foi jogado e o que
// a Ficha de Fogo queimou. A colecao nunca cai abaixo de quatro: se cairia, o
// dado perdido vira um d4 comum.
static void recolhe_quebrados(void)
{
    int n_quebra = 0, n_cinza = 0, n_vidro = 0;
    for (int j = 0; j < 2; j++) {
        int saem[N_FILA], n = 0;
        for (int k = 0; k < F[j].lancados; k++) {
            bool some = false;
            if (F[j].tipo[k] == D_QUEBRADO) { F[j].quebra[k] = true; some = true; n_quebra++; }
            if (F[j].tipo[k] == D_VIDRO && F[j].cru[k] == 1) { F[j].quebra[k] = true; some = true; n_vidro++; }
            if (vis_queima[j][k] && !some)  { F[j].cinza[k] = true; some = true; n_cinza++; }
            if (some) saem[n++] = F[j].idx[k];
        }
        for (int a = 1; a < n; a++)
            for (int b = a; b > 0 && saem[b] > saem[b - 1]; b--) {
                int t = saem[b]; saem[b] = saem[b - 1]; saem[b - 1] = t;
            }
        jogador_t *p = &J[j];
        for (int i = 0; i < n; i++) {
            if (p->n_col <= N_FILA) { p->col[saem[i]] = nova_peca(D_D4); continue; }
            for (int m = saem[i]; m + 1 < p->n_col; m++) {
                p->col[m] = p->col[m + 1];
                p->onde[m] = p->onde[m + 1];
            }
            p->n_col--;
        }
        p->n_mao = 0;
    }
    if (n_cinza && n_quebra) snprintf(nota, sizeof nota, "um dado vira cinza, outro se quebra");
    else if (n_cinza)        snprintf(nota, sizeof nota, n_cinza > 1 ? "os dados queimados viram cinza" : "o dado queimado vira cinza");
    else if (n_quebra)       snprintf(nota, sizeof nota, n_quebra > 1 ? "Quebrados se desfazem" : "o Quebrado se desfaz");
    if (n_vidro && !n_cinza && !n_quebra) snprintf(nota, sizeof nota, "cacos de vidro saem da mesa");
    else if (n_vidro)        snprintf(nota, sizeof nota, "dados se desfazem, e o vidro também");
}

// O desfecho calculado pelo motor de regras: e ele que paga. Os eventos so
// contam na tela, um a um, como se chegou nele.
static desfecho_t resolvido;
// O pote indo para quem levou: de quanto a quanto vai cada placar, e para onde
// as fichas voam (a ficha de cada faixa, guardada quando a faixa e desenhada).
static int res_antes[2], res_quem = -1;
static int ficha_px[2] = { 160, 160 }, ficha_py[2] = { GFX_H - 22, 22 };

// Depois da animacao dos eventos: paga o pote e declara quem levou.
static void conclui_rodada(void)
{
    res_antes[0] = J[0].fichas; res_antes[1] = J[1].fichas;
    res_quem = -1;
    for (int j = 0; j < 2; j++) {
        memcpy(F[j].est, resolvido.est[j], N_FILA);
        memcpy(vis_est[j], resolvido.est[j], N_FILA);
        memcpy(vis_queima[j], resolvido.queima[j], N_FILA);
        F[j].total = resolvido.total[j];
    }
    bool casa = F[0].total == F[1].total && tem_reg(REG_CASA);   // o Crupie leva o empate
    if (F[0].total == F[1].total && !casa) {
        venc_mao = 2;
        som_toca(SOM_EMPATE);
        snprintf(aviso, sizeof aviso, "empate em %d: o pote fica na mesa", F[0].total);
    } else {
        int w = casa ? 1 : (F[0].total > F[1].total ? 0 : 1);
        venc_mao = w;
        res_quem = w;
        J[w].fichas += pote;
        if (casa) snprintf(aviso, sizeof aviso, "empate: a casa leva %d fichas", pote);
        else snprintf(aviso, sizeof aviso, "%s leva %d fichas", J[w].nome, pote);
        if (w == 1 && tem_reg(REG_BATEDOR) && J[0].fichas > 0) {   // o Ladrao leva mais 2 suas
            int b = J[0].fichas < 2 ? J[0].fichas : 2;
            J[0].fichas -= b;
            J[1].fichas += b;
            snprintf(aviso, sizeof aviso, "%s leva %d fichas, e mais %d suas", J[w].nome, pote, b);
        }
        if (tem_am(w, A_POTE)) {                 // Pote Gordo: 2 a mais
            J[w].fichas += 2;
            snprintf(aviso, sizeof aviso, "%s leva %d fichas (+2)", J[w].nome, pote);
            am_acende(A_POTE);
        }
        pote = 0;
        som_toca(SOM_VITORIA);
    }
    int aluguel = virada_forca(1);                // a Dona cobra o aluguel, ganhe quem ganhar
    if (aluguel && J[0].fichas > 0) {
        int a = aluguel == 2 ? 5 : 3;
        if (a > J[0].fichas) a = J[0].fichas;
        J[0].fichas -= a;
        J[1].fichas += a;
        size_t n = strlen(aviso);
        snprintf(aviso + n, sizeof aviso - n, "  ·  aluguel: %d", a);
    }
    op_nervoso = venc_mao == 0;
    if (venc_mao == 0) des_rod_vencidas++;
    if (venc_mao == 0) fala_evento(FA_PERDE_R);
    else if (venc_mao == 1) fala_evento(FA_GANHA_R);
    recolhe_quebrados();
    t_fase = 0;
    fase = F_RESULTADO;
}

// O Jackpot tem som proprio; os outros poderes, o brilho de sempre.
// Cada poder com o seu som: o Jackpot tem o dele; anular, tirar pontos,
// roubar, queimar e zerar tambem; o resto, o brilho de sempre.
static int som_do_evento(int i)
{
    const evento_t *e = &ev[i];
    switch (e->tipo) {
    case EV_JACKPOT: return SOM_JACKPOT;
    case EV_ANULA:   return F[e->de_j].tipo[e->de_k] == D_LASER ? SOM_LASER : SOM_ANULA;
    case EV_TIRA: case EV_PIRATA: return SOM_TIRA;
    case EV_ROUBA:   return SOM_ROUBA;
    case EV_FOGO: case EV_RENASCE: return SOM_FOGO;
    case EV_ZERA:    return SOM_ZERA;
    case EV_BONUS:   return e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_FERREIRO ? SOM_BIGORNA : SOM_EFEITO;
    default:         return SOM_EFEITO;
    }
}

static void desfecho(void)
{
    calcula_desfecho(&resolvido, true);
    for (int j = 0; j < 2; j++) {
        memcpy(vis_est[j], F[j].est, N_FILA);
        vis_nroubo[j] = 0;
        vis_delta[j] = 0;
        vis_semente[j] = 0;
        vis_zerada[j] = 0;
        memset(vis_bonus[j], 0, N_FILA);
        memset(vis_mult[j], 0, N_FILA);
        memset(vis_queima[j], 0, N_FILA);
        memset(vis_espelho[j], 0, N_FILA);
    }
    for (int i = 0; i < n_ev; i++)
        if (ev[i].tipo == EV_ESPELHO) {
            vis_espelho[ev[i].de_j][ev[i].de_k] = 1;
            memcpy(vis_est[ev[i].de_j], est_pre_espelho[ev[i].de_j], N_FILA);
        }
    n_voo = 0;
    ev_i = 0;
    t_fase = 0;
    if (n_ev) { fase = F_EFEITOS; som_toca(som_do_evento(0)); }
    else conclui_rodada();
}

static float duracao_evento(int i)
{
    switch (ev[i].tipo) {
    case EV_ROUBA: return 1.6f;
    case EV_MULT:  return 1.7f;
    case EV_FOGO:  return 1.6f;
    case EV_SEMENTE: return 1.6f;
    case EV_EXTRA: case EV_CHEFE: return 1.6f;
    case EV_RENASCE: return 1.5f;
    case EV_JACKPOT: return 2.4f;
    case EV_ZERA:  return 1.6f;
    case EV_BONUS: return 1.4f;
    default:       return 1.3f;
    }
}

// O evento i terminou: a mesa passa a mostrar o efeito dele.
static void aplica_evento(int i)
{
    evento_t *e = &ev[i];
    switch (e->tipo) {
    case EV_ANULA:  vis_est[e->alvo_j][e->alvo_k] = V_ANULADO; break;
    case EV_ROUBA: {
        vis_est[e->alvo_j][e->alvo_k] = V_ROUBADO;
        int n = vis_nroubo[e->de_j];
        if (n < 4) {
            vis_roubo[e->de_j][n] = (roubado_t){ F[e->alvo_j].tipo[e->alvo_k],
                                                 F[e->alvo_j].fixo[e->alvo_k],
                                                 e->alvo_j, e->valor };
            vis_nroubo[e->de_j] = n + 1;
        }
        break;
    }
    case EV_ESPELHO:                    // virou: a sequencia refeita aparece
        vis_espelho[e->de_j][e->de_k] = 0;
        memcpy(vis_est[e->de_j], F[e->de_j].est, N_FILA);
        break;
    case EV_TIRA:   vis_delta[e->alvo_j] -= e->valor; break;
    case EV_PIRATA: vis_delta[e->alvo_j] -= e->valor; vis_delta[e->de_j] += e->valor; break;
    case EV_BONUS:
        for (int k = 0; k < N_FILA; k++) vis_bonus[e->de_j][k] += e->por_dado[k];
        break;
    case EV_MULT:
        for (int k = 0; k < N_FILA; k++) vis_mult[e->de_j][k] = e->por_dado[k];
        break;
    case EV_FOGO:    vis_queima[e->alvo_j][e->alvo_k] = 1; vis_semente[e->de_j] += e->valor; break;
    case EV_SEMENTE: case EV_JACKPOT: case EV_EXTRA: case EV_CHEFE: vis_semente[e->de_j] += e->valor; break;
    case EV_RENASCE: vis_est[e->de_j][e->de_k] = V_VALIDO; break;
    case EV_ZERA:    vis_zerada[e->de_j] = 1; break;
    }
}

static void termina_turno(void)
{
    som_toca(SOM_TURNO);
    if (vez == primeiro) {
        a_pagar = 0; passes = 0; aumentou = false;
        cursor = 0;
        fase = F_APOSTA;
    } else {
        desfecho();
    }
}

// ===========================================================================
// Aposta (entre os dois turnos)
// ===========================================================================
enum { OP_PASSAR, OP_APOSTAR10, OP_APOSTAR25, OP_PAGAR, OP_AUMENTAR, OP_CORRER };

// Quanto quem esta na vez pode subir alem de cobrir: o dobro, limitado ao
// que sobra para ele depois de cobrir e ao que o rival ainda tem.
static int aumento_possivel(void)
{
    int sobe = a_pagar;
    int sobra = J[vez].fichas - a_pagar;
    if (sobe > sobra) sobe = sobra;
    if (sobe > J[outro(vez)].fichas) sobe = J[outro(vez)].fichas;
    return sobe < 0 ? 0 : sobe;
}

static int opcoes(int *op)
{
    int n = 0;
    if (!a_pagar) { op[n++] = OP_PASSAR; op[n++] = OP_APOSTAR10; op[n++] = OP_APOSTAR25; }
    else {
        op[n++] = OP_PAGAR;
        if (!aumentou && aumento_possivel() > 0)
            op[n++] = OP_AUMENTAR;
        op[n++] = OP_CORRER;
    }
    return n;
}

static int limite(int quer)
{
    int m = J[0].fichas < J[1].fichas ? J[0].fichas : J[1].fichas;
    return quer < m ? quer : m;
}

static void rotulo_op(int op, char *s, int n)
{
    switch (op) {
    case OP_PASSAR:    snprintf(s, n, "passar"); break;
    case OP_APOSTAR10: snprintf(s, n, "apostar %d", limite(10)); break;
    case OP_APOSTAR25: snprintf(s, n, "apostar %d", limite(25)); break;
    case OP_PAGAR:     snprintf(s, n, "pagar %d", a_pagar); break;
    case OP_AUMENTAR:                    // sem fichas para dobrar, diz quanto sobe
        if (aumento_possivel() < a_pagar) snprintf(s, n, "subir %d", aumento_possivel());
        else snprintf(s, n, "dobrar");
        break;
    default:           snprintf(s, n, "correr"); break;
    }
}

// A cor de cada botao da aposta: passar neutro, apostar em vinho (25 mais
// forte), pagar em ouro velho, dobrar em vermelho vivo, correr em preto.
static uint16_t tinta_op(int op)
{
    switch (op) {
    case OP_PASSAR:    return RGB(44, 66, 58);
    case OP_APOSTAR10: return RGB(112, 40, 46);
    case OP_APOSTAR25: return RGB(150, 34, 40);
    case OP_PAGAR:     return RGB(112, 88, 38);
    case OP_AUMENTAR:  return RGB(168, 44, 36);
    default:           return RGB(20, 20, 22);       // correr
    }
}

static void executa(int op)
{
    jogador_t *p = &J[vez];
    if (op != OP_PASSAR && op != OP_CORRER) som_toca(SOM_FICHA);
    switch (op) {
    case OP_PASSAR:
        if (++passes >= 2) { comeca_segundo(); return; }
        break;
    case OP_APOSTAR10:
    case OP_APOSTAR25: {
        int b = limite(op == OP_APOSTAR10 ? 10 : 25);
        if (b <= 0) { comeca_segundo(); return; }
        p->fichas -= b; pote += b; a_pagar = b;
        break;
    }
    case OP_PAGAR: {
        int b = a_pagar < p->fichas ? a_pagar : p->fichas;
        p->fichas -= b; pote += b;
        comeca_segundo();
        return;
    }
    case OP_AUMENTAR: {
        // Cobre a aposta e sobe o mesmo tanto, ate onde as fichas dos dois
        // alcancam. O rival paga so o que de fato subiu.
        int sobe = aumento_possivel();
        p->fichas -= a_pagar + sobe; pote += a_pagar + sobe;
        a_pagar = sobe;
        aumentou = true;
        break;
    }
    case OP_CORRER: {
        int w = outro(vez);
        res_antes[0] = J[0].fichas; res_antes[1] = J[1].fichas;
        res_quem = w;
        J[w].fichas += pote;
        snprintf(aviso, sizeof aviso, "%s correu. %s leva %d", p->nome, J[w].nome, pote);
        som_toca(SOM_CORREU);
        pote = 0;
        venc_mao = -1;
        correu_venc = w;                  // leva tambem o premio, como numa vitoria
        primeiro = w;                     // quem levou joga primeiro na proxima
        for (int j = 0; j < 2; j++) memcpy(vis_est[j], F[j].est, N_FILA);
        recolhe_quebrados();
        t_fase = 0;
        fase = F_RESULTADO;
        return;
    }
    }
    vez = outro(vez);
    cursor = 0;
}

// ===========================================================================
// Premio
// ===========================================================================
// Peso por DADO, nao por raridade: comum 8, incomum 4, raro 2, lendario 1.
// Assim cada lendario e mais raro que cada raro, quantos houver em cada grupo.
// Moeda, d4 e d6 sao so da colecao inicial: nunca saem como premio.
static bool so_inicial(int t) { return t == D_D2 || t == D_D4 || t == D_D6; }
static int peso_dado(int t)
{
    if (so_desafio(t)) return 0;                 // so no Modo Desafiante
    static const int PESO[4] = { 8, 4, 2, 1 };
    return so_inicial(t) ? 0 : PESO[TIPO[t].raridade];
}
static int sorteia_tipo(void)
{
    int total = 0;
    for (int t = 0; t < D_N; t++) total += peso_dado(t);
    int r = sorte(total);
    for (int t = 0; t < D_N; t++) {
        r -= peso_dado(t);
        if (r < 0) return t;
    }
    return D_D8;
}

static void desenha_lance(void);

// Alto-falante miudo na faixa de ERIC quando o som esta ligado.
static void icone_som(int x, int y)
{
    gfx_rect(x, y + 4, 4, 6, C_OURO);
    for (int i = 0; i < 4; i++) gfx_rect(x + 4 + i, y + 4 - i, 1, 6 + 2 * i, C_OURO);
    for (int k = 0; k < 2; k++) {
        int d = 11 + 4 * k;
        gfx_rect(x + d, y + 3 - 2 * k, 1, 8 + 4 * k, C_OURO);
    }
}

static bool des_tem_run(void);
static bool dado_liberado(int t);
static void des_resumo(char *s, int n);
static void des_esc(void);

// As opcoes da lista do menu em uso.
static int menu_opcoes(const char **op)
{
    switch (menu_tela) {
    case MT_UM:      op[0] = "Partida rápida"; op[1] = "Modo Desafiante"; return 2;
    case MT_NIVEL:   op[0] = "Fácil"; op[1] = "Médio"; op[2] = "Difícil"; return 3;
    case MT_DESAFIO: op[0] = "Continuar a run"; op[1] = "Nova run"; return 2;
    default:
        op[0] = "1 jogador"; op[1] = "2 jogadores"; op[2] = "Online";
        op[3] = "Dados da casa"; op[4] = "Como jogar";
        return MN_N;
    }
}

// ---------------------------------------------------------------------------
// Mouse: cada tela, ao se desenhar, marca as areas que aceitam clique. Passar
// o mouse por cima poe o cursor na opcao; clicar e o mesmo que apertar a
// tecla da area (quase sempre ENTER).
// ---------------------------------------------------------------------------
typedef struct { int16_t x, y, w, h; int *var; int val; int tecla; } zona_clique_t;
#define MAX_ZONAS 64
static zona_clique_t zonas[MAX_ZONAS];
static int n_zonas, zonas_fase;            // zonas_fase: a tela que as marcou
static int mouse_x = -1, mouse_y = -1;

static void zona_clique(int x, int y, int w, int h, int *var, int val, int tecla)
{
    if (n_zonas >= MAX_ZONAS) return;
    zonas[n_zonas++] = (zona_clique_t){ (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, var, val, tecla };
}

// Dicas: areas que, com o mouse parado em cima, mostram o que a coisa faz.
enum { DICA_DADO, DICA_AMULETO, DICA_TRUNFO };
typedef struct { int16_t x, y, w, h; uint8_t tipo, id; } dica_t;
#define MAX_DICAS 64
static dica_t dicas[MAX_DICAS];
static int n_dicas;
static void dica_zona(int x, int y, int w, int h, int tipo, int id)
{
    if (n_dicas >= MAX_DICAS) return;
    dicas[n_dicas++] = (dica_t){ (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, (uint8_t)tipo, (uint8_t)id };
}

static bool mouse_em(int x, int y, int w, int h)
{
    return mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h;
}

// Botao pequeno para o que antes so tinha tecla (TAB, X...). x_dir e a borda
// direita do botao.
static void botao_clique(int x_dir, int y, const char *rot, int tecla)
{
    int w = gfx_largura(rot, 1) + 20, h = TXT_H + 4, x = x_dir - w;
    bool em = mouse_em(x, y, w, h);
    ui_botao(x, y, w, h, rot, 0, em ? UI_FOCO : 0);
    zona_clique(x, y, w, h, NULL, 0, tecla);
}

// Botao "<" no canto de cima, a esquerda: volta a tela anterior sem teclado
// (celular, tablet). A area de toque pega o canto inteiro da faixa. Devolve
// onde o titulo da faixa pode comecar.
#define VOLTAR_X0 46
static int botao_voltar(int tecla)
{
    int x = 8, y = (FAIXA_H - 28) / 2, w = 30, h = 28;
    bool em = mouse_em(0, 0, VOLTAR_X0 - 4, FAIXA_H);
    ui_botao(x, y, w, h, "", 0, em ? UI_FOCO : 0);
    uint16_t c = em ? C_BRANCO : C_MARFIM;
    int cx = x + w / 2 - 2, cy = y + h / 2 - 1;
    gfx_linha_grossa(cx + 4, cy - 7, cx - 3, cy, 3, c);          // a seta: so a ponta, para a esquerda
    gfx_linha_grossa(cx - 3, cy, cx + 4, cy + 7, 3, c);
    zona_clique(0, 0, VOLTAR_X0 - 4, FAIXA_H, NULL, 0, tecla);
    return VOLTAR_X0;
}

static void desenha_abertura(void)
{
    desenha_lance();
    if (!abertura_pronta) return;
    // O titulo entra letra a letra, sem pressa.
    const char *titulo = "DADO EM CASA";
    int n = (int)(t_fase / 0.05f);
    if (n > 12) n = 12;
    char parcial[16];
    memcpy(parcial, titulo, (size_t)n);
    parcial[n] = 0;
    int x0 = GFX_W / 2 - gfx_largura(titulo, 3) / 2;
    gfx_texto(x0 + 3, 70 + 3, parcial, C_SOMBRA, 3, true);
    gfx_texto(x0, 70, parcial, C_LATAO, 3, true);
    if (t_fase < 0.75f) return;
    if (menu_tela) botao_voltar(KEY_ESC);
    gfx_rect(GFX_W / 2 - 150, 134, 300, 2, C_LATAO_ESC);
    static const char *const SUB[] = { "Criado por ERICZ", "1 jogador", "Partida rápida · escolha o nível",
                                       "Modo Desafiante" };
    txt_c(GFX_W / 2, 140, SUB[menu_tela], menu_tela ? C_LATAO : C_TEXTO_M, menu_tela != 0);

    const char *op[MN_N];
    int n_op = menu_opcoes(op);
    for (int i = 0; i < n_op; i++) {
        int yb = 170 + i * 26;
        bool cur = i == menu_cur;
        ui_botao(GFX_W / 2 - 110, yb, 220, 24, op[i], cur ? C_LATAO : 0, 0);
        zona_clique(GFX_W / 2 - 110, yb, 220, 24, &menu_cur, i, KEY_ENTER);
    }
    // Nos submenus, uma linha explica a opcao em foco.
    char dica[96] = "";
    if (menu_tela == MT_UM)
        snprintf(dica, sizeof dica, "%s", menu_cur == 0 ? "15 rodadas contra o computador."
                                                          : "Suba os 3 salões do cassino. Perdeu, acabou.");
    else if (menu_tela == MT_NIVEL) {
        static const char *const NIV[] = { "Joga por regras simples.",
                                           "Pensa, mas erra de vez em quando.",
                                           "Calcula cada jogada." };
        snprintf(dica, sizeof dica, "%s", NIV[menu_cur]);
    } else if (menu_tela == MT_DESAFIO) {
        if (menu_cur == 0) des_resumo(dica, sizeof dica);
        else snprintf(dica, sizeof dica, "A run guardada será perdida.");
    }
    if (dica[0]) txt_c(GFX_W / 2, 170 + n_op * 26 + 8, dica, menu_cur == 1 && menu_tela == MT_DESAFIO
                                                          ? C_VINHO_CLR : C_MARFIM_S, false);
    txt_c(GFX_W / 2, GFX_H - FAIXA_H + 12, menu_tela
          ? "mouse ou setas escolhem  ·  clique ou ENTER confirma  ·  ESC volta"
          : "M som  ·  N música  ·  F11 tela cheia  ·  - e + tamanho da mesa",
          C_TEXTO_M, false);
    // A versao, miuda no canto da mesa: diz qual jogo esta aberto.
    char ver[32];
    snprintf(ver, sizeof ver, "versão %s", DADO_VERSAO);
    gfx_texto(MESA_X1 - 12 - gfx_largura(ver, GFX_MIUDO), MESA_Y1 - 18, ver,
              gfx_mistura(C_TEXTO_M, C_FELTRO, 35), GFX_MIUDO, false);
}

// ---------------------------------------------------------------------------
// Roleta do premio
// ---------------------------------------------------------------------------
static float slot_para(int i) { return slot_t0 + i * slot_dt; }   // quando o rolo i trava
static float slot_fim(void)   { return slot_para(2); }
static bool slot_girando(void) { return fase == F_PREMIO && t_slot >= 0 && t_slot < slot_fim(); }

// Os premios em ordem de raridade: o mais raro fica a direita e aparece por
// ultimo. Depois os rolos comecam a girar - no Desafiante com calma, nas
// partidas rapidas num piscar.
static void slot_comeca(void)
{
    for (int i = 1; i < 3; i++)
        for (int k = i; k > 0 && TIPO[oferta[k].tipo].raridade < TIPO[oferta[k - 1].tipo].raridade; k--) {
            peca_t t = oferta[k]; oferta[k] = oferta[k - 1]; oferta[k - 1] = t;
        }
    bool calma = modo == M_DESAFIO;
    slot_t0 = calma ? 0.95f : 0.35f;
    slot_dt = calma ? 0.6f : 0.2f;
    t_slot = 0;
    som_giro(true);
}

// Qualquer tecla (ou clique) durante o giro trava tudo de uma vez.
static void slot_pula(void)
{
    if (!slot_girando()) return;
    t_slot = slot_fim();
    som_giro(false);
}

static void solta_confete(int n)
{
    static const uint16_t COR[] = { C_OURO, C_AMARELO, C_ROSA, C_VERDE, C_REAL, C_BRANCO, C_VINHO_CLR };
    n_confete = n < MAX_CONFETE ? n : MAX_CONFETE;
    for (int i = 0; i < n_confete; i++) {
        confete_t *c = &confete[i];
        bool esq = i % 2 == 0;                   // dois canhoes, um em cada canto de baixo
        c->x = esq ? 30 + frnd(0, 20) : GFX_W - 30 - frnd(0, 20);
        c->y = GFX_H - 50 + frnd(-10, 10);
        c->vx = (esq ? 1 : -1) * frnd(60, 260);
        c->vy = -frnd(240, 470);
        c->giro = frnd(0, 6.28f);
        c->vg = frnd(-14, 14);
        c->cor = COR[i % (int)(sizeof COR / sizeof COR[0])];
        c->w = (uint8_t)(3 + (i % 3));
        c->h = (uint8_t)(2 + (i % 2));
    }
}

// Cacos de vidro: estouram do dado para todos os lados e caem.
static void solta_cacos(int x, int y)
{
    static const uint16_t COR[] = { C_VIDRO, C_BRANCO, C_ACO, C_VIDRO };
    for (int i = 0; i < 36 && n_confete < MAX_CONFETE; i++) {
        confete_t *c = &confete[n_confete++];
        float a = frnd(0, 6.2832f), v = frnd(60, 230);
        c->x = x + cosf(a) * 6;
        c->y = y + sinf(a) * 6;
        c->vx = cosf(a) * v;
        c->vy = sinf(a) * v - 140;
        c->giro = frnd(0, 6.28f);
        c->vg = frnd(-20, 20);
        c->cor = COR[i % 4];
        c->w = (uint8_t)(3 + i % 3);
        c->h = (uint8_t)(2 + i % 2);
    }
}

static void passo_confete(float dt)
{
    int m = 0;
    for (int i = 0; i < n_confete; i++) {
        confete_t *c = &confete[i];
        c->vy += 520 * dt;
        c->vx *= 1 - 1.4f * dt;
        if (c->vy > 90) c->vy = 90 + (c->vy - 90) * (1 - 3 * dt);   // papel plana ao cair
        c->x += c->vx * dt + sinf(c->giro) * 12 * dt;
        c->y += c->vy * dt;
        c->giro += c->vg * dt;
        if (c->y < GFX_H + 10) confete[m++] = *c;
    }
    n_confete = m;
}

static void desenha_confete(void)
{
    for (int i = 0; i < n_confete; i++) {
        const confete_t *c = &confete[i];
        float s = fabsf(cosf(c->giro));          // o papel virando no ar
        int w = (int)(c->w * (0.3f + 0.7f * s)) + 1;
        gfx_rect((int)c->x - w / 2, (int)c->y, w, c->h, s > 0.5f ? c->cor : gfx_mistura(c->cor, C_BARRA, 35));
    }
}

static void passo_slot(float dt)
{
    passo_confete(dt);
    if (t_slot < 0) return;
    if (fase != F_PREMIO) { t_slot = -1; som_giro(false); return; }
    float a = t_slot, fim = slot_fim();
    t_slot += dt;
    for (int i = 0; i < 3; i++)
        if (a < slot_para(i) && t_slot >= slot_para(i)) som_toca(SOM_SLOT);
    if (a < fim && t_slot >= fim) som_giro(false);
    if (a < fim + 0.1f && t_slot >= fim + 0.1f) {
        som_toca(SOM_PREMIO);
        solta_confete(modo == M_DESAFIO ? MAX_CONFETE : 60);
    }
    if (t_slot > fim + 5) t_slot = fim + 5;
}

static void abre_premio(int w)
{
    for (int i = 0; i < 3; i++) {
        bool rep;
        do {
            oferta[i] = nova_peca(sorteia_tipo());
            rep = false;
            for (int k = 0; k < i; k++) if (oferta[k].tipo == oferta[i].tipo) rep = true;
        } while (rep);
    }
    vez = w;
    cursor = 0;
    removendo = false;
    t_tira = 0;
    fase = F_PREMIO;
    slot_comeca();
}

// ===========================================================================
// O aparelho como adversario
// ===========================================================================
// A IA joga pelas mesmas teclas que uma pessoa: poe o cursor onde quer e
// aperta ENTER ou TAB, com uma pausa antes de cada gesto para dar tempo de
// ver o que ela fez. So decide nas fases de escolha; o resto e animacao.
static void trata_tecla(int k);

static bool fase_de_escolha(void)
{
    return fase == F_LOJA || fase == F_ORDEM || fase == F_JOGA
        || fase == F_APOSTA || fase == F_PREMIO;
}

static bool ia_na_vez(void)
{
    return (cpu >> vez & 1) && fase_de_escolha();
}

// Valor medio que o dado mostra ao cair: e o que decide a queda da fila.
static float media(peca_t p)
{
    const tipo_t *t = &TIPO[p.tipo];
    switch (t->efeito) {
    case EF_QUEBRADO: return p.fixo;
    case EF_LASTRO:   return 8.0f;
    case EF_VICIADO:  return 4.47f;
    case EF_EXPLODE:  return 5.1f;
    default:          return (t->lados + 1) / 2.0f;
    }
}

// Dados que so atrapalham a fila de quem nao sabe o que esta fazendo.
static bool arriscado(int tipo) { return tipo == D_MALDITO || tipo == D_TUDO_NADA || tipo == D_EGOISTA; }

// Escolha simples, sem imaginar jogadas: os de maior media, do menor para o
// maior (assim cada um tem boa chance de passar a barra do anterior), sem os
// arriscados quando da. E como a IA imagina a fila do rival, e a sua propria
// quando mede uma compra.
static int fila_simples(const peca_t *pc, int n, int *ordem)
{
    int quer = n < N_FILA ? n : N_FILA, ok = 0;
    for (int i = 0; i < n; i++) if (!arriscado(pc[i].tipo)) ok++;
    bool sem_risco = ok >= quer;
    bool esc[MAX_COL + 1] = { false };
    int m = 0;
    for (int q = 0; q < quer; q++) {
        int b = -1;
        for (int i = 0; i < n; i++) {
            if (esc[i] || (sem_risco && arriscado(pc[i].tipo))) continue;
            if (b < 0 || media(pc[i]) > media(pc[b])) b = i;
        }
        if (b < 0) break;
        esc[b] = true;
        ordem[m++] = b;
    }
    for (int a = 1; a < m; a++)
        for (int b = a; b > 0 && media(pc[ordem[b]]) < media(pc[ordem[b - 1]]); b--) {
            int t = ordem[b]; ordem[b] = ordem[b - 1]; ordem[b - 1] = t;
        }
    // O Dobro dobra o dado seguinte: por ultimo nao serve. Vai uma casa antes.
    for (int b = m - 1; b > 0; b--)
        if ((pc[ordem[b]].tipo == D_DOBRO || pc[ordem[b]].tipo == D_FERREIRO) && b == m - 1) {
            int t = ordem[b]; ordem[b] = ordem[b - 1]; ordem[b - 1] = t;
        }
    return m;
}

// ---- Rodadas imaginadas -----------------------------------------------------
// A IA decide jogando de mentira com as mesmas regras (aplica_regra e
// calcula_desfecho) sobre a mesa de verdade, que depois volta como estava.
// O sorteio e dela: pensar nao mexe nos dados da partida.
static uint32_t semente_ia = 0x2545F491u;
static uint32_t rnd_ia(void)
{
    semente_ia ^= semente_ia << 13; semente_ia ^= semente_ia >> 17; semente_ia ^= semente_ia << 5;
    return semente_ia;
}
static int rola_ia(int lados) { return 1 + (int)(rnd_ia() % (uint32_t)lados); }
static void semeia_ia(uint32_t base, int amostra)
{
    semente_ia = base ^ (2654435761u * (uint32_t)(amostra + 1));
    if (!semente_ia) semente_ia = 1;
}

// Tudo o que as regras escrevem durante uma jogada.
typedef struct {
    fila_t f[2];
    char ver[sizeof veredito];
    int vt, aa, ab, res, prim;
    uint8_t am;
} mesa_t;

static void guarda_mesa(mesa_t *m)
{
    memcpy(m->f, F, sizeof F);
    memcpy(m->ver, veredito, sizeof veredito);
    m->vt = ver_tipo; m->aa = anulou_a; m->ab = anulou_b; m->res = resistiu;
    m->am = anulou_masc; m->prim = primeiro;
}
static void volta_mesa(const mesa_t *m)
{
    memcpy(F, m->f, sizeof F);
    memcpy(veredito, m->ver, sizeof veredito);
    ver_tipo = m->vt; anulou_a = m->aa; anulou_b = m->ab; resistiu = m->res;
    anulou_masc = m->am; primeiro = m->prim;
}

static void sim_lanca(int j)
{
    fila_t *f = &F[j];
    int k = f->lancados, menor;
    int x = sorteia_valor(f->tipo[k], f->fixo[k], rola_ia, &menor);
    f->cru[k] = f->valor[k] = x;
    f->descarte[k] = (uint8_t)menor;
    f->lancados = k + 1;
    pousa(j, k, rola_ia);
}

// Quem abre nao sabe o alvo: olha o risco do proximo dado. O Egoista
// trocaria a fila inteira por um d12: so compensa com pouco na mesa.
static bool abre_continua(int j)
{
    const fila_t *f = &F[j];
    if (tipo_de(j, f->lancados) == D_EGOISTA) return f->pontos < 6;
    int r = risco(j);
    return r < 40 || (r < 60 && f->pontos < 8);
}

// Placar de j com a mesa como esta: 1 vence, 0.5 empata, 0 perde.
static float placar(int j)
{
    desfecho_t d;
    calcula_desfecho(&d, false);
    int a = d.total[j], b = d.total[outro(j)];
    return a > b ? 1.0f : (a == b ? 0.5f : 0.0f);
}

// Joga o turno de j ate o fim da fila ou ate parar. Quem joga depois para
// assim que passa na frente; quem abre segue o risco.
static void sim_turno(int j)
{
    fila_t *f = &F[j];
    while (f->lancados < f->n) {
        if (f->lancados > 0) {
            if (j != primeiro) { if (placar(j) == 1.0f) break; }
            else if (!abre_continua(j)) break;
        }
        sim_lanca(j);
    }
}

// Uma fila provavel para r. A mao de verdade a IA nao olha: sorteia uma entre
// os dados que ele ainda pode ter (ou a colecao toda, para medir compras) e
// escolhe pela regra simples.
static void fila_imaginada(int r, bool toda)
{
    jogador_t *p = &J[r];
    peca_t pool[MAX_COL];
    int n = 0;
    for (int i = 0; i < p->n_col; i++) if (toda || p->onde[i] != FORA) pool[n++] = p->col[i];
    if (n < N_FILA) { n = 0; for (int i = 0; i < p->n_col; i++) pool[n++] = p->col[i]; }
    int m = n < tam_mao(r) ? n : tam_mao(r);
    for (int i = 0; i < m; i++) {
        int k = i + (int)(rnd_ia() % (uint32_t)(n - i));
        peca_t t = pool[i]; pool[i] = pool[k]; pool[k] = t;
    }
    int ordem[N_FILA], q = fila_simples(pool, m, ordem);
    fila_t *f = &F[r];
    memset(f, 0, sizeof *f);
    for (int k = 0; k < q; k++) { f->tipo[k] = pool[ordem[k]].tipo; f->fixo[k] = pool[ordem[k]].fixo; }
    f->n = q;
}

// Termina a rodada de mentira do ponto de vista de j e devolve o placar dele.
// Se o rival ainda nao jogou, ele joga uma fila imaginada.
static float sim_resto(int j)
{
    int r = outro(j);
    if (j == primeiro) fila_imaginada(r, false);
    sim_turno(j);
    if (j == primeiro) sim_turno(r);
    return placar(j);
}

#define IA_AMOSTRAS 240
static int ia_esforco = 1;                   // o simulador pede menos para andar rapido
// Nivel da IA: 0 facil (a IA antiga, de regras simples), 1 medio (a que
// imagina jogadas, mas pensando pouco: erra de vez em quando), 2 dificil.
static int ia_nivel = 2;
static int nivel_jog[2] = { 2, 2 };          // o nivel de cada lado (so conta onde ha IA)
static float ousadia_jog[2];
// Ousadia nas apostas, de -1 (cautela) a +1 (aposta alto): o jeito de cada
// oponente do Modo Desafiante. 0 na partida rapida.
static float ia_ousadia;
// Quantas rodadas de mentira cabem numa decisao, pelo nivel.
static int amostras_ia(int cheio, int pouco)
{
    if (ia_esforco == 0) return pouco;
    if (ia_nivel == 1) return cheio / 6 > pouco ? cheio / 6 : pouco;
    return cheio;
}

// ---- A fila ----------------------------------------------------------------
// Testa toda ordem de ate quatro dados da mao, jogando cada uma varias vezes
// contra filas imaginadas do rival (ou contra a fila que ele ja jogou). Assim
// as sinergias (Dobro antes do maior, Fartura com dados baixos, Copiador,
// Jackpot...) aparecem sozinhas, sem regra escrita para cada uma.

static float avalia_fila(int j, const int8_t *seq, int L, int amostras, uint32_t base,
                         const mesa_t *m)
{
    float s = 0;
    for (int a = 0; a < amostras; a++) {
        volta_mesa(m);
        semeia_ia(base, a);
        fila_t *f = &F[j];
        memset(f, 0, sizeof *f);
        for (int k = 0; k < L; k++) {
            peca_t pc = J[j].col[J[j].mao[seq[k]]];
            f->tipo[k] = pc.tipo;
            f->fixo[k] = pc.fixo;
        }
        f->n = L;
        s += sim_resto(j);
    }
    return s / (float)amostras;
}

#define MAX_CAND 840                         // 7 x 6 x 5 x 4
static void ia_planeja(void)
{
    int j = vez;
    if (plano_rodada == rodada && plano_vez == j) return;
    jogador_t *p = &J[j];
    int n = p->n_mao, L = n < max_fila(j) ? n : max_fila(j);
    plano_rodada = rodada; plano_vez = j; plano_n = 0; plano_p = 0.5f;
    if (L <= 0) return;

    static int8_t cand[MAX_CAND][N_FILA];
    static float nota_c[MAX_CAND];
    static uint64_t assin[MAX_CAND];
    int nc = 0;
    // Todas as sequencias de L posicoes diferentes da mao. Sequencias com os
    // mesmos dados na mesma ordem (dois d6 trocados) contam uma vez so.
    int8_t s[N_FILA] = { 0 };
    int nivel = 0;
    s[0] = -1;
    while (nivel >= 0) {
        s[nivel]++;
        if (s[nivel] >= n) { nivel--; continue; }
        bool rep = false;
        for (int i = 0; i < nivel; i++) if (s[i] == s[nivel]) rep = true;
        if (rep) continue;
        if (nivel + 1 < L) { nivel++; s[nivel] = -1; continue; }
        // Dobro por ultimo nao dobra nada: a IA nem considera.
        if (L >= 2 && (p->col[p->mao[s[L - 1]]].tipo == D_DOBRO || p->col[p->mao[s[L - 1]]].tipo == D_FERREIRO)) continue;
        uint64_t a = 0;
        for (int k = 0; k < L; k++) {
            peca_t pc = p->col[p->mao[s[k]]];
            a = a * 512 + (uint64_t)(pc.tipo * 9 + pc.fixo);
        }
        bool visto = false;
        for (int c = 0; c < nc && !visto; c++) visto = assin[c] == a;
        if (visto || nc >= MAX_CAND) continue;
        assin[nc] = a;
        memcpy(cand[nc++], s, N_FILA);
    }

    mesa_t m;
    guarda_mesa(&m);
    uint32_t base = 0xA5A5u + (uint32_t)rodada * 7919u + (uint32_t)j * 104729u;
    // Primeira peneira com poucas jogadas; as melhores jogam de novo, muito mais.
    int a1 = nc > 60 ? 24 : 64;
    if (ia_esforco == 0) a1 = 4;
    else if (ia_nivel == 1) a1 = 6;
    for (int c = 0; c < nc; c++) nota_c[c] = avalia_fila(j, cand[c], L, a1, base, &m);
    int finalistas = ia_esforco == 0 ? 2 : (ia_nivel == 1 ? 3 : 8);
    int a2 = amostras_ia(IA_AMOSTRAS, 24);
    int melhor = -1;
    float nota_m = -1;
    for (int f = 0; f < finalistas && f < nc; f++) {
        int b = -1;
        for (int c = 0; c < nc; c++) if (nota_c[c] > -1 && (b < 0 || nota_c[c] > nota_c[b])) b = c;
        if (b < 0) break;
        nota_c[b] = -2;                      // ja foi finalista
        float v = avalia_fila(j, cand[b], L, a2, base ^ 0x5bd1e995u, &m);
        if (v > nota_m) { nota_m = v; melhor = b; }
    }
    volta_mesa(&m);
    if (melhor < 0) return;
    memcpy(plano, cand[melhor], N_FILA);
    plano_n = L;
    plano_p = nota_m;
}

static int ia_proximo_da_fila(void)
{
    ia_planeja();
    int k = F[vez].n;
    return k < plano_n ? plano[k] : -1;
}

// ---- Lancar ou parar -----------------------------------------------------
// Compara as duas coisas jogando de mentira: parar agora (e o rival
// responder, se ainda nao jogou) contra lancar mais um e seguir. Um empate
// certo nao vira derrota certa, como lancar um Quebrado que anula o seu 6.
static bool ia_continua(void)
{
    int j = vez;
    fila_t *f = &F[j];
    if (f->lancados >= f->n) return false;
    if (f->lancados == 0) return true;
    mesa_t m;
    guarda_mesa(&m);
    int amostras = amostras_ia(IA_AMOSTRAS, 24);
    uint32_t base = 0xC0FFEEu + (uint32_t)rodada * 31u + (uint32_t)f->lancados;
    float parar = 0, seguir = 0;
    for (int a = 0; a < amostras; a++) {
        volta_mesa(&m);
        semeia_ia(base, a);
        F[j].n = F[j].lancados;              // parar: a fila acaba aqui
        parar += sim_resto(j);
        volta_mesa(&m);
        semeia_ia(base, a);
        sim_lanca(j);
        seguir += sim_resto(j);
    }
    volta_mesa(&m);
    parar /= (float)amostras;
    seguir /= (float)amostras;
    // O Quebrado some da colecao quando e jogado: so vale a pena se ajudar.
    if (tipo_de(j, f->lancados) == D_QUEBRADO) seguir -= 0.02f;
    return seguir > parar + 0.005f;
}

// ---- Aposta ----------------------------------------------------------------
// Chance de vencer a rodada, vista por quem esta na vez.
static float ia_chance(void)
{
    int j = vez;
    if (F[j].lancados == 0) { ia_planeja(); return plano_p; }   // ainda vai jogar
    mesa_t m;
    guarda_mesa(&m);
    int amostras = amostras_ia(IA_AMOSTRAS, 24);
    float s = 0;
    for (int a = 0; a < amostras; a++) {
        volta_mesa(&m);
        semeia_ia(0xBE77u + (uint32_t)rodada, a);
        F[j].n = F[j].lancados;
        s += sim_resto(j);
    }
    volta_mesa(&m);
    return s / (float)amostras;
}

static int ia_aposta(void)
{
    int op[4], n = opcoes(op);
    float p = ia_chance();
    // A ousadia desloca as barras: quem e ousado aposta e dobra com menos
    // chance, e blefa mais; o cauteloso so aposta quando esta bem.
    float o = ia_ousadia * 0.12f;
    int quer;
    if (!a_pagar) {
        quer = p >= 0.72f - o ? OP_APOSTAR25 : (p >= 0.58f - o ? OP_APOSTAR10 : OP_PASSAR);
        int blefe = ia_ousadia > 0.5f ? 4 : (ia_ousadia < -0.5f ? 20 : 8);
        if (quer == OP_PASSAR && p < 0.35f && rnd_ia() % (uint32_t)blefe == 0) quer = OP_APOSTAR10;
    } else {
        // Pagar 'a_pagar' para disputar o pote, que ja tem a aposta do oponente.
        float precisa = (float)a_pagar / (float)(pote + a_pagar);
        if (p >= 0.75f - o) quer = OP_AUMENTAR;
        else quer = p >= precisa - o * 0.5f ? OP_PAGAR : OP_CORRER;
    }
    for (int i = 0; i < n; i++) if (op[i] == quer) return i;
    for (int i = 0; i < n; i++) if (op[i] == OP_PAGAR || op[i] == OP_PASSAR) return i;
    return 0;
}

// ---- Loja e premio -----------------------------------------------------------
// Forca da colecao de j, com um dado a mais (ou nenhum, extra < 0): chance
// media de vencer rodadas imaginadas, com maos sorteadas da colecao toda,
// contra a colecao do rival, metade abrindo e metade respondendo. Pega as
// sinergias com o que ja tem e tambem a diluicao: dado fraco atrapalha.
static float forca_colecao(int j, int extra, int fixo_extra, uint32_t base)
{
    jogador_t *p = &J[j];
    peca_t pool[MAX_COL + 1];
    int n = 0;
    for (int i = 0; i < p->n_col; i++) pool[n++] = p->col[i];
    if (extra >= 0) pool[n++] = (peca_t){ (uint8_t)extra, (uint8_t)fixo_extra };
    mesa_t m;
    guarda_mesa(&m);
    int amostras = amostras_ia(600, 40), r = outro(j);
    float s = 0;
    for (int a = 0; a < amostras; a++) {
        volta_mesa(&m);
        semeia_ia(base, a);
        primeiro = a & 1 ? j : r;
        fila_imaginada(r, true);
        peca_t mao[MAX_COL + 1];
        memcpy(mao, pool, sizeof pool);
        int q = n < tam_mao(j) ? n : tam_mao(j);
        for (int i = 0; i < q; i++) {
            int k = i + (int)(rnd_ia() % (uint32_t)(n - i));
            peca_t t = mao[i]; mao[i] = mao[k]; mao[k] = t;
        }
        int ordem[N_FILA], L = fila_simples(mao, q, ordem);
        fila_t *f = &F[j];
        memset(f, 0, sizeof *f);
        for (int k = 0; k < L; k++) { f->tipo[k] = mao[ordem[k]].tipo; f->fixo[k] = mao[ordem[k]].fixo; }
        f->n = L;
        sim_turno(primeiro);
        sim_turno(outro(primeiro));
        s += placar(j);
    }
    volta_mesa(&m);
    return s / (float)amostras;
}

// Loja: compra o que mais sobe a chance de vencer, pesando o preco: um ponto
// de chance a mais em cada rodada que falta vale mais ou menos um quarto do
// pote medio. Guarda sempre fichas para as apostas.
static int ia_compra(void)
{
    int j = vez;
    if (J[j].n_col >= lim_col()) return -1;
    uint32_t base = 0x51ED27u + (uint32_t)rodada * 977u + (uint32_t)J[j].n_col;
    float sem = forca_colecao(j, -1, 0, base);
    int faltam = MAX_RODADAS - rodada + 1;
    if (faltam < 1) faltam = 1;
    int b = -1;
    float melhor = 0;
    for (int i = 0; i < N_LOJA; i++) {
        int pr = preco_loja(loja[i].tipo);
        if (levou[j][i] || J[j].fichas - pr < 60) continue;
        float ganho = forca_colecao(j, loja[i].tipo, loja[i].fixo, base) - sem;
        float valor = ganho * (float)faltam * 25.0f - (float)pr;
        if (valor > melhor) { melhor = valor; b = i; }
    }
    return b;
}

// Premio: o que mais fortalece a colecao.
static int ia_premio(void)
{
    int j = vez, b = 0;
    uint32_t base = 0x9E37u + (uint32_t)rodada * 131u;
    float melhor = -1;
    for (int i = 0; i < 3; i++) {
        float v = forca_colecao(j, oferta[i].tipo, oferta[i].fixo, base);
        if (v > melhor) { melhor = v; b = i; }
    }
    return b;
}

// ---- A IA antiga, do nivel facil -----------------------------------------------
// Regras fixas, sem imaginar jogadas: a fila pelas medias, parar pelo risco,
// apostar pelos pontos na mesa, comprar e escolher premio pela raridade.
static int facil_proximo_da_fila(void)
{
    jogador_t *p = &J[vez];
    peca_t mao[MAO];
    for (int i = 0; i < p->n_mao; i++) mao[i] = p->col[p->mao[i]];
    int ordem[N_FILA], q = fila_simples(mao, p->n_mao, ordem);
    int k = F[vez].n;
    return k < q ? ordem[k] : -1;
}

static bool facil_continua(void)
{
    fila_t *f = &F[vez];
    if (f->lancados >= f->n) return false;
    if (f->lancados == 0) return true;
    if (vez != primeiro) {                     // parar agora ganha?
        desfecho_t d;
        calcula_desfecho(&d, false);
        return d.total[vez] <= d.total[outro(vez)];
    }
    return abre_continua(vez);
}

static int facil_aposta(void)
{
    int op[4], n = opcoes(op);
    int pts = F[primeiro].pontos;
    bool jogou = vez == primeiro;          // a IA ja tem pontos na mesa?
    int quer;
    if (!a_pagar) {
        if (jogou) quer = pts >= 20 ? OP_APOSTAR25 : (pts >= 13 ? OP_APOSTAR10 : OP_PASSAR);
        else       quer = pts <= 5 ? OP_APOSTAR10 : OP_PASSAR;
    } else if (jogou) {
        quer = pts <= 4 ? OP_CORRER : OP_PAGAR;
    } else {
        quer = (pts >= 22 && a_pagar >= 20) ? OP_CORRER
             : (pts <= 5 ? OP_AUMENTAR : OP_PAGAR);
    }
    for (int i = 0; i < n; i++) if (op[i] == quer) return i;
    for (int i = 0; i < n; i++) if (op[i] == OP_PAGAR || op[i] == OP_PASSAR) return i;
    return 0;
}

static int facil_compra(void)
{
    int b = -1;
    for (int i = 0; i < N_LOJA; i++) {
        int pr = preco_loja(loja[i].tipo);
        if (levou[vez][i] || J[vez].fichas - pr < 75 || J[vez].n_col >= lim_col()) continue;
        if (b < 0 || TIPO[loja[i].tipo].raridade > TIPO[loja[b].tipo].raridade
            || (TIPO[loja[i].tipo].raridade == TIPO[loja[b].tipo].raridade
                && pr > preco_loja(loja[b].tipo)))
            b = i;
    }
    return b;
}

static int facil_premio(void)
{
    int b = 0;
    for (int i = 1; i < 3; i++)
        if (TIPO[oferta[i].tipo].raridade > TIPO[oferta[b].tipo].raridade) b = i;
    return b;
}

static void ia_passo(float dt)
{
    ia_nivel = nivel_jog[vez & 1];
    ia_ousadia = ousadia_jog[vez & 1];
    if (!ia_na_vez() || (fase == F_PREMIO && (t_tira > 0 || slot_girando()))) { ia_fase = -1; return; }
    if (fase != ia_fase || vez != ia_vez) {   // nova decisao: pensa um pouco
        ia_fase = fase; ia_vez = vez;
        t_ia = fase == F_ORDEM ? -0.4f : 0;
    }
    t_ia += dt;
    if (t_ia < (fase == F_JOGA ? 0.3f : 0.55f)) return;
    t_ia = 0;

    switch (fase) {
    case F_LOJA: {
        int i = ia_nivel == 0 ? facil_compra() : ia_compra();
        if (i >= 0) { cursor = i; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    }
    case F_ORDEM: {
        int i = ia_nivel == 0 ? facil_proximo_da_fila() : ia_proximo_da_fila();
        if (i >= 0) { cursor = i; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    }
    case F_JOGA:
        if (ia_nivel == 0 ? facil_continua() : ia_continua()) { cursor = 0; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    case F_APOSTA:
        cursor = ia_nivel == 0 ? facil_aposta() : ia_aposta();
        trata_tecla(KEY_ENTER);
        break;
    case F_PREMIO:
        cursor = ia_nivel == 0 ? facil_premio() : ia_premio();
        trata_tecla(KEY_ENTER);
        break;
    }
}

// ===========================================================================
// Online: o rival joga pelas mesmas teclas, vindas da rede
// ===========================================================================
// Uma tecla so conta na vez de quem decide, e so numa fase de escolha: assim
// ela cai no mesmo ponto da partida dos dois lados, por mais que as
// animacoes de cada computador andem em ritmos diferentes.
static bool aceita_escolha(void)
{
    return fase_de_escolha() && !(fase == F_PREMIO && t_tira > 0);
}

static bool rede_na_vez(void) { return (remoto >> vez & 1) && aceita_escolha(); }

// Digitando o codigo da sala: todas as letras sao do codigo, ate M e N.
bool dado_digitando(void) { return (fase == F_ONLINE && digitando) || fase == F_PCRIA; }

void dado_rede_tecla(int k)
{
    int prox_i = (rede_fim + 1) % FILA_REDE;
    if (prox_i == rede_ini) return;          // fila cheia: nunca numa partida normal
    rede_q[rede_fim] = (uint16_t)k;
    rede_fim = prox_i;
}

void dado_online_comeca(uint32_t sem, int eu)
{
    modo = M_ONLINE;
    cpu = 0;
    baixo = eu;
    remoto = 1 << outro(eu);
    semente_online = sem;
    rede_ini = rede_fim = 0;
    digitando = false;
    dado_inicia(0);
}

// Uma tecla do rival por quadro: da para ver o cursor dele andando.
static void rede_passo(void)
{
    if (rede_ini == rede_fim || !rede_na_vez()) return;
    int k = rede_q[rede_ini];
    rede_ini = (rede_ini + 1) % FILA_REDE;
    trata_tecla(k);
}

static bool conexao_caiu(void)
{
    return modo == M_ONLINE && fase != F_ONLINE && rede_estado() != REDE_JOGANDO;
}

// Volta ao menu de abertura, largando a partida (e a sala, se online).
static void volta_ao_menu(void)
{
    if (modo == M_ONLINE) rede_sai();
    modo = M_UM;
    cpu = remoto = 0;
    baixo = 0;
    dado_inicia(partidas + 1);
    for (int i = 0; i < n_voo; i++) { voo[i].parado = true; voo[i].vx = voo[i].vy = 0; }
    abertura_pronta = true;
    t_fase = 1.0f;
}

// Um nivel acima no menu (ESC ou BACKSPACE num submenu).
static void menu_volta(void)
{
    if (menu_tela == MT_NIVEL || menu_tela == MT_DESAFIO) { menu_cur = menu_tela == MT_NIVEL ? 0 : 1; menu_tela = MT_UM; }
    else { menu_tela = MT_PRINCIPAL; menu_cur = MN_UM; }
    sacode = 1;
}

static void ao_menu_sem_partida(void)
{
    t_fase = 1.0f;
    fase = F_ABERTURA;
}

// ESC: nas telas do menu, volta; no meio da partida, pede um segundo ESC.
static void trata_esc(void)
{
    switch (fase) {
    case F_ABERTURA: if (menu_tela) menu_volta(); return;
    case F_CATALOGO: case F_TUTORIAL: ao_menu_sem_partida(); return;
    case F_MAPA: case F_RLOJA: case F_RBOLSA: case F_RFIM:
        volta_ao_menu();                     // a run fica guardada
        return;
    case F_PERFIS: case F_PCRIA: case F_RDIFIC: case F_REVENTO:
        des_esc();
        return;
    case F_ONLINE:
        if (digitando) { digitando = false; return; }
        rede_sai();
        ao_menu_sem_partida();
        return;
    default:
        if (conexao_caiu() || t_sair >= 0) volta_ao_menu();
        else t_sair = 0;
        return;
    }
}

// ===========================================================================
// Passo de tempo
// ===========================================================================
// Telas de menu, fora de qualquer partida: nelas a velocidade nao muda nada.
static bool menu_sem_partida(void)
{
    return fase == F_ABERTURA || fase == F_CATALOGO || fase == F_TUTORIAL || fase == F_ONLINE
        || fase == F_PERFIS || fase == F_PCRIA || fase == F_RDIFIC;
}

void dado_passo(float dt)
{
    float dt_real = dt;                  // baloes e avisos: no tempo de quem le
    if (rapido && modo != M_ONLINE && !menu_sem_partida()) dt *= VEL_RAPIDO;
    if (t_aviso_vel > 0) t_aviso_vel -= dt_real;
    if (t_aviso_entrada > 0) t_aviso_entrada -= dt_real;
    if (t_elev > 0) {
        float antes = ELEV_T - t_elev;
        t_elev -= dt_real;
        if (antes < ELEV_CHEGA && ELEV_T - t_elev >= ELEV_CHEGA) som_toca(SOM_VALIDO);   // o "dim" do elevador
    }
    if (t_virada > 0) { t_virada -= dt_real; return; }   // o anuncio da virada para a mesa
    t_fase += dt;
    passo_slot(dt);
    for (int a = 0; a < A_N; a++) if (am_brilho[a] > 0) am_brilho[a] -= dt;
    if (t_fala > 0) t_fala -= dt_real;
    ia_passo(dt);
    rede_passo();
    if (t_sair >= 0 && (t_sair += dt_real) > 2.5f) t_sair = -1;
    switch (fase) {
    case F_ABERTURA:
        if (!abertura_pronta) {
            if (fisica(dt) && t_fase > 0.4f) {
                abertura_pronta = true;
                t_fase = 0;
                som_toca(SOM_ABERTURA);
            }
        } else {
            // O crupie afasta os dados para os cantos e abre espaco ao menu.
            static const float ALVO[2][2] = { { 92, 250 }, { 548, 250 } };
            float a = dt * 5 > 1 ? 1 : dt * 5;
            if (sacode > 0) sacode -= dt / 0.85f;
            if (sacode < 0) sacode = 0;
            for (int i = 0; i < n_voo; i++) {
                voo[i].x += (ALVO[i][0] - voo[i].x) * a;
                voo[i].y += (ALVO[i][1] - voo[i].y) * a;
                voo[i].giro *= 1 - a;
            }
        }
        break;
    case F_CATALOGO:
        break;
    case F_PREMIO:
        if (t_tira > 0) {
            t_tira += dt;
            if (t_tira > 0.7f) {
                jogador_t *p = &J[vez];
                for (int m = tira_i; m + 1 < p->n_col; m++) {
                    p->col[m] = p->col[m + 1];
                    p->onde[m] = p->onde[m + 1];
                }
                p->n_col--;
                p->n_mao = 0;
                t_tira = 0;
                removendo = false;
                if (modo == M_DESAFIO) des_premio_feito();
                else nova_rodada();
            }
        }
        break;
    case F_ROLANDO: {
        // Camera lenta: o dado rola devagar, e o quarto mais devagar ainda -
        // e nele que a rodada se decide.
        // O ultimo dado da sequencia rola devagar: e nele que a rodada se
        // decide. Na vez da IA, os outros andam mais rapido.
        bool ia = (cpu >> voo[0].dono & 1) != 0;
        bool ultimo = voo[0].slot == F[voo[0].dono].n - 1;
        float lento = ultimo ? 0.42f : (ia ? 1.0f : 0.62f);
        if (fisica(dt * lento) && t_fase > (ia && !ultimo ? 0.25f : 0.5f)) {
            voo[0].ox = voo[0].x; voo[0].oy = voo[0].y;
            t_fase = 0;
            fase = F_ARRUMA;
        }
        break;
    }
    case F_ARRUMA: {
        bool ia_arr = (cpu >> voo[0].dono & 1) && voo[0].slot != F[voo[0].dono].n - 1;
        float t = t_fase / (ia_arr ? 0.28f : 0.45f);
        if (t > 1) t = 1;
        float e = t * t * (3 - 2 * t);
        voo_t *v = &voo[0];
        v->x = v->ox + (SLOT_X(v->slot) - v->ox) * e;
        v->y = v->oy + (zona(v->dono) - v->oy) * e;
        v->giro *= (1 - e);
        if (t >= 1) {
            int j = v->dono, k = v->slot;
            F[j].lancados = k + 1;
            n_voo = 0;
            som_rufo(false);
            am_disparou = 0;
            pousa(j, k, rola);
            for (int a = 0; a < A_N; a++) if (am_disparou >> a & 1) am_acende(a);
            if ((am_disparou >> A_FERRADURA & 1) && ferr_k[j] < 0) ferr_k[j] = (int8_t)k;
            // A Ventania tem o som dela quando rola o dado de antes.
            if (k > 0 && F[j].tipo[k] == D_VENTANIA) som_toca(SOM_VENTO);
            else if (F[j].tipo[k] == D_VIDRO && F[j].cru[k] == 1) {
                som_toca(SOM_VIDRO);
                solta_cacos(SLOT_X(k), zona(j));
            } else if (ver_tipo == 0) {             // valendo: uma nota acima a cada posicao
                static const int SOM_POS[N_FILA] = { SOM_VALIDO, SOM_VALIDO2, SOM_VALIDO3, SOM_VALIDO4 };
                som_toca(SOM_POS[k < N_FILA ? k : N_FILA - 1]);
            } else som_toca(SOM_ANULA);
            if (j == 0 && ver_tipo == 1) fala_evento(FA_CAI);
            t_fase = 0;
            fase = F_VEREDITO;
        }
        break;
    }
    case F_VEREDITO:
        // Na vez da IA o veredito passa mais rapido (menos no ultimo dado).
        bool ia_v = (cpu >> vez & 1) && F[vez].lancados < F[vez].n;
        if (t_fase > (ver_tipo == 0 ? (ia_v ? 0.35f : 0.6f) : (ia_v ? 0.8f : 1.3f))) {
            if (F[vez].lancados >= F[vez].n) termina_turno();
            else { cursor = 0; fase = F_JOGA; }
        }
        break;
    case F_EFEITOS:
        if (t_fase > duracao_evento(ev_i)) {
            aplica_evento(ev_i);
            ev_i++;
            t_fase = 0;
            if (ev_i >= n_ev) conclui_rodada();
            else som_toca(som_do_evento(ev_i));
        }
        break;
    default:
        break;
    }
}

// ===========================================================================
// Desenho
// ===========================================================================
static int txt_nome(int x, int y, int j, bool destaque);

static void desenha_lance(void)
{
    for (int i = n_voo - 1; i >= 0; i--) {
        voo_t *v = &voo[i];
        float r = R_DADO + (v->pouso > 0 ? v->pouso * 16 : 0);
        bool descartado = false;
        // O lance menor do Viciado: riscado e encolhendo enquanto o maior
        // vai para a fila.
        if (v->fantasma && fase == F_ARRUMA) {
            float q = t_fase / 0.45f;
            q = q > 1 ? 1 : q;
            r = R_DADO * (1 - q * 0.4f);
            descartado = true;
        }
        // Poeira do feltro quando o dado assenta.
        if (v->pouso > 0 && !v->fantasma) {
            float q = 1 - v->pouso / 0.18f;
            for (int k = 0; k < 10; k++) {
                float a = k * 0.628f + v->x * 0.01f;
                int px = (int)(v->x + cosf(a) * (R_DADO + q * 20));
                int py = (int)(v->y + sinf(a) * (R_DADO + q * 20) * 0.6f + R_DADO * 0.3f);
                int t = q < 0.5f ? 3 : 2;
                gfx_rect(px - 1, py - 1, t, t, gfx_mistura(C_FELTRO2, C_MARFIM_S, 30 - (int)(q * 20)));
            }
        }
        // Rolando: quica, e a sombra se afasta quando ele esta no alto.
        sombra_alt = 0;
        if (!v->parado) {
            float vel = sqrtf(v->vx * v->vx + v->vy * v->vy);
            float h = fabsf(sinf(v->giro * 0.9f)) * fminf(1, vel / 420);
            r *= 1 + 0.16f * h;
            sombra_alt = h * 7;
        }
        if (TIPO[v->tipo].lados == 2 && !v->parado && !descartado) {
            float c = cosf(v->giro * 1.6f);            // moeda gira no ar
            moeda_girando_arte(v->x, v->y, r, v->tipo, v->dono, c, c >= 0 ? v->face : 3 - v->face);
            sombra_alt = 0;
            continue;
        }
        // Na tela inicial, depois de assentar, os dados flutuam de leve (o
        // balanco entra devagar, sem pulo), e a sombra acompanha.
        // Quando o cursor do menu anda, chacoalham: pulinhos, um tremor de
        // lado e um giro curto, que se acalmam em menos de um segundo.
        float bob = 0, dx = 0, ang = v->giro;
        if (fase == F_ABERTURA && abertura_pronta && v->parado) {
            float entra = fminf(1, t_fase / 0.8f);
            float alto = (1 + sinf(t_fase * 2.2f + i * 1.9f)) * 3 * entra;   // 0..6 px
            float s = sacode * sacode;
            alto += s * 11 * fabsf(sinf(t_fase * 15 + i * 1.3f));
            dx = s * 3 * sinf(t_fase * 29 + i * 2.1f);
            ang += s * 0.32f * sinf(t_fase * 18 + i * 1.7f);
            bob = -alto;
            sombra_alt = alto * 0.6f;
        }
        desenha_dado(v->x + dx, v->y + bob, r, v->tipo, v->dono, ang,
                     v->parado ? v->valor : v->face, 2, descartado);
        sombra_alt = 0;
    }
    sombra_alt = 0;
}

static void peca_parada(float x, float y, float r, peca_t p, int dono)
{
    marca_nivel = p.fixo;
    desenha_dado(x, y, r, p.tipo, dono, 0, p.fixo, 0, false);
    marca_nivel = -1;
}

// Moldura de destaque em volta de um dado da fila.
static void aro(int x, int y, uint16_t c)
{
    int m = R_DADO + 7;
    ui_contorno(x - m, y - m, 2 * m, 2 * m, c);
    ui_contorno(x - m + 1, y - m + 1, 2 * m - 2, 2 * m - 2, c);
}

// Etiqueta no canto do dado (x2, +8, =9), com fundo para ler sobre o feltro.
static void etiqueta(int x, int y, const char *s)
{
    int w = gfx_largura(s, 1) + 8;
    int ex = x + R_DADO - 8, ey = y - R_DADO - 14;
    uint16_t c = s[0] == '-' ? C_VINHO_CLR : C_OURO;
    gfx_rect(ex + 1, ey + 1, w - 2, TXT_H, C_PLACA);
    ui_contorno(ex, ey, w, TXT_H + 2, c);
    txt(ex + 4, ey + 1, s, c, false);
}

// Painel escuro e translucido atras de um bloco de texto no meio da mesa.
static void painel(int y, int h, int w)
{
    ui_painel(GFX_W / 2 - w / 2, y, w, h);
}

// Largura de um painel: o que ele mostra, com folga dos lados.
static int larg_painel(int conteudo, int minimo)
{
    int w = conteudo + 36;
    return w < minimo ? minimo : w;
}

static void icone_bolsa(int cx, int cy);
static void amuletos_faixa(int j, int x, int y);   // Modo Desafiante (desafio.inc)
static int  retrato_faixa(int j, int x, int y0);
static int  fichas_vis(int j);   // devolve onde o nome comeca

static void faixa_jogador(int j)
{
    jogador_t *p = &J[j];
    bool cima = j != baixo;
    int y0 = cima ? 0 : GFX_H - FAIXA_H;
    gfx_rect(0, y0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, cima ? FAIXA_H - 2 : y0, GFX_W, 2, C_LATAO_ESC);
    int ty = y0 + (FAIXA_H - TXT_H) / 2 - 2;

    bool da_vez = fase != F_RESULTADO && fase != F_EFEITOS && fase != F_FIM
                  && fase != F_ROLANDO && fase != F_ARRUMA && vez == j;
    if (da_vez) gfx_rect(0, y0 + 2, 5, FAIXA_H - 4, p->cor);
    // Na faixa de cima, o "<" sai da partida (pede confirmacao, como o ESC):
    // no celular nao ha tecla.
    int x0 = cima ? botao_voltar(KEY_ESC) - 4 : 6;
    int xn = modo == M_DESAFIO ? retrato_faixa(j, x0, y0) : (cima ? x0 + 4 : 18);
    int x = xn + txt_nome(xn, ty, j, da_vez);
    ficha(x + 22, y0 + FAIXA_H / 2, j == 0 ? C_LATAO : C_ROSA);
    ficha_px[j] = x + 22; ficha_py[j] = y0 + FAIXA_H / 2;
    char s[48];
    snprintf(s, sizeof s, "%d", fichas_vis(j));
    txt(x + 36, ty, s, C_MARFIM, true);
    int fim_fichas = x + 36 + gfx_largura(s, 1);

    // A rodada mora na faixa de cima; o som, na de baixo.
    if (cima && rodada > 0 && fase != F_LOJA) {
        int max_r = modo == M_DESAFIO ? DES_RODADAS : MAX_RODADAS;
        if (rodada > max_r) snprintf(s, sizeof s, "rodada extra  ·  entrada %d", ante_da(rodada));
        else snprintf(s, sizeof s, "rodada %d de %d  ·  entrada %d", rodada, max_r, ante_da(rodada));
        int cx = GFX_W / 2, meio = gfx_largura(s, 1) / 2;
        if (cx - meio < fim_fichas + 20) cx = fim_fichas + 20 + meio;   // nome comprido: anda para a direita
        txt_c(cx, ty, s, t_aviso_entrada > 0 ? C_OURO : C_TEXTO_M, t_aviso_entrada > 0);
    }
    if (!cima && som_ligado()) icone_som(GFX_W / 2 - 8, y0 + FAIXA_H / 2 - 7);
    if (!cima && modo != M_ONLINE) {        // a velocidade, que o toque tambem troca
        const char *v = rapido ? "» rápido" : "» normal";
        int vx = GFX_W / 2 + 22;
        gfx_texto(vx, y0 + FAIXA_H / 2 - 10, v, rapido ? C_OURO : C_TEXTO_M, 1, false);
        zona_clique(vx - 4, y0 + 6, gfx_largura(v, 1) + 8, FAIXA_H - 12, NULL, 0, 'v');
    }
    if (modo == M_DESAFIO)                  // os do oponente, junto da bolsa (o meio e da rodada)
        amuletos_faixa(j, j == 0 ? x + 60 + gfx_largura(s, 1) : GFX_W - 142, y0 + FAIXA_H / 2);

    // A bolsa: quantos ainda estao nela, de quantos o jogador tem.
    int bx = GFX_W - 118, by = y0 + FAIXA_H / 2;
    icone_bolsa(bx + 8, by);
    snprintf(s, sizeof s, "bolsa %d/%d", na_bolsa(p), p->n_col);
    txt(bx + 22, ty, s, C_MARFIM_S, false);
}

// Saquinho de couro fechado por um cordao dourado; (cx, cy) e o meio dele.
static void icone_bolsa(int cx, int cy)
{
    const uint16_t borda = RGB(58, 36, 20), couro = RGB(150, 98, 54);
    const uint16_t claro = RGB(186, 132, 80), escuro = RGB(116, 72, 38);
    gfx_disco(cx, cy + 3, 7, borda);                        // o bojo
    gfx_disco(cx, cy + 3, 6, couro);
    gfx_disco(cx + 1, cy + 5, 4, escuro);                   // sombra embaixo, a direita
    gfx_disco(cx, cy + 4, 4, couro);
    gfx_rect(cx - 3, cy, 2, 2, claro);                      // brilho do couro
    gfx_rect(cx - 4, cy + 2, 1, 2, claro);
    gfx_rect(cx - 3, cy - 6, 7, 3, borda);                  // o pescoco franzido
    gfx_rect(cx - 2, cy - 5, 5, 2, escuro);
    gfx_rect(cx - 5, cy - 9, 11, 3, borda);                 // a boca, abrindo em leque
    gfx_rect(cx - 4, cy - 8, 9, 1, claro);
    gfx_rect(cx - 2, cy - 8, 1, 1, escuro);
    gfx_rect(cx + 1, cy - 8, 1, 1, escuro);
    gfx_rect(cx - 4, cy - 4, 9, 2, C_LATAO_ESC);            // o cordao
    gfx_rect(cx - 3, cy - 4, 7, 1, C_OURO);
    gfx_pixel(cx + 4, cy - 2, C_OURO);                      // a ponta solta
    gfx_pixel(cx + 5, cy - 1, C_OURO);
    gfx_pixel(cx + 5, cy, C_LATAO);
}

// Estilhacos do Quebrado: o dado se abre em lascas que se afastam e somem.
static void estilhaca(int x, int y, float t, int dono, int tipo)
{
    static const int8_t DIR[9][2] = { {-3,-4},{3,-4},{5,0},{3,4},{-3,4},{-5,0},{0,-5},{2,5},{-4,2} };
    uint16_t c = t < 0.5f ? (dono == 0 ? C_MARFIM : C_EBANO_B) : C_TERRA;
    if (tipo == D_VIDRO) c = t < 0.5f ? C_VIDRO : C_ACO;
    for (int i = 0; i < 9; i++) {
        int px = x + (int)(DIR[i][0] * t * 16), py = y + (int)(DIR[i][1] * t * 16);
        int tam = t < 0.6f ? 6 : 4;
        gfx_rect(px - tam / 2, py - tam / 2, tam, tam, c);
    }
}

// Projetil em arco de (ax, ay) a (bx, by); q vai de 0 (sai) a 1 (chega) e
// segue ate 1.5 com o anel do impacto se abrindo no alvo.
static void projetil(int ax, int ay, int bx, int by, float q, uint16_t c)
{
    if (q < 0) return;
    float arco = -fminf(90, fabsf((float)(by - ay)) * 0.25f + 40);
    for (int i = 5; i >= 0; i--) {                     // rastro
        float t = q - i * 0.05f;
        if (t < 0 || t > 1) continue;
        float x = ax + (bx - ax) * t, y = ay + (by - ay) * t + arco * 4 * t * (1 - t);
        int rr = i == 0 ? 5 : 4 - i / 2;
        gfx_disco((int)x, (int)y, rr, i == 0 ? C_BRANCO : gfx_mistura(c, C_FELTRO, i * 15));
        if (i == 0) gfx_disco((int)x, (int)y, 3, c);
    }
    if (q > 1 && q < 1.5f) {                           // impacto
        float t = (q - 1) / 0.5f;
        int rr = (int)(R_DADO * (0.6f + t * 1.2f));
        for (int a = 0; a < 32; a++) {
            float an = a * 0.19635f;
            gfx_rect(bx + (int)(cosf(an) * rr) - 1, by + (int)(sinf(an) * rr) - 1, 3, 3,
                     gfx_mistura(c, C_FELTRO, (int)(t * 80)));
        }
    }
}

// Faiscas douradas subindo de um dado que ganhou alguma coisa.
static void faiscas(int x, int y, float t, uint16_t c)
{
    for (int i = 0; i < 6; i++) {
        float f = fmodf(t * 1.3f + i * 0.17f, 1.0f);
        int px = x - R_DADO + (i * 37) % (2 * R_DADO) + (int)(sinf(t * 5 + i) * 3);
        int py = y + R_DADO - (int)(f * (2 * R_DADO + 16));
        int tam = f < 0.7f ? 2 : 1;
        gfx_rect(px - tam, py, 2 * tam + 1, 1, c);
        gfx_rect(px, py - tam, 1, 2 * tam + 1, c);
    }
}

// O Ventania de j acabou de cair e esta rolando o dado de antes?
static bool ventania_agora(int j)
{
    const fila_t *f = &F[j];
    return fase == F_VEREDITO && j == vez && f->lancados > 1
        && f->tipo[f->lancados - 1] == D_VENTANIA && t_fase < 0.7f;
}

static void desenha_fila(int j)
{
    fila_t *f = &F[j];
    int y = zona(j);
    bool no_desfecho = fase == F_EFEITOS || fase == F_RESULTADO;
    bool pisca = fase == F_VEREDITO && j == vez && ((int)(t_fase * 7) % 2 == 0);
    const evento_t *e = (fase == F_EFEITOS && ev_i < n_ev) ? &ev[ev_i] : NULL;
    float p = e ? t_fase / duracao_evento(ev_i) : 0;
    const int R = R_DADO;

    if (f->n == 0 && j != vez) return;                   // fila alheia ainda vazia

    // Previa do fim da rodada, calculada pelas regras de verdade: o que cada
    // dado ja ganhou de outro (Moeda da sorte, Copiador, multiplicadores) e
    // o total com isso e com o que o rival ja tirou. So durante o jogo; no
    // desfecho sao os eventos que mostram.
    bool previa = !no_desfecho && fase != F_FIM && (F[0].lancados || F[1].lancados);
    desfecho_t pv;
    if (previa) calcula_desfecho(&pv, false);

    for (int k = 0; k < f->n; k++) {
        int x = SLOT_X(k);
        bool voando = n_voo && (fase == F_ROLANDO || fase == F_ARRUMA)
                      && voo[0].dono == j && voo[0].slot == k;
        if (voando) continue;
        int t = tipo_de(j, k);
        dica_zona(x - R, y - R, 2 * R, 2 * R, DICA_DADO, t);
        if (k >= f->lancados) {
            gfx_disco(x, y, R - 2, C_FELTRO_ESC);                    // lugar marcado
            marca_nivel = f->fixo[k];
            desenha_dado((float)x, (float)y, R - 6, t, j, 0, f->fixo[k], 0, false);
            marca_nivel = -1;
            continue;
        }
        uint8_t est = no_desfecho ? vis_est[j][k] : f->est[k];

        // Dado sendo roubado agora: sai do lugar e atravessa a mesa.
        if (e && e->tipo == EV_ROUBA && e->alvo_j == j && e->alvo_k == k && p > 0.25f) {
            float q = (p - 0.25f) / 0.6f;
            if (q > 1) q = 1;
            float s = q * q * (3 - 2 * q);
            int tx = ROUBO_X(vis_nroubo[e->de_j]), ty = zona(e->de_j);
            desenha_dado(x + (tx - x) * s, y + (ty - y) * s, R + (R_ROUBO - R) * s, t, j, s * 6.28f,
                         f->valor[k], 2, false);
            continue;
        }
        if (est == V_ROUBADO) {                           // lugar vazio: foi levado
            gfx_disco(x, y, R - 2, C_FELTRO_ESC);
            continue;
        }
        if (fase == F_RESULTADO && f->quebra[k] && t_fase > 0.35f) {
            if (t_fase < 1.3f) estilhaca(x, y, (t_fase - 0.35f) / 0.95f, j, t);
            continue;
        }
        // Queimado pela Ficha de Fogo: se desfaz de cima para baixo em cinza.
        if (fase == F_RESULTADO && f->cinza[k] && t_fase > 0.35f) {
            float q = (t_fase - 0.35f) / 1.25f;
            if (q < 1) {
                desenha_dado((float)x, (float)y, R, t, j, 0, f->valor[k], 2, false);
                int topo = y - R - 6, corte = topo + (int)(q * (2 * R + 12));
                for (int yy = topo; yy < corte; yy++)            // o feltro volta
                    memcpy(display_fb() + yy * GFX_W + x - R - 6,
                           mesa_pronta + yy * GFX_W + x - R - 6, (size_t)(2 * R + 12) * 2);
                for (int i = 0; i < 9; i++) {
                    int ex = x - R + (i * 11) % (2 * R);
                    int ey = corte - ((int)(t_fase * 90) + i * 13) % 40;
                    gfx_rect(ex, ey, 4, 4, i % 2 ? C_FOGO : C_CINZA);
                }
            }
            continue;
        }
        bool anul = est == V_ANULADO;
        bool recente = fase == F_VEREDITO && j == vez
                       && (k == anulou_a || k == anulou_b || (anulou_masc >> k & 1));
        // O que segurou a queda treme no lugar, como quem balanca a cabeca.
        if (fase == F_VEREDITO && j == vez && k == resistiu && t_fase < 0.7f)
            x += ((int)(t_fase * 28) % 2) ? 4 : -4;
        if (recente && t_fase < 0.5f) x += (int)(sinf(t_fase * 60) * 3);   // treme antes de apagar
        if (recente && pisca) continue;
        // Alvo de anulacao agora: pisca antes de cair.
        bool alvo = e && e->tipo == EV_ANULA && e->alvo_j == j && e->alvo_k == k;
        if (alvo && p > 0.4f && ((int)(t_fase * 8) % 2 == 0)) continue;
        // O dado que acabou de valer da um pulinho.
        float pulo = 0;
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && ver_tipo == 0 && t_fase < 0.25f)
            pulo = sinf(t_fase / 0.25f * 3.1416f) * 8;
        // Espelho que so vira no fim: mostra o que tirou ate o evento dele
        // chegar na metade, e entao o numero do rival.
        bool ainda = no_desfecho && vis_espelho[j][k]
                     && !(e && e->tipo == EV_ESPELHO && e->de_j == j && e->de_k == k && p > 0.5f);
        // Ventania: o dado de antes rodopia mostrando faces ao acaso e
        // assenta no numero novo.
        int face = ainda ? f->cru[k] : f->valor[k];
        float giro = 0;
        if (ventania_agora(j) && k == f->lancados - 2) {
            float q = t_fase / 0.7f;
            giro = (1 - q) * (1 - q) * 9;
            x += (int)(sinf(t_fase * 40 + k) * (1 - q) * 3);
            if (q < 0.7f) face = face_v(t, f->valor[k]);
        }
        marca_nivel = f->fixo[k];
        desenha_dado((float)x, y - pulo, R, t, j, giro, face, 2, anul && giro < 0.5f);
        marca_nivel = -1;
        // Etiqueta: no desfecho, o multiplicador somado (x4) ou o bonus da
        // Moeda (+2); antes, o aviso do que vem (x2 do Dobro, +8, =9).
        char tg[16] = "";
        if (no_desfecho && vis_mult[j][k] > 1) snprintf(tg, sizeof tg, "x%d", vis_mult[j][k]);
        else if (no_desfecho && vis_bonus[j][k]) snprintf(tg, sizeof tg, "+%d", vis_bonus[j][k]);
        else if (ainda) snprintf(tg, sizeof tg, "=?");
        else if (f->tag[k][0]) snprintf(tg, sizeof tg, "%s", f->tag[k]);
        // Bonus que este dado ja recebeu de outros, pela previa. O x2 do
        // Dobro e o +2 do Copiador ficam por conta dela.
        int extra = 0;
        if (previa && f->est[k] == V_VALIDO && pontua(j, k)) {
            int base = valor_pontos(j, k) + f->bonus[k];
            int m = pv.mult[j][k] > 0 ? pv.mult[j][k] : 1;
            int fim = pv.est[j][k] == V_VALIDO ? (base + pv.bonus[j][k]) * m : 0;
            extra = fim - base;
        }
        bool do_fim = (!strcmp(tg, "x2") && f->dobra[k]) || (!strcmp(tg, "+2") && copia_antes(j, k))
                      || (!strcmp(tg, "+6") && k > 0 && tipo_de(j, k - 1) == D_FERREIRO);
        if (previa && do_fim) tg[0] = 0;
        // Egoista chegando: uma onda purpura se abre dele e derruba os outros.
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && t == D_EGOISTA
            && anulou_masc && t_fase < 0.9f) {
            for (int o = 0; o < 2; o++) {
                int rr = (int)(t_fase * 260) - o * 26;
                if (rr <= R_DADO) continue;
                for (int a = 0; a < 48; a++) {
                    float an = a * 0.1309f;
                    gfx_rect(x + (int)(cosf(an) * rr), y + (int)(sinf(an) * rr * 0.45f), 3, 3, C_EGO);
                }
            }
        }
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && ver_tipo == 0) {
            aro(x, y, C_OURO);
            // Trevo com 4: SORTE em dourado, com faiscas em volta.
            if (t == D_TREVO && f->valor[k] == 4) {
                int ty = j == baixo ? y - R - 34 : y + R + 12;
                gfx_rect(x - 28, ty - 2, 56, TXT_H + 4, C_BARRA);
                txt_c(x, ty, "SORTE", C_OURO, true);
                for (int i = 0; i < 6; i++) {
                    float a = t_fase * 5 + i * 1.0472f;
                    int sx = x + (int)(cosf(a) * 36), sy = y + (int)(sinf(a) * 32);
                    gfx_rect(sx - 3, sy, 7, 1, C_OURO);
                    gfx_rect(sx, sy - 3, 1, 7, C_OURO);
                }
            }
        }
        if (e && e->de_j == j && e->de_k == k) aro(x, y, C_VINHO_CLR);   // quem age
        if (alvo) aro(x, y, C_VINHO_CLR);
        // Bonus e multiplicadores: aro dourado nos dados que recebem.
        if (e && (e->tipo == EV_BONUS || e->tipo == EV_MULT || e->tipo == EV_JACKPOT) && e->de_j == j
            && (e->mascara >> k & 1) && p > 0.2f) {
            aro(x, y, C_OURO);
            faiscas(x, y, t_fase + k * 0.3f, C_AMARELO);
        }
        if (e && e->tipo == EV_BONUS && e->de_j == j && e->de_k == k) aro(x, y, C_OURO);
        // Marcado pelo fogo: aro em brasa ate o fim da rodada.
        if (no_desfecho && (vis_queima[j][k] ||
            (e && e->tipo == EV_FOGO && e->alvo_j == j && e->alvo_k == k && p > 0.35f)))
            aro(x, y, ((int)(t_fase * 10) % 2) ? C_FOGO : C_BRASA);
        if (e && e->tipo == EV_ZERA && e->de_j == j && p > 0.15f && ((int)(t_fase * 9) % 2 == 0))
            aro(x, y, C_VIOLETA);
        if (e && (e->tipo == EV_FOGO || e->tipo == EV_SEMENTE) && e->de_j == j && e->de_k == k)
            aro(x, y, e->tipo == EV_FOGO ? C_FOGO : C_VERDE);
        if (e && e->tipo == EV_EXTRA && e->de_j == j && e->de_k == k) aro(x, y, cor_aro(t));
        if (tg[0] && !anul) etiqueta(x, y, tg);        // por cima dos aros
        // O bonus, miudo e apagado, no canto de cima a direita (depois da
        // etiqueta, se houver uma).
        if (extra && !anul) {
            char b[8];
            snprintf(b, sizeof b, "%+d", extra);
            int bx = tg[0] ? x + R + 3 + gfx_largura(tg, 1) : x + R - 4, by = y - R - 10;
            uint16_t cb = extra > 0 ? gfx_mistura(C_MARFIM_S, C_FELTRO, 35)
                                    : gfx_mistura(C_VINHO_CLR, C_FELTRO, 30);
            gfx_texto(bx, by, b, cb, GFX_MIUDO, false);
        }
    }
    // Rajadas da Ventania saindo dela e passando pelo dado de antes.
    if (ventania_agora(j))
        for (int i = 0; i < 7; i++) {
            float q = t_fase / 0.7f;
            int xs = SLOT_X(f->lancados - 1) + 10 - (int)fmodf(t_fase * 500 + i * 61, 160.0f);
            int ys = y - 24 + i * 8;
            gfx_rect(xs, ys, 30 + (i % 3) * 10, 1, gfx_mistura(C_VENTO, C_FELTRO, 30 + (int)(q * 60)));
        }
    // O par que se anulou fica unido por um traco.
    if (fase == F_VEREDITO && j == vez && anulou_a >= 0 && anulou_b >= 0)
        gfx_rect(SLOT_X(anulou_a) + R + 4, y - 2, SLOT_X(anulou_b) - SLOT_X(anulou_a) - 2 * R - 8, 5,
                 C_VINHO_CLR);

    // Dados roubados por este jogador, a esquerda da fila.
    if (no_desfecho)
        for (int i = 0; i < vis_nroubo[j]; i++) {
            roubado_t *r = &vis_roubo[j][i];
            desenha_dado((float)ROUBO_X(i), (float)y, R_ROUBO, r->tipo, r->de_j, 0, r->valor, 2, false);
        }

    if (f->lancados || vis_nroubo[j] || (previa && pv.bruto[j] != 0)) {
        char s[16];
        int pts = fase == F_RESULTADO && venc_mao >= 0 ? f->total
                : no_desfecho ? total_visivel(j) : f->pontos;
        // Rodada zerando: o placar desce ate 0 durante o evento.
        if (e && e->tipo == EV_ZERA && e->de_j == j) {
            float q = p / 0.7f;
            pts = (int)(pts * (q > 1 ? 0 : 1 - q));
        }
        snprintf(s, sizeof s, "%d", pts);
        uint16_t c = (fase == F_RESULTADO && venc_mao == j) ? J[j].cor
                   : f->zerada ? C_VIOLETA
                   : (j == vez && !no_desfecho ? C_MARFIM : C_TEXTO_M);
        ui_painel(PLACAR_X - 34, y - 26, 68, 50);
        txt_c(PLACAR_X, y - 42 + (j == baixo ? 72 : 0), "pontos", C_TEXTO_M, false);
        txt_c2(PLACAR_X, y - 20, s, c);
        // Ao lado, apagado: quanto fica depois dos bonus e dos ataques que ja
        // estao na mesa. Pode ser negativo (Espinhoso, Pirata, Maldito do rival).
        if (previa && pv.bruto[j] != pts) {
            snprintf(s, sizeof s, "(%d)", pv.bruto[j]);
            txt(PLACAR_X + 38, y - 10, s, gfx_mistura(C_TEXTO_M, C_FELTRO, 25), false);
        }
    }
}

// O que o evento em curso desenha por cima da mesa: o traco do ataque, os
// pontos que saem de um placar e entram no outro.
static void desenha_evento(void)
{
    if (fase != F_EFEITOS || ev_i >= n_ev) return;
    const evento_t *e = &ev[ev_i];
    float p = t_fase / duracao_evento(ev_i);
    if (e->tipo == EV_CHEFE) {                   // a virada do Barao: o +N sobe no placar dele
        char t[8];
        snprintf(t, sizeof t, "+%d", e->valor);
        txt_sombra_c(PLACAR_X, zona(e->de_j) - 10 - (int)(p * 30), t, C_VINHO_CLR, 1);
        return;
    }
    int ax = SLOT_X(e->de_k), ay = zona(e->de_j);
    // Ataques: um projetil em arco do dado que age ate o alvo (o dado, ou o
    // placar do rival), com rastro, e um anel de impacto quando chega.
    if (e->tipo == EV_ANULA || e->tipo == EV_TIRA) {
        bool praga = TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_MALDICAO;
        uint16_t c = praga ? C_VIOLETA : C_VINHO_CLR;
        int bx = e->tipo == EV_ANULA ? SLOT_X(e->alvo_k) : PLACAR_X, by = zona(e->alvo_j);
        if (TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_LASER) {
            // Laser: um raio verde reto sai do emissor e chega ao alvo,
            // tremulando, e fica aceso um instante antes do impacto.
            float q = p / 0.3f;
            if (q > 1) q = 1;
            int ex = ax + (int)((bx - ax) * q), ey = ay - R_DADO + (int)((by - (ay - R_DADO)) * q);
            if (p < 0.6f) {
                int tr = ((int)(t_fase * 40) % 2) ? 5 : 4;
                gfx_linha_grossa(ax, ay - R_DADO, ex, ey, tr + 2, gfx_mistura(C_LASER, C_FELTRO, 55));
                gfx_linha_grossa(ax, ay - R_DADO, ex, ey, tr, C_LASER);
                gfx_linha_grossa(ax, ay - R_DADO, ex, ey, 1, C_BRANCO);
                gfx_disco(ex, ey, tr, C_BRANCO);
            }
            float tt = (p - 0.3f) / 0.5f;           // anel do impacto no alvo
            if (tt > 0 && tt < 1) {
                int rr = (int)(R_DADO * (0.6f + tt * 1.2f));
                for (int a = 0; a < 32; a++) {
                    float an = a * 0.19635f;
                    gfx_rect(bx + (int)(cosf(an) * rr) - 1, by + (int)(sinf(an) * rr) - 1, 3, 3,
                             gfx_mistura(C_LASER, C_FELTRO, (int)(tt * 80)));
                }
            }
        } else {
            projetil(ax, ay, bx, by, p / 0.42f, c);
        }
    }
    // Espelho: um reflexo sai do maior dado do rival e chega nele.
    if (e->tipo == EV_ESPELHO) {
        const fila_t *fr = &F[e->alvo_j];
        int a = 0;
        for (int i = 0; i < fr->lancados; i++) if (fr->valor[i] == e->valor) { a = i; break; }
        projetil(SLOT_X(a), zona(e->alvo_j), ax, ay, p / 0.5f, C_ACO);
    }
    // Ficha de Fogo: linha de fogo tremulando ate o alvo, brasas subindo dele.
    if (e->tipo == EV_FOGO) {
        float q = p / 0.35f;
        if (q > 1) q = 1;
        int bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        int ex = ax + (int)((bx - ax) * q), ey = ay + (int)((by - ay) * q);
        uint16_t c = ((int)(t_fase * 16) % 2) ? C_FOGO : C_AMARELO;
        gfx_linha_grossa(ax, ay, ex, ey + ((int)(t_fase * 20) % 3) - 1, 5, c);
        if (p > 0.35f)
            for (int i = 0; i < 10; i++) {
                int px = bx - 18 + (i * 7) % 36;
                int py = by - 24 - ((int)(t_fase * 70) + i * 9) % 40;
                gfx_rect(px, py, 4, 4, i % 2 ? C_AMARELO : C_FOGO);
            }
    }
    // Jackpot: letreiro piscando sobre a maquininha e moedas caindo no placar.
    if (e->tipo == EV_JACKPOT) {
        for (int i = 0; i < 14; i++) {
            float q = fmodf(p * 1.6f + i * 0.137f, 1.0f);
            int mx = ax - 60 + (i * 53) % 120 + (int)(sinf(t_fase * 6 + i) * 4);
            int my = ay - 70 + (int)(q * 90);
            gfx_disco(mx, my, 4, i % 3 ? C_OURO : C_AMARELO);
            gfx_rect(mx - 1, my - 2, 2, 2, C_BRANCO);
        }
        // A placa fica a esquerda da fila, longe do texto do meio da mesa.
        int lx = 96, ly = ay - 24;
        uint16_t c = ((int)(t_fase * 8) % 2) ? C_AMARELO : C_VERMELHO;
        int w = gfx_largura("JACKPOT!", 2) + 16;
        gfx_rect(lx - w / 2, ly - 4, w, 48, C_BARRA);
        gfx_moldura(lx - w / 2, ly - 4, w, 48, 2, c);
        for (int i = 0; i < 8; i++)                        // lampadas da moldura
            gfx_disco(lx - w / 2 + 6 + i * (w - 12) / 7, ly - 4, 2,
                      ((i + (int)(t_fase * 10)) % 2) ? C_AMARELO : C_VERMELHO);
        txt_c2(lx, ly, "JACKPOT!", c);
        if (p > 0.5f) {
            int ty = zona(e->de_j) - 10 - (int)((p - 0.5f) * 30);
            txt_sombra_c(PLACAR_X - 64, ty, "+10", C_OURO, 1);
        }
    }
    // Semente: o broto cresce da moeda, e o +10 sobe em verde.
    if (e->tipo == EV_SEMENTE) {
        float q = p / 0.6f;
        if (q > 1) q = 1;
        int alto = (int)(q * 34);
        int sx = ax, sy = ay - R_DADO;
        gfx_rect(sx - 1, sy - alto, 4, alto, C_VERDE);
        if (q > 0.5f) {
            gfx_rect(sx - 12, sy - alto + 8, 11, 6, C_VERDE);
            gfx_rect(sx + 3, sy - alto + 3, 11, 6, C_VERDE);
        }
        if (p > 0.5f) {
            int ty = zona(e->de_j) - 10 - (int)((p - 0.5f) * 30);
            txt_sombra_c(PLACAR_X - 64, ty, "+10", C_VERDE, 1);
        }
    }
    // Bonus fixos do fim, cada um com o seu gesto, e o +N subindo no placar.
    if (e->tipo == EV_EXTRA) {
        int t = F[e->de_j].tipo[e->de_k];
        uint16_t c = cor_aro(t);
        if (t == D_MISERICORDIOSO) {
            // Uma luz sai de cada dado anulado e vem pousar nele.
            const fila_t *f = &F[e->de_j];
            for (int i = 0; i < f->lancados; i++) {
                if (vis_est[e->de_j][i] != V_ANULADO) continue;
                float q = p / 0.55f - i * 0.08f;
                if (q < 0 || q > 1) continue;
                int sx = SLOT_X(i), bx = ax;
                int lx = sx + (int)((bx - sx) * q), ly = ay - (int)(sinf(q * 3.1416f) * 34);
                gfx_disco(lx, ly, 4, C_HALO);
                gfx_disco(lx, ly, 2, C_BRANCO);
            }
        } else if (t == D_SOLITARIO) {
            // Um facho de luz de cima, so nele.
            float q = p < 0.3f ? p / 0.3f : (p > 0.8f ? (1 - p) / 0.2f : 1);
            int topo = ay - 70, base = ay + R_DADO;
            for (int yy = topo; yy < base; yy += 2) {
                int meia = 6 + (yy - topo) * 22 / (base - topo);
                gfx_rect_alfa(ax - meia, yy, 2 * meia, 2, C_HALO, (int)(40 * q));
            }
        } else if (t == D_DESAFIANTE) {
            // A flamula sobe do dado e se abre, tremulando: ele e o maior da mesa.
            float q = p / 0.4f;
            if (q > 1) q = 1;
            int topo = ay - R_DADO - 4 - (int)(q * 40);
            gfx_linha_grossa(ax, ay - R_DADO - 4, ax, topo, 2, C_LATAO);
            int w = (int)(q * 30), onda = (int)(sinf(t_fase * 12) * 3);
            if (w > 2) {
                float vx[3] = { (float)ax, (float)(ax + w), (float)ax };
                float vy[3] = { (float)topo, (float)(topo + 7 + onda), (float)(topo + 15) };
                poligono(vx, vy, 3, C_CARMIM);
            }
            gfx_disco(ax, topo, 2, C_OURO);
            if (q >= 1 && p < 0.85f) faiscas(ax + 12, topo + 10, t_fase, C_OURO);
        }
        // Anel que se abre do dado.
        float tt = p / 0.6f;
        if (tt < 1) {
            int rr = (int)(R_DADO * (0.8f + tt * 1.3f));
            for (int a = 0; a < 40; a++) {
                float an = a * 0.15708f;
                gfx_rect(ax + (int)(cosf(an) * rr) - 1, ay + (int)(sinf(an) * rr) - 1, 2, 2,
                         gfx_mistura(c, C_FELTRO, (int)(tt * 85)));
            }
        }
        if (p > 0.45f) {
            char mais[8];
            snprintf(mais, sizeof mais, "+%d", e->valor);
            int ty = zona(e->de_j) - 10 - (int)((p - 0.45f) * 30);
            txt_sombra_c(PLACAR_X - 64, ty, mais, C_OURO, 1);
        }
    }
    // Ferreiro: a marreta desce no dado seguinte e solta faiscas.
    if (e->tipo == EV_BONUS && e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_FERREIRO) {
        int bx = SLOT_X(e->de_k + 1), by = zona(e->de_j) - R_DADO - 6;
        float q = p / 0.3f;
        if (q <= 1) {
            float a = -1.1f + q * 1.1f;                    // o cabo girando ate bater
            int hx = bx + (int)(cosf(a - 1.5708f) * 24), hy = by + (int)(sinf(a - 1.5708f) * 24) + 24;
            gfx_linha_grossa(bx + 18, by + 16, hx, hy, 3, C_MADEIRA_CLR);
            gfx_rect(hx - 6, hy - 4, 12, 8, C_FERRO);
        } else if (p < 0.9f) {
            for (int i = 0; i < 10; i++) {
                float an = i * 0.628f + p * 3;
                int rr = (int)((p - 0.3f) * 70);
                gfx_rect(bx + (int)(cosf(an) * rr), by + 8 + (int)(sinf(an) * rr * 0.6f), 3, 2,
                         i % 2 ? C_FOGO : C_AMARELO);
            }
        }
    }
    if ((e->tipo == EV_TIRA || e->tipo == EV_PIRATA) && p > 0.2f) {
        float q = (p - 0.2f) / 0.6f;
        if (q > 1) q = 1;
        int sy = zona(e->alvo_j) - 10 - (int)(q * 18);
        char menos[8];
        snprintf(menos, sizeof menos, "-%d", e->valor);
        txt_sombra_c(PLACAR_X - 64, sy, menos, C_VINHO_CLR, 1);
        if (e->tipo == EV_PIRATA) {
            int y0 = zona(e->alvo_j), y1 = zona(e->de_j);
            int fy = y0 + (int)((y1 - y0) * q);
            ficha(PLACAR_X + 46, fy, C_OURO);
            if (q >= 1) txt_sombra_c(PLACAR_X - 64, zona(e->de_j) - 28, "+4", C_OURO, 1);
        }
    }
}

static void pote_mesa(void)
{
    int n = pote / 10 + 1;
    if (n > 9) n = 9;
    int y = CENTRO + 10;
    for (int i = 0; i < n; i++)
        ficha(POTE_X, y - i * 4, (i % 3 == 0) ? C_LATAO : (i % 3 == 1 ? C_VINHO : C_MARFIM_S));
    char s[16];
    snprintf(s, sizeof s, "%d", pote);
    txt_c(POTE_X, y + 12, s, C_OURO, true);
    txt_c(POTE_X, y - n * 4 - 26, "pote", C_TEXTO_M, false);
}

// Nome do dado em destaque e o poder logo abaixo.
// medio: o texto de uma linha da loja; senao, o curto da fila e do jogo.
static int larg_dado_e_poder(int tipo, bool medio)
{
    char c[32];
    classe(tipo, c, sizeof c);
    int a = gfx_largura(TIPO[tipo].nome, 1) + 12 + gfx_largura(c, 1) + 10;
    int b = gfx_largura(medio ? TEXTO_MEDIO[tipo] : TIPO[tipo].desc, 1);
    return a > b ? a : b;
}

// Uma linha: "VOCÊ joga  ·  Moeda [comum]", e embaixo o que o dado faz.
static int larg_quem_e_dado(const char *quem, int tipo)
{
    char c[32];
    classe(tipo, c, sizeof c);
    int a = gfx_largura(quem, 1) + 20 + gfx_largura(TIPO[tipo].nome, 1) + 12 + gfx_largura(c, 1) + 10;
    int b = gfx_largura(TIPO[tipo].desc, 1);
    return a > b ? a : b;
}

static void quem_e_dado(int y, const char *quem, uint16_t cor, int tipo)
{
    char c[32];
    classe(tipo, c, sizeof c);
    int wq = gfx_largura(quem, 1), wn = gfx_largura(TIPO[tipo].nome, 1), wc = gfx_largura(c, 1) + 10;
    int x = GFX_W / 2 - (wq + 20 + wn + 12 + wc) / 2;
    gfx_texto(x, y, quem, cor, 1, true);
    txt_c(x + wq + 10, y, "·", C_TEXTO_M, false);
    x += wq + 20;
    gfx_texto(x, y, TIPO[tipo].nome, cor_aro(tipo), 1, true);
    int xc = x + wn + 12;
    ui_etiqueta(xc, y + 2, wc, TXT_H - 2, cor_rar(tipo));
    gfx_texto(xc + 5, y, c, cor_rar(tipo), 1, false);
    txt_c(GFX_W / 2, y + TXT_H - 2, TIPO[tipo].desc, C_MARFIM_S, false);
}

static void dado_e_poder(int y, int tipo, bool medio)
{
    // Nome e, ao lado, a etiqueta "D8 · raro" na cor da raridade.
    char c[32];
    classe(tipo, c, sizeof c);
    int wn = gfx_largura(TIPO[tipo].nome, 1), wc = gfx_largura(c, 1) + 10;
    int x = GFX_W / 2 - (wn + 12 + wc) / 2;
    gfx_texto(x, y, TIPO[tipo].nome, cor_aro(tipo), 1, true);
    int xc = x + wn + 12;
    ui_etiqueta(xc, y + 2, wc, TXT_H - 2, cor_rar(tipo));
    gfx_texto(xc + 5, y, c, cor_rar(tipo), 1, false);
    txt_c(GFX_W / 2, y + TXT_H - 1, medio ? TEXTO_MEDIO[tipo] : TIPO[tipo].desc, C_MARFIM_S, false);
}

// Tracinho discreto na cor da raridade, embaixo de um dado.
static void marca_rar(int x, int y, int tipo)
{
    gfx_rect(x - 7, y, 14, 2, cor_rar(tipo));
}

// Uma fileira de botoes. Com tinta, cada um tem a sua cor (a aposta em
// vermelho, correr em preto...), para reconhecer sem ler; o escolhido acende
// e ganha a moldura branca. Sem tinta, o escolhido fica na cor do jogador.
#define BOTAO_H 24
static int larg_botoes(const char **rot, int n)
{
    int larg = -12;
    for (int i = 0; i < n; i++) larg += gfx_largura(rot[i], 1) + 24 + 12;
    return larg;
}

static void botoes(int y, const char **rot, const bool *ativo, int n, uint16_t cor, const uint16_t *tinta)
{
    (void)cor;
    int x = GFX_W / 2 - larg_botoes(rot, n) / 2;
    for (int i = 0; i < n; i++) {
        int w = gfx_largura(rot[i], 1) + 24;
        bool cur = i == cursor && ativo[i];
        ui_botao(x, y, w, BOTAO_H, rot[i], tinta ? tinta[i] : 0, !ativo[i] ? UI_DESLIGADO : cur ? UI_FOCO : 0);
        if (ativo[i]) zona_clique(x, y, w, BOTAO_H, &cursor, i, KEY_ENTER);
        x += w + 12;
    }
}

// Montagem da fila: a mao comprada aparece na linha do jogador, um dado de
// cada vez saindo da bolsa. O numero dourado e a posicao na fila.
static void desenha_ordem(void)
{
    jogador_t *p = &J[vez];
    fila_t *f = &F[vez];
    int y = zona(vez), n = p->n_mao;
    for (int i = 0; i < n; i++) {
        float chega = i * 0.07f;
        if (t_fase < chega) continue;
        float q = (t_fase - chega) / 0.18f;
        if (q > 1) q = 1;
        int dy = (int)((1 - q) * (vez == baixo ? 40 : -40));   // vem do lado da bolsa
        int x = GFX_W / 2 + (int)((i - (n - 1) / 2.0f) * 54);
        int ordem = -1;
        for (int k = 0; k < f->n; k++) if (f->idx[k] == p->mao[i]) ordem = k;
        bool cur = i == cursor && q >= 1;
        if (cur) ui_selecao(x - 25, y - 25, 50, 50, p->cor);
        peca_parada((float)x, (float)(y + dy - (cur ? 3 : 0)), 17, p->col[p->mao[i]], vez);
        if (!cur && q >= 1) marca_rar(x, y + 24, p->col[p->mao[i]].tipo);
        if (ordem >= 0) {
            char o[2] = { (char)('1' + ordem), 0 };
            gfx_disco(x + 16, y - 17, 9, C_OURO);
            gfx_disco(x + 16, y - 17, 7, C_AMARELO);
            txt_c(x + 16, y - 27, o, C_BARRA, true);
        }
        if (cur) gfx_rect(x - 16, y + 23, 32, 3, p->cor);
        zona_clique(x - 25, y - 25, 50, 50, &cursor, i, KEY_ENTER);
        dica_zona(x - 25, y - 25, 50, 50, DICA_DADO, p->col[p->mao[i]].tipo);
    }

    char t[64];
    if (vez == primeiro) snprintf(t, sizeof t, "%s monta a sequência", p->nome);
    else snprintf(t, sizeof t, "%s monta a sequência  ·  alvo %d", p->nome, F[outro(vez)].pontos);
    int y1 = CENTRO - 44;
    int cabe = max_fila(vez);
    const char *cheia = "sequência cheia: TAB ou CTRL confirma";
    int w = gfx_largura(t, 1);
    if (f->n >= cabe) { if (gfx_largura(cheia, 1) > w) w = gfx_largura(cheia, 1); }
    else if (n && larg_dado_e_poder(p->col[p->mao[cursor]].tipo, false) > w)
        w = larg_dado_e_poder(p->col[p->mao[cursor]].tipo, false);
    painel(y1 - 5, 3 * TXT_H + 34, larg_painel(w, 300));
    txt_c(GFX_W / 2, y1, t, p->cor, true);
    if (f->n >= cabe) {
        txt_c(GFX_W / 2, y1 + 24, cheia, C_OURO, true);
    } else if (n) {
        dado_e_poder(y1 + 20, p->col[p->mao[cursor]].tipo, false);
    }
    // Rodape: so a fila como vai ser jogada, 1 » 2 » 3 » 4, no centro.
    int ry = y1 + 3 * TXT_H + 10, fx = GFX_W / 2 - (cabe - 1) * 36 / 2;
    for (int k = 0; k < cabe; k++) {
        int x = fx + k * 36;
        if (k) txt_c(x - 18, ry - 11, "»", C_FELTRO2, false);
        if (k < f->n) {
            peca_t pc = { f->tipo[k], f->fixo[k] };
            peca_parada((float)x, (float)ry, 10, pc, vez);
            // Clicar num dado da sequencia tira ele dela.
            for (int i = 0; i < n; i++)
                if (p->mao[i] == f->idx[k]) zona_clique(x - 14, ry - 14, 28, 28, &cursor, i, KEY_ENTER);
        } else {
            gfx_disco(x, ry, 11, k == f->n ? C_OURO : C_FELTRO2);
            gfx_disco(x, ry, 9, C_FELTRO_ESC);
            char o[2] = { (char)('1' + k), 0 };
            txt_c(x, ry - 10, o, k == f->n ? C_OURO : C_FELTRO2, false);
        }
    }
    if (f->n) botao_clique(GFX_W / 2 + 225, ry - 12, "pronto", KEY_TAB);
}

// Chance (%) de o proximo dado de j derrubar alguma coisa: ele mesmo ou o
// que valia antes. Joga o lance muitas vezes numa copia da mesa, com as
// regras de verdade (Gemeo, Bumerangue, Teimoso, Sentinela, Vidro...); o
// sorteio e o da IA, guardado e devolvido, para nada do jogo mudar.
static int chance_cair(int j)
{
    static fila_t c_fila;                       // a mesa da ultima conta: so refaz se mudou
    static int c_vez = -1, c_valor = 0;
    fila_t *f = &F[j];
    int k = f->lancados;
    if (k >= f->n) return 0;
    if (c_vez == j && !memcmp(&c_fila, f, sizeof c_fila)) return c_valor;
    memcpy(&c_fila, f, sizeof c_fila);
    uint8_t antes[N_FILA];
    memcpy(antes, f->est, sizeof antes);
    bool ego = f->tipo[k] == D_EGOISTA;          // o Egoista derruba os outros de proposito
    mesa_t m;
    guarda_mesa(&m);
    uint32_t sem = semente_ia;
    const int N = 2000;
    int caiu = 0;
    for (int a = 0; a < N; a++) {
        semeia_ia(0xC4A1u + (uint32_t)rodada * 31u + (uint32_t)k, a);
        if (f->tipo[k] == D_ACUMULADOR && f->fixo[k] < 6) f->fixo[k]++;   // o lance soma antes
        sim_lanca(j);
        bool c = f->est[k] != V_VALIDO || (f->tipo[k] == D_TUDO_NADA && f->cru[k] == 1);
        for (int i = 0; i < k && !c && !ego; i++) c = antes[i] == V_VALIDO && f->est[i] != V_VALIDO;
        caiu += c;
        volta_mesa(&m);
    }
    semente_ia = sem;
    c_vez = j;
    c_valor = (caiu * 100 + N / 2) / N;
    return c_valor;
}


// A dica so para quem esta aprendendo: some no Dificil da partida rapida e do
// Nivel 3 do Desafiante em diante.
static bool mostra_chance(void)
{
    if (cpu >> vez & 1) return false;            // vez do computador
    if (modo == M_DESAFIO) return des_dificuldade() < 3;
    if (cpu >> outro(vez) & 1) return nivel_jog[outro(vez)] < 2;
    return true;
}

static void desenha_joga(void)
{
    jogador_t *p = &J[vez];
    fila_t *f = &F[vez];
    char t[64];
    snprintf(t, sizeof t, "%s joga", p->nome);
    int y1 = CENTRO - 40;

    // Os botoes primeiro: a largura do painel depende deles.
    char parar[40] = "parar", jogar[40] = "jogar";
    const char *rot[2] = { jogar, parar };
    if (fase == F_JOGA) {
        // Para quem joga em segundo, o botao ja diz o que parar significa.
        if (vez != primeiro && f->lancados) {
            desfecho_t d;
            calcula_desfecho(&d, false);
            int dif = d.total[vez] - d.total[outro(vez)];
            if (dif > 0)      snprintf(parar, sizeof parar, "parar: vence por %d", dif);
            else if (dif < 0) snprintf(parar, sizeof parar, "parar: perde por %d", -dif);
            else              snprintf(parar, sizeof parar, "parar: empata");
        }
        if (f->lancados == 0) snprintf(parar, sizeof parar, "voltar à sequência");
        if (f->lancados && f->lancados < f->n && mostra_chance()) {
            int c = chance_cair(vez);
            if (c <= 0) snprintf(jogar, sizeof jogar, "jogar: não cai");
            else        snprintf(jogar, sizeof jogar, "jogar: %d%% de cair", c);
        }
    }
    bool prox = fase == F_JOGA && f->lancados < f->n;     // o proximo dado da fila, junto do nome
    int w = prox ? larg_quem_e_dado(t, tipo_de(vez, f->lancados)) : gfx_largura(t, 1);
    if (fase == F_VEREDITO && gfx_largura(veredito, 1) > w) w = gfx_largura(veredito, 1);
    if (fase == F_JOGA && larg_botoes(rot, 2) > w) w = larg_botoes(rot, 2);
    int h = fase == F_JOGA ? (prox ? 2 * TXT_H : TXT_H) + BOTAO_H + 12 : 2 * TXT_H + 4;
    if (fase != F_ROLANDO && fase != F_ARRUMA) painel(y1 - 5, h, larg_painel(w, 220));

    if (prox) quem_e_dado(y1, t, p->cor, tipo_de(vez, f->lancados));
    else txt_c(GFX_W / 2, y1, t, p->cor, true);
    if (fase == F_VEREDITO) {
        uint16_t c = ver_tipo == 0 ? C_OURO : ver_tipo == 2 ? C_ACO
                   : ver_tipo == 4 ? gfx_mistura(C_EGO, C_BRANCO, 25) : C_VINHO_CLR;
        txt_sombra_c(GFX_W / 2, y1 + TXT_H - 2, veredito, c, 1);
    }

    if (fase == F_JOGA) {
        bool ativo[2] = { f->lancados < f->n, true };
        static const uint16_t TINTA_JOGA[2] = { RGB(38, 96, 64), RGB(40, 62, 104) };   // jogar, parar
        botoes(y1 + (prox ? 2 * TXT_H : TXT_H) + 1, rot, ativo, 2, p->cor, TINTA_JOGA);
    }
    pote_mesa();
}

static void desenha_aposta(void)
{
    char t[64], q[64];
    int y1 = CENTRO - 36;
    // Uma linha so: quem fez quantos pontos e a pergunta, cada um na sua cor.
    snprintf(t, sizeof t, "%s fez %d pontos", J[primeiro].nome, F[primeiro].pontos);
    if (a_pagar)            snprintf(q, sizeof q, "%s paga %d para ver?", vez == primeiro ? "" : J[vez].nome, a_pagar);
    else if (vez == primeiro) snprintf(q, sizeof q, "aposta?");
    else                    snprintf(q, sizeof q, "%s aposta?", J[vez].nome);
    const char *qq = q[0] == ' ' ? q + 1 : q;
    int wt = gfx_largura(t, 1), wq = gfx_largura(qq, 1), sep = 20;

    int op[4], n = opcoes(op);
    char r[4][24];
    const char *rot[4];
    bool ativo[4];
    uint16_t tinta[4];
    for (int i = 0; i < n; i++) {
        rotulo_op(op[i], r[i], sizeof r[i]); rot[i] = r[i]; ativo[i] = true;
        tinta[i] = tinta_op(op[i]);
    }
    int w = wt + sep + wq;
    if (larg_botoes(rot, n) > w) w = larg_botoes(rot, n);
    painel(y1 - 5, TXT_H + BOTAO_H + 20, larg_painel(w, 220));
    int x = GFX_W / 2 - (wt + sep + wq) / 2;
    gfx_texto(x, y1, t, J[primeiro].cor, 1, true);
    txt_c(x + wt + sep / 2, y1, "·", C_TEXTO_M, false);
    gfx_texto(x + wt + sep, y1, qq, J[vez].cor, 1, false);
    botoes(y1 + TXT_H + 6, rot, ativo, n, J[vez].cor, tinta);
    pote_mesa();
}

static void desenha_efeitos(void)
{
    if (ev_i >= n_ev) return;
    const evento_t *e = &ev[ev_i];
    uint16_t c = e->tipo == EV_ZERA ? C_VIOLETA
               : e->tipo == EV_FOGO ? C_FOGO
               : e->tipo == EV_SEMENTE ? C_VERDE
               : e->tipo == EV_JACKPOT ? C_AMARELO
               : (e->tipo == EV_ROUBA || e->tipo == EV_BONUS || e->tipo == EV_MULT || e->tipo == EV_EXTRA) ? C_OURO
               : C_VINHO_CLR;
    // Fundo atras do texto: o dado roubado passa por baixo, sem embaralhar.
    painel(CENTRO - 26, 2 * TXT_H + 6, larg_painel(gfx_largura(e->txt, 1), 200));
    txt_c(GFX_W / 2, CENTRO - 22, J[e->de_j].nome, J[e->de_j].cor, true);
    txt_c(GFX_W / 2, CENTRO - 3, e->txt, c, true);
    desenha_evento();
}

static void desenha_resultado(void)
{
    uint16_t c = venc_mao == 0 ? C_LATAO : (venc_mao == 1 ? C_ROSA : C_MARFIM);
    char pl[64] = "";
    if (venc_mao >= 0)                                     // o placar da rodada
        snprintf(pl, sizeof pl, "%s   %d x %d   %s", J[0].nome, F[0].total, F[1].total, J[1].nome);
    int wp = gfx_largura(aviso, 1);
    if (gfx_largura(pl, 1) > wp) wp = gfx_largura(pl, 1);
    if (nota[0] && gfx_largura(nota, 1) > wp) wp = gfx_largura(nota, 1);
    int linhas = 1 + (pl[0] != 0) + (nota[0] != 0), y = CENTRO - 30;
    painel(y - 5, (linhas + 1) * (TXT_H - 2) + 8, larg_painel(wp, 220));
    txt_sombra_c(GFX_W / 2, y, aviso, c, 1);
    y += TXT_H - 2;
    if (pl[0]) { txt_c(GFX_W / 2, y, pl, C_MARFIM_S, false); y += TXT_H - 2; }
    if (nota[0]) { txt_c(GFX_W / 2, y, nota, C_TERRA, false); y += TXT_H - 2; }   // cinza ou cacos
    txt_c(GFX_W / 2, y, "ENTER segue", C_TEXTO_M, false);
    // As fichas do pote voando, uma atras da outra, ate a faixa de quem levou;
    // o placar dela sobe enquanto chegam.
    int w = res_quem;
    if (w < 0) return;
    int ganho = J[w].fichas - res_antes[w];
    int n = ganho / 8 + 3;
    if (n > 10) n = 10;
    float x0 = POTE_X, y0 = CENTRO + 10;
    for (int i = 0; i < n; i++) {
        float q = (t_fase - 0.05f - i * 0.07f) / 0.55f;
        if (q <= 0 || q >= 1) continue;
        float e = q * q * (3 - 2 * q);                        // sai devagar, acelera, pousa
        float fx = x0 + (ficha_px[w] - x0) * e, fy = y0 + (ficha_py[w] - y0) * e - sinf(q * 3.1416f) * 46;
        ficha((int)fx, (int)fy, i % 3 == 0 ? C_LATAO : (i % 3 == 1 ? C_VINHO : C_MARFIM_S));
    }
    float fim = 0.05f + n * 0.07f + 0.55f, q = (t_fase - fim) / 0.9f;
    if (ganho > 0 && q > -0.15f && q < 1) {                  // o "+N" subindo da faixa
        char mais[16];
        snprintf(mais, sizeof mais, "+%d", ganho);
        int sobe = (int)((q < 0 ? 0 : q) * 10);           // no feltro, junto da faixa
        bool cima = ficha_py[w] < CENTRO;
        int ty = cima ? MESA_Y0 + 6 + sobe : MESA_Y1 - TXT_H - 8 - sobe;
        txt_sombra_c(ficha_px[w] + 16, ty, mais, C_OURO, 1);
    }
}

// As fichas da faixa: no resultado, sobem (ou descem) enquanto o pote voa.
static int fichas_vis(int j)
{
    if (fase != F_RESULTADO || res_quem < 0) return J[j].fichas;
    int n = (J[res_quem].fichas - res_antes[res_quem]) / 8 + 3;
    if (n > 10) n = 10;
    float fim = 0.05f + n * 0.07f + 0.55f, q = (t_fase - 0.3f) / (fim - 0.3f);
    if (q >= 1) return J[j].fichas;
    if (q < 0) q = 0;
    return res_antes[j] + (int)((J[j].fichas - res_antes[j]) * q);
}

#define GRADE 12                     // dados por linha na escolha do que sai

static void desenha_premio(void)
{
    jogador_t *p = &J[vez];
    char t[64];
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);

    if (removendo) {
        // Escolha de qual dado sai da colecao: a colecao inteira em duas linhas.
        snprintf(t, sizeof t, "%s: qual dos %d dados sai?", p->nome, p->n_col);
        txt_c(GFX_W / 2, MESA_Y0 + 14, t, p->cor, true);
        txt_c(GFX_W / 2, MESA_Y0 + 38, "ENTER tira para sempre  ·  BACKSPACE volta", C_TEXTO_M, false);
        for (int i = 0; i < p->n_col; i++) {
            int lin = i / GRADE, col = i % GRADE;
            int na_linha = (p->n_col - lin * GRADE) < GRADE ? p->n_col - lin * GRADE : GRADE;
            int x = GFX_W / 2 + (int)((col - (na_linha - 1) / 2.0f) * 46);
            int y = 134 + lin * 52;
            if (t_tira > 0 && i == tira_i) {
                // O escolhido encolhe e cai, deixando um rastro de po.
                float q = t_tira / 0.7f;
                float r = 17 * (1 - q);
                if (r > 2) peca_parada((float)x, y + q * 24, r, p->col[i], vez);
                for (int k = 0; k < 7; k++)
                    gfx_rect(x - 14 + k * 4, y + (int)(q * 34) + (k % 2) * 6, 3, 3, C_TEXTO_M);
                continue;
            }
            if (i == cursor) ui_selecao(x - 22, y - 22, 44, 44, C_VINHO_CLR);
            peca_parada((float)x, (float)y, 17, p->col[i], vez);
            if (i != cursor) marca_rar(x, y + 23, p->col[i].tipo);
            if (i == cursor) gfx_rect(x - 16, y + 23, 32, 3, C_VINHO_CLR);
            zona_clique(x - 22, y - 22, 44, 44, &cursor, i, KEY_ENTER);
        }
        botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar", KEY_BKSP);
        int tc = p->col[cursor < p->n_col ? cursor : 0].tipo;
        txt_c(GFX_W / 2, 236, TIPO[tc].nome, cor_aro(tc), true);
        txt_c(GFX_W / 2, 258, t_tira > 0 ? "some para sempre" : TEXTO_MEDIO[tc],
              t_tira > 0 ? C_VINHO_CLR : C_MARFIM_S, false);
        return;
    }

    if (modo == M_DESAFIO) snprintf(t, sizeof t, "Prêmio por vencer %s", J[1].nome);
    else snprintf(t, sizeof t, "%s levou a rodada e escolhe um prêmio", p->nome);
    txt_c(GFX_W / 2, MESA_Y0 + 14, t, p->cor, true);
    bool girando = slot_girando();
    for (int i = 0; i < 3; i++) {
        int x = GFX_W / 2 + (i - 1) * 150;
        int t2 = oferta[i].tipo;
        float para = slot_para(i);
        bool parado = t_slot < 0 || t_slot >= para;
        bool cur = i == cursor && !girando;
        uint16_t cr = cor_rar(t2);
        ui_cartao(x - 64, 90, 128, 110, cur, cr);
        if (!parado) {
            // O rolo: dados passando de cima para baixo, cada vez mais devagar,
            // ate o ultimo (o premio) chegar ao centro. Os de passagem encolhem
            // perto das bordas, como num cilindro.
            // O rolo e desenhado num rascunho e so a janela volta para a tela:
            // os dados de passagem nao vazam para fora (nem os numeros deles).
            const int wx = x - 60, wy = 94, ww = 120, wh = 64;
            uint16_t *tela = display_fb();
            gfx_desvia(rascunho);
            gfx_rect(wx, wy, ww, wh, C_FELTRO_ESC);
            float r = para - t_slot, o = 9.4f * r * sqrtf(r);
            int k0 = (int)o;
            for (int k = k0 - 1; k <= k0 + 1; k++) {
                if (k < 0) continue;
                float dy = (o - k) * 34;
                float q = 1 - fabsf(dy) / 40;
                if (q <= 0.35f) continue;
                peca_t pc = oferta[i];
                if (k > 0) {
                    uint32_t h = (uint32_t)(k * 2654435761u) ^ (uint32_t)(i * 40503u + 17);
                    // So os tipos que podem sair aqui: os basicos, ou os ja
                    // abertos no Desafiante.
                    pc.tipo = (uint8_t)(h % D_BASICOS);
                    if (modo == M_DESAFIO && dado_liberado((int)(h % D_N))) pc.tipo = (uint8_t)(h % D_N);
                    pc.fixo = (uint8_t)(1 + (h >> 8) % 8);
                }
                peca_parada((float)x, 126 + dy, 24 * q, pc, vez);
            }
            for (int b2 = 0; b2 < 8; b2++) {             // sombra nas bordas: um cilindro
                gfx_rect_alfa(wx, wy + b2, ww, 1, C_BARRA, 150 - b2 * 18);
                gfx_rect_alfa(wx, wy + wh - 1 - b2, ww, 1, C_BARRA, 150 - b2 * 18);
            }
            gfx_desvia(NULL);
            for (int yy = wy; yy < wy + wh; yy++)
                memcpy(tela + yy * GFX_W + wx, rascunho + yy * GFX_W + wx, (size_t)ww * 2);
            txt_c(x, 162, "?", C_TEXTO_M, true);
            continue;
        }
        // Acabou de travar: um pulo e a moldura acende.
        float q = t_slot >= 0 ? (t_slot - para) / 0.3f : 1;
        bool acende = q < 1;
        if (acende) ui_cartao(x - 64, 90, 128, 110, true, gfx_mistura(cr, C_BRANCO, (int)(60 * (1 - q))));
        float pulo = acende ? sinf(q * 3.14159f) * -8 : 0;
        float bob = cur ? sinf(t_fase * 4) * 3 : 0;
        peca_parada((float)x, 130 + bob + pulo, 26 * (acende ? 1 + 0.12f * (1 - q) : 1), oferta[i], vez);
        txt_c(x, 162, TIPO[t2].nome, cur || acende ? cor_aro(t2) : C_MARFIM_S, cur);
        char c2[32];
        classe(t2, c2, sizeof c2);
        txt_c(x, 180, c2, cr, false);
        zona_clique(x - 64, 90, 128, 110, &cursor, i, KEY_ENTER);
    }
    bool pode = p->n_col > N_FILA;
    if (girando) {
        txt_c(GFX_W / 2, 212, "sorteando...", C_OURO, true);
    } else if (cursor < 3) {
        txt_c(GFX_W / 2, 212, TEXTO_MEDIO[oferta[cursor].tipo], C_MARFIM, false);
    } else {
        txt_c(GFX_W / 2, 212, pode ? "troca o prêmio por tirar um dos seus, para sempre"
                                   : "com 4 dados, nenhum pode sair", C_MARFIM, false);
    }

    // Quarta opcao: trocar o premio por afinar a bolsa.
    const char *rot = "remover um dado";
    int w = gfx_largura(rot, 1) + 28;
    bool cur = cursor == 3;
    ui_botao(GFX_W / 2 - w / 2, 240, w, 26, rot, C_VINHO, !pode ? UI_DESLIGADO : cur ? UI_FOCO : 0);
    zona_clique(GFX_W / 2 - w / 2, 240, w, 26, &cursor, 3, KEY_ENTER);
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "recusar", 'x');
    txt_c(GFX_W / 2, 278, "setas escolhem  ·  ENTER leva  ·  X recusa", C_TEXTO_M, false);
}

// Uma quantia de fichas com o icone da ficha na frente, centrada em cx.
static void fichas_c(int cx, int y, int valor, const char *sufixo, uint16_t c)
{
    char t[80];
    snprintf(t, sizeof t, "%d%s", valor, sufixo);
    int w = gfx_largura(t, 1) + 20, x = cx - w / 2;
    ficha(x + 7, y + 11, C_LATAO);
    txt(x + 20, y, t, c, true);
}

static void desenha_loja(void)
{
    jogador_t *p = &J[vez];
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    txt_c2(GFX_W / 2, MESA_Y0 + 2, "LOJA DA CASA", C_LATAO);
    txt_c(GFX_W / 2, MESA_Y0 + 44, "Compre dados especiais com as suas fichas: eles vão para a sua bolsa.",
          C_MARFIM_S, false);
    char t[64];
    snprintf(t, sizeof t, " fichas  ·  %s compra", p->nome);
    fichas_c(GFX_W / 2, MESA_Y0 + 66, p->fichas, t, p->cor);

    for (int i = 0; i < N_LOJA; i++) {
        int x = GFX_W / 2 + (i - 2) * 108, yb = 142;
        int tp = loja[i].tipo, pr = preco_loja(tp);
        bool ja = levou[vez][i], cur = i == cursor, da = p->fichas >= pr;
        uint16_t cr = cor_rar(tp);
        ui_cartao(x - 48, yb, 96, 92, cur, cr);
        float bob = cur && !ja ? sinf(t_fase * 4) * 3 : 0;
        peca_parada((float)x, yb + 32 + bob, 22, loja[i], vez);
        if (ja) {
            txt_c(x, yb + 64, "comprado", C_TEXTO_M, false);
            gfx_rect_alfa(x - 47, yb + 1, 94, 90, C_SOMBRA, 120);
        } else {
            fichas_c(x, yb + 64, pr, "", da ? C_OURO : C_VINHO_CLR);
        }
        zona_clique(x - 48, yb, 96, 92, &cursor, i, KEY_ENTER);
    }
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "terminar", KEY_TAB);
    dado_e_poder(240, loja[cursor].tipo, true);
    if (!levou[vez][cursor] && p->fichas < preco_loja(loja[cursor].tipo))
        txt_c(GFX_W / 2, 286, "fichas insuficientes", C_VINHO_CLR, true);
    else
        txt_c(GFX_W / 2, 286, "setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB/CTRL termina", C_TEXTO_M, false);
}

static void desenha_fim(void)
{
    painel(CENTRO - 70, 150, 440);
    if (venc_partida == 2) {
        txt_sombra_c(GFX_W / 2, CENTRO - 58, "EMPATE", C_MARFIM, 3);
    } else {
        jogador_t *p = &J[venc_partida];
        txt_sombra_c(GFX_W / 2, CENTRO - 58, p->nome, p->cor, 3);
        txt_c(GFX_W / 2, CENTRO + 6, "leva a mesa", C_MARFIM, true);
    }
    char s[48];
    snprintf(s, sizeof s, "%s %d  x  %d %s", J[baixo].nome, J[baixo].fichas,
             J[outro(baixo)].fichas, J[outro(baixo)].nome);
    txt_c(GFX_W / 2, CENTRO + 32, s, C_TEXTO_M, false);
    txt_c(GFX_W / 2, CENTRO + 56, "ENTER nova partida", C_MARFIM_S, false);
}


// Moeda girando no catalogo: a mesma arte da mesa, virando devagar.
static void moeda_girando(int cx, int cy, int tipo, float t)
{
    float c = cosf(t * 2.2f);
    moeda_girando_arte((float)cx, (float)cy, 44, tipo, 0, c, c >= 0 ? 1 : 2);
}

// Faces que o dado pode mostrar, na ordem em que o catalogo as percorre.
static int faces_de(int tipo, int *f)
{
    int n = 0, lados = TIPO[tipo].lados;
    int de = TIPO[tipo].efeito == EF_LASTRO ? 4 : 1;
    for (int v = de; v <= lados; v++) f[n++] = v;
    return n;
}

// Quebra o texto em linhas de ate max_w pixels, sempre nos espacos.
static bool quebra_so_conta;                     // so mede: quantas linhas daria
static int quebra_linhas(int x, int y, int max_w, int max_lin, const char *txt_, uint16_t c)
{
    char linha[200];
    int lin = 0;
    const char *p = txt_;
    while (*p && lin < max_lin) {
        int n = 0, cabe = 0;
        // Vai somando palavras enquanto a linha couber.
        while (p[n]) {
            int fim = n;
            while (p[fim] == ' ') fim++;
            while (p[fim] && p[fim] != ' ') fim++;
            if (fim >= (int)sizeof linha) break;
            memcpy(linha, p, (size_t)fim);
            linha[fim] = 0;
            if (gfx_largura(linha, 1) > max_w && cabe) break;
            cabe = n = fim;
            if (!p[n]) break;
        }
        if (!cabe) cabe = (int)strlen(p) < (int)sizeof linha - 1 ? (int)strlen(p) : (int)sizeof linha - 1;
        memcpy(linha, p, (size_t)cabe);
        linha[cabe] = 0;
        if (!quebra_so_conta) txt(x, y + lin * (TXT_H + 4), linha, c, false);
        lin++;
        p += cabe;
        while (*p == ' ') p++;
    }
    return lin;
}

// Etiquetas de protecao, ao lado do tipo e da raridade: "não anulável"
// (Escudo, Teimoso, Egoista) e "intocável" (so o Escudo: nenhum ataque o
// alcanca). Devolve onde a proxima coisa pode comecar.
static int etiqueta_caixa(int x, int y, const char *s, uint16_t c)
{
    int w = gfx_largura(s, 1) + 12;
    ui_etiqueta(x, y, w, TXT_H + 2, c);
    txt(x + 6, y + 1, s, c, false);
    return x + w + 6;
}
static int etiquetas_protecao(int x, int y, int tipo)
{
    if (nao_anulavel_t(tipo)) x = etiqueta_caixa(x, y, "não anulável", C_ACO);
    if (intocavel_t(tipo))    x = etiqueta_caixa(x, y, "intocável", C_OURO);
    return x;
}

// Colecao em duas secoes, Dados e Amuletos, trocadas pela chave no alto.
// O que ainda esta fechado no Desafiante (para o perfil em uso) fica no
// escuro, com cadeado e o que falta para abrir.
static int  cat_trava_dado(int t, char *s, int n);       // desafio.inc: 0 sempre livre, 1 aberto, 2 fechado
static void desenha_cat_amuleto(int i);
static void cadeado(int x, int y, uint16_t c, int e);

// As faixas da colecao: a chave Dados | Amuletos, a contagem e as teclas.
static void faixas_colecao(int atual, int total)
{
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(botao_voltar(KEY_ESC), ty, "COLEÇÃO", C_LATAO, true);
    static const char *const SEC[2] = { "Dados", "Amuletos" };
    const int w = 104, h = 26, x0 = GFX_W / 2 - w, y0 = (FAIXA_H - h) / 2 - 1;
    for (int i = 0; i < 2; i++) {                 // duas abas, a escolhida em latao
        ui_botao(x0 + i * w, y0, w, h, SEC[i], cat_sec == i ? C_LATAO : 0, 0);
        zona_clique(x0 + i * w, y0, w, h, NULL, 0, i == 0 ? KEY_UP : KEY_DOWN);
    }
    char s[32];
    snprintf(s, sizeof s, "%d de %d", atual + 1, total);
    txt(GFX_W - 18 - gfx_largura(s, 1), ty, s, C_TEXTO_M, false);
    int yb = GFX_H - FAIXA_H;
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
    txt(18, yb + ty + 2, "<  A     D  >", C_TEXTO_M, false);
    txt_c(GFX_W / 2, yb + ty + 2, "TAB troca a seção", C_TEXTO_M, false);
    txt(GFX_W - 18 - gfx_largura("ENTER volta", 1), yb + ty + 2, "ENTER volta", C_TEXTO_M, false);
}

// Fechado: a peca fica no escuro, como na sombra (o fundo do disco nao muda).
static void no_escuro(int cx, int cy, int R, uint16_t fundo)
{
    uint16_t *fb = display_fb();
    for (int y = cy - R; y <= cy + R; y++) {
        if (y < 0 || y >= GFX_H) continue;
        for (int x = cx - R; x <= cx + R; x++) {
            if (x < 0 || x >= GFX_W || (x - cx) * (x - cx) + (y - cy) * (y - cy) > R * R) continue;
            uint16_t *p = &fb[y * GFX_W + x];
            if (*p != fundo) *p = gfx_mistura(*p, C_SOMBRA, 62);
        }
    }
}

// "Bloqueado" e, ao lado e miudo, o que falta para abrir.
static void linha_bloqueio(int x, int y, const char *falta)
{
    cadeado(x + 4, y + 8, C_VINHO_CLR, 1);
    int xb = x + 14;
    txt(xb, y, "Bloqueado", C_VINHO_CLR, true);
    gfx_texto(xb + gfx_largura("Bloqueado", 1) + 8, y, falta, C_TEXTO_M, 1, false);
}

static void desenha_catalogo(void)
{
    if (cat_sec == 1) { desenha_cat_amuleto(cat_a); return; }
    int tipo = cat_tipo(cat_i);
    const tipo_t *d = &TIPO[tipo];
    char s[32];
    faixas_colecao(cat_i, D_N);
    char trava[80];
    int st = cat_trava_dado(tipo, trava, sizeof trava);

    // A peca, a esquerda, girando pelas faces possiveis.
    int cx = 160, cy = CENTRO + 4;
    gfx_disco(cx, cy, 80, gfx_mistura(C_FELTRO_ESC, cor_rar(tipo), 55));   // anel da raridade
    gfx_disco(cx, cy, 77, C_FELTRO_ESC);
    if (tipo == D_TUDO_NADA) {                          // o yin-yang, girando devagar
        so_apagado = true;                              // fechado: apagado
        tudo_ou_nada((float)cx, (float)cy, 46, t_fase * 0.8f, -1, st == 2);
        so_apagado = false;
    } else if (d->lados == 2) {
        moeda_girando(cx, cy, tipo, t_fase);
    } else {
        int f[24], nf = faces_de(tipo, f);
        int i = (int)(t_fase / 0.45f) % nf;
        float dentro = fmodf(t_fase, 0.45f);
        float r = 46 + (dentro < 0.08f ? 4 : 0);          // pulinho a cada face
        float ang = sinf(t_fase * 1.7f) * 0.22f;
        bool com_pips = d->lados == 6 && d->efeito != EF_ESPELHO;
        so_apagado = true;
        desenha_dado((float)cx, (float)cy, r, tipo, 0, ang, com_pips ? f[i] : -1, 2, st == 2);
        so_apagado = false;
        if (!com_pips) {
            snprintf(s, sizeof s, "%d", f[i]);
            uint16_t corpo = C_MARFIM_D, aro = 0, tinta = C_EBANO;
            cores_do_tipo(tipo, 0, &corpo, &aro, &tinta);           // a Viuva tem tinta clara
            gfx_texto(cx - gfx_largura(s, 2) / 2, cy - 24, s, st == 2 ? C_FELTRO2 : tinta, 2, true);
        }
        if (tipo == D_TREVO && f[i] == 4) {
            gfx_rect(cx - 30, cy + 58, 60, TXT_H + 4, C_BARRA);
            txt_c(cx, cy + 60, "SORTE", C_OURO, true);
        }
    }

    if (st == 2) {                                       // fechado: no escuro, cadeado no aro
        no_escuro(cx, cy, 76, C_FELTRO_ESC);
        cadeado(cx + 54, cy - 66, C_VINHO_CLR, 2);
    }

    // A direita: nome, lados, raridade e o que ele faz.
    int x = 290;
    gfx_texto(x, 70, d->nome, st == 2 ? C_TEXTO_M : cor_aro(tipo), 2, true);
    classe(tipo, s, sizeof s);
    int wc = gfx_largura(s, 1) + 12;
    ui_etiqueta(x, 118, wc, TXT_H + 2, cor_rar(tipo));
    txt(x + 6, 119, s, cor_rar(tipo), false);
    int xt = etiquetas_protecao(x + wc + 8, 118, tipo);
    if (so_inicial(tipo)) txt(xt + 2, 119, "dado inicial", C_TEXTO_M, false);
    if (so_desafio(tipo)) txt(xt + 2, 119, "só no Desafiante", C_TEXTO_M, false);
    if (st == 2) {
        linha_bloqueio(x, 139, trava);
    } else if (st == 1) {
        txt(x, 139, trava, C_VERDE, false);
    }
    gfx_rect(x, 160, 320, 1, C_FELTRO2);
    quebra_linhas(x, 170, 320, 6, TEXTO_CAT[tipo], C_MARFIM);
}

// O dado em foco na partida: o do cursor na mao, na loja ou no premio, ou o
// proximo da fila. -1 se nao ha nenhum.
static int tipo_em_foco(void)
{
    switch (fase) {
    case F_ORDEM:
        if (J[vez].n_mao && cursor >= 0 && cursor < J[vez].n_mao) return J[vez].col[J[vez].mao[cursor]].tipo;
        break;
    case F_JOGA:
        if (F[vez].lancados < F[vez].n) return tipo_de(vez, F[vez].lancados);
        break;
    case F_LOJA:
        if (cursor >= 0 && cursor < N_LOJA) return loja[cursor].tipo;
        break;
    case F_PREMIO:
        if (!removendo && cursor >= 0 && cursor < 3) return oferta[cursor].tipo;
        break;
    default:
        return des_foco();
    }
    return -1;
}

// Tecla I: o texto inteiro do dado em foco, por cima da mesa. So olha: a
// partida fica parada onde estava, e qualquer tecla fecha.
static void desenha_detalhe(void)
{
    if (detalhe < 0) return;
    if (fase != detalhe_fase) { detalhe = -1; return; }   // a partida andou
    int tipo = detalhe;
    const int x0 = 70, y0 = 70, w = GFX_W - 140, h = 220;
    gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_SOMBRA, 140);
    ui_cartao(x0, y0, w, h, true, cor_rar(tipo));
    int cx = x0 + 78, cy = y0 + 96;
    gfx_disco(cx, cy, 52, gfx_mistura(C_FELTRO_ESC, cor_rar(tipo), 55));
    gfx_disco(cx, cy, 49, C_FELTRO_ESC);
    peca_t pc = { (uint8_t)tipo, 0 };               // nova_peca sortearia: aqui so se olha
    peca_parada((float)cx, (float)cy, 32, pc, 0);
    int x = x0 + 160;
    gfx_texto(x, y0 + 16, TIPO[tipo].nome, cor_aro(tipo), 1, true);
    char s[32];
    classe(tipo, s, sizeof s);
    int wn = gfx_largura(TIPO[tipo].nome, 1), wc = gfx_largura(s, 1) + 10;
    ui_etiqueta(x + wn + 12, y0 + 18, wc, TXT_H - 2, cor_rar(tipo));
    gfx_texto(x + wn + 17, y0 + 16, s, cor_rar(tipo), 1, false);
    etiquetas_protecao(x + wn + 12 + wc + 6, y0 + 17, tipo);
    gfx_rect(x, y0 + 44, w - 180, 1, C_FELTRO2);
    quebra_linhas(x, y0 + 54, w - 180, 6, TEXTO_CAT[tipo], C_MARFIM);
    txt_c(GFX_W / 2, y0 + h - 28, "qualquer tecla fecha", C_TEXTO_M, false);
}

// O nome do jogador na faixa, com "computador" ao lado quando e a IA.
static int txt_nome(int x, int y, int j, bool destaque)
{
    int w = gfx_texto(x, y, J[j].nome, J[j].cor, 1, true);
    (void)destaque;
    return w;
}

// Faixas de cima e de baixo das telas fora da partida.
static void faixas_menu(const char *titulo, const char *dir, const char *rodape)
{
    int ty = (FAIXA_H - TXT_H) / 2 - 2, yb = GFX_H - FAIXA_H;
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    txt(botao_voltar(KEY_ESC), ty, titulo, C_LATAO, true);
    if (dir) txt(GFX_W - 18 - gfx_largura(dir, 1), ty, dir, C_TEXTO_M, false);
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
    txt_c(GFX_W / 2, yb + ty + 2, rodape, C_TEXTO_M, false);
}

// Botao grande de menu, centrado.
static void botao_menu(int y, int w, const char *rot, bool cur)
{
    ui_botao(GFX_W / 2 - w / 2, y, w, 26, rot, cur ? C_LATAO : 0, 0);
}

// Quatro casas com as letras do codigo da sala.
static void casas_codigo(int y, const char *cod, bool cursor_pisca)
{
    for (int i = 0; i < 4; i++) {
        int x = GFX_W / 2 - 110 + i * 56;
        ui_painel(x, y, 48, 56);
        char c[2] = { cod[i] ? cod[i] : 0, 0 };
        if (c[0]) gfx_texto(x + 24 - gfx_largura(c, 2) / 2, y + 6, c, C_MARFIM, 2, true);
        else if (cursor_pisca && i == (int)strlen(cod) && (int)(t_fase * 2) % 2 == 0)
            gfx_rect(x + 14, y + 44, 20, 3, C_OURO);
    }
}

#define URL_WEB "ericzv.github.io/dices"

static void desenha_online(void)
{
    faixas_menu("ONLINE", NULL, "ESC volta ao menu");
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    if (!rede_disponivel()) {
        txt_c2(GFX_W / 2, 84, "Jogar online", C_LATAO);
        txt_c(GFX_W / 2, 140, "O modo online funciona na versão do jogo para o navegador:", C_MARFIM, false);
        txt_sombra_c(GFX_W / 2, 176, URL_WEB, C_OURO, 2);
        txt_c(GFX_W / 2, 236, "Abra esse endereço, escolha Online e crie uma sala.", C_MARFIM_S, false);
        txt_c(GFX_W / 2, 270, "ENTER volta", C_TEXTO_M, false);
        return;
    }
    int st = rede_estado();
    txt_c2(GFX_W / 2, 64, "Jogar online", C_LATAO);
    if (st == REDE_ERRO) {
        txt_c(GFX_W / 2, 130, "Não deu para conectar:", C_VINHO_CLR, true);
        txt_c(GFX_W / 2, 156, rede_erro(), C_MARFIM, false);
        txt_c(GFX_W / 2, 200, "ENTER tenta de novo  ·  ESC volta", C_TEXTO_M, false);
    } else if (st == REDE_CONECTANDO) {
        char t[32];
        snprintf(t, sizeof t, "conectando%.*s", (int)(t_fase * 3) % 4, "...");
        txt_c(GFX_W / 2, 150, t, C_MARFIM, true);
    } else if (st == REDE_ESPERANDO) {
        txt_c(GFX_W / 2, 116, "Sala criada! Passe este código para o seu amigo:", C_MARFIM, true);
        casas_codigo(146, rede_codigo(), false);
        char t[48];
        snprintf(t, sizeof t, "esperando o amigo entrar%.*s", (int)(t_fase * 3) % 4, "...");
        txt_c(GFX_W / 2, 222, t, C_OURO, false);
        txt_c(GFX_W / 2, 250, "Ele abre o jogo, escolhe Online > Entrar com código e digita o código.",
              C_TEXTO_M, false);
    } else if (digitando) {
        txt_c(GFX_W / 2, 116, "Digite o código da sala do seu amigo:", C_MARFIM, true);
        casas_codigo(146, codigo_dig, true);
        txt_c(GFX_W / 2, 222, strlen(codigo_dig) == 4 ? "ENTER entra na sala" : "4 letras ou números",
              strlen(codigo_dig) == 4 ? C_OURO : C_TEXTO_M, false);
        txt_c(GFX_W / 2, 250, "BACKSPACE apaga  ·  ESC volta", C_TEXTO_M, false);
    } else {
        txt_c(GFX_W / 2, 112, "Cada um no seu computador: um cria a sala, o outro entra com o código.",
              C_MARFIM_S, false);
        botao_menu(150, 240, "Criar sala", online_cur == 0);
        botao_menu(186, 240, "Entrar com código", online_cur == 1);
        zona_clique(GFX_W / 2 - 120, 150, 240, 26, &online_cur, 0, KEY_ENTER);
        zona_clique(GFX_W / 2 - 120, 186, 240, 26, &online_cur, 1, KEY_ENTER);
        txt_c(GFX_W / 2, 238, "Quem cria a sala joga primeiro na loja.", C_TEXTO_M, false);
    }
}

// ---------------------------------------------------------------------------
// Tutorial
// ---------------------------------------------------------------------------
typedef struct { const char *titulo; const char *par[3]; } pagina_t;
static const pagina_t TUTORIAL[] = {
    { "O jogo", {
        "Cada jogador começa com 100 fichas. São 15 rodadas; a entrada de cada uma vai para o pote: 5 fichas, 10 a partir da 6ª rodada e 15 a partir da 11ª.",
        "Quem faz mais pontos na rodada leva o pote. No fim, ganha quem tiver mais fichas.",
        NULL } },
    { "A bolsa e a sequência", {
        "Cada jogador tem uma bolsa de dados. A cada rodada, você tira 7 dados dela e escolhe até 4 para a sua sequência, na ordem que quiser.",
        "Depois joga os dados da sequência um por um, do primeiro ao último.",
        NULL } },
    { "A regra da queda", {
        "O primeiro dado sempre vale. Cada dado seguinte precisa ser IGUAL OU MAIOR que o último que vale.",
        "Se sair menor, ele e o último que valia são anulados juntos. Por isso a ordem importa: dados pequenos na frente, grandes no fim.",
        NULL } },
    { "Parar ou arriscar", {
        "Depois de cada dado: jogar o próximo ou parar com os pontos que tem. O botão jogar mostra a chance de cair (some nas dificuldades altas).",
        "Quem venceu a rodada anterior joga primeiro, no escuro. Quem joga depois já sabe o alvo: o botão parar mostra se vence, perde ou empata.",
        NULL } },
    { "Apostas", {
        "Entre os dois turnos, com os pontos de quem jogou primeiro na mesa, dá para apostar 10 ou 25 fichas.",
        "O outro escolhe: pagar para ver, dobrar a aposta ou correr. Quem corre entrega o pote na hora.",
        NULL } },
    { "Loja, prêmios e dados especiais", {
        "A loja do começo vende dados especiais. Quem vence uma rodada escolhe um prêmio: um dado novo ou tirar um fraco da bolsa.",
        "São %d dados, cada um com um poder: veja todos em Dados da casa.",
        NULL } },
    { "Modo Desafiante", {
        "Em 1 jogador: uma run por 3 salões do cassino, cada um com 5 mesas e o chefe. No mapa você escolhe o caminho: oponentes, lojas e eventos.",
        "Cada partida tem 8 rodadas e 50 fichas de cada lado. Venceu: as fichas viram moedas e você ganha um prêmio. Perdeu, a run acaba.",
        "Vencer o Barão abre a dificuldade seguinte, e os chefes abrem dados novos. Cada perfil guarda o seu progresso e a sua run." } },
    { "Teclas", {
        "Setas movem  ·  ENTER escolhe e joga  ·  TAB/CTRL confirma ou para  ·  BACKSPACE tira  ·  I mostra o dado escolhido",
        "Mouse e toque também jogam: clique no que quiser. O < no alto volta ou sai da partida.",
        "X recusa  ·  ESC duas vezes: menu  ·  M som  ·  N música  ·  V velocidade  ·  F11 tela cheia  ·  - e + tamanho da mesa.  Boa sorte!" } },
};
#define N_TUTORIAL ((int)(sizeof TUTORIAL / sizeof TUTORIAL[0]))

// Um botao de mentira nas ilustracoes, aceso ou apagado.
static void rotulo_tut(int cx, int y, const char *s, bool aceso)
{
    int w = gfx_largura(s, 1) + 24;
    ui_botao(cx - w / 2, y, w, 26, s, aceso ? C_VINHO : 0, aceso ? UI_FOCO : 0);
}

// Ilustracoes: dados de verdade, desenhados pelo mesmo codigo da mesa.
static void ilustra_tutorial(int pag, int y)
{
    int cx = GFX_W / 2;
    switch (pag) {
    case 0:                                          // o pote
        for (int i = 0; i < 6; i++) ficha(cx - 60, y + 30 - i * 4, i % 2 ? C_VINHO : C_LATAO);
        for (int i = 0; i < 4; i++) ficha(cx + 60, y + 30 - i * 4, i % 2 ? C_MARFIM_S : C_ROSA);
        txt_c(cx - 60, y + 44, "suas fichas", C_TEXTO_M, false);
        txt_c(cx + 60, y + 44, "pote", C_TEXTO_M, false);
        break;
    case 1: {                                        // a mao e a ordem
        static const uint8_t M[7] = { D_D6, D_D2, D_D6, D_D4, D_D8, D_D6, D_D2 };
        static const int8_t O[7] = { 3, 1, -1, 2, 4, -1, -1 };
        for (int i = 0; i < 7; i++) {
            int x = cx + (i - 3) * 58;
            peca_parada((float)x, (float)y + 26, 19, nova_peca(M[i]), 0);
            if (O[i] > 0) {
                char o[2] = { (char)('0' + O[i]), 0 };
                gfx_disco(x + 17, y + 6, 10, C_OURO);
                gfx_disco(x + 17, y + 6, 8, C_AMARELO);
                txt_c(x + 17, y - 5, o, C_BARRA, true);
            }
        }
        break;
    }
    case 2: {                                        // 2 vale, 4 vale, sai 3: anula o 3 e o 4
        static const int V[3] = { 2, 4, 3 };
        int passo = (int)(t_fase / 1.1f) % 4;        // 0: so o 2 ... 3: a queda
        for (int i = 0; i < 3 && i <= passo; i++) {
            int x = cx + (i - 1) * 110;
            bool anul = passo == 3 && i > 0;
            desenha_dado((float)x, (float)y + 22, 22, D_D6, 0, 0, V[i], 2, anul);
            const char *r = anul ? (i == 2 ? "menor: anula" : "anulado") : "vale";
            txt_c(x, y + 52, r, anul ? C_VINHO_CLR : C_OURO, false);
        }
        break;
    }
    case 3: {                                        // dois dados valendo; jogar ou parar
        desenha_dado((float)cx - 214, (float)y + 22, 20, D_D6, 0, 0, 2, 2, false);
        desenha_dado((float)cx - 162, (float)y + 22, 20, D_D6, 0, 0, 4, 2, false);
        txt_c(cx - 188, y + 46, "6 pontos", C_OURO, false);
        bool parar = (int)(t_fase / 1.4f) % 2;
        rotulo_tut(cx - 6, y + 10, "jogar: 50% de cair", !parar);
        rotulo_tut(cx + 170, y + 10, "parar: vence por 1", parar);
        break;
    }
    case 4: {                                        // aposta na mesa; pagar, dobrar ou correr
        for (int i = 0; i < 5; i++) ficha(cx - 190, y + 30 - i * 4, i % 2 ? C_VINHO : C_LATAO);
        txt_c(cx - 190, y + 44, "aposta 25", C_TEXTO_M, false);
        static const char *const R[3] = { "pagar", "dobrar", "correr" };
        int k = (int)(t_fase / 1.2f) % 3;
        for (int i = 0; i < 3; i++) rotulo_tut(cx - 50 + i * 110, y + 10, R[i], i == k);
        break;
    }
    case 5: {                                        // alguns dados especiais
        static const uint8_t E[5] = { D_ESCUDO, D_JACKPOT, D_EGOISTA, D_COPIADOR, D_FOGO };
        for (int i = 0; i < 5; i++) {
            int x = cx + (i - 2) * 100;
            float bob = sinf(t_fase * 3 + i) * 2;
            peca_parada((float)x, y + 24 + bob, 19, nova_peca(E[i]), 0);
            txt_c(x, y + 46, TIPO[E[i]].nome, cor_aro(E[i]), false);
        }
        break;
    }
    default: break;
    }
}

static void desenha_tutorial(void)
{
    char s[32];
    snprintf(s, sizeof s, "%d de %d", tut_pag + 1, N_TUTORIAL);
    faixas_menu("COMO JOGAR", s, tut_pag + 1 < N_TUTORIAL
                ? "<  anterior    ·    ENTER ou  >  próxima    ·    ESC menu"
                : "<  anterior    ·    ENTER volta ao menu");
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    const pagina_t *pg = &TUTORIAL[tut_pag];
    txt_sombra_c(GFX_W / 2, MESA_Y0 + 8, pg->titulo, C_LATAO, 2);
    int y = MESA_Y0 + 52;
    for (int i = 0; i < 3 && pg->par[i]; i++) {
        char par[256];                               // "%d": quantos dados ha
        snprintf(par, sizeof par, pg->par[i], D_BASICOS);
        int n = quebra_linhas(44, y, 552, 4, par, i == 0 ? C_MARFIM : C_MARFIM_S);
        y += n * (TXT_H + 4) + 4;
    }
    // A ilustracao vai abaixo do texto: na faixa de sempre ou, se o texto
    // desceu mais, logo depois dele.
    int yi = y + 10 > 226 ? y + 10 : 226;
    if (yi > 234) yi = 234;                          // sem encostar nas bolinhas
    ilustra_tutorial(tut_pag, yi);
    // Bolinhas das paginas.
    for (int i = 0; i < N_TUTORIAL; i++)
        gfx_disco(GFX_W / 2 + (i - N_TUTORIAL / 2) * 16, MESA_Y1 - 6, 3,
                  i == tut_pag ? C_OURO : C_FELTRO2);
}

// Avisos por cima de tudo: sair da partida e conexao perdida.
// A entrada subiu: uma faixa vermelha e dourada no alto da mesa, com fichas
// pulando dos lados, por uns dois segundos e meio.
static void desenha_aviso_entrada(void)
{
    if (t_aviso_entrada <= 0) return;
    float e = ENTRADA_AVISO_T - t_aviso_entrada;
    float abre = e < 0.18f ? e / 0.18f : (t_aviso_entrada < 0.25f ? t_aviso_entrada / 0.25f : 1);
    int h = (int)(46 * abre), y = MESA_Y0 + 34 - h / 2;
    if (h < 4) return;
    gfx_rect(MESA_X0, y + 3, MESA_X1 - MESA_X0, h, C_SOMBRA);
    gfx_rect(MESA_X0, y, MESA_X1 - MESA_X0, h, RGB(120, 26, 34));
    gfx_rect(MESA_X0, y, MESA_X1 - MESA_X0, 2, C_OURO);
    gfx_rect(MESA_X0, y + h - 2, MESA_X1 - MESA_X0, 2, C_OURO);
    if (abre < 1) return;
    char t[40];
    snprintf(t, sizeof t, "ENTRADA: %d FICHAS", ante_da(rodada));
    bool pisca = (int)(e / 0.25f) % 2 == 0 && e < 1.0f;      // pisca no comeco
    txt_sombra_c(GFX_W / 2, y + 7, t, pisca ? C_AMARELO : C_OURO, 2);
    int w = gfx_largura(t, 2) / 2;
    for (int lado = -1; lado <= 1; lado += 2)                // pilhas de fichas pulando
        for (int i = 0; i < 3; i++) {
            int x = GFX_W / 2 + lado * (w + 26 + i * 18);
            int pulo = (int)(fabsf(sinf(e * 7 + i * 1.1f)) * 5);
            ficha(x, y + h / 2 - pulo, i % 2 ? C_VINHO : C_LATAO);
        }
}

static void desenha_avisos(void)
{
    const char *msg = NULL;
    uint16_t c = C_OURO;
    if (conexao_caiu()) { msg = "A conexão com o oponente caiu.  ESC volta ao menu."; c = C_VINHO_CLR; }
    else if (t_sair >= 0) msg = "Aperte ESC (ou toque no <) de novo para sair da partida.";
    else if (t_aviso_vel > 0) msg = rapido ? "Velocidade: rápida  (V troca)" : "Velocidade: normal  (V troca)";
    if (!msg) return;
    int w = gfx_largura(msg, 1) + 40;
    ui_painel(GFX_W / 2 - w / 2, MESA_Y0 + 6, w, TXT_H + 12);
    txt_c(GFX_W / 2, MESA_Y0 + 12, msg, c, false);
}

#include "desafio.inc"

static void desenha_tudo(void);

// ---------------------------------------------------------------------------
// Dica do mouse: uma caixa com o nome e o que a coisa faz, por cima de tudo.
// ---------------------------------------------------------------------------

static void dica_textos(int tipo, int id, const char **nome, uint16_t *cor, const char **desc)
{
    if (tipo == DICA_DADO)         { *nome = TIPO[id].nome; *cor = cor_aro(id); *desc = TEXTO_MEDIO[id]; }
    else if (tipo == DICA_AMULETO) { *nome = AMULETO[id].nome; *cor = AMULETO[id].cor; *desc = AMULETO[id].desc; }
    else {
        *nome = PERSO[id].trunfo; *cor = C_ROSA; *desc = PERSO[id].regra;
        int v = virada_de(id);
        if (v) {                                         // o chefe tambem conta a virada
            static char t[200];
            char vt[96];
            virada_txt(id - PERSO_CHEFE0, v, false, vt, sizeof vt);
            snprintf(t, sizeof t, "%s Virada na 5ª rodada: %s", PERSO[id].regra, vt);
            *desc = t;
        }
    }
}

// A caixa da dica sob o mouse, se houver. Devolve o retangulo em r.
static bool dica_rect(int *r, const dica_t **qual)
{
    if (mouse_x < 0 || (detalhe >= 0 && fase == detalhe_fase)) return false;
    const dica_t *d = NULL;
    for (int i = n_dicas - 1; i >= 0 && !d; i--)
        if (mouse_em(dicas[i].x, dicas[i].y, dicas[i].w, dicas[i].h)) d = &dicas[i];
    if (!d) return false;
    const char *nome, *desc;
    uint16_t cor;
    dica_textos(d->tipo, d->id, &nome, &cor, &desc);
    int larg = 260;
    quebra_so_conta = true;                              // so conta as linhas
    int linhas = quebra_linhas(0, 0, larg - 16, 7, desc, 0);
    quebra_so_conta = false;
    int w = larg, h = 10 + (TXT_H + 2) + linhas * (TXT_H + 4) + 4;
    int x = mouse_x + 14, y = mouse_y + 16;
    if (x + w > GFX_W - 4) x = mouse_x - 14 - w;
    if (y + h > GFX_H - 4) y = mouse_y - 12 - h;
    if (x < 4) x = 4;
    if (y < 4) y = 4;
    r[0] = x; r[1] = y; r[2] = w; r[3] = h;
    *qual = d;
    return true;
}

static void desenha_dica(const int *r, const dica_t *d)
{
    const char *nome, *desc;
    uint16_t cor;
    dica_textos(d->tipo, d->id, &nome, &cor, &desc);
    ui_painel(r[0], r[1], r[2], r[3]);
    gfx_rect(r[0] + 3, r[1] + 3, 3, r[3] - 6, cor);             // a cor da coisa, na beira
    int x = r[0] + 8, y = r[1] + 5;
    int w = gfx_texto(x, y, nome, cor, 1, true);
    if (d->tipo == DICA_DADO) {                          // "D4 · raro", miudo, ao lado
        char c2[32];
        classe(d->id, c2, sizeof c2);
        gfx_texto(x + w + 8, y + 3, c2, cor_rar(d->id), GFX_MIUDO, false);
    }
    quebra_linhas(x, y + TXT_H + 4, r[2] - 16, 7, desc, C_MARFIM_S);
}

void dado_desenha(void)
{
    n_zonas = 0;
    n_dicas = 0;
    zonas_fase = fase;
    desenha_tudo();
    balao_oponente();
    desenha_confete();
    bool menu = fase == F_CATALOGO || fase == F_TUTORIAL || fase == F_ONLINE || fase == F_ABERTURA;
    if (!menu) {
        desenha_detalhe();
        desenha_avisos();
        desenha_aviso_entrada();
        des_elevador();
        des_virada_anuncio();
    }
    int r[4];
    const dica_t *d = NULL;
    bool tem_dica = !menu && dica_rect(r, &d);
    if (tem_dica) desenha_dica(r, d);
    gfx_quantiza();                                      // o quadro so com cores da paleta
}

static void desenha_tudo(void)
{
    mesa();
    if (des_desenha()) return;
    if (fase == F_CATALOGO) { desenha_catalogo(); return; }
    if (fase == F_TUTORIAL) { desenha_tutorial(); return; }
    if (fase == F_ONLINE)   { desenha_online(); return; }
    if (fase == F_ABERTURA) {
        desenha_abertura();
        return;
    }
    faixa_jogador(1);
    faixa_jogador(0);

    switch (fase) {
    case F_LOJA:   desenha_loja();   break;
    case F_PREMIO: desenha_premio(); break;
    case F_FIM:    if (modo == M_DESAFIO) desenha_fim_partida_run(); else desenha_fim(); break;
    case F_ORDEM:
        desenha_fila(outro(vez));
        desenha_ordem();
        break;
    default:
        desenha_fila(outro(vez));
        desenha_fila(vez);
        if (fase == F_APOSTA)         desenha_aposta();
        else if (fase == F_RESULTADO) desenha_resultado();
        else if (fase == F_EFEITOS)   desenha_efeitos();
        else                          desenha_joga();
        if (fase == F_ROLANDO || fase == F_ARRUMA) desenha_lance();
        break;
    }
}

// ===========================================================================
// Teclado
// ===========================================================================
static int eixo(int k)
{
    if (k == 'a' || k == ',' || k == 'w' || k == ';' || k == KEY_LEFT || k == KEY_UP) return -1;
    if (k == 'd' || k == '/' || k == 's' || k == '.' || k == KEY_RIGHT || k == KEY_DOWN) return 1;
    return 0;
}

// A partida acaba ao fim desta rodada? (ultima rodada, ou alguem sem
// fichas para a proxima entrada). Entao nao ha premio: o jogo encerra.
static bool ultima_rodada(void)
{
    int prox = ante_da(rodada + 1);
    return rodada >= MAX_RODADAS || J[0].fichas < prox || J[1].fichas < prox;
}

static void segue_do_resultado(void)
{
    // Quem venceu a rodada, ou quem ficou com o pote porque o outro correu,
    // escolhe um premio - menos quando a partida ja acabou.
    int w = (venc_mao == 0 || venc_mao == 1) ? venc_mao : correu_venc;
    correu_venc = -1;
    // No Desafiante nao ha premio por rodada: so ao fim da partida.
    if (w >= 0 && !ultima_rodada() && modo != M_DESAFIO) {
        primeiro = w;
        abre_premio(w);
    } else {
        if (w >= 0) primeiro = w;
        if (venc_mao == 2) primeiro ^= 1;
        nova_rodada();
    }
}

static void poe_na_fila(int i)
{
    fila_t *f = &F[vez];
    jogador_t *p = &J[vez];
    int c = p->mao[i];
    int ja = -1;
    for (int k = 0; k < f->n; k++) if (f->idx[k] == c) ja = k;
    if (ja >= 0) {
        for (int k = ja; k + 1 < f->n; k++) {
            f->idx[k] = f->idx[k + 1];
            f->tipo[k] = f->tipo[k + 1];
            f->fixo[k] = f->fixo[k + 1];
        }
        f->n--;
    } else if (f->n < max_fila(vez)) {
        f->idx[f->n] = (int8_t)c;
        f->tipo[f->n] = p->col[c].tipo;
        f->fixo[f->n] = p->col[c].fixo;
        f->n++;
    }
    som_toca(SOM_TIQUE);
}

void dado_tecla(const key_event_t *ev_)
{
    int k = ev_->key;
    if (k >= 'A' && k <= 'Z') k += 32;

    if (t_elev > 0) { t_elev = 0; return; }        // qualquer tecla pula o elevador
    if (t_virada > 0) { t_virada = 0; return; }    // e o anuncio da virada
    if (k == 'v' && !dado_digitando()) {
        rapido = !rapido;
        salva_grava("vel", rapido ? "1" : "0");
        t_aviso_vel = 1.6f;
        som_toca(SOM_TIQUE);
        return;
    }
    if (k == 'm' && !dado_digitando()) {
        bool on = som_liga(!som_ligado());
        if (!on && som_falhou()) snprintf(nota, sizeof nota, "sem saída de som neste computador");
        if (on) som_toca(SOM_FICHA);
        return;
    }
    // Detalhes do dado (tecla I): so da tela local, nunca vai para o rival.
    if (detalhe >= 0 && fase == detalhe_fase) { detalhe = -1; return; }   // qualquer tecla fecha
    detalhe = -1;
    // Roleta girando: a tecla so trava os rolos, aqui mesmo (nao vai para o
    // rival, que ve a propria roleta).
    if (slot_girando() && k != KEY_ESC) { slot_pula(); return; }
    if (k == 'i' && !dado_digitando() && tipo_em_foco() >= 0) {
        detalhe = tipo_em_foco();
        detalhe_fase = fase;
        som_toca(SOM_TIQUE);
        return;
    }
    if (k == KEY_ESC) { trata_esc(); return; }
    t_sair = -1;
    if (ia_na_vez()) return;             // a vez e do aparelho: so ele escolhe
    if (modo == M_ONLINE) {
        if (rede_na_vez()) return;       // a vez e do rival: as teclas vem dele
        if (aceita_escolha()) rede_envia_tecla(k);
    }
    trata_tecla(k);
}

static void manda_tecla(int k)
{
    key_event_t ev = { 0 };
    ev.key = k;
    dado_tecla(&ev);
}

// O mouse esta sobre algo clicavel? (a plataforma troca a seta pela maozinha)
bool dado_mouse_clicavel(void)
{
    if (zonas_fase != fase) return false;
    bool alheia = ia_na_vez() || (modo == M_ONLINE && rede_na_vez());
    for (int i = 0; i < n_zonas; i++)
        if (mouse_em(zonas[i].x, zonas[i].y, zonas[i].w, zonas[i].h) && (!alheia || zonas[i].tecla == KEY_ESC))
            return true;
    return false;
}

// Mouse, ja nas coordenadas da tela do jogo. acao: 0 moveu, 1 clique
// esquerdo, 2 clique direito (o mesmo que BACKSPACE: volta ou desfaz).
void dado_mouse(int x, int y, int acao)
{
    mouse_x = x;
    mouse_y = y;
    if (acao == 2) { manda_tecla(KEY_BKSP); return; }
    if (detalhe >= 0 && fase == detalhe_fase) {         // um clique fecha os detalhes
        if (acao == 1) manda_tecla(KEY_ENTER);
        return;
    }
    const zona_clique_t *z = NULL;
    for (int i = zonas_fase == fase ? n_zonas - 1 : -1; i >= 0 && !z; i--)
        if (mouse_em(zonas[i].x, zonas[i].y, zonas[i].w, zonas[i].h)) z = &zonas[i];
    // Na vez da IA ou do rival online, o mouse nao mexe em nada; so o "<" de
    // sair vale sempre.
    if (ia_na_vez() || (modo == M_ONLINE && rede_na_vez())) {
        if (acao == 1 && z && z->tecla == KEY_ESC) manda_tecla(KEY_ESC);
        return;
    }

    if (acao == 0) {
        // Passar por cima escolhe. No online o cursor so anda pelas teclas,
        // que vao para o rival: la so o clique vale.
        if (z && z->var && *z->var != z->val && modo != M_ONLINE) {
            *z->var = z->val;
            som_toca(SOM_TIQUE);
        }
        return;
    }
    if (z) {
        if (z->var && modo == M_ONLINE) {
            for (int i = 0; i < 64 && *z->var != z->val; i++) manda_tecla(KEY_RIGHT);
            if (*z->var != z->val) return;
        } else if (z->var) {
            *z->var = z->val;
        }
        manda_tecla(z->tecla);
        return;
    }
    // Fora dos botoes: nas telas de "aperte para seguir", o clique segue.
    switch (fase) {
    case F_CATALOGO:
        manda_tecla(x < GFX_W / 2 ? KEY_LEFT : KEY_RIGHT);
        break;
    case F_TUTORIAL:
        manda_tecla(x < GFX_W / 3 ? KEY_LEFT : KEY_ENTER);
        break;
    case F_ABERTURA:
        if (!abertura_pronta || t_fase < 0.75f) manda_tecla(KEY_ENTER);   // pula a entrada
        break;
    case F_ONLINE:
        if (!rede_disponivel() || rede_estado() == REDE_ERRO) manda_tecla(KEY_ENTER);
        break;
    case F_RESULTADO: case F_FIM: case F_RFIM: case F_VEREDITO: case F_EFEITOS:
        manda_tecla(KEY_ENTER);
        break;
    case F_PREMIO:
        if (slot_girando()) manda_tecla(KEY_ENTER);       // trava a roleta
        break;
    default:
        break;
    }
}

// A tecla ja traduzida, venha de quem vier: pessoa ou IA.
static void trata_tecla(int k)
{
    int e = eixo(k);
    if (e && (fase == F_ORDEM || fase == F_APOSTA || fase == F_JOGA ||
              fase == F_PREMIO || fase == F_LOJA))
        som_toca(SOM_TIQUE);

    switch (fase) {
    case F_ABERTURA:
        if (!abertura_pronta || t_fase < 0.75f) {             // pula para o menu
            for (int i = 0; i < n_voo; i++) {
                voo[i].parado = true; voo[i].face = voo[i].valor;
                voo[i].vx = voo[i].vy = 0;
            }
            abertura_pronta = true;
            t_fase = 1.0f;
            return;
        }
        {
        const char *op[MN_N];
        int n_op = menu_opcoes(op);
        if (e) {
            menu_cur = (menu_cur + e + n_op) % n_op;
            sacode = 1;                              // como se alguem mexesse na mesa
            som_toca(SOM_TIQUE);
            return;
        }
        if (k == KEY_BKSP && menu_tela) { menu_volta(); return; }
        if (k >= '1' && k < '1' + n_op) { menu_cur = k - '1'; sacode = 1; }
        if (k != KEY_ENTER && !(k >= '1' && k < '1' + n_op)) return;
        }
        som_toca(SOM_FICHA);
        sacode = 1;
        if (menu_tela == MT_UM) {
            if (menu_cur == 0) { menu_tela = MT_NIVEL; menu_cur = 1; }
            else des_abre_perfis();
            return;
        }
        if (menu_tela == MT_NIVEL) {                // partida rapida no nivel escolhido
            modo = M_UM;
            cpu = 2;
            remoto = 0;
            baixo = 0;
            nivel_jog[1] = menu_cur;
            ousadia_jog[1] = 0;
            poe_nomes();
            abre_loja();
            return;
        }
        if (menu_tela == MT_DESAFIO) {
            if (menu_cur == 0) des_continua(); else des_escolhe_nova();
            return;
        }
        switch (menu_cur) {
        case MN_UM:
            menu_tela = MT_UM;
            menu_cur = 0;
            break;
        case MN_DOIS:
            modo = M_DOIS;
            cpu = 0;
            remoto = 0;
            baixo = 0;
            poe_nomes();
            abre_loja();
            break;
        case MN_ONLINE:
            online_cur = 0;
            digitando = false;
            t_fase = 0;
            fase = F_ONLINE;
            break;
        case MN_DADOS:    t_fase = 0; fase = F_CATALOGO; break;
        default:          tut_pag = 0; t_fase = 0; fase = F_TUTORIAL; break;
        }
        return;

    case F_TUTORIAL:
        if (e) {
            int n = tut_pag + e;
            if (n >= 0 && n < N_TUTORIAL) { tut_pag = n; t_fase = 0; som_toca(SOM_TIQUE); }
            return;
        }
        if (k == KEY_ENTER) {
            if (tut_pag + 1 < N_TUTORIAL) { tut_pag++; t_fase = 0; som_toca(SOM_TIQUE); }
            else ao_menu_sem_partida();
        }
        if (k == KEY_BKSP) ao_menu_sem_partida();
        return;

    case F_ONLINE: {
        if (!rede_disponivel()) {
            if (k == KEY_ENTER || k == KEY_BKSP) ao_menu_sem_partida();
            return;
        }
        int st = rede_estado();
        if (st == REDE_ERRO) { if (k == KEY_ENTER) rede_sai(); return; }
        if (st != REDE_PARADA) return;
        if (digitando) {
            int n = (int)strlen(codigo_dig);
            if (k == KEY_BKSP) { if (n) codigo_dig[n - 1] = 0; else digitando = false; return; }
            if (k == KEY_ENTER && n == 4) { rede_entra(codigo_dig); return; }
            bool letra = k >= 'a' && k <= 'z', num = k >= '0' && k <= '9';
            if (n < 4 && (letra || num)) {
                codigo_dig[n] = (char)(letra ? k - 32 : k);
                codigo_dig[n + 1] = 0;
                som_toca(SOM_TIQUE);
            }
            return;
        }
        if (e) { online_cur ^= 1; som_toca(SOM_TIQUE); return; }
        if (k == KEY_ENTER) {
            if (online_cur == 0) rede_cria();
            else { digitando = true; codigo_dig[0] = 0; }
        }
        return;
    }

    case F_CATALOGO:
        // Cima e baixo (e TAB) trocam a secao; os lados passam as paginas.
        if (k == KEY_UP || k == KEY_DOWN || k == 'w' || k == 's' || k == KEY_TAB) {
            int sec = k == KEY_TAB ? !cat_sec : (k == KEY_DOWN || k == 's');
            if (sec != cat_sec) { cat_sec = sec; t_fase = 0; som_toca(SOM_TIQUE); }
            return;
        }
        if (e) {
            if (cat_sec == 0) cat_i = (cat_i + e + D_N) % D_N;
            else cat_a = (cat_a + e + A_N) % A_N;
            t_fase = 0;
            som_toca(SOM_TIQUE);
            return;
        }
        if (k == KEY_ENTER || k == KEY_BKSP) ao_menu_sem_partida();
        return;

    case F_LOJA: {
        jogador_t *p = &J[vez];
        if (e) { cursor = (cursor + e + N_LOJA) % N_LOJA; return; }
        if (k >= '1' && k <= '5') cursor = k - '1';
        if (k == KEY_ENTER || (k >= '1' && k <= '5')) {
            int tp = loja[cursor].tipo;
            if (levou[vez][cursor]) return;
            if (p->fichas < preco_loja(tp) || p->n_col >= lim_col()) return;
            p->fichas -= preco_loja(tp);
            p->col[p->n_col] = loja[cursor];
            p->onde[p->n_col++] = NA_BOLSA;             // a bolsa comeca cheia
            levou[vez][cursor] = true;
            som_toca(SOM_FICHA);
            return;
        }
        if (k == KEY_TAB || k == 'x') {
            if (vez == loja_primeiro) { vez = outro(vez); cursor = 0; }
            else nova_rodada();
        }
        return;
    }

    case F_ORDEM: {
        jogador_t *p = &J[vez];
        fila_t *f = &F[vez];
        if (!p->n_mao) return;
        if (e) { cursor = (cursor + e + p->n_mao) % p->n_mao; return; }
        if (k >= '1' && k <= '8' && k - '1' < p->n_mao) { cursor = k - '1'; poe_na_fila(cursor); return; }
        if (k == KEY_ENTER) { poe_na_fila(cursor); return; }
        if (k == KEY_BKSP && f->n) { f->n--; return; }
        if ((k == KEY_TAB || k == 'x') && f->n >= 1) { cursor = 0; fase = F_JOGA; }
        return;
    }

    case F_JOGA: {
        fila_t *f = &F[vez];
        bool pode_jogar = f->lancados < f->n, pode_parar = f->lancados > 0;
        // Antes do primeiro lance, o segundo botao e "voltar": a sequencia
        // ainda pode ser mudada.
        bool pode_voltar = f->lancados == 0;
        if (e) { if (pode_parar || pode_voltar) cursor ^= 1; return; }
        if (k == KEY_BKSP && pode_voltar) { cursor = 0; t_fase = 1; fase = F_ORDEM; return; }
        if (k == KEY_TAB || k == 'x') { if (pode_parar) termina_turno(); return; }
        if (k == KEY_ENTER) {
            if (cursor == 1 && pode_voltar) { cursor = 0; t_fase = 1; fase = F_ORDEM; }
            else if (cursor == 1 && pode_parar) termina_turno();
            else if (pode_jogar) lanca_proximo();
        }
        return;
    }

    case F_VEREDITO:
        t_fase = 10.0f;
        return;

    case F_EFEITOS:
        t_fase = 10.0f;                      // qualquer tecla adianta o evento
        return;

    case F_APOSTA: {
        int op[4], n = opcoes(op);
        if (e) { cursor = (cursor + e + n) % n; return; }
        if (k >= '1' && k <= '0' + n) { executa(op[k - '1']); return; }
        if (k == KEY_ENTER) executa(op[cursor]);
        return;
    }

    case F_RESULTADO:
        if (k == KEY_ENTER) segue_do_resultado();
        return;

    case F_PREMIO: {
        jogador_t *p = &J[vez];
        slot_pula();                                // tecla do rival: a roleta daqui trava
        if (t_tira > 0) return;                     // dado saindo: espera
        if (removendo) {
            if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= GRADE) cursor -= GRADE; return; }
            if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + GRADE < p->n_col) cursor += GRADE; return; }
            if (e) { cursor = (cursor + e + p->n_col) % p->n_col; return; }
            if (k == KEY_BKSP || k == 'x') { removendo = false; cursor = 3; return; }
            if (k == KEY_ENTER) { tira_i = cursor; t_tira = 0.001f; som_toca(SOM_ANULA); }
            return;
        }
        if (e) { cursor = (cursor + e + 4) % 4; return; }
        if (k == 'x') { if (modo == M_DESAFIO) des_premio_feito(); else nova_rodada(); return; }
        if (k == KEY_ENTER) {
            if (cursor == 3) {
                if (p->n_col > N_FILA) { removendo = true; cursor = 0; }
                return;
            }
            // O dado ganho fica fora da bolsa ate o proximo embaralho: o
            // premio chega, mas nao na rodada seguinte.
            if (p->n_col < lim_col()) {
                p->col[p->n_col] = oferta[cursor];
                p->onde[p->n_col++] = FORA;
            }
            if (modo == M_DESAFIO) des_premio_feito();
            else nova_rodada();
        }
        return;
    }

    case F_FIM:
        if (k != KEY_ENTER) return;
        if (modo == M_DESAFIO) des_apos_fim();
        else dado_inicia(partidas + 1);
        return;

    default:
        des_tecla(k, e);                     // telas do Modo Desafiante
        return;
    }
}

int dado_vencedor(void) { return fase == F_FIM ? venc_partida : -1; }

// Para o simulador de host.
int  dado_dbg_fase(void)     { return fase; }
int  dado_dbg_vez(void)      { return vez; }
int  dado_dbg_primeiro(void) { return primeiro; }
int  dado_dbg_fichas(int j)  { return J[j].fichas; }
int  dado_dbg_pool(int j)    { return J[j].n_mao; }
int  dado_dbg_tipo(int j, int i) { return J[j].col[J[j].mao[i]].tipo; }
int  dado_dbg_col(int j)     { return J[j].n_col; }
int  dado_dbg_bolsa_n(int j) { return na_bolsa(&J[j]); }
int  dado_dbg_lados(int t)   { return TIPO[t].lados; }
int  dado_dbg_pontos(int j)  { return F[j].pontos; }
int  dado_dbg_total(int j)   { return F[j].total; }
int  dado_dbg_lancados(int j){ return F[j].lancados; }
int  dado_dbg_risco(int j)   { return risco(j); }
int  dado_dbg_rodada(void)   { return rodada; }
int  dado_dbg_pote(void)     { return pote; }
int  dado_dbg_venc_mao(void) { return venc_mao; }
int  dado_dbg_pronta(void)   { return abertura_pronta; }
int  dado_dbg_ver(void)      { return ver_tipo; }
const char *dado_dbg_veredito(void) { return veredito; }
int  dado_dbg_n_ev(void)     { return n_ev; }
int  dado_dbg_sorteia(void)  { return sorteia_tipo(); }
const char *dado_dbg_cat_nome(void) { return cat_sec == 0 ? TIPO[cat_tipo(cat_i)].nome : "amuleto"; }
int  dado_dbg_ev_tipo(void)  { return ev_i < n_ev ? ev[ev_i].tipo : -1; }
void dado_dbg_cpu(int mascara) { cpu = mascara; }
int  dado_dbg_n_tipos(void) { return D_BASICOS; }   // os da partida comum
void dado_dbg_ia_esforco(int e) { ia_esforco = e; }
void dado_dbg_tutorial(int pag) { tut_pag = pag; t_fase = 2.5f; fase = F_TUTORIAL; }
int  dado_dbg_margem(void)
{
    desfecho_t d;
    calcula_desfecho(&d, false);
    return d.total[vez] - d.total[outro(vez)];
}
// Troca a colecao e refaz a compra (para montar cenas de teste).
void dado_dbg_bolsa(int j, const int *tipos, int n)
{
    J[j].n_col = 0;
    for (int i = 0; i < n && i < MAX_COL; i++) {
        J[j].col[J[j].n_col] = nova_peca(tipos[i]);
        J[j].onde[J[j].n_col++] = NA_BOLSA;
    }
    J[j].n_mao = 0;
    compra(&J[j]);
}
