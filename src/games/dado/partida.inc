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

// Os dados da bolsa que ainda jogam nesta partida: o que a Autofagia comeu
// so volta na proxima.
static int em_jogo(const jogador_t *p)
{
    int n = 0;
    for (int i = 0; i < p->n_col; i++) if (p->onde[i] != COMIDO) n++;
    return n;
}

// A mao da rodada anterior sai da mesa.
// O dado do Velcro gruda: em vez de sair, volta para a mao (e desgruda).
static void descarta_mao(jogador_t *p)
{
    p->n_mao = 0;
    for (int i = 0; i < p->n_col; i++) {
        if (p->col[i].velcro && p->onde[i] != COMIDO && p->n_mao < MAO) {
            p->col[i].velcro = 0;
            p->onde[i] = NA_MAO;
            p->mao[p->n_mao++] = (int8_t)i;
        } else if (p->onde[i] == NA_MAO) p->onde[i] = FORA;
        p->col[i].velcro = 0;
    }
}

// Compra ate 7 da bolsa, ao acaso. Se a bolsa esvazia no meio, tudo o que
// esta fora volta para ela - menos o que ja esta na mao - e a compra segue.
// Quantos j compra por rodada: 7; 8 com a Bolsa Funda; 6 se o oponente tem
// o Saco Furado.
static int bis_extra[2];                 // dados a mais na proxima compra (o adesivo Bis)
static int ad_moedas[2];                 // moedas do Dourado nesta partida
static int tam_mao(int j)
{
    int n = MAO_NORMAL + bis_extra[j];
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
    if (quer > MAO) quer = MAO;
    bis_extra[p - J] = 0;                        // o Bis vale para uma compra
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
    // A mao em ordem: do dado de menos lados ao de mais (d2, d4, d6, d8, d12,
    // d20); no mesmo tamanho, os do mesmo tipo juntos.
    for (int a = 1; a < p->n_mao; a++) {
        int8_t c = p->mao[a];
        int t = p->col[c].tipo, k = a;
        while (k > 0) {
            int u = p->col[p->mao[k - 1]].tipo;
            if (TIPO[u].lados < TIPO[t].lados || (TIPO[u].lados == TIPO[t].lados && u <= t)) break;
            p->mao[k] = p->mao[k - 1];
            k--;
        }
        p->mao[k] = c;
    }
}

static void some_fantasmas(void);

