// ---------------------------------------------------------------------------
// Salvamento: a estrutura da run em hexadecimal
// ---------------------------------------------------------------------------
// A IA da mesa: Prata faz as mesas dificeis pensarem mais; Diamante, os chefes.
static int nivel_mesa(const no_t *no)
{
    bool pensa = (run.dificuldade >= 2 && no->tipo == NO_DIFICIL) || (run.dificuldade >= 5 && no->tipo == NO_CHEFE);
    return no->nivel + (pensa && no->nivel < 2 ? 1 : 0);
}

static void des_salva(void)
{
    static char buf[2 * sizeof(run_t) + 1];
    hex_de(&run, sizeof run, buf);
    salva_grava(chave_run(), buf);
}

static bool des_carrega(run_t *r)
{
    static char buf[2 * sizeof(run_t) + 8];
    int n = salva_le(chave_run(), buf, (int)sizeof buf);
    if (n != (int)(2 * sizeof(run_t))) return false;
    memset(r, 0, sizeof *r);
    if (!de_hex(buf, r, sizeof *r)) return false;
    if (memcmp(r->marca, "DES1", 4) || r->andar >= DES_ANDARES || r->passo >= DES_PASSOS
        || r->estado > R_EVENTO || r->n_col < N_FILA || r->n_col > MAX_COL) return false;
    for (int i = 0; i < r->n_col; i++) if (r->col[i].tipo >= D_N) return false;
    if (r->dificuldade > N_NIVEIS) return false;
    return true;
}

static bool des_tem_run(void)
{
    run_t r;
    return des_carrega(&r);
}

// "Salão Vermelho · mesa 2 · 14 dados · 120 moedas"
static void des_resumo(char *s, int n)
{
    run_t r;
    if (!des_carrega(&r)) { s[0] = 0; return; }
    if (r.estado == R_AMULETO && r.vitorias == 0) snprintf(s, (size_t)n, "escolhendo o primeiro amuleto");
    else if (r.estado == R_EVENTO) snprintf(s, (size_t)n, "%s · num evento · %d moedas", SALAO[r.andar], r.moedas);
    else if (r.estado == R_LOJA_INI) snprintf(s, (size_t)n, "na loja da entrada · %d moedas", r.moedas);
    else snprintf(s, (size_t)n, "%s · mesa %d de %d · %d dados · %d moedas",
                  SALAO[r.andar], r.passo + 1, DES_PASSOS, r.n_col, r.moedas);
}

// ---------------------------------------------------------------------------
// Andamento da run
// ---------------------------------------------------------------------------
static void abre_mapa(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    run.estado = R_MAPA;
    des_cur = -1;
    for (int l = 0; l < 3 && des_cur < 0; l++) if (escolhivel(l)) des_cur = l;
    if (des_cur < 0) des_cur = 1;
    t_fase = 0;
    fase = F_MAPA;
}

static void abre_loja_run(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    cursor = 0;
    t_fase = 0;
    fase = F_RLOJA;
}

static void encerra_run(bool venceu)
{
    des_venceu_run = venceu;
    des_liberou = -1;
    if (venceu && perfil_atual >= 0) {          // venceu o Barao: a proxima dificuldade abre
        perfil_t *p = &perfis[perfil_atual];
        int d = run.dificuldade;
        if (p->venceu[d] < 255) p->venceu[d]++;
        if (d == p->nivel && p->nivel < N_NIVEIS) { p->nivel++; des_liberou = p->nivel; }
        perfis_salva();
    }
    salva_apaga(chave_run());
    t_fase = 0;
    fase = F_RFIM;
}

// Passo vencido (ou loja deixada): segue no mapa; depois do chefe, sobe de
// salao; depois do terceiro chefe, a run esta ganha.
static void avanca_passo(void)
{
    run.passo++;
    if (run.passo >= DES_PASSOS) {
        run.andar++;
        if (run.andar >= DES_ANDARES) { run.andar = DES_ANDARES - 1; encerra_run(true); return; }
        run.passo = 0;
        gera_andar();
    }
    run.estado = R_MAPA;
    des_salva();
    abre_mapa();
}

static void abre_amuleto(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    cursor = 0;
    t_fase = 0;
    fase = F_RAMULETO;
}

