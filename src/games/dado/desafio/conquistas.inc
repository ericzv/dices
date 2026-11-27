// ---------------------------------------------------------------------------
// Conquistas: alguns dados, amuletos e adesivos abrem por um feito no
// Desafiante, e nao por um chefe (como as bolsas). O item de uma conquista
// abre so por ela; os outros continuam com os chefes. O feito de uma vez
// grava na hora; os contadores (os que somam de run em run) somam no fim de
// cada partida (des_resultado), para a partida refeita nao contar duas vezes.
// Na mesa, nada aparece alem do aviso que desce quando abre; depois da
// partida, o card do item (novidades.inc). Na Colecao, o item fechado mostra
// como abre e, nos que somam, quanto falta.
// ---------------------------------------------------------------------------
enum { CQK_DADO, CQK_AMULETO, CQK_ADESIVO };
enum {
    // A primeira leva: maos de dados faceis, habilidade, bolsa, acumulo e o 1-1-1-1.
    CQ_JACKPOT, CQ_DOMINO, CQ_PRISMA, CQ_DESAFIANTE, CQ_FENIX, CQ_COLECIONADOR, CQ_MOSAICO,
    CQ_CARTOLA, CQ_CORINGA, CQ_ACUMULADOR, CQ_CAMALEAO, CQ_AGOURO,
    // A segunda: mais maos, feitos de partida, o Trono e os de numero repetido.
    CQ_PATINHOS, CQ_GEMEO, CQ_VIUVA, CQ_RANCOR, CQ_TUDO_NADA, CQ_TRONO, CQ_TELA, CQ_DOURADO,
    CQ_CUPOM, CQ_PUXA_FILA, CQ_MALDITO, CQ_ADESIVADO, CQ_AGNUS,
    // A terceira: os encontros, os feitos de partida e os adesivos de build.
    CQ_REVOLVER, CQ_COVEIRO, CQ_SORTE_REVES, CQ_ESPELHO, CQ_ESTRELA, CQ_CAMELO, CQ_PURPURINA,
    CQ_MIRA, CQ_FIGURINHA,
    // A quarta: mais maos e feitos, e mais quatro encontros.
    CQ_EXPLOSIVO, CQ_RELOGIO_BOLSO, CQ_RELOGIO, CQ_TACA, CQ_DINAMITE, CQ_PESADO, CQ_POTE_QUENTE,
    CQ_ECO, CQ_DIGITAL, CQ_GATUNO, CQ_AGIOTA, CQ_MISERICORDIOSO, CQ_OCULTO, CQ_INFECTADO,
    CQ_AMPULHETA,
    // A secreta de sempre: a pedra negativa que se parte (partida.inc, pedra_misteriosa).
    CQ_MISTERIOSO,
    // A da v1.26: a terceira run, as derrotas para chefe e a vitoria com pouco.
    // (So no fim: o bit de cada uma e a posicao dela, guardada no perfil.)
    CQ_BUMERANGUE, CQ_FURIA, CQ_MOEDA_SORTE,
    CQ_N };
#define CQ_V126 CQ_BUMERANGUE                    // as conquistas a partir da v1.26
_Static_assert(CQ_N <= 64, "as conquistas cabem nos 64 bits do perfil");

typedef struct {
    uint8_t k, item;                     // CQK_* e o dado, o amuleto ou o adesivo que ela abre
    uint16_t alvo;                       // o contador: o total (0: um feito so)
    bool secreta;                        // na Colecao, fechada: "Conquista secreta", sem o texto
    const char *texto;                   // como abre (com a primeira letra maiuscula e sem ponto)
    const char *dica;                    // a secreta: o que a Colecao mostra no lugar (NULL: nada)
} conquista_t;

// Os limiares, faceis de mudar.
#define CQ_FENIX_FICHAS   15
#define CQ_COLECAO_DADOS  22
#define CQ_MOSAICO_AD     8
#define CQ_CARTOLA_COISAS 4
#define CQ_ACUM_VITORIAS  30
#define CQ_CAMALEAO_OPS   15
#define CQ_VIUVA_PARTIDAS 5
#define CQ_RANCOR_PERDAS  5
#define CQ_TUDO_POTE      80
#define CQ_TELA_LADOS     3
#define CQ_DOURADO_MOEDAS 15
#define CQ_CUPOM_GASTO    40
#define CQ_PUXA_RODADAS   20
#define CQ_ADESIVADO_AD   5
#define CQ_ENCONTRO_VEZES 3
#define CQ_ESPELHO_PONTOS 12
#define CQ_ESTRELA_PONTOS 40
#define CQ_CAMELO_BOLSAS  5
#define CQ_PURPURINA_AD   3
#define CQ_MIRA_DADOS     2
#define CQ_FIGURINHA_AD   20
#define CQ_BOLSO_RODADAS  4              // vencer o chefe antes da 5a rodada
#define CQ_TACA_SEGUIDAS  5
#define CQ_DINAMITE_DIF   24
#define CQ_POTE_FICHAS    100
#define CQ_INFECTADO_DADOS 3
#define CQ_AMPULHETA_ROD  200
#define CQ_BUMERANGUE_RUNS 3
#define CQ_FURIA_DERROTAS 5
#define CQ_MOEDA_PONTOS   3

