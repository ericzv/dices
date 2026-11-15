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

