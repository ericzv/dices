// ===========================================================================
// A cortina: saindo da loja para um salao novo (o primeiro da
// run tambem), a cortina de veludo fecha dos lados sobre a loja, fica
// fechada um instante e abre sobre o salao novo, ainda escuro; com ela meio
// aberta, as luzes acendem como antes. A sanefa desce no comeco e sobe no
// fim; a barra de baixo atrasa quando a cortina anda e balanca quando para.
// A plaquinha de latao desce com o nome do salao que vai comecar.
// Qualquer tecla (ou clique) pula.
// ===========================================================================
static float t_cort = -1;
static uint16_t cort_buf[GFX_W * GFX_H];          // a loja, como estava ao sair
static float cort_topo, cort_barra, cort_bv;       // a beira em cima e embaixo (0 aberta, 1 fechada)
static float cort_placa_y, cort_placa_v;           // a plaquinha: quanto desceu (0..1) e a velocidade
#define CORT_FECHA  0.45f
#define CORT_FICA   0.8f                          // fechada, com a plaquinha
#define CORT_ABRE   0.6f
#define CORT_TOTAL  (CORT_FECHA + CORT_FICA + CORT_ABRE)
#define CORT_SANEFA 30
#define CORT_DOBRAS 7                               // as pregas de cada metade
#define CORT_FIM_Y  350                             // onde acaba o pano (a franja vem embaixo)

static void cortina_comeca(void)
{
    memcpy(cort_buf, display_fb(), sizeof cort_buf);
    t_cort = 0;
    cort_topo = cort_barra = cort_bv = 0;
    cort_placa_y = cort_placa_v = 0;
    som_toca(SOM_TRANSICAO);
}
// Enquanto a cortina nao abriu o bastante, as luzes do salao esperam.
static bool cort_segura(void) { return t_cort >= 0 && t_cort < CORT_FECHA + CORT_FICA + CORT_ABRE * 0.3f; }

static float cort_alvo(float t)
{
    if (t < CORT_FECHA) {                           // puxada: acelera e bate no meio (a barra passa do ponto)
        float q = t / CORT_FECHA;
        return q * q * (1.6f - 0.6f * q);
    }
    t -= CORT_FECHA;
    if (t < CORT_FICA) return 1;
    t -= CORT_FICA;
    float q = fminf(1, t / CORT_ABRE);               // abrindo: arranca rapido e freia nos lados
    return (1 - q) * (1 - q) * (1 - q * 0.3f);
}

static void cort_passo(float dt)
{
    if (t_cort < 0) return;
    float antes = t_cort;
    t_cort += dt;
    if (antes < CORT_FECHA + CORT_FICA && t_cort >= CORT_FECHA + CORT_FICA) som_toca(SOM_TRANSICAO);
    // a barra segue a beira de cima numa mola frouxa: atrasa e passa do ponto
    int n = (int)(dt * 240) + 1;
    float h = dt / n;
    for (int i = 0; i < n; i++) {
        float tt = antes + (t_cort - antes) * (i + 1) / n;
        cort_topo = cort_alvo(tt);
        float a = (cort_topo - cort_barra) * 150 - cort_bv * 9;
        cort_bv += a * h;
        cort_barra += cort_bv * h;
        // a plaquinha: desce quando fecha, sobe antes de abrir
        float alvo = tt > CORT_FECHA * 0.9f && tt < CORT_FECHA + CORT_FICA - 0.05f ? 1 : 0;
        float ap = (alvo - cort_placa_y) * (alvo > 0 ? 260 : 400) - cort_placa_v * (alvo > 0 ? 13 : 30);
        cort_placa_v += ap * h;
        cort_placa_y += cort_placa_v * h;
    }
    if (t_cort >= CORT_TOTAL) t_cort = -1;
}

// O veludo: preto, vinho, vinho claro, vermelho e o brilho.
static const uint16_t CORT_TOM[5] = { RGB(14, 14, 12), RGB(66, 18, 24), RGB(118, 30, 38), RGB(190, 56, 60), RGB(208, 98, 108) };
static uint16_t cort_veludo(float L, int x, int y)
{
    int k = (int)floorf(L * 4 + bayer(x, y) - 0.5f);
    return CORT_TOM[k < 0 ? 0 : k > 4 ? 4 : k];
}

// As pregas ao longo da metade (u de 0, o lado de fora, a 1, a beira): o
// cosseno (a altura da prega) e o seno (a inclinacao), numa tabela; as
// larguras variam de uma prega para a outra.
#define CORT_LUT 1024
static float cort_c[CORT_LUT], cort_s[CORT_LUT];
static void cort_tabela(void)
{
    static bool feita = false;
    if (feita) return;
    feita = true;
    for (int k = 0; k < CORT_LUT; k++) {
        float u = (k + 0.5f) / CORT_LUT;
        float fase = u * CORT_DOBRAS * 6.2832f + 0.6f + 1.5f * sinf(u * CORT_DOBRAS * 0.83f + 1.3f) + 0.7f * sinf(u * 23.0f);
        cort_c[k] = cosf(fase);
        cort_s[k] = sinf(fase);
    }
}

