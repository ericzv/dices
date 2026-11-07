// ===========================================================================
// Exploracao: a proposta de cada adesivo - a textura, a ferramenta que o poe
// no dado (a colagem na loja) e o gesto dele agindo na mesa. Ver
// adesivo_v2.inc.
// ===========================================================================

// ---- O texto miudo (3x5) dentro do recorte -------------------------------------
// Para o +10 debaixo da prata da Raspadinha: o ponto (x, y) do recorte esta na
// tinta do texto t, centrado em (cx, cy), com s o tamanho de um pixel?
static const uint8_t AN2_MIUDO[13][5] = {
    { 7, 5, 5, 5, 7 }, { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 3, 1, 7 }, { 5, 5, 7, 1, 1 },
    { 7, 4, 7, 1, 7 }, { 7, 4, 7, 5, 7 }, { 7, 1, 1, 2, 2 }, { 7, 5, 7, 5, 7 }, { 7, 5, 7, 1, 7 },
    { 0, 2, 7, 2, 0 }, { 0, 0, 7, 0, 0 }, { 0, 5, 2, 5, 0 },   // + - x
};
static bool an2_miudo(const char *t, float x, float y, float cx, float cy, float s)
{
    int n = (int)strlen(t), w = n * 4 - 1;
    int gx = (int)floorf((x - cx) / s + w * 0.5f), gy = (int)floorf((y - cy) / s + 2.5f);
    if (gx < 0 || gy < 0 || gx >= w || gy >= 5 || gx % 4 == 3) return false;
    char ch = t[gx / 4];
    int g = ch >= '0' && ch <= '9' ? ch - '0' : ch == '+' ? 10 : ch == '-' ? 11 : ch == 'x' ? 12 : -1;
    return g >= 0 && (AN2_MIUDO[g][gy] >> (2 - gx % 4)) & 1;
}

// A Raspadinha: a prata ja raspada (p, 0..1) no ponto (x, y) do recorte? A
// moeda passa em quatro fileiras, de ida e volta, e a beira do raspado e torta.
static bool an2_raspado(float x, float y, float p, int X, int Y)
{
    if (p <= 0) return false;
    float yy = fminf(0.999f, fmaxf(0, (y + 0.40f) / 0.80f)) * 4;
    int k = (int)yy;
    float xx = fminf(1, fmaxf(0, (x + 0.72f) / 1.44f));
    float o = (k + (k & 1 ? 1 - xx : xx)) / 4 + ((hash2(X + 5, Y + 9) & 15) - 7.5f) * 0.004f;
    return o < p;
}

