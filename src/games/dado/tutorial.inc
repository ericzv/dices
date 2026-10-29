// ---------------------------------------------------------------------------
// Tutorial
// ---------------------------------------------------------------------------
// Cada pagina: o titulo, uma mini-mesa animada mostrando a regra (dados de
// verdade, desenhados pelo mesmo codigo da mesa) e tres linhas curtas na
// fonte grande; a nota, na fina, so quando precisa de numero ou detalhe.
typedef struct { const char *titulo; const char *lin[3]; const char *nota; } pagina_t;
static const pagina_t TUTORIAL[] = {
    { "O JOGO", {
        "A cada rodada, os dois pagam a entrada no pote.",
        "Quem faz mais pontos na rodada leva o pote.",
        "No fim, ganha quem tiver mais fichas." },
        "2 jogadores: 100 fichas e 15 rodadas; a entrada é de 5, de 10 a partir da 6ª e de 15 a partir da 11ª." },
    { "A BOLSA E A SEQUÊNCIA", {
        "A cada rodada, você compra 7 dados da sua bolsa,",
        "escolhe até 4 para a sequência, na ordem que quiser,",
        "e joga um por um, do primeiro ao último." },
        NULL },
    { "A REGRA DA SEQUÊNCIA", {
        "O primeiro dado vale. Cada um depois precisa ser",
        "igual ou maior que o último válido. Saiu menor:",
        "ele e o último válido são anulados juntos." },
        "Por isso a ordem importa: dados pequenos na frente, grandes no fim." },
    { "PARAR OU ARRISCAR", {
        "Depois de cada dado: jogar o próximo ou parar.",
        "Jogar mostra o risco de anular; parar, quanto falta.",
        "Quem joga depois sabe o alvo: passou, a rodada é sua." },
        "Quem venceu a rodada anterior abre a próxima, no escuro. O risco some nas dificuldades altas." },
    { "APOSTAS", {
        "Entre os turnos, dá para apostar 10 ou 25 fichas.",
        "O outro paga para ver, aumenta ou corre:",
        "quem corre entrega o pote na hora." },
        "Se quem abriu termina com 0, nem tem aposta: o outro leva o pote sem jogar." },
    { "LOJA, PRÊMIOS E DADOS ESPECIAIS", {
        "Quem vence uma rodada escolhe um prêmio:",
        "um dado novo, ou tirar um fraco da bolsa.",
        "Cada dado especial tem um poder." },
        "São %d dados: veja todos em Dados da casa. A loja do começo também vende dados especiais." },
    { "MODO DESAFIANTE", {
        "Uma run por 3 salões: dois oponentes e o chefe.",
        "Venceu a mesa: moedas e a loja. Perdeu: a run acaba.",
        "Chefes vencidos abrem bolsas, dados e dificuldades." },
        "8 rodadas e 50 fichas por partida. Antes, você escolhe a bolsa. Cada perfil guarda o seu progresso." },
    { "TECLAS", {
        "Mouse e toque também jogam: clique no que quiser.",
        "O < no alto volta, ou sai da partida.",
        "Boa sorte!" },
        NULL },
};
#define N_TUTORIAL ((int)(sizeof TUTORIAL / sizeof TUTORIAL[0]))

// A mini-mesa: uma janela do feltro, sem o escuro da pagina, com a moldura
// de latao em rampa.
#define TUT_X 56
#define TUT_Y 96
#define TUT_W 528
#define TUT_H 114
#define TUT_CX (TUT_X + TUT_W / 2)
#define TUT_CY (TUT_Y + TUT_H / 2)

