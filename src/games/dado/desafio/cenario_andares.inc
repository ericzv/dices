// ===========================================================================
// Os outros andares do salao visto de frente: a parede e o chao
// de cada um saem das salas do predio (predio_salas.inc), e cada salao tem
// uma peca de destaque no vao do meio e pecas pequenas nos outros. No salao
// do Coringa (do 5º para cima), o fim do andar, apagado.
//   1º madeira, dardos e carteado     6º art deco, piano e cantora
//   2º o bar de tijolo                7º a cortina e a mesa de dados
//   3º aco, globo e roleta            8º o lounge do sax
//   4º o jardim de inverno            9º o aquario e o poquer
// ===========================================================================

// ---- pecas de chao e parede que mais de um andar usa -------------------------
// O chao de tabuas atravessadas, em fileiras, com as emendas desencontradas.
static void chao_tabuas(int m, int tb, int prof, int comp)
{
    for (int y = FR_CHAO; y < CH_Y1; y++) {
        int fila = (int)floorf(fr_z(y) / prof), fa = (int)floorf(fr_z(y - 1) / prof);
        int desl = (int)(ruido_xy(fila, 5) % comp);
        int t1 = tb + (ruido_xy(fila, 9) % 4 == 0);
        for (int x = CH_X0; x < CH_X1; x++) {
            int k = (int)floorf((fr_xm(x, y) + cen_off + desl) / comp), ka = (int)floorf((fr_xm(x - 1, y) + cen_off + desl) / comp);
            int t = t1;
            if (fila != fa || k != ka) t = tb - 1;
            if (y < FR_CHAO + 3 && bayer(x, y) < 0.5f) t = tb - 1;
            cp(x, y, m, t);
        }
    }
}
// O xadrez em perspectiva (a casa clara e a escura, cada uma com material e tom).
static void chao_xadrez(int m1, int t1, int m2, int t2, int lado)
{
    for (int y = FR_CHAO; y < CH_Y1; y++) {
        int row = (int)floorf(fr_z(y) / lado);
        for (int x = CH_X0; x < CH_X1; x++) {
            int col = (int)floorf((fr_xm(x, y) + cen_off) / lado);
            bool claro = (row + col) & 1;
            int t = claro ? t1 : t2;
            if (y < FR_CHAO + 3 && bayer(x, y) < 0.5f) t--;
            cp(x, y, claro ? m1 : m2, t);
        }
    }
}
// O carpete: um tom so, com um desenho ralo (0 pontinhos, 1 ondas).
static void chao_carpete(int m, int tb, int desenho)
{
    for (int y = FR_CHAO; y < CH_Y1; y++) {
        float z = fr_z(y);
        for (int x = CH_X0; x < CH_X1; x++) {
            float xm = fr_xm(x, y) + cen_off;
            int t = tb;
            if (desenho == 0 || desenho == 2) {
                int fila = (int)floorf(z / 10), fa = (int)floorf(fr_z(y - 1) / 10);
                if (fila != fa && fabsf(xm + (fila & 1) * 10 - 20 * roundf((xm + (fila & 1) * 10) / 20)) < 0.7f / fr_esc(y)) t = tb + 1;
            } else if ((desenho == 1 || desenho == 3) && fabsf(sinf(xm * 0.12f + z * 0.5f)) < 0.07f && ((int)(z / 6) & 1)) t = tb + 1;
            if (desenho >= 2 && t == tb && bayer(x, y) < 0.25f) t = tb + 1;     // o veludo do carpete, ralo
            if (y < FR_CHAO + 3 && bayer(x, y) < 0.5f) t = tb - 1;
            cp(x, y, m, t);
        }
    }
}
static void rodape(int m, int t)
{
    for (int y = FR_RODA; y < FR_CHAO; y++) chl(CH_X0, y, CH_W, m, y == FR_RODA ? t + 2 : t);
}
// Um peixinho de 5 x 3 (dir 1 para a direita).
static void peixe(int x, int y, int dir, uint16_t c, uint16_t c2)
{
    for (int i = 0; i < 4; i++) {
        int px = x + (dir > 0 ? i : -i);
        gfx_pixel(px, y, i == 3 ? c2 : c);
        if (i > 0 && i < 3) { gfx_pixel(px, y - 1, c); gfx_pixel(px, y + 1, c2); }
    }
    gfx_pixel(x - dir, y - 1, c2); gfx_pixel(x - dir, y + 1, c2);   // o rabo
    gfx_pixel(x + 2 * dir, y - 1, C_BARRA);                          // o olho
}

// ---- o 1º: madeira, dardos e carteado ---------------------------------------
static void deco_alvo(int cx, int cy)
{
    csombra(cx - 9, cy - 8, 23, 23);
    for (int j = -12; j <= 12; j++)
        for (int i = -12; i <= 12; i++) {
            float d = sqrtf((float)(i * i + j * j));
            if (d > 12.2f) continue;
            int setor = (int)((atan2f((float)j, (float)i) + 3.1416f) / (6.2832f / 20)) & 1;
            int m = CM_NEU, t = 1;
            if (d > 11) t = 0;                                       // o aro
            else if (d > 9.5f) { t = 1; if (((int)((atan2f((float)j, (float)i) + 3.1416f) * 57.3f)) % 18 < 2) { m = CM_MARF; t = 2; } }
            else if (d > 8.3f) { m = setor ? CM_VIN : CM_VERDE; t = 3; }   // o dobro
            else if (d > 5.2f) { m = setor ? CM_NEU : CM_MARF; t = setor ? 1 : 2; }
            else if (d > 4.0f) { m = setor ? CM_VIN : CM_VERDE; t = 3; }   // o triplo
            else if (d > 1.8f) { m = setor ? CM_NEU : CM_MARF; t = setor ? 1 : 2; }
            else if (d > 0.9f) { m = CM_VERDE; t = 3; }
            else { m = CM_VIN; t = 4; }
            if (i + j < -12 && d > 9) t++;                           // a luz de cima a esquerda no aro
            cp(cx + i, cy + j, m, t);
        }
    for (int k = 0; k < 7; k++) cp(cx + 2 + k, cy - 2 - k, CM_LAT, k < 2 ? 2 : 3);   // o dardo cravado
    cp(cx + 8, cy - 9, CM_VIN, 4); cp(cx + 9, cy - 8, CM_VIN, 4); cp(cx + 9, cy - 10, CM_VIN, 3); cp(cx + 10, cy - 9, CM_VIN, 3);
}
static void deco_placar(int cx, int y)
{
    int x = cx - 8;
    csombra(x + 1, y + 2, 17, 22);
    crect(x, y, 16, 20, CM_MAD, 3);
    crect(x + 2, y + 2, 12, 16, CM_NEU, 1);
    for (int l = 0; l < 3; l++) {                                    // os risquinhos do placar, de giz
        int ly = y + 4 + l * 5, n = 2 + (int)(ruido_xy(l, 11) % 4);
        for (int k = 0; k < n && k < 4; k++) cvl(x + 4 + k * 2, ly, 3, CM_MARF, 3);
        if (n >= 5) for (int k = 0; k < 7; k++) cp(x + 3 + k, ly + 2 - k / 3, CM_MARF, 3);
    }
}
static void deco_tacos(int cx, int yb)
{
    int x = cx - 11;
    csombra(x + 1, yb - 50, 25, 52);
    crect(x, yb - 6, 22, 5, CM_MAD, 3); chl(x, yb - 6, 22, CM_MAD, 4);   // a base com os furos
    crect(x, yb - 48, 22, 4, CM_MAD, 3); chl(x, yb - 48, 22, CM_MAD, 4); // a presilha de cima
    for (int k = 0; k < 4; k++) {
        int tx = x + 3 + k * 5, topo = yb - 52 + (k & 1) * 2;
        cvl(tx, topo, yb - 6 - topo, CM_MAD, 4);
        cvl(tx + 1, topo + 18, yb - 24 - topo, CM_MAD, 2);          // o cabo, mais grosso embaixo
        cp(tx, topo, CM_MARF, 4);                                     // a sola azul de giz na ponta
        cp(tx, topo - 1, CM_AZ, 3);
    }
}
static void deco_royal(int cx, int cy)
{
    int w = 38, h = 24, x = cx - w / 2, y = cy - h / 2;
    csombra(x + 1, y + 2, w + 4, h + 4);
    crect(x, y, w, h, CM_VERDE, 2);
    for (int k = 0; k < 5; k++) {                                    // as cinco cartas em leque
        int kx = x + 5 + k * 6, ky = y + 6 - (k == 2 ? 2 : k == 1 || k == 3 ? 1 : 0);
        crect(kx - 1, ky - 1, 9, 13, CM_NEU, 0);
        crect(kx, ky, 7, 11, CM_MARF, 4);
        cvl(kx + 6, ky, 11, CM_MARF, 3);
        cp(kx + 1, ky + 1, CM_VIN, 3);
        cp(kx + 3, ky + 5, CM_VIN, 4); cp(kx + 2, ky + 5, CM_VIN, 4); cp(kx + 3, ky + 4, CM_VIN, 4); cp(kx + 4, ky + 5, CM_VIN, 4); cp(kx + 3, ky + 6, CM_VIN, 4);
    }
    cmoldura(x, y, w, h, CM_LAT);
}

