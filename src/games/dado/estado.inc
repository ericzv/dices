// ===========================================================================
// Estado
// ===========================================================================
// Uma peca da bolsa: o tipo, e para o Quebrado o numero gravado nele. No
// Desafiante, os adesivos (adesivos.inc): um por lado, meio byte cada (o lado
// 1 no meio byte baixo de ad[0]), e o que o Fermento dela ja cresceu.
#define AD_LADOS 20
typedef struct { uint8_t tipo, fixo; uint8_t ad[AD_LADOS / 2]; uint8_t fermento; } peca_t;

// Cada dado do jogador esta num de tres lugares.
enum { NA_BOLSA, NA_MAO, FORA, COMIDO };    // COMIDO: a Autofagia o comeu; volta na proxima partida

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
    int perdidas;                // rodadas que ele perdeu nesta partida (Rancor)
    bool perdeu_ultima;          // e se perdeu a ultima (Desforra, a Revanche do Rival)
} jogador_t;

enum { V_ESPERA, V_VALIDO, V_ANULADO, V_ROUBADO };

typedef struct {
    int8_t  idx[N_FILA];         // posicoes em col, na ordem escolhida
    uint8_t tipo[N_FILA], fixo[N_FILA];
    int     n;
    int     lancados;
    int     cru[N_FILA];
    int     valor[N_FILA];
    uint8_t lado[N_FILA];        // o lado que ficou para cima (o dos adesivos); 0: nenhum
    uint8_t est[N_FILA];
    uint8_t dobra[N_FILA];       // so para a etiqueta: o x2 vale no fim
    int8_t  bonus[N_FILA];
    bool    quebra[N_FILA];
    bool    cinza[N_FILA];           // queimado pela Ficha de Fogo
    uint8_t descarte[N_FILA];        // Viciado: o menor dos dois lances
    bool    espera[N_FILA];          // Espelho que so vira no fim (o rival nao tinha jogado)
    bool    ancora[N_FILA];          // preso pela Ancora de antes: nao cai nem e roubado
    bool    calibre[N_FILA];         // o Calibre de antes somou 1 no que ele tirou
    bool    carta;                   // Coringa: o 4o e a carta em suspensao ("?"), que so vira depois do seu turno
    bool    escondido[N_FILA];       // Oculto lancado: o numero so aparece no fim da rodada
    char    tag[N_FILA][14];
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
    uint8_t lado;                    // o lado que fica para cima (os adesivos)
    int8_t etapa;                    // adesivo agindo no pouso: 1 o Avesso vira; 2 o Reflexivo rola de novo
    bool virou;                      // o Avesso ja virou
    bool reflete;                    // depois (ou sem) o Avesso, o Reflexivo
    bool chumbou;                    // ja deu o baque do Chumbo
    float t_etapa;
    int valor_v; uint8_t lado_v;     // depois de virar do avesso
    int valor2, menor2; uint8_t lado2;   // o segundo lance, do Reflexivo
    float hesita, giro_h;            // hora da verdade: quanto ainda hesita na quina, e o angulo dela
} voo_t;

// Eventos do desfecho, tocados um a um.
enum { EV_ANULA, EV_ROUBA, EV_TIRA, EV_PIRATA, EV_BONUS, EV_MULT, EV_FOGO, EV_SEMENTE,
       EV_JACKPOT, EV_ESPELHO,
       EV_EXTRA,                     // bonus fixo do fim (Misericordioso, Solitario, Desafiante, Capitao)
       EV_RENASCE,                   // a Fenix volta a valer
       EV_CHEFE,                     // pontos direto no placar (virada do Barao, Crisantemo, Velorio)
       EV_CAVALO,                    // o Cavalo de Troia se abre: anula os dados de quem o atingiu
       EV_CORTA,                     // o Sabotador: o maior do rival passa a valer metade
       EV_AGIOTA,                    // o Agiota: fichas do rival para o dono, fora do pote
       EV_MEDUSA,                    // a Medusa: a fila vira pedra (Quebrados)
       EV_GARIMPO,                   // o Garimpeiro: os outros validos voltam para a bolsa
       EV_REVELA,                    // o Oculto mostra o que tirou
       EV_VAMPIRO,                   // o Vampiro: -1 em cada dado do oponente que vale
       EV_AVISO,                     // so o texto (a Saideira avisa que tudo vai de novo)
       EV_ADESIVO };                 // um adesivo do lado que ficou para cima (ad: qual)
