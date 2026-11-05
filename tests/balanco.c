// Balanco do Modo Desafiante: a IA joga pela pessoa, runs inteiras ou uma
// mesa so, e mede o quanto cada coisa e dificil. Nao e teste de passar ou
// falhar (fica fora do ctest): imprime numeros para comparar versoes. Tudo
// vem do codigo do jogo (as bolsas, a IA, os trunfos e as viradas dos
// oponentes, a loja, as regras): os numeros acompanham o jogo como ele esta.
//
// A run: 3 saloes de 3 mesas (o oponente facil, o dificil e o chefe - o
// Crupie, a Dona, o Barao) e, do Nivel 5 (CORINGA_DESDE), o salao do Coringa.
//
//   balanco curva <runs> <nivel> [ia] [aposta] [compra] [recomecos] [imortal] [semente]
//       chance de vencer cada mesa para quem chegou nela (MESA), onde as runs
//       param (PAROU: uma coluna por mesa, a ultima = venceu a run) e cada
//       oponente por salao e mesa (PERSO salao mesa jogos vitorias nome).
//       aposta: 0 a IA do jogo, 1 nunca aposta e sempre paga, 2 sempre 25 e
//         sempre dobra, 3 a IA mas nunca corre, 4 nunca aposta e corre de
//         tudo, 5 blefa 25 sempre que abre.
//       compra: 0 o mais raro que couber, 1 o que forca_colecao mede como
//         melhor, 2 nada.
//       recomecos: perdeu a partida, tenta sair com ESC ESC (ate n vezes por
//         mesa): sair no meio conta como derrota, entao nao salva nada.
//       imortal: perder nao acaba a run (mede todas as mesas por igual; a
//         derrota paga como vitoria) e a linha RUN da a chance de vencer a
//         run inteira pelo produto das mesas.
//   balanco inimigos [nivel] [amostras]
//       a progressao dos oponentes: em cada mesa da run, a IA (0 a simples, 1
//       a media, 2 a forte), o trunfo, a virada da 5a rodada (chefes), o
//       amuleto a mais da Cobertura e a bolsa media (dados, especiais por
//       raridade, o valor dos 4 melhores de uma mao). Sem o nivel, do Normal
//       ao 10. ("bolsas" e o mesmo.)
//   balanco dados <partidas> <nivel> <salao> <mesa> <copias> [primeiro] [ultimo]
//       a bolsa do comeco com copias de cada dado, numa mesa so (salao 1 a 3,
//       4 o do Coringa, do Nivel 5; mesa 1 o facil, 2 o dificil, 3 o chefe).
//       primeiro/ultimo: os tipos de dado (D_*) de um pedaco, para rodar em
//       paralelo. Os oponentes so trazem o que o nivel ja abriu, como no jogo.
//   balanco amuletos <partidas> <nivel> <salao> <mesa> [primeiro] [ultimo]
//       um amuleto so, com a bolsa do comeco e dados da vitrine.
//   balanco recomeca
//       mostra que sair no meio conta como derrota (antes recomecava a mesa).
//
// ESFORCO=0 no ambiente faz a IA pensar pouco (bem mais rapido); o jogo usa 1.
// ADESIVOS=0: o robo nao compra adesivos (para comparar). BOLSA=b: a bolsa da
// run (B_*; o Saco de Pano se nao der). NATAL=2: como em dezembro (o Papai
// Noel no lugar do Barao, os oponentes de Natal); sem ele, fora de dezembro,
// qualquer que seja a data.
#include <stdio.h>
#include <stdlib.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
static int64_t relogio = 1;
int64_t esp_timer_get_time(void) { return relogio; }
void som_toca(int s) { (void)s; }
void som_toca_tom(int s, int t) { (void)s; (void)t; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }
void som_rufo_moeda(void) {}
void som_giro(bool on) { (void)on; }
void som_suspense(bool on) { (void)on; }
void som_cadeia(bool on) { (void)on; }
void som_chefe(int q, bool t) { (void)q; (void)t; }
void som_volume(int c, int n) { (void)c; (void)n; }
int som_volume_de(int c) { (void)c; return SOM_NIVEIS; }

