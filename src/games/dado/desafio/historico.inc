// ---------------------------------------------------------------------------
// Historico: o que cada perfil ja jogou. Conta so a pessoa contra a maquina
// (Desafiante e partida rapida), no perfil escolhido por ultimo, e guarda na
// chave do perfil ("HIS2" + hexadecimal). A estrutura tem espaco para os
// dados que vierem e uma sobra no fim: crescer nao apaga o que ja foi contado.
// ---------------------------------------------------------------------------
#define HIST_DADOS 128                   // par: o rabo da estrutura fica no mesmo alinhamento
#define HIST_COL   48                 // dados guardados da run campea (camp_n conta todos)
#define HIST_CAMPOS(N, NIV)                                                                \
    char     marca[4];                   /* "HIS2" */                                      \
    uint32_t rodadas, rodadas_venc;                                                        \
    uint16_t partidas, partidas_venc;                                                      \
    uint16_t runs_venc[NIV + 1];         /* vezes que o Barao caiu em cada dificuldade */  \
    uint16_t maior_rodada, maior_pote;                                                     \
    uint16_t jackpots, filas_cheias;                                                       \
    uint16_t horas, horas_venc;          /* horas da verdade suas, e as que deram a rodada */ \
    uint16_t lancou[N], valeu[N], ganhou[N];                                               \
    /* A run campea: a da dificuldade mais alta (a mais recente, se empatar). */           \
    uint8_t  camp_tem, camp_dif, camp_n, camp_amul_x;   /* amul_x: os amuletos 32+ */       \
    uint8_t  camp_col[HIST_COL];                                                           \
    uint32_t camp_amul, camp_data;       /* amuletos (bit a) e o dia (AAAAMMDD) */         \
    int32_t  camp_moedas;                                                                  \
    uint8_t  reserva[32];
typedef struct { HIST_CAMPOS(HIST_DADOS, N_NIVEIS) } hist_t;
typedef struct { HIST_CAMPOS(96, N_NIVEIS) } hist96_t;           // "HIS2" de antes dos 102 tipos: lido e convertido
typedef struct { HIST_CAMPOS(96, N_NIVEIS_V) } hist1_t;          // "HIS1", dos 5 niveis: idem
typedef struct { HIST_CAMPOS(64, N_NIVEIS_V) } hist64_t;         // o de antes dos 69 tipos: idem
_Static_assert(D_N <= HIST_DADOS && MAX_COL <= 255 && A_N <= 40, "o historico nao comporta os dados");
_Static_assert(sizeof(hist_t) != sizeof(hist1_t) && sizeof(hist_t) != sizeof(hist64_t)
               && sizeof(hist1_t) != sizeof(hist64_t) && sizeof(hist96_t) != sizeof(hist_t)
               && sizeof(hist96_t) != sizeof(hist1_t) && sizeof(hist96_t) != sizeof(hist64_t),
               "os formatos do historico se distinguem pelo tamanho");
_Static_assert(sizeof(hist_t) - offsetof(hist_t, camp_tem) == sizeof(hist96_t) - offsetof(hist96_t, camp_tem),
               "o rabo do historico fica igual quando as listas crescem");

static hist_t hist;
static int hist_de = -1;                 // de que perfil e o historico carregado

static const char *chave_hist(int i)
{
    static const char *const K[N_PERFIS] = { "hist1", "hist2", "hist3" };
    return K[i >= 0 && i < N_PERFIS ? i : 0];
}

