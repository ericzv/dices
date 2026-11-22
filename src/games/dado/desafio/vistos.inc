// ---------------------------------------------------------------------------
// O selo "novo": o dado, o adesivo ou o amuleto que acabou de abrir aparece
// com o selo na loja, no pacote, na escolha do amuleto e na Colecao ate ser
// visto uma vez (nao precisa comprar nem usar). Na loja, no pacote e na
// escolha, ver e o cartao aparecer; na Colecao, e a peca estar na vitrine.
// O selo fica ate a pessoa sair da tela em que viu.
//
// Fica numa chave do perfil ("vistos1" a "vistos3": "VIS1" + os bits em
// hexadecimal), a parte do perfil, para nao mudar o salvamento dele. Sem a
// chave (um perfil de antes desta versao, ou um perfil novo), tudo o que ja
// esta aberto conta como visto: so o que abrir dali em diante ganha o selo.
// ---------------------------------------------------------------------------
enum { VS_DADO, VS_ADESIVO, VS_AMULETO };
#define VS_BASE_AD 144                       // os bits: dados de 0, adesivos de 144, amuletos de 208
#define VS_BASE_AM 208
#define VS_BITS    256
_Static_assert(D_N <= VS_BASE_AD && AD_N <= VS_BASE_AM - VS_BASE_AD && A_N <= VS_BITS - VS_BASE_AM,
               "os itens cabem nos bits dos vistos");
static uint8_t vistos[VS_BITS / 8];
static uint8_t vistos_tela[VS_BITS / 8];     // vistos agora, nesta tela: o selo fica ate sair dela
static int vistos_de = -2;                   // o perfil cujos vistos estao aqui (-2: nenhum ainda)
static int vistos_tela_de = -1;              // a tela de vistos_tela (tela_atual(), quadro.inc)
static int tela_atual(void);

static const char *chave_vistos(int i)
{
    static const char *const K[N_PERFIS] = { "vistos1", "vistos2", "vistos3" };
    return K[i >= 0 && i < N_PERFIS ? i : 0];
}
static int vs_bit(int k, int id)
{
    if (k == VS_DADO) return id >= 0 && id < D_N ? id : -1;
    if (k == VS_ADESIVO) return id > AD_NADA && id < AD_N ? VS_BASE_AD + id : -1;
    return id >= 0 && id < A_N ? VS_BASE_AM + id : -1;
}
static bool vs_aberto(int k, int id)
{
    if (k == VS_DADO) return dado_liberado(id);
    if (k == VS_ADESIVO) return adesivo_aberto(id);
    return amuleto_liberado(id);
}
static bool bit_de(const uint8_t *b, int i) { return i >= 0 && b[i >> 3] >> (i & 7) & 1; }
static void bit_poe(uint8_t *b, int i) { if (i >= 0) b[i >> 3] |= (uint8_t)(1u << (i & 7)); }

static void vistos_grava(void)
{
    if (perfil_atual < 0) return;
    char buf[4 + 2 * sizeof vistos + 1];
    memcpy(buf, "VIS1", 4);
    hex_de(vistos, sizeof vistos, buf + 4);
    salva_grava(chave_vistos(perfil_atual), buf);
}
// Tudo o que ja esta aberto conta como visto (o perfil sem a chave).
static void vistos_do_que_abriu(void)
{
    memset(vistos, 0, sizeof vistos);
    for (int t = 0; t < D_N; t++) if (vs_aberto(VS_DADO, t)) bit_poe(vistos, vs_bit(VS_DADO, t));
    for (int a = AD_NADA + 1; a < AD_N; a++) if (vs_aberto(VS_ADESIVO, a)) bit_poe(vistos, vs_bit(VS_ADESIVO, a));
    for (int a = 0; a < A_N; a++) if (vs_aberto(VS_AMULETO, a)) bit_poe(vistos, vs_bit(VS_AMULETO, a));
}
// Os vistos do perfil de agora (le a chave quando o perfil muda).
static void vistos_carrega(void)
{
    if (vistos_de == perfil_atual) return;
    vistos_de = perfil_atual;
    memset(vistos_tela, 0, sizeof vistos_tela);
    if (perfil_atual < 0) { memset(vistos, 0xFF, sizeof vistos); return; }   // sem perfil: nada e novo
    char b[4 + 2 * sizeof vistos + 8] = { 0 };
    int n = salva_le(chave_vistos(perfil_atual), b, (int)sizeof b);
    if (n == 4 + 2 * (int)sizeof vistos && !memcmp(b, "VIS1", 4) && de_hex(b + 4, vistos, sizeof vistos)) return;
    if (n > 0) salvo_estragado(chave_vistos(perfil_atual), b, "Os itens vistos deste perfil não abriram: uma cópia ficou guardada à parte.");
    vistos_do_que_abriu();
    vistos_grava();
}

// O item k/id esta aberto e e novo (nao visto, ou visto agora nesta tela)?
static bool item_novo(int k, int id)
{
    if (modo != M_DESAFIO && fase != F_CATALOGO) return false;
    vistos_carrega();
    int tela = tela_atual();
    if (tela != vistos_tela_de) { vistos_tela_de = tela; memset(vistos_tela, 0, sizeof vistos_tela); }
    int b = vs_bit(k, id);
    if (b < 0 || !vs_aberto(k, id)) return false;
    return !bit_de(vistos, b) || bit_de(vistos_tela, b);
}
// Viu agora: o selo continua nesta tela e some da proxima vez.
static void item_visto(int k, int id)
{
    if (!item_novo(k, id)) return;
    int b = vs_bit(k, id);
    if (bit_de(vistos, b)) return;
    bit_poe(vistos, b);
    bit_poe(vistos_tela, b);
    vistos_grava();
}
// O selo: a plaquinha de latao "NOVO" (a da sequencia), com a borda
// esquerda em x (selo_novo_esq) ou a direita em x (selo_novo_dir).
static int selo_novo_w(void) { return gfx_largura("NOVO", fonte_de(true)) + 14; }
static void selo_novo_esq(int x, int y) { ui_plaqueta(x + selo_novo_w() / 2, y, "NOVO", C_OURO); }
static void selo_novo_dir(int x, int y) { selo_novo_esq(x - selo_novo_w(), y); }
// Na fileira da Colecao (as pecas miudas): so um ponto de ouro, pulsando.
static void ponto_novo(int x, int y)
{
    int r = 2 + (sinf(t_relogio * 5) > 0.3f);
    gfx_disco(x, y, r + 1, C_BARRA);
    gfx_disco(x, y, r, C_OURO);
}
