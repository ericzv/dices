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
    int atacados[2];                 // as tentativas do outro de anular, atacar ou tirar pontos de cada um (Ourico)
    int agiota[2];                   // fichas que o Agiota de cada um leva do outro, fora do pote
    uint8_t medusa[2];               // a Medusa valeu: a fila vira Quebrados no fim da rodada
    uint8_t come[2], come_de[2];     // a Autofagia comeu: o dado (posicao + 1) e ela (posicao + 1); 0: nada
    uint8_t garimpo[2];              // o Garimpeiro valeu: posicoes dos outros validos que voltam para a bolsa
    uint8_t onda[2];                 // posicoes das Ondas que juntaram 9: viram Tsunami
    uint8_t meia[2];                 // posicoes das Meias anuladas que ja avisaram que guardam
    // Os adesivos (adesivos_de): o que fica depois da rodada.
    uint8_t moedas[2];               // Dourado: moedas para a run
    uint8_t bis[2];                  // Bis: 1 lugar a mais na sequencia da proxima rodada
    int gorjeta[2];                  // Gorjeta: fichas do rival para o dono, fora do pote
    uint8_t fermento[2], raspa[2];   // posicoes na fila: o Fermento que cresce, a Raspadinha que sai
    uint32_t raspa_lados[2][N_FILA]; // e de que lado ela sai (pela Dupla Face ou pela Moldura, nao e o de cima)
    uint8_t selo_rompe[2], selo_lacra[2];   // e o lacre do Selo que se rompe, e o que se fecha
    uint8_t velcro[2], fragil[2];    // o Velcro que volta para a mao, o Fragil que quebra
    uint8_t capa[2];                 // o Impermeavel que agiu: a capa fica ate o fim da partida
    uint8_t ratoeira[2], rato_bits[2];   // Ratoeiras que desarmaram (quantas e quais posicoes)
    uint8_t coleira_bits[2];             // as Coleiras que ja cobraram (quais posicoes)
    uint8_t fig_cresce[2];           // as Figurinhas que ativaram: o dado junta mais uma (posicoes)
    uint8_t fermento2[2], fig_cresce2[2];   // e a segunda vez, carimbadas: crescem de novo
    uint8_t bilhete[2];              // os Bilhetes Premiados que valeram (agem no fim de tudo)
    uint8_t dinamite[2];             // as Dinamites que estouraram ou apagaram: o pavio zera
    uint8_t balao[2];                // os Baloes que estouraram: o ar zera
    uint8_t bil_carimbo[2];          // e os carimbados: o bonus vale x3 em vez de x2
    uint8_t bil_holo[2][N_FILA];     // e quantos cada Holografico copia (por posicao)
} desfecho_t;

static int contribui(int j, int k)
{
    return (valor_pontos(j, k) + F[j].bonus[k]) * (F[j].dobra[k] ? 2 : 1);
}

// Alvo de um ataque na fila de j. maior = true: o maior, para ANULAR (Carrasco,
// Invejoso) - os nao anulaveis ficam de fora. maior = false: o menor, para
// ROUBAR (Caridoso, Gatuno) - os nao roubaveis ficam de fora.
static int alvo_de(const uint8_t *est, int j, bool maior, int exceto)
{
    int a = -1;
    if (!maior && tem_am(j, A_IMA)) return -1;      // Ima: ninguem rouba
    for (int i = 0; i < F[j].lancados; i++) {
        if (i == exceto || est[i] != V_VALIDO) continue;
        if (maior ? firme_sem_capa(j, i) : nao_roubavel(j, i)) continue;
        if (a < 0 || (maior ? F[j].valor[i] > F[j].valor[a] : F[j].valor[i] < F[j].valor[a]))
            a = i;
    }
    return a;
}

// Seguranca: um ataque ao dado a da fila de j cai no Seguranca dele, se houver
// um valendo (e nao for o proprio alvo). O alvo com a capa do Impermeavel nao
// precisa dele: o ataque bate na capa.
static bool impermeavel(int j, int k);
static int desvia_seguranca(const uint8_t *est, int j, int a)
{
    if (a < 0 || impermeavel(j, a)) return a;
    for (int i = 0; i < F[j].lancados; i++)
        if (i != a && est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_SEGURANCA) return i;
    return a;
}

// Os alvos por posicao, para o jogador prever (e se proteger pela ordem):
// o Laser pega o ultimo dado valido do oponente que pode ser anulado; o
// Tsunami, o primeiro; o Berserker, o vizinho na mesma casa do oponente (o
// de antes; sem ele, o de depois) - e, sem nenhum la, o seu.
static int alvo_ultimo(const uint8_t *est, int r)
{
    for (int i = F[r].lancados - 1; i >= 0; i--) if (est[i] == V_VALIDO && !firme_sem_capa(r, i)) return i;
    return -1;
}
static int alvo_primeiro(const uint8_t *est, int r)
{
    for (int i = 0; i < F[r].lancados; i++) if (est[i] == V_VALIDO && !firme_sem_capa(r, i)) return i;
    return -1;
}
static int alvo_vizinho(const uint8_t *est, int j, int k)
{
    for (int s = -1; s <= 1; s += 2) {
        int i = k + s;
        if (i >= 0 && i < F[j].lancados && est[i] == V_VALIDO && !firme_sem_capa(j, i)) return i;
    }
    return -1;
}

// O Terremoto k de j vai sacudir a mesa do oponente: terminou valendo com 7 ou 8.
static bool tremor_armado(int j, int k)
{
    return TIPO[tipo_de(j, k)].efeito == EF_TERREMOTO && F[j].est[k] == V_VALIDO && F[j].valor[k] >= TERREMOTO_DE;
}
static bool tremor_na_mesa(void)
{
    for (int j = 0; j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) if (tremor_armado(j, k)) return true;
    return false;
}
// O dado de r que o tremor sacode: o ultimo valido (o Universo nao treme).
static int alvo_tremor(const uint8_t *est, int r)
{
    for (int i = F[r].lancados - 1; i >= 0; i--) if (est[i] == V_VALIDO && !universo(r, i)) return i;
    return -1;
}
// O dado a de r rola de novo, como ele rola (com os adesivos dele: o Chumbo,
// o Avesso, o Remendo, o Reflexivo), pelo sorteio que vem da semente do
// Terremoto. O Calibre que tinha subido o lance sobe este tambem.
static uint32_t abalo_s;
static int rola_abalo(int lados)
{
    abalo_s ^= abalo_s << 13; abalo_s ^= abalo_s >> 17; abalo_s ^= abalo_s << 5;
    return 1 + (int)(abalo_s % (uint32_t)lados);
}
static void treme_dado(int r, int a, uint32_t semente_abalo)
{
    fila_t *f = &F[r];
    abalo_s = semente_abalo ? semente_abalo : 0x9E3779B9u;
    const peca_t *p = peca_fila(r, a);
    int t = f->tipo[a], menor, lado, antes;
    int x = sorteia_peca(t, f->fixo[a], p, rola_abalo, &menor, &lado, &antes);
    int a1 = ad_no_lado(p, lado);
    f->bomba[a] = a1 == AD_BOMBA;                // o Papel Bomba: rola de novo e soma
    if (a1 == AD_REFLEXIVO || a1 == AD_BOMBA) {
        int x1 = x;
        x = sorteia_peca(t, f->fixo[a], p, rola_abalo, &menor, &lado, &antes);
        if (f->bomba[a]) { f->bomba1[a] = (int8_t)x1; x += x1; }
    }
    int teto = TIPO[t].efeito == EF_SORTE_REVES ? TIPO[t].lados - 1 : TIPO[t].lados;
    if (f->calibre[a] && x < teto) { x++; if (lado && lado < ad_lado_max(t)) lado++; }
    f->cru[a] = f->valor[a] = x;
    f->lado[a] = (uint8_t)lado;
    f->descarte[a] = (uint8_t)menor;
    f->espera[a] = false;
    fatia(r, a, rola_abalo);
}

// Um evento novo no fim da lista (so quando gera). Devolve o evento, para
// completar a mascara e os numeros de cada dado, ou NULL.
static evento_t *evento(bool gera, int tipo, int dj, int dk, int aj, int ak, int valor,
                        const char *fmt, ...) __attribute__((format(printf, 8, 9)));
