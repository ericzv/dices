// O cache dos dados pousados (desenho_dado.inc, dc_*) nao pode mudar um pixel.
// Este teste joga o mesmo roteiro em dois processos - o filho com o cache
// desligado, o pai com ele ligado -, desenha TODOS os quadros nos dois e
// compara a tela inteira, byte a byte. Compara tambem, a cada quadro, o
// rascunho que o ultimo dado desenhado deixou para o seguinte (dc_resto_t):
// quem copia do cache tem de deixar tudo como o desenho de verdade deixaria.
//
// O roteiro: partidas inteiras do Desafiante com a IA dos dois lados (vistas
// como a pessoa ve e como a maquina joga), dois na mesma mesa e pessoa contra
// maquina com teclas ao acaso (a montagem, a aposta, os paineis, a letra
// maior), cenas montadas do fim da rodada (jackpot, multiplicadores, ataques,
// Oculto/Ficha Coringa/Carrossel, mesa cheia de adesivos, o tremor do
// Terremoto), todos os tipos de dado pousados (validos e anulados, com
// adesivo parado e animado), a vinheta do dado muito bom (o dado pousado
// CRESCE), a hora da verdade e a cena do fim da partida.
//
//   cache_dados                o roteiro curto (o ctest)
//   cache_dados --tudo         o roteiro inteiro (minutos)
//   cache_dados --fotos DIR    grava em DIR os primeiros quadros diferentes
//   cache_dados --tempo        mede: ms por quadro, sem e com o cache
//
// So no Linux e no macOS (fork e pipe).
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
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
void som_rufo_de(int q) { (void)q; }
void som_rufo_forca(float f) { (void)f; }
void som_giro(bool on) { (void)on; }
void som_motor(bool on) { (void)on; }
void som_suspense(bool on) { (void)on; }
void som_cadeia(bool on) { (void)on; }
void som_chefe(int q, bool t) { (void)q; (void)t; }
static int volume_[SOM_CANAIS] = { SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS };
void som_volume(int c, int n) { if (c >= 0 && c < SOM_CANAIS) volume_[c] = n < 0 ? 0 : n > SOM_NIVEIS ? SOM_NIVEIS : n; }
int som_volume_de(int c) { return c >= 0 && c < SOM_CANAIS ? volume_[c] : 0; }

#include "salva_mem.h"

// ---- Os dois processos ------------------------------------------------------
enum { FILHO, PAI, SOZINHO };                // sem cache, escreve | com cache, compara | so desenha (a medida)
static int papel = SOZINHO, cano = -1;
static const char *fotos;
static uint16_t fb_filho[GFX_W * GFX_H];
static long n_quadros, n_q_dif, n_px_dif, n_resto_dif;
static long q_hora, q_vinheta, q_cresce, q_tremor, q_anulado, q_montagem, q_voo, q_fim, q_dois, q_letra;
static double t_desenho;