#include "salva_mem.h"

// O primeiro adesivo da loja que da para comprar agora (e colar), ou -1.
static int adesivo_a_comprar(void)
{
    for (int k = 0; k < n_adesivos_loja(); k++) {
        int a = ad_loja(k);
        if (a && !ad_loja_vendido(k) && run.moedas >= preco_adesivo(a) && lj_algum_lado(a)) return k;
    }
    return -1;
}

static uint32_t est = 99991;
static uint32_t rnd_t(void) { est ^= est << 13; est ^= est >> 17; est ^= est << 5; return est; }

static int ia_jog = 2, jeito_aposta, jeito_compra, recomecos, recomecou;
static int adesivos_robo = 1;           // ADESIVOS=0 no ambiente: o robo nao compra adesivos

// Um perfil com o nivel pedido aberto (tudo: todos os dados e amuletos), e
// sempre com o D20 e o Lastro, que abrem nos dois primeiros chefes.
static void perfil_teste(int nivel, bool tudo)
{
    salva_limpa_tudo();
    memset(perfis, 0, sizeof perfis);
    snprintf(perfis[0].nome, sizeof perfis[0].nome, "BALANCO");
    perfis[0].nivel = (uint8_t)(tudo ? N_NIVEIS : nivel);
    perfis[0].marcos = 3;
    perfil_atual = 0;
    perfis_lidos = true;
    des_dif = nivel;
    perfis[0].bolsas = BOLSAS_TODAS;             // todas abertas: BOLSA=b escolhe a da run
    des_bolsa = getenv("BOLSA") ? atoi(getenv("BOLSA")) : B_PANO;
}

static int escolhe_aposta(void)
{
    int op[4], n = opcoes(op), quer = OP_PASSAR;
    ia_nivel = ia_jog;
    ia_ousadia = 0;
    switch (jeito_aposta) {
    case 1: quer = a_pagar ? OP_PAGAR : OP_PASSAR; break;
    case 2: quer = a_pagar ? OP_AUMENTAR : OP_APOSTAR25; break;
    case 3: quer = op[ia_aposta()]; if (quer == OP_CORRER) quer = OP_PAGAR; break;
    case 4: quer = a_pagar ? OP_CORRER : OP_PASSAR; break;
    case 5: quer = a_pagar ? op[ia_aposta()] : OP_APOSTAR25; break;
    }
    for (int i = 0; i < n; i++) if (op[i] == quer) return i;
    for (int i = 0; i < n; i++) if (op[i] == OP_PAGAR || op[i] == OP_PASSAR) return i;
    return 0;
}

// O ENTER da tela do resultado vai acabar a partida perdida? (o teste de nova_rodada)
static bool vai_perder(void)
{
    int r = rodada + 1, f0 = J[0].fichas, f1 = J[1].fichas;
    if (r > DES_RODADAS && !(f0 == f1 && r <= DES_RODADAS + DES_EXTRAS)) return f0 <= f1;
    int ante = ante_da(r);
    if (f0 < ante && f1 < ante) return f0 <= f1;
    return f0 < ante;
}

// ESC duas vezes, pelo mesmo caminho das teclas de uma pessoa.
static void esc_esc(void)
{
    key_event_t e = { KEY_ESC };
    dado_tecla(&e);
    dado_tecla(&e);
}

// Um passo da partida; false quando ela acabou.
static bool partida_passo(void)
{
    relogio += 16667;
    if (fase == F_FIM) return false;
    if (fase == F_APOSTA && vez == 0 && jeito_aposta && t_anuncio <= 0) {
        cursor = escolhe_aposta();
        trata_tecla(KEY_ENTER);
        return true;
    }
    if (fase == F_RESULTADO) {
        if (recomecou < recomecos && vai_perder()) {
            esc_esc();                           // sair agora conta como derrota: a mesa acaba ali
            if (fase != F_FIM) { printf("sair no meio devia encerrar a partida: fase %d\n", fase); exit(1); }
            recomecou++;
            return true;
        }
        trata_tecla(KEY_ENTER);
        return true;
    }
    cpu = 3;
    nivel_jog[0] = ia_jog;
    ousadia_jog[0] = 0;
    dado_passo(1.0f / 60);
    return true;
}

