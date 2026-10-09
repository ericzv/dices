// ===========================================================================
// A regra da fila
// ===========================================================================
static int ultimo_valido(const fila_t *f, const uint8_t *est, int antes)
{
    for (int k = antes - 1; k >= 0; k--)
        if (est[k] == V_VALIDO) return k;
    return -1;
}

// Soma dos validos. O Tudo ou Nada valido com 2 dobra tudo no fim.
// O que o dado vale no placar. So o Trevo difere do que saiu: 4 vira 8, o
// resto vira 0. A queda da fila sempre compara o que saiu (F.valor).
static int valor_pontos(int j, int k)
{
    if (F[j].tipo[k] == D_TREVO) return F[j].valor[k] == 4 ? 8 : 0;
    if (F[j].tipo[k] == D_FERREIRO) return 0;
    return F[j].valor[k];
}

// Nao anulavel: nada o anula - nem a queda da fila, nem o Agouro, nem o
// Egoista, nem Carrasco ou Invejoso. Sao o Escudo, o Teimoso e o Egoista.
// Intocavel: alem disso, nenhum ataque o alcanca (nao pode ser roubado).
// So o Escudo.
static bool nao_anulavel_t(int t)
{
    int ef = TIPO[t].efeito;
    return ef == EF_ESCUDO || ef == EF_TEIMOSO;
}
static bool intocavel_t(int t) { return TIPO[t].efeito == EF_ESCUDO; }
// O Invejoso age com numero impar: 1, 3 ou 5.
static bool invejoso_age(int v) { return v == 1 || v == 3 || v == 5; }
static bool firme(int j, int k) { return nao_anulavel_t(tipo_de(j, k)); }
// Quem nao cai quando o Egoista chega: os nao anulaveis e outro Egoista.
static bool resiste_egoista(int t) { return nao_anulavel_t(t) || TIPO[t].efeito == EF_EGOISTA; }

// O maior dado do rival de j, para o Espelho: entre os que valem; se nenhum
// vale, o maior que ele jogou. 0 se o rival ainda nao jogou nada.
static int maior_do_rival(int j)
{
    const fila_t *r = &F[outro(j)];
    int vale = 0, qualquer = 0;
    for (int i = 0; i < r->lancados; i++) {
        if (r->valor[i] > qualquer) qualquer = r->valor[i];
        if (r->est[i] == V_VALIDO && r->valor[i] > vale) vale = r->valor[i];
    }
    return vale ? vale : qualquer;
}
// Algum Egoista valendo antes do dado k?
static int egoista_antes(int j, int k)
{
    for (int i = 0; i < k; i++)
        if (F[j].est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_EGOISTA) return i;
    return -1;
}
// O Maldito entra na fila, mas o valor dele nao soma para quem o jogou.
// Todo dado que vale soma para quem o jogou (o Maldito tambem, agora).
static bool pontua(int j, int k) { (void)j; (void)k; return true; }

// Soma ao vivo: valor mais bonus de Par. Multiplicadores e Moeda da sorte
// so entram no fim da rodada.
static int soma(int j, const uint8_t *est)
{
    const fila_t *f = &F[j];
    if (f->zerada) return 0;
    int t = 0;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO && pontua(j, k)) t += valor_pontos(j, k) + f->bonus[k];
    return t;
}


static void anula_fila(int j)
{
    for (int k = 0; k < F[j].lancados; k++)
        if (F[j].est[k] == V_VALIDO && !firme(j, k)) F[j].est[k] = V_ANULADO;
}

// Copiador valendo antes de k, com o mesmo numero que k tirou?
static bool copia_antes(int j, int k)
{
    for (int i = 0; i < k; i++)
        if (F[j].est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_COPIADOR
            && F[j].valor[i] == F[j].valor[k]) return true;
    return false;
}

// Tres ou mais dados valendo com o mesmo numero. Devolve o numero (0 se nao
// ha trinca) e, em masc, quais dados formam a trinca.
static int trinca(int j, const uint8_t *est, uint8_t *masc)
{
    for (int a = 0; a < F[j].lancados; a++) {
        if (est[a] != V_VALIDO) continue;
        uint8_t m = 0;
        for (int b = 0; b < F[j].lancados; b++)
            if (est[b] == V_VALIDO && F[j].valor[b] == F[j].valor[a]) m |= (uint8_t)(1 << b);
        int n = 0;
        for (uint8_t x = m; x; x >>= 1) n += x & 1;
        if (n >= 3) { if (masc) *masc = m; return F[j].valor[a]; }
    }
    return 0;
}