static double agora_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}
static void foto(const char *nome, const uint16_t *px)
{
    char p[512];
    snprintf(p, sizeof p, "%s/%s.ppm", fotos, nome);
    FILE *f = fopen(p, "wb");
    if (!f) { printf("nao consegui gravar %s\n", p); return; }
    fprintf(f, "P6\n%d %d\n255\n", GFX_W, GFX_H);
    for (int i = 0; i < GFX_W * GFX_H; i++) {
        uint16_t c = px[i];
        fputc(((c >> 11) & 31) * 255 / 31, f);
        fputc(((c >> 5) & 63) * 255 / 63, f);
        fputc((c & 31) * 255 / 31, f);
    }
    fclose(f);
}
// O que este quadro tem (para o relatorio dizer o que foi coberto).
static void conta(void)
{
    bool mesa_ = fase == F_ORDEM || fase == F_APOSTA || fase == F_ROLANDO || fase == F_ARRUMA || fase == F_RESULTADO
              || fase == F_JOGA || fase == F_VEREDITO || fase == F_EFEITOS || fase == F_FIM;
    if (!mesa_) return;
    if (t_hora > 0.05f) q_hora++;
    if (t_vinheta >= 0) q_vinheta++;
    if (t_vinheta >= 0 && vinheta_pop(vinheta_j, vinheta_k) > 0) q_cresce++;
    if (t_tremor > 0 || t_congela > 0) q_tremor++;
    bool anulado = false;
    for (int j = 0; j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) anulado |= F[j].est[k] != V_VALIDO;
    q_anulado += anulado;
    if (fase == F_ORDEM) {
        q_montagem++;
        bool voo_ = false;
        for (int i = 0; i < MAO; i++) voo_ |= ordv_desde[i] >= 0;
        q_voo += voo_;
    }
    if (fase == F_FIM) q_fim++;
    if (modo == M_DOIS) q_dois++;
    if (gfx_letra_e_maior()) q_letra++;
}
static bool manda(const void *p, size_t tam)
{
    for (size_t n = 0; n < tam; ) {
        ssize_t r = write(cano, (const char *)p + n, tam - n);
        if (r <= 0) return false;
        n += (size_t)r;
    }
    return true;
}
static bool recebe(void *p, size_t tam)
{
    for (size_t n = 0; n < tam; ) {
        ssize_t r = read(cano, (char *)p + n, tam - n);
        if (r <= 0) return false;
        n += (size_t)r;
    }
    return true;
}
static void quadro(void)
{
    double a = agora_ms();
    dado_desenha();
    t_desenho += agora_ms() - a;
    n_quadros++;
    if (papel == SOZINHO) return;
    dc_resto_t resto, resto_filho;                 // o que o ultimo dado deixou para o seguinte
    memset(&resto, 0, sizeof resto);
    dc_resto(&resto, true);
    if (papel == FILHO) {
        if (!manda(fb, sizeof fb) || !manda(&resto, sizeof resto)) _exit(3);
        return;
    }
    conta();
    if (!recebe(fb_filho, sizeof fb_filho) || !recebe(&resto_filho, sizeof resto_filho)) {
        printf("FALHOU: o processo sem cache parou no quadro %ld\n", n_quadros);
        exit(3);
    }
    if (memcmp(&resto, &resto_filho, sizeof resto)) {
        if (!n_resto_dif) printf("  RESTO DIFERENTE no quadro %ld (fase %d, modo %d)\n", n_quadros, fase, modo);
        n_resto_dif++;
    }
    if (!memcmp(fb, fb_filho, sizeof fb)) return;
    long d = 0;
    int x0 = GFX_W, y0 = GFX_H, x1 = -1, y1 = -1;
    for (int i = 0; i < GFX_W * GFX_H; i++) {
        if (fb[i] == fb_filho[i]) continue;
        int x = i % GFX_W, y = i / GFX_W;
        d++;
        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
    }
    if (n_q_dif < 5) {
        printf("  DIFERENTE no quadro %ld: %ld pixels em (%d,%d)-(%d,%d), fase %d, modo %d\n", n_quadros, d, x0, y0, x1, y1, fase, modo);
        if (fotos) {
            char nm[64];
            snprintf(nm, sizeof nm, "dif_%ld_com", n_quadros); foto(nm, fb);
            snprintf(nm, sizeof nm, "dif_%ld_sem", n_quadros); foto(nm, fb_filho);
        }
    }
    n_q_dif++;
    n_px_dif += d;
}
static void anda(void) { dado_passo(1 / 60.0f); relogio += 16667; }
static void passa(int n) { for (int i = 0; i < n; i++) { quadro(); anda(); } }

// ---- O roteiro --------------------------------------------------------------
static uint32_t acaso_e = 2463534242u;
static uint32_t acaso(void) { acaso_e ^= acaso_e << 13; acaso_e ^= acaso_e >> 17; acaso_e ^= acaso_e << 5; return acaso_e; }

