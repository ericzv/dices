// ===========================================================================
// O registro das partidas (versao de teste)
// ===========================================================================
// Com "Registrar partidas" ligado no painel do som (tecla M), cada partida
// sua contra a maquina vira linhas de texto num arquivo da pasta "registros",
// ao lado do jogo. Cada linha e uma coisa que aconteceu, em JSON: a partida e
// as bolsas, cada rodada com as maos, a sequencia escolhida, cada lance, cada
// decisao de lancar ou parar (com o risco e a previa do placar; nas da
// maquina, a conta que ela fez), cada aposta, o fim da rodada com os efeitos
// e o fim da partida. No Desafiante tambem a run, a loja, os amuletos e as
// conquistas. Serve para estudar como voce joga contra a IA: onde ela erra,
// onde o jogo fica facil ou dificil demais.
//
// Um arquivo por vez que o jogo abre (com a data, a hora e a versao no
// nome), aberto so na primeira linha; a pasta fica mesmo trocando o .exe. Cada
// linha vai para o disco na hora: fechar a janela nao perde nada. Gravar nao
// muda a partida: so le o estado (as contas sao as da previa do placar e o
// risco, que nao sorteiam nada da mesa).
//
// So no computador: a plataforma diz a pasta (dado_registro_pasta). Sem ela
// (o site, os testes), nada se grava e a opcao nem aparece.
// ===========================================================================
#ifdef _WIN32
#include <direct.h>
#define REG_CRIA_PASTA(p) _mkdir(p)
#define REG_SEP '\\'
#else
#include <sys/stat.h>
#define REG_CRIA_PASTA(p) mkdir(p, 0755)
#define REG_SEP '/'
#endif

static char    reg_pasta[400];               // a pasta do jogo, com a barra no fim ("": sem registro)
static bool    reg_ligado, reg_lido;         // a opcao do painel (guardada em "registro")
static bool    reg_falhou;                   // a pasta ou o arquivo nao abriram (desligar e ligar tenta de novo)
static FILE   *reg_f;
static int64_t reg_t0;                       // quando o arquivo abriu: o "t" de cada linha conta dali, em segundos
static int     reg_n_partida;                // as partidas deste arquivo (cada linha da partida leva o numero)

void dado_registro_pasta(const char *pasta)
{
    size_t n = pasta ? strlen(pasta) : 0;
    if (!n || n + 2 >= sizeof reg_pasta) { reg_pasta[0] = 0; return; }
    memcpy(reg_pasta, pasta, n + 1);
    if (reg_pasta[n - 1] != '/' && reg_pasta[n - 1] != '\\') { reg_pasta[n] = REG_SEP; reg_pasta[n + 1] = 0; }
}

static bool reg_disponivel(void) { return reg_pasta[0] != 0; }
static bool reg_e_ligado(void)
{
    if (!reg_lido) {
        char b[8];
        int n = salva_le("registro", b, sizeof b);
        reg_ligado = n == 1 && b[0] == '1';
        reg_lido = true;
    }
    return reg_ligado;
}
static void alterna_registro(void)
{
    reg_ligado = !reg_e_ligado();
    salva_grava("registro", reg_ligado ? "1" : "0");
    if (reg_ligado) reg_falhou = false;          // ligou de novo: tenta a pasta outra vez
    som_toca(SOM_TIQUE);
}
// O que o painel mostra na linha da opcao.
static const char *reg_estado_txt(void)
{
    if (!reg_e_ligado()) return "desligado";
    return reg_falhou ? "sem acesso à pasta" : "ligado";
}

// Grava? A run do Desafiante (a loja, os amuletos, as conquistas) com a opcao
// ligada; numa partida, so a sua contra a maquina.
static bool reg_run(void) { return reg_disponivel() && reg_e_ligado() && !reg_falhou; }
static bool reg_vale(void) { return reg_run() && (modo == M_DESAFIO || modo == M_UM) && cpu == 2; }

