// ---------------------------------------------------------------------------
// Salvamento: a estrutura da run em hexadecimal
// ---------------------------------------------------------------------------
// A IA da mesa: do Nivel 5, a mesa dificil do 3o salao pensa mais; do 9, os
// chefes do 2o e do 3o (o primeiro salao fica sempre o mais simples).
static int nivel_mesa(const no_t *no)
{
    bool pensa = (run.dificuldade >= 5 && no->tipo == NO_DIFICIL && run.andar >= 2) || (run.dificuldade >= 9 && no->tipo == NO_CHEFE && run.andar >= 1);
    return no->nivel + (pensa && no->nivel < 2 ? 1 : 0);
}

// ---------------------------------------------------------------------------
// Moedas que contam
// ---------------------------------------------------------------------------
// A faixa mostra as moedas contando ate o valor de verdade (comprou, desce;
// ganhou, sobe). Run nova ou carregada: o numero ja nasce certo.
static contador_t moedas_cont;
static bool moedas_ok;
static int moedas_vis(void) { return moedas_ok ? moedas_cont.mostra : run.moedas; }
// A bolsa da faixa: o dado comprado so conta quando chega nela (voando, ainda
// nao), e ela pula ao receber.
static int   bolsa_voando;
static float t_bolsa_pulo;
static int   faixa_bolsa_x = GFX_W - 120, faixa_bolsa_y = FAIXA_H / 2;
static void loja_passo(float dt);
static float t_mapa_anda;                    // > 0: a ficha do jogador anda ate a mesa da vez

// O saque, no fim de uma partida vencida: as moedas entram uma a uma na
// fileira (a da mesa, depois as do Cofre e as da Taca, cada grupo acendendo o
// seu amuleto), com a moeda batendo cada vez mais aguda; a ultima deita.
#define SAQUE_ESPERA 0.55f
#define SAQUE_FIM    0.35f
static int   saque_n;                       // moedas ja na fileira
static float t_saque;
static bool  saque_pronto;
static int saque_total(void) { return run.ganho + run.colar + run.juros + run.taca + run.dourado; }
// O intervalo entre uma moeda e a outra: as poucas sem pressa; as 20 do
// Barao, em pouco mais de um segundo e meio.
static float saque_passo(void) { int n = saque_total(); return n <= 6 ? 0.16f : 1.7f / n; }

static void nov_passo(void);             // as novidades (novidades.inc)
static void nov_monta_run(void);
static void nov_abre_chefe(void);
static bool nov_tem_chefe(void);

static void des_contadores_passo(float dt, float dt_real)
{
    if (modo != M_DESAFIO) { moedas_ok = false; return; }
    if (!moedas_ok) { contador_fixa(&moedas_cont, run.moedas); moedas_ok = true; }
    contador_passo(&moedas_cont, run.moedas, dt_real, dt_real, 6, 12);
    if (t_bolsa_pulo > 0) t_bolsa_pulo -= dt_real / 0.35f;
    if (t_mapa_anda > 0 && fase == F_MAPA && (t_mapa_anda -= dt_real) <= 0) som_toca(SOM_FICHA);   // a ficha assenta
    loja_passo(dt_real);
    nov_passo();

    if (fase != F_FIM || venc_partida != 0) {
        saque_n = 0;
        t_saque = 0;
        saque_pronto = false;
        return;
    }
    t_saque += dt;
    if (saque_pronto) return;
    if (saque_n == 0 && t_saque < SAQUE_ESPERA) return;     // o letreiro entra; depois, as moedas
    if (saque_n < saque_total()) {
        if (saque_n && t_saque < saque_passo()) return;
        t_saque = 0;
        if (saque_n == run.ganho && run.colar) am_acende(A_COLAR);
        if (saque_n == run.ganho + run.colar && run.juros) am_acende(A_COFRE);
        if (saque_n == run.ganho + run.colar + run.juros && run.taca) am_acende(A_TACA);
        saque_n++;
        som_toca_tom(SOM_MOEDA_BATE, saque_n / 2 < 9 ? saque_n / 2 : 9);
        return;
    }
    if (t_saque >= SAQUE_FIM) {                 // contou: uma pausa e a ultima deita
        saque_pronto = true;
        t_saque = 0;
        som_toca(SOM_MOEDA_POUSA);
    }
}

