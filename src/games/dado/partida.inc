// ===========================================================================
// Partida e rodada
// ===========================================================================
// Oito dados de saida: com a mao de 7, a compra ja pesa desde a 1a rodada.
static void colecao_inicial(jogador_t *p)
{
    static const uint8_t INICIO[] = { D_D6, D_D6, D_D6, D_D6, D_D4, D_D4, D_D2, D_D2 };
    p->n_col = 0;
    for (unsigned i = 0; i < sizeof INICIO; i++) {
        p->col[p->n_col] = nova_peca(INICIO[i]);
        p->onde[p->n_col++] = NA_BOLSA;
    }
}

static int na_bolsa(const jogador_t *p)
{
    int n = 0;
    for (int i = 0; i < p->n_col; i++) if (p->onde[i] == NA_BOLSA) n++;
    return n;
}

// A mao da rodada anterior sai da mesa.
static void descarta_mao(jogador_t *p)
{
    for (int i = 0; i < p->n_col; i++) if (p->onde[i] == NA_MAO) p->onde[i] = FORA;
    p->n_mao = 0;
}

// Compra ate 7 da bolsa, ao acaso. Se a bolsa esvazia no meio, tudo o que
// esta fora volta para ela - menos o que ja esta na mao - e a compra segue.
// Quantos j compra por rodada: 7; 8 com a Bolsa Funda; 6 se o oponente tem
// o Saco Furado.
static int tam_mao(int j)
{
    int n = MAO_NORMAL;
    if (tem_am(j, A_BOLSA)) n++;
    if (tem_am(j ^ 1, A_FURADO)) n--;
    if (j == 1 && op_nervoso && tem_reg(REG_NERVOSO)) n--;
    if (j == 0 && virada_forca(0) == 2) n--;            // o Crupie forte: voce compra 1 a menos
    return n;
}

static void compra(jogador_t *p)
{
    p->embaralhou = false;
    int quer = tam_mao((int)(p - J));
    while (p->n_mao < quer) {
        int n = na_bolsa(p);
        if (!n) {
            bool voltou = false;
            for (int i = 0; i < p->n_col; i++)
                if (p->onde[i] == FORA) { p->onde[i] = NA_BOLSA; voltou = true; }
            if (!voltou) break;                        // todos ja estao na mao
            p->embaralhou = true;
            n = na_bolsa(p);
        }
        int alvo = sorte(n);
        for (int i = 0; i < p->n_col; i++)
            if (p->onde[i] == NA_BOLSA && alvo-- == 0) {
                p->onde[i] = NA_MAO;
                p->mao[p->n_mao++] = (int8_t)i;
                break;
            }
    }
}

static void encerra(int vencedor)
{
    venc_partida = vencedor;
    fase = F_FIM;
    // Contra o computador (ou online), perder tem o seu som; com duas pessoas
    // no mesmo teclado, alguem sempre ganhou.
    bool perdeu = modo != M_DOIS && vencedor != baixo;
    som_toca(vencedor == 2 && modo == M_DOIS ? SOM_EMPATE : (perdeu ? SOM_DERROTA : SOM_FIM));
    if (modo == M_DESAFIO) {
        fala_evento(vencedor == 1 ? FA_GANHA_P : FA_PERDE_P);
        des_resultado();                      // a run anota na hora
    }
}

static void des_virou(void);

