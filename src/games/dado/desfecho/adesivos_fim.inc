// ===========================================================================
// ADESIVOS DO FIM: o que cada adesivo faz no fim da rodada (ad_age), a ordem
// em que agem (adesivos_de: o Selo, os de cada dado, o Porta-Retrato, o
// Papagaio, o Holografico, a Purpurina) e o Bilhete Premiado (bilhete_de).
// Le: F, d e J[] (a bolsa, so para ler: os adesivos de cada dado, o pavio,
// as figurinhas e etiquetas; o nome).
// Grava: d (os pontos e o que fica para depois da rodada: o que sai, cresce,
// quebra, lacra), ev[] e os tres avisos daqui (age_modo, age_lado,
// corrente_antes); com gera, acende a Couraca. Nao grava F nem a bolsa.
// Quem chama: calcula_desfecho (orquestra.inc). ad_do_pouso: tambem a fila.
// ===========================================================================

// Os adesivos que agem no pouso, antes da fila (o Chumbo, o Reflexivo, o
// Avesso, o Remendo, o Post-it, a Nota Dez, o Papel Bomba, a Bussola), ou na
// regra da fila (o Transparente, o Corretivo, o Papel Carbono): no fim da
// rodada so contam para a Purpurina.
static bool ad_do_pouso(int a)
{
    return a == AD_CHUMBO || a == AD_REFLEXIVO || a == AD_AVESSO || a == AD_REMENDO || a == AD_TRANSPARENTE
        || a == AD_CORRETIVO || a == AD_POSTIT || a == AD_NOTA_DEZ || a == AD_BOMBA || a == AD_CARBONO
        || a == AD_BUSSOLA;
}

// As figurinhas juntadas nos dados de j com Figurinha (cada um comeca com 1),
// e as Etiquetas coladas nos dados dele: o que a Figurinha e a Etiqueta pagam.
static int figurinhas_de(int j) { return conta_figurinhas(J[j].col, J[j].onde, J[j].n_col); }
static int etiquetas_de(int j)   { return conta_etiquetas(J[j].col, J[j].onde, J[j].n_col); }

