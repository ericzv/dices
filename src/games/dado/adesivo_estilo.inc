// ===========================================================================
// Exploracao: outros padroes para o adesivo colado no lado do dado
// ===========================================================================
// Prototipos para comparar (tecla J na partida, e tests/prancha_adesivos.c).
// ad_estilo escolhe como o adesivo aparece no lado de cima do dado:
//   ATUAL     o vinil na forma da face, com a textura, a borda branca e o
//             adesivo da loja em miniatura no canto (o jogo de hoje).
//   MATERIAL  o mesmo vinil, sem a miniatura do canto: o material diz qual e.
//             Os que liam mal ganham um desenho que mostra o efeito, e a
//             raridade vira acabamento (vinil, metalizado, holografico).
//   RECORTE   o dado fica a vista: o adesivo e o simbolo recortado, com a
//             margem branca do papel, colado no meio da face.
//   MOLDURA   o vinil com a textura de hoje, sem a miniatura; a borda diz a
//             familia do efeito (pontos, multiplica, mexe no dado, protege,
//             ataca, fica para depois, dinheiro).
//   RELEVO    o vinil liso, com o simbolo em relevo no meio, como numa medalha.
//   RECREL    recorte em relevo: o adesivo e a silhueta do simbolo, recortada,
//             com a margem branca, e o desenho dele em relevo no proprio vinil;
//             o dado fica a vista em volta e o numero vai por cima, com o
//             contorno do vinil (o recorte de cima esbarrava no numero).
//   RELEVO2   relevo grande: o vinil na forma da face, o simbolo grande em
//             relevo (a luz certa: claro em cima a esquerda, a sombra dele
//             embaixo a direita) e o numero por cima.
//   NOVO      adesivo novo (adesivo_novo.inc): cada um no recorte com a forma
//             dele, a margem branca e a sombra, a textura e o material dele
//             (fosco, brilhante, metal, ouro, cromo, holografico, purpurina,
//             papel, tecido, vidro, neon, cera...), e o numero por cima.
// Nos novos, o adesivo que age nao acende um halo: a ponta dele levanta.
enum { ADE_ATUAL, ADE_MATERIAL, ADE_RECORTE, ADE_MOLDURA, ADE_RELEVO, ADE_RECREL, ADE_RELEVO2, ADE_NOVO, ADE_N };
static const char *const ADE_NOME[ADE_N] = { "atual", "material", "recorte", "moldura", "relevo",
                                             "recorte em relevo", "relevo grande", "adesivo novo" };
static int ad_estilo = ADE_NOVO;        // a exploracao: o adesivo novo ja de saida (J troca)
static float t_aviso_adesivo;            // > 0: o aviso do estilo (tecla J) esta na mesa

// As familias de efeito (a moldura e a ativacao falam por elas).
enum { AF_PONTOS, AF_MULT, AF_DADO, AF_PROTEGE, AF_ATACA, AF_FICA, AF_DINHEIRO, AF_SUJO, AF_N };
static const char *const AF_NOME[AF_N] = { "pontos", "multiplica", "mexe no dado", "protege", "ataca",
                                           "fica para depois", "dinheiro", "sujeira" };
static const uint8_t AD_FAMILIA[AD_N] = {
    [AD_CHUMBO] = AF_DADO, [AD_REFLEXIVO] = AF_DADO, [AD_CONFETE] = AF_PONTOS, [AD_ELETRONICO] = AF_MULT,
    [AD_TRANSPARENTE] = AF_PROTEGE, [AD_DOURADO] = AF_DINHEIRO, [AD_LOBO] = AF_ATACA, [AD_REMENDO] = AF_DADO,
    [AD_RASPADINHA] = AF_PONTOS, [AD_FERMENTO] = AF_PONTOS, [AD_BIS] = AF_FICA, [AD_MIRA] = AF_ATACA,
    [AD_AVESSO] = AF_DADO, [AD_GORJETA] = AF_DINHEIRO, [AD_SELO] = AF_FICA, [AD_FRAGIL] = AF_PONTOS,
    [AD_ESTRELINHA] = AF_PONTOS, [AD_CORRETIVO] = AF_DADO, [AD_IMANTADO] = AF_MULT, [AD_POSTIT] = AF_DADO,
    [AD_HOLOGRAFICO] = AF_MULT, [AD_PURPURINA] = AF_PONTOS, [AD_VELCRO] = AF_FICA, [AD_RATOEIRA] = AF_ATACA,
    [AD_FIGURINHA] = AF_PONTOS, [AD_PATINHO] = AF_PONTOS, [AD_ETIQUETA] = AF_PONTOS, [AD_BILHETE] = AF_MULT,
    [AD_NOTA_DEZ] = AF_DADO, [AD_CARBONO] = AF_DADO, [AD_BOMBA] = AF_DADO, [AD_IMPERMEAVEL] = AF_PROTEGE,
    [AD_LENTICULAR] = AF_MULT, [AD_NEON] = AF_PONTOS, [AD_INFECTADO] = AF_PONTOS, [AD_BARRO] = AF_ATACA,
    [AD_SUJEIRA] = AF_SUJO, [AD_CHICLETE] = AF_FICA, [AD_MASSINHA] = AF_DADO, [AD_CARIMBO] = AF_MULT,
    [AD_DUPLA] = AF_MULT, [AD_CASCA] = AF_ATACA,
};

