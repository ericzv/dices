// Salvamento do Modo Desafiante no PC e no navegador, uma entrada por chave.
//   Windows: %APPDATA%\DadoEmCasa\<chave>.sav
//   macOS:   ~/Library/Application Support/DadoEmCasa/<chave>.sav
//   Linux:   $XDG_DATA_HOME (ou ~/.local/share)/DadoEmCasa/<chave>.sav
//   Navegador: localStorage["dadoemcasa.<chave>"]
// A chave "desafio" e a mesma do tempo em que so havia uma run: ela continua
// valendo (vira a run do primeiro perfil).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hal/salva.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// O site de teste (/teste/) guarda em outras chaves: nao mistura com o
// site principal.
EM_JS(void, ls_grava, (const char *k, const char *t), {
    var c = "dadoemcasa." + UTF8ToString(k) + (location.pathname.indexOf("/teste/") >= 0 ? ".teste" : "");
    try { localStorage.setItem(c, UTF8ToString(t)); } catch (e) {}
});
EM_JS(int, ls_le, (const char *k, char *buf, int n), {
    var c = "dadoemcasa." + UTF8ToString(k) + (location.pathname.indexOf("/teste/") >= 0 ? ".teste" : "");
    var t = null;
    try { t = localStorage.getItem(c); } catch (e) {}
    if (!t) return 0;
    stringToUTF8(t, buf, n);
    return Math.min(t.length, n - 1);
});
EM_JS(void, ls_apaga, (const char *k), {
    var c = "dadoemcasa." + UTF8ToString(k) + (location.pathname.indexOf("/teste/") >= 0 ? ".teste" : "");
    try { localStorage.removeItem(c); } catch (e) {}
});

bool salva_grava(const char *chave, const char *texto) { ls_grava(chave, texto); return true; }
int  salva_le(const char *chave, char *buf, int n)     { buf[0] = 0; return ls_le(chave, buf, n); }
void salva_apaga(const char *chave)                    { ls_apaga(chave); }

#else
#ifdef _WIN32
#include <direct.h>
#define CRIA_PASTA(p) _mkdir(p)
#else
#include <sys/stat.h>
#define CRIA_PASTA(p) mkdir(p, 0755)
#endif

// A pasta do jogo, criada se faltar.
static const char *pasta(void)
{
    static char p[1024];
    if (p[0]) return p;
    char base[900] = "";
#if defined(_WIN32)
    const char *a = getenv("APPDATA");
    snprintf(base, sizeof base, "%s", a ? a : ".");
#elif defined(__APPLE__)
    const char *h = getenv("HOME");
    snprintf(base, sizeof base, "%s/Library/Application Support", h ? h : ".");
#else
    const char *x = getenv("XDG_DATA_HOME"), *h = getenv("HOME");
    if (x && *x) snprintf(base, sizeof base, "%s", x);
    else {
        snprintf(base, sizeof base, "%s/.local", h ? h : ".");
        CRIA_PASTA(base);
        strncat(base, "/share", sizeof base - strlen(base) - 1);
    }
#endif
    CRIA_PASTA(base);
    snprintf(p, sizeof p, "%s/DadoEmCasa", base);
    CRIA_PASTA(p);
    return p;
}

static void caminho(const char *chave, char *p, size_t n)
{
    snprintf(p, n, "%s/%s.sav", pasta(), chave);
}

bool salva_grava(const char *chave, const char *texto)
{
    // Grava num arquivo ao lado e troca: um desligamento no meio nao estraga
    // o que estava guardado.
    char c[1100], tmp[1110];
    caminho(chave, c, sizeof c);
    snprintf(tmp, sizeof tmp, "%s.novo", c);
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = fputs(texto, f) >= 0;
    ok = fclose(f) == 0 && ok;
    if (!ok) { remove(tmp); return false; }
    remove(c);
    return rename(tmp, c) == 0;
}

int salva_le(const char *chave, char *buf, int n)
{
    char c[1100];
    caminho(chave, c, sizeof c);
    buf[0] = 0;
    FILE *f = fopen(c, "rb");
    if (!f) return 0;
    int lidos = (int)fread(buf, 1, (size_t)(n - 1), f);
    fclose(f);
    if (lidos < 0) lidos = 0;
    buf[lidos] = 0;
    return lidos;
}

void salva_apaga(const char *chave)
{
    char c[1100];
    caminho(chave, c, sizeof c);
    remove(c);
}
#endif
