// ===========================================================================
// O salao em volta da mesa
// ===========================================================================
// Com a mesa menor que a janela (o - e o + do PC) ou a janela fora de 16:9,
// sobra margem em volta do quadro: a plataforma pede aqui o desenho dela, no
// mesmo tamanho de pixel da mesa (px: w x h, com o quadro do jogo em ox, oy).
// A parede muda com o salao da run - lambri no Verde (e fora da run), papel
// adamascado no Vermelho, veludo com filetes de latao no Dourado, arlequim
// no do Coringa -; por cima, o cone de luz da luminaria sobre a mesa, a luz
// que a mesa espalha na parede, a sombra dela embaixo e uns fios de fumaca
// subindo devagar. A parede so muda com o tamanho ou o salao; a fumaca, a
// cada pedido.

// O salao de agora: o andar da run, ou -1 fora dela (o lambri).
static int salao_andar(void)
{
    if (modo != M_DESAFIO || menu_sem_partida()) return -1;
    return run.andar < DES_ANDARES + 1 ? run.andar : 0;
}

// A rampa de cada parede, do escuro ao claro, so com cores da paleta (a
// rampa automatica de algumas destas cai num cinza no tom de baixo).
static const uint16_t SAL_RAMPA[4][4] = {
    { RGB(36, 32, 29), RGB(56, 32, 20), RGB(78, 46, 30), RGB(120, 78, 50) },     // lambri
    { RGB(36, 32, 29), RGB(65, 16, 24), RGB(115, 28, 32), RGB(150, 62, 80) },    // adamascado vinho
    { RGB(14, 14, 12), RGB(24, 20, 16), RGB(36, 32, 29), RGB(62, 56, 50) },      // veludo
    { RGB(24, 20, 16), RGB(49, 36, 74), RGB(102, 74, 138), RGB(148, 113, 164) }, // arlequim
};
#define SAL_FILETE 9                         // o filete de latao do veludo: nao pega a luz
static uint16_t sal_r[4];                    // a rampa da parede de agora
static uint16_t sal_tom(int k) { return sal_r[k < 0 ? 0 : k > 3 ? 3 : k]; }

// O tom da parede (0..3) em (x, y): o desenho de cada salao, sem luz, com a
// base no 1 (a luz sobe dali).
static int parede_tom(int andar, int x, int y, int h)
{
    switch (andar) {
    case 1: {                                // papel adamascado: losangos com uma flor no meio
        int u = x % 18 - 9, v = y % 24 - 12, d = (u < 0 ? -u : u) * 4 + (v < 0 ? -v : v) * 3;
        if ((d >= 30 && d <= 33) || d <= 6) return 2;
        return 1;
    }
    case 2: {                                // veludo: dobras verticais, filete de latao a cada 64
        if (x % 64 == 0) return SAL_FILETE;
        if (x % 64 == 1) return 0;
        float s = sinf(x * 0.31f) * 0.5f + sinf(x * 0.11f + 1.3f) * 0.5f;
        return s + bayer(x, y) * 0.9f > 0.55f ? 2 : 1;
    }
    case 3: {                                // arlequim: losangos alternados, com a costura
        float a = x / 20.0f + y / 30.0f, b = x / 20.0f - y / 30.0f;
        int i = (int)floorf(a), j = (int)floorf(b);
        if (a - i < 0.05f || b - j < 0.05f) return 0;
        return ((i + j) & 1) ? 1 : 0;
    }
    default: {                               // lambri: tabuas em pe, veio, o rodameio e os paineis embaixo
        int rm = h * 62 / 100;
        if (y >= rm && y < rm + 4) return y == rm ? 3 : y == rm + 3 ? 0 : 2;   // o rodameio
        if (y > rm + 4) {                    // os paineis de baixo, com a moldura
            int px = x % 56, py = (y - rm - 4) % 70;
            if (px == 6 || py == 8) return 0;
            if (px == 50 || py == 62) return 2;
        }
        if (x % 24 == 0) return 0;           // a junta entre as tabuas
        if (ruido_xy(x, y / 9) % 9 == 0) return 0;     // o veio, em pe
        return 1;
    }
    }
}

static uint16_t *sal_fixo;
static int sal_w, sal_h, sal_ox, sal_oy, sal_andar = -9;

