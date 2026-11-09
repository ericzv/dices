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
// O dado do Velcro gruda: em vez de sair, volta para a mao (e desgruda). O do
// Chiclete tambem volta, e fica marcado ate abrir a sequencia (gruda_chiclete).
static void descarta_mao(jogador_t *p)
{
    p->n_mao = 0;
    for (int i = 0; i < p->n_col; i++) {
        if ((p->col[i].velcro || p->col[i].chiclete) && p->onde[i] != COMIDO && p->n_mao < MAO) {
            p->onde[i] = NA_MAO;
            p->mao[p->n_mao++] = (int8_t)i;
        } else {
            if (p->onde[i] == NA_MAO) p->onde[i] = FORA;
            p->col[i].chiclete = 0;
        }
        p->col[i].velcro = 0;
    }
}

// Na vez de j, o dado que grudou ja abre a sequencia: no primeiro lugar, ja
// pousado com o mesmo numero, sem rolar (o primeiro e sempre valido). Ele nao
// sai da fila, e o Chiclete dele nao gruda de novo.
// O dado da mao de j que vai grudar (o lugar dele na bolsa), ou -1.
static int chiclete_na_mao(int j)
{
    const jogador_t *p = &J[j];
    for (int m = 0; m < p->n_mao; m++)
        if (p->col[p->mao[m]].chiclete) return p->mao[m];
    return -1;
}
// Poe o dado i (da bolsa de j) grudado no primeiro lugar da fila. A IA usa
// sozinho tambem, numa copia da mesa, para pesar a aposta antes da vez.
static void poe_grudado(int j, int i)
{
    const jogador_t *p = &J[j];
    fila_t *f = &F[j];
    f->idx[0] = (int8_t)i;
    f->tipo[0] = p->col[i].tipo;
    f->fixo[0] = p->col[i].fixo;
    f->n = 1;
    f->grudado = true;
    f->cru[0] = f->valor[0] = chic_valor[j];
    f->lado[0] = chic_lado[j];
    f->lancados = 1;
    aplica_regra(j, 0);
    f->pontos = soma(j, f->est);
    veredito[0] = 0;
}
// Os adesivos que agem no pouso (adesivos_no_pouso: o Infectado, o Barro, a
// Massinha e o Chiclete) mexem na bolsa na hora, e a mesa mostra cada um, por
// vez - o germe voando ate a bolsa, a bola de barro no dado do oponente, a
// massinha virando o Remendo, a bola de chiclete estourando e a goma
// prendendo o dado; o veredito espera (passo.inc).
#define POUSO_MAX 4
static pouso_ad_t pouso_q[POUSO_MAX];
static int n_pouso;
static float t_pouso;
static uint8_t pavio_mais[2][N_FILA];        // a Dinamite: quanto o pavio do dado k de j cresceu neste lance (a mesa mostra no pouso)
static float pouso_dura(const pouso_ad_t *p)
{
    return p->ad == AD_BARRO ? 2.0f : p->ad == AD_MASSINHA ? 1.9f : p->ad == AD_DINAMITE ? 0.9f : 1.6f;
}
static void flutua_em(int j, int k, uint16_t cor, float atraso, const char *fmt, int v);
static void pouso_toca(const pouso_ad_t *p)
{
    if (p->ad == AD_DINAMITE) {                  // o pavio cresceu: o adesivo saltando, o quanto e o chiado curtinho (nao o BUM)
        flutua_em(p->j, p->k, C_FOGO, 0.25f, "pavio +%d", p->n);
        // Cada pavio a mais, um degrau acima (na escala maior, ate a oitava):
        // a pilha crescendo soa subindo.
        static const int8_t DEGRAU[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };
        int pv = p->i >= 0 && p->i < J[p->j].n_col ? J[p->j].pavio[p->i] : DINAMITE_CADA;
        int d = pv / DINAMITE_CADA - 1;
        som_toca_tom(SOM_PAVIO, DEGRAU[d < 0 ? 0 : d > 7 ? 7 : d]);
        return;
    }
    som_toca(p->ad == AD_BARRO && p->capa ? som_adesivo(AD_IMPERMEAVEL) : som_adesivo(p->ad));
    if (p->ad == AD_MASSINHA) {                  // quando a massinha vira o Remendo: o lado que ganhou +2
        int l = 1;
        while (l < AD_LADOS && !(p->lados >> l & 1)) l++;
        flutua_em(p->j, p->k, RGB(130, 204, 112), 1.05f, "+2 no %d", l);
    }
    if (p->ad == AD_CHICLETE) flutua_em(p->j, p->k, C_ROSA, 0.85f, "gruda", 0);   // quando a bola estoura e a goma prende o dado
}
static void adesivos_pousou(int j, int k)
{
    pouso_ad_t o[2];
    int n = adesivos_no_pouso(j, k, o, 2), antes = n_pouso;
    for (int g = 0; g < n && n_pouso < POUSO_MAX; g++) pouso_q[n_pouso++] = o[g];
    if (k >= 0 && k < N_FILA && pavio_mais[j][k]) {   // a Dinamite que nao saiu: o pavio cresceu
        if (n_pouso < POUSO_MAX)
            pouso_q[n_pouso++] = (pouso_ad_t){ (int8_t)j, (int8_t)k, (int8_t)j, F[j].idx[k], AD_DINAMITE, 0, pavio_mais[j][k], 0 };
        pavio_mais[j][k] = 0;
    }
    if (!antes && n_pouso) { t_pouso = 0; pouso_toca(&pouso_q[0]); }
}
static void pouso_passo(float dt)
{
    if (!n_pouso || (t_pouso += dt) < pouso_dura(&pouso_q[0])) return;
    memmove(pouso_q, pouso_q + 1, (size_t)(--n_pouso) * sizeof pouso_q[0]);
    t_pouso = 0;
    if (n_pouso) pouso_toca(&pouso_q[0]);
}
static void gruda_chiclete(int j)
{
    if (F[j].n || F[j].lancados) return;         // ja montou (voltou para a fila)
    int i = chiclete_na_mao(j);
    if (i < 0) return;
    J[j].col[i].chiclete = 0;
    poe_grudado(j, i);
    adesivos_pousou(j, 0);                       // grudado com o Infectado (o Barro, a Massinha) para cima: age ao chegar (o Chiclete, nao)
    plano_rodada = -1;                           // a IA monta a fila de novo, com ele no primeiro lugar
}

// Compra ate 7 da bolsa, ao acaso. Se a bolsa esvazia no meio, tudo o que
// esta fora volta para ela - menos o que ja esta na mao - e a compra segue.
// Quantos j compra por rodada: 7; 9 com a Bolsa Funda; 5 se o oponente tem
// o Saco Furado; 6 com o Copo de Couro. Os trunfos feitos desses amuletos
// (o Mao grande, com 8; a Dona do Salao, que deixa voce com 6) mexem 1 so.
static int ad_moedas[2];                 // moedas do Dourado nesta partida
// Quantos dados a Bolsa Funda (a mais) ou o Saco Furado (a menos, no outro)
// de j mexem: o amuleto, 2; o trunfo do oponente feito dele, 1.
#define AM_MAO 2
static int am_mao(int j, int a) { return j == 0 || op_amul_extra == a ? AM_MAO : 1; }
static int tam_mao(int j)
{
    int n = MAO_NORMAL;
    if (tem_am(j, A_BOLSA)) n += am_mao(j, A_BOLSA);
    if (tem_bolsa(j, B_COPO)) n--;                      // o Copo de Couro: dados maiores, mao menor
    if (tem_am(j ^ 1, A_FURADO)) n -= am_mao(j ^ 1, A_FURADO);
    if (j == 1 && op_nervoso && tem_reg(REG_NERVOSO)) n--;
    if (j == 0 && virada_forca(0) == 2) n--;            // o Crupie forte: voce compra 1 a menos
    return n;
}

