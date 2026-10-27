// ---------------------------------------------------------------------------
// As bolsas do comeco da run. Cada uma traz os dados de saida e uma regra
// leve, que melhora uma coisa e piora outra. A run comeca com o Saco de
// Pano; as outras abrem jogando, cada uma por um feito que ensina o jeito
// dela. Escolher e na tela de fichas (bolsa_escolha.inc).
// ---------------------------------------------------------------------------
#define BOLSA_DADOS 16                   // os dados do comeco de cada uma (0xFF: acabou)
#define PORCO_GUARDA 2                   // Porquinho: o que cada mesa vencida poe nele
#define PORCO_ABRE   12                  // sair de uma loja com isto (ou mais) abre o Porquinho
#define LATA_ABRE    3                   // adesivos colados na bolsa que abrem a Lata
#define LAPIS_ABRE   3                   // rodadas cheias numa partida que abrem o Estojo
typedef struct {
    const char *nome;
    const char *ganho, *custo;           // a regra: o que melhora e o que piora
    const char *mais;                    // o que vem alem dos dados ("" nenhum)
    const char *abre;                    // o que a abre
    uint16_t cor;                        // a cor da ficha
    uint8_t  dados[BOLSA_DADOS];
} bolsa_t;

#define X6(t) t, t, t, t, t, t
#define X4(t) t, t, t, t
static const bolsa_t BOLSA[B_N] = {
    [B_PANO]     = { "Saco de Pano", "Os dados de sempre.", "Sem regra nenhuma.", "",
                     "Aberta desde o começo.", RGB(120, 78, 50),
                     { X6(D_D6), X4(D_D4), D_D2, D_D2, 0xFF } },
    [B_COPO]     = { "Copo de Couro", "Dados maiores: dois D8 e o Viciado no lugar dos D2.",
                     "Você compra 6 dados por rodada, em vez de 7.", "",
                     "Vença o Crupiê.", RGB(150, 62, 80),
                     { X6(D_D6), X4(D_D4), D_D8, D_D8, D_VICIADO, 0xFF } },
    [B_LATA]     = { "Lata de Biscoito", "Colar adesivo custa 1 moeda a menos.",
                     "A vitrine das lojas mostra 1 dado a menos.", "e 2 adesivos já colados",
                     "Tenha 3 adesivos colados na bolsa.", RGB(56, 86, 150),
                     { X6(D_D6), X4(D_D4), D_D2, D_D2, D_PAR, 0xFF } },
    [B_PORCO]    = { "Porquinho", "Cada mesa vencida põe 2 moedas no porquinho: na loja, quebre-o quando quiser.",
                     "Até quebrar, cada mesa paga 1 moeda a menos.", "",
                     "Saia de uma loja com 12 moedas ou mais.", RGB(226, 164, 168),
                     { X6(D_D6), X4(D_D4), D_D2, D_D2, D_FICHAS, D_COBRADOR, 0xFF } },
    [B_SEMENTES] = { "Saco de Sementes", "Da 7ª rodada em diante, seus dados pontuam +1.",
                     "Nas rodadas 1 e 2, seus dados pontuam -1.", "",
                     "Vença uma partida começando a 7ª rodada atrás em fichas.", RGB(88, 152, 84),
                     { X6(D_D6), X4(D_D4), D_D2, D_D2, D_TREVO, 0xFF } },
    [B_LAPIS]    = { "Estojo de Lápis", "+1 dado na sequência.", "Toda rodada começa com -4.", "",
                     "Vença 3 rodadas de uma partida com 4 dados válidos.", RGB(228, 194, 106),
                     { X6(D_D6), X4(D_D4), D_D2, D_D2, D_DOBRO, 0xFF } },
    [B_BAU]      = { "Bauzinho", "Começa com 2 amuletos.", "Cabem só 4 amuletos.", "",
                     "Tenha 5 amuletos ao mesmo tempo.", RGB(102, 74, 138),
                     { X6(D_D6), X4(D_D4), D_D2, D_D2, D_D12, 0xFF } },
    [B_FOSFORO]  = { "Caixa de Fósforos", "Você vê os dados que o oponente comprou na rodada.",
                     "Cada partida começa com 45 fichas, 5 a menos.", "",
                     "Vença uma run.", RGB(214, 112, 44),
                     { X6(D_D6), X4(D_D4), D_D2, D_D2, D_ESCUDO, 0xFF } },
};
#undef X6
#undef X4