static void perfil_cheio(void)
{
    snprintf(perfis[0].nome, sizeof perfis[0].nome, "TESTE");
    perfis[0].nivel = 10; perfis[0].marcos = 3; perfil_atual = 0; perfis_lidos = true;
    perfis[0].bolsas = 0xFF; perfis[0].conq = ~0ull;
}
typedef struct { uint8_t t, v, l, a; } d_t;  // um dado na mesa: o tipo, o que tirou, e o adesivo a no lado l
// Poe um dado pousado na fila de j, pelo caminho do jogo (pousa).
static void poe(int j, int t, int v, int lado_ad, int ad)
{
    fila_t *f = &F[j];
    int k = f->lancados, i = J[j].n_col++;
    J[j].col[i] = (peca_t){ .tipo = (uint8_t)t };
    if (ad) ad_cola(&J[j].col[i], lado_ad, ad);
    J[j].onde[i] = NA_MAO;
    f->idx[k] = (int8_t)i;
    f->tipo[k] = (uint8_t)t;
    f->cru[k] = f->valor[k] = v;
    f->lado[k] = (uint8_t)lado_do_lance(t, v, 0);
    f->lancados = k + 1;
    if (f->n < f->lancados) f->n = f->lancados;
    pousa(j, k, rola_v);
}
static void monta(const d_t *a, int na, const d_t *b, int nb)
{
    for (int j = 0; j < 2; j++) {
        memset(&F[j], 0, sizeof F[j]);
        for (int k = 0; k < N_FILA; k++) F[j].idx[k] = -1;
        J[j].n_col = 0;
    }
    primeiro = 0; vez = 0;
    for (int i = 0; i < na; i++) poe(0, a[i].t, a[i].v, a[i].l, a[i].a);
    vez = 1;
    for (int i = 0; i < nb; i++) poe(1, b[i].t, b[i].v, b[i].l, b[i].a);
}
// Uma run do Desafiante ate a mesa: o nivel, o andar e (se B) a bolsa.
static void comeca_run(int dif, int andar, const uint8_t (*B)[3], int nb)
{
    modo = M_DESAFIO; cpu = 2; remoto = 0; baixo = 0;
    des_dif = dif; des_bolsa = B_PANO; des_nova_run(); t_elev = 0;
    passa(30);
    amuleto_escolhido(1);
    if (B) {
        run.n_col = 0;
        for (int i = 0; i < nb; i++) {
            peca_t p = { .tipo = B[i][0] };
            if (B[i][2]) ad_cola(&p, B[i][1], B[i][2]);
            run.col[run.n_col++] = p;
        }
    }
    run.andar = (uint8_t)andar; gera_andar(); run.passo = 1;
    des_comeca_partida();
}
// A partida inteira, a IA dos dois lados. pessoa: desenhada como a pessoa ve
// (a montagem, os botoes da aposta); senao, como a maquina joga.
static void partida(int dif, int andar, const uint8_t (*B)[3], int nb, bool pessoa, long passos)
{
    comeca_run(dif, andar, B, nb);
    for (long p = 0; p < passos; p++) {
        if (fase == F_RESULTADO && t_fase > 2.5f) trata_tecla(KEY_ENTER);
        cpu = pessoa ? 2 : 3;
        quadro();
        cpu = 3;
        anda();
        if (modo != M_DESAFIO || fase == F_FIM) break;
    }
    cpu = 2;
    if (fase == F_FIM) passa(240);                // a cena do fim da partida
}
// Teclas ao acaso por um tempo: quem joga e a pessoa (as duas, com dois na mesa).
static void teclas_ao_acaso(long passos)
{
    static const int TECLAS[] = {
        KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_TAB, KEY_TAB,
        KEY_LEFT, KEY_RIGHT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_BKSP,
        'x', 'a', 'd', 'w', 's', 'l', 'i', '1', '2', '3', '4', '5', '6', '7',
    };
    for (long p = 0; p < passos; p++) {
        if (acaso() % 9 == 0) {
            key_event_t ev = { (uint16_t)TECLAS[acaso() % (sizeof TECLAS / sizeof TECLAS[0])] };
            dado_tecla(&ev);
        }
        quadro();
        anda();
        if (fase == F_FIM && t_fim_partida > 5) break;
    }
}
static void dois_na_mesa(long passos)
{
    modo = M_DOIS; cpu = 0; remoto = 0; baixo = 0; poe_nomes(); abre_loja();
    teclas_ao_acaso(passos);
}
static void pessoa_e_maquina(long passos)
{
    modo = M_UM; cpu = 2; remoto = 0; baixo = 0; poe_nomes(); abre_loja();
    teclas_ao_acaso(passos);
}
// Uma mesa montada: fica parada um tempo e vai para o fim da rodada.
static void cena(const d_t *a, int na, const d_t *b, int nb, int espera)
{
    monta(a, na, b, nb);
    fase = F_JOGA; vez = 1; t_fase = 0;
    for (int i = 0; i < espera; i++) { fase = F_JOGA; t_ia = -100; quadro(); anda(); }
    desfecho();
    for (int p = 0; p < 60 * 40 && (fase == F_EFEITOS || fase == F_RESULTADO); p++) {
        if (fase == F_RESULTADO && t_fase > 2.5f) break;
        quadro(); anda();
    }
}
// A mesa parada, e o dado k de j pousando muito bom: o holofote, e ele cresce.
static void muito_bom(const d_t *a, int na, const d_t *b, int nb, int j, int k)
{
    monta(a, na, b, nb);
    fase = F_JOGA; vez = j; t_fase = 0; hora = 0; t_vinheta = -1;
    for (int i = 0; i < 20; i++) { fase = F_JOGA; t_ia = -100; quadro(); anda(); }
    vinheta_pouso(j, k);
    if (t_vinheta < 0 && papel == PAI) { printf("FALHOU: a vinheta nao acendeu (jogador %d, dado %d)\n", j, k); n_q_dif++; }
    for (int i = 0; i < 130; i++) { fase = F_JOGA; t_ia = -100; quadro(); anda(); }
}
// A hora da verdade: a mesa escurece em volta do dado que vai decidir.
static void hora_da_verdade(const d_t *a, int na, const d_t *b, int nb, int grau)
{
    monta(a, na, b, nb);
    fase = F_JOGA; vez = 1; t_fase = 0; t_vinheta = -1;
    hora_x = (float)SLOT_X(nb - 1); hora_y = (float)zona(1);
    for (int i = 0; i < 100; i++) {               // (a luz entra em 0,35 s, como no lance de verdade)
        fase = F_JOGA; t_ia = -100;
        quadro(); anda();
        hora = grau; t_hora = fminf(1, (i + 1) / 21.0f);
    }
    hora = 0;                                     // e sai sozinha
    for (int i = 0; i < 50; i++) { fase = F_JOGA; t_ia = -100; quadro(); anda(); }
}
// A cena do fim da partida, com a mesa como ficou.
static void fim_da_partida(const d_t *a, int na, const d_t *b, int nb, int vencedor)
{
    monta(a, na, b, nb);
    fase = F_JOGA; vez = 1; t_fase = 0;
    passa(10);
    encerra(vencedor);
    for (int i = 0; i < 300 && fase == F_FIM; i++) { quadro(); anda(); }
}

