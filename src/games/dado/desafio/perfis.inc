// ---------------------------------------------------------------------------
// Perfis: ate 3 por aparelho. Cada um tem nome, avatar e o que liberou no
// Modo Desafiante (a dificuldade mais alta e o que vem com ela); cada um
// guarda a sua run.
// ---------------------------------------------------------------------------
#define N_PERFIS 3
enum { AV_ET, AV_BERRO, AV_NOMALISA, AV_INTERRUPTOR, AV_SAMURAI, AV_SUSTO, AV_BLUE, AV_MAICON, AV_N };
static const char *const NOME_AVATAR[AV_N] = { "ET Vilu", "O Berro", "Nomalisa", "Interruptor do Futuro",
                                               "Samurai", "Susto", "Blue", "Maicon" };

typedef struct {
    char     nome[12];                   // "" = lugar vazio
    uint8_t  avatar, nivel;              // nivel: a dificuldade mais alta liberada
    uint8_t  venceu[N_NIVEIS + 1];       // Barao vencido em cada dificuldade
    uint16_t runs;
    uint16_t partidas;                   // partidas jogadas no Desafiante
    uint8_t  marcos;                     // bit c: ja venceu o chefe do salao c (abre dados)
} perfil_t;
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

// "PER4" + perfil escolhido por ultimo + os tres perfis em hexadecimal.
static void perfis_salva(void)
{
    static char buf[8 + 2 * sizeof perfis + 1];
    memcpy(buf, "PER4", 4);
    buf[4] = (char)('0' + (perfil_atual < 0 ? 0 : perfil_atual));
    hex_de(perfis, sizeof perfis, buf + 5);
    salva_grava("perfis", buf);
}

static void perfis_le(void)
{
    if (perfis_lidos) return;
    perfis_lidos = true;
    memset(perfis, 0, sizeof perfis);
    perfil_atual = -1;
    static char buf[16 + 2 * sizeof perfis];
    int n = salva_le("perfis", buf, (int)sizeof buf);
    perfil_t lido[N_PERFIS];
    if (n == 5 + (int)(2 * sizeof(perfil_v_t[N_PERFIS])) && !memcmp(buf, "PER3", 4)) {
        // Os de antes: o nivel n (de 5) vira 2n (de 10), e a vitoria no n, no 2n.
        perfil_v_t v[N_PERFIS];
        if (!de_hex(buf + 5, v, sizeof v)) return;
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
    } else if (n != 5 + (int)(2 * sizeof perfis) || memcmp(buf, "PER4", 4) || !de_hex(buf + 5, lido, sizeof lido)) return;
    for (int i = 0; i < N_PERFIS; i++) {
        lido[i].nome[sizeof lido[i].nome - 1] = 0;
        if (lido[i].avatar >= AV_N) lido[i].avatar = 0;
        if (lido[i].nivel > N_NIVEIS) lido[i].nivel = N_NIVEIS;
    }
    memcpy(perfis, lido, sizeof perfis);
    int a = buf[4] - '0';
    if (a >= 0 && a < N_PERFIS && perfis[a].nome[0]) perfil_atual = a;
}

// Conteudo liberado: o que vem com cada dificuldade.
static const uint8_t NIVEL_AMULETO[A_N] = {
    [A_FERRADURA] = 1, [A_PESADO] = 2, [A_RELOGIO] = 3, [A_CARTOLA] = 4, [A_COROA] = 5,
    [A_CUPOM] = 6, [A_PRENSA] = 7, [A_TACA] = 8, [A_CORINGA] = 9, [A_DUPLA] = 10 };
// Os dados que cada nivel abre, dos faceis de entender aos que mexem mais
// no jogo: no Normal so os de bonus simples (e os dados de numero), e a
// cada nivel uns poucos a mais - ataques, depois os que mexem na sequencia,
// por ultimo os estranhos. Assim quem comeca nao enfrenta tudo de uma vez.
static int nivel_do_dado(int t)
{
    switch (t) {
    case D_EXPLOSIVO: case D_BUMERANGUE: case D_PREVISIVEL: case D_DOMINO:
    case D_HEXA: case D_SENTINELA: case D_PIRATA: case D_COPIADOR:              return 1;
    case D_JACKPOT: case D_SEMENTE: case D_SEGURANCA: case D_AZARAO:
    case D_AMPULHETA: case D_MALDICAO: case D_DESAFIANTE: case D_VAZIO:         return 2;
    case D_CARRASCO: case D_LASER: case D_AGIOTA: case D_ACUMULADOR:
    case D_MEDIA: case D_COLECIONADOR: case D_CALIBRE: case D_MARTIR:           return 3;
    case D_CARIDOSO: case D_GATUNO: case D_VAMPIRO: case D_QUEBRADO:
    case D_VIDRO: case D_PONTE: case D_RANCOR: case D_FICHA_VELHA:              return 4;
    case D_INVEJOSO: case D_FARTURA: case D_SORTE: case D_PUXA_FILA:
    case D_COVEIRO: case D_SACRIFICIO: case D_SANGUESSUGA:                      return 5;
    case D_ESPELHO: case D_VENTANIA: case D_FERREIRO: case D_MISERICORDIOSO:
    case D_GEMEO: case D_ECO:                                                   return 6;
    case D_VIUVA: case D_PRISMA: case D_CHECKPOINT: case D_POTE_QUENTE:
    case D_OURICO: case D_ANCORA:                                               return 7;
    case D_CAPITAO: case D_ESPERANCA: case D_SABOTADOR: case D_FENIX:
    case D_TUDO_NADA: case D_EGOISTA:                                           return 8;
    case D_FOGO: case D_SORTE_REVES: case D_OCULTO: case D_PACTO:
    case D_ASSOMBRACAO: case D_CAVALO:                                          return 9;
    case D_MALDITO: case D_GARIMPEIRO: case D_AUTOFAGIA: case D_BERSERKER:      // Agouro
    case D_TRONO: case D_MEDUSA:                                                return 10;
    default:                                                                    return 0;
    }
}
// D20 e Lastro abrem antes do Barao: na primeira vitoria contra o chefe do
// primeiro e do segundo salao.
static int marco_do_dado(int t) { return t == D_D20 ? 0 : (t == D_LASTRO ? 1 : -1); }
static bool dado_liberado(int t)
{
    if (sem_sorteio(t)) return false;            // nao sai em sorteio nenhum
    int m = marco_do_dado(t);
    if (m >= 0) return perfil()->marcos >> m & 1;
    return nivel_do_dado(t) <= perfil()->nivel;
}
static bool amuleto_liberado(int a) { return NIVEL_AMULETO[a] <= perfil()->nivel; }
