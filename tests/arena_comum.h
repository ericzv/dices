// O que a arena (tests/arena.c) e o teste da IA mestre (tests/ia_mestre.c)
// tem em comum: o jogo inteiro incluido (para alcancar as funcoes de dentro),
// o aparelho de mentira, as bolsas do Desafiante e uma partida igual dos dois
// lados, IA contra IA.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
static int64_t relogio = 1;
int64_t esp_timer_get_time(void) { return relogio; }
void som_toca(int s) { (void)s; }
void som_toca_tom(int s, int t) { (void)s; (void)t; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }
void som_rufo_moeda(void) {}
void som_giro(bool on) { (void)on; }
void som_suspense(bool on) { (void)on; }
void som_cadeia(bool on) { (void)on; }
void som_chefe(int q, bool t) { (void)q; (void)t; }
void som_volume(int c, int n) { (void)c; (void)n; }
int som_volume_de(int c) { (void)c; return SOM_NIVEIS; }

#include "salva_mem.h"

static uint32_t est = 99991;
static uint32_t rnd_t(void) { est ^= est << 13; est ^= est >> 17; est ^= est << 5; return est; }

// Um perfil com tudo aberto: os dados de todos os niveis aparecem nas bolsas.
static void perfil_teste(void)
{
    salva_limpa_tudo();
    memset(perfis, 0, sizeof perfis);
    snprintf(perfis[0].nome, sizeof perfis[0].nome, "ARENA");
    perfis[0].nivel = (uint8_t)N_NIVEIS;
    perfis[0].marcos = 3;
    perfil_atual = 0;
    perfis_lidos = true;
    des_dif = N_NIVEIS;
    perfis[0].bolsas = BOLSAS_TODAS;
    des_bolsa = B_PANO;
}

// ---- As bolsas ----
// A de um oponente do Desafiante: nivel, salao e mesa ao acaso (com chefes).
static int bolsa_oponente(peca_t *col)
{
    int nivel = (int)(rnd_t() % (N_NIVEIS + 1));
    int saloes = nivel >= CORINGA_DESDE ? DES_ANDARES + 1 : DES_ANDARES;
    int a = (int)(rnd_t() % (uint32_t)saloes);
    memset(&run, 0, sizeof run);
    memcpy(run.marca, RUN_MARCA, 4);
    run.semente = rnd_t() | 1;
    run.dificuldade = (uint8_t)nivel;
    run.andar = (uint8_t)a;
    int t = 0;
    if (a == DES_ANDARES) gera_salao_final();
    else { gera_andar(); t = (int)(rnd_t() % DES_MESAS); }
    return monta_oponente(&run.no[t], a, col);
}

// A de um jogador: uma bolsa de saida e dois dados da vitrine por salao andado.
static int bolsa_jogador(peca_t *b)
{
    int q = (int)(rnd_t() % B_N), nb = 0;
    for (int i = 0; i < bolsa_n_dados(q); i++) {
        b[nb] = (peca_t){ .tipo = BOLSA[q].dados[i], .fixo = bolsa_fixo(q, i) };
        if (b[nb].tipo == D_ADESIVADO) adesiva(&b[nb], rola_r, true);
        nb++;
    }
    memset(&run, 0, sizeof run);
    memcpy(run.marca, RUN_MARCA, 4);
    run.semente = rnd_t() | 1;
    run.dificuldade = (uint8_t)(rnd_t() % (N_NIVEIS + 1));
    int saloes = (int)(rnd_t() % 4u);
    for (int k = 0; k < saloes; k++) {
        run.andar = (uint8_t)(k < DES_ANDARES ? k : DES_ANDARES - 1);
        run.lojas++;
        monta_vitrine();
        b[nb++] = run.vitrine[0];
        b[nb++] = run.vitrine[1];
    }
    return nb;
}

