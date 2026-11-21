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
//    e na loja de antes dela. Todas vem decodificadas na memoria e um callback
//    do audio as mistura amostra a amostra: nunca saem do compasso, nem se o
//    jogo engasgar. A voz e so a do chefe do salao, decodificada aos poucos
//    (um pedaco por quadro) quando o salao muda. Na hora da verdade a trilha
//    so quase some, e o coracao (o suspense) toca sozinho.
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
// Como os sons foram feitos: tools/sons/. Creditos: desktop/sons/CREDITOS.md.
#include <math.h>
#include <stdint.h>
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
static Sound rufo, moeda_rola, giro, motor;
static bool rufo_feito, moeda_feito, giro_feito, motor_feito;
static Sound rola[SOM_ROLAS];                // os do acabamento (o 0 fica no rufo)
static bool rola_feito[SOM_ROLAS];
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
// A trilha e a batucada ficam na memoria desde o comeco; a voz e a do chefe
// do salao (voz_buf), trocada quando ele muda.
enum { CAM_TRILHA, CAM_CHEFE, CAM_VOZ, CAMADAS };
static const char *const CAM_ARQ[CAMADAS] = { "trilha", "trilha_chefe", NULL };
static const char *const VOZ_ARQ[SOM_CHEFES] = {
    [SOM_CHEFE_DONA] = "trilha_dona", [SOM_CHEFE_BARAO] = "trilha_barao",
    [SOM_CHEFE_CORINGA] = "trilha_coringa", [SOM_CHEFE_NOEL] = "trilha_noel",
};
static Wave  cam[CAMADAS];
static unsigned cam_n, cam_pos;              // o tamanho do loop e onde ele esta (so o callback mexe)
static AudioStream banda;
static volatile float cam_alvo[CAMADAS];     // o volume que o som_atualiza pede para cada camada
static float cam_vol[CAMADAS];               // o do fim do ultimo bloco: o callback desliza de um ao outro
static float v_chefe, v_voz;                 // quanto a batucada e a voz entraram (0..1)

// A voz do chefe e decodificada aos poucos, para nao engasgar a tela: o
// stb_vorbis que vem dentro do raylib (raudio.c), um pedaco por quadro.
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
#define VOZ_PEDACO 32768                     // quadros decodificados por quadro do jogo (~0,7 s de som)
static short *voz_buf;                       // cam_n quadros estereo
static int voz_qual = -1;                    // a voz que esta (ou vai ficando) no voz_buf
static stb_vorbis *voz_dec;                  // != NULL: decodificando
static unsigned voz_lidos;
static volatile bool voz_ok;                 // inteira no voz_buf: o callback pode tocar

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
    const short *dados[CAMADAS] = { cam[CAM_TRILHA].data, cam[CAM_CHEFE].data, voz_ok ? voz_buf : NULL };
    for (int c = 0; c < CAMADAS; c++) {
        float v0 = cam_vol[c], v1 = cam_alvo[c];
        cam_vol[c] = dados[c] ? v1 : 0;      // a voz que chegar entra do zero
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
    if (cam_n) SetAudioStreamCallback(banda, mistura_banda);
}

// A voz do chefe q no voz_buf, um pedaco por chamada. Troca so com a voz de
// antes calada; ate a nova ficar inteira, nenhuma toca.
static void carrega_voz(int q)
{
    if (!cam_n) return;
    if (q != voz_qual && (q >= 0 || voz_dec)) {
        if (voz_ok && (v_voz > 0.001f || cam_vol[CAM_VOZ] > 0)) return;   // espera a de antes calar
        voz_ok = false;
        espera_o_callback();
        if (voz_dec) { stb_vorbis_close(voz_dec); voz_dec = NULL; }
        voz_qual = q;
        voz_lidos = 0;
        const som_arquivo_t *a = q >= 0 && q < SOM_CHEFES && VOZ_ARQ[q] ? acha(VOZ_ARQ[q]) : NULL;
        if (!a) return;
        if (!voz_buf) voz_buf = MemAlloc(cam_n * 2 * sizeof *voz_buf);
        int erro = 0;
        if (voz_buf) voz_dec = stb_vorbis_open_memory(a->dados, (int)a->tam, &erro, NULL);
        // De outro tamanho (ou outra taxa) que a trilha sairia do compasso: fica de fora.
        if (voz_dec && (stb_vorbis_get_info(voz_dec).sample_rate != 44100
                        || stb_vorbis_stream_length_in_samples(voz_dec) != cam_n)) {
            stb_vorbis_close(voz_dec);
            voz_dec = NULL;
        }
        return;                                // o primeiro pedaco, no proximo quadro
    }
    if (!voz_dec) return;
    unsigned falta = cam_n - voz_lidos, n = falta < VOZ_PEDACO ? falta : VOZ_PEDACO;
    int lidos = n ? stb_vorbis_get_samples_short_interleaved(voz_dec, 2, voz_buf + 2 * voz_lidos, (int)n * 2) : 0;
    voz_lidos += lidos > 0 ? (unsigned)lidos : 0;
    if (lidos > 0 && voz_lidos < cam_n) return;
    stb_vorbis_close(voz_dec);
    voz_dec = NULL;
    if (voz_lidos == cam_n) { espera_o_callback(); voz_ok = true; }
}