// Mascara de amuletos: bit a, o amuleto a (sao mais de 32).
typedef uint64_t amul_t;
#define AM_BIT(a) ((amul_t)1 << (a))
typedef struct {
    uint8_t tipo, de_j, de_k, alvo_j, alvo_k;
    int valor;
    uint8_t mascara;                 // dados da fila atingidos (BONUS, MULT, Capitao, Cavalo)
    int8_t  por_dado[N_FILA];        // bonus ou multiplicador de cada um
    amul_t  am;                      // amuletos que acendem quando ele toca
    uint8_t ad;                      // EV_ADESIVO: o adesivo
    char txt[64];
} evento_t;
#define MAX_EV 64                    // a Saideira repete a cadeia inteira

typedef struct { uint8_t tipo, fixo, de_j, anulado; int valor; } roubado_t;   // anulado: pelo Cavalo

enum { F_ORDEM, F_APOSTA, F_ROLANDO, F_ARRUMA, F_RESULTADO, F_PREMIO, F_FIM,
       F_LOJA, F_ABERTURA, F_JOGA, F_VEREDITO, F_EFEITOS, F_CATALOGO,
       F_ONLINE, F_TUTORIAL,
       F_MAPA, F_RLOJA, F_RBOLSA, F_RFIM, F_RAMULETO,       // Modo Desafiante
       F_PERFIS, F_PCRIA, F_RDIFIC, F_CREDITOS, F_HIST, F_ESTILO, F_NOVIDADES };

static jogador_t J[2];
static fila_t F[2];
static voo_t voo[2];
static int  n_voo;
static int  fase, vez, primeiro, rodada, pote, cursor, partidas;
static int  a_pagar, passes;
static bool aumentou;
static float t_fase, t_som;
// Dados da sequencia que nao foram lancados: no fim do turno voam de volta
// para a bolsa (t_devolve: tempo desde que sairam; -1: nenhum no ar).
#define DEVOLVE_T 0.6f
typedef struct { float x, y; uint8_t tipo, dono, fixo; } devolvido_t;
static devolvido_t devolvidos[N_FILA];
static int   n_devolvidos;
static float t_devolve = -1;
static int   log_base[2];                 // pontos de cada um quando os efeitos do fim comecam
static float t_relogio;                  // corre sempre: brilhos que nao recomecam a cada fase
// A hora da verdade (hora.inc): o ultimo dado da rodada, quando quem leva o
// pote depende da face dele. hora: 0 nada, 1 decide a rodada, 2 a partida.
#define HESITA_T 0.26f                   // o dado hesita na quina antes de deitar (tempo do lance)
static int   hora;
static float t_hora;                     // 0..1: o quanto a mesa ja escureceu em volta dele
static float hora_x, hora_y;             // onde esta a luz
static int  dado_decisivo(int j, int k);
static void hora_comeca(int grau);
// O crescendo: a mesa reage a sequencias longas e a combos. A onda faz os
// dados de onda_j pularem um depois do outro (e as fichas do pote); o
// tremor sacode o quadro todo, a menos que a pessoa o desligue (tecla T).
static float t_onda = -1;
static int   onda_j, onda_n, onda_nivel;
static float t_tremor, tremor_dur, tremor_px;
static bool  tremor_off, tremor_lido;
static float t_aviso_tremor;
static void crescendo(int j, int nivel);
static void reage_evento(const evento_t *e);
// O golpe: nos lances grandes a mesa para alguns quadros, com um clarao em
// quem acertou, e so depois treme (t_congela corre no tempo real).
static float t_congela, congela_dur, golpe_px, golpe_dur;
static int8_t congela_j = -1;
static uint8_t congela_masc;             // dados de congela_j que clareiam; bit N_FILA: o placar
static void golpe(int j, unsigned masc);
static bool ev_espera;                   // o golpe segurou o comeco do efeito seguinte
// Um numero que conta ate o valor de verdade em vez de pular (hora.inc): o
// placar de cada fila e, no Desafiante, as moedas.
typedef struct { float vis, pulo; int mostra, sinal, n_som; } contador_t;
static contador_t placar_c[2];
static float t_tique_placar;             // os tiques da contagem nao se amontoam
#define RODADA_GRANDE 30                 // pontos de quem leva para a mesa vibrar no fim
static int  venc_mao, venc_partida;
static int  correu_venc = -1;        // quem levou o pote porque o outro correu
static int  zerou = -1;              // quem abriu a rodada e ficou com 0: perdeu na hora
static int  vence_j = -1;            // quem joga em segundo e ja passou do alvo: a vez acabou
static float t_vence;                // desde quando (o placar dele acende)
static peca_t oferta[3];
static char aviso[96], veredito[96], nota[48];
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
static uint8_t vis_corte[2][N_FILA];            // cortado pelo Sabotador: vale metade
static uint8_t vis_pedra[2][N_FILA];            // petrificado pela Medusa: vira Quebrado no fim
static uint8_t vis_espelho[2][N_FILA];
static uint8_t vis_oculto[2][N_FILA];        // Oculto que ja apareceu na conta, mas ainda com "?" na mesa
static uint8_t est_pre_espelho[2][N_FILA];   // a sequencia antes do Espelho virar (para a animacao)   // Espelho que ja virou na conta, mas cuja vez de mostrar nao chegou
static int  vis_semente[2];