static void nova_rodada(void)
{
    rodada++;
    plano_rodada = -1;                       // a IA monta outra fila
    ferr_k[0] = ferr_k[1] = -1;
    // A Detetive joga por ultimo na primeira rodada e quando esta atras em fichas.
    if (tem_reg(REG_ULTIMA) && (rodada == 1 || J[1].fichas < J[0].fichas)) primeiro = 0;
    // Relogio de Bolso: na primeira rodada e quando voce esta atras em fichas,
    // o oponente abre (vale mais).
    if (tem_am(0, A_RELOGIO) && (rodada == 1 || J[0].fichas < J[1].fichas)) primeiro = 1;
    for (int j = 0; j < 2; j++) fichas_ini[j] = J[j].fichas;   // para o Cobrador
    // No Desafiante sao 8 rodadas; empatado no fim, ate 3 rodadas extras.
    int max_r = modo == M_DESAFIO ? DES_RODADAS : MAX_RODADAS;
    bool extra = modo == M_DESAFIO && J[0].fichas == J[1].fichas && rodada <= max_r + DES_EXTRAS;
    if (rodada > max_r && !extra) {
        encerra(J[0].fichas == J[1].fichas ? 2 : (J[0].fichas > J[1].fichas ? 0 : 1));
        return;
    }
    // Quem nao paga a entrada perde. Se nenhum dos dois paga, vence quem tem
    // mais fichas (o pote parado na mesa se dividiria igual e nao muda isso);
    // com fichas iguais, empate.
    int ante = ante_da(rodada);
    bool sem0 = J[0].fichas < ante, sem1 = J[1].fichas < ante;
    if (sem0 && sem1) {
        encerra(J[0].fichas == J[1].fichas ? 2 : (J[0].fichas > J[1].fichas ? 0 : 1));
        return;
    }
    if (sem0 || sem1) { encerra(sem0 ? 1 : 0); return; }
    if (rodada == VIRADA_RODADA && fala_perso >= PERSO_CHEFE0 && fala_perso < PERSO_CHEFE0 + 3
        && virada_forca(fala_perso - PERSO_CHEFE0))
        des_virou();                             // o chefe vira: anuncio, retrato e fala
    if (rodada > 1 && ante > ante_da(rodada - 1)) {     // subiu: avisa, com faixa e fichas
        t_aviso_entrada = ENTRADA_AVISO_T;
        som_toca(SOM_FICHA);
    }
    for (int j = 0; j < 2; j++) {
        J[j].fichas -= ante;
        memset(&F[j], 0, sizeof F[j]);
        descarta_mao(&J[j]);
        compra(&J[j]);
    }
    pote += 2 * ante;
    n_voo = 0;
    n_ev = ev_i = 0;
    memset(vis_nroubo, 0, sizeof vis_nroubo);
    memset(vis_delta, 0, sizeof vis_delta);
    memset(vis_bonus, 0, sizeof vis_bonus);
    memset(vis_mult, 0, sizeof vis_mult);
    memset(vis_queima, 0, sizeof vis_queima);
    memset(vis_espelho, 0, sizeof vis_espelho);
    memset(vis_semente, 0, sizeof vis_semente);
    memset(vis_zerada, 0, sizeof vis_zerada);
    aviso[0] = veredito[0] = nota[0] = 0;
    vez = primeiro;
    cursor = 0;
    t_fase = 0;
    fase = F_ORDEM;
    som_toca(rodada == 1 ? SOM_PARTIDA : SOM_BOLSA);   // comeca a partida, ou mais uma rodada
}

// Nomes de cada modo: quem joga aqui e sempre VOCE (ou JOGADOR 1 e 2).
static void poe_nomes(void)
{
    switch (modo) {
    case M_UM:     J[0].nome = "VOCÊ";      J[1].nome = "COMPUTADOR"; break;
    case M_DOIS:   J[0].nome = "JOGADOR 1"; J[1].nome = "JOGADOR 2";  break;
    default:       J[baixo].nome = "VOCÊ";  J[outro(baixo)].nome = "OPONENTE"; break;
    }
}

static void abre_loja(void);

