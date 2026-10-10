// ===========================================================================
// Telas da partida: ordem, jogada, aposta, resultado, premio, loja
// e fim
// ===========================================================================
// Nome do dado em destaque e o poder logo abaixo.
// medio: o texto de uma linha da loja; senao, o curto da fila e do jogo.
static int larg_dado_e_poder(int tipo, bool medio)
{
    char c[32];
    classe(tipo, c, sizeof c);
    int a = gfx_largura(TIPO[tipo].nome, 1) + 12 + gfx_largura(c, 1) + 10;
    int b = gfx_largura(medio ? TEXTO_MEDIO[tipo] : TIPO[tipo].desc, 1);
    return a > b ? a : b;
}

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

// Montagem da fila: a mao comprada aparece na linha do jogador, um dado de
// cada vez saindo da bolsa. O numero dourado e a posicao na fila.
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
        peca_parada((float)x, (float)(y + dy - (cur ? 3 : 0)), 17, p->col[p->mao[i]], vez);
        if (!cur && q >= 1) marca_rar(x, y + 24, p->col[p->mao[i]].tipo);
        if (ordem >= 0) {
            char o[2] = { (char)('1' + ordem), 0 };
            gfx_disco(x + 16, y - 17, 9, C_OURO);
            gfx_disco(x + 16, y - 17, 7, C_AMARELO);
            txt_c(x + 16, y - 27, o, C_BARRA, true);
        }
        if (cur) gfx_rect(x - 16, y + 23, 32, 3, p->cor);
        zona_clique(x - 25, y - 25, 50, 50, &cursor, i, KEY_ENTER);
        dica_zona(x - 25, y - 25, 50, 50, DICA_DADO, p->col[p->mao[i]].tipo);
    }

    char t[64];
    if (vez == primeiro) snprintf(t, sizeof t, "%s monta a sequência", p->nome);
    else snprintf(t, sizeof t, "%s monta a sequência  ·  alvo %d", p->nome, F[outro(vez)].pontos);
    int y1 = CENTRO - 44;
    int cabe = max_fila(vez);
    const char *cheia = "sequência cheia: TAB ou CTRL confirma";
    int w = gfx_largura(t, 1);
    if (f->n >= cabe) { if (gfx_largura(cheia, 1) > w) w = gfx_largura(cheia, 1); }
    else if (n && larg_dado_e_poder(p->col[p->mao[cursor]].tipo, false) > w)
        w = larg_dado_e_poder(p->col[p->mao[cursor]].tipo, false);
    painel(y1 - 5, 3 * TXT_H + 34, larg_painel(w, 300));
    txt_c(GFX_W / 2, y1, t, p->cor, true);
    if (f->n >= cabe) {
        txt_c(GFX_W / 2, y1 + 24, cheia, C_OURO, true);
    } else if (n) {
        dado_e_poder(y1 + 20, p->col[p->mao[cursor]].tipo, false);
    }
    // Rodape: so a fila como vai ser jogada, 1 » 2 » 3 » 4, no centro.
    int ry = y1 + 3 * TXT_H + 10, fx = GFX_W / 2 - (cabe - 1) * 36 / 2;
    for (int k = 0; k < cabe; k++) {
        int x = fx + k * 36;
        if (k) txt_c(x - 18, ry - 11, "»", C_FELTRO2, false);
        if (k < f->n) {
            peca_t pc = { f->tipo[k], f->fixo[k] };
            peca_parada((float)x, (float)ry, 10, pc, vez);
            // Clicar num dado da sequencia tira ele dela.
            for (int i = 0; i < n; i++)
                if (p->mao[i] == f->idx[k]) zona_clique(x - 14, ry - 14, 28, 28, &cursor, i, KEY_ENTER);
        } else {
            gfx_disco(x, ry, 11, k == f->n ? C_OURO : C_FELTRO2);
            gfx_disco(x, ry, 9, C_FELTRO_ESC);
            char o[2] = { (char)('1' + k), 0 };
            txt_c(x, ry - 10, o, k == f->n ? C_OURO : C_FELTRO2, false);
        }
    }
    if (f->n) botao_clique(GFX_W / 2 + 225, ry - 12, "pronto", KEY_TAB);
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
        if (f->tipo[k] == D_ACUMULADOR && f->fixo[k] < 6) f->fixo[k]++;   // o lance soma antes
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
// Nivel 3 do Desafiante em diante.
static bool mostra_chance(void)
{
    if (cpu >> vez & 1) return false;            // vez do computador
    if (modo == M_DESAFIO) return des_dificuldade() < 3;
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
            if (c <= 0) snprintf(jogar, sizeof jogar, "jogar: não cai");
            else        snprintf(jogar, sizeof jogar, "jogar: %d%% de cair", c);
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

    int op[4], n = opcoes(op);
    char r[4][24];
    const char *rot[4];
    bool ativo[4];
    uint16_t tinta[4];
    for (int i = 0; i < n; i++) {
        rotulo_op(op[i], r[i], sizeof r[i]); rot[i] = r[i]; ativo[i] = true;
        tinta[i] = tinta_op(op[i]);
    }
    bool menu = !anuncia_maquina();              // a maquina decide sem botoes
    int w = wt + sep + wq;
    if (menu && larg_botoes(rot, n) > w) w = larg_botoes(rot, n);
    painel(y1 - 5, menu ? TXT_H + BOTAO_H + 20 : TXT_H + 8, larg_painel(w, 220));
    int x = GFX_W / 2 - (wt + sep + wq) / 2;
    gfx_texto(x, y1, t, J[primeiro].cor, 1, true);
    txt_c(x + wt + sep / 2, y1, "·", C_TEXTO_M, false);
    gfx_texto(x + wt + sep, y1, qq, J[vez].cor, 1, false);
    if (menu) botoes(y1 + TXT_H + 6, rot, ativo, n, tinta);
    pote_mesa();
}

