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