// O "HIS1" dos 5 niveis: tudo igual, menos as vitorias por nivel (o n de la
// e o 2n daqui) e o nivel da run campea.
static void hist_converte(hist_t *h, const hist1_t *v)
{
    memset(h, 0, sizeof *h);
    memcpy(h->marca, "HIS2", 4);
    h->rodadas = v->rodadas; h->rodadas_venc = v->rodadas_venc;
    h->partidas = v->partidas; h->partidas_venc = v->partidas_venc;
    for (int d = 0; d <= N_NIVEIS_V; d++) h->runs_venc[2 * d] = v->runs_venc[d];
    h->maior_rodada = v->maior_rodada; h->maior_pote = v->maior_pote;
    h->jackpots = v->jackpots; h->filas_cheias = v->filas_cheias;
    h->horas = v->horas; h->horas_venc = v->horas_venc;
    memcpy(h->lancou, v->lancou, sizeof v->lancou);
    memcpy(h->valeu, v->valeu, sizeof v->valeu);
    memcpy(h->ganhou, v->ganhou, sizeof v->ganhou);
    h->camp_tem = v->camp_tem; h->camp_dif = (uint8_t)(2 * v->camp_dif); h->camp_n = v->camp_n;
    memcpy(h->camp_col, v->camp_col, sizeof h->camp_col);
    h->camp_amul = v->camp_amul; h->camp_data = v->camp_data; h->camp_moedas = v->camp_moedas;
    memcpy(h->reserva, v->reserva, sizeof h->reserva);
}

static void hist_carrega(int i)
{
    if (hist_de == i) return;
    hist_de = i;
    memset(&hist, 0, sizeof hist);
    memcpy(hist.marca, "HIS2", 4);
    static char buf[2 * sizeof(hist_t) + 8];
    static hist64_t v64;
    static hist96_t v96;
    static hist1_t v1;
    hist_t h;
    int n = salva_le(chave_hist(i), buf, (int)sizeof buf);
    bool velho = true;
    if (n == (int)(2 * sizeof v96)) {                  // de antes dos 102 tipos: as tres listas cresceram
        if (!de_hex(buf, &v96, sizeof v96)) return;
        velho = false;
        memset(&h, 0, sizeof h);
        memcpy(&h, &v96, offsetof(hist96_t, lancou));
        memcpy(h.lancou, v96.lancou, sizeof v96.lancou);
        memcpy(h.valeu, v96.valeu, sizeof v96.valeu);
        memcpy(h.ganhou, v96.ganhou, sizeof v96.ganhou);
        memcpy(&h.camp_tem, &v96.camp_tem, sizeof v96 - offsetof(hist96_t, camp_tem));
    } else if (n == (int)(2 * sizeof v64)) {                  // de antes dos 69 tipos: as tres listas cresceram
        if (!de_hex(buf, &v64, sizeof v64)) return;
        memset(&v1, 0, sizeof v1);
        memcpy(&v1, &v64, offsetof(hist64_t, lancou));
        memcpy(v1.lancou, v64.lancou, sizeof v64.lancou);
        memcpy(v1.valeu, v64.valeu, sizeof v64.valeu);
        memcpy(v1.ganhou, v64.ganhou, sizeof v64.ganhou);
        memcpy(&v1.camp_tem, &v64.camp_tem, sizeof v64 - offsetof(hist64_t, camp_tem));
    } else if (n == (int)(2 * sizeof v1)) {            // dos 5 niveis
        if (!de_hex(buf, &v1, sizeof v1)) return;
    } else {
        velho = false;
        if (n != (int)(2 * sizeof h) || !de_hex(buf, &h, sizeof h)) return;
    }
    if (velho) {
        if (memcmp(v1.marca, "HIS1", 4)) return;
        if (v1.camp_dif > N_NIVEIS_V) v1.camp_tem = 0;
        hist_converte(&h, &v1);
    }
    if (memcmp(h.marca, "HIS2", 4)) return;
    if (h.camp_n > MAX_COL || h.camp_dif > N_NIVEIS) h.camp_tem = 0;
    for (int k = 0; h.camp_tem && k < h.camp_n && k < HIST_COL; k++) if (h.camp_col[k] >= D_N) h.camp_tem = 0;
    hist = h;
}

static void hist_grava(void)
{
    if (hist_de < 0) return;
    static char buf[2 * sizeof(hist_t) + 1];
    hex_de(&hist, sizeof hist, buf);
    salva_grava(chave_hist(hist_de), buf);
}

// O perfil apagado leva o historico junto.
static void hist_apaga(int i)
{
    salva_apaga(chave_hist(i));
    if (hist_de == i) hist_de = -1;
}

