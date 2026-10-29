// ===========================================================================
// Os dados da casa (colecao) e o detalhe de um dado
// ===========================================================================
// A moeda girando devagar na vitrine, sem nunca ficar de perfil (um palito):
// estreita ate um terco e vira a outra face de uma vez, como quem a gira na
// mao.
static void moeda_girando(int cx, int cy, int tipo, float t)
{
    float c = cosf(t * 1.4f), e = 0.34f + 0.66f * fabsf(c);
    moeda_girando_arte((float)cx, (float)cy, 44, tipo, 0, c >= 0 ? e : -e, c >= 0 ? 1 : 2);
}

// Faces que o dado pode mostrar, na ordem em que o catalogo as percorre.
static int faces_de(int tipo, int *f)
{
    int n = 0, lados = TIPO[tipo].lados;
    if (TIPO[tipo].efeito == EF_SORTE_REVES) {          // de -7 a 11, de 2 em 2
        for (int v = -7; v <= 11; v += 2) f[n++] = v;
        return n;
    }
    int de = TIPO[tipo].efeito == EF_LASTRO ? 4 : 1;
    if (lados > 24) {                                   // o d100: o 1 e de 5 em 5 (cabe em f[24])
        f[n++] = 1;
        for (int v = 5; v <= lados && n < 24; v += 5) f[n++] = v;
        return n;
    }
    for (int v = de; v <= lados; v++) f[n++] = v;
    return n;
}

// Quebra o texto em linhas de ate max_w pixels, sempre nos espacos.
#define LINHA_FINA 14                     // passo das linhas do texto corrido (fonte fina)
static bool quebra_so_conta;                     // so mede: quantas linhas daria
static int quebra_linhas(int x, int y, int max_w, int max_lin, const char *txt_, uint16_t c)
{
    char linha[200];
    int lin = 0;
    const char *p = txt_;
    while (*p && lin < max_lin) {
        int n = 0, cabe = 0;
        // Vai somando palavras enquanto a linha couber.
        while (p[n]) {
            int fim = n;
            while (p[fim] == ' ') fim++;
            while (p[fim] && p[fim] != ' ') fim++;
            if (fim >= (int)sizeof linha) break;
            memcpy(linha, p, (size_t)fim);
            linha[fim] = 0;
            if (gfx_largura(linha, GFX_FINO) > max_w && cabe) break;
            cabe = n = fim;
            if (!p[n]) break;
        }
        if (!cabe) cabe = (int)strlen(p) < (int)sizeof linha - 1 ? (int)strlen(p) : (int)sizeof linha - 1;
        memcpy(linha, p, (size_t)cabe);
        linha[cabe] = 0;
        if (!quebra_so_conta) txt(x, y + lin * LINHA_FINA, linha, c, false);
        lin++;
        p += cabe;
        while (*p == ' ') p++;
    }
    return lin;
}

// Etiquetas ao lado do tipo e da raridade; as de protecao com o icone dela
// na frente ("não anulável", "não roubável"). Devolve onde a proxima coisa
// pode comecar.
static int etiqueta_icone(int x, int y, const char *s, uint16_t c, int icone)
{
    int iw = icone ? 10 : 0, w = gfx_largura(s, GFX_FINO) + 10 + iw;
    ui_etiqueta(x, y + 2, w, 16, c);
    if (icone) icone_protecao(x + 5, y + 6, icone == 1, icone == 2);
    gfx_texto(x + 5 + iw, y - 1, s, c, GFX_FINO, false);
    return x + w + 4;
}
static int etiqueta_caixa(int x, int y, const char *s, uint16_t c) { return etiqueta_icone(x, y, s, c, 0); }
static int etiquetas_protecao(int x, int y, int tipo)
{
    if (nao_anulavel_t(tipo)) x = etiqueta_icone(x, y, "não anulável", C_ACO, 1);
    if (nao_roubavel_t(tipo)) x = etiqueta_icone(x, y, "não roubável", C_OURO, 2);
    return x;
}

