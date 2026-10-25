// ===========================================================================
// Adesivos (Modo Desafiante)
// ===========================================================================
// Um adesivo vai num lado de um dado da bolsa e age quando o dado termina a
// rodada com esse lado para cima. Um dado pode ter varios, um por lado. A
// loja vende um por visita; escolhe-se o dado e depois o lado.
enum { AD_NADA, AD_CHUMBO, AD_REFLEXIVO, AD_CONFETE, AD_ELETRONICO, AD_TRANSPARENTE, AD_DOURADO,
       AD_LOBO, AD_REMENDO, AD_RASPADINHA, AD_FERMENTO, AD_BIS, AD_MIRA, AD_AVESSO, AD_GORJETA,
       AD_SELO, AD_FRAGIL, AD_ESTRELINHA, AD_CORRETIVO, AD_IMANTADO, AD_POSTIT, AD_HOLOGRAFICO,
       AD_PURPURINA, AD_VELCRO, AD_RATOEIRA,
       AD_N };                                   // um byte por lado (peca_t.ad)

#define AD_SEM_FUNDO 1                          // o Transparente: so o contorno de vidro
#define FERMENTO_MAX 20
#define SELO_PONTOS 5                           // o que o lacre do Selo paga no proximo lance valendo
#define FRAGIL_PONTOS 6                         // o Fragil valendo
#define ESTRELINHA_PONTOS 6                     // a Estrelinha com a sequencia cheia
#define PURPURINA_CADA 3                        // a Purpurina, por adesivo que agiu
#define BIS_DADOS 2                             // o Bis: dados a mais na proxima compra

// A regra em duas linhas (o cartao da loja), o preco, o nivel que abre e as
// cores: o vinil, a borda clara e a tinta do simbolo. O simbolo, 7x7.
typedef struct {
    const char *nome, *l1, *l2;
    uint8_t preco, nivel;
    uint16_t fundo, luz, tinta;
    const char *ic[7];
} adesivo_t;