static const conquista_t CONQ[CQ_N] = {
    [CQ_JACKPOT]      = { CQK_DADO, D_JACKPOT, 0, false, "Tire o mesmo número em três dados válidos de uma sequência", NULL },
    [CQ_DOMINO]       = { CQK_DADO, D_DOMINO, 0, false, "Tire três números seguidos numa sequência, em ordem (como 2-3-4)", NULL },
    [CQ_PRISMA]       = { CQK_DADO, D_PRISMA, 0, false, "Vença uma rodada com 4 dados válidos de 4 tamanhos diferentes", NULL },
    [CQ_DESAFIANTE]   = { CQK_DADO, D_DESAFIANTE, 0, false, "Vença um chefe sem perder nem correr de nenhuma rodada", NULL },
    [CQ_FENIX]        = { CQK_DADO, D_FENIX, 0, false, "Vença uma partida depois de ficar com 15 fichas ou menos", NULL },
    [CQ_COLECIONADOR] = { CQK_DADO, D_COLECIONADOR, 0, false, "Tenha 22 dados na bolsa ao mesmo tempo", NULL },
    [CQ_MOSAICO]      = { CQK_DADO, D_MOSAICO, 0, false, "Tenha 8 adesivos colados na bolsa ao mesmo tempo", NULL },
    [CQ_CARTOLA]      = { CQK_AMULETO, A_CARTOLA, 0, false, "Compre 4 coisas numa mesma loja (dados, adesivos e pacotes contam)", NULL },
    [CQ_CORINGA]      = { CQK_AMULETO, A_CORINGA, 0, false, "Vença o Coringa", NULL },
    [CQ_ACUMULADOR]   = { CQK_DADO, D_ACUMULADOR, CQ_ACUM_VITORIAS, false, "Vença 30 partidas, somando todas as suas runs", NULL },
    [CQ_CAMALEAO]     = { CQK_DADO, D_CAMALEAO, CQ_CAMALEAO_OPS, false, "Vença 15 oponentes diferentes do cassino", NULL },
    [CQ_AGOURO]       = { CQK_DADO, D_MALDITO, 0, true, "Tire 1 em quatro dados de uma mesma sequência", NULL },
    [CQ_PATINHOS]     = { CQK_DADO, D_PATINHOS, 0, false, "Tire 2 em três dados válidos de uma sequência: 2-2-2", NULL },
    [CQ_GEMEO]        = { CQK_DADO, D_GEMEO, 0, false, "Tire dois pares válidos numa sequência (como 2-2-5-5)", NULL },
    [CQ_VIUVA]        = { CQK_DADO, D_VIUVA, CQ_VIUVA_PARTIDAS, false, "Vença uma rodada com só 1 dado válido, em 5 partidas diferentes", NULL },
    [CQ_RANCOR]       = { CQK_DADO, D_RANCOR, 0, false, "Vença uma partida mesmo perdendo 5 rodadas nela", NULL },
    [CQ_TUDO_NADA]    = { CQK_DADO, D_TUDO_NADA, 0, false, "Aposte todas as suas fichas e leve um pote de 80 ou mais", NULL },
    [CQ_TRONO]        = { CQK_DADO, D_TRONO, 0, false, "Vença o Barão sem perder nem correr de nenhuma rodada", NULL },
    [CQ_TELA]         = { CQK_DADO, D_TELA, 0, false, "Tenha um dado com adesivo em 3 lados", NULL },
    [CQ_DOURADO]      = { CQK_ADESIVO, AD_DOURADO, 0, false, "Junte 15 moedas de uma vez", NULL },
    [CQ_CUPOM]        = { CQK_AMULETO, A_CUPOM, 0, false, "Gaste 40 moedas numa mesma run", NULL },
    [CQ_PUXA_FILA]    = { CQK_DADO, D_PUXA_FILA, CQ_PUXA_RODADAS, false, "Termine 20 rodadas com 4 dados válidos, somando as suas runs", NULL },
    [CQ_MALDITO]      = { CQK_DADO, D_MALDICAO, 0, true, "Tire 6 em três dados de uma mesma sequência: 6-6-6", NULL },
    [CQ_ADESIVADO]    = { CQK_DADO, D_ADESIVADO, 0, false, "Tenha 5 adesivos num mesmo dado", NULL },
    [CQ_AGNUS]        = { CQK_DADO, D_AGNUS, 0, true, "Tire 3 em três dados de uma mesma sequência: 3-3-3", NULL },
    [CQ_REVOLVER]     = { CQK_DADO, D_REVOLVER, CQ_ENCONTRO_VEZES, false, "Vença o Caubói 3 vezes", NULL },
    [CQ_COVEIRO]      = { CQK_DADO, D_COVEIRO, CQ_ENCONTRO_VEZES, false, "Vença o Coveiro 3 vezes", NULL },
    [CQ_SORTE_REVES]  = { CQK_DADO, D_SORTE_REVES, 0, false, "Numa partida, vença uma rodada por 1 ponto e perca outra por 1", NULL },
    [CQ_ESPELHO]      = { CQK_DADO, D_ESPELHO, 0, false, "Empate uma rodada com 12 pontos ou mais de cada lado", NULL },
    [CQ_ESTRELA]      = { CQK_DADO, D_ASTRO, 0, false, "Faça 40 pontos ou mais numa única rodada", NULL },
    [CQ_CAMELO]       = { CQK_DADO, D_CAMELO, CQ_CAMELO_BOLSAS, false, "Vença o Crupiê com 5 bolsas diferentes", NULL },
    [CQ_PURPURINA]    = { CQK_ADESIVO, AD_PURPURINA, 0, false, "Faça 3 adesivos seus agirem na mesma rodada", NULL },
    [CQ_MIRA]         = { CQK_ADESIVO, AD_MIRA, 0, false, "Anule 2 dados do oponente numa mesma rodada", NULL },
    [CQ_FIGURINHA]    = { CQK_ADESIVO, AD_FIGURINHA, CQ_FIGURINHA_AD, false, "Cole 20 adesivos, somando todas as suas runs", NULL },
    [CQ_EXPLOSIVO]    = { CQK_DADO, D_EXPLOSIVO, 0, false, "Tire o número máximo em dois dados válidos de 6 lados ou mais numa mesma sequência", NULL },
    [CQ_RELOGIO_BOLSO]= { CQK_AMULETO, A_RELOGIO, 0, false, "Vença um chefe antes da 5ª rodada", NULL },
    [CQ_RELOGIO]      = { CQK_DADO, D_RELOGIO, 0, false, "Vença um chefe virando o placar na última rodada", NULL },
    [CQ_TACA]         = { CQK_AMULETO, A_TACA, 0, false, "Vença 5 rodadas seguidas numa partida", NULL },
    [CQ_DINAMITE]     = { CQK_ADESIVO, AD_DINAMITE, 0, false, "Vença uma rodada por 24 pontos ou mais", NULL },
    [CQ_PESADO]       = { CQK_AMULETO, A_PESADO, 0, false, "Vença uma rodada com quatro dados de seis lados válidos", NULL },
    [CQ_POTE_QUENTE]  = { CQK_DADO, D_POTE_QUENTE, 0, false, "Leve um pote de 100 fichas ou mais", NULL },
    [CQ_ECO]          = { CQK_DADO, D_ECO, 0, false, "Tire o mesmo número nos quatro dados de uma sequência", NULL },
    [CQ_DIGITAL]      = { CQK_DADO, D_DIGITAL, 0, false, "Faça uma escada de quatro numa sequência (como 3-4-5-6)", NULL },
    [CQ_GATUNO]       = { CQK_DADO, D_GATUNO, CQ_ENCONTRO_VEZES, false, "Vença o Ladrão 3 vezes", NULL },
    [CQ_AGIOTA]       = { CQK_DADO, D_AGIOTA, CQ_ENCONTRO_VEZES, false, "Vença a Banqueira 3 vezes", NULL },
    [CQ_MISERICORDIOSO] = { CQK_DADO, D_MISERICORDIOSO, CQ_ENCONTRO_VEZES, false, "Vença o Monge 3 vezes", NULL },
    [CQ_OCULTO]       = { CQK_DADO, D_OCULTO, CQ_ENCONTRO_VEZES, false, "Vença o Ilusionista 3 vezes", NULL },
    [CQ_INFECTADO]    = { CQK_ADESIVO, AD_INFECTADO, 0, false, "Tenha o mesmo adesivo em 3 dados diferentes da bolsa", NULL },
    [CQ_AMPULHETA]    = { CQK_DADO, D_AMPULHETA, CQ_AMPULHETA_ROD, false, "Jogue 200 rodadas, somando todas as suas runs", NULL },
    [CQ_MISTERIOSO]   = { CQK_DADO, D_MISTERIOSO, 0, true, "Jogue um Quebrado de número negativo", "Por que você usaria esse dado?" },
    [CQ_BUMERANGUE]   = { CQK_DADO, D_BUMERANGUE, CQ_BUMERANGUE_RUNS, false, "Comece a sua 3ª run", NULL },
    [CQ_FURIA]        = { CQK_DADO, D_FURIA, CQ_FURIA_DERROTAS, false, "Perca para um chefe 5 vezes, somando as suas runs", NULL },
    [CQ_MOEDA_SORTE]  = { CQK_DADO, D_SORTE, 0, false, "Lance seus dados e vença uma rodada fazendo 3 pontos ou menos", NULL },
};

