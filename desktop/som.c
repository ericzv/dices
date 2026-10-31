// Som do PC: gravacoes que vao dentro do executavel (desktop/sons/*.ogg, que
// o CMake transforma em sons_embutidos.c). Nenhum arquivo solto acompanha o
// jogo.
//
//  - Trilha: a bossa de 16 compassos do jogo, tocada por teclado (um Rhodes
//    gravado) e baixo eletrico, com vassourinha e shaker; em loop. Abaixa
//    com o rufo, na cadeia de efeitos do fim da rodada, na hora da verdade e
//    nos climax (Jackpot, fim, premio).
//  - Camadas da trilha, do tamanho dela e no mesmo compasso: na mesa do
//    chefe entra a batucada (surdo, agogo, o aro e as cordas); na hora da
//    verdade a bossa some e ficam as cordas em tremolo no Do grave. As tres
//    vem decodificadas na memoria e um callback do audio as mistura amostra
//    a amostra: nunca saem do compasso, nem se o jogo engasgar.
//  - Ambiente: o cassino em volta, um loop por salao do Desafiante (gente
//    conversando ao longe, fichas, dados e o detalhe de cada salao). Troca
//    de um salao para o outro cruzando devagar; some na hora da verdade.
//  - Volumes: quatro canais, de 0 a 10 (o painel do som, tecla M): a trilha,
//    os efeitos da mesa, os da interface (o cursor e os menus) e o ambiente.
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
    [SOM_SUSPENSE] = "suspense", [SOM_FORTE] = "forte",
    [SOM_AD_COLA] = "ad_cola",   [SOM_AD_CHUMBO] = "ad_chumbo", [SOM_AD_REFLEXIVO] = "ad_reflexivo",
    [SOM_AD_AVESSO] = "ad_avesso", [SOM_AD_CONFETE] = "ad_confete", [SOM_AD_ELETRONICO] = "ad_eletronico",
    [SOM_AD_TRANSPARENTE] = "ad_transparente", [SOM_AD_DOURADO] = "ad_dourado", [SOM_AD_LOBO] = "ad_lobo",
    [SOM_AD_REMENDO] = "ad_remendo", [SOM_AD_RASPADINHA] = "ad_raspadinha", [SOM_AD_FERMENTO] = "ad_fermento",
    [SOM_AD_BIS] = "ad_bis",     [SOM_AD_MIRA] = "ad_mira",     [SOM_AD_GORJETA] = "ad_gorjeta",
    [SOM_AD_SELO] = "ad_selo",   [SOM_AD_FRAGIL] = "ad_fragil", [SOM_AD_ESTRELINHA] = "ad_estrelinha",
    [SOM_AD_CORRETIVO] = "ad_corretivo", [SOM_AD_IMANTADO] = "ad_imantado", [SOM_AD_POSTIT] = "ad_postit",
    [SOM_AD_HOLOGRAFICO] = "ad_holografico", [SOM_AD_PURPURINA] = "ad_purpurina", [SOM_AD_VELCRO] = "ad_velcro",
    [SOM_AD_RATOEIRA] = "ad_ratoeira", [SOM_AD_FIGURINHA] = "ad_figurinha",
    [SOM_AD_PATINHO] = "ad_patinho", [SOM_AD_ETIQUETA] = "ad_etiqueta", [SOM_AD_BILHETE] = "ad_bilhete",
    [SOM_NOVIDADE] = "novidade", [SOM_NOVIDADE_LENDA] = "novidade_lenda",
    [SOM_MENU_PASSA] = "menu_passa", [SOM_MENU_ESCOLHE] = "menu_escolhe",
    [SOM_BOLSA_ESPIA] = "bolsa_espia",
    [SOM_SINO] = "sino",         [SOM_GELO] = "gelo",         [SOM_ONDA] = "onda",
    [SOM_PRESENTE] = "presente", [SOM_ESTRELA] = "estrela",
    [SOM_CARREGA] = "revolver_carrega", [SOM_TIRO] = "revolver_tiro",
    [SOM_EVOLUI] = "cosmo_evolui", [SOM_BURACO] = "buraco_negro", [SOM_BIG_BANG] = "big_bang",
    [SOM_VAMPIRO] = "vampiro",   [SOM_SABOTADOR] = "sabotador", [SOM_COVEIRO] = "coveiro",
    [SOM_AUTOFAGIA] = "autofagia", [SOM_AMPULHETA] = "ampulheta",
    [SOM_TRANSICAO] = "transicao",
};