// ---- O arquivo e a linha --------------------------------------------------------
static bool reg_abre(void)
{
    if (reg_f) return true;
    if (reg_falhou) return false;
    char dir[sizeof reg_pasta + 16], nome[sizeof dir + 64];
    snprintf(dir, sizeof dir, "%sregistros", reg_pasta);
    REG_CRIA_PASTA(dir);                         // ja existindo, nada muda
    time_t agora = time(NULL);
    struct tm z = { 0 }, *lt = localtime(&agora);
    if (!lt) lt = &z;
    snprintf(nome, sizeof nome, "%s%cpartidas_%04d-%02d-%02d_%02d-%02d-%02d_%s.txt", dir, REG_SEP,
             lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, DADO_VERSAO);
    reg_f = fopen(nome, "a");
    if (!reg_f) { reg_falhou = true; return false; }
    reg_t0 = esp_timer_get_time();
    fprintf(reg_f, "{\"ev\":\"sessao\",\"t\":0.0,\"versao\":\"%s\",\"data\":\"%04d-%02d-%02d %02d:%02d:%02d\"}\n", DADO_VERSAO,
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    fflush(reg_f);
    return true;
}

static char   reg_b[16384];                  // a linha sendo montada
static size_t reg_bn;
static bool   reg_cheia;                     // nao coube: a linha sai sem o resto, mas fechada

static void reg_add(const char *fmt, ...)
{
    if (reg_cheia) return;
    size_t livre = sizeof reg_b - 8 - reg_bn;    // sobra sempre o fecho da linha
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(reg_b + reg_bn, livre, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= livre) { reg_b[reg_bn] = 0; reg_cheia = true; return; }
    reg_bn += (size_t)n;
}
// Um texto entre aspas, com as aspas, a barra e os controles escapados (o
// resto, acentos inclusive, vai como esta).
static void reg_txt(const char *s)
{
    char q[300];
    size_t n = 0;
    q[n++] = '"';
    for (; s && *s && n < sizeof q - 8; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { q[n++] = '\\'; q[n++] = (char)c; }
        else if (c < 0x20) n += (size_t)snprintf(q + n, sizeof q - n, "\\u%04x", c);
        else q[n++] = (char)c;
    }
    q[n++] = '"';
    q[n] = 0;
    reg_add("%s", q);
}
static void reg_k(const char *k)                       { reg_add(",\"%s\":", k); }
static void reg_ki(const char *k, int v)               { reg_add(",\"%s\":%d", k, v); }
static void reg_kf(const char *k, float v)             { reg_add(",\"%s\":%.3f", k, (double)v); }
static void reg_kb(const char *k, bool v)              { reg_add(",\"%s\":%s", k, v ? "true" : "false"); }
static void reg_ks(const char *k, const char *s)       { reg_k(k); reg_txt(s); }
static void reg_par(const char *k, int a, int b)       { reg_add(",\"%s\":[%d,%d]", k, a, b); }

static bool reg_comeca(const char *ev)
{
    if (!reg_abre()) return false;
    reg_bn = 0;
    reg_b[0] = 0;
    reg_cheia = false;
    reg_add("{\"ev\":\"%s\",\"t\":%.1f", ev, (double)(esp_timer_get_time() - reg_t0) / 1e6);
    if (reg_n_partida && reg_vale()) reg_ki("partida", reg_n_partida);   // so as linhas de dentro da partida
    return true;
}
static void reg_fim(void)
{
    if (reg_cheia) { reg_bn = strlen(reg_b); reg_cheia = false; reg_kb("cortada", true); }
    reg_add("}\n");
    if (fputs(reg_b, reg_f) < 0 || fflush(reg_f) != 0) {   // o disco recusou: para de gravar
        fclose(reg_f);
        reg_f = NULL;
        reg_falhou = true;
    }
}

// ---- As pecas -------------------------------------------------------------------
static const char *reg_quem(int j) { return j == 0 ? "jogador" : "ia"; }
static const char *const REG_EST[4] = { "espera", "valido", "anulado", "roubado" };
static const char *reg_est(int e) { return e >= 0 && e < 4 ? REG_EST[e] : "?"; }

// "Nome", "Nome(fixo)" e os adesivos por lado: "D6 2:Trevo 5:Chumbo".
static void reg_peca_txt(char *s, size_t n, int t, int fixo, const peca_t *p)
{
    size_t k = 0;
    int w = snprintf(s, n, "%s", t >= 0 && t < D_N ? TIPO[t].nome : "?");
    k = w > 0 && (size_t)w < n ? (size_t)w : n - 1;
    if (fixo && k < n - 8) {
        w = snprintf(s + k, n - k, "(%d)", t == D_QUEBRADO ? (int)(int8_t)fixo : fixo);
        k += w > 0 && (size_t)w < n - k ? (size_t)w : 0;
    }
    for (int l = 0; p && l < AD_LADOS && k < n - 24; l++) {
        int a = p->ad[l];
        if (a <= AD_NADA || a >= AD_N) continue;
        w = snprintf(s + k, n - k, " %d:%s", l + 1, ADESIVO[a].nome);
        k += w > 0 && (size_t)w < n - k ? (size_t)w : 0;
    }
}
static void reg_peca(const peca_t *p)
{
    char s[200];
    reg_peca_txt(s, sizeof s, p->tipo, p->fixo, p);
    reg_txt(s);
}
static void reg_lista(const peca_t *col, int n)
{
    reg_add("[");
    for (int i = 0; i < n; i++) { if (i) reg_add(","); reg_peca(&col[i]); }
    reg_add("]");
}
static void reg_mao(int j)
{
    const jogador_t *p = &J[j];
    reg_add("[");
    for (int i = 0; i < p->n_mao; i++) {
        int c = p->mao[i];
        if (i) reg_add(",");
        if (c >= 0 && c < p->n_col) reg_peca(&p->col[c]); else reg_txt("?");
    }
    reg_add("]");
}
// O dado k da fila de j, como esta na mesa (o tipo da fila: o Presente aberto, a carta virada).
static void reg_dado_fila(int j, int k)
{
    char s[200];
    reg_peca_txt(s, sizeof s, F[j].tipo[k], F[j].fixo[k], peca_fila(j, k));
    reg_txt(s);
}
static void reg_fila_dados(int j)
{
    reg_add("[");
    for (int k = 0; k < F[j].n && k < N_FILA; k++) { if (k) reg_add(","); reg_dado_fila(j, k); }
    reg_add("]");
}
// Os lancados de j: o dado, o numero e como ficou.
static void reg_fila_lances(int j, const uint8_t *est)
{
    reg_add("[");
    for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
        if (k) reg_add(",");
        reg_add("{\"dado\":");
        reg_dado_fila(j, k);
        reg_add(",\"valor\":%d,\"est\":\"%s\"}", F[j].valor[k], reg_est(est[k]));
    }
    reg_add("]");
}
static void reg_amuletos(int j)
{
    reg_add("[");
    bool um = false;
    for (int a = 0; a < A_N; a++)
        if (tem_am(j, a)) { if (um) reg_add(","); reg_txt(AMULETO[a].nome); um = true; }
    reg_add("]");
}

