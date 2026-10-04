// Tela de inicio: a fileira de jogos no alto, a arte do jogo em foco no fundo
// e, embaixo, o nome, o que ele e e o botao de jogar (ou de continuar, se o
// jogo ficou em pausa). A engrenagem do canto abre os ajustes.
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "console_int.h"
#include "hal/som.h"

enum { A_AJUSTES = 100, A_JOGAR, A_FECHAR, A_TILE0 = 120 };

#define FILA_Y    100
#define TAM_FOCO  176
#define TAM       128
#define RAIO      30

static uint16_t *capa_px;                 // arte + sombras, pronta para copiar
static int capa_de = -1;
static uint16_t *icone_px[8][2];          // [jogo][0 grande, 1 pequeno]

void inicio_capa_suja(void) { capa_de = -1; }

static void prepara_capa(void)
{
    if (!capa_px) capa_px = sis_aloca((size_t)TELA_W * TELA_H * 2);
    if (!capa_px || capa_de == C.foco) return;
    tela_t t = { capa_px, TELA_W, TELA_H }, *antes = d_alvo_atual();
    d_alvo(&t);
    const jogo_t *j = CATALOGO[C.foco];
    if (j->capa) j->capa(&t);
    else d_degrade_v(0, 0, TELA_W, TELA_H, C_FUNDO, C_FUNDO2);
    // Sombras para o texto ler bem: da esquerda, do alto e de baixo.
    d_degrade_h_alfa(0, 0, 940, TELA_H, C_NOITE, 238, 0);
    d_degrade_v_alfa(0, 0, TELA_W, 170, C_NOITE, 190, 0);
    d_degrade_v_alfa(0, 420, TELA_W, 380, C_NOITE, 0, 225);
    d_alvo(antes);
    capa_de = C.foco;
}

static const uint16_t *icone_de(int i, int tam)
{
    int k = tam == TAM_FOCO ? 0 : 1;
    if (i >= 8) return NULL;
    if (!icone_px[i][k]) {
        icone_px[i][k] = sis_aloca((size_t)tam * tam * 2);
        if (!icone_px[i][k]) return NULL;
        tela_t t = { icone_px[i][k], tam, tam }, *antes = d_alvo_atual();
        d_alvo(&t);
        d_degrade_v(0, 0, tam, tam, C_CARTAO, C_FUNDO);
        if (CATALOGO[i]->icone) CATALOGO[i]->icone(&t);
        d_alvo(antes);
    }
    return icone_px[i][k];
}

// Copia a imagem so dentro de um quadrado de cantos arredondados.
static void copia_arred(const uint16_t *src, int tam, int dx, int dy, float r)
{
    tela_t *t = d_alvo_atual();
    float h = tam / 2.0f - r;
    for (int j = 0; j < tam; j++) {
        int y = dy + j;
        if (y < 0 || y >= t->h) continue;
        float py = j + 0.5f - tam / 2.0f, qy = fabsf(py) - h;
        for (int i = 0; i < tam; i++) {
            int x = dx + i;
            if (x < 0 || x >= t->w) continue;
            float px = i + 0.5f - tam / 2.0f, qx = fabsf(px) - h;
            float cob = 1;
            if (qx > 0 && qy > 0) cob = 0.5f - (sqrtf(qx * qx + qy * qy) - r);
            if (cob <= 0) continue;
            uint16_t *p = t->px + y * t->w + x;
            *p = cob >= 1 ? src[j * tam + i] : d_mistura(*p, src[j * tam + i], (int)(cob * 255));
        }
    }
}

static void desenha_fileira(void)
{
    int x = 64;
    for (int i = 0; i < N_CATALOGO; i++) {
        const jogo_t *j = CATALOGO[i];
        bool foco = i == C.foco;
        int tam = foco ? TAM_FOCO : TAM, y = foco ? FILA_Y : FILA_Y + (TAM_FOCO - TAM) / 2;
        bool aperta = z_pressionada() == A_TILE0 + i;
        d_arred(x + 3.0f, y + 12.0f, (float)tam, (float)tam, RAIO, 0, 110);
        const uint16_t *ic = icone_de(i, tam);
        if (ic) copia_arred(ic, tam, x, y, foco ? RAIO : RAIO * 0.75f);
        if (aperta) d_arred((float)x, (float)y, (float)tam, (float)tam, RAIO, 0, 70);
        if (foco) d_arred_borda(x - 7.0f, y - 7.0f, tam + 14.0f, tam + 14.0f, RAIO + 7.0f, 4, C_BRANCO, 255);
        z_marca(A_TILE0 + i, x, y, tam, tam, Z_BOTAO);
        if (foco) {
            int lx = x + tam + 30, ly = FILA_Y + TAM_FOCO / 2 - 30;
            int w = texto(FONTE_NEGRITO, 30, lx, ly, j->nome, C_BRANCO, 255);
            if (C.ativo == j) texto(FONTE_NEGRITO, 19, lx, ly + 40, "EM PAUSA", C_OURO, 255);
            else texto(FONTE_TEXTO, 19, lx, ly + 40, "Pronto para jogar", C_TEXTO2, 255);
            x = lx + (w > 200 ? w : 200) + 40;
        } else {
            x += tam + 26;
        }
    }
    // O lugar dos proximos jogos.
    int y = FILA_Y + (TAM_FOCO - TAM) / 2;
    d_arred((float)x, (float)y, TAM, TAM, RAIO * 0.75f, C_BRANCO, 14);
    d_arred_borda((float)x, (float)y, TAM, TAM, RAIO * 0.75f, 2, C_BRANCO, 60);
    ui_icone_mais(x + TAM / 2.0f, y + TAM / 2.0f - 12, 34, C_BRANCO, 150);
    texto_c(FONTE_NEGRITO, 16, x + TAM / 2, y + TAM / 2 + 20, "Em breve", C_TEXTO2, 200);
}

