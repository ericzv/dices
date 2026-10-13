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
    else if (t_aviso_tremor > 0) msg = tremor_off ? "Tremor da tela: desligado  (T troca)" : "Tremor da tela: ligado  (T troca)";
    if (!msg) return;
    int w = gfx_largura(msg, 1) + 40;
    ui_painel(GFX_W / 2 - w / 2, MESA_Y0 + 6, w, TXT_H + 12);
    txt_c(GFX_W / 2, MESA_Y0 + 12, msg, c, false);
}

// O Dado Misterioso: a pedra negativa se partiu e havia um dado dentro. A mesa
// escurece, um facho de luz desce do alto, raios giram atras e o d20 preto
// aparece no meio dele, com um pulo.
static void desenha_misterio(void)
{
    if (t_misterio <= MIST_ESPERA) return;
    float t = t_misterio - MIST_ESPERA;
    float e = t < 0.6f ? t / 0.6f : 1, ee = e * e * (3 - 2 * e);
    const int cx = GFX_W / 2, cy = 160;
    uint16_t *fb = display_fb();
    // A mesa no escuro, menos o facho (um cone do alto ate o dado) e a poca
    // de luz em volta dele; no primeiro instante, um clarao.
    float clarao = t < 0.3f ? 1 - t / 0.3f : 0;
    for (int y = 0; y < GFX_H; y++) {
        float meia = 16 + (y + 30) * 0.26f;              // a meia largura do facho nesta linha
        for (int x = 0; x < GFX_W; x++) {
            float dx = (float)(x - cx), dy = (y - cy) * 1.5f;
            float poca = 1 - sqrtf(dx * dx + dy * dy) / 96;
            float facho = y < cy ? 1 - fabsf(dx) / meia : 0;
            float luz = poca > facho ? poca : facho;
            if (luz < 0) luz = 0;
            uint16_t *p = &fb[y * GFX_W + x];
            int a = (int)(ee * (88 - 30 * luz) + (BAYER[y & 3][x & 3] - 7.5f) * 1.6f);
            if (a > 0) *p = gfx_mistura(*p, C_BARRA, a > 100 ? 100 : a);
            int b = (int)(luz * (26 + 50 * clarao) + (BAYER[y & 3][x & 3] - 7.5f) * 1.2f);
            if (b > 0) *p = gfx_mistura(*p, C_AMARELO, b > 100 ? 100 : b);
        }
    }
    // Os raios, girando devagar atras do dado.
    float giro = t * 0.35f;
    for (int y = cy - 92; y <= cy + 92; y++)
        for (int x = cx - 92; x <= cx + 92; x++) {
            float dx = (float)(x - cx), dy = (float)(y - cy), d = sqrtf(dx * dx + dy * dy);
            if (d < 30 || d > 92) continue;
            float an = atan2f(dy, dx) / 6.2832f * 12 + giro;
            if (an - floorf(an) > 0.42f) continue;
            int a = (int)(ee * 34 * (1 - (d - 30) / 62) + (BAYER[y & 3][x & 3] - 7.5f) * 1.2f);
            if (a > 0) fb[y * GFX_W + x] = gfx_mistura(fb[y * GFX_W + x], C_OURO, a);
        }
    // O dado: cresce com um pulo, balanca um pouco e fica.
    float q = t < 0.45f ? t / 0.45f : 1;
    float esc = q < 1 ? q * (1.25f - 0.25f * q) + 0.12f * sinf(q * 3.1416f) : 1 + 0.02f * sinf(t * 2.2f);
    float r = 42 * esc;
    if (r > 2) desenha_dado((float)cx, (float)cy, r, D_MISTERIOSO, 0, 0.12f * sinf(t * 1.3f), TIPO[D_MISTERIOSO].lados, 0, false);
    // Faiscas que piscam em volta.
    for (int i = 0; i < 6; i++) {
        float an = i * 1.0472f + 0.4f, b = sinf(t * 2.3f + i * 0.9f);
        if (b < 0.35f || t < 0.3f) continue;
        int fx = cx + (int)(cosf(an) * 70), fy = cy + (int)(sinf(an) * 58), m = b > 0.8f ? 3 : 2;
        gfx_rect(fx - m, fy, 2 * m + 1, 1, C_AMARELO);
        gfx_rect(fx, fy - m, 1, 2 * m + 1, C_AMARELO);
    }
    if (t < 0.35f) return;
    txt_sombra_c(GFX_W / 2, cy + 62, "Dado Misterioso", C_OURO, 2);
    txt_c(GFX_W / 2, cy + 98, "a pedra negativa se partiu, e havia um d20 dentro", C_MARFIM_S, false);
    if (t > MIST_MIN) teclas_c(GFX_W / 2, cy + 124, "ENTER continua", C_TEXTO_M);
}
