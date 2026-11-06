// ===========================================================================
// O adesivo novo (ADE_NOVO): cada adesivo e um recorte com a forma dele (a
// cabeca do Lobo, o peso de Chumbo, a goma esticada do Chiclete, a explosao
// de gibi do Papel Bomba, o Patinho boiando, o ima em ferradura, a fita do
// Corretivo, o Avesso descolando...), com a margem branca do papel e a sombra
// no dado, e por dentro a textura e o material dele: fosco, plastico
// brilhante, metal, chumbo martelado, ouro, cromo, holografico, purpurina,
// papel, tecido, vidro, neon, cera, borracha, resina...
// O dado aparece em volta e o numero vem por cima, escuro, com o contorno
// branco (desenha_dado_base), lendo em qualquer um.
// ===========================================================================

// ---- As formas, em distancia com sinal (negativa dentro), no raio 1 ---------
static float sdn_caixa(float x, float y, float bx, float by, float rr)
{
    float qx = fabsf(x) - bx + rr, qy = fabsf(y) - by + rr;
    float ox = fmaxf(qx, 0), oy = fmaxf(qy, 0);
    return sqrtf(ox * ox + oy * oy) + fminf(fmaxf(qx, qy), 0) - rr;
}
static float sdn_seg(float x, float y, float ax, float ay, float bx, float by)
{
    float ex = bx - ax, ey = by - ay, q = ((x - ax) * ex + (y - ay) * ey) / (ex * ex + ey * ey);
    q = q < 0 ? 0 : q > 1 ? 1 : q;
    float dx = x - ax - q * ex, dy = y - ay - q * ey;
    return sqrtf(dx * dx + dy * dy);
}
// O segmento que afina: raio ra na ponta a, rb na ponta b.
static float sdn_ponta(float x, float y, float ax, float ay, float bx, float by, float ra, float rb)
{
    float ex = bx - ax, ey = by - ay, q = ((x - ax) * ex + (y - ay) * ey) / (ex * ex + ey * ey);
    q = q < 0 ? 0 : q > 1 ? 1 : q;
    float dx = x - ax - q * ex, dy = y - ay - q * ey;
    return sqrtf(dx * dx + dy * dy) - (ra + (rb - ra) * q);
}
static float sdn_elipse(float x, float y, float a, float b)      // aproximada: basta para o recorte
{
    return (sqrtf(x * x / (a * a) + y * y / (b * b)) - 1) * fminf(a, b);
}
static float sdn_hex(float x, float y, float r)               // de ponta para cima, r: o apotema
{
    const float kx = -0.866025404f, ky = 0.5f, kz = 0.577350269f;
    float px = fabsf(y), py = fabsf(x);                       // deitado: as pontas dos lados
    float d = 2 * fminf(kx * px + ky * py, 0);
    px -= d * kx; py -= d * ky;
    px -= fminf(fmaxf(px, -kz * r), kz * r); py -= r;
    return sqrtf(px * px + py * py) * (py > 0 ? 1 : -1);
}
static float sdn_estrela(float x, float y, float r, float rf)   // cinco pontas, uma para cima
{
    const float k1x = 0.809016994f, k1y = -0.587785252f;
    float px = fabsf(x), py = -y;
    float d = 2 * fmaxf(k1x * px + k1y * py, 0); px -= d * k1x; py -= d * k1y;
    d = 2 * fmaxf(-k1x * px + k1y * py, 0); px -= d * -k1x; py -= d * k1y;
    px = fabsf(px); py -= r;
    float bax = rf * -k1y, bay = rf * k1x - 1;
    float h = (px * bax + py * bay) / (bax * bax + bay * bay);
    h = h < 0 ? 0 : h > r ? r : h;
    float ex = px - bax * h, ey = py - bay * h;
    return sqrtf(ex * ex + ey * ey) * ((py * bax - px * bay) > 0 ? 1 : -1);
}
static float sdn_coracao(float x, float y)
{
    float px = fabsf(x) * 0.62f, py = (0.62f - y) * 0.62f;  // a ponta embaixo
    if (py + px > 1) { float dx = px - 0.25f, dy = py - 0.75f; return (sqrtf(dx * dx + dy * dy) - 0.353553f) / 0.62f; }
    float ax = px, ay = py - 1, m = 0.5f * fmaxf(px + py, 0), bx = px - m, by = py - m;
    float d = sqrtf(fminf(ax * ax + ay * ay, bx * bx + by * by));
    return d * (px - py > 0 ? 1 : -1) / 0.62f;
}
static float sdn_losango(float x, float y, float bx, float by)
{
    float px = fabsf(x), py = fabsf(y);
    float h = ((bx - 2 * px) * bx - (by - 2 * py) * by) / (bx * bx + by * by);
    h = h < -1 ? -1 : h > 1 ? 1 : h;
    float dx = px - 0.5f * bx * (1 - h), dy = py - 0.5f * by * (1 + h);
    return sqrtf(dx * dx + dy * dy) * ((px * by + py * bx - bx * by) > 0 ? 1 : -1);
}
static float sdn_tri(float px, float py, float ax, float ay, float bx, float by, float cx, float cy)
{
    float e0x = bx - ax, e0y = by - ay, e1x = cx - bx, e1y = cy - by, e2x = ax - cx, e2y = ay - cy;
    float v0x = px - ax, v0y = py - ay, v1x = px - bx, v1y = py - by, v2x = px - cx, v2y = py - cy;
    float t0 = (v0x * e0x + v0y * e0y) / (e0x * e0x + e0y * e0y), t1 = (v1x * e1x + v1y * e1y) / (e1x * e1x + e1y * e1y);
    float t2 = (v2x * e2x + v2y * e2y) / (e2x * e2x + e2y * e2y);
    t0 = t0 < 0 ? 0 : t0 > 1 ? 1 : t0; t1 = t1 < 0 ? 0 : t1 > 1 ? 1 : t1; t2 = t2 < 0 ? 0 : t2 > 1 ? 1 : t2;
    float q0x = v0x - e0x * t0, q0y = v0y - e0y * t0, q1x = v1x - e1x * t1, q1y = v1y - e1y * t1;
    float q2x = v2x - e2x * t2, q2y = v2y - e2y * t2;
    float s = e0x * e2y - e0y * e2x > 0 ? 1 : -1;
    float d = fminf(q0x * q0x + q0y * q0y, fminf(q1x * q1x + q1y * q1y, q2x * q2x + q2y * q2y));
    float g = fminf(s * (v0x * e0y - v0y * e0x), fminf(s * (v1x * e1y - v1y * e1x), s * (v2x * e2y - v2y * e2x)));
    return -sqrtf(d) * (g > 0 ? 1 : -1);
}
// O trapezio: meia largura rb embaixo, rt em cima, meia altura he (y para baixo).
static float sdn_trap(float px, float py, float rb, float rt, float he)
{
    py = -py;
    float k1x = rt, k1y = he, k2x = rt - rb, k2y = 2 * he;
    px = fabsf(px);
    float cax = px - fminf(px, py < 0 ? rb : rt), cay = fabsf(py) - he;
    float t = ((k1x - px) * k2x + (k1y - py) * k2y) / (k2x * k2x + k2y * k2y);
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    float cbx = px - k1x + k2x * t, cby = py - k1y + k2y * t;
    float s = (cbx < 0 && cay < 0) ? -1 : 1;
    return s * sqrtf(fminf(cax * cax + cay * cay, cbx * cbx + cby * cby));
}
// O poligono qualquer (v: x0, y0, x1, y1...).
static float sdn_poli(float px, float py, const float *v, int n)
{
    float d = (px - v[0]) * (px - v[0]) + (py - v[1]) * (py - v[1]), s = 1;
    for (int i = 0, j = n - 1; i < n; j = i, i++) {
        float ex = v[2 * j] - v[2 * i], ey = v[2 * j + 1] - v[2 * i + 1];
        float wx = px - v[2 * i], wy = py - v[2 * i + 1];
        float t = (wx * ex + wy * ey) / (ex * ex + ey * ey);
        t = t < 0 ? 0 : t > 1 ? 1 : t;
        float bx = wx - ex * t, by = wy - ey * t;
        d = fminf(d, bx * bx + by * by);
        bool c1 = py >= v[2 * i + 1], c2 = py < v[2 * j + 1], c3 = ex * wy > ey * wx;
        if ((c1 && c2 && c3) || (!c1 && !c2 && !c3)) s = -s;
    }
    return s * sqrtf(d);
}
// O circulo de beira torta (cera, goma, barro): o ruido no angulo, de semente s.
static float sdn_bolha(float x, float y, float r, float amp, float freq, int s)
{
    float an = atan2f(y, x), ro = sqrtf(x * x + y * y);
    float w = ruido(cosf(an) * freq + s * 3.1f, sinf(an) * freq + s * 1.7f);
    return ro - r * (1 + amp * w);
}

// A explosao de gibi (onze pontas tortas) e o brilho de quatro pontas.
static float an_exp[44], an_bri[64];
static bool an_poli_ok;
static void an_poli_monta(void)
{
    static const float RO[11] = { 0.98f, 0.80f, 0.95f, 0.76f, 1.0f, 0.84f, 0.93f, 0.78f, 0.99f, 0.82f, 0.90f };
    static const float RI[11] = { 0.56f, 0.62f, 0.54f, 0.60f, 0.57f, 0.63f, 0.55f, 0.59f, 0.58f, 0.56f, 0.61f };
    for (int i = 0; i < 22; i++) {
        float an = -1.5708f + i * 0.285599f + ((i * 7) % 5 - 2) * 0.035f, r = (i & 1) ? RI[i / 2] : RO[i / 2];
        an_exp[2 * i] = cosf(an) * r; an_exp[2 * i + 1] = sinf(an) * r;
    }
    for (int i = 0; i < 32; i++) {                  // os lados curvos para dentro, as pontas finas
        float t = i * 0.19635f, c = cosf(t), s = sinf(t);
        an_bri[2 * i] = (c < 0 ? -1 : 1) * powf(fabsf(c), 3.0f);
        an_bri[2 * i + 1] = (s < 0 ? -1 : 1) * powf(fabsf(s), 3.0f);
    }
    an_poli_ok = true;
}

