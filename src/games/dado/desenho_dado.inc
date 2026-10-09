// ===========================================================================
// Desenho de dados: poligono por numero de lados
// ===========================================================================
static void poligono(const float *vx, const float *vy, int n, uint16_t cor)
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
            if (x1 >= x0) gfx_rect(x0, y, x1 - x0 + 1, 1, cor);
        }
    }
}

// Embaralha dois inteiros: um acaso que so depende da mesa (o alvo do
// Laser, do Berserker...).
static uint32_t hash2(int x, int y)
{
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

// A forma diz quantos lados o dado tem antes de qualquer numero:
// moeda redonda, d4 triangulo, d6 quadrado de canto aparado, d8 losango,
// d12 pentagono, d20 hexagono.
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
    default:   // 20
        for (int i = 0; i < 6; i++) {
            float a = ang + i * 1.0472f;
            vx[n] = cx + cosf(a) * r * 1.08f; vy[n] = cy + sinf(a) * r * 1.08f; n++;
        }
        break;
    }
    return n;
}

static void pips(int cx, int cy, int v, int r, uint16_t c)
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
// guardado (-1: nao sabe).
static int marca_nivel = -1;

static float sombra_alt;                 // altura do dado no ar, em pixels

// Numero centrado no dado: grande nos dados grandes.
static void numero(float cx, float cy, float r, const char *s, uint16_t c)
{
    int e = r >= 34 ? 2 : 1;
    gfx_texto((int)cx - gfx_largura(s, e) / 2, (int)cy - 12 * e, s, c, e, true);
}

// Fechado no catalogo: o dado fica apagado como o anulado, mas sem o X.
static bool so_apagado;

