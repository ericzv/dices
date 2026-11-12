// ===========================================================================
// As salas do predio da tela de Dificuldade: cada andar do cassino com a sua
// cara (Terreo boteco, 1º madeira, 2º bar de tijolo, 3º aco, 4º jardim de
// inverno, 5º ouro, 6º art deco, 7º cortina, 8º lounge, 9º aquario e a
// Cobertura). Tres planos por sala (parede, chao em profundidade e um objeto
// escuro na frente), luz chapada de cima, gente em silhueta com cotovelo e a
// luz de contorno do lado da lampada. Cores da paleta (cores.inc), com nome
// curto aqui; os K_ e as marcas saem de cena no fim do predio.inc.
// ===========================================================================
#define K_PRETO  RGB(14, 14, 12)
#define K_PLACA  RGB(24, 22, 20)
#define K_EB     RGB(36, 32, 29)
#define K_EB2    RGB(62, 56, 50)
#define K_PARDO  RGB(96, 88, 80)
#define K_CINZA  RGB(128, 120, 112)
#define K_CHUMBO RGB(70, 76, 88)
#define K_FERRO  RGB(104, 112, 124)
#define K_ACO    RGB(140, 150, 164)
#define K_PRATA  RGB(184, 194, 208)
#define K_MARF3  RGB(166, 154, 132)
#define K_MARF2  RGB(204, 194, 172)
#define K_MARF   RGB(238, 230, 212)
#define K_BRANCO RGB(252, 248, 236)
#define K_MAD0   RGB(56, 32, 20)
#define K_MAD1   RGB(78, 46, 30)
#define K_MAD2   RGB(120, 78, 50)
#define K_MAD3   RGB(156, 106, 64)
#define K_LAT0   RGB(106, 86, 44)
#define K_LAT1   RGB(150, 122, 62)
#define K_LAT2   RGB(190, 156, 86)
#define K_LAT3   RGB(228, 194, 106)
#define K_LAT4   RGB(246, 222, 132)
#define K_VIN0   RGB(66, 18, 24)
#define K_VIN1   RGB(118, 30, 38)
#define K_VIN2   RGB(150, 62, 80)
#define K_VERM   RGB(190, 56, 60)
#define K_VIN3   RGB(208, 98, 108)
#define K_FOGO   RGB(214, 112, 44)
#define K_FOGO2  RGB(238, 162, 74)
#define K_AZ0    RGB(26, 34, 62)
#define K_AZ1    RGB(56, 86, 150)
#define K_AZ2    RGB(96, 134, 196)
#define K_ROX0   RGB(54, 36, 74)
#define K_ROX1   RGB(102, 74, 138)
#define K_VERDE  RGB(88, 152, 84)
#define K_VERDE2 RGB(130, 204, 112)

static uint32_t hsh(int x, int y)
{
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static void px(int x, int y, uint16_t c) { gfx_pixel(x, y, c); }
static void hl(int x, int y, int w, uint16_t c) { if (w > 0) gfx_rect(x, y, w, 1, c); }
static void vl(int x, int y, int h, uint16_t c) { if (h > 0) gfx_rect(x, y, 1, h, c); }
// Degrade vertical pontilhado por varios tons.
static void degrade_v(int x, int y, int w, int h, const uint16_t *c, int n)
{
    for (int j = 0; j < h; j++) {
        float p = h > 1 ? (float)j / (h - 1) * (n - 1) : 0;
        int k = (int)p;
        if (k >= n - 1) k = n - 2;
        float f = p - k;
        for (int i = 0; i < w; i++) px(x + i, y + j, bayer(x + i, y + j) < f ? c[k + 1] : c[k]);
    }
}
#define K_AGUA   RGB(72, 150, 138)
#define K_AGUA2  RGB(96, 182, 198)
#define K_AZ3    RGB(168, 204, 230)
#define K_ROX2   RGB(150, 112, 160)
#define K_ROX3   RGB(206, 180, 218)
#define K_ROSA   RGB(226, 164, 168)
#define MARCA    RGB(255, 0, 255)                 // corpo
#define MARCA2   RGB(0, 255, 255)                 // roupa
#define MARCA3   RGB(255, 255, 0)                 // chapeu e cabelo

static uint16_t *alvo(void) { return gfx_alvo() ? gfx_alvo() : display_fb(); }
static uint16_t pega(int x, int y) { return (x < 0 || y < 0 || x >= GFX_W || y >= GFX_H) ? 0 : alvo()[y * GFX_W + x]; }
static float frac_(float v) { return v - floorf(v); }
static float onda(float t, float per) { return sinf(t * 6.2832f / per); }


// Um tom acima / abaixo na rampa da cor (como a paleta ve).
static uint16_t acima(uint16_t c)
{
    switch (c) {
    case K_PRETO: return K_PLACA;
    case K_PLACA: return K_EB;
    case K_EB: return K_EB2;
    case K_EB2: return K_PARDO;
    case K_PARDO: return K_CINZA;
    case K_MAD0: return K_MAD1;
    case K_MAD1: return K_MAD2;
    case K_MAD2: return K_MAD3;
    case K_VIN0: return K_VIN1;
    case K_VIN1: return K_VIN2;
    case K_AZ0: return K_AZ1;
    case K_ROX0: return K_ROX1;
    case K_CHUMBO: return K_FERRO;
    case K_FERRO: return K_ACO;
    case K_LAT0: return K_LAT1;
    case K_LAT1: return K_LAT2;
    default: { uint16_t r[4]; rampa_de(gfx_na_paleta(c), r); return r[3]; }
    }
}
static uint16_t abaixo(uint16_t c)
{
    switch (c) {
    case K_PLACA: return K_PRETO;
    case K_EB: return K_PLACA;
    case K_EB2: return K_EB;
    case K_PARDO: return K_EB2;
    case K_CINZA: return K_PARDO;
    case K_MAD1: return K_MAD0;
    case K_MAD2: return K_MAD1;
    case K_MAD3: return K_MAD2;
    case K_MAD0: return K_PLACA;
    case K_VIN1: return K_VIN0;
    case K_VIN2: return K_VIN1;
    case K_VIN0: return K_PLACA;
    case K_AZ1: return K_AZ0;
    case K_AZ0: return K_PLACA;
    case K_ROX1: return K_ROX0;
    case K_ROX0: return K_PLACA;
    case K_FERRO: return K_CHUMBO;
    case K_CHUMBO: return K_EB2;
    case K_LAT1: return K_LAT0;
    case K_LAT0: return K_MAD0;
    case K_MARF3: return K_PARDO;
    case K_MARF2: return K_MARF3;
    default: { uint16_t r[4]; rampa_de(gfx_na_paleta(c), r); return r[1]; }
    }
}
static void clareia_px(int x, int y) { px(x, y, acima(pega(x, y))); }
static void escurece_px(int x, int y) { px(x, y, abaixo(pega(x, y))); }

typedef struct { int x, y, w, h, yF, d; float t; bool ac; uint16_t luz; int lx, ly; } sala_t;
#define FB 7                                      // a faixa do chao em profundidade
static bool dentro(const sala_t *S, int x, int y) { return x >= S->x && x < S->x + S->w && y >= S->y && y < S->yF; }

// O cone de uma luz do teto: um tom acima, chapado, com 2 px pontilhados na beira.
static void cone_chapado(const sala_t *S, int cx, int y0, float abre)
{
    if (!S->ac) return;
    for (int y = y0; y < S->yF; y++) {
        float hw = 3 + (y - y0) * abre;
        for (int x = cx - (int)hw - 1; x <= cx + (int)hw + 1; x++) {
            if (!dentro(S, x, y)) continue;
            float d = hw - fabsf((float)(x - cx));
            if (d >= 2 || (d >= 0 && bayer(x, y) < 0.5f)) clareia_px(x, y);
        }
    }
}
// A vinheta: as beiras um tom abaixo, em degrau, sem pontilhado.
static void vinheta(const sala_t *S)
{
    for (int y = S->y; y < S->yF; y++)
        for (int x = S->x; x < S->x + S->w; x++) {
            int dl = x - S->x, dr = S->x + S->w - 1 - x, dt = y - S->y;
            if (dl < 5 || dr < 5 || dt < 3) escurece_px(x, y);
            if (dl < 2 || dr < 2) escurece_px(x, y);
        }
}
// A sombra de contato no chao: uma elipse de w x 2, um tom abaixo do piso.
static void sombra_pe(int cx, int yF, int w)
{
    for (int i = -w / 2; i <= w / 2; i++) {
        escurece_px(cx + i, yF - 1);
        if (abs(i) < w / 2 - 1) escurece_px(cx + i, yF);
    }
}
// Um objeto escuro no primeiro plano, cortado pela beira (da profundidade).
static void frente(const sala_t *S, int tipo, bool dir)
{
    int x0 = dir ? S->x + S->w - 12 : S->x, yF = S->yF;
    uint16_t a = K_PRETO, b = K_PLACA;
    switch (tipo) {
    case 0:                                       // coluna
        gfx_rect(dir ? x0 + 2 : x0, S->y, 10, S->h, a);
        vl(dir ? x0 + 2 : x0 + 9, S->y, S->h, b);
        hl(dir ? x0 + 1 : x0, S->y + 4, 12, b); hl(dir ? x0 + 1 : x0, yF - 6, 12, b);
        break;
    case 1:                                       // folhagem
        for (int k = 0; k < 7; k++) {
            float a0 = -1.8f + k * 0.42f;
            for (int i = 0; i < 16; i++) {
                int ex = (dir ? S->x + S->w - 2 : S->x + 2) + (int)(cosf(a0) * i * (dir ? -1 : 1) * 0.9f);
                int ey = yF - 2 + (int)(sinf(a0) * i) + i * i / 22;
                if (dentro(S, ex, ey)) px(ex, ey, a);             // so dentro da sala: nao vaza pela laje
                if (dentro(S, ex, ey + 1)) px(ex, ey + 1, a);
                if (i > 4 && dentro(S, ex + (dir ? -1 : 1), ey)) px(ex + (dir ? -1 : 1), ey, b);
            }
        }
        break;
    case 2:                                       // encosto de cadeira
        gfx_rect(dir ? x0 + 3 : x0, yF - 20, 8, 20, a);
        gfx_disco(dir ? x0 + 7 : x0 + 4, yF - 20, 4, a);
        vl(dir ? x0 + 3 : x0 + 7, yF - 20, 19, b);
        break;
    default:                                      // engradado de cerveja
        gfx_rect(x0, yF - 10, 12, 10, a);
        hl(x0, yF - 10, 12, b); hl(x0, yF - 5, 12, b);
        for (int i = 2; i < 12; i += 3) { px(x0 + i, yF - 12, b); px(x0 + i, yF - 11, a); }
        break;
    }
}

// --- gente --------------------------------------------------------------------
static int fbx0 = 9999, fby0 = 9999, fbx1 = -1, fby1 = -1;
static void f_bb(int x, int y, int w, int h)
{
    if (x < fbx0) fbx0 = x;
    if (y < fby0) fby0 = y;
    if (x + w - 1 > fbx1) fbx1 = x + w - 1;
    if (y + h - 1 > fby1) fby1 = y + h - 1;
}
static void f_r(int x, int y, int w, int h, uint16_t m) { if (w > 0 && h > 0) { gfx_rect(x, y, w, h, m); f_bb(x, y, w, h); } }
static void f_l(int x0, int y0, int x1, int y1, int e, uint16_t m)
{
    gfx_linha_grossa(x0, y0, x1, y1, e, m);
    f_bb((x0 < x1 ? x0 : x1) - e, (y0 < y1 ? y0 : y1) - e, abs(x1 - x0) + 2 * e + 1, abs(y1 - y0) + 2 * e + 1);
}
static uint8_t f_tipo[96 * 96];
static void f_fim(uint16_t corpo, uint16_t roupa, uint16_t cabelo, uint16_t borda, int lx, int ly)
{
    if (fbx1 < fbx0) return;
    int w = fbx1 - fbx0 + 1, h = fby1 - fby0 + 1;
    if (w > 96) w = 96;
    if (h > 96) h = 96;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int x = fbx0 + i, y = fby0 + j;
            uint16_t p = pega(x, y);
            uint8_t k = p == MARCA ? 1 : p == MARCA2 ? 2 : p == MARCA3 ? 3 : 0;
            if (k) {
                int sx = lx < x - 2 ? -1 : lx > x + 2 ? 1 : 0;
                uint16_t vz = sx ? pega(x + sx, y) : 0, vc = pega(x, y - 1);
                bool fora_h = sx && vz != MARCA && vz != MARCA2 && vz != MARCA3;
                bool fora_v = ly < y - 6 && vc != MARCA && vc != MARCA2 && vc != MARCA3;
                if ((fora_h || fora_v) && ((x * 3 + y) % 7) != 0) k += 4;   // contorno, interrompido
            }
            f_tipo[j * 96 + i] = k;
        }
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            uint8_t k = f_tipo[j * 96 + i];
            if (!k) continue;
            uint16_t c = k >= 4 ? borda : k == 1 ? corpo : k == 2 ? roupa : cabelo;
            px(fbx0 + i, fby0 + j, c);
        }
    fbx0 = fby0 = 9999; fbx1 = fby1 = -1;
}

