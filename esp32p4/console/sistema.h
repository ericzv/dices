// O que o console pede ao aparelho. Cada plataforma implementa: o firmware
// do ESP32-P4 (main/sistema_p4.c) e o simulador do PC (simulador/).
#pragma once
#include <stdbool.h>
#include <stddef.h>

void *sis_aloca(size_t n);           // memoria grande (no P4, a PSRAM)

void sis_brilho(int pct);            // luz da tela, 0..100
void sis_volume(int pct);            // volume geral, 0..100
void sis_gira(bool gira180);         // vira a imagem de cabeca para baixo
int  sis_toque_estado(void);         // 1 o toque respondeu, 0 nao, -1 nao se sabe ainda

bool sis_hora(int *h, int *m);       // false: o relogio nao foi acertado
void sis_acerta_hora(int h, int m);

int  sis_painel(void);               // versao do painel em uso (1 ou 2), 0 se nao se aplica
void sis_troca_painel(int v);        // guarda a versao e reinicia o aparelho
void sis_reinicia(void);
int  sis_info(char *buf, int n);     // linhas "nome: valor\n" para o cartao "Sobre"

// Cada jogo guarda o que e seu (hal/salva.h) no seu proprio espaco; o console
// usa o espaco "console".
void sis_espaco(const char *nome);
