// ---------------------------------------------------------------------------
// Perfis: ate 3 por aparelho. Cada um tem nome, avatar e o que liberou no
// Modo Desafiante (a dificuldade mais alta e o que vem com ela); cada um
// guarda a sua run.
// ---------------------------------------------------------------------------
#define N_PERFIS 3
enum { AV_ET, AV_BERRO, AV_NOMALISA, AV_INTERRUPTOR, AV_SAMURAI, AV_SUSTO, AV_BLUE, AV_MAICON, AV_N };
static const char *const NOME_AVATAR[AV_N] = { "ET Vilu", "O Berro", "Nomalisa", "Interruptor do Futuro",
                                               "Samurai Z", "Susto", "Blue", "Maicon" };

typedef struct {
    char     nome[12];                   // "" = lugar vazio
    uint8_t  avatar, nivel;              // nivel: a dificuldade mais alta liberada
    uint8_t  venceu[N_NIVEIS + 1];       // runs vencidas em cada dificuldade
    uint16_t runs;
    uint16_t partidas;                   // partidas jogadas no Desafiante
    uint8_t  marcos;                     // bit c: ja venceu o chefe do salao c (abre dados)
    uint8_t  chefes[N_NIVEIS + 1];       // bit CH_*: os chefes ja vencidos em cada dificuldade
    // As bolsas do comeco da run (desafio/bolsas.inc), com lugar para B_MAX
    // (as que vierem depois cabem sem mudar o salvamento):
    uint8_t  bolsa_ult;                  // a ultima escolhida (a escolha comeca nela)
    uint8_t  bolsa_rec[B_MAX];           // o maior nivel vencido com cada uma, +1 (0: nenhum)
    uint32_t bolsas;                     // bit b: a bolsa b abriu (o Saco de Pano, sempre)
    uint32_t bolsas_vistas;              // bit b: ja apareceu aberta na escolha (a nova chega com festa)
    // As conquistas (desafio/conquistas.inc): as que ja aconteceram e os
    // contadores das que somam, de run em run.
    uint64_t conq;                       // bit c: a conquista c aconteceu (o item dela abriu)
    uint32_t cq_oponentes;               // bit p: o oponente p ja foi vencido (Camaleao)
    uint32_t cq_crupie;                  // bit b: o Crupie ja caiu com a bolsa b (Camelo)
    uint16_t cq_vitorias;                // partidas vencidas (Acumulador)
    uint16_t cq_cheias;                  // rodadas terminadas com 4 dados validos (Puxa-fila)
    uint16_t cq_colados;                 // adesivos colados (Figurinha)
    uint16_t cq_rodadas;                 // rodadas jogadas (Ampulheta)
    uint8_t  cq_viuva;                   // partidas com uma rodada vencida com 1 dado valido so (Viuva Negra)
    uint8_t  cq_encontro[6];             // vitorias contra cada oponente de encontro (CQ_ENCONTROS)
    uint8_t  cq_derrotas;                // partidas perdidas para um chefe (Furia)
    uint8_t  legado[8];                  // bit i: o item LEGADO[i] ja estava aberto pela regra de antes (fica aberto)
    uint8_t  cq_reserva[16];             // para as proximas, sem mudar o salvamento
} perfil_t;
typedef struct {                         // "PER7": de antes das conquistas, lido e convertido
    char     nome[12];
    uint8_t  avatar, nivel;
    uint8_t  venceu[N_NIVEIS + 1];
    uint16_t runs;
    uint16_t partidas;
    uint8_t  marcos;
    uint8_t  chefes[N_NIVEIS + 1];
    uint8_t  bolsa_ult;
    uint8_t  bolsa_rec[B_MAX];
    uint32_t bolsas;
    uint32_t bolsas_vistas;
} perfil_7_t;
typedef struct {                         // "PER6": as 8 primeiras bolsas, em 8 bits; lido e convertido
    char     nome[12];
    uint8_t  avatar, nivel;
    uint8_t  venceu[N_NIVEIS + 1];
    uint16_t runs;
    uint16_t partidas;
    uint8_t  marcos;
    uint8_t  chefes[N_NIVEIS + 1];
    uint8_t  bolsas, bolsas_vistas, bolsa_ult;
    uint8_t  bolsa_rec[8];
} perfil_6_t;
typedef struct {                         // "PER5": de antes das bolsas, lido e convertido
    char     nome[12];
    uint8_t  avatar, nivel;
    uint8_t  venceu[N_NIVEIS + 1];
    uint16_t runs;
    uint16_t partidas;
    uint8_t  marcos;
    uint8_t  chefes[N_NIVEIS + 1];
} perfil_5_t;
typedef struct {                         // "PER4": de antes dos chefes, lido e convertido
    char     nome[12];
    uint8_t  avatar, nivel;
    uint8_t  venceu[N_NIVEIS + 1];
    uint16_t runs;
    uint16_t partidas;
    uint8_t  marcos;
} perfil_4_t;
typedef struct {                         // "PER3": do tempo dos 5 niveis, lido e convertido
    char     nome[12];
    uint8_t  avatar, nivel;
    uint8_t  venceu[N_NIVEIS_V + 1];
    uint16_t runs;
    uint16_t partidas;
    uint8_t  marcos;
} perfil_v_t;
static perfil_t perfis[N_PERFIS];
static int perfil_atual = -1;
static bool perfis_lidos;