static bool dentro_poli(float x, float y, const float *vx, const float *vy, int n)
{
    bool d = false;
    for (int i = 0, j = n - 1; i < n; j = i++)
        if ((vy[i] > y) != (vy[j] > y) && x < (vx[j] - vx[i]) * (y - vy[i]) / (vy[j] - vy[i]) + vx[i]) d = !d;
    return d;
}

// ---- O acabamento da raridade ----------------------------------------------
// Por cima do vinil, em poucos pontos: comum, nada (o papel fosco); incomum,
// de tempos em tempos um brilho de vinil passa na diagonal; raro, metalizado
// (a luz parada no alto, pontilhada, e o brilho passa mais vezes); lendario,
// holografico (a faixa que passa e de todas as cores, e uns pontinhos de cor
// acendem). (X, Y): o ponto a partir do meio do adesivo; r: o raio do dado.
static uint16_t ad_acaba(int a, int ix, int iy, float X, float Y, float r, float fase, uint16_t c)
{
    if (ad_estilo == ADE_ATUAL || a <= AD_NADA || a >= AD_N) return c;
    int rar = RAR_ADESIVO[a];
    if (rar == RAR_COMUM) return c;
    static const uint16_t ARCO[6] = { RGB(208, 98, 108), RGB(246, 222, 132), RGB(130, 204, 112),
                                      RGB(168, 204, 230), RGB(206, 180, 218), RGB(226, 164, 168) };
    float per = rar == RAR_INCOMUM ? 6.0f : rar == RAR_RARO ? 4.2f : 3.4f;
    float p = fr(t_relogio / per + fase), pos = (p / 0.3f * 2.6f - 1.3f) * r, d = (X + Y) * 0.7071f - pos;
    if (p < 0.3f && fabsf(d) < 1.1f) return rar == RAR_LENDA ? ARCO[((int)floorf((X - Y) / 3) % 6 + 6) % 6] : C_BRANCO;
    if (p < 0.3f && fabsf(d) < 2.2f && ((ix + iy) & 1)) return rar == RAR_LENDA ? ARCO[((ix + iy) / 2 % 6 + 6) % 6] : C_BRANCO;
    if (rar == RAR_RARO && X + Y < -0.55f * r && bayer(ix, iy) < 0.22f) return gfx_mistura(c, C_BRANCO, 55);   // a luz do metal
    if (rar == RAR_LENDA && (hash2(ix + 64, iy + 64) % 37) == 0 && sinf(t_relogio * 3 + (hash2(ix, iy) % 13)) > 0.4f)
        return ARCO[hash2(ix + 9, iy) % 6];
    return c;
}