static void compra(jogador_t *p)
{
    p->embaralhou = false;
    int quer = tam_mao((int)(p - J));
    if (quer > MAO) quer = MAO;
    int jb = (int)(p - J);                       // o Bis da rodada passada: 1 lugar a mais nesta
    bis_fila[jb] = bis_vem[jb];
    bis_vem[jb] = 0;
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
                if (p->col[i].tipo == D_PISCA) p->col[i].fixo = (uint8_t)(sorte(2) == 0 ? 0 : 1);   // o Pisca: aceso ou nao, meio a meio
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
        if (!f->escondido[k] && !f->curinga[k] && !f->gira[k] && f->tipo[k] != D_UNIVERSO && (b < 0 || f->valor[k] >= f->valor[b])) b = k;   // o Oculto, a Ficha Coringa e o Carrossel ainda nao pararam; o Universo nao se copia
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
        if (J[0].col[J[0].mao[i]].tipo != D_PRESENTE && J[0].col[J[0].mao[i]].tipo != D_UNIVERSO   // o Universo vai sozinho: nao embrulha
            && !J[0].col[J[0].mao[i]].chiclete) ok[n++] = J[0].mao[i];   // nem o grudado pelo Chiclete: ja vai para a mesa, sem ser lancado
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

// A vez de uma pessoa na mesa (nao a maquina, nem o outro computador): a
// plaquinha SUA VEZ desce no painel da sequencia.
static void humor_da_rodada(void);       // desafio/falas.inc
static void avisa_sua_vez(void)
{
    t_sua_vez = !(cpu >> vez & 1) && !(remoto >> vez & 1) ? SUA_VEZ_T : 0;
}

static void marca_online(void);             // online.inc: o marco da partida para o rival conferir
static void nova_rodada(void)
{
    n_pouso = 0;                                 // o que ainda voava no pouso da rodada de antes
    memset(pavio_mais, 0, sizeof pavio_mais);
    marca_online();
    rodada++;
    if (rodada == 1) memset(cosmo_recem, 0, sizeof cosmo_recem);   // partida nova: ninguem acabou de evoluir
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
    if (rodada == 1) des_atras7 = false;
    if (rodada == 7) des_atras7 = J[0].fichas < J[1].fichas;
    // No Desafiante sao 8 rodadas; empatado no fim, ate 3 rodadas extras.
    int max_r = modo == M_DESAFIO ? DES_RODADAS : MAX_RODADAS;
    bool extra = modo == M_DESAFIO && J[0].fichas == J[1].fichas && rodada <= max_r + DES_EXTRAS;
    if (rodada > max_r && !extra) {
        fim_motivo = FM_RODADAS;
        encerra(J[0].fichas == J[1].fichas ? 2 : (J[0].fichas > J[1].fichas ? 0 : 1));
        return;
    }
    // Quem nao paga a entrada perde. Se nenhum dos dois paga, vence quem tem
    // mais fichas (o pote parado na mesa se dividiria igual e nao muda isso);
    // com fichas iguais, empate.
    int ante = ante_da(rodada);
    bool sem0 = J[0].fichas < ante, sem1 = J[1].fichas < ante;
    fim_motivo = FM_FICHAS;
    fim_rodada = rodada;
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
    fosforo_ve();                                // a Caixa de Fosforos: o que ele comprou
    pote += 2 * ante;
    n_voo = 0;
    n_ev = ev_i = 0;
    memset(vis_nroubo, 0, sizeof vis_nroubo);
    memset(vis_delta, 0, sizeof vis_delta);
    memset(vis_bonus, 0, sizeof vis_bonus);
    memset(vis_mult, 0, sizeof vis_mult);
    memset(vis_queima, 0, sizeof vis_queima);
    memset(vis_capa_t, 0, sizeof vis_capa_t);
    memset(vis_espelho, 0, sizeof vis_espelho);
    memset(vis_curinga, 0, sizeof vis_curinga);
    memset(vis_carrossel, 0, sizeof vis_carrossel);
    memset(vis_car_n, 0, sizeof vis_car_n);
    memset(vis_car_ate, 0, sizeof vis_car_ate);
    memset(vis_tremor, 0, sizeof vis_tremor);
    memset(vis_semente, 0, sizeof vis_semente);
    aviso[0] = veredito[0] = nota[0] = 0;
    vez = primeiro;
    cursor = 0;
    t_fase = 0;
    fase = F_ORDEM;
    gruda_chiclete(vez);
    avisa_sua_vez();
    humor_da_rodada();
    som_toca(rodada == 1 ? SOM_PARTIDA : SOM_BOLSA);   // comeca a partida, ou mais uma rodada
}

// Nomes de cada modo: quem joga aqui e sempre VOCE (ou JOGADOR 1 e 2). E as
// cores: com dois na mesma mesa, azul e rosa, para saber de quem e a vez de
// relance; nos outros modos, quem joga aqui fica no latao.
static void poe_nomes(void)
{
    switch (modo) {
    case M_UM:     J[0].nome = "VOCÊ";      J[1].nome = "COMPUTADOR"; break;
    case M_DOIS:   J[0].nome = "JOGADOR 1"; J[1].nome = "JOGADOR 2";  break;
    default:       J[baixo].nome = "VOCÊ";  J[outro(baixo)].nome = "OPONENTE"; break;
    }
    J[0].cor = modo == M_DOIS ? C_CELESTE : C_LATAO;
    J[1].cor = C_ROSA;
}

static void abre_loja(void);
static void som_le_volumes(void);        // painel_som.inc
static bool tem_perfil(void);            // desafio/perfis.inc

void dado_inicia(int n_partidas)
{
    static bool paleta_pronta;
    le_epoca();                          // o Natal (o feltro e os dados dele)
    if (!paleta_pronta) {                // a paleta, com o feltro e a mesa do perfil
        estilo_do_perfil();
        paleta_pronta = true;
        char b[4];                       // a primeira vez: o cursor comeca no Tutorial
        primeira_vez = !tem_perfil() && salva_le("tutorial", b, sizeof b) <= 0;
        if (primeira_vez) menu_cur = MN_TUTORIAL;
    }
    if (!rapido_lido) {                  // a velocidade escolhida da ultima vez
        char b[8];
        int n = salva_le("vel", b, sizeof b), achou = -1;
        for (int v = 0; n == 1 && v < VEL_N; v++) if (b[0] == VEL_GRAVA[v]) achou = v;
        if (achou >= 0) velocidade = achou;
        else if (n > 0) salvo_estragado("vel", b, "A velocidade salva não abriu: uma cópia ficou guardada à parte.");
        rapido_lido = true;
    }
    if (!letra_lida) {                   // a letra maior (tecla L)
        char b[8];
        int n = salva_le("letra", b, sizeof b);
        gfx_letra_maior(n == 1 && b[0] == '1');
        if (n > 0 && !(n == 1 && (b[0] == '0' || b[0] == '1')))
            salvo_estragado("letra", b, "A opção da letra maior não abriu: uma cópia ficou guardada à parte.");
        letra_lida = true;
    }
    static bool som_lido;
    if (!som_lido) {                     // os volumes do painel do som (tecla M)
        som_le_volumes();
        som_lido = true;
    }
    if (!tremor_lido) {                  // e se o tremor da tela foi desligado
        char b[8];
        int n = salva_le("tremor", b, sizeof b);
        tremor_off = n == 1 && b[0] == '0';
        if (n > 0 && !(n == 1 && (b[0] == '0' || b[0] == '1')))
            salvo_estragado("tremor", b, "A opção do tremor da tela não abriu: uma cópia ficou guardada à parte.");
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
    poe_nomes();
    materiais_da_partida(MAT_MARFIM);
    for (int j = 0; j < 2; j++) {
        J[j].fichas = FICHAS_INI;
        colecao_inicial(&J[j]);
    }
    rodada = 0;
    pote = 0;
    memset(bis_fila, 0, sizeof bis_fila);
    memset(bis_vem, 0, sizeof bis_vem);
    memset(ad_moedas, 0, sizeof ad_moedas);
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

// O Estilingue de antes tirou 1: este lance sai no lado mais alto (sem o
// Chumbo, o Avesso e a Nota Dez mexerem). O lance_peca o liga e desliga.
static bool lance_no_maximo;
// As bonecas que a Matrioska abriu no ultimo lance: quantas (0 a 3) e o que
// cada uma tirou (o d6, o d4 e o d2). Empacotado: 2 bits de quantas, 3+3+2 dos numeros.
static uint16_t bonecas_ult;
// O primeiro lance de um dado: um lado ao acaso (o Lastro, de 4 a 12).
static int primeiro_lance(int t, int (*r)(int))
{
    if (lance_no_maximo) return TIPO[t].lados;
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
    case EF_SORTE_REVES: {                   // o d12 da Sorte menos o d4 do Reves
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
    case EF_MATRIOSKA: {                     // abre: o d8 com 6 a 8 solta um d6; o d6 com 5 ou 6, um d4; o d4 com 4, um d2
        static const uint8_t DENTRO[3] = { 6, 4, 2 }, ABRE[3] = { 6, 5, 4 };
        int n = remenda(p, x), soma = n, nb = 0;
        uint16_t b = 0;
        while (nb < 3 && n >= ABRE[nb]) {
            n = r(DENTRO[nb]);
            soma += n;
            b |= (uint16_t)(nb == 0 ? n << 2 : nb == 1 ? n << 5 : n << 8);
            nb++;
        }
        bonecas_ult = (uint16_t)(b | nb);
        return soma;
    }
    // Os de numero fixo nao rolam: o lado e o numero, e so o Remendo age.
    case EF_QUEBRADO: *lado = (int8_t)fixo; break;   // sem sorteio (a pedra de um Reves fica negativa)
    case EF_FANTASMA: *lado = fixo ? (int8_t)fixo : 1; break;
    case EF_PREVISIVEL: *lado = (int8_t)previsivel_valor(fixo); break;   // 3, 4, 5... ate 12
    case EF_RELOGIO:  *lado = relogio_valor(); break;              // o numero da rodada
    case EF_VAZIO:    *lado = 1; return ad_no_lado(p, 1) == AD_REMENDO ? 2 : 0;   // um lado so: o do 0
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
// para um deles); caindo no Avesso, o dado vira do avesso; na Nota Dez, vira
// o lado mais alto (uma vez so: o lado de la nao faz virar de novo); o
// Remendo soma 2 ao lado que ficou para cima (em sorteia_valor_de, antes dos
// poderes). 'lado' recebe o lado de cima (0: fora dos lados do dado);
// 'antes', o lado do Avesso ou da Nota Dez de onde ele virou (0: nao virou).
// Um pouso do dado t, com os adesivos dele: o Chumbo puxa o lance x para o
// lado dele (um: um terco das vezes; dois, o maximo: cada um um terco); caindo
// no Avesso, vira o lado oposto; na Nota Dez, o mais alto (uma vez so: o lado
// de la nao faz virar de novo). 'antes': o lado de onde virou (0: nao virou).
static int pousa_lado(int t, const peca_t *p, int x, int (*r)(int), int *antes)
{
    int ch[AD_LADOS], n = 0;
    for (int l = ad_lado_min(t); l <= ad_lado_max(t); l++) if (ad_no_lado(p, l) == AD_CHUMBO) ch[n++] = l;
    if (n == 1 && r(3) == 1) x = ch[0];
    else if (n > 1) { int q = r(3); if (q <= n) x = ch[q - 1]; }
    *antes = 0;
    int a = ad_no_lado(p, x);
    if (a == AD_AVESSO) { *antes = x; x = ad_oposto(t, x); }
    else if (a == AD_NOTA_DEZ && x != ad_lado_max(t)) { *antes = x; x = ad_lado_max(t); }
    return x;
}

static int sorteia_peca(int t, int fixo, const peca_t *p, int (*r)(int), int *menor, int *lado, int *antes)
{
    int ef = TIPO[t].efeito;
    int x0 = primeiro_lance(t, r);
    *antes = 0;
    bonecas_ult = 0;
    bool pousa = p && ef != EF_QUEBRADO && ef != EF_FANTASMA && ef != EF_PREVISIVEL && ef != EF_RELOGIO
              && ef != EF_DIGITAL && ef != EF_FICHA_CORINGA && ef != EF_CARROSSEL   // os que nao valem o que rolam: o Chumbo e o Avesso nao mexem
              && !lance_no_maximo;                            // nem no lance do Estilingue
    if (pousa) x0 = pousa_lado(t, p, x0, r, antes);
    if (pousa && ef == EF_VICIADO) {
        // O Viciado rola duas vezes, e cada lance e um pouso de verdade: o
        // segundo tambem passa pelo Chumbo, pelo Avesso e pela Nota Dez. Fica o
        // maior (com o Remendo); 'antes' e o do que ficou.
        int b, y = pousa_lado(t, p, r(TIPO[t].lados), r, &b);
        int vx = remenda(p, x0), vy = remenda(p, y);
        if (vy > vx) { *lado = y; *menor = vx; *antes = b; }
        else         { *lado = x0; *menor = vy; }
        int x = vy > vx ? vy : vx;
        if (*lado < ad_lado_min(t) || *lado > ad_lado_max(t)) *lado = 0;
        return x;
    }
    int x = sorteia_valor_de(t, fixo, x0, p, r, menor, lado);
    // O Previsivel nao rola, mas o lado da escada que ficou para cima vira
    // com o Avesso (o 1 vira 12) e com a Nota Dez, como os outros.
    if (p && ef == EF_PREVISIVEL && *lado >= ad_lado_min(t) && *lado <= ad_lado_max(t)) {
        int a = ad_no_lado(p, *lado);
        if (a == AD_AVESSO) { *antes = *lado; *lado = ad_oposto(t, *lado); x = remenda(p, *lado); }
        else if (a == AD_NOTA_DEZ && *lado != ad_lado_max(t)) { *antes = *lado; *lado = ad_lado_max(t); x = remenda(p, *lado); }
    }
    if (*lado < ad_lado_min(t) || *lado > ad_lado_max(t)) *lado = 0;
    return x;
}

// O Presente desembrulha no lance: um dado raro ao acaso, ou o D20 (o dado
// especial, que nao se vende) - no Desafiante, os seus so entre os que voce
// ja abriu -, que sai dele e fica no lugar dele na
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
        if ((TIPO[t].raridade != RAR_RARO && t != D_D20) || sem_sorteio(t) || t == D_PRESENTE) continue;
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
// Reflexivo, rola de novo e vale o segundo (x); com o Papel Bomba, rola de
// novo e vale a soma dos dois. Um dos dois por lance: o lado do segundo lance
// nao faz rolar de novo.
// O Piao e a Gelatina rolam de novo de verdade: n_re lances de antes (re_x,
// re_lado), que o dado mostra um a um antes do que fica.
typedef struct { int x, menor, lado, x1, menor1, lado1, antes1; bool reflete, bomba;
                 uint8_t giros; bool quicou; uint16_t bonecas;
                 uint8_t n_re; int re_x[2]; uint8_t re_lado[2]; } lance_t;   // (re: ate PIAO_GIROS)
static lance_t lanca_peca(int t, int fixo, const peca_t *pc, int (*r)(int))
{
    lance_t l;
    int antes;
    l.giros = 0;
    l.quicou = false;
    l.n_re = 0;
    l.x = l.x1 = sorteia_peca(t, fixo, pc, r, &l.menor, &l.lado, &antes);
    l.bonecas = bonecas_ult;
    l.menor1 = l.menor; l.lado1 = l.lado; l.antes1 = antes;
    int a = ad_no_lado(pc, l.lado);
    l.reflete = a == AD_REFLEXIVO || a == AD_BOMBA;
    l.bomba = a == AD_BOMBA;
    if (l.reflete) {
        // O Previsivel rola de novo de verdade: como um d12 comum, com os
        // adesivos (a escada segue contando do mesmo jeito).
        int t2 = TIPO[t].efeito == EF_PREVISIVEL ? D_D12 : t;
        int x2 = sorteia_peca(t2, fixo, pc, r, &l.menor, &l.lado, &antes);
        l.x = l.bomba ? l.x1 + x2 : x2;
        l.bonecas = bonecas_ult;
    }
    return l;
}

// O Estilingue logo antes do dado k de j tirou 1: este sai no numero mais alto.
static bool estilingue_armado(int j, int k)
{
    return k > 0 && k < N_FILA && TIPO[tipo_de(j, k - 1)].efeito == EF_ESTILINGUE && F[j].valor[k - 1] == 1;
}
// O lance inteiro do dado k de j, com os poderes dele que mexem no lance (o
// da mesa e o que a IA imagina usam este): o Estilingue de antes armado poe
// no maximo; o Piao caido num lado sem adesivo (e com algum) gira de novo,
// ate PIAO_GIROS vezes, e fica o primeiro lado com adesivo ou o ultimo giro;
// a Gelatina que tirou menos que o de antes (o que derrubaria a sequencia)
// quica e rola mais uma vez, e vale o segundo lance (que tambem pode cair).
// *fora: o lance descartado, ou -1. Na mesa, o mesmo dado mostra os lances de antes e rola de novo (re_x).
#define PIAO_GIROS 2
static lance_t lanca_do_dado(int j, int k, int t, int fixo, const peca_t *pc, int (*r)(int), int *fora)
{
    lance_no_maximo = estilingue_armado(j, k);
    lance_t l = lanca_peca(t, fixo, pc, r);
    *fora = -1;
    int ef = TIPO[t].efeito;
    int rx[PIAO_GIROS], rl[PIAO_GIROS], nre = 0;
    if (ef == EF_PIAO && pc && ad_quantos(pc) > 0 && !lance_no_maximo)
        for (int g = 0; g < PIAO_GIROS && !ad_no_lado(pc, l.lado); g++) {
            *fora = rx[nre] = l.x;
            rl[nre++] = l.lado;
            l = lanca_peca(t, fixo, pc, r);
            l.giros = (uint8_t)(g + 1);
        }
    if (ef == EF_GELATINA && !lance_no_maximo && k > 0) {
        int lr = ultimo_da_regra(j, F[j].est, k);
        if (lr >= 0 && cai_depois(j, l.x, F[j].valor[lr])) {
            *fora = rx[nre] = l.x;
            rl[nre++] = l.lado;
            l = lanca_peca(t, fixo, pc, r);      // vale o segundo
            l.quicou = true;
        }
    }
    l.n_re = (uint8_t)nre;
    for (int i = 0; i < nre; i++) { l.re_x[i] = rx[i]; l.re_lado[i] = (uint8_t)rl[i]; }
    lance_no_maximo = false;
    return l;
}

static void lanca_proximo(void)
{
    fila_t *f = &F[vez];
    int k = f->lancados;
    int t = f->tipo[k], lados = TIPO[t].lados;   // o tipo de verdade: o Camaleao rola o proprio d8
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
    if (t == D_REVOLVER) som_toca(SOM_CARREGA);  // lancado, valendo ou nao: entra uma bala

    // Acumulador: entra com o bonus que guardou (ele cresce em passo.inc,
    // cada vez que pousa valendo).
    if ((t == D_ACUMULADOR || t == D_AUTOFAGIA || t == D_PREVISIVEL || t == D_BALAO) && f->idx[k] >= 0 && f->idx[k] < J[vez].n_col)
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
    int fora;                                // o giro do Piao (ou o quique da Gelatina) que ficou de fora (so na IA)
    lance_t l = lanca_do_dado(vez, k, t, f->fixo[k], pc, rola, &fora);
    int postit = -1;                         // o lance que o Post-it (ou a Casca) descartou
    bool tem_postit = k > 0 && ad_fila(vez, k - 1) == AD_POSTIT;
    // A Casca de Banana do oponente: no 4o dado (com mais cascas, do 3o, do
    // 2o, do 1o), o lance escorrega - rola duas vezes e fica o menor. A bolha
    // do Impermeavel segura a casca; o Post-it dele e a casca se anulam (rola
    // uma vez so).
    int casca = 0;
    if (casca_pega(vez, k)) {
        casca = (pc && pc->capa) || t == D_UNIVERSO ? 2 : tem_postit ? 3 : 1;
        if (casca == 3) tem_postit = false;   // (o som vem quando ela chega no dado: casca_pisa)
    }
    // A Bussola no dado de antes: este, caindo num lado sem adesivo (e tendo
    // algum), rola de novo uma vez: fica o segundo se ele cair num adesivo; se
    // nao, o maior dos dois. O outro cai ao lado.
    bool tem_bussola = k > 0 && ad_fila(vez, k - 1) == AD_BUSSOLA && pc && ad_quantos(pc) > 0
                    && !ad_no_lado(pc, l.lado);
    int fora2;
    if (tem_postit) {
        lance_t l2 = lanca_do_dado(vez, k, t, f->fixo[k], pc, rola, &fora2);
        if (l2.x > l.x) { lance_t c = l; l = l2; l2 = c; }
        postit = l2.x;
        som_toca(som_adesivo(AD_POSTIT));
    } else if (tem_bussola && casca != 1) {
        lance_t l2 = lanca_do_dado(vez, k, t, f->fixo[k], pc, rola, &fora2);
        if (ad_no_lado(pc, l2.lado) || l2.x > l.x) { postit = l.x; l = l2; }
        else postit = l2.x;
        som_toca(som_adesivo(AD_BUSSOLA));
    } else if (casca == 1) {
        lance_t l2 = lanca_do_dado(vez, k, t, f->fixo[k], pc, rola, &fora2);
        if (l2.x < l.x) { lance_t c = l; l = l2; l2 = c; }
        postit = l2.x;
    }
    f->bonecas[k] = l.bonecas;
    int x = l.x, menor = l.menor, lado = l.lado;
    // A Dinamite: lancado sem cair nela, o pavio do dado queima mais um pouco.
    pavio_mais[vez][k] = 0;
    if (pc && ad_conta(pc, AD_DINAMITE) && ad_no_lado(pc, lado) != AD_DINAMITE) {
        uint8_t *pv = &J[vez].pavio[f->idx[k]], antes = *pv;
        *pv = (uint8_t)(*pv + DINAMITE_CADA > DINAMITE_MAX ? DINAMITE_MAX : *pv + DINAMITE_CADA);
        pavio_mais[vez][k] = (uint8_t)(*pv - antes);   // a mesa mostra no pouso (no teto, nada)
    }
    int x1 = l.x1, lado1 = l.lado1, antes1 = l.antes1, menor1 = l.menor1;
    bool reflete = l.reflete;
    f->cru[k] = x;
    f->valor[k] = x;
    f->lado[k] = (uint8_t)lado;
    f->bomba[k] = l.bomba;
    f->bomba1[k] = (int8_t)l.x1;
    // O Papel Carbono do lado de cima: no pouso, o numero vira o do ultimo
    // valido da sequencia (a regra o copia de novo quando o dado entra).
    int lr = ultimo_da_regra(vez, f->est, k);
    bool copia = carbono_age(vez, k) && lr >= 0;
    fatia(vez, k, rola);
    // O Previsivel sobe a cada lance, valendo ou nao (ate 12).
    if (t == D_PREVISIVEL && f->idx[k] >= 0 && f->idx[k] < J[vez].n_col && J[vez].col[f->idx[k]].fixo < 12 - PREVISIVEL_COMECA)
        J[vez].col[f->idx[k]].fixo++;

    voo_t *v = &voo[0];
    memset(v, 0, sizeof *v);
    v->dono = (uint8_t)vez;
    v->slot = (uint8_t)k;
    v->tipo = (uint8_t)t;
    v->casca = (uint8_t)casca;
    v->giros = l.giros;
    v->quicou = l.quicou;
    v->bonecas = l.bonecas;
    v->estilingue = estilingue_armado(vez, k);
    if (v->estilingue) som_toca(SOM_ESTILINGUE);   // a pedra do Estilingue sai atras dele
    bool sr = TIPO[t].efeito == EF_SORTE_REVES;
    v->valor = sr ? x1 + menor1 : x1;        // no ar, o da Sorte
    v->lado = (uint8_t)lado1;
    if (antes1) {                            // pousa no Avesso (ou na Nota Dez) e vira
        v->etapa = 1;
        v->ad_vira = (uint8_t)ad_no_lado(pc, antes1);
        v->valor_v = v->valor;
        v->lado_v = (uint8_t)lado1;
        v->valor = antes1;
        v->lado = (uint8_t)antes1;
    }
    if (reflete) {                           // e depois (ou so) o Reflexivo, ou o Papel Bomba
        v->reflete = true;
        v->ad_rola = l.bomba ? AD_BOMBA : AD_REFLEXIVO;
        if (!v->etapa) v->etapa = 2;
        v->valor2 = sr ? x + menor : x;
        v->menor2 = menor;
        v->lado2 = (uint8_t)lado;
    }
    if (copia) {                             // e por ultimo o Papel Carbono copia
        v->copia = true;
        v->valor_c = f->valor[lr];
        if (!v->etapa) v->etapa = 3;
    }
    // O Piao e a Gelatina rolam de novo de verdade: o dado pousa no lance de
    // antes, para um instante e gira no lugar (o Piao) ou quica (a Gelatina)
    // ate o seguinte; no ultimo, seguem os adesivos dele.
    if (l.n_re) {
        int fv = v->valor;
        uint8_t fl = v->lado;
        int8_t fe = v->etapa;
        v->giros = 0;                        // (o giro e o quique agora sao lances de verdade)
        v->quicou = false;
        v->rr_n = v->rr_i = 0;
        for (int i = 1; i < l.n_re; i++) { v->rr_v[v->rr_n] = l.re_x[i]; v->rr_l[v->rr_n++] = l.re_lado[i]; }
        v->rr_v[v->rr_n] = fv; v->rr_l[v->rr_n++] = fl;
        v->rr_etapa = fe;
        v->valor = l.re_x[0];
        v->lado = l.re_lado[0];
        v->etapa = 5;
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
// mostra o lado dele aceso, pula e vira (a Nota Dez tambem, e o lado mais
// alto chega com um brilho de ouro); o Reflexivo mostra o dele e o dado sai
// rolando de novo (o Papel Bomba estoura antes); o Papel Carbono passa a
// folha por cima e o numero sai copiado. Devolve true enquanto isso dura.
#define AD_MOSTRA 0.32f                  // o adesivo aceso antes de agir
#define AD_VIRA   0.30f                  // o pulo do Avesso
#define AD_COPIA  0.42f                  // a folha do Papel Carbono passando
#define RE_MOSTRA 0.40f                  // o Piao e a Gelatina parados no lance de antes
#define RE_GIRO   0.62f                  // o Piao girando de novo no lugar
#define RE_QUIQUE 0.42f                  // a Gelatina no ar, quicando
static void estoura(float x, float y);   // o estouro do Papel Bomba (mesa.inc)
static bool ad_etapa_passo(float dt)
{
    voo_t *v = &voo[0];
    if (!n_voo || !v->etapa) return false;
    float t0 = v->t_etapa;
    v->t_etapa += dt;
    if (t0 < 0.01f && v->t_etapa >= 0.01f && v->etapa <= 3)
        som_toca(som_adesivo(v->etapa == 1 ? v->ad_vira : v->etapa == 2 ? v->ad_rola : AD_CARBONO));
    if (v->etapa == 5) {                     // o Piao gira de novo no lugar; a Gelatina quica
        float dur = v->tipo == D_PIAO ? RE_GIRO : RE_QUIQUE;
        if (t0 < RE_MOSTRA && v->t_etapa >= RE_MOSTRA && v->tipo == D_PIAO) som_toca(SOM_PIAO);   // (a Gelatina: o bloop no pouso)
        if (v->t_etapa < RE_MOSTRA + dur) return true;
        v->valor = v->face = v->rr_v[v->rr_i];
        v->lado = v->rr_l[v->rr_i];
        v->rr_i++;
        assenta(v);                          // deita no lance novo (a Gelatina faz bloop)
        v->t_etapa = 0;
        v->etapa = v->rr_i < v->rr_n ? 5 : v->rr_etapa;
        return true;
    }
    if (v->etapa == 1) {
        float q = (v->t_etapa - AD_MOSTRA) / AD_VIRA;
        if (q >= 0.5f && !v->virou) {            // no alto do pulo, o outro lado
            v->virou = true;
            v->valor = v->face = v->valor_v;
            v->lado = v->lado_v;
        }
        if (q < 1) return true;
        som_toca(TIPO[v->tipo].lados == 2 ? SOM_MOEDA_POUSA : SOM_POUSA);
        assenta(v);
        v->t_etapa = 0;
        v->etapa = v->reflete ? 2 : v->copia ? 3 : 0;
        return v->etapa != 0;
    }
    if (v->etapa == 3) {
        float q = (v->t_etapa - AD_MOSTRA) / AD_COPIA;
        if (q >= 0.5f && v->valor != v->valor_c) v->valor = v->face = v->valor_c;   // a folha no meio: o numero copiado
        if (q < 1) return true;
        v->etapa = 0;
        v->copia = false;
        return false;
    }
    if (v->t_etapa < AD_MOSTRA) return true;
    if (v->ad_rola == AD_BOMBA) estoura(v->x, v->y);   // o papel estoura e o dado sai voando
    // O Reflexivo: o dado (e o fantasma do Viciado ou do Reves) sai rolando de
    // novo; o lance que o Post-it descartou fica onde parou.
    for (int i = 0; i < n_voo; i++) {
        voo_t *w = &voo[i];
        if (w->fantasma && !v->menor2) continue;
        w->parado = false;
        w->pouso = w->t_assenta = 0;
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
    v->etapa = v->copia ? 3 : 0;             // caindo no Papel Carbono, ele copia quando parar
    v->t_etapa = 0;
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
    gruda_chiclete(vez);
    avisa_sua_vez();
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
        if ((d->fermento2[j] >> k & 1) && p->col[i].fermento < FERMENTO_MAX) p->col[i].fermento++;   // carimbado: cresce de novo
        if (d->raspa[j] >> k & 1)                 // a Raspadinha sai do lado de onde agiu (o de cima, ou o da Dupla Face, ou o da Moldura)
            for (int l = 1; l <= AD_LADOS; l++)
                if ((d->raspa_lados[j][k] >> l & 1) && ad_no_lado(&p->col[i], l) == AD_RASPADINHA) ad_cola(&p->col[i], l, AD_NADA);
        if (d->dinamite[j] >> k & 1) p->pavio[i] = 0;    // estourou ou apagou
        if (d->selo_rompe[j] >> k & 1) p->col[i].selo = 0;
        if (d->selo_lacra[j] >> k & 1) p->col[i].selo = 1;
        if (d->velcro[j] >> k & 1) p->col[i].velcro = 1;
        if (d->capa[j] >> k & 1) p->col[i].capa = 1;      // o Impermeavel: ate o fim da partida
        if ((d->fig_cresce[j] >> k & 1) && p->col[i].figurinha < FIGURINHA_MAX) p->col[i].figurinha++;   // junta mais uma
        if ((d->fig_cresce2[j] >> k & 1) && p->col[i].figurinha < FIGURINHA_MAX) p->col[i].figurinha++;
        // O Fragil anulado quebra: vira um Quebrado com o numero que tirou.
        if ((d->fragil[j] >> k & 1) && F[j].tipo[k] != D_QUEBRADO && F[j].tipo[k] != D_FANTASMA) {
            int v = F[j].valor[k] > 99 ? 99 : F[j].valor[k] < -99 ? -99 : F[j].valor[k] ? F[j].valor[k] : 1;
            p->col[i] = (peca_t){ .tipo = D_QUEBRADO, .fixo = (uint8_t)(int8_t)v };
        }
    }
    if (d->bis[j]) bis_vem[j] = 1;
    ad_moedas[j] += d->moedas[j];
    // (O Infectado, o Barro e a Massinha ja mexeram nas bolsas no pouso.)
}

// O ciclo cosmico: a peca vira o estagio t, sem os adesivos. Evoluindo, volta
// para a mao na rodada seguinte (como o Velcro); no Big Bang, nao.
static void cosmo_vira(peca_t *p, int t, bool volta)
{
    *p = (peca_t){ .tipo = (uint8_t)t, .velcro = volta };
}

// O ciclo cosmico que termina a rodada valendo evolui (est: como cada dado
// terminou; quando alguem corre, como estava na sequencia). Na rodada
// seguinte, ele chega na mao com o nome novo (menos depois do Big Bang, que
// o Universo, que nunca cai, sempre faz).
static void cosmo_ficam(int j, const uint8_t *est)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        int i = F[j].idx[k];
        if (i < 0 || i >= J[j].n_col || J[j].col[i].tipo != F[j].tipo[k]) continue;
        peca_t *p = &J[j].col[i];
        if (!cosmico(p->tipo) || est[k] != V_VALIDO) continue;
        cosmo_recem[j][i] = p->tipo != D_UNIVERSO;
        cosmo_vira(p, cosmo_proximo(p->tipo), p->tipo != D_UNIVERSO);
    }
}

// O que os dados guardam da rodada, na peca da bolsa: a Furia, o que o lance
// mexeu no bonus (valendo ou nao); a Onda valendo, o que tirou - e com 9 vira
// Tsunami, de vez; os Tres Patinhos, valendo ou nao, mais uma vez; o Revolver, valendo
// ou nao, mais uma bala (e o tambor vazio depois do tiro); e o ciclo cosmico
// evolui (cosmo_ficam).
// O Balao enche BALAO_CADA por rodada fora da fila (ou anulado), sem limite
// (so o que cabe no fixo, BALAO_GUARDA).
static int balao_enche(int g) { return g + BALAO_CADA > BALAO_GUARDA ? BALAO_GUARDA : g + BALAO_CADA; }
// Os contadores que andam so por terem sido lancados, valendo ou nao: os
// Tres Patinhos (mais uma vez) e o Revolver (mais uma bala; com a quarta,
// disparou e o tambor esvazia). Valem tambem quando a rodada acaba porque
// alguem correu (contadores_correu). (O Pisca nao alterna: acende ou nao,
// ao acaso, cada vez que vem para a mao - compra.)
static bool conta_lance(peca_t *p, int j, int k)
{
    if (p->tipo == D_PATINHOS) { if (F[j].fixo[k] < 255) p->fixo = (uint8_t)(F[j].fixo[k] + 1); return true; }
    if (p->tipo == D_REVOLVER) { p->fixo = revolver_dispara(F[j].fixo[k]) ? 0 : (uint8_t)(F[j].fixo[k] + 1); return true; }
    return false;
}
// A peca da bolsa do dado k da fila de j (NULL se ja nao e ele).
static peca_t *peca_da_fila(int j, int k)
{
    int i = F[j].idx[k];
    if (i < 0 || i >= J[j].n_col || J[j].col[i].tipo != F[j].tipo[k]) return NULL;
    return &J[j].col[i];
}
// Alguem correu: nao ha fim de rodada, mas o que foi lancado conta.
static void contadores_correu(int j)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        peca_t *p = peca_da_fila(j, k);
        if (p) conta_lance(p, j, k);
    }
}
static void contadores_ficam(int j, const desfecho_t *d)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        peca_t *p = peca_da_fila(j, k);
        if (!p) continue;
        bool vale = d->est[j][k] == V_VALIDO;
        if (conta_lance(p, j, k)) continue;
        if (p->tipo == D_FURIA) p->fixo = (uint8_t)(int8_t)furia_depois(F[j].fixo[k], F[j].valor[k]);
        else if (p->tipo == D_ONDA && vale) {
            if (d->onda[j] >> k & 1) { p->tipo = D_TSUNAMI; p->fixo = 0; }
            else p->fixo = (uint8_t)onda_depois(F[j].fixo[k], F[j].valor[k]);
        } else if (p->tipo == D_MEIA && vale) p->fixo = 0;                          // pagou: esvazia
        else if (p->tipo == D_MEIA && d->est[j][k] == V_ANULADO) p->fixo = (uint8_t)meia_depois(F[j].fixo[k]);
        else if (p->tipo == D_BALAO && (d->balao[j] >> k & 1)) p->fixo = 0;        // estourou: volta vazio
        else if (p->tipo == D_BALAO && d->est[j][k] == V_ANULADO) p->fixo = (uint8_t)balao_enche(F[j].fixo[k]);
    }
    // O Balao que ficou fora da fila nesta rodada (na bolsa ou na mao)
    // enche tambem.
    for (int i = 0; i < J[j].n_col; i++) {
        peca_t *p = &J[j].col[i];
        if (p->tipo != D_BALAO || J[j].onde[i] == COMIDO) continue;
        bool lancada = false;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) lancada |= F[j].idx[k] == i;
        if (!lancada) p->fixo = (uint8_t)balao_enche(p->fixo);
    }
    cosmo_ficam(j, d->est[j]);
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
        if (vidro_quebra(j, k)) continue;                                  // estilhacado: ja sai
        if (impermeavel(j, k)) continue;                                   // com a capa, nao vira pedra
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
// O fim da rodada ainda depende da sorte, mesmo que ninguem jogue mais: a
// carta do Coringa por virar (e ser lancada), um Oculto escondido, um
// Terremoto por sacudir, a Casca de Banana de quem joga depois (o 4o dado
// de quem abriu ainda vai escorregar). A previa do fim nao sabe o que vai dar.
static bool casca_no_fim(void)
{
    int n = cascas_de(outro(primeiro));
    for (int i = 0; i < n; i++) {
        int a = CASCA_DADO + 1 - n + i;
        if (a < F[primeiro].lancados && F[primeiro].est[a] == V_VALIDO) return true;
    }
    return false;
}
static bool fim_incerto(void)
{
    if (carta_posta(primeiro) || tem_escondido(0) || tem_escondido(1) || tremor_na_mesa() || casca_no_fim()) return true;
    // O Espelho que espera vira no fim e a sequencia dele e refeita: so entao
    // um Terremoto pode passar a valer, ou o dado em que a Casca pega. A
    // previa vira o Espelho e diz (previa_sorte).
    bool espera = false;
    for (int j = 0; j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) espera |= F[j].espera[k];
    if (!espera) return false;
    desfecho_t d;
    calcula_desfecho(&d, false);
    return previa_sorte;
}
// Algo na mesa ainda vira sozinho no fim: alem do que depende da sorte, o
// Espelho esperando o maior do outro, a Ficha Coringa com o curinga para
// cima e o Carrossel girando. O botao "parar" entao e so "parar": nem vence, nem perde por tanto.
static bool fim_em_aberto(void)
{
    if (fim_incerto()) return true;
    for (int j = 0; j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
            if (F[j].espera[k] || F[j].curinga[k] || F[j].gira[k]) return true;
    return false;
}
// A fila do Coringa ficou pronta com tres dados: o quarto e a copia, ou a
// carta (fora da bolsa dele: idx -1).
static void coringa_completa(int j)
{
    fila_t *f = &F[j];
    if (!coringa_joga(j) || f->n != 3) return;
    peca_t c;
    f->carta = !melhor_lance(outro(j), &c);
    // Jogando depois, sem nada seu para copiar (so o Universo, ou dados que
    // ainda esperam o fim): nao ha quarto dado. A carta e so de quem abre: e
    // ela que vira depois do seu turno.
    if (f->carta && j != primeiro) { f->carta = false; return; }
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
    f->est[3] = V_ESPERA; tira_tag(f, 3);
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

// Quem venceu a rodada, w, tem o Mau-Olhado: o amuleto poe 2 Fantasmas na
// bolsa do outro; a Bruxaria da Bruxa (o trunfo feito dele), 1.
#define MAU_OLHADO_N 2
static void mau_olhado(int w)
{
    if (!tem_am(w, A_MAU_OLHADO)) return;
    int r = outro(w), n = w == 0 || op_amul_extra == A_MAU_OLHADO ? MAU_OLHADO_N : 1, postos = 0;
    for (int i = 0; i < n; i++) postos += poe_fantasma(r, false);
    if (!postos) return;
    size_t m = strlen(aviso);
    if (postos > 1) snprintf(aviso + m, sizeof aviso - m, r == 0 ? "  ·  %d Fantasmas entram na sua bolsa" : "  ·  %d Fantasmas entram na bolsa dele", postos);
    else snprintf(aviso + m, sizeof aviso - m, r == 0 ? "  ·  um Fantasma entra na sua bolsa" : "  ·  um Fantasma entra na bolsa dele");
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
    c.chiclete = 0;                              // a marca de quem grudou nesta rodada e so do original
    p->col[p->n_col] = c;
    p->onde[p->n_col++] = NA_BOLSA;
    return true;
}

// Tira da bolsa de j o dado i: os de depois descem um lugar, cada um com o
// que e dele na bolsa - o lugar (onde), o pavio da Dinamite e a marca do
// ciclo cosmico que acabou de evoluir.
static void tira_da_bolsa(int j, int i)
{
    jogador_t *p = &J[j];
    if (i < 0 || i >= p->n_col) return;
    for (int m = i; m + 1 < p->n_col; m++) {
        p->col[m] = p->col[m + 1];
        p->onde[m] = p->onde[m + 1];
        p->pavio[m] = p->pavio[m + 1];
        cosmo_recem[j][m] = cosmo_recem[j][m + 1];
    }
    p->n_col--;
    p->pavio[p->n_col] = 0;
    cosmo_recem[j][p->n_col] = false;
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
    int n_quebra = 0, n_cinza = 0, n_vidro = 0, n_mist = 0, n_coringa = 0, n_capa = 0, n_inteiro = 0;
    // Os Jingle Bells que terminam valendo deixam uma copia na bolsa: tiradas
    // antes que os dados que saem mudem as posicoes da bolsa.
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
            if (F[j].tipo[k] == D_QUEBRADO) {       // com a capa do Impermeavel, nao se desfaz
                if (impermeavel(j, k)) n_inteiro++;
                else { F[j].quebra[k] = true; some = true; n_quebra++; }
            }
            if (vidro_quebra(j, k)) { F[j].quebra[k] = true; some = true; n_vidro++; }
            if (vis_queima[j][k] && !some)  { F[j].cinza[k] = true; some = true; n_cinza++; }
            if (j == 0 && !some && coringa_destroi() && F[j].tipo[k] != D_FANTASMA
                && F[0].est[k] == V_ANULADO) {         // o estado final (no desfecho, ja com os ataques)
                if (impermeavel(j, k)) n_capa++;      // com a capa, fica
                else { F[j].quebra[k] = true; some = true; n_coringa++; }
            }
            if (some && F[j].idx[k] >= 0) saem[n++] = F[j].idx[k];   // a copia do Coringa nao e da bolsa
        }
        for (int a = 1; a < n; a++)
            for (int b = a; b > 0 && saem[b] > saem[b - 1]; b--) {
                int t = saem[b]; saem[b] = saem[b - 1]; saem[b - 1] = t;
            }
        jogador_t *p = &J[j];
        int tirados[N_FILA], nt = 0;
        for (int i = 0; i < n; i++) {
            if (em_jogo(p) <= FILA_NORMAL) {
                p->col[saem[i]] = nova_peca(D_D4);
                p->pavio[saem[i]] = 0;
                cosmo_recem[j][saem[i]] = false;
                continue;
            }
            tira_da_bolsa(j, saem[i]);
            tirados[nt++] = saem[i];
        }
        // A fila segue apontando para a bolsa (o resultado ainda mostra os
        // adesivos de cada um): os que sairam ficam sem peca e os de depois
        // descem junto.
        for (int k = 0; k < F[j].n && k < N_FILA && nt; k++) {
            int x = F[j].idx[k], novo = x;
            if (x < 0) continue;
            for (int i = 0; i < nt; i++) {
                if (tirados[i] == x) { novo = IDX_SAIU; break; }
                if (tirados[i] < x) novo--;
            }
            F[j].idx[k] = (int8_t)novo;
        }
        p->n_mao = 0;
    }
    if (n_cinza && n_quebra) snprintf(nota, sizeof nota, "um dado vira cinza, outro se quebra");
    else if (n_cinza)        snprintf(nota, sizeof nota, n_cinza > 1 ? "os dados queimados viram cinza" : "o dado queimado vira cinza");
    else if (n_quebra)       snprintf(nota, sizeof nota, n_quebra > 1 ? "Quebrados se desfazem" : "o Quebrado se desfaz");
    else if (n_inteiro)      snprintf(nota, sizeof nota, "o Impermeável segura o Quebrado inteiro");
    if (n_vidro && !n_cinza && !n_quebra) snprintf(nota, sizeof nota, "cacos de vidro saem da mesa");
    else if (n_vidro)        snprintf(nota, sizeof nota, "dados se desfazem, e o vidro também");
    int n_sino = 0;
    for (int j = 0; j < 2; j++) {
        jingle_copias[j] = 0;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
            if (F[j].tipo[k] == D_JINGLE && F[j].est[k] == V_VALIDO && poe_copia_jingle(j, &sino[j][k])) {   // so o que termina valendo
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
            int quantos = F[j].tipo[k] == D_ASSOMBRACAO ? ASSOMBRA_FANTASMAS : 1, pos = 0;
            for (int n = 0; q >= 0 && n < quantos; n++) pos += poe_fantasma(q, false);
            if (pos > 1) snprintf(nota, sizeof nota, "%s ganha %d Fantasmas (%s)", J[q].nome, pos, TIPO[F[j].tipo[k]].nome);
            else if (pos) snprintf(nota, sizeof nota, "%s ganha um Fantasma (%s)", J[q].nome, TIPO[F[j].tipo[k]].nome);
        }
    if (n_coringa)
        snprintf(nota, sizeof nota, n_coringa > 1 ? "o Coringa destrói %d dados seus anulados" : "o Coringa destrói o seu dado anulado", n_coringa);
    else if (n_capa)
        snprintf(nota, sizeof nota, "o Impermeável protege o dado do Coringa");
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
        mau_olhado(w);                                                // Mau-Olhado, a Bruxaria
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
    if (venc_mao == 0 && modo == M_DESAFIO) des_rodada_vencida(seu_pote);
    if (venc_mao == 0) fala_evento(FA_PERDE_R);
    else if (venc_mao == 1) fala_evento(FA_GANHA_R);
    if (venc_mao == 0) rival_reage(F[0].total >= RODADA_GRANDE ? RM_COMBO : RM_PERDE_R);
    else if (venc_mao == 1) rival_reage(RM_GANHA_R);
    hist_rodada(seu_pote);
    desfaz_embrulho();                       // o embrulho do Papai Noel nao lancado volta a ser o dado
    memset(cosmo_recem, 0, sizeof cosmo_recem);
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
        case D_BURACO:  return SOM_BURACO;
        default:        return SOM_ANULA;
        }
    case EV_ONDA:    return SOM_ONDA;
    case EV_TREMOR:                      // a mesa ronca; o dado bate depois (passo.inc). A casca: o escorregao
        if (e->ad == AD_CASCA) return e->bloqueado ? som_adesivo(AD_IMPERMEAVEL) : SOM_AD_CASCA;
        return SOM_FORTE;
    case EV_CORINGA: return e->alvo_k < N_FILA ? SOM_MOEDA_BATE : SOM_ANULA;   // a ficha gira e pousa depois; sem o que copiar, cai
    case EV_CARROSSEL: return SOM_CARROSSEL;   // a caixinha de musica desacelerando e o sino ao parar
    case EV_CAVALO:  return SOM_ANULA;
    case EV_TIRA: case EV_PIRATA: case EV_CORTA: return SOM_TIRA;
    case EV_ROUBA:   return SOM_ROUBA;
    case EV_AGIOTA:  return SOM_FICHA;
    case EV_MEDUSA:  return SOM_BIGORNA;
    case EV_ADESIVO:                     // a copia brilha com o som do Holografico; o Fragil quebrando, vidro
        if (e->modo & EVM_CARIMBO) return SOM_AD_CARIMBO;   // o carimbo bate; o do adesivo vem depois (evento_no_meio)
        if (e->modo & EVM_PAPAGAIO) return SOM_PAPAGAIO;    // o papagaio grita; idem
        if (e->modo & EVM_RETRATO) return SOM_RETRATO;      // a foto; idem
        if (e->modo & EVM_DUPLA) return SOM_AD_DUPLA;       // o dado vira; idem
        if (e->ad == AD_FRAGIL && !e->valor) return e->bloqueado ? som_adesivo(AD_IMPERMEAVEL) : SOM_VIDRO;
        if (e->ad == AD_LENTICULAR && e->valor < 0) return SOM_TIRA;   // anulado: o desenho triste, e perde
        if (e->ad == AD_DINAMITE && !e->valor) return SOM_AD_DINAMITE_APAGA;   // anulada: o pavio apaga, sem o BUM
        return som_adesivo(e->copia ? AD_HOLOGRAFICO : e->ad);
    case EV_GARIMPO: return SOM_BOLSA;
    case EV_FOGO: case EV_RENASCE: return SOM_FOGO;
    case EV_RESSUSCITA: return SOM_AGNUS;
    case EV_EXTRA:
        if (e->de_k >= N_FILA) return SOM_EFEITO;
        switch (F[e->de_j].tipo[e->de_k]) {
        case D_JINGLE:  return SOM_SINO;
        case D_ESTRELA: return SOM_ESTRELA;
        case D_MEIA:    return e->valor ? SOM_AD_DOURADO : SOM_MOEDA_POUSA;   // paga; ou guarda mais uma
        case D_LEQUE:   return SOM_LEQUE;
        case D_BALAO:   return SOM_BALAO;
        default:        return SOM_EFEITO;
        }
    case EV_BONUS:
        if (e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_FERREIRO) return SOM_BIGORNA;
        if (e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_RENA) return SOM_SINO;  // os guizos do treno
        if (e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_TUDO_NADA) {              // a moeda: tudo ou nada
            for (int k = 0; k < N_FILA; k++) if (e->por_dado[k] < 0) return SOM_TUDO_NADA_PERDE;
            return SOM_TUDO_NADA;
        }
        for (int k = 0; k < N_FILA; k++) if (e->por_dado[k] < 0) return SOM_TIRA;
        return SOM_EFEITO;
    default:         return SOM_EFEITO;
    }
}

