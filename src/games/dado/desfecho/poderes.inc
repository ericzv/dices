// ===========================================================================
// PODERES: os poderes dos dados que valem no fim - os ataques, os roubos, os
// que tiram pontos do rival e os que caem no dono (poder_de), o corte do
// Ranzinza, a Fenix e o Agnus.
// Le: F, d, J[].nome, a rodada, os amuletos (a Couraca) e os trunfos.
// Grava: d (est, bonus, delta, roubo_val, atacados, e o que as reacoes
// gravam), ev[] e agiu[] (quem ja agiu); com gera, acende a Couraca. Nao
// grava F nem a bolsa.
// Quem chama: calcula_desfecho e a Saideira (orquestra.inc); agnus_armado
// serve tambem a IA e a mesa.
// ===========================================================================

// Os poderes dos dados de j que valem no fim (ataques, roubos, os que
// tiram do rival e os que agem contra o dono). A Saideira chama de novo.
//
// Um dado ataca antes de cair: quando um poder vai anular um dado do
// oponente que tem um ataque armado contra ele (anular, roubar, cortar ou
// tirar pontos: ataca_oponente), esse ataque acontece primeiro
// (contra_ataque) e so depois o dado e anulado. Assim, dois dados que se
// miram se anulam os dois, quem quer que tenha aberto a rodada. Cada dado
// age uma vez so (agiu[j], um bit por lugar); a Saideira zera os de quem
// repete.
// ACHADO A03: preservado - isso vale para os dados; entre adesivos (Mira contra Mira), quem abriu a rodada leva
static uint8_t agiu[2];
static void poder_de(desfecho_t *d, int j, int k, bool gera);

// O dado k de j (valendo) tem um ataque armado contra o oponente: os poderes
// que contam como ataque para o Ourico (d->atacados).
static bool ataca_oponente(const desfecho_t *d, int j, int k)
{
    if (k < 0 || k >= F[j].lancados || d->est[j][k] != V_VALIDO) return false;
    int ef = TIPO[tipo_de(j, k)].efeito, v = F[j].valor[k], r = outro(j);
    switch (ef) {
    case EF_BERSERKER: return !tem_am(r, A_COURACA) && alvo_vizinho(d->est[r], r, k) >= 0;   // sem vizinho no oponente, cai no dono
    case EF_CARRASCO:  return v == CARRASCO_ANULA && F[j].cru[k] != 1;
    case EF_LASER:     return v >= LASER_DE;
    case EF_TSUNAMI:   return v >= TSUNAMI_DE;
    case EF_BONECO:    return v >= BONECO_DE;
    case EF_INVEJOSO:  return invejoso_age(v);
    case EF_AGNUS:     return v == AGNUS_RAIO;
    case EF_SABOTADOR: return v >= SABOTADOR_DE;
    case EF_VAMPIRO:   return true;
    case EF_MARTELO:   return v >= MARTELO_DE;
    case EF_MALDICAO:  return true;
    case EF_PIRATA:    return v >= PIRATA_DE;
    case EF_CARIDOSO:  return v >= CARIDOSO_DE;
    case EF_GATUNO:
        for (int i = 0; i < F[j].lancados; i++) if (i != k && F[j].cru[i] == 1 && d->est[j][i] == V_VALIDO) return true;
        return false;
    default:           return false;
    }
}

// O dado a de r vai ser anulado por um ataque: se ele tem um ataque armado
// que ainda nao aconteceu, acontece agora, antes.
static void contra_ataque(desfecho_t *d, int r, int a, bool gera)
{
    if (a < 0 || (agiu[r] >> a & 1) || !ataca_oponente(d, r, a)) return;
    poder_de(d, r, a, gera);
}

static void poderes_de(desfecho_t *d, int j, bool gera)
{
    for (int k = 0; k < F[j].lancados; k++)
        // ACHADO A34: preservado - laco sem a guarda < N_FILA (seguro enquanto lancados <= N_FILA)
        if (!(agiu[j] >> k & 1)) poder_de(d, j, k, gera);
}

