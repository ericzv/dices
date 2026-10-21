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
    amul_t am = am_disparou;
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
        f->lado[k] = (uint8_t)lado_do_lance(t, f->valor[k], menor);
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
        if (hora_res == 2) golpe(j, 1u << k);           // o dado que decidiu: a mesa para nele
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
    if (t_congela > 0) {                     // no golpe a mesa esta parada: treme quando soltar
        if (px > golpe_px) { golpe_px = px; golpe_dur = dur; }
        return;
    }
    float agora = t_tremor > 0 ? tremor_px * t_tremor / tremor_dur : 0;
    if (px < agora) return;                  // ja treme mais que isso
    tremor_px = px;
    tremor_dur = t_tremor = dur;
}

// O dado no ar acabou de deitar: no lado do Chumbo, um baque surdo e a mesa
// treme um pouco; no do Remendo, os pontos da agulha (uma vez por pouso).
static void ad_pousou(void)
{
    voo_t *v = &voo[0];
    if (!n_voo || !v->parado || v->chumbou || v->fantasma) return;
    v->chumbou = true;
    int a = ad_no_lado(peca_fila(v->dono, v->slot), v->lado);
    if (a == AD_CHUMBO) { som_toca(som_adesivo(a)); treme(3, 0.22f); }
    else if (a == AD_REMENDO) som_toca(som_adesivo(a));
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

// O golpe: a mesa para por um instante (GOLPE_T, no tempo real), com um
// clarao nos dados masc de j (bit N_FILA: o placar), e o tremor pedido
// nesse meio-tempo so vem depois. Desligado o tremor (tecla T), nada disso.
#define GOLPE_T 0.10f
static void golpe(int j, unsigned masc)
{
    if (tremor_off) return;
    if (t_congela > 0 && congela_j == j) masc |= congela_masc;   // dois golpes juntos: um clarao so
    else if (t_congela <= 0) golpe_px = golpe_dur = 0;
    t_congela = congela_dur = GOLPE_T;
    congela_j = (int8_t)j;
    congela_masc = (uint8_t)masc;
}

// Corre no tempo real, antes do resto do passo: devolve true enquanto a
// mesa esta parada.
static bool golpe_passo(float dt_real)
{
    if (t_congela <= 0) return false;
    t_congela -= dt_real;
    if (t_congela > 0) return true;
    congela_j = -1;
    if (golpe_px > 0) treme(golpe_px, golpe_dur);
    return false;
}

// Os combos do fim da rodada: o Jackpot, o Capitao com a fila toda e um
// multiplicador de x4 para cima. Quando os pontos deles caem no placar, a
// mesa para num golpe, depois os dados pulam e ela treme.
static bool evento_grande(const evento_t *e)
{
    if (e->tipo == EV_JACKPOT) return true;
    if (e->tipo == EV_EXTRA && F[e->de_j].tipo[e->de_k] == D_CAPITAO && e->mascara == (1u << N_FILA) - 1) return true;
    if (e->tipo == EV_MULT)
        for (int k = 0; k < N_FILA; k++) if (e->por_dado[k] >= 4) return true;
    return false;
}

static void golpe_evento(const evento_t *e)
{
    if (!evento_grande(e)) return;
    golpe(e->de_j, (e->mascara & ((1u << N_FILA) - 1)) | 1u << N_FILA);
    crescendo(e->de_j, 2);
}

// No comeco de cada evento: o oponente sente o golpe (ou gosta do que fez).
static void reage_evento(const evento_t *e)
{
    if (e->tipo == EV_JACKPOT && e->de_j == 0) { rival_reage(RM_COMBO); hist_jackpot(); }
    else if ((e->tipo == EV_ANULA || e->tipo == EV_ROUBA || e->tipo == EV_TIRA || e->tipo == EV_PIRATA)
             && e->de_j == 1 && e->alvo_j == 0) rival_reage(RM_ATACOU);
}

// ---------------------------------------------------------------------------
// Numeros que contam: o placar de cada fila e as moedas do Desafiante sobem
// (ou descem) contando ate o valor de verdade, rapido de longe e devagar no
// fim, nunca menos que vmin por segundo (k: o quanto da distancia anda por
// segundo). Cada vez que o numero muda, ele da um pulinho (pulo, 1 -> 0).
// ---------------------------------------------------------------------------
static void contador_fixa(contador_t *c, int v)
{
    c->vis = (float)v;
    c->mostra = v;
    c->pulo = 0;
    c->n_som = 0;
}

// Devolve quantos o numero andou neste passo (com o sinal).
static int contador_passo(contador_t *c, int alvo, float dt, float dt_real, float k, float vmin)
{
    float d = (float)alvo - c->vis;
    if (d != 0) {
        float v = fabsf(d) * k, passo = (v > vmin ? v : vmin) * dt;
        c->vis = fabsf(d) <= passo ? (float)alvo : c->vis + (d > 0 ? passo : -passo);
    } else c->n_som = 0;                 // parou: a proxima contagem comeca a escada de novo
    if (c->pulo > 0) c->pulo -= dt_real / 0.16f;
    int m = (int)lroundf(c->vis), andou = m - c->mostra;
    if (andou) { c->pulo = 1; c->sinal = andou > 0 ? 1 : -1; }
    c->mostra = m;
    return andou;
}

// Quanto o placar de j vale agora: durante o jogo, os pontos da fila; no fim
// da rodada, o que os eventos ja mostraram; no resultado, o total.
static int placar_real(int j)
{
    if (fase == F_RESULTADO && venc_mao >= 0) return F[j].total;
    if (fase == F_EFEITOS || fase == F_RESULTADO) return total_visivel(j);
    return F[j].pontos;
}

// No fim da rodada a contagem faz tique, subindo de tom ate meia oitava; no
// jogo, quem fala e o arpejo do dado que vale.
static void placar_passo(float dt, float dt_real)
{
    bool desfecho_agora = fase == F_EFEITOS || fase == F_RESULTADO;
    if (t_tique_placar > 0) t_tique_placar -= dt_real;
    for (int j = 0; j < 2; j++) {
        contador_t *c = &placar_c[j];
        if (!F[j].lancados && !desfecho_agora) { contador_fixa(c, F[j].pontos); continue; }   // rodada nova
        if (contador_passo(c, placar_real(j), dt, dt_real, 10, 25) && desfecho_agora && t_tique_placar <= 0) {
            som_toca_tom(SOM_TIQUE, c->n_som / 2 < 6 ? c->n_som / 2 : 6);
            c->n_som++;
            t_tique_placar = 0.05f;
        }
    }
}

// ---------------------------------------------------------------------------
// A mola: quando o cursor anda (ou o dado escolhido entra na sequencia), o
// item da vez da um pulinho e assenta, como numa mola amortecida: sobe uns
// 4 pixels, passa um pouco do lugar na volta e para em um terco de segundo.
// ---------------------------------------------------------------------------
static float t_mola = 1;
static int mola_cursor = -1, mola_fase = -1;
static void mola_passo(float dt_real)
{
    if (cursor != mola_cursor || fase != mola_fase) { mola_cursor = cursor; mola_fase = fase; t_mola = 0; }
    else if (t_mola < 1) t_mola += dt_real;
}
static void mola_chuta(void) { t_mola = 0; }
static float mola_brilho(void)          // e o brilho de passagem dele (0..1; -1: nenhum)
{
    return t_mola < 0.45f ? t_mola / 0.45f : -1;
}
static int mola_px(void)                 // quanto o item da vez esta acima do lugar
{
    if (t_mola >= 0.4f) return 0;
    return (int)lroundf(5 * sinf(t_mola * 6 * 2 * 3.14159f) * expf(-t_mola / 0.11f));
}

// A respiracao: o que esta esperando (os dados da mao, os amuletos da faixa)
// sobe e desce um pixel, devagar, cada um no seu tempo - a mesa nunca fica
// parada de todo. Na sequencia nao: ali o numero precisa ficar quieto.
static int respira(int i)
{
    return (int)lroundf(sinf(t_relogio * 2.1f + i * 1.3f) * 0.9f);
}
