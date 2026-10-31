// Teste visual: percorre as telas do jogo (o menu, o Desafiante, o mapa de
// cada salao, a loja, a mesa, o catalogo, o tutorial, o painel do som...)
// sempre do mesmo jeito - o mesmo relogio, a mesma data, o mesmo sorteio - e
// compara cada uma com a referencia guardada em tests/telas_ref.txt.
//
// A referencia nao guarda os pixels: guarda a cor media de cada bloco de
// 20 x 20 da tela (32 x 18 blocos). Uma tela muda quando algum bloco muda
// mais que BLOCO_TOL num canal, ou a tela inteira, em media, mais que
// MEDIA_TOL. Assim um pixel a mais ou a menos de outro compilador nao conta,
// e uma tela quebrada (algo que sumiu, saiu do lugar ou mudou de cor) conta.
//
//   telas REF                compara (o ctest)
//   telas REF --atualiza     grava a referencia de novo (depois de mudar uma
//                            tela de proposito)
//   telas REF --fotos DIR    grava tambem cada tela em DIR/nome.ppm
//
// So entra no ctest do Linux: em outros sistemas a conta em float da
// fisica e das curvas sai um pouco diferente.
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
void som_chefe(bool on) { (void)on; }
static int volume_[SOM_CANAIS] = { SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS };
void som_volume(int c, int n) { if (c >= 0 && c < SOM_CANAIS) volume_[c] = n < 0 ? 0 : n > SOM_NIVEIS ? SOM_NIVEIS : n; }
int som_volume_de(int c) { return c >= 0 && c < SOM_CANAIS ? volume_[c] : 0; }

#include "salva_mem.h"

#define BLOCO     20
#define BX        (GFX_W / BLOCO)
#define BY        (GFX_H / BLOCO)
#define N_ASSIN   (BX * BY * 3)
#define BLOCO_TOL 40                         // um bloco mudou: mais que isto num canal (0..255)
#define MEDIA_TOL 1.0                        // a tela mudou: mais que isto, em media, em todos os blocos
#define MAX_TELAS 32

typedef struct { char nome[24]; uint8_t a[N_ASSIN]; } assin_t;
static assin_t ref[MAX_TELAS], nova[MAX_TELAS];
static int n_ref, n_nova, falhas;
static const char *fotos;

// Um quadro do jogo, como a plataforma faz: desenha e anda 1/60 s.
static void passa(int n)
{
    for (int i = 0; i < n; i++) {
        dado_desenha();
        dado_passo(1 / 60.0f);
        relogio += 16667;
    }
}

