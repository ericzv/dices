// Abertura do aparelho e a primeira configuracao: calibrar o toque (quatro
// alvos nos cantos) e conferir se a imagem esta de pe. A calibracao nao supoe
// nada sobre como o painel de toque esta montado: ela descobre qual eixo cru
// vira qual eixo da tela, para que lado cresce e em que escala.
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "console_int.h"
#include "hal/som.h"

enum { A_SIM = 500, A_GIRA_IMG, A_PULA };

static const int ALVO[4][2] = { { 110, 110 }, { 1170, 110 }, { 1170, 690 }, { 110, 690 } };
static int cal_i, n_amostras, cal_volta = T_INICIO;
static float cal_raw[4][2], soma_x, soma_y;
static bool cal_falhou, apertado;

void partida_passo(float dt)
{
    (void)dt;
    static bool tocou;
    if (!tocou && C.t_tela > 0.15f) { tocou = true; som_toca(SOM_ABERTURA); }
    if (C.t_tela > 2.6f) {
        if (P.calibrado) console_vai(T_INICIO);
        else { cal_volta = T_INICIO; calibra_comeca(); }
    }
}

void calibra_comeca(void)
{
    cal_i = 0;
    n_amostras = 0;
    cal_falhou = false;
    apertado = false;
    if (C.tela == T_AJUSTES) cal_volta = T_AJUSTES;
    console_vai(T_CALIBRA);
}

void calibra_passo(float dt) { (void)dt; }

// Reta de minimos quadrados: tela = a * cru + b.
static bool ajusta(const float *cru, const float *tela, float *a, float *b)
{
    float mc = 0, mt = 0;
    for (int i = 0; i < 4; i++) { mc += cru[i]; mt += tela[i]; }
    mc /= 4; mt /= 4;
    float num = 0, den = 0;
    for (int i = 0; i < 4; i++) { num += (cru[i] - mc) * (tela[i] - mt); den += (cru[i] - mc) * (cru[i] - mc); }
    if (den < 1e-3f) return false;
    *a = num / den;
    *b = mt - *a * mc;
    return fabsf(*a) > 0.02f && fabsf(*a) < 50;
}

static void calcula(void)
{
    float dxr = cal_raw[1][0] - cal_raw[0][0], dyr = cal_raw[1][1] - cal_raw[0][1];
    int troca = fabsf(dyr) > fabsf(dxr);
    float u[4], v[4], tx[4], ty[4], ax, bx, ay, by;
    for (int i = 0; i < 4; i++) {
        u[i] = cal_raw[i][troca ? 1 : 0];
        v[i] = cal_raw[i][troca ? 0 : 1];
        tx[i] = (float)ALVO[i][0];
        ty[i] = (float)ALVO[i][1];
    }
    bool ok = ajusta(u, tx, &ax, &bx) && ajusta(v, ty, &ay, &by);
    float pior = 0;
    for (int i = 0; ok && i < 4; i++) {
        float ex = ax * u[i] + bx - tx[i], ey = ay * v[i] + by - ty[i];
        float e = sqrtf(ex * ex + ey * ey);
        if (e > pior) pior = e;
    }
    if (!ok || pior > 70) {                     // algum toque saiu do alvo: de novo
        cal_falhou = true;
        cal_i = 0;
        som_toca(SOM_ANULA);
        return;
    }
    P.troca = troca;
    P.ax = ax; P.bx = bx; P.ay = ay; P.by = by;
    P.calibrado = true;
    prefs_grava();
    som_toca(SOM_VALIDO);
    console_vai(T_ORIENTA);
}

void calibra_toque(int ev, int rx, int ry)
{
    switch (ev) {
    case EV_APERTA:
        soma_x = soma_y = 0;
        n_amostras = 0;
        apertado = true;
        C.sujo = true;
        break;
    case EV_MOVE:
        // as primeiras leituras de um toque tremem: ficam de fora
        if (++n_amostras > 2) { soma_x += rx; soma_y += ry; }
        break;
    case EV_SOLTA:
        apertado = false;
        if (cal_i >= 4) break;
        if (n_amostras > 2) {
            cal_raw[cal_i][0] = soma_x / (n_amostras - 2);
            cal_raw[cal_i][1] = soma_y / (n_amostras - 2);
        } else {
            cal_raw[cal_i][0] = (float)rx;
            cal_raw[cal_i][1] = (float)ry;
        }
        cal_i++;
        cal_falhou = false;
        som_toca(SOM_TIQUE);
        if (cal_i == 4) calcula();
        break;
    }
}

static void fundo(void)
{
    d_degrade_v(0, 0, TELA_W, TELA_H, C_FUNDO, C_FUNDO2);
}

