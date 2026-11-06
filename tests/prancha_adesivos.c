// Prancha dos adesivos (exploracao, fica fora do ctest): cada adesivo colado
// num dado, em cada padrao de adesivo_estilo.inc, parado e agindo; a mesa
// inteira em cada padrao (para ver se polui); e o acabamento das raridades.
//
//   prancha_adesivos DIR        grava DIR/*.ppm
#include <stdio.h>
#include <stdlib.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
static int64_t relogio = 11;
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
static int volume_[SOM_CANAIS] = { SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS };
void som_volume(int c, int n) { if (c >= 0 && c < SOM_CANAIS) volume_[c] = n < 0 ? 0 : n > SOM_NIVEIS ? SOM_NIVEIS : n; }
int som_volume_de(int c) { return c >= 0 && c < SOM_CANAIS ? volume_[c] : 0; }

#include "salva_mem.h"

static const char *dir = ".";

static void passa(int n)
{
    for (int i = 0; i < n; i++) {
        dado_desenha();
        dado_passo(1 / 60.0f);
        relogio += 16667;
    }
}

static void grava(const char *nome)
{
    char p[512];
    snprintf(p, sizeof p, "%s/%s.ppm", dir, nome);
    FILE *f = fopen(p, "wb");
    if (!f) { printf("nao consegui gravar %s\n", p); return; }
    fprintf(f, "P6\n%d %d\n255\n", GFX_W, GFX_H);
    for (int i = 0; i < GFX_W * GFX_H; i++) {
        uint16_t c = fb[i];
        fputc(((c >> 11) & 31) * 255 / 31, f);
        fputc(((c >> 5) & 63) * 255 / 63, f);
        fputc((c & 31) * 255 / 31, f);
    }
    fclose(f);
}

// Um dado com o adesivo a no lado de cima, no estilo e.
static void dado_com(int e, float x, float y, float r, int tipo, int a, int v, bool aceso, bool zerado, float t)
{
    ad_estilo = e;
    t_relogio = t;
    ad_face = a; ad_face_aceso = aceso;
    desenha_dado(x, y, r, tipo, 0, 0, v, 2, zerado);
    ad_estilo = ADE_ATUAL;
}

// As colunas: os cinco padroes parados e, dos quatro novos, agindo.
static void pagina(int a0, int q)
{
    gfx_clear(C_FELTRO);
    static const char *const TIT[9] = { "atual", "material", "recorte", "moldura", "relevo",
                                        "agindo", "", "", "" };
    for (int c = 0; c < 9; c++)
        if (TIT[c][0]) gfx_texto(108 + c * 58 - gfx_largura(TIT[c], GFX_FINO) / 2, 4, TIT[c], C_MARFIM_S, GFX_FINO, false);
    for (int i = 0; i < 6 && a0 + i < AD_N; i++) {
        int a = a0 + i, y = 40 + i * 54, v = a == AD_REMENDO ? 6 : 4;
        gfx_texto(6, y - 8, ADESIVO[a].nome, ADESIVO[a].luz == C_BRANCO ? C_MARFIM : ADESIVO[a].luz, GFX_FINO, false);
        gfx_texto(6, y + 2, AF_NOME[AD_FAMILIA[a]], C_TEXTO_M, GFX_FINO, false);
        for (int e = 0; e < ADE_RECREL; e++) dado_com(e, 108 + e * 58, y, R_DADO, D_D6, a, v, false, false, 1.3f + i * 0.4f);
        for (int e = 1; e < ADE_RECREL; e++) dado_com(e, 108 + (4 + e) * 58, y, R_DADO, D_D6, a, v, true, false, 1.3f + i * 0.4f);
    }
    char nome[32];
    snprintf(nome, sizeof nome, "estilos_%d", q);
    grava(nome);
}