// X grosso em vinho sobre o dado anulado.
static void risca(float cx, float cy, float r)
{
    if (so_apagado) return;
    int e = r < 12 ? 2 : (int)(r / 6);
    gfx_linha_grossa((int)(cx - r), (int)(cy - r), (int)(cx + r), (int)(cy + r), e, C_VINHO_CLR);
    gfx_linha_grossa((int)(cx - r), (int)(cy + r), (int)(cx + r), (int)(cy - r), e, C_VINHO_CLR);
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

// Um tom do desenho: a cor c entrando p% no corpo. No ebano entra mais; se
// ainda assim nao aparecer, entra a mesma cor, clareada.
static uint16_t tom_desenho(uint16_t corpo, uint16_t c, int p, int dono)
{
    if (!c) return corpo;
    uint16_t t = cor_de_dado(gfx_mistura(corpo, c, dono == 0 ? p : (p + 15 > 100 ? 100 : p + 15)));
    if (t == corpo || (dono != 0 && brilho_de(t) < brilho_de(corpo) + 14))
        t = cor_de_dado(gfx_mistura(corpo, gfx_mistura(c, C_MARFIM, 40), 70));
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

// Os cinco tons do corpo, da sombra ao reflexo. No marfim e no ebano, os
// tons vizinhos da paleta; num corpo tingido, o que a paleta tiver perto.
static void tons_do_corpo(uint16_t corpo, uint16_t *t)
{
    const uint16_t MARFIM[5] = { LUZ_CORPO[2], LUZ_CORPO[1], C_MARFIM_D, LUZ_CORPO[0], C_BRANCO };
    const uint16_t EBANO[5]  = { RGB(24, 22, 20), LUZ_CORPO[3], C_EBANO, LUZ_CORPO[4], LUZ_CORPO[5] };
    if (corpo == C_MARFIM_D) { memcpy(t, MARFIM, sizeof MARFIM); return; }
    if (corpo == C_EBANO)    { memcpy(t, EBANO, sizeof EBANO); return; }
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
        return fabsf(d + 0.08f) < l * 1.3f || fabsf(d - 0.30f) < l * 1.3f;
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
        static const float F[4][2] = { {-0.17f,-0.02f}, {0.17f,-0.02f}, {-0.17f,0.32f}, {0.17f,0.32f} };
        for (int i = 0; i < 4; i++) {
            float dx = u - F[i][0], dy = v - F[i][1];
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
        float y = v - 0.12f;
        return fabsf(sqrtf(u * u + y * y) - 0.42f) < l * 1.2f;
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
    case D_VIUVA: {                               // a teia
        float a = atan2f(v, u);
        if (fabsf(sinf(4 * a)) * ro < l) return 1;
        return fabsf(ro - 0.28f) < l || fabsf(ro - 0.52f) < l || fabsf(ro - 0.76f) < l;
    }
    default:
        return 0;
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
static void pinta_corpo(const float *vx, const float *vy, int n, float dcx, float dcy, int dr,
                        int tipo, const uint16_t *tons, const uint16_t *cores,
                        float cx, float cy, float ang, float r)
{
    float ca = cosf(-ang), sa = sinf(-ang), s = 1 / r;
    int tex = textura_de(tipo);
    float semente = tipo * 3.7f;
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
                float dx = x - cx, dy = y - cy, u = dx * ca - dy * sa, v = dx * sa + dy * ca;
                int p = desenho_interno(tipo, u * s, v * s, s);
                if (p) { gfx_pixel(x, y, cores[p]); continue; }
                float hx = dx * s + 0.34f, hy = dy * s + 0.38f;           // o reflexo
                float luz = -(dx + dy) * s * 0.75f
                          + 1.6f * fmaxf(0, 1 - (hx * hx + hy * hy) / 0.045f)
                          + 0.32f * ruido(u * s / 0.34f + semente, v * s / 0.34f)
                          + 0.14f * ruido(u * s / 0.15f, v * s / 0.15f + semente);
                if (padrao(tex, u, v, r / 22)) luz -= 1;
                int k = (int)lroundf(luz);
                gfx_pixel(x, y, tons[(k < -2 ? -2 : k > 2 ? 2 : k) + 2]);
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
static void marca_por_cima(int tipo, float r, int dono, uint16_t corpo, bool zerado)
{
    float t = r < 5 ? 5 : r;
    if (tipo == D_QUEBRADO) {
        uint16_t rc = dono == 0 ? C_EBANO_B : C_MARFIM_S;
        int e = esp_marca(r);
        gfx_linha_grossa(MX(-0.7f * r, -0.6f * r), MY(-0.7f * r, -0.6f * r), MX(-0.2f * r, -0.1f * r), MY(-0.2f * r, -0.1f * r), e, rc);
        gfx_linha_grossa(MX(-0.2f * r, -0.1f * r), MY(-0.2f * r, -0.1f * r), MX(0.1f * r, -0.4f * r), MY(0.1f * r, -0.4f * r), e, rc);
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
static void ficha_de_fogo(float cx, float cy, float r, float ea, float ang, int valor, int modo, bool zerado)
{
    int R = (int)(r + ea);
    float u = r / 9;                             // unidade do desenho
    uint16_t face = modo == 0 || valor == 1 ? C_FOGO : C_BRASA;
    gfx_disco((int)cx, (int)cy, R, gfx_mistura(face, C_BARRA, 25));
    gfx_disco((int)cx, (int)cy - 1, R - 1, face);
    for (int i = 0; i < 8; i++) {                // marcas da borda
        float a = ang + i * 0.785f;
        int mx = (int)(cx + cosf(a) * (R - 2 * u)), my = (int)(cy + sinf(a) * (R - 2 * u));
        gfx_disco(mx, my, (int)(u + 0.5f), C_AMARELO);
    }
    gfx_disco((int)cx, (int)cy, (int)(R - 4 * u), face);
    if (modo != 0) {
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

// modo 0: parado na mao, mostra quantos lados tem.
// modo 1: rolando, mostra uma face qualquer.
// modo 2: pousado, mostra o valor final (e riscado se zerou).
static void desenha_dado(float cx, float cy, float r, int tipo, int dono,
                         float ang, int valor, int modo, bool zerado)
{
    int lados = TIPO[tipo].lados;
    const visual_t *vi = &VISUAL[tipo];
    uint16_t corpo = dono == 0 ? C_MARFIM_D : C_EBANO;
    uint16_t tinta = dono == 0 ? C_EBANO : C_MARFIM;
    uint16_t aro = aro_de(tipo);
    if (vi->tom) corpo = cor_de_dado(gfx_mistura(corpo, vi->tom, dono == 0 ? vi->pct : vi->pct - 12));
    if (tipo == D_D2) corpo = cor_de_dado(gfx_mistura(corpo, C_OURO, dono == 0 ? 58 : 46));   // moeda dourada
    if (tipo == D_VIUVA) {                        // preta para os dois, com a tinta clara
        corpo = dono == 0 ? RGB(62, 56, 50) : RGB(24, 22, 20);
        tinta = C_MARFIM;
    }
    if (tipo == D_VICIADO) tinta = C_VERMELHO;   // pontos de cassino
    if (zerado) {                        // anulado: o dado fica, apagado
        corpo = dono == 0 ? C_MARFIM_S : C_EBANO_B;
        tinta = dono == 0 ? C_EBANO_B : C_MARFIM_S;
        aro = C_FELTRO2;
    }
    // Os tons do desenho de dentro e os do corpo; anulado, o desenho fica num
    // cinza so e o corpo, sem luz.
    uint16_t cores[4] = { corpo, corpo, corpo, corpo }, tons[5] = { corpo, corpo, corpo, corpo, corpo };
    if (zerado) {
        cores[1] = cores[2] = cores[3] = cor_de_dado(gfx_mistura(corpo, tinta, 18));
    } else {
        cores[1] = tom_desenho(corpo, vi->c1, vi->p1, dono);
        cores[2] = tom_desenho(corpo, vi->c2, vi->p2, dono);
        cores[3] = tom_desenho(corpo, vi->c3, vi->p3, dono);
        tons_do_corpo(corpo, tons);
    }
    bool desenha = r >= 12;
    float ea = r < 12 ? 1 : r / 10;      // espessura do aro
    int so = r < 12 ? 2 : (int)(r / 6);  // deslocamento da sombra
    mca = cosf(ang); msa = sinf(ang); mcx = cx; mcy = cy;

    int alt = (int)sombra_alt;                    // dado no ar: sombra mais longe
    gfx_disco((int)cx + so + alt, (int)cy + so + 1 + alt, (int)(r - alt * 0.2f), C_SOMBRA);   // sombra no feltro
    if (tipo == D_FOGO) { ficha_de_fogo(cx, cy, r, ea, ang, valor, modo, zerado); return; }
    if (tipo == D_TUDO_NADA) { tudo_ou_nada(cx, cy, r, ang, modo == 0 ? -1 : valor, zerado); return; }

    // O aro, o brilho e o corpo por cima do brilho, um pouco deslocado: fica
    // uma luz na aresta de cima.
    uint16_t brilho = zerado ? corpo : gfx_mistura(corpo, C_BRANCO, dono == 0 ? 35 : 18);
    if (lados == 2) {
        int ri = (int)(r - ea);
        gfx_disco((int)cx, (int)cy, (int)(r + ea), aro);
        gfx_disco((int)cx, (int)cy, ri, brilho);
        if (desenha) pinta_corpo(NULL, NULL, 0, cx + 1, cy + 1, ri - 1, tipo, tons, cores, cx, cy, ang, r);
        else gfx_disco((int)cx + 1, (int)cy + 1, ri - 1, corpo);
    } else {
        float vx[12], vy[12], dx = ea * 0.8f;
        int n = forma(lados, cx, cy, r + ea, ang, vx, vy);
        poligono(vx, vy, n, aro);
        n = forma(lados, cx, cy, r - ea, ang, vx, vy);
        poligono(vx, vy, n, brilho);
        if (r >= 12) {
            n = forma(lados, cx + dx, cy + dx, r - ea * 1.6f, ang, vx, vy);
            if (desenha) pinta_corpo(vx, vy, n, 0, 0, 0, tipo, tons, cores, cx, cy, ang, r);
            else poligono(vx, vy, n, corpo);
        }
    }
    marca_por_cima(tipo, r, dono, corpo, zerado);

    char s[12];
    // Vidro que tirou 1: estilhacado, com rachaduras saindo do impacto.
    if (tipo == D_VIDRO && modo == 2 && valor == 1 && zerado) {
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
    if (modo == 0) {
        // Parado na mao mostra quantos lados tem - ou, no Quebrado, o numero
        // gravado nele, que e o que ele vai valer.
        if (tipo == D_QUEBRADO && valor) {
            snprintf(s, sizeof s, "%d", valor);
            numero(cx, cy, r, s, tinta);
            return;
        }
        // A tinta do dado a meio caminho do corpo: le-se bem, mas nao se
        // confunde com um valor tirado, que vem na tinta cheia.
        snprintf(s, sizeof s, "%d", lados);
        numero(cx, cy, r, s, gfx_mistura(tinta, corpo, 38));
        return;
    }
    if (valor < 0) return;               // so o corpo: o catalogo escreve grande
    if (lados == 6 && TIPO[tipo].efeito != EF_ESPELHO && valor >= 1 && valor <= 6
        && (!zerado || so_apagado)) {
        pips((int)cx, (int)cy, valor, (int)r, tinta);
    } else {
        snprintf(s, sizeof s, "%d", valor);
        numero(cx, cy, r, s, tinta);
    }
    if (zerado) risca(cx, cy, r);
}

// Moeda girando no ar: a arte de verdade da ficha (desenhada num rascunho)
// colada achatada na horizontal, pela largura aparente |escx|. Quando ela
// passa de lado, aparece a espessura.
#define CHAVE 0xF81F                                   // magenta: fundo do rascunho
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
        if (v->t_face <= 0) {
            v->face = face_v(v->tipo, v->valor);
            v->t_face = 0.04f + (1.0f - vel / 680.0f) * 0.12f;
        }
        if (vel < 36 || t_fase > 6.0f) {
            v->parado = true;
            v->pouso = 0.18f;
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

static void pinta_mesa(void)
{
    int lv = mesa_andar;
    gfx_clear(C_BARRA);
    int x0 = 4, y0 = MESA_Y0 - 8, x1 = GFX_W - 4, y1 = MESA_Y1 + 8;
    // Do andar do rubi em diante, a madeira e mogno.
    uint16_t mad = lv >= 4 ? gfx_mistura(C_MADEIRA, RGB(92, 26, 30), 45) : C_MADEIRA;
    gfx_rect(x0, y0, x1 - x0, y1 - y0, mad);
    for (int y = y0; y < y1; y += 2)                    // veios da madeira
        for (int x = x0; x < x1; x++)
            if (ruido_xy(x / 7, y) % 5 == 0) gfx_pixel(x, y, gfx_mistura(mad, C_SOMBRA, 30));
    gfx_rect(x0, y0, x1 - x0, 1, C_MADEIRA_CLR);
    // O metal do andar: latao no terreo, bronze, prata, e ouro dali para cima.
    uint16_t metal = lv == 1 ? C_BRONZE : lv == 2 ? C_PRATA : lv >= 3 ? C_OURO : C_LATAO;
    gfx_moldura(MESA_X0 - 4, MESA_Y0 - 4, MESA_X1 - MESA_X0 + 8, MESA_Y1 - MESA_Y0 + 8, 2,
                lv >= 3 ? C_OURO : C_LATAO);
    gfx_moldura(MESA_X0 - 2, MESA_Y0 - 2, MESA_X1 - MESA_X0 + 4, MESA_Y1 - MESA_Y0 + 4, 2, C_SOMBRA);
    float cx = (MESA_X0 + MESA_X1) / 2.0f, cy = (MESA_Y0 + MESA_Y1) / 2.0f;
    float ax = (MESA_X1 - MESA_X0) / 2.0f, ay = (MESA_Y1 - MESA_Y0) / 2.0f;
    // O feltro escurece para as bordas em tres tons da paleta, com pontilhado
    // ordenado (Bayer 4x4) na passagem de um para o outro, e uma fibra ou
    // outra mais clara ou mais escura.
    static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
    static const uint16_t TOM[3] = { C_FELTRO, C_FELTRO_MEIO, C_FELTRO_ESC };
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
            gfx_pixel(x, y, c);
        }
    // Linha de aposta: uma elipse tracejada ao redor do centro da mesa.
    uint16_t linha = lv >= 3 ? gfx_mistura(C_FELTRO2, C_OURO, 30) : C_FELTRO2;
    for (int i = 0; i < 360; i++) {
        if ((i / 4) % 2) continue;
        float a = i * 0.017453f;
        gfx_rect((int)(cx + cosf(a) * 250) , (int)(cy + sinf(a) * 52), 2, 1, linha);
    }
    if (lv <= 0) return;

    // Acabamentos do andar, discretos: so nas bordas.
    if (lv >= 2) filete(6, gfx_mistura(C_FELTRO, metal, 38), lv >= 3);
    if (lv >= 5) filete(9, gfx_mistura(C_FELTRO, metal, 22), true);
    int fx0 = MESA_X0 - 3, fx1 = MESA_X1 + 2, fy0 = MESA_Y0 - 3, fy1 = MESA_Y1 + 2;
    int my = (MESA_Y0 + MESA_Y1) / 2;
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

static void mesa(void)
{
    mesa_andar = modo == M_DESAFIO ? des_dificuldade() : 0;
    if (mesa_andar != mesa_andar_feito) { mesa_feita = false; mesa_andar_feito = mesa_andar; }
    if (!mesa_feita) {
        pinta_mesa();
        memcpy(mesa_pronta, display_fb(), sizeof mesa_pronta);
        mesa_feita = true;
    }
    memcpy(display_fb(), mesa_pronta, sizeof mesa_pronta);
}

static void ficha(int x, int y, uint16_t c)
{
    gfx_disco(x + 1, y + 2, 7, C_SOMBRA);
    gfx_disco(x, y, 7, c);
    for (int i = 0; i < 6; i++) {                        // listras da borda
        float a = i * 1.0472f;
        gfx_rect(x + (int)(cosf(a) * 5.5f) - 1, y + (int)(sinf(a) * 5.5f) - 1, 2, 2, C_MARFIM);
    }
    gfx_disco(x, y, 3, gfx_mistura(c, C_BARRA, 30));
}
