// O console: estado, ajustes guardados, toque e troca de telas. As telas em
// si estao em inicio.c, ajustes.c, partida.c (jogo, barra, central e teclado)
// e primeira.c (abertura, calibracao e lado da imagem).
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "console_int.h"
#include "hal/som.h"
#include "hal/salva.h"

prefs_t P;
console_t C;

bool som_musica(bool on);                 // o mesmo do PC (desktop/plataforma.h)

// ---------------------------------------------------------------------------
// Ajustes guardados (espaco "console", chave "ajustes")
// ---------------------------------------------------------------------------
static void prefs_le(void)
{
    P = (prefs_t){ .volume = 70, .brilho = 80, .musica = true, .gira = false,
                   .ax = 1, .ay = 1 };
    char b[256];
    sis_espaco("console");
    if (salva_le("ajustes", b, sizeof b) > 0) {
        int v, br, m, g, c, t;
        float ax, bx, ay, by;
        if (sscanf(b, "v=%d b=%d m=%d g=%d c=%d t=%d ax=%f bx=%f ay=%f by=%f",
                   &v, &br, &m, &g, &c, &t, &ax, &bx, &ay, &by) == 10) {
            P.volume = v < 0 ? 0 : v > 100 ? 100 : v;
            P.brilho = br < 5 ? 5 : br > 100 ? 100 : br;
            P.musica = m != 0;
            P.gira = g != 0;
            P.calibrado = c != 0;
            P.troca = t != 0;
            P.ax = ax; P.bx = bx; P.ay = ay; P.by = by;
        }
    }
    sis_espaco(C.ativo ? C.ativo->id : "console");
}

void prefs_grava(void)
{
    char b[256];
    snprintf(b, sizeof b, "v=%d b=%d m=%d g=%d c=%d t=%d ax=%.6f bx=%.3f ay=%.6f by=%.3f",
             P.volume, P.brilho, P.musica, P.gira, P.calibrado, P.troca, P.ax, P.bx, P.ay, P.by);
    sis_espaco("console");
    salva_grava("ajustes", b);
    sis_espaco(C.ativo ? C.ativo->id : "console");
}

// A imagem virou 180 graus: o que era o canto (x, y) agora e (W-1-x, H-1-y).
void console_calibra_gira(void)
{
    P.ax = -P.ax; P.bx = (TELA_W - 1) - P.bx;
    P.ay = -P.ay; P.by = (TELA_H - 1) - P.by;
}

void console_toque_identidade(void)
{
    P.calibrado = true;
    P.troca = 0;
    P.ax = P.ay = 1;
    P.bx = P.by = 0;
}

// ---------------------------------------------------------------------------
// Zonas de toque
// ---------------------------------------------------------------------------
typedef struct { int16_t x, y, w, h, id, tipo; } zona_t;
static zona_t zonas[96];
static int n_zonas;

void z_limpa(void) { n_zonas = 0; }

