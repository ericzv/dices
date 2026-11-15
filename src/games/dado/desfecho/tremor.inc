// ===========================================================================
// TREMOR: o Terremoto armado (tremor_armado, tremor_na_mesa) e o dado que
// rola de novo no fim (treme_dado), pelo Terremoto ou pela Casca de Banana.
// Le: F e a peca do dado na bolsa (J[].col, pelos adesivos dele); nao le d
// nem ev[].
// Grava: treme_dado grava F (o dado sacudido: numero, lado, descarte...) e a
// semente abalo_s. Nao grava d, ev[] nem a bolsa.
// Quem chama: fase_anota_sorte, fase_terremoto e fase_casca
// (resolve_mesa.inc), so com gera; e fim_incerto (partida.inc).
// ===========================================================================

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
// O dado a de r rola de novo, como ele rola (com os adesivos dele: o Chumbo,
// o Avesso, o Remendo, o Reflexivo), pelo sorteio que vem da semente do
// Terremoto. O Calibre que tinha subido o lance sobe este tambem.
static uint32_t abalo_s;
static int rola_abalo(int lados)
{
    abalo_s ^= abalo_s << 13; abalo_s ^= abalo_s >> 17; abalo_s ^= abalo_s << 5;
    return 1 + (int)(abalo_s % (uint32_t)lados);
}
// O lance de um dado (lanca_peca, partida.inc). O Piao e a Gelatina rolam
// de novo de verdade: n_re lances de antes (re_x, re_lado), que o dado
// mostra um a um antes do que fica.
typedef struct { int x, menor, lado, x1, menor1, lado1, antes1; bool reflete, bomba;
                 uint8_t giros; bool quicou; uint16_t bonecas;
                 uint8_t n_re; int re_x[2]; uint8_t re_lado[2]; } lance_t;   // (re: ate PIAO_GIROS)
static lance_t lanca_peca(int t, int fixo, const peca_t *pc, int (*r)(int));
// O tremor rola o dado com o mesmo lance da mesa (lanca_peca: o Reflexivo e
// o Papel Bomba rolam de novo; o Previsivel, de novo, como um d12). Os poderes
// que mexem no lance (o Estilingue, o giro do Piao, o quique da Gelatina) nao
// se repetem no tremor: sao do lance de quem joga.
static void treme_dado(int r, int a, uint32_t semente_abalo)
{
    fila_t *f = &F[r];
    abalo_s = semente_abalo ? semente_abalo : 0x9E3779B9u;
    int t = f->tipo[a];
    lance_t l = lanca_peca(t, f->fixo[a], peca_fila(r, a), rola_abalo);
    int x = l.x, lado = l.lado;
    f->bomba[a] = l.bomba;                       // o Papel Bomba: rolou de novo e somou
    f->bomba1[a] = (int8_t)l.x1;
    f->bonecas[a] = l.bonecas;
    int teto = TIPO[t].efeito == EF_SORTE_REVES ? TIPO[t].lados - 1 : TIPO[t].lados;
    if (f->calibre[a] && x < teto) { x++; if (lado && lado < ad_lado_max(t)) lado++; }
    f->cru[a] = f->valor[a] = x;
    f->lado[a] = (uint8_t)lado;
    f->descarte[a] = (uint8_t)l.menor;
    f->espera[a] = false;
    fatia(r, a, rola_abalo);
}

