// Som do PC: gravacoes que vao dentro do executavel (desktop/sons/*.ogg, que
// o CMake transforma em sons_embutidos.c). Nenhum arquivo solto acompanha o
// jogo.
//
//  - Trilha: a bossa de 16 compassos do jogo, tocada por teclado (um Rhodes
//    gravado) e baixo eletrico, com vassourinha e shaker; em loop. Abaixa
//    com o rufo, na cadeia de efeitos do fim da rodada, na hora da verdade e
//    nos climax (Jackpot, fim, premio).
//  - As camadas da trilha, do tamanho dela e no mesmo compasso: na mesa do
//    chefe entra a batucada (surdo, agogo, o aro e as cordas) e, por cima, a
//    voz do chefe (o sax da Dona, as trompas do Barao, a caixinha do Coringa,
//    os guizos do Papai Noel; o Crupie fica so com a batucada), e ja no mapa
//    e na loja de antes dela. Todas ficam decodificadas na memoria e um
//    callback do audio as mistura amostra a amostra: nunca saem do compasso,
//    nem se o jogo engasgar. Sao decodificadas aos poucos (uns pedacos por
//    quadro): a trilha logo que o jogo abre (ate ela ficar inteira, o jogo
//    segue sem musica), a batucada e a voz so quando a run chega num salao.
//    No navegador os arquivos das trilhas nao vao dentro do jogo: ficam ao
//    lado da pagina e sao buscados depois que ele abre. Na hora da verdade a
//    trilha so quase some, e o coracao (o suspense) toca sozinho.
//  - Volumes: tres canais, de 0 a 10 (o painel do som, tecla M): a trilha,
//    os efeitos da mesa e os da interface (o cursor e os menus).
//  - Rufo: o dado rolando no feltro enquanto rola, no som do acabamento
//    dele (a pedra aspera, o metal, a madeira oca, o macio abafado...);
//    abaixa a trilha. Uma moeda tem o seu: a beirada tocando o feltro a cada
//    volta.
//  - Giro: o arpejo do caca-niquel enquanto a roleta do premio gira.
//  - Efeitos: dados, feltro, madeira e fichas de verdade; as jogadas e a
//    partida no mesmo teclado e baixo da trilha. Batidas, pousos e fichas
//    sorteiam entre varias gravacoes e variam um pouco o tom.
//
//  - A carga: decodificados, os efeitos todos passam de 100 MB. Ficam sempre
//    na memoria so os de todo dia (o menu, a mesa, as fichas, os vereditos);
//    os dos dados, dos adesivos e dos acabamentos entram quando a cena pode
//    pedi-los (as duas bolsas, a loja: o jogo diz quais, som_precisa) e saem
//    um tempo depois de deixarem de ser pedidos. Tudo e decodificado aos
//    poucos, em pedacos que cabem em 2 ms por quadro. Um som que toca sem
//    estar na memoria e decodificado na hora (nunca fica mudo).
//
// Como os sons foram feitos: tools/sons/. Creditos: desktop/sons/CREDITOS.md.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "hal/som.h"
#include "plataforma.h"
#include "sons_embutidos.h"
#ifdef __EMSCRIPTEN__
#include <stdlib.h>
#include <emscripten.h>
#endif

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
    [SOM_AD_NOTA_DEZ] = "ad_nota_dez", [SOM_AD_CARBONO] = "ad_carbono", [SOM_AD_BOMBA] = "ad_bomba",
    [SOM_AD_IMPERMEAVEL] = "ad_impermeavel", [SOM_AD_LENTICULAR] = "ad_lenticular", [SOM_AD_NEON] = "ad_neon",
    [SOM_AD_INFECTADO] = "ad_infectado", [SOM_AD_BARRO] = "ad_barro", [SOM_AD_SUJEIRA] = "ad_sujeira", [SOM_AD_CHICLETE] = "ad_chiclete",
    [SOM_AD_MASSINHA] = "ad_massinha", [SOM_AD_CARIMBO] = "ad_carimbo", [SOM_AD_DUPLA] = "ad_dupla", [SOM_AD_CASCA] = "ad_casca",
    [SOM_NOVIDADE] = "novidade", [SOM_NOVIDADE_LENDA] = "novidade_lenda",
    [SOM_MENU_PASSA] = "menu_passa", [SOM_MENU_ESCOLHE] = "menu_escolhe",
    [SOM_BOLSA_ESPIA] = "bolsa_espia",
    [SOM_SINO] = "sino",         [SOM_GELO] = "gelo",         [SOM_ONDA] = "onda",
    [SOM_PRESENTE] = "presente", [SOM_ESTRELA] = "estrela",
    [SOM_CARREGA] = "revolver_carrega", [SOM_TIRO] = "revolver_tiro",
    [SOM_EVOLUI] = "cosmo_evolui", [SOM_BURACO] = "buraco_negro", [SOM_BIG_BANG] = "big_bang",
    [SOM_VAMPIRO] = "vampiro",   [SOM_SABOTADOR] = "sabotador", [SOM_COVEIRO] = "coveiro",
    [SOM_AUTOFAGIA] = "autofagia", [SOM_AMPULHETA] = "ampulheta",
    [SOM_TRANSICAO] = "transicao", [SOM_AGNUS] = "agnus",
    [SOM_AD_CORRENTE] = "ad_corrente", [SOM_AD_DINAMITE] = "ad_dinamite",
    [SOM_AD_BUSSOLA] = "ad_bussola", [SOM_AD_MOLDURA] = "ad_moldura", [SOM_AD_XEROX] = "ad_xerox",
    [SOM_AD_DINAMITE_APAGA] = "ad_dinamite_apaga",
    [SOM_PIAO] = "piao", [SOM_GELATINA] = "gelatina", [SOM_MATRIOSKA] = "matrioska", [SOM_BALAO] = "balao",
    [SOM_LEQUE] = "leque", [SOM_PAPAGAIO] = "papagaio", [SOM_RETRATO] = "retrato", [SOM_ESTILINGUE] = "estilingue",
    [SOM_AD_COLEIRA] = "ad_coleira", [SOM_AD_TIQUETE] = "ad_tiquete",
    [SOM_CONTA] = "conta", [SOM_TUDO_NADA] = "tudo_nada", [SOM_TUDO_NADA_PERDE] = "tudo_nada_perde",
    [SOM_SENTINELA] = "sentinela", [SOM_AGIOTA] = "agiota", [SOM_OCULTO] = "oculto", [SOM_ESPINHOSO] = "espinhoso", [SOM_PAVIO] = "pavio",
    [SOM_CARROSSEL] = "carrossel", [SOM_ELEVADOR] = "elevador_sino",
    [SOM_POUSA_LACA] = "pousa_laca", [SOM_BATE_LACA] = "bate_laca", [SOM_BATE_DADO_LACA] = "bate_dado_laca",
    [SOM_POUSA_VIDRO] = "pousa_vidro", [SOM_BATE_VIDRO] = "bate_vidro", [SOM_BATE_DADO_VIDRO] = "bate_dado_vidro",
    [SOM_POUSA_GELO] = "pousa_gelo", [SOM_BATE_GELO] = "bate_gelo", [SOM_BATE_DADO_GELO] = "bate_dado_gelo",
    [SOM_POUSA_PEDRA] = "pousa_pedra", [SOM_BATE_PEDRA] = "bate_pedra", [SOM_BATE_DADO_PEDRA] = "bate_dado_pedra",
    [SOM_POUSA_METAL] = "pousa_metal", [SOM_BATE_METAL] = "bate_metal", [SOM_BATE_DADO_METAL] = "bate_dado_metal",
    [SOM_POUSA_MADEIRA] = "pousa_madeira", [SOM_BATE_MADEIRA] = "bate_madeira", [SOM_BATE_DADO_MADEIRA] = "bate_dado_madeira",
    [SOM_POUSA_MACIO] = "pousa_macio", [SOM_BATE_MACIO] = "bate_macio", [SOM_BATE_DADO_MACIO] = "bate_dado_macio",
    [SOM_POUSA_SECO] = "pousa_seco", [SOM_BATE_SECO] = "bate_seco", [SOM_BATE_DADO_SECO] = "bate_dado_seco",
    [SOM_POUSA_LOUCA] = "pousa_louca", [SOM_BATE_LOUCA] = "bate_louca", [SOM_BATE_DADO_LOUCA] = "bate_dado_louca",
    [SOM_POUSA_BORRACHA] = "pousa_borracha", [SOM_BATE_BORRACHA] = "bate_borracha", [SOM_BATE_DADO_BORRACHA] = "bate_dado_borracha",
    [SOM_POUSA_NOITE] = "pousa_noite", [SOM_BATE_NOITE] = "bate_noite", [SOM_BATE_DADO_NOITE] = "bate_dado_noite",
    [SOM_POUSA_PAPEL] = "pousa_papel", [SOM_BATE_PAPEL] = "bate_papel", [SOM_BATE_DADO_PAPEL] = "bate_dado_papel",
    [SOM_POUSA_GRAO] = "pousa_grao", [SOM_BATE_GRAO] = "bate_grao", [SOM_BATE_DADO_GRAO] = "bate_dado_grao",
    [SOM_POUSA_FURIA] = "pousa_furia", [SOM_POUSA_TREMOR] = "pousa_tremor", [SOM_SOPRO] = "sopro", [SOM_TRENO_SINO] = "treno_sino",
    [SOM_AS_OCO] = "as_oco", [SOM_AS_ECO] = "as_eco", [SOM_AS_BIGORNA] = "as_bigorna",
    [SOM_AS_ENTERRA] = "as_enterra", [SOM_AS_RETA] = "as_reta", [SOM_AS_ATRAI] = "as_atrai",
    [SOM_AS_CASCO] = "as_casco", [SOM_AS_ASAS] = "as_asas",
    [SOM_MOEDA_POUSA_METAL] = "moeda_pousa_metal", [SOM_MOEDA_BATE_METAL] = "moeda_bate_metal",
    [SOM_AS_ESPINHO] = "as_espinho", [SOM_AS_TAMBOR] = "as_tambor", [SOM_AS_TRAVA] = "as_trava",
    [SOM_AS_PASSO] = "as_passo",
    [SOM_HOLOFOTE] = "holofote",
};
// Os lacos do dado rolando de cada acabamento (o primeiro e o rufo).
static const char *const ROLA_ARQ[SOM_ROLAS] = {
    [SOM_ROLA_FELTRO] = "rufo", [SOM_ROLA_VIDRO] = "rola_vidro", [SOM_ROLA_GELO] = "rola_gelo",
    [SOM_ROLA_PEDRA] = "rola_pedra", [SOM_ROLA_METAL] = "rola_metal", [SOM_ROLA_MADEIRA] = "rola_madeira",
    [SOM_ROLA_MACIO] = "rola_macio", [SOM_ROLA_SECO] = "rola_seco", [SOM_ROLA_LOUCA] = "rola_louca",
    [SOM_ROLA_BORRACHA] = "rola_borracha", [SOM_ROLA_PAPEL] = "rola_papel", [SOM_ROLA_GRAO] = "rola_grao",
    [SOM_ROLA_FURIA] = "rola_furia", [SOM_ROLA_ONDA] = "rola_onda", [SOM_ROLA_MOEDA_METAL] = "rola_moeda_metal",
    [SOM_ROLA_ARRASTA] = "rola_arrasta",
};

