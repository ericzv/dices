// Icones e controles do console: botoes em pilula, chaves liga/desliga,
// barras de deslizar e cartoes. Tudo desenhado com as formas suavizadas.
#include <math.h>
#include <stdio.h>
#include "console_int.h"

#define PI_F 3.14159265f

static void arco(float cx, float cy, float r, float a0, float a1, float esp, cor_t c, int alfa)
{
    int n = (int)(fabsf(a1 - a0) * r / 6) + 2;
    for (int i = 0; i < n; i++) {
        float t0 = a0 + (a1 - a0) * i / n, t1 = a0 + (a1 - a0) * (i + 1) / n;
        d_linha(cx + r * cosf(t0), cy + r * sinf(t0), cx + r * cosf(t1), cy + r * sinf(t1), esp, c, alfa);
    }
}

void ui_icone_casa(float cx, float cy, float s, cor_t c, int alfa)
{
    float teto[6] = { cx - s * 0.56f, cy - s * 0.02f, cx, cy - s * 0.52f, cx + s * 0.56f, cy - s * 0.02f };
    d_poligono(teto, 3, c, alfa);
    d_arred(cx - s * 0.36f, cy - s * 0.12f, s * 0.72f, s * 0.6f, s * 0.07f, c, alfa);
}

void ui_icone_play(float cx, float cy, float s, cor_t c, int alfa)
{
    float t[6] = { cx - s * 0.32f, cy - s * 0.44f, cx + s * 0.46f, cy, cx - s * 0.32f, cy + s * 0.44f };
    d_poligono(t, 3, c, alfa);
}

void ui_icone_x(float cx, float cy, float s, cor_t c, int alfa)
{
    float m = s * 0.34f, e = s * 0.13f;
    d_linha(cx - m, cy - m, cx + m, cy + m, e, c, alfa);
    d_linha(cx - m, cy + m, cx + m, cy - m, e, c, alfa);
}

void ui_icone_mais(float cx, float cy, float s, cor_t c, int alfa)
{
    float m = s * 0.38f, e = s * 0.13f;
    d_linha(cx - m, cy, cx + m, cy, e, c, alfa);
    d_linha(cx, cy - m, cx, cy + m, e, c, alfa);
}

void ui_icone_engrenagem(float cx, float cy, float r, cor_t c, cor_t fundo)
{
    for (int k = 0; k < 8; k++) {
        float a = k * PI_F / 4, ca = cosf(a), sa = sinf(a);
        float w0 = r * 0.24f, w1 = r * 0.17f, r0 = r * 0.55f, r1 = r;
        float p[8] = {
            cx + ca * r0 - sa * w0, cy + sa * r0 + ca * w0,
            cx + ca * r1 - sa * w1, cy + sa * r1 + ca * w1,
            cx + ca * r1 + sa * w1, cy + sa * r1 - ca * w1,
            cx + ca * r0 + sa * w0, cy + sa * r0 - ca * w0,
        };
        d_poligono(p, 4, c, 255);
    }
    d_circulo(cx, cy, r * 0.74f, c, 255);
    d_circulo(cx, cy, r * 0.32f, fundo, 255);
}

void ui_icone_volume(float cx, float cy, float s, int ondas, cor_t c, int alfa)
{
    float x0 = cx - s * 0.46f;
    d_arred(x0, cy - s * 0.15f, s * 0.2f, s * 0.3f, s * 0.04f, c, alfa);
    float cone[8] = { x0 + s * 0.16f, cy - s * 0.15f, x0 + s * 0.44f, cy - s * 0.4f,
                      x0 + s * 0.44f, cy + s * 0.4f, x0 + s * 0.16f, cy + s * 0.15f };
    d_poligono(cone, 4, c, alfa);
    for (int i = 0; i < ondas; i++) {
        float r = s * (0.2f + 0.17f * i);
        arco(x0 + s * 0.46f, cy, r, -0.85f, 0.85f, s * 0.08f, c, alfa);
    }
}

void ui_icone_sol(float cx, float cy, float s, cor_t c, int alfa)
{
    d_circulo(cx, cy, s * 0.22f, c, alfa);
    for (int k = 0; k < 8; k++) {
        float a = k * PI_F / 4;
        d_linha(cx + cosf(a) * s * 0.34f, cy + sinf(a) * s * 0.34f, cx + cosf(a) * s * 0.47f,
                cy + sinf(a) * s * 0.47f, s * 0.09f, c, alfa);
    }
}

void ui_icone_voltar(float cx, float cy, float s, cor_t c, int alfa)
{
    float e = s * 0.14f;
    d_linha(cx + s * 0.14f, cy - s * 0.32f, cx - s * 0.18f, cy, e, c, alfa);
    d_linha(cx - s * 0.18f, cy, cx + s * 0.14f, cy + s * 0.32f, e, c, alfa);
}

void ui_icone_girar(float cx, float cy, float s, cor_t c, int alfa)
{
    float r = s * 0.36f;
    arco(cx, cy, r, -PI_F * 0.35f, PI_F * 1.45f, s * 0.11f, c, alfa);
    float a = -PI_F * 0.35f, px = cx + r * cosf(a), py = cy + r * sinf(a);
    float t[6] = { px - s * 0.2f, py - s * 0.08f, px + s * 0.12f, py - s * 0.2f, px + s * 0.06f, py + s * 0.16f };
    d_poligono(t, 3, c, alfa);
}

