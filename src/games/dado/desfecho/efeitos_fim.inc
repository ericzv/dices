// ===========================================================================
// EFEITOS DO FIM: a conta dos pontos de uma fila (pontos_fila) e os efeitos
// de cada jogador sobre a propria fila - os bonus por dado, a Ficha de Fogo,
// os multiplicadores e os pontos fixos (efeitos_do_fim).
// Le: F, d, J[] (o nome, as rodadas perdidas, os adesivos dos dados), as
// fichas do comeco da rodada, o pote, a rodada, os amuletos e os trunfos.
// Grava: d (bonus, mult, queima, semente, delta, atacados, agiota, medusa,
// garimpo, onda, meia, balao) e ev[]. Nao grava F nem a bolsa.
// Quem chama: calcula_desfecho e a Saideira (orquestra.inc); pontos_fila
// serve tambem ao placar visivel, e ampulheta ao poder armado.
// ===========================================================================

// Pontos de uma fila dados o estado, os bonus e os multiplicadores do fim.
// Multiplicadores se SOMAM (x2, x3, x2 = x7) e valem sobre valor + bonus.
static int pontos_fila(int j, const uint8_t *est, const int8_t *bonus, const int8_t *mult)
{
    int t = -lapis_pena(j);                      // o Estojo de Lapis: a rodada comeca com -4
    for (int k = 0; k < F[j].lancados; k++) {
        if (est[k] != V_VALIDO) continue;
        if (universo(j, k)) { t += F[j].valor[k]; continue; }   // nada mexe no Universo
        int base = valor_pontos(j, k) + F[j].bonus[k] + (bonus ? bonus[k] : 0);
        int m = (mult && mult[k] > 0) ? mult[k] : 1;
        t += base * m;
    }
    return t;
}

// De onde vem cada multiplicador, para o texto do evento: "Dobro x2 no 5 ·
// Fartura x2 nos 1 e 2: 13 vira 25". curto: so os nomes ("Dobro, Fartura").
enum { MULT_DOBRO, MULT_FARTURA, MULT_VIUVA, MULT_AZARAO, MULT_ESPERANCA, MULT_SACRIFICIO, MULT_BARAO,
       MULT_PIRAMIDE, MULT_ABRE_ALAS, MULT_PATINHOS, MULT_PISCA, MULT_N };
static void texto_mult(char *s, size_t n, int j, const uint8_t *de, int n_far, bool curto)
{
    const char *nome[MULT_N] = { "Dobro", "Fartura", "Viúva Negra", "Azarão", "Esperança", "Sacrifício", J[1].nome,
                                 "Pirâmide", "Abre-Alas", "Três Patinhos", "Pisca-pisca" };
    int x[MULT_N] = { 2, 2 * n_far, 2, 2, 2, 2, 2, 2, 2, 3, 2 };
    size_t p = 0;
    s[0] = 0;
    for (int q = 0; q < MULT_N && p < n; q++) {
        if (!de[q]) continue;
        if (p) p += (size_t)snprintf(s + p, n - p, curto ? ", " : " · ");
        if (p >= n) break;
        p += (size_t)snprintf(s + p, n - p, "%s", nome[q]);
        if (curto || p >= n) continue;
        p += (size_t)snprintf(s + p, n - p, " x%d", x[q]);
        if (q == MULT_VIUVA || q == MULT_AZARAO || q == MULT_ESPERANCA || q == MULT_PATINHOS || q == MULT_PISCA
            || p >= n) continue;                 // ele mesmo
        int vals[N_FILA], nv = 0;
        for (int k = 0; k < N_FILA; k++) if (de[q] >> k & 1) vals[nv++] = F[j].valor[k];
        p += (size_t)snprintf(s + p, n - p, nv > 1 ? " nos" : " no");
        for (int i = 0; i < nv && p < n; i++)
            p += (size_t)snprintf(s + p, n - p, i == 0 ? " %d" : (i == nv - 1 ? " e %d" : ", %d"), vals[i]);
    }
}

// A Ampulheta: +1 a cada 3 rodadas ja jogadas, ate +4.
static int ampulheta(void) { int n = (rodada - 1) / 3; return n < 0 ? 0 : n > 4 ? 4 : n; }