// ---- A partida ------------------------------------------------------------------
static void reg_partida_comeca(void)
{
    reg_n_partida++;
    if (!reg_comeca("partida")) return;
    reg_ks("modo", modo == M_DESAFIO ? "desafiante" : "rapida");
    reg_ks("oponente", J[1].nome ? J[1].nome : "?");
    reg_ki("ia_nivel", nivel_jog[1]);
    if (modo == M_DESAFIO) {
        const no_t *no = &run.no[run.passo];
        static const char *const NO_NOME[4] = { "facil", "medio", "dificil", "chefe" };
        reg_ks("mesa", no->tipo < 4 ? NO_NOME[no->tipo] : "?");
        reg_ks("perso", no->perso < sizeof PERSO / sizeof PERSO[0] ? PERSO[no->perso].nome : "?");
        reg_ki("salao", run.andar);
        reg_ki("passo", run.passo);
        reg_ki("dificuldade", run.dificuldade);
        reg_ks("bolsa", run.bolsa < B_N ? BOLSA[run.bolsa].nome : "?");
        reg_ki("moedas", run.moedas);
        reg_ki("vitorias_run", run.vitorias);
        reg_ki("ia_pensa", pensa_jog[1]);
        reg_k("amuletos_jogador"); reg_amuletos(0);
        reg_k("amuletos_ia"); reg_amuletos(1);
    }
    reg_par("fichas", J[0].fichas, J[1].fichas);
    reg_k("bolsa_jogador"); reg_lista(J[0].col, J[0].n_col);
    reg_k("bolsa_ia"); reg_lista(J[1].col, J[1].n_col);
    reg_fim();
}

