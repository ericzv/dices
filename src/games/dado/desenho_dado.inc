// ===========================================================================
// Desenho de dados: poligono por numero de lados
// ===========================================================================
// dens < 1: pontilhado, so os pixels com bayer abaixo de dens pegam a cor (a
// silhueta de um dado que ainda nao esta ali).
static float bayer(int x, int y);
static void poligono_dens(const float *vx, const float *vy, int n, uint16_t cor, float dens)
{
    float ymin = vy[0], ymax = vy[0];
    for (int i = 1; i < n; i++) {
        if (vy[i] < ymin) ymin = vy[i];
        if (vy[i] > ymax) ymax = vy[i];
    }
    for (int y = (int)ceilf(ymin); y <= (int)floorf(ymax); y++) {
        float xs[16];
        int nx = 0;
        for (int i = 0; i < n && nx < 16; i++) {
            int j = (i + 1) % n;
            float ya = vy[i], yb = vy[j];
            if ((ya <= y && yb > y) || (yb <= y && ya > y))
                xs[nx++] = vx[i] + (y - ya) / (yb - ya) * (vx[j] - vx[i]);
        }
        for (int a = 1; a < nx; a++)
            for (int b = a; b > 0 && xs[b] < xs[b - 1]; b--) {
                float t = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = t;
            }
        for (int k = 0; k + 1 < nx; k += 2) {
            int x0 = (int)ceilf(xs[k]), x1 = (int)floorf(xs[k + 1]);
            if (dens >= 1) { if (x1 >= x0) gfx_rect(x0, y, x1 - x0 + 1, 1, cor); }
            else for (int x = x0; x <= x1; x++) if (bayer(x, y) < dens) gfx_pixel(x, y, cor);
        }
    }
}
static void poligono(const float *vx, const float *vy, int n, uint16_t cor) { poligono_dens(vx, vy, n, cor, 1); }

