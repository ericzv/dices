// ---------------------------------------------------------------------------
// Eventos do mapa: uma parada que nao e partida. Cada um oferece escolhas;
// "seguir caminho" sempre pode. O sorteio de cada um sai da semente do no:
// sair e voltar nao muda nada.
// ---------------------------------------------------------------------------
#define EVT_PRECO_LAPIDA 100
#define EVT_PRECO_PUXA   15
#define EVT_PRECO_GARCOM 50
#define EVT_PRECO_COPIA  70
#define EVT_PREMIO_NIQUEL 200
static const int FONTE_MOEDAS[3] = { 25, 50, 100 }, FONTE_CHANCE[3] = { 20, 40, 70 };

enum { EA_SEGUIR, EA_LAPIDA, EA_CAPELA, EA_PUXA, EA_TROCA, EA_FONTE0, EA_FONTE1, EA_FONTE2, EA_GARCOM,
       EA_COMUM, EA_ARRISCA, EA_COPIA };
enum { RL_DADO, RL_MOEDAS, RL_NADA };          // os simbolos dos rolos do caca-niquel

typedef struct { char rot[64], det[32]; bool ok; int acao; } ev_op_t;

static int      ev_id, ev_passo;               // passo: 0 escolhas, 1 escolhe um dado, 2 resultado
static uint32_t ev_sem;
static int      ev_acao;                       // a escolha que pediu um dado da bolsa
static int      ev_lista[MAX_COL], ev_n;       // os dados da bolsa que servem
static char     ev_msg[96], ev_msg2[96];
static peca_t   ev_peca;                       // dado ganho ou mudado (resultado)
static int      ev_mostra = 0;                 // 0 nada, 1 dado, 2 amuleto
static int      ev_amuleto;
static uint8_t  ev_rolos[3] = { RL_DADO, RL_MOEDAS, RL_NADA };
static float    ev_giro = -1;                  // t_fase em que a alavanca desceu
static bool     ev_depois_giro;                // o resultado aparece quando os rolos param

static void ev_sorteio(uint32_t sal) { rs = ev_sem ^ (sal * 0x9E3779B9u); rnd_r(); rnd_r(); }

static int lapida_para(int t)
{
    switch (t) {
    case D_D2: return D_D4;
    case D_D4: return D_D6;
    case D_D6: return D_D8;
    case D_D8: return D_D12;
    default:   return -1;
    }
}

// Este dado da bolsa serve para a escolha?
static bool ev_serve(int acao, peca_t p)
{
    switch (acao) {
    case EA_LAPIDA: return lapida_para(p.tipo) >= 0;
    case EA_CAPELA: return true;
    case EA_TROCA:  return TIPO[p.tipo].raridade < RAR_LENDA;
    case EA_COPIA:  return TIPO[p.tipo].raridade < RAR_LENDA;
    default:        return false;
    }
}

static int ev_monta_lista(int acao)
{
    ev_n = 0;
    for (int i = 0; i < run.n_col; i++) if (ev_serve(acao, run.col[i])) ev_lista[ev_n++] = i;
    return ev_n;
}

static void ev_op(ev_op_t *o, int *n, int acao, bool ok, const char *rot, const char *det)
{
    ev_op_t *x = &o[(*n)++];
    snprintf(x->rot, sizeof x->rot, "%s", rot);
    snprintf(x->det, sizeof x->det, "%s", det ? det : "");
    x->ok = ok;
    x->acao = acao;
}