static void perfis_le(void);
static bool tem_perfil(void)
{
    perfis_le();
    for (int i = 0; i < N_PERFIS; i++) if (perfis[i].nome[0]) return true;
    return false;
}

static const char *chave_run(void)
{
    static const char *const K[N_PERFIS] = { "desafio", "desafio2", "desafio3" };
    return K[perfil_atual >= 0 && perfil_atual < N_PERFIS ? perfil_atual : 0];
}

static const perfil_t *perfil(void)
{
    static const perfil_t nenhum = { .nome = "VOCÊ" };
    return perfil_atual >= 0 && perfis[perfil_atual].nome[0] ? &perfis[perfil_atual] : &nenhum;
}

static void hex_de(const void *p, size_t n, char *out)
{
    const uint8_t *b = (const uint8_t *)p;
    for (size_t i = 0; i < n; i++) {
        out[2 * i]     = "0123456789abcdef"[b[i] >> 4];
        out[2 * i + 1] = "0123456789abcdef"[b[i] & 15];
    }
    out[2 * n] = 0;
}
static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1); }
static bool de_hex(const char *s, void *p, size_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (size_t i = 0; i < n; i++) {
        int a = hexv(s[2 * i]), c = hexv(s[2 * i + 1]);
        if (a < 0 || c < 0) return false;
        b[i] = (uint8_t)(a << 4 | c);
    }
    return true;
}

// "PER9" + perfil escolhido por ultimo + os tres perfis em hexadecimal.
static void perfis_salva(void)
{
    static char buf[8 + 2 * sizeof perfis + 1];
    memcpy(buf, "PER9", 4);
    buf[4] = (char)('0' + (perfil_atual < 0 ? 0 : perfil_atual));
    hex_de(perfis, sizeof perfis, buf + 5);
    salva_grava("perfis", buf);
}

// O texto inteiro guardado na chave. Quem leu pode ter cortado: o espaco de
// leitura e do tamanho do que esta versao grava, e um salvamento de uma
// versao mais nova (com mais coisa guardada) passa dele. NULL: sem memoria.
#define SALVO_MAX (1 << 20)
static char *salvo_inteiro(const char *chave)
{
    for (int n = 4096; n <= SALVO_MAX; n *= 2) {
        char *b = malloc((size_t)n);
        if (!b) return NULL;
        if (salva_le(chave, b, n) < n - 1) return b;
        free(b);
    }
    return NULL;
}

// Um salvamento que nao abre (estragado, cortado, ou de uma versao que nao se
// le mais): nada some calado. Uma copia inteira fica guardada a parte (a
// chave com "_ruim") e a mesa avisa - uma vez por chave na sessao.
#define SALVO_CHAVES 24                          // perfis, 3 runs, 3 historicos, 3 estilos, as opcoes...
static char salvo_avisado[SALVO_CHAVES][16];     // as chaves ja avisadas nesta sessao
static void salvo_estragado(const char *chave, const char *texto, const char *msg)
{
    int i = 0;
    while (i < SALVO_CHAVES && salvo_avisado[i][0] && strcmp(salvo_avisado[i], chave)) i++;
    if (i == SALVO_CHAVES || salvo_avisado[i][0]) return;   // ja avisada (ou chaves demais: nao repete a cada quadro)
    snprintf(salvo_avisado[i], sizeof salvo_avisado[i], "%s", chave);
    char k[24];
    snprintf(k, sizeof k, "%s_ruim", chave);
    char *inteiro = salvo_inteiro(chave);
    salva_grava(k, inteiro ? inteiro : texto);
    free(inteiro);
    snprintf(aviso_salvo, sizeof aviso_salvo, "%s", msg);
    t_aviso_salvo = 6.0f;
}

