// Dado em Casa no console: o adaptador entre o jogo (src/games/dado.c, o
// mesmo do PC) e o menu, mais a arte que o menu mostra (capa, icone e nome).
#include <math.h>
#include <string.h>
#include "catalogo.h"
#include "texto.h"
#include "sistema.h"
#include "games/jogos.h"
#include "hal/display.h"
#include "hal/keyboard.h"
#include "hal/salva.h"
#include "hal/som.h"
#include "ui/gfx.h"

#define PI_F 3.14159265f

#define D_FELTRO     COR(18, 51, 38)
#define D_FELTRO_CLR COR(40, 104, 74)
#define D_SOMBRA     COR(6, 22, 16)
#define D_MARFIM     COR(238, 231, 214)
#define D_MARFIM_L   COR(196, 184, 160)
#define D_EBANO      COR(34, 30, 29)
#define D_EBANO_L    COR(14, 12, 12)
#define D_VINHO      COR(150, 64, 82)
#define D_VINHO_L    COR(96, 36, 50)
#define D_LATAO      COR(190, 156, 86)
#define D_OURO       COR(226, 192, 104)

// ---------------------------------------------------------------------------
// Arte
// ---------------------------------------------------------------------------

// Feltro: claro no meio, escuro nas bordas, com a trama do pano.
static void feltro(tela_t *t, float cx, float cy, float raio)
{
    uint32_t s = 12345;
    for (int y = 0; y < t->h; y++) {
        uint16_t *lin = t->px + y * t->w;
        float dy = (y - cy) / raio;
        for (int x = 0; x < t->w; x++) {
            float dx = (x - cx) / (raio * 1.25f), d = sqrtf(dx * dx + dy * dy);
            float q = d >= 1 ? 1 : powf(d, 1.4f);
            s = s * 1103515245u + 12345u;
            float ruido = (float)((int)((s >> 16) & 15) - 8) * 0.6f;
            lin[x] = d_cor_pontilhada((int)(40 + (6 - 40) * q + ruido), (int)(104 + (22 - 104) * q + ruido * 1.6f),
                                      (int)(74 + (16 - 74) * q + ruido), x, y);
        }
    }
}

// Quadrado de cantos arredondados, girado, como poligono.
static int quadrado_girado(float *p, float cx, float cy, float lado, float ang, float raio)
{
    int n = 0;
    float h = lado / 2 - raio, ca = cosf(ang), sa = sinf(ang);
    static const float CANTO[4][2] = { { 1, 1 }, { -1, 1 }, { -1, -1 }, { 1, -1 } };
    for (int k = 0; k < 4; k++) {
        float a0 = k * PI_F / 2;
        for (int i = 0; i <= 4; i++) {
            float a = a0 + i * (PI_F / 2) / 4;
            float lx = CANTO[k][0] * h + raio * cosf(a), ly = CANTO[k][1] * h + raio * sinf(a);
            p[n++] = cx + lx * ca - ly * sa;
            p[n++] = cy + lx * sa + ly * ca;
        }
    }
    return n / 2;
}

