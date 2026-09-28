// Salvamento do Modo Desafiante: um texto curto guardado entre uma sessao e
// outra. No PC vai para um arquivo na pasta do usuario; no navegador, para o
// localStorage do site. Nos testes sem janela, fica na memoria.
#pragma once
#include <stdbool.h>

bool salva_grava(const char *texto);
int  salva_le(char *buf, int n);        // bytes lidos (0: nao ha nada guardado)
void salva_apaga(void);