// Conta? So a pessoa (embaixo) contra a maquina, com um perfil escolhido.
static bool hist_conta(void)
{
    if ((modo != M_DESAFIO && modo != M_UM) || cpu != 2) return false;
    perfis_le();
    if (perfil_atual < 0 || !perfis[perfil_atual].nome[0]) return false;
    hist_carrega(perfil_atual);
    return true;
}

static void soma16(uint16_t *v, int n) { *v = (uint16_t)(*v + n > 65535 ? 65535 : *v + n); }
static void maior16(uint16_t *v, int n) { if (n > *v) *v = (uint16_t)(n > 65535 ? 65535 : n); }

static uint32_t hoje(void)
{
    time_t t = time(NULL);
    const struct tm *m = localtime(&t);
    // 0: sem data (um relogio que nao sabe o dia fica antes de 2020)
    return m && m->tm_year >= 120 ? (uint32_t)((m->tm_year + 1900) * 10000 + (m->tm_mon + 1) * 100 + m->tm_mday) : 0;
}

// O seu dado pousou: a fila cheia valendo; a hora da verdade (1 caiu, 2 deu
// a rodada).
static void hist_lance(int j, bool cheia, int hora_res)
{
    if (j != 0 || !hist_conta()) return;
    if (cheia) soma16(&hist.filas_cheias, 1);
    if (hora_res) soma16(&hist.horas, 1);
    if (hora_res == 2) soma16(&hist.horas_venc, 1);
}

static void hist_jackpot(void)
{
    if (hist_conta()) soma16(&hist.jackpots, 1);
}

// A rodada acabou (venc_mao ja decidido); pote_seu: o pote que voce levou.
static void hist_rodada(int pote_seu)
{
    if (!hist_conta()) return;
    if (hist.rodadas < 0xFFFFFFFFu) hist.rodadas++;
    if (venc_mao == 0 && hist.rodadas_venc < 0xFFFFFFFFu) hist.rodadas_venc++;
    maior16(&hist.maior_rodada, F[0].total);
    maior16(&hist.maior_pote, pote_seu);
    for (int k = 0; k < F[0].lancados; k++) {
        int t = F[0].tipo[k];
        soma16(&hist.lancou[t], 1);
        if (F[0].est[k] == V_VALIDO) soma16(&hist.valeu[t], 1);
        if (venc_mao == 0) soma16(&hist.ganhou[t], 1);
    }
    hist_grava();
}

static void hist_partida(int vencedor)
{
    if (!hist_conta()) return;
    soma16(&hist.partidas, 1);
    if (vencedor == 0) soma16(&hist.partidas_venc, 1);
    hist_grava();
}

// O Barao caiu: a run conta, e vira a campea se for a mais alta.
static void hist_run_venceu(void)
{
    if (perfil_atual < 0 || !perfis[perfil_atual].nome[0]) return;
    hist_carrega(perfil_atual);
    int d = run.dificuldade;
    soma16(&hist.runs_venc[d], 1);
    if (!hist.camp_tem || d >= hist.camp_dif) {
        hist.camp_tem = 1;
        hist.camp_dif = (uint8_t)d;
        hist.camp_n = run.n_col;              // a bolsa nao tem limite: guarda os 48 primeiros
        for (int k = 0; k < run.n_col && k < HIST_COL; k++) hist.camp_col[k] = run.col[k].tipo;
        hist.camp_amul = (uint32_t)run_amuletos();
        hist.camp_amul_x = (uint8_t)(run_amuletos() >> 32);
        hist.camp_moedas = run.moedas;
        hist.camp_data = hoje();
    }
    hist_grava();
}

// ---- A tela ---------------------------------------------------------------
static int hist_pag, hist_rola;              // pagina (0 resumo, 1 dados) e a rolagem da lista
#define HIST_LINHAS 10                       // linhas por coluna na pagina dos dados
#define HIST_Y0 58                           // os paineis: do alto do feltro ate perto do rodape
#define HIST_H  244