static bool perfis_le_de(const char *buf, int n);
static void cq_converte(perfil_t *p);    // conquistas.inc: o que o chefe ja tinha aberto continua aberto
static void cq_converte_novas(perfil_t *p);   // as conquistas da v1.26 (de um perfil "PER8")
static void legado_converte(perfil_t *p);     // o que muda de chefe e ja estava aberto fica aberto
static void perfis_le(void)
{
    if (perfis_lidos) return;
    perfis_lidos = true;
    memset(perfis, 0, sizeof perfis);
    perfil_atual = -1;
    static char buf[16 + 2 * sizeof perfis];
    int n = salva_le("perfis", buf, (int)sizeof buf);
    if (n > 0 && !perfis_le_de(buf, n)) {
        memset(perfis, 0, sizeof perfis);
        perfil_atual = -1;
        salvo_estragado("perfis", buf, "Os perfis salvos não abriram: uma cópia ficou guardada à parte.");
    }
}
static bool perfis_le_de(const char *buf, int n)
{
    perfil_t lido[N_PERFIS];
    if (n == 5 + (int)(2 * sizeof(perfil_v_t[N_PERFIS])) && !memcmp(buf, "PER3", 4)) {
        // Os de antes: o nivel n (de 5) vira 2n (de 10), e a vitoria no n, no 2n.
        perfil_v_t v[N_PERFIS];
        if (!de_hex(buf + 5, v, sizeof v)) return false;
        memset(lido, 0, sizeof lido);
        for (int i = 0; i < N_PERFIS; i++) {
            memcpy(lido[i].nome, v[i].nome, sizeof lido[i].nome);
            lido[i].avatar = v[i].avatar;
            lido[i].nivel = (uint8_t)(2 * (v[i].nivel > N_NIVEIS_V ? N_NIVEIS_V : v[i].nivel));
            for (int d = 0; d <= N_NIVEIS_V; d++) lido[i].venceu[2 * d] = v[i].venceu[d];
            lido[i].runs = v[i].runs;
            lido[i].partidas = v[i].partidas;
            lido[i].marcos = v[i].marcos;
        }
    } else if (n == 5 + (int)(2 * sizeof(perfil_4_t[N_PERFIS])) && !memcmp(buf, "PER4", 4)) {
        // Os de antes dos chefes: tudo igual, sem chefe nenhum anotado (o
        // que o nivel ja passou fica aberto do mesmo jeito).
        perfil_4_t v[N_PERFIS];
        if (!de_hex(buf + 5, v, sizeof v)) return false;
        memset(lido, 0, sizeof lido);
        for (int i = 0; i < N_PERFIS; i++) {
            memcpy(lido[i].nome, v[i].nome, sizeof lido[i].nome);
            lido[i].avatar = v[i].avatar;
            lido[i].nivel = v[i].nivel;
            memcpy(lido[i].venceu, v[i].venceu, sizeof lido[i].venceu);
            lido[i].runs = v[i].runs;
            lido[i].partidas = v[i].partidas;
            lido[i].marcos = v[i].marcos;
        }
    } else if (n == 5 + (int)(2 * sizeof(perfil_5_t[N_PERFIS])) && !memcmp(buf, "PER5", 4)) {
        // Os de antes das bolsas: tudo igual, so o Saco de Pano aberto (o
        // Copo e os Fosforos abrem pelo que ja foi vencido).
        perfil_5_t v[N_PERFIS];
        if (!de_hex(buf + 5, v, sizeof v)) return false;
        memset(lido, 0, sizeof lido);
        for (int i = 0; i < N_PERFIS; i++) {
            memcpy(lido[i].nome, v[i].nome, sizeof lido[i].nome);
            lido[i].avatar = v[i].avatar;
            lido[i].nivel = v[i].nivel;
            memcpy(lido[i].venceu, v[i].venceu, sizeof lido[i].venceu);
            lido[i].runs = v[i].runs;
            lido[i].partidas = v[i].partidas;
            lido[i].marcos = v[i].marcos;
            memcpy(lido[i].chefes, v[i].chefes, sizeof lido[i].chefes);
        }
    } else if (n == 5 + (int)(2 * sizeof(perfil_6_t[N_PERFIS])) && !memcmp(buf, "PER6", 4)) {
        // Os de antes das bolsas novas: tudo igual, as bolsas (8) em 8 bits.
        perfil_6_t v[N_PERFIS];
        if (!de_hex(buf + 5, v, sizeof v)) return false;
        memset(lido, 0, sizeof lido);
        for (int i = 0; i < N_PERFIS; i++) {
            memcpy(lido[i].nome, v[i].nome, sizeof lido[i].nome);
            lido[i].avatar = v[i].avatar;
            lido[i].nivel = v[i].nivel;
            memcpy(lido[i].venceu, v[i].venceu, sizeof lido[i].venceu);
            lido[i].runs = v[i].runs;
            lido[i].partidas = v[i].partidas;
            lido[i].marcos = v[i].marcos;
            memcpy(lido[i].chefes, v[i].chefes, sizeof lido[i].chefes);
            lido[i].bolsas = v[i].bolsas;
            lido[i].bolsas_vistas = v[i].bolsas_vistas;
            lido[i].bolsa_ult = v[i].bolsa_ult;
            memcpy(lido[i].bolsa_rec, v[i].bolsa_rec, sizeof v[i].bolsa_rec);
        }
    } else if (n == 5 + (int)(2 * sizeof(perfil_7_t[N_PERFIS])) && !memcmp(buf, "PER7", 4)) {
        // Os de antes das conquistas: tudo igual, sem conquista nenhuma
        // (a conversao abaixo mantem aberto o que um chefe ja abriu).
        perfil_7_t v[N_PERFIS];
        if (!de_hex(buf + 5, v, sizeof v)) return false;
        memset(lido, 0, sizeof lido);
        for (int i = 0; i < N_PERFIS; i++) {
            memcpy(lido[i].nome, v[i].nome, sizeof lido[i].nome);
            lido[i].avatar = v[i].avatar;
            lido[i].nivel = v[i].nivel;
            memcpy(lido[i].venceu, v[i].venceu, sizeof lido[i].venceu);
            lido[i].runs = v[i].runs;
            lido[i].partidas = v[i].partidas;
            lido[i].marcos = v[i].marcos;
            memcpy(lido[i].chefes, v[i].chefes, sizeof lido[i].chefes);
            lido[i].bolsa_ult = v[i].bolsa_ult;
            memcpy(lido[i].bolsa_rec, v[i].bolsa_rec, sizeof lido[i].bolsa_rec);
            lido[i].bolsas = v[i].bolsas;
            lido[i].bolsas_vistas = v[i].bolsas_vistas;
        }
    } else if (n != 5 + (int)(2 * sizeof perfis) || (memcmp(buf, "PER9", 4) && memcmp(buf, "PER8", 4))
               || !de_hex(buf + 5, lido, sizeof lido)) return false;
    // De qualquer formato de antes das conquistas: o item que um chefe ja
    // tinha aberto ganha a conquista dele (so aqui, na conversao). O "PER8"
    // (o mesmo perfil, sem as conquistas da v1.26) ganha so as novas. E de
    // qualquer um de antes do "PER9", o que a tabela nova de chefes abre mais
    // tarde fica aberto para quem ja tinha.
    bool per8 = !memcmp(buf, "PER8", 4), per9 = !memcmp(buf, "PER9", 4);
    for (int i = 0; i < N_PERFIS; i++) {
        if (!lido[i].nome[0] || per9) continue;
        if (per8) cq_converte_novas(&lido[i]);
        else cq_converte(&lido[i]);
        legado_converte(&lido[i]);
    }
    for (int i = 0; i < N_PERFIS; i++) {
        lido[i].nome[sizeof lido[i].nome - 1] = 0;
        if (lido[i].avatar >= AV_N) lido[i].avatar = 0;
        if (lido[i].nivel > N_NIVEIS) lido[i].nivel = N_NIVEIS;
        if (lido[i].bolsa_ult >= B_N) lido[i].bolsa_ult = B_PANO;
        lido[i].bolsas &= BOLSAS_TODAS;               // so as que existem
        lido[i].bolsas_vistas &= BOLSAS_TODAS;
    }
    memcpy(perfis, lido, sizeof perfis);
    int a = buf[4] - '0';
    if (a >= 0 && a < N_PERFIS && perfis[a].nome[0]) perfil_atual = a;
    return true;
}