// ---- A ficha de cada um: a forma, o material, o tamanho e o giro -----------
enum { FO_CIRCULO, FO_QUADRADO, FO_RETANGULO, FO_HEXAGONO, FO_LOSANGO, FO_ESTRELA, FO_CORACAO, FO_NUVEM,
       FO_PUFE, FO_LACRE, FO_RESPINGO, FO_ESCORRIDO, FO_CHIP, FO_FERRADURA, FO_DESCOLA, FO_DUPLO, FO_FOLHAS,
       FO_CANTO, FO_ETIQUETA, FO_INGRESSO, FO_LOBO, FO_EXPLOSAO, FO_CASCA, FO_PATO, FO_FITA, FO_RACHADO,
       FO_FELPUDO, FO_MIRA, FO_TABUA, FO_CARTA, FO_ROSETA, FO_VIRUS, FO_GOMA, FO_MASSA, FO_METADES,
       FO_PESO, FO_BRILHO4 };
// MN_IGUAL: (so na segunda parte) o mesmo material da primeira.
enum { MN_IGUAL, MN_FOSCO, MN_VINIL, MN_BRILHO, MN_METAL, MN_OURO, MN_CROMO, MN_HOLO, MN_GLITTER, MN_PAPEL,
       MN_TECIDO, MN_VIDRO, MN_NEON, MN_CERA, MN_BORRACHA, MN_MADEIRA, MN_LAMA, MN_CETIM, MN_LENTE, MN_CHUMBO,
       MN_PELO, MN_RESINA, MN_PURPURINA };
// forma, material, o material da segunda parte, o tamanho (% do raio do dado), o giro (graus),
// a cor e a segunda cor (as pontas do ima, o queijo, o verso, a agua do Patinho...).
typedef struct { uint8_t forma, material, mat2, tam; int8_t giro; uint16_t cor, cor2; } ad_novo_t;
static const ad_novo_t AD_NOVO[AD_N] = {
    [AD_CHUMBO]       = { FO_PESO,      MN_CHUMBO,    MN_METAL,  80,   0, RGB(70, 76, 88),    RGB(104, 112, 124) },
    [AD_REFLEXIVO]    = { FO_CIRCULO,   MN_CROMO,     0,         70,   0, RGB(184, 194, 208), 0 },
    [AD_CONFETE]      = { FO_NUVEM,     MN_PAPEL,     0,         76,   8, RGB(252, 248, 236), 0 },
    [AD_ELETRONICO]   = { FO_CHIP,      MN_FOSCO,     MN_METAL,  62,   0, RGB(72, 150, 138),  RGB(184, 194, 208) },
    [AD_TRANSPARENTE] = { FO_QUADRADO,  MN_VIDRO,     0,         72,  -6, RGB(168, 204, 230), 0 },
    [AD_DOURADO]      = { FO_CIRCULO,   MN_OURO,      0,         68,   0, RGB(228, 194, 106), RGB(150, 122, 62) },
    [AD_LOBO]         = { FO_LOBO,      MN_PELO,      0,         80,   0, RGB(190, 56, 60),   RGB(66, 18, 24) },
    [AD_REMENDO]      = { FO_QUADRADO,  MN_TECIDO,    0,         70,  10, RGB(88, 152, 84),   RGB(130, 204, 112) },
    [AD_RASPADINHA]   = { FO_RETANGULO, MN_PAPEL,     0,         76,  -5, RGB(206, 180, 218), RGB(184, 194, 208) },
    [AD_FERMENTO]     = { FO_PUFE,      MN_FOSCO,     0,         74,   0, RGB(246, 222, 132), RGB(238, 162, 74) },
    [AD_BIS]          = { FO_DUPLO,     MN_VINIL,     0,         64,   0, RGB(102, 74, 138),  RGB(150, 112, 160) },
    [AD_MIRA]         = { FO_MIRA,      MN_LENTE,     0,         62,   0, RGB(66, 18, 24),    RGB(208, 98, 108) },
    [AD_AVESSO]       = { FO_DESCOLA,   MN_VINIL,     MN_PAPEL,  76, -35, RGB(72, 150, 138),  RGB(238, 230, 212) },
    [AD_GORJETA]      = { FO_CIRCULO,   MN_BRILHO,    0,         66,   0, RGB(56, 86, 150),   RGB(238, 230, 212) },
    [AD_SELO]         = { FO_LACRE,     MN_CERA,      0,         66,   0, RGB(118, 30, 38),   RGB(66, 18, 24) },
    [AD_FRAGIL]       = { FO_RACHADO,   MN_PAPEL,     0,         76,  -4, RGB(214, 112, 44),  0 },
    [AD_ESTRELINHA]   = { FO_ESTRELA,   MN_GLITTER,   0,         84,   0, RGB(96, 134, 196),  0 },
    [AD_CORRETIVO]    = { FO_FITA,      MN_FOSCO,     0,         96, -24, RGB(252, 248, 236), 0 },
    [AD_IMANTADO]     = { FO_FERRADURA, MN_BRILHO,    MN_METAL,  78,   0, RGB(190, 56, 60),   RGB(184, 194, 208) },
    [AD_POSTIT]       = { FO_CANTO,     MN_PAPEL,     0,         72,  -5, RGB(246, 222, 132), RGB(228, 194, 106) },
    [AD_HOLOGRAFICO]  = { FO_LOSANGO,   MN_HOLO,      0,         84,   0, RGB(184, 194, 208), 0 },
    [AD_PURPURINA]    = { FO_BRILHO4,   MN_PURPURINA, 0,         96,   0, RGB(150, 62, 80),   0 },
    [AD_VELCRO]       = { FO_FELPUDO,   MN_TECIDO,    0,         72,   0, RGB(36, 32, 29),    0 },
    [AD_RATOEIRA]     = { FO_TABUA,     MN_MADEIRA,   MN_FOSCO,  74,   0, RGB(120, 78, 50),   RGB(246, 222, 132) },
    [AD_FIGURINHA]    = { FO_CARTA,     MN_BRILHO,    0,         76,   6, RGB(130, 204, 112), 0 },
    [AD_PATINHO]      = { FO_PATO,      MN_FOSCO,     MN_BRILHO, 86,   0, RGB(128, 120, 112), RGB(168, 204, 230) },
    [AD_ETIQUETA]     = { FO_ETIQUETA,  MN_PAPEL,     0,         74, -12, RGB(190, 156, 86),  0 },
    [AD_BILHETE]      = { FO_INGRESSO,  MN_BRILHO,    0,         78,   6, RGB(54, 36, 74),    0 },
    [AD_NOTA_DEZ]     = { FO_ROSETA,    MN_CETIM,     0,         64,   0, RGB(208, 98, 108),  RGB(56, 86, 150) },
    [AD_CARBONO]      = { FO_FOLHAS,    MN_VINIL,     MN_PAPEL,  68,  -4, RGB(26, 34, 62),    RGB(252, 248, 236) },
    [AD_BOMBA]        = { FO_EXPLOSAO,  MN_PAPEL,     0,         92,   8, RGB(190, 56, 60),   RGB(246, 222, 132) },
    [AD_IMPERMEAVEL]  = { FO_CIRCULO,   MN_RESINA,    0,         70,   0, RGB(246, 222, 132), 0 },
    [AD_LENTICULAR]   = { FO_RETANGULO, MN_BRILHO,    0,         74,   0, RGB(150, 112, 160), RGB(96, 182, 198) },
    [AD_NEON]         = { FO_CORACAO,   MN_NEON,      0,         76,   0, RGB(36, 32, 29),    0 },
    [AD_INFECTADO]    = { FO_VIRUS,     MN_BRILHO,    0,         64,   0, RGB(130, 204, 112), 0 },
    [AD_BARRO]        = { FO_RESPINGO,  MN_LAMA,      0,         72,   0, RGB(156, 106, 64),  0 },
    [AD_SUJEIRA]      = { FO_ESCORRIDO, MN_LAMA,      0,         72,   0, RGB(120, 78, 50),   0 },
    [AD_CHICLETE]     = { FO_GOMA,      MN_BORRACHA,  0,         74,   0, RGB(226, 164, 168), 0 },
    [AD_MASSINHA]     = { FO_MASSA,     MN_FOSCO,     0,         74,  12, RGB(206, 180, 218), 0 },
    [AD_CARIMBO]      = { FO_RETANGULO, MN_PAPEL,     0,         76,  -8, RGB(238, 230, 212), RGB(190, 56, 60) },
    [AD_DUPLA]        = { FO_METADES,   MN_VINIL,     0,         70, -10, RGB(36, 32, 29),    RGB(238, 230, 212) },
    [AD_CASCA]        = { FO_CASCA,     MN_BRILHO,    MN_FOSCO,  90,   0, RGB(246, 222, 132), RGB(120, 78, 50) },
};

// A forma de cima (a aba dobrada, a folha da frente, a agua sobre o pato...):
// an_forma deixa aqui a distancia ate a beira dela; perto dela, por fora, a
// sombra que ela faz na de baixo.
static float an_vinco;
static float an_vinco_px;                // a mesma, no pixel que se pinta (em pixels)
static int an_mat_px = -1;               // != -1: o material so deste pixel (a folha de ouro, o arame...)

// As tres abas da casca: a raiz, a dobra e a ponta.
static const float AN_ABA[3][6] = { { -0.14f, 0.00f, -0.56f, 0.16f, -0.88f, 0.62f },
                                    {  0.00f, 0.08f,  0.02f, 0.50f,  0.12f, 0.92f },
                                    {  0.14f, 0.00f,  0.54f, 0.12f,  0.88f, 0.54f } };