// O poder do dado k de j, uma vez.
// Um golpe que anula, do dado k de j no dado a0 do oponente (ou -1, sem
// alvo): o Seguranca dele pode levar no lugar, a capa do Impermeavel segura,
// e o dado atingido ataca antes de cair (contra_ataque). Depois, a Coleira,
// a Ratoeira e o Cavalo de Troia (apos_ataque).
typedef struct {
    const char *quem;                            // quem ataca (o texto da capa e o do Seguranca)
    const char *seg;                             // quando o Seguranca leva: com quem ataca e o rival
    const char *anula;                           // o golpe: com o numero e o rival (ou so o rival)
    bool so_rival;                               // o texto do golpe so tem o nome do rival
} golpe_t;
#define SEG_NO_LUGAR "%s: o Segurança de %s leva no lugar"
static void golpe_anula(desfecho_t *d, int j, int k, int a0, const golpe_t *g, bool gera)
{
    int r = outro(j), a = desvia_seguranca(d->est[r], r, a0);
    if (a < 0 || bloqueia_capa(d, gera, j, k, r, a, g->quem, 0)) return;
    contra_ataque(d, r, a, gera);                // o ataque dele, se tiver, vem antes
    // ACHADO A22: preservado - o alvo nao e reconferido depois do contra-ataque: pode ser anulado de novo
    d->est[r][a] = V_ANULADO;
    if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a], g->seg, g->quem, J[r].nome);
    else if (g->so_rival) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a], g->anula, J[r].nome);
    else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a], g->anula, F[r].valor[a], J[r].nome);
    d->atacados[r]++;
    apos_ataque(d, j, k, r, a, gera);
}

// O maior dado valendo de r (pelo numero na face; o Universo nao) passa a
// pontuar metade, arredondando o corte para cima: o Sabotador e o Ranzinza.
// Devolve qual foi (ou -1, sem corte), quanto saiu e quanto ele pontua.
static int corta_metade(desfecho_t *d, int r, int *corte, int *fica)
{
    int a = -1;
    // ACHADO A20: preservado - o alvo e o de maior numero na face; se ele pontua 0, o corte e 0 e ninguem e cortado
    for (int i = 0; i < F[r].lancados && i < N_FILA; i++)
        if (d->est[r][i] == V_VALIDO && !universo(r, i) && (a < 0 || F[r].valor[i] > F[r].valor[a])) a = i;
    if (a < 0) return -1;
    int base = valor_pontos(r, a) + F[r].bonus[a] + d->bonus[r][a];
    int c = base > 0 ? (base + 1) / 2 : 0;
    if (!c) return -1;
    d->bonus[r][a] = (int8_t)(d->bonus[r][a] - c);
    *corte = c;
    *fica = base - c;
    return a;
}

// O dado k de j tira n pontos do placar do oponente (o Espinhoso, o Maldito,
// o Pirata): fora dos dados, e conta como ataque para o Ourico. fmt: o texto,
// com n e o nome do oponente.
static void tira_placar(desfecho_t *d, int j, int k, int n, const char *fmt, bool gera)
{
    int r = outro(j);
    d->delta[r] -= n;
    d->atacados[r]++;
    evento(gera, EV_TIRA, j, k, r, 0, n, fmt, n, J[r].nome);
}

