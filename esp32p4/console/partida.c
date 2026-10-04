// Durante o jogo: a mesa ampliada em cima, a barra do sistema embaixo (o
// botao de inicio e o volume), o teclado da tela quando o jogo pede texto e a
// central de controle, que pausa o jogo e sobe por cima dele.
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "console_int.h"
#include "hal/keyboard.h"
#include "hal/som.h"

enum {
    A_CENTRAL = 300, A_VOL_MENOS, A_VOL_MAIS,
    A_CONTINUAR = 400, A_INICIO, A_FECHAR, A_FORA, A_C_VOL, A_C_BRILHO, A_C_AJUSTES,
    A_TECLA0 = 600,
};

static int mesa_x, mesa_y, mesa_k = 2, mesa_w = 640, mesa_h = 360;

// ---------------------------------------------------------------------------
// Teclado da tela
// ---------------------------------------------------------------------------
#define TECLADO_Y  468
#define TECLA_W    104
#define TECLA_H    64
#define TECLA_GAP  12

typedef struct { int16_t x, y, w; int16_t k; const char *rot; } tecla_t;
static tecla_t teclas[48];
static int n_teclas;

static void monta_teclado(void)
{
    if (n_teclas) return;
    static const char *const LINHAS[4] = { "1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
    static char rot[40][2];
    int x0 = (TELA_W - (10 * TECLA_W + 9 * TECLA_GAP)) / 2;
    for (int l = 0; l < 4; l++) {
        int y = TECLADO_Y + 24 + l * (TECLA_H + TECLA_GAP), x = x0;
        for (const char *c = LINHAS[l]; *c; c++) {
            char *r = rot[n_teclas];
            r[0] = *c;
            r[1] = 0;
            int k = (*c >= 'A' && *c <= 'Z') ? *c + 32 : *c;
            teclas[n_teclas++] = (tecla_t){ (int16_t)x, (int16_t)y, TECLA_W, (int16_t)k, r };
            x += TECLA_W + TECLA_GAP;
        }
        if (l == 2) teclas[n_teclas++] = (tecla_t){ (int16_t)x, (int16_t)y, TECLA_W, KEY_BKSP, NULL };
        if (l == 3) {
            teclas[n_teclas++] = (tecla_t){ (int16_t)x, (int16_t)y, 184, ' ', "espaço" };
            x += 184 + TECLA_GAP;
            teclas[n_teclas++] = (tecla_t){ (int16_t)x, (int16_t)y, 140, KEY_ENTER, "OK" };
        }
    }
}

bool jogo_teclado_aberto(void)
{
    return C.ativo && C.ativo->digitando && C.ativo->digitando();
}

int jogo_limite(void) { return jogo_teclado_aberto() ? TECLADO_Y : JOGO_H; }

static void desenha_teclado(void)
{
    monta_teclado();
    d_arred(0, TECLADO_Y, TELA_W, TELA_H - TECLADO_Y + 40, 28, COR(22, 25, 34), 252);
    d_arred(TELA_W / 2 - 30, TECLADO_Y + 9, 60, 5, 2.5f, C_BRANCO, 50);
    int apertada = z_pressionada();
    for (int i = 0; i < n_teclas; i++) {
        const tecla_t *t = &teclas[i];
        bool ap = apertada == A_TECLA0 + i, ok = t->k == KEY_ENTER;
        cor_t fundo = ok ? C_AZUL : COR(54, 60, 80);
        if (ap) fundo = d_clareia(fundo, 30);
        d_arred(t->x, t->y + 3.0f, t->w, TECLA_H, 14, 0, 90);
        d_arred(t->x, t->y, t->w, TECLA_H, 14, fundo, 255);
        float cx = t->x + t->w / 2.0f, cy = t->y + TECLA_H / 2.0f;
        if (t->k == KEY_BKSP) ui_icone_tecla_apaga(cx, cy, 42, C_BRANCO, 255);
        else texto_c(FONTE_NEGRITO, t->rot[1] ? 22 : 30, (int)cx, (int)(cy - (t->rot[1] ? 11 : 15)), t->rot,
                     C_BRANCO, 255);
        z_marca(A_TECLA0 + i, t->x - TECLA_GAP / 2, t->y - TECLA_GAP / 2, t->w + TECLA_GAP, TECLA_H + TECLA_GAP,
                Z_BOTAO);
    }
}

// ---------------------------------------------------------------------------
// Barra do sistema
// ---------------------------------------------------------------------------
static void desenha_barra(void)
{
    d_ret(0, BARRA_Y, TELA_W, TELA_H - BARRA_Y, C_BARRA);
    d_ret(0, BARRA_Y, TELA_W, 1, COR(40, 40, 37));
    bool ap = z_pressionada() == A_CENTRAL;
    d_arred(16, BARRA_Y + 11, 170, 58, 29, C_BRANCO, ap ? 70 : 22);
    ui_icone_casa(54, BARRA_Y + 41, 26, C_BRANCO, 235);
    texto(FONTE_NEGRITO, 22, 80, BARRA_Y + 29, "Início", C_BRANCO, 235);
    z_marca(A_CENTRAL, 0, BARRA_Y, 200, TELA_H - BARRA_Y, Z_BOTAO);

    if (C.ativo) texto_c(FONTE_TEXTO, 19, TELA_W / 2, BARRA_Y + 30, C.ativo->nome, C_TEXTO3, 200);

    // Volume: menos, os tracinhos, mais.
    for (int k = 0; k < 2; k++) {
        int id = k ? A_VOL_MAIS : A_VOL_MENOS;
        float cx = k ? 1232.0f : 1056.0f, cy = BARRA_Y + 40.0f;
        d_circulo(cx, cy, 27, C_BRANCO, z_pressionada() == id ? 70 : 22);
        ui_icone_volume(cx + 2, cy, 30, k ? 3 : 1, C_BRANCO, 235);
        z_marca(id, (int)cx - 36, BARRA_Y, 72, TELA_H - BARRA_Y, Z_BOTAO);
    }
    int nivel = (P.volume + 5) / 10;
    for (int i = 0; i < 10; i++) {
        int h = 8 + i * 2, x = 1100 + i * 9;
        d_arred((float)x, BARRA_Y + 52.0f - h, 6, (float)h, 2, C_BRANCO, i < nivel ? 230 : 50);
    }
}

void jogo_desenha_tela(void)
{
    const jogo_t *j = C.ativo;
    if (!j) { console_vai(T_INICIO); return; }
    int w = 0, h = 0;
    const uint16_t *fb = j->desenha(&w, &h);
    if (w > 0 && h > 0) {
        int k = TELA_W / w < JOGO_H / h ? TELA_W / w : JOGO_H / h;
        if (k < 1) k = 1;
        mesa_k = k;
        mesa_w = w;
        mesa_h = h;
        mesa_x = (TELA_W - w * k) / 2;
        mesa_y = (JOGO_H - h * k) / 2;
        if (mesa_x > 0 || mesa_y > 0) d_ret(0, 0, TELA_W, JOGO_H, C_BARRA);
        d_amplia(fb, w, h, mesa_x, mesa_y, k);
        texto_jogo_desenha(mesa_x, mesa_y, k);
    }
    if (jogo_teclado_aberto()) desenha_teclado();
    else desenha_barra();
}

static void para_mesa(int x, int y, int *gx, int *gy)
{
    *gx = (x - mesa_x) / mesa_k;
    *gy = (y - mesa_y) / mesa_k;
    if (*gx < 0) *gx = 0;
    if (*gy < 0) *gy = 0;
    if (*gx >= mesa_w) *gx = mesa_w - 1;
    if (*gy >= mesa_h) *gy = mesa_h - 1;
}

// Toque na mesa: apertar passa o "mouse" por cima (escolhe), soltar clica,
// segurar mostra os detalhes do que esta embaixo do dedo.
void jogo_toque(int ev, int x, int y)
{
    const jogo_t *j = C.ativo;
    if (!j) return;
    int gx, gy;
    para_mesa(x, y, &gx, &gy);
    switch (ev) {
    case EV_APERTA:
    case EV_MOVE:
        j->toque(gx, gy, 0);
        break;
    case EV_SOLTA:
        j->toque(gx, gy, 0);
        j->toque(gx, gy, 1);
        break;
    case EV_LONGO:
        if (j->detalhe) j->detalhe();
        break;
    default:
        break;
    }
}

static void muda_volume(int d)
{
    int v = P.volume + d;
    v = v < 0 ? 0 : v > 100 ? 100 : v;
    P.volume = v;
    sis_volume(v);
    prefs_grava();
    som_toca(SOM_TIQUE);
}

void jogo_acao(int id)
{
    if (id >= A_TECLA0 && id < A_TECLA0 + n_teclas) {
        if (C.ativo) C.ativo->tecla(teclas[id - A_TECLA0].k);
        return;
    }
    switch (id) {
    case A_CENTRAL:   central_abre(); break;
    case A_VOL_MENOS: muda_volume(-10); break;
    case A_VOL_MAIS:  muda_volume(10); break;
    }
}

void jogo_desliza(int id, int x) { (void)id; (void)x; }

// ---------------------------------------------------------------------------
// Central de controle
// ---------------------------------------------------------------------------
static uint16_t *foto;                    // o jogo parado, ja escurecido
static bool foto_ok;
static int c_vol_x, c_bri_x;
#define C_DESL_W 300
#define PAINEL_Y 452

void central_abre(void)
{
    foto_ok = false;
    console_vai(T_CENTRAL);
    C.fade = 0;
    C.anima = 0.3f;
    som_toca(SOM_TIQUE);
}

static float painel_y(void)
{
    float t = C.t_tela / 0.24f;
    if (t > 1) t = 1;
    float e = 1 - (1 - t) * (1 - t) * (1 - t);
    return PAINEL_Y + (1 - e) * (TELA_H - PAINEL_Y);
}

static void desliza_c(int id, int x, int cy, int valor, int vmin, int *gx, bool sol)
{
    if (sol) ui_icone_sol(x + 22, (float)cy, 34, C_TEXTO2, 255);
    else ui_icone_volume(x + 24, (float)cy, 34, valor == 0 ? 0 : valor < 40 ? 1 : valor < 75 ? 2 : 3, C_TEXTO2, 255);
    *gx = x + 74;
    ui_desliza(id, *gx, cy, C_DESL_W, valor, vmin, 100);
    char b[8];
    snprintf(b, sizeof b, "%d%%", valor);
    texto(FONTE_NEGRITO, 22, x + 74 + C_DESL_W + 30, cy - 12, b, C_TEXTO2, 255);
}

void central_desenha(void)
{
    tela_t *t = d_alvo_atual();
    if (!foto) foto = sis_aloca((size_t)TELA_W * TELA_H * 2);
    if (foto && !foto_ok) {
        memcpy(foto, t->px, (size_t)TELA_W * TELA_H * 2);
        tela_t f = { foto, TELA_W, TELA_H };
        d_alvo(&f);
        d_ret_alfa(0, 0, TELA_W, TELA_H, C_NOITE, 150);
        d_alvo(t);
        foto_ok = true;
    }
    if (foto) d_copia(foto, TELA_W, TELA_H, 0, 0);
    int py = (int)painel_y();
    z_marca(A_FORA, 0, 0, TELA_W, py, Z_BOTAO);
    d_arred(0, (float)py, TELA_W, TELA_H - py + 60.0f, 36, COR(24, 29, 45), 255);
    d_arred(TELA_W / 2 - 32, py + 12.0f, 64, 6, 3, C_BRANCO, 60);

    const jogo_t *j = C.ativo;
    if (j) {
        texto(FONTE_NEGRITO, 30, 64, py + 34, j->nome, C_BRANCO, 255);
        texto(FONTE_NEGRITO, 18, 64, py + 74, "EM PAUSA", C_OURO, 255);
    }
    ui_relogio(1100, py + 38, 28, C_TEXTO2);
    bool ap = z_pressionada() == A_C_AJUSTES;
    d_circulo(1180, py + 56.0f, 30, C_BRANCO, ap ? 80 : 26);
    ui_icone_engrenagem(1180, py + 56.0f, 19, C_BRANCO, d_mistura(COR(24, 29, 45), C_BRANCO, ap ? 80 : 26));
    z_marca(A_C_AJUSTES, 1140, py + 16, 80, 80, Z_BOTAO);

    int by = py + 126;
    ui_botao(A_CONTINUAR, 64, by, 370, 96, "Continuar", IC_PLAY, B_PRIMARIO, 28);
    ui_botao(A_INICIO, 455, by, 370, 96, "Ir para o início", IC_CASA, B_VIDRO, 26);
    if (console_armado(A_FECHAR)) ui_botao(A_FECHAR, 846, by, 370, 96, "Toque de novo", IC_X, B_PERIGO, 26);
    else ui_botao(A_FECHAR, 846, by, 370, 96, "Fechar o jogo", IC_X, B_VIDRO, 26);

    int sy = py + 284;
    desliza_c(A_C_VOL, 64, sy, P.volume, 0, &c_vol_x, false);
    desliza_c(A_C_BRILHO, 680, sy, P.brilho, 5, &c_bri_x, true);
}

void central_acao(int id)
{
    switch (id) {
    case A_CONTINUAR:
    case A_FORA:
        console_vai(T_JOGO);
        C.fade = 0;
        som_toca(SOM_TIQUE);
        break;
    case A_INICIO:
        console_vai(T_INICIO);
        som_toca(SOM_TIQUE);
        break;
    case A_FECHAR:
        if (console_armado(A_FECHAR)) {
            console_fecha_jogo();
            console_vai(T_INICIO);
            som_toca(SOM_ANULA);
        } else {
            console_arma(A_FECHAR);
            som_toca(SOM_TIQUE);
        }
        break;
    case A_C_AJUSTES:
        C.central_de_ajustes = true;
        console_vai(T_AJUSTES);
        som_toca(SOM_TIQUE);
        break;
    case A_C_VOL:
    case A_C_BRILHO:
        prefs_grava();
        if (id == A_C_VOL) som_toca(SOM_FICHA);
        break;
    }
}

void central_desliza(int id, int x)
{
    (void)x;
    if (id == A_C_VOL && z_desliza_valor(id, c_vol_x, C_DESL_W, &P.volume, 0, 100)) {
        sis_volume(P.volume);
        C.sujo = true;
    }
    if (id == A_C_BRILHO && z_desliza_valor(id, c_bri_x, C_DESL_W, &P.brilho, 5, 100)) {
        sis_brilho(P.brilho);
        C.sujo = true;
    }
}