// A forma do adesivo a no ponto (x, y) do recorte (raio 1): a distancia com
// sinal, e em *parte, a parte (0 o corpo; 1 a segunda cor: as pontas do ima,
// o queijo, o verso, a agua...; 2 o miolo claro da casca).
static float an_forma(int a, float x, float y, int *parte)
{
    const ad_novo_t *v = &AD_NOVO[a];
    *parte = 0;
    an_vinco = 9;
    switch (v->forma) {
    case FO_CIRCULO: return sqrtf(x * x + y * y) - 0.92f;
    case FO_QUADRADO: return sdn_caixa(x, y, 0.82f, 0.82f, 0.22f);
    case FO_RETANGULO: return sdn_caixa(x, y, 0.95f, 0.68f, 0.14f);
    case FO_HEXAGONO: return sdn_hex(x, y, 0.84f);
    case FO_LOSANGO: return sdn_losango(x, y, 0.78f, 1.0f);
    case FO_ESTRELA: return sdn_estrela(x, y + 0.06f, 1.0f, 0.52f);
    case FO_CORACAO: return sdn_coracao(x, y + 0.04f) - 0.04f;
    case FO_NUVEM: {                               // a beira em ondas, como a de um papel de festa
        float an = atan2f(y, x), ro = sqrtf(x * x + y * y);
        return ro - (0.80f + 0.11f * fabsf(sinf(an * 4.5f)));
    }
    case FO_PUFE: {                                // a massa crescendo: tres bolotas em cima, a base reta
        float d = sdn_caixa(x, y - 0.32f, 0.86f, 0.42f, 0.2f);
        d = fminf(d, sqrtf((x + 0.48f) * (x + 0.48f) + (y + 0.05f) * (y + 0.05f)) - 0.46f);
        d = fminf(d, sqrtf((x - 0.48f) * (x - 0.48f) + (y + 0.05f) * (y + 0.05f)) - 0.46f);
        d = fminf(d, sqrtf(x * x + (y + 0.34f) * (y + 0.34f)) - 0.56f);
        return d;
    }
    case FO_LACRE: {                               // o lacre: a cera que escorreu torta e dois pingos
        float d = sdn_bolha(x, y, 0.84f, 0.13f, 2.0f, a);
        d = fminf(d, sqrtf((x - 0.80f) * (x - 0.80f) + (y - 0.52f) * (y - 0.52f)) - 0.11f);
        d = fminf(d, sqrtf((x + 0.70f) * (x + 0.70f) + (y - 0.66f) * (y - 0.66f)) - 0.08f);
        return d;
    }
    case FO_MASSA: return sdn_bolha(x * 0.86f, y * 1.12f, 0.80f, 0.12f, 1.3f, a);
    case FO_RESPINGO: {                            // a mancha e os pingos que voaram
        float d = sdn_bolha(x, y, 0.72f, 0.22f, 2.4f, a);
        static const float P[4][3] = { { 0.82f, -0.58f, 0.13f }, { -0.88f, 0.35f, 0.10f }, { 0.55f, 0.84f, 0.09f }, { -0.5f, -0.86f, 0.08f } };
        for (int i = 0; i < 4; i++) d = fminf(d, sqrtf((x - P[i][0]) * (x - P[i][0]) + (y - P[i][1]) * (y - P[i][1])) - P[i][2]);
        return d;
    }
    case FO_ESCORRIDO: {                           // a lama escorrendo: a mancha e tres pingos descendo
        float d = sdn_bolha(x, y + 0.12f, 0.70f, 0.16f, 2.0f, a);
        static const float P[3][3] = { { -0.42f, 0.86f, 0.11f }, { 0.08f, 0.98f, 0.12f }, { 0.50f, 0.74f, 0.09f } };
        for (int i = 0; i < 3; i++) d = fminf(d, sdn_seg(x, y, P[i][0], 0.2f, P[i][0], P[i][1]) - P[i][2]);
        return d;
    }
    case FO_CHIP: {                                // a placa e as perninhas de metal dos dois lados
        float d = sdn_caixa(x, y, 0.80f, 0.86f, 0.10f);
        for (int i = 0; i < 4; i++) {
            float py = -0.6f + i * 0.4f, dp = sdn_caixa(fabsf(x) - 0.98f, y - py, 0.2f, 0.09f, 0.02f);
            if (dp < d) { d = dp; *parte = 1; }
        }
        return d;
    }
    case FO_FERRADURA: {                           // o ima: o U grosso, as pontas de metal em cima
        float ro = sqrtf(x * x + (y - 0.05f) * (y - 0.05f));
        float anel = fabsf(ro - 0.62f) - 0.30f;
        if (y < 0.05f) anel = sdn_caixa(fabsf(x) - 0.62f, y + 0.36f, 0.30f, 0.42f, 0.04f);
        if (y < -0.46f) *parte = 1;
        return anel;
    }
    case FO_DESCOLA: {                             // o Avesso: o redondo descolando, a aba dobrada mostra o verso
        const float c = 0.46f;                     // a dobra: a reta x = c
        float corpo = fmaxf(sqrtf(x * x + y * y) - 0.88f, x - c), mx = 2 * c - x;
        float aba = fmaxf(sqrtf(mx * mx + y * y) - 0.88f, x - c);
        an_vinco = aba;
        if (aba < 0) *parte = 1;
        return corpo;
    }
    case FO_DUPLO: {                               // o Bis: dois quadrados, o de tras deslocado
        float f = sdn_caixa(x + 0.16f, y + 0.16f, 0.66f, 0.66f, 0.16f), t = sdn_caixa(x - 0.22f, y - 0.22f, 0.66f, 0.66f, 0.16f);
        an_vinco = f;
        if (f > 0) *parte = 1;
        return fminf(f, t);
    }
    case FO_FOLHAS: {                              // o Papel Carbono: a folha de copia e o papel branco por baixo
        float f = sdn_caixa(x + 0.10f, y + 0.10f, 0.76f, 0.80f, 0.06f), t = sdn_caixa(x - 0.18f, y - 0.16f, 0.76f, 0.80f, 0.06f);
        an_vinco = f;
        if (f > 0) *parte = 1;
        return fminf(f, t);
    }
    case FO_CANTO: {                               // o Post-it: a quina de baixo, a direita, dobrada para cima
        const float k = 0.98f;                     // a dobra: a reta x + y = k
        float corpo = fmaxf(sdn_caixa(x, y, 0.84f, 0.84f, 0.04f), x + y - k);
        float aba = fmaxf(sdn_caixa(k - y, k - x, 0.84f, 0.84f, 0.04f), x + y - k);
        an_vinco = aba;
        if (aba < 0) *parte = 1;
        return corpo;
    }
    case FO_ETIQUETA: {                            // a etiqueta de bagagem: a ponta, o furo e o barbante (extra)
        float d = sdn_caixa(x + 0.24f, y, 0.76f, 0.60f, 0.08f);
        float ponta = fmaxf(fabsf(y) + (x - 0.46f) * 1.05f - 0.60f, -(x - 0.40f));
        d = fminf(d, fmaxf(ponta, x - 1.02f));
        return fmaxf(d, -(sqrtf((x - 0.70f) * (x - 0.70f) + y * y) - 0.10f));
    }
    case FO_INGRESSO: {                            // o bilhete: os dois recortes redondos dos lados
        float d = sdn_caixa(x, y, 0.98f, 0.62f, 0.06f);
        d = fmaxf(d, -(sqrtf((x + 0.98f) * (x + 0.98f) + y * y) - 0.20f));
        return fmaxf(d, -(sqrtf((x - 0.98f) * (x - 0.98f) + y * y) - 0.20f));
    }
    case FO_LOBO: {                                // a cabeca do lobo: as orelhas em pe, os tufos da bochecha, o focinho
        float ex = fabsf(x);
        float d = sqrtf(x * x + (y + 0.04f) * (y + 0.04f)) - 0.58f;
        d = fminf(d, sdn_trap(x, y - 0.44f, 0.18f, 0.44f, 0.36f) - 0.07f);
        d = fminf(d, sdn_tri(ex, y, 0.12f, -0.38f, 0.70f, -0.98f, 0.62f, -0.16f) - 0.03f);
        d = fminf(d, sdn_tri(ex, y, 0.44f, -0.14f, 0.88f, 0.14f, 0.48f, 0.22f));
        d = fminf(d, sdn_tri(ex, y, 0.38f, 0.16f, 0.74f, 0.44f, 0.30f, 0.42f));
        if (sdn_tri(ex, y, 0.26f, -0.40f, 0.63f, -0.80f, 0.56f, -0.28f) < 0) *parte = 1;   // o dentro da orelha
        return d;
    }
    case FO_EXPLOSAO: {                            // o Papel Bomba: a explosao de gibi, o miolo amarelo
        if (!an_poli_ok) an_poli_monta();
        float d = sdn_poli(x, y, an_exp, 22);
        float qx = (x * 0.990f - y * 0.141f) / 0.60f, qy = (x * 0.141f + y * 0.990f) / 0.60f;
        float miolo = sdn_poli(qx, qy, an_exp, 22) * 0.60f;
        an_vinco = miolo;
        if (miolo < 0) *parte = 1;
        return d;
    }
    case FO_BRILHO4: {                             // a Purpurina: o brilho de quatro pontas e um menor ao lado
        if (!an_poli_ok) an_poli_monta();
        float d = sdn_poli((x + 0.10f) / 0.80f, (y - 0.06f) / 0.98f, an_bri, 32) * 0.80f;
        d = fminf(d, sdn_poli((x - 0.62f) / 0.36f, (y + 0.58f) / 0.40f, an_bri, 32) * 0.36f);
        return fminf(d, sdn_poli((x + 0.66f) / 0.22f, (y - 0.70f) / 0.24f, an_bri, 32) * 0.22f);
    }
    case FO_CASCA: {                               // a casca de banana: o cabinho, o pescoco e tres abas caidas
        float d = sdn_trap(x, y + 0.36f, 0.24f, 0.10f, 0.26f) - 0.04f;
        d = fminf(d, sdn_elipse(x, y + 0.04f, 0.32f, 0.22f));
        for (int i = 0; i < 3; i++) {
            const float *A = AN_ABA[i];
            float f = fminf(sdn_ponta(x, y, A[0], A[1], A[2], A[3], 0.21f, 0.15f), sdn_ponta(x, y, A[2], A[3], A[4], A[5], 0.15f, 0.06f));
            if (f < -0.075f && y > 0.10f) *parte = 2;          // o lado de dentro da aba, claro
            d = fminf(d, f);
        }
        float cab = sdn_caixa(x, y + 0.70f, 0.08f, 0.13f, 0.05f);
        if (cab < 0 && cab < d) *parte = 1;
        return fminf(d, cab);
    }
    case FO_PATO: {                                // o Patinho: o pato de lado, boiando na agua
        float d = sdn_elipse(x - 0.10f, y - 0.20f, 0.70f, 0.40f);
        d = fminf(d, sdn_tri(x, y, 0.48f, 0.04f, 0.98f, -0.34f, 0.76f, 0.30f) - 0.02f);   // o rabo
        d = fminf(d, sqrtf((x + 0.36f) * (x + 0.36f) + (y + 0.40f) * (y + 0.40f)) - 0.30f);   // a cabeca
        d = fminf(d, sdn_seg(x, y, -0.34f, -0.30f, -0.20f, 0.06f) - 0.17f);                   // o pescoco
        d = fminf(d, sdn_tri(x, y, -0.58f, -0.48f, -0.98f, -0.34f, -0.60f, -0.24f) - 0.02f);   // o bico
        float agua = fmaxf(sdn_caixa(x, y - 0.70f, 0.94f, 0.20f, 0.10f), 0.54f + 0.06f * sinf(x * 9.5f) - y);
        an_vinco = agua;
        if (agua < 0) *parte = 1;
        return fminf(d, agua);
    }
    case FO_FITA: {                                // o Corretivo: a fita branca na diagonal, os dentes do corte
        float d = sdn_caixa(x, y, 1.0f, 0.34f, 0.04f);
        float dente = fabsf(fr(y * 4.0f + 0.5f) - 0.5f) * 0.18f;
        return fmaxf(d, fabsf(x) - 0.90f - dente);
    }
    case FO_RACHADO: {                             // o Fragil: a etiqueta partida por uma rachadura
        float d = sdn_caixa(x, y, 0.92f, 0.70f, 0.08f);
        static const float P[5][2] = { { 0.18f, -0.80f }, { 0.02f, -0.30f }, { 0.20f, 0.02f }, { -0.06f, 0.36f }, { 0.10f, 0.82f } };
        float g = 9;
        for (int i = 0; i < 4; i++) g = fminf(g, sdn_seg(x, y, P[i][0], P[i][1], P[i + 1][0], P[i + 1][1]));
        return fmaxf(d, -(g - 0.045f));
    }
    case FO_FELPUDO: {                             // o Velcro: a tira com a beira felpuda
        float d = sdn_caixa(x, y, 0.92f, 0.66f, 0.18f);
        return d + 0.05f * ((float)(hash2((int)floorf(x * 22) + 9, (int)floorf(y * 22) + 9) % 100) / 100.0f - 0.5f);
    }
    case FO_MIRA: {                                // a luneta: o circulo e as quatro marcas para fora
        float d = sqrtf(x * x + y * y) - 0.82f;
        d = fminf(d, sdn_caixa(x, y, 0.10f, 1.04f, 0.03f));
        return fminf(d, sdn_caixa(x, y, 1.04f, 0.10f, 0.03f));
    }
    case FO_TABUA: {                               // a ratoeira: a tabua e o queijo por cima, saindo pela quina
        float d = sdn_caixa(x + 0.08f, y + 0.06f, 0.86f, 0.60f, 0.08f);
        float q = sdn_tri(x, y, 0.20f, 0.50f, 1.00f, 0.50f, 1.00f, -0.12f) - 0.03f;
        an_vinco = q;
        if (q < 0) *parte = 1;
        return fminf(d, q);
    }
    case FO_CARTA: return sdn_caixa(x, y, 0.66f, 0.92f, 0.10f);
    case FO_ROSETA: {                              // a roseta da nota: a beira em ondinhas e as duas fitas embaixo
        float an = atan2f(y + 0.12f, x), ro = sqrtf(x * x + (y + 0.12f) * (y + 0.12f));
        float d = ro - (0.70f + 0.06f * sinf(an * 12));
        an_vinco = d;
        for (int s = -1; s <= 1; s += 2) {
            float fx = x - s * 0.26f, fy = y - 0.62f;
            float fita = sdn_caixa(fx * 0.94f - s * fy * 0.34f, fy * 0.94f + s * fx * 0.34f, 0.17f, 0.36f, 0.02f);
            fita = fmaxf(fita, -(fabsf(fx * 0.94f - s * fy * 0.34f) * 1.1f - (fy - 0.30f)));   // o corte em V na ponta
            if (fita < d && d > 0) { d = fita; *parte = 1; }
        }
        return d;
    }
    case FO_VIRUS: {                               // o germe: a bola e os espinhos de ponta redonda
        float d = sqrtf(x * x + y * y) - 0.66f;
        for (int i = 0; i < 8; i++) {
            float an = i * 0.7854f + 0.39f, ex = cosf(an), ey = sinf(an);
            float e = sdn_seg(x, y, ex * 0.6f, ey * 0.6f, ex * 0.88f, ey * 0.88f) - 0.07f;
            e = fminf(e, sqrtf((x - ex * 0.9f) * (x - ex * 0.9f) + (y - ey * 0.9f) * (y - ey * 0.9f)) - 0.12f);
            d = fminf(d, e);
        }
        return d;
    }
    case FO_GOMA: {                                // o Chiclete: a goma mascada e os fios esticados ate os pingos
        float d = sdn_bolha(x + 0.16f, y + 0.14f, 0.62f, 0.13f, 1.8f, a);
        static const float F[3][7] = { { 0.24f, 0.34f, 0.50f, 0.70f, 0.82f, 0.78f, 0.13f },
                                       { 0.40f, -0.12f, 0.68f, 0.04f, 0.94f, 0.10f, 0.09f },
                                       { -0.24f, 0.40f, -0.28f, 0.72f, -0.06f, 0.94f, 0.08f } };
        for (int i = 0; i < 3; i++) {
            const float *f = F[i];
            float fio = fminf(sdn_ponta(x, y, f[0], f[1], f[2], f[3], 0.07f, 0.03f), sdn_ponta(x, y, f[2], f[3], f[4], f[5], 0.03f, 0.045f));
            float pingo = sqrtf((x - f[4]) * (x - f[4]) + (y - f[5]) * (y - f[5])) - f[6];
            d = fminf(d, fminf(fio, pingo));
        }
        return d;
    }
    case FO_METADES: {                             // a Dupla Face: o quadrado partido na diagonal, as duas caras
        float q = sdn_caixa(x, y, 0.80f, 0.80f, 0.22f);
        if (x + y > 0) *parte = 1;
        return fmaxf(q, -(fabsf(x + y) * 0.7071f - 0.04f));
    }
    case FO_PESO: {                                // o Chumbo: o peso de ferro de balanca, com a alca em cima
        float d = sdn_trap(x, y - 0.30f, 0.86f, 0.54f, 0.50f) - 0.08f;
        float ro = sqrtf(x * x + (y + 0.50f) * (y + 0.50f)), alca = fabsf(ro - 0.27f) - 0.09f;
        an_vinco = d;
        if (alca < d) *parte = 1;
        return fminf(d, alca);
    }
    default: return sqrtf(x * x + y * y) - 0.9f;
    }
}

