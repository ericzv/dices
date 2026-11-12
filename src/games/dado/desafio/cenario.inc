// ===========================================================================
// O salao visto de frente: a parede do andar ao fundo, o rodape,
// o chao em perspectiva, o tapete na frente (vindo da porta, na ponta
// esquerda da parede) e o estrado do chefe a direita. Cada andar do predio
// tem um cenario comprido e cada salao e um trecho dele: o Verde a esquerda,
// o Dourado a direita, o do Coringa no fim, apagado e roxo. As janelas
// mostram a mesma cidade de outro angulo (o longe anda menos que o perto);
// as paredes sem janela trocam as decoracoes. Sobre o salao, a luminaria de
// cada mesa, na cor do salao.
//
// O fundo fica guardado como material e tom de cada pixel (cen_m, cen_t); a
// luz das luminarias sobe e desce o tom dentro da rampa do material, com o
// pontilhado na passagem (como o chao da planta). Os materiais de luz
// propria (o corredor, o ceu, as janelas da cidade, o neon) nao escurecem.
// ===========================================================================
#define FR_RODA    128                  // o rodape: a parede vai de CH_Y0 ate aqui
#define FR_CHAO    132                  // o chao comeca junto da parede
#define FR_EST_Y   188                  // a quina de latao do estrado (abaixo dos nomes do chefe)
#define FR_EST_Y1  193                  // o pe do espelho do estrado
#define FR_PX0     33                   // a porta, na ponta esquerda da parede
#define FR_PX1     63
#define FR_PY0     82
#define FR_VAO0    38                   // o vao aberto
#define FR_VAO1    58

enum { CM_NEU, CM_MAD, CM_LAT, CM_VIN, CM_MARF, CM_AZ, CM_ROX, CM_AGUA, CM_ACO, CM_VERDE,
       CM_LUZ, CM_CEU, CM_JAN, CM_NEON, CM_AQUA, CM_N };
#define CM_EMITE(m) ((m) >= CM_LUZ)
static const uint16_t CEN_RAMPA[CM_N][6] = {
    { RGB(14, 14, 12), RGB(24, 22, 20), RGB(36, 32, 29), RGB(62, 56, 50), RGB(96, 88, 80), RGB(128, 120, 112) },
    { RGB(14, 14, 12), RGB(56, 32, 20), RGB(78, 46, 30), RGB(120, 78, 50), RGB(156, 106, 64), RGB(190, 156, 86) },
    { RGB(24, 22, 20), RGB(56, 32, 20), RGB(106, 86, 44), RGB(150, 122, 62), RGB(190, 156, 86), RGB(228, 194, 106) },
    { RGB(14, 14, 12), RGB(66, 18, 24), RGB(118, 30, 38), RGB(150, 62, 80), RGB(190, 56, 60), RGB(208, 98, 108) },
    { RGB(36, 32, 29), RGB(62, 56, 50), RGB(96, 88, 80), RGB(166, 154, 132), RGB(204, 194, 172), RGB(238, 230, 212) },
    { RGB(14, 14, 12), RGB(26, 34, 62), RGB(56, 86, 150), RGB(96, 134, 196), RGB(168, 204, 230), RGB(252, 248, 236) },
    { RGB(14, 14, 12), RGB(54, 36, 74), RGB(102, 74, 138), RGB(150, 112, 160), RGB(206, 180, 218), RGB(252, 248, 236) },
    { RGB(24, 22, 20), RGB(26, 34, 62), RGB(72, 150, 138), RGB(96, 182, 198), RGB(168, 204, 230), RGB(252, 248, 236) },
    { RGB(14, 14, 12), RGB(36, 32, 29), RGB(70, 76, 88), RGB(104, 112, 124), RGB(140, 150, 164), RGB(184, 194, 208) },
    { RGB(14, 14, 12), RGB(24, 22, 20), RGB(36, 32, 29), RGB(88, 152, 84), RGB(130, 204, 112), RGB(252, 248, 236) },
    // os de luz propria
    { RGB(56, 32, 20), RGB(120, 78, 50), RGB(214, 112, 44), RGB(238, 162, 74), RGB(246, 222, 132), RGB(252, 248, 236) },
    { RGB(14, 14, 12), RGB(26, 34, 62), RGB(54, 36, 74), RGB(56, 86, 150), RGB(166, 154, 132), RGB(252, 248, 236) },
    { RGB(14, 14, 12), RGB(106, 86, 44), RGB(150, 122, 62), RGB(190, 156, 86), RGB(228, 194, 106), RGB(246, 222, 132) },
    { RGB(66, 18, 24), RGB(150, 62, 80), RGB(208, 98, 108), RGB(226, 164, 168), RGB(252, 248, 236), RGB(252, 248, 236) },
    { RGB(14, 14, 12), RGB(26, 34, 62), RGB(72, 150, 138), RGB(96, 182, 198), RGB(168, 204, 230), RGB(252, 248, 236) },
};

// O tom mais alto que a luz leva cada material (as cores fortes nao estouram na poca).
static const uint8_t CEN_TETO[CM_N] = { 5, 5, 5, 4, 5, 3, 3, 4, 5, 4, 5, 5, 5, 5, 5 };
static uint8_t cen_m[CH_H][CH_W], cen_t[CH_H][CH_W];
static int cen_chave = -1, cen_off, cen_andar;
// O que mexe, desenhado a cada quadro por cima do guardado.
enum { DT_FAROL, DT_GLOBO, DT_FONTE, DT_MARQUISE, DT_PEIXES, DT_BOLHAS, DT_VELA, DT_LETREIRO };
typedef struct { uint8_t tipo; int16_t x, y, w, h; } det_t;
#define CEN_DET 6
static det_t cen_det[CEN_DET];
static int cen_nd;
static void cdet(int tipo, int x, int y, int w, int h)
{
    if (cen_nd < CEN_DET) cen_det[cen_nd++] = (det_t){ (uint8_t)tipo, (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h };
}
// As luzes pequenas do cenario (arandelas, a TV, o neon): pocas fixas.
#define CEN_POCAS 6
static poca_t cen_pc[CEN_POCAS];
static int cen_np;

static void cp(int x, int y, int m, int t)
{
    if (x < CH_X0 || x >= CH_X1 || y < CH_Y0 || y >= CH_Y1) return;
    cen_m[y - CH_Y0][x - CH_X0] = (uint8_t)m;
    cen_t[y - CH_Y0][x - CH_X0] = (uint8_t)(t < 0 ? 0 : t > 5 ? 5 : t);
}
static int cm_de(int x, int y) { return x < CH_X0 || x >= CH_X1 || y < CH_Y0 || y >= CH_Y1 ? -1 : cen_m[y - CH_Y0][x - CH_X0]; }
static void crect(int x, int y, int w, int h, int m, int t) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) cp(x + i, y + j, m, t); }
static void chl(int x, int y, int w, int m, int t) { crect(x, y, w, 1, m, t); }
static void cvl(int x, int y, int h, int m, int t) { crect(x, y, 1, h, m, t); }
static void cdisco(int cx, int cy, int r, int m, int t)
{
    for (int j = -r; j <= r; j++)
        for (int i = -r; i <= r; i++)
            if (i * i + j * j <= r * r + r / 2) cp(cx + i, cy + j, m, t);
}
static void cluz(int x, int y, int rx, int ry, float s)
{
    if (cen_np < CEN_POCAS) cen_pc[cen_np++] = (poca_t){ x, y, rx, ry, s };
}
// Uma moldura de 3 px (o contorno, a luz em cima e a esquerda, a sombra
// embaixo e a direita) em volta de (x, y, w, h).
static void cmoldura(int x, int y, int w, int h, int m)
{
    chl(x - 3, y - 3, w + 6, CM_NEU, 0); chl(x - 3, y + h + 2, w + 6, CM_NEU, 0);
    cvl(x - 3, y - 3, h + 6, CM_NEU, 0); cvl(x + w + 2, y - 3, h + 6, CM_NEU, 0);
    chl(x - 2, y - 2, w + 4, m, 4); cvl(x - 2, y - 2, h + 4, m, 4);
    chl(x - 2, y + h + 1, w + 4, m, 2); cvl(x + w + 1, y - 2, h + 4, m, 2);
    chl(x - 1, y - 1, w + 2, m, 3); cvl(x - 1, y - 1, h + 2, m, 3);
    chl(x - 1, y + h, w + 2, m, 1); cvl(x + w, y - 1, h + 2, m, 1);
    cp(x - 2, y - 2, m, 5);
}
// A sombra de uma peca na parede: um tom abaixo, embaixo e a direita.
static void csombra(int x, int y, int w, int h)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int px = x + i, py = y + j, m = cm_de(px, py);
            if (m < 0 || CM_EMITE(m)) continue;
            int t = cen_t[py - CH_Y0][px - CH_X0];
            if (t > 0) cen_t[py - CH_Y0][px - CH_X0] = (uint8_t)(t - 1);
        }
}

// O chao em perspectiva: a escala da linha (0.72 junto da parede, 1 na
// frente), a profundidade e o x no chao do mundo.
static float fr_esc(int y) { return 0.72f + 0.28f * (y - FR_CHAO) / (float)(CH_Y1 - FR_CHAO); }
static float fr_z(int y) { return 571.0f * logf(fr_esc(y) / 0.72f); }
static float fr_xm(int x, int y) { return 320 + (x - 320) / fr_esc(y); }