static void desenha_abertura(void)
{
    d_limpa(0);
    float t = C.t_tela;
    int a = t < 0.2f ? 0 : t < 0.9f ? (int)((t - 0.2f) / 0.7f * 255) : t > 2.2f ? (int)((2.6f - t) / 0.4f * 255) : 255;
    if (a < 0) a = 0;
    if (a <= 0) return;
    // Uma face de dado (o tres) e o nome.
    float cx = TELA_W / 2.0f, cy = 300, l = 120;
    d_arred(cx - l / 2, cy - l / 2, l, l, 28, C_BRANCO, a);
    for (int k = -1; k <= 1; k++) d_circulo(cx + k * 30.0f, cy + k * 30.0f, 11, C_NOITE, a);
    texto_c(FONTE_PRETO, 64, TELA_W / 2, 400, "ERICZ", C_BRANCO, a);
    texto_c(FONTE_TEXTO, 24, TELA_W / 2, 478, "jogos em casa", C_TEXTO3, a);
}

static void desenha_alvo(float x, float y, bool ativo)
{
    float pulso = 1 + 0.12f * sinf(C.t_tela * 6);
    if (ativo) {
        d_circulo(x, y, 44 * pulso, C_AZUL, apertado ? 120 : 50);
        d_anel(x, y, 34, 3, C_BRANCO, 255);
        d_linha(x - 52, y, x - 14, y, 3, C_BRANCO, 255);
        d_linha(x + 14, y, x + 52, y, 3, C_BRANCO, 255);
        d_linha(x, y - 52, x, y - 14, 3, C_BRANCO, 255);
        d_linha(x, y + 14, x, y + 52, 3, C_BRANCO, 255);
        d_circulo(x, y, 6, C_BRANCO, 255);
    } else {
        d_circulo(x, y, 10, C_AZUL, 255);
    }
}

static void desenha_calibra(void)
{
    fundo();
    texto_c(FONTE_PRETO, 44, TELA_W / 2, 270, "Vamos ajustar o toque", C_BRANCO, 255);
    texto_c(FONTE_TEXTO, 25, TELA_W / 2, 336, "Toque bem no centro de cada alvo, um de cada vez.", C_TEXTO2, 255);
    char b[32];
    snprintf(b, sizeof b, "%d de 4", cal_i < 4 ? cal_i + 1 : 4);
    texto_c(FONTE_NEGRITO, 22, TELA_W / 2, 392, b, C_TEXTO3, 255);
    if (cal_falhou)
        texto_c(FONTE_NEGRITO, 22, TELA_W / 2, 450, "Um dos toques saiu do alvo. Vamos de novo, com calma.", C_OURO, 255);
    if (sis_toque_estado() == 0) {
        texto_c(FONTE_NEGRITO, 22, TELA_W / 2, 500, "O controlador de toque (GSL3680) não respondeu.", C_VERMELHO, 255);
        texto_c(FONTE_TEXTO, 20, TELA_W / 2, 532, "Confira o cabo do toque (CTP) e reinicie o aparelho.", C_TEXTO2, 255);
    }
    for (int i = 0; i < cal_i && i < 4; i++) desenha_alvo((float)ALVO[i][0], (float)ALVO[i][1], false);
    if (cal_i < 4) desenha_alvo((float)ALVO[cal_i][0], (float)ALVO[cal_i][1], true);
}

static void desenha_orienta(void)
{
    fundo();
    // Uma seta para cima: se ela aparece apontando para baixo, a imagem esta
    // de cabeca para baixo.
    float cx = TELA_W / 2.0f;
    float seta[6] = { cx, 70, cx + 44, 130, cx - 44, 130 };
    d_poligono(seta, 3, C_BRANCO, 255);
    d_arred(cx - 12, 124, 24, 70, 6, C_BRANCO, 255);
    texto_c(FONTE_NEGRITO, 20, TELA_W / 2, 206, "CIMA", C_TEXTO2, 255);

    texto_c(FONTE_PRETO, 46, TELA_W / 2, 290, "Toque ajustado!", C_BRANCO, 255);
    texto_c(FONTE_TEXTO, 26, TELA_W / 2, 360, "A seta aponta para cima? Se a imagem estiver de cabeça", C_TEXTO2, 255);
    texto_c(FONTE_TEXTO, 26, TELA_W / 2, 396, "para baixo, gire. Dá para mudar depois, nos Ajustes.", C_TEXTO2, 255);
    ui_botao(A_SIM, TELA_W / 2 - 350, 490, 330, 88, "Está certa", IC_NADA, B_PRIMARIO, 28);
    ui_botao(A_GIRA_IMG, TELA_W / 2 + 20, 490, 330, 88, "Girar a imagem", IC_GIRAR, B_VIDRO, 26);
}

void primeira_desenha(void)
{
    switch (C.tela) {
    case T_PARTIDA: desenha_abertura(); break;
    case T_CALIBRA: desenha_calibra(); break;
    case T_ORIENTA: desenha_orienta(); break;
    }
}

void primeira_acao(int id)
{
    switch (id) {
    case A_SIM:
        prefs_grava();
        console_vai(cal_volta);
        cal_volta = T_INICIO;
        som_toca(SOM_FICHA);
        break;
    case A_GIRA_IMG:
        P.gira = !P.gira;
        sis_gira(P.gira);
        console_calibra_gira();
        prefs_grava();
        C.sujo = true;
        som_toca(SOM_TIQUE);
        break;
    }
}
