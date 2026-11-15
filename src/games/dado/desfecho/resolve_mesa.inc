// As fases da mesa: o que muda nos dados antes dos poderes do fim (o corte
// da previa, o Oculto, o Espelho, o tremor, a Casca, a Ficha Coringa e o
// Carrossel), na ordem em que calcula_desfecho chama. Sao as unicas fases
// que gravam em F; na previa, a mesa e o veredito voltam (mesa_devolve).

// A previa nao sabe o que o tremor e a Casca de Banana vao dar, mas sabe se
// vao agir: e o que ela anota aqui, com o Espelho que esperava ja virado e a
// sequencia dele refeita (fim_incerto, partida.inc).
static bool previa_sorte;
static bool casca_no_fim(void);

// O que as fases da mesa dividem numa chamada de calcula_desfecho.
typedef struct {
    bool gera;                                // o desfecho de verdade (gera eventos), ou a previa
    bool virou;                               // alguma fase mexeu na mesa: a previa a devolve no fim
    bool pre[2];                              // a sequencia de antes de cada um ja guardada (animacao)
    uint8_t revelados[2];                     // os Ocultos revelados agora, um bit por lugar
    fila_t *f_antes;                          // a mesa como estava
    char *ver_antes;                          // e o veredito da sequencia
    int vt, aa, ab, res;                      // ver_tipo, anulou_a, anulou_b, resistiu
    bool vc;                                  // ver_comum
    uint8_t am;                               // anulou_masc
} resolve_t;

// Guarda a mesa e o veredito da sequencia, que as fases refazem.
static void mesa_guarda(resolve_t *rm)
{
    rm->vt = ver_tipo; rm->aa = anulou_a; rm->ab = anulou_b; rm->res = resistiu;
    rm->vc = ver_comum;
    rm->am = anulou_masc;
    memcpy(rm->f_antes, F, sizeof F);
    memcpy(rm->ver_antes, veredito, sizeof veredito);
}

// A previa conta a rodada como se acabasse agora: os dados da sequencia
// ainda por lancar ja teriam saido da mesa, e a Rede passa a valer no
// ultimo dado lancado, com a sequencia refeita (descarta_sobras). A mesa
// volta no fim.
static void fase_sobras_rede(resolve_t *rm)
{
    for (int j = 0; !rm->gera && j < 2; j++)
        if (F[j].n > F[j].lancados) {
            F[j].n = F[j].lancados;
            if (tem_am(j, A_REDE)) { refaz_sequencia(j, F[j].lancados); rm->virou = true; }
        }
}

// O Oculto mostra o que tirou (so no desfecho de verdade: a previa nao
// sabe, e a IA imagina o numero antes): a sequencia e refeita com ele.
static void fase_oculto(resolve_t *rm)
{
    for (int j = 0; rm->gera && j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
            if (F[j].escondido[k]) {
                F[j].escondido[k] = false;
                rm->revelados[j] |= (uint8_t)(1 << k);
                evento(rm->gera, EV_REVELA, j, k, j, k, F[j].valor[k], "Oculto: tirou %d", F[j].valor[k]);
            }
}

// Espelho que esperava o oponente (com "?"): vira agora o maior dado dele,
// e a sequencia do dono e refeita com o numero novo - so agora ele pode
// cair ou derrubar outro. Enquanto esperava, nada o anulava (firme).
// No desfecho de verdade fica assim; na previa, a mesa volta como estava.
static void fase_espelho(resolve_t *rm)
{
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
            evento(rm->gera, EV_ESPELHO, j, k, outro(j), 0, alvo, "Espelho vira o %d de %s", alvo, J[outro(j)].nome);
        }
        if (!virados && !rm->revelados[j]) continue;
        if (rm->gera) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        rm->pre[j] = true;
        refaz_sequencia(j, F[j].lancados);
        for (int k = 0; k < F[j].lancados; k++)
            if (quais >> k & 1) poe_tag(&F[j], k, "=%d", alvo);
        if (rm->gera) guarda_seq_de(0, j, EV_REVELA, EV_ESPELHO);
        rm->virou = true;
    }
}

// A previa anota se o tremor ou a Casca vao agir (previa_sorte).
static void fase_anota_sorte(resolve_t *rm)
{
    if (!rm->gera) previa_sorte = tremor_na_mesa() || casca_no_fim();
}