// O fim de j, 1: os bonus que entram antes dos multiplicadores (Moeda da
// sorte, Copiador, Treno, Duende, Rena, Dupla, Ferreiro, Sentinela e Tudo
// ou Nada).
static void fim_bonus(desfecho_t *d, int j, bool gera)
{
    const fila_t *f = &F[j];
    const uint8_t *est = d->est[j];

    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_SORTE) continue;
        int face = f->valor[k];
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        for (int i = 0; i < f->lancados; i++) {
            if (est[i] != V_VALIDO) continue;
            bool impar = f->valor[i] % 2 == 1;
            // ACHADO A34: preservado - laco sem a guarda < N_FILA (seguro enquanto lancados <= N_FILA)
            if (face == 1 && impar)  { d->bonus[j][i] += 1; mask |= 1 << i; por[i] = 1; }
            if (face == 2 && !impar) { d->bonus[j][i] += 1; mask |= 1 << i; por[i] = 1; }
        }
        const char *txt = face == 1 ? "Moeda sorte deu 1: +1 nos ímpares" : "Moeda sorte deu 2: +1 nos pares";
        evento_t *e = mask ? evento(gera, EV_BONUS, j, k, 0, 0, 0, "%s", txt) : NULL;
        if (e) { e->mascara = mask; memcpy(e->por_dado, por, sizeof por); }
    }

    // Copiador: +COPIADOR_CADA em cada outro dado seu com o mesmo numero que ele.
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_COPIADOR) continue;
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        int nl = f->lancados < N_FILA ? f->lancados : N_FILA;
        for (int i = 0; i < nl; i++) {
            if (i == k || est[i] != V_VALIDO || f->valor[i] != f->valor[k]) continue;
            d->bonus[j][i] += COPIADOR_CADA; mask |= (uint8_t)(1 << i); por[i] = COPIADOR_CADA;
        }
        evento_t *e = mask ? evento(gera, EV_BONUS, j, k, 0, 0, 0, "Copiador %d: +%d em cada %d",
                                    f->valor[k], COPIADOR_CADA, f->valor[k]) : NULL;
        if (e) { e->mascara = mask; memcpy(e->por_dado, por, sizeof por); }
    }

    // Treno valendo: puxa os seus dados valendo que vieram antes dele - o
    // primeiro pontua +1, o segundo +2, o terceiro +3.
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_TRENO) continue;
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        int n = 0;
        for (int i = 0; i < k; i++)
            if (est[i] == V_VALIDO) { n++; d->bonus[j][i] += (int8_t)n; por[i] = (int8_t)n; mask |= (uint8_t)(1 << i); }
        evento_t *e = !mask ? NULL : n == 1 ? evento(gera, EV_BONUS, j, k, 0, 0, 0, "Trenó puxa o dado de antes: +1")
                                           : evento(gera, EV_BONUS, j, k, 0, 0, 0, "Trenó puxa %d dados: +1 a +%d", n, n);
        if (e) { e->mascara = mask; memcpy(e->por_dado, por, sizeof por); }
    }

    // O Duende (trunfo): cada dado dele valendo ganha +1 por outro dado do
    // mesmo tipo valendo na sequencia dele (a linha de montagem).
    if (j == 1 && tem_reg(REG_DUENDE)) {
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        for (int k = 0; k < f->lancados && k < N_FILA; k++) {
            if (est[k] != V_VALIDO) continue;
            int n = 0;
            for (int i = 0; i < f->lancados && i < N_FILA; i++) n += i != k && est[i] == V_VALIDO && tipo_de(j, i) == tipo_de(j, k);
            if (n) { d->bonus[j][k] += (int8_t)n; por[k] = (int8_t)n; mask |= (uint8_t)(1 << k); }
        }
        evento_t *e = mask ? evento(gera, EV_BONUS, j, 0xFF, 0, 0, 0, "Linha de montagem: +1 por dado igual") : NULL;
        if (e) { e->mascara = mask; memcpy(e->por_dado, por, sizeof por); }
    }

    // Rena valendo: +1 no seu dado lancado logo antes e no logo depois dela,
    // se terminarem valendo. Duas Renas lado a lado puxam uma a outra.
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_RENA) continue;
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        for (int i = k - 1; i <= k + 1; i += 2)
            if (i >= 0 && i < f->lancados && i < N_FILA && est[i] == V_VALIDO) {
                d->bonus[j][i] += 1; por[i] = 1; mask |= (uint8_t)(1 << i);
            }
        evento_t *e = mask ? evento(gera, EV_BONUS, j, k, 0, 0, 0, "%s", (mask & (mask - 1)) ? "Rena puxa os dois vizinhos: +1"
                                                                                       : "Rena puxa o vizinho: +1") : NULL;
        if (e) { e->mascara = mask; memcpy(e->por_dado, por, sizeof por); }
    }

    // Dupla (amuleto): +2 em cada dado seu que repete o numero de outro seu.
    if (tem_am(j, A_DUPLA)) {
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        int nl = f->lancados < N_FILA ? f->lancados : N_FILA;
        for (int k = 0; k < nl; k++) {
            if (est[k] != V_VALIDO) continue;
            for (int i = 0; i < nl; i++)
                if (i != k && est[i] == V_VALIDO && f->valor[i] == f->valor[k]) {
                    d->bonus[j][k] += 2; mask |= (uint8_t)(1 << k); por[k] = 2;
                    break;
                }
        }
        evento_t *e = mask ? evento(gera, EV_BONUS, j, 0xFF, 0, 0, 0, "Dupla: +2 nos números repetidos") : NULL;
        if (e) { e->mascara = mask; memcpy(e->por_dado, por, sizeof por); }
    }

    // Ferreiro valendo com 4: +FERREIRO_FORJA no dado logo depois dele, se este termina valendo.
    for (int k = 0; k + 1 < f->lancados && k + 1 < N_FILA; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_FERREIRO || f->valor[k] < FERREIRO_ARMA) continue;
        if (est[k + 1] != V_VALIDO) continue;
        d->bonus[j][k + 1] += FERREIRO_FORJA;
        evento_t *e = evento(gera, EV_BONUS, j, k, 0, 0, 0, "Ferreiro forja +%d no %d", FERREIRO_FORJA, f->valor[k + 1]);
        if (e) { e->mascara = (uint8_t)(1 << (k + 1)); e->por_dado[k + 1] = FERREIRO_FORJA; }
    }

    // Sentinela que caiu pela sequencia protegendo o dado de antes: ele ganha
    // +SENTINELA_PTS, se terminar valendo.
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        int l = f->guarda[k] - 1;
        if (l < 0 || l >= N_FILA || TIPO[tipo_de(j, k)].efeito != EF_SENTINELA || est[l] != V_VALIDO) continue;
        d->bonus[j][l] += SENTINELA_PTS;
        evento_t *e = evento(gera, EV_BONUS, j, k, 0, 0, 0, "Sentinela cai protegendo: +%d no %d", SENTINELA_PTS, f->valor[l]);
        if (e) { e->mascara = (uint8_t)(1 << l); e->por_dado[l] = SENTINELA_PTS; }
    }

    // Tudo ou Nada: com 2 (valendo), +5 em cada dado seu valendo; com 1
    // (valendo ou nao), -5 em cada um. Entra antes dos multiplicadores.
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (TIPO[tipo_de(j, k)].efeito != EF_TUDO_NADA) continue;
        int s = f->cru[k] == 1 ? -5 : (est[k] == V_VALIDO && f->cru[k] == 2 ? 5 : 0);
        if (!s) continue;
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        for (int i = 0; i < f->lancados && i < N_FILA; i++)
            if (est[i] == V_VALIDO) { d->bonus[j][i] += (int8_t)s; mask |= (uint8_t)(1 << i); por[i] = (int8_t)s; }
        evento_t *e = mask ? evento(gera, EV_BONUS, j, k, 0, 0, 0, "%s", s > 0 ? "Tudo ou Nada deu 2: +5 em cada dado"
                                                                          : "Tudo ou Nada deu 1: -5 em cada dado") : NULL;
        if (e) { e->mascara = mask; memcpy(e->por_dado, por, sizeof por); }
    }
}

