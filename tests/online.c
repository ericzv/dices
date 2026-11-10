// Teste do modo online: duas copias do jogo em dois processos (fork), ligadas
// por canos como os dois navegadores pelo servidor. Uma anda a 60 quadros por
// segundo; a outra varia de 30 a 144, como um monitor diferente. Cada lado
// aperta teclas ao acaso; as da vez viajam para o outro, como no navegador.
// Os marcos da partida (rede_marco: o estado das regras a cada rodada e a
// cada turno) dos dois lados tem que ser os mesmos, na mesma ordem, e a
// partida tem que terminar nos dois. E o que garante que o placar de um
// computador e o placar do outro. O numero que o jogo manda (o hash da
// partida) ja leva os adesivos, o Fermento, o ar do Balao, as bonecas e os
// lances crus; o teste confere tambem o seu, feito a parte, para pegar um
// campo que o do jogo deixe de fora.
//
// Para forcar: os dados novos nas bolsas dos dois (1: os que o online tem;
// 2: tambem os de adesivo, com adesivos colados, a forca), as teclas do
// outro chegando atrasadas ate 'atraso' quadros (em ordem, como a rede
// entrega), quadros enormes de vez em quando (a aba de lado) e as teclas
// que so mexem neste computador (velocidade, letra, detalhe, bolsa, som).
//
// E mais: os dados que esperam o fim da rodada fora da sequencia (o
// Carrossel, a Ficha Coringa, o Oculto, o Espelho, o Terremoto, o Jingle) em
// bolsas sorteadas, com adesivos sorteados (a Casca de Banana, o Chiclete e
// os que agem no fim entre eles) - dados 3; o mouse passando e clicando
// (locais 2); um lado que antes de entrar na sala jogou outra coisa neste
// computador (sujo: um, o Desafiante com amuletos; o outro, 2 jogadores); e a
// conexao caindo (quedas): as teclas do outro param de chegar por ate um
// minuto e chegam todas de uma vez, no meio do que estiver acontecendo.
// Em todo quadro, o desenho (com as previas e as dicas da interface) nao
// pode mexer no estado das regras nem gastar o sorteio da partida.
//
//   online [salas] [quadros] [dados 0|1|2|3] [atraso] [engasga 0|1] [base da semente] [locais 0|1|2]
//          [sujo 0|1] [quedas 0|1|2] [pressa: desenha 1 quadro em cada tantos e aperta mais teclas]
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
static int64_t relogio = 11;
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
static int volume_[SOM_CANAIS] = { SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS };
void som_volume(int c, int n) { if (c >= 0 && c < SOM_CANAIS) volume_[c] = n; }
int som_volume_de(int c) { return c >= 0 && c < SOM_CANAIS ? volume_[c] : 0; }
#include "salva_mem.h"

