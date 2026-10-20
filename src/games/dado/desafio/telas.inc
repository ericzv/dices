// ---------------------------------------------------------------------------
// Perfis e dificuldades
// ---------------------------------------------------------------------------
static const char *const NOME_DIF[N_NIVEIS + 1] = { "Normal", "Desafiante - Nível 1", "Desafiante - Nível 2",
    "Desafiante - Nível 3", "Desafiante - Nível 4", "Desafiante - Nível 5", "Desafiante - Nível 6",
    "Desafiante - Nível 7", "Desafiante - Nível 8", "Desafiante - Nível 9", "Desafiante - Nível 10" };
static const char *const NOME_DIF_CURTO[N_NIVEIS + 1] = { "Normal", "Nível 1", "Nível 2", "Nível 3", "Nível 4",
    "Nível 5", "Nível 6", "Nível 7", "Nível 8", "Nível 9", "Nível 10" };
// Cada dificuldade e um andar do cassino, mais chique a cada um: a mesa
// mostra o andar nos acabamentos (veja pinta_mesa).
static const char *const ANDAR_DIF[N_NIVEIS + 1] = { "Térreo", "1º Andar", "2º Andar", "3º Andar", "4º Andar",
    "5º Andar", "6º Andar", "7º Andar", "8º Andar", "9º Andar", "Cobertura" };
static const uint16_t COR_DIF[N_NIVEIS + 1] = { C_MARFIM, C_BRONZE, RGB(210, 120, 80), C_PRATA, RGB(110, 190, 150),
    C_OURO, RGB(232, 150, 60), RGB(222, 70, 92), RGB(176, 110, 210), RGB(90, 140, 230), RGB(150, 220, 240) };
// O que cada dificuldade acrescenta (vale junto com as de baixo): uma coisa
// por nivel, para a subida ser aos poucos.
static const char *const PENA_DIF[N_NIVEIS + 1] = {
    "Os salões como sempre; só os chefes têm trunfo.",
    "O Crupiê passa a virar na 5ª rodada.",
    "A Dona também vira na 5ª rodada.",
    "A loja fica 25% mais cara.",
    "O Barão também vira na 5ª rodada.",
    "As mesas difíceis pensam mais.",
    "Oponentes trazem 1 dado raro a mais.",
    "Viradas do Crupiê e da Dona mais fortes.",
    "Oponentes trazem 2 dados raros a mais.",
    "Barão vira mais forte; chefes pensam mais.",
    "Os oponentes ganham um amuleto.",
};

static const char *nome_dif(int d) { return NOME_DIF_CURTO[d <= N_NIVEIS ? d : 0]; }

// O nome do oponente sem o artigo e so com a inicial maiuscula ("Caubói").
static void nome_curto(const char *nome, char *s, int n)
{
    if ((nome[0] == 'O' || nome[0] == 'A') && nome[1] == ' ') nome += 2;
    int k = 0;
    for (int i = 0; nome[i] && k < n - 1; i++) {
        unsigned char c = (unsigned char)nome[i];
        if (i > 0 && c >= 'A' && c <= 'Z') c += 32;
        else if (i > 0 && c >= 0x80 && c <= 0x9E && (unsigned char)nome[i - 1] == 0xC3) c += 0x20;   // Á, Ã, Ó...
        s[k++] = (char)c;
    }
    s[k] = 0;
}

// Os oponentes comuns que passam a trazer o trunfo na dificuldade d:
// "Cautelosa, Apostador e Caubói". Devolve quantos.
static int trunfos_do_nivel(int d, char *s, int n)
{
    int q = 0, tot = 0;
    for (int p = 0; p < N_PERSO; p++) tot += !perso_chefe(p) && PERSO[p].andar_trunfo == d;
    s[0] = 0;
    for (int p = 0; p < N_PERSO; p++) {
        if (perso_chefe(p) || PERSO[p].andar_trunfo != d) continue;
        char nm[24];
        nome_curto(PERSO[p].nome, nm, sizeof nm);
        int l = (int)strlen(s);
        snprintf(s + l, (size_t)(n - l), "%s%s", q == 0 ? "" : q == tot - 1 ? " e " : ", ", nm);
        q++;
    }
    return q;
}
static uint16_t cor_dif(int d) { return COR_DIF[d <= N_NIVEIS ? d : 0]; }

static int  pf_cur;                      // card em foco na tela de perfis
static bool pf_apagando;                 // pedindo confirmacao para apagar
static int  pf_avatar;                   // tela de criar perfil
static char pf_nome[11];
static int  dif_cur;