// A bolsa de saida da partida rapida: os oito dados de sempre.
static int bolsa_de_saida(peca_t *b)
{
    jogador_t p;
    memset(&p, 0, sizeof p);
    colecao_inicial(&p);
    memcpy(b, p.col, sizeof(peca_t) * (size_t)p.n_col);
    return p.n_col;
}

// Doze dados de qualquer tipo (todos os poderes passam pela IA), um em cada
// tres com um adesivo num lado.
static int bolsa_ao_acaso(peca_t *b)
{
    int nb = 0;
    while (nb < 12) {
        int t = 1 + (int)(rnd_t() % (uint32_t)(D_N - 1));
        if (sem_sorteio(t)) continue;
        b[nb] = peca_r(t);
        if (rnd_t() % 3u == 0) {
            int a = 1 + (int)(rnd_t() % (uint32_t)(AD_N - 1));
            int l = ad_lado_min(t) + (int)(rnd_t() % (uint32_t)(ad_lado_max(t) - ad_lado_min(t) + 1));
            if (ad_a_venda(a) && l <= AD_LADOS && !ad_no_lado(&b[nb], l) && ad_lado_serve(&b[nb], l)) ad_cola(&b[nb], l, a);
        }
        nb++;
    }
    return nb;
}

// ---- A partida ----
// Como des_comeca_partida, mas igual dos dois lados: sem trunfo, sem amuleto,
// sem a regra da bolsa, as duas IAs com 50 fichas, 8 rodadas e a entrada que
// sobe. niv0 e niv1: o nivel da IA de cada lugar.
static void comeca(const peca_t *b0, int n0, const peca_t *b1, int n1, int niv0, int niv1, uint32_t sem)
{
    memset(&run, 0, sizeof run);
    memcpy(run.marca, RUN_MARCA, 4);
    run.semente = sem | 1;
    run.no[0].tipo = NO_FACIL;
    run.no[0].semente = sem | 1;
    modo = M_DESAFIO;
    cpu = 3;
    remoto = 0;
    baixo = 0;
    semente = (sem * 2654435761u) | 1;
    for (int i = 0; i < 8; i++) rnd();
    memset(J, 0, sizeof J);
    memset(F, 0, sizeof F);
    J[0].nome = "ZERO"; J[1].nome = "UM";
    J[0].cor = C_LATAO; J[1].cor = C_ROSA;
    amul[0] = amul[1] = 0;
    op_amul_extra = -1;
    des_rod_vencidas = des_rod_cheias = 0;
    reg_op = REG_NADA;
    estilo_op = 0;
    op_nervoso = false;
    fala_perso = 0;
    perso_virado = -1;
    t_virada = 0; t_fala = 0; t_reacao = t_curta = 0;
    fala_e_curta = false;
    J[0].fichas = J[1].fichas = DES_FICHAS;
    J[0].n_col = n0; memcpy(J[0].col, b0, sizeof(peca_t) * (size_t)n0);
    J[1].n_col = n1; memcpy(J[1].col, b1, sizeof(peca_t) * (size_t)n1);
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < J[j].n_col; i++) { J[j].onde[i] = NA_BOLSA; J[j].col[i].velcro = 0; }
        zera_acumuladores(J[j].col, J[j].n_col);
        memset(J[j].pavio, 0, sizeof J[j].pavio);
    }
    nivel_jog[0] = niv0; nivel_jog[1] = niv1;
    pensa_jog[0] = pensa_jog[1] = 0;
    ousadia_jog[0] = ousadia_jog[1] = 0;
    ia_conta = sem;
    materiais_da_partida(MAT_MARFIM);
    rodada = 0;
    memset(bis_fila, 0, sizeof bis_fila);
    memset(bis_vem, 0, sizeof bis_vem);
    memset(ad_moedas, 0, sizeof ad_moedas);
    pote = 0;
    primeiro = (int)(rnd() & 1);
    venc_partida = -1;
    correu_venc = -1;
    n_voo = 0;
    t_sair = -1;
    des_saiu = false;
    des_primeira_derrota = false;
    detalhe = -1;
    nova_rodada();
}

static double agora_ms(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}
