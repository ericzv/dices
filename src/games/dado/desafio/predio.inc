// ===========================================================================
// A tela de Dificuldade como o predio do cassino: cada nivel e um andar
// (Terreo embaixo, Cobertura em cima), em corte. O andar escolhido abre em
// sanfona e acende; os outros ficam fechados numa faixa fina com uma pista do
// que tem la dentro. Os andares que ainda nao abriram ficam de luz apagada,
// vazios, com a corrente na porta do poco. Ao lado, a botoeira de latao do
// elevador: um botao por andar, o escolhido aceso; o trancado tem cadeado e,
// se apertado, fica vermelho e o cadeado treme. A cabine do elevador sobe e
// desce no poco ate o andar escolhido.
// ===========================================================================
#define TECLA_TRANCADO 0x210                       // o clique num botao trancado (+ o andar)

// A estrela de "ja venceu uma run aqui": 5 x 5 em latao claro, contorno
// escuro e o brilho em cima a esquerda. A mesma na plaquinha e no botao.
static void estrela_venceu(int cx, int cy)
{
    static const char *const E[5] = { "..X..", ".XXX.", "XXXXX", ".XXX.", ".X.X." };
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < 5; i++)
            if (E[j][i] == 'X')
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) px(cx - 2 + i + dx, cy - 2 + j + dy, C_BARRA);
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < 5; i++)
            if (E[j][i] == 'X') px(cx - 2 + i, cy - 2 + j, K_LAT4);
    px(cx - 1, cy - 1, K_BRANCO);
}

// --- a sala fechada (11 px): o papel, o chao e uma pista do que tem la ----
static void gentinha(int x, int yF, uint16_t c)
{
    gfx_rect(x + 1, yF - 7, 2, 2, c);
    gfx_rect(x, yF - 5, 4, 3, c);
    vl(x + 1, yF - 2, 2, c); vl(x + 2, yF - 2, 2, c);
}
static void sala_mini(int d, int x, int y, int w, int h, float t)
{
    sala_t S = { x, y, w, h, y + h, d, t, true, luz_andar(d), x + w / 2, y };
    int yF = y + h;
    if (d < N_NIVEIS) parede_g(&S);
    else {
        const uint16_t C[] = { K_PRETO, K_AZ0 };
        degrade_v(x, y, w, h - 2, C, 2);
        for (int i = 0; i < w; i += 9) px(x + i + (int)(hsh(i, d) % 7), y + 2 + (int)(hsh(i, 3) % 5), K_MARF3);
        for (int i = 0; i < w; i += 21) vl(x + i, y, h - 2, K_LAT1);
    }
    chao_g(&S);
    hl(x, y, w, K_PLACA);
    switch (d) {
    case 0: px(x + 24, y + 2, K_MARF); px(x + 94, y + 2, K_MARF);                 // o saguao: os globos, o guiche e a mesa
            gfx_rect(x + 5, yF - 8, 30, 6, K_MAD1); hl(x + 9, yF - 7, 22, K_LAT2); hl(x + 5, yF - 8, 30, K_MAD2);
            hl(x + 84, yF - 5, 26, feltro_tons[4]); break;
    case 1: gfx_disco(x + 12, y + 4, 2, K_VERM); px(x + 12, y + 4, K_MARF);
            px(x + 74, y + 2, K_VERDE2); px(x + 100, y + 2, K_VERDE2); hl(x + 62, yF - 5, 50, feltro_tons[4]); break;
    case 2: hl(x + 70, y + 2, 9, K_FOGO2); hl(x + 6, y + 3, 40, K_VERDE); for (int i = 0; i < 40; i += 3) px(x + 6 + i, y + 3, i % 2 ? K_LAT2 : K_VIN2);
            gfx_rect(x + 2, yF - 6, 58, 4, K_MAD1); gfx_rect(x + w - 16, yF - 9, 8, 7, K_MAD2); px(x + w - 13, yF - 8, ((int)(t * 6)) & 1 ? K_VERM : K_LAT3); break;
    case 3: gfx_rect(x + w / 2 + 5, y + 2, 3, 3, K_PRATA); px(x + w / 2 + 5, y + 2, K_BRANCO); hl(x + 26, yF - 5, 56, feltro_tons[4]);
            hl(x + 34, yF - 6, 9, ((int)(t * 8)) & 1 ? K_VERM : K_PLACA); break;
    case 4: gfx_rect(x + 50, y + 2, 30, 5, K_AZ0); px(x + 72, y + 3, K_MARF2); for (int i = 0; i < 6; i++) px(x + 14 + i, yF - 4 - i % 3, K_VERDE2); break;
    case 5: hl(x + w / 2 - 4, y + 2, 9, K_LAT3); px(x + w / 2, y + 3, K_BRANCO); hl(x + 14, yF - 5, 52, feltro_tons[4]); break;
    case 6: gfx_rect(x + 6, yF - 6, 26, 3, K_PRETO); px(x + 20, yF - 9, K_LAT4); break;
    case 7: hl(x + 26, yF - 5, 66, feltro_tons[4]); px(x + 50, yF - 6, K_MARF); px(x + 54, yF - 6, K_MARF); break;
    case 8: px(x + 6, y + 3, K_ROX3); px(x + 78, y + 3, K_ROX3); gfx_rect(x + 14, yF - 6, 40, 4, K_ROX1); break;
    case 9: gfx_rect(x + 6, y + 2, 40, 6, K_AGUA2); px(x + 10 + (int)(frac_(t * 0.1f) * 34), y + 4, K_FOGO2); hl(x + 70, yF - 5, 46, feltro_tons[4]); break;
    default: hl(x + 52, yF - 5, 20, K_MARF2); px(x + 62, yF - 7, K_LAT3); break;
    }
    // A gente de cada andar: de 1 a 3, cada andar no seu lugar; a primeira
    // anda 1 px a cada 0,8 s, para la e para ca.
    int n = 1 + (int)(hsh(d, 7) % 3);
    for (int i = 0; i < n; i++) {
        int gx = 8 + (int)(hsh(d, i + 11) % (unsigned)(w - 20));
        if (i == 0) { int pa = (int)(t / 0.8f) % 12; gx += pa < 6 ? pa : 12 - pa; }
        gentinha(x + gx, yF - 2, K_PLACA);
    }
    ui_escurece(x, y, w, h, 70);
}
// A faixa de um andar trancado: so a persiana fechada, sem cor nem gente.
static void sala_trancada(int x, int y, int w, int h)
{
    gfx_rect(x, y, w, h, K_PLACA);
    for (int j = 1; j < h - 1; j += 2) hl(x, y + j, w, K_EB);
    hl(x, y, w, K_PRETO);
    hl(x, y + h - 1, w, K_PRETO);
}