// Um dado que cai na mini-mesa: t e o tempo desde que foi lancado (< 0: nao
// apareceu). Cai girando e trocando de face, quica e assenta no valor.
static void tut_dado(int x, int y, float r, int tipo, int valor, float t, bool anulado)
{
    if (t < 0) return;
    if (t < 0.35f) {
        float q = t / 0.35f;
        int v = 1 + (int)(t * 23) % (TIPO[tipo].lados > 2 ? TIPO[tipo].lados : 2);
        desenha_dado((float)x, y - (1 - q) * 40, r, tipo, 0, (1 - q) * 2.5f, v, 2, false);
        return;
    }
    float q = (t - 0.35f) / 0.25f, pulo = q < 1 ? sinf(q * 3.1416f) * 5 : 0;
    desenha_dado((float)x, y - pulo, r, tipo, 0, 0, valor, 2, anulado);
}

// Uma ficha voando em arco de (x0, y0) a (x1, y1); q de 0 a 1.
static void tut_ficha_voa(float x0, float y0, float x1, float y1, float q, uint16_t c)
{
    if (q <= 0 || q >= 1) return;
    float e = q * q * (3 - 2 * q);
    ficha((int)(x0 + (x1 - x0) * e), (int)(y0 + (y1 - y0) * e - sinf(q * 3.1416f) * 26), c);
}
static void tut_pilha(int x, int y, int n, uint16_t a, uint16_t b)
{
    for (int i = 0; i < n; i++) ficha(x, y - i * 4, i % 2 ? b : a);
}

// Um botao de mentira, com o cursor (cantoneiras) quando aceso.
static void rotulo_tut(int cx, int y, const char *s, bool aceso, uint16_t tinta)
{
    int w = gfx_largura(s, 1) + 24;
    ui_botao(cx - w / 2, y, w, 26, s, tinta, aceso ? UI_FOCO : 0);
}
// Um botao da aposta de mentira: o icone e o rotulo, como na mesa.
static int tut_aposta(int x, int y, int op, const char *rot, bool aceso)
{
    int w = 10 + icone_aposta_w(op) + 6 + gfx_largura(rot, 1) + 12, d;
    uint16_t c = ui_botao_face(x, y, w, BOTAO_AP_H, tinta_op(op), aceso ? UI_FOCO : 0, &d);
    if (op == OP_CORRER) c = C_ROSA;
    icone_aposta(op, x + 10, y + d + (BOTAO_AP_H - 4) / 2);
    gfx_texto(x + 10 + icone_aposta_w(op) + 6, y + d + (BOTAO_AP_H - 4 - TXT_H) / 2 + 1, rot, c, 1, false);
    return w;
}

static void tut_cena_desafio(int cx, int cy, float t);    // desafio/desenho.inc