// A rede: as teclas vao pelo cano; os marcos ficam anotados.
#define MAX_MARCOS 4096
typedef struct { int32_t i; uint32_t h, x; } marco_t;
static marco_t marcos[MAX_MARCOS];
static int n_marcos, fd_in = -1, fd_out = -1, fim_f0, fim_f1;   // as fichas no ultimo marco
bool rede_disponivel(void) { return true; }
void rede_cria(void) {}
void rede_entra(const char *c) { (void)c; }
void rede_envia_tecla(int k)
{
    int32_t v = k;
    if (write(fd_out, &v, 4) != 4) { /* o outro ja terminou */ }
}
void rede_sai(void) {}
int rede_estado(void) { return REDE_JOGANDO; }
const char *rede_codigo(void) { return "TEST"; }
const char *rede_erro(void) { return ""; }
static int forca_modo;                  // 0: como o jogo; 1: os dados novos do online; 2: e os de adesivo, a forca
#define XMIX(v) (h = (h ^ (uint32_t)(v)) * 16777619u)
static void xbytes(uint32_t *hp, const void *p, size_t n)
{
    uint32_t h = *hp;
    const uint8_t *b = p;
    for (size_t i = 0; i < n; i++) XMIX(b[i]);
    *hp = h;
}
// Tudo o que as regras guardam, contado a parte do hash do jogo: os
// adesivos, o Fermento, o ar do Balao, as bonecas, os lances crus, o pavio.
static uint32_t hash_largo(void)
{
    uint32_t h = 2166136261u;
    XMIX(semente); XMIX(rodada); XMIX(vez); XMIX(pote); XMIX(primeiro);
    for (int j = 0; j < 2; j++) {
        XMIX(J[j].fichas); XMIX(J[j].n_col); XMIX(J[j].perdidas); XMIX(J[j].n_mao);
        xbytes(&h, J[j].col, sizeof(peca_t) * (size_t)J[j].n_col);
        xbytes(&h, J[j].onde, (size_t)J[j].n_col);
        xbytes(&h, J[j].pavio, sizeof J[j].pavio);
        xbytes(&h, J[j].mao, sizeof J[j].mao);
        const fila_t *f = &F[j];
        XMIX(f->n); XMIX(f->lancados); XMIX(f->pontos); XMIX(f->total); XMIX(f->carta); XMIX(f->grudado); XMIX(f->chiclete);
        for (int k = 0; k < f->n && k < N_FILA; k++) {
            XMIX(f->idx[k]); XMIX(f->tipo[k]); XMIX(f->fixo[k]); XMIX(f->cru[k]); XMIX(f->valor[k]); XMIX(f->lado[k]);
            XMIX(f->est[k]); XMIX(f->dobra[k]); XMIX(f->bonus[k]); XMIX(f->quebra[k]); XMIX(f->bonecas[k]);
            XMIX(f->abalo[k]); XMIX(f->guarda[k]); XMIX(f->presente[k]); XMIX(f->copiou[k]); XMIX(f->bomba[k]);
            XMIX(f->cinza[k]); XMIX(f->descarte[k]); XMIX(f->espera[k]); XMIX(f->ancora[k]); XMIX(f->calibre[k]);
            XMIX(f->bomba1[k]); XMIX(f->escondido[k]); XMIX(f->fruta[k]); XMIX(f->faixa[k]); XMIX(f->curinga[k]);
            XMIX(f->curinga_nada[k]); XMIX(f->gira[k]);
        }
        XMIX(J[j].embaralhou); XMIX(J[j].perdeu_ultima);
    }
    xbytes(&h, bis_fila, sizeof bis_fila);
    xbytes(&h, ad_moedas, sizeof ad_moedas);
    return h;
}
// O que apareceu na partida (para dizer o quanto o teste cobriu): nos marcos,
// os dados esperando o fim; nos efeitos do fim, o que cada um fez.
enum { ST_GIRA, ST_CURINGA, ST_ESCONDIDO, ST_ESPERA, ST_ABALO, ST_CASCA, ST_CHICLETE,
       ST_EV_TREMOR, ST_EV_CARROSSEL, ST_EV_CORINGA, ST_EV_REVELA, ST_EV_ESPELHO, ST_N };
static const char *ST_NOME[ST_N] = { "carrossel girando", "ficha coringa", "oculto", "espelho esperando", "terremoto armado",
                                     "casca na mesa", "chiclete", "ev tremor/casca", "ev carrossel", "ev coringa", "ev revela", "ev espelho" };
static int32_t st[ST_N], mexeu, mexeu_fase;
void rede_marco(int i, uint32_t h)
{
    if (n_marcos < MAX_MARCOS) marcos[n_marcos++] = (marco_t){ i, h, hash_largo() };
    fim_f0 = J[0].fichas; fim_f1 = J[1].fichas;
    for (int j = 0; j < 2; j++) {
        const fila_t *f = &F[j];
        for (int k = 0; k < f->lancados && k < N_FILA; k++) {
            st[ST_GIRA] += f->gira[k]; st[ST_CURINGA] += f->curinga[k]; st[ST_ESCONDIDO] += f->escondido[k];
            st[ST_ESPERA] += f->espera[k]; st[ST_ABALO] += f->abalo[k] != 0;
        }
        st[ST_CASCA] += cascas_de(j) > 0; st[ST_CHICLETE] += f->grudado || f->chiclete;
    }
}
const char *rede_convite(void) { return ""; }
void rede_copia_convite(void) {}
int rede_convite_copiado(void) { return 0; }