// O fim de j, 2: se ha Fartura valendo, e quantas.
static void fim_mult_prepara(const desfecho_t *d, int j, bool *fartura, int *n_far)
{
    const fila_t *f = &F[j];
    const uint8_t *est = d->est[j];

    *fartura = false;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_FARTURA) *fartura = true;
    *n_far = 0;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_FARTURA) (*n_far)++;
}

// O fim de j, 3: a Ficha de Fogo.
// Ficha de Fogo: com 2, +10; com 1, nada. Nos dois casos o maior dado seu
// que pontua (sem contar ela) queima no fim. Duas fichas queimam dois
// dados diferentes. O dado com a capa do Impermeavel e alvo como os
// outros, mas nao queima - e o fogo nao passa para outro.
static void fim_fogo(desfecho_t *d, int j, bool gera)
{
    const fila_t *f = &F[j];
    const uint8_t *est = d->est[j];

    uint8_t mirado[N_FILA] = { 0 };
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_FOGO) continue;
        int a = -1;
        for (int i = 0; i < f->lancados; i++) {
            if (i == k || est[i] != V_VALIDO || mirado[i]) continue;
            if (TIPO[tipo_de(j, i)].efeito == EF_FOGO) continue;
            if (a < 0 || f->valor[i] > f->valor[a]) a = i;
        }
        if (a < 0) continue;
        mirado[a] = 1;
        bool capa = impermeavel(j, a);
        if (!capa) d->queima[j][a] = 1;
        evento_t *e;
        if (f->cru[k] == 2) {
            d->semente[j] += 10;
            e = evento(gera, EV_FOGO, j, k, j, a, 10, capa ? "Ficha Fogo deu 2: +10, mas o %d está protegido"
                                                           : "Ficha Fogo deu 2: +10, e o %d vira cinza", f->valor[a]);
        } else e = evento(gera, EV_FOGO, j, k, j, a, 0, capa ? "Ficha Fogo deu 1: o %d está protegido"
                                                             : "Ficha Fogo deu 1: o %d vira cinza", f->valor[a]);
        if (e && capa) e->bloqueado = (uint8_t)(1 << a);
    }
}

