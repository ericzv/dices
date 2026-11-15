// Saideira (amuleto): na ultima rodada da partida (a 8a e as extras), os
// poderes do fim dos dados de j agem uma segunda vez, depois de tudo.
static bool saideira(int j) { return tem_am(j, A_SAIDEIRA) && rodada >= DES_RODADAS; }


// Funcao pura sobre copias: serve ao desfecho de verdade (gerando eventos) e
// a previa do botao "parar" de quem joga em segundo.
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
        int b = -1;
        for (int i = 0; i < F[j].lancados && i < N_FILA; i++)
            if (i != k && d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i)
                && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
        if (b < 0 || !autofagia_come_pode(j, k, b)) continue;
        if (bloqueia_capa(gera, j, k, j, b, "Autofagia", 0)) return;   // o de capa: nao e comido
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
    d_atual = d;
    if (gera) n_ev = 0;
}

// A Saideira: os poderes do fim de quem a tem agem de novo, na mesma ordem.
static void fase_saideira(desfecho_t *d, const int *ordem, bool gera)
{
    for (int o = 0; o < 2; o++) {
        int j = ordem[o], antes = n_ev;
        if (!saideira(j)) continue;
        evento_t *e = evento(gera, EV_AVISO, j, 0xFF, j, 0, 0, "Saideira: tudo de novo!");
        if (e) e->am = AM_BIT(A_SAIDEIRA);
        agiu[j] = 0;                             // os dele agem de novo; os do rival, nao
        poderes_de(d, j, gera);
        fenix_de(d, j, gera);
        agnus_de(d, j, gera);
        efeitos_do_fim(d, j, gera);
        if (e && n_ev == antes + 1) n_ev = antes;      // nada agiu de novo: nem o aviso
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
    ranzinza_corta(d, gera);
    for (int o = 0; o < 2; o++) fenix_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) agnus_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) adesivos_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) efeitos_do_fim(d, ordem[o], gera);
    fase_saideira(d, ordem, gera);
    for (int o = 0; o < 2; o++) bilhete_de(d, ordem[o], gera);   // o Bilhete Premiado, por ultimo
    fase_totais(d);
    for (int o = 0; o < 2; o++) autofagia_de(d, ordem[o], gera);   // depois da contagem: nao muda os pontos
    mesa_devolve(&rm);
}