// Os vaos da parede: entre a porta, os retratos e o estrado.
typedef struct { int x0, x1; } vao_t;
static int fr_vaos(vao_t *v)
{
    int n = mesas_do_salao(), nv = 0, x = FR_PX1 + 3;
    for (int p = 0; p < n; p++) {
        if (salao_chefe(p)) continue;
        int hw = 56, cx = salao_x(p);                       // as cantoneiras da selecao vao a 50 px: 6 de folga
        v[nv++] = (vao_t){ x, cx - hw };
        x = cx + hw;
    }
    v[nv++] = (vao_t){ x, estrado_cx() - ESTRADO_W / 2 - 3 };
    return nv;
}
static int vmeio(const vao_t *v) { return (v->x0 + v->x1) / 2; }
static int vlarg(const vao_t *v) { return v->x1 - v->x0; }

// ---- pecas que servem a mais de um andar ------------------------------------
// Um quadro numa moldura dourada: paisagem (0), retrato (1), natureza-morta (2).
static void deco_quadro(int cx, int cy, int w, int h, int tipo)
{
    int x = cx - w / 2, y = cy - h / 2;
    csombra(x + 1, y + 2, w + 4, h + 4);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int m = CM_MAD, t = 1;
            float u = (i + 0.5f) / w, q = (j + 0.5f) / h;
            if (tipo == 0) {                              // o ceu, o morro e o rio
                float morro = 0.55f + 0.12f * sinf(u * 5.1f + 0.7f) - 0.1f * u;
                if (q < morro) { m = CM_AZ; t = q < 0.25f ? 1 : 2; if (q > 0.3f && q < 0.36f && u > 0.2f && u < 0.5f) { m = CM_MARF; t = 3; } }
                else if (q > 0.8f && fabsf(u - 0.6f - (q - 0.8f)) < 0.12f) { m = CM_AZ; t = 2; }
                else { m = CM_VERDE; t = q < morro + 0.08f ? 3 : 2; }
            } else if (tipo == 1) {                       // o retrato: o fundo escuro, o busto e a cara
                float dx = (u - 0.5f) * w, dy = (q - 0.38f) * h;
                if (dx * dx / 16 + dy * dy / 25 < 1) { m = CM_MARF; t = dx < 0 ? 4 : 3; }
                else if (q > 0.62f && fabsf(u - 0.5f) < 0.36f - (1 - q) * 0.2f) { m = CM_NEU; t = 1; }
                else { m = CM_MAD; t = 1; }
            } else {                                      // a fruteira
                float dx = (u - 0.5f) * w, dy = (q - 0.62f) * h;
                if (dy > 0 && dy < 3 && fabsf(dx) < w * 0.32f) { m = CM_LAT; t = 3; }
                else if ((dx + 3) * (dx + 3) + (dy + 2) * (dy + 2) < 7) { m = CM_VIN; t = 4; }
                else if ((dx - 3) * (dx - 3) + (dy + 2) * (dy + 2) < 7) { m = CM_LAT; t = 4; }
                else if (dx * dx + (dy + 5) * (dy + 5) < 6) { m = CM_VERDE; t = 3; }
                else { m = CM_NEU; t = 1; }
            }
            cp(x + i, y + j, m, t);
        }
    cmoldura(x, y, w, h, CM_LAT);
}

// A arandela: o espelho de latao na parede, o braco e a vela acesa (com a
// sua poca de luz).
static void deco_arandela(int x, int y)
{
    crect(x - 1, y + 4, 3, 6, CM_LAT, 3);
    cp(x - 1, y + 4, CM_LAT, 4);
    cvl(x, y + 1, 3, CM_LAT, 2);
    chl(x - 2, y + 1, 5, CM_LAT, 4);
    chl(x - 1, y + 2, 3, CM_LAT, 2);
    crect(x - 1, y - 3, 3, 4, CM_MARF, 4);
    cp(x, y - 4, CM_LUZ, 4); cp(x, y - 5, CM_LUZ, 5);
    cluz(x, y, 20, 18, 1.1f);
}

// ---- o Terreo: o saguao do cassino --------------------------------------------
// O papel creme listrado, o lambri com o friso de latao e o taco em espinha.
// Verde: o quadro do torneio e o cabideiro; Vermelho: o guiche do caixa;
// Dourado: o retrato do fundador e os trofeus.

// O quadro do torneio: a cortica, os papeizinhos com os nomes e as chaves a
// tinta, ate o campeao (o percevejo de latao).
static void deco_torneio(int cx, int y)
{
    int w = 44, h = 28, x = cx - w / 2;
    csombra(x + 1, y + 2, w + 4, h + 4);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) cp(x + i, y + j, CM_MAD, ruido_xy(x + i, y + j) % 5 == 0 ? 2 : 3);
    static const int Y1[4] = { 3, 9, 16, 22 }, Y2[2] = { 6, 19 };
    for (int k = 0; k < 4; k++) {                                   // a 1a rodada: quatro duelos
        crect(x + 2, y + Y1[k], 8, 3, CM_MARF, 4); chl(x + 3, y + Y1[k] + 1, 5, CM_NEU, 2);
        cp(x + 5, y + Y1[k], CM_VIN, 4);                              // o percevejo
        chl(x + 10, y + Y1[k] + 1, 3, CM_NEU, 1);
    }
    for (int k = 0; k < 2; k++) {                                   // as chaves ate a semifinal
        cvl(x + 13, y + Y1[2 * k] + 1, Y1[2 * k + 1] - Y1[2 * k] + 1, CM_NEU, 1);
        chl(x + 13, y + Y2[k] + 1, 4, CM_NEU, 1);
        crect(x + 17, y + Y2[k], 8, 3, CM_MARF, 4); chl(x + 18, y + Y2[k] + 1, 4, CM_NEU, 2);
        cp(x + 20, y + Y2[k], CM_VERDE, 4);
        chl(x + 25, y + Y2[k] + 1, 3, CM_NEU, 1);
    }
    cvl(x + 28, y + Y2[0] + 1, Y2[1] - Y2[0] + 1, CM_NEU, 1);        // a final
    chl(x + 28, y + 13, 4, CM_NEU, 1);
    crect(x + 32, y + 12, 9, 3, CM_MARF, 5); chl(x + 33, y + 13, 5, CM_NEU, 2);
    cdisco(x + 36, y + 9, 1, CM_LAT, 4); cp(x + 36, y + 8, CM_LAT, 5);   // o campeao
    cmoldura(x, y, w, h, CM_MAD);
}
// O cabideiro de pe: a haste torneada, os ganchos, o chapeu e o sobretudo.
static void deco_cabideiro(int cx, int yb)
{
    int top = yb - 54;
    for (int k = -1; k <= 1; k++) {                                 // os tres pes
        int fx = cx + k * 6;
        chl(k < 0 ? fx : cx, yb - 1, abs(k) * 6 + 1, CM_MAD, 2);
        cp(fx, yb - 2, CM_MAD, 3);
    }
    cvl(cx - 1, top + 3, yb - top - 4, CM_MAD, 4); cvl(cx, top + 3, yb - top - 4, CM_MAD, 3);
    cvl(cx + 1, top + 3, yb - top - 4, CM_MAD, 1);
    for (int j = 0; j < 3; j++) chl(cx - 1, yb - 20 + j * 7, 2, CM_MAD, 1);   // os aneis do torneado
    cdisco(cx, top + 1, 2, CM_LAT, 3); cp(cx - 1, top, CM_LAT, 5);   // a bola de cima
    for (int s = -1; s <= 1; s += 2) {                              // os ganchos
        cp(cx + s * 2, top + 5, CM_LAT, 3); cp(cx + s * 3, top + 4, CM_LAT, 3); cp(cx + s * 4, top + 3, CM_LAT, 4);
    }
    for (int j = 0; j < 5; j++) {                                   // o chapeu no gancho da esquerda
        int hw = j < 2 ? 3 : j == 4 ? 7 : 4;
        chl(cx - 6 - hw, top + 2 + j, 2 * hw + 1, CM_NEU, j == 4 ? 2 : 1);
    }
    chl(cx - 9, top + 5, 7, CM_VIN, 3);                             // a fita
    cp(cx - 9, top + 3, CM_NEU, 3);
    for (int j = 0; j < 24; j++) {                                  // o sobretudo no da direita, caindo
        int hw = 2 + j / 6;
        for (int i = 0; i <= hw; i++) cp(cx + 3 + i, top + 5 + j, CM_MAD, i == 0 ? 2 : i == hw ? 1 : 2);
        cp(cx + 3, top + 5 + j, CM_MAD, 3);
    }
    chl(cx + 3, top + 18, 5, CM_MAD, 1);                            // o cinto
}
// O guiche do caixa: a cabine de madeira ate o chao, a janela com a grade de
// latao, o abajur verde aceso la dentro e a plaquinha em cima.
static void deco_caixa(int cx, int y)
{
    int w = 42, x = cx - w / 2, yb = FR_RODA;
    csombra(x + 1, y + 2, w + 3, yb - y);
    crect(x, y, w, yb - y, CM_MAD, 2);
    chl(x, y, w, CM_MAD, 4); cvl(x, y, yb - y, CM_MAD, 4);
    cvl(x + w - 1, y, yb - y, CM_MAD, 1);
    crect(x - 2, y - 3, w + 4, 3, CM_MAD, 3); chl(x - 2, y - 3, w + 4, CM_MAD, 4);   // a cornija
    crect(cx - 9, y + 2, 18, 5, CM_LAT, 3); chl(cx - 9, y + 2, 18, CM_LAT, 4);       // a plaquinha: CAIXA
    chl(cx - 6, y + 4, 12, CM_LAT, 1);
    int jx = x + 5, jy = y + 10, jw = w - 10, jh = 20;              // a janela: o fundo escuro e o abajur
    crect(jx, jy, jw, jh, CM_NEU, 1);
    chl(jx + 1, jy + 1, jw - 2, CM_LUZ, 3); chl(jx + 1, jy + 2, jw - 2, CM_LUZ, 1);   // a luz de dentro
    for (int k = 0; k < 4; k++) {                                   // as pilhas de fichas no balcao de dentro
        int hk = 3 + (int)(ruido_xy(k, 7) % 6), fx = jx + 3 + k * 7;
        for (int j = 0; j < hk; j++) chl(fx, jy + jh - 1 - j, 4, k == 2 ? CM_VIN : CM_LAT, (j & 1) ? 3 : 4);
    }
    for (int i = 2; i < jw; i += 4) cvl(jx + i, jy, jh, CM_LAT, 3);   // a grade
    chl(jx, jy + 6, jw, CM_LAT, 2);
    cmoldura(jx, jy, jw, jh, CM_MAD);
    crect(x - 1, jy + jh + 3, w + 2, 3, CM_MAD, 4); chl(x - 1, jy + jh + 3, w + 2, CM_MAD, 5);   // o balcao
    chl(x - 1, jy + jh + 6, w + 2, CM_MAD, 0);
    for (int k = 0; k < 2; k++) {                                   // as almofadas da frente
        int px0 = x + 4 + k * (w / 2 - 2), py0 = jy + jh + 9, pw = w / 2 - 6, ph = yb - py0 - 4;
        if (ph < 4) break;
        chl(px0, py0, pw, CM_MAD, 1); cvl(px0, py0, ph, CM_MAD, 1);
        chl(px0, py0 + ph, pw + 1, CM_MAD, 3); cvl(px0 + pw, py0, ph, CM_MAD, 3);
    }
    cluz(cx, jy + jh, 30, 16, 0.8f);
}
static void deco_relogio(int cx, int cy)
{
    csombra(cx - 7, cy - 6, 16, 16);
    cdisco(cx, cy, 8, CM_NEU, 0);
    cdisco(cx, cy, 7, CM_MARF, 3);
    cdisco(cx - 1, cy - 1, 5, CM_MARF, 4);
    cp(cx, cy - 6, CM_NEU, 1); cp(cx, cy + 6, CM_NEU, 1); cp(cx - 6, cy, CM_NEU, 1); cp(cx + 6, cy, CM_NEU, 1);
    cvl(cx, cy - 5, 5, CM_NEU, 0);
    chl(cx, cy, 4, CM_NEU, 0);
}
// A prateleira dos trofeus: a taca grande do campeao no meio e duas menores.
static void deco_trofeus(int cx, int y)
{
    int w = 44, x = cx - w / 2;
    csombra(x - 1, y + 2, w + 4, 3);
    chl(x - 2, y, w + 4, CM_MAD, 4); chl(x - 2, y + 1, w + 4, CM_MAD, 2);
    for (int k = 0; k < 2; k++) {                                   // as maos-francesas
        int bx = k ? x + w - 6 : x + 3;
        cvl(bx, y + 2, 4, CM_MAD, 1); cp(bx + 1, y + 2, CM_MAD, 1); cp(bx + 1, y + 3, CM_MAD, 1); cp(bx + 2, y + 2, CM_MAD, 1);
    }
    static const int DX[3] = { -14, 0, 14 }, H[3] = { 12, 18, 10 };
    for (int k = 0; k < 3; k++) {
        int tx = cx + DX[k], h = H[k], r = h / 3;
        crect(tx - r, y - 3, 2 * r + 1, 3, CM_MAD, 1); chl(tx - r, y - 3, 2 * r + 1, CM_MAD, 3);   // a base
        cvl(tx, y - h / 2 - 1, h / 2 - 2, CM_LAT, 3);                // a haste
        chl(tx - 1, y - 4, 3, CM_LAT, 2);
        for (int j = 0; j <= r + 1; j++) {                          // a taca
            int hw = r + 1 - j * j / (r + 2);
            for (int i = -hw; i <= hw; i++) cp(tx + i, y - h + j, CM_LAT, i < -hw / 2 ? 5 : i > hw / 2 ? 2 : 4);
        }
        cp(tx - r - 2, y - h + 1, CM_LAT, 3); cp(tx - r - 2, y - h + 2, CM_LAT, 3);   // as alcas
        cp(tx + r + 2, y - h + 1, CM_LAT, 2); cp(tx + r + 2, y - h + 2, CM_LAT, 2);
    }
    crect(cx - 5, y + 3, 11, 3, CM_LAT, 3); chl(cx - 5, y + 3, 11, CM_LAT, 4);   // a plaquinha
}

