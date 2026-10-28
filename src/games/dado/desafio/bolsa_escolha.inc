// ---------------------------------------------------------------------------
// A escolha da bolsa, no comeco de cada run: as bolsas sao fichas no
// feltro, cada uma da sua cor, com o icone no centro. As fichas caem uma
// depois da outra; a do cursor sobe, vira e gira, e embaixo vem o nome (na
// letra do titulo), a regra e os dados com que ela comeca. A que ainda nao
// abriu fica escura, com o cadeado, e diz o que a abre; a que acabou de
// abrir chega fechada e o cadeado estoura. Escolhida, a ficha assenta e os
// dados saltam dela; a run comeca.
// ---------------------------------------------------------------------------
#define BS_Y      116                    // o centro das fichas
#define BS_R      32
#define BS_DX     72
#define BS_CAI(i) (0.12f + 0.07f * (i))  // quando a ficha i comeca a cair
#define BS_QUEDA  0.28f
#define BS_VIRA   0.26f                  // a ficha em foco vira (de lado e de volta)
#define BS_REVELA 0.95f                  // o cadeado da nova: treme e estoura
#define BS_CLIQUE_T 1.15f                // escolhida: ate a run comecar
static int   bs_cur;
static int   bs_ant = -1;
static float t_bs_troca = 9;             // desde que o foco mudou
static float bs_sobe[B_N], bs_giro[B_N];
static float t_bs_clique = -1;
static float t_bs_nega = -1;             // fechada: treme e o "para abrir" pisca
static uint8_t bs_novas;                 // as que chegam abertas agora (a festa)
static uint8_t bs_caiu, bs_estourou;     // os sons que ja tocaram
static int bs_x(int i) { return GFX_W / 2 + (2 * i - (B_N - 1)) * BS_DX / 2; }

// Quando o cadeado da nova estoura: depois que todas cairam, uma de cada vez.
static float bs_revela(int b)
{
    int k = 0;
    for (int i = 0; i < b; i++) k += bs_novas >> i & 1;
    return BS_CAI(B_N) + BS_QUEDA + 0.25f + k * (BS_REVELA + 0.15f);
}
static bool bs_fechada(int b)
{
    if (!bolsa_aberta(b)) return true;
    return (bs_novas >> b & 1) && t_fase < bs_revela(b) + BS_REVELA * 0.55f;
}

static void abre_bolsas(void)
{
    const perfil_t *p = perfil();
    bs_novas = 0;
    for (int b = 1; b < B_N; b++)
        if (bolsa_aberta(b) && !(p->bolsas_vistas >> b & 1)) bs_novas |= (uint8_t)(1u << b);
    bs_cur = p->bolsa_ult < B_N && bolsa_aberta(p->bolsa_ult) ? p->bolsa_ult : B_PANO;
    for (int b = 0; b < B_N; b++) if (bs_novas >> b & 1) { bs_cur = b; break; }   // a nova chama
    bs_ant = -1;
    t_bs_troca = 9;
    t_bs_clique = t_bs_nega = -1;
    bs_caiu = bs_estourou = 0;
    for (int b = 0; b < B_N; b++) { bs_sobe[b] = 0; bs_giro[b] = b * 0.5f; }
    cursor = 0;
    t_fase = 0;
    fase = F_RBOLSAS;
}

// As novas ja foram vistas: daqui em diante chegam abertas, sem festa.
static void bs_marca_vistas(void)
{
    if (perfil_atual < 0 || !bs_novas) return;
    perfis[perfil_atual].bolsas_vistas |= bs_novas;
    perfis_salva();
}

static void bs_escolhe(void)
{
    if (t_bs_clique >= 0) return;
    if (t_fase < BS_CAI(bs_cur) + BS_QUEDA) { t_fase = BS_CAI(B_N) + BS_QUEDA; return; }   // ENTER adianta a queda
    if (bs_fechada(bs_cur)) {
        t_bs_nega = 0;
        som_toca_tom(SOM_TIQUE, -7);
        return;
    }
    t_bs_clique = 0;
    som_toca(SOM_MENU_ESCOLHE);
}