// Embaralha dois inteiros: um acaso que so depende da mesa (o alvo do
// Laser, do Berserker...).
static uint32_t hash2(int x, int y)
{
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

// ---------------------------------------------------------------------------
// O pontilhado: todo degrade dos dados e da luz da mesa escolhe, pixel a
// pixel, entre dois tons vizinhos de uma rampa da paleta pelo limiar de uma
// matriz de Bayer 4x4 presa a tela. Nada de misturar cores e deixar a
// quantizacao decidir: isso desfaria o pontilhado.
// ---------------------------------------------------------------------------
static const uint8_t BAYER4[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static float bayer(int x, int y) { return (BAYER4[y & 3][x & 3] + 0.5f) / 16; }

// As familias da paleta (cores.inc): o aro em rampa sai da familia da cor
// dele, do escuro ao claro, dois tons abaixo e um acima; faltando tom, entra
// a familia mais perto (os escuros quentes embaixo, o marfim em cima).
static const uint8_t PX_FAMILIA[][2] = { { 0, 6 }, { 6, 10 }, { 10, 14 }, { 21, 23 }, { 23, 27 }, { 27, 32 },
                                         { 32, 38 }, { 38, 40 }, { 40, 44 }, { 44, 46 }, { 46, 50 }, { 50, 52 } };
static uint16_t cor_de_dado(uint16_t c);
static int brilho_de(uint16_t c);
static void rampa_de(uint16_t c, uint16_t *r)
{
    int n = (int)(sizeof PALETA / sizeof PALETA[0]), i = 0;
    while (i < n && PALETA[i] != c) i++;
    if (i == n) { c = cor_de_dado(c); for (i = 0; i < n && PALETA[i] != c; i++) {} }
    if (i == n) { r[0] = r[1] = r[2] = r[3] = c; return; }
    int a = i, b = i + 1;
    for (unsigned f = 0; f < sizeof PX_FAMILIA / sizeof PX_FAMILIA[0]; f++)
        if (i >= PX_FAMILIA[f][0] && i < PX_FAMILIA[f][1]) { a = PX_FAMILIA[f][0]; b = PX_FAMILIA[f][1]; }
    uint16_t L[12] = { 0 };
    int nl = 0;
    if (a > 0) {                                  // os escuros embaixo (o marfim desce ao pardo)
        L[nl++] = PALETA[a == 10 ? 4 : 2];
        L[nl++] = PALETA[a == 10 ? 5 : 3];
    }
    int f0 = nl;
    for (int k = a; k < b; k++) L[nl++] = PALETA[k];
    for (int p = f0 + 1; p < nl; p++)             // a familia do escuro ao claro
        for (int q = p; q > f0 && brilho_de(L[q]) < brilho_de(L[q - 1]); q--) { uint16_t t = L[q]; L[q] = L[q - 1]; L[q - 1] = t; }
    if (a != 10) L[nl++] = PALETA[a == 27 ? 10 : 11];   // e o marfim em cima
    int j = 0;
    while (j < nl && L[j] != PALETA[i]) j++;
    r[0] = L[j - 2 >= 0 ? j - 2 : 0];
    r[1] = L[j - 1 >= 0 ? j - 1 : 0];
    r[2] = L[j];
    r[3] = L[j + 1 < nl ? j + 1 : nl - 1];
}

// O fundo escuro tingido de uma cor (etiquetas, selos): o tom escuro da
// familia dela na paleta, para a tinta nao se perder no arredondamento. As
// familias sem um escuro com tinta (pardos, cinzas, marfim) ficam na placa.
static uint16_t tinta_escura(uint16_t c)
{
    int n = (int)(sizeof PALETA / sizeof PALETA[0]), i = 0;
    uint16_t q = gfx_na_paleta(c);
    while (i < n && PALETA[i] != c && PALETA[i] != q) i++;
    if (i < 14 || i >= n) return RGB(36, 32, 29);        // escuros, cinzas e marfim: a placa clara
    if (i < 23) return feltro_tons[2];                   // feltro e verdes
    if (i < 32 || (i >= 38 && i < 40) || i >= 50) return RGB(56, 32, 20);   // madeira, latao, fogo e pele
    if (i < 38) return RGB(66, 18, 24);                  // vinho e vermelho
    if (i < 46) return RGB(26, 34, 62);                  // azuis e agua
    return RGB(54, 36, 74);                              // roxos
}

// Um tom da rampa de 4 pela luz l (-1 de costas, 1 de frente para a luz de
// cima, a esquerda): faixas cheias, com o pontilhado so na passagem.
static uint16_t tom_rampa(const uint16_t *r, float l, int x, int y)
{
    float f = (l + 1) * 1.5f;                     // 0..3
    int k = (int)floorf(f);
    float fr_ = f - k;
    fr_ = (fr_ - 0.5f) * 2.4f + 0.5f;            // a passagem, mais estreita
    if (fr_ > 0 && (fr_ >= 1 || bayer(x, y) < fr_)) k++;
    return r[k < 0 ? 0 : k > 3 ? 3 : k];
}
static float luz_de(float dx, float dy)
{
    float d = sqrtf(dx * dx + dy * dy);
    return d < 0.5f ? 0 : -(dx + dy) / (d * 1.4142f);
}

// O aro do modo Pixel: o poligono (n > 0) ou o disco (n == 0, raio R) cheio,
// cada pixel no tom da rampa pela direcao em que a borda olha.
static void aro_rampa(const float *vx, const float *vy, int n, float cx, float cy, int R, uint16_t aro)
{
    uint16_t r[4];
    rampa_de(aro, r);
    int y0, y1;
    if (n) {
        float ymin = vy[0], ymax = vy[0];
        for (int i = 1; i < n; i++) { if (vy[i] < ymin) ymin = vy[i]; if (vy[i] > ymax) ymax = vy[i]; }
        y0 = (int)ceilf(ymin); y1 = (int)floorf(ymax);
    } else { y0 = (int)cy - R; y1 = (int)cy + R; }
    for (int y = y0; y <= y1; y++) {
        float xs[16];
        int nx = 0;
        if (n) {
            for (int i = 0; i < n && nx < 16; i++) {
                int j = (i + 1) % n;
                float ya = vy[i], yb = vy[j];
                if ((ya <= y && yb > y) || (yb <= y && ya > y)) xs[nx++] = vx[i] + (y - ya) / (yb - ya) * (vx[j] - vx[i]);
            }
            for (int a = 1; a < nx; a++)
                for (int b = a; b > 0 && xs[b] < xs[b - 1]; b--) { float t = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = t; }
        } else {                                  // o mesmo disco de gfx_disco
            int dy = y - (int)cy, w = 0;
            if (dy * dy > R * R + R) continue;
            while ((w + 1) * (w + 1) + dy * dy <= R * R + R) w++;
            xs[0] = (float)((int)cx - w); xs[1] = (float)((int)cx + w); nx = 2;
        }
        for (int k = 0; k + 1 < nx; k += 2)
            for (int x = (int)ceilf(xs[k]); x <= (int)floorf(xs[k + 1]); x++)
                gfx_pixel(x, y, tom_rampa(r, luz_de(x - cx, y - cy), x, y));
    }
}

// A forma diz quantos lados o dado tem antes de qualquer numero:
// moeda redonda, d4 triangulo, d6 quadrado de canto aparado, d8 losango,
// d12 pentagono, d20 hexagono, d100 a esfera.
static int forma(int lados, float cx, float cy, float r, float ang,
                 float *vx, float *vy)
{
    int n = 0;
    switch (lados) {
    case 4:
        for (int i = 0; i < 3; i++) {
            float a = ang - 1.5708f + i * 2.0944f;
            vx[n] = cx + cosf(a) * r * 1.2f; vy[n] = cy + sinf(a) * r * 1.2f + r * 0.2f; n++;
        }
        break;
    case 6:
        for (int i = 0; i < 4; i++) {
            float a = ang + 0.7854f + i * 1.5708f;
            for (int k = -1; k <= 1; k += 2) {
                float b = a + k * 0.24f;
                vx[n] = cx + cosf(b) * r * 1.08f; vy[n] = cy + sinf(b) * r * 1.08f; n++;
            }
        }
        break;
    case 8:
        for (int i = 0; i < 4; i++) {
            float a = ang + i * 1.5708f;
            vx[n] = cx + cosf(a) * r * 1.18f; vy[n] = cy + sinf(a) * r * 1.18f; n++;
        }
        break;
    case 12:
        for (int i = 0; i < 5; i++) {
            float a = ang - 1.5708f + i * 1.2566f;
            vx[n] = cx + cosf(a) * r * 1.08f; vy[n] = cy + sinf(a) * r * 1.08f; n++;
        }
        break;
    case 100:  // a esfera do Universo: um circulo
        for (int i = 0; i < 12; i++) {
            float a = ang + i * 0.5236f;
            vx[n] = cx + cosf(a) * r * 1.04f; vy[n] = cy + sinf(a) * r * 1.04f; n++;
        }
        break;
    default:   // 20
        for (int i = 0; i < 6; i++) {
            float a = ang + i * 1.0472f;
            vx[n] = cx + cosf(a) * r * 1.08f; vy[n] = cy + sinf(a) * r * 1.08f; n++;
        }
        break;
    }
    return n;
}

// A forma do dado de raio r com as bordas afastadas d pixels (d < 0: para
// dentro), paralelas as de fora: o aro, o bisel e o corpo saem todos da mesma
// forma e do mesmo centro, sem escalar (escalar engrossava o aro do d4 nas
// pontas e o tirava do centro).
static int forma_d(int lados, float cx, float cy, float r, float ang, float d, float *vx, float *vy)
{
    int n = forma(lados, cx, cy, r, ang, vx, vy);
    if (n < 3 || d == 0) return n;
    float gx = 0, gy = 0, nx[16], ny[16], c[16];
    for (int i = 0; i < n; i++) { gx += vx[i]; gy += vy[i]; }
    gx /= n; gy /= n;
    for (int i = 0; i < n; i++) {                 // cada aresta: a reta n.p = c, com a normal para fora
        int k = (i + 1) % n;
        float ex = vx[k] - vx[i], ey = vy[k] - vy[i], l = sqrtf(ex * ex + ey * ey);
        if (l < 1e-4f) l = 1e-4f;
        float a = ey / l, b = -ex / l;
        if ((vx[i] - gx) * a + (vy[i] - gy) * b < 0) { a = -a; b = -b; }
        nx[i] = a; ny[i] = b; c[i] = vx[i] * a + vy[i] * b + d;
    }
    for (int i = 0; i < n; i++) {                 // o vertice novo: o encontro das duas arestas afastadas
        int h = (i + n - 1) % n;
        float det = nx[h] * ny[i] - nx[i] * ny[h];
        if (fabsf(det) < 1e-5f) { vx[i] += nx[i] * d; vy[i] += ny[i] * d; continue; }
        vx[i] = (c[h] * ny[i] - c[i] * ny[h]) / det;
        vy[i] = (nx[h] * c[i] - nx[i] * c[h]) / det;
    }
    return n;
}

// O centro da forma: onde vao o numero e o desenho de dentro (no d4, o
// meio do triangulo, abaixo do centro do dado).
static float forma_cy(int lados, float cy, float r) { return lados == 4 ? cy + r * 0.2f : cy; }

// O bisel: o poligono (n > 0) ou o disco (raio R) em dois tons pela direcao
// em que a beira olha, o claro do lado da luz (de cima, a esquerda).
static void poligono_bisel(const float *vx, const float *vy, int n, float cx, float cy, int R,
                           uint16_t claro, uint16_t escuro)
{
    int y0, y1;
    if (n) {
        float ymin = vy[0], ymax = vy[0];
        for (int i = 1; i < n; i++) { if (vy[i] < ymin) ymin = vy[i]; if (vy[i] > ymax) ymax = vy[i]; }
        y0 = (int)ceilf(ymin); y1 = (int)floorf(ymax);
    } else { y0 = (int)cy - R; y1 = (int)cy + R; }
    for (int y = y0; y <= y1; y++) {
        float xs[16];
        int nx = 0;
        if (n) {
            for (int i = 0; i < n && nx < 16; i++) {
                int j = (i + 1) % n;
                float ya = vy[i], yb = vy[j];
                if ((ya <= y && yb > y) || (yb <= y && ya > y)) xs[nx++] = vx[i] + (y - ya) / (yb - ya) * (vx[j] - vx[i]);
            }
            for (int a = 1; a < nx; a++)
                for (int b = a; b > 0 && xs[b] < xs[b - 1]; b--) { float t = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = t; }
        } else {
            int dy = y - (int)cy, w = 0;
            if (dy * dy > R * R + R) continue;
            while ((w + 1) * (w + 1) + dy * dy <= R * R + R) w++;
            xs[0] = (float)((int)cx - w); xs[1] = (float)((int)cx + w); nx = 2;
        }
        for (int k = 0; k + 1 < nx; k += 2)
            for (int x = (int)ceilf(xs[k]); x <= (int)floorf(xs[k + 1]); x++)
                gfx_pixel(x, y, (x - cx) + (y - cy) <= 0 ? claro : escuro);
    }
}

// luz: o tom da beira do furo, no modo Pixel (0: o furo liso de sempre).
static void pips_luz(int cx, int cy, int v, int r, uint16_t c, uint16_t luz)
{
    int o = r * 5 / 11;
    static const int8_t P[7][6][2] = {
        {{0}}, {{0,0}}, {{-1,-1},{1,1}}, {{-1,-1},{0,0},{1,1}},
        {{-1,-1},{1,-1},{-1,1},{1,1}}, {{-1,-1},{1,-1},{0,0},{-1,1},{1,1}},
        {{-1,-1},{1,-1},{-1,0},{1,0},{-1,1},{1,1}},
    };
    if (v < 1 || v > 6) return;
    int pr = r / 7 > 1 ? r / 7 : 1;
    for (int i = 0; i < v; i++) {
        int x = cx + P[v][i][0] * o, y = cy + P[v][i][1] * o;
        if (r < 12) { gfx_rect(x - 1, y - 1, 3, 3, c); continue; }
        gfx_disco(x, y, pr, c);
        if (luz) {                                // o furo com relevo: a luz bate na beira de baixo, a direita
            gfx_pixel(x + pr - 1, y + pr - 1, luz);
            if (pr >= 3) { gfx_pixel(x + pr - 2, y + pr - 1, luz); gfx_pixel(x + pr - 1, y + pr - 2, luz); }
        }
    }
}

// ---------------------------------------------------------------------------
// O desenho dos dados. Cada dado se le assim:
//
//   FORMA    quantos lados tem (forma(), acima). Nada sai do contorno.
//   CORPO    de quem e: marfim para quem joga embaixo, ebano para quem joga
//            em cima, com um brilho na aresta de cima e um grao miudo (ou as
//            linhas, ou os pontos, em alguns). O especial pode vir tingido
//            na cor dele (o Vidro azulado, o Pirata de madeira).
//   ARO      a cor do dado (VISUAL); o dado comum tem o aro de latao.
//   DESENHO  so nos especiais: um padrao discreto por dentro do corpo, ligado
//            ao nome e ao poder (desenho_interno): os espinhos do Espinhoso,
//            o coracao do Caridoso, as listras do Gatuno. Gira com o dado.
//
// A Ficha de Fogo e o Tudo ou Nada sao fichas de verdade, desenhadas a
// parte; o Jackpot, o Acumulador e o Quebrado levam uma peca por cima
// (marca_por_cima). O valor vai sempre no meio, por cima de tudo.
// ---------------------------------------------------------------------------

// Quanto o Acumulador mostra de carga: quem desenha a peca avisa o bonus
// guardado (-1: nao sabe). E o mesmo para quem guarda alguma coisa: o bonus da
// Furia, a agua da Onda, os patinhos acesos dos Tres Patinhos.
static int marca_nivel = -1;
// Os Tres Patinhos pousados valendo na mesa: esta vez ja conta (um patinho a
// mais aceso; os tres acesos, x3).
static bool patos_pousou;
// Quantos patinhos acesos: os da contagem e, pousado valendo, mais um.
static int patos_acesos(void)
{
    if (marca_nivel < 0) return 2;                 // so o desenho (a colecao): dois de tres
    return marca_nivel % PATINHOS_CADA + (patos_pousou ? 1 : 0);
}
// O Revolver lancado (no ar ou ja na mesa): este lance ja carregou a bala dele.
static bool bala_lancada;
// Quantas balas no tambor: as de antes e, lancado, mais uma (com seis, ele
// dispara). Na colecao, meio tambor.
static int balas_desta(void)
{
    if (marca_nivel < 0) return REVOLVER_BALAS / 2;
    int n = marca_nivel + (bala_lancada ? 1 : 0);
    return n > REVOLVER_BALAS ? REVOLVER_BALAS : n;
}
// O dado em curso e o embrulho do Papai Noel: a caixa mostra "?", nao os
// lados do Presente. Quem desenha avisa; o desenho consome.
static bool caixa_fechada;
// As frutas do Panetone na fatia desta vez (-1: na mao, ainda nao cortada;
// mostra as tres).
static int frutas_desta = -1;
// O Relogio: as horas que o mostrador marca (o arco enche ate elas).
static int relogio_desta = 1;
// O Pisca-pisca aceso? Na mao, como vem no proximo lance; na fila, como veio.
static bool pisca_desta(void) { return marca_nivel < 0 || pisca_aceso(marca_nivel); }
// A Meia na Lareira: o que ela guardou (na colecao, um pouco).
static int meia_desta(void) { return marca_nivel < 0 ? 4 : marca_nivel; }
// A distancia do ponto (u, v) ao segmento de (ax, ay) a (bx, by).
static float dist_seg(float u, float v, float ax, float ay, float bx, float by)
{
    float dx = bx - ax, dy = by - ay, tt = ((u - ax) * dx + (v - ay) * dy) / (dx * dx + dy * dy);
    tt = tt < 0 ? 0 : tt > 1 ? 1 : tt;
    float ex = u - ax - tt * dx, ey = v - ay - tt * dy;
    return sqrtf(ex * ex + ey * ey);
}
// A agua da Onda, de 0 a 1 (o que ela guardou, de 9).
static float onda_cheia(void)
{
    if (marca_nivel < 0) return 0.45f;
    float q = marca_nivel / (float)ONDA_VIRA;
    return q < 0.08f ? 0.08f : q > 1 ? 1 : q;
}

static float sombra_alt;                 // altura do dado no ar, em pixels
// O Sorte/Reves no ar sao dois dados: 1 o d12 da Sorte (marfim, aro de ouro,
// "+"), 2 o d8 do Reves (ebano, aro vinho, "-"). 0: o dado inteiro, na fila.
static int sr_lado;

// Numero centrado no dado: grande nos dados grandes.
// Os algarismos sao os da Jersey (a fonte dos titulos), com traco de dois
// pixels e a sombra escura embaixo, mas o 6 e o 9 abertos - o 6 sem o gancho
// em cima, o 9 sem o de baixo - para nao se confundirem com o 8. A caixa do
// algarismo (7x10, o dobro nos dados grandes) fica no centro da forma.
static const char ALG_CARS[] = "0123456789?+-";
static const char *const ALG[13][10] = {
    { ".####..", "######.", "##..##.", "##..##.", "##..##.", "##..##.", "##..##.", "##..##.", "######.", ".####.." },
    { "###....", "###....", ".##....", ".##....", ".##....", ".##....", ".##....", ".##....", ".##....", ".##...." },
    { ".#####.", "#######", "##...##", "....###", "...###.", "..###..", ".###...", "###....", "#######", "#######" },
    { ".#####.", "#######", "##...##", ".....##", "..####.", "..####.", ".....##", "##...##", "#######", ".#####." },
    { "....##.", "...###.", "..####.", ".##.##.", "##..##.", "##..##.", "######.", "######.", "....##.", "....##." },
    { "######.", "######.", "##.....", "##.....", "#####..", ".#####.", "....##.", "##..##.", "######.", ".####.." },
    { "..#####", ".######", "##.....", "##.....", "######.", "#######", "##...##", "##...##", "#######", ".#####." },
    { "######.", "######.", "....##.", "....##.", "...###.", "..###..", "..###..", "..##...", "..##...", "..##..." },
    { ".#####.", "#######", "##...##", "##...##", ".#####.", "#######", "##...##", "##...##", "#######", ".#####." },
    { ".#####.", "#######", "##...##", "##...##", "#######", ".######", ".....##", ".....##", "######.", "#####.." },
    { ".####..", "######.", "##..##.", "...###.", "..###..", "..##...", "..##...", ".......", "..##...", "..##..." },
    { ".......", ".......", "..##...", "..##...", "######.", "######.", "..##...", "..##...", ".......", "......." },
    { ".......", ".......", ".......", ".......", "######.", "######.", ".......", ".......", ".......", "......." }
};
#define ALG_H 10
static int num_cresce;                     // o "pop" do numero no pouso: uma escala a mais
static void numero_em(float cx, float cy, float r, const char *s, uint16_t c, bool sombra)
{
    int e = (r >= 34 ? 2 : 1) + num_cresce, n = 0, g[8], a[8], w[8], W = 0;
    for (const char *p = s; *p; p++) {            // mede: cada algarismo so nas colunas que usa
        const char *q = strchr(ALG_CARS, *p);
        if (!q || n >= 8) {                       // fora dos algarismos: a letra da fonte
            gfx_texto((int)cx - gfx_largura(s, e) / 2, (int)cy - 12 * e, s, c, e, sombra);
            return;
        }
        int k = (int)(q - ALG_CARS), lo = 7, hi = -1;
        for (int y = 0; y < ALG_H; y++)
            for (int x = 0; x < 7; x++)
                if (ALG[k][y][x] == '#') { if (x < lo) lo = x; if (x > hi) hi = x; }
        if (hi < lo) lo = hi = 0;
        g[n] = k; a[n] = lo; w[n] = hi - lo + 1; W += w[n] + (n ? 1 : 0); n++;
    }
    // Centrado no pixel do meio da forma: um algarismo de largura impar cai
    // com tantos pixels de um lado quanto do outro.
    int x0 = (int)floorf(cx - W * e / 2.0f + 0.5f), y0 = (int)floorf(cy - ALG_H * e / 2.0f + 0.5f);
    for (int camada = sombra ? 0 : 1; camada < 2; camada++) {   // a sombra primeiro, um pixel abaixo
        int x = x0, dy = camada ? 0 : e;
        uint16_t cc = camada ? c : C_BARRA;
        for (int i = 0; i < n; i++) {
            for (int y = 0; y < ALG_H; y++)
                for (int k = 0; k < w[i]; k++)
                    if (ALG[g[i]][y][a[i] + k] == '#') gfx_rect(x + k * e, y0 + y * e + dy, e, e, cc);
            x += (w[i] + 1) * e;
        }
    }
}
static void numero(float cx, float cy, float r, const char *s, uint16_t c) { numero_em(cx, cy, r, s, c, true); }

// As duas protecoes, sempre com o mesmo desenho, em todo lugar: o escudo de
// aco (nao anulavel) e o cadeado de ouro (nao roubavel), lado a lado a partir
// de (x, y), cada um com 7x8 pixels e o contorno escuro. Devolve a largura.
static int icone_protecao(int x, int y, bool anula, bool rouba)
{
    static const char *const ESCUDO[8] = { ".XXXXX.", "XPAAAAX", "XPAAAAX", "XPAAAAX", "XAAAAAX", ".XAAAX.", "..XAX..", "...X..." };
    static const char *const CADEADO[8] = { "..XXX..", ".XP.PX.", ".XP.PX.", "XXXXXXX", "XOOOOOX", "XOOXOOX", "XOOXOOX", "XXXXXXX" };
    int w = 0;
    for (int q = 0; q < 2; q++) {
        if (!(q ? rouba : anula)) continue;
        const char *const *m = q ? CADEADO : ESCUDO;
        for (int yy = 0; yy < 8; yy++)
            for (int xx = 0; xx < 7; xx++) {
                char c = m[yy][xx];
                if (c == '.') continue;
                gfx_pixel(x + w + xx, y + yy, c == 'X' ? C_BARRA : c == 'P' ? C_PRATA : c == 'A' ? C_ACO : C_OURO);
            }
        w += 8;
    }
    return w ? w - 1 : 0;
}
// O cadeado, o mesmo em todo lugar (bolsa fechada, dado e estilo ainda
// fechados): 9 x 11 pixels em escala e, centrado em (cx, cy), com o contorno
// escuro, a alca de aco e o corpo na cor, com a luz de cima a esquerda.
static void icone_cadeado(int cx, int cy, int e, uint16_t cor)
{
    static const char *const M[11] = { "..KKKKK..", ".KSSSSsK.", ".KSK.KsK.", ".KSK.KsK.", "KKKKKKKKK", "KYYYYYYDK",
                                       "KYyyKyyDK", "KYyyKyyDK", "KYyyyyyDK", "KDDDDDDDK", "KKKKKKKKK" };
    uint16_t r[4];
    rampa_de(gfx_na_paleta(cor), r);
    for (int y = 0; y < 11; y++)
        for (int x = 0; x < 9; x++) {
            char c = M[y][x];
            if (c == '.') continue;
            uint16_t k = c == 'K' ? C_BARRA : c == 'S' ? C_PRATA : c == 's' ? C_ACO : c == 'Y' ? r[3] : c == 'y' ? r[2] : r[1];
            gfx_rect(cx + (x - 4) * e, cy + (y - 5) * e, e, e, k);
        }
}

// No canto de cima, a esquerda, de um dado de raio r em (cx, cy).
static void marca_protecao(float cx, float cy, float r, bool anula, bool rouba)
{
    if (r < 12 || (!anula && !rouba)) return;
    icone_protecao((int)(cx - r * 0.95f) - 2, (int)(cy - r * 0.95f) - 2, anula, rouba);
}

// O numero com um contorno de um pixel em volta (0: sem contorno).
static void numero_contorno(float cx, float cy, float r, const char *s, uint16_t c, uint16_t contorno)
{
    static const int8_t D[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {1,-1}, {-1,1}, {1,1} };
    if (contorno)
        for (int i = 0; i < 8; i++) numero_em(cx + D[i][0], cy + D[i][1], r, s, contorno, false);
    numero_em(cx, cy, r, s, c, !contorno);
}

// Fechado no catalogo: o dado fica apagado como o anulado, mas sem o X.
static bool so_apagado;

// X grosso em vinho sobre o dado anulado.
// O X do dado anulado: dois tracos de tinta vinho, cada um com o contorno
// escuro e tres tons - a beira de cima clara (a luz vem de cima), o meio e a
// de baixo escura. O segundo passa por cima do primeiro. Entrando (risca_q de
// 0 a 2), cada traco cresce da ponta de cima, um depois do outro; quando
// fecha, da um estalo (risca_pop: o traco engrossa e acende, 1 -> 0).
static float risca_q = 2, risca_pop;
static bool sem_risca;                     // anulado por ataque: a trinca no lugar do X
static void risca_anima(float t)           // t: desde que caiu (9: ja estava)
{
    risca_q = fminf(2, fmaxf(0, (t - 0.08f) / 0.12f));
    risca_pop = t >= 0.32f && t < 0.46f ? 1 - (t - 0.32f) / 0.14f : 0;
}
static void risca_traco(float ax, float ay, float bx, float by, float q, float hw, bool pop)
{
    if (q <= 0) return;
    float dx = bx - ax, dy = by - ay, len = sqrtf(dx * dx + dy * dy), ux = dx / len, uy = dy / len;
    float nx = -uy, ny = ux;                     // a normal que aponta para cima
    if (ny > 0) { nx = -nx; ny = -ny; }
    float ate = len * fminf(1, q);
    uint16_t claro = pop ? C_ROSA : C_VINHO_CLR, meio = pop ? C_VINHO_CLR : C_VERMELHO, escuro = RGB(118, 30, 38);
    int x0 = (int)floorf(fminf(ax, bx) - hw - 2), x1 = (int)ceilf(fmaxf(ax, bx) + hw + 2);
    int y0 = (int)floorf(fminf(ay, by) - hw - 2), y1 = (int)ceilf(fmaxf(ay, by) + hw + 2);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float px = x + 0.5f - ax, py = y + 0.5f - ay, t = px * ux + py * uy;
            float tc = t < 0 ? 0 : t > ate ? ate : t;            // o ponto mais perto no traco (pontas redondas)
            float ex = px - ux * tc, ey = py - uy * tc, d = sqrtf(ex * ex + ey * ey);
            if (d > hw + 1) continue;
            if (d > hw) { gfx_pixel(x, y, C_BARRA); continue; }     // o contorno
            float lado = ex * nx + ey * ny;                         // > 0: a beira de cima
            gfx_pixel(x, y, lado > hw * 0.4f ? claro : lado < -hw * 0.4f ? escuro : meio);
        }
}
static void risca(float cx, float cy, float r)
{
    if (so_apagado || sem_risca) return;
    float L = r * 0.8f, hw = (r < 12 ? 1.1f : r / 11) + risca_pop * 0.7f;
    bool pop = risca_pop > 0;
    risca_traco(cx - L, cy - L, cx + L, cy + L, risca_q, hw, pop);          // de cima a esquerda para baixo
    risca_traco(cx + L, cy - L, cx - L, cy + L, risca_q - 1, hw, pop);      // de cima a direita para baixo
}

// O corpo de um dado sempre numa cor da paleta, escolhida entre as que servem
// a um dado: nem pele (dos retratos) nem feltro (da mesa).
static uint16_t cor_de_dado(uint16_t c)
{
    int r = ((c >> 11) & 31) * 255 / 31, g = ((c >> 5) & 63) * 255 / 63, b = (c & 31) * 255 / 31;
    uint16_t melhor = c;
    long dmin = -1;
    for (unsigned i = 0; i < sizeof PALETA / sizeof PALETA[0]; i++) {
        uint16_t p = PALETA[i];
        if (i >= PAL_PELE || (i >= PAL_FELTRO && i < PAL_FELTRO + 7)) continue;
        int pr = ((p >> 11) & 31) * 255 / 31, pg = ((p >> 5) & 63) * 255 / 63, pb = (p & 31) * 255 / 31;
        int rm = (r + pr) / 2, dr = r - pr, dg = g - pg, db = b - pb;
        long d = (long)(512 + rm) * dr * dr / 256 + 4L * dg * dg + (long)(767 - rm) * db * db / 256;
        if (dmin < 0 || d < dmin) { dmin = d; melhor = p; }
    }
    return melhor;
}

static int brilho_de(uint16_t c)
{
    return (((c >> 11) << 3) * 3 + (((c >> 5) & 63) << 2) * 6 + ((c & 31) << 3)) / 10;
}

// Um tom do desenho: a cor c entrando p% no corpo; se nao aparecer, entra a
// mesma cor, clareada.
static uint16_t tom_desenho(uint16_t corpo, uint16_t c, int p)
{
    if (!c) return corpo;
    uint16_t t = cor_de_dado(gfx_mistura(corpo, c, p));
    if (t == corpo) t = cor_de_dado(gfx_mistura(corpo, gfx_mistura(c, C_MARFIM, 40), 70));
    return t;
}

static float fr(float x) { return x - floorf(x); }
static float tri(float x) { return 1 - fabsf(2 * fr(x) - 1); }         // 0..1..0 a cada 1

// Alguns dados tem ainda um padrao miudo no corpo, como sempre tiveram.
enum { T_LISO, T_LINHAS, T_PONTOS };

static int textura_de(int tipo)
{
    switch (tipo) {
    case D_JACKPOT:                    return T_LINHAS;
    case D_ACUMULADOR: case D_MARTELO: return T_PONTOS;
    default:                           return T_LISO;
    }
}

// 1 onde o padrao escurece, 0 onde deixa: no ponto (u, v) do dado, em
// pixels, com s = escala (1 num dado da mesa).
static int padrao(int tex, float u, float v, float s)
{
    switch (tex) {
    case T_LINHAS:  return ((int)floorf((u + v) / (3 * s)) & 3) == 0;
    case T_PONTOS: {
        float a = fmodf(u / s + 100, 5), b = fmodf(v / s + 100, 5);
        return a < 1.2f && b < 1.2f;
    }
    default:        return 0;
    }
}

// Ruido suave, de -1 a 1: manchas largas que se fundem umas nas outras.
static float ruido(float x, float y)
{
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    float a = (hash2(ix, iy) & 1023) / 511.5f - 1, b = (hash2(ix + 1, iy) & 1023) / 511.5f - 1;
    float c = (hash2(ix, iy + 1) & 1023) / 511.5f - 1, d = (hash2(ix + 1, iy + 1) & 1023) / 511.5f - 1;
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}

// Os cinco tons do corpo de um dado especial, da sombra ao reflexo. No
// marfim, os do material; num corpo tingido, o que a paleta tiver perto.
static void tons_do_corpo(uint16_t corpo, uint16_t *t)
{
    if (corpo == MATERIAL[MAT_MARFIM].tons[2]) { memcpy(t, MATERIAL[MAT_MARFIM].tons, 5 * sizeof *t); return; }
    // Num corpo tingido os vizinhos da paleta ficam longe: so a sombra do
    // canto e o reflexo mudam de tom, e so se a paleta tiver um da mesma cor,
    // um pouco mais escuro ou mais claro. Se nao tiver, fica o proprio corpo.
    int cr = (corpo >> 11) << 3, cg = ((corpo >> 5) & 63) << 2, cb = (corpo & 31) << 3;
    for (int k = 0; k < 5; k++) t[k] = corpo;
    for (int k = -1; k <= 1; k += 2) {
        uint16_t c = cor_de_dado(gfx_mistura(corpo, k < 0 ? C_SOMBRA : C_BRANCO, 28));
        int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
        int dr = r - cr, dg = g - cg, db = b - cb;
        bool perto = abs(dr) <= 48 && abs(dg) <= 48 && abs(db) <= 48 && abs(dr - dg) <= 24 && abs(dg - db) <= 24;
        bool rumo = k < 0 ? brilho_de(c) < brilho_de(corpo) : brilho_de(c) > brilho_de(corpo);
        if (perto && rumo) t[k < 0 ? 0 : 4] = c;
    }
}

// Distancia do ponto (u, v) ao segmento a-b.
static float ate_seg(float u, float v, float ax, float ay, float bx, float by)
{
    float ex = bx - ax, ey = by - ay;
    float q = ((u - ax) * ex + (v - ay) * ey) / (ex * ex + ey * ey);
    q = q < 0 ? 0 : q > 1 ? 1 : q;
    float dx = u - ax - q * ex, dy = v - ay - q * ey;
    return sqrtf(dx * dx + dy * dy);
}

// Bitola de um poligono regular de n lados com apotema a, centrado em
// (cu, cv) e girado de rot: 1 na borda, menos por dentro.
static float bitola(float u, float v, int n, float a, float cu, float cv, float rot)
{
    float g = -1;
    for (int i = 0; i < n; i++) {
        float t = rot + i * 6.2832f / n;
        float d = ((u - cu) * cosf(t) + (v - cv) * sinf(t)) / a;
        if (d > g) g = d;
    }
    return g;
}

// A bitola do corpo do d4 (1 na borda) e, em t, onde o ponto cai ao longo
// da borda mais proxima: de -1 a 1, de uma ponta a outra.
static float borda_d4(float u, float v, float *t)
{
    const float cu = 0.07f, cv = 0.24f, a = 0.5f;  // o centro do corpo (com o brilho) e o apotema
    float g = -1;
    for (int i = 0; i < 3; i++) {
        float an = 1.5708f + i * 2.0944f, nx = cosf(an), ny = sinf(an);
        float d = ((u - cu) * nx + (v - cv) * ny) / a;
        if (d > g) { g = d; *t = ((u - cu) * -ny + (v - cv) * nx) / (a * 1.732f); }
    }
    return g;
}

// O desenho de dentro de cada dado especial, no ponto (u, v) do dado em
// raios (o centro e 0; a borda do corpo fica perto de 0.84), ja girado com
// ele. s e o tamanho de um pixel em raios. Devolve 0 (o corpo) ou o tom
// do desenho: 1, 2 ou 3.
static int desenho_interno(int tipo, float u, float v, float s)
{
    float ro = sqrtf(u * u + v * v), l = s * 0.6f;          // l: meio traco de um pixel
    switch (tipo) {
    case D_DOBRO: {                               // >> : passa o dobro ao seguinte
        float d = u + 0.75f * fabsf(v);
        if (fabsf(v) > 0.46f) return 0;
        return fabsf(d + 0.10f) < 0.055f + l || fabsf(d - 0.30f) < 0.055f + l;
    }
    case D_VICIADO:                               // o chumbo, assentado embaixo
        return v > 0.56f;
    case D_LASTRO:                                // metal escovado
        return fr(v / (3 * s)) < 0.34f;
    case D_PAR:                                   // =
        return fabsf(v + 0.45f) < s || fabsf(v - 0.47f) < s;
    case D_ESCUDO: {                              // um brasao por tras dos pontos
        float w = 0.58f;
        if (v < -0.64f || fabsf(u) > w) return 0;
        return v < 0.1f || fabsf(u) < w * (1 - (v - 0.1f) / 0.68f);
    }
    case D_CARRASCO:                              // o capuz: a ponta de cima escura
        return v < -0.30f;
    case D_EXPLOSIVO: {                           // oito raios saindo do meio
        if (ro < 0.36f) return 0;
        float a = atan2f(v, u) / 6.2832f * 8;
        return fabsf(fr(a + 0.5f) - 0.5f) < 0.11f;
    }
    case D_MARTELO: {                             // Espinhoso: tres espinhos em cada borda, para dentro
        float t = 0, g = borda_d4(u, v, &t);
        float tk = roundf(t * 2) / 2;
        tk = tk < -0.5f ? -0.5f : tk > 0.5f ? 0.5f : tk;
        float d = fabsf(t - tk) / 0.15f;
        return d < 1 && g > 1 - 0.6f * (1 - d);
    }
    case D_PIRATA:                                // veio de madeira e a tira do tapa-olho
        if (fabsf(u + v + 0.80f) < 0.10f) return 2;
        return fr((u / s + sinf(v * 9) * 1.5f) / 4) < 0.25f;
    case D_ESPELHO:                               // as duas metades e o eixo
        if (fabsf(u) < l) return 2;
        return u > 0;
    case D_FICHAS: {                              // uma ficha de cassino grande, atras do numero
        float x = u - 0.06f, y = v - 0.24f, rr = sqrtf(x * x + y * y);
        if (rr > 0.47f) return 0;
        if (rr > 0.47f - l * 2) return 3;          // o contorno
        if (rr > 0.33f)                           // a beira, com seis listras claras
            return fabsf(fr(atan2f(y, x) / 6.2832f * 6) - 0.5f) < 0.2f ? 2 : 1;
        return rr > 0.33f - l * 2 ? 3 : 1;        // e o anel de dentro
    }
    case D_MALDITO: {                             // Agouro: manchas
        static const float M[7][4] = {
            {-0.52f,-0.36f,0.2f,1}, {0.46f,-0.46f,0.16f,2}, {0.56f,0.34f,0.18f,1},
            {-0.3f,0.58f,0.15f,2}, {-0.66f,0.2f,0.12f,2}, {0.14f,-0.72f,0.12f,1}, {0.22f,0.7f,0.11f,1} };
        for (int i = 0; i < 7; i++) {
            float dx = u - M[i][0], dy = v - M[i][1];
            if (dx * dx + dy * dy < M[i][2] * M[i][2]) return (int)M[i][3];
        }
        return 0;
    }
    case D_GATUNO:                                // listras de ladrao
        return fr(v / 0.25f) < 0.5f;
    case D_INVEJOSO: {                            // um olho, olhando de lado
        float e = 0.40f * (1 - u * u / 0.68f);
        if (fabsf(u) < 0.82f && fabsf(fabsf(v) - e) < l) return 1;
        float dx = u - 0.26f;
        return fabsf(sqrtf(dx * dx + v * v) - 0.16f) < l;
    }
    case D_CARIDOSO: {                            // um coracao por tras do numero
        float x = u / 0.34f, y = -(v - 0.22f) / 0.34f;
        float q = x * x + y * y - 1;
        return q * q * q - x * x * y * y * y <= 0;
    }
    case D_FARTURA: {                             // graos em fileiras
        float y = v / 0.22f, x = u / 0.2f + ((int)floorf(y) & 1) * 0.5f;
        return fabsf(fr(x) - 0.5f) < 0.16f && fabsf(fr(y) - 0.5f) < 0.24f;
    }
    case D_SORTE: {                               // o aro de uma ficha de cassino, azul e vermelho
        if (ro < 0.6f || ro > 0.76f) return 0;
        return ((int)floorf(fr(atan2f(v, u) / 6.2832f + 0.0625f) * 8) & 1) ? 2 : 1;
    }
    case D_TEIMOSO:                               // um X por tras dos pontos: nao cai
        if (fabsf(u) > 0.6f || fabsf(v) > 0.6f) return 0;
        return fabsf(u - v) < 0.2f || fabsf(u + v) < 0.2f;
    case D_SEMENTE: {                             // sementes em espiral, de girassol
        static float SX[34], SY[34];
        if (!SX[1])
            for (int k = 1; k < 34; k++) {
                float rr = 0.125f * sqrtf((float)k), a = k * 2.39996f;
                SX[k] = rr * cosf(a); SY[k] = rr * sinf(a);
            }
        float rs = 0.05f > s * 0.75f ? 0.05f : s * 0.75f;
        if (ro > 0.75f + rs) return 0;
        for (int k = 1; k < 34; k++) {
            float dx = u - SX[k], dy = v - SY[k];
            if (dx * dx + dy * dy < rs * rs) return 1;
        }
        return 0;
    }
    case D_MALDICAO: {                            // Maldito: a espiral de um feitico
        float x = u - 0.05f, y = v - 0.22f, rr = sqrtf(x * x + y * y);
        if (rr > 0.44f || rr < 0.03f) return 0;
        float k = rr / 0.16f - atan2f(y, x) / 6.2832f;
        return fabsf(k - roundf(k)) * 0.16f < l * 1.2f;
    }
    case D_TREVO: {                               // quatro folhas redondas
        static const float FOLHA[4][2] = { {-0.17f,-0.02f}, {0.17f,-0.02f}, {-0.17f,0.32f}, {0.17f,0.32f} };
        for (int i = 0; i < 4; i++) {
            float dx = u - FOLHA[i][0], dy = v - FOLHA[i][1];
            if (dx * dx + dy * dy < 0.17f * 0.17f) return 1;
        }
        return 0;
    }
    case D_COPIADOR:                              // uma copia do contorno, deslocada
        return fabsf(bitola(u, v, 4, 0.48f, -0.2f, -0.2f, 0) - 1) < l / 0.48f;
    case D_EGOISTA: {                             // uma coroa no alto
        float topo = -0.40f - 0.22f * tri(u / 0.4f + 0.5f);
        return fabsf(u) < 0.46f && v > topo && v < -0.32f;
    }
    case D_LASER:                                 // a mira
        if (fabsf(ro - 0.6f) < l) return 1;
        return ro > 0.3f && ro < 0.78f && (fabsf(u) < l || fabsf(v) < l);
    case D_VENTANIA:                              // rajadas de vento
        return sinf(v * 24 + sinf(u * 9) * 1.6f) > 0.86f;
    case D_MISERICORDIOSO: {                      // uma aureola em volta do numero
        float x = u - 0.03f, y = v - 0.12f;     // em volta do numero, sem encostar nas bordas do d4
        return fabsf(sqrtf(x * x + y * y) - 0.30f) < 0.04f + l;
    }
    case D_SOLITARIO: {                           // uma estrela so, num canto
        float x = u - 0.40f, y = v - 0.42f;
        return sqrtf(fabsf(x)) + sqrtf(fabsf(y)) < 0.46f;
    }
    case D_SENTINELA: {                           // pedras de muralha, em fiadas desencontradas
        float a = u / s + 100, b = v / s + 100;
        int fiada = (int)floorf(b / 6);
        float desloc = (fiada & 1) * 5.5f;
        if (fmodf(b, 6) < 1.1f || fmodf(a + desloc, 11) < 1.1f) return 1;
        return hash2((int)floorf((a + desloc) / 11), fiada) % 3 == 0 ? 2 : 0;
    }
    case D_FERREIRO:                              // ferro recartilhado
        return fr((u + v) / 0.22f) < s / 0.22f * 1.1f || fr((u - v) / 0.22f) < s / 0.22f * 1.1f;
    case D_COBRADOR:                              // as pautas do livro-caixa
        if (fabsf(u + 0.38f) < l) return 2;
        return fr((v + 0.05f) / 0.2f) < s / 0.2f * 1.1f;
    case D_DESAFIANTE: {                          // divisas para cima: o maior
        float d = v + 0.8f * fabsf(u);
        return fabsf(d + 0.66f) < s * 1.1f || fabsf(d + 0.44f) < s * 1.1f;
    }
    case D_VIDRO:                                 // dois reflexos
        return ate_seg(u, v, -0.62f, -0.12f, -0.12f, -0.62f) < l * 1.3f
            || ate_seg(u, v, -0.62f, 0.12f, -0.42f, -0.08f) < l * 1.3f;
    case D_SEGURANCA:                             // as barras de uma grade
        return fr((u + 0.11f) / 0.22f) < s / 0.22f * 1.2f;
    case D_BUMERANGUE: {                          // a curva do bumerangue
        float dx = u - 0.62f, dy = v - 0.62f;
        return fabsf(sqrtf(dx * dx + dy * dy) - 1.16f) < 0.08f;
    }
    case D_GEMEO:                                 // dois tracos iguais
        return fabsf(fabsf(u) - 0.23f) < l;
    case D_PRISMA: {                              // o espectro rente a borda de baixo, a esquerda
        if (u > 0.05f || v < -0.05f) return 0;
        float e = (v - u) * 0.7071f;
        return e > 0.58f ? 0 : e > 0.52f ? 3 : e > 0.46f ? 2 : e > 0.40f ? 1 : 0;
    }
    case D_FENIX: {                               // um sol nascendo da beira de baixo
        float y = v - 0.86f, rr = sqrtf(u * u + y * y);
        if (rr < 0.32f) return 1;
        if (rr < 0.40f || rr > 0.66f || y > 0) return 0;
        float a = atan2f(u, -y) / 0.5236f;
        return fabsf(fr(a + 0.5f) - 0.5f) < 0.16f ? 2 : 0;
    }
    case D_TRONO:                                 // uma moldura de ouro por dentro
        return fabsf(bitola(u, v, 5, 0.874f, 0, 0, -1.5708f + 0.6283f) - 0.72f) < s * 1.1f / 0.874f;
    case D_CAVALO:                                // as tabuas do cavalo
        return fr(u / 0.26f) < s / 0.26f * 1.1f;
    case D_BERSERKER:                             // tres garras
        if (fabsf(v) > 0.6f) return 0;
        for (int k = -1; k <= 1; k++) {
            float d = u - 0.45f * v - k * 0.26f - 0.04f, w = 0.05f * (1 - v * v / 0.36f) + l;
            if (fabsf(d) < w) return 1;
        }
        return 0;
    case D_CAPITAO:                               // tres galoes de capitao, embaixo
        if (fabsf(u) > 0.42f) return 0;
        for (int i = 0; i < 3; i++)
            if (fabsf(v - 0.38f - i * 0.13f) < s * 1.1f) return 1;
        return 0;
    case D_VIUVA: {                               // a teia
        float a = atan2f(v, u);
        if (fabsf(sinf(4 * a)) * ro < l) return 1;
        return fabsf(ro - 0.28f) < l || fabsf(ro - 0.52f) < l || fabsf(ro - 0.76f) < l;
    }
    case D_CHECKPOINT: {                          // a faixa quadriculada da chegada, embaixo
        if (v < 0.36f || v > 0.66f || fabsf(u) > 0.62f) return 0;
        int cx = (int)floorf((u + 0.62f) / 0.155f), cy = (int)floorf((v - 0.36f) / 0.15f);
        return (cx + cy) & 1 ? 1 : 2;
    }
    case D_AUTOFAGIA: {                           // a cobra que come o proprio rabo, em volta do numero
        // a cabeca, em cima a esquerda, morde a ponta do rabo; o corpo engrossa ate ela
        float hx = u + 0.17f, hy = v + 0.58f;
        if (hx * hx / (0.18f * 0.18f) + hy * hy / (0.13f * 0.13f) < 1 + l * 8) {
            float ex = hx + 0.05f, ey = hy + 0.04f;              // o olho
            return ex * ex + ey * ey < 0.035f * 0.035f + l * 0.4f ? 2 : 1;
        }
        float t = atan2f(u, -v) / 6.2832f;        // 0 em cima, no sentido do relogio
        if (t < 0) t += 1;
        if (t < 0.02f || t > 0.93f) return 0;
        float w = 0.015f + 0.085f * t, d = ro - 0.60f;
        if (fabsf(d) >= w + l) return 0;
        return fabsf(fr(t * 22) - 0.5f) < 0.17f && fabsf(d) < w * 0.5f ? 3 : 1;   // as escamas no dorso
    }
    case D_ECO: {                                 // o eco: aneis em volta do numero, cada vez mais fracos
        for (int i = 0; i < 3; i++)
            if (fabsf(ro - 0.38f - i * 0.17f) < 0.03f + l * 0.6f) return 1 + i;
        return 0;
    }
    case D_DOMINO:                                // a pedra de domino: a linha do meio e o pino
        if (u * u + v * v < 0.075f * 0.075f) return 2;
        return fabsf(v) < l * 1.4f && fabsf(u) < 0.74f;
    case D_ANCORA: {                              // a ancora, por tras do numero
        float y = v - 0.20f;                      // o meio do corpo do d4
        float w = 0.045f + l * 0.5f;
        if (fabsf(sqrtf(u * u + (y + 0.38f) * (y + 0.38f)) - 0.075f) < w * 0.8f) return 1;     // a argola
        if (fabsf(u) < w && y > -0.31f && y < 0.33f) return 1;                                   // a haste
        if (fabsf(y + 0.19f) < w * 0.85f && fabsf(u) < 0.19f) return 2;                          // o cepo
        float dd = sqrtf(u * u + (y + 0.02f) * (y + 0.02f));
        if (y > 0.08f && fabsf(dd - 0.33f) < w) return 1;                                        // os bracos
        float px = fabsf(u) - 0.31f, py = y - 0.12f;                                             // as pontas
        if (py < 0 && py > -0.12f && fabsf(px + py * 0.5f) < w * 0.9f) return 1;
        return 0;
    }
    case D_MARTIR: {                              // sangue escorrendo da ponta, e a cruz embaixo
        float y = v - 0.50f;
        if ((fabsf(u) < l * 1.6f && fabsf(y) < 0.15f) || (fabsf(y + 0.05f) < l * 1.6f && fabsf(u) < 0.10f)) return 2;
        if (v < -0.34f) return 1;                                  // a mancha da ponta
        static const float GX[4] = { -0.17f, -0.05f, 0.07f, 0.17f }, GL[4] = { 0.06f, 0.22f, 0.12f, 0.02f };
        for (int i = 0; i < 4; i++) {                              // os fios que escorrem
            float fim = -0.34f + GL[i], dx = u - GX[i];
            if (fabsf(dx) < 0.028f + l * 0.5f && v < fim) return 1;
            float gy = v - fim - 0.03f;                            // a gota na ponta do fio
            if (dx * dx + gy * gy < 0.045f * 0.045f) return gy < -0.01f && dx < 0 ? 3 : 1;
        }
        return 0;
    }
    case D_PUXA_FILA: {                           // duas divisas para a frente e a estrela da frente
        float x = u + 0.44f, y = v;               // a estrela, a esquerda do numero
        float r = sqrtf(x * x + y * y), a = atan2f(x, -y);
        float sct = fmodf(a + 31.416f, 1.2566f), d = fabsf(sct - 0.6283f) / 0.6283f;
        if (r < 0.06f + 0.08f * d) return 2;
        if (fabsf(v) > 0.24f) return 0;
        for (int i = 0; i < 2; i++)
            if (fabsf(u - 0.66f + i * 0.18f + 0.55f * fabsf(v)) < l * 1.6f) return 1;
        return 0;
    }
    case D_COVEIRO: {                             // a lapide de pedra, manchada, com a cruz gravada
        if (fabsf(v - 0.66f) < 0.03f + l && fabsf(u) < 0.62f) return 2;           // o chao
        float y = v - 0.12f;
        bool dentro = fabsf(u) < 0.34f && y < 0.54f && (y > -0.20f || u * u + (y + 0.20f) * (y + 0.20f) < 0.34f * 0.34f);
        if (!dentro) return 0;
        bool borda = fabsf(u) > 0.34f - 2 * s || (y < -0.20f && u * u + (y + 0.20f) * (y + 0.20f) > (0.34f - 2 * s) * (0.34f - 2 * s));
        if (borda) return 2;
        if ((fabsf(u) < 0.025f + l * 0.5f && y > -0.30f && y < 0.02f) || (fabsf(y + 0.18f) < 0.025f + l * 0.5f && fabsf(u) < 0.10f))
            return 2;                                                               // a cruz gravada
        uint32_t h = hash2((int)floorf(u / (2.2f * s) + 300), (int)floorf(v / (2.2f * s) + 300));
        return h % 5 == 0 ? 3 : h % 13 == 0 ? 2 : 1;                                // a pedra, manchada
    }
    case D_HEXA:                                  // um favo: o hexagono em volta e as celulas do canto
        if (fabsf(bitola(u, v, 6, 0.60f, 0, 0, 0) - 1) < s * 1.1f / 0.60f) return 1;
        return 0;
    case D_OURICO: {                              // os espinhos em volta, para dentro
        float x = u - 0.08f, y = v - 0.08f;       // o meio do corpo
        float e = fabsf(x) > fabsf(y) ? fabsf(x) : fabsf(y);
        float along = fabsf(x) > fabsf(y) ? y : x;
        return e > 0.66f - 0.18f * tri(along / 0.22f + 0.5f);
    }
    case D_CALIBRE: {                             // a mira em volta do numero
        float y = v - 0.20f, dd = sqrtf(u * u + y * y);
        if (fabsf(dd - 0.30f) < l * 1.2f) return 1;
        if (dd > 0.22f && dd < 0.40f && (fabsf(u) < l * 1.2f || fabsf(y) < l * 1.2f)) return 1;
        return 0;
    }
    case D_SABOTADOR: {                           // a adaga em formas: losango, cabo, guarda e a lamina em duas faces
        float t = (u - v) * 0.7071f + 0.877f;     // ao longo da diagonal: 0 no pe, 1.75 na ponta
        float d = (u + v) * 0.7071f;              // de lado
        if (fabsf(t - 0.12f) + fabsf(d) < 0.13f) return 2;                  // o pomo
        if (t > 0.22f && t < 0.48f && fabsf(d) < 0.075f) return 2;          // o cabo
        if (fabsf(t - 0.53f) / 0.07f + fabsf(d) / 0.32f < 1) return 2;      // a guarda
        if (t > 0.58f && t < 1.70f && fabsf(d) < 0.17f * (1.70f - t) / 1.12f)
            return d < 0 ? 3 : 1;                                           // a lamina: a face clara e a escura
        return 0;
    }
    case D_AZARAO: {                              // a seta que sobe, no canto de baixo
        float y = v - 0.30f, x = u - 0.24f;
        if (ate_seg(x, y, -0.16f, 0.14f, 0.12f, -0.14f) < l * 1.5f) return 1;
        if (ate_seg(x, y, 0.12f, -0.14f, 0.01f, -0.14f) < l * 1.5f || ate_seg(x, y, 0.12f, -0.14f, 0.12f, -0.03f) < l * 1.5f)
            return 1;
        return 0;
    }
    case D_AMPULHETA: {                           // dois triangulos pelo apice, as bases nas bordas
        float a = fabsf(v) - fabsf(u);
        if (fabsf(a) < l * 0.8f) return 3;
        if (a < 0) return 0;
        return v < 0 ? 1 : 2;                     // a areia de cima, mais clara, e a que ja caiu
    }
    case D_AGIOTA: {                              // a pilha de moedas, embaixo
        for (int i = 0; i < 3; i++) {
            float x = u / 0.24f, y = (v - 0.50f + i * 0.09f) / 0.07f;
            float q = x * x + y * y;
            if (q < 1) return q > 0.62f ? 1 : 2;
        }
        return 0;
    }
    case D_MEDUSA: {                              // as cobras descendo pelo corpo, sem cabeca, afinando no rabo
        static const float X0[3] = { -0.58f, 0.0f, 0.58f }, FASE[3] = { 0, 2.6f, 4.4f };
        for (int i = 0; i < 3; i++) {
            float x = u - X0[i] - 0.14f * sinf(v * 5.5f + FASE[i]);
            float w = 0.13f * (v < 0.10f ? 1 : v > 0.78f ? 0.25f : 1 - 0.75f * (v - 0.10f) / 0.68f);
            if (fabsf(x) > w + l * 0.8f) continue;
            if (fabsf(x) > w - l * 0.8f) return 2;                          // o contorno
            float q = fabsf(fr(v / 0.19f + i * 0.37f) - 0.5f) * 2;          // os losangos do dorso
            return fabsf(x) / (w * 0.5f) + q * 1.25f < 1 ? 3 : 1;
        }
        return 0;
    }
    case D_RANCOR: {                              // camadas que se assentam no fundo, cada vez mais escuras
        // Triangulos do mesmo formato, todos apoiados na base do corpo: o
        // ponto entra na camada s se, trazido da base por 1/s, cai no corpo.
        static const float S[4] = { 0.86f, 0.64f, 0.43f, 0.23f };
        const float hu = 0.07f, hv = 0.74f;       // o meio da base
        float t, g = borda_d4(u, v, &t);
        if (g > 1) return 0;
        int c = 0;
        for (int i = 0; i < 4; i++) {
            float q = borda_d4(hu + (u - hu) / S[i], hv + (v - hv) / S[i], &t);
            if (q > 1) break;
            if (q > 1 - (l * 1.6f) / (0.5f * S[i])) return 0;   // o fio do corpo entre as camadas
            c = i + 1;
        }
        return c == 0 ? 0 : c == 1 ? 0 : c == 2 ? 1 : c == 3 ? 2 : 3;
    }
    case D_ESPERANCA: {                           // a luz que sobe do ponto mais baixo, em leque
        const float hu = 0.08f, hv = 0.84f;      // a ponta de baixo do losango
        float dx = u - hu, dy = v - hv, dd = sqrtf(dx * dx + dy * dy);
        if (dd < 0.20f) return 2;                                            // a luz
        if (dd < 0.28f) return 0;
        float an = atan2f(-dy, dx) / 3.1416f * 11;                           // onze raios no meio circulo
        float w = fabsf(fr(an) - 0.5f) * 3.1416f / 11 * dd;                  // distancia ao raio, em raios
        if (w > 0.018f + dd * 0.03f + l * 0.4f) return 0;
        return (int)floorf(an) % 2 ? 1 : 3;                                  // raios em dois tons
    }
    case D_PONTE: {                               // um salto: o arco tracejado entre dois pontos
        const float cu = 0.07f, cv = 0.62f, ra = 0.48f;
        for (int m = -1; m <= 1; m += 2) {
            float x = u - cu - m * ra, y = v - cv;
            if (x * x + y * y < 0.075f * 0.075f) return 2;                   // os pontos das pontas
        }
        float dx = u - cu, dy = v - cv, dd = sqrtf(dx * dx + dy * dy);
        if (dy > 0) return 0;
        if (fabsf(dd - ra) < 0.035f + l * 0.5f)
            return fr(atan2f(-dy, dx) / 3.1416f * 7 + 0.25f) < 0.6f ? 1 : 0;   // o arco, em tracos
        return dd < ra - 0.07f ? 3 : 0;                                      // o vao por baixo
    }
    case D_GARIMPEIRO: {                          // camadas de terra e as pepitas presas nelas
        static const float P[5][2] = { { -0.46f, -0.42f }, { 0.50f, -0.30f }, { -0.50f, 0.30f },
                                       { 0.40f, 0.52f }, { -0.04f, 0.64f } };
        for (int i = 0; i < 5; i++) {
            float x = fabsf(u - P[i][0]) / 0.10f, y = fabsf(v - P[i][1]) / 0.075f;
            if (x + y < 1) return 3;
        }
        float f = (v + 0.05f * sinf(u * 3.6f + 1.0f)) / 0.27f + 9.35f;   // o meio fica na camada clara
        int c = (int)floorf(f) % 3;
        return c;                                                            // claro, terra e couro
    }
    case D_FANTASMA: {                            // nevoa ciano, irregular, e dois contornos tracejados
        float x = u - 0.08f, y = v - 0.08f;
        float e = fabsf(x) > fabsf(y) ? fabsf(x) : fabsf(y);
        float ao_longo = fabsf(x) > fabsf(y) ? y : x;
        for (int i = 0; i < 2; i++) {
            float m = i ? 0.36f : 0.58f;
            if (fabsf(e - m) < l * 1.3f && fr(ao_longo / 0.16f + i * 0.5f) < 0.55f) return 3;
        }
        float nv = sinf(u * 5.3f + 1.1f) * sinf(v * 4.1f - 0.7f) + 0.6f * sinf((u + v) * 7.9f + 2.0f)
                 + 0.35f * sinf(u * 13.0f - v * 11.0f);
        return nv > 0.55f ? 2 : nv > -0.15f ? 1 : 0;                         // manchas de tres tons
    }
    case D_POTE_QUENTE: {                         // o calor concentrado: losangos que esquentam para dentro
        float dd = (fabsf(u - 0.08f) + fabsf(v - 0.08f)) / 0.99f;
        if (dd < 0.15f) return 3;                                            // o miolo
        static const float R[3] = { 0.33f, 0.56f, 0.78f }, W[3] = { 0.07f, 0.045f, 0.025f };
        for (int i = 0; i < 3; i++)
            if (fabsf(dd - R[i]) < W[i] + l * 0.5f) return i < 1 ? 2 : 1;
        return 0;
    }
    case D_PREVISIVEL: {                          // a escada: barras que sobem, uma a mais a cada lance
        static const float X[4] = { -0.42f, -0.14f, 0.14f, 0.42f }, H[4] = { 0.14f, 0.25f, 0.36f, 0.47f };
        for (int i = 0; i < 4; i++)
            if (fabsf(u - X[i]) < 0.085f && v < 0.70f && v > 0.70f - H[i]) return i == 3 ? 2 : 1;
        return fabsf(v - 0.74f) < l * 1.2f && fabsf(u) < 0.56f ? 3 : 0;      // o chao da escada
    }
    case D_OCULTO: {                              // o veu: tracos em diagonal, e um losango vazio no meio
        float dd = fabsf(u) + fabsf(v);
        if (dd < 0.40f) return 0;
        if (dd < 0.40f + l * 2.2f) return 3;                                  // a borda do que nao se ve
        return fr((u - v) / 0.17f) < 0.32f ? 1 : 0;
    }
    case D_SACRIFICIO: {                          // a oferta: um triangulo invertido dentro do corpo, aceso
        float cu = 0.07f, cv = 0.40f;                                         // o meio dele, mais para baixo
        float g = -1;
        for (int i = 0; i < 3; i++) {                                        // o d4 de ponta-cabeca, apotema 0.30
            float an = -1.5708f + i * 2.0944f, nx = cosf(an), ny = sinf(an);
            float d = ((u - cu) * nx + (v - cv) * ny) / 0.30f;
            if (d > g) g = d;
        }
        if (g < 1 && g > 1 - (l * 1.8f + 0.03f) / 0.30f) return 3;           // o contorno de ouro
        if (g < 1) return 2;                                                 // o fogo dentro
        float t, gb = borda_d4(u, v, &t);
        return gb > 0.86f ? 1 : 0;                                           // a borda escura do corpo
    }
    case D_VAZIO: {                               // nada: um anel aberto em volta do zero
        float x = u - 0.08f, y = v - 0.08f, rr = sqrtf(x * x + y * y);
        if (fabsf(rr - 0.56f) > 0.035f + l) return 0;
        float an = atan2f(y, x);
        return an > -2.2f && an < -1.3f ? 0 : 1;                             // a abertura, em cima
    }
    case D_MEDIA: {                               // barras desiguais, e a linha da media cortando todas
        static const float X[4] = { -0.22f, -0.03f, 0.17f, 0.36f }, H[4] = { 0.16f, 0.36f, 0.22f, 0.42f };
        const float base = 0.70f, media = base - 0.29f;
        if (fabsf(v - media) < 0.022f + l && u > -0.34f && u < 0.48f) return 2;   // a media
        for (int i = 0; i < 4; i++)
            if (fabsf(u - X[i]) < 0.06f && v < base && v > base - H[i]) return v < media ? 3 : 1;
        return 0;
    }
    case D_VAMPIRO: {                             // gotas que escorrem das bordas de cima
        static const float X[3] = { -0.20f, 0.07f, 0.34f }, Y0[3] = { 0.02f, -0.30f, 0.02f }, C[3] = { 0.30f, 0.46f, 0.24f };
        for (int i = 0; i < 3; i++) {
            float x = u - X[i], y1 = Y0[i] + C[i];
            if (v > Y0[i] - 0.4f && v < y1 && fabsf(x) < 0.028f + l) return 1;   // o fio
            float y = v - (y1 + 0.03f);
            if (x * x + y * y < 0.065f * 0.065f) return 2;                       // a gota
        }
        return 0;
    }
    case D_COLECIONADOR: {                        // o album: uma grade de casas, quase todas preenchidas
        float x = (u + 0.70f) / 0.36f, y = (v + 0.62f) / 0.36f;              // quatro por quatro
        if (x < 0 || y < 0 || x >= 4 || y >= 4) return 0;
        float fx = fr(x), fy = fr(y);
        if (fx < 0.12f || fy < 0.12f) return 0;                              // o vao entre as casas
        int cxi = (int)x, cyi = (int)y, h = (cxi * 7 + cyi * 13 + cxi * cyi * 5) % 11;
        return h < 2 ? 0 : h < 6 ? 1 : h < 9 ? 3 : 2;                        // vazias, e tres tons
    }
    case D_PACTO: {                               // dois aneis presos um no outro
        float x0 = u + 0.20f, x1 = u - 0.20f, y = v - 0.04f;
        float r0 = sqrtf(x0 * x0 + y * y), r1 = sqrtf(x1 * x1 + y * y);
        bool a0 = fabsf(r0 - 0.40f) < 0.05f + l, a1 = fabsf(r1 - 0.40f) < 0.05f + l;
        // o entrelace: em cima o anel da esquerda passa por cima, embaixo o da direita
        if (a0 && a1) return y < 0 ? 1 : 2;
        if (a0) return 1;
        if (a1) return 2;
        return r0 < 0.40f && r1 < 0.40f ? 3 : 0;                              // o que os dois guardam
    }
    case D_ASSOMBRACAO: {                         // contornos tracejados que se afastam para o lado de la
        for (int i = 0; i < 3; i++) {
            float d = i * 0.10f;
            float x = u - 0.08f - d, y = v - 0.08f + d;
            float e = fabsf(x) > fabsf(y) ? fabsf(x) : fabsf(y);
            float ao_longo = fabsf(x) > fabsf(y) ? y : x;
            if (fabsf(e - 0.56f) < l * 1.3f && fr(ao_longo / 0.16f + i * 0.33f) < 0.55f - i * 0.12f) return i == 0 ? 1 : i == 1 ? 2 : 3;
        }
        return 0;
    }
    case D_FICHA_VELHA: {                         // uma ficha de cassino gasta: listras apagadas, lasca e rachadura
        float an = atan2f(v, u);
        // a lasca: uma cunha que se foi da beira, mostrando o miolo escuro
        float dl = an + 0.70f;
        if (ro > 0.62f && fabsf(dl) < (ro - 0.62f) * 1.3f) return 3;
        // a rachadura, saindo da lasca, em ziguezague, pela direita do numero
        static const float CR[5][2] = { { 0.50f, -0.42f }, { 0.36f, -0.24f }, { 0.47f, -0.08f }, { 0.37f, 0.10f }, { 0.44f, 0.24f } };
        for (int i = 0; i < 4; i++) {
            float ax = CR[i][0], ay = CR[i][1], bx = CR[i + 1][0] - ax, by = CR[i + 1][1] - ay;
            float tt = ((u - ax) * bx + (v - ay) * by) / (bx * bx + by * by);
            tt = tt < 0 ? 0 : tt > 1 ? 1 : tt;
            float ex = u - ax - tt * bx, ey = v - ay - tt * by;
            if (ex * ex + ey * ey < (l * 1.1f) * (l * 1.1f)) return 3;
        }
        // as seis listras da beira, ate a borda, desbotadas; duas ja quase sumiram
        if (ro > 0.62f) {
            float a = fr(an / 6.2832f * 6 + 0.25f);
            int qual = (int)floorf(an / 6.2832f * 6 + 0.25f + 6) % 6;
            if (fabsf(a - 0.5f) < 0.20f) return (qual == 1 || qual == 4) ? (ro > 0.78f ? 2 : 0) : 1;
            return 0;
        }
        // o filete de dentro, gasto aos pedacos
        if (fabsf(ro - 0.56f) < l && fr(an / 6.2832f * 9) < 0.62f) return 2;
        // manchas de patina
        static const float PT[3][3] = { { -0.46f, 0.32f, 0.06f }, { -0.26f, 0.50f, 0.045f }, { 0.08f, 0.50f, 0.05f } };
        for (int i = 0; i < 3; i++) {
            float x = u - PT[i][0], y = v - PT[i][1];
            if (x * x + y * y < PT[i][2] * PT[i][2]) return 2;
        }
        return 0;
    }
    case D_SANGUESSUGA: {                         // o corpo em gomos, curvado por baixo do numero, e a ventosa
        const float R = 0.50f;
        float an = atan2f(v, u);                  // 0 a direita, pi/2 embaixo, pi a esquerda
        float hx = u - R * cosf(-0.30f), hy = v - R * sinf(-0.30f), hd = sqrtf(hx * hx + hy * hy);
        if (hd < 0.17f) return hd > 0.115f ? 3 : hd > 0.055f ? 1 : 3;     // a ventosa: aro, boca, garganta
        if (an > -0.18f && an < 3.0f) {                                    // da cabeca (direita) ao rabo
            float q = (an + 0.18f) / 3.18f, w = 0.13f - 0.08f * q;
            if (fabsf(ro - R) < w) return fr(q * 7 + 0.5f) < 0.17f ? 2 : 1; // os gomos, com aneis ambar
        }
        float gx = u - 0.60f, gy = v - 0.06f;                              // uma gota na boca
        return gx * gx + gy * gy * 0.55f < 0.045f * 0.045f ? 3 : 0;
    }
    case D_TRENO: {                               // o treno descendo a encosta (a beira de baixo) e os presentes do outro lado
        float w = 0.04f + l * 0.6f;
        // eixos da encosta: ao longo da beira de baixo a direita (a) e para dentro (b)
        float a = (u - v) * 0.7071f, b = (u + v) * 0.7071f;
        if (fabsf(b - 0.60f) < w && a > -0.30f && a < 0.36f) return 2;                  // o patim
        float fx = a + 0.30f, fy = b - 0.47f, fd = sqrtf(fx * fx + fy * fy);
        if (fabsf(fd - 0.13f) < w && (fx < 0 || fy > 0)) return 2;                      // a frente enrolada
        if (fabsf(a - 0.22f) < w && b > 0.50f && b < 0.60f) return 2;                   // os suportes
        if (fabsf(a + 0.08f) < w && b > 0.50f && b < 0.60f) return 2;
        if (b > 0.36f && b < 0.50f && a > -0.22f && a < 0.36f) return 1;                // o assento
        if (b > 0.20f && b < 0.50f && a > 0.24f && a < 0.36f) return 1;                 // o encosto, atras
        // os presentes, empilhados na ponta da esquerda: caixas com a fita em cruz
        static const float PX[2] = { -0.62f, -0.58f }, PY[2] = { 0.10f, -0.15f }, PH[2] = { 0.15f, 0.11f };
        for (int i = 0; i < 2; i++) {
            float x = u - PX[i], y = v - PY[i];
            if (fabsf(x) > PH[i] || fabsf(y) > PH[i]) continue;
            if (fabsf(x) < w * 0.9f || fabsf(y) < w * 0.9f) return 2;      // a fita
            return i == 0 ? 1 : 3;
        }
        float lx = u - PX[1], ly = v - (PY[1] - PH[1] - 0.04f);           // o laco da caixa de cima
        if (fabsf(fabsf(lx) - 0.06f) < 0.04f && fabsf(ly) < 0.035f) return 2;
        return 0;
    }
    case D_JINGLE: {                              // a correia do treno atravessando a beira de baixo, com tres guizos
        for (int k = -1; k <= 1; k++) {
            float x = u - k * 0.26f, y = v - 0.67f, d = x * x + y * y;
            if (d < 0.085f * 0.085f) {                                                       // o guizo: a fenda, o contorno e o ouro
                if (fabsf(y - 0.02f) < s * 0.6f && fabsf(x) < 0.05f) return 2;
                return d > (0.085f - s) * (0.085f - s) ? 2 : 1;
            }
        }
        if (v > 0.62f && v < 0.72f) return fabsf(v - 0.67f) < s * 0.5f && fr(u / 0.08f) < 0.5f ? 0 : 3;   // a correia, pespontada
        return 0;
    }
    case D_FURIA: {                               // as chamas subindo da base e as sobrancelhas bravas
        // as sobrancelhas, em V, por cima do numero (so no dado grande: no
        // pequeno o numero as cobre)
        for (int k = -1; k <= 1 && s < 1 / 30.0f; k += 2) {
            float t = (u * k - 0.07f) / 0.26f;
            if (t > 0 && t < 1 && fabsf(v - (-0.24f - t * 0.13f)) < 0.04f + l * 0.7f) return 3;
        }
        // as chamas: linguas que sobem da base do d4, mais altas no meio
        float base = 0.80f, alto = 0.20f + 0.16f * fabsf(sinf(u * 7.0f + 0.6f)) + 0.12f * (1 - fabsf(u));
        float y = base - v;
        if (y < 0 || y > alto) return 0;
        return y < alto * 0.55f ? 2 : 1;
    }
    case D_ONDA: {                                // a agua sobe com o que ela guardou; a crista em cima
        float cheia = onda_cheia();
        float fase = u * 8.0f + 0.8f, sup = 0.78f - cheia * 1.55f + 0.06f * sinf(fase);
        if (v < sup - l * 1.5f) return 0;
        if (v < sup + 0.05f) return sinf(fase) < -0.55f ? 3 : 2;                         // a crista, com espuma no alto
        return fr((v - 0.04f * sinf(fase + 1.6f)) / 0.18f) < 0.16f ? 2 : 1;               // a agua, em listras
    }
    case D_TSUNAMI: {                             // a onda gigante: sobe da esquerda e se enrola por cima
        float cx = 0.20f, cy = -0.20f, dx = u - cx, dy = v - cy, dd = sqrtf(dx * dx + dy * dy), an = atan2f(dy, dx);
        // o rolo: um anel grosso que comeca embaixo a direita e se fecha sobre a esquerda
        if (dd > 0.26f && dd < 0.46f && (an < 0.9f || an > 2.6f)) {
            if (dd > 0.42f) return 3;                                                        // a espuma por fora
            return dd > 0.36f ? 2 : 1;
        }
        if (dd <= 0.26f && an > -2.8f && an < -0.2f && dy < 0) return 0;                     // o oco do tubo
        // o paredao: a agua que sobe da esquerda ate o rolo
        float topo = 0.62f - 0.70f * (u + 0.75f) / 1.0f;
        if (u < cx && v > topo && v > cy) return fr((v - topo) * 4.0f) < 0.18f ? 2 : 1;
        if (v > 0.52f) return fr(u * 3.0f + v * 2.0f) < 0.20f ? 2 : 1;                       // o mar embaixo
        // as garras de espuma, caindo da ponta do rolo
        float px = u + 0.24f, py = v + 0.10f;
        if (px > -0.10f && px < 0.10f && py > -0.04f && py < 0.16f && fr(px * 9.0f + 0.5f) < 0.45f) return 3;
        return 0;
    }
    case D_PATINHOS: {                            // tres patinhos em fila na beira da agua: os acesos contam
        int acesos = patos_acesos();
        if (fabsf(v - 0.75f) < l * 1.2f && fabsf(u) < 0.86f) return 3;                     // a linha d'agua
        for (int i = 0; i < 3; i++) {
            float x = u - (-0.50f + i * 0.50f), y = v - 0.62f;
            bool aceso = i < acesos;
            float bx = (x + 0.02f) / 0.18f, by = y / 0.10f;
            float hx = x - 0.09f, hy = y + 0.12f;
            if (hx > 0.05f && hx < 0.15f && fabsf(hy + 0.005f) < 0.03f + l * 0.5f) return aceso ? 2 : 3;   // o bico
            bool corpo = (bx * bx + by * by < 1 && y > -0.05f) || hx * hx + hy * hy < 0.075f * 0.075f
                         || (x < -0.13f && x > -0.22f && y < -0.01f && y > -0.09f);       // o rabinho
            if (corpo) return aceso ? 1 : 3;
        }
        return 0;
    }
    case D_REVOLVER: {                            // o tambor: seis camaras em volta do numero, as carregadas com a bala
        float rt = 0.74f;
        if (ro > rt + l) return 0;
        int balas = balas_desta();
        for (int i = 0; i < REVOLVER_BALAS; i++) {
            float a = -1.0472f + i * 1.0472f, cx = cosf(a) * 0.5f, cy = sinf(a) * 0.5f;   // nenhuma em cima nem embaixo do numero
            float d = sqrtf((u - cx) * (u - cx) + (v - cy) * (v - cy));
            if (d < 0.155f) {
                if (i >= balas) return d < 0.115f ? 3 : 1;                    // vazia: o buraco, com a beira
                return d < 0.03f + l ? 1 : 2;                                 // a bala: o latao, e a espoleta
            }
            float fa = a + 0.5236f, fx = cosf(fa) * rt, fy = sinf(fa) * rt;    // o sulco entre duas camaras, na beira
            if ((u - fx) * (u - fx) + (v - fy) * (v - fy) < 0.11f * 0.11f) return 0;
        }
        if (ro > rt - 0.05f - l) return 3;                                     // a beira do tambor
        return 1;
    }
    case D_PRESENTE: {                            // o papel de presente: listras finas na diagonal, e o laco na beira de cima
        for (int k = -1; k <= 1; k += 2) {                                                   // o laco: duas alcas, com o furo, e o no
            float bx = u - k * 0.13f, by = v + 0.60f;
            float e = bx * bx / (0.12f * 0.12f) + by * by / (0.075f * 0.075f);
            if (e < 1) return e < 0.25f ? 0 : 2;
        }
        if (u * u + (v + 0.60f) * (v + 0.60f) < 0.05f * 0.05f) return 2;
        return fr((u + v) / 0.22f) < 0.22f ? 1 : 0;
    }
    case D_PISCA: {                               // o fio de luzinhas atravessando a base: acesas ou apagadas
        bool aceso = pisca_desta();
        float fio = 0.30f + 0.14f * (1 - (u / 0.80f) * (u / 0.80f));                        // o fio, caido no meio
        if (fabsf(v - fio) < l * 1.3f + 0.015f && fabsf(u) < 0.84f) return 3;
        for (int i = 0; i < 4; i++) {
            float bx = -0.54f + i * 0.36f;
            float by = 0.30f + 0.14f * (1 - (bx / 0.80f) * (bx / 0.80f));
            float x = u - bx, y = v - by;
            if (fabsf(x) < 0.05f && y > 0 && y < 0.08f) return 3;                              // o soquete
            float e = x * x / (0.085f * 0.085f) + (y - 0.18f) * (y - 0.18f) / (0.12f * 0.12f);
            if (e < 1) {
                if (!aceso) return 3;                                                        // apagada: so o vidro escuro
                bool pisca = (int)(t_relogio * 2.4f + i * 1.5f) % 3 != 0;                      // acesas, piscando em roda
                return e < 0.25f && pisca ? 0 : (i % 2 ? 2 : 1);
            }
        }
        return 0;
    }
    case D_MEIA: {                                // o trico da meia, em carreiras de zigue-zague, e na beira de baixo as moedas que ela guardou
        int g = meia_desta();
        int moedas = g <= 0 ? 0 : g < 4 ? 1 : g < 8 ? 2 : 3;
        for (int i = 0; i < moedas; i++) {
            float x = u - (-0.21f + i * 0.28f), y = v - 0.54f, d = x * x + y * y;
            if (d < 0.085f * 0.085f) return d > 0.05f * 0.05f ? 2 : 3;
        }
        float t = v / 0.14f + tri(u / 0.16f) * 0.5f;
        return fr(t) < s * 1.1f / 0.14f;
    }
    case D_ESTRELA: {                             // o ceu da noite: estrelinhas espalhadas longe do numero, e a estrela de Belem na ponta de cima
        float ax = fabsf(u), ay = fabsf(v + 0.60f);
        if (ax < 0.06f * (1 - ay / 0.17f) && ay < 0.17f) return ay < 0.045f ? 2 : 1;      // as pontas, finas
        if (ay < 0.06f * (1 - ax / 0.15f) && ax < 0.15f) return ax < 0.045f ? 2 : 1;
        if (ro < 0.34f) return 0;
        const float cel = 0.16f;                                                             // as estrelinhas: uma ou nenhuma por celula
        int gx = (int)floorf(u / cel), gy = (int)floorf(v / cel);
        uint32_t h = hash2(gx + 7, gy + 23);
        if (h % 100 >= 40) return 0;
        float ox = (gx + 0.25f + (h >> 8 & 255) / 255.0f * 0.5f) * cel, oy = (gy + 0.25f + (h >> 16 & 255) / 255.0f * 0.5f) * cel;
        float dx = fabsf(u - ox), dy = fabsf(v - oy);
        if ((h >> 24) % 4 == 0 && ((dx < s * 0.6f && dy < 0.04f) || (dy < s * 0.6f && dx < 0.04f))) return 3;   // umas em cruz
        bool apaga = (int)(t_relogio * 1.5f + (h >> 4) % 7) % 5 == 0;                       // e umas piscando
        return dx < s * 0.7f && dy < s * 0.7f && !apaga ? 3 : 0;
    }
    case D_RENA: {                                // o pelo da rena, em tufinhos, a galhada pequena no alto e o nariz vermelho embaixo
        float au = fabsf(u), w = 0.024f + l * 0.6f;
        if (dist_seg(au, v, 0.04f, -0.55f, 0.15f, -0.74f) < w || dist_seg(au, v, 0.10f, -0.65f, 0.23f, -0.64f) < w) return 2;
        float ny = v - 0.68f;
        if (u * u + ny * ny < 0.075f * 0.075f) return 3;
        float fy = v / 0.13f, fx = u / 0.15f + ((int)floorf(fy) & 1) * 0.5f;                 // o pelo: tracinhos em fileiras desencontradas
        float px = (fr(fx) - 0.5f) * 0.15f, py = (fr(fy) - 0.5f) * 0.13f;
        return fabsf(py) < 0.045f && fabsf(px - py * 0.45f) < s * 0.6f;
    }
    case D_FICHA_CORINGA: {                       // a ficha de cassino: seis listras de marfim na beira, o anel de ouro, o miolo liso
        if (ro > 0.60f) {
            if (fabsf(fr(atan2f(v, u) / 6.2832f * 6 + 0.25f) - 0.5f) < 0.15f) return 1;
            return ro > 0.86f - l ? 3 : 0;                                    // a beira escura entre as listras
        }
        return fabsf(ro - 0.54f) < l * 1.3f + 0.015f ? 2 : 0;
    }
    case D_TERREMOTO: {                           // a pedra rachada: uma falha com lava embaixo a esquerda, outra seca no alto a direita
        static const float LAVA[6][2] = { {-0.94f,0.02f}, {-0.64f,0.14f}, {-0.58f,0.36f}, {-0.32f,0.44f}, {-0.24f,0.66f}, {0.02f,0.94f} };
        static const float SECA[5][2] = { {0.02f,-0.94f}, {0.20f,-0.70f}, {0.42f,-0.66f}, {0.50f,-0.44f}, {0.80f,-0.22f} };
        float d = 9, g = 9;
        for (int i = 0; i < 5; i++) { float q = dist_seg(u, v, LAVA[i][0], LAVA[i][1], LAVA[i + 1][0], LAVA[i + 1][1]); if (q < d) d = q; }
        for (int i = 0; i < 4; i++) { float q = dist_seg(u, v, SECA[i][0], SECA[i][1], SECA[i + 1][0], SECA[i + 1][1]); if (q < g) g = q; }
        float w = 0.05f + l;
        if (d < w * 0.5f) return 3;                                           // o miolo aceso da lava
        if (d < w) return 2;                                                  // a lava
        if (d < w + s * 1.2f || g < s * 0.8f + 0.012f) return 1;              // a beira da falha, e a falha seca
        float pu = u - 0.46f, pv = v - 0.52f;                                 // duas pedrinhas soltas embaixo a direita
        if (pu * pu + pv * pv < 0.06f * 0.06f || (u + 0.48f) * (u + 0.48f) + (v + 0.40f) * (v + 0.40f) < 0.045f * 0.045f) return 1;
        return 0;
    }
    case D_DIGITAL: {                             // o aparelho: o visor de cristal com a moldura, dois botoes e a luz de ligado
        float au = fabsf(u), av = fabsf(v + 0.06f);
        if (au < 0.60f && av < 0.40f) return au > 0.60f - s * 1.4f || av > 0.40f - s * 1.4f ? 2 : 1;
        float bu = au - 0.22f, bv = v - 0.58f;
        if (bu * bu + bv * bv < 0.075f * 0.075f) return 2;                    // os botoes
        float lu = u + 0.52f, lv = v + 0.62f;
        if (lu * lu + lv * lv < 0.045f * 0.045f) return 3;                    // a luz de ligado
        return 0;
    }
    case D_RELOGIO: {                             // o mostrador: os tracos das horas, o aro, o arco das rodadas e o ponteiro
        float an = atan2f(u, -v);                                             // 0 no meio-dia, no sentido do relogio
        if (an < 0) an += 6.2832f;
        int h = (relogio_desta - 1) % 12 + 1;
        float fim = h * 0.5236f;
        if (ro > 0.76f && ro < 0.86f && an <= fim + 0.02f) return 3;          // o arco que enche a cada rodada
        if (fabsf(ro - 0.72f) < l + 0.008f) return 2;                         // o aro do mostrador
        float hm = an / 0.5236f, dh = fabsf(hm - roundf(hm)) * 0.5236f * ro;  // a distancia ao traco da hora mais perto
        bool grande = ((int)roundf(hm) % 3) == 0;
        if (ro > (grande ? 0.52f : 0.58f) && ro < 0.68f && dh < (grande ? 0.04f : 0.022f) + l * 0.6f) return 1;
        float pa = (h % 12) * 0.5236f, px = sinf(pa), py = -cosf(pa);          // o ponteiro, para fora do numero
        if (dist_seg(u, v, px * 0.40f, py * 0.40f, px * 0.62f, py * 0.62f) < 0.035f + l * 0.6f) return 3;
        return 0;
    }
    case D_ADESIVADO: {                           // pedacos de adesivo tortos nos cantos, em volta do numero
        static const float A[5][5] = { {-0.50f,-0.50f,0.20f,0.13f,0.50f}, {0.52f,-0.46f,0.17f,0.12f,-0.60f},
                                       {-0.48f,0.52f,0.16f,0.13f,-0.30f}, {0.50f,0.52f,0.21f,0.12f,0.35f},
                                       {0.04f,-0.68f,0.12f,0.07f,0.10f} };
        for (int i = 0; i < 5; i++) {
            float c = cosf(A[i][4]), sn = sinf(A[i][4]);
            float x = (u - A[i][0]) * c + (v - A[i][1]) * sn, y = -(u - A[i][0]) * sn + (v - A[i][1]) * c;
            if (fabsf(x) >= A[i][2] || fabsf(y) >= A[i][3]) continue;
            if (x + y > A[i][2] + A[i][3] - 0.09f) return 0;                  // o canto que descolou
            return 1 + i % 3;
        }
        return 0;
    }
    case D_MAIS_ADESIVADO: {                      // o losango quase todo coberto: oito pedacos tortos, uns por cima dos outros
        static const float A[8][5] = { {0.00f,-0.66f,0.17f,0.11f,0.30f}, {0.40f,-0.38f,0.18f,0.12f,-0.70f},
                                       {0.66f,0.02f,0.16f,0.12f,0.20f},  {0.40f,0.42f,0.19f,0.11f,0.75f},
                                       {0.02f,0.68f,0.17f,0.12f,-0.25f}, {-0.40f,0.40f,0.18f,0.12f,-0.80f},
                                       {-0.66f,-0.02f,0.16f,0.12f,-0.15f}, {-0.38f,-0.40f,0.19f,0.11f,0.65f} };
        for (int i = 0; i < 8; i++) {
            float c = cosf(A[i][4]), sn = sinf(A[i][4]);
            float x = (u - A[i][0]) * c + (v - A[i][1]) * sn, y = -(u - A[i][0]) * sn + (v - A[i][1]) * c;
            if (fabsf(x) >= A[i][2] || fabsf(y) >= A[i][3]) continue;
            if (x - y > A[i][2] + A[i][3] - 0.08f) return 0;                  // o canto que descolou
            return 1 + i % 3;
        }
        return 0;
    }
    case D_SORTE_REVES:                           // metade sorte, metade reves, com o meio livre
        if (fabsf(ro - 0.58f) < l * 1.2f) return 2;   // o anel de ouro em volta do numero
        if (ro < 0.58f) return 0;
        return u + v > 0 ? 1 : 0;
    default:
        return 0;
    }
}

// O brilho dos lendarios: de tempos em tempos (BRILHO_CADA s) uma faixa de
// luz fina, com um fio mais fino atras, atravessa o dado na diagonal, de cima
// a esquerda para baixo a direita (o lado de onde vem a luz), em BRILHO_DUR s.
// Devolve onde a faixa esta agora, na diagonal (dx + dy) / r do dado, ou um
// valor fora do dado quando ela nao passa.
#define BRILHO_CADA 3.4f
#define BRILHO_DUR  0.5f
static bool brilho_apagado;              // o dado em curso esta anulado: sem brilho
// O brilho de passagem, mais leve, em qualquer dado: uma vez so, quando ele
// se move (pousou valendo, o cursor o escolheu). Quem desenha poe aqui o
// quanto a faixa ja andou (0..1) logo antes de desenhar o dado; o desenho
// consome (volta a -1), entao nao passa para o dado seguinte.
static float brilho_passa = -1;
static float brilho_desta = -1;          // o de passagem do dado em curso
// O material do corpo comum deste dado (veio da madeira, furta-cor da
// perola); -1: os outros, sem textura propria.
static int mat_desta = -1;
static float brilho_lenda(int tipo, bool *forte)
{
    *forte = false;
    if (brilho_apagado) return -9;
    if (brilho_desta >= 0 && brilho_desta <= 1) {
        *forte = TIPO[tipo].raridade == RAR_LENDA;
        return -1.7f + 3.4f * brilho_desta;
    }
    if (TIPO[tipo].raridade != RAR_LENDA) return -9;
    *forte = true;
    float f = fmodf(t_relogio + tipo * 0.77f, BRILHO_CADA);     // cada lendario no seu tempo
    return f < BRILHO_DUR ? -1.7f + 3.4f * f / BRILHO_DUR : -9;
}

// Dois dados de Natal sao o proprio material, nao um desenho no meio: o
// Panetone e o miolo dele, o Boneco de Neve e a neve, com o cachecol e a
// cartola nas pontas. corpo_desta diz qual (0: nenhum); apagado, so o que e
// desenho (passas, frutas, cartola, cachecol) fica, num cinza so.
static int corpo_desta;
static bool corpo_apagado;

// O miolo do Panetone no ponto (u, v) do dado, em raios, girando com ele: a
// massa creme com os alveolos compridos (a massa sobe e estica), mais raros
// no meio, umas passas na beira e, nos cantos, as frutas cristalizadas da
// fatia (frutas_desta; na mao, as tres). nivel: a luz do corpo ali (-2 a 2).
// Devolve a cor, ou c (o corpo) onde nao muda.
static const uint16_t MIOLO[] = {
    RGB(246, 222, 132), RGB(228, 194, 106),                               // o alveolo e o fundo dele (a massa: MIOLO_MASSA)
    RGB(156, 106, 64),                                                    // a passa
    RGB(238, 162, 74), RGB(214, 112, 44),                                 // a laranja cristalizada
    RGB(208, 98, 108), RGB(190, 56, 60),                                  // a cereja
    RGB(130, 204, 112), RGB(88, 152, 84),                                 // a cidra
};
static uint16_t miolo_panetone(float u, float v, float s, int nivel, uint16_t c, uint16_t cinza)
{
    // as frutas da fatia, nos cantos: cubinhos tortos, a beira de cima mais
    // clara (o giro de cada um ja em cosseno e seno; so perto se faz a conta)
    static const float FX[3] = { -0.48f, 0.47f, 0.02f }, FY[3] = { 0.02f, -0.04f, 0.52f };
    static const float FC[3] = { 0.921f, 0.955f, 0.622f }, FS[3] = { 0.389f, -0.296f, 0.783f };
    int n = frutas_desta < 0 ? 3 : frutas_desta;
    for (int i = 0; i < n && i < 3; i++) {
        float x = u - FX[i], y = v - FY[i];
        if (x * x + y * y > 0.13f * 0.13f) continue;
        float a = x * FC[i] - y * FS[i], b = x * FS[i] + y * FC[i], lado = 0.072f + 0.01f * ruido(a * 30 + i, b * 30);
        if (fabsf(a) < lado && fabsf(b) < lado * 0.85f) {
            if (corpo_apagado) return cinza;
            bool luz = a < -lado + 0.03f || b < -lado * 0.85f + 0.03f;
            return MIOLO[3 + i * 2 + (luz ? 0 : 1)];
        }
    }
    // umas passas, so perto da beira
    static const float PX[5] = { -0.10f, 0.30f, -0.36f, 0.22f, -0.20f }, PY[5] = { -0.58f, -0.40f, -0.36f, 0.40f, 0.44f };
    for (int i = 0; i < 5; i++) {
        float x = u - PX[i], y = v - PY[i];
        if (x * x + y * y < 0.045f * 0.045f * (1 + 0.5f * ruido(x * 40 + i * 7, y * 40))) return corpo_apagado ? cinza : MIOLO[2];
    }
    if (corpo_apagado) return c;
    // a massa creme: alveolos tortos e compridos, dourados (a massa sobe e
    // estica), mais raros perto do meio, onde fica o numero
    float ro = sqrtf(u * u + v * v), meio = ro < 0.5f ? (0.5f - ro) * 0.9f : 0;
    float w1 = ruido(u * 3 + 11, v * 3 + 3), w2 = ruido(u * 3 + 5, v * 3 + 17);
    float m = ruido((u + 0.12f * w1) * 10 + 3, (v + 0.12f * w2) * 4.2f + 7) + 0.5f * ruido(u * 21 + 9, v * 9 + 1);
    if (m > 0.95f + meio) return MIOLO[1];
    if (m > 0.52f + meio || nivel <= -2) return MIOLO[0];                 // o alveolo e a sombra do canto
    return MIOLO_MASSA;
}

// A neve do Boneco no ponto (u, v) do dado, em raios, girando com ele: lisa,
// com um grao miudo e uns brilhos que piscam longe do numero; o cachecol de
// trico na ponta de baixo e a cartola na de cima.
static const uint16_t NEVE[] = {
    RGB(184, 194, 208), RGB(168, 204, 230),                               // o grao e o brilho
    RGB(36, 32, 29),                                                      // a cartola
    RGB(190, 56, 60), RGB(118, 30, 38), RGB(252, 248, 236),              // o cachecol (e a fita): o vermelho, o ponto e a listra
};
static uint16_t neve_boneco(float u, float v, float s, int nivel, uint16_t c, uint16_t cinza)
{
    // o cachecol de trico atravessando a ponta de baixo, com a listra branca
    if (v > 0.50f && v < 0.66f) {
        if (corpo_apagado) return cinza;
        float y = v - 0.50f;
        if (fabsf(y - 0.08f) < 0.012f + s * 0.4f) return NEVE[5];
        if (y < s * 1.2f || y > 0.16f - s * 1.2f) return NEVE[4];
        if (s < 0.05f) {                                                     // grande: os pontos do trico, em V
            float sx = fr(u / 0.06f), sy = fr(y / 0.04f);
            if (fabsf(fabsf(sx - 0.5f) - (1 - sy) * 0.5f) < 0.07f) return NEVE[4];
        }
        return NEVE[3];
    }
    // a cartola na ponta de cima: a copa, a fita vermelha e a aba
    if ((fabsf(u) < 0.10f && v > -0.72f && v < -0.55f) || (fabsf(u) < 0.19f && v >= -0.55f && v < -0.50f)) {
        if (corpo_apagado) return cinza;
        return v > -0.60f && v < -0.55f ? NEVE[3] : NEVE[2];
    }
    if (corpo_apagado) return c;
    // a neve: o grao miudo, mais raro perto do meio, e os brilhos que acendem e apagam
    float ro = sqrtf(u * u + v * v);
    uint32_t h = hash2((int)floorf(u / s) + 700, (int)floorf(v / s) + 700);
    if (ro > 0.32f && h % 61 == 0 && (h >> 8) % 4 == ((int)(t_relogio * 2) & 3)) return NEVE[1];
    if (ro > 0.32f && h % 23 == 1) return NEVE[0];
    if (nivel <= -2) return NEVE[0];                                      // a sombra do canto
    return c;
}

// O ciclo cosmico e o proprio ceu, pintado ponto a ponto no corpo de noite
// (girando com o dado): a Estrela, um ponto de luz grande no meio, com o halo
// e os raios; a Constelacao, estrelas ligadas por fios; a Galaxia, dois
// bracos em espiral girando devagar; o Buraco Negro, o escuro com o anel de
// luz e o disco de fogo em volta; o Universo, uma esfera de nebulosas e
// estrelas, com a luz de cima (nivel) e a beira mais escura. (u, v) em raios;
// devolve a cor, ou c (o corpo) onde nao muda. Apagado, so a luz principal
// fica, num cinza so.
static const uint16_t COSMO[] = {
    RGB(26, 34, 62), RGB(56, 86, 150), RGB(96, 134, 196), RGB(168, 204, 230),       // 0-3 o azul da noite ao gelo
    RGB(252, 248, 236), RGB(246, 222, 132), RGB(228, 194, 106),                    // 4-6 o branco e os ouros
    RGB(54, 36, 74), RGB(102, 74, 138), RGB(150, 112, 160), RGB(206, 180, 218),    // 7-10 os roxos
    RGB(14, 14, 12), RGB(214, 112, 44), RGB(238, 162, 74), RGB(150, 62, 80),       // 11-14 o breu, o fogo e o vinho
};
enum { CO_NOITE, CO_AZUL, CO_CEU, CO_GELO, CO_BRANCO, CO_TRIGO, CO_OURO,
       CO_ROXO_E, CO_ROXO, CO_AMEIXA, CO_LILAS, CO_BREU, CO_FOGO, CO_AMBAR, CO_VINHO };
// Estrelinhas soltas: uma ou nenhuma por celula, longe do meio (o numero).
static int cosmo_estrelinha(float u, float v, float s, float cel, int semente, int chance, float longe)
{
    if (u * u + v * v < longe * longe) return 0;
    int gx = (int)floorf(u / cel), gy = (int)floorf(v / cel);
    uint32_t h = hash2(gx + semente, gy + 31);
    if ((int)(h % 100) >= chance) return 0;
    float ox = (gx + 0.25f + (h >> 8 & 255) / 255.0f * 0.5f) * cel, oy = (gy + 0.25f + (h >> 16 & 255) / 255.0f * 0.5f) * cel;
    float dx = fabsf(u - ox), dy = fabsf(v - oy);
    if ((int)(t_relogio * 1.4f + (h >> 4) % 9) % 6 == 0) return 0;                  // umas piscando
    if ((h >> 24) % 3 == 0 && ((dx < s * 0.6f && dy < s * 1.6f) || (dy < s * 0.6f && dx < s * 1.6f))) return 2;   // em cruz
    return dx < s * 0.7f && dy < s * 0.7f ? 1 : 0;
}
static uint16_t ceu_cosmico(int tipo, float u, float v, float s, int nivel, uint16_t c, uint16_t cinza)
{
    float ro = sqrtf(u * u + v * v), ax = fabsf(u), ay = fabsf(v);
    bool xadrez = ((int)floorf(u / s) + (int)floorf(v / s)) & 1;                     // o pontilhado das bordas de luz
    switch (tipo) {
    case D_ASTRO: {                               // o ponto de luz: o miolo branco, o halo de ouro e quatro raios
        if (ro < 0.27f) return corpo_apagado ? cinza : COSMO[CO_BRANCO];
        if (ro < 0.37f) return corpo_apagado ? cinza : COSMO[CO_TRIGO];
        float fino = s * 0.55f;
        bool raio = (ax < fino + 0.035f * (1 - ay / 0.74f) && ay < 0.74f) || (ay < fino + 0.035f * (1 - ax / 0.74f) && ax < 0.74f);
        if (raio) return corpo_apagado ? cinza : (ro < 0.5f ? COSMO[CO_TRIGO] : COSMO[CO_OURO]);
        float d1 = fabsf(u - v) * 0.7071f, d2 = fabsf(u + v) * 0.7071f;
        if ((d1 < fino || d2 < fino) && ro < 0.52f) return corpo_apagado ? cinza : COSMO[CO_OURO];
        if (corpo_apagado) return c;
        if (ro < 0.43f) return COSMO[CO_OURO];
        if (ro < 0.50f && xadrez) return COSMO[CO_OURO];                             // o halo se desfazendo no ceu
        if (ro < 0.58f && xadrez && ((int)floorf(v / s) & 1)) return COSMO[CO_AZUL];
        int e = cosmo_estrelinha(u, v, s, 0.17f, 7, 38, 0.62f);
        return e == 2 ? COSMO[CO_BRANCO] : e ? COSMO[CO_CEU] : c;
    }
    case D_CONSTELACAO: {                         // estrelas ligadas por fios: o W em cima, a cauda embaixo
        static const float P[7][2] = { { -0.52f, -0.16f }, { -0.30f, -0.48f }, { -0.02f, -0.30f }, { 0.26f, -0.54f },
                                       { 0.52f, -0.20f }, { 0.20f, 0.48f }, { -0.14f, 0.62f } };
        for (int i = 0; i < 7; i++) {
            float x = u - P[i][0], y = v - P[i][1], d = x * x + y * y;
            bool apaga = (int)(t_relogio * 1.3f + i * 1.7f) % 7 == 0;                // uma de cada vez pisca
            if (d < 0.065f * 0.065f) return corpo_apagado ? cinza : COSMO[apaga ? CO_TRIGO : CO_BRANCO];
            if (d < 0.12f * 0.12f && (fabsf(x) < s * 0.6f || fabsf(y) < s * 0.6f)) return corpo_apagado ? cinza : COSMO[CO_TRIGO];
        }
        if (corpo_apagado) return c;
        for (int i = 0; i < 6; i++)
            if (dist_seg(u, v, P[i][0], P[i][1], P[i + 1][0], P[i + 1][1]) < s * 0.55f) return COSMO[CO_CEU];
        int e = cosmo_estrelinha(u, v, s, 0.15f, 41, 30, 0.0f);
        return e ? COSMO[CO_AZUL] : c;
    }
    case D_GALAXIA: {                             // dois bracos em espiral, girando devagar, e o miolo claro
        if (ro < 0.10f) return corpo_apagado ? cinza : COSMO[CO_BRANCO];
        if (ro < 0.17f) return corpo_apagado ? cinza : COSMO[CO_LILAS];
        if (ro < 0.25f) return corpo_apagado ? cinza : (xadrez ? COSMO[CO_LILAS] : COSMO[CO_AMEIXA]);
        if (corpo_apagado) return c;
        float a = atan2f(v, u);
        float b = fr((a + t_relogio * 0.45f - 2.6f * logf(ro + 0.04f)) / 3.14159f);  // 0 no meio de um braco
        float w = 0.13f + 0.12f * (1 - ro);
        uint32_t h = hash2((int)floorf(u / s) + 500, (int)floorf(v / s) + 500);
        if (ro < 0.90f && b < w * 0.55f) return h % 9 == 0 ? COSMO[CO_LILAS] : h % 23 == 1 ? COSMO[CO_BRANCO] : COSMO[CO_AMEIXA];
        if (ro < 0.90f && b < w) return h % 3 ? COSMO[CO_ROXO] : c;
        if (ro < 0.90f && b > 1 - w * 0.4f) return h % 2 ? COSMO[CO_ROXO] : c;       // a poeira atras do braco
        int e = cosmo_estrelinha(u, v, s, 0.16f, 77, 26, 0.0f);
        return e == 2 ? COSMO[CO_LILAS] : e ? COSMO[CO_AMEIXA] : c;
    }
    case D_BURACO: {                              // o escuro, o anel de luz e o disco de fogo girando em volta
        const float hor = 0.34f;
        float el = (u / 0.88f) * (u / 0.88f) + (v / 0.27f) * (v / 0.27f);
        bool disco = el > 0.50f && el < 1.0f && (v > 0 || ro > hor + 0.05f);         // a metade da frente passa por cima do escuro
        if (disco) {
            if (corpo_apagado) return cinza;
            float a = atan2f(v * 3.2f, u) - t_relogio * 1.6f;                         // o redemoinho
            uint32_t h = hash2((int)floorf(a * 5.0f) + 900, (int)floorf(el * 9.0f));
            if (el < 0.64f) return h % 4 ? COSMO[CO_TRIGO] : COSMO[CO_AMBAR];
            if (el < 0.82f) return h % 3 ? COSMO[CO_AMBAR] : COSMO[CO_FOGO];
            return h % 3 ? COSMO[CO_FOGO] : COSMO[CO_VINHO];
        }
        if (fabsf(ro - hor) < s * 0.7f + 0.025f) return corpo_apagado ? cinza : COSMO[v < 0 ? CO_TRIGO : CO_AMBAR];   // o anel de luz curvada
        if (ro < hor) return corpo_apagado ? c : COSMO[CO_BREU];
        if (corpo_apagado) return c;
        if (ro < hor + 0.12f && v < 0 && xadrez) return COSMO[CO_FOGO];               // o disco de tras, dobrado por cima
        int e = cosmo_estrelinha(u, v, s, 0.17f, 13, 22, 0.55f);
        return e ? COSMO[CO_VINHO] : c;
    }
    case D_UNIVERSO: {                            // a esfera: nebulosas, estrelas, a luz de cima e a beira escura
        if (corpo_apagado) {
            uint32_t h = hash2((int)floorf(u / s) + 300, (int)floorf(v / s) + 300);
            return h % 29 == 0 ? cinza : c;
        }
        float tt = t_relogio * 0.05f;
        float neb = ruido(u * 2.3f + 3 + tt, v * 2.3f + 1) + 0.5f * ruido(u * 5.0f + 9, v * 5.0f + 2 - tt);
        float rubor = ruido(u * 2.8f + 21 - tt, v * 2.8f + 12);
        static const uint8_t ESCALA[] = { CO_BREU, CO_NOITE, CO_ROXO_E, CO_ROXO, CO_AMEIXA, CO_LILAS, CO_BRANCO };
        int i = neb > 0.85f ? 5 : neb > 0.5f ? 4 : neb > 0.2f ? 3 : neb > -0.1f ? 2 : 1;
        i += nivel >= 2 ? 1 : nivel <= -2 ? -1 : 0;                                   // a luz da esfera
        if (ro > 0.86f) i--;                                                          // a beira, mais escura
        i = i < 0 ? 0 : i > 6 ? 6 : i;
        uint16_t cor = COSMO[ESCALA[i]];
        if (rubor > 0.62f && i >= 2 && i <= 4) cor = COSMO[CO_VINHO];                  // o vinho das nebulosas, aqui e ali
        uint32_t h = hash2((int)floorf(u / s) + 300, (int)floorf(v / s) + 300);
        bool pisca = (int)(t_relogio * 1.7f + (h >> 6) % 11) % 7 == 0;
        if (h % 37 == 0 && !pisca) return COSMO[(h >> 9) % 3 ? CO_BRANCO : CO_TRIGO];
        if (h % 53 == 1) return COSMO[CO_GELO];
        if (nivel >= 2 && xadrez && ro < 0.7f) return COSMO[CO_CEU];                  // o reflexo, pontilhado
        return cor;
    }
    default:
        return c;
    }
}

// O corpo com a luz e o desenho de dentro: o poligono (ou o disco da moeda,
// quando n == 0) pintado ponto a ponto. (cx, cy) e o centro do dado, r o
// raio. tons: os cinco tons do corpo, da sombra ao reflexo; cores: os tres
// tons do desenho (cores[1..3]).
//
// A luz vem de cima, a esquerda, sempre da tela: o corpo escurece aos poucos
// para baixo e para a direita, e tem um reflexo suave no alto. Por cima,
// manchas largas e discretas, do proprio material, que giram com o dado.
// O desenho de dentro foi feito com o corpo um pouco deslocado do centro (o
// brilho antigo na aresta de cima): (arte_du, arte_dv), em pixels, e onde o
// centro do corpo cai nas coordenadas do desenho. Somado depois de girar, o
// desenho fica preso ao corpo, parado ou girando.
static float arte_du, arte_dv;
static void pinta_corpo(const float *vx, const float *vy, int n, float dcx, float dcy, int dr,
                        int tipo, const uint16_t *tons, const uint16_t *cores,
                        float cx, float cy, float ang, float r)
{
    float ca = cosf(-ang), sa = sinf(-ang), s = 1 / r;
    bool forte;
    float brilho = brilho_lenda(tipo, &forte);
    int tex = textura_de(tipo);
    float desvio = tipo * 3.7f;
    int y0, y1;
    if (n) {
        float ymin = vy[0], ymax = vy[0];
        for (int i = 1; i < n; i++) { if (vy[i] < ymin) ymin = vy[i]; if (vy[i] > ymax) ymax = vy[i]; }
        y0 = (int)ceilf(ymin); y1 = (int)floorf(ymax);
    } else {
        y0 = (int)dcy - dr; y1 = (int)dcy + dr;
    }
    for (int y = y0; y <= y1; y++) {
        float xs[16];
        int nx = 0;
        if (n) {
            for (int i = 0; i < n && nx < 16; i++) {
                int j = (i + 1) % n;
                float ya = vy[i], yb = vy[j];
                if ((ya <= y && yb > y) || (yb <= y && ya > y))
                    xs[nx++] = vx[i] + (y - ya) / (yb - ya) * (vx[j] - vx[i]);
            }
            for (int a = 1; a < nx; a++)
                for (int b = a; b > 0 && xs[b] < xs[b - 1]; b--) { float t = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = t; }
        } else {                                  // o mesmo disco de gfx_disco
            int dy = y - (int)dcy, w = 0;
            while ((w + 1) * (w + 1) + dy * dy <= dr * dr + dr) w++;
            xs[0] = (float)((int)dcx - w); xs[1] = (float)((int)dcx + w); nx = 2;
        }
        for (int k = 0; k + 1 < nx; k += 2)
            for (int x = (int)ceilf(xs[k]); x <= (int)floorf(xs[k + 1]); x++) {
                float dx = x - cx, dy = y - cy, u = dx * ca - dy * sa + arte_du, v = dx * sa + dy * ca + arte_dv;
                int p = desenho_interno(tipo, u * s, v * s, s);
                uint16_t c;
                if (p) c = cores[p];
                else {
                    float hx = dx * s + 0.34f, hy = dy * s + 0.38f;           // o reflexo
                    float reflexo = mat_desta == MAT_BRONZE ? 2.1f : mat_desta == MAT_BOLA ? 2.6f   // o metal e o espelho
                                  : mat_desta == MAT_VELUDO || mat_desta == MAT_BISCOITO ? 0.7f : 1.6f;  // foscos
                    float luz = -(dx + dy) * s * 1.1f
                              + reflexo * fmaxf(0, 1 - (hx * hx + hy * hy) / 0.045f);
                    bool perola = mat_desta == MAT_PEROLA || mat_desta == MAT_PEROLA_NEGRA;
                    if (mat_desta == MAT_MADEIRA) {                           // o veio: aneis largos e tortos
                        float w = v * s * 5.5f + 1.3f * ruido(u * s * 1.6f + 4.1f, v * s * 1.1f)
                                + 0.25f * ruido(u * s * 6.0f, v * s * 0.8f + 2.0f);
                        luz += 0.85f * (tri(w) - 0.55f);
                    } else if (perola || mat_desta == MAT_MARMORE || mat_desta == MAT_BRONZE || mat_desta == MAT_BOLA
                               || mat_desta == MAT_GELO) {
                        luz += 0.12f * ruido(u * s / 0.4f + desvio, v * s / 0.4f);   // lisos: so uma ondulacao leve
                    } else if (mat_desta == MAT_LAPIS) {                      // a pedra: manchas largas, mais fortes
                        luz += 0.42f * ruido(u * s / 0.3f + desvio, v * s / 0.3f);
                    } else if (mat_desta == MAT_VELUDO) {                     // o pelo: miudo, e a beira mais clara
                        luz += 0.30f * ruido(u * s / 0.08f + desvio, v * s / 0.08f) + 0.9f * fmaxf(0, (dx * dx + dy * dy) * s * s - 0.62f);
                    } else {
                        luz += 0.16f * ruido(u * s / 0.34f + desvio, v * s / 0.34f)
                             + 0.07f * ruido(u * s / 0.15f, v * s / 0.15f + desvio);
                    }
                    if (padrao(tex, u, v, r / 22)) luz -= 1;
                    int nivel = (int)floorf(luz + bayer(x, y));   // o tom pelo limiar: entre os dois vizinhos, pontilhado
                    nivel = nivel < -2 ? -2 : nivel > 2 ? 2 : nivel;
                    c = tons[nivel + 2];
                    if (perola && (nivel == 0 || nivel == 1)) {               // o furta-cor, em faixas
                        float f = sinf((u * 0.7f + v) * s * 3.6f + 2.2f * ruido(u * s * 1.2f + 1, v * s * 1.2f));
                        bool negra = mat_desta == MAT_PEROLA_NEGRA;           // a negra, em fios mais finos
                        const uint16_t *furta = negra ? PEROLA_NEGRA_FURTA : PEROLA_FURTA;
                        float corte = negra ? 0.84f : 0.62f;
                        if (f > corte) c = furta[0];
                        else if (f < -corte) c = furta[1];
                    }
                    if (mat_desta == MAT_BOLA) {                             // o espelho: a janela refletida e a luz de baixo
                        float jx = dx * s + 0.30f, jy = dy * s + 0.36f;
                        if (fabsf(jx + jy * 0.3f) < 0.07f && fabsf(jy) < 0.16f) c = tons[4];
                        else if (fabsf(jx - 0.10f + jy * 0.3f) < 0.035f && fabsf(jy) < 0.12f) c = tons[3];
                        float bx = dx * s - 0.40f, by = dy * s - 0.42f;
                        if (bx * bx + by * by < 0.012f && nivel < 1) c = tons[3];
                    } else if (mat_desta == MAT_BISCOITO) {                  // as migalhas: furinhos e grao tostado
                        uint32_t hm = hash2((int)floorf(u * s * 9 + 40), (int)floorf(v * s * 9 + 40)) % 29;
                        if (hm == 0) c = BISCOITO_MIGALHA[0];
                        else if (hm == 1 && nivel < 2) c = BISCOITO_MIGALHA[1];
                    } else if (mat_desta == MAT_GELO) {                      // o floco gravado, girando com o dado
                        float fu = u * s, fv = v * s, fro = sqrtf(fu * fu + fv * fv);
                        float an = atan2f(fv, fu) / 1.0472f, br = fabsf(fr(an + 0.5f) - 0.5f) * 1.0472f * fro;
                        bool sulco = fro < 0.70f && br < 0.028f;              // os seis bracos
                        float ra = fro - 0.40f;                               // e os galhinhos, em V
                        if (fabsf(ra) < 0.13f && fabsf(br - fabsf(ra) * 0.7f) < 0.024f && ra > -0.02f) sulco = true;
                        if (sulco) c = GELO_FLOCO[0];
                        else if (fro < 0.70f && br < 0.05f && fu - fv > 0) c = GELO_FLOCO[1];   // a luz na beira do sulco
                    }
                    if (mat_desta == MAT_BRONZE) {                           // o metal: um risco de luz na diagonal
                        float d = fabsf((dx - dy) * s * 0.7f + (dx + dy) * s * 0.25f + 0.05f);
                        if (d < 0.13f && nivel < 2) c = tons[nivel + 3];
                    }
                    if (mat_desta == MAT_MARMORE) {                           // os veios: finos, tortos, que se cruzam
                        float f1 = (u + 0.6f * v) * s * 1.7f + 1.6f * ruido(u * s * 1.4f + 3.3f, v * s * 1.4f);
                        float f2 = (v - 0.4f * u) * s * 1.2f + 1.3f * ruido(u * s * 2.1f, v * s * 2.1f + 5.7f);
                        float d1 = fabsf(fr(f1) - 0.5f), d2 = fabsf(fr(f2 + 0.3f) - 0.5f);
                        if (d1 < 0.035f) c = MARMORE_VEIO[0];
                        else if (d1 < 0.06f || d2 < 0.03f) c = MARMORE_VEIO[1];
                    } else if (mat_desta == MAT_LAPIS && hash2((int)floorf(u) + 91, (int)floorf(v) + 37) % 43 == 0) {
                        c = LAPIS_OURO;                                       // as pintas de pirita, girando com o dado
                    }
                    if (corpo_desta == D_PANETONE) c = miolo_panetone(u * s, v * s, s, nivel, c, cores[1]);
                    else if (corpo_desta == D_BONECO) c = neve_boneco(u * s, v * s, s, nivel, c, cores[1]);
                    else if (cosmico(corpo_desta)) c = ceu_cosmico(corpo_desta, u * s, v * s, s, nivel, c, cores[1]);
                }
                if (brilho > -5) {
                    float d = fabsf((dx + dy) * s - brilho), d2 = fabsf((dx + dy) * s - brilho + 0.3f);
                    if (forte) {                       // o do lendario: miolo branco, beira dourada
                        if (d < 0.06f) c = C_BRANCO;
                        else if (d < 0.12f) c = gfx_mistura(c, C_AMARELO, 50);
                        else if (d2 < 0.04f) c = gfx_mistura(c, C_BRANCO, 75);
                    } else {                           // o de passagem: um fio claro e outro mais fraco
                        if (d < 0.05f) c = gfx_mistura(c, C_BRANCO, 80);
                        else if (d2 < 0.03f) c = gfx_mistura(c, C_BRANCO, 50);
                    }
                }
                gfx_pixel(x, y, c);
            }
    }
}

// ---------------------------------------------------------------------------
// As pecas desenhadas a parte, como sempre foram.
// ---------------------------------------------------------------------------
static float mca, msa, mcx, mcy;            // a rotacao do dado em curso
static int MX(float ox, float oy) { return (int)lroundf(mcx + ox * mca - oy * msa); }
static int MY(float ox, float oy) { return (int)lroundf(mcy + ox * msa + oy * mca); }

// Retangulo girado com o dado: centro (ox, oy), meia-largura hw, meia-altura hh.
static void marca_ret(float ox, float oy, float hw, float hh, uint16_t c)
{
    float o[8] = { ox - hw, oy - hh, ox + hw, oy - hh, ox + hw, oy + hh, ox - hw, oy + hh };
    float vx[4], vy[4];
    for (int i = 0; i < 4; i++) {
        vx[i] = mcx + o[2 * i] * mca - o[2 * i + 1] * msa;
        vy[i] = mcy + o[2 * i] * msa + o[2 * i + 1] * mca;
    }
    poligono(vx, vy, 4, c);
}

// Espessura dos tracos: 1 px num dado pequeno, mais num grande.
static int esp_marca(float t) { return t < 12 ? 1 : (int)(t / 11); }

// Por cima do corpo: as tres caixinhas com o 7 do Jackpot (embaixo do
// dado), as seis casas do Acumulador (acesas: o bonus guardado) e a
// rachadura do Quebrado.
static void marca_por_cima(int tipo, float r, uint16_t corpo, bool zerado)
{
    float t = r < 5 ? 5 : r;
    if (tipo == D_QUEBRADO) {
        uint16_t rc = C_EBANO_B;
        int e = esp_marca(r);
        // no alto: o sinal de menos da pedra negativa fica livre
        gfx_linha_grossa(MX(-0.66f * r, -0.66f * r), MY(-0.66f * r, -0.66f * r), MX(-0.24f * r, -0.26f * r), MY(-0.24f * r, -0.26f * r), e, rc);
        gfx_linha_grossa(MX(-0.24f * r, -0.26f * r), MY(-0.24f * r, -0.26f * r), MX(0.08f * r, -0.54f * r), MY(0.08f * r, -0.54f * r), e, rc);
    }
    if (zerado) return;
    if (tipo == D_JACKPOT) {
        float y = 1.16f * t, h = 0.19f * t + 0.8f, passo = 0.46f * t + 1;
        marca_ret(0, y, passo * 1.5f + h * 0.3f, h + 1.2f, C_LATAO_ESC);
        int e = t < 14 ? 1 : 2;
        for (int i = -1; i <= 1; i++) {
            float bx = i * passo;
            marca_ret(bx, y, h, h, C_MARFIM);
            // o 7: o traco de cima e a perna em diagonal
            gfx_linha_grossa(MX(bx - h * 0.55f, y - h * 0.6f), MY(bx - h * 0.55f, y - h * 0.6f),
                             MX(bx + h * 0.55f, y - h * 0.6f), MY(bx + h * 0.55f, y - h * 0.6f), e, C_VERMELHO);
            gfx_linha_grossa(MX(bx + h * 0.55f, y - h * 0.6f), MY(bx + h * 0.55f, y - h * 0.6f),
                             MX(bx - h * 0.15f, y + h * 0.7f), MY(bx - h * 0.15f, y + h * 0.7f), e, C_VERMELHO);
        }
    }
    if (tipo == D_ACUMULADOR) {
        int n = marca_nivel < 0 ? 0 : (marca_nivel > 6 ? 6 : marca_nivel);
        static const float P[6][2] = { {0,-0.74f}, {0.56f,-0.3f}, {0.56f,0.3f}, {0,0.74f}, {-0.56f,0.3f}, {-0.56f,-0.3f} };
        int pr = (int)(0.08f * t + 0.8f);
        for (int i = 0; i < 6; i++) {
            int x = MX(P[i][0] * t, P[i][1] * t), y = MY(P[i][0] * t, P[i][1] * t);
            gfx_disco(x, y, pr + 1, gfx_mistura(corpo, C_BARRA, 30));
            gfx_disco(x, y, pr, i < n ? RGB(238, 162, 74) : gfx_mistura(corpo, C_BARRA, 12));
        }
    }
}

// Borda de moeda serrilhada: aro com dentinhos em volta.
static void serrilha(float cx, float cy, int R, uint16_t aro, uint16_t dente)
{
    gfx_disco((int)cx, (int)cy, R, aro);
    int n = R < 14 ? 12 : 20;
    for (int i = 0; i < n; i++) {
        float a = i * 6.2832f / n;
        gfx_pixel((int)(cx + cosf(a) * (R - 1)), (int)(cy + sinf(a) * (R - 1)), dente);
        if (R >= 14) gfx_pixel((int)(cx + cosf(a) * (R - 2)), (int)(cy + sinf(a) * (R - 2)), dente);
    }
}

// Tudo ou Nada: moeda de ouro. Na mao, yin-yang de marfim e ebano (pode dar
// tudo, pode dar nada); caida, o lado que saiu: TUDO (2) marfim com brilhos
// dourados, NADA (1) ebano rachado.
static void tudo_ou_nada(float cx, float cy, float r, float ang, int valor, bool zerado)
{
    int R = (int)(r + (r < 12 ? 1 : r / 10)), Ri = R - (R < 14 ? 2 : 4);
    serrilha(cx, cy, R, zerado ? C_FELTRO2 : C_OURO, zerado ? C_FELTRO_ESC : C_LATAO_ESC);
    if (valor < 0) {                                   // yin-yang, girando com a moeda
        float ca = cosf(-ang), sa = sinf(-ang), h = Ri / 2.0f;
        for (int dy = -Ri; dy <= Ri; dy++)
            for (int dx = -Ri; dx <= Ri; dx++) {
                if (dx * dx + dy * dy > Ri * Ri) continue;
                float u = dx * ca - dy * sa, v = dx * sa + dy * ca;
                float dc = u * u + (v + h) * (v + h), db = u * u + (v - h) * (v - h);
                bool claro = dc <= h * h ? true : db <= h * h ? false : u < 0;
                float olho = h * 0.3f;
                if (u * u + (v + h) * (v + h) <= olho * olho) claro = false;
                if (u * u + (v - h) * (v - h) <= olho * olho) claro = true;
                gfx_pixel((int)cx + dx, (int)cy + dy, claro ? (zerado ? C_MARFIM_S : C_MARFIM)
                                                            : (zerado ? C_EBANO_B : C_EBANO));
            }
        return;
    }
    bool tudo = valor == 2;
    uint16_t face = zerado ? C_EBANO_B : tudo ? C_MARFIM : C_EBANO;
    gfx_disco((int)cx, (int)cy, Ri, face);
    if (!zerado && r >= 12) {
        // anel fino por dentro: dourado no TUDO, violeta no NADA
        for (int i = 0; i < 48; i++) {
            float a = i * 0.1309f;
            gfx_pixel((int)(cx + cosf(a) * (Ri - 3)), (int)(cy + sinf(a) * (Ri - 3)), tudo ? C_OURO : C_VIOLETA);
        }
        if (tudo)                                       // brilhos dourados
            for (int i = 0; i < 4; i++) {
                float a = ang + 0.6f + i * 1.5708f;
                int bx = (int)(cx + cosf(a) * Ri * 0.62f), by = (int)(cy + sinf(a) * Ri * 0.62f);
                gfx_rect(bx - 2, by, 5, 1, C_OURO);
                gfx_rect(bx, by - 2, 1, 5, C_OURO);
            }
        else {                                          // rachadura
            int e = esp_marca(r);
            gfx_linha_grossa((int)(cx + Ri * 0.15f), (int)(cy + Ri * 0.85f), (int)(cx + Ri * 0.35f), (int)(cy + Ri * 0.55f), e, C_EBANO_B);
            gfx_linha_grossa((int)(cx + Ri * 0.35f), (int)(cy + Ri * 0.55f), (int)(cx + Ri * 0.65f), (int)(cy + Ri * 0.62f), e, C_EBANO_B);
        }
    }
    char n[2] = { (char)('0' + valor), 0 };
    numero(cx, cy, r, n, zerado ? C_MARFIM_S : tudo ? C_REAL : C_MARFIM);
    if (zerado) risca(cx, cy, r);
}

// Ficha de Fogo: ficha de cassino laranja (lado 1) e brasa (lado 2), com
// marcas na borda. Na mao, o centro mostra a chama; jogada, o valor.
static void ficha_de_fogo(float cx, float cy, float r, float ea, float ang, int valor, int vista, bool zerado)
{
    int R = (int)(r + ea);
    float u = r / 9;                             // unidade do desenho
    uint16_t face = vista == 0 || valor == 1 ? C_FOGO : C_BRASA;
    gfx_disco((int)cx, (int)cy, R, gfx_mistura(face, C_BARRA, 25));
    gfx_disco((int)cx, (int)cy - 1, R - 1, face);
    for (int i = 0; i < 8; i++) {                // marcas da borda
        float a = ang + i * 0.785f;
        int mx = (int)(cx + cosf(a) * (R - 2 * u)), my = (int)(cy + sinf(a) * (R - 2 * u));
        gfx_disco(mx, my, (int)(u + 0.5f), C_AMARELO);
    }
    gfx_disco((int)cx, (int)cy, (int)(R - 4 * u), face);
    if (vista != 0) {
        char s2[4] = { (char)('0' + valor), 0 };
        numero(cx, cy, r, s2, C_MARFIM);
    } else {                                     // a chama
        int U = (int)(u + 0.5f);
        gfx_rect((int)cx, (int)(cy - 5 * u), U, U, C_AMARELO);
        gfx_rect((int)(cx - u), (int)(cy - 4 * u), 3 * U, 2 * U, C_AMARELO);
        gfx_rect((int)(cx - 2 * u), (int)(cy - 2 * u), 5 * U, 4 * U, C_AMARELO);
        gfx_rect((int)(cx - u), (int)(cy + 2 * u), 3 * U, U, C_AMARELO);
        gfx_rect((int)cx, (int)(cy - u), U, 2 * U, C_BRASA);
    }
    if (zerado) risca(cx, cy, r);
}

#define SO_CORPO (-1000)                 // valor que pede so o corpo, sem numero
// vista 0: parado na mao, mostra quantos lados tem.
// vista 1: rolando, mostra uma face qualquer.
// vista 2: pousado, mostra o valor final (e riscado se zerou).
// O Oculto ainda virado: a face mostra "?" no lugar do numero.
static bool face_escondida;
// O dado com a capa do Impermeavel: o Vidro com 1 nao aparece estilhacado.
static bool face_capa;
// A Ficha Coringa ainda sem numero: o curinga para cima (quem desenha avisa;
// na mao e no ar, sempre).
static bool face_curinga;
// O visor do Digital: -1 apagado (ainda fora da sequencia: so os tracos),
// de 0 a 1 acendendo (acabou de entrar), 1 aceso. Quem desenha avisa antes;
// volta a 1 depois de cada dado.
static float digital_q = 1;

// O adesivo do lado que esta para cima: quem desenha o dado na mesa avisa
// antes (como o brilho de passagem), e a etiqueta vai na face, por baixo do
// numero. ad_face_aceso: o halo de quando ela acende (rolando, ao passar pelo
// lado; pousando nele; agindo no fim).
static int  ad_face;
static bool ad_face_aceso;

// O vinil de cada adesivo nao e so a cor: tem uma textura discreta, sempre
// nas cores vizinhas da paleta e em poucos pontos, para o numero (que tem o
// contorno da cor do vinil) continuar lendo bem. (X, Y): o ponto em pixels a
// partir do meio da etiqueta, girando com o dado; de: os pixels ate a borda;
// r: o raio do dado; fase: o acaso deste dado (o que acende de tempos em
// tempos nao acende em todos juntos); mov: muda quando o dado anda ou gira (o
// Holografico).
static uint16_t ad_textura(int a, float X, float Y, float de, float r, float fase, float mov, uint16_t fundo)
{
    int ix = (int)floorf(X), iy = (int)floorf(Y);
    uint32_t h = hash2(ix + 64, iy + 64);
    float t = t_relogio, dist = sqrtf(X * X + Y * Y);
    switch (a) {
    case AD_CHUMBO: {                             // metal escovado: riscos finos na horizontal
        uint32_t q = hash2((int)floorf(X / 5) + 64, iy + 64) % 9;
        if (q == 0) return C_FERRO;
        if (q == 1 && (h & 3) == 0) return RGB(36, 32, 29);
        return fundo;
    }
    case AD_REFLEXIVO: {                          // o reflexo: uma faixa de luz e duas luzinhas que piscam
        float f = X + Y + r * 0.45f;
        if (f > 0 && f < 2) return C_BRANCO;
        if (f > 3.5f && f < 4.5f && (ix & 1)) return C_BRANCO;
        static const float L[2][3] = { { -0.42f, 0.30f, 0.0f }, { 0.36f, -0.34f, 2.4f } };
        for (int i = 0; i < 2; i++) {
            float lx = X - L[i][0] * r, ly = Y - L[i][1] * r, s = sinf(t * 1.7f + L[i][2] + fase * 6.3f);
            if (s < 0.2f) continue;
            float b = s > 0.75f ? 2.5f : 1.5f;     // no pico, a cruz cresce
            if ((fabsf(lx) < 0.5f && fabsf(ly) < b) || (fabsf(ly) < 0.5f && fabsf(lx) < b)) return C_BRANCO;
        }
        return fundo;
    }
    case AD_CONFETE: {                            // papeizinhos de cor, espalhados
        static const uint16_t K[6] = { RGB(246, 222, 132), RGB(168, 204, 230), RGB(208, 98, 108),
                                       RGB(130, 204, 112), RGB(206, 180, 218), RGB(252, 248, 236) };
        int cxl = (int)floorf(X / 5), cyl = (int)floorf(Y / 5);
        uint32_t q = hash2(cxl + 64, cyl + 64);
        if (q % 3) return fundo;
        int px = cxl * 5 + 1 + (int)(q >> 4) % 3, py = cyl * 5 + 1 + (int)(q >> 6) % 3;
        bool deitado = q >> 8 & 1;
        if ((ix == px || ix == px + deitado) && (iy == py || iy == py + !deitado)) return K[(q >> 9) % 6];
        return fundo;
    }
    case AD_ELETRONICO: {                         // o raio, apagado; de tempos em tempos, acende (duas piscadas)
        float c = r * 0.17f;
        int i = (int)floorf(X / c + 3.5f), j = (int)floorf(Y / c + 3.5f);
        if (i < 0 || i > 6 || j < 0 || j > 6 || ADESIVO[a].ic[j][i] != '#') return fundo;
        float p = fr(t / 4.5f + fase);
        if (p < 0.02f || (p > 0.035f && p < 0.065f)) return p < 0.02f ? C_VENTO : C_BRANCO;
        return (ix + iy) & 1 ? RGB(72, 150, 138) : fundo;
    }
    case AD_DOURADO: {                            // folha de ouro: o grao, e um brilho que passa de vez em quando
        float p = fr(t / 4 + fase) * 4, pos = -1.7f * r + p / 0.55f * 3.4f * r, f = X + Y - pos;
        if (p < 0.55f && fabsf(f) < 1) return C_BRANCO;
        if (p < 0.55f && fabsf(f) < 2.5f) return C_AMARELO;
        if (h % 11 == 0) return C_AMARELO;
        if (h % 13 == 1) return RGB(190, 156, 86);
        return fundo;
    }
    case AD_LOBO: {                               // tres arranhoes no canto
        for (int i = 0; i < 3; i++) {
            float ax = (0.12f + i * 0.15f) * r, ay = 0.05f * r, bx = ax + 0.24f * r, by = 0.6f * r;
            float vx = bx - ax, vy = by - ay, w = fmaxf(0, fminf(1, ((X - ax) * vx + (Y - ay) * vy) / (vx * vx + vy * vy)));
            float ex = X - ax - w * vx, ey = Y - ay - w * vy;
            if (ex * ex + ey * ey < (w > 0.15f && w < 0.85f ? 0.55f : 0.25f)) return RGB(118, 30, 38);
        }
        return fundo;
    }
    case AD_REMENDO: {                            // o pesponto em volta, e a trama do pano
        if (de >= 2 && de < 3) return (int)floorf(atan2f(Y, X) * r * 0.62f / 2) & 1 ? RGB(130, 204, 112) : fundo;
        if (de >= 4 && ((ix & 3) == 0 && (iy & 1) == 0)) return RGB(72, 150, 138);
        return fundo;
    }
    case AD_RASPADINHA: {                         // ranhuras curtas na diagonal, de onde ja rasparam
        int d1 = (int)floorf(X - Y + 200), d2 = (int)floorf((X + Y) / 3);
        if (d1 % 5 > 1) return fundo;
        uint32_t q = hash2(d1 / 5, d2 + 64);
        if (q % 3) return fundo;
        return d1 % 5 == 0 ? C_BRANCO : RGB(150, 112, 160);
    }
    case AD_FERMENTO: {                           // bolhinhas subindo devagar
        float yy = Y + t * 2.5f;
        int cxl = (int)floorf(X / 6), cyl = (int)floorf(yy / 6);
        uint32_t q = hash2(cxl + 64, cyl + 64);
        if (q % 3) return fundo;
        float bx = cxl * 6 + 2 + (q >> 4) % 3, by = cyl * 6 + 2 + (q >> 6) % 3, bd = (X - bx) * (X - bx) + (yy - by) * (yy - by);
        if (q >> 8 & 1) return bd > 0.8f && bd < 2.6f ? C_AMARELO : fundo;    // bolha de anel
        return bd < 0.5f ? C_AMARELO : fundo;
    }
    case AD_BIS:                                  // borda dupla
        return (de >= 1.5f && de < 2.5f) || (de >= 3.5f && de < 4.5f) ? C_REAL : fundo;
    case AD_MIRA: {                               // a luneta: um anel e quatro marcas
        bool anel = fabsf(dist - 0.5f * r) < 0.6f;
        bool marca = (fabsf(X) < 0.5f && fabsf(Y) > 0.36f * r && fabsf(Y) < 0.66f * r)
                  || (fabsf(Y) < 0.5f && fabsf(X) > 0.36f * r && fabsf(X) < 0.66f * r);
        return anel || marca ? RGB(118, 30, 38) : fundo;
    }
    case AD_AVESSO: {                             // o canto descolando, mostrando o avesso
        float f = X + Y - 0.58f * r;
        if (f > 1) return (ix + iy) & 1 ? C_MARFIM : RGB(204, 194, 172);
        if (f > 0) return RGB(96, 182, 198);
        return fundo;
    }
    case AD_GORJETA: {                            // o desenho de cedula: ondas finas que se cruzam
        float o = 1.4f * sinf(X * 0.5f);
        if (fr((Y + o + 100) / 6) < 0.17f && (ix % 3)) return RGB(96, 134, 196);
        if (fr((Y - o + 103) / 6) < 0.17f && (ix % 3) == 1) return RGB(96, 134, 196);
        return fundo;
    }
    case AD_SELO: {                               // a cera: manchas, e o brilho no alto
        float m = ruido(X / 3.5f + 7, Y / 3.5f);
        if (m > 0.6f) return RGB(150, 62, 80);
        if (m < -0.65f && (h & 1)) return RGB(66, 18, 24);
        return fundo;
    }
    case AD_FRAGIL: {                             // uma rachadura saindo do canto
        static const float P[6][2] = { { 0.70f, -0.70f }, { 0.32f, -0.30f }, { 0.40f, 0.02f }, { 0.12f, 0.30f },
                                       { 0.32f, -0.30f }, { 0.02f, -0.44f } };
        static const int8_t S[4][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 4, 5 } };
        for (int i = 0; i < 4; i++) {
            float ax = P[S[i][0]][0] * r, ay = P[S[i][0]][1] * r, vx = P[S[i][1]][0] * r - ax, vy = P[S[i][1]][1] * r - ay;
            float w = fmaxf(0, fminf(1, ((X - ax) * vx + (Y - ay) * vy) / (vx * vx + vy * vy)));
            float ex = X - ax - w * vx, ey = Y - ay - w * vy;
            if (ex * ex + ey * ey < 0.3f) return C_MADEIRA_CLR;
        }
        return fundo;
    }
    case AD_ESTRELINHA: {                         // estrelinhas miudas, piscando cada uma no seu tempo
        int cxl = (int)floorf(X / 7), cyl = (int)floorf(Y / 7);
        uint32_t q = hash2(cxl + 64, cyl + 64);
        if (q % 3) return fundo;
        int px = cxl * 7 + 2 + (int)(q >> 4) % 3, py = cyl * 7 + 2 + (int)(q >> 6) % 3;
        bool pisca = sinf(t * 2.2f + (q >> 8) % 17) > 0.35f;
        if (ix == px && iy == py) return C_AMARELO;
        if (pisca && abs(ix - px) + abs(iy - py) == 1) return C_AMARELO;
        return fundo;
    }
    case AD_CORRETIVO: {                          // as passadas da fita, uma do lado da outra
        int b = (int)floorf((Y + 0.7f * sinf(X * 0.35f + floorf(Y / 4) * 2)) / 4);
        return b & 1 && (h % 5) ? C_MARFIM_D : fundo;
    }
    case AD_IMANTADO: {                           // as linhas de forca, em limalha
        float py = -1.0f * r, p1 = -0.32f * r, p2 = 0.32f * r;
        float phi = atan2f(Y - py, X - p1) - atan2f(Y - py, X - p2);
        if (fr(phi / 0.32f) < 0.16f && (h & 1)) return C_AZUL;
        return fundo;
    }
    case AD_POSTIT:                               // pautado
        return fr((Y + 100) / 5) < 0.2f ? RGB(228, 194, 106) : fundo;
    case AD_HOLOGRAFICO: {                        // faixas de cor que correm quando o dado anda ou gira
        static const uint16_t K[5] = { RGB(226, 164, 168), RGB(246, 222, 132), RGB(130, 204, 112),
                                       RGB(168, 204, 230), RGB(206, 180, 218) };
        float f = (X * 0.6f + Y) / r * 1.3f + mov;
        if (tri(f * 0.35f) < 0.45f) return fundo;              // so uma faixa de cada vez
        return (ix + iy) & 1 ? K[((int)floorf(f * 2.5f) % 5 + 5) % 5] : fundo;
    }
    case AD_PURPURINA: {                          // purpurina: pontinhos de brilho piscando
        if (h % 9) return fundo;
        static const uint16_t K[3] = { RGB(246, 222, 132), RGB(252, 248, 236), RGB(226, 164, 168) };
        return sinf(t * 3 + (h >> 4) % 29) > -0.2f ? K[(h >> 9) % 3] : RGB(208, 98, 108);
    }
    case AD_VELCRO:                               // os fios: o pelo miudo, mais escuro e mais claro
        if (h % 6 == 0) return RGB(14, 14, 12);
        if (h % 6 == 1 && (iy & 1)) return RGB(62, 56, 50);
        if (h % 29 == 2) return RGB(96, 88, 80);
        return fundo;
    case AD_RATOEIRA: {                           // o veio da tabua
        float w = tri(Y * 0.22f + 0.9f * ruido(X / 7 + 3, Y / 3));
        if (w > 0.86f) return C_MADEIRA;
        if (w < 0.1f && (ix & 1)) return C_BRONZE;
        return fundo;
    }
    case AD_FIGURINHA:                            // a moldura do cromo, e a retícula da impressao
        if (de >= 2 && de < 3) return C_BRANCO;
        if (de >= 4 && (ix % 3) == 0 && (iy % 3) == 0) return RGB(88, 152, 84);
        return fundo;
    case AD_PATINHO: {                            // a agua do lago, com ondinhas indo devagar
        float o = 1.1f * sinf(X * 0.42f + t * 1.4f);
        return fr((Y + o + 100) / 6) < 0.16f && (ix % 4) ? C_BRANCO : fundo;
    }
    case AD_ETIQUETA: {                           // o papel pardo: fibras curtas
        uint32_t q = hash2((int)floorf(X / 3) + 64, iy + 64) % 13;
        if (q == 0) return RGB(190, 156, 86);
        if (q == 1) return RGB(106, 86, 44);
        return fundo;
    }
    case AD_BILHETE: {                            // o picote do canhoto, e um po de ouro
        if (fabsf(X - 0.42f * r) < 0.6f && (iy & 1)) return C_REAL;
        if (h % 23 == 0) return RGB(150, 122, 62);
        return fundo;
    }
    case AD_NOTA_DEZ: {                           // a roseta de nota: a beira de ouro em ondinhas, e estrelinhas piscando
        float an = atan2f(Y, X), onda = de - 0.9f * (0.5f + 0.5f * sinf(an * 9));
        if (onda >= 1.0f && onda < 2.4f) return (ix + iy) & 1 ? C_AMARELO : C_OURO;
        int cxl = (int)floorf(X / 6), cyl = (int)floorf(Y / 6);
        uint32_t q = hash2(cxl + 64, cyl + 64);
        if (q % 4) return fundo;
        int px = cxl * 6 + 2 + (int)(q >> 4) % 3, py = cyl * 6 + 2 + (int)(q >> 6) % 3;
        if (ix == px && iy == py) return C_AMARELO;
        if (sinf(t * 2.6f + (q >> 8) % 19) > 0.55f && abs(ix - px) + abs(iy - py) == 1) return C_ROSA;
        return fundo;
    }
    case AD_CARBONO: {                            // a folha de copia: a letra que passou, linha por linha, e o brilho da cera
        float p = fr(t / 5 + fase) * 5, pos = -1.8f * r + p / 0.6f * 3.6f * r;
        if (p < 0.6f && fabsf(X - 0.5f * Y - pos) < 1.0f) return RGB(96, 134, 196);   // o brilho passando
        int linha = (int)floorf((Y + 100) / 5), dy = iy + 100 - linha * 5;
        int palavra = (int)floorf((X + 100 + linha * 7) / 6);
        if (hash2(palavra, linha) % 4 == 0 || ((int)floorf(X + 100 + linha * 7) % 6) == 5) return fundo;   // entre as palavras
        int sobe = (ix + linha) & 2 ? 1 : 0;      // a letra corrida: sobe e desce um pixel
        if (dy == 2 - sobe || (dy == 1 && (hash2(ix + 64, linha) % 7) == 0)) return C_AZUL;
        return fundo;
    }
    case AD_BOMBA: {                              // o papel da etiqueta: a moldura vermelha, as fibras e, de vez em quando, o pavio correndo
        if (de >= 1.6f && de < 2.6f) {
            float an = atan2f(Y, X) / 6.2832f + 0.5f, q = fr(t / 3.5f + fase) * 3.5f;   // o fogo da a volta em 1 s
            float d = fabsf(an - q);
            if (q < 1 && d < 0.035f) return (h & 1) ? C_AMARELO : C_FOGO;
            if (q < 1 && an < q && q - an < 0.14f) return (h & 1) ? C_EBANO_B : C_BRASA;   // o rastro queimado
            return C_VERMELHO;
        }
        uint32_t q = hash2((int)floorf(X / 3) + 64, iy + 64) % 15;
        if (q == 0) return RGB(166, 154, 132);
        if (q == 1) return C_BRANCO;
        return fundo;
    }
    case AD_IMPERMEAVEL: {                        // o plastico da capa: gotas que escorrem devagar, com o brilho de cada uma
        float yy = Y - t * 1.4f;
        int cxl = (int)floorf(X / 6), cyl = (int)floorf(yy / 7);
        uint32_t q = hash2(cxl + 64, cyl + 64);
        if (q % 3) return fundo;
        float bx = cxl * 6 + 2.5f + (q >> 4) % 2, by = cyl * 7 + 3 + (q >> 6) % 2, dx = X - bx, dy = yy - by;
        if (dx * dx + dy * dy < 1.6f) return dx < -0.3f && dy < -0.3f ? C_BRANCO : RGB(96, 134, 196);
        if (fabsf(dx) < 0.5f && dy < -1.2f && dy > -3.5f && (q >> 9 & 1)) return RGB(228, 194, 106);   // o rastro da gota
        return fundo;
    }
    case AD_LENTICULAR: {                         // as lentes em listras finas; os aneis de baixo pulam quando o angulo muda
        if (((ix % 3) + 3) % 3 == 0) return RGB(102, 74, 138);
        float d = dist + (sinf(mov * 2.4f + fase * 6) > 0 ? 2.2f : 0.0f);
        if (fr(d / 5) < 0.28f) return (ix + iy) & 1 ? RGB(206, 180, 218) : fundo;
        return fundo;
    }
    case AD_NEON: {                               // o tubo de luz na parede escura, respirando: o brilho em volta cresce e
                                                  // encolhe, o tubo fica branco no pico; de vez em quando pisca e apaga
        float p = fr(t / 6 + fase), pulso = 0.5f + 0.5f * sinf(t * 4.2f + fase * 6.3f);
        bool apaga = (p > 0.02f && p < 0.035f) || (p > 0.05f && p < 0.06f);
        if (de >= 2.2f && de < 3.2f) {                       // o tubo: de vermelho apagado a branco
            if (apaga) return C_VINHO;
            if (pulso < 0.25f) return C_VINHO_CLR;
            return pulso > 0.65f && (ix + iy) & 1 ? C_BRANCO : C_ROSA;
        }
        float fora = de < 2.2f ? 2.2f - de : de - 3.2f, alcance = 1.0f + 3.8f * pulso;
        if (!apaga && fora < alcance && bayer(ix, iy) < (1 - fora / alcance) * (0.35f + 0.65f * pulso))
            return fora < 1.6f && pulso > 0.5f ? C_VINHO_CLR : C_VINHO;
        int linha = (int)floorf((Y + 100) / 4);
        if (fr((Y + 100) / 4) < 0.22f || fr((X + 100 + (linha & 1) * 3) / 6) < 0.15f) return C_EBANO;   // os tijolos
        return fundo;
    }
    default: return fundo;
    }
}