// A voz de cada poder, quando o som do evento seria o comum (efeito, anula,
// tira): um som da casa que lembre o que o dado faz e o tom dele, para uma
// cadeia de efeitos nao soar como a mesma nota repetida. som 0: sem voz.
static const struct { int16_t som; int8_t tom; } VOZ_PODER[EF_N] = {
    [EF_CARRASCO] = { SOM_ANULA, -5 },        [EF_INVEJOSO] = { SOM_ANULA, 3 },
    [EF_BERSERKER] = { SOM_ANULA, -2 },       [EF_VIDRO] = { SOM_VIDRO, 0 },
    [EF_AGNUS] = { SOM_AGNUS, 4 },
    [EF_SANGUESSUGA] = { SOM_AD_COLA, -2 },   [EF_EGOISTA] = { SOM_ANULA, -7 },
    [EF_MARTELO] = { SOM_ESPINHOSO, 0 },      [EF_PIRATA] = { SOM_TIRA, -3 },
    [EF_MALDICAO] = { SOM_TIRA, -6 },         [EF_VAMPIRO] = { SOM_VAMPIRO, 0 },
    [EF_OURICO] = { SOM_TIRA, 5 },            [EF_SABOTADOR] = { SOM_SABOTADOR, 0 },
    [EF_FICHAS] = { SOM_FICHA, 0 },           [EF_COBRADOR] = { SOM_FICHA, -2 },
    [EF_POTE] = { SOM_FICHA, 3 },             [EF_FICHA_VELHA] = { SOM_MOEDA_POUSA, 0 },
    [EF_SORTE] = { SOM_MOEDA_BATE, 0 },       [EF_TREVO] = { SOM_AD_ESTRELINHA, 0 },
    [EF_SEMENTE] = { SOM_AD_FERMENTO, 0 },    [EF_DOBRO] = { SOM_AD_BIS, 0 },
    [EF_FARTURA] = { SOM_AD_BIS, 2 },         [EF_VIUVA] = { SOM_AD_BIS, -3 },
    [EF_AZARAO] = { SOM_AD_BIS, 4 },          [EF_ESPERANCA] = { SOM_AD_ESTRELINHA, 3 },
    [EF_SACRIFICIO] = { SOM_AD_BIS, -5 },     [EF_PATINHOS] = { SOM_AD_PATINHO, 0 },
    [EF_PISCA] = { SOM_AD_ELETRONICO, 0 },    [EF_COPIADOR] = { SOM_AD_HOLOGRAFICO, 0 },
    [EF_ESPELHO] = { SOM_AD_REFLEXIVO, 0 },   [EF_TRONO] = { SOM_AD_DOURADO, 0 },
    [EF_DESAFIANTE] = { SOM_AD_DOURADO, 3 },  [EF_CAPITAO] = { SOM_VALIDO2, 0 },
    [EF_CHECKPOINT] = { SOM_VALIDO3, 0 },     [EF_PUXA] = { SOM_VALIDO2, 4 },
    [EF_COVEIRO] = { SOM_COVEIRO, 0 },        [EF_MISERICORDIA] = { SOM_ESTRELA, 0 },
    [EF_MARTIR] = { SOM_ESTRELA, -4 },        [EF_PRISMA] = { SOM_AD_PURPURINA, 0 },
    [EF_SEGURANCA] = { SOM_BATE_DADO, 0 },    [EF_DOMINO] = { SOM_BATE_DADO, 3 },
    [EF_COLECIONADOR] = { SOM_AD_FIGURINHA, 0 }, [EF_AUTOFAGIA] = { SOM_AUTOFAGIA, 0 },
    [EF_ASSOMBRA] = { SOM_AD_TRANSPARENTE, 0 }, [EF_PACTO] = { SOM_AD_TRANSPARENTE, -3 },
    [EF_TRENO] = { SOM_SINO, -4 },            [EF_REVOLVER] = { SOM_TIRO, 0 },
    [EF_AMPULHETA] = { SOM_AMPULHETA, 0 },    [EF_PAR] = { SOM_VALIDO3, 3 },
    [EF_SOLITARIO] = { SOM_EFEITO, -7 },      [EF_HEXA] = { SOM_EFEITO, 5 },
    [EF_RANCOR] = { SOM_EFEITO, -9 },         [EF_MEDIA] = { SOM_EFEITO, -4 },
    [EF_JINGLE] = { SOM_SINO, 0 },            [EF_PANETONE] = { SOM_PRESENTE, 0 },
    [EF_JACKPOT] = { SOM_FICHA, 4 },
    [EF_TERREMOTO] = { SOM_FORTE, -3 },       [EF_FICHA_CORINGA] = { SOM_MOEDA_BATE, 2 },
    [EF_MOSAICO] = { SOM_AD_FIGURINHA, -2 },
    [EF_LEQUE] = { SOM_LEQUE, 0 },            [EF_BALAO] = { SOM_BALAO, 0 },
    [EF_SENTINELA] = { SOM_SENTINELA, 0 },    [EF_AGIOTA] = { SOM_AGIOTA, 0 },
    [EF_OCULTO] = { SOM_OCULTO, 0 },
};

