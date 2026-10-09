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
       F_PERFIS, F_PCRIA, F_RDIFIC, F_REVENTO, F_CREDITOS };

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
// A vez da maquina contra uma pessoa: os menus dela nao aparecem; o que ela
// decidiu (parou, passou, apostou...) aparece numa linha curta no lugar.
static char anuncio[64];
static uint16_t anuncio_cor;
static float t_anuncio;
// Na loja e no premio, o anuncio mostra tambem os dados que ela levou.
static peca_t anuncio_peca[5];
static int n_anuncio_peca, anuncio_dono;
static int  ver_tipo;
static bool ver_comum;               // veredito do dia a dia (vale 5, caiu): so o numero fala
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
static bool dado_liberado(int t);
static void des_resumo(char *s, int n);
static void des_esc(void);
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
