// Rede da versao do navegador: a conversa com o servidor fica no JavaScript
// da pagina (web/shell.html, objeto DadoRede); aqui so a ponte.
#include <emscripten.h>
#include "games/jogos.h"

EM_JS(int,  js_disponivel, (void), { return typeof DadoRede !== 'undefined' ? 1 : 0; });
EM_JS(void, js_cria,   (void), { DadoRede.cria(); });
EM_JS(void, js_entra,  (const char *c), { DadoRede.entra(UTF8ToString(c)); });
EM_JS(void, js_envia,  (int k), { DadoRede.envia(k); });
EM_JS(void, js_sai,    (void), { DadoRede.sai(); });
EM_JS(int,  js_estado, (void), { return DadoRede.estado; });
EM_JS(void, js_marco,  (int i, int h), { DadoRede.marco(i, h >>> 0); });
EM_JS(void, js_copia,  (void), { DadoRede.copia(); });
EM_JS(int,  js_copiado, (void), { return DadoRede.copiado | 0; });
// 0: o codigo da sala, 1: o erro, 2: o link de convite.
EM_JS(void, js_texto,  (int qual, char *buf, int n), {
    stringToUTF8(qual === 1 ? DadoRede.erro : qual === 2 ? DadoRede.convite : DadoRede.codigo, buf, n);
});

static char codigo[16], erro[160], convite[256];

bool rede_disponivel(void)          { return js_disponivel(); }
void rede_cria(void)                { js_cria(); }
void rede_entra(const char *c)      { js_entra(c); }
void rede_envia_tecla(int k)        { js_envia(k); }
void rede_sai(void)                 { js_sai(); }
int  rede_estado(void)              { return js_estado(); }
const char *rede_codigo(void)       { js_texto(0, codigo, sizeof codigo); return codigo; }
const char *rede_erro(void)         { js_texto(1, erro, sizeof erro); return erro; }
void rede_marco(int i, uint32_t h)  { js_marco(i, (int)h); }
const char *rede_convite(void)      { js_texto(2, convite, sizeof convite); return convite; }
void rede_copia_convite(void)       { js_copia(); }
int  rede_convite_copiado(void)     { return js_copiado(); }
