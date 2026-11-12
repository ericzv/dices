// ===========================================================================
// O elevador do comeco da run: a placa de latao de perto. Ela sobe da beira
// de baixo (passa um tico e assenta) e um brilho corre pelo latao; o botao do
// andar afunda e acende e a setinha de "subindo" acende; o ponteiro sobe pelo
// mostrador (cada lampadinha pisca quando ele passa e a placa treme de leve
// com o motor), freia, passa um tico e volta. Na chegada o sino balanca, o
// numero do andar acende na cor dele, a placa de esmalte mostra o andar e o
// botao apaga; a placa desce. Os tempos saem de elev_* (estado.inc): quanto
// mais alto o andar, mais demora a subida.
// ===========================================================================
#define EV_PI 3.14159265f
static float ev_cl(float q) { return q < 0 ? 0 : q > 1 ? 1 : q; }
static float ev_out(float q) { q = ev_cl(q); return 1 - (1 - q) * (1 - q) * (1 - q); }
static float ev_in(float q) { q = ev_cl(q); return q * q * q; }
static float ev_ang(float i) { return EV_PI * (0.9f - 0.8f * i / N_NIVEIS); }
static void ev_gravado(int cx, int y, const char *s, int f)
{
    int x = cx - gfx_largura(s, f) / 2;
    gfx_texto(x + 1, y + 1, s, K_LAT4, f, false);
    gfx_texto(x, y, s, K_LAT0, f, false);
}

// O mostrador em arco: raios de sol no latao, a faixa de esmalte preto com os
// numeros, as lampadinhas de cada andar e o ponteiro.
static void ev_mostrador(int cx, int cy, int R, float p, int d, float aceso, float t, float flash_ate)
{
    int Rb = R - 4, Rf = R - (R >= 60 ? 18 : 13);
    for (int y = cy - R; y <= cy; y++)
        for (int x = cx - R; x <= cx + R; x++) {
            float dx = (float)(x - cx), dy = (float)(cy - y) + 0.5f;
            float r = sqrtf(dx * dx + dy * dy);
            if (r > R + 0.5f) continue;
            float a = atan2f(dy, dx);
            uint16_t c;
            if (r > Rb) {                                     // o aro, com a luz de cima a esquerda
                float k = cosf(a - 2.35f);
                float idx = 1.6f + 1.6f * k;
                int i = (int)idx;
                if (idx - i > bayer(x, y)) i++;
                static const uint16_t RR[5] = { K_LAT0, K_LAT1, K_LAT2, K_LAT3, K_LAT4 };
                c = RR[i < 0 ? 0 : i > 4 ? 4 : i];
                if (r > R - 0.5f) c = C_BARRA;
            } else if (r > Rf) {
                c = r > Rb - 1 ? K_PRETO : K_PLACA;
            } else {
                int raio = (int)(a / (EV_PI / 28));
                c = (raio & 1) ? K_LAT1 : K_LAT2;
                if (r > Rf - 2) c = K_LAT0;
                if (r < Rf * 0.45f && (raio & 1)) c = K_LAT2;
                if (r < Rf * 0.45f && !(raio & 1)) c = K_LAT3;
            }
            px(x, y, c);
        }
    // a barra de baixo
    gfx_rect(cx - R, cy + 1, 2 * R + 1, 5, K_LAT2);
    hl(cx - R, cy + 1, 2 * R + 1, K_LAT4);
    hl(cx - R, cy + 5, 2 * R + 1, K_LAT0);
    ui_contorno(cx - R - 1, cy, 2 * R + 3, 7, C_BARRA);
    // os numeros e as lampadinhas
    int f = R >= 60 ? 1 : GFX_MIUDO;
    for (int i = 0; i <= N_NIVEIS; i++) {
        float a = ev_ang((float)i);
        float rn = (Rb + Rf) / 2.0f + 0.5f;
        int nx = cx + (int)lroundf(cosf(a) * rn), ny = cy - (int)lroundf(sinf(a) * rn);
        uint16_t cor = K_MARF3;
        bool este = i == d && aceso > 0;
        if (este) cor = gfx_na_paleta(COR_DIF[d]);
        int lw = gfx_largura(MARCA_E[i], f);
        int alt = f == 1 ? 15 : 9;
        int ty = ny - alt / 2 - (f == 1 ? 5 : 3);
        if (este && aceso > 0.2f) {                            // o halo do numero aceso
            for (int yy = ny - 9; yy <= ny + 9; yy++)
                for (int xx = nx - 9; xx <= nx + 9; xx++) {
                    float dd = sqrtf((float)((xx - nx) * (xx - nx) + (yy - ny) * (yy - ny)));
                    float rr2 = sqrtf((float)((xx - cx) * (xx - cx) + (cy - yy) * (cy - yy)));
                    if (rr2 > Rb - 1 || rr2 < Rf + 1) continue;
                    if (bayer(xx, yy) < (1 - dd / 9) * 0.6f * aceso) px(xx, yy, tinta_escura(cor) == K_EB ? K_EB2 : tinta_escura(cor));
                }
        }
        gfx_texto(nx - lw / 2, ty, MARCA_E[i], cor, f, f == 1);
        // a lampadinha
        float rl = Rf - 4.5f;
        int lx = cx + (int)lroundf(cosf(a) * rl), ly = cy - (int)lroundf(sinf(a) * rl);
        bool pisca = p >= i - 0.15f && p < i + 0.6f && t < flash_ate;
        bool fica = este;
        uint16_t lc = fica ? K_LAT4 : pisca ? K_FOGO2 : K_VIN0;
        gfx_rect(lx - 1, ly - 1, 2, 2, lc);
        if (fica || pisca) px(lx - 1, ly - 1, K_BRANCO);
    }
    // o ponteiro: preto, com a ponta em seta e o rabo
    float a = ev_ang(p);
    float ca = cosf(a), sa = sinf(a);
    int L = Rf - 8;
    int tx = cx + (int)lroundf(ca * L), ty = cy - (int)lroundf(sa * L);
    int bx = cx - (int)lroundf(ca * (R >= 60 ? 6 : 4)), by = cy + (int)lroundf(sa * (R >= 60 ? 6 : 4));
    gfx_linha_grossa(bx + 1, by + 1, tx + 1, ty + 1, R >= 60 ? 3 : 2, K_LAT0);   // a sombra no latao
    gfx_linha_grossa(bx, by, tx, ty, R >= 60 ? 3 : 2, K_PRETO);
    int sx = cx + (int)lroundf(ca * (L - 6)), sy = cy - (int)lroundf(sa * (L - 6));
    int nx = (int)lroundf(-sa * 3), ny = (int)lroundf(-ca * 3);
    gfx_linha(sx + nx, sy + ny, tx, ty, K_PRETO);
    gfx_linha(sx - nx, sy - ny, tx, ty, K_PRETO);
    gfx_disco(bx, by, R >= 60 ? 3 : 2, K_PRETO);
    // o pino
    gfx_disco(cx, cy, R >= 60 ? 6 : 4, K_LAT0);
    gfx_disco(cx, cy, R >= 60 ? 4 : 3, K_LAT3);
    px(cx - 1, cy - 1, K_BRANCO);
}

