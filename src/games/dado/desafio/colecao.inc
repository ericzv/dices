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

    int wn = gfx_texto(x, 60, AMULETO[a].nome, AMULETO[a].cor, 2, true);
    item_visto(VS_AMULETO, a);                           // na vitrine: visto
    if (item_novo(VS_AMULETO, a)) selo_novo_esq(x + wn + 10, 66);
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
            if (nivel_do_adesivo(a) == nv && !ad_aposentado(a) && i-- == 0) return a;
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
        quem_abre(nivel_do_adesivo(a), chefe_do_adesivo(a), -1, s, sizeof s);
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
    int wn = gfx_texto(x, 60, d->nome, d->luz, 2, true);
    item_visto(VS_ADESIVO, a);                           // na vitrine: visto
    if (item_novo(VS_ADESIVO, a)) selo_novo_esq(x + wn + 10, 66);
    if (!ad_a_venda(a)) {                        // a Sujeira: so pelo Barro do oponente
        int ex = etiqueta_caixa(x, 104, "não se vende", C_TEXTO_M) + 2;
        txt(ex, 105, "vem do Barro do oponente", C_TEXTO_M, false);
    } else {
        snprintf(s, sizeof s, "%d %s", d->preco, d->preco == 1 ? "moeda" : "moedas");
        int ex = etiqueta_caixa(x, 104, s, C_OURO) + 2;
        // Com a letra maior a linha passaria da borda: volta para a fina de sempre.
        const char *ond = "adesivo · na loja e no pacote de adesivos";
        bool cabe = ex + gfx_largura(ond, fonte_de(false)) <= x + 320;
        gfx_texto(ex, 105, ond, C_TEXTO_M, cabe ? fonte_de(false) : GFX_FINO_FIXO, false);
    }
    int dy = 0;                                  // a linha da conquista, em duas: o resto desce
    if (nivel_do_adesivo(a) == 0) txt(x, 124, "Aberto desde o começo", C_TEXTO_M, false);
    else if (cq >= 0) dy = linha_conquista(x, cq);
    else {
        if (perfil_atual >= 0) snprintf(s, sizeof s, "Aberto no Desafiante (%s)", perfil()->nome);
        else snprintf(s, sizeof s, "Aberto no Desafiante");
        txt(x, 124, s, C_VERDE, false);
    }
    gfx_rect(x, 144 + dy, 320, 1, C_FELTRO2);
    // Quando ele age (a etiqueta) e o que faz, curto, na fonte do texto;
    // embaixo, menores e apagados, os detalhes e os casos especiais.
    const int ATE = 268;                         // a fileira de baixo
    ad_quando_selo(x, 150 + dy, a);              // quando ele age
    dy += ad_quando_h(a);
    int y = 152 + dy + quebra_linhas(x, 152 + dy, 320, 4, AD_TEXTO[a], C_MARFIM) * LINHA_FINA + 5;
    if (AD_DETALHE[a] && ATE - y >= 10) {
        // Com a letra maior, o detalhe que nao cabe volta para a miuda de
        // sempre (como quebra_cabe): nao corta no meio da frase.
        int f = GFX_MIUDO, passo = LINHA_MIUDA;
        quebra_so_conta = true;
        int nl = quebra_linhas_em(x, y, 320, 99, AD_DETALHE[a], 0, f, passo);
        quebra_so_conta = false;
        if (nl * passo > ATE - y) { f = GFX_MIUDO_FIXO; passo = 10; }
        quebra_linhas_em(x, y, 320, (ATE - y) / passo, AD_DETALHE[a], C_TEXTO_M, f, passo);
    }
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
// tela com o retrato dele grande - calmo; no meio do anuncio ele muda de cara,
// com um tremor e um clarao na cor do salao - e ele diz a frase dele.
static void des_virou(void)
{
    perso_virado = fala_perso;
    t_virada = VIRADA_DUR;
    som_toca(SOM_ZERA);
    snprintf(fala_txt, sizeof fala_txt, "%s", virada_frase(fala_perso));
    fala_comeca(3.4f);
}

