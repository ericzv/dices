// ===========================================================================
// ADESIVOS DO POUSO: os que agem assim que o dado cai (adesivos_no_pouso: o
// Chiclete, o Xerox, o Infectado, o Barro, a Massinha) e o sorteio que eles
// usam, tirado da mesa (pega_semente).
// Le: F, J[] (a bolsa dos dois) e a rodada. Nao le d nem ev[].
// Grava: a bolsa (J[].col: cola adesivos, marca o Chiclete) - e o unico
// destes arquivos que grava nela; F[].chiclete, chic_valor e chic_lado.
// Quem chama: o lance (partida.inc). pega_semente serve tambem ao tremor e a
// Casca do fim (desfecho/resolve_mesa.inc).
// ===========================================================================

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
    // ACHADO A25: preservado - o sorteio aceita o dado COMIDO pela Autofagia (nao olha J[q].onde)
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
// o Xerox (o adesivo do lado oposto ao dele e copiado num lado livre de um
// dado sorteado da sua bolsa - de preferencia outro dado; carimbado, duas
// vezes),
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