typedef struct {
    Sound base[VARIANTES], voz[VARIANTES][VOZES];
    int   n, prox, ultima;
} efeito_t;

static efeito_t ef[SOM_N];
static Sound rufo, moeda_rola, giro;
static Sound *rolando = &rufo;               // o loop da vez: o dado ou a moeda
static bool  pronto, falhou, ligado = true;
static bool  rufando, girando, suspense, cadeia, chefe;
static int   nivel[SOM_CANAIS] = { SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS };
#define VOL_TRILHA 0.35f                     // a trilha (e o ambiente) no volume 10

// A trilha em camadas (a "banda"): 16 bits, estereo, todas com cam_n quadros.
enum { CAM_TRILHA, CAM_CHEFE, CAM_TENSAO, CAMADAS };
static const char *const CAM_ARQ[CAMADAS] = { "trilha", "trilha_chefe", "trilha_tensao" };
static Wave  cam[CAMADAS];
static unsigned cam_n, cam_pos;              // o tamanho do loop e onde ele esta (so o callback mexe)
static AudioStream banda;
static volatile float cam_alvo[CAMADAS];     // o volume que o som_atualiza pede para cada camada
static float cam_vol[CAMADAS];               // o do fim do ultimo bloco: o callback desliza de um ao outro
static float v_chefe, v_tensao;              // quanto cada camada entrou (0..1)

// O ambiente de cada salao: um stream cada, que pausa quando ninguem ouve.
#define AMBIENTES 4
static Music amb[AMBIENTES];
static float amb_v[AMBIENTES];               // o volume de cada um, cruzando na troca de salao
static bool  amb_comecou[AMBIENTES];         // ja tocou uma vez: depois de pausar, continua de onde parou
static int   amb_quer = -1;                  // o salao de agora (-1: nenhum)
static float amb_baixa = 1;                  // a sala calando na hora da verdade

// O ganho de um canal, na escala do ouvido (metade do nivel nao e metade do
// volume que se ouve); o 10 e o volume de sempre.
static float ganho(int c) { return nivel[c] <= 0 ? 0 : powf((float)nivel[c] / SOM_NIVEIS, 1.7f); }
static int canal_de(int s)
{
    return s == SOM_TIQUE || s == SOM_PASSA || s == SOM_MENU_PASSA || s == SOM_MENU_ESCOLHE || s == SOM_BOLSA_ESPIA
                   || s == SOM_TRANSICAO
               ? SOM_CANAL_INTERFACE : SOM_CANAL_EFEITOS;
}
static float vol_rufo, vol_giro, duck = 1;   // duck: quanto a trilha abaixa
static float t_climax;                       // > 0: um climax tocando (a trilha abaixa ate ele acabar)

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

// O callback do audio (na thread dele): soma as camadas, cada uma no seu
// volume, deslizando ao longo do bloco do volume de antes ao pedido agora
// (nada de degrau). Escreve float estereo.
static void mistura_banda(void *saida, unsigned int quadros)
{
    float *o = saida;
    memset(o, 0, quadros * 2 * sizeof *o);
    for (int c = 0; c < CAMADAS; c++) {
        float v0 = cam_vol[c], v1 = cam_alvo[c];
        cam_vol[c] = v1;
        if (!cam[c].data || (v0 <= 0 && v1 <= 0)) continue;
        const short *x = cam[c].data;
        float g = v0 / 32768.0f, dg = (v1 - v0) / 32768.0f / quadros;
        unsigned p = cam_pos;
        for (unsigned k = 0; k < quadros; k++, g += dg) {
            o[2 * k] += x[2 * p] * g;
            o[2 * k + 1] += x[2 * p + 1] * g;
            if (++p == cam_n) p = 0;
        }
    }
    cam_pos = (cam_pos + quadros) % cam_n;
}

