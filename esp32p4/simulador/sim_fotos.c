// Simulador sem janela: liga o console, faz a primeira configuracao, abre o
// jogo, a central, os ajustes e o teclado, tocando nos botoes como um dedo
// faria, e salva uma foto (.ppm) de cada tela. Serve de teste: se alguma tela
// travar ou o toque cair no lugar errado, a foto mostra.
//
//   console_fotos PASTA
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"
#include "sistema_pc.h"
#include "games/jogos.h"
#include "hal/keyboard.h"

void pc_volume_mudou(int pct) { (void)pct; }
void som_toca(int s) { (void)s; }
bool som_liga(bool on) { return on; }
bool som_ligado(void) { return true; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }
void som_rufo_moeda(void) {}
void som_giro(bool on) { (void)on; }
bool som_musica(bool on) { return on; }

static uint16_t px[TELA_W * TELA_H];
static tela_t tela = { px, TELA_W, TELA_H };
static const char *pasta;
static int n_fotos, n_quadros;

static void quadros(int n)
{
    for (int i = 0; i < n; i++) {
        console_passo(1.0f / 60);
        if (console_desenha(&tela)) n_quadros++;
    }
}

static void foto(const char *nome)
{
    char p[512];
    snprintf(p, sizeof p, "%s/%02d_%s.ppm", pasta, ++n_fotos, nome);
    FILE *f = fopen(p, "wb");
    if (!f) { perror(p); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", TELA_W, TELA_H);
    for (int i = 0; i < TELA_W * TELA_H; i++) {
        uint16_t c = px[i];
        unsigned char rgb[3] = { (unsigned char)(((c >> 11) & 31) * 255 / 31),
                                 (unsigned char)(((c >> 5) & 63) * 255 / 63),
                                 (unsigned char)((c & 31) * 255 / 31) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    printf("foto %s\n", p);
}

static void toca(int x, int y)
{
    console_toque(true, x, y);
    quadros(2);
    console_toque(true, x, y);
    quadros(2);
    console_toque(false, x, y);
    console_toque(false, x, y);
    quadros(4);
}

static void segura(int x, int y, float s)
{
    for (int i = 0; i < (int)(s * 60); i++) { console_toque(true, x, y); quadros(1); }
    console_toque(false, x, y);
    console_toque(false, x, y);
    quadros(4);
}

static void tecla_jogo(int k)
{
    key_event_t ev = { (uint16_t)k };
    dado_tecla(&ev);
    quadros(20);
}

int main(int argc, char **argv)
{
    pasta = argc > 1 ? argv[1] : ".";
    pc_pasta_salva(NULL);
    console_inicia();

    quadros(40);
    foto("abertura");
    quadros(140);
    foto("calibra");
    // Os quatro alvos, com o toque cru igual a tela (e um quinto de folga).
    static const int ALVO[4][2] = { { 110, 110 }, { 1170, 110 }, { 1170, 690 }, { 110, 690 } };
    for (int i = 0; i < 4; i++) toca(ALVO[i][0] + 3, ALVO[i][1] - 2);
    quadros(20);
    foto("orienta");
    toca(TELA_W / 2 - 185, 534);                // "Esta certa"
    quadros(30);
    foto("inicio");

    toca(200, 703);                             // Jogar
    quadros(150);
    foto("jogo_menu");
    toca(140, 760);                             // Inicio (barra): central
    quadros(30);
    foto("central");
    toca(1180, 508);                            // engrenagem da central
    quadros(30);
    foto("ajustes");
    toca(80, 64);                               // volta para a central
    quadros(30);
    toca(640, 626);                             // Ir para o inicio
    quadros(30);
    foto("inicio_pausado");
    toca(200, 703);                             // Continuar
    quadros(20);

    // Uma partida rapida contra o aparelho, pelo teclado do jogo.
    tecla_jogo(KEY_ENTER);                      // 1 jogador
    tecla_jogo(KEY_ENTER);                      // partida rapida
    tecla_jogo(KEY_ENTER);                      // nivel
    quadros(240);
    foto("partida");
    segura(320, 640, 0.9f);                     // segurar: detalhes do que esta embaixo
    quadros(10);
    foto("segurar");
    toca(640, 360);
    quadros(10);

    // Modo Desafiante: criar perfil pede o teclado da tela.
    tecla_jogo(KEY_ESC);
    tecla_jogo(KEY_ESC);
    quadros(60);
    tecla_jogo('1');                            // 1 jogador
    tecla_jogo('2');                            // Modo Desafiante
    tecla_jogo(KEY_ENTER);                      // perfil vazio: criar
    quadros(30);
    toca(66 + 52, 468 + 24 + 32 + 76);          // tecla Q
    toca(66 + 52 + 116 * 2, 468 + 24 + 32 + 76); // E
    quadros(20);
    foto("teclado");

    printf("%d fotos, %d quadros desenhados\n", n_fotos, n_quadros);
    return 0;
}
