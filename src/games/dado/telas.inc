// ===========================================================================
// Telas da partida: ordem, jogada, aposta, resultado, premio, loja
// e fim
// ===========================================================================
// Uma linha: "VOCÊ joga  ·  Moeda [comum]", e embaixo o que o dado faz.
static int larg_quem_e_dado(const char *quem, int tipo)
{
    char c[32];
    classe(tipo, c, sizeof c);
    int a = gfx_largura(quem, 1) + 20 + gfx_largura(TIPO[tipo].nome, 1) + 12 + gfx_largura(c, 1) + 10;
    int b = gfx_largura(TIPO[tipo].desc, 1);
    return a > b ? a : b;
}

static void quem_e_dado(int y, const char *quem, uint16_t cor, int tipo)
{
    char c[32];
    classe(tipo, c, sizeof c);
    int wq = gfx_largura(quem, 1), wn = gfx_largura(TIPO[tipo].nome, 1), wc = gfx_largura(c, 1) + 10;
    int x = GFX_W / 2 - (wq + 20 + wn + 12 + wc) / 2;
    gfx_texto(x, y, quem, cor, 1, true);
    txt_c(x + wq + 10, y, "·", C_TEXTO_M, false);
    x += wq + 20;
    gfx_texto(x, y, TIPO[tipo].nome, cor_dado(tipo), 1, true);
    int xc = x + wn + 12;
    ui_etiqueta(xc, y + 2, wc, TXT_H - 2, cor_rar(tipo));
    gfx_texto(xc + 5, y, c, cor_rar(tipo), 1, false);
    txt_c(GFX_W / 2, y + TXT_H - 2, TIPO[tipo].desc, C_MARFIM_S, false);
}

static void dado_e_poder(int y, int tipo, bool medio)
{
    // Nome e, ao lado, a etiqueta "D8 · raro" na cor da raridade.
    char c[32];
    classe(tipo, c, sizeof c);
    int wn = gfx_largura(TIPO[tipo].nome, 1), wc = gfx_largura(c, 1) + 10;
    int x = GFX_W / 2 - (wn + 12 + wc) / 2;
    gfx_texto(x, y, TIPO[tipo].nome, cor_dado(tipo), 1, true);
    int xc = x + wn + 12;
    ui_etiqueta(xc, y + 2, wc, TXT_H - 2, cor_rar(tipo));
    gfx_texto(xc + 5, y, c, cor_rar(tipo), 1, false);
    txt_c(GFX_W / 2, y + TXT_H - 1, medio ? TEXTO_MEDIO[tipo] : TIPO[tipo].desc, C_MARFIM_S, false);
}

// Tracinho discreto na cor da raridade, embaixo de um dado.
static void marca_rar(int x, int y, int tipo)
{
    gfx_rect(x - 7, y, 14, 2, cor_rar(tipo));
}

// Uma fileira de botoes. Com tinta, cada um tem a sua cor (a aposta em
// vermelho, correr em preto...), para reconhecer sem ler; o escolhido acende
// e ganha a moldura branca. Sem tinta, o escolhido fica na cor do jogador.
#define BOTAO_H 24
static int larg_botoes(const char **rot, int n)
{
    int larg = -12;
    for (int i = 0; i < n; i++) larg += gfx_largura(rot[i], 1) + 24 + 12;
    return larg;
}

static void botoes(int y, const char **rot, const bool *ativo, int n, const uint16_t *tinta)
{
    int x = GFX_W / 2 - larg_botoes(rot, n) / 2;
    for (int i = 0; i < n; i++) {
        int w = gfx_largura(rot[i], 1) + 24;
        bool cur = i == cursor && ativo[i];
        ui_botao(x, y, w, BOTAO_H, rot[i], tinta ? tinta[i] : 0, !ativo[i] ? UI_DESLIGADO : cur ? UI_FOCO : 0);
        if (ativo[i]) zona_clique(x, y, w, BOTAO_H, &cursor, i, KEY_ENTER);
        x += w + 12;
    }
}

// Os botoes de decisao (aposta, pronto): altura e icones, em sprite_em.
#define BOTAO_AP_H 28
static const char *const IC_FICHAS[] = { ".#######.", "#+++++++#", ".#######.", "#o#o#o#o#", ".#######.", "#o#o#o#o#", ".#######." };
static const char *const IC_SETA[] = { "...#...", "..###..", ".#####.", "###.###", "##...##" };
static const char *const IC_X[] = { "##...##", "###.###", ".#####.", "..###..", ".#####.", "###.###", "##...##" };
static const char *const IC_VISTO[] = { "......##", ".....##.", "##..##..", ".####...", "..##...." };

// A posicao na sequencia, numa plaquinha de metal com o algarismo gravado: a
// mesma embaixo dos lugares do painel e em cima do dado da mao que ja tem
// lugar. cx, cy e o meio; grande: 15 px, senao 13.
static void plaqueta_num(int cx, int cy, int k, uint16_t cor, uint16_t letra, bool grande)
{
    uint16_t r[4];
    rampa_de(cor, r);
    int s = grande ? 15 : 13, x = cx - s / 2, y = cy - s / 2;
    gfx_rect(x + 1, y + 1, s - 2, s - 2, r[2]);
    gfx_rect(x + 2, y + 1, s - 4, 1, r[3]);              // a luz na beira de cima
    gfx_rect(x + 1, y + s - 2, s - 2, 1, r[1]);          // e a sombra na de baixo
    ui_contorno(x, y, s, s, C_BARRA);
    char o[2] = { (char)('1' + k), 0 };
    numero_em(x + s / 2.0f, y + s / 2.0f, 13, o, letra, false);
}

