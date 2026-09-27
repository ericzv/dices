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
#include <string.h>
#include <math.h>
#include "esp_timer.h"
#include "../ui/gfx.h"
#include "../hal/keyboard.h"
#include "../hal/som.h"
#include "../hal/display.h"
#include "jogos.h"

#define C_FELTRO     RGB(18, 51, 38)
#define C_FELTRO2    RGB(24, 64, 48)
#define C_FELTRO_ESC RGB(12, 36, 27)
#define C_LATAO      RGB(190, 156, 86)
#define C_LATAO_ESC  RGB(110, 90, 46)
#define C_BARRA      RGB(16, 16, 15)
#define C_MARFIM     RGB(238, 231, 214)
#define C_MARFIM_S   RGB(196, 188, 170)
#define C_EBANO      RGB(34, 30, 29)
#define C_EBANO_B    RGB(64, 57, 54)
#define C_ROSA       RGB(214, 158, 170)
#define C_VINHO      RGB(150, 64, 82)
#define C_VINHO_CLR  RGB(206, 96, 112)
#define C_AGUA       RGB(84, 160, 142)
#define C_VIOLETA    RGB(128, 100, 168)
#define C_ACO        RGB(128, 148, 166)
#define C_OURO       RGB(226, 192, 104)
#define C_TERRA      RGB(184, 116, 76)
#define C_SOMBRA     RGB(8, 26, 19)
#define C_VERDE      RGB(92, 156, 84)        // Moeda Semente
#define C_FOGO       RGB(226, 118, 44)       // Ficha de Fogo: lado 1
#define C_BRASA      RGB(186, 52, 36)        // lado 2
#define C_AMARELO    RGB(238, 200, 84)
#define C_CINZA      RGB(120, 116, 110)      // cinza do que queimou
#define C_LIMA       RGB(150, 176, 64)       // Invejoso: verde de inveja
#define C_ROXO       RGB(96, 64, 128)        // Maldito
#define C_PRATA      RGB(176, 196, 214)      // Espelho
#define C_OSSO       RGB(224, 214, 186)      // chifres do Teimoso
#define C_CORACAO    RGB(214, 64, 84)
#define C_SALMAO     RGB(222, 138, 126)      // faixa de imunidade (pastel)
#define C_TREVO      RGB(52, 150, 90)
#define C_MARFIM_D   RGB(232, 220, 194)      // corpo dos dados de ERIC: off-white
#define C_AZUL       RGB(64, 104, 178)       // ficha da Moeda da sorte: lado 1
#define C_VERMELHO   RGB(178, 58, 58)        // lado 2
#define C_TEXTO_M    RGB(170, 190, 178)
#define C_BRANCO     RGB(255, 252, 240)
#define C_CIANO      RGB(96, 196, 214)       // Copiador
#define C_CHUMBO     RGB(84, 90, 100)        // gota de chumbo do Viciado
#define C_REAL       RGB(186, 70, 160)       // Egoista: purpura real
#define C_MADEIRA    RGB(78, 46, 30)
#define C_MADEIRA_ESC RGB(60, 34, 22)
#define C_MADEIRA_CLR RGB(120, 78, 50)

#define ANTE        5
#define FICHAS_INI  100
#define MAX_RODADAS 15
#define MAX_COL     24             // dados que um jogador pode ter
#define MAO         7              // quantos compra da bolsa por rodada
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
#define R_DADO     22                       // raio de um dado na mesa
#define SLOT_X(k)  (212 + 72 * (k))
#define ROUBO_X(n) (128 - 56 * (n))         // dados roubados, a esquerda da fila
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
       EF_JACKPOT, EF_COPIADOR, EF_EGOISTA };

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
       D_JACKPOT, D_COPIADOR, D_EGOISTA, D_N };

// "Fica" = termina a rodada valendo, sem ter sido anulado.
static const tipo_t TIPO[D_N] = {
    { "moeda",      2,  EF_NADA,      RAR_COMUM,   "1 ou 2; boa para abrir a fila" },
    { "d4",         4,  EF_NADA,      RAR_COMUM,   "quatro lados" },
    { "d6",         6,  EF_NADA,      RAR_COMUM,   "seis lados" },
    { "d8",         8,  EF_NADA,      RAR_COMUM,   "oito lados" },
    { "d12",       12,  EF_NADA,      RAR_INCOMUM, "doze lados" },
    { "d20",       20,  EF_NADA,      RAR_RARO,    "vinte lados" },
    { "Dobro",      6,  EF_DOBRO,     RAR_INCOMUM, "no fim, o seguinte vale x2" },
    { "Viciado",    6,  EF_VICIADO,   RAR_INCOMUM, "rola 2 vezes, fica o maior" },
    { "Lastro",    12,  EF_LASTRO,    RAR_INCOMUM, "nunca menos que 6" },
    { "Par",        4,  EF_PAR,       RAR_INCOMUM, "+8 se igual ao anterior" },
    { "Escudo",     6,  EF_ESCUDO,    RAR_INCOMUM, "nada anula este dado" },
    { "Carrasco",   4,  EF_CARRASCO,  RAR_RARO,    "fica com 4: anula o maior do rival" },
    { "Explosivo",  8,  EF_EXPLODE,   RAR_RARO,    "no 8 rola de novo e soma" },
    { "Espinhoso",  4,  EF_MARTELO,   RAR_RARO,    "se fica: vale e tira o valor do rival" },
    { "Pirata",     8,  EF_PIRATA,    RAR_RARO,    "fica com 6+: leva 4 do rival" },
    { "Espelho",    6,  EF_ESPELHO,   RAR_RARO,    "1 ou 2: vira o maior do rival" },
    { "Fichas",     4,  EF_FICHAS,    RAR_RARO,    "fica e vence: ganha o valor" },
    { "Tudo ou Nada",2,  EF_TUDO_NADA, RAR_LENDA,   "2: x2 em tudo; 1: rodada vale 0" },
    { "Agouro",    20,  EF_MALDITO,   RAR_LENDA,   "4 ou menos zera a fila" },
    { "Gatuno",     6,  EF_GATUNO,    RAR_RARO,    "algum 1 seu: rouba o menor" },
    { "Quebrado",   8,  EF_QUEBRADO,  RAR_COMUM,   "sem sorteio; quebra ao ser jogado" },
    { "Invejoso",   6,  EF_INVEJOSO,  RAR_RARO,    "anula o maior de cada lado" },
    { "Caridoso",   4,  EF_CARIDOSO,  RAR_RARO,    "se fica: rouba o menor rival" },
    { "Fartura",    4,  EF_FARTURA,   RAR_INCOMUM, "no fim: x2 nos dados de 1 e 2" },
    { "Moeda sorte",2,  EF_SORTE,     RAR_INCOMUM, "1: +1 nos ímpares; 2: +1 nos pares" },
    { "Teimoso",    6,  EF_TEIMOSO,   RAR_COMUM,   "imune a queda da fila" },
    { "Semente",    2,  EF_SEMENTE,   RAR_INCOMUM, "dados somando 20+: +10 no fim" },
    { "Ficha Fogo", 2,  EF_FOGO,      RAR_RARO,    "x2 no seu maior, que queima" },
    { "Maldito",    4,  EF_MALDICAO,  RAR_RARO,    "tira do rival o que tirar" },
    { "Trevo",      4,  EF_TREVO,     RAR_INCOMUM, "4: SORTE, vale 8. 1 a 3: vale 0" },
    { "Jackpot",    4,  EF_JACKPOT,   RAR_INCOMUM, "três dados iguais: +10 no fim" },
    { "Copiador",   6,  EF_COPIADOR,  RAR_INCOMUM, "cada dado igual a ele: +2 no fim" },
    { "Egoísta",   12,  EF_EGOISTA,   RAR_RARO,    "nada o anula; anula os seus outros" },
};

// Texto longo de cada dado, para o catalogo. Sem palavras que pedem acento,
// salvo par e impar, que nao tem troca melhor.
static const char *TEXTO_CAT[D_N] = {
    [D_D2]        = "Uma moeda: 1 ou 2. Como primeiro dado, deixa a barra baixa para o resto da fila.",
    [D_D4]        = "Quatro lados. Valor baixo, bom para abrir a fila.",
    [D_D6]        = "Seis lados. O dado comum da casa.",
    [D_D8]        = "Oito lados. Levanta a barra, mas vale mais.",
    [D_D12]       = "Doze lados. Forte no fim da fila, arriscado na frente.",
    [D_D20]       = "Vinte lados. O maior da mesa: melhor no fim da fila.",
    [D_DOBRO]     = "No fim da rodada, o dado que ficar logo depois dele vale x2.",
    [D_VICIADO]   = "Rola duas vezes e fica com o maior resultado.",
    [D_LASTRO]    = "Doze lados, mas nunca vale menos que 6.",
    [D_PAR]       = "Ganha +8 se repetir o valor do dado anterior na fila. Valor igual nunca anula; menor anula.",
    [D_ESCUDO]    = "Nada anula este dado: nem queda, nem ataque do rival.",
    [D_CARRASCO]  = "Se terminar valendo com 4, anula o maior dado do rival.",
    [D_EXPLOSIVO] = "Se tirar 8, rola de novo e soma. Pode explodir 3 vezes seguidas.",
    [D_MARTELO]   = "Coberto de espinhos. Se terminar valendo, soma para você e tira do rival o mesmo valor que tirou.",
    [D_PIRATA]    = "Se terminar valendo com 6 ou mais, tira 4 pontos do rival e fica com eles.",
    [D_ESPELHO]   = "Se tirar 1 ou 2, vira o maior dado do rival. Sem rival na mesa, repete o anterior.",
    [D_FICHAS]    = "Quatro lados. Se terminar valendo e a rodada for sua, ganha fichas iguais ao valor.",
    [D_TUDO_NADA] = "Risco puro. 2: x2 em todos os seus dados no fim. 1: sua rodada vale zero, sem volta.",
    [D_MALDITO]   = "Vinte lados. Se tirar 4 ou menos, zera a fila inteira.",
    [D_TEIMOSO]   = "Seis lados. Uma queda na fila nunca o anula, mas os ataques do rival pegam nele.",
    [D_SEMENTE]   = "Moeda verde. Se no fim os seus dados somarem 20 ou mais, ganha +10.",
    [D_FOGO]      = "Ficha em chamas. No fim, x2 no seu maior dado — que depois vira cinza e sai do jogo.",
    [D_TREVO]     = "Quatro folhas. Tirando 4: SORTE, vale 8. Tirando 1, 2 ou 3, vale zero. A queda da fila compara o que saiu.",
    [D_JACKPOT]   = "Caça-níquel de quatro lados. Se ele terminar valendo e três dos seus dados que valem mostrarem o mesmo número, contando ele ou não: JACKPOT, +10 no fim.",
    [D_COPIADOR]  = "Seis lados. No fim, cada outro dado seu que mostrar o mesmo número que ele ganha +2. Iguais nunca caem na fila.",
    [D_EGOISTA]   = "Doze lados e uma coroa. Nada o anula: nem a queda, nem ataques. Mas ele anula todos os seus outros dados da rodada, antes e depois dele. Só o Escudo resiste.",
    [D_MALDICAO]  = "Quatro lados. O valor dele vai contra o rival: no fim, sai do placar dele. Entra na fila como os outros.",
    [D_GATUNO]    = "Se algum dado seu tirar 1, rouba o menor dado do rival. Basta ter sido jogado.",
    [D_QUEBRADO]  = "Oito lados sem sorteio: vale o número gravado nele. Quebra depois de jogado.",
    [D_INVEJOSO]  = "Anula o maior dado do rival, e junto o seu maior outro dado.",
    [D_CARIDOSO]  = "Se terminar valendo, rouba o menor dado do rival.",
    [D_FARTURA]   = "Quatro lados. No fim da rodada, x2 em todos os seus dados de valor 1 ou 2.",
    [D_SORTE]     = "Ficha azul e vermelha. Azul (1): +1 em cada dado ímpar. Vermelha (2): +1 em cada dado par.",
};

