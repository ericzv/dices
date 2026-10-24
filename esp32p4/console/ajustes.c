// Ajustes: som, tela, as opcoes do jogo, o relogio e o "sobre" do aparelho.
// Daqui abre a tela de acertar a data e a hora (T_DATA).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "console_int.h"
#include "hal/som.h"

enum {
    A_VOLTAR = 200, A_VOL, A_MUSICA, A_BRILHO, A_GIRA, A_CALIBRA, A_DESLIGA, A_LIMPA, A_DATA, A_PRONTO,
    A_OPCAO0 = 230,
    A_PASSO0 = 250,                       // T_DATA: A_PASSO0 + 2 * campo (menos), + 1 (mais)
};

#define COL_E  64
#define COL_D  656
#define COL_W  560

static int vol_x, bri_x, desl_w = 250;

static const jogo_t *jogo_dos_ajustes(void) { return C.ativo ? C.ativo : CATALOGO[C.foco]; }

static void rotulo(int x, int cy, const char *s)
{
    texto(FONTE_TEXTO, 25, x, cy - 14, s, C_BRANCO, 255);
}

static void linha_divisoria(int x, int y, int w)
{
    d_ret(x + 28, y, w - 56, 1, C_LINHA);
}

static void desliza_linha(int id, int x, int cy, const char *nome, int valor, int vmin, int *gx, bool sol)
{
    if (sol) ui_icone_sol(x + 44, cy, 30, C_TEXTO2, 255);
    else ui_icone_volume(x + 46, cy, 30, valor == 0 ? 0 : valor < 40 ? 1 : valor < 75 ? 2 : 3, C_TEXTO2, 255);
    rotulo(x + 78, cy, nome);
    *gx = x + 214;
    ui_desliza(id, *gx, cy, desl_w, valor, vmin, 100);
    char b[8];
    snprintf(b, sizeof b, "%d%%", valor);
    texto_d(FONTE_NEGRITO, 22, x + COL_W - 28, cy - 12, b, C_TEXTO2, 255);
}

// ---------------------------------------------------------------------------
// A data
// ---------------------------------------------------------------------------
enum { D_DIA, D_MES, D_ANO, D_HORA, D_MIN, N_CAMPOS };
static const char *const MESES[12] = {
    "janeiro", "fevereiro", "março", "abril", "maio", "junho",
    "julho", "agosto", "setembro", "outubro", "novembro", "dezembro",
};
static const char *const SEMANA[7] = {
    "domingo", "segunda-feira", "terça-feira", "quarta-feira", "quinta-feira", "sexta-feira", "sábado",
};

