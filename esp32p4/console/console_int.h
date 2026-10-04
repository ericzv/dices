// O que as telas do console dividem entre si (nao e para os jogos).
#pragma once
#include <stdbool.h>
#include "console.h"
#include "desenho.h"
#include "texto.h"
#include "catalogo.h"
#include "sistema.h"

#define C_BRANCO   COR(255, 255, 255)
#define C_TEXTO2   COR(200, 207, 224)
#define C_TEXTO3   COR(134, 144, 168)
#define C_AZUL     COR(66, 130, 255)
#define C_FUNDO    COR(10, 15, 30)
#define C_FUNDO2   COR(3, 5, 12)
#define C_CARTAO   COR(22, 29, 48)
#define C_LINHA    COR(46, 55, 80)
#define C_OURO     COR(214, 178, 98)
#define C_VERMELHO COR(222, 60, 70)
#define C_NOITE    COR(8, 12, 24)
#define C_BARRA    COR(16, 16, 15)        // as faixas do Dado em Casa

#define JOGO_H     720                    // o jogo ocupa 1280x720 (640x360 em 2x)
#define BARRA_Y    JOGO_H

enum tela_id { T_PARTIDA, T_CALIBRA, T_ORIENTA, T_INICIO, T_AJUSTES, T_JOGO, T_CENTRAL };

typedef struct {
    int volume, brilho;                   // 0..100
    bool musica, efeitos, gira;
    bool calibrado;
    int troca;                            // 1: o eixo x da tela vem do y cru
    float ax, bx, ay, by;                 // tela = a * cru + b, por eixo
} prefs_t;

typedef struct {
    int tela;
    float t_tela;                         // segundos na tela atual
    float fade;                           // escurecido da entrada que ainda falta (s)
    bool sujo;                            // redesenhar
    float anima;                          // seguir redesenhando por mais tantos segundos
    const jogo_t *ativo;                  // jogo aberto (rodando ou em pausa)
    int foco;                             // jogo em foco no inicio
    int armado;                           // botao que pede um segundo toque (-1: nenhum)
    float t_armado;
    int hora, minuto;                     // relogio mostrado (-1: sem relogio)
    bool central_de_ajustes;              // os ajustes vieram da central: voltam para ela
} console_t;

extern prefs_t P;
extern console_t C;

// Toque: zonas marcadas no desenho; tocar numa e soltar dentro dispara a acao.
enum { Z_BOTAO, Z_DESLIZA };
void z_limpa(void);
void z_marca(int id, int x, int y, int w, int h, int tipo);
int  z_pressionada(void);                 // a zona sob o dedo agora (-1)
bool z_desliza_valor(int id, int x0, int w, int *valor, int vmin, int vmax);

void console_vai(int tela);               // troca de tela (com a entrada escurecida)
void console_marca_sujo(void);
void prefs_grava(void);
void console_aplica_som(void);            // musica e efeitos conforme a tela e os ajustes
void console_abre_jogo(const jogo_t *j);
void console_fecha_jogo(void);
void console_calibra_gira(void);          // a imagem virou 180: a calibracao vira junto
void console_arma(int id);                // primeiro toque de "tem certeza?"
bool console_armado(int id);

// Desenhos reaproveitados.
void ui_icone_casa(float cx, float cy, float s, cor_t c, int alfa);
void ui_icone_play(float cx, float cy, float s, cor_t c, int alfa);
void ui_icone_x(float cx, float cy, float s, cor_t c, int alfa);
void ui_icone_engrenagem(float cx, float cy, float r, cor_t c, cor_t fundo);
void ui_icone_volume(float cx, float cy, float s, int ondas, cor_t c, int alfa);
void ui_icone_sol(float cx, float cy, float s, cor_t c, int alfa);
void ui_icone_voltar(float cx, float cy, float s, cor_t c, int alfa);
void ui_icone_girar(float cx, float cy, float s, cor_t c, int alfa);
void ui_icone_mais(float cx, float cy, float s, cor_t c, int alfa);
void ui_icone_tecla_apaga(float cx, float cy, float s, cor_t c, int alfa);
void ui_icone_mao(float cx, float cy, float s, cor_t c, int alfa);

enum { B_PRIMARIO, B_VIDRO, B_PERIGO, B_AZUL };
void ui_botao(int id, int x, int y, int w, int h, const char *rotulo, int icone, int estilo, float px);
enum { IC_NADA, IC_PLAY, IC_CASA, IC_X, IC_GIRAR, IC_MAO, IC_VOLTAR };
void ui_alterna(int id, int x, int y, bool ligado);           // 76x42
void ui_desliza(int id, int x, int y, int w, int valor, int vmin, int vmax);
void ui_cartao(int x, int y, int w, int h, const char *titulo);
void ui_relogio(int xd, int y, float px, cor_t c);            // hora no canto, se houver

// As telas.
void inicio_desenha(void);
void inicio_acao(int id);
void inicio_capa_suja(void);
void ajustes_desenha(void);
void ajustes_acao(int id);
void ajustes_desliza(int id, int x);
void jogo_desenha_tela(void);
void jogo_acao(int id);
void jogo_desliza(int id, int x);
void jogo_toque(int ev, int x, int y);    // toque na mesa
bool jogo_teclado_aberto(void);
int  jogo_limite(void);                   // ate onde (y) o toque e da mesa
void central_desenha(void);
void central_acao(int id);
void central_desliza(int id, int x);
void central_abre(void);
void primeira_desenha(void);
void primeira_acao(int id);
void calibra_toque(int ev, int rx, int ry);
void calibra_comeca(void);
void calibra_passo(float dt);
void partida_passo(float dt);

// Eventos de toque passados as telas.
enum { EV_APERTA, EV_MOVE, EV_SOLTA, EV_LONGO, EV_SOLTA_LONGO };