static const char *RARIDADE[] = { "comum", "incomum", "raro", "lenda" };

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
                                   D_JACKPOT, D_COPIADOR };
static int preco_loja(int t)
{
    switch (t) {
    case D_D2:       return 4;
    case D_D4:       return 5;
    case D_QUEBRADO: return 6;
    case D_D8:       return 9;
    case D_PAR:      return 12;
    case D_D12:      return 14;
    case D_ESCUDO:   return 14;
    case D_SORTE:    return 14;
    case D_TEIMOSO:  return 8;
    case D_SEMENTE:  return 12;
    case D_TREVO:    return 10;
    case D_JACKPOT:  return 12;
    case D_COPIADOR: return 12;
    case D_FARTURA:  return 15;
    case D_DOBRO:    return 16;
    case D_VICIADO:  return 16;
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
    case EF_EGOISTA:   return C_REAL;
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
       EV_ZERA, EV_JACKPOT };
typedef struct {
    uint8_t tipo, de_j, de_k, alvo_j, alvo_k;
    int valor;
    uint8_t mascara;                 // dados da fila atingidos (BONUS, MULT)
    int8_t  por_dado[N_FILA];        // bonus ou multiplicador de cada um
    char txt[48];
} evento_t;
#define MAX_EV 16

typedef struct { uint8_t tipo, fixo, de_j; int valor; } roubado_t;

enum { F_ORDEM, F_APOSTA, F_ROLANDO, F_ARRUMA, F_RESULTADO, F_PREMIO, F_FIM,
       F_LOJA, F_ABERTURA, F_JOGA, F_VEREDITO, F_EFEITOS, F_CATALOGO,
       F_ONLINE, F_TUTORIAL };

static jogador_t J[2];
static fila_t F[2];
static voo_t voo[2];
static int  n_voo;
static int  fase, vez, primeiro, rodada, pote, cursor, partidas;
static int  a_pagar, passes;
static bool aumentou;
static float t_fase, t_som;
static int  venc_mao, venc_partida;
static peca_t oferta[3];
static char aviso[64], veredito[64], nota[48];
static int  ver_tipo;
static int  anulou_a, anulou_b;
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
static int  vis_semente[2];
static uint8_t vis_zerada[2];

#define N_LOJA 5
static peca_t loja[N_LOJA];
static bool levou[2][N_LOJA];
static int  loja_primeiro;
static bool abertura_pronta;
static int  menu_cur;                // opcao do menu de abertura
enum { MN_UM, MN_DOIS, MN_ONLINE, MN_DADOS, MN_TUTORIAL, MN_N };

// Modos: contra o computador, dois no mesmo teclado, ou online.
enum { M_UM, M_DOIS, M_ONLINE };
static int  modo;
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
static int  cat_i;                   // dado em exibicao no catalogo
static bool removendo;               // premio trocado por tirar um dado
static float t_tira;                 // animacao do dado saindo (0 = parado)
static int  tira_i;

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
    float vx[8], vy[8];
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
    case D_EGOISTA:  tom = C_REAL;   pct = 30; break;
    case D_VICIADO:  *tinta = C_VERMELHO; break;               // pips de cassino
    default: break;
    }
    if (pct) *corpo = mistura(*corpo, tom, dono == 0 ? pct : pct - 12);
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
    if (tipo == D_PAR) {                          // gemeo pequeno encostado
        float rr = r * 0.55f;
        int n = forma(lados, MX(-r * 0.95f, r * 0.35f), MY(-r * 0.95f, r * 0.35f), rr + 1, ang, vx, vy);
        poligono(vx, vy, n, aro);
        n = forma(lados, MX(-r * 0.95f, r * 0.35f), MY(-r * 0.95f, r * 0.35f), rr - 1, ang, vx, vy);
        poligono(vx, vy, n, corpo);
    }
}

// Na frente: tudo o que se prende ao dado sem cobrir o valor.
// Faixa diagonal no corpo, como a de placa de proibido: "\" ou "/".
static void faixa(float t, bool barra, uint16_t c)
{
    float a = 0.62f * t, w = (0.15f * t + 0.6f) * 0.7071f;
    float s1 = barra ? 1.0f : -1.0f;
    float o[8] = { -a + w, -s1 * a - s1 * w, a + w, s1 * a - s1 * w,
                    a - w, s1 * a + s1 * w, -a - w, -s1 * a + s1 * w };
    marca_poli(o, 4, c);
}

// Espinho: triangulo com base no ponto (bx, by) e ponta na direcao (nx, ny).
static void espinho(float bx, float by, float nx, float ny, float comp, float meia, uint16_t c)
{
    float o[6] = { bx - ny * meia, by + nx * meia, bx + nx * comp, by + ny * comp,
                   bx + ny * meia, by - nx * meia };
    marca_poli(o, 3, c);
}

// Espessura dos tracos das marcas: 1 px num dado pequeno, mais num grande.
static int esp_marca(float t) { return t < 12 ? 1 : (int)(t / 11); }

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
    case D_TEIMOSO:                               // uma faixa: nao anula na queda
        faixa(t, true, C_SALMAO);
        break;
    case D_ESCUDO:                                // duas faixas: nada o anula
        faixa(t, true, C_SALMAO);
        faixa(t, false, C_SALMAO);
        break;
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
    case D_EGOISTA: {                             // coroa de ouro na ponta de cima
        float o[14] = { -0.46f * t, -0.98f * t, 0.46f * t, -0.98f * t, 0.52f * t, -1.58f * t,
                        0.22f * t, -1.26f * t, 0, -1.66f * t, -0.22f * t, -1.26f * t,
                        -0.52f * t, -1.58f * t };
        marca_poli(o, 7, C_OURO);
        marca_ret(0, -1.02f * t, 0.46f * t, 0.07f * t + 0.5f, C_LATAO_ESC);
        int jr = (int)(0.08f * t + 0.8f);
        gfx_disco(MX(0, -1.22f * t), MY(0, -1.22f * t), jr, C_VERMELHO);
        gfx_disco(MX(-0.3f * t, -1.16f * t), MY(-0.3f * t, -1.16f * t), jr > 1 ? jr - 1 : 1, C_CIANO);
        gfx_disco(MX(0.3f * t, -1.16f * t), MY(0.3f * t, -1.16f * t), jr > 1 ? jr - 1 : 1, C_CIANO);
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
    default:
        break;
    }
}

// Numero centrado no dado: grande nos dados grandes.
static void numero(float cx, float cy, float r, const char *s, uint16_t c)
{
    int e = r >= 34 ? 2 : 1;
    gfx_texto((int)cx - gfx_largura(s, e) / 2, (int)cy - 12 * e, s, c, e, true);
}

// X grosso em vinho sobre o dado anulado.
static void risca(float cx, float cy, float r)
{
    int e = r < 12 ? 2 : (int)(r / 6);
    gfx_linha_grossa((int)(cx - r), (int)(cy - r), (int)(cx + r), (int)(cy + r), e, C_VINHO_CLR);
    gfx_linha_grossa((int)(cx - r), (int)(cy + r), (int)(cx + r), (int)(cy - r), e, C_VINHO_CLR);
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

    gfx_disco((int)cx + so, (int)cy + so + 1, (int)r, C_SOMBRA);   // sombra no feltro

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
        // Metade clara, metade escura: tudo ou nada. Caida, mostra so um lado.
        int R = (int)(r + ea), Ri = (int)(r - ea);
        gfx_disco((int)cx, (int)cy, R, aro);
        for (int dy = -Ri; dy <= Ri; dy++) {
            int w = (int)sqrtf((float)(Ri * Ri - dy * dy));
            uint16_t esq = C_MARFIM, dir = C_EBANO;
            if (modo != 0) esq = dir = (valor == 2 ? C_MARFIM : C_EBANO);
            gfx_rect((int)cx - w, (int)cy + dy, w, 1, zerado ? corpo : esq);
            gfx_rect((int)cx, (int)cy + dy, w + 1, 1, zerado ? corpo : dir);
        }
        tinta = (modo != 0 && valor == 2) ? C_VIOLETA : C_MARFIM;
        if (modo == 0) return;
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
            poligono(vx, vy, n, corpo);
        }
    }
    if (!zerado) marca_frente(tipo, r, dono, corpo);

    char s[12];
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
        snprintf(s, sizeof s, "%d", lados);
        numero(cx, cy, r, s, dono == 0 ? C_MARFIM_S : C_EBANO_B);
        return;
    }
    if (valor < 0) return;               // so o corpo: o catalogo escreve grande
    if (lados == 6 && TIPO[tipo].efeito != EF_ESPELHO && valor >= 1 && valor <= 6
        && !zerado) {
        pips((int)cx, (int)cy, valor, (int)r, tinta);
    } else {
        snprintf(s, sizeof s, "%d", valor);
        numero(cx, cy, r, s, tinta);
    }
    if (zerado) risca(cx, cy, r);
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
            v->face = v->tipo == D_QUEBRADO ? v->valor : rola_v(TIPO[v->tipo].lados);
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