// O comeco de cada rodada (fim de nova_rodada): quem abre, a entrada, as maos.
static void reg_rodada_comeca(void)
{
    if (!reg_vale()) return;
    if (rodada == 1) reg_partida_comeca();
    if (!reg_comeca("rodada")) return;
    reg_ki("rodada", rodada);
    reg_ks("abre", reg_quem(primeiro));
    reg_ki("entrada", ante_da(rodada));
    reg_ki("pote", pote);
    reg_par("fichas", J[0].fichas, J[1].fichas);
    reg_k("mao_jogador"); reg_mao(0);
    reg_k("mao_ia"); reg_mao(1);
    reg_fim();
}

// A sequencia que j montou, no primeiro lance dela.
static void reg_fila(int j)
{
    if (!reg_vale() || !reg_comeca("fila")) return;
    reg_ks("quem", reg_quem(j));
    reg_kb("abre", j == primeiro);
    reg_k("dados"); reg_fila_dados(j);
    reg_fim();
}

// Lancar ou parar (a pessoa ou a maquina, pela mesma tecla): o risco do
// proximo dado, a previa do placar se parasse agora e, da maquina, a conta.
static void reg_decide(bool lanca)
{
    if (!reg_vale()) return;
    int j = vez;
    const fila_t *f = &F[j];
    if (lanca && f->lancados == 0) reg_fila(j);
    desfecho_t d;
    calcula_desfecho(&d, false);
    if (!reg_comeca("decide")) return;
    reg_ks("quem", reg_quem(j));
    reg_ks("acao", lanca ? "lanca" : "para");
    reg_ki("lancados", f->lancados);
    reg_ki("na_fila", f->n);
    if (f->lancados < f->n) {
        reg_k("proximo"); reg_dado_fila(j, f->lancados);
        reg_ki("risco", risco(j));
    }
    reg_par("previa", d.total[0], d.total[1]);
    reg_kb("abriu", j == primeiro);
    if ((cpu >> j & 1) && ia_ult_parar >= 0) { reg_kf("ia_parar", ia_ult_parar); reg_kf("ia_seguir", ia_ult_seguir); }
    reg_fim();
}

// Um dado pousou (passo.inc, depois dos adesivos do pouso).
static void reg_lance(int j, int k)
{
    if (!reg_vale() || k < 0 || k >= N_FILA || !reg_comeca("lance")) return;
    const fila_t *f = &F[j];
    reg_ks("quem", reg_quem(j));
    reg_ki("k", k);
    reg_k("dado"); reg_dado_fila(j, k);
    reg_ki("valor", f->valor[k]);
    if (f->cru[k] != f->valor[k]) reg_ki("cru", f->cru[k]);
    reg_ks("est", reg_est(f->est[k]));
    if (f->escondido[k]) reg_kb("escondido", true);
    if (veredito[0]) reg_ks("nota", veredito);
    reg_fim();
}

