// Os sons do jogo, embutidos no executavel. O .c e gerado no build pelo
// cmake/embute_sons.cmake a partir de desktop/sons/*.ogg.
#pragma once

typedef struct {
    const char          *nome;    // o nome do arquivo, sem o .ogg
    const unsigned char *dados;   // o arquivo OGG inteiro
    int                  tam;
} som_arquivo_t;

extern const som_arquivo_t SONS_ARQ[];
extern const int SONS_N;