// --- o predio ---------------------------------------------------------------
#define G_PIL 13                                   // o pilar da esquerda, com as plaquinhas
#define G_SH  17                                   // o poco do elevador
#define G_WI  2                                    // a parede entre o poco e as salas
#define G_RM  128                                  // as salas
#define G_WR  4                                    // a parede da direita
#define G_ABRE  66                                 // a sala aberta
#define G_FECHA 11                                 // a sala fechada
typedef struct { int x0, base, nv, fy0[N_NIVEIS + 1], fy1[N_NIVEIS + 1]; } pg_t;
static int g_sh(const pg_t *p) { return p->x0 + G_PIL; }
static int g_rm(const pg_t *p) { return p->x0 + G_PIL + G_SH + G_WI; }
static int g_x1(const pg_t *p) { return g_rm(p) + G_RM + G_WR; }

// O estado da tela: quanto cada andar esta aberto e aceso, onde a cabine
// esta (com mola), o botao recem-apertado e o trancado que foi negado.
static float pr_abre[N_NIVEIS + 1], pr_luz[N_NIVEIS + 1];
static float pr_cab = -1, pr_cab_v, pr_t_ant = -1, pr_t_visto = -1;
static int   pr_sel_ant = -1;
static float pr_aperto_t = -9, pr_chegada_t = -9;
static bool  pr_chegou;
static int   pr_sala_y = -1, pr_parede_x = -1;     // o meio do andar aberto e a parede direita (para o fio)

static void g_monta(pg_t *p, const int *alt)
{
    int y = p->base;
    for (int d = 0; d <= p->nv; d++) {
        p->fy1[d] = y - 3;
        p->fy0[d] = p->fy1[d] - alt[d];
        y = p->fy0[d];
    }
}

static void g_parede(int x, int y0, int y1, int W, bool esq)
{
    for (int y = y0; y < y1; y++)
        for (int i = 0; i < W; i++) {
            bool junta = (y % 4) == 3 || ((x + i + ((y / 4) & 1) * 2) % 4) == 0;
            uint16_t c = junta ? K_VIN0 : K_VIN1;
            if (i == 0) c = esq ? K_VIN2 : K_VIN1;
            if (i == W - 1) c = K_VIN0;
            if (esq && i == 1) c = K_PARDO;
            if (esq && i == 2) c = K_EB2;
            px(x + i, y, c);
        }
}

