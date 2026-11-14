// ---------------------------------------------------------------------------
// Colecao: o que esta aberto e o que esta fechado no Desafiante
// ---------------------------------------------------------------------------
// Cadeado: (x, y) e o meio do topo do corpo; e = escala (1 ou 2); o corpo na cor c.
static void cadeado(int x, int y, uint16_t c, int e) { icone_cadeado(x, y + e, e, c); }

// O que vem com o nivel: o chefe c (CH_*) o abre na dificuldade de baixo.
static void quem_abre(int nivel, int c, int marco, char *s, int n)
{
    if (marco == 0) snprintf(s, (size_t)n, "vença o Crupiê");
    else if (marco == 1) snprintf(s, (size_t)n, "vença a Dona do Salão");
    else if (nivel == 1) snprintf(s, (size_t)n, "vença %s", nome_chefe(c));
    else snprintf(s, (size_t)n, "vença %s no %s", nome_chefe(c), NOME_DIF_CURTO[nivel - 1]);
}

// A coluna de uma peca fechada que abre por conquista: o nome apagado, a
// medalha com "Conquista" em latao e o que fazer (a secreta, so "Conquista
// secreta" e a dica, se houver); nas que somam, quanto ja foi e a barra.
static void coluna_conquista(int x, const char *nome, int c)
{
    gfx_texto(x, 60, nome, C_TEXTO_M, 2, true);
    medalha(x + 5, 112, C_OURO);
    bool secreta = CONQ[c].secreta;
    txt(x + 16, 104, secreta ? "Conquista secreta" : "Conquista", C_OURO, true);
    const char *t = secreta ? CONQ[c].dica : CONQ[c].texto;
    int y = 130;
    if (t) {
        char s[200];
        snprintf(s, sizeof s, "%s%s", t, secreta ? "" : ".");
        y += quebra_linhas(x, y, 320, 4, s, C_MARFIM) * LINHA_FINA + 8;
    }
    if (CONQ[c].alvo && !secreta) {             // o contador: "9 de 15" e a barra fina
        int v = cq_conta(c), a = CONQ[c].alvo;
        if (v > a) v = a;
        char s[32];
        snprintf(s, sizeof s, "%d de %d", v, a);
        txt(x, y, s, C_LATAO, true);
        int bx = x + gfx_largura(s, fonte_de(true)) + 10, bw = 160, by = y + TXT_H / 2 - 1;
        gfx_rect(bx, by, bw, 3, C_FELTRO2);
        gfx_rect(bx, by, bw * v / a, 3, C_OURO);
    }
}
// A linha do item ja aberto por conquista: como ganhou, na fina, em uma ou
// duas linhas. Devolve quanto o resto da coluna desce (0 ou uma linha).
static int linha_conquista(int x, int c)
{
    char s[200];
    snprintf(s, sizeof s, "Conquista: %s.", CONQ[c].texto);
    int n = quebra_linhas(x, 120, 320, 2, s, C_VERDE);
    return n > 1 ? LINHA_FINA : 0;
}
// A linha do item aberto por conquista: como ganhou.
static void cq_linha_aberta(int c, char *s, int n) { snprintf(s, (size_t)n, "Conquista: %s", CONQ[c].texto); }

// 0: sempre livre; 1: ja aberto; 2: fechado (e o que falta em s).
static int cat_trava_dado(int t, char *s, int n)
{
    perfis_le();
    int c = cq_do_dado(t);
    if (c >= 0) {                                // o que abre por conquista: so por ela
        if (cq_aberta(c)) { cq_linha_aberta(c, s, n); return 1; }
        snprintf(s, (size_t)n, "%s", CONQ[c].texto);
        return 2;
    }
    int m = marco_do_dado(t), nv = nivel_do_dado(t);
    if (m < 0 && nv == 0) return 0;
    if (dado_liberado(t)) {
        if (perfil_atual >= 0) snprintf(s, (size_t)n, "Aberto no Desafiante (%s)", perfil()->nome);
        else snprintf(s, (size_t)n, "Aberto no Desafiante");
        return 1;
    }
    quem_abre(nv, chefe_do_dado(t), m, s, n);
    return 2;
}

