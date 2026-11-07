// ===========================================================================
// Exploracao: a proposta de cada adesivo - a textura, a ferramenta que o poe
// no dado (a colagem na loja) e o gesto dele agindo na mesa. Ver
// adesivo_v2.inc.
// ===========================================================================

// ---- O texto miudo (3x5) dentro do recorte -------------------------------------
// Para o +10 debaixo da prata da Raspadinha: o ponto (x, y) do recorte esta na
// tinta do texto t, centrado em (cx, cy), com s o tamanho de um pixel?
static const uint8_t AN2_MIUDO[16][5] = {
    { 7, 5, 5, 5, 7 }, { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 3, 1, 7 }, { 5, 5, 7, 1, 1 },
    { 7, 4, 7, 1, 7 }, { 7, 4, 7, 5, 7 }, { 7, 1, 1, 2, 2 }, { 7, 5, 7, 5, 7 }, { 7, 5, 7, 1, 7 },
    { 0, 2, 7, 2, 0 }, { 0, 0, 7, 0, 0 }, { 0, 5, 2, 5, 0 },   // + - x
    { 6, 5, 6, 5, 5 }, { 3, 6, 2, 3, 6 }, { 0, 0, 0, 2, 4 },   // R $ ,
};
static bool an2_miudo(const char *t, float x, float y, float cx, float cy, float s)
{
    int n = (int)strlen(t), w = n * 4 - 1;
    int gx = (int)floorf((x - cx) / s + w * 0.5f), gy = (int)floorf((y - cy) / s + 2.5f);
    if (gx < 0 || gy < 0 || gx >= w || gy >= 5 || gx % 4 == 3) return false;
    char ch = t[gx / 4];
    int g = ch >= '0' && ch <= '9' ? ch - '0' : ch == '+' ? 10 : ch == '-' ? 11 : ch == 'x' ? 12
          : ch == 'R' ? 13 : ch == '$' ? 14 : ch == ',' ? 15 : -1;
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
    case AD_MASSINHA: {                            // a massinha fosca: a digital do dedao que apertou
        if (an_fx.p2 > 0) return cor;              // ainda nao apertou
        float fx = (x - 0.06f) / 0.86f, fy = (y + 0.02f) / 1.10f, fr2 = sqrtf(fx * fx + fy * fy);
        if (fr2 < 0.58f && fr(fr2 * 10.5f + 0.18f * sinf(atan2f(fy, fx) * 2 + 0.6f)) < 0.30f) return v->cor2;
        return cor;
    }
    case AD_INFECTADO: {                           // a gosma verde: as veias escuras pulsando, as bolhas e as manchas
        float pulso = 0.5f + 0.5f * sinf(t_relogio * 3.2f + fase * 6.3f);
        if (parte == 1) return pulso > 0.6f && e > 1.0f ? cor : v->cor2;
        for (int i = 0; i < 4; i++) {
            float bx = -0.20f + (i & 1) * 0.36f + 0.03f * sinf(t_relogio * 2 + i), by = -0.16f + (i >> 1) * 0.30f;
            float br = 0.08f + 0.03f * (i == 1), dd = sqrtf((x - bx) * (x - bx) + (y - by) * (y - by));
            if (fabsf(dd - br) < s * 0.7f) return v->cor2;
            if (dd < br && x < bx && y < by) { an_final = C_BRANCO; return cor; }
        }
        if (ruido(x * 4 + 3, y * 4 + 9) > 0.50f) return v->cor2;
        return cor;
    }
    case AD_BARRO: {                               // o barro molhado: os calombos e o brilho da agua
        float n = ruido(x * 3.2f + 5, y * 3.2f + 2);
        if (n > 0.42f) return v->cor2;
        if (n < -0.55f && e > 1.5f) { an_final = RGB(190, 156, 86); return cor; }
        return cor;
    }
    case AD_SUJEIRA: {                             // a lama seca: as rachaduras e a mosca andando em cima
        float fa = t_relogio * 2.3f + fase * 6.3f, mx = 0.40f * cosf(fa), my = -0.12f + 0.28f * sinf(fa * 2);
        float dxm = (x - mx) / s, dym = (y - my) / s;
        if (fabsf(dxm) < 1.0f && fabsf(dym) < 0.6f) { an_final = C_BARRA; return cor; }
        if (fabsf(dxm) < 1.6f && dym > -1.8f && dym < -0.6f && ((int)(t_relogio * 20) & 1)) { an_final = C_MARFIM_S; return cor; }
        if (fabsf(ruido(x * 3.0f + 1, y * 3.0f + 7)) < 0.05f) return RGB(56, 32, 20);
        return ruido(x * 5 + 2, y * 5 + 4) > 0.40f ? RGB(156, 106, 64) : cor;
    }
    case AD_LOBO: {                                // o rasgo: o fundo escuro, a beira de cima clara (o dado lascado), a de baixo na sombra
        if (e < 1.3f && an_dd_gx + an_dd_gy < -0.4f) { an_final = C_BRANCO; return cor; }
        if (e < 1.0f && an_dd_gx + an_dd_gy > 0.4f) { an_final = gfx_mistura(an_le(X, Y), C_SOMBRA, 50); return cor; }
        an_final = e > 2.0f ? RGB(36, 32, 29) : v->cor;
        return cor;
    }
    case AD_BOMBA: {                               // o canudo vermelho: as faixas amarelas, o papel enrugado; o pavio cinza
        if (parte == 2) return y < -0.50f && an_fx.p2 > 0 ? (((int)(t_relogio * 24) & 1) ? C_AMARELO : C_FOGO) : RGB(128, 120, 112);
        if (fabsf(fabsf(x) - 0.48f) < 0.09f) return v->cor2;
        if (fabsf(x) > 0.66f) return RGB(238, 230, 212);              // as pontas de papel dobrado
        if (fr(x * 5.5f + 0.3f * y) < 0.12f) return RGB(118, 30, 38); // as rugas do papel
        return cor;
    }
    case AD_DOURADO: {                             // a folha de ouro: fina, a beira escura do lado da sombra, as rachas e as rugas acesas
        if (e < 1.3f && an_dd_gx + an_dd_gy > 0.3f) { an_final = RGB(150, 122, 62); return cor; }
        if (e < 1.2f && an_dd_gx + an_dd_gy < -0.5f) { an_final = RGB(246, 222, 132); return cor; }
        float n = ruido(x * 6 + 4, y * 6 + 8);
        if (fabsf(n) < 0.06f) { an_final = RGB(106, 86, 44); return cor; }   // uma racha
        if (n > 0.50f) { an_mat_px = MN_OURO; return RGB(246, 222, 132); }   // uma ruga acesa
        if (n < -0.45f) return RGB(190, 156, 86);
        return cor;
    }
    case AD_NEON: {                                // o tubo aceso na plaquinha escura; acendendo (p2), pisca antes de firmar
        if (an_fx.p2 > 0) {
            float q = an_fx.p2;
            bool liga = (q > 0.18f && q < 0.26f) || (q > 0.40f && q < 0.46f) || q > 0.62f;
            if (e > 1.6f && e < 3.0f) return liga ? C_ROSA : C_VINHO;
            if (!liga && e >= 3.0f && e < 4.2f) return cor;
        }
        return an_textura(a, parte, x, y, e, s, X, Y, fase, mov, cor);
    }
    case AD_REMENDO: {                             // o pano: a trama, a beira desfiando e o pesponto, que a agulha faz aos poucos (p2)
        if (e < 1.2f && (hash2((int)floorf(x / s) + 40, (int)floorf(y / s) + 70) % 5) == 0) { an_pula = true; return 0; }
        if (e > 1.5f && e < 2.6f && ((X + Y) % 4) < 2) {
            float an = fr((atan2f(y, x) + 3.1416f) / 6.2832f + 0.25f);
            if (an_fx.p2 <= 0 || an < an_fx.p2) return C_MARFIM;
        }
        return cor;
    }
    case AD_FRAGIL: {                              // a fita laranja: a taca quebrada impressa de tanto em tanto e os fios da beira
        if (fabsf(fabsf(y) - 0.22f) < s * 0.55f) return v->cor2;
        float cx = fr((x + 1.2f) / 0.62f) - 0.5f, cy = y;           // a taca: o bojo, a haste e o pe
        if ((cx * cx / 0.020f + (cy + 0.06f) * (cy + 0.06f) / 0.012f < 1 && cy < -0.02f) || (fabsf(cx) < 0.03f && cy > -0.04f && cy < 0.10f)
            || (fabsf(cx) < 0.10f && fabsf(cy - 0.12f) < 0.03f)) return v->cor2;
        return cor;
    }
    case AD_DUPLA: {                               // o filme vermelho por cima da espuma; a beira dele, clara, levantando
        if (parte == 1) return an_vinco_px > -1.3f ? RGB(226, 164, 168) : v->cor2;
        if (an_vinco_px < 1.2f) { an_final = gfx_mistura(an_le(X, Y), C_SOMBRA, 30); an_final = RGB(204, 194, 172); return cor; }
        return ((X + Y) % 3) ? cor : RGB(204, 194, 172);              // a espuma, com os furinhos
    }
    case AD_ELETRONICO: {                          // o chip preto: as letras gravadas, a marca do pino 1 e o LED piscando
        if (parte == 1) return 0;
        if (fabsf(x + 0.52f) < 0.12f && fabsf(y + 0.58f) < 0.12f) return RGB(128, 120, 112);   // a marca do pino 1
        if (fabsf(x - 0.46f) < 0.13f && fabsf(y + 0.56f) < 0.13f) {   // o LED
            bool aceso = an_fx.p2 > 0 || fr(t_relogio * 0.8f + fase) < 0.5f;
            an_final = aceso ? RGB(130, 204, 112) : RGB(24, 62, 46);
            return cor;
        }
        if (fabsf(x) < 0.52f && (fabsf(y - 0.12f) < s * 0.6f || fabsf(y - 0.40f) < s * 0.6f) && ((int)floorf(x / s) % 4) != 3) return RGB(96, 88, 80);
        return cor;
    }
    case AD_REFLEXIVO: {                           // o espelho: a moldura fina de metal em volta
        if (e < 1.6f) { an_mat_px = MN_METAL; return v->cor2; }
        return an_textura(a, parte, x, y, e, s, X, Y, fase, mov, cor);
    }
    case AD_CHUMBO: {                              // o peso de chumbo, com dois rebites prendendo no dado
        for (int i = -1; i <= 1; i += 2) {
            float rx = x - i * 0.46f, ry = y - 0.58f;
            if (rx * rx + ry * ry < 0.016f) { an_mat_px = MN_METAL; return rx + ry < 0 ? C_PRATA : RGB(104, 112, 124); }
        }
        return an_textura(a, parte, x, y, e, s, X, Y, fase, mov, cor);
    }
    case AD_POSTIT: {                              // o bilhetinho: o rabisco a lapis e a faixa de cola, um nada mais escura
        if (parte == 1) return an_textura(a, parte, x, y, e, s, X, Y, fase, mov, cor);
        if (y < -0.62f) return RGB(228, 194, 106);
        float rx = x + 0.10f, ry = y + 0.28f + 0.07f * sinf(x * 14);
        if (fabsf(ry) < s * 0.55f && rx > -0.50f && rx < 0.42f) return RGB(128, 120, 112);
        return cor;
    }
    case AD_CONFETE: {                             // cada pedacinho da sua cor
        static const uint16_t P[6] = { RGB(190, 56, 60), RGB(228, 194, 106), RGB(96, 134, 196), RGB(130, 204, 112),
                                       RGB(208, 98, 108), RGB(150, 112, 160) };
        return P[(parte * 5) % 6];
    }
    case AD_PURPURINA: {                           // a cola coberta de purpurina; colando (p2), ainda branca, a purpurina caindo
        if (parte == 1) { an_final = (hash2(X, Y) & 1) ? C_BRANCO : C_AMARELO; return cor; }
        if (an_fx.p2 > 0 && (hash2((int)floorf(x / s) + 3, (int)floorf(y / s) + 8) % 100) < an_fx.p2 * 100) { an_mat_px = MN_BRILHO; return C_MARFIM; }
        return cor;
    }
    case AD_ESTRELINHA: {                          // a estrelinha dourada da professora: cada ponta com uma face clara e uma escura
        float an = atan2f(y + 0.06f, x) + 1.5708f, k = fr(an / 1.25664f + 0.5f);
        float ro = sqrtf(x * x + (y + 0.06f) * (y + 0.06f));
        if (ro < 0.10f) return RGB(246, 222, 132);
        if (fabsf(k - 0.5f) < 0.035f / fmaxf(ro, 0.2f)) return v->cor2;   // a aresta do meio de cada ponta
        return k < 0.5f ? RGB(246, 222, 132) : RGB(190, 156, 86);
    }
    case AD_HOLOGRAFICO: {                         // o holograma: a estrela que aparece e some conforme o dado mexe
        float vis = sinf(t_relogio * 1.3f + mov * 2 + fase * 6);
        if (vis > 0.25f && fabsf(sdn_estrela(x / 0.52f, y / 0.52f, 1.0f, 0.48f) * 0.52f) < s * 0.8f) { an_final = C_BRANCO; return cor; }
        return cor;
    }
    case AD_FIGURINHA: {                           // a figurinha: a moldura, o retrato em silhueta e a estrela do album
        if (e > 1.4f && e < 2.4f) return C_BRANCO;
        if (e >= 2.4f && e < 3.2f) return v->cor2;
        if (sdn_estrela((x + 0.36f) / 0.22f, (y + 0.64f) / 0.22f, 1.0f, 0.5f) < 0) { an_mat_px = MN_OURO; return C_OURO; }
        float hx = x - 0.02f, hy = y + 0.12f;
        if (hx * hx + hy * hy < 0.075f || (fabsf(x) < 0.46f && y > 0.24f && y < 0.66f && fabsf(x) < 0.46f - (0.66f - y) * 0.2f)) return v->cor2;
        return cor;
    }
    case AD_BILHETE: {                             // o bilhete de loteria: a faixa vermelha, os numeros, o picote e o selo dourado
        if (y < -0.36f) return ((int)floorf(x / s) % 3 == 0 && fabsf(y + 0.48f) < s) ? C_BRANCO : v->cor2;
        if (fabsf(x - 0.56f) < s * 0.6f && ((int)floorf(y / s) & 1)) return RGB(166, 154, 132);   // o picote
        if (x < 0.46f && (an2_miudo("1735", x, y, -0.18f, -0.14f, s) || an2_miudo("4062", x, y, -0.18f, 0.24f, s))) return RGB(36, 32, 29);
        float sx = x - 0.76f, sy = y - 0.30f;
        if (sx * sx + sy * sy < 0.026f) { an_mat_px = MN_OURO; return sx + sy < 0 ? C_AMARELO : C_OURO; }
        return cor;
    }
    case AD_LENTICULAR: {                          // o cromo 3D: de um lado o x2, do outro o -4, trocando com o angulo
        bool frente = sinf(mov * 3 + t_relogio * 0.9f + fase * 6) > 0;
        if (fr((x + 3) / (s * 2)) < 0.5f) an_mat_px = MN_VINIL;           // as lentes em fileira
        if (an2_miudo(frente ? "x2" : "-4", x, y, 0, -0.38f, s)) return frente ? C_AMARELO : C_ROSA;
        return frente ? cor : v->cor2;
    }
    case AD_BIS: {                                 // o ingresso do bis: o sinal de repetir da musica dos dois lados do numero
        for (int lado = -1; lado <= 1; lado += 2) {
            float lx = x * lado;                   // o espelho: |: de um lado e :| do outro
            if (lx < -0.58f && lx > -0.70f && fabsf(y) < 0.44f) return v->cor2;              // a barra grossa
            if (lx < -0.48f && lx > -0.53f && fabsf(y) < 0.44f) return v->cor2;              // a fina
            float dx = lx + 0.36f;
            if (dx * dx + (y - 0.16f) * (y - 0.16f) < 0.007f || dx * dx + (y + 0.16f) * (y + 0.16f) < 0.007f) return v->cor2;   // os dois pontos
        }
        return cor;
    }
    case AD_AVESSO: {                              // colado do avesso: o verso de papel com a grade; na ponta dobrada, a frente colorida
        if (parte == 1) return ((X + Y) % 6) < 2 ? RGB(96, 182, 198) : v->cor2;
        if (fr(x / 0.26f) < s / 0.26f * 0.9f || fr(y / 0.26f) < s / 0.26f * 0.9f) return RGB(204, 194, 172);
        if (an2_miudo("2", -x, y, 0.30f, -0.40f, s)) return RGB(166, 154, 132);   // a impressao, de tras para a frente
        return cor;
    }
    case AD_ETIQUETA: {                            // a etiqueta da maquininha: o preco em vermelho e o codigo de barras
        if (an2_miudo("R$9,99", x, y, 0, -0.26f, s)) return v->cor2;
        if (y > 0.10f && y < 0.40f && fabsf(x) < 0.66f && (hash2((int)floorf(x / s) + 77, 3) % 3) != 0) return RGB(36, 32, 29);
        return cor;
    }
    case AD_CHICLETE: {                            // a goma mascada: as marcas dos dentes
        if (ruido(x * 5 + 3, y * 5 + 7) > 0.52f) return v->cor2;
        return cor;
    }
    default: return an_textura(a, parte, x, y, e, s, X, Y, fase, mov, cor);   // a textura de hoje
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

// A luva de desenho animado, o dedao para baixo (aperta a massinha, o
// post-it, bate a figurinha). (x, y): a ponta do dedao.
static void fer_luva(int x, int y)
{
    static const char *const P[12] = {
        "..####....", ".#bbbb#...", "#bbbbbb#..", "#bbbbbbm#.", "#bbbbbbm#.", ".#bbbbm#..",
        ".##bbm##..", "#bm##m#...", "#bbm#....", "#bbm#....", ".#mm#....", "..##.....",
    };
    for (int j = 0; j < 12; j++)
        for (int i = 0; P[j][i]; i++) {
            char c = P[j][i];
            uint16_t k = c == '#' ? C_BARRA : c == 'b' ? C_BRANCO : c == 'm' ? C_MARFIM_S : 0;
            if (k) gfx_pixel(x - 2 + i, y - 11 + j, k);
        }
}

// Uma bolota de massa (a cor c e a sombra s) de raio r, com o brilho em cima.
static void fer_bolota(int x, int y, int r, uint16_t c, uint16_t sm, bool brilho)
{
    gfx_disco(x + 1, y + 2, r, C_SOMBRA);
    gfx_disco(x, y, r + 1, C_BARRA);
    gfx_disco(x, y, r, sm);
    gfx_disco(x - 1, y - 1, r > 1 ? r - 1 : 1, c);
    if (brilho) { gfx_pixel(x - r / 2, y - r / 2, C_BRANCO); if (r > 4) gfx_pixel(x - r / 2 + 1, y - r / 2, C_BRANCO); }
}

// O martelinho: a cabeca de ferro e o cabo de madeira. (x, y): a ponta que bate.
static void fer_martelo(int x, int y, float ang)
{
    float c = cosf(ang), s = sinf(ang);
    for (int i = 2; i < 13; i++) {                 // o cabo, para cima e para a direita
        int px = x + (int)lroundf(c * i + 4), py = y - (int)lroundf(s * i + 2);
        gfx_rect(px - 1, py - 1, 3, 3, C_BARRA);
    }
    for (int i = 2; i < 13; i++) {
        int px = x + (int)lroundf(c * i + 4), py = y - (int)lroundf(s * i + 2);
        gfx_pixel(px, py, C_MADEIRA_CLR);
    }
    gfx_rect(x - 3, y - 4, 8, 5, C_BARRA);          // a cabeca
    gfx_rect(x - 2, y - 3, 6, 3, RGB(104, 112, 124));
    gfx_rect(x - 2, y - 3, 6, 1, C_PRATA);
}

// A agulha com a linha: (x, y) a ponta; a linha sai do olho e vai ate (lx, ly).
static void fer_agulha(int x, int y, int lx, int ly)
{
    gfx_linha(x + 6, y - 6, lx, ly, C_MARFIM);
    gfx_linha(x, y, x + 6, y - 6, C_BARRA);
    gfx_linha(x + 1, y, x + 7, y - 6, C_BARRA);
    gfx_linha(x, y - 1, x + 6, y - 7, C_PRATA);
    gfx_pixel(x, y, C_BRANCO);
}

// O rolo de fita: o anel de papelao e a fita enrolada. (x, y): o meio.
static void fer_rolo(int x, int y, uint16_t fita)
{
    gfx_disco(x + 1, y + 1, 7, C_SOMBRA);
    gfx_disco(x, y, 7, C_BARRA);
    gfx_disco(x, y, 6, fita);
    gfx_disco(x - 1, y - 1, 4, tom_rampa(an_rampa_de(fita), 0.7f, x, y));
    gfx_disco(x, y, 3, C_BARRA);
    gfx_disco(x, y, 2, C_TERRA);
    gfx_disco(x, y, 1, C_BARRA);
}

// O lapis: o corpo amarelo, a borracha e a ponta de grafite. (x, y): a ponta.
static void fer_lapis(int x, int y)
{
    for (int i = 0; i < 16; i++) {
        int px = x + 1 + (int)(i * 0.72f), py = y - 1 - (int)(i * 0.72f);
        gfx_rect(px - 2, py - 2, 4, 4, C_BARRA);
    }
    for (int i = 0; i < 16; i++) {
        int px = x + 1 + (int)(i * 0.72f), py = y - 1 - (int)(i * 0.72f);
        uint16_t c = i < 2 ? C_EBANO_B : i < 4 ? RGB(204, 194, 172) : i > 13 ? C_ROSA : i > 12 ? C_PRATA : C_AMARELO;
        gfx_rect(px - 1, py - 1, 2, 2, c);
        if (i >= 4 && i <= 12) gfx_pixel(px - 1, py - 1, C_OURO);
    }
    gfx_pixel(x, y, C_EBANO);
}

// A poeira (ou a agua) subindo da base de quem caiu: q 0..1.
static void cn2_poeira(int x, int y, float R, float q, uint16_t c1, uint16_t c2)
{
    for (int i = 0; i < 10; i++) {
        float an = 3.1416f + (i - 4.5f) * 0.36f, v = R + 2 + q * (8 + (i * 7) % 5);
        int px = x + (int)(cosf(an) * v * 1.2f), py = y + (int)(R * 0.6f) - (int)(fabsf(sinf(an)) * v * 0.35f) - (int)(q * 4);
        if (bayer(px, py) < 1.1f - q) gfx_rect(px, py, 2, 2, i & 1 ? c1 : c2);
    }
}

// O adesivo solto, de raio R e girado th (o recorte da proposta, sem o dado).
static void cn2_solto(float cx, float cy, int a, float R, float th)
{
    an_op_t o = { 0 };
    o.R = R; o.th = th;
    o.marg = R >= 20 ? 2 : 1;
    o.sx = 1; o.sy = 2;
    o.escuro = C_BARRA; o.sombra = C_SOMBRA;
    o.brilho = o.onda = -1;
    o.fase = 0.3f;
    o.liso = R < 7;
    an_desenha(cx, cy, a, &o);
}

// O lanca-confete: o cone listrado de boca para cima e o cordao. (x, y): a boca.
static void fer_canhao(int x, int y)
{
    for (int j = 0; j < 12; j++) {
        int w = 4 - j / 3;
        gfx_rect(x - w - 1, y + j, 2 * w + 3, 1, C_BARRA);
        for (int i = -w; i <= w; i++) gfx_pixel(x + i, y + j, ((i + j) & 3) < 2 ? C_VINHO_CLR : C_AMARELO);
    }
    gfx_rect(x - 5, y - 1, 11, 2, C_BARRA);
    gfx_rect(x - 4, y - 1, 9, 1, C_OURO);
    gfx_linha(x, y + 12, x + 3, y + 16, C_MARFIM);          // o cordao
}

// O frasco de cola branca, de bico para baixo. (x, y): a ponta do bico.
static void fer_cola(int x, int y)
{
    gfx_rect(x - 1, y - 4, 3, 5, C_BARRA); gfx_pixel(x, y - 3, C_FOGO); gfx_pixel(x, y - 2, C_FOGO);
    gfx_rect(x - 4, y - 16, 9, 13, C_BARRA);
    gfx_rect(x - 3, y - 15, 7, 11, C_BRANCO);
    gfx_rect(x + 2, y - 15, 1, 11, C_MARFIM_S);
    gfx_rect(x - 3, y - 11, 7, 3, RGB(238, 162, 74));       // o rotulo
}

// O potinho de purpurina, de boca para baixo, com os furinhos. (x, y): a tampa.
static void fer_saleiro(int x, int y)
{
    gfx_rect(x - 5, y - 12, 11, 13, C_BARRA);
    gfx_rect(x - 4, y - 11, 9, 9, RGB(150, 62, 80));
    gfx_rect(x - 4, y - 11, 2, 9, RGB(208, 98, 108));
    gfx_rect(x - 4, y - 2, 9, 3, C_PRATA);                   // a tampa de metal com os furos
    gfx_pixel(x - 2, y, C_BARRA); gfx_pixel(x, y, C_BARRA); gfx_pixel(x + 2, y, C_BARRA);
}

// A lata de spray: o corpo cinza, a tampa vermelha e o bico. (x, y): o bico.
static void fer_spray(int x, int y)
{
    gfx_rect(x - 4, y - 3, 9, 19, C_BARRA);
    gfx_rect(x - 3, y + 3, 7, 12, RGB(140, 150, 164));
    gfx_rect(x - 3, y + 3, 2, 12, C_PRATA);
    gfx_rect(x - 3, y + 7, 7, 3, RGB(246, 222, 132));       // o rotulo amarelo
    gfx_rect(x - 2, y - 2, 5, 5, C_VERMELHO);                // a tampa
    gfx_rect(x - 3, y - 1, 2, 2, C_BARRA);                   // o bico, olhando para a esquerda
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

// Os objetos chegam de um jeito so deles: caem (o Chumbo pesado, a Casca
// mole), giram no ar (a ficha), sao puxados de lado (o ima), deslizam (o
// vidro), descem flutuando (a folha de ouro), sao desenrolados (a fita) ou
// rasgados (a garra).
enum { CJ_NADA, CJ_CAI, CJ_PESADO, CJ_MOLE, CJ_GIRA, CJ_PUXA, CJ_DESLIZA, CJ_FLUTUA, CJ_ROLO, CJ_PATA, CJ_AGUA };
static int cn2_jeito(int a)
{
    switch (a) {
    case AD_CHUMBO: return CJ_PESADO;
    case AD_CASCA: return CJ_MOLE;
    case AD_GORJETA: return CJ_GIRA;
    case AD_IMANTADO: return CJ_PUXA;
    case AD_TRANSPARENTE: return CJ_DESLIZA;
    case AD_DOURADO: return CJ_FLUTUA;
    case AD_FRAGIL: return CJ_ROLO;
    case AD_LOBO: return CJ_PATA;
    case AD_PATINHO: return CJ_AGUA;
    case AD_REFLEXIVO: case AD_ELETRONICO: case AD_REMENDO: case AD_RATOEIRA: case AD_MIRA: case AD_NEON:
    case AD_VELCRO: case AD_BOMBA: case AD_POSTIT: case AD_DUPLA: case AD_CARBONO: return CJ_CAI;
    default: return CJ_NADA;
    }
}
static float cn2_toca(int a)
{
    switch (a) {
    case AD_CORRETIVO: return 0.42f;
    case AD_CARIMBO: return 0.46f;
    case AD_SELO: return 0.34f;
    case AD_NOTA_DEZ: return 0.30f;
    case AD_CHICLETE: return 0.62f;
    case AD_MASSINHA: return 0.34f;
    case AD_BARRO: case AD_SUJEIRA: return 0.30f;
    case AD_INFECTADO: return 0.36f;
    case AD_FERMENTO: return 0.30f;
    case AD_CONFETE: return 0.45f;
    case AD_PURPURINA: return 0.22f;
    case AD_IMPERMEAVEL: return 0.20f;
    default: break;
    }
    switch (cn2_jeito(a)) {
    case CJ_PESADO: return 0.24f;
    case CJ_MOLE: return 0.36f;
    case CJ_GIRA: return 0.52f;
    case CJ_PUXA: return 0.30f;
    case CJ_DESLIZA: return 0.08f;
    case CJ_FLUTUA: return 0.72f;
    case CJ_ROLO: return 0.12f;
    case CJ_PATA: return 0.18f;
    case CJ_AGUA: return 0.32f;
    case CJ_CAI: return 0.30f;
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
    case AD_CHICLETE: return 1.45f;
    case AD_MASSINHA: return 1.30f;
    case AD_BARRO: case AD_SUJEIRA: return 1.10f;
    case AD_INFECTADO: return 1.45f;
    case AD_FERMENTO: return 1.60f;
    case AD_CHUMBO: return 1.30f;
    case AD_ELETRONICO: return 1.50f;
    case AD_REMENDO: return 1.65f;
    case AD_NEON: return 1.40f;
    case AD_BOMBA: return 1.25f;
    case AD_DOURADO: return 1.45f;
    case AD_CARBONO: return 1.25f;
    case AD_GORJETA: return 1.20f;
    case AD_CONFETE: return 1.35f;
    case AD_PURPURINA: return 1.60f;
    case AD_IMPERMEAVEL: return 1.35f;
    default: return 1.05f;
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
    case AD_CHICLETE: if (cn2_passou(t0, t1, 0.02f)) som_toca(SOM_AD_CHICLETE); break;
    case AD_MASSINHA: if (cn2_passou(t0, t1, 0.30f)) som_toca(SOM_AD_MASSINHA); break;
    case AD_BARRO: case AD_SUJEIRA: if (cn2_passou(t0, t1, 0.10f)) som_toca(SOM_AD_BARRO); break;
    case AD_INFECTADO: if (cn2_passou(t0, t1, 0.30f)) som_toca(SOM_AD_INFECTADO); break;
    case AD_FERMENTO: if (cn2_passou(t0, t1, 0.28f)) som_toca(SOM_AD_FERMENTO); break;
    case AD_CHUMBO:
        if (cn2_passou(t0, t1, 0.24f)) som_toca(SOM_AD_CHUMBO);
        if (cn2_passou(t0, t1, 0.62f) || cn2_passou(t0, t1, 0.86f)) som_toca(SOM_TIQUE);   // as marteladas
        break;
    case AD_ELETRONICO:
        for (int i = 0; i < 4; i++) if (cn2_passou(t0, t1, 0.48f + i * 0.17f)) som_toca(SOM_TIQUE);   // cada perninha soldada
        if (cn2_passou(t0, t1, 1.18f)) som_toca(SOM_AD_ELETRONICO);
        break;
    case AD_REMENDO: if (cn2_passou(t0, t1, 0.36f)) som_toca(SOM_AD_REMENDO); break;
    case AD_NEON: if (cn2_passou(t0, t1, 0.36f)) som_toca(SOM_AD_NEON); break;
    case AD_BOMBA: if (cn2_passou(t0, t1, 0.40f)) som_toca(SOM_AD_BOMBA); break;
    case AD_GORJETA: if (cn2_passou(t0, t1, 0.02f)) som_toca(SOM_AD_GORJETA); break;
    case AD_IMANTADO: if (cn2_passou(t0, t1, 0.28f)) som_toca(SOM_AD_IMANTADO); break;
    case AD_VELCRO: if (cn2_passou(t0, t1, 0.34f)) som_toca(SOM_AD_VELCRO); break;
    case AD_RATOEIRA: if (cn2_passou(t0, t1, 0.62f)) som_toca(SOM_AD_RATOEIRA); break;
    case AD_PATINHO: if (cn2_passou(t0, t1, 0.30f)) som_toca(SOM_AD_PATINHO); break;
    case AD_CASCA: if (cn2_passou(t0, t1, 0.34f)) som_toca(SOM_AD_CASCA); break;
    case AD_LOBO: if (cn2_passou(t0, t1, 0.10f)) som_toca(SOM_AD_LOBO); break;
    case AD_DOURADO: if (cn2_passou(t0, t1, 0.70f)) som_toca(SOM_AD_DOURADO); break;
    case AD_FRAGIL: if (cn2_passou(t0, t1, 0.10f)) som_toca(SOM_AD_FRAGIL); break;
    case AD_DUPLA: if (cn2_passou(t0, t1, 0.10f)) som_toca(SOM_AD_DUPLA); break;
    case AD_POSTIT: if (cn2_passou(t0, t1, 0.28f)) som_toca(SOM_AD_POSTIT); break;
    case AD_CARBONO: if (cn2_passou(t0, t1, 0.46f)) som_toca(SOM_AD_CARBONO); break;
    case AD_REFLEXIVO: case AD_MIRA: case AD_TRANSPARENTE:
        if (cn2_passou(t0, t1, cn2_toca(a) - 0.02f)) som_toca(som_adesivo(a));
        break;
    case AD_CONFETE: if (cn2_passou(t0, t1, 0.24f)) som_toca(SOM_AD_CONFETE); break;
    case AD_PURPURINA: if (cn2_passou(t0, t1, 0.70f)) som_toca(SOM_AD_PURPURINA); break;
    case AD_IMPERMEAVEL: if (cn2_passou(t0, t1, 0.16f)) som_toca(SOM_AD_IMPERMEAVEL); break;
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
    case AD_CHICLETE: {                            // a goma paira; colando, a bola enche, treme e estoura
        if (tc <= 0) { fer_bolota(x + 2, (int)(y - 26 + bob), 4, C_ROSA, C_VINHO_CLR, true); return; }
        int bx = x + 2, by = y - 14;
        if (tc < 0.55f) {
            float q = tc / 0.55f;
            int rr = (int)(3 + q * q * 11) + (q > 0.85f ? (int)(t_fase * 40) % 2 : 0);
            gfx_disco(bx + 1, by - rr + 3, rr, C_SOMBRA);
            gfx_disco(bx, by - rr + 2, rr + 1, C_VINHO_CLR);
            gfx_disco(bx, by - rr + 2, rr, C_ROSA);
            gfx_rect(bx - rr / 2, by - rr + 2 - rr / 2, 2, 2, C_BRANCO);
            fer_bolota(bx, by + 2, 3, C_ROSA, C_VINHO_CLR, false);   // a goma de onde ela sai
        } else {                                   // PLOC: os pedacos voam
            float q = (tc - 0.55f) / 0.12f;
            for (int i = 0; i < 12; i++) {
                float an = i * 0.5236f + 0.2f, vv = 6 + (i * 29) % 7 + q * 12;
                gfx_rect(bx + (int)(cosf(an) * vv), by - 10 + (int)(sinf(an) * vv * 0.8f + q * q * 8), 2, 2, i % 3 ? C_ROSA : C_VINHO_CLR);
            }
            float k = (tc - 0.55f) / (cn2_toca(a) - 0.55f);   // e a goma desce
            fer_bolota(bx, by + 2 + (int)(k * 12), 3, C_ROSA, C_VINHO_CLR, false);
        }
        return;
    }
    case AD_MASSINHA: case AD_INFECTADO: case AD_FERMENTO: {   // a bolota paira; colando, cai no lado
        uint16_t c = an_cor(a), sm = an_rampa_de(c)[1];
        int r = a == AD_FERMENTO ? 6 : 5;
        float hy = y - 26 + bob;
        if (tc > 0) { float q = tc / cn2_toca(a); hy = (y - 26) + 24 * q * q; }
        fer_bolota(x, (int)hy, r, c, sm, a != AD_MASSINHA);
        if (a == AD_INFECTADO)                     // o germe: os cilios mexendo e os olhinhos
            for (int i = 0; i < 6; i++) {
                float an = i * 1.047f + sinf(t_fase * 9 + i) * 0.2f;
                gfx_pixel(x + (int)(cosf(an) * (r + 2)), (int)hy + (int)(sinf(an) * (r + 2)), sm);
            }
        if (a == AD_INFECTADO) { gfx_pixel(x - 2, (int)hy - 1, C_BRANCO); gfx_pixel(x + 1, (int)hy - 1, C_BRANCO); gfx_pixel(x - 2, (int)hy, C_BARRA); gfx_pixel(x + 1, (int)hy, C_BARRA); }
        return;
    }
    case AD_CONFETE: {                             // o lanca-confete paira; colando, treme e estoura
        float sac = tc > 0 && tc < 0.24f ? sinf(tc * 80) * 1.5f : 0;
        if (tc < 0.30f) fer_canhao(x + 14 + (int)sac, (int)(y - 30 + (tc <= 0 ? bob : 0)));
        return;
    }
    case AD_PURPURINA: {                           // o frasco de cola paira; colando, desce ate o lado
        float q = tc > 0 ? fminf(1, tc / cn2_toca(a)) : 0;
        fer_cola(x + 2, (int)(y - 22 + bob * (1 - q) + q * 16));
        return;
    }
    case AD_IMPERMEAVEL: {                         // a lata de spray paira; colando, chacoalha
        float sac = tc > 0 ? sinf(tc * 70) * 2 : 0;
        fer_spray(x + 20, (int)(y - 22 + (tc <= 0 ? bob : 0) + sac));
        return;
    }
    case AD_BARRO: case AD_SUJEIRA: {              // a bola de barro: paira girando; colando, e atirada num arco
        float gi = t_fase * 10;
        int bx = x, by = (int)(y - 26 + bob);
        if (tc > 0) {
            float q = tc / cn2_toca(a);
            bx = x - 44 + (int)(44 * q);
            by = (y - 30) + (int)(30 * q) - (int)(sinf(q * 3.1416f) * 14);
        }
        fer_bolota(bx, by, 5, RGB(156, 106, 64), RGB(120, 78, 50), false);
        for (int c = 0; c < 3; c++)                // os calombos dando a volta
            gfx_pixel(bx + (int)(cosf(gi + c * 2.1f) * 3), by + (int)(sinf(gi + c * 2.1f) * 3), RGB(78, 46, 30));
        gfx_pixel(bx - 2, by - 2, RGB(190, 156, 86));
        return;
    }
    default: break;
    }
    int j = cn2_jeito(a);
    if (j == CJ_NADA) return;
    float R0 = R * 0.92f;
    if (j == CJ_ROLO) {                            // o rolo de fita paira; colando, encosta na beira esquerda
        float q = tc > 0 ? fminf(1, tc / cn2_toca(a)) : 0;
        fer_rolo(x - (int)(R * 0.95f * q), (int)(y - 24 * (1 - q) + bob * (1 - q)), an_cor(a));
        return;
    }
    if (j == CJ_PATA) {                            // a garra: tres riscos de vento chegando de cima, a direita
        if (tc <= 0) { for (int i = 0; i < 3; i++) gfx_rect(x + 10 + i * 4, (int)(y - 28 + bob) + i, 2, 5, RGB(190, 56, 60)); return; }
        float q = tc / cn2_toca(a);
        for (int i = 0; i < 3; i++)
            gfx_linha(x + 18 + i * 5 - (int)(q * 10), y - 30 + (int)(q * 16), x + 22 + i * 5 - (int)(q * 10), y - 36 + (int)(q * 16), C_BRANCO);
        return;
    }
    if (j == CJ_DESLIZA) {                         // o vidro: paira; colando, encosta pela esquerda
        if (tc <= 0) cn2_solto((float)x, y - 26 + bob, a, R0, th);
        return;
    }
    if (tc <= 0) {                                 // pairando sobre o lado
        if (j == CJ_GIRA) { float w = fabsf(cosf(t_fase * 2.5f)); gfx_rect(x - (int)(w * 7) - 1, (int)(y - 27 + bob), (int)(w * 14) + 3, 5, C_BARRA); gfx_rect(x - (int)(w * 7), (int)(y - 26 + bob), (int)(w * 14) + 1, 3, an_cor(a)); return; }
        cn2_solto((float)x, y - 26 + bob, a, R0, th + (j == CJ_FLUTUA ? sinf(t_fase * 2) * 0.3f : 0));
        return;
    }
    float q = fminf(1, tc / cn2_toca(a));
    switch (j) {
    case CJ_PESADO: case CJ_CAI: case CJ_MOLE: case CJ_AGUA: {   // cai: o pesado, de mais alto e mais depressa
        float h = j == CJ_PESADO ? 40 : 26, yy = (y - h) + h * q * q;
        cn2_solto((float)x, yy, a, R0 + (R - R0) * q, th);
        break;
    }
    case CJ_GIRA: {                                // a ficha: sobe girando (cara, coroa) e cai
        float yy = y - 26 - sinf(q * 3.1416f) * 14 + 26 * q * q, w = fabsf(cosf(q * 3.1416f * 5));
        int hw = (int)(w * 7), yi = (int)yy;
        gfx_rect(x - hw - 1, yi - 1, 2 * hw + 3, 5, C_BARRA);
        gfx_rect(x - hw, yi, 2 * hw + 1, 3, ((int)(q * 10) & 1) ? an_cor(a) : RGB(238, 230, 212));
        break;
    }
    case CJ_PUXA: {                                // o ima: puxado de longe, pela direita, cada vez mais depressa
        cn2_solto(x + 50 * (1 - q * q), (float)y - 2 * (1 - q), a, R, th);
        for (int i = 0; i < 3 && q < 0.9f; i++)   // os riscos de velocidade
            gfx_rect(x + (int)(50 * (1 - q * q)) + (int)R + 4 + i * 4, y - 4 + i * 4, 4 + i * 2, 1, C_MARFIM_S);
        break;
    }
    case CJ_FLUTUA: {                              // a folha de ouro: desce balancando, como pena
        float yy = (y - 26) + 26 * q, sx = sinf(q * 3.1416f * 2.5f) * 9 * (1 - q);
        cn2_solto(x + sx, yy, a, R0 + (R - R0) * q, th + sinf(q * 3.1416f * 2.5f) * 0.45f * (1 - q));
        break;
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
    case AD_CHICLETE: {                            // a goma esmagada abre do meio e os fios esticam; balanca e assenta
        float d = tc - t0, q = d / 0.22f;
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        an_fx.escala = 1 + 0.14f * sinf(fminf(1, d / 0.6f) * 3.1416f * 3) * (1 - fminf(1, d / 0.6f));
        break;
    }
    case AD_MASSINHA: {                            // achata ao cair; o dedao aperta e deixa a digital
        float d = tc - t0;
        an_fx.escala = d < 0.12f ? 1.16f - 0.16f * d / 0.12f : d > 0.30f && d < 0.46f ? 1.06f : 1;
        an_fx.p2 = d < 0.42f ? 1 : 0;
        break;
    }
    case AD_BARRO: case AD_SUJEIRA: {              // SPLAT: abre de uma vez, do meio para fora
        float d = tc - t0, q = d / 0.08f;
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        an_fx.escala = d < 0.14f ? 1.12f - 0.12f * d / 0.14f : 1;
        break;
    }
    case AD_INFECTADO: {                           // a gosma assenta e as veias correm pela face
        float d = tc - t0, q = d / 0.85f;
        an_fx.revela = q < 1 ? 0.45f * fminf(1, d / 0.12f) + 0.55f * fmaxf(0, (q * 0.85f - 0.12f) / 0.73f) : -1;
        an_fx.escala = d < 0.12f ? 1.12f - 0.12f * d / 0.12f : 1;
        break;
    }
    case AD_FERMENTO: {                            // cai murcha e cresce, as bolhas subindo
        float d = tc - t0, q = fminf(1, d / 1.05f);
        an_fx.escala = 0.55f + 0.45f * (1 - (1 - q) * (1 - q));
        break;
    }
    default: break;
    }
    float d = tc - t0;
    switch (cn2_jeito(a)) {
    case CJ_PESADO: an_fx.escala = d < 0.10f ? 1.10f - d : 1; break;
    case CJ_MOLE: {                                // a casca: esparrama e volta, mole
        float q = fminf(1, d / 0.5f);
        an_fx.escala = 1 + 0.12f * sinf(q * 3.1416f * 2) * (1 - q);
        break;
    }
    case CJ_CAI: case CJ_AGUA: case CJ_GIRA: case CJ_PUXA: an_fx.escala = d < 0.08f ? 1.06f : 1; break;
    case CJ_DESLIZA: case CJ_ROLO: {               // aparece da esquerda para a direita, com quem passa
        float q = d / (cn2_jeito(a) == CJ_ROLO ? 0.50f : 0.34f);
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        break;
    }
    case CJ_PATA: {                                // o rasgo abre de cima para baixo, num golpe
        float q = d / 0.16f;
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        break;
    }
    default: break;
    }
    if (a == AD_CONFETE) { float q = d / 0.65f; an_fx.revela = q < 1 ? fmaxf(0, q) : -1; }   // os papeizinhos caem um a um
    if (a == AD_PURPURINA) {                       // a cola sai do meio para fora; depois a purpurina cai por cima
        float q = d / 0.36f;
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        an_fx.p2 = d < 0.52f ? 1 : d < 1.10f ? 1 - (d - 0.52f) / 0.58f : 0;
    }
    if (a == AD_IMPERMEAVEL) {                     // o verniz aparece pontilhando com o spray; depois o brilho passa
        float q = d / 0.62f;
        an_fx.revela = q < 1 ? fmaxf(0, q) : -1;
        an_fx.brilho = d > 0.70f && d < 1.10f ? (d - 0.70f) / 0.40f : -1;
    }
    if (a == AD_REMENDO) an_fx.p2 = d < 1.10f ? fmaxf(0.001f, (d - 0.10f) / 1.0f) : 0;   // a agulha costurando em volta
    if (a == AD_NEON) an_fx.p2 = d < 0.95f ? fmaxf(0.001f, d / 0.95f) : 0;               // acende piscando
    if (a == AD_BOMBA) an_fx.p2 = d > 0.12f && d < 0.55f ? 1 : 0;                         // o pavio solta faisca e apaga
    if (a == AD_POSTIT) an_fx.dobra = d < 0.70f ? 0.10f * fabsf(sinf(d * 14)) * (1 - d / 0.70f) : 0;   // a ponta balanca e assenta
    if (a == AD_ELETRONICO) an_fx.p2 = d > 0.88f ? 1 : 0;                                 // o LED acende no fim
    if (a == AD_DOURADO) an_fx.brilho = d > 0.10f && d < 0.60f ? (d - 0.10f) / 0.50f : -1;   // o pincel alisa e o brilho passa
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
    case AD_MASSINHA: {                            // o dedao desce, aperta e sobe
        float d = tc - t0;
        if (d > 0.10f && d < 0.62f) {
            float q = (d - 0.10f) / 0.52f, dy = q < 0.35f ? 16 * (1 - q / 0.35f) : q < 0.65f ? 0 : 18 * (q - 0.65f) / 0.35f;
            fer_luva(x + 1, y - 1 - (int)dy);
        }
        break;
    }
    case AD_BARRO: case AD_SUJEIRA: {              // os respingos voando do splat
        float d = tc - t0;
        if (d < 0.30f) {
            float q = d / 0.30f;
            for (int i = 0; i < 16; i++) {
                float an = i * 0.3927f + 0.2f, vv = R + 2 + (6 + (i * 37) % 9) * q;
                int sx = x + (int)(cosf(an) * vv), sy = y + (int)(sinf(an) * vv * 0.8f + 18 * q * q);
                if (q > 0.7f && i % 2) continue;
                int sz = i % 4 == 0 ? 3 : 2;
                gfx_rect(sx + 1, sy + 1, sz, sz, C_SOMBRA);
                gfx_rect(sx, sy, sz, sz, i % 3 ? RGB(156, 106, 64) : RGB(120, 78, 50));
            }
        }
        break;
    }
    case AD_INFECTADO: {                           // os esporos subindo da gosma
        float d = tc - t0;
        for (int i = 0; i < 8 && d < 1.0f; i++) {
            float q = d / 1.0f - i * 0.06f;
            if (q <= 0 || q >= 1) continue;
            int sx = x - 10 + (i * 7) % 21 + (int)(sinf(q * 8 + i) * 2), sy = y - 4 - (int)(q * 24);
            gfx_rect(sx, sy, 2, 2, i % 2 ? RGB(130, 204, 112) : RGB(88, 152, 84));
        }
        break;
    }
    case AD_FERMENTO: {                            // as bolhas estourando em cima, enquanto cresce
        float d = tc - t0;
        for (int i = 0; i < 6 && d < 1.1f; i++) {
            float c = fmodf(d * 2.2f + i * 0.37f, 1.0f);
            if (c > 0.25f) continue;
            int sx = x - 8 + (i * 5) % 17, sy = y - 8 - (int)(c * 20);
            gfx_pixel(sx, sy, C_BRANCO); gfx_pixel(sx + 1, sy - 1, C_AMARELO);
        }
        break;
    }
    default: break;
    }
    float d = tc - t0;
    switch (cn2_jeito(a)) {                        // o impacto de cada jeito de chegar
    case CJ_PESADO: case CJ_CAI: case CJ_MOLE:
        if (d < 0.30f) cn2_poeira(x, y, R, d / 0.30f, C_MARFIM_S, RGB(204, 194, 172));
        if (cn2_jeito(a) == CJ_PESADO && d < 0.45f) cn2_poeira(x, y + 2, R + 3, d / 0.45f, RGB(128, 120, 112), C_MARFIM_S);
        break;
    case CJ_AGUA:                                  // o patinho cai na agua: espirra azul
        if (d < 0.40f) cn2_poeira(x, y, R, d / 0.40f, RGB(168, 204, 230), C_BRANCO);
        break;
    case CJ_PUXA:                                  // as limalhas pulam em volta do ima
        for (int i = 0; i < 10 && d < 0.40f; i++) {
            float q = d / 0.40f, an = i * 0.628f;
            int px = x + (int)(cosf(an) * (R + 3 + q * 6)), py = y + (int)(sinf(an) * (R + 2 + q * 4)) - (int)(sinf(q * 3.1416f) * 5);
            gfx_rect(px, py, 1 + (i & 1), 1, i % 3 ? RGB(104, 112, 124) : C_PRATA);
        }
        break;
    case CJ_ROLO: {                                // o rolo passa da esquerda para a direita desenrolando
        float q = fminf(1, d / 0.50f);
        if (d < 0.80f) {
            int rx = x - (int)(R * 0.95f) + (int)(2 * R * 0.95f * q);
            if (q >= 1) rx += (int)((d - 0.50f) / 0.30f * 22);
            fer_rolo(rx, y - 4, an_cor(a));
        }
        break;
    }
    case CJ_PATA:                                  // os fiapos e o farelo do rasgo
        for (int i = 0; i < 9 && d < 0.40f; i++) {
            float q = d / 0.40f;
            int px = x - 10 + (i * 5) % 21 - (int)(q * 8), py = y + 6 + (int)(q * q * 18) - (int)(sinf(q * 3.1416f) * 6);
            gfx_rect(px, py, 2, 1, i & 1 ? C_MARFIM_S : RGB(118, 30, 38));
        }
        break;
    case CJ_DESLIZA:                               // o vidro: um brilho corre na quina ao encaixar
        if (d > 0.34f && d < 0.60f) {
            float q = (d - 0.34f) / 0.26f;
            int px = x - (int)R + (int)(2 * R * q), py = y - (int)R;
            gfx_rect(px - 1, py, 3, 1, C_BRANCO); gfx_rect(px, py - 1, 1, 3, C_BRANCO);
        }
        break;
    default: break;
    }
    switch (a) {                                   // e a ferramenta de cada um
    case AD_CONFETE: {                             // a chuva de papel picado: sobe do canhao e cai no lado e em volta
        static const uint16_t P[6] = { RGB(190, 56, 60), RGB(228, 194, 106), RGB(96, 134, 196), RGB(130, 204, 112),
                                       RGB(208, 98, 108), RGB(150, 112, 160) };
        float tt = tc - 0.24f;
        if (tt > 0 && tt < 1.0f && tc < 0.32f) fer_canhao(x + 14, y - 30);
        for (int i = 0; i < 22 && tt > 0 && tt < 1.05f; i++) {
            float vx = ((int)(hash2(i, 9) % 40) - 20) * 0.9f, vy = -40 - (int)(hash2(i, 4) % 30);
            float q = tt - (i % 4) * 0.03f;
            if (q <= 0) continue;
            float px = x + 14 + vx * q, py = y - 30 + vy * q + 90 * q * q;
            if (py > y + 22) continue;
            gfx_rect((int)px, (int)py, 2, 1 + (i & 1), P[i % 6]);
        }
        break;
    }
    case AD_PURPURINA: {
        if (d < 0.40f) fer_cola(x + 2 + (int)(sinf(d * 30) * 2), y - 6);            // a cola saindo, o frasco apertado
        else if (d < 0.52f) fer_cola(x + 2 + (int)((d - 0.40f) * 160), y - 6 - (int)((d - 0.40f) * 120));
        if (d > 0.50f && d < 1.15f) {              // o potinho chacoalhando em cima e a purpurina caindo
            fer_saleiro(x + (int)(sinf(d * 40) * 3), y - 22);
            for (int i = 0; i < 12; i++) {
                float c = fmodf(d * 3 + i * 0.083f, 1.0f);
                int px = x - 6 + (int)(hash2(i, 2) % 13), py = y - 20 + (int)(c * 22);
                gfx_pixel(px, py, (i % 3) == 0 ? C_BRANCO : (i % 3) == 1 ? C_AMARELO : C_ROSA);
            }
        }
        if (d > 1.20f && d < 1.50f)                // o sopro: o excesso voa
            for (int i = 0; i < 10; i++) {
                float q = (d - 1.20f) / 0.30f;
                gfx_pixel(x + 4 + (int)(q * (14 + i * 3)), y - 6 + i - (int)(q * 8), i & 1 ? C_AMARELO : C_BRANCO);
            }
        break;
    }
    case AD_IMPERMEAVEL:                           // a nevoa do spray saindo do bico ate o lado
        fer_spray(x + 20, y - 22);
        for (int i = 0; i < 26 && d < 0.70f; i++) {
            float c = fmodf(d * 2.6f + i * 0.0385f, 1.0f);
            int px = x + 17 - (int)(c * (26 + (i % 5) * 2)), py = y - 22 + (int)(c * (16 + (int)(hash2(i, 7) % 12) - 6));
            if (bayer(px, py) < 0.8f - c * 0.4f) gfx_pixel(px, py, i % 3 ? C_MARFIM : C_CELESTE);
        }
        break;
    case AD_CHUMBO:                                // duas marteladas nos rebites
        if (d > 0.25f && d < 0.72f) {
            float q = (d - 0.25f) / 0.47f;
            int k = q < 0.5f ? -1 : 1, rx, ry;
            cn2_tela(k * 0.46f, 0.58f, (float)x, (float)y, R, th, &rx, &ry);
            float bate = fabsf(sinf(q * 3.1416f * 2));
            fer_martelo(rx, ry - 1 - (int)(bate * 6), 0.6f + bate * 0.5f);
        }
        break;
    case AD_ELETRONICO:                            // a solda em cada perninha: a faisca e a fumacinha
        for (int i = 0; i < 4; i++) {
            float q = (d - 0.18f - i * 0.17f) / 0.20f;
            if (q < 0 || q > 1) continue;
            int lx, ly;
            cn2_tela(i & 1 ? 1.0f : -1.0f, -0.6f + (i >> 1) * 0.8f, (float)x, (float)y, R, th, &lx, &ly);
            if (q < 0.4f) { gfx_rect(lx - 1, ly, 3, 1, C_AMARELO); gfx_rect(lx, ly - 1, 1, 3, C_AMARELO); gfx_pixel(lx, ly, C_BRANCO); }
            else gfx_rect(lx + (int)((q - 0.4f) * 4), ly - 3 - (int)((q - 0.4f) * 14), 2, 2, q < 0.7f ? C_MARFIM_S : C_CINZA);
        }
        break;
    case AD_REMENDO: {                             // a agulha dando a volta no pano
        float q = (d - 0.10f) / 1.0f;
        if (q > 0 && q < 1) {
            float an = (q - 0.25f) * 6.2832f - 3.1416f, rr = R * 0.82f;
            int px = x + (int)(cosf(an + th) * rr), py = y + (int)(sinf(an + th) * rr);
            fer_agulha(px, py, px + 10, py - 14);
        }
        break;
    }
    case AD_RATOEIRA:                              // a barra puxada para tras: o clique de armar
        if (d > 0.22f && d < 0.40f) {
            int px, py;
            cn2_tela(-0.30f, -0.50f, (float)x, (float)y, R, th, &px, &py);
            gfx_rect(px - 2, py - 4, 5, 1, C_BRANCO);
        }
        break;
    case AD_VELCRO: case AD_DUPLA: case AD_POSTIT:  // a luva aperta
        if (d > 0.02f && d < 0.48f) {
            float q = (d - 0.02f) / 0.46f, dy = q < 0.35f ? 14 * (1 - q / 0.35f) : q < 0.6f ? 0 : 16 * (q - 0.6f) / 0.4f;
            int px = x, py = y;
            if (a == AD_POSTIT) cn2_tela(0, -0.70f, (float)x, (float)y, R, th, &px, &py);
            fer_luva(px + 1, py - 1 - (int)dy);
        }
        break;
    case AD_CARBONO:                               // o lapis passa riscando por cima
        if (d > 0.14f && d < 0.70f) {
            float q = (d - 0.14f) / 0.56f;
            int px = x - (int)(R * 0.6f) + (int)(R * 1.2f * q), py = y - 3 + (int)(sinf(q * 3.1416f * 5) * 3);
            fer_lapis(px, py);
        }
        break;
    case AD_DOURADO:                               // o pincel macio alisando a folha
        if (d > 0.08f && d < 0.62f) {
            float q = (d - 0.08f) / 0.54f;
            fer_pincel(x - (int)(R * 0.7f) + (int)(R * 1.4f * q), y - 2 + (int)(sinf(q * 3.1416f * 3) * 3), 1);
        }
        for (int i = 0; i < 5 && d > 0.2f && d < 0.7f; i++) {   // as lascas que soltam e caem
            float q = (d - 0.2f) / 0.5f;
            gfx_pixel(x - 8 + i * 4, y + (int)R + (int)(q * q * 12) + i % 2, i & 1 ? C_OURO : C_AMARELO);
        }
        break;
    case AD_BOMBA:                                 // o pavio: as faiscas e a fumacinha quando apaga
        if (d > 0.12f && d < 0.75f) {
            int px, py;
            cn2_tela(0.80f, -0.62f, (float)x, (float)y, R, th, &px, &py);
            if (d < 0.55f) for (int i = 0; i < 4; i++) {
                float an = t_fase * 23 + i * 1.6f;
                gfx_pixel(px + (int)(cosf(an) * (2 + i % 2)), py + (int)(sinf(an) * (2 + i % 2)), i & 1 ? C_AMARELO : C_FOGO);
            } else gfx_rect(px + 1, py - 2 - (int)((d - 0.55f) * 30), 2, 2, C_CINZA);
        }
        break;
    default: break;
    }
}

// ---- A acao na mesa, na proposta ---------------------------------------------------
// ag2_fx: o adesivo agindo no dado (o lugar de an_fx_agindo). e: o evento
// (no pouso, NULL).
static bool ag2_tem(int a) { return ad_v2(a); }
static float ag2_dura(const evento_t *e)
{
    if (ad_v2(e->ad) && e->ad == AD_RASPADINHA && !e->copia) return 2.1f;
    return 0;
}
// O gesto pelo acabamento: o papel descola a ponta (o de hoje); a tinta nao
// descola, so o brilho passa; a massa balanca; o objeto da um pulinho.
static void ag2_fx_acab(int a, float p)
{
    switch (an_acab(a)) {
    case ACAB_GOSMA: {
        float q = fminf(1, p / 0.7f);
        an_fx.escala = 1 + 0.10f * sinf(q * 3.1416f * 5) * (1 - q);
        an_fx.brilho = p >= 0.10f && p < 0.46f ? (p - 0.10f) / 0.36f : -1;
        break;
    }
    case ACAB_TINTA: case ACAB_TINTA_FINA:
        an_fx.brilho = p >= 0.08f && p < 0.50f ? (p - 0.08f) / 0.42f : -1;
        break;
    case ACAB_OBJETO: case ACAB_PECAS:
        an_fx.escala = p < 0.14f ? 1 + 0.12f * sinf(p / 0.14f * 3.1416f) : 1;
        an_fx.brilho = p >= 0.10f && p < 0.46f ? (p - 0.10f) / 0.36f : -1;
        break;
    default: an_fx_agindo(p, false); break;
    }
}
static void ag2_fx(int a, float p, const evento_t *e)
{
    switch (a) {
    case AD_FERMENTO:                              // a massa incha e fica maior
        an_fx.escala = p < 0.6f ? 1 + 0.16f * sinf(p / 0.6f * 1.5708f) : 1.16f - 0.16f * (p - 0.6f) / 0.4f;
        break;
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
    default: ag2_fx_acab(a, p); break;
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