static bool bs_tecla(int k, int e)
{
    if (t_bs_clique >= 0) {                       // escolhida: ENTER pula a festa
        if (k == KEY_ENTER) t_bs_clique = BS_CLIQUE_T;
        return true;
    }
    if (e) { bs_cur = (bs_cur + e + B_N) % B_N; return true; }
    if (k >= '1' && k < '1' + B_N) { bs_cur = k - '1'; return true; }
    if (k == KEY_ENTER) bs_escolhe();
    return true;
}

// O passo de tudo das bolsas: o aviso de nova, o Porquinho quebrando, a luz
// do fosforo e, na escolha, as fichas.
static void bolsas_passo(float dt)
{
    if (bolsa_nova >= 0) t_bolsa_nova += dt;
    if (t_porco > 0 && (t_porco += dt) > 1.2f) t_porco = 0;
    t_fosf += dt;
    if (fase != F_RBOLSAS) return;
    if (bs_cur != bs_ant) {
        if (bs_ant >= 0) som_toca_tom(SOM_MENU_PASSA, bs_cur);
        bs_ant = bs_cur;
        t_bs_troca = 0;
    }
    t_bs_troca += dt;
    if (t_bs_nega >= 0 && (t_bs_nega += dt) > 0.6f) t_bs_nega = -1;
    float a = fminf(1, dt * 12);
    for (int b = 0; b < B_N; b++) {
        bool foco = b == bs_cur;
        bs_sobe[b] += ((foco ? 1.0f : 0.0f) - bs_sobe[b]) * a;
        if (foco) bs_giro[b] += dt * 0.8f;
        if (!(bs_caiu >> b & 1) && t_fase >= BS_CAI(b) + BS_QUEDA * 0.8f) {   // a ficha bate no feltro
            bs_caiu |= (uint8_t)(1u << b);
            som_toca_tom(SOM_FICHA, b - 3);
        }
        if ((bs_novas >> b & 1) && !(bs_estourou >> b & 1) && t_fase >= bs_revela(b) + BS_REVELA * 0.55f) {
            bs_estourou |= (uint8_t)(1u << b);   // o cadeado estoura
            som_toca(SOM_NOVIDADE_LENDA);
            solta_confete(40);
            if (bs_estourou == bs_novas) bs_marca_vistas();
        }
    }
    if (t_bs_clique < 0) return;
    float antes = t_bs_clique;
    t_bs_clique += dt;
    bs_giro[bs_cur] += dt * 16 * fmaxf(0, 1 - t_bs_clique / 0.5f);   // gira rapido e deita
    if (antes < 0.3f && t_bs_clique >= 0.3f) som_toca(SOM_BOLSA_ESPIA);   // abre: os dados saltam
    if (t_bs_clique >= BS_CLIQUE_T) {
        t_bs_clique = -1;
        bs_marca_vistas();
        des_bolsa = bs_cur;
        des_nova_run();
    }
}

// ---- O desenho -------------------------------------------------------------
// Uma linha da regra, centrada, com a setinha antes: na letra fina, ou na
// miuda se for comprida.
static void bs_seta(int x, int y, bool sobe, uint16_t c);
static void bs_linha(int y, const char *s, uint16_t c, bool sobe, uint16_t cs)
{
    int f = gfx_largura(s, GFX_FINO) <= 540 ? GFX_FINO : GFX_MIUDO;
    int w = gfx_largura(s, f), x = GFX_W / 2 - (w + 12) / 2;
    bs_seta(x + 3, y + 8, sobe, cs);
    gfx_texto(x + 12, y + (f == GFX_MIUDO ? 4 : 0), s, c, f, false);
}

// O nome, na letra do titulo: as letras caem uma a uma quando o foco chega.
static void bs_nome(int cx, int y, const char *nome, uint16_t c, float t)
{
    int w = gfx_largura(nome, 3), x = cx - w / 2;
    const char *p = nome;
    char pre[48];
    for (int i = 0; *p; i++) {
        const char *ini = p;
        gfx_proximo_car(&p);
        size_t nb = (size_t)(p - ini), np = (size_t)(ini - nome);
        if (np + nb >= sizeof pre) break;
        memcpy(pre, nome, np);
        pre[np] = 0;
        int lx = x + gfx_largura(pre, 3);
        char um[8];
        memcpy(um, ini, nb);
        um[nb] = 0;
        float q = (t - i * 0.028f) / 0.16f;
        if (q <= 0) continue;
        int dy = q < 1 ? (int)((1 - sai_mola(q)) * -14) : 0;
        gfx_texto(lx + 3, y + 3 + dy, um, C_SOMBRA, 3, true);
        gfx_texto(lx, y + dy, um, q < 0.5f ? C_BRANCO : c, 3, true);
    }
}

