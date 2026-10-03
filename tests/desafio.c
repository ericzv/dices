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
void som_giro(bool on) { (void)on; }

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

    int jog[DES_ANDARES][4] = { { 0 } }, ven[DES_ANDARES][4] = { { 0 } };
    int vitorias = 0, chegou[DES_ANDARES * DES_PASSOS + 1] = { 0 }, partidas = 0, ganhas = 0;
    long soma_dados = 0, soma_moedas = 0, soma_compras = 0, amuletos_tidos = 0;
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
        CONFERE(fase == F_RAMULETO && run.moedas == (dific >= 4 ? 30 : DES_MOEDAS) && run.n_col == 8
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
            CONFERE(nivel_do_dado(run.col[i].tipo) <= dific, "dado %d ainda fechado", run.col[i].tipo);
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
    printf("amuletos por run: %.1f\n", (double)amuletos_tidos / runs);
    printf("onde parou (mesa 1..%d, %d = venceu):", DES_ANDARES * DES_PASSOS, DES_ANDARES * DES_PASSOS);
    for (int i = 0; i <= DES_ANDARES * DES_PASSOS; i++) printf(" %d", chegou[i]);
    putchar('\n');
    return 0;
}