static void g_plaquinha(const pg_t *p, int d, bool sel, bool trancado, float t)
{
    static const char *const M[N_NIVEIS + 1] = { "T", "1", "2", "3", "4", "5", "6", "7", "8", "9", "C" };
    uint16_t c = trancado ? K_CINZA : gfx_na_paleta(COR_DIF[d]), r[4];
    rampa_de(c, r);
    int h = p->fy1[d] - p->fy0[d] >= 13 ? 11 : 9;
    int x = p->x0 + 1, w = 11, y = (p->fy0[d] + p->fy1[d]) / 2 - h / 2;
    ui_contorno(x, y, w, h, C_BARRA);
    uint16_t rr[4];
    rampa_de(sel ? C_OURO : C_LATAO, rr);
    if (!sel) rampa_de(rr[1], rr);
    if (trancado) rampa_de(rr[0], rr);
    float k[4] = { 2, 2, 1, 0 };
    ui_aro(x + 1, y + 1, w - 2, h - 2, 1, rr, k);
    gfx_rect(x + 2, y + 2, w - 4, h - 4, sel ? tinta_escura(c) : K_PLACA);
    gfx_texto(x + w / 2 - gfx_largura(M[d], GFX_MIUDO) / 2, y + (h - 9) / 2 - 2, M[d], sel ? r[3] : c, GFX_MIUDO, false);
    if (perfil()->venceu[d]) estrela_venceu(x + w - 2, y + 1);
    (void)t;
}

static void g_poco(const pg_t *p, int ytop, float cab, float t)
{
    int x = g_sh(p), y1 = p->base;
    for (int y = ytop; y < y1; y++)
        for (int i = 0; i < G_SH; i++) {
            bool tij = (y % 4) == 3 || ((x + i + ((y / 4) & 1) * 2) % 5) == 0;
            px(x + i, y, tij ? K_PRETO : K_PLACA);
        }
    vl(x + 2, ytop, y1 - ytop, K_CHUMBO); vl(x + G_SH - 3, ytop, y1 - ytop, K_CHUMBO);
    for (int d = 0; d <= p->nv; d++) {
        hl(x, p->fy1[d], G_SH, K_EB2);
        bool aqui = fabsf(cab - d) < 0.5f;
        px(x + G_SH - 1, p->fy1[d] - 13 > p->fy0[d] ? p->fy1[d] - 13 : p->fy0[d] + 1, aqui ? K_FOGO2 : K_VIN0);
    }
    int fl = (int)floorf(cab);
    if (fl < 0) fl = 0;
    if (fl > p->nv) fl = p->nv;
    int fc = fl + 1 <= p->nv ? fl + 1 : fl;
    float fr = cab - fl;
    int cyb = (int)lroundf(p->fy1[fl] + (p->fy1[fc] - p->fy1[fl]) * fr);
    int cw = 13, ch = 15, cx = x + 2, cy = cyb - ch;
    vl(cx + cw / 2, ytop, cy - ytop - 2, K_FERRO);              // o cabo, so acima da cabine
    {                                                           // o contrapeso: desce quando a cabine sobe
        int y0 = ytop + 4, y1c = y1 - 10;
        int py = y1c - (cyb - y0) * (y1c - y0) / (y1 - y0 > 0 ? y1 - y0 : 1);
        if (py < y0) py = y0;
        if (py > y1c) py = y1c;
        vl(x + G_SH - 2, ytop, py - ytop, K_CHUMBO);
        gfx_rect(x + G_SH - 3, py, 3, 6, K_FERRO);
        px(x + G_SH - 3, py, K_ACO);
        hl(x + G_SH - 3, py + 5, 3, K_EB);
    }
    ui_contorno(cx - 1, cy - 1, cw + 2, ch + 2, C_BARRA);
    const uint16_t D[3] = { K_LAT4, K_LAT3, K_LAT1 };
    degrade_v(cx + 1, cy + 2, cw - 2, ch - 3, D, 3);
    gentinha(cx + cw / 2 - 2, cyb - 1, K_PLACA);
    for (int j = 0; j < ch - 3; j++)
        for (int i = 0; i < cw - 2; i++)
            if (((i + j) % 4) == 0 || ((i - j + 64) % 4) == 0) px(cx + 1 + i, cy + 2 + j, K_LAT0);
    hl(cx, cy, cw, K_LAT3); hl(cx, cy + 1, cw, K_LAT1);
    vl(cx, cy, ch, K_LAT2); vl(cx + cw - 1, cy, ch, K_LAT0); hl(cx, cy + ch - 1, cw, K_LAT0);
    hl(cx + cw / 2 - 2, cy - 2, 5, K_LAT2); px(cx + cw / 2, cy - 3, K_LAT3);
    (void)t;
}

