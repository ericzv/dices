// DADO EM CASA para PC: o mesmo jogo do Cardputer, numa janela.
//
// O jogo desenha num framebuffer RGB565 de 640x360. Aqui esse quadro vira uma textura, ampliada em escala inteira
// para os pixels ficarem nitidos; o que sobra da janela fica na cor das
// faixas da mesa.
//
// Teclas do PC alem das do jogo: F11 ou Alt+Enter alternam tela cheia,
// N liga e desliga a musica, F12 salva uma foto da tela.
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// Os nomes KEY_* do jogo colidem com os do raylib: guardamos os valores do
// jogo com outro nome antes de incluir o raylib.
#include "hal/keyboard.h"
enum {
    J_BKSP = KEY_BKSP, J_TAB = KEY_TAB, J_ENTER = KEY_ENTER, J_ESC = KEY_ESC,
    J_UP = KEY_UP, J_DOWN = KEY_DOWN, J_LEFT = KEY_LEFT, J_RIGHT = KEY_RIGHT,
};
#undef KEY_BKSP
#undef KEY_TAB
#undef KEY_ENTER
#undef KEY_ESC
#undef KEY_UP
#undef KEY_DOWN
#undef KEY_LEFT
#undef KEY_RIGHT

#include "raylib.h"
#include "ui/gfx.h"
#include "hal/display.h"
#include "games/jogos.h"
#include "esp_timer.h"
#include "plataforma.h"
#include "icone.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define TITULO "Dado em Casa"
#define C_FAIXA (Color){ 16, 16, 15, 255 }      // C_BARRA do dado.c

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }

// Relogio de parede, para cada partida aberta ter outra semente.
int64_t esp_timer_get_time(void)
{
    return (int64_t)time(NULL) * 1000000 + (int64_t)(GetTime() * 1e6);
}

static void tecla(int k)
{
    key_event_t ev = { (uint16_t)k };
    dado_tecla(&ev);
}

static void alterna_tela_cheia(void)
{
#ifdef __EMSCRIPTEN__
    ToggleFullscreen();
#else
    ToggleBorderlessWindowed();
#endif
}

static void le_teclado(void)
{
    static const struct { int rl, jogo; } ESPECIAL[] = {
        { KEY_ENTER, J_ENTER }, { KEY_KP_ENTER, J_ENTER },
        { KEY_TAB, J_TAB },     { KEY_BACKSPACE, J_BKSP }, { KEY_ESCAPE, J_ESC },
        { KEY_UP, J_UP },       { KEY_DOWN, J_DOWN },
        { KEY_LEFT, J_LEFT },   { KEY_RIGHT, J_RIGHT },
    };
    bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
    unsigned n = sizeof ESPECIAL / sizeof ESPECIAL[0];

    // A fila de teclas do raylib guarda cada toque, mesmo o que aperta e
    // solta entre dois quadros: nada se perde num computador lento.
    for (int k = GetKeyPressed(); k; k = GetKeyPressed()) {
        if (k == KEY_F11 || (alt && (k == KEY_ENTER || k == KEY_KP_ENTER))) {
            alterna_tela_cheia();
            continue;
        }
        if (k == KEY_F12) {
            char nome[48];
            snprintf(nome, sizeof nome, "dado_%lld.png", (long long)time(NULL));
            TakeScreenshot(nome);
            continue;
        }
        for (unsigned i = 0; i < n; i++) if (ESPECIAL[i].rl == k) tecla(ESPECIAL[i].jogo);
    }
    // Tecla segurada repete (setas andando pelo menu, por exemplo).
    for (unsigned i = 0; i < n; i++)
        if (IsKeyPressedRepeat(ESPECIAL[i].rl) && !(alt && ESPECIAL[i].jogo == J_ENTER))
            tecla(ESPECIAL[i].jogo);
    // Letras, numeros e pontuacao chegam ja traduzidos pelo layout do
    // teclado (ABNT2, US...). O jogo so conhece ASCII.
    for (int c = GetCharPressed(); c > 0; c = GetCharPressed()) {
        if ((c == 'n' || c == 'N') && !dado_digitando()) { som_musica(!som_musica_ligada()); continue; }
        if (c >= 0x20 && c < 0x7F) tecla(c);
    }
}

// Janela inicial: o maior multiplo inteiro de 240x135 que cabe em 85% do
// monitor.
static void dimensiona_janela(void)
{
    int m = GetCurrentMonitor();
    int mw = GetMonitorWidth(m), mh = GetMonitorHeight(m);
    if (mw <= 0 || mh <= 0) return;           // monitor desconhecido: fica 2x
    int k = 1;
    while (GFX_W * (k + 1) <= mw * 85 / 100 && GFX_H * (k + 1) <= mh * 85 / 100) k++;
    int w = GFX_W * k, h = GFX_H * k;
    SetWindowSize(w, h);
    Vector2 p = GetMonitorPosition(m);
    SetWindowPosition((int)p.x + (mw - w) / 2, (int)p.y + (mh - h) / 2);
}

static void poe_icone(void)
{
    Image ic = {
        .data = (void *)ICONE_RGBA, .width = ICONE_LADO, .height = ICONE_LADO,
        .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    SetWindowIcon(ic);
}

// Escala inteira quando a janela comporta; abaixo de 1x, encolhe como der.
static Rectangle area_do_jogo(void)
{
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    float k = (float)(int)(W / GFX_W < H / GFX_H ? W / GFX_W : H / GFX_H);
    if (k < 1) k = W / GFX_W < H / GFX_H ? W / GFX_W : H / GFX_H;
    float w = GFX_W * k, h = GFX_H * k;
    return (Rectangle){ (float)(int)((W - w) / 2), (float)(int)((H - h) / 2), w, h };
}

static Texture2D tela;

// Um quadro do jogo: teclado, passo, desenho e a textura ampliada na janela.
static void quadro(void)
{
    le_teclado();
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;          // janela arrastada: nada de salto
    dado_passo(dt);
    som_atualiza(dt);
    dado_desenha();
    UpdateTexture(tela, fb);

    BeginDrawing();
    ClearBackground(C_FAIXA);
    DrawTexturePro(tela, (Rectangle){ 0, 0, GFX_W, GFX_H }, area_do_jogo(),
                   (Vector2){ 0, 0 }, 0, WHITE);
    EndDrawing();
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(GFX_W * 2, GFX_H * 2, TITULO);
    SetWindowMinSize(GFX_W, GFX_H);
    SetExitKey(KEY_NULL);                  // ESC e do jogo, nao fecha a janela
#ifndef __EMSCRIPTEN__
    dimensiona_janela();
    poe_icone();
#endif

    InitAudioDevice();
    som_inicia();

    Image img = {
        .data = fb, .width = GFX_W, .height = GFX_H,
        .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R5G6B5,
    };
    tela = LoadTextureFromImage(img);
    SetTextureFilter(tela, TEXTURE_FILTER_POINT);

    dado_inicia(0);
#ifdef __EMSCRIPTEN__
    // No navegador quem manda no ritmo e a pagina: um quadro por repintura.
    emscripten_set_main_loop(quadro, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) quadro();

    UnloadTexture(tela);
    som_encerra();
    CloseAudioDevice();
    CloseWindow();
#endif
    return 0;
}