// O ataque do dado k de j chegou no dado a de r, que esta com a capa do
// Impermeavel: vai ate ele e nada pega - nem passa para outro dado. quem: o
// nome de quem ataca; ad: o adesivo que ataca (Mira, Ratoeira), ou 0.
static evento_t *capa_ev;                   // o evento do ultimo bloqueio (NULL sem gerar)
static desfecho_t *d_atual;                  // o desfecho em andamento (o Ourico conta ate o ataque que a bolha segura)
static bool bloqueia_capa(bool gera, int j, int k, int r, int a, const char *quem, int ad)
{
    if (a < 0 || !impermeavel(r, a)) return false;
    if (d_atual && j != r) d_atual->atacados[r]++;
    capa_ev = evento(gera, ad ? EV_ADESIVO : EV_ANULA, j, k, r, a, F[r].valor[a], "%s: o %d está protegido",
                     quem, F[r].valor[a]);
    if (capa_ev) { capa_ev->bloqueado = (uint8_t)(1 << a); capa_ev->ad = (uint8_t)ad; }
    return true;
}
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
// A Ratoeira (posicao a de r, com o lado dela para cima) foi anulada por um
// poder de j: desarma e anula o maior dado valendo de j (o Seguranca dele
// leva no lugar; a Couraca protege). Uma vez por dado e por rodada.
static void cavalo_abre(desfecho_t *d, int j, int k, int r, int a, bool gera);
static void ratoeira_desarma(desfecho_t *d, int j, int r, int a, bool gera)
{
    uint8_t bit = (uint8_t)(1 << a);
    if (d->rato_bits[r] & bit) return;
    d->rato_bits[r] |= bit;
    d->ratoeira[r]++;
    if (tem_am(j, A_COURACA)) {
        if (gera) am_acende(A_COURACA);
        return;
    }
    int b = -1;
    for (int i = 0; i < F[j].lancados && i < N_FILA; i++)
        if (d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i) && (b < 0 || F[j].valor[i] > F[j].valor[b])) b = i;
    b = desvia_seguranca(d->est[j], j, b);
    if (b < 0 || bloqueia_capa(gera, r, a, j, b, "Ratoeira", AD_RATOEIRA)) return;
    d->est[j][b] = V_ANULADO;
    d->atacados[j]++;
    evento_t *e = evento(gera, EV_ADESIVO, r, a, j, b, F[j].valor[b], "Ratoeira: anula o %d de %s", F[j].valor[b], J[j].nome);
    if (e) e->ad = AD_RATOEIRA;
    cavalo_abre(d, r, a, j, b, gera);               // o dado dele que caiu pode ser um Cavalo
}

// A Coleira: o dado dela (no lado de cima) foi roubado ou anulado por um
// ataque do oponente j (o dado k dele). O ataque vale; quem fez perde
// COLEIRA_PERDE no placar - uma vez por dado.
static void coleira_cobra(desfecho_t *d, int j, int k, int r, int a, bool gera)
{
    uint8_t bit = (uint8_t)(1 << a);
    if (j == r || !ad_fila_tem(r, a, AD_COLEIRA) || (d->coleira_bits[r] & bit)) return;
    d->coleira_bits[r] |= bit;
    d->delta[j] -= COLEIRA_PERDE;
    evento_t *e = evento(gera, EV_ADESIVO, r, a, j, k, COLEIRA_PERDE, "Coleira: %s %s e perde %d", J[j].nome,
                         d->est[r][a] == V_ROUBADO ? "rouba" : "anula", COLEIRA_PERDE);
    if (e) e->ad = AD_COLEIRA;
}
// Depois de um ataque do dado k de j que pegou o dado a de r: a Coleira cobra,
// a Ratoeira desarma, o Cavalo de Troia abre.
static void cavalo_abre(desfecho_t *d, int j, int k, int r, int a, bool gera)
{
    if (a >= 0 && a < N_FILA && (d->est[r][a] == V_ANULADO || d->est[r][a] == V_ROUBADO)) coleira_cobra(d, j, k, r, a, gera);
    if (a >= 0 && a < N_FILA && d->est[r][a] == V_ANULADO && ad_fila_tem(r, a, AD_RATOEIRA)) ratoeira_desarma(d, j, r, a, gera);
    if (TIPO[tipo_de(r, a)].efeito != EF_CAVALO) return;
    if (tem_am(j, A_COURACA)) {
        if (gera) am_acende(A_COURACA);
        return;
    }
    uint8_t mask = 0, capa = 0;
    int8_t perde[N_FILA] = { 0 };
    for (int i = 0; i < F[j].lancados; i++)
        if (d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i)) {
            if (impermeavel(j, i)) { capa |= (uint8_t)(1 << i); continue; }   // bate na capa
            perde[i] = (int8_t)contribui(j, i);
            d->est[j][i] = V_ANULADO;
            mask |= (uint8_t)(1 << i);
            d->atacados[j]++;
        }
    int roubado = d->roubo_val[j];
    d->roubo_val[j] = 0;
    if (!mask && !roubado && !capa) return;
    evento_t *e = evento(gera, EV_CAVALO, r, a, j, 0, roubado, "Cavalo de Troia: anula os de %s", J[j].nome);
    if (e) { e->mascara = mask; memcpy(e->por_dado, perde, sizeof perde); e->bloqueado = capa; }
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
    // dados diferentes. O dado com a capa do Impermeavel e alvo como os
    // outros, mas nao queima - e o fogo nao passa para outro.
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
        if (TIPO[tipo_de(j, k)].efeito == EF_PATINHOS && patinhos_ativo(f->fixo[k])) { m += 3; de[MULT_PATINHOS] |= bit; }
        // Pisca-pisca aceso: x2 nele mesmo.
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

    // Semente: se os dados deste jogador (com bonus, multiplicadores e os que
    // roubou) somam SEMENTE_BROTA ou mais, +SEMENTE_PTS no fim - fora de qualquer multiplicador.
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

    // A virada do Barao: +2 para ele por dado seu anulado na rodada.
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
    // Bonus fixos, fora dos multiplicadores (no mesmo lugar da Semente):
    // Misericordioso, Solitario, Prisma, Desafiante, Capitao, Trono e Seguranca.
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
    // anulada, avisa que guarda mais 2 (uma vez so, mesmo com a Saideira).
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        if (TIPO[tipo_de(j, k)].efeito != EF_MEIA) continue;
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

