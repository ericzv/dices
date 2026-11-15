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

static void calcula_desfecho(desfecho_t *d, bool gera)
{
    memset(d, 0, sizeof *d);
    d_atual = d;
    if (gera) n_ev = 0;

    static fila_t f_antes[2];
    static char ver_antes[sizeof veredito];
    resolve_t rm = { .gera = gera, .f_antes = f_antes, .ver_antes = ver_antes };
    mesa_guarda(&rm);
    fase_sobras_rede(&rm);
    fase_oculto(&rm);
    fase_espelho(&rm);
    fase_anota_sorte(&rm);
    // O Terremoto valendo com 7 ou 8 sacode a mesa do oponente: o ultimo dado
    // valido dele rola de novo e a sequencia dele e refeita com o numero
    // novo (menor que o de antes, cai e leva o anterior). So no fim de
    // verdade: a previa e a IA nao sabem o que o tremor vai dar. A Couraca
    // do oponente segura a mesa dele.
    for (int o = 0; gera && o < 2; o++) {
        int j = o ? outro(primeiro) : primeiro, r = outro(j);
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!tremor_armado(j, k)) continue;
            if (tem_am(r, A_COURACA)) { am_acende(A_COURACA); continue; }
            int a = alvo_tremor(F[r].est, r);
            if (a < 0) continue;
            int antes = F[r].valor[a];
            if (!rm.pre[r]) memcpy(est_pre_espelho[r], F[r].est, N_FILA);
            rm.pre[r] = true;
            // (sem a semente do lance - o Camaleao atras de um Terremoto, o
            // Terremoto que ja veio grudado pelo Chiclete -, a da mesa, como a Casca)
            treme_dado(r, a, F[j].abalo[k] ? F[j].abalo[k] : pega_semente(j, k));
            refaz_sequencia(r, F[r].lancados);
            evento_t *e;
            if (F[r].valor[a] == antes) e = evento(gera, EV_TREMOR, j, k, r, a, antes, "Terremoto: o %d de %s treme e fica %d", antes, J[r].nome, antes);
            else e = evento(gera, EV_TREMOR, j, k, r, a, F[r].valor[a], "Terremoto: o %d de %s rola de novo: %d", antes, J[r].nome, F[r].valor[a]);
            if (e) e->antes = antes;
            guarda_seq(e, r);
            rm.virou = true;
        }
    }
    // A Casca de Banana de quem jogou depois (a de quem abriu age no lance,
    // partida.inc): o oponente ja tinha jogado, e o 4o dado dele (com mais
    // cascas, o 3o, o 2o, o 1o) rola de novo e fica o menor dos dois - menor
    // que o de antes, a sequencia derruba os dois. Sem esse dado (ou ja
    // anulado), nada. Como o tremor, so no fim de verdade (a previa e a IA nao
    // sabem o que vai dar), pelo sorteio que vem da mesa. A Couraca e a bolha
    // do Impermeavel seguram.
    if (gera) {
        int r = outro(primeiro), t = primeiro;   // r: o dono das cascas; t: quem escorrega
        int nc = cascas_de(r);
        for (int n = 0; n < nc; n++) {
            int kc = casca_dado(r, n), a = CASCA_DADO + 1 - nc + n;
            if (a >= F[t].lancados || F[t].est[a] != V_VALIDO || universo(t, a)) continue;
            if (tem_am(t, A_COURACA)) { am_acende(A_COURACA); continue; }
            int antes = F[t].valor[a];
            evento_t *e;
            d->atacados[t]++;
            if (!rm.pre[t]) memcpy(est_pre_espelho[t], F[t].est, N_FILA);
            rm.pre[t] = true;
            if (impermeavel(t, a)) {
                e = evento(gera, EV_TREMOR, r, kc, t, a, antes, "Casca de Banana: o %d de %s está protegido", antes, J[t].nome);
                if (e) { e->antes = antes; e->ad = AD_CASCA; e->bloqueado = (uint8_t)(1 << a); }
                guarda_seq(e, t);
                continue;
            }
            static fila_t guarda;
            guarda = F[t];
            treme_dado(t, a, pega_semente(r, kc) ^ (0x5CA1AB1Eu * (uint32_t)(n + 1)));
            bool fica = F[t].valor[a] >= antes;
            if (fica) F[t] = guarda;                 // caiu de novo no mesmo, ou maior: fica o de antes
            else refaz_sequencia(t, F[t].lancados);
            if (fica) e = evento(gera, EV_TREMOR, r, kc, t, a, antes, "Casca de Banana: o %d de %s escorrega e fica", antes, J[t].nome);
            else e = evento(gera, EV_TREMOR, r, kc, t, a, F[t].valor[a], "Casca de Banana: o %d de %s escorrega: %d", antes, J[t].nome, F[t].valor[a]);
            if (e) { e->antes = antes; e->ad = AD_CASCA; }
            guarda_seq(e, t);
            rm.virou = true;
        }
    }
    // A Ficha Coringa vira: passa a ter o numero do ultimo dado valido do
    // oponente (depois do tremor) e entra na sequencia do dono com ele - ai
    // pode cair, ou derrubar o seguinte. Sem nenhum valido do oponente, ela e
    // anulada. As dos dois lados viram juntas, cada uma olhando a mesa do
    // outro antes de qualquer uma virar. Na previa tambem (nada ao acaso), e
    // a mesa volta no fim.
    int c_alvo[2], c_val[2];
    for (int j = 0; j < 2; j++) {
        c_alvo[j] = alvo_curinga(j);
        c_val[j] = c_alvo[j] >= 0 ? F[outro(j)].valor[c_alvo[j]] : 0;
    }
    for (int j = 0; j < 2; j++) {
        if (!tem_curinga(j)) continue;
        if (gera && !rm.pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        rm.pre[j] = true;
        int ev0 = n_ev;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!F[j].curinga[k]) continue;
            F[j].curinga[k] = false;
            if (c_alvo[j] < 0) {
                F[j].curinga_nada[k] = true;
                evento(gera, EV_CORINGA, j, k, outro(j), 0xFF, 0, "Ficha Coringa: %s não tem dado válido, anulada", J[outro(j)].nome);
                continue;
            }
            int v = c_val[j] + (F[j].calibre[k] ? 1 : 0);
            F[j].valor[k] = v;
            lado_vira(j, k, v);
            evento(gera, EV_CORINGA, j, k, outro(j), c_alvo[j], v, "Ficha Coringa vira o %d de %s", c_val[j], J[outro(j)].nome);
        }
        refaz_sequencia(j, F[j].lancados);
        if (gera) guarda_seq_de(ev0, j, EV_CORINGA, EV_CORINGA);
        rm.virou = true;
    }
    // O Carrossel para: vale quantos dados valem na mesa agora, dos dois
    // lados, depois de tudo o que rm.virou acima (o Oculto, o Espelho, o tremor,
    // a Casca, a Ficha Coringa) e antes dos ataques do fim. Ele conta, e os
    // outros Carrosseis que ainda giram tambem (de 1 a 8; com o Calibre
    // antes, +1, ate o 8). Entra na sequencia do dono com ele - pode cair, ou
    // derrubar o seguinte. Com mais de um, param um de cada vez (os de quem
    // abriu a rodada primeiro), cada um no numero da mesa naquela hora. Na
    // previa tambem (nada ao acaso), e a mesa volta no fim.
    for (int o = 0; o < 2; o++) {
        int j = o ? outro(primeiro) : primeiro;
        if (!tem_gira(j)) continue;
        if (gera && !rm.pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        rm.pre[j] = true;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!F[j].gira[k]) continue;
            int roda = carrossel_conta();
            F[j].gira[k] = false;
            int v = roda + (F[j].calibre[k] && roda < CARROSSEL_MAX ? 1 : 0);
            F[j].valor[k] = v;
            lado_vira(j, k, v);
            refaz_sequencia(j, F[j].lancados);
            evento_t *e;
            if (v != roda) e = evento(gera, EV_CARROSSEL, j, k, j, k, v, "Carrossel para no %d: %d válidos e o Calibre", v, roda);
            else if (v == 1) e = evento(gera, EV_CARROSSEL, j, k, j, k, v, "Carrossel para no 1: só ele vale na mesa");
            else e = evento(gera, EV_CARROSSEL, j, k, j, k, v, "Carrossel para no %d: dados válidos na mesa", v);
            guarda_seq(e, j);
        }
        rm.virou = true;
    }
    fase_espelho_tardio(&rm);
    mesa_devolve_veredito(&rm);
    fase_fotografa(d);

    int ordem[2] = { primeiro, outro(primeiro) };
    agiu[0] = agiu[1] = 0;
    for (int o = 0; o < 2; o++) poderes_de(d, ordem[o], gera);
    ranzinza_corta(d, gera);
    for (int o = 0; o < 2; o++) fenix_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) agnus_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) adesivos_de(d, ordem[o], gera);
    for (int o = 0; o < 2; o++) efeitos_do_fim(d, ordem[o], gera);
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
    for (int o = 0; o < 2; o++) bilhete_de(d, ordem[o], gera);   // o Bilhete Premiado, por ultimo
    for (int j = 0; j < 2; j++) {
        int t = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j]
              + d->delta[j] + d->semente[j];
        d->bruto[j] = t;
        d->total[j] = t < 0 ? 0 : t;
    }
    for (int o = 0; o < 2; o++) autofagia_de(d, ordem[o], gera);   // depois da contagem: nao muda os pontos
    if (!gera && rm.virou) memcpy(F, f_antes, sizeof F);   // previa: o Espelho segue esperando
    for (int j = 0; !gera && j < 2; j++) F[j].n = f_antes[j].n;
}