// A banda: as camadas na memoria e o stream que as toca. Uma camada de outro
// tamanho que a trilha sairia do compasso: fica de fora.
static void monta_banda(void)
{
    for (int c = 0; c < CAMADAS; c++) {
        const som_arquivo_t *a = CAM_ARQ[c] ? acha(CAM_ARQ[c]) : NULL;
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

static void monta_efeito(int s)
{
    efeito_t *e = &ef[s];
    if (e->feito) return;
    e->feito = true;
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

// Os lacos (o rufo, a moeda rolando, o giro, o motor): decodificados na
// primeira vez que tocam, se a carga aos poucos ainda nao tiver chegado neles.
static Sound *laco(int i)
{
    static const char *const ARQ[4] = { "rufo", "moeda_rola", "giro", "elevador_motor" };
    Sound *s[4] = { &rufo, &moeda_rola, &giro, &motor };
    bool *feito[4] = { &rufo_feito, &moeda_feito, &giro_feito, &motor_feito };
    if (!*feito[i]) { *feito[i] = true; *s[i] = carrega(acha(ARQ[i])); }
    return s[i];
}

// O laco do acabamento; o SOM_ROLA_FELTRO e o rufo de sempre.
static Sound *laco_rola(int q)
{
    if (q <= 0 || q >= SOM_ROLAS) return laco(0);
    if (!rola_feito[q]) {
        rola_feito[q] = true;
        const som_arquivo_t *a = acha(ROLA_ARQ[q]);
        rola[q] = a ? carrega(a) : (Sound){ 0 };
        if (!a) return laco(0);
    }
    return rola[q].frameCount ? &rola[q] : laco(0);
}

// Os efeitos vem aos poucos, depois da abertura: decodificar os ~150 de uma
// vez segurava a janela preta na abertura (no aparelho fraco, segundos). A
// cada quadro, so o que cabe em uns 2 ms (os do menu e da mesa primeiro); um
// som pedido antes da vez dele e decodificado na hora (nunca fica mudo).
#define PRE_LACOS SOM_N                      // na fila, depois dos efeitos: os 4 lacos
#define PRE_ROLAS (SOM_N + 4)                // e os do acabamento
static int16_t pre_fila[SOM_N + 4 + SOM_ROLAS];
static int pre_n, pre_i;
static void monta_fila_de_carga(void)
{
    static const int16_t PRIMEIRO[] = {
        SOM_MENU_PASSA, SOM_MENU_ESCOLHE, SOM_TIQUE, SOM_TRANSICAO, SOM_PASSA, SOM_ABERTURA,
        SOM_BATE, SOM_BATE_DADO, SOM_POUSA, SOM_FICHA, SOM_VALIDO, SOM_ANULA, SOM_TURNO, SOM_PARTIDA,
    };
    bool posto[SOM_N] = { false };
    pre_n = pre_i = 0;
    for (int i = 0; i < (int)(sizeof PRIMEIRO / sizeof *PRIMEIRO); i++) { pre_fila[pre_n++] = PRIMEIRO[i]; posto[PRIMEIRO[i]] = true; }
    for (int k = 0; k < 4; k++) pre_fila[pre_n++] = (int16_t)(PRE_LACOS + k);
    for (int k = 1; k < SOM_ROLAS; k++) pre_fila[pre_n++] = (int16_t)(PRE_ROLAS + k);
    for (int s = 0; s < SOM_N; s++) if (!posto[s]) pre_fila[pre_n++] = (int16_t)s;
}
static void carrega_aos_poucos(void)
{
    double t0 = GetTime();
    while (pre_i < pre_n) {
        int k = pre_fila[pre_i++];
        if (k >= PRE_ROLAS) laco_rola(k - PRE_ROLAS);
        else if (k >= PRE_LACOS) laco(k - PRE_LACOS);
        else monta_efeito(k);
        if (GetTime() - t0 > 0.002) break;
    }
}

void som_inicia(void)
{
    if (!IsAudioDeviceReady()) { falhou = true; ligado = false; return; }
    monta_fila_de_carga();
    monta_banda();
    pronto = true;
}

void som_encerra(void)
{
    if (!pronto) return;
    for (int s = 0; s < SOM_N; s++) {
        for (int v = 0; v < ef[s].n; v++) {
            for (int k = 0; k < VOZES; k++) UnloadSoundAlias(ef[s].voz[v][k]);
            UnloadSound(ef[s].base[v]);
        }
        ef[s].n = 0; ef[s].feito = false;
    }
    saindo = NULL;
    if (rufo_feito) UnloadSound(rufo);
    if (moeda_feito) UnloadSound(moeda_rola);
    if (giro_feito) UnloadSound(giro);
    if (motor_feito) UnloadSound(motor);
    rufo_feito = moeda_feito = giro_feito = motor_feito = false;
    for (int q = 1; q < SOM_ROLAS; q++) {
        if (rola_feito[q] && rola[q].frameCount) UnloadSound(rola[q]);
        rola_feito[q] = false;
    }
    if (cam_n) UnloadAudioStream(banda);      // antes das camadas: o callback para de ler
    for (int c = 0; c < CAMADAS; c++) UnloadWave(cam[c]);
    if (voz_dec) stb_vorbis_close(voz_dec);
    MemFree(voz_buf);
    voz_dec = NULL; voz_buf = NULL; voz_ok = false; voz_qual = -1;
    pronto = false;
}

void som_atualiza(float dt)
{
    if (!pronto) return;
    carrega_aos_poucos();
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
    // A banda do chefe entra em uns 2 s e sai em 1,5.
    carrega_voz(chefe_qual);
    float alvo_v = chefe_toca ? 1 : 0;
    v_chefe += (alvo_v - v_chefe) * fminf(1, dt / (chefe_toca ? 0.7f : 0.5f));
    float alvo_voz = voz_ok && voz_qual == chefe_qual ? alvo_v : 0;
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
    if (on) comeca_a_rolar(laco(0));
    else rufando = false;
}

void som_rufo_moeda(void)
{
    if (pronto) comeca_a_rolar(laco(1));
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
        laco(2);
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
        laco(3);
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
