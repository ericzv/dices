// ===========================================================================
// O quadro dos controles (tecla K, o LT/L2 do controle, ou o botao
// "controles" do menu)
// ===========================================================================
// O que cada botao do controle faz, com os botoes desenhados como nas dicas
// da tela (interface.inc). Mostra o controle que se esta usando; as setas
// trocam entre o do Xbox e o do PlayStation. Qualquer outra tecla, ou um
// clique fora das setas, fecha. Fora do online, a partida espera.
static bool  ctrl_aberto;
static int   ctrl_estilo = CONTROLE_XBOX;      // o controle mostrado
static float t_ctrl;

static void ctrl_abre(void)
{
    ctrl_aberto = true;
    t_ctrl = 0;
    if (controle_estilo) ctrl_estilo = controle_estilo;
    som_toca(SOM_TIQUE);
}

static void ctrl_tecla(int k)
{
    if (k == KEY_LEFT || k == KEY_RIGHT || k == 'a' || k == 'd') {
        ctrl_estilo = ctrl_estilo == CONTROLE_PS ? CONTROLE_XBOX : CONTROLE_PS;
        som_toca(SOM_TIQUE);
        return;
    }
    if (k == KEY_UP || k == KEY_DOWN) return;
    ctrl_aberto = false;
}

static bool ctrl_passo(float dt_real)
{
    if (ctrl_aberto) t_ctrl += dt_real;
    return ctrl_aberto;
}

// Uma linha: os botoes (na sintaxe das dicas: "ENTER", "setas"...) e o que
// fazem, a partir de x.
static void ctrl_linha(int x, int y, const char *s)
{
    linha_teclas(x, y, s, C_MARFIM_S, true);
}

static void desenha_controles(void)
{
    if (!ctrl_aberto) return;
    n_zonas = 0;                                 // nada por baixo responde ao clique
    const int w = 500, h = 262, x = GFX_W / 2 - w / 2;
    int y = CENTRO - h / 2 - 4 + ui_entra(t_ctrl, 0, 14);
    ui_escurece(0, 0, GFX_W, GFX_H, 110);
    ui_painel(x, y, w, h);
    txt_c(GFX_W / 2, y + 6, "CONTROLES", C_LATAO, true);

    // Qual controle: o nome entre duas setas (clicaveis).
    const char *nome = ctrl_estilo == CONTROLE_PS ? "PlayStation" : "Xbox";
    int nw = gfx_largura(nome, 1), ny = y + 28;
    txt_c(GFX_W / 2, ny, nome, C_OURO, false);
    int sx0 = GFX_W / 2 - nw / 2 - 22, sx1 = GFX_W / 2 + nw / 2 + 10;
    for (int i = 0; i < 4; i++) {
        gfx_rect(sx0 + 3 + i, ny + 11 - i, 1, 2 * i + 1, C_LATAO);          // <
        gfx_rect(sx1 + 8 - i, ny + 11 - i, 1, 2 * i + 1, C_LATAO);          // >
    }
    zona_clique(sx0 - 6, ny, 24, 22, NULL, 0, KEY_LEFT);
    zona_clique(sx1 - 6, ny, 24, 22, NULL, 0, KEY_RIGHT);

    // Os botoes, desenhados no estilo do controle mostrado.
    int antes = controle_estilo;
    controle_estilo = ctrl_estilo;
    static const char *const ESQ[] = {
        "setas movem o cursor",
        "ENTER escolhe e joga o dado",
        "BACKSPACE volta ou tira o último dado",
        "TAB segue ou confirma; nos perfis, o histórico",
        "I mostra o dado; nos perfis, o estilo",
    };
    static const char *const DIR[] = {
        "ESC volta ou sai da partida",
        "B a sua bolsa",
        "X recusa",
        "V velocidade",
        "M o som, a letra e o tremor",
        "K este quadro",
    };
    int y0 = y + 62, col1 = x + 22, col2 = x + w / 2 + 16;
    gfx_rect(x + w / 2 + 2, y0 - 4, 1, 6 * 24 - 2, C_LATAO_ESC);          // o fio entre as colunas
    for (int i = 0; i < (int)(sizeof ESQ / sizeof ESQ[0]); i++) ctrl_linha(col1, y0 + i * 24, ESQ[i]);
    for (int i = 0; i < (int)(sizeof DIR / sizeof DIR[0]); i++) ctrl_linha(col2, y0 + i * 24, DIR[i]);
    controle_estilo = antes;

    txt_c(GFX_W / 2, y + h - 52, "2 jogadores: com dois controles, cada um joga na sua vez.", C_TEXTO_M, false);
    teclas_c(GFX_W / 2, y + h - 22, "setas trocam o controle  ·  ENTER fecha", C_TEXTO_M);
}