static void cen_primeiro(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)                           // as tabuas em pe, com os nos
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, u = X % 9, k = X / 9, t = 1;
            if (u == 0) t = 0;
            else if (ruido_xy(k, 3) % 8 == 0) t = 2;
            if (ruido_xy(k, (y - CH_Y0) / 7) % 53 == 0 && u > 2 && u < 7 && (y % 7) < 2) t = 0;   // um no da madeira
            if (y < CH_Y0 + 3) t = y == CH_Y0 + 1 ? 3 : 0;          // a sanca
            if (y == 74) t = 3;                                       // a moldura de pendurar quadro
            if (y == 75) t = 0;
            cp(x, y, CM_MAD, t);
        }
    rodape(CM_MAD, 1);
    chao_tabuas(CM_MAD, 2, 9, 70);
    if (a == 0) {                                                    // o Verde: o alvo de dardos
        if (vlarg(&v[0]) >= 26) deco_tacos(vmeio(&v[0]), 128);
        if (nv > 1 && vlarg(&v[1]) >= 50) { int c = vmeio(&v[1]); deco_alvo(c - 8, 96); deco_placar(c + 18, 88); }
        if (nv > 2 && vlarg(&v[2]) >= 28) deco_quadro(vmeio(&v[2]), 94, 22, 16, 0);
    } else if (a == 1) {                                             // o Vermelho: a mao de cartas no quadro
        if (vlarg(&v[0]) >= 26) deco_quadro(vmeio(&v[0]), 94, 20, 24, 1);
        if (nv > 1 && vlarg(&v[1]) >= 44) deco_royal(vmeio(&v[1]), 96);
        if (nv > 2 && vlarg(&v[2]) >= 26) deco_tacos(vmeio(&v[2]), 128);
    } else {                                                         // o Dourado: a paisagem grande
        if (vlarg(&v[0]) >= 20) deco_placar(vmeio(&v[0]), 88);
        if (nv > 1 && vlarg(&v[1]) >= 52) deco_quadro(vmeio(&v[1]), 96, 44, 28, 0);
        if (nv > 2 && vlarg(&v[2]) >= 28) deco_quadro(vmeio(&v[2]), 94, 20, 16, 2);
    }
}

// ---- o 2º: o bar de tijolo ---------------------------------------------------
// As letras do letreiro, 3 x 5 (B, A, R), em dobro.
static void deco_letreiro(int cx, int cy, bool aceso)
{
    static const char *const L[3] = { "110101110101110", "010101111101101", "110101110101101" };
    int x0 = cx - 21, y0 = cy - 10;
    for (int l = 0; l < 3; l++)
        for (int j = 0; j < 5; j++)
            for (int i = 0; i < 3; i++)
                if (L[l][j * 3 + i] == '1') crect(x0 + l * 16 + i * 4, y0 + j * 4, 3, 3, aceso ? CM_NEON : CM_VIN, aceso ? (i + j) & 1 ? 2 : 1 : 2);
    if (!aceso) return;
    for (int j = -3; j < 24; j++)                                      // o halo pontilhado em volta
        for (int i = -3; i < 46; i++) {
            int x = x0 + i, y = y0 + j, m = cm_de(x, y);
            if (m < 0 || m == CM_NEON) continue;
            bool perto = false;
            for (int b = -2; b <= 2 && !perto; b++)
                for (int a2 = -2; a2 <= 2 && !perto; a2++) if (cm_de(x + a2, y + b) == CM_NEON) perto = true;
            if (perto && bayer(x, y) < 0.45f) cp(x, y, CM_NEON, 0);
        }
    cdet(DT_LETREIRO, x0 - 3, y0 - 3, 49, 26);
    cluz(cx, cy + 10, 44, 26, 1.0f);
}
static void deco_espelho_bar(int cx, int y, int w)
{
    int x = cx - w / 2, h = 34;
    csombra(x + 2, y + 2, w + 4, h + 4);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) cp(x + i, y + j, CM_ACO, ((i + j) % 19 < 2) ? 2 : 1);
    for (int s2 = 0; s2 < 2; s2++) {                                  // duas prateleiras de garrafas
        int py = y + 14 + s2 * 13;
        for (int i = 2; i < w - 2; i += 4) {
            int h2 = 6 + (int)(ruido_xy(i, s2) % 4);
            int m = (i / 4 + s2) % 3 == 0 ? CM_LAT : (i / 4 + s2) % 3 == 1 ? CM_VERDE : CM_VIN;
            crect(x + i, py - h2, 2, h2, m, 3); cp(x + i, py - h2, m, 4);
            cp(x + i, py - h2 - 1, CM_NEU, 1);
        }
        chl(x, py, w, CM_MAD, 4); chl(x, py + 1, w, CM_MAD, 1);
    }
    cmoldura(x, y, w, h, CM_MAD);
}
static void deco_vitrola(int cx, int yb)
{
    int w = 26, h = 44, x = cx - w / 2, y = yb - h;
    csombra(x + 2, y + 2, w + 2, h);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            float dx = i - w / 2 + 0.5f, dy = j - 13;
            if (j < 13 && dx * dx / 169 + dy * dy / 169 > 1) continue;   // o arco de cima
            int m = CM_MAD, t = i < 3 ? 4 : i > w - 4 ? 2 : 3;
            float r = sqrtf(dx * dx + dy * dy);
            if (j < 18 && r > 8 && r < 11) { m = CM_NEON + 0; t = (int)(r * 2) % 3 ? 2 : 3; }   // as faixas de luz no arco
            if (j < 18 && r > 9 && r < 10) { m = CM_LUZ; t = 4; }
            if (j < 15 && r <= 8) { m = CM_NEU; t = 1; if ((i + j) % 4 == 0) t = 2; }   // o vidro com os discos atras
            if (j > 22 && j < 34 && i > 4 && i < w - 4) { m = CM_NEU; t = ((i + j) & 1) ? 0 : 2; }   // a grade do alto-falante
            if (j >= h - 4) { m = CM_MAD; t = 1; }
            cp(x + i, y + j, m, t);
        }
    crect(x + 4, y + 18, w - 8, 3, CM_LUZ, 3);                       // a fita de luz do meio
    cluz(cx, yb - 22, 30, 24, 0.9f);
}
static void deco_cartaz(int cx, int y)
{
    int x = cx - 10, w = 20, h = 26;
    csombra(x + 1, y + 2, w + 3, h + 3);
    crect(x, y, w, h, CM_MARF, 3);
    crect(x + 2, y + 2, w - 4, 12, CM_VIN, 3);
    cdisco(cx, y + 9, 4, CM_LAT, 4);                                 // o sol da propaganda
    for (int l = 0; l < 3; l++) chl(x + 3, y + 17 + l * 3, w - 6 - (l & 1) * 4, CM_NEU, 1);
    cmoldura(x, y, w, h, CM_MAD);
}

