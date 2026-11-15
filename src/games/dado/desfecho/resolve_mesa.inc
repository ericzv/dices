// As fases da mesa: o que muda nos dados antes dos poderes do fim (o
// Oculto, o Espelho, o corte da previa e, em calcula_desfecho, o tremor, a
// Casca, a Ficha Coringa e o Carrossel). Sao as unicas fases que gravam em
// F; na previa, a mesa e o veredito voltam no fim de calcula_desfecho.

// A previa nao sabe o que o tremor e a Casca de Banana vao dar, mas sabe se
// vao agir: e o que ela anota aqui, com o Espelho que esperava ja virado e a
// sequencia dele refeita (fim_incerto, partida.inc).
static bool previa_sorte;
static bool casca_no_fim(void);

// O que as fases da mesa dividem numa chamada de calcula_desfecho.
typedef struct {
    bool gera;                                // o desfecho de verdade (gera eventos), ou a previa
    bool virou;                               // alguma fase mexeu na mesa: a previa a devolve no fim
    bool pre[2];                              // a sequencia de antes de cada um ja guardada (animacao)
    uint8_t revelados[2];                     // os Ocultos revelados agora, um bit por lugar
    fila_t *f_antes;                          // a mesa como estava
    char *ver_antes;                          // e o veredito da sequencia
    int vt, aa, ab, res;                      // ver_tipo, anulou_a, anulou_b, resistiu
    bool vc;                                  // ver_comum
    uint8_t am;                               // anulou_masc
} resolve_t;

// Guarda a mesa e o veredito da sequencia, que as fases refazem.
static void mesa_guarda(resolve_t *rm)
{
    rm->vt = ver_tipo; rm->aa = anulou_a; rm->ab = anulou_b; rm->res = resistiu;
    rm->vc = ver_comum;
    rm->am = anulou_masc;
    memcpy(rm->f_antes, F, sizeof F);
    memcpy(rm->ver_antes, veredito, sizeof veredito);
}

// A previa conta a rodada como se acabasse agora: os dados da sequencia
// ainda por lancar ja teriam saido da mesa, e a Rede passa a valer no
// ultimo dado lancado, com a sequencia refeita (descarta_sobras). A mesa
// volta no fim.
static void fase_sobras_rede(resolve_t *rm)
{
    for (int j = 0; !rm->gera && j < 2; j++)
        if (F[j].n > F[j].lancados) {
            F[j].n = F[j].lancados;
            if (tem_am(j, A_REDE)) { refaz_sequencia(j, F[j].lancados); rm->virou = true; }
        }
}

// O Oculto mostra o que tirou (so no desfecho de verdade: a previa nao
// sabe, e a IA imagina o numero antes): a sequencia e refeita com ele.
static void fase_oculto(resolve_t *rm)
{
    for (int j = 0; rm->gera && j < 2; j++)
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
            if (F[j].escondido[k]) {
                F[j].escondido[k] = false;
                rm->revelados[j] |= (uint8_t)(1 << k);
                evento(rm->gera, EV_REVELA, j, k, j, k, F[j].valor[k], "Oculto: tirou %d", F[j].valor[k]);
            }
}

// Espelho que esperava o oponente (com "?"): vira agora o maior dado dele,
// e a sequencia do dono e refeita com o numero novo - so agora ele pode
// cair ou derrubar outro. Enquanto esperava, nada o anulava (firme).
// No desfecho de verdade fica assim; na previa, a mesa volta como estava.
static void fase_espelho(resolve_t *rm)
{
    for (int j = 0; j < 2; j++) {
        int alvo = maior_do_rival(j), virados = 0;
        uint8_t quais = 0;
        for (int k = 0; k < F[j].lancados; k++) {
            if (!F[j].espera[k] || alvo <= 0) continue;
            F[j].valor[k] = alvo;
            lado_vira(j, k, alvo);
            F[j].espera[k] = false;
            quais |= (uint8_t)(1 << k);
            virados++;
            evento(rm->gera, EV_ESPELHO, j, k, outro(j), 0, alvo, "Espelho vira o %d de %s", alvo, J[outro(j)].nome);
        }
        if (!virados && !rm->revelados[j]) continue;
        if (rm->gera) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        rm->pre[j] = true;
        refaz_sequencia(j, F[j].lancados);
        for (int k = 0; k < F[j].lancados; k++)
            if (quais >> k & 1) poe_tag(&F[j], k, "=%d", alvo);
        if (rm->gera) guarda_seq_de(0, j, EV_REVELA, EV_ESPELHO);
        rm->virou = true;
    }
}

// A previa anota se o tremor ou a Casca vao agir (previa_sorte).
static void fase_anota_sorte(resolve_t *rm)
{
    if (!rm->gera) previa_sorte = tremor_na_mesa() || casca_no_fim();
}

// O Espelho que nao achou o que copiar no comeco (o oponente so com o
// Universo, o Vazio, ou com a Ficha Coringa e o Carrossel, que so viraram
// agora): vira aqui o maior do oponente, como la em cima. Se nem assim ha
// o que copiar, e anulado (como a Ficha Coringa sem alvo), a sequencia e
// refeita sem ele e a marca "=?" sai da mesa.
static void fase_espelho_tardio(resolve_t *rm)
{
    for (int j = 0; j < 2; j++) {
        int alvo = maior_do_rival(j), ev0 = n_ev;
        uint8_t quais = 0, nada = 0;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!F[j].espera[k]) continue;
            F[j].espera[k] = false;
            rm->virou = true;
            if (alvo <= 0) {
                if (!strcmp(F[j].tag[k], "=?")) F[j].tag[k][0] = 0;
                if (!strcmp(F[j].tag_num[k], "=?")) F[j].tag_num[k][0] = 0;
                F[j].espelho_nada[k] = true;
                nada |= (uint8_t)(1 << k);
                evento(rm->gera, EV_ESPELHO, j, k, outro(j), 0xFF, 0, "Espelho: %s não tem o que copiar, anulado", J[outro(j)].nome);
                continue;
            }
            F[j].valor[k] = alvo;
            lado_vira(j, k, alvo);
            quais |= (uint8_t)(1 << k);
            evento(rm->gera, EV_ESPELHO, j, k, outro(j), 0, alvo, "Espelho vira o %d de %s", alvo, J[outro(j)].nome);
        }
        if (!quais && !nada) continue;
        if (rm->gera && !rm->pre[j]) memcpy(est_pre_espelho[j], F[j].est, N_FILA);
        rm->pre[j] = true;
        refaz_sequencia(j, F[j].lancados);
        for (int k = 0; k < F[j].lancados; k++)
            if (quais >> k & 1) poe_tag(&F[j], k, "=%d", alvo);
        if (rm->gera) guarda_seq_de(ev0, j, EV_ESPELHO, EV_ESPELHO);
    }
}

// O veredito da sequencia volta a ser o de antes das fases.
static void mesa_devolve_veredito(const resolve_t *rm)
{
    memcpy(veredito, rm->ver_antes, sizeof veredito);
    ver_tipo = rm->vt; anulou_a = rm->aa; anulou_b = rm->ab; resistiu = rm->res; anulou_masc = rm->am; ver_comum = rm->vc;
}

// O estado de cada dado depois das fases da mesa: o desfecho parte dele.
static void fase_fotografa(desfecho_t *d)
{
    for (int j = 0; j < 2; j++) memcpy(d->est[j], F[j].est, N_FILA);
}