static const adesivo_t ADESIVO[AD_N] = {
    [AD_CHUMBO]       = { "Chumbo", "sai neste lado", "metade das vezes", 6, 7,
                          RGB(70, 76, 88), RGB(140, 150, 164), C_BRANCO,
                          { ".......", "######.", ".######", "..###..", "..###..", ".#####.", "#######" } },
    [AD_REFLEXIVO]    = { "Reflexivo", "tirando este lado,", "rola de novo (1 vez)", 4, 0,
                          RGB(184, 194, 208), RGB(252, 248, 236), RGB(26, 34, 62),
                          { "..###..", ".#...#.", "#...###", "#....#.", "#......", ".#...#.", "..###.." } },
    [AD_CONFETE]      = { "Confete", "tirando este lado,", "se terminar válido: +4", 3, 0,
                          RGB(226, 164, 168), RGB(252, 248, 236), RGB(118, 30, 38), { 0 } },
    [AD_ELETRONICO]   = { "Eletrônico", "tirando este lado,", "se terminar válido: x2", 5, 0,
                          RGB(96, 182, 198), RGB(168, 204, 230), RGB(26, 34, 62),
                          { "....##.", "...##..", "..##...", ".#####.", "...##..", "..##...", ".##...." } },
    [AD_TRANSPARENTE] = { "Transparente", "tirando este lado,", "não pode ser anulado", 5, 3,
                          AD_SEM_FUNDO, RGB(168, 204, 230), RGB(168, 204, 230),
                          { "#######", "#....##", "#...#.#", "#..#..#", "#.#...#", "##....#", "#######" } },
    [AD_DOURADO]      = { "Dourado", "tirando este lado:", "+1 moeda", 5, 5,
                          RGB(228, 194, 106), RGB(246, 222, 132), RGB(106, 86, 44),
                          { ".#####.", "#.....#", "#.###.#", "#.#.#.#", "#.###.#", "#.....#", ".#####." } },
    [AD_LOBO]         = { "Lobo", "tirando este lado:", "-2 para o oponente", 3, 0,
                          RGB(190, 56, 60), RGB(226, 164, 168), C_BRANCO,
                          { "#.....#", "##...##", "#######", "#.###.#", "#######", ".#####.", "..###.." } },
    [AD_REMENDO]      = { "Remendo", "este lado", "tem 2 a mais", 4, 0,
                          RGB(88, 152, 84), RGB(130, 204, 112), C_BRANCO,
                          { "#.#.#.#", ".......", "#..#..#", "..###..", "#..#..#", ".......", "#.#.#.#" } },
    [AD_RASPADINHA]   = { "Raspadinha", "tirando este lado, se terminar", "válido: +10, e o adesivo sai", 2, 0,
                          RGB(206, 180, 218), RGB(252, 248, 236), RGB(54, 36, 74),
                          { "#######", "#..#..#", "#.###.#", "#..#..#", "#.#.#.#", "#######", "......." } },
    [AD_FERMENTO]     = { "Fermento", "tirando este lado, se terminar", "válido: +1, crescendo a cada vez", 5, 6,
                          RGB(238, 162, 74), RGB(246, 222, 132), RGB(56, 32, 20),
                          { "...#...", "..###..", ".#####.", "...#...", "..###..", ".#####.", "#######" } },
    [AD_BIS]          = { "Bis", "tirando este lado: 2 dados", "a mais na próxima rodada", 4, 2,
                          RGB(102, 74, 138), RGB(150, 112, 160), C_BRANCO,
                          { "####...", "#..#...", "#..####", "####..#", "...#..#", "...####", "......." } },
    [AD_MIRA]         = { "Mira", "tirando este lado, se terminar", "válido: anula o menor do oponente", 6, 8,
                          RGB(66, 18, 24), RGB(208, 98, 108), RGB(226, 164, 168),
                          { "...#...", ".#####.", ".#.#.#.", "###.###", ".#.#.#.", ".#####.", "...#..." } },
    [AD_AVESSO]       = { "Avesso", "tirando este lado:", "vira o lado oposto", 4, 4,
                          RGB(72, 150, 138), RGB(96, 182, 198), C_BRANCO,
                          { ".#.....", "###....", ".#.....", ".#...#.", ".#...#.", "....###", ".....#." } },
    [AD_GORJETA]      = { "Gorjeta", "tirando este lado: o oponente", "te paga 2 fichas", 3, 1,
                          RGB(56, 86, 150), RGB(96, 134, 196), RGB(168, 204, 230),
                          { ".#####.", "#.#.#.#", "#######", ".#####.", "#.#.#.#", "#######", "......." } },
    [AD_SELO]         = { "Selo", "tirando este lado: no próximo", "lance, se terminar válido, +5", 3, 0,
                          RGB(118, 30, 38), RGB(190, 56, 60), RGB(226, 164, 168),
                          { "..###..", ".#####.", "###.###", "##...##", "###.###", ".#####.", "..#.##." } },
    [AD_FRAGIL]       = { "Frágil", "tirando este lado: válido, +6;", "anulado, o dado quebra", 3, 1,
                          RGB(214, 112, 44), RGB(238, 162, 74), RGB(252, 248, 236),
                          { "#######", "#.....#", ".#.#.#.", "..###..", "...#...", "...#...", "..###.." } },
    [AD_ESTRELINHA]   = { "Estrelinha", "tirando este lado: com os 4", "dados válidos no fim, +6", 3, 3,
                          RGB(96, 134, 196), RGB(168, 204, 230), RGB(246, 222, 132),
                          { "...#...", "..###..", "#######", ".#####.", "..###..", ".##.##.", ".#...#." } },
    [AD_CORRETIVO]    = { "Corretivo", "tirando este lado, o dado", "não sobe a régua da sequência", 4, 5,
                          RGB(252, 248, 236), RGB(184, 194, 208), RGB(70, 76, 88),
                          { "..###..", "..###..", "...#...", ".#####.", "##...##", "##...##", ".#####." } },
    [AD_IMANTADO]     = { "Imantado", "tirando este lado: se o próximo", "tirar igual, os dois x2", 3, 4,
                          RGB(26, 34, 62), RGB(96, 134, 196), RGB(190, 56, 60),
                          { "##...##", "##...##", "#.....#", "#.....#", "#.....#", ".#...#.", "..###.." } },
    [AD_POSTIT]       = { "Post-it", "tirando este lado: o próximo", "rola 2 vezes e fica o maior", 4, 6,
                          RGB(246, 222, 132), RGB(252, 248, 236), RGB(150, 122, 62),
                          { "#######", "#.....#", "#.###.#", "#.....#", "#.###.#", "#....#.", "#####.." } },
    [AD_HOLOGRAFICO]  = { "Holográfico", "tirando este lado: copia os", "adesivos do fim dos seus dados", 6, 9,
                          RGB(184, 194, 208), RGB(252, 248, 236), RGB(102, 74, 138),
                          { "...#...", "..###..", ".#####.", "#######", ".#####.", "..###..", "...#..." } },
    [AD_PURPURINA]    = { "Purpurina", "tirando este lado, se válido:", "+3 por adesivo que agiu", 5, 7,
                          RGB(150, 62, 80), RGB(208, 98, 108), RGB(246, 222, 132),
                          { ".#.....", "###..#.", ".#..###", ".....#.", "..#....", ".###.#.", "..#...." } },
    [AD_VELCRO]       = { "Velcro", "tirando este lado: o dado volta", "para a mão na próxima rodada", 3, 2,
                          RGB(36, 32, 29), RGB(96, 88, 80), RGB(166, 154, 132),
                          { "#.#.#.#", "#.#.#.#", "#######", ".......", "#######", ".#.#.#.", ".#.#.#." } },
    [AD_RATOEIRA]     = { "Ratoeira", "tirando este lado: anulado pelo", "oponente, o maior dele cai", 4, 8,
                          RGB(120, 78, 50), RGB(156, 106, 64), RGB(204, 194, 172),
                          { ".......", "....##.", "..####.", ".##.###", "#######", ".......", "#######" } },
};