static void des_salva(void)
{
    static char buf[2 * sizeof(run_t) + 1];
    hex_de(&run, sizeof run, buf);
    salva_grava(chave_run(), buf);
}

// A run de antes dos adesivos de um byte (run13_t) na de agora, campo a campo.
static void run_de_13(run_t *r, const run13_t *v)
{
    memcpy(r->marca, v->marca, 4);
    r->andar = v->andar; r->passo = v->passo; r->estado = v->estado; r->vitorias = v->vitorias;
    r->moedas = v->moedas; r->semente = v->semente; r->n_col = v->n_col; r->dificuldade = v->dificuldade;
    for (int i = 0; i < MAX_COL; i++) r->col[i] = peca_de_13(&v->col[i]);
    memcpy(r->no, v->no, sizeof r->no);
    r->amuletos = v->amuletos; r->amuletos_hi = v->amuletos_hi;
    memcpy(r->am_oferta, v->am_oferta, sizeof r->am_oferta);
    r->am_volta = v->am_volta; r->am_selo = v->am_selo; r->am_novo = v->am_novo;
    r->ganho = v->ganho; r->juros = v->juros; r->taca = v->taca;
    for (int i = 0; i < VITRINE_MAX; i++) r->vitrine[i] = peca_de_13(&v->vitrine[i]);
    memcpy(r->levou, v->levou, sizeof r->levou);
    r->trocas = v->trocas; r->remocoes = v->remocoes; r->lojas = v->lojas;
    r->pacote = v->pacote; r->pac_estado = v->pac_estado;
    for (int i = 0; i < PAC_OPCOES; i++) r->pac_op[i] = peca_de_13(&v->pac_op[i]);
    r->adesivo = v->adesivo; r->ad_vendido = v->ad_vendido; r->dourado = v->dourado;
    r->amuletos_x = v->amuletos_x; r->colar = v->colar; r->presente = v->presente;
}

// A run salva so abre se for desta estrutura (RUN_MARCA): as de antes ficam
// para tras. A de antes dos adesivos de um byte abre convertida, e a de antes
// do Presente (sem o ultimo byte) abre com ele em 0.
static bool des_carrega(run_t *r)
{
    static char buf[2 * sizeof(run_t) + 8];
    static run13_t v;
    int n = salva_le(chave_run(), buf, (int)sizeof buf);
    memset(r, 0, sizeof *r);
    memset(&v, 0, sizeof v);
    if (n == (int)(2 * sizeof v) || n == (int)(2 * offsetof(run13_t, presente))) {
        if (!de_hex(buf, &v, (size_t)n / 2)) return false;
        run_de_13(r, &v);
    } else {
        size_t tam = n == (int)(2 * offsetof(run_t, presente)) ? offsetof(run_t, presente) : sizeof *r;
        if (n != (int)(2 * tam) || !de_hex(buf, r, tam)) return false;
    }
    if (memcmp(r->marca, RUN_MARCA, 4) || r->andar > DES_ANDAR_FINAL
        || (r->andar == DES_ANDAR_FINAL && r->dificuldade < CORINGA_DESDE)
        || r->passo >= DES_PASSOS || r->no[r->passo].tipo == NO_VAZIO
        || r->estado > R_AMULETO || r->n_col < 1 || r->n_col > MAX_COL) return false;
    for (int i = 0; i < r->n_col; i++) if (r->col[i].tipo >= D_N) return false;
    for (int i = 0; i < PAC_OPCOES; i++) if (r->pac_op[i].tipo >= D_N || r->pac_op[i].ad[0] >= AD_N) return false;
    if ((r->pacote > GR_ADESIVOS && r->pacote != GR_NENHUM) || r->pac_estado > PAC_USADO) return false;
    if (r->adesivo >= AD_N) return false;
    if (r->dificuldade > N_NIVEIS) return false;
    if (r->am_novo > A_N || r->estado != R_AMULETO) r->am_novo = 0;   // so a troca usa
    r->amuletos_x &= (uint8_t)((1u << (A_N - 32)) - 1);
    if (r->colar > 1) r->colar = 0;
    r->presente = 0;                             // sem uso (todo Presente fica agora)
    return true;
}