// Os encontros: vencer o oponente, de run em run, abre o dado dele. Ele
// leva o dado mesmo antes de voce o ter (mapa.inc, monta_oponente).
#define PERSO_LADRAO      3
#define PERSO_ILUSIONISTA 11
#define PERSO_CAUBOI      12
#define PERSO_MONGE       13
#define PERSO_BANQUEIRA   19
#define PERSO_COVEIRO     20
#define CQ_ENCONTROS      6
static const struct { uint8_t perso, cq; } ENCONTRO[CQ_ENCONTROS] = {
    { PERSO_CAUBOI, CQ_REVOLVER }, { PERSO_COVEIRO, CQ_COVEIRO }, { PERSO_LADRAO, CQ_GATUNO },
    { PERSO_BANQUEIRA, CQ_AGIOTA }, { PERSO_MONGE, CQ_MISERICORDIOSO }, { PERSO_ILUSIONISTA, CQ_OCULTO } };
_Static_assert(sizeof ((perfil_t *)0)->cq_encontro == CQ_ENCONTROS, "um contador por encontro");

// ---- Quem e de conquista ------------------------------------------------------
static int cq_do(int k, int item)
{
    for (int c = 0; c < CQ_N; c++) if (CONQ[c].k == k && CONQ[c].item == item) return c;
    return -1;
}
static int cq_do_dado(int t)     { return cq_do(CQK_DADO, t); }
static int cq_do_amuleto(int a)  { return cq_do(CQK_AMULETO, a); }
static int cq_do_adesivo(int a)  { return cq_do(CQK_ADESIVO, a); }
static bool cq_aberta_de(const perfil_t *p, int c) { return c >= 0 && c < CQ_N && (p->conq >> c & 1); }
static bool cq_aberta(int c) { return cq_aberta_de(perfil(), c); }
static int cq_encontro(int perso)
{
    for (int i = 0; i < CQ_ENCONTROS; i++) if (ENCONTRO[i].perso == perso) return i;
    return -1;
}
// O dado do encontro que ele ainda nao deu: no mapa, a mesa dele brilha.
static bool cq_encontro_aberto(int perso)
{
    int e = cq_encontro(perso);
    return e >= 0 && !cq_aberta(ENCONTRO[e].cq);
}

static int cq_bits(uint32_t m) { int n = 0; while (m) { n += m & 1; m >>= 1; } return n; }
// Quanto o contador da conquista c ja somou (o alvo e CONQ[c].alvo).
static int cq_conta_de(const perfil_t *p, int c)
{
    switch (c) {
    case CQ_ACUMULADOR: return p->cq_vitorias;
    case CQ_CAMALEAO:   return cq_bits(p->cq_oponentes);
    case CQ_VIUVA:      return p->cq_viuva;
    case CQ_PUXA_FILA:  return p->cq_cheias;
    case CQ_FIGURINHA:  return p->cq_colados;
    case CQ_AMPULHETA:  return p->cq_rodadas;
    case CQ_CAMELO:     return cq_bits(p->cq_crupie);
    case CQ_BUMERANGUE: return p->runs;
    case CQ_FURIA:      return p->cq_derrotas;
    default:
        for (int i = 0; i < CQ_ENCONTROS; i++) if (ENCONTRO[i].cq == c) return p->cq_encontro[i];
        return 0;
    }
}
static int cq_conta(int c) { return cq_conta_de(perfil(), c); }

// O nome do item, para os avisos e a lista.
static const char *cq_nome(int c)
{
    switch (CONQ[c].k) {
    case CQK_AMULETO: return AMULETO[CONQ[c].item].nome;
    case CQK_ADESIVO: return ADESIVO[CONQ[c].item].nome;
    default:          return TIPO[CONQ[c].item].nome;
    }
}