// A raridade de cada um, na mesma escala dos dados: pesa no adesivo da loja
// do Desafiante (como na vitrine, pelo salao) e no premio do modo de 2
// jogadores (pela rodada).
static const uint8_t RAR_ADESIVO[AD_N] = {
    [AD_RASPADINHA] = RAR_COMUM, [AD_CONFETE] = RAR_COMUM, [AD_LOBO] = RAR_COMUM, [AD_GORJETA] = RAR_COMUM,
    [AD_FRAGIL] = RAR_COMUM, [AD_VELCRO] = RAR_COMUM,
    [AD_REFLEXIVO] = RAR_INCOMUM, [AD_REMENDO] = RAR_INCOMUM, [AD_BIS] = RAR_INCOMUM, [AD_AVESSO] = RAR_INCOMUM,
    [AD_CORRETIVO] = RAR_INCOMUM, [AD_POSTIT] = RAR_INCOMUM, [AD_RATOEIRA] = RAR_INCOMUM,
    [AD_SELO] = RAR_INCOMUM, [AD_ESTRELINHA] = RAR_INCOMUM, [AD_IMANTADO] = RAR_INCOMUM,
    [AD_ELETRONICO] = RAR_RARO, [AD_TRANSPARENTE] = RAR_RARO, [AD_DOURADO] = RAR_RARO, [AD_FERMENTO] = RAR_RARO,
    [AD_PURPURINA] = RAR_RARO,
    [AD_CHUMBO] = RAR_LENDA, [AD_MIRA] = RAR_LENDA, [AD_HOLOGRAFICO] = RAR_LENDA,
};
// Fora do Desafiante nao ha moedas: o Dourado nao sai no premio.
static bool ad_no_premio(int a) { return a > AD_NADA && a < AD_N && a != AD_DOURADO; }

