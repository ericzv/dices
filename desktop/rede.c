// Versao de computador sem rede: o modo online mora na versao do navegador
// (web/rede.c). O menu mostra o endereco dela.
#include "games/jogos.h"

bool rede_disponivel(void)            { return false; }
void rede_cria(void)                  { }
void rede_entra(const char *codigo)   { (void)codigo; }
void rede_envia_tecla(int k)          { (void)k; }
void rede_sai(void)                   { }
int  rede_estado(void)                { return REDE_PARADA; }
const char *rede_codigo(void)         { return ""; }
const char *rede_erro(void)           { return ""; }
void rede_marco(int i, uint32_t h)    { (void)i; (void)h; }
const char *rede_convite(void)        { return ""; }
void rede_copia_convite(void)         { }
int  rede_convite_copiado(void)       { return 0; }
