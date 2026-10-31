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
    bool cheia = seguidos >= FILA_NORMAL;
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
// Enquanto o dado decide, a luz da cena fica so nele: uma poca clara no
// feltro embaixo dele, e o circulo pulsa como um coracao (tum-tum, pausa).
static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static void luz_no_feltro(int cx, int cy, int rx, int ry, float tons);   // catalogo.inc
static void desenha_hora(void)
{
    if (t_hora <= 0) return;
    float t = t_hora * t_hora * (3 - 2 * t_hora);
    luz_no_feltro((int)hora_x, (int)hora_y + 16, 64, 24, 2.6f * t);
    float bate = fmodf(t_relogio * 1.15f, 1.0f);
    float pulso = bate < 0.08f ? 1 : bate > 0.16f && bate < 0.24f ? 0.6f : 0;
    float raio = 92 + (1 - t) * 280 + pulso * 6 * t, borda = 110;
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
    if (e->tipo == EV_EXTRA && F[e->de_j].tipo[e->de_k] == D_CAPITAO) {
        int n = 0;
        for (int k = 0; k < N_FILA; k++) n += e->mascara >> k & 1;
        if (n >= FILA_NORMAL) return true;
    }
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

// ---------------------------------------------------------------------------
// O tiro do Revolver, quando a sexta bala entra: o estampido, a mesa para
// num clarao e treme, sai o fogo da boca do cano, a bala risca a mesa ate a
// fila do oponente (e ricocheteia la) e a fumaca sobe do tambor.
// ---------------------------------------------------------------------------
#define TIRO_T 1.5f
static float t_tiro = -1;
static int tiro_x, tiro_y, tiro_ate, tiro_dir;

static void dispara_revolver(int j, int k)
{
    t_tiro = 0;
    tiro_x = SLOT_X(k);
    tiro_y = zona(j);
    tiro_ate = zona(outro(j));
    tiro_dir = tiro_ate < tiro_y ? -1 : 1;
    som_toca(SOM_TIRO);
    golpe(j, 1u << k);
    treme(4, 0.35f);
}

static void tiro_passo(float dt)
{
    if (t_tiro < 0) return;
    t_tiro += dt;
    if (t_tiro > TIRO_T) t_tiro = -1;
}

static void desenha_tiro(void)
{
    if (t_tiro < 0) return;
    float t = t_tiro;
    int bx = tiro_x, by = tiro_y + tiro_dir * (R_DADO + 2);      // a boca do cano: a beira do dado, do lado do oponente
    // A fumaca: nuvens que saem da boca, abrem para os lados (em volta do
    // numero que sobe do dado), crescem e sobem devagar.
    static const float FU[8][3] = { { -1.0f, 0.1f, 0.00f }, { 1.0f, 0.2f, 0.02f }, { -0.5f, 0.6f, 0.05f },
                                    { 0.6f, 0.7f, 0.08f }, { -1.6f, 0.9f, 0.11f }, { 1.5f, 1.0f, 0.14f },
                                    { -0.2f, 1.3f, 0.18f }, { 0.9f, 1.5f, 0.22f } };
    for (int i = 0; i < 8; i++) {
        float q = (t - FU[i][2]) / (TIRO_T - FU[i][2]);
        if (q <= 0 || q >= 1) continue;
        float x = bx + FU[i][0] * 14 * (0.5f + q), y = by + tiro_dir * FU[i][1] * 10 - q * 30;
        int r = (int)(3 + 8 * sqrtf(q) * (1 - q * 0.45f));
        if (q > 0.7f && ((int)(t * 30) + i) & 1) continue;   // some aos poucos, piscando
        gfx_disco((int)x + 1, (int)y + 2, r, C_CINZA);
        gfx_disco((int)x, (int)y, r, q < 0.45f ? RGB(204, 194, 172) : RGB(166, 154, 132));
        if (r > 4) gfx_disco((int)x - r / 3, (int)y - r / 3, r / 3, q < 0.45f ? RGB(238, 230, 212) : RGB(204, 194, 172));
    }
    // A bala: um risco claro indo da boca do cano ate a fila de la, e a faisca do ricochete.
    float qb = t / 0.14f;
    if (qb < 1) {
        int y = (int)(by + (tiro_ate - by) * qb), y0 = y - tiro_dir * 22;
        gfx_linha_grossa(bx, y0, bx, y, 2, C_AMARELO);
        gfx_linha_grossa(bx, y - tiro_dir * 6, bx, y, 3, C_BRANCO);
    } else if (t < 0.40f) {
        float q = (t - 0.14f) / 0.26f;
        int y = tiro_ate - tiro_dir * (R_DADO + 4), n = (int)(5 + q * 10);
        for (int i = 0; i < 7; i++) {
            float a = -1.5708f * tiro_dir + (i - 3) * 0.42f;
            gfx_linha(bx + (int)(cosf(a) * n * 0.45f), y - (int)(sinf(a) * n * 0.45f),
                      bx + (int)(cosf(a) * n), y - (int)(sinf(a) * n), q < 0.5f ? C_BRANCO : C_AMARELO);
        }
        if (q < 0.6f) gfx_disco(bx, y, (int)(3 * (1 - q)) + 1, C_BRANCO);
    }
}

// O fogo da boca do cano: uma estrela de raios, logo no estampido - por cima
// de tudo, ate do numero que sobe do dado (a fumaca e a bala ficam embaixo).
static void desenha_tiro_fogo(void)
{
    float t = t_tiro;
    int bx = tiro_x, by = tiro_y + tiro_dir * (R_DADO + 2);
    if (t >= 0 && t < 0.2f) {
        float q = t / 0.2f;
        int R = (int)(14 + q * 12);
        for (int i = 0; i < 9; i++) {
            float a = 1.5708f * tiro_dir + (i - 4) * 0.36f, len = R * (i % 2 ? 0.55f : 1.0f) * (i == 4 ? 1.5f : 1.0f);
            gfx_linha_grossa(bx, by, bx + (int)(cosf(a) * len), by + (int)(sinf(a) * len), i == 4 ? 4 : 3,
                             q < 0.35f ? C_BRANCO : (i % 2 ? C_FOGO : C_AMARELO));
        }
        gfx_disco(bx, by, (int)(8 * (1 - q)) + 2, q < 0.5f ? C_BRANCO : C_AMARELO);
    }
}

// ---------------------------------------------------------------------------
// O ciclo cosmico. O dado que vai evoluir, ao pousar: a luz se junta nele
// girando, um clarao, um anel e raios abrem e as faiscas sobem (o som, um
// pouco mais agudo a cada estagio); ate o fim da rodada, uma estrelinha gira
// em volta dele. O Universo, ao pousar, faz o Big Bang: a mesa escurece e as
// estrelas sao sugadas para ele, o clarao branco toma a tela, a mesa treme,
// e aneis e estrelas se espalham. Na rodada seguinte, o dado que evoluiu
// chega na mao com o brilho e o nome novo.
// ---------------------------------------------------------------------------
#define EVOLUI_T      1.2f
#define EVOLUI_CLARAO 0.30f                  // quando a luz junta e estoura
#define BANG_T        2.8f
#define BANG_ESTOURO  0.38f                  // o estouro, junto com o do som
static float t_evolui = -1, t_bang = -1;
static int evo_x, evo_y, evo_tipo, bang_j, bang_k;
static int cosmo_tom(int t) { static const int8_t TOM[5] = { 0, 2, 4, 5, 7 }; return cosmico(t) ? TOM[t - D_ASTRO] : 0; }

// Quando o dado i da mao que evoluiu se revela: depois de chegar, um de
// cada vez (o nome de um nao cobre o do outro).
#define CHEGOU_T 1.5f
static float cosmo_chega(const jogador_t *p, int i)
{
    int antes = 0;
    for (int q = 0; q < i; q++) antes += cosmo_recem[p - J][p->mao[q]];
    return i * 0.07f + 0.18f + antes * CHEGOU_T;
}

static void cosmo_pousa(int j, int k)
{
    int t = F[j].tipo[k];
    if (!cosmico(t) || !F[j].evolui[k] || F[j].idx[k] < 0) return;   // a copia do Coringa nao evolui
    evo_x = SLOT_X(k);
    evo_y = zona(j);
    evo_tipo = t;
    if (t == D_UNIVERSO) {
        t_bang = 0;
        bang_j = j; bang_k = k;
        som_toca(SOM_BIG_BANG);
        return;
    }
    t_evolui = 0;
    som_toca_tom(SOM_EVOLUI, cosmo_tom(t));
}

// O veredito espera a luz estourar (e o Big Bang espalhar) antes de seguir.
static bool cosmo_segura(void)
{
    return (t_evolui >= 0 && t_evolui < EVOLUI_CLARAO + 0.35f) || (t_bang >= 0 && t_bang < BANG_ESTOURO + 1.3f);
}

static void cosmo_passo(float dt)
{
    if (t_evolui >= 0 && (t_evolui += dt) > EVOLUI_T) t_evolui = -1;
    if (t_bang >= 0) {
        float antes = t_bang;
        t_bang += dt;
        if (antes < BANG_ESTOURO && t_bang >= BANG_ESTOURO) {   // o estouro: a mesa para no clarao e treme
            golpe(bang_j, 1u << bang_k | 1u << N_FILA);
            treme(7, 0.8f);
        }
        if (t_bang > BANG_T) t_bang = -1;
    }
    // O dado que evoluiu chegando na mao: o som do estagio novo.
    if (fase == F_ORDEM && !anuncia_maquina() && t_anuncio <= 0) {
        const jogador_t *p = &J[vez];
        for (int i = 0; i < p->n_mao; i++) {
            int c = p->mao[i];
            float chega = cosmo_chega(p, i);
            if (cosmo_recem[vez][c] && t_fase - dt < chega && t_fase >= chega) som_toca_tom(SOM_EVOLUI, cosmo_tom(p->col[c].tipo));
        }
    }
}

// Um anel de um ou dois pixels de espessura.
static void anel(int cx, int cy, int r, int esp, uint16_t c)
{
    int n = r * 6 + 12;
    for (int i = 0; i < n; i++) {
        float a = i * 6.2832f / n;
        gfx_rect(cx + (int)lroundf(cosf(a) * r), cy + (int)lroundf(sinf(a) * r), esp, esp, c);
    }
}

static void desenha_evolui(void)
{
    if (t_evolui < 0) return;
    int R = R_DADO, cx = evo_x, cy = evo_y;
    uint16_t luz = VISUAL[evo_tipo].c1, cor = aro_de(evo_tipo);
    if (t_evolui < EVOLUI_CLARAO) {               // a luz se junta no dado, girando
        float q = t_evolui / EVOLUI_CLARAO, e = q * q;
        for (int i = 0; i < 10; i++) {
            float a = i * 0.6283f + q * 2.5f, d = (1 - e) * (R + 30) + R * 0.5f;
            estrela4(cx + (int)(cosf(a) * d), cy + (int)(sinf(a) * d), i % 3 ? 1 : 2, C_BRANCO, luz, cor);
        }
        return;
    }
    float s = t_evolui - EVOLUI_CLARAO, q = s / (EVOLUI_T - EVOLUI_CLARAO);
    if (s < 0.06f) gfx_disco(cx, cy, R + 4, C_BRANCO);                       // o clarao, por um instante
    if (q < 0.7f) anel(cx, cy, R + 3 + (int)(q * 36), q < 0.3f ? 2 : 1, q < 0.3f ? luz : cor);
    if (q < 0.35f)                                                          // oito raios curtos
        for (int i = 0; i < 8; i++) {
            float a = i * 0.7854f + 0.39f, d0 = R + 4 + q * 24, d1 = d0 + 8 + q * 16;
            gfx_linha(cx + (int)(cosf(a) * d0), cy + (int)(sinf(a) * d0), cx + (int)(cosf(a) * d1), cy + (int)(sinf(a) * d1),
                      i % 2 ? cor : luz);
        }
    for (int i = 0; i < 9; i++) {                                           // as faiscas subindo
        uint32_t h = hash2(i, 33);
        float x = cx + ((int)(h % (2 * R)) - R) * 0.9f + sinf(s * 6 + i) * 3;
        float y = cy + R * 0.3f - s * (38 + (h >> 8) % 30);
        if (q > 0.7f && ((int)(t_evolui * 20) + i) & 1) continue;
        estrela4((int)x, (int)y, i % 3 ? 1 : 2, C_BRANCO, luz, cor);
    }
}

// A mira de um poder armado sobre o dado que ele vai pegar no fim: um anel
// vermelho com quatro tracos cruzando, pulsando de leve.
static void mira_sobre(int x, int y, int R)
{
    int r = R + 5 + (int)lroundf(sinf(t_relogio * 5.5f));
    anel(x, y, r, 1, C_VINHO_CLR);
    for (int i = 0; i < 4; i++) {
        int dx = i == 0 ? 1 : i == 1 ? -1 : 0, dy = i == 2 ? 1 : i == 3 ? -1 : 0;
        gfx_linha_grossa(x + dx * (r - 3), y + dy * (r - 3), x + dx * (r + 4), y + dy * (r + 4), 2, C_VINHO_CLR);
    }
}

// A estrelinha que gira em volta do dado que vai evoluir, ate o fim da rodada.
static void cosmo_orbita(int x, int y, int R, int t)
{
    uint16_t luz = VISUAL[t].c1, cor = aro_de(t);
    for (int i = 0; i < 2; i++) {
        float a = t_relogio * 2.6f + i * 3.1416f, d = R + 5;
        estrela4(x + (int)(cosf(a) * d), y + (int)(sinf(a) * d * 0.9f), i ? 1 : 2, C_BRANCO, luz, cor);
    }
}

static void desenha_big_bang(void)
{
    if (t_bang < 0) return;
    int cx = evo_x, cy = evo_y;
    if (t_bang < BANG_ESTOURO) {                  // tudo sendo sugado para o dado
        float q = t_bang / BANG_ESTOURO;
        gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_BARRA, (int)(210 * q));
        for (int i = 0; i < 28; i++) {
            uint32_t h = hash2(i, 77);
            float a = (h % 628) / 100.0f, d0 = 240 + (h >> 10) % 160;
            float d = d0 * (1 - q) * (1 - q) + 6, d2 = d + 8 + 34 * q;
            gfx_linha(cx + (int)(cosf(a) * d), cy + (int)(sinf(a) * d), cx + (int)(cosf(a) * d2), cy + (int)(sinf(a) * d2),
                      i % 3 ? C_BRANCO : RGB(206, 180, 218));
        }
        gfx_disco(cx, cy, (int)(4 + 7 * q), C_BRANCO);
        return;
    }
    float s = t_bang - BANG_ESTOURO, q = s / (BANG_T - BANG_ESTOURO);
    for (int k = 0; k < 3; k++) {                 // os aneis
        float rr = (s - k * 0.12f) * (440 + k * 70);
        if (rr <= 2 || rr > 760) continue;
        static const uint16_t AN[3] = { RGB(252, 248, 236), RGB(206, 180, 218), RGB(246, 222, 132) };
        anel(cx, cy, (int)rr, q < 0.35f ? 3 : q < 0.6f ? 2 : 1, AN[k]);
    }
    for (int i = 0; i < 56; i++) {                // as estrelas voando para fora, com rastro
        uint32_t h = hash2(i, 191);
        float a = (h % 6283) / 1000.0f, v = 110 + (h >> 12) % 280;
        float d = 10 + v * s * (1 - 0.2f * s), d2 = d - 5 - v * 0.05f;
        int x = cx + (int)(cosf(a) * d), y = cy + (int)(sinf(a) * d * 0.85f);
        if (x < -10 || x > GFX_W + 10 || y < -10 || y > GFX_H + 10) continue;
        if (q > 0.65f && ((int)(t_bang * 16) + i) & 1) continue;             // somem piscando
        static const uint16_t EST[4] = { RGB(252, 248, 236), RGB(246, 222, 132), RGB(206, 180, 218), RGB(168, 204, 230) };
        uint16_t c = EST[(h >> 5) & 3];
        gfx_linha(cx + (int)(cosf(a) * d2), cy + (int)(sinf(a) * d2 * 0.85f), x, y, gfx_mistura(c, C_BARRA, 45));
        if ((h >> 9) % 3 == 0) estrela4(x, y, 2, C_BRANCO, c, gfx_mistura(c, C_BARRA, 40));
        else gfx_rect(x - 1, y - 1, 2, 2, c);
    }
    if (s < 0.5f) gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_BRANCO, (int)(255 * (1 - s / 0.5f) * (1 - s / 0.5f)));   // o clarao
}