// O perfil de antes das conquistas: o que um chefe ja tinha aberto pela
// regra antiga continua aberto (a conquista dele conta como feita).
static bool cq_abria_antes(const perfil_t *p, int c)
{
    int it = CONQ[c].item;
    switch (CONQ[c].k) {
    case CQK_AMULETO: return chefe_abriu_de(p, NIVEL_AMULETO_ANTES[it] - 1, CH_DONA);
    case CQK_ADESIVO: return chefe_abriu_de(p, ADESIVO[it].nivel - 1, CH_CRUPIE);   // o nivel de antes
    default:          return nivel_antes(it) > 0 && chefe_abriu_de(p, nivel_antes(it) - 1, chefe_antes(it));
    }
}
static void cq_converte(perfil_t *p)
{
    for (int c = 0; c < CQ_N; c++) if (cq_abria_antes(p, c)) p->conq |= (uint64_t)1 << c;
}
// O perfil da v1.25 ("PER8"): so as conquistas novas, do mesmo jeito.
static void cq_converte_novas(perfil_t *p)
{
    for (int c = CQ_V126; c < CQ_N; c++) if (cq_abria_antes(p, c)) p->conq |= (uint64_t)1 << c;
}

// A medalha de latao: o sinal de conquista (na Colecao e, no mapa, sobre a
// mesa de um encontro).
static void medalha(int x, int y, uint16_t c)
{
    gfx_disco(x, y, 5, c);
    gfx_disco(x, y, 3, C_FELTRO_ESC);
    gfx_pixel(x, y, c);
    gfx_rect(x - 4, y - 10, 3, 5, C_VINHO_CLR);
    gfx_rect(x + 2, y - 10, 3, 5, C_VINHO_CLR);
}
// ---- Abrir: o aviso que desce e o card depois -----------------------------------
#define CQ_FILA 16
static uint8_t cq_avisos[CQ_FILA];       // os avisos que esperam para descer, um de cada vez
static int     n_cq_avisos;
static float   t_cq_aviso;
static uint8_t cq_cards[CQ_FILA];        // os cards que esperam a proxima tela de novidades
static int     n_cq_cards;
static char    cq_motivo[CQ_N][80];      // o que aconteceu, com os numeros ("": so o criterio)
static uint8_t cq_dados_de[CQ_N];        // bit k: o seu k-esimo dado da sequencia contou
static uint8_t cq_rod[CQ_FILA];          // as que abriram nesta rodada: o cartao do fim
static int     n_cq_rod;
static uint8_t cq_brilho;                // os seus dados que contaram nesta rodada
static bool    cq_em_rodada;             // a mesa esta numa rodada do Desafiante
static int bolsa_nova;

// Uma run nova (ou retomada) comeca sem avisos nem cards de antes: os de
// outra run, ou de outro perfil, ja foram vistos ou estao na Colecao.
static void cq_esquece(void) { n_cq_avisos = n_cq_cards = n_cq_rod = 0; t_cq_aviso = 0; cq_brilho = 0; cq_em_rodada = false; }
static bool cq_vale(void) { return modo == M_DESAFIO && perfil_atual >= 0 && perfis[perfil_atual].nome[0]; }

// O motivo: cada conquista que abre guarda o que aconteceu de fato, com os
// numeros ("3-3-3 na sua sequência"), e os seus dados que contaram. O aviso
// que desce mostra o motivo e os dados brilham na mesa; no fim da rodada, o
// cartao (F_RESULTADO) lista as que abriram nela e espera o clique; o card
// depois da partida mostra de novo.
static void cq_abre_m(int c, uint8_t dados, const char *fmt, ...)
{
    if (!cq_vale() || c < 0 || c >= CQ_N || cq_aberta(c)) return;
    cq_motivo[c][0] = 0;
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(cq_motivo[c], sizeof cq_motivo[c], fmt, ap);
        va_end(ap);
    }
    cq_dados_de[c] = dados;
    perfis[perfil_atual].conq |= (uint64_t)1 << c;
    perfis_salva();
    if (n_cq_avisos < CQ_FILA) cq_avisos[n_cq_avisos++] = (uint8_t)c;
    if (n_cq_cards < CQ_FILA) cq_cards[n_cq_cards++] = (uint8_t)c;
    if (cq_em_rodada && n_cq_rod < CQ_FILA) { cq_rod[n_cq_rod++] = (uint8_t)c; cq_brilho |= dados; }
    if (n_cq_avisos == 1) { t_cq_aviso = 0; som_toca(SOM_NOVIDADE); }
    reg_conquista(cq_nome(c), CONQ[c].texto, cq_motivo[c]);
}

static inline void cq_abre(int c) { cq_abre_m(c, 0, NULL); }   // sem motivo (os testes)

// A rodada: comeca limpa (nova_rodada) e, montada a mesa, passa a juntar as
// que abrem nela; a partida que acaba nao conta (as da partida tem o card).
static void cq_rodada_limpa(void) { n_cq_rod = 0; cq_brilho = 0; cq_em_rodada = false; }
static void cq_rodada_abre(void) { cq_em_rodada = modo == M_DESAFIO; }
// Os seus dados que brilham agora: os da conquista do aviso que esta descendo
// e, no fim da rodada, os de todas as que abriram nela.
static uint8_t cq_brilho_agora(void)
{
    if (!cq_em_rodada || modo != M_DESAFIO) return 0;
    if (fase == F_RESULTADO) return cq_brilho;
    return n_cq_avisos && bolsa_nova < 0 ? cq_dados_de[cq_avisos[0]] : 0;
}

// O passo do aviso (bolsas_passo): espera o da bolsa nova descer primeiro.
#define CQ_AVISO_T 3.6f
static void cq_passo(float dt)
{
    if (n_cq_avisos && fase == F_NOVIDADES) { n_cq_avisos = 0; return; }   // os cards ja mostram
    if (!n_cq_avisos || bolsa_nova >= 0) return;
    t_cq_aviso += dt;
    if (t_cq_aviso < CQ_AVISO_T) return;
    memmove(cq_avisos, cq_avisos + 1, (size_t)(n_cq_avisos - 1));
    n_cq_avisos--;
    t_cq_aviso = 0;
    if (n_cq_avisos) som_toca(SOM_NOVIDADE);
}

