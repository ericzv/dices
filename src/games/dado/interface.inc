// ===========================================================================
// Kit de interface: as mesmas pecas em todo o jogo, no desenho dos dados.
//   ui_painel  placa de ebano com moldura de latao (avisos sobre a mesa)
//   ui_cartao  cartao de feltro escuro (loja, premio, perfis, listas)
//   ui_botao   botao com espessura: face em dois tons, beira de baixo, apertado
//   ui_cantos  a selecao: quatro cantoneiras de latao que respiram
// Como nos dados: contorno quase preto de um pixel, metal em rampa com a luz
// de cima a esquerda (o pontilhado so na passagem) e sombra com a beira
// pontilhada. Cantos aparados de um pixel.
// ===========================================================================
#define C_PLACA     RGB(24, 22, 20)              // fundo das placas
#define C_PLACA_LUZ RGB(36, 32, 29)
static float bayer(int x, int y);                 // desenho_dado.inc: o pontilhado
static void rampa_de(uint16_t c, uint16_t *r);    // e a rampa de 4 tons de uma cor

// Contorno de um pixel com os quatro cantos aparados.
static void ui_contorno(int x, int y, int w, int h, uint16_t c)
{
    gfx_rect(x + 1, y, w - 2, 1, c);
    gfx_rect(x + 1, y + h - 1, w - 2, 1, c);
    gfx_rect(x, y + 1, 1, h - 2, c);
    gfx_rect(x + w - 1, y + 1, 1, h - 2, c);
}

// A sombra de uma peca de w x h: dois pixels a direita e tres embaixo, com o
// miolo cheio e a ultima coluna e a ultima linha pontilhadas.
static void ui_sombra(int x, int y, int w, int h)
{
    int sx = x + 2, sy = y + 3;
    gfx_rect(sx, sy, w - 1, h - 1, C_SOMBRA);
    for (int i = 0; i < w; i++)
        if (bayer(sx + i, sy + h - 1) < 0.5f) gfx_pixel(sx + i, sy + h - 1, C_SOMBRA);
    for (int j = 0; j < h - 1; j++)
        if (bayer(sx + w - 1, sy + j) < 0.5f) gfx_pixel(sx + w - 1, sy + j, C_SOMBRA);
}

// Moldura de metal de 'e' pixels por dentro do retangulo, na rampa r (4 tons
// do escuro ao claro): cada beira num tom (k: 0..3; cima, esquerda, direita,
// baixo). Com 2 pixels ou mais, nos cantos em que duas beiras se encontram a
// passagem de uma a outra e pontilhada, como no aro dos dados; com 1 pixel o
// pontilhado viraria tracejado, e o canto so troca de tom.
static void ui_aro(int x, int y, int w, int h, int e, const uint16_t *r, const float *k)
{
    int K = (w < h ? w : h) / 3;
    if (K > 8) K = 8;
    if (K < 2 || e < 2) K = 0;
    if (e == 1) {                                 // 1 pixel: quatro filetes, os cantos com a beira de cima e a de baixo
        gfx_rect(x, y, w, 1, r[(int)k[0]]);
        gfx_rect(x, y + h - 1, w, 1, r[(int)k[3]]);
        gfx_rect(x, y + 1, 1, h - 2, r[(int)k[1]]);
        gfx_rect(x + w - 1, y + 1, 1, h - 2, r[(int)k[2]]);
        return;
    }
    for (int v = 0; v < h; v++)
        for (int u = 0; u < w; u++) {
            int dt = v, db = h - 1 - v, dl = u, dr = w - 1 - u;
            int dh = dt < db ? dt : db, dv = dl < dr ? dl : dr;
            if (dh >= e && dv >= e) { u = w - 1 - e; continue; }   // o meio: pula para a outra beira
            float t;
            if (dh <= dv) {                       // beira de cima ou de baixo
                float b = dt <= db ? k[0] : k[3];
                float a = dl <= dr ? k[1] : k[2];   // a beira do lado mais perto
                int d = dv;
                t = d >= K ? b : b + ((a + b) / 2 - b) * (1 - (float)d / K);
            } else {                              // beira da esquerda ou da direita
                float b = dl <= dr ? k[1] : k[2];
                float a = dt <= db ? k[0] : k[3];
                int d = dh;
                t = d >= K ? b : b + ((a + b) / 2 - b) * (1 - (float)d / K);
            }
            int i = (int)floorf(t);
            if (t - i > bayer(x + u, y + v)) i++;
            gfx_pixel(x + u, y + v, r[i < 0 ? 0 : i > 3 ? 3 : i]);
        }
}

