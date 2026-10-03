// DADO EM CASA para PC: o mesmo jogo do Cardputer, numa janela.
//
// O jogo desenha num framebuffer RGB565 de 640x360. Aqui esse quadro vira uma
// textura, ampliada para encher a janela (sem distorcer): cada pixel do jogo
// vira um bloco nitido, so com a borda suavizada quando a escala nao e
// inteira. O que sobra da janela fica na cor das faixas da mesa.
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
#include "rlgl.h"
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
#include <emscripten/html5.h>
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

#ifdef __EMSCRIPTEN__
// No navegador a tela cheia e da pagina inteira (o canvas ocupa 100% dela).
// O ToggleFullscreen do raylib marca a janela como cheia mesmo quando o
// navegador recusa, e dai em diante para de acompanhar o tamanho do canvas:
// o desenho e o mouse se desencontram.
EM_JS(void, web_tela_cheia, (void), {
    try {
        if (document.fullscreenElement) document.exitFullscreen();
        else document.documentElement.requestFullscreen();
    } catch (e) {}
});
#endif

static void alterna_tela_cheia(void)
{
#ifdef __EMSCRIPTEN__
    web_tela_cheia();
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
#ifdef __EMSCRIPTEN__
        // A pagina segura o espaco (para nao rolar) e o caractere nao chega:
        // vem pela tecla.
        { KEY_SPACE, ' ' },
#endif
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

// Enche a janela mantendo a proporcao do jogo.
static Rectangle area_do_jogo(void)
{
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
#ifdef __EMSCRIPTEN__
    // O tamanho de verdade do canvas, que e onde se desenha.
    int cw = 0, ch = 0;
    emscripten_get_canvas_element_size("#canvas", &cw, &ch);
    if (cw > 0 && ch > 0) { W = (float)cw; H = (float)ch; }
#endif
    float k = W / GFX_W < H / GFX_H ? W / GFX_W : H / GFX_H;
    float w = (float)(int)(GFX_W * k), h = (float)(int)(GFX_H * k);
    return (Rectangle){ (float)(int)((W - w) / 2), (float)(int)((H - h) / 2), w, h };
}

static Texture2D tela;

// Ampliacao "sharp bilinear": dentro de cada pixel do jogo a cor e chapada;
// so na faixa de um pixel da tela entre dois vizinhos ela mistura. Em escala
// inteira da o mesmo que ampliar sem filtro; nas outras, nenhum pixel sai
// mais largo que o vizinho.
#if defined(__EMSCRIPTEN__)
#define GLSL_CAB "#version 100\nprecision mediump float;\nvarying vec2 fragTexCoord;\nvarying vec4 fragColor;\n" \
                 "#define SAI gl_FragColor\n#define TEX texture2D\n"
#else
#define GLSL_CAB "#version 330\nin vec2 fragTexCoord;\nin vec4 fragColor;\nout vec4 saida;\n" \
                 "#define SAI saida\n#define TEX texture\n"
#endif
static const char *const AMPLIA_FS = GLSL_CAB
    "uniform sampler2D texture0;\n"
    "uniform vec2 tam;\n"
    "uniform float esc;\n"
    "void main() {\n"
    "    vec2 t = fragTexCoord * tam;\n"
    "    vec2 base = floor(t);\n"
    "    vec2 d = fract(t) - 0.5;\n"
    "    float r = 0.5 - 0.5 / esc;\n"
    "    vec2 f = (d - clamp(d, -r, r)) * esc + 0.5;\n"
    "    SAI = TEX(texture0, (base + f) / tam) * fragColor;\n"
    "}\n";
static Shader amplia;
static int amplia_esc = -1;
static bool amplia_ok;

static void prepara_ampliacao(void)
{
    amplia = LoadShaderFromMemory(NULL, AMPLIA_FS);
    amplia_ok = amplia.id != 0 && amplia.id != rlGetShaderIdDefault();
    if (amplia_ok) {
        float tam[2] = { (float)GFX_W, (float)GFX_H };
        SetShaderValue(amplia, GetShaderLocation(amplia, "tam"), tam, SHADER_UNIFORM_VEC2);
        amplia_esc = GetShaderLocation(amplia, "esc");
        SetTextureFilter(tela, TEXTURE_FILTER_BILINEAR);
    } else {
        SetTextureFilter(tela, TEXTURE_FILTER_POINT);   // sem shader: amplia sem filtro
    }
    SetTextureWrap(tela, TEXTURE_WRAP_CLAMP);
}

#ifdef __EMSCRIPTEN__
// Telas densas (celular, Mac, Windows com zoom): o canvas ganha um pixel por
// pixel do aparelho, em vez de ser esticado pelo navegador.
EM_JS(int, web_tam_real, (int *w, int *h), {
    var c = Module.canvas, k = window.devicePixelRatio || 1;
    var r = c.getBoundingClientRect();
    HEAP32[w >> 2] = Math.max(1, Math.round(r.width * k));
    HEAP32[h >> 2] = Math.max(1, Math.round(r.height * k));
    return 1;
});

static void web_ajusta_canvas(void)
{
    int w = 0, h = 0, cw = 0, ch = 0;
    web_tam_real(&w, &h);
    emscripten_get_canvas_element_size("#canvas", &cw, &ch);
    if (w != cw || h != ch) emscripten_set_canvas_element_size("#canvas", w, h);
    // O raylib acha que o canvas tem o tamanho da pagina: o desenho segue o de verdade.
    rlViewport(0, 0, w, h);
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, w, h, 0, 0, 1);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
}
#endif

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
#ifndef __EMSCRIPTEN__
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
#endif