// A parede com a luz, guardada: o cone da luminaria de cima, a luz que a mesa
// espalha em volta dela e a sombra embaixo.
static void pinta_parede(int w, int h, int ox, int oy, int andar)
{
    memcpy(sal_r, SAL_RAMPA[andar < 0 ? 0 : andar > 3 ? 3 : andar], sizeof sal_r);
    float cx = ox + GFX_W / 2.0f;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int k = parede_tom(andar, x, y, h);
            if (k == SAL_FILETE) { sal_fixo[y * w + x] = y % 3 ? C_LATAO_ESC : C_LATAO; continue; }
            // O cone: estreito na luminaria, abre ate a largura da mesa.
            float l = 0;
            if (y < oy) {
                float meia = 22 + (GFX_W / 2.0f + 30 - 22) * y / (float)(oy > 0 ? oy : 1);
                float dx = fabsf(x - cx) / meia;
                if (dx < 1) l += (1 - dx) * 1.1f;
            }
            // A mesa acesa espalha luz na parede em volta, mais forte perto.
            float ex = x < ox ? ox - x : x >= ox + GFX_W ? x - (ox + GFX_W - 1) : 0;
            float ey = y < oy ? oy - y : y >= oy + GFX_H ? y - (oy + GFX_H - 1) : 0;
            float dist = sqrtf(ex * ex + ey * ey);
            if (dist < 30) l += (1 - dist / 30) * 0.9f;
            // A sombra da mesa, embaixo dela e um pouco dos lados.
            if (y >= oy + GFX_H && y < oy + GFX_H + 18 && x > ox - 10 && x < ox + GFX_W + 10)
                l -= 1.3f * (1 - (y - oy - GFX_H) / 18.0f);
            int dk = (int)floorf(l + bayer(x, y));
            sal_fixo[y * w + x] = sal_tom(k + dk);
        }
    // A luminaria: a cupula de latao no alto, sobre o meio da mesa, com o fio.
    if (oy >= 26) {
        uint16_t rl[4];
        rampa_de(C_LATAO, rl);
        int lx = (int)cx, ly = oy / 3 > 8 ? oy / 3 : 8;
        for (int y = 0; y < ly - 8; y++) sal_fixo[y * w + lx] = C_BARRA;     // o fio
        for (int j = 0; j < 9; j++) {
            int meia = 6 + j * 2;
            for (int i = -meia; i <= meia; i++) {
                int x = lx + i, y = ly - 8 + j;
                if (x < 0 || x >= w || y < 0 || y >= h) continue;
                uint16_t c = i == -meia || i == meia || j == 0 ? C_BARRA
                           : tom_rampa(rl, 0.7f - (i + meia) / (float)meia * 0.8f, x, y);
                sal_fixo[y * w + x] = c;
            }
        }
        for (int i = -22; i <= 22; i++)                 // a boca da cupula, acesa
            if (lx + i >= 0 && lx + i < w && ly + 1 < h) sal_fixo[(ly + 1) * w + lx + i] = abs(i) < 20 ? C_AMARELO : C_BARRA;
    }
}

void dado_salao(uint16_t *px, int w, int h, int ox, int oy)
{
    int andar = salao_andar();
    if (w <= 0 || h <= 0) return;
    if (!sal_fixo || w != sal_w || h != sal_h || ox != sal_ox || oy != sal_oy || andar != sal_andar) {
        uint16_t *n = realloc(sal_fixo, (size_t)w * h * sizeof(uint16_t));
        if (!n) return;
        sal_fixo = n; sal_w = w; sal_h = h; sal_ox = ox; sal_oy = oy; sal_andar = andar;
        pinta_parede(w, h, ox, oy, andar);
    }
    memcpy(px, sal_fixo, (size_t)w * h * sizeof(uint16_t));
    // A fumaca: fios subindo devagar, balancando, que clareiam a parede um
    // tom com pontilhado ralo. So na parede, nunca por cima da mesa.
    float t = t_relogio;
    for (int f = 0; f < 4; f++) {
        float bx = f < 2 ? ox * (0.3f + 0.4f * f) : w - ox * (0.3f + 0.4f * (f - 2));
        if (ox < 12) bx = ox + GFX_W * (0.2f + 0.2f * f);      // margem estreita: os fios sobem pelo alto
        float sobe = fmodf(t * 9 + f * 97, (float)h + 120) - 60;   // onde o fio esta, subindo
        for (int y = 0; y < h; y++) {
            float ate = (y - (h - sobe)) / 90.0f;              // o fio tem uns 180 px, mais forte no meio
            if (ate < -1 || ate > 1) continue;
            float forca = (1 - fabsf(ate)) * 0.45f;
            float xc = bx + sinf(y * 0.045f + t * 0.7f + f * 1.7f) * (10 + f * 2);
            for (int x = (int)xc - 3; x <= (int)xc + 3; x++) {
                if (x < 0 || x >= w) continue;
                if (x >= ox && x < ox + GFX_W && y >= oy && y < oy + GFX_H) continue;
                float perto = 1 - fabsf(x - xc) / 3.5f;
                if (bayer(x, y) < forca * perto) {
                    uint16_t *p = &px[y * w + x];
                    for (int k = 0; k < 3; k++) if (*p == sal_r[k]) { *p = sal_r[k + 1]; break; }
                }
            }
        }
    }
}
