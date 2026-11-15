// ===========================================================================
// ORQUESTRA: calcula_desfecho, a entrada do desfecho - chama as fases em
// ordem. E as que so ela usa: a Saideira, o Ourico, os totais e a Autofagia.
// Le: F, d, J[] (a bolsa, so para ler: a Autofagia conta os dados em jogo),
// quem abriu a rodada (primeiro), a rodada e os amuletos.
// Grava: d inteiro (zera, e cada fase preenche a sua parte; aqui, come,
// come_de, bruto e total), ev[]/n_ev (so com gera) e agiu[]. F so
// pelas fases de resolve_mesa.inc; na previa, ela volta.
// Quem chama: a partida (uma vez com gera, e as previas), a IA, a mesa, a
// hora da verdade, as telas e as falas do Desafiante.
// ===========================================================================

// Saideira (amuleto): na ultima rodada da partida (a 8a e as extras), os
// poderes do fim dos dados de j agem uma segunda vez, depois de tudo.
static bool saideira(int j) { return tem_am(j, A_SAIDEIRA) && rodada >= DES_RODADAS; }


// A Autofagia (posicao a da fila de j) pode comer o dado b? So com os dois
// vindos da bolsa e com pelo menos 8 dados em jogo.
static bool autofagia_come_pode(int j, int a, int b)
{
    const jogador_t *p = &J[j];
    int ia = F[j].idx[a], ib = F[j].idx[b];
    if (ia < 0 || ia >= p->n_col || ib < 0 || ib >= p->n_col || ia == ib) return false;
    if (p->col[ia].tipo != D_AUTOFAGIA) return false;
    int vivos = 0;
    for (int i = 0; i < p->n_col; i++) vivos += p->onde[i] != COMIDO;
    return vivos >= AUTO_BOLSA;
}

// Autofagia: depois da contagem, a que terminou valendo come o outro dado
// seu de menor numero que vale (no empate, o primeiro). Ele ja pontuou nesta
// rodada: nao cai, mas sai da bolsa ate o fim da partida, e o numero dele
// fica guardado na Autofagia (autofagia_come), que o pontua quando tirar 8.
// Escudo e Teimoso nao sao comidos (vai o seguinte); o de capa se protege.
// Com menos de 8 dados em jogo na bolsa, ela nao come. Uma refeicao por
// rodada para cada jogador.
static void autofagia_de(desfecho_t *d, int j, bool gera)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA && !d->come[j]; k++) {
        if (TIPO[tipo_de(j, k)].efeito != EF_AUTOFAGIA || d->est[j][k] != V_VALIDO) continue;
        int b = menor_valido(d->est[j], j, (uint8_t)(1 << k));
        if (b < 0 || !autofagia_come_pode(j, k, b)) continue;
        if (bloqueia_capa(d, gera, j, k, j, b, "Autofagia", 0)) return;   // o de capa: nao e comido
        d->come[j] = (uint8_t)(b + 1);
        d->come_de[j] = (uint8_t)(k + 1);
        int guarda = F[j].valor[b] > 0 ? F[j].valor[b] : 0;
        evento(gera, EV_COME, j, k, j, b, guarda, "Autofagia come o %d do próprio %s: guarda +%d",
               F[j].valor[b], J[j].nome, guarda);
    }
}

// Comeca o desfecho do zero (e a lista de eventos, quando gera).
static void desfecho_prepara(desfecho_t *d, bool gera)
{
    memset(d, 0, sizeof *d);
    if (gera) n_ev = 0;
    miras_zera();
}

// A Saideira: os poderes do fim de quem a tem agem de novo, na mesma ordem -
// em duas partes, para nada ser pago sobre a mesa velha: os poderes (que
// anulam, roubam e cortam) logo depois da primeira passada, antes de tudo o
// que conta pontos; os efeitos do fim, depois dos efeitos do fim dos dois.
// O aviso sai uma vez, na primeira parte que fizer algo.
static bool saideira_avisou[2];
static evento_t *saideira_aviso(int j, bool gera)
{
    evento_t *e = evento(gera, EV_AVISO, j, 0xFF, j, 0, 0, "Saideira: tudo de novo!");
    if (e) e->am = AM_BIT(A_SAIDEIRA);
    return e;
}
static void fase_saideira_poderes(desfecho_t *d, const int *ordem, bool gera)
{
    for (int o = 0; o < 2; o++) {
        int j = ordem[o], antes = n_ev;
        saideira_avisou[j] = false;
        if (!saideira(j)) continue;
        evento_t *e = saideira_aviso(j, gera);
        agiu[j] = 0;                             // os dele agem de novo; os do rival, nao
        poderes_de(d, j, gera);
        if (e && n_ev == antes + 1) n_ev = antes;      // nada agiu de novo: o aviso fica para os efeitos do fim
        else saideira_avisou[j] = true;
    }
}
static void fase_saideira_fim(desfecho_t *d, const int *ordem, bool gera)
{
    for (int o = 0; o < 2; o++) {
        int j = ordem[o], antes = n_ev;
        if (!saideira(j)) continue;
        evento_t *e = saideira_avisou[j] ? NULL : saideira_aviso(j, gera);
        efeitos_do_fim(d, j, gera);
        if (e && n_ev == antes + 1) n_ev = antes;      // nada agiu de novo: nem o aviso
    }
}