static void desenha_efeitos(void)
{
    if (ev_i >= n_ev) return;
    const evento_t *e = &ev[ev_i];
    uint16_t c = e->tipo == EV_ZERA ? C_VIOLETA
               : e->tipo == EV_FOGO ? C_FOGO
               : e->tipo == EV_SEMENTE ? C_VERDE
               : e->tipo == EV_JACKPOT ? C_AMARELO
               : (e->tipo == EV_ROUBA || e->tipo == EV_BONUS || e->tipo == EV_MULT || e->tipo == EV_EXTRA) ? C_OURO
               : C_VINHO_CLR;
    // Fundo atras do texto: o dado roubado passa por baixo, sem embaralhar.
    // Uma linha so: o efeito, na cor do que ele faz. De quem e, mostram os
    // dados que se mexem e os numeros que sobem deles.
    painel(CENTRO - 15, TXT_H + 8, larg_painel(gfx_largura(e->txt, GFX_FINO), 160));
    txt_c(GFX_W / 2, CENTRO - 11, e->txt, c, false);
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
    int livres = (LOG_Y1 - LOG_Y0) / LOG_LIN - 1 - fim;         // tirando os placares
    // Quantas linhas cada efeito ocupa; se nao couberem todos, ficam os
    // ultimos, com uma linha de "..." no lugar dos de antes.
    int de = ate;
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
    int y = LOG_Y0;
    log_placar(y, "início", log_base);
    y += LOG_LIN + 2;
    if (de > 0) {
        gfx_texto(LOG_X0 + 8, y, "...", C_TEXTO_M, GFX_MIUDO, false);
        y += LOG_LIN;
    }
    for (int i = de; i <= ate; i++) {
        const evento_t *e = &ev[i];
        uint16_t c = i == ate && !fim ? C_BRANCO : C_MARFIM_S;
        gfx_rect(LOG_X0, y + 3, 3, 3, J[e->de_j].cor);
        y += LOG_LIN * log_quebra(e->txt, LOG_X0 + 8, y, LOG_W - 8, c, true);
    }
    if (fim) {
        int tot[2] = { F[0].total, F[1].total };
        log_placar(y + 2, "final", tot);
    }
}