static void cen_terreo(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, m = CM_MARF, t = 3;
            if (X % 6 == 0) t = 2;                                    // a listra fina do papel
            if (y < CH_Y0 + 3) { m = CM_MAD; t = y == CH_Y0 + 1 ? 3 : 0; }   // a sanca
            else if (y < CH_Y0 + 8 && bayer(x, y) < (CH_Y0 + 8 - y) / 6.0f) t--;
            if (y >= 102) {                                           // o friso de latao e o lambri
                m = CM_MAD; t = 2;
                int u = X % 24, j = y - 106;
                if (y == 102) { m = CM_LAT; t = 4; }
                else if (y == 103) { m = CM_LAT; t = 2; }
                else if (y == 104) t = 0;
                else if (u < 2 || j < 0 || j > 17) t = u == 0 ? 1 : 2;   // o quadro do lambri
                else t = (u == 2 || j == 0) ? 1 : (u == 23 || j == 17) ? 3 : 2;   // a almofada, chanfrada
            }
            cp(x, y, m, t);
        }
    for (int y = FR_RODA; y < FR_CHAO; y++) chl(CH_X0, y, CH_W, CM_MAD, y == FR_RODA ? 3 : 1);   // o rodape
    for (int y = FR_CHAO; y < CH_Y1; y++) {                         // o taco em espinha de peixe
        float z = fr_z(y), e = fr_esc(y);
        for (int x = CH_X0; x < CH_X1; x++) {
            float xm = fr_xm(x, y) + cen_off;
            int col = (int)floorf(xm / 16);
            float u = xm - col * 16, d = (col & 1) ? u + z * 0.8f : 16 - u + z * 0.8f;
            int fila = (int)floorf(d / 8);
            float f = d - fila * 8;
            int t = 2 + (ruido_xy(col, fila) % 5 == 0 && bayer(x, y) < 0.5f);
            if (f < 0.6f / e || u < 0.5f / e) t = 1;                 // as juntas
            if (y < FR_CHAO + 3 && bayer(x, y) < 0.5f) t = 1;
            cp(x, y, CM_MAD, t);
        }
    }
    // As decoracoes de cada salao, nos vaos.
    if (a == 0) {                                         // o Verde: o quadro do torneio e o cabideiro
        if (vlarg(&v[0]) >= 18) deco_relogio(vmeio(&v[0]), 80);
        if (nv > 1 && vlarg(&v[1]) >= 50) deco_torneio(vmeio(&v[1]), 70);
        if (nv > 2 && vlarg(&v[2]) >= 22) deco_cabideiro(vmeio(&v[2]), FR_CHAO + 8);
    } else if (a == 1) {                                  // o Vermelho: o guiche do caixa
        if (vlarg(&v[0]) >= 28) deco_quadro(vmeio(&v[0]), 84, 20, 16, 0);
        if (nv > 1 && vlarg(&v[1]) >= 48) deco_caixa(vmeio(&v[1]), 72);
        if (nv > 2 && vlarg(&v[2]) >= 20) deco_arandela(vmeio(&v[2]), 84);
    } else {                                              // o Dourado: o retrato do fundador e os trofeus
        if (vlarg(&v[0]) >= 28) deco_quadro(vmeio(&v[0]), 84, 20, 26, 1);
        if (nv > 1 && vlarg(&v[1]) >= 48) deco_trofeus(vmeio(&v[1]), 92);
        if (nv > 2 && vlarg(&v[2]) >= 22) deco_cabideiro(vmeio(&v[2]), FR_CHAO + 8);
    }
}

