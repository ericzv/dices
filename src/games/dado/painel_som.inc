// ===========================================================================
// O painel de opcoes (tecla M): os volumes e as opcoes do jogo
// ===========================================================================
// Tres volumes, de 0 (mudo) a 10, cada um guardado: a musica, os efeitos da
// mesa e a interface (o cursor e os menus). As setas escolhem a linha e
// mudam o volume, que se ouve na hora: a trilha sobe e desce, o efeito toca
// uma ficha, a interface um tique. O mouse clica direto no degrau. N liga e
// desliga so a musica, voltando ao volume de antes.
static bool  som_aberto;
static int   som_cur;                          // a linha escolhida
static float t_som;                            // ha quanto tempo o painel abriu
static int   som_escada_x;                     // onde comeca a escada (o clique acha o degrau)
static int   som_meio_x;                       // o meio do valor da opcao em foco (o clique a esquerda dele volta)
static int   som_musica_antes = SOM_NIVEIS;    // o volume da musica antes do N
#define TECLA_SOM  0x201                       // o clique numa linha do painel (val: a linha)
#define SOM_DEGRAU 13                          // um degrau da escada, com o vao
#define SOM_LIN    21                          // a altura de uma linha
static const char *const SOM_ROTULO[SOM_CANAIS] = { "Música", "Efeitos", "Interface" };
// As linhas do painel, de cima para baixo.
static const int SOM_LINHA[SOM_CANAIS] = { SOM_CANAL_MUSICA, SOM_CANAL_EFEITOS, SOM_CANAL_INTERFACE };
// Depois dos volumes, as opcoes: a velocidade da partida (V), a letra maior
// (L), o tremor da tela (T), a tela cheia e o tamanho da mesa (F11, - e +:
// sao da janela, e so aparecem onde a plataforma as entrega, dado_janela), o
// medidor de quadros (a plataforma escreve no canto quantos quadros por
// segundo o aparelho esta dando: para conferir num tablet) e, no computador,
// o registro das partidas (registro.inc: a versao de teste, que grava as
// partidas contra a maquina numa pasta ao lado do jogo). Ficam aqui para quem
// joga com o controle ou no toque, que nao tem essas teclas: o RT (ou o
// "Menu" da faixa de baixo da mesa) abre o painel e os lados trocam.
static bool reg_disponivel(void);                // registro.inc
static bool reg_e_ligado(void);
static const char *reg_estado_txt(void);
static void alterna_registro(void);
static bool menu_sem_partida(void);              // passo.inc
enum { OP_VELOCIDADE, OP_LETRA, OP_TREMOR, OP_TELA_CHEIA, OP_MESA, OP_MEDIDOR, OP_REGISTRO, OP_N };
static const char *const SOM_OPCAO[OP_N] = { "Velocidade", "Letra", "Tremor da tela", "Tela cheia", "Tamanho da mesa",
                                             "Medidor de quadros", "Registrar partidas" };
static bool medidor_on, medidor_lido;            // o medidor de quadros, guardado ("medidor")
bool dado_medidor(void) { return medidor_on; }
// A janela (jogos.h): a tela cheia e o tamanho da mesa sao da plataforma.
static dado_janela_t janela;
void dado_janela(const dado_janela_t *j) { if (j) janela = *j; else memset(&janela, 0, sizeof janela); }

// As opcoes que valem agora, na ordem do painel. A velocidade nao muda nada
// numa partida online (os dois lados andam juntos): la a linha nao aparece.
static int som_opcoes(int *op)
{
    int n = 0;
    if (modo != M_ONLINE || menu_sem_partida()) op[n++] = OP_VELOCIDADE;
    op[n++] = OP_LETRA;
    op[n++] = OP_TREMOR;
    if (janela.tela_cheia && janela.alterna_tela_cheia) op[n++] = OP_TELA_CHEIA;
    if (janela.mesa && janela.muda_mesa) op[n++] = OP_MESA;
    op[n++] = OP_MEDIDOR;
    if (reg_disponivel()) op[n++] = OP_REGISTRO;   // o registro so onde ha pasta (no site, nao)
    return n;
}

