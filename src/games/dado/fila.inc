// ===========================================================================
// A regra da fila
// ===========================================================================
// v, depois de L, derruba a sequencia? Menor cai; com o Corrimao (amuleto),
// so 1 abaixo ainda fica.
static bool cai_depois(int j, int v, int L) { return v < L && !(tem_am(j, A_CORRIMAO) && v == L - 1); }

// O ultimo que vale e que a regra da fila enxerga: a Ponte fica de fora (o
// dado depois dela compara com o de antes dela).
static int ultimo_da_regra(int j, const uint8_t *est, int antes)
{
    for (int k = antes - 1; k >= 0; k--)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito != EF_PONTE) return k;
    return -1;
}

// Soma dos validos. O Tudo ou Nada valido com 2 dobra tudo no fim.
// O que o dado vale no placar. So o Trevo difere do que saiu: 4 vira 8, o
// resto vira 0. A queda da fila sempre compara o que saiu (F.valor).
static int valor_pontos(int j, int k)
{
    if (F[j].tipo[k] == D_TREVO) return F[j].valor[k] == 4 ? 10 : 0;
    if (F[j].tipo[k] == D_FERREIRO || F[j].tipo[k] == D_SACRIFICIO || F[j].tipo[k] == D_ONDA) return 0;
    if (F[j].tipo[k] == D_PISCA && !pisca_aceso(F[j].fixo[k])) return 0;
    return F[j].valor[k];
}

// A Furia guarda o bonus no proprio dado (o fixo, com sinal): com o que ja
// tinha (g), o que fica depois de tirar v - e e o que ela soma agora.
static int furia_depois(int g, int v)
{
    int b = (int8_t)g + (v == 4 ? FURIA_GANHA : v == 1 ? -FURIA_PERDE : 0);
    return b > FURIA_MAX ? FURIA_MAX : b < -FURIA_MAX ? -FURIA_MAX : b;
}
// Os Tres Patinhos ja terminaram valendo c vezes: valendo agora, e a 3a (6a,
// 9a...) vez, e pontuam x3.
static bool patinhos_ativo(int c) { return (c + 1) % PATINHOS_CADA == 0; }
// A Meia na Lareira que guardou g e terminou anulada: o quanto fica.
static int meia_depois(int g) { return g + MEIA_GUARDA > MEIA_MAX ? MEIA_MAX : g + MEIA_GUARDA; }
// A Onda que guardou g e tirou v: o quanto fica, no maximo o que a vira.
static int onda_depois(int g, int v) { int n = g + (v > 0 ? v : 0); return n > ONDA_VIRA ? ONDA_VIRA : n; }

// A peca da bolsa que esta no lugar k da fila de j (a copia do Coringa nao
// tem), e o adesivo do lado que ficou para cima nela (0: nenhum).
static const peca_t *peca_fila(int j, int k)
{
    int i = F[j].idx[k];
    return i >= 0 && i < J[j].n_col ? &J[j].col[i] : NULL;
}
// A Onda no lugar k veio da bolsa (a copia do Coringa nao vira Tsunami).
static bool onda_da_bolsa(int j, int k) { const peca_t *p = peca_fila(j, k); return p && p->tipo == D_ONDA; }
static int ad_fila(int j, int k) { return ad_no_lado(peca_fila(j, k), F[j].lado[k]); }
// O Panetone, a cada lance: a fatia vem com 0 a 3 frutas.
static void fatia(int j, int k, int (*r)(int))
{
    F[j].fruta[k] = (uint8_t)(TIPO[F[j].tipo[k]].efeito == EF_PANETONE ? r(4) - 1 : 0);
}
// O lado que um lance deixa para cima: o d12 da Sorte no Sorte/Reves, o
// primeiro lance no Explosivo; o Vazio nao mostra (0). Fora dos lados, 0.
static int lado_do_lance(int t, int x, int menor)
{
    int ef = TIPO[t].efeito;
    if (ef == EF_SORTE_REVES) x += menor;
    else if (ef == EF_EXPLODE && x > TIPO[t].lados) x = TIPO[t].lados;
    else if (ef == EF_VAZIO) return 0;
    return x >= ad_lado_min(t) && x <= ad_lado_max(t) ? x : 0;
}
// O numero mudou depois do lance (Gemeo, Eco, Espelho, Prensa, Calibre): o
// lado de cima e o numero novo, se o dado tem esse lado.
static void lado_vira(int j, int k, int v)
{
    int t = F[j].tipo[k];
    F[j].lado[k] = (uint8_t)(v >= ad_lado_min(t) && v <= ad_lado_max(t) ? v : 0);
}