static bool des_tem_run(void)
{
    run_t r;
    return des_carrega(&r);
}

// "Salão Vermelho · mesa 2 de 4 · 14 dados · 7 moedas"
static void des_resumo(char *s, int n)
{
    run_t r;
    if (!des_carrega(&r)) { s[0] = 0; return; }
    if (r.estado == R_AMULETO && r.vitorias == 0) snprintf(s, (size_t)n, "escolhendo o primeiro amuleto");
    else if (r.estado == R_LOJA_INI) snprintf(s, (size_t)n, "na loja da entrada · %d moedas", r.moedas);
    else if (r.estado == R_LOJA) snprintf(s, (size_t)n, "%s · na loja · %d dados · %d moedas",
                                          SALAO[r.andar], r.n_col, r.moedas);
    else snprintf(s, (size_t)n, "%s · mesa %d de %d · %d dados · %d moedas",
                  SALAO[r.andar], r.passo + 1, r.andar == DES_ANDAR_FINAL ? 1 : DES_PASSOS, r.n_col, r.moedas);
}

// ---------------------------------------------------------------------------
// Andamento da run
// ---------------------------------------------------------------------------
static void abre_mapa(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    run.estado = R_MAPA;
    des_cur = run.passo;
    t_fase = 0;
    fase = F_MAPA;
}

static void loja_abriu(void);
static void abre_loja_run(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    cursor = 0;
    t_fase = 0;
    fase = F_RLOJA;
    loja_abriu();
}

static void encerra_run(bool venceu)
{
    des_venceu_run = venceu;
    des_liberou = -1;
    if (!venceu) des_chefe_novo = false;
    if (venceu && perfil_atual >= 0) {          // venceu a run: a proxima dificuldade abre
        perfil_t *p = &perfis[perfil_atual];
        int d = run.dificuldade;
        if (p->venceu[d] < 255) p->venceu[d]++;
        if (d == p->nivel && p->nivel < N_NIVEIS) { p->nivel++; des_liberou = p->nivel; }
        perfis_salva();
        hist_run_venceu();
    }
    if (venceu) nov_monta_run();                 // o que abriu, para mostrar card a card
    salva_apaga(chave_run());
    t_fase = 0;
    fase = F_RFIM;
}

// A mesa vencida fecha a run? O Barao, antes do Nivel 5; e o Coringa.
static bool fim_da_run(void)
{
    if (run.andar == DES_ANDAR_FINAL) return true;
    return run.andar == DES_ANDARES - 1 && run.passo == DES_PASSOS - 1 && !com_coringa();
}

// Loja deixada: a proxima mesa do salao; depois do chefe, o salao de cima
// (e, depois do Barao, do Nivel 5 em diante, o do Coringa).
static void avanca_passo(void)
{
    run.passo++;
    if (run.passo >= mesas_do_salao()) {
        run.andar++;
        run.passo = 0;
        if (run.andar == DES_ANDAR_FINAL && com_coringa()) gera_salao_final();
        else if (run.andar >= DES_ANDARES) { run.andar = DES_ANDARES - 1; encerra_run(true); return; }
        else gera_andar();
    }
    run.estado = R_MAPA;
    des_salva();
    abre_mapa();
    t_mapa_anda = run.passo ? 0.7f : 0;          // o marcador sai da mesa vencida
}

// De onde veio o amuleto da tela de escolha (run.am_volta), e para onde ela segue.
enum { AV_INICIO,                                // o primeiro: depois, a loja da entrada
       AV_CHEFE,                                 // o do chefe: depois (e do Selo), a loja
       AV_SELO };                                // o segundo do chefe, do Selo de Divida

static void abre_amuleto(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    cursor = 0;
    if (!run.am_novo) while (cursor < 2 && run.am_oferta[cursor] >= A_N) cursor++;
    t_fase = 0;
    fase = F_RAMULETO;
}