// ---- As texturas ---------------------------------------------------------------
static uint16_t an2_textura(int a, int parte, float x, float y, float e, float s, int X, int Y, float fase, float mov, uint16_t cor)
{
    const ad_novo_t *v = an_ficha(a);
    (void)parte; (void)fase; (void)mov;
    switch (a) {
    case AD_CORRETIVO: {                           // a tinta branca e grossa: os fios do pincel e a beira em relevo
        float gx = an_dd_gx, gy = an_dd_gy;        // a normal da beira (para fora)
        bool molhado = an_fx.p2 > 0;
        if (e < 1.6f && gx + gy > 0.5f) { an_final = e < 0.8f ? RGB(166, 154, 132) : RGB(204, 194, 172); return cor; }   // o relevo, do lado da sombra
        if (e < 1.3f && gx + gy < -0.6f) { an_final = C_BRANCO; return cor; }   // e do lado da luz
        float n = ruido(x * 1.4f + 11, y * 13.0f + 3);
        if (n > 0.40f) { an_final = molhado ? RGB(204, 194, 172) : RGB(238, 230, 212); return cor; }   // um sulco do pincel
        an_final = molhado && n < -0.45f ? C_BRANCO : (n > 0.12f ? RGB(238, 230, 212) : C_BRANCO);
        return cor;
    }
    case AD_CARIMBO: {                             // a tinta do carimbo sobre o dado: chapada, falhando aqui e ali
        float gx = x / s, gy = y / s;
        float n = ruido(gx * 0.30f + 11, gy * 0.30f + 5);
        if (n > 0.55f || (hash2((int)floorf(gx) + 900, (int)floorf(gy) + 300) % 13) == 0) { an_pula = true; return 0; }
        bool molhado = an_fx.p2 > 0;
        an_final = n > 0.30f ? RGB(208, 98, 108) : !molhado && n < -0.40f ? v->cor2 : v->cor;   // rala na beira da falha, forte no meio
        return v->cor;
    }
    case AD_NOTA_DEZ: {                            // a caneta vermelha: o traco cheio; o 10, em letra miuda
        if (parte == 1 && !an2_miudo("10", x, y, 0, -0.84f, s)) { an_pula = true; return 0; }
        an_final = hash2(X, Y) % 9 ? v->cor : v->cor2;
        return v->cor;
    }
    case AD_SELO: {                                // a cera: o sinete afundado (o anel e o brasao); rompendo, as rachaduras
        float qx = x + 0.05f, qy = y + 0.06f, ro = sqrtf(qx * qx + qy * qy);
        if (an_fx.p2 < 0) {                        // rompendo (-p2: 0..1): as rachaduras correm do meio
            float q = -an_fx.p2;
            static const float RC[5][4] = { { 0.0f, 0.0f, 0.62f, -0.40f }, { 0.0f, 0.0f, -0.55f, -0.52f }, { 0.0f, 0.0f, 0.20f, 0.70f },
                                            { 0.30f, 0.18f, 0.72f, 0.36f }, { -0.18f, 0.20f, -0.66f, 0.46f } };
            for (int i = 0; i < 5; i++) {
                float vx = RC[i][2] - RC[i][0], vy = RC[i][3] - RC[i][1], L = sqrtf(vx * vx + vy * vy);
                float t = ((qx - RC[i][0]) * vx + (qy - RC[i][1]) * vy) / (L * L);
                if (t < 0 || t > fminf(1, q * 1.6f - i * 0.12f)) continue;
                float dx = qx - RC[i][0] - vx * t, dy = qy - RC[i][1] - vy * t;
                float dd = sqrtf(dx * dx + dy * dy) - 0.012f * sinf(t * 23 + i);
                float abre = q > 0.55f ? (q - 0.55f) * 0.30f : 0;      // os pedacos se afastam
                if (dd < s * 0.7f + abre) { if (abre > 0 && dd < abre) { an_pula = true; return 0; } return RGB(36, 32, 29); }
            }
        }
        if (an_fx.p2 > 0) return cor;              // ainda lisa: o sinete nao desceu
        float k = ro - 0.48f;
        if (fabsf(k) < s * 1.3f) return ((qx + qy < 0) == (k > 0)) ? v->cor2 : RGB(190, 56, 60);   // o anel em relevo
        if (ro < 0.48f) {
            float st = sdn_estrela(qx / 0.27f, qy / 0.27f, 1.0f, 0.48f) * 0.27f;
            if (st < 0 && st > -s * 1.1f) return (qx + qy < 0) ? RGB(190, 56, 60) : v->cor2;     // a estrela do brasao
            if (st < 0) return RGB(150, 62, 80);
            return gfx_mistura(cor, v->cor2, 35);                     // o fundo do sinete, afundado
        }
        return cor;
    }
    case AD_RASPADINHA: {                          // o cartao: o painel de prata; raspado, o +10 no dourado embaixo
        if (fabsf(x) < 0.72f && fabsf(y) < 0.42f) {
            if (an2_raspado(x, y, an_fx.p2, X, Y)) {
                an_mat_px = MN_OURO;
                return an2_miudo("+10", x, y, 0, 0.02f, s) ? RGB(118, 30, 38) : RGB(246, 222, 132);
            }
            an_mat_px = MN_METAL;
            float f = fr((x - y) * 1.6f);                              // os riscos de luz da prata
            return f < 0.12f ? C_BRANCO : ((X + Y) & 1) ? v->cor2 : RGB(140, 150, 164);
        }
        if (e > 1.2f && e < 2.1f) return RGB(150, 112, 160);           // o fio da beira do cartao
        if (fabsf(fabsf(y) - 0.56f) < s * 0.6f && fabsf(x) < 0.62f && ((int)floorf(x / s) % 3)) return RGB(150, 112, 160);
        return cor;
    }
    default: return 0;
    }
}

