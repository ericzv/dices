// ===========================================================================
// Os sons que a cena pode pedir (a carga sob demanda do PC e do site)
// ===========================================================================
// Decodificados, os efeitos ocupam mais de 100 MB; quase tudo e som de um
// dado, de um adesivo ou de um acabamento que nao esta na mesa. A plataforma
// (desktop/som.c) guarda sempre os sons de todo dia (o menu, a mesa de
// marfim, as fichas, os vereditos...) e pergunta aqui, a cada quadro, quais
// dos outros a cena pode pedir: os dos dados e adesivos das duas bolsas (a
// do oponente entra quando a partida e montada), os da fila, os das lojas e
// do premio. Ela vai decodificando esses aos poucos e solta os que deixaram
// de ser pedidos ha um tempo.
//
// Um som que ficou de fora desta conta nao fica mudo: a plataforma o
// decodifica na hora em que toca (um quadro mais lento, uma vez). O teste
// tests/simula.c confere que isso quase nunca acontece.

// Os sons sob demanda: os dos adesivos, os dos acabamentos e das assinaturas
// e os dos poderes que so um dado toca. O resto fica sempre na memoria.
static bool som_sob_demanda(int s)
{
    if (s >= SOM_AD_COLA && s <= SOM_AD_CASCA) return true;
    if (s >= SOM_AD_CORRENTE && s <= SOM_AD_DINAMITE_APAGA) return true;
    if (s >= SOM_POUSA_LACA && s <= SOM_AS_PASSO) return true;      // acabamentos, assinaturas e a moeda de metal
    switch (s) {
    case SOM_JACKPOT: case SOM_LASER: case SOM_VENTO: case SOM_BIGORNA:
    case SOM_SINO: case SOM_GELO: case SOM_ONDA: case SOM_PRESENTE: case SOM_ESTRELA:
    case SOM_CARREGA: case SOM_TIRO: case SOM_EVOLUI: case SOM_BURACO: case SOM_BIG_BANG:
    case SOM_VAMPIRO: case SOM_SABOTADOR: case SOM_COVEIRO: case SOM_AUTOFAGIA: case SOM_AMPULHETA:
    case SOM_AGNUS: case SOM_PIAO: case SOM_GELATINA: case SOM_MATRIOSKA: case SOM_BALAO:
    case SOM_LEQUE: case SOM_PAPAGAIO: case SOM_RETRATO: case SOM_ESTILINGUE:
    case SOM_AD_COLEIRA: case SOM_AD_TIQUETE:
    case SOM_TUDO_NADA: case SOM_TUDO_NADA_PERDE: case SOM_SENTINELA: case SOM_AGIOTA:
    case SOM_OCULTO: case SOM_ESPINHOSO: case SOM_PAVIO: case SOM_CARROSSEL:
    case SOM_ELEVADOR:
        return true;
    default:
        return false;
    }
}

static uint8_t *sp_som, *sp_rola;        // onde a conta da vez vai sendo marcada
static int sp_lacos;
static void sp(int s) { if (s >= 0 && s < SOM_N) sp_som[s] = 1; }
static void sp_laco(int q) { if (q > 0 && q < SOM_ROLAS) sp_rola[q] = 1; }

// Os sons de um adesivo: o dele agindo, o de colar e os que so ele puxa.
static void sp_adesivo(int a)
{
    a &= ~AD_BIT_VELHO;
    if (a <= AD_NADA || a >= AD_N) return;
    sp(som_adesivo(a));
    sp(som_cola());
    switch (a) {
    case AD_DINAMITE: sp(SOM_AD_DINAMITE_APAGA); sp(SOM_PAVIO); break;
    case AD_BARRO:    sp(som_adesivo(AD_SUJEIRA)); break;      // o que ele deixa no dado do oponente
    case AD_FRAGIL: case AD_CASCA: sp(som_adesivo(AD_IMPERMEAVEL)); break;   // barrados pela bolha
    case AD_CARIMBO:  sp(SOM_AD_CARIMBO); break;
    case AD_DUPLA:    sp(SOM_AD_DUPLA); break;
    default: break;
    }
}

