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

// Uma run nova (ou retomada) comeca sem avisos nem cards de antes: os de
// outra run, ou de outro perfil, ja foram vistos ou estao na Colecao.
static void cq_esquece(void) { n_cq_avisos = n_cq_cards = 0; t_cq_aviso = 0; }
static bool cq_vale(void) { return modo == M_DESAFIO && perfil_atual >= 0 && perfis[perfil_atual].nome[0]; }
static void cq_abre(int c)
{
    if (!cq_vale() || c < 0 || c >= CQ_N || cq_aberta(c)) return;
    perfis[perfil_atual].conq |= (uint64_t)1 << c;
    perfis_salva();
    if (n_cq_avisos < CQ_FILA) cq_avisos[n_cq_avisos++] = (uint8_t)c;
    if (n_cq_cards < CQ_FILA) cq_cards[n_cq_cards++] = (uint8_t)c;
    if (n_cq_avisos == 1) { t_cq_aviso = 0; som_toca(SOM_NOVIDADE); }
}

// O passo do aviso (bolsas_passo): espera o da bolsa nova descer primeiro.
#define CQ_AVISO_T 3.6f
static int bolsa_nova;
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

// O aviso que desce do alto: "Conquista! Jackpot", com o "?" de latao (o
// desenho do item so aparece no card, depois da partida).
static const char *const CQ_TIPO_NOVO[3] = { "um dado novo na Coleção", "um amuleto novo na Coleção", "um adesivo novo na Coleção" };
static void cq_aviso_desenha(void)
{
    if (!n_cq_avisos || bolsa_nova >= 0 || modo != M_DESAFIO) return;
    int c = cq_avisos[0];
    float t = t_cq_aviso;
    float q = t < 0.35f ? sai_mola(t / 0.35f) : t > CQ_AVISO_T - 0.3f ? (CQ_AVISO_T - t) / 0.3f : 1;
    char s[80];
    snprintf(s, sizeof s, "Conquista! %s", cq_nome(c));
    const char *sub = CQ_TIPO_NOVO[CONQ[c].k];
    int ws = gfx_largura(s, 1), wb = gfx_largura(sub, GFX_FINO);
    int w = (ws > wb ? ws : wb) + 64, h = 40, x = GFX_W / 2 - w / 2, y = -h + (int)((h + FAIXA_H + 6) * q);
    ui_cartao_aceso(x, y, w, h, true, C_OURO);
    gfx_disco(x + 22, y + h / 2, 14, C_LATAO_ESC);
    gfx_disco(x + 22, y + h / 2, 11, C_FELTRO_ESC);
    txt_c(x + 22, y + h / 2 - TXT_H / 2, "?", C_OURO, true);
    txt(x + 44, y + 3, s, C_OURO, true);
    gfx_texto(x + 44, y + 17, sub, C_MARFIM_S, GFX_FINO, false);
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
static int  cq_compras_loja;            // o que ja se comprou nesta loja (Cartola)
static void cq_partida_comeca(void)
{
    cq_min_fichas = 1 << 20;
    cq_seguidas = cq_rod_partida = cq_cheias_partida = 0;
    cq_por1_v = cq_por1_p = cq_viuva_partida = cq_atras_ultima = false;
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
    int val = 0, val6 = 0, tam = 0, cont_v[64] = { 0 }, cont_l[64] = { 0 }, max_v = 0;
    uint32_t tamanhos = 0;
    for (int k = 0; k < n; k++) {
        int t = f->tipo[k], v = f->valor[k];
        bool rola = cq_rola(t), valido = f->est[k] == V_VALIDO;
        if (rola && v >= 0 && v < 64) cont_l[v]++;
        if (!valido) continue;
        val++;
        int l = TIPO[t].lados;
        if (l == 6) val6++;
        if (l >= 0 && l < 32 && !(tamanhos >> l & 1)) { tamanhos |= 1u << l; tam++; }
        if (rola && v >= 0 && v < 64) cont_v[v]++;
        if (rola && l >= 6 && v >= l) max_v++;
    }
    // As maos de dados: iguais, pares, seguidos, o maximo.
    int pares = 0;
    bool trinca = false, quadra_l = false;
    for (int v = 0; v < 64; v++) {
        pares += cont_v[v] >= 2;
        trinca |= cont_v[v] >= 3;
        quadra_l |= cont_l[v] >= 4;
        if (cont_v[v] >= 4) pares++;                  // a quadra vale dois pares
    }
    if (trinca) cq_abre(CQ_JACKPOT);
    if (cont_v[2] >= 3) cq_abre(CQ_PATINHOS);
    if (pares >= 2) cq_abre(CQ_GEMEO);
    if (cont_l[1] >= 4) cq_abre(CQ_AGOURO);
    if (cont_l[6] >= 3) cq_abre(CQ_MALDITO);
    if (cont_l[3] >= 3) cq_abre(CQ_AGNUS);
    if (quadra_l) cq_abre(CQ_ECO);
    if (max_v >= 2) cq_abre(CQ_EXPLOSIVO);
    for (int k = 0; k + 2 < n; k++) {                 // em ordem, vizinhos na fila: 2-3-4 e a escada de quatro
        int s = 1;
        while (k + s < n && cq_rola(f->tipo[k + s - 1]) && cq_rola(f->tipo[k + s]) && f->valor[k + s] == f->valor[k + s - 1] + 1) s++;
        if (s >= 3) cq_abre(CQ_DOMINO);
        if (s >= 4) cq_abre(CQ_DIGITAL);
    }
    // As de rodada vencida.
    if (venci && val >= 4 && tam >= 4) cq_abre(CQ_PRISMA);
    if (venci && val6 >= 4) cq_abre(CQ_PESADO);
    if (venci && val == 1) cq_viuva_partida = true;
    if (venci && res_antes[0] == 0 && pote_levado >= CQ_TUDO_POTE) cq_abre(CQ_TUDO_NADA);
    if (venci && pote_levado >= CQ_POTE_FICHAS) cq_abre(CQ_POTE_QUENTE);
    cq_seguidas = venci ? cq_seguidas + 1 : 0;
    if (cq_seguidas >= CQ_TACA_SEGUIDAS) cq_abre(CQ_TACA);
    // As que comparam as duas filas: so com as duas na mesa, sem ninguem correr.
    if (!correu) {
        int a = F[0].total, b = F[1].total;
        if (a >= CQ_ESTRELA_PONTOS) cq_abre(CQ_ESTRELA);
        // A Moeda sorte: vencer com 3 pontos ou menos, tendo lancado (quando
        // o oponente estoura antes, voce vence com 0 sem jogar: nao conta).
        if (venc_mao == 0 && f->lancados > 0 && a <= CQ_MOEDA_PONTOS) cq_abre(CQ_MOEDA_SORTE);
        if (venc_mao == 0 && a - b >= CQ_DINAMITE_DIF) cq_abre(CQ_DINAMITE);
        if (a == b && a >= CQ_ESPELHO_PONTOS) cq_abre(CQ_ESPELHO);   // o empate (o Crupie leva o dele, mas empatou)
        if (venc_mao == 0 && a == b + 1) cq_por1_v = true;
        if (venc_mao == 1 && b == a + 1) cq_por1_p = true;
        if (cq_por1_v && cq_por1_p) cq_abre(CQ_SORTE_REVES);
        if (resolvido.ad_agiram[0] >= CQ_PURPURINA_AD) cq_abre(CQ_PURPURINA);
        uint8_t anulei = 0;                           // dados dele que os meus anularam (e ficaram anulados)
        for (int i = 0; i < n_ev; i++)
            if (ev[i].tipo == EV_ANULA && ev[i].de_j == 0 && ev[i].alvo_j == 1 && ev[i].alvo_k < N_FILA
                && resolvido.est[1][ev[i].alvo_k] == V_ANULADO) anulei |= (uint8_t)(1u << ev[i].alvo_k);
        if (cq_bits(anulei) >= CQ_MIRA_DADOS) cq_abre(CQ_MIRA);
    }
    // O que a partida guarda para o fim.
    cq_rod_partida++;
    if (val >= FILA_NORMAL) cq_cheias_partida++;
    if (J[0].fichas < cq_min_fichas) cq_min_fichas = J[0].fichas;
    cq_atras_ultima = fichas_ini[0] < fichas_ini[1];
}

// A pedra negativa se partiu na sua mao: o Dado Misterioso (partida.inc).
static void cq_misterioso(void) { cq_abre(CQ_MISTERIOSO); }

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
        if (chefe && J[0].perdidas == 0) cq_abre(CQ_DESAFIANTE);
        if (c == CH_BARAO && J[0].perdidas == 0) cq_abre(CQ_TRONO);
        if (c == CH_CORINGA) cq_abre(CQ_CORINGA);
        if (cq_min_fichas <= CQ_FENIX_FICHAS) cq_abre(CQ_FENIX);
        if (J[0].perdidas >= CQ_RANCOR_PERDAS) cq_abre(CQ_RANCOR);
        if (chefe && cq_rod_partida <= CQ_BOLSO_RODADAS) cq_abre(CQ_RELOGIO_BOLSO);
        if (chefe && cq_atras_ultima) cq_abre(CQ_RELOGIO);
    }
    for (int k = 0; k < CQ_N; k++)               // os contadores que fecharam
        if (CONQ[k].alvo && cq_conta_de(p, k) >= CONQ[k].alvo) cq_abre(k);
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
    if (run.n_col >= CQ_COLECAO_DADOS) cq_abre(CQ_COLECIONADOR);
    if (ad >= CQ_MOSAICO_AD) cq_abre(CQ_MOSAICO);
    if (max_um >= CQ_TELA_LADOS) cq_abre(CQ_TELA);
    if (max_um >= CQ_ADESIVADO_AD) cq_abre(CQ_ADESIVADO);
    for (int a = AD_NADA + 1; a < AD_N; a++) if (em[a] >= CQ_INFECTADO_DADOS) { cq_abre(CQ_INFECTADO); break; }
    if (run.moedas >= CQ_DOURADO_MOEDAS) cq_abre(CQ_DOURADO);
    if (run.gasto >= CQ_CUPOM_GASTO) cq_abre(CQ_CUPOM);
}

// A loja: o que se comprou nela (dado, adesivo ou pacote) e o adesivo colado.
static void cq_loja_abriu(void) { cq_compras_loja = 0; }
static void cq_comprou(void)
{
    if (++cq_compras_loja >= CQ_CARTOLA_COISAS) cq_abre(CQ_CARTOLA);
    cq_confere();
}
static void cq_colou(void)
{
    if (!cq_vale()) return;
    perfil_t *p = &perfis[perfil_atual];
    if (p->cq_colados < 60000) p->cq_colados++;
    if (p->cq_colados >= CONQ[CQ_FIGURINHA].alvo) cq_abre(CQ_FIGURINHA);
    else perfis_salva();
}
