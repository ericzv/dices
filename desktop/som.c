// Som do PC: gravacoes que vao dentro do executavel (desktop/sons/*.ogg, que
// o CMake transforma em sons_embutidos.c). Nenhum arquivo solto acompanha o
// jogo.
//
//  - Trilha: a bossa de 16 compassos do jogo, tocada por teclado (um Rhodes
//    gravado) e baixo eletrico, com vassourinha e shaker; em loop.
//  - Rufo: o dado rolando no feltro enquanto rola; abaixa a trilha. Uma
//    moeda tem o seu: a beirada tocando o feltro a cada volta.
//  - Giro: o arpejo do caca-niquel enquanto a roleta do premio gira.
//  - Efeitos: dados, feltro, madeira e fichas de verdade; as jogadas e a
//    partida no mesmo teclado e baixo da trilha. Batidas, pousos e fichas
//    sorteiam entre varias gravacoes e variam um pouco o tom.
//
// Como os sons foram feitos: tools/sons/. Creditos: desktop/sons/CREDITOS.md.
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "hal/som.h"
#include "plataforma.h"
#include "sons_embutidos.h"

#define VOZES     4                          // copias de cada gravacao, para sobrepor
#define VARIANTES 5                          // gravacoes diferentes do mesmo som, no maximo

// O arquivo de cada efeito, sem o .ogg. Quem tem variantes vem como
// nome_0, nome_1, ...
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
    Sound base[VARIANTES], voz[VARIANTES][VOZES];
    int   n, prox, ultima;
} efeito_t;

static efeito_t ef[SOM_N];
static Sound rufo, moeda_rola, giro;
static Sound *rolando = &rufo;               // o loop da vez: o dado ou a moeda
static Music trilha;
static bool  pronto, falhou, ligado = true, musica = true;
static bool  rufando, girando;
static float vol_rufo, vol_giro, duck = 1;   // duck: quanto a trilha abaixa

static const som_arquivo_t *acha(const char *nome)
{
    for (int i = 0; i < SONS_N; i++)
        if (strcmp(SONS_ARQ[i].nome, nome) == 0) return &SONS_ARQ[i];
    return NULL;
}

static Sound carrega(const som_arquivo_t *a)
{
    Sound s = { 0 };
    if (!a) return s;
    Wave w = LoadWaveFromMemory(".ogg", a->dados, a->tam);
    s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

static void monta_efeito(int s)
{
    efeito_t *e = &ef[s];
    const som_arquivo_t *a = acha(NOME[s]);
    if (a) e->base[e->n++] = carrega(a);
    else
        for (int v = 0; v < VARIANTES; v++) {
            char nome[32];
            snprintf(nome, sizeof nome, "%s_%d", NOME[s], v);
            if (!(a = acha(nome))) break;
            e->base[e->n++] = carrega(a);
        }
    for (int v = 0; v < e->n; v++)
        for (int k = 0; k < VOZES; k++) e->voz[v][k] = LoadSoundAlias(e->base[v]);
}

void som_inicia(void)
{
    if (!IsAudioDeviceReady()) { falhou = true; ligado = false; return; }
    for (int s = 0; s < SOM_N; s++) monta_efeito(s);
    rufo = carrega(acha("rufo"));
    moeda_rola = carrega(acha("moeda_rola"));
    giro = carrega(acha("giro"));
    const som_arquivo_t *t = acha("trilha");
    if (t) {
        // O raylib le o OGG aos poucos, direto da memoria do executavel.
        trilha = LoadMusicStreamFromMemory(".ogg", t->dados, t->tam);
        trilha.looping = true;
        SetMusicVolume(trilha, 0.35f);
        PlayMusicStream(trilha);
    }
    pronto = true;
}

void som_encerra(void)
{
    if (!pronto) return;
    for (int s = 0; s < SOM_N; s++)
        for (int v = 0; v < ef[s].n; v++) {
            for (int k = 0; k < VOZES; k++) UnloadSoundAlias(ef[s].voz[v][k]);
            UnloadSound(ef[s].base[v]);
        }
    UnloadSound(rufo);
    UnloadSound(moeda_rola);
    UnloadSound(giro);
    UnloadMusicStream(trilha);
    pronto = false;
}

void som_atualiza(float dt)
{
    if (!pronto) return;
    // O rufo entra em 60 ms e sai em 120 ms; a trilha abaixa enquanto ele toca.
    float alvo = rufando && ligado ? 1 : 0;
    vol_rufo += (alvo - vol_rufo) * fminf(1, dt / (rufando ? 0.06f : 0.12f));
    if (IsSoundPlaying(*rolando)) {
        SetSoundVolume(*rolando, vol_rufo * 0.8f);
        if (!rufando && vol_rufo < 0.01f) StopSound(*rolando);
    }
    // O giro entra na hora e sai em 80 ms, quando o ultimo rolo trava.
    float ag = girando && ligado ? 1 : 0;
    vol_giro += (ag - vol_giro) * fminf(1, dt / (girando ? 0.02f : 0.08f));
    if (IsSoundPlaying(giro)) {
        SetSoundVolume(giro, vol_giro);
        if (!girando && vol_giro < 0.01f) StopSound(giro);
    }
    float d = rufando || girando ? 0.45f : 1;
    duck += (d - duck) * fminf(1, dt / 0.25f);
    SetMusicVolume(trilha, 0.35f * duck * (ligado && musica ? 1 : 0));
    UpdateMusicStream(trilha);
}

void som_toca(int s)
{
    if (!pronto || !ligado || s < 0 || s >= SOM_N || !ef[s].n) return;
    efeito_t *e = &ef[s];
    int v = 0;
    if (e->n > 1) {                          // sorteia outra gravacao, nunca a mesma de antes
        v = GetRandomValue(0, e->n - 2);
        if (v >= e->ultima) v++;
        e->ultima = v;
    }
    Sound voz = e->voz[v][e->prox++ % VOZES];
    // Batidas, pousos e fichas tambem variam um pouco de tom.
    if (s == SOM_BATE || s == SOM_BATE_DADO || s == SOM_POUSA || s == SOM_FICHA ||
        s == SOM_MOEDA_BATE || s == SOM_MOEDA_POUSA)
        SetSoundPitch(voz, 0.9f + 0.2f * (float)GetRandomValue(0, 1000) / 1000.0f);
    PlaySound(voz);
}

static void comeca_a_rolar(Sound *s)
{
    if (rufando && rolando == s) return;
    StopSound(*rolando);
    rolando = s;
    vol_rufo = 0;
    SetSoundVolume(*s, 0);
    PlaySound(*s);
    rufando = true;
}

void som_rufo(bool on)
{
    if (!pronto) return;
    if (on) comeca_a_rolar(&rufo);
    else rufando = false;
}

void som_rufo_moeda(void)
{
    if (pronto) comeca_a_rolar(&moeda_rola);
}

void som_giro(bool on)
{
    if (!pronto) return;
    if (on && !girando) {
        vol_giro = 0;
        SetSoundVolume(giro, 0);
        PlaySound(giro);
    }
    girando = on;
}

bool som_liga(bool on)  { ligado = on && pronto; return ligado; }
bool som_ligado(void)   { return ligado; }
bool som_falhou(void)   { return falhou; }

bool som_musica(bool on) { musica = on; return musica; }
bool som_musica_ligada(void) { return musica; }