// A cadeia do fim da rodada: cada poder com a sua voz (VOZ_PODER). Os sons
// comuns que sobram (ficha, efeito e tira sem voz propria) sobem um semitom
// a cada efeito seguido, ate +7; os que tem voz propria (Laser, Jackpot,
// Fogo, roubo...) ficam no tom deles.
#define ESCADA_MAX 7
static void toca_evento(int i)
{
    int s = som_do_evento(i);
    const evento_t *e = &ev[i];
    int ef = e->tipo != EV_ADESIVO && e->de_j < 2 && e->de_k < N_FILA ? TIPO[F[e->de_j].tipo[e->de_k]].efeito : 0;
    bool comum = s == SOM_EFEITO || s == SOM_ANULA || s == SOM_TIRA || s == SOM_FICHA;
    if (comum && ef > 0 && ef < EF_N && VOZ_PODER[ef].som) som_toca_tom(VOZ_PODER[ef].som, VOZ_PODER[ef].tom);
    else if (s == SOM_EFEITO || s == SOM_FICHA || s == SOM_TIRA) som_toca_tom(s, i < ESCADA_MAX ? i : ESCADA_MAX);
    else som_toca(s);
    for (int a = 0; a < A_N; a++) if (ev[i].am >> a & 1) am_acende(a);   // o amuleto que agiu acende
    // O combo: do 3o efeito seguido do mesmo lado, um tique por cima, cada vez mais agudo.
    int nc = 0;
    for (int q = 0; q <= i; q++) nc += ev[q].de_j == e->de_j;
    if (nc >= 3) som_toca_tom(SOM_CONTA, nc * 2 > 16 ? 16 : nc * 2);
}

