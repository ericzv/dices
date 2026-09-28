// Teste de host: joga partidas inteiras com teclas ao acaso, sem janela, e
// confere que o jogo nunca trava nem sai do trilho.
//
//   simula [partidas] [--fotos DIR] [--ia-forte]
//
// Com --fotos, salva em DIR um .ppm da primeira vez que cada tela aparece.
// Compile com -fsanitize=address,undefined para pegar erro de memoria.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/gfx.h"
#include "hal/display.h"
#include "hal/keyboard.h"
#include "hal/som.h"
#include "games/jogos.h"
#include "esp_timer.h"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }

static int64_t relogio = 1;
int64_t esp_timer_get_time(void) { return relogio; }

void som_toca(int s) { (void)s; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }

static uint32_t estado = 2463534242u;
static uint32_t sorteio(void)
{
    estado ^= estado << 13; estado ^= estado >> 17; estado ^= estado << 5;
    return estado;
}

// Teclas que o jogo entende, com peso: ENTER e TAB fazem o jogo andar.
static const int TECLAS[] = {
    KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_TAB, KEY_TAB,
    KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_BKSP, KEY_ESC,
    'x', 'a', 'd', 'w', 's', 'm', 'i', '1', '2', '3', '4', '5', '6', '7',
};

static void tecla(int k)
{
    key_event_t ev = { (uint16_t)k };
    dado_tecla(&ev);
}

static void foto(const char *dir, int fase)
{
    char path[512];
    snprintf(path, sizeof path, "%s/fase%02d.ppm", dir, fase);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", GFX_W, GFX_H);
    for (int i = 0; i < GFX_W * GFX_H; i++) {
        uint16_t c = fb[i];
        fputc((((c >> 11) & 0x1F) * 255) / 31, f);
        fputc((((c >> 5) & 0x3F) * 255) / 63, f);
        fputc(((c & 0x1F) * 255) / 31, f);
    }
    fclose(f);
}

static int falhas;
#define CONFERE(cond, ...) do { if (!(cond)) { \
    fprintf(stderr, "FALHOU %s: ", #cond); fprintf(stderr, __VA_ARGS__); \
    fputc('\n', stderr); falhas++; } } while (0)

static void confere_estado(int partida)
{
    for (int j = 0; j < 2; j++) {
        CONFERE(dado_dbg_fichas(j) >= 0, "partida %d jogador %d fichas %d", partida, j, dado_dbg_fichas(j));
        CONFERE(dado_dbg_col(j) >= 4 && dado_dbg_col(j) <= 24, "partida %d colecao %d", partida, dado_dbg_col(j));
        CONFERE(dado_dbg_pool(j) >= 0 && dado_dbg_pool(j) <= 7, "partida %d mao %d", partida, dado_dbg_pool(j));
        CONFERE(dado_dbg_lancados(j) >= 0 && dado_dbg_lancados(j) <= 4, "partida %d lancados %d", partida, dado_dbg_lancados(j));
    }
    CONFERE(dado_dbg_rodada() <= 16, "partida %d rodada %d", partida, dado_dbg_rodada());
    CONFERE(dado_dbg_pote() >= 0, "partida %d pote %d", partida, dado_dbg_pote());
}

int main(int argc, char **argv)
{
    int partidas = 50;
    const char *dir_fotos = NULL;
    bool ia_forte = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--fotos") && i + 1 < argc) dir_fotos = argv[++i];
        else if (!strcmp(argv[i], "--ia-forte")) ia_forte = true;
        else partidas = atoi(argv[i]);
    }
    // A IA pensa jogando rodadas de mentira; aqui ela pensa pouco, para as
    // partidas andarem rapido (--ia-forte usa o esforco do jogo de verdade).
    dado_dbg_ia_esforco(ia_forte ? 1 : 0);

    // Tres mesas se revezam: duas pessoas, pessoa contra IA, IA contra IA.
    static const int MESAS[3] = { 0, 2, 3 };
    int vitorias[3] = { 0 }, rodadas = 0, ia_ganhou = 0, ia_jogou = 0;
    bool fotografada[32] = { false };
    const float dt = 1.0f / 60;

    for (int p = 0; p < partidas && !falhas; p++) {
        relogio = 1000003LL * (p + 1);
        dado_inicia(p);
        // Metade das partidas comeca com bolsas sorteadas entre todos os
        // tipos, para os dados raros aparecerem logo.
        if (p % 2) {
            for (int j = 0; j < 2; j++) {
                int tipos[12];
                for (int i = 0; i < 12; i++) tipos[i] = (int)(sorteio() % (uint32_t)dado_dbg_n_tipos());
                dado_dbg_bolsa(j, tipos, 12);
            }
        }
        long passos = 0;
        while (dado_vencedor() < 0) {
            if (++passos > 400000) {
                fprintf(stderr, "FALHOU partida %d parada na fase %d\n", p, dado_dbg_fase());
                return 1;
            }
            if (sorteio() % 2) tecla(TECLAS[sorteio() % (sizeof TECLAS / sizeof TECLAS[0])]);
            if (dado_dbg_fase() != 8) dado_dbg_cpu(MESAS[p % 3]);   // 8: abertura
            int quadros = 1 + (int)(sorteio() % 30);
            for (int q = 0; q < quadros; q++) {
                relogio += 16667;
                dado_passo(dt);
            }
            int fase = dado_dbg_fase();
            bool nova = fase >= 0 && fase < 32 && !fotografada[fase];
            if (nova || passos % 16 == 0) dado_desenha();
            if (nova && dir_fotos) {
                foto(dir_fotos, fase);
                fotografada[fase] = true;
            }
            confere_estado(p);
            if (falhas) break;
        }
        rodadas += dado_dbg_rodada();
        vitorias[dado_vencedor()]++;
        if (MESAS[p % 3] == 2) { ia_jogou++; ia_ganhou += dado_vencedor() == 1; }
        dado_desenha();                          // a tela final tambem
    }

    if (falhas) return 1;
    if (dir_fotos)                               // e as paginas do tutorial
        for (int pag = 0; pag < 7; pag++) {
            dado_dbg_tutorial(pag);
            dado_desenha();
            foto(dir_fotos, 20 + pag);
        }
    printf("%d partidas: ERIC %d, LILI %d, empates %d, %.1f rodadas em media\n",
           partidas, vitorias[0], vitorias[1], vitorias[2],
           partidas ? (double)rodadas / partidas : 0.0);
    printf("contra teclas ao acaso, a IA venceu %d de %d\n", ia_ganhou, ia_jogou);
    return 0;
}