// A banda: as camadas na memoria e o stream que as toca. Uma camada de outro
// tamanho que a trilha sairia do compasso: fica de fora.
static void monta_banda(void)
{
    for (int c = 0; c < CAMADAS; c++) {
        const som_arquivo_t *a = acha(CAM_ARQ[c]);
        if (!a) continue;
        cam[c] = LoadWaveFromMemory(".ogg", a->dados, a->tam);
        if (cam[c].data) WaveFormat(&cam[c], 44100, 16, 2);
    }
    if (!cam[CAM_TRILHA].data || !cam[CAM_TRILHA].frameCount) return;
    cam_n = cam[CAM_TRILHA].frameCount;
    for (int c = 1; c < CAMADAS; c++)
        if (cam[c].data && cam[c].frameCount != cam_n) { UnloadWave(cam[c]); cam[c] = (Wave){ 0 }; }
    banda = LoadAudioStream(44100, 32, 2);
    SetAudioStreamCallback(banda, mistura_banda);
    PlayAudioStream(banda);
}

static void monta_ambiente(void)
{
    for (int a = 0; a < AMBIENTES; a++) {
        char nome[24];
        snprintf(nome, sizeof nome, "ambiente_%d", a);
        const som_arquivo_t *f = acha(nome);
        if (!f) continue;
        // O raylib le o OGG aos poucos, direto da memoria do executavel.
        amb[a] = LoadMusicStreamFromMemory(".ogg", f->dados, f->tam);
        amb[a].looping = true;
    }
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
    monta_banda();
    monta_ambiente();
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
    if (cam_n) UnloadAudioStream(banda);      // antes das camadas: o callback para de ler
    for (int c = 0; c < CAMADAS; c++) UnloadWave(cam[c]);
    for (int a = 0; a < AMBIENTES; a++)
        if (amb[a].frameCount) UnloadMusicStream(amb[a]);
    pronto = false;
}

void som_atualiza(float dt)
{
    if (!pronto) return;
    // O rufo entra em 60 ms e sai em 120 ms; a trilha abaixa enquanto ele toca.
    float alvo = rufando && ligado ? 1 : 0;
    vol_rufo += (alvo - vol_rufo) * fminf(1, dt / (rufando ? 0.06f : 0.12f));
    if (IsSoundPlaying(*rolando)) {
        SetSoundVolume(*rolando, vol_rufo * 0.8f * ganho(SOM_CANAL_EFEITOS));
        if (!rufando && vol_rufo < 0.01f) StopSound(*rolando);
    }
    // O giro entra na hora e sai em 80 ms, quando o ultimo rolo trava.
    float ag = girando && ligado ? 1 : 0;
    vol_giro += (ag - vol_giro) * fminf(1, dt / (girando ? 0.02f : 0.08f));
    if (IsSoundPlaying(giro)) {
        SetSoundVolume(giro, vol_giro * ganho(SOM_CANAL_EFEITOS));
        if (!girando && vol_giro < 0.01f) StopSound(giro);
    }
    // Na cadeia de efeitos do fim da rodada a trilha abaixa, para cada poder
    // se ouvir; na hora da verdade quase some; nos climax (Jackpot, fim da
    // partida, premio) abaixa enquanto eles tocam. Depois volta sem pressa.
    if (t_climax > 0) t_climax -= dt;
    float d = cadeia ? 0.5f : 1;
    if (rufando || girando) d = fminf(d, 0.45f);
    if (suspense) d = 0.08f;
    if (t_climax > 0 && d > 0.35f) d = 0.35f;
    duck += (d - duck) * fminf(1, dt / (d > duck && duck < 0.6f ? 0.9f : 0.25f));
    // As camadas: a do chefe entra em uns 2 s e sai em 1,5; as cordas da
    // hora da verdade chegam em meio segundo e saem junto com a bossa
    // voltando.
    v_chefe += ((chefe ? 1 : 0) - v_chefe) * fminf(1, dt / (chefe ? 0.7f : 0.5f));
    v_tensao += ((suspense ? 1 : 0) - v_tensao) * fminf(1, dt / (suspense ? 0.18f : 0.4f));
    float m = VOL_TRILHA * ganho(SOM_CANAL_MUSICA) * (ligado ? 1 : 0);
    cam_alvo[CAM_TRILHA] = m * duck;
    cam_alvo[CAM_CHEFE] = m * duck * v_chefe;
    cam_alvo[CAM_TENSAO] = m * v_tensao;
    // O ambiente: o salao de agora entra em 1,5 s, o outro sai em 1 s (os
    // dois cruzam); na hora da verdade a sala quase se cala.
    float ab = suspense ? 0.25f : 1;
    amb_baixa += (ab - amb_baixa) * fminf(1, dt / (ab < amb_baixa ? 0.15f : 0.6f));
    float sala = VOL_TRILHA * ganho(SOM_CANAL_AMBIENTE) * (ligado ? 1 : 0);
    for (int a = 0; a < AMBIENTES; a++) {
        if (!amb[a].frameCount) continue;
        float alvo = a == amb_quer && sala > 0 ? 1 : 0;
        amb_v[a] += (alvo - amb_v[a]) * fminf(1, dt / (alvo > amb_v[a] ? 0.5f : 0.35f));
        if (alvo == 0 && amb_v[a] < 0.003f) {   // ninguem ouve: pausa (e volta de onde parou)
            amb_v[a] = 0;
            if (IsMusicStreamPlaying(amb[a])) PauseMusicStream(amb[a]);
            continue;
        }
        if (!IsMusicStreamPlaying(amb[a])) {
            if (amb_comecou[a]) ResumeMusicStream(amb[a]);
            else PlayMusicStream(amb[a]);
            amb_comecou[a] = true;
        }
        SetMusicVolume(amb[a], sala * amb_v[a] * amb_baixa);
        UpdateMusicStream(amb[a]);
    }
}