static int ev_opcoes(ev_op_t *o)
{
    int n = 0;
    bool cabe = run.n_col < MAX_COL;
    char d[32];
    switch (ev_id) {
    case EVT_LAPIDADOR:
        snprintf(d, sizeof d, "%d moedas", EVT_PRECO_LAPIDA);
        ev_op(o, &n, EA_LAPIDA, run.moedas >= EVT_PRECO_LAPIDA && ev_monta_lista(EA_LAPIDA) > 0,
              "Lapidar um dado simples", d);
        break;
    case EVT_CAPELA:
        ev_op(o, &n, EA_CAPELA, run.n_col > N_FILA, "Tirar até 2 dados da bolsa", "grátis");
        break;
    case EVT_NIQUEL:
        snprintf(d, sizeof d, "%d moedas", EVT_PRECO_PUXA);
        ev_op(o, &n, EA_PUXA, run.moedas >= EVT_PRECO_PUXA, "Puxar a alavanca", d);
        break;
    case EVT_TROCADOR:
        ev_op(o, &n, EA_TROCA, ev_monta_lista(EA_TROCA) > 0, "Trocar um dado: sobe uma raridade", "grátis");
        break;
    case EVT_FONTE: {
        uint8_t a;
        uint32_t guarda = rs;
        sorteia_amuletos(&a, 1);                 // ainda ha amuleto para sair?
        rs = guarda;
        for (int i = 0; i < 3; i++) {
            char r[64];
            snprintf(r, sizeof r, "Jogar %d moedas: %d%% de sair um amuleto", FONTE_MOEDAS[i], FONTE_CHANCE[i]);
            snprintf(d, sizeof d, "%d moedas", FONTE_MOEDAS[i]);
            ev_op(o, &n, EA_FONTE0 + i, run.moedas >= FONTE_MOEDAS[i] && a != 0xFF, r, d);
        }
        break;
    }
    case EVT_GARCOM:
        snprintf(d, sizeof d, "%d moedas", EVT_PRECO_GARCOM);
        ev_op(o, &n, EA_GARCOM, run.moedas >= EVT_PRECO_GARCOM && !run.garcom,
              "Subornar o garçom", d);
        break;
    case EVT_ESQUECIDO:
        ev_op(o, &n, EA_COMUM, cabe, "Pegar um dado comum", "grátis");
        ev_op(o, &n, EA_ARRISCA, cabe, "Arriscar: metade raro, metade Quebrado", "grátis");
        break;
    case EVT_COPISTA:
        snprintf(d, sizeof d, "%d moedas", EVT_PRECO_COPIA);
        ev_op(o, &n, EA_COPIA, cabe && run.moedas >= EVT_PRECO_COPIA && ev_monta_lista(EA_COPIA) > 0,
              "Copiar um dado (não copia lendário)", d);
        break;
    }
    ev_op(o, &n, EA_SEGUIR, true, "Seguir caminho", NULL);
    return n;
}

// O evento terminou: o passo esta dado (sair agora volta ao mapa) e o
// resultado fica na tela ate o ENTER.
static void ev_fim(void)
{
    run.passo++;
    run.estado = R_MAPA;
    des_salva();
    ev_passo = 2;
    cursor = 0;
}

static void ev_poe_dado(peca_t p)
{
    if (run.n_col < MAX_COL) run.col[run.n_col++] = p;
    ev_peca = p;
    ev_mostra = 1;
}

static int sorteia_de(int rar)
{
    float w[4] = { 0, 0, 0, 0 };
    w[rar] = 1;
    return sorteia_raridade(w);
}