// ---- As ferramentas (pixel a pixel, com contorno) ---------------------------------
// O frasco de corretivo: a tampa preta (o cabo do pincel), o vidro branco e o
// rotulo. (x, y): o pe do frasco; tampa: false, ja tirou a tampa.
static void fer_frasco(int x, int y, bool tampa)
{
    for (int j = -13; j <= 0; j++) {
        int w = j > -2 ? 3 : j > -9 ? 4 : 3;
        for (int i = -w; i <= w; i++) {
            uint16_t c = i <= -w + 1 ? C_BRANCO : i >= w - 1 ? C_MARFIM_S : C_MARFIM;
            if (j >= -7 && j <= -4) c = i <= -w + 1 ? RGB(96, 134, 196) : RGB(56, 86, 150);   // o rotulo
            if (j == -6 && i > -w + 1 && i < w - 1) c = C_BRANCO;
            gfx_pixel(x + i, y + j, c);
        }
        gfx_pixel(x - w - 1, y + j, C_BARRA); gfx_pixel(x + w + 1, y + j, C_BARRA);
    }
    gfx_rect(x - 3, y + 1, 7, 1, C_BARRA);
    gfx_rect(x - 4, y - 14, 9, 1, C_BARRA);
    if (tampa) {
        gfx_rect(x - 3, y - 20, 7, 7, C_BARRA);
        gfx_rect(x - 2, y - 19, 5, 6, C_EBANO);
        gfx_rect(x - 2, y - 19, 1, 6, C_EBANO_B);
    } else {                                       // a boca do frasco, sem tampa
        gfx_rect(x - 2, y - 15, 5, 2, C_BARRA);
        gfx_rect(x - 1, y - 15, 3, 1, C_MARFIM_S);
    }
}

// O pincel do corretivo: a tampa preta (o cabo), a haste e o tufo de pelos
// branco de tinta. (x, y): a ponta do tufo; inclinado para cima, a direita.
static void fer_pincel(int x, int y, float incl)
{
    (void)incl;
    static const char *const P[13] = {             // . nada  # contorno  e ebano  E ebano claro  c haste  b branco  m marfim
        "......#####", ".....#EEee#", ".....#Eeee#", "....#Eeee#.", "....#Eeee#.", "...#####...",
        "...#cc#....", "..#cc#.....", ".##b##.....", "#bbbm#.....", "#bbmm#.....", "#bmm#......", ".###.......",
    };
    for (int j = 0; j < 13; j++)
        for (int i = 0; P[j][i]; i++) {
            char c = P[j][i];
            uint16_t k = c == '#' ? C_BARRA : c == 'e' ? C_EBANO : c == 'E' ? C_EBANO_B : c == 'c' ? C_CINZA
                       : c == 'b' ? C_BRANCO : c == 'm' ? C_MARFIM_S : 0;
            if (k) gfx_pixel(x - 2 + i, y - 12 + j, k);
        }
}

// O carimbo: o puxador redondo, o cabo e o bloco de madeira, a borracha
// vermelha embaixo. (x, y): o meio da borracha; aperta 0..1 (o cabo afunda).
static void fer_carimbo(int x, int y, float aperta)
{
    int sq = (int)lroundf(aperta * 2);
    gfx_rect(x - 13, y - 1, 27, 3, C_BARRA);                   // a borracha
    gfx_rect(x - 12, y, 25, 1, RGB(118, 30, 38));
    gfx_rect(x - 14, y - 8 + sq, 29, 8 - sq, C_BARRA);         // o bloco
    gfx_rect(x - 13, y - 7 + sq, 27, 6 - sq, C_MADEIRA_CLR);
    gfx_rect(x - 13, y - 7 + sq, 27, 1, C_TERRA);
    gfx_rect(x - 13, y - 2, 27, 1, C_MADEIRA);
    gfx_rect(x - 2, y - 13 + sq, 5, 6, C_BARRA);               // o cabo
    gfx_rect(x - 1, y - 13 + sq, 3, 5, C_MADEIRA);
    gfx_disco(x, y - 17 + sq, 4, C_BARRA);                     // o puxador
    gfx_disco(x, y - 17 + sq, 3, C_MADEIRA_CLR);
    gfx_pixel(x - 1, y - 19 + sq, C_TERRA); gfx_pixel(x - 2, y - 18 + sq, C_TERRA);
}