// O dado que evoluiu chegando na mao (s: desde que se revelou): um anel e
// faiscas em volta e, embaixo, o nome novo.
static void cosmo_chegou(int x, int y, int t, float s)
{
    if (s < 0 || s > CHEGOU_T) return;
    uint16_t luz = VISUAL[t].c1, cor = aro_de(t);
    float q = s / CHEGOU_T;
    if (s < 0.06f) gfx_disco(x, y, 22, C_BRANCO);
    if (s < 0.5f) anel(x, y, 20 + (int)(s * 40), s < 0.2f ? 2 : 1, s < 0.2f ? luz : cor);
    for (int i = 0; i < 6; i++) {
        float a = t_relogio * 1.8f + i * 1.0472f, d = 24 + 3 * sinf(s * 5 + i);
        if (q > 0.8f && ((int)(s * 16) + i) & 1) continue;
        estrela4(x + (int)(cosf(a) * d), y + (int)(sinf(a) * d), i % 2 ? 1 : 2, C_BRANCO, luz, cor);
    }
    if (q > 0.85f && (int)(s * 12) & 1) return;                        // o nome some piscando
    char nome[32];
    snprintf(nome, sizeof nome, "%s!", TIPO[t].nome);
    int w = gfx_largura(nome, 1), sobe = s < 0.3f ? (int)((0.3f - s) * 20) : 0;
    int ty = y + 29 + sobe;
    gfx_rect(x - w / 2 - 4, ty - 2, w + 8, TXT_H + 3, C_BARRA);
    gfx_rect(x - w / 2 - 4, ty + TXT_H, w + 8, 1, cor);
    txt_c(x, ty, nome, cor_dado(t), true);
}