// O dado da vitrine que mais sobe a forca da bolsa (contra o proximo oponente), por moeda.
static int compra_medida(void)
{
    memset(J, 0, sizeof J);
    memset(F, 0, sizeof F);
    J[0].n_col = run.n_col;
    memcpy(J[0].col, run.col, sizeof run.col);
    int p = run.passo + 1 < mesas_do_salao() ? run.passo + 1 : run.passo;
    J[1].n_col = monta_oponente(&run.no[p], run.andar, J[1].col);
    amul[0] = run_amuletos();
    amul[1] = 0;
    reg_op = REG_NADA;
    rodada = 1;
    uint32_t base = 0x51ED27u + run.lojas * 977u;
    float sem = forca_colecao(0, -1, 0, base), melhor = 0;
    int b = -1;
    for (int i = 0; i < n_vitrine(); i++) {
        int t = run.vitrine[i].tipo;
        if (run.levou[i] || run.moedas < preco_run(t)) continue;
        float g = forca_colecao(0, t, run.vitrine[i].fixo, base) - sem;
        if (g > 0.004f && g / (float)preco_run(t) > melhor) { melhor = g / (float)preco_run(t); b = i; }
    }
    modo = M_DESAFIO;
    fase = F_RLOJA;
    return b;
}

// A loja da run: compra pelo jeito pedido; com sobra, abre o pacote, tira uma
// moeda (D2) ou troca a vitrine uma vez.
static void loja_passo_robo(int *compras)
{
    relogio += 16667;
    if (lj_colando) {                            // o adesivo: no dado de mais lados; o Reflexivo e o
        if (t_lj_cola > 0) { dado_passo(1.0f / 60); return; }   // Avesso no lado mais baixo, os outros no mais alto
        if (lj_colando == 1) {
            int b = -1;
            for (int i = 0; i < run.n_col; i++)
                if (ad_cabe_o(&run.col[i], cola_ad()) && (b < 0 || TIPO[run.col[i].tipo].lados > TIPO[run.col[b].tipo].lados)) b = i;
            cursor = b;
            trata_tecla(KEY_ENTER);
            return;
        }
        const peca_t *pc = &run.col[lj_cola_dado];
        int t = pc->tipo, a = cola_ad(), c = -1;
        bool baixo = a == AD_REFLEXIVO || a == AD_AVESSO;
        for (int l = ad_lado_min(t); l <= ad_lado_max(t); l++)
            if (!ad_no_lado(pc, l) && (c < 0 || (baixo ? l < c : l > c))) c = l;
        cursor = c - ad_lado_min(t);
        trata_tecla(KEY_ENTER);
        return;
    }
    if (lj_pac) {                                // o pacote aberto: leva o mais raro (dado ou adesivo)
        if (t_pac < PAC_PRONTO || t_pac_fim > 0) { dado_passo(1.0f / 60); return; }
        int b = 0;
        for (int i = 1; i < pac_n(); i++)
            if (pacote_de_adesivos() ? RAR_ADESIVO[pac_ad(i)] > RAR_ADESIVO[pac_ad(b)]
                                     : TIPO[pac_carta_p(i)->tipo].raridade > TIPO[pac_carta_p(b)->tipo].raridade) b = i;
        cursor = b;
        trata_tecla(KEY_ENTER);
        (*compras)++;
        return;
    }
    if (lj_tirando) {
        if (t_lj_tira > 0 || t_lj_volta > 0) { dado_passo(1.0f / 60); return; }
        int m = -1;
        for (int i = 0; i < run.n_col && m < 0; i++) if (run.col[i].tipo == D_D2) m = i;
        if (m < 0) { trata_tecla(KEY_BKSP); return; }
        cursor = m;
        trata_tecla(KEY_ENTER);
        for (int q = 0; q < 60 && t_lj_tira > 0; q++) dado_passo(1.0f / 60);
        return;
    }
    int nv = n_vitrine(), b = -1, ka;
    if (jeito_compra == 0) {
        for (int i = 0; i < nv; i++) {
            int t = run.vitrine[i].tipo;
            if (run.levou[i] || run.moedas < preco_run(t) || t == D_QUEBRADO) continue;
            if (b < 0 || TIPO[t].raridade > TIPO[run.vitrine[b].tipo].raridade) b = i;
        }
    } else if (jeito_compra == 1) b = compra_medida();
    bool moeda = false;
    for (int i = 0; i < run.n_col; i++) moeda |= run.col[i].tipo == D_D2;
    if (b >= 0) { cursor = b; trata_tecla(KEY_ENTER); (*compras)++; }
    else if (jeito_compra != 2 && run.pac_estado == PAC_VENDA && tem_pacote() && run.moedas >= preco_pacote()
             && (!pacote_de_adesivos() || (adesivos_robo && lj_algum_lado_livre()))) {
        cursor = lj_item_pacote();               // sobrou: abre o pacote
        trata_tecla(KEY_ENTER);
    }
    else if (jeito_compra != 2 && adesivos_robo && (ka = adesivo_a_comprar()) >= 0) { cursor = lj_item_adesivo(ka); trata_tecla(KEY_ENTER); }
    else if (jeito_compra != 2 && moeda && run.moedas >= preco_remove() + 3) { cursor = lj_item_tira(); trata_tecla(KEY_ENTER); }
    else if (jeito_compra != 2 && !run.trocas && run.moedas >= 6) { cursor = lj_item_troca(); trata_tecla(KEY_ENTER); }
    else trata_tecla(KEY_TAB);
}

