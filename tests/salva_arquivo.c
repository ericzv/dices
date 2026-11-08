// O salvamento em arquivo do PC (desktop/salva.c): grava num ".novo" ao lado
// e troca. Se o jogo desligar entre apagar o velho e renomear o novo, o que
// fica e so o ".novo" - e ele tem de abrir. E apagar leva os dois.
// A pasta vem do ctest (XDG_DATA_HOME, APPDATA e HOME apontam para a do teste).
#include <stdio.h>
#include <string.h>
#include "salva.c"

static int falhas;
#define CONFERE(cond, ...) do { if (!(cond)) { printf("FALHOU %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); putchar('\n'); falhas++; } } while (0)

static void escreve(const char *p, const char *t)
{
    FILE *f = fopen(p, "wb");
    if (f) { fputs(t, f); fclose(f); }
}

int main(void)
{
    char c[1100], novo[1110], b[64];
    caminho("teste", c, sizeof c);
    snprintf(novo, sizeof novo, "%s.novo", c);
    remove(c); remove(novo);
    // O caminho normal.
    CONFERE(salva_grava("teste", "um") && salva_le("teste", b, sizeof b) == 2 && !strcmp(b, "um"), "gravar e ler: %s", b);
    // Desligou depois de apagar o velho, antes de renomear: so o ".novo".
    escreve(novo, "dois");
    remove(c);
    CONFERE(salva_le("teste", b, sizeof b) == 4 && !strcmp(b, "dois"), "o .novo sozinho nao abriu: '%s'", b);
    CONFERE(salva_grava("teste", "tres") && salva_le("teste", b, sizeof b) == 4 && !strcmp(b, "tres"), "gravar depois: %s", b);
    // Com os dois, vale o principal (o ".novo" pode estar pela metade).
    escreve(novo, "meio");
    CONFERE(salva_le("teste", b, sizeof b) == 4 && !strcmp(b, "tres"), "com os dois, leu '%s'", b);
    // Apagar leva os dois: nada volta pelo ".novo".
    salva_apaga("teste");
    CONFERE(salva_le("teste", b, sizeof b) == 0, "apagado, mas leu '%s'", b);
    if (falhas) { printf("%d falhas\n", falhas); return 1; }
    printf("salva_arquivo ok\n");
    return 0;
}
