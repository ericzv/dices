// ===========================================================================
// ALVOS: quanto um dado vale num roubo (contribui) e quem cada poder atinge -
// o maior, o menor, o ultimo, o primeiro, o vizinho, o que treme - com o
// desvio para o Seguranca.
// Le: F (a mesa), o estado dos dados que quem chama passa (d->est ou F[].est)
// e os amuletos (a Couraca); contribui, tambem os cortes do fim (d->bonus).
// Nao le J[] nem ev[].
// Grava: nada de fora; mira_menores marca so a copia do estado e a mascara
// que recebe.
// Quem chama: reacoes, poderes, adesivos_fim, armado e resolve_mesa.
// ===========================================================================

// Quanto o dado k de j vale num roubo (e no aviso do Cavalo de Troia): o que
// ele pontua agora, ja com os cortes do fim (o Sabotador, o Vampiro: d->bonus).
// O corte nao o leva abaixo de 0; o que ja era negativo (o Reves) fica.
static int contribui(const desfecho_t *d, int j, int k)
{
    int base = valor_pontos(j, k) + F[j].bonus[k], v = base + d->bonus[j][k];
    if (base >= 0 && v < 0) v = 0;
    return v * (F[j].dobra[k] ? 2 : 1);
}

// Alvo de um ataque na fila de j. maior = true: o maior, para ANULAR (Carrasco,
// Invejoso) - os nao anulaveis ficam de fora. maior = false: o menor, para
// ROUBAR (Caridoso, Gatuno) - os nao roubaveis ficam de fora.
static int alvo_de(const uint8_t *est, int j, bool maior, int exceto)
{
    int a = -1;
    if (!maior && tem_am(j, A_COURACA)) return -1;  // Couraca: ninguem rouba
    for (int i = 0; i < F[j].lancados; i++) {
        if (i == exceto || est[i] != V_VALIDO) continue;
        if (maior ? firme_sem_capa(j, i) : nao_roubavel(j, i)) continue;
        if (a < 0 || (maior ? F[j].valor[i] > F[j].valor[a] : F[j].valor[i] < F[j].valor[a]))
            a = i;
    }
    return a;
}

// Seguranca: um ataque (ou roubo) ao dado a da fila de j cai no Seguranca
// dele, se houver um valendo (e nao for o proprio alvo). O alvo com a capa do
// Impermeavel nao precisa dele: o ataque, e o roubo, batem na capa.
static bool impermeavel(int j, int k);
static int desvia_seguranca(const uint8_t *est, int j, int a)
{
    if (a < 0 || impermeavel(j, a)) return a;
    // O preso pela Ancora tambem leva o golpe no lugar, e o segura (bloqueia_desvio).
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

// O dado de r que o tremor sacode: o ultimo valido (o Universo nao treme).
static int alvo_tremor(const uint8_t *est, int r)
{
    for (int i = F[r].lancados - 1; i >= 0; i--) if (est[i] == V_VALIDO && !universo(r, i)) return i;
    return -1;
}

// O dado valendo de j de menor (ou maior) numero que pode ser anulado, pela
// lista est: sem os firmes (a capa do Impermeavel nao conta: o ataque bate
// nela) e sem os lugares da mascara fora. No empate, o primeiro. -1 sem ele.
static int menor_valido(const uint8_t *est, int j, uint8_t fora)
{
    int b = -1;
    for (int i = 0; i < F[j].lancados && i < N_FILA; i++)
        if (!(fora >> i & 1) && est[i] == V_VALIDO && !firme_sem_capa(j, i)
            && (b < 0 || F[j].valor[i] < F[j].valor[b])) b = i;
    return b;
}
static int maior_valido(const uint8_t *est, int j, uint8_t fora)
{
    int b = -1;
    for (int i = 0; i < F[j].lancados && i < N_FILA; i++)
        if (!(fora >> i & 1) && est[i] == V_VALIDO && !firme_sem_capa(j, i)
            && (b < 0 || F[j].valor[i] > F[j].valor[b])) b = i;
    return b;
}
static void mira_menores(uint8_t *est, int j, int n, int exceto, uint8_t *masc)
{
    for (int q = 0; q < n; q++) {                // os n menores que podem ser anulados, um de cada vez
        int b = menor_valido(est, j, exceto >= 0 ? (uint8_t)(1 << exceto) : 0);
        if (b < 0) return;
        est[b] = V_ANULADO;
        *masc |= (uint8_t)(1 << b);
    }
}

