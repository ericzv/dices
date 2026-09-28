// Teste das regras dos dados, com jogadas montadas a mao: cada caso poe dados
// e valores exatos na fila e confere o que a regra da fila e o fim da rodada
// fazem. Inclui o dado.c para alcancar as funcoes internas.
#include <stdio.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
int64_t esp_timer_get_time(void) { return 1; }
void som_toca(int s) { (void)s; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }

static int falhas;
#define CONFERE(cond, ...) do { if (!(cond)) { printf("FALHOU %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); putchar('\n'); falhas++; } } while (0)

static void limpa(void)
{
    memset(F, 0, sizeof F);
    primeiro = 0;
    n_ev = 0;
}

// Poe o dado de tipo t com o valor v no fim da fila de j e aplica a regra.
static void joga(int j, int t, int v)
{
    fila_t *f = &F[j];
    int k = f->lancados;
    f->tipo[k] = (uint8_t)t;
    f->n = k + 1;
    f->cru[k] = f->valor[k] = v;
    f->lancados = k + 1;
    aplica_regra(j, k);
}

static desfecho_t fim(void)
{
    desfecho_t d;
    calcula_desfecho(&d, true);
    return d;
}

// Mesa de apostas limpa: A (jogador 0) joga primeiro e abre a aposta.
static void aposta_zera(int a, int b)
{
    J[0].fichas = a; J[1].fichas = b;
    pote = 0; a_pagar = 0; passes = 0; aumentou = false;
    primeiro = vez = 0;
    fase = F_APOSTA;
}

// Comeco de rodada com estas fichas e este pote parado na mesa.
static void entrada(int a, int b, int p)
{
    for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
    J[0].fichas = a; J[1].fichas = b; pote = p;
    rodada = 1;
    fase = F_RESULTADO;
    nova_rodada();
}

static uint32_t teste_estado = 12345;
static uint32_t teste_rnd(void)
{
    teste_estado ^= teste_estado << 13; teste_estado ^= teste_estado >> 17; teste_estado ^= teste_estado << 5;
    return teste_estado;
}

