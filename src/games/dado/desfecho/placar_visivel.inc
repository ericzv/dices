// ===========================================================================
// PLACAR VISIVEL: os pontos que a mesa mostra enquanto os eventos do fim
// tocam, um a um (total_visivel).
// Le: F e o que a animacao ja mostrou (vis_est, vis_bonus, vis_mult,
// vis_delta, vis_semente, vis_roubo, vis_espelho, vis_tremor). Nao le d.
// Grava: nada.
// Quem chama: a partida (partida.inc) e a hora da verdade (hora.inc).
// ===========================================================================

// Pontuacao que a mesa mostra durante e depois da animacao dos eventos.
static int total_visivel(int j)
{
    int t = pontos_fila(j, vis_est[j], vis_bonus[j], vis_mult[j]) + vis_delta[j] + vis_semente[j];
    for (int i = 0; i < vis_nroubo[j]; i++) if (!vis_roubo[j][i].anulado) t += vis_roubo[j][i].valor;
    // Espelho cujo evento ainda nao tocou: na tela, ainda com o que tirou.
    // (Esses eventos vem antes de todos, sem bonus nem multiplicador ainda.)
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        if (vis_espelho[j][k] && vis_est[j][k] == V_VALIDO) t -= F[j].valor[k] - F[j].cru[k];
        if (vis_tremor[j][k] && vis_est[j][k] == V_VALIDO) t -= F[j].valor[k] - vis_tremor_de[j][k];   // o sacudido, ainda parado
    }
    return t < 0 ? 0 : t;
}