// O bastao de lacre: a barra de cera inclinada, a ponta derretida e a
// chama. (x, y): a ponta de baixo; chama 0..1.
static void fer_lacre(int x, int y, float chama)
{
    for (int i = 0; i < 15; i++) {                             // a barra, a 40 graus
        int px = x + (int)(i * 0.77f), py = y - (int)(i * 0.64f);
        gfx_rect(px - 2, py - 2, 5, 5, C_BARRA);
    }
    for (int i = 0; i < 15; i++) {
        int px = x + (int)(i * 0.77f), py = y - (int)(i * 0.64f);
        gfx_rect(px - 1, py - 1, 3, 3, i < 2 ? RGB(66, 18, 24) : RGB(118, 30, 38));
        gfx_pixel(px - 1, py - 1, i < 2 ? RGB(118, 30, 38) : RGB(190, 56, 60));
    }
    if (chama <= 0) return;                        // a chama na ponta, tremendo
    int h = (int)(3 + 3 * chama + sinf(t_relogio * 31) * 1.2f);
    for (int j = 0; j < h; j++) {
        int w = j < h / 2 ? 1 : 0, cx = x - 3 + (int)(sinf(t_relogio * 23 + j) * 0.8f);
        gfx_rect(cx - w, y - 2 - j, 2 * w + 1, 1, j < 2 ? C_FOGO : C_AMARELO);
    }
}

// O sinete: o disco de latao e o cabo de madeira. (x, y): o meio do disco
// embaixo.
static void fer_sinete(int x, int y)
{
    gfx_rect(x - 7, y - 3, 15, 4, C_BARRA);
    gfx_rect(x - 6, y - 2, 13, 2, C_LATAO);
    gfx_rect(x - 6, y - 2, 13, 1, C_OURO);
    gfx_rect(x - 3, y - 6, 7, 3, C_BARRA);
    gfx_rect(x - 2, y - 5, 5, 2, C_LATAO_ESC);
    gfx_rect(x - 3, y - 17, 7, 11, C_BARRA);                   // o cabo
    gfx_rect(x - 2, y - 16, 5, 10, C_MADEIRA_CLR);
    gfx_rect(x - 2, y - 16, 1, 10, C_TERRA);
    gfx_disco(x, y - 19, 3, C_BARRA);
    gfx_disco(x, y - 19, 2, C_MADEIRA_CLR);
}

// A caneta vermelha: o corpo, a ponta escura e o clipe. (x, y): a ponta;
// a caneta sobe para a direita.
static void fer_caneta(int x, int y, bool tampa)
{
    for (int i = 0; i < 17; i++) {
        int px = x + 2 + (int)(i * 0.71f), py = y - 2 - (int)(i * 0.71f);
        gfx_rect(px - 2, py - 2, 4, 4, C_BARRA);
    }
    for (int i = 0; i < 17; i++) {
        int px = x + 2 + (int)(i * 0.71f), py = y - 2 - (int)(i * 0.71f);
        uint16_t c = i < 3 ? C_PRATA : tampa && i > 11 ? RGB(150, 62, 80) : RGB(190, 56, 60);
        gfx_rect(px - 1, py - 1, 2, 2, c);
        gfx_pixel(px - 1, py - 1, i < 3 ? C_BRANCO : RGB(208, 98, 108));
    }
    gfx_pixel(x, y, C_EBANO); gfx_pixel(x + 1, y - 1, C_EBANO);   // a ponta
}

// A moeda de raspar, de pe, inclinada. (x, y): a beira que raspa.
static void fer_moeda(int x, int y, float incl)
{
    int ox = (int)lroundf(incl * 3);
    for (int j = -9; j <= 0; j++) {
        int w = j < -7 || j > -2 ? 2 : 3;
        int cx = x + ox * (-j) / 9;
        gfx_rect(cx - w - 1, y + j, 2 * w + 3, 1, C_BARRA);
    }
    for (int j = -8; j <= -1; j++) {
        int w = j < -6 || j > -3 ? 1 : 2;
        int cx = x + ox * (-j) / 9;
        gfx_rect(cx - w, y + j, 2 * w + 1, 1, C_OURO);
        gfx_pixel(cx - w, y + j, C_AMARELO);
        gfx_pixel(cx + w, y + j, C_LATAO);
    }
}

// ---- A colagem na proposta ---------------------------------------------------------
// O recorte de papel cola como hoje (o papel cai, ele desce e assenta); os
// outros tem a sua ferramenta e o seu tempo. cn2_toca: quando a ferramenta
// encosta no dado (dali em diante o lado mostra o adesivo, aparecendo);
// cn2_dura: a colagem inteira.
static bool cn2_tem(int a) { return ad_v2(a) && an_acab(a) != ACAB_RECORTE; }
static float cn2_toca(int a)
{
    switch (a) {
    case AD_CORRETIVO: return 0.42f;
    case AD_CARIMBO: return 0.46f;
    case AD_SELO: return 0.34f;
    case AD_NOTA_DEZ: return 0.30f;
    default: return CN_BATE;
    }
}
static float cn2_dura(int a)
{
    switch (a) {
    case AD_CORRETIVO: return 1.75f;
    case AD_CARIMBO: return 1.30f;
    case AD_SELO: return 2.05f;
    case AD_NOTA_DEZ: return 1.65f;
    default: return 1.0f;
    }
}