static void cen_segundo(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)                           // o tijolo, com o rejunte escuro
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, fila = (y - CH_Y0) / 5, desl = (fila & 1) * 5;
            int col = (X + desl) / 10, m = CM_VIN, t = 1;
            uint32_t h = ruido_xy(col, fila);
            if ((y - CH_Y0) % 5 == 4 || (X + desl) % 10 == 0) t = 0;     // o rejunte: o tom vizinho, mais escuro
            else if (h % 9 == 0) t = 2;
            if (y < CH_Y0 + 3) { m = CM_MAD; t = y == CH_Y0 + 1 ? 2 : 0; }   // a viga
            cp(x, y, m, t);
        }
    rodape(CM_MAD, 0);
    chao_tabuas(CM_MAD, 1, 8, 60);
    if (a == 0) {                                                    // o Verde: o letreiro BAR
        if (vlarg(&v[0]) >= 26) deco_cartaz(vmeio(&v[0]), 84);
        if (nv > 1 && vlarg(&v[1]) >= 50) deco_letreiro(vmeio(&v[1]), 74, true);
        if (nv > 2 && vlarg(&v[2]) >= 20) deco_relogio(vmeio(&v[2]), 84);
    } else if (a == 1) {                                             // o Vermelho: o espelho com as garrafas
        if (vlarg(&v[0]) >= 20) deco_relogio(vmeio(&v[0]), 84);
        if (nv > 1 && vlarg(&v[1]) >= 50) deco_espelho_bar(vmeio(&v[1]), 78, 46);
        if (nv > 2 && vlarg(&v[2]) >= 26) deco_cartaz(vmeio(&v[2]), 84);
    } else {                                                         // o Dourado: a vitrola
        if (vlarg(&v[0]) >= 26) deco_cartaz(vmeio(&v[0]), 84);
        if (nv > 1 && vlarg(&v[1]) >= 34) deco_vitrola(vmeio(&v[1]), FR_CHAO + 6);
        if (nv > 2 && vlarg(&v[2]) >= 20) deco_relogio(vmeio(&v[2]), 84);
    }
}

// ---- o 3º: aco, globo e roleta -----------------------------------------------
static void deco_roleta(int cx, int cy)
{
    csombra(cx - 13, cy - 12, 30, 30);
    for (int j = -16; j <= 16; j++)
        for (int i = -16; i <= 16; i++) {
            float d = sqrtf((float)(i * i + j * j));
            if (d > 16.3f) continue;
            float a = atan2f((float)j, (float)i) + 3.1416f;
            int setor = (int)(a / (6.2832f / 37)), m, t;
            if (d > 14.5f) { m = CM_MAD; t = i + j < 0 ? 4 : 2; }     // o aro de madeira
            else if (d > 13.5f) { m = CM_LAT; t = 3; }
            else if (d > 9.5f) {                                     // as casas, vermelha e preta (e o zero verde)
                if (setor == 0) { m = CM_VERDE; t = 3; } else { m = setor & 1 ? CM_VIN : CM_NEU; t = setor & 1 ? 3 : 0; }
                if (d < 10.5f) { m = CM_LAT; t = 2; }
            } else if (d > 3) { m = CM_MAD; t = 2 + (((int)(a * 2.9f)) & 1); }   // o cone
            else { m = CM_LAT; t = d < 1.5f ? 5 : 4; }               // o pino do meio
            cp(cx + i, cy + j, m, t);
        }
    cdisco(cx + 9, cy - 9, 1, CM_MARF, 5);                          // a bolinha
}
static void deco_globo(int cx, int cy)
{
    cvl(cx, CH_Y0, cy - 11 - CH_Y0, CM_ACO, 2);
    for (int j = -11; j <= 11; j++)
        for (int i = -11; i <= 11; i++) {
            if (i * i + j * j > 125) continue;
            int t = ((i + 12) / 2 + (j + 12) / 2) & 1 ? 3 : 2;
            if (i + j < -6) t += 1;
            if (i * i + j * j > 95) t -= 1;
            if ((i == -4 || i == -3) && (j == -5 || j == -6)) t = 5;
            cp(cx + i, cy + j, CM_ACO, t);
        }
    cdet(DT_GLOBO, CH_X0, CH_Y0, CH_W, FR_RODA - CH_Y0);
}
static void deco_niqueis(int cx, int yb, int n)
{
    int w = 16, gap = 4, x0 = cx - (n * w + (n - 1) * gap) / 2;
    for (int k = 0; k < n; k++) {
        int x = x0 + k * (w + gap), y = yb - 34;
        csombra(x + 2, y + 2, w + 1, 34);
        crect(x, y, w, 34, CM_ACO, 2); cvl(x, y, 34, CM_ACO, 4);
        crect(x, y, w, 6, CM_VIN, 3); chl(x, y, w, CM_VIN, 4);      // o topo
        crect(x + 2, y + 9, w - 4, 8, CM_NEU, 0);
        for (int r = 0; r < 3; r++) {                                // os tres rolos, com os simbolos
            crect(x + 3 + r * 4, y + 10, 3, 6, CM_JAN, 3);
            int sm = (k + r) % 3;
            cp(x + 4 + r * 4, y + 13, sm == 0 ? CM_NEON : sm == 1 ? CM_JAN : CM_AQUA, sm == 1 ? 2 : 2);
        }
        crect(x + 3, y + 20, w - 6, 3, CM_NEU, 1);                   // a bandeja
        crect(x + 2, y + 27, w - 4, 4, CM_NEU, 1);
        cvl(x + w, y + 8, 9, CM_ACO, 4); cdisco(x + w, y + 7, 1, CM_VIN, 4);   // a alavanca
    }
    cluz(cx, yb - 20, n * 14, 22, 0.8f);
}

static void cen_terceiro(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)                           // as chapas com rebite e a fita de LED
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, u = X % 26, w = (y - CH_Y0 - 4) % 16, m = CM_ACO, t = 1;
            if (u == 0 || w == 0) t = 0;
            else if ((u == 3 || u == 23) && (w == 3 || w == 13)) t = 3;
            else if (w == 1) t = 2;                                  // a beira de cima de cada chapa pega a luz
            if (y < CH_Y0 + 3) { m = CM_CEU; t = y == CH_Y0 + 1 ? 3 : 1; }   // a fita de LED azul
            if (y >= FR_RODA - 2) { m = CM_CEU; t = (X % 48) < 9 ? 0 : 1; }
            cp(x, y, m, t);
        }
    rodape(CM_ACO, 0);
    for (int y = FR_CHAO; y < CH_Y1; y++)                           // o piso de concreto polido, com o reflexo do LED
        for (int x = CH_X0; x < CH_X1; x++) {
            int row = (int)floorf(fr_z(y) / 22), ra = (int)floorf(fr_z(y - 1) / 22);
            int col = (int)floorf((fr_xm(x, y) + cen_off) / 22), ca = (int)floorf((fr_xm(x - 1, y) + cen_off) / 22);
            int m = CM_ACO, t = 1;
            if (row != ra || col != ca) t = 0;
            if (y < FR_CHAO + 4 && bayer(x, y) < 0.35f) { m = CM_AZ; t = 1; }
            cp(x, y, m, t);
        }
    cluz(CH_X0 + CH_W / 2, FR_RODA, CH_W / 2 + 40, 6, 0.6f);
    if (a == 0) {                                                    // o Verde: a roda da roleta na parede
        if (nv > 1 && vlarg(&v[1]) >= 40) deco_roleta(vmeio(&v[1]), 98);
    } else if (a == 1) {                                             // o Vermelho: o globo espelhado
        if (nv > 1 && vlarg(&v[1]) >= 28) deco_globo(vmeio(&v[1]), 78);
    } else {                                                         // o Dourado: os caca-niqueis
        if (nv > 1 && vlarg(&v[1]) >= 40) deco_niqueis(vmeio(&v[1]), FR_CHAO + 4, 2);
    }
}

