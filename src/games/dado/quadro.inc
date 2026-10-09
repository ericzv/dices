// ===========================================================================
// O quadro: as dicas do mouse e cada tela desenhada por fase
// ===========================================================================
static void desenha_tudo(void);

// ---------------------------------------------------------------------------
// Dica do mouse: uma caixa com o nome e o que a coisa faz, por cima de tudo.
// ---------------------------------------------------------------------------

static void dica_textos(int tipo, int id, const char **nome, uint16_t *cor, const char **desc)
{
    if (tipo == DICA_DADO)         { *nome = TIPO[id].nome; *cor = cor_dado(id); *desc = TEXTO_MEDIO[id]; }
    else if (tipo == DICA_AMULETO) { *nome = AMULETO[id].nome; *cor = AMULETO[id].cor; *desc = AMULETO[id].desc; }
    else {
        *nome = PERSO[id].trunfo; *cor = C_ROSA; *desc = PERSO[id].regra;
        int v = virada_de(id);
        if (v) {                                         // o chefe tambem conta a virada
            static char t[200];
            char vt[96];
            virada_txt(id - PERSO_CHEFE0, v, false, vt, sizeof vt);
            snprintf(t, sizeof t, "%s Virada na 5ª rodada: %s", PERSO[id].regra, vt);
            *desc = t;
        }
    }
}

// A caixa da dica sob o mouse, se houver. Devolve o retangulo em r.
static bool dica_rect(int *r, const dica_t **qual)
{
    if (mouse_x < 0 || (detalhe >= 0 && fase == detalhe_fase)) return false;
    const dica_t *d = NULL;
    for (int i = n_dicas - 1; i >= 0 && !d; i--)
        if (mouse_em(dicas[i].x, dicas[i].y, dicas[i].w, dicas[i].h)) d = &dicas[i];
    if (!d) return false;
    const char *nome, *desc;
    uint16_t cor;
    dica_textos(d->tipo, d->id, &nome, &cor, &desc);
    int larg = 260;
    quebra_so_conta = true;                              // so conta as linhas
    int linhas = quebra_linhas(0, 0, larg - 16, 7, desc, 0);
    quebra_so_conta = false;
    int w = larg, h = 10 + (TXT_H + 2) + linhas * LINHA_FINA + 6;
    int x = mouse_x + 14, y = mouse_y + 16;
    if (x + w > GFX_W - 4) x = mouse_x - 14 - w;
    if (y + h > GFX_H - 4) y = mouse_y - 12 - h;
    if (x < 4) x = 4;
    if (y < 4) y = 4;
    r[0] = x; r[1] = y; r[2] = w; r[3] = h;
    *qual = d;
    return true;
}

static void desenha_dica(const int *r, const dica_t *d)
{
    const char *nome, *desc;
    uint16_t cor;
    dica_textos(d->tipo, d->id, &nome, &cor, &desc);
    ui_painel(r[0], r[1], r[2], r[3]);
    gfx_rect(r[0] + 3, r[1] + 3, 3, r[3] - 6, cor);             // a cor da coisa, na beira
    int x = r[0] + 8, y = r[1] + 5;
    int w = gfx_texto(x, y, nome, cor, 1, true);
    if (d->tipo == DICA_DADO) {                          // "D4 · raro", miudo, ao lado
        char c2[32];
        classe(d->id, c2, sizeof c2);
        gfx_texto(x + w + 8, y + 3, c2, cor_rar(d->id), GFX_MIUDO, false);
    }
    quebra_linhas(x, y + TXT_H + 2, r[2] - 16, 7, desc, C_MARFIM_S);
}

void dado_desenha(void)
{
    n_zonas = 0;
    n_dicas = 0;
    zonas_fase = fase;
    desenha_tudo();
    desenha_flutua();
    // O que a maquina fez fica por cima dos numeros que ainda sobem dos dados.
    if (t_anuncio > 0 && (fase == F_ORDEM || fase == F_APOSTA || fase == F_JOGA || fase == F_LOJA))
        desenha_anuncio();
    balao_oponente();
    desenha_confete();
    bool menu = fase == F_CATALOGO || fase == F_TUTORIAL || fase == F_ONLINE || fase == F_ABERTURA;
    if (!menu) {
        desenha_detalhe();
        desenha_avisos();
        desenha_aviso_entrada();
        des_elevador();
        des_virada_anuncio();
    }
    int r[4];
    const dica_t *d = NULL;
    bool tem_dica = !menu && dica_rect(r, &d);
    if (tem_dica) desenha_dica(r, d);
    gfx_quantiza();                                      // o quadro so com cores da paleta
}

static void desenha_tudo(void)
{
    mesa();
    if (des_desenha()) return;
    if (fase == F_CATALOGO) { desenha_catalogo(); return; }
    if (fase == F_TUTORIAL) { desenha_tutorial(); return; }
    if (fase == F_ONLINE)   { desenha_online(); return; }
    if (fase == F_ABERTURA) {
        desenha_abertura();
        return;
    }
    faixa_jogador(1);
    faixa_jogador(0);

    switch (fase) {
    // A loja e o premio da maquina contra uma pessoa nao aparecem: so a
    // mesa, e o anuncio do que ela levou por cima.
    case F_LOJA:   if (!anuncia_maquina() && t_anuncio <= 0) desenha_loja();   break;
    case F_PREMIO: if (!anuncia_maquina()) desenha_premio(); break;
    case F_FIM:    if (modo == M_DESAFIO) desenha_fim_partida_run(); else desenha_fim(); break;
    case F_ORDEM:
        desenha_fila(outro(vez));
        if (t_anuncio > 0) break;                // o anuncio vem por cima, no fim
        if (anuncia_maquina()) {                 // a maquina monta sem menu
            desenha_fila(vez);
            desenha_maquina_monta();
            break;
        }
        desenha_ordem();
        break;
    default:
        desenha_fila(outro(vez));
        desenha_fila(vez);
        if (t_anuncio > 0 && (fase == F_APOSTA || fase == F_JOGA)) pote_mesa();   // o anuncio vem no fim
        else if (fase == F_APOSTA)    desenha_aposta();
        else if (fase == F_RESULTADO) desenha_resultado();
        else if (fase == F_EFEITOS)   desenha_efeitos();
        else                          desenha_joga();
        if (fase == F_ROLANDO || fase == F_ARRUMA) desenha_lance();
        break;
    }
}
