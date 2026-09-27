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
EM_JS(void, js_texto,  (int qual, char *buf, int n), {
    stringToUTF8(qual ? DadoRede.erro : DadoRede.codigo, buf, n);
});

static char codigo[16], erro[96];

bool rede_disponivel(void)          { return js_disponivel(); }
void rede_cria(void)                { js_cria(); }
void rede_entra(const char *c)      { js_entra(c); }
void rede_envia_tecla(int k)        { js_envia(k); }
void rede_sai(void)                 { js_sai(); }
int  rede_estado(void)              { return js_estado(); }
const char *rede_codigo(void)       { js_texto(0, codigo, sizeof codigo); return codigo; }
const char *rede_erro(void)         { js_texto(1, erro, sizeof erro); return erro; }