// O fim de j, 4: o Sacrificio e os multiplicadores, num evento so.
// Multiplicadores: Dobro (no dado valido logo acima), Fartura (nos de
// valor 1 e 2).
// Sacrificio valendo: x2 no dado seu de menor numero que pontua (sem
// contar os que nao pontuam); dois Sacrificios no mesmo dado, x4.
static void fim_mult(desfecho_t *d, int j, bool gera, bool fartura, int n_far)
{
    const fila_t *f = &F[j];
    const uint8_t *est = d->est[j];

    int sac[N_FILA] = { 0 };
    for (int s = 0; s < f->lancados && s < N_FILA; s++) {
        if (est[s] != V_VALIDO || TIPO[tipo_de(j, s)].efeito != EF_SACRIFICIO) continue;
        int a = -1;
        for (int i = 0; i < f->lancados && i < N_FILA; i++)
            if (est[i] == V_VALIDO && valor_pontos(j, i) > 0 && (a < 0 || f->valor[i] < f->valor[a])) a = i;
        if (a >= 0) sac[a] += 2;
    }
    uint8_t mask = 0;
    uint8_t de[MULT_N] = { 0 };                  // de onde veio cada multiplicador (para o texto)
    int8_t mult0[N_FILA];                        // os de antes (da primeira passada da Saideira)
    memcpy(mult0, d->mult[j], N_FILA);
    int sozinhos = 0;                            // dados valendo: a Viuva quer ser a unica
    for (int k = 0; k < f->lancados; k++) sozinhos += est[k] == V_VALIDO;
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (est[k] != V_VALIDO || universo(j, k)) continue;   // nada multiplica o Universo
        int m = 0;
        uint8_t bit = (uint8_t)(1 << k);
        // Dobro: vale so para o dado logo depois dele, e so se os dois
        // terminarem valendo. Anulado o seguinte, o x2 se perde; nao passa
        // adiante para o proximo que vale.
        // E so se o Dobro tirou de 1 a 3.
        if (k > 0 && est[k - 1] == V_VALIDO && TIPO[tipo_de(j, k - 1)].efeito == EF_DOBRO
            && f->valor[k - 1] <= 3) { m += 2; de[MULT_DOBRO] |= bit; }
        if (fartura && f->valor[k] <= 2) { m += 2 * n_far; de[MULT_FARTURA] |= bit; }
        if (TIPO[tipo_de(j, k)].efeito == EF_VIUVA && sozinhos == 1) { m += 2; de[MULT_VIUVA] |= bit; }
        if (TIPO[tipo_de(j, k)].efeito == EF_AZARAO && fichas_ini[j] < fichas_ini[outro(j)]) {
            m += 2;                              // comecou a rodada atras em fichas
            de[MULT_AZARAO] |= bit;
        }
        if (TIPO[tipo_de(j, k)].efeito == EF_ESPERANCA && fichas_ini[j] <= ESPERANCA_FICHAS) {
            m += 2;                              // comecou a rodada com ESPERANCA_FICHAS fichas ou menos
            de[MULT_ESPERANCA] |= bit;
        }
        if (sac[k]) { m += sac[k]; de[MULT_SACRIFICIO] |= bit; }
        if (j == 1 && virada_forca(2) && (k == 0 || (virada_forca(2) == 2 && k == 1))) {
            m += 2;                              // o Barao virado: x2 no primeiro (e no segundo, na Cobertura)
            de[MULT_BARAO] |= bit;
        }
        // Amuletos: Piramide (x2 nos de 4 lados), Abre-Alas (x2 no primeiro da sequencia).
        if (tem_am(j, A_PIRAMIDE) && TIPO[tipo_de(j, k)].lados == 4) { m += 2; de[MULT_PIRAMIDE] |= bit; }
        // Tres Patinhos: na 3a (6a, 9a...) vez que sao lancados, x3 (se valem).
        // ACHADO A07: preservado - copiado pelo Camaleao, le o fixo do Camaleao (0), nao o do outro dado (nao testado)
        if (TIPO[tipo_de(j, k)].efeito == EF_PATINHOS && patinhos_ativo(f->fixo[k])) { m += 3; de[MULT_PATINHOS] |= bit; }
        // Pisca-pisca aceso: x2 nele mesmo.
        // ACHADO A07: preservado - o Camaleao atras do Pisca-pisca le o proprio fixo (0 = aceso): x2 sempre
        if (TIPO[tipo_de(j, k)].efeito == EF_PISCA && pisca_aceso(f->fixo[k])) { m += 2; de[MULT_PISCA] |= bit; }
        if (k == 0 && tem_am(j, A_ABRE_ALAS)) { m += 2; de[MULT_ABRE_ALAS] |= bit; }
        // Somam aos que ja havia (a Saideira passa uma segunda vez: x2 de novo e x4).
        if (m) { d->mult[j][k] = (int8_t)(d->mult[j][k] + m); mask |= 1 << k; }
    }
    if (mask && gera) {
        int antes = pontos_fila(j, d->est[j], d->bonus[j], mult0);
        int depois = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]);
        char quem[64];
        texto_mult(quem, sizeof quem, j, de, n_far, false);
        if (strlen(quem) > 46) texto_mult(quem, sizeof quem, j, de, n_far, true);   // so os nomes
        evento_t *e = evento(gera, EV_MULT, j, 0xFF, 0, 0, 0, "%s: %d vira %d", quem, antes, depois);
        if (e) {
            e->mascara = mask;
            memcpy(e->por_dado, d->mult[j], N_FILA);
            if (de[MULT_PIRAMIDE]) e->am |= AM_BIT(A_PIRAMIDE);
            if (de[MULT_ABRE_ALAS]) e->am |= AM_BIT(A_ABRE_ALAS);
        }
    }
}