// Um dado escolhido na bolsa (indice em run.col).
static void ev_aplica_dado(int i)
{
    peca_t antes = run.col[i];
    switch (ev_acao) {
    case EA_LAPIDA: {
        int novo = lapida_para(antes.tipo);
        if (novo < 0 || run.moedas < EVT_PRECO_LAPIDA) return;
        run.moedas -= EVT_PRECO_LAPIDA;
        run.col[i] = (peca_t){ (uint8_t)novo, 0 };
        run.lapidou = 1;
        snprintf(ev_msg, sizeof ev_msg, "%s virou %s.", TIPO[antes.tipo].nome, TIPO[novo].nome);
        ev_msg2[0] = 0;
        ev_peca = run.col[i]; ev_mostra = 1;
        som_toca(SOM_BIGORNA);
        ev_fim();
        return;
    }
    case EA_CAPELA:
        if (run.n_col <= N_FILA) return;
        for (int k = i; k + 1 < run.n_col; k++) run.col[k] = run.col[k + 1];
        run.n_col--;
        run.tirados++;
        som_toca(SOM_TIRA);
        snprintf(ev_msg, sizeof ev_msg, "A Capela levou %d %s da sua bolsa.", run.tirados,
                 run.tirados == 1 ? "dado" : "dados");
        snprintf(ev_msg2, sizeof ev_msg2, "%s saiu.", TIPO[antes.tipo].nome);
        ev_mostra = 0;
        if (run.tirados >= 2 || run.n_col <= N_FILA) { ev_fim(); return; }
        des_salva();
        ev_monta_lista(EA_CAPELA);
        if (cursor >= ev_n) cursor = ev_n - 1;
        return;
    case EA_TROCA: {
        int r = TIPO[antes.tipo].raridade;
        if (r >= RAR_LENDA) return;
        ev_sorteio(31);
        run.col[i] = peca_r(sorteia_de(r + 1));
        snprintf(ev_msg, sizeof ev_msg, "%s virou %s.", TIPO[antes.tipo].nome, TIPO[run.col[i].tipo].nome);
        ev_msg2[0] = 0;
        ev_peca = run.col[i]; ev_mostra = 1;
        som_toca(SOM_PREMIO);
        ev_fim();
        return;
    }
    case EA_COPIA:
        if (run.n_col >= MAX_COL || run.moedas < EVT_PRECO_COPIA || TIPO[antes.tipo].raridade >= RAR_LENDA) return;
        run.moedas -= EVT_PRECO_COPIA;
        ev_poe_dado(antes);
        snprintf(ev_msg, sizeof ev_msg, "Agora você tem mais um %s.", TIPO[antes.tipo].nome);
        ev_msg2[0] = 0;
        som_toca(SOM_PREMIO);
        ev_fim();
        return;
    }
}

// Uma puxada no caca-niquel: tira as moedas, sorteia os rolos e, se deram
// tres iguais, ja paga (o resultado so aparece quando os rolos param).
static void ev_puxa(void)
{
    if (run.moedas < EVT_PRECO_PUXA) return;
    run.moedas -= EVT_PRECO_PUXA;
    run.puxadas++;
    ev_sorteio(100u + run.puxadas);
    for (int i = 0; i < 3; i++) ev_rolos[i] = (uint8_t)sorte_r(3);
    ev_giro = t_fase;
    som_giro(true);
    ev_mostra = 0;
    if (ev_rolos[0] == ev_rolos[1] && ev_rolos[1] == ev_rolos[2]) {
        ev_msg2[0] = 0;
        if (ev_rolos[0] == RL_DADO) {
            if (run.n_col < MAX_COL) {
                ev_poe_dado(peca_r(sorteia_de(RAR_RARO)));
                snprintf(ev_msg, sizeof ev_msg, "Três dados: você ganhou um %s.", TIPO[ev_peca.tipo].nome);
            } else {
                run.moedas += EVT_PREMIO_NIQUEL / 2;
                snprintf(ev_msg, sizeof ev_msg, "Três dados, mas a bolsa está cheia: %d moedas.", EVT_PREMIO_NIQUEL / 2);
            }
        } else if (ev_rolos[0] == RL_MOEDAS) {
            run.moedas += EVT_PREMIO_NIQUEL;
            snprintf(ev_msg, sizeof ev_msg, "Três moedas: você ganhou %d moedas.", EVT_PREMIO_NIQUEL);
        } else {
            snprintf(ev_msg, sizeof ev_msg, "Três vazios: a máquina apagou. Nada.");
        }
        ev_fim();
        ev_passo = 0;                        // a tela de resultado espera os rolos
        ev_depois_giro = true;
    } else {
        des_salva();
        snprintf(ev_msg, sizeof ev_msg, "Não deu três iguais.");
        snprintf(ev_msg2, sizeof ev_msg2, "%d %s até aqui.", run.puxadas, run.puxadas == 1 ? "puxada" : "puxadas");
    }
}

static bool ev_girando(void) { return ev_giro >= 0 && t_fase - ev_giro < 1.1f; }

// Os rolos pararam: some o som e, se deram tres iguais, vem o resultado.
// Devolve true quando acabou de mudar de tela.
static bool ev_atualiza(void)
{
    if (ev_giro < 0 || ev_girando()) return false;
    ev_giro = -2;
    som_giro(false);
    if (!ev_depois_giro) return false;
    ev_depois_giro = false;
    ev_passo = 2;
    som_toca(SOM_PREMIO);
    return true;
}