static int bolsa_n_dados(int b)
{
    int n = 0;
    while (n < BOLSA_DADOS && BOLSA[b].dados[n] != 0xFF) n++;
    return n;
}

static bool tem_bolsa(int j, int b) { return modo == M_DESAFIO && j == 0 && run.bolsa == b; }
static int am_lugares(void) { return run.bolsa == B_BAU ? AM_LUGARES - 1 : AM_LUGARES; }

// ---- O que abre cada uma ---------------------------------------------------
static int   bolsa_nova = -1;            // a que acabou de abrir: o aviso desce no alto
static float t_bolsa_nova;

// Aberta para o perfil? O Copo e os Fosforos abrem tambem pelo que ja foi
// vencido antes delas existirem.
static bool bolsa_aberta(int b)
{
    const perfil_t *p = perfil();
    if (b == B_PANO || (p->bolsas >> b & 1)) return true;
    if (b == B_COPO) return p->marcos & 1;             // o Crupie ja caiu
    if (b == B_FOSFORO)
        for (int d = 0; d <= N_NIVEIS; d++) if (p->venceu[d]) return true;
    return false;
}

static void abre_bolsa(int b)
{
    if (perfil_atual < 0 || b <= B_PANO || b >= B_N || bolsa_aberta(b)) return;
    perfis[perfil_atual].bolsas |= (uint8_t)(1u << b);
    perfis_salva();
    bolsa_nova = b;
    t_bolsa_nova = 0;
    som_toca(SOM_NOVIDADE);
}

// O que da para conferir a qualquer hora (des_salva chama): os adesivos
// colados na bolsa e os amuletos.
static void bolsas_confere(void)
{
    if (perfil_atual < 0) return;
    int ad = 0;
    for (int i = 0; i < run.n_col; i++) ad += ad_quantos(&run.col[i]);
    if (ad >= LATA_ABRE) abre_bolsa(B_LATA);
    if (am_conta(run_amuletos()) >= AM_LUGARES) abre_bolsa(B_BAU);
}

// Rodada vencida (partida.inc): as com 4 dados validos abrem o Estojo de Lapis.
static void des_rodada_vencida(void)
{
    int n = 0;
    for (int k = 0; k < F[0].lancados && k < N_FILA; k++) n += F[0].est[k] == V_VALIDO;
    if (n >= FILA_NORMAL && ++des_rod_cheias >= LAPIS_ABRE) abre_bolsa(B_LAPIS);
}

// ---- A run comeca com a bolsa ----------------------------------------------
static int des_bolsa;                    // a escolhida para a proxima run

static void bolsa_monta_run(int b)
{
    run.bolsa = (uint8_t)b;
    run.n_col = 0;
    for (int i = 0; i < bolsa_n_dados(b); i++) run.col[run.n_col++] = (peca_t){ .tipo = BOLSA[b].dados[i] };
    if (b != B_LATA) return;
    // A Lata: dois D6 com um adesivo cada, sorteados entre os abertos.
    rs = run.semente ^ 0x1A7AB15Cu;
    rnd_r();
    int abertos[AD_N], na = 0;
    for (int a = AD_NADA + 1; a < AD_N; a++)
        if (adesivo_liberado(a) && a != AD_CHUMBO) abertos[na++] = a;
    for (int k = 0, d6 = 0; k < 2 && na; d6++) {
        if (d6 >= run.n_col) break;
        if (run.col[d6].tipo != D_D6) continue;
        int q = sorte_r(na), lado = 1 + sorte_r(6);
        ad_cola(&run.col[d6], lado, abertos[q]);
        abertos[q] = abertos[--na];              // dois diferentes
        k++;
        d6 += 2;                                 // nao no D6 vizinho
    }
}

