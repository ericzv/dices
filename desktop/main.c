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
#include <string.h>
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
#include "fonte_ttf.h"
#include "ui/fonte_tela.h"
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
        { KEY_LEFT_CONTROL, J_TAB }, { KEY_RIGHT_CONTROL, J_TAB },  // CTRL faz o que o TAB faz
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
    // Setas e BACKSPACE seguradas repetem; ENTER, TAB e CTRL nao (segurar nao
    // deve jogar tres dados de uma vez).
    for (unsigned i = 0; i < n; i++) {
        int jg = ESPECIAL[i].jogo;
        bool repete = jg == J_UP || jg == J_DOWN || jg == J_LEFT || jg == J_RIGHT || jg == J_BKSP;
        if (repete && IsKeyPressedRepeat(ESPECIAL[i].rl)) tecla(jg);
    }
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

// ---------------------------------------------------------------------------
// Texto nitido: o jogo avisa cada texto do quadro; aqui ele e desenhado com a
// Jersey 10 na resolucao da tela, por cima da mesa de pixels.
// ---------------------------------------------------------------------------
typedef struct { int16_t x, y; uint8_t esc; bool negrito; uint16_t cor; int ini; } texto_t;
static texto_t textos[768];
static int n_textos, usado;
static char letras[32768];
static Font fonte;

static void recebe_texto(int x, int y, const char *s, uint16_t c, int esc, bool negrito)
{
    int n = (int)strlen(s) + 1;
    if (n_textos >= (int)(sizeof textos / sizeof textos[0]) || usado + n > (int)sizeof letras) return;
    textos[n_textos++] = (texto_t){ (int16_t)x, (int16_t)y, (uint8_t)esc, negrito, c, usado };
    memcpy(letras + usado, s, (size_t)n);
    usado += n;
}

static Color rgb565(uint16_t c)
{
    return (Color){ (unsigned char)(((c >> 11) & 31) * 255 / 31),
                    (unsigned char)(((c >> 5) & 63) * 255 / 63),
                    (unsigned char)((c & 31) * 255 / 31), 255 };
}

static void carrega_fonte(void)
{
    int cps[FONTE_N], n = 0;
    for (int c = 0x20; c < 0x100; c++) if (c < 0x7F || c >= 0xA0) cps[n++] = c;
    fonte = LoadFontFromMemory(".ttf", FONTE_TTF, (int)sizeof FONTE_TTF, 96, cps, n);
    GenTextureMipmaps(&fonte.texture);
    SetTextureFilter(fonte.texture, TEXTURE_FILTER_TRILINEAR);
    gfx_texto_nitido(recebe_texto);
}

// Cada letra vai na posicao que o jogo calculou (mesmos avancos da tabela),
// assim o centro e as quebras de linha batem com o que o jogo mediu.
static void desenha_textos(Rectangle area)
{
    float k = area.width / GFX_W;
    for (int i = 0; i < n_textos; i++) {
        const texto_t *t = &textos[i];
        float e = t->esc == GFX_MIUDO ? GFX_MIUDO_ESC : (float)t->esc;
        float tam = FT_EM * e * k;
        float x = area.x + t->x * k, y = area.y + t->y * k;
        Color c = rgb565(t->cor);
        const char *s = letras + t->ini;
        while (*s) {
            int cp = gfx_proximo_car(&s);
            DrawTextCodepoint(fonte, cp, (Vector2){ x, y }, tam, c);
            if (t->negrito) DrawTextCodepoint(fonte, cp, (Vector2){ x + k * 0.45f * e, y }, tam, c);
            x += FT_AVANCO[cp - FONTE_PRIM] * e * k / 64.0f;
        }
    }
}

// Cliques: o raylib so ve o botao apertado no comeco de cada quadro, e um
// toque de touchpad aperta e solta no mesmo quadro. Por isso cada clique e
// anotado na hora, pela GLFW (que o raylib usa no PC e no navegador), e
// repassado ao raylib em seguida.
typedef struct GLFWwindow GLFWwindow;
typedef void (*GLFWmousebuttonfun)(GLFWwindow *, int, int, int);
GLFWwindow *glfwGetCurrentContext(void);
GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow *, GLFWmousebuttonfun);

static GLFWmousebuttonfun botao_raylib;
static uint8_t cliques[16];
static int n_cliques;

static void anota_clique(GLFWwindow *w, int botao, int acao, int mods)
{
    if (acao == 1 && (botao == 0 || botao == 1) && n_cliques < (int)sizeof cliques)
        cliques[n_cliques++] = (uint8_t)(botao + 1);          // 1 esquerdo, 2 direito
    if (botao_raylib) botao_raylib(w, botao, acao, mods);
}

// Mouse: a posicao na janela vira posicao na tela do jogo (640x360).
static void le_mouse(void)
{
    static int ult_x = -1, ult_y = -1;
    int n = n_cliques;
    n_cliques = 0;
    Rectangle a = area_do_jogo();
    Vector2 m = GetMousePosition();
    if (m.x < a.x || m.y < a.y || m.x >= a.x + a.width || m.y >= a.y + a.height) return;
    int x = (int)((m.x - a.x) * GFX_W / a.width), y = (int)((m.y - a.y) * GFX_H / a.height);
    if (x != ult_x || y != ult_y) { ult_x = x; ult_y = y; dado_mouse(x, y, 0); }
    for (int i = 0; i < n; i++) dado_mouse(x, y, cliques[i]);
}

// Um quadro do jogo: teclado, mouse, passo, desenho e a textura ampliada na janela.
static void quadro(void)
{
    le_teclado();
    le_mouse();
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;          // janela arrastada: nada de salto
    dado_passo(dt);
    som_atualiza(dt);
    n_textos = usado = 0;
    dado_desenha();
    UpdateTexture(tela, fb);

    BeginDrawing();
    ClearBackground(C_FAIXA);
    Rectangle area = area_do_jogo();
    DrawTexturePro(tela, (Rectangle){ 0, 0, GFX_W, GFX_H }, area, (Vector2){ 0, 0 }, 0, WHITE);
    desenha_textos(area);
    EndDrawing();
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(GFX_W * 2, GFX_H * 2, TITULO);
    SetWindowMinSize(GFX_W, GFX_H);
    SetExitKey(KEY_NULL);                  // ESC e do jogo, nao fecha a janela
    botao_raylib = glfwSetMouseButtonCallback(glfwGetCurrentContext(), anota_clique);
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
    carrega_fonte();

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