typedef struct {
    Sound base[VARIANTES], voz[VARIANTES][VOZES];
    int   n, prox, ultima;
    bool  feito;                             // ja decodificado (aos poucos, ou no primeiro toque)
} efeito_t;

static efeito_t ef[SOM_N];
// Os lacos: o rufo, a moeda rolando, o giro da roleta e o motor do elevador;
// depois, os do acabamento (o 0 deles e o rufo: fica vazio).
enum { LACO_RUFO, LACO_MOEDA, LACO_GIRO, LACO_MOTOR, LACOS };
static Sound lacos[LACOS + SOM_ROLAS];
static bool laco_feito[LACOS + SOM_ROLAS];
#define rufo       lacos[LACO_RUFO]
#define moeda_rola lacos[LACO_MOEDA]
#define giro       lacos[LACO_GIRO]
#define motor      lacos[LACO_MOTOR]
// Um item da carga: um efeito (0..SOM_N-1) ou um laco (SOM_N + o indice em lacos).
#define ITENS (SOM_N + LACOS + SOM_ROLAS)
static uint8_t quer[ITENS];                  // a cena pode pedir (som_precisa)
static uint8_t a_pedido[ITENS];              // veio aos poucos, a pedido: sai quando deixa de ser pedido
static float sem_pedido[ITENS];              // ha quanto tempo deixou de ser pedido
#define SOLTA_DEPOIS 90.0f                   // segundos sem ser pedido (e sem tocar) ate sair da memoria
static long mem_sons;                        // bytes dos sons decodificados (o medidor de quadros mostra)
static int  na_hora;                         // quantos tiveram de ser decodificados na hora de tocar
static Sound *rolando = &rufo;               // o loop da vez: o dado ou a moeda
static Sound *saindo;                        // o laco que sai quando outro entra no meio do lance (o arrasto da Ancora)
static float forca_rufo = 1, vol_saindo;     // o laco da vez mais baixo (o arrasto perdendo a forca)
static bool  pronto, falhou, ligado = true;
static bool  rufando, girando, motor_on, suspense, cadeia;
static int   chefe_qual = -1;              // o som_chefe de agora
static bool  chefe_toca;
static int   nivel[SOM_CANAIS] = { SOM_NIVEIS, SOM_NIVEIS, SOM_NIVEIS };
#define VOL_TRILHA 0.35f                     // a trilha no volume 10
#define VOL_BATUCADA 0.708f                  // a batucada do chefe 3 dB abaixo da trilha: as tres juntas nao estouram
// O ajuste de ouvido de cada chefe, por cima do volume medido no gera.py: o
// Crupie nao tem voz, entao o dele vai na batucada; o dos outros, na voz.
// (Com eles, o pico da trilha + batucada + voz fica abaixo de 0,46.)
static const float AJUSTE_CHEFE[SOM_CHEFES] = {
    [SOM_CHEFE_CRUPIE] = 1.15f, [SOM_CHEFE_DONA] = 1.05f, [SOM_CHEFE_BARAO] = 1.10f,
    [SOM_CHEFE_CORINGA] = 1.10f, [SOM_CHEFE_NOEL] = 1.0f };