// Os sons de um dado do tipo t na mao de dono: o acabamento (pouso, batidas
// e o laco rolando), a assinatura e o poder.
static void sp_dado(int t, int dono)
{
    if (t < 0 || t >= D_N) return;
    if (TIPO[t].lados == 2) {                      // moedas e fichas
        sp(d2_pousa(t)); sp(d2_bate(t));
        if (d2_rola(t) >= 0) sp_laco(d2_rola(t)); else sp_lacos |= SONS_LACO_MOEDA;
    } else {
        const acabamento_t *ac = &ACAB[acab_de(t, dono)];
        sp(ac->som_pousa); sp(ac->som_bate); sp(ac->som_bate_dado);
        sp_laco(ac->rola);
    }
    switch (assina_de(t)) {
    case AS_FURIA:   sp(SOM_POUSA_FURIA); sp_laco(SOM_ROLA_FURIA); break;
    case AS_TREMOR:  sp(SOM_POUSA_TREMOR); break;
    case AS_ONDA:    sp_laco(SOM_ROLA_ONDA); break;
    case AS_ARRASTA: sp_laco(SOM_ROLA_ARRASTA); break;
    case AS_RAJADA:  sp(SOM_SOPRO); break;
    case AS_DESLIZA: sp(SOM_TRENO_SINO); break;
    case AS_OCO:     sp(SOM_AS_OCO); break;
    case AS_ECO:     sp(SOM_AS_ECO); break;
    case AS_BIGORNA: sp(SOM_AS_BIGORNA); break;
    case AS_ENTERRA: sp(SOM_AS_ENTERRA); break;
    case AS_RETA:    sp(SOM_AS_RETA); break;
    case AS_ATRAI:   sp(SOM_AS_ATRAI); break;
    case AS_GALOPE:  sp(SOM_AS_CASCO); break;
    case AS_SEM_SOMBRA: sp(SOM_AS_ASAS); break;
    case AS_ESPINHO: sp(SOM_AS_ESPINHO); break;
    case AS_TAMBOR:  sp(SOM_AS_TAMBOR); sp(SOM_AS_TRAVA); break;
    case AS_ANDA:    sp(SOM_AS_PASSO); break;
    default: break;
    }
    int ef = TIPO[t].efeito;                       // a voz do poder na cadeia do fim da rodada
    if (ef > 0 && ef < EF_N && VOZ_PODER[ef].som) sp(VOZ_PODER[ef].som);
    switch (t) {                                   // e o que o som_do_evento e a mesa tocam por ele
    case D_JACKPOT:  sp(SOM_JACKPOT); break;
    case D_LASER:    sp(SOM_LASER); break;
    case D_BONECO:   sp(SOM_GELO); break;
    case D_VENTANIA: sp(SOM_VENTO); break;
    case D_ONDA: case D_TSUNAMI:                   // a Onda vira Tsunami
        sp(SOM_ONDA); sp_laco(SOM_ROLA_ONDA);
        sp(ACAB[ACAB_TIPO[D_TSUNAMI]].som_pousa); sp(ACAB[ACAB_TIPO[D_TSUNAMI]].som_bate); sp(ACAB[ACAB_TIPO[D_TSUNAMI]].som_bate_dado);
        break;
    case D_FERREIRO: case D_MEDUSA: sp(SOM_BIGORNA); break;
    case D_JINGLE: case D_RENA: sp(SOM_SINO); break;
    case D_PRESENTE: sp(SOM_PRESENTE); break;
    case D_ESTRELA:  sp(SOM_ESTRELA); break;
    case D_MEIA:     sp(SOM_AD_DOURADO); break;
    case D_REVOLVER: sp(SOM_CARREGA); sp(SOM_TIRO); break;
    case D_RELOGIO:  sp(SOM_CARREGA); break;
    case D_DIGITAL:  sp(SOM_AD_ELETRONICO); break;
    case D_ASTRO: case D_CONSTELACAO: case D_GALAXIA: case D_BURACO: case D_UNIVERSO:   // o ciclo cosmico: um vira o outro
        sp(SOM_EVOLUI); sp(SOM_BURACO); sp(SOM_BIG_BANG); sp(SOM_AS_ATRAI);
        break;
    case D_AUTOFAGIA: sp(SOM_AUTOFAGIA); break;
    case D_AGNUS:    sp(SOM_AGNUS); break;
    case D_PIAO:     sp(SOM_PIAO); break;
    case D_GELATINA: sp(SOM_GELATINA); break;
    case D_MATRIOSKA: sp(SOM_MATRIOSKA); break;
    case D_BALAO:    sp(SOM_BALAO); break;
    case D_LEQUE:    sp(SOM_LEQUE); break;
    case D_PAPAGAIO: sp(SOM_PAPAGAIO); break;
    case D_RETRATO:  sp(SOM_RETRATO); break;
    case D_ESTILINGUE: sp(SOM_ESTILINGUE); break;
    case D_TUDO_NADA: sp(SOM_TUDO_NADA); sp(SOM_TUDO_NADA_PERDE); break;
    case D_CARROSSEL: sp(SOM_CARROSSEL); break;
    default: break;
    }
}