// ---- o 5º: ouro --------------------------------------------------------------
static void deco_espelho(int cx, int y, int w, int h)
{
    int x = cx - w / 2;
    csombra(x + 2, y + 2, w + 4, h + 4);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int t = 1, k = (i + j) % 23;
            if (k < 3 && i > 2) t = 2;                       // os riscos de luz no vidro
            if ((i + j / 2) % 31 == 0) t = 2;
            cp(x + i, y + j, CM_ACO, t);
        }
    crect(cx - 3, y + 6, 7, 3, CM_LUZ, 1);                  // a luminaria refletida, la no fundo
    cp(cx, y + 7, CM_LUZ, 3);
    cmoldura(x, y, w, h, CM_LAT);
    cdisco(cx, y - 4, 3, CM_LAT, 3);                        // o florao em cima
    cp(cx - 1, y - 5, CM_LAT, 5);
    crect(x - 4, y + h + 2, w + 8, 3, CM_MAD, 3);          // o aparador
    chl(x - 4, y + h + 2, w + 8, CM_LAT, 4);
    csombra(x - 3, y + h + 5, w + 8, 2);
}
// O cortinado atras do chefe: o bando em cima, os dois panos presos na
// abracadeira de latao.
static void deco_cortinado(int cx)
{
    for (int s = -1; s <= 1; s += 2)
        for (int y = CH_Y0 + 4; y < FR_RODA; y++) {
            float q = y < 102 ? 0 : (y - 102) / 26.0f;
            int dentro = cx + s * 36, fora = cx + s * 58;
            int borda = dentro + s * (int)(q * q * 12);
            if (y > 100 && y < 106) borda = dentro + s * (5 + (y - 100));
            int xa = s < 0 ? fora : borda, xb = s < 0 ? borda : fora;
            for (int x = xa; x <= xb; x++) {
                int k = ((x - fora) * s + 40) % 6, t = k == 0 ? 1 : k < 3 ? 3 : 2;
                if (x == xa || x == xb) t = 1;
                cp(x, y, CM_VIN, t);
            }
        }
    for (int s = -1; s <= 1; s += 2) {                      // as abracadeiras
        int bx = cx + s * 44;
        crect(bx - 3, 103, 7, 2, CM_LAT, 4);
        cp(bx + s * 4, 104, CM_LAT, 3);
        cvl(bx, 105, 4, CM_LAT, 2);
    }
    for (int x = cx - 60; x <= cx + 60; x++) {             // o bando, em ondas, com a franja
        int fundo = CH_Y0 + 8 + (int)(fabsf(sinf((x - cx) * 0.105f)) * 4);
        for (int y = CH_Y0 + 3; y < fundo; y++) cp(x, y, CM_VIN, y == fundo - 1 ? 2 : 3);
        if (x % 2 == 0) cp(x, fundo, CM_LAT, 3);
    }
}
// Os moveis cobertos com lencol, no escuro.
static void deco_coberto(int cx, int yb, int w, int h)
{
    for (int j = 0; j < h; j++) {
        float q = (float)j / h;
        int hw = (int)(w / 2 * (0.6f + 0.4f * sqrtf(q)));
        for (int i = -hw; i <= hw; i++) {
            int t = 2;
            if (i == -hw || i == hw) t = 1;
            if ((i + 50 + j / 3) % 7 == 0 && q > 0.3f) t = 1;   // as dobras
            if (i < -hw / 2 && q < 0.4f) t = 3;
            cp(cx + i, yb - h + j, CM_MARF, t);
        }
    }
    for (int i = -w / 2; i <= w / 2; i++) if (ruido_xy(i, cx) % 3 == 0) cp(cx + i, yb, CM_MARF, 1);   // a barra irregular
}

static void cen_quinto(int a, const vao_t *v, int nv)
{
    for (int y = CH_Y0; y < FR_RODA; y++)
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off, m = CM_LAT, t = 1;
            int yy = y - CH_Y0;
            if (yy < 4) t = yy == 0 ? 0 : yy == 1 ? 4 : yy == 2 ? 3 : 1;           // a cornija
            else if (y < 108) {                                  // o damasco: o losango com o florao
                int u = X % 14, w = (y - CH_Y0 - 4) % 20;
                int d = abs(u - 7) * 10 / 7 + abs(w - 10);
                if (d == 10 || (d <= 2 && w != 10)) t = 2;
                if (yy < 10 && bayer(x, y) < (10 - yy) / 7.0f) t = t ? t - 1 : 0;
            } else if (y < 110) t = y == 108 ? 4 : 2;            // o friso
            else {                                                // o lambri, em almofadas
                int u = X % 30, w = y - 110;
                m = CM_MAD; t = 2;
                if (u == 0 || w == 0 || w == 17) t = 1;
                else if (u > 3 && u < 27 && w > 3 && w < 14) t = (u == 4 || w == 4) ? 1 : (u == 26 || w == 13) ? 3 : 2;
            }
            cp(x, y, m, t);
        }
    for (int y = FR_RODA; y < FR_CHAO; y++) chl(CH_X0, y, CH_W, CM_MAD, y == FR_RODA ? 3 : 1);
    for (int y = FR_CHAO; y < CH_Y1; y++) {                 // o tapete vinho, com os pontinhos de latao
        int row = (int)floorf(fr_z(y) / 9);
        bool linha_ponto = (int)floorf(fr_z(y) / 9) != (int)floorf(fr_z(y - 1) / 9);
        for (int x = CH_X0; x < CH_X1; x++) {
            float xm = fr_xm(x, y) + cen_off + (row & 1) * 9;
            int t = 1, m = CM_VIN;
            if (linha_ponto && fabsf(xm - 18 * roundf(xm / 18)) < 0.7f / fr_esc(y)) { m = CM_LAT; t = 2; }
            if (y < FR_CHAO + 3 && m == CM_VIN) t = bayer(x, y) < 0.5f ? 0 : 1;
            cp(x, y, m, t);
        }
    }
    if (a == 0) {
        if (vlarg(&v[0]) >= 28) deco_quadro(vmeio(&v[0]), 92, 22, 18, 0);
        if (nv > 1 && vlarg(&v[1]) >= 40) {
            int c = vmeio(&v[1]);
            deco_quadro(c, 90, 24, 30, 1);
            if (vlarg(&v[1]) >= 60) { deco_arandela(c - 25, 86); deco_arandela(c + 25, 86); }
        }
        if (nv > 2 && vlarg(&v[2]) >= 30) deco_quadro(vmeio(&v[2]), 90, 22, 18, 2);
    } else if (a == 1) {
        if (vlarg(&v[0]) >= 20) deco_arandela(vmeio(&v[0]), 86);
        if (nv > 1 && vlarg(&v[1]) >= 50) {
            int c = vmeio(&v[1]);
            deco_espelho(c, 68, 32, 34);
            if (vlarg(&v[1]) >= 62) { deco_arandela(c - 27, 84); deco_arandela(c + 27, 84); }
        }
        if (nv > 2 && vlarg(&v[2]) >= 30) deco_quadro(vmeio(&v[2]), 90, 22, 26, 0);
    } else if (a == 2) {
        if (vlarg(&v[0]) >= 28) deco_quadro(vmeio(&v[0]), 90, 20, 28, 1);
        if (nv > 1 && vlarg(&v[1]) >= 40) deco_quadro(vmeio(&v[1]), 90, 30, 30, 2);
        deco_cortinado(estrado_cx());
    } else {                                                // o fim da galeria: tudo coberto
        int x0 = v[0].x0, x1 = v[0].x1;
        deco_coberto(x0 + (x1 - x0) / 4, 168, 30, 26);
        deco_coberto(x0 + (x1 - x0) * 3 / 5, 164, 44, 18);
        deco_quadro(x0 + (x1 - x0) * 2 / 5, 90, 30, 24, 0);
        for (int j = 0; j < 22; j++)                         // o pano por cima do quadro
            for (int i = -18; i <= 18 - j / 3; i++) {
                int cx = x0 + (x1 - x0) * 2 / 5 + 1;
                if (j > 14 && (i + j) % 5 == 0) continue;
                cp(cx + i, 76 + j, CM_MARF, (i + 40) % 6 == 0 ? 1 : i < -8 ? 3 : 2);
            }
    }
}

// ---- a Cobertura: o terraco de vidro -----------------------------------------
// A cidade: predios um do lado do outro, a partir do x do mundo; perto, mais
// altos e com janelas acesas; longe, mais baixos e mais claros (o ar).
#define CID_PERTO 0.12f
#define CID_LONGE 0.05f
#define CID_BAIA  520                                       // onde a cidade acaba e comeca a baia
#define CID_TORRE 230
static void cen_cidade(bool perto, int base_y)
{
    float f = perto ? CID_PERTO : CID_LONGE;
    int sx = (int)(cen_off * f);
    uint32_t s = perto ? 77 : 1234;
    for (int bx = 0; bx < sx + CH_W + 40;) {
        s = s * 1103515245u + 12345u;
        int bw = perto ? 10 + (int)((s >> 16) % 17) : 7 + (int)((s >> 16) % 12);
        int bh = perto ? 14 + (int)((s >> 8) % 30) : 6 + (int)((s >> 8) % 14);
        if (perto && bx <= CID_TORRE && bx + bw > CID_TORRE) { bw = 12; bh = 50; }
        if (perto && bx > CID_BAIA) break;
        if (!perto && bx > 330) bh = 3 + bh / 4;               // a outra margem, baixa
        int x0 = CH_X0 + bx - sx;
        for (int j = 0; j < bh; j++)
            for (int i = 0; i < bw; i++) {
                int x = x0 + i, y = base_y - bh + j;
                if (x < CH_X0 || x >= CH_X1) continue;
                int m = CM_CEU, t = perto ? 0 : 1;
                if (perto) {
                    if (i % 3 == 1 && j % 3 == 1 && j > 1 && i < bw - 1 && ruido_xy(bx + i, j) % 6 == 0) { m = CM_JAN; t = 1 + (int)(ruido_xy(bx, j + i) % 2); }
                } else if (i % 2 == 0 && j % 3 == 2 && ruido_xy(bx + i, j + 9) % 7 == 0) { m = CM_JAN; t = 1; }
                if (j == 0 && perto && (s >> 28) % 3 == 0) t = 1;   // a cumeeira que pega a luz da lua
                cp(x, y, m, t);
            }
        if (perto && bw == 12 && bh == 50) {                 // a torre: a antena e o farol
            cvl(x0 + 6, base_y - bh - 6, 6, CM_CEU, 0);
            cdet(DT_FAROL, x0 + 5, base_y - bh - 8, 3, 3);
        }
        bx += bw + (int)((s >> 24) % 3);
    }
}