// Uma pessoa de 24 px (5 cabecas). Bracos em dois pedacos: o ombro em angulo
// (0 pendurado, pi/2 para o lado, pi para cima) e o cotovelo dobrando.
enum { R_TERNO, R_VESTIDO, R_BERMUDA, R_COLETE, R_CAUDA };
enum { C_NADA, C_CARTOLA, C_FEDORA, C_COQUE, C_BONE, C_COCO, C_RABO, C_CARECA, C_CHEIO };
enum { AC_NADA, AC_GRAVATA, AC_COLAR, AC_FLOR, AC_LENCO };
typedef struct {
    float oe, od, ce, cd;          // ombros e cotovelos
    int sent, dir, anda, roupa, cab, ac, barriga;
    uint16_t cor;                  // a cor do toque (gravata, colar, flor)
} pes_t;
static pes_t P(float oe, float od) { pes_t p; memset(&p, 0, sizeof p); p.oe = oe; p.od = od; p.ce = 0.35f; p.cd = 0.35f; p.dir = 1; return p; }

static void pes_mao(int cx, int yF, const pes_t *p, int s, int *hx, int *hy)
{
    int top = yF - 24 + (p->sent ? 6 : 0);
    float o = s < 0 ? p->oe : p->od, c = s < 0 ? p->ce : p->cd;
    int sx = cx + s * 3, sy = top + 7;
    int ex = sx + (int)lroundf(sinf(o) * 5 * s), ey = sy + (int)lroundf(cosf(o) * 5);
    *hx = ex + (int)lroundf(sinf(o + c) * 5 * s);
    *hy = ey + (int)lroundf(cosf(o + c) * 5);
}
static bool sala_vazia = false;          // andar fechado: ninguem dentro
static void pessoa(int cx, int yF, pes_t p, uint16_t borda, int lx, int ly, float t)
{
    if (sala_vazia) return;
    int resp = ((int)(t * 1.1f + cx * 0.37f)) % 2;                 // a respiracao: o ombro sobe 1 px
    int top = yF - 24 + (p.sent ? 6 : 0);
    int dir = p.dir ? p.dir : 1;
    sombra_pe(cx + (p.sent ? dir * 2 : 0), yF, p.sent ? 12 : 10);
    // pernas
    if (p.sent) {
        f_r(dir > 0 ? cx - 2 : cx - 6, yF - 7, 9, 2, MARCA2);
        f_r(cx + dir * 5 - (dir > 0 ? 0 : 1), yF - 6, 2, 6, MARCA);
    } else if (p.roupa == R_VESTIDO) {
        for (int k = 0; k < 10; k++) f_r(cx - 2 - k / 3, top + 13 + k, 5 + 2 * (k / 3), 1, MARCA2);
        f_r(cx - 2, yF - 1, 2, 1, MARCA); f_r(cx + 1, yF - 1, 2, 1, MARCA);
    } else {
        int a = p.anda ? (p.anda & 1) : 0;
        uint16_t calca = p.roupa == R_BERMUDA ? MARCA2 : MARCA;
        f_r(cx - 2 - a, top + 14, 2, 10, MARCA); f_r(cx + 1 + a, top + 14, 2, 10, MARCA);
        if (!p.anda) { f_r(cx + 1, top + 18, 1, 6, MARCA); }               // o peso numa perna: o joelho dobra
        if (p.roupa == R_BERMUDA) { f_r(cx - 2, top + 14, 5, 4, calca); }
    }
    // tronco: ombros de 7, cintura de 5
    uint16_t tr = p.roupa == R_BERMUDA ? MARCA : MARCA2;
    f_r(cx - 3, top + 6 + resp, 7, 2, tr);
    f_r(cx - 3, top + 8, 6 + (p.barriga ? 2 : 0), 3, tr);
    f_r(cx - 2, top + 11, 5 + (p.barriga ? 2 : 0), 3, tr);
    if (p.roupa == R_CAUDA) f_r(cx - 2 - dir * 2, top + 13, 3, 6, MARCA2);  // a cauda da casaca
    if (p.roupa == R_COLETE) f_r(cx - 1, top + 7, 3, 6, MARCA);
    f_r(cx - 1, top + 5, 2, 1, MARCA);                                     // pescoco
    // cabeca 5 x 5
    f_r(cx - 1, top, 3, 1, MARCA); f_r(cx - 2, top + 1, 5, 3, MARCA); f_r(cx - 1, top + 4, 3, 1, MARCA);
    switch (p.cab) {
    case C_CARTOLA: f_r(cx - 2, top - 4, 5, 4, MARCA3); f_r(cx - 3, top, 7, 1, MARCA3); break;
    case C_FEDORA:  f_r(cx - 2, top - 2, 5, 2, MARCA3); f_r(cx - 4, top - 0, 9, 1, MARCA3); px(cx, top - 2, K_PRETO); break;
    case C_COQUE:   f_r(cx - 2, top, 5, 2, MARCA3); f_r(cx - 2 * dir - 1, top - 2, 3, 3, MARCA3); break;
    case C_BONE:    f_r(cx - 2, top - 1, 5, 2, MARCA3); f_r(dir > 0 ? cx + 2 : cx - 5, top, 4, 1, MARCA3); break;
    case C_COCO:    f_r(cx - 2, top - 2, 5, 2, MARCA3); f_r(cx - 3, top - 0, 7, 1, MARCA3); break;
    case C_RABO:    f_r(cx - 2, top, 5, 2, MARCA3); f_l(cx - 3 * dir, top + 1, cx - 5 * dir, top + 6, 1, MARCA3); break;
    case C_CHEIO:   f_r(cx - 3, top - 1, 7, 3, MARCA3); f_r(cx - 3, top + 2, 2, 3, MARCA3); f_r(cx + 2, top + 2, 2, 3, MARCA3); break;
    default: break;
    }
    // bracos (dois pedacos; a mao afina para 1 px)
    for (int s = -1; s <= 1; s += 2) {
        float o = s < 0 ? p.oe : p.od, c = s < 0 ? p.ce : p.cd;
        int sx = cx + s * 3, sy = top + 7 + resp;
        int ex = sx + (int)lroundf(sinf(o) * 5 * s), ey = sy + (int)lroundf(cosf(o) * 5);
        int hx = ex + (int)lroundf(sinf(o + c) * 5 * s), hy = ey + (int)lroundf(cosf(o + c) * 5);
        f_l(sx, sy, ex, ey, 2, tr);
        f_l(ex, ey, hx, hy, 1, p.roupa == R_BERMUDA ? MARCA : tr);
        f_r(hx, hy, 1, 1, MARCA);
    }
    f_fim(K_PLACA, K_EB, K_PRETO, borda, lx, ly);
    // o toque de cor e o brilho dos oculos
    switch (p.ac) {
    case AC_GRAVATA: vl(cx, top + 6, 3, p.cor); break;
    case AC_COLAR:   hl(cx - 1, top + 6, 3, p.cor); break;
    case AC_FLOR:    px(cx - 2 * dir, top + 8, p.cor); px(cx - 2 * dir, top + 7, p.cor); break;
    case AC_LENCO:   hl(cx - 2 * dir - 1, top + 8, 2, p.cor); break;
    default: break;
    }
    if (p.cab == C_CARECA) { px(cx + dir, top + 2, K_PRATA); px(cx + dir * 2, top + 2, K_PRATA); px(cx, top, K_EB2); }
}

// --- pecas com volume (topo claro, frente media, base escura, sombra) ---------
static void mesa_g(int x, int yF, int w, bool aceso)
{
    for (int i = 2; i < w - 2; i++) escurece_px(x + i, yF);           // a sombra no chao
    hl(x + 2, yF - 13, w - 4, aceso ? feltro_tons[5] : feltro_tons[4]);
    hl(x + 1, yF - 12, w - 2, feltro_tons[3]);
    hl(x, yF - 11, w, aceso ? K_MAD3 : K_MAD2);
    hl(x, yF - 10, w, K_MAD2);
    hl(x, yF - 9, w, K_MAD0);
    gfx_rect(x + 3, yF - 8, w - 6, 1, K_PLACA);
    int c = x + w / 2;
    gfx_rect(c - 2, yF - 7, 4, 6, K_MAD1);
    vl(c - 2, yF - 7, 6, K_MAD2);
    gfx_rect(c - 6, yF - 2, 12, 2, K_MAD1);
    hl(c - 6, yF - 2, 12, K_MAD2); hl(c - 6, yF - 1, 12, K_MAD0);
}
static void fichas(int x, int y, int n, uint16_t c)
{
    uint16_t r[4];
    rampa_de(gfx_na_paleta(c), r);
    for (int i = 0; i < n; i++) hl(x, y - i, 3, (i & 1) ? r[2] : r[3]);
    px(x + 2, y, r[1]);
}
static void carta(int x, int y, bool costas)
{
    gfx_rect(x, y, 3, 4, costas ? K_VIN1 : K_MARF);
    px(x + 1, y + 1, costas ? K_VIN2 : K_VERM);
    vl(x + 2, y + 1, 3, costas ? K_VIN0 : K_MARF2);
}
static void copo(int x, int y, uint16_t liq)
{
    vl(x, y, 3, K_MARF3); vl(x + 2, y, 3, K_MARF3);
    px(x + 1, y + 1, liq); px(x + 1, y + 2, liq); px(x + 1, y + 3, K_MARF3);
    px(x, y, K_MARF2);
}
static void cadeira_g(int x, int yF, int dir, uint16_t m, uint16_t estofo)
{
    int costas = dir > 0 ? x : x + 6;
    vl(costas, yF - 15, 15, m);
    vl(costas + (dir > 0 ? 1 : -1), yF - 14, 6, estofo);
    hl(x, yF - 8, 7, estofo);
    hl(x, yF - 7, 7, abaixo(m));
    vl(dir > 0 ? x + 6 : x, yF - 6, 6, m);
    escurece_px(x + 3, yF);
}
static void banqueta(int x, int yF, uint16_t assento)
{
    hl(x, yF - 9, 6, acima(assento)); hl(x, yF - 8, 6, assento); hl(x + 1, yF - 7, 4, abaixo(assento));
    vl(x + 2, yF - 6, 5, K_ACO); vl(x + 3, yF - 6, 5, K_FERRO);
    hl(x + 1, yF - 4, 4, K_ACO);
    hl(x, yF - 1, 6, K_FERRO);
    escurece_px(x + 2, yF); escurece_px(x + 3, yF);
}
static void lustre_g(int cx, int y, bool aceso, float t, int larg, int velas)
{
    vl(cx, y, 3, K_LAT1);
    gfx_rect(cx - 1, y + 3, 3, 3, K_LAT2); px(cx - 1, y + 3, K_LAT3);
    for (int s = -1; s <= 1; s += 2)
        for (int i = 1; i <= larg; i++) px(cx + s * i, y + 5 + (i * i) / (larg * 2 + 1), i == larg ? K_LAT3 : K_LAT2);
    int brilha = ((int)(t * 6)) % (velas * 2 + 1);
    for (int k = 0; k < velas; k++) {
        int vx = cx - larg + k * (2 * larg) / (velas - 1);
        int yy = y + 5 + ((vx - cx) * (vx - cx)) / (larg * 2 + 1);
        vl(vx, yy - 2, 2, K_MARF);
        px(vx, yy - 3, aceso ? (((int)(t * 9 + k)) % 4 ? K_LAT4 : K_FOGO2) : K_LAT0);
        px(vx, yy + 1, aceso && k * 2 == brilha ? K_BRANCO : (aceso ? K_LAT4 : K_LAT1));   // o pingente
        px(vx, yy + 2, aceso ? K_LAT3 : K_LAT0);
    }
}
static void nota_mus(int x, int y, uint16_t c)
{
    vl(x + 2, y, 4, c); px(x + 3, y, c); px(x + 4, y + 1, c);
    gfx_rect(x, y + 3, 2, 2, c);
}
static void notas_s(int x, int y, float t, uint16_t c, int n)
{
    for (int i = 0; i < n; i++) {
        float q = frac_(t * 0.4f + i / (float)n);
        int nx = x + (int)(sinf(q * 9 + i) * 3), ny = y - (int)(q * 18);
        if (q < 0.8f) nota_mus(nx, ny, q < 0.55f ? c : abaixo(c));
    }
}

