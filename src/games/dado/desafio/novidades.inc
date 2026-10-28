// ---------------------------------------------------------------------------
// Novidades: o que abriu, um card de cada vez
// ---------------------------------------------------------------------------
// Logo depois de cada chefe vencido pela primeira vez na dificuldade (o
// pedaco dele: dados, amuleto, adesivo; e o D20 ou o Lastro, na primeira
// vitoria contra o Crupie e a Dona) e no fim da run vencida (a dificuldade
// nova e o pedaco de quem fecha a run). Os cards esperam de costas na fileira de baixo; um de cada
// vez sobe, vira no meio da mesa e volta de frente para o lugar, sozinho.
// ENTER acelera, TAB mostra tudo; no fim, o resumo (com um card so, ele fica).
#define NOV_MAX   (PEDACO_MAX + 2)       // um pedaco, o dado do marco e a dificuldade
#define NOV_ENTRA 0.35f                  // a fileira aparece antes do primeiro
#define NOV_VOA   0.22f                  // de costas, da fileira ao meio
#define NOV_VIRA  0.20f
#define NOV_VOLTA 0.20f                  // de frente, de volta ao lugar
#define NOV_CW    300
#define NOV_CH    132
#define NOV_CY    170
#define NOV_FY    (MESA_Y1 - 24)         // a fileira
static novidade_t nov[NOV_MAX];
static int  n_nov;
static bool nov_da_run;                  // depois: os creditos (fim da run) ou a run segue (o chefe)
static char nov_sub[64];
static int  nov_tocou;                   // quantos cards ja viraram, com o som
static bool nov_fim_tocou;

static int nov_quantas(void) { return n_nov; }
static void nov_poe(int k, int v) { if (n_nov < NOV_MAX) nov[n_nov++] = (novidade_t){ (uint8_t)k, (uint8_t)v }; }

// A dificuldade e os lendarios ficam mais tempo no meio, com o som grande.
static bool nov_grande(int i)
{
    return nov[i].k == NV_DIF || (nov[i].k == NV_DADO && TIPO[nov[i].v].raridade == RAR_LENDA);
}
static float nov_mostra(int i) { return nov_grande(i) ? 1.2f : 0.8f; }
static float nov_ini(int i)
{
    float t = NOV_ENTRA;
    for (int k = 0; k < i && k < n_nov; k++) t += NOV_VOA + NOV_VIRA + nov_mostra(k) + NOV_VOLTA;
    return t;
}
static float nov_virou(int i) { return nov_ini(i) + NOV_VOA + NOV_VIRA; }   // o card i ja de frente
// Acabou de mostrar: o resumo (com mais de um) ou o card unico de frente.
static bool nov_pronto(void) { return n_nov > 1 ? t_fase >= nov_ini(n_nov) : t_fase >= nov_virou(0); }

// O card da vez e a etapa dele: -1 a fileira entrando, 0 voando, 1 virando,
// 2 no meio, 3 voltando, 4 todos ja vistos. q: quanto da etapa ja passou.
static int nov_atual(int *etapa, float *q)
{
    float t = t_fase - NOV_ENTRA;
    if (t < 0) { *etapa = -1; *q = t_fase / NOV_ENTRA; return 0; }
    for (int i = 0; i < n_nov; i++) {
        float d[4] = { NOV_VOA, NOV_VIRA, nov_mostra(i), NOV_VOLTA };
        for (int e = 0; e < 4; e++) {
            if (n_nov == 1 && e == 2) { *etapa = 2; *q = t / d[2] < 1 ? t / d[2] : 1; return 0; }   // um so: fica
            if (t < d[e]) { *etapa = e; *q = t / d[e]; return i; }
            t -= d[e];
        }
    }
    *etapa = 4; *q = 1;
    return n_nov;
}

// ---- O que abriu ------------------------------------------------------------
// O pedaco do chefe vencido (perfis.inc), quando e novo, sem a dificuldade.
static void nov_poe_pedaco(void)
{
    if (!des_chefe_novo) return;
    novidade_t p[PEDACO_MAX];
    int n = pedaco(run.dificuldade, des_chefe_c, p);
    for (int i = 0; i < n; i++) if (p[i].k != NV_DIF) nov_poe(p[i].k, p[i].v);
    des_chefe_novo = false;
}