// O Terremoto valendo com 7 ou 8 sacode a mesa do oponente: o ultimo dado
// valido dele rola de novo e a sequencia dele e refeita com o numero
// novo (menor que o de antes, cai e leva o anterior). So no fim de
// verdade: a previa e a IA nao sabem o que o tremor vai dar. A Couraca
// do oponente segura a mesa dele.
static void fase_terremoto(resolve_t *rm)
{
    for (int o = 0; rm->gera && o < 2; o++) {
        int j = o ? outro(primeiro) : primeiro, r = outro(j);
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!tremor_armado(j, k)) continue;
            if (tem_am(r, A_COURACA)) { am_acende(A_COURACA); continue; }
            int a = alvo_tremor(F[r].est, r);
            if (a < 0) continue;
            int antes = F[r].valor[a];
            if (!rm->pre[r]) memcpy(est_pre_espelho[r], F[r].est, N_FILA);
            rm->pre[r] = true;
            // (sem a semente do lance - o Camaleao atras de um Terremoto, o
            // Terremoto que ja veio grudado pelo Chiclete -, a da mesa, como a Casca)
            treme_dado(r, a, F[j].abalo[k] ? F[j].abalo[k] : pega_semente(j, k));
            refaz_sequencia(r, F[r].lancados);
            evento_t *e;
            if (F[r].valor[a] == antes) e = evento(rm->gera, EV_TREMOR, j, k, r, a, antes, "Terremoto: o %d de %s treme e fica %d", antes, J[r].nome, antes);
            else e = evento(rm->gera, EV_TREMOR, j, k, r, a, F[r].valor[a], "Terremoto: o %d de %s rola de novo: %d", antes, J[r].nome, F[r].valor[a]);
            if (e) e->antes = antes;
            guarda_seq(e, r);
            rm->virou = true;
        }
    }
}

// A Casca de Banana de quem jogou depois (a de quem abriu age no lance,
// partida.inc): o oponente ja tinha jogado, e o 4o dado dele (com mais
// cascas, o 3o, o 2o, o 1o) rola de novo e fica o menor dos dois - menor
// que o de antes, a sequencia derruba os dois. Sem esse dado (ou ja
// anulado), nada. Como o tremor, so no fim de verdade (a previa e a IA nao
// sabem o que vai dar), pelo sorteio que vem da mesa. A Couraca e a bolha
// do Impermeavel seguram.
static void fase_casca(resolve_t *rm, desfecho_t *d)
{
    if (rm->gera) {
        int r = outro(primeiro), t = primeiro;   // r: o dono das cascas; t: quem escorrega
        int nc = cascas_de(r);
        for (int n = 0; n < nc; n++) {
            int kc = casca_dado(r, n), a = CASCA_DADO + 1 - nc + n;
            if (a >= F[t].lancados || F[t].est[a] != V_VALIDO || universo(t, a)) continue;
            if (tem_am(t, A_COURACA)) { am_acende(A_COURACA); continue; }
            int antes = F[t].valor[a];
            evento_t *e;
            d->atacados[t]++;
            if (!rm->pre[t]) memcpy(est_pre_espelho[t], F[t].est, N_FILA);
            rm->pre[t] = true;
            if (impermeavel(t, a)) {
                e = evento(rm->gera, EV_TREMOR, r, kc, t, a, antes, "Casca de Banana: o %d de %s está protegido", antes, J[t].nome);
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
            if (fica) e = evento(rm->gera, EV_TREMOR, r, kc, t, a, antes, "Casca de Banana: o %d de %s escorrega e fica", antes, J[t].nome);
            else e = evento(rm->gera, EV_TREMOR, r, kc, t, a, F[t].valor[a], "Casca de Banana: o %d de %s escorrega: %d", antes, J[t].nome, F[t].valor[a]);
            if (e) { e->antes = antes; e->ad = AD_CASCA; }
            guarda_seq(e, t);
            rm->virou = true;
        }
    }
}

// A Ficha Coringa vira: passa a ter o numero do ultimo dado valido do
// oponente (depois do tremor) e entra na sequencia do dono com ele - ai
// pode cair, ou derrubar o seguinte. Sem nenhum valido do oponente, ela e
// anulada. As dos dois lados viram juntas, cada uma olhando a mesa do
// outro antes de qualquer uma virar. Na previa tambem (nada ao acaso), e
// a mesa volta no fim.
static void fase_coringa(resolve_t *rm)
{
    int c_alvo[2], c_val[2];
    for (int j = 0; j < 2; j++) {
        c_alvo[j] = alvo_curinga(j);
        c_val[j] = c_alvo[j] >= 0 ? F[outro(j)].valor[c_alvo[j]] : 0;
    }
    for (int j = 0; j < 2; j++) {
        if (!tem_curinga(j)) continue;
        if (rm->gera && !rm->pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        rm->pre[j] = true;
        int ev0 = n_ev;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!F[j].curinga[k]) continue;
            F[j].curinga[k] = false;
            if (c_alvo[j] < 0) {
                F[j].curinga_nada[k] = true;
                evento(rm->gera, EV_CORINGA, j, k, outro(j), 0xFF, 0, "Ficha Coringa: %s não tem dado válido, anulada", J[outro(j)].nome);
                continue;
            }
            int v = c_val[j] + (F[j].calibre[k] ? 1 : 0);
            F[j].valor[k] = v;
            lado_vira(j, k, v);
            evento(rm->gera, EV_CORINGA, j, k, outro(j), c_alvo[j], v, "Ficha Coringa vira o %d de %s", c_val[j], J[outro(j)].nome);
        }
        refaz_sequencia(j, F[j].lancados);
        if (rm->gera) guarda_seq_de(ev0, j, EV_CORINGA, EV_CORINGA);
        rm->virou = true;
    }
}

