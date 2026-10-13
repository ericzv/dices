// ===========================================================================
// A hora da verdade e o crescendo: a mesa reage ao que importa
// ===========================================================================
// A hora da verdade: o ultimo dado da rodada, quando quem leva o pote depende
// da face que ele tirar. A mesa escurece em volta dele, ele rola mais devagar
// e maior, hesita na quina antes de deitar, a trilha some e o som segura ate
// o veredito. Nada disso muda a regra: cada face e imaginada numa copia da
// mesa (como a IA faz), sem tocar no sorteio da partida - e por isso os dois
// lados do online chegam na mesma conta.
static int face_fixa;
static int rola_fixo(int lados) { return face_fixa < lados ? face_fixa : lados; }

// Quem leva a rodada com o desfecho d: 0, 1 ou 2 (empate; o Crupie leva o seu).
static int leva_rodada(const desfecho_t *d)
{
    if (d->total[0] == d->total[1]) return tem_reg(REG_CASA) ? 1 : 2;
    return d->total[0] > d->total[1] ? 0 : 1;
}

// Como fica a partida se w levar o pote desta rodada (2: ninguem leva):
// -1 segue; 0 ou 1 venceu; 2 empatou. As mesmas contas da nova_rodada.
static int partida_se(int w)
{
    int f[2] = { J[0].fichas, J[1].fichas };
    if (w < 2) f[w] += pote;
    int r = rodada + 1, max_r = modo == M_DESAFIO ? DES_RODADAS : MAX_RODADAS;
    int lider = f[0] == f[1] ? 2 : (f[0] > f[1] ? 0 : 1);
    bool extra = modo == M_DESAFIO && f[0] == f[1] && r <= max_r + DES_EXTRAS;
    if (r > max_r && !extra) return lider;
    int ante = ante_da(r);
    bool sem0 = f[0] < ante, sem1 = f[1] < ante;
    if (sem0 && sem1) return lider;
    if (sem0 || sem1) return sem0 ? 1 : 0;
    return -1;
}

// O dado k de j esta para ser lancado: e o ultimo da rodada (quem joga
// depois, no fim da fila) e o vencedor muda com a face? So conta se cada
// lado tem ao menos 1 chance em 10 - quase certo nao prende ninguem.
// 0 nao; 1 decide a rodada; 2 decide tambem a partida.
static int dado_decisivo(int j, int k)
{
    if (j == primeiro || k != F[j].n - 1) return 0;
    int t = F[j].tipo[k];
    // O Sorte/Reves vale de -7 a 11, cada diferenca com o seu peso (quantos
    // pares de d12 e d8 a dao); os outros, uma face de cada vez.
    bool duplo = TIPO[t].efeito == EF_SORTE_REVES;
    int L = TIPO[t].lados;
    int n = duplo ? L + SR_REVES - 1 : TIPO[t].efeito == EF_LASTRO ? 9 : L;
    mesa_t m;
    guarda_mesa(&m);
    uint32_t am = am_disparou;
    int ganha = 0, total = 0, fins = 0;      // jeitos em que j leva, de quantos; bit: como a partida fica
    for (int c = 0; c < n; c++) {
        fila_t *f = &F[j];
        int menor, peso = 1;
        if (duplo) {
            int dv = c - (SR_REVES - 1);
            int b0 = dv < 0 ? 1 - dv : 1, b1 = L - dv < SR_REVES ? L - dv : SR_REVES;
            peso = b1 - b0 + 1;              // os Reveses b que dao essa diferenca com uma Sorte a = dv + b
            f->cru[k] = f->valor[k] = dv;
            menor = b0;
        } else {
            face_fixa = c + 1;
            f->cru[k] = f->valor[k] = sorteia_valor(t, f->fixo[k], rola_fixo, &menor);
        }
        f->descarte[k] = (uint8_t)menor;
        f->lancados = k + 1;
        pousa(j, k, rola_fixo);
        desfecho_t d;
        calcula_desfecho(&d, false);
        int w = leva_rodada(&d);
        total += peso;
        if (w == j) ganha += peso;
        fins |= 1 << (partida_se(w) + 1);
        volta_mesa(&m);
    }
    am_disparou = am;
    if (ganha * 10 < total || (total - ganha) * 10 < total) return 0;
    bool acaba = (fins & ~1) != 0;
    return acaba && (fins & (fins - 1)) ? 2 : 1;
}

static void hora_comeca(int grau)
{
    hora = grau;
    if (!hora) return;
    som_suspense(true);                      // a trilha some
    som_toca(SOM_SUSPENSE);                  // e o coracao bate
    rival_reage(RM_HORA);                    // e o oponente fica serio
}

// O dado k de j acabou de pousar: a mesa reage. A fila cheia, os quatro
// valendo: a onda. Na hora da verdade a trilha volta - e, se o dado deu a
// rodada a quem jogou, a mesa treme junto.
static void reage_lance(int j, int k)
{
    int seguidos = 0;
    while (seguidos <= k && F[j].est[seguidos] == V_VALIDO) seguidos++;
    bool cheia = seguidos == N_FILA;
    int nivel = cheia ? 1 : 0, hora_res = 0;
    if (hora) {
        som_suspense(false);
        desfecho_t d;
        calcula_desfecho(&d, false);
        hora_res = leva_rodada(&d) == j ? 2 : 1;
        if (hora_res == 2 && nivel < 2) nivel = 2;
    }
    if (nivel) crescendo(j, nivel);
    hist_lance(j, cheia, hora_res);
}

