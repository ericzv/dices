// ---------------------------------------------------------------------------
// O pop-up da bolsa: clicar numa bolsa (a sua ou a do oponente, nas faixas da
// partida; a sua, nas telas da run) mostra os dados dela por cima de tudo. So
// olha: fora do online a partida espera, e qualquer tecla ou clique fecha.
// Mostra todos os dados de quem e, sem marcar os que ja sairam: nao da para
// saber o que ele comprou (so a Caixa de Fosforos ve isso).
// ---------------------------------------------------------------------------
static void bolsa_abre(int quem)
{
    if (quem < 0 || quem > BOLSA_RUN) return;
    bolsa_pop = quem;
    t_bolsa_pop = 0;
    detalhe = -1;
    if (fase == F_ROLANDO) som_rufo(false);           // o dado no ar espera calado
    som_toca(SOM_BOLSA_ESPIA);
}

static void bolsa_fecha(void)
{
    bolsa_pop = -1;
    if (fase == F_ROLANDO && n_voo) {
        if (TIPO[voo[0].tipo].lados == 2) som_rufo_moeda();
        else som_rufo(true);
    }
}

// Tecla B: a sua bolsa, se ela esta a vista nesta tela.
static bool bolsa_abre_tecla(void)
{
    if (bolsas_vistas >> baixo & 1) bolsa_abre(baixo);
    else if (bolsas_vistas >> BOLSA_RUN & 1) bolsa_abre(BOLSA_RUN);
    else return false;
    return true;
}

static void desenha_bolsa_pop(void)
{
    if (bolsa_pop < 0) return;
    n_zonas = 0;                                      // so o que esta no pop-up responde ao mouse
    n_dicas = 0;
    const peca_t *col;
    int n, dono = 0;
    if (bolsa_pop == BOLSA_RUN) { col = run.col; n = run.n_col; }
    else {
        const jogador_t *p = &J[bolsa_pop];
        col = p->col; n = p->n_col; dono = bolsa_pop;
    }
    // Os do mesmo tipo juntos.
    static int ord[MAX_COL];
    int no = 0;
    for (int t = 0; t < D_N; t++)
        for (int i = 0; i < n; i++)
            if (col[i].tipo == t) ord[no++] = i;
    bool sua = modo == M_DESAFIO && (bolsa_pop == BOLSA_RUN || bolsa_pop == 0);   // a da run: a regra dela

    float q = fminf(1, t_bolsa_pop / 0.16f);
    ui_escurece(0, 0, GFX_W, GFX_H, (int)(150 * q));
    grade_t g = grade_cabe(no, 14, 36, 168);
    float f = g.f;
    int linhas = no ? (no + g.por - 1) / g.por : 1, passo = (int)(36 * f);
    int w = 548, h = 78 + linhas * passo + 30 + (sua ? 16 : 0);
    int x0 = GFX_W / 2 - w / 2, y0 = CENTRO - h / 2 + ui_entra(t_bolsa_pop, 0, UI_ENTRA_PLACA);   // a entrada padrao
    ui_cartao_aceso(x0, y0, w, h, true, C_LATAO);

    char s[96];
    if (bolsa_pop == BOLSA_RUN || (bolsa_pop == baixo && modo != M_DOIS)) snprintf(s, sizeof s, "SUA BOLSA");
    else snprintf(s, sizeof s, "BOLSA DE %s", J[bolsa_pop].nome);
    int wt = gfx_largura(s, 2);
    if (sua) icone_da_run(GFX_W / 2 - wt / 2 - 16, y0 + 22);
    else icone_bolsa(GFX_W / 2 - wt / 2 - 14, y0 + 22);
    txt_c2(GFX_W / 2 + 4, y0 + 8, s, C_LATAO);
    if (sua) {                                        // a bolsa da run e a regra dela
        const bolsa_t *bo = &BOLSA[run.bolsa < B_N ? run.bolsa : 0];
        snprintf(s, sizeof s, "%s  ·  %d dados", bo->nome, n);
        txt_c(GFX_W / 2, y0 + 42, s, bo->cor == BOLSA[B_PANO].cor ? C_MARFIM_S : bo->cor, true);
        char r2[200];
        if (run.bolsa == B_PANO) snprintf(r2, sizeof r2, "Sem regra.");
        else snprintf(r2, sizeof r2, "%s %s", bo->ganho, bo->custo);
        int f = gfx_largura(r2, GFX_FINO) <= 560 ? GFX_FINO : GFX_MIUDO;
        gfx_texto(GFX_W / 2 - gfx_largura(r2, f) / 2, y0 + (f == GFX_FINO ? 56 : 62), r2, C_TEXTO_M, f, false);
    } else {
        snprintf(s, sizeof s, "%d dados", n);
        txt_c(GFX_W / 2, y0 + 44, s, C_MARFIM_S, false);
    }

    // Os dados. O mouse em cima diz o que cada um faz.
    int topo = y0 + 66 + (sua ? 16 : 0), r = (int)(13 * f + 0.5f);
    for (int k = 0; k < no; k++) {
        int i = ord[k], lin = k / g.por, c = k % g.por;
        int na_linha = no - lin * g.por < g.por ? no - lin * g.por : g.por;
        int x = GFX_W / 2 + (int)((c - (na_linha - 1) / 2.0f) * 36 * f);
        int y = topo + (int)((lin + 0.5f) * 36 * f);
        caixa_fechada = col == J[0].col && i == embrulho;
        peca_parada((float)x, (float)y, (float)r, col[i], dono);
        if (col == J[0].col && i == embrulho) dica_zona(x - r, y - r, 2 * r, 2 * r, DICA_DADO, DICA_EMBRULHO);
        else dica_peca(x - r, y - r, 2 * r, 2 * r, &col[i]);
    }
    if (!no) txt_c(GFX_W / 2, topo + 8, "Nenhum dado.", C_TEXTO_M, false);
    txt_c(GFX_W / 2, y0 + h - 24, "clique ou qualquer tecla fecha", C_TEXTO_M, false);
}
