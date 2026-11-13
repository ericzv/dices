// ===========================================================================
// Os dados da casa (colecao) e o detalhe de um dado
// ===========================================================================
// A moeda girando devagar na vitrine, sem nunca ficar de perfil (um palito):
// estreita ate um terco e vira a outra face de uma vez, como quem a gira na
// mao.
static void moeda_girando(int cx, int cy, int tipo, float t)
{
    float c = cosf(t * 1.4f), e = 0.34f + 0.66f * fabsf(c);
    face_curinga = tipo == D_FICHA_CORINGA && c >= 0;   // a Ficha Coringa: o curinga de um lado, o numero que virou do outro
    moeda_girando_arte((float)cx, (float)cy, 44, tipo, 0, c >= 0 ? e : -e, tipo == D_FICHA_CORINGA ? 7 : c >= 0 ? 1 : 2);
    face_curinga = false;
}

// Faces que o dado pode mostrar, na ordem em que o catalogo as percorre.
static int faces_de(int tipo, int *f)
{
    int n = 0, lados = TIPO[tipo].lados;
    if (tipo == D_FANTASMA) { f[0] = 1; return 1; }     // o Fantasma nao e sorteado: e sempre o 1
    if (TIPO[tipo].efeito == EF_SORTE_REVES) {          // de 1 - SR_REVES a 11, de 2 em 2
        for (int v = 1 - SR_REVES; v <= 11; v += 2) f[n++] = v;
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

// Quebra o texto em linhas de ate max_w pixels, sempre nos espacos, na
// fonte f com as linhas a cada passo pixels (quebra_linhas: a fonte fina).
#define LINHA_FINA (gfx_letra_e_maior() ? 16 : 14)   // passo das linhas do texto corrido (fonte fina)
#define LINHA_MIUDA (gfx_letra_e_maior() ? 14 : 10)  // e da miuda (com a letra maior, ela vira a fina)
static bool quebra_so_conta;                     // so mede: quantas linhas daria
static int quebra_linhas_em(int x, int y, int max_w, int max_lin, const char *txt_, uint16_t c, int f, int passo)
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
            if (gfx_largura(linha, f) > max_w && cabe) break;
            cabe = n = fim;
            if (!p[n]) break;
        }
        if (!cabe) cabe = (int)strlen(p) < (int)sizeof linha - 1 ? (int)strlen(p) : (int)sizeof linha - 1;
        memcpy(linha, p, (size_t)cabe);
        linha[cabe] = 0;
        if (!quebra_so_conta) gfx_texto(x, y + lin * passo, linha, c, f, false);
        lin++;
        p += cabe;
        while (*p == ' ') p++;
    }
    return lin;
}
static int quebra_linhas(int x, int y, int max_w, int max_lin, const char *txt_, uint16_t c)
{
    return quebra_linhas_em(x, y, max_w, max_lin, txt_, c, GFX_FINO, LINHA_FINA);
}
// O texto inteiro numa caixa de max_w x max_h: na fina; se nao couber, na
// miuda (o Agnus); e com a letra maior, se nem a miuda dela (que vira a
// fina) couber, na miuda de verdade (o Revolver).
static int quebra_cabe(int x, int y, int max_w, int max_h, const char *txt_, uint16_t c)
{
    static const int F[3] = { GFX_FINO, GFX_MIUDO, GFX_MIUDO_FIXO };
    for (int i = 0; i < 3; i++) {
        int passo = i == 0 ? LINHA_FINA : i == 1 ? LINHA_MIUDA : 10, dy = i ? 2 : 0;
        quebra_so_conta = true;
        int n = quebra_linhas_em(x, y + dy, max_w, 99, txt_, c, F[i], passo);
        quebra_so_conta = false;
        if (n * passo <= max_h || i == 2) return quebra_linhas_em(x, y + dy, max_w, max_h / passo, txt_, c, F[i], passo);
    }
    return 0;
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
// O que ainda esta fechado no Desafiante (para o perfil em uso) nao se
// mostra: um "?", o nome e o que falta para abrir.
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
// Fechada, nada da peca se mostra: na vitrine, so um "?" de latao flutuando
// na luz, no lugar dela (nem o desenho, nem o que faz); na fileira de baixo,
// um "?" pequeno. Ao lado, so o nome e como abre (coluna_fechada).
static void vitrine_misterio(int cx, int cy)
{
    txt_sombra_c(cx, cy - GFX_ALT_TEXTO(6) / 2 + 6, "?", C_LATAO, 6);
}
static void fileira_misterio(int x, int y)
{
    gfx_texto(x - gfx_largura("?", 1) / 2, y - TXT_H / 2, "?", feltro_tons[5], 1, false);
}

// A fileira embaixo: a pagina de pecas em que esta a da vitrine, a dela com
// as cantoneiras. As fechadas sao so um "?". O mouse em cima
// troca a vitrine; a pagina so vira pelas setas (ou na ponta dela).
#define FIL_N     15
#define FIL_PASSO 38
#define FIL_Y     (MESA_Y1 - 24)
#define CAT_TEXTO_H (FIL_Y - 20 - 152)       // o texto da coluna da direita: de y 152 ate a fileira
// A coluna da direita de uma peca fechada: o nome apagado, o cadeado com
// "Bloqueado" e, embaixo, como abrir (com a primeira letra maiuscula).
static void coluna_fechada(int x, const char *nome, const char *como)
{
    gfx_texto(x, 60, nome, C_TEXTO_M, 2, true);
    cadeado(x + 4, 112, C_VINHO_CLR, 1);
    txt(x + 14, 104, "Bloqueado", C_VINHO_CLR, true);
    char s[200];
    snprintf(s, sizeof s, "%s.", como);
    if (s[0] >= 'a' && s[0] <= 'z') s[0] = (char)(s[0] - 'a' + 'A');
    quebra_cabe(x, 130, 320, FIL_Y - 20 - 130, s, C_MARFIM);   // ate a fileira de baixo
}
enum { FIL_DADO, FIL_AMULETO, FIL_ADESIVO };
static void silhueta_dado(float cx, float cy, float r, int lados, uint16_t cor, float dens);
static int cat_amuleto(int i);               // desafio/colecao.inc
static int cat_adesivo(int i);
static bool amuleto_liberado(int a);
static bool adesivo_aberto(int a);
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
            if (fechado) fileira_misterio(x, y);
            else { peca_t pc = { .tipo = (uint8_t)t }; peca_parada((float)x, (float)y, 11, pc, 0); }
        } else if (sec == FIL_AMULETO) {
            int a = cat_amuleto(i);
            fechado = !amuleto_liberado(a);
            if (fechado) fileira_misterio(x, y);
            else desenha_amuleto(x, y, 10, a, false);
        } else {
            int a = cat_adesivo(i);
            fechado = !adesivo_aberto(a);
            if (fechado) fileira_misterio(x, y);
            else ad_solto_d(x, y, a, ad_estilo == ADE_NOVO ? 2 : 1, 0);   // o recorte pede uns pixels a mais
        }
        zona_clique(x - FIL_PASSO / 2, FIL_Y - 18, FIL_PASSO, 36, var, i, 0);
    }
    int pags = (total + FIL_N - 1) / FIL_N, pag = atual / FIL_N;
    if (pags > 1) {                                   // a pagina: pontinhos na ponta da prateleira
        int px = x0 + (FIL_N - 1) * FIL_PASSO + 30, py = FIL_Y + 20;
        for (int k = 0; k < pags; k++) gfx_rect(px - (pags - k) * 5, py, 3, 3, k == pag ? C_OURO : C_FELTRO2);
    }
}