// E acelera de leve: cada efeito corre 5% mais depressa que o anterior (o
// tempo do jogo anda mais rapido, a animacao inteira vai junto), ate 1,3
// vezes. Somado a velocidade da mesa (vel), no maximo 1,4 vezes o normal: na
// mesa rapida o resto anda ligeiro, mas os efeitos ficam legiveis.
#define RITMO_PASSO 0.95f
#define RITMO_MAX   1.3f
#define RITMO_TETO  1.4f
static float ritmo_efeitos(float vel)
{
    float r = powf(1 / RITMO_PASSO, (float)ev_i), teto = RITMO_TETO / vel;
    if (r > RITMO_MAX) r = RITMO_MAX;
    return r < teto ? r : teto;
}

// A causa decisiva do fim da rodada: o efeito cujo dado, se nao tivesse
// agido, viraria a rodada (ou empataria) - entre esses, o que mais mudaria.
// -1: nenhum, sozinho, decide. So os efeitos dos dados (nao os de amuleto ou
// trunfo), e so os do fim da rodada.
static int ev_decisivo = -1;
static int leva_rodada(const desfecho_t *d);
static int acha_decisivo(void)
{
    int w = leva_rodada(&resolvido);
    if (zerou >= 0 || !n_ev || w > 1) return -1;
    static fila_t salvo[2];
    memcpy(salvo, F, sizeof F);
    int melhor = -1, menor = 0;
    for (int i = 0; i < n_ev; i++) {
        const evento_t *e = &ev[i];
        if (e->de_k >= N_FILA || e->de_j > 1) continue;
        bool visto = false;                      // o mesmo dado, uma vez so
        for (int q = 0; q < i; q++) visto |= ev[q].de_j == e->de_j && ev[q].de_k == e->de_k;
        if (visto) continue;
        calado_j = (int8_t)e->de_j; calado_k = (int8_t)e->de_k;
        desfecho_t d;
        calcula_desfecho(&d, false);
        calado_j = calado_k = -1;
        memcpy(F, salvo, sizeof F);
        int margem = d.total[w] - d.total[outro(w)];
        if (leva_rodada(&d) != w && (melhor < 0 || margem < menor)) { melhor = i; menor = margem; }
    }
    return melhor;
}

