// A tela de 10,1": painel JD9365 em pe (800x1280) por MIPI-DSI, com tres
// quadros na PSRAM. O console desenha deitado (1280x800) num quadro proprio;
// tela_mostra gira esse quadro com a PPA para um quadro livre do painel, que
// entra no fim da varredura (sem rasgo na imagem).
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

esp_err_t tela_inicia(int versao);       // versao do painel: 1 ou 2 (placa.h, painel_jd9365.h)
uint16_t *tela_quadro(void);             // 1280x800 RGB565
void tela_mostra(void);
// O jogo sem passar pelo quadro do console: 'px' (w x h), ampliado k vezes e
// passado pela paleta 'lut' (NULL: como esta), vai para (x, y) da tela
// deitada; o resto vem do quadro do console. Volta na hora: o segundo nucleo
// monta o quadro do painel e da 'livre' quando ja nao le 'px'. false: aqui
// nao da; use tela_mostra.
bool tela_mostra_jogo(const uint16_t *px, int w, int h, int k, int x, int y, const uint16_t *lut,
                      SemaphoreHandle_t livre);
// Para o log: quadros montados pelo segundo nucleo desde a ultima chamada e
// quanto cada um levou, em ms.
void tela_tempos(int *quadros, float *ms_giro);
void tela_brilho(int pct);               // 0..100 (0 apaga)
void tela_gira(bool gira180);
void tela_id(uint8_t id[3]);
// Apaga a luz, poe o painel para dormir e segura os pinos dele em baixo
// (para o sono profundo do aparelho).
void tela_desliga(void);             // o que o JD9365 respondeu ao comando 0x04
