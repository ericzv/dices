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
// O mesmo vale para a Mira (adesivo): a fase das Miras (miras_de) faz o dado
// que uma Mira derruba ainda disparar a sua.
static uint8_t agiu[2];
static void poder_de(desfecho_t *d, int j, int k, bool gera);

// O dado k de j (valendo) tem um ataque armado contra o oponente: os poderes
// que contam como ataque para o Ourico (d->atacados).
static bool ataca_oponente(const desfecho_t *d, int j, int k)
{
    if (k < 0 || k >= F[j].lancados || d->est[j][k] != V_VALIDO) return false;
    int ef = TIPO[tipo_de(j, k)].efeito, v = F[j].valor[k], r = outro(j);
    if (ef == EF_BERSERKER) return !tem_am(r, A_COURACA) && alvo_vizinho(d->est[r], r, k) >= 0;   // sem vizinho no oponente, cai no dono
    if (ef == EF_GATUNO) {
        for (int i = 0; i < F[j].lancados && i < N_FILA; i++) if (i != k && F[j].cru[i] == 1 && d->est[j][i] == V_VALIDO) return true;
        return false;
    }
    // Divergencia mantida: o Carrasco com 1 nao conta aqui, mas no poder_de, com 4 na face, anula tambem.
    if (ef == EF_CARRASCO && F[j].cru[k] == 1) return false;
    // Divergencia mantida: sem olhar a Couraca (o contra-ataque so acende o amuleto).
    return ataque_armado(ef, v);
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
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        if (!(agiu[j] >> k & 1)) poder_de(d, j, k, gera);
}

// O poder do dado k de j, uma vez.
// Um golpe que anula, do dado k de j no dado a0 do oponente (ou -1, sem
// alvo): o Seguranca dele pode levar no lugar, a capa do Impermeavel segura,
// e o dado atingido ataca antes de cair (contra_ataque). Depois, a Coleira,
// a Ratoeira e o Cavalo de Troia (apos_ataque).
// Como cada golpe escolhe o alvo (golpe_mira): o maior do oponente (o
// Carrasco, o Invejoso, o Agnus), o ultimo (o Laser), o primeiro (o
// Tsunami), o vizinho (o Berserker) ou o 4o (o Boneco de Neve).
enum { MIRA_MAIOR, MIRA_ULTIMO, MIRA_PRIMEIRO, MIRA_VIZINHO, MIRA_QUARTO };
typedef struct {
    const char *quem;                            // quem ataca (o texto da capa e o do Seguranca)
    const char *seg;                             // quando o Seguranca leva: com quem ataca e o rival
    const char *anula;                           // o golpe: com o numero e o rival (ou so o rival)
    bool so_rival;                               // o texto do golpe so tem o nome do rival
    uint8_t mira;                                // como escolhe o alvo (MIRA_*)
} golpe_t;
#define SEG_NO_LUGAR "%s: o Segurança de %s leva no lugar"
// Os golpes que anulam, um por poder (constantes: nao ocupam a pilha a cada
// contra-ataque).
static const golpe_t G_BERSERKER = { "Berserker", SEG_NO_LUGAR, "Berserker anula o %d de %s", false, MIRA_VIZINHO };
static const golpe_t G_CARRASCO  = { "Carrasco", SEG_NO_LUGAR, "Carrasco anula o %d de %s", false, MIRA_MAIOR };
static const golpe_t G_LASER     = { "Laser", SEG_NO_LUGAR, "Laser anula o %d de %s", false, MIRA_ULTIMO };
static const golpe_t G_TSUNAMI   = { "Tsunami", SEG_NO_LUGAR, "Tsunami leva o %d de %s", false, MIRA_PRIMEIRO };
static const golpe_t G_BONECO    = { "Boneco de Neve", SEG_NO_LUGAR, "Boneco de Neve congela o 4º de %s", true, MIRA_QUARTO };
static const golpe_t G_INVEJOSO  = { "Invejoso", SEG_NO_LUGAR, "Invejoso anula o %d de %s", false, MIRA_MAIOR };
static const golpe_t G_AGNUS     = { "Agnus", "%s: o Segurança de %s leva a luz", "Agnus: a luz anula o %d de %s", false, MIRA_MAIOR };
// O alvo do golpe g do dado k de j, com a mesa de agora (ou -1). A Couraca
// do oponente fica com quem chama (ela segura antes de qualquer alvo).
static int golpe_mira(const desfecho_t *d, int j, int k, const golpe_t *g)
{
    int r = outro(j);
    switch (g->mira) {
    case MIRA_ULTIMO:   return alvo_ultimo(d->est[r], r);
    case MIRA_PRIMEIRO: return alvo_primeiro(d->est[r], r);
    case MIRA_VIZINHO:  return alvo_vizinho(d->est[r], r, k);
    case MIRA_QUARTO:   return F[r].lancados > 3 && d->est[r][3] == V_VALIDO && !firme_sem_capa(r, 3) ? 3 : -1;
    default:            return alvo_de(d->est[r], r, true, -1);
    }
}
static void golpe_anula(desfecho_t *d, int j, int k, int a0, const golpe_t *g, bool gera)
{
    int r = outro(j);
    // O alvo que caiu no contra-ataque (a Ratoeira do dono derrubou o
    // proprio dado, por exemplo) nao pode ser anulado de novo: o golpe
    // procura outro alvo pela mesma regra (golpe_mira). Sem outro, se perde,
    // mas conta como ataque. Cada volta tira um dado valendo: acaba.
    for (int volta = 0; volta <= N_FILA; volta++) {
        int a = desvia_seguranca(d->est[r], r, a0);
        if (a < 0) { if (volta) d->atacados[r]++; return; }
        if (bloqueia_desvio(d, gera, j, k, r, a, a0, g->quem, 0)) return;
        contra_ataque(d, r, a, gera);            // o ataque dele, se tiver, vem antes
        if (d->est[r][a] != V_VALIDO) { a0 = golpe_mira(d, j, k, g); continue; }
        d->est[r][a] = V_ANULADO;
        if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a], g->seg, g->quem, J[r].nome);
        else if (g->so_rival) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a], g->anula, J[r].nome);
        else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a], g->anula, F[r].valor[a], J[r].nome);
        d->atacados[r]++;
        apos_ataque(d, j, k, r, a, gera);
        return;
    }
}