void dado_inicia(int n_partidas)
{
    static bool paleta_pronta;
    if (!paleta_pronta) {
        gfx_paleta(PALETA, (int)(sizeof PALETA / sizeof PALETA[0]));
        gfx_paleta_exata(LUZ_CORPO, (int)(sizeof LUZ_CORPO / sizeof LUZ_CORPO[0]));
        paleta_pronta = true;
    }
    if (!rapido_lido) {                  // a velocidade escolhida da ultima vez
        char b[8];
        rapido = salva_le("vel", b, sizeof b) > 0 && b[0] == '1';
        rapido_lido = true;
    }
    partidas = n_partidas;
    semente_v = (uint32_t)esp_timer_get_time() | 1;
    // Online, os dois lados sorteiam igual: a semente vem da sala.
    if (modo == M_ONLINE) semente = (semente_online ^ (uint32_t)n_partidas * 0x9E3779B9u) | 1;
    else                  semente = (uint32_t)esp_timer_get_time() | 1;
    som_rufo(false);
    for (int i = 0; i < 8; i++) rnd();
    memset(J, 0, sizeof J);
    J[0].cor = C_LATAO;
    J[1].cor = C_ROSA;
    poe_nomes();
    for (int j = 0; j < 2; j++) {
        J[j].fichas = FICHAS_INI;
        colecao_inicial(&J[j]);
    }
    rodada = 0;
    pote = 0;
    primeiro = partidas % 2;
    venc_partida = -1;
    correu_venc = -1;
    menu_tela = MT_PRINCIPAL;

    uint8_t v[sizeof VITRINE];
    memcpy(v, VITRINE, sizeof v);
    for (int i = (int)sizeof v - 1; i > 0; i--) {
        int j = sorte(i + 1);
        uint8_t t = v[i]; v[i] = v[j]; v[j] = t;
    }
    for (int i = 0; i < N_LOJA; i++) loja[i] = nova_peca(v[i]);
    memset(levou, 0, sizeof levou);
    loja_primeiro = primeiro;

    n_voo = 2;
    memset(voo, 0, sizeof voo);
    voo[0] = (voo_t){ .x = 60, .y = 270, .vx = 650, .vy = -180, .spin = 11,
                      .dono = 0, .tipo = D_D6, .valor = 6, .face = 1 };
    voo[1] = (voo_t){ .x = 580, .y = 245, .vx = -650, .vy = -110, .spin = -11,
                      .dono = 1, .tipo = D_D6, .valor = 6, .face = 3 };
    abertura_pronta = false;
    t_fase = 0;
    fase = F_ABERTURA;
    t_sair = -1;
    if (modo == M_ONLINE) abre_loja();     // online, a revanche vai direto a loja
}

static void abre_loja(void)
{
    vez = loja_primeiro;
    cursor = 0;
    n_voo = 0;
    fase = F_LOJA;
}

// O que o dado de tipo t tira, com os poderes que mexem no sorteio. 'r' e
// quem sorteia: o da partida ou o da IA quando ela imagina jogadas. No
// Viciado, 'menor' recebe o lance descartado.
static int sorteia_valor(int t, int fixo, int (*r)(int), int *menor)
{
    int lados = TIPO[t].lados;
    int x = r(lados);
    *menor = 0;
    switch (TIPO[t].efeito) {
    case EF_VICIADO: {                       // dois lances; fica o maior
        int y = r(lados);
        *menor = x < y ? x : y;
        if (y > x) x = y;
        break;
    }
    case EF_LASTRO:  x = 3 + r(9); break;    // 4 a 12, 1 em 9 cada
    case EF_EXPLODE: {
        int n = x, g = 0;
        while (n == lados && g++ < 3) { n = r(lados); x += n; }
        break;
    }
    case EF_QUEBRADO: x = fixo; break;       // sem sorteio
    default: break;
    }
    return x;
}