// A aposta, antes de valer (executa): a opcao, quanto poe e a mesa.
static void reg_aposta(int op)
{
    if (!reg_vale()) return;
    static const char *const OP_NOME[] = { "passar", "apostar10", "apostar25", "pagar", "aumentar", "correr" };
    int j = vez, valor = 0;
    switch (op) {
    case OP_APOSTAR10: valor = limite(10); break;
    case OP_APOSTAR25: valor = limite(25); break;
    case OP_PAGAR:     valor = a_pagar < J[j].fichas ? a_pagar : J[j].fichas; break;
    case OP_AUMENTAR:  valor = a_pagar + aumento_possivel(); break;
    }
    desfecho_t d;
    calcula_desfecho(&d, false);
    if (!reg_comeca("aposta")) return;
    reg_ks("quem", reg_quem(j));
    reg_ks("op", op >= 0 && op <= OP_CORRER ? OP_NOME[op] : "?");
    reg_ki("valor", valor);
    reg_ki("pote", pote);
    reg_ki("a_pagar", a_pagar);
    reg_par("fichas", J[0].fichas, J[1].fichas);
    reg_par("previa", d.total[0], d.total[1]);
    reg_kb("abriu", j == primeiro);
    if (cpu >> j & 1) {
        if (ia_ult_chance >= 0) reg_kf("ia_chance", ia_ult_chance);
        if (nivel_jog[j & 1] >= 3 && ms_ult_n > 0) {      // a mestre: o valor de cada opcao
            reg_add(",\"ia_opcoes\":{");
            for (int a = 0; a < ms_ult_n && a < 3; a++)
                reg_add("%s\"%s\":%.3f", a ? "," : "", ms_ult_ops[a] >= 0 && ms_ult_ops[a] <= OP_CORRER ? OP_NOME[ms_ult_ops[a]] : "?",
                        (double)ms_ult_q[a]);
            reg_add("}");
        }
    }
    reg_fim();
}

// O fim da rodada: no fim de tudo (conclui_rodada) ou porque alguem correu.
static void reg_rodada_fim(bool correu)
{
    if (!reg_vale() || !reg_comeca("fim_rodada")) return;
    reg_ki("rodada", rodada);
    if (correu) {
        reg_ks("vencedor", reg_quem(res_quem));
        reg_ks("correu", reg_quem(outro(res_quem)));
    } else {
        reg_ks("vencedor", venc_mao == 0 || venc_mao == 1 ? reg_quem(venc_mao) : "empate");
        reg_par("pontos", F[0].total, F[1].total);
        if (zerou >= 0) reg_ks("zerou", reg_quem(zerou));
    }
    reg_par("fichas", J[0].fichas, J[1].fichas);
    reg_k("jogador"); reg_fila_lances(0, F[0].est);
    reg_k("ia"); reg_fila_lances(1, F[1].est);
    if (!correu && n_ev) {
        reg_add(",\"efeitos\":[");
        for (int i = 0; i < n_ev; i++) { if (i) reg_add(","); reg_txt(ev[i].txt); }
        reg_add("]");
    }
    if (aviso[0]) reg_ks("aviso", aviso);
    reg_fim();
}

// O fim da partida (encerra).
static void reg_partida_fim(int vencedor)
{
    if (!reg_vale() || !reg_comeca("fim_partida")) return;
    bool saiu = modo == M_DESAFIO && des_saiu;   // voce saiu no meio: a rodada de agora nao acabou
    reg_ks("vencedor", vencedor == 0 || vencedor == 1 ? reg_quem(vencedor) : "empate");
    reg_ki("rodadas", rodada - 1);                 // as que terminaram
    reg_ks("motivo", saiu ? "abandonou" : fim_motivo == FM_RODADAS ? "rodadas" : "fichas");
    reg_par("fichas", J[0].fichas, J[1].fichas);
    if (modo == M_DESAFIO) { reg_ki("moedas", run.moedas); reg_ki("vitorias_run", run.vitorias); }
    reg_fim();
}

