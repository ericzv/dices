// A carga dos sons sob demanda, conferida nos testes de host (simula.c e
// desafio.c). O PC e o site so guardam na memoria os sons que o jogo diz que
// a cena pode pedir (dado_sons_precisos, a cada quadro); um som que toca sem
// ter sido avisado e decodificado na hora, com um engasgo. Aqui os som_* de
// mentira anotam cada som que tocou sem aviso; no fim, sons_relata confere
// que foram raros (e os lista, se pedido).
//
// Quem inclui chama sons_avisa() depois de cada dado_passo, como o PC faz.
#pragma once
#include <stdio.h>
#include "hal/som.h"
#include "games/jogos.h"

static uint8_t avisado[SOM_N], avisado_rola[SOM_ROLAS];
static int avisado_lacos;
static long sons_tocados, sons_sem_aviso;
static int sem_aviso[SOM_N], sem_aviso_rola[SOM_ROLAS], sem_aviso_laco[3];

static bool avisou;
static void sons_avisa(void) { avisado_lacos = dado_sons_precisos(avisado, avisado_rola); avisou = true; }

static void tocou(int s)
{
    if (s < 0 || s >= SOM_N) return;
    if (!avisou) sons_avisa();                   // antes do primeiro quadro: a conta de agora
    sons_tocados++;
    if (!avisado[s]) { sons_sem_aviso++; sem_aviso[s]++; }
}
static void tocou_laco(int bit)
{
    sons_tocados++;
    if (!(avisado_lacos & bit)) { sons_sem_aviso++; sem_aviso_laco[bit == SONS_LACO_MOEDA ? 0 : bit == SONS_LACO_GIRO ? 1 : 2]++; }
}

void som_toca(int s) { tocou(s); }
void som_toca_tom(int s, int t) { (void)t; tocou(s); }
void som_rufo(bool on) { (void)on; }
void som_rufo_moeda(void) { tocou_laco(SONS_LACO_MOEDA); }
void som_rufo_de(int q)
{
    if (q <= 0 || q >= SOM_ROLAS) return;        // o rufo de sempre
    sons_tocados++;
    if (!avisado_rola[q]) { sons_sem_aviso++; sem_aviso_rola[q]++; }
}
void som_rufo_forca(float f) { (void)f; }
void som_giro(bool on) { if (on) tocou_laco(SONS_LACO_GIRO); }
void som_motor(bool on) { if (on) tocou_laco(SONS_LACO_MOTOR); }

// Imprime a conta (e, com lista, cada som que tocou sem aviso). Devolve
// false se passaram de por_mil a cada mil sons tocados.
static bool sons_relata(bool lista, int por_mil)
{
    printf("sons: %ld tocados, %ld sem aviso (%.2f%%)\n", sons_tocados, sons_sem_aviso,
           sons_tocados ? 100.0 * sons_sem_aviso / sons_tocados : 0.0);
    if (lista) {
        for (int s = 0; s < SOM_N; s++) if (sem_aviso[s]) printf("  som %d: %d vezes\n", s, sem_aviso[s]);
        for (int q = 0; q < SOM_ROLAS; q++) if (sem_aviso_rola[q]) printf("  laco de rolar %d: %d vezes\n", q, sem_aviso_rola[q]);
        for (int k = 0; k < 3; k++)
            if (sem_aviso_laco[k]) printf("  laco %s: %d vezes\n", k == 0 ? "da moeda" : k == 1 ? "do giro" : "do motor", sem_aviso_laco[k]);
    }
    if (sons_sem_aviso * 1000 <= sons_tocados * por_mil) return true;
    printf("FALHOU: %ld de %ld sons tocaram sem aviso (o limite e %d por mil): falta alguem em dado_sons_precisos\n",
           sons_sem_aviso, sons_tocados, por_mil);
    return false;
}