// O que abre com a dificuldade n: dois amuletos e ate quatro dados.
#define LIB_DADOS 8
static void liberados(int n, int *am, int *dado)
{
    int k = 0;
    for (int a = 0; a < A_N && k < 2; a++) if (NIVEL_AMULETO[a] == n && amuleto_no_jogo(a)) am[k++] = a;
    for (; k < 2; k++) am[k] = -1;
    k = 0;
    for (int t = 0; t < D_N && k < LIB_DADOS; t++) if (nivel_do_dado(t) == n && marco_do_dado(t) < 0) dado[k++] = t;
    for (; k < LIB_DADOS; k++) dado[k] = -1;
}

static void des_abre_perfis(void)
{
    perfis_le();
    pf_cur = perfil_atual >= 0 ? perfil_atual : 0;
    pf_apagando = false;
    t_fase = 0;
    fase = F_PERFIS;
}

// Nova run: com dificuldades abertas, pergunta qual antes.
static void des_escolhe_nova(void)
{
    if (perfil()->nivel > 0) {
        dif_cur = perfil()->nivel;
        t_fase = 0;
        fase = F_RDIFIC;
        return;
    }
    des_dif = 0;
    des_nova_run();
}

static void des_entra_perfil(int i)
{
    perfil_atual = i;
    perfis_salva();
    estilo_do_perfil();                          // o feltro, a mesa e os dados dele
    if (des_tem_run()) {                         // continuar ou comecar outra
        menu_tela = MT_DESAFIO;
        menu_cur = 0;
        t_fase = 1.0f;
        fase = F_ABERTURA;
        return;
    }
    des_escolhe_nova();
}

static int avatar_retrato(void) { return perfil_atual >= 0 && perfis[perfil_atual].nome[0] ? RETRATO_AVATAR + perfil()->avatar : -1; }