// Os tons das beiras (cima, esquerda, direita, baixo): a cor em cima e a
// esquerda, um e dois tons abaixo do lado da sombra. Apagado (fora de foco,
// ja feito) e a mesma moldura, dois tons abaixo.
static const float UI_VIVO[4] = { 2, 2, 1, 0 };
static void ui_rampa(uint16_t cor, bool apagada, uint16_t *r)
{
    rampa_de(cor, r);
    if (apagada) rampa_de(r[0], r);
}

// O fundo com a luz de cima: as primeiras linhas pontilhadas no tom 'luz'.
static void ui_fundo(int x, int y, int w, int h, uint16_t c, uint16_t luz)
{
    gfx_rect(x, y, w, h, c);
    int n = h < 12 ? h / 2 : 6;
    for (int j = 0; j < n; j++) {
        float q = 0.75f * (1 - (float)j / n);
        for (int i = 0; i < 4 && i < w; i++)      // a matriz e de 4: um pixel aceso a cada 4, em passos
            if (bayer(x + i, y + j) < q)
                for (int u = i; u < w; u += 4) gfx_pixel(x + u, y + j, luz);
    }
}

static void ui_painel(int x, int y, int w, int h)
{
    uint16_t r[4];
    rampa_de(C_LATAO, r);
    ui_sombra(x, y, w, h);
    ui_fundo(x + 2, y + 2, w - 4, h - 4, C_PLACA, C_PLACA_LUZ);
    ui_contorno(x, y, w, h, C_BARRA);
    ui_aro(x + 1, y + 1, w - 2, h - 2, 1, r, UI_VIVO);
}

// A selecao: quatro cantoneiras de 2 pixels na rampa da cor (0: latao), com
// contorno, 'fora' pixels por fora do retangulo, respirando um pixel.
static void ui_cantos_em(int x, int y, int w, int h, uint16_t cor, int fora)
{
    uint16_t r[4];
    rampa_de(cor ? cor : C_OURO, r);
    int L = (w < h ? w : h) / 4;
    if (L > 9) L = 9;
    if (L < 4) L = 4;
    int x0 = x - fora, y0 = y - fora, x1 = x + w - 1 + fora, y1 = y + h - 1 + fora;
    static const int8_t TOM[4] = { 3, 2, 2, 1 };   // a luz de cima, a esquerda
    for (int c = 0; c < 4; c++) {
        int cx = c & 1 ? x1 - 1 : x0, cy = c & 2 ? y1 - 1 : y0;   // o quadrado 2x2 do canto
        int hx = c & 1 ? x1 - L + 1 : x0, vy = c & 2 ? y1 - L + 1 : y0;
        gfx_rect(hx - 1, cy - 1, L + 2, 4, C_BARRA);               // o contorno
        gfx_rect(cx - 1, vy - 1, 4, L + 2, C_BARRA);
        gfx_rect(hx, cy, L, 2, r[TOM[c]]);
        gfx_rect(cx, vy, 2, L, r[TOM[c]]);
    }
}
static int ui_respira(void) { return sinf(t_relogio * 5.0f) > 0 ? 1 : 0; }
static void ui_cantos(int x, int y, int w, int h, uint16_t cor) { ui_cantos_em(x, y, w, h, cor, 2 + ui_respira()); }

