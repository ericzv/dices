// ===========================================================================
// Exploracao: a proposta de cada adesivo (K troca com a de hoje)
// ===========================================================================
// Hoje todo adesivo e um recorte de papel: a margem branca, o contorno escuro
// e a sombra no dado. A proposta trata cada um como a coisa que ele e: o
// Corretivo e tinta pincelada, o Carimbo e a marca da tinta, o Selo e cera
// derramada, a Nota Dez e caneta vermelha; so os adesivos de verdade (a
// Figurinha, o Holografico, a Raspadinha...) ficam no recorte. O acabamento
// de cada um:
//   ACAB_RECORTE    o recorte de papel de hoje (a margem branca e a sombra);
//   ACAB_OBJETO     a coisa presa no dado: o contorno e a sombra, sem margem;
//   ACAB_TINTA      a tinta grossa: sem margem nem contorno; a beira em
//                   relevo e uma sombrinha no dado;
//   ACAB_TINTA_FINA a tinta rala (o carimbo, a caneta): sem sombra, falhando,
//                   o dado aparecendo por baixo;
//   ACAB_GOSMA      a massa mole (cera, goma, barro): o contorno no escuro da
//                   cor dela, sem margem;
//   ACAB_PECAS      pedacinhos soltos (o confete): so a sombrinha de cada um.
// E cada um aparece do seu jeito (rev), na colagem: de uma vez, do meio para
// fora, na diagonal, da esquerda para a direita, pontilhando, ou na ordem que
// a forma diz (as pinceladas, o traco da caneta, a cera derramando).
// A proposta so vale onde a tabela tem o adesivo (tam != 0); os outros
// seguem como hoje.
// ===========================================================================

enum { ACAB_RECORTE, ACAB_OBJETO, ACAB_TINTA, ACAB_TINTA_FINA, ACAB_GOSMA, ACAB_PECAS };
enum { REV_TUDO, REV_RADIAL, REV_VARRE, REV_ESQ, REV_CIMA, REV_PONTOS, REV_FORMA };

static int ad_versao = 1;                // 1: a proposta, onde houver; 0: a de hoje (K troca)

