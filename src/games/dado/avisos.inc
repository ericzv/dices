// ===========================================================================
// Avisos sobre a mesa: a entrada que subiu, o que a maquina fez e
// os de sair da partida, conexao e velocidade
// ===========================================================================
// A entrada subiu: uma faixa vermelha e dourada no alto da mesa, com fichas
// pulando dos lados, por uns dois segundos e meio.
static void desenha_aviso_entrada(void)
{
    if (t_aviso_entrada <= 0) return;
    float e = ENTRADA_AVISO_T - t_aviso_entrada;
    float abre = e < 0.18f ? e / 0.18f : (t_aviso_entrada < 0.25f ? t_aviso_entrada / 0.25f : 1);
    int h = (int)(46 * abre), y = MESA_Y0 + 34 - h / 2;
    if (h < 4) return;
    gfx_rect(MESA_X0, y + 3, MESA_X1 - MESA_X0, h, C_SOMBRA);
    gfx_rect(MESA_X0, y, MESA_X1 - MESA_X0, h, RGB(120, 26, 34));
    gfx_rect(MESA_X0, y, MESA_X1 - MESA_X0, 2, C_OURO);
    gfx_rect(MESA_X0, y + h - 2, MESA_X1 - MESA_X0, 2, C_OURO);
    if (abre < 1) return;
    char t[40];
    snprintf(t, sizeof t, "ENTRADA: %d FICHAS", ante_da(rodada));
    bool pisca = (int)(e / 0.25f) % 2 == 0 && e < 1.0f;      // pisca no comeco
    txt_sombra_c(GFX_W / 2, y + 7, t, pisca ? C_AMARELO : C_OURO, 2);
    int w = gfx_largura(t, 2) / 2;
    for (int lado = -1; lado <= 1; lado += 2)                // pilhas de fichas pulando
        for (int i = 0; i < 3; i++) {
            int x = GFX_W / 2 + lado * (w + 26 + i * 18);
            int pulo = (int)(fabsf(sinf(e * 7 + i * 1.1f)) * 5);
            ficha(x, y + h / 2 - pulo, i % 2 ? C_VINHO : C_LATAO);
        }
}

// O que a maquina acabou de fazer, numa linha, onde ficariam os menus dela.
static void desenha_anuncio(void)
{
    int y1 = CENTRO - 40;
    if (!n_anuncio_peca) {
        painel(y1 - 5, TXT_H + 8, larg_painel(gfx_largura(anuncio, 1), 160));
        txt_c(GFX_W / 2, y1 - 1, anuncio, anuncio_cor, true);
        return;
    }
    // Com os dados que a maquina levou: cada um desenhado, com o nome embaixo.
    const int passo = 104, h = 104;
    int n = n_anuncio_peca, y0 = CENTRO - h / 2 - 6;
    painel(y0, h, larg_painel(n * passo - 24, gfx_largura(anuncio, 1) + 36));
    txt_c(GFX_W / 2, y0 + 6, anuncio, anuncio_cor, true);
    for (int i = 0; i < n; i++) {
        int x = GFX_W / 2 + (2 * i - (n - 1)) * passo / 2;
        peca_parada((float)x, (float)(y0 + 52), 18, anuncio_peca[i], anuncio_dono);
        txt_c(x, y0 + 76, TIPO[anuncio_peca[i].tipo].nome, cor_dado(anuncio_peca[i].tipo), false);
        dica_zona(x - 22, y0 + 30, 44, 44, DICA_DADO, anuncio_peca[i].tipo);
    }
}

// A maquina montando a sequencia: so o nome, sem a mao e sem o menu.
static void desenha_maquina_monta(void)
{
    char t[64];
    snprintf(t, sizeof t, "%s monta a sequência", J[vez].nome);
    int y1 = CENTRO - 40;
    painel(y1 - 5, TXT_H + 8, larg_painel(gfx_largura(t, 1), 160));
    txt_c(GFX_W / 2, y1 - 1, t, J[vez].cor, true);
}

static void desenha_avisos(void)
{
    const char *msg = NULL;
    uint16_t c = C_OURO;
    if (conexao_caiu()) { msg = "A conexão com o oponente caiu.  ESC volta ao menu."; c = C_VINHO_CLR; }
    else if (t_sair >= 0) msg = "Aperte ESC (ou toque no <) de novo para sair da partida.";
    else if (t_aviso_vel > 0) msg = rapido ? "Velocidade: rápida  (V troca)" : "Velocidade: normal  (V troca)";
    if (!msg) return;
    int w = gfx_largura(msg, 1) + 40;
    ui_painel(GFX_W / 2 - w / 2, MESA_Y0 + 6, w, TXT_H + 12);
    txt_c(GFX_W / 2, MESA_Y0 + 12, msg, c, false);
}