// Os chefes que deixam amuleto: o do primeiro e o do segundo salao; o Barao,
// quando o Coringa vem depois (antes do Nivel 5 a run acaba nele).
static bool chefe_deixa_amuleto(void)
{
    return run.no[run.passo].tipo == NO_CHEFE
        && (run.andar < DES_ANDARES - 1 || (run.andar == DES_ANDARES - 1 && com_coringa()));
}

// A loja depois de uma mesa vencida.
static void abre_loja_pos(void)
{
    run.estado = R_LOJA;
    monta_loja_run();
    des_salva();
    abre_loja_run();
}

static void amuleto_segue(void);

// O Selo de Divida cobra: o chefe deixa mais um, sorteado entre os comuns e
// os incomuns (nao se escolhe).
static void selo_segundo(void)
{
    run.am_selo = 1;
    run.am_volta = AV_SELO;
    rs = run.semente ^ (0x5E10D1u * (uint32_t)(run.andar * 8 + run.passo + 1));
    rnd_r();
    uint8_t a;
    sorteia_amuletos(&a, 1, AMR(AMR_COMUM) | AMR(AMR_INCOMUM));
    if (a >= A_N) { amuleto_segue(); return; }
    run.am_oferta[0] = run.am_oferta[2] = 0xFF;
    run.am_oferta[1] = a;
    run.estado = R_AMULETO;
    des_salva();
    abre_amuleto();
}

// Depois do amuleto (levado, trocado ou deixado de lado): segue de onde veio.
static void amuleto_segue(void)
{
    run.am_novo = 0;
    switch (run.am_volta) {
    case AV_INICIO:
        run.estado = R_LOJA_INI;
        monta_loja_run();
        des_salva();
        abre_loja_run();
        break;
    case AV_CHEFE:
        if ((run_amuletos() >> A_SELO & 1) && !run.am_selo) { selo_segundo(); break; }
        run.am_selo = 0;
        abre_loja_pos();
        break;
    default:                                     // o do Selo
        run.am_selo = 0;
        abre_loja_pos();
        break;
    }
}

// O amuleto a chega. Com lugar, entra e a run segue; com os 5 cheios, espera
// na tela da troca (sai um dos seus, ou ele fica de fora).
static void ganha_amuleto(int a, int volta)
{
    run.am_volta = (uint8_t)volta;
    if (!am_lugar()) {
        run.am_novo = (uint8_t)(a + 1);
        run.estado = R_AMULETO;
        des_salva();
        abre_amuleto();
        return;
    }
    run_poe_amuleto(a);
    som_toca(SOM_PREMIO);
    solta_confete(60);
    amuleto_segue();
}

// Na troca: sai o amuleto tirar (ou nenhum, -1: o novo fica de fora).
static void amuleto_troca(int tirar)
{
    int a = run.am_novo - 1;
    if (a < 0 || a >= A_N) { amuleto_segue(); return; }
    if (tirar >= 0 && (run_amuletos() >> tirar & 1)) {
        run_tira_amuleto(tirar);
        run_poe_amuleto(a);
        som_toca(SOM_PREMIO);
        solta_confete(40);
    } else som_toca(SOM_FICHA);
    amuleto_segue();
}

// Desiste da troca: volta para a escolha dos tres. O do Selo, que nao se
// escolhe, fica na tela.
static void amuleto_desiste(void)
{
    if (run.am_volta != AV_INICIO && run.am_volta != AV_CHEFE) return;
    int a = run.am_novo - 1;
    run.am_novo = 0;
    des_salva();
    abre_amuleto();
    for (int i = 0; i < 3; i++) if (run.am_oferta[i] == a) cursor = i;
}