// Caca-niquel de 14 x 22: janela de marfim com tres simbolos e a alavanca.
static void caca_g(int x, int yF, float t, int k)
{
    float ciclo = fmodf(t + k * 1.3f, 3.4f);
    bool gira = ciclo < 1.2f;
    int top = yF - 22;
    escurece_px(x + 3, yF); for (int i = 2; i < 14; i++) escurece_px(x + i, yF);
    gfx_rect(x + 2, top, 10, 2, K_VIN1); hl(x + 3, top, 8, K_VERM);
    gfx_rect(x, top + 2, 14, 20, K_VIN1);
    gfx_rect(x + 1, top + 2, 11, 19, K_VERM);
    vl(x + 1, top + 2, 19, K_VIN3);
    for (int i = 0; i < 3; i++) px(x + 4 + i * 3, top + 1, ((int)(t * 5) + i + k) % 3 == 0 ? K_LAT4 : K_LAT1);
    gfx_rect(x + 2, top + 5, 10, 7, K_ACO);
    gfx_rect(x + 3, top + 6, 8, 5, K_MARF);
    static const uint16_t SIMB[4] = { K_VERM, K_LAT3, K_PLACA, K_VERDE };
    for (int r = 0; r < 3; r++) {
        bool roda = gira && ciclo < 0.5f + r * 0.3f;
        int rx = x + 3 + r * 3;
        if (roda) { for (int j = 0; j < 5; j++) px(rx + 1, top + 6 + j, SIMB[((int)(t * 18) + r + j) % 4]); }
        else { int s = (k + r * 2 + (int)(t / 3.4f)) % 4; gfx_rect(rx, top + 7, 2, 3, SIMB[s]); }
    }
    gfx_rect(x + 3, top + 14, 8, 2, K_PLACA);
    hl(x + 2, top + 17, 10, K_LAT1);
    hl(x, yF - 1, 14, K_VIN0);
    bool puxa = ciclo < 0.25f;
    int ly = puxa ? top + 11 : top + 4;
    vl(x + 15, ly, top + 12 - ly, K_ACO);
    gfx_disco(x + 15, ly - 1, 1, K_VERM);
}

// --- parede e chao de cada andar -----------------------------------------------
static void parede_g(const sala_t *S)
{
    int x0 = S->x, y0 = S->y, w = S->w, hp = S->h - (S->h < 30 ? 2 : FB);
    bool ac = S->ac;
    float t = S->t;
    for (int j = 0; j < hp; j++)
        for (int i = 0; i < w; i++) {
            int u = x0 + i, v = y0 + j;
            uint16_t c = K_EB;
            switch (S->d) {
            case 0: {                                     // azulejo de y = 30 para baixo; pintura em cima
                int faixa = hp > 40 ? 30 : hp / 2;
                if (j >= faixa + 2) {
                    int a = (u - x0) % 6, b = (j - faixa - 2) % 6;
                    c = (a == 0 || b == 0) ? K_MARF3 : K_MARF2;
                    if (!ac) c = c == K_MARF3 ? K_PARDO : K_CINZA;
                } else if (j >= faixa) c = ((u + j) & 1) ? K_AGUA : (ac ? K_AGUA2 : K_AGUA);
                else c = ac ? K_EB2 : K_EB;
                break;
            }
            case 1: {                                     // tabuas de 8 com junta de 1
                int a = (u - x0) % 8;
                c = a == 0 ? K_MAD0 : K_MAD1;
                break;
            }
            case 2: {                                     // tijolo 6 x 3, rejunte escuro
                int lin = j / 4, a = (u - x0 + (lin & 1) * 3) % 7, b = j % 4;
                c = (b == 3 || a == 0) ? K_EB : K_VIN0;
                if (c == K_VIN0 && (hsh((u - x0 + (lin & 1) * 3) / 7, lin) % 12) == 0) c = K_VIN1;
                break;
            }
            case 3: {                                     // chapas escuras com rebite
                int a = (u - x0) % 18, b = j % 13;
                c = (a == 0 || b == 0) ? K_FERRO : K_CHUMBO;
                if ((a == 3 || a == 15) && (b == 3 || b == 10)) c = K_ACO;
                if (j < 2 || j >= hp - 2) c = ac ? K_AZ1 : K_AZ0;     // a fita de LED
                break;
            }
            case 4: {                                     // trelica em losango, passo 8
                int a = (u - x0) % 8, b = j % 8;
                c = (a == b || a == 7 - b) ? K_EB2 : K_EB;
                break;
            }
            case 5: {                                     // damasco 8 x 12 em dois tons e lambri de 12
                if (j >= hp - 12) {
                    int a = (u - x0) % 16;
                    c = j == hp - 12 ? K_LAT2 : j == hp - 11 ? K_MAD0 : (a == 0 ? K_MAD0 : K_MAD1);
                    if (a > 2 && a < 14 && j > hp - 10 && j < hp - 2) c = (a == 3 || j == hp - 9) ? K_MAD0 : (a == 13 || j == hp - 3) ? K_MAD2 : K_MAD1;
                } else {
                    int a = (u - x0) % 8, b = j % 12;
                    int m = abs(a - 4) * 3 + abs(b - 6);
                    c = (m == 6 || (a == 4 && (b == 2 || b == 10))) ? K_LAT1 : K_LAT0;
                }
                break;
            }
            case 6: {                                     // raios chapados a partir do semicirculo dourado
                int cx = x0 + 68, cy = y0 + hp;
                float dx = (float)(u - cx) + 0.5f, dy = (float)(cy - v);
                float r = sqrtf(dx * dx + dy * dy), ang = atan2f(dy, dx);
                float rr = ang / 0.2618f;
                if (r < 12) c = r < 11 ? (r < 7 ? K_LAT3 : K_LAT2) : K_LAT1;
                else c = (fabsf(frac_(rr) - 0.5f) > 0.43f) ? K_LAT2 : (((int)rr) & 1) ? K_MAD2 : K_MAD1;
                if (j < 3) c = j == 2 ? K_LAT2 : K_MAD0;
                break;
            }
            case 7: {                                     // pregas de larguras irregulares, sombra a direita
                static const int LARG[4] = { 5, 7, 4, 6 };
                int pos = (u - x0 + (int)lroundf(sinf(j * 0.12f + t * 1.1f) * 0.8f) + 400) % 22, k = 0, acc = 0;
                while (k < 4 && pos >= acc + LARG[k]) acc += LARG[k++];
                int dentro_p = pos - acc, l = LARG[k < 4 ? k : 3];
                c = dentro_p == 0 ? K_VIN2 : dentro_p >= l - 2 ? K_VIN0 : K_VIN1;
                break;
            }
            case 8: {                                     // listras de 6 em dois roxos, lambri escuro
                if (j >= hp - 11) c = j == hp - 11 ? K_LAT0 : K_PLACA;
                else c = ((u - x0) / 6) & 1 ? K_ROX0 : K_ROX1;
                break;
            }
            case 9: {                                     // losangos sutis
                int a = (u - x0) % 10, b = j % 10;
                c = (abs(a - 5) + abs(b - 5) == 5) ? K_PLACA : K_AZ0;
                break;
            }
            default: c = K_PRETO; break;
            }
            px(u, v, c);
        }
    hl(x0, y0, w, K_PRETO);
    if (S->d >= 5 && S->d < 10) hl(x0, y0 + 1, w, K_LAT1);
}

// O chao: a linha junto da parede mais escura, clareando em dois degraus, com
// juntas em perspectiva (12 px no fundo, 16 na frente).
static void chao_g(const sala_t *S)
{
    int fbh = S->h < 30 ? 2 : FB;
    int x0 = S->x, w = S->w, y0 = S->yF - fbh, cx = x0 + w / 2;
    static const uint16_t T[11][4] = {   // fundo, meio, frente, junta
        { K_VIN0, K_VIN1, K_VIN1, K_MARF3 }, { K_MAD0, K_MAD1, K_MAD2, K_MAD0 }, { K_PLACA, K_MAD0, K_MAD1, K_PLACA },
        { K_EB2, K_CHUMBO, K_FERRO, K_EB2 }, { K_EB, K_EB2, K_PARDO, K_EB }, { K_VIN0, K_VIN1, K_VIN1, K_LAT0 },
        { K_PLACA, K_EB, K_EB2, K_MARF3 }, { K_VIN0, K_VIN0, K_VIN1, K_VIN0 }, { K_PLACA, K_ROX0, K_ROX0, K_ROX1 },
        { K_PLACA, K_AZ0, K_AZ0, K_AZ1 }, { K_MAD1, K_MAD2, K_MAD3, K_MAD1 } };
    const uint16_t *c4 = T[S->d];
    for (int j = 0; j < fbh; j++) {
        float passo = 12 + 4.0f * j / (fbh > 1 ? fbh - 1 : 1);
        uint16_t base = j == 0 ? c4[0] : j < 3 ? c4[1] : c4[2];
        for (int i = 0; i < w; i++) {
            int u = x0 + i;
            float k = (u - cx) / passo;
            bool junta = fabsf(k - roundf(k)) * passo < 0.6f;
            uint16_t c = base;
            switch (S->d) {
            case 0: case 6: {                             // xadrez em perspectiva
                int q = (int)floorf(k) + j / 2;
                c = (q & 1) ? (S->d == 0 ? K_MARF3 : K_MARF3) : (S->d == 0 ? K_VIN1 : K_PLACA);
                if (j == 0) c = abaixo(c);
                break;
            }
            case 5: if (((u % 8 == 0) && j % 3 == 1)) c = K_LAT1; break;
            case 9: if (fabsf(sinf(u * 0.21f + j * 0.9f)) < 0.1f) c = c4[3]; break;
            case 7: case 8: break;
            default: if (junta) c = c4[3]; break;
            }
            if (S->d != 0 && S->d != 6 && j > 0 && (j % 3) == 0 && S->d != 7 && S->d != 8 && S->d != 5) c = c4[3];   // as juntas de travessa
            px(u, y0 + j, c);
        }
    }
    hl(x0, y0 - 1, w, K_PRETO);                                     // o rodape
}