// ---- O Porquinho ---------------------------------------------------------
static void des_salva(void);
static bool porco_inteiro(void) { return run.bolsa == B_PORCO && !run.porco_quebrou; }
static float t_porco;                    // > 0: quebrando (loja.inc desenha)
static int   porco_saiu;                 // as moedas que sairam dele
static void porco_quebra(void)
{
    if (!porco_inteiro()) return;
    porco_saiu = run.porco;
    run.moedas += run.porco;
    run.porco = 0;
    run.porco_quebrou = 1;
    t_porco = 0.001f;
    som_toca(SOM_VIDRO);
    solta_confete(30);
    des_salva();
}

// ---- Os icones -------------------------------------------------------------
// Cada bolsa em 16x16 pixels: as letras do mapa e as cores delas. O mesmo
// desenho vai na faixa (1 pixel por pixel) e na ficha (2). Cada um tem um
// jeito de se mexer (t >= 0): o pingente do saco balanca, o copo chacoalha
// e o dado pula, a tampa da lata salta, a moeda cai no porquinho, o broto
// balanca, o lapis escreve, o bau brilha e o fogo do fosforo tremula.
typedef struct { const char *chaves; uint16_t cores[14]; const char *m[16]; } icone_bolsa_t;
static const icone_bolsa_t ICONE_BOLSA[B_N] = {
    [B_PANO] = { "KlmdGge", { RGB(36, 22, 16), RGB(186, 132, 80), RGB(150, 98, 54), RGB(116, 72, 38), RGB(228, 194, 106), RGB(150, 122, 62), RGB(78, 46, 30) },
              { "................", "...KKKKKKKKKK...",
              "..KllmllmllmlK..", "...KmKmlKmlKK...",
              "....KmmmmmmK....", "....KgGGGGgKG...",
              "....KmmmmmmK.G..", "...KlmmmmmmmKG..",
              "..KlmmmmmmmmdK..", ".KlmmmmmmmmmmdK.",
              ".KllmmmmmmmmmdK.", ".KlmmmmmmmmmddK.",
              ".KmmmmmmmmmmddK.", "..KmmmmmmmmddK..",
              "...KKdddddddK...", ".....KKKKKKK...." } },
    [B_COPO] = { "KwpoORrdsm", { RGB(36, 22, 16), RGB(252, 248, 236), RGB(36, 22, 16), RGB(66, 18, 24), RGB(36, 22, 16), RGB(208, 98, 108), RGB(150, 62, 80), RGB(118, 30, 38), RGB(238, 230, 212), RGB(204, 194, 172) },
              { "...........KKK..", "..........KwwwK.",
              ".........KwwpwwK", ".........KpwwwmK",
              "..........KwwmK.", "...KKKKKKKKKKK..",
              "..KdoOOOOOOOodK.", "..KRdddddddddrK.",
              "..KRrrrrrrrrrdK.", "..KsRsrsrsrsrsK.",
              "..KRRrrrrrrrrdK.", "...KRrrrrrrrdK..",
              "...KRrrrrrrrdK..", "...KsRsrsrsrsK..",
              "...KdddddddddK..", "....KKKKKKKKK..." } },
    [B_LATA] = { "KBbnwYyRrt", { RGB(36, 22, 16), RGB(96, 134, 196), RGB(56, 86, 150), RGB(26, 34, 62), RGB(252, 248, 236), RGB(228, 194, 106), RGB(150, 122, 62), RGB(190, 56, 60), RGB(118, 30, 38), RGB(238, 230, 212) },
              { "..KKKKKKKKKK....", ".KBBBBBBBBBBK...",
              "KBBwBBBBBBwBbK..", "KBBBBBwwBBBBbK..",
              ".KYYYYYYYYYYK...", "..KKKKKKKKKK.KK.",
              "..KRRK.KtK..KtK.", "..KRrRK.K..KtK..",
              ".KKKKKKKKKKKKKK.", "KBbbbbbbbbbbbbnK",
              "KBbwbbbwbbbwbbnK", "KYYYYYYYYYYYYYyK",
              "KBbbbbbbbbbbbbnK", "KBbwbbbwbbbwbbnK",
              ".KnnnnnnnnnnnnK.", "..KKKKKKKKKKKK.." } },
    [B_PORCO] = { "KPpweYyz", { RGB(118, 30, 38), RGB(226, 164, 168), RGB(208, 98, 108), RGB(252, 248, 236), RGB(36, 22, 16), RGB(246, 222, 132), RGB(228, 194, 106), RGB(150, 122, 62) },
              { ".....KYYK.......", ".....KyyK.......",
              ".....KyzK..KK...", "...KKKeeKKKpPK..",
              "..KPPPPPPPPPpK..", ".KPwPPPPPPPPPPK.",
              ".KPPPPPPPPPPePK.", "KPKPPPPPPPPPPPKK",
              "KK.KPPPPPPPPPppK", "...KPPPPPPPPPepK",
              "...KpPPPPPPPPppK", "...KppPPPPPPPpK.",
              "....KppppppppK..", "....KpK...KpK...",
              "....KKK...KKK...", "................" } },
    [B_SEMENTES] = { "KgGjJhvs", { RGB(36, 22, 16), RGB(130, 204, 112), RGB(88, 152, 84), RGB(190, 156, 86), RGB(150, 122, 62), RGB(106, 86, 44), RGB(88, 152, 84), RGB(40, 84, 62) },
              { "....gg....gg....", "...gGGg..gGGg...",
              "....gGGggGGg....", ".....sGGGGs.....",
              ".......GG.......", ".....KKGGKK.....",
              "....KjjvvjjK....", "....KJvvvvJK....",
              "...KjjjjjjjjK...", "..KjjJjjjJjjhK..",
              ".KjjjjjjjjjjjhK.", ".KjJjjjJjjjJjhK.",
              ".KjjjjjjjjjjhhK.", ".KjjJjjjJjjjhhK.",
              "..KhhhhhhhhhhK..", "...KKKKKKKKKK..." } },
    [B_LAPIS] = { "KYyowgrSsbRV", { RGB(36, 22, 16), RGB(246, 222, 132), RGB(228, 194, 106), RGB(214, 112, 44), RGB(240, 200, 164), RGB(62, 56, 50), RGB(226, 164, 168), RGB(184, 194, 208), RGB(140, 150, 164), RGB(96, 134, 196), RGB(190, 56, 60), RGB(88, 152, 84) },
              { "............KKK.", "...........KrrrK",
              "..........KSSrrK", ".........KyYSSK.",
              "........KyYYyK..", ".......KyYYyK...",
              "......KyYYyK....", ".....KyYYyK.....",
              "....KyYYyK......", "...KwyYyK.......",
              "...KwwyK........", "..KgwwK.........",
              "..KggK..........", ".KgKK...........",
              ".KK.............", "................" } },
    [B_BAU] = { "KWweYyzLS", { RGB(36, 22, 16), RGB(156, 106, 64), RGB(120, 78, 50), RGB(78, 46, 30), RGB(246, 222, 132), RGB(228, 194, 106), RGB(150, 122, 62), RGB(36, 22, 16), RGB(252, 248, 236) },
              { "...........S....", "....KKKKKKK.S.S.",
              "...KWWWWWWWKS...", "..KWyWWWWWWyWK..",
              ".KWWyWWWWWWyWWK.", ".KwwywwwwwwywwK.",
              ".KKKKKyYYyKKKKK.", ".KWWyWyLLyWyWWK.",
              ".KWWyWYLYYWyWWK.", ".KwwywyyyywywwK.",
              ".KWWyWWWWWWyWWK.", ".KwwywwwwwwywwK.",
              ".KeezeeeeeezeeK.", ".KyyyyyyyyyyyyK.",
              ".KKKKKKKKKKKKKK.", "................" } },
    [B_FOSFORO] = { "KFfoRrtTYywdb", { RGB(36, 22, 16), RGB(246, 222, 132), RGB(238, 162, 74), RGB(214, 112, 44), RGB(190, 56, 60), RGB(118, 30, 38), RGB(240, 200, 164), RGB(200, 142, 104), RGB(228, 194, 106), RGB(150, 122, 62), RGB(252, 248, 236), RGB(66, 18, 24), RGB(56, 86, 150) },
              { "..........F.....", ".........FfF....",
              ".........FofF...", "........KffoK...",
              "........KRRK....", ".......KRRK.....",
              "......KtTK......", ".....KtTK.......",
              "....KtTK........", ".KKKKKKKKKKKKK..",
              "KYYYYYYYYYYYYYK.", "KRRRRRRRRRRRRYKK",
              "KRwwRbbbbRwwRYKK", "KRRRRRRRRRRRRyKK",
              "KrrrrrrrrrrrrrK.", ".KKKKKKKKKKKKK.." } },
};