// ---- MATERIAL: os desenhos novos dos que liam mal ---------------------------
// O mesmo contrato de ad_textura (o ponto (X, Y) do meio da etiqueta, de: os
// pixels ate a borda). Devolve a cor, ou 0 para usar a textura de hoje.
#define ADN_NADA 0
static uint16_t ad_textura_nova(int a, float X, float Y, float de, float r, float fase, uint16_t fundo)
{
    int ix = (int)floorf(X), iy = (int)floorf(Y);
    float t = t_relogio, dist = sqrtf(X * X + Y * Y);
    switch (a) {
    case AD_ESTRELINHA: {                         // uma estrela grande de nota, em marca d'agua, e duas faiscas que piscam
        float an = atan2f(Y, X) + 1.5708f, k = cosf(2.5f * an);   // 5 pontas, a de cima para cima
        float raio = r * (0.30f + 0.20f * (k > 0 ? k * k : 0));
        bool estrela = dist < raio;
        if (estrela && dist > raio - 1.2f) return C_AMARELO;     // o contorno cheio
        if (estrela) return (ix + iy) & 1 ? RGB(246, 222, 132) : fundo;   // o miolo pontilhado: o numero le por cima
        for (int i = 0; i < 2; i++) {
            float fx = (i ? -0.55f : 0.52f) * r, fy = (i ? 0.48f : -0.5f) * r;
            float s = sinf(t * 2.4f + i * 3.1f + fase * 6.3f);
            if (s < 0.3f) continue;
            float b = s > 0.8f ? 2.5f : 1.5f;
            if ((fabsf(X - fx) < 0.5f && fabsf(Y - fy) < b) || (fabsf(Y - fy) < 0.5f && fabsf(X - fx) < b)) return C_BRANCO;
        }
        return ADN_NADA;
    }
    case AD_DOURADO: {                            // a moeda do jogo, cunhada no meio: o aro, o anel gravado e o brilho dando a volta
        float rr = 0.58f * r, an = atan2f(Y, X);
        if (dist < rr + 1.0f && dist >= rr) return RGB(106, 86, 44);         // o contorno, para a moeda sair da folha
        if (dist < rr && dist > rr - 2.0f) {                                   // o aro: claro em cima a esquerda, escuro embaixo
            float gira = fr(t / 3.8f + fase) * 3.8f, alvo = -2.4f + gira / 0.9f * 6.2832f;
            if (gira < 0.9f && fabsf(an - alvo) < 0.4f) return C_BRANCO;
            return X + Y < 0 ? C_AMARELO : RGB(190, 156, 86);
        }
        if (fabsf(dist - rr * 0.5f) < 0.55f) return X + Y < 0 ? RGB(150, 122, 62) : C_AMARELO;   // o anel gravado
        if (dist < rr) return (hash2(ix + 64, iy + 64) % 11) == 0 ? C_AMARELO : RGB(228, 194, 106);
        if ((hash2(ix + 64, iy + 64) % 7) == 0) return RGB(228, 194, 106);    // a folha em volta, mais funda
        return RGB(190, 156, 86);
    }
    case AD_FERMENTO: {                           // a massa crescendo: bolhas que sobem, crescem e estouram na beira
        for (int i = 0; i < 3; i++) {
            float c = fr(t / 2.2f + fase + i * 0.33f), bx = (-0.42f + i * 0.42f) * r + sinf(c * 6 + i) * 1.2f;
            float by = 0.55f * r - c * 1.1f * r, br = 0.8f + c * 2.6f, db = sqrtf((X - bx) * (X - bx) + (Y - by) * (Y - by));
            if (c > 0.92f) {                                  // estourou: quatro respingos
                if (fabsf(fabsf(X - bx) - br) < 0.6f && fabsf(Y - by) < 0.6f) return C_AMARELO;
                if (fabsf(fabsf(Y - by) - br) < 0.6f && fabsf(X - bx) < 0.6f) return C_AMARELO;
                continue;
            }
            if (fabsf(db - br) < 0.7f) return C_AMARELO;     // o anel da bolha
            if (db < br - 0.7f && X - bx < -br * 0.3f && Y - by < -br * 0.3f) return C_BRANCO;   // o brilho dela
        }
        if (bayer(ix, iy) < 0.18f && Y > 0.2f * r) return RGB(214, 112, 44);   // a massa mais densa embaixo
        return ADN_NADA;
    }
    case AD_MIRA: {                               // a luneta clara: o anel, as quatro marcas e, de tempos em tempos, trava
        float trava = fr(t / 3.2f + fase), aperta = trava > 0.85f ? (trava - 0.85f) / 0.15f : 0;
        float rr = 0.48f * r - aperta * 2;
        if (fabsf(dist - rr) < 0.6f) return RGB(226, 164, 168);
        bool marca = (fabsf(X) < 0.5f && fabsf(Y) > rr - 2.5f && fabsf(Y) < rr + 3.5f)
                  || (fabsf(Y) < 0.5f && fabsf(X) > rr - 2.5f && fabsf(X) < rr + 3.5f);
        if (marca) return RGB(226, 164, 168);
        if (dist < 1.2f) return aperta > 0 && (int)(t * 14) % 2 ? C_BRANCO : C_VERMELHO;   // o ponto: pisca quando trava
        return ADN_NADA;
    }
    case AD_SELO: {                               // o lacre de cera no papel: a gota vermelha, o brilho e a marca no meio
        float bord = 0.46f * r + 1.2f * sinf(atan2f(Y, X) * 5 + 1) + 0.6f * sinf(atan2f(Y, X) * 9);
        if (dist < bord) {
            if (dist > bord - 1.1f) return RGB(66, 18, 24);                  // a beira da cera
            float lx = X + 0.18f * r, ly = Y + 0.18f * r;
            if (lx * lx + ly * ly < 4.5f && X < 0 && Y < 0) return RGB(226, 164, 168);   // o brilho
            if (fabsf(dist - 0.24f * r) < 0.6f) return RGB(66, 18, 24);      // a marca carimbada: um anel
            return RGB(150, 62, 80);
        }
        if ((hash2(ix + 64, iy + 64) % 17) == 0) return RGB(190, 156, 86);   // o papel, com fibras
        return C_MARFIM;
    }
    case AD_RATOEIRA: {                           // a tabua, a mola de arame e o queijo no canto
        float u = X - 0.08f * r, v = Y + 0.05f * r;
        bool arame = (fabsf(v + 0.32f * r) < 0.6f && fabsf(u) < 0.45f * r)
                  || (fabsf(u - 0.45f * r) < 0.6f && v > -0.32f * r && v < 0.38f * r)
                  || (fabsf(u + 0.45f * r) < 0.6f && v > -0.32f * r && v < 0.38f * r);
        float treme = fr(t / 2.7f + fase) < 0.08f ? (ix & 1) : 0;
        if (arame && !treme) return C_PRATA;
        if (arame) return C_BRANCO;
        float qx = (X + 0.58f * r) / (0.46f * r), qy = (0.56f * r - Y) / (0.40f * r);   // o queijo: a fatia no canto, com furos
        if (qx >= 0 && qy >= 0 && qx + qy <= 1) {
            if (qx + qy > 0.86f || qy < 0.12f) return RGB(190, 156, 86);       // a casca
            if ((ix * 7 + iy * 3) % 11 == 0) return RGB(228, 194, 106);         // os furos
            return C_AMARELO;
        }
        float w = tri(Y * 0.22f + 0.9f * ruido(X / 7 + 3, Y / 3));     // o veio da tabua de hoje
        if (w > 0.86f) return C_MADEIRA;
        return ADN_NADA;
    }
    case AD_GORJETA: {                            // a ficha de cassino: as listras da beira e o miolo; outra chega de lado
        float cx2 = 0, cy2 = 0;
        float ch = fr(t / 4 + fase);
        for (int f = 1; f >= 0; f--) {
            float ox = f ? (-0.9f + fminf(1, ch / 0.25f) * 0.9f) * r * (ch < 0.6f ? 1 : 0) + (ch >= 0.6f ? 0 : 0) : 0;
            if (f && (ch < 0.02f || ch > 0.6f)) continue;     // a segunda so passa e volta
            float dx = X - cx2 - ox - (f ? 1.5f : 0), dy = Y - cy2 - (f ? 1.5f : 0), d2 = sqrtf(dx * dx + dy * dy);
            float rr = 0.46f * r;
            if (d2 > rr) continue;
            float an = atan2f(dy, dx);
            if (d2 > rr - 2.2f) return (int)floorf((an + 3.1416f) / 0.5236f) % 2 ? C_MARFIM : RGB(56, 86, 150);
            if (d2 > rr - 3.2f) return RGB(26, 34, 62);
            return (ix + iy) & 1 ? RGB(96, 134, 196) : RGB(56, 86, 150);
        }
        return ADN_NADA;
    }
    case AD_BIS: {                                // o eco: um contorno da face sai para o lado e volta, como um dado a mais
        float c = fr(t / 3 + fase), sai = c < 0.5f ? c / 0.5f : 1 - (c - 0.5f) / 0.5f;
        float off = sai * 3.0f, dd = de - off;
        if (fabsf(dd - 2.0f) < 0.6f && ((ix + iy) & 1)) return C_BRANCO;
        if (de >= 1.5f && de < 2.5f) return C_REAL;
        return ADN_NADA;
    }
    default: return ADN_NADA;
    }
}
static bool ad_redesenhado(int a)
{
    return a == AD_ESTRELINHA || a == AD_DOURADO || a == AD_FERMENTO || a == AD_MIRA || a == AD_SELO
        || a == AD_RATOEIRA || a == AD_GORJETA || a == AD_BIS;
}
// A tinta do numero sobre o adesivo, no estilo de agora (o lacre e a moeda
// pedem a tinta escura).
static uint16_t ad_tinta_estilo(int a)
{
    if (ad_estilo == ADE_MATERIAL && a == AD_SELO) return RGB(66, 18, 24);
    if (ad_estilo == ADE_MATERIAL && a == AD_ESTRELINHA) return C_BRANCO;
    if (ad_estilo == ADE_MATERIAL && a == AD_GORJETA) return C_MARFIM;
    if (ad_estilo == ADE_RECORTE) return RGB(36, 32, 29);               // o numero fica no dado, na tinta escura
    return ADESIVO[a].tinta;
}
static uint16_t ad_fundo_estilo(int a)
{
    if (ad_estilo == ADE_MATERIAL && a == AD_SELO) return C_MARFIM;      // o papel do lacre
    return ADESIVO[a].fundo;
}

