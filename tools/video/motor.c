// Motor do video: o jogo inteiro numa biblioteca, com acesso ao estado de
// dentro, para um roteiro em Python dirigir (ctypes).
#include <stdio.h>
#include "games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
static int64_t relogio = 1;
int64_t esp_timer_get_time(void) { return relogio; }

// Sons: cada chamada vira um evento que o roteiro recolhe a cada quadro.
#define MAX_SONS 256
static int sons[MAX_SONS], n_sons;
static int rufo_on, giro_on, rufo_moeda;
void som_toca(int s) { if (n_sons < MAX_SONS) sons[n_sons++] = s; }
bool som_liga(bool on) { (void)on; return true; }
bool som_ligado(void) { return true; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { rufo_on = on; if (!on) rufo_moeda = 0; }
void som_rufo_moeda(void) { rufo_on = 1; rufo_moeda = 1; }
void som_giro(bool on) { giro_on = on; }

#include "salva_mem.h"

int  mv_sons(int *out) { int n = n_sons; memcpy(out, sons, sizeof(int) * (size_t)n); n_sons = 0; return n; }
int  mv_rufo(void) { return rufo_on ? (rufo_moeda ? 2 : 1) : 0; }
int  mv_giro(void) { return giro_on; }
void mv_relogio(int64_t t) { relogio = t; }
void mv_avanca(int64_t us) { relogio += us; }
uint16_t *mv_fb(void) { return fb; }

// Zonas de clique da tela desenhada por ultimo.
int mv_n_zonas(void) { return zonas_fase == fase ? n_zonas : 0; }
void mv_zona(int i, int *o)
{
    o[0] = zonas[i].x; o[1] = zonas[i].y; o[2] = zonas[i].w; o[3] = zonas[i].h;
    o[4] = zonas[i].val; o[5] = zonas[i].tecla;
    o[6] = zonas[i].var == &cursor ? 1 : zonas[i].var == &menu_cur ? 2 : zonas[i].var == &pf_cur ? 3
         : zonas[i].var == &des_cur ? 4 : zonas[i].var == &dif_cur ? 5 : zonas[i].var ? 9 : 0;
}

int mv_fase(void) { return fase; }
int mv_vez(void) { return vez; }
int mv_cursor(void) { return cursor; }
int mv_ia_na_vez(void) { return ia_na_vez(); }
int mv_anuncio(void) { return t_anuncio > 0; }
int mv_elev(void) { return t_elev > 0; }
int mv_virada(void) { return t_virada > 0; }
float mv_t_fase(void) { return t_fase; }
int mv_menu_tela(void) { return menu_tela; }
int mv_menu_cur(void) { return menu_cur; }
int mv_pronta(void) { return abertura_pronta; }
int mv_fichas(int j) { return J[j].fichas; }
int mv_pote(void) { return pote; }
int mv_rodada(void) { return rodada; }
int mv_primeiro(void) { return primeiro; }
int mv_n_mao(int j) { return J[j].n_mao; }
int mv_mao_tipo(int j, int i) { return J[j].col[J[j].mao[i]].tipo; }
int mv_mao_lados(int j, int i) { return TIPO[J[j].col[J[j].mao[i]].tipo].lados; }
const char *mv_mao_nome(int j, int i) { return TIPO[J[j].col[J[j].mao[i]].tipo].nome; }
int mv_fila_n(int j) { return F[j].n; }
int mv_fila_lancados(int j) { return F[j].lancados; }
int mv_fila_pontos(int j) { return F[j].pontos; }
int mv_fila_total(int j) { return F[j].total; }
int mv_fila_valor(int j, int k) { return F[j].valor[k]; }
int mv_fila_est(int j, int k) { return F[j].est[k]; }
int mv_venc_mao(void) { return venc_mao; }
int mv_venc_partida(void) { return venc_partida; }
int mv_slot_girando(void) { return slot_girando(); }
int mv_risco(int j) { return risco(j); }
int mv_n_voo(void) { return n_voo; }
int mv_detalhe(void) { return detalhe; }
// Run
int mv_run_passo(void) { return run.passo; }
int mv_run_andar(void) { return run.andar; }
int mv_run_moedas(void) { return run.moedas; }
int mv_run_ncol(void) { return run.n_col; }
int mv_no_tipo(int p, int l) { return run.no[p][l].tipo; }
int mv_no_perso(int p, int l) { return run.no[p][l].perso; }
const char *mv_perso_nome(int p) { return PERSO[p].nome; }
int mv_escolhivel(int l) { return escolhivel(l); }
int mv_des_cur(void) { return des_cur; }
int mv_loja_tipo(int i) { return run.loja[i].tipo; }
const char *mv_tipo_nome(int t) { return TIPO[t].nome; }
int mv_tipo_lados(int t) { return TIPO[t].lados; }
int mv_preco_run(int t) { return preco_run(t); }
int mv_am_oferta(int i) { return run.am_oferta[i]; }
const char *mv_am_nome(int a) { return AMULETO[a].nome; }
int mv_oferta_tipo(int i) { return oferta[i].tipo; }
int mv_n_col(int j) { return J[j].n_col; }
int mv_col_tipo(int j, int i) { return J[j].col[i].tipo; }
int mv_mapa_x(int p) { return MAPA_X[p]; }
int mv_mapa_y(int l) { return MAPA_Y[l]; }
int mv_mouse_clicavel(void) { return dado_mouse_clicavel(); }
// Atalhos para montar cenas.
void mv_set_rapido(int r) { rapido = r; }
int mv_rapido(void) { return rapido; }
void mv_salva_limpa(void) { salva_limpa_tudo(); }
void mv_ia_esforco(int e) { ia_esforco = e; }

// Conselhos: o que a IA dificil faria no lugar da pessoa (jogador 0).
static int ia_salva_n; static float ia_salva_o;
static void conselho_ini(void) { ia_salva_n = ia_nivel; ia_salva_o = ia_ousadia; ia_nivel = 2; ia_ousadia = 0; }
static void conselho_fim(void) { ia_nivel = ia_salva_n; ia_ousadia = ia_salva_o; }
int mv_conselho_fila(int *out)
{
    conselho_ini();
    plano_rodada = -1;
    ia_planeja();
    for (int k = 0; k < plano_n; k++) out[k] = plano[k];
    int n = plano_n;
    plano_rodada = -1;                       // o rival planeja do zero
    conselho_fim();
    return n;
}
int mv_conselho_continua(void) { conselho_ini(); int r = ia_continua(); conselho_fim(); return r; }
int mv_conselho_aposta(void) { conselho_ini(); int r = ia_aposta(); conselho_fim(); return r; }
int mv_conselho_premio(void) { conselho_ini(); int r = ia_premio(); conselho_fim(); return r; }
float mv_conselho_chance(void) { conselho_ini(); float r = ia_chance(); conselho_fim(); plano_rodada = -1; return r; }
int mv_aposta_op(int i) { int op[4], n = opcoes(op); return i < n ? op[i] : -1; }
int mv_aposta_n(void) { int op[4]; return opcoes(op); }
int mv_a_pagar(void) { return a_pagar; }
const char *mv_anuncio_txt(void) { return anuncio; }
const char *mv_aviso_txt(void) { return aviso; }