static void pinta_mesa(void)
{
    gfx_clear(C_BARRA);
    int x0 = 4, y0 = MESA_Y0 - 8, x1 = GFX_W - 4, y1 = MESA_Y1 + 8;
    gfx_rect(x0, y0, x1 - x0, y1 - y0, C_MADEIRA);
    for (int y = y0; y < y1; y += 2)                    // veios da madeira
        for (int x = x0; x < x1; x++)
            if (ruido_xy(x / 7, y) % 5 == 0) gfx_pixel(x, y, C_MADEIRA_ESC);
    gfx_rect(x0, y0, x1 - x0, 1, C_MADEIRA_CLR);
    gfx_moldura(MESA_X0 - 4, MESA_Y0 - 4, MESA_X1 - MESA_X0 + 8, MESA_Y1 - MESA_Y0 + 8, 2, C_LATAO);
    gfx_moldura(MESA_X0 - 2, MESA_Y0 - 2, MESA_X1 - MESA_X0 + 4, MESA_Y1 - MESA_Y0 + 4, 2, C_SOMBRA);
    float cx = (MESA_X0 + MESA_X1) / 2.0f, cy = (MESA_Y0 + MESA_Y1) / 2.0f;
    float ax = (MESA_X1 - MESA_X0) / 2.0f, ay = (MESA_Y1 - MESA_Y0) / 2.0f;
    for (int y = MESA_Y0; y < MESA_Y1; y++)
        for (int x = MESA_X0; x < MESA_X1; x++) {
            float dx = (x - cx) / ax, dy = (y - cy) / ay;
            int pct = (int)((dx * dx * 0.6f + dy * dy) * 55);
            if (pct > 80) pct = 80;
            uint16_t c = gfx_mistura(C_FELTRO, C_FELTRO_ESC, pct);
            uint32_t h = ruido_xy(x, y) % 11;
            if (h == 0) c = gfx_mistura(c, C_FELTRO2, 70);
            else if (h == 1) c = gfx_mistura(c, C_SOMBRA, 30);
            gfx_pixel(x, y, c);
        }
    // Linha de aposta: uma elipse tracejada ao redor do centro da mesa.
    for (int i = 0; i < 360; i++) {
        if ((i / 4) % 2) continue;
        float a = i * 0.017453f;
        gfx_rect((int)(cx + cosf(a) * 250) , (int)(cy + sinf(a) * 52), 2, 1, C_FELTRO2);
    }
}

static void mesa(void)
{
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
    return F[j].valor[k];
}

// Escudo: nada o anula. Teimoso: so a queda da fila nao o anula.
// Egoista: nada o anula, como o Escudo.
static bool escudo(int j, int k)
{
    int ef = TIPO[tipo_de(j, k)].efeito;
    return ef == EF_ESCUDO || ef == EF_EGOISTA;
}
static bool firme(int j, int k)
{
    int ef = TIPO[tipo_de(j, k)].efeito;
    return ef == EF_ESCUDO || ef == EF_TEIMOSO || ef == EF_EGOISTA;
}
// Dado que resiste ao Egoista: o Escudo e o proprio Egoista.
static bool resiste_egoista(int t) { return TIPO[t].efeito == EF_ESCUDO || TIPO[t].efeito == EF_EGOISTA; }
// Algum Egoista valendo antes do dado k?
static int egoista_antes(int j, int k)
{
    for (int i = 0; i < k; i++)
        if (F[j].est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_EGOISTA) return i;
    return -1;
}
// O Maldito entra na fila, mas o valor dele nao soma para quem o jogou.
static bool pontua(int j, int k) { return TIPO[tipo_de(j, k)].efeito != EF_MALDICAO; }

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
    bool espelhou = false;

    // Espelho caido baixo vira o maior dado do rival (ou o seu ultimo, se o
    // rival ainda nao jogou). Nunca cai, portanto.
    if (ef == EF_ESPELHO && v <= 2) {
        int r = outro(j), alvo = -1;
        for (int i = 0; i < F[r].lancados; i++)
            if (F[r].est[i] == V_VALIDO && F[r].valor[i] > alvo) alvo = F[r].valor[i];
        if (alvo < 0 && l >= 0) alvo = f->valor[l];
        if (alvo > v) {
            v = f->valor[k] = alvo;
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v);
            espelhou = true;
        }
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
        f->pontos = soma(j, f->est);
        return;
    }

    bool condenou = ef == EF_TUDO_NADA && v == 1 && !f->zerada;
    if (ef == EF_TUDO_NADA && v == 1) f->zerada = true;
    if (ef == EF_MALDITO && v <= 4) {
        anula_fila(j);
        f->est[k] = V_ANULADO;
        anulou_a = k;
        snprintf(veredito, sizeof veredito, "%s tirou %d: fila zerada", TIPO[t].nome, v);
        ver_tipo = 3;
        f->pontos = soma(j, f->est);
        return;
    }

    if (l >= 0 && v < f->valor[l]) {
        bool esc_l = firme(j, l), esc_k = firme(j, k);
        if (!esc_l) f->est[l] = V_ANULADO;
        f->est[k] = esc_k ? V_VALIDO : V_ANULADO;
        anulou_a = esc_l ? -1 : l;
        anulou_b = esc_k ? -1 : k;
        resistiu = esc_l ? l : (esc_k ? k : -1);
        if (esc_l)      snprintf(veredito, sizeof veredito, "%d < %d: %s segura", v, f->valor[l], TIPO[tipo_de(j, l)].nome);
        else if (esc_k) snprintf(veredito, sizeof veredito, "%d < %d: %s fica, %d cai", v, f->valor[l], TIPO[t].nome, f->valor[l]);
        else            snprintf(veredito, sizeof veredito, "%d é menor que %d: anulou os dois", v, f->valor[l]);
        ver_tipo = (esc_l || esc_k) ? 2 : 1;
    } else {
        f->est[k] = V_VALIDO;
        if (ef == EF_PAR && l >= 0 && v == f->valor[l]) {
            f->bonus[k] = 8;
            snprintf(f->tag[k], sizeof f->tag[k], "+8");
        }
        if (l >= 0 && TIPO[tipo_de(j, l)].efeito == EF_DOBRO) {
            f->dobra[k] = 1;
            snprintf(f->tag[k], sizeof f->tag[k], "x2");
        }
        int vale = v + f->bonus[k];
        if (ef == EF_TREVO) {
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v == 4 ? 8 : 0);
            if (v == 4) snprintf(veredito, sizeof veredito, "SORTE! o Trevo vale 8");
            else        snprintf(veredito, sizeof veredito, "Trevo tirou %d: vale 0", v);
        } else if (ef == EF_MALDICAO) {
            snprintf(f->tag[k], sizeof f->tag[k], "-%d", v);
            snprintf(veredito, sizeof veredito, "Maldito de pé: -%d no rival", v);
        } else if (ef == EF_COPIADOR) {
            snprintf(veredito, sizeof veredito, "Copiador tirou %d: cada %d seu ganha +2", v, v);
        } else if (copia_antes(j, k)) {
            snprintf(f->tag[k], sizeof f->tag[k], "+2");
            snprintf(veredito, sizeof veredito, "vale %d, igual ao Copiador: +2", vale);
        } else if (espelhou)          snprintf(veredito, sizeof veredito, "Espelho vira %d e vale", v);
        else if (f->dobra[k])  snprintf(veredito, sizeof veredito, "vale %d, x2 no fim", vale);
        else if (f->bonus[k])  snprintf(veredito, sizeof veredito, "vale %d (Par)", vale);
        else if (l < 0 && k)   snprintf(veredito, sizeof veredito, "vale %d: começa fila nova", vale);
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
} desfecho_t;

static int contribui(int j, int k)
{
    if (!pontua(j, k)) return 0;
    return (valor_pontos(j, k) + F[j].bonus[k]) * (F[j].dobra[k] ? 2 : 1);
}

static int alvo_de(const uint8_t *est, int j, bool maior, int exceto)
{
    int a = -1;
    for (int i = 0; i < F[j].lancados; i++) {
        if (i == exceto || est[i] != V_VALIDO || escudo(j, i)) continue;   // so o Escudo barra ataque
        if (a < 0 || (maior ? F[j].valor[i] > F[j].valor[a] : F[j].valor[i] < F[j].valor[a]))
            a = i;
    }
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
        for (int i = 0; i < f->lancados; i++) {
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
    // Ficha de Fogo: x2 no maior dado seu que pontua (sem contar ela), que
    // depois queima. Duas fichas queimam dois dados diferentes.
    int8_t fogo[N_FILA] = { 0 };
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
        fogo[a] += 2;
        evento(gera, EV_FOGO, j, k, j, a, f->valor[a],
               "Fogo: x2 no %d, que vira cinza", f->valor[a]);
    }

    uint8_t mask = 0;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || !pontua(j, k)) continue;
        int m = fogo[k];
        int l = ultimo_valido(f, est, k);
        if (l >= 0 && TIPO[tipo_de(j, l)].efeito == EF_DOBRO) m += 2;
        if (fartura && f->valor[k] <= 2) m += 2 * n_far;
        if (tudo) m += 2;
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
    for (int j = 0; j < 2; j++) memcpy(d->est[j], F[j].est, N_FILA);
    if (gera) n_ev = 0;

    int ordem[2] = { primeiro, outro(primeiro) };
    for (int o = 0; o < 2; o++) {
        int j = ordem[o], r = outro(j);
        bool tem_um = false;
        for (int k = 0; k < F[j].lancados; k++) if (F[j].cru[k] == 1) tem_um = true;

        for (int k = 0; k < F[j].lancados; k++) {
            int ef = TIPO[tipo_de(j, k)].efeito, v = F[j].valor[k];
            if (d->est[j][k] == V_ROUBADO) continue;
            // O Gatuno so precisa ter sido jogado; os outros precisam ficar.
            if (ef == EF_GATUNO ? !tem_um : d->est[j][k] != V_VALIDO) continue;
            const char *nome = TIPO[tipo_de(j, k)].nome;

            if (ef == EF_CARRASCO && v == 4) {
                int a = alvo_de(d->est[r], r, true, -1);
                if (a >= 0) {
                    d->est[r][a] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                           "Carrasco anula o %d de %s", F[r].valor[a], J[r].nome);
                }
            } else if (ef == EF_INVEJOSO) {
                int a = alvo_de(d->est[r], r, true, -1);
                int b = alvo_de(d->est[j], j, true, k);
                if (a >= 0) {
                    d->est[r][a] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                           "Invejoso anula o %d de %s", F[r].valor[a], J[r].nome);
                }
                if (b >= 0) {
                    d->est[j][b] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                           "e anula o %d do próprio %s", F[j].valor[b], J[j].nome);
                }
            } else if (ef == EF_MARTELO) {
                d->delta[r] -= v;
                evento(gera, EV_TIRA, j, k, r, 0, v, "Espinhoso: -%d para %s", v, J[r].nome);
            } else if (ef == EF_MALDICAO) {
                d->delta[r] -= v;
                evento(gera, EV_TIRA, j, k, r, 0, v, "Maldito: -%d para %s", v, J[r].nome);
            } else if (ef == EF_PIRATA && v >= 6) {
                d->delta[r] -= 4; d->delta[j] += 4;
                evento(gera, EV_PIRATA, j, k, r, 0, 4, "Pirata leva 4 de %s", J[r].nome);
            } else if (ef == EF_CARIDOSO || ef == EF_GATUNO) {
                int a = alvo_de(d->est[r], r, false, -1);
                if (a >= 0) {
                    int vale = contribui(r, a);
                    d->est[r][a] = V_ROUBADO;
                    d->roubo_val[j] += vale;
                    evento(gera, EV_ROUBA, j, k, r, a, vale,
                           "%s rouba o %d de %s", nome, vale, J[r].nome);
                }
            }
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
        d->total[j] = (t < 0 || d->zerada[j]) ? 0 : t;
    }
}