// Como o adesivo age agora (adesivos_de): EVM_CARIMBO, a segunda vez dele,
// carimbado pelo dado de antes - o que multiplica sobe so um degrau (x2 vira
// x3), e o que so marca o dado (Bis, Selo, Velcro, Chiclete, Impermeavel, o
// Fragil que quebra) nao repete; EVM_DUPLA, o do lado oposto, pela Dupla Face.
static int age_modo;
static int age_lado;                             // o lado do adesivo que esta agindo (o de cima, o de baixo, o vizinho)
static int corrente_antes;                       // a Corrente: os adesivos que agiram nos dados antes deste
// O que o Papagaio e o Porta-Retrato podem repetir em outro dado: os do fim
// da rodada (os do pouso, os que agem depois - o Holografico, a Purpurina, o
// Bilhete - e os que so fazem outro agir, nao).
static bool copia_pode(int a)
{
    // ACHADO A21: preservado - a Sujeira nao esta na lista: o Porta-Retrato a repete no dado de antes
    return a && a != AD_HOLOGRAFICO && a != AD_PURPURINA && a != AD_CARIMBO && a != AD_DUPLA && a != AD_MOLDURA
        && a != AD_BILHETE && a != AD_BARRO && a != AD_MASSINHA && a != AD_CHICLETE && a != AD_XEROX && !ad_do_pouso(a);
}
// O dado de antes de k tirou o Carimbo (a segunda vez e dele, nao da Tela em Branco).
static bool carimbado_ja(int j, int k) { return k > 0 && ad_fila_tem(j, k - 1, AD_CARIMBO); }
// O que o adesivo a faz no fim da rodada, pelo dado k de j: e dele, ou e o
// Holografico (no dado k; ou o Papagaio, ou o Porta-Retrato) copiando o do
// dado ko (copia: nada sai nem cresce no dado de la; o Fermento paga o que
// pagou la). Devolve se agiu.
// ACHADO A17: preservado - apesar do que diz acima, o Fragil copiado num dado anulado quebra o dado da copia
static bool ad_age(desfecho_t *d, int j, int k, int ko, int a, bool gera, bool copia)
{
    int r = outro(j);
    bool vale = d->est[j][k] == V_VALIDO, de_novo = age_modo & EVM_CARIMBO;
    uint8_t bit = (uint8_t)(1 << k);
    int8_t mx = de_novo ? 1 : 2;                 // o x2 carimbado: so mais um degrau
    // ACHADO A35: preservado - carimbado_ja nao ve se o Carimbo de antes foi roubado: a Tela sai como Carimbo
    const char *pre = age_modo & EVM_PAPAGAIO ? "Papagaio, como " : age_modo & EVM_RETRATO ? "Porta-Retrato » "
                    : copia ? (de_novo ? "Carimbo » Holográfico, como " : "Holográfico, como ")
                    : de_novo ? (F[j].tipo[k] == D_TELA && !carimbado_ja(j, k) ? "Tela em Branco » " : "Carimbo » ")
                    : age_modo & EVM_DUPLA ? "Dupla Face » "
                    : age_modo & EVM_MOLDURA ? "Moldura » " : "";
    evento_t *e = NULL;
    switch (a) {
    case AD_CONFETE:
        if (!vale) return false;
        d->semente[j] += 4;
        e = evento(gera, EV_ADESIVO, j, k, j, k, 4, "%sConfete: +4", pre);
        break;
    case AD_RASPADINHA:
        if (!vale) return false;
        d->semente[j] += 10;
        if (!copia) { d->raspa[j] |= bit; d->raspa_lados[j][k] |= 1u << age_lado; }
        e = evento(gera, EV_ADESIVO, j, k, j, k, 10, copia ? "%sRaspadinha: +10" : "%sRaspadinha: +10, e o adesivo sai", pre);
        break;
    case AD_FERMENTO: {
        const peca_t *po = peca_fila(j, ko);
        if (!vale || !po) return false;
        int n = FERMENTO_CADA * (po->fermento + 1 + (de_novo ? 1 : 0));   // carimbado: ja o degrau seguinte
        d->semente[j] += n;
        if (!copia) { if (de_novo) d->fermento2[j] |= bit; else d->fermento[j] |= bit; }
        e = evento(gera, EV_ADESIVO, j, k, j, k, n, copia ? "%sFermento: +%d" : "%sFermento: +%d, e cresce", pre, n);
        break;
    }
    case AD_ELETRONICO:
        if (!vale) return false;
        d->mult[j][k] = (int8_t)(d->mult[j][k] + mx);
        e = evento(gera, EV_ADESIVO, j, k, j, k, mx, de_novo ? "%sEletrônico: x3 no %d" : "%sEletrônico: x2 no %d", pre, F[j].valor[k]);
        if (e) { e->mascara = bit; memcpy(e->por_dado, d->mult[j], N_FILA); }
        break;
    case AD_LOBO:
        d->delta[r] -= 2;
        d->atacados[r]++;
        e = evento(gera, EV_ADESIVO, j, k, r, 0, 2, "%sLobo: -2 para %s", pre, J[r].nome);
        break;
    case AD_GORJETA:
        d->gorjeta[j] += GORJETA_FICHAS;
        e = evento(gera, EV_ADESIVO, j, k, r, 0, GORJETA_FICHAS, "%sGorjeta: %s paga %d fichas", pre, J[r].nome, GORJETA_FICHAS);
        break;
    case AD_DOURADO:
        d->moedas[j]++;
        e = evento(gera, EV_ADESIVO, j, k, j, k, 1, "%sDourado: +1 moeda", pre);
        break;
    case AD_BIS:
        if (de_novo) return false;
        d->bis[j] = 1;                           // dois Bis na mesma rodada: ainda 1 lugar so
        e = evento(gera, EV_ADESIVO, j, k, j, k, 1, "%sBis: +1 dado na sequência da próxima rodada", pre);
        break;
    case AD_SELO:
        if (de_novo) return false;
        d->selo_lacra[j] |= bit;
        e = evento(gera, EV_ADESIVO, j, k, j, k, 0, "%sSelo: lacrado até o próximo lance", pre);
        break;
    case AD_VELCRO:
        if (de_novo) return false;
        d->velcro[j] |= bit;
        e = evento(gera, EV_ADESIVO, j, k, j, k, 0, "%sVelcro: volta para a mão", pre);
        break;
    case AD_FRAGIL:                              // valendo, +6; anulado, quebra (com a capa, nao)
        if (vale) {
            d->semente[j] += FRAGIL_PONTOS;
            e = evento(gera, EV_ADESIVO, j, k, j, k, FRAGIL_PONTOS, "%sFrágil: +%d", pre, FRAGIL_PONTOS);
        } else if (de_novo) return false;         // quebrar, quebra uma vez so
        else if (d->est[j][k] == V_ANULADO && peca_fila(j, k) && impermeavel(j, k)) {
            e = evento(gera, EV_ADESIVO, j, k, j, k, 0, "%sFrágil: anulado, mas o dado está protegido", pre);
            if (e) e->bloqueado = bit;
        } else if (d->est[j][k] == V_ANULADO && peca_fila(j, k)) {
            // ACHADO A17: preservado - vale para a copia tambem: o Holografico anulado quebra o proprio dado
            d->fragil[j] |= bit;
            e = evento(gera, EV_ADESIVO, j, k, j, k, 0, "%sFrágil: anulado, o dado quebra", pre);
        } else return false;
        break;
    case AD_ESTRELINHA: {                        // +2 por dado valendo da sequencia, ele incluido
        int n = 0;
        uint8_t m = 0;
        for (int i = 0; i < F[j].lancados && i < N_FILA; i++)
            if (d->est[j][i] == V_VALIDO) { n++; m |= (uint8_t)(1 << i); }
        if (!n) return false;
        d->semente[j] += ESTRELINHA_CADA * n;
        e = evento(gera, EV_ADESIVO, j, k, j, k, ESTRELINHA_CADA * n, "%sEstrelinha: %d válido%s, +%d", pre, n, n > 1 ? "s" : "",
                   ESTRELINHA_CADA * n);
        if (e) { e->mascara = m; for (int i = 0; i < N_FILA; i++) e->por_dado[i] = (int8_t)(m >> i & 1 ? ESTRELINHA_CADA : 0); }
        break;
    }
    case AD_IMANTADO: {                          // o vizinho (o seguinte ou o anterior) tirou o mesmo numero: x2 nos dois
        bool agiu = false, ele = false;          // ele: o x2 do proprio dado entra uma vez so
        for (int lado = 0; lado < 2; lado++) {
            int p = lado ? k - 1 : k + 1;
            if (p < 0 || p >= F[j].lancados || p >= N_FILA || F[j].valor[p] != F[j].valor[k]) continue;
            bool vale2 = d->est[j][p] == V_VALIDO;
            if (!vale2 && (!vale || ele)) continue;
            uint8_t m = 0;
            if (vale && !ele) { d->mult[j][k] = (int8_t)(d->mult[j][k] + mx); m |= bit; ele = true; }
            if (vale2) { d->mult[j][p] = (int8_t)(d->mult[j][p] + mx); m |= (uint8_t)(1 << p); }
            e = evento(gera, EV_ADESIVO, j, k, j, p, mx, de_novo ? "%sImantado: dois %d, x3" : "%sImantado: dois %d, x2", pre, F[j].valor[k]);
            if (e) { e->mascara = m; memcpy(e->por_dado, d->mult[j], N_FILA); e->ad = (uint8_t)a; e->copia = copia; e->modo = (uint8_t)age_modo; }
            agiu = true;
        }
        if (!agiu) return false;
        break;
    }
    case AD_FIGURINHA: {                         // ativa todas as figurinhas juntadas nos seus dados
        if (!vale) return false;
        int n = figurinhas_de(j);
        d->semente[j] += n;
        if (!copia) { if (de_novo) d->fig_cresce2[j] |= bit; else d->fig_cresce[j] |= bit; }
        e = evento(gera, EV_ADESIVO, j, k, j, k, n, copia ? "%sFigurinha: %d figurinhas, +%d"
                                                         : "%sFigurinha: %d figurinhas, +%d, e junta mais uma", pre, n, n);
        break;
    }
    case AD_ETIQUETA: {                          // +1 por Etiqueta nos seus dados
        if (!vale) return false;
        int n = etiquetas_de(j);
        d->semente[j] += n;
        e = evento(gera, EV_ADESIVO, j, k, j, k, n, "%sEtiqueta: %d na bolsa, +%d", pre, n, n);
        break;
    }
    case AD_LENTICULAR:                          // valendo, x2; anulado, -4 no seu placar
        if (vale) {
            d->mult[j][k] = (int8_t)(d->mult[j][k] + mx);
            e = evento(gera, EV_ADESIVO, j, k, j, k, mx, de_novo ? "%sLenticular: x3 no %d" : "%sLenticular: x2 no %d", pre, F[j].valor[k]);
            if (e) { e->mascara = bit; memcpy(e->por_dado, d->mult[j], N_FILA); }
        } else if (d->est[j][k] == V_ANULADO) {
            d->delta[j] -= LENTICULAR_PERDE;
            e = evento(gera, EV_ADESIVO, j, k, j, k, -LENTICULAR_PERDE, "%sLenticular: anulado, -%d", pre, LENTICULAR_PERDE);
        } else return false;
        break;
    case AD_NEON: {                              // valendo: +2 por adesivo no dado (o deste lado junto, e a Sujeira tambem)
        const peca_t *pn = peca_fila(j, k);
        if (!vale || !pn) return false;
        int n = ad_quantos(pn);
        d->semente[j] += NEON_CADA * n;
        e = evento(gera, EV_ADESIVO, j, k, j, k, NEON_CADA * n, "%sNeon: %d adesivo%s, +%d", pre, n, n > 1 ? "s" : "", NEON_CADA * n);
        break;
    }
    case AD_INFECTADO:                           // valendo, +3 (o germe ja pegou na bolsa no pouso: adesivos_no_pouso)
        if (!vale) return false;
        d->semente[j] += INFECTADO_PONTOS;
        e = evento(gera, EV_ADESIVO, j, k, j, k, INFECTADO_PONTOS, "%sInfectado: +%d", pre, INFECTADO_PONTOS);
        if (e) e->bolsa_i = 0xFF;
        break;
    case AD_BARRO:                               // o Barro, a Massinha e o Chiclete ja agiram no pouso (adesivos_no_pouso)
    case AD_MASSINHA:
    case AD_CHICLETE:
        return false;
    case AD_RATOEIRA:                            // anulado, ate pela sequencia: o queijo fica (o ataque fecha a armadilha: ratoeira_desarma)
        if (vale) return false;
        d->semente[j] += RATOEIRA_PONTOS;
        e = evento(gera, EV_ADESIVO, j, k, j, k, RATOEIRA_PONTOS, "%sRatoeira: anulado, o queijo fica: +%d", pre, RATOEIRA_PONTOS);
        break;
    case AD_TIQUETE: {                           // a sequencia cheia: 4 dados seus valendo
        int nv = 0;
        for (int i = 0; i < F[j].lancados && i < N_FILA; i++) nv += d->est[j][i] == V_VALIDO;
        if (nv < TIQUETE_VALIDOS) return false;
        d->semente[j] += TIQUETE_PONTOS;
        e = evento(gera, EV_ADESIVO, j, k, j, k, TIQUETE_PONTOS, "%sTíquete de Metrô: %d válidos, +%d", pre, nv, TIQUETE_PONTOS);
        break;
    }
    case AD_SUJEIRA:                             // o barro do oponente: -2 no seu placar
        d->delta[j] -= SUJEIRA_PONTOS;
        d->atacados[j]++;                        // o barro veio do oponente
        e = evento(gera, EV_ADESIVO, j, k, j, k, -SUJEIRA_PONTOS, "%sSujeira: -%d no placar", pre, SUJEIRA_PONTOS);
        break;
    case AD_IMPERMEAVEL: {                       // a capa, ate o fim da partida (nem anulado ele fica)
        const peca_t *pc = peca_fila(j, k);
        if (!pc || pc->capa || de_novo || (d->capa[j] & bit)) return false;   // ja estava com a capa
        d->capa[j] |= bit;
        e = evento(gera, EV_ADESIVO, j, k, j, k, 0, "%sImpermeável: não pode ser anulado nem destruído", pre);
        break;
    }
    case AD_PATINHO: {                           // valendo: no lado dele, -2; caindo em outro, +2
        if (!vale) return false;
        bool cima = !copia && ko == k;           // (ko < 0: um Patinho de outro lado do dado)
        int pts = cima ? -PATINHO_PONTOS : PATINHO_PONTOS;
        d->semente[j] += pts;
        e = evento(gera, EV_ADESIVO, j, k, j, k, pts, cima ? "%sPatinho Feio: caiu nele, %d" : "%sPatinho Feio: +%d", pre, pts);
        break;
    }
    case AD_CORRENTE: {                          // +CORRENTE_CADA (3) por elo: ela e cada adesivo seu que agiu antes
        if (!vale) return false;
        // ACHADO A12: preservado - copiada, conta os elos de corrente_antes: nem os que pagou, nem os do dado da copia
        int elos = corrente_antes + 1, n = CORRENTE_CADA * elos;
        d->semente[j] += n;
        if (corrente_antes > 0)
            e = evento(gera, EV_ADESIVO, j, k, j, k, n, "%sCorrente: %d elos, +%d", pre, elos, n);
        else
            e = evento(gera, EV_ADESIVO, j, k, j, k, n, "%sCorrente: um elo, +%d", pre, n);
        break;
    }
    case AD_DINAMITE: {                          // valendo, paga o pavio; anulado, o pavio apaga
        int i = F[j].idx[k];
        if (de_novo || copia || i < 0 || i >= J[j].n_col || (d->dinamite[j] & bit)) return false;
        int n = J[j].pavio[i];
        if (vale && n > 0) {
            d->semente[j] += n;
            d->dinamite[j] |= bit;
            e = evento(gera, EV_ADESIVO, j, k, j, k, n, "%sDinamite: o pavio estoura, +%d", pre, n);
        } else if (d->est[j][k] == V_ANULADO && n > 0) {
            d->dinamite[j] |= bit;
            e = evento(gera, EV_ADESIVO, j, k, j, k, 0, "%sDinamite: anulada, o pavio apaga (%d)", pre, n);
        } else return false;
        break;
    }
    case AD_MIRA: {                              // o menor dado do oponente que vale (e que cai)
        if (!vale) return false;
        bool couraca = tem_am(r, A_COURACA);
        if (couraca && gera) am_acende(A_COURACA);
        int a0 = -1;
        for (int i = 0; i < F[r].lancados && !couraca; i++)
            if (d->est[r][i] == V_VALIDO && !firme_sem_capa(r, i) && (a0 < 0 || F[r].valor[i] < F[r].valor[a0])) a0 = i;
        int alvo = desvia_seguranca(d->est[r], r, a0);
        if (alvo < 0) return false;
        // ACHADO A26: preservado - copiada pelo Papagaio contra a capa, o texto sai como Holografico, como Mira
        if (bloqueia_capa(gera, j, k, r, alvo, copia ? "Holográfico, como Mira" : de_novo ? "Carimbo » Mira" : "Mira", AD_MIRA)) {
            if (capa_ev) { capa_ev->copia = copia; capa_ev->modo = (uint8_t)age_modo; }
            return true;
        }
        // ACHADO A03: preservado - o adesivo anula na hora, sem o contra-ataque dos dados: a Mira do alvo nao age mais
        d->est[r][alvo] = V_ANULADO;
        d->atacados[r]++;
        if (alvo != a0) e = evento(gera, EV_ADESIVO, j, k, r, alvo, F[r].valor[alvo], "%sMira: o Segurança de %s leva no lugar", pre, J[r].nome);
        else e = evento(gera, EV_ADESIVO, j, k, r, alvo, F[r].valor[alvo], "%sMira: anula o %d de %s", pre, F[r].valor[alvo], J[r].nome);
        if (e) { e->ad = (uint8_t)a; e->copia = copia; e->modo = (uint8_t)age_modo; }
        apos_ataque(d, j, k, r, alvo, gera);
        return true;
    }
    default: return false;
    }
    if (e) { e->ad = (uint8_t)a; e->copia = copia; e->modo = (uint8_t)age_modo; }
    return true;
}