// O adesivo do lado 'lado' (0: nenhum).
static int ad_no_lado(const peca_t *p, int lado)
{
    if (!p || lado < 1 || lado > AD_LADOS) return AD_NADA;
    return p->ad[lado - 1] < AD_N ? p->ad[lado - 1] : AD_NADA;
}
// O numero que o lado tira: o Remendo soma 2, e e esse o numero que conta,
// na sequencia e nos poderes do dado (o Trevo com Remendo no 2 tira 4).
static int remenda(const peca_t *p, int lado) { return ad_no_lado(p, lado) == AD_REMENDO ? lado + 2 : lado; }
static void ad_cola(peca_t *p, int lado, int a)
{
    if (lado >= 1 && lado <= AD_LADOS) p->ad[lado - 1] = (uint8_t)a;
}
// A peca da run salva de antes (adesivos em meio byte: o lado 1 no meio
// byte baixo de ad[0]) na de agora.
static peca_t peca_de_13(const peca13_t *v)
{
    peca_t p = { .tipo = v->tipo, .fixo = v->fixo, .fermento = v->fermento, .selo = v->selo };
    for (int l = 1; l <= AD_LADOS; l++) {
        uint8_t b = v->ad[(l - 1) / 2];
        p.ad[l - 1] = (uint8_t)((l - 1) & 1 ? b >> 4 : b & 15);
    }
    return p;
}
static int ad_quantos(const peca_t *p)
{
    int n = 0;
    for (int l = 1; l <= AD_LADOS; l++) n += ad_no_lado(p, l) != AD_NADA;
    return n;
}
// Os lados que um tipo mostra: o Lastro so de 4 a 12; o Sorte/Reves, os 12
// da Sorte.
static int ad_lado_min(int t) { return TIPO[t].efeito == EF_LASTRO ? 4 : 1; }
static int ad_lado_max(int t) { return TIPO[t].lados; }
static int ad_n_lados(int t) { return ad_lado_max(t) - ad_lado_min(t) + 1; }
// O lado do avesso: o 1 do d6 e o 6; o 4 do Lastro, o 12.
static int ad_oposto(int t, int lado) { return ad_lado_min(t) + ad_lado_max(t) - lado; }
// Ainda cabe adesivo nele? (um por lado)
static bool ad_cabe(const peca_t *p)
{
    for (int l = ad_lado_min(p->tipo); l <= ad_lado_max(p->tipo); l++) if (!ad_no_lado(p, l)) return true;
    return false;
}
// Quantos lados dele tem o adesivo a.
static int ad_conta(const peca_t *p, int a)
{
    int n = 0;
    for (int l = 1; l <= AD_LADOS; l++) n += ad_no_lado(p, l) == a;
    return n;
}
// Cabe o adesivo a nele? Um por lado, e no maximo dois Chumbos por dado.
#define CHUMBO_MAX 2
static bool ad_cabe_o(const peca_t *p, int a)
{
    return ad_cabe(p) && (a != AD_CHUMBO || ad_conta(p, AD_CHUMBO) < CHUMBO_MAX);
}