static void desenha_resultado(void)
{
    uint16_t c = venc_mao == 0 ? C_LATAO : (venc_mao == 1 ? C_ROSA : C_MARFIM);
    char pl[64] = "";
    if (venc_mao >= 0)                                     // o placar da rodada
        snprintf(pl, sizeof pl, "%s   %d x %d   %s", J[0].nome, F[0].total, F[1].total, J[1].nome);
    int wp = gfx_largura(aviso, 1);
    if (gfx_largura(pl, 1) > wp) wp = gfx_largura(pl, 1);
    if (nota[0] && gfx_largura(nota, 1) > wp) wp = gfx_largura(nota, 1);
    int linhas = 1 + (pl[0] != 0) + (nota[0] != 0), y = CENTRO - 30;
    painel(y - 5, (linhas + 1) * (TXT_H - 2) + 8, larg_painel(wp, 220));
    txt_sombra_c(GFX_W / 2, y, aviso, c, 1);
    y += TXT_H - 2;
    if (pl[0]) { txt_c(GFX_W / 2, y, pl, C_MARFIM_S, false); y += TXT_H - 2; }
    if (nota[0]) { txt_c(GFX_W / 2, y, nota, C_TERRA, false); y += TXT_H - 2; }   // cinza ou cacos
    teclas_c(GFX_W / 2, y + 3, "ENTER segue", C_TEXTO_M);
    // As fichas do pote voando, uma atras da outra, ate a faixa de quem levou;
    // o placar dela sobe enquanto chegam.
    int w = res_quem;
    if (w < 0) return;
    int ganho = J[w].fichas - res_antes[w];
    int n = ganho / 8 + 3;
    if (n > 10) n = 10;
    float x0 = POTE_X, y0 = CENTRO + 10;
    for (int i = 0; i < n; i++) {
        float q = (t_fase - 0.05f - i * 0.07f) / 0.55f;
        if (q <= 0 || q >= 1) continue;
        float e = q * q * (3 - 2 * q);                        // sai devagar, acelera, pousa
        float fx = x0 + (ficha_px[w] - x0) * e, fy = y0 + (ficha_py[w] - y0) * e - sinf(q * 3.1416f) * 46;
        ficha((int)fx, (int)fy, i % 3 == 0 ? C_LATAO : (i % 3 == 1 ? C_VINHO : C_MARFIM_S));
    }
    float fim = 0.05f + n * 0.07f + 0.55f, q = (t_fase - fim) / 0.9f;
    if (ganho > 0 && q > -0.15f && q < 1) {                  // o "+N" subindo da faixa
        char mais[16];
        snprintf(mais, sizeof mais, "+%d", ganho);
        int sobe = (int)((q < 0 ? 0 : q) * 10);           // no feltro, junto da faixa
        bool cima = ficha_py[w] < CENTRO;
        int ty = cima ? MESA_Y0 + 6 + sobe : MESA_Y1 - TXT_H - 8 - sobe;
        txt_sombra_c(ficha_px[w] + 16, ty, mais, C_OURO, 1);
    }
}

// As fichas da faixa: no resultado, sobem (ou descem) enquanto o pote voa.
static int fichas_vis(int j)
{
    if (fase != F_RESULTADO || res_quem < 0) return J[j].fichas;
    int n = (J[res_quem].fichas - res_antes[res_quem]) / 8 + 3;
    if (n > 10) n = 10;
    float fim = 0.05f + n * 0.07f + 0.55f, q = (t_fase - 0.3f) / (fim - 0.3f);
    if (q >= 1) return J[j].fichas;
    if (q < 0) q = 0;
    return res_antes[j] + (int)((J[j].fichas - res_antes[j]) * q);
}

#define GRADE 12                     // dados por linha na escolha do que sai