// O vinil pintado ponto a ponto, com a textura: o poligono (ou o disco, n ==
// 0, com o mesmo contorno de gfx_disco) com o meio em (cx, cy).
static void ad_vinil(const float *vx, const float *vy, int n, int rr, int a, uint16_t fundo,
                     float cx, float cy, float ang, float r)
{
    float ca = cosf(-ang), sa = sinf(-ang);
    float nxs[12], nys[12], hs[12];               // as normais dos lados e a distancia do meio a cada um
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float ex = vx[j] - vx[i], ey = vy[j] - vy[i], l = sqrtf(ex * ex + ey * ey);
        nxs[i] = ey / l; nys[i] = -ex / l;
        hs[i] = (vx[i] - cx) * nxs[i] + (vy[i] - cy) * nys[i];
        if (hs[i] < 0) { nxs[i] = -nxs[i]; nys[i] = -nys[i]; hs[i] = -hs[i]; }
    }
    float fase = (hash2((int)cx >> 4, (int)cy >> 4) & 255) / 256.0f;
    float mov = ang * 0.8f + (cx + cy) * 0.02f;
    int y0, y1;
    if (n) {
        float ymin = vy[0], ymax = vy[0];
        for (int i = 1; i < n; i++) { if (vy[i] < ymin) ymin = vy[i]; if (vy[i] > ymax) ymax = vy[i]; }
        y0 = (int)ceilf(ymin); y1 = (int)floorf(ymax);
    } else {
        y0 = (int)cy - rr; y1 = (int)cy + rr;
    }
    for (int y = y0; y <= y1; y++) {
        float xs[16];
        int nx = 0;
        if (n) {
            for (int i = 0; i < n && nx < 16; i++) {
                int j = (i + 1) % n;
                float ya = vy[i], yb = vy[j];
                if ((ya <= y && yb > y) || (yb <= y && ya > y))
                    xs[nx++] = vx[i] + (y - ya) / (yb - ya) * (vx[j] - vx[i]);
            }
            for (int p = 1; p < nx; p++)
                for (int b = p; b > 0 && xs[b] < xs[b - 1]; b--) { float q = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = q; }
        } else {
            int dy = y - (int)cy, w = 0;
            while ((w + 1) * (w + 1) + dy * dy <= rr * rr + rr) w++;
            xs[0] = (float)((int)cx - w); xs[1] = (float)((int)cx + w); nx = 2;
        }
        for (int k = 0; k + 1 < nx; k += 2)
            for (int x = (int)ceilf(xs[k]); x <= (int)floorf(xs[k + 1]); x++) {
                float dx = x - cx, dy = y - cy, de = 1e9f;
                if (n) for (int i = 0; i < n; i++) de = fminf(de, hs[i] - dx * nxs[i] - dy * nys[i]);
                else de = rr - sqrtf(dx * dx + dy * dy);
                gfx_pixel(x, y, ad_textura(a, dx * ca - dy * sa, dx * sa + dy * ca, de, r, fase, mov, fundo));
            }
    }
}

