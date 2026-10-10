// ===========================================================================
// Desfecho: poderes que mexem no rival, em sequencia
// ===========================================================================
typedef struct {
    uint8_t est[2][N_FILA];
    int8_t  bonus[2][N_FILA];        // Moeda da sorte
    int8_t  mult[2][N_FILA];         // soma dos multiplicadores (0 = nenhum)
    uint8_t queima[2][N_FILA];       // alvos da Ficha de Fogo
    int semente[2];
    uint8_t zerada[2];
    int roubo_val[2];
    int delta[2];
    int total[2];
    int bruto[2];                    // o total antes de travar em 0 (a previa mostra negativo)
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
        int t = tipo_de(j, i);
        if (maior ? nao_anulavel_t(t) : intocavel_t(t)) continue;
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
        if (d->est[j][i] == V_VALIDO && !nao_anulavel_t(tipo_de(j, i))) {
            perde[i] = (int8_t)contribui(j, i);
            d->est[j][i] = V_ANULADO;
            mask |= (uint8_t)(1 << i);
        }
    int roubado = d->roubo_val[j];
    d->roubo_val[j] = 0;
    if (!mask && !roubado) return;
    evento_t *e = evento(gera, EV_CAVALO, r, a, j, 0, roubado, "Cavalo de Troia: anula os de %s", J[j].nome);
    if (e) { e->mascara = mask; memcpy(e->por_dado, perde, sizeof perde); }
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
        const char *txt = face == 1 ? "Moeda 1: +1 nos ímpares" : "Moeda 2: +1 nos pares";
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

    // Multiplicadores: Dobro (no dado valido logo acima), Fartura (nos de
    // valor 1 e 2), Tudo ou Nada com 2 (em todos).
    bool fartura = false, tudo = false;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO) continue;
        int ef = TIPO[tipo_de(j, k)].efeito;
        if (ef == EF_FARTURA) fartura = true;
        if (ef == EF_TUDO_NADA && f->valor[k] == 2) tudo = true;
    }
    int n_far = 0;
    for (int k = 0; k < f->lancados; k++)
        if (est[k] == V_VALIDO && TIPO[tipo_de(j, k)].efeito == EF_FARTURA) n_far++;
    // Ficha de Fogo: +10, e o maior dado seu que pontua (sem contar ela)
    // queima no fim. Duas fichas queimam dois dados diferentes.
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
        d->semente[j] += 10;
        evento(gera, EV_FOGO, j, k, j, a, 10, "Fogo: +10, e o %d vira cinza", f->valor[a]);
    }

    uint8_t mask = 0;
    int sozinhos = 0;                            // dados valendo: a Viuva quer ser a unica
    for (int k = 0; k < f->lancados; k++) sozinhos += est[k] == V_VALIDO;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO) continue;
        int m = 0;
        // Dobro: vale so para o dado logo depois dele, e so se os dois
        // terminarem valendo. Anulado o seguinte, o x2 se perde; nao passa
        // adiante para o proximo que vale.
        // E so se o Dobro tirou de 1 a 3.
        if (k > 0 && est[k - 1] == V_VALIDO && TIPO[tipo_de(j, k - 1)].efeito == EF_DOBRO
            && f->valor[k - 1] <= 3) m += 2;
        if (fartura && f->valor[k] <= 2) m += 2 * n_far;
        if (tudo) m += 2;
        if (TIPO[tipo_de(j, k)].efeito == EF_TRONO && k == f->lancados - 1) m += 2;   // Trono
        if (TIPO[tipo_de(j, k)].efeito == EF_VIUVA && sozinhos == 1) m += 2;          // Viuva Negra sozinha
        if (j == 1 && virada_forca(2) && (k == 0 || (virada_forca(2) == 2 && k == 1)))
            m += 2;                              // o Barao virado: x2 no primeiro (e no segundo, na Cobertura)
        if (m) { d->mult[j][k] = (int8_t)m; mask |= 1 << k; }
    }
    if (mask && gera) {
        int antes = pontos_fila(j, d->est[j], d->bonus[j], NULL);
        int depois = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]);
        evento_t *e = evento(gera, EV_MULT, j, 0xFF, 0, 0, 0, "multiplicadores: %d vira %d", antes, depois);
        if (e) { e->mascara = mask; memcpy(e->por_dado, d->mult[j], N_FILA); }
    }

    // Semente: se os dados deste jogador (com bonus, multiplicadores e os que
    // roubou) somam 20 ou mais, +10 no fim - fora de qualquer multiplicador.
    int dados = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j];
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_SEMENTE) continue;
        if (dados < 20) continue;
        d->semente[j] += 10;
        evento(gera, EV_SEMENTE, j, k, j, 0, 10, "Semente: %d pontos, brota +10", dados);
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
    // Bonus fixos, fora dos multiplicadores (no mesmo lugar da Semente):
    // Misericordioso, Solitario, Prisma, Desafiante e Capitao.
    int anulados = 0, valendo = 0;
    for (int k = 0; k < f->lancados; k++) {
        if (est[k] == V_ANULADO) anulados++;
        if (est[k] == V_VALIDO) valendo++;
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
            d->semente[j] += 4;
            evento(gera, EV_EXTRA, j, k, j, 0, 4, "Solitário sem vizinhos: +4");
        } else if (ef == EF_PRISMA) {
            uint64_t tipos = 0;
            for (int i = 0; i < f->lancados; i++)
                if (est[i] == V_VALIDO) tipos |= 1ull << tipo_de(j, i);
            int nt = 0;
            for (; tipos; tipos >>= 1) nt += (int)(tipos & 1);
            d->semente[j] += nt;
            evento(gera, EV_EXTRA, j, k, j, 0, nt, "Prisma: %d tipo%s, +%d", nt, nt > 1 ? "s" : "", nt);
        } else if (ef == EF_CAPITAO) {               // +2 por dado seu valendo, ele incluido
            d->semente[j] += 2 * valendo;
            evento_t *e = evento(gera, EV_EXTRA, j, k, j, 0, 2 * valendo, "Capitão: %d dado%s, +%d",
                                 valendo, valendo > 1 ? "s" : "", 2 * valendo);
            for (int i = 0; e && i < f->lancados && i < N_FILA; i++)
                if (est[i] == V_VALIDO) { e->mascara |= (uint8_t)(1 << i); e->por_dado[i] = 2; }
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

    // Jackpot: tres dados valendo com o mesmo numero, +10 fora dos
    // multiplicadores (soma no mesmo bonus fixo da Semente).
    uint8_t trio = 0;
    int num = trinca(j, est, &trio);
    for (int k = 0; num && k < f->lancados; k++) {
        if (est[k] != V_VALIDO || TIPO[tipo_de(j, k)].efeito != EF_JACKPOT) continue;
        d->semente[j] += 10;
        evento_t *e = evento(gera, EV_JACKPOT, j, k, j, 0, 10, "JACKPOT! três %d: +10", num);
        if (e) e->mascara = trio;
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
        if (!virados) continue;
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
            // o lancou, fora ele mesmo.
            if (ef == EF_CARRASCO && F[j].cru[k] == 1) {
                int b = -1;
                for (int i = 0; i < F[j].lancados; i++)
                    if (i != k && d->est[j][i] == V_VALIDO && !nao_anulavel_t(tipo_de(j, i))
                        && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
                if (b >= 0) {
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
                    if (i != k && d->est[j][i] == V_VALIDO && !nao_anulavel_t(tipo_de(j, i))) alvos[n++] = i;
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
                    cavalo_abre(d, j, r, a, gera);
                }
            } else if (ef == EF_LASER && v >= 5) {
                // Um dado anulavel do oponente, ao acaso - mas um acaso que so
                // depende da mesa, para a previa e o desfecho darem o mesmo.
                int alvos[N_FILA], n = 0;
                for (int i = 0; i < F[r].lancados && !couraca; i++)
                    if (d->est[r][i] == V_VALIDO && !nao_anulavel_t(tipo_de(r, i))) alvos[n++] = i;
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
                        cavalo_abre(d, j, r, a, gera);
                    }
                }
            } else if (ef == EF_INVEJOSO && invejoso_age(v)) {   // Invejoso: com 1, 3 ou 5
                int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1), a = desvia_seguranca(d->est[r], r, a0);
                // Depois de anular o do oponente, anula o maior anulavel da
                // propria sequencia - ele mesmo, se for o maior.
                int b;
                if (a >= 0) {
                    d->est[r][a] = V_ANULADO;
                    if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                        "Invejoso: o Segurança de %s leva no lugar", J[r].nome);
                    else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Invejoso anula o %d de %s", F[r].valor[a], J[r].nome);
                    cavalo_abre(d, j, r, a, gera);
                }
                b = alvo_de(d->est[j], j, true, -1);
                if (b >= 0) {
                    d->est[j][b] = V_ANULADO;
                    evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                           "e anula o %d do próprio %s", F[j].valor[b], J[j].nome);
                }
            } else if (ef == EF_MARTELO && v >= 3) {     // Espinhoso: com 3 ou 4
                d->delta[r] -= 4;
                evento(gera, EV_TIRA, j, k, r, 0, 4, "Espinhoso: -4 para %s", J[r].nome);
            } else if (ef == EF_MALDICAO) {              // vale para o dono e tira do oponente
                d->delta[r] -= v;
                evento(gera, EV_TIRA, j, k, r, 0, v, "Maldito: -%d para %s", v, J[r].nome);
            } else if (ef == EF_PIRATA && v >= 6) {      // so tira; nao fica com os pontos
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
    for (int o = 0; o < 2; o++) {
        int j = ordem[o];
        if (!F[j].zerada) continue;
        d->zerada[j] = 1;
        int k = -1;
        for (int i = 0; i < F[j].lancados; i++)
            if (TIPO[tipo_de(j, i)].efeito == EF_TUDO_NADA && F[j].cru[i] == 1) { k = i; break; }
        evento(gera, EV_ZERA, j, k < 0 ? 0 : k, j, 0, 0, "Tudo ou Nada: a rodada vale 0");
    }
    for (int j = 0; j < 2; j++) {
        int t = pontos_fila(j, d->est[j], d->bonus[j], d->mult[j]) + d->roubo_val[j]
              + d->delta[j] + d->semente[j];
        d->bruto[j] = d->zerada[j] ? 0 : t;
        d->total[j] = (t < 0 || d->zerada[j]) ? 0 : t;
    }
    if (!gera && virou) memcpy(F, f_antes, sizeof F);   // previa: o Espelho segue esperando
}