// A luz da hora da verdade: em volta do dado o feltro como esta; fora do
// circulo, escurece aos poucos (com o pontilhado de sempre, para a paleta
// fazer degraus suaves). O circulo aperta enquanto a mesa escurece.
static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static void desenha_hora(void)
{
    if (t_hora <= 0) return;
    float t = t_hora * t_hora * (3 - 2 * t_hora);
    float raio = 92 + (1 - t) * 280, borda = 110;
    float forte = (hora == 2 ? 70 : 60) * t;           // % de escuro, no maximo
    uint16_t *fb = display_fb();
    for (int y = 0; y < GFX_H; y++) {
        float dy = (y - hora_y) * 1.3f;
        for (int x = 0; x < GFX_W; x++) {
            float dx = x - hora_x, d2 = dx * dx + dy * dy;
            if (d2 <= raio * raio) continue;
            float s = (sqrtf(d2) - raio) / borda;
            if (s > 1) s = 1;
            int a = (int)(s * forte + (BAYER[y & 3][x & 3] - 7.5f) * 1.6f);
            if (a <= 0) continue;
            uint16_t *p = &fb[y * GFX_W + x];
            *p = gfx_mistura(*p, C_BARRA, a > 100 ? 100 : a);
        }
    }
}

// Corre com o tempo do jogo: a luz entra enquanto o dado decide e sai depois
// do veredito; a onda e o tremor do crescendo se acalmam.
static void hora_passo(float dt, float dt_real)
{
    bool ativa = hora && (fase == F_ROLANDO || fase == F_ARRUMA || (fase == F_VEREDITO && t_fase < 0.3f));
    if (ativa) t_hora = fminf(1, t_hora + dt / 0.35f);
    else if (t_hora > 0) t_hora = fmaxf(0, t_hora - dt / 0.6f);
    if (hora && !ativa && t_hora <= 0) { hora = 0; som_suspense(false); }   // saiu da partida no meio
    if (n_voo && (fase == F_ROLANDO || fase == F_ARRUMA)) { hora_x = voo[0].x; hora_y = voo[0].y; }
    if (t_onda >= 0 && (t_onda += dt) > 0.9f + 0.07f * N_FILA) t_onda = -1;
    if (t_tremor > 0) t_tremor -= dt_real;
}

// ---------------------------------------------------------------------------
// O crescendo: nivel 1, a onda (os dados pulam um depois do outro, e as
// fichas do pote); 2, com faiscas e tremor; 3, com tremor maior.
// ---------------------------------------------------------------------------
static void treme(float px, float dur)
{
    float agora = t_tremor > 0 ? tremor_px * t_tremor / tremor_dur : 0;
    if (px < agora) return;                  // ja treme mais que isso
    tremor_px = px;
    tremor_dur = t_tremor = dur;
}

static void crescendo(int j, int nivel)
{
    if (t_onda < 0 || onda_j != j || nivel >= onda_nivel) {
        onda_j = j;
        onda_n = F[j].lancados;
        onda_nivel = nivel;
        t_onda = 0;
    }
    if (nivel >= 3) treme(3, 0.4f);
    else if (nivel == 2) treme(2, 0.28f);
}

// Quanto o dado k de j sobe na onda agora (em pixels).
static float onda_pulo(int j, int k)
{
    if (t_onda < 0 || j != onda_j || k >= onda_n) return 0;
    float q = (t_onda - 0.25f - k * 0.07f) / 0.28f;   // depois do pulinho de quem acabou de valer
    if (q <= 0 || q >= 1) return 0;
    return sinf(q * 3.1416f) * (onda_nivel >= 2 ? 7 : 5);
}

// O tremor: o quadro inteiro anda alguns pixels inteiros (sem borrar), e a
// beira que fica descoberta repete a ultima linha da tela.
static void aplica_tremor(void)
{
    if (t_tremor <= 0 || tremor_off) return;
    float q = t_tremor / tremor_dur;
    float a = tremor_px * q;
    int dx = (int)lroundf(a * sinf(t_relogio * 71)), dy = (int)lroundf(a * 0.6f * cosf(t_relogio * 53));
    if (!dx && !dy) return;
    static uint16_t antes[GFX_W * GFX_H];
    uint16_t *fb = display_fb();
    memcpy(antes, fb, sizeof antes);
    for (int y = 0; y < GFX_H; y++) {
        int sy = y - dy;
        sy = sy < 0 ? 0 : (sy >= GFX_H ? GFX_H - 1 : sy);
        for (int x = 0; x < GFX_W; x++) {
            int sx = x - dx;
            sx = sx < 0 ? 0 : (sx >= GFX_W ? GFX_W - 1 : sx);
            fb[y * GFX_W + x] = antes[sy * GFX_W + sx];
        }
    }
}

// Os combos do fim da rodada: o Jackpot, o Capitao com a fila toda e um
// multiplicador de x4 para cima fazem a mesa tremer.
static void reage_evento(const evento_t *e)
{
    int nivel = 0;
    if (e->tipo == EV_JACKPOT) nivel = 2;
    else if (e->tipo == EV_EXTRA && F[e->de_j].tipo[e->de_k] == D_CAPITAO && e->mascara == (1u << N_FILA) - 1) nivel = 2;
    else if (e->tipo == EV_MULT)
        for (int k = 0; k < N_FILA; k++) if (e->por_dado[k] >= 4) nivel = 2;
    if (nivel) crescendo(e->de_j, nivel);
    // O oponente sente o golpe (ou gosta do que fez).
    if (e->tipo == EV_JACKPOT && e->de_j == 0) { rival_reage(RM_COMBO); hist_jackpot(); }
    else if ((e->tipo == EV_ANULA || e->tipo == EV_ROUBA || e->tipo == EV_TIRA || e->tipo == EV_PIRATA)
             && e->de_j == 1 && e->alvo_j == 0) rival_reage(RM_ATACOU);
}
