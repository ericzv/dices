// Teste do Modo Desafiante: joga runs inteiras com a IA no lugar da pessoa
// (escolhe caminho, compra, escolhe premio) e confere que a run anda do
// comeco ao fim sem sair do trilho. Tambem mede o quanto e dificil.
//
//   desafio [runs] [nivel da IA do jogador 0..2] [esforco 0|1] [caminho]
//
// caminho: 0 ao acaso, 1 prefere mesas faceis, 2 prefere dificeis.
#include <stdio.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
static int64_t relogio = 1;
int64_t esp_timer_get_time(void) { return relogio; }
void som_toca(int s) { (void)s; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }
void som_rufo_moeda(void) {}
void som_giro(bool on) { (void)on; }
void som_suspense(bool on) { (void)on; }

#include "salva_mem.h"

static uint32_t est = 99991;
static uint32_t rnd_t(void) { est ^= est << 13; est ^= est >> 17; est ^= est << 5; return est; }

static int falhas;
#define CONFERE(c, ...) do { if (!(c)) { printf("FALHOU: "); printf(__VA_ARGS__); putchar('\n'); falhas++; } } while (0)

int main(int argc, char **argv)
{
    int runs = argc > 1 ? atoi(argv[1]) : 6;
    int nivel = argc > 2 ? atoi(argv[2]) : 2;
    ia_esforco = argc > 3 ? atoi(argv[3]) : 0;
    int estrategia = argc > 4 ? atoi(argv[4]) : 0;
    int dific = argc > 5 ? atoi(argv[5]) : 0;
    if (dific < 0 || dific > N_NIVEIS) dific = 0;

    // Historico: conta so a pessoa contra a maquina, no perfil escolhido;
    // grava, le de volta, recusa o salvamento estragado e some com o perfil.
    {
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[0].nome, sizeof perfis[0].nome, "HIST");
        perfil_atual = 0;
        perfis_lidos = true;
        hist_de = -1;
        modo = M_UM; cpu = 2;
        memset(F, 0, sizeof F);
        F[0].tipo[0] = D_D6; F[0].tipo[1] = D_D8; F[0].n = F[0].lancados = 2;
        F[0].est[0] = V_VALIDO; F[0].est[1] = V_ANULADO;
        F[0].total = 37; venc_mao = 0;
        hist_rodada(25);
        F[0].total = 4; venc_mao = 1;
        hist_rodada(0);
        hist_lance(0, true, 2);
        hist_lance(1, true, 2);                  // o lance do rival nao conta
        hist_partida(0);
        hist_partida(1);
        modo = M_DOIS; cpu = 0;                  // duas pessoas: nao conta
        hist_rodada(99);
        hist_partida(0);
        CONFERE(hist.rodadas == 2 && hist.rodadas_venc == 1 && hist.maior_rodada == 37 && hist.maior_pote == 25,
                "historico das rodadas: %u %u %d %d", (unsigned)hist.rodadas, (unsigned)hist.rodadas_venc,
                hist.maior_rodada, hist.maior_pote);
        CONFERE(hist.lancou[D_D6] == 2 && hist.valeu[D_D6] == 2 && hist.ganhou[D_D6] == 1
                && hist.lancou[D_D8] == 2 && hist.valeu[D_D8] == 0 && hist.ganhou[D_D8] == 1,
                "historico por dado errado");
        CONFERE(hist.filas_cheias == 1 && hist.horas == 1 && hist.horas_venc == 1 && hist.partidas == 2
                && hist.partidas_venc == 1, "historico dos lances e partidas errado");
        memset(&run, 0, sizeof run);
        run.dificuldade = 2; run.n_col = 3; run.moedas = 123; run.amuletos = 1u << A_LUVA;
        run.col[0] = (peca_t){ D_D6, 0 }; run.col[1] = (peca_t){ D_LASER, 0 }; run.col[2] = (peca_t){ D_D20, 0 };
        hist_run_venceu();
        CONFERE(hist.camp_tem && hist.camp_dif == 2 && hist.camp_n == 3 && hist.camp_col[1] == D_LASER
                && hist.camp_moedas == 123 && (hist.camp_amul >> A_LUVA & 1) && hist.runs_venc[2] == 1,
                "run campea nao ficou guardada");
        run.dificuldade = 1; run.moedas = 999;   // uma run mais baixa nao toma o lugar
        hist_run_venceu();
        CONFERE(hist.camp_dif == 2 && hist.camp_moedas == 123 && hist.runs_venc[1] == 1, "run campea trocada");
        hist_t antes = hist;
        hist_de = -1;
        hist_carrega(0);
        CONFERE(!memcmp(&antes, &hist, sizeof hist), "historico nao voltou igual do salvamento");
        salva_grava("hist1", "HIS1zz");
        hist_de = -1;
        hist_carrega(0);
        CONFERE(hist.rodadas == 0 && !hist.camp_tem, "salvamento estragado devia comecar do zero");
        hist = antes; hist_de = 0;
        hist_grava();
        hist_apaga(0);
        hist_carrega(0);
        CONFERE(hist.rodadas == 0, "apagar o perfil devia levar o historico");
        hist_de = -1;
    }

    // Estilo: so o que o andar ja abriu vale; o resto volta ao padrao. Os
    // oponentes tem o material do nivel; os chefes, obsidiana.
    {
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[0].nome, sizeof perfis[0].nome, "ESTILO");
        perfil_atual = 0;
        perfis_lidos = true;
        int f, m, s;
        estilo_le(0, &f, &m, &s);
        CONFERE(f == FT_VERDE && m == MAT_MARFIM && s == MS_NOGUEIRA, "estilo novo devia ser o padrao");
        estilo_grava(0, FT_VINHO, MAT_JADE, MS_MOGNO);
        estilo_le(0, &f, &m, &s);
        CONFERE(f == FT_VERDE && m == MAT_MARFIM && s == MS_NOGUEIRA, "estilo fechado nao devia valer");
        perfis[0].venceu[0] = 1;                 // venceu o Barao no terreo: abre o Vinho (e o Osso)
        estilo_le(0, &f, &m, &s);
        CONFERE(f == FT_VINHO && m == MAT_MARFIM && s == MS_NOGUEIRA, "o Vinho devia abrir no terreo");
        perfis[0].venceu[1] = perfis[0].venceu[2] = 1;
        estilo_le(0, &f, &m, &s);
        CONFERE(f == FT_VINHO && m == MAT_JADE && s == MS_MOGNO, "Jade e Mogno deviam abrir");
        estilo_do_perfil();
        CONFERE(estilo_feltro == FT_VINHO && estilo_mesa == MS_MOGNO && estilo_material_seu() == MAT_JADE
                && C_FELTRO == FELTROS[FT_VINHO].tons[3], "o estilo nao foi para a mesa");
        baixo = 0;
        modo = M_UM; nivel_jog[1] = 2;
        materiais_da_partida(material_rapida());
        CONFERE(material_j[0] == MAT_JADE && material_j[1] == MAT_JADE, "o computador dificil devia ser de jade");
        nivel_jog[1] = 1;
        materiais_da_partida(material_rapida());
        CONFERE(material_j[1] == MAT_OSSO, "o computador medio devia ser de osso");
        modo = M_DOIS;
        materiais_da_partida(material_rapida());
        CONFERE(material_j[1] == MAT_MARFIM, "duas pessoas: o outro lado devia ser de marfim");
        char l[96];
        CONFERE(estilo_do_andar(5, l, sizeof l) == 1 && strstr(l, "Obsidiana"), "a cobertura devia abrir a obsidiana");
        perfil_atual = -1;
        estilo_do_perfil();                      // sem perfil: o padrao
        CONFERE(estilo_feltro == FT_VERDE && estilo_material_seu() == MAT_MARFIM, "sem perfil devia voltar ao padrao");
        modo = M_UM;
    }

    int jog[DES_ANDARES][4] = { { 0 } }, ven[DES_ANDARES][4] = { { 0 } };
    int vitorias = 0, chegou[DES_ANDARES * DES_PASSOS + 1] = { 0 }, partidas = 0, ganhas = 0;
    long soma_dados = 0, soma_moedas = 0, soma_compras = 0, amuletos_tidos = 0, eventos_vistos = 0;
    for (int r = 0; r < runs && !falhas; r++) {
        relogio = 7919LL * (r + 1) * 1000003LL;
        modo = M_UM;
        dado_inicia(r);
        // Um perfil com a dificuldade pedida aberta (e o que vem com ela).
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[0].nome, sizeof perfis[0].nome, "TESTE");
        perfis[0].nivel = (uint8_t)dific;
        perfil_atual = 0;
        perfis_lidos = true;
        des_dif = dific;
        des_nova_run();
        CONFERE(fase == F_RAMULETO && run.moedas == DES_MOEDAS + DES_MOEDAS_ANDAR * dific && run.n_col == 8
                && !run_amuletos() && run.dificuldade == dific && perfis[0].runs == 1, "run nova: fase %d", fase);
        long passos = 0;
        int compras = 0;
        while (fase != F_RFIM && !falhas) {
            if (++passos > 3000000) { CONFERE(0, "run %d parada na fase %d", r, fase); break; }
            relogio += 16667;
            switch (fase) {
            case F_RAMULETO: {               // um amuleto ao acaso entre os oferecidos
                uint32_t antes = run_amuletos();
                cursor = (int)(rnd_t() % 3u);
                if (run.am_oferta[cursor] >= A_N) cursor = 0;
                trata_tecla(KEY_ENTER);
                CONFERE(am_conta(run_amuletos()) == am_conta(antes) + 1, "amuleto nao entrou");
                for (int a = 0; a < A_N; a++)
                    CONFERE(!(run_amuletos() >> a & 1) || NIVEL_AMULETO[a] <= dific, "amuleto %d ainda fechado", a);
                amuletos_tidos += am_conta(run_amuletos()) - am_conta(antes);
                break;
            }
            case F_RLOJA: {                  // compra o de maior raridade que couber
                // Com folga de moedas, leva o amuleto da loja.
                if (run.am_loja < A_N && !run.am_levou && run.moedas >= preco_am() + 30) {
                    cursor = N_LOJA;
                    trata_tecla(KEY_ENTER);
                    CONFERE(run.am_levou && (run_amuletos() >> run.am_loja & 1), "amuleto da loja nao entrou");
                    amuletos_tidos++;
                    break;
                }
                int b = -1;
                for (int i = 0; i < N_LOJA; i++) {
                    if (run.levou[i] || run.moedas < preco_run(run.loja[i].tipo)) continue;
                    if (b < 0 || TIPO[run.loja[i].tipo].raridade > TIPO[run.loja[b].tipo].raridade) b = i;
                }
                if (b >= 0) { cursor = b; trata_tecla(KEY_ENTER); compras++; }
                else trata_tecla(KEY_TAB);
                break;
            }
            case F_MAPA: {
                int opc[3], n = 0;
                for (int l = 0; l < 3; l++) if (escolhivel(l)) opc[n++] = l;
                CONFERE(n > 0, "mapa sem saida: andar %d passo %d", run.andar, run.passo);
                if (!n) break;
                int l = opc[rnd_t() % (uint32_t)n];
                for (int i = 0; i < n && estrategia; i++) {
                    int t = run.no[run.passo][opc[i]].tipo, tl = run.no[run.passo][l].tipo;
                    if (t == NO_LOJA) continue;
                    if (tl == NO_LOJA || (estrategia == 1 ? t < tl : t > tl)) l = opc[i];
                }
                // Com moedas para um dado bom, passa na loja.
                for (int i = 0; i < n; i++)
                    if (run.no[run.passo][opc[i]].tipo == NO_LOJA && run.moedas >= 60) l = opc[i];
                des_cur = l;
                trata_tecla(KEY_ENTER);
                if (fase == F_ORDEM || fase == F_JOGA) partidas++;
                if (fase == F_REVENTO) eventos_vistos++;
                break;
            }
            case F_PREMIO:
                if (t_tira > 0) { dado_passo(1.0f / 60); break; }
                vez = 0;
                if (removendo) {                 // tira a moeda, depois o D4
                    int m = -1;
                    for (int i = 0; i < J[0].n_col && m < 0; i++) if (J[0].col[i].tipo == D_D2) m = i;
                    for (int i = 0; i < J[0].n_col && m < 0; i++) if (J[0].col[i].tipo == D_D4) m = i;
                    cursor = m < 0 ? 0 : m;
                    trata_tecla(KEY_ENTER);
                    break;
                }
                {
                    bool fraco = false;
                    for (int i = 0; i < J[0].n_col; i++) fraco |= J[0].col[i].tipo == D_D2 || J[0].col[i].tipo == D_D4;
                    bool so_comum = true;
                    for (int i = 0; i < 3; i++) so_comum &= TIPO[oferta[i].tipo].raridade == RAR_COMUM;
                    if (fraco && J[0].n_col >= 11 && so_comum) { cursor = 3; trata_tecla(KEY_ENTER); break; }
                }
                cursor = ia_premio();
                trata_tecla(KEY_ENTER);
                break;
            case F_FIM: {
                const no_t *no = &run.no[run.passo][run.caminho[run.passo]];
                int tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
                jog[run.andar][tier]++;
                ven[run.andar][tier] += venc_partida == 0;
                if (venc_partida == 0) ganhas++;
                trata_tecla(KEY_ENTER);
                break;
            }
            case F_RESULTADO:
                trata_tecla(KEY_ENTER);
                break;
            case F_REVENTO: {                // evento: uma escolha ao acaso entre as que valem
                if (ev_girando()) { dado_passo(1.0f / 60); break; }
                if (ev_passo == 2) { trata_tecla(KEY_ENTER); break; }
                if (ev_passo == 1) {
                    cursor = ev_n ? (int)(rnd_t() % (uint32_t)ev_n) : 0;
                    int antes_col = run.n_col;
                    trata_tecla(ev_acao == EA_CAPELA && run.tirados ? KEY_BKSP : KEY_ENTER);
                    CONFERE(run.n_col >= N_FILA && run.n_col <= MAX_COL, "evento deixou a bolsa com %d", run.n_col);
                    (void)antes_col;
                    break;
                }
                ev_op_t op[6];
                int n = ev_opcoes(op), ok[6], nok = 0;
                for (int i = 0; i < n; i++) if (op[i].ok) ok[nok++] = i;
                CONFERE(nok > 0, "evento sem saida");
                cursor = ok[rnd_t() % (uint32_t)nok];
                if (ev_id == EVT_NIQUEL && run.puxadas >= 4) cursor = n - 1;   // nao fica la para sempre
                int moedas_antes = run.moedas;
                trata_tecla(KEY_ENTER);
                CONFERE(run.moedas >= 0 && run.moedas <= moedas_antes + EVT_PREMIO_NIQUEL, "moedas do evento: %d", run.moedas);
                break;
            }
            case F_RBOLSA:
                trata_tecla(KEY_TAB);
                break;
            default:                         // partida: a IA joga dos dois lados
                cpu = 3;
                nivel_jog[0] = nivel;
                ousadia_jog[0] = 0;
                dado_passo(1.0f / 60);
                if (modo == M_DESAFIO && fase != F_FIM && fase != F_PREMIO)
                    CONFERE(J[0].fichas >= 0 && J[1].fichas >= 0 && rodada <= DES_RODADAS + DES_EXTRAS + 1,
                            "partida fora do trilho: fichas %d/%d rodada %d", J[0].fichas, J[1].fichas, rodada);
                break;
            }
            // Salvar e voltar tem que dar no mesmo lugar.
            if (fase == F_MAPA && passos % 997 == 0) {
                run_t antes = run, lido;
                CONFERE(des_carrega(&lido) && !memcmp(&antes, &lido, sizeof antes), "salvamento nao confere");
            }
        }
        for (int i = 0; i < run.n_col; i++)
            CONFERE(dado_liberado(run.col[i].tipo), "dado %d ainda fechado", run.col[i].tipo);
        if (des_venceu_run) {
            vitorias++;
            CONFERE(perfis[0].venceu[dific] == 1 && perfis[0].nivel == (dific < N_NIVEIS ? dific + 1 : dific),
                    "vitoria nao abriu a dificuldade seguinte");
        }
        int ate = des_venceu_run ? DES_ANDARES * DES_PASSOS : run.andar * DES_PASSOS + run.passo;
        chegou[ate]++;
        soma_dados += run.n_col;
        soma_moedas += run.moedas;
        soma_compras += compras;
        CONFERE(!des_tem_run(), "run acabada ainda salva");
    }
    if (falhas) return 1;
    printf("%d runs (IA do jogador nivel %d, caminho %d, dificuldade %s): %d vencidas (%.0f%%)\n", runs, nivel,
           estrategia, NOME_DIF[dific], vitorias, 100.0 * vitorias / runs);
    printf("partidas %d, vencidas %d; media ao fim: %.1f dados, %.0f moedas, %.1f compras\n", partidas, ganhas,
           (double)soma_dados / runs, (double)soma_moedas / runs, (double)soma_compras / runs);
    for (int a = 0; a < DES_ANDARES; a++) {
        printf("salao %d:", a + 1);
        for (int t = 0; t < 4; t++) printf("  %s %d/%d", NOME_NO[t], ven[a][t], jog[a][t]);
        putchar('\n');
    }
    printf("amuletos por run: %.1f  ·  eventos por run: %.1f\n", (double)amuletos_tidos / runs,
           (double)eventos_vistos / runs);
    printf("onde parou (mesa 1..%d, %d = venceu):", DES_ANDARES * DES_PASSOS, DES_ANDARES * DES_PASSOS);
    for (int i = 0; i <= DES_ANDARES * DES_PASSOS; i++) printf(" %d", chegou[i]);
    putchar('\n');
    return 0;
}