// --- as onze salas -----------------------------------------------------------
#define LUZS(S) (S)->luz, (S)->lx, (S)->ly
static void sala0(const sala_t *S)                               // terreo: o boteco
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    int hx, hy;
    // a prateleira com as garrafas de cachaca e o calendario
    hl(x + 4, y + 22, 30, K_MAD2); hl(x + 4, y + 23, 30, K_MAD0);
    for (int i = 0; i < 9; i++) { uint16_t g = i % 3 == 0 ? K_LAT2 : i % 3 == 1 ? K_VERDE : K_MARF3; gfx_rect(x + 6 + i * 3, y + 17, 2, 5, g); px(x + 6 + i * 3, y + 16, K_EB2); px(x + 6 + i * 3, y + 18, ac ? K_BRANCO : g); }
    gfx_rect(x + 40, y + 8, 9, 12, K_MARF); hl(x + 40, y + 8, 9, K_VERM); hl(x + 40, y + 9, 9, K_VERM);
    for (int j = 0; j < 3; j++) for (int i = 0; i < 4; i++) px(x + 41 + i * 2, y + 12 + j * 2, K_PARDO);
    px(x + 44, y + 6, K_EB2); vl(x + 44, y + 7, 1, K_EB2);
    // a TV no suporte, com o brilho azulado na parede
    int tx = x + 62, ty = y + 6;
    if (ac) for (int j = -3; j < 17; j++) for (int i = -3; i < 23; i++) if (i < 0 || i > 19 || j < 0 || j > 13) clareia_px(tx + i, ty + j);
    gfx_rect(tx, ty, 20, 14, K_PLACA);
    gfx_rect(tx + 1, ty + 1, 18, 11, K_EB2);
    gfx_rect(tx + 2, ty + 2, 16, 9, K_VERDE);
    vl(tx + 10, ty + 2, 9, K_VERDE2); px(tx + 10, ty + 6, K_VERDE2);
    int bxp = tx + 3 + (int)((onda(t, 2.4f) * 0.5f + 0.5f) * 13), byp = ty + 3 + (int)((onda(t, 1.3f) * 0.5f + 0.5f) * 6);
    px(bxp, byp, K_BRANCO);
    px(tx + 5 + (int)(onda(t, 1.7f) * 2 + 2), ty + 7, K_VERM); px(tx + 13 - (int)(onda(t, 2.1f) * 2 + 2), ty + 5, K_AZ3);
    bool gol = ((int)(t * 2)) % 7 == 0;
    hl(tx + 3, ty + 2, 5, gol ? K_LAT4 : K_PLACA);                      // o placar piscando no gol
    vl(tx + 9, ty + 14, 3, K_EB2); hl(tx + 6, ty + 17, 8, K_EB2);
    // o balcao vermelho, o garcom de bone e o freguês de bermuda na banqueta
    pes_t gar = P(0.4f, 1.6f + onda(t, 0.8f) * 0.3f);
    gar.cab = C_BONE; gar.roupa = R_COLETE; gar.ac = AC_NADA;
    pessoa(x + 18, yF - 6, gar, LUZS(S), t);
    pes_mao(x + 18, yF - 6, &gar, 1, &hx, &hy); copo(hx - 1, hy - 3, K_MARF3);
    gfx_rect(x + 3, yF - 15, 34, 1, ac ? K_MARF : K_MARF2);
    gfx_rect(x + 3, yF - 14, 34, 1, K_MARF3);
    gfx_rect(x + 3, yF - 13, 34, 11, K_VERM);
    for (int i = 5; i < 34; i += 5) vl(x + 3 + i, yF - 13, 11, K_VIN1);
    vl(x + 3, yF - 13, 11, K_VIN3); hl(x + 3, yF - 3, 34, K_VIN0);
    for (int i = 2; i < 34; i++) escurece_px(x + 3 + i, yF - 1);
    banqueta(x + 40, yF - 1, K_VERM);
    pes_t fre = P(0.2f, 2.0f + onda(t, 3.0f) * 0.3f);
    fre.sent = 1; fre.dir = -1; fre.roupa = R_BERMUDA; fre.cab = C_CARECA; fre.barriga = 1;
    pessoa(x + 43, yF - 3, fre, LUZS(S), t);
    pes_mao(x + 43, yF - 3, &fre, 1, &hx, &hy); copo(hx - 1, hy - 3, K_LAT3); px(hx, hy - 3, K_BRANCO);
    // os caca-niqueis e quem joga
    caca_g(x + w - 34, yF - 2, t, 0);
    caca_g(x + w - 18, yF - 2, t, 1);
    pes_t jog = P(0.3f, fmodf(t + 0, 3.4f) < 0.25f ? 1.9f : 1.2f);
    jog.cab = C_BONE; jog.roupa = R_BERMUDA; jog.dir = 1;
    pessoa(x + w - 41, yF - 1, jog, LUZS(S), t);
    // o teto: fluorescentes (uma falha), o ventilador e a mosca
    for (int k = 0; k < 2; k++) {
        int fx = x + 14 + k * 70;
        bool pisca = k == 1 && ((int)(t * 8)) % 29 < 2;
        hl(fx, y + 1, 20, K_MARF3);
        hl(fx + 1, y + 2, 18, ac && !pisca ? K_BRANCO : K_MARF3);
    }
    int vx = x + 56;
    vl(vx, y + 1, 3, K_EB2);
    gfx_rect(vx - 1, y + 4, 3, 2, K_MAD2);
    int fq = ((int)(t * 12)) % 4;
    static const int L1[4] = { 10, 7, 2, 7 };
    hl(vx - L1[fq], y + 5, 2 * L1[fq] + 1, K_MAD1);
    hl(vx - L1[(fq + 2) % 4], y + 6, 2 * L1[(fq + 2) % 4] + 1, K_MAD0);
    px(vx - 12 + fq * 6, y + 1, K_PLACA);                               // a sombra das pas no teto
    int mx = x + 30 + (int)(sinf(t * 3.1f) * 5), my = y + 6 + (int)(cosf(t * 4.3f) * 2);
    px(mx, my, K_PRETO);
    frente(S, 3, false);
}

static void sala1(const sala_t *S)                               // 1o: madeira, dardos e carteado
{
    int x = S->x, y = S->y, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    int hx, hy;
    // o quadro de paisagem
    int qx = x + 38, qy = y + 7;
    gfx_rect(qx, qy, 18, 12, K_LAT1); hl(qx, qy, 18, K_LAT2); vl(qx, qy, 12, K_LAT2);
    gfx_rect(qx + 1, qy + 1, 16, 10, K_LAT0);
    gfx_rect(qx + 2, qy + 2, 14, 5, K_AZ1); gfx_rect(qx + 2, qy + 7, 14, 3, K_VERDE);
    for (int i = 0; i < 14; i++) { int hm = 4 - abs((i % 8) - 4); vl(qx + 2 + i, qy + 7 - hm, hm, K_FERRO); if (hm == 4) px(qx + 2 + i, qy + 3, K_BRANCO); }
    // o alvo (9 px) com o dardo cravando e vibrando
    int ax = x + 14, ay = y + 14;
    gfx_disco(ax, ay, 4, K_PLACA); gfx_disco(ax, ay, 3, K_VERM); gfx_disco(ax, ay, 2, K_MARF2); gfx_disco(ax, ay, 1, K_VERDE); px(ax, ay, K_VERM);
    float ciclo = fmodf(t, 2.6f);
    int jx = x + 32;
    float om = ciclo < 0.3f ? 2.5f : ciclo < 0.42f ? 2.5f - (ciclo - 0.3f) * 12 : 1.1f;
    pes_t jog = P(0.3f, om);
    jog.dir = -1; jog.cd = ciclo < 0.3f ? 1.4f : 0.2f; jog.roupa = R_COLETE; jog.cab = C_FEDORA; jog.ac = AC_GRAVATA; jog.cor = gfx_na_paleta(COR_DIF[1]);
    pessoa(jx, yF - 2, jog, LUZS(S), t);
    if (ciclo > 0.4f && ciclo < 0.7f) {
        float q = (ciclo - 0.4f) / 0.3f;
        pes_mao(jx, yF - 2, &jog, 1, &hx, &hy);
        int dx = hx + (int)((ax + 2 - hx) * q), dy = hy + (int)((ay - hy) * q - sinf(q * 3.14f) * 5);
        hl(dx, dy, 3, K_LAT3); px(dx + 3, dy, K_VERM);
    }
    int vib = ciclo > 0.7f && ciclo < 0.8f ? ((int)(ciclo * 60) & 1) : 0;
    for (int i = 0; i < (ciclo > 0.7f ? 3 : 2); i++) { px(ax + 1 + i, ay - 1 + i, K_LAT3); px(ax + 2 + i, ay - 1 + i - (i == 2 ? vib : 0), K_VERM); }
    // a mesa de carteado com tres jogadores e o cone de fumaca
    int mx = x + 60;
    pes_t fundo = P(0.9f, 0.9f + onda(t, 2.3f) * 0.2f);
    fundo.cab = C_COCO; fundo.roupa = R_TERNO; fundo.ce = 0.9f; fundo.cd = 0.9f;
    pessoa(mx + 25, yF - 8, fundo, LUZS(S), t);
    mesa_g(mx, yF - 1, 50, ac);
    for (int s = -1; s <= 1; s += 2) {
        pes_t p = P(0.8f, 1.0f);
        p.sent = 1; p.dir = -s; p.cab = s > 0 ? C_COQUE : C_CHEIO; p.roupa = s > 0 ? R_VESTIDO : R_COLETE;
        p.ac = s > 0 ? AC_COLAR : AC_LENCO; p.cor = gfx_na_paleta(COR_DIF[1]); p.ce = 1.0f; p.cd = 1.0f;
        int px0 = s < 0 ? mx - 6 : mx + 56;
        cadeira_g(s < 0 ? px0 - 6 : px0 - 1, yF - 1, -s, K_MAD2, K_VERDE);
        pessoa(px0, yF - 3, p, LUZS(S), t);
        pes_mao(px0, yF - 3, &p, -s, &hx, &hy);
        carta(hx - 1, hy - 3, false); carta(hx, hy - 4, false);
    }
    for (int i = 0; i < 4; i++) carta(mx + 14 + i * 4, yF - 16, i > 1);
    fichas(mx + 34, yF - 14, 3, K_VERM); fichas(mx + 38, yF - 14, 2, K_AZ2);
    for (int k = 0; k < 6; k++) {                                     // a fumaca subindo no cone
        float q = frac_(t * 0.25f + k / 6.0f);
        int fx = mx + 30 + (int)(sinf(q * 8 + k) * 3), fy = yF - 24 - (int)(q * 22);
        if (q < 0.85f && dentro(S, fx, fy)) px(fx, fy, q < 0.5f ? K_MARF3 : K_PARDO);
    }
    for (int k = 0; k < 2; k++) {                                     // as lampadas verdes de bilhar
        int lx = mx + 12 + k * 26;
        vl(lx, y, 6, K_PRETO);
        hl(lx - 2, y + 6, 5, K_VERDE2); hl(lx - 4, y + 7, 9, K_VERDE); hl(lx - 5, y + 8, 11, K_VERDE);
        px(lx - 4, y + 7, K_VERDE2);
        hl(lx - 1, y + 9, 3, ac ? K_LAT4 : K_LAT0);
    }
    frente(S, 2, true);
}