// ---- A ponta que levanta (o adesivo agindo, nos estilos novos) --------------
// A quina de cima, a direita, descola: o verso branco dobrado, a sombra
// embaixo dele e o lado do dado que aparece. q: o quanto (0..1).
static void ad_ponta(float cx, float cy, float r, float q, uint16_t luz)
{
    if (q <= 0) return;
    int L = (int)(2 + q * 0.30f * r), x0 = (int)(cx + 0.62f * r), y0 = (int)(cy - 0.62f * r);
    for (int j = 0; j < L; j++)                         // o pedaco descolado: o dado por baixo, na sombra
        for (int i = 0; i < L - j; i++) gfx_pixel(x0 - i, y0 + j, C_SOMBRA);
    for (int j = 0; j < L; j++)                         // o verso dobrado, branco, com a beira na luz
        for (int i = 0; i <= j; i++) {
            int x = x0 - L + 1 + i + (L - 1 - j), y = y0 + j - (L - 1 - j) - 1;
            gfx_pixel(x + 1, y, i == j || i == 0 ? luz : C_MARFIM);
        }
}

// ---- MOLDURA: a borda diz a familia -----------------------------------------
static void ad_moldura_px(int fam, int x, int y, float cx, float cy, float de, uint16_t luz)
{
    float an = atan2f(y - cy, x - cx), t = t_relogio;
    int passo = (int)floorf((an + 3.1416f) / 6.2832f * 40);   // ~40 marcas na volta
    uint16_t c = C_BRANCO;
    switch (fam) {
    case AF_PONTOS:                                   // o ouro com brilhos que acendem um de cada vez
        c = passo % 5 == (int)(t * 3) % 5 && de > 0.8f ? C_BRANCO : (passo & 1 ? C_OURO : C_AMARELO);
        break;
    case AF_MULT:                                     // a borda dupla: branca, um fio do vinil e branca de novo
        c = de < 0.9f || de > 1.9f ? C_BRANCO : luz;
        break;
    case AF_DADO:                                     // as setas que correm em volta (o dado que muda)
        c = ((passo + (int)(t * 6)) % 4 == 0) ? C_EBANO_B : C_BRANCO;
        break;
    case AF_PROTEGE:                                  // o vidro grosso: claro em cima a esquerda, azul embaixo
        c = (x - cx) + (y - cy) < 0 ? C_BRANCO : (de < 1.2f ? C_VENTO : RGB(96, 134, 196));
        break;
    case AF_ATACA:                                    // o serrilhado vermelho
        c = passo % 2 ? C_VERMELHO : (de > 1.2f ? RGB(66, 18, 24) : C_VERMELHO);
        break;
    case AF_FICA:                                     // a costura: o pesponto branco no tecido escuro
        c = passo % 3 == 0 ? C_EBANO_B : (de > 0.6f && de < 1.8f ? C_BRANCO : C_MARFIM_S);
        break;
    case AF_DINHEIRO:                                 // a serrilha da moeda
        c = passo % 2 ? C_OURO : C_LATAO_ESC;
        break;
    default:                                          // a sujeira: a borda falhada
        c = hash2(passo, 7) % 3 ? RGB(120, 78, 50) : C_MARFIM_S;
        break;
    }
    gfx_pixel(x, y, c);
}