static int atraso_max, engasga, locais, sujo, quedas, pressa = 1;
static uint32_t r32(uint32_t *s) { *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s; }

// As bolsas sorteadas (as mesmas nos dois lados: saem da semente da sala):
// metade dos dados e dos que esperam o fim da rodada, o resto e qualquer um
// do jogo; e cada dado leva ate 3 adesivos, metade deles dos que agem no fim
// ou no dado do outro.
static void bolsas_sorteadas(uint32_t sem)
{
    static const uint8_t ESPERAM[] = { D_CARROSSEL, D_FICHA_CORINGA, D_OCULTO, D_ESPELHO, D_TERREMOTO, D_JINGLE,
                                       D_CARROSSEL, D_FICHA_CORINGA, D_TERREMOTO, D_OCULTO };
    static const uint8_t ADS[] = { AD_CASCA, AD_CASCA, AD_CHICLETE, AD_CHICLETE, AD_CARIMBO, AD_DUPLA, AD_IMPERMEAVEL, AD_POSTIT,
                                   AD_BOMBA, AD_REFLEXIVO, AD_HOLOGRAFICO, AD_INFECTADO, AD_BARRO, AD_MASSINHA, AD_BIS,
                                   AD_FERMENTO, AD_DINAMITE };
    uint32_t s = (sem * 2654435761u) | 1;
    for (int j = 0; j < 2; j++) {
        J[j].n_col = 0;
        for (int i = 0; i < 12; i++) {
            int t;
            if (i < 6) t = ESPERAM[r32(&s) % sizeof ESPERAM];
            else do t = (int)(r32(&s) % D_N); while (oculto(t) || cosmo_oculto(t) || adesivado_secreto(t) || aposentado(t));
            if (fora_de_epoca(t)) t = D_ESPELHO;
            peca_t p = nova_peca(t);
            for (int a = (int)(r32(&s) % 4); a > 0; a--) {
                int ad = (r32(&s) & 1) ? ADS[r32(&s) % sizeof ADS] : 1 + (int)(r32(&s) % (AD_N - 1));
                int l0 = ad_lado_min(t), l1 = ad_lado_max(t);
                int l = l0 + (int)(r32(&s) % (uint32_t)(l1 - l0 + 1));
                if (ad_a_venda(ad) && l <= AD_LADOS && ad_lado_serve(&p, l) && !ad_no_lado(&p, l) && ad_cabe_o(&p, ad)) ad_cola(&p, l, ad);
            }
            J[j].col[J[j].n_col] = p;
            J[j].onde[J[j].n_col++] = NA_BOLSA;
        }
    }
}

static const int TECLAS[] = { KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_ENTER, KEY_LEFT, KEY_RIGHT,
                              KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_TAB, KEY_BKSP, KEY_ENTER,
                              'v', 'i', 'b', 'l', 't', 'm', 'k', 'j' };   // e as teclas so deste lado

// Antes de entrar na sala, este computador jogou outra coisa: um lado, uma
// run do Desafiante (com amuletos, contra a maquina); o outro, 2 jogadores.
// Nada disso pode sobrar na partida online.
static void suja(int eu, uint32_t sem, uint32_t *rs)
{
    relogio += 1234567 * (eu + 1);
    modo = M_UM;
    dado_inicia((int)(sem % 1000) + eu);
    if (eu == 1) {
        salva_limpa_tudo();
        memset(perfis, 0, sizeof perfis);
        snprintf(perfis[0].nome, sizeof perfis[0].nome, "TESTE");
        perfis[0].nivel = 3;
        perfil_atual = 0;
        perfis_lidos = true;
        des_dif = 3;
        des_nova_run();
        run.amuletos |= 1u << A_FERRADURA | 1u << A_REDE | 1u << A_CORINGA | 1u << A_COURACA;
    } else {
        menu_tela = MT_PRINCIPAL;
        menu_executa(MN_DOIS);
    }
    int n = 3000 + (int)(r32(rs) % 9000);
    for (int q = 0; q < n; q++) {
        if (r32(rs) % 5 == 0) {
            key_event_t e = { 0 };
            e.key = TECLAS[r32(rs) % 20];
            dado_tecla(&e);
        }
        if (q % 4 == 0) dado_desenha();
        dado_passo(1 / 30.0f);
        relogio += 33333;
    }
    for (int i = 0; i < 8 && fase != F_ABERTURA; i++) trata_esc();   // sai como a pessoa sai: ESC, ESC
}