// O cadeado das fichas que ainda nao abriram, 12x14.
static const char *const CADEADO[14] = {
    "...KKKKKK...", "..KSSSSSSK..", ".KSK....KSK.", ".KSK....KSK.", ".KSK....KSK.", "KKKKKKKKKKKK",
    "KYYYYYYYYYYK", "KYyyyyyyyyYK", "KYyyyKKyyyYK", "KYyyyKKyyyYK", "KYyyyyKyyyYK", "KYyyyyyyyyYK",
    "KYYYYYYYYYYK", "KKKKKKKKKKKK" };

// O jeito de cada um se mexer, pixel a pixel: o deslocamento, a cor e se
// some. est: o Saco de Sementes na partida (0 semente, 1 broto, 2 em flor).
static void ic_anima(int b, int x, int y, char c, float t, int est, int *dx, int *dy, uint16_t *cor, bool *some)
{
    if (b == B_SEMENTES && y <= 4 && c != 'K') {            // o broto: some na semente
        if (est == 0 && y <= 3) { *some = true; return; }
        if (est == 0) { *cor = RGB(88, 152, 84); }
    }
    if (t < 0) return;
    switch (b) {
    case B_PANO:
        if (c == 'G' && x >= 12) *dx = (int)lroundf(sinf(t * 2.6f) * 1.2f);
        break;
    case B_COPO: {
        float q = fmodf(t, 2.2f);
        if (q < 0.45f) {
            *dx = (int)lroundf(sinf(q * 42) * 1.1f);
            if (y <= 4 && x >= 9) *dy = -(int)lroundf(sinf(q / 0.45f * 3.1416f) * 3);
        }
        break;
    }
    case B_LATA: {
        float q = fmodf(t, 2.6f);
        if (y <= 4 && q < 0.4f) *dy = -(int)lroundf(sinf(q / 0.4f * 3.1416f) * 2);
        int brilho = (int)((q - 0.5f) * 28);             // o brilho corre a lata na diagonal
        if ((c == 'B' || c == 'b') && x + y == brilho) *cor = RGB(168, 204, 230);
        break;
    }
    case B_PORCO: {
        float q = fmodf(t, 1.8f);
        if (y <= 2 && x >= 5 && x <= 8) {                 // a moeda: desce e entra
            int d = q < 0.7f ? (int)(q / 0.7f * 4) - 3 : 99;
            if (d > 2) { *some = true; break; }
            *dy = d;
            if (y + d < 0) *some = true;
        }
        if (q >= 0.7f && q < 0.85f && y >= 3) *dy = 1;    // e o porquinho pula
        break;
    }
    case B_SEMENTES:
        if (y <= 2 && c != 'K') *dx = (int)lroundf(sinf(t * 2.2f + y * 0.5f) * 1.1f);
        break;
    case B_LAPIS: {
        float q = fmodf(t, 2.0f);
        if (q < 0.9f) { *dx = (int)lroundf(sinf(q * 22) * 1); *dy = (int)lroundf(cosf(q * 22) * 1); }
        break;
    }
    case B_BAU:
        if (c == 'S') *some = fmodf(t * 2.5f + x * 0.37f, 2) < 0.8f;
        if (y >= 1 && y <= 5 && fmodf(t, 2.4f) < 0.25f) *dy = -1;
        break;
    case B_FOSFORO:
        if (y <= 3 && (c == 'F' || c == 'f' || c == 'o')) {
            int f = (int)(t * 11) % 3;
            if (c == 'F' && f == 1) *cor = RGB(238, 162, 74);
            if (c == 'f' && f == 2) *cor = RGB(246, 222, 132);
            if (y <= 1) *dx = f == 0 ? -1 : f == 2 ? 1 : 0;
        }
        break;
    }
}