// Fim da run vencida: a dificuldade que abriu e o pedaco de quem fechou a run.
static void nov_monta_run(void)
{
    n_nov = 0;
    if (des_liberou > 0) nov_poe(NV_DIF, des_liberou);
    nov_poe_pedaco();
    snprintf(nov_sub, sizeof nov_sub, "Você venceu %s no %s", PERSO[run.no[run.passo].perso].nome,
             NOME_DIF_CURTO[run.dificuldade]);
    nov_da_run = true;
}

// A linha da tela do fim da run: quantas, e a dificuldade que abriu.
static void nov_linha_fim(int y)
{
    char s[64];
    const char *nn = n_nov == 1 ? "novidade" : "novidades";
    if (des_liberou > 0) snprintf(s, sizeof s, "Abriu o %s  ·  %d %s", NOME_DIF_CURTO[des_liberou], n_nov, nn);
    else snprintf(s, sizeof s, "%d %s", n_nov, nn);
    txt_c(GFX_W / 2, y, s, des_liberou > 0 ? COR_DIF[des_liberou] : C_OURO, true);
}

static void nov_abre(void)
{
    nov_tocou = 0;
    nov_fim_tocou = false;
    t_fase = 0;
    fase = F_NOVIDADES;
}

// Um chefe vencido no meio da run: o pedaco dele (na primeira vez nesta
// dificuldade) e, na primeira vitoria contra o Crupie e a Dona, o D20 e o
// Lastro.
static bool nov_tem_chefe(void) { return des_marco >= 0 || des_chefe_novo; }
static void nov_abre_chefe(void)
{
    n_nov = 0;
    bool so_marco = !des_chefe_novo;
    if (des_marco >= 0) nov_poe(NV_DADO, des_marco);
    nov_poe_pedaco();
    const char *nome = PERSO[run.no[run.passo].perso].nome;
    if (so_marco) snprintf(nov_sub, sizeof nov_sub, "Primeira vitória contra %s", nome);
    else snprintf(nov_sub, sizeof nov_sub, "Você venceu %s no %s", nome, NOME_DIF_CURTO[run.dificuldade]);
    nov_da_run = false;
    des_marco = -1;
    nov_abre();
}

static void nov_segue(void)
{
    if (nov_da_run) { t_fase = 0; fase = F_CREDITOS; }   // venceu a run: os creditos antes do menu
    else des_pos_vitoria();                               // o chefe: o amuleto dele e a loja
}

// O som de cada card quando ele vira: um semitom acima a cada um; o grande
// tem o seu. E o do resumo, quando ele entra.
static void nov_passo(void)
{
    if (fase != F_NOVIDADES) return;
    if (nov_tocou < n_nov && t_fase >= nov_ini(nov_tocou) + NOV_VOA + NOV_VIRA * 0.5f) {
        int i = nov_tocou++;
        if (nov_grande(i)) som_toca(SOM_NOVIDADE_LENDA);
        else som_toca_tom(SOM_NOVIDADE, i < 9 ? i : 9);
    }
    if (n_nov > 1 && nov_pronto() && !nov_fim_tocou) { nov_fim_tocou = true; som_toca(SOM_PREMIO); }
}

static void nov_tudo(void)               // TAB: direto ao resumo (um card so: ele de frente)
{
    if (n_nov > 1) { nov_tocou = n_nov; t_fase = nov_ini(n_nov) + 0.001f; }
    else if (t_fase < nov_virou(0)) t_fase = nov_virou(0) + 0.001f;
}

static bool nov_tecla(int k)
{
    if (k == KEY_TAB || k == 'x') { if (nov_pronto()) nov_segue(); else nov_tudo(); return true; }
    if (k != KEY_ENTER) return true;
    if (nov_pronto()) { nov_segue(); return true; }
    int e;
    float q;
    int i = nov_atual(&e, &q);           // ENTER acelera: vira ja, ou ja volta, ou o proximo
    if (e < 2)       t_fase = nov_virou(i) + 0.001f;             // a folga: cai do lado certo da etapa
    else if (e == 2) t_fase = nov_virou(i) + nov_mostra(i) + 0.001f;
    else             t_fase = nov_ini(i + 1) + 0.001f;
    return true;
}