// A etiqueta: o vinil na forma do dado, com borda branca, e o adesivo da loja
// em miniatura no canto. Anulado, apagada como o dado.
static void ad_etiqueta(float cx, float cy, float r, int lados, float ang, int a, bool aceso, bool zerado)
{
    const adesivo_t *d = &ADESIVO[a];
    bool vidro = d->fundo == AD_SEM_FUNDO;
    uint16_t fundo = vidro ? 0 : d->fundo, borda = vidro ? d->luz : C_BRANCO, halo = d->luz;
    if (zerado) {
        fundo = gfx_mistura(fundo, C_FELTRO2, 60);
        borda = gfx_mistura(borda, C_FELTRO2, 60);
        aceso = false;
    }
    float k = lados == 4 ? 0.6f : lados == 6 ? 0.7f : 0.62f, oy = lados == 4 ? r * 0.1f : 0;
    float vx[12], vy[12];
    int n;
    bool textura = !zerado && r >= 10;            // anulada ou miuda, so a cor
    if (lados == 2) {
        int rr = (int)(r * 0.66f);
        if (aceso) gfx_disco((int)cx, (int)cy, rr + 3, halo);
        if (vidro) {
            for (int q = 0; q < 24; q++) {
                float t = q * 0.2618f;
                gfx_pixel((int)(cx + cosf(t) * rr), (int)(cy + sinf(t) * rr), borda);
            }
        } else {
            gfx_disco((int)cx + 1, (int)cy + 1, rr + 1, gfx_mistura(fundo, C_BARRA, 45));
            gfx_disco((int)cx, (int)cy, rr + 1, borda);
            if (textura) ad_vinil(NULL, NULL, 0, rr, a, fundo, cx, cy, ang, r);
            else gfx_disco((int)cx, (int)cy, rr, fundo);
        }
    } else if (vidro) {                          // so o contorno de vidro, e um risco de brilho
        for (int q = 0; q <= (aceso ? 2 : 0); q += 2) {
            n = forma(lados, cx, cy + oy, r * (k + q * 0.07f), ang, vx, vy);
            for (int i = 0; i < n; i++)
                gfx_linha((int)vx[i], (int)vy[i], (int)vx[(i + 1) % n], (int)vy[(i + 1) % n], q ? C_BRANCO : borda);
        }
        gfx_linha((int)(cx - r * 0.38f), (int)(cy - r * 0.05f), (int)(cx - r * 0.05f), (int)(cy - r * 0.38f), borda);
    } else {
        if (aceso) { n = forma(lados, cx, cy + oy, r * (k + 0.2f), ang, vx, vy); poligono(vx, vy, n, halo); }
        n = forma(lados, cx + 1, cy + oy + 1, r * (k + 0.06f), ang, vx, vy); poligono(vx, vy, n, gfx_mistura(fundo, C_BARRA, 45));
        n = forma(lados, cx, cy + oy, r * (k + 0.08f), ang, vx, vy); poligono(vx, vy, n, borda);
        n = forma(lados, cx, cy + oy, r * k, ang, vx, vy);
        if (textura) ad_vinil(vx, vy, n, 0, a, fundo, cx, cy + oy, ang, r);
        else poligono(vx, vy, n, fundo);
    }
    if (r < 12) return;
    // O adesivo da loja em miniatura, no canto da face: diz qual e.
    int x = (int)(cx - r * 0.88f), y = (int)(cy - r * 0.88f);
    if (lados == 4) { x = (int)cx - 5; y = (int)(cy - r * 0.98f); }
    uint16_t f2 = vidro ? C_FELTRO_ESC : d->fundo;
    if (zerado) f2 = gfx_mistura(f2, C_FELTRO2, 60);
    gfx_rect(x + 1, y + 2, 11, 11, C_SOMBRA);
    ad_caixa(x, y, 11, 11, zerado ? C_FELTRO2 : C_BRANCO);
    ad_caixa(x + 1, y + 1, 9, 9, f2);
    ad_icone(x + 2, y + 2, a, 1, zerado ? gfx_mistura(d->tinta, C_FELTRO2, 60) : d->tinta);
}