// SUA VEZ: a plaquinha de ouro que desce na beira de cima do painel da
// sequencia quando chega a vez de uma pessoa (com um pulinho), fica um pouco
// e sai virando de lado, como placa que gira no prego. Com duas pessoas na
// mesa, diz de quem e a vez.
static float sai_mola(float q);          // desafio/desenho.inc
static void plaqueta_sua_vez(int cx, int y_beira)
{
    if (t_sua_vez <= 0 || fase != F_ORDEM) return;
    float e = SUA_VEZ_T - t_sua_vez - 0.12f;            // espera a mao chegar
    if (e < 0) return;
    char s[40];
    if (modo == M_DOIS) snprintf(s, sizeof s, "VEZ DE %s", J[vez].nome);
    else snprintf(s, sizeof s, "SUA VEZ");
    int f = fonte_de(true), W = gfx_largura(s, f) + 22, h = UI_PLAQUETA_H + 2;
    float q = t_sua_vez < 0.22f ? t_sua_vez / 0.22f : 1;
    int w = (int)(W * q);
    if (w < 4) return;
    int dy = e < 0.3f ? (int)((1 - sai_mola(e / 0.3f)) * -14) : 0;
    int x = cx - w / 2, y = y_beira - h / 2 - 2 + dy;
    uint16_t r[4];
    rampa_de(C_OURO, r);
    ui_sombra(x, y, w, h);
    gfx_rect(x + 1, y + 1, w - 2, h - 2, r[2]);
    gfx_rect(x + 2, y + 1, w - 4, 1, r[3]);              // a luz na beira de cima
    gfx_rect(x + 1, y + h - 2, w - 2, 1, r[1]);          // e a sombra na de baixo
    ui_contorno(x, y, w, h, C_BARRA);
    if (q < 1) return;
    gfx_rect(x + 4, y + h / 2 - 1, 2, 2, r[0]);          // os pregos
    gfx_rect(x + w - 6, y + h / 2 - 1, 2, 2, r[0]);
    gfx_texto(x + 11, y - 1, s, C_BARRA, f, true);
}

// O lugar de um dado que ainda nao chegou: a silhueta dele pontilhada, com a
// beira cheia.
static void silhueta_dado(float cx, float cy, float r, int lados, uint16_t cor, float dens)
{
    float vx[16], vy[16];
    int n = forma(lados == 2 ? 100 : lados, cx, cy, r, 0, vx, vy);
    poligono_dens(vx, vy, n, cor, dens);
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        gfx_linha((int)lroundf(vx[i]), (int)lroundf(vy[i]), (int)lroundf(vx[j]), (int)lroundf(vy[j]), cor);
    }
}

// Os lugares da fila de j no meio da mesa, 1 » 2 » 3 » 4: o dado que ja tem
// lugar, ou a silhueta pontilhada do que falta - o da vez em ouro, com a
// forma do dado em foco (lados_cur; 0: um D6) -, e embaixo de cada um a
// plaquinha com a posicao. Devolve onde o ultimo lugar termina, a direita.
#define LUGAR_PASSO 44
#define LUGAR_R     12
static int lugar_x(int cabe, int k) { return GFX_W / 2 + (2 * k - (cabe - 1)) * LUGAR_PASSO / 2; }
static int linha_lugares(int j) { return j == baixo ? CENTRO + 6 : CENTRO - 14; }
static int lugares_fila(int j, int ry, int lados_cur)
{
    const fila_t *f = &F[j];
    int cabe = max_fila(j);
    for (int k = 0; k < cabe; k++) {
        int x = lugar_x(cabe, k);
        if (k) txt_c(x - LUGAR_PASSO / 2, ry - 11, "»", C_FELTRO2, false);
        bool feito = k < f->n, proximo = k == f->n;
        if (feito) {
            const peca_t *pf = peca_fila(j, k);      // a da bolsa (com os adesivos); a carta do Coringa nao tem
            peca_t pc = pf ? *pf : (peca_t){ .tipo = f->tipo[k], .fixo = f->fixo[k] };
            pc.fixo = f->fixo[k];
            peca_parada((float)x, (float)ry, LUGAR_R, pc, j);
        } else if (proximo && lados_cur) {
            float pulsa = 0.3f + 0.12f * sinf(t_fase * 5);
            silhueta_dado((float)x, (float)ry, LUGAR_R, lados_cur, C_OURO, pulsa);
        } else silhueta_dado((float)x, (float)ry, LUGAR_R, 6, proximo ? C_OURO : C_FELTRO2, 0.22f);
        plaqueta_num(x, ry + LUGAR_R + 12, k, feito ? C_LATAO : proximo ? C_OURO : C_EBANO_B,
                     feito || proximo ? C_BARRA : C_TEXTO_M, false);
    }
    return lugar_x(cabe, cabe - 1) + LUGAR_R;
}

// Montagem da fila: a mao comprada aparece na linha do jogador, um dado de
// cada vez saindo da bolsa. A plaquinha dourada e a posicao na fila.
static void desenha_ordem(void)
{
    jogador_t *p = &J[vez];
    fila_t *f = &F[vez];
    int y = zona(vez), n = p->n_mao;
    for (int i = 0; i < n; i++) {
        float chega = i * 0.07f;
        if (t_fase < chega) continue;
        float q = (t_fase - chega) / 0.18f;
        if (q > 1) q = 1;
        int dy = (int)((1 - q) * (vez == baixo ? 40 : -40));   // vem do lado da bolsa
        int x = GFX_W / 2 + (int)((i - (n - 1) / 2.0f) * 54);
        int ordem = -1;
        for (int k = 0; k < f->n; k++) if (f->idx[k] == p->mao[i]) ordem = k;
        bool cur = i == cursor && q >= 1;
        if (cur) ui_selecao(x - 25, y - 25, 50, 50, p->cor);
        if (cur) brilho_passa = mola_brilho();          // pulou: o brilho passa junto
        int rs = respira(i);                     // esperando na mao: respira
        caixa_fechada = p == &J[0] && p->mao[i] == embrulho;
        peca_parada((float)x, (float)(y + dy - (cur ? 3 + mola_px() : 0) + rs), 17, p->col[p->mao[i]], vez);
        if (!cur && q >= 1) marca_rar(x, y + 24, p->col[p->mao[i]].tipo);
        if (ordem >= 0) plaqueta_num(x + 16, y - 17 + rs, ordem, C_OURO, C_BARRA, true);
        if (cur) gfx_rect(x - 16, y + 23, 32, 3, p->cor);
        if (cosmo_recem[vez][p->mao[i]]) cosmo_chegou(x, y + dy + rs, p->col[p->mao[i]].tipo, t_fase - cosmo_chega(p, i));   // evoluiu: o nome novo
        zona_clique(x - 25, y - 25, 50, 50, &cursor, i, KEY_ENTER);
        if (p == &J[0] && p->mao[i] == embrulho) dica_zona(x - 25, y - 25, 50, 50, DICA_DADO, DICA_EMBRULHO);
        else dica_peca(x - 25, y - 25, 50, 50, &p->col[p->mao[i]]);
    }

    // No meio da mesa, so a fila: os lugares sendo preenchidos e o pronto.
    // O que o dado em foco faz fica na dica do mouse (ou no I).
    int cabe = max_fila(vez), ry = linha_lugares(vez);
    int no_cur = n && cursor < n ? p->mao[cursor] : -1;
    for (int k = 0; k < f->n; k++) if (f->idx[k] == no_cur) no_cur = -1;   // ja na fila: o ENTER tira
    int lados_cur = no_cur < 0 ? 0 : p == &J[0] && no_cur == embrulho ? 6   // o embrulho nao entrega a forma
                  : TIPO[p->col[no_cur].tipo].lados;
    int fim = lugares_fila(vez, ry, lados_cur);
    for (int k = 0; k < f->n; k++)               // clicar num dado da sequencia tira ele dela
        for (int i = 0; i < n; i++)
            if (p->mao[i] == f->idx[k]) zona_clique(lugar_x(cabe, k) - 16, ry - 16, 32, 32, &cursor, i, KEY_ENTER);
    if (f->n) {                                  // o pronto, logo depois do ultimo lugar
        const char *pr = "pronto";
        int wp = 10 + 8 + 6 + gfx_largura(pr, 1) + 12, bx = fim + 22, by = ry - BOTAO_H / 2 - 1, d;
        bool em = mouse_em(bx, by, wp, BOTAO_H), cheia = f->n >= cabe;
        uint16_t c = ui_botao_face(bx, by, wp, BOTAO_H, C_VERDE, em || cheia ? UI_FOCO : 0, &d);
        if (cheia && !em) ui_cantos_em(bx, by, wp, BOTAO_H + 1, C_OURO, 3 + ui_respira());   // cheia: so falta confirmar
        sprite_em(bx + 10 + 4, by + d + (BOTAO_H - 4) / 2, IC_VISTO, 5, C_MARFIM, C_BRANCO, C_MARFIM, false, 0);
        gfx_texto(bx + 10 + 8 + 6, by + d + (BOTAO_H - 4 - TXT_H) / 2 + 1, pr, c, 1, false);
        zona_clique(bx, by, wp, BOTAO_H, NULL, 0, KEY_TAB);
    }
    plaqueta_sua_vez(GFX_W / 2, vez == baixo ? ry - 34 : ry + LUGAR_R + 36);   // do lado de longe da mao
}