#ifdef __EMSCRIPTEN__
// Celular e tablet: sem teclado fisico, o jogo pede texto (nome do perfil,
// codigo da sala) e um toque na tela poe o foco num campo invisivel, o que
// abre o teclado do aparelho. O que se digita ali vem para o jogo por aqui.
EMSCRIPTEN_KEEPALIVE void web_tecla(int k) { tecla(k); }
EMSCRIPTEN_KEEPALIVE int web_digitando(void) { return dado_digitando(); }
EM_JS(void, web_teclado_inicia, (int enter, int apaga), {
    var e = document.createElement('input');
    e.type = 'text';
    e.setAttribute('autocomplete', 'off');
    e.setAttribute('autocorrect', 'off');
    e.setAttribute('autocapitalize', 'characters');
    e.setAttribute('spellcheck', 'false');
    e.setAttribute('aria-hidden', 'true');
    e.style.cssText = 'position:fixed;left:0;bottom:0;width:1px;height:1px;opacity:0;' +
                      'font-size:16px;border:0;padding:0;';
    document.body.appendChild(e);
    Module.dadoCampo = e;
    // As teclas do campo nao chegam ao raylib (senao cada letra viria duas
    // vezes): o texto vem pelo evento 'input'; ENTER e apagar, daqui.
    var deixa = ['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown', 'Escape', 'Tab'];
    e.addEventListener('keydown', function (ev) {
        if (ev.key === 'Enter') { Module._web_tecla(enter); ev.preventDefault(); }
        else if (ev.key === 'Backspace' && !e.value) { Module._web_tecla(apaga); ev.preventDefault(); }
        if (deixa.indexOf(ev.key) < 0) ev.stopPropagation();
    });
    e.addEventListener('keypress', function (ev) { ev.stopPropagation(); });
    e.addEventListener('keyup', function (ev) { if (deixa.indexOf(ev.key) < 0) ev.stopPropagation(); });
    e.addEventListener('input', function (ev) {
        if (ev.inputType === 'deleteContentBackward') Module._web_tecla(apaga);
        else for (var i = 0; i < e.value.length; i++) {
            var c = e.value.charCodeAt(i);
            if (c >= 32 && c < 127) Module._web_tecla(c);
        }
        e.value = "";
    });
    Module.canvas.addEventListener('pointerup', function (ev) {
        if (ev.pointerType !== 'mouse' && Module._web_digitando()) e.focus();
    });
});
// Fora das telas de digitar, o campo solta o foco (e o teclado do aparelho fecha).
EM_JS(void, web_teclado_confere, (int digitando), {
    var e = Module.dadoCampo;
    if (e && !digitando && document.activeElement === e) { e.blur(); Module.canvas.focus(); }
});