// Escolha de amuleto do comeco (os comuns) e de cada chefe (o primeiro deixa
// incomuns; o segundo e o Barao, quando o Coringa vem depois, raros).
static void oferece_amuleto(int volta)
{
    rs = run.semente ^ (0xA3E1u * (uint32_t)(run.andar * 8 + run.passo + 1 + volta * 64));
    rnd_r();
    unsigned rar = volta == AV_INICIO ? AMR(AMR_COMUM) : AMR(run.andar == 0 ? AMR_INCOMUM : AMR_RARO);
    sorteia_amuletos(run.am_oferta, 3, rar);
    run.am_volta = (uint8_t)volta;
    run.am_novo = 0;
    if (run.am_oferta[0] == 0xFF) { amuleto_segue(); return; }   // ja tem todos
    run.estado = R_AMULETO;
    des_salva();
    abre_amuleto();
}

static void amuleto_escolhido(int i)
{
    if (run.am_novo) return;                     // na troca, as teclas sao outras
    if (i < 0 || i >= 3 || run.am_oferta[i] >= A_N) return;
    ganha_amuleto(run.am_oferta[i], run.am_volta);
}

// A bolsa do comeco: dados simples, muitos fracos. Tirar os fracos (na loja)
// e parte da run.
static const uint8_t BOLSA_INICIO[] = { D_D6, D_D6, D_D6, D_D6, D_D6, D_D6, D_D4, D_D4, D_D4, D_D4, D_D2, D_D2 };

static int moedas_iniciais(int dif) { return DES_MOEDAS + dif / DES_MOEDAS_NIVEL; }

static void des_nova_run(void)
{
    memset(&run, 0, sizeof run);
    memcpy(run.marca, RUN_MARCA, 4);
    run.semente = ((uint32_t)esp_timer_get_time() * 2654435761u) ^ 0x5bd1e995u;
    if (!run.semente) run.semente = 1;
    run.dificuldade = (uint8_t)des_dif;
    run.moedas = moedas_iniciais(run.dificuldade);
    if (perfil_atual >= 0) { if (perfis[perfil_atual].runs < 65535) perfis[perfil_atual].runs++; perfis_salva(); }
    for (unsigned i = 0; i < sizeof BOLSA_INICIO; i++) run.col[run.n_col++] = (peca_t){ .tipo = BOLSA_INICIO[i], .fixo = 0 };
    gera_andar();
    oferece_amuleto(AV_INICIO);                  // a run comeca com um amuleto
    t_elev = ELEV_T;                             // e o elevador sobe ate o andar
    moedas_ok = false;                           // as moedas da faixa nascem certas
}

// Monta a mesa de uma partida da run e comeca a primeira rodada. A mesma mesa,
// recomecada (quem saiu no meio e voltou), sorteia do mesmo jeito.
// O Acumulador (e a Furia) guarda o bonus so dentro de uma partida: a cada
// partida da run ele recomeca do zero. A Onda e os Tres Patinhos levam a
// contagem de uma partida para a outra.
static void zera_acumuladores(peca_t *col, int n)
{
    for (int i = 0; i < n; i++) {
        if (col[i].tipo == D_ACUMULADOR || col[i].tipo == D_AUTOFAGIA || col[i].tipo == D_PREVISIVEL
            || col[i].tipo == D_FURIA) col[i].fixo = 0;
        col[i].figurinha = 0;                    // as figurinhas juntadas so valem na partida
    }
}

