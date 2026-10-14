// ===========================================================================
// Desfecho: poderes que mexem no rival, em sequencia
// ===========================================================================
typedef struct {
    uint8_t est[2][N_FILA];
    int8_t  bonus[2][N_FILA];        // Moeda da sorte
    int8_t  mult[2][N_FILA];         // soma dos multiplicadores (0 = nenhum)
    uint8_t queima[2][N_FILA];       // alvos da Ficha de Fogo
    int semente[2];
    int roubo_val[2];
    int delta[2];
    int total[2];
    int bruto[2];                    // o total antes de travar em 0 (a previa mostra negativo)
    int atacados[2];                 // dados de cada um que poderes do outro anularam ou roubaram (Ourico)
    int agiota[2];                   // fichas que o Agiota de cada um leva do outro, fora do pote
    uint8_t medusa[2];               // a Medusa valeu: a fila vira Quebrados no fim da rodada
    uint8_t come[2], come_de[2];     // a Autofagia comeu: o dado (posicao + 1) e ela (posicao + 1); 0: nada
    uint8_t garimpo[2];              // o Garimpeiro valeu: posicoes dos outros validos que voltam para a bolsa
} desfecho_t;

static int contribui(int j, int k)
{
    return (valor_pontos(j, k) + F[j].bonus[k]) * (F[j].dobra[k] ? 2 : 1);
}

// Alvo de um ataque na fila de j. maior = true: o maior, para ANULAR (Carrasco,
// Invejoso) - os nao anulaveis ficam de fora. maior = false: o menor, para
// ROUBAR (Caridoso, Gatuno) - so o intocavel fica de fora.
static int alvo_de(const uint8_t *est, int j, bool maior, int exceto)
{
    int a = -1;
    if (!maior && tem_am(j, A_IMA)) return -1;      // Ima: ninguem rouba
    for (int i = 0; i < F[j].lancados; i++) {
        if (i == exceto || est[i] != V_VALIDO) continue;
        if (maior ? firme(j, i) : intocavel(j, i)) continue;
        if (a < 0 || (maior ? F[j].valor[i] > F[j].valor[a] : F[j].valor[i] < F[j].valor[a]))
            a = i;
    }
    return a;
}

// Seguranca: um ataque ao dado a da fila de j cai no Seguranca dele, se houver
// um valendo (e nao for o proprio alvo).
static int desvia_seguranca(const uint8_t *est, int j, int a)
{
    if (a < 0) return a;
    for (int i = 0; i < F[j].lancados; i++)
        if (i != a && est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_SEGURANCA) return i;
    return a;
}

// Um evento novo no fim da lista (so quando gera). Devolve o evento, para
// completar a mascara e os numeros de cada dado, ou NULL.
static evento_t *evento(bool gera, int tipo, int dj, int dk, int aj, int ak, int valor,
                        const char *fmt, ...) __attribute__((format(printf, 8, 9)));
static evento_t *evento(bool gera, int tipo, int dj, int dk, int aj, int ak, int valor,
                        const char *fmt, ...)
{
    if (!gera || n_ev >= MAX_EV) return NULL;
    evento_t *e = &ev[n_ev++];
    memset(e, 0, sizeof *e);
    e->tipo = (uint8_t)tipo; e->de_j = (uint8_t)dj; e->de_k = (uint8_t)dk;
    e->alvo_j = (uint8_t)aj; e->alvo_k = (uint8_t)ak; e->valor = valor;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(e->txt, sizeof e->txt, fmt, ap);
    va_end(ap);
    return e;
}

// Cavalo de Troia: o dado a de r acabou de ser anulado ou roubado por um
// poder de j. Se for o Cavalo, ele se abre: os dados de j que valem caem
// (Escudo e Teimoso resistem; a Couraca de j segura tudo), e o que j roubou
// nesta rodada, o proprio Cavalo inclusive, deixa de contar para ele.
static void cavalo_abre(desfecho_t *d, int j, int r, int a, bool gera)
{
    if (TIPO[tipo_de(r, a)].efeito != EF_CAVALO) return;
    if (tem_am(j, A_COURACA)) {
        if (gera) am_acende(A_COURACA);
        return;
    }
    uint8_t mask = 0;
    int8_t perde[N_FILA] = { 0 };
    for (int i = 0; i < F[j].lancados; i++)
        if (d->est[j][i] == V_VALIDO && !firme(j, i)) {
            perde[i] = (int8_t)contribui(j, i);
            d->est[j][i] = V_ANULADO;
            mask |= (uint8_t)(1 << i);
            d->atacados[j]++;
        }
    int roubado = d->roubo_val[j];
    d->roubo_val[j] = 0;
    if (!mask && !roubado) return;
    evento_t *e = evento(gera, EV_CAVALO, r, a, j, 0, roubado, "Cavalo de Troia: anula os de %s", J[j].nome);
    if (e) { e->mascara = mask; memcpy(e->por_dado, perde, sizeof perde); }
}

// A Autofagia (posicao a da fila de j) pode comer o dado b? So com os dois
// vindos da bolsa, com ela abaixo de +3 e com pelo menos 8 dados em jogo.
static bool autofagia_come_pode(int j, int a, int b)
{
    const jogador_t *p = &J[j];
    int ia = F[j].idx[a], ib = F[j].idx[b];
    if (ia < 0 || ia >= p->n_col || ib < 0 || ib >= p->n_col || ia == ib) return false;
    if (p->col[ia].tipo != D_AUTOFAGIA || p->col[ia].fixo >= AUTO_MAX) return false;
    int vivos = 0;
    for (int i = 0; i < p->n_col; i++) vivos += p->onde[i] != COMIDO;
    return vivos >= AUTO_BOLSA;
}

