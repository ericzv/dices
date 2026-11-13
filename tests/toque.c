// Teste do toque (celular, tablet): nas acoes do jogo, o primeiro toque so
// escolhe e o segundo confirma; o clique do mouse continua valendo com um so;
// voltar, olhar a bolsa e os menus seguem com um toque.
//
//   toque
#include <stdio.h>
#include <stdlib.h>
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
void som_motor(bool on) { (void)on; }
void som_suspense(bool on) { (void)on; }
void som_cadeia(bool on) { (void)on; }
void som_chefe(int q, bool t) { (void)q; (void)t; }
static int volume_[SOM_CANAIS] = { SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS };
void som_volume(int c, int n) { if (c >= 0 && c < SOM_CANAIS) volume_[c] = n; }
int som_volume_de(int c) { return c >= 0 && c < SOM_CANAIS ? volume_[c] : 0; }
#include "salva_mem.h"

static int falhas;
static void confere(bool ok, const char *oque)
{
    printf("%s %s\n", ok ? "ok    " : "FALHOU", oque);
    if (!ok) falhas++;
}

static void passa(int n)
{
    for (int i = 0; i < n; i++) {
        dado_desenha();
        dado_passo(1 / 60.0f);
        relogio += 16667;
    }
}

// A zona de agora que satisfaz o pedido: com cursor (var) ou nao, a tecla
// dela, e se pede dois toques.
static const zona_clique_t *acha_zona(int tecla, bool com_cursor, bool confirma, int pula)
{
    dado_desenha();
    for (int i = 0; i < n_zonas; i++) {
        const zona_clique_t *z = &zonas[i];
        if (z->tecla == tecla && (z->var != NULL) == com_cursor && z->confirma == confirma && pula-- <= 0) return z;
    }
    return NULL;
}

// Como a plataforma faz: o dedo pousa (o mouse vai para la) e toca.
static void toca(const zona_clique_t *z, int acao)
{
    int x = z->x + z->w / 2, y = z->y + z->h / 2;
    dado_mouse(x, y, 0);
    dado_mouse(x, y, acao);
    passa(3);
}

