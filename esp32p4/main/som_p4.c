// O som do jogo no aparelho: o mesmo contrato do PC (hal/som.h, mais a
// trilha), com as gravacoes do PC (desktop/sons/*.ogg) ja convertidas em
// sons.bin (ferramentas/gera_sons.c) e um misturador que a tarefa de audio
// chama no segundo nucleo. Cada voz le o ADPCM direto do sons.bin, sem abrir
// nada na memoria.
//
// O comportamento segue desktop/som.c: cada gravacao pode tocar ate quatro
// vezes junto; quem tem variantes (bate_0, bate_1...) sorteia uma diferente
// da anterior; batidas, pousos e fichas variam um pouco de tom; o rufo (do
// dado ou da moeda) e o giro entram e saem com rampa e abaixam a trilha.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "hal/som.h"
#include "som_p4.h"

static const char *TAG = "som";

extern const uint8_t sons_bin_inicio[] asm("_binary_sons_bin_start");

#define NOME_TAM  24
#define MAX_TRECHOS 96
#define VOZES     4                          // copias de cada gravacao, para sobrepor
#define VARIANTES 5                          // gravacoes diferentes do mesmo som, no maximo
#define N_EFEITOS 16                         // efeitos tocando ao mesmo tempo, no maximo

// O arquivo de cada efeito, sem o .ogg (o mesmo NOME de desktop/som.c).
static const char *const NOME[SOM_N] = {
    [SOM_TIQUE] = "tique",       [SOM_BATE] = "bate",         [SOM_POUSA] = "pousa",
    [SOM_VALIDO] = "valido",     [SOM_ANULA] = "anula",       [SOM_FICHA] = "ficha",
    [SOM_EFEITO] = "efeito",     [SOM_VITORIA] = "vitoria",   [SOM_CORREU] = "correu",
    [SOM_ABERTURA] = "abertura", [SOM_JACKPOT] = "jackpot",   [SOM_LASER] = "laser",
    [SOM_TIRA] = "tira",         [SOM_ROUBA] = "rouba",       [SOM_FOGO] = "fogo",
    [SOM_ZERA] = "zera",         [SOM_VENTO] = "vento",       [SOM_PARTIDA] = "partida",
    [SOM_FIM] = "fim",           [SOM_DERROTA] = "derrota",   [SOM_TURNO] = "turno",
    [SOM_BOLSA] = "bolsa",       [SOM_EMPATE] = "empate",     [SOM_SLOT] = "slot",
    [SOM_PREMIO] = "premio",     [SOM_VIDRO] = "vidro",       [SOM_BIGORNA] = "bigorna",
    [SOM_VALIDO2] = "valido2",   [SOM_VALIDO3] = "valido3",   [SOM_VALIDO4] = "valido4",
    [SOM_BATE_DADO] = "bate_dado", [SOM_PASSA] = "passa",
    [SOM_MOEDA_BATE] = "moeda_bate", [SOM_MOEDA_POUSA] = "moeda_pousa",
};

typedef struct {
    char nome[NOME_TAM + 1];
    const uint8_t *d;                        // ADPCM
    uint32_t n;                              // amostras
} trecho_t;
static trecho_t trechos[MAX_TRECHOS];
static int n_trechos;

typedef struct {
    int var[VARIANTES], n, ultima;           // os trechos de cada efeito
} efeito_t;
static efeito_t ef[SOM_N];
static int t_rufo = -1, t_moeda = -1, t_giro = -1, t_trilha = -1;

static bool pronto, falhou, ligado = true, musica = true;

// ---------------------------------------------------------------------------
// IMA ADPCM, lido aos poucos
// ---------------------------------------------------------------------------
static const int16_t PASSOS[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80,
    88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635,
    13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};