// ---- Runs inteiras ----
// As mesas da run, em ordem: as tres de cada salao (o facil, o dificil e o
// chefe) e, por ultimo, a do Coringa.
#define N_MESAS (DES_ANDARES * DES_MESAS + 1)
static int mesa_idx(int andar, int passo) { return andar >= DES_ANDARES ? N_MESAS - 1 : andar * DES_MESAS + passo; }
static const char *nome_mesa(int i)
{
    static const char *const CHEFE[DES_ANDARES] = { "Crupie", "Dona", "Barao" };   // (em dezembro, o Noel no lugar do Barao)
    static char s[N_MESAS][24];
    if (i < 0 || i >= N_MESAS) return "?";
    if (i == N_MESAS - 1) return "S4 chefe (Coringa)";
    int a = i / DES_MESAS, p = i % DES_MESAS;
    if (p == DES_MESAS - 1) snprintf(s[i], sizeof s[i], "S%d chefe (%s)", a + 1, CHEFE[a]);
    else snprintf(s[i], sizeof s[i], "S%d %s", a + 1, p == 0 ? "facil" : "dificil");
    return s[i];
}
static int chegou[N_MESAS], venceu[N_MESAS], parou[N_MESAS + 1];
static long margem[N_MESAS], bolsa[N_MESAS];
static int pj[DES_ANDARES + 1][4][N_PERSO], pv[DES_ANDARES + 1][4][N_PERSO];