static const uint8_t B1[][3] = {
    { D_JACKPOT }, { D_COPIADOR }, { D_FENIX }, { D_OCULTO }, { D_CAPITAO }, { D_TREVO }, { D_ESPELHO }, { D_VIDRO },
    { D_SANGUESSUGA }, { D_ECO }, { D_CARROSSEL }, { D_D6, 3, AD_ELETRONICO }, { D_D8, 5, AD_HOLOGRAFICO }, { D_D6, 6, AD_DOURADO },
    { D_D12, 9, AD_REMENDO }, { D_D6 }, { D_D6 }, { D_D4 }, { D_D8 } };
static const uint8_t B2[][3] = {
    { D_TERREMOTO }, { D_EXPLOSIVO }, { D_MATRIOSKA }, { D_EGOISTA }, { D_FICHA_CORINGA }, { D_RELOGIO }, { D_DIGITAL }, { D_GELATINA },
    { D_D6, 1, AD_NEON }, { D_D6, 2, AD_PURPURINA }, { D_D6, 3, AD_LENTICULAR }, { D_D6, 4, AD_ESTRELINHA }, { D_D6, 5, AD_REFLEXIVO },
    { D_D6, 6, AD_AVESSO }, { D_D8, 7, AD_BOMBA }, { D_D8, 8, AD_CARBONO }, { D_D8, 3, AD_CHICLETE }, { D_D4, 2, AD_PATINHO },
    { D_D4, 4, AD_IMPERMEAVEL }, { D_D12, 11, AD_FERMENTO }, { D_D6, 6, AD_BIS }, { D_D8, 1, AD_MIRA } };