static void encerra(int vencedor)
{
    some_fantasmas();
    venc_partida = vencedor;
    fase = F_FIM;
    hist_partida(vencedor);
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

// O dado de j que tirou o valor mais alto no turno dele (empate: o ultimo).
static bool melhor_lance(int j, peca_t *out)
{
    const fila_t *f = &F[j];
    int b = -1;
    for (int k = 0; k < f->lancados && k < f->n; k++)
        if (!f->escondido[k] && (b < 0 || f->valor[k] >= f->valor[b])) b = k;   // o Oculto ainda nao mostrou
    if (b < 0) return false;
    *out = (peca_t){ .tipo = f->tipo[b], .fixo = f->fixo[b] };
    return true;
}

// O Presente surpresa do Papai Noel: no comeco da rodada, um dado da sua mao
// (nao um Presente de verdade) vira uma caixa embrulhada - na mao, na fila e
// na bolsa -, e so mostra o que e quando voce o lanca (desembrulha). No fim da
// rodada, lancado ou nao, volta a ser o que era.
static void desfaz_embrulho(void)
{
    if (embrulho >= 0 && embrulho < J[0].n_col && J[0].col[embrulho].tipo == D_PRESENTE)
        J[0].col[embrulho].tipo = embrulho_tipo;
    embrulho = -1;
}
static void embrulha_um(void)
{
    embrulho = -1;
    if (!tem_reg(REG_PRESENTE) || !J[0].n_mao) return;
    int ok[MAO], n = 0;
    for (int i = 0; i < J[0].n_mao; i++)
        if (J[0].col[J[0].mao[i]].tipo != D_PRESENTE) ok[n++] = J[0].mao[i];
    if (!n) return;
    embrulho = (int16_t)ok[rnd() % (uint32_t)n];
    embrulho_tipo = J[0].col[embrulho].tipo;
    J[0].col[embrulho].tipo = D_PRESENTE;
    if (rodada == 1) {
        snprintf(fala_txt, sizeof fala_txt, "Ho-ho-ho! Embrulhei um dos seus dados. Adivinha qual?");
        fala_comeca(3.0f);
        fala_e_curta = false;
    }
}

static void nova_rodada(void)
{
    rodada++;
    plano_rodada = -1;                       // a IA monta outra fila
    carta_virando = false;
    zerou = -1;
    vence_j = -1;
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
    if (rodada == VIRADA_RODADA && chefe_idx(fala_perso) >= 0 && virada_forca(chefe_idx(fala_perso)))
        des_virou();                             // o chefe vira: anuncio, retrato e fala
    if (rodada == VIRADA_RODADA && tem_reg(REG_CORINGA) && des_dificuldade() >= CORINGA_SOME) {   // o Coringa avisa: daqui, o que cai e dele
        snprintf(fala_txt, sizeof fala_txt, "Daqui em diante, dado seu que for anulado é meu! Hahaha!");
        fala_comeca(3.2f);
        fala_e_curta = false;
    }
    if (rodada > 1 && ante > ante_da(rodada - 1)) {     // subiu: avisa, com faixa e fichas
        t_aviso_entrada = ENTRADA_AVISO_T;
        som_toca(SOM_FICHA);
    }
    // Os Juros da Banqueira: comecou atras em fichas, a sua entrada custa 2 a
    // mais (o que der, sem passar do que voce tem).
    juros_rodada = 0;
    if (tem_reg(REG_JUROS) && J[0].fichas < J[1].fichas) {
        juros_rodada = J[0].fichas - ante < 2 ? J[0].fichas - ante : 2;
        J[0].fichas -= juros_rodada;
        pote += juros_rodada;
    }
    // A moeda do Maluco: cara, +2 nos dados dele nesta rodada; coroa, -1.
    if (tem_reg(REG_MALUCO)) {
        maluco_lado = (int8_t)((rnd() & 1) ? 2 : -1);
        snprintf(fala_txt, sizeof fala_txt, "%s", maluco_lado > 0 ? "Cara! Meus dados +2 nesta!" : "Coroa... -1 nos meus. Que seja!");
        fala_comeca(2.6f);
        fala_e_curta = false;
    }
    desfaz_embrulho();
    for (int j = 0; j < 2; j++) {
        J[j].fichas -= ante;
        memset(&F[j], 0, sizeof F[j]);
        descarta_mao(&J[j]);
        compra(&J[j]);
    }
    embrulha_um();
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
    le_epoca();                          // o Natal (o feltro e os dados dele)
    if (!paleta_pronta) {                // a paleta, com o feltro e a mesa do perfil
        estilo_do_perfil();
        paleta_pronta = true;
    }
    if (!rapido_lido) {                  // a velocidade escolhida da ultima vez
        char b[8];
        if (salva_le("vel", b, sizeof b) > 0)
            for (int v = 0; v < VEL_N; v++) if (b[0] == VEL_GRAVA[v]) velocidade = v;
        rapido_lido = true;
    }
    if (!tremor_lido) {                  // e se o tremor da tela foi desligado
        char b[8];
        tremor_off = salva_le("tremor", b, sizeof b) > 0 && b[0] == '0';
        tremor_lido = true;
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
    materiais_da_partida(MAT_MARFIM);
    for (int j = 0; j < 2; j++) {
        J[j].fichas = FICHAS_INI;
        colecao_inicial(&J[j]);
    }
    rodada = 0;
    pote = 0;
    memset(bis_extra, 0, sizeof bis_extra);
    memset(ad_moedas, 0, sizeof ad_moedas);
    memset(album, 0, sizeof album);
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
    materiais_da_partida(material_rapida());     // a partida comeca na loja
    vez = loja_primeiro;
    cursor = 0;
    n_voo = 0;
    fase = F_LOJA;
}

// O primeiro lance de um dado: um lado ao acaso (o Lastro, de 4 a 12).
static int primeiro_lance(int t, int (*r)(int))
{
    int x = r(TIPO[t].lados);
    if (TIPO[t].efeito == EF_LASTRO) x = 3 + r(9);    // 4 a 12, 1 em 9 cada
    return x;
}

// O que o dado de tipo t tira, com os poderes que mexem no sorteio, a partir
// do primeiro lance x. 'r' e quem sorteia: o da partida ou o da IA quando ela
// imagina jogadas. No Viciado, 'menor' recebe o lance descartado. Com a peca
// p (NULL: sem adesivos), o lado com Remendo ja tira 2 a mais aqui dentro: o
// 6 remendado do Explosivo e um 8 e explode; o Viciado compara os lances
// remendados. 'lado' recebe o lado que ficou para cima.
static int sorteia_valor_de(int t, int fixo, int x, const peca_t *p, int (*r)(int), int *menor, int *lado)
{
    int lados = TIPO[t].lados;
    *menor = 0;
    *lado = x;
    switch (TIPO[t].efeito) {
    case EF_VICIADO: {                       // dois lances; fica o maior
        int y = r(lados), vx = remenda(p, x), vy = remenda(p, y);
        if (vy > vx) { *lado = y; *menor = vx; return vy; }
        *menor = vy;
        return vx;
    }
    case EF_SORTE_REVES: {                   // o d12 da Sorte menos o d8 do Reves
        int b = r(SR_REVES);
        *menor = b;
        return remenda(p, x) - b;
    }
    case EF_EXPLODE: {                       // o 8 (ou mais, remendado) rola de novo e soma
        int n = remenda(p, x), g = 0;
        x = n;
        while (n >= lados && g++ < 3) { n = r(lados); x += n; }
        return x;
    }
    // Os de numero fixo nao rolam: o lado e o numero, e so o Remendo age.
    case EF_QUEBRADO: *lado = (int8_t)fixo; break;   // sem sorteio (a pedra de um Reves fica negativa)
    case EF_FANTASMA: *lado = fixo ? (int8_t)fixo : 1; break;
    case EF_PREVISIVEL: *lado = fixo + 1 < 12 ? fixo + 1 : 12; break;   // 1, 2, 3... ate 12
    case EF_VAZIO:    return ad_no_lado(p, x) == AD_REMENDO ? 2 : 0;   // o lado e o que rolou
    default: break;
    }
    return remenda(p, *lado);
}
static int sorteia_valor(int t, int fixo, int (*r)(int), int *menor)
{
    int lado;
    return sorteia_valor_de(t, fixo, primeiro_lance(t, r), NULL, r, menor, &lado);
}

// O lance de um dado da bolsa, com os adesivos dele (p; NULL: sem). O Chumbo
// puxa o primeiro lance para o lado dele metade das vezes (com mais de um,
// para um deles); caindo no Avesso, o dado vira do avesso; o Remendo soma 2
// ao lado que ficou para cima (em sorteia_valor_de, antes dos poderes). 'lado'
// recebe o lado de cima (0: fora dos lados do dado); 'antes', o lado do
// Avesso de onde ele virou (0: nao virou).
static int sorteia_peca(int t, int fixo, const peca_t *p, int (*r)(int), int *menor, int *lado, int *antes)
{
    int ef = TIPO[t].efeito;
    int x0 = primeiro_lance(t, r);
    *antes = 0;
    if (p && ef != EF_QUEBRADO && ef != EF_FANTASMA && ef != EF_PREVISIVEL) {
        int ch[AD_LADOS], n = 0;
        for (int l = ad_lado_min(t); l <= ad_lado_max(t); l++) if (ad_no_lado(p, l) == AD_CHUMBO) ch[n++] = l;
        // Um Chumbo: metade das vezes; dois (o maximo), cada um a metade.
        if (n == 1 && r(2) == 1) x0 = ch[0];
        else if (n > 1) x0 = ch[r(n) - 1];
        if (ad_no_lado(p, x0) == AD_AVESSO) { *antes = x0; x0 = ad_oposto(t, x0); }
    }
    int x = sorteia_valor_de(t, fixo, x0, p, r, menor, lado);
    if (*lado < ad_lado_min(t) || *lado > ad_lado_max(t)) *lado = 0;
    return x;
}

// O Presente desembrulha no lance: um dado raro ao acaso (no Desafiante, os
// seus so entre os que voce ja abriu), que sai dele e fica no lugar dele na
// bolsa - no Desafiante, pela run (a bolsa volta para a run como terminou a
// partida); nas outras, ate o fim da partida.
static int desembrulha(int j, int k, int (*r)(int))
{
    fila_t *fe = &F[j];
    if (j == 0 && embrulho >= 0 && fe->idx[k] == embrulho && embrulho < J[0].n_col) {   // o do Papai Noel
        peca_t *p = &J[0].col[embrulho];
        p->tipo = embrulho_tipo;
        fe->tipo[k] = embrulho_tipo;
        fe->fixo[k] = p->fixo;
        fe->presente[k] = 3;
        embrulho = -1;
        return fe->tipo[k];
    }
    int ok[D_N], n = 0;
    for (int t = 0; t < D_N; t++) {
        if (TIPO[t].raridade != RAR_RARO || sem_sorteio(t) || t == D_PRESENTE) continue;
        if (modo == M_DESAFIO ? (j == 0 && !dado_liberado(t)) : so_desafio(t)) continue;
        ok[n++] = t;
    }
    int t = n ? ok[r(n) - 1] : D_D6;
    fila_t *f = &F[j];
    f->tipo[k] = (uint8_t)t;
    f->fixo[k] = nova_peca(t).fixo;
    f->presente[k] = 1;
    int i = f->idx[k];
    if (i >= 0 && i < J[j].n_col && J[j].col[i].tipo == D_PRESENTE) {   // a copia do Coringa nao tem peca
        J[j].col[i].tipo = (uint8_t)t;          // fica: os adesivos seguem com ele
        J[j].col[i].fixo = f->fixo[k];
        f->presente[k] = 2;
    }
    return t;
}

// Um lance inteiro de uma peca, com os adesivos que agem no sorteio: x1 e o
// que ela tira primeiro (com o Avesso, antes1 e o lado de onde virou); com o
// Reflexivo, rola de novo e vale o segundo (x).
typedef struct { int x, menor, lado, x1, menor1, lado1, antes1; bool reflete; } lance_t;
static lance_t lanca_peca(int t, int fixo, const peca_t *pc, int (*r)(int))
{
    lance_t l;
    int antes;
    l.x = l.x1 = sorteia_peca(t, fixo, pc, r, &l.menor, &l.lado, &antes);
    l.menor1 = l.menor; l.lado1 = l.lado; l.antes1 = antes;
    l.reflete = ad_no_lado(pc, l.lado) == AD_REFLEXIVO;
    if (l.reflete) l.x = sorteia_peca(t, fixo, pc, r, &l.menor, &l.lado, &antes);
    return l;
}

static void lanca_proximo(void)
{
    fila_t *f = &F[vez];
    int k = f->lancados;
    int t = tipo_de(vez, k), lados = TIPO[t].lados;
    if (f->carta && k == 3) {                // a carta do Coringa: posta, sem rolar
        f->lancados = 4;
        f->est[3] = V_ESPERA;
        snprintf(veredito, sizeof veredito, "carta na manga: vira depois do seu turno");
        ver_tipo = 0;
        ver_comum = false;
        anulou_a = anulou_b = resistiu = -1;
        anulou_masc = 0;
        som_toca(SOM_BATE_DADO);
        t_fase = 0;
        fase = F_VEREDITO;
        return;
    }
    if (t == D_PRESENTE) {
        t = desembrulha(vez, k, rola);
        lados = TIPO[t].lados;
        som_toca(SOM_PRESENTE);
    }

    // Acumulador: entra com o bonus que guardou (ele cresce em passo.inc,
    // cada vez que pousa valendo).
    if ((t == D_ACUMULADOR || t == D_AUTOFAGIA || t == D_PREVISIVEL) && f->idx[k] >= 0 && f->idx[k] < J[vez].n_col)
        f->fixo[k] = J[vez].col[f->idx[k]].fixo;
    hora_comeca(dado_decisivo(vez, k));      // antes do sorteio: so olha as faces possiveis
    if (modo == M_DESAFIO) {                 // para o oponente saber quem passou quem
        desfecho_t d;
        calcula_desfecho(&d, false);
        placar_antes[0] = d.total[0]; placar_antes[1] = d.total[1];
    }
    // Os adesivos agem no lance (adesivos.inc): o Chumbo e o Avesso no
    // sorteio; o Reflexivo, caindo no lado dele, faz rolar de novo uma vez, e
    // vale o segundo lance.
    // Com o Post-it no lado de cima do dado de antes, o lance inteiro
    // acontece duas vezes e fica o maior: o outro cai ao lado, fantasma.
    const peca_t *pc = peca_fila(vez, k);
    lance_t l = lanca_peca(t, f->fixo[k], pc, rola);
    int postit = -1;                         // o lance que o Post-it descartou
    if (k > 0 && ad_fila(vez, k - 1) == AD_POSTIT) {
        lance_t l2 = lanca_peca(t, f->fixo[k], pc, rola);
        if (l2.x > l.x) { lance_t c = l; l = l2; l2 = c; }
        postit = l2.x;
        som_toca(som_adesivo(AD_POSTIT));
    }
    int x = l.x, menor = l.menor, lado = l.lado;
    int x1 = l.x1, lado1 = l.lado1, antes1 = l.antes1, menor1 = l.menor1;
    bool reflete = l.reflete;
    f->cru[k] = x;
    f->valor[k] = x;
    f->lado[k] = (uint8_t)lado;
    fatia(vez, k, rola);
    // O Previsivel sobe a cada lance, valendo ou nao (ate 12).
    if (t == D_PREVISIVEL && f->idx[k] >= 0 && f->idx[k] < J[vez].n_col && J[vez].col[f->idx[k]].fixo < 11)
        J[vez].col[f->idx[k]].fixo++;

    voo_t *v = &voo[0];
    memset(v, 0, sizeof *v);
    v->dono = (uint8_t)vez;
    v->slot = (uint8_t)k;
    v->tipo = (uint8_t)t;
    bool sr = TIPO[t].efeito == EF_SORTE_REVES;
    v->valor = sr ? x1 + menor1 : x1;        // no ar, o da Sorte
    v->lado = (uint8_t)lado1;
    if (antes1) {                            // pousa no Avesso e vira
        v->etapa = 1;
        v->valor_v = v->valor;
        v->lado_v = (uint8_t)lado1;
        v->valor = antes1;
        v->lado = (uint8_t)antes1;
    }
    if (reflete) {                           // e depois (ou so) o Reflexivo
        v->reflete = true;
        if (!v->etapa) v->etapa = 2;
        v->valor2 = sr ? x + menor : x;
        v->menor2 = menor;
        v->lado2 = (uint8_t)lado;
    }
    v->x = GFX_W / 2 + frnd(-110, 110);
    v->y = vez == baixo ? MESA_Y1 - R_DADO - 4 : MESA_Y0 + R_DADO + 4;
    v->vx = frnd(-300, 300);
    v->vy = (vez == baixo ? -1 : 1) * frnd(420, 600);
    v->spin = frnd(-14, 14);
    v->face = face_v(t, x1);
    if (hora) v->hesita = HESITA_T;
    n_voo = 1;
    f->descarte[k] = (uint8_t)menor;
    // O Viciado rola de verdade dois dados: o do menor valor e um fantasma
    // que some quando o maior vai para a fila. O Sorte/Reves tambem: o
    // fantasma e o d8 do Reves, que se junta ao da Sorte antes da fila.
    if (menor) {
        voo_t *w = &voo[1];
        *w = *v;
        w->fantasma = true;
        w->etapa = 0;
        w->valor = menor1;
        w->x = v->x + (v->x < GFX_W / 2 ? 60 : -60);
        w->vx = -v->vx * 0.8f + frnd(-60, 60);
        w->vy = v->vy * frnd(0.8f, 1.0f);
        w->spin = -v->spin;
        w->face = rola_v(TIPO[t].efeito == EF_SORTE_REVES ? SR_REVES : lados);
        n_voo = 2;
    }
    else if (postit >= 0) {                  // o lance do Post-it que ficou de fora
        voo_t *w = &voo[1];
        *w = *v;
        w->fantasma = true;
        w->etapa = 0;
        w->reflete = false;
        w->valor = postit;
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

// O adesivo que age no pouso, antes de o dado ir para a fila: o Avesso
// mostra o lado dele aceso, pula e vira; o Reflexivo mostra o dele e o dado
// sai rolando de novo. Devolve true enquanto isso dura.
#define AD_MOSTRA 0.32f                  // o adesivo aceso antes de agir
#define AD_VIRA   0.30f                  // o pulo do Avesso
static bool ad_etapa_passo(float dt)
{
    voo_t *v = &voo[0];
    if (!n_voo || !v->etapa) return false;
    float t0 = v->t_etapa;
    v->t_etapa += dt;
    if (t0 < 0.01f && v->t_etapa >= 0.01f) som_toca(som_adesivo(v->etapa == 1 ? AD_AVESSO : AD_REFLEXIVO));
    if (v->etapa == 1) {
        float q = (v->t_etapa - AD_MOSTRA) / AD_VIRA;
        if (q >= 0.5f && !v->virou) {            // no alto do pulo, o outro lado
            v->virou = true;
            v->valor = v->face = v->valor_v;
            v->lado = v->lado_v;
        }
        if (q < 1) return true;
        som_toca(TIPO[v->tipo].lados == 2 ? SOM_MOEDA_POUSA : SOM_POUSA);
        v->pouso = 0.18f;
        v->t_etapa = 0;
        v->etapa = v->reflete ? 2 : 0;
        return v->etapa != 0;
    }
    if (v->t_etapa < AD_MOSTRA) return true;
    // O Reflexivo: o dado (e o fantasma do Viciado ou do Reves) sai rolando de
    // novo; o lance que o Post-it descartou fica onde parou.
    for (int i = 0; i < n_voo; i++) {
        voo_t *w = &voo[i];
        if (w->fantasma && !v->menor2) continue;
        w->parado = false;
        w->pouso = 0;
        w->t_face = 0;
        w->vx = frnd(-260, 260);
        w->vy = (w->y > CENTRO ? -1 : 1) * frnd(240, 360);
        w->spin = frnd(-12, 12);
        w->hesita = hora ? HESITA_T : 0;
        w->chumbou = false;
    }
    v->valor = v->valor2;
    v->lado = v->lado2;
    if (n_voo > 1 && v->menor2) voo[1].valor = v->menor2;
    v->etapa = 0;
    v->reflete = false;
    t_fase = 0;
    if (TIPO[v->tipo].lados == 2) som_rufo_moeda(); else som_rufo(true);
    return true;
}

// O Sorte/Reves para no feltro e as duas metades se juntam num dado so, com o
// valor final, antes de ir para a fila: o tempo da juncao.
#define SR_JUNTA 0.35f
static float junta_t(void) { return n_voo > 1 && voo[0].tipo == D_SORTE_REVES ? SR_JUNTA : 0; }

static void comeca_segundo(void)
{
    vez = outro(primeiro);
    cursor = 0;
    t_fase = 0;
    fase = F_ORDEM;
}

// O que os adesivos deixam depois da rodada: o Fermento cresce (no dado, a
// run inteira), a Raspadinha sai do dado, o lacre do Selo se rompe ou se
// fecha (o que este lance tirou vale para o proximo), o Bis guarda os dados
// a mais da proxima compra e o Dourado as moedas (a run as recebe no fim da
// partida).
static void adesivos_ficam(int j, const desfecho_t *d)
{
    jogador_t *p = &J[j];
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        int i = F[j].idx[k];
        if (i < 0 || i >= p->n_col) continue;
        if ((d->fermento[j] >> k & 1) && p->col[i].fermento < FERMENTO_MAX) p->col[i].fermento++;
        if (d->raspa[j] >> k & 1) ad_cola(&p->col[i], F[j].lado[k], AD_NADA);
        if (d->selo_rompe[j] >> k & 1) p->col[i].selo = 0;
        if (d->selo_lacra[j] >> k & 1) p->col[i].selo = 1;
        if (d->velcro[j] >> k & 1) p->col[i].velcro = 1;
        if (d->figurinha[j] >> k & 1) ad_poe_no_album(&p->col[i], F[j].lado[k]);   // no album ate o fim da partida
        // O Fragil anulado quebra: vira um Quebrado com o numero que tirou.
        if ((d->fragil[j] >> k & 1) && F[j].tipo[k] != D_QUEBRADO && F[j].tipo[k] != D_FANTASMA) {
            int v = F[j].valor[k] > 99 ? 99 : F[j].valor[k] < -99 ? -99 : F[j].valor[k] ? F[j].valor[k] : 1;
            p->col[i] = (peca_t){ .tipo = D_QUEBRADO, .fixo = (uint8_t)(int8_t)v };
        }
    }
    bis_extra[j] += d->bis[j];
    ad_moedas[j] += d->moedas[j];
    album[j] = d->album[j];
}

// O que os dados guardam da rodada, na peca da bolsa: a Furia, o que o lance
// mexeu no bonus (valendo ou nao); a Onda valendo, o que tirou - e com 9 vira
// Tsunami, de vez; os Tres Patinhos valendo, mais uma vez.
static void contadores_ficam(int j, const desfecho_t *d)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        int i = F[j].idx[k];
        if (i < 0 || i >= J[j].n_col || J[j].col[i].tipo != F[j].tipo[k]) continue;
        peca_t *p = &J[j].col[i];
        bool vale = d->est[j][k] == V_VALIDO;
        if (p->tipo == D_FURIA) p->fixo = (uint8_t)(int8_t)furia_depois(F[j].fixo[k], F[j].valor[k]);
        else if (p->tipo == D_ONDA && vale) {
            if (d->onda[j] >> k & 1) { p->tipo = D_TSUNAMI; p->fixo = 0; }
            else p->fixo = (uint8_t)onda_depois(F[j].fixo[k], F[j].valor[k]);
        } else if (p->tipo == D_PATINHOS && vale && F[j].fixo[k] < 255) p->fixo = (uint8_t)(F[j].fixo[k] + 1);
        else if (p->tipo == D_PISCA) p->fixo = (uint8_t)(F[j].fixo[k] ^ 1);          // lancado: alterna
        else if (p->tipo == D_MEIA && vale) p->fixo = 0;                            // pagou: esvazia
        else if (p->tipo == D_MEIA && d->est[j][k] == V_ANULADO) p->fixo = (uint8_t)meia_depois(F[j].fixo[k]);
    }
}

// A Autofagia comeu: o dado sai da bolsa ate o fim da partida e ela guarda +1.
static void autofagia_come(int j, const desfecho_t *d)
{
    if (!d->come[j] || !d->come_de[j]) return;
    int b = F[j].idx[d->come[j] - 1], a = F[j].idx[d->come_de[j] - 1];
    if (a < 0 || a >= J[j].n_col || b < 0 || b >= J[j].n_col) return;
    J[j].onde[b] = COMIDO;
    if (J[j].col[a].fixo < AUTO_MAX) J[j].col[a].fixo++;
}

// O Garimpeiro valeu: os outros dados de j que terminaram valendo voltam para
// a bolsa agora, em vez de ficar fora ate ela esvaziar. Ele mesmo, os que
// cairam e os que ficaram na mao seguem o caminho de sempre.
static void garimpa(int j, const desfecho_t *d)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        if (!(d->garimpo[j] >> k & 1)) continue;
        int c = F[j].idx[k];
        if (c >= 0 && c < J[j].n_col && J[j].onde[c] == NA_MAO) J[j].onde[c] = NA_BOLSA;
    }
}