// ---- Os textos e as figuras ---------------------------------------------------
static const char *nov_nome(int i)
{
    int v = nov[i].v;
    switch (nov[i].k) {
    case NV_DIF:     return NOME_DIF[v];
    case NV_AMULETO: return AMULETO[v].nome;
    case NV_DADO:    return TIPO[v].nome;
    case NV_ADESIVO: return ADESIVO[v].nome;
    default:         return "Estilo novo";
    }
}
static uint16_t nov_cor(int i)           // o filete do card e o rotulo
{
    int v = nov[i].v;
    switch (nov[i].k) {
    case NV_DIF:     return COR_DIF[v];
    case NV_AMULETO: return AMULETO[v].cor;
    case NV_DADO:    return cor_rar(v);
    case NV_ADESIVO: return ADESIVO[v].luz;
    default:         return C_OURO;
    }
}
static uint16_t nov_cor_nome(int i) { return nov[i].k == NV_DADO ? cor_dado(nov[i].v) : nov_cor(i); }
static void nov_rotulo(int i, char *s, int n)
{
    switch (nov[i].k) {
    case NV_DIF:     snprintf(s, (size_t)n, "dificuldade · %s", ANDAR_DIF[nov[i].v]); break;
    case NV_AMULETO: snprintf(s, (size_t)n, "amuleto"); break;
    case NV_DADO:    classe(nov[i].v, s, n); break;
    case NV_ADESIVO: snprintf(s, (size_t)n, "adesivo"); break;
    default:         snprintf(s, (size_t)n, "estilo · %s", ANDAR_DIF[nov[i].v]); break;
    }
}
static const char *nov_texto(int i)
{
    static char s[200];
    int v = nov[i].v;
    switch (nov[i].k) {
    case NV_DIF: {
        char cj[96];
        if (coringa_txt(v, cj, sizeof cj)) snprintf(s, sizeof s, "O que muda: %s %s", PENA_DIF[v], cj);
        else snprintf(s, sizeof s, "O que muda: %s", PENA_DIF[v]);
        return s;
    }
    case NV_AMULETO: return AMULETO[v].desc;
    case NV_DADO:    return TEXTO_MEDIO[v];
    case NV_ADESIVO: return AD_TEXTO[v];
    default: {
        char es[96];
        estilo_do_andar(v, es, sizeof es);
        snprintf(s, sizeof s, "%s. Escolha no botão estilo, na tela dos perfis.", es);
        return s;
    }
    }
}

// A figura grande, no quadro do card.
static void nov_figura(int i, int cx, int cy, float desde)
{
    int v = nov[i].v;
    switch (nov[i].k) {
    case NV_DIF: {
        char s[4];
        snprintf(s, sizeof s, "%d", v);
        gfx_disco(cx, cy, 30, COR_DIF[v]);
        gfx_disco(cx, cy, 27, C_FELTRO_ESC);
        txt_sombra_c(cx, cy - 14, s, COR_DIF[v], 2);
        break;
    }
    case NV_AMULETO: desenha_amuleto(cx, cy, 24, v, false); break;
    case NV_DADO:
        if (desde < 0.5f) brilho_passa = desde / 0.5f;   // a luz atravessa ao virar
        desenha_dado((float)cx, cy + sinf(desde * 3) * 2, 28, v, 0, 0, 0, 0, false);
        break;
    case NV_ADESIVO: ad_solto_d(cx, cy, v, 4, 0); break;
    default: {
        int lin[4], k[4], n = estilo_partes(v, lin, k);
        for (int j = 0; j < n; j++)                 // tres partes: mais juntas, acima do rotulo
            es_amostra(lin[j], k[j], cx, n >= 3 ? cy - 4 + (j - 1) * 32 : cy + (j * 2 - (n - 1)) * 18, true);
        break;
    }
    }
}