#define AGNUS_NUMERO 7                 // o Agnus: o numero que ressuscita
#define AGNUS_RAIO   8                 // e o que anula, com a luz, o maior do oponente (valendo)
// O evento dele na mesa: a gloria acende sobre o Agnus e, a partir de
// AGNUS_LUZ_0 segundos, uma luz sai dela para cada anulado, um a cada
// AGNUS_LUZ_PASSO; ela leva AGNUS_LEVANTA para chegar, e o dado se levanta.
#define AGNUS_LUZ_0     0.45f
#define AGNUS_LUZ_PASSO 0.32f
#define AGNUS_LEVANTA   0.22f

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
    if (ef == EF_AGNUS)                                         // com 7, valendo ou nao: ressuscita os anulados no fim;
        return v == AGNUS_NUMERO || (v == AGNUS_RAIO && f->est[k] == V_VALIDO);   // com 8 valendo: a luz no oponente
    if (ef == EF_FICHA_VELHA) return f->est[k] == V_ANULADO;   // anulada: +4 no fim
    if (ef == EF_ESTILINGUE) return v == 1 && k + 1 < f->n && f->lancados == k + 1;   // com 1, valendo ou nao: o proximo sai no maximo
    if (ef == EF_REVOLVER) return f->est[k] == V_ANULADO && revolver_dispara(f->fixo[k]);   // o tiro anulado: +12 no fim
    if (ef == EF_MEIA && f->est[k] == V_ANULADO) return 1;     // anulada: guarda +2
    if (ef == EF_FICHA_CORINGA) return f->curinga[k];          // com o curinga para cima: vira no fim
    if (ef == EF_CARROSSEL) return f->gira[k];                 // girando: para no fim
    if (ef == EF_MARTIR) {                                      // caido, com algum seu valendo
        if (f->est[k] != V_ANULADO) return 0;
        for (int i = 0; i < f->lancados; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    }
    if (f->est[k] != V_VALIDO) return 0;
    int outros = 0, baixos = 0, iguais = 0, uns = 0;           // os outros dados seus
    for (int i = 0; i < f->lancados; i++) {
        if (i == k || f->est[i] != V_VALIDO) continue;
        uns += f->cru[i] == 1;                                 // os 1 dos outros valendo (o Gatuno)
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
    case EF_SEMENTE:      return soma(j, f->est) >= SEMENTE_BROTA;
    case EF_FOGO:         return outros > 0 ? (f->cru[k] == 2 ? 1 : 2) : 0;   // com 1, so queima
    case EF_TREVO:        return v == 4;
    case EF_JACKPOT:      return iguais > 0;                    // outro seu valendo com o numero dele
    case EF_COPIADOR:     return iguais > 0;
    case EF_LASER:        return v >= 5;
    case EF_TERREMOTO:    return v >= TERREMOTO_DE;
    case EF_FERREIRO:     return v >= FERREIRO_ARMA;
    case EF_TRONO:        return k == FILA_NORMAL - 1 && outros == f->lancados - 1;
    case EF_VIUVA:        return outros == 0;
    case EF_DOMINO:       return k + 1 < f->lancados && f->valor[k + 1] >= v && f->valor[k + 1] <= v + 2;
    case EF_ANCORA:       return k == f->lancados - 1 || f->ancora[k + 1];
    case EF_CALIBRE:      return k == f->lancados - 1 || f->calibre[k + 1];
    case EF_PUXA:         return k == 0 && f->lancados >= FILA_NORMAL && outros == f->lancados - 1;
    case EF_COVEIRO:
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
    case EF_MOSAICO:      return 1;
    case EF_LEQUE:                                           // um outro seu valendo no lado com adesivo
        for (int i = 0; i < f->lancados; i++) if (i != k && f->est[i] == V_VALIDO && ad_fila(j, i)) return 1;
        return 0;
    case EF_BALAO:        return f->fixo[k] > 0 && v >= BALAO_ESTOURA && f->tipo[k] == D_BALAO;
    case EF_PAPAGAIO:     return 1;
    case EF_RETRATO:      return k > 0 && ad_fila(j, k) && f->est[k - 1] == V_VALIDO;
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
    case EF_BERSERKER:                                       // o vizinho do oponente; sem ele, o seu (contra o dono)
        if (!tem_am(outro(j), A_COURACA) && alvo_vizinho(F[outro(j)].est, outro(j), k) >= 0) return 1;
        return alvo_vizinho(f->est, j, k) >= 0 ? 2 : 0;
    case EF_AUTOFAGIA:
        for (int i = 0; i < f->lancados; i++)
            if (i != k && f->est[i] == V_VALIDO && !firme_sem_capa(j, i)) return 2;
        return 0;
    case EF_COLECIONADOR: return v >= COLECIONADOR_DE;
    case EF_BURACO:                                          // vai engolir um vizinho seu
        for (int s = -1; s <= 1 && f->faixa[k]; s += 2)
            if (k + s >= 0 && k + s < f->lancados && f->est[k + s] == V_VALIDO && !firme_sem_capa(j, k + s)) return 2;
        return 0;
    case EF_BONECO:       return v >= 8;
    case EF_TSUNAMI:      return v >= 9;
    case EF_PATINHOS:     return patinhos_ativo(f->fixo[k]);
    case EF_PISCA:        return pisca_aceso(f->fixo[k]);
    case EF_MEIA:         return f->fixo[k] > 0;
    case EF_ESTRELA:                                         // por ultimo, com um seu valendo abaixo
        if (k != f->lancados - 1) return 0;
        for (int i = 0; i < k; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    case EF_RENA:                                            // um vizinho valendo
        return (k > 0 && f->est[k - 1] == V_VALIDO) || (k + 1 < f->lancados && f->est[k + 1] == V_VALIDO);
    case EF_ONDA:         return onda_da_bolsa(j, k) && onda_depois(f->fixo[k], v) >= ONDA_VIRA;
    case EF_TRENO:                                           // um dado seu valendo antes dele
        for (int i = 0; i < k; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    case EF_JINGLE:                                          // um outro sino valendo
        for (int i = 0; i < f->lancados; i++)
            if (i != k && f->est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_JINGLE) return 1;
        return 0;
    case EF_SACRIFICIO:                                      // um outro dado seu que pontua
        for (int i = 0; i < f->lancados; i++) if (i != k && f->est[i] == V_VALIDO && valor_pontos(j, i) > 0) return 1;
        return 0;
    case EF_SEGURANCA:    return outros == f->lancados - 1;   // o +2: nenhum seu caido (o desvio vale sempre)
    case EF_SORTE: case EF_MALDICAO: case EF_MISERICORDIA: case EF_PRISMA: case EF_CAPITAO:
    case EF_MEDIA: case EF_ASSOMBRA: case EF_VAMPIRO:
        return 1;                                            // basta terminar valendo
    default:              return 0;
    }
}

// A mira: os dados que os poderes armados agora vao pegar no fim da rodada
// (anular, roubar ou cortar), pelas mesmas regras de alvo do fim, com a mesa
// como esta. Uma mascara por jogador (de quem e o dado mirado).
static void mira_menores(uint8_t *est, int j, int n, int exceto, uint8_t *masc)
{
    for (int q = 0; q < n; q++) {                // os n menores que podem ser anulados, um de cada vez
        int b = -1;
        for (int i = 0; i < F[j].lancados && i < N_FILA; i++)
            if (i != exceto && est[i] == V_VALIDO && !firme_sem_capa(j, i) && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
        if (b < 0) return;
        est[b] = V_ANULADO;
        *masc |= (uint8_t)(1 << b);
    }
}
static void miras_agora(uint8_t mira[2])
{
    mira[0] = mira[1] = 0;
    for (int j = 0; j < 2; j++) {
        int r = outro(j);
        const fila_t *f = &F[j];
        bool couraca = tem_am(r, A_COURACA);
        for (int k = 0; k < f->lancados && k < N_FILA; k++) {
            if (!poder_armado(j, k)) continue;
            int ef = TIPO[tipo_de(j, k)].efeito, v = f->valor[k], a = -1;
            bool desvia = true;                  // o Seguranca do oponente toma o ataque no lugar
            uint8_t est[N_FILA];
            memcpy(est, f->est, N_FILA);
            switch (ef) {
            case EF_CARRASCO:
                if (f->cru[k] == 1) mira_menores(est, j, 1, -1, &mira[j]);   // o seu menor (ele mesmo, se for)
                else if (v == 4 && !couraca) a = alvo_de(F[r].est, r, true, -1);
                break;
            case EF_INVEJOSO:
                if (invejoso_age(v)) { if (!couraca) a = alvo_de(F[r].est, r, true, -1); }
                else mira_menores(est, j, 2, -1, &mira[j]);                 // os seus dois menores
                break;
            case EF_LASER:   if (!couraca) a = alvo_ultimo(F[r].est, r); break;
            case EF_TSUNAMI: if (!couraca) a = alvo_primeiro(F[r].est, r); break;
            case EF_TERREMOTO:                   // sacode o ultimo valido do oponente (sem desvio)
                if (!couraca) a = alvo_tremor(F[r].est, r);
                desvia = false;
                break;
            case EF_BONECO:
                if (!couraca && F[r].lancados > 3 && F[r].est[3] == V_VALIDO && !firme_sem_capa(r, 3)) a = 3;
                break;
            case EF_SABOTADOR:                   // corta o maior do oponente (sem desvio)
                for (int i = 0; i < F[r].lancados && i < N_FILA; i++)
                    if (F[r].est[i] == V_VALIDO && !universo(r, i) && (a < 0 || F[r].valor[i] > F[r].valor[a])) a = i;
                desvia = false;
                break;
            case EF_CARIDOSO: a = alvo_de(F[r].est, r, false, -1); break;
            case EF_GATUNO: {                    // um roubo por 1 dos seus outros dados valendo
                int n = 0;
                uint8_t er[N_FILA];
                memcpy(er, F[r].est, N_FILA);
                for (int i = 0; i < f->lancados; i++) n += i != k && f->cru[i] == 1 && f->est[i] == V_VALIDO;
                for (int q = 0; q < n; q++) {
                    int b = desvia_seguranca(er, r, alvo_de(er, r, false, -1));
                    if (b < 0) break;
                    er[b] = V_ROUBADO;
                    mira[r] |= (uint8_t)(1 << b);
                }
                break;
            }
            case EF_BERSERKER: {                 // o vizinho do oponente; sem ele (ou com a Couraca), o seu
                a = couraca ? -1 : alvo_vizinho(F[r].est, r, k);
                int b = a < 0 ? alvo_vizinho(est, j, k) : -1;
                if (b >= 0) mira[j] |= (uint8_t)(1 << b);
                break;
            }
            case EF_AGNUS:                       // com 8 valendo: a luz no maior do oponente
                if (v == AGNUS_RAIO && f->est[k] == V_VALIDO && !couraca) a = alvo_de(F[r].est, r, true, -1);
                break;
            case EF_AUTOFAGIA: mira_menores(est, j, 1, k, &mira[j]); break;
            case EF_BURACO:
                for (int s = -1; s <= 1 && f->faixa[k]; s += 2)
                    if (k + s >= 0 && k + s < f->lancados && f->est[k + s] == V_VALIDO && !firme_sem_capa(j, k + s))
                        mira[j] |= (uint8_t)(1 << (k + s));
                break;
            default: break;
            }
            if (a >= 0 && desvia) a = desvia_seguranca(F[r].est, r, a);
            if (a >= 0) mira[r] |= (uint8_t)(1 << a);
        }
    }
}

// Os poderes dos dados de j que valem no fim (ataques, roubos, os que
// tiram do rival e os que agem contra o dono). A Saideira chama de novo.
//
// Um dado ataca antes de cair: quando um poder vai anular um dado do
// oponente que tem um ataque armado contra ele (anular, roubar, cortar ou
// tirar pontos: ataca_oponente), esse ataque acontece primeiro
// (contra_ataque) e so depois o dado e anulado. Assim, dois dados que se
// miram se anulam os dois, quem quer que tenha aberto a rodada. Cada dado
// age uma vez so (agiu[j], um bit por lugar); a Saideira zera os de quem
// repete.
static uint8_t agiu[2];
static void poder_de(desfecho_t *d, int j, int k, bool gera);

// O dado k de j (valendo) tem um ataque armado contra o oponente: os poderes
// que contam como ataque para o Ourico (d->atacados).
static bool ataca_oponente(const desfecho_t *d, int j, int k)
{
    if (k < 0 || k >= F[j].lancados || d->est[j][k] != V_VALIDO) return false;
    int ef = TIPO[tipo_de(j, k)].efeito, v = F[j].valor[k], r = outro(j);
    switch (ef) {
    case EF_BERSERKER: return !tem_am(r, A_COURACA) && alvo_vizinho(d->est[r], r, k) >= 0;   // sem vizinho no oponente, cai no dono
    case EF_CARRASCO:  return v == 4 && F[j].cru[k] != 1;
    case EF_LASER:     return v >= 5;
    case EF_TSUNAMI:   return v >= 9;
    case EF_BONECO:    return v >= 8;
    case EF_INVEJOSO:  return invejoso_age(v);
    case EF_AGNUS:     return v == AGNUS_RAIO;
    case EF_SABOTADOR: return v >= 5;
    case EF_VAMPIRO:   return true;
    case EF_MARTELO:   return v >= 3;
    case EF_MALDICAO:  return true;
    case EF_PIRATA:    return v >= 7;
    case EF_CARIDOSO:  return v >= 3;
    case EF_GATUNO:
        for (int i = 0; i < F[j].lancados; i++) if (i != k && F[j].cru[i] == 1 && d->est[j][i] == V_VALIDO) return true;
        return false;
    default:           return false;
    }
}

// O dado a de r vai ser anulado por um ataque: se ele tem um ataque armado
// que ainda nao aconteceu, acontece agora, antes.
static void contra_ataque(desfecho_t *d, int r, int a, bool gera)
{
    if (a < 0 || (agiu[r] >> a & 1) || !ataca_oponente(d, r, a)) return;
    poder_de(d, r, a, gera);
}

static void poderes_de(desfecho_t *d, int j, bool gera)
{
    for (int k = 0; k < F[j].lancados; k++)
        if (!(agiu[j] >> k & 1)) poder_de(d, j, k, gera);
}

// O poder do dado k de j, uma vez.
static void poder_de(desfecho_t *d, int j, int k, bool gera)
{
    int r = outro(j);
    agiu[j] |= (uint8_t)(1 << k);
    int ef = TIPO[tipo_de(j, k)].efeito, v = F[j].valor[k];
    if (d->est[j][k] == V_ROUBADO) return;
    // Carrasco que tirou 1 (valendo ou nao): anula o menor dado de quem
    // o lancou - ele mesmo, se estiver valendo e for o menor (no empate,
    // ele).
    if (ef == EF_CARRASCO && F[j].cru[k] == 1) {
        int b = d->est[j][k] == V_VALIDO ? k : -1;
        for (int i = 0; i < F[j].lancados; i++)
            if (i != k && d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i)
                && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
        if (bloqueia_capa(gera, j, k, j, b, "Carrasco tirou 1", 0)) {
        } else if (b == k) {
            d->est[j][b] = V_ANULADO;
            evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b], "Carrasco tirou 1: anula a si mesmo");
        } else if (b >= 0) {
            d->est[j][b] = V_ANULADO;
            evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                   "Carrasco tirou 1: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
        }
    }
    // Todo poder so age se o dado terminar valendo. O Gatuno, alem
    // disso, rouba um dado por 1 tirado por outro dado seu valendo.
    int roubos = 1;
    if (ef == EF_GATUNO) {
        roubos = 0;
        for (int i = 0; i < F[j].lancados; i++) roubos += i != k && F[j].cru[i] == 1 && d->est[j][i] == V_VALIDO;
    }
    if (d->est[j][k] != V_VALIDO || roubos <= 0) return;
    const char *nome = TIPO[tipo_de(j, k)].nome;

    // Berserker: anula o dado do oponente na casa vizinha (a de antes; sem
    // ele, a de depois). Sem vizinho no oponente (ou com a Couraca), a furia
    // cai no dono: o dado logo antes dele (sem ele, o logo depois).
    if (ef == EF_BERSERKER) {
        int ro = tem_am(r, A_COURACA) ? -1 : alvo_vizinho(d->est[r], r, k);
        if (ro >= 0) {
            int a = desvia_seguranca(d->est[r], r, ro);
            if (a >= 0 && !bloqueia_capa(gera, j, k, r, a, "Berserker", 0)) {
                contra_ataque(d, r, a, gera);           // o ataque dele, se tiver, vem antes
                d->est[r][a] = V_ANULADO;
                if (a != ro) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                    "Berserker: o Segurança de %s leva no lugar", J[r].nome);
                else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                            "Berserker anula o %d de %s", F[r].valor[a], J[r].nome);
                d->atacados[r]++;
                cavalo_abre(d, j, k, r, a, gera);
            }
            return;
        }
        int b = alvo_vizinho(d->est[j], j, k);
        if (b >= 0 && !bloqueia_capa(gera, j, k, j, b, "Berserker", 0)) {
            d->est[j][b] = V_ANULADO;
            evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                   "Berserker: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
        }
        return;
    }

    // Autofagia: come o outro dado seu de menor numero que vale (no
    // empate, o primeiro): ele cai e sai da bolsa ate a proxima
    // partida, e ela ganha +1 (ate +3). Com a bolsa abaixo de 8 dados
    // em jogo, ou ja com +3, so anula. Escudo e Teimoso resistem.
    if (ef == EF_AUTOFAGIA) {
        int b = -1;
        for (int i = 0; i < F[j].lancados; i++)
            if (i != k && d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i)
                && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
        bool pode = b >= 0 && !d->come[j] && autofagia_come_pode(j, k, b);
        if (bloqueia_capa(gera, j, k, j, b, "Autofagia", 0)) {   // o de capa: nem anula, nem come
        } else if (b >= 0) {
            d->est[j][b] = V_ANULADO;
            bool come = pode;
            if (come) { d->come[j] = (uint8_t)(b + 1); d->come_de[j] = (uint8_t)(k + 1); }
            if (come) evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                             "Autofagia come o %d do próprio %s: +1", F[j].valor[b], J[j].nome);
            else evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                        "Autofagia: anula o %d do próprio %s", F[j].valor[b], J[j].nome);
        }
        return;
    }

    // Buraco Negro que evolui: engole os seus dados logo antes e logo
    // depois dele na sequencia (os firmes resistem).
    if (ef == EF_BURACO) {
        for (int s = -1; s <= 1 && F[j].faixa[k]; s += 2) {
            int b = k + s;
            if (b < 0 || b >= F[j].lancados || d->est[j][b] != V_VALIDO || firme_sem_capa(j, b)) continue;
            if (bloqueia_capa(gera, j, k, j, b, "Buraco Negro", 0)) continue;
            d->est[j][b] = V_ANULADO;
            evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                   "Buraco Negro engole o %d do próprio %s", F[j].valor[b], J[j].nome);
        }
        return;
    }

    bool couraca = tem_am(r, A_COURACA);         // os ataques dele nao anulam
    if (couraca && gera && ((ef == EF_CARRASCO && v == 4) || (ef == EF_LASER && v >= 5) || (ef == EF_INVEJOSO && invejoso_age(v))
                            || (ef == EF_BONECO && v >= 8) || (ef == EF_TSUNAMI && v >= 9) || (ef == EF_AGNUS && v == AGNUS_RAIO)))
        am_acende(A_COURACA);
    if (gera && tem_am(r, A_IMA) && (ef == EF_CARIDOSO || ef == EF_GATUNO)) am_acende(A_IMA);
    if (ef == EF_CARRASCO && v == 4) {
        int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1), a = desvia_seguranca(d->est[r], r, a0);
        if (a >= 0 && !bloqueia_capa(gera, j, k, r, a, "Carrasco", 0)) {
            contra_ataque(d, r, a, gera);           // o ataque dele, se tiver, vem antes
            d->est[r][a] = V_ANULADO;
            if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Carrasco: o Segurança de %s leva no lugar", J[r].nome);
            else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                        "Carrasco anula o %d de %s", F[r].valor[a], J[r].nome);
            d->atacados[r]++;
            cavalo_abre(d, j, k, r, a, gera);
        }
    } else if ((ef == EF_LASER && v >= 5) || (ef == EF_TSUNAMI && v >= 9)) {
        // O Laser (5 ou 6) pega o ultimo dado do oponente que pode ser
        // anulado; o Tsunami (9 a 12), o primeiro.
        {
            int a0 = couraca ? -1 : ef == EF_LASER ? alvo_ultimo(d->est[r], r) : alvo_primeiro(d->est[r], r);
            int a = desvia_seguranca(d->est[r], r, a0);
            if (a >= 0 && !bloqueia_capa(gera, j, k, r, a, nome, 0)) {
                contra_ataque(d, r, a, gera);           // o ataque dele, se tiver, vem antes
                d->est[r][a] = V_ANULADO;
                if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                    "%s: o Segurança de %s leva no lugar", nome, J[r].nome);
                else if (ef == EF_TSUNAMI) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                                  "Tsunami leva o %d de %s", F[r].valor[a], J[r].nome);
                else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                            "Laser anula o %d de %s", F[r].valor[a], J[r].nome);
                d->atacados[r]++;
                cavalo_abre(d, j, k, r, a, gera);
            }
        }
    } else if (ef == EF_BONECO && v >= 8) {     // Boneco de Neve com 8: congela o 4o do oponente
        int a0 = !couraca && F[r].lancados > 3 && d->est[r][3] == V_VALIDO && !firme_sem_capa(r, 3) ? 3 : -1;
        int a = desvia_seguranca(d->est[r], r, a0);
        if (a >= 0 && !bloqueia_capa(gera, j, k, r, a, "Boneco de Neve", 0)) {
            contra_ataque(d, r, a, gera);           // o ataque dele, se tiver, vem antes
            d->est[r][a] = V_ANULADO;
            if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Boneco de Neve: o Segurança de %s leva no lugar", J[r].nome);
            else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                        "Boneco de Neve congela o 4º de %s", J[r].nome);
            d->atacados[r]++;
            cavalo_abre(d, j, k, r, a, gera);
        }
    } else if (ef == EF_INVEJOSO && invejoso_age(v)) {   // Invejoso com 5 ou 6: o maior do oponente
        int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1), a = desvia_seguranca(d->est[r], r, a0);
        if (a >= 0 && !bloqueia_capa(gera, j, k, r, a, "Invejoso", 0)) {
            contra_ataque(d, r, a, gera);           // o ataque dele, se tiver, vem antes
            d->est[r][a] = V_ANULADO;
            if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Invejoso: o Segurança de %s leva no lugar", J[r].nome);
            else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                        "Invejoso anula o %d de %s", F[r].valor[a], J[r].nome);
            d->atacados[r]++;
            cavalo_abre(d, j, k, r, a, gera);
        }
    } else if (ef == EF_AGNUS && v == AGNUS_RAIO) {   // Agnus com 8: a luz anula o maior do oponente
        int a0 = couraca ? -1 : alvo_de(d->est[r], r, true, -1), a = desvia_seguranca(d->est[r], r, a0);
        if (a >= 0 && !bloqueia_capa(gera, j, k, r, a, "Agnus", 0)) {
            contra_ataque(d, r, a, gera);           // o ataque dele, se tiver, vem antes
            d->est[r][a] = V_ANULADO;
            if (a != a0) evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                                "Agnus: o Segurança de %s leva a luz", J[r].nome);
            else evento(gera, EV_ANULA, j, k, r, a, F[r].valor[a],
                        "Agnus: a luz anula o %d de %s", F[r].valor[a], J[r].nome);
            d->atacados[r]++;
            cavalo_abre(d, j, k, r, a, gera);
        }
    } else if (ef == EF_INVEJOSO && v <= 2) {    // com 1 ou 2: os seus dois menores, ele incluido
        uint8_t ja = 0;                              // (o de capa e um dos dois: bate nela e fica)
        for (int n = 0; n < 2; n++) {
            int b = -1;
            for (int i = 0; i < F[j].lancados; i++)
                if (d->est[j][i] == V_VALIDO && !firme_sem_capa(j, i) && !(ja >> i & 1)
                    && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
            if (b < 0) break;
            ja |= (uint8_t)(1 << b);
            if (bloqueia_capa(gera, j, k, j, b, "Invejoso", 0)) continue;
            d->est[j][b] = V_ANULADO;
            if (b == k) evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b], "Invejoso tirou %d: anula a si mesmo", v);
            else evento(gera, EV_ANULA, j, k, j, b, F[j].valor[b],
                        "Invejoso tirou %d: anula o %d do próprio %s", v, F[j].valor[b], J[j].nome);
        }
    } else if (ef == EF_SABOTADOR && v >= 5) {   // o maior do oponente passa a valer metade
        int a = -1;
        for (int i = 0; i < F[r].lancados; i++)
            if (d->est[r][i] == V_VALIDO && !universo(r, i) && (a < 0 || F[r].valor[i] > F[r].valor[a])) a = i;
        if (a >= 0) {
            int base = valor_pontos(r, a) + F[r].bonus[a] + d->bonus[r][a];
            int corte = base > 0 ? (base + 1) / 2 : 0;
            if (corte) {
                d->atacados[r]++;
                d->bonus[r][a] = (int8_t)(d->bonus[r][a] - corte);
                evento(gera, EV_CORTA, j, k, r, a, corte, "Sabotador corta o %d de %s: pontua %d",
                       F[r].valor[a], J[r].nome, base - corte);
            }
        }
    } else if (ef == EF_VAMPIRO) {               // valendo: -1 em cada dado do oponente que vale
        uint8_t m = 0;
        for (int i = 0; i < F[r].lancados && i < N_FILA; i++)
            if (d->est[r][i] == V_VALIDO && !universo(r, i)) { d->bonus[r][i] = (int8_t)(d->bonus[r][i] - 1); m |= (uint8_t)(1 << i); }
        if (m) d->atacados[r]++;
        evento_t *e = m ? evento(gera, EV_VAMPIRO, j, k, r, 0, 1, "Vampiro: -1 em cada dado de %s", J[r].nome) : NULL;
        if (e) e->mascara = m;
    } else if (ef == EF_MARTELO && v >= 3) {     // Espinhoso: com 3 ou 4
        d->delta[r] -= 4;
        d->atacados[r]++;
        evento(gera, EV_TIRA, j, k, r, 0, 4, "Espinhoso: -4 para %s", J[r].nome);
    } else if (ef == EF_MALDICAO) {              // vale para o dono e tira do oponente
        d->delta[r] -= v;
        d->atacados[r]++;
        evento(gera, EV_TIRA, j, k, r, 0, v, "Maldito: -%d para %s", v, J[r].nome);
    } else if (ef == EF_PIRATA && v >= 7) {      // so tira; nao fica com os pontos
        d->delta[r] -= 4;
        d->atacados[r]++;
        evento(gera, EV_TIRA, j, k, r, 0, 4, "Pirata tira 4 de %s", J[r].nome);
    } else if ((ef == EF_CARIDOSO && v >= 3) || ef == EF_GATUNO) {   // Caridoso: com 3 ou 4; Gatuno: um por 1
        for (int q = 0; q < roubos; q++) {                             // o menor, depois o menor que sobrou...
            int a0 = alvo_de(d->est[r], r, false, -1), a = desvia_seguranca(d->est[r], r, a0);
            if (a < 0) break;
            int vale = contribui(r, a);
            d->est[r][a] = V_ROUBADO;
            d->roubo_val[j] += vale;
            if (a != a0) evento(gera, EV_ROUBA, j, k, r, a, vale,
                                "%s: o Segurança de %s vai no lugar (%d)", nome, J[r].nome, vale);
            else evento(gera, EV_ROUBA, j, k, r, a, vale,
                        "%s rouba o %d de %s", nome, vale, J[r].nome);
            d->atacados[r]++;
            cavalo_abre(d, j, k, r, a, gera);
        }
    }
}