// O ponto (i, j) do simbolo 7x7 de a e tinta? (o Confete nao tem desenho em
// ic: os papeizinhos de ad_icone.)
static bool ic_tinta(int a, int i, int j)
{
    if (i < 0 || j < 0 || i > 6 || j > 6) return false;
    if (a == AD_CONFETE) {
        static const int8_t P[7][2] = { {1,0}, {5,1}, {3,2}, {0,4}, {6,4}, {2,6}, {5,6} };
        for (int k = 0; k < 7; k++) if (P[k][0] == i && P[k][1] == j) return true;
        return false;
    }
    return ADESIVO[a].ic[j] && ADESIVO[a].ic[j][i] != '.';
}

// ---- RECORTE: o simbolo recortado, com a margem do papel ---------------------
// A mascara do simbolo em escala s, alargada um ponto e com os buracos
// cheios (o vinil); em volta, a margem branca e o contorno escuro.
#define REC_MAX 48
// A mascara: 1 a tinta do simbolo, 2 o vinil (o simbolo alargado um ponto,
// com os buracos de dentro cheios), 3 a margem branca, 4 o contorno.
static uint8_t M[REC_MAX][REC_MAX];
static int rec_mascara(int a, int s)
{
    int w = 7 * s, m = 3, W = w + 2 * m;
    memset(M, 0, sizeof M);
    for (int j = 0; j < 7; j++)
        for (int i = 0; i < 7; i++) {
            if (!ic_tinta(a, i, j)) continue;
            for (int b = 0; b < s; b++) for (int e = 0; e < s; e++) M[m + j * s + b][m + i * s + e] = 1;
        }
    // alarga 1 (o vinil), enche os buracos, alarga mais 1 (a margem) e 1 (o contorno)
    for (int pass = 2; pass <= 4; pass++)
        for (int y = 1; y < W - 1; y++)
            for (int x = 1; x < W - 1; x++)
                if (!M[y][x] && (M[y - 1][x] == pass - 1 || M[y + 1][x] == pass - 1 || M[y][x - 1] == pass - 1
                                 || M[y][x + 1] == pass - 1 || (pass == 2 && M[y][x] == 0 &&
                                 (M[y - 1][x] == 1 || M[y + 1][x] == 1 || M[y][x - 1] == 1 || M[y][x + 1] == 1))))
                    M[y][x] = (uint8_t)(pass == 2 ? 2 : pass);
    // os buracos de dentro do vinil: o que nao se alcanca de fora vira vinil
    static uint8_t fora[REC_MAX][REC_MAX];
    memset(fora, 0, sizeof fora);
    int pilha[REC_MAX * REC_MAX], np = 0;
    for (int i = 0; i < W; i++) { pilha[np++] = i; pilha[np++] = (W - 1) * W + i; pilha[np++] = i * W; pilha[np++] = i * W + W - 1; }
    while (np) {
        int p = pilha[--np], x = p % W, y = p / W;
        if (x < 0 || y < 0 || x >= W || y >= W || fora[y][x] || (M[y][x] && M[y][x] <= 2)) continue;
        fora[y][x] = 1;
        if (np < REC_MAX * REC_MAX - 4) { pilha[np++] = p + 1; pilha[np++] = p - 1; pilha[np++] = p + W; pilha[np++] = p - W; }
    }
    for (int y = 0; y < W; y++) for (int x = 0; x < W; x++) if (!fora[y][x] && !M[y][x]) M[y][x] = 2;
    return W;
}
static void ad_recorte(float cx, float cy, float r, int a, bool aceso, bool zerado)
{
    const adesivo_t *d = &ADESIVO[a];
    int s = r >= 16 ? 2 : 1, m = 3, W = rec_mascara(a, s);
    int x0 = (int)cx - W / 2, y0 = (int)cy - W / 2 + (aceso ? -1 : 0);
    uint16_t vin = d->fundo == AD_SEM_FUNDO ? RGB(168, 204, 230) : d->fundo, marg = C_BRANCO, cont = C_EBANO_B;
    if (zerado) { vin = gfx_mistura(vin, C_FELTRO2, 60); marg = gfx_mistura(marg, C_FELTRO2, 60); cont = C_FELTRO2; }
    float fase = (hash2((int)cx >> 4, (int)cy >> 4) & 255) / 256.0f;
    for (int y = 0; y < W; y++)                          // a sombra do papel, um ponto abaixo e a direita
        for (int x = 0; x < W; x++)
            if (M[y][x] && M[y][x] <= 4 && x + 1 < W && y + 1 < W && !M[y + 1][x + 1]) gfx_pixel(x0 + x + 1, y0 + y + 1, C_SOMBRA);
    for (int y = 0; y < W; y++)
        for (int x = 0; x < W; x++) {
            int v = M[y][x];
            if (!v) continue;
            uint16_t c = v == 4 ? cont : v == 3 ? marg : vin;
            if (v <= 2 && !zerado) {                     // o vinil: a luz em cima a esquerda, e o acabamento
                if (x + y < W * 0.7f && bayer(x0 + x, y0 + y) < 0.35f) c = d->luz == C_BRANCO ? gfx_mistura(vin, C_BRANCO, 35) : d->luz;
                c = ad_acaba(a, x0 + x, y0 + y, (float)(x - W / 2), (float)(y - W / 2), r, fase, c);
            }
            gfx_pixel(x0 + x, y0 + y, c);
        }
    ad_icone(x0 + m, y0 + m, a, s, zerado ? gfx_mistura(d->tinta, C_FELTRO2, 60) : d->tinta);
    if (aceso) ad_ponta((float)(x0 + W / 2), (float)(y0 + W / 2), (float)W * 0.62f, 1, d->luz);
}