// Chance (%) de o proximo dado de j derrubar alguma coisa: ele mesmo ou o
// que valia antes. Joga o lance muitas vezes numa copia da mesa, com as
// regras de verdade (Gemeo, Bumerangue, Teimoso, Sentinela, Vidro...); o
// sorteio e o da IA, guardado e devolvido, para nada do jogo mudar.
static int chance_cair(int j)
{
    static fila_t c_fila;                       // a mesa da ultima conta: so refaz se mudou
    static int c_vez = -1, c_valor = 0;
    fila_t *f = &F[j];
    int k = f->lancados;
    if (k >= f->n) return 0;
    if (c_vez == j && !memcmp(&c_fila, f, sizeof c_fila)) return c_valor;
    memcpy(&c_fila, f, sizeof c_fila);
    uint8_t antes[N_FILA];
    memcpy(antes, f->est, sizeof antes);
    bool ego = f->tipo[k] == D_EGOISTA;          // o Egoista derruba os outros de proposito
    mesa_t m;
    guarda_mesa(&m);
    uint32_t sem = semente_ia;
    const int N = 2000;
    int caiu = 0;
    for (int a = 0; a < N; a++) {
        semeia_ia(0xC4A1u + (uint32_t)rodada * 31u + (uint32_t)k, a);
        sim_lanca(j);
        bool c = f->est[k] != V_VALIDO || (f->tipo[k] == D_TUDO_NADA && f->cru[k] == 1);
        for (int i = 0; i < k && !c && !ego; i++) c = antes[i] == V_VALIDO && f->est[i] != V_VALIDO;
        caiu += c;
        volta_mesa(&m);
    }
    semente_ia = sem;
    c_vez = j;
    c_valor = (caiu * 100 + N / 2) / N;
    return c_valor;
}


// A dica so para quem esta aprendendo: some no Dificil da partida rapida e do
// Nivel 6 do Desafiante em diante.
static bool mostra_chance(void)
{
    if (cpu >> vez & 1) return false;            // vez do computador
    if (modo == M_DESAFIO) return des_dificuldade() < 6;
    if (cpu >> outro(vez) & 1) return nivel_jog[outro(vez)] < 2;
    return true;
}

static void desenha_joga(void)
{
    jogador_t *p = &J[vez];
    fila_t *f = &F[vez];
    char t[64];
    snprintf(t, sizeof t, "%s joga", p->nome);
    int y1 = CENTRO - 40;

    // Os botoes primeiro: a largura do painel depende deles.
    char parar[40] = "parar", jogar[40] = "jogar";
    const char *rot[2] = { jogar, parar };
    if (fase == F_JOGA) {
        // Para quem joga em segundo, o botao ja diz o que parar significa.
        if (vez != primeiro && f->lancados) {
            desfecho_t d;
            calcula_desfecho(&d, false);
            int dif = d.total[vez] - d.total[outro(vez)];
            if (dif > 0)      snprintf(parar, sizeof parar, "parar: vence por %d", dif);
            else if (dif < 0) snprintf(parar, sizeof parar, "parar: perde por %d", -dif);
            else              snprintf(parar, sizeof parar, "parar: empata");
        }
        if (f->lancados == 0) snprintf(parar, sizeof parar, "voltar à sequência");
        if (f->lancados && f->lancados < f->n && mostra_chance()) {
            int c = chance_cair(vez);
            if (c <= 0) snprintf(jogar, sizeof jogar, "jogar: sem risco");
            else        snprintf(jogar, sizeof jogar, "jogar: %d%% de risco", c);
        }
    }
    // Rolando, o dado que voa fala sozinho. No veredito do dia a dia (vale 5,
    // caiu) fala o numero que sobe do dado; o painel so aparece para o que
    // e especial, numa linha.
    if (fase == F_ROLANDO || fase == F_ARRUMA || (fase == F_VEREDITO && ver_comum)) { pote_mesa(); return; }
    // A maquina joga com a mesa limpa: nem o nome nem o dado da vez, so os
    // dados caindo (o que for especial ainda aparece no veredito).
    if (fase == F_JOGA && anuncia_maquina()) { pote_mesa(); return; }
    if (fase == F_VEREDITO) {
        uint16_t c = ver_tipo == 0 ? C_OURO : ver_tipo == 2 ? C_ACO
                   : ver_tipo == 4 ? gfx_mistura(C_EGO, C_BRANCO, 25) : C_VINHO_CLR;
        painel(y1 - 5, TXT_H + 8, larg_painel(gfx_largura(veredito, 1), 160));
        txt_c(GFX_W / 2, y1 - 1, veredito, c, false);
        pote_mesa();
        return;
    }
    bool prox = fase == F_JOGA && f->lancados < f->n;     // o proximo dado da fila, junto do nome
    bool menu = fase == F_JOGA && !anuncia_maquina();     // a maquina joga sem botoes
    int w = prox ? larg_quem_e_dado(t, tipo_de(vez, f->lancados)) : gfx_largura(t, 1);
    if (menu && larg_botoes(rot, 2) > w) w = larg_botoes(rot, 2);
    int h = fase == F_JOGA ? (prox ? 2 * TXT_H : TXT_H) + (menu ? BOTAO_H + 12 : 4) : 2 * TXT_H + 4;
    painel(y1 - 5, h, larg_painel(w, 220));

    if (prox) quem_e_dado(y1, t, p->cor, tipo_de(vez, f->lancados));
    else txt_c(GFX_W / 2, y1, t, p->cor, true);

    if (menu) {
        bool ativo[2] = { f->lancados < f->n, true };
        static const uint16_t TINTA_JOGA[2] = { RGB(38, 96, 64), RGB(40, 62, 104) };   // jogar, parar
        botoes(y1 + (prox ? 2 * TXT_H : TXT_H) + 1, rot, ativo, 2, TINTA_JOGA);
    }
    pote_mesa();
}