// ---- o 4º: o jardim de inverno -----------------------------------------------
static void deco_folhas(int x0, int x1, int y0, int y1, int semente)
{
    for (int x = x0; x < x1; x++)                                    // a trepadeira: folhinhas pela trelica
        for (int y = y0; y < y1; y++) {
            uint32_t h = ruido_xy(x / 2 + semente, y / 2);
            float q = (float)(y - y0) / (y1 - y0);
            if (h % 7 < (unsigned)(q < 0.3f ? 4 : 2)) cp(x, y, CM_VERDE, (h >> 8) % 3 == 0 ? 3 : 2);
        }
}
static void deco_janela_arco(int cx, int y, int w, int h)
{
    int x = cx - w / 2, r = w / 2;
    for (int j = -2; j < h + 2; j++)
        for (int i = -2; i < w + 2; i++) {
            float dx = i - w / 2 + 0.5f, dy = j - r;
            bool dentro = j >= r || dx * dx + dy * dy <= r * r;
            bool aro = !dentro && (j >= r - 2 || dx * dx + dy * dy <= (r + 2) * (r + 2));
            if (aro) { cp(x + i, y + j, CM_MARF, i < w / 2 ? 3 : 2); continue; }
            if (!dentro || i < 0 || i >= w || j >= h) continue;
            float q = (float)j / h;
            int t = q < 0.5f ? 0 : 1;
            if (q > 0.45f && q < 0.55f && bayer(x + i, y + j) < 0.5f) t = 1;
            if (ruido_xy(x + i, y + j) % 61 == 0 && q < 0.6f) t = 4;  // as estrelas
            int m = CM_CEU;
            if (i == w / 2 || j == h / 2 + 4) { m = CM_MARF; t = 2; }  // o caixilho
            cp(x + i, y + j, m, t);
        }
    cdisco(x + w / 4 + 1, y + r - 2, 3, CM_CEU, 4);                // a lua
    cp(x + w / 4, y + r - 3, CM_CEU, 5);
    chl(x - 4, y + h + 2, w + 8, CM_MARF, 3); chl(x - 4, y + h + 3, w + 8, CM_MARF, 1);   // o peitoril
    cluz(cx, FR_CHAO + 16, w, 14, 0.9f);                            // o luar no chao
}
static void deco_fonte(int cx, int yb)
{
    for (int j = 0; j < 11; j++) {                                   // a bacia grande, de marmore
        int hw = 17 - j / 2;
        for (int i = -hw - 1; i <= hw + 1; i++) cp(cx + i, yb - 11 + j, i == -hw - 1 || i == hw + 1 ? CM_NEU : CM_MARF, i == -hw - 1 || i == hw + 1 ? 0 : j == 0 ? 5 : i < -hw / 2 ? 4 : i < hw / 2 ? 3 : 2);
    }
    for (int i = -15; i <= 15; i++) { cp(cx + i, yb - 10, CM_AGUA, (i & 3) ? 3 : 4); cp(cx + i, yb - 9, CM_AGUA, 2); }
    crect(cx - 2, yb - 30, 5, 19, CM_MARF, 3); cvl(cx - 2, yb - 30, 19, CM_MARF, 4); cvl(cx + 2, yb - 30, 19, CM_MARF, 2);   // a coluna
    for (int j = 0; j < 5; j++) {                                    // a taca de cima
        int hw = 8 - j;
        for (int i = -hw; i <= hw; i++) cp(cx + i, yb - 34 + j, CM_MARF, j == 0 ? 5 : i < 0 ? 4 : 3);
    }
    chl(cx - 6, yb - 33, 13, CM_AGUA, 3);
    cdet(DT_FONTE, cx - 14, yb - 46, 29, 36);
}
static void deco_palmeira(int cx, int yb)
{
    for (int j = 0; j < 10; j++) {                                   // o vaso de barro
        int hw = 6 - j / 3;
        for (int i = -hw; i <= hw; i++) cp(cx + i, yb - 10 + j, CM_MAD, i < 0 ? 4 : 3);
    }
    chl(cx - 7, yb - 11, 15, CM_MAD, 4);
    cvl(cx, yb - 30, 19, CM_MAD, 2); cvl(cx + 1, yb - 30, 19, CM_MAD, 1);   // o tronco
    for (int f = 0; f < 6; f++) {                                    // as folhas em arco
        float ang = -2.8f + f * 0.62f;
        for (int k = 0; k < 18; k++) {
            float q = k / 18.0f;
            int fx = cx + (int)(cosf(ang) * k * 1.1f), fy = yb - 31 + (int)(sinf(ang) * k * 0.7f + q * q * 10);
            cp(fx, fy, CM_VERDE, 4);
            if (k > 3 && k % 2 == 0) { cp(fx, fy + 1, CM_VERDE, 3); cp(fx, fy + 2, CM_VERDE, 3); }
        }
    }
}
static void deco_lanternas(int x0, int x1, int y)
{
    for (int x = x0; x < x1; x++) {
        int yy = y + (int)(sinf((x - x0) / (float)(x1 - x0) * 3.1416f) * 6);
        cp(x, yy, CM_NEU, 0);
        if ((x - x0) % 16 == 8) {
            crect(x - 2, yy + 1, 5, 6, CM_LUZ, 2); chl(x - 1, yy + 1, 3, CM_LUZ, 3);
            cp(x - 2, yy + 1, CM_LUZ, 1); cp(x + 2, yy + 6, CM_LUZ, 1);
        }
    }
}

static void cen_quarto(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)                           // a trelica de ripas sobre a parede escura
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, u = X % 10, w = (y - CH_Y0) % 10, m = CM_NEU, t = 1;
            if (u == w || u == 9 - w) { m = CM_MAD; t = (u == w) ? 3 : 2; }
            if (y < CH_Y0 + 3) { m = CM_MARF; t = y == CH_Y0 + 1 ? 2 : 0; }
            cp(x, y, m, t);
        }
    rodape(CM_MARF, 0);
    for (int y = FR_CHAO; y < CH_Y1; y++)                           // as lajotas de pedra, desencontradas
        for (int x = CH_X0; x < CH_X1; x++) {
            int row = (int)floorf(fr_z(y) / 14), ra = (int)floorf(fr_z(y - 1) / 14);
            float d = (ruido_xy(row, 4) % 20);
            int col = (int)floorf((fr_xm(x, y) + cen_off + d) / 24), ca = (int)floorf((fr_xm(x - 1, y) + cen_off + d) / 24);
            int t = 1 + (ruido_xy(col, row) % 3 == 0);
            if (row != ra || col != ca) t = 0;
            if (y < FR_CHAO + 3 && bayer(x, y) < 0.5f) t = 0;
            cp(x, y, CM_NEU, t);
        }
    for (int k = 0; k < nv; k++) {                                   // a trepadeira nos vaos, descendo do alto
        if (vlarg(&v[k]) < 12) continue;
        deco_folhas(v[k].x0 + 2, v[k].x1 - 2, CH_Y0 + 3, CH_Y0 + 20 + (int)(ruido_xy(k, a) % 14), k * 31 + a * 7);
    }
    if (a == 0) {                                                    // o Verde: a janela em arco com a lua
        if (nv > 1 && vlarg(&v[1]) >= 44) deco_janela_arco(vmeio(&v[1]), 72, 36, 46);
        if (vlarg(&v[0]) >= 18) deco_palmeira(vmeio(&v[0]), FR_CHAO + 6);
    } else if (a == 1) {                                             // o Vermelho: a fonte
        if (nv > 1 && vlarg(&v[1]) >= 34) deco_fonte(vmeio(&v[1]), FR_CHAO + 4);
        if (nv > 2 && vlarg(&v[2]) >= 18) deco_palmeira(vmeio(&v[2]), FR_CHAO + 6);
    } else {                                                         // o Dourado: as lanternas e a palmeira
        if (nv > 1 && vlarg(&v[1]) >= 50) { deco_lanternas(v[1].x0 + 4, v[1].x1 - 4, 82); deco_palmeira(vmeio(&v[1]), FR_CHAO + 6); }
        if (vlarg(&v[0]) >= 18) deco_palmeira(vmeio(&v[0]), FR_CHAO + 6);
    }
}