// O Carrossel para: vale quantos dados valem na mesa agora, dos dois
// lados, depois de tudo o que virou acima (o Oculto, o Espelho, o tremor,
// a Casca, a Ficha Coringa) e antes dos ataques do fim. Ele conta, e os
// outros Carrosseis que ainda giram tambem (de 1 a 8; com o Calibre
// antes, +1, ate o 8). Entra na sequencia do dono com ele - pode cair, ou
// derrubar o seguinte. Com mais de um, param um de cada vez (os de quem
// abriu a rodada primeiro), cada um no numero da mesa naquela hora. Na
// previa tambem (nada ao acaso), e a mesa volta no fim.
static void fase_carrossel(resolve_t *rm)
{
    for (int o = 0; o < 2; o++) {
        int j = o ? outro(primeiro) : primeiro;
        if (!tem_gira(j)) continue;
        if (rm->gera && !rm->pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        rm->pre[j] = true;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!F[j].gira[k]) continue;
            int roda = carrossel_conta();
            F[j].gira[k] = false;
            int v = roda + (F[j].calibre[k] && roda < CARROSSEL_MAX ? 1 : 0);
            F[j].valor[k] = v;
            lado_vira(j, k, v);
            refaz_sequencia(j, F[j].lancados);
            evento_t *e;
            if (v != roda) e = evento(rm->gera, EV_CARROSSEL, j, k, j, k, v, "Carrossel para no %d: %d válidos e o Calibre", v, roda);
            else if (v == 1) e = evento(rm->gera, EV_CARROSSEL, j, k, j, k, v, "Carrossel para no 1: só ele vale na mesa");
            else e = evento(rm->gera, EV_CARROSSEL, j, k, j, k, v, "Carrossel para no %d: dados válidos na mesa", v);
            guarda_seq(e, j);
        }
        rm->virou = true;
    }
}

// O Espelho que nao achou o que copiar no comeco (o oponente so com o
// Universo, o Vazio, ou com a Ficha Coringa e o Carrossel, que so viraram
// agora): vira aqui o maior do oponente, como la em cima. Se nem assim ha
// o que copiar, e anulado (como a Ficha Coringa sem alvo), a sequencia e
// refeita sem ele e a marca "=?" sai da mesa.
static void fase_espelho_tardio(resolve_t *rm)
{
    for (int j = 0; j < 2; j++) {
        int alvo = maior_do_rival(j), ev0 = n_ev;
        uint8_t quais = 0, nada = 0;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!F[j].espera[k]) continue;
            F[j].espera[k] = false;
            rm->virou = true;
            if (alvo <= 0) {
                if (!strcmp(F[j].tag[k], "=?")) F[j].tag[k][0] = 0;
                if (!strcmp(F[j].tag_num[k], "=?")) F[j].tag_num[k][0] = 0;
                F[j].espelho_nada[k] = true;
                nada |= (uint8_t)(1 << k);
                evento(rm->gera, EV_ESPELHO, j, k, outro(j), 0xFF, 0, "Espelho: %s não tem o que copiar, anulado", J[outro(j)].nome);
                continue;
            }
            F[j].valor[k] = alvo;
            lado_vira(j, k, alvo);
            quais |= (uint8_t)(1 << k);
            evento(rm->gera, EV_ESPELHO, j, k, outro(j), 0, alvo, "Espelho vira o %d de %s", alvo, J[outro(j)].nome);
        }
        if (!quais && !nada) continue;
        if (rm->gera && !rm->pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        rm->pre[j] = true;
        refaz_sequencia(j, F[j].lancados);
        for (int k = 0; k < F[j].lancados; k++)
            if (quais >> k & 1) poe_tag(&F[j], k, "=%d", alvo);
        if (rm->gera) guarda_seq_de(ev0, j, EV_ESPELHO, EV_ESPELHO);
    }
}

// O veredito da sequencia volta a ser o de antes das fases.
static void mesa_devolve_veredito(const resolve_t *rm)
{
    memcpy(veredito, rm->ver_antes, sizeof veredito);
    ver_tipo = rm->vt; anulou_a = rm->aa; anulou_b = rm->ab; resistiu = rm->res; anulou_masc = rm->am; ver_comum = rm->vc;
}

// O estado de cada dado depois das fases da mesa: o desfecho parte dele.
static void fase_fotografa(desfecho_t *d)
{
    for (int j = 0; j < 2; j++) memcpy(d->est[j], F[j].est, N_FILA);
}

// Na previa, a mesa volta como estava antes das fases (o Espelho segue
// esperando), e o tamanho da sequencia de cada um tambem.
static void mesa_devolve(const resolve_t *rm)
{
    if (!rm->gera && rm->virou) memcpy(F, rm->f_antes, sizeof F);   // previa: o Espelho segue esperando
    for (int j = 0; !rm->gera && j < 2; j++) F[j].n = rm->f_antes[j].n;
}