// O desenho do que a conquista abriu (o dado, o amuleto ou o adesivo), com
// raio r, no lugar do "?": o aviso e o cartao do fim da rodada.
static void cq_icone(int c, int cx, int cy, int r)
{
    int it = CONQ[c].item;
    switch (CONQ[c].k) {
    case CQK_AMULETO: desenha_amuleto(cx, cy, r, it, false); break;
    case CQK_ADESIVO: ad_solto_d(cx, cy, it, r >= 10 ? 2 : 1, 0); break;
    default:          desenha_dado((float)cx, (float)cy, (float)r, it, 0, 0, 0, 0, false); break;
    }
}

// O aviso que desce do alto: "Conquista! Jackpot", com o desenho do item,
// o que aconteceu ("3-3-3 na sua sequência") e o que se ganhou.
static const char *const CQ_TIPO_NOVO[3] = { "um dado novo na Coleção", "um amuleto novo na Coleção", "um adesivo novo na Coleção" };
static void cq_aviso_desenha(void)
{
    if (!n_cq_avisos || bolsa_nova >= 0 || modo != M_DESAFIO || cq_cartao_visivel()) return;   // (o cartao do fim da rodada ja mostra)
    int c = cq_avisos[0];
    float t = t_cq_aviso;
    float q = t < 0.35f ? sai_mola(t / 0.35f) : t > CQ_AVISO_T - 0.3f ? (CQ_AVISO_T - t) / 0.3f : 1;
    char s[80];
    snprintf(s, sizeof s, "Conquista! %s", cq_nome(c));
    const char *mot = cq_motivo[c], *sub = CQ_TIPO_NOVO[CONQ[c].k];
    int ws = gfx_largura(s, 1), wb = gfx_largura(sub, GFX_FINO), wm = mot[0] ? gfx_largura(mot, 1) : 0;
    int larg = ws > wb ? ws : wb;
    if (wm > larg) larg = wm;
    int w = larg + 64, h = mot[0] ? 52 : 40, x, y;
    aviso_alto_lugar(w, h, q, &x, &y);
    ui_cartao_aceso(x, y, w, h, true, C_OURO);
    cq_icone(c, x + 22, y + h / 2, 12);
    txt(x + 44, y + 3, s, C_OURO, true);
    if (mot[0]) txt(x + 44, y + 17, mot, C_MARFIM, false);
    gfx_texto(x + 44, y + (mot[0] ? 31 : 17), sub, C_MARFIM_S, GFX_FINO, false);
}

// No fim da rodada (F_RESULTADO), as conquistas que abriram nela: um cartao
// parado ate o clique, entre as duas filas (os seus dados que contaram ficam
// a vista, brilhando), cada uma com o item, o que se ganhou e por que - o que
// aconteceu, com os numeros, e o criterio. Com tres ou mais, sem o criterio;
// passando de tres, as outras ficam para o card do fim da partida.
static bool cq_cartao_visivel(void) { return fase == F_RESULTADO && n_cq_rod && modo == M_DESAFIO && t_fase >= 0.25f; }
static void cq_cartao_rodada(void)
{
    if (!cq_cartao_visivel()) return;
    int n = n_cq_rod < 3 ? n_cq_rod : 3;
    bool curto = n_cq_rod >= 3;                   // sem a linha do criterio
    int lh = curto ? 26 : 37, larg = gfx_largura("CONQUISTAS NESTA RODADA", GFX_FINO) + 90;
    for (int i = 0; i < n; i++) {
        int c = cq_rod[i];
        char s[96];
        snprintf(s, sizeof s, "%s  ·  %s", cq_nome(c), CQ_TIPO_NOVO[CONQ[c].k]);
        int a = gfx_largura(s, 1) + 2, b = gfx_largura(cq_motivo[c][0] ? cq_motivo[c] : CONQ[c].texto, 1);
        int d = curto || !cq_motivo[c][0] ? 0 : gfx_largura(CONQ[c].texto, GFX_FINO);
        if (a > larg) larg = a;
        if (b > larg) larg = b;
        if (d > larg) larg = d;
    }
    int w = larg + 52, h = 22 + n * lh + 8 + (n_cq_rod > n ? 11 : 0);
    if (w > MESA_X1 - MESA_X0 - 16) w = MESA_X1 - MESA_X0 - 16;
    int x = GFX_W / 2 - w / 2, y = LINHA_CIMA + R_DADO + 8 + ui_entra(t_fase, 0.25f, 10);
    ui_cartao_aceso(x, y, w, h, true, C_OURO);
    gfx_texto(x + 10, y + 7, n_cq_rod > 1 ? "CONQUISTAS NESTA RODADA" : "CONQUISTA NESTA RODADA", C_OURO, GFX_FINO, false);
    int tw = linha_teclas(0, y + 6, "ENTER segue", C_TEXTO_M, false);
    linha_teclas(x + w - 10 - tw, y + 6, "ENTER segue", C_TEXTO_M, true);
    for (int i = 0; i < n; i++) {
        int c = cq_rod[i], ly = y + 22 + i * lh, my = ly + (curto ? 11 : 16);
        cq_icone(c, x + 20, my, 10);
        char s[96];
        snprintf(s, sizeof s, "%s  ·  %s", cq_nome(c), CQ_TIPO_NOVO[CONQ[c].k]);
        txt(x + 36, ly, s, C_OURO, true);
        txt(x + 36, ly + 12, cq_motivo[c][0] ? cq_motivo[c] : CONQ[c].texto, C_MARFIM, false);
        if (!curto && cq_motivo[c][0]) gfx_texto(x + 36, ly + 25, CONQ[c].texto, C_MARFIM_S, GFX_FINO, false);
    }
    if (n_cq_rod > n) {
        char s[48];
        snprintf(s, sizeof s, "e mais %d: no card do fim da partida", n_cq_rod - n);
        gfx_texto(x + 36, y + 22 + n * lh, s, C_MARFIM_S, GFX_FINO, false);
    }
}