// No navegador o mouse (e o toque) e lido direto da pagina, em pixels do
// canvas: nao depende de como o raylib acompanha o tamanho da janela.
EM_JS(void, web_mouse_inicia, (void), {
    var c = Module.canvas;
    Module.dadoMouse = { x: -1, y: -1, cliques: [] };
    function pos(e) {
        var r = c.getBoundingClientRect();
        if (r.width <= 0 || r.height <= 0) return;
        Module.dadoMouse.x = (e.clientX - r.left) * c.width / r.width;
        Module.dadoMouse.y = (e.clientY - r.top) * c.height / r.height;
    }
    c.addEventListener('pointermove', pos);
    c.addEventListener('pointerdown', function (e) {
        pos(e);
        if (e.button === 0 || e.button === 2)
            Module.dadoMouse.cliques.push([e.button === 0 ? 1 : 2, Module.dadoMouse.x, Module.dadoMouse.y]);
    });
});
EM_JS(float, web_mouse_x, (void), { return Module.dadoMouse ? Module.dadoMouse.x : -1; });
EM_JS(float, web_mouse_y, (void), { return Module.dadoMouse ? Module.dadoMouse.y : -1; });
// Tira o proximo clique da fila: devolve o botao (0 se nao ha) e a posicao.
EM_JS(int, web_clique, (float *x, float *y), {
    var q = Module.dadoMouse ? Module.dadoMouse.cliques : [];
    if (!q.length) return 0;
    var k = q.shift();
    HEAPF32[x >> 2] = k[1];
    HEAPF32[y >> 2] = k[2];
    return k[0];
});
#endif

// Posicao da janela (ou do canvas) em posicao da tela do jogo (640x360);
// false se caiu fora da mesa.
static bool no_jogo(float mx, float my, int *x, int *y)
{
    Rectangle a = area_do_jogo();
    if (mx < a.x || my < a.y || mx >= a.x + a.width || my >= a.y + a.height) return false;
    *x = (int)((mx - a.x) * GFX_W / a.width);
    *y = (int)((my - a.y) * GFX_H / a.height);
    return true;
}

// Mouse: a posicao na janela vira posicao na tela do jogo (640x360).
static void le_mouse(void)
{
    static int ult_x = -1, ult_y = -1;
#ifdef __EMSCRIPTEN__
    int x, y;
    if (no_jogo(web_mouse_x(), web_mouse_y(), &x, &y) && (x != ult_x || y != ult_y)) {
        ult_x = x; ult_y = y;
        dado_mouse(x, y, 0);
    }
    float cx, cy;
    for (int b; (b = web_clique(&cx, &cy)) != 0; )
        if (no_jogo(cx, cy, &x, &y)) {
            if (x != ult_x || y != ult_y) { ult_x = x; ult_y = y; dado_mouse(x, y, 0); }
            dado_mouse(x, y, b);
        }
#else
    int n = n_cliques, x, y;
    n_cliques = 0;
    Vector2 m = GetMousePosition();
    if (!no_jogo(m.x, m.y, &x, &y)) return;
    if (x != ult_x || y != ult_y) { ult_x = x; ult_y = y; dado_mouse(x, y, 0); }
    for (int i = 0; i < n; i++) dado_mouse(x, y, cliques[i]);
#endif
}

// Maozinha sobre o que se pode clicar (checado depois do desenho, que e
// quando o jogo marca as areas clicaveis).
static void poe_cursor(void)
{
    static int atual = -1;
    int c = dado_mouse_clicavel() ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT;
    if (c != atual) { SetMouseCursor(c); atual = c; }
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
    poe_cursor();
    UpdateTexture(tela, fb);

#ifdef __EMSCRIPTEN__
    web_teclado_confere(dado_digitando());
    web_ajusta_canvas();
#endif
    BeginDrawing();
#ifdef __EMSCRIPTEN__
    web_ajusta_canvas();                   // o BeginDrawing refaz a matriz do raylib
#endif
    ClearBackground(C_FAIXA);
    Rectangle area = area_do_jogo();
    if (amplia_ok) {
        float esc = area.width / GFX_W;
        SetShaderValue(amplia, amplia_esc, &esc, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(amplia);
    }
    DrawTexturePro(tela, (Rectangle){ 0, 0, GFX_W, GFX_H }, area, (Vector2){ 0, 0 }, 0, WHITE);
    if (amplia_ok) EndShaderMode();
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
#ifdef __EMSCRIPTEN__
    web_mouse_inicia();
    web_teclado_inicia(J_ENTER, J_BKSP);
#else
    botao_raylib = glfwSetMouseButtonCallback(glfwGetCurrentContext(), anota_clique);
#endif
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
    prepara_ampliacao();
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
