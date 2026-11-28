// DADO EM CASA para PC: o mesmo jogo do Cardputer, numa janela.
//
// O jogo desenha num framebuffer RGB565 de 640x360. Aqui esse quadro vira uma
// textura, ampliada para encher a janela (sem distorcer): cada pixel do jogo
// vira um bloco nitido, so com a borda suavizada quando a escala nao e
// inteira. O que sobra da janela fica na cor das faixas da mesa.
//
// Teclas do PC alem das do jogo: F11 ou Alt+Enter alternam tela cheia,
// - e + mudam o tamanho da mesa (as duas coisas tambem ficam no painel de
// opcoes do jogo, para o controle e o toque: dado_janela), N liga e desliga
// a musica, F12 salva uma foto da tela.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
#include "hal/som.h"
#include "games/jogos.h"
#include "esp_timer.h"
#include "plataforma.h"
#include "icone.h"
#include "hal/salva.h"
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
    // (sem gesto do usuario - um botao do controle, em alguns navegadores - a
    // recusa vem como promessa rejeitada: so a ignora; a linha das opcoes
    // mostra o estado de verdade)
    try {
        var p = document.fullscreenElement ? document.exitFullscreen() : document.documentElement.requestFullscreen();
        if (p && p.catch) p.catch(function () {});
    } catch (e) {}
});
EM_JS(int, web_tela_e_cheia, (void), { return document.fullscreenElement ? 1 : 0; });
// O navegador sabe por a pagina em tela cheia? (o do iPhone, nao)
EM_JS(int, web_tela_cheia_pode, (void), { return document.documentElement.requestFullscreen ? 1 : 0; });
#endif

static void alterna_tela_cheia(void)
{
#ifdef __EMSCRIPTEN__
    web_tela_cheia();
#else
    ToggleBorderlessWindowed();
#endif
}

static bool tela_e_cheia(void)
{
#ifdef __EMSCRIPTEN__
    return web_tela_e_cheia();
#else
    return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
#endif
}

// Tamanho da mesa, em % do maior que cabe: num monitor grande em tela
// cheia, a mesa inteira pode ficar grande demais. Fica guardado.
#define ZOOM_MIN 50
static int zoom = 100;
static float zoom_aviso;                 // segundos que o aviso ainda fica

static void le_zoom(void)
{
    char b[16];
    if (salva_le("tela", b, sizeof b) > 0) {
        int z = atoi(b);
        if (z >= ZOOM_MIN && z <= 100) zoom = z;
    }
}

static float escala_cheia(void);           // a maior escala que cabe na janela (area_do_jogo)
// O menor tamanho que ainda muda alguma coisa: a mesa nunca fica com menos
// de um pixel da tela por pixel do jogo (numa janela pequena, no celular,
// as vezes nem o 95% cabe).
static int zoom_minimo(void)
{
    float k = escala_cheia();
    int z = ZOOM_MIN;
    while (z < 100 && k * z / 100 < 1) z += 5;
    return z;
}
static void muda_zoom(int d)
{
    int z = zoom + d, zmin = zoom_minimo();
    if (z < zmin) z = d < 0 && zoom < zmin ? zoom : zmin;   // (o guardado abaixo do que cabe fica, mas nao desce mais)
    if (z > 100) z = 100;
    if (z == zoom) return;
    zoom = z;
    char b[16];
    snprintf(b, sizeof b, "%d", zoom);
    salva_grava("tela", b);
}

// Pelas teclas - e +: com o aviso do tamanho por um instante. (Pelo painel
// de opcoes nao precisa: a linha ja mostra o numero.)
static void muda_zoom_tecla(int d)
{
    muda_zoom(d);
    zoom_aviso = 1.6f;
}
// O tamanho de verdade: o guardado, ou 100% quando ele nao cabe nesta janela.
static int zoom_agora(void) { return escala_cheia() * zoom / 100 >= 1 ? zoom : 100; }