static void lanca_proximo(void)
{
    fila_t *f = &F[vez];
    int k = f->lancados;
    int t = tipo_de(vez, k), lados = TIPO[t].lados;

    // Acumulador: cada lance guarda +1 nele, para sempre (ate +6).
    if (t == D_ACUMULADOR && f->idx[k] >= 0 && f->idx[k] < J[vez].n_col) {
        peca_t *pc = &J[vez].col[f->idx[k]];
        if (pc->fixo < 6) pc->fixo++;
        f->fixo[k] = pc->fixo;
    }
    int menor;
    int x = sorteia_valor(t, f->fixo[k], rola, &menor);
    f->cru[k] = x;
    f->valor[k] = x;

    voo_t *v = &voo[0];
    memset(v, 0, sizeof *v);
    v->dono = (uint8_t)vez;
    v->slot = (uint8_t)k;
    v->tipo = (uint8_t)t;
    v->valor = x;
    v->x = GFX_W / 2 + frnd(-110, 110);
    v->y = vez == baixo ? MESA_Y1 - R_DADO - 4 : MESA_Y0 + R_DADO + 4;
    v->vx = frnd(-300, 300);
    v->vy = (vez == baixo ? -1 : 1) * frnd(420, 600);
    v->spin = frnd(-14, 14);
    v->face = face_v(t, x);
    n_voo = 1;
    f->descarte[k] = (uint8_t)menor;
    // O Viciado rola de verdade dois dados: o do menor valor e um fantasma
    // que some quando o maior vai para a fila.
    if (menor) {
        voo_t *w = &voo[1];
        *w = *v;
        w->fantasma = true;
        w->valor = menor;
        w->x = v->x + (v->x < GFX_W / 2 ? 60 : -60);
        w->vx = -v->vx * 0.8f + frnd(-60, 60);
        w->vy = v->vy * frnd(0.8f, 1.0f);
        w->spin = -v->spin;
        w->face = rola_v(lados);
        n_voo = 2;
    }
    if (lados == 2) som_rufo_moeda();        // a moeda girando ate deitar
    else som_rufo(true);                     // o dado rolando ate assentar
    t_fase = 0;
    fase = F_ROLANDO;
}

static void comeca_segundo(void)
{
    vez = outro(primeiro);
    cursor = 0;
    t_fase = 0;
    fase = F_ORDEM;
}

// Quebrados jogados saem da bolsa de quem os tinha. A bolsa nunca cai abaixo
// de quatro: se cairia, o Quebrado vira um d4 comum.
// Quebrados jogados saem da colecao de quem os tinha. A colecao nunca cai
// abaixo de quatro: se cairia, o Quebrado vira um d4 comum.
// Dados que saem do jogo ao fim da rodada: o Quebrado que foi jogado e o que
// a Ficha de Fogo queimou. A colecao nunca cai abaixo de quatro: se cairia, o
// dado perdido vira um d4 comum.
static void recolhe_quebrados(void)
{
    int n_quebra = 0, n_cinza = 0, n_vidro = 0;
    for (int j = 0; j < 2; j++) {
        int saem[N_FILA], n = 0;
        for (int k = 0; k < F[j].lancados; k++) {
            bool some = false;
            if (F[j].tipo[k] == D_QUEBRADO) { F[j].quebra[k] = true; some = true; n_quebra++; }
            if (F[j].tipo[k] == D_VIDRO && F[j].cru[k] == 1) { F[j].quebra[k] = true; some = true; n_vidro++; }
            if (vis_queima[j][k] && !some)  { F[j].cinza[k] = true; some = true; n_cinza++; }
            if (some) saem[n++] = F[j].idx[k];
        }
        for (int a = 1; a < n; a++)
            for (int b = a; b > 0 && saem[b] > saem[b - 1]; b--) {
                int t = saem[b]; saem[b] = saem[b - 1]; saem[b - 1] = t;
            }
        jogador_t *p = &J[j];
        for (int i = 0; i < n; i++) {
            if (p->n_col <= N_FILA) { p->col[saem[i]] = nova_peca(D_D4); continue; }
            for (int m = saem[i]; m + 1 < p->n_col; m++) {
                p->col[m] = p->col[m + 1];
                p->onde[m] = p->onde[m + 1];
            }
            p->n_col--;
        }
        p->n_mao = 0;
    }
    if (n_cinza && n_quebra) snprintf(nota, sizeof nota, "um dado vira cinza, outro se quebra");
    else if (n_cinza)        snprintf(nota, sizeof nota, n_cinza > 1 ? "os dados queimados viram cinza" : "o dado queimado vira cinza");
    else if (n_quebra)       snprintf(nota, sizeof nota, n_quebra > 1 ? "Quebrados se desfazem" : "o Quebrado se desfaz");
    if (n_vidro && !n_cinza && !n_quebra) snprintf(nota, sizeof nota, "cacos de vidro saem da mesa");
    else if (n_vidro)        snprintf(nota, sizeof nota, "dados se desfazem, e o vidro também");
}