// ---- o 6º: art deco, piano e cantora ------------------------------------------
// O leque de raios dourados (meio sol), com o centro em (cx, cy) e raio r.
static void deco_sol(int cx, int cy, int r, int t0)
{
    for (int j = -r; j <= 0; j++)
        for (int i = -r; i <= r; i++) {
            float d = sqrtf((float)(i * i + j * j));
            if (d > r) continue;
            float a = atan2f((float)-j, (float)i);
            int raio = (int)(a / 0.2618f);
            int t = (raio & 1) ? t0 : t0 - 1;
            if (fabsf(fmodf(a, 0.2618f) - 0.131f) > 0.115f) t = t0 + 1;   // os filetes entre os raios
            if (d < r * 0.22f) t = t0 + 2;                            // o miolo
            if (d > r - 1.2f) t = t0 + 1;
            cp(cx + i, cy + j, CM_LAT, t);
        }
}
static void deco_piano(int cx, int yb)
{
    int x = cx - 18;
    for (int i = 0; i < 40; i++) {                                    // o corpo de cauda, de lado
        float q = i / 40.0f;
        int topo = yb - 22 + (int)(q > 0.55f ? (q - 0.55f) * (q - 0.55f) * 30 : 0);
        for (int y = topo; y < yb - 12; y++) cp(x + i, y, CM_NEU, y == topo ? 5 : y == topo + 1 ? 3 : y == yb - 13 ? 2 : 1);
        if (i == 0) for (int y = topo; y < yb - 12; y++) cp(x, y, CM_NEU, 4);   // a luz de contorno, de cima e da esquerda
    }
    chl(x, yb - 12, 40, CM_LAT, 3);                                   // o friso dourado embaixo do corpo
    for (int k = 0; k < 24; k++) { cp(x + 5 + k, yb - 23 - k / 2, CM_NEU, k < 2 ? 3 : 5); cp(x + 5 + k, yb - 22 - k / 2, CM_NEU, 2); }   // a tampa aberta
    cvl(x + 24, yb - 33, 10, CM_NEU, 3);                              // o calco da tampa
    crect(x - 4, yb - 17, 6, 3, CM_MARF, 4);                          // o teclado
    for (int k = 0; k < 6; k += 2) cp(x - 4 + k, yb - 17, CM_NEU, 0);
    for (int k = 0; k < 3; k++) { cvl(x + 3 + k * 16, yb - 12, 12, CM_NEU, 3); cp(x + 3 + k * 16, yb - 1, CM_LAT, 3); }   // as pernas
    crect(x - 10, yb - 10, 8, 2, CM_NEU, 2); cvl(x - 9, yb - 8, 8, CM_NEU, 1); cvl(x - 4, yb - 8, 8, CM_NEU, 1);   // o banco
    cluz(cx, yb - 14, 34, 16, 0.6f);
}
static void deco_microfone(int cx, int yb)
{
    for (int i = -12; i <= 12; i++) { cp(cx + i, yb - 3, CM_MAD, 3); cp(cx + i, yb - 2, CM_MAD, 2); cp(cx + i, yb - 1, CM_MAD, 1); }   // o tablado
    cvl(cx, yb - 28, 25, CM_ACO, 3);
    chl(cx - 4, yb - 4, 9, CM_ACO, 2);
    crect(cx - 1, yb - 33, 3, 5, CM_ACO, 4); cp(cx, yb - 33, CM_ACO, 5);   // a capsula
    cluz(cx, yb - 20, 26, 30, 1.0f);                                  // o foco da cantora
}
// O arco escalonado dourado, o fundo do palquinho da cantora.
static void deco_arco_deco(int cx, int yb, int w, int h)
{
    for (int s2 = 0; s2 < 3; s2++) {                                 // tres degraus, de fora para dentro
        int ww = w - s2 * 8, hh = h - s2 * 5, x0 = cx - ww / 2, y0 = yb - hh;
        for (int j = 0; j < hh; j++) {
            int recuo = j < 4 ? (4 - j) * 2 : 0;
            cp(x0 + recuo, y0 + j, CM_LAT, s2 == 0 ? 4 : 3 - (s2 > 1));
            cp(x0 + ww - 1 - recuo, y0 + j, CM_LAT, s2 == 0 ? 2 : 2 - (s2 > 1));
        }
        chl(x0 + 8, y0, ww - 16, CM_LAT, s2 == 0 ? 4 : 3);
    }
    for (int j = 0; j < h - 16; j++) chl(cx - w / 2 + 13, yb - h + 15 + j, w - 26, CM_NEU, 0);   // o fundo, mais escuro
}
static void deco_arandela_deco(int x, int y)
{
    for (int k = 0; k < 3; k++) chl(x - 4 + k, y + k * 2, 9 - 2 * k, CM_LAT, 4 - k);   // os degraus
    crect(x - 1, y + 6, 3, 4, CM_LAT, 2);
    chl(x - 3, y - 1, 7, CM_LUZ, 4); chl(x - 2, y - 2, 5, CM_LUZ, 3);
    cluz(x, y - 4, 16, 18, 1.0f);
}

static void cen_sexto(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)                           // o laqueado preto, as pilastras e os leques
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, u = X % 64, m = CM_NEU, t = 1;
            if (u < 6) { m = CM_LAT; t = u == 0 ? 0 : u == 1 ? 3 : u == 4 ? 1 : 2; if (u == 2 || u == 3) t = ((y / 2) & 1) ? 2 : 3; }   // a pilastra canelada
            if (y >= 110) { m = CM_NEU; t = y == 110 ? 0 : y == 111 ? 3 : 0; if (y == 111) m = CM_LAT; }   // o friso e o rodameio escuro
            if (y < CH_Y0 + 4) { m = CM_LAT; t = y == CH_Y0 ? 0 : y == CH_Y0 + 1 ? 4 : y == CH_Y0 + 2 ? 2 : 1; }   // a cornija em degrau
            cp(x, y, m, t);
        }
    for (int X0 = 32 - cen_off % 64 - 64; X0 < CH_W + 64; X0 += 64) deco_sol(CH_X0 + X0 + 3, CH_Y0 + 16, 10, 2);   // um leque em cada painel
    rodape(CM_NEU, 0);
    chao_xadrez(CM_MARF, 0, CM_NEU, 0, 18);
    for (int y = 148; y < CH_Y1; y++)                                  // na faixa dos nomes, o xadrez se apaga
        for (int x = CH_X0; x < CH_X1; x++)
            if (cm_de(x, y) == CM_MARF && bayer(x, y) < (y - 148) / 16.0f) cp(x, y, CM_NEU, 0);
    if (a == 0) {                                                    // o Verde: o piano de cauda
        if (nv > 1 && vlarg(&v[1]) >= 60) deco_piano(vmeio(&v[1]) + 4, FR_CHAO + 8);
        if (vlarg(&v[0]) >= 16) deco_arandela_deco(vmeio(&v[0]), 92);
    } else if (a == 1) {                                             // o Vermelho: o microfone da cantora
        if (nv > 1 && vlarg(&v[1]) >= 50) deco_arco_deco(vmeio(&v[1]), FR_RODA, 46, 48);
        if (nv > 1 && vlarg(&v[1]) >= 30) deco_microfone(vmeio(&v[1]), FR_CHAO + 6);
        if (vlarg(&v[0]) >= 16) deco_arandela_deco(vmeio(&v[0]), 92);
        if (nv > 2 && vlarg(&v[2]) >= 16) deco_arandela_deco(vmeio(&v[2]), 92);
    } else if (a == 2) {                                             // o Dourado: o grande sol atras do chefe
        deco_sol(estrado_cx(), FR_RODA, 60, 2);
        if (nv > 1 && vlarg(&v[1]) >= 40) deco_quadro(vmeio(&v[1]), 96, 26, 30, 1);
    } else {                                                         // o fim do andar: o palco vazio
        int x0 = v[0].x0, x1 = v[0].x1;
        deco_coberto(x0 + (x1 - x0) / 3, FR_CHAO + 8, 46, 24);          // o piano coberto com o lencol
        deco_microfone(x0 + (x1 - x0) * 2 / 3, FR_CHAO + 6);
        cen_np = 0;                                                  // apagado: sem o foco
    }
}