// forma, material, mat2, tam, giro, cor, cor2, acabamento, como aparece, altura (a sombra a mais)
static const ad_novo_t AD_NOVO2[AD_N] = {
    [AD_CORRETIVO]  = { FO_PINCEL,    MN_TINTA,      0,         84, -14, RGB(252, 248, 236), RGB(238, 230, 212), ACAB_TINTA,      REV_FORMA,  0 },
    [AD_CARIMBO]    = { FO_CARIMBO,   MN_TINTA_FINA, 0,         82,  -7, RGB(190, 56, 60),   RGB(118, 30, 38),   ACAB_TINTA_FINA, REV_TUDO,   0 },
    [AD_SELO]       = { FO_CERA,      MN_CERA,       0,         72,   0, RGB(118, 30, 38),   RGB(66, 18, 24),    ACAB_GOSMA,      REV_FORMA,  1 },
    [AD_RASPADINHA] = { FO_RETANGULO, MN_PAPEL,      0,         78,  -5, RGB(206, 180, 218), RGB(184, 194, 208), ACAB_RECORTE,    REV_TUDO,   0 },
    [AD_NOTA_DEZ]   = { FO_CANETA,    MN_TINTA_FINA, 0,         86,   0, RGB(190, 56, 60),   RGB(118, 30, 38),   ACAB_TINTA_FINA, REV_FORMA,  0 },
    // as massas moles: sem margem, o contorno no escuro da cor delas
    [AD_CHICLETE]   = { FO_GOMA,      MN_BORRACHA,   0,         78,   0, RGB(226, 164, 168), RGB(208, 98, 108),  ACAB_GOSMA,      REV_RADIAL, 1 },
    [AD_MASSINHA]   = { FO_MASSA,     MN_FOSCO,      0,         76,  12, RGB(206, 180, 218), RGB(150, 112, 160), ACAB_GOSMA,      REV_TUDO,   1 },
    [AD_BARRO]      = { FO_RESPINGO,  MN_LAMA,       0,         78,   0, RGB(156, 106, 64),  RGB(120, 78, 50),   ACAB_GOSMA,      REV_RADIAL, 0 },
    [AD_SUJEIRA]    = { FO_ESCORRIDO, MN_LAMA,       0,         76,   0, RGB(120, 78, 50),   RGB(78, 46, 30),    ACAB_GOSMA,      REV_CIMA,   0 },
    [AD_INFECTADO]  = { FO_GERME,     MN_BORRACHA,   0,         74,   0, RGB(130, 204, 112), RGB(88, 152, 84),   ACAB_GOSMA,      REV_FORMA,  0 },
    [AD_FERMENTO]   = { FO_PUFE,      MN_FOSCO,      0,         74,   0, RGB(246, 222, 132), RGB(238, 162, 74),  ACAB_GOSMA,      REV_TUDO,   1 },
    // os objetos presos no dado: sem margem, o contorno e a sombra (os grossos, mais alta)
    [AD_CHUMBO]     = { FO_PESO,      MN_CHUMBO,     MN_METAL,  80,   0, RGB(70, 76, 88),    RGB(104, 112, 124), ACAB_OBJETO,     REV_TUDO,   1 },
    [AD_REFLEXIVO]  = { FO_CIRCULO,   MN_CROMO,      0,         72,   0, RGB(184, 194, 208), RGB(104, 112, 124), ACAB_OBJETO,     REV_TUDO,   0 },
    [AD_ELETRONICO] = { FO_CHIP,      MN_BRILHO,     MN_METAL,  64,   0, RGB(36, 32, 29),    RGB(184, 194, 208), ACAB_OBJETO,     REV_TUDO,   1 },
    [AD_TRANSPARENTE] = { FO_QUADRADO, MN_VIDRO,     0,         74,  -6, RGB(168, 204, 230), 0,                  ACAB_PECAS,      REV_ESQ,    0 },
    [AD_REMENDO]    = { FO_QUADRADO,  MN_TECIDO,     0,         72,  10, RGB(88, 152, 84),   RGB(130, 204, 112), ACAB_OBJETO,     REV_TUDO,   0 },
    [AD_GORJETA]    = { FO_CIRCULO,   MN_BRILHO,     0,         66,   0, RGB(56, 86, 150),   RGB(238, 230, 212), ACAB_OBJETO,     REV_TUDO,   1 },
    [AD_IMANTADO]   = { FO_FERRADURA, MN_BRILHO,     MN_METAL,  80,   0, RGB(190, 56, 60),   RGB(184, 194, 208), ACAB_OBJETO,     REV_TUDO,   1 },
    [AD_RATOEIRA]   = { FO_TABUA,     MN_MADEIRA,    MN_FOSCO,  76,   0, RGB(120, 78, 50),   RGB(246, 222, 132), ACAB_OBJETO,     REV_TUDO,   1 },
    [AD_MIRA]       = { FO_MIRA,      MN_LENTE,      0,         64,   0, RGB(66, 18, 24),    RGB(208, 98, 108),  ACAB_OBJETO,     REV_TUDO,   0 },
    [AD_NEON]       = { FO_CORACAO,   MN_NEON,       0,         78,   0, RGB(36, 32, 29),    0,                  ACAB_OBJETO,     REV_TUDO,   0 },
    [AD_VELCRO]     = { FO_FELPUDO,   MN_TECIDO,     0,         74,   0, RGB(36, 32, 29),    0,                  ACAB_OBJETO,     REV_ESQ,    0 },
    [AD_PATINHO]    = { FO_PATO,      MN_FOSCO,      MN_BRILHO, 88,   0, RGB(128, 120, 112), RGB(168, 204, 230), ACAB_OBJETO,     REV_TUDO,   0 },
    [AD_CASCA]      = { FO_CASCA,     MN_BRILHO,     MN_FOSCO,  92,   0, RGB(246, 222, 132), RGB(120, 78, 50),   ACAB_OBJETO,     REV_TUDO,   1 },
    [AD_BOMBA]      = { FO_BOMBINHA,  MN_PAPEL,      0,         86, -26, RGB(190, 56, 60),   RGB(246, 222, 132), ACAB_OBJETO,     REV_TUDO,   1 },
    // as marcas e as coisas finas: as garras rasgadas, a folha de ouro, as fitas e os papeis
    [AD_LOBO]       = { FO_GARRAS,    MN_FOSCO,      0,         86,   0, RGB(66, 18, 24),    RGB(118, 30, 38),   ACAB_TINTA_FINA, REV_FORMA,  0 },
    [AD_DOURADO]    = { FO_FOLHA,     MN_OURO,       0,         74,   8, RGB(228, 194, 106), RGB(150, 122, 62),  ACAB_PECAS,      REV_TUDO,   0 },
    [AD_POSTIT]     = { FO_CANTO,     MN_PAPEL,      0,         74,  -5, RGB(246, 222, 132), RGB(228, 194, 106), ACAB_GOSMA,      REV_TUDO,   0 },
    [AD_FRAGIL]     = { FO_FITA2,     MN_VINIL,      0,         98, -24, RGB(214, 112, 44),  RGB(118, 30, 38),   ACAB_GOSMA,      REV_ESQ,    0 },
    [AD_DUPLA]      = { FO_ESPUMA,    MN_FOSCO,      MN_BRILHO, 72, -10, RGB(238, 230, 212), RGB(190, 56, 60),   ACAB_GOSMA,      REV_TUDO,   1 },
    [AD_CARBONO]    = { FO_FOLHAS,    MN_VINIL,      MN_PAPEL,  70,  -4, RGB(26, 34, 62),    RGB(252, 248, 236), ACAB_GOSMA,      REV_TUDO,   0 },
    // os soltos (o confete, a purpurina) e o verniz do Impermeavel
    [AD_CONFETE]    = { FO_CONFETE,   MN_PAPEL,      0,         92,   0, RGB(252, 248, 236), 0,                  ACAB_PECAS,      REV_FORMA,  0 },
    [AD_PURPURINA]  = { FO_PURPURINA, MN_PURPURINA,  0,         86,   0, RGB(150, 62, 80),   0,                  ACAB_PECAS,      REV_RADIAL, 0 },
    [AD_IMPERMEAVEL] = { FO_CIRCULO,  MN_RESINA,     0,         86,   0, RGB(246, 222, 132), 0,                  ACAB_TINTA,      REV_PONTOS, 0 },
    // os adesivos de verdade: o recorte, com a margem branca do papel, e o desenho de cada um
    [AD_ESTRELINHA] = { FO_ESTRELA,   MN_OURO,       0,         88,   0, RGB(228, 194, 106), RGB(150, 122, 62),  ACAB_RECORTE,    REV_TUDO,   0 },
    [AD_HOLOGRAFICO] = { FO_LOSANGO,  MN_HOLO,       0,         86,   0, RGB(184, 194, 208), 0,                  ACAB_RECORTE,    REV_TUDO,   0 },
    [AD_FIGURINHA]  = { FO_CARTA,     MN_BRILHO,     0,         78,   6, RGB(130, 204, 112), RGB(88, 152, 84),   ACAB_RECORTE,    REV_TUDO,   0 },
    [AD_BILHETE]    = { FO_INGRESSO,  MN_PAPEL,      0,         80,   6, RGB(238, 230, 212), RGB(190, 56, 60),   ACAB_RECORTE,    REV_TUDO,   0 },
    [AD_LENTICULAR] = { FO_RETANGULO, MN_BRILHO,     0,         76,   0, RGB(102, 74, 138),  RGB(190, 56, 60),   ACAB_RECORTE,    REV_TUDO,   0 },
    [AD_BIS]        = { FO_INGRESSO,  MN_VINIL,      0,         80,   0, RGB(102, 74, 138),  RGB(246, 222, 132), ACAB_RECORTE,    REV_TUDO,   0 },
    [AD_AVESSO]     = { FO_DESCOLA,   MN_PAPEL,      MN_VINIL,  78, -35, RGB(238, 230, 212), RGB(72, 150, 138),  ACAB_RECORTE,    REV_TUDO,   0 },
    [AD_ETIQUETA]   = { FO_PRECO,     MN_PAPEL,      0,         80,  -8, RGB(252, 248, 236), RGB(190, 56, 60),   ACAB_RECORTE,    REV_TUDO,   0 },
};