// 'cor' (0: latao) e o metal da moldura. Aceso, o cartao que se mostra (a
// bolsa aberta, a peca da colecao); apagado, o que espera.
static void ui_cartao_aceso(int x, int y, int w, int h, bool aceso, uint16_t cor)
{
    if (!cor) cor = C_LATAO;
    uint16_t r[4];
    ui_rampa(cor, !aceso, r);
    ui_sombra(x, y, w, h);
    gfx_rect(x + 2, y + 2, w - 4, h - 4, aceso ? C_FELTRO : C_FELTRO_ESC);
    ui_contorno(x, y, w, h, C_BARRA);
    ui_aro(x + 1, y + 1, w - 2, h - 2, 1, r, UI_VIVO);
}

// O cartao de uma escolha: o do cursor acende e ganha as cantoneiras
// (sempre de latao: a cor e da coisa).
static void ui_cartao(int x, int y, int w, int h, bool foco, uint16_t cor)
{
    ui_cartao_aceso(x, y, w, h, foco, cor);
    if (foco) ui_cantos_em(x, y, w, h, C_OURO, ui_respira());
}

// ---------------------------------------------------------------------------
// Numeros flutuantes: o que um dado acabou de valer, perder ou multiplicar
// sobe de cima dele e some. Dizem o resultado sem frase.
// ---------------------------------------------------------------------------
// 'y' e a beira de onde ele sai; 'dir' -1 sobe (fila de baixo), +1 desce
// (fila de cima): sempre para o meio da mesa.
typedef struct { int16_t x, y; int8_t dir; float t, atraso; char s[12]; uint16_t cor; } flutua_t;
#define MAX_FLUT 16
#define FLUT_T 0.95f
static flutua_t flut[MAX_FLUT];
static int n_flut;

static void flutua(int x, int y, int dir, uint16_t cor, float atraso, const char *fmt, int v)
{
    if (n_flut >= MAX_FLUT) return;
    flutua_t *f = &flut[n_flut++];
    f->x = (int16_t)x; f->y = (int16_t)y; f->dir = (int8_t)dir; f->t = 0; f->atraso = atraso; f->cor = cor;
    snprintf(f->s, sizeof f->s, fmt, v);
}

static void flutua_passo(float dt)
{
    int n = 0;
    for (int i = 0; i < n_flut; i++) {
        if (flut[i].atraso > 0) flut[i].atraso -= dt;
        else flut[i].t += dt;
        if (flut[i].t < FLUT_T) flut[n++] = flut[i];
    }
    n_flut = n;
}