// ---------------------------------------------------------------------------
// O desenho: o simbolo, o adesivo solto e os selinhos da borda
// ---------------------------------------------------------------------------
// A regra inteira, para a colecao.
static const char *const AD_TEXTO[AD_N] = {
    [AD_CHUMBO]       = "O dado tira o lado do Chumbo metade das vezes; na outra metade rola normalmente (e ainda pode tirar esse lado). Cabem no máximo dois Chumbos num dado: com dois, cada um sai metade das vezes.",
    [AD_REFLEXIVO]    = "Tirando este lado, o dado rola de novo, uma vez só, e pontua o segundo lance.",
    [AD_CONFETE]      = "Tirando este lado, se o dado terminar válido: +4 pontos no fim da rodada.",
    [AD_ELETRONICO]   = "Tirando este lado, se o dado terminar válido: pontua x2 no fim da rodada (soma com os outros multiplicadores).",
    [AD_TRANSPARENTE] = "Tirando este lado, o dado não pode ser anulado: nem pela sequência, nem por outro dado. Ainda pode ser roubado.",
    [AD_DOURADO]      = "Tirando este lado, válido ou anulado: +1 moeda, que entra com o saque da mesa vencida.",
    [AD_LOBO]         = "Tirando este lado, válido ou anulado: -2 pontos para o oponente na rodada.",
    [AD_REMENDO]      = "Este lado tem 2 a mais, e o dado tira esse número: um 3 vira 5, e o 5 conta na sequência e nos poderes do dado (no Explosivo, o 6 vira 8 e explode).",
    [AD_RASPADINHA]   = "Tirando este lado, se o dado terminar válido: +10 pontos no fim da rodada, e o adesivo sai do dado.",
    [AD_FERMENTO]     = "Tirando este lado, se o dado terminar válido: +1 na primeira vez, +2 na segunda, e assim por diante. O dado guarda o quanto cresceu, a run inteira.",
    [AD_BIS]          = "Tirando este lado, válido ou anulado: você compra 2 dados a mais na próxima rodada.",
    [AD_MIRA]         = "Tirando este lado, se o dado terminar válido: anula o menor dado válido do oponente.",
    [AD_AVESSO]       = "Tirando este lado, o dado vira e fica com o lado oposto para cima (no d6, o 1 vira 6; no d20, o 1 vira 20).",
    [AD_GORJETA]      = "Tirando este lado, válido ou anulado: o oponente te paga 2 fichas, fora do pote.",
    [AD_SELO]         = "Tirando este lado, válido ou anulado, o dado fica lacrado. Na próxima vez que ele for lançado, caia no lado que cair, se terminar válido: +5 pontos no fim da rodada. Lançado, o lacre se rompe, valendo ou não; tirando o Selo de novo, lacra outra vez.",
    [AD_FRAGIL]       = "Tirando este lado: se o dado terminar válido, +6 pontos no fim da rodada; se terminar anulado, ele quebra e vira um Quebrado com o número que tirou (pontua esse número uma vez e sai da bolsa).",
    [AD_ESTRELINHA]   = "Tirando este lado: se a sua sequência terminar com os 4 dados válidos, +6 pontos no fim da rodada.",
    [AD_CORRETIVO]    = "Tirando este lado, o dado pontua mas não sobe a régua: o próximo só precisa ser igual ou maior que o dado válido de antes dele. Como a Ponte, mas o dado com o Corretivo ainda cai se sair menor.",
    [AD_IMANTADO]     = "Tirando este lado: se o próximo dado da sequência tirar o mesmo número, os dois pontuam x2 no fim da rodada (soma com os outros multiplicadores).",
    [AD_POSTIT]       = "Tirando este lado, o próximo dado da sequência rola duas vezes e fica com o maior resultado. O Viciado rola os dois dados duas vezes e fica com o maior de todos.",
    [AD_HOLOGRAFICO]  = "Tirando este lado, copia os adesivos dos seus outros dados que agiram no fim da rodada, como se fossem dele: o Confete dá +4, o Eletrônico dobra este dado, o Selo lacra este dado. Não copia os que agem no pouso (Chumbo, Reflexivo, Avesso, Remendo, Transparente, Corretivo, Post-it) nem a Ratoeira.",
    [AD_PURPURINA]    = "Tirando este lado, se o dado terminar válido: +3 pontos por adesivo seu que agiu na rodada, ela junto. Os que agem no pouso também contam.",
    [AD_VELCRO]       = "Tirando este lado, válido ou anulado, o dado gruda: na rodada seguinte ele volta direto para a sua mão (e conta como um dos dados da compra), em vez de ir para fora.",
    [AD_RATOEIRA]     = "Tirando este lado: se o oponente anular este dado com um poder (não a sequência), o maior dado válido dele também é anulado.",
};