static void curva(int runs, int nivel, bool imortal, uint32_t sem)
{
    int vencidas = 0, total_recomecos = 0, salvas = 0;
    long compras = 0, tirados = 0, dados_fim = 0;
    est = sem;
    for (int r = 0; r < runs; r++) {
        relogio = 7919LL * (r + 1) * 1000003LL + sem;
        modo = M_UM;
        dado_inicia(r);
        perfil_teste(nivel, false);
        des_nova_run();
        int mesa = -1, c = 0;
        long passos = 0;
        while (fase != F_RFIM) {
            if (++passos > 5000000) { printf("run parada na fase %d\n", fase); exit(1); }
            switch (fase) {
            case F_RAMULETO:                     // ao acaso; cheio, troca um ao acaso
                if (run.am_novo) cursor = (int)(rnd_t() % TROCA_N);
                else { cursor = (int)(rnd_t() % 3u); while (run.am_oferta[cursor] >= A_N) cursor = (cursor + 1) % 3; }
                trata_tecla(KEY_ENTER);
                break;
            case F_RLOJA:
                loja_passo_robo(&c);
                break;
            case F_MAPA:
                trata_tecla(KEY_ENTER);
                if (fase == F_ORDEM || fase == F_JOGA) {
                    mesa = mesa_idx(run.andar, run.passo);
                    chegou[mesa]++;
                    bolsa[mesa] += run.n_col;
                    recomecou = 0;
                }
                break;
            case F_FIM: {
                const no_t *no = &run.no[run.passo];
                pj[run.andar][no->tipo][no->perso]++;
                if (venc_partida == 0) { pv[run.andar][no->tipo][no->perso]++; venceu[mesa]++; salvas += recomecou > 0; }
                margem[mesa] += J[0].fichas - J[1].fichas;
                total_recomecos += recomecou;
                if (imortal && venc_partida != 0) { venc_partida = 0; des_resultado(); }
                trata_tecla(KEY_ENTER);
                break;
            }
            case F_RBOLSA:
            case F_NOVIDADES:
                trata_tecla(KEY_TAB);
                break;
            case F_RAVISO:                       // vencido o Barao do Nivel 5: o aviso
                trata_tecla(KEY_ENTER);
                break;
            default:
                partida_passo();
                break;
            }
        }
        if (des_venceu_run) { vencidas++; parou[N_MESAS]++; }
        else parou[mesa]++;
        compras += c;
        tirados += run.remocoes;
        dados_fim += run.n_col;
    }
    printf("RUNS %d nivel %d: vencidas %d (%.1f%%)  compras %.1f tirados %.1f dados_fim %.1f recomecos %d salvas %d\n",
           runs, nivel, vencidas, 100.0 * vencidas / runs, (double)compras / runs, (double)tirados / runs,
           (double)dados_fim / runs, total_recomecos, salvas);
    printf("MESA n chegou venceu %% margem bolsa  mesa\n");
    double run_toda = 1;
    for (int i = 0; i < N_MESAS; i++)
        if (chegou[i]) {
            printf("MESA %2d %6d %6d %5.1f %6.1f %5.1f  %s\n", i, chegou[i], venceu[i], 100.0 * venceu[i] / chegou[i],
                   (double)margem[i] / chegou[i], (double)bolsa[i] / chegou[i], nome_mesa(i));
            run_toda *= (double)venceu[i] / chegou[i];
        }
    if (imortal) printf("RUN vencida, pelo produto das mesas: %.1f%%\n", 100.0 * run_toda);
    printf("PAROU");
    for (int i = 0; i <= N_MESAS; i++) printf(" %d", parou[i]);
    putchar('\n');
    for (int a = 0; a <= DES_ANDARES; a++)
        for (int t = 0; t < 4; t++)
            for (int p = 0; p < N_PERSO; p++)
                if (pj[a][t][p]) printf("PERSO %d %d %d %d %s\n", a, t, pj[a][t][p], pv[a][t][p], PERSO[p].nome);
}

// ---- Uma mesa so ----
static bool duelo(const peca_t *b, int nb, uint32_t am, int nivel, int salao, int mesa, int *marg)
{
    relogio += 7777777;
    modo = M_UM;
    memset(&run, 0, sizeof run);
    memcpy(run.marca, RUN_MARCA, 4);
    run.semente = rnd_t() | 1;
    run.dificuldade = (uint8_t)nivel;
    run.andar = (uint8_t)salao;
    run.bolsa = (uint8_t)(des_bolsa >= 0 && des_bolsa < B_N ? des_bolsa : B_PANO);   // a regra da bolsa vale na mesa
    if (salao >= DES_ANDARES) gera_salao_final();
    else { gera_andar(); run.passo = (uint8_t)mesa; }
    run.n_col = (uint8_t)nb;
    memcpy(run.col, b, sizeof(peca_t) * (size_t)nb);
    run_poe_amuletos(am);
    recomecou = recomecos;
    des_comeca_partida();
    long passos = 0;
    while (partida_passo())
        if (++passos > 3000000) { printf("partida parada na fase %d\n", fase); exit(1); }
    *marg = J[0].fichas - J[1].fichas;
    return venc_partida == 0;
}