// O icone da bolsa b com o centro em (cx, cy), e pixels de e x e, estreitado
// na horizontal por sx (a ficha virando). apaga: 0..100, misturado no escuro.
static void icone_bolsa_em(int b, int cx, int cy, int e, float sx, float t, int est, int apaga)
{
    if (b < 0 || b >= B_N || sx <= 0.05f) return;
    const icone_bolsa_t *ic = &ICONE_BOLSA[b];
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            char c = ic->m[y][x];
            if (c == '.') continue;
            const char *p = strchr(ic->chaves, c);
            if (!p) continue;
            uint16_t cor = ic->cores[p - ic->chaves];
            int dx = 0, dy = 0;
            bool some = false;
            ic_anima(b, x, y, c, t, est, &dx, &dy, &cor, &some);
            if (some) continue;
            if (apaga) cor = gfx_mistura(cor, C_BARRA, apaga);
            float x0 = (x - 8 + dx) * e * sx, x1 = (x - 7 + dx) * e * sx;
            int px = cx + (int)floorf(x0), pw = (int)floorf(x1) - (int)floorf(x0);
            gfx_rect(px, cy + (y - 8 + dy) * e, pw > 0 ? pw : 1, e, cor);
        }
    if (b == B_SEMENTES && est == 2) {                    // em flor: duas flores no broto
        int e2 = e;
        gfx_rect(cx + (int)(-6 * e2 * sx), cy - 8 * e2, e2 * 2, e2 * 2, C_ROSA);
        gfx_rect(cx + (int)(3 * e2 * sx), cy - 7 * e2, e2 * 2, e2 * 2, C_AMARELO);
    }
}