static bool campo_focado;               // navegador: o teclado do aparelho, num campo escondido
static float dt_quadro;                 // quanto tempo passou desde o quadro anterior (segundos)

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
        dado_controle(CONTROLE_NENHUM);           // as dicas voltam a mostrar as teclas
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
        for (unsigned i = 0; i < n; i++)
            if (ESPECIAL[i].rl == k) {
                int jg = ESPECIAL[i].jogo;
                // Com o campo em foco, ENTER, apagar e o espaco vem dele (web_tecla).
                if (campo_focado && (jg == J_ENTER || jg == J_BKSP || jg == ' ')) continue;
                tecla(jg);
            }
    }
    // Setas e BACKSPACE seguradas repetem; ENTER, TAB e CTRL nao (segurar nao
    // deve jogar tres dados de uma vez).
    for (unsigned i = 0; i < n; i++) {
        int jg = ESPECIAL[i].jogo;
        bool repete = jg == J_UP || jg == J_DOWN || jg == J_LEFT || jg == J_RIGHT || jg == J_BKSP;
        if (repete && IsKeyPressedRepeat(ESPECIAL[i].rl) && !(campo_focado && jg == J_BKSP)) tecla(jg);
    }
    // Letras, numeros e pontuacao chegam ja traduzidos pelo layout do
    // teclado (ABNT2, US...). O jogo so conhece ASCII.
    for (int c = GetCharPressed(); c > 0; c = GetCharPressed()) {
        if (campo_focado) continue;           // as letras vem do campo (web_tecla)
        if ((c == '-' || c == '_') && !dado_digitando()) { muda_zoom_tecla(-5); continue; }
        if ((c == '+' || c == '=') && !dado_digitando()) { muda_zoom_tecla(5); continue; }
        if (c >= 0x20 && c < 0x7F) tecla(c);
    }
}

// Controles (Xbox, PlayStation e os parecidos), ate dois: os botoes viram as
// teclas do jogo. O A (a cruz) confirma, o B (a bola) volta ou desfaz, o Y
// (o triangulo) segue, o X (o quadrado) abre os detalhes; START (OPTIONS)
// e o ESC, VIEW (SHARE) abre a bolsa, LB/L1 recusa, RB/R1 muda a
// velocidade, LT/L2 mostra o que faz cada botao e RT/R2 abre as opcoes. O direcional e o analogico esquerdo andam, repetindo quando
// segurados. No modo 2 jogadores, com os dois ligados, cada um joga na sua
// vez (o primeiro e o JOGADOR 1), e as dicas da tela mostram os botoes do
// controle de quem joga.
#define CONTROLES 2
static void le_controles(void)
{
    static const struct { int bt, jogo; bool so_da_vez; } BOTAO[] = {
        { GAMEPAD_BUTTON_RIGHT_FACE_DOWN, J_ENTER, true },
        { GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, J_BKSP, true },
        { GAMEPAD_BUTTON_RIGHT_FACE_UP, J_TAB, true },
        { GAMEPAD_BUTTON_RIGHT_FACE_LEFT, 'i', false },
        { GAMEPAD_BUTTON_MIDDLE_RIGHT, J_ESC, false },
        { GAMEPAD_BUTTON_MIDDLE_LEFT, 'b', false },
        { GAMEPAD_BUTTON_LEFT_TRIGGER_1, 'x', true },
        { GAMEPAD_BUTTON_RIGHT_TRIGGER_1, 'v', false },
        { GAMEPAD_BUTTON_LEFT_TRIGGER_2, 'k', false },     // LT/L2: o quadro dos controles
        { GAMEPAD_BUTTON_RIGHT_TRIGGER_2, 'm', false },    // RT/R2: o painel de opcoes
    };
    static const struct { int bt, jogo; } DIRECAO[] = {
        { GAMEPAD_BUTTON_LEFT_FACE_UP, J_UP }, { GAMEPAD_BUTTON_LEFT_FACE_DOWN, J_DOWN },
        { GAMEPAD_BUTTON_LEFT_FACE_LEFT, J_LEFT }, { GAMEPAD_BUTTON_LEFT_FACE_RIGHT, J_RIGHT },
    };
    static float segura[CONTROLES][4];        // ha quanto tempo cada direcao esta apertada
    static float proxima[CONTROLES][4];       // quando ela repete
    static bool ligado[CONTROLES];            // ja estava ligado no quadro anterior
    int ligados = 0;
    for (int g = 0; g < CONTROLES; g++) ligados += IsGamepadAvailable(g);
    float dt = dt_quadro;
    for (int g = 0; g < CONTROLES; g++) {
        if (!IsGamepadAvailable(g)) { memset(segura[g], 0, sizeof segura[g]); ligado[g] = false; dado_controle_de(g, CONTROLE_NENHUM); continue; }
        int vez = ligados > 1 ? dado_vez_do_controle() : -1;
        bool outro = vez >= 0 && vez != g;    // a vez e do outro controle
        const char *nome = GetGamepadName(g);
        int estilo = CONTROLE_XBOX;
        if (nome && (strstr(nome, "PS") || strstr(nome, "Sony") || strstr(nome, "DualS") || strstr(nome, "PLAYSTATION")
                     || strstr(nome, "054c") || !strcmp(nome, "Wireless Controller")))
            estilo = CONTROLE_PS;
        dado_controle_de(g, estilo);              // (com dois ligados, as dicas mostram os botoes de quem joga agora)
        // Ligou agora (ou ja estava ligado ao abrir o jogo): as dicas mostram
        // os botoes dele desde ja, sem esperar o primeiro botao.
        if (!ligado[g]) { ligado[g] = true; dado_controle(estilo); }
        for (unsigned i = 0; i < sizeof BOTAO / sizeof BOTAO[0]; i++)
            if (IsGamepadButtonPressed(g, BOTAO[i].bt)) {
                dado_controle(estilo);
                bool letra = BOTAO[i].jogo < 0x80 && BOTAO[i].jogo >= 'a';
                if (letra && dado_digitando()) continue;     // escrevendo um nome: o botao nao vira letra
                if (!(outro && BOTAO[i].so_da_vez)) tecla(BOTAO[i].jogo);
            }
        // O direcional ou o analogico: anda ao apertar e, segurado, repete.
        float ax = GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_X), ay = GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_Y);
        bool dir[4] = { ay < -0.55f, ay > 0.55f, ax < -0.55f, ax > 0.55f };
        for (int d = 0; d < 4; d++) {
            dir[d] |= IsGamepadButtonDown(g, DIRECAO[d].bt);
            if (!dir[d]) { segura[g][d] = 0; continue; }
            bool anda = segura[g][d] == 0;
            segura[g][d] += dt;
            if (anda) proxima[g][d] = 0.40f;
            else if (segura[g][d] >= proxima[g][d]) { anda = true; proxima[g][d] += 0.11f; }
            if (anda) {
                dado_controle(estilo);
                if (!outro) tecla(DIRECAO[d].jogo);
            }
        }
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
static float escala_ult = 1;                // a maior escala que cabia no ultimo quadro
static float escala_cheia(void) { return escala_ult; }
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
    escala_ult = k;
    if (k * zoom / 100 >= 1) k = k * zoom / 100;         // nunca menor que 1 pixel por pixel
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
static int amplia_esc = -1, amplia_tam = -1;
static bool amplia_ok;