// O desfecho calculado pelo motor de regras: e ele que paga. Os eventos so
// contam na tela, um a um, como se chegou nele.
static desfecho_t resolvido;
// O pote indo para quem levou: de quanto a quanto vai cada placar, e para onde
// as fichas voam (a ficha de cada faixa, guardada quando a faixa e desenhada).
static int res_antes[2], res_quem = -1;
static int ficha_px[2] = { 160, 160 }, ficha_py[2] = { GFX_H - 22, 22 };

// Depois da animacao dos eventos: paga o pote e declara quem levou.
static void conclui_rodada(void)
{
    res_antes[0] = J[0].fichas; res_antes[1] = J[1].fichas;
    res_quem = -1;
    for (int j = 0; j < 2; j++) {
        memcpy(F[j].est, resolvido.est[j], N_FILA);
        memcpy(vis_est[j], resolvido.est[j], N_FILA);
        memcpy(vis_queima[j], resolvido.queima[j], N_FILA);
        F[j].total = resolvido.total[j];
    }
    bool casa = F[0].total == F[1].total && tem_reg(REG_CASA);   // o Crupie leva o empate
    if (F[0].total == F[1].total && !casa) {
        venc_mao = 2;
        som_toca(SOM_EMPATE);
        snprintf(aviso, sizeof aviso, "empate em %d: o pote fica na mesa", F[0].total);
    } else {
        int w = casa ? 1 : (F[0].total > F[1].total ? 0 : 1);
        venc_mao = w;
        res_quem = w;
        J[w].fichas += pote;
        if (casa) snprintf(aviso, sizeof aviso, "empate: a casa leva %d fichas", pote);
        else snprintf(aviso, sizeof aviso, "%s leva %d fichas", J[w].nome, pote);
        if (w == 1 && tem_reg(REG_BATEDOR) && J[0].fichas > 0) {   // o Ladrao leva mais 2 suas
            int b = J[0].fichas < 2 ? J[0].fichas : 2;
            J[0].fichas -= b;
            J[1].fichas += b;
            snprintf(aviso, sizeof aviso, "%s leva %d fichas, e mais %d suas", J[w].nome, pote, b);
        }
        if (tem_am(w, A_POTE)) {                 // Pote Gordo: 2 a mais
            J[w].fichas += 2;
            snprintf(aviso, sizeof aviso, "%s leva %d fichas (+2)", J[w].nome, pote);
            am_acende(A_POTE);
        }
        pote = 0;
        som_toca(SOM_VITORIA);
    }
    int aluguel = virada_forca(1);                // a Dona cobra o aluguel, ganhe quem ganhar
    if (aluguel && J[0].fichas > 0) {
        int a = aluguel == 2 ? 5 : 3;
        if (a > J[0].fichas) a = J[0].fichas;
        J[0].fichas -= a;
        J[1].fichas += a;
        size_t n = strlen(aviso);
        snprintf(aviso + n, sizeof aviso - n, "  ·  aluguel: %d", a);
    }
    op_nervoso = venc_mao == 0;
    if (venc_mao == 0) des_rod_vencidas++;
    if (venc_mao == 0) fala_evento(FA_PERDE_R);
    else if (venc_mao == 1) fala_evento(FA_GANHA_R);
    recolhe_quebrados();
    t_fase = 0;
    fase = F_RESULTADO;
}

