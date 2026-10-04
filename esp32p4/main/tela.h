// A tela de 10,1": painel JD9365 em pe (800x1280) por MIPI-DSI, com dois
// quadros na PSRAM. O console desenha deitado (1280x800) num quadro proprio;
// tela_mostra gira esse quadro com a PPA para o quadro de tras do painel e
// troca os dois no fim da varredura (sem rasgo na imagem).
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t tela_inicia(int versao);       // versao do painel: 1 ou 2 (placa.h, painel_jd9365.h)
uint16_t *tela_quadro(void);             // 1280x800 RGB565
void tela_mostra(void);
void tela_brilho(int pct);               // 0..100 (0 apaga)
void tela_gira(bool gira180);
void tela_id(uint8_t id[3]);             // o que o JD9365 respondeu ao comando 0x04