static void poder_de(desfecho_t *d, int j, int k, bool gera)
{
    int r = outro(j);
    agiu[j] |= (uint8_t)(1 << k);
    int ef = TIPO[tipo_de(j, k)].efeito, v = F[j].valor[k];
    if (d->est[j][k] == V_ROUBADO) return;
    // Carrasco que tirou 1 (valendo ou nao): anula o menor dado de quem
    // o lancou - ele mesmo, se estiver valendo e for o menor (no empate,
    // ele).
    if (ef == EF_CARRASCO && F[j].cru[k] == 1) {
        // ACHADO A08: preservado - o Carrasco com 1 entra como alvo sem olhar a Ancora: preso por ela, anula a si mesmo
        int b = d->est[j][k] == V_VALIDO ? k : -1;
        for (int i = 0; i < F[j].lancados; i++)
            if (i != k && d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i)
                && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
        if (bloqueia_capa(d, gera, j, k, j, b, "Carrasco tirou 1", 0)) {
        } else if (b == k) {
            d->est[j][b] = V_ANULADO;
            evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b], "Carrasco tirou 1: anula a si mesmo");
        } else if (b >= 0) {
            d->est[j][b] = V_ANULADO;
            evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                   "Carrasco tirou 1: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
        }
    }
    // Todo poder so age se o dado terminar valendo. O Gatuno, alem
    // disso, rouba um dado por 1 tirado por outro dado seu valendo.
    int roubos = 1;
    if (ef == EF_GATUNO) {
        roubos = 0;
        for (int i = 0; i < F[j].lancados; i++) roubos += i != k && F[j].cru[i] == 1 && d->est[j][i] == V_VALIDO;
    }
    if (d->est[j][k] != V_VALIDO || roubos <= 0) return;
    const char *nome = TIPO[tipo_de(j, k)].nome;

    // Berserker: anula o dado do oponente na casa vizinha (a de antes; sem
    // ele, a de depois). Sem vizinho no oponente (ou com a Couraca), a furia
    // cai no dono: o dado logo antes dele (sem ele, o logo depois).
    if (ef == EF_BERSERKER) {
        int ro = tem_am(r, A_COURACA) ? -1 : alvo_vizinho(d->est[r], r, k);
        if (ro >= 0) {
            golpe_anula(d, j, k, ro, &(golpe_t){ "Berserker", SEG_NO_LUGAR, "Berserker anula o %d de %s", false },
                        gera);
            return;
        }
        int b = alvo_vizinho(d->est[j], j, k);
        if (b >= 0 && !bloqueia_capa(d, gera, j, k, j, b, "Berserker", 0)) {
            d->est[j][b] = V_ANULADO;
            evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                   "Berserker: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
        }
        return;
    }

    // Autofagia: nao ataca; come so depois da contagem (autofagia_de).
    if (ef == EF_AUTOFAGIA) return;

    // Buraco Negro que evolui: engole os seus dados logo antes e logo
    // depois dele na sequencia (os firmes resistem).
    if (ef == EF_BURACO) {
        for (int s = -1; s <= 1 && F[j].faixa[k]; s += 2) {
            int b = k + s;
            if (b < 0 || b >= F[j].lancados || d->est[j][b] != V_VALIDO || firme_sem_capa(j, b)) continue;
            if (bloqueia_capa(d, gera, j, k, j, b, "Buraco Negro", 0)) continue;
            d->est[j][b] = V_ANULADO;
            evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                   "Buraco Negro engole o %d do próprio %s", F[j].valor[b], J[j].nome);
        }
        return;
    }

    bool couraca = tem_am(r, A_COURACA);         // os ataques dele nao anulam
    if (couraca && gera && ((ef == EF_CARRASCO && v == CARRASCO_ANULA) || (ef == EF_LASER && v >= LASER_DE) || (ef == EF_INVEJOSO && invejoso_age(v))
                            || (ef == EF_BONECO && v >= BONECO_DE) || (ef == EF_TSUNAMI && v >= TSUNAMI_DE) || (ef == EF_AGNUS && v == AGNUS_RAIO)))
        am_acende(A_COURACA);
    // ACHADO A27: preservado - a Couraca acende ate com o Caridoso que tirou 1 ou 2 (nao rouba)
    if (gera && couraca && (ef == EF_CARIDOSO || ef == EF_GATUNO)) am_acende(A_COURACA);
    if (ef == EF_CARRASCO && v == CARRASCO_ANULA) {
        int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1);
        golpe_anula(d, j, k, a0, &(golpe_t){ "Carrasco", SEG_NO_LUGAR, "Carrasco anula o %d de %s", false }, gera);
    } else if ((ef == EF_LASER && v >= LASER_DE) || (ef == EF_TSUNAMI && v >= TSUNAMI_DE)) {
        // O Laser (5 ou 6) pega o ultimo dado do oponente que pode ser
        // anulado; o Tsunami (9 a 12), o primeiro.
        int a0 = couraca ? -1 : ef == EF_LASER ? alvo_ultimo(d->est[r], r) : alvo_primeiro(d->est[r], r);
        const char *anula = ef == EF_TSUNAMI ? "Tsunami leva o %d de %s" : "Laser anula o %d de %s";
        golpe_anula(d, j, k, a0, &(golpe_t){ nome, SEG_NO_LUGAR, anula, false }, gera);
    } else if (ef == EF_BONECO && v >= BONECO_DE) {     // Boneco de Neve com 8: congela o 4o do oponente
        int a0 = !couraca && F[r].lancados > 3 && d->est[r][3] == V_VALIDO && !firme_sem_capa(r, 3) ? 3 : -1;
        golpe_anula(d, j, k, a0, &(golpe_t){ "Boneco de Neve", SEG_NO_LUGAR, "Boneco de Neve congela o 4º de %s", true },
                    gera);
    } else if (ef == EF_INVEJOSO && invejoso_age(v)) {   // Invejoso com 5 ou 6: o maior do oponente
        int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1);
        golpe_anula(d, j, k, a0, &(golpe_t){ "Invejoso", SEG_NO_LUGAR, "Invejoso anula o %d de %s", false }, gera);
    } else if (ef == EF_AGNUS && v == AGNUS_RAIO) {   // Agnus com 8: a luz anula o maior do oponente
        int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1);
        golpe_anula(d, j, k, a0, &(golpe_t){ "Agnus", "%s: o Segurança de %s leva a luz",
                                             "Agnus: a luz anula o %d de %s", false }, gera);
    } else if (ef == EF_INVEJOSO && v <= 2) {    // com 1 ou 2: os seus dois menores, ele incluido
        uint8_t ja = 0;                              // (o de capa e um dos dois: bate nela e fica)
        for (int n = 0; n < 2; n++) {
            int b = -1;
            for (int i = 0; i < F[j].lancados; i++)
                if (d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i) && !(ja >> i & 1)
                    && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
            if (b < 0) break;
            ja |= (uint8_t)(1 << b);
            if (bloqueia_capa(d, gera, j, k, j, b, "Invejoso", 0)) continue;
            d->est[j][b] = V_ANULADO;
            if (b == k) evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b], "Invejoso tirou %d: anula a si mesmo", v);
            else evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                        "Invejoso tirou %d: anula o %d do próprio %s", v, F[j].valor[b], J[j].nome);
        }
    } else if (ef == EF_SABOTADOR && v >= SABOTADOR_DE) {   // o maior do oponente passa a valer metade
        int corte, fica, a = corta_metade(d, r, &corte, &fica);
        if (a >= 0) {
            d->atacados[r]++;
            evento(gera, EV_CORTA, j, k, r, a, corte, "Sabotador corta o %d de %s: pontua %d",
                   F[r].valor[a], J[r].nome, fica);
        }
    } else if (ef == EF_VAMPIRO) {               // valendo: -1 em cada dado do oponente que vale
        uint8_t m = 0;
        for (int i = 0; i < F[r].lancados && i < N_FILA; i++)
            // ACHADO A18: preservado - tira 1 tambem do dado que pontua 0: ele fica em -1
            if (d->est[r][i] == V_VALIDO && !universo(r, i)) { d->bonus[r][i] = (int8_t)(d->bonus[r][i] - 1); m |= (uint8_t)(1 << i); }
        if (m) d->atacados[r]++;
        evento_t *e = m ? evento(gera, EV_VAMPIRO, j, k, r, 0, 1, "Vampiro: -1 em cada dado de %s", J[r].nome) : NULL;
        if (e) e->mascara = m;
    } else if (ef == EF_MARTELO && v >= MARTELO_DE) {     // Espinhoso: com 3 ou 4
        tira_placar(d, j, k, 4, "Espinhoso: -%d para %s", gera);
    } else if (ef == EF_MALDICAO) {              // vale para o dono e tira do oponente
        tira_placar(d, j, k, v, "Maldito: -%d para %s", gera);
    } else if (ef == EF_PIRATA && v >= PIRATA_DE) {      // so tira; nao fica com os pontos
        tira_placar(d, j, k, 4, "Pirata tira %d de %s", gera);
    } else if ((ef == EF_CARIDOSO && v >= CARIDOSO_DE) || ef == EF_GATUNO) {   // Caridoso: com 3 ou 4; Gatuno: um por 1
        for (int q = 0; q < roubos; q++) {                             // o menor, depois o menor que sobrou...
            // ACHADO A05: preservado - o laco nao reconfere se o Gatuno ainda vale: anulado pelo Cavalo, segue roubando
            int a0 = alvo_de(d->est[r], r, false, -1), a = desvia_seguranca(d->est[r], r, a0);
            if (a < 0) break;
            // ACHADO A10: preservado - o roubo vale o dado cheio, sem o corte ja aplicado em d->bonus
            int vale = contribui(r, a);
            // ACHADO A14: preservado - o roubo nao passa por bloqueia_capa: o dado com a capa do Impermeavel e roubado
            d->est[r][a] = V_ROUBADO;
            d->roubo_val[j] += vale;
            if (a != a0) evento(gera, EV_ROUBA, j, k, r, a, vale,
                                "%s: o Segurança de %s vai no lugar (%d)", nome, J[r].nome, vale);
            else evento(gera, EV_ROUBA, j, k, r, a, vale,
                        "%s rouba o %d de %s", nome, vale, J[r].nome);
            d->atacados[r]++;
            apos_ataque(d, j, k, r, a, gera);
        }
    }
}