// A Medusa valeu: cada dado lancado por j (menos os roubados e os que ja
// saem da bolsa) vira um Quebrado com o numero que tirou.
static void medusa_petrifica(int j, const desfecho_t *d)
{
    if (!d->medusa[j]) return;
    fila_t *f = &F[j];
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (d->est[j][k] == V_ROUBADO || f->tipo[k] == D_QUEBRADO || f->tipo[k] == D_FANTASMA || vis_queima[j][k]) continue;
        if (f->tipo[k] == D_VIDRO && f->cru[k] == 1) continue;              // estilhacado: ja sai
        if (f->idx[k] < 0 || f->idx[k] >= J[j].n_col) continue;
        int v = f->valor[k] > 99 ? 99 : f->valor[k] < -99 ? -99 : f->valor[k] ? f->valor[k] : 1;
        J[j].col[f->idx[k]] = (peca_t){ .tipo = D_QUEBRADO, .fixo = (uint8_t)(int8_t)v };    // o Reves negativo vira pedra negativa
    }
}

// O Coringa: o 4o dado dele e a copia do seu dado que tirou o valor mais
// alto no seu turno (empate: o ultimo deles). Se voce ja jogou, ele sabe
// qual e. Se ele abre a rodada, o 4o e uma carta: posta na mesa, fica em
// suspensao ("?") - nao cai nem e atacada - e so depois do seu turno vira a
// copia e e lancada, com a regra da sequencia valendo para ela.
static bool coringa_joga(int j) { return j == 1 && tem_reg(REG_CORINGA); }
static bool carta_posta(int j) { return F[j].carta && F[j].lancados > 3; }
// A fila do Coringa ficou pronta com tres dados: o quarto e a copia, ou a
// carta (fora da bolsa dele: idx -1).
static void coringa_completa(int j)
{
    fila_t *f = &F[j];
    if (!coringa_joga(j) || f->n != 3) return;
    peca_t c;
    f->carta = !melhor_lance(outro(j), &c);
    if (f->carta) c = (peca_t){ .tipo = D_D6, .fixo = 0 };       // so o lugar: o tipo vem depois
    f->tipo[3] = c.tipo; f->fixo[3] = c.fixo; f->idx[3] = -1;
    f->n = 4;
}
// A carta virou: passa a ser a copia, pronta para ser lancada (lancados 3).
// Sem nenhum dado seu lancado, nao tem o que copiar e sai da fila. Devolve
// se ha o que lancar.
static bool vira_carta(int j)
{
    fila_t *f = &F[j];
    peca_t c;
    f->carta = false;
    f->lancados = 3;
    f->est[3] = V_ESPERA; f->tag[3][0] = 0;
    if (!melhor_lance(outro(j), &c)) { f->n = 3; return false; }
    f->tipo[3] = c.tipo; f->fixo[3] = c.fixo;
    return true;
}