// O Ranzinza (trunfo): da 5a rodada em diante, o seu maior dado valendo
// pontua metade - uma vez por rodada, depois dos poderes.
#define RANZINZA_RODADA 5
static void ranzinza_corta(desfecho_t *d, bool gera)
{
    if (!tem_reg(REG_RANZINZA) || rodada < RANZINZA_RODADA) return;
    int a = -1;
    for (int i = 0; i < F[0].lancados && i < N_FILA; i++)
        if (d->est[0][i] == V_VALIDO && !universo(0, i) && (a < 0 || F[0].valor[i] > F[0].valor[a])) a = i;
    if (a < 0) return;
    int base = valor_pontos(0, a) + F[0].bonus[a] + d->bonus[0][a];
    int corte = base > 0 ? (base + 1) / 2 : 0;
    if (!corte) return;
    d->bonus[0][a] = (int8_t)(d->bonus[0][a] - corte);
    evento(gera, EV_CORTA, 1, 0xFF, 0, a, corte, "%s: o seu %d pontua só %d", J[1].nome, F[0].valor[a], base - corte);
}

// Fenix: anulada, renasce valendo o que tirou (roubada, nao).
static void fenix_de(desfecho_t *d, int j, bool gera)
{
    for (int k = 0; k < F[j].lancados; k++)
        if (d->est[j][k] == V_ANULADO && TIPO[tipo_de(j, k)].efeito == EF_FENIX) {
            d->est[j][k] = V_VALIDO;
            evento(gera, EV_RENASCE, j, k, j, k, F[j].valor[k], "Fênix renasce e pontua %d", F[j].valor[k]);
        }
}