// A Furia na mao: o bonus guardado num selo no alto, a direita (vermelho se
// ja ficou negativo). A Meia na Lareira tambem: o que ela guardou, em ouro.
static void furia_selo(float cx, float cy, float r, int b, bool ouro)
{
    char t[8];
    snprintf(t, sizeof t, "%+d", b);
    int w = gfx_largura(t, GFX_MIUDO) + 5, h = 9, x = (int)(cx + r * 0.40f), y = (int)(cy - r * 1.05f);
    gfx_rect(x + 1, y + 2, w, h, C_SOMBRA);
    gfx_rect(x, y, w, h, ouro ? C_OURO : b > 0 ? C_FOGO : C_VINHO_CLR);
    gfx_rect(x + 1, y + 1, w - 2, h - 2, C_BARRA);
    gfx_texto(x + 3, y - 2, t, ouro ? C_OURO : b > 0 ? C_AMARELO : C_VINHO_CLR, GFX_MIUDO, false);
}

// Um desenho de pontos (como sprite_em) em escala esc: cada ponto um bloco,
// e o contorno escuro de 1 pixel em volta.
static void sprite_esc(int cx, int cy, const char *const *l, int h, int esc, uint16_t c1, uint16_t c2, uint16_t c3)
{
    int w = (int)strlen(l[0]), x0 = cx - w * esc / 2, y0 = cy - h * esc / 2;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (l[j][i] != '.') gfx_rect(x0 + i * esc - 1, y0 + j * esc - 1, esc + 2, esc + 2, C_BARRA);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            char c = l[j][i];
            if (c != '.') gfx_rect(x0 + i * esc, y0 + j * esc, esc, esc, c == '+' ? c2 : c == 'o' ? c3 : c1);
        }
}
// O curinga da Ficha Coringa: o chapeu de bobo da corte, tres pontas com os
// guizos de ouro, as de fora vermelhas e a do meio de marfim.
static const char *const CHAPEU_CURINGA[] = { "+....+....+", "#....o....#", "##..ooo..##", ".##.ooo.##.",
                                              ".###ooo###.", "..##ooo##..", "..#ooooo#..", ".+++++++++.",
                                              ".+o+o+o+o+." };
static void curinga_face(float cx, float cy, float r, bool zerado)
{
    int esc = r >= 15 ? (int)(r / 9) : 1;
    if (esc < 1) esc = 1;
    if (zerado) sprite_esc((int)cx, (int)cy, CHAPEU_CURINGA, 9, esc, C_EBANO_B, C_CINZA, C_MARFIM_S);
    else sprite_esc((int)cx, (int)cy, CHAPEU_CURINGA, 9, esc, C_VERMELHO, C_OURO, C_MARFIM);
}

// O visor do Digital: algarismos de sete segmentos (a, b, c, d, e, f, g nos
// bits 0 a 6; 10 e o traco), os acesos em lima sobre o cristal escuro e os
// apagados quase sumindo.
static const uint8_t SEG7[11] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x40 };
static void seg7(int x, int y, int e, int k, int hl, uint8_t m, uint16_t aceso, uint16_t apagado)
{
    int w = 2 * e + hl, gy = e + k;
    const int S[7][4] = { { x + e, y, hl, e }, { x + w - e, y + e, e, k }, { x + w - e, y + gy + e, e, k },
                          { x + e, y + 2 * gy, hl, e }, { x, y + gy + e, e, k }, { x, y + e, e, k }, { x + e, y + gy, hl, e } };
    for (int i = 0; i < 7; i++)
        if ((m >> i & 1) || apagado) gfx_rect(S[i][0], S[i][1], S[i][2], S[i][3], (m >> i & 1) ? aceso : apagado);
}
// O que o visor mostra: o numero (v), "+1" (na mao: o que ele faz) ou os
// tracos (q < 0: ainda fora da sequencia). q de 0 a 1: acendendo - os
// segmentos piscam ao acaso, acendem todos ("88") e entao o numero, num
// clarao. zerado: anulado, o numero apagado.
static void visor_digital(float cx, float cy, float r, int v, bool mais_um, float q, bool zerado)
{
    int e = r >= 24 ? (int)(r / 12) : r >= 14 ? 2 : 1, k = (int)(r * 0.19f), hl = (int)(r * 0.24f);
    if (k < 1) k = 1;
    if (hl < 2) hl = 2;
    int w = 2 * e + hl, h = 3 * e + 2 * k, gap = e + 1;
    uint16_t apagado = RGB(36, 32, 29), aceso = zerado ? C_EBANO_B : C_LASER;
    uint8_t m[2] = { SEG7[10], SEG7[10] };       // os tracos
    int n = 2;
    if (mais_um) { m[0] = 0; m[1] = SEG7[1]; }
    else if (q >= 0) {
        int vv = v < 0 ? 0 : v > 99 ? 99 : v;
        if (vv < 10) { n = 1; m[0] = SEG7[vv]; }
        else { m[0] = SEG7[vv / 10]; m[1] = SEG7[vv % 10]; }
    }
    if (q >= 0 && q < 0.30f) {                   // ligando: pisca ao acaso
        uint32_t hs = hash2((int)(t_relogio * 24), (int)cx);
        m[0] = (uint8_t)(hs & 0x7F); m[1] = (uint8_t)(hs >> 7 & 0x7F); n = 2;
        aceso = C_VERDE;
    } else if (q >= 0.30f && q < 0.50f) { m[0] = m[1] = 0x7F; n = 2; aceso = C_LASER; }   // "88"
    else if (q >= 0.50f && q < 0.62f) aceso = C_BRANCO;                   // o clarao do numero
    if (q < 0 && (int)(t_relogio * 3) % 2) aceso = C_VERDE;              // os tracos piscando devagar
    int x0 = (int)cx - (n * w + (n - 1) * gap) / 2, y0 = (int)cy - h / 2 - 1;
    for (int i = 0; i < n; i++) seg7(x0 + i * (w + gap), y0, e, k, hl, m[i], aceso, apagado);
    if (mais_um) {                               // o "+" no lugar do primeiro algarismo
        int px = x0 + w / 2, py = y0 + h / 2;
        gfx_rect(px - hl / 2 - e + 1, py - e / 2, hl + 2 * e - 2, e, aceso);
        gfx_rect(px - e / 2, py - hl / 2 - e + 1, e, hl + 2 * e - 2, aceso);
    }
}
// O dado esta numa partida (o Relogio mostra a rodada; fora dela, gira).
static bool na_partida(void)
{
    return rodada >= 1 && (fase == F_ORDEM || fase == F_APOSTA || fase == F_ROLANDO || fase == F_ARRUMA || fase == F_RESULTADO
                           || fase == F_JOGA || fase == F_VEREDITO || fase == F_EFEITOS || fase == F_PREMIO || fase == F_LOJA);
}