static bool ad_v2(int a) { return ad_versao && ad_estilo == ADE_NOVO && a > AD_NADA && a < AD_N && AD_NOVO2[a].tam; }
static const ad_novo_t *an_ficha(int a) { return ad_v2(a) ? &AD_NOVO2[a] : &AD_NOVO[a]; }
static int an_acab(int a) { return ad_v2(a) ? AD_NOVO2[a].acab : ACAB_RECORTE; }

// A ordem em que o pixel aparece na colagem (0..1): a forma diz (REV_FORMA)
// em an_ordem; os outros, pelo jeito de aparecer.
static float an_ordem;
static float an_ordem_de(int a, float u, float w)
{
    switch (an_ficha(a)->rev) {
    case REV_RADIAL: return fminf(1, sqrtf(u * u + w * w));
    case REV_VARRE: return fminf(1, fmaxf(0, (u + w + 1.4f) / 2.8f));
    case REV_ESQ: return fminf(1, fmaxf(0, (u + 1) * 0.5f));
    case REV_CIMA: return fminf(1, fmaxf(0, (w + 1) * 0.5f));
    case REV_PONTOS: return (hash2((int)floorf(u * 9) + 50, (int)floorf(w * 9) + 50) & 255) / 255.0f;
    case REV_FORMA: return an_ordem >= 0 ? an_ordem : 0;
    default: return 0;
    }
}