// Os amuletos que saem no jogo, por raridade (comuns, incomuns, raros) e,
// dentro de cada uma, na ordem em que abrem.
static int n_amuletos_jogo(void)
{
    int n = 0;
    for (int a = 0; a < A_N; a++) n += amuleto_no_jogo(a);
    return n;
}
static int cat_amuleto(int i)
{
    for (int r = AMR_COMUM; r <= AMR_RARO; r++)
        for (int nv = 0; nv <= N_NIVEIS; nv++)
            for (int a = 0; a < A_N; a++)
                if (RAR_AMULETO[a] == r && NIVEL_AMULETO[a] == nv && i-- == 0) return a;
    return 0;
}

static void desenha_cat_amuleto(int i)
{
    perfis_le();
    int a = cat_amuleto(i);
    char s[96];
    faixas_colecao(i, n_amuletos_jogo());

    bool aberto = amuleto_liberado(a);
    int cq = cq_do_amuleto(a);
    // Na vitrine (catalogo.inc): flutuando na poca de luz; fechado, so o "?",
    // o nome e quem o abre (ou a conquista).
    int cx = VIT_X, sobe = vitrine_chao(cx, 40), cy = VIT_Y - sobe + ui_entra(t_fase, 0, UI_ENTRA_PLACA);
    int x = 290;
    if (!aberto) {
        vitrine_misterio(cx, cy, cq >= 0);
        fileira_colecao(FIL_AMULETO, i, n_amuletos_jogo(), &cat_a);
        if (cq >= 0) { coluna_conquista(x, AMULETO[a].nome, cq); return; }
        quem_abre(NIVEL_AMULETO[a], CH_DONA, -1, s, sizeof s);
        coluna_fechada(x, AMULETO[a].nome, s);
        return;
    }
    desenha_amuleto(cx, cy, 40, a, false);
    fileira_colecao(FIL_AMULETO, i, n_amuletos_jogo(), &cat_a);

    gfx_texto(x, 60, AMULETO[a].nome, AMULETO[a].cor, 2, true);
    int ex = etiqueta_caixa(x, 104, nome_rar_am(a), cor_rar_am(a)) + 2;
    static const char *const QUANDO[] = { "", "no começo da run", "depois do 1º chefe", "depois do 2º chefe" };
    snprintf(s, sizeof s, "amuleto · %s", QUANDO[RAR_AMULETO[a]]);
    txt(ex, 105, s, C_TEXTO_M, false);
    int dy = 0;                                  // a linha da conquista, em duas: o resto desce
    if (NIVEL_AMULETO[a] == 0) txt(x, 124, "Aberto desde o começo", C_TEXTO_M, false);
    else if (cq >= 0) dy = linha_conquista(x, cq);
    else {
        if (perfil_atual >= 0) snprintf(s, sizeof s, "Aberto no Desafiante (%s)", perfil()->nome);
        else snprintf(s, sizeof s, "Aberto no Desafiante");
        txt(x, 124, s, C_VERDE, false);
    }
    gfx_rect(x, 144 + dy, 320, 1, C_FELTRO2);
    quebra_cabe(x, 152 + dy, 320, CAT_TEXTO_H - dy, AMULETO[a].desc, C_MARFIM);
}

// Os adesivos, na ordem em que abrem.
static int cat_adesivo(int i)
{
    for (int nv = 0; nv <= N_NIVEIS; nv++)
        for (int a = AD_NADA + 1; a < AD_N; a++)
            if (ADESIVO[a].nivel == nv && !ad_aposentado(a) && i-- == 0) return a;
    return AD_CONFETE;
}