// Nao anulavel: nada o anula - nem a queda da fila, nem o Agouro, nem o
// Egoista, nem Carrasco ou Invejoso. Sao o Escudo, o Teimoso e a Sanguessuga.
// Intocavel: alem disso, nenhum ataque o alcanca (nao pode ser roubado).
// So o Escudo.
static bool nao_anulavel_t(int t)
{
    int ef = TIPO[t].efeito;
    return ef == EF_ESCUDO || ef == EF_TEIMOSO || ef == EF_SANGUESSUGA || ef == EF_VAZIO;
}
static bool intocavel_t(int t) { return TIPO[t].efeito == EF_ESCUDO; }
// O mesmo, pela posicao: o dado preso pela Ancora tambem.
static bool intocavel(int j, int k) { return intocavel_t(tipo_de(j, k)) || F[j].ancora[k]; }
// O Invejoso age com numero impar: 1, 3 ou 5.
static bool invejoso_age(int v) { return v >= 5; }   // contra o oponente; com 1 ou 2, contra o dono
// O Transparente no lado de cima tambem segura o dado, como o Teimoso.
static bool firme(int j, int k) { return nao_anulavel_t(tipo_de(j, k)) || F[j].ancora[k] || ad_fila(j, k) == AD_TRANSPARENTE; }
// Quem nao cai quando o Egoista chega: os nao anulaveis e outro Egoista.
static bool resiste_egoista(int t) { return nao_anulavel_t(t) || TIPO[t].efeito == EF_EGOISTA; }
static bool resiste_ego(int j, int k) { return resiste_egoista(tipo_de(j, k)) || F[j].ancora[k] || ad_fila(j, k) == AD_TRANSPARENTE; }

