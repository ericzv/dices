// ---------------------------------------------------------------------------
// Colecao: o que esta aberto e o que esta fechado no Desafiante
// ---------------------------------------------------------------------------
// Cadeado: (x, y) e o meio do topo do corpo; e = escala (1 ou 2).
static void cadeado(int x, int y, uint16_t c, int e)
{
    for (int i = 0; i <= 12; i++) {                      // a alca
        float a = 3.1416f + i * 0.2618f;
        gfx_rect(x + (int)(cosf(a) * 3 * e), y + (int)(sinf(a) * 3.5f * e), e, e, c);
    }
    gfx_rect(x - 4 * e, y, 9 * e, 7 * e, c);             // o corpo
    gfx_rect(x - 4 * e, y, 9 * e, e, gfx_mistura(c, C_BRANCO, 30));
    gfx_rect(x, y + 2 * e, e, 3 * e, C_BARRA);           // a fechadura
}

static void quem_abre(int nivel, int marco, char *s, int n)
{
    if (marco == 0) snprintf(s, (size_t)n, "vença o Crupiê");
    else if (marco == 1) snprintf(s, (size_t)n, "vença a Dona do Salão");
    else if (nivel == 1) snprintf(s, (size_t)n, "vença o Barão");
    else snprintf(s, (size_t)n, "vença o Barão no %s", NOME_DIF_CURTO[nivel - 1]);
}

// 0: sempre livre; 1: ja aberto; 2: fechado (e o que falta em s).
static int cat_trava_dado(int t, char *s, int n)
{
    perfis_le();
    int m = marco_do_dado(t), nv = nivel_do_dado(t);
    if (m < 0 && nv == 0) return 0;
    if (dado_liberado(t)) {
        if (perfil_atual >= 0) snprintf(s, (size_t)n, "Aberto no Desafiante (%s)", perfil()->nome);
        else snprintf(s, (size_t)n, "Aberto no Desafiante");
        return 1;
    }
    quem_abre(nv, m, s, n);
    return 2;
}

// Os amuletos na ordem em que abrem.
static int cat_amuleto(int i)
{
    for (int nv = 0; nv <= N_NIVEIS; nv++)
        for (int a = 0; a < A_N; a++)
            if (NIVEL_AMULETO[a] == nv && i-- == 0) return a;
    return 0;
}

static void desenha_cat_amuleto(int i)
{
    perfis_le();
    int a = cat_amuleto(i);
    char s[96];
    faixas_colecao(i, A_N);

    bool aberto = amuleto_liberado(a);
    int cx = 160, cy = CENTRO + 4;
    gfx_disco(cx, cy, 80, gfx_mistura(C_FELTRO_ESC, AMULETO[a].cor, 45));
    gfx_disco(cx, cy, 77, C_FELTRO_ESC);
    desenha_amuleto(cx, cy + (int)(sinf(t_fase * 2) * 2), 40, a, !aberto);
    if (!aberto) {                                   // fechado: no escuro
        no_escuro(cx, cy, 76, C_FELTRO_ESC);
        cadeado(cx + 54, cy - 66, C_VINHO_CLR, 2);
    }

    int x = 290;
    gfx_texto(x, 70, AMULETO[a].nome, aberto ? AMULETO[a].cor : C_TEXTO_M, 2, true);
    const char *et = "amuleto";
    txt(etiqueta_caixa(x, 118, et, C_LATAO) + 2, 119, "só no Desafiante", C_TEXTO_M, false);
    if (NIVEL_AMULETO[a] == 0) txt(x, 139, "Aberto desde o começo", C_TEXTO_M, false);
    else if (aberto) {
        if (perfil_atual >= 0) snprintf(s, sizeof s, "Aberto no Desafiante (%s)", perfil()->nome);
        else snprintf(s, sizeof s, "Aberto no Desafiante");
        txt(x, 139, s, C_VERDE, false);
    } else {
        quem_abre(NIVEL_AMULETO[a], -1, s, sizeof s);
        linha_bloqueio(x, 139, s);
    }
    gfx_rect(x, 160, 320, 1, C_FELTRO2);
    quebra_linhas(x, 170, 320, 6, AMULETO[a].desc, C_MARFIM);
}

static int des_dificuldade(void) { return run.dificuldade; }