static void sala2(const sala_t *S)                               // 2o: o bar de tijolo
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    int hx, hy;
    // o letreiro BAR: halo chapado de 2 px e o A que falha
    bool falha = fmodf(t, 4.0f) < 0.1f;
    int nx = x + 74, ny = y + 3;
    const char *L[3] = { "B", "A", "R" };
    int ax = nx;
    for (int i = 0; i < 3; i++) {
        bool on = !(i == 1 && falha);
        if (on) for (int dy = -2; dy <= 2; dy++) for (int dx = -2; dx <= 2; dx++) if (abs(dx) + abs(dy) <= 3 && (dx || dy)) gfx_texto(ax + dx, ny + dy, L[i], K_VIN1, 1, true);
        gfx_texto(ax, ny, L[i], on ? K_FOGO2 : K_VIN0, 1, true);
        ax += gfx_largura(L[i], 1);
    }
    // o espelho e as prateleiras atras do balcao
    gfx_rect(x + 4, y + 7, 50, 18, K_EB);
    gfx_rect(x + 5, y + 8, 48, 16, K_EB2);
    for (int i = 0; i < 5; i++) px(x + 12 + i, y + 9 + i, K_PARDO);
    for (int s = 0; s < 2; s++) {
        int sy = y + 15 + s * 8;
        hl(x + 4, sy, 50, K_MAD2); hl(x + 4, sy + 1, 50, K_MAD0);
        for (int i = 0; i < 12; i++) {
            static const uint16_t G[6] = { K_VERDE, K_VIN2, K_LAT2, K_AZ2, K_MAD3, K_AGUA };
            uint16_t g = G[(i + s * 3) % 6];
            int bx = x + 6 + i * 4;
            gfx_rect(bx, sy - 5, 2, 5, g); px(bx, sy - 6, g); px(bx, sy - 7, K_EB2);
            px(bx, sy - 4, acima(g));
        }
    }
    // o barman: a coqueteleira em arco de 3 posicoes
    int fase = ((int)(t * 7)) % 4;
    static const float OM[4] = { 2.4f, 2.7f, 2.95f, 2.7f };
    pes_t bar = P(OM[fase], OM[fase]);
    bar.ce = 0.9f; bar.cd = 0.9f; bar.roupa = R_COLETE; bar.ac = AC_GRAVATA; bar.cor = K_VERM; bar.cab = C_CHEIO;
    pessoa(x + 26, yF - 4, bar, LUZS(S), t);
    pes_mao(x + 26, yF - 4, &bar, 1, &hx, &hy);
    gfx_rect(hx - 1, hy - 4, 3, 5, K_PRATA); px(hx, hy - 5, K_ACO); px(hx - 1, hy - 4, K_BRANCO);
    // o balcao comprido com o reflexo do neon e o apoio de latao
    gfx_rect(x + 2, yF - 16, 58, 1, K_MAD3);
    gfx_rect(x + 2, yF - 15, 58, 1, ac ? K_LAT3 : K_LAT1);
    for (int i = 0; i < 58; i += 3) if (ac && !falha) px(x + 40 + (i % 18), yF - 16, K_FOGO);
    gfx_rect(x + 2, yF - 14, 58, 11, K_MAD1);
    for (int i = 6; i < 58; i += 10) { gfx_rect(x + i - 3, yF - 12, 7, 6, K_MAD0); hl(x + i - 3, yF - 12, 7, K_PLACA); hl(x + i - 3, yF - 7, 7, K_MAD2); }
    hl(x + 2, yF - 3, 58, K_LAT1); hl(x + 2, yF - 2, 58, K_MAD0);
    for (int i = 2; i < 58; i++) escurece_px(x + 2 + i, yF - 1);
    for (int k = 0; k < 2; k++) {                                     // os clientes nas banquetas
        int cx = x + 14 + k * 26;
        banqueta(cx - 3, yF + 0, K_VIN1);
        pes_t c = P(0.3f, k ? 2.5f + onda(t + k, 2.7f) * 0.3f : 1.3f);
        c.sent = 1; c.dir = 1; c.cab = k ? C_RABO : C_FEDORA; c.roupa = k ? R_VESTIDO : R_TERNO; c.cd = 0.9f;
        c.ac = k ? AC_COLAR : AC_FLOR; c.cor = gfx_na_paleta(COR_DIF[2]);
        pessoa(cx, yF - 4, c, LUZS(S), t);
        pes_mao(cx, yF - 4, &c, 1, &hx, &hy); copo(hx - 1, hy - 3, k ? K_VIN2 : K_LAT3);
    }
    // a vitrola com tres faixas de cor e o casal dancando
    int jx = x + w - 30, jy = yF - 26;
    for (int i = 2; i < 18; i++) escurece_px(jx + i, yF - 1);
    gfx_rect(jx, jy + 6, 16, 19, K_MAD2);
    gfx_disco(jx + 8, jy + 7, 8, K_MAD2);
    static const uint16_t AR[3] = { K_VERM, K_LAT3, K_AZ2 };
    for (int r = 0; r < 3; r++) {
        int rr = 7 - r * 2;
        for (int i = -rr; i <= rr; i++) { int yy = jy + 7 - (int)sqrtf((float)(rr * rr - i * i)); px(jx + 8 + i, yy, AR[(r + (int)(t * 3)) % 3]); px(jx + 8 + i, yy + 1, AR[(r + (int)(t * 3)) % 3]); }
    }
    gfx_rect(jx + 3, jy + 10, 10, 5, K_MARF3); for (int i = 0; i < 2; i++) hl(jx + 4, jy + 11 + i * 2, 8, K_EB2);
    for (int j = 0; j < 7; j++) hl(jx + 3, jy + 17 + j, 10, (j & 1) ? K_LAT0 : K_LAT1);
    vl(jx, jy + 6, 19, K_MAD3); vl(jx + 15, jy + 6, 19, K_MAD1); hl(jx, yF - 2, 16, K_MAD0);
    notas_s(jx + 6, jy - 2, t, K_FOGO2, 2);
    int dz = ((int)(t * 2.2f)) & 1;
    pes_t a = P(dz ? 2.3f : 1.3f, dz ? 0.6f : 2.1f), b = P(dz ? 0.6f : 2.1f, dz ? 2.3f : 1.3f);
    a.cab = C_FEDORA; a.ac = AC_LENCO; a.cor = K_VERM;
    b.roupa = R_VESTIDO; b.cab = C_RABO; b.dir = -1; b.ac = AC_COLAR; b.cor = K_LAT4;
    pessoa(jx - 22 + dz, yF - 1, a, LUZS(S), t);
    pessoa(jx - 12 - dz, yF - 1, b, LUZS(S), t);
    for (int k = 0; k < 3; k++) {                                     // as lampadas de filamento
        int lx = x + 64 + k * 10, cabo = 4 + (k * 5) % 7;
        vl(lx, y, cabo, K_PRETO);
        px(lx, y + cabo, ac ? K_LAT4 : K_LAT1); px(lx, y + cabo + 1, ac ? K_FOGO2 : K_LAT0);
    }
    frente(S, 0, true);
}

static void sala3(const sala_t *S)                               // 3o: aco, globo e roleta
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    int hx, hy;
    // os losangos de luz do globo: cinco raios girando, dois pontos em cada
    if (ac)
        for (int b = 0; b < 5; b++) {
            float a = t * 0.5f + b * 1.2566f;
            for (int k = 1; k <= 2; k++) {
                float r = 18 + k * 16;
                int sx = x + w / 2 + 6 + (int)(cosf(a) * r * 1.4f), sy = y + 9 + (int)(fabsf(sinf(a)) * r * 0.8f) + 4;
                if (!dentro(S, sx, sy) || !dentro(S, sx + 1, sy + 1)) continue;
                px(sx, sy, K_AZ3); px(sx + 1, sy, K_AZ3); px(sx, sy + 1, K_AZ3); px(sx + 1, sy + 1, K_PRATA);
            }
        }
    // a roleta com o crupie de rodo e a bolinha orbitando
    int mx = x + 30;
    pes_t cru = P(1.2f + onda(t, 1.9f) * 0.35f, 0.9f);
    cru.roupa = R_COLETE; cru.ac = AC_GRAVATA; cru.cor = K_PRATA; cru.cab = C_CHEIO;
    pessoa(mx + 38, yF - 8, cru, LUZS(S), t);
    pes_mao(mx + 38, yF - 8, &cru, -1, &hx, &hy);
    gfx_linha(hx, hy, hx - 11, hy + 3, K_MAD2); hl(hx - 13, hy + 3, 4, K_MAD3);
    mesa_g(mx, yF - 1, 58, ac);
    int rx = mx + 12, ry = yF - 16;
    for (int i = -7; i <= 7; i++) {
        int seg = (((i + (int)(t * 12)) / 2) % 2 + 2) % 2;
        px(rx + i, ry, seg ? K_VERM : K_PLACA); px(rx + i, ry + 1, seg ? K_VIN1 : K_PLACA);
    }
    hl(rx - 8, ry + 2, 17, K_MAD3); hl(rx - 8, ry + 3, 17, K_MAD1); hl(rx - 7, ry - 1, 15, K_MAD2);
    vl(rx, ry - 4, 3, K_LAT3); px(rx, ry - 5, K_LAT4);
    float bola = t * 3.4f;
    px(rx + (int)(cosf(bola) * 7), ry - (sinf(bola) > 0 ? 1 : 0), K_MARF);
    for (int i = 0; i < 4; i++) fichas(mx + 28 + i * 5, yF - 14, 1 + (i % 3), i & 1 ? K_VERM : K_LAT3);
    for (int k = 0; k < 2; k++) {
        bool vibra = fmodf(t, 3.0f) > 2.4f;
        pes_t p = P(0.3f, vibra && k ? 2.9f : 0.5f);
        p.roupa = k ? R_VESTIDO : R_TERNO; p.cab = k ? C_COQUE : C_CARTOLA; p.dir = k ? -1 : 1;
        p.ac = k ? AC_COLAR : AC_GRAVATA; p.cor = gfx_na_paleta(COR_DIF[3]); p.cd = vibra && k ? 0.2f : 0.35f;
        pessoa(k ? mx + 66 : mx - 7, yF - 1, p, LUZS(S), t);
    }
    for (int k = 0; k < 2; k++) banqueta(x + w - 26 + k * 9, yF - 1, K_PRATA);
    // o globo espelhado
    int gx = x + w / 2 + 6, gy = y + 9;
    vl(gx, y + 1, 4, K_FERRO);
    for (int j = -4; j <= 4; j++)
        for (int i = -4; i <= 4; i++) {
            if (i * i + j * j > 18) continue;
            int f = (((i + (int)(t * 5)) & 1) ^ ((j + 8) & 1));
            uint16_t c = f ? K_PRATA : K_ACO;
            if (i < -1 && j < -1) c = K_BRANCO;
            if (i > 2 || j > 2) c = f ? K_ACO : K_FERRO;
            px(gx + i, gy + j, c);
        }
    // o feixe do globo varrendo o piso
    if (ac) { int fx = x + 10 + (int)(frac_(t * 0.2f) * (w - 20)); for (int i = 0; i < 6; i++) clareia_px(fx + i, yF - 3); for (int i = 1; i < 5; i++) clareia_px(fx + i, yF - 2); }
    frente(S, 0, false);
}

static void sala4(const sala_t *S)                               // 4o: jardim de inverno
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    // a janela em arco com a lua, o peitoril e o luar no chao
    int wx = x + 46, ww = 40, wy = y + 5, wh = S->h - FB - 9;
    for (int j = 0; j < wh; j++)
        for (int i = 0; i < ww; i++) {
            float r = ww / 2.0f, dy = j < r ? r - j : 0, dx = i - r + 0.5f;
            if (dx * dx + dy * dy > r * r) continue;
            uint16_t c = j < wh * 0.55f ? K_PRETO : K_AZ0;
            if ((hsh(wx + i, wy + j) % 53) == 0) c = K_MARF3;
            px(wx + i, wy + j, c);
        }
    gfx_disco(wx + 28, wy + 9, 4, K_MARF2); gfx_disco(wx + 30, wy + 8, 3, K_PRETO);
    vl(wx + ww / 2, wy, wh, K_MAD2); hl(wx, wy + wh / 2, ww, K_MAD2);
    for (int j = 0; j < wh; j++) { float r = ww / 2.0f, dy = j < r ? r - j : 0; int hw = (int)sqrtf(r * r - dy * dy); px(wx + ww / 2 - hw, wy + j, K_MAD2); px(wx + ww / 2 + hw - 1, wy + j, K_MAD2); }
    gfx_rect(wx - 3, wy + wh, ww + 6, 2, K_MAD2); hl(wx - 3, wy + wh, ww + 6, K_MAD3);
    if (ac) for (int j = 0; j < FB; j++) for (int i = -2 - j; i < ww + 2 + j * 2; i++) clareia_px(wx + i + j, yF - FB + j);
    // as trepadeiras (folhas balancando 1 px)
    for (int k = 0; k < 6; k++) {
        int vx = x + 8 + k * 10 + (k > 2 ? 52 : 0);
        if (vx > x + w - 6) break;
        int comp = 10 + (int)(hsh(k, 3) % 10);
        for (int j = 0; j < comp; j++) {
            int ox = ((int)(t * 1.5f + k + j * 0.3f)) % 3 == 0 ? 1 : 0;
            px(vx + ox, y + 2 + j, K_MAD1);
            if (j % 3 == 1) { px(vx + ox - 1, y + 2 + j, K_VERDE); px(vx + ox + 1, y + 2 + j, K_VERDE2); }
        }
    }
    // a palmeira: vaso de 8 x 6 e cinco folhas em arco
    int pv = x + 18;
    for (int i = 0; i < 12; i++) escurece_px(pv - 6 + i, yF - 1);
    gfx_rect(pv - 4, yF - 7, 8, 6, K_MAD3); hl(pv - 5, yF - 7, 10, K_MAD3); vl(pv + 3, yF - 7, 6, K_MAD2); hl(pv - 4, yF - 2, 8, K_MAD1);
    for (int j = 0; j < 16; j++) { px(pv + (j > 10), yF - 8 - j, (j % 3) ? K_MAD2 : K_MAD1); }
    for (int f = 0; f < 5; f++) {
        float a0 = -2.8f + f * 1.3f + ((((int)(t * 1.2f + f)) % 3 == 0) ? 0.05f : 0);
        for (int i = 0; i < 13; i++) {
            int ex = pv + 1 + (int)(cosf(a0) * i * 1.1f), ey = yF - 24 + (int)(sinf(a0) * i * 0.6f) + (i * i) / 13;
            px(ex, ey, K_VERDE); if (i > 2 && i % 2) px(ex, ey - 1, K_VERDE2); if (i > 3) px(ex, ey + 1, K_VERDE);
        }
    }
    // a fonte (12 px) com o jato em 3 quadros e o respingo
    int fx = x + w - 30;
    gfx_rect(fx - 6, yF - 6, 13, 5, K_PARDO); hl(fx - 7, yF - 7, 15, K_MARF3); hl(fx - 6, yF - 2, 13, K_EB2);
    hl(fx - 5, yF - 6, 11, K_AGUA2);
    vl(fx, yF - 15, 8, K_MARF3); gfx_rect(fx - 2, yF - 16, 5, 2, K_MARF3);
    int fq = ((int)(t * 8)) % 3;
    for (int s = -1; s <= 1; s += 2)
        for (int i = 0; i < 6; i++) { int ddx = i * s, ddy = -2 + (i * i) / 3 + (fq == i % 3 ? 1 : 0); px(fx + ddx, yF - 17 + ddy, i % 2 ? K_AZ3 : K_AGUA2); }
    px(fx - 5 + fq * 3, yF - 7, K_BRANCO);
    // o casal na mesinha
    hl(x + 34, yF - 10, 12, K_MARF3); vl(x + 39, yF - 9, 8, K_MAD2); hl(x + 36, yF - 2, 7, K_MAD1);
    copo(x + 35, yF - 14, K_VIN2); copo(x + 42, yF - 14, K_LAT3);
    pes_t a = P(0.4f, 1.0f + onda(t, 2.0f) * 0.2f); a.sent = 1; a.dir = 1; a.cab = C_FEDORA; a.ac = AC_FLOR; a.cor = K_VERDE2;
    pes_t b = P(1.0f, 0.4f); b.sent = 1; b.dir = -1; b.cab = C_COQUE; b.roupa = R_VESTIDO; b.ac = AC_COLAR; b.cor = K_VERDE2;
    pessoa(x + 30, yF - 3, a, LUZS(S), t);
    pessoa(x + 50, yF - 3, b, LUZS(S), t);
    // as lanternas e os vaga-lumes em fases diferentes
    for (int k = 0; k < 3; k++) {
        int lx = x + 30 + k * 32, cabo = 3 + k % 2 * 2;
        vl(lx, y, cabo, K_PRETO);
        gfx_disco(lx, y + cabo + 3, 3, ac ? K_FOGO2 : K_FOGO);
        hl(lx - 3, y + cabo + 3, 7, K_FOGO); px(lx - 1, y + cabo + 1, ac ? K_LAT4 : K_FOGO2);
    }
    for (int k = 0; k < 6; k++) {
        float a2 = t * 0.5f + k * 1.7f;
        int vx = x + 10 + (int)((sinf(a2 * 0.7f + k) * 0.5f + 0.5f) * (w - 20)), vy = y + 14 + (int)((cosf(a2 * 0.9f) * 0.5f + 0.5f) * (S->h - 28));
        if (frac_(t * 0.7f + k * 0.37f) < 0.6f) px(vx, vy, K_LAT4);
    }
    frente(S, 1, true);
}