static void hist_abre(int i)
{
    hist_carrega(i);
    hist_pag = hist_rola = 0;
    t_fase = 0;
    fase = F_HIST;
}

// Os dados que ja foram lancados, do mais usado ao menos.
static int hist_ordem(int *ord)
{
    int n = 0;
    for (int t = 0; t < D_N; t++) if (hist.lancou[t]) ord[n++] = t;
    for (int a = 1; a < n; a++)
        for (int b = a; b > 0 && hist.lancou[ord[b]] > hist.lancou[ord[b - 1]]; b--) {
            int x = ord[b]; ord[b] = ord[b - 1]; ord[b - 1] = x;
        }
    return n;
}

static int pct(uint32_t a, uint32_t b) { return b ? (int)((a * 100u + b / 2) / b) : 0; }

// Uma linha do resumo: o rotulo na fina, a esquerda; o valor a direita.
static void hist_linha(int x, int y, int w, const char *rot, const char *val, uint16_t cv)
{
    txt(x, y + 2, rot, C_TEXTO_M, false);
    gfx_texto(x + w - gfx_largura(val, 1), y, val, cv, 1, true);
    gfx_rect(x, y + TXT_H + 2, w, 1, gfx_mistura(C_FELTRO2, C_BARRA, 35));
}

static void hist_resumo(void)
{
    const perfil_t *p = &perfis[hist_de];
    char s[64];
    int x = 34, w = 262, y = HIST_Y0 + 8, dy = 25;
    ui_painel(x - 12, HIST_Y0, w + 24, HIST_H);
    snprintf(s, sizeof s, "%d", p->runs);
    hist_linha(x, y, w, "Runs no Desafiante", s, C_MARFIM); y += dy;
    int barao = 0;
    for (int d = 0; d <= N_NIVEIS; d++) barao += hist.runs_venc[d];
    snprintf(s, sizeof s, "%d", barao);
    hist_linha(x, y, w, "Runs vencidas", s, barao ? C_OURO : C_MARFIM); y += dy;
    snprintf(s, sizeof s, "%d  ·  %d%%", hist.partidas, pct(hist.partidas_venc, hist.partidas));
    hist_linha(x, y, w, "Partidas  ·  vencidas", s, C_MARFIM); y += dy;
    snprintf(s, sizeof s, "%u  ·  %d%%", (unsigned)hist.rodadas, pct(hist.rodadas_venc, hist.rodadas));
    hist_linha(x, y, w, "Rodadas  ·  vencidas", s, C_MARFIM); y += dy;
    snprintf(s, sizeof s, "%d de %d", hist.horas_venc, hist.horas);
    hist_linha(x, y, w, "Horas da verdade vencidas", s, C_MARFIM); y += dy;
    snprintf(s, sizeof s, "%d pontos", hist.maior_rodada);
    hist_linha(x, y, w, "Maior rodada", s, C_OURO); y += dy;
    snprintf(s, sizeof s, "%d fichas", hist.maior_pote);
    hist_linha(x, y, w, "Maior pote levado", s, C_OURO); y += dy;
    snprintf(s, sizeof s, "%d  ·  %d", hist.filas_cheias, hist.jackpots);
    hist_linha(x, y, w, "Filas cheias  ·  Jackpots", s, C_MARFIM); y += dy;
    // O dado mais usado, desenhado.
    int ord[HIST_DADOS], n = hist_ordem(ord);
    txt(x, y + 6, "Dado mais usado", C_TEXTO_M, false);
    if (n) {
        int t = ord[0];
        snprintf(s, sizeof s, "%s  ·  %d", TIPO[t].nome, hist.lancou[t]);
        int sw = gfx_largura(s, 1);
        gfx_texto(x + w - sw, y + 4, s, cor_dado(t), 1, true);
        desenha_dado((float)(x + w - sw - 16), (float)(y + 13), 10, t, 0, 0, TIPO[t].lados, 0, false);
    } else gfx_texto(x + w - gfx_largura("-", 1), y + 4, "-", C_TEXTO_M, 1, true);

    // A run campea, a direita: a bolsa que venceu e os amuletos dela.
    int px = 330, pw = 288;
    ui_painel(px, y = HIST_Y0, pw, HIST_H);
    txt_c(px + pw / 2, y + 8, "Run campeã", C_LATAO, true);
    if (!hist.camp_tem) {
        txt_c(px + pw / 2, y + 100, "Vença uma run: a run campeã", C_TEXTO_M, false);
        txt_c(px + pw / 2, y + 116, "fica guardada aqui.", C_TEXTO_M, false);
        return;
    }
    int dt = (int)hist.camp_data;
    if (dt) snprintf(s, sizeof s, "%s  ·  %02d/%02d/%04d  ·  %d moedas", NOME_DIF_CURTO[hist.camp_dif],
                     dt % 100, dt / 100 % 100, dt / 10000, hist.camp_moedas);
    else snprintf(s, sizeof s, "%s  ·  %d moedas", NOME_DIF_CURTO[hist.camp_dif], hist.camp_moedas);
    txt_c(px + pw / 2, y + 30, s, hist.camp_dif ? COR_DIF[hist.camp_dif] : C_MARFIM_S, false);
    // Os dados guardados (ate 48) numa grade que encolhe; os que passam disso viram "+N".
    int nc = hist.camp_n < HIST_COL ? hist.camp_n : HIST_COL, mais = hist.camp_n - nc;
    int alt = mais ? 134 : 150;
    grade_t g = grade_cabe(nc, 9, 32, alt);
    int r = (int)(11 * g.f), linhas = (nc + g.por - 1) / g.por;
    float passo = 30 * g.f, passo_y = 32 * g.f;
    int topo = y + 56 + (int)((alt - linhas * passo_y) / 2 + passo_y / 2);
    for (int k = 0; k < nc; k++) {
        int lin = k / g.por, col = k % g.por;
        int na = nc - lin * g.por < g.por ? nc - lin * g.por : g.por;
        int cx = px + pw / 2 + (int)((col - (na - 1) / 2.0f) * passo), cy = topo + (int)(lin * passo_y);
        peca_parada((float)cx, (float)cy, (float)r, (peca_t){ .tipo = hist.camp_col[k], .fixo = 0 }, 0);
        dica_zona(cx - r, cy - r, 2 * r, 2 * r, DICA_DADO, hist.camp_col[k]);
    }
    if (mais) {
        snprintf(s, sizeof s, "+%d dados", mais);
        txt_c(px + pw / 2, topo + (int)((linhas - 1) * passo_y) + r + 6, s, C_TEXTO_M, false);
    }
    amul_t am = (amul_t)hist.camp_amul | (amul_t)hist.camp_amul_x << 32;
    int na = am_conta(am);
    int ax = px + pw / 2 - (na - 1) * 13, ay = y + HIST_H - 26;
    for (int a = 0; a < A_N; a++) {
        if (!(am >> a & 1)) continue;
        desenha_amuleto(ax, ay, 10, a, false);
        dica_zona(ax - 10, ay - 10, 20, 20, DICA_AMULETO, a);
        ax += 26;
    }
}