// Os adesivos do fim da rodada, pelo lado que ficou para cima de cada dado.
// Os de "caiu aqui" agem valendo ou nao (Dourado, Lobo, Bis, Gorjeta, Selo,
// Velcro; o Fragil anulado quebra); os outros, so com o dado valendo.
// Roubado, o dado e do outro: o adesivo nao age. A Saideira nao os repete
// (sao do dado, nao poderes dele). Antes de tudo, o lacre que um Selo deixou
// num lance de antes: este lance o rompe, e valendo paga. Depois dos outros,
// o Holografico (repete cada um que agiu, como se fosse dele) e por ultimo a
// Purpurina (+3 por adesivo que agiu, ela junto; a Ratoeira conta se
// desarmou nos ataques). A Dupla Face faz agir tambem o adesivo do lado
// oposto do dado; o Carimbo faz o adesivo do dado seguinte agir duas vezes
// (os dois contam uma vez cada para a Purpurina, quando fazem agir).

// O dado k de j foi carimbado: o dado de antes dele (que nao foi roubado)
// tirou o Carimbo, ou a Dupla Face com o Carimbo do outro lado.
static bool carimbado(const desfecho_t *d, int j, int k)
{
    return k > 0 && k < N_FILA && d->est[j][k - 1] != V_ROUBADO && ad_fila_tem(j, k - 1, AD_CARIMBO);
}
// Um adesivo a do dado k de j agindo no fim (primeira: nao e a repeticao do
// Carimbo; so a primeira conta para a Purpurina e serve ao Holografico).
static bool age_um(desfecho_t *d, int j, int k, int a, bool gera, bool primeira,
                   int *fonte, int *qual, int *n_fonte, int *agiram)
{
    uint8_t bit = (uint8_t)(1 << k);
    if (a == AD_HOLOGRAFICO || a == AD_PURPURINA || a == AD_CARIMBO || a == AD_DUPLA) return false;   // agem depois (ou por outro)
    // ACHADO A21: preservado - sozinha, a Sujeira nao conta para a Purpurina; repetida pelo Porta-Retrato, conta
    if (a == AD_SUJEIRA) return ad_age(d, j, k, k, a, gera, false);   // o barro do oponente: nao e adesivo seu
    if (ad_do_pouso(a)) { if (primeira) (*agiram)++; return false; }
    // O Barro, a Massinha e o germe do Infectado ja agiram no pouso
    // (adesivos_no_pouso), valendo ou nao: aqui so contam - para a Purpurina,
    // e o Carimbo e a Dupla Face que os fizeram agir tambem. O Infectado
    // valendo ainda paga os +INFECTADO_PONTOS (3) (ad_age).
    if (a == AD_BARRO || a == AD_MASSINHA || (a == AD_INFECTADO && d->est[j][k] != V_VALIDO)) {
        if (primeira) (*agiram)++;
        return true;
    }
    // O Chiclete que grudou no pouso conta tambem (uma vez: carimbado, ele
    // nao gruda duas); o que nao grudou (outro ja tinha grudado), nao.
    if (a == AD_CHICLETE) {
        if (F[j].chiclete != k + 1 || (age_modo & EVM_CARIMBO)) return false;
        if (primeira) (*agiram)++;
        return true;
    }
    if (a == AD_BILHETE) {                       // o Bilhete age no fim de tudo (bilhete_de)
        if (d->est[j][k] != V_VALIDO) return false;
        // ACHADO A02: preservado - o Bilhete e anotado aqui; se a Mira do rival anular o dado depois, dobra assim mesmo
        if (primeira) { d->bilhete[j] |= bit; (*agiram)++; }
        else d->bil_carimbo[j] |= bit;
        return true;
    }
    if (!ad_age(d, j, k, k, a, gera, false)) return false;
    if (primeira) {
        if (a != AD_RATOEIRA || !(d->rato_bits[j] >> k & 1)) (*agiram)++;   // a que desarmou ja contou
        fonte[*n_fonte] = k; qual[(*n_fonte)++] = a;
    }
    return true;
}
// A Moldura no lado de cima do dado k de j: os adesivos dos lados de numero
// vizinho (o de baixo e o de cima) tambem agem - menos os do lance, o Patinho
// Feio, a Dupla Face e outra Moldura (nada emoldura de novo).
static void moldura_fila(int j, int k, int *viz)
{
    viz[0] = viz[1] = AD_NADA;
    const peca_t *p = peca_fila(j, k);
    if (!p || ad_no_lado(p, F[j].lado[k]) != AD_MOLDURA) return;
    for (int q = 0; q < 2; q++) {
        int l = F[j].lado[k] + (q ? 1 : -1);
        if (l < ad_lado_min(p->tipo) || l > ad_lado_max(p->tipo)) continue;
        int b = ad_no_lado(p, l);
        if (b == AD_MOLDURA || b == AD_DUPLA || b == AD_PATINHO || b == AD_HOLOGRAFICO || b == AD_PURPURINA
            || b == AD_CARIMBO || ad_do_pouso(b)) continue;
        viz[q] = b;
    }
}
static void adesivos_de(desfecho_t *d, int j, bool gera)
{
    // ACHADO A32: preservado - as Ratoeiras que desarmaram ja entram na conta da Purpurina (nao reproduz diferenca)
    int agiram = d->ratoeira[j];
    int fonte[N_FILA * AD_LADOS], qual[N_FILA * AD_LADOS], n_fonte = 0;   // o que o Holografico copia: o dado e o adesivo
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        const peca_t *pc = peca_fila(j, k);
        if (!pc || !pc->selo) continue;
        d->selo_rompe[j] |= (uint8_t)(1 << k);
        if (d->est[j][k] != V_VALIDO) continue;
        d->semente[j] += SELO_PONTOS;
        agiram++;
        evento_t *e = evento(gera, EV_ADESIVO, j, k, j, k, SELO_PONTOS, "Selo: o lacre se rompe, +%d", SELO_PONTOS);
        if (e) e->ad = AD_SELO;
    }
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        int a = ad_fila(j, k);
        if (!a || d->est[j][k] == V_ROUBADO) continue;
        const peca_t *pc = peca_fila(j, k);      // (com adesivo, ha peca)
        bool carimbo = carimbado(d, j, k) || F[j].tipo[k] == D_TELA, duplo = false, carimbou = false;   // a Tela: duas vezes
        int b = a == AD_DUPLA ? ad_oposto_fila(j, k) : AD_NADA;
        int viz[2];                              // a Moldura: os dos lados vizinhos
        moldura_fila(j, k, viz);
        // ACHADO A12: preservado - fica com a conta do ultimo dado com adesivo: as copias da Corrente usam esse valor
        corrente_antes = agiram;
        for (int q = 0; q < 4; q++) {            // o de cima, pela Dupla Face o de baixo, pela Moldura os vizinhos
            int x = q == 0 ? a : q == 1 ? b : viz[q - 2];
            if (!x) continue;
            int lc = F[j].lado[k];
            age_lado = q == 0 ? lc : q == 1 ? ad_oposto(pc->tipo, lc) : q == 2 ? lc - 1 : lc + 1;
            for (int c = 0; c < (carimbo ? 2 : 1); c++) {   // e carimbado, outra vez
                age_modo = (q == 1 ? EVM_DUPLA : q >= 2 ? EVM_MOLDURA : 0) | (c ? EVM_CARIMBO : 0);
                bool agiu = age_um(d, j, k, x, gera, c == 0, fonte, qual, &n_fonte, &agiram);
                duplo |= agiu && q;
                carimbou |= agiu && c;
            }
        }
        age_modo = 0;
        agiram += duplo + carimbou;              // a Dupla Face e o Carimbo de antes, quando fizeram agir
        // O Porta-Retrato valendo: o adesivo do lado dele age tambem no dado de
        // antes, se ele valer, como se fosse dele.
        if (TIPO[tipo_de(j, k)].efeito == EF_RETRATO && d->est[j][k] == V_VALIDO && k > 0
            && d->est[j][k - 1] == V_VALIDO && copia_pode(a)) {
            age_modo = EVM_RETRATO;
            // ACHADO A21: preservado - com a Sujeira, o -2 cai de novo, conta outro ataque e +1 para a Purpurina
            if (ad_age(d, j, k - 1, k, a, gera, true)) agiram++;
            age_modo = 0;
        }
    }
    // O Patinho Feio dos outros lados do dado: caiu em outro lado, +2.
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        const peca_t *pc = peca_fila(j, k);
        if (!pc || d->est[j][k] == V_ROUBADO) continue;
        int n = ad_conta(pc, AD_PATINHO) - (ad_fila(j, k) == AD_PATINHO);
        for (int i = 0; i < n && n_fonte < N_FILA * AD_LADOS; i++)
            if (ad_age(d, j, k, -1, AD_PATINHO, gera, false)) { agiram++; fonte[n_fonte] = k; qual[n_fonte++] = AD_PATINHO; }
    }
    // O Papagaio valendo: o ultimo adesivo seu que agiu na rodada age de novo nele.
    for (int k = 0; k < F[j].lancados && k < N_FILA && n_fonte; k++) {
        if (TIPO[tipo_de(j, k)].efeito != EF_PAPAGAIO || d->est[j][k] != V_VALIDO) continue;
        int ul = n_fonte - 1;
        if (!copia_pode(qual[ul])) continue;
        age_modo = EVM_PAPAGAIO;
        if (ad_age(d, j, k, fonte[ul], qual[ul], gera, true)) agiram++;
        age_modo = 0;
    }
    int holo[N_FILA], n_holo = 0;
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        if (!ad_fila_tem(j, k, AD_HOLOGRAFICO) || d->est[j][k] == V_ROUBADO) continue;
        // ACHADO A11: preservado - aqui a Tela em Branco nao conta: o Holografico nela age uma vez so
        bool carimbo = carimbado(d, j, k), algum = false;
        int md = ad_fila(j, k) == AD_HOLOGRAFICO ? 0 : EVM_DUPLA;
        holo[n_holo++] = k;
        for (int c = 0; c < (carimbo ? 2 : 1); c++) {
            age_modo = md | (c ? EVM_CARIMBO : 0);
            bool esta = false;
            for (int f = 0; f < n_fonte; f++) esta |= ad_age(d, j, k, fonte[f], qual[f], gera, true);
            // ACHADO A16: preservado - carimbado (c == 1), o Holografico anota outra copia cheia do Bilhete (x8)
            if (d->bilhete[j] && d->est[j][k] == V_VALIDO) {     // o Bilhete, copiado no fim de tudo
                for (int b = 0; b < N_FILA; b++) d->bil_holo[j][k] += d->bilhete[j] >> b & 1;
                esta = true;
            }
            if (esta && c) agiram++;             // o Carimbo de antes
            algum |= esta;
        }
        age_modo = 0;
        if (algum) agiram += 1 + (md != 0);      // ele, e a Dupla Face que o fez agir
    }
    int purp[2 * N_FILA], n_purp = 0, n_purp_ad = 0;
    uint8_t purp_md[2 * N_FILA];
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        if (!ad_fila_tem(j, k, AD_PURPURINA) || d->est[j][k] != V_VALIDO) continue;
        int md = ad_fila(j, k) == AD_PURPURINA ? 0 : EVM_DUPLA;
        purp_md[n_purp] = (uint8_t)md; purp[n_purp++] = k; n_purp_ad += 1 + (md != 0);
        // ACHADO A11: preservado - aqui a Tela em Branco nao conta: a Purpurina nela age uma vez so
        if (carimbado(d, j, k)) { purp_md[n_purp] = (uint8_t)(md | EVM_CARIMBO); purp[n_purp++] = k; n_purp_ad++; }
    }
    int cada = PURPURINA_CADA * (agiram + n_purp_ad);
    d->ad_agiram[j] = (uint8_t)(agiram + n_purp_ad > 250 ? 250 : agiram + n_purp_ad);
    for (int i = 0; i < n_purp; i++) {
        d->semente[j] += cada;
        const char *pre = purp_md[i] & EVM_CARIMBO ? "Carimbo » " : purp_md[i] & EVM_DUPLA ? "Dupla Face » " : "";
        evento_t *e = evento(gera, EV_ADESIVO, j, purp[i], j, purp[i], cada, "%sPurpurina: %d adesivo%s, +%d", pre,
                             agiram + n_purp_ad, agiram + n_purp_ad > 1 ? "s" : "", cada);
        if (e) { e->ad = AD_PURPURINA; e->modo = purp_md[i]; }
    }
    for (int h = 0; h < n_holo && n_purp; h++) {       // o Holografico valendo copia a Purpurina tambem
        if (d->est[j][holo[h]] != V_VALIDO) continue;
        for (int i = 0; i < n_purp; i++) {
            d->semente[j] += cada;
            evento_t *e = evento(gera, EV_ADESIVO, j, holo[h], j, holo[h], cada, "Holográfico, como Purpurina: +%d", cada);
            if (e) { e->ad = AD_PURPURINA; e->copia = 1; }
        }
    }
}