// Escolha de amuleto: volta = 0 vai para a loja da entrada; 1 segue no mapa.
static void oferece_amuleto(int volta)
{
    rs = run.semente ^ (0xA3E1u * (uint32_t)(run.andar * 8 + run.passo + 1 + volta * 64));
    rnd_r();
    sorteia_amuletos(run.am_oferta, 3);
    if (run.am_oferta[0] == 0xFF) {              // ja tem todos
        if (volta) avanca_passo();
        return;
    }
    run.am_volta = (uint8_t)volta;
    run.estado = R_AMULETO;
    des_salva();
    abre_amuleto();
}

static void amuleto_escolhido(int i)
{
    if (i >= 0 && i < 3 && run.am_oferta[i] != 0xFF) run_poe_amuleto(run.am_oferta[i]);
    som_toca(SOM_PREMIO);
    solta_confete(60);
    if (run.am_volta == 0) {
        run.estado = R_LOJA_INI;
        monta_loja_run();
        run.am_loja = 0xFF;                      // a loja da entrada so vende dados
        des_salva();
        abre_loja_run();
    } else {
        avanca_passo();
    }
}

static void des_nova_run(void)
{
    memset(&run, 0, sizeof run);
    memcpy(run.marca, "DES1", 4);
    run.semente = ((uint32_t)esp_timer_get_time() * 2654435761u) ^ 0x5bd1e995u;
    if (!run.semente) run.semente = 1;
    run.dificuldade = (uint8_t)des_dif;
    run.moedas = DES_MOEDAS + DES_MOEDAS_ANDAR * run.dificuldade;   // andar mais alto, bolsa mais cheia
    if (perfil_atual >= 0) { if (perfis[perfil_atual].runs < 65535) perfis[perfil_atual].runs++; perfis_salva(); }
    static const uint8_t INICIO[] = { D_D6, D_D6, D_D6, D_D6, D_D4, D_D4, D_D2, D_D2 };
    for (unsigned i = 0; i < sizeof INICIO; i++) run.col[run.n_col++] = (peca_t){ INICIO[i], 0 };
    gera_andar();
    oferece_amuleto(0);                          // a run comeca com um amuleto
    t_elev = ELEV_T;                             // e o elevador sobe ate o andar
}

// Monta a mesa de uma partida da run e comeca a primeira rodada. A mesma mesa,
// recomecada (quem saiu no meio e voltou), sorteia do mesmo jeito.
static void des_comeca_partida(void)
{
    const no_t *no = &run.no[run.passo][run.caminho[run.passo]];
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
    amul[1] = PERSO[no->perso].amul;              // o trunfo do oponente
    op_amul_extra = -1;
    if (run.dificuldade >= 5) {                   // Diamante: mais um amuleto para ele
        static const uint8_t EXTRA[] = { A_LUVA, A_REDE, A_BOLSA, A_OURO, A_PENA, A_IMA,
                                         A_COURACA, A_RESERVA, A_POTE, A_FURADO };
        uint32_t h = no->semente * 2246822519u;
        for (int i = 0; i < 4; i++) {
            int a = EXTRA[(h >> (i * 8)) % sizeof EXTRA];
            if (!(amul[1] >> a & 1)) { amul[1] |= 1u << a; op_amul_extra = a; break; }
        }
    }
    des_rod_vencidas = 0;
    reg_op = PERSO[no->perso].reg;
    op_nervoso = false;
    fala_perso = no->perso;
    perso_virado = -1;
    t_virada = 0;
    t_fala = 0;
    J[0].fichas = DES_FICHAS + (tem_am(0, A_RESERVA) ? 10 : 0);
    J[0].n_col = run.n_col;
    memcpy(J[0].col, run.col, sizeof run.col);
    J[1].nome = PERSO[no->perso].nome;
    J[1].cor = C_ROSA;
    J[1].fichas = DES_FICHAS + (tem_am(1, A_RESERVA) ? 10 : 0);
    if (no->tipo == NO_CHEFE && tem_am(0, A_PEDAGIO)) J[1].fichas -= 10;         // Pedagio
    if (run.garcom) J[1].fichas -= 10;                                            // o Garcom
    if (run.andar == 0 && run.passo == 0) J[1].fichas -= 10;                      // a primeira mesa aquece
    J[1].n_col = monta_oponente(no, run.andar, J[1].col);
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < J[j].n_col; i++) J[j].onde[i] = NA_BOLSA;
    nivel_jog[1] = nivel_mesa(no);
    ousadia_jog[1] = PERSO[no->perso].ousadia;
    rodada = 0;
    pote = 0;
    primeiro = (int)(rnd() & 1);
    venc_partida = -1;
    correu_venc = -1;
    n_voo = 0;
    t_sair = -1;
    detalhe = -1;
    nova_rodada();
    fala_evento(FA_INICIO);
}