static void des_comeca_partida(void)
{
    const no_t *no = &run.no[run.passo];
    run.estado = R_PARTIDA;
    des_salva();
    modo = M_DESAFIO;
    cpu = 2;
    remoto = 0;
    baixo = 0;
    semente = (no->semente * 2654435761u) | 1;
    for (int i = 0; i < 8; i++) rnd();
    som_rufo(false);
    memset(J, 0, sizeof J);
    memset(F, 0, sizeof F);
    J[0].nome = perfil()->nome;
    J[0].cor = C_LATAO;
    amul[0] = run_amuletos();
    memset(am_brilho, 0, sizeof am_brilho);
    amul[1] = trunfo_ativo(no->perso) ? PERSO[no->perso].amul : 0;   // o trunfo do oponente, se ja vale
    op_amul_extra = -1;
    // Na Cobertura, mais um amuleto para ele (o Barao, nao: ele ja vira forte).
    if (run.dificuldade >= N_NIVEIS && chefe_idx(no->perso) != 2) {
        static const uint8_t EXTRA[] = { A_LUVA, A_REDE, A_BOLSA, A_OURO, A_PENA, A_IMA,
                                         A_COURACA, A_RESERVA, A_POTE, A_FURADO };
        uint32_t h = no->semente * 2246822519u;
        for (int i = 0; i < 4; i++) {
            int a = EXTRA[(h >> (i * 8)) % sizeof EXTRA];
            if (!(amul[1] >> a & 1)) { amul[1] |= AM_BIT(a); op_amul_extra = a; break; }
        }
    }
    des_rod_vencidas = 0;
    reg_op = trunfo_ativo(no->perso) ? PERSO[no->perso].reg : REG_NADA;
    estilo_op = PERSO[no->perso].estilo;
    op_nervoso = false;
    fala_perso = no->perso;
    perso_virado = -1;
    t_virada = 0;
    t_fala = 0;
    t_reacao = t_curta = 0;
    fala_e_curta = false;
    J[0].fichas = DES_FICHAS + (tem_am(0, A_RESERVA) ? 10 : 0);
    J[0].n_col = run.n_col;
    memcpy(J[0].col, run.col, sizeof run.col);
    for (int i = 0; i < J[0].n_col; i++) J[0].col[i].velcro = 0;   // o Velcro so segura de uma rodada para a outra
    zera_acumuladores(J[0].col, J[0].n_col);
    J[1].nome = PERSO[no->perso].nome;
    J[1].cor = C_ROSA;
    J[1].fichas = DES_FICHAS + (tem_am(1, A_RESERVA) ? 10 : 0);
    if (no->tipo == NO_CHEFE && tem_am(0, A_PEDAGIO)) J[1].fichas -= 10;         // Pedagio
    J[1].n_col = monta_oponente(no, run.andar, J[1].col);
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < J[j].n_col; i++) J[j].onde[i] = NA_BOLSA;
    nivel_jog[1] = nivel_mesa(no);
    // O material do oponente, pelo nivel da mesa: a maioria de marfim; osso
    // nas dificeis do 2o salao e no 3o; jade nas dificeis do 3o; os chefes,
    // de obsidiana.
    bool dificil = no->tipo == NO_DIFICIL;
    materiais_da_partida(no->tipo == NO_CHEFE ? MAT_OBSIDIANA
                         : run.andar == 2 ? (dificil ? MAT_JADE : MAT_OSSO)
                         : run.andar == 1 && dificil ? MAT_OSSO : MAT_MARFIM);
    ousadia_jog[1] = PERSO[no->perso].ousadia;
    rodada = 0;
    memset(bis_extra, 0, sizeof bis_extra);
    memset(ad_moedas, 0, sizeof ad_moedas);
    pote = 0;
    primeiro = (int)(rnd() & 1);
    venc_partida = -1;
    correu_venc = -1;
    n_voo = 0;
    t_sair = -1;
    des_saiu = false;
    detalhe = -1;
    nova_rodada();
    fala_evento(FA_INICIO);
}