// O maior dado de r que pontua (alvo_corte: pelo numero na face, entre os
// que valem mais que 0) passa a pontuar metade, arredondando o corte para
// cima: o Sabotador e o Ranzinza. Devolve qual foi (ou -1, sem corte),
// quanto saiu e quanto ele pontua.
static int corta_metade(desfecho_t *d, int r, int *corte, int *fica)
{
    int a = alvo_corte(d->est[r], d->bonus[r], r);
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
    // Carrasco que tirou 1 (valendo ou nao): anula todos os dados de quem
    // o lancou que mostram 1 na mesa, ele mesmo incluido (carrasco_castiga).
    // O firme (o nao anulavel, o preso pela Ancora) fica; a capa segura o dela.
    if (ef == EF_CARRASCO && F[j].cru[k] == 1) {
        for (int i = 0; i < F[j].lancados && i < N_FILA; i++) {
            if (!carrasco_castiga(j, i, d->est[j][i])) continue;
            if (bloqueia_capa(d, gera, j, k, j, i, "Carrasco tirou 1", 0)) continue;
            d->est[j][i] = V_ANULADO;
            if (i == k) evento(gera, EV_ANULA, j, k, j, i, F[j].valor[i], "Carrasco tirou 1: anula a si mesmo");
            else evento(gera, EV_ANULA, j, k, j, i, F[j].valor[i], "Carrasco tirou 1: anula o 1 do próprio %s", J[j].nome);
        }
    }
    // Todo poder so age se o dado terminar valendo. O Gatuno, alem
    // disso, rouba um dado por 1 tirado por outro dado seu valendo.
    int roubos = 1;
    if (ef == EF_GATUNO) {
        roubos = 0;
        for (int i = 0; i < F[j].lancados && i < N_FILA; i++) roubos += i != k && F[j].cru[i] == 1 && d->est[j][i] == V_VALIDO;
    }
    if (d->est[j][k] != V_VALIDO || roubos <= 0) return;
    const char *nome = TIPO[tipo_de(j, k)].nome;

    // Berserker: anula o dado do oponente na casa vizinha (a de antes; sem
    // ele, a de depois). Sem vizinho no oponente (ou com a Couraca), a furia
    // cai no dono: o dado logo antes dele (sem ele, o logo depois).
    if (ef == EF_BERSERKER) {
        int ro = tem_am(r, A_COURACA) ? -1 : alvo_vizinho(d->est[r], r, k);
        if (ro >= 0) {
            golpe_anula(d, j, k, ro, &G_BERSERKER, gera);
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
    bool arma = ataque_armado(ef, v);
    // Divergencia mantida: o Berserker (la em cima) cai no dono sem acender a Couraca.
    if (couraca && gera && arma && PODER_ATAQUE[ef].golpe == GOLPE_ANULA) am_acende(A_COURACA);
    // O roubo armado (o Caridoso com 3 ou 4; o Gatuno chega aqui com algum 1) bate na Couraca.
    if (gera && couraca && ((ef == EF_CARIDOSO && arma) || ef == EF_GATUNO)) am_acende(A_COURACA);
    // Os golpes que anulam: o Carrasco (4) e o Invejoso (5 ou 6), o maior do
    // oponente; o Laser (5 ou 6), o ultimo que pode ser anulado; o Tsunami
    // (9 a 12), o primeiro; o Boneco de Neve (8), o 4o; o Agnus (8), a luz no maior.
    const golpe_t *g = !arma ? NULL : ef == EF_CARRASCO ? &G_CARRASCO : ef == EF_LASER ? &G_LASER
                     : ef == EF_TSUNAMI ? &G_TSUNAMI : ef == EF_BONECO ? &G_BONECO
                     : ef == EF_INVEJOSO ? &G_INVEJOSO : ef == EF_AGNUS ? &G_AGNUS : NULL;
    if (g) {
        golpe_anula(d, j, k, couraca ? -1 : golpe_mira(d, j, k, g), g, gera);
    } else if (ef == EF_INVEJOSO && v <= 2) {    // com 1 ou 2: os seus dois menores, ele incluido
        uint8_t ja = 0;                              // (o de capa e um dos dois: bate nela e fica)
        for (int n = 0; n < 2; n++) {
            int b = menor_valido(d->est[j], j, ja);
            if (b < 0) break;
            ja |= (uint8_t)(1 << b);
            if (bloqueia_capa(d, gera, j, k, j, b, "Invejoso", 0)) continue;
            d->est[j][b] = V_ANULADO;
            if (b == k) evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b], "Invejoso tirou %d: anula a si mesmo", v);
            else evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                        "Invejoso tirou %d: anula o %d do próprio %s", v, F[j].valor[b], J[j].nome);
        }
    } else if (ef == EF_SABOTADOR && arma) {   // o maior do oponente passa a valer metade
        int corte, fica, a = corta_metade(d, r, &corte, &fica);
        if (a >= 0) {
            d->atacados[r]++;
            evento(gera, EV_CORTA, j, k, r, a, corte, "Sabotador corta o %d de %s: pontua %d",
                   F[r].valor[a], J[r].nome, fica);
        }
    } else if (ef == EF_VAMPIRO) {               // valendo: -1 em cada dado do oponente que vale
        uint8_t m = 0;
        for (int i = 0; i < F[r].lancados && i < N_FILA; i++)
            if (d->est[r][i] == V_VALIDO && !universo(r, i)                       // so de quem pontua (como o Sabotador)
                && valor_pontos(r, i) + F[r].bonus[i] + d->bonus[r][i] > 0) {
                d->bonus[r][i] = (int8_t)(d->bonus[r][i] - 1);
                m |= (uint8_t)(1 << i);
            }
        if (m) d->atacados[r]++;
        evento_t *e = m ? evento(gera, EV_VAMPIRO, j, k, r, 0, 1, "Vampiro: -1 em cada dado de %s", J[r].nome) : NULL;
        if (e) e->mascara = m;
    } else if (ef == EF_MARTELO && arma) {     // Espinhoso: com 3 ou 4
        tira_placar(d, j, k, 4, "Espinhoso: -%d para %s", gera);
    } else if (ef == EF_MALDICAO) {              // vale para o dono e tira do oponente
        tira_placar(d, j, k, v, "Maldito: -%d para %s", gera);
    } else if (ef == EF_PIRATA && arma) {      // so tira; nao fica com os pontos
        tira_placar(d, j, k, 4, "Pirata tira %d de %s", gera);
    } else if ((ef == EF_CARIDOSO && arma) || ef == EF_GATUNO) {   // Caridoso: com 3 ou 4; Gatuno: um por 1
        for (int q = 0; q < roubos; q++) {                             // o menor, depois o menor que sobrou...
            if (d->est[j][k] != V_VALIDO) break;  // o Cavalo de Troia roubado o anulou: para de roubar
            int a0 = alvo_de(d->est[r], r, false, -1), a = desvia_seguranca(d->est[r], r, a0);
            if (a < 0) break;
            if (bloqueia_desvio(d, gera, j, k, r, a, a0, nome, 0)) continue;   // a capa (ou o Seguranca ancorado) segura
            int vale = contribui(d, r, a);       // como ele pontua agora (com o corte do Sabotador, o -1 do Vampiro)
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

// O Ranzinza (trunfo): da 5a rodada em diante, o seu maior dado que
// pontua (alvo_corte) vale metade - uma vez por rodada, depois dos poderes.
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
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        if (d->est[j][k] == V_ANULADO && TIPO[tipo_de(j, k)].efeito == EF_FENIX) {
            d->est[j][k] = V_VALIDO;
            evento(gera, EV_RENASCE, j, k, j, k, F[j].valor[k], "Fênix renasce e pontua %d", F[j].valor[k]);
        }
}

// Agnus: tirando 7 (valendo ou nao; roubado, nao), no fim todos os dados
// anulados do dono nesta rodada ressuscitam - os que a sequencia derrubou e
// os que os ataques anularam, ele mesmo incluido - e pontuam o que tiraram.
// Os roubados nao voltam. Um evento so, com todos (mascara).
// O primeiro Agnus de j com 7 que nao foi roubado (pelo estado est), ou -1.
static int agnus_armado_em(const uint8_t *est, int j)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        if (TIPO[tipo_de(j, k)].efeito == EF_AGNUS && F[j].valor[k] == AGNUS_NUMERO && est[k] != V_ROUBADO) return k;
    return -1;
}
static int agnus_armado(int j) { return agnus_armado_em(F[j].est, j); }   // a mesa de agora (a IA e a mesa)
static void agnus_de(desfecho_t *d, int j, bool gera)
{
    int ag = agnus_armado_em(d->est[j], j);       // roubado um, o outro Agnus com 7 ressuscita
    if (ag < 0) return;
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