// Conteudo liberado: o que vem com cada dificuldade.
// ---- A regra de antes (ate a v1.25), so para converter perfis: os niveis de
// antes e a divisao de cada nivel por raridade, em tres partes. Nao muda mais.
static const uint8_t NIVEL_AMULETO_ANTES[A_N] = {
    [A_FERRADURA] = 1, [A_PESADO] = 2, [A_RELOGIO] = 3, [A_CARTOLA] = 4, [A_COROA] = 5,
    [A_CUPOM] = 6, [A_PRENSA] = 7, [A_TACA] = 8, [A_CORINGA] = 9, [A_DUPLA] = 10 };
static int nivel_antes(int t)
{
    switch (t) {
    case D_EXPLOSIVO: case D_BUMERANGUE: case D_PREVISIVEL: case D_DOMINO:
    case D_HEXA: case D_SENTINELA: case D_PIRATA: case D_COPIADOR:              return 1;
    case D_MOSAICO: case D_COLCHA: case D_CARROSSEL:                            return 4;
    case D_ESTILINGUE:                                                          return 2;
    case D_PIAO: case D_GELATINA:                                               return 3;
    case D_LEQUE:                                                               return 5;
    case D_BALAO: case D_PAPAGAIO:                                              return 6;
    case D_CAMALEAO: case D_RETRATO:                                            return 7;   // o Porta-Retrato: ja habituado aos adesivos
    case D_CAMELO:                                                              return 8;
    case D_MATRIOSKA:                                                           return 9;
    case D_TELA:                                                                return 5;
    case D_JACKPOT: case D_SEMENTE: case D_SEGURANCA: case D_AZARAO:
    case D_AMPULHETA: case D_MALDICAO: case D_DESAFIANTE: case D_VAZIO:         return 2;
    case D_CARRASCO: case D_LASER: case D_AGIOTA: case D_ACUMULADOR:
    case D_MEDIA: case D_COLECIONADOR: case D_CALIBRE: case D_MARTIR:
    case D_REVOLVER: case D_ADESIVADO:                                          return 3;
    case D_CARIDOSO: case D_GATUNO: case D_VAMPIRO: case D_QUEBRADO:
    case D_VIDRO: case D_PONTE: case D_RANCOR: case D_FICHA_VELHA:
    case D_RELOGIO:                                                             return 4;
    case D_INVEJOSO: case D_FARTURA: case D_SORTE: case D_PUXA_FILA:
    case D_COVEIRO: case D_SACRIFICIO: case D_SANGUESSUGA: case D_PATINHOS:
    case D_TERREMOTO:                                                           return 5;
    case D_ESPELHO: case D_VENTANIA: case D_FERREIRO: case D_MISERICORDIOSO:
    case D_GEMEO: case D_ECO: case D_FURIA:                                     return 6;
    case D_VIUVA: case D_PRISMA: case D_CHECKPOINT: case D_POTE_QUENTE:
    case D_OURICO: case D_ANCORA: case D_ONDA:                                  return 7;
    case D_CAPITAO: case D_ESPERANCA: case D_SABOTADOR: case D_FENIX:
    case D_TUDO_NADA: case D_EGOISTA: case D_ASTRO: case D_DIGITAL:
    case D_AGNUS:                                                               return 8;
    case D_FOGO: case D_SORTE_REVES: case D_OCULTO: case D_PACTO:
    case D_ASSOMBRACAO: case D_CAVALO: case D_FICHA_CORINGA:                    return 9;
    case D_MALDITO: case D_GARIMPEIRO: case D_AUTOFAGIA: case D_BERSERKER:      // Agouro
    case D_TRONO: case D_MEDUSA:                                                return 10;
    default:                                                                    return 0;
    }
}
// D20 e Lastro abrem antes do Barao: na primeira vitoria contra o chefe do
// primeiro e do segundo salao.
static int marco_do_dado(int t) { return t == D_D20 ? 0 : (t == D_LASTRO ? 1 : -1); }