static void desenha_flutua(void)
{
    for (int i = 0; i < n_flut; i++) {
        const flutua_t *f = &flut[i];
        if (f->atraso > 0) continue;
        float q = f->t / FLUT_T, sobe = 1 - (1 - q) * (1 - q);    // sai rapido e assenta
        // Na escala 2 a cifra vai da linha 10 (topo) a 30 (base) do glifo.
        int y = f->dir < 0 ? f->y - 2 * FONTE_BASE - (int)(sobe * 14)
                           : f->y - 10 + (int)(sobe * 14);
        // No fim ele escurece em dois passos, em cores da paleta, ate sumir.
        uint16_t c = q < 0.7f ? f->cor : q < 0.85f ? gfx_mistura(f->cor, C_FELTRO, 45) : gfx_mistura(f->cor, C_FELTRO, 75);
        int x = f->x - gfx_largura(f->s, 2) / 2;
        static const int8_t CONT[8][2] = { {-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {1,-1}, {-1,1}, {1,1} };
        for (int d = 0; d < 8; d++) gfx_texto(x + CONT[d][0], y + CONT[d][1], f->s, C_BARRA, 2, false);
        gfx_texto(x, y, f->s, c, 2, false);
    }
}

// ---------------------------------------------------------------------------
// Teclas desenhadas: "ENTER senta  ·  TAB bolsa" vira [ENTER] senta  [TAB] bolsa.
// Cada trecho entre " · " comeca pelas teclas (palavras em maiusculas, < > -
// + ou "setas"), ligadas por "ou"/"e"; o resto e o que ela faz.
// ---------------------------------------------------------------------------
#define TECLA_H 17
static bool eh_tecla(const char *p, int n)
{
    if (n == 5 && !strncmp(p, "setas", 5)) return true;
    if (n == 1 && strchr("<>-+", p[0])) return true;
    bool letra = false;
    for (int i = 0; i < n; i++) {
        if (p[i] >= 'a' && p[i] <= 'z') return false;
        if ((p[i] >= 'A' && p[i] <= 'Z') || (p[i] >= '0' && p[i] <= '9')) letra = true;
    }
    return letra;
}

// Uma tecla: face de ebano com espessura embaixo. 'setas' vira duas setinhas.
static int tecla(int x, int y, const char *rot, int n, bool desenha)
{
    bool setas = n == 5 && !strncmp(rot, "setas", 5);
    char s[16];
    snprintf(s, sizeof s, "%.*s", n < 15 ? n : 15, rot);
    int w = setas ? 22 : gfx_largura(s, 1) + 9;
    if (!desenha) return w;
    gfx_rect(x + 1, y + 1, w - 2, TECLA_H - 2, C_EBANO);            // a espessura
    gfx_rect(x + 1, y + 1, w - 2, TECLA_H - 4, C_EBANO_B);          // a face
    gfx_rect(x + 2, y + 1, w - 4, 1, C_CINZA);                      // luz
    ui_contorno(x, y, w, TECLA_H, C_BARRA);
    if (setas) {
        for (int i = 0; i < 4; i++) {
            gfx_rect(x + 5 + i, y + 7 - i, 1, 2 * i + 1, C_MARFIM);  // <
            gfx_rect(x + 16 - i, y + 7 - i, 1, 2 * i + 1, C_MARFIM); // >
        }
    } else gfx_texto(x + 5, y - 3, s, C_MARFIM, 1, false);
    return w;
}

// Mede (desenha = false) ou desenha a linha de teclas a partir de x.
static int linha_teclas(int x, int y, const char *str, uint16_t cor, bool desenha)
{
    int x0 = x;
    const char *p = str;
    bool primeiro_trecho = true;
    while (*p) {
        while (*p == ' ') p++;
        const char *fim = strstr(p, "·");
        int n = fim ? (int)(fim - p) : (int)strlen(p);
        while (n > 0 && p[n - 1] == ' ') n--;
        if (n > 0) {
            if (!primeiro_trecho) x += 16;
            primeiro_trecho = false;
            // As teclas do comeco do trecho, com "ou"/"e" entre elas.
            const char *q = p, *ate = p + n;
            bool alguma = false;
            while (q < ate) {
                const char *e = q;
                while (e < ate && *e != ' ') e++;
                int m = (int)(e - q);
                bool conj = (m == 2 && !strncmp(q, "ou", 2)) || (m == 1 && q[0] == 'e');
                if (eh_tecla(q, m)) {
                    x += tecla(x, y, q, m, desenha) + 3;
                    alguma = true;
                } else if (conj && alguma && e < ate) {
                    // "ou" so liga se vier outra tecla depois
                    const char *r = e;
                    while (r < ate && *r == ' ') r++;
                    const char *re = r;
                    while (re < ate && *re != ' ') re++;
                    if (!eh_tecla(r, (int)(re - r))) break;
                    char c[4];
                    snprintf(c, sizeof c, "%.*s", m, q);
                    if (desenha) gfx_texto(x + 1, y - 3, c, cor, GFX_FINO, false);
                    x += gfx_largura(c, GFX_FINO) + 5;
                } else break;
                q = e;
                while (q < ate && *q == ' ') q++;
            }
            if (q < ate) {                                  // o que a tecla faz
                char a[96];
                snprintf(a, sizeof a, "%.*s", (int)(ate - q), q);
                if (alguma) x += 2;
                if (desenha) gfx_texto(x, y - 3, a, cor, GFX_FINO, false);
                x += gfx_largura(a, GFX_FINO);
            } else x -= 3;
        }
        if (!fim) break;
        p = fim + strlen("·");
    }
    return x - x0;
}

// A linha de teclas centrada em cx; y e o topo das teclas.
static void teclas_c(int cx, int y, const char *s, uint16_t cor)
{
    int w = linha_teclas(0, y, s, cor, false);
    linha_teclas(cx - w / 2, y, s, cor, true);
}

// Selecao de uma peca numa grade: fundo levemente aceso e as cantoneiras
// na cor, rentes a caixa (as pecas da grade ficam perto umas das outras).
static void ui_selecao(int x, int y, int w, int h, uint16_t cor)
{
    gfx_rect(x + 1, y + 1, w - 2, h - 2, gfx_mistura(C_FELTRO, cor, 22));
    ui_cantos_em(x, y, w, h, cor, ui_respira());
}

// A peca marcada (a que esta em uso), que nao e o cursor: fundo aceso e
// contorno fino na cor, parado.
static void ui_marca(int x, int y, int w, int h, uint16_t cor)
{
    gfx_rect(x + 1, y + 1, w - 2, h - 2, gfx_mistura(C_FELTRO, cor, 22));
    ui_contorno(x, y, w, h, cor);
}

// Etiqueta pequena (raridade, protecao): fundo escuro tingido, o filete na
// cor (cima e esquerda na cor, o lado da sombra um tom abaixo) e o contorno
// escuro por fora, como as placas.
static void ui_etiqueta(int x, int y, int w, int h, uint16_t cor)
{
    uint16_t r[4];
    rampa_de(cor, r);
    gfx_rect(x + 1, y + 1, w - 2, h - 2, gfx_mistura(C_PLACA, cor, 22));
    gfx_rect(x + 1, y, w - 2, 1, cor);
    gfx_rect(x, y + 1, 1, h - 2, cor);
    gfx_rect(x + w - 1, y + 1, 1, h - 2, r[1]);
    gfx_rect(x + 1, y + h - 1, w - 2, 1, r[1]);
    ui_contorno(x - 1, y - 1, w + 2, h + 2, C_BARRA);
}

// A fonte f, ou - com a letra maior, se o texto nao couber em w pixels - a
// mesma fonte no tamanho de antes (fixa).
static int fonte_que_cabe(const char *s, int f, int w)
{
    if (!gfx_letra_e_maior() || gfx_largura(s, f) <= w) return f;
    return f == GFX_MIUDO ? GFX_MIUDO_FIXO : f == GFX_FINO ? GFX_FINO_FIXO : f;
}

// O selo de raridade (ou de tipo: "D8 · raro", "adesivo · comum"), igual em
// todo o jogo: a etiqueta e o nome na cor do metal da raridade. y e o do
// texto, como no gfx_texto; f e a fonte (GFX_MIUDO nos cartoes, GFX_FINO nas
// linhas, 1 ao lado de um nome). Devolve a largura.
static int ui_selo(int x, int y, const char *s, uint16_t cor, int f)
{
    int pad = f == GFX_MIUDO ? 4 : 5, w = gfx_largura(s, f) + 2 * pad;
    if (f == GFX_MIUDO) ui_etiqueta(x, y + 1, w, 12, cor);
    else ui_etiqueta(x, y + 2, w, f == GFX_FINO ? 16 : TXT_H - 2, cor);
    gfx_texto(x + pad, f == GFX_FINO ? y - 1 : y, s, cor, f, false);
    return w;
}
static int ui_selo_c(int cx, int y, const char *s, uint16_t cor, int f)
{
    int w = gfx_largura(s, f) + 2 * (f == GFX_MIUDO ? 4 : 5);
    return ui_selo(cx - w / 2, y, s, cor, f);
}

// Um desenho miudo de pontos, centrado em (cx, cy): linhas de texto com
// '#' a cor, '+' o claro, 'o' o escuro e '.' nada, e o contorno escuro em
// volta, como os dados. espelha: de lado trocado; some (0..1): o quanto ja
// sumiu, no pontilhado.
static void sprite_em(int cx, int cy, const char *const *l, int h, uint16_t c1, uint16_t c2, uint16_t c3,
                      bool espelha, float some)
{
    int w = (int)strlen(l[0]), x0 = cx - w / 2, y0 = cy - h / 2;
#define SP_CHEIO(i, j) ((i) >= 0 && (j) >= 0 && (i) < w && (j) < h && l[j][espelha ? w - 1 - (i) : (i)] != '.')
    for (int j = -1; j <= h; j++)                  // o contorno: o vazio que encosta no desenho
        for (int i = -1; i <= w; i++) {
            if (SP_CHEIO(i, j) || !(SP_CHEIO(i - 1, j) || SP_CHEIO(i + 1, j) || SP_CHEIO(i, j - 1) || SP_CHEIO(i, j + 1)))
                continue;
            if (some > 0 && bayer(x0 + i, y0 + j) < some) continue;
            gfx_pixel(x0 + i, y0 + j, C_BARRA);
        }
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            char c = l[j][espelha ? w - 1 - i : i];
            if (c == '.' || (some > 0 && bayer(x0 + i, y0 + j) < some)) continue;
            gfx_pixel(x0 + i, y0 + j, c == '+' ? c2 : c == 'o' ? c3 : c1);
        }
#undef SP_CHEIO
}

