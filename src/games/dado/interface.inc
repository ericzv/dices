// ===========================================================================
// Kit de interface: as mesmas tres pecas em todo o jogo.
//   ui_painel  placa de ebano com filete de latao (avisos sobre a mesa)
//   ui_cartao  cartao de feltro escuro (loja, premio, perfis, listas)
//   ui_botao   botao com espessura: face, brilho em cima, borda de baixo
// Cantos aparados de um pixel; contorno quase preto; sombra dura embaixo.
// ===========================================================================
#define C_PLACA     RGB(24, 22, 20)              // fundo das placas
#define C_PLACA_LUZ RGB(36, 32, 29)

// Contorno de um pixel com os quatro cantos aparados.
static void ui_contorno(int x, int y, int w, int h, uint16_t c)
{
    gfx_rect(x + 1, y, w - 2, 1, c);
    gfx_rect(x + 1, y + h - 1, w - 2, 1, c);
    gfx_rect(x, y + 1, 1, h - 2, c);
    gfx_rect(x + w - 1, y + 1, 1, h - 2, c);
}

static void ui_painel(int x, int y, int w, int h)
{
    gfx_rect(x + 2, y + 3, w, h, C_SOMBRA);                  // sombra dura
    gfx_rect(x + 1, y + 1, w - 2, h - 2, C_PLACA);
    gfx_rect(x + 2, y + 1, w - 4, 1, C_PLACA_LUZ);           // luz na beira de cima
    ui_contorno(x, y, w, h, C_BARRA);
    ui_contorno(x + 2, y + 2, w - 4, h - 4, C_LATAO_ESC);    // filete de latao
}

// 'cor' (0: ouro) e o filete do cartao em foco; fora de foco ele fica apagado.
static void ui_cartao(int x, int y, int w, int h, bool foco, uint16_t cor)
{
    if (!cor) cor = C_OURO;
    gfx_rect(x + 2, y + 3, w, h, C_SOMBRA);
    gfx_rect(x + 1, y + 1, w - 2, h - 2, foco ? C_FELTRO : C_FELTRO_ESC);
    ui_contorno(x, y, w, h, C_BARRA);
    ui_contorno(x + 1, y + 1, w - 2, h - 2, foco ? cor : gfx_mistura(cor, C_FELTRO_ESC, 55));
    if (foco) ui_contorno(x + 2, y + 2, w - 4, h - 4, gfx_mistura(cor, C_BARRA, 45));
}

// ---------------------------------------------------------------------------
// Numeros flutuantes: o que um dado acabou de valer, perder ou multiplicar
// sobe de cima dele e some. Dizem o resultado sem frase.
// ---------------------------------------------------------------------------
// 'y' e a beira de onde ele sai; 'dir' -1 sobe (fila de baixo), +1 desce
// (fila de cima): sempre para o meio da mesa.
typedef struct { int16_t x, y; int8_t dir; float t, atraso; char s[8]; uint16_t cor; } flutua_t;
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

// Selecao de uma peca numa grade: fundo levemente aceso e contorno na cor.
static void ui_selecao(int x, int y, int w, int h, uint16_t cor)
{
    gfx_rect(x + 1, y + 1, w - 2, h - 2, gfx_mistura(C_FELTRO, cor, 22));
    ui_contorno(x, y, w, h, cor);
}

// Etiqueta pequena (raridade, protecao): fundo escuro tingido e contorno na cor.
static void ui_etiqueta(int x, int y, int w, int h, uint16_t cor)
{
    gfx_rect(x + 1, y + 1, w - 2, h - 2, gfx_mistura(C_PLACA, cor, 22));
    ui_contorno(x, y, w, h, cor);
}

// Botao: estado 0 normal, 1 em foco, 2 desligado. 'tinta' e a cor da face
// (0: ebano). O rotulo vai centrado na face.
#define UI_FOCO 1
#define UI_DESLIGADO 2
static void ui_botao(int x, int y, int w, int h, const char *rot, uint16_t tinta, int estado)
{
    uint16_t face = estado == UI_DESLIGADO ? C_EBANO : tinta ? tinta : C_EBANO_B;
    if (estado == UI_FOCO) face = gfx_na_paleta(gfx_mistura(face, C_BRANCO, 18));
    uint16_t borda = gfx_mistura(face, C_BARRA, 45), luz = gfx_mistura(face, C_BRANCO, 30);
    gfx_rect(x + 1, y + 2, w - 2, h - 1, C_SOMBRA);                 // sombra no feltro
    gfx_rect(x + 1, y + 1, w - 2, h - 2, borda);                    // a espessura
    gfx_rect(x + 1, y + 1, w - 2, h - 4, face);                     // a face
    if (estado != UI_DESLIGADO) gfx_rect(x + 2, y + 1, w - 4, 1, luz);
    ui_contorno(x, y, w, h, estado == UI_FOCO ? C_OURO : C_BARRA);
    uint16_t c = estado == UI_DESLIGADO ? C_EBANO_B : estado == UI_FOCO ? C_BRANCO : C_MARFIM;
    // Face clara (latao, ouro): letra escura.
    int lum = ((face >> 11) & 31) * 2 + ((face >> 5) & 63) + (face & 31);
    if (lum > 85 && estado != UI_DESLIGADO) c = C_BARRA;
    int tw = gfx_largura(rot, 1);
    gfx_texto(x + (w - tw) / 2, y + (h - 4 - TXT_H) / 2 + 1, rot, c, 1, false);
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
enum { DICA_DADO, DICA_AMULETO, DICA_TRUNFO };
typedef struct { int16_t x, y, w, h; uint8_t tipo, id; } dica_t;
#define MAX_DICAS 64
static dica_t dicas[MAX_DICAS];
static int n_dicas;
static void dica_zona(int x, int y, int w, int h, int tipo, int id)
{
    if (n_dicas >= MAX_DICAS) return;
    dicas[n_dicas++] = (dica_t){ (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, (uint8_t)tipo, (uint8_t)id };
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
