// ===========================================================================
// O poder armado de cada dado, animado: alem da borda de luz (brilho_poder),
// um desenho miudo e discreto que diz o que ele vai fazer no fim da rodada
// - o coracao do Caridoso batendo, a mira do Laser indo de um lado ao outro,
// o martelo do Ferreiro batendo, a areia da Ampulheta caindo...
// Os desenhos sao de pontos, com o contorno escuro em volta, como os dados.
// ===========================================================================

// Linhas de um desenho: '#' a cor, '+' o claro, 'o' o escuro, '.' nada.
static const char *const SP_CORACAO[] = { ".##.##.", "#+#####", "#######", ".#####.", "..###..", "...#..." };
static const char *const SP_CORACAO_G[] = { ".###.###.", "#++######", "#+#######", "#########", ".#######.",
                                            "..#####..", "...###...", "....#...." };
static const char *const SP_MIRA[] = { "...#...", ".##.##.", ".#...#.", "#..+..#", ".#...#.", ".##.##.", "...#..." };
static const char *const SP_MARTELO[] = { "o####o.", "o#++#o.", "o####o.", "...#...", "...#...", "...#...", "...#..." };
static const char *const SP_MOEDA[] = { ".###.", "#++#o", "#+##o", "##ooo", ".ooo." };
static const char *const SP_CHAMA[] = { "..#..", ".##..", ".#+#.", "##+##", "#+++#", "#+o+#", ".###." };
static const char *const SP_TREVO[] = { ".##.##.", "#+##+##", ".##.##.", "...o...", ".##.##.", "#+##+##", ".##.##." };
static const char *const SP_CAVEIRA[] = { ".#####.", "#######", "#oo#oo#", "#oo#oo#", ".##.##.", "..###..", "..#.#.." };
static const char *const SP_ESPINHO[] = { "...#...", ".#.#.#.", "..###..", "###+###", "..###..", ".#.#.#.", "...#..." };
static const char *const SP_ESPADAS[] = { "#.....#", ".#...#.", "..#.#..", "...#...", "..#.#..", "oo...oo", "oo...oo" };
static const char *const SP_OLHO[] = { ".#####.", "#+++++#", "#++o++#", "#+++++#", ".#####." };
static const char *const SP_OLHO_F[] = { ".......", ".......", "#######", ".......", "......." };
static const char *const SP_ANCORA[] = { "..o#o..", "...#...", ".#####.", "...#...", "#..#..#", ".#.#.#.", "..###.." };
static const char *const SP_SINO[] = { "...#...", "..###..", ".##+##.", ".#+###.", ".#####.", "#######", "...o..." };
static const char *const SP_FLOCO[] = { "...#...", ".#.#.#.", "..###..", "###+###", "..###..", ".#.#.#.", "...#..." };
static const char *const SP_ONDA[] = { "...##..", "..#++#.", ".#+..#.", "#+...#.", "#######" };
static const char *const SP_PATO[] = { "..##...", ".#o#++.", "..##...", "#####..", "######.", ".####.." };
static const char *const SP_MEIA[] = { ".+++.", ".###.", ".###.", ".###.", ".####", "#####", "###.." };
static const char *const SP_ESTRELA[] = { "...#...", "...#...", "###+###", ".#####.", "..###..", ".##.##.", ".#...#." };
static const char *const SP_CHIFRES[] = { "#.#...#.#", ".##...##.", "..#...#..", "...#o#...", "...###..." };
static const char *const SP_TRENO[] = { "#........", "#.+++++..", ".#######.", "..o...o..", "ooooooooo" };
static const char *const SP_COROA[] = { "#..#..#", "##.#.##", "#######", "#+#+#+#", "#######" };
static const char *const SP_COBRA[] = { ".###...", "#o#o#..", "######.", "...###.", "....##." };
static const char *const SP_COBRA_L[] = { ".###...", "#o#o#..", "######o", "...###.", "....##." };
static const char *const SP_MORCEGO[] = { "#...#...#", "##.###.##", ".#######.", "..#.#.#.." };
static const char *const SP_MORCEGO_B[] = { "....#....", ".#.###.#.", "#########", "#.#.#.#.#" };
static const char *const SP_FANTASMA[] = { "..###..", ".#####.", "##o#o##", "#######", "#######", "#######", "#.#.#.#" };
static const char *const SP_BALA[] = { ".oooo..", "ooooo#+", ".oooo.." };
static const char *const SP_DOMINO[] = { "+++++", "+o+++", "+++o+", "ooooo", "+o+o+", "+++++", "+o+o+" };
static const char *const SP_PA[] = { "..#..", "..#..", "..#..", "..#..", ".ooo.", "o+++o", "o+++o", ".ooo." };
static const char *const SP_HEXA[] = { "..###..", ".#+++#.", "#+o+o+#", "#++o++#", "#+o+o+#", ".#+++#.", "..###.." };
static const char *const SP_CHAVE[] = { ".#...#.", ".##.##.", "..###..", "...#...", "...#...", "...#...", "...#..." };
static const char *const SP_NUVEM[] = { "..##...", ".####..", "#######", ".#####." };
static const char *const SP_RAIO[] = { "..#", ".#.", "###", ".#.", "#.." };
static const char *const SP_VELA[] = { ".+.", "+o+", ".#.", "###", "###", "###", "###" };
static const char *const SP_VELA_B[] = { "+..", "+o.", ".#.", "###", "###", "###", "###" };
static const char *const SP_PICARETA[] = { ".#####.", "#.....#", "...o...", "...o...", "...o...", "...o..." };
static const char *const SP_BANDEIRA[] = { "#o+o+", "#+o+o", "#o+o+", "#....", "#....", "#...." };
static const char *const SP_BANDEIRA_B[] = { "#.o+o+", "#+o+o.", "#.o+o+", "#.....", "#.....", "#....." };
static const char *const SP_LUA[] = { ".###.", "##...", "#....", "#....", "##...", ".###." };
static const char *const SP_FLAMULA[] = { "#####..", "#++++#.", "#+++++#", "#++++#.", "#####..", "#......", "#......" };
static const char *const SP_FLAMULA_B[] = { "####...", "#+++##.", "#+++++#", "#++++#.", "######.", "#......", "#......" };
static const char *const SP_RAIVA[] = { ".#...#.", "##...##", ".......", ".......", "##...##", ".#...#." };
static const char *const SP_BOCA[] = { ".###.", "####.", "###..", "####.", ".###." };
static const char *const SP_BOCA_F[] = { ".###.", "#####", "#####", "#####", ".###." };
static const char *const SP_CARTA[] = { "#####", "#+++#", "#+o+#", "#ooo#", "#+o+#", "#+++#", "#####" };
static const char *const SP_ESCUDO[] = { "#######", "#+++++#", "#+++++#", "#+++++#", ".#+++#.", "..#+#..", "...#..." };
static const char *const SP_BALANCA[] = { "...#...", "#######", "#..#..#", "o..#..o", "...#...", "..###.." };
static const char *const SP_GALAO[] = { "#.....#", "##...##", "+##.##+", ".+###+.", "..+#+.." };
static const char *const SP_BOLSA[] = { "..#.#..", "...#...", ".#####.", "##+####", "#+#####", "#######", ".#####." };
static const char *const SP_MASCARA[] = { "#######", "#o###o#", ".#...#." };
static const char *const SP_COPIA[] = { "####..", "#++###", "#+#++#", "###++#", "..####" };
static const char *const SP_TRIGO[] = { "..#..", ".#+#.", "..#..", ".#+#.", "..#..", ".#+#.", "..#.." };
static const char *const SP_ADAGA[] = { ".+.", ".#.", ".#.", ".#.", "ooo", ".o.", ".o." };
static const char *const SP_FICHA_V[] = { ".###.", "#+o+#", "#ooo#", "#+o+#", ".###." };
static const char *const SP_TAMBOR[] = { ".###.", "#+o+#", "#o#o#", "#+o+#", ".###." };
static const char *const SP_ARANHA[] = { "#.#.#.#", ".#####.", "##o#o##", ".#####.", "#.#.#.#" };
static const char *const SP_IGUAL[] = { "#####", ".....", "#####" };
static const char *const SP_EXCLAMA[] = { "##", "##", "##", "..", "##" };
static const char *const SP_SETAS[] = { "#...#..", ".#...#.", "..#...#", ".#...#.", "#...#.." };
static const char *const SP_PRISMA[] = { "...#...", "..#+#..", ".#+++#.", "#######" };
static const char *const SP_PANETONE[] = { ".###.", "#+o+#", "#o+o#", "#####" };

