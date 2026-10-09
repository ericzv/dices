// ===========================================================================
// Premio
// ===========================================================================
// Peso por DADO, nao por raridade: comum 8, incomum 4, raro 2, lendario 1.
// Assim cada lendario e mais raro que cada raro, quantos houver em cada grupo.
// Moeda, d4 e d6 sao so da colecao inicial: nunca saem como premio.
static bool so_inicial(int t) { return t == D_D2 || t == D_D4 || t == D_D6; }
static int peso_dado(int t)
{
    if (so_desafio(t)) return 0;                 // so no Modo Desafiante
    static const int PESO[4] = { 8, 4, 2, 1 };
    return so_inicial(t) ? 0 : PESO[TIPO[t].raridade];
}
static int sorteia_tipo(void)
{
    int total = 0;
    for (int t = 0; t < D_N; t++) total += peso_dado(t);
    int r = sorte(total);
    for (int t = 0; t < D_N; t++) {
        r -= peso_dado(t);
        if (r < 0) return t;
    }
    return D_D8;
}

// ---------------------------------------------------------------------------
// Roleta do premio
// ---------------------------------------------------------------------------
static float slot_para(int i) { return slot_t0 + i * slot_dt; }   // quando o rolo i trava
static float slot_fim(void)   { return slot_para(2); }
static bool slot_girando(void) { return fase == F_PREMIO && t_slot >= 0 && t_slot < slot_fim(); }

// Os premios em ordem de raridade: o mais raro fica a direita e aparece por
// ultimo. Depois os rolos comecam a girar - no Desafiante com calma, nas
// partidas rapidas num piscar.
static void slot_comeca(void)
{
    for (int i = 1; i < 3; i++)
        for (int k = i; k > 0 && TIPO[oferta[k].tipo].raridade < TIPO[oferta[k - 1].tipo].raridade; k--) {
            peca_t t = oferta[k]; oferta[k] = oferta[k - 1]; oferta[k - 1] = t;
        }
    bool calma = modo == M_DESAFIO;
    slot_t0 = calma ? 0.95f : 0.35f;
    slot_dt = calma ? 0.6f : 0.2f;
    t_slot = 0;
    som_giro(true);
}

// Qualquer tecla (ou clique) durante o giro trava tudo de uma vez.
static void slot_pula(void)
{
    if (!slot_girando()) return;
    t_slot = slot_fim();
    som_giro(false);
}

static void solta_confete(int n)
{
    static const uint16_t COR[] = { C_OURO, C_AMARELO, C_ROSA, C_VERDE, C_REAL, C_BRANCO, C_VINHO_CLR };
    n_confete = n < MAX_CONFETE ? n : MAX_CONFETE;
    for (int i = 0; i < n_confete; i++) {
        confete_t *c = &confete[i];
        bool esq = i % 2 == 0;                   // dois canhoes, um em cada canto de baixo
        c->x = esq ? 30 + frnd(0, 20) : GFX_W - 30 - frnd(0, 20);
        c->y = GFX_H - 50 + frnd(-10, 10);
        c->vx = (esq ? 1 : -1) * frnd(60, 260);
        c->vy = -frnd(240, 470);
        c->giro = frnd(0, 6.28f);
        c->vg = frnd(-14, 14);
        c->cor = COR[i % (int)(sizeof COR / sizeof COR[0])];
        c->w = (uint8_t)(3 + (i % 3));
        c->h = (uint8_t)(2 + (i % 2));
    }
}

// Cacos de vidro: estouram do dado para todos os lados e caem.
static void solta_cacos(int x, int y)
{
    static const uint16_t COR[] = { C_VIDRO, C_BRANCO, C_ACO, C_VIDRO };
    for (int i = 0; i < 36 && n_confete < MAX_CONFETE; i++) {
        confete_t *c = &confete[n_confete++];
        float a = frnd(0, 6.2832f), v = frnd(60, 230);
        c->x = x + cosf(a) * 6;
        c->y = y + sinf(a) * 6;
        c->vx = cosf(a) * v;
        c->vy = sinf(a) * v - 140;
        c->giro = frnd(0, 6.28f);
        c->vg = frnd(-20, 20);
        c->cor = COR[i % 4];
        c->w = (uint8_t)(3 + i % 3);
        c->h = (uint8_t)(2 + i % 2);
    }
}

static void passo_confete(float dt)
{
    int m = 0;
    for (int i = 0; i < n_confete; i++) {
        confete_t *c = &confete[i];
        c->vy += 520 * dt;
        c->vx *= 1 - 1.4f * dt;
        if (c->vy > 90) c->vy = 90 + (c->vy - 90) * (1 - 3 * dt);   // papel plana ao cair
        c->x += c->vx * dt + sinf(c->giro) * 12 * dt;
        c->y += c->vy * dt;
        c->giro += c->vg * dt;
        if (c->y < GFX_H + 10) confete[m++] = *c;
    }
    n_confete = m;
}

static void desenha_confete(void)
{
    for (int i = 0; i < n_confete; i++) {
        const confete_t *c = &confete[i];
        float s = fabsf(cosf(c->giro));          // o papel virando no ar
        int w = (int)(c->w * (0.3f + 0.7f * s)) + 1;
        gfx_rect((int)c->x - w / 2, (int)c->y, w, c->h, s > 0.5f ? c->cor : gfx_mistura(c->cor, C_BARRA, 35));
    }
}

static void passo_slot(float dt)
{
    passo_confete(dt);
    if (t_slot < 0) return;
    if (fase != F_PREMIO) { t_slot = -1; som_giro(false); return; }
    float a = t_slot, fim = slot_fim();
    t_slot += dt;
    for (int i = 0; i < 3; i++)
        if (a < slot_para(i) && t_slot >= slot_para(i)) som_toca(SOM_SLOT);
    if (a < fim && t_slot >= fim) som_giro(false);
    if (a < fim + 0.1f && t_slot >= fim + 0.1f) {
        som_toca(SOM_PREMIO);
        solta_confete(modo == M_DESAFIO ? MAX_CONFETE : 60);
    }
    if (t_slot > fim + 5) t_slot = fim + 5;
}

static void abre_premio(int w)
{
    for (int i = 0; i < 3; i++) {
        bool rep;
        do {
            oferta[i] = nova_peca(sorteia_tipo());
            rep = false;
            for (int k = 0; k < i; k++) if (oferta[k].tipo == oferta[i].tipo) rep = true;
        } while (rep);
    }
    vez = w;
    cursor = 0;
    removendo = false;
    t_tira = 0;
    fase = F_PREMIO;
    if (anuncia_maquina()) t_slot = -1;      // a maquina escolhe sem a roleta
    else slot_comeca();
}