// ---- o 7º: a cortina e a mesa de dados ----------------------------------------
static void deco_canhao(int cx, int yb)
{
    for (int k = -1; k <= 1; k++) for (int j = 0; j < 16; j++) cp(cx + k * j / 3, yb - 16 + j, CM_ACO, 2);   // o tripe
    for (int i = 0; i < 12; i++) for (int j = 0; j < 7; j++) cp(cx - 6 + i, yb - 22 + j - i / 3, CM_NEU, j == 0 ? 3 : 1);   // o corpo
    crect(cx + 5, yb - 27, 2, 6, CM_LUZ, 4);                          // a lente acesa
    for (int k = 0; k < 26; k++) for (int w = -k / 3; w <= k / 3; w++)   // o cone de luz na cortina
        if (bayer(cx + 8 + k, yb - 26 - k + w) < 0.18f) cp(cx + 8 + k, yb - 26 - k + w, CM_LUZ, 2);
    cluz(cx + 30, yb - 50, 22, 18, 1.0f);
}
static void deco_cordao(int x0, int x1, int yb)
{
    int n = (x1 - x0) / 26 + 1, passo = n > 1 ? (x1 - x0) / (n - 1) : 0;
    for (int k = 0; k < n; k++) {
        int px = x0 + k * passo;
        crect(px - 1, yb - 18, 3, 18, CM_LAT, 3); cvl(px - 1, yb - 18, 18, CM_LAT, 4);
        cdisco(px, yb - 20, 2, CM_LAT, 4);
        chl(px - 3, yb - 1, 7, CM_LAT, 2);
        if (k + 1 < n)                                                // o cordao de veludo, caindo entre os postes
            for (int i = 2; i < passo - 1; i++) {
                float q = (float)i / passo;
                int yy = yb - 16 + (int)(sinf(q * 3.1416f) * 6);
                cp(px + i, yy, CM_VIN, 4); cp(px + i, yy + 1, CM_VIN, 2);
            }
    }
}
static void deco_marquise(int cx, int cy)
{
    int w = 44, h = 28, x = cx - w / 2, y = cy - h / 2;
    csombra(x + 2, y + 2, w + 2, h + 2);
    crect(x, y, w, h, CM_VIN, 1);
    crect(x - 2, y - 2, w + 4, 2, CM_LAT, 3); crect(x - 2, y + h, w + 4, 2, CM_LAT, 2);
    crect(x - 2, y, 2, h, CM_LAT, 3); crect(x + w, y, 2, h, CM_LAT, 2);
    for (int k = 0; k < 2; k++) {                                     // os dois dados
        int dx = x + 10 + k * 14, dy = y + 8 + k * 2;
        crect(dx - 1, dy - 1, 12, 12, CM_NEU, 0);
        crect(dx, dy, 10, 10, CM_MARF, 4);
        cvl(dx + 9, dy, 10, CM_MARF, 3); chl(dx, dy + 9, 10, CM_MARF, 3);
        cp(dx + 2, dy + 2, CM_VIN, 3); cp(dx + 7, dy + 7, CM_VIN, 3);
        if (k) { cp(dx + 7, dy + 2, CM_VIN, 3); cp(dx + 2, dy + 7, CM_VIN, 3); cp(dx + 4, dy + 4, CM_VIN, 3); }
    }
    for (int i = 0; i < w + 4; i += 4) { cp(x - 2 + i, y - 4, CM_LUZ, 4); cp(x - 2 + i, y + h + 3, CM_LUZ, 4); }   // as lampadas em volta
    for (int j = 0; j < h; j += 4) { cp(x - 4, y + j, CM_LUZ, 4); cp(x + w + 3, y + j, CM_LUZ, 4); }
    cdet(DT_MARQUISE, x - 4, y - 4, w + 8, h + 8);
    cluz(cx, cy, 40, 30, 1.0f);
}
static void deco_luz_fantasma(int cx, int yb)
{
    for (int k = -1; k <= 1; k++) for (int j = 0; j < 8; j++) { cp(cx + k * j, yb - 8 + j, CM_ACO, 3); cp(cx + k * j + 1, yb - 8 + j, CM_ACO, 1); }   // o tripe
    cvl(cx, yb - 44, 38, CM_ACO, 3); cvl(cx + 1, yb - 44, 38, CM_ACO, 1);   // o poste
    for (int j = -6; j <= 6; j++)                                     // a gaiola de arame
        for (int i = -5; i <= 5; i++) {
            float e = i * i / 25.0f + j * j / 36.0f;
            if (e > 1.05f || (e < 0.6f && (i + 5) % 3 && (j + 6) % 4)) continue;
            cp(cx + i, yb - 52 + j, CM_ACO, i < 0 ? 4 : 3);
        }
    cdisco(cx, yb - 52, 3, CM_LUZ, 4); cdisco(cx - 1, yb - 53, 1, CM_LUZ, 5);   // a lampada nua, acesa
    cluz(cx, yb - 46, 56, 50, 1.5f);
}

static void cen_setimo(int a, const vao_t *v, int nv)
{
    static const int LARG[5] = { 14, 20, 11, 18, 15 };
    for (int y = CH_Y0; y < FR_RODA; y++)                           // a cortina de pregas, do teto ao chao
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, pos = X % 78, k = 0, acc = 0;
            while (k < 5 && pos >= acc + LARG[k]) acc += LARG[k++];
            float u = (pos - acc + 0.5f) / LARG[k < 5 ? k : 4];
            float c = cosf(u * 6.2832f);
            int t = c > 0.5f ? 2 : 1;
            if (u < 0.08f && bayer(x, y) < 0.5f) t = 2;
            if (c < -0.7f) t = bayer(x, y) < 0.6f ? 0 : 1;
            if (y > FR_RODA - 6) t -= bayer(x, y) < 0.5f;              // a barra embolada no chao
            cp(x, y, CM_VIN, t);
        }
    for (int x = CH_X0; x < CH_X1; x++) {                            // a sanefa com a franja dourada
        int X = x - CH_X0 + cen_off, fundo = CH_Y0 + 6 + (int)(fabsf(sinf(X * 0.06f)) * 4);
        for (int y = CH_Y0; y < fundo; y++) cp(x, y, CM_VIN, y == CH_Y0 ? 0 : y == fundo - 1 ? 2 : 3);
        cp(x, fundo, CM_LAT, 3); if (x & 1) cp(x, fundo + 1, CM_LAT, 2);
    }
    rodape(CM_VIN, 0);
    chao_carpete(CM_VIN, 1, 0);                                      // o carpete vinho, com a barra dourada perto da parede
    chl(CH_X0, FR_CHAO + 4, CH_W, CM_LAT, 2); chl(CH_X0, FR_CHAO + 7, CH_W, CM_LAT, 1);
    if (a == 0) {                                                    // o Verde: o canhao de luz
        if (vlarg(&v[0]) >= 20) deco_canhao(vmeio(&v[0]) - 4, FR_CHAO + 6);
        if (nv > 1 && vlarg(&v[1]) >= 44) deco_cordao(v[1].x0 + 6, v[1].x1 - 6, FR_CHAO + 8);
    } else if (a == 1) {                                             // o Vermelho: o letreiro de lampadas
        if (nv > 1 && vlarg(&v[1]) >= 52) deco_marquise(vmeio(&v[1]), 96);
    } else if (a == 2) {                                             // o Dourado: o cordao de veludo da entrada do chefe
        if (nv > 1 && vlarg(&v[1]) >= 44) deco_cordao(v[1].x0 + 6, v[1].x1 - 6, FR_CHAO + 8);
        if (nv > 2 && vlarg(&v[2]) >= 20) deco_canhao(vmeio(&v[2]) - 4, FR_CHAO + 6);
    } else {                                                         // o fim: o teatro vazio, so a luz de servico
        int x0 = v[0].x0, x1 = v[0].x1;
        deco_luz_fantasma(x0 + (x1 - x0) / 2, FR_CHAO + 14);
        deco_cordao(x0 + 20, x0 + 74, FR_CHAO + 8);
    }
}

