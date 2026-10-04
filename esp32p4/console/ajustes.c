// Ajustes: som, tela, as opcoes do jogo, o relogio e o "sobre" do aparelho.
#include <stdio.h>
#include <string.h>
#include "console_int.h"
#include "hal/som.h"

enum {
    A_VOLTAR = 200, A_VOL, A_MUSICA, A_BRILHO, A_GIRA, A_CALIBRA,
    A_H_MENOS, A_H_MAIS, A_M_MENOS, A_M_MAIS, A_PAINEL, A_OPCAO0 = 230,
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

static void passo_hora(int id_menos, int cx, int cy, const char *valor)
{
    for (int k = 0; k < 2; k++) {
        int id = id_menos + k, bx = cx + (k ? 62 : -62);
        bool aperta = z_pressionada() == id;
        d_circulo((float)bx, (float)cy, 25, C_BRANCO, aperta ? 90 : 34);
        if (k) ui_icone_mais((float)bx, (float)cy, 26, C_BRANCO, 255);
        else d_linha(bx - 9.0f, (float)cy, bx + 9.0f, (float)cy, 3.4f, C_BRANCO, 255);
        z_marca(id, bx - 30, cy - 30, 60, 60, Z_BOTAO);
    }
    texto_c(FONTE_NEGRITO, 34, cx, cy - 18, valor, C_BRANCO, 255);
}

void ajustes_desenha(void)
{
    d_degrade_v(0, 0, TELA_W, TELA_H, C_FUNDO, C_FUNDO2);
    // Topo
    bool aperta = z_pressionada() == A_VOLTAR;
    d_circulo(80, 64, 32, C_BRANCO, aperta ? 80 : 26);
    ui_icone_voltar(80, 64, 34, C_BRANCO, 255);
    z_marca(A_VOLTAR, 30, 14, 100, 100, Z_BOTAO);
    texto(FONTE_PRETO, 40, 134, 40, "Ajustes", C_BRANCO, 255);
    ui_relogio(1216, 44, 30, C_BRANCO);

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
    rotulo(COL_E + 28, y + 268, "Calibrar o toque");
    ui_botao(A_CALIBRA, COL_E + COL_W - 28 - 160, y + 242, 160, 52, "Calibrar", IC_NADA, B_VIDRO, 21);

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
    rotulo(COL_D + 28, y + 96, "Hora certa");
    char hh[12] = "--", mm[12] = "--";
    if (C.hora >= 0) {
        snprintf(hh, sizeof hh, "%02d", C.hora);
        snprintf(mm, sizeof mm, "%02d", C.minuto);
    }
    int mx = COL_D + COL_W - 28 - 87, hx = mx - 216;
    passo_hora(A_H_MENOS, hx, y + 96, hh);
    texto_c(FONTE_NEGRITO, 34, (hx + mx) / 2, y + 96 - 20, ":", C_TEXTO2, 255);
    passo_hora(A_M_MENOS, mx, y + 96, mm);

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
    if (sis_painel()) {
        char b[48];
        int outro = sis_painel() == 1 ? 2 : 1;
        if (console_armado(A_PAINEL)) {
            snprintf(b, sizeof b, "Reiniciar com o painel V%d", outro);
            ui_botao(A_PAINEL, COL_D + 28, ys + hs - 72, COL_W - 56, 50, b, IC_NADA, B_PERIGO, 20);
        } else {
            snprintf(b, sizeof b, "Usar o painel V%d", outro);
            ui_botao(A_PAINEL, COL_D + 28, ys + hs - 72, 260, 50, b, IC_NADA, B_VIDRO, 20);
        }
    }
    texto_c(FONTE_TEXTO, 18, TELA_W / 2, 752, "Os ajustes ficam guardados no aparelho.", C_TEXTO3, 200);
}

static void acerta(int dh, int dm)
{
    int h = C.hora >= 0 ? C.hora : 12, m = C.minuto >= 0 ? C.minuto : 0;
    h = (h + dh + 24) % 24;
    m = (m + dm + 60) % 60;
    sis_acerta_hora(h, m);
    C.hora = h;
    C.minuto = m;
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
    case A_H_MENOS: acerta(-1, 0); som_toca(SOM_TIQUE); break;
    case A_H_MAIS:  acerta(1, 0);  som_toca(SOM_TIQUE); break;
    case A_M_MENOS: acerta(0, -1); som_toca(SOM_TIQUE); break;
    case A_M_MAIS:  acerta(0, 1);  som_toca(SOM_TIQUE); break;
    case A_PAINEL:
        if (console_armado(A_PAINEL)) sis_troca_painel(sis_painel() == 1 ? 2 : 1);
        else console_arma(A_PAINEL);
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
