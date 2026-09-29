// Salvamento do Modo Desafiante no PC e no navegador.
//   Windows: %APPDATA%\DadoEmCasa\desafio.sav
//   macOS:   ~/Library/Application Support/DadoEmCasa/desafio.sav
//   Linux:   $XDG_DATA_HOME (ou ~/.local/share)/DadoEmCasa/desafio.sav
//   Navegador: localStorage["dadoemcasa.desafio"]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hal/salva.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, ls_grava, (const char *t), {
    try { localStorage.setItem('dadoemcasa.desafio', UTF8ToString(t)); } catch (e) {}
});
EM_JS(int, ls_le, (char *buf, int n), {
    var t = null;
    try { t = localStorage.getItem('dadoemcasa.desafio'); } catch (e) {}
    if (!t) return 0;
    stringToUTF8(t, buf, n);
    return Math.min(t.length, n - 1);
});
EM_JS(void, ls_apaga, (void), {
    try { localStorage.removeItem('dadoemcasa.desafio'); } catch (e) {}
});

bool salva_grava(const char *texto) { ls_grava(texto); return true; }
int  salva_le(char *buf, int n)     { return ls_le(buf, n); }
void salva_apaga(void)              { ls_apaga(); }

#else
#ifdef _WIN32
#include <direct.h>
#define CRIA_PASTA(p) _mkdir(p)
#else
#include <sys/stat.h>
#define CRIA_PASTA(p) mkdir(p, 0755)
#endif

// Monta o caminho do arquivo e cria as pastas que faltarem.
static const char *caminho(void)
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
    strncat(p, "/desafio.sav", sizeof p - strlen(p) - 1);
    return p;
}

bool salva_grava(const char *texto)
{
    // Grava num arquivo ao lado e troca: um desligamento no meio nao estraga
    // a run guardada.
    char tmp[1100];
    snprintf(tmp, sizeof tmp, "%s.novo", caminho());
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = fputs(texto, f) >= 0;
    ok = fclose(f) == 0 && ok;
    if (!ok) { remove(tmp); return false; }
    remove(caminho());
    return rename(tmp, caminho()) == 0;
}

int salva_le(char *buf, int n)
{
    FILE *f = fopen(caminho(), "rb");
    if (!f) return 0;
    int lidos = (int)fread(buf, 1, (size_t)(n - 1), f);
    fclose(f);
    buf[lidos < 0 ? 0 : lidos] = 0;
    return lidos < 0 ? 0 : lidos;
}

void salva_apaga(void) { remove(caminho()); }
#endif