// O desenho pequeno: na fileira e no resumo.
static void nov_icone(int i, int cx, int cy, int r)
{
    int v = nov[i].v;
    switch (nov[i].k) {
    case NV_DIF: {
        char s[4];
        snprintf(s, sizeof s, "%d", v);
        gfx_disco(cx, cy, r, COR_DIF[v]);
        gfx_disco(cx, cy, r - 2, C_FELTRO_ESC);
        txt_c(cx, cy - TXT_H / 2, s, COR_DIF[v], true);
        break;
    }
    case NV_AMULETO: desenha_amuleto(cx, cy, r, v, false); break;
    case NV_DADO:    desenha_dado((float)cx, (float)cy, (float)r, v, 0, 0, 0, 0, false); break;
    case NV_ADESIVO: ad_solto_d(cx, cy, v, r > 10 ? 2 : 1, 0); break;
    default: {                                   // a primeira parte: o feltro, o dado no material ou a madeira
        int lin[4], k[4];
        if (estilo_partes(v, lin, k)) estilo_mini(lin[0], k[0], cx, cy, r);
        break;
    }
    }
}

// As costas: o vinho do cassino, pontilhado, com o "?" no meio.
static void nov_verso(int cx, int cy, int w, int h)
{
    if (w < 2) return;
    int x = cx - w / 2, y = cy - h / 2;
    gfx_rect(x + 2, y + 3, w, h, C_SOMBRA);
    gfx_rect(x, y, w, h, C_LATAO_ESC);
    if (w > 4) gfx_rect(x + 2, y + 2, w - 4, h - 4, RGB(96, 28, 34));
    for (int j = 6; j < h - 6; j += 10)
        for (int i = 6; i < w - 6; i += 10)
            if (((i + j) / 10) & 1) gfx_rect(x + i, y + j, 2, 2, RGB(140, 52, 56));
    if (w > 44 && h > 44) {
        gfx_disco(cx, cy, 16, C_LATAO_ESC);
        gfx_disco(cx, cy, 13, RGB(96, 28, 34));
        txt_c(cx, cy - TXT_H / 2, "?", C_OURO, true);
    }
}

// A frente inteira, no meio da mesa. desde: o tempo desde que virou.
static void nov_frente(int i, int cx, int cy, float desde)
{
    int x0 = cx - NOV_CW / 2, y0 = cy - NOV_CH / 2;
    uint16_t c = nov_cor(i);
    ui_cartao_aceso(x0, y0, NOV_CW, NOV_CH, true, c);
    gfx_rect(x0 + 8, y0 + 8, 96, NOV_CH - 16, gfx_mistura(c, C_FELTRO_ESC, 84));     // o quadro da figura
    nov_figura(i, x0 + 56, y0 + NOV_CH / 2 - 6, desde);
    char s[48];
    nov_rotulo(i, s, sizeof s);
    gfx_texto(x0 + 56 - gfx_largura(s, GFX_MIUDO) / 2, y0 + NOV_CH - 22, s, c, GFX_MIUDO, false);
    txt_sombra_c(x0 + 208, y0 + 10, nov_nome(i), nov_cor_nome(i), 1);
    quebra_linhas(x0 + 114, y0 + 36, NOV_CW - 124, 6, nov_texto(i), C_MARFIM_S);
    if (nov_grande(i)) {                         // o grande: uma faixa de luz atravessa o card
        float f = fmodf(desde, 2.2f) / 0.7f;
        if (f < 1) {
            int bx = x0 - NOV_CH + (int)(f * (NOV_CW + NOV_CH));
            for (int yy = 2; yy < NOV_CH - 2; yy++) {
                int xx = bx + (NOV_CH - yy) / 2;
                int a = xx < x0 + 2 ? x0 + 2 : xx, b = xx + 7 > x0 + NOV_CW - 2 ? x0 + NOV_CW - 2 : xx + 7;
                if (b > a) gfx_rect_alfa(a, y0 + yy, b - a, 1, C_BRANCO, 46);
            }
        }
    }
}