static void moldura_retrato(int x, int y, int esc, int av, float t, bool cur)
{
    gfx_rect(x - 3, y - 3, RT_W * esc + 6, RT_H * esc + 6, cur ? C_OURO : C_LATAO_ESC);
    gfx_rect(x - 1, y - 1, RT_W * esc + 2, RT_H * esc + 2, C_BARRA);
    gfx_rect(x, y, RT_W * esc, RT_H * esc, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(x, y, esc, RETRATO_AVATAR + av, t, false, 0, RT_H);
}

static void faixa_titulo(const char *s)
{
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(botao_voltar(KEY_ESC), ty, "MODO DESAFIANTE", C_LATAO, true);
    txt_c(GFX_W / 2, ty, s, C_MARFIM, true);
}

static void desenha_perfis(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_titulo("Quem vai jogar?");
    for (int i = 0; i < N_PERFIS; i++) {
        const perfil_t *p = &perfis[i];
        int w = 170, h = 190, x = GFX_W / 2 + (i - 1) * 186 - w / 2, y = 66;
        bool cur = i == pf_cur;
        ui_cartao(x, y, w, h, cur, 0);
        zona_clique(x, y, w, h, &pf_cur, i, KEY_ENTER);
        if (!p->nome[0]) {
            int cx = x + w / 2, cy = y + 84;
            uint16_t c = cur ? C_OURO : C_TEXTO_M;
            gfx_rect(cx - 14, cy - 2, 28, 4, c);
            gfx_rect(cx - 2, cy - 14, 4, 28, c);
            txt_c(cx, y + 120, "novo perfil", c, true);
            continue;
        }
        int esc = 3, rx = x + (w - RT_W * esc) / 2, ry = y + 14;
        moldura_retrato(rx, ry, esc, p->avatar, t_fase + i * 2.1f, cur);
        txt_c(x + w / 2, ry + RT_H * esc + 8, p->nome, cur ? C_OURO : C_MARFIM, true);
        char s[48];
        int nv = p->nivel;
        txt_c(x + w / 2, ry + RT_H * esc + 26, NOME_DIF[nv], nv ? COR_DIF[nv] : C_MARFIM_S, false);
        snprintf(s, sizeof s, "%d %s", p->partidas, p->partidas == 1 ? "partida" : "partidas");
        txt_c(x + w / 2, ry + RT_H * esc + 42, s, C_TEXTO_M, false);
        int ganhou = 0;
        for (int d = 0; d <= N_NIVEIS; d++) ganhou += p->venceu[d];
        if (ganhou) {
            snprintf(s, sizeof s, "Barão vencido: %d", ganhou);
            txt_c(x + w / 2, ry + RT_H * esc + 58, s, C_OURO, false);
        }
    }
    if (pf_apagando && perfis[pf_cur].nome[0]) {
        char s[80];
        snprintf(s, sizeof s, "Apagar %s e a run guardada?", perfis[pf_cur].nome);
        txt_c(GFX_W / 2, 266, s, C_VINHO_CLR, true);
        rodape_run("ENTER ou X apaga  ·  ESC deixa como está");
    } else {
        if (perfis[pf_cur].nome[0]) {
            int bx = GFX_W / 2 + (pf_cur - 1) * 186 - 104;
            ui_botao(bx, 264, 64, 22, "estilo", 0, 0);
            zona_clique(bx, 264, 64, 22, NULL, 0, 'e');
            ui_botao(bx + 70, 264, 80, 22, "histórico", 0, 0);
            zona_clique(bx + 70, 264, 80, 22, NULL, 0, 'h');
            ui_botao(bx + 156, 264, 60, 22, "apagar", C_VINHO, 0);
            zona_clique(bx + 156, 264, 60, 22, NULL, 0, 'x');
        }
        rodape_run("setas escolhem  ·  ENTER entra  ·  E estilo  ·  H histórico  ·  X apaga  ·  ESC volta");
    }
}

static void desenha_cria_perfil(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_titulo("Novo perfil");
    int esc = 4, rx = GFX_W / 2 - RT_W * esc / 2, ry = 58;
    moldura_retrato(rx, ry, esc, pf_avatar, t_fase, true);
    // Setas para trocar o retrato.
    for (int lado = -1; lado <= 1; lado += 2) {
        int cx = GFX_W / 2 + lado * (RT_W * esc / 2 + 34), cy = ry + RT_H * esc / 2;
        float o[6] = { (float)(cx + lado * 8), (float)cy, (float)(cx - lado * 6), (float)(cy - 10),
                       (float)(cx - lado * 6), (float)(cy + 10) };
        float vx[3] = { o[0], o[2], o[4] }, vy[3] = { o[1], o[3], o[5] };
        poligono(vx, vy, 3, C_LATAO);
        zona_clique(cx - 16, cy - 18, 32, 36, NULL, 0, lado < 0 ? KEY_LEFT : KEY_RIGHT);
    }
    int y = ry + RT_H * esc + 8;
    txt_c(GFX_W / 2, y, NOME_AVATAR[pf_avatar], C_OURO, true);
    // O nome, digitado.
    int bw = 180, bx = GFX_W / 2 - bw / 2, by = y + 22;
    gfx_rect(bx + 1, by + 1, bw - 2, 18, C_PLACA);
    ui_contorno(bx, by, bw, 20, C_LATAO);
    char s[16];
    bool pisca = fmodf(t_fase, 1.0f) < 0.55f;
    snprintf(s, sizeof s, "%s%s", pf_nome, pisca && strlen(pf_nome) < 10 ? "_" : " ");
    if (pf_nome[0] || pisca) txt_c(GFX_W / 2, by + 2, s, C_MARFIM, true);
    if (!pf_nome[0] && !pisca) txt_c(GFX_W / 2, by + 2, "seu nome", C_TEXTO_M, false);
    // Botao de criar.
    int ok = pf_nome[0] != 0;
    ui_botao(GFX_W / 2 - 50, by + 28, 100, 24, "criar", C_LATAO, ok ? 0 : UI_DESLIGADO);
    zona_clique(GFX_W / 2 - 50, by + 30, 100, 20, NULL, 0, KEY_ENTER);
    rodape_run("setas trocam o retrato  ·  digite o nome  ·  ENTER cria  ·  ESC volta");
}

static void desenha_dificuldade(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_titulo("Dificuldade da run");
    int nv = perfil()->nivel, ult = nv < N_NIVEIS ? nv + 1 : N_NIVEIS;
    int x0 = 28, y0 = 54;
    for (int d = 0; d <= ult; d++) {
        bool aberta = d <= nv, cur = d == dif_cur;
        int y = y0 + d * 22;
        ui_cartao(x0, y, 150, 20, cur, 0);
        uint16_t c = aberta ? COR_DIF[d] : C_FELTRO2;
        gfx_disco(x0 + 14, y + 10, 6, gfx_mistura(c, C_BARRA, 30));
        gfx_disco(x0 + 14, y + 10, 4, c);
        txt(x0 + 28, y, NOME_DIF_CURTO[d], aberta ? c : C_TEXTO_M, true);
        if (perfil()->venceu[d]) {                  // medalha: o Barao ja caiu nesta
            gfx_disco(x0 + 134, y + 10, 5, C_LATAO_ESC);
            gfx_disco(x0 + 134, y + 10, 4, C_OURO);
            gfx_disco(x0 + 133, y + 9, 1, C_BRANCO);
        }
        if (aberta) zona_clique(x0, y, 150, 20, &dif_cur, d, KEY_ENTER);
        else zona_clique(x0, y, 150, 20, &dif_cur, d, 0);
    }
    // O que vale na escolhida: o que ela acrescenta, em destaque, e o que vem
    // de baixo, miudo.
    int px = 194, py = 54;
    ui_painel(px, py, 416, 240);
    int d = dif_cur;
    int wt = gfx_texto(px + 12, py + 8, NOME_DIF[d], COR_DIF[d], 1, true);
    txt(px + 12 + wt + 12, py + 8, ANDAR_DIF[d], C_TEXTO_M, false);
    int ly = py + 30;
    if (d > nv) {
        char s[64];
        snprintf(s, sizeof s, "Vença o Barão no %s para abrir.", NOME_DIF_CURTO[d - 1]);
        txt(px + 12, ly, s, C_VINHO_CLR, false);
        ly += 20;
    }
    txt(px + 12, ly, "+", COR_DIF[d], true);
    txt(px + 24, ly, PENA_DIF[d], C_MARFIM, false);
    ly += 19;
    for (int k = d - 1, i = 0; k >= 1; k--, i++)    // as de baixo, da mais recente, em duas colunas
        gfx_texto(px + 26 + (i % 2) * 196, ly + 2 + (i / 2) * 11, PENA_DIF[k], C_TEXTO_M, GFX_MIUDO, false);
    if (d > 1) ly += (d / 2) * 11 + 4;
    if (d > 0) {                                     // quem passa a trazer o trunfo
        char nomes[96], t[128];
        if (trunfos_do_nivel(d, nomes, sizeof nomes)) {
            snprintf(t, sizeof t, "Com trunfo agora: %s.", nomes);
            txt(px + 24, ly, t, C_ROSA, false);
            ly += 17;
        }
    }
    {
        char m[64];                                  // o mesmo comeco em todo andar
        snprintf(m, sizeof m, "Você começa com %d moedas e %d dados.", moedas_iniciais(d), (int)sizeof BOLSA_INICIO);
        txt(px + 24, ly, m, C_VERDE, false);
        ly += 17;
    }
    if (d > 0) {                                     // o amuleto, os dados e o adesivo que abrem
        int am[2], dado[LIB_DADOS];
        liberados(d, am, dado);
        int it[2 + LIB_DADOS + AD_N], ni = 0;            // amuleto: -1 - a; dado: o tipo; adesivo: ADE + a
        enum { ADE = 1000 };
        for (int i = 0; i < 2; i++) if (am[i] >= 0) it[ni++] = -1 - am[i];
        for (int i = 0; i < LIB_DADOS; i++) if (dado[i] >= 0) it[ni++] = dado[i];
        for (int a = AD_NADA + 1; a < AD_N; a++) if (ADESIVO[a].nivel == d) it[ni++] = ADE + a;
        ly += 4;
        txt(px + 12, ly, d <= nv ? "Abriu para você:" : "Abre:", C_TEXTO_M, false);
        ly += 24;
        int ix = px + 24, iy = ly;
        for (int i = 0; i < ni; i++) {              // em fluxo: cada um com a largura do seu nome
            const char *nome = it[i] < 0 ? AMULETO[-1 - it[i]].nome : it[i] >= ADE ? ADESIVO[it[i] - ADE].nome : TIPO[it[i]].nome;
            int wi = 14 + gfx_largura(nome, GFX_FINO) + 16;
            if (ix + wi > px + 408) { ix = px + 24; iy += 24; }
            if (it[i] < 0) {
                int a = -1 - it[i];
                desenha_amuleto(ix, iy, 9, a, d > nv);
                txt(ix + 14, iy - 6, nome, d > nv ? C_TEXTO_M : AMULETO[a].cor, false);
                if (d <= nv) dica_zona(ix - 10, iy - 10, wi, 20, DICA_AMULETO, a);
            } else if (it[i] >= ADE) {
                int a = it[i] - ADE;
                ad_solto_d(ix, iy, a, 1, 0);
                txt(ix + 14, iy - 6, nome, d > nv ? C_TEXTO_M : ADESIVO[a].luz, false);
                if (d <= nv) dica_zona(ix - 10, iy - 10, wi, 20, DICA_ADESIVO, a);
            } else {
                desenha_dado((float)ix, (float)iy, 9, it[i], 0, 0, 0, 0, d > nv);
                txt(ix + 14, iy - 6, nome, d > nv ? C_TEXTO_M : cor_dado(it[i]), false);
                if (d <= nv) dica_zona(ix - 10, iy - 10, wi, 20, DICA_DADO, it[i]);
            }
            ix += wi;
        }
    }
    rodape_run("setas escolhem  ·  ENTER começa a run  ·  ESC volta");
}