// ---- RELEVO: o vinil liso e o simbolo em relevo ------------------------------
static void ad_relevo(float cx, float cy, float r, int a, bool aceso, bool zerado, float *vx, float *vy, int n)
{
    const adesivo_t *d = &ADESIVO[a];
    uint16_t base = d->fundo == AD_SEM_FUNDO ? RGB(168, 204, 230) : d->fundo, rmp[4];
    rampa_de(base, rmp);                              // [0] claro ... [3] escuro
    if (zerado) for (int i = 0; i < 4; i++) rmp[i] = gfx_mistura(rmp[i], C_FELTRO2, 60);
    float fase = (hash2((int)cx >> 4, (int)cy >> 4) & 255) / 256.0f;
    int s = r >= 16 ? 2 : 1, w = 7 * s, ox = (int)cx - w / 2, oy = (int)cy - w / 2;
    float ymin = 1e9f, ymax = -1e9f, xmin = 1e9f, xmax = -1e9f;
    for (int i = 0; i < n; i++) { ymin = fminf(ymin, vy[i]); ymax = fmaxf(ymax, vy[i]); xmin = fminf(xmin, vx[i]); xmax = fmaxf(xmax, vx[i]); }
    for (int y = (int)ymin; y <= (int)ymax; y++)
        for (int x = (int)xmin; x <= (int)xmax; x++) {
            if (!dentro_poli(x + 0.5f, y + 0.5f, vx, vy, n)) continue;
            float g = ((x - cx) + (y - cy)) / (2 * r) + 0.5f;  // 0 em cima a esquerda, 1 embaixo a direita
            uint16_t c = g < 0.35f ? (bayer(x, y) < 0.5f ? rmp[1] : rmp[2]) : g > 0.72f ? (bayer(x, y) < 0.5f ? rmp[2] : rmp[3]) : rmp[2];
            int ix = x - ox, iy = y - oy;                // o simbolo: a beira de cima clara, a de baixo escura
            bool em = ix >= 0 && iy >= 0 && ix < w && iy < w && ic_tinta(a, ix / s, iy / s);
            if (em) {
                bool cima = ix - 1 < 0 || iy - 1 < 0 || !ic_tinta(a, (ix - 1) / s, (iy - 1) / s);
                bool baixo = ix + 1 >= w || iy + 1 >= w || !ic_tinta(a, (ix + 1) / s, (iy + 1) / s);
                c = cima ? rmp[0] : baixo ? rmp[3] : (aceso ? d->luz : rmp[1]);
                if (aceso && cima) c = C_BRANCO;
            }
            if (!zerado) c = ad_acaba(a, x, y, x - cx, y - cy, r, fase, c);
            gfx_pixel(x, y, c);
        }
    if (aceso) ad_ponta(cx, cy, r * 0.95f, 1, d->luz);
}