// Agnus: tirando 7 (valendo ou nao; roubado, nao), no fim todos os dados
// anulados do dono nesta rodada ressuscitam - os que a sequencia derrubou e
// os que os ataques anularam, ele mesmo incluido - e pontuam o que tiraram.
// Os roubados nao voltam. Um evento so, com todos (mascara).
static int agnus_armado(int j)
{
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        if (TIPO[tipo_de(j, k)].efeito == EF_AGNUS && F[j].valor[k] == AGNUS_NUMERO) return k;
    return -1;
}
static void agnus_de(desfecho_t *d, int j, bool gera)
{
    int ag = agnus_armado(j);
    if (ag < 0 || d->est[j][ag] == V_ROUBADO) return;
    uint8_t m = 0;
    int n = 0;
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        if (d->est[j][k] == V_ANULADO) { d->est[j][k] = V_VALIDO; m |= (uint8_t)(1 << k); n++; }
    if (!m) return;
    evento_t *e = evento(gera, EV_RESSUSCITA, j, ag, j, ag, n, n == 1 ? "Agnus: o anulado ressuscita"
                                                                      : "Agnus: %d anulados ressuscitam", n);
    if (!e) return;
    e->mascara = m;
    for (int k = 0; k < N_FILA; k++)              // o que cada um tirou
        e->por_dado[k] = (int8_t)(m >> k & 1 ? (F[j].valor[k] > 127 ? 127 : F[j].valor[k]) : 0);
}

