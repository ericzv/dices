// Simulador do console numa janela do PC: o mesmo console e o mesmo jogo do
// firmware, com o mouse fazendo o dedo e o som de verdade (desktop/som.c).
//
//   Esc: botao de inicio (o "PS")    F12: foto da tela (console_*.png)
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include "console.h"
#include "sistema_pc.h"

// Os nomes KEY_* do jogo colidem com os do raylib: o console nao os usa aqui.
#include "raylib.h"

void som_inicia(void);
void som_atualiza(float dt);

void pc_volume_mudou(int pct) { if (IsAudioDeviceReady()) SetMasterVolume(pct / 100.0f); }

static uint16_t px[TELA_W * TELA_H];
static tela_t tela = { px, TELA_W, TELA_H };

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(TELA_W, TELA_H, "Console (simulador)");
    int m = GetCurrentMonitor();
    float k = 1;
    while (TELA_W * k > GetMonitorWidth(m) * 0.9f || TELA_H * k > GetMonitorHeight(m) * 0.9f) k -= 0.05f;
    if (k < 0.3f) k = 0.3f;
    SetWindowSize((int)(TELA_W * k), (int)(TELA_H * k));
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    som_inicia();

    mkdir("console_salvos", 0755);
    pc_pasta_salva("console_salvos");
    console_inicia();
    console_toque_identidade();            // o mouse ja fala em pixels da tela
    pc_volume_mudou(pc_volume);

    Image img = { .data = px, .width = TELA_W, .height = TELA_H, .mipmaps = 1,
                  .format = PIXELFORMAT_UNCOMPRESSED_R5G6B5 };
    Texture2D tex = LoadTextureFromImage(img);
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        float ew = GetScreenWidth() / (float)TELA_W, eh = GetScreenHeight() / (float)TELA_H;
        float e = ew < eh ? ew : eh;
        float ox = (GetScreenWidth() - TELA_W * e) / 2, oy = (GetScreenHeight() - TELA_H * e) / 2;
        Vector2 mp = GetMousePosition();
        int mx = (int)((mp.x - ox) / e), my = (int)((mp.y - oy) / e);
        console_toque(IsMouseButtonDown(MOUSE_BUTTON_LEFT), mx, my);
        if (IsKeyPressed(KEY_ESCAPE)) console_botao();
        console_passo(dt);
        som_atualiza(dt);
        if (console_desenha(&tela)) UpdateTexture(tex, px);

        BeginDrawing();
        ClearBackground(BLACK);
        Rectangle src = { 0, 0, TELA_W, TELA_H };
        if (pc_gira) { src.width = -TELA_W; src.height = -TELA_H; }   // a imagem de cabeca para baixo
        DrawTexturePro(tex, src, (Rectangle){ ox, oy, TELA_W * e, TELA_H * e }, (Vector2){ 0, 0 }, 0, WHITE);
        // O brilho: um veu escuro por cima.
        DrawRectangle((int)ox, (int)oy, (int)(TELA_W * e), (int)(TELA_H * e),
                      (Color){ 0, 0, 0, (unsigned char)((100 - pc_brilho) * 2) });
        EndDrawing();
        if (IsKeyPressed(KEY_F12)) {
            char nome[48];
            snprintf(nome, sizeof nome, "console_%lld.png", (long long)time(NULL));
            TakeScreenshot(nome);
        }
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