// Mau-Olhado (a Bruxaria da Bruxa): quem o tem venceu a rodada, e um
// Fantasma (vale 1) entra na bolsa do outro, j, ate o fim da partida. O
// aviso da rodada ganha a linha dele.
static bool poe_fantasma(int j, bool avisa)
{
    jogador_t *p = &J[j];
    if (p->n_col >= lim_col()) return false;     // so se a estrutura estiver cheia (128)
    p->col[p->n_col] = (peca_t){ .tipo = D_FANTASMA, .fixo = 1 };
    p->onde[p->n_col++] = NA_BOLSA;
    t_fantasma = 1.6f;
    fantasma_para = (int8_t)j;
    if (j == 1) am_acende(A_MAU_OLHADO);
    if (avisa) {
        size_t n = strlen(aviso);
        snprintf(aviso + n, sizeof aviso - n, j == 0 ? "  ·  um Fantasma entra na sua bolsa" : "  ·  um Fantasma entra na bolsa dele");
    }
    return true;
}

// A copia de um Jingle Bell: a mesma peca, adesivos e tudo, marcada (fixo 1)
// para sumir no fim da partida.
static bool copia_jingle(const peca_t *pc) { return pc->tipo == D_JINGLE && pc->fixo; }
static bool poe_copia_jingle(int j, const peca_t *orig)
{
    jogador_t *p = &J[j];
    if (p->n_col >= lim_col()) return false;
    peca_t c = orig && orig->tipo == D_JINGLE ? *orig : nova_peca(D_JINGLE);
    c.fixo = 1;
    p->col[p->n_col] = c;
    p->onde[p->n_col++] = NA_BOLSA;
    return true;
}