static int cursor_aposta(void);           // aposta.inc: onde o cursor comeca (nunca no correr)
static void desfecho(void)
{
    calcula_desfecho(&resolvido, true);
    ev_decisivo = acha_decisivo();
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
        memset(vis_golpe_cor[j], 0, sizeof vis_golpe_cor[j]);
        vis_selo[j][0] = vis_selo[j][1] = vis_fragil[j] = 0;
        memset(vis_tsunami[j], 0, N_FILA);
        memset(vis_espelho[j], 0, N_FILA);
        memset(vis_oculto[j], 0, N_FILA);
        memset(vis_curinga[j], 0, N_FILA);
        memset(vis_carrossel[j], 0, N_FILA);
        memset(vis_tremor[j], 0, N_FILA);
    }
    // O que se refez no comeco do desfecho (o Espelho, o Oculto, o tremor, a
    // Ficha Coringa, o Carrossel) aparece na mesa so quando o evento de cada
    // um chega.
    for (int i = 0; i < n_ev; i++) {
        const evento_t *e = &ev[i];
        int q = e->tipo == EV_TREMOR ? e->alvo_j : e->de_j;
        if (e->tipo == EV_ESPELHO) vis_espelho[e->de_j][e->de_k] = 1;
        else if (e->tipo == EV_REVELA) vis_oculto[e->de_j][e->de_k] = 1;
        else if (e->tipo == EV_CORINGA) vis_curinga[e->de_j][e->de_k] = 1;
        else if (e->tipo == EV_CARROSSEL) vis_carrossel[e->de_j][e->de_k] = 1;
        else if (e->tipo == EV_TREMOR && e->alvo_k < N_FILA && !vis_tremor[q][e->alvo_k]) {
            vis_tremor[q][e->alvo_k] = 1;                 // o primeiro tremor no dado: o que ele tinha
            vis_tremor_de[q][e->alvo_k] = e->antes;
        } else continue;
        memcpy(vis_est[q], est_pre_espelho[q], N_FILA);
    }
    n_voo = 0;
    ev_i = 0;
    t_fase = 0;
    t_respiro = -1;
    for (int j = 0; j < 2; j++) log_base[j] = total_visivel(j);
    if (n_ev) { fase = F_EFEITOS; toca_evento(0); reage_evento(&ev[0]); }
    else conclui_rodada();
}