// ---- As rampas de luz (cada cor da textura, a sua) --------------------------
static uint16_t an_rampa_cor[24], an_rampa[24][4];
static int an_rampa_n;
static const uint16_t *an_rampa_de(uint16_t c)
{
    for (int i = 0; i < an_rampa_n; i++) if (an_rampa_cor[i] == c) return an_rampa[i];
    int i = an_rampa_n < 24 ? an_rampa_n++ : (int)(c % 24);
    an_rampa_cor[i] = c;
    rampa_de(c, an_rampa[i]);
    return an_rampa[i];
}

// O material no ponto: a luz l (como em tom_rampa, ~0.33 a propria cor) que
// ele da a cor c, ou o brilho que ele poe por cima (*sobre != 0). (x, y) no
// recorte (raio 1, sem o giro: a luz e da mesa); e: o quanto falta ate a
// beira, em pixels; (nx, ny): a normal da beira, para fora; s: o tamanho de
// um pixel no raio 1.
static const uint16_t AN_ARCO[6] = { RGB(208, 98, 108), RGB(246, 222, 132), RGB(130, 204, 112),
                                     RGB(168, 204, 230), RGB(206, 180, 218), RGB(226, 164, 168) };
static float an_luz(int m, float x, float y, float e, float nx, float ny, float s, int X, int Y, float fase,
                    float mov, uint16_t *sobre)
{
    float t = t_relogio, lado = -(nx + ny) * 0.7071f;                 // 1: a beira que olha para a luz
    float beira = e < 2.6f ? (1 - e / 2.6f) : 0, bisel = beira * lado;
    float g = -(x + y) * 0.16f;                                        // a luz de cima a esquerda, no adesivo todo
    uint32_t h = hash2(X + 311, Y + 97);
    float grao = ((h & 255) / 255.0f - 0.5f);
    *sobre = 0;
    switch (m) {
    case MN_FOSCO: return 0.33f + bisel * 0.55f + g + grao * 0.22f;
    case MN_VINIL: {
        float hx = x + 0.38f, hy = y + 0.40f;
        return 0.33f + bisel * 0.7f + g + (hx * hx + hy * hy < 0.09f ? 0.32f : 0);
    }
    case MN_BRILHO: {                              // o plastico: a beira de cima acesa, um risco de luz e um ponto
        if (e > 1.2f && e < 3.2f * (s < 0.05f ? 1.6f : 1) && lado > 0.55f && y < 0.25f) { *sobre = C_BRANCO; return 0; }
        float px = x + 0.42f, py = y + 0.42f;
        if (px * px + py * py < 0.016f) { *sobre = C_BRANCO; return 0; }
        return 0.33f + bisel * 0.9f + g * 1.4f;
    }
    case MN_METAL: {                               // o metal escovado: faixas largas de luz, riscos finos
        float faixa = sinf((y * 0.9f + x * 0.25f) * 4.2f + 0.8f);
        float risco = ((hash2(X / 4 + 50, Y + 50) & 7) == 0) ? 0.35f : 0;
        return 0.20f + faixa * 0.42f + bisel * 0.8f + risco - g * 0.5f;
    }
    case MN_CHUMBO: {                              // o chumbo: fosco, a luz larga e mole, as mossas do martelo
        float am = ruido(X * 0.34f + 7, Y * 0.34f + 3);
        return 0.26f + bisel * 0.9f + g * 1.5f + (am > 0.38f ? 0.24f : am < -0.38f ? -0.24f : 0) + grao * 0.08f;
    }
    case MN_OURO: {                                // o ouro: faixas, brilhos que acendem e a luz passando
        float faixa = sinf((y * 0.8f + x * 0.45f) * 3.4f + 1.4f);
        if ((h % 53) == 0 && sinf(t * 3.4f + (h >> 8) % 23) > 0.7f) { *sobre = C_BRANCO; return 0; }
        float p = fr(t / 4.2f + fase), d = (x + y) * 0.7071f - (p / 0.25f * 2.8f - 1.4f);
        if (p < 0.25f && fabsf(d) < 0.12f) { *sobre = C_BRANCO; return 0; }
        return 0.30f + faixa * 0.38f + bisel * 0.9f + g;
    }
    case MN_CROMO: {                               // o espelho: o ceu em cima, a linha escura do horizonte, o chao
        if (y < -0.12f) return 0.95f - (y + 0.9f) * 0.25f + bisel * 0.3f;
        if (y < 0.08f) return -0.85f;
        float hx = x + 0.45f, hy = y + 0.52f;
        if (hx * hx + hy * hy < 0.02f) { *sobre = C_BRANCO; return 0; }
        return -0.20f + (y - 0.08f) * 0.9f + bisel * 0.4f;
    }
    case MN_HOLO: {                                // o holografico: as cores correndo na diagonal, mudando com o dado
        float f = (x * 0.7f - y) * 1.7f + t * 0.45f + mov + fase * 3;
        int k = ((int)floorf(f * 2.2f) % 6 + 6) % 6;
        if (((X + Y) & 1) || fabsf(fr(f * 2.2f) - 0.5f) < 0.18f) *sobre = AN_ARCO[k];
        if ((h % 31) == 0 && sinf(t * 4 + (h >> 8) % 17) > 0.6f) *sobre = C_BRANCO;
        return 0.45f + bisel * 0.6f;
    }
    case MN_GLITTER: {                             // a purpurina fina: cada graozinho acende no seu tempo
        if ((h % 5) == 0) {
            float b = sinf(t * 3.1f + (h >> 6) % 29);
            if (b > 0.55f) { *sobre = (h >> 12) % 3 == 0 ? C_BRANCO : (h >> 12) % 3 == 1 ? C_AMARELO : C_ROSA; return 0; }
            return 0.33f + (b > 0 ? 0.45f : -0.35f);
        }
        return 0.33f + bisel * 0.6f + g + grao * 0.3f;
    }
    case MN_PURPURINA: {                           // a purpurina grossa: muito grao, de muitas cores, piscando
        static const uint16_t P[5] = { RGB(252, 248, 236), RGB(246, 222, 132), RGB(226, 164, 168), RGB(168, 204, 230), RGB(206, 180, 218) };
        if ((h % 3) == 0) {
            float b = sinf(t * 2.6f + (h >> 5) % 31);
            if (b > 0.25f) { *sobre = P[(h >> 9) % 5]; return 0; }
            return 0.33f + (b > -0.4f ? 0.55f : -0.45f);
        }
        return 0.22f + bisel * 0.6f + g + grao * 0.6f;
    }
    case MN_PAPEL: {                               // o papel: fosco, as fibras e a beira um nada mais escura
        float fibra = ((hash2(X / 3 + 77, Y + 13) % 11) == 0) ? (grao > 0 ? 0.3f : -0.3f) : 0;
        return 0.33f + bisel * 0.35f + g * 0.6f + fibra - (e < 1.0f ? 0.3f : 0);
    }
    case MN_TECIDO: {                              // o tecido: a trama, um fio de cada vez mais claro
        float trama = ((X >> 1) + (Y >> 1)) & 1 ? 0.16f : -0.16f;
        return 0.33f + trama + bisel * 0.5f + g * 0.6f + grao * 0.18f;
    }
    case MN_PELO: return 0.33f + bisel * 0.5f + g * 0.9f + grao * 0.25f;   // o pelo: fosco, a luz mole
    case MN_CERA: {                                // a cera: funda, com dois brilhos molhados
        float a1x = x + 0.36f, a1y = y + 0.40f, a2x = x - 0.30f, a2y = y + 0.10f;
        if (a1x * a1x + a1y * a1y * 2.2f < 0.03f) { *sobre = C_BRANCO; return 0; }
        if (a2x * a2x + a2y * a2y < 0.006f) { *sobre = C_BRANCO; return 0; }
        return 0.25f + bisel * 1.1f + g * 1.4f;
    }
    case MN_BORRACHA: {                            // a goma: um brilho largo e mole em cima, e o ponto de luz
        float hx = x + 0.30f, hy = y + 0.34f, d2 = hx * hx * 0.7f + hy * hy * 1.6f;
        if (d2 < 0.012f) { *sobre = C_BRANCO; return 0; }
        return 0.30f + (d2 < 0.10f ? 0.42f : 0) + bisel * 0.8f + g;
    }
    case MN_RESINA: {                              // a resina: o domo grosso de verniz, a janela refletida no alto
        float rr = (x * x + y * y) / 0.85f;
        if (rr > 1) rr = 1;
        float nz = sqrtf(1 - rr), dif = -x * 0.52f - y * 0.60f + nz * 0.62f;
        float wx = x + 0.32f, wy = y + 0.34f;
        if (fabsf(wx) < 0.22f && fabsf(wy) < 0.15f && fabsf(wx) > s * 0.5f && fabsf(wy) > s * 0.5f) { *sobre = C_BRANCO; return 0; }
        float bx = x - 0.40f, by = y - 0.38f;
        if (bx * bx + by * by < 0.010f) { *sobre = C_BRANCO; return 0; }   // o brilho pequeno, do outro lado
        if (e < 1.2f) return 0.70f;                              // a beira da gota de resina
        if (rr > 0.66f && x + y > 0.58f) return 0.80f;          // embaixo, acesa pela luz que atravessa
        return fminf(0.30f + (dif - 0.62f) * 0.85f, 0.52f);
    }
    case MN_MADEIRA: return 0.33f + bisel * 0.45f + g * 0.7f + grao * 0.15f;
    case MN_LAMA: return 0.30f + bisel * 0.6f + g * 0.8f + grao * 0.3f;
    case MN_CETIM: {                               // o cetim: faixas largas e macias de brilho
        float f = sinf((x * 0.6f + y) * 3.2f + t * 0.6f);
        return 0.30f + f * 0.32f + bisel * 0.7f;
    }
    case MN_LENTE: {                               // a lente: o vidro escuro, o reflexo em arco no alto
        float ro = sqrtf(x * x + y * y);
        if (ro > 0.48f && ro < 0.62f && x < -0.05f && y < -0.05f) { *sobre = gfx_mistura(C_BRANCO, C_ACO, 30); return 0; }
        return 0.20f + bisel * 0.7f + g;
    }
    case MN_NEON: return 0.33f + bisel * 0.3f;
    default: return 0.33f + bisel * 0.6f + g;
    }
}