// O simbolo em pixels de s x s; o Confete sao pontinhos de cores.
static void ad_icone(int x, int y, int a, int s, uint16_t c)
{
    if (a == AD_CONFETE) {
        static const int8_t P[7][2] = { {1,0}, {5,1}, {3,2}, {0,4}, {6,4}, {2,6}, {5,6} };
        static const uint16_t K[7] = { RGB(228, 194, 106), RGB(96, 134, 196), RGB(118, 30, 38), RGB(88, 152, 84),
                                       RGB(190, 56, 60), RGB(102, 74, 138), RGB(56, 86, 150) };
        for (int i = 0; i < 7; i++) gfx_rect(x + P[i][0] * s, y + P[i][1] * s, s, s, K[i]);
        return;
    }
    if (a <= AD_NADA || a >= AD_N) return;
    // Os de varias cores (fora do cinza da mao): o Holografico muda de cor em
    // faixas; a Purpurina, um brilho de cada cor; o Imantado, as pontas de
    // metal; a Ratoeira, o queijo sobre a tabua.
    static const uint16_t ARCO[6] = { RGB(208, 98, 108), RGB(246, 222, 132), RGB(88, 152, 84),
                                      RGB(96, 182, 198), RGB(96, 134, 196), RGB(150, 112, 160) };
    bool cores = c == ADESIVO[a].tinta;
    for (int j = 0; j < 7; j++)
        for (int i = 0; i < 7; i++) {
            if (ADESIVO[a].ic[j][i] != '#') continue;
            uint16_t k = c;
            if (cores && a == AD_HOLOGRAFICO) k = ARCO[(i + j) % 6];
            else if (cores && a == AD_PURPURINA) k = j <= 2 && i <= 2 ? RGB(246, 222, 132) : j <= 3 ? C_BRANCO : i <= 3 ? RGB(226, 164, 168) : RGB(246, 222, 132);
            else if (cores && a == AD_IMANTADO && j < 2) k = RGB(204, 194, 172);
            else if (cores && a == AD_RATOEIRA && j < 5) k = RGB(246, 222, 132);
            gfx_rect(x + i * s, y + j * s, s, s, k);
        }
}

// Retangulo de cantos aparados, cheio e so a borda.
static void ad_caixa(int x, int y, int w, int h, uint16_t c)
{
    gfx_rect(x + 1, y, w - 2, h, c);
    gfx_rect(x, y + 1, w, h - 2, c);
}
static void ad_caixa_borda(int x, int y, int w, int h, uint16_t c)
{
    gfx_rect(x + 1, y, w - 2, 1, c); gfx_rect(x + 1, y + h - 1, w - 2, 1, c);
    gfx_rect(x, y + 1, 1, h - 2, c); gfx_rect(x + w - 1, y + 1, 1, h - 2, c);
}

// O adesivo solto (loja, tela do lado, voando): vinil recortado, borda branca,
// sombra e a ponta de baixo descolando. s: o tamanho de cada pixel do simbolo;
// descola (0..1): o quanto a ponta levanta.
static void ad_solto_d(int cx, int cy, int a, int s, float descola)
{
    const adesivo_t *d = &ADESIVO[a];
    int w = 7 * s + 6 * (s > 1 ? 2 : 1), x = cx - w / 2, y = cy - w / 2;
    gfx_rect(x + 2, y + 3, w, w, C_SOMBRA);
    ad_caixa(x - 1, y - 1, w + 2, w + 2, C_BRANCO);
    if (d->fundo == AD_SEM_FUNDO) {
        ad_caixa(x + 1, y + 1, w - 2, w - 2, C_FELTRO_ESC);
        ad_caixa_borda(x + 1, y + 1, w - 2, w - 2, d->luz);
    } else {
        ad_caixa(x + 1, y + 1, w - 2, w - 2, d->fundo);
    }
    ad_icone(cx - 7 * s / 2, cy - 7 * s / 2, a, s, d->tinta);
    int p = s + 3 + (int)(descola * (2 * s + 3));   // a ponta descolando
    for (int i = 0; i < p; i++) gfx_rect(x + w + 1 - p + i, y + w - i, p - i, 1, C_FELTRO);
    for (int i = 0; i < p; i++) gfx_rect(x + w + 1 - p, y + w + 1 - p + i, i + 1, 1, C_MARFIM_S);
    if (descola > 0) gfx_linha(x + w + 1 - p, y + w, x + w, y + w + 1 - p, C_BRANCO);
}


