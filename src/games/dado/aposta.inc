// ===========================================================================
// Aposta (entre os dois turnos)
// ===========================================================================
enum { OP_PASSAR, OP_APOSTAR10, OP_APOSTAR25, OP_PAGAR, OP_AUMENTAR, OP_CORRER };

// Quanto quem esta na vez pode subir alem de cobrir: o dobro, limitado ao
// que sobra para ele depois de cobrir e ao que o rival ainda tem.
static int aumento_possivel(void)
{
    int sobe = a_pagar;
    int sobra = J[vez].fichas - a_pagar;
    if (sobe > sobra) sobe = sobra;
    if (sobe > J[outro(vez)].fichas) sobe = J[outro(vez)].fichas;
    return sobe < 0 ? 0 : sobe;
}

static int opcoes(int *op)
{
    int n = 0;
    if (!a_pagar) { op[n++] = OP_PASSAR; op[n++] = OP_APOSTAR10; op[n++] = OP_APOSTAR25; }
    else {
        op[n++] = OP_PAGAR;
        if (!aumentou && aumento_possivel() > 0)
            op[n++] = OP_AUMENTAR;
        op[n++] = OP_CORRER;
    }
    return n;
}

static int limite(int quer)
{
    int m = J[0].fichas < J[1].fichas ? J[0].fichas : J[1].fichas;
    return quer < m ? quer : m;
}

static void rotulo_op(int op, char *s, int n)
{
    switch (op) {
    case OP_PASSAR:    snprintf(s, n, "passar"); break;
    case OP_APOSTAR10: snprintf(s, n, "apostar %d", limite(10)); break;
    case OP_APOSTAR25: snprintf(s, n, "apostar %d", limite(25)); break;
    case OP_PAGAR:     snprintf(s, n, "pagar %d", a_pagar); break;
    case OP_AUMENTAR:                    // sem fichas para dobrar, diz quanto sobe
        if (aumento_possivel() < a_pagar) snprintf(s, n, "subir %d", aumento_possivel());
        else snprintf(s, n, "dobrar");
        break;
    default:           snprintf(s, n, "correr"); break;
    }
}

// A cor de cada botao da aposta: passar neutro, apostar em vinho (25 mais
// forte), pagar em ouro velho, dobrar em vermelho vivo, correr em preto.
static uint16_t tinta_op(int op)
{
    switch (op) {
    case OP_PASSAR:    return RGB(44, 66, 58);
    case OP_APOSTAR10: return RGB(112, 40, 46);
    case OP_APOSTAR25: return RGB(150, 34, 40);
    case OP_PAGAR:     return RGB(112, 88, 38);
    case OP_AUMENTAR:  return RGB(168, 44, 36);
    default:           return RGB(20, 20, 22);       // correr
    }
}

static void executa(int op)
{
    jogador_t *p = &J[vez];
    bool maquina = anuncia_maquina();
    if (op != OP_PASSAR && op != OP_CORRER) som_toca(SOM_FICHA);
    switch (op) {
    case OP_PASSAR:
        if (maquina) anuncia("%s passou", p->nome, 0);
        if (++passes >= 2) { comeca_segundo(); return; }
        break;
    case OP_APOSTAR10:
    case OP_APOSTAR25: {
        int b = limite(op == OP_APOSTAR10 ? 10 : 25);
        if (b <= 0) { comeca_segundo(); return; }
        p->fichas -= b; pote += b; a_pagar = b;
        if (maquina) anuncia("%s apostou %d", p->nome, b);
        break;
    }
    case OP_PAGAR: {
        int b = a_pagar < p->fichas ? a_pagar : p->fichas;
        p->fichas -= b; pote += b;
        if (maquina) anuncia("%s pagou %d", p->nome, b);
        comeca_segundo();
        return;
    }
    case OP_AUMENTAR: {
        // Cobre a aposta e sobe o mesmo tanto, ate onde as fichas dos dois
        // alcancam. O rival paga so o que de fato subiu.
        int sobe = aumento_possivel();
        p->fichas -= a_pagar + sobe; pote += a_pagar + sobe;
        a_pagar = sobe;
        aumentou = true;
        if (maquina) anuncia("%s pagou e aumentou %d", p->nome, sobe);
        break;
    }
    case OP_CORRER: {
        int w = outro(vez);
        res_antes[0] = J[0].fichas; res_antes[1] = J[1].fichas;
        res_quem = w;
        J[w].fichas += pote;
        snprintf(aviso, sizeof aviso, "%s correu. %s leva %d", p->nome, J[w].nome, pote);
        som_toca(SOM_CORREU);
        pote = 0;
        venc_mao = -1;
        correu_venc = w;                  // leva tambem o premio, como numa vitoria
        primeiro = w;                     // quem levou joga primeiro na proxima
        for (int j = 0; j < 2; j++) memcpy(vis_est[j], F[j].est, N_FILA);
        recolhe_quebrados();
        t_fase = 0;
        fase = F_RESULTADO;
        return;
    }
    }
    vez = outro(vez);
    cursor = 0;
}