// Pagina dos dados: duas colunas, do mais usado ao menos; cada linha com o
// dado, quantas vezes foi lancado, quantas valeu e em quantas rodadas que
// voce levou ele estava na fila.
static void hist_dados(void)
{
    int ord[HIST_DADOS], n = hist_ordem(ord);
    ui_painel(22, HIST_Y0, GFX_W - 44, HIST_H);
    if (!n) { txt_c(GFX_W / 2, HIST_Y0 + HIST_H / 2 - 8, "Nenhum dado lançado ainda.", C_TEXTO_M, false); return; }
    if (hist_rola > n - 1) hist_rola = n - 1;
    if (hist_rola < 0) hist_rola = 0;
    for (int c = 0; c < 2; c++) {
        int x = 40 + c * 296, y = HIST_Y0 + 8;
        gfx_texto(x + 150, y, "lances", C_TEXTO_M, GFX_MIUDO, false);
        gfx_texto(x + 192, y, "válido", C_TEXTO_M, GFX_MIUDO, false);
        gfx_texto(x + 232, y, "vence", C_TEXTO_M, GFX_MIUDO, false);
        for (int l = 0; l < HIST_LINHAS; l++) {
            int i = hist_rola + c * HIST_LINHAS + l;
            if (i >= n) break;
            int t = ord[i], ly = y + 16 + l * 20;
            desenha_dado((float)(x + 8), (float)(ly + 8), 8, t, 0, 0, TIPO[t].lados, 0, false);
            dica_zona(x, ly, 140, 18, DICA_DADO, t);
            txt(x + 22, ly + 1, TIPO[t].nome, cor_dado(t), false);
            char s[16];
            snprintf(s, sizeof s, "%d", hist.lancou[t]);
            gfx_texto(x + 180 - gfx_largura(s, 1), ly - 2, s, C_MARFIM, 1, true);
            int v = pct(hist.valeu[t], hist.lancou[t]), g = pct(hist.ganhou[t], hist.lancou[t]);
            snprintf(s, sizeof s, "%d%%", v);
            gfx_texto(x + 222 - gfx_largura(s, 1), ly - 2, s, C_MARFIM_S, 1, true);
            snprintf(s, sizeof s, "%d%%", g);
            gfx_texto(x + 262 - gfx_largura(s, 1), ly - 2, s, g >= 50 ? C_OURO : C_MARFIM_S, 1, true);
        }
    }
    int yl = HIST_Y0 + HIST_H - 18;
    gfx_texto(40, yl, "válido: terminou válido  ·  vence: você levou a rodada", C_TEXTO_M, GFX_MIUDO, false);
    if (n > 2 * HIST_LINHAS) {
        char s[40];
        snprintf(s, sizeof s, "%d-%d de %d", hist_rola + 1, hist_rola + 2 * HIST_LINHAS < n ? hist_rola + 2 * HIST_LINHAS : n, n);
        gfx_texto(GFX_W - 40 - gfx_largura(s, GFX_MIUDO), yl, s, C_TEXTO_M, GFX_MIUDO, false);
    }
}