// O botao apertado: um clique (ou o ENTER no botao em foco) afunda o botao
// que esta naquele ponto por um instante.
#define UI_APERTO_T 0.16f
static float t_aperto;
static int aperto_x = -1, aperto_y = -1;
static int foco_bx = -1, foco_by = -1;        // o meio do botao em foco no ultimo quadro
static void ui_aperta(int x, int y) { aperto_x = x; aperto_y = y; t_aperto = UI_APERTO_T; }

// Botao: estado 0 normal, 1 em foco, 2 desligado. 'tinta' e a cor da face
// (0: ebano). A face vai em dois tons da rampa da cor, o claro em cima e a
// passagem pontilhada; embaixo, dois pixels de espessura. Apertado, a face
// desce sobre a espessura. O rotulo vai centrado na face.
#define UI_FOCO 1
#define UI_DESLIGADO 2
// A face do botao, sem o rotulo: devolve a cor da letra que vai nela e, em
// *desce, quanto a face desceu (apertado).
static uint16_t ui_botao_face(int x, int y, int w, int h, uint16_t tinta, int estado, int *desce)
{
    uint16_t face = estado == UI_DESLIGADO ? C_EBANO : tinta ? tinta : C_EBANO_B;
    uint16_t r[4];
    rampa_de(face, r);
    if (estado == UI_FOCO) { face = r[3]; rampa_de(face, r); }   // em foco, um tom acima
    if (estado == UI_FOCO) { foco_bx = x + w / 2; foco_by = y + h / 2; }
    bool baixo = t_aperto > 0 && estado != UI_DESLIGADO && aperto_x >= x && aperto_x < x + w
                 && aperto_y >= y && aperto_y < y + h;
    int d = baixo ? 2 : 0;                                          // apertado: desce a espessura
    if (!baixo) {                                                   // a sombra no feltro
        gfx_rect(x + 2, y + h, w - 3, 1, C_SOMBRA);
        for (int i = 1; i < w; i++) if (bayer(x + i, y + h + 1) < 0.5f) gfx_pixel(x + i, y + h + 1, C_SOMBRA);
    }
    gfx_rect(x + 1, y + 1 + d, w - 2, h - 2 - d, r[0]);            // a espessura
    int fh = h - 4;                                                 // a face: claro, e a passagem embaixo, fora da letra
    for (int j = 0; j < fh; j++) {
        int Y = y + 1 + d + j;
        float m = (j - (fh - 4.5f)) / 3.0f;                         // 0..1 nas linhas de baixo
        if (estado == UI_DESLIGADO || m <= 0) gfx_rect(x + 1, Y, w - 2, 1, r[2]);
        else if (m >= 1) gfx_rect(x + 1, Y, w - 2, 1, r[1]);
        else for (int i = 1; i < w - 1; i++) gfx_pixel(x + i, Y, bayer(x + i, Y) < m ? r[1] : r[2]);
    }
    if (estado != UI_DESLIGADO) gfx_rect(x + 2, y + 1 + d, w - 4, 1, r[3]);   // a luz na beira de cima
    ui_contorno(x, y + d, w, h - d, C_BARRA);
    if (estado == UI_FOCO && !baixo) ui_cantos(x, y, w, h + 1, C_OURO);
    uint16_t c = estado == UI_DESLIGADO ? C_EBANO_B : estado == UI_FOCO ? C_BRANCO : C_MARFIM;
    // Face clara (latao, ouro): letra escura.
    int lum = ((face >> 11) & 31) * 2 + ((face >> 5) & 63) + (face & 31);
    if (lum > 85 && estado != UI_DESLIGADO) c = C_BARRA;
    *desce = d;
    return c;
}
static void ui_botao(int x, int y, int w, int h, const char *rot, uint16_t tinta, int estado)
{
    int d;
    uint16_t c = ui_botao_face(x, y, w, h, tinta, estado, &d);
    int tw = gfx_largura(rot, 1);
    gfx_texto(x + (w - tw) / 2, y + d + (h - 4 - TXT_H) / 2 + 1, rot, c, 1, false);
}