static void cen_cobertura(int a, const vao_t *v, int nv)
{
    (void)nv;
    // o ceu em tres faixas, as estrelas e a lua (quase parada: muito longe)
    for (int y = CH_Y0; y < 120; y++)
        for (int x = CH_X0; x < CH_X1; x++) {
            float q = (y - CH_Y0) / 62.0f + (bayer(x, y) - 0.5f) * 0.08f;
            int t = q < 0.42f ? 0 : q < 0.74f ? 1 : 2;
            int Xs = x - CH_X0 + (int)(cen_off * 0.004f);
            uint32_t h = ruido_xy(Xs, y + 31);
            if (y < 104 && h % 170 == 0) t = (h >> 9) % 4 == 0 ? 5 : 4;
            cp(x, y, CM_CEU, t);
        }
    int lx = CH_X0 + 252 - (int)(cen_off * 0.004f), ly = 72;
    for (int j = -10; j <= 10; j++)                         // o halo da lua
        for (int i = -10; i <= 10; i++) {
            float d = sqrtf((float)(i * i + j * j));
            if (d > 6.5f && d < 10 && bayer(lx + i, ly + j) < (10 - d) / 7) cp(lx + i, ly + j, CM_CEU, 3);
        }
    cdisco(lx, ly, 6, CM_CEU, 4);
    cdisco(lx - 1, ly - 1, 4, CM_CEU, 5);
    cp(lx + 2, ly + 1, CM_CEU, 4); cp(lx - 2, ly + 2, CM_CEU, 4); cp(lx + 1, ly - 3, CM_CEU, 4);
    cen_cidade(false, 119);
    cen_cidade(true, 120);
    // a baia: a agua e a ponte estaiada, com as luzes refletidas
    int sx = (int)(cen_off * CID_PERTO);
    for (int x = CH_X0; x < CH_X1; x++) {
        int X = x - CH_X0 + sx;
        if (X < CID_BAIA + 10) continue;
        for (int y = 112; y < 120; y++) {
            int t = 1;
            if (X > CID_BAIA + 30 && X < CID_BAIA + 330 && X % 6 == 0 && (y - 112) % 2 == 0 && bayer(x, y) < 0.6f) t = 4;
            cp(x, y, CM_CEU, t);
        }
        if (X > CID_BAIA + 20 && X < CID_BAIA + 340) {        // o tabuleiro
            cp(x, 106, CM_CEU, 0); cp(x, 107, CM_CEU, 0);
            if (X % 6 == 0) cp(x, 105, CM_JAN, 4);
        }
        for (int k = 0; k < 2; k++) {                         // os dois mastros e os estais
            int mx = CID_BAIA + 90 + k * 160, dx = X - mx;
            if (dx == 0 || dx == 1) cvl(x, 84, 28, CM_CEU, 0);
            if (abs(dx) < 70 && abs(dx) > 1) {
                int yc = 86 + abs(dx) * 20 / 70;
                if (abs(dx) % 9 == 0) for (int y = yc; y < 106; y += 1) if ((y + dx) % 7 == 0) cp(x, y, CM_CEU, 0);
                cp(x, yc, CM_CEU, 0);
            }
            if (dx == 0) cp(x, 83, CM_JAN, 5);
        }
    }
    // os montantes da fachada (perto: andam junto com o salao) e os reflexos no vidro
    for (int x = CH_X0; x < CH_X1; x++) {
        int X = x - CH_X0 + cen_off, u = X % 130;
        for (int y = CH_Y0; y < 120; y++) {
            int m = cm_de(x, y), t = cen_t[y - CH_Y0][x - CH_X0];
            if (u < 2) { cp(x, y, CM_ACO, u == 0 ? 3 : 1); continue; }
            int r = (X + (y - CH_Y0) * 2) % 130;
            if (m == CM_CEU && t < 3 && r > 30 && r < 36 && bayer(x, y) < 0.5f) cp(x, y, CM_CEU, t + 1);
        }
    }
    for (int x = CH_X0; x < CH_X1; x++) {                   // a viga de cima
        cp(x, CH_Y0, CM_ACO, 0); cp(x, CH_Y0 + 1, CM_ACO, 3); cp(x, CH_Y0 + 2, CM_ACO, 2); cp(x, CH_Y0 + 3, CM_ACO, 1);
    }
    for (int y = 119; y < FR_RODA; y++)                     // o guarda-corpo: o corrimao e o vidro fosco
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0 + cen_off;
            int t = y == 119 ? 3 : y == 120 ? 2 : (X % 26 == 0 ? 2 : 1);
            cp(x, y, CM_ACO, t);
        }
    for (int y = FR_RODA; y < FR_CHAO; y++) chl(CH_X0, y, CH_W, CM_ACO, y == FR_RODA ? 2 : 0);
    for (int y = FR_CHAO; y < CH_Y1; y++)                   // o deck: tabuas no sentido do fundo
        for (int x = CH_X0; x < CH_X1; x++) {
            float xm = fr_xm(x, y) + cen_off, xa = fr_xm(x - 1, y) + cen_off;
            int k = (int)floorf(xm / 13), ka = (int)floorf(xa / 13);
            int t = 1;
            if (k != ka) t = 0;
            int fim = (int)floorf((fr_z(y) + (ruido_xy(k, 7) % 40)) / 46), fa = (int)floorf((fr_z(y - 1) + (ruido_xy(k, 7) % 40)) / 46);
            if (fim != fa) t = 0;
            if (y < FR_CHAO + 3 || (y > 176 && bayer(x, y) < (y - 176) / 30.0f)) t = 0;
            cp(x, y, CM_MAD, t);
        }
    if (a == 0 && vlarg(&v[0]) >= 24) {                     // a luneta no tripe
        int cx = vmeio(&v[0]);
        for (int k = -1; k <= 1; k++) {
            int fx = cx + k * 7;
            for (int j = 0; j < 30; j++) cp(cx + (fx - cx) * j / 30, 112 + j, CM_LAT, k ? 2 : 1);
        }
        for (int i = 0; i < 22; i++) {
            int x = cx - 10 + i, y = 113 - i * 2 / 5;
            int g = i < 6 ? 1 : 2;
            for (int j = -g; j <= g; j++) cp(x, y + j, CM_LAT, j < 0 ? 4 : j == 0 ? 3 : 2);
        }
        cp(cx + 12, 107, CM_MARF, 4);
    }
    if (a == 2) {                                            // o varal de lampadinhas na viga
        for (int x = CH_X0; x < CH_X1; x++) {
            int X = x - CH_X0, seg = X % 146;
            int y = CH_Y0 + 5 + (int)(sinf(seg / 146.0f * 3.1416f) * 9);
            cp(x, y, CM_NEU, 0);
            if (seg % 12 == 6) { cp(x, y + 1, CM_LUZ, 4); cp(x, y + 2, CM_LUZ, 3); }
        }
    }
}

#include "cenario_andares.inc"

// ---- o que vale para todos os andares ----------------------------------------
static void cen_estrado(void)
{
    int ex0 = estrado_cx() - ESTRADO_W / 2;
    for (int y = FR_CHAO; y < FR_EST_Y; y++)               // o tampo: tabuas de comprido, um tom acima
        for (int x = ex0; x < CH_X1; x++) {
            int fila = (int)floorf(fr_z(y) / 11), fa = (int)floorf(fr_z(y - 1) / 11);
            int t = 2;
            if (fila != fa && bayer(x, y) < 0.5f) t = 1;        // as juntas, discretas (o nome do chefe fica em cima)
            if (y < FR_CHAO + 3) t -= bayer(x, y) < 0.5f;
            cp(x, y, CM_MAD, t);
        }
    chl(ex0, FR_EST_Y, CH_X1 - ex0, CM_LAT, 4);
    chl(ex0, FR_EST_Y + 1, CH_X1 - ex0, CM_LAT, 2);
    for (int y = FR_EST_Y + 2; y < FR_EST_Y1; y++) chl(ex0, y, CH_X1 - ex0, CM_MAD, 1);   // o espelho
    cvl(ex0, FR_CHAO, FR_EST_Y1 - FR_CHAO, CM_LAT, 3);     // a quina, que pega a luz
    cvl(ex0 + 1, FR_EST_Y + 2, FR_EST_Y1 - FR_EST_Y - 2, CM_LAT, 1);
    chl(ex0, FR_EST_Y1, CH_X1 - ex0, CM_NEU, 0);            // a sombra no chao
}

static void cen_tapete(void)
{
    int fim = tapete_fim();
    for (int y = TAP_Y0; y < TAP_Y1; y++)
        for (int x = CH_X0; x < CH_X1; x++) {
            int k = tapete_tom(x, y - TAP_Y0, fim);
            if (k == -1) continue;
            if (k == PL_FILETE) cp(x, y, CM_LAT, 3); else cp(x, y, CM_VIN, k + 1);
        }
    for (int y = FR_CHAO; y < TAP_Y0 + 3; y++) {           // o pedaco que vem da porta, em perspectiva
        float q = (y - FR_CHAO) / (float)(TAP_Y0 - FR_CHAO);
        int xl = (int)lroundf(FR_VAO0 - q * 9), xr = (int)lroundf(FR_VAO1 + q * 1);
        int w = xr - xl;
        for (int x = xl; x <= xr; x++) {
            int r = x - xl;
            if (r == 0 || r == w) cp(x, y, CM_VIN, 1);
            else if (r == 2 || r == w - 2) cp(x, y, CM_LAT, 3);
            else if (r == 1 || r == w - 1) cp(x, y, CM_VIN, 3);
            else {
                int d = abs(2 * (r - w / 2)) + abs(2 * ((int)(fr_z(y) / 2) % 14) - 13);
                cp(x, y, CM_VIN, d == 9 ? 3 : d <= 3 ? 4 : 2);
            }
        }
    }
}