static void desenha_historico(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    int tx = botao_voltar(KEY_ESC);
    txt(tx, ty, "HISTÓRICO", C_LATAO, true);
    txt(tx + gfx_largura("HISTÓRICO", 1) + 12, ty, perfis[hist_de].nome, C_MARFIM, true);
    static const char *const ABA[2] = { "resumo", "dados" };
    for (int a = 0; a < 2; a++) {                  // as abas, a direita da faixa
        int bw = 74, bx = GFX_W - 16 - (2 - a) * (bw + 6), by = (FAIXA_H - 24) / 2;
        ui_botao(bx, by, bw, 24, ABA[a], a == hist_pag ? C_LATAO : 0, a == hist_pag ? UI_FOCO : 0);
        zona_clique(bx, by, bw, 24, &hist_pag, a, 0);
    }
    if (hist_pag == 0) hist_resumo();
    else hist_dados();
    rodape_run(hist_pag ? "TAB resumo  ·  setas trocam a página  ·  ESC volta"
                        : "TAB os números de cada dado  ·  ESC volta");
}

static bool hist_tecla(int k, int e)
{
    if (k == KEY_TAB) { hist_pag ^= 1; hist_rola = 0; som_toca(SOM_TIQUE); return true; }
    if (hist_pag == 1 && e) {                    // de pagina em pagina
        int ord[HIST_DADOS], n = hist_ordem(ord), pg = 2 * HIST_LINHAS;
        int r = hist_rola + e * pg;
        if (r > (n - 1) / pg * pg) r = (n - 1) / pg * pg;
        if (r < 0) r = 0;
        if (r != hist_rola) { hist_rola = r; som_toca(SOM_TIQUE); }
        return true;
    }
    if (k == KEY_ENTER) { t_fase = 0; fase = F_PERFIS; }
    return true;
}