// O Jackpot tem som proprio; os outros poderes, o brilho de sempre.
// Cada poder com o seu som: o Jackpot tem o dele; anular, tirar pontos,
// roubar, queimar e zerar tambem; o resto, o brilho de sempre.
static int som_do_evento(int i)
{
    const evento_t *e = &ev[i];
    switch (e->tipo) {
    case EV_JACKPOT: return SOM_JACKPOT;
    case EV_ANULA:   return F[e->de_j].tipo[e->de_k] == D_LASER ? SOM_LASER : SOM_ANULA;
    case EV_CAVALO:  return SOM_ANULA;
    case EV_TIRA: case EV_PIRATA: return SOM_TIRA;
    case EV_ROUBA:   return SOM_ROUBA;
    case EV_FOGO: case EV_RENASCE: return SOM_FOGO;
    case EV_ZERA:    return SOM_ZERA;
    case EV_BONUS:   return e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_FERREIRO ? SOM_BIGORNA : SOM_EFEITO;
    default:         return SOM_EFEITO;
    }
}

static void desfecho(void)
{
    calcula_desfecho(&resolvido, true);
    for (int j = 0; j < 2; j++) {
        memcpy(vis_est[j], F[j].est, N_FILA);
        vis_nroubo[j] = 0;
        vis_delta[j] = 0;
        vis_semente[j] = 0;
        vis_zerada[j] = 0;
        memset(vis_bonus[j], 0, N_FILA);
        memset(vis_mult[j], 0, N_FILA);
        memset(vis_queima[j], 0, N_FILA);
        memset(vis_espelho[j], 0, N_FILA);
    }
    for (int i = 0; i < n_ev; i++)
        if (ev[i].tipo == EV_ESPELHO) {
            vis_espelho[ev[i].de_j][ev[i].de_k] = 1;
            memcpy(vis_est[ev[i].de_j], est_pre_espelho[ev[i].de_j], N_FILA);
        }
    n_voo = 0;
    ev_i = 0;
    t_fase = 0;
    for (int j = 0; j < 2; j++) log_base[j] = total_visivel(j);
    if (n_ev) { fase = F_EFEITOS; som_toca(som_do_evento(0)); }
    else conclui_rodada();
}

static float duracao_evento(int i)
{
    switch (ev[i].tipo) {
    case EV_ROUBA: return 1.6f;
    case EV_MULT:  return 1.7f;
    case EV_FOGO:  return 1.6f;
    case EV_SEMENTE: return 1.6f;
    case EV_EXTRA:   return ev[i].mascara ? 1.9f : 1.6f;    // o Capitao passa por cada dado
    case EV_CHEFE:   return 1.6f;
    case EV_CAVALO:  return 2.0f;
    case EV_RENASCE: return 1.5f;
    case EV_JACKPOT: return 2.4f;
    case EV_ZERA:  return 1.6f;
    case EV_BONUS: return 1.4f;
    default:       return 1.3f;
    }
}

// Onde sai o numero flutuante de um dado (k < N_FILA) ou do placar de j: da
// beira do lado do meio da mesa.
static int flut_x(int k) { return k < N_FILA ? SLOT_X(k) : PLACAR_X; }
static int flut_d(int j) { return j == baixo ? -1 : 1; }
static int flut_y(int j, int k)
{
    int r = k < N_FILA ? R_DADO + 3 : 30;
    return j == baixo ? zona(j) - r : zona(j) + r;
}
static void flutua_em(int j, int k, uint16_t cor, float atraso, const char *fmt, int v)
{
    flutua(flut_x(k), flut_y(j, k), flut_d(j), cor, atraso, fmt, v);
}

// O dado k de j acabou de pousar: o que ele vale sobe dele; o que caiu com
// ele mostra o que perdeu.
static void flutua_pouso(int j, int k)
{
    const fila_t *f = &F[j];
    if (f->est[k] == V_VALIDO) {
        int v = valor_pontos(j, k) + f->bonus[k];
        if (v) flutua_em(j, k, C_OURO, 0, "+%d", v);
    }
    uint8_t caiu = anulou_masc;
    if (anulou_a >= 0) caiu |= (uint8_t)(1 << anulou_a);
    if (anulou_b >= 0) caiu |= (uint8_t)(1 << anulou_b);
    for (int i = 0; i < k; i++)
        if ((caiu >> i & 1) && f->est[i] != V_VALIDO) {
            int v = valor_pontos(j, i) + f->bonus[i];
            if (v) flutua_em(j, i, C_VINHO_CLR, 0.08f, "-%d", v);
        }
}