#define N_LOJA 5
static peca_t loja[N_LOJA];
static bool levou[2][N_LOJA];
static int  loja_primeiro;
static bool abertura_pronta;
static int  menu_cur;                // opcao do menu de abertura
// Qual lista o menu mostra: a principal ou a escolha da run do Modo
// Desafiante (continuar ou comecar outra).
enum { MT_PRINCIPAL, MT_DESAFIO };
static int  menu_tela;
static float sacode;                 // 1 quando o cursor do menu anda: os dados chacoalham
enum { MN_UM, MN_DOIS, MN_ONLINE, MN_DADOS, MN_TUTORIAL, MN_N };
// As fichas do menu: o quanto cada uma esta erguida (o cursor ergue), o giro
// das listras da borda e a escolhida assentando (a acao vem no fim).
static float menu_sobe[MN_N], menu_giro[MN_N];
static float t_menu_clique = -1;
static int   menu_escolhida, menu_ant = -1, menu_tela_ant = -1;

// Modos: contra o computador, dois no mesmo teclado, ou online.
enum { M_UM, M_DOIS, M_ONLINE, M_DESAFIO };
static int  modo;
// Quantos dados cabem na colecao: em qualquer modo, so o espaco da
// estrutura (128, bem longe do que uma run junta); nao ha limite de jogo.
static int lim_col(void) { return MAX_COL; }

// Amuletos: so no Modo Desafiante, so de quem joga (o oponente nunca tem).
// A run guarda quais tem; a partida copia para amul[0].
enum { A_LUVA, A_REDE, A_BOLSA, A_OURO, A_PENA, A_CONTRATO, A_IMA, A_COURACA,
       A_COFRE, A_RESERVA, A_POTE, A_FURADO,
       // Liberados pelas dificuldades (dois por nivel).
       A_FERRADURA, A_PESADO, A_RELOGIO, A_CARTOLA, A_COROA, A_CUPOM,
       A_PRENSA, A_TACA, A_CORINGA, A_DUPLA, A_SELO, A_PEDAGIO,
       // Vindos dos trunfos do Rival, da Bruxa e do Coveiro (abertos desde o comeco).
       A_DESFORRA, A_MAU_OLHADO, A_CRISANTEMO,
       // Os das raridades: dois incomuns e tres raros (abertos desde o comeco).
       A_TRINCA, A_PIRAMIDE, A_ABRE_ALAS, A_SAIDEIRA, A_CORRIMAO,
       // Comum, aberto desde o comeco.
       A_COLAR, A_N };
#define AM_LUGARES 5                 // amuletos que cabem na run; o sexto pede troca
#define A_BASICOS A_FERRADURA
static amul_t amul[2];
static amul_t am_disparou;           // amuletos que acabaram de agir (a faixa acende)
static int8_t ferr_k[2] = { -1, -1 };  // dado que a Ferradura salvou nesta rodada
static int des_rod_vencidas;         // rodadas que voce venceu na partida (Taca)
static float am_brilho[A_N];
static bool tem_am(int j, int a) { return modo == M_DESAFIO && (amul[j & 1] >> a & 1); }
// Trunfos dos oponentes do Desafiante que nao sao amuletos (o resto deles
// usa amul[1]): Nervoso (Novato), Batedor de carteira (Ladrao), A casa
// sempre ganha (Crupie).
enum { REG_NADA, REG_NERVOSO, REG_BATEDOR, REG_CASA, REG_ULTIMA, REG_FOLEGO, REG_SAQUE,
       REG_MALUCO, REG_JUROS, REG_CORINGA };