// A tecla do outro entra na fila do jogo (0: a fila esta cheia, fica para depois).
#ifdef FILA_PERDE                            // so para ver o teste falhar: como era, a tecla a mais se perdia
#define ENTREGA(k) (dado_rede_tecla(k), 1)
#else
#define ENTREGA(k) dado_rede_tecla(k)
#endif

// Um lado da sala: joga ate a partida acabar (a revanche comecar).
static bool joga(int eu, uint32_t sem, int quadros)
{
    uint32_t rs = 0x9E3779B9u ^ sem ^ (uint32_t)eu * 7919u;
    natal_forcado = forca_modo == 3 && (sem & 1) ? 2 : 0;   // a mesma epoca em qualquer dia do ano (nas sorteadas, metade no Natal)
    if (sujo) suja(eu, sem, &rs);
    dado_online_comeca(sem, eu);
    if (forca_modo == 3) bolsas_sorteadas(sem);
    else if (forca_modo) {
        static const uint8_t LEGAIS[] = { D_BALAO, D_MATRIOSKA, D_GELATINA, D_ESTILINGUE, D_CAMALEAO, D_AGNUS, D_D6, D_D4 };
        static const uint8_t TODOS[] = { D_PIAO, D_CAMELO, D_LEQUE, D_RETRATO, D_PAPAGAIO, D_COLCHA,
                                         D_BALAO, D_MATRIOSKA, D_GELATINA, D_ESTILINGUE, D_CAMALEAO, D_D6, D_D6 };
        const uint8_t *L = forca_modo == 1 ? LEGAIS : TODOS;
        int n = forca_modo == 1 ? (int)sizeof LEGAIS : (int)sizeof TODOS;
        for (int j = 0; j < 2; j++) {
            J[j].n_col = 0;
            for (int i = 0; i < n; i++) {
                peca_t p = nova_peca(L[i]);
                if (forca_modo == 2) {               // os adesivos novos e os perigosos
                    if (L[i] == D_PIAO) { ad_cola(&p, 2, AD_COLEIRA); ad_cola(&p, 5, AD_CORRENTE); }
                    if (L[i] == D_LEQUE) { ad_cola(&p, 1, AD_TIQUETE); ad_cola(&p, 6, AD_CORRENTE); }
                    if (L[i] == D_RETRATO) { ad_cola(&p, 3, AD_DUPLA); ad_cola(&p, 4, AD_CORRENTE); }
                    if (L[i] == D_PAPAGAIO) { ad_cola(&p, 1, AD_CARIMBO); ad_cola(&p, 6, AD_COLEIRA); }
                    if (L[i] == D_D6 && i == n - 2) { ad_cola(&p, 1, AD_XEROX); ad_cola(&p, 2, AD_MOLDURA); ad_cola(&p, 6, AD_RASPADINHA); }
                    if (L[i] == D_D6 && i == n - 1) { ad_cola(&p, 3, AD_BUSSOLA); ad_cola(&p, 4, AD_TIQUETE); ad_cola(&p, 5, AD_COLEIRA); }
                }
                J[j].col[J[j].n_col] = p;
                J[j].onde[J[j].n_col++] = NA_BOLSA;
            }
        }
    }
    // quedas 2: a pagina de um lado fica parada na rede logo de saida (a aba de
    // lado, a conexao caida) enquanto o outro, na loja, passeia o cursor - mais
    // de mil teclas, muito mais do que cabe na fila do jogo. Nenhuma pode sumir.
    int queda_ate = quedas == 2 && eu == 1 ? 4000 : 0, fase_ant = fase;
    for (int q = 0; q < quadros; q++) {
        int32_t v;
        static int32_t pend[8192]; static int pend_q[8192], np, ultimo_q;
        while (read(fd_in, &v, 4) == 4 && np < 8192) {
            int quando = q + (atraso_max ? (int)(r32(&rs) % (uint32_t)atraso_max) : 0);
            if (quando < ultimo_q) quando = ultimo_q;   // a rede entrega em ordem
            ultimo_q = quando;
            pend[np] = v; pend_q[np++] = quando;
        }
        // A conexao cai: nada chega por um tempo (de 5 s a um minuto), e
        // depois vem tudo de uma vez, como quando a pagina reconecta.
        if (quedas && q >= queda_ate && r32(&rs) % 2500 == 0) queda_ate = q + 300 + (int)(r32(&rs) % 3300);
        int ent = 0;
        while (q >= queda_ate && ent < np && pend_q[ent] <= q && ENTREGA(pend[ent])) ent++;   // fila cheia: fica para depois
        if (ent) { memmove(pend, pend + ent, sizeof(int32_t) * (size_t)(np - ent)); memmove(pend_q, pend_q + ent, sizeof(int) * (size_t)(np - ent)); np -= ent; }
        if (engasga && r32(&rs) % 400 == 0) {        // a aba ficou de lado: um quadro enorme
            dado_desenha(); dado_passo(0.25f + (r32(&rs) % 100) / 100.0f); relogio += 500000;
        }
        if (r32(&rs) % (pressa > 1 ? 3 : 9) == 0) {   // de vez em quando, uma tecla ao acaso
            key_event_t e = { 0 };
            e.key = TECLAS[r32(&rs) % (locais ? 20 : 12)];
            if (quedas == 2 && q < 4000) e.key = eu == 0 ? ((r32(&rs) & 1) ? KEY_LEFT : KEY_RIGHT) : 'i';
            dado_tecla(&e);
        }
        if (locais == 2 && r32(&rs) % 4 == 0) {      // o mouse: passa por cima (as dicas) e, as vezes, clica ou toca
            int x = (int)(r32(&rs) % GFX_W), y = (int)(r32(&rs) % GFX_H), acao = 0;
            uint32_t a = r32(&rs) % 24;
            if (a < 3) acao = (int)a + 1;
            for (int i = 0; acao && zonas_fase == fase && i < n_zonas; i++)   // menos no "<" de sair
                if (zonas[i].tecla == KEY_ESC && x >= zonas[i].x && x < zonas[i].x + zonas[i].w
                    && y >= zonas[i].y && y < zonas[i].y + zonas[i].h) acao = 0;
            dado_mouse(x, y, acao);
        }
        if (fase == F_EFEITOS && fase_ant != F_EFEITOS)
            for (int i = 0; i < n_ev; i++) {
                int t = ev[i].tipo;
                st[ST_EV_TREMOR] += t == EV_TREMOR; st[ST_EV_CARROSSEL] += t == EV_CARROSSEL; st[ST_EV_CORINGA] += t == EV_CORINGA;
                st[ST_EV_REVELA] += t == EV_REVELA; st[ST_EV_ESPELHO] += t == EV_ESPELHO;
            }
        fase_ant = fase;
        // O desenho (as previas, as dicas, o que a interface calcula para
        // mostrar) nao mexe nas regras nem gasta o sorteio da partida.
        // (com pressa, para caber mais partidas: so um quadro em cada tantos e desenhado)
        if (pressa <= 1 || r32(&rs) % (uint32_t)pressa == 0) {
            uint32_t h0 = hash_largo() ^ semente;
            dado_desenha();
            if ((hash_largo() ^ semente) != h0 && !mexeu++) mexeu_fase = fase;
        }
        float dt = eu == 0 ? 1 / 60.0f : 1 / 144.0f + (r32(&rs) % 1000) / 1000.0f * (1 / 30.0f - 1 / 144.0f);
        dado_passo(dt);
        relogio += (int64_t)(dt * 1e6f);
#ifdef DESSINCRONIZA
        if (eu == 1 && q == 3000) rnd();     // so para ver o teste falhar: um sorteio a mais de um lado
#endif
        if (partidas >= 1) return true;      // a partida acabou e a revanche comecou
    }
    return false;
}