// Os dados de saida da bolsa da run (BOLSA=b; o Saco de Pano se nao der),
// como bolsa_monta_run: os Quebrados com o numero gravado, o Adesivado da
// Lata ja com os adesivos.
static int bolsa_inicio(peca_t *b)
{
    int q = des_bolsa >= 0 && des_bolsa < B_N ? des_bolsa : B_PANO;
    for (int i = 0; i < bolsa_n_dados(q); i++) {
        b[i] = (peca_t){ .tipo = BOLSA[q].dados[i], .fixo = bolsa_fixo(q, i) };
        if (b[i].tipo == D_ADESIVADO) adesiva(&b[i], rola_r, true);
    }
    return bolsa_n_dados(q);
}

// Cada dado com as mesmas sementes: a diferenca e so o dado. A primeira
// linha (+D6) e a bolsa do comeco com D6 a mais.
static void dados(int n, int nivel, int salao, int mesa, int copias, int t0, int t1)
{
    perfil_teste(nivel, false);                  // os oponentes so com o que o nivel abriu (o dado medido entra direto)
    for (int t = t0; t < t1 && t < D_N; t++) {
        if (t >= 0 && (so_inicial(t) || sem_sorteio(t) || t == D_QUEBRADO)) continue;
        int tt = t < 0 ? D_D6 : t;
        peca_t b[MAX_COL];
        int nb = bolsa_inicio(b);
        for (int c = 0; c < copias; c++) b[nb++] = peca_r(tt);
        est = 4242;
        int v = 0, mg;
        long m = 0;
        for (int i = 0; i < n; i++) { v += duelo(b, nb, 0, nivel, salao, mesa, &mg); m += mg; }
        printf("DADO %-16s %-9s %5.1f%% %6.1f\n", t < 0 ? "(+D6)" : TIPO[t].nome, RARIDADE[TIPO[tt].raridade],
               100.0 * v / n, (double)m / n);
        fflush(stdout);
    }
}

// Cada amuleto sozinho, com a bolsa do comeco e 2 dados da vitrine por salao
// (os mesmos para todos). A primeira linha e sem amuleto.
static void amuletos(int n, int nivel, int salao, int mesa, int a0, int a1)
{
    perfil_teste(nivel, false);                  // como no jogo: oponentes e vitrine com o que o nivel abriu
    for (int a = a0; a < a1 && a < A_N; a++) {
        if (a >= 0 && !amuleto_no_jogo(a)) continue;
        est = 4243;
        int v = 0, mg;
        long m = 0;
        for (int i = 0; i < n; i++) {
            peca_t b[MAX_COL];
            int nb = bolsa_inicio(b);
            memset(&run, 0, sizeof run);
            run.semente = rnd_t();
            run.andar = (uint8_t)(salao < DES_ANDARES ? salao : DES_ANDARES - 1);
            run.lojas = (uint8_t)i;
            for (int k = 0; k <= run.andar; k++) {
                run.lojas++;
                monta_vitrine();
                b[nb++] = run.vitrine[0];
                b[nb++] = run.vitrine[1];
            }
            v += duelo(b, nb, a < 0 ? 0 : 1u << a, nivel, salao, mesa, &mg);
            m += mg;
        }
        printf("AMUL %-18s %-8s %5.1f%% %6.1f\n", a < 0 ? "(nenhum)" : AMULETO[a].nome, a < 0 ? "" : nome_rar_am(a),
               100.0 * v / n, (double)m / n);
        fflush(stdout);
    }
}