// O brilho que corre pelo latao: uma faixa diagonal que sobe um tom do
// latao escovado por onde passa (o resto fica como esta).
static void ev_brilho(int x, int y, int w, int h, float q)
{
    if (q <= 0 || q >= 1) return;
    float s = -20 + q * (w + h * 0.6f + 40);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            float d = i + j * 0.6f - s;
            if (d < -7 || d > 7) continue;
            if (d < -4 || d > 4) { if (bayer(x + i, y + j) > 0.5f) continue; }
            uint16_t c = pega(x + i, y + j);
            if (c == K_LAT1) px(x + i, y + j, K_LAT2);
            else if (c == K_LAT2) px(x + i, y + j, K_LAT3);
            else if (c == K_LAT3) px(x + i, y + j, K_LAT4);
        }
}

// A setinha de "subindo": o triangulo de vidro ambar, apagado ou aceso.
static void ev_seta(int cx, int cy, float luz)
{
    gfx_disco(cx, cy, 6, C_BARRA);
    gfx_disco(cx, cy, 5, K_LAT0);
    gfx_disco(cx, cy, 4, K_PLACA);
    for (int j = 0; j < 5; j++) {
        int hw = j;
        for (int i = -hw; i <= hw; i++) {
            uint16_t c = luz > 0.5f ? (j < 2 ? K_LAT4 : K_FOGO2) : luz > 0.05f ? K_FOGO : K_VIN0;
            px(cx + i, cy - 2 + j, c);
        }
    }
    if (luz > 0.5f) px(cx, cy - 2, K_BRANCO);
}