#define CAB_N (6 + ST_N)
typedef struct { pid_t pid; int res; } lado_t;
static lado_t abre_lado(int eu, uint32_t sem, int quadros, int cano_in, int cano_out, int fecha1, int fecha2)
{
    int res[2];
    if (pipe(res)) { perror("pipe"); exit(2); }
    fflush(stdout);
    pid_t p = fork();
    if (p < 0) { perror("fork"); exit(2); }
    if (p == 0) {
        signal(SIGPIPE, SIG_IGN);
        close(res[0]); close(fecha1); close(fecha2);
        fd_in = cano_in; fd_out = cano_out;
        fcntl(fd_in, F_SETFL, O_NONBLOCK);
        bool fim = joga(eu, sem, quadros);
        int32_t cab[CAB_N] = { fim, n_marcos, fim_f0, fim_f1, mexeu, mexeu_fase };
        memcpy(cab + 6, st, sizeof st);
        if (write(res[1], cab, sizeof cab) != sizeof cab) _exit(3);
        if (n_marcos && write(res[1], marcos, sizeof(marco_t) * (size_t)n_marcos) != (ssize_t)(sizeof(marco_t) * (size_t)n_marcos))
            _exit(3);
        _exit(0);
    }
    close(res[1]);
    return (lado_t){ p, res[0] };
}