static void des_virada_anuncio(void)
{
    if (t_virada <= 0 || chefe_idx(perso_virado) < 0) return;
    int c = chefe_idx(perso_virado);
    float e = VIRADA_DUR - t_virada, ev = e - VIRADA_TV;        // ev >= 0: ja virou
    n_zonas = 0;
    bool branco = !tremor_off && ev >= 0 && ev < 0.07f;         // o clarao branco (quem desligou o tremor e o clarao, tecla T, nao o ve)
    int alfa = e < 0.2f ? (int)(e / 0.2f * 140) : 140;
    ui_escurece(0, 0, GFX_W, GFX_H, alfa);
    static const uint16_t COR[3] = { RGB(46, 120, 84), RGB(150, 30, 46), RGB(150, 36, 40) };
    static const uint16_t CLARAO[3] = { C_LASER, C_VINHO_CLR, C_FOGO };
    float q = e / 0.3f;
    if (q > 1) q = 1;
    int dx = -(int)((1 - (1 - q) * (1 - q)) * GFX_W) + GFX_W;   // entra pela esquerda
    int h = 72, y = CENTRO - h / 2;
    // A faixa: escura e calma antes; no instante da virada, um clarao na cor
    // do salao, e depois ela fica acesa.
    uint16_t fundo = gfx_mistura(COR[c], C_BARRA, ev < 0 ? 70 : 35), filete = C_OURO;
    if (branco) { fundo = C_BRANCO; filete = C_BRANCO; }
    else if (ev >= 0.07f && ev < 0.2f) fundo = CLARAO[c];
    gfx_rect(-dx, y, GFX_W, h, fundo);
    if (ev >= 0.2f)                                             // a luz vem do retrato: pontilhado que some para a direita
        for (int j = 2; j < h - 2; j++)
            for (int i = 0; i < 230; i++)
                if (bayer(i, j) < 0.55f * (1 - i / 230.0f)) gfx_pixel(i - dx, y + j, COR[c]);
    gfx_rect(-dx, y, GFX_W, 2, filete);
    gfx_rect(-dx, y + h - 2, GFX_W, 2, filete);
    if (ev >= 0 && ev < 0.45f) {                                // os riscos do clarao, abrindo para cima e para baixo
        int a = (int)(ev / 0.45f * 16);
        uint16_t rc = ev < 0.2f && !tremor_off ? C_BRANCO : CLARAO[c];
        gfx_rect(0, y - 3 - a, GFX_W, ev < 0.2f ? 2 : 1, rc);
        gfx_rect(0, y + h + 1 + a, GFX_W, ev < 0.2f ? 2 : 1, rc);
    }
    // O retrato do chefe, grande (4x), na moldura de ouro que passa da faixa:
    // calmo; treme um pouco antes; no instante, sacode e muda de cara (o
    // tremor fica parado para quem o desligou, tecla T).
    int esc = 4, w = RT_W * esc, hh = RT_H * esc, px = 40 - dx, py = CENTRO - hh / 2 - 2;
    if (!tremor_off && ev > -0.4f && ev < 0) px += ((int)(e * 60) & 2) ? 1 : 0;                         // a espera
    if (!tremor_off && ev >= 0 && ev < 0.6f) px += (int)lroundf(6 * expf(-6 * ev) * sinf(48 * ev));      // a sacudida
    uint16_t r[4];
    ui_rampa(ev >= 0.07f ? C_BRASA : C_OURO, false, r);
    if (branco) r[0] = r[1] = r[2] = r[3] = C_BRANCO;
    ui_sombra(px - 5, py - 5, w + 10, hh + 10);
    ui_contorno(px - 5, py - 5, w + 10, hh + 10, C_BARRA);
    ui_aro(px - 4, py - 4, w + 8, hh + 8, 3, r, UI_VIVO);
    ui_contorno(px - 1, py - 1, w + 2, hh + 2, C_BARRA);
    gfx_rect(px, py, w, hh, ev >= 0.07f ? gfx_mistura(COR[c], C_BARRA, 60) : gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(px, py, esc, perso_virado, t_fase + perso_virado * 1.3f, false, 0, RT_H);
    if (branco)
        for (int j = 0; j < hh; j++) for (int i = 0; i < w; i++) if (bayer(px + i, py + j) < 0.7f) gfx_pixel(px + i, py + j, C_BRANCO);
    if (q < 1) return;
    int cx = (40 + w + 8 + GFX_W) / 2, larg = GFX_W - (40 + w + 8) - 24;
    const char *tit = virada_titulo(perso_virado);
    txt_sombra_c(cx, y + 9, tit, ev >= 0 && ev < 0.2f ? C_BARRA : C_OURO, gfx_largura(tit, 2) <= larg ? 2 : 1);
    if (ev < 0.12f) return;                                     // a regra entra com a virada
    char s[96];
    virada_txt(c, virada_forca(c), false, s, sizeof s);
    txt_c_duas(cx, y + 43, s, C_MARFIM, larg);
}

// Sentar a mesa da vez, no mapa: a partida comeca e o retrato do oponente
// voa do lugar dele no salao para o canto de cima da mesa. O chefe tem antes
// a apresentacao: a mesa escura, o retrato grande entrando pelo lado na
// moldura de ouro, o nome na cor do salao, o trunfo e a fala de abertura.
static void des_senta(void)
{
    int p = run.passo, esc = salao_esc(p);
    bool chefe = salao_chefe(p);
    entrada_x = salao_x(p) - RT_W * esc / 2; entrada_y = salao_topo(p); entrada_esc = esc;
    des_comeca_partida();
    t_entrada = entrada_dur = chefe ? ENTRADA_CHEFE : ENTRADA_VOO;
}
static void des_entrada(void)
{
    if (t_entrada <= 0 || modo != M_DESAFIO || fala_perso < 0) return;
    n_zonas = 0;
    bool chefe = entrada_dur > ENTRADA_VOO;
    float e = entrada_dur - t_entrada, v = (e - (entrada_dur - ENTRADA_VOO)) / ENTRADA_VOO;
    if (v < 0) v = 0;
    float s = v * v * (3 - 2 * v);                              // o voo para o canto
    int ax = entrada_x, ay = entrada_y, aesc = entrada_esc;
    if (chefe) {
        float q = e / 0.25f;
        if (q > 1) q = 1;
        aesc = 4;
        ax = 40 - (int)((1 - q) * (1 - q) * (40 + RT_W * aesc + 12));   // entra pela esquerda
        ay = CENTRO - RT_H * aesc / 2 - 14;
        ui_escurece(0, 0, GFX_W, GFX_H, (int)(225 * q * (1 - s)));   // (quase tudo: o nome nao briga com a mesa)
    }
    int bx = retrato_faixa_x[1], by = 5;                        // o lugar dele na faixa (retrato_faixa)
    int esc = aesc - (int)lroundf((aesc - 2) * s), w = RT_W * esc, h = RT_H * esc;
    // (o meio do retrato vai em linha reta; o tamanho muda em volta dele)
    int cx = ax + RT_W * aesc / 2 + (int)lroundf((bx + RT_W - ax - RT_W * aesc / 2) * s);
    int cy = ay + RT_H * aesc / 2 + (int)lroundf((by + RT_H - ay - RT_H * aesc / 2) * s);
    int px = cx - w / 2, py = cy - h / 2, m = esc >= 4 ? 3 : 2;
    uint16_t r[4];
    ui_rampa(perso_chefe(fala_perso) || chefe ? C_OURO : C_LATAO, false, r);
    ui_sombra(px - m - 2, py - m - 2, w + 2 * m + 4, h + 2 * m + 4);
    ui_contorno(px - m - 2, py - m - 2, w + 2 * m + 4, h + 2 * m + 4, C_BARRA);
    ui_aro(px - m - 1, py - m - 1, w + 2 * m + 2, h + 2 * m + 2, m, r, UI_VIVO);
    ui_contorno(px - 1, py - 1, w + 2, h + 2, C_BARRA);
    gfx_rect(px, py, w, h, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(px, py, esc, fala_perso, t_relogio + fala_perso * 1.3f, false, 0, RT_H);
    if (!chefe || e < 0.25f || v > 0) return;                   // o nome, o trunfo e a fala: com o retrato parado
    const perso_t *pp = &PERSO[fala_perso];
    int tx0 = ax + w + 22, larg = GFX_W - tx0 - 24, tcx = tx0 + larg / 2, ty = ay + 10;
    uint16_t cs = COR_SALAO[run.andar <= DES_ANDARES ? run.andar : 0];
    txt_sombra_c(tcx, ty, pp->nome, cs, gfx_largura(pp->nome, 2) <= larg ? 2 : 1);
    const char *sl = SALAO[run.andar <= DES_ANDARES ? run.andar : 0];
    gfx_texto(tcx - gfx_largura(sl, GFX_FINO) / 2, ty + 28, sl, C_MARFIM_S, GFX_FINO, false);
    if (pp->trunfo && trunfo_ativo(fala_perso)) {
        gfx_texto(tcx - gfx_largura(pp->trunfo, 1) / 2, ty + 50, pp->trunfo, C_OURO, 1, false);
        txt_c_duas(tcx, ty + 66, regra_perso(fala_perso), C_MARFIM, larg);
    }
    if (fala_txt[0]) {                                          // a fala de abertura, inteira, embaixo do retrato
        balao_x_min = px; balao_x_max = GFX_W - 24;
        balao_em(px + w / 2, py + h + m + 4, fala_txt, fala_txt, 1);
        balao_x_max = 0;
    }
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
    if (fase == F_RDIFIC && dif_pop) { dif_pop = false; return; }        // o pop-up dos desbloqueios fecha
    if (fase == F_RDIFIC && dif_escolhe) { dif_cur = dif_antes; dif_escolhe = false; return; }   // e o "Trocar de andar" desiste
    if (fase == F_PCRIA || fase == F_HIST || fase == F_ESTILO) { t_fase = 0; fase = F_PERFIS; return; }
    if (fase == F_RBOLSAS && t_bs_clique >= 0) return;          // ja escolhida: a run vai comecar
    if (fase == F_RBOLSAS && perfil()->nivel > 0) { dif_cur = des_dif; dif_pop = dif_escolhe = false; dif_foco = DIF_INICIAR; t_fase = 0; fase = F_RDIFIC; return; }
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
    if (fase == F_NOVIDADES) {                   // o card que se ve no meio, se for um dado
        int s = nov_parado() ? nov_sel : n_nov == 1 && nov_pronto() ? 0 : -1;
        return s >= 0 && s < n_nov && nov[s].k == NV_DADO ? nov[s].v : -1;
    }
    return -1;
}