// A velocidade: d > 0 a proxima (normal, media, rapida, e volta), d < 0 a de antes.
static void muda_velocidade(int d)
{
    velocidade = (velocidade + (d < 0 ? VEL_N - 1 : 1)) % VEL_N;
    char g[2] = { VEL_GRAVA[velocidade], 0 };
    salva_grava("vel", g);
    som_toca(SOM_TIQUE);
}

static void alterna_letra(void)
{
    gfx_letra_maior(!gfx_letra_e_maior());
    salva_grava("letra", gfx_letra_e_maior() ? "1" : "0");
    som_toca(SOM_TIQUE);
}

static void alterna_tremor(void)
{
    tremor_off = !tremor_off;
    salva_grava("tremor", tremor_off ? "0" : "1");
    som_toca(SOM_TIQUE);
}

static void alterna_medidor(void)
{
    medidor_on = !medidor_on;
    salva_grava("medidor", medidor_on ? "1" : "0");
    som_toca(SOM_TIQUE);
}

// A opcao guardada da ultima vez (dado_inicia).
static void medidor_le(void)
{
    if (medidor_lido) return;
    char b[8];
    medidor_on = salva_le("medidor", b, sizeof b) == 1 && b[0] == '1';
    medidor_lido = true;
}

// A opcao o trocada pelos lados (d: -1 a esquerda, 1 a direita) ou pelo clique.
static void som_troca_opcao(int o, int d)
{
    switch (o) {
    case OP_VELOCIDADE: muda_velocidade(d); break;
    case OP_LETRA:      alterna_letra(); break;
    case OP_TREMOR:     alterna_tremor(); break;
    case OP_TELA_CHEIA: janela.alterna_tela_cheia(); som_toca(SOM_TIQUE); break;
    case OP_MESA: {
        int antes = janela.mesa();
        janela.muda_mesa(d < 0 ? -5 : 5);
        if (janela.mesa() != antes) som_toca(SOM_TIQUE);
        break;
    }
    case OP_MEDIDOR:    alterna_medidor(); break;
    default:            alterna_registro(); break;
    }
}

// O que a opcao o mostra a direita.
static const char *som_valor(int o)
{
    static char s[16];
    switch (o) {
    case OP_VELOCIDADE: return velocidade == VEL_NORMAL ? "normal" : velocidade == VEL_MEDIA ? "média" : "rápida";
    case OP_LETRA:      return gfx_letra_e_maior() ? "maior" : "normal";
    case OP_TREMOR:     return tremor_off ? "desligado" : "ligado";
    case OP_TELA_CHEIA: return janela.tela_cheia() ? "ligada" : "desligada";
    case OP_MESA:       snprintf(s, sizeof s, "%d%%", janela.mesa()); return s;
    case OP_MEDIDOR:    return medidor_on ? "ligado" : "desligado";
    default:            return reg_estado_txt();
    }
}

static void som_grava(void)
{
    char s[32];
    snprintf(s, sizeof s, "%d,%d,%d", som_volume_de(SOM_CANAL_MUSICA), som_volume_de(SOM_CANAL_EFEITOS),
             som_volume_de(SOM_CANAL_INTERFACE));
    salva_grava("som", s);
}

// Os volumes da ultima vez (dado_inicia). Um "som" com um quarto numero (o
// ambiente dos saloes, que saiu) le os tres primeiros. Estragado: os volumes
// cheios, uma copia a parte e o aviso.
static void som_le_volumes(void)
{
    char b[32];
    int v[SOM_CANAIS], n = salva_le("som", b, sizeof b);
    bool ok = n > 0 && sscanf(b, "%d,%d,%d", &v[0], &v[1], &v[2]) == SOM_CANAIS;
    for (int c = 0; ok && c < SOM_CANAIS; c++) ok = v[c] >= 0 && v[c] <= SOM_NIVEIS;
    if (ok)
        for (int c = 0; c < SOM_CANAIS; c++) som_volume(c, v[c]);
    else if (n > 0) salvo_estragado("som", b, "Os volumes salvos não abriram: uma cópia ficou guardada à parte.");
    if (som_volume_de(SOM_CANAL_MUSICA) > 0) som_musica_antes = som_volume_de(SOM_CANAL_MUSICA);
}