static bool tem_jackpot(int j, const uint8_t *est)
{
    for (int k = 0; k < F[j].lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_JACKPOT) return true;
    return false;
}

// Aplica a regra ao dado k que acabou de pousar. O veredito e curto: diz o
// que aconteceu, nao o porque em detalhe.
static void aplica_regra(int j, int k)
{
    if (k < 0 || k >= N_FILA) return;
    fila_t *f = &F[j];
    int t = tipo_de(j, k), ef = TIPO[t].efeito;
    int l = ultimo_valido(f, f->est, k);
    int v = f->valor[k];
    anulou_a = anulou_b = -1;
    anulou_masc = 0;
    resistiu = -1;
    ver_comum = false;
    bool espelhou = false, espera = false;

    // Espelho caido com 1 ou 2 vira sempre o maior dado do rival. Se o rival
    // ainda nao jogou, espera: vira no calculo do fim da rodada, e ate la
    // vale como esta, sem cair.
    if (ef == EF_ESPELHO && v <= 2) {
        int alvo = maior_do_rival(j);
        if (alvo > 0) {
            v = f->valor[k] = alvo;
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v);
            espelhou = true;
        } else {
            espera = true;
            f->espera[k] = true;
            snprintf(f->tag[k], sizeof f->tag[k], "=?");
        }
    }

    // Gemeo: tira o mesmo do ultimo dado que vale. Prensa (amuleto): 1 vira 3.
    if (ef == EF_GEMEO && l >= 0) {
        v = f->valor[k] = f->valor[l];
        snprintf(f->tag[k], sizeof f->tag[k], "=%d", v);
    }
    if (v == 1 && tem_am(j, A_PRENSA) && ef != EF_VIDRO && ef != EF_TUDO_NADA && ef != EF_ESPELHO
        && ef != EF_TREVO && ef != EF_BUMERANGUE) {
        v = f->valor[k] = 3;
        snprintf(f->tag[k], sizeof f->tag[k], "=3");
        am_disparou |= 1u << A_PRENSA;
    }

    // Vidro com 1: se estilhaca na hora. Fica anulado e derruba o dado de
    // antes, como na queda; no fim da rodada sai da bolsa.
    if (ef == EF_VIDRO && v == 1) {
        f->est[k] = V_ANULADO;
        anulou_b = k;
        if (l >= 0 && !firme(j, l)) {             // e derruba o anterior, como na queda
            f->est[l] = V_ANULADO;
            anulou_a = l;
            snprintf(veredito, sizeof veredito, "o Vidro se estilhaçou e anulou o %d", f->valor[l]);
        } else {
            if (l >= 0) resistiu = l;
            snprintf(veredito, sizeof veredito, "o Vidro tirou 1 e se estilhaçou");
        }
        ver_tipo = 1;
        f->pontos = soma(j, f->est);
        return;
    }

    // Egoista na fila: nenhum dado novo vale ao lado dele (menos o Escudo).
    int ego = egoista_antes(j, k);
    if (ego >= 0 && !resiste_egoista(t)) {
        f->est[k] = V_ANULADO;
        anulou_b = k;
        resistiu = ego;
        snprintf(veredito, sizeof veredito, "o Egoísta não divide a mesa: %d anulado", v);
        ver_tipo = 4;
        f->pontos = soma(j, f->est);
        return;
    }
    // Egoista chegando: anula todos os outros que valiam, e vale sozinho.
    if (ef == EF_EGOISTA) {
        uint8_t *est = f->est;
        est[k] = V_VALIDO;
        int n = 0;
        for (int i = 0; i < k; i++)
            if (est[i] == V_VALIDO && !resiste_egoista(f->tipo[i])) {
                est[i] = V_ANULADO;
                anulou_masc |= (uint8_t)(1 << i);
                n++;
            }
        if (n) snprintf(veredito, sizeof veredito, "Egoísta tirou %d e anulou %d dado%s", v, n, n > 1 ? "s" : "");
        else   snprintf(veredito, sizeof veredito, "Egoísta vale %d, sozinho", v);
        ver_tipo = n ? 4 : 0;
        // Ele nao se anula, mas pode cair: se sobrou antes dele um dado que
        // resiste (Escudo, Teimoso) maior que ele, a queda da sequencia vale.
        int l2 = ultimo_valido(f, est, k);
        if (l2 >= 0 && v < f->valor[l2]) {
            bool esc_l = firme(j, l2);
            if (!esc_l) { est[l2] = V_ANULADO; anulou_a = l2; }
            est[k] = V_ANULADO;
            anulou_b = k;
            resistiu = esc_l ? l2 : -1;
            snprintf(veredito, sizeof veredito, "%d < %d: o Egoísta cai", v, f->valor[l2]);
            ver_tipo = esc_l ? 2 : 1;
        }
        f->pontos = soma(j, f->est);
        return;
    }

    bool condenou = ef == EF_TUDO_NADA && v == 1 && !f->zerada;
    if (ef == EF_TUDO_NADA && v == 1) f->zerada = true;
    if (ef == EF_MALDITO && v <= 7) {          // Agouro: 7 ou menos derruba a sequencia
        anula_fila(j);
        f->est[k] = V_ANULADO;
        anulou_a = k;
        // Escudo, Teimoso e Egoista seguram: a mensagem nao diz que zerou tudo.
        int segura = -1;
        for (int i = 0; i < k; i++) if (f->est[i] == V_VALIDO) { segura = i; break; }
        if (segura >= 0) {
            resistiu = segura;
            snprintf(veredito, sizeof veredito, "%s zerou a sequência; %s segura", TIPO[t].nome,
                     TIPO[tipo_de(j, segura)].nome);
        } else
            snprintf(veredito, sizeof veredito, "%s tirou %d e zerou a sequência", TIPO[t].nome, v);
        ver_tipo = 3;
        f->pontos = soma(j, f->est);
        return;
    }

    if (!espera && l >= 0 && v < f->valor[l]) {
        // A Sentinela cai sozinha: o dado de antes fica de pe. A Luva de
        // Couro segura o primeiro da sequencia; a Rede, o de antes do ultimo.
        bool sentinela = ef == EF_SENTINELA;
        bool luva = l == 0 && tem_am(j, A_LUVA) && !firme(j, l);
        bool rede = k == f->n - 1 && tem_am(j, A_REDE) && !firme(j, l) && !sentinela && !luva;
        // Ferradura: uma vez por rodada, na primeira queda que nada mais
        // segurou (Luva, Rede, Sentinela, dado firme), o dado de antes fica.
        bool ferr = tem_am(j, A_FERRADURA) && (ferr_k[j] < 0 || ferr_k[j] == k) && !firme(j, l)
                    && !sentinela && !luva && !rede;
        if (luva) am_disparou |= 1u << A_LUVA;
        if (rede) am_disparou |= 1u << A_REDE;
        if (ferr) am_disparou |= 1u << A_FERRADURA;
        bool esc_l = firme(j, l) || sentinela || luva || rede || ferr, esc_k = firme(j, k);
        if (!esc_l) f->est[l] = V_ANULADO;
        f->est[k] = esc_k ? V_VALIDO : V_ANULADO;
        anulou_a = esc_l ? -1 : l;
        anulou_b = esc_k ? -1 : k;
        resistiu = esc_l ? l : (esc_k ? k : -1);
        if (ferr && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Ferradura segura o %d", v, f->valor[l], f->valor[l]);
        else if (sentinela && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Sentinela cai sozinha", v, f->valor[l]);
        else if (luva && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Luva de Couro segura o %d", v, f->valor[l], f->valor[l]);
        else if (rede && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Rede segura o %d", v, f->valor[l], f->valor[l]);
        else if (esc_l) snprintf(veredito, sizeof veredito, "%d < %d: %s segura", v, f->valor[l], TIPO[tipo_de(j, l)].nome);
        else if (esc_k) snprintf(veredito, sizeof veredito, "%d < %d: %s fica, %d cai", v, f->valor[l], TIPO[t].nome, f->valor[l]);
        else            snprintf(veredito, sizeof veredito, "%d é menor que %d: anulou os dois", v, f->valor[l]);
        ver_tipo = (esc_l || esc_k) ? 2 : 1;
        ver_comum = ver_tipo == 1;
    } else {
        f->est[k] = V_VALIDO;
        if (ef == EF_PAR && l >= 0 && v == f->valor[l]) {
            f->bonus[k] = 8;
            snprintf(f->tag[k], sizeof f->tag[k], "+8");
        }
        // Dobro: so o dado logo depois dele na sequencia (com o Dobro valendo).
        if (l >= 0 && l == k - 1 && TIPO[tipo_de(j, l)].efeito == EF_DOBRO && f->valor[l] <= 3) {
            f->dobra[k] = 1;
            snprintf(f->tag[k], sizeof f->tag[k], "x2");
        }
        // Ferreiro com 3 ou 4 valendo logo antes: +6 no fim, se os dois ficarem.
        if (l >= 0 && l == k - 1 && TIPO[tipo_de(j, l)].efeito == EF_FERREIRO && f->valor[l] >= 3)
            snprintf(f->tag[k], sizeof f->tag[k], "+6");
        if ((ef == EF_COBRADOR && fichas_ini[outro(j)] > fichas_ini[j])     // atras: o Cobrador cobra
            || (ef == EF_FICHAS && fichas_ini[j] > fichas_ini[outro(j)])) {   // na frente: o Fichas rende
            f->bonus[k] = 2;
            snprintf(f->tag[k], sizeof f->tag[k], "$+2");
        }
        if (ef == EF_ACUMULADOR && f->fixo[k]) {
            f->bonus[k] = (int8_t)(f->fixo[k] < 6 ? f->fixo[k] : 6);
            snprintf(f->tag[k], sizeof f->tag[k], "+%d", f->bonus[k]);
        }
        if (ef == EF_FERREIRO) snprintf(f->tag[k], sizeof f->tag[k], "=0");
        // Amuletos: Dado de Ouro (+2 no numero maximo), Peso Pena (+1 nos de
        // 2 e 4 lados).
        int am_b = 0;
        if (tem_am(j, A_OURO) && v == TIPO[t].lados && ef != EF_ESPELHO) { am_b += 2; am_disparou |= 1u << A_OURO; }
        if (tem_am(j, A_PENA) && (TIPO[t].lados == 2 || TIPO[t].lados == 4)) { am_b += 1; am_disparou |= 1u << A_PENA; }
        if (tem_am(j, A_PESADO) && TIPO[t].lados == 6) { am_b += 1; am_disparou |= 1u << A_PESADO; }
        if (tem_am(j, A_CONTRATO)) { am_b += 2; am_disparou |= 1u << A_CONTRATO; }   // Contrato Sujo
        if (tem_am(j, A_COROA) && fichas_ini[j] < fichas_ini[outro(j)]) { am_b += 1; am_disparou |= 1u << A_COROA; }
        if (j == 1 && rodada >= 5 && tem_reg(REG_FOLEGO)) am_b += 1;              // a Maratonista
        if (j == 1 && k == 0 && tem_reg(REG_SAQUE)) am_b += 1;                    // o Caubói
        if (j == 1 && virada_forca(0)) am_b += 1;                                 // a casa tem vantagem
        if (k == 0 && tem_am(j, A_CORINGA)) { am_b += 2; am_disparou |= 1u << A_CORINGA; }
        if (am_b) {
            f->bonus[k] = (int8_t)(f->bonus[k] + am_b);
            snprintf(f->tag[k], sizeof f->tag[k], "+%d", f->bonus[k]);
        }
        int vale = valor_pontos(j, k) + f->bonus[k];
        if (ef == EF_TREVO) {
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v == 4 ? 8 : 0);
            if (v == 4) snprintf(veredito, sizeof veredito, "SORTE! o Trevo vale 8");
            else        snprintf(veredito, sizeof veredito, "Trevo tirou %d: vale 0", v);
        } else if (ef == EF_MALDICAO) {
            snprintf(veredito, sizeof veredito, "Maldito vale %d e tira %d do oponente", v, v);
        } else if (ef == EF_COPIADOR) {
            snprintf(veredito, sizeof veredito, "Copiador tirou %d: cada %d seu ganha +2", v, v);
        } else if (copia_antes(j, k)) {
            snprintf(f->tag[k], sizeof f->tag[k], "+2");
            snprintf(veredito, sizeof veredito, "vale %d, igual ao Copiador: +2", vale);
        } else if (ef == EF_FERREIRO) {
            if (v >= 3) snprintf(veredito, sizeof veredito, "Ferreiro: não pontua, forja +6 no próximo");
            else        snprintf(veredito, sizeof veredito, "Ferreiro tirou %d: não pontua nem forja", v);
        } else if (ef == EF_COBRADOR && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Cobrador cobra: vale %d (+2)", vale);
        } else if (ef == EF_FICHAS && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Fichas rende: vale %d (+2)", vale);
        } else if (ef == EF_ACUMULADOR && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Acumulador vale %d (+%d guardado)", vale, f->bonus[k]);
        } else if (espelhou)          snprintf(veredito, sizeof veredito, "Espelho vira %d e vale", v);
        else if (espera)       snprintf(veredito, sizeof veredito, "Espelho espera: no fim vira o maior do oponente");
        else {
            if (f->dobra[k])       snprintf(veredito, sizeof veredito, "vale %d, x2 no fim", vale);
            else if (f->bonus[k])  snprintf(veredito, sizeof veredito, "vale %d (Par)", vale);
            else if (l < 0 && k)   snprintf(veredito, sizeof veredito, "vale %d: começa sequência nova", vale);
            else                   snprintf(veredito, sizeof veredito, "vale %d", vale);
            ver_comum = true;
        }
        ver_tipo = 0;
    }
    if (condenou) {                              // o aviso vale mais que o resto
        snprintf(veredito, sizeof veredito, "Tudo ou Nada deu 1: a rodada vale 0");
        ver_tipo = 3;
        ver_comum = false;
    } else if (ef == EF_VICIADO && f->descarte[k] && ver_tipo == 0 && !f->tag[k][0]) {
        snprintf(veredito, sizeof veredito, "Viciado: %d e %d, fica o %d", v, f->descarte[k], v);
        ver_comum = false;
    } else if (f->est[k] == V_VALIDO && trinca(j, f->est, NULL) && tem_jackpot(j, f->est)) {
        snprintf(veredito, sizeof veredito, "três iguais: JACKPOT no fim!");
        ver_tipo = 0;
        ver_comum = false;
    }
    f->pontos = soma(j, f->est);
}

static int sorteia_valor(int t, int fixo, int (*r)(int), int *menor);

// Refaz a sequencia de j do comeco ate o dado n (sem ele), com os valores que
// estao la: a regra da queda, o Egoista, o Tudo ou Nada... tudo de novo.
// Serve a Ventania (que rolou o de antes) e ao Espelho que virou no fim.
static void refaz_sequencia(int j, int n)
{
    fila_t *f = &F[j];
    for (int i = 0; i < n; i++) {
        f->est[i] = V_ESPERA;
        f->bonus[i] = 0;
        f->dobra[i] = 0;
        f->tag[i][0] = 0;
    }
    f->zerada = false;
    int lanc = f->lancados;
    for (int i = 0; i < n; i++) { f->lancados = i + 1; aplica_regra(j, i); }
    f->lancados = lanc;
}

// Um dado acabou de cair na posicao k. O Ventania, antes da regra dele, rola
// de novo o dado lancado logo antes dele e refaz a sequencia do comeco com o
// valor novo (dados podem cair ou voltar a valer). Nao rola a si mesmo nem os
// outros. 'r' e o sorteio: o da partida ou o da IA imaginando jogadas.
static void pousa(int j, int k, int (*r)(int))
{
    fila_t *f = &F[j];
    bool voltou = false;
    int tirou = f->valor[k];
    if (f->tipo[k] == D_BUMERANGUE && tirou <= 2) {          // volta e rola de novo, uma vez
        f->cru[k] = f->valor[k] = r(6);
        voltou = true;
    }
    if (f->tipo[k] != D_VENTANIA || k == 0) {
        aplica_regra(j, k);
        if (voltou && ver_tipo == 0) {
            snprintf(veredito, sizeof veredito, "o Bumerangue voltou: %d", f->valor[k]);
            ver_comum = false;
        }
        if (voltou) snprintf(f->tag[k], sizeof f->tag[k], "%d>%d", tirou, f->valor[k]);
        return;
    }
    int i = k - 1;
    if (!(f->tipo[i] == D_VIDRO && f->cru[i] == 1)) {           // vidro quebrado nao volta
        int menor;
        f->cru[i] = f->valor[i] = sorteia_valor(f->tipo[i], f->fixo[i], r, &menor);
        f->descarte[i] = (uint8_t)menor;
        f->espera[i] = false;
    }
    refaz_sequencia(j, k);
    aplica_regra(j, k);
    if (f->est[k] == V_VALIDO && ver_tipo == 0) {
        snprintf(veredito, sizeof veredito, "Ventania! o dado de antes rolou: %d", f->valor[i]);
        ver_comum = false;
    }
}