static void desenha_cat_adesivo(int i)
{
    perfis_le();
    int a = cat_adesivo(i);
    const adesivo_t *d = &ADESIVO[a];
    char s[96];
    faixas_colecao(i, N_AD_COLECAO);
    bool aberto = adesivo_aberto(a);             // a Sujeira vem com o Barro
    int cq = cq_do_adesivo(a);
    // Na vitrine (catalogo.inc): o adesivo flutua na luz, a ponta descolando
    // de tempos em tempos, e embaixo, no chao, tres dados com ele no lado de
    // cima; fechado, so o "?", o nome e quem o abre.
    int cx = VIT_X, sobe = vitrine_chao(cx, 50), cy = VIT_Y - sobe + ui_entra(t_fase, 0, UI_ENTRA_PLACA);
    int x = 290;
    if (!aberto) {
        vitrine_misterio(cx, cy - 22, cq >= 0);
        fileira_colecao(FIL_ADESIVO, i, N_AD_COLECAO, &cat_ad);
        if (cq >= 0) { coluna_conquista(x, d->nome, cq); return; }
        quem_abre(d->nivel, CH_CRUPIE, -1, s, sizeof s);
        coluna_fechada(x, d->nome, s);
        return;
    }
    float ciclo = fmodf(t_fase, 3.1f), descola = ciclo < 0.5f ? sinf(ciclo / 0.5f * 3.1416f) : 0;
    static const int T[3] = { D_D4, D_D6, D_D20 };
    for (int k = 0; k < 3; k++) {
        int v = k == 0 ? 3 : k == 1 ? 6 : 19;
        ad_face = a;
        desenha_dado((float)(cx - 44 + k * 44), (float)(VIT_CHAO - 10), 14, T[k], 0, 0, v + (a == AD_REMENDO ? 2 : 0), 2, false);
    }
    ad_solto_d(cx, cy - 22, a, 6, descola);
    fileira_colecao(FIL_ADESIVO, i, N_AD_COLECAO, &cat_ad);
    gfx_texto(x, 60, d->nome, d->luz, 2, true);
    if (!ad_a_venda(a)) {                        // a Sujeira: so pelo Barro do oponente
        int ex = etiqueta_caixa(x, 104, "não se vende", C_TEXTO_M) + 2;
        txt(ex, 105, "vem do Barro do oponente", C_TEXTO_M, false);
    } else {
        snprintf(s, sizeof s, "%d %s", d->preco, d->preco == 1 ? "moeda" : "moedas");
        int ex = etiqueta_caixa(x, 104, s, C_OURO) + 2;
        txt(ex, 105, "adesivo · na loja e no pacote de adesivos", C_TEXTO_M, false);
    }
    int dy = 0;                                  // a linha da conquista, em duas: o resto desce
    if (d->nivel == 0) txt(x, 124, "Aberto desde o começo", C_TEXTO_M, false);
    else if (cq >= 0) dy = linha_conquista(x, cq);
    else {
        if (perfil_atual >= 0) snprintf(s, sizeof s, "Aberto no Desafiante (%s)", perfil()->nome);
        else snprintf(s, sizeof s, "Aberto no Desafiante");
        txt(x, 124, s, C_VERDE, false);
    }
    gfx_rect(x, 144 + dy, 320, 1, C_FELTRO2);
    // O que o adesivo faz, curto, na fonte do texto; embaixo, menores e
    // apagados, os detalhes e os casos especiais e, se couber inteiro acima da
    // fileira, como os adesivos funcionam (a Sujeira nao: e o oponente que a cola).
    const int ATE = 268;                         // a fileira de baixo
    int y = 152 + dy + quebra_linhas(x, 152 + dy, 320, 4, AD_TEXTO[a], C_MARFIM) * LINHA_FINA + 5;
    if (AD_DETALHE[a] && ATE - y >= LINHA_MIUDA)
        y += quebra_linhas_em(x, y, 320, (ATE - y) / LINHA_MIUDA, AD_DETALHE[a], C_TEXTO_M, GFX_MIUDO, LINHA_MIUDA) * LINHA_MIUDA + 5;
    static const char *const COMO = "Vai num lado de um dado da sua bolsa e só age quando o dado termina a rodada com esse lado para cima.";
    quebra_so_conta = true;
    int nc = quebra_linhas_em(x, 0, 320, 9, COMO, 0, GFX_MIUDO, LINHA_MIUDA);
    quebra_so_conta = false;
    if (ad_a_venda(a) && y + nc * LINHA_MIUDA <= ATE) quebra_linhas_em(x, y, 320, nc, COMO, gfx_mistura(C_TEXTO_M, C_FELTRO, 50), GFX_MIUDO, LINHA_MIUDA);
}