static void cadeado_em(int cx, int cy, int e, float t)
{
    static const char CH[] = "KSYy";
    const uint16_t COR[4] = { RGB(36, 22, 16), C_ACO, C_LATAO, C_LATAO_ESC };
    int dx = t >= 0 ? (int)lroundf(sinf(t * 50) * 1.4f) : 0;
    for (int y = 0; y < 14; y++)
        for (int x = 0; x < 12; x++) {
            const char *p = strchr(CH, CADEADO[y][x]);
            if (CADEADO[y][x] == '.' || !p) continue;
            gfx_rect(cx + (x - 6) * e + dx, cy + (y - 7) * e, e, e, COR[p - CH]);
        }
}

// O estagio do Saco de Sementes agora: na partida, pela rodada.
static int sementes_est(void)
{
    bool partida = modo == M_DESAFIO && rodada >= 1 && fase <= F_EFEITOS && fase != F_LOJA;
    if (!partida) return 1;
    return rodada <= 2 ? 0 : rodada >= 7 ? 2 : 1;
}

// A bolsa da run, pequena, na faixa (o mapa, a loja e a partida).
static void icone_da_run(int cx, int cy)
{
    icone_bolsa_em(run.bolsa < B_N ? run.bolsa : B_PANO, cx, cy, 1, 1, t_relogio, sementes_est(), 0);
}