static void ev_escolhe(int acao)
{
    ev_msg[0] = ev_msg2[0] = 0;
    switch (acao) {
    case EA_SEGUIR:
        run.passo++;
        run.estado = R_MAPA;
        des_salva();
        abre_mapa();
        return;
    case EA_LAPIDA: case EA_CAPELA: case EA_TROCA: case EA_COPIA:
        ev_acao = acao;
        if (!ev_monta_lista(acao)) return;
        ev_passo = 1;
        cursor = 0;
        return;
    case EA_PUXA:
        ev_puxa();
        return;
    case EA_FONTE0: case EA_FONTE1: case EA_FONTE2: {
        int k = acao - EA_FONTE0;
        if (run.moedas < FONTE_MOEDAS[k]) return;
        run.moedas -= FONTE_MOEDAS[k];
        ev_sorteio(41);
        if (sorte_r(100) < FONTE_CHANCE[k]) {
            uint8_t a;
            sorteia_amuletos(&a, 1);
            if (a < A_N) {
                run_poe_amuleto(a);
                ev_amuleto = a;
                ev_mostra = 2;
                snprintf(ev_msg, sizeof ev_msg, "Saiu um amuleto: %s.", AMULETO[a].nome);
                snprintf(ev_msg2, sizeof ev_msg2, "%s", AMULETO[a].desc);
                som_toca(SOM_PREMIO);
                solta_confete(40);
            }
        } else {
            ev_mostra = 0;
            snprintf(ev_msg, sizeof ev_msg, "As %d moedas afundaram. Nada desta vez.", FONTE_MOEDAS[k]);
            som_toca(SOM_FICHA);
        }
        ev_fim();
        return;
    }
    case EA_GARCOM:
        if (run.moedas < EVT_PRECO_GARCOM) return;
        run.moedas -= EVT_PRECO_GARCOM;
        run.garcom = 1;
        ev_mostra = 0;
        snprintf(ev_msg, sizeof ev_msg, "Combinado: o próximo oponente começa com 10 fichas a menos.");
        som_toca(SOM_FICHA);
        ev_fim();
        return;
    case EA_COMUM: case EA_ARRISCA:
        if (run.n_col >= MAX_COL) return;
        ev_sorteio(53);
        if (acao == EA_COMUM) ev_poe_dado(peca_r(sorteia_de(RAR_COMUM)));
        else ev_poe_dado(sorte_r(2) ? peca_r(sorteia_de(RAR_RARO)) : peca_r(D_QUEBRADO));
        snprintf(ev_msg, sizeof ev_msg, "Você pegou um %s.", TIPO[ev_peca.tipo].nome);
        som_toca(ev_peca.tipo == D_QUEBRADO ? SOM_TIRA : SOM_PREMIO);
        ev_fim();
        return;
    }
}

// Abre a tela do evento do passo atual (vindo do mapa ou de um salvamento).
static void abre_evento(void)
{
    const no_t *no = &run.no[run.passo][run.caminho[run.passo]];
    modo = M_DESAFIO;
    cpu = remoto = 0;
    ev_id = no->perso < EVT_N ? no->perso : EVT_CAPELA;
    ev_sem = no->semente;
    ev_passo = 0;
    ev_giro = -1;
    ev_depois_giro = false;
    ev_mostra = 0;
    ev_msg[0] = ev_msg2[0] = 0;
    if (ev_id == EVT_CAPELA && run.tirados)
        snprintf(ev_msg, sizeof ev_msg, "A Capela já levou %d %s.", run.tirados, run.tirados == 1 ? "dado" : "dados");
    cursor = 0;
    t_fase = 0;
    fase = F_REVENTO;
}

static void entra_evento(void)
{
    run.estado = R_EVENTO;
    run.tirados = 0;
    run.puxadas = 0;
    des_salva();
    abre_evento();
}