// ---- A lista das conquistas (a quarta aba) -----------------------------------
// Todas as conquistas e, depois, as bolsas (que tambem abrem por um feito),
// numa lista em duas colunas; embaixo, a que esta em foco: como abre e, nas
// que somam, quanto falta. As setas passam a lista; a pagina vira sozinha.
#define LCQ_LIN   9                      // linhas por coluna
#define LCQ_POR   (2 * LCQ_LIN)          // por pagina
#define LCQ_Y0    (MESA_Y0 + 10)
#define LCQ_H     21
#define LCQ_W     292
static int n_cat_conquistas(void) { return CQ_N + B_N - 1; }   // o Saco de Pano nao abre: esta sempre la
static int lcq_bolsa(int i) { return i - CQ_N + 1; }            // i >= CQ_N: a bolsa (do Copo em diante)
static bool lcq_aberta(int i) { return i < CQ_N ? cq_aberta(i) : bolsa_aberta(lcq_bolsa(i)); }
static const char *lcq_nome(int i) { return i < CQ_N ? cq_nome(i) : BOLSA[lcq_bolsa(i)].nome; }
static const char *lcq_tipo(int i)
{
    static const char *const K[3] = { "dado", "amuleto", "adesivo" };
    return i < CQ_N ? K[CONQ[i].k] : "bolsa";
}
// O desenho pequeno: o item aberto; fechado, o "?" de latao (a bolsa, a ficha apagada).
static void lcq_icone(int i, int cx, int cy, bool aberta)
{
    if (i >= CQ_N) { ficha_bolsa(cx, cy, 8, lcq_bolsa(i), 0, 0, false, !aberta, 1, t_fase, -1); return; }
    if (!aberta) { txt_c(cx, cy - TXT_H / 2, "?", C_LATAO, true); return; }
    int it = CONQ[i].item;
    switch (CONQ[i].k) {
    case CQK_AMULETO: desenha_amuleto(cx, cy, 7, it, false); break;
    case CQK_ADESIVO: ad_solto_d(cx, cy, it, 1, 0); break;
    default: { peca_t pc = { .tipo = (uint8_t)it }; peca_parada((float)cx, (float)cy, 7, pc, 0); break; }
    }
}
static void desenha_cat_conquistas(int i)
{
    perfis_le();
    int n = n_cat_conquistas();
    if (i < 0 || i >= n) i = cat_cq = 0;
    faixas_colecao(i, n);
    int abertas = 0;
    for (int k = 0; k < n; k++) abertas += lcq_aberta(k);
    char s[200];
    int i0 = i / LCQ_POR * LCQ_POR;
    for (int k = i0; k < n && k < i0 + LCQ_POR; k++) {
        int col = (k - i0) / LCQ_LIN, lin = (k - i0) % LCQ_LIN;
        int x = GFX_W / 2 - LCQ_W - 6 + col * (LCQ_W + 12), y = LCQ_Y0 + lin * LCQ_H;
        bool ab = lcq_aberta(k), foco = k == i;
        int dy = ui_entra(t_fase, lin + col, UI_ENTRA_PLACA / 2);
        y += dy;
        gfx_rect(x, y, LCQ_W, LCQ_H - 3, foco ? gfx_mistura(C_LATAO_ESC, C_FELTRO_ESC, 60) : gfx_mistura(C_FELTRO_ESC, C_BARRA, 25));
        if (foco) ui_selecao(x - 1, y - 1, LCQ_W + 2, LCQ_H - 1, C_OURO);
        lcq_icone(k, x + 12, y + (LCQ_H - 3) / 2, ab);
        gfx_texto(x + 26, y + (LCQ_H - 3 - TXT_H) / 2, lcq_nome(k), ab ? C_MARFIM : C_TEXTO_M, 1, false);
        // A direita: aberta, quanto falta ou o tipo.
        bool conta = k < CQ_N && CONQ[k].alvo && !CONQ[k].secreta;
        if (ab) snprintf(s, sizeof s, k >= CQ_N ? "aberta" : "aberto");
        else if (conta) snprintf(s, sizeof s, "%d/%d", cq_conta(k) < CONQ[k].alvo ? cq_conta(k) : CONQ[k].alvo, CONQ[k].alvo);
        else snprintf(s, sizeof s, "%s", lcq_tipo(k));
        uint16_t cs = ab ? C_VERDE : conta ? C_LATAO : C_FELTRO2;
        gfx_texto(x + LCQ_W - 8 - gfx_largura(s, GFX_FINO), y + (LCQ_H - 3 - GFX_ALT_TEXTO(1)) / 2 + 1, s, cs, GFX_FINO, false);
        zona_clique(x, y, LCQ_W, LCQ_H - 2, &cat_cq, k, 0);
    }
    // Embaixo, a que esta em foco.
    int y = LCQ_Y0 + LCQ_LIN * LCQ_H + 6, x = GFX_W / 2 - LCQ_W - 6;
    gfx_rect(x, y - 3, 2 * LCQ_W + 12, 1, C_LATAO_ESC);
    bool ab = lcq_aberta(i);
    snprintf(s, sizeof s, "%s  ·  %s", lcq_nome(i), lcq_tipo(i));
    txt(x, y + 2, s, ab ? C_MARFIM : C_TEXTO_M, true);
    snprintf(s, sizeof s, "%d de %d abertos", abertas, n);   // os itens e as bolsas
    txt(x + 2 * LCQ_W + 12 - gfx_largura(s, fonte_de(false)), y + 2, s, C_LATAO, false);
    const char *como;
    char buf[200];
    if (i >= CQ_N) como = BOLSA[lcq_bolsa(i)].abre;
    else if (CONQ[i].secreta && !ab) como = CONQ[i].dica ? CONQ[i].dica : "Conquista secreta.";
    else { snprintf(buf, sizeof buf, "%s.", CONQ[i].texto); como = buf; }
    if (i < CQ_N && CONQ[i].secreta && !ab && CONQ[i].dica) { snprintf(buf, sizeof buf, "Conquista secreta: %s", CONQ[i].dica); como = buf; }
    quebra_linhas(x, y + 2 + LINHA_FINA + 2, 2 * LCQ_W + 12, 2, como, ab ? C_VERDE : C_MARFIM);
    int pags = (n + LCQ_POR - 1) / LCQ_POR, pag = i / LCQ_POR;   // a pagina: pontinhos no canto
    for (int k = 0; k < pags; k++) gfx_rect(GFX_W / 2 + (k - pags / 2) * 6, MESA_Y1 - 6, 3, 3, k == pag ? C_OURO : C_FELTRO2);
}