static void cen_porta(int f, bool apagada)
{
    int fm = f == 10 ? CM_ACO : CM_LAT;
    // o corredor do outro lado: claro, mais forte embaixo (o chao dele)
    for (int y = FR_PY0 + 4; y < FR_CHAO; y++)
        for (int x = FR_VAO0; x < FR_VAO1; x++) {
            float q = (y - FR_PY0) / (float)(FR_CHAO - FR_PY0);
            int t = q < 0.5f ? 0 : 1;
            if (y > FR_CHAO - 10) t = 1 + (bayer(x, y) < 0.5f);   // o chao do corredor, que pega a luz
            int meio = (FR_VAO0 + FR_VAO1) / 2;
            if (x == meio || x == meio - 1) t = y > FR_PY0 + 8 ? 3 : 2;   // a fresta clara, la no fim
            if (apagada) cp(x, y, CM_NEU, t ? 1 : 0);
            else cp(x, y, CM_LUZ, t);
        }
    for (int s = 0; s < 2; s++) {                            // as folhas abertas para dentro
        int x0 = s ? FR_VAO1 - 4 : FR_VAO0;
        for (int y = FR_PY0 + 4; y < FR_CHAO; y++)
            for (int i = 0; i < 4; i++) {
                int t = i == (s ? 0 : 3) ? 1 : 2;
                if (y == FR_PY0 + 20 || y == FR_PY0 + 34) t = 1;
                cp(x0 + i, y, CM_MAD, t);
            }
        cp(s ? FR_VAO1 - 3 : FR_VAO0 + 2, FR_PY0 + 27, CM_LAT, 4);
    }
    for (int y = FR_PY0; y < FR_CHAO; y++) {                 // os batentes
        cvl(FR_PX0, y, 1, CM_NEU, 0); cvl(FR_PX1 - 1, y, 1, CM_NEU, 0);
        for (int i = 1; i < 5; i++) { cp(FR_PX0 + i, y, fm, i == 1 ? 4 : i == 4 ? 1 : 3); cp(FR_PX1 - 1 - i, y, fm, i == 4 ? 4 : i == 1 ? 1 : 2); }
    }
    for (int x = FR_PX0; x < FR_PX1; x++) {                  // a verga
        cp(x, FR_PY0, CM_NEU, 0);
        for (int j = 1; j < 4; j++) cp(x, FR_PY0 + j, fm, j == 1 ? 4 : j == 2 ? 3 : 1);
    }
    csombra(FR_PX1, FR_PY0 + 2, 2, FR_CHAO - FR_PY0 - 2);
    if (apagada) { cvl(FR_VAO0, FR_PY0 + 4, FR_CHAO - FR_PY0 - 4, CM_ROX, 2); cvl(FR_VAO1 - 1, FR_PY0 + 4, FR_CHAO - FR_PY0 - 4, CM_ROX, 1); }
}

static void cen_pinta(int d, int a)
{
    cen_off = a * CH_W;
    cen_andar = d < 0 ? 0 : d > N_NIVEIS ? N_NIVEIS : d;
    cen_nd = 0;
    cen_np = 0;
    vao_t v[DES_PASSOS + 2];
    int nv = fr_vaos(v);
    switch (cen_andar) {
    case 0: cen_terreo(a, v, nv); break;
    case 1: cen_primeiro(a, v, nv); break;
    case 2: cen_segundo(a, v, nv); break;
    case 3: cen_terceiro(a, v, nv); break;
    case 4: cen_quarto(a, v, nv); break;
    case 5: cen_quinto(a, v, nv); break;
    case 6: cen_sexto(a, v, nv); break;
    case 7: cen_setimo(a, v, nv); break;
    case 8: cen_oitavo(a, v, nv); break;
    case 9: cen_nono(a, v, nv); break;
    default: cen_cobertura(a, v, nv); break;
    }
    cen_estrado();
    cen_tapete();
    cen_porta(cen_andar, a >= DES_ANDARES);
    // a sombra de quem senta na parede: dois tons em volta do retrato (le melhor)
    for (int p = 0, n = mesas_do_salao(); p < n; p++) {
        int x = salao_x(p), hw = RT_W * salao_esc(p) / 2 + 3, y0 = salao_topo(p) - 3, y1 = y0 + RT_H * salao_esc(p) + 6;
        for (int y = y0 - 8; y < y1 + 2 && y < FR_CHAO; y++)
            for (int xx = x - hw - 8; xx < x + hw + 8; xx++) {
                int m = cm_de(xx, y);
                if (m < 0 || CM_EMITE(m)) continue;
                int dx = xx < x - hw ? x - hw - xx : xx >= x + hw ? xx - x - hw + 1 : 0, dy = y < y0 ? y0 - y : 0;
                int d = dx > dy ? dx : dy;
                int k = d == 0 ? 0 : d <= 4 ? 2 : 1;
                if (!k || (k == 1 && bayer(xx, y) < 0.5f)) continue;
                int t = cen_t[y - CH_Y0][xx - CH_X0];
                cen_t[y - CH_Y0][xx - CH_X0] = (uint8_t)(t > k ? t - k : 0);
            }
    }
}

// ---- a luz ------------------------------------------------------------------
static bool fr_coringa(void) { return run.andar >= DES_ANDARES; }
#ifndef FR_BASE
#define FR_BASE       0.25f              // a luz do salao inteiro (0: o tom pintado), clara como a sala do predio
#define FR_BASE_COR  -0.4f               // o do Coringa, mais apagado
#define FR_POCA       1.3f               // a luz de cada mesa por jogar
#define FR_POCA_FEITA 0.5f               // a de uma mesa ja jogada (o salao nao apaga)
#endif
static void fr_luz(const poca_t *pc, int np, int x0, int x1, int y0, int y1)
{
    int ex0 = estrado_cx() - ESTRADO_W / 2;
    bool cor = fr_coringa();
    float base = cor ? FR_BASE_COR : FR_BASE;
    if (x0 < CH_X0) x0 = CH_X0;
    if (x1 > CH_X1) x1 = CH_X1;
    if (y0 < CH_Y0) y0 = CH_Y0;
    if (y1 > CH_Y1) y1 = CH_Y1;
    float luz[CH_W];
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) luz[x - CH_X0] = base + (x >= ex0 && y >= FR_CHAO && y < FR_EST_Y1 ? 0.35f : 0);
        for (int i = 0; i < np; i++) {
            float dy = (y - pc[i].y) / (float)pc[i].ry, dy2 = dy * dy;
            if (dy2 >= 1) continue;
            float meia = pc[i].rx * sqrtf(1 - dy2), inv = 1.0f / pc[i].rx;
            int px0 = pc[i].x - (int)meia, px1 = pc[i].x + (int)meia;
            if (px0 < x0) px0 = x0;
            for (int x = px0; x <= px1 && x < x1; x++) {
                float dx = (x - pc[i].x) * inv, d = dx * dx + dy2;
                if (d < 1) luz[x - CH_X0] += pc[i].s * (1 - d);
            }
        }
        const uint8_t *ml = cen_m[y - CH_Y0], *tl = cen_t[y - CH_Y0];
        uint16_t *lin = chao_px[y - CH_Y0];
        for (int x = x0; x < x1; x++) {
            int m = ml[x - CH_X0], t = tl[x - CH_X0];
            float L = luz[x - CH_X0], b = bayer(x, y);
            if (CM_EMITE(m)) {
                if (m == CM_CEU && L > 1.3f && t < 3 && b < 0.35f) t++;    // a sala refletida no vidro
                lin[x - CH_X0] = CEN_RAMPA[m][t];
                continue;
            }
            int n = (int)floorf(L + b), k = t + n;
            if (cor && L + b * 0.8f > 1.1f && m != CM_VIN) m = CM_ROX;   // a luz do Coringa e roxa
            if (k > CEN_TETO[m]) k = CEN_TETO[m];
            lin[x - CH_X0] = CEN_RAMPA[m][k < 0 ? 0 : k > 5 ? 5 : k];
        }
    }
}

#define FR_POCAS (3 * DES_PASSOS + 2 + CEN_POCAS)
static poca_t fr_pc[FR_POCAS];
static int fr_np = -1;
static void fr_chao(const poca_t *pc0, int np0)
{
    int chave = run.dificuldade * 1000 + run.andar * 100 + mesas_do_salao();
    if (chave != cen_chave) { cen_pinta(run.dificuldade, run.andar); cen_chave = chave; fr_np = -1; }
    poca_t pc[FR_POCAS];
    int np = 0;
    for (int i = 0; i < np0 && np < FR_POCAS; i++) pc[np++] = pc0[i];
    for (int i = 0; i < cen_np && np < FR_POCAS; i++) pc[np++] = cen_pc[i];
    if (np != fr_np) fr_luz(pc, np, CH_X0, CH_X1, CH_Y0, CH_Y1);
    else
        for (int i = 0; i < np; i++) {
            if (!memcmp(&pc[i], &fr_pc[i], sizeof pc[i])) continue;
            const poca_t *q = &pc[i], *o = &fr_pc[i];
            fr_luz(pc, np, q->x - q->rx, q->x + q->rx + 1, q->y - q->ry, q->y + q->ry + 1);
            if (o->x != q->x || o->y != q->y || o->rx != q->rx || o->ry != q->ry)
                fr_luz(pc, np, o->x - o->rx, o->x + o->rx + 1, o->y - o->ry, o->y + o->ry + 1);
        }
    fr_np = np;
    memcpy(fr_pc, pc, (size_t)np * sizeof pc[0]);
    uint16_t *fb = display_fb();
    for (int y = CH_Y0; y < CH_Y1; y++) memcpy(fb + y * GFX_W + CH_X0, chao_px[y - CH_Y0], sizeof chao_px[0]);
}