// Plaquinha de metal com um rotulo curto (o "VENCE" do placar), presa na
// beira de uma placa: face na rampa da cor com luz em cima e sombra embaixo,
// contorno e a letra escura gravada. cx e o meio; y, o topo.
#define UI_PLAQUETA_H 16
static void ui_plaqueta(int cx, int y, const char *s, uint16_t cor)
{
    uint16_t r[4];
    rampa_de(cor, r);
    int w = gfx_largura(s, fonte_de(true)) + 14, h = UI_PLAQUETA_H, x = cx - w / 2;
    ui_sombra(x, y, w, h);
    gfx_rect(x + 1, y + 1, w - 2, h - 2, r[2]);
    gfx_rect(x + 2, y + 1, w - 4, 1, r[3]);                          // a luz na beira de cima
    gfx_rect(x + 1, y + h - 2, w - 2, 1, r[1]);                      // e a sombra na de baixo
    ui_contorno(x, y, w, h, C_BARRA);
    gfx_texto(x + 7, y - 2, s, C_BARRA, fonte_de(true), true);
}

// A entrada das placas e dos cartoes, igual em todo lugar: descem 'dist'
// pixels de cima e assentam com a mola (sai_mola) em UI_ENTRA_T segundos;
// cartoes lado a lado entram em cascata, UI_CASCATA depois do vizinho.
// t: ha quanto tempo a tela (ou a placa) apareceu; i: a vez na cascata.
// ui_entra_q > 0: ja apareceu; ui_entra: quanto ainda falta descer (y).
#define UI_ENTRA_T      0.3f
#define UI_CASCATA      0.07f
#define UI_ENTRA_PLACA  10                  // placas e paineis: um pulinho
#define UI_ENTRA_CARTAO 40                  // cartoes: caem de mais alto
static float sai_mola(float q);             // desafio/desenho.inc
static float ui_entra_q(float t, int i) { return (t - i * UI_CASCATA) / UI_ENTRA_T; }
static int ui_entra(float t, int i, int dist)
{
    float q = ui_entra_q(t, i);
    return q >= 1 ? 0 : -(int)((1 - sai_mola(q < 0 ? 0 : q)) * dist);
}