static int des_dificuldade(void) { return run.dificuldade; }

// O elevador do comeco da run: a placa de latao de perto (elevador.inc).
// Pula com qualquer tecla.
static void des_elevador(void)
{
    if (t_elev <= 0 || modo != M_DESAFIO) return;
    n_zonas = 0;                                 // nada por baixo responde ao clique
    elevador_placa(elev_dur(elev_d) - t_elev, elev_d);
}

// O chefe vira na 5a rodada: a mesa para um instante, uma faixa atravessa a
// tela, o retrato muda e ele diz a frase dele.
static void des_virou(void)
{
    perso_virado = fala_perso;
    t_virada = 2.4f;
    som_toca(SOM_ZERA);
    snprintf(fala_txt, sizeof fala_txt, "%s", virada_frase(fala_perso));
    fala_comeca(3.4f);
}

static void des_virada_anuncio(void)
{
    if (t_virada <= 0 || chefe_idx(perso_virado) < 0) return;
    int c = chefe_idx(perso_virado);
    float e = 2.4f - t_virada;
    n_zonas = 0;
    int alfa = e < 0.2f ? (int)(e / 0.2f * 140) : 140;
    ui_escurece(0, 0, GFX_W, GFX_H, alfa);
    static const uint16_t COR[3] = { RGB(46, 120, 84), RGB(150, 30, 46), RGB(150, 36, 40) };
    float q = e / 0.3f;
    if (q > 1) q = 1;
    int dx = -(int)((1 - (1 - q) * (1 - q)) * GFX_W) + GFX_W;   // entra pela esquerda
    int y = CENTRO - 30, h = 60;
    gfx_rect(-dx, y, GFX_W, h, gfx_mistura(COR[c], C_BARRA, 55));
    gfx_rect(-dx, y, GFX_W, 2, C_OURO);
    gfx_rect(-dx, y + h - 2, GFX_W, 2, C_OURO);
    if (q < 1) return;
    txt_sombra_c(GFX_W / 2, y + 6, virada_titulo(perso_virado), C_OURO, 2);
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
    case F_NOVIDADES: desenha_novidades(); return true;
    case F_RAMULETO: desenha_amuleto_run(); return true;
    case F_RAVISO: desenha_aviso();     return true;
    case F_PERFIS: desenha_perfis();    return true;
    case F_PCRIA:  desenha_cria_perfil(); return true;
    case F_RDIFIC: desenha_dificuldade(); return true;
    case F_RBOLSAS: desenha_bolsas();   return true;
    case F_HIST:   desenha_historico(); return true;
    case F_ESTILO: desenha_estilo();    return true;
    default:       return false;
    }
}