// O fim de j, 5: os pontos fixos, fora dos multiplicadores.
// Semente: se os dados deste jogador (com bonus, multiplicadores e os que
// roubou) somam SEMENTE_BROTA ou mais, +SEMENTE_PTS no fim - fora de qualquer multiplicador.
static void fim_fixos(desfecho_t *d, int j, bool gera)
{
    const fila_t *f = &F[j];
    const uint8_t *est = d->est[j];

    int dados = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j];
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_SEMENTE) continue;
        if (dados < SEMENTE_BROTA) continue;
        d->semente[j] += SEMENTE_PTS;
        evento(gera, EV_SEMENTE, j, k, j, 0, SEMENTE_PTS, "Semente: %d pontos, brota +%d", dados, SEMENTE_PTS);
    }

    // Crisantemo (o Velorio do Coveiro): +1 por dado de j anulado na rodada.
    // Do oponente, so vem do trunfo do Coveiro; seu, do amuleto.
    if (tem_am(j, A_CRISANTEMO)) {
        int mortos = 0;
        for (int k = 0; k < f->lancados; k++) if (d->est[j][k] == V_ANULADO) mortos++;
        if (mortos) {
            d->semente[j] += mortos;
            evento_t *e = NULL;
            if (j == 0) e = evento(gera, EV_CHEFE, 0, 0xFF, 0, 0, mortos, "Crisântemo: %d dado%s seu%s anulado%s, +%d",
                                   mortos, mortos > 1 ? "s" : "", mortos > 1 ? "s" : "", mortos > 1 ? "s" : "", mortos);
            if (e) e->am = AM_BIT(A_CRISANTEMO);
            else evento(gera, EV_CHEFE, 1, 0xFF, 1, 0, mortos, "Velório: %d dado%s dele anulado%s, +%d",
                        mortos, mortos > 1 ? "s" : "", mortos > 1 ? "s" : "", mortos);
        }
    }

    // A virada do Barao: +VIRADA_BARAO (3) para ele por dado seu anulado na rodada.
    if (j == 1 && virada_forca(2)) {
        int caidos = 0;
        for (int k = 0; k < F[0].lancados; k++) if (d->est[0][k] == V_ANULADO) caidos++;
        if (caidos) {
            d->semente[1] += VIRADA_BARAO * caidos;
            evento(gera, EV_CHEFE, 1, 0xFF, 1, 0, VIRADA_BARAO * caidos, "%s: %d dado%s seu%s anulado%s, +%d", J[1].nome,
                   caidos, caidos > 1 ? "s" : "", caidos > 1 ? "s" : "", caidos > 1 ? "s" : "", VIRADA_BARAO * caidos);
        }
    }
    // Checkpoint valendo: cada dado seu lancado depois dele e valendo ganha +3.
    for (int c = 0; c < f->lancados; c++) {
        if (tipo_de(j, c) != D_CHECKPOINT || est[c] != V_VALIDO) continue;
        int n = 0;
        for (int i = c + 1; i < f->lancados; i++) n += est[i] == V_VALIDO;
        if (!n) continue;
        d->semente[j] += 3 * n;
        evento_t *e = evento(gera, EV_EXTRA, j, c, j, 0, 3 * n, "Checkpoint: %d dado%s depois dele, +%d",
                             n, n > 1 ? "s" : "", 3 * n);
        for (int i = c + 1; e && i < f->lancados && i < N_FILA; i++)
            if (est[i] == V_VALIDO) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = 3; }
    }
    // Daqui ate o fim, os bonus fixos, fora dos multiplicadores (no mesmo
    // lugar da Semente): Jingle Bells, Estrela de Belem, Meia na Lareira, a
    // Onda que vira, Ourico, Ficha Velha, Revolver e Martir; depois, de cada
    // dado valendo, Misericordioso, Solitario, Prisma, Capitao, Trono,
    // Seguranca, Domino, Puxa-fila, Coveiro, Hexa, Ampulheta, Assombracao,
    // Leque, Balao, Mosaico, Medusa, Rancor, Media, Pote Quente, Garimpeiro,
    // Agiota e Desafiante; por ultimo, o Jackpot.
    int anulados = 0, valendo = 0;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] == V_ANULADO) anulados++;
        if (est[k] == V_VALIDO) valendo++;
    }
    // Jingle Bells: com dois ou mais valendo, os sinos tocam - +4 em cada um,
    // fora dos multiplicadores.
    int sinos = 0, sino0 = -1;
    uint8_t m_sino = 0;
    for (int k = 0; k < f->lancados && k < N_FILA; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_JINGLE) {
            if (sino0 < 0) sino0 = k;
            sinos++;
            m_sino |= (uint8_t)(1 << k);
        }
    if (sinos >= 2) {
        d->semente[j] += JINGLE_PTS * sinos;
        evento_t *e = evento(gera, EV_EXTRA, j, sino0, j, 0, JINGLE_PTS * sinos, "Jingle Bells: %d sinos, +%d em cada",
                             sinos, JINGLE_PTS);
        for (int i = 0; e && i < N_FILA; i++)
            if (m_sino >> i & 1) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = JINGLE_PTS; }
    }
    // Estrela de Belem valendo e lancada por ultimo: +2 por dado seu valendo
    // abaixo dela, fora dos multiplicadores.
    int ult = f->lancados - 1;
    if (ult >= 0 && ult < N_FILA && est[ult] == V_VALIDO && TIPO[tipo_de(j, ult)].efeito == EF_ESTRELA) {
        int n = 0;
        for (int i = 0; i < ult; i++) n += est[i] == V_VALIDO;
        if (n) {
            d->semente[j] += ESTRELA_PTS * n;
            evento_t *e = evento(gera, EV_EXTRA, j, ult, j, 0, ESTRELA_PTS * n, "Estrela de Belém no topo: %d dado%s, +%d",
                                 n, n > 1 ? "s" : "", ESTRELA_PTS * n);
            for (int i = 0; e && i < ult; i++)
                if (est[i] == V_VALIDO) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = ESTRELA_PTS; }
        }
    }
    // Meia na Lareira: valendo, paga o que guardou (fora dos multiplicadores);
    // anulada, avisa que guarda mais MEIA_GUARDA (4) (uma vez so, mesmo com a Saideira).
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (TIPO[tipo_de(j, k)].efeito != EF_MEIA) continue;
        // ACHADO A07: preservado - copiado pelo Camaleao, le o fixo do Camaleao (0), nao o do outro dado (nao testado)
        if (est[k] == V_VALIDO && f->fixo[k]) {
            d->semente[j] += f->fixo[k];
            evento(gera, EV_EXTRA, j, k, j, 0, f->fixo[k], "Meia na Lareira paga +%d", f->fixo[k]);
        } else if (est[k] == V_ANULADO && !(d->meia[j] >> k & 1)) {
            d->meia[j] |= (uint8_t)(1 << k);
            int g = meia_depois(f->fixo[k]);
            evento(gera, EV_EXTRA, j, k, j, 0, 0, "Meia na Lareira anulada: guarda +%d (%d na meia)", g - f->fixo[k], g);
        }
    }
    // A Onda valendo que, com o que tirou, junta 9: vira Tsunami no fim (so a
    // que veio da bolsa; uma vez so, mesmo com a Saideira).
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_ONDA || (d->onda[j] >> k & 1)) continue;
        if (!onda_da_bolsa(j, k) || onda_depois(f->fixo[k], f->valor[k]) < ONDA_VIRA) continue;
        d->onda[j] |= (uint8_t)(1 << k);
        evento(gera, EV_ONDA, j, k, j, k, 0, "Onda junta %d: vira Tsunami!", ONDA_VIRA);
    }
    // O Ourico na fila (valido ou anulado; roubado, nao): cada tentativa do
    // oponente de anular, atacar ou tirar pontos seus na rodada custa a ele
    // OURICO_PTS - ate a que a bolha do Impermeavel segurou.
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (est[k] == V_ROUBADO || TIPO[tipo_de(j, k)].efeito != EF_OURICO) continue;
        int r = outro(j), n = d->atacados[j];
        if (!n) continue;
        // ACHADO A13: preservado - na Saideira do dono, cobra de novo o total: os ataques ja cobrados custam outra vez
        // ACHADO A01: preservado - se o dono do Ourico abriu a rodada, cobra antes de a Assombracao do rival contar
        d->delta[r] -= OURICO_PTS * n;
        evento(gera, EV_TIRA, j, k, r, 0, OURICO_PTS * n, "Ouriço: %d ataque%s, -%d para %s", n,
               n > 1 ? "s" : "", OURICO_PTS * n, J[r].nome);
    }
    for (int k = 0; k < f->lancados; k++) {               // Ficha Velha anulada: +4, caida ou atacada
        if (est[k] != V_ANULADO || TIPO[tipo_de(j, k)].efeito != EF_FICHA_VELHA) continue;
        d->semente[j] += FICHA_VELHA_PTS;
        evento(gera, EV_EXTRA, j, k, j, 0, FICHA_VELHA_PTS, "Ficha Velha anulada: +%d", FICHA_VELHA_PTS);
    }
    for (int k = 0; k < f->lancados && k < N_FILA; k++) { // Revolver anulado na quarta: o tiro ja saiu, +REVOLVER_TIRO (12) por fora
        // ACHADO A07: preservado - copiado pelo Camaleao, le o fixo do Camaleao (0), nao o do outro dado (nao testado)
        if (est[k] != V_ANULADO || TIPO[tipo_de(j, k)].efeito != EF_REVOLVER || !revolver_dispara(f->fixo[k])) continue;
        d->semente[j] += REVOLVER_TIRO;
        evento(gera, EV_EXTRA, j, k, j, 0, REVOLVER_TIRO, "Revólver anulado: o tiro já saiu, +%d", REVOLVER_TIRO);
    }
    for (int k = 0; k < f->lancados && valendo; k++) {    // Martir caido: +2 em cada um que vale
        if (est[k] != V_ANULADO || TIPO[tipo_de(j, k)].efeito != EF_MARTIR) continue;
        d->semente[j] += 2 * valendo;
        evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, 2 * valendo, "Mártir anulado: +2 em %d dado%s",
                             valendo, valendo > 1 ? "s" : "");
        for (int i = 0; e && i < f->lancados && i < N_FILA; i++)
            if (est[i] == V_VALIDO) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = 2; }
    }
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO) continue;
        int ef = TIPO[tipo_de(j, k)].efeito;
        if (ef == EF_MISERICORDIA && anulados) {
            d->semente[j] += 2 * anulados;
            evento(gera, EV_EXTRA, j, k, j, 0, 2 * anulados, "Misericordioso: %d anulado%s, +%d",
                   anulados, anulados > 1 ? "s" : "", 2 * anulados);
        } else if (ef == EF_SOLITARIO && !(k > 0 && est[k - 1] == V_VALIDO)
                   && !(k + 1 < f->lancados && est[k + 1] == V_VALIDO)) {
            d->semente[j] += SOLITARIO_PTS;
            evento(gera, EV_EXTRA, j, k, j, 0, SOLITARIO_PTS, "Solitário sem vizinhos: +%d", SOLITARIO_PTS);
        } else if (ef == EF_PRISMA) {                // +1 por tamanho de dado diferente valendo: d2, d4, d6...
            int lados[N_FILA], nt = 0;
            for (int i = 0; i < f->lancados && i < N_FILA; i++) {
                if (est[i] != V_VALIDO) continue;
                int l = TIPO[tipo_de(j, i)].lados, ja = 0;
                for (int q = 0; q < nt; q++) ja |= lados[q] == l;
                if (!ja) lados[nt++] = l;
            }
            d->semente[j] += nt;
            evento(gera, EV_EXTRA, j, k, j, 0, nt, "Prisma: %d tipo%s de dado, +%d", nt, nt > 1 ? "s" : "", nt);
        } else if (ef == EF_CAPITAO) {               // +1 por dado seu valendo, ele incluido
            d->semente[j] += valendo;
            evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, valendo, "Capitão: %d dado%s, +%d",
                                 valendo, valendo > 1 ? "s" : "", valendo);
            for (int i = 0; e && i < f->lancados && i < N_FILA; i++)
                if (est[i] == V_VALIDO) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = 1; }
        } else if (ef == EF_TRONO && k == FILA_NORMAL - 1 && valendo == f->lancados) {
            d->semente[j] += TRONO_BONUS;            // o 4o da fila, e nenhum dos seus caiu
            evento(gera, EV_EXTRA, j, k, j, 0, TRONO_BONUS, "Trono: a sequência inteira válida, +%d", TRONO_BONUS);
        } else if (ef == EF_SEGURANCA && valendo == f->lancados) {
            d->semente[j] += 2;                      // nenhum dos seus caiu
            evento(gera, EV_EXTRA, j, k, j, 0, 2, "Segurança: a sequência inteira válida, +2");
        } else if (ef == EF_DOMINO && k + 1 < f->lancados && f->valor[k + 1] >= f->valor[k] && f->valor[k + 1] <= f->valor[k] + 2) {
            // ACHADO A19: preservado - encaixa mesmo com o dado seguinte anulado ou roubado (nao olha est[k + 1])
            int n = 3 - (f->valor[k + 1] - f->valor[k]);   // o mesmo: +3; um a mais: +2; dois a mais: +1
            d->semente[j] += n;
            evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, n, "Dominó: a peça %d encaixa, +%d", f->valor[k + 1], n);
            if (e) { e->mascara = (uint8_t)(3 << k); e->por_dado[k] = (int8_t)n; }   // a peca cai no seguinte
        } else if (ef == EF_PUXA && k == 0 && f->lancados >= FILA_NORMAL && valendo == f->lancados) {
            d->semente[j] += 5;
            evento(gera, EV_EXTRA, j, k, j, 0, 5, "Puxa-fila: a sequência inteira válida, +5");
        } else if (ef == EF_COVEIRO) {
            int n = 0;
            for (int q = 0; q < 2; q++)
                for (int i = 0; i < F[q].lancados; i++) n += d->est[q][i] == V_ANULADO;
            if (n) {
                d->semente[j] += n;
                evento(gera, EV_EXTRA, j, k, j, 0, n, "Coveiro: %d anulado%s na mesa, +%d", n, n > 1 ? "s" : "", n);
            }
        } else if (ef == EF_HEXA) {                  // +1 por dado de seis lados seu valendo
            int n = 0;
            for (int i = 0; i < f->lancados; i++) n += est[i] == V_VALIDO && TIPO[tipo_de(j, i)].lados == 6;
            d->semente[j] += n;
            evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, n, "Hexa: %d dado%s de seis lados, +%d", n, n > 1 ? "s" : "", n);
            for (int i = 0; e && i < f->lancados && i < N_FILA; i++)
                if (est[i] == V_VALIDO && TIPO[tipo_de(j, i)].lados == 6) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = 1; }
        } else if (ef == EF_AMPULHETA && ampulheta() > 0) {
            int n = ampulheta();
            d->semente[j] += n;
            evento(gera, EV_EXTRA, j, k, j, 0, n, "Ampulheta: %d rodadas jogadas, +%d", rodada - 1, n);
        } else if (ef == EF_ASSOMBRA) {                   // valendo, o susto: -2 no oponente (os Fantasmas vao no fim da rodada)
            int r = outro(j);
            d->delta[r] -= ASSOMBRA_PTS;
            // ACHADO A01: preservado - o Ourico do rival so cobra este susto se o dono da Assombracao abriu a rodada
            d->atacados[r]++;
            evento(gera, EV_TIRA, j, k, r, 0, ASSOMBRA_PTS, "Assombração assusta %s: -%d", J[r].nome, ASSOMBRA_PTS);
        } else if (ef == EF_LEQUE) {                      // +2 por outro dado seu valendo que caiu num lado com adesivo
            int n = 0;
            uint8_t m = 0;
            for (int i = 0; i < f->lancados && i < N_FILA; i++)
                if (i != k && est[i] == V_VALIDO && ad_fila(j, i)) { m |= (uint8_t)(1 << i); n++; }
            if (n) {
                d->semente[j] += LEQUE_PTS * n;
                evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, LEQUE_PTS * n, "Leque: %d dado%s no adesivo, +%d",
                                     n, n > 1 ? "s" : "", LEQUE_PTS * n);
                for (int i = 0; e && i < N_FILA; i++) if (m >> i & 1) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = LEQUE_PTS; }
            }
        } else if (ef == EF_BALAO && f->valor[k] >= BALAO_ESTOURA && f->fixo[k] > 0 && F[j].tipo[k] == D_BALAO) {
            int n = f->fixo[k];                           // com 7 ou 8, estoura: o ar guardado entra de uma vez
            d->semente[j] += n;
            d->balao[j] |= (uint8_t)(1 << k);
            evento(gera, EV_EXTRA, j, k, j, 0, n, "Balão estoura: +%d", n);
        } else if (ef == EF_MOSAICO) {                    // +1 por adesivo nos seus dados lancados, ele incluido
            int n = 0;
            for (int i = 0; i < f->lancados && i < N_FILA; i++) {
                const peca_t *pm = est[i] == V_ROUBADO ? NULL : peca_fila(j, i);
                if (pm) n += ad_quantos(pm);
            }
            if (n) {
                d->semente[j] += n;
                evento(gera, EV_EXTRA, j, k, j, 0, n, "Mosaico: %d adesivo%s nos lançados, +%d", n, n > 1 ? "s" : "", n);
            }
        } else if (ef == EF_MEDUSA && !d->medusa[j]) {      // a fila inteira vira pedra (so a primeira age)
            d->medusa[j] = 1;
            evento_t *e = evento(gera, EV_MEDUSA, j, k, j, 0, 0, "Medusa: a sua sequência vira pedra");
            for (int i = 0; e && i < f->lancados && i < N_FILA; i++) {
                if (est[i] == V_ROUBADO) continue;
                if (impermeavel(j, i)) e->bloqueado |= (uint8_t)(1 << i);   // com a capa, nao vira pedra
                else e->mascara |= (uint8_t)(1 << i);
            }
        } else if (ef == EF_RANCOR && J[j].perdidas > 0) {   // +1 por rodada perdida na partida
            int n = J[j].perdidas;
            d->semente[j] += n;
            evento(gera, EV_EXTRA, j, k, j, 0, n, "Rancor: %d rodada%s perdida%s, +%d", n, n > 1 ? "s" : "", n > 1 ? "s" : "", n);
        } else if (ef == EF_MEDIA) {                        // o total da fila dividido por 4
            int tot = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]), n = tot > 0 ? tot / 4 : 0;
            if (n) {
                d->semente[j] += n;
                evento(gera, EV_EXTRA, j, k, j, 0, n, "Média: %d ÷ 4, +%d", tot, n);
            }
        } else if (ef == EF_POTE && pote >= 25) {           // +1 a cada 25 fichas no pote
            int n = pote / 25;
            d->semente[j] += n;
            evento(gera, EV_EXTRA, j, k, j, 0, n, "Pote Quente: %d fichas no pote, +%d", pote, n);
        } else if (ef == EF_GARIMPEIRO && !d->garimpo[j]) { // os outros validos voltam para a bolsa (so o primeiro age)
            uint8_t m = 0;
            int n = 0, maior = 0;
            for (int i = 0; i < f->lancados && i < N_FILA; i++)
                if (est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito != EF_GARIMPEIRO) {
                    m |= (uint8_t)(1 << i); n++;
                    if (F[j].valor[i] > maior) maior = F[j].valor[i];
                }
            if (m) {
                d->garimpo[j] = m;
                evento_t *e = evento(gera, EV_GARIMPO, j, k, j, 0, 0, "Garimpeiro: %d dado%s volta%s para a bolsa",
                                     n, n > 1 ? "s" : "", n > 1 ? "m" : "");
                if (e) e->mascara = m;
                int pepita = (maior + 1) / 2;              // a bateia: o maior dos que voltam fica, pela metade
                if (pepita > 0) {
                    d->semente[j] += pepita;
                    evento(gera, EV_EXTRA, j, k, j, 0, pepita, "Garimpeiro: a pepita do %d, +%d", maior, pepita);
                }
            }
        } else if (ef == EF_AGIOTA) {
            d->agiota[j] += AGIOTA_FICHAS;
            evento(gera, EV_AGIOTA, j, k, outro(j), 0, AGIOTA_FICHAS, "Agiota: %d fichas de %s", AGIOTA_FICHAS, J[outro(j)].nome);
        } else if (ef == EF_DESAFIANTE) {
            bool maior = true;
            for (int q = 0; q < 2 && maior; q++)
                for (int i = 0; i < F[q].lancados; i++)
                    if (!(q == j && i == k) && d->est[q][i] == V_VALIDO && F[q].valor[i] >= f->valor[k]) {
                        maior = false;
                        break;
                    }
            if (maior) {
                d->semente[j] += 4;
                evento(gera, EV_EXTRA, j, k, j, 0, 4, "Desafiante: o %d é o maior, +4", f->valor[k]);
            }
        }
    }

    // Jackpot: tres dados valendo com o numero dele (ele incluido), +10; so
    // dois, +4. Iguais entre si, mas nao a ele, nao contam. Fora dos
    // multiplicadores (soma no mesmo bonus fixo da Semente).
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_JACKPOT) continue;
        uint8_t iguais = 0;
        int num = f->valor[k], n = jackpot_com(j, est, num, &iguais);
        if (n >= 3) {
            d->semente[j] += 10;
            evento_t *e = evento(gera, EV_JACKPOT, j, k, j, 0, 10, "JACKPOT! três %d: +10", num);
            if (e) e->mascara = iguais;
        } else if (n == 2) {
            d->semente[j] += 4;
            evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, 4, "Jackpot: dois %d, +4", num);
            for (int i = 0; e && i < N_FILA; i++)
                if (iguais >> i & 1) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = 2; }
        }
    }
}

// Efeitos do fim sobre a propria fila, em cinco partes, nesta ordem:
// fim_bonus (os bonus por dado), fim_mult_prepara (conta as Farturas),
// fim_fogo (a Ficha de Fogo), fim_mult (o Sacrificio e os multiplicadores)
// e fim_fixos (os pontos fixos). Decididos sobre o estado depois dos ataques.
static void efeitos_do_fim(desfecho_t *d, int j, bool gera)
{
    fim_bonus(d, j, gera);
    bool fartura;
    int n_far;
    fim_mult_prepara(d, j, &fartura, &n_far);
    fim_fogo(d, j, gera);
    fim_mult(d, j, gera, fartura, n_far);
    fim_fixos(d, j, gera);
}