static const uint8_t B3[][3] = {
    { D_D12, 12, AD_REMENDO }, { D_D12 }, { D_D8, 8, AD_CHUMBO }, { D_D8 }, { D_D20 }, { D_D20, 20, AD_SELO }, { D_D6, 6, AD_GORJETA },
    { D_D6 }, { D_D6 }, { D_D4 }, { D_CAPITAO }, { D_VAMPIRO }, { D_BURACO }, { D_ESTRELA }, { D_PISCA }, { D_BALAO }, { D_PANETONE } };
#define N_DE(B) ((int)(sizeof B / sizeof B[0]))

#define CENA(A, Bb, espera) cena(A, (int)(sizeof A / sizeof A[0]), Bb, (int)(sizeof Bb / sizeof Bb[0]), espera)
static long q_parte;
static void parte(const char *nome)
{
    if (papel == PAI) printf("  %-34s %6ld quadros\n", nome, n_quadros - q_parte);
    q_parte = n_quadros;
}
// As mesas montadas que valem para os dois roteiros.
static void cenas_montadas(void)
{
    comeca_run(7, 2, NULL, 0);
    passa(20);
    cpu = 2;
    { d_t a[] = { { D_JACKPOT, 6 }, { D_D6, 6 }, { D_CAPITAO, 6 }, { D_D8, 7 } }, b[] = { { D_VAMPIRO, 3 }, { D_PIRATA, 4 }, { D_D6, 5 } }; CENA(a, b, 60); }
    { d_t a[] = { { D_D6, 3, 3, AD_ELETRONICO }, { D_D8, 5, 5, AD_HOLOGRAFICO }, { D_ESPERANCA, 6 }, { D_D12, 9 } },
          b[] = { { D_FOGO, 2 }, { D_MALDICAO, 3 }, { D_D8, 6 }, { D_MARTELO, 7 } }; CENA(a, b, 60); }
    { d_t a[] = { { D_CHECKPOINT, 2 }, { D_D6, 3 }, { D_MARTIR, 4 }, { D_D8, 6 } }, b[] = { { D_OURICO, 3 }, { D_CARRASCO, 5 }, { D_GATUNO, 6 } }; CENA(a, b, 60); }
    { d_t a[] = { { D_OCULTO, 7 }, { D_D6, 2 }, { D_D6, 1 }, { D_LEQUE, 5 } }, b[] = { { D_ESPELHO, 3 }, { D_FICHA_CORINGA, 1 }, { D_CARROSSEL, 1 } }; CENA(a, b, 60); }
    { d_t a[] = { { D_D6, 3, 3, AD_ELETRONICO }, { D_D8, 5, 5, AD_HOLOGRAFICO }, { D_D6, 6, 6, AD_DOURADO }, { D_D12, 9, 9, AD_NEON } },
          b[] = { { D_D6, 2, 2, AD_LENTICULAR }, { D_D8, 3, 3, AD_REFLEXIVO }, { D_D6, 4, 4, AD_ESTRELINHA }, { D_D12, 7, 7, AD_HOLOGRAFICO } }; CENA(a, b, 120); }
    { d_t a[] = { { D_D6, 3, 3, AD_REMENDO }, { D_D8, 5, 5, AD_CHUMBO }, { D_D6, 6, 6, AD_BIS }, { D_D12, 9, 9, AD_MIRA } },
          b[] = { { D_D6, 2, 2, AD_GORJETA }, { D_D8, 3, 3, AD_SELO }, { D_D6, 4, 4, AD_FRAGIL }, { D_D12, 7, 7, AD_POSTIT } }; CENA(a, b, 90); }
    { d_t a[] = { { D_TERREMOTO, 8 }, { D_D8, 8 } }, b[] = { { D_D6, 2 }, { D_D6, 5 }, { D_D8, 6 } }; CENA(a, b, 60); }
    parte("cenas do fim da rodada");
}
// Todos os tipos de dado pousados: 4 validos em cima (um com adesivo, a cada
// volta um adesivo diferente) e os mesmos 4 embaixo, os dois ultimos anulados.
static void todos_os_tipos(int espera, int passo)
{
    for (int t = 0; t + 3 < D_N; t += 4 * passo) {
        d_t a[4], b[4];
        for (int i = 0; i < 4; i++) {
            int ta = t + i, la = TIPO[ta].lados > 0 ? TIPO[ta].lados : 6;
            a[i] = (d_t){ (uint8_t)ta, (uint8_t)(la >= 4 ? 1 + i * (la - 1) / 3 : 1 + (i > 1)), 0, 0 };
            b[i] = (d_t){ (uint8_t)ta, (uint8_t)(i < 2 ? 1 + i : 1), 0, 0 };
            if (i == 1) { a[i].l = a[i].v; a[i].a = (uint8_t)(1 + (t / 4) % (AD_SUJEIRA - 1)); }
        }
        cena(a, 4, b, 4, espera);
    }
    parte("todos os tipos de dado");
}
// As cenas novas: a vinheta (o dado pousado cresce), a hora da verdade e o
// fim da partida - no Desafiante e com dois na mesa, na letra normal e na maior.
static void cenas_novas(bool tudo)
{
    for (int volta = 0; volta < (tudo ? 4 : 2); volta++) {
        bool dois = volta & 1, letra = volta >= 2;
        gfx_letra_maior(letra);
        if (dois) { modo = M_DOIS; cpu = 0; remoto = 0; baixo = 0; poe_nomes(); abre_loja(); passa(10); }
        else { comeca_run(5 + 2 * volta, 1, NULL, 0); passa(20); cpu = 2; }
        // (as filas sobem: todos valem)
        d_t a[] = { { D_D8, 8 }, { D_D12, 10, 10, AD_CHUMBO }, { D_D12, 12 }, { D_D20, 20, 20, AD_HOLOGRAFICO } };
        d_t b[] = { { D_D6, 5 }, { D_D8, 8, 8, AD_SELO }, { D_D12, 11 }, { D_D20, 15 } };
        muito_bom(a, 4, b, 4, 0, 0);
        muito_bom(a, 4, b, 4, 0, 1);               // com adesivo parado
        muito_bom(a, 4, b, 4, 1, 2);               // o de cima
        if (tudo || volta == 0) {
            muito_bom(a, 4, b, 4, 0, 3);           // com adesivo animado
            muito_bom(a, 4, b, 4, 1, 1);
            muito_bom(a, 4, b, 4, 0, 2);
        }
        hora_da_verdade(a, 4, b, 3, 1);
        if (tudo) hora_da_verdade(a, 3, b, 4, 2);
        fim_da_partida(a, 4, b, 4, volta & 1);
        if (tudo) fim_da_partida(a, 4, b, 4, 2 - (volta & 1) * 2);
    }
    gfx_letra_maior(false);
    parte("vinheta, hora da verdade e fim");
}
static void roteiro(bool tudo)
{
    natal_forcado = 0;                             // fora da epoca do Natal, qualquer que seja o dia
    dado_inicia(0);
    perfil_cheio();
    q_parte = 0;
    partida(7, 2, B1, N_DE(B1), true, tudo ? 60 * 150 : 60 * 45);  parte("Desafiante nivel 7 (a pessoa ve)");
    if (tudo) {
        partida(10, 2, B2, N_DE(B2), false, 60 * 150);             parte("Desafiante nivel 10 (a maquina)");
        partida(3, 1, NULL, 0, true, 60 * 100);                    parte("Desafiante nivel 3, a bolsa de sempre");
        partida(5, 0, B3, N_DE(B3), false, 60 * 150);              parte("Desafiante nivel 5, dados altos");
        partida(1, 0, B2, N_DE(B2), true, 60 * 100);               parte("Desafiante nivel 1");
    }
    dois_na_mesa(tudo ? 60 * 240 : 60 * 40);                       parte("dois na mesa, teclas ao acaso");
    pessoa_e_maquina(tudo ? 60 * 150 : 60 * 20);                   parte("pessoa e maquina, teclas ao acaso");
    cenas_montadas();
    todos_os_tipos(tudo ? 45 : 12, 1);
    cenas_novas(tudo);
}