static void sala5(const sala_t *S)                               // 5o: ouro, espelho e vinte-e-um
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    int hx, hy;
    int lx = x + 52;
    // o espelho reflete o lustre e as costas do crupie, em dois cinzas
    int ex = x + 88, ey = y + 7, ew = 26, eh = S->h - FB - 23;
    gfx_rect(ex - 2, ey - 2, ew + 4, eh + 4, K_LAT1); hl(ex - 2, ey - 2, ew + 4, K_LAT3); vl(ex - 2, ey - 2, eh + 4, K_LAT3);
    gfx_rect(ex, ey, ew, eh, K_EB2);
    gfx_rect(ex, ey + eh - 9, ew, 9, K_EB);
    gfx_disco(ex + 8, ey + eh - 10, 2, K_EB); gfx_rect(ex + 5, ey + eh - 8, 7, 8, K_EB);      // as costas refletidas
    hl(ex + 2, ey + 2, 9, K_PARDO); px(ex + 6, ey + 3, K_PARDO);                                 // o lustre refletido
    for (int i = 0; i < 5; i++) px(ex + 15 + i, ey + 2 + i, K_PARDO);
    gfx_disco(ex + ew / 2, ey - 3, 2, K_LAT3);
    // a mesa de vinte-e-um, o crupie e a carta voando com rastro
    int mx = x + 16;
    pes_t cru = P(1.3f, 1.5f + onda(t, 1.2f) * 0.3f);
    cru.roupa = R_COLETE; cru.ac = AC_GRAVATA; cru.cor = K_VERM; cru.cab = C_CHEIO; cru.ce = 0.9f; cru.cd = 0.9f;
    pessoa(mx + 26, yF - 8, cru, LUZS(S), t);
    mesa_g(mx, yF - 1, 52, ac);
    float c = fmodf(t, 1.6f);
    if (c < 0.5f) {
        int alvo_x = ((int)(t / 1.6f) & 1) ? mx + 44 : mx + 6;
        for (int r = 2; r >= 0; r--) {
            float q = c / 0.5f - r * 0.08f;
            if (q < 0) continue;
            int cx = mx + 28 + (int)((alvo_x - mx - 28) * q), cy = yF - 20 - (int)(sinf(q * 3.14f) * 5);
            if (r) px(cx + 1, cy + 2, r == 1 ? K_MARF2 : K_MARF3); else carta(cx, cy, q < 0.5f);
        }
    }
    for (int i = 0; i < 3; i++) { carta(mx + 4 + i * 2, yF - 17, false); carta(mx + 40 + i * 2, yF - 17, false); }
    for (int i = 0; i < 3; i++) fichas(mx + 18 + i * 6, yF - 14, 2 + i, i == 1 ? K_VERM : K_LAT3);
    for (int s = -1; s <= 1; s += 2) {
        pes_t p = P(0.9f, 1.0f);
        p.sent = 1; p.dir = -s; p.cab = s < 0 ? C_CARTOLA : C_COQUE; p.roupa = s < 0 ? R_CAUDA : R_VESTIDO;
        p.ac = s < 0 ? AC_GRAVATA : AC_COLAR; p.cor = s < 0 ? K_BRANCO : K_LAT4; p.ce = 1.0f; p.cd = 1.0f;
        int px0 = s < 0 ? mx - 6 : mx + 58;
        cadeira_g(s < 0 ? px0 - 6 : px0 - 1, yF - 1, -s, K_MAD1, K_VIN1);
        pessoa(px0, yF - 3, p, LUZS(S), t);
    }
    // a dama de pe, com a tacinha e o colar de perolas
    pes_t d = P(0.3f, 2.3f + onda(t, 3.3f) * 0.12f);
    d.roupa = R_VESTIDO; d.cab = C_COQUE; d.dir = -1; d.ac = AC_COLAR; d.cor = K_BRANCO; d.cd = 1.2f;
    pessoa(x + 80, yF - 2, d, LUZS(S), t);
    pes_mao(x + 80, yF - 2, &d, 1, &hx, &hy); copo(hx - 1, hy - 3, K_LAT4);
    // o reflexo no piso embaixo das coisas claras
    for (int i = 0; i < 10; i++) { if (ac) clareia_px(mx + 20 + i, yF - 1); }
    lustre_g(lx, y + 1, ac, t, 10, 5);
    frente(S, 0, false);
    (void)w;
}

static void sala6(const sala_t *S)                               // 6o: art deco, piano e cantora
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    // o foco oval na cantora
    int cx = x + 68;
    if (ac) {
        for (int yy = y + 3; yy < yF; yy++) {
            float q = (float)(yy - y - 3) / (S->h - 3);
            int hw = 3 + (int)(q * 7);
            for (int i = -hw; i <= hw; i++) if (abs(i) < hw - 1 || bayer(cx + i, yy) < 0.5f) clareia_px(cx + i, yy);
        }
        for (int i = -10; i <= 10; i++) { clareia_px(cx + i, yF - 2); if (abs(i) < 8) clareia_px(cx + i, yF - 3); }
    }
    // o piano de cauda: tampa em diagonal e o brilho de 1 px no tampo
    int pxp = x + 8;
    for (int i = 0; i < 30; i++) escurece_px(pxp + i, yF - 1);
    gfx_rect(pxp, yF - 15, 30, 6, K_PRETO);
    hl(pxp + 2, yF - 15, 26, K_MARF3);
    gfx_linha(pxp + 4, yF - 16, pxp + 26, yF - 27, K_PRETO);
    gfx_linha(pxp + 5, yF - 16, pxp + 27, yF - 27, K_EB);
    vl(pxp + 25, yF - 26, 10, K_EB);
    for (int i = 0; i < 9; i++) px(pxp + 2 + i, yF - 10, (i % 3 == 2) ? K_PRETO : K_MARF);
    vl(pxp + 3, yF - 9, 8, K_PRETO); vl(pxp + 27, yF - 9, 8, K_PRETO); vl(pxp + 15, yF - 9, 6, K_PRETO);
    hl(pxp + 13, yF - 3, 5, K_LAT2);
    hl(x + 38, yF - 8, 8, K_MAD0); vl(x + 39, yF - 7, 6, K_MAD0); vl(x + 44, yF - 7, 6, K_MAD0);
    int tecla = ((int)(t * 7)) & 1;
    pes_t pia = P(1.3f + tecla * 0.2f, 1.3f + (1 - tecla) * 0.2f);
    pia.sent = 1; pia.dir = -1; pia.roupa = R_CAUDA; pia.ce = 0.5f; pia.cd = 0.5f; pia.cab = C_CHEIO;
    pessoa(x + 42, yF - 3, pia, LUZS(S), t);
    notas_s(pxp + 14, yF - 28, t, ac ? K_LAT4 : K_LAT1, 3);
    // a cantora no microfone, com o vestido brilhando
    vl(cx + 6, yF - 18, 17, K_ACO); hl(cx + 3, yF - 1, 7, K_ACO); px(cx + 5, yF - 19, K_PRATA); px(cx + 5, yF - 20, K_PLACA);
    pes_t can = P(2.5f + onda(t, 2.2f) * 0.25f, 1.4f);
    can.roupa = R_VESTIDO; can.cab = C_CHEIO; can.dir = 1; can.cd = 0.9f; can.ac = AC_COLAR; can.cor = K_LAT4;
    pessoa(cx, yF - 1, can, ac ? K_FOGO2 : S->luz, cx, y, t);
    for (int k = 0; k < 2; k++) if (((int)(t * 5) + k * 3) % 4 == 0) px(cx - 2 + k * 3, yF - 10 + k * 3, K_LAT4);
    // o casal dancando
    int dz = ((int)(t * 1.8f)) % 3;
    static const float AE[3] = { 2.4f, 1.6f, 2.8f }, AD[3] = { 1.2f, 2.6f, 0.8f };
    pes_t a = P(AE[dz], AD[dz]), b = P(AD[dz], AE[dz]);
    a.roupa = R_CAUDA; a.cab = C_CARTOLA; a.ac = AC_FLOR; a.cor = K_FOGO2;
    b.roupa = R_VESTIDO; b.cab = C_COQUE; b.dir = -1; b.ac = AC_COLAR; b.cor = K_LAT4;
    pessoa(x + 96 + (dz == 1), yF - 1, a, LUZS(S), t);
    pessoa(x + 106 - (dz == 1), yF - 1, b, LUZS(S), t);
    for (int k = 0; k < 2; k++) {                                     // as luminarias escalonadas
        int lx = x + 30 + k * 64;
        hl(lx - 4, y + 1, 9, K_LAT2); hl(lx - 3, y + 2, 7, K_LAT1); hl(lx - 2, y + 3, 5, K_LAT2); hl(lx - 1, y + 4, 3, ac ? K_LAT4 : K_LAT0);
    }
    frente(S, 1, false);
    (void)w;
}