// Raios atras do card, girando devagar, entre o titulo e a fileira.
static void nov_raios(int i, int cx, int cy, float desde, float forca)
{
    if (forca <= 0) return;
    uint16_t c = gfx_mistura(nov_cor(i), C_FELTRO_ESC, (int)(80 - 25 * forca));
    int ymin = MESA_Y0 + 44, ymax = NOV_FY - 18, n = nov_grande(i) ? 20 : 14;
    for (int k = 0; k < n; k++) {
        float a = k * 6.2832f / n + desde * 0.35f;
        float ca = cosf(a), sa = sinf(a);
        float sx = cx + ca * (NOV_CW / 2 + 10), sy = cy + sa * (NOV_CH / 2 + 8);
        float ex = cx + ca * (NOV_CW / 2 + 10 + 90 * forca), ey = cy + sa * (NOV_CH / 2 + 8 + 50 * forca);
        if (sy < ymin || sy > ymax) continue;
        if (ey < ymin) { float f = (ymin - sy) / (ey - sy); ex = sx + f * (ex - sx); ey = (float)ymin; }
        if (ey > ymax) { float f = (ymax - sy) / (ey - sy); ex = sx + f * (ex - sx); ey = (float)ymax; }
        gfx_linha_grossa((int)sx, (int)sy, (int)ex, (int)ey, 3, c);
    }
}

// O clarao de quando vira: um aro de pontos que abre em volta do card.
static void nov_clarao(int i, int cx, int cy, float desde)
{
    float q = desde / 0.35f;
    if (q < 0 || q >= 1) return;
    uint16_t c = gfx_mistura(C_BRANCO, nov_cor(i), (int)(q * 80));
    float rx = NOV_CW / 2 + 4 + q * 60, ry = NOV_CH / 2 + 4 + q * 30;
    for (int k = 0; k < 48; k++) {
        float a = k * 0.1309f;
        gfx_rect(cx + (int)(cosf(a) * rx) - 1, cy + (int)(sinf(a) * ry) - 1, 2, 2, c);
    }
}

static int nov_slot_x(int i) { return GFX_W / 2 + (int)((i - (n_nov - 1) / 2.0f) * 30); }

// A fileira: os vistos de frente, pequenos; os que faltam, de costas.
static void nov_fileira(int atual, int etapa, float q)
{
    for (int i = 0; i < n_nov; i++) {
        int x = nov_slot_x(i), y = NOV_FY;
        if (etapa < 0) {                         // entrando, um atras do outro
            float e = q * 1.6f - i * 0.04f;
            if (e <= 0) continue;
            y += (int)((1 - (e < 1 ? e : 1)) * 14);
        }
        bool visto = i < atual || (i == atual && etapa == 3 && q >= 1);
        if (visto) {
            gfx_rect(x - 12, y - 12, 24, 24, gfx_mistura(nov_cor(i), C_FELTRO_ESC, 72));
            ui_contorno(x - 12, y - 12, 24, 24, gfx_mistura(nov_cor(i), C_FELTRO_ESC, 30));
            nov_icone(i, x, y, 9);
        } else if (i == atual && etapa >= 0) {
            ui_contorno(x - 11, y - 13, 22, 26, C_FELTRO2);   // o lugar vazio: ele esta no meio
        } else {
            nov_verso(x, y, 20, 26);
        }
    }
}

// O resumo: tudo de frente, em ate duas linhas, cada um com a sua dica.
static void nov_resumo(float desde)
{
    int cols = n_nov <= 6 ? n_nov : (n_nov + 1) / 2;
    if (cols > 7) cols = 7;
    int w = cols > 6 ? 80 : 92, h = 72, vao = 6, linhas = (n_nov + cols - 1) / cols;
    int y0 = (MESA_Y0 + 50 + NOV_FY) / 2 - (linhas * (h + 8) - 8) / 2;
    for (int i = 0; i < n_nov; i++) {
        int lin = i / cols, col = i % cols, na_lin = lin == linhas - 1 ? n_nov - lin * cols : cols;
        int cx = GFX_W / 2 + (int)((col - (na_lin - 1) / 2.0f) * (w + vao)), y = y0 + lin * (h + 8);
        float q = (desde - i * 0.04f) / 0.25f;
        if (q <= 0) continue;
        int dy = (int)((1 - sai_mola(q)) * 10);
        uint16_t c = nov_cor(i);
        ui_cartao_aceso(cx - w / 2, y + dy, w, h, false, c);
        nov_icone(i, cx, y + dy + 26, 13);
        nome_no_cartao(cx, y + dy + 48, nov_nome(i), nov_cor_nome(i), w - 8);
        int dt = nov[i].k == NV_DADO ? DICA_DADO : nov[i].k == NV_AMULETO ? DICA_AMULETO
               : nov[i].k == NV_ADESIVO ? DICA_ADESIVO : -1;
        if (dt >= 0) dica_zona(cx - w / 2, y, w, h, dt, nov[i].v);
    }
}