// Toca o som s no tom tom (1: o gravado). Cada voz guarda o ultimo tom que
// recebeu, entao todo toque diz o seu.
static void toca(int s, float tom)
{
    efeito_t *e = &ef[s];
    int v = 0;
    if (e->n > 1) {                          // sorteia outra gravacao, nunca a mesma de antes
        v = GetRandomValue(0, e->n - 2);
        if (v >= e->ultima) v++;
        e->ultima = v;
    }
    Sound voz = e->voz[v][e->prox++ % VOZES];
    SetSoundPitch(voz, tom);
    SetSoundVolume(voz, ganho(canal_de(s)));
    PlaySound(voz);
    if (s == SOM_JACKPOT || s == SOM_FIM || s == SOM_DERROTA || s == SOM_PREMIO) {
        float dur = voz.stream.sampleRate ? (float)voz.frameCount / voz.stream.sampleRate / tom : 1;
        if (dur > t_climax) t_climax = dur;
    }
}

void som_toca(int s)
{
    if (!pronto || !ligado || s < 0 || s >= SOM_N || !ef[s].n || !nivel[canal_de(s)]) return;
    // Batidas, pousos e fichas variam um pouco de tom.
    bool varia = s == SOM_BATE || s == SOM_BATE_DADO || s == SOM_POUSA || s == SOM_FICHA ||
                 s == SOM_MOEDA_BATE || s == SOM_MOEDA_POUSA;
    toca(s, varia ? 0.9f + 0.2f * (float)GetRandomValue(0, 1000) / 1000.0f : 1.0f);
}

// Um semitom e a raiz duodecima de 2: doze sobem uma oitava.
void som_toca_tom(int s, int semitons)
{
    if (!pronto || !ligado || s < 0 || s >= SOM_N || !ef[s].n || !nivel[canal_de(s)]) return;
    toca(s, powf(2.0f, semitons / 12.0f));
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

void som_suspense(bool on) { suspense = on; }
void som_cadeia(bool on)   { cadeia = on; }
void som_chefe(bool on)    { chefe = on; }
void som_ambiente(int salao) { amb_quer = salao >= 0 && salao < AMBIENTES ? salao : -1; }

void som_volume(int c, int n)
{
    if (c >= 0 && c < SOM_CANAIS) nivel[c] = n < 0 ? 0 : n > SOM_NIVEIS ? SOM_NIVEIS : n;
}
int som_volume_de(int c) { return c >= 0 && c < SOM_CANAIS ? nivel[c] : 0; }

bool som_liga(bool on)  { ligado = on && pronto; return ligado; }
// Ligado: o aparelho tem som e algum canal se ouve.
bool som_ligado(void)
{
    for (int c = 0; c < SOM_CANAIS; c++)
        if (nivel[c]) return ligado;
    return false;
}
bool som_falhou(void)   { return falhou; }