// A trilha em camadas (a "banda"): 16 bits, estereo, todas com cam_n quadros.
// A trilha fica na memoria desde que acaba de ser decodificada; a batucada e
// a voz (a do chefe do salao, trocada quando ele muda), so durante a run.
enum { CAM_TRILHA, CAM_CHEFE, CAM_VOZ, CAMADAS };
// Os arquivos das camadas: a trilha, a batucada e a voz de cada chefe.
#define ARQ_TRILHAS (2 + SOM_CHEFES)
static const char *const TRILHA_ARQ[ARQ_TRILHAS] = {
    "trilha", "trilha_chefe",
    [2 + SOM_CHEFE_DONA] = "trilha_dona", [2 + SOM_CHEFE_BARAO] = "trilha_barao",
    [2 + SOM_CHEFE_CORINGA] = "trilha_coringa", [2 + SOM_CHEFE_NOEL] = "trilha_noel",
};
static short *cam_buf[CAMADAS];              // cam_n quadros estereo cada
static volatile bool cam_ok[CAMADAS];        // inteira no cam_buf: o callback pode tocar
static bool cam_falhou[CAMADAS];             // o arquivo nao serve (outro tamanho, outra taxa): fica de fora
static unsigned cam_n, cam_pos;              // o tamanho do loop e onde ele esta (so o callback mexe)
static AudioStream banda;
static bool banda_on;                        // o stream existe (a trilha ja esta inteira)
static volatile float cam_alvo[CAMADAS];     // o volume que o som_atualiza pede para cada camada
static float cam_vol[CAMADAS];               // o do fim do ultimo bloco: o callback desliza de um ao outro
static float v_chefe, v_voz;                 // quanto a batucada e a voz entraram (0..1)

// Tudo e decodificado aos poucos, para nao engasgar a tela: o stb_vorbis
// que vem dentro do raylib (raudio.c), uns pedacos por quadro.
typedef struct stb_vorbis stb_vorbis;
typedef struct { char *alloc_buffer; int alloc_buffer_length_in_bytes; } stb_vorbis_alloc;
stb_vorbis *stb_vorbis_open_memory(const unsigned char *data, int len, int *error, const stb_vorbis_alloc *alloc);
int stb_vorbis_get_samples_short_interleaved(stb_vorbis *f, int channels, short *buffer, int num_shorts);
void stb_vorbis_close(stb_vorbis *f);
unsigned int stb_vorbis_stream_length_in_samples(stb_vorbis *f);
typedef struct {
    unsigned int sample_rate;
    int channels;
    unsigned int setup_memory_required, setup_temp_memory_required, temp_memory_required;
    int max_frame_size;
} stb_vorbis_info;
stb_vorbis_info stb_vorbis_get_info(stb_vorbis *f);
#define PEDACO 4096                          // quadros de som por pedaco (~0,1 s; uns 0,1 ms num PC)
#define CARGA_S 0.002                        // o tempo da carga por quadro do jogo
static int voz_qual = -1;                    // a voz que esta (ou vai ficando) no cam_buf[CAM_VOZ]
#define voz_ok cam_ok[CAM_VOZ]

// Um arquivo sendo decodificado: n quadros de canais canais, lidos ate agora.
typedef struct {
    stb_vorbis *dec;
    short *buf;
    unsigned n, lidos, taxa;
    int canais;
} decod_t;

// Abre o arquivo. canais 0: os do arquivo. buf NULL: aloca (decod_fecha solta).
static bool decod_abre(decod_t *d, const unsigned char *dados, int tam, int canais, short *buf)
{
    int erro = 0;
    memset(d, 0, sizeof *d);
    d->dec = stb_vorbis_open_memory(dados, tam, &erro, NULL);
    if (!d->dec) return false;
    stb_vorbis_info inf = stb_vorbis_get_info(d->dec);
    d->canais = canais ? canais : inf.channels;
    d->taxa = inf.sample_rate;
    d->n = stb_vorbis_stream_length_in_samples(d->dec);
    d->buf = buf ? buf : MemAlloc((unsigned)(d->n ? d->n : 1) * d->canais * sizeof(short));
    if (d->buf) return true;
    stb_vorbis_close(d->dec);
    d->dec = NULL;
    return false;
}
// Mais um pedaco. Devolve true quando o arquivo acabou.
static bool decod_anda(decod_t *d)
{
    unsigned falta = d->n - d->lidos, n = falta < PEDACO ? falta : PEDACO;
    int lidos = n ? stb_vorbis_get_samples_short_interleaved(d->dec, d->canais, d->buf + (size_t)d->canais * d->lidos, (int)n * d->canais) : 0;
    if (lidos > 0) d->lidos += (unsigned)lidos;
    return !(lidos > 0 && d->lidos < d->n);
}
static void decod_fecha(decod_t *d, bool solta)
{
    if (d->dec) stb_vorbis_close(d->dec);
    if (solta) MemFree(d->buf);
    memset(d, 0, sizeof *d);
}