// Um selinho da borda: a pilula na cor do adesivo com o numero do lado.
static int ad_pilula_w(int lado)
{
    char s[4];
    snprintf(s, sizeof s, "%d", lado);
    return gfx_largura(s, GFX_MIUDO) + 5;
}
static void ad_pilula(int cx, int cy, int a, int lado, bool aceso)
{
    const adesivo_t *d = &ADESIVO[a];
    char s[4];
    snprintf(s, sizeof s, "%d", lado);
    int w = ad_pilula_w(lado), h = 9, x = cx - w / 2, y = cy - h / 2;
    gfx_rect(x + 1, y + 2, w, h, C_SOMBRA);
    if (aceso) ad_caixa(x - 1, y - 1, w + 2, h + 2, C_BRANCO);
    ad_caixa(x, y, w, h, d->luz);
    ad_caixa(x + 1, y + 1, w - 2, h - 2, d->fundo == AD_SEM_FUNDO ? C_FELTRO_ESC : d->fundo);
    gfx_texto(x + 3, y - 2, s, d->tinta, GFX_MIUDO, false);
}

// Os selinhos de um dado parado (mao, bolsa, loja): em fila na beira de
// baixo, cada um com o numero do seu lado; mais de 4, "+N". Pequeno demais
// para numero, so pontinhos da cor. 'aceso': o lado que pisca (0: nenhum).
static void ad_selos(float cx, float cy, float r, const peca_t *p, int aceso)
{
    int lados[AD_LADOS], n = 0;
    for (int l = 1; l <= AD_LADOS; l++) if (ad_no_lado(p, l)) lados[n++] = l;
    if (!n) return;
    if (r < 12) {                                // miudo: um ponto por adesivo
        int x = (int)cx - (n * 4 - 1) / 2, y = (int)(cy + r * 0.8f);
        for (int i = 0; i < n && i < 5; i++, x += 4) {
            gfx_rect(x, y, 3, 3, ADESIVO[ad_no_lado(p, lados[i])].luz);
            gfx_pixel(x + 1, y + 1, ADESIVO[ad_no_lado(p, lados[i])].fundo == AD_SEM_FUNDO ? C_FELTRO_ESC
                                                                                         : ADESIVO[ad_no_lado(p, lados[i])].fundo);
        }
        return;
    }
    int m = n > 4 ? 3 : n, tot = 0, ws[4];
    for (int i = 0; i < m; i++) { ws[i] = ad_pilula_w(lados[i]); tot += ws[i] + 1; }
    char mais[12] = "";
    if (n > m) { snprintf(mais, sizeof mais, "+%d", n - m); tot += gfx_largura(mais, GFX_MIUDO) + 2; }
    int x = (int)cx - tot / 2, y = (int)(cy + r * 0.85f);
    for (int i = 0; i < m; i++) {
        ad_pilula(x + ws[i] / 2, y, ad_no_lado(p, lados[i]), lados[i], lados[i] == aceso);
        x += ws[i] + 1;
    }
    if (mais[0]) {
        gfx_texto(x + 2, y - 5, mais, C_SOMBRA, GFX_MIUDO, false);
        gfx_texto(x + 1, y - 6, mais, C_MARFIM, GFX_MIUDO, false);
    }
}

// O lacre de cera que o Selo deixa no dado, de raio r, centrado em (x, y):
// a cera escorrida em volta, o carimbo afundado no meio e um brilho. lado:
// so uma metade (-1 a esquerda, 1 a direita: o lacre rompendo); 0, inteiro.
static const uint16_t CERA[3] = { RGB(118, 30, 38), RGB(190, 56, 60), RGB(226, 164, 168) };
static void lacre_cera(int x, int y, int r, int lado)
{
    for (int dy = -r - 1; dy <= r + 1; dy++)
        for (int dx = -r - 1; dx <= r + 1; dx++) {
            if ((lado < 0 && dx > 0) || (lado > 0 && dx < 0)) continue;
            int d2 = dx * dx + dy * dy;
            bool gota = d2 <= (r + 1) * (r + 1) && ((dx * 7 + dy * 3 + 64) % 5 == 0);   // a beira escorrida, torta
            if (d2 > r * r && !gota) continue;
            uint16_t c = d2 > (r - 1) * (r - 1) ? CERA[0] : CERA[1];
            if (r >= 4 && ((dx == 0 && dy * dy <= 2) || (dy == 0 && dx * dx <= 2))) c = CERA[0];   // o carimbo, em cruz
            if (dx == -r / 2 && dy == -r / 2 && r >= 3) c = CERA[2];                            // o brilho
            gfx_pixel(x + dx + 1, y + dy + 1, C_SOMBRA);
            gfx_pixel(x + dx, y + dy, c);
        }
}