// Um dado da fila: o tipo, o numero e o adesivo (se tiver) no lado de cima.
static void poe(int j, int i, int tipo, int v, int a)
{
    J[j].col[i] = (peca_t){ .tipo = (uint8_t)tipo };
    if (a) ad_cola(&J[j].col[i], lado_do_lance(tipo, v, 0), a);
    fila_t *f = &F[j];
    int k = f->lancados;
    f->idx[k] = (int8_t)i; f->tipo[k] = (uint8_t)tipo;
    f->cru[k] = f->valor[k] = v;
    f->lado[k] = (uint8_t)lado_do_lance(tipo, v, 0);
    f->lancados = k + 1; f->n = k + 1;
    aplica_regra(j, k);
}

// A mesa de verdade, com oito dados adesivados, em cada estilo.
static void mesas(void)
{
    modo = M_DESAFIO; cpu = 2;
    des_dif = 2; des_bolsa = B_PANO; des_nova_run(); t_elev = 0; amuleto_escolhido(1);
    run.andar = 0; gera_andar(); run.passo = 0;
    des_comeca_partida(); passa(240);
    memset(&F[0], 0, sizeof F[0]); memset(&F[1], 0, sizeof F[1]);
    poe(0, 0, D_D6, 2, AD_ESTRELINHA); poe(0, 1, D_D6, 3, AD_CHICLETE); poe(0, 2, D_D8, 5, AD_DOURADO);
    poe(0, 3, D_D6, 6, AD_ELETRONICO); poe(0, 4, D_D12, 9, AD_FERMENTO);
    poe(1, 0, D_D6, 1, AD_CASCA); poe(1, 1, D_D6, 4, AD_MIRA); poe(1, 2, D_D8, 6, AD_SELO);
    for (int j = 0; j < 2; j++) memcpy(vis_est[j], F[j].est, N_FILA);
    vez = 0; primeiro = 0; n_voo = 0; n_pouso = 0;
    fase = F_VEREDITO; t_fase = 0.1f;
    for (int e = 0; e < ADE_N; e++) {
        ad_estilo = e;
        t_relogio = 2.1f;
        dado_desenha();
        char nome[32];
        snprintf(nome, sizeof nome, "mesa_%d_%s", e, ADE_NOME[e]);
        grava(nome);
    }
    ad_estilo = ADE_ATUAL;
}

// O acabamento: um de cada raridade, no instante em que o brilho passa.
static void acabamentos(void)
{
    gfx_clear(C_FELTRO);
    static const uint8_t A[4] = { AD_CHICLETE, AD_ESTRELINHA, AD_DOURADO, AD_MIRA };   // comum, incomum, raro, lendario
    static const char *const R[4] = { "comum", "incomum", "raro", "lendario" };
    for (int e = 1; e < ADE_N; e++) {
        for (int k = 0; k < 4; k++) {
            int x = 130 + k * 120, y = 40 + (e - 1) * 80;
            if (e == 1) gfx_texto(x - gfx_largura(R[k], GFX_FINO) / 2, 6, R[k], C_MARFIM_S, GFX_FINO, false);
            for (int f = 0; f < 3; f++) {               // tres instantes do brilho passando
                int rar = RAR_ADESIVO[A[k]];
                float per = rar == RAR_INCOMUM ? 6.0f : rar == RAR_RARO ? 4.2f : 3.4f;
                float fase = (hash2((x - 32 + f * 32) >> 4, y >> 4) & 255) / 256.0f;
                float alvo = (0.05f + f * 0.1f - fase) * per;
                while (alvo < 0) alvo += per;
                dado_com(e, x - 32 + f * 32, y, R_DADO, D_D6, A[k], 4, false, false, alvo);
            }
        }
        gfx_texto(6, 36 + (e - 1) * 80, ADE_NOME[e], C_MARFIM, GFX_FINO, false);
    }
    grava("acabamento");
}