// Os botoes da aposta: maiores que os outros, cada um com o seu desenho e a
// sua cor, para nao errar no clique ligeiro. Passar: o visto. Pagar: as
// fichas. Apostar 10 e dobrar: as fichas e uma seta. Apostar 25: as fichas e
// tres setas. Correr: o X vermelho no ebano, afastado dos outros.
#define AP_VAO     14                    // entre um botao e outro
#define AP_LONGE   34                    // e do correr para o vizinho
static int icone_aposta_w(int op)
{
    return op == OP_PASSAR ? 8 : op == OP_CORRER ? 7 : op == OP_PAGAR ? 9 : 9 + 2 + 7;
}
static void icone_aposta(int op, int x, int cy)
{
    if (op == OP_PASSAR) { sprite_em(x + 4, cy, IC_VISTO, 5, C_MARFIM, C_BRANCO, C_MARFIM, false, 0); return; }
    if (op == OP_CORRER) { sprite_em(x + 3, cy, IC_X, 7, C_VERMELHO, C_ROSA, C_VERMELHO, false, 0); return; }
    sprite_em(x + 4, cy + 1, IC_FICHAS, 7, C_MARFIM, C_BRANCO, C_VERMELHO, false, 0);   // a pilha de fichas, de lado
    if (op == OP_APOSTAR10 || op == OP_AUMENTAR) sprite_em(x + 14, cy - 1, IC_SETA, 5, C_AMARELO, C_BRANCO, C_AMARELO, false, 0);
    if (op == OP_APOSTAR25)                                  // tres setas, uma em cima da outra
        for (int i = 0; i < 3; i++) sprite_em(x + 14, cy - 4 + i * 4, IC_SETA, 5, C_AMARELO, C_BRANCO, C_AMARELO, false, 0);
}
static int larg_aposta(const int *op, int n, char rot[][24], int *w)
{
    int tot = 0;
    for (int i = 0; i < n; i++) {
        w[i] = 10 + icone_aposta_w(op[i]) + 6 + gfx_largura(rot[i], 1) + 12;
        tot += w[i] + (i ? (op[i] == OP_CORRER || op[i - 1] == OP_CORRER ? AP_LONGE : AP_VAO) : 0);
    }
    return tot;
}
static void botoes_aposta(int y, const int *op, int n, char rot[][24])
{
    int w[4], x = GFX_W / 2 - larg_aposta(op, n, rot, w) / 2;
    for (int i = 0; i < n; i++) {
        if (i) x += op[i] == OP_CORRER || op[i - 1] == OP_CORRER ? AP_LONGE : AP_VAO;
        int d;
        uint16_t c = ui_botao_face(x, y, w[i], BOTAO_AP_H, tinta_op(op[i]), i == cursor ? UI_FOCO : 0, &d);
        if (op[i] == OP_CORRER) c = C_ROSA;                  // o correr fala em vermelho
        icone_aposta(op[i], x + 10, y + d + (BOTAO_AP_H - 4) / 2);
        gfx_texto(x + 10 + icone_aposta_w(op[i]) + 6, y + d + (BOTAO_AP_H - 4 - TXT_H) / 2 + 1, rot[i], c, 1, false);
        zona_clique(x, y, w[i], BOTAO_AP_H, &cursor, i, KEY_ENTER);
        x += w[i];
    }
}

static void desenha_aposta(void)
{
    char t[64], q[64];
    int y1 = CENTRO - 36;
    // Uma linha so: quem fez quantos pontos e a pergunta, cada um na sua cor.
    snprintf(t, sizeof t, "%s fez %d pontos", J[primeiro].nome, F[primeiro].pontos);
    if (a_pagar)            snprintf(q, sizeof q, "%s paga %d para ver?", vez == primeiro ? "" : J[vez].nome, a_pagar);
    else if (vez == primeiro) snprintf(q, sizeof q, "aposta?");
    else                    snprintf(q, sizeof q, "%s aposta?", J[vez].nome);
    const char *qq = q[0] == ' ' ? q + 1 : q;
    int wt = gfx_largura(t, 1), wq = gfx_largura(qq, 1), sep = 20;

    int op[4], n = opcoes(op), wb[4];
    char r[4][24];
    for (int i = 0; i < n; i++) rotulo_op(op[i], r[i], sizeof r[i]);
    bool menu = !anuncia_maquina();              // a maquina decide sem botoes
    int w = wt + sep + wq;
    if (menu && larg_aposta(op, n, r, wb) > w) w = larg_aposta(op, n, r, wb);
    painel(y1 - 5, menu ? TXT_H + BOTAO_AP_H + 22 : TXT_H + 8, larg_painel(w, 220));
    int x = GFX_W / 2 - (wt + sep + wq) / 2;
    gfx_texto(x, y1, t, J[primeiro].cor, 1, true);
    txt_c(x + wt + sep / 2, y1, "·", C_TEXTO_M, false);
    gfx_texto(x + wt + sep, y1, qq, J[vez].cor, 1, false);
    if (menu) botoes_aposta(y1 + TXT_H + 6, op, n, r);
    pote_mesa();
}