// ESC nas telas de perfil e dificuldade: um passo para tras.
static void des_esc(void)
{
    if (fase == F_PERFIS && pf_apagando) { pf_apagando = false; return; }
    if (fase == F_PCRIA || fase == F_HIST || fase == F_ESTILO) { t_fase = 0; fase = F_PERFIS; return; }
    if (fase == F_RBOLSAS && t_bs_clique >= 0) return;          // ja escolhida: a run vai comecar
    if (fase == F_RBOLSAS && perfil()->nivel > 0) { dif_cur = des_dif; t_fase = 0; fase = F_RDIFIC; return; }
    if (fase == F_RBOLSAS) { menu_tela = des_tem_run() ? MT_DESAFIO : MT_PRINCIPAL; menu_cur = menu_tela ? 1 : MN_UM; t_fase = 1.0f; fase = F_ABERTURA; return; }
    bool tem = fase == F_RDIFIC && des_tem_run();      // da dificuldade, de volta a continuar ou nova
    menu_tela = tem ? MT_DESAFIO : MT_PRINCIPAL;
    menu_cur = tem ? 1 : MN_UM;
    t_fase = 1.0f;
    fase = F_ABERTURA;
}

// O dado em foco nas telas da run (tecla I).
static int des_foco(void)
{
    if (fase == F_RLOJA && lj_pac) return t_pac >= PAC_PRONTO && !pacote_de_adesivos() && !lj_colando && cursor >= 0 && cursor < pac_n()
                                          ? pac_carta_p(cursor)->tipo : -1;
    if (fase == F_RLOJA && lj_tirando && cursor >= 0 && cursor < run.n_col) return run.col[cursor].tipo;
    if (fase == F_RLOJA && lj_colando == 1 && cursor >= 0 && cursor < run.n_col) return run.col[cursor].tipo;
    if (fase == F_RLOJA && lj_colando) return -1;
    if (fase == F_RLOJA && !lj_tirando && cursor >= 0 && cursor < n_vitrine()) return run.vitrine[cursor].tipo;
    if (fase == F_RBOLSA && cursor >= 0 && cursor < run.n_col) return run.col[cursor].tipo;
    return -1;
}