// A colagem proposta, quadro a quadro (so desenho: a loja de hoje nao muda):
// o adesivo chega com o papel por baixo, o verso descola, ele desce e
// aperta (achata um ponto), o ar sai do meio para a beira num brilho, e
// assenta com o brilho da raridade.
static void colagem(void)
{
    gfx_clear(C_FELTRO);
    static const uint8_t A[3] = { AD_CHICLETE, AD_ESTRELINHA, AD_MIRA };
    static const char *const Q[6] = { "paira com o papel", "o verso descola", "desce", "aperta", "o ar sai", "assenta" };
    for (int k = 0; k < 6; k++) gfx_texto(70 + k * 96 - gfx_largura(Q[k], GFX_FINO) / 2, 6, Q[k], C_MARFIM_S, GFX_FINO, false);
    for (int i = 0; i < 3; i++) {
        int a = A[i], y = 70 + i * 100;
        static const char *const R[4] = { "comum", "incomum", "raro", "lendario" };
        gfx_texto(4, y + 26, R[RAR_ADESIVO[a]], C_TEXTO_M, GFX_FINO, false);
        for (int k = 0; k < 6; k++) {
            int x = 70 + k * 96;
            t_relogio = 0.5f;
            bool tem = k >= 3;
            ad_estilo = ADE_MATERIAL;
            ad_face = tem ? a : 0;
            ad_face_aceso = k == 3;
            float rr = R_DADO + (k == 3 ? 1.0f : 0);   // aperta: o dado afunda um nada e a etiqueta achata
            desenha_dado((float)x, (float)y + (k == 3 ? 2 : 0), rr, D_D6, 0, 0, 4, 2, false);
            ad_estilo = ADE_ATUAL;
            if (k == 0) ad_solto_d(x + 2, y - 30, a, 2, 0.0f);
            if (k == 1) ad_solto_d(x + 2, y - 26, a, 2, 0.8f);
            if (k == 2) ad_solto_d(x + 2, y - 10, a, 1, 0.2f);
            if (k == 4) {                               // o ar saindo: um anel claro do meio para a beira
                for (int q = 0; q < 40; q++) {
                    float an = q * 0.157f, d = 9.5f;
                    if ((q + i) % 3 == 0) continue;
                    gfx_pixel(x + (int)(cosf(an) * d), y + (int)(sinf(an) * d * 0.95f), C_BRANCO);
                }
            }
            if (k == 5) {                               // o brilho da raridade, quatro faiscas
                static const uint16_t COR[4] = { C_MARFIM, RGB(168, 204, 230), RGB(246, 222, 132), RGB(226, 164, 168) };
                uint16_t c = COR[RAR_ADESIVO[a]];
                for (int f = 0; f < 4; f++) {
                    int fx = x + (f & 1 ? 20 : -20), fy = y + (f & 2 ? 16 : -18);
                    gfx_rect(fx - 1, fy, 3, 1, c); gfx_rect(fx, fy - 1, 1, 3, c);
                    if (RAR_ADESIVO[a] >= RAR_RARO) { gfx_pixel(fx - 2, fy, c); gfx_pixel(fx + 2, fy, c); gfx_pixel(fx, fy - 2, c); gfx_pixel(fx, fy + 2, c); }
                }
            }
        }
    }
    grava("colagem");
}

// A grade: os 42 adesivos num estilo, cada um num d6 com 5 (o ponto do meio)
// e num d8 com 7 (o algarismo): onde o adesivo e o numero brigam.
static void grade(int e, bool aceso, const char *nome)
{
    gfx_clear(C_FELTRO);
    char tit[64];
    snprintf(tit, sizeof tit, "%s%s", ADE_NOME[e], aceso ? " - agindo" : "");
    gfx_texto(4, 3, tit, C_MARFIM, GFX_FINO, false);
    for (int a = 1; a < AD_N; a++) {
        int i = a - 1, cx = (i % 7) * 91, cy = 14 + (i / 7) * 57;
        gfx_texto(cx + 4, cy, ADESIVO[a].nome, ADESIVO[a].luz == C_BRANCO ? C_MARFIM_S : ADESIVO[a].luz, GFX_FINO, false);
        float t = 1.3f + i * 0.37f;
        dado_com(e, (float)(cx + 24), (float)(cy + 31), R_DADO, D_D6, a, a == AD_REMENDO ? 6 : 5, aceso, false, t);
        dado_com(e, (float)(cx + 67), (float)(cy + 31), R_DADO, D_D8, a, 7, aceso, false, t);
    }
    grava(nome);
}