// A nota ao lado da etiqueta de raridade: inteira se couber ate o fim da
// coluna; senao (a letra maior), a forma curta.
static void nota_etiqueta(int x, int fim, const char *longa, const char *curta)
{
    txt(x, 105, curta && txt_quebra(longa, fim - x) ? curta : longa, C_TEXTO_M, false);
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
    if (st == 2) {                                      // fechado: so o "?", o nome e como abre
        vitrine_misterio(cx, cy);
        fileira_colecao(FIL_DADO, cat_i, n_catalogo(), &cat_i);
        coluna_fechada(290, d->nome, trava);
        return;
    }
    if (tipo == D_TUDO_NADA) {                          // o yin-yang, girando devagar
        tudo_ou_nada((float)cx, (float)cy, 46, t_fase * 0.8f, -1, false);
    } else if (d->lados == 2) {
        moeda_girando(cx, cy, tipo, t_fase);
    } else {
        int f[24], nf = faces_de(tipo, f);
        int i = (int)(t_fase / 0.9f) % nf;
        float dentro = fmodf(t_fase, 0.9f);
        float r = 46 + (dentro < 0.08f && nf > 1 ? 3 : 0);   // pulinho a cada face (com uma so, fica parado)
        float ang = sinf(t_fase * 0.9f) * 0.3f;           // balanca devagar
        if (d->efeito) brilho_poder((float)cx, (float)cy, r, tipo, ang, true);
        so_apagado = true;
        peca_t am = adesivado_amostra();                 // o Adesivado mostra os adesivos de uma amostra
        if (tipo == D_ADESIVADO) ad_face = ad_no_lado(&am, f[i]);
        if (tipo == D_DIGITAL && dentro < DIGITAL_ACENDE) digital_q = dentro / DIGITAL_ACENDE;   // o visor acende a cada numero
        desenha_dado((float)cx, (float)cy, r, tipo, 0, ang, f[i], 2, false);
        if (tipo == D_ADESIVADO) ad_selos((float)cx, (float)cy, r, &am, 0);
        so_apagado = false;
        if (d->efeito) anima_poder(cx, cy, (int)r, tipo, 0);
        if (tipo == D_TREVO && f[i] == 4) {
            gfx_rect(cx - 30, cy + 58, 60, TXT_H + 4, C_BARRA);
            txt_c(cx, cy + 60, "SORTE", C_OURO, true);
        }
    }
    fileira_colecao(FIL_DADO, cat_i, n_catalogo(), &cat_i);

    // A direita: nome, lados, raridade e o que ele faz.
    int x = 290;
    gfx_texto(x, 60, d->nome, cor_dado(tipo), 2, true);
    classe(tipo, s, sizeof s);
    int xt = etiquetas_protecao(etiqueta_caixa(x, 104, s, cor_rar(tipo)), 104, tipo);
    if (so_inicial(tipo)) nota_etiqueta(xt + 2, x + 320, "dado inicial", NULL);
    if (so_desafio(tipo)) nota_etiqueta(xt + 2, x + 320, "só no Desafiante", NULL);
    if (de_natal(tipo)) nota_etiqueta(xt + 2, x + 320, "de Natal: de 20 de outubro a 6 de janeiro", "de Natal: 20 out. a 6 jan.");
    if (tipo == D_TSUNAMI) nota_etiqueta(xt + 2, x + 320, "nasce da Onda", NULL);
    if (tipo == D_D20) nota_etiqueta(xt + 2, x + 320, "sai do Presente e da Sacola de Presente", "do Presente e da Sacola");
    if (st == 1) txt(x, 124, trava, C_VERDE, false);
    gfx_rect(x, 144, 320, 1, C_FELTRO2);
    quebra_cabe(x, 152, 320, CAT_TEXTO_H, TEXTO_CAT[tipo], C_MARFIM);   // ate a fileira de baixo (x: a coluna da direita)
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
    ui_escurece(0, 0, GFX_W, GFX_H, 140);
    ui_cartao_aceso(x0, y0, w, h, true, cor_rar(tipo));
    int cx = x0 + 78, cy = y0 + 96;
    for (int y = -7; y <= 7; y++)                                    // a sombra no cartao, pontilhada
        for (int x = -40; x <= 40; x++) {
            float d = (x * x) / 1600.0f + (y * y) / 49.0f;
            if (d < 1 && bayer(cx + x, cy + 40 + y) < (1 - d) * 1.4f) gfx_pixel(cx + x, cy + 40 + y, C_BARRA);
        }
    peca_t pc = { .tipo = (uint8_t)tipo, .fixo = 0 };               // nova_peca sortearia: aqui so se olha
    if (tipo == D_ADESIVADO) pc = adesivado_amostra();
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
    quebra_cabe(x, y0 + 54, w - 180, h - 54 - 34, TEXTO_CAT[tipo], C_MARFIM);   // ate o "qualquer tecla fecha"
    txt_c(GFX_W / 2, y0 + h - 28, "qualquer tecla fecha", C_TEXTO_M, false);
}