// Pontuacao que a mesa mostra durante e depois da animacao dos eventos.
static int total_visivel(int j)
{
    if (vis_zerada[j]) return 0;
    int t = pontos_fila(j, vis_est[j], vis_bonus[j], vis_mult[j]) + vis_delta[j] + vis_semente[j];
    for (int i = 0; i < vis_nroubo[j]; i++) t += vis_roubo[j][i].valor;
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
    if (ef == EF_EGOISTA) return 0;                                   // nunca cai
    int ruins = 0, total = 0;
    if (ef == EF_VICIADO) {
        for (int a = 1; a <= lados; a++)
            for (int b = 1; b <= lados; b++) { total++; if ((a > b ? a : b) < L) ruins++; }
        return ruins * 100 / total;
    }
    for (int a = 1; a <= lados; a++) {
        int v = a;
        total++;
        if (ef == EF_LASTRO && v < 6) v = 6;
        if (ef == EF_EXPLODE && v == lados) continue;
        if (ef == EF_ESPELHO && v <= 2) continue;
        if ((ef == EF_MALDITO && v <= 4) || (ef == EF_TUDO_NADA && v == 1)) { ruins++; continue; }
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
static void compra(jogador_t *p)
{
    p->embaralhou = false;
    while (p->n_mao < MAO) {
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
}

static void nova_rodada(void)
{
    rodada++;
    if (rodada > MAX_RODADAS) {
        encerra(J[0].fichas == J[1].fichas ? 2 : (J[0].fichas > J[1].fichas ? 0 : 1));
        return;
    }
    for (int j = 0; j < 2; j++)
        if (J[j].fichas < ANTE) { encerra(outro(j)); return; }
    for (int j = 0; j < 2; j++) {
        J[j].fichas -= ANTE;
        memset(&F[j], 0, sizeof F[j]);
        descarta_mao(&J[j]);
        compra(&J[j]);
    }
    pote += 2 * ANTE;
    n_voo = 0;
    n_ev = ev_i = 0;
    memset(vis_nroubo, 0, sizeof vis_nroubo);
    memset(vis_delta, 0, sizeof vis_delta);
    memset(vis_bonus, 0, sizeof vis_bonus);
    memset(vis_mult, 0, sizeof vis_mult);
    memset(vis_queima, 0, sizeof vis_queima);
    memset(vis_semente, 0, sizeof vis_semente);
    memset(vis_zerada, 0, sizeof vis_zerada);
    aviso[0] = veredito[0] = nota[0] = 0;
    vez = primeiro;
    cursor = 0;
    t_fase = 0;
    fase = F_ORDEM;
}

// Nomes de cada modo: quem joga aqui e sempre VOCE (ou JOGADOR 1 e 2).
static void poe_nomes(void)
{
    switch (modo) {
    case M_UM:     J[0].nome = "VOCÊ";      J[1].nome = "COMPUTADOR"; break;
    case M_DOIS:   J[0].nome = "JOGADOR 1"; J[1].nome = "JOGADOR 2";  break;
    default:       J[baixo].nome = "VOCÊ";  J[outro(baixo)].nome = "RIVAL"; break;
    }
}

static void abre_loja(void);

void dado_inicia(int n_partidas)
{
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

static void lanca_proximo(void)
{
    fila_t *f = &F[vez];
    int k = f->lancados;
    int t = tipo_de(vez, k), lados = TIPO[t].lados;

    int x = rola(lados), menor = 0;
    switch (TIPO[t].efeito) {
    case EF_VICIADO: {                       // dois lances; fica o maior
        int y = rola(lados);
        menor = x < y ? x : y;
        if (y > x) x = y;
        break;
    }
    case EF_LASTRO:  if (x < 6) x = 6; break;
    case EF_EXPLODE: {
        int n = x, g = 0;
        while (n == lados && g++ < 3) { n = rola(lados); x += n; }
        break;
    }
    case EF_QUEBRADO: x = f->fixo[k]; break;        // sem sorteio
    default: break;
    }
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
    v->face = t == D_QUEBRADO ? x : rola_v(lados);
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
    int n_quebra = 0, n_cinza = 0;
    for (int j = 0; j < 2; j++) {
        int saem[N_FILA], n = 0;
        for (int k = 0; k < F[j].lancados; k++) {
            bool some = false;
            if (F[j].tipo[k] == D_QUEBRADO) { F[j].quebra[k] = true; some = true; n_quebra++; }
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
}

// Depois da animacao dos eventos: paga o pote e declara quem levou.
static void conclui_rodada(void)
{
    for (int j = 0; j < 2; j++) {
        memcpy(F[j].est, vis_est[j], N_FILA);
        F[j].total = total_visivel(j);
    }
    if (F[0].total == F[1].total) {
        venc_mao = 2;
        snprintf(aviso, sizeof aviso, "empate em %d: o pote fica na mesa", F[0].total);
    } else {
        int w = F[0].total > F[1].total ? 0 : 1;
        venc_mao = w;
        J[w].fichas += pote;
        snprintf(aviso, sizeof aviso, "%s leva %d fichas", J[w].nome, pote);
        pote = 0;
        for (int k = 0; k < F[w].lancados; k++)
            if (F[w].est[k] == V_VALIDO && TIPO[tipo_de(w, k)].efeito == EF_FICHAS)
                J[w].fichas += F[w].valor[k];
        som_toca(SOM_VITORIA);
    }
    recolhe_quebrados();
    t_fase = 0;
    fase = F_RESULTADO;
}

// O Jackpot tem som proprio; os outros poderes, o brilho de sempre.
static int som_do_evento(int i) { return ev[i].tipo == EV_JACKPOT ? SOM_JACKPOT : SOM_EFEITO; }

static void desfecho(void)
{
    desfecho_t d;
    calcula_desfecho(&d, true);
    for (int j = 0; j < 2; j++) {
        memcpy(vis_est[j], F[j].est, N_FILA);
        vis_nroubo[j] = 0;
        vis_delta[j] = 0;
        vis_semente[j] = 0;
        vis_zerada[j] = 0;
        memset(vis_bonus[j], 0, N_FILA);
        memset(vis_mult[j], 0, N_FILA);
        memset(vis_queima[j], 0, N_FILA);
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
    case EV_TIRA:   vis_delta[e->alvo_j] -= e->valor; break;
    case EV_PIRATA: vis_delta[e->alvo_j] -= e->valor; vis_delta[e->de_j] += e->valor; break;
    case EV_BONUS:
        for (int k = 0; k < N_FILA; k++) vis_bonus[e->de_j][k] += e->por_dado[k];
        break;
    case EV_MULT:
        for (int k = 0; k < N_FILA; k++) vis_mult[e->de_j][k] = e->por_dado[k];
        break;
    case EV_FOGO:    vis_queima[e->alvo_j][e->alvo_k] = 1; break;
    case EV_SEMENTE: case EV_JACKPOT: vis_semente[e->de_j] += e->valor; break;
    case EV_ZERA:    vis_zerada[e->de_j] = 1; break;
    }
}

static void termina_turno(void)
{
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

static int opcoes(int *op)
{
    int n = 0;
    if (!a_pagar) { op[n++] = OP_PASSAR; op[n++] = OP_APOSTAR10; op[n++] = OP_APOSTAR25; }
    else {
        op[n++] = OP_PAGAR;
        if (!aumentou && J[vez].fichas > a_pagar && J[outro(vez)].fichas > 0)
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
    case OP_AUMENTAR:  snprintf(s, n, "dobrar"); break;
    default:           snprintf(s, n, "correr"); break;
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
        int sobe = a_pagar;
        if (sobe > J[outro(vez)].fichas) sobe = J[outro(vez)].fichas;
        int b = a_pagar + sobe;
        if (b > p->fichas) b = p->fichas;
        p->fichas -= b; pote += b;
        a_pagar = sobe;
        aumentou = true;
        break;
    }
    case OP_CORRER: {
        int w = outro(vez);
        J[w].fichas += pote;
        snprintf(aviso, sizeof aviso, "%s correu. %s leva %d", p->nome, J[w].nome, pote);
        som_toca(SOM_CORREU);
        pote = 0;
        venc_mao = -1;
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
    gfx_rect(GFX_W / 2 - 150, 134, 300, 2, C_LATAO_ESC);
    txt_c(GFX_W / 2, 140, "Criado por ERICZ", C_TEXTO_M, false);

    static const char *OP[MN_N] = { "1 jogador", "2 jogadores", "Online",
                                    "Dados da casa", "Como jogar" };
    for (int i = 0; i < MN_N; i++) {
        int yb = 170 + i * 26;
        bool cur = i == menu_cur;
        gfx_rect(GFX_W / 2 - 110 + 2, yb + 2, 220, 22, C_SOMBRA);
        gfx_rect(GFX_W / 2 - 110, yb, 220, 22, cur ? C_LATAO : C_FELTRO_ESC);
        gfx_moldura(GFX_W / 2 - 110, yb, 220, 22, 1, cur ? C_BRANCO : C_FELTRO2);
        txt_c(GFX_W / 2, yb + 1, OP[i], cur ? C_BARRA : C_MARFIM, cur);
    }
    txt_c(GFX_W / 2, GFX_H - FAIXA_H + 12,
          "setas escolhem  ·  ENTER confirma  ·  M som  ·  N música  ·  F11 tela cheia",
          C_TEXTO_M, false);
}

static void abre_premio(void)
{
    for (int i = 0; i < 3; i++) {
        bool rep;
        do {
            oferta[i] = nova_peca(sorteia_tipo());
            rep = false;
            for (int k = 0; k < i; k++) if (oferta[k].tipo == oferta[i].tipo) rep = true;
        } while (rep);
    }
    vez = venc_mao;
    cursor = 0;
    removendo = false;
    t_tira = 0;
    fase = F_PREMIO;
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
    case EF_LASTRO:   return 7.75f;
    case EF_VICIADO:  return 4.47f;
    case EF_EXPLODE:  return 5.1f;
    default:          return (t->lados + 1) / 2.0f;
    }
}

// Dados que so atrapalham a fila de quem nao sabe o que esta fazendo.
static bool arriscado(int tipo) { return tipo == D_MALDITO || tipo == D_TUDO_NADA || tipo == D_EGOISTA; }

// Fila: os quatro de maior media, do menor para o maior, que assim cada um
// tem boa chance de passar a barra do anterior.
static int ia_proximo_da_fila(void)
{
    jogador_t *p = &J[vez];
    fila_t *f = &F[vez];
    int quer = p->n_mao < N_FILA ? p->n_mao : N_FILA;
    int ok = 0;
    for (int i = 0; i < p->n_mao; i++) if (!arriscado(p->col[p->mao[i]].tipo)) ok++;
    bool sem_risco = ok >= quer;
    // Os escolhidos: os 'quer' de maior media.
    bool esc[MAO] = { false };
    for (int n = 0; n < quer; n++) {
        int b = -1;
        for (int i = 0; i < p->n_mao; i++) {
            peca_t pc = p->col[p->mao[i]];
            if (esc[i] || (sem_risco && arriscado(pc.tipo))) continue;
            if (b < 0 || media(pc) > media(p->col[p->mao[b]])) b = i;
        }
        if (b >= 0) esc[b] = true;
    }
    // Entre os escolhidos ainda fora da fila, o de menor media vai agora.
    int b = -1;
    for (int i = 0; i < p->n_mao; i++) {
        if (!esc[i]) continue;
        bool ja = false;
        for (int k = 0; k < f->n; k++) if (f->idx[k] == p->mao[i]) ja = true;
        if (ja) continue;
        if (b < 0 || media(p->col[p->mao[i]]) < media(p->col[p->mao[b]])) b = i;
    }
    return b;
}

// Lancar mais um? Quem joga depois sabe o alvo; quem abre olha o risco.
static bool ia_continua(void)
{
    fila_t *f = &F[vez];
    if (f->lancados >= f->n) return false;
    if (f->lancados == 0) return true;
    if (vez != primeiro) {                     // parar agora ganha?
        desfecho_t d;
        calcula_desfecho(&d, false);
        return d.total[vez] <= d.total[outro(vez)];
    }
    // O Egoista trocaria a fila inteira por um d12: so compensa com pouco na mesa.
    if (tipo_de(vez, f->lancados) == D_EGOISTA) return f->pontos < 6;
    int r = risco(vez);
    return r < 40 || (r < 60 && f->pontos < 8);
}

static int ia_aposta(void)
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

// Loja: leva o melhor que couber, guardando fichas para as apostas.
static int ia_compra(void)
{
    int b = -1;
    for (int i = 0; i < N_LOJA; i++) {
        int pr = preco_loja(loja[i].tipo);
        if (levou[vez][i] || J[vez].fichas - pr < 75 || J[vez].n_col >= MAX_COL) continue;
        if (b < 0 || TIPO[loja[i].tipo].raridade > TIPO[loja[b].tipo].raridade
            || (TIPO[loja[i].tipo].raridade == TIPO[loja[b].tipo].raridade
                && pr > preco_loja(loja[b].tipo)))
            b = i;
    }
    return b;
}

static void ia_passo(float dt)
{
    if (!ia_na_vez() || (fase == F_PREMIO && t_tira > 0)) { ia_fase = -1; return; }
    if (fase != ia_fase || vez != ia_vez) {   // nova decisao: pensa um pouco
        ia_fase = fase; ia_vez = vez;
        t_ia = fase == F_ORDEM ? -0.4f : 0;
    }
    t_ia += dt;
    if (t_ia < 0.55f) return;
    t_ia = 0;

    switch (fase) {
    case F_LOJA: {
        int i = ia_compra();
        if (i >= 0) { cursor = i; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    }
    case F_ORDEM: {
        int i = ia_proximo_da_fila();
        if (i >= 0) { cursor = i; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    }
    case F_JOGA:
        if (ia_continua()) { cursor = 0; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    case F_APOSTA:
        cursor = ia_aposta();
        trata_tecla(KEY_ENTER);
        break;
    case F_PREMIO: {
        int b = 0;
        for (int i = 1; i < 3; i++)
            if (TIPO[oferta[i].tipo].raridade > TIPO[oferta[b].tipo].raridade) b = i;
        cursor = b;
        trata_tecla(KEY_ENTER);
        break;
    }
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
bool dado_digitando(void) { return fase == F_ONLINE && digitando; }

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

static void ao_menu_sem_partida(void)
{
    t_fase = 1.0f;
    fase = F_ABERTURA;
}

// ESC: nas telas do menu, volta; no meio da partida, pede um segundo ESC.
static void trata_esc(void)
{
    switch (fase) {
    case F_ABERTURA: return;
    case F_CATALOGO: case F_TUTORIAL: ao_menu_sem_partida(); return;
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
void dado_passo(float dt)
{
    t_fase += dt;
    ia_passo(dt);
    rede_passo();
    if (t_sair >= 0 && (t_sair += dt) > 2.5f) t_sair = -1;
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
            static const float ALVO[2][2] = { { 120, 250 }, { 520, 250 } };
            float a = dt * 5 > 1 ? 1 : dt * 5;
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
                nova_rodada();
            }
        }
        break;
    case F_ROLANDO: {
        // Camera lenta: o dado rola devagar, e o quarto mais devagar ainda -
        // e nele que a rodada se decide.
        float lento = voo[0].slot == N_FILA - 1 ? 0.42f : 0.62f;
        if (fisica(dt * lento) && t_fase > 0.5f) {
            voo[0].ox = voo[0].x; voo[0].oy = voo[0].y;
            t_fase = 0;
            fase = F_ARRUMA;
        }
        break;
    }
    case F_ARRUMA: {
        float t = t_fase / 0.45f;
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
            aplica_regra(j, k);
            som_toca(ver_tipo == 0 ? SOM_VALIDO : SOM_ANULA);
            t_fase = 0;
            fase = F_VEREDITO;
        }
        break;
    }
    case F_VEREDITO:
        if (t_fase > (ver_tipo == 0 ? 0.6f : 1.3f)) {
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
        desenha_dado(v->x, v->y, r, v->tipo, v->dono, v->giro,
                     v->parado ? v->valor : v->face, 2, descartado);
    }
}

static void peca_parada(float x, float y, float r, peca_t p, int dono)
{
    desenha_dado(x, y, r, p.tipo, dono, 0, p.fixo, 0, false);
}

// Moldura de destaque em volta de um dado da fila.
static void aro(int x, int y, uint16_t c)
{
    int m = R_DADO + 7;
    gfx_moldura(x - m, y - m, 2 * m, 2 * m, 3, c);
}

// Etiqueta no canto do dado (x2, +8, =9), com fundo para ler sobre o feltro.
static void etiqueta(int x, int y, const char *s)
{
    int w = gfx_largura(s, 1) + 8;
    int ex = x + R_DADO - 8, ey = y - R_DADO - 14;
    gfx_rect(ex, ey, w, TXT_H + 2, C_BARRA);
    gfx_moldura(ex, ey, w, TXT_H + 2, 1, s[0] == '-' ? C_VINHO_CLR : C_OURO);
    txt(ex + 4, ey + 1, s, s[0] == '-' ? C_VINHO_CLR : C_OURO, true);
}

// Painel escuro e translucido atras de um bloco de texto no meio da mesa.
static void painel(int y, int h, int w)
{
    gfx_rect_alfa(GFX_W / 2 - w / 2, y, w, h, C_SOMBRA, 170);
}

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
    int x = 18 + txt_nome(18, ty, j, da_vez);
    ficha(x + 22, y0 + FAIXA_H / 2, j == 0 ? C_LATAO : C_ROSA);
    char s[32];
    snprintf(s, sizeof s, "%d", p->fichas);
    txt(x + 36, ty, s, C_MARFIM, true);

    // A rodada mora na faixa de cima; o som, na de baixo.
    if (cima && rodada > 0 && fase != F_LOJA) {
        snprintf(s, sizeof s, "rodada %d de %d", rodada > MAX_RODADAS ? MAX_RODADAS : rodada,
                 MAX_RODADAS);
        txt_c(GFX_W / 2, ty, s, C_TEXTO_M, false);
    }
    if (!cima && som_ligado()) icone_som(GFX_W / 2 - 8, y0 + FAIXA_H / 2 - 7);

    // A bolsa: quantos ainda estao nela, de quantos o jogador tem.
    int bx = GFX_W - 118, by = y0 + FAIXA_H / 2;
    gfx_rect(bx, by - 5, 16, 13, C_LATAO_ESC);              // saquinho
    gfx_rect(bx + 4, by - 9, 8, 4, C_LATAO_ESC);
    gfx_rect(bx + 2, by - 3, 12, 9, C_LATAO);
    snprintf(s, sizeof s, "bolsa %d/%d", na_bolsa(p), p->n_col);
    txt(bx + 22, ty, s, C_MARFIM_S, false);
}

// Estilhacos do Quebrado: o dado se abre em lascas que se afastam e somem.
static void estilhaca(int x, int y, float t, int dono)
{
    static const int8_t DIR[9][2] = { {-3,-4},{3,-4},{5,0},{3,4},{-3,4},{-5,0},{0,-5},{2,5},{-4,2} };
    uint16_t c = t < 0.5f ? (dono == 0 ? C_MARFIM : C_EBANO_B) : C_TERRA;
    for (int i = 0; i < 9; i++) {
        int px = x + (int)(DIR[i][0] * t * 16), py = y + (int)(DIR[i][1] * t * 16);
        int tam = t < 0.6f ? 6 : 4;
        gfx_rect(px - tam / 2, py - tam / 2, tam, tam, c);
    }
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

    for (int k = 0; k < f->n; k++) {
        int x = SLOT_X(k);
        bool voando = n_voo && (fase == F_ROLANDO || fase == F_ARRUMA)
                      && voo[0].dono == j && voo[0].slot == k;
        if (voando) continue;
        int t = tipo_de(j, k);
        if (k >= f->lancados) {
            gfx_disco(x, y, R - 2, C_FELTRO_ESC);                    // lugar marcado
            desenha_dado((float)x, (float)y, R - 6, t, j, 0, f->fixo[k], 0, false);
            continue;
        }
        uint8_t est = no_desfecho ? vis_est[j][k] : f->est[k];

        // Dado sendo roubado agora: sai do lugar e atravessa a mesa.
        if (e && e->tipo == EV_ROUBA && e->alvo_j == j && e->alvo_k == k && p > 0.25f) {
            float q = (p - 0.25f) / 0.6f;
            if (q > 1) q = 1;
            float s = q * q * (3 - 2 * q);
            int tx = ROUBO_X(vis_nroubo[e->de_j]), ty = zona(e->de_j);
            desenha_dado(x + (tx - x) * s, y + (ty - y) * s, R, t, j, s * 6.28f, f->valor[k], 2, false);
            continue;
        }
        if (est == V_ROUBADO) {                           // lugar vazio: foi levado
            gfx_disco(x, y, R - 2, C_FELTRO_ESC);
            continue;
        }
        if (fase == F_RESULTADO && f->quebra[k] && t_fase > 0.35f) {
            if (t_fase < 1.3f) estilhaca(x, y, (t_fase - 0.35f) / 0.95f, j);
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
        if (recente && pisca) continue;
        // Alvo de anulacao agora: pisca antes de cair.
        bool alvo = e && e->tipo == EV_ANULA && e->alvo_j == j && e->alvo_k == k;
        if (alvo && p > 0.4f && ((int)(t_fase * 8) % 2 == 0)) continue;
        // O dado que acabou de valer da um pulinho.
        float pulo = 0;
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && ver_tipo == 0 && t_fase < 0.25f)
            pulo = sinf(t_fase / 0.25f * 3.1416f) * 8;
        desenha_dado((float)x, y - pulo, R, t, j, 0, f->valor[k], 2, anul);
        // Etiqueta: no desfecho, o multiplicador somado (x4) ou o bonus da
        // Moeda (+2); antes, o aviso do que vem (x2 do Dobro, +8, =9).
        char tg[16] = "";
        if (no_desfecho && vis_mult[j][k] > 1) snprintf(tg, sizeof tg, "x%d", vis_mult[j][k]);
        else if (no_desfecho && vis_bonus[j][k]) snprintf(tg, sizeof tg, "+%d", vis_bonus[j][k]);
        else if (f->tag[k][0]) snprintf(tg, sizeof tg, "%s", f->tag[k]);
        // Egoista chegando: uma onda purpura se abre dele e derruba os outros.
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && t == D_EGOISTA
            && anulou_masc && t_fase < 0.9f) {
            for (int o = 0; o < 2; o++) {
                int rr = (int)(t_fase * 260) - o * 26;
                if (rr <= R_DADO) continue;
                for (int a = 0; a < 48; a++) {
                    float an = a * 0.1309f;
                    gfx_rect(x + (int)(cosf(an) * rr), y + (int)(sinf(an) * rr * 0.45f), 3, 3, C_REAL);
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
            && (e->mascara >> k & 1) && p > 0.2f)
            aro(x, y, C_OURO);
        if (e && e->tipo == EV_BONUS && e->de_j == j && e->de_k == k) aro(x, y, C_OURO);
        // Marcado pelo fogo: aro em brasa ate o fim da rodada.
        if (no_desfecho && (vis_queima[j][k] ||
            (e && e->tipo == EV_FOGO && e->alvo_j == j && e->alvo_k == k && p > 0.35f)))
            aro(x, y, ((int)(t_fase * 10) % 2) ? C_FOGO : C_BRASA);
        if (e && e->tipo == EV_ZERA && e->de_j == j && p > 0.15f && ((int)(t_fase * 9) % 2 == 0))
            aro(x, y, C_VIOLETA);
        if (e && (e->tipo == EV_FOGO || e->tipo == EV_SEMENTE) && e->de_j == j && e->de_k == k)
            aro(x, y, e->tipo == EV_FOGO ? C_FOGO : C_VERDE);
        if (tg[0] && !anul) etiqueta(x, y, tg);        // por cima dos aros
    }
    // O par que se anulou fica unido por um traco.
    if (fase == F_VEREDITO && j == vez && anulou_a >= 0 && anulou_b >= 0)
        gfx_rect(SLOT_X(anulou_a) + R + 4, y - 2, SLOT_X(anulou_b) - SLOT_X(anulou_a) - 2 * R - 8, 5,
                 C_VINHO_CLR);

    // Dados roubados por este jogador, a esquerda da fila.
    if (no_desfecho)
        for (int i = 0; i < vis_nroubo[j]; i++) {
            roubado_t *r = &vis_roubo[j][i];
            desenha_dado((float)ROUBO_X(i), (float)y, R, r->tipo, r->de_j, 0, r->valor, 2, false);
        }

    if (f->lancados || vis_nroubo[j]) {
        char s[16];
        int pts = no_desfecho ? total_visivel(j) : f->pontos;
        // Rodada zerando: o placar desce ate 0 durante o evento.
        if (e && e->tipo == EV_ZERA && e->de_j == j) {
            float q = p / 0.7f;
            pts = (int)(pts * (q > 1 ? 0 : 1 - q));
        }
        snprintf(s, sizeof s, "%d", pts);
        uint16_t c = (fase == F_RESULTADO && venc_mao == j) ? J[j].cor
                   : f->zerada ? C_VIOLETA
                   : (j == vez && !no_desfecho ? C_MARFIM : C_TEXTO_M);
        gfx_rect_alfa(PLACAR_X - 34, y - 26, 68, 50, C_SOMBRA, 140);
        txt_c(PLACAR_X, y - 42 + (j == baixo ? 72 : 0), "pontos", C_TEXTO_M, false);
        txt_c2(PLACAR_X, y - 20, s, c);
    }
}

// O que o evento em curso desenha por cima da mesa: o traco do ataque, os
// pontos que saem de um placar e entram no outro.
static void desenha_evento(void)
{
    if (fase != F_EFEITOS || ev_i >= n_ev) return;
    const evento_t *e = &ev[ev_i];
    float p = t_fase / duracao_evento(ev_i);
    int ax = SLOT_X(e->de_k), ay = zona(e->de_j);
    if (e->tipo == EV_ANULA && p < 0.6f) {
        float q = p / 0.4f;
        if (q > 1) q = 1;
        int bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        int ex = ax + (int)((bx - ax) * q), ey = ay + (int)((by - ay) * q);
        gfx_linha_grossa(ax, ay, ex, ey, 4, C_VINHO_CLR);
    }
    // Maldito e Espinhoso: um traco do dado ate o placar do rival.
    if (e->tipo == EV_TIRA && p < 0.5f) {
        float q = p / 0.35f;
        if (q > 1) q = 1;
        bool praga = TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_MALDICAO;
        int bx = PLACAR_X, by = zona(e->alvo_j);
        int ex = ax + (int)((bx - ax) * q), ey = ay + (int)((by - ay) * q);
        gfx_linha_grossa(ax, ay, ex, ey, 4, praga ? C_VIOLETA : C_VINHO_CLR);
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
static void dado_e_poder(int y, int tipo)
{
    txt_c(GFX_W / 2, y, TIPO[tipo].nome, cor_aro(tipo), true);
    txt_c(GFX_W / 2, y + TXT_H + 2, TIPO[tipo].desc, C_MARFIM_S, false);
}

static void botoes(int y, const char **rot, const bool *ativo, int n, uint16_t cor)
{
    int larg = -12;
    for (int i = 0; i < n; i++) larg += gfx_largura(rot[i], 1) + 24 + 12;
    int x = GFX_W / 2 - larg / 2;
    for (int i = 0; i < n; i++) {
        int w = gfx_largura(rot[i], 1) + 24;
        bool cur = i == cursor && ativo[i];
        gfx_rect(x + 2, y + 3, w, 28, C_SOMBRA);
        gfx_rect(x, y, w, 28, cur ? cor : C_FELTRO_ESC);
        gfx_moldura(x, y, w, 28, 1, cur ? C_BRANCO : C_FELTRO2);
        txt(x + 12, y + 4, rot[i], cur ? C_BARRA : (ativo[i] ? C_MARFIM : C_FELTRO2), cur);
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
        int x = GFX_W / 2 + (int)((i - (n - 1) / 2.0f) * 62);
        int ordem = -1;
        for (int k = 0; k < f->n; k++) if (f->idx[k] == p->mao[i]) ordem = k;
        bool cur = i == cursor && q >= 1;
        if (cur) gfx_rect_alfa(x - 29, y - 29, 58, 58, p->cor, 60);
        peca_parada((float)x, (float)(y + dy - (cur ? 3 : 0)), 20, p->col[p->mao[i]], vez);
        if (ordem >= 0) {
            char o[2] = { (char)('1' + ordem), 0 };
            gfx_disco(x + 18, y - 20, 10, C_OURO);
            gfx_disco(x + 18, y - 20, 8, C_AMARELO);
            txt_c(x + 18, y - 31, o, C_BARRA, true);
        }
        if (cur) gfx_rect(x - 18, y + 27, 36, 3, p->cor);
    }

    char t[64];
    if (vez == primeiro) snprintf(t, sizeof t, "%s monta a fila", p->nome);
    else snprintf(t, sizeof t, "%s monta a fila  ·  alvo %d", p->nome, F[outro(vez)].pontos);
    int y1 = CENTRO - 44;
    painel(y1 - 6, 100, 470);
    txt_c(GFX_W / 2, y1, t, p->cor, true);
    if (f->n == N_FILA) {
        txt_c(GFX_W / 2, y1 + 30, "fila cheia: TAB ou CTRL confirma", C_OURO, true);
    } else if (n) {
        dado_e_poder(y1 + 26, p->col[p->mao[cursor]].tipo);
    }
    // Rodape: o que a compra fez com a bolsa.
    if (p->embaralhou) snprintf(t, sizeof t, "bolsa vazia: tudo volta para ela e embaralha");
    else if (f->n)     snprintf(t, sizeof t, "ENTER põe/tira  ·  TAB/CTRL confirma com %d na fila", f->n);
    else               snprintf(t, sizeof t, "comprou %d, sobram %d na bolsa  ·  ENTER põe na fila", n, na_bolsa(p));
    txt_c(GFX_W / 2, y1 + 70, t, p->embaralhou ? C_TERRA : C_TEXTO_M, false);
}

static void desenha_joga(void)
{
    jogador_t *p = &J[vez];
    fila_t *f = &F[vez];
    char t[64];
    snprintf(t, sizeof t, "%s joga", p->nome);
    int y1 = CENTRO - 48;
    if (fase != F_ROLANDO && fase != F_ARRUMA) painel(y1 - 4, fase == F_JOGA ? 104 : 60, 470);
    txt_c(GFX_W / 2, y1, t, p->cor, true);

    if (fase == F_VEREDITO) {
        uint16_t c = ver_tipo == 0 ? C_OURO : ver_tipo == 2 ? C_ACO
                   : ver_tipo == 4 ? C_REAL : C_VINHO_CLR;
        txt_sombra_c(GFX_W / 2, y1 + 28, veredito, c, 1);
    } else if (fase == F_JOGA && f->lancados < f->n) {
        dado_e_poder(y1 + 22, tipo_de(vez, f->lancados));   // o proximo dado da fila
    }

    if (fase == F_JOGA) {
        // Para quem joga em segundo, o botao ja diz o que parar significa.
        char parar[40] = "parar";
        if (vez != primeiro && f->lancados) {
            desfecho_t d;
            calcula_desfecho(&d, false);
            int dif = d.total[vez] - d.total[outro(vez)];
            if (dif > 0)      snprintf(parar, sizeof parar, "parar: vence por %d", dif);
            else if (dif < 0) snprintf(parar, sizeof parar, "parar: perde por %d", -dif);
            else              snprintf(parar, sizeof parar, "parar: empata");
        }
        const char *rot[2] = { "jogar", parar };
        bool ativo[2] = { f->lancados < f->n, f->lancados > 0 };
        botoes(y1 + 70, rot, ativo, 2, p->cor);
    }
    pote_mesa();
}

static void desenha_aposta(void)
{
    char t[64];
    int y1 = CENTRO - 44;
    painel(y1 - 6, 96, 520);
    snprintf(t, sizeof t, "%s fez %d pontos", J[primeiro].nome, F[primeiro].pontos);
    txt_c(GFX_W / 2, y1, t, J[primeiro].cor, true);
    if (a_pagar) snprintf(t, sizeof t, "%s: paga %d para ver?", J[vez].nome, a_pagar);
    else         snprintf(t, sizeof t, "%s: aposta?", J[vez].nome);
    txt_c(GFX_W / 2, y1 + 26, t, J[vez].cor, false);

    int op[4], n = opcoes(op);
    char r[4][24];
    const char *rot[4];
    bool ativo[4];
    for (int i = 0; i < n; i++) { rotulo_op(op[i], r[i], sizeof r[i]); rot[i] = r[i]; ativo[i] = true; }
    botoes(y1 + 60, rot, ativo, n, J[vez].cor);
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
               : (e->tipo == EV_ROUBA || e->tipo == EV_BONUS || e->tipo == EV_MULT) ? C_OURO
               : C_VINHO_CLR;
    // Fundo atras do texto: o dado roubado passa por baixo, sem embaralhar.
    int w = gfx_largura(e->txt, 1) + 40;
    if (w < 240) w = 240;
    painel(CENTRO - 30, 58, w);
    txt_c(GFX_W / 2, CENTRO - 24, J[e->de_j].nome, J[e->de_j].cor, true);
    txt_c(GFX_W / 2, CENTRO + 2, e->txt, c, true);
    desenha_evento();
}

static void desenha_resultado(void)
{
    uint16_t c = venc_mao == 0 ? C_LATAO : (venc_mao == 1 ? C_ROSA : C_MARFIM);
    painel(CENTRO - 40, 86, 420);
    txt_sombra_c(GFX_W / 2, CENTRO - 32, aviso, c, 1);
    if (nota[0]) txt_c(GFX_W / 2, CENTRO - 6, nota, C_TERRA, false);   // cinza ou cacos
    txt_c(GFX_W / 2, CENTRO + 20, "ENTER segue", C_TEXTO_M, false);
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
            if (i == cursor) gfx_rect_alfa(x - 22, y - 22, 44, 44, C_VINHO_CLR, 80);
            peca_parada((float)x, (float)y, 17, p->col[i], vez);
            if (i == cursor) gfx_rect(x - 16, y + 23, 32, 3, C_VINHO_CLR);
        }
        int tc = p->col[cursor < p->n_col ? cursor : 0].tipo;
        txt_c(GFX_W / 2, 236, TIPO[tc].nome, cor_aro(tc), true);
        txt_c(GFX_W / 2, 258, t_tira > 0 ? "some para sempre" : TIPO[tc].desc,
              t_tira > 0 ? C_VINHO_CLR : C_MARFIM_S, false);
        return;
    }

    snprintf(t, sizeof t, "%s levou a rodada e escolhe um prêmio", p->nome);
    txt_c(GFX_W / 2, MESA_Y0 + 14, t, p->cor, true);
    for (int i = 0; i < 3; i++) {
        int x = GFX_W / 2 + (i - 1) * 150;
        int t2 = oferta[i].tipo;
        bool cur = i == cursor;
        gfx_rect(x - 64, 90, 128, 110, cur ? C_FELTRO_ESC : C_FELTRO);
        gfx_moldura(x - 64, 90, 128, 110, cur ? 3 : 1, cur ? cor_aro(t2) : C_FELTRO2);
        float bob = cur ? sinf(t_fase * 4) * 3 : 0;
        peca_parada((float)x, 134 + bob, 28, oferta[i], vez);
        txt_c(x, 172, TIPO[t2].nome, cur ? cor_aro(t2) : C_MARFIM_S, cur);
    }
    bool pode = p->n_col > N_FILA;
    if (cursor < 3) {
        char r[96];
        snprintf(r, sizeof r, "%s  ·  %s", RARIDADE[TIPO[oferta[cursor].tipo].raridade],
                 TIPO[oferta[cursor].tipo].desc);
        txt_c(GFX_W / 2, 212, r, C_MARFIM, false);
    } else {
        txt_c(GFX_W / 2, 212, pode ? "troca o prêmio por tirar um dos seus, para sempre"
                                   : "com 4 dados, nenhum pode sair", C_MARFIM, false);
    }

    // Quarta opcao: trocar o premio por afinar a bolsa.
    const char *rot = "remover um dado";
    int w = gfx_largura(rot, 1) + 28;
    bool cur = cursor == 3;
    gfx_rect(GFX_W / 2 - w / 2, 240, w, 26, cur ? (pode ? C_VINHO_CLR : C_FELTRO2) : C_FELTRO_ESC);
    gfx_moldura(GFX_W / 2 - w / 2, 240, w, 26, 1, cur ? C_BRANCO : C_FELTRO2);
    txt_c(GFX_W / 2, 243, rot, cur ? C_BARRA : (pode ? C_MARFIM : C_FELTRO2), cur);
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
        gfx_rect(x - 48, yb, 96, 92, cur ? C_FELTRO_ESC : C_FELTRO);
        gfx_moldura(x - 48, yb, 96, 92, cur ? 3 : 1, cur ? cor_aro(tp) : C_FELTRO2);
        float bob = cur && !ja ? sinf(t_fase * 4) * 3 : 0;
        peca_parada((float)x, yb + 32 + bob, 22, loja[i], vez);
        if (ja) {
            txt_c(x, yb + 64, "comprado", C_TEXTO_M, false);
            gfx_rect_alfa(x - 47, yb + 1, 94, 90, C_SOMBRA, 120);
        } else {
            fichas_c(x, yb + 64, pr, "", da ? C_OURO : C_VINHO_CLR);
        }
    }
    dado_e_poder(240, loja[cursor].tipo);
    if (!levou[vez][cursor] && p->fichas < preco_loja(loja[cursor].tipo))
        txt_c(GFX_W / 2, 240 - TXT_H - 2, "fichas insuficientes", C_VINHO_CLR, true);
    txt_c(GFX_W / 2, 286, "setas escolhem  ·  ENTER compra  ·  TAB/CTRL termina as compras", C_TEXTO_M, false);
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

static void elipse(int cx, int cy, int rx, int ry, uint16_t c)
{
    if (rx < 1) rx = 1;
    for (int dy = -ry; dy <= ry; dy++) {
        float f = 1.0f - (float)(dy * dy) / (float)(ry * ry);
        int w = (int)(rx * sqrtf(f > 0 ? f : 0));
        gfx_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}

// Moeda girando: a largura aparente e |cos|, e a face troca quando ela
// passa de lado. A Moeda da sorte mostra azul (1) e vermelho (2).
static void moeda_girando(int cx, int cy, int tipo, float t)
{
    const int R = 48;
    float c = cosf(t * 3.0f);
    int face = c >= 0 ? 1 : 2;
    int rx = (int)(R * fabsf(c));
    uint16_t aro_c = cor_aro(tipo), corpo = C_MARFIM_D, marca = C_MARFIM;
    if (tipo == D_D2) corpo = gfx_mistura(C_MARFIM_D, C_OURO, 58);   // moeda dourada
    bool ficha_casino = true;
    if (tipo == D_SORTE)        { aro_c = C_MARFIM_S; corpo = face == 1 ? C_AZUL : C_VERMELHO; }
    else if (tipo == D_SEMENTE) { aro_c = C_MARFIM_S; corpo = C_VERDE; }
    else if (tipo == D_FOGO)    { aro_c = C_BRASA; corpo = face == 1 ? C_FOGO : C_BRASA; marca = C_AMARELO; }
    else {
        ficha_casino = false;
        if (tipo == D_TUDO_NADA) { aro_c = C_VIOLETA; corpo = face == 2 ? C_MARFIM : C_EBANO; }
    }

    elipse(cx + 6, cy + 8, rx + 2, R, C_SOMBRA);
    elipse(cx, cy, rx + 4, R + 4, aro_c);
    elipse(cx, cy, rx, R, corpo);
    if (ficha_casino && rx > 12)
        for (int i = 0; i < 8; i++) {
            float a = i * 0.785f;
            int mx = cx + (int)(cosf(a) * (rx - 9)), my = cy + (int)(sinf(a) * (R - 9));
            gfx_disco(mx, my, 4, marca);
        }
    if (fabsf(c) > 0.55f) {
        char n[2] = { (char)('0' + face), 0 };
        uint16_t cn = ficha_casino ? C_MARFIM
                    : (tipo == D_TUDO_NADA ? (face == 2 ? C_VIOLETA : C_MARFIM) : C_EBANO);
        gfx_texto(cx - gfx_largura(n, 3) / 2, cy - 36, n, cn, 3, true);
    }
}

// Faces que o dado pode mostrar, na ordem em que o catalogo as percorre.
static int faces_de(int tipo, int *f)
{
    int n = 0, lados = TIPO[tipo].lados;
    int de = TIPO[tipo].efeito == EF_LASTRO ? 6 : 1;
    for (int v = de; v <= lados; v++) f[n++] = v;
    return n;
}

// Quebra o texto em linhas de ate max_col caracteres (contando acentos como
// um so), sempre nos espacos.
static int quebra_linhas(int x, int y, int max_col, int max_lin, const char *txt_, uint16_t c)
{
    char linha[160];
    int lin = 0;
    const char *p = txt_;
    while (*p && lin < max_lin) {
        int n = 0, cols = 0, ultimo_espaco = -1;
        while (p[n] && cols < max_col) {
            if (p[n] == ' ') ultimo_espaco = n;
            n++;
            while (((unsigned char)p[n] & 0xC0) == 0x80) n++;
            cols++;
        }
        if (p[n] && ultimo_espaco > 0) n = ultimo_espaco;
        memcpy(linha, p, (size_t)n);
        linha[n] = 0;
        txt(x, y + lin * (TXT_H + 4), linha, c, false);
        lin++;
        p += n;
        while (*p == ' ') p++;
    }
    return lin;
}

static void desenha_catalogo(void)
{
    int tipo = cat_tipo(cat_i);
    const tipo_t *d = &TIPO[tipo];

    // Faixas: o que se ve e como se navega.
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(18, ty, "DADOS DA CASA", C_LATAO, true);
    char s[32];
    snprintf(s, sizeof s, "%d de %d", cat_i + 1, D_N);
    txt(GFX_W - 18 - gfx_largura(s, 1), ty, s, C_TEXTO_M, false);
    int yb = GFX_H - FAIXA_H;
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
    txt(18, yb + ty + 2, "<  A     D  >", C_TEXTO_M, false);
    txt(GFX_W - 18 - gfx_largura("ENTER volta", 1), yb + ty + 2, "ENTER volta", C_TEXTO_M, false);

    // A peca, a esquerda, girando pelas faces possiveis.
    int cx = 160, cy = CENTRO + 4;
    gfx_disco(cx, cy, 78, C_FELTRO_ESC);
    if (d->lados == 2) {
        moeda_girando(cx, cy, tipo, t_fase);
    } else {
        int f[24], nf = faces_de(tipo, f);
        int i = (int)(t_fase / 0.45f) % nf;
        float dentro = fmodf(t_fase, 0.45f);
        float r = 46 + (dentro < 0.08f ? 4 : 0);          // pulinho a cada face
        float ang = sinf(t_fase * 1.7f) * 0.22f;
        bool com_pips = d->lados == 6 && d->efeito != EF_ESPELHO;
        desenha_dado((float)cx, (float)cy, r, tipo, 0, ang, com_pips ? f[i] : -1, 2, false);
        if (!com_pips) {
            snprintf(s, sizeof s, "%d", f[i]);
            gfx_texto(cx - gfx_largura(s, 2) / 2, cy - 24, s, C_EBANO, 2, true);
        }
        if (tipo == D_TREVO && f[i] == 4) {
            gfx_rect(cx - 30, cy + 58, 60, TXT_H + 4, C_BARRA);
            txt_c(cx, cy + 60, "SORTE", C_OURO, true);
        }
    }

    // A direita: nome, lados, raridade e o que ele faz.
    int x = 290;
    gfx_texto(x, 70, d->nome, cor_aro(tipo), 2, true);
    const char *rar = so_inicial(tipo) ? "dado inicial" : RARIDADE[d->raridade];
    if (d->lados == 2) snprintf(s, sizeof s, "moeda  ·  %s", rar);
    else snprintf(s, sizeof s, "d%d  ·  %s", d->lados, rar);
    txt(x, 118, s, C_TEXTO_M, false);
    gfx_rect(x, 144, 320, 1, C_FELTRO2);
    quebra_linhas(x, 156, 40, 6, TEXTO_CAT[tipo], C_MARFIM);
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
    txt(18, ty, titulo, C_LATAO, true);
    if (dir) txt(GFX_W - 18 - gfx_largura(dir, 1), ty, dir, C_TEXTO_M, false);
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
    txt_c(GFX_W / 2, yb + ty + 2, rodape, C_TEXTO_M, false);
}

// Botao grande de menu, centrado.
static void botao_menu(int y, int w, const char *rot, bool cur)
{
    gfx_rect(GFX_W / 2 - w / 2 + 2, y + 2, w, 26, C_SOMBRA);
    gfx_rect(GFX_W / 2 - w / 2, y, w, 26, cur ? C_LATAO : C_FELTRO_ESC);
    gfx_moldura(GFX_W / 2 - w / 2, y, w, 26, 1, cur ? C_BRANCO : C_FELTRO2);
    txt_c(GFX_W / 2, y + 3, rot, cur ? C_BARRA : C_MARFIM, cur);
}

// Quatro casas com as letras do codigo da sala.
static void casas_codigo(int y, const char *cod, bool cursor_pisca)
{
    for (int i = 0; i < 4; i++) {
        int x = GFX_W / 2 - 110 + i * 56;
        gfx_rect(x, y, 48, 56, C_BARRA);
        gfx_moldura(x, y, 48, 56, 2, C_LATAO);
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
        txt_c(GFX_W / 2, 238, "Quem cria a sala joga primeiro na loja.", C_TEXTO_M, false);
    }
}

// ---------------------------------------------------------------------------
// Tutorial
// ---------------------------------------------------------------------------
typedef struct { const char *titulo; const char *par[3]; } pagina_t;
static const pagina_t TUTORIAL[] = {
    { "O jogo", {
        "Cada jogador começa com 100 fichas. São 15 rodadas, e cada uma custa 5 fichas de entrada, que vão para o pote.",
        "Quem faz mais pontos na rodada leva o pote. No fim, ganha quem tiver mais fichas.",
        NULL } },
    { "A bolsa e a fila", {
        "Cada jogador tem uma bolsa de dados. A cada rodada, você tira 7 dados dela e escolhe até 4 para a sua fila, na ordem que quiser.",
        "Depois joga os dados da fila um por um, do primeiro ao último.",
        NULL } },
    { "A regra da queda", {
        "O primeiro dado sempre vale. Cada dado seguinte precisa ser IGUAL OU MAIOR que o último que vale.",
        "Se sair menor, ele e o último que valia são anulados juntos. Por isso a ordem importa: dados pequenos na frente, grandes no fim.",
        NULL } },
    { "Parar ou arriscar", {
        "Depois de cada dado você escolhe: jogar o próximo ou parar e ficar com os pontos que tem.",
        "Quem venceu a rodada anterior joga primeiro, no escuro. Quem joga depois já sabe o alvo: o botão parar mostra se você vence, perde ou empata.",
        NULL } },
    { "Apostas", {
        "Entre os dois turnos, com os pontos de quem jogou primeiro na mesa, dá para apostar 10 ou 25 fichas.",
        "O outro escolhe: pagar para ver, dobrar a aposta ou correr. Quem corre entrega o pote na hora.",
        NULL } },
    { "Loja, prêmios e dados especiais", {
        "No começo da partida, a loja vende dados especiais em troca de fichas.",
        "Quem vence uma rodada escolhe um prêmio: um dado novo, ou tirar um dado fraco da bolsa.",
        "São 33 dados, cada um com um poder: veja todos em Dados da casa." } },
    { "Teclas", {
        "Setas movem  ·  ENTER escolhe e joga  ·  TAB/CTRL confirma ou para",
        "BACKSPACE tira da fila  ·  X recusa o prêmio  ·  ESC duas vezes: menu",
        "M som  ·  N música  ·  F11 tela cheia.  Boa sorte!" } },
};
#define N_TUTORIAL ((int)(sizeof TUTORIAL / sizeof TUTORIAL[0]))

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
    case 5: {                                        // alguns dados especiais
        static const uint8_t E[5] = { D_ESCUDO, D_JACKPOT, D_EGOISTA, D_COPIADOR, D_FOGO };
        for (int i = 0; i < 5; i++) {
            int x = cx + (i - 2) * 80;
            float bob = sinf(t_fase * 3 + i) * 3;
            peca_parada((float)x, y + 32 + bob, 20, nova_peca(E[i]), 0);
            txt_c(x, y + 54, TIPO[E[i]].nome, cor_aro(E[i]), false);
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
    int y = MESA_Y0 + 56;
    for (int i = 0; i < 3 && pg->par[i]; i++) {
        int n = quebra_linhas(44, y, 69, 4, pg->par[i], i == 0 ? C_MARFIM : C_MARFIM_S);
        y += n * (TXT_H + 4) + 4;
    }
    ilustra_tutorial(tut_pag, 226);                  // faixa fixa, abaixo do texto
    // Bolinhas das paginas.
    for (int i = 0; i < N_TUTORIAL; i++)
        gfx_disco(GFX_W / 2 + (i - N_TUTORIAL / 2) * 16, MESA_Y1 - 6, 3,
                  i == tut_pag ? C_OURO : C_FELTRO2);
}

// Avisos por cima de tudo: sair da partida e conexao perdida.
static void desenha_avisos(void)
{
    const char *msg = NULL;
    uint16_t c = C_OURO;
    if (conexao_caiu()) { msg = "A conexão com o rival caiu.  ESC volta ao menu."; c = C_VINHO_CLR; }
    else if (t_sair >= 0) msg = "Aperte ESC de novo para sair da partida.";
    if (!msg) return;
    int w = gfx_largura(msg, 1) + 40;
    gfx_rect(GFX_W / 2 - w / 2, MESA_Y0 + 6, w, TXT_H + 12, C_BARRA);
    gfx_moldura(GFX_W / 2 - w / 2, MESA_Y0 + 6, w, TXT_H + 12, 2, c);
    txt_c(GFX_W / 2, MESA_Y0 + 11, msg, c, true);
}

void dado_desenha(void)
{
    mesa();
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
    case F_FIM:    desenha_fim();    break;
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
    desenha_avisos();
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

static void segue_do_resultado(void)
{
    if (venc_mao == 0 || venc_mao == 1) {
        primeiro = venc_mao;
        abre_premio();
    } else {
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
    } else if (f->n < N_FILA) {
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

    if (k == 'm' && !dado_digitando()) {
        bool on = som_liga(!som_ligado());
        if (!on && som_falhou()) snprintf(nota, sizeof nota, "sem saída de som neste computador");
        if (on) som_toca(SOM_FICHA);
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
        if (e) { menu_cur = (menu_cur + e + MN_N) % MN_N; som_toca(SOM_TIQUE); return; }
        if (k >= '1' && k < '1' + MN_N) menu_cur = k - '1';
        if (k != KEY_ENTER && !(k >= '1' && k < '1' + MN_N)) return;
        som_toca(SOM_FICHA);
        switch (menu_cur) {
        case MN_UM: case MN_DOIS:
            modo = menu_cur == MN_UM ? M_UM : M_DOIS;
            cpu = modo == M_UM ? 2 : 0;
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
        if (e) {
            cat_i = (cat_i + e + D_N) % D_N;
            t_fase = 0;
            som_toca(SOM_TIQUE);
            return;
        }
        if (k == KEY_ENTER || k == KEY_BKSP || k == KEY_TAB) ao_menu_sem_partida();
        return;

    case F_LOJA: {
        jogador_t *p = &J[vez];
        if (e) { cursor = (cursor + e + N_LOJA) % N_LOJA; return; }
        if (k >= '1' && k <= '5') cursor = k - '1';
        if (k == KEY_ENTER || (k >= '1' && k <= '5')) {
            int tp = loja[cursor].tipo;
            if (levou[vez][cursor]) return;
            if (p->fichas < preco_loja(tp) || p->n_col >= MAX_COL) return;
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
        if (k >= '1' && k <= '7' && k - '1' < p->n_mao) { cursor = k - '1'; poe_na_fila(cursor); return; }
        if (k == KEY_ENTER) { poe_na_fila(cursor); return; }
        if (k == KEY_BKSP && f->n) { f->n--; return; }
        if ((k == KEY_TAB || k == 'x') && f->n >= 1) { cursor = 0; fase = F_JOGA; }
        return;
    }

    case F_JOGA: {
        fila_t *f = &F[vez];
        bool pode_jogar = f->lancados < f->n, pode_parar = f->lancados > 0;
        if (e) { if (pode_parar) cursor ^= 1; return; }
        if (k == KEY_TAB || k == 'x') { if (pode_parar) termina_turno(); return; }
        if (k == KEY_ENTER) {
            if (cursor == 1 && pode_parar) termina_turno();
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
        if (k == 'x') { nova_rodada(); return; }
        if (k == KEY_ENTER) {
            if (cursor == 3) {
                if (p->n_col > N_FILA) { removendo = true; cursor = 0; }
                return;
            }
            // O dado ganho fica fora da bolsa ate o proximo embaralho: o
            // premio chega, mas nao na rodada seguinte.
            if (p->n_col < MAX_COL) {
                p->col[p->n_col] = oferta[cursor];
                p->onde[p->n_col++] = FORA;
            }
            nova_rodada();
        }
        return;
    }

    case F_FIM:
        if (k == KEY_ENTER) dado_inicia(partidas + 1);
        return;

    default:
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
const char *dado_dbg_cat_nome(void) { return TIPO[cat_tipo(cat_i)].nome; }
int  dado_dbg_ev_tipo(void)  { return ev_i < n_ev ? ev[ev_i].tipo : -1; }
void dado_dbg_cpu(int mascara) { cpu = mascara; }
int  dado_dbg_n_tipos(void) { return D_N; }
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