// As figurinhas juntadas nos dados de j com Figurinha (cada um comeca com 1),
// e as Etiquetas coladas nos dados dele: o que a Figurinha e a Etiqueta pagam.
static int figurinhas_de(int j) { return conta_figurinhas(J[j].col, J[j].onde, J[j].n_col); }
static int etiquetas_de(int j)   { return conta_etiquetas(J[j].col, J[j].onde, J[j].n_col); }

// Os adesivos que agem no pouso, antes da fila (o Chumbo, o Reflexivo, o
// Avesso, o Remendo, o Post-it, a Nota Dez, o Papel Bomba), ou na regra da
// fila (o Transparente, o Corretivo, o Papel Carbono): no fim da rodada so
// contam para a Purpurina.
static bool ad_do_pouso(int a)
{
    return a == AD_CHUMBO || a == AD_REFLEXIVO || a == AD_AVESSO || a == AD_REMENDO || a == AD_TRANSPARENTE
        || a == AD_CORRETIVO || a == AD_POSTIT || a == AD_NOTA_DEZ || a == AD_BOMBA || a == AD_CARBONO
        || a == AD_BUSSOLA;
}

// O Infectado e o Barro sorteiam um lado livre de um dado da bolsa de q -
// qualquer dado (no Quebrado, so o lado do numero gravado; os estagios
// escondidos do ciclo cosmico nao levam adesivo; o lado virado para cima de
// um dado que ja caiu nesta rodada tambem nao). O sorteio sai do que esta
// na mesa (a rodada, quem age, o que tirou), sem gastar o sorteio da
// partida: os dois lados do online escolhem sempre o mesmo lugar (a Casca
// de Banana do fim usa a mesma semente).
static uint32_t pega_mistura(uint32_t h, uint32_t v) { return h ^ (v + 0x9E3779B9u + (h << 6) + (h >> 2)); }
static uint32_t pega_semente(int j, int k)
{
    uint32_t h = 0x51ED270Bu;
    h = pega_mistura(h, (uint32_t)rodada);
    h = pega_mistura(h, (uint32_t)(j * 16 + k));
    h = pega_mistura(h, (uint32_t)F[j].valor[k]);
    h = pega_mistura(h, (uint32_t)F[j].lado[k]);
    h = pega_mistura(h, (uint32_t)(J[0].n_col * 256 + J[1].n_col));
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
    return h;
}
// O lado l do dado i de q esta em jogo na mesa nesta rodada: virado para
// cima num dado que ja caiu, ou embaixo da Dupla Face de cima. O que ja caiu
// nao muda de cara: ai nada pega agora.
static bool lado_em_jogo(int q, int i, int l)
{
    const peca_t *p = &J[q].col[i];
    for (int k = 0; k < F[q].lancados && k < N_FILA; k++) {
        if (F[q].idx[k] != i) continue;
        int c = F[q].lado[k];
        if (l == c || (ad_no_lado(p, c) == AD_DUPLA && l == ad_oposto(p->tipo, c))) return true;
    }
    return false;
}
static int pega_lados(int q, int i, uint8_t *lados)
{
    const peca_t *p = &J[q].col[i];
    if (cosmo_oculto(p->tipo)) return 0;
    int n = 0;
    for (int l = ad_lado_min(p->tipo); l <= ad_lado_max(p->tipo) && l <= AD_LADOS; l++)
        if (ad_lado_serve(p, l) && !ad_no_lado(p, l) && !lado_em_jogo(q, i, l)) lados[n++] = (uint8_t)l;
    return n;
}
static int pega_dado(int q, uint32_t h)
{
    int cand[MAX_COL], n = 0;
    uint8_t lados[AD_LADOS];
    for (int i = 0; i < J[q].n_col && i < MAX_COL; i++) if (pega_lados(q, i, lados)) cand[n++] = i;
    return n ? cand[h % (uint32_t)n] : -1;
}
// O dado i da bolsa de q esta com a bolha do Impermeavel: de antes, nesta
// partida, ou porque caiu no lado dele nesta rodada (como nos ataques).
static bool bolsa_impermeavel(int q, int i)
{
    if (J[q].col[i].capa) return true;
    for (int k = 0; k < F[q].lancados && k < N_FILA; k++)
        if (F[q].idx[k] == i && impermeavel(q, k)) return true;
    return false;
}
// O que um adesivo fez assim que o dado caiu, para a mesa mostrar: o de j no
// dado k (ad), na bolsa de q, no dado i de la, nos lados (bit l: o lado l);
// n: quantos lados pegaram; capa: a bolha do Impermeavel segurou (o Barro).
typedef struct { int8_t j, k, q, i; uint8_t ad, capa, n; uint32_t lados; } pouso_ad_t;
// Os adesivos que agem no pouso do dado (o do lado de cima, ou o de baixo
// pela Dupla Face), valendo ou nao - e por isso mesmo se alguem correr:
// o Infectado (um germe num lado livre de um dado sorteado da sua bolsa), o
// Barro (um lado livre de um dado sorteado da bolsa do oponente vira Sujeira;
// com a bolha do Impermeavel, nao gruda) e a Massinha (o lado mais baixo sem
// adesivo do proprio dado ganha um Remendo), carimbados duas vezes e para
// sempre; e o Chiclete (o dado gruda: abre a proxima sequencia com o mesmo
// numero - um so por rodada, uma vez mesmo carimbado, e o que ja veio grudado
// nao gruda de novo). O sorteio vem da mesa (o online tira o mesmo). Devolve
// quantas vezes agiram (ate max), e como, em o.
static int adesivos_no_pouso(int j, int k, pouso_ad_t *o, int max)
{
    if (k < 0 || k >= F[j].lancados || k >= N_FILA) return 0;
    int a = ad_fila(j, k), r = outro(j), lado_a = F[j].lado[k];   // lado_a: de onde o adesivo age
    if (a == AD_DUPLA) {                         // o de baixo, pela Dupla Face, como se estivesse para cima
        a = ad_oposto_fila(j, k);
        const peca_t *pd = peca_fila(j, k);
        if (pd) lado_a = ad_oposto(pd->tipo, lado_a);
    }
    if (a == AD_CHICLETE) {
        int i = F[j].idx[k];
        if (max < 1 || !peca_fila(j, k) || F[j].chiclete || (k == 0 && F[j].grudado)) return 0;
        J[j].col[i].chiclete = 1;
        chic_valor[j] = F[j].valor[k];
        chic_lado[j] = F[j].lado[k];
        F[j].chiclete = (uint8_t)(k + 1);
        o[0] = (pouso_ad_t){ (int8_t)j, (int8_t)k, (int8_t)j, (int8_t)i, AD_CHICLETE, 0, 1, 0 };
        return 1;
    }
    if (a == AD_XEROX) {
        const peca_t *px = peca_fila(j, k);
        if (!px || max < 1) return 0;
        int b = ad_no_lado(px, ad_oposto(px->tipo, lado_a));   // o do outro lado do Xerox (pela Dupla Face, ela mesma)
        if (!b || b == AD_XEROX) return 0;
        int vezes = 1 + ((k > 0 && ad_fila_tem(j, k - 1, AD_CARIMBO)) || F[j].tipo[k] == D_TELA), n = 0;
        for (int g = 0; g < vezes && n < max; g++) {
            uint32_t h = pega_semente(j, k) ^ (0x5A17C3E9u * (uint32_t)(g + 1));
            int i = pega_dado(j, h);
            if (i < 0 || i == F[j].idx[k]) i = pega_dado(j, h * 2654435761u);   // de preferencia, outro dado
            if (i < 0 || i >= J[j].n_col) break;
            uint8_t lados[AD_LADOS];
            int nl = pega_lados(j, i, lados);
            if (!nl) break;
            int l = lados[(h >> 16) % (uint32_t)nl];
            ad_cola(&J[j].col[i], l, b);
            o[n++] = (pouso_ad_t){ (int8_t)j, (int8_t)k, (int8_t)j, (int8_t)i, AD_XEROX, 0, 1, 1u << l };
        }
        return n;
    }
    if (a != AD_INFECTADO && a != AD_BARRO && a != AD_MASSINHA) return 0;
    int vezes = 1 + ((k > 0 && ad_fila_tem(j, k - 1, AD_CARIMBO)) || F[j].tipo[k] == D_TELA), n = 0;
    for (int g = 0; g < vezes && n < max; g++) {
        uint32_t h = pega_semente(j, k) ^ (0x1F3C7E5Du * (uint32_t)(g + 1));
        pouso_ad_t p = { (int8_t)j, (int8_t)k, (int8_t)(a == AD_BARRO ? r : j), -1, (uint8_t)a, 0, 0, 0 };
        uint8_t lados[AD_LADOS];
        int i = a == AD_MASSINHA ? F[j].idx[k] : pega_dado(p.q, h);
        if (i < 0 || i >= J[p.q].n_col || (a == AD_MASSINHA && !peca_fila(j, k))) break;
        int nl = pega_lados(p.q, i, lados);
        if (!nl) break;                          // nenhum lado livre: nada
        p.i = (int8_t)i;
        if (a == AD_MASSINHA) {                  // o lado mais baixo (pega_lados vem dele)
            ad_cola(&J[j].col[i], lados[0], AD_REMENDO);
            p.lados = 1u << lados[0];
            p.n = 1;
        } else if (a == AD_INFECTADO) {
            int l = lados[(h >> 16) % (uint32_t)nl];
            ad_cola(&J[j].col[i], l, AD_INFECTADO);
            p.lados = 1u << l;
            p.n = 1;
        } else {                                 // o Barro: BARRO_LADOS lados sorteados, sem repetir
            p.capa = bolsa_impermeavel(r, i);
            for (int c = 0; c < BARRO_LADOS && nl > 0; c++) {
                int s2 = (int)((h >> (8 + c * 5)) % (uint32_t)nl);
                if (!p.capa) { ad_cola(&J[r].col[i], lados[s2], AD_SUJEIRA); p.n++; }
                p.lados |= 1u << lados[s2];
                lados[s2] = lados[--nl];
            }
        }
        o[n++] = p;
    }
    return n;
}

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
static bool ad_do_pouso(int a);
static bool copia_pode(int a)
{
    return a && a != AD_HOLOGRAFICO && a != AD_PURPURINA && a != AD_CARIMBO && a != AD_DUPLA && a != AD_MOLDURA
        && a != AD_BILHETE && a != AD_BARRO && a != AD_MASSINHA && a != AD_CHICLETE && a != AD_XEROX && !ad_do_pouso(a);
}
// O que o adesivo a faz no fim da rodada, pelo dado k de j: e dele, ou e o
// Holografico (no dado k) copiando o do dado ko (copia: nada sai nem cresce
// no dado de la; o Fermento paga o que pagou la). Devolve se agiu.
// O dado de antes de k tirou o Carimbo (a segunda vez e dele, nao da Tela em Branco).
static bool carimbado_ja(int j, int k) { return k > 0 && ad_fila_tem(j, k - 1, AD_CARIMBO); }
static bool ad_age(desfecho_t *d, int j, int k, int ko, int a, bool gera, bool copia)
{
    int r = outro(j);
    bool vale = d->est[j][k] == V_VALIDO, de_novo = age_modo & EVM_CARIMBO;
    uint8_t bit = (uint8_t)(1 << k);
    int8_t mx = de_novo ? 1 : 2;                 // o x2 carimbado: so mais um degrau
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
    case AD_INFECTADO:                           // valendo, +2 (o germe ja pegou na bolsa no pouso: adesivos_no_pouso)
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
    case AD_CORRENTE: {                          // +2 por elo: ela e cada adesivo seu que agiu nos dados antes dela
        if (!vale) return false;
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
        if (bloqueia_capa(gera, j, k, r, alvo, copia ? "Holográfico, como Mira" : de_novo ? "Carimbo » Mira" : "Mira", AD_MIRA)) {
            if (capa_ev) { capa_ev->copia = copia; capa_ev->modo = (uint8_t)age_modo; }
            return true;
        }
        d->est[r][alvo] = V_ANULADO;
        d->atacados[r]++;
        if (alvo != a0) e = evento(gera, EV_ADESIVO, j, k, r, alvo, F[r].valor[alvo], "%sMira: o Segurança de %s leva no lugar", pre, J[r].nome);
        else e = evento(gera, EV_ADESIVO, j, k, r, alvo, F[r].valor[alvo], "%sMira: anula o %d de %s", pre, F[r].valor[alvo], J[r].nome);
        if (e) { e->ad = (uint8_t)a; e->copia = copia; e->modo = (uint8_t)age_modo; }
        cavalo_abre(d, j, k, r, alvo, gera);
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
    if (a == AD_SUJEIRA) return ad_age(d, j, k, k, a, gera, false);   // o barro do oponente: nao e adesivo seu
    if (ad_do_pouso(a)) { if (primeira) (*agiram)++; return false; }
    // O Barro, a Massinha e o germe do Infectado ja agiram no pouso
    // (adesivos_no_pouso), valendo ou nao: aqui so contam - para a Purpurina,
    // e o Carimbo e a Dupla Face que os fizeram agir tambem. O Infectado
    // valendo ainda paga os +2 (ad_age).
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
        bool carimbo = carimbado(d, j, k), algum = false;
        int md = ad_fila(j, k) == AD_HOLOGRAFICO ? 0 : EVM_DUPLA;
        holo[n_holo++] = k;
        for (int c = 0; c < (carimbo ? 2 : 1); c++) {
            age_modo = md | (c ? EVM_CARIMBO : 0);
            bool esta = false;
            for (int f = 0; f < n_fonte; f++) esta |= ad_age(d, j, k, fonte[f], qual[f], gera, true);
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
        if (carimbado(d, j, k)) { purp_md[n_purp] = (uint8_t)(md | EVM_CARIMBO); purp[n_purp++] = k; n_purp_ad++; }
    }
    int cada = PURPURINA_CADA * (agiram + n_purp_ad);
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
        if (d->bilhete[j] >> k & 1) {
            dobra_bonus(d, j, k, gera, false, false);
            if (d->bil_carimbo[j] >> k & 1) dobra_bonus(d, j, k, gera, false, true);
        }
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
        for (int c = 0; c < d->bil_holo[j][k]; c++) dobra_bonus(d, j, k, gera, true, false);
}

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
    // ainda por lancar ja teriam saido da mesa (descarta_sobras), e a Rede
    // olha o ultimo dado lancado, como no fim de verdade. O n volta no fim.
    for (int j = 0; !gera && j < 2; j++)
        if (F[j].n > F[j].lancados) F[j].n = F[j].lancados;
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
    if (!gera && virou) memcpy(F, f_antes, sizeof F);   // previa: o Espelho segue esperando
    for (int j = 0; !gera && j < 2; j++) F[j].n = f_antes[j].n;
}