// Os numeros de um evento do fim da rodada, quando ele se aplica.
static void flutua_evento(const evento_t *e)
{
    int dj = e->de_j, dk = e->de_k < N_FILA ? e->de_k : N_FILA, aj = e->alvo_j, ak = e->alvo_k;
    switch (e->tipo) {
    case EV_BONUS: case EV_MULT:
        for (int k = 0; k < N_FILA; k++)
            if (e->por_dado[k] > (e->tipo == EV_MULT ? 1 : 0))
                flutua_em(dj, k, C_OURO, k * 0.06f, e->tipo == EV_MULT ? "x%d" : "+%d", e->por_dado[k]);
        break;
    case EV_ANULA:   flutua_em(aj, ak, C_VINHO_CLR, 0, "-%d", e->valor); break;
    case EV_TIRA:    flutua_em(aj, N_FILA, C_VINHO_CLR, 0, "-%d", e->valor); break;
    case EV_PIRATA:
        flutua_em(aj, N_FILA, C_VINHO_CLR, 0, "-%d", e->valor);
        flutua_em(dj, N_FILA, C_OURO, 0.1f, "+%d", e->valor);
        break;
    case EV_ROUBA:   flutua_em(dj, N_FILA, C_OURO, 0, "+%d", e->valor); break;
    case EV_EXTRA:
        if (e->mascara) {                   // Capitao: +2 em cada dado seu que vale
            for (int k = 0; k < N_FILA; k++)
                if (e->mascara >> k & 1) flutua_em(dj, k, C_OURO, k * 0.06f, "+%d", e->por_dado[k]);
            break;
        }
        if (e->valor) flutua_em(dj, dk, C_OURO, 0, "+%d", e->valor);
        break;
    case EV_FOGO: case EV_SEMENTE: case EV_JACKPOT: case EV_RENASCE:
        if (e->valor) flutua_em(dj, dk, C_OURO, 0, "+%d", e->valor);
        break;
    case EV_CAVALO:
        for (int k = 0; k < N_FILA; k++)
            if (e->mascara >> k & 1) flutua_em(aj, k, C_VINHO_CLR, k * 0.06f, "-%d", e->por_dado[k]);
        if (e->valor) flutua_em(aj, N_FILA, C_VINHO_CLR, 0.1f, "-%d", e->valor);   // o que tinha roubado
        break;
    case EV_CHEFE:   flutua_em(dj, N_FILA, C_VINHO_CLR, 0, "+%d", e->valor); break;
    default: break;
    }
}