static void dado(float cx, float cy, float lado, float ang, int valor, cor_t corpo, cor_t lado_c, cor_t pinta)
{
    float p[80];
    int n = quadrado_girado(p, cx + lado * 0.10f, cy + lado * 0.16f, lado * 1.04f, ang, lado * 0.2f);
    d_poligono(p, n, 0, 90);                                   // sombra no feltro
    n = quadrado_girado(p, cx + lado * 0.035f, cy + lado * 0.06f, lado, ang, lado * 0.2f);
    d_poligono(p, n, lado_c, 255);                             // a quina de baixo
    n = quadrado_girado(p, cx, cy, lado, ang, lado * 0.2f);
    d_poligono(p, n, corpo, 255);
    n = quadrado_girado(p, cx - lado * 0.04f, cy - lado * 0.05f, lado * 0.8f, ang, lado * 0.16f);
    d_poligono(p, n, d_clareia(corpo, 8), 120);                // brilho da face
    static const signed char PINTAS[7][6][2] = {
        { { 0 } }, { { 0, 0 } }, { { -1, -1 }, { 1, 1 } }, { { -1, -1 }, { 0, 0 }, { 1, 1 } },
        { { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }, { { -1, -1 }, { 1, -1 }, { 0, 0 }, { -1, 1 }, { 1, 1 } },
        { { -1, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 1, 1 } },
    };
    float ca = cosf(ang), sa = sinf(ang), e = lado * 0.27f;
    for (int i = 0; i < valor; i++) {
        float lx = PINTAS[valor][i][0] * e, ly = PINTAS[valor][i][1] * e;
        float px = cx + lx * ca - ly * sa, py = cy + lx * sa + ly * ca;
        d_circulo(px, py + lado * 0.012f, lado * 0.085f, d_clareia(corpo, -30), 255);
        d_circulo(px, py, lado * 0.08f, pinta, 255);
    }
}

// Ficha vista de cima: aro com marcas e miolo.
static void ficha(float cx, float cy, float r, cor_t c)
{
    d_circulo(cx + r * 0.08f, cy + r * 0.14f, r * 1.02f, 0, 90);
    d_circulo(cx, cy + r * 0.07f, r, d_clareia(c, -35), 255);
    d_circulo(cx, cy, r, c, 255);
    for (int k = 0; k < 8; k++) {
        float a = k * PI_F / 4 + 0.2f, ca = cosf(a), sa = sinf(a), w = r * 0.12f;
        float p[8] = {
            cx + ca * r * 0.72f - sa * w, cy + sa * r * 0.72f + ca * w,
            cx + ca * r * 0.98f - sa * w, cy + sa * r * 0.98f + ca * w,
            cx + ca * r * 0.98f + sa * w, cy + sa * r * 0.98f - ca * w,
            cx + ca * r * 0.72f + sa * w, cy + sa * r * 0.72f - ca * w,
        };
        d_poligono(p, 4, D_MARFIM, 235);
    }
    d_anel(cx, cy, r * 0.62f, r * 0.05f, d_clareia(c, 30), 255);
    d_circulo(cx, cy, r * 0.5f, d_clareia(c, -10), 255);
}

static void capa(tela_t *t)
{
    feltro(t, 900, 420, 620);
    // A linha da mesa, em latao.
    for (int i = 0; i < 3; i++)
        d_anel(980, 1060, 760.0f - i * 3, 2, D_LATAO, i == 1 ? 90 : 40);
    texto(FONTE_JOGO, 34, 760, 316, "APOSTE  ·  ARRISQUE  ·  LEVE O POTE", D_LATAO, 70);

    ficha(1150, 690, 52, D_VINHO);
    ficha(1060, 728, 48, COR(64, 104, 178));
    ficha(1210, 600, 44, D_LATAO);
    dado(1150, 210, 104, -0.15f, 2, D_LATAO, COR(120, 94, 46), D_EBANO);
    dado(905, 470, 168, 0.32f, 5, D_MARFIM, D_MARFIM_L, D_EBANO);
    dado(1105, 470, 126, -0.42f, 3, D_VINHO, D_VINHO_L, D_MARFIM);
    dado(770, 650, 118, 0.7f, 6, D_EBANO, D_EBANO_L, D_MARFIM);
}

static void icone(tela_t *t)
{
    float k = t->w / 176.0f;
    feltro(t, t->w * 0.45f, t->h * 0.4f, t->w * 0.75f);
    dado(t->w * 0.62f, t->h * 0.62f, 74 * k, -0.35f, 3, D_VINHO, D_VINHO_L, D_MARFIM);
    dado(t->w * 0.38f, t->h * 0.4f, 86 * k, 0.28f, 5, D_MARFIM, D_MARFIM_L, D_EBANO);
}

static void logo(int x, int y, int alfa)
{
    const char *nome = "DADO EM CASA";
    float px = 150;
    int w = texto_largura(FONTE_JOGO, px, nome);
    if (w > 680) px = px * 680 / w;
    texto(FONTE_JOGO, px, x + 6, y + 6, nome, 0, alfa * 150 / 255);
    texto(FONTE_JOGO, px, x, y, nome, D_OURO, alfa);
}

// ---------------------------------------------------------------------------
// O jogo
// ---------------------------------------------------------------------------
static bool aberto, ja_abriu;
static int partidas;

static void abre(void)
{
    if (aberto) return;
    dado_inicia(partidas++);
    aberto = ja_abriu = true;
}

static void fecha(void)
{
    aberto = false;
    som_rufo(false);
    som_giro(false);
}

static void passo(float dt) { dado_passo(dt); }

static const uint16_t *desenha(int *w, int *h)
{
    texto_jogo_limpa();
    dado_desenha();
    *w = GFX_W;
    *h = GFX_H;
    return display_fb();
}

static void toque(int x, int y, int acao) { dado_mouse(x, y, acao); }

static void tecla(int k)
{
    key_event_t ev = { (uint16_t)k };
    dado_tecla(&ev);
}

static bool digitando(void) { return dado_digitando(); }

// Segurar o dedo: a tecla I (tudo sobre o dado em foco). Fora de foco, nada.
static bool detalhe(void)
{
    if (dado_digitando()) return false;
    tecla('i');
    return true;
}

// "Animacoes rapidas" e a tecla V. Antes da primeira partida o jogo ainda
// nao leu o que esta guardado: ai a opcao muda direto no salvamento.
static bool opcao_le(int i)
{
    char b[8];
    return i == 0 && salva_le("vel", b, sizeof b) > 0 && b[0] == '1';
}

static void opcao_troca(int i)
{
    if (i != 0) return;
    if (ja_abriu) tecla('v');
    else salva_grava("vel", opcao_le(0) ? "0" : "1");
}

// As dicas do jogo falam de teclas; aqui o aparelho so tem toque. O console
// troca cada uma na hora de desenhar (centralizada no mesmo lugar).
static const char *const TROCAS[][2] = {
    { "mouse ou setas escolhem  ·  clique ou ENTER confirma  ·  ESC volta",
      "toque para escolher  ·  o  <  no alto volta" },
    { "M som  ·  N música  ·  F11 tela cheia  ·  - e + tamanho da mesa",
      "toque para escolher  ·  segure o dedo num dado para ver o que ele faz" },
    { "sequência cheia: TAB ou CTRL confirma", "sequência cheia: toque em pronto" },
    { "ENTER segue", "toque para seguir" },
    { "ENTER tira para sempre  ·  BACKSPACE volta", "toque num dado para tirar para sempre  ·  voltar desfaz" },
    { "setas escolhem  ·  ENTER leva  ·  X recusa", "toque no prêmio para levar  ·  ou em recusar" },
    { "setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB/CTRL termina",
      "toque para comprar  ·  segure o dedo para ver detalhes  ·  terminar fecha a loja" },
    { "ENTER nova partida", "toque para uma nova partida" },
    { "qualquer tecla fecha", "toque para fechar" },
    { "<  anterior    ·    ENTER ou  >  próxima    ·    ESC menu",
      "toque à esquerda: anterior    ·    à direita: próxima    ·    <  menu" },
    { "<  anterior    ·    ENTER volta ao menu", "toque à esquerda: anterior    ·    à direita: volta ao menu" },
    { "Aperte ESC (ou toque no <) de novo para sair da partida.", "Toque no  <  de novo para sair da partida." },
    { "setas escolhem  ·  ENTER senta à mesa  ·  TAB bolsa  ·  ESC menu",
      "toque numa mesa para sentar  ·  sua bolsa mostra os dados  ·  <  menu" },
    { "setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB sai da loja",
      "toque para comprar  ·  segure o dedo para ver detalhes" },
    { "setas escolhem  ·  I detalhes  ·  TAB volta ao mapa", "segure o dedo num dado para ver detalhes" },
    { "ENTER volta ao menu", "toque para voltar ao menu" },
    { "ENTER escolhe o prêmio", "toque para escolher o prêmio" },
    { "setas escolhem  ·  ENTER confirma", "toque para escolher" },
    { "setas escolhem  ·  ENTER confirma  ·  I detalhes  ·  BACKSPACE volta",
      "toque para escolher  ·  segure o dedo para ver detalhes" },
    { "ENTER segue para o mapa", "toque para seguir para o mapa" },
    { "setas escolhem  ·  ENTER leva o amuleto", "toque num amuleto para levar" },
    { "setas escolhem  ·  ENTER entra  ·  X apaga  ·  ESC volta", "toque num perfil para entrar  ·  <  volta" },
    { "setas trocam o retrato  ·  digite o nome  ·  ENTER cria  ·  ESC volta",
      "as setas trocam o retrato  ·  digite o nome  ·  OK cria" },
    { "setas escolhem  ·  ENTER começa a run  ·  ESC volta", "toque numa dificuldade para começar  ·  <  volta" },
};

const jogo_t JOGO_DADO = {
    .id = "dado",
    .nome = "Dado em Casa",
    .descricao = "Duelo de apostas com dados. Monte a sequência, jogue um dado de cada vez "
                 "e decida: parar ou arriscar o próximo. Quem levar o pote vence a rodada.",
    .etiquetas = { "1 ou 2 jogadores", "Modo Desafiante", "por ERICZ" },
    .capa = capa,
    .icone = icone,
    .logo = logo,
    .abre = abre,
    .fecha = fecha,
    .passo = passo,
    .desenha = desenha,
    .toque = toque,
    .tecla = tecla,
    .digitando = digitando,
    .detalhe = detalhe,
    .n_opcoes = 1,
    .opcoes = { "Animações rápidas" },
    .opcao_le = opcao_le,
    .opcao_troca = opcao_troca,
    .trocas = TROCAS,
    .n_trocas = (int)(sizeof TROCAS / sizeof TROCAS[0]),
};