// A colmeia (hexagonos de ponta para cima, de raio tam): 0 no meio de cada
// celula, 1 na beira dela.
static float an_colmeia(float x, float y, float tam)
{
    float q = (0.57735f * x - 0.33333f * y) / tam, r = 0.66667f * y / tam, c = -q - r;
    float rq = roundf(q), rr = roundf(r), rc = roundf(c);
    float dq = fabsf(rq - q), dr = fabsf(rr - r), dc = fabsf(rc - c);
    if (dq > dr && dq > dc) rq = -rr - rc; else if (dr > dc) rr = -rq - rc;
    float hx = tam * (1.73205f * rq + 0.86603f * rr), hy = tam * 1.5f * rr;
    float px = fabsf(x - hx), py = fabsf(y - hy);
    return fmaxf(px, px * 0.5f + py * 0.866025f) / (tam * 0.866025f);
}

// ---- As texturas proprias ---------------------------------------------------
// Devolve a cor, ou 0 para usar a textura de sempre (ad_textura). Pode
// trocar o material so deste pixel (an_mat_px).
static uint16_t an_textura(int a, int parte, float x, float y, float e, float s, int X, int Y, float fase,
                           float mov, uint16_t cor)
{
    const ad_novo_t *v = &AD_NOVO[a];
    float t = t_relogio;
    uint32_t h = hash2(X + 64, Y + 64);
    switch (a) {
    case AD_CHUMBO: {                              // a alca de ferro; no peso, a chapa da frente com o friso
        if (parte == 1) return v->cor2;
        if (e > 2.4f && e < 3.3f) return RGB(104, 112, 124);
        return cor;
    }
    case AD_DOURADO: {                             // a moeda: o aro e o anel gravado
        float ro = sqrtf(x * x + y * y);
        if (ro > 0.74f) return x + y < 0 ? RGB(246, 222, 132) : RGB(190, 156, 86);
        if (fabsf(ro - 0.46f) < s * 0.9f) return x + y < 0 ? RGB(150, 122, 62) : RGB(246, 222, 132);
        return cor;
    }
    case AD_GORJETA: {                             // a ficha de cassino: as listras da beira e o anel de dentro
        float ro = sqrtf(x * x + y * y), an = atan2f(y, x);
        if (ro > 0.66f) return (int)floorf((an + 3.1416f) / 0.5236f) % 2 ? v->cor2 : cor;
        if (ro > 0.60f) return RGB(26, 34, 62);
        if (fabsf(ro - 0.40f) < s * 0.8f && ((int)floorf((an + 3.1416f) / 0.2618f) & 1)) return v->cor2;
        return cor;
    }
    case AD_MIRA: {                                // a luneta: o reticulo vermelho que, de tempos em tempos, trava
        float ro = sqrtf(x * x + y * y), trava = fr(t / 3.2f + fase), aperta = trava > 0.85f ? (trava - 0.85f) / 0.15f : 0;
        float rr = 0.50f - aperta * 0.12f;
        if (fabsf(x) > 0.86f || fabsf(y) > 0.86f) { an_mat_px = MN_METAL; return RGB(140, 150, 164); }   // as marcas de fora
        if (fabsf(ro - rr) < s * 0.7f) return v->cor2;
        if ((fabsf(x) < s * 0.6f && ro > 0.18f && ro < 0.70f) || (fabsf(y) < s * 0.6f && ro > 0.18f && ro < 0.70f)) return v->cor2;
        if (ro < s * 1.3f) return aperta > 0 && (int)(t * 14) % 2 ? C_BRANCO : C_VERMELHO;
        if (ro > 0.72f) { an_mat_px = MN_METAL; return RGB(104, 112, 124); }   // o aro de metal da lente
        return cor;
    }
    case AD_RASPADINHA: {                          // o cartao: o painel prateado no meio, ja raspado em riscos
        if (fabsf(x) < 0.72f && fabsf(y) < 0.40f) {
            int d1 = (int)floorf((x - y) / s + 200), d2 = (int)floorf((x + y) / s / 3);
            uint32_t q = hash2(d1 / 4, d2 + 64);
            if (d1 % 4 <= 1 && (q % 3) == 0) return d1 % 4 == 0 ? C_BRANCO : RGB(150, 112, 160);   // o raspado
            an_mat_px = MN_METAL;
            return ((X + Y) & 1) ? v->cor2 : RGB(140, 150, 164);
        }
        if (fabsf(fabsf(y) - 0.54f) < s * 0.6f && fabsf(x) < 0.80f && (X & 1)) return RGB(150, 112, 160);   // as linhas do cartao
        return cor;
    }
    case AD_FERMENTO: {                            // a massa: bolhas subindo, crescendo e estourando na beira
        for (int i = 0; i < 3; i++) {
            float c = fr(t / 2.2f + fase + i * 0.33f), bx = -0.42f + i * 0.42f + sinf(c * 6 + i) * 0.06f;
            float by = 0.55f - c * 1.05f, br = 0.05f + c * 0.14f, db = sqrtf((x - bx) * (x - bx) + (y - by) * (y - by));
            if (c > 0.92f) continue;
            if (fabsf(db - br) < s * 0.7f) return v->cor2;
            if (db < br && x - bx < -br * 0.3f && y - by < -br * 0.3f) return C_BRANCO;
        }
        if (y > 0.35f && bayer(X, Y) < 0.22f) return v->cor2;       // a massa mais densa embaixo
        return cor;
    }
    case AD_BIS: {                                 // o de tras na cor clara, a borda do da frente marcada
        if (parte == 1) return v->cor2;
        if (e < 2.5f && e > 1.2f) return C_REAL;
        return cor;
    }
    case AD_AVESSO: {                              // a frente listrada; a aba: o verso, o papel liso de baixo
        if (parte == 1) {
            if (an_vinco_px > -1.0f) return RGB(204, 194, 172);       // a beira da aba
            return ((X + 2 * Y) % 5 == 0) ? RGB(204, 194, 172) : v->cor2;
        }
        return ((X + Y) % 6) < 2 ? RGB(96, 182, 198) : cor;
    }
    case AD_SELO: {                                // o sinete afundado na cera: o anel (escuro de um lado, claro do outro)
        float ro = sqrtf(x * x + y * y), k = ro - 0.50f;
        if (fabsf(k) < s * 1.1f) return ((x + y < 0) == (k > 0)) ? v->cor2 : RGB(190, 56, 60);
        if (ro < 0.42f && ro > 0.30f && ((int)floorf((atan2f(y, x) + 3.1416f) / 0.3927f) & 1) && fabsf(ro - 0.36f) < s * 0.6f)
            return v->cor2;                        // os pontinhos do sinete
        return cor;
    }
    case AD_CORRETIVO: {                           // a fita branca por cima da escrita: as letras aparecem de leve
        float ly = fr((y + 3) * 2.6f);
        if (fabsf(ly - 0.5f) < fmaxf(0.10f, s * 1.3f) && (hash2((int)floorf((x + 3) * 6), (int)floorf((y + 3) * 2.6f)) % 3) != 0
            && ((X + Y) & 1)) return RGB(204, 194, 172);
        return cor;
    }
    case AD_REMENDO: {                             // o pano costurado: o ponto da linha em volta
        if (e > 1.5f && e < 2.6f && ((X + Y) % 4) < 2) return C_MARFIM;
        return 0;
    }
    case AD_POSTIT: {                              // a quina dobrada: o verso do papel, mais escuro
        if (parte == 1) return an_vinco_px > -1.0f ? RGB(190, 156, 86) : v->cor2;
        return 0;
    }
    case AD_PURPURINA: return cor;
    case AD_VELCRO: {                              // os ganchinhos do velcro, em fileiras
        int hx = X % 3, hy = Y % 3;
        if (hy == 0 && hx == 0) return RGB(96, 88, 80);
        if (hy == 0 && hx == 1) return RGB(62, 56, 50);
        return cor;
    }
    case AD_RATOEIRA: {                            // a madeira com veio, o arame da mola e o queijo furado
        if (parte == 1) {                          // o queijo: os furos redondos
            static const float F[3][3] = { { 0.86f, 0.28f, 0.08f }, { 0.62f, 0.40f, 0.06f }, { 0.90f, 0.02f, 0.05f } };
            for (int i = 0; i < 3; i++) {
                float fx = x - F[i][0], fy = y - F[i][1], fr2 = fmaxf(F[i][2], s * 1.2f);
                if (fx * fx + fy * fy < fr2 * fr2) return RGB(190, 156, 86);
            }
            return v->cor2;
        }
        float fio = fabsf(sdn_caixa(x + 0.30f, y + 0.06f, 0.40f, 0.36f, 0.10f));   // o arco da mola
        if (fio < s * 0.65f) { an_mat_px = MN_METAL; return C_PRATA; }
        if (fabsf(sdn_caixa(x + 0.30f - s, y + 0.06f - s, 0.40f, 0.36f, 0.10f)) < s * 0.6f) return RGB(56, 32, 20);
        if (fabsf(x - 0.16f) < s * 1.4f && fabsf(y + 0.06f) < 0.16f) { an_mat_px = MN_METAL; return C_ACO; }   // a mola enrolada
        float gv = ruido(x * 1.4f + 2, y * 6.0f + 5);
        if (fabsf(fr(y * 3.6f + gv * 0.5f) - 0.5f) < 0.09f) return RGB(78, 46, 30);
        if ((h % 29) == 0) return RGB(156, 106, 64);
        return cor;
    }
    case AD_FIGURINHA: {                           // a figurinha do album: a moldura branca e a estrela dourada
        if (e > 1.4f && e < 2.4f) return C_BRANCO;
        if (e >= 2.4f && e < 3.2f) return RGB(88, 152, 84);
        if (sdn_estrela((x + 0.36f) / 0.24f, (y + 0.62f) / 0.24f, 1.0f, 0.5f) < 0) { an_mat_px = MN_OURO; return C_OURO; }
        return 0;
    }
    case AD_PATINHO: {                             // o patinho cinza: o bico, o olho e a asa; a agua com ondinhas
        if (parte == 1) {
            float o = 0.05f * sinf(x * 6 + t * 1.6f + fase * 6);
            if (fr((y + o) * 5.5f + 10) < 0.16f && (X % 5) != 0) return C_BRANCO;
            return v->cor2;
        }
        if (x < -0.58f && y < -0.20f) return C_FOGO;
        float ox = x + 0.42f, oy = y + 0.46f, re = fmaxf(s * 1.1f, 0.06f);
        if (ox * ox + oy * oy < re * re) return C_EBANO;
        float wx = x - 0.16f, wy = y - 0.12f, wr = wx * wx / 0.13f + wy * wy / 0.035f;
        if (wr < 1 && wr > 0.40f && wy < 0.06f) return RGB(96, 88, 80);
        return cor;
    }
    case AD_ETIQUETA: {                            // o papel kraft: o reforco do furo e as linhas escritas
        float hr = sqrtf((x - 0.70f) * (x - 0.70f) + y * y);
        if (hr < 0.21f) return RGB(150, 122, 62);
        if (x < 0.40f && x > -0.80f && (fabsf(y + 0.26f) < s * 0.6f || fabsf(y + 0.02f) < s * 0.6f || fabsf(y - 0.22f) < s * 0.6f)
            && ((X / 2) % 4) != 0) return RGB(106, 86, 44);
        return cor;
    }
    case AD_BILHETE: {                             // o bilhete roxo: a faixa holografica, o picote e a borda dourada
        if (x < -0.52f && x > -0.80f) { an_mat_px = MN_HOLO; return RGB(184, 194, 208); }
        if (fabsf(x - 0.54f) < s * 0.6f && ((Y >> 1) & 1)) { an_mat_px = MN_OURO; return C_OURO; }
        if (e > 1.3f && e < 2.2f) { an_mat_px = MN_OURO; return C_OURO; }
        if ((h % 41) == 0) { an_mat_px = MN_OURO; return C_AMARELO; }
        return cor;
    }
    case AD_BOMBA: {                               // a explosao de gibi: o contorno do miolo e a retícula de pontos
        if (an_vinco_px > -0.9f && an_vinco_px < 0.9f) return RGB(118, 30, 38);
        bool ponto = ((X + Y) % 4 == 0) && ((X - Y + 400) % 4 == 0);
        if (parte == 1) return ponto ? C_FOGO : v->cor2;
        return ponto ? RGB(118, 30, 38) : (e < 2.0f ? C_FOGO : cor);
    }
    case AD_LENTICULAR: {                          // as lentes em fileira: cada uma mostra uma das duas imagens, pelo angulo
        int col = (int)floorf((x + 3) / (s * 2));
        int vira = (int)floorf(mov * 3 + sinf(t * 0.7f + fase * 6) * 1.5f);
        bool b = ((col + vira) & 1);
        if (fr((x + 3) / (s * 2)) < 0.5f) an_mat_px = MN_VINIL;   // o lado da lente que olha para a luz
        return b ? cor : v->cor2;
    }
    case AD_CONFETE: {                             // o papel de festa e os papeizinhos coloridos
        static const uint16_t P[6] = { RGB(190, 56, 60), RGB(228, 194, 106), RGB(96, 134, 196), RGB(130, 204, 112),
                                       RGB(208, 98, 108), RGB(150, 112, 160) };
        uint32_t q = hash2(X / 2 + 300, Y / 2 + 300);
        if ((q % 4) == 0 && !(((q >> 7) & 3) == 0 && ((X + Y) & 1))) return P[(q >> 4) % 6];
        return cor;
    }
    case AD_CHICLETE: {                            // a goma mascada: as marcas dos dentes
        float n = ruido(x * 5 + 3, y * 5 + 7);
        if (n > 0.52f) return RGB(208, 98, 108);
        return cor;
    }
    case AD_CARIMBO: {                             // o carimbo de borracha: a moldura dupla e as linhas, a tinta falhando
        bool tinta = (e > 1.4f && e < 2.7f) || (e > 3.6f && e < 4.4f);
        if (!tinta && fabsf(x) < 0.60f && (fabsf(y + 0.24f) < s * 0.8f || fabsf(y - 0.24f) < s * 0.8f)) tinta = ((X / 3) % 4) != 3;
        if (!tinta) return cor;
        if (ruido(X * 0.45f + 11, Y * 0.45f + 5) > 0.42f || (h % 7) == 0) return cor;   // a tinta que nao pegou
        an_mat_px = MN_FOSCO;
        return v->cor2;
    }
    case AD_DUPLA: return parte ? v->cor2 : cor;
    case AD_CASCA: {                               // a casca: as pintas marrons, as pontas passadas, o miolo claro
        if (parte == 1) return v->cor2;
        if (parte == 2) return C_MARFIM;
        for (int i = 0; i < 3; i++) {
            float dx = x - AN_ABA[i][4], dy = y - AN_ABA[i][5];
            if (dx * dx + dy * dy < 0.028f) return RGB(156, 106, 64);
        }
        if ((h % 23) == 0 || (h % 37) == 1) return RGB(156, 106, 64);
        if (e < 1.2f) return RGB(228, 194, 106);
        return cor;
    }
    case AD_LOBO: {                                // o lobo: os olhos amarelos, o focinho claro, o nariz e o pelo
        float ex = fabsf(x);
        if (parte == 1) return v->cor2;
        float ox = ex - 0.25f, oy = y + 0.17f + ox * 0.45f;
        if (ox * ox / 0.017f + oy * oy / 0.0036f < 1) return fabsf(ox) < s * 0.7f ? C_EBANO : C_AMARELO;
        if (y > 0.62f && y < 0.82f && ex < 0.15f - (y - 0.62f) * 0.5f) return C_EBANO;   // o nariz
        if (y > 0.30f && ex < 0.30f - (y - 0.30f) * 0.30f) return C_MARFIM;             // o focinho
        if (ex < 0.055f && y > -0.50f && y < 0.30f) return RGB(208, 98, 108);           // a risca da testa
        float an = atan2f(y + 0.05f, x), ro = sqrtf(x * x + (y + 0.05f) * (y + 0.05f));
        float n = ruido(an * ro * 9.0f + 40, ro * 2.6f + 3);                             // os fios: compridos para fora
        if (n < -0.36f) return RGB(118, 30, 38);
        if (n > 0.50f) return RGB(208, 98, 108);
        return cor;
    }
    case AD_NEON: {                                // o tubo de luz no fundo escuro, respirando
        float p = fr(t / 6 + fase), pulso = 0.5f + 0.5f * sinf(t * 4.2f + fase * 6.3f);
        bool apaga = (p > 0.02f && p < 0.035f) || (p > 0.05f && p < 0.06f);
        if (e > 1.6f && e < 3.0f) return apaga ? C_VINHO : pulso > 0.65f && ((X + Y) & 1) ? C_BRANCO : C_ROSA;
        if (e >= 3.0f && e < 4.2f && !apaga && bayer(X, Y) < 0.3f + 0.4f * pulso) return C_VINHO_CLR;
        int linha = (int)floorf((y + 3) / 0.22f);
        if (fr((y + 3) / 0.22f) < 0.2f || fr((x + 3 + (linha & 1) * 0.16f) / 0.34f) < 0.14f) return C_EBANO;   // os tijolos
        return cor;
    }
    case AD_IMPERMEAVEL: {                         // a bolha de protecao: a colmeia do escudo por baixo da resina
        float ro2 = x * x + y * y, k = 1 / (0.55f + 0.45f * sqrtf(fmaxf(1 - ro2 / 0.85f, 0)));   // curva com o domo
        float c = an_colmeia(x * k, y * k, 0.30f);
        if ((1 - c) * 0.30f * 0.866f < s * 0.55f) return RGB(228, 194, 106);
        return cor;
    }
    default:
        if (parte == 1 && v->cor2) return v->cor2;
        return 0;
    }
}