static void sala7(const sala_t *S)                               // 7o: cortina e a mesa de dados
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    int hx, hy;
    // a sanefa com franja dourada de 3 px
    for (int i = 0; i < w; i++) {
        int b = 3 + (int)(fabsf(sinf(i * 0.16f)) * 3);
        vl(x + i, y + 1, b, K_VIN2); px(x + i, y + 1, K_VERM);
        for (int f = 0; f < 3; f++) if ((i + f) % 2 == 0) px(x + i, y + 1 + b + f, f == 0 ? K_LAT3 : K_LAT1);
    }
    // o canhao de luz no canto, um cone chapado ate a mesa
    gfx_rect(x + 3, y + 8, 5, 4, K_PLACA); px(x + 7, y + 11, K_LAT4);
    if (ac)
        for (int yy = y + 12; yy < yF - 14; yy++) {
            float q = (float)(yy - y - 12) / (yF - 26 - y);
            int c = x + 8 + (int)(q * 46), hw = 2 + (int)(q * 10);
            for (int xx = c - hw; xx <= c + hw; xx++) if (abs(xx - c) < hw - 1 || bayer(xx, yy) < 0.5f) clareia_px(xx, yy);
        }
    // a mesa de dados com borda alta de 3 px
    int mx = x + 28, mw = 64;
    pes_t pal = P(0.9f, 1.3f);
    pal.roupa = R_COLETE; pal.ac = AC_GRAVATA; pal.cor = K_LAT4; pal.cab = C_CHEIO; pal.cd = 0.4f;
    pessoa(mx + mw / 2, yF - 8, pal, LUZS(S), t);
    pes_mao(mx + mw / 2, yF - 8, &pal, 1, &hx, &hy);
    gfx_linha(hx, hy, hx + 14, hy + 2, K_MAD2);
    mesa_g(mx, yF - 1, mw, ac);
    gfx_rect(mx, yF - 16, mw, 3, K_MAD1); hl(mx, yF - 16, mw, K_MAD3); hl(mx + 2, yF - 14, mw - 4, feltro_tons[3]);
    // os dois dados quicando em parabola e parando numa face
    float c = fmodf(t, 2.6f);
    for (int k = 0; k < 2; k++) {
        float q = c / 1.0f;
        int dx, dy, face;
        if (q < 1) {
            float b = q * 3;
            int quique = (int)b;
            float fb = b - quique;
            dx = mx + 6 + (int)(q * (mw - 20)) + k * 5;
            dy = yF - 18 - (int)(4 * (1 - quique * 0.35f) * 4 * fb * (1 - fb)) - k;
            face = ((int)(q * 14) + k) % 6;
        } else { dx = mx + mw - 14 + k * 5; dy = yF - 18 - k; face = k ? 5 : 3; }
        gfx_rect(dx, dy - 3, 4, 4, K_MARF); hl(dx, dy - 3, 4, K_BRANCO); vl(dx + 3, dy - 2, 3, K_MARF3);
        if (face == 0 || face == 2 || face == 4) px(dx + 1, dy - 2 + 1, K_PLACA);
        if (face >= 1) { px(dx, dy - 3, K_PLACA); px(dx + 2, dy - 1, K_PLACA); }
        if (face >= 3) { px(dx + 2, dy - 3, K_PLACA); px(dx, dy - 1, K_PLACA); }
    }
    bool vibra = c > 1.05f && c < 2.0f;
    pes_t lan = P(0.3f, c < 0.25f ? 2.2f + c * 2 : 1.3f);
    lan.dir = 1; lan.cab = C_FEDORA; lan.roupa = R_TERNO; lan.ac = AC_FLOR; lan.cor = K_VERM;
    pessoa(mx - 7, yF - 1, lan, LUZS(S), t);
    pes_t v1 = P(vibra ? 2.9f : 0.4f, vibra ? 2.9f : 0.4f);
    v1.roupa = R_VESTIDO; v1.cab = C_RABO; v1.dir = -1; v1.ce = vibra ? 0.1f : 0.35f; v1.cd = v1.ce; v1.ac = AC_COLAR; v1.cor = K_LAT4;
    pessoa(mx + mw + 7, yF - 1, v1, LUZS(S), t);
    pes_t v2 = P(0.5f, vibra ? 3.0f : 0.6f);
    v2.cab = C_CARTOLA; v2.roupa = R_CAUDA; v2.cd = vibra ? 0.1f : 0.35f; v2.ac = AC_GRAVATA; v2.cor = K_VERM;
    pessoa(mx + mw + 20, yF - 1, v2, LUZS(S), t);
    // o cordao de veludo, no primeiro plano
    for (int k = 0; k < 2; k++) {
        int px0 = x + 4 + k * 12;
        vl(px0, yF - 10, 10, K_LAT3); px(px0, yF - 11, K_LAT4); hl(px0 - 1, yF - 1, 3, K_LAT1); vl(px0 + 1, yF - 9, 8, K_LAT1);
        if (k < 1) for (int i = 1; i < 12; i++) { px(px0 + i, yF - 9 + (int)(sinf(i / 12.0f * 3.14f) * 3), K_VERM); px(px0 + i, yF - 8 + (int)(sinf(i / 12.0f * 3.14f) * 3), K_VIN1); }
    }
    (void)w;
}

static void sala8(const sala_t *S)                               // 8o: o lounge do sax
{
    int x = S->x, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    int hx, hy;
    // abajures de franja com halo chapado de 12 px
    for (int k = 0; k < 2; k++) {
        int lx = x + 10 + k * 70;
        if (ac) for (int j = -6; j <= 6; j++) for (int i = -6; i <= 6; i++) if (i * i + j * j <= 36) clareia_px(lx + i, yF - 30 + j);
        vl(lx, yF - 26, 25, K_LAT1); hl(lx - 3, yF - 1, 7, K_LAT0);
        for (int j = 0; j < 6; j++) hl(lx - 2 - j / 2, yF - 33 + j, 5 + j, ac ? K_ROX3 : K_ROX2);
        hl(lx - 5, yF - 27, 11, K_LAT2);
        for (int i = -5; i <= 5; i += 2) px(lx + i, yF - 26 + (((int)(t * 6) + i) & 1), K_LAT2);   // a franja tremendo
    }
    // o sofa com bracos e capitone
    int sx = x + 18;
    pes_t a = P(0.5f, 1.5f); a.sent = 1; a.dir = 1; a.cab = C_FEDORA; a.ac = AC_LENCO; a.cor = K_ROX3; a.cd = 1.0f;
    pes_t b = P(0.4f, 0.4f); b.sent = 1; b.dir = 1; b.cab = C_CHEIO; b.roupa = R_VESTIDO; b.ac = AC_COLAR; b.cor = K_LAT4;
    gfx_rect(sx, yF - 17, 40, 8, K_ROX1); hl(sx, yF - 17, 40, K_ROX2);
    for (int i = 3; i < 40; i += 4) for (int j = 0; j < 2; j++) px(sx + i + j * 2, yF - 15 + j * 3, K_ROX0);
    pessoa(sx + 12, yF - 4, a, LUZS(S), t);
    pessoa(sx + 28, yF - 4, b, LUZS(S), t);
    gfx_rect(sx - 2, yF - 10, 44, 6, K_ROX1); hl(sx - 2, yF - 10, 44, K_ROX2); hl(sx - 2, yF - 5, 44, K_ROX0);
    gfx_rect(sx - 5, yF - 15, 5, 11, K_ROX1); hl(sx - 5, yF - 15, 5, K_ROX2); gfx_rect(sx + 40, yF - 15, 5, 11, K_ROX1); hl(sx + 40, yF - 15, 5, K_ROX2);
    vl(sx - 3, yF - 4, 3, K_MAD0); vl(sx + 42, yF - 4, 3, K_MAD0);
    for (int i = -4; i < 46; i++) escurece_px(sx + i, yF - 1);
    pes_mao(sx + 12, yF - 4, &a, 1, &hx, &hy);
    bool brasa = ((int)(t * 2)) % 3 != 0;
    hl(hx, hy - 1, 3, K_MAD2); px(hx + 3, hy - 1, brasa ? K_FOGO2 : K_FOGO);
    for (int k = 0; k < 10; k++) {                                    // a fita de fumaca
        float q = frac_(t * 0.3f) + k * 0.05f;
        int fx = hx + 3 + (int)(sinf(q * 10) * 2) + k / 3, fy = hy - 2 - k * 2;
        if (frac_(t * 0.3f) < 0.7f || k < 4) px(fx, fy, k < 5 ? K_ROX3 : K_ROX2);
    }
    // a mesinha com a vela
    int mx = x + 64;
    hl(mx, yF - 9, 10, K_LAT2); vl(mx + 5, yF - 8, 7, K_LAT0); hl(mx + 2, yF - 1, 7, K_LAT0);
    vl(mx + 3, yF - 12, 3, K_MARF); px(mx + 3, yF - 13 - ((int)(t * 8) & 1), ((int)(t * 11)) % 3 ? K_LAT4 : K_FOGO2);
    copo(mx + 6, yF - 13, K_VIN2);
    // o palco e o saxofonista (sobe e desce 1 px, uma nota de cada vez)
    int px0 = x + w - 36;
    gfx_rect(px0, yF - 5, 30, 4, K_MAD1); hl(px0, yF - 5, 30, K_LAT1); hl(px0, yF - 2, 30, K_MAD0);
    int bob = ((int)(t * 2.5f)) & 1;
    pes_t sax = P(1.9f, 2.1f);
    sax.dir = -1; sax.cab = C_FEDORA; sax.ce = 1.1f; sax.cd = 1.1f; sax.ac = AC_GRAVATA; sax.cor = K_ROX3;
    int sxp = px0 + 16;
    pessoa(sxp, yF - 5 - bob, sax, LUZS(S), t);
    gfx_linha(sxp - 1, yF - 26 - bob, sxp - 3, yF - 13 - bob, K_LAT3);
    gfx_linha(sxp - 3, yF - 13 - bob, sxp - 7, yF - 14 - bob, K_LAT3);
    px(sxp - 7, yF - 15 - bob, K_LAT4); px(sxp - 8, yF - 14 - bob, K_LAT2);
    float nq = frac_(t * 0.6f);
    if (nq < 0.8f) nota_mus(sxp - 12 + (int)(sinf(nq * 6) * 2), yF - 20 - (int)(nq * 14), ac ? K_ROX3 : K_ROX2);
    frente(S, 2, false);
}