// Pontuacao que a mesa mostra durante e depois da animacao dos eventos.
static int total_visivel(int j)
{
    if (vis_zerada[j]) return 0;
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
    int l = ultimo_valido(f->est, k);
    int L = l >= 0 ? f->valor[l] : 0;
    if (ef == EF_QUEBRADO) return f->fixo[k] < L ? 100 : 0;
    if (egoista_antes(j, k) >= 0 && !resiste_egoista(t)) return 100;   // cai com certeza
    if (ef == EF_EGOISTA) {                  // so cai diante de quem resiste a ele
        int m = 0;
        for (int i = 0; i < k; i++)
            if (f->est[i] == V_VALIDO && resiste_egoista(tipo_de(j, i)) && f->valor[i] > m) m = f->valor[i];
        int ruins = 0;
        for (int a = 1; a <= lados; a++) ruins += a < m;
        return ruins * 100 / lados;
    }
    int ruins = 0, total = 0;
    if (ef == EF_VICIADO) {
        for (int a = 1; a <= lados; a++)
            for (int b = 1; b <= lados; b++) { total++; if ((a > b ? a : b) < L) ruins++; }
        return ruins * 100 / total;
    }
    for (int a = ef == EF_LASTRO ? 4 : 1; a <= lados; a++) {   // Lastro: so 4 a 12
        int v = a;
        total++;
        if (ef == EF_EXPLODE && v == lados) continue;
        if (ef == EF_ESPELHO && v <= 2) continue;
        if ((ef == EF_MALDITO && v <= 7) || (ef == EF_TUDO_NADA && v == 1) || (ef == EF_VIDRO && v == 1)) { ruins++; continue; }
        if (v < L) ruins++;
    }
    return ruins * 100 / total;
}
