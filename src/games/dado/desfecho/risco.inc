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