// A beira da metade da esquerda na linha y (a da direita e o espelho).
static float cort_beira(float meia, int y)
{
    float s = (y - CORT_SANEFA) / (float)(CORT_FIM_Y - CORT_SANEFA);
    if (s < 0) s = 0;
    s = powf(s, 1.6f);
    float e = cort_topo + (cort_barra - cort_topo) * s;
    return e * meia;
}

// Uma metade (lado -1 a esquerda, 1 a direita): as pregas, a beira de galao,
// a sombra no que esta atras e a franja na barra.
static void cort_metade(int lado, float fechada, float sanefa_y)
{
    uint16_t *fb = display_fb();
    float meia = GFX_W / 2 + 7;
    uint16_t rl[4];
    rampa_de(C_LATAO, rl);
    (void)fechada;
    cort_tabela();
    int y0 = (int)sanefa_y + 6;
    if (y0 < 0) y0 = 0;
    for (int y = y0; y < GFX_H; y++) {
        int yy = y < CORT_FIM_Y ? y : CORT_FIM_Y - 1;
        float e = cort_beira(meia, yy);
        if (e < 1) continue;
        int ex = (int)e;
        float aperto = 1 - e / meia;                  // aberta: o pano junta e as pregas fundam
        float vert = 0.1f - 0.2f * y / GFX_H;          // a luz vem de cima: o alto mais claro
        if (y < CORT_SANEFA + 10) vert -= 0.22f * (1 - (y - CORT_SANEFA) / 10.0f);   // a sombra da sanefa
        int passo = (CORT_LUT << 10) / ex;           // u em 1/1024 da tabela, por pixel
        for (int i = 0; i < ex + 8 && i < GFX_W; i++) {
            int x = lado < 0 ? i : GFX_W - 1 - i;
            uint16_t *px = &fb[y * GFX_W + x];
            if (i >= ex) {                            // a sombra da cortina no que esta atras
                if (y < CORT_FIM_Y + 8) {
                    int a = (int)((1 - (i - ex) / 8.0f) * 70 + (bayer(x, y) - 0.5f) * 30);
                    if (a > 0) *px = gfx_mistura(*px, C_BARRA, a > 100 ? 100 : a);
                }
                continue;
            }
            int k = (i * passo) >> 10;
            if (k >= CORT_LUT) k = CORT_LUT - 1;
            int barra = CORT_FIM_Y - (cort_c[k] < -0.3f ? 1 : 0) - (cort_c[k] < -0.8f ? 1 : 0);   // a barra, ondulando com as pregas
            if (y >= barra) {                         // a franja, balancando com a barra
                int fy = y - barra;
                int sway = (int)lroundf(sinf(i * 0.21f + t_cort * 9) * fminf(2.5f, fabsf(cort_bv) * 0.6f));
                int fi = i + sway;
                if (fy < 2) *px = fy == 0 ? rl[1] : rl[3];
                else if (fy < 9 - (fi % 3 == 0) && (fi & 1)) *px = (fi % 4 == 1) ? rl[2] : rl[1];
                continue;
            }
            if (i >= ex - 3) {                        // o galao da beira, em trancinha
                int k = (y / 2 + (ex - i)) % 3;
                *px = i == ex - 1 ? C_BARRA : k == 0 ? rl[3] : k == 1 ? rl[2] : rl[1];
                continue;
            }
            float u = (float)k / CORT_LUT;
            float c = cort_c[k], sl = cort_s[k] * lado;
            float L = 0.5f + 0.15f * sl * (1 + 0.6f * aperto) + vert;
            if (c > 0.75f) L += 0.18f * (c - 0.75f) / 0.25f;              // a crista: o brilho do veludo
            if (c < -0.55f) L -= (0.2f + 0.2f * aperto) * (-0.55f - c) / 0.45f;   // o fundo da prega, vinho escuro
            if (L < 0.27f) L = 0.27f;                 // nunca o preto: o vale fica no vinho
            if (u > 0.93f) L -= (u - 0.93f) * 3;       // a ultima prega, virando
            if (lado < 0 && c > 0.3f && cort_s[k] > 0.75f) L += 0.25f;   // o filete de luz no flanco esquerdo
            *px = cort_veludo(L, x, y);
        }
    }
}