// As cenas: t corre desde que a pagina abriu e cada cena repete no seu ciclo.
static void ilustra_tutorial(int pag, float t)
{
    int cx = TUT_CX, cy = TUT_CY;
    switch (pag) {
    case 0: {                                        // a entrada vai ao pote; o pote vai a quem levou
        float c = fmodf(t, 4.2f);
        int xv = cx - 170, xr = cx + 170, yb = cy + 22;
        bool pagou = c > 0.9f, levou = c > 2.6f;
        tut_pilha(xv, yb, (pagou ? 6 : 7) + (levou ? 4 : 0), C_LATAO, C_VINHO);
        tut_pilha(xr, yb, pagou ? 6 : 7, C_ROSA, C_MARFIM_S);
        if (c < 0.9f) {                              // a entrada: uma de cada lado
            tut_ficha_voa((float)xv, yb - 24.0f, (float)cx - 8, (float)yb, (c - 0.1f) / 0.7f, C_LATAO);
            tut_ficha_voa((float)xr, yb - 24.0f, (float)cx + 8, (float)yb, (c - 0.2f) / 0.7f, C_ROSA);
        } else if (c < 1.8f) {
            tut_pilha(cx - 8, yb, 2, C_LATAO, C_VINHO);
            tut_pilha(cx + 8, yb, 2, C_ROSA, C_MARFIM_S);
        } else if (c < 2.6f) {                       // quem fez mais pontos leva
            for (int i = 0; i < 4; i++) tut_ficha_voa((float)cx, (float)yb - 6, (float)xv, yb - 40.0f, (c - 1.8f - i * 0.08f) / 0.55f,
                                                      i % 2 ? C_ROSA : C_LATAO);
        } else if (c < 3.6f) txt_sombra_c(xv, yb - 78 - (int)((c - 2.6f) * 8), "+4", C_OURO, 1);
        if (c >= 0.9f && c < 1.8f) {
            painel(cy - 46, TXT_H + 6, 200);
            txt_c(cx, cy - 44, "rodada: 9 x 7", C_MARFIM, true);
        }
        txt_c(xv, yb + 12, "você", C_LATAO, false);
        txt_c(cx, yb + 12, "pote", C_TEXTO_M, false);
        txt_c(xr, yb + 12, "rival", C_ROSA, false);
        break;
    }
    case 1: {                                        // da bolsa para a mao, da mao para a fila
        static const uint8_t M[7] = { D_D6, D_D2, D_D6, D_D4, D_D8, D_D6, D_D2 };
        static const int8_t ORDEM[4] = { 1, 3, 0, 4 };      // a ordem escolhida
        float c = fmodf(t, 5.6f);
        int bx = TUT_X + 34, by = cy + 26;
        icone_bolsa(bx, by);
        int na_fila = c < 1.5f ? 0 : (int)((c - 1.5f) / 0.5f) + 1;
        if (na_fila > 4) na_fila = 4;
        for (int i = 0; i < 7; i++) {
            float q = (c - 0.15f - i * 0.12f) / 0.3f;
            if (q <= 0) continue;
            int x = cx - 150 + i * 44, y = cy + 30;
            int k = -1;
            for (int o = 0; o < 4; o++) if (ORDEM[o] == i) k = o;
            if (k >= 0 && k < na_fila) {             // saiu da mao: voa para o lugar dele
                float v = (c - 1.5f - k * 0.5f) / 0.35f;
                int lx = cx - 66 + k * 44, ly = cy - 26;
                if (v < 1) {
                    float e = v * v * (3 - 2 * v);
                    peca_parada(x + (lx - x) * e, y + (ly - y) * e - sinf(v * 3.1416f) * 18, 13, nova_peca(M[i]), 0);
                }
                continue;
            }
            float e = q < 1 ? q : 1;
            peca_parada(bx + (x - bx) * e, by + (y - by) * e - (q < 1 ? sinf(q * 3.1416f) * 12 : 0), 13, nova_peca(M[i]), 0);
        }
        for (int k = 0; k < 4; k++) {                // os lugares da fila
            int lx = cx - 66 + k * 44, ly = cy - 26;
            float v = (c - 1.5f - k * 0.5f) / 0.35f;
            if (k < na_fila && v >= 1) peca_parada((float)lx, (float)ly, 13, nova_peca(M[ORDEM[k]]), 0);
            else silhueta_dado((float)lx, (float)ly, 12, 6, k == na_fila ? C_OURO : C_FELTRO2, 0.22f);
            plaqueta_num(lx, ly + 24, k, k < na_fila ? C_LATAO : k == na_fila ? C_OURO : C_EBANO_B,
                         k <= na_fila ? C_BARRA : C_TEXTO_M, false);
        }
        break;
    }
    case 2: {                                        // 2 vale, 4 vale (>= 2), sai 3: anula o 3 e o 4
        static const int V[3] = { 2, 4, 3 };
        float c = fmodf(t, 5.6f);
        bool cai = c > 3.0f;
        for (int i = 0; i < 3; i++) {
            int x = cx - 110 + i * 110;
            float ti = c - 0.2f - i * 1.1f;
            bool anul = cai && i > 0;
            tut_dado(x, cy - 8, 22, D_D6, V[i], ti, anul);
            if (ti > 0.6f) {
                const char *r = anul ? (i == 2 ? "menor: anula" : "anulado") : "válido";
                txt_c(x, cy + 24, r, anul ? C_VINHO_CLR : C_OURO, true);
            }
        }
        if (c > 1.9f && !cai) {                      // o "maior ou igual" entre os dois validos
            txt_sombra_c(cx - 55, cy - 24, ">", C_OURO, 1);
            gfx_rect(cx - 59, cy - 4, 8, 2, C_OURO);
        }
        if (cai) txt_sombra_c(cx + 55, cy - 24, "<", C_VINHO_CLR, 1);
        break;
    }
    case 3: {                                        // 6 pontos, alvo 7: parar perde, jogar arrisca
        float c = fmodf(t, 6.0f);
        desenha_dado((float)cx - 220, (float)cy - 6, 18, D_D6, 0, 0, 2, 2, false);
        desenha_dado((float)cx - 176, (float)cy - 6, 18, D_D6, 0, 0, 4, 2, false);
        bool lancou = c > 2.5f, deu = c > 2.85f;
        tut_dado(cx - 132, cy - 6, 18, D_D6, 5, c - 2.5f, false);
        int pts = deu ? 11 : 6;
        char s[16];
        snprintf(s, sizeof s, "%d", pts);
        ui_painel(cx - 92, cy - 30, 56, 46);
        txt_c2(cx - 64, cy - 26, s, deu ? C_OURO : C_MARFIM);
        if (deu) ui_plaqueta(cx - 64, cy + 8, "VENCE", C_OURO);
        else txt_c(cx - 64, cy + 18, "alvo 7", C_TEXTO_M, false);
        if (!lancou) {
            bool em_jogar = c > 1.3f;
            rotulo_tut(cx + 70, cy - 30, "jogar · 50% de risco", em_jogar, C_VINHO);
            rotulo_tut(cx + 70, cy + 4, "parar · perde por 1", !em_jogar, 0);
        }
        break;
    }
    case 4: {                                        // abrir a aposta; depois responder a ela
        float c = fmodf(t, 6.4f);
        bool resposta = c > 3.2f;
        float d = resposta ? c - 3.2f : c;
        int k = d < 1.0f ? 0 : 1;                    // o cursor anda para apostar 10 / pagar
        int op[3], n = 3;
        const char *rot[3];
        if (!resposta) { op[0] = OP_PASSAR; op[1] = OP_APOSTAR10; op[2] = OP_APOSTAR25; rot[0] = "passar"; rot[1] = "apostar 10"; rot[2] = "apostar 25"; }
        else { op[0] = OP_CORRER; op[1] = OP_PAGAR; op[2] = OP_AUMENTAR; rot[0] = "correr"; rot[1] = "pagar 10"; rot[2] = "aumentar"; }
        int w = 0;
        for (int i = 0; i < n; i++) w += 10 + icone_aposta_w(op[i]) + 6 + gfx_largura(rot[i], 1) + 12 + (i ? 24 : 0);
        int x = cx - w / 2, y = cy - 40;
        for (int i = 0; i < n; i++) x += tut_aposta(x, y, op[i], rot[i], i == k && d < 2.2f) + 24;
        int px = cx, py = cy + 26;
        int no_pote = 2 + (d > 1.9f ? 2 : 0) + (resposta ? 2 : 0);
        tut_pilha(px, py, no_pote, C_LATAO, C_ROSA);
        for (int i = 0; i < 2; i++)                  // as fichas da aposta indo ao pote
            tut_ficha_voa(resposta ? cx + 170.0f : cx - 170.0f, py - 4.0f, (float)px, py - 8.0f - no_pote * 4,
                          (d - 1.4f - i * 0.1f) / 0.45f, resposta ? C_LATAO : C_ROSA);
        txt_c(px, py + 8, "pote", C_TEXTO_M, false);
        break;
    }
    case 5: {                                        // dados especiais, com o poder armado
        static const uint8_t E[4] = { D_LASER, D_CARIDOSO, D_ESCUDO, D_JACKPOT };
        for (int i = 0; i < 4; i++) {
            int x = cx - 165 + i * 110, y = cy - 6 + (int)(sinf(t * 2.4f + i) * 2);
            peca_t pc = nova_peca(E[i]);
            brilho_poder((float)x, (float)y, 20, E[i], 0, true);
            peca_parada((float)x, (float)y, 20, pc, 0);
            anima_poder(x, y, 20, E[i], i * 0.37f);
            txt_c(x, cy + 26, TIPO[E[i]].nome, cor_dado(E[i]), true);
        }
        break;
    }
    case 6:                                          // o salao: as tres mesas e o caminho
        tut_cena_desafio(cx, cy, t);
        break;
    default: {                                       // as teclas, em duas colunas
        static const char *const K[2][4] = {
            { "setas movem", "ENTER escolhe e joga", "TAB confirma ou para", "BACKSPACE tira" },
            { "I mostra o dado", "X recusa", "ESC duas vezes: menu", "V velocidade  ·  M som" } };
        for (int col = 0; col < 2; col++)
            for (int i = 0; i < 4; i++) linha_teclas(TUT_X + 40 + col * 250, TUT_Y + 16 + i * 26, K[col][i], C_MARFIM_S, true);
        break;
    }
    }
}