// Pontos de uma fila dados o estado, os bonus e os multiplicadores do fim.
// Multiplicadores se SOMAM (x2, x3, x2 = x7) e valem sobre valor + bonus.
static int pontos_fila(int j, const uint8_t *est, const int8_t *bonus, const int8_t *mult)
{
    int t = 0;
    for (int k = 0; k < F[j].lancados; k++) {
        if (est[k] != V_VALIDO) continue;
        int base = valor_pontos(j, k) + F[j].bonus[k] + (bonus ? bonus[k] : 0);
        int m = (mult && mult[k] > 0) ? mult[k] : 1;
        t += base * m;
    }
    return t;
}

// De onde vem cada multiplicador, para o texto do evento: "Dobro x2 no 5 ·
// Fartura x2 nos 1 e 2: 13 vira 25". curto: so os nomes ("Dobro, Fartura").
enum { MULT_DOBRO, MULT_FARTURA, MULT_VIUVA, MULT_AZARAO, MULT_ESPERANCA, MULT_SACRIFICIO, MULT_BARAO, MULT_N };
static void texto_mult(char *s, size_t n, int j, const uint8_t *de, int n_far, bool curto)
{
    const char *nome[MULT_N] = { "Dobro", "Fartura", "Viúva Negra", "Azarão", "Esperança", "Sacrifício", J[1].nome };
    int x[MULT_N] = { 2, 2 * n_far, 2, 2, 2, 2, 2 };
    size_t p = 0;
    s[0] = 0;
    for (int q = 0; q < MULT_N && p < n; q++) {
        if (!de[q]) continue;
        if (p) p += (size_t)snprintf(s + p, n - p, curto ? ", " : " · ");
        if (p >= n) break;
        p += (size_t)snprintf(s + p, n - p, "%s", nome[q]);
        if (curto || p >= n) continue;
        p += (size_t)snprintf(s + p, n - p, " x%d", x[q]);
        if (q == MULT_VIUVA || q == MULT_AZARAO || q == MULT_ESPERANCA || p >= n) continue;   // ele mesmo
        int vals[N_FILA], nv = 0;
        for (int k = 0; k < N_FILA; k++) if (de[q] >> k & 1) vals[nv++] = F[j].valor[k];
        p += (size_t)snprintf(s + p, n - p, nv > 1 ? " nos" : " no");
        for (int i = 0; i < nv && p < n; i++)
            p += (size_t)snprintf(s + p, n - p, i == 0 ? " %d" : (i == nv - 1 ? " e %d" : ", %d"), vals[i]);
    }
}

// A Ampulheta: +1 a cada 3 rodadas ja jogadas, ate +4.
static int ampulheta(void) { int n = (rodada - 1) / 3; return n < 0 ? 0 : n > 4 ? 4 : n; }