// O Buraco Negro engolindo um vizinho, no fim da rodada: a poeira do dado
// vai rodando para dentro dele.
static void desenha_buraco_suga(void)
{
    if (fase != F_EFEITOS || ev_i >= n_ev || t_respiro >= 0) return;
    const evento_t *e = &ev[ev_i];
    if (e->tipo != EV_ANULA || e->de_k >= N_FILA || F[e->de_j].tipo[e->de_k] != D_BURACO) return;
    float p = t_fase / duracao_evento(ev_i);
    if (p > 0.75f) return;
    int bx = SLOT_X(e->de_k), by = zona(e->de_j), ax = SLOT_X(e->alvo_k), ay = zona(e->alvo_j);
    for (int i = 0; i < 26; i++) {
        float f = fr(p * 1.6f + i / 26.0f);                   // cada grao: do alvo ao centro do buraco
        float a = i * 2.4f + f * 9.0f, raio = (1 - f) * R_DADO * 0.9f;   // girando cada vez mais junto
        int x = (int)(ax + (bx - ax) * f * f + cosf(a) * raio), y = (int)(ay + (by - ay) * f * f + sinf(a) * raio * 0.8f);
        uint16_t c = f > 0.75f ? C_FOGO : i % 3 ? RGB(238, 162, 74) : C_AMARELO;
        gfx_rect(x - 1, y - 1, 3, 3, gfx_mistura(c, C_BARRA, 35));
        gfx_rect(x, y, 2, 2, c);
    }
    anel(bx, by, R_DADO + 3 + (int)(3 * sinf(p * 30)), 1, C_FOGO);
}