// Colecao em tres secoes, Dados, Amuletos e Adesivos, trocadas pelas abas no alto.
// O que ainda esta fechado no Desafiante (para o perfil em uso) fica no
// escuro, com cadeado e o que falta para abrir.
static int  cat_trava_dado(int t, char *s, int n);       // desafio.inc: 0 sempre livre, 1 aberto, 2 fechado
static void desenha_cat_amuleto(int i);
static void desenha_cat_adesivo(int i);
static void cadeado(int x, int y, uint16_t c, int e);

// As faixas da colecao: a chave Dados | Amuletos, a contagem e as teclas.
static void faixas_colecao(int atual, int total)
{
    faixa_madeira(0);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(botao_voltar(KEY_ESC), ty, "COLEÇÃO", C_LATAO, true);
    static const char *const SEC[3] = { "Dados", "Amuletos", "Adesivos" };
    const int w = 96, h = 26, x0 = GFX_W / 2 - w * 3 / 2, y0 = (FAIXA_H - h) / 2 - 1;
    for (int i = 0; i < 3; i++) {                 // tres abas, a escolhida em latao
        ui_botao(x0 + i * w, y0, w, h, SEC[i], cat_sec == i ? C_LATAO : 0, 0);
        zona_clique(x0 + i * w, y0, w, h, NULL, 0, '1' + i);
    }
    char s[32];
    snprintf(s, sizeof s, "%d de %d", atual + 1, total);
    txt(GFX_W - 18 - gfx_largura(s, 1), ty, s, C_TEXTO_M, false);
    int yb = GFX_H - FAIXA_H;
    faixa_madeira(yb);
    teclas_c(GFX_W / 2, yb + (FAIXA_H - TECLA_H) / 2 + 1, cat_sec == 0 ? "setas trocam o dado  ·  TAB troca a seção  ·  ENTER volta"
             : cat_sec == 1 ? "setas trocam o amuleto  ·  TAB troca a seção  ·  ENTER volta" : "setas trocam o adesivo  ·  TAB troca a seção  ·  ENTER volta", C_TEXTO_M);
}

// ---- A vitrine e a fileira ------------------------------------------------
// A vitrine: a peca flutua sobre uma poca de luz no feltro, com a sombra
// embaixo encolhendo quando ela sobe. A luz e a sombra mexem no proprio
// feltro: cada pixel sobe (ou desce) um ou dois tons na rampa dele, mais no
// meio, e o pontilhado decide a passagem - a textura continua por baixo.
#define VIT_X   160
#define VIT_Y   (CENTRO - 22)
#define VIT_CHAO (VIT_Y + 56)                   // onde fica a poca de luz
static int feltro_idx(uint16_t c)
{
    for (int i = 0; i < 6; i++) if (feltro_tons[i] == c) return i;
    return -1;
}
static void luz_no_feltro(int cx, int cy, int rx, int ry, float tons)
{
    uint16_t *fb = display_fb();
    for (int y = cy - ry; y <= cy + ry; y++) {
        if (y < MESA_Y0 || y >= MESA_Y1) continue;
        for (int x = cx - rx; x <= cx + rx; x++) {
            if (x < MESA_X0 || x >= MESA_X1) continue;
            float dx = (x - cx) / (float)rx, dy = (y - cy) / (float)ry, d = dx * dx + dy * dy;
            if (d >= 1) continue;
            float q = (1 - d) * fabsf(tons);
            int k = (int)q;
            if (bayer(x, y) < q - k) k++;
            if (!k) continue;
            uint16_t *p = &fb[y * GFX_W + x];
            int i = feltro_idx(*p);
            if (i < 0) continue;
            int j = tons > 0 ? i + k : i - k;
            *p = feltro_tons[j < 0 ? 0 : j > 5 ? 5 : j];
        }
    }
}
// A luz e a sombra da vitrine; devolve quanto a peca sobe agora (o vaivem).
static int vitrine_chao(int cx, int r)
{
    float sobe = 3 + 3 * sinf(t_relogio * 1.8f);
    luz_no_feltro(cx, VIT_CHAO, 96, 26, 2.2f);
    int sr = (int)(r * (0.95f - sobe * 0.03f));
    luz_no_feltro(cx, VIT_CHAO, sr, sr / 4 + 2, -2.4f);
    return (int)lroundf(sobe);
}
// Fechada: a peca no escuro (so os pixels que ela pintou, comparados com o
// feltro guardado antes) e o cadeado.
#define VIT_R 104
static uint16_t vit_fundo[(2 * VIT_R) * (2 * VIT_R)];
static void vitrine_guarda(int cx, int cy)
{
    const uint16_t *fb = display_fb();
    for (int y = 0; y < 2 * VIT_R; y++)
        for (int x = 0; x < 2 * VIT_R; x++) {
            int X = cx - VIT_R + x, Y = cy - VIT_R + y;
            vit_fundo[y * 2 * VIT_R + x] = X >= 0 && Y >= 0 && X < GFX_W && Y < GFX_H ? fb[Y * GFX_W + X] : 0;
        }
}
static void vitrine_no_escuro(int cx, int cy)
{
    uint16_t *fb = display_fb();
    for (int y = 0; y < 2 * VIT_R; y++)
        for (int x = 0; x < 2 * VIT_R; x++) {
            int X = cx - VIT_R + x, Y = cy - VIT_R + y;
            if (X < 0 || Y < 0 || X >= GFX_W || Y >= GFX_H) continue;
            uint16_t *p = &fb[Y * GFX_W + X];
            if (*p != vit_fundo[y * 2 * VIT_R + x]) *p = gfx_mistura(*p, C_SOMBRA, 62);
        }
    cadeado(cx + 46, cy - 58, C_VINHO_CLR, 2);
}