// ---- Desenho -------------------------------------------------------------
// Um simbolo do rolo do caca-niquel, centrado em (x, y).
static void simbolo_rolo(int x, int y, int s)
{
    if (s == RL_DADO) {
        gfx_rect(x - 6, y - 6, 12, 12, C_EBANO_B);
        gfx_rect(x - 5, y - 5, 10, 10, C_MARFIM);
        gfx_rect(x - 3, y - 3, 2, 2, C_EBANO_B); gfx_rect(x - 1, y - 1, 2, 2, C_EBANO_B);
        gfx_rect(x + 1, y + 1, 2, 2, C_EBANO_B);
    } else if (s == RL_MOEDAS) {
        gfx_disco(x, y, 6, C_LATAO_ESC);
        gfx_disco(x, y, 5, C_OURO);
        gfx_disco(x - 2, y - 2, 1, gfx_mistura(C_OURO, C_BRANCO, 60));
    } else {
        gfx_rect(x - 5, y, 10, 2, gfx_mistura(C_FELTRO2, C_BARRA, 30));   // vazio
    }
}

// A maquina, no quadro do evento (66x81 a partir de x, y).
static void desenha_niquel(int x, int y)
{
    uint16_t corpo = RGB(150, 40, 48), borda = C_OURO;
    gfx_rect(x + 6, y + 6, 52, 70, gfx_mistura(corpo, C_BARRA, 35));
    gfx_rect(x + 5, y + 4, 52, 70, corpo);
    gfx_moldura(x + 5, y + 4, 52, 70, 1, borda);
    gfx_rect(x + 9, y + 8, 44, 10, C_BARRA);                         // letreiro
    for (int i = 0; i < 5; i++) {
        bool acesa = ((int)(t_fase * 3) + i) % 2 == 0;
        gfx_disco(x + 13 + i * 9, y + 13, 2, acesa ? C_AMARELO : gfx_mistura(C_AMARELO, C_BARRA, 60));
    }
    bool gira = ev_girando();
    for (int i = 0; i < 3; i++) {                                    // os tres rolos
        int rx = x + 10 + i * 14, ry = y + 24;
        gfx_rect(rx, ry, 12, 20, C_MARFIM_S);
        gfx_moldura(rx - 1, ry - 1, 14, 22, 1, C_EBANO_B);
        bool este = gira && t_fase - ev_giro < 0.45f + i * 0.3f;
        if (este) {
            int f = (int)((t_fase - ev_giro) * 18) + i;
            simbolo_rolo(rx + 6, ry + 4 + (f % 2) * 6, f % 3);
            simbolo_rolo(rx + 6, ry + 16 + (f % 2) * 6, (f + 1) % 3);
            gfx_rect(rx, ry - 1, 12, 2, C_EBANO_B);
            gfx_rect(rx, ry + 19, 12, 2, C_EBANO_B);
        } else {
            simbolo_rolo(rx + 6, ry + 10, ev_rolos[i]);
        }
    }
    gfx_rect(x + 9, y + 50, 44, 6, gfx_mistura(corpo, C_BARRA, 40));   // bandeja das moedas
    gfx_rect(x + 13, y + 60, 36, 10, C_BARRA);
    bool desce = gira && t_fase - ev_giro < 0.35f;                  // a alavanca
    int ly = desce ? y + 40 : y + 14;
    gfx_rect(x + 58, y + 30, 3, 14, C_ACO);
    gfx_linha_grossa(x + 60, y + 32, x + 62, ly, 2, C_ACO);
    gfx_disco(x + 62, ly, 3, C_VERMELHO);
}

