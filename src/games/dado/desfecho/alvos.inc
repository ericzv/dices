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
    if (!maior && tem_am(j, A_COURACA)) return -1;  // Couraca: ninguem rouba
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