static void g_telhado(const pg_t *p, float t)
{
    int top = p->fy0[p->nv], x0 = p->x0, x1 = g_x1(p);
    hl(x0, top - 3, x1 - x0, K_PARDO); hl(x0, top - 2, x1 - x0, K_EB2); hl(x0, top - 1, x1 - x0, K_PLACA);
    int py = top - 7;
    gfx_rect(x0 - 1, py, x1 - x0 + 2, 4, K_PARDO);
    hl(x0 - 1, py, x1 - x0 + 2, K_LAT2); hl(x0 - 1, py + 3, x1 - x0 + 2, K_EB2);
    for (int i = x0 + 2; i < x1 - 2; i += 6) px(i, py + 2, K_CINZA);
    int mx = g_sh(p) - 1, mw = G_SH + 2, mh = 12, my = py - mh;
    gfx_rect(mx, my, mw, mh, K_EB2); hl(mx, my, mw, K_PARDO); vl(mx, my, mh, K_PARDO); vl(mx + mw - 1, my, mh, K_EB);
    for (int j = 0; j < 3; j++) hl(mx + 3, my + 3 + j * 2, 6, K_EB);
    gfx_rect(mx + 11, my + 4, 5, 8, K_PLACA);
    int cx = x1 - 26, cw = 14, ch = 11, cy = py - 7 - ch;
    vl(cx + 2, py - 7, 7, K_EB); vl(cx + cw - 3, py - 7, 7, K_EB); hl(cx + 1, py - 4, cw - 2, K_EB);
    for (int i = 0; i < cw; i++) vl(cx + i, cy, ch, i == 0 ? K_MAD3 : i == cw - 1 ? K_MAD0 : (i % 3 == 0 ? K_MAD1 : K_MAD2));
    hl(cx, cy + 3, cw, K_EB); hl(cx, cy + ch - 3, cw, K_EB);
    for (int j = 0; j < 4; j++) hl(cx + j - 1, cy - 1 - j, cw - 2 * j + 2, j == 0 ? K_MAD0 : K_MAD1);
    // o letreiro em neon, com uma letra que falha
    const char *s = "CASSINO";
    int lw = gfx_largura(s, 1), lx = (x0 + x1) / 2 - lw / 2 - 8, ly = py - 26;
    for (int i = lx + 3; i < lx + lw - 2; i += 8) vl(i, ly + 15, py - (ly + 15), K_FERRO);
    hl(lx - 2, ly + 15, lw + 4, K_FERRO);
    int apaga = ((int)(t * 2.3f)) % 9 == 0 ? 3 : -1, ax = lx;
    for (int i = 0; s[i]; i++) {
        char l[2] = { s[i], 0 };
        bool on = i != apaga;
        if (on) for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) if (dx || dy) gfx_texto(ax + dx, ly + dy, l, K_VIN1, 1, true);
        gfx_texto(ax, ly, l, on ? K_FOGO2 : K_VIN0, 1, true);
        ax += gfx_largura(l, 1);
    }
}


// O ceu da noite atras do predio, com as estrelas e a cidade la longe.
static void ceu(int x, int y, int w, int h, float t)
{
    const uint16_t C[] = { K_PRETO, K_AZ0, K_AZ0, K_ROX0 };
    degrade_v(x, y, w, h, C, 4);
    for (int j = 0; j < h * 6 / 10; j++)
        for (int i = 0; i < w; i++) {
            uint32_t q = hsh(x + i, y + j);
            if (q % 97 == 0) {
                bool brilha = (q >> 8) % 5 == 0 && ((int)(t * 2 + (q & 7)) % 4 == 0);
                px(x + i, y + j, brilha ? K_BRANCO : ((q >> 12) & 1) ? K_MARF3 : K_CINZA);
            }
        }
    // a cidade la atras
    int sx = x;
    uint32_t s = 7;
    while (sx < x + w) {
        s = s * 1103515245u + 12345u;
        int bw = 8 + (int)((s >> 16) % 14), bh = 16 + (int)((s >> 8) % 40);
        int by = y + h - bh;
        for (int j = by; j < y + h; j++)
            for (int i = sx; i < sx + bw && i < x + w; i++) {
                uint16_t c = K_AZ0;
                if (j > by + 2 && ((i - sx) % 3) == 1 && ((j - by) % 4) == 1 && (hsh(i, j) % 5) == 0) c = (hsh(i, j) >> 4) % 3 ? K_LAT0 : K_LAT1;
                px(i, j, c);
            }
        hl(sx, by, bw, K_ROX0);
        sx += bw + (int)((s >> 4) % 3);
    }
}

static void calcada(int x, int w, int y, int h)
{
    hl(x, y, w, K_CINZA);
    gfx_rect(x, y + 1, w, 3, K_PARDO);
    for (int i = x + 3; i < x + w; i += 9) vl(i, y + 1, 3, K_EB2);
    hl(x, y + 4, w, K_EB);
    gfx_rect(x, y + 5, w, h - 5, K_PLACA);
    for (int i = x + 4; i < x + w; i += 12) hl(i, y + 8, 5, K_PARDO);   // a faixa da rua
}


// O lugar das pecas na tela: o predio, a botoeira ao lado e o painel.
#define PRED_X  20
#define PRED_Y  54
#define PRED_W  178
#define PRED_H  254
#define PRED_BX 202                                  // a botoeira
#define PRED_BW 30