static void desenha_efeitos(void)
{
    if (ev_i >= n_ev) return;
    const evento_t *e = &ev[ev_i];
    uint16_t c = e->tipo == EV_ADESIVO ? ADESIVO[e->ad].luz
               : e->tipo == EV_FOGO ? C_FOGO
               : e->tipo == EV_SEMENTE ? C_VERDE
               : e->tipo == EV_JACKPOT ? C_AMARELO
               : e->tipo == EV_BONUS && e->por_dado[0] + e->por_dado[1] + e->por_dado[2] + e->por_dado[3] < 0 ? C_VINHO_CLR
               : (e->tipo == EV_ROUBA || e->tipo == EV_BONUS || e->tipo == EV_MULT || e->tipo == EV_EXTRA
                  || e->tipo == EV_AVISO || (e->tipo == EV_CHEFE && e->de_j == 0)) ? C_OURO
               : C_VINHO_CLR;
    // Fundo atras do texto: o dado roubado passa por baixo, sem embaralhar.
    // Uma linha so: o efeito, na cor do que ele faz, e na frente o desenho do
    // poder do dado que age (o mesmo que anima em cima dele). De quem e,
    // mostram os dados que se mexem e os numeros que sobem deles. Fica a vista
    // tambem no respiro, depois que o efeito se aplica.
    int tipo = e->tipo != EV_ADESIVO && e->de_j < 2 && e->de_k < N_FILA ? F[e->de_j].tipo[e->de_k] : -1;
    int f = gfx_largura(e->txt, 1) > 300 ? GFX_FINO : 1;   // o texto comprido fica no fino
    int wt = gfx_largura(e->txt, f), wi = tipo >= 0 && TIPO[tipo].efeito ? 20 : 0;
    painel(CENTRO - 15, TXT_H + 8, larg_painel(wt + wi, 160));
    int x = GFX_W / 2 - (wt + wi) / 2;
    if (wi && !icone_poder(x + 6, CENTRO - 15 + (TXT_H + 8) / 2, tipo)) x += wi / 2, wi = 0;
    gfx_texto(x + wi, CENTRO - 11 - (f == 1), e->txt, c, f, f == 1);
    desenha_evento();
}

// O registro do fim da rodada, a esquerda da mesa: os pontos de cada um no
// comeco, cada efeito numa linha (com a cor de quem agiu) a medida que
// acontece, e os pontos no fim. Miudo e apagado: e para conferir.
#define LOG_X0  (MESA_X0 + 10)
#define LOG_W   156                      // ate onde o aviso do meio nao chega
#define LOG_Y0  (LINHA_CIMA + R_DADO + 14)
#define LOG_Y1  (LINHA_BAIXO - R_DADO - 14)
#define LOG_LIN 10
// Quebra s em linhas de no maximo w pixels. Devolve quantas; com desenha,
// escreve a partir de (x, y).
static int log_quebra(const char *s, int x, int y, int w, uint16_t c, bool desenha)
{
    char linha[64];
    int n = 0;
    while (*s) {
        int cabe = 0, i = 0;
        while (s[i]) {
            int fim = i;
            while (s[fim] == ' ') fim++;
            while (s[fim] && s[fim] != ' ') fim++;
            if (fim >= (int)sizeof linha) break;
            memcpy(linha, s, (size_t)fim);
            linha[fim] = 0;
            if (gfx_largura(linha, GFX_MIUDO) > w && cabe) break;
            cabe = i = fim;
        }
        if (!cabe) cabe = (int)strlen(s) < (int)sizeof linha - 1 ? (int)strlen(s) : (int)sizeof linha - 1;
        memcpy(linha, s, (size_t)cabe);
        linha[cabe] = 0;
        if (desenha) gfx_texto(x, y + n * LOG_LIN, linha, c, GFX_MIUDO, false);
        n++;
        s += cabe;
        while (*s == ' ') s++;
    }
    return n;
}

// "JOGADOR 1 17  ·  JOGADOR 2 14": os dois placares numa linha.
static void log_placar(int y, const char *rot, const int *pts)
{
    int x = LOG_X0;
    x += gfx_texto(x, y, rot, C_TEXTO_M, GFX_MIUDO, false) + 6;
    for (int o = 0; o < 2; o++) {
        int j = o == 0 ? outro(baixo) : baixo;   // na ordem da mesa: o de cima primeiro
        char s[40];
        snprintf(s, sizeof s, "%s %d", J[j].nome, pts[j]);
        gfx_rect(x, y + 3, 3, 3, J[j].cor);
        x += 6 + gfx_texto(x + 6, y, s, C_TEXTO_M, GFX_MIUDO, false) + 10;
    }
}

static void desenha_log(void)
{
    if (!n_ev || (fase != F_EFEITOS && fase != F_RESULTADO)) return;
    int ate = fase == F_EFEITOS ? ev_i : n_ev - 1;             // o ultimo que ja aparece
    bool fim = fase == F_RESULTADO;
    // Quantas linhas cada efeito ocupa; se nao couberem todos, ficam os
    // ultimos, com uma linha de "..." no lugar dos de antes (e, se a decisiva
    // ficou la, duas linhas embaixo para repeti-la).
    int de = ate;
    for (int reserva = 0; reserva <= 3; reserva += 3) {
        int livres = (LOG_Y1 - LOG_Y0) / LOG_LIN - 1 - fim - reserva;   // tirando os placares
        for (int corta = 0; corta < 2; corta++) {
            int usadas = 0;
            de = ate;
            for (int i = ate; i >= 0; i--) {
                int n = log_quebra(ev[i].txt, 0, 0, LOG_W - 8, 0, false);
                if (usadas + n > livres - corta) break;
                usadas += n;
                de = i;
            }
            if (de == 0) break;
        }
        if (!(fim && ev_decisivo >= 0 && ev_decisivo < de)) break;
    }
    int y = LOG_Y0;
    log_placar(y, "início", log_base);
    y += LOG_LIN + 2;
    if (de > 0) {
        gfx_texto(LOG_X0 + 8, y, "...", C_TEXTO_M, GFX_MIUDO, false);
        y += LOG_LIN;
    }
    for (int i = de; i <= ate; i++) {
        const evento_t *e = &ev[i];
        bool decide = fim && i == ev_decisivo;   // no fim, a linha que decidiu a rodada, em ouro
        uint16_t c = decide ? C_OURO : i == ate && !fim ? C_BRANCO : C_MARFIM_S;
        if (decide) estrela4(LOG_X0 + 1, y + 4, 2, C_BRANCO, C_OURO, C_LATAO);
        else gfx_rect(LOG_X0, y + 3, 3, 3, J[e->de_j].cor);
        y += LOG_LIN * log_quebra(e->txt, LOG_X0 + 8, y, LOG_W - 8, c, true);
    }
    if (fim) {
        int tot[2] = { F[0].total, F[1].total };
        log_placar(y + 2, "final", tot);
        if (ev_decisivo >= 0 && ev_decisivo < de) {          // a decisiva ficou no "...": repete embaixo
            char s[96];
            snprintf(s, sizeof s, "decidiu: %s", ev[ev_decisivo].txt);
            estrela4(LOG_X0 + 1, y + LOG_LIN + 8, 2, C_BRANCO, C_OURO, C_LATAO);
            log_quebra(s, LOG_X0 + 8, y + LOG_LIN + 4, LOG_W - 8, C_OURO, true);
        }
    }
}

