// Salvamento do Modo Desafiante: textos curtos guardados entre uma sessao e
// outra, cada um com a sua chave ("perfis", "desafio", "desafio2"...). No PC
// cada chave vira um arquivo na pasta do usuario; no navegador, uma entrada
// do localStorage do site. Nos testes sem janela, fica na memoria.
#pragma once
#include <stdbool.h>

bool salva_grava(const char *chave, const char *texto);
int  salva_le(const char *chave, char *buf, int n);   // bytes lidos (0: nada guardado)
void salva_apaga(const char *chave);