// Pontuacao que a mesa mostra durante e depois da animacao dos eventos.
static int total_visivel(int j)
{
    int t = pontos_fila(j, vis_est[j], vis_bonus[j], vis_mult[j]) + vis_delta[j] + vis_semente[j];
    for (int i = 0; i < vis_nroubo[j]; i++) if (!vis_roubo[j][i].anulado) t += vis_roubo[j][i].valor;
    // Espelho cujo evento ainda nao tocou: na tela, ainda com o que tirou.
    // (Esses eventos vem antes de todos, sem bonus nem multiplicador ainda.)
    for (int k = 0; k < F[j].lancados; k++) {
        if (vis_espelho[j][k] && vis_est[j][k] == V_VALIDO) t -= F[j].valor[k] - F[j].cru[k];
        if (vis_tremor[j][k] && vis_est[j][k] == V_VALIDO) t -= F[j].valor[k] - vis_tremor_de[j][k];   // o sacudido, ainda parado
    }
    return t < 0 ? 0 : t;
}

// Probabilidade de o proximo dado cair. Nao aparece mais na tela; fica para
// o simulador medir o jogo.
static bool estilingue_armado(int j, int k);
static int risco(int j)
{
    const fila_t *f = &F[j];
    int k = f->lancados;
    if (k >= f->n) return 0;
    int t = tipo_de(j, k), lados = TIPO[f->tipo[k]].lados, ef = TIPO[t].efeito;   // os lados: os do dado de verdade
    int l = ultimo_da_regra(j, f->est, k);
    int L = l >= 0 ? f->valor[l] : 0;
    const peca_t *p = peca_fila(j, k);       // cada lado com o numero que tira (o Remendo soma 2)
    if (estilingue_armado(j, k)) return l >= 0 && cai_depois(j, remenda(p, lados), L) ? 100 : 0;   // sai no maximo
    if (ef == EF_PONTE || ef == EF_VAZIO || ef == EF_SANGUESSUGA) return 0;   // nunca caem
    if (TIPO[f->tipo[k]].efeito == EF_FICHA_CORINGA) return 0;   // fica de fora ate o fim (a de verdade: o Camaleao atras dela entra na sequencia)
    if (TIPO[f->tipo[k]].efeito == EF_CARROSSEL) return 0;    // gira de fora ate o fim (o de verdade: o Camaleao atras dele nao gira)
    if (ef == EF_RELOGIO) return l >= 0 && cai_depois(j, remenda(p, relogio_valor()), L) ? 100 : 0;
    if (ef == EF_PREVISIVEL) return l >= 0 && cai_depois(j, remenda(p, previsivel_valor(f->fixo[k])), L) ? 100 : 0;
    if (ef == EF_QUEBRADO) return l >= 0 && cai_depois(j, remenda(p, (int8_t)f->fixo[k]), L) ? 100 : 0;
    if (ef == EF_FANTASMA) return l >= 0 && cai_depois(j, remenda(p, f->fixo[k] ? (int8_t)f->fixo[k] : 1), L) ? 100 : 0;
    if (ef == EF_EGOISTA || ef == EF_UNIVERSO) return 0;              // nao anulaveis: nao caem
    if (egoista_antes(j, k) >= 0 && !nao_anulavel_t(t)) return 100;   // cai com certeza
    if (ef == EF_DIGITAL) return 0;                           // acende sempre um acima
    int ruins = 0, total = 0;
    if (ef == EF_VICIADO || ef == EF_SORTE_REVES) {
        for (int a = 1; a <= lados; a++)
            for (int b = 1; b <= (ef == EF_SORTE_REVES ? SR_REVES : lados); b++) {
                total++;
                int va = remenda(p, a), vb = ef == EF_VICIADO ? remenda(p, b) : b;
                if (cai_depois(j, ef == EF_VICIADO ? (va > vb ? va : vb) : va - vb, L)) ruins++;
            }
        return ruins * 100 / total;
    }
    for (int a = ef == EF_LASTRO ? 4 : 1; a <= lados; a++) {   // Lastro: so 4 a 12
        int v = remenda(p, a);
        total++;
        if (ef == EF_EXPLODE && v >= lados) continue;
        if (ef == EF_MATRIOSKA && v >= 6) v += 3;               // abre: o d6 de dentro soma
        if (ef == EF_ESPELHO && v <= 2) continue;
        if ((ef == EF_MALDITO && v <= AGOURO_ATE) || (ef == EF_VIDRO && v == 1)) { ruins++; continue; }
        if (ad_no_lado(p, lado_do_lance(t, a, 0)) == AD_TRANSPARENTE) continue;   // a sequencia nao o ve: nao cai
        if (cai_depois(j, v, L)) ruins++;
    }
    if (ef == EF_GELATINA) return ruins * ruins * 100 / (total * total);   // quica: so cai se os dois lances caem
    return ruins * 100 / total;
}