// A fileira embaixo: a pagina de pecas em que esta a da vitrine, a dela com
// as cantoneiras. As fechadas sao so a silhueta, no escuro. O mouse em cima
// troca a vitrine; a pagina so vira pelas setas (ou na ponta dela).
#define FIL_N     15
#define FIL_PASSO 38
#define FIL_Y     (MESA_Y1 - 24)
enum { FIL_DADO, FIL_AMULETO, FIL_ADESIVO };
static void silhueta_dado(float cx, float cy, float r, int lados, uint16_t cor, float dens);
static int cat_amuleto(int i);               // desafio/colecao.inc
static int cat_adesivo(int i);
static bool amuleto_liberado(int a);
static bool adesivo_liberado(int a);
static void fileira_colecao(int sec, int atual, int total, int *var)
{
    int i0 = atual / FIL_N * FIL_N, n = total - i0 < FIL_N ? total - i0 : FIL_N;
    int x0 = GFX_W / 2 - (FIL_N - 1) * FIL_PASSO / 2;
    gfx_rect(x0 - 24, FIL_Y + 16, (FIL_N - 1) * FIL_PASSO + 48, 1, C_LATAO_ESC);   // a prateleira
    for (int k = 0; k < n; k++) {
        int i = i0 + k, x = x0 + k * FIL_PASSO, y = FIL_Y - (i == atual ? 2 : 0);
        if (i == atual) ui_selecao(x - 17, y - 17, 34, 34, C_OURO);
        bool fechado;
        if (sec == FIL_DADO) {
            char tr[80];
            int t = cat_tipo(i);
            fechado = cat_trava_dado(t, tr, sizeof tr) == 2;
            int lados = TIPO[t].lados;
            if (fechado) silhueta_dado((float)x, (float)y, 11, lados == 2 ? 100 : lados, C_SOMBRA, 0.85f);
            else { peca_t pc = { .tipo = (uint8_t)t }; peca_parada((float)x, (float)y, 11, pc, 0); }
        } else if (sec == FIL_AMULETO) {
            int a = cat_amuleto(i);
            fechado = !amuleto_liberado(a);
            if (fechado) silhueta_dado((float)x, (float)y, 10, 100, C_SOMBRA, 0.85f);
            else desenha_amuleto(x, y, 10, a, false);
        } else {
            int a = cat_adesivo(i);
            fechado = !adesivo_liberado(a);
            if (fechado) silhueta_dado((float)x, (float)y, 9, 6, C_SOMBRA, 0.85f);
            else ad_solto_d(x, y, a, 1, 0);
        }
        zona_clique(x - FIL_PASSO / 2, FIL_Y - 18, FIL_PASSO, 36, var, i, 0);
    }
    int pags = (total + FIL_N - 1) / FIL_N, pag = atual / FIL_N;
    if (pags > 1) {                                   // a pagina: pontinhos na ponta da prateleira
        int px = x0 + (FIL_N - 1) * FIL_PASSO + 30, py = FIL_Y + 20;
        for (int k = 0; k < pags; k++) gfx_rect(px - (pags - k) * 5, py, 3, 3, k == pag ? C_OURO : C_FELTRO2);
    }
}

