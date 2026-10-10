// ===========================================================================
// Os dados da casa (colecao) e o detalhe de um dado
// ===========================================================================
static void moeda_girando(int cx, int cy, int tipo, float t)
{
    float c = cosf(t * 2.2f);
    moeda_girando_arte((float)cx, (float)cy, 44, tipo, 0, c, c >= 0 ? 1 : 2);
}

// Faces que o dado pode mostrar, na ordem em que o catalogo as percorre.
static int faces_de(int tipo, int *f)
{
    int n = 0, lados = TIPO[tipo].lados;
    int de = TIPO[tipo].efeito == EF_LASTRO ? 4 : 1;
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

// Etiquetas de protecao, ao lado do tipo e da raridade: "não anulável"
// (Escudo, Teimoso, Egoista) e "intocável" (so o Escudo: nenhum ataque o
// alcanca). Devolve onde a proxima coisa pode comecar.
static int etiqueta_caixa(int x, int y, const char *s, uint16_t c)
{
    int w = gfx_largura(s, GFX_FINO) + 10;
    ui_etiqueta(x, y + 2, w, 16, c);
    gfx_texto(x + 5, y - 1, s, c, GFX_FINO, false);
    return x + w + 4;
}
static int etiquetas_protecao(int x, int y, int tipo)
{
    if (nao_anulavel_t(tipo)) x = etiqueta_caixa(x, y, "não anulável", C_ACO);
    if (intocavel_t(tipo))    x = etiqueta_caixa(x, y, "intocável", C_OURO);
    return x;
}

// Colecao em duas secoes, Dados e Amuletos, trocadas pela chave no alto.
// O que ainda esta fechado no Desafiante (para o perfil em uso) fica no
// escuro, com cadeado e o que falta para abrir.
static int  cat_trava_dado(int t, char *s, int n);       // desafio.inc: 0 sempre livre, 1 aberto, 2 fechado
static void desenha_cat_amuleto(int i);
static void cadeado(int x, int y, uint16_t c, int e);

// As faixas da colecao: a chave Dados | Amuletos, a contagem e as teclas.
static void faixas_colecao(int atual, int total)
{
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(botao_voltar(KEY_ESC), ty, "COLEÇÃO", C_LATAO, true);
    static const char *const SEC[2] = { "Dados", "Amuletos" };
    const int w = 104, h = 26, x0 = GFX_W / 2 - w, y0 = (FAIXA_H - h) / 2 - 1;
    for (int i = 0; i < 2; i++) {                 // duas abas, a escolhida em latao
        ui_botao(x0 + i * w, y0, w, h, SEC[i], cat_sec == i ? C_LATAO : 0, 0);
        zona_clique(x0 + i * w, y0, w, h, NULL, 0, i == 0 ? KEY_UP : KEY_DOWN);
    }
    char s[32];
    snprintf(s, sizeof s, "%d de %d", atual + 1, total);
    txt(GFX_W - 18 - gfx_largura(s, 1), ty, s, C_TEXTO_M, false);
    int yb = GFX_H - FAIXA_H;
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
    teclas_c(GFX_W / 2, yb + (FAIXA_H - TECLA_H) / 2 + 1, "setas trocam o dado  ·  TAB troca a seção  ·  ENTER volta", C_TEXTO_M);
}

// Fechado: a peca fica no escuro, como na sombra (o fundo do disco nao muda).
static void no_escuro(int cx, int cy, int R, uint16_t fundo)
{
    uint16_t *fb = display_fb();
    for (int y = cy - R; y <= cy + R; y++) {
        if (y < 0 || y >= GFX_H) continue;
        for (int x = cx - R; x <= cx + R; x++) {
            if (x < 0 || x >= GFX_W || (x - cx) * (x - cx) + (y - cy) * (y - cy) > R * R) continue;
            uint16_t *p = &fb[y * GFX_W + x];
            if (*p != fundo) *p = gfx_mistura(*p, C_SOMBRA, 62);
        }
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
    int tipo = cat_tipo(cat_i);
    const tipo_t *d = &TIPO[tipo];
    char s[32];
    faixas_colecao(cat_i, D_N);
    char trava[80];
    int st = cat_trava_dado(tipo, trava, sizeof trava);

    // A peca, a esquerda, girando pelas faces possiveis.
    int cx = 160, cy = CENTRO + 4;
    gfx_disco(cx, cy, 80, gfx_mistura(C_FELTRO_ESC, cor_rar(tipo), 55));   // anel da raridade
    gfx_disco(cx, cy, 77, C_FELTRO_ESC);
    if (tipo == D_TUDO_NADA) {                          // o yin-yang, girando devagar
        so_apagado = true;                              // fechado: apagado
        tudo_ou_nada((float)cx, (float)cy, 46, t_fase * 0.8f, -1, st == 2);
        so_apagado = false;
    } else if (d->lados == 2) {
        moeda_girando(cx, cy, tipo, t_fase);
    } else {
        int f[24], nf = faces_de(tipo, f);
        int i = (int)(t_fase / 0.45f) % nf;
        float dentro = fmodf(t_fase, 0.45f);
        float r = 46 + (dentro < 0.08f ? 4 : 0);          // pulinho a cada face
        float ang = sinf(t_fase * 1.7f) * 0.22f;
        bool com_pips = d->lados == 6 && d->efeito != EF_ESPELHO;
        so_apagado = true;
        desenha_dado((float)cx, (float)cy, r, tipo, 0, ang, com_pips ? f[i] : -1, 2, st == 2);
        so_apagado = false;
        if (!com_pips) {
            snprintf(s, sizeof s, "%d", f[i]);
            gfx_texto(cx - gfx_largura(s, 2) / 2, cy - 24, s, st == 2 ? C_FELTRO2 : C_EBANO, 2, true);
        }
        if (tipo == D_TREVO && f[i] == 4) {
            gfx_rect(cx - 30, cy + 58, 60, TXT_H + 4, C_BARRA);
            txt_c(cx, cy + 60, "SORTE", C_OURO, true);
        }
    }

    if (st == 2) {                                       // fechado: no escuro, cadeado no aro
        no_escuro(cx, cy, 76, C_FELTRO_ESC);
        cadeado(cx + 54, cy - 66, C_VINHO_CLR, 2);
    }

    // A direita: nome, lados, raridade e o que ele faz.
    int x = 290;
    gfx_texto(x, 70, d->nome, st == 2 ? C_TEXTO_M : cor_dado(tipo), 2, true);
    classe(tipo, s, sizeof s);
    int xt = etiquetas_protecao(etiqueta_caixa(x, 118, s, cor_rar(tipo)), 118, tipo);
    if (so_inicial(tipo)) txt(xt + 2, 119, "dado inicial", C_TEXTO_M, false);
    if (so_desafio(tipo)) txt(xt + 2, 119, "só no Desafiante", C_TEXTO_M, false);
    if (st == 2) {
        linha_bloqueio(x, 139, trava);
    } else if (st == 1) {
        txt(x, 139, trava, C_VERDE, false);
    }
    gfx_rect(x, 160, 320, 1, C_FELTRO2);
    quebra_linhas(x, 170, 320, 6, TEXTO_CAT[tipo], C_MARFIM);
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
        if (!removendo && cursor >= 0 && cursor < 3) return oferta[cursor].tipo;
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
    const int x0 = 70, y0 = 70, w = GFX_W - 140, h = 220;
    gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_SOMBRA, 140);
    ui_cartao(x0, y0, w, h, true, cor_rar(tipo));
    int cx = x0 + 78, cy = y0 + 96;
    gfx_disco(cx, cy, 52, gfx_mistura(C_FELTRO_ESC, cor_rar(tipo), 55));
    gfx_disco(cx, cy, 49, C_FELTRO_ESC);
    peca_t pc = { (uint8_t)tipo, 0 };               // nova_peca sortearia: aqui so se olha
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