// O que abre, chefe a chefe. Os chefes da dificuldade d abrem, um pedaco
// cada, o que vem com o nivel d + 1 (a tabela abaixo): o Crupie, os mais
// comuns; a Dona, o amuleto e os do meio; o Barao e o Coringa, os mais raros;
// e quem fecha a run (o Barao, ou do Nivel 5 o Coringa) tambem o estilo do
// andar d e a dificuldade seguinte. O que a dificuldade ja passou fica todo
// aberto.
enum { CH_CRUPIE, CH_DONA, CH_BARAO, CH_CORINGA, CH_N };
static const char *const NOME_CHEFE[CH_N] = { "o Crupiê", "a Dona do Salão", "o Barão", "o Coringa" };
// Em dezembro, quem senta no lugar do Barao e o Papai Noel.
static const char *nome_chefe(int c) { return c == 2 && dezembro ? "o Papai Noel" : NOME_CHEFE[c]; }
static int chefe_final(int d) { return d >= CORINGA_DESDE ? CH_CORINGA : CH_BARAO; }
static int chefes_de(int d) { return chefe_final(d) + 1; }
static bool chefe_abriu_de(const perfil_t *p, int d, int c)
{
    if (d < 0) return true;
    return p->nivel > d || p->venceu[d] || (p->chefes[d] >> c & 1);
}
static bool chefe_abriu(int d, int c) { return chefe_abriu_de(perfil(), d, c); }

// O chefe de antes do dado t: os dados do nivel dele, do mais comum ao mais
// raro, em tres partes.
static int chefe_antes(int t)
{
    int nv = nivel_antes(t), n = 0, antes = 0;
    for (int u = 0; u < D_N; u++) {
        if (nivel_antes(u) != nv || marco_do_dado(u) >= 0) continue;
        n++;
        antes += TIPO[u].raridade < TIPO[t].raridade || (TIPO[u].raridade == TIPO[t].raridade && u < t);
    }
    return n ? antes * 3 / n : CH_CRUPIE;
}
// O adesivo de antes: ADESIVO[a].nivel, sempre com o Crupie.

