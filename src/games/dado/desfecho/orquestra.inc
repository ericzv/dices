// Saideira (amuleto): na ultima rodada da partida (a 8a e as extras), os
// poderes do fim dos dados de j agem uma segunda vez, depois de tudo.
static bool saideira(int j) { return tem_am(j, A_SAIDEIRA) && rodada >= DES_RODADAS; }

// A sequencia de j acabou de ser refeita por causa do evento e (ou dos
// eventos do tipo t1 ou t2 de j, de 'de' em diante): cada um guarda como ela
// ficou nessa hora. A mesa mostra isso quando o evento chega - e nao o fim de
// tudo, que ainda depende dos que viram depois (o Carrossel conta os validos
// da mesa de agora).
static void guarda_seq(evento_t *e, int j)
{
    if (!e) return;
    e->tem_seq = true;
    memcpy(e->seq, F[j].est, N_FILA);
}
static void guarda_seq_de(int de, int j, int t1, int t2)
{
    for (int i = de; i < n_ev; i++)
        if (ev[i].de_j == j && (ev[i].tipo == t1 || ev[i].tipo == t2)) guarda_seq(&ev[i], j);
}

// A previa nao sabe o que o tremor e a Casca de Banana vao dar, mas sabe se
// vao agir: e o que ela anota aqui, com o Espelho que esperava ja virado e a
// sequencia dele refeita (fim_incerto, partida.inc).
static bool previa_sorte;
static bool casca_no_fim(void);