// ---- Os feitos --------------------------------------------------------------
// "Tire N": o numero do lance, ou depois de adesivo e poder; o dado que nao
// rola (o Quebrado, o Fantasma, o Previsivel...) nao conta.
static bool cq_rola(int t)
{
    int ef = TIPO[t].efeito;
    return ef != EF_QUEBRADO && ef != EF_FANTASMA && ef != EF_PREVISIVEL && ef != EF_RELOGIO && ef != EF_DIGITAL
        && ef != EF_FICHA_CORINGA && ef != EF_UNIVERSO && ef != EF_CARROSSEL;
}

// A partida: o que se conta ate o fim dela (cq_partida_fim soma no perfil).
static int  cq_min_fichas, cq_seguidas, cq_rod_partida, cq_cheias_partida;
static bool cq_por1_v, cq_por1_p, cq_viuva_partida, cq_atras_ultima;
static int  cq_por1[4];                  // os placares: a vitoria por 1 (seu, dele) e a derrota por 1 (o motivo do Sorte/Reves)
static int  cq_compras_loja;            // o que ja se comprou nesta loja (Cartola)
static void cq_partida_comeca(void)
{
    cq_min_fichas = 1 << 20;
    cq_seguidas = cq_rod_partida = cq_cheias_partida = 0;
    cq_por1_v = cq_por1_p = cq_viuva_partida = cq_atras_ultima = false;
}

// Os numeros de uma mao, em ordem de fila: "3-3-3", "2-3-4".
static void cq_numeros(char *s, size_t n, const uint8_t *k, int q)
{
    size_t u = 0;
    s[0] = 0;
    for (int i = 0; i < q && u + 6 < n; i++) {
        int w = snprintf(s + u, n - u, i ? "-%d" : "%d", F[0].valor[k[i]]);
        if (w > 0) u += (size_t)w;
    }
}
// Os dados de uma mascara, em ordem.
static int cq_da_mascara(uint8_t m, uint8_t *k)
{
    int q = 0;
    for (int i = 0; i < N_FILA; i++) if (m >> i & 1) k[q++] = (uint8_t)i;
    return q;
}
static void cq_numeros_m(char *s, size_t n, uint8_t m)
{
    uint8_t k[N_FILA];
    cq_numeros(s, n, k, cq_da_mascara(m, k));
}

// Os "tire" da sua sequencia: os numeros que os seus dados que rolam mostram
// agora, valendo ou nao. Conta no pouso de cada dado (passo.inc: o aviso na
// hora, com o Oculto ainda escondido de fora - ninguem viu o numero dele) e de
// novo no fim da rodada (com o que a mesa mostra entao). Aberta no pouso, um
// Terremoto depois nao tira mais.
static void cq_tirou(bool pula_escondido)
{
    const fila_t *f = &F[0];
    int n = f->lancados < N_FILA ? f->lancados : N_FILA;
    uint8_t de[64] = { 0 };                           // os dados que tiraram cada numero
    int cont[64] = { 0 };
    bool rola[N_FILA] = { false };
    for (int k = 0; k < n; k++) {
        int v = f->valor[k];
        rola[k] = cq_rola(f->tipo[k]) && !(pula_escondido && f->escondido[k]);
        if (!rola[k] || v < 0 || v >= 64) continue;
        cont[v]++;
        de[v] |= (uint8_t)(1u << k);
    }
    char s[48];
    if (cont[1] >= 4) { cq_numeros_m(s, sizeof s, de[1]); cq_abre_m(CQ_AGOURO, de[1], "%s na sua sequência", s); }
    if (cont[6] >= 3) { cq_numeros_m(s, sizeof s, de[6]); cq_abre_m(CQ_MALDITO, de[6], "%s na sua sequência", s); }
    if (cont[3] >= 3) { cq_numeros_m(s, sizeof s, de[3]); cq_abre_m(CQ_AGNUS, de[3], "%s na sua sequência", s); }
    for (int v = 0; v < 64; v++)
        if (cont[v] >= 4) { cq_numeros_m(s, sizeof s, de[v]); cq_abre_m(CQ_ECO, de[v], "%s: o mesmo número em quatro dados", s); break; }
    for (int k = 0; k + 2 < n; k++) {                 // em ordem, vizinhos na fila: 2-3-4 e a escada de quatro
        int q = 1;
        while (k + q < n && rola[k + q - 1] && rola[k + q] && f->valor[k + q] == f->valor[k + q - 1] + 1) q++;
        if (q < 3) continue;
        uint8_t m = (uint8_t)(((1u << q) - 1) << k);
        cq_numeros_m(s, sizeof s, m);
        cq_abre_m(CQ_DOMINO, m, "%s: seguidos e em ordem", s);
        if (q >= 4) cq_abre_m(CQ_DIGITAL, m, "%s: uma escada de %d", s, q);
    }
}

// Um dado pousou (passo.inc): os "tire" da sua sequencia, na hora.
static void cq_pousou(void)
{
    if (!cq_vale() || !cq_em_rodada) return;
    cq_tirou(true);
}