static void desenha_novidades(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 130);
    faixa_run(0);
    float q;
    int etapa, i = nov_atual(&etapa, &q), cx = GFX_W / 2;
    float desde = t_fase - (i < n_nov ? nov_virou(i) : nov_ini(n_nov));
    // Os raios vem antes do titulo: passam por baixo dele.
    if (etapa == 2) nov_raios(i, cx, NOV_CY, desde, desde < 0.15f ? desde / 0.15f : 1);
    else if (etapa == 3) nov_raios(i, cx, NOV_CY, desde, 1 - q);
    txt_sombra_c(GFX_W / 2, MESA_Y0 + 4, "NOVIDADES", C_OURO, 2);
    txt_c(GFX_W / 2, MESA_Y0 + 30, nov_sub, C_TEXTO_M, false);
    if (etapa == 4) {
        nov_resumo(desde);
        rodape_run("ENTER segue");
        return;
    }
    nov_fileira(i, etapa, q);
    if (etapa >= 0) {
        int sx = nov_slot_x(i);
        if (etapa == 0) {                        // de costas, da fileira ao meio, num arco
            float e = 1 - (1 - q) * (1 - q) * (1 - q);
            int w = 20 + (int)((NOV_CW - 20) * e), h = 26 + (int)((NOV_CH - 26) * e);
            int x = sx + (int)((cx - sx) * e), y = NOV_FY + (int)((NOV_CY - NOV_FY) * e) - (int)(sinf(q * 3.1416f) * 16);
            nov_verso(x, y, w, h);
        } else if (etapa == 1) {                 // vira: as costas fecham, a frente abre
            if (q < 0.5f) nov_verso(cx, NOV_CY, (int)(NOV_CW * (1 - 2 * q)), NOV_CH);
            else {
                int w = (int)(NOV_CW * (2 * q - 1));
                if (w > 4) ui_cartao_aceso(cx - w / 2, NOV_CY - NOV_CH / 2, w, NOV_CH, true, nov_cor(i));
            }
        } else if (etapa == 2) {
            int pulo = desde < 0.16f ? (int)(sinf(desde / 0.16f * 3.1416f) * 4) : 0;
            nov_frente(i, cx, NOV_CY - pulo, desde);
            nov_clarao(i, cx, NOV_CY, desde);
            char s[24];
            snprintf(s, sizeof s, "%d de %d", i + 1, n_nov);
            if (n_nov > 1) txt_c(cx, NOV_CY + NOV_CH / 2 + 6, s, C_TEXTO_M, false);
            if (nov[i].k == NV_DADO) dica_zona(cx - NOV_CW / 2, NOV_CY - NOV_CH / 2, NOV_CW, NOV_CH, DICA_DADO, nov[i].v);
        } else {                                 // de frente, encolhendo de volta ao lugar
            float e = q * q;
            int w = NOV_CW + (int)((24 - NOV_CW) * e), h = NOV_CH + (int)((24 - NOV_CH) * e);
            int x = cx + (int)((sx - cx) * e), y = NOV_CY + (int)((NOV_FY - NOV_CY) * e);
            if (w > 4 && h > 4) ui_cartao_aceso(x - w / 2, y - h / 2, w, h, true, nov_cor(i));
            nov_icone(i, x, y, 9 + (int)((1 - e) * 14));
        }
    }
    rodape_run(nov_pronto() ? "ENTER segue" : n_nov > 1 ? "ENTER acelera  ·  TAB mostra tudo" : "ENTER acelera");
}