// ---- A medida ---------------------------------------------------------------
// A mesa cheia, parada: 5 + 5 dados. 0: todos com adesivo animado; 1: com
// adesivos parados; 2: sem adesivo.
static void mesa_cheia(int qual)
{
    comeca_run(7, 2, NULL, 0);
    passa(20);
    cpu = 2;
    if (qual == 0) {
        d_t a[] = { { D_D6, 3, 3, AD_ELETRONICO }, { D_D8, 5, 5, AD_HOLOGRAFICO }, { D_D6, 6, 6, AD_DOURADO }, { D_D12, 9, 9, AD_NEON }, { D_D8, 8, 8, AD_PURPURINA } };
        d_t b[] = { { D_D6, 2, 2, AD_LENTICULAR }, { D_D8, 3, 3, AD_REFLEXIVO }, { D_D6, 4, 4, AD_ESTRELINHA }, { D_D12, 7, 7, AD_HOLOGRAFICO }, { D_D8, 8, 8, AD_ELETRONICO } };
        monta(a, 5, b, 5);
    } else if (qual == 1) {
        d_t a[] = { { D_D6, 3, 3, AD_REMENDO }, { D_D8, 5, 5, AD_CHUMBO }, { D_D6, 6, 6, AD_BIS }, { D_D12, 9, 9, AD_MIRA }, { D_D8, 8, 8, AD_FERMENTO } };
        d_t b[] = { { D_D6, 2, 2, AD_GORJETA }, { D_D8, 3, 3, AD_SELO }, { D_D6, 4, 4, AD_FRAGIL }, { D_D12, 7, 7, AD_POSTIT }, { D_D8, 8, 8, AD_VELCRO } };
        monta(a, 5, b, 5);
    } else {
        d_t a[] = { { D_D6, 3 }, { D_CAPITAO, 5 }, { D_D6, 6 }, { D_D12, 9 }, { D_SANGUESSUGA, 8 } };
        d_t b[] = { { D_D4, 2 }, { D_D8, 3 }, { D_ECO, 4 }, { D_D12, 7 }, { D_VIDRO, 8 } };
        monta(a, 5, b, 5);
    }
    fase = F_JOGA; vez = 1; t_fase = 0;
    for (int i = 0; i < 30; i++) { fase = F_JOGA; t_ia = -100; quadro(); anda(); }
}
static int cmp_d(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static double mediana(double *m, int n) { qsort(m, (size_t)n, sizeof m[0], cmp_d); return m[n / 2]; }
// ms por quadro (a mediana de varias voltas) de uma cena, com o cache ligado ou nao.
static double mede(int cena_, bool liga)
{
    double m[15];
    dc_liga = liga;
    natal_forcado = 0;
    dado_inicia(0);
    perfil_cheio();
    if (cena_ < 3) {                               // a mesa cheia parada
        mesa_cheia(cena_);
        for (int r = 0; r < 15; r++) {
            t_desenho = 0;
            for (int i = 0; i < 200; i++) { fase = F_JOGA; t_ia = -100; quadro(); anda(); }
            m[r] = t_desenho / 200;
        }
        return mediana(m, 15);
    }
    for (int r = 0; r < 5; r++) {                  // a partida inteira (3), dois na mesa (4), a vinheta (5)
        dado_inicia(0);
        perfil_cheio();
        acaso_e = 2463534242u;
        t_desenho = 0; n_quadros = 0;
        if (cena_ == 3) partida(7, 2, B1, N_DE(B1), true, 60 * 150);
        else if (cena_ == 4) dois_na_mesa(60 * 60);
        else {
            comeca_run(7, 1, NULL, 0); passa(20); cpu = 2;
            d_t a[] = { { D_D8, 8 }, { D_D12, 10, 10, AD_CHUMBO }, { D_D12, 12 }, { D_D20, 20 } }, b[] = { { D_D8, 8 }, { D_D12, 10 }, { D_D12, 11 }, { D_D20, 15 } };
            t_desenho = 0; n_quadros = 0;
            for (int i = 0; i < 6; i++) muito_bom(a, 4, b, 4, i & 1, i % 3);
        }
        m[r] = t_desenho / n_quadros;
    }
    return mediana(m, 5);
}
static int mede_tudo(void)
{
    static const char *const NOME[] = {
        "mesa cheia, 10 dados com adesivo animado", "mesa cheia, 10 dados com adesivo parado", "mesa cheia, 10 dados sem adesivo",
        "partida inteira do Desafiante (IA x IA)", "dois na mesa, teclas ao acaso", "a vinheta do dado muito bom (6 pousos)" };
    printf("ms por quadro (mediana)                      sem cache   com cache\n");
    for (int c = 0; c < 6; c++) {
        double sem = mede(c, false), com = mede(c, true);
        printf("%-44s %8.4f    %8.4f   (%+.0f%%)\n", NOME[c], sem, com, (com - sem) / sem * 100);
        fflush(stdout);
    }
    printf("memoria do cache: %d recortes de %d x %d, %.0f KB\n", DC_N, DC_L, DC_L, sizeof dc / 1024.0);
    return 0;
}

int main(int argc, char **argv)
{
    bool tudo = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--tudo")) tudo = true;
        else if (!strcmp(argv[i], "--fotos") && i + 1 < argc) fotos = argv[++i];
        else if (!strcmp(argv[i], "--tempo")) return mede_tudo();
    }
    int p[2];
    if (pipe(p)) { perror("pipe"); return 1; }
    fflush(stdout);
    pid_t filho = fork();
    if (filho < 0) { perror("fork"); return 1; }
    if (filho == 0) {                              // o filho: sem cache, manda cada quadro
        close(p[0]);
        cano = p[1]; papel = FILHO; dc_liga = false;
        roteiro(tudo);
        close(cano);
        _exit(0);
    }
    close(p[1]);
    cano = p[0]; papel = PAI; dc_liga = true;      // o pai: com cache, compara
    roteiro(tudo);
    char sobra;
    bool filho_acabou_junto = read(cano, &sobra, 1) == 0;
    int st = 0;
    waitpid(filho, &st, 0);
    printf("%ld quadros comparados: %ld diferentes, %ld pixels diferentes; %ld com o rascunho do dado diferente\n",
           n_quadros, n_q_dif, n_px_dif, n_resto_dif);
    printf("entre eles: %ld com a hora da verdade, %ld com a vinheta (%ld com o dado crescido), %ld com tremor ou golpe,\n"
           "  %ld com dado anulado na mesa, %ld na montagem (%ld com dado voando), %ld no fim da partida,\n"
           "  %ld com dois na mesa, %ld com a letra maior\n",
           q_hora, q_vinheta, q_cresce, q_tremor, q_anulado, q_montagem, q_voo, q_fim, q_dois, q_letra);
    printf("o cache: %ld dados copiados, %ld guardados, %ld pintados ao vivo por lerem o relogio\n", dc_copias, dc_guardados, dc_vivos);
    bool ok = true;
    if (n_q_dif) { printf("FALHOU: o cache mudou o desenho\n"); ok = false; }
    if (n_resto_dif) { printf("FALHOU: quem copia do cache nao deixa o rascunho como o desenho de verdade\n"); ok = false; }
    if (!filho_acabou_junto || !WIFEXITED(st) || WEXITSTATUS(st)) { printf("FALHOU: os dois processos nao andaram juntos\n"); ok = false; }
    if (dc_copias < n_quadros) { printf("FALHOU: o cache quase nao foi usado (o teste nao prova nada)\n"); ok = false; }
    if (!q_hora || !q_cresce || !q_tremor || !q_anulado || !q_voo || !q_fim || !q_dois || !q_letra) {
        printf("FALHOU: o roteiro deixou de passar por alguma cena\n");
        ok = false;
    }
    return ok ? 0 : 1;
}