// O Bilhete Premiado (adesivos_de anotou os que valeram), no fim de tudo: o
// bonus dos dados validos de j vale em dobro - o que os poderes e amuletos
// somaram em cada dado e os pontos a parte (a semente: os dos adesivos e dos
// poderes que pontuam por fora). O numero que cada dado tirou, nao. Com dois
// Bilhetes, dobra duas vezes; o Holografico copia cada um, e dobra de novo.
static void dobra_bonus(desfecho_t *d, int j, int k, bool gera, bool copia, bool carimbo)
{
    int8_t por[N_FILA] = { 0 };
    uint8_t m = 0;
    int extra = 0;
    for (int i = 0; i < F[j].lancados && i < N_FILA; i++) {
        if (d->est[j][i] != V_VALIDO) continue;
        int b = F[j].bonus[i] + d->bonus[j][i];
        if (carimbo) b /= 2;                     // carimbado: o x2 vira x3 (mais a metade do ja dobrado)
        if (b <= 0) continue;
        if (d->bonus[j][i] + b > 127) b = 127 - d->bonus[j][i];
        d->bonus[j][i] = (int8_t)(d->bonus[j][i] + b);
        por[i] = (int8_t)b;
        m |= (uint8_t)(1 << i);
        extra += b * (d->mult[j][i] > 0 ? d->mult[j][i] : 1);
    }
    int sem = d->semente[j] > 0 ? d->semente[j] : 0;
    if (carimbo) sem /= 2;
    d->semente[j] += sem;
    extra += sem;
    const char *nome = copia ? "Holográfico, como Bilhete" : carimbo ? "Carimbo » Bilhete" : "Bilhete Premiado";   // curto: o aviso cabe na mesa
    evento_t *e = evento(gera, EV_ADESIVO, j, k, j, k, sem, extra ? (carimbo ? "%s: bônus x3, +%d" : "%s: bônus em dobro, +%d")
                                                                 : "%s: nenhum bônus para dobrar", nome, extra);
    if (e) { e->ad = AD_BILHETE; e->copia = copia; e->modo = carimbo ? EVM_CARIMBO : 0; e->mascara = m; memcpy(e->por_dado, por, N_FILA); }
}

static void bilhete_de(desfecho_t *d, int j, bool gera)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        // ACHADO A02: preservado - dobra sem reconferir se o dado do Bilhete ainda vale
        if (d->bilhete[j] >> k & 1) {
            dobra_bonus(d, j, k, gera, false, false);
            if (d->bil_carimbo[j] >> k & 1) dobra_bonus(d, j, k, gera, false, true);
        }
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        // ACHADO A16: preservado - cada copia do Holografico dobra cheio (carimbo = false), a carimbada tambem
        for (int c = 0; c < d->bil_holo[j][k]; c++) dobra_bonus(d, j, k, gera, true, false);
}