// O caminho do pincel do Corretivo (q 0..1 das pinceladas), no recorte.
static void cn2_pincel_pos(float q, float *x, float *y)
{
    if (q >= 0.90f) q = 0.899f;
    int i = (int)(q / 0.30f);
    float t = (q - i * 0.30f) / 0.30f, x0 = -0.84f + 0.06f * i, x1 = 0.86f - 0.10f * (i == 1);
    *x = (i & 1) ? x1 + (x0 - x1) * t : x0 + (x1 - x0) * t;
    *y = -0.50f + i * 0.50f + 0.05f * sinf(*x * 3.1f + i * 1.7f);
}

// Do recorte (raio 1, girado th) para a tela, no lado em (x, y) de raio R.
static void cn2_tela(float u, float w, float cx, float cy, float R, float th, int *x, int *y)
{
    float c = cosf(th), s = sinf(th);
    *x = (int)lroundf(cx + (u * c - w * s) * R);
    *y = (int)lroundf(cy + (u * s + w * c) * R);
}

// Os sons da colagem, quando o tempo passa por eles (t0 -> t1).
static bool cn2_passou(float t0, float t1, float t) { return t0 < t && t1 >= t; }
static void cn2_sons(int a, float t0, float t1)
{
    switch (a) {
    case AD_CORRETIVO:
        if (cn2_passou(t0, t1, 0.02f)) som_toca(SOM_AD_CORRETIVO);   // o frasco chacoalhando e o pincel
        break;
    case AD_CARIMBO:
        if (cn2_passou(t0, t1, 0.16f)) som_toca(SOM_AD_CARIMBO);     // o TUM cai em 0.46
        break;
    case AD_SELO:
        if (cn2_passou(t0, t1, 0.34f)) som_toca(SOM_AD_FERMENTO);    // a cera pingando
        if (cn2_passou(t0, t1, 1.24f)) som_toca(SOM_AD_SELO);        // o sinete
        break;
    case AD_NOTA_DEZ:
        if (cn2_passou(t0, t1, 0.30f)) som_toca(SOM_AD_CARBONO);     // a caneta riscando
        if (cn2_passou(t0, t1, 1.36f)) som_toca(SOM_AD_NOTA_DEZ);
        break;
    default: break;
    }
}