// Fim da partida: os Fantasmas e as copias dos Jingle Bells somem das bolsas.
static void some_fantasmas(void)
{
    for (int j = 0; j < 2; j++) {
        jogador_t *p = &J[j];
        int n = 0;
        for (int i = 0; i < p->n_col; i++)
            if (p->col[i].tipo != D_FANTASMA && !copia_jingle(&p->col[i])) { p->col[n] = p->col[i]; p->onde[n++] = p->onde[i]; }
        p->n_col = n;
        p->n_mao = 0;
    }
}

// A pedra negativa (um Reves que a Medusa petrificou), jogada por voce no
// Desafiante: ao se partir, tem um dado dentro.
static bool pedra_misteriosa(int j, int k)
{
    return modo == M_DESAFIO && j == 0 && F[j].tipo[k] == D_QUEBRADO && (int8_t)F[j].fixo[k] < 0
        && F[j].idx[k] >= 0 && F[j].idx[k] < J[j].n_col;
}

// Dados que saem do jogo ao fim da rodada: o Quebrado que foi jogado, o Vidro
// estilhacado e o que a Ficha de Fogo queimou. A bolsa nunca cai abaixo da
// mao: se cairia, o dado perdido vira um d4 comum. A pedra negativa do
// Desafiante se parte e deixa o Dado Misterioso no lugar.
// O Coringa, da 5a rodada em diante (do Nivel 7): o dado seu que termina
// anulado e dele.
static bool coringa_destroi(void) { return tem_reg(REG_CORINGA) && rodada >= VIRADA_RODADA && des_dificuldade() >= CORINGA_SOME; }