// ---- RECREL: o recorte em relevo --------------------------------------------
// A silhueta do simbolo, recortada (a mascara do recorte), no vinil: claro em
// cima a esquerda e mais escuro embaixo; o desenho em relevo, no tom claro,
// com a beira de baixo escura e a sombra dele no vinil; a margem branca, o
// contorno e a sombra do papel no dado. Agindo, o desenho acende e a ponta
// levanta. O numero vem por cima (desenha_dado_base), com o contorno do vinil.
static void ad_recrel(float cx, float cy, float r, int a, bool aceso, bool zerado)
{
    const adesivo_t *d = &ADESIVO[a];
    int s = r >= 26 ? 3 : r >= 16 ? 2 : 1, W = rec_mascara(a, s);
    int x0 = (int)cx - W / 2, y0 = (int)cy - W / 2 - (aceso ? 1 : 0), so = aceso ? 2 : 1;   // agindo, descola um nada
    uint16_t vin = d->fundo == AD_SEM_FUNDO ? RGB(168, 204, 230) : d->fundo, rmp[4], marg = C_BRANCO, cont = C_EBANO_B;
    rampa_de(vin, rmp);                                   // [0] o escuro ... [3] o claro
    if (zerado) {
        for (int i = 0; i < 4; i++) rmp[i] = gfx_mistura(rmp[i], C_FELTRO2, 60);
        marg = gfx_mistura(marg, C_FELTRO2, 60); cont = C_FELTRO2;
    }
    float fase = (hash2((int)cx >> 4, (int)cy >> 4) & 255) / 256.0f;
    for (int y = 0; y < W; y++)                           // a sombra do papel no dado, abaixo e a direita
        for (int x = 0; x < W; x++)
            if (M[y][x] && (x + so >= W || y + so >= W || !M[y + so][x + so])) gfx_pixel(x0 + x + so, y0 + y + so, C_SOMBRA);
    for (int y = 0; y < W; y++)
        for (int x = 0; x < W; x++) {
            int v = M[y][x];
            if (!v) continue;
            int X = x0 + x, Y = y0 + y;
            uint16_t c;
            if (v == 4) c = cont;
            else if (v == 3) c = marg;
            else {
                float g = (float)(x + y) / (2 * W);           // 0 em cima a esquerda, 1 embaixo a direita
                c = g < 0.34f ? (bayer(X, Y) < 0.45f ? rmp[3] : rmp[2]) : g > 0.70f ? (bayer(X, Y) < 0.5f ? rmp[1] : rmp[2]) : rmp[2];
                if (v == 1) {                                  // o desenho, alto: claro, com a beira de baixo escura
                    bool baixo = x + 1 < W && y + 1 < W && M[y + 1][x + 1] != 1 && M[y + 1][x] != 1;
                    c = aceso ? (baixo ? rmp[1] : (d->luz == C_BRANCO ? C_BRANCO : d->luz)) : baixo ? rmp[1] : rmp[3];
                } else if (x > 0 && y > 0 && M[y - 1][x - 1] == 1) c = c == rmp[1] ? rmp[0] : rmp[1];   // a sombra do desenho no vinil
                if (!zerado) c = ad_acaba(a, X, Y, (float)(x - W / 2), (float)(y - W / 2), r, fase, c);
            }
            gfx_pixel(X, Y, c);
        }
    if (aceso) ad_ponta((float)(x0 + W / 2), (float)(y0 + W / 2), (float)W * 0.62f, 1, d->luz);
}