// Muda o volume do canal c para n e deixa ouvir como ficou.
static void som_muda(int c, int n)
{
    int antes = som_volume_de(c);
    som_volume(c, n);
    int agora = som_volume_de(c);
    if (agora == antes) return;
    if (c == SOM_CANAL_MUSICA && agora > 0) som_musica_antes = agora;
    if (c == SOM_CANAL_EFEITOS) som_toca(SOM_FICHA);
    else if (c == SOM_CANAL_INTERFACE) som_toca(SOM_TIQUE);
    som_grava();
}

// N: so a musica, liga e desliga (volta ao volume de antes).
static void som_alterna_musica(void)
{
    som_muda(SOM_CANAL_MUSICA, som_volume_de(SOM_CANAL_MUSICA) ? 0 : som_musica_antes);
}

static void som_abre(void)
{
    som_aberto = true;
    t_som = 0;
    som_toca(SOM_TIQUE);
}

// Com o painel aberto, ele fica com todas as teclas.
static void som_tecla(int k)
{
    int op[OP_N], n = SOM_CANAIS + som_opcoes(op);
    if (som_cur >= n) som_cur = n - 1;
    if (k == KEY_UP || k == 'w') { if (som_cur > 0) { som_cur--; som_toca(SOM_TIQUE); } return; }
    if (k == KEY_DOWN || k == 's') { if (som_cur < n - 1) { som_cur++; som_toca(SOM_TIQUE); } return; }
    if (k == 'n') { som_alterna_musica(); return; }
    if (k == 'm' || k == KEY_ESC || k == KEY_ENTER || k == KEY_TAB || k == KEY_BKSP) { som_aberto = false; return; }
    bool esq = k == KEY_LEFT || k == 'a', dir = k == KEY_RIGHT || k == 'd';
    if (som_cur >= SOM_CANAIS) {                // as opcoes: os lados (ou o clique) trocam
        int o = op[som_cur - SOM_CANAIS];
        // O clique: a esquerda do valor volta (a mesa diminui), o resto avanca.
        if (k == TECLA_SOM) som_troca_opcao(o, (o == OP_MESA || o == OP_VELOCIDADE) && mouse_x < som_meio_x ? -1 : 1);
        else if (esq || dir) som_troca_opcao(o, esq ? -1 : 1);
        return;
    }
    int c = SOM_LINHA[som_cur];
    if (esq) { som_muda(c, som_volume_de(c) - 1); return; }
    if (dir) { som_muda(c, som_volume_de(c) + 1); return; }
    if (k == TECLA_SOM) {                       // o clique na linha: o degrau embaixo do mouse (antes dele, mudo)
        int dx = mouse_x - som_escada_x;
        if (dx >= -SOM_DEGRAU) som_muda(c, dx < 0 ? 0 : dx / SOM_DEGRAU + 1);
    }
}

// O tempo do painel, a trilha abaixando enquanto a cadeia de efeitos corre e
// o som da cena (a mesa do chefe). Devolve se o painel esta aberto: fora do
// online, a partida espera por ele.
static void som_da_cena(void);                   // salao.inc
static bool som_passo(float dt_real)
{
    if (som_aberto) t_som += dt_real;
    som_cadeia(fase == F_EFEITOS);
    som_da_cena();
    return som_aberto;
}