// O valor medio dos 4 melhores de uma mao de 7: o que a sequencia costuma levar.
static float melhores4(const peca_t *col, int n)
{
    float s = 0;
    for (int a = 0; a < 200; a++) {
        peca_t m[MAX_COL];
        memcpy(m, col, sizeof(peca_t) * (size_t)n);
        int q = n < MAO_NORMAL ? n : MAO_NORMAL;
        float v[MAO_NORMAL];
        for (int i = 0; i < q; i++) {
            int k = i + (int)(rnd_t() % (uint32_t)(n - i));
            peca_t t = m[i]; m[i] = m[k]; m[k] = t;
            v[i] = media(m[i]);
        }
        for (int i = 0; i < q; i++)
            for (int k = i + 1; k < q; k++) if (v[k] > v[i]) { float t = v[i]; v[i] = v[k]; v[k] = t; }
        for (int i = 0; i < 4 && i < q; i++) s += v[i];
    }
    return s / 200;
}

// ---- A progressao dos oponentes ----
// Num nivel, mesa a mesa da run, como o jogo monta (gera_andar,
// monta_oponente, nivel_mesa, trunfo_ativo, virada_de): a IA (0 a simples,
// 1 a media, 2 a forte); o trunfo (os comuns sao sorteados por run: em
// quantas amostras o dele ja vale); a virada da 5a rodada dos chefes (0
// nenhuma, 1, 2 a forte; a do Coringa: o seu dado anulado some); o amuleto
// a mais da Cobertura; e a bolsa media - quantos dados, os especiais por
// raridade e o valor dos 4 melhores de uma mao (a media da IA por dado).
static void inimigos(int nivel, int amostras)
{
    perfil_teste(nivel, false);
    peca_t b[MAX_COL];
    int nb = bolsa_inicio(b);
    printf("NIVEL %d (%s): %d moedas no comeco; a sua bolsa: %d dados, 4 melhores %.1f\n", nivel, NOME_DIF[nivel],
           moedas_iniciais(nivel), nb, melhores4(b, nb));
    printf("INIM %-18s %4s %6s %6s %7s %6s %9s  %-22s %6s\n", "mesa", "IA", "trunfo", "virada", "amuleto", "dados",
           "especiais", "(com inc rar len)", "4melh");
    int saloes = nivel >= CORINGA_DESDE ? DES_ANDARES + 1 : DES_ANDARES;
    for (int a = 0; a < saloes; a++)
        for (int t = 0; t < (a == DES_ANDARES ? 1 : DES_MESAS); t++) {
            double sn = 0, sm = 0, ia = 0, tr = 0, vi = 0, am = 0, rar[4] = { 0 };
            for (int i = 0; i < amostras; i++) {
                memset(&run, 0, sizeof run);
                run.semente = rnd_t() | 1;
                run.dificuldade = (uint8_t)nivel;
                run.andar = (uint8_t)a;
                if (a == DES_ANDARES) gera_salao_final(); else gera_andar();
                const no_t *no = &run.no[t];
                peca_t col[MAX_COL];
                int n = monta_oponente(no, a, col);
                for (int k = 0; k < n; k++) {
                    int r = TIPO[col[k].tipo].raridade;
                    if (!so_numero(col[k].tipo) && r >= 0 && r < 4) rar[r]++;
                }
                sn += n;
                sm += melhores4(col, n);
                ia += nivel_mesa(no);
                tr += trunfo_ativo(no->perso);
                vi += no->perso == PERSO_CORINGA ? nivel >= CORINGA_SOME : virada_de(no->perso);
                am += nivel >= N_NIVEIS && chefe_idx(no->perso) != 2;   // na Cobertura, todos menos o Barao
            }
            double q = amostras, esp = rar[0] + rar[1] + rar[2] + rar[3];
            char r4[40];
            snprintf(r4, sizeof r4, "(%.1f %.1f %.1f %.1f)", rar[0] / q, rar[1] / q, rar[2] / q, rar[3] / q);
            printf("INIM %-18s %4.1f %5.0f%% %6.1f %6.0f%% %6.1f %9.1f  %-22s %6.1f\n", nome_mesa(mesa_idx(a, t)), ia / q,
                   100 * tr / q, vi / q, 100 * am / q, sn / q, esp / q, r4, sm / q);
        }
}