static void recolhe_quebrados(void)
{
    int n_quebra = 0, n_cinza = 0, n_vidro = 0, n_mist = 0, n_coringa = 0;
    // Os Jingle Bells lancados deixam uma copia na bolsa: tiradas antes que
    // os dados que saem mudem as posicoes da bolsa.
    peca_t sino[2][N_FILA];
    for (int j = 0; j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            const peca_t *pc = peca_fila(j, k);
            if (F[j].tipo[k] == D_JINGLE) sino[j][k] = pc && pc->tipo == D_JINGLE ? *pc : nova_peca(D_JINGLE);
        }
    for (int j = 0; j < 2; j++) {
        int saem[N_FILA], n = 0;
        for (int k = 0; k < F[j].lancados; k++) {
            bool some = false;
            if (pedra_misteriosa(j, k)) {
                F[j].quebra[k] = true;
                J[j].col[F[j].idx[k]] = (peca_t){ .tipo = D_MISTERIOSO, .fixo = 0 };
                n_mist++;
                continue;
            }
            if (F[j].tipo[k] == D_QUEBRADO) { F[j].quebra[k] = true; some = true; n_quebra++; }
            if (F[j].tipo[k] == D_VIDRO && F[j].cru[k] == 1) { F[j].quebra[k] = true; some = true; n_vidro++; }
            if (vis_queima[j][k] && !some)  { F[j].cinza[k] = true; some = true; n_cinza++; }
            if (j == 0 && !some && coringa_destroi() && F[j].tipo[k] != D_FANTASMA
                && F[0].est[k] == V_ANULADO) {         // o estado final (no desfecho, ja com os ataques)
                F[j].quebra[k] = true; some = true; n_coringa++;
            }
            if (some && F[j].idx[k] >= 0) saem[n++] = F[j].idx[k];   // a copia do Coringa nao e da bolsa
        }
        for (int a = 1; a < n; a++)
            for (int b = a; b > 0 && saem[b] > saem[b - 1]; b--) {
                int t = saem[b]; saem[b] = saem[b - 1]; saem[b - 1] = t;
            }
        jogador_t *p = &J[j];
        for (int i = 0; i < n; i++) {
            if (em_jogo(p) <= N_FILA) { p->col[saem[i]] = nova_peca(D_D4); continue; }
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
    int n_sino = 0;
    for (int j = 0; j < 2; j++) {
        jingle_copias[j] = 0;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
            if (F[j].tipo[k] == D_JINGLE && poe_copia_jingle(j, &sino[j][k])) {
                jingle_copias[j] |= (uint8_t)(1 << k);
                n_sino++;
            }
    }
    if (n_sino)
        snprintf(nota, sizeof nota, n_sino > 1 ? "cópias dos Jingle Bells entram na bolsa" : "uma cópia do Jingle Bell entra na bolsa");
    // Pacto (cada lance) e Assombracao (valendo): Fantasmas nas bolsas.
    for (int j = 0; j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            int q = F[j].tipo[k] == D_PACTO ? j
                  : F[j].tipo[k] == D_ASSOMBRACAO && F[j].est[k] == V_VALIDO ? outro(j) : -1;
            if (q >= 0 && poe_fantasma(q, false))
                snprintf(nota, sizeof nota, "%s ganha um Fantasma (%s)", J[q].nome, TIPO[F[j].tipo[k]].nome);
        }
    if (n_coringa)
        snprintf(nota, sizeof nota, n_coringa > 1 ? "o Coringa destrói %d dados seus anulados" : "o Coringa destrói o seu dado anulado", n_coringa);
    if (n_mist) {
        snprintf(nota, sizeof nota, "a pedra se parte... havia um dado dentro");
        if (!(cpu & 1)) t_misterio = 0.001f;    // a vinheta, para quem joga
    }
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
    int seu_pote = 0;                            // o pote que voce levou (historico)
    for (int j = 0; j < 2; j++) {
        memcpy(F[j].est, resolvido.est[j], N_FILA);
        memcpy(vis_est[j], resolvido.est[j], N_FILA);
        memcpy(vis_queima[j], resolvido.queima[j], N_FILA);
        F[j].total = resolvido.total[j];
    }
    int w = zerou >= 0 ? outro(zerou) : -1;      // quem abriu fez 0: o outro leva, sem jogar
    bool casa = w < 0 && F[0].total == F[1].total && tem_reg(REG_CASA);   // o Crupie leva o empate
    if (w < 0 && F[0].total == F[1].total && !casa) {
        venc_mao = 2;
        J[0].perdeu_ultima = J[1].perdeu_ultima = false;
        som_toca(SOM_EMPATE);
        snprintf(aviso, sizeof aviso, "empate em %d: o pote fica na mesa", F[0].total);
    } else {
        if (w < 0) w = casa ? 1 : (F[0].total > F[1].total ? 0 : 1);
        venc_mao = w;
        res_quem = w;
        J[outro(w)].perdidas++;                  // o Rancor conta
        J[outro(w)].perdeu_ultima = true;        // e a Revanche do Rival
        J[w].perdeu_ultima = false;
        J[w].fichas += pote;
        if (casa) snprintf(aviso, sizeof aviso, "empate: a casa leva %d fichas", pote);
        else if (zerou >= 0) snprintf(aviso, sizeof aviso, "%s fez 0: %s leva %d fichas", J[zerou].nome, J[w].nome, pote);
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
        if (w == 0) seu_pote = pote;
        pote = 0;
        if (tem_am(w, A_MAU_OLHADO)) poe_fantasma(outro(w), true);   // Mau-Olhado, a Bruxaria
        // Rodada grande: a vitoria vem mais cheia, e a mesa treme.
        if (F[w].total >= RODADA_GRANDE) { som_toca(SOM_FORTE); golpe(w, 1u << N_FILA); crescendo(w, 3); }
        else som_toca(SOM_VITORIA);
    }
    for (int j = 0; j < 2; j++) {                 // a Gorjeta paga, ganhe quem ganhar
        int r = outro(j), g = resolvido.gorjeta[j] < J[r].fichas ? resolvido.gorjeta[j] : J[r].fichas;
        if (g <= 0) continue;
        J[r].fichas -= g;
        J[j].fichas += g;
        size_t n = strlen(aviso);
        snprintf(aviso + n, sizeof aviso - n, "  ·  Gorjeta: %d de %s", g, J[r].nome);
    }
    for (int j = 0; j < 2; j++) {                 // o Agiota cobra, ganhe quem ganhar
        int r = outro(j), a = resolvido.agiota[j] < J[r].fichas ? resolvido.agiota[j] : J[r].fichas;
        if (a <= 0) continue;
        J[r].fichas -= a;
        J[j].fichas += a;
        size_t n = strlen(aviso);
        snprintf(aviso + n, sizeof aviso - n, "  ·  Agiota: %d de %s", a, J[r].nome);
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
    if (venc_mao == 0) rival_reage(F[0].total >= RODADA_GRANDE ? RM_COMBO : RM_PERDE_R);
    else if (venc_mao == 1) rival_reage(RM_GANHA_R);
    hist_rodada(seu_pote);
    desfaz_embrulho();                       // o embrulho do Papai Noel nao lancado volta a ser o dado
    for (int j = 0; j < 2; j++) {
        adesivos_ficam(j, &resolvido);
        contadores_ficam(j, &resolvido);
        autofagia_come(j, &resolvido);
        medusa_petrifica(j, &resolvido);
        garimpa(j, &resolvido);
    }
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
    case EV_ANULA:
        switch (F[e->de_j].tipo[e->de_k]) {
        case D_LASER:   return SOM_LASER;
        case D_BONECO:  return SOM_GELO;
        case D_TSUNAMI: return SOM_ONDA;
        default:        return SOM_ANULA;
        }
    case EV_ONDA:    return SOM_ONDA;
    case EV_CAVALO:  return SOM_ANULA;
    case EV_TIRA: case EV_PIRATA: case EV_CORTA: return SOM_TIRA;
    case EV_ROUBA:   return SOM_ROUBA;
    case EV_AGIOTA:  return SOM_FICHA;
    case EV_MEDUSA:  return SOM_BIGORNA;
    case EV_ADESIVO:                     // a copia brilha com o som do Holografico; o Fragil quebrando, vidro
        if (e->ad == AD_FRAGIL && !e->valor) return SOM_VIDRO;
        return som_adesivo(e->copia ? AD_HOLOGRAFICO : e->ad);
    case EV_GARIMPO: return SOM_BOLSA;
    case EV_FOGO: case EV_RENASCE: return SOM_FOGO;
    case EV_EXTRA:
        if (e->de_k >= N_FILA) return SOM_EFEITO;
        switch (F[e->de_j].tipo[e->de_k]) {
        case D_JINGLE:  return SOM_SINO;
        case D_ESTRELA: return SOM_ESTRELA;
        case D_MEIA:    return e->valor ? SOM_AD_DOURADO : SOM_MOEDA_POUSA;   // paga; ou guarda mais uma
        default:        return SOM_EFEITO;
        }
    case EV_BONUS:
        if (e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_FERREIRO) return SOM_BIGORNA;
        if (e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_RENA) return SOM_SINO;  // os guizos do treno
        for (int k = 0; k < N_FILA; k++) if (e->por_dado[k] < 0) return SOM_TIRA;   // Tudo ou Nada com 1
        return SOM_EFEITO;
    default:         return SOM_EFEITO;
    }
}

// A cadeia do fim da rodada sobe de tom: os sons de ficha e de poder comum
// tocam um semitom acima a cada efeito seguido, ate +7. Os que tem voz
// propria (Laser, Jackpot, Fogo, roubo...) ficam no tom deles, mas contam
// na escada.
#define ESCADA_MAX 7
static void toca_evento(int i)
{
    int s = som_do_evento(i);
    if (s == SOM_EFEITO || s == SOM_FICHA || s == SOM_TIRA) som_toca_tom(s, i < ESCADA_MAX ? i : ESCADA_MAX);
    else som_toca(s);
    for (int a = 0; a < A_N; a++) if (ev[i].am >> a & 1) am_acende(a);   // o amuleto que agiu acende
}

// E acelera: cada efeito corre 12% mais depressa que o anterior (o tempo do
// jogo anda mais rapido, a animacao inteira vai junto), ate 2,2 vezes; somado
// a velocidade da mesa (vel), no maximo 3 vezes o normal.
#define RITMO_PASSO 0.88f
#define RITMO_MAX   2.2f
#define RITMO_TETO  3.0f
static float ritmo_efeitos(float vel)
{
    float r = powf(1 / RITMO_PASSO, (float)ev_i), teto = RITMO_TETO / vel;
    if (r > RITMO_MAX) r = RITMO_MAX;
    if (teto < 1) teto = 1;
    return r < teto ? r : teto;
}

static void desfecho(void)
{
    calcula_desfecho(&resolvido, true);
    for (int j = 0; j < 2; j++) {
        memcpy(vis_est[j], F[j].est, N_FILA);
        vis_nroubo[j] = 0;
        vis_delta[j] = 0;
        vis_semente[j] = 0;
        memset(vis_bonus[j], 0, N_FILA);
        memset(vis_mult[j], 0, N_FILA);
        memset(vis_queima[j], 0, N_FILA);
        memset(vis_corte[j], 0, N_FILA);
        memset(vis_pedra[j], 0, N_FILA);
        memset(vis_gelo[j], 0, N_FILA);
        vis_selo[j][0] = vis_selo[j][1] = vis_fragil[j] = 0;
        memset(vis_tsunami[j], 0, N_FILA);
        memset(vis_espelho[j], 0, N_FILA);
        memset(vis_oculto[j], 0, N_FILA);
    }
    for (int i = 0; i < n_ev; i++)
        if (ev[i].tipo == EV_ESPELHO || ev[i].tipo == EV_REVELA) {
            if (ev[i].tipo == EV_ESPELHO) vis_espelho[ev[i].de_j][ev[i].de_k] = 1;
            else vis_oculto[ev[i].de_j][ev[i].de_k] = 1;
            memcpy(vis_est[ev[i].de_j], est_pre_espelho[ev[i].de_j], N_FILA);
        }
    n_voo = 0;
    ev_i = 0;
    t_fase = 0;
    for (int j = 0; j < 2; j++) log_base[j] = total_visivel(j);
    if (n_ev) { fase = F_EFEITOS; toca_evento(0); reage_evento(&ev[0]); }
    else conclui_rodada();
}

// O evento da vez acabou e ja se aplicou: entra o seguinte (ou a rodada
// fecha). Depois de um golpe, so quando a mesa solta (passo.inc).
static void proximo_evento(void)
{
    ev_i++;
    t_fase = 0;
    if (ev_i >= n_ev) conclui_rodada();
    else { toca_evento(ev_i); reage_evento(&ev[ev_i]); }
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
    case EV_CORTA:   return 1.5f;
    case EV_AGIOTA:  return 1.6f;
    case EV_MEDUSA:  return 1.9f;
    case EV_ONDA:    return 2.1f;
    case EV_GARIMPO: return 1.7f;
    case EV_REVELA:  return 1.4f;
    case EV_VAMPIRO: return 1.6f;
    case EV_RENASCE: return 1.5f;
    case EV_AVISO:   return 1.5f;
    case EV_JACKPOT: return 2.4f;
    case EV_BONUS: return 1.4f;
    case EV_ADESIVO: return 1.5f;
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
        if (v > 0) flutua_em(j, k, C_OURO, 0, "+%d", v);
        else if (v < 0) flutua_em(j, k, C_VINHO_CLR, 0, "%d", v);   // o Sorte/Reves negativo
    }
    uint8_t caiu = anulou_masc;
    if (anulou_a >= 0) caiu |= (uint8_t)(1 << anulou_a);
    if (anulou_b >= 0) caiu |= (uint8_t)(1 << anulou_b);
    for (int i = 0; i < k; i++)
        if ((caiu >> i & 1) && f->est[i] != V_VALIDO) {
            int v = valor_pontos(j, i) + f->bonus[i];
            if (v > 0) flutua_em(j, i, C_VINHO_CLR, 0.08f, "-%d", v);
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
            else if (e->tipo == EV_BONUS && e->por_dado[k] < 0)             // Tudo ou Nada com 1
                flutua_em(dj, k, C_VINHO_CLR, k * 0.06f, "%d", e->por_dado[k]);
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
                if ((e->mascara >> k & 1) && e->por_dado[k] > 0) flutua_em(dj, k, C_OURO, k * 0.06f, "+%d", e->por_dado[k]);
            break;
        }
        if (e->valor) flutua_em(dj, dk, C_OURO, 0, "+%d", e->valor);
        else if (dk < N_FILA && F[dj].tipo[dk] == D_MEIA) flutua_em(dj, dk, C_LATAO, 0.3f, "guarda +%d", MEIA_GUARDA);
        break;
    case EV_FOGO: case EV_SEMENTE: case EV_JACKPOT: case EV_RENASCE:
        if (e->valor) flutua_em(dj, dk, C_OURO, 0, "+%d", e->valor);
        break;
    case EV_CAVALO:
        for (int k = 0; k < N_FILA; k++)
            if (e->mascara >> k & 1) flutua_em(aj, k, C_VINHO_CLR, k * 0.06f, "-%d", e->por_dado[k]);
        if (e->valor) flutua_em(aj, N_FILA, C_VINHO_CLR, 0.1f, "-%d", e->valor);   // o que tinha roubado
        break;
    case EV_CHEFE:   flutua_em(dj, N_FILA, dj == 0 ? C_OURO : C_VINHO_CLR, 0, "+%d", e->valor); break;
    case EV_CORTA:   flutua_em(aj, ak, C_VINHO_CLR, 0, "-%d", e->valor); break;
    case EV_VAMPIRO:
        for (int k = 0; k < N_FILA; k++)
            if (e->mascara >> k & 1) flutua_em(aj, k, C_VINHO_CLR, 0.25f + k * 0.08f, "-%d", e->valor);
        break;
    case EV_ADESIVO:
        switch (e->ad) {
        case AD_ELETRONICO: flutua_em(dj, dk, C_OURO, 0, "x%d", e->por_dado[dk < N_FILA ? dk : 0]); break;
        case AD_LOBO:       flutua_em(aj, N_FILA, C_VINHO_CLR, 0, "-%d", e->valor); break;
        case AD_MIRA:       flutua_em(aj, ak, C_VINHO_CLR, 0, "-%d", e->valor); break;
        case AD_GORJETA:    flutua_em(dj, dk, C_OURO, 0, "+%d fichas", e->valor); break;
        case AD_DOURADO:    flutua_em(dj, dk, C_OURO, 0, "+%d moeda", e->valor); break;
        case AD_BIS:        flutua_em(dj, dk, C_OURO, 0, "+%d dados", e->valor); break;
        case AD_SELO:       flutua_em(dj, dk, e->valor ? C_OURO : C_VINHO_CLR, 0, e->valor ? "+%d" : "lacrado", e->valor); break;
        case AD_RATOEIRA:   flutua_em(aj, ak, C_VINHO_CLR, 0, "-%d", e->valor); break;
        case AD_VELCRO:     flutua_em(dj, dk, C_OURO, 0, "gruda", 0); break;
        case AD_FIGURINHA:  flutua_em(dj, dk, C_OURO, 0, e->valor ? "+%d" : "no álbum", e->valor); break;
        case AD_FRAGIL:     flutua_em(dj, dk, e->valor ? C_OURO : C_VINHO_CLR, 0, e->valor ? "+%d" : "quebra", e->valor); break;
        case AD_IMANTADO:
            for (int k = 0; k < N_FILA; k++)
                if (e->mascara >> k & 1) flutua_em(dj, k, C_OURO, k * 0.08f, "x%d", e->por_dado[k]);
            break;
        default:            flutua_em(dj, dk, C_OURO, 0, "+%d", e->valor); break;
        }
        break;
    default: break;
    }
}