// Antes de encostar: a ferramenta paira sobre o lado da vez (tc 0) e, colando,
// vai ate ele. (x, y): o meio do lado; rd: o raio do dado.
static void cn2_antes(int x, int y, int tipo, float rd, int a, float tc)
{
    (void)tipo;
    float bob = sinf(t_fase * 3) * 2;
    const ad_novo_t *v = an_ficha(a);
    float R = rd * v->tam / 100.0f, th = v->giro * 0.017453f;
    switch (a) {
    case AD_CORRETIVO: {                           // o frasco chacoalha, a tampa sai com o pincel e desce
        if (tc <= 0) { fer_frasco(x + 4, (int)(y - 24 + bob), true); return; }
        float sac = tc < 0.16f ? sinf(tc * 90) * 2 : 0;
        int fx = x + 16, fy = y - 22;
        fer_frasco(fx + (int)sac, fy, tc < 0.16f);
        if (tc >= 0.16f) {                         // o pincel sai do frasco e vai para o comeco da pincelada
            float q = (tc - 0.16f) / (cn2_toca(a) - 0.16f), qq = q * q * (3 - 2 * q);
            float ux, uw;
            cn2_pincel_pos(0, &ux, &uw);
            int px, py;
            cn2_tela(ux, uw, (float)x, (float)y, R, th, &px, &py);
            int sx = fx, sy = fy - 16;
            fer_pincel(sx + (int)((px - sx) * qq), sy + (int)((py - sy) * qq) - (int)(sinf(q * 3.1416f) * 6), 1);
        }
        return;
    }
    case AD_CARIMBO: {                             // o carimbo paira; colando, sobe um nada e desce batendo
        float hy = y - 30 + bob;
        if (tc > 0) {
            float q = tc / cn2_toca(a);
            hy = q < 0.45f ? y - 30 - 8 * sinf(q / 0.45f * 1.5708f) : (y - 38) + (8 + 38) * ((q - 0.45f) / 0.55f) * ((q - 0.45f) / 0.55f);
        }
        if (hy > y) hy = y;
        fer_carimbo(x, (int)hy, 0);
        return;
    }
    case AD_SELO: {                                // o bastao de lacre, com a chama acendendo
        float ch = tc > 0 ? fminf(1, tc / 0.2f) : 0.25f + 0.1f * sinf(t_fase * 9);
        int px, py;
        cn2_tela(-0.08f, -0.10f, (float)x, (float)y, R, th, &px, &py);
        float q = tc > 0 ? fminf(1, tc / cn2_toca(a)) : 0;
        fer_lacre(px + 3, (int)(py - 18 + bob * (1 - q) + q * 8), ch);
        return;
    }
    case AD_NOTA_DEZ: {                            // a caneta: a tampa sai e ela desce ate o comeco da volta
        float ux, uw;
        an_dez_ponta(0, &ux, &uw);
        int px, py;
        cn2_tela(ux, uw, (float)x, (float)y, R, th, &px, &py);
        if (tc <= 0) { fer_caneta(x - 2, (int)(y - 20 + bob), true); return; }
        float q = fminf(1, tc / cn2_toca(a)), qq = q * q * (3 - 2 * q);
        int sx = x - 2, sy = y - 20;
        fer_caneta(sx + (int)((px - sx) * qq), sy + (int)((py - sy) * qq), false);
        if (q < 1) {                               // a tampa voa para cima e some
            int tx = x + 14 + (int)(q * 10), ty = y - 34 - (int)(sinf(q * 3.1416f) * 10);
            gfx_rect(tx - 2, ty - 3, 5, 7, C_BARRA);
            gfx_rect(tx - 1, ty - 2, 3, 5, RGB(150, 62, 80));
        }
        return;
    }
    default: break;
    }
}

// Depois de encostar (tc: o tempo desde a escolha): o que o lado mostra.
static void cn2_fx(int a, float tc)
{
    float t0 = cn2_toca(a);
    switch (a) {
    case AD_CORRETIVO: {                           // as pinceladas aparecem atras do pincel; o pingo escorre por ultimo
        float q = (tc - t0) / 0.95f;
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        an_fx.p2 = tc < cn2_dura(a) - 0.12f ? 1 : 0;   // molhada ate o fim
        break;
    }
    case AD_CARIMBO:                               // a marca inteira, molhada, depois que a borracha sai
        an_fx.p2 = tc < t0 + 0.55f ? 1 : 0;
        break;
    case AD_SELO: {                                // a cera derrama do meio para fora; o sinete desce e deixa o brasao
        float q = (tc - t0) / 0.78f;
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        an_fx.p2 = tc < 1.24f ? 1 : 0;
        if (tc >= 1.24f && tc < 1.36f) an_fx.escala = 1.06f;   // apertou: a cera abre um nada
        break;
    }
    case AD_NOTA_DEZ: {
        float q = (tc - t0) / 1.05f;
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        an_fx.p2 = 1;
        break;
    }
    default: break;
    }
}