void z_marca(int id, int x, int y, int w, int h, int tipo)
{
    if (n_zonas >= (int)(sizeof zonas / sizeof zonas[0])) return;
    zonas[n_zonas++] = (zona_t){ (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, (int16_t)id, (int16_t)tipo };
}

static const zona_t *z_em(int x, int y, int folga)
{
    for (int i = n_zonas - 1; i >= 0; i--) {
        const zona_t *z = &zonas[i];
        if (x >= z->x - folga && y >= z->y - folga && x < z->x + z->w + folga && y < z->y + z->h + folga)
            return z;
    }
    return NULL;
}

static const zona_t *z_por_id(int id)
{
    for (int i = n_zonas - 1; i >= 0; i--) if (zonas[i].id == id) return &zonas[i];
    return NULL;
}

// ---------------------------------------------------------------------------
// Toque
// ---------------------------------------------------------------------------
static struct {
    bool dedo;              // dedo na tela (ja sem tremidas)
    int sem;                // leituras seguidas sem dedo
    int x, y, x0, y0;       // posicao na tela, agora e onde apertou
    int rx, ry;             // ultima leitura crua
    float t;                // tempo apertado
    bool longo, moveu, na_mesa;
    int zona, zona_tipo;
} toq = { .zona = -1 };

int z_pressionada(void) { return toq.dedo ? toq.zona : -1; }

bool z_desliza_valor(int id, int x0, int w, int *valor, int vmin, int vmax)
{
    (void)id;
    int v = vmin + (int)lroundf((float)(toq.x - x0) * (vmax - vmin) / (float)w);
    if (v < vmin) v = vmin;
    if (v > vmax) v = vmax;
    if (v == *valor) return false;
    *valor = v;
    return true;
}

static void mapeia(int rx, int ry, int *x, int *y)
{
    float u = (float)(P.troca ? ry : rx), v = (float)(P.troca ? rx : ry);
    int a = (int)lroundf(P.ax * u + P.bx), b = (int)lroundf(P.ay * v + P.by);
    *x = a < 0 ? 0 : a >= TELA_W ? TELA_W - 1 : a;
    *y = b < 0 ? 0 : b >= TELA_H ? TELA_H - 1 : b;
}

static void desliza(int id, int x)
{
    switch (C.tela) {
    case T_AJUSTES: ajustes_desliza(id, x); break;
    case T_JOGO:    jogo_desliza(id, x); break;
    case T_CENTRAL: central_desliza(id, x); break;
    default: break;
    }
}

static void acao(int id)
{
    switch (C.tela) {
    case T_INICIO:  inicio_acao(id); break;
    case T_AJUSTES: ajustes_acao(id); break;
    case T_JOGO:    jogo_acao(id); break;
    case T_CENTRAL: central_acao(id); break;
    default:        primeira_acao(id); break;
    }
}

void console_toque(bool tocando, int rx, int ry)
{
    if (tocando) {
        toq.sem = 0;
        toq.rx = rx;
        toq.ry = ry;
    } else if (toq.dedo && ++toq.sem < 2) {
        return;                           // uma leitura vazia so nao solta o dedo
    }
    bool calibrando = C.tela == T_CALIBRA;
    int x = 0, y = 0;
    if (tocando && !calibrando) mapeia(rx, ry, &x, &y);

    if (tocando && !toq.dedo) {                       // apertou
        toq.dedo = true;
        toq.t = 0;
        toq.longo = toq.moveu = toq.na_mesa = false;
        toq.x = toq.x0 = x;
        toq.y = toq.y0 = y;
        toq.zona = -1;
        if (calibrando) { calibra_toque(EV_APERTA, rx, ry); return; }
        const zona_t *z = z_em(x, y, 0);
        if (!z && C.tela != T_JOGO) z = z_em(x, y, 12);   // dedo e grosso: um pouco de folga
        if (z) {
            toq.zona = z->id;
            toq.zona_tipo = z->tipo;
            if (z->tipo == Z_DESLIZA) desliza(z->id, x);
            C.sujo = true;
        } else if (C.tela == T_JOGO && y < jogo_limite()) {
            toq.na_mesa = true;
            jogo_toque(EV_APERTA, x, y);
        }
        return;
    }
    if (tocando && toq.dedo) {                        // arrastou
        if (calibrando) { calibra_toque(EV_MOVE, rx, ry); return; }
        if (x == toq.x && y == toq.y) return;
        toq.x = x;
        toq.y = y;
        if (abs(x - toq.x0) > 24 || abs(y - toq.y0) > 24) toq.moveu = true;
        if (toq.zona >= 0 && toq.zona_tipo == Z_DESLIZA) desliza(toq.zona, x);
        else if (toq.na_mesa) jogo_toque(EV_MOVE, x, y < jogo_limite() ? y : jogo_limite() - 1);
        return;
    }
    if (!tocando && toq.dedo) {                       // soltou
        toq.dedo = false;
        if (calibrando) { calibra_toque(EV_SOLTA, toq.rx, toq.ry); return; }
        int id = toq.zona;
        toq.zona = -1;
        if (id >= 0) {
            C.sujo = true;
            if (toq.zona_tipo == Z_BOTAO) {
                const zona_t *z = z_por_id(id);
                if (z && toq.x >= z->x - 24 && toq.x < z->x + z->w + 24 && toq.y >= z->y - 24 &&
                    toq.y < z->y + z->h + 24)
                    acao(id);
            } else {
                acao(id);                     // soltou a barra de deslizar
            }
        } else if (toq.na_mesa) {
            jogo_toque(toq.longo ? EV_SOLTA_LONGO : EV_SOLTA, toq.x, toq.y < jogo_limite() ? toq.y : jogo_limite() - 1);
        }
    }
}

// ---------------------------------------------------------------------------
// Telas
// ---------------------------------------------------------------------------
void console_marca_sujo(void) { C.sujo = true; }

void console_vai(int tela)
{
    if (tela != C.tela) C.fade = 0.22f;
    C.tela = tela;
    C.t_tela = 0;
    C.sujo = true;
    C.armado = -1;
    toq.zona = -1;
    toq.na_mesa = false;
    sis_espaco(C.ativo ? C.ativo->id : "console");
    console_aplica_som();
}

void console_aplica_som(void)
{
    bool no_jogo = C.tela == T_JOGO && C.ativo;
    som_liga(true);
    som_musica(P.musica && no_jogo);
    if (!no_jogo) {
        som_rufo(false);
        som_giro(false);
    }
}

void console_arma(int id) { C.armado = id; C.t_armado = 3.0f; C.sujo = true; }
bool console_armado(int id) { return C.armado == id; }

void console_abre_jogo(const jogo_t *j)
{
    if (C.ativo && C.ativo != j) console_fecha_jogo();
    if (!C.ativo) {
        C.ativo = j;
        sis_espaco(j->id);
        texto_jogo_trocas(j->trocas, j->n_trocas);
        j->abre();
    }
    console_vai(T_JOGO);
    som_toca(SOM_FICHA);
}

void console_fecha_jogo(void)
{
    if (!C.ativo) return;
    sis_espaco(C.ativo->id);
    C.ativo->fecha();
    C.ativo = NULL;
    sis_espaco("console");
    console_aplica_som();
}

void console_inicia(void)
{
    texto_inicia();
    texto_jogo_registra();
    memset(&C, 0, sizeof C);
    C.armado = -1;
    C.hora = C.minuto = -1;
    prefs_le();
    sis_brilho(P.brilho);
    sis_volume(P.volume);
    sis_gira(P.gira);
    C.tela = T_PARTIDA;
    C.sujo = true;
    console_aplica_som();
}

void console_botao(void)
{
    switch (C.tela) {
    case T_JOGO:    central_abre(); break;
    case T_CENTRAL: console_vai(T_JOGO); break;
    case T_AJUSTES: console_vai(T_INICIO); break;
    case T_INICIO:  if (C.ativo) console_vai(T_JOGO); break;
    default: break;
    }
}

void console_passo(float dt)
{
    C.t_tela += dt;
    if (C.fade > 0) C.fade -= dt;
    if (C.anima > 0) C.anima -= dt;
    if (C.armado >= 0 && (C.t_armado -= dt) <= 0) { C.armado = -1; C.sujo = true; }

    if (toq.dedo) {
        toq.t += dt;
        if (!toq.longo && !toq.moveu && toq.t > 0.55f && C.tela != T_CALIBRA) {
            toq.longo = true;
            if (toq.na_mesa) jogo_toque(EV_LONGO, toq.x, toq.y);
        }
    }

    // O relogio do canto: confere uma vez por segundo.
    static float t_rel = 1;
    if ((t_rel += dt) >= 1) {
        t_rel = 0;
        int h = -1, m = -1;
        if (!sis_hora(&h, &m)) h = m = -1;
        if (h != C.hora || m != C.minuto) {
            C.hora = h;
            C.minuto = m;
            if (C.tela == T_INICIO || C.tela == T_CENTRAL || C.tela == T_AJUSTES) C.sujo = true;
        }
    }

    switch (C.tela) {
    case T_PARTIDA: partida_passo(dt); break;
    case T_CALIBRA: calibra_passo(dt); break;
    case T_JOGO:
        if (C.ativo) C.ativo->passo(dt);
        break;
    default: break;
    }
}

bool console_desenha(tela_t *t)
{
    d_alvo(t);
    if (!(C.sujo || C.anima > 0 || C.fade > 0 || C.tela == T_JOGO || C.tela == T_PARTIDA ||
          C.tela == T_CALIBRA))
        return false;
    C.sujo = false;
    z_limpa();
    switch (C.tela) {
    case T_INICIO:  inicio_desenha(); break;
    case T_AJUSTES: ajustes_desenha(); break;
    case T_JOGO:    jogo_desenha_tela(); break;
    case T_CENTRAL: central_desenha(); break;
    default:        primeira_desenha(); break;
    }
    d_recorte_tudo();
    if (C.fade > 0) {
        float q = C.fade / 0.22f;
        d_ret_alfa(0, 0, TELA_W, TELA_H, 0, (int)(255 * q * q));
    }
    return true;
}