// O recorte de antes, o recorte em relevo e o relevo grande lado a lado, no
// d6 com 1 e com 5 e no d8 com 7: o que muda quando o numero cai em cima.
static void compara(void)
{
    gfx_clear(C_FELTRO);
    static const int E[3] = { ADE_RECORTE, ADE_RECREL, ADE_RELEVO2 };
    static const uint8_t A[6] = { AD_CHUMBO, AD_CONFETE, AD_ELETRONICO, AD_MIRA, AD_DOURADO, AD_VELCRO };
    for (int e = 0; e < 3; e++) {
        const char *t = ADE_NOME[E[e]];
        gfx_texto(170 + e * 168 - gfx_largura(t, GFX_FINO) / 2, 4, t, C_MARFIM, GFX_FINO, false);
    }
    for (int i = 0; i < 6; i++) {
        int a = A[i], y = 38 + i * 54;
        gfx_texto(6, y - 4, ADESIVO[a].nome, ADESIVO[a].luz == C_BRANCO ? C_MARFIM : ADESIVO[a].luz, GFX_FINO, false);
        for (int e = 0; e < 3; e++) {
            int x0 = 116 + e * 168;
            dado_com(E[e], (float)x0, (float)y, R_DADO, D_D6, a, 1, false, false, 1.3f + i * 0.4f);
            dado_com(E[e], (float)(x0 + 50), (float)y, R_DADO, D_D6, a, 5, false, false, 1.3f + i * 0.4f);
            dado_com(E[e], (float)(x0 + 104), (float)y, R_DADO, D_D8, a, 7, false, false, 1.3f + i * 0.4f);
        }
    }
    grava("compara");
}

// A grade grande: 21 adesivos por pagina, num d6 e num d8 maiores, para ver
// o material de perto (parado e, embaixo de cada um, o d6 anulado).
static void grade_grande(int e, int a0, bool aceso, const char *nome)
{
    gfx_clear(C_FELTRO);
    char tit[64];
    snprintf(tit, sizeof tit, "%s%s", ADE_NOME[e], aceso ? " - agindo" : "");
    gfx_texto(4, 3, tit, C_MARFIM, GFX_FINO, false);
    for (int i = 0; i < 21 && a0 + i < AD_N; i++) {
        int a = a0 + i, cx = (i % 7) * 91, cy = 14 + (i / 7) * 115;
        gfx_texto(cx + 4, cy, ADESIVO[a].nome, ADESIVO[a].luz == C_BRANCO ? C_MARFIM_S : ADESIVO[a].luz, GFX_FINO, false);
        float t = 1.3f + i * 0.37f;
        dado_com(e, (float)(cx + 24), (float)(cy + 38), 22, D_D6, a, a == AD_REMENDO ? 6 : 5, aceso, false, t);
        dado_com(e, (float)(cx + 67), (float)(cy + 38), 22, D_D8, a, 7, aceso, false, t);
        dado_com(e, (float)(cx + 24), (float)(cy + 88), 20, D_D6, a, 3, false, true, t);
        dado_com(e, (float)(cx + 67), (float)(cy + 88), 20, D_D4, a, 3, false, false, t);
    }
    grava(nome);
}