static void desenha_premio(void)
{
    jogador_t *p = &J[vez];
    char t[64];
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);

    if (removendo) {
        // Escolha de qual dado sai da colecao: a colecao inteira em duas linhas.
        snprintf(t, sizeof t, "%s: qual dos %d dados sai?", p->nome, p->n_col);
        txt_c(GFX_W / 2, MESA_Y0 + 14, t, p->cor, true);
        teclas_c(GFX_W / 2, MESA_Y0 + 40, "ENTER tira para sempre  ·  BACKSPACE volta", C_TEXTO_M);
        for (int i = 0; i < p->n_col; i++) {
            int lin = i / GRADE, col = i % GRADE;
            int na_linha = (p->n_col - lin * GRADE) < GRADE ? p->n_col - lin * GRADE : GRADE;
            int x = GFX_W / 2 + (int)((col - (na_linha - 1) / 2.0f) * 46);
            int y = 134 + lin * 52;
            if (t_tira > 0 && i == tira_i) {
                // O escolhido encolhe e cai, deixando um rastro de po.
                float q = t_tira / 0.7f;
                float r = 17 * (1 - q);
                if (r > 2) peca_parada((float)x, y + q * 24, r, p->col[i], vez);
                for (int k = 0; k < 7; k++)
                    gfx_rect(x - 14 + k * 4, y + (int)(q * 34) + (k % 2) * 6, 3, 3, C_TEXTO_M);
                continue;
            }
            if (i == cursor) ui_selecao(x - 22, y - 22, 44, 44, C_VINHO_CLR);
            peca_parada((float)x, (float)y, 17, p->col[i], vez);
            if (i != cursor) marca_rar(x, y + 23, p->col[i].tipo);
            if (i == cursor) gfx_rect(x - 16, y + 23, 32, 3, C_VINHO_CLR);
            zona_clique(x - 22, y - 22, 44, 44, &cursor, i, KEY_ENTER);
        }
        botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar", KEY_BKSP);
        int tc = p->col[cursor < p->n_col ? cursor : 0].tipo;
        txt_c(GFX_W / 2, 236, TIPO[tc].nome, cor_dado(tc), true);
        txt_c(GFX_W / 2, 258, t_tira > 0 ? "some para sempre" : TEXTO_MEDIO[tc],
              t_tira > 0 ? C_VINHO_CLR : C_MARFIM_S, false);
        return;
    }

    if (modo == M_DESAFIO) snprintf(t, sizeof t, "Prêmio por vencer %s", J[1].nome);
    else snprintf(t, sizeof t, "%s levou a rodada e escolhe um prêmio", p->nome);
    txt_c(GFX_W / 2, MESA_Y0 + 14, t, p->cor, true);
    bool girando = slot_girando();
    for (int i = 0; i < 3; i++) {
        int x = GFX_W / 2 + (i - 1) * 150;
        int t2 = oferta[i].tipo;
        float para = slot_para(i);
        bool parado = t_slot < 0 || t_slot >= para;
        bool cur = i == cursor && !girando;
        uint16_t cr = cor_rar(t2);
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
                if (k > 0) {
                    uint32_t h = (uint32_t)(k * 2654435761u) ^ (uint32_t)(i * 40503u + 17);
                    // So os tipos que podem sair aqui: os basicos, ou os ja
                    // abertos no Desafiante.
                    pc.tipo = (uint8_t)(h % D_BASICOS);
                    if (modo == M_DESAFIO && dado_liberado((int)(h % D_N))) pc.tipo = (uint8_t)(h % D_N);
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
        if (acende) ui_cartao(x - 64, 90, 128, 110, true, gfx_mistura(cr, C_BRANCO, (int)(60 * (1 - q))));
        float pulo = acende ? sinf(q * 3.14159f) * -8 : 0;
        float bob = cur ? sinf(t_fase * 4) * 3 : 0;
        peca_parada((float)x, 130 + bob + pulo, 26 * (acende ? 1 + 0.12f * (1 - q) : 1), oferta[i], vez);
        txt_c(x, 162, TIPO[t2].nome, cur || acende ? cor_dado(t2) : C_MARFIM_S, true);
        char c2[32];
        classe(t2, c2, sizeof c2);
        txt_c(x, 180, c2, cr, false);
        zona_clique(x - 64, 90, 128, 110, &cursor, i, KEY_ENTER);
    }
    bool pode = p->n_col > N_FILA;
    if (girando) {
        txt_c(GFX_W / 2, 212, "sorteando...", C_OURO, true);
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
        float bob = cur && !ja ? sinf(t_fase * 4) * 3 : 0;
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