int main(void)
{
    desfecho_t d;

    // Espinhoso: tira do rival o valor que tirou, e ainda soma para o dono.
    limpa();
    joga(0, D_MARTELO, 3);
    joga(1, D_D6, 6);
    d = fim();
    CONFERE(d.total[1] == 3, "Espinhoso 3: rival devia ficar com 3, ficou %d", d.total[1]);
    CONFERE(d.total[0] == 3, "Espinhoso 3: dono devia fazer 3, fez %d", d.total[0]);

    // Copiador 4: cada outro 4 ganha +2.
    limpa();
    joga(0, D_COPIADOR, 4);
    joga(0, D_D6, 4);
    joga(0, D_D8, 4);
    joga(0, D_D8, 7);
    d = fim();
    CONFERE(d.total[0] == 4 + 6 + 6 + 7, "Copiador: esperava 23, deu %d", d.total[0]);

    // Copiador anulado nao da nada.
    limpa();
    joga(0, D_D8, 5);
    joga(0, D_COPIADOR, 3);          // menor: anula os dois
    joga(0, D_D6, 3);
    d = fim();
    CONFERE(d.total[0] == 3, "Copiador anulado: esperava 3, deu %d", d.total[0]);

    // Jackpot: tres iguais (sem contar ele) com o Jackpot valendo: +10.
    limpa();
    joga(0, D_JACKPOT, 1);
    joga(0, D_D6, 5);
    joga(0, D_D6, 5);
    joga(0, D_D8, 5);
    d = fim();
    CONFERE(d.total[0] == 1 + 15 + 10, "Jackpot com trinca: esperava 26, deu %d", d.total[0]);
    bool viu = false;
    for (int i = 0; i < n_ev; i++) if (ev[i].tipo == EV_JACKPOT) viu = true;
    CONFERE(viu, "Jackpot com trinca: faltou o evento");

    // Jackpot contando ele mesmo na trinca.
    limpa();
    joga(0, D_D4, 3);
    joga(0, D_JACKPOT, 3);
    joga(0, D_D6, 3);
    d = fim();
    CONFERE(d.total[0] == 9 + 10, "Jackpot na trinca: esperava 19, deu %d", d.total[0]);

    // Sem trinca, nada.
    limpa();
    joga(0, D_JACKPOT, 2);
    joga(0, D_D6, 2);
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.total[0] == 9, "Jackpot sem trinca: esperava 9, deu %d", d.total[0]);

    // Trinca sem Jackpot, nada.
    limpa();
    joga(0, D_D6, 4);
    joga(0, D_D6, 4);
    joga(0, D_D6, 4);
    d = fim();
    CONFERE(d.total[0] == 12, "Trinca sem Jackpot: esperava 12, deu %d", d.total[0]);

    // Egoista: anula os anteriores ao chegar e os que vierem depois; o Escudo resiste.
    limpa();
    joga(0, D_D6, 2);
    joga(0, D_ESCUDO, 3);
    joga(0, D_EGOISTA, 9);
    CONFERE(F[0].est[0] == V_ANULADO, "Egoista: devia anular o d6 anterior");
    CONFERE(F[0].est[1] == V_VALIDO, "Egoista: o Escudo devia resistir");
    CONFERE(F[0].est[2] == V_VALIDO, "Egoista: devia valer");
    joga(0, D_D12, 11);
    CONFERE(F[0].est[3] == V_ANULADO, "Egoista: devia anular o dado de depois");
    d = fim();
    CONFERE(d.total[0] == 3 + 9, "Egoista: esperava 12, deu %d", d.total[0]);

    // Egoista nao cai na queda da fila.
    limpa();
    joga(0, D_D12, 12);
    joga(0, D_EGOISTA, 1);
    CONFERE(F[0].est[1] == V_VALIDO, "Egoista 1 depois de 12: nao devia cair");
    CONFERE(F[0].est[0] == V_ANULADO, "Egoista 1 depois de 12: o 12 devia ser anulado por ele");

    // Nem por ataque: o Carrasco com 4 procura outro alvo.
    limpa();
    joga(1, D_EGOISTA, 8);
    joga(0, D_CARRASCO, 4);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Carrasco nao devia anular o Egoista");
    CONFERE(d.total[1] == 8, "Egoista atacado: esperava 8, deu %d", d.total[1]);

    // E o risco depois dele e total (a IA usa isso para parar).
    limpa();
    joga(0, D_EGOISTA, 6);
    F[0].tipo[1] = D_D20; F[0].n = 2;
    CONFERE(risco(0) == 100, "risco depois do Egoista devia ser 100, deu %d", risco(0));

    // Moeda da sorte: 1 da +1 nos impares (ela inclusive), 2 da +1 nos pares.
    limpa();
    joga(0, D_SORTE, 1);
    joga(0, D_D6, 3);
    joga(0, D_D6, 4);
    d = fim();
    CONFERE(d.total[0] == (1 + 1) + (3 + 1) + 4, "Moeda 1: esperava 10, deu %d", d.total[0]);
    limpa();
    joga(0, D_SORTE, 2);
    joga(0, D_D6, 3);
    joga(0, D_D6, 4);
    d = fim();
    CONFERE(d.total[0] == (2 + 1) + 3 + (4 + 1), "Moeda 2: esperava 11, deu %d", d.total[0]);

    // Viciado: o veredito conta os dois lances.
    limpa();
    F[0].descarte[0] = 2;
    joga(0, D_VICIADO, 5);
    CONFERE(strstr(veredito, "5 e 2") != NULL, "Viciado: veredito devia citar os dois lances: %s", veredito);

    // ---- Apostas -------------------------------------------------------
    // Dobrar completo: A aposta 10, B cobre e sobe 10, A paga 10.
    aposta_zera(100, 100);
    executa(OP_APOSTAR10);
    executa(OP_AUMENTAR);
    CONFERE(a_pagar == 10, "dobrar completo: A devia pagar 10, paga %d", a_pagar);
    CONFERE(J[1].fichas == 80 && pote == 30, "dobrar completo: B %d, pote %d", J[1].fichas, pote);
    executa(OP_PAGAR);
    CONFERE(J[0].fichas == 80 && J[1].fichas == 80 && pote == 40,
            "dobrar completo: A %d B %d pote %d", J[0].fichas, J[1].fichas, pote);

    // Dobrar parcial: B tem 30 contra a aposta de 25; cobre 25 e sobe so 5.
    aposta_zera(100, 30);
    executa(OP_APOSTAR25);
    int op[4], n_op = opcoes(op);
    char rot[24] = "";
    for (int i = 0; i < n_op; i++) if (op[i] == OP_AUMENTAR) rotulo_op(op[i], rot, sizeof rot);
    CONFERE(!strcmp(rot, "subir 5"), "dobrar parcial: rotulo devia ser 'subir 5', foi '%s'", rot);
    executa(OP_AUMENTAR);
    CONFERE(J[1].fichas == 0, "dobrar parcial: B devia ficar com 0, ficou %d", J[1].fichas);
    CONFERE(a_pagar == 5, "dobrar parcial: A devia pagar 5, paga %d", a_pagar);
    executa(OP_PAGAR);
    CONFERE(J[0].fichas == 70 && pote == 60, "dobrar parcial: A %d pote %d", J[0].fichas, pote);

    // Limitado pelo rival: A aposta 25 e fica com 3; B so pode subir 3.
    aposta_zera(28, 100);
    executa(OP_APOSTAR25);
    executa(OP_AUMENTAR);
    CONFERE(a_pagar == 3 && J[1].fichas == 72, "limite do rival: a pagar %d, B %d", a_pagar, J[1].fichas);
    executa(OP_PAGAR);
    CONFERE(J[0].fichas == 0 && pote == 56, "limite do rival: A %d pote %d", J[0].fichas, pote);

    // Sem nada para subir alem de cobrir, nao ha opcao de dobrar.
    aposta_zera(100, 25);
    executa(OP_APOSTAR25);
    n_op = opcoes(op);
    bool tem_dobrar = false;
    for (int i = 0; i < n_op; i++) tem_dobrar |= op[i] == OP_AUMENTAR;
    CONFERE(!tem_dobrar, "B com fichas so para cobrir nao devia poder dobrar");

    // Nunca fica saldo negativo, e o pote e sempre o que saiu dos dois.
    for (int a = 0; a <= 60; a += 3)
        for (int b = 0; b <= 60; b += 4)
            for (int abre = 0; abre < 2; abre++) {
                aposta_zera(a, b);
                executa(abre ? OP_APOSTAR25 : OP_APOSTAR10);
                if (fase != F_APOSTA) continue;
                n_op = opcoes(op);
                for (int i = 0; i < n_op; i++) if (op[i] == OP_AUMENTAR) { executa(OP_AUMENTAR); break; }
                if (fase == F_APOSTA && a_pagar) executa(OP_PAGAR);
                CONFERE(J[0].fichas >= 0 && J[1].fichas >= 0, "saldo negativo com %d x %d", a, b);
                CONFERE(J[0].fichas + J[1].fichas + pote == a + b, "fichas sumiram com %d x %d", a, b);
                CONFERE(a - J[0].fichas == b - J[1].fichas, "contribuicoes diferentes com %d x %d: %d e %d",
                        a, b, a - J[0].fichas, b - J[1].fichas);
            }

    // ---- Entrada da rodada ----------------------------------------------
    entrada(3, 100, 0);
    CONFERE(fase == F_FIM && venc_partida == 1, "so A sem entrada: B devia vencer (%d)", venc_partida);
    entrada(100, 4, 0);
    CONFERE(fase == F_FIM && venc_partida == 0, "so B sem entrada: A devia vencer (%d)", venc_partida);
    entrada(4, 2, 194);
    CONFERE(fase == F_FIM && venc_partida == 0, "os dois sem entrada: vence quem tem mais (%d)", venc_partida);
    entrada(1, 3, 196);
    CONFERE(fase == F_FIM && venc_partida == 1, "os dois sem entrada: vence quem tem mais (%d)", venc_partida);
    entrada(0, 0, 200);
    CONFERE(fase == F_FIM && venc_partida == 2, "os dois zerados: devia empatar (%d)", venc_partida);

    // ---- Multiplicadores se somam -------------------------------------
    // Dobro antes: x2. Dobro e Fartura no mesmo dado de 2: x2 + x2 = x4.
    limpa();
    joga(0, D_DOBRO, 1);
    joga(0, D_D6, 5);
    d = fim();
    CONFERE(d.total[0] == 1 + 10, "Dobro: esperava 11, deu %d", d.total[0]);
    limpa();
    joga(0, D_FARTURA, 1);
    joga(0, D_DOBRO, 1);
    joga(0, D_D4, 2);
    d = fim();
    // Fartura 1 (x2 = 2), Dobro 1 (x2 = 2), d4 2: Dobro + Fartura = x4 = 8.
    CONFERE(d.total[0] == 2 + 2 + 8, "Dobro + Fartura: esperava 12, deu %d", d.total[0]);
    // Com o Tudo ou Nada 2 junto: x2 + x2 + x2 = x6 (soma, nao x8).
    limpa();
    joga(0, D_TUDO_NADA, 2);
    joga(0, D_FARTURA, 2);
    joga(0, D_DOBRO, 2);
    joga(0, D_D4, 2);
    d = fim();
    // TN 2 (x2+x2 = 8), Fartura 2 (x2 tn + x2 far = 8), Dobro 2 (x2 x2 = 8), d4 2 (x6 = 12).
    CONFERE(d.total[0] == 8 + 8 + 8 + 12, "tres multiplicadores: esperava 36, deu %d", d.total[0]);

    // ---- Agouro: Teimoso e Escudo seguram, e a mensagem diz isso -------
    limpa();
    joga(0, D_TEIMOSO, 6);
    joga(0, D_MALDITO, 1);
    d = fim();
    CONFERE(d.total[0] == 6, "Agouro x Teimoso: o Teimoso devia ficar (6), deu %d", d.total[0]);
    CONFERE(strstr(veredito, "Teimoso") != NULL, "Agouro x Teimoso: veredito devia citar o Teimoso: %s", veredito);
    limpa();
    joga(0, D_D6, 5);
    joga(0, D_MALDITO, 3);
    CONFERE(strstr(veredito, "zerou a fila") != NULL, "Agouro: veredito '%s'", veredito);

    // ---- O resultado pago e o do motor, e os eventos chegam nele --------
    for (int caso = 0; caso < 4000; caso++) {
        limpa();
        for (int j = 0; j < 2; j++) {
            int n = 1 + (int)(teste_rnd() % N_FILA);
            for (int k = 0; k < n; k++) {
                int t = (int)(teste_rnd() % D_N);
                int v = 1 + (int)(teste_rnd() % TIPO[t].lados);
                F[j].fixo[F[j].lancados] = (uint8_t)v;
                joga(j, t, v);
            }
        }
        J[0].fichas = J[1].fichas = 100; pote = 10;
        desfecho();
        for (int i = 0; i < n_ev; i++) aplica_evento(i);
        for (int j = 0; j < 2; j++)
            CONFERE(total_visivel(j) == resolvido.total[j],
                    "caso %d: eventos mostram %d, o motor paga %d (jogador %d)",
                    caso, total_visivel(j), resolvido.total[j], j);
        if (fase == F_EFEITOS) conclui_rodada();
        for (int j = 0; j < 2; j++)
            CONFERE(F[j].total == resolvido.total[j], "caso %d: pagou %d, motor %d", caso, F[j].total, resolvido.total[j]);
        if (falhas > 20) break;
    }

    // ---- Lastro: 6 a 12, cada um 1 em 7 ---------------------------------
    {
        int cont[13] = { 0 }, n = 70000, menor;
        semente = 0x1234567u;                    // o sorteio da partida, semeado
        for (int i = 0; i < n; i++) {
            int v = sorteia_valor(D_LASTRO, 0, rola, &menor);
            CONFERE(v >= 6 && v <= 12, "Lastro tirou %d", v);
            if (v >= 6 && v <= 12) cont[v]++;
        }
        for (int v = 6; v <= 12; v++)
            CONFERE(cont[v] > n / 7 * 0.94 && cont[v] < n / 7 * 1.06,
                    "Lastro: o %d saiu %d vezes em %d (esperado ~%d)", v, cont[v], n, n / 7);
        CONFERE(TIPO[D_LASTRO].raridade == RAR_RARO, "Lastro devia ser raro");
        // Risco com barra 9: 6, 7 e 8 caem = 3 em 7.
        limpa();
        joga(0, D_D12, 9);
        F[0].tipo[1] = D_LASTRO; F[0].n = 2;
        CONFERE(risco(0) == 300 / 7, "Lastro com barra 9: risco %d, esperado %d", risco(0), 300 / 7);
    }

    // ---- Previa: o total com os bonus pode ser negativo ------------------
    limpa();
    primeiro = 1;
    joga(1, D_MARTELO, 4);              // Espinhoso do rival: -4
    joga(0, D_D4, 1);
    {
        desfecho_t pv;
        calcula_desfecho(&pv, false);
        CONFERE(pv.bruto[0] == -3 && pv.total[0] == 0, "previa negativa: bruto %d total %d", pv.bruto[0], pv.total[0]);
    }

    // ---- Nao anulaveis (Escudo, Teimoso, Egoista) e intocavel (Escudo) --
    limpa();
    joga(0, D_TEIMOSO, 3);
    joga(0, D_EGOISTA, 7);
    CONFERE(F[0].est[0] == V_VALIDO, "Egoista nao devia anular o Teimoso");
    limpa();
    joga(1, D_TEIMOSO, 6);
    joga(1, D_D4, 2);                  // cai: o Teimoso segura, o 2 cai
    joga(0, D_CARRASCO, 4);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Carrasco nao devia anular o Teimoso");
    limpa();
    joga(1, D_EGOISTA, 9);
    joga(0, D_INVEJOSO, 3);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Invejoso nao devia anular o Egoista");
    // Roubar pode: o Egoista e o Teimoso sao roubaveis; o Escudo, nao.
    limpa();
    joga(1, D_EGOISTA, 5);
    joga(0, D_CARIDOSO, 2);
    d = fim();
    CONFERE(d.est[1][0] == V_ROUBADO, "Caridoso devia roubar o Egoista");
    limpa();
    joga(1, D_TEIMOSO, 5);
    joga(0, D_CARIDOSO, 2);
    d = fim();
    CONFERE(d.est[1][0] == V_ROUBADO, "Caridoso devia roubar o Teimoso");
    limpa();
    joga(1, D_ESCUDO, 5);
    joga(0, D_CARIDOSO, 2);
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Escudo e intocavel: nao devia ser roubado");

    // ---- Gatuno: so rouba se terminar valendo ---------------------------
    limpa();
    joga(1, D_D6, 4);
    joga(0, D_D2, 1);
    joga(0, D_GATUNO, 3);
    d = fim();
    CONFERE(d.est[1][0] == V_ROUBADO, "Gatuno valendo com um 1 seu devia roubar");
    limpa();
    joga(1, D_D6, 4);
    joga(0, D_D8, 5);
    joga(0, D_GATUNO, 1);              // 1 < 5: cai junto com o 5
    CONFERE(F[0].est[1] == V_ANULADO, "Gatuno 1 depois de 5 devia cair");
    d = fim();
    CONFERE(d.est[1][0] == V_VALIDO, "Gatuno anulado nao devia roubar");

    // ---- Espelho: vira sempre o maior do rival ---------------------------
    limpa();
    primeiro = 1;
    joga(1, D_D8, 7);
    joga(1, D_D4, 1);                  // cai: sobra... nada, os dois caem
    joga(1, D_D12, 9);
    joga(0, D_ESPELHO, 2);
    CONFERE(F[0].valor[0] == 9, "Espelho do segundo devia virar o 9 do rival, virou %d", F[0].valor[0]);
    limpa();
    primeiro = 1;
    joga(1, D_D2, 1);
    joga(0, D_ESPELHO, 2);
    CONFERE(F[0].valor[0] == 1, "Espelho vira sempre o maior do rival, ate menor: devia ser 1, e %d", F[0].valor[0]);
    // Quem abre: o Espelho espera, nao cai, e vira no fim.
    limpa();
    primeiro = 0;
    joga(0, D_D6, 5);
    joga(0, D_ESPELHO, 1);
    CONFERE(F[0].est[1] == V_VALIDO && F[0].espera[1], "Espelho de quem abre devia esperar valendo");
    joga(1, D_D6, 3);
    joga(1, D_D12, 11);
    {
        desfecho_t pv;
        calcula_desfecho(&pv, false);
        CONFERE(pv.total[0] == 5 + 11, "previa com Espelho esperando: esperava 16, deu %d", pv.total[0]);
        CONFERE(F[0].valor[1] == 1 && F[0].espera[1], "a previa nao devia mexer no Espelho");
    }
    d = fim();
    CONFERE(d.total[0] == 16 && F[0].valor[1] == 11 && !F[0].espera[1],
            "Espelho no fim: total %d, valor %d", d.total[0], F[0].valor[1]);
    bool viu_esp = false;
    for (int i = 0; i < n_ev; i++) viu_esp |= ev[i].tipo == EV_ESPELHO;
    CONFERE(viu_esp, "Espelho no fim: faltou o evento");

    // ---- Premio: quem ganha no correr tambem escolhe; no fim, nao ha ------
    for (int j = 0; j < 2; j++) colecao_inicial(&J[j]);
    J[0].fichas = J[1].fichas = 80;
    rodada = 3;
    aposta_zera(80, 80);
    executa(OP_APOSTAR10);
    executa(OP_CORRER);                // B corre: A leva o pote
    segue_do_resultado();
    CONFERE(fase == F_PREMIO && vez == 0, "quem leva no correr devia escolher premio (fase %d vez %d)", fase, vez);
    rodada = MAX_RODADAS;
    venc_mao = 1;
    J[0].fichas = J[1].fichas = 80;
    fase = F_RESULTADO;
    segue_do_resultado();
    CONFERE(fase == F_FIM, "ultima rodada: sem premio, a partida acaba (fase %d)", fase);
    rodada = 5;
    venc_mao = 0;
    J[0].fichas = 150; J[1].fichas = 3;
    fase = F_RESULTADO;
    segue_do_resultado();
    CONFERE(fase == F_FIM && venc_partida == 0, "rival sem fichas: sem premio, acaba (fase %d)", fase);

    // ---- IA: nao troca empate certo por derrota certa --------------------
    // O rival abriu e fez 6. A IA tem um 6 valendo e o proximo e um Quebrado
    // com 1 gravado: lancar anula o 6 e o 1. Parar empata.
    limpa();
    primeiro = 1;
    joga(1, D_D6, 6);
    joga(0, D_D6, 6);
    F[0].tipo[1] = D_QUEBRADO; F[0].fixo[1] = 1; F[0].n = 2;
    vez = 0;
    CONFERE(!ia_continua(), "IA: com empate na mao, nao devia lancar o Quebrado 1");
    // Perdendo, lanca: parar perde com certeza.
    limpa();
    primeiro = 1;
    joga(1, D_D6, 6);
    joga(0, D_D4, 2);
    F[0].tipo[1] = D_D8; F[0].n = 2;
    vez = 0;
    CONFERE(ia_continua(), "IA: perdendo, devia lancar mais um");
    // Vencendo, para.
    limpa();
    primeiro = 1;
    joga(1, D_D6, 3);
    joga(0, D_D6, 5);
    F[0].tipo[1] = D_D6; F[0].n = 2;
    vez = 0;
    CONFERE(!ia_continua(), "IA: vencendo, devia parar");
    // Pensar nao mexe na mesa nem no sorteio da partida.
    uint32_t sem_antes = semente;
    fila_t f_antes[2];
    memcpy(f_antes, F, sizeof F);
    (void)ia_continua();
    CONFERE(semente == sem_antes, "IA: pensar mexeu no sorteio da partida");
    CONFERE(!memcmp(f_antes, F, sizeof F), "IA: pensar mexeu na mesa");

    if (falhas) { printf("%d falhas\n", falhas); return 1; }
    puts("regras ok");
    return 0;
}