static void elevador_placa(float e, int d)
{
    float chega = elev_chega(d), dur = elev_dur(d), sai0 = dur - 0.32f;
    float v = e < 0.15f ? e / 0.15f : e > sai0 ? 1 - (e - sai0) / 0.32f : 1;
    if (v > 0) ui_escurece(0, 0, GFX_W, GFX_H, (int)(165 * ev_cl(v)));
    // a placa: sobe com mola (passa uns 6 px e assenta) e desce acelerando
    const int w = 200, h = 272, x = GFX_W / 2 - w / 2;
    float off = e < 0.5f ? 330 * expf(-14 * e) * cosf(11 * e) : 0;
    if (e > sai0) off = 340 * ev_in((e - sai0) / 0.32f);
    if (e > ELEV_SOBE && e < chega - 0.08f && d > 0) off += ((int)(e * 26)) & 1;   // o motor: a placa treme 1 px
    int y = 44 + (int)lroundf(off);
    if (y >= GFX_H) return;
    placa_latao(x, y, w, h);
    parafuso(x + 8, y + 8, 0.6f); parafuso(x + w - 9, y + 8, 2.2f);
    parafuso(x + 8, y + h - 9, 1.4f); parafuso(x + w - 9, y + h - 9, 0.2f);
    int cx = x + w / 2, cy = y + 94;
    // o ponteiro
    float p = elev_ponteiro(e, d);
    float aceso = e >= chega ? ev_out((e - chega) / 0.12f) : 0;
    ev_mostrador(cx, cy, 76, p, d, aceso, e, chega);
    // a placa de esmalte embaixo do mostrador: gravada; na chegada mostra o
    // andar na cor dele, com um risco de luz passando
    int py = cy + 12;
    if (aceso > 0) {
        const char *s = ANDAR_DIF[d];
        int lw = gfx_largura(s, 1) + 18;
        uint16_t cor = gfx_na_paleta(COR_DIF[d]);
        ui_contorno(cx - lw / 2, py, lw, 19, C_BARRA);
        gfx_rect(cx - lw / 2 + 1, py + 1, lw - 2, 17, K_PLACA);
        hl(cx - lw / 2 + 1, py + 1, lw - 2, K_EB2);
        float q = (e - chega - 0.1f) / 0.3f;
        if (q > 0 && q < 1) {                             // o risco de luz no esmalte, por tras das letras
            int sx = cx - lw / 2 + (int)(q * (lw + 10)) - 5;
            for (int j = 2; j < 18; j++)
                for (int k = 0; k < 2; k++) {
                    int xx = sx + k - j / 4;
                    if (xx > cx - lw / 2 && xx < cx + lw / 2 - 1) px(xx, py + j, k ? K_EB2 : K_CINZA);
                }
        }
        gfx_texto(cx - gfx_largura(s, 1) / 2, py - 1, s, aceso > 0.6f ? cor : K_MARF3, 1, true);
    } else ev_gravado(cx, py + 2, "ELEVADOR", GFX_MIUDO);
    // os botoes: duas colunas, de baixo para cima como no painel de verdade
    int by0 = py + 34;
    for (int i = 0; i <= N_NIVEIS + 1; i++) {
        int col = i % 2, lin = 5 - i / 2;
        int bx = cx + (col ? 26 : -26), byy = by0 + lin * 21;
        if (i == N_NIVEIS + 1) {                               // o alarme: um sininho gravado
            botao_el(bx, byy, 9, "", 0, false);
            gfx_disco(bx, byy - 1, 3, K_LAT2);
            gfx_rect(bx - 3, byy - 1, 7, 3, K_LAT2);
            hl(bx - 4, byy + 2, 9, K_LAT3);
            px(bx, byy + 3, K_LAT3);
            continue;
        }
        float luz = 0;
        bool fundo = false;
        if (i == d) {
            if (e >= ELEV_BOTAO && e < ELEV_BOTAO + 0.12f) fundo = true;      // afunda
            if (e >= ELEV_BOTAO + 0.04f && e < chega) luz = ev_out((e - ELEV_BOTAO - 0.04f) / 0.08f);
            if (e >= chega) luz = 1 - ev_out((e - chega) / 0.25f);          // chegou: o botao apaga
        }
        botao_el(bx, byy, 9, MARCA_E[i], luz, fundo);
    }
    // a setinha de subindo, no alto a esquerda: acende com o botao, apaga na chegada
    {
        float l = e >= ELEV_BOTAO + 0.04f && e < chega && d > 0 ? 1 : 0;
        ev_seta(x + 22, y + 20, l);
    }
    // o sino da chegada, no alto a direita: balanca, com as ondinhas do som
    {
        int sx = x + w - 22, sy = y + 18;
        float ang = e >= chega && e < chega + 0.7f ? sinf((e - chega) * 30) * (1 - (e - chega) / 0.7f) * 0.5f : 0;
        int ox = (int)lroundf(sinf(ang) * 3);
        vl(sx, sy - 6, 3, K_LAT0);
        gfx_disco(sx + ox, sy, 4, C_BARRA);
        gfx_disco(sx + ox, sy, 3, K_LAT3);
        gfx_rect(sx - 4 + ox, sy, 9, 4, K_LAT3);
        hl(sx - 5 + ox, sy + 4, 11, K_LAT1);
        px(sx - 1 + ox, sy - 2, K_BRANCO);
        px(sx + ox, sy + 5, K_LAT0);
        if (ang != 0) {
            int k = (int)((e - chega) * 12) % 3;
            for (int r = 7 + k * 2; r <= 9 + k * 2; r += 2) {
                px(sx - r, sy - 1, K_LAT4); px(sx - r - 1, sy + 1, K_LAT4);
                px(sx + r, sy - 1, K_LAT4); px(sx + r + 1, sy + 1, K_LAT4);
            }
        }
    }
    ev_brilho(x + 2, y + 2, w - 4, h - 4, (e - 0.16f) / 0.5f);   // o brilho, quando a placa assenta
}
