// ---------------------------------------------------------------------------
// Perfis: ate 3 por aparelho. Cada um tem nome, avatar e o que liberou no
// Modo Desafiante (a dificuldade mais alta e o que vem com ela); cada um
// guarda a sua run.
// ---------------------------------------------------------------------------
#define N_PERFIS 3
#define N_NIVEIS 5                       // dificuldades acima da normal
enum { AV_ET, AV_BERRO, AV_NOMALISA, AV_INTERRUPTOR, AV_N };
static const char *const NOME_AVATAR[AV_N] = { "ET Vilu", "O Berro", "Nomalisa", "Interruptor do Futuro" };

typedef struct {
    char     nome[12];                   // "" = lugar vazio
    uint8_t  avatar, nivel;              // nivel: a dificuldade mais alta liberada
    uint8_t  venceu[N_NIVEIS + 1];       // Barao vencido em cada dificuldade
    uint16_t runs;
    uint16_t partidas;                   // partidas jogadas no Desafiante
    uint8_t  marcos;                     // bit c: ja venceu o chefe do salao c (abre dados)
} perfil_t;
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

// "PER3" + perfil escolhido por ultimo + os tres perfis em hexadecimal.
static void perfis_salva(void)
{
    static char buf[8 + 2 * sizeof perfis + 1];
    memcpy(buf, "PER3", 4);
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
    if (n != 5 + (int)(2 * sizeof perfis) || memcmp(buf, "PER3", 4)) return;
    perfil_t lido[N_PERFIS];
    if (!de_hex(buf + 5, lido, sizeof lido)) return;
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
    [A_FERRADURA] = 1, [A_PESADO] = 1, [A_RELOGIO] = 2, [A_CARTOLA] = 2, [A_COROA] = 3,
    [A_CUPOM] = 3, [A_PRENSA] = 4, [A_TACA] = 4, [A_CORINGA] = 5, [A_DUPLA] = 5 };
static int nivel_do_dado(int t)
{
    switch (t) {
    case D_BUMERANGUE: return 1;
    case D_GEMEO:      return 2;
    case D_PRISMA:     return 3;
    case D_FENIX:      return 4;
    case D_TRONO:      return 5;
    case D_CAPITAO:    return 4;
    case D_CAVALO:     return 5;                 // Cavalo de Troia
    case D_BERSERKER:  return 5;
    // Os fortes que ja existiam: presos no comeco.
    case D_EXPLOSIVO:  return 1;
    case D_EGOISTA:    return 2;
    case D_TUDO_NADA:  return 3;
    case D_MALDITO:    return 4;                 // Agouro
    default:           return 0;
    }
}
// D20 e Lastro abrem antes do Barao: na primeira vitoria contra o chefe do
// primeiro e do segundo salao.
static int marco_do_dado(int t) { return t == D_D20 ? 0 : (t == D_LASTRO ? 1 : -1); }
static bool dado_liberado(int t)
{
    int m = marco_do_dado(t);
    if (m >= 0) return perfil()->marcos >> m & 1;
    return nivel_do_dado(t) <= perfil()->nivel;
}
static bool amuleto_liberado(int a) { return NIVEL_AMULETO[a] <= perfil()->nivel; }
