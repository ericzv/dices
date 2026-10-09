// ===========================================================================
// Online: o rival joga pelas mesmas teclas, vindas da rede
// ===========================================================================
// Uma tecla so conta na vez de quem decide, e so numa fase de escolha: assim
// ela cai no mesmo ponto da partida dos dois lados, por mais que as
// animacoes de cada computador andem em ritmos diferentes.
static bool aceita_escolha(void)
{
    return fase_de_escolha() && !(fase == F_PREMIO && t_tira > 0);
}

static bool rede_na_vez(void) { return (remoto >> vez & 1) && aceita_escolha(); }

// Digitando o codigo da sala: todas as letras sao do codigo, ate M e N.
bool dado_digitando(void) { return (fase == F_ONLINE && digitando) || fase == F_PCRIA; }

void dado_rede_tecla(int k)
{
    int prox_i = (rede_fim + 1) % FILA_REDE;
    if (prox_i == rede_ini) return;          // fila cheia: nunca numa partida normal
    rede_q[rede_fim] = (uint16_t)k;
    rede_fim = prox_i;
}

void dado_online_comeca(uint32_t sem, int eu)
{
    modo = M_ONLINE;
    cpu = 0;
    baixo = eu;
    remoto = 1 << outro(eu);
    semente_online = sem;
    rede_ini = rede_fim = 0;
    digitando = false;
    dado_inicia(0);
}

// Uma tecla do rival por quadro: da para ver o cursor dele andando.
static void rede_passo(void)
{
    if (rede_ini == rede_fim || !rede_na_vez()) return;
    int k = rede_q[rede_ini];
    rede_ini = (rede_ini + 1) % FILA_REDE;
    trata_tecla(k);
}

static bool conexao_caiu(void)
{
    return modo == M_ONLINE && fase != F_ONLINE && rede_estado() != REDE_JOGANDO;
}

// Volta ao menu de abertura, largando a partida (e a sala, se online).
static void volta_ao_menu(void)
{
    if (modo == M_ONLINE) rede_sai();
    modo = M_UM;
    cpu = remoto = 0;
    baixo = 0;
    dado_inicia(partidas + 1);
    for (int i = 0; i < n_voo; i++) { voo[i].parado = true; voo[i].vx = voo[i].vy = 0; }
    abertura_pronta = true;
    t_fase = 1.0f;
}

// Um nivel acima no menu (ESC ou BACKSPACE num submenu).
static void menu_volta(void)
{
    if (menu_tela == MT_NIVEL || menu_tela == MT_DESAFIO) { menu_cur = menu_tela == MT_NIVEL ? 0 : 1; menu_tela = MT_UM; }
    else { menu_tela = MT_PRINCIPAL; menu_cur = MN_UM; }
    sacode = 1;
}

static void ao_menu_sem_partida(void)
{
    t_fase = 1.0f;
    fase = F_ABERTURA;
}

// ESC: nas telas do menu, volta; no meio da partida, pede um segundo ESC.
static void trata_esc(void)
{
    switch (fase) {
    case F_ABERTURA: if (menu_tela) menu_volta(); return;
    case F_CATALOGO: case F_TUTORIAL: ao_menu_sem_partida(); return;
    case F_MAPA: case F_RLOJA: case F_RBOLSA: case F_RFIM: case F_CREDITOS:
        volta_ao_menu();                     // a run fica guardada
        return;
    case F_PERFIS: case F_PCRIA: case F_RDIFIC: case F_REVENTO:
        des_esc();
        return;
    case F_ONLINE:
        if (digitando) { digitando = false; return; }
        rede_sai();
        ao_menu_sem_partida();
        return;
    default:
        if (conexao_caiu() || t_sair >= 0) volta_ao_menu();
        else t_sair = 0;
        return;
    }
}