// ---- O recorte inteiro ------------------------------------------------------
// A forma vai primeiro para uma grade (a distancia em pixels): a sombra, a
// normal da beira e o brilho do neon leem dela, sem medir de novo.
#define AN_G 168
static float an_dd[AN_G * AN_G], an_vc[AN_G * AN_G];
static uint8_t an_pt[AN_G * AN_G];
static uint16_t an_le(int x, int y)                // o que ja esta pintado embaixo (o dado, pelo vidro)
{
    uint16_t *b = gfx_alvo();
    if (!b) b = display_fb();
    if (x < 0 || y < 0 || x >= GFX_W || y >= GFX_H) return C_FELTRO;
    return b[y * GFX_W + x];
}
static uint16_t an_cor(int a)
{
    const ad_novo_t *v = &AD_NOVO[a];
    return v->cor ? v->cor : ADESIVO[a].fundo == AD_SEM_FUNDO ? C_CELESTE : ADESIVO[a].fundo;
}

// Um ponto de dentro do recorte, na grade de W de lado (k = j * W + i): a
// textura, o material e a luz; o vidro mostra o que ja esta embaixo,
// tingido. (dx, dy): o ponto em relacao ao meio, na tela; (ca, sa): o giro
// ao contrario; e: a distancia ate a beira, em pixels; mais: a luz a mais
// (agindo, acende; na sombra da ponta dobrada, apaga). Liso (miudo): so as
// cores das partes, com a luz de cima a esquerda.
static void an_ponto(int a, int k, int i, int j, int W, int x, int y, float dx, float dy, float ca, float sa, float R,
                     float e, float fase, float mov, float mais, bool zerado, bool liso)
{
    const ad_novo_t *v = &AD_NOVO[a];
    int pt = an_pt[k];
    float s = 1 / R, u = (dx * ca - dy * sa) * s, w = (dx * sa + dy * ca) * s, tux = dx * s, tuy = dy * s;
    uint16_t base = pt == 1 && v->cor2 ? v->cor2 : an_cor(a);
    float gx = an_dd[k + (i < W - 1)] - an_dd[k - (i > 0)], gy = an_dd[k + (j < W - 1) * W] - an_dd[k - (j > 0) * W];
    float gl = sqrtf(gx * gx + gy * gy), nx = gl > 1e-6f ? gx / gl : 0, ny = gl > 1e-6f ? gy / gl : -1;
    if (v->material == MN_VIDRO) {                 // o vidro: o que esta embaixo, tingido; a beira e os reflexos
        if (e < 1.5f) { gfx_pixel(x, y, zerado ? C_FELTRO2 : v->cor); return; }
        float f = (tux + tuy) * 0.7071f + 0.25f;
        if (!zerado && (fabsf(f) < s * 1.1f || (fabsf(f - 0.24f) < s * 0.6f && ((x + y) & 1)))) { gfx_pixel(x, y, C_BRANCO); return; }
        if (!zerado && e < 2.5f && nx + ny < -0.5f) { gfx_pixel(x, y, C_BRANCO); return; }   // a quina acesa
        gfx_pixel(x, y, gfx_mistura(an_le(x, y), zerado ? C_FELTRO2 : v->cor, zerado ? 50 : 32));
        return;
    }
    an_mat_px = -1;
    an_vinco_px = an_vc[k];
    uint16_t c = an_textura(a, pt, u, w, e, s, x, y, fase, mov, base);
    if (!c) c = liso ? base : ad_textura(a, u * R, w * R, e, R / 0.70f, fase, mov, base);
    int mat = an_mat_px >= 0 ? an_mat_px : pt && v->mat2 ? v->mat2 : v->material;
    an_mat_px = -1;
    if (zerado) { gfx_pixel(x, y, gfx_mistura(c, C_FELTRO2, 60)); return; }
    float l;
    uint16_t sobre = 0;
    if (liso) l = 0.40f - (tux + tuy) * 0.30f + (e < 1.2f ? -(nx + ny) * 0.30f : 0);
    else l = an_luz(mat, tux, tuy, e, nx, ny, s, x, y, fase, mov, &sobre);
    if (an_vc[k] > 0 && an_vc[k] < 1.3f) l -= 0.60f;   // a sombra da parte de cima na de baixo
    l += mais;
    if (sobre) { gfx_pixel(x, y, sobre); return; }
    gfx_pixel(x, y, tom_rampa(an_rampa_de(c), l < -1 ? -1 : l > 1 ? 1 : l, x, y));
}