static bool px_sem_sombra;               // o modo Pixel faz a sombra (pontilhada) por fora
static void desenha_dado_base(float cx, float cy, float r, int tipo, int dono,
                              float ang, int valor, int vista, bool zerado)
{
    brilho_desta = brilho_passa;                  // o brilho de passagem e so deste dado
    brilho_passa = -1;
    bool caixa = caixa_fechada && tipo == D_PRESENTE;
    caixa_fechada = false;
    bool curinga = tipo == D_FICHA_CORINGA && (face_curinga || vista < 2);   // na mao e no ar, sempre o curinga
    face_curinga = false;
    float dq = digital_q;
    digital_q = 1;
    if (tipo == D_RELOGIO)                        // as horas do mostrador: as que tirou; na mao, as da rodada
        relogio_desta = vista == 2 && valor >= 1 && valor != SO_CORPO ? valor
                      : na_partida() ? relogio_valor() : 1 + (int)(t_relogio * 1.2f) % 12;
    int adf = vista && valor != SO_CORPO && !face_escondida && !curinga && tipo != D_DIGITAL ? ad_face : 0;   // a etiqueta e so deste dado
    bool adf_aceso = ad_face_aceso;
    ad_face = 0;
    ad_face_aceso = false;
    int lados = TIPO[tipo].lados;
    if (tipo == D_SORTE_REVES && sr_lado == 2) lados = SR_REVES;   // o Reves e um d8
    const visual_t *vi = &VISUAL[tipo];
    // Os dados comuns no material de quem joga com eles; os especiais (e a
    // moeda) tem um desenho so, sobre o marfim, para os dois lados.
    bool comum = TIPO[tipo].efeito == EF_NADA && lados != 2 && tipo != D_MISTERIOSO;
    const material_t *mt = &MATERIAL[comum ? material_j[dono & 1] : MAT_MARFIM];
    mat_desta = comum && !zerado ? material_j[dono & 1] : -1;
    corpo_desta = tipo == D_PANETONE || tipo == D_BONECO || cosmico(tipo) ? tipo : 0;
    corpo_apagado = zerado;
    uint16_t corpo = mt->tons[2];
    uint16_t tinta = mt->tinta;
    uint16_t aro = aro_de(tipo);
    if (vi->tom) corpo = cor_de_dado(gfx_mistura(corpo, vi->tom, vi->pct));
    if (tipo == D_D2) corpo = cor_de_dado(gfx_mistura(corpo, C_OURO, 58));   // moeda dourada
    if (tipo == D_VIUVA) {                        // preta, com a tinta clara
        corpo = RGB(62, 56, 50);
        tinta = C_MARFIM;
    }
    if (tipo == D_MISTERIOSO) {                   // todo preto, a tinta de marfim
        corpo = RGB(24, 22, 22);
        tinta = C_MARFIM;
    }
    if (tipo == D_OCULTO) {                       // cinza-carvao: quase o preto do Misterioso
        corpo = RGB(50, 50, 52);
        tinta = C_MARFIM;
    }
    if (tipo == D_VICIADO) tinta = C_VERMELHO;   // pontos de cassino
    if (tipo == D_SACRIFICIO || tipo == D_FICHA_CORINGA) tinta = C_MARFIM;   // o corpo vinho (e o roxo da ficha) pede a tinta clara
    if (cosmico(tipo)) {                          // o ciclo cosmico: o ceu da noite (o Buraco, o breu; a Galaxia, roxo)
        corpo = tipo == D_GALAXIA ? RGB(54, 36, 74) : tipo == D_BURACO ? RGB(14, 14, 12) : RGB(26, 34, 62);
        tinta = tipo == D_ASTRO ? RGB(26, 34, 62) : C_MARFIM;   // na Estrela, o numero escuro sobre a luz
    }
    // O Sorte/Reves no ar: cada metade no material dela. Jogado, o dado
    // inteiro fica com a cara de quem venceu: marfim se a Sorte foi maior (ou
    // empatou), ebano se o Reves foi.
    const material_t *lado = NULL;
    if (tipo == D_SORTE_REVES && sr_lado) {
        lado = &MATERIAL[sr_lado == 1 ? MAT_MARFIM : MAT_EBANO];
        aro = sr_lado == 1 ? C_OURO : C_VINHO_CLR;
    } else if (tipo == D_SORTE_REVES && vista == 2 && valor != SO_CORPO) {
        lado = &MATERIAL[valor >= 0 ? MAT_MARFIM : MAT_EBANO];
    }
    if (lado) {
        corpo = lado->tons[2];
        tinta = lado->tinta;
    }
    if (zerado) {                        // anulado: o dado fica, apagado
        corpo = mt->apagado;
        tinta = mt->tinta_apagada;
        aro = C_FELTRO2;
    }
    // Os tons do desenho de dentro e os do corpo; anulado, o desenho fica num
    // cinza so e o corpo, sem luz.
    uint16_t cores[4] = { corpo, corpo, corpo, corpo }, tons[5] = { corpo, corpo, corpo, corpo, corpo };
    if (zerado) {
        cores[1] = cores[2] = cores[3] = cor_de_dado(gfx_mistura(corpo, tinta, 18));
        if (tipo == D_REVOLVER) {                // anulado, as balas do tambor continuam a vista
            cores[2] = RGB(150, 122, 62);
            cores[3] = cor_de_dado(gfx_mistura(corpo, tinta, 55));
        }
    } else {
        cores[1] = tom_desenho(corpo, vi->c1, vi->p1);
        cores[2] = tom_desenho(corpo, vi->c2, vi->p2);
        cores[3] = tom_desenho(corpo, vi->c3, vi->p3);
        if (comum) memcpy(tons, mt->tons, sizeof tons);
        else tons_do_corpo(corpo, tons);
        if (lado) {                              // o Sorte/Reves: liso, no material do lado
            memcpy(tons, lado->tons, sizeof tons);
            cores[1] = cores[2] = cores[3] = corpo;
        }
    }
    bool desenha = r >= 12;
    float ea = r < 12 ? 1 : r / 10;      // espessura do aro
    int so = r < 12 ? 2 : (int)(r / 6);  // deslocamento da sombra
    mca = cosf(ang); msa = sinf(ang); mcx = cx; mcy = cy;

    int alt = (int)sombra_alt;                    // dado no ar: sombra mais longe
    if (tipo == D_VAZIO) {                        // o Vazio: so o contorno; dentro, o que estiver atras
        float vx[12], vy[12], ea2 = r < 12 ? 1 : r / 10;
        int n = forma(lados, cx, cy, r, ang, vx, vy);
        uint16_t c = zerado ? C_FELTRO2 : aro;
        for (int i = 0; i < n; i++)
            gfx_linha_grossa((int)vx[i] + 1, (int)vy[i] + 2, (int)vx[(i + 1) % n] + 1, (int)vy[(i + 1) % n] + 2, (int)ea2 + 1, C_SOMBRA);
        for (int i = 0; i < n; i++)
            gfx_linha_grossa((int)vx[i], (int)vy[i], (int)vx[(i + 1) % n], (int)vy[(i + 1) % n], (int)ea2 + 1, c);
        if (adf) ad_etiqueta(cx, cy, r, lados, ang, adf, adf_aceso, zerado);
        uint16_t t0 = adf && ADESIVO[adf].fundo != AD_SEM_FUNDO ? ADESIVO[adf].tinta : gfx_mistura(c, C_BRANCO, 30);
        if (valor != SO_CORPO) numero(cx, cy, r, "0", zerado ? C_FELTRO2 : t0);
        if (zerado && vista) risca(cx, cy, r);
        return;
    }
    if (!px_sem_sombra) gfx_disco((int)cx + so + alt, (int)cy + so + 1 + alt, (int)(r - alt * 0.2f), C_SOMBRA);   // sombra no feltro
    if (tipo == D_FOGO || tipo == D_TUDO_NADA) {
        if (tipo == D_FOGO) ficha_de_fogo(cx, cy, r, ea, ang, valor, vista, zerado);
        else tudo_ou_nada(cx, cy, r, ang, vista == 0 ? -1 : valor, zerado);
        if (adf) {                                // as fichas: a etiqueta por cima, com o numero
            char s1[6];
            snprintf(s1, sizeof s1, "%d", valor);
            ad_etiqueta(cx, cy, r, 2, ang, adf, adf_aceso, zerado);
            numero(cx, cy, r, s1, ADESIVO[adf].fundo == AD_SEM_FUNDO ? C_MARFIM : ADESIVO[adf].tinta);
            if (zerado) risca(cx, cy, r);
        }
        return;
    }

    // O aro, o brilho e o corpo por cima do brilho, um pouco deslocado: fica
    // uma luz na aresta de cima.
    if (lado) mt = lado;
    brilho_apagado = zerado;
    uint16_t brilho = zerado ? corpo : mt->brilho ? mt->brilho : gfx_mistura(corpo, C_BRANCO, mt->brilho_pct);
    if (cosmico(tipo) && !zerado)                 // no ceu, a aresta acesa e um tom da propria noite
        brilho = tipo == D_GALAXIA || tipo == D_UNIVERSO ? RGB(102, 74, 138) : tipo == D_BURACO ? RGB(62, 56, 50) : RGB(56, 86, 150);
    // Tudo em volta do mesmo centro: o aro, o bisel (a aresta de dentro, clara
    // do lado da luz) e o corpo. O desenho de dentro guarda a sua referencia
    // (arte_du, arte_dv: onde o corpo caia nas coordenadas dele).
    float bcy = forma_cy(lados, cy, r);
    uint16_t sombra_bisel = zerado ? corpo : tons[1];
    if (lados == 2 || lados == 100) {             // a moeda e a esfera do Universo
        int ri = (int)(r - ea);
        if (!zerado) aro_rampa(NULL, NULL, 0, cx, cy, (int)(r + ea), aro);   // o aro em rampa
        else gfx_disco((int)cx, (int)cy, (int)(r + ea), aro);
        poligono_bisel(NULL, NULL, 0, cx, cy, ri, brilho, sombra_bisel);
        arte_du = arte_dv = 1;
        if (desenha) pinta_corpo(NULL, NULL, 0, cx, cy, ri - 1, tipo, tons, cores, cx, cy, ang, r);
        else gfx_disco((int)cx, (int)cy, ri - 1, corpo);
        mcx = cx - (mca - msa); mcy = cy - (msa + mca);
    } else {
        float vx[12], vy[12], dx = ea * 0.8f, d = ea * 0.85f;
        int n = forma_d(lados, cx, cy, r, ang, d, vx, vy);
        if (!zerado) aro_rampa(vx, vy, n, cx, bcy, 0, aro);   // o aro em rampa
        else poligono(vx, vy, n, aro);
        n = forma_d(lados, cx, cy, r, ang, -d, vx, vy);
        poligono_bisel(vx, vy, n, cx, bcy, 0, brilho, sombra_bisel);
        arte_du = dx;                             // o corpo antigo: dx abaixo e a direita (e o d4, mais abaixo)
        arte_dv = dx + (lados == 4 ? (r - ea * 1.6f) * 0.2f : 0);
        if (r >= 12) {
            n = forma_d(lados, cx, cy, r, ang, -d - 1, vx, vy);
            if (desenha) pinta_corpo(vx, vy, n, 0, 0, 0, tipo, tons, cores, cx, bcy, ang, r);
            else poligono(vx, vy, n, corpo);
        }
        // as pecas por cima (marca_por_cima) ficam presas ao corpo como o desenho
        mcx = cx - (arte_du * mca - arte_dv * msa);
        mcy = bcy - (arte_du * msa + arte_dv * mca);
    }
    arte_du = arte_dv = 0;
    marca_por_cima(tipo, r, corpo, zerado);
    mcx = cx; mcy = cy;
    float cy0 = cy;                               // a etiqueta do adesivo se mede do centro do dado
    cy = bcy;                                     // daqui para baixo, o numero e o que vai em cima dele: no centro da forma

    char s[12];
    // Vidro que tirou 1: estilhacado, com rachaduras saindo do impacto.
    if (tipo == D_VIDRO && vista == 2 && valor == 1 && zerado && !face_capa) {
        int e = r < 12 ? 1 : (int)(r / 11);
        static const float RA[6][2] = { {-0.9f,-0.3f}, {-0.2f,-0.95f}, {0.7f,-0.6f}, {0.95f,0.25f}, {0.2f,0.9f}, {-0.6f,0.7f} };
        float ix = cx + r * 0.15f, iy = cy - r * 0.1f;
        for (int i = 0; i < 6; i++) {
            float mx = ix + RA[i][0] * r * 0.5f + (i % 2 ? 2 : -2), my = iy + RA[i][1] * r * 0.5f;
            gfx_linha_grossa((int)ix, (int)iy, (int)mx, (int)my, e, C_VIDRO);
            gfx_linha_grossa((int)mx, (int)my, (int)(ix + RA[i][0] * r), (int)(iy + RA[i][1] * r), e, C_VIDRO);
        }
        numero(cx, cy, r, "1", tinta);
        return;
    }
    // O ciclo cosmico: o numero sobre o ceu desenhado ganha o contorno do
    // corpo (na Estrela, nao: o numero escuro fica sobre a luz).
    uint16_t contorno = cosmico(tipo) && tipo != D_ASTRO && !zerado && r >= 12 ? corpo : 0;
    if (tipo == D_FICHA_CORINGA && !zerado && r >= 12) contorno = RGB(54, 36, 74);
    if (curinga && valor != SO_CORPO) {          // a Ficha Coringa: o curinga, ate virar
        curinga_face(cx, cy, r, zerado);
        if (zerado && vista) risca(cx, cy, r);
        return;
    }
    if (tipo == D_DIGITAL && valor != SO_CORPO) {   // o visor: "+1" na mao, os tracos no ar, o numero na sequencia
        visor_digital(cx, cy, r, valor, vista == 0, vista == 1 ? -1 : dq, zerado);
        if (zerado && vista) risca(cx, cy, r);
        return;
    }
    if (vista == 0) {
        // Parado na mao mostra quantos lados tem - ou, no Quebrado, o numero
        // gravado nele, que e o que ele vai valer.
        if ((tipo == D_QUEBRADO || tipo == D_FANTASMA) && valor) {
            snprintf(s, sizeof s, "%d", (int8_t)valor);
            numero(cx, cy, r, s, tinta);
            return;
        }
        if (tipo == D_MISTERIOSO || caixa) { numero(cx, cy, r, "?", tinta); return; }
        if (tipo == D_PREVISIVEL) {               // o proximo da escada: o que ele vai tirar
            snprintf(s, sizeof s, "%d", valor + 1 < 12 ? valor + 1 : 12);
            numero(cx, cy, r, s, tinta);
            return;
        }
        if (tipo == D_VAZIO) { numero(cx, cy, r, "0", tinta); return; }
        if (tipo == D_RELOGIO && na_partida()) {  // o numero que ele vai tirar: o da rodada
            snprintf(s, sizeof s, "%d", relogio_valor());
            numero(cx, cy, r, s, tinta);
            return;
        }
        // A tinta do dado a meio caminho do corpo: le-se bem, mas nao se
        // confunde com um valor tirado, que vem na tinta cheia.
        snprintf(s, sizeof s, "%d", lados);
        numero_contorno(cx, cy, r, s, gfx_mistura(tinta, corpo, 38), contorno);
        if (tipo == D_FURIA && marca_nivel >= 0 && (int8_t)marca_nivel && !zerado && r >= 12)
            furia_selo(cx, cy, r, (int8_t)marca_nivel, false);
        if (tipo == D_MEIA && marca_nivel > 0 && !zerado && r >= 12) furia_selo(cx, cy, r, marca_nivel, true);
        return;
    }
    if (valor == SO_CORPO) return;       // so o corpo: o catalogo escreve grande
    if (face_escondida) {                // o Oculto: ninguem ve o que saiu
        numero(cx, cy, r, "?", tinta);
        if (zerado) risca(cx, cy, r);
        return;
    }
    if (adf) {                           // o adesivo do lado de cima: a etiqueta, e o numero na tinta dela
        ad_etiqueta(cx, cy0, r, lados, ang, adf, adf_aceso, zerado);
        if (ADESIVO[adf].fundo != AD_SEM_FUNDO) {
            tinta = zerado ? gfx_mistura(ADESIVO[adf].tinta, C_FELTRO2, 60) : ADESIVO[adf].tinta;
            // o numero sobre o simbolo ganha um contorno da cor do vinil
            if (r >= 12) contorno = zerado ? gfx_mistura(ADESIVO[adf].fundo, C_FELTRO2, 60) : ADESIVO[adf].fundo;
        }
    }
    if (lados == 6 && TIPO[tipo].efeito != EF_ESPELHO && TIPO[tipo].efeito != EF_REVOLVER && TIPO[tipo].efeito != EF_ASTRO
        && tipo != D_ADESIVADO
        && valor >= 1 && valor <= 6 && (!zerado || so_apagado)) {
        uint16_t luz = cor_de_dado(gfx_mistura(tinta, corpo, 45));   // o furo com relevo
        pips_luz((int)cx, (int)cy, valor, (int)r, tinta, luz == tinta ? 0 : luz);
    } else {
        snprintf(s, sizeof s, sr_lado ? (sr_lado == 1 ? "+%d" : "-%d") : "%d", valor);
        numero_contorno(cx, cy, r, s, tinta, contorno);
    }
    if (zerado) risca(cx, cy, r);
}

// ---- O dado num rascunho, colado com o contorno -------------------------
// O dado (aro, corpo, numero) e desenhado num rascunho e colado no destino
// de agora (a tela, ou o rascunho da moeda girando) com um contorno de 1 px
// em C_BARRA por fora da silhueta (vizinhos em cruz). A sombra vem por baixo,
// com o miolo cheio e a beira pontilhada.
#define CHAVE 0xF81F                                   // magenta: fundo do rascunho
static uint16_t px_buf[GFX_W * GFX_H];
static int px_borda[GFX_W * GFX_H / 4];
static bool px_dentro;

static void sombra_px(uint16_t *buf, int cx, int cy, int R)
{
    if (R < 1) return;
    float b = R >= 6 ? 2.5f : 1.5f;                     // a largura da beira pontilhada
    for (int y = cy - R - 1; y <= cy + R + 1; y++)
        for (int x = cx - R - 1; x <= cx + R + 1; x++) {
            if (x < 0 || y < 0 || x >= GFX_W || y >= GFX_H) continue;
            float d = sqrtf((float)((x - cx) * (x - cx) + (y - cy) * (y - cy)));
            float q = (R + 0.5f - d) / b;               // 1 no miolo, 0 fora
            if (q <= 0 || (q < 1 && bayer(x, y) >= q)) continue;
            buf[y * GFX_W + x] = C_SOMBRA;
        }
}

static void desenha_dado(float cx, float cy, float r, int tipo, int dono,
                         float ang, int valor, int vista, bool zerado)
{
    if (px_dentro) { desenha_dado_base(cx, cy, r, tipo, dono, ang, valor, vista, zerado); return; }
    uint16_t *alvo = gfx_alvo(), *fb = alvo ? alvo : display_fb();
    int so = r < 12 ? 2 : (int)(r / 6), alt = (int)sombra_alt;
    bool sombra = tipo != D_VAZIO;                      // o Vazio faz a dele (as linhas)
    int m = (int)(r * 1.7f) + 8;
    int x0 = (int)cx - m, y0 = (int)cy - m, x1 = (int)cx + m, y1 = (int)cy + m;
    if (x0 < 1) x0 = 1;
    if (y0 < 1) y0 = 1;
    if (x1 > GFX_W - 2) x1 = GFX_W - 2;
    if (y1 > GFX_H - 2) y1 = GFX_H - 2;
    if (x0 > x1 || y0 > y1) return;
    for (int y = y0 - 1; y <= y1 + 1; y++)
        for (int x = x0 - 1; x <= x1 + 1; x++) px_buf[y * GFX_W + x] = CHAVE;
    gfx_desvia(px_buf);
    px_dentro = true;
    px_sem_sombra = sombra;
    desenha_dado_base(cx, cy, r, tipo, dono, ang, valor, vista, zerado);
    px_dentro = px_sem_sombra = false;
    gfx_desvia(alvo);
    // O contorno: o fundo que encosta na silhueta, em cruz.
    int nb = 0, max_b = (int)(sizeof px_borda / sizeof px_borda[0]);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            int i = y * GFX_W + x;
            if (px_buf[i] != CHAVE) continue;
            if ((px_buf[i - 1] != CHAVE || px_buf[i + 1] != CHAVE || px_buf[i - GFX_W] != CHAVE || px_buf[i + GFX_W] != CHAVE)
                && nb < max_b) px_borda[nb++] = i;
        }
    for (int k = 0; k < nb; k++) px_buf[px_borda[k]] = C_BARRA;
    // A sombra: a propria silhueta (com o contorno) deslocada para baixo e a
    // direita, onde o dado nao cobre; o miolo cheio e a beira (o que veio do
    // contorno) pontilhada. Um disco ficava sobrando em volta do D4.
    if (sombra) {
        int dx = so + alt, dy = so + 1 + alt;
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                uint16_t c = px_buf[y * GFX_W + x];
                int X = x + dx, Y = y + dy;
                if (c == CHAVE || X > GFX_W - 1 || Y > GFX_H - 1) continue;
                if (X <= x1 && Y <= y1 && px_buf[Y * GFX_W + X] != CHAVE) continue;   // o dado cobre
                if (c == C_BARRA && bayer(X, Y) >= 0.5f) continue;                     // a beira, a meio tom
                fb[Y * GFX_W + X] = C_SOMBRA;
            }
    }
    for (int y = y0; y <= y1; y++)                      // cola o que nao e fundo
        for (int x = x0; x <= x1; x++) {
            uint16_t c = px_buf[y * GFX_W + x];
            if (c != CHAVE) fb[y * GFX_W + x] = c;
        }
}

// Moeda girando no ar: a arte de verdade da ficha (desenhada num rascunho)
// colada achatada na horizontal, pela largura aparente |escx|. Quando ela
// passa de lado, aparece a espessura.
static uint16_t rascunho[GFX_W * GFX_H];

static void moeda_girando_arte(float cx, float cy, float r, int tipo, int dono, float escx, int face)
{
    int caixa = (int)(r * 1.5f) + 4, x0 = (int)cx - caixa, y0 = (int)cy - caixa;
    for (int y = y0; y <= y0 + 2 * caixa; y++)
        for (int x = x0; x <= x0 + 2 * caixa; x++)
            if (x >= 0 && y >= 0 && x < GFX_W && y < GFX_H) rascunho[y * GFX_W + x] = CHAVE;
    gfx_desvia(rascunho);                              // o numero vai junto, achatado
    desenha_dado(cx, cy, r, tipo, dono, 0, face, 2, false);
    gfx_desvia(NULL);
    float e = fabsf(escx);
    if (e < 0.06f) e = 0.06f;
    uint16_t *fb = display_fb();
    for (int y = y0; y <= y0 + 2 * caixa; y++) {
        if (y < 0 || y >= GFX_H) continue;
        for (int x = (int)(cx - caixa * e); x <= (int)(cx + caixa * e); x++) {
            if (x < 0 || x >= GFX_W) continue;
            int sx = (int)lroundf(cx + (x - cx) / e);
            if (sx < x0 || sx > x0 + 2 * caixa || sx < 0 || sx >= GFX_W) continue;
            uint16_t c = rascunho[y * GFX_W + sx];
            if (c != CHAVE) fb[y * GFX_W + x] = c;
        }
    }
    if (fabsf(escx) < 0.35f)                           // a borda da moeda, de lado
        gfx_rect((int)cx - 1, (int)(cy - r), 3, (int)(2 * r), gfx_mistura(aro_de(tipo), C_SOMBRA, 30));
}

// O dado deitou no feltro: o pulinho do pouso, o relogio dele (o numero) e
// o anel de poeira no lugar (mesa.inc).
static void poeira_solta(float x, float y);
static void assenta(voo_t *v)
{
    v->pouso = 0.18f;
    v->t_assenta = 0.001f;
    if (!v->fantasma) poeira_solta(v->x, v->y);
}

// som: SOM_BATE (na borda de madeira) ou SOM_BATE_DADO (num outro dado).
static void estala(float forca, int som)
{
    if (forca > 100 && t_fase - t_som > 0.03f) { som_toca(som); t_som = t_fase; }
}

// Um passo de fisica para todos os dados no ar. Devolve true quando todos
// pararam. Serve ao lancamento e a abertura.
static bool fisica(float dt)
{
    bool todos = true;
    for (int i = 0; i < n_voo; i++) {
        voo_t *v = &voo[i];
        if (v->pouso > 0) v->pouso -= dt;
        if (v->parado) continue;
        todos = false;
        v->x += v->vx * dt;
        v->y += v->vy * dt;
        v->giro += v->spin * dt;
        float atrito = 1.0f - 2.4f * dt;
        v->vx *= atrito; v->vy *= atrito; v->spin *= atrito;

        const float r = R_DADO + 2;
        bool moeda = TIPO[v->tipo].lados == 2;     // moeda tem os sons dela, mais leves
        int borda = moeda ? SOM_MOEDA_BATE : SOM_BATE;
        if (v->x < MESA_X0 + r) { v->x = MESA_X0 + r; estala(fabsf(v->vx), borda); v->vx = -v->vx * 0.62f; v->spin = -v->spin; }
        if (v->x > MESA_X1 - r) { v->x = MESA_X1 - r; estala(fabsf(v->vx), borda); v->vx = -v->vx * 0.62f; v->spin = -v->spin; }
        if (v->y < MESA_Y0 + r) { v->y = MESA_Y0 + r; estala(fabsf(v->vy), borda); v->vy = -v->vy * 0.62f; }
        if (v->y > MESA_Y1 - r) { v->y = MESA_Y1 - r; estala(fabsf(v->vy), borda); v->vy = -v->vy * 0.62f; }

        float vel = sqrtf(v->vx * v->vx + v->vy * v->vy);
        v->t_face -= dt;
        // A hora da verdade: quase parado, o dado hesita na quina, balancando,
        // e a face troca cada vez mais devagar antes de deitar.
        if ((vel < 36 || t_fase > 6.0f) && v->hesita > 0) {
            if (v->hesita >= HESITA_T) v->giro_h = v->giro;
            v->hesita -= dt;
            float q = fmaxf(0, v->hesita / HESITA_T);
            v->vx = v->vy = v->spin = 0;
            v->giro = v->giro_h + 0.2f * q * sinf((1 - q) * 22);
            if (v->t_face <= 0) {
                v->face = face_v(v->tipo, v->valor);
                v->t_face = 0.025f + (1 - q) * 0.08f;
            }
            if (v->hesita > 0) continue;
            v->hesita = -1;
        }
        if (v->t_face <= 0) {
            v->face = face_v(v->tipo, v->valor);
            v->t_face = 0.04f + (1.0f - vel / 680.0f) * 0.12f;
        }
        if (vel < 36 || t_fase > 6.0f) {
            v->parado = true;
            assenta(v);
            v->face = v->valor;
            som_toca(moeda ? SOM_MOEDA_POUSA : SOM_POUSA);
        }
    }
    // Dados se empurram quando se encontram.
    for (int a = 0; a < n_voo; a++)
        for (int b = a + 1; b < n_voo; b++) {
            voo_t *p = &voo[a], *q = &voo[b];
            float dx = q->x - p->x, dy = q->y - p->y;
            float d = sqrtf(dx * dx + dy * dy);
            const float D = 2 * R_DADO + 4;
            if (d >= D || d < 0.01f) continue;
            float nx = dx / d, ny = dy / d, meio = (D - d) / 2;
            p->x -= nx * meio; p->y -= ny * meio;
            q->x += nx * meio; q->y += ny * meio;
            float rv = (q->vx - p->vx) * nx + (q->vy - p->vy) * ny;
            if (rv < 0) {
                float imp = -rv * 0.8f;
                estala(imp, TIPO[p->tipo].lados == 2 || TIPO[q->tipo].lados == 2 ? SOM_MOEDA_BATE : SOM_BATE_DADO);
                p->vx -= imp * nx; p->vy -= imp * ny;
                q->vx += imp * nx; q->vy += imp * ny;
                if (p->parado || q->parado) { p->parado = q->parado = false; }
            }
        }
    return todos;
}

// A mesa: trilho de madeira com filete de latao e o feltro, mais escuro nas
// bordas. Nao muda nunca: e pintada uma vez e copiada a cada quadro.
static uint16_t mesa_pronta[GFX_W * GFX_H];
static bool mesa_feita;