// No comeco de cada evento: o oponente sente o golpe (ou gosta do que fez).
static void reage_evento(const evento_t *e)
{
    if (e->tipo == EV_JACKPOT && e->de_j == 0) { rival_reage(RM_COMBO); hist_jackpot(); }
    else if ((e->tipo == EV_ANULA || e->tipo == EV_ROUBA || e->tipo == EV_TIRA || e->tipo == EV_PIRATA || e->tipo == EV_TREMOR)
             && e->de_j == 1 && e->alvo_j == 0) rival_reage(RM_ATACOU);
    if (e->tipo == EV_TREMOR) treme(3, 0.95f);   // o chao ronca enquanto a falha corre
}

// O meio dos eventos que tem tempo proprio, entre t0 e t1: o dado que o
// Terremoto sacudiu bate no feltro (poeira, baque, um tremor curto); a
// Ficha Coringa cai deitada.
static void evento_no_meio(float t0, float t1)
{
    if (fase != F_EFEITOS || ev_i >= n_ev || t_respiro >= 0) return;
    const evento_t *e = &ev[ev_i];
    float d = duracao_evento(ev_i), a = t0 / d, b = t1 / d;
    if (e->tipo == EV_TREMOR && e->alvo_k < N_FILA && a < TREMOR_POUSA && b >= TREMOR_POUSA) {
        som_toca(SOM_BATE);
        poeira_solta((float)SLOT_X(e->alvo_k), (float)zona(e->alvo_j));
        treme(2, 0.25f);
    }
    if (e->tipo == EV_TREMOR && a < TREMOR_SALTA && b >= TREMOR_SALTA) som_toca_tom(SOM_BATE_DADO, -5);   // o dado salta do chao
    if (e->tipo == EV_CORINGA && e->alvo_k < N_FILA && a < CORINGA_POUSA && b >= CORINGA_POUSA) {
        som_toca(SOM_MOEDA_POUSA);
        poeira_solta((float)SLOT_X(e->de_k), (float)zona(e->de_j));
    }
}

// ---------------------------------------------------------------------------
// Numeros que contam: o placar de cada fila, o pote, as fichas das faixas e
// as moedas do Desafiante sobem
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
static int fichas_vis(int j);           // telas.inc: no resultado, as fichas chegando com o pote
static void placar_passo(float dt, float dt_real)
{
    if (rodada == 0) {                       // a partida montando: as fichas ja nascem certas
        contador_fixa(&pote_c, pote);
        for (int j = 0; j < 2; j++) contador_fixa(&faixa_c[j], J[j].fichas);
    }
    contador_passo(&pote_c, pote, dt_real, dt_real, 10, 30);
    for (int j = 0; j < 2; j++) contador_passo(&faixa_c[j], fichas_vis(j), dt_real, dt_real, 10, 30);
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
