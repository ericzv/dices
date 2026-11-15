// Cavalo de Troia: o dado a de r acabou de ser anulado ou roubado por um
// poder de j. Se for o Cavalo, ele se abre: os dados de j que valem caem
// (Escudo e Teimoso resistem; a Couraca de j segura tudo), e o que j roubou
// nesta rodada, o proprio Cavalo inclusive, deixa de contar para ele.
// A Ratoeira (posicao a de r, com o lado dela para cima) foi anulada por um
// poder de j: desarma e anula o maior dado valendo de j (o Seguranca dele
// leva no lugar; a Couraca protege). Uma vez por dado e por rodada.
static void cavalo_abre(desfecho_t *d, int j, int k, int r, int a, bool gera);
static void ratoeira_desarma(desfecho_t *d, int j, int r, int a, bool gera)
{
    uint8_t bit = (uint8_t)(1 << a);
    if (d->rato_bits[r] & bit) return;
    d->rato_bits[r] |= bit;
    d->ratoeira[r]++;
    if (tem_am(j, A_COURACA)) {
        if (gera) am_acende(A_COURACA);
        return;
    }
    int b = -1;
    for (int i = 0; i < F[j].lancados && i < N_FILA; i++)
        if (d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i) && (b < 0 || F[j].valor[i] > F[j].valor[b])) b = i;
    b = desvia_seguranca(d->est[j], j, b);
    if (b < 0 || bloqueia_capa(gera, r, a, j, b, "Ratoeira", AD_RATOEIRA)) return;
    d->est[j][b] = V_ANULADO;
    d->atacados[j]++;
    evento_t *e = evento(gera, EV_ADESIVO, r, a, j, b, F[j].valor[b], "Ratoeira: anula o %d de %s", F[j].valor[b], J[j].nome);
    if (e) e->ad = AD_RATOEIRA;
    cavalo_abre(d, r, a, j, b, gera);               // o dado dele que caiu pode ser um Cavalo
}

// A Coleira: o dado dela (no lado de cima) foi roubado ou anulado por um
// ataque do oponente j (o dado k dele). O ataque vale; quem fez perde
// COLEIRA_PERDE no placar - uma vez por dado.
static void coleira_cobra(desfecho_t *d, int j, int k, int r, int a, bool gera)
{
    uint8_t bit = (uint8_t)(1 << a);
    if (j == r || !ad_fila_tem(r, a, AD_COLEIRA) || (d->coleira_bits[r] & bit)) return;
    d->coleira_bits[r] |= bit;
    d->delta[j] -= COLEIRA_PERDE;
    evento_t *e = evento(gera, EV_ADESIVO, r, a, j, k, COLEIRA_PERDE, "Coleira: %s %s e perde %d", J[j].nome,
                         d->est[r][a] == V_ROUBADO ? "rouba" : "anula", COLEIRA_PERDE);
    if (e) e->ad = AD_COLEIRA;
}
// Depois de um ataque do dado k de j que pegou o dado a de r: a Coleira cobra,
// a Ratoeira desarma, o Cavalo de Troia abre.
static void cavalo_abre(desfecho_t *d, int j, int k, int r, int a, bool gera)
{
    if (a >= 0 && a < N_FILA && (d->est[r][a] == V_ANULADO || d->est[r][a] == V_ROUBADO)) coleira_cobra(d, j, k, r, a, gera);
    if (a >= 0 && a < N_FILA && d->est[r][a] == V_ANULADO && ad_fila_tem(r, a, AD_RATOEIRA)) ratoeira_desarma(d, j, r, a, gera);
    if (TIPO[tipo_de(r, a)].efeito != EF_CAVALO) return;
    if (tem_am(j, A_COURACA)) {
        if (gera) am_acende(A_COURACA);
        return;
    }
    uint8_t mask = 0, capa = 0;
    int8_t perde[N_FILA] = { 0 };
    for (int i = 0; i < F[j].lancados; i++)
        if (d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i)) {
            if (impermeavel(j, i)) { capa |= (uint8_t)(1 << i); continue; }   // bate na capa
            perde[i] = (int8_t)contribui(j, i);
            d->est[j][i] = V_ANULADO;
            mask |= (uint8_t)(1 << i);
            d->atacados[j]++;
        }
    int roubado = d->roubo_val[j];
    d->roubo_val[j] = 0;
    if (!mask && !roubado && !capa) return;
    evento_t *e = evento(gera, EV_CAVALO, r, a, j, 0, roubado, "Cavalo de Troia: anula os de %s", J[j].nome);
    if (e) { e->mascara = mask; memcpy(e->por_dado, perde, sizeof perde); e->bloqueado = capa; }
}