enum { MV_FLUTUA, MV_PULSA, MV_BATIDA, MV_VARRE, MV_SOBE, MV_CAI, MV_BATE, MV_BALANCA, MV_TREME,
       MV_QUADROS, MV_PISCADA, MV_CHAMA, MV_ESPECIAL };

typedef struct {
    const char *const *sp, *const *sp2;  // o desenho e o outro quadro (batida, piscada, quadros)
    uint8_t h, h2, mv;
    uint16_t c1, c2, c3;                 // a cor, o claro e o escuro
} poder_anima_t;

#define PA_SP(a) a, NULL, sizeof a / sizeof a[0], 0
#define PA_SP2(a, b) a, b, sizeof a / sizeof a[0], sizeof b / sizeof b[0]
static const poder_anima_t PODER_ANIMA[EF_UNIVERSO + 1] = {
    [EF_CARIDOSO]     = { PA_SP2(SP_CORACAO, SP_CORACAO_G), MV_BATIDA, C_VINHO_CLR, C_ROSA, C_VINHO },
    [EF_LASER]        = { PA_SP(SP_MIRA), MV_ESPECIAL, C_LASER, C_BRANCO, C_LASER },
    [EF_FERREIRO]     = { PA_SP(SP_MARTELO), MV_BATE, C_FERRO, C_PRATA, C_TERRA },
    [EF_MARTELO]      = { PA_SP(SP_ESPINHO), MV_PULSA, C_VERDE, C_LASER, C_VERDE },         // o Espinhoso
    [EF_PIRATA]       = { PA_SP(SP_ESPADAS), MV_TREME, C_PRATA, C_BRANCO, C_TERRA },
    [EF_CARRASCO]     = { PA_SP(SP_CAVEIRA), MV_PULSA, C_MARFIM, C_BRANCO, C_EBANO },
    [EF_MALDICAO]     = { PA_SP(SP_CAVEIRA), MV_TREME, C_REAL, C_ROSA, C_EBANO },          // o Maldito
    [EF_INVEJOSO]     = { PA_SP2(SP_OLHO, SP_OLHO_F), MV_PISCADA, C_VERDE, C_BRANCO, C_EBANO },
    [EF_ESPELHO]      = { NULL, NULL, 0, 0, MV_ESPECIAL, C_BRANCO, C_BRANCO, C_PRATA },
    [EF_FICHAS]       = { PA_SP(SP_MOEDA), MV_PULSA, C_OURO, C_AMARELO, C_LATAO_ESC },
    [EF_COBRADOR]     = { PA_SP(SP_BOLSA), MV_ESPECIAL, C_TERRA, C_OURO, C_MADEIRA },
    [EF_AGIOTA]       = { PA_SP(SP_MOEDA), MV_VARRE, C_OURO, C_AMARELO, C_LATAO_ESC },
    [EF_POTE]         = { PA_SP(SP_PANETONE), MV_ESPECIAL, C_TERRA, C_FOGO, C_MADEIRA },
    [EF_GARIMPEIRO]   = { PA_SP(SP_PICARETA), MV_BATE, C_FERRO, C_PRATA, C_TERRA },
    [EF_TUDO_NADA]    = { PA_SP(SP_EXCLAMA), MV_PISCADA, C_OURO, C_AMARELO, C_OURO },
    [EF_GATUNO]       = { PA_SP(SP_MASCARA), MV_BALANCA, C_EBANO_B, C_PRATA, C_BRANCO },
    [EF_FARTURA]      = { PA_SP(SP_TRIGO), MV_BALANCA, C_OURO, C_AMARELO, C_LATAO_ESC },
    [EF_SEMENTE]      = { NULL, NULL, 0, 0, MV_ESPECIAL, C_VERDE, C_LASER, C_TERRA },
    [EF_FOGO]         = { PA_SP(SP_CHAMA), MV_CHAMA, C_FOGO, C_AMARELO, C_BRANCO },
    [EF_FENIX]        = { PA_SP(SP_CHAMA), MV_SOBE, C_FOGO, C_AMARELO, C_BRANCO },
    [EF_TREVO]        = { PA_SP(SP_TREVO), MV_FLUTUA, C_VERDE, C_LASER, C_TERRA },
    [EF_JACKPOT]      = { NULL, NULL, 0, 0, MV_ESPECIAL, C_OURO, C_VERMELHO, C_AMARELO },
    [EF_COPIADOR]     = { PA_SP(SP_COPIA), MV_ESPECIAL, C_PRATA, C_BRANCO, C_ACO },
    [EF_DOBRO]        = { NULL, NULL, 0, 0, MV_ESPECIAL, C_OURO, C_AMARELO, C_OURO },
    [EF_AZARAO]       = { NULL, NULL, 0, 0, MV_ESPECIAL, C_OURO, C_AMARELO, C_OURO },
    [EF_ESPERANCA]    = { PA_SP2(SP_VELA, SP_VELA_B), MV_QUADROS, C_MARFIM, C_AMARELO, C_FOGO },
    [EF_VIUVA]        = { PA_SP(SP_ARANHA), MV_ESPECIAL, C_EBANO_B, C_VERMELHO, C_VERMELHO },
    [EF_SACRIFICIO]   = { PA_SP(SP_ADAGA), MV_BATE, C_PRATA, C_BRANCO, C_VINHO },
    [EF_PAR]          = { PA_SP(SP_IGUAL), MV_PULSA, C_OURO, C_AMARELO, C_OURO },
    [EF_TRONO]        = { PA_SP(SP_COROA), MV_PULSA, C_OURO, C_AMARELO, C_VERMELHO },
    [EF_DESAFIANTE]   = { PA_SP2(SP_FLAMULA, SP_FLAMULA_B), MV_QUADROS, C_CARMIM, C_ROSA, C_EBANO },
    [EF_DOMINO]       = { PA_SP(SP_DOMINO), MV_BALANCA, C_EBANO, C_MARFIM, C_EBANO },
    [EF_ANCORA]       = { PA_SP(SP_ANCORA), MV_BALANCA, C_ACO, C_PRATA, C_FERRO },
    [EF_CALIBRE]      = { PA_SP(SP_BALA), MV_ESPECIAL, C_OURO, C_AMARELO, C_LATAO_ESC },
    [EF_PUXA]         = { PA_SP(SP_SETAS), MV_ESPECIAL, C_OURO, C_AMARELO, C_OURO },
    [EF_CHECKPOINT]   = { PA_SP2(SP_BANDEIRA, SP_BANDEIRA_B), MV_QUADROS, C_EBANO_B, C_BRANCO, C_EBANO },
    [EF_COVEIRO]      = { PA_SP(SP_PA), MV_BATE, C_TERRA, C_PRATA, C_FERRO },
    [EF_HEXA]         = { PA_SP(SP_HEXA), MV_PULSA, C_MARFIM, C_BRANCO, C_EBANO },
    [EF_OURICO]       = { PA_SP(SP_ESPINHO), MV_TREME, C_TERRA, C_MARFIM_S, C_MADEIRA },
    [EF_SABOTADOR]    = { PA_SP(SP_CHAVE), MV_BALANCA, C_ACO, C_PRATA, C_FERRO },
    [EF_AMPULHETA]    = { NULL, NULL, 0, 0, MV_ESPECIAL, C_LATAO, C_AMARELO, C_LATAO_ESC },
    [EF_MEDUSA]       = { PA_SP2(SP_COBRA, SP_COBRA_L), MV_PISCADA, C_VERDE, C_LASER, C_VERMELHO },
    [EF_RANCOR]       = { NULL, NULL, 0, 0, MV_ESPECIAL, C_CHUMBO, C_AMARELO, C_FERRO },
    [EF_SOLITARIO]    = { PA_SP(SP_LUA), MV_FLUTUA, C_AMARELO, C_BRANCO, C_OURO },
    [EF_BERSERKER]    = { PA_SP(SP_RAIVA), MV_TREME, C_VERMELHO, C_ROSA, C_VINHO },
    [EF_AUTOFAGIA]    = { PA_SP2(SP_BOCA, SP_BOCA_F), MV_QUADROS, C_VINHO_CLR, C_ROSA, C_VINHO },
    [EF_COLECIONADOR] = { PA_SP(SP_CARTA), MV_ESPECIAL, C_MARFIM, C_BRANCO, C_VERMELHO },
    [EF_BURACO]       = { NULL, NULL, 0, 0, MV_ESPECIAL, C_VIOLETA, C_REAL, C_BRANCO },
    [EF_BONECO]       = { PA_SP(SP_FLOCO), MV_CAI, C_VENTO, C_BRANCO, C_VENTO },
    [EF_TSUNAMI]      = { PA_SP(SP_ONDA), MV_BALANCA, C_AZUL, C_VENTO, C_AZUL },
    [EF_ONDA]         = { PA_SP(SP_ONDA), MV_FLUTUA, C_VENTO, C_BRANCO, C_AZUL },
    [EF_PATINHOS]     = { PA_SP(SP_PATO), MV_FLUTUA, C_AMARELO, C_FOGO, C_EBANO },
    [EF_PISCA]        = { NULL, NULL, 0, 0, MV_ESPECIAL, C_VERMELHO, C_VERDE, C_AMARELO },
    [EF_MEIA]         = { PA_SP(SP_MEIA), MV_BALANCA, C_VERMELHO, C_BRANCO, C_VINHO },
    [EF_ESTRELA]      = { PA_SP(SP_ESTRELA), MV_PULSA, C_OURO, C_AMARELO, C_OURO },
    [EF_RENA]         = { PA_SP(SP_CHIFRES), MV_BALANCA, C_TERRA, C_MARFIM_S, C_VERMELHO },
    [EF_TRENO]        = { PA_SP(SP_TRENO), MV_VARRE, C_VERMELHO, C_OURO, C_LATAO },
    [EF_JINGLE]       = { PA_SP(SP_SINO), MV_BALANCA, C_OURO, C_AMARELO, C_LATAO_ESC },
    [EF_SORTE]        = { PA_SP(SP_MOEDA), MV_ESPECIAL, C_OURO, C_AMARELO, C_LATAO_ESC },
    [EF_MISERICORDIA] = { NULL, NULL, 0, 0, MV_ESPECIAL, C_AMARELO, C_BRANCO, C_OURO },
    [EF_MARTIR]       = { NULL, NULL, 0, 0, MV_ESPECIAL, C_AMARELO, C_BRANCO, C_OURO },
    [EF_PRISMA]       = { PA_SP(SP_PRISMA), MV_ESPECIAL, C_PRATA, C_BRANCO, C_ACO },
    [EF_CAPITAO]      = { PA_SP(SP_GALAO), MV_FLUTUA, C_OURO, C_AMARELO, C_LATAO_ESC },
    [EF_SEGURANCA]    = { PA_SP(SP_ESCUDO), MV_PULSA, C_ACO, C_PRATA, C_FERRO },
    [EF_MEDIA]        = { PA_SP(SP_BALANCA), MV_BALANCA, C_LATAO, C_OURO, C_LATAO_ESC },
    [EF_ASSOMBRA]     = { PA_SP(SP_FANTASMA), MV_FLUTUA, C_MARFIM_S, C_BRANCO, C_EBANO },
    [EF_VAMPIRO]      = { PA_SP2(SP_MORCEGO, SP_MORCEGO_B), MV_QUADROS, C_VIOLETA, C_REAL, C_VERMELHO },
    [EF_FICHA_VELHA]  = { PA_SP(SP_FICHA_V), MV_PULSA, C_TERRA, C_OURO, C_MADEIRA },
    [EF_REVOLVER]     = { PA_SP(SP_TAMBOR), MV_ESPECIAL, C_FERRO, C_PRATA, C_EBANO },
};
#undef PA_SP
#undef PA_SP2