// O fim de uma rodada do Desafiante: vencida (w == 0) no fim ou porque o
// oponente correu (correu), com o pote que voce levou. Vale tudo o que se ve
// na sua fila.
static void cq_rodada(int w, int pote_levado, bool correu)
{
    if (!cq_vale()) return;
    bool venci = w == 0;
    const fila_t *f = &F[0];
    int n = f->lancados < N_FILA ? f->lancados : N_FILA;
    int val = 0, val6 = 0, tam = 0, cont_v[64] = { 0 }, max_v = 0;
    uint8_t de_v[64] = { 0 }, m_val = 0, m_val6 = 0, m_max = 0;
    uint32_t tamanhos = 0;
    for (int k = 0; k < n; k++) {
        int t = f->tipo[k], v = f->valor[k];
        bool rola = cq_rola(t), valido = f->est[k] == V_VALIDO;
        if (!valido) continue;
        val++;
        m_val |= (uint8_t)(1u << k);
        int l = TIPO[t].lados;
        if (l == 6) { val6++; m_val6 |= (uint8_t)(1u << k); }
        if (l >= 0 && l < 32 && !(tamanhos >> l & 1)) { tamanhos |= 1u << l; tam++; }
        if (rola && v >= 0 && v < 64) { cont_v[v]++; de_v[v] |= (uint8_t)(1u << k); }
        if (rola && l >= 6 && v >= l) { max_v++; m_max |= (uint8_t)(1u << k); }
    }
    // As maos de dados valendo: iguais, pares, o maximo.
    int pares = 0;
    uint8_t m_pares = 0;
    char s[48];
    for (int v = 0; v < 64; v++) {
        if (cont_v[v] >= 2) { pares++; m_pares |= de_v[v]; }
        if (cont_v[v] >= 4) pares++;                  // a quadra vale dois pares
    }
    for (int v = 0; v < 64; v++)
        if (cont_v[v] >= 3) { cq_numeros_m(s, sizeof s, de_v[v]); cq_abre_m(CQ_JACKPOT, de_v[v], "%s em dados válidos", s); break; }
    if (cont_v[2] >= 3) { cq_numeros_m(s, sizeof s, de_v[2]); cq_abre_m(CQ_PATINHOS, de_v[2], "%s em dados válidos", s); }
    if (pares >= 2) { cq_numeros_m(s, sizeof s, m_pares); cq_abre_m(CQ_GEMEO, m_pares, "%s: dois pares válidos", s); }
    if (max_v >= 2) { cq_numeros_m(s, sizeof s, m_max); cq_abre_m(CQ_EXPLOSIVO, m_max, "%s: o número máximo em %d dados", s, max_v); }
    cq_tirou(false);                                  // os "tire": de novo, com a mesa de agora
    // As de rodada vencida.
    if (venci && val >= 4 && tam >= 4) {
        char t[48] = "";                              // os tamanhos: "d4, d6, d8 e d12"
        size_t u = 0;
        for (int l = 0, i = 0; l < 32; l++)
            if (tamanhos >> l & 1) {
                int w = snprintf(t + u, sizeof t - u, "%sd%d", i == 0 ? "" : i == tam - 1 ? " e " : ", ", l);
                if (w > 0 && (size_t)w < sizeof t - u) u += (size_t)w;
                i++;
            }
        cq_abre_m(CQ_PRISMA, m_val, "venceu com %s válidos", t);
    }
    if (venci && val6 >= 4) cq_abre_m(CQ_PESADO, m_val6, "venceu com %d dados de seis lados válidos", val6);
    if (venci && val == 1) cq_viuva_partida = true;
    if (venci && res_antes[0] == 0 && pote_levado >= CQ_TUDO_POTE)
        cq_abre_m(CQ_TUDO_NADA, 0, "apostou todas as fichas e levou um pote de %d", pote_levado);
    if (venci && pote_levado >= CQ_POTE_FICHAS) cq_abre_m(CQ_POTE_QUENTE, 0, "levou um pote de %d fichas", pote_levado);
    cq_seguidas = venci ? cq_seguidas + 1 : 0;
    if (cq_seguidas >= CQ_TACA_SEGUIDAS) cq_abre_m(CQ_TACA, 0, "%d rodadas seguidas vencidas nesta partida", cq_seguidas);
    // As que comparam as duas filas: so com as duas na mesa, sem ninguem correr.
    if (!correu) {
        int a = F[0].total, b = F[1].total;
        if (a >= CQ_ESTRELA_PONTOS) cq_abre_m(CQ_ESTRELA, m_val, "%d pontos nesta rodada", a);
        // A Moeda sorte: vencer com 3 pontos ou menos, tendo lancado (quando
        // o oponente estoura antes, voce vence com 0 sem jogar: nao conta).
        if (venc_mao == 0 && f->lancados > 0 && a <= CQ_MOEDA_PONTOS) cq_abre_m(CQ_MOEDA_SORTE, m_val, "venceu com %d a %d", a, b);
        if (venc_mao == 0 && a - b >= CQ_DINAMITE_DIF) cq_abre_m(CQ_DINAMITE, m_val, "venceu por %d: %d a %d", a - b, a, b);
        if (a == b && a >= CQ_ESPELHO_PONTOS) cq_abre_m(CQ_ESPELHO, m_val, "empatou em %d a %d", a, b);   // o empate (o Crupie leva o dele, mas empatou)
        if (venc_mao == 0 && a == b + 1) { cq_por1_v = true; cq_por1[0] = a; cq_por1[1] = b; }
        if (venc_mao == 1 && b == a + 1) { cq_por1_p = true; cq_por1[2] = a; cq_por1[3] = b; }
        if (cq_por1_v && cq_por1_p)                   // o motivo conta as duas metades, nao so a desta rodada
            cq_abre_m(CQ_SORTE_REVES, 0, "venceu uma rodada por %d a %d e perdeu outra por %d a %d",
                      cq_por1[0], cq_por1[1], cq_por1[2], cq_por1[3]);
        if (resolvido.ad_agiram[0] >= CQ_PURPURINA_AD)
            cq_abre_m(CQ_PURPURINA, 0, "%d adesivos seus agiram nesta rodada", resolvido.ad_agiram[0]);
        uint8_t anulei = 0;                           // dados dele que os meus anularam (e ficaram anulados)
        for (int i = 0; i < n_ev; i++)
            if (ev[i].tipo == EV_ANULA && ev[i].de_j == 0 && ev[i].alvo_j == 1 && ev[i].alvo_k < N_FILA
                && resolvido.est[1][ev[i].alvo_k] == V_ANULADO) anulei |= (uint8_t)(1u << ev[i].alvo_k);
        if (cq_bits(anulei) >= CQ_MIRA_DADOS) cq_abre_m(CQ_MIRA, 0, "seus dados anularam %d dados do oponente", cq_bits(anulei));
    }
    // O que a partida guarda para o fim.
    cq_rod_partida++;
    if (val >= FILA_NORMAL) cq_cheias_partida++;
    if (J[0].fichas < cq_min_fichas) cq_min_fichas = J[0].fichas;
    cq_atras_ultima = fichas_ini[0] < fichas_ini[1];
}

// A pedra negativa se partiu na sua mao: o Dado Misterioso (partida.inc).
static void cq_misterioso(void) { cq_abre_m(CQ_MISTERIOSO, 0, "jogou um Quebrado de número negativo"); }

