// DADO EM CASA para PC: o mesmo jogo do Cardputer, numa janela.
//
// O jogo desenha num framebuffer RGB565 de 240x135, exatamente como no
// aparelho. Aqui esse quadro vira uma textura, ampliada em escala inteira
// para os pixels ficarem nitidos; o que sobra da janela fica na cor das
// faixas da mesa.
//
// Teclas do PC alem das do jogo: F11 ou Alt+Enter alternam tela cheia,
// F12 salva uma foto da tela.
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
#include "board.h"
#include "hal/display.h"
#include "games/jogos.h"
#include "esp_timer.h"
#include "plataforma.h"
#include "icone.h"

#define TITULO "Dado em Casa"
#define C_FAIXA (Color){ 16, 16, 15, 255 }      // C_BARRA do dado.c

static uint16_t fb[LCD_W * LCD_H];
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

static bool apertou(int k) { return IsKeyPressed(k) || IsKeyPressedRepeat(k); }

static void alterna_tela_cheia(void)
{
    ToggleBorderlessWindowed();
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

    if (IsKeyPressed(KEY_F11) || (alt && IsKeyPressed(KEY_ENTER))) alterna_tela_cheia();
    if (IsKeyPressed(KEY_F12)) {
        char nome[48];
        snprintf(nome, sizeof nome, "dado_%lld.png", (long long)time(NULL));
        TakeScreenshot(nome);
    }
    for (unsigned i = 0; i < sizeof ESPECIAL / sizeof ESPECIAL[0]; i++) {
        if (alt && ESPECIAL[i].jogo == J_ENTER) continue;
        if (apertou(ESPECIAL[i].rl)) tecla(ESPECIAL[i].jogo);
    }
    // Letras, numeros e pontuacao chegam ja traduzidos pelo layout do
    // teclado (ABNT2, US...). O jogo so conhece ASCII.
    for (int c = GetCharPressed(); c > 0; c = GetCharPressed())
        if (c >= 0x20 && c < 0x7F) tecla(c);
}

// Janela inicial: o maior multiplo inteiro de 240x135 que cabe em 85% do
// monitor.
static void dimensiona_janela(void)
{
    int m = GetCurrentMonitor();
    int mw = GetMonitorWidth(m), mh = GetMonitorHeight(m);
    if (mw <= 0 || mh <= 0) return;           // monitor desconhecido: fica 4x
    int k = 1;
    while (LCD_W * (k + 1) <= mw * 85 / 100 && LCD_H * (k + 1) <= mh * 85 / 100) k++;
    int w = LCD_W * k, h = LCD_H * k;
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
    float k = (float)(int)(W / LCD_W < H / LCD_H ? W / LCD_W : H / LCD_H);
    if (k < 1) k = W / LCD_W < H / LCD_H ? W / LCD_W : H / LCD_H;
    float w = LCD_W * k, h = LCD_H * k;
    return (Rectangle){ (float)(int)((W - w) / 2), (float)(int)((H - h) / 2), w, h };
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(LCD_W * 4, LCD_H * 4, TITULO);
    SetWindowMinSize(LCD_W, LCD_H);
    SetExitKey(KEY_NULL);                  // ESC e do jogo, nao fecha a janela
    dimensiona_janela();
    poe_icone();
    SetTargetFPS(60);

    InitAudioDevice();
    som_inicia();

    Image img = {
        .data = fb, .width = LCD_W, .height = LCD_H,
        .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R5G6B5,
    };
    Texture2D tela = LoadTextureFromImage(img);
    SetTextureFilter(tela, TEXTURE_FILTER_POINT);

    dado_inicia(0);
    while (!WindowShouldClose()) {
        le_teclado();
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;          // janela arrastada: nada de salto
        dado_passo(dt);
        dado_desenha();
        UpdateTexture(tela, fb);

        BeginDrawing();
        ClearBackground(C_FAIXA);
        DrawTexturePro(tela, (Rectangle){ 0, 0, LCD_W, LCD_H }, area_do_jogo(),
                       (Vector2){ 0, 0 }, 0, WHITE);
        EndDrawing();
    }

    UnloadTexture(tela);
    som_encerra();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