static const int8_t INDICES[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

// Uma voz: o trecho, onde a leitura esta e as duas amostras em volta da
// posicao (para variar o tom com interpolacao).
typedef struct {
    int t;                                   // trecho (-1: livre)
    uint32_t i;                              // proxima amostra a decodificar
    int pred, idx;
    int a, b;                                // amostras k e k+1
    uint32_t k, frac, passo;                 // posicao (k + frac/65536) e passo 16.16
    int vol;                                 // 0..256
    bool laco;
    uint32_t ordem;                          // muda a cada (re)comeco
} voz_t;

static inline int proxima(voz_t *v)
{
    const trecho_t *t = &trechos[v->t];
    if (v->i >= t->n) return 0;
    uint32_t i = v->i++;
    int nib = i & 1 ? t->d[i / 2] >> 4 : t->d[i / 2] & 15;
    int passo = PASSOS[v->idx], d = passo >> 3;
    if (nib & 4) d += passo;
    if (nib & 2) d += passo >> 1;
    if (nib & 1) d += passo >> 2;
    v->pred += nib & 8 ? -d : d;
    if (v->pred > 32767) v->pred = 32767;
    if (v->pred < -32768) v->pred = -32768;
    v->idx += INDICES[nib];
    if (v->idx < 0) v->idx = 0;
    if (v->idx > 88) v->idx = 88;
    return v->pred;
}

// Volta ao comeco do trecho (o ADPCM so se le do inicio).
static void rebobina(voz_t *v)
{
    v->i = v->k = v->frac = 0;
    v->pred = v->idx = 0;
    v->a = proxima(v);
    v->b = proxima(v);
}

static uint32_t contador;                    // so mexido com a trava

static void comeca(voz_t *v, int t, uint32_t passo, int vol, bool laco)
{
    *v = (voz_t){ .t = t, .passo = passo, .vol = vol, .laco = laco, .ordem = ++contador };
    rebobina(v);
}

// Mistura n amostras da voz em acc; devolve false quando o trecho acabou.
static bool mistura_voz(voz_t *v, int32_t *acc, int n)
{
    uint32_t fim = trechos[v->t].n;
    for (int j = 0; j < n; j++) {
        int x = v->a + (int)(((int64_t)(v->b - v->a) * v->frac) >> 16);
        acc[j] += x * v->vol >> 8;
        v->frac += v->passo;
        while (v->frac >= 65536) {
            v->frac -= 65536;
            if (++v->k >= fim) {
                if (!v->laco) return false;
                rebobina(v);
                break;
            }
            v->a = v->b;
            v->b = proxima(v);
        }
    }
    return true;
}

// As vozes: os efeitos e, a parte, o rufo, o giro e a trilha. O laco
// principal mexe nelas com a trava; o misturador trabalha numa copia.
enum { V_RUFO = N_EFEITOS, V_GIRO, V_TRILHA, N_VOZES };
static voz_t vozes[N_VOZES];
static portMUX_TYPE trava = portMUX_INITIALIZER_UNLOCKED;

// Estado do lado do jogo (como no PC).
static bool rufando, girando;
static int rolando = -1;                     // o trecho do rufo da vez: dado ou moeda
static float f_rufo, f_giro, duck = 1;

// ---------------------------------------------------------------------------
// sons.bin
// ---------------------------------------------------------------------------
static uint32_t u32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

static int acha(const char *nome)
{
    for (int i = 0; i < n_trechos; i++)
        if (!strcmp(trechos[i].nome, nome)) return i;
    return -1;
}

static void monta_efeito(int s)
{
    efeito_t *e = &ef[s];
    int t = acha(NOME[s]);
    if (t >= 0) e->var[e->n++] = t;
    else
        for (int v = 0; v < VARIANTES; v++) {
            char nome[32];
            snprintf(nome, sizeof nome, "%s_%d", NOME[s], v);
            if ((t = acha(nome)) < 0) break;
            e->var[e->n++] = t;
        }
    if (!e->n) ESP_LOGW(TAG, "sem gravacao para '%s'", NOME[s]);
}

bool som_carrega(void)
{
    const uint8_t *d = sons_bin_inicio;
    int n = d[6] | d[7] << 8;
    if (memcmp(d, "DSOM", 4) || (d[4] | d[5] << 8) != 2 || n > MAX_TRECHOS) {
        ESP_LOGE(TAG, "sons.bin de outra versao: gere de novo com ferramentas/gera_sons");
        falhou = true;
        return false;
    }
    const uint8_t *ent = d + 12, *dados = d + 12 + n * (NOME_TAM + 8);
    uint32_t total = 0;
    for (int i = 0; i < n; i++, ent += NOME_TAM + 8) {
        trecho_t *t = &trechos[i];
        memcpy(t->nome, ent, NOME_TAM);
        t->nome[NOME_TAM] = 0;
        t->n = u32(ent + NOME_TAM);
        t->d = dados + u32(ent + NOME_TAM + 4);
        total += t->n;
    }
    n_trechos = n;
    for (int s = 0; s < SOM_N; s++) monta_efeito(s);
    t_rufo = acha("rufo");
    t_moeda = acha("moeda_rola");
    t_giro = acha("giro");
    t_trilha = acha("trilha");
    for (int v = 0; v < N_VOZES; v++) vozes[v].t = -1;
    if (t_trilha >= 0) comeca(&vozes[V_TRILHA], t_trilha, 1u << 16, 0, true);
    ESP_LOGI(TAG, "%d gravacoes, %.0f s de som", n, total / 44100.0);
    pronto = true;
    return true;
}

// ---------------------------------------------------------------------------
// Misturador (tarefa de audio)
// ---------------------------------------------------------------------------
void som_mistura(int16_t *saida, int n)
{
    if (!pronto) { memset(saida, 0, (size_t)n * 2); return; }
    static int32_t acc[512];
    static voz_t local[N_VOZES];
    if (n > 512) n = 512;
    memset(acc, 0, (size_t)n * 4);

    portENTER_CRITICAL(&trava);
    memcpy(local, vozes, sizeof local);
    portEXIT_CRITICAL(&trava);
    for (int v = 0; v < N_VOZES; v++) {
        voz_t *z = &local[v];
        if (z->t < 0) continue;
        if (!z->vol) {                       // muda: so anda (a trilha nao perde o compasso)
            if (!z->laco) continue;
            static int32_t lixo[512];
            mistura_voz(z, lixo, n);
            continue;
        }
        if (!mistura_voz(z, acc, n)) z->t = -1;
    }
    portENTER_CRITICAL(&trava);
    for (int v = 0; v < N_VOZES; v++)
        if (vozes[v].ordem == local[v].ordem && vozes[v].t >= 0) {   // ninguem trocou a voz no meio
            int vol = vozes[v].vol;
            vozes[v] = local[v];
            vozes[v].vol = vol;
        }
    portEXIT_CRITICAL(&trava);

    for (int i = 0; i < n; i++) {
        int32_t x = acc[i];
        saida[i] = (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x);
    }
}

// ---------------------------------------------------------------------------
// hal/som.h
// ---------------------------------------------------------------------------
void som_toca(int s)
{
    if (!pronto || !ligado || s < 0 || s >= SOM_N || !ef[s].n) return;
    efeito_t *e = &ef[s];
    int var = 0;
    if (e->n > 1) {                          // sorteia outra gravacao, nunca a mesma de antes
        var = rand() % (e->n - 1);
        if (var >= e->ultima) var++;
        e->ultima = var;
    }
    int t = e->var[var];
    uint32_t passo = 1u << 16;
    // Batidas, pousos e fichas tambem variam um pouco de tom.
    if (s == SOM_BATE || s == SOM_BATE_DADO || s == SOM_POUSA || s == SOM_FICHA ||
        s == SOM_MOEDA_BATE || s == SOM_MOEDA_POUSA)
        passo = (uint32_t)((0.9f + 0.2f * (float)(rand() % 1001) / 1000.0f) * 65536);
    portENTER_CRITICAL(&trava);
    int iguais = 0, velha_igual = -1, livre = -1, velha = 0;
    for (int v = 0; v < N_EFEITOS; v++) {
        voz_t *z = &vozes[v];
        if (z->t == t) {
            iguais++;
            if (velha_igual < 0 || z->ordem < vozes[velha_igual].ordem) velha_igual = v;
        }
        if (z->t < 0 && livre < 0) livre = v;
        if (z->ordem < vozes[velha].ordem) velha = v;
    }
    int v = iguais >= VOZES ? velha_igual : livre >= 0 ? livre : velha;
    comeca(&vozes[v], t, passo, 256, false);
    portEXIT_CRITICAL(&trava);
}

bool som_liga(bool on) { ligado = on && pronto; return ligado; }
bool som_ligado(void) { return ligado; }
bool som_falhou(void) { return falhou; }

static void comeca_a_rolar(int t)
{
    if (t < 0 || (rufando && rolando == t)) return;
    rolando = t;
    f_rufo = 0;
    portENTER_CRITICAL(&trava);
    comeca(&vozes[V_RUFO], t, 1u << 16, 0, false);
    portEXIT_CRITICAL(&trava);
    rufando = true;
}

void som_rufo(bool on)
{
    if (!pronto) return;
    if (on) comeca_a_rolar(t_rufo);
    else rufando = false;
}

void som_rufo_moeda(void)
{
    if (pronto) comeca_a_rolar(t_moeda);
}

void som_giro(bool on)
{
    if (!pronto) return;
    if (on && !girando && t_giro >= 0) {
        f_giro = 0;
        portENTER_CRITICAL(&trava);
        comeca(&vozes[V_GIRO], t_giro, 1u << 16, 0, false);
        portEXIT_CRITICAL(&trava);
    }
    girando = on;
}

bool som_musica(bool on) { musica = on; return musica; }
bool som_musica_ligada(void) { return musica; }

// Uma vez por quadro, como o som_atualiza do PC.
void som_atualiza(float dt)
{
    if (!pronto) return;
    // O rufo entra em 60 ms e sai em 120 ms; a trilha abaixa enquanto ele toca.
    float alvo = rufando && ligado ? 1 : 0;
    f_rufo += (alvo - f_rufo) * fminf(1, dt / (rufando ? 0.06f : 0.12f));
    // O giro entra na hora e sai em 80 ms, quando o ultimo rolo trava.
    float ag = girando && ligado ? 1 : 0;
    f_giro += (ag - f_giro) * fminf(1, dt / (girando ? 0.02f : 0.08f));
    float d = rufando || girando ? 0.45f : 1;
    duck += (d - duck) * fminf(1, dt / 0.25f);

    portENTER_CRITICAL(&trava);
    voz_t *r = &vozes[V_RUFO], *g = &vozes[V_GIRO];
    r->vol = (int)(f_rufo * 0.8f * 256);
    if (!rufando && f_rufo < 0.01f) r->t = -1;
    g->vol = (int)(f_giro * 256);
    if (!girando && f_giro < 0.01f) g->t = -1;
    vozes[V_TRILHA].vol = (int)(0.35f * duck * (ligado && musica ? 1 : 0) * 256);
    portEXIT_CRITICAL(&trava);
}