int main(void)
{
    natal_forcado = 0;
    dado_inicia(0);
    passa(30);
    snprintf(perfis[0].nome, sizeof perfis[0].nome, "TESTE");
    perfis[0].nivel = 10; perfis[0].marcos = 3; perfil_atual = 0; perfis_lidos = true;
    perfis[0].bolsas = 0xFF;
    des_dif = 2; des_bolsa = B_LATA; des_nova_run(); t_elev = 0; passa(30);
    amuleto_escolhido(1);
    run.andar = 0; gera_andar(); run.passo = 0;

    // A loja: o primeiro dado da vitrine, com o mouse: um clique compra.
    run.moedas = 99; abre_loja_run(); passa(120);
    const zona_clique_t *z = acha_zona(KEY_ENTER, true, true, 0);
    confere(z != NULL && z->val == 0 && n_vitrine() > 0, "a loja tem dados que pedem dois toques");
    if (z) {
        zona_clique_t item = *z;
        int antes = run.moedas;
        toca(&item, 1);
        confere(run.moedas < antes, "loja: o clique do mouse compra com um so");
    }
    // Re-sortear a loja, no toque: o primeiro so escolhe, o segundo troca.
    const zona_clique_t *zt = NULL;
    dado_desenha();
    for (int i = 0; i < n_zonas; i++) if (zonas[i].var == &cursor && zonas[i].val == lj_item_troca()) zt = &zonas[i];
    confere(zt != NULL && zt->confirma, "loja: re-sortear pede dois toques");
    if (zt) {
        zona_clique_t troca = *zt;
        int antes = run.moedas, trocas = run.trocas;
        toca(&troca, 3);
        confere(run.moedas == antes && run.trocas == trocas && fase == F_RLOJA, "loja: o primeiro toque nao re-sorteia");
        confere(armada.ativo && cursor == troca.val, "loja: o primeiro toque escolhe (o cursor vai para la)");
        dado_desenha();
        toca(&troca, 3);
        confere(run.trocas == trocas + 1, "loja: o segundo toque re-sorteia");
        confere(!armada.ativo, "loja: depois de re-sortear, nada fica esperando");
    }
    // Um dado novo da vitrine, no toque.
    z = acha_zona(KEY_ENTER, true, true, 0);
    if (z && n_vitrine() > 0) {
        zona_clique_t item = *z;
        int antes = run.moedas;
        toca(&item, 3);
        confere(run.moedas == antes, "loja: o primeiro toque num dado nao compra");
        dado_desenha();
        toca(&item, 3);
        confere(run.moedas < antes, "loja: o segundo toque compra");
    }
    // Um toque num item e outro toque noutro: o segundo so escolhe.
    const zona_clique_t *a = acha_zona(KEY_ENTER, true, true, 0), *b = acha_zona(KEY_ENTER, true, true, 1);
    if (a && b) {
        zona_clique_t za = *a, zb = *b;
        int antes = run.moedas;
        toca(&za, 3);
        dado_desenha();
        toca(&zb, 3);
        confere(run.moedas == antes && cursor == zb.val, "loja: tocar noutro item troca a escolha, sem comprar");
        manda_tecla(KEY_LEFT);
        confere(!armada.ativo, "loja: uma tecla desarma o toque");
    }
    // Seguir para o mapa: dois toques.
    z = acha_zona(KEY_TAB, false, true, 0);
    confere(z != NULL, "loja: o 'seguir' pede dois toques");
    if (z) {
        zona_clique_t seguir = *z;
        toca(&seguir, 3);
        confere(fase == F_RLOJA, "loja: o primeiro toque no 'seguir' nao sai");
        dado_desenha();
        toca(&seguir, 3);
        confere(fase == F_MAPA, "loja: o segundo toque no 'seguir' vai para o mapa");
    }
    // No mapa, a bolsa da faixa so abre a tela de olhar: um toque.
    abre_mapa(); t_luz = 0; t_cort = -1; t_mapa_anda = 0; passa(60);
    z = acha_zona(KEY_TAB, false, false, 0);
    confere(z != NULL, "mapa: a bolsa da faixa vale com um toque");
    if (z) {
        zona_clique_t bolsa = *z;
        toca(&bolsa, 3);
        confere(fase == F_RBOLSA, "mapa: um toque na bolsa abre a bolsa");
    }
    // A mesa do mapa: dois toques para sentar.
    abre_mapa(); t_luz = 0; t_cort = -1; t_mapa_anda = 0; passa(60);
    z = acha_zona(KEY_ENTER, true, true, 0);
    confere(z != NULL, "mapa: a mesa da vez pede dois toques");
    if (z) {
        zona_clique_t mesa = *z;
        toca(&mesa, 3);
        confere(fase == F_MAPA, "mapa: o primeiro toque na mesa nao senta");
        dado_desenha();
        toca(&mesa, 3);
        confere(fase != F_MAPA, "mapa: o segundo toque na mesa comeca a partida");
    }
    // A partida: por um dado na sequencia (ou a aposta, se ela vier antes). A
    // vez do oponente passa sozinha; o resultado dele espera um ENTER.
    for (int i = 0; i < 3000 && !((fase == F_ORDEM || fase == F_APOSTA) && !ia_na_vez()); i++) {
        passa(1);
        if (i % 30 == 29 && (fase == F_RESULTADO || fase == F_EFEITOS || fase == F_VEREDITO)) manda_tecla(KEY_ENTER);
    }
    confere((fase == F_ORDEM || fase == F_APOSTA) && !ia_na_vez(), "partida: chegou a vez de montar a sequencia ou apostar");
    if (fase == F_ORDEM && !ia_na_vez()) {
        z = acha_zona(KEY_ENTER, true, true, 0);
        confere(z != NULL, "partida: os dados da mao pedem dois toques");
        if (z) {
            zona_clique_t dado = *z;
            int n0 = F[vez].n;
            toca(&dado, 3);
            confere(F[vez].n == n0, "partida: o primeiro toque num dado nao o poe na sequencia");
            dado_desenha();
            toca(&dado, 3);
            confere(F[vez].n == n0 + 1, "partida: o segundo toque poe o dado na sequencia");
        }
    } else if (fase == F_APOSTA && !ia_na_vez()) {
        z = acha_zona(KEY_ENTER, true, true, 0);
        confere(z != NULL, "partida: os botoes da aposta pedem dois toques");
        if (z) {
            zona_clique_t bt = *z;
            toca(&bt, 3);
            confere(fase == F_APOSTA, "partida: o primeiro toque na aposta nao decide");
            dado_desenha();
            toca(&bt, 3);
            confere(fase != F_APOSTA || ia_na_vez(), "partida: o segundo toque decide a aposta");
        }
    }
    // O menu principal: um toque basta.
    fase = F_ABERTURA; menu_tela = MT_PRINCIPAL; t_fase = 0; menu_cur = 0; passa(150);
    z = acha_zona(KEY_ENTER, true, false, 1);
    confere(z != NULL, "menu: as fichas do menu valem com um toque");
    if (z) {
        zona_clique_t ficha = *z;
        int tela_antes = menu_tela, fase_antes = fase;
        toca(&ficha, 3);
        passa(90);                               // a troca de tela (a varredura)
        confere(menu_tela != tela_antes || fase != fase_antes, "menu: um toque abre a opcao");
    }
    // O botao "controles" do menu: um toque abre o quadro; outro, fora das setas, fecha.
    fase = F_ABERTURA; menu_tela = MT_PRINCIPAL; t_fase = 0; passa(150);
    z = acha_zona('k', false, false, 0);
    confere(z != NULL, "menu: o botao 'controles' vale com um toque");
    if (z) {
        zona_clique_t bt = *z;
        toca(&bt, 3);
        confere(ctrl_aberto, "menu: um toque em 'controles' abre o quadro");
        dado_mouse(GFX_W / 2, GFX_H - 4, 3);
        passa(3);
        confere(!ctrl_aberto, "controles: um toque fora fecha o quadro");
    }
    // A escolha da bolsa: um toque numa ficha do lado gira a vitrine ate ela;
    // a do meio pede dois. Escolhida, o dedo (ou o mouse) passando pelos
    // pontinhos nao troca mais a bolsa da run.
    ctrl_aberto = false;
    perfis[0].bolsas = perfis[0].bolsas_vistas = BOLSAS_TODAS;
    perfis[0].bolsa_ult = B_COPO;
    abre_bolsas(); passa(90);
    z = acha_zona(TECLA_BS_VAI + bs_pos(B_COPO) + 1, false, false, 0);
    confere(z != NULL, "bolsas: a ficha do lado vale com um toque");
    if (z) {
        zona_clique_t lado = *z;
        toca(&lado, 3);
        confere(bs_cur == B_LATA, "bolsas: um toque na do lado vai ate ela");
    }
    passa(40);
    // Os botoes < e >: o mouse por cima nao gira; o clique (ou um toque) gira.
    z = acha_zona(KEY_RIGHT, false, false, 0);
    confere(z != NULL, "bolsas: o botao > vale com um toque");
    if (z) {
        zona_clique_t seta = *z;
        dado_mouse(seta.x + seta.w / 2, seta.y + seta.h / 2, 0);
        passa(10);
        confere(bs_cur == B_LATA, "bolsas: o mouse no > nao gira a vitrine");
        toca(&seta, 3);
        confere(bs_cur == B_PORCO, "bolsas: um toque no > vai para a proxima");
        z = acha_zona(KEY_LEFT, false, false, 0);
        if (z) { zona_clique_t volta = *z; toca(&volta, 1); }
        confere(bs_cur == B_LATA, "bolsas: um clique no < volta para a anterior");
        dado_mouse(-1, -1, 0);
    }
    passa(40);
    z = acha_zona(KEY_ENTER, true, true, 0);
    confere(z != NULL && z->val == B_LATA, "bolsas: a do meio pede dois toques");
    if (z) {
        zona_clique_t meio = *z;
        toca(&meio, 3);
        confere(t_bs_clique < 0, "bolsas: o primeiro toque na do meio nao escolhe");
        toca(&meio, 3);
        confere(t_bs_clique >= 0, "bolsas: o segundo toque escolhe");
        int x0 = GFX_W / 2 - (B_N - 1) * BS_PT_VAO / 2;
        for (int i = 0; i < B_N; i++) { dado_mouse(x0 + i * BS_PT_VAO, BS_PT_Y, 0); passa(3); }
        passa(120);
        confere(fase != F_RBOLSAS && run.bolsa == B_LATA, "bolsas: escolhida, o mouse nos pontinhos nao troca a bolsa");
    }
    if (falhas) { printf("\n%d falha(s)\n", falhas); return 1; }
    printf("\ntudo ok\n");
    return 0;
}