static void sala9(const sala_t *S)                               // 9o: aquario e poquer
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    bool ac = S->ac;
    int hx, hy;
    // o aquario na base de madeira, com a luz dele no chao
    int ax = x + 10, ay = y + 6, aw = 52, ah = S->h - FB - 22;
    gfx_rect(ax - 2, ay - 2, aw + 4, ah + 4, K_LAT1); hl(ax - 2, ay - 2, aw + 4, K_LAT3);
    const uint16_t AG[] = { K_AGUA2, K_AGUA, K_AZ1 };
    degrade_v(ax, ay, aw, ah, AG, 3);
    gfx_rect(ax, ay + ah - 3, aw, 3, K_LAT1); hl(ax, ay + ah - 3, aw, K_LAT2);
    gfx_rect(ax - 2, ay + ah + 2, aw + 4, 5, K_MAD1); hl(ax - 2, ay + ah + 2, aw + 4, K_MAD2); hl(ax - 2, ay + ah + 6, aw + 4, K_MAD0);
    if (ac) for (int j = 0; j < FB; j++) for (int i = 0; i < aw + j * 2; i++) if (bayer(ax + i, yF - FB + j) < 0.5f || i % 3) { uint16_t p = pega(ax - j + i, yF - FB + j); px(ax - j + i, yF - FB + j, p == K_AZ0 || p == K_PLACA ? K_AZ1 : p); }
    for (int k = 0; k < 5; k++) { int px0 = ax + 4 + k * 10; for (int j = 0; j < 6 + k % 3 * 2; j++) px(px0 + (int)lroundf(sinf(j * 0.6f + t * 2 + k)), ay + ah - 4 - j, j % 2 ? K_VERDE : K_VERDE2); }
    // o cardume de tres virando junto, e um peixe sozinho
    float q = frac_(t * 0.09f);
    bool ida = q < 0.5f;
    float pp = ida ? q * 2 : 2 - q * 2;
    for (int k = 0; k < 3; k++) {
        int fx = ax + 4 + (int)(pp * (aw - 14)) + (k == 1 ? 4 : 0), fy = ay + 4 + k * 3 + (int)lroundf(sinf(t * 2 + k) * 0.8f);
        gfx_rect(fx, fy, 4, 2, K_FOGO2); px(ida ? fx - 1 : fx + 4, fy, K_FOGO); px(ida ? fx - 1 : fx + 4, fy + 1, K_FOGO); px(ida ? fx + 3 : fx, fy, K_PRETO);
    }
    float q2 = frac_(t * 0.06f + 0.4f);
    int f2 = ax + 3 + (int)((q2 < 0.5f ? q2 * 2 : 2 - q2 * 2) * (aw - 9));
    gfx_rect(f2, ay + ah - 9, 4, 2, K_LAT4); px(q2 < 0.5f ? f2 - 1 : f2 + 4, ay + ah - 9, K_LAT3);
    for (int k = 0; k < 5; k++) { float b = frac_(t * 0.5f + k / 5.0f); px(ax + aw - 6 + (int)(sinf(b * 12 + k) * 1.5f), ay + ah - 4 - (int)(b * (ah - 6)), K_AZ3); }
    for (int i = 0; i < 4; i++) px(ax + 2 + i, ay + 1 + i, K_BRANCO);
    // a mesa de poquer (mais para o centro)
    int mx = x + 62;
    pes_t cru = P(1.3f, 1.3f + onda(t, 1.4f) * 0.2f);
    cru.roupa = R_COLETE; cru.ac = AC_GRAVATA; cru.cor = K_AZ3; cru.cab = C_CHEIO; cru.ce = 0.9f; cru.cd = 0.9f;
    pessoa(mx + 22, yF - 8, cru, LUZS(S), t);
    mesa_g(mx, yF - 1, 46, ac);
    for (int i = 0; i < 5; i++) carta(mx + 9 + i * 5, yF - 17, false);
    fichas(mx + 4, yF - 14, 4, K_AZ2); fichas(mx + 38, yF - 14, 3, K_LAT3);
    pes_t p = P(0.9f, 1.1f); p.sent = 1; p.dir = -1; p.cab = C_CARTOLA; p.roupa = R_CAUDA; p.ce = 1.0f; p.cd = 1.0f; p.ac = AC_GRAVATA; p.cor = K_BRANCO;
    cadeira_g(mx + 47, yF - 1, -1, K_MAD1, K_AZ1);
    pessoa(mx + 51, yF - 3, p, LUZS(S), t);
    // o garcom: atravessa, para, serve e volta
    float g = fmodf(t, 6.0f);
    float pos = g < 2 ? g / 2 : g < 3 ? 1 : g < 5 ? 1 - (g - 3) / 2 : 0;
    int gx = x + 22 + (int)(pos * 26);
    pes_t gar = P(0.2f, g >= 2 && g < 3 ? 1.6f : 1.9f);
    gar.anda = (g < 2 || (g >= 3 && g < 5)) ? (int)(t * 6) + 1 : 0;
    gar.dir = g < 3 ? 1 : -1; gar.cd = 1.3f; gar.roupa = R_COLETE; gar.ac = AC_GRAVATA; gar.cor = K_BRANCO;
    pessoa(gx, yF, gar, K_AGUA2, ax, ay, t);
    pes_mao(gx, yF, &gar, 1, &hx, &hy);
    hl(hx - 3, hy - 1, 7, K_PRATA); copo(hx - 2, hy - 5, K_LAT4); copo(hx + 1, hy - 5, K_VIN2);
    lustre_g(x + w / 2 + 22, y + 1, ac, t, 7, 4);
    frente(S, 0, true);
}

static void sala10(const sala_t *S)                              // cobertura: o terraco
{
    int x = S->x, y = S->y, w = S->w, yF = S->yF;
    float t = S->t;
    int hp = S->h - FB;
    int hx, hy;
    // o ceu em tres faixas chapadas
    for (int j = 1; j < hp; j++) {
        float q = (float)j / hp;
        uint16_t c = q < 0.38f ? K_PRETO : q < 0.62f ? K_AZ0 : K_ROX0;
        if ((q > 0.36f && q < 0.40f) || (q > 0.60f && q < 0.64f)) c = (((x + j) & 1) ? (q < 0.5f ? K_AZ0 : K_ROX0) : c);
        hl(x, y + j, w, c);
    }
    for (int i = 0; i < w; i++) for (int j = 2; j < hp * 0.6f; j++) { uint32_t qh = hsh(x + i, y + j + 99); if (qh % 71 == 0) px(x + i, y + j, ((qh >> 9) & 3) == 0 && ((int)(t * 2 + (qh & 7)) % 3 == 0) ? K_BRANCO : K_MARF3); }
    float est = fmodf(t, 7.0f);                                     // a estrela cadente (rara)
    if (est < 0.4f) { int sx = x + 90 - (int)(est * 120), sy = y + 4 + (int)(est * 20); for (int k = 0; k < 4; k++) px(sx + k, sy - k / 2, k ? K_MARF3 : K_BRANCO); }
    // a cidade la embaixo, em silhueta com janelinhas
    uint32_t s = 3;
    int cy0 = y + hp - 12;
    for (int bx = x; bx < x + w;) {
        s = s * 1103515245u + 12345u;
        int bw = 5 + (int)((s >> 16) % 8), bh = 4 + (int)((s >> 8) % 9);
        for (int j = 0; j < bh; j++)
            for (int i = 0; i < bw && bx + i < x + w; i++) {
                uint16_t c = K_PRETO;
                if (j > 0 && (i % 2) == 1 && (j % 2) == 1 && (hsh(bx + i, j) % 3) == 0) c = K_LAT3;
                px(bx + i, cy0 + 12 - bh + j, c);
            }
        bx += bw + 1;
    }
    // os fogos: subida de 1 px, estouro em 8 raios, o brilho no piso
    float ciclo = fmodf(t, 2.4f);
    uint16_t cor_f = K_LAT4;
    bool estouro = false;
    if (ciclo < 1.4f) {
        int n = (int)(t / 2.4f);
        int fx = x + 24 + (int)(hsh(n, 1) % (uint32_t)(w - 48)), fy = y + 7 + (int)(hsh(n, 2) % 8);
        cor_f = gfx_na_paleta(COR_DIF[1 + hsh(n, 3) % 10]);
        if (ciclo < 0.35f) { int sy = y + hp - 12 - (int)(ciclo / 0.35f * (hp - 12 - (fy - y))); px(fx, sy, K_LAT4); px(fx, sy + 1, K_FOGO); }
        else {
            estouro = ciclo < 0.9f;
            float q = (ciclo - 0.35f) / 1.05f;
            for (int k = 0; k < 8; k++) {
                float a = k * 0.785f;
                for (int r = (int)(q * 10) - 2; r <= (int)(q * 10); r++) {
                    if (r < 1) continue;
                    int ex = fx + (int)lroundf(cosf(a) * r), ey = fy + (int)lroundf(sinf(a) * r) + (int)(q * q * 5);
                    px(ex, ey, r == (int)(q * 10) && q < 0.6f ? K_BRANCO : cor_f);
                }
            }
        }
    }
    for (int k = 1; k < 4; k++) vl(x + k * w / 4, y + 1, hp - 1, K_LAT1);   // so 4 montantes
    // o deck, a mureta, a torre de tacas com champanhe e a luneta
    gfx_rect(x, yF - FB - 4, w, 4, K_MARF3); hl(x, yF - FB - 4, w, K_MARF2); hl(x, yF - FB - 1, w, K_PARDO);
    if (estouro) for (int i = 0; i < w; i += 2) px(x + i, yF - 3, cor_f);
    int tx = x + 54;
    hl(tx - 2, yF - 11, 22, K_MARF2); hl(tx - 2, yF - 10, 22, K_MARF3); vl(tx + 9, yF - 9, 8, K_LAT1); hl(tx + 5, yF - 2, 9, K_LAT1);
    for (int i = 0; i < 20; i++) escurece_px(tx + i - 1, yF - 1);
    for (int lin = 0; lin < 4; lin++)
        for (int k = 0; k <= lin; k++) {
            int cx = tx + 9 - lin * 2 + k * 4, cy = yF - 15 - (3 - lin) * 4;
            hl(cx - 1, cy, 3, K_MARF2); px(cx, cy + 1, K_LAT4); px(cx, cy + 2, K_MARF3);
        }
    float esc = frac_(t * 0.8f);                                    // o champanhe escorrendo
    px(tx + 9 + (esc < 0.5f ? 2 : -2), yF - 26 + (int)(esc * 14), K_LAT4);
    int lx = x + w - 26;
    gfx_linha(lx, yF - 2, lx + 4, yF - 12, K_LAT0); gfx_linha(lx + 8, yF - 2, lx + 4, yF - 12, K_LAT0); vl(lx + 4, yF - 12, 10, K_LAT0);
    gfx_linha_grossa(lx, yF - 18, lx + 10, yF - 24, 2, K_LAT2); px(lx + 10, yF - 25, K_LAT4); px(lx + 1, yF - 18, K_LAT3);
    // o casal brindando
    bool brinde = fmodf(t, 3.0f) < 0.6f;
    pes_t a = P(0.3f, brinde ? 2.6f : 1.8f); a.dir = 1; a.roupa = R_CAUDA; a.cab = C_CARTOLA; a.cd = 0.9f; a.ac = AC_GRAVATA; a.cor = K_BRANCO;
    pes_t b = P(brinde ? 2.6f : 1.8f, 0.3f); b.roupa = R_VESTIDO; b.cab = C_COQUE; b.dir = -1; b.ce = 0.9f; b.ac = AC_COLAR; b.cor = K_LAT4;
    pessoa(x + 22, yF - 1, a, K_MARF, x + 30, y, t);
    pessoa(x + 36, yF - 1, b, K_MARF, x + 30, y, t);
    pes_mao(x + 22, yF - 1, &a, 1, &hx, &hy); copo(hx - 1, hy - 3, K_LAT4);
    pes_mao(x + 36, yF - 1, &b, -1, &hx, &hy); copo(hx - 1, hy - 3, K_LAT4);
    // o varal de lampadinhas acendendo em sequencia
    for (int i = 0; i < w; i++) {
        int yy = y + 3 + (int)(sinf((float)i / w * 3.14159f) * 5);
        if (i % 2 == 0) px(x + i, yy, K_PLACA);
        if (i % 9 == 4) px(x + i, yy + 1, (i / 9) == ((int)(t * 6)) % (w / 9 + 1) ? K_BRANCO : K_LAT3);
    }
    frente(S, 2, false);
}

static uint16_t luz_andar(int d)
{
    static const uint16_t L[N_NIVEIS + 1] = { RGB(204, 194, 172), RGB(190, 156, 86), RGB(238, 162, 74), RGB(168, 204, 230),
        RGB(168, 204, 230), RGB(246, 222, 132), RGB(238, 162, 74), RGB(208, 98, 108), RGB(206, 180, 218),
        RGB(96, 182, 198), RGB(238, 230, 212) };
    return L[d];
}

static bool andar_fechado = false;
static void sala_g(int d, int x, int y, int w, int h, float t, bool aceso)
{
    if (andar_fechado) { aceso = false; sala_vazia = true; }
    sala_t S = { x, y, w, h, y + h, d, t, aceso, luz_andar(d), x + w / 2, y };
    if (!aceso) S.luz = K_EB2;
    if (d < N_NIVEIS) parede_g(&S);
    chao_g(&S);
    switch (d) {                                          // a luz do teto, antes da mobilia
    case 0: cone_chapado(&S, x + 24, y + 3, 0.35f); cone_chapado(&S, x + 94, y + 3, 0.35f); break;
    case 1: cone_chapado(&S, x + 72, y + 10, 0.4f); cone_chapado(&S, x + 98, y + 10, 0.4f); break;
    case 2: cone_chapado(&S, x + 74, y + 10, 0.45f); break;
    case 4: cone_chapado(&S, x + 30, y + 10, 0.35f); cone_chapado(&S, x + 94, y + 10, 0.35f); break;
    case 5: cone_chapado(&S, x + 52, y + 12, 0.5f); break;
    case 8: break;
    case 9: cone_chapado(&S, x + w / 2 + 22, y + 10, 0.4f); break;
    default: break;
    }
    if (d < N_NIVEIS) vinheta(&S);
    switch (d) {
    case 0: sala0(&S); break;
    case 1: sala1(&S); break;
    case 2: sala2(&S); break;
    case 3: sala3(&S); break;
    case 4: sala4(&S); break;
    case 5: sala5(&S); break;
    case 6: sala6(&S); break;
    case 7: sala7(&S); break;
    case 8: sala8(&S); break;
    case 9: sala9(&S); break;
    default: sala10(&S); break;
    }
    if (!aceso) ui_escurece(x, y, w, h, 90);
    if (andar_fechado) { ui_escurece(x, y, w, h, 150); sala_vazia = false; }
}