// A sanefa: a cornija dourada, os bandos em onda com a franja e as borlas.
static void cort_sanefa(int oy)
{
    uint16_t *fb = display_fb(), rl[4];
    rampa_de(C_LATAO, rl);
    const int bw = 128;
    for (int y = 0; y < CORT_SANEFA + 14; y++) {
        int sy = y + oy;
        if (sy < 0 || sy >= GFX_H) continue;
        for (int x = 0; x < GFX_W; x++) {
            int u = x % bw;
            float q = (float)u / bw;
            int fundo = 18 + (int)(sinf(q * 3.1416f) * 12);       // a barra do bando (a faixa de cima e inteira)
            uint16_t *px = &fb[sy * GFX_W + x];
            if (y < 5) { *px = y == 0 || y == 4 ? C_BARRA : y == 1 ? rl[3] : y == 2 ? rl[2] : rl[1]; continue; }
            if (y < fundo) {
                float v = (y - 5) / (float)(fundo - 5);
                float band = sinf(v * 3.1416f * 3);               // tres dobras no bando, acompanhando a curva
                float L = 0.48f + 0.2f * band - 0.12f * (1 - v) + 0.12f * (0.5f - fabsf(q - 0.5f)) - 0.1f * q;
                if (L < 0.3f) L = 0.3f;
                *px = cort_veludo(L, x, sy);
                continue;
            }
            if (y < fundo + 2) { *px = y == fundo ? rl[3] : rl[1]; continue; }
            if (y < fundo + 7 && (x & 1) && (x % 6 != 1 || y < fundo + 5)) *px = x % 4 == 1 ? rl[2] : rl[1];
        }
    }
    for (int k = 0; k <= GFX_W / bw; k++) {                   // as borlas, entre os bandos
        int bx = k * bw, by = 16 + oy;
        gfx_disco(bx, by, 4, C_BARRA); gfx_disco(bx, by, 3, rl[2]); gfx_pixel(bx - 1, by - 1, rl[3]);
        for (int j = 0; j < 13; j++) {
            int hw = j < 2 ? 1 : 1 + j / 3;
            gfx_rect(bx - hw - 1, by + 3 + j, 2 * hw + 3, 1, C_BARRA);
            for (int i = -hw; i <= hw; i++) gfx_pixel(bx + i, by + 3 + j, j == 2 ? rl[3] : (i + j) & 1 ? rl[1] : rl[2]);
        }
    }
}

// A plaquinha de latao com o nome do salao novo, presa em duas correntes.
static void cort_plaquinha(int oy)
{
    if (cort_placa_y < 0.02f) return;
    int a = run.andar <= DES_ANDARES ? run.andar : 0;
    char s[24];
    if (run.andar >= DES_ANDARES) snprintf(s, sizeof s, "o salão final");
    else snprintf(s, sizeof s, "%d de %d", run.andar + 1, com_coringa() ? DES_ANDARES + 1 : DES_ANDARES);
    int w = gfx_largura(SALAO[a], 1) + 48, h = 50;
    if (w < 170) w = 170;
    int cx = GFX_W / 2, y = (int)(-h - 10 + cort_placa_y * (CORT_SANEFA + 30 + h)) + oy;
    uint16_t rl[4];
    rampa_de(C_LATAO, rl);
    for (int s2 = -1; s2 <= 1; s2 += 2) {                     // as correntes
        int chx = cx + s2 * (w / 2 - 18);
        for (int cy = oy + 6; cy < y; cy += 3) {
            gfx_rect(chx - 1, cy, 3, 2, C_BARRA);
            gfx_pixel(chx, cy, ((cy / 3) & 1) ? rl[3] : rl[1]);
        }
    }
    int x = cx - w / 2;
    gfx_rect(x + 3, y + 4, w, h, C_BARRA);                    // a sombra no veludo
    gfx_rect(x - 1, y - 1, w + 2, h + 2, C_BARRA);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            bool aro = i < 3 || j < 3 || i >= w - 3 || j >= h - 3;
            uint16_t c;
            if (aro) c = (i < 3 || j < 3) ? (i == 0 || j == 0 ? rl[3] : rl[2]) : rl[1];
            else if (i < 5 || j < 5 || i >= w - 5 || j >= h - 5) c = C_BARRA;
            else c = (j < 8 && bayer(x + i, y + j) < 0.4f) ? C_EBANO_B : C_EBANO;
            gfx_pixel(x + i, y + j, c);
        }
    for (int k = 0; k < 4; k++) {                             // os parafusos
        int px = k & 1 ? x + w - 9 : x + 8, py = k & 2 ? y + h - 9 : y + 8;
        gfx_pixel(px, py, rl[3]); gfx_pixel(px + 1, py, rl[1]); gfx_pixel(px, py + 1, rl[1]);
    }
    txt_c(cx, y + 6, SALAO[a], COR_SALAO[a], true);
    txt_c(cx, y + 25, s, C_MARFIM_S, false);
}

static void desenha_cortina(void)
{
    if (t_cort < 0) return;
    uint16_t *fb = display_fb();
    if (t_cort < CORT_FECHA) memcpy(fb, cort_buf, sizeof cort_buf);   // fechando: ainda e a loja atras
    float fim = CORT_TOTAL - t_cort;
    float desce = fminf(1, t_cort / 0.22f), sobe = fminf(1, fim / 0.28f);
    float s = fminf(desce, sobe);
    s = s * s * (3 - 2 * s);
    int oy = (int)((1 - s) * -(CORT_SANEFA + 18));
    float fechada = fminf(cort_topo, cort_barra);
    if (fechada < 0) fechada = 0;
    cort_metade(1, fechada, (float)oy);                      // a da direita por baixo
    cort_metade(-1, fechada, (float)oy);
    cort_sanefa(oy);
    cort_plaquinha(oy);
}