static void foto(const char *nome)
{
    char p[512];
    snprintf(p, sizeof p, "%s/%s.ppm", fotos, nome);
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

// Desenha a tela de agora e guarda a assinatura dela.
static void tela(const char *nome)
{
    dado_desenha();
    if (n_nova >= MAX_TELAS) return;
    assin_t *s = &nova[n_nova++];
    snprintf(s->nome, sizeof s->nome, "%s", nome);
    for (int by = 0; by < BY; by++)
        for (int bx = 0; bx < BX; bx++) {
            unsigned soma[3] = { 0, 0, 0 };
            for (int y = by * BLOCO; y < (by + 1) * BLOCO; y++)
                for (int x = bx * BLOCO; x < (bx + 1) * BLOCO; x++) {
                    uint16_t c = fb[y * GFX_W + x];
                    soma[0] += ((c >> 11) & 31) * 255 / 31;
                    soma[1] += ((c >> 5) & 63) * 255 / 63;
                    soma[2] += (c & 31) * 255 / 31;
                }
            for (int k = 0; k < 3; k++) s->a[(by * BX + bx) * 3 + k] = (uint8_t)(soma[k] / (BLOCO * BLOCO));
        }
    if (fotos) foto(nome);
}

// O passeio: as mesmas telas, sempre na mesma ordem e no mesmo instante.
static void passeio(void)
{
    natal_forcado = 0;                       // fora da epoca do Natal, qualquer que seja o dia
    dado_inicia(0);
    passa(30);
    snprintf(perfis[0].nome, sizeof perfis[0].nome, "TESTE");
    perfis[0].nivel = 10; perfis[0].marcos = 3; perfil_atual = 0; perfis_lidos = true;
    perfis[0].bolsas = 0xFF;

    fase = F_ABERTURA; menu_tela = MT_PRINCIPAL; t_fase = 0; menu_cur = 0; passa(150); tela("menu");
    som_abre(); passa(30); tela("painel_som"); som_aberto = false;
    menu_tela = MT_DESAFIO; t_fase = 0; passa(60); tela("menu_desafio");
    des_abre_perfis(); passa(40); tela("perfis");
    dif_cur = 2; fase = F_RDIFIC; t_fase = 0; passa(40); tela("dificuldade");
    abre_bolsas(); passa(260); tela("bolsas");
    des_dif = 2; des_bolsa = B_LATA; des_nova_run(); t_elev = 0; passa(30);
    abre_amuleto(); passa(60); tela("amuleto");
    amuleto_escolhido(1);
    static const char *const MAPA[] = { "mapa_verde", "mapa_vermelho", "mapa_dourado", "mapa_coringa" };
    for (int a = 0; a <= DES_ANDAR_FINAL; a++) {
        run.andar = (uint8_t)a;
        if (a == DES_ANDAR_FINAL) gera_salao_final(); else gera_andar();
        run.passo = 0;
        abre_mapa(); t_mapa_anda = 0; t_luz = 0; passa(60); tela(MAPA[a]);
    }
    run.andar = 0; gera_andar(); run.passo = 0;
    run.moedas = 14; abre_loja_run(); passa(90); tela("loja");
    des_comeca_partida(); passa(20); tela("mesa");
    bolsa_abre(0); passa(30); tela("bolsa_aberta"); bolsa_fecha();
    t_fase = 0; fase = F_CATALOGO; passa(40); tela("catalogo");
    tut_pag = 0; t_fase = 0; fase = F_TUTORIAL; passa(40); tela("tutorial");
    estilo_abre(0); passa(40); tela("estilo");
    hist_abre(0); passa(40); tela("historico");
    des_bolsa = B_PANO; des_nova_run(); t_elev = 0; amuleto_escolhido(1); des_comeca_partida();
    encerra_run(false); passa(90); tela("fim_da_run");
}

static int hex(int c) { return c <= '9' ? c - '0' : (c | 32) - 'a' + 10; }

static bool le_ref(const char *caminho)
{
    FILE *f = fopen(caminho, "r");
    if (!f) return false;
    static char linha[N_ASSIN * 2 + 64];
    while (n_ref < MAX_TELAS && fgets(linha, sizeof linha, f)) {
        if (linha[0] == '#' || linha[0] == '\n') continue;
        assin_t *s = &ref[n_ref];
        char *esp = strchr(linha, ' ');
        if (!esp || strlen(esp + 1) < N_ASSIN * 2) continue;
        *esp = 0;
        snprintf(s->nome, sizeof s->nome, "%.23s", linha);
        for (int i = 0; i < N_ASSIN; i++) s->a[i] = (uint8_t)(hex(esp[1 + 2 * i]) * 16 + hex(esp[2 + 2 * i]));
        n_ref++;
    }
    fclose(f);
    return true;
}

static bool grava_ref(const char *caminho)
{
    FILE *f = fopen(caminho, "w");
    if (!f) return false;
    fprintf(f, "# Referencia do teste visual (tests/telas.c): a cor media de cada bloco de %dx%d\n"
               "# de cada tela, em RGB. Refaca com: build/telas tests/telas_ref.txt --atualiza\n", BLOCO, BLOCO);
    for (int t = 0; t < n_nova; t++) {
        fprintf(f, "%s ", nova[t].nome);
        for (int i = 0; i < N_ASSIN; i++) fprintf(f, "%02x", nova[t].a[i]);
        fputc('\n', f);
    }
    fclose(f);
    return true;
}

static void compara(void)
{
    for (int t = 0; t < n_nova; t++) {
        const assin_t *s = &nova[t], *r = NULL;
        for (int k = 0; k < n_ref; k++)
            if (!strcmp(ref[k].nome, s->nome)) r = &ref[k];
        if (!r) { printf("%-14s sem referencia\n", s->nome); falhas++; continue; }
        int mudou = 0, pior = 0, pior_i = 0;
        double soma = 0;
        for (int i = 0; i < N_ASSIN; i++) {
            int d = abs((int)s->a[i] - (int)r->a[i]);
            soma += d;
            if (d > pior) { pior = d; pior_i = i / 3; }
            if (d > BLOCO_TOL) mudou++;
        }
        double media = soma / N_ASSIN;
        bool falha = mudou > 0 || media > MEDIA_TOL;
        printf("%-14s %s  media %.2f  pior bloco %d (x %d, y %d)\n", s->nome, falha ? "MUDOU" : "ok   ", media, pior,
               pior_i % BX * BLOCO, pior_i / BX * BLOCO);
        if (falha) falhas++;
    }
    for (int k = 0; k < n_ref; k++) {
        bool achou = false;
        for (int t = 0; t < n_nova; t++) achou |= !strcmp(ref[k].nome, nova[t].nome);
        if (!achou) { printf("%-14s nao foi desenhada\n", ref[k].nome); falhas++; }
    }
}

int main(int argc, char **argv)
{
    const char *caminho = argc > 1 ? argv[1] : "tests/telas_ref.txt";
    bool atualiza = false;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--atualiza")) atualiza = true;
        else if (!strcmp(argv[i], "--fotos") && i + 1 < argc) fotos = argv[++i];
    }
    passeio();
    if (atualiza) {
        if (!grava_ref(caminho)) { printf("nao consegui gravar %s\n", caminho); return 1; }
        printf("%d telas gravadas em %s\n", n_nova, caminho);
        return 0;
    }
    if (!le_ref(caminho)) { printf("nao achei a referencia %s\n", caminho); return 1; }
    compara();
    if (falhas) {
        printf("\n%d tela(s) mudaram. Para ver: build/telas %s --fotos pasta/\n"
               "Se a mudanca foi de proposito: build/telas %s --atualiza\n", falhas, caminho, caminho);
        return 1;
    }
    printf("%d telas iguais a referencia\n", n_nova);
    return 0;
}
