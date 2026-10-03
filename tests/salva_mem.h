// Salvamento na memoria, para os testes sem janela: algumas chaves, cada
// uma com o seu texto.
#include <string.h>
#include "hal/salva.h"

static struct { char chave[16]; char texto[4096]; } salva_mem[8];

static int salva_slot(const char *chave, bool cria)
{
    for (int i = 0; i < 8; i++) if (!strcmp(salva_mem[i].chave, chave)) return i;
    if (!cria) return -1;
    for (int i = 0; i < 8; i++)
        if (!salva_mem[i].chave[0]) { snprintf(salva_mem[i].chave, sizeof salva_mem[i].chave, "%s", chave); return i; }
    return -1;
}
bool salva_grava(const char *chave, const char *t)
{
    int i = salva_slot(chave, true);
    if (i < 0) return false;
    snprintf(salva_mem[i].texto, sizeof salva_mem[i].texto, "%s", t);
    return true;
}
int salva_le(const char *chave, char *buf, int n)
{
    int i = salva_slot(chave, false);
    if (i < 0) { buf[0] = 0; return 0; }
    int l = (int)strlen(salva_mem[i].texto);
    if (l > n - 1) l = n - 1;
    memcpy(buf, salva_mem[i].texto, (size_t)l);
    buf[l] = 0;
    return l;
}
void salva_apaga(const char *chave)
{
    int i = salva_slot(chave, false);
    if (i >= 0) salva_mem[i].chave[0] = salva_mem[i].texto[0] = 0;
}
// Zera tudo (cada teste comeca limpo).
static void salva_limpa_tudo(void) { memset(salva_mem, 0, sizeof salva_mem); }