// O Ourico na fila (valido ou anulado; roubado, nao): cada tentativa do
// oponente de anular, atacar ou tirar pontos seus na rodada custa a ele
// OURICO_PTS - ate a que a bolha do Impermeavel segurou. Cobra depois dos
// efeitos do fim dos dois (a Assombracao do rival tambem conta, abra quem
// abrir), uma vez so; com a Saideira do dono, em dobro (tudo de novo).
static void ourico_de(desfecho_t *d, int j, bool gera)
{
    int r = outro(j), n = d->atacados[j], vezes = saideira(j) ? 2 : 1;
    if (!n) return;
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        if (d->est[j][k] == V_ROUBADO || TIPO[tipo_de(j, k)].efeito != EF_OURICO) continue;
        int perde = OURICO_PTS * n * vezes;
        d->delta[r] -= perde;
        evento_t *e = vezes > 1
            ? evento(gera, EV_TIRA, j, k, r, 0, perde, "Ouriço: %d ataque%s, duas vezes, -%d para %s", n, n > 1 ? "s" : "",
                     perde, J[r].nome)
            : evento(gera, EV_TIRA, j, k, r, 0, perde, "Ouriço: %d ataque%s, -%d para %s", n, n > 1 ? "s" : "",
                     perde, J[r].nome);
        if (e && vezes > 1) e->am = AM_BIT(A_SAIDEIRA);
    }
}

// Os totais: os pontos da sequencia, os roubos, o que tiraram e a Semente;
// o placar nunca fica abaixo de 0 (o bruto guarda o numero de verdade).
static void fase_totais(desfecho_t *d)
{
    for (int j = 0; j < 2; j++) {
        int t = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j]
              + d->delta[j] + d->semente[j];
        d->bruto[j] = t;
        d->total[j] = t < 0 ? 0 : t;
    }
}

// O fim da rodada inteiro, fase por fase. Com gera, e o desfecho de verdade:
// gera os eventos, e a mesa fica como as fases a deixaram. Sem gera, e a
// previa: nao gera eventos e a mesa volta como estava (as fases mexem na
// mesa de verdade, nao numa copia; mesa_devolve a repoe). A previa serve ao
// botao "parar", a IA e as dicas da mesa.
static void calcula_desfecho(desfecho_t *d, bool gera)
{
    desfecho_prepara(d, gera);

    // A mesa: o que muda nos dados antes dos poderes (resolve_mesa.inc).
    static fila_t f_antes[2];
    static char ver_antes[sizeof veredito];
    resolve_t rm = { .gera = gera, .f_antes = f_antes, .ver_antes = ver_antes };
    mesa_guarda(&rm);
    fase_sobras_rede(&rm);
    fase_oculto(&rm);
    fase_espelho(&rm);
    fase_anota_sorte(&rm);
    fase_terremoto(&rm);
    fase_casca(&rm, d);
    fase_coringa(&rm);
    fase_carrossel(&rm);
    fase_espelho_tardio(&rm);
    mesa_devolve_veredito(&rm);
    fase_fotografa(d);

    // Os poderes e os adesivos do fim, de quem abriu a rodada e depois do outro.
    int ordem[2] = { primeiro, outro(primeiro) };
    agiu[0] = agiu[1] = 0;
    for (int o = 0; o < 2; o++) poderes_de(d, ordem[o], gera);
    fase_saideira_poderes(d, ordem, gera);       // a Saideira: os poderes de novo, antes de contar pontos
    ranzinza_corta(d, gera);
    for (int o = 0; o < 2; o++) miras_de(d, ordem[o], gera);      // as Miras, antes dos adesivos que pagam
    // A Fenix e o Agnus 7 depois de tudo o que anula - os poderes e as Miras -
    // e de novo depois dos adesivos (uma Mira copiada pelo Holografico, pelo
    // Papagaio ou pelo Porta-Retrato ainda anula ali).
    for (int o = 0; o < 2; o++) fenix_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) agnus_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) adesivos_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) fenix_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) agnus_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) efeitos_do_fim(d, ordem[o], gera);
    fase_saideira_fim(d, ordem, gera);           // a Saideira: os efeitos do fim de novo
    for (int o = 0; o < 2; o++) ourico_de(d, ordem[o], gera);   // depois de todos os ataques
    for (int o = 0; o < 2; o++) bilhete_de(d, ordem[o], gera);   // o Bilhete Premiado, por ultimo
    fase_totais(d);
    for (int o = 0; o < 2; o++) autofagia_de(d, ordem[o], gera);   // depois da contagem: nao muda os pontos
    mesa_devolve(&rm);
}