// ---- A tabela de hoje (v1.26): cada item com o nivel que o abre (o 1 vem
// com o Normal vencido) e o chefe dele. Os chefes de um nivel abrem
// quantidades parecidas; quem fecha a run, um a menos (abre tambem o estilo
// e o nivel seguinte). O que nao esta aqui vem aberto desde o comeco, abre
// por conquista ou e marco (D20 e Lastro).
typedef struct { uint8_t nivel, chefe; } abre_t;
static const abre_t ABRE_DADO[D_N] = {
    [D_SENTINELA] = { 1, CH_CRUPIE }, [D_HEXA] = { 1, CH_CRUPIE }, [D_COPIADOR] = { 1, CH_DONA }, [D_PIRATA] = { 1, CH_BARAO }, [D_PREVISIVEL] = { 1, CH_BARAO },   // Nivel 1
    [D_VAZIO] = { 2, CH_CRUPIE }, [D_ESTILINGUE] = { 2, CH_CRUPIE }, [D_SEMENTE] = { 2, CH_DONA }, [D_SEGURANCA] = { 2, CH_DONA }, [D_LASER] = { 2, CH_BARAO }, [D_AZARAO] = { 2, CH_BARAO },   // Nivel 2
    [D_PIAO] = { 3, CH_CRUPIE }, [D_CALIBRE] = { 3, CH_CRUPIE }, [D_MEDIA] = { 3, CH_DONA }, [D_GELATINA] = { 3, CH_DONA }, [D_CARRASCO] = { 3, CH_BARAO }, [D_MARTIR] = { 3, CH_BARAO },   // Nivel 3
    [D_QUEBRADO] = { 4, CH_CRUPIE }, [D_VIDRO] = { 4, CH_CRUPIE }, [D_COLCHA] = { 4, CH_DONA }, [D_FICHA_VELHA] = { 4, CH_DONA }, [D_CARIDOSO] = { 4, CH_BARAO }, [D_VAMPIRO] = { 4, CH_BARAO },   // Nivel 4
    [D_FARTURA] = { 5, CH_CRUPIE }, [D_SACRIFICIO] = { 5, CH_CRUPIE }, [D_SANGUESSUGA] = { 5, CH_DONA }, [D_INVEJOSO] = { 5, CH_BARAO }, [D_TERREMOTO] = { 5, CH_BARAO },   // Nivel 5
    [D_FERREIRO] = { 6, CH_CRUPIE }, [D_BALAO] = { 6, CH_CRUPIE }, [D_PAPAGAIO] = { 6, CH_DONA }, [D_LEQUE] = { 6, CH_BARAO }, [D_VENTANIA] = { 6, CH_CORINGA }, [D_PONTE] = { 6, CH_CORINGA },   // Nivel 6
    [D_ANCORA] = { 7, CH_CRUPIE }, [D_ONDA] = { 7, CH_CRUPIE }, [D_OURICO] = { 7, CH_DONA }, [D_RETRATO] = { 7, CH_DONA }, [D_CHECKPOINT] = { 7, CH_BARAO }, [D_CARROSSEL] = { 7, CH_CORINGA },   // Nivel 7
    [D_SABOTADOR] = { 8, CH_CRUPIE }, [D_ASSOMBRACAO] = { 8, CH_CRUPIE }, [D_EGOISTA] = { 8, CH_DONA }, [D_CAPITAO] = { 8, CH_BARAO }, [D_ESPERANCA] = { 8, CH_CORINGA },   // Nivel 8
    [D_CAVALO] = { 9, CH_CRUPIE }, [D_FOGO] = { 9, CH_CRUPIE }, [D_PACTO] = { 9, CH_DONA }, [D_MATRIOSKA] = { 9, CH_BARAO }, [D_FICHA_CORINGA] = { 9, CH_CORINGA },   // Nivel 9
    [D_GARIMPEIRO] = { 10, CH_CRUPIE }, [D_AUTOFAGIA] = { 10, CH_DONA }, [D_MEDUSA] = { 10, CH_BARAO }, [D_BERSERKER] = { 10, CH_CORINGA },   // Nivel 10
};
static const abre_t ABRE_ADESIVO[AD_N] = {
    [AD_GORJETA] = { 1, CH_CRUPIE }, [AD_FRAGIL] = { 1, CH_CRUPIE }, [AD_BOMBA] = { 1, CH_DONA }, [AD_CORRENTE] = { 1, CH_BARAO },   // Nivel 1
    [AD_VELCRO] = { 2, CH_CRUPIE }, [AD_TIQUETE] = { 2, CH_CRUPIE }, [AD_BIS] = { 2, CH_DONA }, [AD_IMPERMEAVEL] = { 2, CH_BARAO },   // Nivel 2
    [AD_BARRO] = { 3, CH_CRUPIE }, [AD_ESTRELINHA] = { 3, CH_CRUPIE }, [AD_LENTICULAR] = { 3, CH_DONA }, [AD_TRANSPARENTE] = { 3, CH_BARAO },   // Nivel 3
    [AD_CHICLETE] = { 4, CH_CRUPIE }, [AD_IMANTADO] = { 4, CH_CRUPIE }, [AD_MASSINHA] = { 4, CH_DONA }, [AD_BUSSOLA] = { 4, CH_BARAO },   // Nivel 4
    [AD_CORRETIVO] = { 5, CH_CRUPIE }, [AD_CASCA] = { 5, CH_CRUPIE }, [AD_CARBONO] = { 5, CH_DONA }, [AD_MOLDURA] = { 5, CH_BARAO },   // Nivel 5
    [AD_POSTIT] = { 6, CH_CRUPIE }, [AD_COLEIRA] = { 6, CH_DONA }, [AD_FERMENTO] = { 6, CH_BARAO },   // Nivel 6
    [AD_XEROX] = { 7, CH_CRUPIE }, [AD_NEON] = { 7, CH_BARAO }, [AD_CHUMBO] = { 7, CH_CORINGA },   // Nivel 7
    [AD_RATOEIRA] = { 8, CH_CRUPIE }, [AD_PATINHO] = { 8, CH_BARAO }, [AD_NOTA_DEZ] = { 8, CH_CORINGA },   // Nivel 8
    [AD_AVESSO] = { 9, CH_CRUPIE }, [AD_HOLOGRAFICO] = { 9, CH_BARAO }, [AD_CARIMBO] = { 9, CH_CORINGA },   // Nivel 9
    [AD_ETIQUETA] = { 10, CH_CRUPIE }, [AD_DUPLA] = { 10, CH_BARAO }, [AD_BILHETE] = { 10, CH_CORINGA },   // Nivel 10
};
// Os amuletos: a Dona traz um em cada nivel.
static const uint8_t NIVEL_AMULETO[A_N] = {
    [A_FERRADURA] = 1, [A_DESFORRA] = 2, [A_TRINCA] = 3, [A_CONTRATO] = 4, [A_COROA] = 5, [A_CORRIMAO] = 6, [A_PRENSA] = 7, [A_ABRE_ALAS] = 8, [A_PIRAMIDE] = 9, [A_DUPLA] = 10 };
