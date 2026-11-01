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

// Os marcos da partida online. A cada rodada que comeca e a cada vez que
// alguem para, o estado das regras - o sorteio, as fichas, o pote, as bolsas
// e as sequencias - vira um numero, e a rede manda o ultimo ao rival no sinal
// de vida. Os dois lados passam pelos mesmos marcos na mesma ordem, cada um
// no seu tempo; se o numero de um marco diferir, as duas mesas sairam do
// compasso e a partida para com o aviso, em vez de seguir com dois placares.
static int n_marco;
static uint32_t hash_partida(void)
{
    uint32_t h = 2166136261u;
#define MIX(v) (h = (h ^ (uint32_t)(v)) * 16777619u)
    MIX(semente); MIX(partidas); MIX(rodada); MIX(vez); MIX(primeiro); MIX(pote);
    for (int j = 0; j < 2; j++) {
        MIX(J[j].fichas);
        MIX(J[j].n_col);
        for (int i = 0; i < J[j].n_col; i++) { MIX(J[j].col[i].tipo); MIX(J[j].col[i].fixo); }
        MIX(F[j].n);
        for (int k = 0; k < F[j].n; k++) { MIX(F[j].tipo[k]); MIX(F[j].valor[k]); MIX(F[j].est[k]); }
    }
#undef MIX
    return h;
}
static void marca_online(void)
{
    if (modo == M_ONLINE) rede_marco(++n_marco, hash_partida());
}

void dado_online_comeca(uint32_t sem, int eu)
{
    modo = M_ONLINE;
    cpu = 0;
    baixo = eu;
    remoto = 1 << outro(eu);
    semente_online = sem;
    rede_ini = rede_fim = 0;
    n_marco = 0;
    digitando = false;
    dado_inicia(0);
}

// Quem abre um link de convite cai direto na tela do online; a plataforma
// entra na sala logo em seguida (a tela mostra "entrando na sala").
void dado_online_abre(void)
{
    if (modo == M_ONLINE && fase != F_ONLINE) return;   // ja numa partida online
    menu_tela = MT_PRINCIPAL;
    abertura_pronta = true;
    online_cur = 1;
    digitando = false;
    codigo_dig[0] = 0;
    t_fase = 0;
    fase = F_ONLINE;
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
    menu_tela = MT_PRINCIPAL;
    menu_cur = MN_UM;
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
    case F_MAPA: case F_RLOJA: case F_RBOLSA: case F_RFIM: case F_CREDITOS: case F_NOVIDADES:
        volta_ao_menu();                     // a run fica guardada
        return;
    case F_PERFIS: case F_PCRIA: case F_RDIFIC: case F_HIST: case F_ESTILO: case F_RBOLSAS:
        des_esc();
        return;
    case F_ONLINE:
        if (digitando) { digitando = false; return; }
        rede_sai();
        ao_menu_sem_partida();
        return;
    default:
        if (!conexao_caiu() && t_sair < 0) t_sair = 0;
        else if (des_abandono()) des_abandona();   // Desafiante: sair no meio e perder a mesa
        else volta_ao_menu();
        return;
    }
}