// O resultado sem caixa de texto: o placar ja diz quem levou (mesa.inc) e o
// pote voa, ficha a ficha, ate a faixa de quem levou, cujo placar sobe
// enquanto elas chegam; quem perdeu fichas alem do pote (Gorjeta, Agiota,
// aluguel...) ve o "-N" na faixa dele. No empate o pote fica na mesa e pula.
// So no fim, discreto, o ENTER para seguir.
static int res_fim_voo(void)
{
    int w = res_quem;
    if (w < 0) return 0;
    int n = (J[w].fichas - res_antes[w]) / 8 + 3;
    return n > 10 ? 10 : n;
}
static void res_numero(int j, const char *s, uint16_t c, float q)
{
    int sobe = (int)((q < 0 ? 0 : q) * 10);           // no feltro, junto da faixa
    bool cima = ficha_py[j] < CENTRO;
    int ty = cima ? MESA_Y0 + 6 + sobe : MESA_Y1 - TXT_H - 8 - sobe;
    txt_sombra_c(ficha_px[j] + 16, ty, s, c, 1);
}
static void desenha_resultado(void)
{
    int w = res_quem, n = res_fim_voo();
    float fim = 0.05f + n * 0.07f + 0.55f;
    if (venc_mao == 2) {                                   // empate: o pote fica e pula no lugar
        float q = t_fase < 0.6f ? t_fase / 0.6f : 0;
        int pulo = (int)(sinf(q * 3.1416f * 2) * 4 * (1 - q));
        int y = CENTRO + 38 - pulo;
        pote_mesa();
        ui_plaqueta(POTE_X, y, "EMPATE", C_MARFIM_S);
        fim = 0.6f;
    }
    if (w >= 0) {
        float x0 = POTE_X, y0 = CENTRO + 10;
        for (int i = 0; i < n; i++) {
            float q = (t_fase - 0.05f - i * 0.07f) / 0.55f;
            if (q <= 0 || q >= 1) continue;
            float e = q * q * (3 - 2 * q);                    // sai devagar, acelera, pousa
            float fx = x0 + (ficha_px[w] - x0) * e, fy = y0 + (ficha_py[w] - y0) * e - sinf(q * 3.1416f) * 46;
            ficha((int)fx, (int)fy, i % 3 == 0 ? C_LATAO : (i % 3 == 1 ? C_VINHO : C_MARFIM_S));
        }
    }
    float q = (t_fase - fim) / 0.9f;
    if (q > -0.15f && q < 1)                               // o "+N" de quem ganhou, o "-N" de quem perdeu
        for (int j = 0; j < 2; j++) {
            int d = J[j].fichas - res_antes[j];
            if (!d) continue;
            char t[16];
            snprintf(t, sizeof t, "%+d", d);
            res_numero(j, t, d > 0 ? C_OURO : C_VINHO_CLR, q);
        }
    if (t_fase > fim + 0.35f) teclas_c(GFX_W / 2, CENTRO - 10, "ENTER segue", C_TEXTO_M);
}

// As fichas da faixa: no resultado, sobem (ou descem) enquanto o pote voa.
static int fichas_vis(int j)
{
    if (fase != F_RESULTADO || res_quem < 0) return J[j].fichas;
    int n = res_fim_voo();
    float fim = 0.05f + n * 0.07f + 0.55f, q = (t_fase - 0.3f) / (fim - 0.3f);
    if (q >= 1) return J[j].fichas;
    if (q < 0) q = 0;
    return res_antes[j] + (int)((J[j].fichas - res_antes[j]) * q);
}

// A bolsa nao tem limite de dados: uma grade de n dados que, se nao couber
// na altura alt com o tamanho cheio, poe mais dados por linha e encolhe
// todos por igual (f: a escala; a largura da grade fica a mesma).
typedef struct { int por; float f; } grade_t;
static grade_t grade_cabe(int n, int por, int passo_y, int alt)
{
    grade_t g = { por, 1.0f };
    while (g.f > 0.25f && (n + g.por - 1) / g.por * passo_y * g.f > alt) {
        g.por++;
        g.f = (float)por / g.por;
    }
    return g;
}

// A escolha do que sai: 12 por linha, duas linhas e meia antes de encolher.
#define TIRA_Y 108                   // o alto da grade
static grade_t grade_tira(int n) { return grade_cabe(n, 12, 52, 130); }

