// O som do aparelho alem do hal/som.h: carregar os sons, misturar (para a
// tarefa de audio) e o que o PC chama de plataforma (trilha e passo).
#pragma once
#include <stdbool.h>
#include <stdint.h>

bool som_carrega(void);
void som_mistura(int16_t *saida, int n);
void som_atualiza(float dt);
bool som_musica(bool on);
bool som_musica_ligada(void);