// "Bloqueado" e, ao lado e miudo, o que falta para abrir.
static void linha_bloqueio(int x, int y, const char *falta)
{
    cadeado(x + 4, y + 8, C_VINHO_CLR, 1);
    int xb = x + 14;
    txt(xb, y, "Bloqueado", C_VINHO_CLR, true);
    gfx_texto(xb + gfx_largura("Bloqueado", 1) + 8, y, falta, C_TEXTO_M, 1, false);
}

static void desenha_catalogo(void)
{
    if (cat_sec == 1) { desenha_cat_amuleto(cat_a); return; }
    if (cat_sec == 2) { desenha_cat_adesivo(cat_ad); return; }
    if (cat_i >= n_catalogo()) cat_i = 0;              // o Natal acabou com a colecao aberta num deles
    int tipo = cat_tipo(cat_i);
    const tipo_t *d = &TIPO[tipo];
    char s[32];
    faixas_colecao(cat_i, n_catalogo());
    char trava[80];
    int st = cat_trava_dado(tipo, trava, sizeof trava);

    // A peca, a esquerda, flutuando na luz e passando pelas faces, com o
    // poder animado como na mesa (o brilho girando e o desenho dele).
    int cx = VIT_X, sobe = vitrine_chao(cx, 46), cy = VIT_Y - sobe + ui_entra(t_fase, 0, UI_ENTRA_PLACA);
    if (st == 2) vitrine_guarda(cx, cy);
    if (tipo == D_TUDO_NADA) {                          // o yin-yang, girando devagar
        so_apagado = true;                              // fechado: apagado
        tudo_ou_nada((float)cx, (float)cy, 46, t_fase * 0.8f, -1, st == 2);
        so_apagado = false;
    } else if (d->lados == 2) {
        moeda_girando(cx, cy, tipo, t_fase);
    } else {
        int f[24], nf = faces_de(tipo, f);
        int i = (int)(t_fase / 0.9f) % nf;
        float dentro = fmodf(t_fase, 0.9f);
        float r = 46 + (dentro < 0.08f ? 3 : 0);          // pulinho a cada face
        float ang = sinf(t_fase * 0.9f) * 0.3f;           // balanca devagar
        if (d->efeito && st != 2) brilho_poder((float)cx, (float)cy, r, tipo, ang, true);
        so_apagado = true;
        desenha_dado((float)cx, (float)cy, r, tipo, 0, ang, f[i], 2, st == 2);
        so_apagado = false;
        if (d->efeito && st != 2) anima_poder(cx, cy, (int)r, tipo, 0);
        if (tipo == D_TREVO && f[i] == 4) {
            gfx_rect(cx - 30, cy + 58, 60, TXT_H + 4, C_BARRA);
            txt_c(cx, cy + 60, "SORTE", C_OURO, true);
        }
    }
    if (st == 2) vitrine_no_escuro(cx, cy);             // fechado: no escuro, com o cadeado
    fileira_colecao(FIL_DADO, cat_i, n_catalogo(), &cat_i);

    // A direita: nome, lados, raridade e o que ele faz.
    int x = 290;
    gfx_texto(x, 60, d->nome, st == 2 ? C_TEXTO_M : cor_dado(tipo), 2, true);
    classe(tipo, s, sizeof s);
    int xt = etiquetas_protecao(etiqueta_caixa(x, 104, s, cor_rar(tipo)), 104, tipo);
    if (so_inicial(tipo)) txt(xt + 2, 105, "dado inicial", C_TEXTO_M, false);
    if (so_desafio(tipo)) txt(xt + 2, 105, "só no Desafiante", C_TEXTO_M, false);
    if (de_natal(tipo)) txt(xt + 2, 105, "de Natal: de 20 de outubro a 6 de janeiro", C_TEXTO_M, false);
    if (tipo == D_TSUNAMI) txt(xt + 2, 105, "nasce da Onda", C_TEXTO_M, false);
    if (st == 2) {
        linha_bloqueio(x, 124, trava);
    } else if (st == 1) {
        txt(x, 124, trava, C_VERDE, false);
    }
    gfx_rect(x, 144, 320, 1, C_FELTRO2);
    quebra_linhas(x, 152, 320, 6, TEXTO_CAT[tipo], C_MARFIM);
}