// O elevador: um mostrador antigo, em arco, com o ponteiro subindo do terreo
// ate o andar da run. Pula com qualquer tecla.
static void des_elevador(void)
{
    if (t_elev <= 0 || modo != M_DESAFIO) return;
    n_zonas = 0;                                 // nada por baixo responde ao clique
    float e = ELEV_T - t_elev;
    int d = run.dificuldade;
    const int w = 250, h = 150, x0 = GFX_W / 2 - w / 2, y0 = CENTRO - h / 2 - 4;
    const int cx = GFX_W / 2, cy = y0 + 106, R = 64;
    gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_SOMBRA, 150);
    ui_painel(x0, y0, w, h);
    for (int i = 0; i <= 60; i++) {              // o arco do mostrador
        float a = 3.1416f * (0.08f + 0.84f * i / 60.0f);
        gfx_rect(cx - (int)(cosf(a) * R), cy - (int)(sinf(a) * R), 2, 2, C_LATAO_ESC);
    }
    // O ponteiro: sai do terreo, passa um pouco do andar e assenta.
    float q = e / ELEV_CHEGA;
    if (q > 1) q = 1;
    float p = d * (1 - (1 - q) * (1 - q) * (1 - q));
    if (q >= 1 && e < ELEV_CHEGA + 0.25f) p = d + sinf((e - ELEV_CHEGA) / 0.25f * 3.1416f) * 0.08f * (d > 0);
    bool chegou = e >= ELEV_CHEGA;
    static const char *const MARCA[N_NIVEIS + 1] = { "T", "1", "2", "3", "4", "C" };
    for (int i = 0; i <= N_NIVEIS; i++) {
        float a = 3.1416f * (0.08f + 0.84f * i / N_NIVEIS);
        int mx = cx - (int)(cosf(a) * R), my = cy - (int)(sinf(a) * R);
        bool aceso = chegou && i == d;
        gfx_disco(mx, my, 3, aceso ? cor_dif(i) : C_LATAO_ESC);
        int lx = cx - (int)(cosf(a) * (R + 15)), ly = cy - (int)(sinf(a) * (R + 15));
        txt_c(lx, ly - TXT_H / 2 - 1, MARCA[i], aceso ? cor_dif(i) : C_TEXTO_M, true);
    }
    float a = 3.1416f * (0.08f + 0.84f * p / N_NIVEIS);
    gfx_linha_grossa(cx, cy, cx - (int)(cosf(a) * (R - 12)), cy - (int)(sinf(a) * (R - 12)), 2, C_OURO);
    gfx_disco(cx, cy, 6, C_LATAO_ESC);
    gfx_disco(cx, cy, 4, C_LATAO);
    txt_c(cx, cy + 12, chegou ? ANDAR_DIF[d] : "subindo...", chegou ? (d ? cor_dif(d) : C_MARFIM) : C_TEXTO_M, chegou);
}

// O chefe vira na 5a rodada: a mesa para um instante, uma faixa atravessa a
// tela, o retrato muda e ele diz a frase dele.
static void des_virou(void)
{
    perso_virado = fala_perso;
    t_virada = 2.4f;
    som_toca(SOM_ZERA);
    snprintf(fala_txt, sizeof fala_txt, "%s", VIRADA_FRASE[fala_perso - PERSO_CHEFE0]);
    t_fala = 3.4f;
}

static void des_virada_anuncio(void)
{
    if (t_virada <= 0 || perso_virado < PERSO_CHEFE0) return;
    int c = perso_virado - PERSO_CHEFE0;
    float e = 2.4f - t_virada;
    n_zonas = 0;
    int alfa = e < 0.2f ? (int)(e / 0.2f * 140) : 140;
    gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_SOMBRA, alfa);
    static const uint16_t COR[3] = { RGB(46, 120, 84), RGB(150, 30, 46), RGB(150, 36, 40) };
    float q = e / 0.3f;
    if (q > 1) q = 1;
    int dx = -(int)((1 - (1 - q) * (1 - q)) * GFX_W) + GFX_W;   // entra pela esquerda
    int y = CENTRO - 30, h = 60;
    gfx_rect(-dx, y, GFX_W, h, gfx_mistura(COR[c], C_BARRA, 55));
    gfx_rect(-dx, y, GFX_W, 2, C_OURO);
    gfx_rect(-dx, y + h - 2, GFX_W, 2, C_OURO);
    if (q < 1) return;
    txt_sombra_c(GFX_W / 2, y + 6, VIRADA_TITULO[c], C_OURO, 2);
    char s[96];
    virada_txt(c, virada_forca(c), false, s, sizeof s);
    txt_c(GFX_W / 2, y + 36, s, C_MARFIM, false);
}

static bool des_desenha(void)
{
    switch (fase) {
    case F_MAPA:   desenha_mapa();      return true;
    case F_RLOJA:  desenha_loja_run();  return true;
    case F_RBOLSA: desenha_bolsa_run(); return true;
    case F_RFIM:   desenha_fim_run();   return true;
    case F_CREDITOS: desenha_creditos(); return true;
    case F_RAMULETO: desenha_amuleto_run(); return true;
    case F_PERFIS: desenha_perfis();    return true;
    case F_PCRIA:  desenha_cria_perfil(); return true;
    case F_RDIFIC: desenha_dificuldade(); return true;
    case F_REVENTO: desenha_evt();   return true;
    default:       return false;
    }
}

// ESC nas telas de perfil e dificuldade: um passo para tras.
static void des_esc(void)
{
    if (fase == F_PERFIS && pf_apagando) { pf_apagando = false; return; }
    if (fase == F_REVENTO) {                     // escolhendo dado: volta as escolhas; senao, menu
        if (ev_passo == 1) ev_tecla(KEY_BKSP, 0);
        else if (!ev_girando()) volta_ao_menu();
        return;
    }
    if (fase == F_PCRIA) { t_fase = 0; fase = F_PERFIS; return; }
    menu_tela = fase == F_RDIFIC && des_tem_run() ? MT_DESAFIO : MT_UM;
    menu_cur = 1;
    t_fase = 1.0f;
    fase = F_ABERTURA;
}

// O dado em foco nas telas da run (tecla I).
static int des_foco(void)
{
    if (fase == F_RLOJA && cursor >= 0 && cursor < N_LOJA) return run.loja[cursor].tipo;
    if (fase == F_RBOLSA && cursor >= 0 && cursor < run.n_col) return run.col[cursor].tipo;
    if (fase == F_REVENTO && ev_passo == 1 && cursor >= 0 && cursor < ev_n) return run.col[ev_lista[cursor]].tipo;
    if (fase == F_REVENTO && ev_passo == 2 && ev_mostra == 1) return ev_peca.tipo;
    return -1;
}