static void desenha_evt(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 120);
    faixa_run(0);
    const evt_t *ev = &EVT[ev_id];
    // O quadro da esquerda: quem (ou o que) esta ali.
    int esc = 4, qx = 30, qy = 60, qw = RT_W * esc, qh = RT_H * esc;
    gfx_rect(qx - 3, qy - 3, qw + 6, qh + 6, cor_no(NO_EVENTO));
    gfx_rect(qx - 1, qy - 1, qw + 2, qh + 2, C_BARRA);
    gfx_rect(qx, qy, qw, qh, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    if (ev_id == EVT_NIQUEL) desenha_niquel(qx + (qw - 66) / 2, qy + (qh - 81) / 2);
    else retrato(qx, qy, esc, RETRATO_EVENTO + ev_id, t_fase, false, 0, RT_H);
    int x = qx + qw + 16, w = MESA_X1 - 14 - x;
    gfx_texto(x, 54, ev->nome, cor_no(NO_EVENTO), 2, true);
    txt(x, 86, ev->cena, C_MARFIM_S, false);
    int nl = quebra_linhas(x, 102, w, 2, ev->resumo, C_MARFIM);
    int y0 = 102 + nl * LINHA_FINA + 14;

    ev_atualiza();

    if (ev_passo == 0) {
        ev_op_t op[6];
        int n = ev_opcoes(op);
        if (cursor >= n) cursor = n - 1;
        for (int i = 0; i < n; i++) {
            int y = y0 + i * 27;
            bool cur = i == cursor;
            ui_botao(x, y, w, 24, "", cur && op[i].ok ? C_LATAO : 0, !op[i].ok ? UI_DESLIGADO : cur ? UI_FOCO : 0);
            uint16_t c = !op[i].ok ? C_EBANO_B : (cur ? C_BARRA : C_MARFIM);
            gfx_texto(x + 10, y + 1, op[i].rot, c, 1, false);
            if (op[i].det[0]) gfx_texto(x + w - 10 - gfx_largura(op[i].det, 1), y + 1, op[i].det, c, 1, false);
            zona_clique(x, y, w, 24, &cursor, i, KEY_ENTER);
        }
        if (!ev_girando()) {
            int ym = y0 + n * 27 + 6;
            if (ev_msg[0]) txt(x, ym, ev_msg, C_AMARELO, true);
            if (ev_msg2[0]) txt(x, ym + TXT_H + 2, ev_msg2, C_MARFIM_S, false);
        }
        rodape_run("setas escolhem  ·  ENTER confirma");
    } else if (ev_passo == 1) {
        static const char *const PEDE[] = { [EA_LAPIDA] = "Qual dado lapidar?", [EA_CAPELA] = "Qual dado tirar da bolsa?",
                                            [EA_TROCA] = "Qual dado trocar?", [EA_COPIA] = "Qual dado copiar?" };
        txt(x, y0, PEDE[ev_acao], C_AMARELO, true);
        botao_clique(MESA_X1 - 12, y0 - 4, ev_acao == EA_CAPELA && run.tirados ? "pronto" : "voltar", KEY_BKSP);
        int por_linha = (w - 4) / 44;
        for (int k = 0; k < ev_n; k++) {
            int px = x + 20 + (k % por_linha) * 44, py = y0 + 44 + (k / por_linha) * 42;
            if (k == cursor) ui_selecao(px - 20, py - 20, 40, 40, C_OURO);
            peca_parada((float)px, (float)py, 15, run.col[ev_lista[k]], 0);
            zona_clique(px - 20, py - 20, 40, 40, &cursor, k, KEY_ENTER);
        }
        int yd = y0 + 44 + ((ev_n - 1) / por_linha + 1) * 42 - 8;
        if (cursor >= 0 && cursor < ev_n) {
            peca_t p = run.col[ev_lista[cursor]];
            char s[96];
            if (ev_acao == EA_LAPIDA) snprintf(s, sizeof s, "%s vira %s", TIPO[p.tipo].nome, TIPO[lapida_para(p.tipo)].nome);
            else if (ev_acao == EA_TROCA) snprintf(s, sizeof s, "%s (%s) vira um dado %s, sorteado", TIPO[p.tipo].nome,
                                                   RARIDADE[TIPO[p.tipo].raridade], RARIDADE[TIPO[p.tipo].raridade + 1]);
            else snprintf(s, sizeof s, "%s: %s", TIPO[p.tipo].nome, TIPO[p.tipo].desc);
            txt(x, yd, s, C_MARFIM, false);
        }
        if (ev_acao == EA_CAPELA && run.tirados) txt(x, yd + TXT_H + 4, ev_msg, C_MARFIM_S, false);
        rodape_run("setas escolhem  ·  ENTER confirma  ·  I detalhes  ·  BACKSPACE volta");
    } else {
        txt(x, y0, ev_msg, C_AMARELO, true);
        if (ev_msg2[0] && ev_mostra != 2) txt(x, y0 + TXT_H + 4, ev_msg2, C_MARFIM_S, false);
        int yc = y0 + 64;
        if (ev_mostra == 1) {
            peca_parada((float)(x + 30), (float)yc, 22, ev_peca, 0);
            txt(x + 66, yc - 22, TIPO[ev_peca.tipo].nome, cor_dado(ev_peca.tipo), true);
            txt(x + 66 + gfx_largura(TIPO[ev_peca.tipo].nome, 1) + 8, yc - 22, RARIDADE[TIPO[ev_peca.tipo].raridade],
                cor_rar(ev_peca.tipo), false);
            quebra_linhas(x + 66, yc - 4, w - 70, 2, TEXTO_MEDIO[ev_peca.tipo], C_MARFIM_S);
        } else if (ev_mostra == 2) {
            desenha_amuleto(x + 30, yc, 22, ev_amuleto, false);
            txt(x + 66, yc - 22, AMULETO[ev_amuleto].nome, AMULETO[ev_amuleto].cor, true);
            quebra_linhas(x + 66, yc - 4, w - 70, 2, AMULETO[ev_amuleto].desc, C_MARFIM_S);
        }
        botao_clique(MESA_X1 - 12, MESA_Y1 - 30, "seguir", KEY_ENTER);
        rodape_run("ENTER segue para o mapa");
    }
}