// O som de colar um adesivo na loja, e o de cada um quando age.
static int som_cola(void) { return SOM_AD_COLA; }
static int som_adesivo(int a)
{
    static const uint8_t SOM[AD_N] = {
        [AD_CHUMBO] = SOM_AD_CHUMBO, [AD_REFLEXIVO] = SOM_AD_REFLEXIVO, [AD_CONFETE] = SOM_AD_CONFETE,
        [AD_ELETRONICO] = SOM_AD_ELETRONICO, [AD_TRANSPARENTE] = SOM_AD_TRANSPARENTE, [AD_DOURADO] = SOM_AD_DOURADO,
        [AD_LOBO] = SOM_AD_LOBO, [AD_REMENDO] = SOM_AD_REMENDO, [AD_RASPADINHA] = SOM_AD_RASPADINHA,
        [AD_FERMENTO] = SOM_AD_FERMENTO, [AD_BIS] = SOM_AD_BIS, [AD_MIRA] = SOM_AD_MIRA,
        [AD_AVESSO] = SOM_AD_AVESSO, [AD_GORJETA] = SOM_AD_GORJETA, [AD_SELO] = SOM_AD_SELO,
        [AD_FRAGIL] = SOM_AD_FRAGIL, [AD_ESTRELINHA] = SOM_AD_ESTRELINHA, [AD_CORRETIVO] = SOM_AD_CORRETIVO,
        [AD_IMANTADO] = SOM_AD_IMANTADO, [AD_POSTIT] = SOM_AD_POSTIT, [AD_HOLOGRAFICO] = SOM_AD_HOLOGRAFICO,
        [AD_PURPURINA] = SOM_AD_PURPURINA, [AD_VELCRO] = SOM_AD_VELCRO, [AD_RATOEIRA] = SOM_AD_RATOEIRA,
    };
    return a > AD_NADA && a < AD_N ? SOM[a] : SOM_EFEITO;
}

// Os adesivos de um dado numa linha so (a tela da bolsa): o selinho de cada
// lado e o nome; o Fermento com o quanto ja cresceu. Nao cabendo, "+N".
static void ad_linha(int cx, int y, const peca_t *p)
{
    int lados[AD_LADOS], n = 0, ws[AD_LADOS], tot = 0, m = 0;
    char nomes[AD_LADOS][24];
    for (int l = 1; l <= AD_LADOS; l++) if (ad_no_lado(p, l)) lados[n++] = l;
    for (int i = 0; i < n; i++) {
        int a = ad_no_lado(p, lados[i]);
        if (a == AD_FERMENTO && p->fermento) snprintf(nomes[i], sizeof nomes[i], "%s +%d", ADESIVO[a].nome, p->fermento + 1);
        else if (a == AD_SELO && p->selo) snprintf(nomes[i], sizeof nomes[i], "%s lacrado", ADESIVO[a].nome);
        else snprintf(nomes[i], sizeof nomes[i], "%s", ADESIVO[a].nome);
        ws[i] = ad_pilula_w(lados[i]) + 3 + gfx_largura(nomes[i], GFX_MIUDO);
        if (tot + ws[i] > 520) break;
        tot += ws[i] + (m ? 10 : 0);
        m++;
    }
    if (!m) return;
    char mais[12] = "";
    if (m < n) { snprintf(mais, sizeof mais, "+%d", n - m); tot += 8 + gfx_largura(mais, GFX_MIUDO); }
    int x = cx - tot / 2;
    for (int i = 0; i < m; i++) {
        int a = ad_no_lado(p, lados[i]), pw = ad_pilula_w(lados[i]);
        ad_pilula(x + pw / 2, y + 5, a, lados[i], false);
        gfx_texto(x + pw + 3, y - 1, nomes[i], ADESIVO[a].luz, GFX_MIUDO, false);
        x += ws[i] + 10;
    }
    if (mais[0]) gfx_texto(x - 2, y - 1, mais, C_MARFIM_S, GFX_MIUDO, false);
}