// O desenho centrado em (cx, cy). espelha: de lado trocado; some (0..1): o
// quanto ja sumiu, no pontilhado.
static void sprite_em(int cx, int cy, const char *const *l, int h, uint16_t c1, uint16_t c2, uint16_t c3,
                      bool espelha, float some)
{
    int w = (int)strlen(l[0]), x0 = cx - w / 2, y0 = cy - h / 2;
#define SP_CHEIO(i, j) ((i) >= 0 && (j) >= 0 && (i) < w && (j) < h && l[j][espelha ? w - 1 - (i) : (i)] != '.')
    for (int j = -1; j <= h; j++)                  // o contorno: o vazio que encosta no desenho
        for (int i = -1; i <= w; i++) {
            if (SP_CHEIO(i, j) || !(SP_CHEIO(i - 1, j) || SP_CHEIO(i + 1, j) || SP_CHEIO(i, j - 1) || SP_CHEIO(i, j + 1)))
                continue;
            if (some > 0 && bayer(x0 + i, y0 + j) < some) continue;
            gfx_pixel(x0 + i, y0 + j, C_BARRA);
        }
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            char c = l[j][espelha ? w - 1 - i : i];
            if (c == '.' || (some > 0 && bayer(x0 + i, y0 + j) < some)) continue;
            gfx_pixel(x0 + i, y0 + j, c == '+' ? c2 : c == 'o' ? c3 : c1);
        }
