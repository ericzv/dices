// O que o console pede ao aparelho. Cada plataforma implementa: o firmware
// do ESP32-P4 (main/sistema_p4.c) e o simulador do PC (simulador/).
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void *sis_aloca(size_t n);           // memoria grande (no P4, a PSRAM)

void sis_brilho(int pct);            // luz da tela, 0..100
void sis_volume(int pct);            // volume geral, 0..100
void sis_gira(bool gira180);         // vira a imagem de cabeca para baixo
int  sis_toque_estado(void);         // 1 o toque respondeu, 0 nao, -1 nao se sabe ainda

bool sis_hora(int *h, int *m);       // false: o relogio nao foi acertado
void sis_acerta_hora(int h, int m);

void sis_reinicia(void);
void sis_desliga(void);              // apaga tudo e dorme; liga de novo pelo botao RST
// A bateria: false se nao ha (ou ainda nao se sabe). pct 0..100, mv a tensao.
bool sis_bateria(int *pct, int *mv, bool *carregando);
// O jogo direto na tela, sem passar pelo quadro do console: 'px' (w x h)
// ampliado k vezes em (x, y), cada pixel passado pela paleta 'lut' (ou como
// esta, se NULL). O aparelho mostra isso junto com o resto do quadro do
// console. false: aqui nao da, o console amplia no proprio quadro.
bool sis_mostra_jogo(const uint16_t *px, int w, int h, int k, int x, int y, const uint16_t *lut);
int  sis_info(char *buf, int n);     // linhas "nome: valor\n" para o cartao "Sobre"

// Cada jogo guarda o que e seu (hal/salva.h) no seu proprio espaco; o console
// usa o espaco "console".
void sis_espaco(const char *nome);