static void desenha_premio(void)
{
    jogador_t *p = &J[vez];
    char t[64];
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    if (premio_colando()) { premio_cola_desenha(); return; }   // o adesivo ganho: o dado e o lado

    if (removendo) {
        // Escolha de qual dado sai da colecao: a colecao inteira em duas linhas.
        snprintf(t, sizeof t, "%s: qual dos %d dados sai?", p->nome, p->n_col);
        txt_c(GFX_W / 2, MESA_Y0 + 14, t, p->cor, true);
        teclas_c(GFX_W / 2, MESA_Y0 + 40, "ENTER tira para sempre  ·  BACKSPACE volta", C_TEXTO_M);
        grade_t g = grade_tira(p->n_col);
        float f = g.f;
        int sel = (int)(22 * f), meio = (int)(16 * f);
        for (int i = 0; i < p->n_col; i++) {
            int lin = i / g.por, col = i % g.por;
            int na_linha = (p->n_col - lin * g.por) < g.por ? p->n_col - lin * g.por : g.por;
            int x = GFX_W / 2 + (int)((col - (na_linha - 1) / 2.0f) * 46 * f);
            int y = TIRA_Y + (int)((lin + 0.5f) * 52 * f);
            if (t_tira > 0 && i == tira_i) {
                // O escolhido encolhe e cai, deixando um rastro de po.
                float q = t_tira / 0.7f;
                float r = 17 * f * (1 - q);
                if (r > 2) peca_parada((float)x, y + q * 24 * f, r, p->col[i], vez);
                for (int k = 0; k < 7; k++)
                    gfx_rect(x - (int)(14 * f) + (int)(k * 4 * f), y + (int)(q * 34 * f) + (k % 2) * 6, 3, 3, C_TEXTO_M);
                continue;
            }
            if (i == cursor) ui_selecao(x - sel, y - sel, 2 * sel, 2 * sel, C_VINHO_CLR);
            if (i == cursor) brilho_passa = mola_brilho();
            peca_parada((float)x, (float)(y - (i == cursor ? mola_px() : 0)), 17 * f, p->col[i], vez);
            if (i != cursor) marca_rar(x, y + (int)(23 * f), p->col[i].tipo);
            if (i == cursor) gfx_rect(x - meio, y + (int)(23 * f), 2 * meio, 3, C_VINHO_CLR);
            zona_clique(x - sel, y - sel, 2 * sel, 2 * sel, &cursor, i, KEY_ENTER);
        }
        botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar", KEY_BKSP);
        int tc = p->col[cursor < p->n_col ? cursor : 0].tipo;
        int linhas = (p->n_col + g.por - 1) / g.por, ty = TIRA_Y + (int)(linhas * 52 * f) + 24;
        if (ty < 236) ty = 236;              // o nome do dado logo abaixo da grade
        txt_c(GFX_W / 2, ty, TIPO[tc].nome, cor_dado(tc), true);
        txt_c(GFX_W / 2, ty + 22, t_tira > 0 ? "some para sempre" : TEXTO_MEDIO[tc],
              t_tira > 0 ? C_VINHO_CLR : C_MARFIM_S, false);
        return;
    }

    snprintf(t, sizeof t, "%s levou a rodada e escolhe um prêmio", p->nome);
    txt_c(GFX_W / 2, MESA_Y0 + 14, t, p->cor, true);
    bool girando = slot_girando();
    for (int i = 0; i < 3; i++) {
        int x = GFX_W / 2 + (i - 1) * 150;
        int t2 = oferta[i].tipo, ad = oferta_ad[i];
        float para = slot_para(i);
        bool parado = t_slot < 0 || t_slot >= para;
        bool cur = i == cursor && !girando;
        uint16_t cr = COR_RAR[rar_oferta(i)];
        ui_cartao(x - 64, 90, 128, 110, cur, cr);
        if (!parado) {
            // O rolo: dados passando de cima para baixo, cada vez mais devagar,
            // ate o ultimo (o premio) chegar ao centro. Os de passagem encolhem
            // perto das bordas, como num cilindro.
            // O rolo e desenhado num rascunho e so a janela volta para a tela:
            // os dados de passagem nao vazam para fora (nem os numeros deles).
            const int wx = x - 60, wy = 94, ww = 120, wh = 64;
            uint16_t *tela = display_fb();
            gfx_desvia(rascunho);
            gfx_rect(wx, wy, ww, wh, C_FELTRO_ESC);
            float r = para - t_slot, o = 9.4f * r * sqrtf(r);
            int k0 = (int)o;
            for (int k = k0 - 1; k <= k0 + 1; k++) {
                if (k < 0) continue;
                float dy = (o - k) * 34;
                float q = 1 - fabsf(dy) / 40;
                if (q <= 0.35f) continue;
                peca_t pc = oferta[i];
                uint32_t h = (uint32_t)(k * 2654435761u) ^ (uint32_t)(i * 40503u + 17);
                if (ad) {                                // o rolo de adesivos: passam outros
                    int a = k > 0 ? 1 + (int)(h % (uint32_t)(AD_N - 1)) : ad;
                    if (!ad_no_premio(a)) a = AD_CONFETE;          // (o Dourado nao sai aqui)
                    ad_solto_d(x, 126 + (int)dy, a, q > 0.75f ? 5 : 4, 0);
                    continue;
                }
                if (k > 0) {
                    // So os tipos que podem sair aqui: os basicos.
                    pc.tipo = (uint8_t)basico((int)(h % (uint32_t)n_basicos()));
                    pc.fixo = (uint8_t)(1 + (h >> 8) % 8);
                }
                peca_parada((float)x, 126 + dy, 24 * q, pc, vez);
            }
            for (int b2 = 0; b2 < 8; b2++) {             // sombra nas bordas: um cilindro
                gfx_rect_alfa(wx, wy + b2, ww, 1, C_BARRA, 150 - b2 * 18);
                gfx_rect_alfa(wx, wy + wh - 1 - b2, ww, 1, C_BARRA, 150 - b2 * 18);
            }
            gfx_desvia(NULL);
            for (int yy = wy; yy < wy + wh; yy++)
                memcpy(tela + yy * GFX_W + wx, rascunho + yy * GFX_W + wx, (size_t)ww * 2);
            txt_c(x, 162, "?", C_TEXTO_M, true);
            continue;
        }
        // Acabou de travar: um pulo e a moldura acende.
        float q = t_slot >= 0 ? (t_slot - para) / 0.3f : 1;
        bool acende = q < 1;
        if (acende) ui_cartao_aceso(x - 64, 90, 128, 110, true, gfx_mistura(cr, C_BRANCO, (int)(60 * (1 - q))));
        float pulo = acende ? sinf(q * 3.14159f) * -8 : 0;
        float bob = cur ? sinf(t_fase * 4) * 3 - mola_px() : 0;
        if (cur) brilho_passa = mola_brilho();
        char c2[40];
        if (ad) {                                    // o adesivo: solto, a ponta levantando na vez dele
            float descola = cur ? 0.25f + 0.15f * sinf(t_fase * 4) : 0;
            ad_solto_d(x, 128 + (int)(bob + pulo), ad, acende && q < 0.5f ? 6 : 5, descola);
            txt_c(x, 162, ADESIVO[ad].nome, cur || acende ? ADESIVO[ad].luz : C_MARFIM_S, true);
            snprintf(c2, sizeof c2, "adesivo · %s", RARIDADE[RAR_ADESIVO[ad]]);
        } else {
            peca_parada((float)x, 130 + bob + pulo, 26 * (acende ? 1 + 0.12f * (1 - q) : 1), oferta[i], vez);
            txt_c(x, 162, TIPO[t2].nome, cur || acende ? cor_dado(t2) : C_MARFIM_S, true);
            classe(t2, c2, sizeof c2);
        }
        ui_selo_c(x, 180, c2, cr, GFX_FINO);
        zona_clique(x - 64, 90, 128, 110, &cursor, i, KEY_ENTER);
    }
    bool pode = p->n_col > FILA_NORMAL;
    if (girando) {
        txt_c(GFX_W / 2, 212, "sorteando...", C_OURO, true);
    } else if (cursor < 3 && oferta_ad[cursor]) {   // o adesivo: o que faz, e que vai num lado de um dado seu
        const adesivo_t *d = &ADESIVO[oferta_ad[cursor]];
        if (premio_cabe(oferta_ad[cursor])) {
            char l[112];
            snprintf(l, sizeof l, "%s %s", d->l1, d->l2);
            txt_c(GFX_W / 2, 212, l, C_MARFIM, false);
        } else txt_c(GFX_W / 2, 212, "nenhum dado seu tem lado livre", C_VINHO_CLR, false);
    } else if (cursor < 3) {
        txt_c(GFX_W / 2, 212, TEXTO_MEDIO[oferta[cursor].tipo], C_MARFIM, false);
    } else {
        txt_c(GFX_W / 2, 212, pode ? "troca o prêmio por tirar um dos seus, para sempre"
                                   : "com 4 dados, nenhum pode sair", C_MARFIM, false);
    }

    // Quarta opcao: trocar o premio por afinar a bolsa.
    const char *rot = "remover um dado";
    int w = gfx_largura(rot, 1) + 28;
    bool cur = cursor == 3;
    ui_botao(GFX_W / 2 - w / 2, 240, w, 26, rot, C_VINHO, !pode ? UI_DESLIGADO : cur ? UI_FOCO : 0);
    zona_clique(GFX_W / 2 - w / 2, 240, w, 26, &cursor, 3, KEY_ENTER);
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "recusar", 'x');
    teclas_c(GFX_W / 2, 280, "setas escolhem  ·  ENTER leva  ·  X recusa", C_TEXTO_M);
}