static bool bissexto(int a) { return (a % 4 == 0 && a % 100) || a % 400 == 0; }
static int dias_no_mes(int a, int m)
{
    static const int8_t D[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    return m == 2 && bissexto(a) ? 29 : D[m - 1];
}
static int dia_da_semana(int a, int m, int d)   // 0 domingo (Sakamoto)
{
    static const int8_t T[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (m < 3) a--;
    return (a + a / 4 - a / 100 + a / 400 + T[m - 1] + d) % 7;
}

// "sexta-feira, 24 de outubro de 2026" (curto: "sexta, 24 de outubro de 2026")
static void data_por_extenso(char *b, int n, const sis_data_t *d, bool curto)
{
    char sem[16];
    snprintf(sem, sizeof sem, "%s", SEMANA[dia_da_semana(d->ano, d->mes, d->dia)]);
    char *h = strchr(sem, '-');
    if (curto && h) *h = 0;
    if (!curto && sem[0] >= 'a' && sem[0] <= 'z') sem[0] -= 32;
    snprintf(b, (size_t)n, "%s, %d de %s de %d", sem, d->dia, MESES[d->mes - 1], d->ano);
}

// O relogio agora; sem data, o dia em que este firmware foi compilado (perto
// de hoje); sem hora, meio-dia.
static sis_data_t data_agora(bool *tem_hora, bool *tem_data)
{
    sis_data_t d;
    *tem_hora = sis_data_hora(&d);
    *tem_data = *tem_hora && d.ano > 0;
    if (!*tem_hora) d.h = 12, d.m = 0;
    if (!*tem_data) {
        static const char NOMES[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char m[4] = { __DATE__[0], __DATE__[1], __DATE__[2], 0 };
        const char *p = strstr(NOMES, m);
        d.mes = p ? (int)(p - NOMES) / 3 + 1 : 1;
        d.dia = (__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0');
        d.ano = atoi(__DATE__ + 7);
    }
    return d;
}

#define ANO_MIN 2025
#define ANO_MAX 2099

static void data_passo_campo(int campo, int dir)
{
    bool th, td;
    sis_data_t d = data_agora(&th, &td);
    switch (campo) {
    case D_DIA:  d.dia = (d.dia - 1 + dir + dias_no_mes(d.ano, d.mes)) % dias_no_mes(d.ano, d.mes) + 1; break;
    case D_MES:  d.mes = (d.mes - 1 + dir + 12) % 12 + 1; break;
    case D_ANO:  d.ano = d.ano + dir > ANO_MAX ? ANO_MIN : d.ano + dir < ANO_MIN ? ANO_MAX : d.ano + dir; break;
    case D_HORA: d.h = (d.h + dir + 24) % 24; break;
    case D_MIN:  d.m = (d.m + dir + 60) % 60; break;
    }
    if (d.ano < ANO_MIN) d.ano = ANO_MIN;
    if (d.dia > dias_no_mes(d.ano, d.mes)) d.dia = dias_no_mes(d.ano, d.mes);
    sis_acerta_data_hora(&d);
    C.hora = d.h;
    C.minuto = d.m;
    C.sujo = true;
}

// Um numero com + em cima e - embaixo.
static void passo_vertical(int campo, int cx, int y, const char *nome, const char *valor)
{
    texto_c(FONTE_TEXTO, 22, cx, y + 58, nome, C_TEXTO2, 255);
    for (int k = 0; k < 2; k++) {
        int id = A_PASSO0 + 2 * campo + k, by = k ? y + 140 : y + 304;
        bool aperta = z_pressionada() == id;
        d_circulo((float)cx, (float)by, 34, C_BRANCO, aperta ? 90 : 34);
        if (k) ui_icone_mais((float)cx, (float)by, 34, C_BRANCO, 255);
        else d_linha(cx - 12.0f, (float)by, cx + 12.0f, (float)by, 4.0f, C_BRANCO, 255);
        z_marca(id, cx - 60, by - 42, 120, 84, Z_BOTAO);
    }
    texto_c(FONTE_PRETO, 64, cx, y + 190, valor, C_BRANCO, 255);
}

static void data_desenha(void)
{
    d_degrade_v(0, 0, TELA_W, TELA_H, C_FUNDO, C_FUNDO2);
    bool aperta = z_pressionada() == A_VOLTAR;
    d_circulo(80, 64, 32, C_BRANCO, aperta ? 80 : 26);
    ui_icone_voltar(80, 64, 34, C_BRANCO, 255);
    z_marca(A_VOLTAR, 30, 14, 100, 100, Z_BOTAO);
    texto(FONTE_PRETO, 40, 134, 40, "Data e hora", C_BRANCO, 255);
    int wr = ui_relogio(1216, 44, 30, C_BRANCO);
    ui_bateria(1216 - wr - (wr ? 22 : 0), 61, 1.05f, C_BRANCO);

    bool th, td;
    sis_data_t d = data_agora(&th, &td);
    char v[N_CAMPOS][8];
    snprintf(v[D_DIA], 8, "%02d", d.dia);
    snprintf(v[D_MES], 8, "%02d", d.mes);
    snprintf(v[D_ANO], 8, "%d", d.ano);
    snprintf(v[D_HORA], 8, "%02d", d.h);
    snprintf(v[D_MIN], 8, "%02d", d.m);
    if (!td) strcpy(v[D_DIA], "--"), strcpy(v[D_MES], "--"), strcpy(v[D_ANO], "----");
    if (!th) strcpy(v[D_HORA], "--"), strcpy(v[D_MIN], "--");

    int y = 150, wd = 708, xh = COL_E + wd + 20, wh = TELA_W - 64 - xh;
    ui_cartao(COL_E, y, wd, 360, "DATA");
    passo_vertical(D_DIA, COL_E + wd / 6, y, "Dia", v[D_DIA]);
    passo_vertical(D_MES, COL_E + wd / 2, y, "Mês", v[D_MES]);
    passo_vertical(D_ANO, COL_E + wd * 5 / 6, y, "Ano", v[D_ANO]);
    ui_cartao(xh, y, wh, 360, "HORA");
    passo_vertical(D_HORA, xh + wh / 4, y, "Hora", v[D_HORA]);
    texto_c(FONTE_PRETO, 64, xh + wh / 2, y + 184, ":", C_TEXTO2, 255);
    passo_vertical(D_MIN, xh + wh * 3 / 4, y, "Minuto", v[D_MIN]);

    char ext[64];
    if (td) data_por_extenso(ext, sizeof ext, &d, false);
    else snprintf(ext, sizeof ext, "A data ainda não foi acertada");
    texto_c(FONTE_NEGRITO, 30, TELA_W / 2, 540, ext, td ? C_BRANCO : C_OURO, 255);
    texto_c(FONTE_TEXTO, 20, TELA_W / 2, 586, "Segure o + ou o − para os números correrem.", C_TEXTO3, 255);
    ui_botao(A_PRONTO, TELA_W / 2 - 140, 640, 280, 64, "Pronto", IC_NADA, B_PRIMARIO, 24);
    texto_c(FONTE_TEXTO, 18, TELA_W / 2, 752, "O jogo usa a data: o Natal chega em 20 de outubro.", C_TEXTO3, 200);
}

// Segurar um + ou - repete, cada vez mais rapido.
static int seg_id = -1, seg_n;
static float seg_t;
static bool seg_repetiu;

void data_passo(float dt)
{
    int id = z_pressionada();
    if (id != seg_id) {
        seg_id = id;
        seg_t = 0;
        seg_n = 0;
        if (id < 0) seg_repetiu = false;
        return;
    }
    if (id < A_PASSO0 || id >= A_PASSO0 + 2 * N_CAMPOS) return;
    seg_t += dt;
    float espera = seg_n == 0 ? 0.5f : seg_n < 8 ? 0.14f : 0.06f;
    if (seg_t >= espera) {
        seg_t -= espera;
        seg_n++;
        seg_repetiu = true;
        data_passo_campo((id - A_PASSO0) / 2, (id - A_PASSO0) % 2 ? 1 : -1);
        som_toca(SOM_TIQUE);
    }
}

void ajustes_desenha(void)
{
    if (C.tela == T_DATA) { data_desenha(); return; }
    d_degrade_v(0, 0, TELA_W, TELA_H, C_FUNDO, C_FUNDO2);
    // Topo
    bool aperta = z_pressionada() == A_VOLTAR;
    d_circulo(80, 64, 32, C_BRANCO, aperta ? 80 : 26);
    ui_icone_voltar(80, 64, 34, C_BRANCO, 255);
    z_marca(A_VOLTAR, 30, 14, 100, 100, Z_BOTAO);
    texto(FONTE_PRETO, 40, 134, 40, "Ajustes", C_BRANCO, 255);
    int wr = ui_relogio(1216, 44, 30, C_BRANCO);
    ui_bateria(1216 - wr - (wr ? 22 : 0), 61, 1.05f, C_BRANCO);

    // Som
    int y = 124;
    ui_cartao(COL_E, y, COL_W, 244, "SOM");
    desliza_linha(A_VOL, COL_E, y + 96, "Volume", P.volume, 0, &vol_x, false);
    linha_divisoria(COL_E, y + 140, COL_W);
    rotulo(COL_E + 28, y + 186, "Música do jogo");
    ui_alterna(A_MUSICA, COL_E + COL_W - 28 - 76, y + 165, P.musica);

    // Tela
    y = 388;
    ui_cartao(COL_E, y, COL_W, 324, "TELA");
    desliza_linha(A_BRILHO, COL_E, y + 96, "Brilho", P.brilho, 5, &bri_x, true);
    linha_divisoria(COL_E, y + 140, COL_W);
    rotulo(COL_E + 28, y + 186, "Girar a imagem 180°");
    ui_alterna(A_GIRA, COL_E + COL_W - 28 - 76, y + 165, P.gira);
    linha_divisoria(COL_E, y + 222, COL_W);
    ui_botao(A_CALIBRA, COL_E + 28, y + 242, 240, 52, "Calibrar o toque", IC_NADA, B_VIDRO, 20);
    ui_botao(A_LIMPA, COL_E + 28 + 256, y + 242, 248, 52, "Limpar marcas da tela", IC_NADA, B_VIDRO, 20);

    // Opcoes do jogo
    const jogo_t *j = jogo_dos_ajustes();
    char tit[48];
    snprintf(tit, sizeof tit, "%s", j->nome);
    for (char *p = tit; *p; p++) if (*p >= 'a' && *p <= 'z') *p -= 32;
    y = 124;
    int h_jogo = 64 + 80 * (j->n_opcoes ? j->n_opcoes : 1);
    ui_cartao(COL_D, y, COL_W, h_jogo, tit);
    if (!j->n_opcoes) texto(FONTE_TEXTO, 21, COL_D + 28, y + 80, "Este jogo não tem ajustes.", C_TEXTO3, 255);
    for (int i = 0; i < j->n_opcoes; i++) {
        int cy = y + 96 + i * 80;
        if (i) linha_divisoria(COL_D, cy - 40, COL_W);
        rotulo(COL_D + 28, cy, j->opcoes[i]);
        sis_espaco(j->id);
        bool on = j->opcao_le && j->opcao_le(i);
        sis_espaco(C.ativo ? C.ativo->id : "console");
        ui_alterna(A_OPCAO0 + i, COL_D + COL_W - 28 - 76, cy - 21, on);
    }

    // Relogio
    y = 124 + h_jogo + 20;
    ui_cartao(COL_D, y, COL_W, 164, "RELÓGIO");
    rotulo(COL_D + 28, y + 84, "Data e hora");
    bool th, td;
    sis_data_t d = data_agora(&th, &td);
    char ext[64];
    if (td) data_por_extenso(ext, sizeof ext, &d, true);
    else snprintf(ext, sizeof ext, th ? "falta acertar a data" : "falta acertar a data e a hora");
    texto(FONTE_TEXTO, 20, COL_D + 28, y + 106, ext, td ? C_TEXTO2 : C_OURO, 255);
    ui_botao(A_DATA, COL_D + COL_W - 28 - 160, y + 70, 160, 52, "Acertar", IC_NADA, B_VIDRO, 20);

    // Sobre
    int ys = y + 184, hs = 712 - ys;
    ui_cartao(COL_D, ys, COL_W, hs, "SOBRE ESTE APARELHO");
    char info[512];
    info[0] = 0;
    sis_info(info, sizeof info);
    int ly = ys + 60;
    for (char *l = info; *l && ly < ys + hs - 90;) {
        char *fim = strchr(l, '\n');
        if (fim) *fim = 0;
        texto(FONTE_TEXTO, 19, COL_D + 28, ly, l, C_TEXTO2, 255);
        ly += 29;
        if (!fim) break;
        l = fim + 1;
    }
    // Desligar: pede um segundo toque. Liga de novo pelo botao RST da placa.
    int by = ys + hs - 72;
    if (console_armado(A_DESLIGA)) {
        ui_botao(A_DESLIGA, COL_D + 28, by, COL_W - 56, 50, "Toque de novo para desligar", IC_DESLIGA, B_PERIGO, 20);
    } else {
        ui_botao(A_DESLIGA, COL_D + 28, by, 200, 50, "Desligar", IC_DESLIGA, B_VIDRO, 20);
        texto(FONTE_TEXTO, 17, COL_D + 250, by + 3, "Para ligar de novo, aperte", C_TEXTO3, 255);
        texto(FONTE_TEXTO, 17, COL_D + 250, by + 25, "o botão RST da placa.", C_TEXTO3, 255);
    }
    texto_c(FONTE_TEXTO, 18, TELA_W / 2, 752, "Os ajustes ficam guardados no aparelho.", C_TEXTO3, 200);
}

void ajustes_acao(int id)
{
    const jogo_t *j = jogo_dos_ajustes();
    if (id >= A_OPCAO0 && id < A_OPCAO0 + j->n_opcoes) {
        sis_espaco(j->id);
        if (j->opcao_troca) j->opcao_troca(id - A_OPCAO0);
        sis_espaco(C.ativo ? C.ativo->id : "console");
        som_toca(SOM_TIQUE);
        return;
    }
    if (C.tela == T_DATA) {
        if (id >= A_PASSO0 && id < A_PASSO0 + 2 * N_CAMPOS) {
            if (!seg_repetiu) {               // segurou: ja correu, o soltar nao conta mais um
                data_passo_campo((id - A_PASSO0) / 2, (id - A_PASSO0) % 2 ? 1 : -1);
                som_toca(SOM_TIQUE);
            }
            seg_repetiu = false;
        } else if (id == A_VOLTAR || id == A_PRONTO) {
            console_vai(T_AJUSTES);
            som_toca(SOM_TIQUE);
        }
        return;
    }
    switch (id) {
    case A_VOLTAR:
        prefs_grava();
        console_vai(C.central_de_ajustes && C.ativo ? T_CENTRAL : T_INICIO);
        som_toca(SOM_TIQUE);
        break;
    case A_MUSICA:
        P.musica = !P.musica;
        prefs_grava();
        console_aplica_som();
        som_toca(SOM_TIQUE);
        break;
    case A_GIRA:
        P.gira = !P.gira;
        sis_gira(P.gira);
        console_calibra_gira();
        prefs_grava();
        som_toca(SOM_TIQUE);
        break;
    case A_CALIBRA:
        prefs_grava();
        calibra_comeca();
        som_toca(SOM_TIQUE);
        break;
    case A_LIMPA:
        prefs_grava();
        limpa_comeca();
        break;
    case A_DATA:
        console_vai(T_DATA);
        som_toca(SOM_TIQUE);
        break;
    case A_DESLIGA:
        if (console_armado(A_DESLIGA)) {
            prefs_grava();
            sis_desliga();
        } else {
            console_arma(A_DESLIGA);
        }
        break;
    case A_VOL: case A_BRILHO:
        prefs_grava();                    // soltou a barra
        if (id == A_VOL) som_toca(SOM_FICHA);
        break;
    }
}

void ajustes_desliza(int id, int x)
{
    (void)x;
    if (id == A_VOL && z_desliza_valor(id, vol_x, desl_w, &P.volume, 0, 100)) {
        sis_volume(P.volume);
        C.sujo = true;
    }
    if (id == A_BRILHO && z_desliza_valor(id, bri_x, desl_w, &P.brilho, 5, 100)) {
        sis_brilho(P.brilho);
        C.sujo = true;
    }
}