// O predio em sanfona: o andar escolhido abre grande e acende (piscando um
// pouco, como lampada velha), os outros fecham numa faixa fina.
static uint16_t pr_guarda[G_ABRE * G_RM];          // o que a sala abrindo cobriria em cima
static void predio_g(int sel, int aberto, float t, float dt)
{
    int nv = N_NIVEIS;
    int bx = PRED_X, by = PRED_Y, bw = PRED_W, bh = PRED_H;
    int alt[N_NIVEIS + 1];
    for (int k = 0; k <= nv; k++) {
        // A luz so acende com a sala aberta e a cabine ja no andar; a de
        // antes apaga logo (segurar a seta nao deixa rastro aceso).
        float alvo_l = k == sel && pr_abre[k] > 0.9f && k <= aberto && fabsf(pr_cab - (float)sel) < 0.15f ? 1.0f : 0.0f;
        if (alvo_l > pr_luz[k]) pr_luz[k] += dt / 0.45f;
        else if (alvo_l < pr_luz[k]) pr_luz[k] -= dt / 0.1f;   // acesa de vez fica acesa (sem tremer)
        if (pr_luz[k] < 0) pr_luz[k] = 0;
        if (pr_luz[k] > 1) pr_luz[k] = 1;
        alt[k] = G_FECHA + (int)lroundf(pr_abre[k] * (G_ABRE - G_FECHA));
    }
    pg_t p;
    memset(&p, 0, sizeof p);
    p.nv = nv;
    p.x0 = bx + 4;
    p.base = by + bh - 12;
    g_monta(&p, alt);
    ui_painel(bx, by, bw, bh);
    ceu(bx + 2, by + 2, bw - 4, p.base - by - 2, t);
    calcada(bx + 2, bw - 4, p.base, by + bh - 2 - p.base);
    int ytop = p.fy0[nv] - 3;
    uint16_t *a = alvo();
    for (int d = 0; d <= nv; d++) {
        int y = p.fy0[d], h = p.fy1[d] - p.fy0[d];
        bool aceso = pr_luz[d] > 0.98f || (pr_luz[d] > 0.05f && (hsh((int)(t * 20), d) % 100) < pr_luz[d] * 100);
        andar_fechado = d > aberto;
        if (h > 24) {                                          // abrindo: a sala inteira, cortada em cima
            int x = g_rm(&p), y0 = p.fy1[d] - G_ABRE, n = 0;
            for (int yy = y0; yy < y; yy++, n++)
                if (yy >= 0 && yy < GFX_H) memcpy(pr_guarda + n * G_RM, a + yy * GFX_W + x, G_RM * 2);
            sala_g(d, x, y0, G_RM, G_ABRE, t, aceso);
            n = 0;
            for (int yy = y0; yy < y; yy++, n++)
                if (yy >= 0 && yy < GFX_H) memcpy(a + yy * GFX_W + x, pr_guarda + n * G_RM, G_RM * 2);
            if (h < G_ABRE) hl(x, y, G_RM, K_PLACA);
        } else if (d > aberto) sala_trancada(g_rm(&p), y, G_RM, h);
        else sala_mini(d, g_rm(&p), y, G_RM, h, t);
        andar_fechado = false;
        // a laje embaixo da sala
        hl(g_rm(&p), p.fy1[d], G_RM, K_PARDO); hl(g_rm(&p), p.fy1[d] + 1, G_RM, K_EB2); hl(g_rm(&p), p.fy1[d] + 2, G_RM, K_PLACA);
        int wx = g_rm(&p) - G_WI;                                  // a porta do andar no poco
        for (int yy = y; yy < p.fy1[d]; yy++) {
            bool porta = yy >= p.fy1[d] - 14 && h >= 16;
            bool brilha = porta && d == sel && t - pr_chegada_t < 2 / 60.0f;   // a porta acende no quadro em que a cabine chega
            px(wx, yy, brilha ? K_LAT4 : porta ? K_LAT2 : K_PARDO); px(wx + 1, yy, brilha ? K_LAT3 : porta ? K_LAT0 : K_EB2);
        }
        hl(wx, p.fy1[d], G_WI, K_EB2);
        if (d > aberto) {                                          // fechado: a corrente na porta do poco
            int sx = g_sh(&p) + G_SH - 7, sy = p.fy1[d] - 7;
            bool pisca = d == negado_d && t - negado_t < 0.15f;
            for (int i = 0; i < 8; i++) px(sx + i, sy + (i < 4 ? i / 2 : (7 - i) / 2), pisca ? K_VERM : (i & 1) ? K_FERRO : K_ACO);
            gfx_rect(sx + 2, sy + 2, 4, 3, K_LAT1); px(sx + 3, sy + 1, K_FERRO); px(sx + 4, sy + 1, K_FERRO);
        }
    }
    g_poco(&p, ytop, pr_cab, t);
    g_parede(p.x0, ytop, p.base, G_PIL, true);
    g_parede(g_rm(&p) + G_RM, ytop, p.base, G_WR, false);
    {                                                          // a parede do andar aceso vira latao, com a luz
        int wx0 = g_rm(&p) + G_RM;
        for (int yy = p.fy0[sel]; yy < p.fy1[sel]; yy++)
            for (int i = 0; i < G_WR; i++) {
                if (bayer(wx0 + i, yy) >= pr_luz[sel]) continue;
                bool junta = (yy % 4) == 3 || ((wx0 + i + ((yy / 4) & 1) * 2) % 4) == 0;
                px(wx0 + i, yy, i == G_WR - 1 ? K_LAT0 : junta ? K_LAT1 : K_LAT2);
            }
        pr_sala_y = (p.fy0[sel] + p.fy1[sel]) / 2;
        pr_parede_x = g_x1(&p);
    }
    for (int d = 0; d <= nv; d++) {
        int yy = p.fy1[d];
        hl(p.x0 - 1, yy, G_PIL + 1, K_PARDO); hl(p.x0 - 1, yy + 1, G_PIL + 1, K_EB2); hl(p.x0 - 1, yy - 1, G_PIL + 1, K_LAT1);
        hl(g_x1(&p) - 1, yy, 2, K_LAT1);
    }
    g_telhado(&p, t);
    // a entrada do terreo com o toldo
    int ex = g_x1(&p) - 2, ey = p.fy1[0];
    gfx_rect(ex - 2, ey - 14, 4, 14, K_LAT2); gfx_rect(ex - 1, ey - 13, 2, 13, K_LAT4);
    for (int i = 0; i < 8; i++) { uint16_t c = ((i / 2) & 1) ? K_MARF : K_VERM; vl(ex + i - 1, ey - 18, 3, c); if ((i % 4) < 2) px(ex + i - 1, ey - 15, c); }
    for (int d = 0; d <= nv; d++) {
        g_plaquinha(&p, d, d == sel, d > aberto, t);
    }
    ui_cantos(g_rm(&p), p.fy0[sel], G_RM, p.fy1[sel] - p.fy0[sel], 0);
}

