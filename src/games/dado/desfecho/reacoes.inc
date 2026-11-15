// ===========================================================================
// REACOES: o que o dado atingido faz de volta - a Coleira cobra, a Ratoeira
// desarma, o Cavalo de Troia se abre (apos_ataque).
// Le: F, d (o estado dos dados, o que ja foi roubado), J[].nome e os
// amuletos (a Couraca).
// Grava: d (est, atacados, delta, roubo_val, ratoeira, rato_bits,
// coleira_bits) e ev[]; com gera, acende a Couraca. Nao grava F nem a bolsa.
// Quem chama: poder_de (poderes.inc) e a Mira (adesivos_fim.inc), depois de
// cada ataque que pega.
// ===========================================================================

static void apos_ataque(desfecho_t *d, int j, int k, int r, int a, bool gera);
// A Ratoeira (posicao a de r, com o lado dela para cima) foi anulada por um
// poder de j: desarma e anula o maior dado valendo de j (o Seguranca dele
// leva no lugar; a Couraca protege). Uma vez por dado e por rodada.
static void ratoeira_desarma(desfecho_t *d, int j, int r, int a, bool gera)
{
    uint8_t bit = (uint8_t)(1 << a);
    if (d->rato_bits[r] & bit) return;
    d->rato_bits[r] |= bit;
    // ACHADO A32: preservado - conta como adesivo que agiu mesmo sem pegar ninguem (nao reproduz diferenca no placar)
    d->ratoeira[r]++;
    if (tem_am(j, A_COURACA)) {
        if (gera) am_acende(A_COURACA);
        return;
    }
    int b = desvia_seguranca(d->est[j], j, maior_valido(d->est[j], j, 0));
    if (b < 0 || bloqueia_capa(d, gera, r, a, j, b, "Ratoeira", AD_RATOEIRA)) return;
    d->est[j][b] = V_ANULADO;
    d->atacados[j]++;
    evento_t *e = evento(gera, EV_ADESIVO, r, a, j, b, F[j].valor[b], "Ratoeira: anula o %d de %s", F[j].valor[b], J[j].nome);
    if (e) e->ad = AD_RATOEIRA;
    apos_ataque(d, r, a, j, b, gera);               // o dado dele que caiu pode ser um Cavalo
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
// apos_ataque roda depois de cada ataque do dado k de j que pegou o dado a
// de r: a Coleira cobra, a Ratoeira desarma e, por ultimo, o Cavalo se abre.
// Cavalo de Troia: o dado a de r acabou de ser anulado ou roubado por um
// poder de j. Se for o Cavalo, ele se abre: os dados de j que valem caem
// (Escudo e Teimoso resistem; a Couraca de j segura tudo), e o que j roubou
// nesta rodada, o proprio Cavalo inclusive, deixa de contar para ele.
static void apos_ataque(desfecho_t *d, int j, int k, int r, int a, bool gera)
{
    if (a >= 0 && a < N_FILA && (d->est[r][a] == V_ANULADO || d->est[r][a] == V_ROUBADO)) coleira_cobra(d, j, k, r, a, gera);
    if (a >= 0 && a < N_FILA && d->est[r][a] == V_ANULADO && ad_fila_tem(r, a, AD_RATOEIRA)) ratoeira_desarma(d, j, r, a, gera);
    // ACHADO A34: preservado - sem a guarda de a das duas linhas acima (todos os chamadores passam a >= 0)
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
            perde[i] = (int8_t)contribui(d, j, i);
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