static bool le_tudo(int fd, void *buf, size_t n)
{
    char *p = buf;
    while (n) {
        ssize_t k = read(fd, p, n);
        if (k <= 0) return false;
        p += k; n -= (size_t)k;
    }
    return true;
}

typedef struct { int32_t fim, n, f0, f1, mexeu, mexeu_fase, st[ST_N]; marco_t m[MAX_MARCOS]; } resultado_t;
static resultado_t res_lado[2];

// O que sobrou de outra partida neste computador nao entra na online. Os dois
// casos que as salas "sujas" acharam, aqui diretos: a sequencia de antes (ia
// no primeiro marco, e as mesas "ficavam diferentes" na saida da loja) e a
// tela de colar o adesivo ganho, largada no meio em 2 jogadores (o premio
// seguinte abria nela, e as teclas da escolha iam para o lugar errado).
static int confere_sobras(void)
{
    int f = 0;
    natal_forcado = 0;
    modo = M_DOIS;
    dado_inicia(1);
    F[0].n = 3; F[0].lancados = 2; F[0].pontos = 9; F[1].n = 1; F[1].total = 4;
    cola_premio = true; lj_colando = 1;
    dado_online_comeca(7, 0);
    static const fila_t limpa[2];
    if (memcmp(F, limpa, sizeof F)) { printf("FALHOU: a sequencia da partida de antes entra no primeiro marco da online\n"); f++; }
    abre_premio(0);
    if (premio_colando()) { printf("FALHOU: o premio da online abre na tela de colar que ficou da partida de antes\n"); f++; }
    return f;
}