static uint32_t ruido_xy(int x, int y)
{
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Os andares do cassino: no Desafiante, cada dificuldade e um andar mais
// chique, e a mesa mostra isso nos acabamentos (0 = terreo, a mesa de sempre).
static int mesa_andar, mesa_andar_feito = -1;
#define C_RUBI     RGB(190, 56, 60)
#define C_DIAMANTE RGB(168, 204, 230)

// Tacha de metal: um botao com brilho em cima e sombra embaixo.
static void tacha(int x, int y, uint16_t m)
{
    gfx_disco(x + 1, y + 1, 2, C_SOMBRA);
    gfx_disco(x, y, 2, m);
    gfx_pixel(x - 1, y - 1, gfx_mistura(m, C_BRANCO, 55));
    gfx_pixel(x + 1, y + 1, gfx_mistura(m, C_SOMBRA, 45));
}

// Pedra lapidada em losango, com aro de ouro e um ponto de luz.
static void pedra(int x, int y, uint16_t g)
{
    for (int d = -4; d <= 4; d++) {
        int w = 4 - (d < 0 ? -d : d);
        gfx_rect(x - w, y + d, 2 * w + 1, 1, C_OURO);
    }
    for (int d = -3; d <= 3; d++) {
        int w = 3 - (d < 0 ? -d : d);
        gfx_rect(x - w, y + d, 2 * w + 1, 1, d < 0 ? gfx_mistura(g, C_BRANCO, 25) : g);
    }
    gfx_pixel(x - 1, y - 2, C_BRANCO);
}

// Filete fino no feltro, a `in` pixels da borda, com os cantos recortados
// para dentro (um quarto de circulo) nos andares de ouro em diante.
static void filete(int in, uint16_t c, bool recorte)
{
    int a = MESA_X0 + in, b = MESA_X1 - 1 - in, t = MESA_Y0 + in, u = MESA_Y1 - 1 - in;
    int r = recorte ? 7 : 0;
    gfx_rect(a + r, t, b - a - 2 * r + 1, 1, c);
    gfx_rect(a + r, u, b - a - 2 * r + 1, 1, c);
    gfx_rect(a, t + r, 1, u - t - 2 * r + 1, c);
    gfx_rect(b, t + r, 1, u - t - 2 * r + 1, c);
    if (!recorte) return;
    for (int i = 0; i <= 16; i++) {                     // quarto de circulo para dentro
        float q = i * 0.09817f;                         // 0 a 90 graus
        int dx = (int)(cosf(q) * r + 0.5f), dy = (int)(sinf(q) * r + 0.5f);
        gfx_pixel(a + dx, t + dy, c);
        gfx_pixel(b - dx, t + dy, c);
        gfx_pixel(a + dx, u - dy, c);
        gfx_pixel(b - dx, u - dy, c);
    }
}

// ---------------------------------------------------------------------------
// Os enfeites dos estilos novos. O que mexe (estrelas que cintilam, o
// pisca-pisca, o led que respira) troca de quadro no ritmo dos retratos e e
// pintado direto na mesa pronta: o fundo de cada coisa que mexe fica
// guardado para voltar no quadro seguinte.
// ---------------------------------------------------------------------------
#define ANIMA_PASSO 0.24f                // um quadro, como os retratos
#define ANIMA_MAX   128
#define ANIMA_POOL  12000
typedef struct { int16_t x, y; uint8_t w, h, tipo, cor; uint16_t de; } anima_t;
enum { AN_ESTRELA, AN_LAMPADA, AN_REFLEXO, AN_FLOCO, AN_FOGOS };
static anima_t anima[ANIMA_MAX];
static int n_anima, anima_usado;
static uint16_t anima_fundo[ANIMA_POOL];

// Um enfeite que mexe, na caixa (x, y, w, h): so mesa_anima o desenha.
static void anima_poe(int x, int y, int w, int h, int tipo, int cor)
{
    if (n_anima >= ANIMA_MAX || anima_usado + w * h > ANIMA_POOL) return;
    anima[n_anima++] = (anima_t){ (int16_t)x, (int16_t)y, (uint8_t)w, (uint8_t)h, (uint8_t)tipo, (uint8_t)cor,
                                  (uint16_t)anima_usado };
    anima_usado += w * h;
}
// Com a mesa inteira pintada: o fundo de cada caixa, para voltar a cada quadro.
static void anima_guarda(void)
{
    const uint16_t *fb = display_fb();
    for (int k = 0; k < n_anima; k++) {
        const anima_t *a = &anima[k];
        for (int j = 0; j < a->h; j++)
            for (int i = 0; i < a->w; i++) {
                int xx = a->x + i, yy = a->y + j;
                anima_fundo[a->de + j * a->w + i] = xx >= 0 && xx < GFX_W && yy >= 0 && yy < GFX_H ? fb[yy * GFX_W + xx] : 0;
            }
    }
}
static void anima_volta(const anima_t *a, uint16_t *fb)
{
    for (int j = 0; j < a->h; j++)
        for (int i = 0; i < a->w; i++) {
            int xx = a->x + i, yy = a->y + j;
            if (xx >= 0 && xx < GFX_W && yy >= 0 && yy < GFX_H) fb[yy * GFX_W + xx] = anima_fundo[a->de + j * a->w + i];
        }
}

// Uma estrela de quatro pontas: o miolo e os bracos (b de comprimento), do
// claro para o fraco.
static void estrela4(int x, int y, int b, uint16_t miolo, uint16_t braco, uint16_t ponta)
{
    gfx_pixel(x, y, miolo);
    for (int k = 1; k <= b; k++) {
        uint16_t c = k == b && b > 1 ? ponta : braco;
        gfx_pixel(x - k, y, c); gfx_pixel(x + k, y, c);
        gfx_pixel(x, y - k, c); gfx_pixel(x, y + k, c);
    }
}

// O ceu de Galaxias: a Via Lactea em nevoa, atravessando a mesa torta, e as
// estrelas - muitas fracas, poucas claras, umas mais quentes, outras mais
// frias, mais juntas dentro da faixa. Algumas grandes cintilam.
static void pinta_galaxia(void)
{
    static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
    float cx = (MESA_X0 + MESA_X1) / 2.0f, cy = (MESA_Y0 + MESA_Y1) / 2.0f;
    for (int y = MESA_Y0; y < MESA_Y1; y++)
        for (int x = MESA_X0; x < MESA_X1; x++) {
            float t = (x - cx) / 300.0f;
            float meio = cy - 0.30f * (x - cx) + sinf(t * 2.6f + 0.6f) * 16;
            float d = fabsf(y - meio) / (40 + 14 * sinf(t * 3.7f + 1.1f));
            float n = 0.55f * ruido(x / 41.0f, y / 19.0f) + 0.30f * ruido(x / 13.0f + 7, y / 7.0f);
            float dens = 1.0f - d + n * 0.7f;
            if (dens <= 0.05f) continue;
            float v = dens * 2.2f;
            int i = (int)v;
            if ((v - i) * 16 > BAYER[y & 3][x & 3]) i++;
            if (i <= 0) continue;
            uint16_t c;
            if (i == 1) c = GAL_NEVOA[0];
            else if (i == 2) c = ruido(x / 57.0f + 3, y / 31.0f) > 0.05f ? GAL_NEVOA[1] : GAL_NEVOA[2];
            else c = GAL_NEVOA[3];
            gfx_pixel(x, y, c);
        }
    // As estrelas: a chance sobe dentro da faixa.
    for (int y = MESA_Y0 + 2; y < MESA_Y1 - 2; y++)
        for (int x = MESA_X0 + 2; x < MESA_X1 - 2; x++) {
            float t = (x - cx) / 300.0f;
            float meio = cy - 0.30f * (x - cx) + sinf(t * 2.6f + 0.6f) * 16;
            bool faixa = fabsf(y - meio) < 34;
            uint32_t h = ruido_xy(x * 7 + 3, y * 13 + 5);
            uint32_t q = h % (faixa ? 66u : 160u);
            if (q) continue;
            uint32_t k = (h >> 8) % 100;
            if (k < 62) gfx_pixel(x, y, GAL_ESTRELA[0]);
            else if (k < 84) gfx_pixel(x, y, GAL_ESTRELA[1]);
            else if (k < 91) gfx_pixel(x, y, GAL_ESTRELA[2]);
            else if (k < 96) gfx_pixel(x, y, GAL_ESTRELA[3]);
            else {                                       // a clara, com uma cruz miuda e apagada
                gfx_pixel(x, y, GAL_ESTRELA[4]);
                gfx_pixel(x - 1, y, GAL_ESTRELA[0]); gfx_pixel(x + 1, y, GAL_ESTRELA[0]);
                gfx_pixel(x, y - 1, GAL_ESTRELA[0]); gfx_pixel(x, y + 1, GAL_ESTRELA[0]);
            }
        }
    // As grandes, que cintilam: espalhadas, longe da borda.
    for (int i = 0; i < 11; i++) {
        uint32_t h = ruido_xy(i * 97 + 11, i * 31 + 7);
        int x = MESA_X0 + 18 + (int)(h % (uint32_t)(MESA_X1 - MESA_X0 - 36));
        int y = MESA_Y0 + 14 + (int)((h >> 12) % (uint32_t)(MESA_Y1 - MESA_Y0 - 28));
        anima_poe(x - 3, y - 3, 7, 7, AN_ESTRELA, (uint8_t)(h >> 24) & 1);
    }
}

// Uma lampada do pisca-pisca: o soquete e o vidro (2 x 3); acesa, com um
// brilho em volta e um ponto de luz.
static void lampada(int x, int y, int c, bool acesa)
{
    if (acesa) {
        gfx_pixel(x - 1, y, NATAL_BRILHO[c]); gfx_pixel(x + 2, y, NATAL_BRILHO[c]);
        gfx_pixel(x - 1, y + 1, NATAL_BRILHO[c]); gfx_pixel(x + 2, y + 1, NATAL_BRILHO[c]);
        gfx_pixel(x, y + 2, NATAL_BRILHO[c]); gfx_pixel(x + 1, y + 2, NATAL_BRILHO[c]);
    }
    gfx_rect(x, y - 2, 2, 1, NATAL_FIO);
    gfx_rect(x, y - 1, 2, 3, acesa ? NATAL_ACESA[c] : NATAL_APAGADA[c]);
    if (acesa) gfx_pixel(x, y - 1, C_BRANCO);
}

// Uma folha de azevinho de comprimento n, a partir de (x, y), deitada (dx)
// ou em pe (dy): a nervura escura no meio, a metade de cima mais clara e as
// pontas da borda saindo de um lado e do outro.
static void folha_azevinho(int x, int y, int dx, int dy, int n)
{
    for (int i = 0; i < n; i++) {
        float q = (i + 0.5f) / n;
        int w = (int)lroundf(sinf(q * 3.1416f) * 2.4f);
        int pico = i > 1 && i < n - 1 && i % 3 == 1 ? ((i / 3) & 1 ? 1 : -1) : 0;   // as pontas
        for (int k = -w - (pico < 0); k <= w + (pico > 0); k++) {
            int px = x + dx * i + dy * k, py = y + dy * i + dx * k;
            gfx_pixel(px, py, k == 0 ? NATAL_FOLHA[0] : k < 0 ? NATAL_FOLHA[2] : NATAL_FOLHA[1]);
        }
    }
}

// Uma bolinha de azevinho (4 x 4), com o brilho no alto.
static void bolinha(int x, int y)
{
    gfx_rect(x + 1, y, 2, 4, C_VERMELHO);
    gfx_rect(x, y + 1, 4, 2, C_VERMELHO);
    gfx_pixel(x + 3, y + 2, NATAL_FITA[1]); gfx_pixel(x + 2, y + 3, NATAL_FITA[1]);
    gfx_pixel(x + 1, y + 1, C_ROSA);
}

// O raminho do canto (x, y) da moldura: uma folha correndo na borda de cima
// e outra na do lado, para dentro (sx, sy = +-1), e tres bolinhas no canto.
static void azevinho(int x, int y, int sx, int sy)
{
    folha_azevinho(x + 3 * sx, y + 1 * sy, sx, 0, 12);
    folha_azevinho(x + 1 * sx, y + 3 * sy, 0, sy, 12);
    bolinha(x + (sx > 0 ? 1 : -4), y + (sy > 0 ? 1 : -4));
    bolinha(x + (sx > 0 ? 5 : -8), y + (sy > 0 ? 2 : -5));
    bolinha(x + (sx > 0 ? 2 : -5), y + (sy > 0 ? 5 : -8));
}

// Um floco de neve: seis bracos curtos, a ponta mais apagada.
static void floco(int x, int y, uint16_t c)
{
    gfx_pixel(x, y, c);
    for (int k = 1; k <= 2; k++) {
        uint16_t b = k == 2 ? NATAL_NEVE[1] : c;
        gfx_pixel(x - k, y, b); gfx_pixel(x + k, y, b);
        gfx_pixel(x, y - k, b); gfx_pixel(x, y + k, b);
    }
    gfx_pixel(x - 1, y - 1, NATAL_NEVE[1]); gfx_pixel(x + 1, y + 1, NATAL_NEVE[1]);
    gfx_pixel(x + 1, y - 1, NATAL_NEVE[1]); gfx_pixel(x - 1, y + 1, NATAL_NEVE[1]);
}

// O Natal: uma fita de veludo vinho com filete de ouro no lugar da moldura,
// com o pisca-pisca correndo nela, e azevinho nos cantos (na mesa Led, a fita
// de led fica e o Natal so enfeita o feltro); no feltro, neve amontoada nos
// cantos de baixo e uns flocos, longe do meio.
static void natal_fita(int a, int b, int t, int u)
{
    for (int k = 0; k < 5; k++) {
        bool ouro = k == 0 || k == 4;
        uint16_t claro = ouro ? (k == 0 ? C_LATAO : C_LATAO_ESC) : k == 1 ? NATAL_FITA[2] : k == 2 ? NATAL_FITA[1] : NATAL_FITA[0];
        uint16_t escuro = ouro ? (k == 0 ? C_LATAO_ESC : C_LATAO) : k == 1 ? NATAL_FITA[0] : NATAL_FITA[1];
        gfx_rect(a + k, t + k, b - a - 2 * k + 1, 1, claro);      // em cima e a esquerda, a luz
        gfx_rect(a + k, t + k, 1, u - t - 2 * k + 1, claro);
        gfx_rect(a + k, u - k, b - a - 2 * k + 1, 1, escuro);     // embaixo e a direita, a sombra
        gfx_rect(b - k, t + k, 1, u - t - 2 * k + 1, escuro);
    }
}

// Um monte de neve no canto (cx, cy), crescendo para dentro (dx, dy = +-1).
static void monte_neve(int cx, int cy, int dx, int dy)
{
    for (int j = 0; j < 12; j++)
        for (int i = 0; i < 34; i++) {
            float q = i / 34.0f, alto = 10 * (1 - q * q) + 1.2f * sinf(i * 0.7f);
            if (j > alto) continue;
            uint16_t c = j > alto - 1.2f || j > 2 ? NATAL_NEVE[0] : NATAL_NEVE[1];   // a base na sombra
            if (j > 2 && j < alto - 2 && (i * 3 + j * 5) % 17 == 0) c = NATAL_NEVE[1];
            gfx_pixel(cx + dx * i, cy + dy * j, c);
        }
}

static void pinta_natal(void)
{
    float cx = (MESA_X0 + MESA_X1) / 2.0f, cy = (MESA_Y0 + MESA_Y1) / 2.0f;
    for (int y = MESA_Y0 + 10; y < MESA_Y1 - 10; y++)               // a neve caindo, longe do meio
        for (int x = MESA_X0 + 10; x < MESA_X1 - 10; x++) {
            uint32_t h = ruido_xy(x * 3 + 1, y * 5 + 9);
            float dx = (x - cx) / 300, dy = (y - cy) / 128, d = dx * dx + dy * dy;
            if (d < 0.34f) continue;
            if (h % 1100 == 0) gfx_pixel(x, y, NATAL_NEVE[1]);
            else if (h % 3300 == 1) { gfx_rect(x, y, 2, 2, NATAL_NEVE[1]); gfx_pixel(x, y, NATAL_NEVE[0]); }
            else if (h % 12000 == 2) floco(x, y, NATAL_NEVE[0]);
        }
    monte_neve(MESA_X0, MESA_Y1 - 1, 1, -1);
    monte_neve(MESA_X1 - 1, MESA_Y1 - 1, -1, -1);
    if (estilo_mesa == MS_LED || estilo_mesa == MS_LAREIRA) return;   // a fita de led e a brasa ficam
    int a = MESA_X0 - 5, b = MESA_X1 + 4, t = MESA_Y0 - 5, u = MESA_Y1 + 4;
    natal_fita(a, b, t, u);
    // O pisca-pisca: na fita, uma lampada a cada 14 pixels, na ordem
    // vermelha, dourada, verde e azul, dando a volta na mesa.
    int nl = 0;
    for (int x = a + 16; x < b - 14; x += 14, nl++) anima_poe(x - 1, t, 4, 6, AN_LAMPADA, (uint8_t)(nl % 4));
    for (int y = t + 16; y < u - 14; y += 14, nl++) anima_poe(b - 4, y - 2, 4, 6, AN_LAMPADA, (uint8_t)(nl % 4));
    for (int x = b - 18; x > a + 12; x -= 14, nl++) anima_poe(x - 1, u - 4, 4, 6, AN_LAMPADA, (uint8_t)(nl % 4));
    for (int y = u - 16; y > t + 12; y -= 14, nl++) anima_poe(a + 1, y - 2, 4, 6, AN_LAMPADA, (uint8_t)(nl % 4));
    azevinho(a, t, 1, 1);
    azevinho(b, t, -1, 1);
    azevinho(a, u, 1, -1);
    azevinho(b, u, -1, -1);
}

// O enfeite de um feltro num retalho pequeno (amostras e icones): as
// estrelas e a nevoa do ceu; a fita, o pisca-pisca e a neve do Natal.
static uint16_t xadrez(int x, int y, int i, uint16_t base);
static void retalho_enfeite(int k, int x, int y, int w, int h)
{
    if (k == FT_XADREZ) {                        // o xadrez, menor: meio sett
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++) {
                int sx = i * 2 + 4, sy = j * 2 + 10;
                gfx_pixel(x + i, y + j, xadrez(sx, sy, 0, FELTROS[k].tons[3]));
            }
        return;
    }
    if (k == FT_NOITE_FELIZ) {                   // a neve: pontinhos e uns flocos
        for (int j = 1; j < h - 1; j++)
            for (int i = 1; i < w - 1; i++) {
                uint32_t q = ruido_xy(i * 5 + 1, j * 11 + 3) % 40;
                if (q == 0) gfx_pixel(x + i, y + j, NOITE_NEVE[(ruido_xy(i, j) >> 7) % 3 ? 0 : 1]);
            }
        if (w >= 20) { floco(x + w / 3, y + h / 3, NOITE_NEVE[1]); floco(x + w * 3 / 4, y + h * 2 / 3, NOITE_NEVE[1]); }
        return;
    }
    if (k == FT_REVEILLON) {                     // o filete de ouro e um fogo no canto
        gfx_moldura(x + 1, y + 1, w - 2, h - 2, 1, FOGOS_TOM[3]);
        if (w >= 20) {
            int cx = x + w / 3, cy = y + h / 2 - 2;
            for (int a2 = 0; a2 < 12; a2++) {
                float an = a2 * 0.5236f;
                gfx_pixel(cx + (int)lroundf(cosf(an) * 6), cy + (int)lroundf(sinf(an) * 6), FOGOS_TOM[1]);
                gfx_pixel(cx + (int)lroundf(cosf(an) * 3), cy + (int)lroundf(sinf(an) * 3), FOGOS_TOM[2]);
            }
            gfx_pixel(cx, cy, FOGOS_TOM[0]);
            gfx_pixel(x + w * 3 / 4, y + h / 3, FOGOS_TOM[2]);
            gfx_pixel(x + w * 2 / 3, y + h * 3 / 4, FOGOS_TOM[3]);
        } else gfx_pixel(x + w / 2, y + h / 2, FOGOS_TOM[1]);
        return;
    }
    if (k == FT_GALAXIA) {
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++) {
                float d = fabsf(j - h * 0.5f + (i - w * 0.5f) * 0.35f) / (h * 0.28f);
                if (d < 1 && ((i + j) & 1 || d < 0.45f)) gfx_pixel(x + i, y + j, d < 0.45f ? GAL_NEVOA[1] : GAL_NEVOA[0]);
                uint32_t q = ruido_xy(i * 7 + 3, j * 13 + 5) % 23;
                if (q == 0) gfx_pixel(x + i, y + j, GAL_ESTRELA[(ruido_xy(i, j) >> 9) % 4]);
            }
        if (w >= 20) estrela4(x + w / 3, y + h / 3, 2, GAL_CINTILA[3], GAL_CINTILA[1], GAL_CINTILA[0]);
    } else if (k == FT_NATAL) {
        gfx_moldura(x, y, w, h, 1, C_LATAO);
        if (w >= 20) {
            gfx_moldura(x + 1, y + 1, w - 2, h - 2, 2, NATAL_FITA[1]);
            gfx_moldura(x + 3, y + 3, w - 6, h - 6, 1, C_LATAO);
            for (int i = 0; i < w - 2; i += 2) gfx_pixel(x + 1 + i, y, NATAL_NEVE[0]);   // neve em cima
            for (int i = 0; i < 4; i++) {                                           // o pisca-pisca
                int lx = x + w / 5 + i * (w * 3 / 5) / 3, ly = y + 7 + (i == 1 || i == 2);
                gfx_rect(lx, ly, 2, 2, NATAL_ACESA[i]);
            }
            floco(x + w * 3 / 4, y + h * 2 / 3, NATAL_NEVE[0]);
        } else {
            gfx_moldura(x + 1, y + 1, w - 2, h - 2, 1, NATAL_FITA[1]);
            gfx_pixel(x + w / 3, y + 3, NATAL_ACESA[0]);
            gfx_pixel(x + w * 2 / 3, y + 3, NATAL_ACESA[1]);
        }
    } else if (k == FT_MAR) {                    // a lua, umas ondas e o reflexo
        int lx = x + w / 4, ly = y + h / 4;
        for (int j = 2; j < h - 1; j += 3)
            for (int i = 1; i < w - 4; i += 6)
                if (ruido_xy(i * 3 + 1, j * 5) % 3 == 0) {
                    gfx_pixel(x + i, y + j, MAR_TOM[0]); gfx_pixel(x + i + 1, y + j - 1, MAR_TOM[0]);
                    gfx_pixel(x + i + 2, y + j, MAR_TOM[0]);
                }
        if (w >= 20) {
            gfx_disco(lx, ly, 3, MAR_TOM[2]);
            for (int j = ly + 6; j < y + h - 1; j += 3) gfx_rect(lx - 1 - (j - ly) / 8, j, 3 + (j - ly) / 4, 1, MAR_TOM[3]);
        } else gfx_rect(lx, ly, 2, 2, MAR_TOM[2]);
    } else if (k == FT_DECO) {                   // o filete e os leques, pequenos
        gfx_moldura(x + 1, y + 1, w - 2, h - 2, 1, C_LATAO_ESC);
        int r = w >= 20 ? 8 : 4;
        static const int SX[4] = { 1, -1, 1, -1 }, SY[4] = { 1, 1, -1, -1 };
        for (int c = 0; c < 4; c++) {
            int ox = SX[c] > 0 ? x + 1 : x + w - 2, oy = SY[c] > 0 ? y + 1 : y + h - 2;
            for (int a2 = 0; a2 <= 8; a2++) {
                float t = a2 * 0.19635f;
                gfx_pixel(ox + SX[c] * (int)lroundf(cosf(t) * r), oy + SY[c] * (int)lroundf(sinf(t) * r), C_LATAO);
                if (a2 % 2 == 0 && r > 4)
                    for (int q = 2; q < r; q += 2) gfx_pixel(ox + SX[c] * (int)lroundf(cosf(t) * q), oy + SY[c] * (int)lroundf(sinf(t) * q), C_LATAO_ESC);
            }
        }
    } else if (k == FT_COURO) {                  // a costura em volta
        int m = w >= 20 ? 3 : 1;
        for (int i = 0; i < w - 2 * m; i++)
            if (i % 4 < 2) { gfx_pixel(x + m + i, y + m, COURO_LINHA[0]); gfx_pixel(x + m + i, y + h - 1 - m, COURO_LINHA[0]); }
        for (int j = 0; j < h - 2 * m; j++)
            if (j % 4 < 2) { gfx_pixel(x + m, y + m + j, COURO_LINHA[0]); gfx_pixel(x + w - 1 - m, y + m + j, COURO_LINHA[0]); }
    } else if (k == FT_LOSANGO) {                // a estampa, um tom acima
        int p = w >= 20 ? 16 : 8;
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++)
                if ((i + 2 * j) % p == 0 || (i - 2 * j + 4096) % p == 0) gfx_pixel(x + i, y + j, FELTROS[k].tons[4]);
    }
}

// O mar a noite: ondinhas espalhadas, a lua no alto a esquerda e o caminho
// do reflexo descendo ate a borda de baixo, em tracos que cintilam
// (mesa_anima), cada vez mais abertos para perto de quem joga.
#define LUA_X (MESA_X0 + 92)
#define LUA_Y (MESA_Y0 + 26)
static void pinta_mar(void)
{
    for (int y = MESA_Y0 + 6; y < MESA_Y1 - 6; y += 7)            // as ondas: um "~" aqui e ali
        for (int x = MESA_X0 + 6; x < MESA_X1 - 10; x += 13) {
            uint32_t h = ruido_xy(x * 5 + 3, y * 7 + 1);
            if (h % 3) continue;
            int ox = x + (int)(h >> 8) % 9, oy = y + (int)(h >> 12) % 4;
            uint16_t c = (h >> 16) % 5 == 0 ? MAR_TOM[1] : MAR_TOM[0];
            gfx_pixel(ox, oy, c); gfx_pixel(ox + 1, oy - 1, c); gfx_pixel(ox + 2, oy - 1, c);
            gfx_pixel(ox + 3, oy, c); gfx_pixel(ox + 4, oy, c);
        }
    for (int j = -12; j <= 12; j++)                                 // o halo da lua, pontilhado, mais ralo para fora
        for (int i = -12; i <= 12; i++) {
            int d = i * i + j * j;
            if (d > 64 && d <= 100 && ((i + j) & 1)) gfx_pixel(LUA_X + i, LUA_Y + j, MAR_TOM[1]);
            else if (d > 100 && d <= 144 && !((i + j) & 3)) gfx_pixel(LUA_X + i, LUA_Y + j, MAR_TOM[0]);
        }
    gfx_disco(LUA_X, LUA_Y, 7, MAR_TOM[2]);
    gfx_pixel(LUA_X - 2, LUA_Y - 1, MAR_TOM[3]); gfx_pixel(LUA_X + 3, LUA_Y + 2, MAR_TOM[3]);
    gfx_pixel(LUA_X + 1, LUA_Y - 3, MAR_TOM[3]); gfx_pixel(LUA_X - 3, LUA_Y + 3, MAR_TOM[3]);
    for (int y = LUA_Y + 16, k = 0; y < MESA_Y1 - 4; y += 4, k++) {  // o reflexo, so as caixas: mesa_anima desenha
        int abre = 5 + (y - LUA_Y) / 14;
        anima_poe(LUA_X - abre, y, 2 * abre + 1, 1, AN_REFLEXO, (uint8_t)k);
    }
}

// O art deco: um filete de ouro velho rente a borda e, em cada canto, um
// leque de arcos e raios finos.
static void leque_deco(int x, int y, int sx, int sy)
{
    for (int a = 0; a <= 24; a++) {
        float t = a * 0.06545f;                                     // 0 a 90 graus
        float ca = cosf(t), sa = sinf(t);
        for (int r = 9; r <= 25; r += 8) gfx_pixel(x + sx * (int)lroundf(ca * r), y + sy * (int)lroundf(sa * r), r == 25 ? C_LATAO : C_LATAO_ESC);
        if (a % 4 == 0)
            for (int r = 4; r < 25; r++) if (r < 8 || r > 10) gfx_pixel(x + sx * (int)lroundf(ca * r), y + sy * (int)lroundf(sa * r), C_LATAO_ESC);
    }
    gfx_rect(x + (sx > 0 ? 0 : -1), y + (sy > 0 ? 0 : -1), 2, 2, C_LATAO);
}
static void pinta_deco(void)
{
    int a = MESA_X0 + 4, b = MESA_X1 - 5, t = MESA_Y0 + 4, u = MESA_Y1 - 5;
    gfx_rect(a + 28, t, b - a - 55, 1, C_LATAO_ESC);
    gfx_rect(a + 28, u, b - a - 55, 1, C_LATAO_ESC);
    gfx_rect(a, t + 28, 1, u - t - 55, C_LATAO_ESC);
    gfx_rect(b, t + 28, 1, u - t - 55, C_LATAO_ESC);
    leque_deco(a, t, 1, 1);
    leque_deco(b, t, -1, 1);
    leque_deco(a, u, 1, -1);
    leque_deco(b, u, -1, -1);
    int m = (a + b) / 2;                                            // no meio de cima e de baixo, um losango
    for (int k = -3; k <= 3; k++) {
        int w = 3 - (k < 0 ? -k : k);
        gfx_rect(m - w, t + k, 2 * w + 1, 1, k == 0 ? C_LATAO : C_LATAO_ESC);
        gfx_rect(m - w, u + k, 2 * w + 1, 1, k == 0 ? C_LATAO : C_LATAO_ESC);
    }
}

// O couro: a costura em volta, tracos de linha com o furo escuro no fim e um
// vinco por fora.
static void pinta_costura(void)
{
    int a = MESA_X0 + 6, b = MESA_X1 - 7, t = MESA_Y0 + 6, u = MESA_Y1 - 7;
    gfx_rect(a - 2, t - 2, b - a + 5, 1, C_SOMBRA);                 // o vinco
    gfx_rect(a - 2, u + 2, b - a + 5, 1, C_FELTRO2);
    gfx_rect(a - 2, t - 2, 1, u - t + 5, C_SOMBRA);
    gfx_rect(b + 2, t - 2, 1, u - t + 5, C_FELTRO2);
    for (int x = a; x <= b; x++) {
        int q = (x - a) % 6;
        uint16_t c = q < 4 ? COURO_LINHA[0] : q == 4 ? COURO_LINHA[1] : 0;
        if (c) { gfx_pixel(x, t, c); gfx_pixel(x, u, c); }
    }
    for (int y = t; y <= u; y++) {
        int q = (y - t) % 6;
        uint16_t c = q < 4 ? COURO_LINHA[0] : q == 4 ? COURO_LINHA[1] : 0;
        if (c) { gfx_pixel(a, y, c); gfx_pixel(b, y, c); }
    }
}

// A fita de led da mesa Led, na forca k (0 a 4): tres pixels no lugar da
// moldura, o do meio com os leds acesos a cada 3 pixels, e o halo na laca.
// No pico (4) tudo sobe mais um tom: o led mais aceso, puxando para o rosa.
static void fita_led(int k)
{
    int x = MESA_X0, y = MESA_Y0, w = MESA_X1 - MESA_X0, h = MESA_Y1 - MESA_Y0;
    int c = k >= 4 ? k + 1 : k, q = c + 1 < LED_N ? c + 1 : LED_N - 1;
    gfx_moldura(x - 8, y - 8, w + 16, h + 16, 1, LED_HALO[c > 0 ? c - 1 : 0]);
    gfx_moldura(x - 7, y - 7, w + 14, h + 14, 1, LED_HALO[q]);
    gfx_moldura(x - 6, y - 6, w + 12, h + 12, 1, LED_TOM[c]);
    gfx_moldura(x - 5, y - 5, w + 10, h + 10, 1, LED_TOM[q]);
    gfx_moldura(x - 4, y - 4, w + 8, h + 8, 1, LED_TOM[c]);
    gfx_moldura(x - 3, y - 3, w + 6, h + 6, 1, LED_HALO[q]);
    gfx_moldura(x - 2, y - 2, w + 4, h + 4, 2, C_SOMBRA);
    uint16_t chip = LED_TOM[c + 2 < LED_N ? c + 2 : LED_N - 1];
    for (int i = x - 5; i < x + w + 5; i += 3) { gfx_pixel(i, y - 5, chip); gfx_pixel(i, y + h + 4, chip); }
    for (int j = y - 2; j < y + h + 3; j += 3) { gfx_pixel(x - 5, j, chip); gfx_pixel(x + w + 4, j, chip); }
}

// A forca do led no quadro q: respira devagar, um ciclo a cada 20 quadros.
static int led_forca(int q) { return (int)lroundf((sinf(q * 6.2832f / 20) + 1) * 2); }

// O xadrez escoces (sett de 64), discreto: a faixa larga verde, apagada, e
// um fio so um pouco mais claro que o feltro, nos dois sentidos; onde o verde
// cruza o vermelho, o tom de mistura; o fio, em sarja (um pixel de cada
// sentido). i: o tom do feltro ali (0 o claro do meio, 2 o escuro da beira);
// base: a cor dele.
static int faixa_xadrez(int p)
{
    p %= 64;
    return p < 16 ? 1 : p == 40 ? 2 : 0;                 // 0 vermelho, 1 verde, 2 fio
}
static uint16_t xadrez(int x, int y, int i, uint16_t base)
{
    int fx = faixa_xadrez(x + 8), fy = faixa_xadrez(y + 26);
    int f = ((x + y) & 1) ? fx : fy;                     // a sarja: o fio da vez
    if (f == 2) return XADREZ_FIO[i == 0];
    int o = f == fx ? fy : fx;                           // o outro sentido, por baixo
    if (o == 2) o = 0;
    if (f == 1 && o == 1) return XADREZ_VERDE[2 - i];
    if (f == 1 || o == 1) return XADREZ_MISTO[2 - i];
    return base;
}

// A Noite Feliz: neve fraquinha parada ao longe e flocos caindo devagar,
// cada um na sua coluna (mesa_anima).
static void pinta_noite_feliz(void)
{
    for (int y = MESA_Y0 + 3; y < MESA_Y1 - 3; y++)
        for (int x = MESA_X0 + 3; x < MESA_X1 - 3; x++) {
            uint32_t h = ruido_xy(x * 5 + 11, y * 3 + 2);
            if (h % 520 == 0) gfx_pixel(x, y, NOITE_NEVE[0]);
        }
    for (int i = 0; i < 22; i++) {
        uint32_t h = ruido_xy(i * 71 + 5, i * 13 + 3);
        int x = MESA_X0 + 12 + (int)(h % (uint32_t)(MESA_X1 - MESA_X0 - 24));
        int alto = 70 + (int)((h >> 10) % 70), y = MESA_Y0 + 4 + (int)((h >> 18) % (uint32_t)(MESA_Y1 - MESA_Y0 - alto - 8));
        anima_poe(x - 2, y, 5, alto, AN_FLOCO, i);
    }
}

// O Reveillon: um filete de ouro velho rente a borda, pontinhos de ouro aqui
// e ali e, nos quatro cantos, fogos pequenos, um de cada vez.
#define FOGOS_LADO 45
static void pinta_reveillon(void)
{
    int a = MESA_X0 + 5, b = MESA_X1 - 6, t = MESA_Y0 + 5, u = MESA_Y1 - 6;
    gfx_rect(a, t, b - a + 1, 1, FOGOS_TOM[3]); gfx_rect(a, u, b - a + 1, 1, FOGOS_TOM[3]);
    gfx_rect(a, t, 1, u - t + 1, FOGOS_TOM[3]); gfx_rect(b, t, 1, u - t + 1, FOGOS_TOM[3]);
    for (int y = MESA_Y0 + 8; y < MESA_Y1 - 8; y++)
        for (int x = MESA_X0 + 8; x < MESA_X1 - 8; x++) {
            uint32_t h = ruido_xy(x * 9 + 4, y * 7 + 1);
            if (h % 900 == 0) gfx_pixel(x, y, FOGOS_TOM[(h >> 12) % 3 ? 3 : 2]);
        }
    static const int SX[4] = { 0, 1, 0, 1 }, SY[4] = { 0, 0, 1, 1 };
    for (int c = 0; c < 4; c++) {
        int x = SX[c] ? MESA_X1 - 10 - FOGOS_LADO : MESA_X0 + 10, y = SY[c] ? MESA_Y1 - 8 - FOGOS_LADO : MESA_Y0 + 8;
        anima_poe(x, y, FOGOS_LADO, FOGOS_LADO, AN_FOGOS, c == 0 ? 0 : c == 3 ? 1 : c == 1 ? 2 : 3);
    }
}
// Um quadro (f, de 0 a 25) do fogo na caixa a: a faisca subindo do pe da
// caixa, o estouro que abre em raios e cai um pouco, e o resto apagando.
static void fogos_quadro(const anima_t *a, int f)
{
    int cx = a->x + a->w / 2, base = a->y + a->h - 2, cy = a->y + a->h / 2 - 2;
    if (f < 4) {                                         // a faisca subindo, com um rastro curto
        int y = base - (base - cy) * (f + 1) / 4;
        gfx_pixel(cx, y, FOGOS_TOM[0]);
        gfx_pixel(cx, y + 1, FOGOS_TOM[2]);
        gfx_pixel(cx, y + 2, FOGOS_TOM[3]);
        return;
    }
    int k = f - 4;                                       // o estouro: 8 quadros
    if (k >= 8) return;
    float r = 4 + k * 2.2f;
    int tom = k < 2 ? 0 : k < 4 ? 1 : k < 6 ? 2 : 3;
    for (int i = 0; i < 12; i++) {
        float an = i * 0.5236f + 0.26f;
        int px = cx + (int)lroundf(cosf(an) * r), py = cy + (int)lroundf(sinf(an) * r) + k * k / 8;   // cai um pouco
        gfx_pixel(px, py, FOGOS_TOM[tom]);
        if (k < 5) gfx_pixel(cx + (int)lroundf(cosf(an) * r * 0.7f), cy + (int)lroundf(sinf(an) * r * 0.7f) + k * k / 8,
                             FOGOS_TOM[tom + 1 < 4 ? tom + 1 : 3]);
    }
    if (k == 0) gfx_rect(cx - 1, cy - 1, 3, 3, FOGOS_TOM[0]);
}