static void sp_peca(const peca_t *p, int dono)
{
    sp_dado(p->tipo, dono);
    for (int l = 0; l < AD_LADOS; l++) if (p->ad[l]) sp_adesivo(p->ad[l]);
}

// som: SOM_N marcas (1: a cena pode pedir); rola: SOM_ROLAS marcas, dos
// lacos do dado rolando. Devolve os outros lacos (SONS_LACO_*).
int dado_sons_precisos(uint8_t *som, uint8_t *rola)
{
    sp_som = som; sp_rola = rola; sp_lacos = 0;
    for (int s = 0; s < SOM_N; s++) som[s] = !som_sob_demanda(s);
    memset(rola, 0, SOM_ROLAS);
    rola[SOM_ROLA_FELTRO] = 1;
    for (int j = 0; j < 2; j++) {                  // as duas bolsas e o que esta na fila (o que mudou de forma)
        for (int i = 0; i < J[j].n_col && i < MAX_COL; i++) sp_peca(&J[j].col[i], j);
        for (int k = 0; k < F[j].n && k < N_FILA; k++) sp_dado(F[j].tipo[k], j);
    }
    for (int i = 0; i < n_voo && i < 2; i++) sp_dado(voo[i].tipo, voo[i].dono);   // o dado no ar (o Presente desembrulhado)
    if (fase == F_PREMIO || fase == F_RESULTADO || fase == F_FIM) {   // o premio: os cartoes e a roleta
        for (int i = 0; i < 3; i++) { sp_peca(&oferta[i], vez); sp_adesivo(oferta_ad[i]); }
        sp_lacos |= SONS_LACO_GIRO;
    }
    if (fase == F_LOJA)
        for (int i = 0; i < N_LOJA; i++) sp_peca(&loja[i], vez);
    if (modo == M_DESAFIO) {                       // a run: a sua bolsa, a loja, o pacote e o elevador
        for (int i = 0; i < run.n_col && i < MAX_COL; i++) sp_peca(&run.col[i], 0);
        for (int i = 0; i < VITRINE_MAX; i++) sp_peca(&run.vitrine[i], 0);
        sp_adesivo(run.adesivo); sp_adesivo(run.adesivo2);
        for (int i = 0; i < PAC_OPCOES; i++) sp_peca(&run.pac_op[i], 0);
        sp(SOM_ELEVADOR);
        sp_lacos |= SONS_LACO_MOTOR | SONS_LACO_GIRO;
    }
    return sp_lacos;
}