static uint8_t reg_op;
static uint8_t estilo_op;                // o jeito de apostar do oponente (ES_*, desafio/run.inc)
static int8_t maluco_lado;               // a moeda do Maluco nesta rodada: +2 (cara) ou -1 (coroa)
static int juros_rodada;                 // a entrada a mais que a Banqueira cobrou nesta rodada
static float t_fantasma;                 // > 0: um Fantasma voando para uma bolsa...
static int8_t fantasma_para;             // ...a de quem (Bruxaria da Bruxa, ou o seu Mau-Olhado)
static bool carta_virando;               // a carta do Coringa virou e esta sendo lancada agora
static bool op_nervoso;              // o Novato perdeu a ultima rodada
static bool tem_reg(int r) { return modo == M_DESAFIO && reg_op == r; }
// Falas do oponente (desafio.inc): o que acabou de acontecer.
enum { FA_INICIO, FA_CAI, FA_GANHA_R, FA_PERDE_R, FA_GANHA_P, FA_PERDE_P, FA_N };
static void fala_evento(int cat);
static char  fala_txt[64];
static float t_fala;                     // > 0: o balao do oponente esta na mesa
static float fala_dur, fala_dita;        // quanto a fala dura, e ha quanto tempo comecou (sai letra por letra)
static void fala_comeca(float dur) { t_fala = fala_dur = dur; fala_dita = 0; }
// Reacoes do oponente (falas.inc): o rosto muda por um instante quando algo
// acontece na mesa e, as vezes, sai uma palavra num balao curto.
enum { RM_PASSOU, RM_ELE_PASSOU, RM_CAI_DELE, RM_CAI_SEU, RM_COMBO, RM_ATACOU, RM_HORA,
       RM_APOSTA, RM_CORREU, RM_GANHA_R, RM_PERDE_R, RM_N };
static void rival_reage(int momento);
// O historico do perfil (desafio/historico.inc): o que se conta na partida.
static void hist_lance(int j, bool cheia, int hora_res);
static void hist_jackpot(void);
static void hist_rodada(int pote_seu);
static void hist_partida(int vencedor);
static void hist_run_venceu(void);
static void estilo_do_perfil(void);
static int nivel_jog[2];                // o nivel da IA de cada lado (ia.inc)      // le o estilo do perfil escolhido e monta a paleta (desafio/estilo.inc)
static void rival_lance(int j, int k);
static int   reacao_expr;                // a expressao por cima da de sempre (EX_*)
static float t_reacao;                   // > 0: o rosto ainda reage
static float t_curta;                    // > 0: espera antes de outra palavra
static bool  fala_e_curta;               // o balao da vez e so uma palavra
static int   placar_antes[2];            // os totais antes do lance: quem passou quem
// Velocidade: normal, media (uma vez e meia) ou rapida (em dobro), nas
// animacoes e na vez do computador (menos no online, onde os dois lados
// precisam andar juntos). Fica guardada: "0" normal, "2" media, "1" rapida
// (o "1" e o rapido de antes).
enum { VEL_NORMAL, VEL_MEDIA, VEL_RAPIDA, VEL_N };
static const float VEL_FATOR[VEL_N] = { 1.0f, 1.5f, 2.0f };
static const char VEL_GRAVA[VEL_N] = { '0', '2', '1' };
static int velocidade = VEL_MEDIA;
static bool rapido_lido;
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
// A vinheta do Dado Misterioso: > 0 enquanto corre (primeiro a pedra se parte
// na mesa; depois de MIST_ESPERA, a luz e o dado, com a mesa parada).
#define MIST_ESPERA 1.0f
#define MIST_MIN    1.4f                 // so depois disso uma tecla fecha
#define MIST_MAX    20.0f
static float t_misterio;
static int des_dificuldade(void);
// A virada que o oponente perso traz na dificuldade da run: 0 nenhuma, 1, 2.
static int virada_de(int perso)
{
    int c = perso - PERSO_CHEFE0;
    if (c < 0 || c > 2) return 0;
    static const int8_t ABRE[3] = { 3, 2, 4 }, FORTE[3] = { 8, 7, 10 };   // os niveis em que vira, e forte
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
// O Coringa monta 3: o quarto e a copia do seu dado que mais tirou (partida.inc).
static int max_fila(int j) { return tem_am(j, A_CONTRATO) || (j == 1 && tem_reg(REG_CORINGA)) ? 3 : N_FILA; }

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
static void des_contadores_passo(float dt, float dt_real);
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
static bool des_saiu;                // Desafiante: saiu no meio da partida (conta como derrota)
static void des_abandona(void);
static bool des_abandono(void);
static int  tut_pag;                 // pagina do tutorial
static int  cpu;                     // bit j: jogador j e do aparelho
static float t_ia;                   // espera da IA antes de cada gesto
static int  ia_fase = -1, ia_vez = -1;
static int8_t plano[N_FILA];         // a fila que a IA decidiu montar (posicoes na mao)
static int  plano_n, plano_rodada = -1, plano_vez = -1;
static float plano_p;                // chance de vencer com o plano
static int  detalhe = -1, detalhe_fase;   // dado aberto pela tecla I (-1: nenhum)
static int  cat_i, cat_a;         // dado e amuleto em exibicao na colecao
static int  cat_sec;              // secao da colecao: 0 dados, 1 amuletos, 2 adesivos
static int  cat_ad;               // o adesivo em foco na colecao
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