// ---- o 8º: o lounge do sax ---------------------------------------------------
static void deco_sofa(int cx, int yb, int w)
{
    int x = cx - w / 2;
    csombra(x + 2, yb - 20, w + 2, 20);
    for (int j = 0; j < 16; j++)                                      // o encosto capitone, alto
        for (int i = 4; i < w - 4; i++) {
            int t = 3;
            if ((i % 6 == 3) && (j % 6 == 3)) t = 1;                 // os botoes
            else if (((i + j) % 6 == 0 || (i - j + 60) % 6 == 0) && j > 1) t = 2;   // os gomos
            if (j == 0) t = 4;
            cp(x + i, yb - 26 + j, CM_VIN, t);
        }
    for (int s2 = 0; s2 < 2; s2++) {                                  // os bracos enrolados
        int bx = s2 ? x + w - 6 : x;
        for (int j = 0; j < 16; j++) for (int i = 0; i < 6; i++) cp(bx + i, yb - 18 + j, CM_VIN, j < 2 || (s2 == 0 && i == 0) ? 4 : 3);
    }
    crect(x + 4, yb - 10, w - 8, 4, CM_VIN, 3); chl(x + 4, yb - 10, w - 8, CM_VIN, 4);   // o assento
    crect(x, yb - 6, w, 3, CM_VIN, 2);
    cvl(x + 2, yb - 3, 3, CM_MAD, 1); cvl(x + w - 3, yb - 3, 3, CM_MAD, 1);
}
static void deco_abajur(int cx, int yb)
{
    cvl(cx, yb - 34, 32, CM_LAT, 2);
    chl(cx - 4, yb - 2, 9, CM_LAT, 2);
    for (int j = 0; j < 8; j++) {                                     // a cupula com a franja
        int hw = 4 + j / 2;
        for (int i = -hw; i <= hw; i++) cp(cx + i, yb - 44 + j, CM_LUZ, j == 7 ? 2 : i < 0 ? 4 : 3);
    }
    for (int i = -7; i <= 7; i += 2) cvl(cx + i, yb - 36, 2, CM_LUZ, 2);
    cluz(cx, yb - 36, 30, 26, 1.2f);
}
static void deco_sax(int cx, int yb)
{
    for (int i = -16; i <= 16; i++) { cp(cx + i, yb - 4, CM_MAD, 3); cp(cx + i, yb - 3, CM_MAD, 2); cp(cx + i, yb - 2, CM_MAD, 1); }   // o tablado
    for (int k = 0; k < 26; k++) {                                    // o corpo do sax, em pe, alargando para baixo
        int g = 1 + k / 9;
        for (int i = 0; i <= g; i++) cp(cx - 4 + k / 8 + i, yb - 40 + k, CM_LAT, i == 0 ? 5 : i == g ? 2 : 3);
    }
    for (int k = 0; k < 9; k++) for (int i = 0; i < 4; i++) cp(cx - 1 + k, yb - 14 + (k < 4 ? k / 2 : 2 - (k - 4) / 2) + i - 1, CM_LAT, i == 0 ? 4 : 3);   // a curva de baixo
    crect(cx + 6, yb - 24, 5, 9, CM_LAT, 3); cvl(cx + 6, yb - 24, 9, CM_LAT, 4); chl(cx + 5, yb - 24, 7, CM_LAT, 5);   // a campana para cima
    for (int k = 0; k < 4; k++) cp(cx - 3 + k, yb - 26 + k * 3, CM_MARF, 4);   // as chaves
    for (int k = 0; k < 6; k++) cp(cx - 5 - k / 2, yb - 41 - k, CM_NEU, 3);    // o tudel
    cvl(cx - 1, yb - 12, 8, CM_NEU, 2); chl(cx - 4, yb - 5, 8, CM_NEU, 2);      // o suporte
    cvl(cx + 11, yb - 24, 20, CM_NEU, 2); crect(cx + 7, yb - 28, 9, 6, CM_NEU, 2);   // a estante de partitura
    chl(cx + 8, yb - 27, 7, CM_MARF, 3);
    cluz(cx, yb - 22, 26, 26, 1.0f);
}
static void deco_discos(int cx, int cy)
{
    for (int k = -1; k <= 1; k++) {
        int dx = cx + k * 18, w = 14;
        crect(dx - w / 2, cy - w / 2, w, w, CM_NEU, 0);
        cdisco(dx, cy, 6, CM_NEU, 1);
        cdisco(dx, cy, 2, k == 0 ? CM_VIN : k < 0 ? CM_LAT : CM_AZ, 3);
        cp(dx - 3, cy - 3, CM_NEU, 3);
        cmoldura(dx - w / 2, cy - w / 2, w, w, CM_LAT);
    }
}
static void deco_vela(int cx, int yb)
{
    cdisco(cx, yb - 12, 6, CM_MAD, 2); chl(cx - 6, yb - 13, 13, CM_MAD, 3);   // a mesinha
    cvl(cx, yb - 12, 12, CM_MAD, 1); chl(cx - 4, yb - 1, 9, CM_MAD, 1);
    crect(cx - 1, yb - 19, 3, 5, CM_MARF, 3);
    cp(cx, yb - 21, CM_LUZ, 4); cp(cx, yb - 20, CM_LUZ, 3);
    cdet(DT_VELA, cx - 1, yb - 23, 3, 4);
    cluz(cx, yb - 18, 24, 18, 1.2f);
}

static void cen_oitavo(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)                           // as listras roxas e o lambri escuro
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, m = CM_ROX, t = ((X / 7) & 1) ? 2 : 1;
            if (y >= 106) { m = CM_NEU; t = (X % 32 == 0) ? 0 : 1; if (y == 106) { m = CM_LAT; t = 3; } if (y == 107) { m = CM_LAT; t = 1; } }
            if (y < CH_Y0 + 3) { m = CM_NEU; t = y == CH_Y0 + 1 ? 2 : 0; }
            cp(x, y, m, t);
        }
    rodape(CM_NEU, 0);
    chao_carpete(CM_ROX, 1, 2);
    if (a == 0) {                                                    // o Verde: o sofa capitone e o abajur
        if (nv > 1 && vlarg(&v[1]) >= 56) deco_sofa(vmeio(&v[1]), FR_CHAO + 6, 50);
        if (vlarg(&v[0]) >= 18) deco_abajur(vmeio(&v[0]), FR_CHAO + 6);
    } else if (a == 1) {                                             // o Vermelho: o sax no palquinho
        if (nv > 1 && vlarg(&v[1]) >= 34) deco_sax(vmeio(&v[1]), FR_CHAO + 6);
        if (nv > 2 && vlarg(&v[2]) >= 18) deco_abajur(vmeio(&v[2]), FR_CHAO + 6);
    } else if (a == 2) {                                             // o Dourado: os discos na parede
        if (nv > 1 && vlarg(&v[1]) >= 54) deco_discos(vmeio(&v[1]), 90);
        if (vlarg(&v[0]) >= 18) deco_abajur(vmeio(&v[0]), FR_CHAO + 6);
    } else {                                                         // o fim: o lounge vazio, a vela acesa
        int x0 = v[0].x0, x1 = v[0].x1;
        deco_sofa(x0 + (x1 - x0) / 4, FR_CHAO + 6, 50);
        deco_abajur(x0 + (x1 - x0) / 4 + 36, FR_CHAO + 6);
        cen_np--;                                                    // o abajur apagado: so a vela acende
        for (int y = FR_CHAO - 40; y < FR_CHAO - 30; y++) for (int x = x0 + (x1 - x0) / 4 + 28; x < x0 + (x1 - x0) / 4 + 45; x++)
            if (cm_de(x, y) == CM_LUZ) cp(x, y, CM_MAD, 1);
        deco_vela(x0 + (x1 - x0) * 3 / 5, FR_CHAO + 8);
        deco_sax(x0 + (x1 - x0) * 4 / 5, FR_CHAO + 6);
        cen_np--;                                                    // e o sax no escuro
    }
}