// O evento da vez acabou e ja se aplicou: entra o seguinte (ou a rodada
// fecha). Depois de um golpe, so quando a mesa solta (passo.inc).
static void proximo_evento(void)
{
    t_respiro = -1;
    ev_i++;
    t_fase = 0;
    if (ev_i >= n_ev) conclui_rodada();
    else { toca_evento(ev_i); reage_evento(&ev[ev_i]); }
}

// Os tempos de dentro do tremor e da Ficha Coringa (fracoes do evento): a
// falha corre, o dado salta e pousa; a ficha sobe, vira e cai. E o visor do
// Digital, que acende (segundos) quando ele entra na sequencia.
#define TREMOR_RACHA  0.06f
#define TREMOR_SALTA  0.40f
#define TREMOR_POUSA  0.80f
#define CASCA_PISA    0.16f                  // a Casca: o dado pisa nela (e desliza ate o salto)
#define CASCA_DESLIZA 14                     // os pixels do deslize
#define CASCA_VOA     0.30f                  // no lance: os segundos da casca voando ate o dado rolando
#define CASCA_PE      (R_DADO - 2)           // o pe da casca, abaixo do meio do dado
// A casca do oponente chega no dado rolando: ele pisa nela e escorrega (da
// uma guinada de lado, girando mais), e ela sai deslizando para tras dele.
static void casca_pisa(voo_t *v)
{
    if (!v->casca || v->casca_caiu) return;   // (a bolha e o Post-it: so marca onde ela bateu)
    v->casca_caiu = true;
    som_toca(som_adesivo(v->casca == 2 ? AD_IMPERMEAVEL : v->casca == 3 ? AD_POSTIT : AD_CASCA));
    v->casca_x = v->x;
    v->casca_y = v->y + CASCA_PE;
    float vel = sqrtf(v->vx * v->vx + v->vy * v->vy);
    v->casca_vx = vel > 1 ? -v->vx / vel : 0;
    v->casca_vy = vel > 1 ? -v->vy / vel : 1;
    if (v->casca != 1 || v->parado) return;
    float a = v->vx >= 0 ? 0.6f : -0.6f, c = cosf(a), s = sinf(a);
    float vx = v->vx * c - v->vy * s, vy = v->vx * s + v->vy * c;
    v->vx = vx * 1.1f;
    v->vy = vy * 1.1f;
    v->spin += a > 0 ? 9 : -9;
}
#define CORINGA_SOBE  0.30f
#define CORINGA_POUSA 0.74f
#define CORINGA_VIRA  ((CORINGA_SOBE + CORINGA_POUSA) / 2)
#define DIGITAL_ACENDE 0.55f
// O Carimbo e a Dupla Face vem antes do adesivo agir de novo: o carimbo
// descendo no dado (e a tinta que fica), ou o dado virando para mostrar o lado
// de baixo. Depois, o gesto do adesivo, como sempre.
#define CARIMBO_T 0.78f
#define DUPLA_T   0.72f
// O Papagaio grita e o balao mostra o adesivo que ele repete; o Porta-Retrato
// tira a foto e ela voa ate o dado de antes. Depois, o adesivo age.
#define PAPAGAIO_T 0.80f
#define RETRATO_T  0.80f
static float ev_intro(const evento_t *e)
{
    if (e->tipo != EV_ADESIVO) return 0;
    return e->modo & EVM_CARIMBO ? CARIMBO_T : e->modo & EVM_DUPLA ? DUPLA_T
         : e->modo & EVM_PAPAGAIO ? PAPAGAIO_T : e->modo & EVM_RETRATO ? RETRATO_T : 0;
}
static float duracao_de(const evento_t *e)
{
    switch (e->tipo) {
    case EV_ROUBA: return 1.6f;
    case EV_MULT:  return 1.7f;
    case EV_FOGO:  return 1.6f;
    case EV_SEMENTE: return 1.6f;
    case EV_EXTRA:   return e->mascara ? 1.9f : 1.6f;    // o Capitao passa por cada dado
    case EV_CHEFE:   return 1.6f;
    case EV_CAVALO:  return 2.0f;
    case EV_CORTA:   return 1.5f;
    case EV_AGIOTA:  return 1.6f;
    case EV_MEDUSA:  return 1.9f;
    case EV_ONDA:    return 2.1f;
    case EV_GARIMPO: return 1.7f;
    case EV_REVELA:  return 1.4f;
    case EV_CORINGA: return 1.9f;
    case EV_CARROSSEL: return CARROSSEL_PARA + 0.9f;   // gira devagar ate parar, e o numero assenta
    case EV_TREMOR:  return e->ad == AD_CASCA ? 2.0f : 2.2f;
    case EV_VAMPIRO: return 1.6f;
    case EV_RENASCE: return 1.5f;
    case EV_RESSUSCITA: return AGNUS_LUZ_0 + AGNUS_LUZ_PASSO * (e->valor > 1 ? e->valor - 1 : 0) + 1.3f;   // a luz desce em cada um
    case EV_AVISO:   return 1.5f;
    case EV_JACKPOT: return 2.4f;
    case EV_BONUS: return 1.4f;
    case EV_ADESIVO:                                // o germe e o barro viajam; a massinha voa, amassa e vira remendo
        return e->ad == AD_BARRO ? 2.0f : e->ad == AD_MASSINHA ? 1.9f : e->ad == AD_DINAMITE && e->valor ? 1.8f : 1.5f;
    default:       return 1.3f;
    }
}
static float duracao_base(int i) { return duracao_de(&ev[i]); }
static float duracao_evento(int i) { return duracao_base(i) + ev_intro(&ev[i]); }