// Funcao pura sobre copias: serve ao desfecho de verdade (gerando eventos) e
// a previa do botao "parar" de quem joga em segundo.
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

    // Espelho que esperava o oponente (com "?"): vira agora o maior dado dele,
    // e a sequencia do dono e refeita com o numero novo - so agora ele pode
    // cair ou derrubar outro. Enquanto esperava, nada o anulava (firme).
    // No desfecho de verdade fica assim; na previa, a mesa volta como estava.
    static fila_t f_antes[2];
    static char ver_antes[sizeof veredito];
    int vt = ver_tipo, aa = anulou_a, ab = anulou_b, res = resistiu;
    bool vc = ver_comum;
    uint8_t am = anulou_masc;
    bool virou = false;
    memcpy(f_antes, F, sizeof F);
    memcpy(ver_antes, veredito, sizeof veredito);
    // A previa conta a rodada como se acabasse agora: os dados da sequencia
    // ainda por lancar ja teriam saido da mesa, e a Rede passa a valer no
    // ultimo dado lancado, com a sequencia refeita (descarta_sobras). A mesa
    // volta no fim.
    for (int j = 0; !gera && j < 2; j++)
        if (F[j].n > F[j].lancados) {
            F[j].n = F[j].lancados;
            if (tem_am(j, A_REDE)) { refaz_sequencia(j, F[j].lancados); virou = true; }
        }
    // O Oculto mostra o que tirou (so no desfecho de verdade: a previa nao
    // sabe, e a IA imagina o numero antes): a sequencia e refeita com ele.
    uint8_t revelados[2] = { 0, 0 };
    for (int j = 0; gera && j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
            if (F[j].escondido[k]) {
                F[j].escondido[k] = false;
                revelados[j] |= (uint8_t)(1 << k);
                evento(gera, EV_REVELA, j, k, j, k, F[j].valor[k], "Oculto: tirou %d", F[j].valor[k]);
            }
    bool pre[2] = { false, false };              // a sequencia de antes de cada um ja guardada (animacao)
    for (int j = 0; j < 2; j++) {
        int alvo = maior_do_rival(j), virados = 0;
        uint8_t quais = 0;
        for (int k = 0; k < F[j].lancados; k++) {
            if (!F[j].espera[k] || alvo <= 0) continue;
            F[j].valor[k] = alvo;
            lado_vira(j, k, alvo);
            F[j].espera[k] = false;
            quais |= (uint8_t)(1 << k);
            virados++;
            evento(gera, EV_ESPELHO, j, k, outro(j), 0, alvo, "Espelho vira o %d de %s", alvo, J[outro(j)].nome);
        }
        if (!virados && !revelados[j]) continue;
        if (gera) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        pre[j] = true;
        refaz_sequencia(j, F[j].lancados);
        for (int k = 0; k < F[j].lancados; k++)
            if (quais >> k & 1) poe_tag(&F[j], k, "=%d", alvo);
        if (gera) guarda_seq_de(0, j, EV_REVELA, EV_ESPELHO);
        virou = true;
    }
    if (!gera) previa_sorte = tremor_na_mesa() || casca_no_fim();
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
            if (!pre[r]) memcpy(est_pre_espelho[r], F[r].est, N_FILA);
            pre[r] = true;
            // (sem a semente do lance - o Camaleao atras de um Terremoto, o
            // Terremoto que ja veio grudado pelo Chiclete -, a da mesa, como a Casca)
            treme_dado(r, a, F[j].abalo[k] ? F[j].abalo[k] : pega_semente(j, k));
            refaz_sequencia(r, F[r].lancados);
            evento_t *e;
            if (F[r].valor[a] == antes) e = evento(gera, EV_TREMOR, j, k, r, a, antes, "Terremoto: o %d de %s treme e fica %d", antes, J[r].nome, antes);
            else e = evento(gera, EV_TREMOR, j, k, r, a, F[r].valor[a], "Terremoto: o %d de %s rola de novo: %d", antes, J[r].nome, F[r].valor[a]);
            if (e) e->antes = antes;
            guarda_seq(e, r);
            virou = true;
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
            if (!pre[t]) memcpy(est_pre_espelho[t], F[t].est, N_FILA);
            pre[t] = true;
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
            virou = true;
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
        if (gera && !pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        pre[j] = true;
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
        virou = true;
    }
    // O Carrossel para: vale quantos dados valem na mesa agora, dos dois
    // lados, depois de tudo o que virou acima (o Oculto, o Espelho, o tremor,
    // a Casca, a Ficha Coringa) e antes dos ataques do fim. Ele conta, e os
    // outros Carrosseis que ainda giram tambem (de 1 a 8; com o Calibre
    // antes, +1, ate o 8). Entra na sequencia do dono com ele - pode cair, ou
    // derrubar o seguinte. Com mais de um, param um de cada vez (os de quem
    // abriu a rodada primeiro), cada um no numero da mesa naquela hora. Na
    // previa tambem (nada ao acaso), e a mesa volta no fim.
    for (int o = 0; o < 2; o++) {
        int j = o ? outro(primeiro) : primeiro;
        if (!tem_gira(j)) continue;
        if (gera && !pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        pre[j] = true;
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
        virou = true;
    }
    // O Espelho que nao achou o que copiar no comeco (o oponente so com o
    // Universo, o Vazio, ou com a Ficha Coringa e o Carrossel, que so viraram
    // agora): vira aqui o maior do oponente, como la em cima. Se nem assim ha
    // o que copiar, e anulado (como a Ficha Coringa sem alvo), a sequencia e
    // refeita sem ele e a marca "=?" sai da mesa.
    for (int j = 0; j < 2; j++) {
        int alvo = maior_do_rival(j), ev0 = n_ev;
        uint8_t quais = 0, nada = 0;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!F[j].espera[k]) continue;
            F[j].espera[k] = false;
            virou = true;
            if (alvo <= 0) {
                if (!strcmp(F[j].tag[k], "=?")) F[j].tag[k][0] = 0;
                if (!strcmp(F[j].tag_num[k], "=?")) F[j].tag_num[k][0] = 0;
                F[j].espelho_nada[k] = true;
                nada |= (uint8_t)(1 << k);
                evento(gera, EV_ESPELHO, j, k, outro(j), 0xFF, 0, "Espelho: %s não tem o que copiar, anulado", J[outro(j)].nome);
                continue;
            }
            F[j].valor[k] = alvo;
            lado_vira(j, k, alvo);
            quais |= (uint8_t)(1 << k);
            evento(gera, EV_ESPELHO, j, k, outro(j), 0, alvo, "Espelho vira o %d de %s", alvo, J[outro(j)].nome);
        }
        if (!quais && !nada) continue;
        if (gera && !pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        pre[j] = true;
        refaz_sequencia(j, F[j].lancados);
        for (int k = 0; k < F[j].lancados; k++)
            if (quais >> k & 1) poe_tag(&F[j], k, "=%d", alvo);
        if (gera) guarda_seq_de(ev0, j, EV_ESPELHO, EV_ESPELHO);
    }
    memcpy(veredito, ver_antes, sizeof veredito);
    ver_tipo = vt; anulou_a = aa; anulou_b = ab; resistiu = res; anulou_masc = am; ver_comum = vc;
    for (int j = 0; j < 2; j++) memcpy(d->est[j], F[j].est, N_FILA);

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
    if (!gera && virou) memcpy(F, f_antes, sizeof F);   // previa: o Espelho segue esperando
    for (int j = 0; !gera && j < 2; j++) F[j].n = f_antes[j].n;
}