// Sair no meio (ESC ESC) conta como derrota: a run acaba ali, e continuar
// nao recomeca a mesa. (Antes recomecava a mesma partida, com as mesmas maos.)
static void recomeca(void)
{
    perfil_teste(0, false);
    modo = M_UM;
    dado_inicia(1);
    des_nova_run();
    while (fase == F_RAMULETO) { cursor = 0; trata_tecla(KEY_ENTER); }
    if (fase == F_RLOJA) trata_tecla(KEY_TAB);
    trata_tecla(KEY_ENTER);
    while (!(fase == F_RESULTADO && rodada == 3)) partida_passo();
    int f0 = J[0].fichas, f1 = J[1].fichas;
    esc_esc();
    printf("ESC ESC na rodada 3 (%d x %d fichas): %s, run guardada %s\n", f0, f1,
           fase == F_FIM && venc_partida == 1 ? "a partida conta como perdida" : "a partida seguiu",
           des_tem_run() ? "sim" : "nao");
}

// A mesa pedida existe? Salao 0 a 2 com as mesas 0 a 2 (2: o chefe); o 3 e o
// do Coringa, so do Nivel 5, com uma mesa so.
static bool mesa_existe(int nivel, int salao, int *mesa)
{
    if (nivel < 0 || nivel > N_NIVEIS || salao < 0 || salao > DES_ANDARES) return false;
    if (salao == DES_ANDARES) { *mesa = 0; return nivel >= CORINGA_DESDE; }
    return *mesa >= 0 && *mesa < DES_MESAS;
}

int main(int argc, char **argv)
{
    #define ARG(i, d) (argc > (i) ? atoi(argv[i]) : (d))
    ia_esforco = getenv("ESFORCO") ? atoi(getenv("ESFORCO")) : 1;
    adesivos_robo = getenv("ADESIVOS") ? atoi(getenv("ADESIVOS")) : 1;
    natal_forcado = getenv("NATAL") ? atoi(getenv("NATAL")) : 0;   // fora de dezembro, qualquer que seja a data
    const char *m = argc > 1 ? argv[1] : "";
    if (!strcmp(m, "curva")) {
        int nivel = ARG(3, 0);
        if (nivel < 0 || nivel > N_NIVEIS) { printf("nivel 0 (Normal) a %d\n", N_NIVEIS); return 1; }
        ia_jog = ARG(4, 2);
        jeito_aposta = ARG(5, 0);
        jeito_compra = ARG(6, 0);
        recomecos = ARG(7, 0);
        curva(ARG(2, 100), nivel, ARG(8, 0), (uint32_t)ARG(9, 1) * 2654435761u | 1);
    } else if (!strcmp(m, "dados") || !strcmp(m, "amuletos")) {
        int nivel = ARG(3, 4), salao = ARG(4, 2) - 1, mesa = ARG(5, 2) - 1;
        if (!mesa_existe(nivel, salao, &mesa)) {
            printf("salao 1 a 3 (4: o do Coringa, do Nivel %d) e mesa 1 a %d (%d: o chefe)\n", CORINGA_DESDE, DES_MESAS, DES_MESAS);
            return 1;
        }
        if (m[0] == 'd') dados(ARG(2, 100), nivel, salao, mesa, ARG(6, 2), ARG(7, -1), ARG(8, D_N));
        else amuletos(ARG(2, 100), nivel, salao, mesa, ARG(6, -1), ARG(7, A_N));
    } else if (!strcmp(m, "inimigos") || !strcmp(m, "bolsas")) {
        int nivel = ARG(2, -1), amostras = ARG(3, 200);
        if (nivel > N_NIVEIS) { printf("nivel 0 (Normal) a %d\n", N_NIVEIS); return 1; }
        for (int n = nivel < 0 ? 0 : nivel; n <= (nivel < 0 ? N_NIVEIS : nivel); n++) inimigos(n, amostras);
    } else if (!strcmp(m, "recomeca")) {
        recomeca();
    } else {
        printf("uso: balanco curva|inimigos|dados|amuletos|recomeca ... (veja o comeco de tests/balanco.c)\n");
        return 1;
    }
    return 0;
}