// Efeitos do fim sobre a propria fila: Moeda da sorte, depois os
// multiplicadores. Decididos sobre o estado depois dos ataques.
static void efeitos_do_fim(desfecho_t *d, int j, bool gera)
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
            if (face == 1 && impar)  { d->bonus[j][i] += 1; mask |= 1 << i; por[i] = 1; }
            if (face == 2 && !impar) { d->bonus[j][i] += 1; mask |= 1 << i; por[i] = 1; }
        }
        const char *txt = face == 1 ? "Moeda sorte deu 1: +1 nos ímpares" : "Moeda sorte deu 2: +1 nos pares";
        evento_t *e = mask ? evento(gera, EV_BONUS, j, k, 0, 0, 0, "%s", txt) : NULL;
        if (e) { e->mascara = mask; memcpy(e->por_dado, por, sizeof por); }
    }

    // Copiador: +2 em cada outro dado seu com o mesmo numero que ele.
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_COPIADOR) continue;
        uint8_t mask = 0;
        int8_t por[N_FILA] = { 0 };
        int nl = f->lancados < N_FILA ? f->lancados : N_FILA;
        for (int i = 0; i < nl; i++) {
            if (i == k || est[i] != V_VALIDO || f->valor[i] != f->valor[k]) continue;
            d->bonus[j][i] += 2; mask |= (uint8_t)(1 << i); por[i] = 2;
        }
        evento_t *e = mask ? evento(gera, EV_BONUS, j, k, 0, 0, 0, "Copiador %d: +2 em cada %d",
                                    f->valor[k], f->valor[k]) : NULL;
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

    // Ferreiro com 3 ou 4: +6 no dado logo depois dele, se os dois terminam valendo.
    for (int k = 0; k + 1 < f->lancados && k + 1 < N_FILA; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_FERREIRO || f->valor[k] < 3) continue;
        if (est[k + 1] != V_VALIDO) continue;
        d->bonus[j][k + 1] += 6;
        evento_t *e = evento(gera, EV_BONUS, j, k, 0, 0, 0, "Ferreiro forja +6 no %d", f->valor[k + 1]);
        if (e) { e->mascara = (uint8_t)(1 << (k + 1)); e->por_dado[k + 1] = 6; }
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

    // Multiplicadores: Dobro (no dado valido logo acima), Fartura (nos de
    // valor 1 e 2).
    bool fartura = false;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_FARTURA) fartura = true;
    int n_far = 0;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_FARTURA) n_far++;
    // Ficha de Fogo: com 2, +10; com 1, nada. Nos dois casos o maior dado seu
    // que pontua (sem contar ela) queima no fim. Duas fichas queimam dois
    // dados diferentes.
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_FOGO) continue;
        int a = -1;
        for (int i = 0; i < f->lancados; i++) {
            if (i == k || est[i] != V_VALIDO || d->queima[j][i]) continue;
            if (TIPO[tipo_de(j, i)].efeito == EF_FOGO) continue;
            if (a < 0 || f->valor[i] > f->valor[a]) a = i;
        }
        if (a < 0) continue;
        d->queima[j][a] = 1;
        if (f->cru[k] == 2) {
            d->semente[j] += 10;
            evento(gera, EV_FOGO, j, k, j, a, 10, "Ficha Fogo deu 2: +10, e o %d vira cinza", f->valor[a]);
        } else evento(gera, EV_FOGO, j, k, j, a, 0, "Ficha Fogo deu 1: o %d vira cinza", f->valor[a]);
    }

    // Sacrificio valendo: x2 no dado seu de menor numero que pontua (sem
    // contar os que nao pontuam); dois Sacrificios no mesmo dado, x4.
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
    int sozinhos = 0;                            // dados valendo: a Viuva quer ser a unica
    for (int k = 0; k < f->lancados; k++) sozinhos += est[k] == V_VALIDO;
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (est[k] != V_VALIDO) continue;
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
            m += 2;                              // comecou a rodada com 15 fichas ou menos
            de[MULT_ESPERANCA] |= bit;
        }
        if (sac[k]) { m += sac[k]; de[MULT_SACRIFICIO] |= bit; }
        if (j == 1 && virada_forca(2) && (k == 0 || (virada_forca(2) == 2 && k == 1))) {
            m += 2;                              // o Barao virado: x2 no primeiro (e no segundo, na Cobertura)
            de[MULT_BARAO] |= bit;
        }
        if (m) { d->mult[j][k] = (int8_t)m; mask |= 1 << k; }
    }
    if (mask && gera) {
        int antes = pontos_fila(j, d->est[j], d->bonus[j], NULL);
        int depois = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]);
        char quem[64];
        texto_mult(quem, sizeof quem, j, de, n_far, false);
        if (strlen(quem) > 46) texto_mult(quem, sizeof quem, j, de, n_far, true);   // so os nomes
        evento_t *e = evento(gera, EV_MULT, j, 0xFF, 0, 0, 0, "%s: %d vira %d", quem, antes, depois);
        if (e) { e->mascara = mask; memcpy(e->por_dado, d->mult[j], N_FILA); }
    }

    // Semente: se os dados deste jogador (com bonus, multiplicadores e os que
    // roubou) somam 14 ou mais, +7 no fim - fora de qualquer multiplicador.
    int dados = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j];
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_SEMENTE) continue;
        if (dados < 14) continue;
        d->semente[j] += 7;
        evento(gera, EV_SEMENTE, j, k, j, 0, 7, "Semente: %d pontos, brota +7", dados);
    }

    // Crisantemo (o Velorio do Coveiro): +1 por dado de j anulado na rodada.
    // Do oponente, so vem do trunfo do Coveiro; seu, do amuleto.
    if (tem_am(j, A_CRISANTEMO)) {
        int mortos = 0;
        for (int k = 0; k < f->lancados; k++) if (d->est[j][k] == V_ANULADO) mortos++;
        if (mortos) {
            d->semente[j] += mortos;
            if (j == 0) evento(gera, EV_CHEFE, 0, 0xFF, 0, 0, mortos, "Crisântemo: %d dado%s seu%s anulado%s, +%d",
                               mortos, mortos > 1 ? "s" : "", mortos > 1 ? "s" : "", mortos > 1 ? "s" : "", mortos);
            else evento(gera, EV_CHEFE, 1, 0xFF, 1, 0, mortos, "Velório: %d dado%s dele anulado%s, +%d",
                        mortos, mortos > 1 ? "s" : "", mortos > 1 ? "s" : "", mortos);
        }
    }

    // A virada do Barao: +2 para ele por dado seu anulado na rodada.
    if (j == 1 && virada_forca(2)) {
        int caidos = 0;
        for (int k = 0; k < F[0].lancados; k++) if (d->est[0][k] == V_ANULADO) caidos++;
        if (caidos) {
            d->semente[1] += VIRADA_BARAO * caidos;
            evento(gera, EV_CHEFE, 1, 0xFF, 1, 0, VIRADA_BARAO * caidos, "%s: %d dado%s seu%s caiu, +%d", J[1].nome,
                   caidos, caidos > 1 ? "s" : "", caidos > 1 ? "s" : "", VIRADA_BARAO * caidos);
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
    // Bonus fixos, fora dos multiplicadores (no mesmo lugar da Semente):
    // Misericordioso, Solitario, Prisma, Desafiante, Capitao, Trono e Seguranca.
    int anulados = 0, valendo = 0;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] == V_ANULADO) anulados++;
        if (est[k] == V_VALIDO) valendo++;
    }
    for (int k = 0; k < f->lancados && valendo; k++) {    // Martir caido: +2 em cada um que vale
        if (est[k] != V_ANULADO || TIPO[tipo_de(j, k)].efeito != EF_MARTIR) continue;
        d->semente[j] += 2 * valendo;
        evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, 2 * valendo, "Mártir caído: +2 em %d dado%s",
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
            d->semente[j] += 6;
            evento(gera, EV_EXTRA, j, k, j, 0, 6, "Solitário sem vizinhos: +6");
        } else if (ef == EF_PRISMA) {
            uint64_t tipos = 0;
            for (int i = 0; i < f->lancados; i++)
                if (est[i] == V_VALIDO) tipos |= 1ull << tipo_de(j, i);
            int nt = 0;
            for (; tipos; tipos >>= 1) nt += (int)(tipos & 1);
            d->semente[j] += nt;
            evento(gera, EV_EXTRA, j, k, j, 0, nt, "Prisma: %d tipo%s, +%d", nt, nt > 1 ? "s" : "", nt);
        } else if (ef == EF_CAPITAO) {               // +1 por dado seu valendo, ele incluido
            d->semente[j] += valendo;
            evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, valendo, "Capitão: %d dado%s, +%d",
                                 valendo, valendo > 1 ? "s" : "", valendo);
            for (int i = 0; e && i < f->lancados && i < N_FILA; i++)
                if (est[i] == V_VALIDO) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = 1; }
        } else if (ef == EF_TRONO && k == N_FILA - 1 && valendo == f->lancados) {
            d->semente[j] += 4;                      // o 4o da fila, e nenhum dos seus caiu
            evento(gera, EV_EXTRA, j, k, j, 0, 4, "Trono: a fila inteira valendo, +4");
        } else if (ef == EF_SEGURANCA) {
            d->semente[j] += 2;
            evento(gera, EV_EXTRA, j, k, j, 0, 2, "Segurança: +2");
        } else if (ef == EF_DOMINO && k + 1 < f->lancados && f->valor[k + 1] == f->valor[k]) {
            d->semente[j] += 3;
            evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, 3, "Dominó: o seguinte também deu %d, +3", f->valor[k]);
            if (e) { e->mascara = (uint8_t)(3 << k); e->por_dado[k] = 3; }   // a peca cai no seguinte
        } else if (ef == EF_PUXA && k == 0 && f->lancados == N_FILA && valendo == N_FILA) {
            d->semente[j] += 5;
            evento(gera, EV_EXTRA, j, k, j, 0, 5, "Puxa-fila: a fila inteira valendo, +5");
        } else if (ef == EF_COVEIRO && f->valor[k] >= 5) {
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
        } else if (ef == EF_OURICO && d->atacados[j]) {   // cada dado seu atacado custa 4 ao oponente
            int r = outro(j), n = d->atacados[j];
            d->delta[r] -= 4 * n;
            evento(gera, EV_TIRA, j, k, r, 0, 4 * n, "Ouriço: %d ataque%s, -%d para %s", n, n > 1 ? "s" : "", 4 * n, J[r].nome);
        } else if (ef == EF_MEDUSA && !d->medusa[j]) {      // a fila inteira vira pedra (so a primeira age)
            d->medusa[j] = 1;
            evento_t *e = evento(gera, EV_MEDUSA, j, k, j, 0, 0, "Medusa: a sua fila vira pedra");
            for (int i = 0; e && i < f->lancados && i < N_FILA; i++)
                if (est[i] != V_ROUBADO) e->mascara |= (uint8_t)(1 << i);
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
            int n = 0;
            for (int i = 0; i < f->lancados && i < N_FILA; i++)
                if (est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito != EF_GARIMPEIRO) { m |= (uint8_t)(1 << i); n++; }
            if (m) {
                d->garimpo[j] = m;
                evento_t *e = evento(gera, EV_GARIMPO, j, k, j, 0, 0, "Garimpeiro: %d dado%s volta%s para a bolsa",
                                     n, n > 1 ? "s" : "", n > 1 ? "m" : "");
                if (e) e->mascara = m;
            }
        } else if (ef == EF_AGIOTA) {
            d->agiota[j] += 2;
            evento(gera, EV_AGIOTA, j, k, outro(j), 0, 2, "Agiota: 2 fichas de %s", J[outro(j)].nome);
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

    // Jackpot: tres dados valendo com o mesmo numero, +10; so dois, +4. Fora
    // dos multiplicadores (soma no mesmo bonus fixo da Semente).
    uint8_t trio = 0, dupla = 0;
    int num = trinca(j, est, &trio), par = num ? 0 : mesmo_numero(j, est, 2, &dupla);
    for (int k = 0; (num || par) && k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_JACKPOT) continue;
        if (num) {
            d->semente[j] += 10;
            evento_t *e = evento(gera, EV_JACKPOT, j, k, j, 0, 10, "JACKPOT! três %d: +10", num);
            if (e) e->mascara = trio;
        } else {
            d->semente[j] += 4;
            evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, 4, "Jackpot: dois %d, +4", par);
            for (int i = 0; e && i < N_FILA; i++)
                if (dupla >> i & 1) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = 2; }
        }
    }
}

