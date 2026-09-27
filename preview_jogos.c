// Renderiza quadros dos jogos no PC, com o mesmo codigo do firmware.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ui/gfx.h"
#include "games/jogos.h"

static uint16_t FBUF[240 * 135];
uint16_t *display_fb(void) { return FBUF; }

static int64_t relogio;
int64_t esp_timer_get_time(void) { return relogio; }

static uint16_t presas[4];
bool keyboard_presa(uint16_t k)
{
    for (int i = 0; i < 4; i++) if (presas[i] == k) return true;
    return false;
}
static void segura(uint16_t a, uint16_t b)
{
    presas[0] = a; presas[1] = b; presas[2] = presas[3] = 0;
}
void keyboard_modo_jogo(bool on) { (void)on; }
void keyboard_solta_tudo(void) { }

static void dump(const char *nome)
{
    char path[64];
    snprintf(path, sizeof path, "build/%s.ppm", nome);
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n240 135\n255\n");
    for (int i = 0; i < 240 * 135; i++) {
        uint16_t c = FBUF[i];
        fputc((uint8_t)(((c >> 11) & 0x1F) * 255 / 31), f);
        fputc((uint8_t)(((c >> 5) & 0x3F) * 255 / 63), f);
        fputc((uint8_t)((c & 0x1F) * 255 / 31), f);
    }
    fclose(f);
}

// Segura as teclas equivalentes dos dois jogadores: so a do turno tem efeito,
// entao o roteiro funciona sem saber de quem e a vez.
static void avanca(float segundos, uint16_t p1, uint16_t p2)
{
    segura(p1, p2);
    for (int i = 0; i < (int)(segundos / 0.02f); i++) {
        relogio += 20000;
        duelo_passo(0.02f);
    }
    segura(0, 0);
}

int main(void)
{
    relogio = 1234567;

    duelo_inicia(100);
    duelo_desenha();
    dump("g1_duelo_mira");

    avanca(1.0f, 's', 'k');          // baixa a forca para um tiro curto
    avanca(0.9f, 'a', 'j');          // abaixa a mira
    duelo_desenha();
    dump("g2_duelo_mira_baixa");

    avanca(0.30f, 'q', 'o');         // segura a acao: carrega
    avanca(0.28f, 0, 0);             // soltou: disparou, projetil no ar
    duelo_desenha();
    dump("g3_duelo_voo");

    for (int t = 0; t < 6; t++) {    // troca de tiros ate abrir cratera
        avanca(6.0f, 0, 0);
        avanca(0.7f, 's', 'k');
        avanca(0.6f, 'a', 'j');
        avanca(0.30f, 'q', 'o');
        avanca(0.25f, 0, 0);
    }
    avanca(6.0f, 0, 0);
    duelo_desenha();
    dump("g4_duelo_cratera");

    motos_inicia();
    relogio += 100000;
    for (int i = 0; i < 120; i++) {
        segura(i == 30 ? 'w' : (i == 60 ? 'd' : 0), i == 45 ? 'k' : 0);
        motos_passo(0.02f);
    }
    segura(0, 0);
    motos_desenha();
    dump("g5_motos");

    for (int i = 0; i < 900 && motos_vencedor() < 0; i++) {
        segura(i % 37 == 0 ? 'w' : 0, i % 53 == 0 ? 'j' : 0);
        motos_passo(0.02f);
    }
    motos_desenha();
    dump("g6_motos_fim");

    puts("ok");
    return 0;
}