// A placa no meio da tela, por cima de tudo: cada canal numa linha, com o
// nome, a escada de 10 degraus (cada um um pouco mais alto, acesos ate o
// volume) e o numero; depois de um fio, as opcoes, com o valor a direita.
static void desenha_som(void)
{
    if (!som_aberto) return;
    n_zonas = 0;                                 // nada por baixo responde ao clique
    int op[OP_N], n_op = som_opcoes(op);
    if (som_cur >= SOM_CANAIS + n_op) som_cur = SOM_CANAIS + n_op - 1;
    const int w = 340, h = 32 + (SOM_CANAIS + n_op) * SOM_LIN + 6 + (som_falhou() ? 44 : 28), x = GFX_W / 2 - w / 2;
    int y = GFX_H / 2 - h / 2 + ui_entra(t_som, 0, 14);
    ui_escurece(0, 0, GFX_W, GFX_H, 110);
    ui_painel(x, y, w, h);
    txt_c(GFX_W / 2, y + 4, "OPÇÕES", C_LATAO, true);
    som_escada_x = x + 132;
    for (int l = 0; l < SOM_CANAIS; l++) {
        int c = SOM_LINHA[l], ly = y + 34 + l * SOM_LIN, nv = som_volume_de(c);
        bool cur = l == som_cur;
        if (cur) ui_cantos(x + 10, ly - 3, w - 20, SOM_LIN - 2, C_OURO);
        txt(x + 22, ly - 2, SOM_ROTULO[c], cur ? C_MARFIM : C_MARFIM_S, false);
        for (int i = 0; i < SOM_NIVEIS; i++) {
            int dh = 5 + i, dx = som_escada_x + i * SOM_DEGRAU, dy = ly + 15 - dh;
            bool aceso = i < nv;
            gfx_rect(dx - 1, dy - 1, SOM_DEGRAU - 2, dh + 2, C_BARRA);
            gfx_rect(dx, dy, SOM_DEGRAU - 4, dh, aceso ? (cur ? C_OURO : C_LATAO) : C_PLACA_LUZ);
            if (aceso) gfx_rect(dx, dy, SOM_DEGRAU - 4, 1, cur ? C_AMARELO : C_OURO);   // a luz em cima
        }
        char s[8];
        snprintf(s, sizeof s, "%d", nv);
        int nx = som_escada_x + SOM_NIVEIS * SOM_DEGRAU + 6;
        if (nv) txt(nx, ly - 4, s, cur ? C_OURO : C_MARFIM_S, true);
        else txt(nx, ly - 2, "mudo", C_VINHO_CLR, false);
        zona_clique(x + 10, ly - 3, w - 20, SOM_LIN - 1, &som_cur, l, TECLA_SOM);
    }
    // As opcoes, depois de um fio.
    int oy = y + 34 + SOM_CANAIS * SOM_LIN + 6;
    gfx_rect(x + 22, oy - 6, w - 44, 1, C_LATAO_ESC);
    for (int i = 0; i < n_op; i++) {
        int l = SOM_CANAIS + i, ly = oy + i * SOM_LIN;
        bool cur = l == som_cur;
        if (cur) ui_cantos(x + 10, ly - 3, w - 20, SOM_LIN - 2, C_OURO);
        txt(x + 22, ly - 2, SOM_OPCAO[op[i]], cur ? C_MARFIM : C_MARFIM_S, false);
        const char *v = som_valor(op[i]);
        int vw = gfx_largura(v, 1), vx = x + w - 34 - vw;
        txt(vx, ly - 2, v, cur ? C_OURO : C_MARFIM_S, cur);
        if (cur) som_meio_x = vx + vw / 2;
        for (int k = 0; k < 4 && cur; k++) {          // as setinhas: os lados trocam
            gfx_rect(vx - 12 + k, ly + 6 - k, 1, 2 * k + 1, C_LATAO);
            gfx_rect(vx + vw + 11 - k, ly + 6 - k, 1, 2 * k + 1, C_LATAO);
        }
        zona_clique(x + 10, ly - 3, w - 20, SOM_LIN - 1, &som_cur, l, TECLA_SOM);
    }
    if (som_falhou()) txt_c(GFX_W / 2, oy + n_op * SOM_LIN - 2, "sem saída de som neste computador", C_VINHO_CLR, false);
    teclas_c(GFX_W / 2, y + h - 21, controle_estilo ? "setas mudam  ·  M fecha" : "setas mudam  ·  N música  ·  M fecha", C_TEXTO_M);
}