// --- a botoeira -------------------------------------------------------------
static const char *const MARCA_E[N_NIVEIS + 1] = { "T", "1", "2", "3", "4", "5", "6", "7", "8", "9", "C" };

// Latao escovado: cada coluna num tom (fixo), com a luz de cima a esquerda.
static void latao_escovado(int x, int y, int w, int h)
{
    for (int i = 0; i < w; i++)
        for (int j = 0; j < h; j++) {
            int u = x + i, v = y + j;
            uint16_t c = K_LAT2;
            if ((hsh(u, (v + (int)(hsh(u, 3) % 16)) / 16) % 23) == 0) c = K_LAT3;   // riscos em pe do escovado
            else if ((hsh(u + 911, (v + (int)(hsh(u, 5) % 12)) / 12) % 31) == 0) c = K_LAT1;
            px(u, v, c);
        }
}
// A placa de latao com chanfro e contorno.
static void placa_latao(int x, int y, int w, int h)
{
    ui_sombra(x, y, w, h);
    latao_escovado(x + 1, y + 1, w - 2, h - 2);
    hl(x + 1, y + 1, w - 2, K_LAT4); hl(x + 2, y + 2, w - 4, K_LAT3);
    vl(x + 1, y + 1, h - 2, K_LAT4); vl(x + 2, y + 2, h - 4, K_LAT3);
    hl(x + 1, y + h - 2, w - 2, K_LAT0); hl(x + 2, y + h - 3, w - 4, K_LAT1);
    vl(x + w - 2, y + 1, h - 2, K_LAT0); vl(x + w - 3, y + 2, h - 4, K_LAT1);
    ui_contorno(x, y, w, h, C_BARRA);
}
static void parafuso(int x, int y, float ang)
{
    gfx_disco(x, y, 3, K_LAT0);
    gfx_disco(x, y, 2, K_LAT2);
    px(x - 1, y - 1, K_LAT4);
    int dx = (int)lroundf(cosf(ang) * 2), dy = (int)lroundf(sinf(ang) * 2);
    gfx_linha(x - dx, y - dy, x + dx, y + dy, K_LAT0);
}