// O que mexe, devagar e no maximo uma coisa por salao: a TV, o neon, o
// farol da torre, o globo, a fonte, as lampadas do letreiro, os peixes, as
// bolhas e a vela.
static void fr_detalhes(void)
{
    float t = t_fase;
    for (int k = 0; k < cen_nd; k++) {
        const det_t *d = &cen_det[k];
        switch (d->tipo) {
        case DT_LETREIRO: {                                 // o neon: firme, com uma piscada rara
            float c = fmodf(t + d->x * 0.01f, 9.0f);
            if ((c > 6.0f && c < 6.08f) || (c > 6.2f && c < 6.26f))
                for (int j = 0; j < d->h; j++)
                    for (int i = 0; i < d->w; i++) {
                        int x = d->x + i, y = d->y + j;
                        bool letra = d->tipo == DT_LETREIRO ? i >= d->w / 3 && i < 2 * d->w / 3 : true;   // so o A falha
                        if (letra && cm_de(x, y) == CM_NEON) gfx_pixel(x, y, CEN_RAMPA[CM_NEON][0]);
                    }
            break;
        }
        case DT_FAROL: {                                    // o farol vermelho da torre
            int x = d->x + 1, y = d->y + 1;
            if (fmodf(t, 1.8f) < 0.25f) {
                gfx_pixel(x, y, RGB(226, 164, 168));
                gfx_pixel(x - 1, y, C_BRASA); gfx_pixel(x + 1, y, C_BRASA);
                gfx_pixel(x, y - 1, C_BRASA); gfx_pixel(x, y + 1, C_BRASA);
            } else gfx_pixel(x, y, C_VINHO);
            break;
        }
        default: fr_detalhe_andar(d, t); break;
        }
    }
}

// A moldura do mapa: a madeira entre dois filetes, em volta (a porta agora e
// na parede do fundo).
static void fr_moldura(void)
{
    static const uint16_t P[PL_PAREDE] = { C_BARRA, C_MADEIRA_CLR, C_MADEIRA, C_BARRA };
    int w = PL_X1 - PL_X0, h = PL_Y1 - PL_Y0;
    for (int i = 0; i < PL_PAREDE; i++) {
        gfx_rect(PL_X0 + i, PL_Y0 + i, w - 2 * i, 1, P[i]);
        gfx_rect(PL_X0 + i, PL_Y1 - 1 - i, w - 2 * i, 1, P[i]);
        gfx_rect(PL_X0 + i, PL_Y0 + i, 1, h - 2 * i, P[i]);
        gfx_rect(PL_X1 - 1 - i, PL_Y0 + i, 1, h - 2 * i, P[i]);
    }
}

// A planta no vaso, de frente: o vaso de latao e a copa redonda.
static void fr_planta(int x, int yb, bool escura)
{
    uint16_t rl[4];
    rampa_de(C_LATAO, rl);
    for (int j = 0; j < 10; j++) {                          // o vaso, afinando para baixo
        int hw = 6 - j / 3;
        gfx_rect(x - hw - 1, yb - 10 + j, 2 * hw + 3, 1, C_BARRA);
        for (int i = -hw; i <= hw; i++) gfx_pixel(x + i, yb - 10 + j, i < -hw / 2 ? rl[2] : i < hw / 2 ? rl[1] : rl[0]);
    }
    gfx_rect(x - 8, yb - 13, 17, 4, C_BARRA);
    gfx_rect(x - 7, yb - 12, 15, 2, rl[3]);
    gfx_rect(x - 7, yb - 10, 15, 1, rl[1]);
    static const int8_t F[][3] = { { 0, -14, 7 }, { -7, -10, 6 }, { 7, -10, 6 }, { -4, -20, 5 }, { 5, -19, 5 }, { -10, -16, 4 }, { 10, -15, 4 }, { 0, -24, 4 } };
    int n = sizeof F / sizeof F[0];
    for (int k = 0; k < n; k++) gfx_disco(x + F[k][0], yb - 12 + F[k][1], F[k][2] + 1, C_BARRA);
    for (int k = 0; k < n; k++) {
        int fx = x + F[k][0], fy = yb - 12 + F[k][1], r = F[k][2];
        uint16_t c0 = escura ? C_EBANO : (k & 1 ? C_VERDE : RGB(56, 112, 60));
        uint16_t c1 = escura ? (k & 1 ? RGB(54, 36, 74) : C_EBANO_B) : (k & 1 ? RGB(130, 204, 112) : C_VERDE);
        gfx_disco(fx, fy, r, c0);
        gfx_disco(fx - 1, fy - 1, r - 2, c1);
        if (!escura) gfx_pixel(fx - r / 2, fy - r / 2, RGB(130, 204, 112));
    }
}

// A luminaria de cima de uma mesa: o fio, a cupula na cor do salao e a
// lampada (acesa ou nao).
static void fr_luminaria(int x, int y, bool acesa, uint16_t cor)
{
    uint16_t r[4];
    rampa_de(cor, r);
    gfx_rect(x, CH_Y0, 1, y - CH_Y0, C_BARRA);
    for (int j = 0; j < 7; j++) {
        int hw = 3 + j * 5 / 6;
        gfx_rect(x - hw - 1, y + j, 2 * hw + 3, 1, C_BARRA);
        for (int i = -hw; i <= hw; i++) {
            uint16_t c = i < -hw / 3 ? r[2] : i < hw / 2 ? r[1] : r[0];
            if (j == 0) c = r[3];
            if (!acesa) c = i < 0 ? r[1] : r[0];
            gfx_pixel(x + i, y + j, c);
        }
    }
    gfx_rect(x - 8, y + 7, 17, 1, C_BARRA);
    if (acesa) {
        gfx_rect(x - 6, y + 7, 13, 1, C_AMARELO);
        gfx_rect(x - 2, y + 8, 5, 1, C_BRANCO);
        gfx_pixel(x, y + 9, C_AMARELO);
        for (int j = 9; j < 16; j++) {                // o cone, pontilhado ralo, na cor do salao
            int hw = 6 + (j - 9);
            for (int i = -hw; i <= hw; i++)
                if (bayer(x + i, y + j) < 0.22f - (j - 9) * 0.02f) gfx_pixel(x + i, y + j, r[3]);
        }
    } else gfx_rect(x - 2, y + 7, 5, 1, C_EBANO_B);
}
// As arandelas do chefe, dos dois lados do retrato.
static void fr_arandela_chefe(int x, int y, bool acesa, uint16_t cor)
{
    uint16_t r[4];
    rampa_de(cor, r);
    gfx_rect(x - 2, y + 6, 5, 8, C_BARRA); gfx_rect(x - 1, y + 7, 3, 6, C_LATAO);
    gfx_rect(x, y + 3, 1, 4, C_LATAO_ESC);
    for (int j = 0; j < 5; j++) {                           // a tulipa, aberta para cima
        int hw = 2 + (4 - j) * 3 / 4;
        gfx_rect(x - hw - 1, y - 2 + j, 2 * hw + 3, 1, C_BARRA);
        for (int i = -hw; i <= hw; i++) gfx_pixel(x + i, y - 2 + j, acesa ? (i < 0 ? r[3] : r[2]) : (i < 0 ? r[1] : r[0]));
    }
    if (acesa) { gfx_rect(x - 1, y - 4, 3, 2, C_AMARELO); gfx_pixel(x, y - 5, C_BRANCO); }
}

// A cadeira vazia, de frente: o assento, os pes e o encosto do lado de fora.
static void fr_cadeira(int x, int y, int lado)
{
    int sx = x - 4;
    gfx_rect(sx - 1, y - 1, 11, 5, C_BARRA);
    gfx_rect(sx, y, 9, 2, TAPETE[2]);
    gfx_rect(sx, y + 2, 9, 1, TAPETE[0]);
    for (int k = 0; k < 2; k++) {
        int lx = k ? sx + 7 : sx;
        gfx_rect(lx - 1, y + 3, 4, 9, C_BARRA);
        gfx_rect(lx, y + 3, 2, 8, k ? C_MADEIRA : C_MADEIRA_CLR);
    }
    int bx = lado < 0 ? sx - 1 : sx + 7;
    gfx_rect(bx - 1, y - 14, 5, 15, C_BARRA);
    gfx_rect(bx, y - 13, 3, 13, TAPETE[1]);
    gfx_rect(bx, y - 13, 1, 13, TAPETE[2]);
    gfx_rect(bx, y - 13, 3, 1, C_MADEIRA_CLR);
}
// O pe da mesa: a saia do tampo (a grossura da borda), a coluna torneada e a base.
static void fr_mesa_pe(int x, int y, int rx, int ry)
{
    int yb = y + ry + 14;
    gfx_rect(x - 9, yb - 1, 19, 4, C_BARRA);
    gfx_rect(x - 8, yb, 17, 2, C_MADEIRA);
    gfx_rect(x - 8, yb, 17, 1, C_MADEIRA_CLR);
    gfx_rect(x - 3, y + ry, 7, yb - y - ry, C_BARRA);
    gfx_rect(x - 2, y + ry, 5, yb - y - ry, C_MADEIRA);
    gfx_rect(x - 2, y + ry, 1, yb - y - ry, C_MADEIRA_CLR);
    gfx_rect(x - 4, y + ry + 6, 9, 3, C_BARRA);
    gfx_rect(x - 3, y + ry + 7, 7, 1, C_MADEIRA_CLR);
    for (int i = -rx; i <= rx; i++) {                        // a saia, embaixo da metade da frente
        float u = i / (float)(rx + 1);
        int ya = y + (int)(ry * sqrtf(1 - u * u));
        uint16_t c = i < -rx / 3 ? C_MADEIRA_CLR : i < rx / 2 ? C_MADEIRA : C_MADEIRA_ESC;
        gfx_rect(x + i, ya, 1, 3, c);
        gfx_pixel(x + i, ya + 3, C_BARRA);
    }
}

