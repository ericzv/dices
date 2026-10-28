// ---------------------------------------------------------------------------
// A escolha de amuleto: os tres do comeco e de cada chefe, o sorteado do
// Selo de Divida e, com os 5 lugares cheios, a troca.
// ---------------------------------------------------------------------------
// Os seus, na ordem do numero (a troca e a faixa contam igual).
static int meus_amuletos(int *a)
{
    int n = 0;
    for (int i = 0; i < A_N; i++) if (run_amuletos() >> i & 1) a[n++] = i;
    return n;
}

// Os 5 cheios: o novo em cima; embaixo os seus e, no fim, ficar sem o novo.
#define TROCA_N   (am_lugares() + 1)        // os lugares de agora (4 com o Bauzinho) e ficar sem o novo
#define TROCA_W   88
#define TROCA_PASSO 98
static void desenha_troca_amuleto(void)
{
    int novo = run.am_novo - 1, meus[A_N], n = meus_amuletos(meus);
    txt_c2(GFX_W / 2, MESA_Y0 + 2, "SEM LUGAR: QUAL SAI?", C_OURO);
    char s[96];
    snprintf(s, sizeof s, "Cabem %d amuletos. Para levar o novo, um dos seus sai.", am_lugares());
    txt_c(GFX_W / 2, MESA_Y0 + 44, s, C_MARFIM_S, false);
    // O novo, com o que faz.
    int x = 120, yc = 140;
    desenha_amuleto(x, yc + (int)(sinf(t_fase * 3) * 2), 22, novo, false);
    txt(x + 34, yc - 22, AMULETO[novo].nome, AMULETO[novo].cor, true);
    if (nome_rar_am(novo)[0])
        ui_selo(x + 34 + gfx_largura(AMULETO[novo].nome, 1) + 10, yc - 22, nome_rar_am(novo), cor_rar_am(novo), GFX_FINO);
    quebra_linhas(x + 34, yc - 4, 400, 2, AMULETO[novo].desc, C_MARFIM_S);
    // Os seus e a ultima saida.
    int x0 = GFX_W / 2 - (TROCA_N - 1) * TROCA_PASSO / 2, cy = 214;
    for (int i = 0; i < TROCA_N; i++) {
        int cx = x0 + i * TROCA_PASSO;
        bool cur = i == cursor, fica = i == TROCA_N - 1 || i >= n;
        ui_cartao(cx - TROCA_W / 2, cy - 38, TROCA_W, 84, cur, fica ? C_TEXTO_M : C_VINHO_CLR);
        float bob = cur ? sinf(t_fase * 4) * 2 - mola_px() : 0;
        if (i < n && i < TROCA_N - 1) {
            desenha_amuleto(cx, cy - 8 + (int)bob, 18, meus[i], false);
            if (cur) {                               // o que sai fica riscado
                gfx_linha_grossa(cx - 14, cy - 22 + (int)bob, cx + 14, cy + 6 + (int)bob, 2, C_VINHO_CLR);
            }
            nome_no_cartao(cx, cy + 20, AMULETO[meus[i]].nome, cur ? AMULETO[meus[i]].cor : C_MARFIM_S, TROCA_W - 6);
        } else if (i == TROCA_N - 1) {
            desenha_amuleto(cx, cy - 8 + (int)bob, 18, novo, true);   // o novo, apagado: fica de fora
            nome_no_cartao(cx, cy + 20, "deixar o novo", cur ? C_MARFIM : C_TEXTO_M, TROCA_W - 6);
        }
        zona_clique(cx - TROCA_W / 2, cy - 38, TROCA_W, 84, &cursor, i, KEY_ENTER);
    }
    if (cursor >= 0 && cursor < n && cursor < TROCA_N - 1) {
        snprintf(s, sizeof s, "Sai: %s", AMULETO[meus[cursor]].desc);
        txt_c(GFX_W / 2, 268, s, C_VINHO_CLR, false);
    } else if (cursor == TROCA_N - 1)
        txt_c(GFX_W / 2, 268, "Fica tudo como está: o novo vai embora.", C_MARFIM_S, false);
    bool volta = run.am_volta == AV_INICIO || run.am_volta == AV_CHEFE;
    if (volta) botao_clique(MESA_X1 - 12, MESA_Y1 - 30, "voltar", KEY_BKSP);
    rodape_run(volta ? "setas escolhem  ·  ENTER confirma  ·  BACKSPACE volta"
                     : "setas escolhem  ·  ENTER confirma");
}

static void desenha_amuleto_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run(0);
    if (run.am_novo) { desenha_troca_amuleto(); return; }
    bool selo = run.am_volta == AV_SELO;
    bool bau2 = run.am_volta == AV_INICIO && run.bolsa == B_BAU && am_conta(run_amuletos()) == 1;   // o Bauzinho: o segundo
    txt_c2(GFX_W / 2, MESA_Y0 + 2, selo ? "O SELO DE DÍVIDA COBRA MAIS UM"
                                 : run.am_volta == AV_CHEFE ? "O CHEFE DEIXOU UM AMULETO"
                                 : bau2 ? "O BAUZINHO: MAIS UM" : "ESCOLHA UM AMULETO", C_OURO);
    char s[96];
    if (selo) snprintf(s, sizeof s, "Sorteado entre os comuns e os incomuns.");
    else if (!am_lugar()) snprintf(s, sizeof s, "Seus %d lugares estão cheios: para levar, um dos seus sai.", am_lugares());
    else snprintf(s, sizeof s, "Ele vale a run inteira.");
    txt_c(GFX_W / 2, MESA_Y0 + 44, s, am_lugar() || selo ? C_MARFIM_S : C_OURO, false);
    for (int i = 0; i < 3; i++) {
        int a = run.am_oferta[i];
        if (a >= A_N) continue;
        int x = GFX_W / 2 + (i - 1) * 150;
        bool cur = i == cursor;
        ui_cartao(x - 64, 124, 128, 100, cur, 0);
        float bob = cur ? sinf(t_fase * 4) * 3 - mola_px() : 0;
        desenha_amuleto(x, 156 + (int)bob, 26, a, false);
        txt_c(x, 186, AMULETO[a].nome, cur ? AMULETO[a].cor : C_MARFIM_S, true);
        if (nome_rar_am(a)[0]) ui_selo_c(x, 205, nome_rar_am(a), cor_rar_am(a), GFX_MIUDO);
        zona_clique(x - 64, 124, 128, 100, &cursor, i, KEY_ENTER);
    }
    if (cursor >= 0 && cursor < 3 && run.am_oferta[cursor] < A_N)
        txt_c(GFX_W / 2, 234, AMULETO[run.am_oferta[cursor]].desc, C_MARFIM, false);
    // Os que ja tem, miudos, embaixo.
    int meus[A_N], na = meus_amuletos(meus);
    int lugares = am_lugares();
    snprintf(s, sizeof s, "seus amuletos  ·  %d de %d", na, lugares);
    txt_c(GFX_W / 2, 258, s, C_TEXTO_M, false);
    int x0 = GFX_W / 2 - (lugares - 1) * 12;
    for (int i = 0; i < lugares; i++) {
        if (i < na) desenha_amuleto(x0 + 24 * i, 288, 9, meus[i], false);
        else {                                       // lugar vazio: so o aro
            gfx_disco(x0 + 24 * i, 288, 9, C_FELTRO2);
            gfx_disco(x0 + 24 * i, 288, 7, C_FELTRO_ESC);
        }
    }
    rodape_run(selo ? "ENTER leva o amuleto" : "setas escolhem  ·  ENTER leva o amuleto");
}