// Os extras que saem do recorte (o barbante da etiqueta, o brilho do neon),
// pela grade de W de lado em (x0, y0).
static void an_extras(int a, float cx, float cy, float R, float th, int W, int x0, int y0, int marg, float fase)
{
    const ad_novo_t *v = &AD_NOVO[a];
    float ct = cosf(th), st = sinf(th);
    if (v->forma == FO_ETIQUETA) {                 // o barbante saindo do furo
        float fx = 0.70f, fy = 0, ex = 1.18f, ey = -0.66f;
        float X0 = cx + (fx * ct - fy * st) * R, Y0 = cy + (fx * st + fy * ct) * R;
        float X1 = cx + (ex * ct - ey * st) * R, Y1 = cy + (ex * st + ey * ct) * R;
        gfx_linha((int)X0, (int)Y0, (int)X1, (int)Y1, RGB(204, 194, 172));
        gfx_linha((int)X0 + 1, (int)Y0, (int)X1 + 1, (int)Y1, RGB(150, 122, 62));
    } else if (v->material == MN_NEON) {           // o brilho do neon em volta, respirando
        float pulso = 0.5f + 0.5f * sinf(t_relogio * 4.2f + fase * 6.3f), alc = (2.0f + 3.0f * pulso) * fminf(1, R / 15);
        for (int j = 0; j < W; j++)
            for (int i = 0; i < W; i++) {
                float dd = an_dd[j * W + i] - marg - 1;
                if (dd > 0 && dd < alc && bayer(x0 + i, y0 + j) < (1 - dd / alc) * 0.55f * (0.4f + 0.6f * pulso))
                    gfx_pixel(x0 + i, y0 + j, C_VINHO_CLR);
            }
    }
}

// A forma na grade de W de lado: (ox, oy), o meio do pixel (0, 0) em relacao
// ao meio do recorte.
static void an_grade(int a, int W, float ox, float oy, float ca, float sa, float R)
{
    float s = 1 / R;
    for (int j = 0; j < W; j++)
        for (int i = 0; i < W; i++) {
            float dx = ox + i, dy = oy + j;
            int pt;
            an_dd[j * W + i] = an_forma(a, (dx * ca - dy * sa) * s, (dx * sa + dy * ca) * s, &pt) * R;
            an_pt[j * W + i] = (uint8_t)pt;
            an_vc[j * W + i] = an_vinco * R;
        }
}