// O evento i terminou: a mesa passa a mostrar o efeito dele.
static void aplica_evento(int i)
{
    evento_t *e = &ev[i];
    flutua_evento(e);
    switch (e->tipo) {
    case EV_ANULA:
        vis_est[e->alvo_j][e->alvo_k] = V_ANULADO;
        if (F[e->de_j].tipo[e->de_k] == D_BONECO) vis_gelo[e->alvo_j][e->alvo_k] = 1;   // fica congelado
        break;
    case EV_ONDA:   vis_tsunami[e->de_j][e->de_k] = 1; break;
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
    case EV_REVELA:                     // o Oculto mostrou: idem
        vis_espelho[e->de_j][e->de_k] = 0;
        vis_oculto[e->de_j][e->de_k] = 0;
        memcpy(vis_est[e->de_j], F[e->de_j].est, N_FILA);
        break;
    case EV_VAMPIRO:
        for (int k = 0; k < N_FILA; k++) if (e->mascara >> k & 1) vis_bonus[e->alvo_j][k] -= (int8_t)e->valor;
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
    case EV_SEMENTE: case EV_JACKPOT: case EV_EXTRA: case EV_CHEFE:
        vis_semente[e->de_j] += e->valor;
        break;
    case EV_RENASCE: vis_est[e->de_j][e->de_k] = V_VALIDO; break;
    case EV_CORTA:   vis_bonus[e->alvo_j][e->alvo_k] -= (int8_t)e->valor; vis_corte[e->alvo_j][e->alvo_k] = 1; break;
    case EV_MEDUSA:
        for (int k = 0; k < N_FILA; k++) if (e->mascara >> k & 1) vis_pedra[e->de_j][k] = 1;
        break;
    case EV_CAVALO:                     // tudo o que era de quem atingiu o Cavalo cai
        for (int k = 0; k < N_FILA; k++)
            if (e->mascara >> k & 1) vis_est[e->alvo_j][k] = V_ANULADO;
        for (int r = 0; r < vis_nroubo[e->alvo_j]; r++) vis_roubo[e->alvo_j][r].anulado = 1;
        break;
    case EV_ADESIVO:                    // o adesivo agiu: pontos, x2, o ataque ao rival
        switch (e->ad) {
        case AD_CONFETE: case AD_RASPADINHA: case AD_FERMENTO: case AD_ESTRELINHA: case AD_PURPURINA: case AD_FIGURINHA:
            vis_semente[e->de_j] += e->valor;
            break;
        case AD_FRAGIL:                 // valendo paga; anulado, racha (e quebra no fim)
            vis_semente[e->de_j] += e->valor;
            if (!e->valor && e->de_k < N_FILA) vis_fragil[e->de_j] |= (uint8_t)(1 << e->de_k);
            break;
        case AD_SELO:                   // o lacre rompeu (pagando) ou fechou
            vis_semente[e->de_j] += e->valor;
            if (e->de_k < N_FILA) vis_selo[e->de_j][e->valor ? 0 : 1] |= (uint8_t)(1 << e->de_k);
            break;
        case AD_ELETRONICO: case AD_IMANTADO:
            for (int k = 0; k < N_FILA; k++) if (e->mascara >> k & 1) vis_mult[e->de_j][k] = e->por_dado[k];
            break;
        case AD_LOBO: vis_delta[e->alvo_j] -= e->valor; break;
        case AD_MIRA: case AD_RATOEIRA: vis_est[e->alvo_j][e->alvo_k] = V_ANULADO; break;
        default: break;
        }
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

// Fim do turno de j: os dados da sequencia que ele nao lancou saem da mesa e
// ficam fora da bolsa, como os lancados e os que ficaram na mao: so voltam
// quando a bolsa esvaziar.
static void descarta_sobras(int j)
{
    fila_t *f = &F[j];
    jogador_t *p = &J[j];
    n_devolvidos = 0;
    for (int k = f->lancados; k < f->n && k < N_FILA; k++) {
        int c = f->idx[k];
        if (c < 0 || c >= p->n_col) continue;
        p->onde[c] = FORA;
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
    if (f->lancados <= 3) f->carta = false;      // parou antes da carta do Coringa
    t_devolve = n_devolvidos ? 0 : -1;
}

// Quem abriu a rodada terminou com 0, e nada mais muda isso: sem Espelho
// esperando o outro, sem Oculto escondido, sem a carta do Coringa, e 0 tambem
// depois dos efeitos do fim. Qualquer dado do outro ja venceria: a rodada
// acaba aqui.
static bool zerou_de_vez(int j)
{
    if (carta_posta(j)) return false;
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        if (F[j].espera[k] || F[j].escondido[k]) return false;
    desfecho_t d;
    calcula_desfecho(&d, false);
    return d.total[j] == 0;
}

// Quem joga em segundo ja passou do alvo: com o que esta na mesa, parar agora
// vence (a previa do fim da rodada, a mesma do botao "parar"). So quando
// nada ainda vai mudar sozinho: sem Oculto escondido e sem a carta do Coringa
// por virar. Jogar mais so poderia perder: a vez acaba ali.
static bool ja_vence(int j)
{
    if (j == primeiro || !F[j].lancados || carta_posta(primeiro) || tem_escondido(0) || tem_escondido(1)) return false;
    desfecho_t d;
    calcula_desfecho(&d, false);
    return d.total[j] > d.total[outro(j)];
}

// Quem nao chegou a jogar: a mao inteira volta para a bolsa.
static void devolve_mao(int j)
{
    jogador_t *p = &J[j];
    for (int i = 0; i < p->n_mao; i++) p->onde[p->mao[i]] = NA_BOLSA;
    p->n_mao = 0;
}

static void termina_turno(void)
{
    som_toca(SOM_TURNO);
    if (carta_virando) {                     // a carta do Coringa ja foi lancada: o fim
        carta_virando = false;
        desfecho();
        return;
    }
    descarta_sobras(vez);
    // Voce terminou e a carta do Coringa esperava: ela vira a copia do seu
    // dado que mais tirou e e lancada agora.
    if (vez != primeiro && carta_posta(primeiro)) {
        int c = primeiro;
        if (vira_carta(c)) {
            vez = c;
            carta_virando = true;
            cursor = 0;
            peca_t copia = { .tipo = F[c].tipo[3], .fixo = F[c].fixo[3] };
            anuncia_pecas("%s vira a carta", &copia, 1);
            lanca_proximo();
            return;
        }
    }
    if (vez == primeiro && zerou_de_vez(vez)) {  // fez 0: nem aposta, nem o turno do outro
        zerou = vez;
        devolve_mao(outro(vez));
        desfecho();
        return;
    }
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
