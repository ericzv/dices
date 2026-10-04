// O que o simulador de PC expoe as suas duas frentes (janela e fotos).
#pragma once
#include <stdbool.h>

extern int pc_brilho, pc_volume;
extern bool pc_gira;
void pc_pasta_salva(const char *pasta);   // NULL: salvamento so na memoria
void pc_volume_mudou(int pct);            // cada frente faz o que quiser com isso