// Uma quantia de fichas com o icone da ficha na frente, centrada em cx.
static void fichas_c(int cx, int y, int valor, const char *sufixo, uint16_t c)
{
    char t[80];
    snprintf(t, sizeof t, "%d%s", valor, sufixo);
    int w = gfx_largura(t, 1) + 20, x = cx - w / 2;
    ficha(x + 7, y + 11, C_LATAO);
    txt(x + 20, y, t, c, true);
}

static void desenha_loja(void)
{
    jogador_t *p = &J[vez];
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    txt_c2(GFX_W / 2, MESA_Y0 + 2, "LOJA DA CASA", C_LATAO);
    char t[64];
    snprintf(t, sizeof t, " fichas  ·  %s compra", p->nome);
    fichas_c(GFX_W / 2, MESA_Y0 + 54, p->fichas, t, p->cor);

    for (int i = 0; i < N_LOJA; i++) {
        int x = GFX_W / 2 + (i - 2) * 108, yb = 142;
        int tp = loja[i].tipo, pr = TIPO[tp].preco;
        bool ja = levou[vez][i], cur = i == cursor, da = p->fichas >= pr;
        uint16_t cr = cor_rar(tp);
        ui_cartao(x - 48, yb, 96, 92, cur, cr);
        float bob = cur && !ja ? sinf(t_fase * 4) * 3 - mola_px() : 0;
        if (cur && !ja) brilho_passa = mola_brilho();
        peca_parada((float)x, yb + 32 + bob, 22, loja[i], vez);
        if (ja) {
            txt_c(x, yb + 64, "comprado", C_TEXTO_M, false);
            gfx_rect_alfa(x - 47, yb + 1, 94, 90, C_SOMBRA, 120);
        } else {
            fichas_c(x, yb + 64, pr, "", da ? C_OURO : C_VINHO_CLR);
        }
        if (levou[outro(vez)][i] && (cpu >> outro(vez) & 1))    // a maquina comprou este
            ficha(x + 36, yb + 11, J[outro(vez)].cor);
        zona_clique(x - 48, yb, 96, 92, &cursor, i, KEY_ENTER);
    }
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "terminar", KEY_TAB);
    dado_e_poder(240, loja[cursor].tipo, true);
    char rival[48];
    snprintf(rival, sizeof rival, "%s também comprou este", J[outro(vez)].nome);
    if (!levou[vez][cursor] && p->fichas < TIPO[loja[cursor].tipo].preco)
        txt_c(GFX_W / 2, 286, "fichas insuficientes", C_VINHO_CLR, true);
    else if (levou[outro(vez)][cursor] && (cpu >> outro(vez) & 1))
        txt_c(GFX_W / 2, 286, rival, J[outro(vez)].cor, true);
    else
        teclas_c(GFX_W / 2, 288, "setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB termina", C_TEXTO_M);
}

static void desenha_fim(void)
{
    painel(CENTRO - 70, 150, 440);
    if (venc_partida == 2) {
        txt_sombra_c(GFX_W / 2, CENTRO - 58, "EMPATE", C_MARFIM, 3);
    } else {
        jogador_t *p = &J[venc_partida];
        txt_sombra_c(GFX_W / 2, CENTRO - 58, p->nome, p->cor, 3);
        txt_c(GFX_W / 2, CENTRO + 6, "leva a mesa", C_MARFIM, true);
    }
    char s[48];
    snprintf(s, sizeof s, "%s %d  x  %d %s", J[baixo].nome, J[baixo].fichas,
             J[outro(baixo)].fichas, J[outro(baixo)].nome);
    txt_c(GFX_W / 2, CENTRO + 32, s, C_TEXTO_M, false);
    teclas_c(GFX_W / 2, CENTRO + 58, "ENTER nova partida", C_MARFIM_S);
}


// Moeda girando no catalogo: a mesma arte da mesa, virando devagar.