// A lareira: tijolinho em volta do feltro (fiadas de 4 com o rejunte, a
// junta desencontrada, um tijolo ou outro mais claro).
static void tijolos(int x0, int y0, int x1, int y1)
{
    const uint16_t *pm = MESAS[MS_LAREIRA].madeira;
    for (int y = y0; y < y1; y++) {
        int fiada = (y - y0) / 4, dy = (y - y0) % 4;
        for (int x = x0; x < x1; x++) {
            int bx = x - x0 + (fiada & 1) * 5, dx = bx % 10;
            uint16_t c;
            if (dy == 3 || dx == 9) c = pm[0];                                   // o rejunte
            else {
                uint32_t h = ruido_xy(bx / 10 + 3, fiada * 7 + 1);
                c = h % 4 == 0 ? pm[2] : h % 7 == 1 ? BRASA_TIJOLO[0] : pm[1];
                if (dy == 0 && c == pm[1]) c = BRASA_TIJOLO[1];                    // a beira de cima, na luz
            }
            gfx_pixel(x, y, c);
        }
    }
}
static uint16_t brasa_cor(int x, int y, int m, int k)
{
    if (m == 0) return BRASA_TOM[k >= 3 ? 2 : 1];                  // a beira, no calor
    if (m == 3) return C_SOMBRA;                                   // a sombra rente ao feltro
    uint32_t h = ruido_xy(x / 2 * 3 + 7, y * 5 + 3);
    int b = (int)(h % 5);                                          // cada pedaco de carvao, com o seu brilho
    int n = b + k - 1;
    if ((h >> 8) % 4 == 0) n -= 3;                                 // o carvao apagado
    n = n < 0 ? 0 : n > BRASA_N - 1 ? BRASA_N - 1 : n;
    return BRASA_TOM[n];
}
// A brasa no lugar da moldura, na forca k (0 a 4, com o led): o carvao com
// as brasas acendendo e um calor no tijolo em volta.
static void fita_brasa(int k)
{
    int x = MESA_X0, y = MESA_Y0, w = MESA_X1 - MESA_X0, h = MESA_Y1 - MESA_Y0;
    for (int m = 0; m < 4; m++) {                        // de fora (m = 0) para dentro
        int a = x - 4 + m, b = x + w + 3 - m, t = y - 4 + m, u = y + h + 3 - m;
        for (int i = a; i <= b; i++)
            for (int lado = 0; lado < 2; lado++) {
                int yy = lado ? u : t;
                gfx_pixel(i, yy, brasa_cor(i, yy, m, k));
            }
        for (int j = t + 1; j < u; j++)
            for (int lado = 0; lado < 2; lado++) {
                int xx = lado ? b : a;
                gfx_pixel(xx, j, brasa_cor(xx, j, m, k));
            }
    }
}
// O pinho: uma fita vermelha de cetim no lugar da moldura, com o laco no
// meio de cima.
static void fita_pinho(void)
{
    int a = MESA_X0 - 5, b = MESA_X1 + 4, t = MESA_Y0 - 5, u = MESA_Y1 + 4;
    for (int k = 0; k < 5; k++) {
        uint16_t claro = k == 1 ? FITA_VERM[2] : k == 4 ? C_SOMBRA : FITA_VERM[1];
        uint16_t escuro = k == 4 ? C_SOMBRA : k == 0 ? FITA_VERM[0] : FITA_VERM[1];
        gfx_rect(a + k, t + k, b - a - 2 * k + 1, 1, claro);
        gfx_rect(a + k, t + k, 1, u - t - 2 * k + 1, claro);
        gfx_rect(a + k, u - k, b - a - 2 * k + 1, 1, escuro);
        gfx_rect(b - k, t + k, 1, u - t - 2 * k + 1, escuro);
    }
    int m = (a + b) / 2, c = t + 2;                      // o laco: duas alcas, o no e as pontas
    for (int s = -1; s <= 1; s += 2) {
        for (int j = -4; j <= 4; j++)
            for (int i = 1; i <= 10; i++) {
                float e = (i - 5.5f) * (i - 5.5f) / 30.0f + j * j / 18.0f;
                if (e >= 1) continue;
                gfx_pixel(m + s * (i + 1), c + j, e < 0.35f ? FITA_VERM[0] : j < 0 ? FITA_VERM[2] : FITA_VERM[1]);
            }
        for (int i = 0; i < 7; i++) gfx_rect(m + s * (2 + i / 2) - (s < 0), c + 3 + i, 2, 1, FITA_VERM[1]);
    }
    gfx_rect(m - 2, c - 3, 5, 7, FITA_VERM[1]);
    gfx_rect(m - 1, c - 2, 3, 5, FITA_VERM[2]);
}

// As faixas de cima e de baixo sao a borda da propria mesa: a madeira dela
// ate a beira da tela, a quina de fora com a luz, uma canaleta escura
// rebaixada onde ficam os textos (o contraste de sempre: sombra na beira de
// cima, luz na de baixo) e tachas na beira de fora.
#define CANAL_0 8                        // a canaleta, contada da beira da tela
#define CANAL_1 37
static void trilhos(uint16_t veio, uint16_t claro, uint16_t metal, bool tachas)
{
    for (int b = 0; b < 2; b++) {
        int ya = b ? GFX_H - 1 - CANAL_1 : CANAL_0, yb = b ? GFX_H - 1 - CANAL_0 : CANAL_1;
        gfx_rect(0, b ? GFX_H - 1 : 0, GFX_W, 1, veio);                   // a quina: o escuro na beira
        gfx_rect(0, b ? GFX_H - 2 : 1, GFX_W, 1, claro);                  // e a luz logo dentro
        for (int y = ya; y <= yb; y++)                                     // a canaleta, com um veio apagado
            for (int x = 0; x < GFX_W; x++)
                gfx_pixel(x, y, ruido_xy(x / 9, y) % 11 == 0 ? C_MADEIRA_ESC : C_EBANO);
        gfx_rect(0, ya - 1, GFX_W, 1, C_BARRA);
        gfx_rect(0, yb + 1, GFX_W, 1, claro);
        if (!tachas) continue;
        int ty = b ? GFX_H - 5 : 4;
        for (int x = 10; x < GFX_W - 6; x += 124) tacha(x, ty, metal);
        tacha(GFX_W - 11, ty, metal);
    }
}

static void pinta_mesa_toda(void);
static void pinta_mesa(void)
{
    n_anima = anima_usado = 0;
    pinta_mesa_toda();
    anima_guarda();
}

static void pinta_mesa_toda(void)
{
    int lv = mesa_andar;
    gfx_clear(C_BARRA);
    int x0 = 0, y0 = 0, x1 = GFX_W, y1 = GFX_H;          // a madeira vai ate a beira da tela: as faixas sao a borda
    // A mesa escolhida tem a madeira e o metal dela; a de sempre e nogueira,
    // mogno do andar do rubi em diante, e o metal sobe com os andares:
    // latao no terreo, bronze, prata, e ouro dali para cima.
    bool propria = estilo_mesa != MS_NOGUEIRA;
    const uint16_t *pm = MESAS[estilo_mesa].madeira;
    uint16_t mad = propria ? pm[1] : lv >= 4 ? gfx_mistura(C_MADEIRA, RGB(92, 26, 30), 45) : C_MADEIRA;
    uint16_t veio = propria ? pm[0] : gfx_mistura(mad, C_SOMBRA, 30);
    gfx_rect(x0, y0, x1 - x0, y1 - y0, mad);
    if (estilo_mesa == MS_LAREIRA) tijolos(x0, y0, x1, y1);
    else
        for (int y = y0; y < y1; y += 2)                // veios da madeira
            for (int x = x0; x < x1; x++)
                if (ruido_xy(x / 7, y) % 5 == 0) gfx_pixel(x, y, veio);
    uint16_t metal = propria ? MESAS[estilo_mesa].metal
                             : lv == 1 ? C_BRONZE : lv == 2 ? C_PRATA : lv >= 3 ? C_OURO : C_LATAO;
    trilhos(veio, propria ? pm[2] : C_MADEIRA_CLR, metal, estilo_mesa != MS_LAREIRA);
    bool led = estilo_mesa == MS_LED;
    // Sem moldura de metal: a fita de led, a brasa e a fita do pinho.
    bool sem_metal = led || estilo_mesa == MS_LAREIRA || estilo_mesa == MS_PINHO;
    if (led) fita_led(led_forca(0));                    // a moldura e a fita (mesa_anima a faz respirar)
    else if (estilo_mesa == MS_LAREIRA) fita_brasa(led_forca(0));
    else if (estilo_mesa == MS_PINHO) fita_pinho();
    else {
        uint16_t rm[4];                                 // o filete de metal em dois tons: a luz por fora
        rampa_de(propria ? metal : lv >= 3 ? C_OURO : C_LATAO, rm);
        gfx_moldura(MESA_X0 - 4, MESA_Y0 - 4, MESA_X1 - MESA_X0 + 8, MESA_Y1 - MESA_Y0 + 8, 1, rm[3]);
        gfx_moldura(MESA_X0 - 3, MESA_Y0 - 3, MESA_X1 - MESA_X0 + 6, MESA_Y1 - MESA_Y0 + 6, 1, rm[1]);
        gfx_moldura(MESA_X0 - 2, MESA_Y0 - 2, MESA_X1 - MESA_X0 + 4, MESA_Y1 - MESA_Y0 + 4, 2, C_SOMBRA);
    }
    float cx = (MESA_X0 + MESA_X1) / 2.0f, cy = (MESA_Y0 + MESA_Y1) / 2.0f;
    float ax = (MESA_X1 - MESA_X0) / 2.0f, ay = (MESA_Y1 - MESA_Y0) / 2.0f;
    // O feltro escurece para as bordas em tres tons da paleta, com pontilhado
    // ordenado (Bayer 4x4) na passagem de um para o outro, e uma fibra ou
    // outra mais clara ou mais escura.
    static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
    const uint16_t TOM[3] = { C_FELTRO, C_FELTRO_MEIO, C_FELTRO_ESC };
    for (int y = MESA_Y0; y < MESA_Y1; y++)
        for (int x = MESA_X0; x < MESA_X1; x++) {
            float dx = (x - cx) / ax, dy = (y - cy) / ay;
            float v = (dx * dx * 0.6f + dy * dy) * 1.5f - 0.15f;
            if (v < 0) v = 0;
            int i = (int)v;
            if (i >= 2) i = 2;
            else if ((v - i) * 16 > BAYER[y & 3][x & 3]) i++;
            uint16_t c = TOM[i];
            uint32_t h = ruido_xy(x, y) % 23;
            if (h == 0) c = i == 0 ? C_FELTRO2 : TOM[i - 1];
            else if (h == 1 && i < 2) c = TOM[i + 1];
            if (estilo_feltro == FT_COURO) {                // o couro: poros miudos e rugas largas
                float g = ruido(x / 2.3f, y / 2.3f) + 0.35f * ruido(x / 17.0f + 5, y / 11.0f);
                if (g < -0.62f && i < 2) c = TOM[i + 1];
                else if (g > 0.66f) c = i == 0 ? C_FELTRO2 : TOM[i - 1];
            } else if (estilo_feltro == FT_LOSANGO) {       // a estampa: losangos largos, um tom acima
                int lx = x - MESA_X0 + 2 * (y - MESA_Y0);
                int ly = x - MESA_X0 - 2 * (y - MESA_Y0) + 4096;
                if (lx % 32 < 2 || ly % 32 < 2) c = i == 0 ? C_FELTRO2 : TOM[i - 1];
            } else if (estilo_feltro == FT_XADREZ) {        // o xadrez: as faixas nos dois sentidos, em sarja
                c = xadrez(x - MESA_X0, y - MESA_Y0, i, c);
            }
            gfx_pixel(x, y, c);
        }
    if (estilo_feltro == FT_GALAXIA) pinta_galaxia();
    else if (estilo_feltro == FT_NOITE_FELIZ) pinta_noite_feliz();
    else if (estilo_feltro == FT_REVEILLON) pinta_reveillon();
    else if (estilo_feltro == FT_MAR) pinta_mar();
    else if (estilo_feltro == FT_DECO) pinta_deco();
    else if (estilo_feltro == FT_COURO) pinta_costura();
    // Linha de aposta: uma elipse tracejada ao redor do centro da mesa.
    uint16_t linha = lv >= 3 ? gfx_mistura(C_FELTRO2, C_OURO, 30) : C_FELTRO2;
    for (int i = 0; i < 360; i++) {
        if ((i / 4) % 2) continue;
        float a = i * 0.017453f;
        gfx_rect((int)(cx + cosf(a) * 250) , (int)(cy + sinf(a) * 52), 2, 1, linha);
    }
    if (estilo_feltro == FT_NATAL) pinta_natal();      // por cima da linha: a fita contorna tudo
    if (lv <= 0) return;

    // Acabamentos do andar, discretos: so nas bordas.
    if (lv >= 2) filete(6, gfx_mistura(C_FELTRO, metal, 38), lv >= 3);
    if (lv >= 5) filete(9, gfx_mistura(C_FELTRO, metal, 22), true);
    int fx0 = MESA_X0 - 3, fx1 = MESA_X1 + 2, fy0 = MESA_Y0 - 3, fy1 = MESA_Y1 + 2;
    int my = (MESA_Y0 + MESA_Y1) / 2;
    if (sem_metal || estilo_feltro == FT_NATAL) return;   // a fita (de led, de brasa, do pinho, do Natal) e o acabamento
    // Nos cantos da moldura: tachas; no rubi e no diamante, pedras.
    uint16_t gema = lv == 4 ? C_RUBI : C_DIAMANTE;
    const int CX[4] = { fx0, fx1, fx0, fx1 }, CY[4] = { fy0, fy0, fy1, fy1 };
    for (int i = 0; i < 4; i++) {
        if (lv >= 4) pedra(CX[i], CY[i], gema);
        else tacha(CX[i], CY[i], metal);
    }
    // No meio das bordas (prata em diante): mais tachas; no rubi e no
    // diamante, uma pedra no meio de cima e de baixo.
    if (lv >= 2) {
        for (int k = 1; k <= 3; k++) {
            int x = MESA_X0 + (MESA_X1 - MESA_X0) * k / 4;
            if (lv >= 4 && k == 2) { pedra(x, fy0, gema); pedra(x, fy1, gema); continue; }
            tacha(x, fy0, metal); tacha(x, fy1, metal);
        }
        if (lv >= 4) { pedra(fx0, my, gema); pedra(fx1, my, gema); }
        else { tacha(fx0, my, metal); tacha(fx1, my, metal); }
    }
}

static int des_dificuldade(void);

// O quadro q dos enfeites que mexem, pintado na mesa pronta: as estrelas
// grandes cintilam cada uma no seu tempo, as lampadas apagam uma de cada vez
// em volta da mesa e o led respira.
static void mesa_anima(int q)
{
    uint16_t *fb = mesa_pronta;
    gfx_desvia(fb);
    for (int i = 0; i < n_anima; i++) {
        const anima_t *a = &anima[i];
        anima_volta(a, fb);
        int x = a->x + 3, y = a->y + 3;
        if (a->tipo == AN_ESTRELA) {
            int f = (q + i * 7 + a->cor * 3) % 19;           // brilha forte por dois quadros, fraca por um
            if (f == 0 || f == 2) estrela4(x, y, 3, GAL_CINTILA[3], GAL_CINTILA[a->cor ? 2 : 1], GAL_CINTILA[0]);
            else if (f == 1) estrela4(x, y, 3, C_BRANCO, GAL_CINTILA[3], GAL_CINTILA[1]);
            else if (f == 3) estrela4(x, y, 1, GAL_CINTILA[1], GAL_CINTILA[0], GAL_CINTILA[0]);
            else estrela4(x, y, 2, GAL_CINTILA[3], GAL_CINTILA[1], GAL_CINTILA[0]);
        } else if (a->tipo == AN_FLOCO) {              // a neve da Noite Feliz: cai devagar, balancando
            int h = a->h - 2, yy = (q * 2 + a->cor * 37) % (h + 12);
            if (yy >= h) continue;                       // entre um floco e o outro, uma pausa
            int xx = a->x + 1 + (int)lroundf(sinf((q + a->cor * 5) * 0.35f));
            gfx_pixel(xx, a->y + 1 + yy, NOITE_NEVE[1]);
            if (a->cor % 3 == 0) {                       // um ou outro, maior
                gfx_pixel(xx - 1, a->y + 1 + yy, NOITE_NEVE[0]); gfx_pixel(xx + 1, a->y + 1 + yy, NOITE_NEVE[0]);
                if (yy > 0) gfx_pixel(xx, a->y + yy, NOITE_NEVE[0]);
                if (yy + 1 < h) gfx_pixel(xx, a->y + 2 + yy, NOITE_NEVE[0]);
            }
        } else if (a->tipo == AN_FOGOS) {              // os fogos do Reveillon: sobe, estoura, apaga
            fogos_quadro(a, (q + a->cor * 11) % 26);
        } else if (a->tipo == AN_REFLEXO) {            // um traco do reflexo da lua: muda de tamanho e de lugar devagar
            uint32_t h = ruido_xy(a->cor * 13 + 7, q / 2 * 31 + a->cor);
            if (h % 5 == 0) continue;                    // apagado neste quadro
            int w = a->w, len = 2 + (int)(h >> 8) % (w / 2 > 3 ? w / 2 : 3);
            int ox = (int)(h >> 16) % (w - len + 1);
            int tom = 3 + a->cor / 14;                   // apaga descendo: do claro, perto da lua, ao que some
            if (tom > MAR_N - 1) tom = MAR_N - 1;
            if ((h >> 24) % 5 == 0 && tom > 3) tom--;    // um ou outro brilha um tom mais
            gfx_rect(a->x + ox, a->y, len, 1, MAR_TOM[tom]);
        } else {
            bool acesa = (q + i * 5) % 9 < 7;
            lampada(a->x + 1, a->y + 2, a->cor, acesa);
        }
    }
    if (estilo_mesa == MS_LED) fita_led(led_forca(q));
    if (estilo_mesa == MS_LAREIRA) fita_brasa(led_forca(q));
    gfx_desvia(NULL);
}

// ---- A luz de cena ---------------------------------------------------------
// A vinheta escura nas bordas do feltro vai pintada na mesa pronta (nao
// muda); por cima, a cada quadro, uma poca quente sob a fila de quem joga e
// um pulso curto quando um dado pousa valendo. Cada pixel do feltro sobe ou
// desce degraus da rampa (os tons do feltro, o latao escuro, o latao e o
// ouro) pelo limiar de Bayer; o que nao e feltro (os enfeites) fica como
// esta. A mesa com a luz do quadro fica em mesa_acesa: o que apaga um dado
// (acinzenta) e o que devolve o feltro comparam com ela.
static float t_px_pulso = -1;
static int px_pulso_x, px_pulso_y;
static void px_pulsa(int x, int y) { t_px_pulso = 0; px_pulso_x = x; px_pulso_y = y; }
#define PX_VINHETA 26                    // a largura da vinheta, em pixels
#define PX_PULSO_T 0.4f
static uint16_t px_rampa[9];
static uint8_t  px_lut[65536];           // a cor do feltro -> o degrau dela na rampa (0xFF: nao e feltro)
static int px_lut_versao = -1;
static uint16_t mesa_acesa_buf[GFX_W * GFX_H];
static const uint16_t *mesa_acesa = mesa_pronta;    // a mesa deste quadro, com a luz
static void px_rampa_monta(void)
{
    if (px_lut_versao == estilo_versao) return;
    px_lut_versao = estilo_versao;
    for (int i = 0; i < 6; i++) px_rampa[i] = feltro_tons[i];
    px_rampa[6] = C_LATAO_ESC; px_rampa[7] = C_LATAO; px_rampa[8] = C_OURO;
    memset(px_lut, 0xFF, sizeof px_lut);
    for (int i = 5; i >= 0; i--) px_lut[feltro_tons[i]] = (uint8_t)i;
}
// Sobe (ou desce) o pixel (x, y) do feltro por s degraus, pelo limiar.
static void px_degrau(uint16_t *fb, int x, int y, float s)
{
    int st = (int)floorf(s + bayer(x, y));
    if (!st) return;
    uint16_t *p = &fb[y * GFX_W + x];
    int k = px_lut[*p];
    if (k == 0xFF) return;                                // nao e feltro: fica
    k += st;
    *p = px_rampa[k < 0 ? 0 : k > 8 ? 8 : k];
}
// O risco do proximo dado, na sua vez: uma luz no feltro atras do lugar
// dele, na cor do risco (a do botao) - verde sem risco, latao ate 40%, vinho
// dali para cima -, pulsando devagar. Do feltro do meio para cima, os tons
// ja sao os da cor; ali a poca de quem joga nao entra (luz_risco_*).
static int chance_cair(int j);              // telas.inc
static bool mostra_chance(void);
static const uint16_t RISCO_COR[3][3] = {
    { RGB(88, 152, 84), RGB(88, 152, 84), RGB(130, 204, 112) },     // verde
    { RGB(106, 86, 44), RGB(150, 122, 62), RGB(190, 156, 86) },     // latao
    { RGB(118, 30, 38), RGB(190, 56, 60), RGB(208, 98, 108) },      // vinho
};
static int luz_risco_x = -1, luz_risco_y;
#define LUZ_RISCO_RX 30
#define LUZ_RISCO_RY 24
static float poca_cx, poca_cy, poca_rx, poca_ry;      // a poca de quem joga (0 de raio: nenhuma)
static float poca_luz(int x, int y)                  // quanto a poca sobe o feltro em (x, y)
{
    if (poca_rx <= 0) return 0;
    float qx = (x - poca_cx) / poca_rx, qy = (y - poca_cy) / poca_ry, q = qx * qx + qy * qy;
    return q < 1 ? 1.8f * (1 - q) * (1 - q * 0.5f) : 0;
}
static void luz_do_risco(uint16_t *fb)
{
    const fila_t *f = &F[vez];
    luz_risco_x = -1;
    if (fase != F_JOGA || f->lancados <= 0 || f->lancados >= f->n || !mostra_chance()) return;
    int c = chance_cair(vez), i = c <= 0 ? 0 : c < 40 ? 1 : 2;
    uint16_t rampa[9];
    for (int k = 0; k < 4; k++) rampa[k] = feltro_tons[k];
    for (int k = 4; k < 9; k++) rampa[k] = RISCO_COR[i][k - 4 < 2 ? k - 4 : 2];
    // Discreta: no fraco do pulso quase some; no forte, so uns pontos da cor.
    float pulso = 0.5f + 0.5f * sinf(t_relogio * 6.2832f * 0.7f), forca = 0.05f + 1.1f * pulso * pulso;
    int cx = SLOT_X(f->lancados), cy = zona(vez) + 5, rx = LUZ_RISCO_RX, ry = LUZ_RISCO_RY;
    luz_risco_x = cx; luz_risco_y = cy;
    for (int y = cy - ry < MESA_Y0 ? MESA_Y0 : cy - ry; y <= cy + ry && y < MESA_Y1; y++)
        for (int x = cx - rx < MESA_X0 ? MESA_X0 : cx - rx; x <= cx + rx && x < MESA_X1; x++) {
            float qx = (x - cx) / (float)rx, qy = (y - cy) / (float)ry, q = qx * qx + qy * qy;
            if (q >= 1) continue;
            uint16_t *p = &fb[y * GFX_W + x];
            int k = px_lut[*p];
            if (k == 0xFF) continue;
            // Perto do dado, a luz da cor; na beira, pontilhado, a da poca. O
            // ponto que nao chega a acender na cor fica com a luz da poca.
            float lp = poca_luz(x, y);
            if (bayer(x + 2, y + 1) < (1 - q) * 1.7f) {
                int kc = (k > 3 ? 3 : k) + (int)floorf(forca * (1 - q) * (1 - q * 0.3f) + bayer(x, y));
                if (kc > 3) { *p = rampa[kc > 8 ? 8 : kc]; continue; }
            }
            int kn = k + (int)floorf(lp + bayer(x, y));
            *p = px_rampa[kn < 0 ? 0 : kn > 8 ? 8 : kn];
        }
}
// A vinheta, uma vez, na mesa pintada.
static void px_vinheta(uint16_t *fb)
{
    px_rampa_monta();
    for (int y = MESA_Y0; y < MESA_Y1; y++) {
        int dy = y - MESA_Y0 < MESA_Y1 - 1 - y ? y - MESA_Y0 : MESA_Y1 - 1 - y;
        for (int x = MESA_X0; x < MESA_X1; x++) {
            int d = x - MESA_X0 < MESA_X1 - 1 - x ? x - MESA_X0 : MESA_X1 - 1 - x;
            if (dy < d) d = dy;
            if (d >= PX_VINHETA) { if (dy >= PX_VINHETA) x = MESA_X1 - PX_VINHETA - 1; continue; }
            px_degrau(fb, x, y, -1.3f * (1 - (float)d / PX_VINHETA));
        }
    }
}
// A poca e o pulso deste quadro, por cima da mesa ja copiada para a tela.
// Devolve se acendeu alguma coisa.
static bool luz_de_cena(void)
{
    px_rampa_monta();
    uint16_t *fb = display_fb();
    bool poca = fase == F_ORDEM || fase == F_APOSTA || fase == F_ROLANDO || fase == F_ARRUMA || fase == F_JOGA
             || fase == F_VEREDITO;
    bool pulso = t_px_pulso >= 0 && t_px_pulso < PX_PULSO_T;
    luz_risco_x = -1;
    poca_rx = 0;
    if (poca) {
        int n = max_fila(vez);
        poca_cx = (SLOT_X(0) + SLOT_X(n - 1)) * 0.5f; poca_cy = (float)zona(vez);
        poca_rx = (SLOT_X(n - 1) - SLOT_X(0)) * 0.5f + 64; poca_ry = 46;
        luz_do_risco(fb);                                 // antes da poca, que nao entra nela
    }
    if (poca) {                                           // a poca, embaixo da fila de quem joga
        float pcx = poca_cx, pcy = poca_cy, prx = poca_rx, pry = poca_ry;
        int x0 = (int)(pcx - prx), x1 = (int)(pcx + prx), y0 = (int)(pcy - pry), y1 = (int)(pcy + pry);
        for (int y = y0 < MESA_Y0 ? MESA_Y0 : y0; y <= y1 && y < MESA_Y1; y++)
            for (int x = x0 < MESA_X0 ? MESA_X0 : x0; x <= x1 && x < MESA_X1; x++) {
                float qx = (x - pcx) / prx, qy = (y - pcy) / pry, q = qx * qx + qy * qy;
                if (luz_risco_x >= 0) {                   // a luz do risco e so dela
                    float rx2 = (x - luz_risco_x) / (float)LUZ_RISCO_RX, ry2 = (y - luz_risco_y) / (float)LUZ_RISCO_RY;
                    if (rx2 * rx2 + ry2 * ry2 < 1) continue;
                }
                if (q < 1) px_degrau(fb, x, y, 1.8f * (1 - q) * (1 - q * 0.5f));
            }
    }
    if (pulso) {                                          // o pulso do dado que pousou valendo
        float f = 1 - t_px_pulso / PX_PULSO_T;
        for (int y = px_pulso_y - 40 < MESA_Y0 ? MESA_Y0 : px_pulso_y - 40; y <= px_pulso_y + 40 && y < MESA_Y1; y++)
            for (int x = px_pulso_x - 40 < MESA_X0 ? MESA_X0 : px_pulso_x - 40; x <= px_pulso_x + 40 && x < MESA_X1; x++) {
                float qx = (x - px_pulso_x) / 40.0f, qy = (y - px_pulso_y) / 40.0f, q = qx * qx + qy * qy;
                if (q < 1) px_degrau(fb, x, y, 2.4f * f * (1 - q));
            }
    }
    return poca || pulso;
}

static void mesa(void)
{
    mesa_andar = modo == M_DESAFIO ? (des_dificuldade() + 1) / 2 : 0;   // um acabamento a cada dois andares
    if (mesa_andar != mesa_andar_feito) { mesa_feita = false; mesa_andar_feito = mesa_andar; }
    static int versao_feita = -1;                        // trocou o feltro ou a mesa: pinta de novo
    if (estilo_versao != versao_feita) { mesa_feita = false; versao_feita = estilo_versao; }
    static int quadro_feito = -1;
    if (!mesa_feita) {
        pinta_mesa();
        px_vinheta(display_fb());                         // a vinheta, pintada com a mesa
        memcpy(mesa_pronta, display_fb(), sizeof mesa_pronta);
        mesa_feita = true;
        quadro_feito = -1;
    }
    int q = (int)(t_relogio / ANIMA_PASSO);
    if ((n_anima || estilo_mesa == MS_LED || estilo_mesa == MS_LAREIRA) && q != quadro_feito) {
        mesa_anima(q);
        quadro_feito = q;
    }
    memcpy(display_fb(), mesa_pronta, sizeof mesa_pronta);
    mesa_acesa = mesa_pronta;
    if (luz_de_cena()) {                                  // a luz do quadro: guarda a mesa acesa
        memcpy(mesa_acesa_buf, display_fb(), sizeof mesa_acesa_buf);
        mesa_acesa = mesa_acesa_buf;
    }
}

// O fundo de uma faixa (y0: o alto dela): a borda da mesa, como mesa()
// acabou de por na tela.
static void faixa_madeira(int y0)
{
    memcpy(display_fb() + y0 * GFX_W, mesa_acesa + y0 * GFX_W, (size_t)FAIXA_H * GFX_W * sizeof(uint16_t));
}

// Uma ficha vista de cima, no material dos dados. A face na rampa da cor,
// com a luz de cima a esquerda e o pontilhado so na passagem; com 'beira', o
// contorno escuro e a beira de um pixel, clara do lado da luz e escura do
// outro. Sem beira, so a face (o miolo de dentro do friso), no mesmo degrade
// da ficha inteira: (lx, ly) e o centro da ficha, de onde a luz se mede.
// rx < ry: a ficha virando de lado.
static void ficha_disco_em(int cx, int cy, int rx, int ry, uint16_t cor, bool beira, int lx, int ly, int lr)
{
    uint16_t r[4];
    rampa_de(cor, r);
    if (rx < 1) rx = 1;
    if (ry < 1) ry = 1;
    float ax = rx + 0.5f, ay = ry + 0.5f, bx = rx - 0.5f, by = ry - 0.5f;
    float ia = 1 / (ax * ax), ib = 1 / (ay * ay), ic = 1 / ((ax + 1) * (ax + 1)), id = 1 / ((ay + 1) * (ay + 1));
    float ie = bx > 0 ? 1 / (bx * bx) : 4, ig = by > 0 ? 1 / (by * by) : 4, il = 1.0f / lr;
    for (int y = -ry - 1; y <= ry + 1; y++)
        for (int x = -rx - 1; x <= rx + 1; x++) {
            int X = cx + x, Y = cy + y;
            if (x * x * ia + y * y * ib > 1) {            // fora: o contorno, so com beira
                if (beira && x * x * ic + y * y * id <= 1) gfx_pixel(X, Y, C_BARRA);
                continue;
            }
            if (beira && x * x * ie + y * y * ig > 1) {   // a beira: o tom pela direcao em que ela olha
                gfx_pixel(X, Y, tom_rampa(r, luz_de((float)x, (float)y), X, Y));
                continue;
            }
            // A face: na cor, com um toque de luz e de sombra so nos cantos
            // (o tom cheio vai de |s| 0 a 1.15: ali nem precisa da rampa).
            float s = ((X - lx) + (Y - ly)) * il;
            gfx_pixel(X, Y, s > -1.15f && s < 1.15f ? r[2] : tom_rampa(r, 0.33f - s * 0.16f, X, Y));
        }
}
static void ficha_disco(int cx, int cy, int rx, int ry, uint16_t cor)
{
    ficha_disco_em(cx, cy, rx, ry, cor, true, cx, cy, ry);
}

// A sombra de uma ficha: a dos dados (miolo cheio, beira pontilhada).
static void ficha_sombra(int cx, int cy, int R)
{
    uint16_t *alvo = gfx_alvo();
    sombra_px(alvo ? alvo : display_fb(), cx, cy, R);
}

static void ficha(int x, int y, uint16_t c)
{
    ficha_sombra(x + 1, y + 2, 6);
    ficha_disco(x, y, 6, 6, c);
    for (int i = 0; i < 6; i++) {                        // listras da borda
        float a = i * 1.0472f;
        gfx_rect(x + (int)(cosf(a) * 5.5f) - 1, y + (int)(sinf(a) * 5.5f) - 1, 2, 2, C_MARFIM);
    }
    uint16_t r[4];
    rampa_de(c, r);
    gfx_disco(x, y, 3, r[1]);                            // o miolo, fundo
}