// O maior dado do rival de j, para o Espelho: entre os que valem; se nenhum
// vale, o maior que ele jogou. 0 se o rival ainda nao jogou nada.
static int maior_do_rival(int j)
{
    const fila_t *r = &F[outro(j)];
    int vale = 0, qualquer = 0;
    for (int i = 0; i < r->lancados; i++) {
        if (r->escondido[i]) continue;               // o Oculto ainda nao mostrou
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
// Soma ao vivo: valor mais bonus de Par. Multiplicadores e Moeda da sorte
// so entram no fim da rodada.
static int soma(int j, const uint8_t *est)
{
    const fila_t *f = &F[j];
    int t = 0;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO) t += valor_pontos(j, k) + f->bonus[k];
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
// Pelo menos q dados valendo de j com o mesmo numero: devolve o numero (o
// primeiro que fecha) e, em masc, quais sao. 0: nao ha.
static int mesmo_numero(int j, const uint8_t *est, int q, uint8_t *masc)
{
    for (int a = 0; a < F[j].lancados; a++) {
        if (est[a] != V_VALIDO) continue;
        uint8_t m = 0;
        for (int b = 0; b < F[j].lancados; b++)
            if (est[b] == V_VALIDO && F[j].valor[b] == F[j].valor[a]) m |= (uint8_t)(1 << b);
        int n = 0;
        for (uint8_t x = m; x; x >>= 1) n += x & 1;
        if (n >= q) { if (masc) *masc = m; return F[j].valor[a]; }
    }
    return 0;
}
static int trinca(int j, const uint8_t *est, uint8_t *masc) { return mesmo_numero(j, est, 3, masc); }

static bool tem_jackpot(int j, const uint8_t *est)
{
    for (int k = 0; k < F[j].lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_JACKPOT) return true;
    return false;
}

// Os lances de um Explosivo que somaram v: o primeiro (o 8, ou mais com o
// Remendo) e os que rolou de novo, maximos ate o ultimo ("(8+8+3)").
static void lances_explosivo(char *s, size_t n, int v, int lados, int lance1)
{
    if (lance1 < lados || lance1 >= v) lance1 = lados;
    size_t p = (size_t)snprintf(s, n, "(%d+", lance1);
    v -= lance1;
    while (v > lados && p < n) { p += (size_t)snprintf(s + p, n - p, "%d+", lados); v -= lados; }
    if (p < n) snprintf(s + p, n - p, "%d)", v);
}

// Aplica a regra ao dado k que acabou de pousar. O veredito e curto: diz o
// que aconteceu, nao o porque em detalhe.
static void aplica_regra(int j, int k)
{
    if (k < 0 || k >= N_FILA) return;
    fila_t *f = &F[j];
    if (f->carta && k == 3) { f->est[k] = V_ESPERA; return; }   // a carta do Coringa: ainda em suspensao
    int t = tipo_de(j, k), ef = TIPO[t].efeito;
    int l = ultimo_da_regra(j, f->est, k);
    int v = f->valor[k];
    anulou_a = anulou_b = -1;
    anulou_masc = 0;
    resistiu = -1;
    ver_comum = false;
    bool espelhou = false, espera = false;
    // Preso pela Ancora: o dado logo depois dela, com ela valendo.
    f->ancora[k] = k > 0 && tipo_de(j, k - 1) == D_ANCORA && f->est[k - 1] == V_VALIDO;
    // O Oculto lancado: ninguem ve o numero; fica fora da sequencia (nao cai,
    // nao derruba, o seguinte compara com o de antes dele) ate o fim.
    if (f->escondido[k]) {
        f->est[k] = V_ESPERA;
        snprintf(veredito, sizeof veredito, "Oculto: o número só aparece no fim");
        ver_tipo = 0;
        f->pontos = soma(j, f->est);
        return;
    }

    // Espelho caido com 1 ou 2 vira sempre o maior dado do rival. Se o rival
    // ainda nao jogou, espera: vira no calculo do fim da rodada, e ate la
    // vale como esta, sem cair.
    if (ef == EF_ESPELHO && v <= 2) {
        int alvo = maior_do_rival(j);
        if (alvo > 0) {
            v = f->valor[k] = alvo;
            lado_vira(j, k, v);
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v);
            espelhou = true;
        } else {
            espera = true;
            f->espera[k] = true;
            snprintf(f->tag[k], sizeof f->tag[k], "=?");
        }
    }

    // Gemeo: tira o mesmo do ultimo dado que vale, se couber nos seis lados
    // dele; acima de 6, fica com o que rolou. Prensa (amuleto): 1 vira 3.
    if (ef == EF_GEMEO && l >= 0 && f->valor[l] <= TIPO[t].lados) {
        v = f->valor[k] = f->valor[l];
        lado_vira(j, k, v);
        snprintf(f->tag[k], sizeof f->tag[k], "=%d", v);
    }
    // Eco: tira o mesmo do primeiro dado que vale (ate 6); sendo ele o primeiro,
    // ou passando de 6, fica com o que rolou.
    if (ef == EF_ECO) {
        int p0 = -1;
        for (int i = 0; i < k && p0 < 0; i++) if (f->est[i] == V_VALIDO) p0 = i;
        if (p0 >= 0 && f->valor[p0] <= TIPO[t].lados) {
            v = f->valor[k] = f->valor[p0];
            lado_vira(j, k, v);
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v);
        }
    }
    // Explosivo que explodiu: no dado o total; na etiqueta, os lances ("(8+7)").
    if (ef == EF_EXPLODE && v > TIPO[t].lados) lances_explosivo(f->tag[k], sizeof f->tag[k], v, TIPO[t].lados, remenda(peca_fila(j, k), f->lado[k]));
    // Sorte/Reves: na etiqueta, os dois lances ("12-5").
    if (ef == EF_SORTE_REVES && f->descarte[k]) snprintf(f->tag[k], sizeof f->tag[k], "%d-%d", (uint8_t)(v + f->descarte[k]), f->descarte[k]);
    if (v == 1 && tem_am(j, A_PRENSA) && ef != EF_VIDRO && ef != EF_TUDO_NADA && ef != EF_ESPELHO && ef != EF_SORTE_REVES
        && ef != EF_TREVO && ef != EF_BUMERANGUE) {
        v = f->valor[k] = 3;
        lado_vira(j, k, 3);
        snprintf(f->tag[k], sizeof f->tag[k], "=3");
        am_disparou |= AM_BIT(A_PRENSA);
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
    if (ego >= 0 && !resiste_ego(j, k)) {
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
            if (est[i] == V_VALIDO && !resiste_ego(j, i)) {
                est[i] = V_ANULADO;
                anulou_masc |= (uint8_t)(1 << i);
                n++;
            }
        if (n) snprintf(veredito, sizeof veredito, "Egoísta tirou %d e anulou %d dado%s", v, n, n > 1 ? "s" : "");
        else   snprintf(veredito, sizeof veredito, "Egoísta pontua %d, sozinho", v);
        ver_tipo = n ? 4 : 0;
        // Ele nao se anula, mas pode cair: se sobrou antes dele um dado que
        // resiste (Escudo, Teimoso) maior que ele, a queda da sequencia vale.
        int l2 = ultimo_da_regra(j, est, k);
        if (l2 >= 0 && cai_depois(j, v, f->valor[l2])) {
            bool esc_l = firme(j, l2);
            if (!esc_l) { est[l2] = V_ANULADO; anulou_a = l2; }
            est[k] = V_ANULADO;
            anulou_b = k;
            resistiu = esc_l ? l2 : -1;
            snprintf(veredito, sizeof veredito, "%d < %d: o Egoísta é anulado", v, f->valor[l2]);
            ver_tipo = esc_l ? 2 : 1;
        }
        f->pontos = soma(j, f->est);
        return;
    }

    bool condenou = ef == EF_TUDO_NADA && v == 1;    // -5 em cada dado seu no fim
    if (ef == EF_MALDITO && v <= 7) {          // Agouro: 7 ou menos derruba a sequencia
        anula_fila(j);
        f->est[k] = V_ANULADO;
        anulou_a = k;
        // Escudo, Teimoso e Egoista seguram: a mensagem nao diz que zerou tudo.
        int segura = -1;
        for (int i = 0; i < k; i++) if (f->est[i] == V_VALIDO) { segura = i; break; }
        if (segura >= 0) {
            resistiu = segura;
            snprintf(veredito, sizeof veredito, "%s zerou a sequência; %s continua válido", TIPO[t].nome,
                     TIPO[tipo_de(j, segura)].nome);
        } else
            snprintf(veredito, sizeof veredito, "%s tirou %d e zerou a sequência", TIPO[t].nome, v);
        ver_tipo = 3;
        f->pontos = soma(j, f->est);
        return;
    }

    bool corrimao = !espera && l >= 0 && v == f->valor[l] - 1 && tem_am(j, A_CORRIMAO);
    if (!espera && ef != EF_PONTE && ef != EF_VAZIO && ef != EF_SANGUESSUGA && l >= 0 && cai_depois(j, v, f->valor[l])) {
        // (a Ponte, o Vazio e a Sanguessuga nunca caem)
        // A Sentinela cai sozinha: o dado de antes fica de pe. A Luva de
        // Couro segura o primeiro da sequencia; a Rede, o de antes do ultimo.
        bool sentinela = ef == EF_SENTINELA;
        bool luva = l == 0 && tem_am(j, A_LUVA) && !firme(j, l);
        bool rede = k == f->n - 1 && tem_am(j, A_REDE) && !firme(j, l) && !sentinela && !luva;
        // Ferradura: uma vez por rodada, na primeira queda que nada mais
        // segurou (Luva, Rede, Sentinela, dado firme), o dado de antes fica.
        bool ferr = tem_am(j, A_FERRADURA) && (ferr_k[j] < 0 || ferr_k[j] == k) && !firme(j, l)
                    && !sentinela && !luva && !rede;
        if (luva) am_disparou |= AM_BIT(A_LUVA);
        if (rede) am_disparou |= AM_BIT(A_REDE);
        if (ferr) am_disparou |= AM_BIT(A_FERRADURA);
        bool esc_l = firme(j, l) || sentinela || luva || rede || ferr, esc_k = firme(j, k);
        if (!esc_l) f->est[l] = V_ANULADO;
        f->est[k] = esc_k ? V_VALIDO : V_ANULADO;
        anulou_a = esc_l ? -1 : l;
        anulou_b = esc_k ? -1 : k;
        resistiu = esc_l ? l : (esc_k ? k : -1);
        if (ferr && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Ferradura mantém o %d válido", v, f->valor[l], f->valor[l]);
        else if (sentinela && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: só a Sentinela é anulada", v, f->valor[l]);
        else if (luva && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Luva de Couro mantém o %d válido", v, f->valor[l], f->valor[l]);
        else if (rede && !esc_k) snprintf(veredito, sizeof veredito, "%d < %d: a Rede mantém o %d válido", v, f->valor[l], f->valor[l]);
        else if (esc_l) snprintf(veredito, sizeof veredito, "%d < %d: %s continua válido", v, f->valor[l], TIPO[tipo_de(j, l)].nome);
        else if (esc_k) snprintf(veredito, sizeof veredito, "%d < %d: %s continua válido, o %d é anulado", v, f->valor[l], TIPO[t].nome, f->valor[l]);
        else            snprintf(veredito, sizeof veredito, "%d é menor que %d: anulou os dois", v, f->valor[l]);
        ver_tipo = (esc_l || esc_k) ? 2 : 1;
        ver_comum = ver_tipo == 1;
    } else {
        f->est[k] = V_VALIDO;
        if (ef == EF_PAR && l >= 0 && v == f->valor[l]) {
            f->bonus[k] = 6;
            snprintf(f->tag[k], sizeof f->tag[k], "+6");
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
            f->bonus[k] = 4;
            snprintf(f->tag[k], sizeof f->tag[k], "$+4");
        }
        if ((ef == EF_ACUMULADOR || ef == EF_AUTOFAGIA) && f->fixo[k]) {
            int teto = ef == EF_AUTOFAGIA ? AUTO_MAX : ACUM_MAX;
            f->bonus[k] = (int8_t)(f->fixo[k] < teto ? f->fixo[k] : teto);
            snprintf(f->tag[k], sizeof f->tag[k], "+%d", f->bonus[k]);
        }
        // Furia: o bonus guardado, ja com este lance (4 guarda +2, 1 perde 1).
        if (ef == EF_FURIA && furia_depois(f->fixo[k], v)) {
            f->bonus[k] = (int8_t)furia_depois(f->fixo[k], v);
            snprintf(f->tag[k], sizeof f->tag[k], "%+d", f->bonus[k]);
        }
        if (ef == EF_FERREIRO || ef == EF_SACRIFICIO) snprintf(f->tag[k], sizeof f->tag[k], "=0");
        // Onda: nao pontua; a etiqueta diz quanto ela tera guardado se ficar.
        if (ef == EF_ONDA) snprintf(f->tag[k], sizeof f->tag[k], "%d/%d", onda_depois(f->fixo[k], v), ONDA_VIRA);
        if (ef == EF_PATINHOS && patinhos_ativo(f->fixo[k])) snprintf(f->tag[k], sizeof f->tag[k], "x3");
        // Panetone: as frutas da fatia somam no que ele tirou.
        if (ef == EF_PANETONE && f->fruta[k]) {
            f->bonus[k] = (int8_t)(f->bonus[k] + f->fruta[k]);
            snprintf(f->tag[k], sizeof f->tag[k], "+%d", f->bonus[k]);
        }
        if (ef == EF_PISCA) snprintf(f->tag[k], sizeof f->tag[k], pisca_aceso(f->fixo[k]) ? "x2" : "=0");
        if (ef == EF_MEIA && f->fixo[k]) snprintf(f->tag[k], sizeof f->tag[k], "+%d", f->fixo[k]);
        // Colecionador com 6: a metade dos dados que o dono tem (os Fantasmas contam).
        if (ef == EF_COLECIONADOR && v == 6) {
            int tem = 0;
            for (int i = 0; i < J[j].n_col; i++) tem += J[j].onde[i] != COMIDO;
            f->bonus[k] = (int8_t)(f->bonus[k] + tem / 2);
            snprintf(f->tag[k], sizeof f->tag[k], "+%d", f->bonus[k]);
        }
        // Amuletos: Dado de Ouro (+2 no numero maximo), Peso Pena (+1 nos de
        // 2 e 4 lados).
        int am_b = 0;
        if (tem_am(j, A_OURO) && v == TIPO[t].lados && ef != EF_ESPELHO) { am_b += 2; am_disparou |= AM_BIT(A_OURO); }
        if (tem_am(j, A_PENA) && (TIPO[t].lados == 2 || TIPO[t].lados == 4)) { am_b += 1; am_disparou |= AM_BIT(A_PENA); }
        if (tem_am(j, A_PESADO) && TIPO[t].lados == 6) { am_b += 1; am_disparou |= AM_BIT(A_PESADO); }
        if (tem_am(j, A_CONTRATO)) { am_b += 2; am_disparou |= AM_BIT(A_CONTRATO); }   // Contrato Sujo
        if (tem_am(j, A_COROA) && fichas_ini[j] < fichas_ini[outro(j)]) { am_b += 1; am_disparou |= AM_BIT(A_COROA); }
        if (j == 1 && rodada >= 6 && tem_reg(REG_FOLEGO)) am_b += 1;              // a Maratonista
        if (j == 1 && k == 0 && tem_reg(REG_SAQUE)) am_b += 1;                    // o Caubói
        if (j == 1 && virada_forca(0)) am_b += 1;                                 // a casa tem vantagem
        if (tem_am(j, A_DESFORRA) && J[j].perdeu_ultima) { am_b += 1; am_disparou |= AM_BIT(A_DESFORRA); }
        if (j == 1 && tem_reg(REG_MALUCO)) am_b += maluco_lado;                   // a moeda do Maluco
        if (k == 0 && tem_am(j, A_CORINGA)) { am_b += 2; am_disparou |= AM_BIT(A_CORINGA); }
        if (tem_am(j, A_TRINCA) && v == 3 && !espera) { am_b += 3; am_disparou |= AM_BIT(A_TRINCA); }
        if (corrimao) am_disparou |= AM_BIT(A_CORRIMAO);
        if (am_b) {
            f->bonus[k] = (int8_t)(f->bonus[k] + am_b);
            snprintf(f->tag[k], sizeof f->tag[k], "%+d", f->bonus[k]);
        }
        int vale = valor_pontos(j, k) + f->bonus[k];
        if (ef == EF_TREVO) {
            snprintf(f->tag[k], sizeof f->tag[k], "=%d", v == 4 ? 10 : 0);
            if (v == 4) snprintf(veredito, sizeof veredito, "SORTE! o Trevo pontua 10");
            else        snprintf(veredito, sizeof veredito, "Trevo tirou %d: pontua 0", v);
        } else if (ef == EF_MALDICAO) {
            snprintf(veredito, sizeof veredito, "Maldito pontua %d e tira %d do oponente", v, v);
        } else if (ef == EF_COPIADOR) {
            snprintf(veredito, sizeof veredito, "Copiador tirou %d: cada %d seu ganha +2", v, v);
        } else if (copia_antes(j, k)) {
            snprintf(f->tag[k], sizeof f->tag[k], "+2");
            snprintf(veredito, sizeof veredito, "pontua %d, igual ao Copiador: +2", vale);
        } else if (ef == EF_SACRIFICIO) {
            snprintf(veredito, sizeof veredito, "Sacrifício: não pontua; no fim, x2 no seu menor");
        } else if (ef == EF_VAZIO) {
            snprintf(veredito, sizeof veredito, "Vazio: pontua 0; depois dele, qualquer número é válido");
        } else if (ef == EF_COLECIONADOR && v == 6) {
            snprintf(veredito, sizeof veredito, "Colecionador tirou 6: +%d", f->bonus[k]);
        } else if (ef == EF_FERREIRO) {
            if (v >= 3) snprintf(veredito, sizeof veredito, "Ferreiro: não pontua, forja +6 no próximo");
            else        snprintf(veredito, sizeof veredito, "Ferreiro tirou %d: não pontua nem forja", v);
        } else if (ef == EF_COBRADOR && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Cobrador cobra: pontua %d (+4)", vale);
        } else if (ef == EF_FICHAS && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Fichas rende: pontua %d (+4)", vale);
        } else if ((ef == EF_ACUMULADOR || ef == EF_AUTOFAGIA) && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "%s pontua %d (+%d guardado)", TIPO[t].nome, vale, f->bonus[k]);
        } else if (ef == EF_FURIA && (v == 4 || v == 1)) {
            if (v == 4) snprintf(veredito, sizeof veredito, "Fúria! guarda +%d: pontua %d (%+d)", FURIA_GANHA, vale, f->bonus[k]);
            else        snprintf(veredito, sizeof veredito, "Fúria tirou 1: perde %d, pontua %d (%+d)", FURIA_PERDE, vale, f->bonus[k]);
        } else if (ef == EF_FURIA && f->bonus[k]) {
            snprintf(veredito, sizeof veredito, "Fúria pontua %d (%+d guardado)", vale, f->bonus[k]);
        } else if (ef == EF_ONDA) {
            int g = onda_depois(f->fixo[k], v);
            if (g >= ONDA_VIRA) snprintf(veredito, sizeof veredito, "Onda chega a %d: se ficar, vira Tsunami!", ONDA_VIRA);
            else snprintf(veredito, sizeof veredito, "Onda: não pontua, guarda %d (%d de %d)", v, g, ONDA_VIRA);
        } else if (ef == EF_PATINHOS && patinhos_ativo(f->fixo[k])) {
            snprintf(veredito, sizeof veredito, "Três Patinhos: a %dª vez, x3 no fim!", f->fixo[k] + 1);
        } else if (ef == EF_PANETONE) {
            if (f->fruta[k]) snprintf(veredito, sizeof veredito, "Panetone com %d fruta%s: pontua %d", f->fruta[k],
                                      f->fruta[k] > 1 ? "s" : "", vale);
            else snprintf(veredito, sizeof veredito, "Panetone sem frutas: pontua %d", vale);
        } else if (ef == EF_PISCA) {
            if (pisca_aceso(f->fixo[k])) snprintf(veredito, sizeof veredito, "Pisca-pisca aceso: x2 no fim");
            else snprintf(veredito, sizeof veredito, "Pisca-pisca apagado: pontua 0");
        } else if (ef == EF_MEIA && f->fixo[k]) {
            snprintf(veredito, sizeof veredito, "Meia na Lareira: se ficar, paga +%d", f->fixo[k]);
        } else if (ef == EF_EXPLODE && v > TIPO[t].lados) {
            char lances[24];
            lances_explosivo(lances, sizeof lances, v, TIPO[t].lados, remenda(peca_fila(j, k), f->lado[k]));
            snprintf(veredito, sizeof veredito, "Explosivo! %s pontua %d", lances, vale);
        } else if (espelhou)          snprintf(veredito, sizeof veredito, "Espelho vira %d e é válido", v);
        else if (espera)       snprintf(veredito, sizeof veredito, "Espelho espera: no fim vira o maior do oponente");
        else if (corrimao)     snprintf(veredito, sizeof veredito, "%d, 1 abaixo do %d: o Corrimão mantém os dois válidos", v, f->valor[l]);
        else {
            if (f->dobra[k])       snprintf(veredito, sizeof veredito, "pontua %d, x2 no fim", vale);
            else if (f->bonus[k] && ef == EF_PAR) snprintf(veredito, sizeof veredito, "pontua %d (Par)", vale);
            else if (f->bonus[k])  snprintf(veredito, sizeof veredito, "pontua %d (%+d)", vale, f->bonus[k]);
            else if (l < 0 && k)   snprintf(veredito, sizeof veredito, "pontua %d: começa sequência nova", vale);
            else                   snprintf(veredito, sizeof veredito, "pontua %d", vale);
            ver_comum = true;
        }
        ver_tipo = 0;
    }
    // Sanguessuga: ao pousar, anula o seu dado de antes que ainda vale (o
    // ultimo valendo antes dela, qualquer que seja). Ela nunca cai: menor ou
    // maior, so suga; e a sequencia segue a partir dela.
    if (ef == EF_SANGUESSUGA && f->est[k] == V_VALIDO) {
        int a = -1;
        for (int i = k - 1; i >= 0 && a < 0; i--) if (f->est[i] == V_VALIDO) a = i;
        if (a >= 0 && !firme(j, a)) {
            f->est[a] = V_ANULADO;
            anulou_a = a;
            snprintf(veredito, sizeof veredito, "Sanguessuga suga o %d: anulado", f->valor[a]);
            ver_tipo = 4;
            ver_comum = false;
        } else if (a >= 0) {
            resistiu = a;
            snprintf(veredito, sizeof veredito, "Sanguessuga não suga o %s", TIPO[tipo_de(j, a)].nome);
            ver_comum = false;
        }
    }
    if (condenou) {                              // o aviso vale mais que o resto
        snprintf(veredito, sizeof veredito, "Tudo ou Nada deu 1: -5 em cada dado seu no fim");
        ver_tipo = 3;
        ver_comum = false;
    } else if (ef == EF_VICIADO && f->descarte[k] && ver_tipo == 0 && !f->tag[k][0]) {
        snprintf(veredito, sizeof veredito, "Viciado: %d e %d, fica o %d", v, f->descarte[k], v);
        ver_comum = false;
    } else if (ef == EF_SORTE_REVES && f->descarte[k] && ver_tipo == 0) {
        snprintf(veredito, sizeof veredito, "Sorte %d, Revés %d: pontua %d", v + f->descarte[k], f->descarte[k], v);
        ver_comum = false;
    } else if (f->est[k] == V_VALIDO && trinca(j, f->est, NULL) && tem_jackpot(j, f->est)) {
        snprintf(veredito, sizeof veredito, "três iguais: JACKPOT no fim!");
        ver_tipo = 0;
        ver_comum = false;
    } else if (f->est[k] == V_VALIDO && ver_tipo == 0 && mesmo_numero(j, f->est, 2, NULL) == v && tem_jackpot(j, f->est)) {
        snprintf(veredito, sizeof veredito, "dois iguais: Jackpot +4 no fim");
        ver_comum = false;
    }
    f->pontos = soma(j, f->est);
}

static int sorteia_valor(int t, int fixo, int (*r)(int), int *menor);
static int sorteia_peca(int t, int fixo, const peca_t *p, int (*r)(int), int *menor, int *lado, int *antes);

// Refaz a sequencia de j do comeco ate o dado n (sem ele), com os valores que
// estao la: a regra da queda, o Egoista, o Tudo ou Nada... tudo de novo.
// Serve a Ventania (que rolou o primeiro) e ao Espelho que virou no fim.
static void refaz_sequencia(int j, int n)
{
    fila_t *f = &F[j];
    for (int i = 0; i < n; i++) {
        f->est[i] = V_ESPERA;
        f->bonus[i] = 0;
        f->dobra[i] = 0;
        f->tag[i][0] = 0;
    }
    int lanc = f->lancados;
    for (int i = 0; i < n; i++) { f->lancados = i + 1; aplica_regra(j, i); }
    f->lancados = lanc;
}

// Algum Oculto de j ainda escondido?
static bool tem_escondido(int j)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) if (F[j].escondido[k]) return true;
    return false;
}

// Um dado acabou de cair na posicao k. O Ventania, antes da regra dele, rola
// de novo o primeiro dado da sequencia e refaz a sequencia do comeco com o
// valor novo (dados podem cair ou voltar a valer). Nao rola a si mesmo nem os
// outros. 'r' e o sorteio: o da partida ou o da IA imaginando jogadas.
static void pousa(int j, int k, int (*r)(int))
{
    fila_t *f = &F[j];
    f->escondido[k] = TIPO[f->tipo[k]].efeito == EF_OCULTO;    // o Oculto cai virado
    bool voltou = false;
    int tirou = f->valor[k];
    if (f->tipo[k] == D_BUMERANGUE && tirou <= 2) {          // volta e rola de novo, uma vez
        int x = r(6);
        f->lado[k] = (uint8_t)x;
        f->cru[k] = f->valor[k] = remenda(peca_fila(j, k), x);   // o Remendo vale no segundo lance tambem
        voltou = true;
    }
    // Calibre valendo logo antes: o que ele tirou sobe 1, ate o maximo dele.
    int mira = f->valor[k];
    f->calibre[k] = false;
    if (k > 0 && tipo_de(j, k - 1) == D_CALIBRE && f->est[k - 1] == V_VALIDO && f->tipo[k] != D_QUEBRADO && f->tipo[k] != D_FANTASMA) {
        int t = f->tipo[k], teto = TIPO[t].efeito == EF_SORTE_REVES ? TIPO[t].lados - 1 : TIPO[t].lados;
        if (mira < teto) {
            f->cru[k] = f->valor[k] = mira + 1;
            f->calibre[k] = true;
            if (f->lado[k] && f->lado[k] < ad_lado_max(t)) f->lado[k]++;     // o dado mostra um lado acima
        }
    }
    if (f->tipo[k] != D_VENTANIA || k == 0) {
        aplica_regra(j, k);
        if (voltou && ver_tipo == 0) {
            snprintf(veredito, sizeof veredito, "o Bumerangue voltou: %d", f->valor[k]);
            ver_comum = false;
        }
        if (voltou) snprintf(f->tag[k], sizeof f->tag[k], "%d>%d", (int8_t)tirou, (int8_t)f->valor[k]);
        if (f->calibre[k] && !f->tag[k][0]) snprintf(f->tag[k], sizeof f->tag[k], "%d>%d", (int8_t)mira, (int8_t)(mira + 1));
        if (f->calibre[k] && ver_tipo == 0 && !voltou) {
            snprintf(veredito, sizeof veredito, "Calibre: o %d vira %d", mira, mira + 1);
            ver_comum = false;
        }
        if (f->presente[k] && ver_comum) {           // o que saiu do Presente (e fica no lugar dele)
            snprintf(veredito, sizeof veredito, f->presente[k] == 3 ? "o embrulho era o seu %s: pontua %d"
                                                                    : "do Presente, um %s: pontua %d e fica",
                     TIPO[f->tipo[k]].nome, valor_pontos(j, k) + f->bonus[k]);
            ver_comum = false;
        }
        return;
    }
    int i = 0;                                                   // o primeiro da sequencia
    if (!(f->tipo[i] == D_VIDRO && f->cru[i] == 1)) {           // vidro quebrado nao volta
        int menor, lado, antes;
        f->cru[i] = f->valor[i] = sorteia_peca(f->tipo[i], f->fixo[i], peca_fila(j, i), r, &menor, &lado, &antes);
        f->lado[i] = (uint8_t)lado;
        fatia(j, i, r);
        f->descarte[i] = (uint8_t)menor;
        f->espera[i] = false;
    }
    refaz_sequencia(j, k);
    aplica_regra(j, k);
    if (f->est[k] == V_VALIDO && ver_tipo == 0) {
        snprintf(veredito, sizeof veredito, "Ventania! o primeiro dado rolou: %d", f->valor[i]);
        ver_comum = false;
    }
}