static void prepara_ampliacao(void)
{
    amplia = LoadShaderFromMemory(NULL, AMPLIA_FS);
    amplia_ok = amplia.id != 0 && amplia.id != rlGetShaderIdDefault();
    if (amplia_ok) {
        float tam[2] = { (float)GFX_W, (float)GFX_H };
        amplia_tam = GetShaderLocation(amplia, "tam");
        SetShaderValue(amplia, amplia_tam, tam, SHADER_UNIFORM_VEC2);
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

// Devolve se o canvas mudou de tamanho (a tela tem de ser pintada de novo).
static bool web_ajusta_canvas(void)
{
    int w = 0, h = 0, cw = 0, ch = 0;
    web_tam_real(&w, &h);
    emscripten_get_canvas_element_size("#canvas", &cw, &ch);
    bool mudou = w != cw || h != ch;
    if (mudou) emscripten_set_canvas_element_size("#canvas", w, h);
    // O raylib acha que o canvas tem o tamanho da pagina: o desenho segue o de verdade.
    rlViewport(0, 0, w, h);
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, w, h, 0, 0, 1);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    return mudou;
}
// A capa do site ("clique para jogar") ainda cobre o jogo?
EM_JS(int, web_coberto, (void), { return Module.dadoAberto ? 0 : 1; });
EM_JS(int, web_memoria_mb, (void), { return HEAP8.length >> 20; });
#endif

// O medidor de quadros (a opcao do painel do som, para conferir o aparelho:
// um tablet, sobretudo). No canto de cima, refeito a cada segundo:
//   - quantos quadros por segundo o jogo fez e, deles, quantos foram para a
//     tela (no navegador, o quadro igual ao anterior nao e reenviado);
//   - o intervalo entre dois quadros, medio e o pior do segundo;
//   - o tempo de trabalho de um quadro (ler, andar, desenhar), medio e pior;
//   - desde que foi ligado, quantos quadros atrasaram (passaram de 25 ms: a
//     60 por segundo, um quadro perdido) e quantos passaram de 50 ms;
//   - a memoria dos sons decodificados, quantos sons tiveram de ser
//     decodificados na hora de tocar e quantos esperam na fila (e, no
//     navegador, a memoria toda do jogo).
static struct {
    bool ligado;
    double t_ult, t_seg;                 // o comeco do quadro anterior e do segundo em conta
    int n, telas;
    double soma, pior, trab_soma, trab_pior;
    int atrasados, longos;
    char linha[3][72];
} med;

// No comeco de cada quadro: o intervalo desde o anterior.
static void medidor_quadro(double agora)
{
    bool on = dado_medidor();
    if (on != med.ligado) { memset(&med, 0, sizeof med); med.ligado = on; med.t_seg = agora; }
    if (!on) return;
    if (med.t_ult > 0) {
        double ms = (agora - med.t_ult) * 1000;
        med.n++;
        med.soma += ms;
        if (ms > med.pior) med.pior = ms;
        med.atrasados += ms > 25;
        med.longos += ms > 50;
    }
    med.t_ult = agora;
    double seg = agora - med.t_seg;
    if (seg < 1 || !med.n) return;
    int kb = 0, na_hora = 0, fila = 0;
    som_medida(&kb, &na_hora, &fila);
    snprintf(med.linha[0], sizeof med.linha[0], "%.0f qps (tela %.0f)  quadro %.1f ms, pior %.1f",
             med.n / seg, med.telas / seg, med.soma / med.n, med.pior);
    snprintf(med.linha[1], sizeof med.linha[1], "trabalho %.1f ms, pior %.1f  atrasados %d (+50 ms: %d)",
             med.trab_soma / med.n, med.trab_pior, med.atrasados, med.longos);
#ifdef __EMSCRIPTEN__
    snprintf(med.linha[2], sizeof med.linha[2], "sons %d MB, na hora %d, fila %d  jogo %d MB", (kb + 512) / 1024, na_hora, fila, web_memoria_mb());
#else
    snprintf(med.linha[2], sizeof med.linha[2], "sons %d MB, na hora %d, fila %d", (kb + 512) / 1024, na_hora, fila);
#endif
    med.t_seg = agora;
    med.n = med.telas = 0;
    med.soma = med.pior = med.trab_soma = med.trab_pior = 0;
}
// No fim: quanto o quadro trabalhou e se foi para a tela.
static void medidor_fim(double t_ini, bool na_tela)
{
    if (!med.ligado) return;
    double ms = (GetTime() - t_ini) * 1000;
    med.trab_soma += ms;
    if (ms > med.trab_pior) med.trab_pior = ms;
    med.telas += na_tela;
}
static void medidor_desenha(void)
{
    if (!med.ligado || !med.linha[0][0]) return;
    int w = 0;
    for (int i = 0; i < 3; i++) { int l = gfx_largura(med.linha[i], 1); if (l > w) w = l; }
    gfx_rect_alfa(2, 2, w + 10, 3 * (FONTE_ALT + 1) + 5, 0x0000, 215);
    for (int i = 0; i < 3; i++) gfx_texto(7, 4 + i * (FONTE_ALT + 1), med.linha[i], i ? 0xD6BA : 0xFFE0, 1, false);
}

// Um bloco de pixels resumido num numero: no navegador, o quadro (ou a
// margem) igual ao que ja esta na tela nao e enviado de novo (quatro contas
// em paralelo, uns 0,03 ms no quadro; a mistura depois de cada palavra faz
// os bits altos contarem tambem).
static uint64_t resumo_de(const void *px, size_t tam)
{
    const uint64_t K = 0x9E3779B97F4A7C15ull;
    uint64_t h[4] = { 1, 2, 3, 4 }, w[4];
    const unsigned char *p = (const unsigned char *)px;
    size_t i = 0;
    for (; i + 32 <= tam; i += 32) {
        memcpy(w, p + i, 32);
        for (int k = 0; k < 4; k++) { h[k] = (h[k] ^ w[k]) * K; h[k] ^= h[k] >> 29; }
    }
    for (; i < tam; i++) { h[0] = (h[0] ^ p[i]) * K; h[0] ^= h[0] >> 29; }   // (a sobra, na margem)
    return h[0] ^ (h[1] << 1 | h[1] >> 63) ^ (h[2] << 2 | h[2] >> 62) ^ (h[3] << 3 | h[3] >> 61);
}
static uint64_t resumo_do_quadro(void) { return resumo_de(fb, sizeof fb); }

// O tamanho da mesa escolhido com - e +, por um instante, desenhado no
// proprio quadro do jogo (no mesmo grid de pixels do resto).
static void aviso_zoom(void)
{
    if (zoom_aviso <= 0) return;
    zoom_aviso -= dt_quadro;
    char b[48];
    snprintf(b, sizeof b, "Mesa %d%%   ( -  e  + )", zoom_agora());
    int w = gfx_largura(b, 1), x = (GFX_W - w) / 2, y = GFX_H / 8;
    gfx_rect_alfa(x - 12, y - 4, w + 24, FONTE_ALT + 6, 0x1082, 230);
    gfx_texto(x, y, b, 0xD58C, 1, false);
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
    // Com o campo em foco, o texto vem so dele: pelo evento 'input' (e ENTER
    // e apagar, pelo keydown dele). O raylib tambem ve as teclas (a GLFW
    // escuta a janela antes do campo), e o le_teclado as ignora enquanto o
    // campo tem o foco - senao, no iPhone e no iPad, cada letra vinha duas.
    var deixa = ['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown', 'Escape', 'Tab'];
    e.addEventListener('keydown', function (ev) {
        if (ev.isComposing) return;
        if (ev.key === 'Enter') { Module._web_tecla(enter); ev.preventDefault(); }
        else if (ev.key === 'Backspace' && !e.value) { Module._web_tecla(apaga); ev.preventDefault(); }
        if (deixa.indexOf(ev.key) < 0) ev.stopPropagation();
    });
    e.addEventListener('keypress', function (ev) { ev.stopPropagation(); });
    e.addEventListener('keyup', function (ev) { if (deixa.indexOf(ev.key) < 0) ev.stopPropagation(); });
    // O que ja foi para o jogo do texto que esta no campo. Fora de uma
    // composicao (o teclado do Android monta a palavra antes de soltar), o
    // campo e esvaziado a cada letra; dentro dela, so manda a diferenca.
    var mandado = '', compondo = false;
    function so_ascii(s) {
        var r = '';
        for (var i = 0; i < s.length; i++) { var c = s.charCodeAt(i); if (c >= 32 && c < 127) r += s.charAt(i); }
        return r;
    }
    function sincroniza() {
        var v = so_ascii(e.value), k = 0;
        while (k < mandado.length && k < v.length && mandado[k] === v[k]) k++;
        for (var i = mandado.length; i > k; i--) Module._web_tecla(apaga);
        for (var i = k; i < v.length; i++) Module._web_tecla(v.charCodeAt(i));
        mandado = v;
    }
    function esvazia() { e.value = ''; mandado = ''; }
    e.addEventListener('compositionstart', function () { compondo = true; });
    e.addEventListener('compositionend', function () {   // o Safari ainda manda um 'input' depois
        compondo = false;
        sincroniza();
        setTimeout(function () { if (!compondo) esvazia(); }, 0);
    });
    e.addEventListener('input', function (ev) {
        sincroniza();
        if (!compondo && !ev.isComposing) esvazia();
    });
    e.addEventListener('blur', function () { compondo = false; esvazia(); });
    Module.canvas.addEventListener('pointerup', function (ev) {
        if (ev.pointerType !== 'mouse' && Module._web_digitando()) e.focus();
    });
});
// O campo esta com o foco (o teclado do aparelho aberto)?
EM_JS(int, web_campo_focado, (void), { return Module.dadoCampo && document.activeElement === Module.dadoCampo ? 1 : 0; });
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
    // Um dedo (ou caneta) na tela e um toque (3): nas acoes do jogo, o
    // primeiro escolhe e o segundo confirma.
    c.addEventListener('pointerdown', function (e) {
        pos(e);
        if (e.pointerType && e.pointerType !== 'mouse') {
            if (e.isPrimary !== false) Module.dadoMouse.cliques.push([3, Module.dadoMouse.x, Module.dadoMouse.y]);
        } else if (e.button === 0 || e.button === 2)
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
    // So o mouse que andou na janela "passa por cima" (acao 0). Quando muda
    // so a escala (o tamanho da mesa, a tela cheia), o mouse parado cai em
    // outro ponto do jogo: o jogo fica sabendo onde (acao -1), sem trocar a
    // escolha do cursor.
    static int ult_x = -1, ult_y = -1;
    static float jan_x = -1, jan_y = -1;
#ifdef __EMSCRIPTEN__
    int x, y;
    float wx = web_mouse_x(), wy = web_mouse_y();
    bool andou = wx != jan_x || wy != jan_y;
    jan_x = wx; jan_y = wy;
    if (no_jogo(wx, wy, &x, &y) && (x != ult_x || y != ult_y)) {
        ult_x = x; ult_y = y;
        dado_mouse(x, y, andou ? 0 : -1);
    }
    float cx, cy;
    for (int b; (b = web_clique(&cx, &cy)) != 0; )
        if (no_jogo(cx, cy, &x, &y)) {
            if (x != ult_x || y != ult_y) { ult_x = x; ult_y = y; dado_mouse(x, y, 0); }
            dado_controle(CONTROLE_NENHUM);
            dado_mouse(x, y, b);
        }
#else
    int n = n_cliques, x, y;
    n_cliques = 0;
    Vector2 m = GetMousePosition();
    bool andou = m.x != jan_x || m.y != jan_y;
    jan_x = m.x; jan_y = m.y;
    if (!no_jogo(m.x, m.y, &x, &y)) return;
    if (x != ult_x || y != ult_y) { ult_x = x; ult_y = y; dado_mouse(x, y, andou ? 0 : -1); }
    for (int i = 0; i < n; i++) { dado_controle(CONTROLE_NENHUM); dado_mouse(x, y, cliques[i]); }
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

// O salao em volta da mesa: com a mesa menor que a janela (o - e o +) ou a
// janela fora de 16:9, sobra margem, e o jogo pinta ali a parede do salao,
// a luz e a fumaca (dado_salao), no mesmo tamanho de pixel da mesa. A
// textura e refeita umas 12 vezes por segundo (a fumaca anda devagar).
static Texture2D salao;
static uint16_t *salao_px;
static int salao_w, salao_h;
static float salao_falta;
#define SALAO_MAX (2000 * 1200)              // pixels do jogo: numa janela enorme, nao passa disso
static bool salao_tem;                       // ha margem (e textura) neste quadro
// Refaz a textura do salao quando e hora. Devolve se o que vai para a tela mudou.
static bool prepara_salao(Rectangle area, float dt)
{
    float k = area.width / GFX_W;
    bool tinha = salao_tem;
    salao_tem = false;
    if (area.x < k && area.y < k) return tinha;    // a mesa enche a janela: nao ha margem
    int ox = (int)ceilf(area.x / k), oy = (int)ceilf(area.y / k);
    int w = 2 * ox + GFX_W + 1, h = 2 * oy + GFX_H + 1;
    if ((long)w * h > SALAO_MAX) return tinha;
    bool mudou = !tinha;
    if (w != salao_w || h != salao_h) {
        uint16_t *n = realloc(salao_px, (size_t)w * h * sizeof(uint16_t));
        if (!n) return tinha;
        salao_px = n;
        if (salao_w) UnloadTexture(salao);
        salao_w = w; salao_h = h;
        Image img = { .data = salao_px, .width = w, .height = h, .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R5G6B5 };
        dado_salao(salao_px, w, h, ox, oy);
        salao = LoadTextureFromImage(img);
        SetTextureFilter(salao, amplia_ok ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
        SetTextureWrap(salao, TEXTURE_WRAP_CLAMP);
        salao_falta = 0;
        mudou = true;
    }
    if ((salao_falta -= dt) <= 0) {
        // (a margem parada nao vai de novo para a placa: a tela parada fica
        // de fato parada, sem repintar 12 vezes por segundo)
        static uint64_t salao_resumo;
        dado_salao(salao_px, w, h, ox, oy);
        uint64_t r = resumo_de(salao_px, (size_t)w * h * sizeof(uint16_t));
        if (r != salao_resumo || mudou) {
            UpdateTexture(salao, salao_px);
            salao_resumo = r;
            mudou = true;
        }
        salao_falta = 1 / 12.0f;
    }
    salao_tem = true;
    return mudou;
}
static void desenha_salao(Rectangle area)
{
    if (!salao_tem) return;
    float k = area.width / GFX_W;
    int ox = (int)ceilf(area.x / k), oy = (int)ceilf(area.y / k);
    int w = salao_w, h = salao_h;
    Rectangle dst = { area.x - ox * k, area.y - oy * k, w * k, h * k };
    if (amplia_ok) {
        float tam[2] = { (float)w, (float)h };
        SetShaderValue(amplia, amplia_tam, tam, SHADER_UNIFORM_VEC2);
        SetShaderValue(amplia, amplia_esc, &k, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(amplia);
    }
    DrawTexturePro(salao, (Rectangle){ 0, 0, (float)w, (float)h }, dst, (Vector2){ 0, 0 }, 0, WHITE);
    if (amplia_ok) {
        EndShaderMode();
        float tam[2] = { (float)GFX_W, (float)GFX_H };
        SetShaderValue(amplia, amplia_tam, tam, SHADER_UNIFORM_VEC2);
    }
}

// Um quadro do jogo: teclado, mouse, passo, desenho e a textura ampliada na janela.
static float periodo_quadro = 1 / 60.0f;    // o intervalo entre quadros (no site, o da tela; no PC, 60 por segundo)
static void quadro(void)
{
    double t_ini = GetTime();
#ifdef __EMSCRIPTEN__
    // O navegador chama a cada repintura da tela. Numa tela de 120 Hz (ou
    // 144) isso seria o dobro do trabalho, da bateria e do calor, sem ganho
    // nenhum para o pixel art: a repintura que chega menos de 9,5 ms depois
    // de um quadro e pulada inteira (120 Hz da 60 quadros por segundo; 144,
    // 72). A de 60 e a de 90 Hz seguem como sao. O tempo do quadro e medido
    // aqui (o do raylib conta de EndDrawing a EndDrawing, e ha quadros sem).
    // Pula so quando a tela e mesmo rapida (o intervalo medio entre as
    // repinturas e curto): numa de 60 Hz, a repintura que vem logo depois de
    // um quadro atrasado nao e pulada.
    static double t_ult = -1, t_raf = -1;
    static float raf_medio = 1 / 60.0f;
    if (t_raf >= 0) {
        float d = (float)(t_ini - t_raf);
        raf_medio += ((d > 0.05f ? 0.05f : d) - raf_medio) * 0.1f;
    }
    t_raf = t_ini;
    if (raf_medio < 0.0095f && t_ult >= 0 && t_ini - t_ult < 0.0095) return;
    periodo_quadro = raf_medio < 0.0095f ? 2 * raf_medio : raf_medio;   // (a de 120 Hz e pulada um sim, um nao)
    dt_quadro = t_ult < 0 ? 1 / 60.0f : (float)(t_ini - t_ult);
    t_ult = t_ini;
    campo_focado = web_campo_focado();
#else
    dt_quadro = GetFrameTime();
#endif
    medidor_quadro(t_ini);
    le_teclado();
    le_mouse();
    le_controles();
    float dt = dt_quadro;
    if (dt > 0.05f) dt = 0.05f;          // janela arrastada: nada de salto
    // A maquina pensa aos pedacos, no que sobra do quadro depois do desenho
    // (entre 2 e 8 ms): a tela nao para enquanto ela decide.
    static double ult_desenho = 0.002;
    double orc = 0.012 - ult_desenho;
    dado_ia_orcamento((int)(1e6 * (orc < 0.002 ? 0.002 : orc > 0.008 ? 0.008 : orc)));
    dado_passo(dt);
    {   // Os sons que a cena pode pedir: a carga os traz aos poucos (som.c).
        uint8_t qs[SOM_N], qr[SOM_ROLAS];
        int l = dado_sons_precisos(qs, qr);
        som_precisa(qs, qr, l & SONS_LACO_MOEDA, l & SONS_LACO_GIRO, l & SONS_LACO_MOTOR);
        // A carga dos sons usa so a folga do quadro: uns 78% do intervalo
        // entre quadros (13 ms a 60 Hz, 8,7 ms a 90) menos o que o passo ja
        // gastou e o que o desenho deve gastar.
        som_carga_orcamento(periodo_quadro * 0.78 - (GetTime() - t_ini) - ult_desenho);
    }
#ifdef __EMSCRIPTEN__
    som_folga(web_coberto());
#endif
    som_atualiza(dt);
    double t_des = GetTime();
    dado_desenha();
    ult_desenho = GetTime() - t_des;
    aviso_zoom();
    medidor_desenha();
    poe_cursor();

#ifdef __EMSCRIPTEN__
    web_teclado_confere(dado_digitando());
    bool tam_mudou = web_ajusta_canvas();
#endif
    Rectangle area = area_do_jogo();
    bool salao_mudou = prepara_salao(area, dt);
#ifdef __EMSCRIPTEN__
    // A tela parada (o menu, a loja, a mesa esperando a jogada): se o quadro
    // saiu igual ao que ja esta na tela, nao e enviado nem pintado de novo -
    // o navegador segue mostrando o anterior. O jogo andou e desenhou como
    // sempre (animacoes, relogios e a IA nao mudam); so a copia para a placa
    // de video e a pintura da pagina e que sao poupadas. Por garantia, pinta
    // ao menos duas vezes por segundo.
    static uint64_t resumo_na_tela;
    static double t_na_tela = -1;
    static Rectangle area_na_tela;
    uint64_t resumo = resumo_do_quadro();
    bool area_mudou = memcmp(&area, &area_na_tela, sizeof area) != 0;      // o tamanho da mesa (- e +)
    if (!tam_mudou && !salao_mudou && !area_mudou && resumo == resumo_na_tela && t_na_tela >= 0 && t_ini - t_na_tela < 0.5) {
        PollInputEvents();                 // o que o EndDrawing faria: as teclas e os controles deste quadro
        medidor_fim(t_ini, false);
        return;
    }
    resumo_na_tela = resumo;
    t_na_tela = t_ini;
    area_na_tela = area;
#else
    (void)salao_mudou;
#endif
    UpdateTexture(tela, fb);
    BeginDrawing();
#ifdef __EMSCRIPTEN__
    web_ajusta_canvas();                   // o BeginDrawing refaz a matriz do raylib
#endif
    ClearBackground(C_FAIXA);
    desenha_salao(area);
    if (amplia_ok) {
        float esc = area.width / GFX_W;
        SetShaderValue(amplia, amplia_esc, &esc, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(amplia);
    }
    DrawTexturePro(tela, (Rectangle){ 0, 0, GFX_W, GFX_H }, area, (Vector2){ 0, 0 }, 0, WHITE);
    if (amplia_ok) EndShaderMode();
    medidor_fim(t_ini, true);
    EndDrawing();
}

#ifndef __EMSCRIPTEN__
// O registro das partidas (a versao de teste, ligada no painel do som): a
// pasta "registros" fica ao lado do jogo, fora dos salvos, e continua la
// quando o .exe e trocado por uma versao nova. No Mac, ao lado do .app (nao
// dentro dele, que se troca inteiro).
static void registro_pasta(void)
{
    char p[1024];
    snprintf(p, sizeof p, "%s", GetApplicationDirectory());
    char *app = strstr(p, ".app/Contents/MacOS");
    if (app) {
        *app = 0;
        char *barra = strrchr(p, '/');
        if (barra) barra[1] = 0;
    }
    dado_registro_pasta(p);
}
#endif

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

    le_zoom();
    // A tela cheia e o tamanho da mesa, tambem no painel de opcoes do jogo.
    dado_janela_t janela = { tela_e_cheia, alterna_tela_cheia, zoom_agora, muda_zoom };
#ifdef __EMSCRIPTEN__
    if (!web_tela_cheia_pode()) janela.tela_cheia = NULL;
#endif
    dado_janela(&janela);
#ifndef __EMSCRIPTEN__
    registro_pasta();                      // a pasta "registros" fica ao lado do jogo
#endif
    dado_inicia(0);
#ifdef __EMSCRIPTEN__
    // No navegador quem manda no ritmo e a pagina: um quadro por repintura
    // (no maximo uns 60 por segundo: ver quadro).
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