// ---- A run do Desafiante ------------------------------------------------------------
static void reg_run_comeca(void)
{
    if (!reg_run() || !reg_comeca("run")) return;
    reg_ki("dificuldade", run.dificuldade);
    reg_ks("bolsa", run.bolsa < B_N ? BOLSA[run.bolsa].nome : "?");
    reg_ki("moedas", run.moedas);
    reg_k("dados"); reg_lista(run.col, run.n_col);
    reg_fim();
}
static void reg_run_fim(bool venceu)
{
    if (!reg_run() || !reg_comeca("fim_run")) return;
    reg_kb("venceu", venceu);
    reg_ki("salao", run.andar);
    reg_ki("vitorias_run", run.vitorias);
    reg_ki("moedas", run.moedas);
    reg_k("dados"); reg_lista(run.col, run.n_col);
    reg_fim();
}

// A loja que abriu: a vitrine com os precos, os adesivos, o pacote e a bolsa.
static void reg_loja_abre(void)
{
    if (!reg_run() || !reg_comeca("loja")) return;
    reg_ki("salao", run.andar);
    reg_ki("passo", run.passo);
    reg_ki("moedas", run.moedas);
    reg_add(",\"vitrine\":[");
    bool um = false;
    for (int i = 0; i < n_vitrine(); i++) {
        if (um) reg_add(",");
        reg_add("{\"dado\":");
        reg_peca(&run.vitrine[i]);
        reg_add(",\"preco\":%d}", preco_run(run.vitrine[i].tipo));
        um = true;
    }
    reg_add("],\"adesivos\":[");
    for (int k = 0; k < AD_LOJA_MAX; k++) {
        int a = ad_loja(k);
        if (a <= AD_NADA || a >= AD_N) continue;
        reg_add("%s{\"adesivo\":", k ? "," : "");
        reg_txt(ADESIVO[a].nome);
        reg_add(",\"preco\":%d}", preco_adesivo(a));
    }
    reg_add("]");
    if (tem_pacote()) { reg_ks("pacote", pacote_de_adesivos() ? "adesivos" : "dados"); reg_ki("preco_pacote", preco_pacote()); }
    reg_ki("preco_troca", preco_troca());
    reg_ki("preco_tirar", preco_remove());
    reg_k("bolsa"); reg_lista(run.col, run.n_col);
    reg_fim();
}
// Uma coisa feita na loja: comprou, colou, trocou, tirou, levou do pacote, saiu.
static void reg_loja(const char *acao, const peca_t *p, int adesivo, int preco)
{
    if (!reg_run() || !reg_comeca("loja_acao")) return;
    reg_ks("acao", acao);
    if (p) { reg_k("dado"); reg_peca(p); }
    if (adesivo > AD_NADA && adesivo < AD_N) reg_ks("adesivo", ADESIVO[adesivo].nome);
    if (preco >= 0) reg_ki("preco", preco);
    reg_ki("moedas", run.moedas);
    if (!strcmp(acao, "sai")) { reg_k("bolsa"); reg_lista(run.col, run.n_col); }
    reg_fim();
}
// O amuleto que chegou, e o que mais havia para escolher.
static void reg_amuleto(int a)
{
    if (!reg_run() || !reg_comeca("amuleto")) return;
    reg_ks("levou", a >= 0 && a < A_N ? AMULETO[a].nome : "?");
    reg_add(",\"ofertas\":[");
    bool um = false;
    for (int i = 0; i < 3; i++) {
        int o = run.am_oferta[i];
        if (o >= A_N) continue;
        if (um) reg_add(",");
        reg_txt(AMULETO[o].nome);
        um = true;
    }
    reg_add("]");
    reg_fim();
}
// A conquista que abriu (cq_abre), com o motivo de agora.
static void reg_conquista(const char *item, const char *criterio, const char *motivo)
{
    if (!reg_run() || !reg_comeca("conquista")) return;
    reg_ks("item", item);
    reg_ks("criterio", criterio);
    if (motivo && motivo[0]) reg_ks("motivo", motivo);
    if (fase == F_JOGA || fase == F_VEREDITO || fase == F_EFEITOS || fase == F_RESULTADO || fase == F_ARRUMA)
        reg_ki("rodada", rodada);
    reg_fim();
}