// O fim da partida (des_resultado): os feitos de partida e os contadores.
static void cq_partida_fim(const no_t *no, bool venceu)
{
    if (!cq_vale()) return;
    perfil_t *p = &perfis[perfil_atual];
    p->cq_rodadas = (uint16_t)(p->cq_rodadas + cq_rod_partida < 60000 ? p->cq_rodadas + cq_rod_partida : 60000);
    p->cq_cheias = (uint16_t)(p->cq_cheias + cq_cheias_partida < 60000 ? p->cq_cheias + cq_cheias_partida : 60000);
    if (cq_viuva_partida && p->cq_viuva < 250) p->cq_viuva++;
    if (!venceu && no->tipo == NO_CHEFE && p->cq_derrotas < 250) p->cq_derrotas++;   // Furia
    if (venceu) {
        bool chefe = no->tipo == NO_CHEFE;
        int c = chefe ? run.andar : -1;          // 0 o Crupie, 1 a Dona, 2 o Barao (ou o Papai Noel), 3 o Coringa
        if (p->cq_vitorias < 60000) p->cq_vitorias++;
        if (no->perso < 32) p->cq_oponentes |= (uint32_t)1 << no->perso;
        int e = cq_encontro(no->perso);
        if (e >= 0 && p->cq_encontro[e] < 250) p->cq_encontro[e]++;
        if (c == CH_CRUPIE && run.bolsa < 32) p->cq_crupie |= (uint32_t)1 << run.bolsa;
        const char *quem = no->perso < sizeof PERSO / sizeof PERSO[0] ? PERSO[no->perso].nome : "o chefe";
        if (chefe && J[0].perdidas == 0) cq_abre_m(CQ_DESAFIANTE, 0, "venceu %s sem perder nem correr de nenhuma rodada", quem);
        if (c == CH_BARAO && J[0].perdidas == 0) cq_abre_m(CQ_TRONO, 0, "venceu %s sem perder nem correr de nenhuma rodada", quem);
        if (c == CH_CORINGA) cq_abre_m(CQ_CORINGA, 0, "venceu %s", quem);
        if (cq_min_fichas <= CQ_FENIX_FICHAS) cq_abre_m(CQ_FENIX, 0, "chegou a %d fichas e venceu a partida", cq_min_fichas);
        if (J[0].perdidas >= CQ_RANCOR_PERDAS) cq_abre_m(CQ_RANCOR, 0, "perdeu %d rodadas e venceu a partida", J[0].perdidas);
        if (chefe && cq_rod_partida <= CQ_BOLSO_RODADAS) cq_abre_m(CQ_RELOGIO_BOLSO, 0, "venceu %s em %d rodadas", quem, cq_rod_partida);
        if (chefe && cq_atras_ultima) cq_abre_m(CQ_RELOGIO, 0, "venceu %s: estava atrás antes da última rodada", quem);
    }
    for (int k = 0; k < CQ_N; k++)               // os contadores que fecharam
        if (CONQ[k].alvo && cq_conta_de(p, k) >= CONQ[k].alvo)
            cq_abre_m(k, 0, "chegou a %d de %d", cq_conta_de(p, k), CONQ[k].alvo);
    perfis_salva();
}

// A bolsa e as moedas, a qualquer hora (bolsas_confere).
static void cq_confere(void)
{
    if (!cq_vale()) return;
    int ad = 0, max_um = 0;
    uint8_t em[AD_N] = { 0 };                     // em quantos dados cada adesivo esta
    for (int i = 0; i < run.n_col; i++) {
        const peca_t *pc = &run.col[i];
        int q = ad_quantos(pc);
        ad += q;
        if (q > max_um) max_um = q;
        bool tem[AD_N] = { false };
        for (int l = 0; l < AD_LADOS; l++) {
            int a = pc->ad[l];
            if (a > AD_NADA && a < AD_N && !tem[a]) { tem[a] = true; if (em[a] < 250) em[a]++; }
        }
    }
    if (run.n_col >= CQ_COLECAO_DADOS) cq_abre_m(CQ_COLECIONADOR, 0, "%d dados na sua bolsa", run.n_col);
    if (ad >= CQ_MOSAICO_AD) cq_abre_m(CQ_MOSAICO, 0, "%d adesivos colados na sua bolsa", ad);
    if (max_um >= CQ_TELA_LADOS) cq_abre_m(CQ_TELA, 0, "um dado seu com adesivo em %d lados", max_um);
    if (max_um >= CQ_ADESIVADO_AD) cq_abre_m(CQ_ADESIVADO, 0, "um dado seu com %d adesivos", max_um);
    for (int a = AD_NADA + 1; a < AD_N; a++)
        if (em[a] >= CQ_INFECTADO_DADOS) { cq_abre_m(CQ_INFECTADO, 0, "%s em %d dados da bolsa", ADESIVO[a].nome, em[a]); break; }
    if (run.moedas >= CQ_DOURADO_MOEDAS) cq_abre_m(CQ_DOURADO, 0, "juntou %d moedas", (int)run.moedas);
    if (run.gasto >= CQ_CUPOM_GASTO) cq_abre_m(CQ_CUPOM, 0, "gastou %d moedas nesta run", run.gasto);
}

// A loja: o que se comprou nela (dado, adesivo ou pacote) e o adesivo colado.
static void cq_loja_abriu(void) { cq_compras_loja = 0; }
static void cq_comprou(void)
{
    if (++cq_compras_loja >= CQ_CARTOLA_COISAS) cq_abre_m(CQ_CARTOLA, 0, "comprou %d coisas nesta loja", cq_compras_loja);
    cq_confere();
}
static void cq_colou(void)
{
    if (!cq_vale()) return;
    perfil_t *p = &perfis[perfil_atual];
    if (p->cq_colados < 60000) p->cq_colados++;
    if (p->cq_colados >= CONQ[CQ_FIGURINHA].alvo) cq_abre_m(CQ_FIGURINHA, 0, "colou %d adesivos, somando as suas runs", p->cq_colados);
    else perfis_salva();
}
