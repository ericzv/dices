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
// O dado em curso e o embrulho do Papai Noel: a caixa mostra "?", nao os
// lados do Presente. Quem desenha avisa; o desenho consome.
static bool caixa_fechada;
// As frutas do Panetone na fatia desta vez (-1: na mao, ainda nao cortada;
// mostra as tres).
static int frutas_desta = -1;
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
    case D_JINGLE: {                              // o guizo: a esfera dourada, a fenda embaixo e o laco vermelho
        float x = u, y = v - 0.06f, rr = sqrtf(x * x + y * y);
        float w = 0.035f + l * 0.6f;
        if (fabsf(sqrtf(x * x + (v + 0.66f) * (v + 0.66f)) - 0.07f) < w) return 2;          // a argola
        for (int k = -1; k <= 1; k += 2) {                                                   // o laco, um de cada lado
            float bx = u - k * 0.17f, by = v + 0.62f;
            if (bx * bx / (0.11f * 0.11f) + by * by / (0.065f * 0.065f) < 1) return 3;
        }
        if (rr < 0.58f) {
            float bx = x + 0.24f, by = y + 0.24f;                                            // o brilho, no alto a esquerda
            if (bx * bx + by * by < 0.10f * 0.10f) return 0;
            // a fenda: uma linha com um furo em cada ponta, e o corte ate a boca
            if (fabsf(y - 0.30f) < w && fabsf(x) < 0.30f) return 2;
            if ((fabsf(x) - 0.30f) * (fabsf(x) - 0.30f) + (y - 0.30f) * (y - 0.30f) < 0.06f * 0.06f) return 2;
            if (fabsf(x) < w && y > 0.30f) return 2;
            if (rr > 0.58f - w * 1.6f) return 2;                                             // o contorno
            if (fabsf(y + 0.05f) < w * 0.7f) return 2;                                       // o friso do meio
            return 1;
        }
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
    case D_PRESENTE: {                            // a caixa embrulhada: a fita em cruz, o laco em cima e o papel listrado
        float w = 0.075f + l * 0.5f;
        for (int k = -1; k <= 1; k += 2) {                                                   // o laco: duas alcas e as pontas
            float bx = u - k * 0.17f, by = v + 0.54f;
            float e = bx * bx / (0.15f * 0.15f) + by * by / (0.09f * 0.09f);
            if (e < 1) return e < 0.28f ? 1 : 2;
            if (dist_seg(u, v, k * 0.03f, -0.50f, k * 0.18f, -0.34f) < w * 0.6f) return 2;   // as pontas
        }
        if (u * u + (v + 0.52f) * (v + 0.52f) < 0.07f * 0.07f) return 2;                    // o no
        if (fabsf(u) < w || fabsf(v - 0.04f) < w) return 2;                                   // a fita em cruz
        return fr((u + v) / 0.30f) < 0.12f ? 1 : 0;                                           // o papel, em listras
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
    case D_MEIA: {                                // a meia pendurada: o punho branco, o pe para a direita e o que tem dentro
        int g = meia_desta();
        int moedas = g <= 0 ? 0 : g < 4 ? 1 : g < 8 ? 2 : 3;
        static const float MX[3] = { -0.10f, -0.26f, 0.06f }, MY[3] = { -0.52f, -0.46f, -0.46f };
        for (int i = 0; i < moedas; i++) {                                                   // as moedas saindo pelo punho
            float x = u - MX[i], y = v - MY[i];
            float d = x * x + y * y;
            if (d < 0.085f * 0.085f) return d < 0.04f * 0.04f ? 2 : 3;
        }
        if (v > -0.40f && v < -0.24f && u > -0.36f && u < 0.16f) return 2;                  // o punho
        if (u > -0.30f && u < 0.10f && v >= -0.24f && v < 0.44f) {                           // a perna
            return fabsf(v - 0.06f) < 0.03f + l ? 2 : 1;                                     // com uma listra
        }
        float px = (u - 0.10f) / 0.40f, py = (v - 0.48f) / 0.17f;                             // o pe, com o calcanhar e a ponta brancos
        if (u > -0.30f && u <= 0.10f && v >= 0.44f && v < 0.65f) return u < -0.16f && v > 0.52f ? 2 : 1;
        if (u > 0.10f && px * px + py * py < 1) return px > 0.62f ? 2 : 1;
        return 0;
    }
    case D_ESTRELA: {                             // a estrela no alto, com quatro pontas longas, e a cauda de luz descendo
        float x = u, y = v + 0.46f, ax = fabsf(x), ay = fabsf(y);
        // as pontas: losangos finos nas quatro direcoes, e quatro curtas nas diagonais
        if (ax < 0.10f * (1 - ay / 0.40f) && ay < 0.40f) return ay < 0.10f ? 2 : 1;
        if (ay < 0.10f * (1 - ax / 0.34f) && ax < 0.34f) return ax < 0.10f ? 2 : 1;
        float dx = (x + y) * 0.7071f, dy = (x - y) * 0.7071f;
        if ((fabsf(dx) < 0.05f * (1 - fabsf(dy) / 0.18f) && fabsf(dy) < 0.18f)
            || (fabsf(dy) < 0.05f * (1 - fabsf(dx) / 0.18f) && fabsf(dx) < 0.18f)) return 1;
        if (v > -0.06f) {                                                                    // a cauda, abrindo para baixo
            float meia = 0.03f + (v + 0.06f) * 0.16f;
            if (ax < meia && fr(v / 0.12f) < 0.55f) return 2;
        }
        static const float PX[4] = { -0.52f, 0.50f, 0.34f, -0.36f }, PY[4] = { 0.06f, -0.06f, 0.42f, 0.48f };
        for (int i = 0; i < 4; i++) {                                                        // estrelinhas em volta
            float px = u - PX[i], py = v - PY[i];
            if ((fabsf(px) < l * 1.4f + 0.012f && fabsf(py) < 0.05f) || (fabsf(py) < l * 1.4f + 0.012f && fabsf(px) < 0.05f)) return 3;
        }
        return 0;
    }
    case D_RENA: {                               // a rena: a galhada abrindo nos cantos de cima e o nariz vermelho
        float w = 0.032f + l * 0.6f, au = fabsf(u);
        // cada galho: a haste subindo para fora, com pontas para cima e para o lado
        if (dist_seg(au, v, 0.12f, -0.26f, 0.26f, -0.58f) < w || dist_seg(au, v, 0.17f, -0.38f, 0.40f, -0.44f) < w
            || dist_seg(au, v, 0.40f, -0.44f, 0.50f, -0.58f) < w || dist_seg(au, v, 0.40f, -0.44f, 0.58f, -0.38f) < w
            || dist_seg(au, v, 0.22f, -0.50f, 0.36f, -0.62f) < w) return 2;
        float nx = u, ny = v - 0.40f, nd = nx * nx + ny * ny;                                 // o nariz, com um brilho
        if (nd < 0.12f * 0.12f) return (nx + 0.04f) * (nx + 0.04f) + (ny + 0.04f) * (ny + 0.04f) < 0.032f * 0.032f ? 0 : 3;
        float mx = u / 0.30f, my = (v - 0.38f) / 0.20f;                                       // o focinho
        if (mx * mx + my * my < 1) return 1;
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
// Panetone e o miolo dele, o Boneco de Neve e a neve batida com a cara e o
// cachecol. corpo_desta diz qual (0: nenhum); apagado, so o que e desenho
// (passas, frutas, carvao, cachecol) fica, num cinza so.
static int corpo_desta;
static bool corpo_apagado;

// O miolo do Panetone no ponto (u, v) do dado, em raios, girando com ele: a
// massa dourada com os alveolos compridos (a massa sobe e estica), as passas
// e as frutas cristalizadas da fatia (frutas_desta; na mao, as tres). nivel:
// a luz do corpo ali (-2 a 2). Devolve a cor, ou c (o corpo) onde nao muda.
static const uint16_t MIOLO[] = {
    0, RGB(228, 194, 106), RGB(190, 156, 86),                              // (0: a massa, que e o corpo), o alveolo e o fundo dele
    RGB(120, 78, 50), RGB(78, 46, 30),                                    // a passa e a sombra dela
    RGB(238, 162, 74), RGB(214, 112, 44),                                 // a laranja cristalizada
    RGB(208, 98, 108), RGB(190, 56, 60),                                  // a cereja
    RGB(130, 204, 112), RGB(88, 152, 84),                                 // a cidra
};
static uint16_t miolo_panetone(float u, float v, float s, int nivel, uint16_t c, uint16_t cinza)
{
    // as frutas da fatia: cubinhos tortos, a beira de cima mais clara (o
    // giro de cada um ja em cosseno e seno; so perto dele se faz a conta)
    static const float FX[3] = { -0.44f, 0.42f, 0.02f }, FY[3] = { 0.02f, -0.06f, 0.46f };
    static const float FC[3] = { 0.921f, 0.955f, 0.622f }, FS[3] = { 0.389f, -0.296f, 0.783f };
    int n = frutas_desta < 0 ? 3 : frutas_desta;
    for (int i = 0; i < n && i < 3; i++) {
        float x = u - FX[i], y = v - FY[i];
        if (x * x + y * y > 0.15f * 0.15f) continue;
        float a = x * FC[i] - y * FS[i], b = x * FS[i] + y * FC[i], lado = 0.085f + 0.012f * ruido(a * 30 + i, b * 30);
        if (fabsf(a) < lado && fabsf(b) < lado * 0.85f) {
            if (corpo_apagado) return cinza;
            bool luz = a < -lado + 0.035f || b < -lado * 0.85f + 0.035f;
            return MIOLO[5 + i * 2 + (luz ? 0 : 1)];
        }
    }
    // as passas: gotas escuras e enrugadas, cada uma num rumo
    static const float PX[9] = { -0.12f, 0.24f, -0.30f, 0.10f, -0.20f, 0.30f, -0.52f, 0.54f, -0.02f };
    static const float PY[9] = { -0.54f, -0.32f, -0.24f, 0.18f, 0.30f, 0.26f, -0.10f, 0.14f, -0.16f };
    static const float PC[9] = { 1, 0.199f, -0.920f, -0.567f, 0.694f, 0.844f, -0.358f, -0.986f, -0.036f };
    static const float PS[9] = { 0, 0.980f, 0.391f, -0.824f, -0.720f, 0.537f, 0.934f, -0.164f, -0.999f };
    for (int i = 0; i < 9; i++) {
        float x = u - PX[i], y = v - PY[i];
        if (x * x + y * y > 0.12f * 0.12f) continue;
        float a = x * PC[i] - y * PS[i], b = x * PS[i] + y * PC[i];
        float e = a * a / (0.072f * 0.072f) + b * b / (0.052f * 0.052f);
        if (e < 1 + 0.55f * ruido(a * 28 + i * 7, b * 28 + i)) {
            if (corpo_apagado) return cinza;
            return e > 0.45f && a < -0.01f && b < 0.01f ? MIOLO[3] : MIOLO[4];            // a borda de cima pega luz
        }
    }
    if (corpo_apagado) return c;
    // a massa: alveolos tortos e compridos (a massa sobe e estica), dos
    // grandes aos poros miudos, e a sombra do canto
    float w1 = ruido(u * 3 + 11, v * 3 + 3), w2 = ruido(u * 3 + 5, v * 3 + 17);
    float a = ruido((u + 0.12f * w1) * 10 + 3, (v + 0.12f * w2) * 4.2f + 7);
    float b = ruido(u * 21 + 9, v * 9 + 1);
    float m = a + 0.5f * b;
    if (m > 0.98f) return MIOLO[2];
    if (m > 0.58f) return MIOLO[1];
    if (hash2((int)floorf(u / (1.4f * s)) + 300, (int)floorf(v / (1.4f * s)) + 300) % 13 == 0) return MIOLO[1];
    if (nivel <= -2) return MIOLO[1];
    return c;
}

// A neve do Boneco no ponto (u, v) do dado, em raios, girando com ele: neve
// batida, em torroes com a sombra azulada, e uns brilhos que piscam; a cara
// de carvao e a cenoura em volta do numero, e o cachecol de trico embaixo.
static const uint16_t NEVE[] = {
    RGB(184, 194, 208), RGB(168, 204, 230), 0,                             // a sombra do torrao e o brilho (2: sem uso)
    RGB(36, 32, 29), RGB(96, 88, 80),                                     // o carvao e o brilho dele
    RGB(238, 162, 74), RGB(214, 112, 44),                                 // a cenoura
    RGB(190, 56, 60), RGB(118, 30, 38), RGB(252, 248, 236),              // o cachecol: o vermelho, o ponto e a listra
};
static uint16_t neve_boneco(float u, float v, float s, int nivel, uint16_t c, uint16_t cinza)
{
    // o cachecol no pescoco, embaixo: a volta de trico vermelha, com as
    // listras brancas, e a ponta caindo pela frente, com a franja
    float base = 0.42f + 0.05f * (u / 0.4f) * (u / 0.4f), y = v - base;
    bool faixa = y > 0 && y < 0.16f;
    float px = u + 0.07f - (v - 0.50f) * 0.18f;                         // a ponta, meio torta
    bool ponta = v > 0.50f && v < 0.72f && fabsf(px) < 0.075f;
    bool franja = v >= 0.72f && v < 0.79f && fabsf(px) < 0.075f;
    if (faixa || ponta || franja) {
        if (corpo_apagado) return cinza;
        if (franja) return fr(px / 0.035f) < 0.5f ? NEVE[7] : c;
        if (ponta && (y >= 0.16f || y <= 0)) {
            if (fabsf(fabsf(px) - 0.075f) < s * 0.9f) return NEVE[8];                 // a beira da ponta
            float ly = v - base - 0.16f;
            if (fabsf(ly - 0.06f) < 0.018f + s * 0.4f) return NEVE[9];               // a listra
            return NEVE[7];
        }
        if (ponta && fabsf(fabsf(px) - 0.075f) < s * 0.9f) return NEVE[8];           // a ponta por cima da volta
        if (y < s * 1.2f || y > 0.16f - s * 1.2f) return NEVE[8];                    // a beira da volta
        if (s < 0.05f) {                                                     // grande: os pontos do trico, em V
            float sx = fr(u / 0.06f), sy = fr(y / 0.04f);
            if (fabsf(fabsf(sx - 0.5f) - (1 - sy) * 0.5f) < 0.07f) return NEVE[8];
        }
        return NEVE[7];
    }
    // a cara de carvao: os olhos acima do numero e o sorriso de pedrinhas embaixo
    static const float CX[7] = { -0.16f, 0.16f, -0.20f, -0.10f, 0.0f, 0.10f, 0.20f };
    static const float CY[7] = { -0.54f, -0.54f, 0.14f, 0.21f, 0.235f, 0.21f, 0.14f };
    for (int i = 0; i < 7; i++) {
        float x = u - CX[i], y2 = v - CY[i], rr = i < 2 ? 0.065f : 0.032f + s * 0.4f, d = x * x + y2 * y2;
        if (d < rr * rr * 1.3f && d < rr * rr * (1 + 0.3f * ruido(x * 50 + i, y2 * 50))) {
            if (corpo_apagado) return cinza;
            return i < 2 && x < -0.01f && y2 < -0.01f ? NEVE[4] : NEVE[3];
        }
    }
    // a cenoura: sai de tras do numero para a direita, afinando, com os riscos
    float cy = v + 0.18f;
    if (u > 0.06f && u < 0.48f && fabsf(cy) < 0.075f * (0.48f - u) / 0.42f + s * 0.3f) {
        if (corpo_apagado) return cinza;
        return cy > 0.012f || fabsf(fr(u / 0.09f) - 0.5f) < 0.08f ? NEVE[6] : NEVE[5];
    }
    if (corpo_apagado) return c;
    // a neve batida: torroes de tamanhos tortos, so alguns, cada um com um
    // risco de sombra embaixo, a direita (a luz vem de cima, a esquerda), e os
    // brilhos que acendem e apagam
    const float cel = 0.30f;
    int gx = (int)floorf(u / cel), gy = (int)floorf(v / cel);
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++) {
            uint32_t q = hash2(gx + i + 50, gy + j + 80);
            if (q % 100 < 60) continue;
            float ox = (gx + i + 0.15f + (q >> 7 & 255) / 255.0f * 0.7f) * cel;
            float oy = (gy + j + 0.15f + (q >> 15 & 255) / 255.0f * 0.7f) * cel;
            float rr = cel * (0.30f + (q >> 23 & 255) / 255.0f * 0.45f);
            float x = u - ox, y2 = v - oy, e = s * 1.2f + 0.012f;
            float d = x * x + y2 * y2;
            if (d < rr * rr && (x + e) * (x + e) + (y2 + e) * (y2 + e) > rr * rr && x + y2 > rr * 0.75f)
                return NEVE[0];
        }
    uint32_t h = hash2((int)floorf(u / s) + 700, (int)floorf(v / s) + 700);
    if (h % 61 == 0 && (h >> 8) % 4 == ((int)(t_relogio * 2) & 3)) return NEVE[1];   // o brilho
    if (h % 43 == 1) return NEVE[0];                                      // o grao
    if (nivel <= -2) return NEVE[0];
    return c;
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
                float dx = x - cx, dy = y - cy, u = dx * ca - dy * sa, v = dx * sa + dy * ca;
                int p = desenho_interno(tipo, u * s, v * s, s);
                uint16_t c;
                if (p) c = cores[p];
                else {
                    float hx = dx * s + 0.34f, hy = dy * s + 0.38f;           // o reflexo
                    float reflexo = mat_desta == MAT_BRONZE ? 2.1f : mat_desta == MAT_BOLA ? 2.6f   // o metal e o espelho
                                  : mat_desta == MAT_VELUDO || mat_desta == MAT_BISCOITO ? 0.7f : 1.6f;  // foscos
                    float luz = -(dx + dy) * s * 0.75f
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
                        luz += 0.32f * ruido(u * s / 0.34f + desvio, v * s / 0.34f)
                             + 0.14f * ruido(u * s / 0.15f, v * s / 0.15f + desvio);
                    }
                    if (padrao(tex, u, v, r / 22)) luz -= 1;
                    int nivel = (int)lroundf(luz);
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

// O adesivo do lado que esta para cima: quem desenha o dado na mesa avisa
// antes (como o brilho de passagem), e a etiqueta vai na face, por baixo do
// numero. ad_face_aceso: o halo de quando ela acende (rolando, ao passar pelo
// lado; pousando nele; agindo no fim).
static int  ad_face;
static bool ad_face_aceso;

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
            gfx_disco((int)cx, (int)cy, rr, fundo);
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
        n = forma(lados, cx, cy + oy, r * k, ang, vx, vy); poligono(vx, vy, n, fundo);
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

static void desenha_dado(float cx, float cy, float r, int tipo, int dono,
                         float ang, int valor, int vista, bool zerado)
{
    brilho_desta = brilho_passa;                  // o brilho de passagem e so deste dado
    brilho_passa = -1;
    bool caixa = caixa_fechada && tipo == D_PRESENTE;
    caixa_fechada = false;
    int adf = vista && valor != SO_CORPO && !face_escondida ? ad_face : 0;   // a etiqueta e so deste dado
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
    corpo_desta = tipo == D_PANETONE || tipo == D_BONECO ? tipo : 0;
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
    if (tipo == D_SACRIFICIO) tinta = C_MARFIM;   // o corpo vinho pede a tinta clara
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
    gfx_disco((int)cx + so + alt, (int)cy + so + 1 + alt, (int)(r - alt * 0.2f), C_SOMBRA);   // sombra no feltro
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
    marca_por_cima(tipo, r, corpo, zerado);

    char s[12];
    // Vidro que tirou 1: estilhacado, com rachaduras saindo do impacto.
    if (tipo == D_VIDRO && vista == 2 && valor == 1 && zerado) {
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
        // A tinta do dado a meio caminho do corpo: le-se bem, mas nao se
        // confunde com um valor tirado, que vem na tinta cheia.
        snprintf(s, sizeof s, "%d", lados);
        numero(cx, cy, r, s, gfx_mistura(tinta, corpo, 38));
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
    uint16_t contorno = 0;               // o numero sobre o simbolo ganha um contorno da cor do vinil
    if (adf) {                           // o adesivo do lado de cima: a etiqueta, e o numero na tinta dela
        ad_etiqueta(cx, cy, r, lados, ang, adf, adf_aceso, zerado);
        if (ADESIVO[adf].fundo != AD_SEM_FUNDO) {
            tinta = zerado ? gfx_mistura(ADESIVO[adf].tinta, C_FELTRO2, 60) : ADESIVO[adf].tinta;
            if (r >= 12) contorno = zerado ? gfx_mistura(ADESIVO[adf].fundo, C_FELTRO2, 60) : ADESIVO[adf].fundo;
        }
    }
    if (lados == 6 && TIPO[tipo].efeito != EF_ESPELHO && valor >= 1 && valor <= 6
        && (!zerado || so_apagado)) {
        pips((int)cx, (int)cy, valor, (int)r, tinta);
    } else {
        snprintf(s, sizeof s, sr_lado ? (sr_lado == 1 ? "+%d" : "-%d") : "%d", valor);
        if (contorno) {
            static const int8_t D[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {1,-1}, {-1,1}, {1,1} };
            for (int i = 0; i < 8; i++) numero(cx + D[i][0], cy + D[i][1], r, s, contorno);
        }
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
    int x0 = 4, y0 = MESA_Y0 - 8, x1 = GFX_W - 4, y1 = MESA_Y1 + 8;
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
    gfx_rect(x0, y0, x1 - x0, 1, propria ? pm[2] : C_MADEIRA_CLR);
    uint16_t metal = propria ? MESAS[estilo_mesa].metal
                             : lv == 1 ? C_BRONZE : lv == 2 ? C_PRATA : lv >= 3 ? C_OURO : C_LATAO;
    bool led = estilo_mesa == MS_LED;
    // Sem moldura de metal: a fita de led, a brasa e a fita do pinho.
    bool sem_metal = led || estilo_mesa == MS_LAREIRA || estilo_mesa == MS_PINHO;
    if (led) fita_led(led_forca(0));                    // a moldura e a fita (mesa_anima a faz respirar)
    else if (estilo_mesa == MS_LAREIRA) fita_brasa(led_forca(0));
    else if (estilo_mesa == MS_PINHO) fita_pinho();
    else {
        gfx_moldura(MESA_X0 - 4, MESA_Y0 - 4, MESA_X1 - MESA_X0 + 8, MESA_Y1 - MESA_Y0 + 8, 2,
                    propria ? metal : lv >= 3 ? C_OURO : C_LATAO);
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

static void mesa(void)
{
    mesa_andar = modo == M_DESAFIO ? (des_dificuldade() + 1) / 2 : 0;   // um acabamento a cada dois andares
    if (mesa_andar != mesa_andar_feito) { mesa_feita = false; mesa_andar_feito = mesa_andar; }
    static int versao_feita = -1;                        // trocou o feltro ou a mesa: pinta de novo
    if (estilo_versao != versao_feita) { mesa_feita = false; versao_feita = estilo_versao; }
    static int quadro_feito = -1;
    if (!mesa_feita) {
        pinta_mesa();
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