#undef SP_CHEIO
}

// Um rotulo miudo (o "x2"), com contorno, centrado em (cx, cy).
static void rotulo_poder(int cx, int cy, const char *s, uint16_t c)
{
    int w = gfx_largura(s, GFX_MIUDO), x = cx - w / 2, y = cy - 6;
    static const int8_t D[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {1,-1}, {-1,1}, {1,1} };
    for (int i = 0; i < 8; i++) gfx_texto(x + D[i][0], y + D[i][1], s, C_BARRA, GFX_MIUDO, false);
    gfx_texto(x, y, s, c, GFX_MIUDO, false);
}

// A animacao do poder do dado (x, y) de raio R, armado. fase: para os dados
// lado a lado nao baterem juntos.
static void anima_poder(int x, int y, int R, int tipo, float fase)
{
    int ef = TIPO[tipo].efeito;
    if (ef < 0 || ef > EF_UNIVERSO) return;
    const poder_anima_t *a = &PODER_ANIMA[ef];
    if (!a->sp && a->mv != MV_ESPECIAL) return;
    float t = t_relogio + fase;
    int ax = x, ay = y - R - 12;                   // o lugar do desenho: em cima do dado, livre da borda de luz
    switch (a->mv) {
    case MV_FLUTUA:                                // sobe e desce devagar
        sprite_em(ax, ay + (int)lroundf(sinf(t * 2.4f) * 1.5f), a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
        break;
    case MV_PULSA:                                 // um pulinho e o brilho a cada batida
        { float s = fmodf(t * 1.1f, 1.0f);
          bool alto = s < 0.18f;
          sprite_em(ax, ay - alto, a->sp, a->h, alto ? a->c2 : a->c1, alto ? C_BRANCO : a->c2, a->c3, false, 0); }
        break;
    case MV_BATIDA:                                // o coracao: tum-tum, e descansa
        { float s = fmodf(t * 0.9f, 1.0f);
          bool bate = s < 0.1f || (s > 0.2f && s < 0.3f);
          if (bate && a->sp2) sprite_em(ax, ay - 1, a->sp2, a->h2, a->c1, a->c2, a->c3, false, 0);
          else sprite_em(ax, ay, a->sp, a->h, a->c1, a->c2, a->c3, false, 0); }
        break;
    case MV_VARRE:                                 // vai de um lado ao outro
        sprite_em(ax + (int)lroundf(sinf(t * 2.0f) * (R - 6)), ay, a->sp, a->h, a->c1, a->c2, a->c3,
                  cosf(t * 2.0f) < 0, 0);
        break;
    case MV_SOBE:                                  // sai do dado, sobe e some
        { float q = fmodf(t * 0.6f, 1.0f);
          sprite_em(ax, y - R - 6 - (int)(q * 14), a->sp, a->h, a->c1, a->c2, a->c3, false, q > 0.55f ? (q - 0.55f) / 0.45f : 0); }
        break;
    case MV_CAI:                                   // cai devagar em cima do dado
        { float q = fmodf(t * 0.5f, 1.0f);
          float some = q < 0.2f ? 1 - q / 0.2f : q > 0.8f ? (q - 0.8f) / 0.2f : 0;
          sprite_em(ax + (int)lroundf(sinf(t * 3) * 2), y - R - 22 + (int)(q * 12), a->sp, a->h, a->c1, a->c2, a->c3, false, some); }
        break;
    case MV_BATE:                                  // bate: desce de uma vez, solta faisca e volta
        { float q = fmodf(t * 1.2f, 1.0f);
          int d = q < 0.1f ? 3 : q < 0.2f ? 2 : 0;
          sprite_em(ax, ay + d, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
          if (q < 0.14f) { gfx_pixel(ax - 5, ay + a->h / 2 + 3, C_AMARELO); gfx_pixel(ax + 5, ay + a->h / 2 + 2, C_AMARELO); } }
        break;
    case MV_BALANCA:                               // balanca de um lado para o outro
        sprite_em(ax + (int)lroundf(sinf(t * 2.6f) * 2), ay, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
        break;
    case MV_TREME:                                 // treme, nervoso
        { int f = (int)(t * 14);
          sprite_em(ax + (f % 3) - 1, ay + ((f / 3) % 2), a->sp, a->h, a->c1, a->c2, a->c3, false, 0); }
        break;
    case MV_QUADROS:                               // dois quadros alternando (asa, chama, bandeira)
        { bool b = fmodf(t * 2.6f, 1.0f) < 0.5f && a->sp2;
          if (b) sprite_em(ax, ay, a->sp2, a->h2, a->c1, a->c2, a->c3, false, 0);
          else sprite_em(ax, ay, a->sp, a->h, a->c1, a->c2, a->c3, false, 0); }
        break;
    case MV_PISCADA:                               // o segundo quadro so de relance (o olho pisca, a lingua sai);
        { bool b = fmodf(t * 0.6f, 1.0f) < 0.12f;  // sem segundo quadro, ele apaga um instante
          if (b && a->sp2) sprite_em(ax, ay, a->sp2, a->h2, a->c1, a->c2, a->c3, false, 0);
          else if (!b) sprite_em(ax, ay, a->sp, a->h, a->c1, a->c2, a->c3, false, 0); }
        break;
    case MV_CHAMA:                                 // a chama: tremula de lado e sobe um nada
        { int f = (int)(t * 7);
          sprite_em(ax, ay - (f % 2), a->sp, a->h, a->c1, a->c2, a->c3, f % 3 == 0, 0); }
        break;
    case MV_ESPECIAL:
        switch (ef) {
        case EF_LASER: {                           // a mira passa de um lado ao outro, com o ponto do laser
            float s = sinf(t * 1.6f);
            int mx = ax + (int)lroundf(s * (R - 4));
            sprite_em(mx, ay, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
            for (int i = 1; i <= 3; i++)          // o rastro, apagando
                if (bayer(mx - i * 3, ay) < 0.6f - i * 0.15f) gfx_pixel(mx - (int)(i * 3 * (s > 0 ? 1 : -1)), ay, C_LASER);
            break;
        }
        case EF_ESPELHO: {                         // um reflexo atravessa o dado, na diagonal
            float q = fmodf(t * 0.55f, 1.0f) * 3 - 1;   // de -1 a 2: a maior parte do tempo, fora
            if (q < -0.2f || q > 1.2f) break;
            int d = (int)lroundf(q * 2.4f * R) - R;
            uint16_t *fb = gfx_alvo() ? gfx_alvo() : display_fb();
            for (int i = -R; i <= R; i++) {
                int px = x + d + i, py = y + i;
                for (int w = 0; w < 2; w++) {
                    int X = px + w, Y = py;
                    if (X < 0 || Y < 0 || X >= GFX_W || Y >= GFX_H || abs(X - x) > R - 2 || abs(Y - y) > R - 2) continue;
                    uint16_t c = fb[Y * GFX_W + X];
                    if (c == C_BARRA || c == mesa_acesa[Y * GFX_W + X] || c == C_SOMBRA) continue;   // so no dado
                    if (w == 0 || bayer(X, Y) < 0.5f) fb[Y * GFX_W + X] = C_BRANCO;
                }
            }
            break;
        }
        case EF_COBRADOR: {                        // a bolsa e uma moeda caindo nela
            sprite_em(ax, ay + 1, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
            float q = fmodf(t * 0.8f, 1.0f);
            if (q < 0.6f) sprite_em(ax + 1, ay - 9 + (int)(q * 10), SP_MOEDA, 5, C_OURO, C_AMARELO, C_LATAO_ESC, false,
                                    q > 0.45f ? (q - 0.45f) / 0.15f : 0);
            break;
        }
        case EF_POTE: {                            // a panela e o vapor subindo
            sprite_em(ax, ay + 2, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
            for (int i = 0; i < 3; i++) {
                float q = fmodf(t * 0.7f + i * 0.33f, 1.0f);
                int vx = ax - 2 + i * 2 + (int)lroundf(sinf(t * 4 + i * 2) * 1.2f), vy = ay - 1 - (int)(q * 8);
                if (bayer(vx, vy) >= q) gfx_pixel(vx, vy, C_MARFIM_S);
            }
            break;
        }
        case EF_SEMENTE: {                         // o broto cresce do chao do dado e abre as folhas
            float q = fmodf(t * 0.45f, 1.0f);
            int hcaule = 1 + (int)(fminf(q / 0.5f, 1) * 5), base = ay + 4;
            gfx_rect(ax - 1, base - hcaule - 1, 3, hcaule + 2, C_BARRA);
            gfx_rect(ax, base - hcaule, 1, hcaule + 1, a->c1);
            if (q > 0.5f) {
                static const char *const FOLHAS[] = { "##.##", "#+.+#", ".#.#." };
                sprite_em(ax, base - hcaule - 1, FOLHAS, 3, a->c1, a->c2, a->c3, false, q > 0.9f ? (q - 0.9f) / 0.1f : 0);
            }
            gfx_rect(ax - 3, base + 1, 7, 2, C_BARRA);
            gfx_rect(ax - 2, base + 1, 5, 1, a->c3);
            break;
        }
        case EF_JACKPOT: {                         // o 7 da maquina, piscando em ouro e vermelho
            bool b = fmodf(t * 1.8f, 1.0f) < 0.5f;
            rotulo_poder(ax, ay, "777", b ? a->c1 : a->c2);
            break;
        }
        case EF_COPIADOR: {                        // as duas folhas se afastam e se juntam
            int d = (int)lroundf((sinf(t * 2.2f) + 1) * 1.2f);
            sprite_em(ax - d, ay - d, SP_CARTA, 7, a->c3, a->c1, a->c3, false, 0);
            sprite_em(ax + d, ay + d, SP_CARTA, 7, a->c1, a->c2, a->c3, false, 0);
            break;
        }
        case EF_DOBRO: {                           // o x2 vai para o dado seguinte
            float q = fmodf(t * 0.7f, 1.0f);
            rotulo_poder(ax + (int)(q * 14) - 4, ay, "x2", q < 0.75f ? a->c1 : a->c2);
            break;
        }
        case EF_AZARAO: {                          // o x2 de baixo para cima
            float s = fmodf(t * 1.1f, 1.0f);
            rotulo_poder(ax, ay + (s < 0.2f ? -1 : 0), "x2", s < 0.2f ? a->c2 : a->c1);
            break;
        }
        case EF_VIUVA: {                           // a aranha desce e sobe pelo fio
            int d = (int)lroundf((sinf(t * 1.6f) + 1) * 2);
            gfx_rect(ax, ay - 9, 1, d + 5, C_MARFIM_S);
            sprite_em(ax, ay - 3 + d, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
            break;
        }
        case EF_CALIBRE: {                         // a bala sai para o dado seguinte
            float q = fmodf(t * 0.8f, 1.0f);
            sprite_em(ax + (int)(q * 16) - 6, ay, a->sp, a->h, a->c1, a->c2, a->c3, false, q > 0.7f ? (q - 0.7f) / 0.3f : 0);
            break;
        }
        case EF_PUXA: {                            // as setas puxam a fila para a frente
            float q = fmodf(t * 1.2f, 1.0f);
            sprite_em(ax + (int)(q * 6) - 3, ay, a->sp, a->h, q < 0.5f ? a->c1 : a->c2, a->c2, a->c3, false, 0);
            break;
        }
        case EF_AMPULHETA: {                       // a areia escorre de cima para baixo, e recomeca
            static const char *const VIDRO[] = { "#####", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#####" };
            static const int8_t CIMA[4][2] = { { 0, -1 }, { 0, -2 }, { -1, -2 }, { 1, -2 } };   // os graos, do fundo para o alto
            static const int8_t BAIXO[4][2] = { { 0, 2 }, { -1, 2 }, { 1, 2 }, { 0, 1 } };
            sprite_em(ax, ay, VIDRO, 7, a->c1, a->c2, a->c3, false, 0);
            int fica = 4 - (int)(fmodf(t * 0.35f, 1.0f) * 5);
            for (int i = 0; i < fica; i++) gfx_pixel(ax + CIMA[i][0], ay + CIMA[i][1], a->c2);
            for (int i = 0; i < 4 - fica; i++) gfx_pixel(ax + BAIXO[i][0], ay + BAIXO[i][1], a->c2);
            if (fica > 0 && fmodf(t * 6, 1.0f) < 0.5f) gfx_pixel(ax, ay, a->c2);   // o fio de areia no gargalo
            break;
        }
        case EF_RANCOR: {                          // a nuvem carregada e, de vez em quando, o raio
            sprite_em(ax, ay - 2, SP_NUVEM, 4, a->c1, a->c2, a->c3, false, 0);
            float q = fmodf(t * 0.7f, 1.0f);
            if (q < 0.08f || (q > 0.14f && q < 0.2f)) sprite_em(ax + 1, ay + 4, SP_RAIO, 5, a->c2, C_BRANCO, a->c2, false, 0);
            break;
        }
        case EF_COLECIONADOR: {                    // a figurinha e um brilho girando em volta
            sprite_em(ax, ay, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
            float an = t * 3.0f;
            estrela4(ax + (int)lroundf(cosf(an) * 6), ay + (int)lroundf(sinf(an) * 5), 1, C_BRANCO, C_AMARELO, C_AMARELO);
            break;
        }
        case EF_BURACO: {                          // os pontos girando para dentro, em espiral
            for (int i = 0; i < 6; i++) {
                float q = fmodf(t * 0.5f + i / 6.0f, 1.0f), an = t * 2.5f + i * 1.05f + q * 3;
                float d = (1 - q) * 7 + 1;
                int px = ax + (int)lroundf(cosf(an) * d), py = ay + (int)lroundf(sinf(an) * d * 0.8f);
                gfx_pixel(px, py, q < 0.6f ? a->c2 : a->c1);
            }
            gfx_rect(ax - 1, ay - 1, 3, 3, C_BARRA);
            gfx_pixel(ax, ay, a->c1);
            break;
        }
        case EF_PISCA: {                           // o fio de luzinhas: uma acesa, outra apagada, trocando
            gfx_linha(ax - 9, ay - 2, ax + 9, ay - 2, C_EBANO_B);
            static const uint16_t COR[3] = { C_VERMELHO, C_VERDE, C_AMARELO };
            bool troca = fmodf(t * 1.6f, 1.0f) < 0.5f;
            for (int i = 0; i < 4; i++) {
                int bx = ax - 8 + i * 5 + 1, by = ay;
                bool acesa = (i % 2 == 0) == troca;
                gfx_rect(bx - 1, by - 2, 3, 4, C_BARRA);
                gfx_rect(bx, by - 1, 1, 2, acesa ? COR[i % 3] : gfx_mistura(COR[i % 3], C_BARRA, 60));
                if (acesa) gfx_pixel(bx, by - 1, C_BRANCO);
            }
            break;
        }
        case EF_SORTE: {                           // a moeda girando: de frente, de lado, de frente
            int f = (int)(t * 3.2f) % 4;
            static const char *const LADO[] = { ".#.", "#+o", "#+o", "#+o", ".o." };
            static const char *const FINA[] = { "#", "#", "o", "o", "o" };
            if (f == 0) sprite_em(ax, ay, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
            else if (f == 2) sprite_em(ax, ay, FINA, 5, a->c1, a->c2, a->c3, false, 0);
            else sprite_em(ax, ay, LADO, 5, a->c1, a->c2, a->c3, f == 3, 0);
            break;
        }
        case EF_MISERICORDIA: case EF_MARTIR: {    // a aureola em cima do dado, com um brilho correndo
            int rx = R / 2 + 2, cy = y - R - 8;
            float b = fmodf(t * 0.7f, 1.0f) * 6.2832f;
            for (int i = 0; i < 40; i++) {
                float an = i * 0.157f;
                int px = x + (int)lroundf(cosf(an) * rx), py = cy + (int)lroundf(sinf(an) * 2);
                float d = fabsf(fmodf(an - b + 9.4248f, 6.2832f) - 3.1416f);
                gfx_pixel(px, py + 1, C_BARRA);
                gfx_pixel(px, py, d > 2.7f ? C_BRANCO : (an < 3.1416f ? a->c3 : a->c1));
            }
            break;
        }
        case EF_PRISMA: {                          // o prisma e as tres cores saindo dele, correndo
            sprite_em(ax - 3, ay, a->sp, a->h, a->c1, a->c2, a->c3, false, 0);
            static const uint16_t ARCO[3] = { C_VERMELHO, C_AMARELO, C_VENTO };
            int f = (int)(t * 5);
            for (int i = 0; i < 3; i++)
                for (int k = 0; k < 4; k++)
                    if ((k + f) % 4 != 3) gfx_pixel(ax + 1 + k, ay - 1 + i, ARCO[i]);
            break;
        }
        case EF_REVOLVER: {                        // o tambor gira um dente de cada vez
            sprite_em(ax, ay, a->sp, a->h, a->c1, a->c2, a->c3, ((int)(t * 2.5f)) % 2, 0);
            break;
        }
        default:
            break;
        }
        break;
    }
}

// O desenho do poder, parado no lugar, centrado em (cx, cy): o da legenda do
// efeito, no fim da rodada, que liga o texto ao dado que esta agindo.
static bool icone_poder(int cx, int cy, int tipo)
{
    if (tipo < 0 || tipo >= D_N) return false;
    int ef = TIPO[tipo].efeito;
    if (ef <= 0 || ef > EF_UNIVERSO) return false;
    const poder_anima_t *a = &PODER_ANIMA[ef];
    if (a->sp) { sprite_em(cx, cy, a->sp, a->h, a->c1, a->c2, a->c3, false, 0); return true; }
    if (a->mv != MV_ESPECIAL) return false;
    if (ef == EF_ESPELHO) { estrela4(cx, cy, 3, C_BRANCO, C_PRATA, C_ACO); return true; }
    if (ef == EF_DOBRO) { rotulo_poder(cx, cy, "x2", a->c1); return true; }
    anima_poder(cx, cy + R_DADO + 12, R_DADO, tipo, 0);   // os outros animam ali mesmo, sem sair do lugar
    return true;
}