// ---- o 9º: o aquario e o poquer ----------------------------------------------
static void deco_aquario(int cx, int y, int w, int h, bool aceso)
{
    int x = cx - w / 2;
    csombra(x + 2, y + 2, w + 2, h + 14);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            float q = (float)j / h;
            int t = aceso ? (q < 0.3f ? 2 : 1) : (q < 0.5f ? 1 : 0);
            if (aceso && q < 0.75f && bayer(x + i, y + j) < 0.25f) t--;
            int m = CM_AQUA;
            if (j > h - 5) { m = CM_MARF; t = aceso ? 3 : 1; if (ruido_xy(i, j) % 4 == 0) t--; }   // a areia
            float pl = fabsf(i - w * 0.25f) + fabsf(i - w * 0.8f);       // as plantas, em silhueta
            if (j > h - 18 && ((i % 7 == 2 && j > h - 16 + (i % 3) * 3) || (i % 11 == 5 && j > h - 12))) { m = CM_AQUA; t = 1; }
            (void)pl;
            cp(x + i, y + j, m, t);
        }
    crect(x - 2, y - 4, w + 4, 4, CM_NEU, 1); chl(x - 2, y - 4, w + 4, CM_NEU, 3);   // a tampa
    crect(x - 2, y + h, w + 4, 12, CM_MAD, 2); chl(x - 2, y + h, w + 4, CM_MAD, 4);  // o movel de madeira
    for (int i = x + 4; i < x + w - 4; i += 12) cvl(i, y + h + 3, 7, CM_MAD, 1);
    cvl(x - 2, y, h, CM_ACO, 3); cvl(x + w + 1, y, h, CM_ACO, 2);
    if (aceso) { cdet(DT_PEIXES, x, y, w, h - 5); cluz(cx, y + h + 6, w, 16, 0.9f); }
}
static void deco_escotilhas(int cx, int cy)
{
    for (int k = -1; k <= 1; k++) {
        int ex = cx + k * 18;
        cdisco(ex, cy, 8, CM_LAT, 3);
        cdisco(ex - 1, cy - 1, 7, CM_LAT, 4);
        cdisco(ex, cy, 6, CM_AQUA, 2);
        cdisco(ex - 1, cy - 2, 3, CM_AQUA, 3);
        for (int a = 0; a < 6; a++) cp(ex + (int)(cosf(a * 1.047f) * 7.5f), cy + (int)(sinf(a * 1.047f) * 7.5f), CM_LAT, 1);   // os parafusos
    }
}
// O quadrinho com as pilhas de fichas de poquer.
static void deco_fichas(int cx, int cy)
{
    int w = 22, h = 20, x = cx - w / 2, y = cy - h / 2;
    csombra(x + 1, y + 2, w + 4, h + 4);
    crect(x, y, w, h, CM_VERDE, 2);
    static const int8_t M[3] = { CM_VIN, CM_AZ, CM_MARF };
    for (int k = 0; k < 3; k++) {
        int px = x + 4 + k * 6, n = 3 + k;
        for (int f = 0; f < n; f++) {
            int py = y + h - 4 - f * 2;
            chl(px - 1, py, 5, M[k], M[k] == CM_MARF ? 4 : 3);
            chl(px - 1, py + 1, 5, M[k], M[k] == CM_MARF ? 2 : 2);
            cp(px + 1, py, CM_MARF, 5);
        }
    }
    cmoldura(x, y, w, h, CM_LAT);
}
static void deco_coluna(int cx, int yb)
{
    int w = 18, h = 56, x = cx - w / 2, y = yb - h - 6;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            float dx = (i - w / 2 + 0.5f) / (w / 2.0f);
            int t = 1 + (fabsf(dx) < 0.3f) - (fabsf(dx) > 0.85f);
            cp(x + i, y + j, CM_AQUA, t);
        }
    crect(x - 2, y - 3, w + 4, 3, CM_LAT, 3); crect(x - 2, yb - 6, w + 4, 6, CM_LAT, 2); chl(x - 2, yb - 6, w + 4, CM_LAT, 4);
    cdet(DT_BOLHAS, x, y, w, h);
    cluz(cx, yb - 30, 22, 36, 0.9f);
}

static void cen_nono(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)                           // o azul-noite com os losangos discretos
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, u = X % 12, w = (y - CH_Y0) % 12, m = CM_AZ, t = 1;
            if (abs(u - 6) + abs(w - 6) == 6) t = 0;
            if (y >= 112) { m = CM_MAD; t = y == 112 ? 3 : 1; }      // o lambri de madeira
            if (y < CH_Y0 + 3) { m = CM_LAT; t = y == CH_Y0 + 1 ? 3 : 0; }
            cp(x, y, m, t);
        }
    rodape(CM_MAD, 0);
    chao_carpete(CM_AZ, 0, 3);
    if (a == 0) {                                                    // o Verde: o aquario comprido
        if (nv > 1 && vlarg(&v[1]) >= 52) deco_aquario(vmeio(&v[1]), 84, 46, 30, true);
    } else if (a == 1) {                                             // o Vermelho: as escotilhas
        if (nv > 1 && vlarg(&v[1]) >= 54) deco_escotilhas(vmeio(&v[1]), 94);
        if (nv > 2 && vlarg(&v[2]) >= 28) deco_fichas(vmeio(&v[2]), 92);
    } else if (a == 2) {                                             // o Dourado: a coluna de agua com bolhas
        if (nv > 1 && vlarg(&v[1]) >= 24) deco_coluna(vmeio(&v[1]), FR_CHAO + 6);
        if (nv > 2 && vlarg(&v[2]) >= 24) deco_fichas(vmeio(&v[2]), 92);
    } else {                                                         // o fim: o aquario com a bomba desligada
        int x0 = v[0].x0, x1 = v[0].x1;
        deco_aquario(x0 + (x1 - x0) / 2, 84, 70, 30, false);
    }
}

// ---- o que mexe nos andares novos ---------------------------------------------
static void fr_detalhe_andar(const det_t *d, float t)
{
    switch (d->tipo) {
    case DT_GLOBO: {                                                  // os pontos de luz do globo, correndo a parede
        for (int k = 0; k < 14; k++) {
            float fx = fmodf(k * 61.0f + t * 9.0f, (float)d->w);
            int x = d->x + (int)fx, y = d->y + 6 + (int)(ruido_xy(k, 3) % (unsigned)(d->h - 10));
            for (int j = 0; j < 2; j++)
                for (int i = 0; i < 2; i++) {
                    int px = x + i, py = y + j, m = cm_de(px, py);
                    if (m < 0 || CM_EMITE(m) || py >= FR_RODA) continue;
                    gfx_pixel(px, py, (i + j) & 1 ? C_CELESTE : C_BRANCO);
                }
        }
        break;
    }
    case DT_FONTE: {                                                  // o jato e as gotas caindo
        int cx = d->x + d->w / 2, topo = d->y + 2;
        for (int k = 0; k < 5; k++) {
            float q = fmodf(t * 0.8f + k * 0.2f, 1.0f);
            for (int s2 = -1; s2 <= 1; s2 += 2) {
                int x = cx + s2 * (int)(q * 12), y = topo + 8 + (int)(q * q * 26 - q * 8);
                gfx_pixel(x, y, k & 1 ? C_CELESTE : C_BRANCO);
            }
        }
        gfx_pixel(cx, topo + 4, C_BRANCO); gfx_pixel(cx, topo + 3, C_CELESTE);
        break;
    }
    case DT_MARQUISE: {                                               // as lampadas em volta, correndo devagar
        int fase = (int)(t * 3);
        for (int j = 0; j < d->h; j++)
            for (int i = 0; i < d->w; i++) {
                int x = d->x + i, y = d->y + j;
                if (cm_de(x, y) != CM_LUZ) continue;
                if (((i + j) / 4 + fase) % 3 == 0) gfx_pixel(x, y, CEN_RAMPA[CM_LUZ][2]);
            }
        break;
    }
    case DT_PEIXES: {                                                 // o cardume de tres e um peixe sozinho
        float c = fmodf(t * 6.0f, (float)(d->w + 30)) - 15;
        for (int k = 0; k < 3; k++) {
            int x = d->x + (int)c - k * 6, y = d->y + 9 + k * 3 + (int)(sinf(t * 2 + k) * 1.5f);
            if (x > d->x + 1 && x < d->x + d->w - 2) peixe(x, y, 1, C_FOGO, C_AMARELO);
        }
        float c2 = fmodf(t * 4.0f + 40, (float)(d->w + 20)) - 10;
        int x2 = d->x + d->w - (int)c2, y2 = d->y + d->h - 8;
        if (x2 > d->x + 2 && x2 < d->x + d->w - 2) peixe(x2, y2, -1, C_AMARELO, C_BRANCO);
        break;
    }
    case DT_BOLHAS: {
        for (int k = 0; k < 6; k++) {
            float q = fmodf(t * 0.35f + k * 0.17f, 1.0f);
            int x = d->x + 3 + (int)(ruido_xy(k, 1) % (unsigned)(d->w - 6)) + (int)(sinf(t * 3 + k) * 1.2f);
            int y = d->y + d->h - 2 - (int)(q * (d->h - 4));
            gfx_pixel(x, y, C_BRANCO);
            if (k & 1) gfx_pixel(x + 1, y, C_CELESTE);
        }
        break;
    }
    case DT_VELA: {                                                   // a chama tremendo
        int x = d->x + 1, y = d->y + 1, f = (int)(t * 7) % 3;
        gfx_pixel(x + (f == 1 ? 1 : f == 2 ? -1 : 0), y, C_AMARELO);
        gfx_pixel(x, y + 1, C_BRANCO);
        break;
    }
    default: break;
    }
}
