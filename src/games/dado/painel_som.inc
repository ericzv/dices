// ===========================================================================
// O painel do som (tecla M)
// ===========================================================================
// Tres volumes, de 0 (mudo) a 10, cada um guardado: a musica, os efeitos da
// mesa e a interface (o cursor e os menus). As setas escolhem a linha e
// mudam o volume, que se ouve na hora: a trilha sobe e desce, o efeito toca
// uma ficha, a interface um tique. O mouse clica direto no degrau. N liga e
// desliga so a musica, voltando ao volume de antes.
static bool  som_aberto;
static int   som_cur;                          // a linha escolhida
static float t_som;                            // ha quanto tempo o painel abriu
static int   som_escada_x;                     // onde comeca a escada (o clique acha o degrau)
static int   som_musica_antes = SOM_NIVEIS;    // o volume da musica antes do N
#define TECLA_SOM  0x201                       // o clique numa linha do painel (val: a linha)
#define SOM_DEGRAU 13                          // um degrau da escada, com o vao
static const char *const SOM_ROTULO[SOM_CANAIS] = { "Música", "Efeitos", "Interface" };

static void som_grava(void)
{
    char s[24];
    snprintf(s, sizeof s, "%d,%d,%d", som_volume_de(SOM_CANAL_MUSICA), som_volume_de(SOM_CANAL_EFEITOS),
             som_volume_de(SOM_CANAL_INTERFACE));
    salva_grava("som", s);
}

// Os volumes da ultima vez (dado_inicia).
static void som_le_volumes(void)
{
    char b[24];
    int v[SOM_CANAIS];
    if (salva_le("som", b, sizeof b) > 0 && sscanf(b, "%d,%d,%d", &v[0], &v[1], &v[2]) == SOM_CANAIS)
        for (int c = 0; c < SOM_CANAIS; c++) som_volume(c, v[c]);
    if (som_volume_de(SOM_CANAL_MUSICA) > 0) som_musica_antes = som_volume_de(SOM_CANAL_MUSICA);
}

// Muda o volume do canal c para n e deixa ouvir como ficou.
static void som_muda(int c, int n)
{
    int antes = som_volume_de(c);
    som_volume(c, n);
    int agora = som_volume_de(c);
    if (agora == antes) return;
    if (c == SOM_CANAL_MUSICA && agora > 0) som_musica_antes = agora;
    if (c == SOM_CANAL_EFEITOS) som_toca(SOM_FICHA);
    else if (c == SOM_CANAL_INTERFACE) som_toca(SOM_TIQUE);
    som_grava();
}

// N: so a musica, liga e desliga (volta ao volume de antes).
static void som_alterna_musica(void)
{
    som_muda(SOM_CANAL_MUSICA, som_volume_de(SOM_CANAL_MUSICA) ? 0 : som_musica_antes);
}

static void som_abre(void)
{
    som_aberto = true;
    t_som = 0;
    som_toca(SOM_TIQUE);
}

// Com o painel aberto, ele fica com todas as teclas.
static void som_tecla(int k)
{
    if (k == KEY_UP || k == 'w') { if (som_cur > 0) { som_cur--; som_toca(SOM_TIQUE); } return; }
    if (k == KEY_DOWN || k == 's') { if (som_cur < SOM_CANAIS - 1) { som_cur++; som_toca(SOM_TIQUE); } return; }
    if (k == KEY_LEFT || k == 'a') { som_muda(som_cur, som_volume_de(som_cur) - 1); return; }
    if (k == KEY_RIGHT || k == 'd') { som_muda(som_cur, som_volume_de(som_cur) + 1); return; }
    if (k == 'n') { som_alterna_musica(); return; }
    if (k == TECLA_SOM) {                       // o clique na linha: o degrau embaixo do mouse (antes dele, mudo)
        int dx = mouse_x - som_escada_x;
        if (dx >= -SOM_DEGRAU) som_muda(som_cur, dx < 0 ? 0 : dx / SOM_DEGRAU + 1);
        return;
    }
    if (k == 'm' || k == KEY_ESC || k == KEY_ENTER || k == KEY_TAB) som_aberto = false;
}

// O tempo do painel e a trilha abaixando enquanto a cadeia de efeitos corre.
// Devolve se o painel esta aberto: fora do online, a partida espera por ele.
static bool som_passo(float dt_real)
{
    if (som_aberto) t_som += dt_real;
    som_cadeia(fase == F_EFEITOS);
    return som_aberto;
}

// A placa no meio da tela, por cima de tudo: cada canal numa linha, com o
// nome, a escada de 10 degraus (cada um um pouco mais alto, acesos ate o
// volume) e o numero.
static void desenha_som(void)
{
    if (!som_aberto) return;
    n_zonas = 0;                                 // nada por baixo responde ao clique
    const int w = 300, h = 136, x = GFX_W / 2 - w / 2;
    int y = CENTRO - h / 2 - 6 + ui_entra(t_som, 0, 14);
    ui_escurece(0, 0, GFX_W, GFX_H, 110);
    ui_painel(x, y, w, h);
    txt_c(GFX_W / 2, y + 6, "SOM", C_LATAO, true);
    som_escada_x = x + 112;
    for (int c = 0; c < SOM_CANAIS; c++) {
        int ly = y + 40 + c * 24, nv = som_volume_de(c);
        bool cur = c == som_cur;
        if (cur) ui_cantos(x + 10, ly - 4, w - 20, 22, C_OURO);
        txt(x + 22, ly - 2, SOM_ROTULO[c], cur ? C_MARFIM : C_MARFIM_S, false);
        for (int i = 0; i < SOM_NIVEIS; i++) {
            int dh = 6 + i, dx = som_escada_x + i * SOM_DEGRAU, dy = ly + 15 - dh;
            bool aceso = i < nv;
            gfx_rect(dx - 1, dy - 1, SOM_DEGRAU - 2, dh + 2, C_BARRA);
            gfx_rect(dx, dy, SOM_DEGRAU - 4, dh, aceso ? (cur ? C_OURO : C_LATAO) : C_PLACA_LUZ);
            if (aceso) gfx_rect(dx, dy, SOM_DEGRAU - 4, 1, cur ? C_AMARELO : C_OURO);   // a luz em cima
        }
        char s[8];
        snprintf(s, sizeof s, "%d", nv);
        int nx = som_escada_x + SOM_NIVEIS * SOM_DEGRAU + 6;
        if (nv) txt(nx, ly - 4, s, cur ? C_OURO : C_MARFIM_S, true);
        else txt(nx, ly - 2, "mudo", C_VINHO_CLR, false);
        zona_clique(x + 10, ly - 4, w - 20, 22, &som_cur, c, TECLA_SOM);
    }
    if (som_falhou()) txt_c(GFX_W / 2, y + h - 40, "sem saída de som neste computador", C_VINHO_CLR, false);
    teclas_c(GFX_W / 2, y + h - 20, "setas mudam  ·  N música  ·  M fecha", C_TEXTO_M);
}