// Depois de encostar: a ferramenta trabalhando, por cima do lado.
static void cn2_depois(int x, int y, float rd, int a, float tc)
{
    const ad_novo_t *v = an_ficha(a);
    float R = rd * v->tam / 100.0f, th = v->giro * 0.017453f, t0 = cn2_toca(a);
    switch (a) {
    case AD_CORRETIVO: {
        int fx = x + 16, fy = y - 22;
        float q = (tc - t0) / 0.95f;
        if (tc < cn2_dura(a) - 0.10f) fer_frasco(fx, fy, false);
        if (q < 0.90f) {                           // o pincel passando
            float ux, uw;
            cn2_pincel_pos(fmaxf(0, q), &ux, &uw);
            int px, py;
            cn2_tela(ux, uw, (float)x, (float)y, R, th, &px, &py);
            fer_pincel(px, py, 1);
        } else if (tc < cn2_dura(a) - 0.12f) {     // volta para o frasco, e a tampa fecha
            float ux, uw;
            cn2_pincel_pos(0.899f, &ux, &uw);
            int px, py;
            cn2_tela(ux, uw, (float)x, (float)y, R, th, &px, &py);
            float k = fminf(1, (tc - (t0 + 0.95f * 0.90f)) / 0.40f), kk = k * k * (3 - 2 * k);
            fer_pincel(px + (int)((fx - px) * kk), py + (int)((fy - 16 - py) * kk) - (int)(sinf(k * 3.1416f) * 8), 1);
        }
        break;
    }
    case AD_CARIMBO: {
        float d = tc - t0;
        if (d < 0.14f) { fer_carimbo(x, y, 1 - d / 0.14f); break; }   // apertado
        if (d < 0.70f) {                           // sobe e vai embora
            float q = (d - 0.14f) / 0.56f;
            fer_carimbo(x + (int)(q * q * 26), y - (int)(q * 34), 0);
        }
        if (d < 0.18f)                             // o ar e a tinta: quatro respingos dos cantos
            for (int i = 0; i < 4; i++) {
                int sx = i & 1 ? 1 : -1, sy = i & 2 ? 1 : -1;
                gfx_rect(x + sx * (15 + (int)(d * 40)), y + sy * (7 + (int)(d * 20)), 2, 1, C_BRANCO);
            }
        break;
    }
    case AD_SELO: {
        float d = tc - t0;
        int px, py;
        cn2_tela(-0.08f, -0.10f, (float)x, (float)y, R, th, &px, &py);
        if (d < 0.80f) {                           // o bastao pingando
            fer_lacre(px + 3, py - 10, 1);
            float g = fmodf(d * 8.0f, 1.0f);       // um pingo caindo de cada vez
            gfx_rect(px, py - 9 + (int)(g * 8), 2, 2, RGB(118, 30, 38));
        } else if (d < 0.94f) {                    // vai embora
            float q = (d - 0.80f) / 0.14f;
            fer_lacre(px + 3 + (int)(q * 22), py - 10 - (int)(q * 14), 1 - q);
        }
        float s0 = 0.84f, s1 = 1.24f - t0, s2 = s1 + 0.22f, s3 = s2 + 0.30f;   // o sinete: desce, aperta, sobe
        if (d >= s0 && d < s3) {
            int sy;
            if (d < s1) { float q = (d - s0) / (s1 - s0); sy = py - 30 + (int)(q * q * 30); }
            else if (d < s2) sy = py;
            else { float q = (d - s2) / (s3 - s2); sy = py - (int)(q * 30); }
            fer_sinete(px, sy - 1);
        }
        if (d >= s2 && d < s3 + 0.25f)             // a fumacinha da cera quente
            for (int i = 0; i < 5; i++) {
                float q = (d - s2) / 0.55f - i * 0.06f;
                if (q <= 0 || q >= 1) continue;
                int fx = px - 6 + i * 3 + (int)(sinf(q * 9 + i) * 2), fy = py - 4 - (int)(q * 20);
                if ((i + (int)(q * 10)) % 3) gfx_rect(fx, fy, 2, 2, q < 0.5f ? C_MARFIM_S : C_CINZA);
            }
        break;
    }
    case AD_NOTA_DEZ: {
        float q = (tc - t0) / 1.05f, ux, uw;
        int px, py;
        if (q < 1) {
            an_dez_ponta(fmaxf(0, q), &ux, &uw);
            cn2_tela(ux, uw, (float)x, (float)y, R, th, &px, &py);
            bool levantou = (q > 0.60f && q < 0.62f) || (q > 0.71f && q < 0.72f) || (q > 0.84f && q < 0.86f);
            fer_caneta(px, py - (levantou ? 3 : 0), false);
        } else {                                   // sobe e vai embora
            float k = fminf(1, (q - 1) * 1.05f / 0.35f);
            an_dez_ponta(0.999f, &ux, &uw);
            cn2_tela(ux, uw, (float)x, (float)y, R, th, &px, &py);
            if (k < 1) fer_caneta(px + (int)(k * 24), py - (int)(k * 26), false);
        }
        break;
    }
    default: break;
    }
}