// O ganho de um canal, na escala do ouvido (metade do nivel nao e metade do
// volume que se ouve); o 10 e o volume de sempre.
static float ganho(int c) { return nivel[c] <= 0 ? 0 : powf((float)nivel[c] / SOM_NIVEIS, 1.7f); }
static int canal_de(int s)
{
    return s == SOM_TIQUE || s == SOM_PASSA || s == SOM_MENU_PASSA || s == SOM_MENU_ESCOLHE || s == SOM_BOLSA_ESPIA
                   || s == SOM_TRANSICAO
               ? SOM_CANAL_INTERFACE : SOM_CANAL_EFEITOS;
}
static float vol_rufo, vol_giro, vol_motor, duck = 1;   // duck: quanto a trilha abaixa
static float t_climax;                       // > 0: um climax tocando (a trilha abaixa ate ele acabar)

static const som_arquivo_t *acha(const char *nome)
{
    for (int i = 0; i < SONS_N; i++)
        if (strcmp(SONS_ARQ[i].nome, nome) == 0) return &SONS_ARQ[i];
    return NULL;
}

static long bytes_de(Sound s) { return (long)s.frameCount * s.stream.channels * (s.stream.sampleSize / 8); }
// O arquivo inteiro, de uma vez (o som que toca antes de a carga chegar nele).
static Sound carrega(const som_arquivo_t *a)
{
    Sound s = { 0 };
    if (!a) return s;
    Wave w = LoadWaveFromMemory(".ogg", a->dados, a->tam);
    s = LoadSoundFromWave(w);
    UnloadWave(w);
    mem_sons += bytes_de(s);
    return s;
}
// O que o decodificador juntou vira um som (e o decodificador fecha).
static Sound decod_som(decod_t *d)
{
    Sound s = { 0 };
    if (d->lidos) {
        Wave w = { .frameCount = d->lidos, .sampleRate = (int)d->taxa, .sampleSize = 16, .channels = d->canais, .data = d->buf };
        s = LoadSoundFromWave(w);
        mem_sons += bytes_de(s);
    }
    decod_fecha(d, true);
    return s;
}
static void solta_som(Sound *s)
{
    if (s->frameCount) { mem_sons -= bytes_de(*s); UnloadSound(*s); }
    *s = (Sound){ 0 };
}