// ---- RELEVO2: o relevo grande ----------------------------------------------
// O vinil na forma da face, com a luz da mesa (claro em cima a esquerda); o
// simbolo grande, quase a face toda, em relevo: alto e claro, a beira de
// baixo escura e a sombra dele no vinil. O numero vem por cima.
static void ad_relevo2(float cx, float cy, float r, int a, bool aceso, bool zerado, float *vx, float *vy, int n)
{
    const adesivo_t *d = &ADESIVO[a];
    uint16_t base = d->fundo == AD_SEM_FUNDO ? RGB(168, 204, 230) : d->fundo, rmp[4];
    rampa_de(base, rmp);                                  // [0] o escuro ... [3] o claro
    if (zerado) for (int i = 0; i < 4; i++) rmp[i] = gfx_mistura(rmp[i], C_FELTRO2, 60);
    float fase = (hash2((int)cx >> 4, (int)cy >> 4) & 255) / 256.0f;
    int s = r >= 26 ? 4 : r >= 16 ? 3 : 2, w = 7 * s, ox = (int)cx - w / 2, oy = (int)cy - w / 2;
    float ymin = 1e9f, ymax = -1e9f, xmin = 1e9f, xmax = -1e9f;
    for (int i = 0; i < n; i++) { ymin = fminf(ymin, vy[i]); ymax = fmaxf(ymax, vy[i]); xmin = fminf(xmin, vx[i]); xmax = fmaxf(xmax, vx[i]); }
#define EM(ix, iy) ((ix) >= 0 && (iy) >= 0 && (ix) < w && (iy) < w && ic_tinta(a, (ix) / s, (iy) / s))
    for (int y = (int)ymin; y <= (int)ymax; y++)
        for (int x = (int)xmin; x <= (int)xmax; x++) {
            if (!dentro_poli(x + 0.5f, y + 0.5f, vx, vy, n)) continue;
            float g = ((x - cx) + (y - cy)) / (2 * r) + 0.5f;   // 0 em cima a esquerda, 1 embaixo a direita
            uint16_t c = g < 0.36f ? (bayer(x, y) < 0.5f ? rmp[3] : rmp[2]) : g > 0.70f ? (bayer(x, y) < 0.5f ? rmp[1] : rmp[2]) : rmp[2];
            int ix = x - ox, iy = y - oy;
            if (EM(ix, iy)) {                              // o simbolo, alto
                bool baixo = !EM(ix + 1, iy + 1) && (!EM(ix + 1, iy) || !EM(ix, iy + 1));
                bool cima = !EM(ix - 1, iy - 1) && (!EM(ix - 1, iy) || !EM(ix, iy - 1));
                c = baixo ? rmp[1] : cima ? (aceso ? C_BRANCO : rmp[3]) : (aceso ? (d->luz == C_BRANCO ? rmp[3] : d->luz) : rmp[3]);
            } else if (EM(ix - 1, iy - 1)) c = rmp[0];           // a sombra dele no vinil
            if (!zerado) c = ad_acaba(a, x, y, x - cx, y - cy, r, fase, c);
            gfx_pixel(x, y, c);
        }
#undef EM
    if (aceso) ad_ponta(cx, cy, r * 0.95f, 1, d->luz);
}