// Uma ficha de bolsa, vista de cima (como as do menu): a cor da bolsa, as
// listras de marfim na borda e o icone no centro. sobe: o cursor a ergue;
// giro: o angulo das listras; sx: 1 de frente, perto de 0 de lado (virando);
// fechada: escura, com o cadeado (t_cad >= 0: o cadeado treme).
static void elipse(int cx, int cy, int rx, int ry, uint16_t c)
{
    if (rx < 1) rx = 1;
    for (int y = -ry; y <= ry; y++) {
        float f = 1 - (float)(y * y) / (float)(ry * ry);
        int w = (int)lroundf(rx * sqrtf(f > 0 ? f : 0));
        gfx_rect(cx - w, cy + y, 2 * w + 1, 1, c);
    }
}
static void ficha_bolsa(int cx, int cy, int r, int b, float sobe, float giro, bool foco, bool fechada,
                        float sx, float t_ic, float t_cad)
{
    uint16_t cor = BOLSA[b].cor;
    if (fechada) cor = gfx_mistura(cor, C_BARRA, 72);
    int y = cy - (int)lroundf(sobe * 6);
    int rx = (int)lroundf(r * sx);
    elipse(cx + 2, cy + 4, rx - (int)lroundf(sobe * 2), r - (int)lroundf(sobe * 2), C_SOMBRA);   // a sombra no feltro
    if (sx < 0.25f) {                                     // de lado: so a borda, com as listras
        elipse(cx, y, rx + 1, r, gfx_mistura(cor, C_BARRA, 45));
        for (int k = -r + 3; k < r - 2; k += 6) gfx_rect(cx - rx, y + k, 2 * rx + 1, 2, fechada ? C_EBANO_B : C_MARFIM_S);
        return;
    }
    if (foco) elipse(cx, y, rx + 2, r + 2, C_OURO);
    elipse(cx, y, rx, r, gfx_mistura(cor, C_BARRA, 45));
    elipse(cx, y, rx - 1, r - 1, cor);
    bool grande = r >= 24;                                // pequena (o aviso): sem listras, so o centro
    for (int k = 0; grande && k < 6; k++) {               // as listras da borda, girando com a ficha
        float a = giro + k * 1.0472f, d = 0.17f;
        float vx[4] = { cx + cosf(a - d) * (r - 7) * sx, cx + cosf(a - d) * (r - 1.5f) * sx,
                        cx + cosf(a + d) * (r - 1.5f) * sx, cx + cosf(a + d) * (r - 7) * sx };
        float vy[4] = { y + sinf(a - d) * (r - 7), y + sinf(a - d) * (r - 1.5f),
                        y + sinf(a + d) * (r - 1.5f), y + sinf(a + d) * (r - 7) };
        poligono(vx, vy, 4, fechada ? C_EBANO_B : C_MARFIM_S);
    }
    if (grande) {
        elipse(cx, y, (int)lroundf((r - 8) * sx), r - 8, gfx_mistura(cor, C_BARRA, 35));   // o friso de dentro
        elipse(cx, y, (int)lroundf((r - 9) * sx), r - 9, cor);
        for (int k = 0; k < 26; k++) {                    // o pontilhado em volta do centro
            float a = giro * 0.5f + k * 0.2417f;
            gfx_rect(cx + (int)lroundf(cosf(a) * (r - 10.5f) * sx), y + (int)lroundf(sinf(a) * (r - 10.5f)), 1, 1,
                     gfx_mistura(cor, fechada ? C_EBANO_B : C_MARFIM, 55));
        }
    }
    int rc = grande ? r - 12 : r - 3;
    elipse(cx, y, (int)lroundf((rc + 1) * sx), rc + 1, gfx_mistura(cor, C_BARRA, 30));
    elipse(cx, y, (int)lroundf(rc * sx), rc, fechada ? C_EBANO : C_MARFIM);
    if (!fechada) elipse(cx - (int)(4 * sx), y - 4, (int)lroundf((rc - 6) * sx), rc - 6, C_BRANCO);   // a luz do marfim
    if (!fechada) elipse(cx + (int)(1 * sx), y + 1, (int)lroundf((rc - 2) * sx), rc - 2, C_MARFIM);
    int e = rc >= 19 ? 2 : 1;
    if (fechada) {
        icone_bolsa_em(b, cx, y, e, sx, -1, 1, 88);       // a silhueta, quase apagada
        cadeado_em(cx, y + 2, 1, t_cad);
    } else icone_bolsa_em(b, cx, y, e, sx, t_ic, 1, 0);
}