// O poder do dado k de j esta armado, pelo que ele tirou e pela mesa de
// agora? E o brilho dele enquanto a sequencia e lancada. 0: nao; 1: sim;
// 2: sim, mas contra quem o jogou (Carrasco ou Tudo ou Nada com 1, Berserker, Autofagia).
static int poder_armado(int j, int k)
{
    const fila_t *f = &F[j];
    if (k >= f->lancados) return 0;
    int ef = TIPO[tipo_de(j, k)].efeito, v = f->valor[k];
    if (ef == EF_CARRASCO && f->cru[k] == 1) return 2;          // valendo ou nao
    if (ef == EF_TUDO_NADA && f->cru[k] == 1) return 2;
    if (ef == EF_FENIX) return f->est[k] == V_ANULADO;         // anulada: renasce no fim
    if (ef == EF_MARTIR) {                                      // caido, com algum seu valendo
        if (f->est[k] != V_ANULADO) return 0;
        for (int i = 0; i < f->lancados; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    }
    if (f->est[k] != V_VALIDO) return 0;
    int outros = 0, baixos = 0, iguais = 0, uns = 0;           // os outros dados seus
    for (int i = 0; i < f->lancados; i++) {
        uns += f->cru[i] == 1;
        if (i == k || f->est[i] != V_VALIDO) continue;
        outros++;
        baixos += f->valor[i] <= 2;
        iguais += f->valor[i] == v;
    }
    switch (ef) {
    case EF_DOBRO:        return v <= 3;
    case EF_PAR:          return f->bonus[k] >= 6;
    case EF_CARRASCO:     return v == 4;
    case EF_MARTELO:      return v >= 3;
    case EF_PIRATA:       return v >= 7;
    case EF_ESPELHO:      return f->cru[k] <= 2;
    case EF_FICHAS:       return fichas_ini[j] > fichas_ini[outro(j)];
    case EF_COBRADOR:     return fichas_ini[outro(j)] > fichas_ini[j];
    case EF_TUDO_NADA:    return v == 2;
    case EF_GATUNO:       return uns > 0;
    case EF_INVEJOSO:     return invejoso_age(v) ? 1 : v <= 2 ? 2 : 0;
    case EF_CARIDOSO:     return v >= 3;
    case EF_FARTURA:      return baixos > 0 || v <= 2;
    case EF_SEMENTE:      return soma(j, f->est) >= 14;
    case EF_FOGO:         return outros > 0 ? (f->cru[k] == 2 ? 1 : 2) : 0;   // com 1, so queima
    case EF_TREVO:        return v == 4;
    case EF_JACKPOT:      return mesmo_numero(j, f->est, 2, NULL) != 0;
    case EF_COPIADOR:     return iguais > 0;
    case EF_LASER:        return v >= 5;
    case EF_FERREIRO:     return v >= 3;
    case EF_TRONO:        return k == N_FILA - 1 && outros == f->lancados - 1;
    case EF_VIUVA:        return outros == 0;
    case EF_DOMINO:       return k + 1 < f->lancados && f->valor[k + 1] == v;
    case EF_ANCORA:       return k == f->lancados - 1 || f->ancora[k + 1];
    case EF_CALIBRE:      return k == f->lancados - 1 || f->calibre[k + 1];
    case EF_PUXA:         return k == 0 && f->lancados == N_FILA && outros == N_FILA - 1;
    case EF_COVEIRO:
        if (v < 5) return 0;
        for (int q = 0; q < 2; q++)
            for (int i = 0; i < F[q].lancados; i++) if (F[q].est[i] == V_ANULADO) return 1;
        return 0;
    case EF_HEXA:         return 1;
    case EF_OURICO:       return 1;
    case EF_SABOTADOR:    return v >= 5;
    case EF_AZARAO:       return fichas_ini[j] < fichas_ini[outro(j)];
    case EF_AMPULHETA:    return ampulheta() > 0;
    case EF_AGIOTA:       return 1;
    case EF_MEDUSA:       return 1;
    case EF_RANCOR:       return J[j].perdidas > 0;
    case EF_ESPERANCA:    return fichas_ini[j] <= ESPERANCA_FICHAS;
    case EF_POTE:         return pote >= 25;
    case EF_GARIMPEIRO:                                      // um outro dado seu valendo
        for (int i = 0; i < f->lancados; i++)
            if (i != k && f->est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito != EF_GARIMPEIRO) return 1;
        return 0;
    case EF_CHECKPOINT:                                      // um dado seu valendo depois dele
        for (int i = k + 1; i < f->lancados; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    case EF_SOLITARIO:
        return !(k > 0 && f->est[k - 1] == V_VALIDO) && !(k + 1 < f->lancados && f->est[k + 1] == V_VALIDO);
    case EF_DESAFIANTE:                                      // o maior da mesa, sozinho
        for (int q = 0; q < 2; q++)
            for (int i = 0; i < F[q].lancados; i++)
                if (!(q == j && i == k) && F[q].est[i] == V_VALIDO && F[q].valor[i] >= v) return 0;
        return 1;
    case EF_BERSERKER: case EF_AUTOFAGIA:
        for (int i = 0; i < f->lancados; i++)
            if (i != k && f->est[i] == V_VALIDO && !firme(j, i)) return 2;
        return 0;
    case EF_VAMPIRO:      return v >= 3;
    case EF_COLECIONADOR: return v == 6;
    case EF_SACRIFICIO:                                      // um outro dado seu que pontua
        for (int i = 0; i < f->lancados; i++) if (i != k && f->est[i] == V_VALIDO && valor_pontos(j, i) > 0) return 1;
        return 0;
    case EF_SORTE: case EF_MALDICAO: case EF_MISERICORDIA: case EF_PRISMA: case EF_CAPITAO: case EF_SEGURANCA:
    case EF_MEDIA: case EF_ASSOMBRA:
        return 1;                                            // basta terminar valendo
    default:              return 0;
    }
}

// Funcao pura sobre copias: serve ao desfecho de verdade (gerando eventos) e
// a previa do botao "parar" de quem joga em segundo.
static void calcula_desfecho(desfecho_t *d, bool gera)
{
    memset(d, 0, sizeof *d);
    if (gera) n_ev = 0;

    // Espelho que esperava o oponente (com "?"): vira agora o maior dado dele,
    // e a sequencia do dono e refeita com o numero novo - so agora ele pode
    // cair ou derrubar outro. Enquanto esperava, nao anulava nem era anulado.
    // No desfecho de verdade fica assim; na previa, a mesa volta como estava.
    static fila_t f_antes[2];
    static char ver_antes[sizeof veredito];
    int vt = ver_tipo, aa = anulou_a, ab = anulou_b, res = resistiu;
    bool vc = ver_comum;
    uint8_t am = anulou_masc;
    bool virou = false;
    memcpy(f_antes, F, sizeof F);
    memcpy(ver_antes, veredito, sizeof veredito);
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
    for (int j = 0; j < 2; j++) {
        int alvo = maior_do_rival(j), virados = 0;
        uint8_t quais = 0;
        for (int k = 0; k < F[j].lancados; k++) {
            if (!F[j].espera[k] || F[j].est[k] != V_VALIDO || alvo <= 0) continue;
            F[j].valor[k] = alvo;
            F[j].espera[k] = false;
            quais |= (uint8_t)(1 << k);
            virados++;
            evento(gera, EV_ESPELHO, j, k, outro(j), 0, alvo, "Espelho vira o %d de %s", alvo, J[outro(j)].nome);
        }
        if (!virados && !revelados[j]) continue;
        if (gera) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        refaz_sequencia(j, F[j].lancados);
        for (int k = 0; k < F[j].lancados; k++)
            if (quais >> k & 1) snprintf(F[j].tag[k], sizeof F[j].tag[k], "=%d", alvo);
        virou = true;
    }
    memcpy(veredito, ver_antes, sizeof veredito);
    ver_tipo = vt; anulou_a = aa; anulou_b = ab; resistiu = res; anulou_masc = am; ver_comum = vc;
    for (int j = 0; j < 2; j++) memcpy(d->est[j], F[j].est, N_FILA);

    int ordem[2] = { primeiro, outro(primeiro) };
    for (int o = 0; o < 2; o++) {
        int j = ordem[o], r = outro(j);
        bool tem_um = false;
        for (int k = 0; k < F[j].lancados; k++) if (F[j].cru[k] == 1) tem_um = true;

        for (int k = 0; k < F[j].lancados; k++) {
            int ef = TIPO[tipo_de(j, k)].efeito, v = F[j].valor[k];
            if (d->est[j][k] == V_ROUBADO) continue;
            // Carrasco que tirou 1 (valendo ou nao): anula o menor dado de quem
            // o lancou - ele mesmo, se estiver valendo e for o menor (no empate,
            // ele).
            if (ef == EF_CARRASCO && F[j].cru[k] == 1) {
                int b = d->est[j][k] == V_VALIDO ? k : -1;
                for (int i = 0; i < F[j].lancados; i++)
                    if (i != k && d->est[j][i] == V_VALIDO && !firme(j, i)
                        && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
                if (b == k) {
                    d->est[j][b] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b], "Carrasco tirou 1: anula a si mesmo");
                } else if (b >= 0) {
                    d->est[j][b] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                           "Carrasco tirou 1: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
                }
            }
            // Todo poder so age se o dado terminar valendo. O Gatuno, alem
            // disso, precisa de algum 1 tirado por um dado seu.
            if (d->est[j][k] != V_VALIDO || (ef == EF_GATUNO && !tem_um)) continue;
            const char *nome = TIPO[tipo_de(j, k)].nome;

            // Berserker: anula um outro dado de quem o lancou, ao acaso - um
            // acaso que so depende da mesa, como o do Laser.
            if (ef == EF_BERSERKER) {
                int alvos[N_FILA], n = 0;
                for (int i = 0; i < F[j].lancados; i++)
                    if (i != k && d->est[j][i] == V_VALIDO && !firme(j, i)) alvos[n++] = i;
                if (n) {
                    uint32_t h = (uint32_t)(j * 5 + k);
                    for (int i = 0; i < F[j].lancados; i++) h = h * 29u + (uint32_t)F[j].valor[i];
                    for (int i = 0; i < F[r].lancados; i++) h = h * 13u + (uint32_t)F[r].valor[i];
                    int b = alvos[hash2((int)h, k + 23) % (uint32_t)n];
                    d->est[j][b] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                           "Berserker: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
                }
                continue;
            }

            // Autofagia: come o outro dado seu de menor numero que vale (no
            // empate, o primeiro): ele cai e sai da bolsa ate a proxima
            // partida, e ela ganha +1 (ate +3). Com a bolsa abaixo de 8 dados
            // em jogo, ou ja com +3, so anula. Escudo e Teimoso resistem.
            if (ef == EF_AUTOFAGIA) {
                int b = -1;
                for (int i = 0; i < F[j].lancados; i++)
                    if (i != k && d->est[j][i] == V_VALIDO && !firme(j, i)
                        && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
                if (b >= 0) {
                    d->est[j][b] = V_ANULADO;
                    bool come = !d->come[j] && autofagia_come_pode(j, k, b);
                    if (come) { d->come[j] = (uint8_t)(b + 1); d->come_de[j] = (uint8_t)(k + 1); }
                    if (come) evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                                     "Autofagia come o %d do próprio %s: +1", F[j].valor[b], J[j].nome);
                    else evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                                "Autofagia: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
                }
                continue;
            }

            bool couraca = tem_am(r, A_COURACA);         // os ataques dele nao anulam
            if (couraca && gera && ((ef == EF_CARRASCO && v == 4) || (ef == EF_LASER && v >= 5) || (ef == EF_INVEJOSO && invejoso_age(v))))
                am_acende(A_COURACA);
            if (gera && tem_am(r, A_IMA) && (ef == EF_CARIDOSO || ef == EF_GATUNO)) am_acende(A_IMA);
            if (ef == EF_CARRASCO && v == 4) {
                int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1), a = desvia_seguranca(d->est[r], r, a0);
                if (a >= 0) {
                    d->est[r][a] = V_ANULADO;
                    if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                        "Carrasco: o Segurança de %s leva no lugar", J[r].nome);
                    else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Carrasco anula o %d de %s", F[r].valor[a], J[r].nome);
                    d->atacados[r]++;
                    cavalo_abre(d, j, r, a, gera);
                }
            } else if (ef == EF_LASER && v >= 5) {
                // Um dado anulavel do oponente, ao acaso - mas um acaso que so
                // depende da mesa, para a previa e o desfecho darem o mesmo.
                int alvos[N_FILA], n = 0;
                for (int i = 0; i < F[r].lancados && !couraca; i++)
                    if (d->est[r][i] == V_VALIDO && !firme(r, i)) alvos[n++] = i;
                if (n) {
                    uint32_t h = (uint32_t)(j * 7 + k);
                    for (int i = 0; i < F[r].lancados; i++) h = h * 31u + (uint32_t)F[r].valor[i];
                    for (int i = 0; i < F[j].lancados; i++) h = h * 17u + (uint32_t)F[j].valor[i];
                    int a0 = alvos[hash2((int)h, k + 11) % (uint32_t)n], a = desvia_seguranca(d->est[r], r, a0);
                    if (a >= 0) {
                        d->est[r][a] = V_ANULADO;
                        if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                            "Laser: o Segurança de %s leva no lugar", J[r].nome);
                        else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                    "Laser anula o %d de %s", F[r].valor[a], J[r].nome);
                        d->atacados[r]++;
                        cavalo_abre(d, j, r, a, gera);
                    }
                }
            } else if (ef == EF_INVEJOSO && invejoso_age(v)) {   // Invejoso com 5 ou 6: o maior do oponente
                int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1), a = desvia_seguranca(d->est[r], r, a0);
                if (a >= 0) {
                    d->est[r][a] = V_ANULADO;
                    if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                        "Invejoso: o Segurança de %s leva no lugar", J[r].nome);
                    else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Invejoso anula o %d de %s", F[r].valor[a], J[r].nome);
                    d->atacados[r]++;
                    cavalo_abre(d, j, r, a, gera);
                }
            } else if (ef == EF_INVEJOSO && v <= 2) {    // com 1 ou 2: os seus dois menores, ele incluido
                for (int n = 0; n < 2; n++) {
                    int b = -1;
                    for (int i = 0; i < F[j].lancados; i++)
                        if (d->est[j][i] == V_VALIDO && !firme(j, i)
                            && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
                    if (b < 0) break;
                    d->est[j][b] = V_ANULADO;
                    if (b == k) evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b], "Invejoso tirou %d: anula a si mesmo", v);
                    else evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                                "Invejoso tirou %d: anula o %d do próprio %s", v, F[j].valor[b], J[j].nome);
                }
            } else if (ef == EF_SABOTADOR && v >= 5) {   // o maior do oponente passa a valer metade
                int a = -1;
                for (int i = 0; i < F[r].lancados; i++)
                    if (d->est[r][i] == V_VALIDO && (a < 0 || F[r].valor[i] > F[r].valor[a])) a = i;
                if (a >= 0) {
                    int base = valor_pontos(r, a) + F[r].bonus[a] + d->bonus[r][a];
                    int corte = base > 0 ? (base + 1) / 2 : 0;
                    if (corte) {
                        d->bonus[r][a] = (int8_t)(d->bonus[r][a] - corte);
                        evento(gera, EV_CORTA, j, k, r, a, corte, "Sabotador corta o %d de %s: vale %d",
                               F[r].valor[a], J[r].nome, base - corte);
                    }
                }
            } else if (ef == EF_VAMPIRO && v >= 3) {     // -1 em cada dado do oponente que vale
                uint8_t m = 0;
                for (int i = 0; i < F[r].lancados && i < N_FILA; i++)
                    if (d->est[r][i] == V_VALIDO) { d->bonus[r][i] = (int8_t)(d->bonus[r][i] - 1); m |= (uint8_t)(1 << i); }
                evento_t *e = m ? evento(gera, EV_VAMPIRO, j, k, r, 0, 1, "Vampiro: -1 em cada dado de %s", J[r].nome) : NULL;
                if (e) e->mascara = m;
            } else if (ef == EF_MARTELO && v >= 3) {     // Espinhoso: com 3 ou 4
                d->delta[r] -= 4;
                evento(gera, EV_TIRA, j, k, r, 0, 4, "Espinhoso: -4 para %s", J[r].nome);
            } else if (ef == EF_MALDICAO) {              // vale para o dono e tira do oponente
                d->delta[r] -= v;
                evento(gera, EV_TIRA, j, k, r, 0, v, "Maldito: -%d para %s", v, J[r].nome);
            } else if (ef == EF_PIRATA && v >= 7) {      // so tira; nao fica com os pontos
                d->delta[r] -= 4;
                evento(gera, EV_TIRA, j, k, r, 0, 4, "Pirata tira 4 de %s", J[r].nome);
            } else if ((ef == EF_CARIDOSO && v >= 3) || ef == EF_GATUNO) {   // Caridoso: com 3 ou 4
                int a0 = alvo_de(d->est[r], r, false, -1), a = desvia_seguranca(d->est[r], r, a0);
                if (a >= 0) {
                    int vale = contribui(r, a);
                    d->est[r][a] = V_ROUBADO;
                    d->roubo_val[j] += vale;
                    if (a != a0) evento(gera, EV_ROUBA, j, k, r, a, vale,
                                        "%s: o Segurança de %s vai no lugar (%d)", nome, J[r].nome, vale);
                    else evento(gera, EV_ROUBA, j, k, r, a, vale,
                                "%s rouba o %d de %s", nome, vale, J[r].nome);
                    d->atacados[r]++;
                    cavalo_abre(d, j, r, a, gera);
                }
            }
        }
    }
    // Fenix: anulada, renasce valendo o que tirou (roubada, nao).
    for (int o = 0; o < 2; o++) {
        int j = ordem[o];
        for (int k = 0; k < F[j].lancados; k++)
            if (d->est[j][k] == V_ANULADO && TIPO[tipo_de(j, k)].efeito == EF_FENIX) {
                d->est[j][k] = V_VALIDO;
                evento(gera, EV_RENASCE, j, k, j, k, F[j].valor[k], "Fênix renasce valendo %d", F[j].valor[k]);
            }
    }
    for (int o = 0; o < 2; o++) efeitos_do_fim(d, ordem[o], gera);
    for (int j = 0; j < 2; j++) {
        int t = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j]
              + d->delta[j] + d->semente[j];
        d->bruto[j] = t;
        d->total[j] = t < 0 ? 0 : t;
    }
    if (!gera && virou) memcpy(F, f_antes, sizeof F);   // previa: o Espelho segue esperando
}