static void desenha_tutorial(void)
{
    char s[32];
    snprintf(s, sizeof s, "%d de %d", tut_pag + 1, N_TUTORIAL);
    faixas_menu("TUTORIAL", s, tut_pag + 1 < N_TUTORIAL
                ? "< anterior  ·  ENTER ou > próxima  ·  ESC menu"
                : "< anterior  ·  ENTER volta ao menu");
    // O escuro da pagina em volta da mini-mesa, que fica com o feltro aceso.
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, TUT_Y - 3 - MESA_Y0, C_SOMBRA, 110);
    gfx_rect_alfa(MESA_X0, TUT_Y + TUT_H + 3, MESA_X1 - MESA_X0, MESA_Y1 - TUT_Y - TUT_H - 3, C_SOMBRA, 110);
    gfx_rect_alfa(MESA_X0, TUT_Y - 3, TUT_X - 3 - MESA_X0, TUT_H + 6, C_SOMBRA, 110);
    gfx_rect_alfa(TUT_X + TUT_W + 3, TUT_Y - 3, MESA_X1 - TUT_X - TUT_W - 3, TUT_H + 6, C_SOMBRA, 110);
    uint16_t rm[4];
    rampa_de(C_LATAO, rm);
    ui_contorno(TUT_X - 3, TUT_Y - 3, TUT_W + 6, TUT_H + 6, C_BARRA);
    ui_aro(TUT_X - 2, TUT_Y - 2, TUT_W + 4, TUT_H + 4, 2, rm, UI_VIVO);
    const pagina_t *pg = &TUTORIAL[tut_pag];
    txt_sombra_c(GFX_W / 2, MESA_Y0 + 4, pg->titulo, C_LATAO, 2);
    ilustra_tutorial(tut_pag, t_fase);
    int y = TUT_Y + TUT_H + 4;
    for (int i = 0; i < 3 && pg->lin[i]; i++) {      // as tres linhas, na fonte grande
        gfx_texto(GFX_W / 2 - gfx_largura(pg->lin[i], 1) / 2, y, pg->lin[i], i == 0 ? C_MARFIM : C_MARFIM_S, 1, false);
        y += 19;
    }
    if (pg->nota) {                                  // o detalhe, na fina e apagado
        char nota[200];                              // "%d": quantos dados ha
        snprintf(nota, sizeof nota, pg->nota, n_basicos());
        int f = gfx_largura(nota, GFX_FINO) <= 584 ? GFX_FINO : GFX_MIUDO;
        gfx_texto(GFX_W / 2 - gfx_largura(nota, f) / 2, y - 1 + (f == GFX_MIUDO ? 4 : 0), nota, C_TEXTO_M, f, false);
    }
    // Bolinhas das paginas.
    for (int i = 0; i < N_TUTORIAL; i++)
        gfx_disco(GFX_W / 2 + (i - N_TUTORIAL / 2) * 16, MESA_Y1 - 5, 3,
                  i == tut_pag ? C_OURO : C_FELTRO2);
}