// ---- O aviso: abriu uma bolsa nova -----------------------------------------
#define BOLSA_NOVA_T 3.6f
static void des_bolsa_nova(void)
{
    if (bolsa_nova < 0) return;
    float t = t_bolsa_nova;
    if (t >= BOLSA_NOVA_T) { bolsa_nova = -1; return; }
    float q = t < 0.35f ? sai_mola(t / 0.35f) : t > BOLSA_NOVA_T - 0.3f ? (BOLSA_NOVA_T - t) / 0.3f : 1;
    char s[64];
    snprintf(s, sizeof s, "Nova bolsa: %s", BOLSA[bolsa_nova].nome);
    int w = gfx_largura(s, 1) + 64, h = 40, x = GFX_W / 2 - w / 2, y = -h + (int)((h + FAIXA_H + 6) * q);
    ui_cartao(x, y, w, h, true, BOLSA[bolsa_nova].cor);
    ficha_bolsa(x + 22, y + h / 2, 15, bolsa_nova, 0, t * 2, false, false, 1, t, -1);
    txt(x + 44, y + 3, s, C_OURO, true);
    gfx_texto(x + 44, y + 23, "escolha no começo de uma run", C_MARFIM_S, GFX_MIUDO, false);
}

// ---- Caixa de Fosforos: a mao que o oponente comprou -------------------------
static uint8_t fosf_tipo[MAO], fosf_fixo[MAO];
static int8_t  fosf_idx[MAO];
static int     fosf_n;
static float   t_fosf;                   // desde que o fosforo riscou
static void fosforo_ve(void)
{
    fosf_n = 0;
    if (!tem_bolsa(0, B_FOSFORO)) return;
    const jogador_t *p = &J[1];
    for (int i = 0; i < p->n_mao && fosf_n < MAO; i++) {
        int c = p->mao[i];
        fosf_tipo[fosf_n] = p->col[c].tipo;
        fosf_fixo[fosf_n] = p->col[c].fixo;
        fosf_idx[fosf_n++] = (int8_t)c;
    }
    t_fosf = 0;
    som_toca(SOM_FOGO);
}

// Em cima da fila dele: os dados na luz do fosforo, e os ja lancados apagados.
static void desenha_fosforo(void)
{
    if (!fosf_n || !tem_bolsa(0, B_FOSFORO) || fase == F_FIM || fase == F_LOJA || fase == F_PREMIO) return;
    // A direita, antes do placar dele: o balao da fala fica a esquerda.
    int j = 1, passo = 19, w = fosf_n * passo, x0 = PLACAR_X - 44 - w + passo / 2;
    int y = (zona(j) < CENTRO ? MESA_Y0 + 13 : MESA_Y1 - 13);
    float acende = fminf(1, t_fosf / 0.35f);
    float treme = 0.85f + 0.15f * sinf(t_relogio * 17) * sinf(t_relogio * 5.3f);
    int alfa = (int)(70 * acende * treme);
    gfx_rect_alfa(x0 - passo / 2 - 22, y - 10, w + 30, 20, C_FOGO, alfa / 2);   // a luz quente
    gfx_rect_alfa(x0 - passo / 2 - 14, y - 8, w + 14, 16, C_AMARELO, alfa / 3);
    // O fosforo, de pe a esquerda, com a chama.
    int fx = x0 - passo / 2 - 12;
    gfx_rect(fx, y - 3, 2, 10, RGB(240, 200, 164));
    gfx_rect(fx - 1, y - 6, 4, 4, C_BRASA);
    int ch = (int)(t_relogio * 12) % 3;
    if (acende > 0.2f) {
        gfx_rect(fx - 1 + (ch == 0 ? -1 : ch == 2 ? 1 : 0), y - 11, 3, 5, C_FOGO);
        gfx_rect(fx, y - 13 + (ch == 1), 1, 4, C_AMARELO);
    }
    for (int i = 0; i < fosf_n; i++) {
        float q = (t_fosf - 0.15f - i * 0.05f) / 0.18f;
        if (q <= 0) continue;
        bool foi = false;                                 // ja lancado: apagado
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) foi |= F[j].idx[k] == fosf_idx[i];
        int x = x0 + i * passo;
        float r = 7 * (q < 1 ? sai_mola(q) : 1);
        peca_t p = { .tipo = fosf_tipo[i], .fixo = fosf_fixo[i] };
        peca_parada((float)x, (float)y, r, p, j);
        if (foi) gfx_rect_alfa(x - 9, y - 9, 19, 19, C_SOMBRA, 150);
        dica_zona(x - 8, y - 8, 16, 16, DICA_DADO, fosf_tipo[i]);
    }
}