// O callback do audio (na thread dele): soma as camadas, cada uma no seu
// volume, deslizando ao longo do bloco do volume de antes ao pedido agora
// (nada de degrau). Escreve float estereo.
static void mistura_banda(void *saida, unsigned int quadros)
{
    float *o = saida;
    memset(o, 0, quadros * 2 * sizeof *o);
    const short *dados[CAMADAS];
    for (int c = 0; c < CAMADAS; c++) dados[c] = cam_ok[c] ? cam_buf[c] : NULL;
    for (int c = 0; c < CAMADAS; c++) {
        float v0 = cam_vol[c], v1 = cam_alvo[c];
        cam_vol[c] = dados[c] ? v1 : 0;      // a camada que chegar entra do zero
        if (!dados[c] || (v0 <= 0 && v1 <= 0)) continue;
        const short *x = dados[c];
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

// Uma barreira com o callback: o raylib o chama com a trava do audio presa, e
// SetAudioStreamCallback pega a mesma trava. Na volta, nenhuma mistura que
// comecou antes ainda esta rodando, e as proximas veem tudo que foi escrito
// antes daqui.
static void espera_o_callback(void)
{
    if (banda_on) SetAudioStreamCallback(banda, mistura_banda);
}

// ---------------------------------------------------------------------------
// Os arquivos das trilhas. No PC vem embutidos, como os efeitos. No navegador
// ficam ao lado da pagina (trilha.ogg, trilha_chefe.ogg...) e sao buscados
// quando fazem falta: o jogo abre sem esperar por eles e segue sem musica ate
// chegarem. Se a busca falhar (a rede caiu), tenta de novo dali a pouco.
// ---------------------------------------------------------------------------
enum { TA_NADA, TA_PEDIDO, TA_PRONTO, TA_FALHOU };
static struct {
    const unsigned char *dados;
    int tam;
    uint8_t estado;
    bool buscado;                            // veio da rede: a memoria e nossa (trilha_solta)
    double depois;                           // falhou: quando tentar de novo
    float espera;                            // e quanto esperar na proxima falha (dobra ate 4 min)
} tarq[ARQ_TRILHAS];

#ifdef __EMSCRIPTEN__
EM_JS(void, web_busca, (const char *nome, int f), {
    var url = UTF8ToString(nome) + '.ogg';
    if (Module.locateFile) url = Module.locateFile(url);
    Module.dadoSons = Module.dadoSons || {};
    fetch(url).then(function (r) {
        if (!r.ok) throw new Error(r.status);
        return r.arrayBuffer();
    }).then(function (b) {
        Module.dadoSons[f] = new Uint8Array(b);
        Module._som_chegou(f, b.byteLength);
    }, function () { Module._som_chegou(f, 0); });
});
// Copia o arquivo que chegou para a memoria do jogo (p 0: so o descarta).
EM_JS(void, web_copia, (int f, unsigned char *p), {
    var b = Module.dadoSons && Module.dadoSons[f];
    if (b && p) HEAPU8.set(b, p);
    if (b) delete Module.dadoSons[f];
});
EMSCRIPTEN_KEEPALIVE void som_chegou(int f, int tam)
{
    if (f < 0 || f >= ARQ_TRILHAS || tarq[f].estado != TA_PEDIDO) return;
    unsigned char *p = tam > 0 ? malloc((size_t)tam) : NULL;
    web_copia(f, p);
    if (!p) {                                // sem rede (ou 404): tenta de novo, cada vez mais espacado
        tarq[f].espera = tarq[f].espera < 15 ? 15 : tarq[f].espera * 2 > 240 ? 240 : tarq[f].espera * 2;
        tarq[f].estado = TA_FALHOU; tarq[f].depois = GetTime() + tarq[f].espera;
        return;
    }
    tarq[f].dados = p; tarq[f].tam = tam; tarq[f].buscado = true;
    tarq[f].estado = TA_PRONTO;
}
#endif

// O arquivo f esta na memoria? Se nao, pede (no navegador) e responde que nao.
static bool trilha_arquivo(int f)
{
    if (tarq[f].estado == TA_PRONTO) return true;
    if (!TRILHA_ARQ[f]) return false;
    if (tarq[f].estado == TA_NADA) {
        const som_arquivo_t *a = acha(TRILHA_ARQ[f]);
        if (a) { tarq[f].dados = a->dados; tarq[f].tam = a->tam; tarq[f].estado = TA_PRONTO; return true; }
#ifndef __EMSCRIPTEN__
        tarq[f].estado = TA_FALHOU;          // nao veio no executavel: fica sem
        tarq[f].depois = 1e30;
#endif
    }
#ifdef __EMSCRIPTEN__
    if (tarq[f].estado == TA_NADA || (tarq[f].estado == TA_FALHOU && GetTime() >= tarq[f].depois)) {
        tarq[f].estado = TA_PEDIDO;
        web_busca(TRILHA_ARQ[f], f);
    }
#endif
    return false;
}
// Decodificado, o arquivo que veio da rede sai da memoria (se fizer falta de
// novo, o navegador o tem guardado).
static void trilha_solta(int f)
{
    if (!tarq[f].buscado) return;
#ifdef __EMSCRIPTEN__
    free((void *)tarq[f].dados);
#endif
    tarq[f].dados = NULL; tarq[f].tam = 0; tarq[f].buscado = false;
    tarq[f].estado = TA_NADA;
}

// ---------------------------------------------------------------------------
// A banda: as camadas na memoria e o stream que as toca, que so comeca
// quando a trilha esta inteira. Uma camada de outro tamanho (ou outra taxa)
// que a trilha sairia do compasso: fica de fora.
// ---------------------------------------------------------------------------
static decod_t cam_dec;                      // a camada sendo decodificada
static int cam_lendo = -1, cam_lendo_arq;    // qual (-1: nenhuma) e de que arquivo
static float t_sem_chefe;                    // ha quanto tempo fora da run (a batucada e a voz saem da memoria)

static void camada_abre(int c, int f)
{
    if (c != CAM_TRILHA && !cam_buf[c]) cam_buf[c] = MemAlloc(cam_n * 2 * sizeof(short));
    bool ok = (c == CAM_TRILHA || cam_buf[c]) && decod_abre(&cam_dec, tarq[f].dados, tarq[f].tam, 2, cam_buf[c]);
    if (ok && (cam_dec.taxa != 44100 || !cam_dec.n || (c != CAM_TRILHA && cam_dec.n != cam_n))) {
        decod_fecha(&cam_dec, c == CAM_TRILHA);
        ok = false;
    }
    if (!ok) { cam_falhou[c] = true; trilha_solta(f); return; }
    cam_buf[c] = cam_dec.buf;
    cam_lendo = c; cam_lendo_arq = f;
}

// Um pedaco do trabalho da banda. Devolve false quando nao ha o que fazer agora.
static bool banda_passo(void)
{
    // O chefe mudou com a voz de antes pela metade: ela e largada.
    if (cam_lendo == CAM_VOZ && voz_qual != chefe_qual) { decod_fecha(&cam_dec, false); cam_lendo = -1; }
    if (cam_lendo >= 0) {
        if (!decod_anda(&cam_dec)) return true;
        int c = cam_lendo;
        unsigned n = cam_dec.n;
        bool inteira = cam_dec.lidos == n;
        decod_fecha(&cam_dec, false);
        trilha_solta(cam_lendo_arq);
        cam_lendo = -1;
        if (!inteira) { cam_falhou[c] = true; return true; }
        if (c == CAM_TRILHA) {               // a trilha chegou: a banda comeca a tocar
            cam_n = n;
            cam_ok[c] = true;
            banda = LoadAudioStream(44100, 32, 2);
            SetAudioStreamCallback(banda, mistura_banda);
            PlayAudioStream(banda);
            banda_on = true;
        } else {
            espera_o_callback();
            cam_ok[c] = true;
        }
        return true;
    }
    if (!cam_ok[CAM_TRILHA]) {
        if (cam_falhou[CAM_TRILHA] || !trilha_arquivo(0)) return false;
        camada_abre(CAM_TRILHA, 0);
        return true;
    }
    int q = chefe_qual;
    if (q < 0 || q >= SOM_CHEFES) return false;
    // Outro chefe: a voz troca so com a de antes calada; ate a nova ficar
    // inteira, nenhuma toca.
    if (q != voz_qual && !(voz_ok && (v_voz > 0.001f || cam_vol[CAM_VOZ] > 0))) {
        voz_ok = false;
        espera_o_callback();
        voz_qual = q;
        cam_falhou[CAM_VOZ] = false;
    }
    bool falta_bat = !cam_ok[CAM_CHEFE] && !cam_falhou[CAM_CHEFE];
    bool falta_voz = voz_qual == q && !voz_ok && !cam_falhou[CAM_VOZ] && TRILHA_ARQ[2 + q];
    bool tem_bat = falta_bat && trilha_arquivo(1), tem_voz = falta_voz && trilha_arquivo(2 + q);   // as duas buscas saem juntas
    if (tem_bat) { camada_abre(CAM_CHEFE, 1); return true; }
    if (tem_voz) { camada_abre(CAM_VOZ, 2 + q); return true; }
    return false;
}

// Fora da run ha um minuto, com as duas caladas: a batucada e a voz saem da
// memoria (voltam, aos poucos, quando a run chegar de novo num salao).
static void banda_enxuga(float dt)
{
    t_sem_chefe = chefe_qual < 0 ? t_sem_chefe + dt : 0;
    if (t_sem_chefe < 60 || (!cam_buf[CAM_CHEFE] && !cam_buf[CAM_VOZ])) return;
    if (cam_lendo == CAM_CHEFE || cam_lendo == CAM_VOZ) return;
    if (v_chefe > 0.001f || v_voz > 0 || cam_vol[CAM_CHEFE] > 0 || cam_vol[CAM_VOZ] > 0) return;
    cam_ok[CAM_CHEFE] = cam_ok[CAM_VOZ] = false;
    espera_o_callback();
    MemFree(cam_buf[CAM_CHEFE]); MemFree(cam_buf[CAM_VOZ]);
    cam_buf[CAM_CHEFE] = cam_buf[CAM_VOZ] = NULL;
    voz_qual = -1;
}

// ---------------------------------------------------------------------------
// Os efeitos e os lacos
// ---------------------------------------------------------------------------
static bool item_feito(int it) { return it < SOM_N ? ef[it].feito : laco_feito[it - SOM_N]; }

// O arquivo da gravacao v do item (um laco tem uma so; um efeito, "nome" ou
// "nome_0", "nome_1"...). *unico: o efeito e de um arquivo so.
static const som_arquivo_t *arq_do_item(int it, int v, bool *unico)
{
    static const char *const ARQ[LACOS] = { "rufo", "moeda_rola", "giro", "elevador_motor" };
    if (it >= SOM_N) {
        int l = it - SOM_N;
        *unico = true;
        return v ? NULL : acha(l < LACOS ? ARQ[l] : ROLA_ARQ[l - LACOS]);
    }
    const som_arquivo_t *a = v == 0 ? acha(NOME[it]) : NULL;
    if (a) { *unico = true; return a; }
    char nome[40];
    snprintf(nome, sizeof nome, "%s_%d", NOME[it], v);
    return acha(nome);
}

static void faz_vozes(efeito_t *e)
{
    for (int v = 0; v < e->n; v++)
        for (int k = 0; k < VOZES; k++) e->voz[v][k] = LoadSoundAlias(e->base[v]);
}

// A carga aos poucos: um item de cada vez, um arquivo de cada vez, um pedaco
// de cada vez.
static struct {
    int item, v, n;
    bool unico;
    decod_t d;
    Sound base[VARIANTES];
} carga = { .item = -1 };

static int carga_adiada;                     // (carga_passo)
// Desiste do item da vez (ele vai ser decodificado na hora, inteiro).
static void carga_larga(void)
{
    if (carga.item < 0) return;
    decod_fecha(&carga.d, true);
    for (int v = 0; v < carga.n; v++) solta_som(&carga.base[v]);
    carga.item = -1;
    carga_adiada = 0;
}

static void carga_conclui(void)
{
    int it = carga.item;
    if (it < SOM_N) {
        efeito_t *e = &ef[it];
        e->n = carga.n;
        for (int v = 0; v < carga.n; v++) e->base[v] = carga.base[v];
        if (e->ultima >= e->n) e->ultima = 0;
        faz_vozes(e);
        e->feito = true;
    } else {
        lacos[it - SOM_N] = carga.n ? carga.base[0] : (Sound){ 0 };
        laco_feito[it - SOM_N] = true;
    }
    a_pedido[it] = 1;
    sem_pedido[it] = 0;
    carga.item = -1;
}

// Os primeiros da fila, na ordem: o menu, a mesa e o dado rolando. Depois
// deles, o resto do que a cena pede, na ordem da lista.
static const int16_t PRIMEIRO[] = {
    SOM_MENU_PASSA, SOM_MENU_ESCOLHE, SOM_TIQUE, SOM_TRANSICAO, SOM_PASSA, SOM_ABERTURA,
    SOM_BATE, SOM_BATE_DADO, SOM_POUSA, SOM_FICHA, SOM_VALIDO, SOM_ANULA, SOM_TURNO, SOM_PARTIDA,
    SOM_N + LACO_RUFO,
    // Os outros lacos tambem logo: pedidos so no instante de tocar (a roleta
    // do premio comecando a girar), eles vinham decodificados na hora - um
    // engasgo a cada premio no aparelho lento.
    SOM_N + LACO_GIRO, SOM_N + LACO_MOEDA, SOM_N + LACO_MOTOR,
};
#define N_PRIMEIRO ((int)(sizeof PRIMEIRO / sizeof *PRIMEIRO))
static int pri_i;                            // os PRIMEIRO antes deste ja estao feitos
static int proximo_item(void)
{
    for (; pri_i < N_PRIMEIRO; pri_i++)
        if (!item_feito(PRIMEIRO[pri_i])) return PRIMEIRO[pri_i];
    for (int it = 0; it < ITENS; it++)
        if (quer[it] && !item_feito(it)) return it;
    return -1;
}

// Um pedaco do trabalho da carga. Devolve false quando nao ha o que fazer
// (ou quando o que ha fica para o comeco do proximo quadro).
static bool carga_folgada;                   // nada da carga rodou ainda neste quadro
static double carga_orc = CARGA_S;           // o tempo da carga neste quadro: a folga que o quadro tem (som_carga_orcamento)
static bool carga_coberta;                   // a capa do site cobre a tela: pode usar mais
static int carga_adiada;                     // quadros que o som comprido esperou folga para virar som
static double carga_lim = CARGA_S;           // o tempo da carga neste quadro, ja com a capa do site (carrega_aos_poucos)
static bool carga_passo(void)
{
    if (carga.item < 0) {
        int it = proximo_item();
        if (it < 0) return false;
        carga.item = it; carga.v = carga.n = 0; carga.unico = false;
    }
    if (!carga.d.dec) {                      // abre a proxima gravacao do item (ou ele acabou)
        const som_arquivo_t *a = carga.v < VARIANTES && !(carga.unico && carga.v) ? arq_do_item(carga.item, carga.v, &carga.unico) : NULL;
        if (!a) { carga_conclui(); return true; }
        if (!decod_abre(&carga.d, a->dados, a->tam, 0, NULL)) carga.v++;     // estragado: fica sem essa
        return true;
    }
    if (carga.d.lidos < carga.d.n && !decod_anda(&carga.d)) return true;
    // Decodificado. Falta virar som do raylib, que o converte para a taxa do
    // aparelho de uma vez so: num som comprido (um laco de 4 s) isso custa
    // alguns milissegundos, e fica para o comeco de um quadro, sozinho.
    // E so quando o quadro tem folga para ele (ou depois de esperar 2 s).
    if (carga.d.lidos > 44100 && (!carga_folgada || (carga_lim < 0.004 && ++carga_adiada < 120))) return false;
    carga_adiada = 0;
    Sound s = decod_som(&carga.d);
    if (s.frameCount) carga.base[carga.n++] = s;
    carga.v++;
    return true;
}

// Na hora: o som que toca sem estar na memoria (a carga ainda nao chegou
// nele, ou a cena nao avisou que ia pedir) e decodificado inteiro, agora.
static void monta_efeito(int s)
{
    efeito_t *e = &ef[s];
    if (e->feito) return;
    if (carga.item == s) carga_larga();
    e->feito = true;
    e->n = 0;
    a_pedido[s] = 0;
    na_hora++;
    const som_arquivo_t *a = acha(NOME[s]);
    if (a) e->base[e->n++] = carrega(a);
    else
        for (int v = 0; v < VARIANTES; v++) {
            char nome[40];
            snprintf(nome, sizeof nome, "%s_%d", NOME[s], v);
            if (!(a = acha(nome))) break;
            e->base[e->n++] = carrega(a);
        }
    if (e->ultima >= e->n) e->ultima = 0;
    faz_vozes(e);
}

// Os lacos (o rufo, a moeda rolando, o giro, o motor e os do acabamento,
// LACOS + SOM_ROLA_*): tambem na hora, se a carga ainda nao chegou neles.
static Sound *laco(int i)
{
    if (!laco_feito[i]) {
        bool unico;
        if (carga.item == SOM_N + i) carga_larga();
        laco_feito[i] = true;
        a_pedido[SOM_N + i] = 0;
        na_hora++;
        lacos[i] = carrega(arq_do_item(SOM_N + i, 0, &unico));
    }
    return &lacos[i];
}

// O laco do acabamento; o SOM_ROLA_FELTRO e o rufo de sempre.
static Sound *laco_rola(int q)
{
    if (q <= 0 || q >= SOM_ROLAS) return laco(LACO_RUFO);
    Sound *s = laco(LACOS + q);
    return s->frameCount ? s : laco(LACO_RUFO);
}

static bool item_tocando(int it)
{
    if (it >= SOM_N) {
        Sound *s = &lacos[it - SOM_N];
        return s == rolando || s == saindo || IsSoundPlaying(*s);
    }
    const efeito_t *e = &ef[it];
    for (int v = 0; v < e->n; v++)
        for (int k = 0; k < VOZES; k++) if (IsSoundPlaying(e->voz[v][k])) return true;
    return false;
}

static void item_solta(int it)
{
    if (it >= SOM_N) {
        solta_som(&lacos[it - SOM_N]);
        laco_feito[it - SOM_N] = false;
        return;
    }
    efeito_t *e = &ef[it];
    for (int v = 0; v < e->n; v++) {
        for (int k = 0; k < VOZES; k++) UnloadSoundAlias(e->voz[v][k]);
        solta_som(&e->base[v]);
    }
    e->n = 0;
    e->feito = false;
}

// Uma vez por segundo: o que veio a pedido e ja nao e pedido ha um tempo
// (a partida acabou, o adesivo saiu da loja) sai da memoria, se nao estiver
// tocando. O que foi decodificado na hora fica: a cena nao soube avisar, e
// da proxima vez tocaria com outro engasgo.
static void solta_os_sem_pedido(float dt)
{
#ifndef __EMSCRIPTEN__
    (void)dt;
    return;                                  // no computador a memoria sobra: o que veio fica (sem decodificar de novo depois)
#endif
    static float t;
    if ((t += dt) < 1) return;
    float passou = t;
    t = 0;
    for (int it = 0; it < ITENS; it++) {
        if (quer[it] || !a_pedido[it] || !item_feito(it)) { sem_pedido[it] = 0; continue; }
        if ((sem_pedido[it] += passou) < SOLTA_DEPOIS || item_tocando(it) || carga.item == it) continue;
        item_solta(it);
    }
}

// O que cabe no tempo da carga deste quadro: primeiro os sons do menu e da
// mesa; depois a trilha e o resto, um pedaco de cada, revezando.
static void carrega_aos_poucos(void)
{
    double t0 = GetTime(), carga_s = carga_coberta && carga_orc < 4 * CARGA_S ? 4 * CARGA_S : carga_orc;
    carga_lim = carga_s;
    carga_folgada = true;
    for (int vez = 0; ; vez ^= 1) {
        bool fez;
        // (enquanto um som comprido da frente espera folga, a trilha anda)
        if (pri_i < N_PRIMEIRO) fez = carga_passo() || banda_passo();
        else if (vez) fez = banda_passo() || carga_passo();
        else fez = carga_passo() || banda_passo();
        carga_folgada = false;
        if (!fez || GetTime() - t0 > carga_s) break;
    }
}

// O que a cena pode pedir (o jogo diz a cada quadro: dado_sons_precisos): os
// efeitos (SOM_N marcas), os lacos do acabamento (SOM_ROLAS) e os outros.
void som_precisa(const uint8_t *som, const uint8_t *rola, bool moeda, bool roleta, bool elevador)
{
    memcpy(quer, som, SOM_N);
    quer[SOM_N + LACO_RUFO] = 1;
    // (a moeda, a roleta e o elevador ficam sempre: sao pequenos, e pedidos so
    // na hora de tocar chegavam tarde)
    (void)moeda; (void)roleta; (void)elevador;
    quer[SOM_N + LACO_MOEDA] = quer[SOM_N + LACO_GIRO] = quer[SOM_N + LACO_MOTOR] = 1;
    quer[SOM_N + LACOS] = 0;
    for (int q = 1; q < SOM_ROLAS; q++) quer[SOM_N + LACOS + q] = rola[q];
}

// Com a tela coberta (a capa do site, antes do clique) a carga pode usar
// mais tempo por quadro: ninguem ve o quadro atrasar.
void som_folga(bool on) { carga_coberta = on; }
// O tempo que a carga pode usar neste quadro: a plataforma da a folga que
// sobra depois do passo e do desenho. No quadro leve a carga anda depressa
// (no PC termina em segundos); no pesado quase nao entra.
void som_carga_orcamento(double s) { carga_orc = s < 0.0005 ? 0.0005 : s > 0.006 ? 0.006 : s; }

// Para o medidor de quadros: a memoria dos sons decodificados (KB), quantos
// tiveram de ser decodificados na hora de tocar e quantos ainda estao na fila.
void som_medida(int *kb, int *n_na_hora, int *na_fila)
{
    long b = mem_sons;
    for (int c = 0; c < CAMADAS; c++) if (cam_buf[c]) b += (long)(c == CAM_TRILHA && !cam_n ? cam_dec.n : cam_n) * 2 * (long)sizeof(short);
    int f = 0;
    for (int it = 0; it < ITENS; it++) f += quer[it] && !item_feito(it);
    *kb = (int)(b / 1024); *n_na_hora = na_hora; *na_fila = f;
}

void som_inicia(void)
{
    if (!IsAudioDeviceReady()) { falhou = true; ligado = false; return; }
    pronto = true;
}

void som_encerra(void)
{
    if (!pronto) return;
    carga_larga();
    saindo = NULL;
    rolando = &rufo;
    for (int it = 0; it < ITENS; it++) if (item_feito(it)) item_solta(it);
    if (banda_on) UnloadAudioStream(banda);  // antes das camadas: o callback para de ler
    banda_on = false;
    if (cam_lendo >= 0) decod_fecha(&cam_dec, false);
    cam_lendo = -1;
    for (int c = 0; c < CAMADAS; c++) {
        MemFree(cam_buf[c]);
        cam_buf[c] = NULL; cam_ok[c] = false; cam_falhou[c] = false;
    }
    for (int f = 0; f < ARQ_TRILHAS; f++) trilha_solta(f);
    cam_n = cam_pos = 0;
    voz_qual = -1;
    pri_i = 0;
    pronto = false;
}

void som_atualiza(float dt)
{
    if (!pronto) return;
    carrega_aos_poucos();
    solta_os_sem_pedido(dt);
    banda_enxuga(dt);
    // O rufo entra em 60 ms e sai em 120 ms; a trilha abaixa enquanto ele toca.
    // O laco que outro substituiu no meio do lance sai em 80 ms, sem corte.
    if (saindo) {
        vol_saindo -= dt / 0.08f;
        if (vol_saindo <= 0 || !ligado) { StopSound(*saindo); saindo = NULL; }
        else SetSoundVolume(*saindo, vol_saindo * 0.8f * ganho(SOM_CANAL_EFEITOS));
    }
    float alvo = rufando && ligado ? forca_rufo : 0;
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
    // O motor do elevador entra em 30 ms e sai em 120 ms, quando a cabine chega
    // (o tom sobe na propria gravacao: numa subida curta, so se ouve o comeco).
    float am = motor_on && ligado ? 1 : 0;
    vol_motor += (am - vol_motor) * fminf(1, dt / (motor_on ? 0.03f : 0.12f));
    if (IsSoundPlaying(motor)) {
        SetSoundVolume(motor, vol_motor * ganho(SOM_CANAL_EFEITOS));
        if (!motor_on && vol_motor < 0.01f) StopSound(motor);
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
    // A banda do chefe entra em uns 2 s e sai em 1,5 (cada camada, quando
    // ja esta inteira na memoria).
    float alvo_v = chefe_toca && cam_ok[CAM_CHEFE] ? 1 : 0;
    v_chefe += (alvo_v - v_chefe) * fminf(1, dt / (chefe_toca ? 0.7f : 0.5f));
    float alvo_voz = chefe_toca && voz_ok && voz_qual == chefe_qual ? 1 : 0;
    v_voz += (alvo_voz - v_voz) * fminf(1, dt / (alvo_voz > v_voz ? 0.7f : 0.5f));
    if (v_voz < 0.001f && alvo_voz == 0) v_voz = 0;
    float m = VOL_TRILHA * ganho(SOM_CANAL_MUSICA) * (ligado ? 1 : 0);
    cam_alvo[CAM_TRILHA] = m * duck;
    float aj_bat = chefe_qual == SOM_CHEFE_CRUPIE ? AJUSTE_CHEFE[SOM_CHEFE_CRUPIE] : 1;
    float aj_voz = voz_qual > SOM_CHEFE_CRUPIE && voz_qual < SOM_CHEFES ? AJUSTE_CHEFE[voz_qual] : 1;
    cam_alvo[CAM_CHEFE] = m * duck * v_chefe * VOL_BATUCADA * aj_bat;
    cam_alvo[CAM_VOZ] = m * duck * v_voz * aj_voz;
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
    if (!pronto || !ligado || s < 0 || s >= SOM_N || !nivel[canal_de(s)]) return;
    monta_efeito(s);                         // ainda nao veio: agora
    if (!ef[s].n) return;
    // Batidas, pousos e fichas variam um pouco de tom.
    bool varia = s == SOM_BATE || s == SOM_BATE_DADO || s == SOM_POUSA || s == SOM_FICHA ||
                 s == SOM_MOEDA_BATE || s == SOM_MOEDA_POUSA || s == SOM_MOEDA_BATE_METAL || s == SOM_MOEDA_POUSA_METAL ||
                 (s >= SOM_POUSA_LACA && s <= SOM_POUSA_TREMOR);
    toca(s, varia ? 0.9f + 0.2f * (float)GetRandomValue(0, 1000) / 1000.0f : 1.0f);
}

// Um semitom e a raiz duodecima de 2: doze sobem uma oitava.
void som_toca_tom(int s, int semitons)
{
    if (!pronto || !ligado || s < 0 || s >= SOM_N || !nivel[canal_de(s)]) return;
    monta_efeito(s);
    if (!ef[s].n) return;
    toca(s, powf(2.0f, semitons / 12.0f));
}

static void comeca_a_rolar(Sound *s)
{
    forca_rufo = 1;
    if (rufando && rolando == s) return;
    if (saindo) { StopSound(*saindo); saindo = NULL; }
    if (rufando && vol_rufo > 0.01f && IsSoundPlaying(*rolando)) { saindo = rolando; vol_saindo = vol_rufo; }   // o de antes sai devagar
    else StopSound(*rolando);
    rolando = s;
    vol_rufo = 0;
    SetSoundVolume(*s, 0);
    PlaySound(*s);
    rufando = true;
}

void som_rufo(bool on)
{
    if (!pronto) return;
    if (on) comeca_a_rolar(laco(LACO_RUFO));
    else rufando = false;
}

void som_rufo_moeda(void)
{
    if (pronto) comeca_a_rolar(laco(LACO_MOEDA));
}

void som_rufo_de(int q)
{
    if (pronto) comeca_a_rolar(laco_rola(q));
}

void som_rufo_forca(float f) { forca_rufo = f < 0 ? 0 : f > 1 ? 1 : f; }

void som_giro(bool on)
{
    if (!pronto) return;
    if (on && !girando) {
        laco(LACO_GIRO);
        vol_giro = 0;
        SetSoundVolume(giro, 0);
        PlaySound(giro);
    }
    girando = on;
}

void som_motor(bool on)
{
    if (!pronto) return;
    if (on && !motor_on) {
        laco(LACO_MOTOR);
        vol_motor = 0;
        SetSoundVolume(motor, 0);
        PlaySound(motor);
    }
    motor_on = on;
}

void som_suspense(bool on) { suspense = on; }
void som_cadeia(bool on)   { cadeia = on; }
void som_chefe(int qual, bool toca) { chefe_qual = qual; chefe_toca = qual >= 0 && toca; }

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