// O adesivo colado no lado de cima do dado (r: o raio do dado).
static void ad_novo(float cx, float cy, float r, int a, bool aceso, bool zerado, float ang, int lados)
{
    const ad_novo_t *v = &AD_NOVO[a];
    const adesivo_t *d = &ADESIVO[a];
    float R = r * v->tam / 100.0f * (lados == 4 ? 0.78f : 1.0f);   // o raio do recorte, em pixels
    float th = ang + v->giro * 0.017453f, ca = cosf(-th), sa = sinf(-th);
    int marg = r >= 26 ? 2 : 1;                   // a margem branca, em pixels
    int sob = aceso ? 2 : 1, lev = aceso ? 1 : 0;   // a sombra no dado (agindo: descola um nada)
    float fase = (hash2((int)cx >> 4, (int)cy >> 4) & 255) / 256.0f, mov = ang * 0.8f + (cx + cy) * 0.02f;
    int m = (int)(R * 1.35f) + 4;
    if (2 * m + 1 > AN_G) m = (AN_G - 1) / 2;
    int W = 2 * m + 1, x0 = (int)cx - m, y0 = (int)cy - m;
    bool vidro = v->material == MN_VIDRO, liso = r < 12;
    uint16_t branco = zerado ? gfx_mistura(C_BRANCO, C_FELTRO2, 60) : C_BRANCO, escuro = zerado ? C_FELTRO2 : C_EBANO_B;
    float ox = x0 - cx + 0.5f, oy = y0 - cy + lev + 0.5f;
    an_grade(a, W, ox, oy, ca, sa, R);
    // 1) a sombra no dado (e, agindo, o brilho da cor dele em volta)
    for (int j = 0; j < W; j++)
        for (int i = 0; i < W; i++) {
            int si = i - sob, sj = j - sob;
            if (an_dd[j * W + i] < marg + 1) continue;     // o proprio adesivo cobre (e o vidro mostra o dado limpo)
            if (si >= 0 && sj >= 0 && an_dd[sj * W + si] < marg + 1) gfx_pixel(x0 + i, y0 + j, C_SOMBRA);
            if (aceso && an_dd[j * W + i] < marg + 2) gfx_pixel(x0 + i, y0 + j, d->luz);
        }
    // 2) o contorno, a margem e o adesivo
    for (int j = 0; j < W; j++)
        for (int i = 0; i < W; i++) {
            int k = j * W + i, x = x0 + i, y = y0 + j;
            float dd = an_dd[k];
            if (dd >= marg + 1.0f) continue;
            if (dd >= marg) { gfx_pixel(x, y, escuro); continue; }
            if (dd >= 0) { gfx_pixel(x, y, vidro ? (zerado ? C_FELTRO2 : v->cor) : branco); continue; }
            an_ponto(a, k, i, j, W, x, y, ox + i, oy + j, ca, sa, R, -dd, fase, mov, aceso ? 0.22f : 0, zerado, liso);
        }
    if (!zerado && !liso) an_extras(a, cx, cy - lev, R, th, W, x0, y0, marg, fase);
}

// O adesivo solto (a loja, o pacote, a colecao, o registro, voando): o mesmo
// recorte, sem o dado e sem o numero, do tamanho do adesivo de antes (s: o
// pixel do simbolo de antes; a caixa dele: 7s + 8, ou 7s + 14 de s 2 em
// diante). A sombra na mesa e, descolando (descola 0..1), a ponta de baixo, a
// direita, dobrada sobre ele mostrando o verso.
static void ad_novo_solto(int cx, int cy, int a, int s, float descola)
{
    const ad_novo_t *v = &AD_NOVO[a];
    int caixa = 7 * s + 6 * (s > 1 ? 2 : 1) + 2, marg = s >= 4 ? 2 : 1;
    float f = v->tam / 80.0f;
    f = f < 0.85f ? 0.85f : f > 1.15f ? 1.15f : f;
    float R = (caixa * 0.5f - marg - 1) * f * 1.12f;
    float th = v->giro * 0.017453f, ca = cosf(-th), sa = sinf(-th);
    float fase = (hash2(cx >> 4, cy >> 4) & 255) / 256.0f;
    bool vidro = v->material == MN_VIDRO, liso = R < 7;
    int m = (int)(R * 1.25f) + marg + 3;
    if (2 * m + 1 > AN_G) m = (AN_G - 1) / 2;
    int W = 2 * m + 1, x0 = cx - m, y0 = cy - m;
    float ox = -m + 0.5f, oy = -m + 0.5f;
    an_grade(a, W, ox, oy, ca, sa, R);
    // A dobra: a reta i + j = C, P passos antes da ponta de baixo a direita.
    // Do lado de la, o adesivo levantou; do lado de ca, o pedaco dobrado
    // (o espelho do que levantou) cobre, com o verso para cima.
    int smax = -1;
    for (int j = 0; j < W; j++)
        for (int i = 0; i < W; i++)
            if (an_dd[j * W + i] < marg + 1 && i + j > smax) smax = i + j;
    int P = descola > 0 ? (int)lroundf((0.08f + 0.40f * descola) * R * 1.4142f) + 2 : 0;   // parado, sem dobra
    int C = P > 0 ? smax - P : 1 << 20;
    enum { AC_NADA, AC_CONTORNO, AC_MARGEM, AC_DENTRO, AC_VERSO, AC_SOMBRA };
    static uint8_t cl[AN_G * AN_G];
    for (int j = 0; j < W; j++)
        for (int i = 0; i < W; i++) {
            int k = j * W + i, sg = i + j;
            float dd = an_dd[k];
            uint8_t c = AC_NADA;
            if (sg > C) {
                if (sg == C + 1 && dd < marg + 1) c = AC_CONTORNO;     // a beira da dobra
            } else {
                int i2 = C - j, j2 = C - i;                             // o espelho na dobra
                float d2 = P > 0 && i2 >= 0 && j2 >= 0 && i2 < W && j2 < W ? an_dd[j2 * W + i2] : 99;
                if (d2 < marg) c = AC_VERSO;
                else if (d2 < marg + 1) c = AC_CONTORNO;
                else if (dd < 0) c = d2 < marg + 2.5f ? AC_SOMBRA : AC_DENTRO;
                else if (dd < marg) c = AC_MARGEM;
                else if (dd < marg + 1) c = AC_CONTORNO;
            }
            cl[k] = c;
        }
    // A sombra na mesa: a forma toda, um pouco para baixo e para a direita.
    int sx = s >= 2 ? 2 : 1, sy = s >= 2 ? 3 : 1;
    for (int j = 0; j < W; j++)
        for (int i = 0; i < W; i++) {
            int si = i - sx, sj = j - sy;
            if (cl[j * W + i] == AC_NADA && si >= 0 && sj >= 0 && cl[sj * W + si] != AC_NADA)
                gfx_pixel(x0 + i, y0 + j, C_SOMBRA);
        }
    uint16_t rv[4];
    rampa_de(C_MARFIM, rv);
    for (int j = 0; j < W; j++)
        for (int i = 0; i < W; i++) {
            int k = j * W + i, x = x0 + i, y = y0 + j;
            switch (cl[k]) {
            case AC_CONTORNO: gfx_pixel(x, y, C_BARRA); break;
            case AC_MARGEM: gfx_pixel(x, y, vidro ? v->cor : C_BRANCO); break;
            case AC_VERSO: {                        // o verso: claro na curva da dobra, mais escuro na ponta
                float t = (float)(C - (i + j)) / P;
                if (descola > 0 && i + j >= C - 1) { gfx_pixel(x, y, C_BRANCO); break; }
                uint16_t c = tom_rampa(rv, 0.55f - 0.95f * t, x, y);
                gfx_pixel(x, y, vidro ? gfx_mistura(an_le(x, y), C_CELESTE, 45) : c);
                break;
            }
            case AC_DENTRO: case AC_SOMBRA:
                an_ponto(a, k, i, j, W, x, y, ox + i, oy + j, ca, sa, R, -an_dd[k], fase, 0,
                         cl[k] == AC_SOMBRA ? -0.55f : 0, false, liso);
                break;
            default: break;
            }
        }
    if (!liso) an_extras(a, (float)cx, (float)cy, R, th, W, x0, y0, marg, fase);
}

// A silhueta do recorte solto, numa cor so (o que ainda nao abriu, na
// prateleira da colecao).
static void ad_novo_silhueta(int cx, int cy, int a, int s, uint16_t cor)
{
    const ad_novo_t *v = &AD_NOVO[a];
    int caixa = 7 * s + 6 * (s > 1 ? 2 : 1) + 2, marg = s >= 4 ? 2 : 1;
    float f = v->tam / 80.0f;
    f = f < 0.85f ? 0.85f : f > 1.15f ? 1.15f : f;
    float R = (caixa * 0.5f - marg - 1) * f * 1.12f, th = v->giro * 0.017453f;
    int m = (int)(R * 1.25f) + marg + 3;
    if (2 * m + 1 > AN_G) m = (AN_G - 1) / 2;
    int W = 2 * m + 1;
    an_grade(a, W, -m + 0.5f, -m + 0.5f, cosf(-th), sinf(-th), R);
    for (int j = 0; j < W; j++)
        for (int i = 0; i < W; i++)
            if (an_dd[j * W + i] < marg + 1) gfx_pixel(cx - m + i, cy - m + j, cor);
}

// O selinho do lado (a pilula com o numero) no estilo novo: a cor do
// recorte, a borda branca do papel e o numero escuro ou claro, o que ler.
static bool ad_pilula_nova(int a, uint16_t *borda, uint16_t *fundo, uint16_t *tinta)
{
    if (ad_estilo != ADE_NOVO || a <= AD_NADA || a >= AD_N) return false;
    uint16_t c = an_cor(a);
    int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
    *borda = C_BRANCO;
    *fundo = c;
    *tinta = r * 3 + g * 6 + b < 1100 ? C_BRANCO : C_EBANO;
    return true;
}

// O adesivo solto de adesivos.inc passa por aqui: no estilo novo, o recorte.
static bool ad_solto_novo(int cx, int cy, int a, int s, float descola)
{
    if (ad_estilo != ADE_NOVO || a <= AD_NADA || a >= AD_N) return false;
    ad_novo_solto(cx, cy, a, s < 1 ? 1 : s, descola);
    return true;
}