// ---- O caminho da caneta da Nota Dez ----------------------------------------
// Uma volta e um pouco em volta do numero (a ponta passa por fora do comeco),
// o 10 no canto de cima, a direita, e o visto embaixo, a esquerda. O mesmo
// caminho da a forma e leva a caneta na colagem.
#define AN_DEZ_VOLTA 7.2f                                // o angulo da volta (um pouco mais que 2 pi)
#define AN_DEZ_A0    (-2.25f)                            // onde a caneta encosta: em cima, a esquerda
static float an_dez_raio(float th) { return 0.58f + 0.06f * th / AN_DEZ_VOLTA + 0.02f * sinf(th * 1.5f + 0.4f); }
// O 10 e o visto, em tracos: x0, y0, x1, y1, e quando cada traco comeca e acaba.
static const float AN_DEZ_TRACOS[4][6] = {
    { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 },          // (o 10 vai em letra miuda, na textura)
    { -0.36f, 0.86f, 0.40f, 0.80f, 0.86f, 1.00f },       // o sublinhado, um risco so, subindo um nada
    { 0, 0, 0, 0, 0, 0 },
};
// A ponta da caneta na ordem t (0..1), no recorte (raio 1).
static void an_dez_ponta(float t, float *x, float *y)
{
    if (t < 0.60f) {
        float th = t / 0.60f * AN_DEZ_VOLTA, an = AN_DEZ_A0 + th, r = an_dez_raio(th);
        *x = cosf(an) * r / 0.92f; *y = sinf(an) * r + 0.07f;
        return;
    }
    for (int i = 2; i < 3; i++) {
        const float *k = AN_DEZ_TRACOS[i];
        if (t >= k[4] && t < k[5]) { float q = (t - k[4]) / (k[5] - k[4]); *x = k[0] + (k[2] - k[0]) * q; *y = k[1] + (k[3] - k[1]) * q; return; }
    }
    if (t >= 0.62f && t < 0.84f) {                       // o 10: escrevendo, da esquerda para a direita, subindo e descendo
        float q = (t - 0.62f) / 0.22f;
        *x = -0.27f + 0.54f * q; *y = -0.82f + 0.16f * sinf(q * 18.85f);
        return;
    }
    if (t < 0.86f) { *x = -0.36f; *y = 0.86f; return; }
    *x = 0.40f; *y = 0.80f;
}