static bool ev_tecla(int k, int e)
{
    if (ev_girando() || ev_atualiza()) return true;
    if (ev_passo == 2) {
        if (k == KEY_ENTER) { som_toca(SOM_FICHA); abre_mapa(); }
        return true;
    }
    if (ev_passo == 1) {
        if (k == KEY_BKSP || k == 'x') {
            if (ev_acao == EA_CAPELA && run.tirados) ev_fim();
            else { ev_passo = 0; cursor = 0; }
            return true;
        }
        if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= 11) cursor -= 11; return true; }
        if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + 11 < ev_n) cursor += 11; return true; }
        if (e && ev_n) { cursor = (cursor + e + ev_n) % ev_n; som_toca(SOM_TIQUE); return true; }
        if (k == KEY_ENTER && cursor >= 0 && cursor < ev_n) ev_aplica_dado(ev_lista[cursor]);
        return true;
    }
    ev_op_t op[6];
    int n = ev_opcoes(op);
    if (e) { cursor = (cursor + e + n) % n; som_toca(SOM_TIQUE); return true; }
    if (k == KEY_ENTER && cursor >= 0 && cursor < n && op[cursor].ok) {
        som_toca(SOM_FICHA);
        ev_escolhe(op[cursor].acao);
    }
    return true;
}

// ---- Escolha de amuleto ---------------------------------------------------
static void desenha_amuleto_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run(0);
    txt_c2(GFX_W / 2, MESA_Y0 + 2, run.am_volta ? "O CHEFE DEIXOU UM AMULETO" : "ESCOLHA UM AMULETO", C_OURO);
    txt_c(GFX_W / 2, MESA_Y0 + 44, "Ele vale a run inteira.", C_MARFIM_S, false);
    for (int i = 0; i < 3; i++) {
        int a = run.am_oferta[i];
        if (a >= A_N) continue;
        int x = GFX_W / 2 + (i - 1) * 150;
        bool cur = i == cursor;
        ui_cartao(x - 64, 124, 128, 100, cur, 0);
        float bob = cur ? sinf(t_fase * 4) * 3 : 0;
        desenha_amuleto(x, 162 + (int)bob, 26, a, false);
        txt_c(x, 198, AMULETO[a].nome, cur ? AMULETO[a].cor : C_MARFIM_S, true);
        zona_clique(x - 64, 124, 128, 100, &cursor, i, KEY_ENTER);
    }
    if (cursor >= 0 && cursor < 3 && run.am_oferta[cursor] < A_N)
        txt_c(GFX_W / 2, 234, AMULETO[run.am_oferta[cursor]].desc, C_MARFIM, false);
    // Os que ja tem, miudos, embaixo.
    int na = am_conta(run_amuletos());
    if (na) {
        int x0 = GFX_W / 2 - (na - 1) * 10, n = 0;
        txt_c(GFX_W / 2, 258, "seus amuletos", C_TEXTO_M, false);
        for (int a = 0; a < A_N; a++)
            if (run_amuletos() >> a & 1) desenha_amuleto(x0 + 20 * n++, 288, 8, a, false);
    }
    rodape_run("setas escolhem  ·  ENTER leva o amuleto");
}