// Fim de uma partida da run (chamado assim que ela acaba, para que sair na
// tela final nao deixe jogar de novo): vitoria vira moedas e premio; derrota
// ou empate encerram a run.
static void des_resultado(void)
{
    const no_t *no = &run.no[run.passo][run.caminho[run.passo]];
    // A bolsa volta para a run como terminou: o Quebrado e o que o Fogo
    // queimou nao voltam.
    run.n_col = (uint8_t)J[0].n_col;
    memcpy(run.col, J[0].col, sizeof run.col);
    run.garcom = 0;                              // o agrado do Garcom valeu para esta
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
    if (no->tipo == NO_CHEFE && perfil_atual >= 0 && run.andar < 2 && !(perfis[perfil_atual].marcos >> run.andar & 1)) {
        perfis[perfil_atual].marcos |= (uint8_t)(1u << run.andar);    // abre o D20 ou o Lastro
        perfis_salva();
        des_marco = run.andar == 0 ? D_D20 : D_LASTRO;
    }
    run.ganho = J[0].fichas;
    run.juros = 0;
    if (run_amuletos() >> A_COFRE & 1) {                                    // Cofre: 1 a cada 5
        int j = (run.moedas + run.ganho) / 5;               // sem limite (so o do byte)
        run.juros = (uint8_t)(j > 255 ? 255 : j);
    }
    run.taca = 0;
    if (run_amuletos() >> A_TACA & 1) {                                     // Taca: 2 por rodada vencida
        int t = 2 * des_rod_vencidas;
        run.taca = (uint8_t)(t > 40 ? 40 : t);
    }
    run.moedas += run.ganho + run.juros + run.taca;
    run.tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
    int tp = run.tier + ((run_amuletos() >> A_CARTOLA & 1) && run.tier < 3 ? 1 : 0);   // Cartola
    rs = no->semente ^ 0xA11CEu;
    rnd_r();
    sorteia_distintos(run.oferta, 3, PESO_PREMIO[tp]);
    run.estado = R_PREMIO;
    des_salva();
}

static void abre_premio_run(void)
{
    modo = M_DESAFIO;
    cpu = 0;
    baixo = 0;
    J[0].nome = perfil()->nome;
    J[0].cor = C_LATAO;
    J[0].n_col = run.n_col;
    memcpy(J[0].col, run.col, sizeof run.col);
    memcpy(oferta, run.oferta, sizeof run.oferta);
    vez = 0;
    cursor = 0;
    removendo = false;
    t_tira = 0;
    t_fase = 0;
    fase = F_PREMIO;
    slot_comeca();
}

// ENTER na tela do fim da partida.
static void des_apos_fim(void)
{
    if (venc_partida == 0) abre_premio_run();
    else encerra_run(false);
}

// Premio escolhido, recusado ou trocado por tirar um dado.
static void des_premio_feito(void)
{
    run.n_col = (uint8_t)J[0].n_col;
    memcpy(run.col, J[0].col, sizeof run.col);
    // Selo de Divida: o chefe paga um segundo premio.
    if (run.tier == 3 && run.andar < DES_ANDARES - 1 && (run_amuletos() >> A_SELO & 1) && !run.premio2) {
        const no_t *no = &run.no[run.passo][run.caminho[run.passo]];
        rs = no->semente ^ 0x5E10D1u;
        rnd_r();
        sorteia_distintos(run.oferta, 3, PESO_PREMIO[3]);
        run.premio2 = 1;
        run.estado = R_PREMIO;
        des_salva();
        abre_premio_run();
        return;
    }
    run.premio2 = 0;
    // Chefe vencido (menos o ultimo, que fecha a run): mais um amuleto.
    if (run.tier == 3 && run.andar < DES_ANDARES - 1) oferece_amuleto(1);
    else avanca_passo();
}

static void des_continua(void)
{
    if (!des_carrega(&run)) { des_nova_run(); return; }
    switch (run.estado) {
    case R_LOJA_INI: case R_LOJA: abre_loja_run(); break;
    case R_PARTIDA:               des_comeca_partida(); break;
    case R_PREMIO:                abre_premio_run(); break;
    case R_AMULETO:               abre_amuleto(); break;
    case R_EVENTO:                abre_evento(); break;
    default:                      abre_mapa(); break;
    }
}