// Os de conquista guardam o nivel de antes (so para mostrar).
static int nivel_do_dado(int t)  { return ABRE_DADO[t].nivel ? ABRE_DADO[t].nivel : nivel_antes(t); }
static int chefe_do_dado(int t)  { return ABRE_DADO[t].nivel ? ABRE_DADO[t].chefe : chefe_antes(t); }
static int nivel_do_adesivo(int a) { return ABRE_ADESIVO[a].nivel ? ABRE_ADESIVO[a].nivel : ADESIVO[a].nivel; }
static int chefe_do_adesivo(int a) { return ABRE_ADESIVO[a].nivel ? ABRE_ADESIVO[a].chefe : CH_CRUPIE; }
// As conquistas (conquistas.inc): o item que tem uma abre so por ela.
static int  cq_do_dado(int t);
static int  cq_do_amuleto(int a);
static int  cq_do_adesivo(int a);
static bool cq_aberta(int c);
// O legado: os itens que a tabela da v1.26 abre mais tarde do que a regra de
// antes (outro nivel, um chefe depois, ou um amuleto que vinha aberto). Quem
// ja tinha um deles aberto continua com ele (o bit em perfil_t.legado,
// marcado na conversao). A lista so cresce no fim: o bit e a posicao.
enum { LG_DADO, LG_ADESIVO, LG_AMULETO };
static const struct { uint8_t k, id; } LEGADO[] = {
    { LG_DADO, D_PREVISIVEL }, { LG_DADO, D_SEMENTE }, { LG_DADO, D_AZARAO }, { LG_DADO, D_CARRASCO },
    { LG_DADO, D_FICHA_VELHA }, { LG_DADO, D_COLCHA }, { LG_DADO, D_CARIDOSO }, { LG_DADO, D_INVEJOSO },
    { LG_DADO, D_LEQUE }, { LG_DADO, D_VENTANIA }, { LG_DADO, D_PONTE }, { LG_DADO, D_OURICO },
    { LG_DADO, D_CARROSSEL }, { LG_DADO, D_EGOISTA }, { LG_DADO, D_CAPITAO }, { LG_DADO, D_ESPERANCA },
    { LG_DADO, D_FICHA_CORINGA }, { LG_DADO, D_BERSERKER }, { LG_ADESIVO, AD_BOMBA }, { LG_ADESIVO, AD_CORRENTE },
    { LG_ADESIVO, AD_BIS }, { LG_ADESIVO, AD_IMPERMEAVEL }, { LG_ADESIVO, AD_LENTICULAR }, { LG_ADESIVO, AD_TRANSPARENTE },
    { LG_ADESIVO, AD_MASSINHA }, { LG_ADESIVO, AD_BUSSOLA }, { LG_ADESIVO, AD_CARBONO }, { LG_ADESIVO, AD_MOLDURA },
    { LG_ADESIVO, AD_COLEIRA }, { LG_ADESIVO, AD_FERMENTO }, { LG_ADESIVO, AD_NEON }, { LG_ADESIVO, AD_CHUMBO },
    { LG_ADESIVO, AD_PATINHO }, { LG_ADESIVO, AD_NOTA_DEZ }, { LG_ADESIVO, AD_AVESSO }, { LG_ADESIVO, AD_HOLOGRAFICO },
    { LG_ADESIVO, AD_CARIMBO }, { LG_ADESIVO, AD_ETIQUETA }, { LG_ADESIVO, AD_DUPLA }, { LG_ADESIVO, AD_BILHETE },
    { LG_AMULETO, A_DESFORRA }, { LG_AMULETO, A_TRINCA }, { LG_AMULETO, A_CONTRATO }, { LG_AMULETO, A_CORRIMAO },
    { LG_AMULETO, A_ABRE_ALAS }, { LG_AMULETO, A_PIRAMIDE } };