// Onde sai o numero flutuante de um dado (k < N_FILA) ou do placar de j: da
// beira do lado do meio da mesa.
static int flut_x(int k) { return k < N_FILA ? SLOT_X(k) : PLACAR_X; }
static int flut_d(int j) { return j == baixo ? -1 : 1; }
static int flut_y(int j, int k)
{
    int r = k < N_FILA ? R_DADO + 3 : 30;
    return j == baixo ? zona(j) - r : zona(j) + r;
}
static float flut_extra;                    // o evento com o Carimbo ou a Dupla Face antes: o numero espera
static void flutua_em(int j, int k, uint16_t cor, float atraso, const char *fmt, int v)
{
    flutua(flut_x(k), flut_y(j, k), flut_d(j), cor, atraso + flut_extra, fmt, v);
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
static void flutua_evento_(const evento_t *e);
static void flutua_evento(const evento_t *e) { flut_extra = ev_intro(e); flutua_evento_(e); flut_extra = 0; }
static void flutua_evento_(const evento_t *e)
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
    case EV_ANULA:   if (!(e->bloqueado >> ak & 1)) flutua_em(aj, ak, C_VINHO_CLR, 0, "-%d", e->valor); break;
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
    case EV_RESSUSCITA:                 // o que cada um que se levantou pontua, um atras do outro
        for (int k = 0, i = 0; k < N_FILA; k++)
            if (e->mascara >> k & 1) {
                if (e->por_dado[k]) flutua_em(dj, k, C_OURO, i * 0.08f, "+%d", e->por_dado[k]);
                i++;
            }
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
    case EV_TREMOR:                     // a Casca: o quanto o dado perdeu escorregando (protegido: aplica_evento)
        if (e->ad == AD_CASCA && !e->bloqueado && ak < N_FILA)
            flutua_em(aj, ak, e->antes > e->valor ? C_VINHO_CLR : C_LATAO, 0, e->antes > e->valor ? "-%d" : "fica", e->antes - e->valor);
        break;
    case EV_ADESIVO:
        switch (e->ad) {
        case AD_ELETRONICO: flutua_em(dj, dk, C_OURO, 0, "x%d", e->por_dado[dk < N_FILA ? dk : 0]); break;
        case AD_LOBO: case AD_COLEIRA: flutua_em(aj, N_FILA, C_VINHO_CLR, 0, "-%d", e->valor); break;
        case AD_MIRA:       if (!e->bloqueado) flutua_em(aj, ak, C_VINHO_CLR, 0, "-%d", e->valor); break;
        case AD_GORJETA:    flutua_em(dj, dk, C_OURO, 0, "+%d fichas", e->valor); break;
        case AD_DOURADO:    flutua_em(dj, dk, C_OURO, 0, "+%d moeda", e->valor); break;
        case AD_BIS:        flutua_em(dj, dk, C_OURO, 0, "+%d dados", e->valor); break;
        case AD_SELO:       flutua_em(dj, dk, e->valor ? C_OURO : C_VINHO_CLR, 0, e->valor ? "+%d" : "lacrado", e->valor); break;
        case AD_RATOEIRA:                           // o queijo (no dado dela) ou a armadilha (no maior dele)
            if (aj == dj) flutua_em(dj, dk, C_OURO, 0, "+%d", e->valor);
            else if (!e->bloqueado) flutua_em(aj, ak, C_VINHO_CLR, 0, "-%d", e->valor);
            break;
        case AD_VELCRO:     flutua_em(dj, dk, C_OURO, 0, "gruda", 0); break;
        case AD_MASSINHA:   flutua_em(dj, dk, RGB(130, 204, 112), 1.05f, "+2 no %d", e->valor); break;   // quando vira o Remendo
        case AD_PATINHO:    flutua_em(dj, dk, e->valor < 0 ? C_VINHO_CLR : C_OURO, 0, e->valor < 0 ? "%d" : "+%d", e->valor); break;
        case AD_BILHETE:                        // o bonus de cada dado e o de fora, em dobro
            for (int k = 0; k < N_FILA; k++)
                if (e->mascara >> k & 1) flutua_em(dj, k, C_OURO, k * 0.08f, "+%d", e->por_dado[k]);
            if (e->valor) flutua_em(dj, N_FILA, C_OURO, 0.1f, "+%d", e->valor);
            break;
        case AD_FRAGIL:
            if (!e->bloqueado) flutua_em(dj, dk, e->valor ? C_OURO : C_VINHO_CLR, 0, e->valor ? "+%d" : "quebra", e->valor);
            break;
        case AD_LENTICULAR:
            if (e->valor > 0) flutua_em(dj, dk, C_OURO, 0, "x%d", e->por_dado[dk < N_FILA ? dk : 0]);
            else flutua_em(dj, N_FILA, C_VINHO_CLR, 0, "%d", e->valor);
            break;
        case AD_IMPERMEAVEL: flutua_em(dj, dk, C_VENTO, 0, "protegido", 0); break;
        case AD_BARRO:      break;                  // o rotulo vai junto do dado que ele sujou (mesa.inc)
        case AD_INFECTADO:  if (e->valor) flutua_em(dj, dk, C_OURO, 0, "+%d", e->valor); break;   // anulado, so espalha
        case AD_SUJEIRA:    flutua_em(dj, dk, C_VINHO_CLR, 0, "%d", e->valor); break;
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

// O dado (j, k) caiu num ataque: racha, na cor de quem atacou.
static void golpe_marca(int j, int k, uint16_t cor)
{
    if (k < 0 || k >= N_FILA) return;
    vis_golpe_cor[j][k] = cor ? cor : C_LATAO;
    vis_golpe_t[j][k] = t_relogio;
}

// O evento i terminou: a mesa passa a mostrar o efeito dele.
static void aplica_evento(int i)
{
    evento_t *e = &ev[i];
    flutua_evento(e);
    // O Impermeavel segurou o efeito nos dados com a capa: a bolha acende
    // neles, "protegido", e o efeito nao pega.
    for (int k = 0; k < N_FILA && e->alvo_j < 2; k++) {
        if (!(e->bloqueado >> k & 1)) continue;
        flutua_em(e->alvo_j, k, C_VENTO, k * 0.05f, "protegido", 0);
        vis_capa_t[e->alvo_j][k] = t_relogio;
    }
    if (e->bloqueado && !(e->tipo == EV_ADESIVO && e->ad == AD_FRAGIL)) som_toca(som_adesivo(AD_IMPERMEAVEL));
    switch (e->tipo) {
    case EV_ANULA:
        if (e->bloqueado >> e->alvo_k & 1) break;
        vis_est[e->alvo_j][e->alvo_k] = V_ANULADO;
        golpe_marca(e->alvo_j, e->alvo_k, aro_de(F[e->de_j].tipo[e->de_k]));
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
    case EV_CORINGA:                    // a Ficha Coringa virou: idem
    case EV_CARROSSEL:                  // o Carrossel parou: idem
        vis_espelho[e->de_j][e->de_k] = 0;
        vis_oculto[e->de_j][e->de_k] = 0;
        vis_curinga[e->de_j][e->de_k] = 0;
        vis_carrossel[e->de_j][e->de_k] = 0;
        memcpy(vis_est[e->de_j], e->tem_seq ? e->seq : F[e->de_j].est, N_FILA);   // como ficou nessa hora, nao no fim de tudo
        break;
    case EV_TREMOR:                     // o dado sacudido pousou: a sequencia do oponente refeita
        if (e->alvo_k < N_FILA) vis_tremor[e->alvo_j][e->alvo_k] = 0;
        memcpy(vis_est[e->alvo_j], e->tem_seq ? e->seq : F[e->alvo_j].est, N_FILA);
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
    case EV_FOGO:
        if (!(e->bloqueado >> e->alvo_k & 1)) vis_queima[e->alvo_j][e->alvo_k] = 1;
        vis_semente[e->de_j] += e->valor;
        break;
    case EV_SEMENTE: case EV_JACKPOT: case EV_EXTRA: case EV_CHEFE:
        vis_semente[e->de_j] += e->valor;
        break;
    case EV_RENASCE: vis_est[e->de_j][e->de_k] = V_VALIDO; break;
    case EV_RESSUSCITA:                 // (a mesa os levanta um a um, com a luz: agnus_levantou)
        for (int k = 0; k < N_FILA; k++) if (e->mascara >> k & 1) vis_est[e->de_j][k] = V_VALIDO;
        break;
    case EV_CORTA:   vis_bonus[e->alvo_j][e->alvo_k] -= (int8_t)e->valor; vis_corte[e->alvo_j][e->alvo_k] = 1; break;
    case EV_MEDUSA:
        for (int k = 0; k < N_FILA; k++) if (e->mascara >> k & 1) vis_pedra[e->de_j][k] = 1;
        break;
    case EV_CAVALO:                     // tudo o que era de quem atingiu o Cavalo cai
        for (int k = 0; k < N_FILA; k++)
            if (e->mascara >> k & 1) {
                vis_est[e->alvo_j][k] = V_ANULADO;
                golpe_marca(e->alvo_j, k, aro_de(F[e->de_j].tipo[e->de_k]));
            }
        for (int r = 0; r < vis_nroubo[e->alvo_j]; r++) vis_roubo[e->alvo_j][r].anulado = 1;
        break;
    case EV_ADESIVO:                    // o adesivo agiu: pontos, x2, o ataque ao rival
        switch (e->ad) {
        case AD_CONFETE: case AD_RASPADINHA: case AD_FERMENTO: case AD_ESTRELINHA: case AD_PURPURINA: case AD_FIGURINHA:
        case AD_ETIQUETA: case AD_PATINHO: case AD_NEON: case AD_INFECTADO:
        case AD_CORRENTE: case AD_DINAMITE:     // (a Dinamite anulada so apaga: valor 0)
            vis_semente[e->de_j] += e->valor;
            break;
        case AD_SUJEIRA: vis_delta[e->de_j] += e->valor; break;   // o barro do oponente: -2 no seu placar
        case AD_LENTICULAR:             // valendo, o x2; anulado, os pontos que perde
            if (e->valor > 0) { for (int k = 0; k < N_FILA; k++) if (e->mascara >> k & 1) vis_mult[e->de_j][k] = e->por_dado[k]; }
            else vis_delta[e->de_j] += e->valor;
            break;
        case AD_FRAGIL:                 // valendo paga; anulado, racha (e quebra no fim; com a capa, nao)
            vis_semente[e->de_j] += e->valor;
            if (!e->valor && !e->bloqueado && e->de_k < N_FILA) vis_fragil[e->de_j] |= (uint8_t)(1 << e->de_k);
            break;
        case AD_SELO:                   // o lacre rompeu (pagando) ou fechou
            vis_semente[e->de_j] += e->valor;
            if (e->de_k < N_FILA) vis_selo[e->de_j][e->valor ? 0 : 1] |= (uint8_t)(1 << e->de_k);
            break;
        case AD_BILHETE:                // o bonus dos dados (por dado) e o de fora (valor)
            for (int k = 0; k < N_FILA; k++) if (e->mascara >> k & 1) vis_bonus[e->de_j][k] += e->por_dado[k];
            vis_semente[e->de_j] += e->valor;
            break;
        case AD_ELETRONICO: case AD_IMANTADO:
            for (int k = 0; k < N_FILA; k++) if (e->mascara >> k & 1) vis_mult[e->de_j][k] = e->por_dado[k];
            break;
        case AD_LOBO: case AD_COLEIRA: vis_delta[e->alvo_j] -= e->valor; break;   // a Coleira: no placar de quem tentou roubar
        case AD_TIQUETE: vis_semente[e->de_j] += e->valor; break;
        case AD_MIRA: case AD_RATOEIRA:
            if (e->ad == AD_RATOEIRA && e->alvo_j == e->de_j) { vis_semente[e->de_j] += e->valor; break; }   // o queijo: +4
            if (e->bloqueado) break;    // bateu na capa
            vis_est[e->alvo_j][e->alvo_k] = V_ANULADO;
            golpe_marca(e->alvo_j, e->alvo_k, ADESIVO[e->ad].fundo == AD_SEM_FUNDO ? ADESIVO[e->ad].tinta : ADESIVO[e->ad].fundo);
            break;
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
    t_anuncio = anuncio_dur = 1.3f;
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
    t_anuncio = anuncio_dur = n ? 2.2f + 0.4f * n : 1.3f;
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
        f->idx[k] = 0; f->tipo[k] = 0; f->fixo[k] = 0; tira_tag(f, k);
    }
    if (f->n > f->lancados) f->n = f->lancados;
    if (f->lancados <= 3) f->carta = false;      // parou antes da carta do Coringa
    t_devolve = n_devolvidos ? 0 : -1;
}

// Quem abriu a rodada terminou com 0, e nada mais muda isso: sem Espelho
// esperando o outro, sem Oculto escondido, sem Ficha Coringa por virar, sem
// Carrossel girando, sem a carta do Coringa, e 0 tambem depois dos efeitos do fim. Qualquer dado do
// outro ja venceria: a rodada acaba aqui.
static bool zerou_de_vez(int j)
{
    if (carta_posta(j)) return false;
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        if (F[j].espera[k] || F[j].escondido[k] || F[j].curinga[k] || F[j].gira[k]) return false;
    desfecho_t d;
    calcula_desfecho(&d, false);
    return d.bruto[j] + lapis_pena(j) <= 0;      // os dados fizeram 0 (o -4 do Estojo de Lapis nao conta)
}

// Quem joga em segundo ja passou do alvo: com o que esta na mesa, parar agora
// vence (a previa do fim da rodada, a mesma do botao "parar"). So quando
// nada ainda vai mudar sozinho: sem Oculto escondido, sem a carta do Coringa
// por virar e sem Terremoto por sacudir. Jogar mais so poderia perder: a vez
// acaba ali.
static bool ja_vence(int j)
{
    if (j == primeiro || !F[j].lancados || fim_incerto()) return false;
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
    marca_online();
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
    if (vez == primeiro) {
        a_pagar = 0; passes = 0; aumentou = false;
        cursor = cursor_aposta();
        fase = F_APOSTA;
    } else {
        desfecho();
    }
}