// As setinhas da regra: para cima (o que melhora) e para baixo (o que piora).
static void bs_seta(int x, int y, bool sobe, uint16_t c)
{
    for (int i = 0; i < 4; i++) {
        int w = sobe ? i : 3 - i;
        gfx_rect(x - w, y + i, 2 * w + 1, 1, c);
    }
}

static void desenha_bolsas(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_titulo("Escolha a bolsa da run");
    {                                                     // a dificuldade, a direita da faixa
        int ty = (FAIXA_H - TXT_H) / 2 - 2;
        const char *dn = NOME_DIF_CURTO[des_dif];
        txt(MESA_X1 - 8 - gfx_largura(dn, 1), ty, dn, COR_DIF[des_dif], true);
    }
    // A luz no feltro, embaixo da ficha em foco.
    if (t_fase > BS_CAI(bs_cur)) {
        int lx = bs_x(bs_cur);
        gfx_rect_alfa(lx - 40, BS_Y - 46, 80, 92, C_FELTRO2, 50);
        gfx_rect_alfa(lx - 30, BS_Y - 40, 60, 80, C_FELTRO2, 40);
    }
    for (int b = 0; b < B_N; b++) {
        float q = (t_fase - BS_CAI(b)) / BS_QUEDA;
        if (q <= 0) continue;
        int cx = bs_x(b), cy = BS_Y - (q < 1 ? (int)((1 - sai_mola(q)) * 70) : 0);
        bool foco = b == bs_cur, fechada = bs_fechada(b);
        float sx = 1, t_cad = -1;
        if (foco && t_bs_troca < BS_VIRA && q >= 1)           // virando: de lado e de volta
            sx = fabsf(cosf(t_bs_troca / BS_VIRA * 3.1416f));
        if (bs_novas >> b & 1) {                              // a nova: o cadeado treme e estoura
            float r = t_fase - bs_revela(b);
            if (r >= 0 && r < BS_REVELA * 0.55f) t_cad = r;
            float v = (r - BS_REVELA * 0.45f) / 0.2f;         // e ela vira, aberta
            if (v > 0 && v < 1) sx = fabsf(cosf(v * 3.1416f));
        }
        if (foco && t_bs_nega >= 0) { cx += (int)(sinf(t_bs_nega * 60) * 3 * (1 - t_bs_nega / 0.6f)); t_cad = t_bs_nega; }
        float aperta = foco && t_bs_clique >= 0 ? sinf(fminf(t_bs_clique / 0.35f, 1) * 3.1416f) : 0;
        cy += (int)lroundf(aperta * 3);
        if (foco && t_bs_clique >= 0 && t_bs_clique < 0.6f) {   // o anel da que assenta
            float c = t_bs_clique / 0.6f;
            int ra = BS_R + 4 + (int)(c * 26);
            uint16_t ca = gfx_mistura(C_OURO, C_FELTRO, (int)(c * 100));
            for (int k = 0; k < 48; k++) {
                float an = k * 0.1309f;
                gfx_rect(cx + (int)(cosf(an) * ra), cy + (int)(sinf(an) * ra), 1, 1, ca);
            }
        }
        ficha_bolsa(cx, cy, BS_R, b, bs_sobe[b], bs_giro[b], foco, fechada, sx, foco ? t_relogio : -1, t_cad);
        if ((bs_novas >> b & 1) && t_fase - bs_revela(b) >= 0 && t_fase - bs_revela(b) < BS_REVELA) {
            float r = (t_fase - bs_revela(b) - BS_REVELA * 0.55f) / 0.4f;   // os pedacos do cadeado
            for (int k = 0; r > 0 && r < 1 && k < 10; k++) {
                float an = k * 0.628f;
                int px = cx + (int)(cosf(an) * 30 * r), py = cy + (int)(-sinf(an) * 30 * r + 40 * r * r);
                gfx_rect(px, py, 2, 2, k % 2 ? C_LATAO : C_ACO);
            }
        }
        if ((bs_novas >> b & 1) && !fechada) {               // a etiqueta de nova, em cima
            int ny = cy - BS_R - 13 - (int)lroundf(bs_sobe[b] * 6) + (int)(sinf(t_relogio * 4 + b) * 1.5f);
            int w = gfx_largura("NOVA", GFX_MIUDO) + 6;
            gfx_rect(cx - w / 2, ny, w, 10, C_VINHO);
            gfx_texto(cx - w / 2 + 3, ny - 2, "NOVA", C_BRANCO, GFX_MIUDO, false);
        }
        const perfil_t *pf = perfil();
        if (!fechada && pf->bolsa_rec[b]) {                    // o recorde: o nivel mais alto vencido com ela
            char nv[4];
            int d = pf->bolsa_rec[b] - 1, mx = cx + BS_R - 6, my = cy + BS_R - 6 - (int)lroundf(bs_sobe[b] * 6);
            snprintf(nv, sizeof nv, "%d", d);
            gfx_disco(mx, my, 7, C_LATAO_ESC);
            gfx_disco(mx, my, 6, COR_DIF[d]);
            gfx_texto(mx - gfx_largura(nv, GFX_MIUDO) / 2, my - 6, nv, C_BARRA, GFX_MIUDO, false);
        }
        zona_clique(cx - BS_R, BS_Y - BS_R - 6, 2 * BS_R, 2 * BS_R + 10, &bs_cur, b, KEY_ENTER);
    }

    // A escolhida: os dados saltam da ficha, giram e caem fora da mesa.
    if (t_bs_clique >= 0.3f) {
        float t = t_bs_clique - 0.3f;
        const bolsa_t *bo = &BOLSA[bs_cur];
        int n = bolsa_n_dados(bs_cur), cx = bs_x(bs_cur);
        for (int i = 0; i < n; i++) {
            float an = 3.1416f * (0.12f + 0.76f * (n > 1 ? (float)i / (n - 1) : 0.5f));
            float v = 150 + (i * 37 % 60), tt = t - i * 0.012f;
            if (tt <= 0) continue;
            float x = cx + cosf(an) * v * tt * 0.9f, y = BS_Y - sinf(an) * v * tt + 260 * tt * tt;
            desenha_dado(x, y, 9, bo->dados[i], 0, tt * (i % 2 ? 9 : -9), 0, 0, false);
        }
        float esc = (t_bs_clique - 0.75f) / (BS_CLIQUE_T - 0.75f);    // e a mesa escurece
        if (esc > 0) gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_BARRA, (int)(fminf(esc, 1) * 230));
    }

    // Embaixo, a da vez: o nome, a regra e os dados do comeco.
    int b = bs_cur;
    const bolsa_t *bo = &BOLSA[b];
    bool fechada = bs_fechada(b);
    if (t_fase < BS_CAI(b) + BS_QUEDA * 0.5f) { rodape_run("setas escolhem  ·  ENTER começa a run  ·  ESC volta"); return; }
    float t = t_bs_troca;
    uint16_t cn = fechada ? C_TEXTO_M : (b == B_PANO ? C_LATAO : bo->cor);
    bs_nome(GFX_W / 2, 148, bo->nome, cn, t);
    float qa = fminf(1, (t - 0.12f) / 0.2f);                  // a regra acende depois do nome
    if (qa > 0) {
        int y = 194;
        uint16_t cg = gfx_mistura(fechada ? C_TEXTO_M : C_MARFIM, C_FELTRO, (int)((1 - qa) * 100));
        uint16_t cc = gfx_mistura(fechada ? C_TEXTO_M : C_MARFIM_S, C_FELTRO, (int)((1 - qa) * 100));
        if (b == B_PANO) {
            txt_c(GFX_W / 2, y + 6, "Os dados de sempre, sem regra nenhuma.", cg, false);
        } else {
            bs_linha(y, bo->ganho, cg, true, gfx_mistura(C_LASER, C_FELTRO, (int)((1 - qa) * 100)));
            bs_linha(y + 18, bo->custo, cc, false, gfx_mistura(C_VINHO_CLR, C_FELTRO, (int)((1 - qa) * 100)));
        }
    }
    // Os dados do comeco: os de numero juntos (D6 x6), e os de poder, depois
    // do "+", com o nome embaixo. Entram um de cada vez.
    int tipos[BOLSA_DADOS], conta[BOLSA_DADOS], ng = 0, n = bolsa_n_dados(b), dy = 252;
    for (int i = 0; i < n; i++) {
        int g = 0;
        while (g < ng && tipos[g] != bo->dados[i]) g++;
        if (g == ng) { tipos[ng] = bo->dados[i]; conta[ng++] = 0; }
        conta[g]++;
    }
    int larg[BOLSA_DADOS], w = 0;
    bool mais_antes = false;
    for (int g = 0; g < ng; g++) {
        bool poder = TIPO[tipos[g]].efeito != EF_NADA;
        char xn[8];
        snprintf(xn, sizeof xn, "x%d", conta[g]);
        larg[g] = poder ? 26 > gfx_largura(TIPO[tipos[g]].nome, GFX_MIUDO) + 4 ? 26 : gfx_largura(TIPO[tipos[g]].nome, GFX_MIUDO) + 4
                        : 24 + (conta[g] > 1 ? gfx_largura(xn, GFX_MIUDO) + 2 : 0);
        if (poder && !mais_antes) { w += 14; mais_antes = true; }   // o "+"
        w += larg[g] + 6;
    }
    if (bo->mais[0]) w += gfx_largura(bo->mais, GFX_FINO) + 8;
    int x = GFX_W / 2 - w / 2;
    gfx_texto(GFX_W / 2 - gfx_largura("começa com", GFX_MIUDO) / 2, dy - 24, "começa com", C_TEXTO_M, GFX_MIUDO, false);
    mais_antes = false;
    for (int g = 0; g < ng; g++) {
        float q = (t - 0.2f - g * 0.05f) / 0.16f;
        bool poder = TIPO[tipos[g]].efeito != EF_NADA;
        if (poder && !mais_antes) {
            mais_antes = true;
            if (q > 0) txt(x + 2, dy - 10, "+", fechada ? C_TEXTO_M : C_OURO, true);
            x += 14;
        }
        if (q > 0) {
            int cx = poder ? x + larg[g] / 2 : x + 12;
            peca_t p = { .tipo = (uint8_t)tipos[g] };
            float r = (poder ? 11 : 10) * (q < 1 ? sai_mola(q) : 1);
            peca_parada((float)cx, (float)dy, r, p, 0);
            if (fechada) gfx_rect_alfa(cx - 12, dy - 12, 25, 25, C_SOMBRA, 120);
            dica_zona(cx - 11, dy - 11, 22, 22, DICA_DADO, tipos[g]);
            char xn[8];
            snprintf(xn, sizeof xn, "x%d", conta[g]);
            if (poder) {
                const char *nm = TIPO[tipos[g]].nome;
                gfx_texto(cx - gfx_largura(nm, GFX_MIUDO) / 2, dy + 12, nm, fechada ? C_TEXTO_M : cor_dado(tipos[g]), GFX_MIUDO, false);
            } else if (conta[g] > 1) gfx_texto(x + 24, dy - 2, xn, fechada ? C_TEXTO_M : C_MARFIM_S, GFX_MIUDO, false);
        }
        x += larg[g] + 6;
    }
    if (bo->mais[0] && t > 0.2f + ng * 0.05f) txt(x + 2, dy - 10, bo->mais, fechada ? C_TEXTO_M : C_MARFIM_S, false);
    // Embaixo de tudo: o que a abre, ou o recorde com ela.
    char s[96];
    if (fechada && !bolsa_aberta(b)) {
        bool pisca = t_bs_nega >= 0 && (int)(t_bs_nega * 10) % 2;
        snprintf(s, sizeof s, "Para abrir: %s", bo->abre);
        txt_c(GFX_W / 2, 276, s, pisca ? C_BRANCO : C_OURO, true);
    } else if (perfil()->bolsa_rec[b]) {
        int d = perfil()->bolsa_rec[b] - 1;
        snprintf(s, sizeof s, "Já venceu uma run com ela no %s", NOME_DIF_CURTO[d]);
        txt_c(GFX_W / 2, 278, s, COR_DIF[d], false);
    }
    rodape_run(fechada && !bolsa_aberta(b) ? "setas escolhem  ·  esta ainda não abriu  ·  ESC volta"
                                           : "setas escolhem  ·  ENTER começa a run  ·  ESC volta");
}