static void desenha_topo(void)
{
    int w = texto(FONTE_PRETO, 32, 64, 30, "Jogos", C_BRANCO, 255);
    d_arred(64, 74, (float)w, 4, 2, C_BRANCO, 255);
    // Engrenagem dos ajustes, no canto.
    bool aperta = z_pressionada() == A_AJUSTES;
    float gx = 1184, gy = 58;
    d_circulo(gx, gy, 34, C_BRANCO, aperta ? 80 : 26);
    ui_icone_engrenagem(gx, gy, 22, C_BRANCO, d_mistura(C_NOITE, C_BRANCO, aperta ? 80 : 26));
    z_marca(A_AJUSTES, (int)gx - 44, (int)gy - 44, 88, 88, Z_BOTAO);
    ui_relogio(1130, 40, 32, C_BRANCO);
}

static void desenha_destaque(void)
{
    const jogo_t *j = CATALOGO[C.foco];
    if (j->logo) j->logo(64, 334, 255);
    else texto(FONTE_PRETO, 76, 64, 340, j->nome, C_BRANCO, 255);
    int y = 476 + texto_quebra(FONTE_TEXTO, 25, 64, 476, 640, 1.38f, j->descricao, C_TEXTO2, 255);
    // Etiquetas
    int x = 64;
    y += 14;
    for (int k = 0; k < 3 && j->etiquetas[k]; k++) {
        int w = texto_largura(FONTE_NEGRITO, 17, j->etiquetas[k]) + 30;
        d_arred((float)x, (float)y, (float)w, 36, 18, C_BRANCO, 30);
        texto(FONTE_NEGRITO, 17, x + 15, y + 9, j->etiquetas[k], C_BRANCO, 230);
        x += w + 10;
    }
    // Botoes
    bool pausado = C.ativo == j;
    int by = 664;
    ui_botao(A_JOGAR, 64, by, 270, 78, pausado ? "Continuar" : "Jogar", IC_PLAY, B_PRIMARIO, 30);
    if (pausado) {
        if (console_armado(A_FECHAR)) {
            ui_botao(A_FECHAR, 354, by, 270, 78, "Fechar o jogo", IC_X, B_PERIGO, 24);
            texto(FONTE_TEXTO, 20, 646, by + 14, "Toque de novo para fechar.", C_BRANCO, 230);
            texto(FONTE_TEXTO, 20, 646, by + 42, "A partida em andamento se perde.", C_TEXTO2, 220);
        } else {
            ui_botao(A_FECHAR, 354, by, 78, 78, NULL, IC_X, B_VIDRO, 24);
        }
    }
}

void inicio_desenha(void)
{
    prepara_capa();
    if (capa_px) d_copia(capa_px, TELA_W, TELA_H, 0, 0);
    else d_degrade_v(0, 0, TELA_W, TELA_H, C_FUNDO, C_FUNDO2);
    desenha_topo();
    desenha_fileira();
    desenha_destaque();
}

void inicio_acao(int id)
{
    if (id >= A_TILE0 && id < A_TILE0 + N_CATALOGO) {
        int i = id - A_TILE0;
        if (i == C.foco) console_abre_jogo(CATALOGO[i]);
        else {
            C.foco = i;
            C.armado = -1;
            som_toca(SOM_TIQUE);
        }
        return;
    }
    switch (id) {
    case A_JOGAR:
        console_abre_jogo(CATALOGO[C.foco]);
        break;
    case A_FECHAR:
        if (console_armado(A_FECHAR)) {
            console_fecha_jogo();
            C.armado = -1;
            som_toca(SOM_ANULA);
        } else {
            console_arma(A_FECHAR);
            som_toca(SOM_TIQUE);
        }
        break;
    case A_AJUSTES:
        C.central_de_ajustes = false;
        console_vai(T_AJUSTES);
        som_toca(SOM_TIQUE);
        break;
    }
}