// ---------------------------------------------------------------------------
// Mouse: cada tela, ao se desenhar, marca as areas que aceitam clique. Passar
// o mouse por cima poe o cursor na opcao; clicar e o mesmo que apertar a
// tecla da area (quase sempre ENTER).
// ---------------------------------------------------------------------------
typedef struct { int16_t x, y, w, h; int *var; int val; int tecla; } zona_clique_t;
#define MAX_ZONAS 64
static zona_clique_t zonas[MAX_ZONAS];
static int n_zonas, zonas_fase;            // zonas_fase: a tela que as marcou
static int mouse_x = -1, mouse_y = -1;

static void zona_clique(int x, int y, int w, int h, int *var, int val, int tecla)
{
    if (n_zonas >= MAX_ZONAS) return;
    zonas[n_zonas++] = (zona_clique_t){ (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, var, val, tecla };
}

// Dicas: areas que, com o mouse parado em cima, mostram o que a coisa faz.
enum { DICA_DADO, DICA_AMULETO, DICA_TRUNFO, DICA_ADESIVO };
#define DICA_EMBRULHO D_N                // o id do dado embrulhado pelo Papai Noel (nao diz qual e)
typedef struct { int16_t x, y, w, h; uint8_t tipo, id, com_peca; peca_t pc; } dica_t;
#define MAX_DICAS 64
static dica_t dicas[MAX_DICAS];
static int n_dicas;
static void dica_zona(int x, int y, int w, int h, int tipo, int id)
{
    if (n_dicas >= MAX_DICAS) return;
    dicas[n_dicas++] = (dica_t){ (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, (uint8_t)tipo, (uint8_t)id, 0, { 0 } };
}
// A dica de um dado que se tem na mao: com a peca, mostra tambem os adesivos
// colados nele (os selinhos de baixo sao miudos demais para dizer qual e).
static void dica_peca(int x, int y, int w, int h, const peca_t *p)
{
    if (!p) return;
    dica_zona(x, y, w, h, DICA_DADO, p->tipo);
    if (n_dicas > 0 && dicas[n_dicas - 1].id == p->tipo) { dicas[n_dicas - 1].com_peca = 1; dicas[n_dicas - 1].pc = *p; }
}

static bool mouse_em(int x, int y, int w, int h)
{
    return mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h;
}

// Botao pequeno para o que antes so tinha tecla (TAB, X...). x_dir e a borda
// direita do botao.
static void botao_clique(int x_dir, int y, const char *rot, int tecla)
{
    int w = gfx_largura(rot, 1) + 20, h = TXT_H + 4, x = x_dir - w;
    bool em = mouse_em(x, y, w, h);
    ui_botao(x, y, w, h, rot, 0, em ? UI_FOCO : 0);
    zona_clique(x, y, w, h, NULL, 0, tecla);
}

// Botao "<" no canto de cima, a esquerda: volta a tela anterior sem teclado
// (celular, tablet). A area de toque pega o canto inteiro da faixa. Devolve
// onde o titulo da faixa pode comecar.
#define VOLTAR_X0 46
static int botao_voltar(int tecla)
{
    int x = 8, y = (FAIXA_H - 28) / 2, w = 30, h = 28;
    bool em = mouse_em(0, 0, VOLTAR_X0 - 4, FAIXA_H);
    ui_botao(x, y, w, h, "", 0, em ? UI_FOCO : 0);
    uint16_t c = em ? C_BRANCO : C_MARFIM;
    int cx = x + w / 2 - 2, cy = y + h / 2 - 1;
    gfx_linha_grossa(cx + 4, cy - 7, cx - 3, cy, 3, c);          // a seta: so a ponta, para a esquerda
    gfx_linha_grossa(cx - 3, cy, cx + 4, cy + 7, 3, c);
    zona_clique(0, 0, VOLTAR_X0 - 4, FAIXA_H, NULL, 0, tecla);
    return VOLTAR_X0;
}