// Uma mesa do salao de frente: as cadeiras, a luminaria, quem joga sentado
// atras (o retrato na moldura), o tampo com o pe e, na frente, o nome.
static void fr_salao_mesa(int p)
{
    const no_t *no = &run.no[p];
    int x = salao_x(p), esc = salao_esc(p), w = RT_W * esc, h = RT_H * esc, y = salao_topo(p);
    int my = mesa_y(p), rx = mesa_rx(p), ry = mesa_ry(p), ly = rotulo_y(p);
    bool feita = p < run.passo, da_vez = p == run.passo, chefe = salao_chefe(p);
    bool acesa = !feita && luz_mesa(p);
    uint16_t cs = COR_SALAO[run.andar <= DES_ANDARES ? run.andar : 0];
    if (chefe) {
        fr_arandela_chefe(x - w / 2 - 12, y + 26, acesa, cs);
        fr_arandela_chefe(x + w / 2 + 12, y + 26, acesa, cs);
    } else fr_luminaria(x, y - 17, acesa, cs);
    fr_mesa_pe(x, my, rx, ry);
    fr_cadeira(x - rx - 8, my + 3, -1);
    fr_cadeira(x + rx + 8, my + 3, 1);
    uint16_t moldura = chefe ? C_OURO : (feita ? C_FELTRO2 : C_LATAO_ESC);
    uint16_t rm[4];
    ui_rampa(moldura, feita, rm);
    ui_contorno(x - w / 2 - 3, y - 3, w + 6, h + 6, C_BARRA);
    ui_aro(x - w / 2 - 2, y - 2, w + 4, h + 4, 2, rm, UI_VIVO);
    gfx_rect(x - w / 2, y, w, h, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(x - w / 2, y, esc, no->perso, t_fase + no->perso * 1.3f, false, 0, RT_H);
    if (feita) {
        ui_escurece(x - w / 2, y, w, h, 150);
        int vy = y + h / 2;
        gfx_linha_grossa(x - 9, vy, x - 2, vy + 8, 4, C_OURO);
        gfx_linha_grossa(x - 2, vy + 8, x + 12, vy - 10, 4, C_OURO);
    }
    pl_mesa(x, my, rx, ry, acesa, chefe);
    uint16_t cn = feita ? C_TEXTO_M : cor_no(no->tipo);
    const char *nome = PERSO[no->perso].nome;
    int f = gfx_largura(nome, GFX_FINO) <= 2 * rx + 40 ? GFX_FINO : GFX_MIUDO;
    texto_no_chao(x - gfx_largura(nome, f) / 2, ly + (f == GFX_MIUDO ? 2 : 0), nome, cn, f);
    int vale = moedas_mesa(p);
    if (run.andar == DES_ANDARES - 1 && chefe && !com_coringa()) vale = 0;
    char v[16] = "";
    const char *tag = feita ? "vencida" : NOME_NO[no->tipo];
    if (!feita) snprintf(v, sizeof v, vale ? "+%d" : "final", vale);
    int wt = gfx_largura(tag, GFX_FINO), wv = v[0] ? gfx_largura(v, GFX_FINO) + (vale ? 12 : 0) + 8 : 0;
    int tx = x - (wt + wv) / 2, ty = ly + 12;
    texto_no_chao(tx, ty, tag, cn, GFX_FINO);
    if (v[0]) {
        int vx = tx + wt + 8;
        if (vale) { moeda_r(vx + 4, ty + 10, ICONE_P); vx += 12; }
        texto_no_chao(vx, ty, v, vale ? C_OURO : C_ROSA, GFX_FINO);
    }
    int meia = chefe ? 62 : 50, bx = x - meia, bw = 2 * meia, by = y - 6, bh = ty + 19 - by;
    if (p == des_cur) ui_cantos_em(bx, by, bw, bh, C_OURO, ui_respira());
    zona_clique(bx, by, bw, bh, &des_cur, p, da_vez ? KEY_ENTER : 0);
}

// O salao acendendo, de frente: a porta clareia, a luz corre o tapete, cada
// luminaria acende quando a luz chega nela; no fim, o resto clareia. O que
// tem luz propria (a cidade, o corredor, o neon) fica aceso no escuro.
// O pixel e de um detalhe com luz propria (a TV, o neon, o farol)?
static bool fr_detalhe_em(int x, int y)
{
    for (int k = 0; k < cen_nd; k++) {
        const det_t *d = &cen_det[k];
        if (x < d->x || x >= d->x + d->w || y < d->y || y >= d->y + d->h || d->tipo == DT_GLOBO) continue;
        int m = cm_de(x, y);
        return m >= 0 && CM_EMITE(m);
    }
    return false;
}
static void fr_escuro(void)
{
    if (t_luz <= 0) return;
    float e = LUZ_T - t_luz, amb = luz_q((e - (LUZ_T - 0.5f)) / 0.5f), porta = luz_q((e - 0.08f) / 0.25f);
    float frente = PL_X0 + (tapete_fim() + 8 - PL_X0) * luz_q((e - LUZ_ONDA0) / LUZ_ONDA);
    poca_t pc[DES_PASSOS];
    int np = 0;
    for (int p = 0, n = mesas_do_salao(); p < n; p++) {
        if (!luz_mesa(p)) continue;
        int y0 = CH_Y0, y1 = rotulo_y(p) + 28;
        pc[np++] = (poca_t){ salao_x(p), (y0 + y1) / 2, mesa_rx(p) + 48, (y1 - y0) / 2 + 12, 1.8f };
    }
    uint16_t *fb = display_fb();
    int pmx = (FR_VAO0 + FR_VAO1) / 2;
    for (int y = PL_Y0; y < PL_Y1; y++) {
        float dyt = abs(y - TAP_MEIO) / 22.0f;
        for (int x = PL_X0; x < PL_X1; x++) {
            int m = cm_de(x, y);
            if (m >= 0 && CM_EMITE(m)) {                 // o que tem luz propria, se nada foi desenhado por cima
                if (fb[y * GFX_W + x] == chao_px[y - CH_Y0][x - CH_X0] || fr_detalhe_em(x, y)) continue;
            }
            float L = amb, dx = (x - pmx) / 46.0f, dy = (y - 128) / 70.0f, d = dx * dx + dy * dy;
            if (d < 1) L = fmaxf(L, (1 - d) * 1.5f * porta);
            if (dyt < 1 && x < frente) L = fmaxf(L, (1 - dyt) * 0.9f * fminf(1, (frente - x) / 24));
            for (int i = 0; i < np && L < 1; i++) {
                float u = (x - pc[i].x) / (float)pc[i].rx, v = (y - pc[i].y) / (float)pc[i].ry, q = u * u + v * v;
                if (q < 1) L = fmaxf(L, (1 - q) * pc[i].s);
            }
            if (L >= 1) continue;
            int a = (int)((1 - L) * 92 + (bayer(x, y) - 0.5f) * 26);
            if (a <= 0) continue;
            uint16_t *px = &fb[y * GFX_W + x];
            *px = gfx_mistura(*px, C_BARRA, a > 100 ? 100 : a);
        }
    }
}

static void desenha_mapa(void)
{
    faixa_run(KEY_ESC);
    rodape_run("setas veem as mesas  ·  ENTER senta à mesa  ·  TAB bolsa  ·  ESC menu");
    int n = mesas_do_salao();
    poca_t pc[3 * DES_PASSOS + 2];
    int np = 0;
    if (!fr_coringa()) pc[np++] = (poca_t){ (FR_VAO0 + FR_VAO1) / 2, FR_CHAO + 6, 40, 24, 0.9f };   // a luz do corredor no chao
    for (int p = 0; p < n; p++) {
        int x = salao_x(p), y = mesa_y(p), rx = mesa_rx(p), ry = mesa_ry(p);
        bool chefe = salao_chefe(p);
        {
            float s = p < run.passo ? FR_POCA_FEITA : FR_POCA + (chefe ? 0.1f : 0);
            if (p == run.passo) s += 0.45f * (0.5f + 0.5f * sinf(t_fase * 4));
            pc[np++] = (poca_t){ x, chefe ? 106 : 112, chefe ? 88 : 68, chefe ? 52 : 44, s };   // a parede
            pc[np++] = (poca_t){ x, y + 2, rx + 26, ry + 6, s * 0.7f };                           // o chao, junto da mesa
        }
        pc[np++] = (poca_t){ x + 3, y + ry + 15, rx + 10, 6, -1.4f };
    }
    fr_chao(pc, np);
    fr_detalhes();
    fr_moldura();
    int ex = estrado_cx();
    fr_planta(ex - ESTRADO_W / 2 + 18, FR_CHAO + 10, fr_coringa());
    fr_planta(ex + ESTRADO_W / 2 - 16, FR_CHAO + 10, fr_coringa());
    for (int p = 0; p < n; p++) fr_salao_mesa(p);
    int px = salao_x(run.passo), py = TAP_MEIO;
    if (t_luz > 0) px = PL_X0 + 14;
    else if (t_mapa_anda > 0) {
        float q = 1 - t_mapa_anda / 0.7f;
        if (q < 0) q = 0;
        int ax = run.passo > 0 ? salao_x(run.passo - 1) : PL_X0 + 14;
        float s = q * q * (3 - 2 * q);
        px = ax + (int)((px - ax) * s);
        py -= (int)(fabsf(sinf(q * 6.2832f)) * 16);
    } else py += (int)(sinf(t_fase * 4) * 2) - 1;
    ficha_jogador(px, py);
    fr_escuro();
    info_mesa(des_cur);
    for (int a = 0, k = 0; a < A_N; a++)
        if (run_amuletos() >> a & 1) {
            int ax = PL_X0 + 15 + k * 19, ay = PL_Y0 + 15;
            desenha_amuleto(ax, ay, 8, a, false);
            dica_zona(ax - 9, ay - 9, 18, 18, DICA_AMULETO, a);
            k++;
        }
}