// Fim de uma partida da run (chamado assim que ela acaba, para que sair na
// tela final nao deixe jogar de novo): vitoria vira moedas; derrota ou empate
// encerram a run.
static void des_resultado(void)
{
    const no_t *no = &run.no[run.passo];
    // A bolsa volta para a run como terminou: o Quebrado e o que o Fogo
    // queimou nao voltam (e o embrulho do Papai Noel, desfeito).
    desfaz_embrulho();
    run.n_col = (uint8_t)J[0].n_col;
    memcpy(run.col, J[0].col, sizeof run.col);
    zera_acumuladores(run.col, run.n_col);
    if (perfil_atual >= 0 && perfis[perfil_atual].partidas < 65535) {
        perfis[perfil_atual].partidas++;
        perfis_salva();
    }
    if (venc_partida != 0) {
        des_perdeu_para = no->perso;
        des_perdeu_andar = run.andar;
        salva_apaga(chave_run());
        return;
    }
    run.vitorias++;
    des_marco = -1;
    des_chefe_novo = false;
    if (no->tipo == NO_CHEFE && perfil_atual >= 0) {
        perfil_t *p = &perfis[perfil_atual];
        if (run.andar < 2 && !(p->marcos >> run.andar & 1)) {
            p->marcos |= (uint8_t)(1u << run.andar);                  // abre o D20 ou o Lastro
            des_marco = run.andar == 0 ? D_D20 : D_LASTRO;
        }
        // O chefe abre o pedaco dele nesta dificuldade (o salao do chefe e o
        // indice dele: o Coringa e o do salao final). A dificuldade
        // seguinte, se for dele, so no fim da run.
        int c = run.andar, d = run.dificuldade;
        if (!chefe_abriu(d, c)) {
            novidade_t pd[PEDACO_MAX];
            int n = pedaco(d, c, pd);
            for (int i = 0; i < n; i++) des_chefe_novo |= pd[i].k != NV_DIF;
        }
        p->chefes[d] |= (uint8_t)(1u << c);
        des_chefe_c = c;
        perfis_salva();
    }
    // As moedas da mesa (o fim da run nao paga: nao ha mais loja), o Colar
    // (1), o Cofre (1 a cada 5 que voce ja tinha, ate 3) e a Taca (1 a cada 3
    // rodadas vencidas na partida).
    run.ganho = run.colar = run.juros = run.taca = run.dourado = 0;
    if (!fim_da_run()) {
        run.ganho = MOEDAS_MESA[run.andar][run.passo];
        if (run_amuletos() >> A_COLAR & 1) run.colar = 1;
        run.dourado = (uint8_t)(ad_moedas[0] < 255 ? ad_moedas[0] : 255);   // o adesivo Dourado, na partida
        if (run_amuletos() >> A_COFRE & 1) run.juros = (uint8_t)(run.moedas / 5 < 3 ? run.moedas / 5 : 3);
        if (run_amuletos() >> A_TACA & 1) run.taca = (uint8_t)(des_rod_vencidas / 3);
    }
    run.moedas += saque_total();
    run.estado = R_VENCEU;
    des_salva();
}

// Depois da mesa vencida: o fim da run, o amuleto do chefe ou a loja.
static void des_pos_vitoria(void)
{
    if (fim_da_run()) { encerra_run(true); return; }
    run.am_selo = 0;
    if (chefe_deixa_amuleto()) oferece_amuleto(AV_CHEFE);
    else abre_loja_pos();
}

// Sair no meio de uma partida da run (ESC duas vezes) conta como derrota:
// senao, continuar recomecaria a mesma mesa do zero, e dava para tentar de
// novo sem limite. Na tela do fim da ultima rodada a partida ja esta
// decidida: termina com o resultado de verdade.
static bool des_abandono(void)
{
    return modo == M_DESAFIO && fase != F_FIM && fase != F_RAMULETO && fase != F_NOVIDADES;
}

static void segue_do_resultado(void);
static void des_abandona(void)
{
    t_sair = -1;
    if (fase == F_RESULTADO && partida_se(2) >= 0) { segue_do_resultado(); return; }   // o pote ja foi: o fim de verdade
    des_saiu = true;
    n_voo = 0;                                   // os dados no ar ficam onde estao
    som_rufo(false);
    encerra(1);
}

// ENTER na tela do fim da partida.
static void des_apos_fim(void)
{
    if (venc_partida == 0 && nov_tem_chefe() && !fim_da_run()) nov_abre_chefe();   // o chefe abriu coisas: os cards antes
    else if (venc_partida == 0) des_pos_vitoria();
    else encerra_run(false);
}

static void des_continua(void)
{
    if (!des_carrega(&run)) { des_nova_run(); return; }
    moedas_ok = false;
    switch (run.estado) {
    case R_LOJA_INI: case R_LOJA: abre_loja_run(); break;
    case R_PARTIDA:               des_comeca_partida(); break;
    case R_VENCEU:                des_pos_vitoria(); break;
    case R_AMULETO:               abre_amuleto(); break;
    default:                      abre_mapa(); break;
    }
}