void ui_icone_tecla_apaga(float cx, float cy, float s, cor_t c, int alfa)
{
    float p[10] = { cx - s * 0.5f, cy, cx - s * 0.22f, cy - s * 0.3f, cx + s * 0.5f, cy - s * 0.3f,
                    cx + s * 0.5f, cy + s * 0.3f, cx - s * 0.22f, cy + s * 0.3f };
    d_poligono(p, 5, c, alfa);
    float m = s * 0.13f, x = cx + s * 0.12f;
    d_linha(x - m, cy - m, x + m, cy + m, s * 0.07f, C_CARTAO, 255);
    d_linha(x - m, cy + m, x + m, cy - m, s * 0.07f, C_CARTAO, 255);
}

void ui_icone_mao(float cx, float cy, float s, cor_t c, int alfa)
{
    d_circulo(cx, cy, s * 0.5f, c, alfa / 3);
    d_circulo(cx, cy, s * 0.28f, c, alfa);
}

static void icone(int ic, float cx, float cy, float s, cor_t c)
{
    switch (ic) {
    case IC_PLAY:   ui_icone_play(cx, cy, s, c, 255); break;
    case IC_CASA:   ui_icone_casa(cx, cy, s, c, 255); break;
    case IC_X:      ui_icone_x(cx, cy, s, c, 255); break;
    case IC_GIRAR:  ui_icone_girar(cx, cy, s, c, 255); break;
    case IC_MAO:    ui_icone_mao(cx, cy, s, c, 255); break;
    case IC_VOLTAR: ui_icone_voltar(cx, cy, s, c, 255); break;
    default: break;
    }
}

void ui_botao(int id, int x, int y, int w, int h, const char *rotulo, int ic, int estilo, float px)
{
    bool aperta = z_pressionada() == id;
    float fx = (float)x, fy = (float)y, fw = (float)w, fh = (float)h;
    if (aperta) { fx += w * 0.02f; fy += h * 0.03f; fw -= w * 0.04f; fh -= h * 0.06f; }
    cor_t fundo, tinta;
    int alfa = 255;
    switch (estilo) {
    case B_PRIMARIO: fundo = C_BRANCO; tinta = C_NOITE; break;
    case B_PERIGO:   fundo = C_VERMELHO; tinta = C_BRANCO; break;
    case B_AZUL:     fundo = C_AZUL; tinta = C_BRANCO; break;
    default:         fundo = C_BRANCO; tinta = C_BRANCO; alfa = 38; break;
    }
    if (aperta && estilo == B_VIDRO) alfa = 70;
    else if (aperta) fundo = d_clareia(fundo, -14);
    if (estilo != B_VIDRO) d_arred(fx + 1, fy + 4, fw, fh, fh / 2, 0, 70);      // sombra
    d_arred(fx, fy, fw, fh, fh / 2, fundo, alfa);
    if (estilo == B_VIDRO) d_arred_borda(fx, fy, fw, fh, fh / 2, 1.5f, C_BRANCO, 46);

    float s = fh * 0.36f;
    int tw = rotulo ? texto_largura(FONTE_NEGRITO, px, rotulo) : 0;
    float total = tw + (ic ? s + (rotulo ? 14 : 0) : 0);
    float cx = fx + fw / 2 - total / 2;
    if (ic) {
        icone(ic, cx + s / 2, fy + fh / 2, s, tinta);
        cx += s + 14;
    }
    if (rotulo) texto(FONTE_NEGRITO, px, (int)cx, (int)(fy + fh / 2 - px / 2), rotulo, tinta, 255);
    z_marca(id, x, y, w, h, Z_BOTAO);
}

void ui_alterna(int id, int x, int y, bool ligado)
{
    d_arred((float)x, (float)y, 76, 42, 21, ligado ? C_AZUL : COR(66, 74, 98), 255);
    float kx = ligado ? x + 55.0f : x + 21.0f;
    d_circulo(kx, y + 23.0f, 17, 0, 60);
    d_circulo(kx, y + 21.0f, 17, C_BRANCO, 255);
    z_marca(id, x - 14, y - 16, 104, 74, Z_BOTAO);
}

void ui_desliza(int id, int x, int y, int w, int valor, int vmin, int vmax)
{
    float q = vmax > vmin ? (float)(valor - vmin) / (float)(vmax - vmin) : 0;
    if (q < 0) q = 0;
    if (q > 1) q = 1;
    d_arred((float)x, y - 4.0f, (float)w, 8, 4, COR(66, 74, 98), 255);
    if (q > 0) d_arred((float)x, y - 4.0f, w * q, 8, 4, C_AZUL, 255);
    float kx = x + w * q, r = z_pressionada() == id ? 19.0f : 16.0f;
    d_circulo(kx, y + 2.0f, r, 0, 70);
    d_circulo(kx, (float)y, r, C_BRANCO, 255);
    z_marca(id, x - 22, y - 30, w + 44, 60, Z_DESLIZA);
}

void ui_cartao(int x, int y, int w, int h, const char *titulo)
{
    d_arred((float)x, (float)y, (float)w, (float)h, 26, C_CARTAO, 255);
    if (titulo) texto(FONTE_NEGRITO, 19, x + 28, y + 20, titulo, C_TEXTO3, 255);
}

void ui_relogio(int xd, int y, float px, cor_t c)
{
    if (C.hora < 0) return;
    char b[16];
    snprintf(b, sizeof b, "%02d:%02d", C.hora % 24, C.minuto % 60);
    texto_d(FONTE_NEGRITO, px, xd, y, b, c, 255);
}