// O evento i terminou: a mesa passa a mostrar o efeito dele.
static void aplica_evento(int i)
{
    evento_t *e = &ev[i];
    flutua_evento(e);
    switch (e->tipo) {
    case EV_ANULA:  vis_est[e->alvo_j][e->alvo_k] = V_ANULADO; break;
    case EV_ROUBA: {
        vis_est[e->alvo_j][e->alvo_k] = V_ROUBADO;
        int n = vis_nroubo[e->de_j];
        if (n < 4) {
            vis_roubo[e->de_j][n] = (roubado_t){ .tipo = F[e->alvo_j].tipo[e->alvo_k],
                                                 .fixo = F[e->alvo_j].fixo[e->alvo_k],
                                                 .de_j = e->alvo_j, .valor = e->valor };
            vis_nroubo[e->de_j] = n + 1;
        }
        break;
    }
    case EV_ESPELHO:                    // virou: a sequencia refeita aparece
        vis_espelho[e->de_j][e->de_k] = 0;
        memcpy(vis_est[e->de_j], F[e->de_j].est, N_FILA);
        break;
    case EV_TIRA:   vis_delta[e->alvo_j] -= e->valor; break;
    case EV_PIRATA: vis_delta[e->alvo_j] -= e->valor; vis_delta[e->de_j] += e->valor; break;
    case EV_BONUS:
        for (int k = 0; k < N_FILA; k++) vis_bonus[e->de_j][k] += e->por_dado[k];
        break;
    case EV_MULT:
        for (int k = 0; k < N_FILA; k++) vis_mult[e->de_j][k] = e->por_dado[k];
        break;
    case EV_FOGO:    vis_queima[e->alvo_j][e->alvo_k] = 1; vis_semente[e->de_j] += e->valor; break;
    case EV_SEMENTE: case EV_JACKPOT: case EV_EXTRA: case EV_CHEFE: vis_semente[e->de_j] += e->valor; break;
    case EV_RENASCE: vis_est[e->de_j][e->de_k] = V_VALIDO; break;
    case EV_ZERA:    vis_zerada[e->de_j] = 1; break;
    case EV_CAVALO:                     // tudo o que era de quem atingiu o Cavalo cai
        for (int k = 0; k < N_FILA; k++)
            if (e->mascara >> k & 1) vis_est[e->alvo_j][k] = V_ANULADO;
        for (int r = 0; r < vis_nroubo[e->alvo_j]; r++) vis_roubo[e->alvo_j][r].anulado = 1;
        break;
    }
}

// A maquina joga contra uma pessoa? Entao os menus dela nao aparecem, e o que
// ela decide vira uma linha curta (maquina contra maquina, nos testes, nao).
static bool anuncia_maquina(void)
{
    return (cpu >> vez & 1) && !(cpu >> outro(vez) & 1);
}

static void anuncia(const char *fmt, const char *quem, int v)
{
    snprintf(anuncio, sizeof anuncio, fmt, quem, v);
    anuncio_cor = J[vez].cor;
    t_anuncio = 1.3f;
    n_anuncio_peca = 0;
}

// O que a maquina levou da loja ou do premio: o texto e os dados, por mais
// tempo, para dar para ver cada um.
static void anuncia_pecas(const char *fmt, const peca_t *p, int n)
{
    anuncia(fmt, J[vez].nome, 0);
    for (int i = 0; i < n && i < 5; i++) anuncio_peca[i] = p[i];
    n_anuncio_peca = n < 5 ? n : 5;
    anuncio_dono = vez;
    t_anuncio = n ? 2.2f + 0.4f * n : 1.3f;
}

// Fim do turno de j: os dados da sequencia que ele nao lancou saem da mesa
// e da mao e voltam para a bolsa (podem sair de novo na proxima compra).
static void devolve_sobras(int j)
{
    fila_t *f = &F[j];
    jogador_t *p = &J[j];
    n_devolvidos = 0;
    for (int k = f->lancados; k < f->n && k < N_FILA; k++) {
        int c = f->idx[k];
        if (c < 0 || c >= p->n_col) continue;
        p->onde[c] = NA_BOLSA;
        for (int i = 0; i < p->n_mao; i++)
            if (p->mao[i] == c) {
                memmove(&p->mao[i], &p->mao[i + 1], (size_t)(p->n_mao - i - 1));
                p->n_mao--;
                break;
            }
        devolvidos[n_devolvidos++] = (devolvido_t){ (float)SLOT_X(k), (float)zona(j), f->tipo[k], (uint8_t)j, f->fixo[k] };
        f->idx[k] = 0; f->tipo[k] = 0; f->fixo[k] = 0; f->tag[k][0] = 0;
    }
    if (f->n > f->lancados) f->n = f->lancados;
    t_devolve = n_devolvidos ? 0 : -1;
}

static void termina_turno(void)
{
    som_toca(SOM_TURNO);
    devolve_sobras(vez);
    if (anuncia_maquina() && vez == primeiro)
        anuncia("%s parou com %d pontos", J[vez].nome, F[vez].pontos);
    if (vez == primeiro) {
        a_pagar = 0; passes = 0; aumentou = false;
        cursor = 0;
        fase = F_APOSTA;
    } else {
        desfecho();
    }
}