// O Ranzinza (trunfo): da 5a rodada em diante, o seu maior dado valendo
// pontua metade - uma vez por rodada, depois dos poderes.
#define RANZINZA_RODADA 5
static void ranzinza_corta(desfecho_t *d, bool gera)
{
    if (!tem_reg(REG_RANZINZA) || rodada < RANZINZA_RODADA) return;
    int corte, fica, a = corta_metade(d, 0, &corte, &fica);
    if (a < 0) return;
    evento(gera, EV_CORTA, 1, 0xFF, 0, a, corte, "%s: o seu %d pontua só %d", J[1].nome, F[0].valor[a], fica);
}

// Fenix: anulada, renasce valendo o que tirou (roubada, nao).
static void fenix_de(desfecho_t *d, int j, bool gera)
{
    for (int k = 0; k < F[j].lancados; k++)
        if (d->est[j][k] == V_ANULADO && TIPO[tipo_de(j, k)].efeito == EF_FENIX) {
            d->est[j][k] = V_VALIDO;
            evento(gera, EV_RENASCE, j, k, j, k, F[j].valor[k], "Fênix renasce e pontua %d", F[j].valor[k]);
        }
}

// Agnus: tirando 7 (valendo ou nao; roubado, nao), no fim todos os dados
// anulados do dono nesta rodada ressuscitam - os que a sequencia derrubou e
// os que os ataques anularam, ele mesmo incluido - e pontuam o que tiraram.
// Os roubados nao voltam. Um evento so, com todos (mascara).
static int agnus_armado(int j)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        // ACHADO A06: preservado - devolve o primeiro Agnus com 7, mesmo roubado
        if (TIPO[tipo_de(j, k)].efeito == EF_AGNUS && F[j].valor[k] == AGNUS_NUMERO) return k;
    return -1;
}
static void agnus_de(desfecho_t *d, int j, bool gera)
{
    int ag = agnus_armado(j);
    // ACHADO A06: preservado - com o primeiro Agnus roubado, desiste: o segundo Agnus com 7 nao ressuscita ninguem
    if (ag < 0 || d->est[j][ag] == V_ROUBADO) return;
    uint8_t m = 0;
    int n = 0;
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        if (d->est[j][k] == V_ANULADO) { d->est[j][k] = V_VALIDO; m |= (uint8_t)(1 << k); n++; }
    if (!m) return;
    evento_t *e = evento(gera, EV_RESSUSCITA, j, ag, j, ag, n, n == 1 ? "Agnus: o anulado ressuscita"
                                                                      : "Agnus: %d anulados ressuscitam", n);
    if (!e) return;
    e->mascara = m;
    for (int k = 0; k < N_FILA; k++)              // o que cada um tirou
        e->por_dado[k] = (int8_t)(m >> k & 1 ? (F[j].valor[k] > 127 ? 127 : F[j].valor[k]) : 0);
}