// O dado em foco na partida: o do cursor na mao, na loja ou no premio, ou o
// proximo da fila. -1 se nao ha nenhum.
static int tipo_em_foco(void)
{
    switch (fase) {
    case F_ORDEM:
        if (J[vez].n_mao && cursor >= 0 && cursor < J[vez].n_mao) return J[vez].col[J[vez].mao[cursor]].tipo;
        break;
    case F_JOGA:
        if (F[vez].lancados < F[vez].n) return tipo_de(vez, F[vez].lancados);
        break;
    case F_LOJA:
        if (cursor >= 0 && cursor < N_LOJA) return loja[cursor].tipo;
        break;
    case F_PREMIO:
        if (!removendo && !premio_colando() && cursor >= 0 && cursor < 3 && !oferta_ad[cursor]) return oferta[cursor].tipo;
        break;
    default:
        return des_foco();
    }
    return -1;
}

// Tecla I: o texto inteiro do dado em foco, por cima da mesa. So olha: a
// partida fica parada onde estava, e qualquer tecla fecha.
static void desenha_detalhe(void)
{
    if (detalhe < 0) return;
    if (fase != detalhe_fase) { detalhe = -1; return; }   // a partida andou
    int tipo = detalhe;
    const int x0 = 70, y0 = 70 + ui_entra(t_detalhe, 0, UI_ENTRA_CARTAO / 2), w = GFX_W - 140, h = 220;
    gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_SOMBRA, 140);
    ui_cartao_aceso(x0, y0, w, h, true, cor_rar(tipo));
    int cx = x0 + 78, cy = y0 + 96;
    for (int y = -7; y <= 7; y++)                                    // a sombra no cartao, pontilhada
        for (int x = -40; x <= 40; x++) {
            float d = (x * x) / 1600.0f + (y * y) / 49.0f;
            if (d < 1 && bayer(cx + x, cy + 40 + y) < (1 - d) * 1.4f) gfx_pixel(cx + x, cy + 40 + y, C_BARRA);
        }
    peca_t pc = { .tipo = (uint8_t)tipo, .fixo = 0 };               // nova_peca sortearia: aqui so se olha
    peca_parada((float)cx, (float)cy, 32, pc, 0);
    int x = x0 + 160;
    gfx_texto(x, y0 + 16, TIPO[tipo].nome, cor_dado(tipo), 1, true);
    char s[32];
    classe(tipo, s, sizeof s);
    int wn = gfx_largura(TIPO[tipo].nome, 1), wc = gfx_largura(s, 1) + 10;
    ui_etiqueta(x + wn + 12, y0 + 18, wc, TXT_H - 2, cor_rar(tipo));
    gfx_texto(x + wn + 17, y0 + 16, s, cor_rar(tipo), 1, false);
    etiquetas_protecao(x + wn + 12 + wc + 6, y0 + 17, tipo);
    gfx_rect(x, y0 + 44, w - 180, 1, C_FELTRO2);
    quebra_linhas(x, y0 + 54, w - 180, 6, TEXTO_CAT[tipo], C_MARFIM);
    txt_c(GFX_W / 2, y0 + h - 28, "qualquer tecla fecha", C_TEXTO_M, false);
}