// Um botao redondo de elevador: aro de latao, o miolo escuro (ou aceso, com o
// brilho em volta) e a marca do andar. 'fundo': apertado, afundado 1 px.
static void botao_el(int cx, int cy, int r, const char *rot, float luz, bool fundo)
{
    if (luz > 0.05f) {                                        // o brilho em volta, pontilhado
        int R2 = r + 5;
        for (int y = cy - R2; y <= cy + R2; y++)
            for (int x = cx - R2; x <= cx + R2; x++) {
                float dd = sqrtf((float)((x - cx) * (x - cx) + (y - cy) * (y - cy)));
                if (dd <= r + 0.5f || dd > R2) continue;
                if (bayer(x, y) < (1 - (dd - r) / 5) * 0.75f * luz) px(x, y, K_LAT4);
            }
    }
    int dy = fundo ? 1 : 0;
    gfx_disco(cx + 1, cy + 2, r, K_LAT0);                     // a sombra
    gfx_disco(cx, cy + dy, r, C_BARRA);
    for (int y = -r + 1; y <= r - 1; y++)
        for (int x = -r + 1; x <= r - 1; x++) {
            float dd = sqrtf((float)(x * x + y * y));
            if (dd > r - 0.6f) continue;
            uint16_t c;
            if (dd > r - 2.5f) {                              // o aro
                float k = (-(float)x - (float)y) / (dd + 0.01f);
                c = k > 0.4f ? K_LAT4 : k > -0.2f ? K_LAT3 : k > -0.7f ? K_LAT1 : K_LAT0;
                if (fundo) c = k > 0.4f ? K_LAT1 : K_LAT2;
            } else {
                c = luz > 0.5f ? (dd > r - 3.5f ? K_FOGO2 : K_LAT4) : luz > 0.05f ? K_FOGO : K_PLACA;
                if (luz <= 0.05f && x < 0 && y < 0 && dd > r - 4) c = K_EB;
            }
            px(cx + x, cy + y + dy, c);
        }
    int lw = gfx_largura(rot, GFX_MIUDO_FIXO);
    gfx_texto(cx - lw / 2, cy + dy - 6, rot, luz > 0.5f ? K_PLACA : K_MARF2, GFX_MIUDO_FIXO, false);
}

// O botao do andar i. Trancado: tampa escura com cadeado; apertado assim
// mesmo, fica vermelho e o cadeado treme.
static void botao_andar(int cx, int cy, int r, int i, int sel, int aberto, float t)
{
    if (i > aberto) {                                          // um furo na placa, sem aro, com o cadeado
        float dn = t - negado_t;
        bool nega = negado_d == i && dn < 0.4f;
        int sh = nega ? (int)lroundf(sinf(dn * 88) * 3 * (1 - dn / 0.4f)) : 0;
        gfx_disco(cx, cy, r - 1, nega ? (dn < 0.15f ? K_VERM : K_VIN1) : K_LAT0);   // a beira do furo, na sombra
        gfx_disco(cx + 1, cy + 1, r - 2, K_LAT1);
        gfx_disco(cx + 1, cy + 1, r - 3, K_LAT0);
        icone_cadeado(cx + sh, cy, 1, nega ? K_VERM : K_FERRO);
        if (!nega) px(cx - 2 + sh, cy - 1, K_ACO);
        return;
    }
    float desde = t - pr_aperto_t;
    bool fundo = i == sel && desde < 0.12f;                    // o dedo ainda no botao
    float luz = 0;
    if (i == sel) {
        luz = 0.85f + 0.15f * sinf(t * 4);
        if (desde < 0.2f) luz *= desde / 0.2f;                 // a lampada esquentando
    }
    botao_el(cx, cy, r, MARCA_E[i], luz, fundo);
    if (perfil()->venceu[i]) { gfx_disco(cx + r + 1, cy - r + 1, 3, C_BARRA); estrela_venceu(cx + r + 1, cy - r + 1); }
    if (i == sel) ui_cantos(cx - r - 3, cy - r - 3, 2 * r + 7, 2 * r + 7, 0);
}

// A botoeira em pe, ao lado do predio (C em cima, T embaixo). Cada botao e
// uma zona de clique: o aberto escolhe (e o clique comeca a run); o
// trancado so nega.
static void botoeira_v(int x, int y, int w, int h, int sel, int aberto, float t)
{
    placa_latao(x, y, w, h);
    parafuso(x + w / 2, y + 6, 0.7f); parafuso(x + w / 2, y + h - 7, 2.1f);
    int passo = (h - 24) / (N_NIVEIS + 1);
    for (int i = 0; i <= N_NIVEIS; i++) {
        int cy = y + h - 14 - passo / 2 - i * passo;
        botao_andar(x + w / 2, cy, 8, i, sel, aberto, t);
        if (i <= aberto) zona_clique(x + 2, cy - passo / 2, w - 4, passo, &dif_cur, i, KEY_ENTER);
        else zona_clique(x + 2, cy - passo / 2, w - 4, passo, NULL, 0, TECLA_TRANCADO + i);
    }
}