// ---- A acao na mesa, na proposta ---------------------------------------------------
// ag2_fx: o adesivo agindo no dado (o lugar de an_fx_agindo). e: o evento
// (no pouso, NULL).
static bool ag2_tem(int a) { return ad_v2(a) && (a == AD_RASPADINHA || a == AD_SELO); }
static float ag2_dura(const evento_t *e)
{
    if (ad_v2(e->ad) && e->ad == AD_RASPADINHA && !e->copia) return 2.1f;
    return 0;
}
static void ag2_fx(int a, float p, const evento_t *e)
{
    switch (a) {
    case AD_RASPADINHA:                            // a moeda raspa a prata (o +10 aparece) e depois o cartao inteiro
        if (e && e->copia) { an_fx_agindo(p, false); break; }
        an_fx.p2 = fminf(1, p / 0.34f);
        if (p >= 0.50f) an_fx.apaga = fminf(1, (p - 0.50f) / 0.36f);
        break;
    case AD_SELO:
        if (e && e->valor) an_fx.p2 = -fminf(1, p / 0.55f);   // pagando: o lacre racha e se parte
        else an_fx.escala = p < 0.12f ? 1 + 0.10f * sinf(p / 0.12f * 3.1416f) : 1;   // armando: aperta
        if (e && e->valor && p > 0.85f) an_fx.some = true;
        break;
    default: an_fx_agindo(p, false); break;
    }
}

// O que sai do dado agindo, na proposta (a moeda raspando, as raspas, os
// cacos do lacre).
static void ag2_desenha(const evento_t *e, float p, int ax, int ay)
{
    int a = e->ad;
    const ad_novo_t *v = an_ficha(a);
    float R = R_DADO * v->tam / 100.0f, th = v->giro * 0.017453f;
    switch (a) {
    case AD_RASPADINHA: {
        if (e->copia) break;
        float u, w;
        int px, py;
        if (p < 0.34f) {                           // raspando a prata: quatro fileiras de ida e volta
            float q = p / 0.34f * 4;
            int k = (int)q;
            if (k > 3) k = 3;
            float xx = q - k;
            u = -0.72f + 1.44f * ((k & 1) ? 1 - xx : xx);
            w = -0.30f + k * 0.20f;
        } else if (p < 0.50f) {                    // o premio: a moeda espera de lado
            u = 0.95f; w = 0.20f;
        } else if (p < 0.86f) {                    // e raspa o cartao inteiro, em cinco fileiras
            float q = (p - 0.50f) / 0.36f * 5;
            int k = (int)q;
            if (k > 4) k = 4;
            float xx = q - k;
            u = -1.0f + 2.0f * ((k & 1) ? 1 - xx : xx);
            w = -0.64f + k * 0.32f;
        } else { u = 1.2f; w = 0.6f; }
        cn2_tela(u, w, (float)ax, (float)ay, R, th, &px, &py);
        if (p < 0.92f) fer_moeda(px, py + 1, sinf(t_fase * 30) > 0 ? 1 : -1);
        bool raspa = p < 0.34f || (p >= 0.50f && p < 0.86f);
        for (int i = 0; i < 10 && raspa; i++) {    // as raspas voando da moeda
            float f = fmodf(t_fase * 3.5f + i * 0.137f, 1.0f);
            int rx = px + (int)((i - 4.5f) * f * 3), ry = py - (int)(sinf(f * 3.1416f) * 8) + (int)(f * f * 10);
            gfx_pixel(rx, ry, p < 0.34f ? (i & 1 ? C_PRATA : C_ACO) : (i & 1 ? RGB(206, 180, 218) : RGB(150, 112, 160)));
        }
        if (p >= 0.86f && p < 1)                   // o po que ficou, sumindo
            for (int i = 0; i < 14; i++) {
                float q = (p - 0.86f) / 0.14f;
                int rx = ax - 12 + (int)(hash2(i, 3) % 25), ry = ay - 6 + (int)(hash2(i, 7) % 13) + (int)(q * 4);
                if (bayer(rx, ry) > q) gfx_pixel(rx, ry, i & 1 ? RGB(206, 180, 218) : C_PRATA);
            }
        break;
    }
    case AD_SELO: {
        if (!e->valor || p < 0.55f) break;         // rompeu: os cacos de cera caem
        float q = (p - 0.55f) / 0.45f;
        for (int i = 0; i < 9; i++) {
            float an = i * 0.698f + 0.3f, vv = 6 + (i * 7) % 6;
            int cx = ax + (int)(cosf(an) * vv * (0.4f + q)), cy = ay + (int)(sinf(an) * vv * 0.6f * (0.4f + q) + q * q * 26);
            if (q > 0.8f && i % 2) continue;
            int sz = i % 3 ? 2 : 3;
            gfx_rect(cx + 1, cy + 1, sz, sz, C_SOMBRA);
            gfx_rect(cx, cy, sz, sz, i % 2 ? RGB(118, 30, 38) : RGB(150, 62, 80));
            gfx_pixel(cx, cy, RGB(190, 56, 60));
        }
        break;
    }
    default: break;
    }
}