// Pontuacao que a mesa mostra durante e depois da animacao dos eventos.
static int total_visivel(int j)
{
    int t = pontos_fila(j, vis_est[j], vis_bonus[j], vis_mult[j]) + vis_delta[j] + vis_semente[j];
    for (int i = 0; i < vis_nroubo[j]; i++) if (!vis_roubo[j][i].anulado) t += vis_roubo[j][i].valor;
    // Espelho cujo evento ainda nao tocou: na tela, ainda com o que tirou.
    // (Esses eventos vem antes de todos, sem bonus nem multiplicador ainda.)
    for (int k = 0; k < F[j].lancados; k++)
        if (vis_espelho[j][k] && vis_est[j][k] == V_VALIDO) t -= F[j].valor[k] - F[j].cru[k];
    return t < 0 ? 0 : t;
}

// Probabilidade de o proximo dado cair. Nao aparece mais na tela; fica para
// o simulador medir o jogo.
static int risco(int j)
{
    const fila_t *f = &F[j];
    int k = f->lancados;
    if (k >= f->n) return 0;
    int t = tipo_de(j, k), lados = TIPO[t].lados, ef = TIPO[t].efeito;
    int l = ultimo_da_regra(j, f->est, k);
    int L = l >= 0 ? f->valor[l] : 0;
    if (ef == EF_PONTE || ef == EF_VAZIO) return 0;   // nunca caem
    if (ef == EF_PREVISIVEL) return l >= 0 && (f->fixo[k] + 1 < 12 ? f->fixo[k] + 1 : 12) < L ? 100 : 0;
    if (ef == EF_QUEBRADO) return l >= 0 && (int8_t)f->fixo[k] < L ? 100 : 0;
    if (ef == EF_FANTASMA) return l >= 0 && (f->fixo[k] ? (int8_t)f->fixo[k] : 1) < L ? 100 : 0;
    if (egoista_antes(j, k) >= 0 && !resiste_egoista(t)) return 100;   // cai com certeza
    if (ef == EF_EGOISTA) {                  // so cai diante de quem resiste a ele
        int m = 0;
        for (int i = 0; i < k; i++)
            if (f->est[i] == V_VALIDO && resiste_ego(j, i) && f->valor[i] > m) m = f->valor[i];
        int ruins = 0;
        for (int a = 1; a <= lados; a++) ruins += a < m;
        return ruins * 100 / lados;
    }
    int ruins = 0, total = 0;
    if (ef == EF_VICIADO || ef == EF_SORTE_REVES) {
        for (int a = 1; a <= lados; a++)
            for (int b = 1; b <= (ef == EF_SORTE_REVES ? SR_REVES : lados); b++) {
                total++;
                if ((ef == EF_VICIADO ? (a > b ? a : b) : a - b) < L) ruins++;
            }
        return ruins * 100 / total;
    }
    for (int a = ef == EF_LASTRO ? 4 : 1; a <= lados; a++) {   // Lastro: so 4 a 12
        int v = a;
        total++;
        if (ef == EF_EXPLODE && v == lados) continue;
        if (ef == EF_ESPELHO && v <= 2) continue;
        if ((ef == EF_MALDITO && v <= 7) || (ef == EF_VIDRO && v == 1)) { ruins++; continue; }
        if (v < L) ruins++;
    }
    return ruins * 100 / total;
}