int main(int argc, char **argv)
{
    static const uint32_t SEMENTES[] = { 7, 2026, 31337, 424242 };
    int salas = argc > 1 ? atoi(argv[1]) : 2, quadros = argc > 2 ? atoi(argv[2]) : 150000;
    forca_modo = argc > 3 ? atoi(argv[3]) : 0; atraso_max = argc > 4 ? atoi(argv[4]) : 0; engasga = argc > 5 ? atoi(argv[5]) : 0; locais = argc > 7 ? atoi(argv[7]) : 0;
    sujo = argc > 8 ? atoi(argv[8]) : 0; quedas = argc > 9 ? atoi(argv[9]) : 0; pressa = argc > 10 ? atoi(argv[10]) : 1;
    uint32_t base = argc > 6 ? (uint32_t)atoi(argv[6]) : 0;
    if (salas < 1) salas = 1;
    int falhas = 0, falhas_x = 0, falhas_d = 0;
    long total[ST_N] = { 0 }, marcos_t = 0;
    lado_t L[4][2];
    for (int s0 = 0; s0 < salas; s0 += 4) {           // quatro salas de cada vez, ao mesmo tempo
        int lote = salas - s0 < 4 ? salas - s0 : 4;
        uint32_t sems[4];
        for (int s = 0; s < lote; s++) {
            sems[s] = SEMENTES[s] + base * 1000003u + (uint32_t)(s0 / 4) * 7777777u;
            if (getenv("SEMENTE")) sems[s] = (uint32_t)strtoul(getenv("SEMENTE"), NULL, 10) + (uint32_t)(s0 + s);   // para repetir uma sala
            int a2b[2], b2a[2];
            if (pipe(a2b) || pipe(b2a)) { perror("pipe"); return 2; }
            L[s][0] = abre_lado(0, sems[s], quadros, b2a[0], a2b[1], a2b[0], b2a[1]);
            L[s][1] = abre_lado(1, sems[s], quadros, a2b[0], b2a[1], b2a[0], a2b[1]);
            close(a2b[0]); close(a2b[1]); close(b2a[0]); close(b2a[1]);
        }
        for (int s = 0; s < lote; s++) {
            for (int j = 0; j < 2; j++) {
                resultado_t *r = &res_lado[j];
                memset(r, 0, sizeof *r);
                if (!le_tudo(L[s][j].res, r, sizeof(int32_t) * CAB_N) || r->n < 0 || r->n > MAX_MARCOS
                    || (r->n && !le_tudo(L[s][j].res, r->m, sizeof(marco_t) * (size_t)r->n))) r->fim = -1;
                close(L[s][j].res);
                waitpid(L[s][j].pid, NULL, 0);
            }
            resultado_t *a = &res_lado[0], *b = &res_lado[1];
            int igual = 0;
            while (igual < a->n && igual < b->n && a->m[igual].i == b->m[igual].i && a->m[igual].h == b->m[igual].h) igual++;
            int igual_x = 0;
            while (igual_x < a->n && igual_x < b->n && a->m[igual_x].x == b->m[igual_x].x) igual_x++;
            if (igual_x < igual || (igual_x < a->n && igual == a->n)) printf("   sala %u: o hash largo diverge no marco %d (o do jogo so no %d)\n", sems[s], igual_x, igual);
            if (igual_x < a->n) falhas_x++;
            if (a->mexeu || b->mexeu) {
                printf("   sala %u: o desenho mexeu nas regras ou gastou o sorteio (%d e %d quadros; a primeira vez na fase %d)\n",
                       sems[s], a->mexeu, b->mexeu, a->mexeu ? a->mexeu_fase : b->mexeu_fase);
                falhas_d++;
            }
            bool ok = a->fim == 1 && b->fim == 1 && a->n == b->n && igual == a->n && a->n >= 3
                   && a->f0 == b->f0 && a->f1 == b->f1;
            printf("sala %u: %s  marcos %d/%d (iguais ate o %d), fichas %d x %d / %d x %d%s\n", sems[s],
                   ok ? "ok   " : "FALHOU", a->n, b->n, igual, a->f0, a->f1, b->f0, b->f1,
                   a->fim == 1 && b->fim == 1 ? "" : "  (a partida nao terminou nos dois)");
            if (!ok) falhas++;
            for (int i = 0; i < ST_N; i++) total[i] += a->st[i];
            marcos_t += a->n;
        }
    }
    if (forca_modo == 3) {
        printf("\nem %ld marcos:", marcos_t);
        for (int i = 0; i < ST_N; i++) printf("%s %s %ld", i ? "," : "", ST_NOME[i], total[i]);
        printf("\n");
    }
    if (falhas_x) printf("%d sala(s) com o hash largo diferente\n", falhas_x);
    if (falhas_d) printf("%d sala(s) em que o desenho mexeu no estado\n", falhas_d);
    int f = falhas > falhas_x ? falhas : falhas_x;
    if (falhas_d > f) f = falhas_d;
    if (confere_sobras()) f++;
    if (f) { printf("\n%d falha(s)\n", f); return 1; }
    printf("\nos dois lados iguais em %d sala(s)\n", salas);
    return 0;
}