// ---- As formas novas ------------------------------------------------------
// Como em an_forma: a distancia com sinal, no raio 1; *parte, a parte; e a
// ordem em que aparece, em an_ordem. Falso: a forma de sempre (an_forma).
static bool an2_forma(int a, float x, float y, int *parte, float *dist)
{
    const ad_novo_t *v = an_ficha(a);
    (void)parte;
    switch (v->forma) {
    case FO_PINCEL: {                              // o Corretivo: tres pinceladas de ida e volta e um pingo escorrendo
        float d = 9, ord = 1;
        for (int i = 0; i < 3; i++) {
            float yc = -0.50f + i * 0.50f + 0.05f * sinf(x * 3.1f + i * 1.7f);              // a pincelada ondula
            float lg = 0.22f + 0.03f * sinf(x * 6.3f + i * 2.3f) + 0.03f * ruido(x * 5 + i * 5, i * 3.0f);
            float x0 = -0.84f + 0.06f * i, x1 = 0.86f - 0.10f * (i == 1);
            bool ida = (i & 1) == 0;               // ida: da esquerda para a direita; a volta, ao contrario
            float di = sdn_caixa(x - (x0 + x1) * 0.5f, y - yc, (x1 - x0) * 0.5f, lg, lg * 0.85f);
            // onde o pincel levanta, a tinta acaba e os fios ficam: listras sem tinta
            float pf = ida ? (x - (x1 - 0.40f)) / 0.40f : ((x0 + 0.40f) - x) / 0.40f;
            if (pf > 0 && di < 0) {
                float fio = fr((y - yc) * 11.0f + 0.37f * i + 0.2f * ruido(x * 3, i * 7.0f));
                if (fio < 0.10f + 0.55f * pf * pf) di = fmaxf(di, 0.03f);
            }
            if (di < d) {
                d = di;
                float q = ida ? (x - x0) / (x1 - x0) : (x1 - x) / (x1 - x0);
                ord = (i + fminf(1, fmaxf(0, q))) / 3.0f * 0.90f;
            }
        }
        float dp = fminf(sdn_ponta(x, y, 0.28f, 0.62f, 0.31f, 0.90f, 0.085f, 0.055f),
                         sqrtf((x - 0.31f) * (x - 0.31f) + (y - 0.94f) * (y - 0.94f)) - 0.09f);
        if (dp < d) { d = dp; ord = 0.90f + 0.10f * fminf(1, fmaxf(0, (y - 0.62f) / 0.34f)); }
        an_ordem = ord;
        *dist = d;
        return true;
    }
    case FO_CARIMBO: {                             // o Carimbo: a moldura dupla e as duas fileiras de letras
        float fora = sdn_caixa(x, y, 0.94f, 0.74f, 0.12f);
        float d = fmaxf(fora, -(fora + 0.15f));                      // a moldura grossa
        d = fminf(d, fabsf(sdn_caixa(x, y, 0.71f, 0.51f, 0.05f)) - 0.04f);   // e o fio de dentro
        for (int r = -1; r <= 1; r += 2) {                           // as letras, deixando o meio para o numero
            float ly = y - r * 0.355f;
            if (fabsf(ly) < 0.06f && fabsf(x) < 0.57f) {
                float cx = (x + 0.57f) / 0.16f;
                int col = (int)floorf(cx);
                if (fr(cx) > 0.18f && fr(cx) < 0.82f && (hash2(col + 30, r + 40) % 6) != 0) d = fminf(d, -0.02f);
            }
        }
        *dist = d;
        return true;
    }
    case FO_CERA: {                                // o Selo: a poca de cera derramada e os pingos que escorreram
        float d = sdn_bolha(x + 0.05f, y + 0.06f, 0.70f, 0.11f, 2.3f, 7);
        d = fminf(d, sdn_ponta(x, y, 0.40f, 0.40f, 0.62f, 0.70f, 0.17f, 0.09f));
        d = fminf(d, sqrtf((x - 0.65f) * (x - 0.65f) + (y - 0.74f) * (y - 0.74f)) - 0.12f);
        d = fminf(d, sdn_ponta(x, y, -0.30f, 0.52f, -0.35f, 0.78f, 0.12f, 0.07f));
        d = fminf(d, sqrtf((x + 0.36f) * (x + 0.36f) + (y - 0.82f) * (y - 0.82f)) - 0.085f);
        an_ordem = fminf(1, sqrtf((x + 0.08f) * (x + 0.08f) + (y + 0.10f) * (y + 0.10f)) / 1.05f);   // derrama de perto do meio
        *dist = d;
        return true;
    }
    case FO_CANETA: {                              // a Nota Dez: a volta de caneta, o 10 e o visto
        const float lg = 0.062f;
        float ex = x * 0.92f, ey = y - 0.07f, ro = sqrtf(ex * ex + ey * ey), an = atan2f(ey, ex);   // a volta, um nada abaixo do meio
        float th = an - AN_DEZ_A0;
        while (th < 0) th += 6.2832f;
        float d = 9, ord = 1;
        for (int k = 0; k < 2; k++, th += 6.2832f) {                 // as duas passadas da volta
            if (th > AN_DEZ_VOLTA) break;
            float dk = fabsf(ro - an_dez_raio(th)) - lg;
            if (dk < d) { d = dk; ord = th / AN_DEZ_VOLTA * 0.60f; }
        }
        for (int i = 2; i < 3; i++) {                                // o sublinhado, embaixo
            const float *k = AN_DEZ_TRACOS[i];
            float dk = sdn_seg(x, y, k[0], k[1], k[2], k[3]) - lg;
            if (dk < d) {
                float ex2 = k[2] - k[0], ey2 = k[3] - k[1], q = ((x - k[0]) * ex2 + (y - k[1]) * ey2) / (ex2 * ex2 + ey2 * ey2);
                d = dk; ord = k[4] + (k[5] - k[4]) * fminf(1, fmaxf(0, q));
            }
        }
        // o 10, miudo, em cima: a caixa dele (a textura escolhe os pixels da letra)
        float dz = sdn_caixa(x, y + 0.82f, 0.27f, 0.18f, 0.02f);
        if (dz < d && dz < 0) { d = dz; *parte = 1; ord = 0.62f + 0.22f * fminf(1, fmaxf(0, (x + 0.27f) / 0.54f)); }
        an_ordem = ord;
        *dist = d;
        return true;
    }
    case FO_GERME: {                               // o Infectado: a gosma com bolhas e as veias correndo pela face
        float d = sdn_bolha(x, y, 0.50f, 0.16f, 2.6f, 11);
        static const float V[6][4] = { { 0.38f, -0.30f, 0.86f, -0.62f }, { -0.42f, -0.22f, -0.90f, -0.44f }, { 0.20f, 0.44f, 0.46f, 0.92f },
                                       { -0.30f, 0.36f, -0.70f, 0.82f }, { 0.48f, 0.12f, 0.96f, 0.22f }, { -0.04f, -0.48f, 0.10f, -0.96f } };
        float ord = fminf(1, sqrtf(x * x + y * y) / 0.55f) * 0.45f;
        for (int i = 0; i < 6; i++) {                                // a veia afina para a ponta, e se divide no meio
            float mx = (V[i][0] + V[i][2]) * 0.5f + (V[i][3] - V[i][1]) * 0.12f, my = (V[i][1] + V[i][3]) * 0.5f - (V[i][2] - V[i][0]) * 0.12f;
            float dv = fminf(sdn_ponta(x, y, V[i][0], V[i][1], mx, my, 0.075f, 0.05f), sdn_ponta(x, y, mx, my, V[i][2], V[i][3], 0.05f, 0.02f));
            if (dv < d) {
                d = dv; *parte = 1;
                float L = sqrtf((x - V[i][0]) * (x - V[i][0]) + (y - V[i][1]) * (y - V[i][1]));
                ord = 0.45f + 0.55f * fminf(1, L / 0.62f);
            }
        }
        an_ordem = ord;
        *dist = d;
        return true;
    }
    case FO_GARRAS: {                              // o Lobo: tres rasgos de garra, curvos, finos nas pontas
        float d = 9, ord = 1;
        for (int i = 0; i < 3; i++) {
            float ox = (i - 1) * 0.36f, oy = (i == 1) * -0.06f;
            float ax = ox + 0.44f, ay = -0.80f + oy, mx = ox + 0.06f, my = 0.0f + oy, bx = ox - 0.36f, by = 0.82f + oy;
            float d1 = sdn_ponta(x, y, ax, ay, mx, my, 0.04f, 0.16f), d2 = sdn_ponta(x, y, mx, my, bx, by, 0.16f, 0.03f);
            float di = fminf(d1, d2);
            if (di < d) {
                d = di;
                float t = d1 < d2 ? 0.5f * fminf(1, fmaxf(0, (y - ay) / (my - ay))) : 0.5f + 0.5f * fminf(1, fmaxf(0, (y - my) / (by - my)));
                ord = t * 0.82f + i * 0.09f;
            }
        }
        an_ordem = ord;
        *dist = d;
        return true;
    }
    case FO_BOMBINHA: {                            // o Papel Bomba: o canudo de papel e o pavio torto saindo da ponta
        float d = sdn_caixa(x, y, 0.74f, 0.32f, 0.16f);
        float p1 = sdn_ponta(x, y, 0.70f, -0.06f, 0.86f, -0.36f, 0.045f, 0.04f), p2 = sdn_ponta(x, y, 0.86f, -0.36f, 0.80f, -0.62f, 0.04f, 0.035f);
        float pv = fminf(p1, p2);
        if (pv < d) { d = pv; *parte = 2; }
        *dist = d;
        return true;
    }
    case FO_FOLHA: {                               // o Dourado: a folha de ouro, a beira rasgada e as lascas soltas em volta
        float d = sdn_caixa(x, y, 0.74f, 0.70f, 0.08f) + 0.07f * ruido(x * 7 + 3, y * 7 + 1);
        static const float L[5][3] = { { 0.88f, -0.52f, 0.07f }, { -0.86f, 0.62f, 0.06f }, { 0.70f, 0.86f, 0.05f }, { -0.62f, -0.88f, 0.05f }, { 0.94f, 0.30f, 0.04f } };
        for (int i = 0; i < 5; i++) d = fminf(d, sdn_caixa(x - L[i][0], y - L[i][1], L[i][2], L[i][2] * 0.8f, 0.01f));
        *dist = d;
        return true;
    }
    case FO_FITA2: {                               // o Fragil: a fita de embalagem atravessando a face, as pontas picotadas
        float d = sdn_caixa(x, y, 1.06f, 0.30f, 0.02f);
        float dente = fabsf(fr(y * 5.0f + 0.5f) - 0.5f) * 0.16f;
        *dist = fmaxf(d, fabsf(x) - 0.98f - dente);
        return true;
    }
    case FO_ESPUMA: {                              // a Dupla Face: a espuma branca e o filme vermelho, a ponta descascada
        float d = sdn_caixa(x, y, 0.80f, 0.80f, 0.08f);
        if (x + y < 0.86f) *parte = 1;             // o filme cobre tudo menos a quina de baixo, a direita
        an_vinco = x + y - 0.86f;
        *dist = d;
        return true;
    }
    case FO_CONFETE: {                             // o papel picado: doze pedacinhos soltos, cada um do seu jeito
        float d = 9;
        int pi = 0;
        for (int i = 0; i < 12; i++) {
            uint32_t h = hash2(i * 7 + 3, 91);
            float an = i * 2.39996f + 0.4f, rr = 0.22f + 0.70f * sqrtf((i + 0.5f) / 12);
            float px = cosf(an) * rr, py = sinf(an) * rr, gi = (h % 628) / 100.0f, c = cosf(gi), sn = sinf(gi);
            float lx = (x - px) * c + (y - py) * sn, ly = -(x - px) * sn + (y - py) * c, di;
            switch ((h >> 10) & 3) {
            case 0: di = sdn_caixa(lx, ly, 0.14f, 0.065f, 0.01f); break;                 // a tirinha
            case 1: di = sqrtf(lx * lx + ly * ly) - 0.085f; break;                       // a bolinha
            case 2: di = sdn_caixa(lx, ly, 0.085f, 0.085f, 0.01f); break;                // o quadradinho
            default: di = sdn_tri(lx, ly, -0.11f, 0.08f, 0.11f, 0.08f, 0, -0.11f); break;   // o triangulo
            }
            if (di < d) { d = di; pi = i; }
        }
        *parte = pi;
        an_ordem = (hash2(pi + 17, 5) % 100) / 100.0f;            // caem um de cada vez, fora de ordem
        *dist = d;
        return true;
    }
    case FO_PURPURINA: {                           // a mancha de cola coberta de purpurina e uns graos soltos em volta
        float d = sdn_bolha(x, y, 0.56f, 0.18f, 2.0f, 5);
        for (int i = 0; i < 14; i++) {
            float an = i * 2.39996f + 0.5f, rr = 0.66f + 0.28f * fr(i * 0.618f);
            float di = sdn_caixa(x - cosf(an) * rr, y - sinf(an) * rr, 0.045f, 0.045f, 0);
            if (di < d) { d = di; *parte = 1; }
        }
        *dist = d;
        return true;
    }
    case FO_PRECO: {                               // a Etiqueta de preco: o papel com a beira de cima e a de baixo serrilhadas
        float dente = fabsf(fr(x * 4.2f + 0.5f) - 0.5f) * 0.14f;
        *dist = fmaxf(fabsf(x) - 0.92f, fabsf(y) - 0.52f - dente);
        return true;
    }
    default: return false;
    }
}

// A textura da proposta (adesivo_v2_anima.inc). Pode deixar a cor pronta em
// an_final (sem a luz do material: a tinta rala sobre o dado) ou pular o
// pixel (an_pula: a tinta falhou, a prata ja foi raspada).
static uint16_t an_final;
static bool an_pula;
static float an_dd_gx, an_dd_gy;         // a normal da beira no pixel (para fora, na tela), para a textura
static uint16_t an2_textura(int a, int parte, float x, float y, float e, float s, int X, int Y, float fase, float mov, uint16_t cor);

// A ordem em que o pixel sai (0..1), raspando ou rompendo: a Raspadinha sai
// em zigue-zague, de cima para baixo, como a moeda passa.
static float an2_sai(int a, float u, float w)
{
    if (a == AD_RASPADINHA) {                      // cinco passadas de ida e volta
        float yy = fminf(0.999f, fmaxf(0, (w + 0.80f) / 1.60f)) * 5;
        int k = (int)yy;
        float xx = fminf(1, fmaxf(0, (u + 1.0f) / 2.0f));
        return (k + (k & 1 ? 1 - xx : xx)) / 5;
    }
    return 2;
}
