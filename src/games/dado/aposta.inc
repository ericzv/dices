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

// As opcoes, da esquerda para a direita, por quantas fichas entram: o que
// nao poe ficha nenhuma (passar; correr, separado) e, a direita, o que sobe.
// Assim a mao aprende um lado so: apostar 25 e dobrar ficam sempre a direita,
// e correr nunca cai onde estava o apostar.
static int opcoes(int *op)
{
    int n = 0;
    if (!a_pagar) { op[n++] = OP_PASSAR; op[n++] = OP_APOSTAR10; op[n++] = OP_APOSTAR25; }
    else {
        op[n++] = OP_CORRER;
        op[n++] = OP_PAGAR;
        if (!aumentou && aumento_possivel() > 0)
            op[n++] = OP_AUMENTAR;
    }
    return n;
}

// Onde o cursor comeca: no que nao arrisca nada a mais (passar ou pagar),
// nunca no correr.
static int cursor_aposta(void)
{
    int op[4], n = opcoes(op);
    for (int i = 0; i < n; i++) if (op[i] == OP_PASSAR || op[i] == OP_PAGAR) return i;
    return 0;
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

// A cor de cada botao da aposta, da paleta (a face sai na rampa dela):
// passar cinza, apostar e dobrar em vinho (o 25 em vermelho vivo), pagar em
// latao e correr no ebano, com o X vermelho.
static uint16_t tinta_op(int op)
{
    switch (op) {
    case OP_PASSAR:    return C_EBANO_B;
    case OP_APOSTAR10: return C_VINHO;
    case OP_APOSTAR25: return C_VERMELHO;
    case OP_PAGAR:     return C_LATAO;
    case OP_AUMENTAR:  return C_VINHO;
    default:           return C_EBANO;               // correr
    }
}

static void executa(int op)
{
    jogador_t *p = &J[vez];
    bool maquina = anuncia_maquina();
    if (op != OP_PASSAR && op != OP_CORRER) som_toca(SOM_FICHA);
    if (vez == 0 && (op == OP_APOSTAR10 || op == OP_APOSTAR25 || op == OP_AUMENTAR)) rival_reage(RM_APOSTA);
    else if (vez == 0 && op == OP_CORRER) rival_reage(RM_CORREU);
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
        p->perdidas++;                    // correr conta como perder (Rancor)
        p->perdeu_ultima = true;
        J[w].perdeu_ultima = false;
        J[w].fichas += pote;
        snprintf(aviso, sizeof aviso, "%s correu. %s leva %d", p->nome, J[w].nome, pote);
        if (tem_am(w, A_MAU_OLHADO)) poe_fantasma(outro(w), true);   // Mau-Olhado, a Bruxaria
        som_toca(SOM_CORREU);
        pote = 0;
        venc_mao = -1;
        correu_venc = w;                  // leva tambem o premio, como numa vitoria
        primeiro = w;                     // quem levou joga primeiro na proxima
        for (int j = 0; j < 2; j++) memcpy(vis_est[j], F[j].est, N_FILA);
        memset(cosmo_recem, 0, sizeof cosmo_recem);   // o ciclo cosmico evolui mesmo assim
        for (int j = 0; j < 2; j++) cosmo_ficam(j);
        recolhe_quebrados();
        t_fase = 0;
        fase = F_RESULTADO;
        return;
    }
    }
    vez = outro(vez);
    cursor = cursor_aposta();
}