#define N_LEGADO ((int)(sizeof LEGADO / sizeof LEGADO[0]))
_Static_assert(sizeof LEGADO / sizeof LEGADO[0] <= 8 * sizeof ((perfil_t *)0)->legado, "o legado cabe no perfil");
static bool legado_de(const perfil_t *p, int k, int id)
{
    for (int i = 0; i < N_LEGADO; i++)
        if (LEGADO[i].k == k && LEGADO[i].id == id) return p->legado[i >> 3] >> (i & 7) & 1;
    return false;
}
// Na conversao de um perfil de antes da v1.26: o que a regra de antes ja
// tinha aberto entra no legado.
static void legado_converte(perfil_t *p)
{
    for (int i = 0; i < N_LEGADO; i++) {
        int id = LEGADO[i].id;
        bool aberto;
        switch (LEGADO[i].k) {
        case LG_DADO:    aberto = nivel_antes(id) > 0 && chefe_abriu_de(p, nivel_antes(id) - 1, chefe_antes(id)); break;
        case LG_ADESIVO: aberto = chefe_abriu_de(p, ADESIVO[id].nivel - 1, CH_CRUPIE); break;
        default:         aberto = chefe_abriu_de(p, NIVEL_AMULETO_ANTES[id] - 1, CH_DONA); break;
        }
        if (aberto) p->legado[i >> 3] |= (uint8_t)(1u << (i & 7));
    }
}

static bool dado_liberado(int t)
{
    if (sem_sorteio(t)) return false;            // nao sai em sorteio nenhum
    if (de_natal(t)) return true;                // no Natal, abertos para todos
    int c = cq_do_dado(t);
    if (c >= 0) return cq_aberta(c);
    int m = marco_do_dado(t);
    if (m >= 0) return perfil()->marcos >> m & 1;
    return chefe_abriu(nivel_do_dado(t) - 1, chefe_do_dado(t)) || legado_de(perfil(), LG_DADO, t);
}
static bool amuleto_liberado(int a)
{
    int c = cq_do_amuleto(a);
    if (c >= 0) return cq_aberta(c);
    return chefe_abriu(NIVEL_AMULETO[a] - 1, CH_DONA) || legado_de(perfil(), LG_AMULETO, a);
}

// O pedaco do chefe c na dificuldade d, como os cards o mostram: a
// dificuldade seguinte (so quem fecha a run), o amuleto, os dados (os
// lendarios por ultimo), os adesivos e o estilo do andar.
enum { NV_DIF, NV_AMULETO, NV_DADO, NV_ADESIVO, NV_ESTILO };
typedef struct { uint8_t k, v, cq; } novidade_t;    // cq: a conquista que o abriu, + 1 (0: o chefe)
#define PEDACO_MAX 16
static int pedaco(int d, int c, novidade_t *out)
{
    int n = 0, nv = d + 1;
    bool fecha = c == chefe_final(d);
    if (fecha && d < N_NIVEIS) out[n++] = (novidade_t){ NV_DIF, (uint8_t)nv, 0 };
    if (c == CH_DONA)
        for (int a = 0; a < A_N && n < PEDACO_MAX; a++)
            if (NIVEL_AMULETO[a] == nv && amuleto_no_jogo(a) && cq_do_amuleto(a) < 0) out[n++] = (novidade_t){ NV_AMULETO, (uint8_t)a, 0 };
    for (int r = RAR_COMUM; r <= RAR_LENDA; r++)
        for (int t = 0; t < D_N && n < PEDACO_MAX; t++)
            if (nivel_do_dado(t) == nv && marco_do_dado(t) < 0 && TIPO[t].raridade == r && chefe_do_dado(t) == c
                && cq_do_dado(t) < 0)
                out[n++] = (novidade_t){ NV_DADO, (uint8_t)t, 0 };
    for (int a = AD_NADA + 1; a < AD_N && n < PEDACO_MAX; a++)
        if (nivel_do_adesivo(a) == nv && chefe_do_adesivo(a) == c && ad_a_venda(a) && cq_do_adesivo(a) < 0)
            out[n++] = (novidade_t){ NV_ADESIVO, (uint8_t)a, 0 };
    char es[96];
    if (fecha && n < PEDACO_MAX && estilo_do_andar(d, es, sizeof es)) out[n++] = (novidade_t){ NV_ESTILO, (uint8_t)d, 0 };
    return n;
}