// O fio de latao: sai da botoeira na altura do botao aceso, desce (ou sobe)
// pela fresta e entra na parede do andar aberto. Acende com a luz do andar.
static void fio_andar(int yb, float luz)
{
    if (pr_sala_y < 0) return;
    int xb = PRED_BX - 1, xc = PRED_BX - 3, ys = pr_sala_y;
    int y0 = yb < ys ? yb : ys, y1 = yb < ys ? ys : yb;
    #define FIO(x, y) do { px(x, y, bayer(x, y) < luz ? K_LAT3 : K_EB2); } while (0)
    for (int x = xc; x <= xb; x++) { FIO(x, yb); px(x, yb + 1, K_LAT0); }
    for (int y = y0; y <= y1; y++) { FIO(xc, y); px(xc + 1, y, K_LAT0); }
    for (int x = pr_parede_x; x <= xc; x++) { FIO(x, ys); px(x, ys + 1, K_LAT0); }
    #undef FIO
}

// Apertaram o botao de um andar trancado.
static void predio_nega(int d)
{
    negado_d = d;
    negado_t = t_relogio;
    pr_cab_v += 3;                                             // a cabine da um tranco; a mola devolve
    som_toca(SOM_BATE);
}

// O lado esquerdo da tela de Dificuldade: o predio e a botoeira.
static void predio_dificuldade(int sel, int aberto)
{
    float t = t_relogio;
    if (pr_t_visto < 0 || t_fase < pr_t_visto) {      // acabou de entrar: a cabine ja esta la, o andar abre e acende
        for (int k = 0; k <= N_NIVEIS; k++) { pr_abre[k] = 0; pr_luz[k] = 0; }
        pr_cab = 0;                                            // a cabine sobe do terreo
        pr_cab_v = 0;
        pr_chegou = false;
        pr_chegada_t = -9;
        pr_t_ant = t;
        pr_sel_ant = sel;
        pr_aperto_t = -9;
        negado_d = -1;
    }
    pr_t_visto = t_fase;
    float dt = t - pr_t_ant;
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    pr_t_ant = t;
    if (sel != pr_sel_ant) { pr_aperto_t = t; pr_sel_ant = sel; pr_chegou = false; }
    for (float r = dt; r > 0; r -= 1 / 120.0f) {               // a cabine, com mola (em passos curtos)
        float h = r < 1 / 120.0f ? r : 1 / 120.0f;
        float acel = ((float)sel - pr_cab) * 180 - pr_cab_v * 19;   // passa um tico e volta
        pr_cab_v += acel * h;
        pr_cab += pr_cab_v * h;
    }
    if (!pr_chegou && fabsf(pr_cab - (float)sel) < 0.15f) {   // chegou: a porta acende e soa
        pr_chegou = true;
        pr_chegada_t = t;
        som_toca(SOM_PASSA);
    }
    for (int k = 0; k <= N_NIVEIS; k++)
        pr_abre[k] += ((k == sel ? 1.0f : 0.0f) - pr_abre[k]) * (1 - expf(-dt * 12));
    predio_g(sel, aberto, t, dt);
    botoeira_v(PRED_BX, PRED_Y, PRED_BW, PRED_H, sel, aberto, t);
    int passo = (PRED_H - 24) / (N_NIVEIS + 1);
    fio_andar(PRED_Y + PRED_H - 14 - passo / 2 - sel * passo, pr_luz[sel]);
}

#include "elevador.inc"                     // o elevador do comeco da run: a placa de latao de perto

// As cores e as medidas de cena saem daqui (o resto do jogo tem as dele).
#undef G_PIL
#undef G_SH
#undef G_WI
#undef G_RM
#undef G_WR
#undef G_ABRE
#undef G_FECHA
#undef MARCA
#undef MARCA2
#undef MARCA3
#undef FB
#undef LUZS
#undef K_PRETO
#undef K_PLACA
#undef K_EB
#undef K_EB2
#undef K_PARDO
#undef K_CINZA
#undef K_CHUMBO
#undef K_FERRO
#undef K_ACO
#undef K_PRATA
#undef K_MARF3
#undef K_MARF2
#undef K_MARF
#undef K_BRANCO
#undef K_MAD0
#undef K_MAD1
#undef K_MAD2
#undef K_MAD3
#undef K_LAT0
#undef K_LAT1
#undef K_LAT2
#undef K_LAT3
#undef K_LAT4
#undef K_VIN0
#undef K_VIN1
#undef K_VIN2
#undef K_VERM
#undef K_VIN3
#undef K_FOGO
#undef K_FOGO2
#undef K_AZ0
#undef K_AZ1
#undef K_AZ2
#undef K_ROX0
#undef K_ROX1
#undef K_VERDE
#undef K_VERDE2
#undef K_AGUA
#undef K_AGUA2
#undef K_AZ3
#undef K_ROX2
#undef K_ROX3
#undef K_ROSA
#undef PRED_X
#undef PRED_Y
#undef PRED_W
#undef PRED_H
#undef PRED_BX
#undef PRED_BW