// Os miudos: os 42 adesivos em dados pequenos (a mao, a bolsa): r 11 (so a
// silhueta) e r 14, e o d2 (a moeda).
static void miudos(int e, const char *nome)
{
    gfx_clear(C_FELTRO);
    gfx_texto(4, 3, "miudos: r 11, r 14 e o d2", C_MARFIM, GFX_FINO, false);
    for (int a = 1; a < AD_N; a++) {
        int i = a - 1, cx = 22 + (i % 14) * 44, cy = 34 + (i / 14) * 106;
        float t = 1.3f + i * 0.37f;
        dado_com(e, (float)cx, (float)cy, 11, D_D6, a, 4, false, false, t);
        dado_com(e, (float)cx, (float)(cy + 34), 14, D_D6, a, 2, false, false, t);
        dado_com(e, (float)cx, (float)(cy + 70), 16, D_D2, a, 1, false, false, t);
    }
    grava(nome);
}

// Antes e depois: os que o recorte em relevo descaracterizava, no padrao de
// hoje, no recorte em relevo e no adesivo novo, num dado grande (r 26).
static void antes_depois(void)
{
    gfx_clear(C_FELTRO);
    static const int E[3] = { ADE_ATUAL, ADE_RECREL, ADE_NOVO };
    static const char *const T[3] = { "hoje", "recorte em relevo", "adesivo novo" };
    static const uint8_t A[5] = { AD_LOBO, AD_CHICLETE, AD_CORRETIVO, AD_IMPERMEAVEL, AD_BOMBA };
    for (int e = 0; e < 3; e++) gfx_texto(150 + e * 170 - gfx_largura(T[e], GFX_FINO) / 2, 2, T[e], C_MARFIM, GFX_FINO, false);
    for (int i = 0; i < 5; i++) {
        int a = A[i], y = 50 + i * 66;
        gfx_texto(6, y - 6, ADESIVO[a].nome, ADESIVO[a].luz == C_BRANCO ? C_MARFIM : ADESIVO[a].luz, GFX_FINO, false);
        for (int e = 0; e < 3; e++) {
            int x0 = 118 + e * 170;
            dado_com(E[e], (float)x0, (float)y, 26, D_D6, a, 5, false, false, 1.3f + i * 0.4f);
            dado_com(E[e], (float)(x0 + 64), (float)y, 26, D_D8, a, 7, false, false, 1.3f + i * 0.4f);
        }
    }
    grava("antes_depois");
}

int main(int argc, char **argv)
{
    if (argc > 1) dir = argv[1];
    natal_forcado = 0;
    dado_inicia(0);
    passa(30);
    snprintf(perfis[0].nome, sizeof perfis[0].nome, "TESTE");
    perfis[0].nivel = 10; perfis[0].marcos = 3; perfil_atual = 0; perfis_lidos = true;
    modo = M_DESAFIO;
    if (argc > 2 && !strcmp(argv[2], "novo")) {   // so o adesivo novo: as grades e a mesa
        grade(ADE_NOVO, false, "novo_grade");
        grade(ADE_NOVO, true, "novo_grade_agindo");
        grade_grande(ADE_NOVO, 1, false, "novo_grande_1");
        grade_grande(ADE_NOVO, 22, false, "novo_grande_2");
        miudos(ADE_NOVO, "novo_miudos");
        antes_depois();
        mesas();
        printf("adesivo novo em %s\n", dir);
        return 0;
    }
    int q = 0;
    for (int a0 = 1; a0 < AD_N; a0 += 6) pagina(a0, q++);
    acabamentos();
    colagem();
    mesas();
    compara();
    static const int G[] = { ADE_ATUAL, ADE_RECORTE, ADE_RELEVO, ADE_RECREL, ADE_RELEVO2, ADE_NOVO };
    for (unsigned g = 0; g < sizeof G / sizeof G[0]; g++) {
        char nm[48];
        snprintf(nm, sizeof nm, "grade_%u_%s", g, ADE_NOME[G[g]]);
        for (char *c = nm; *c; c++) if (*c == ' ') *c = '_';
        grade(G[g], false, nm);
        if (G[g] >= ADE_RECREL) { strcat(nm, "_agindo"); grade(G[g], true, nm); }
    }
    printf("%d paginas, acabamento, %d mesas e as grades em %s\n", q, ADE_N, dir);
    return 0;
}
