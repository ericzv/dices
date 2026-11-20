// ===========================================================================
// Os acabamentos: do que cada dado e feito. O acabamento muda o reflexo do
// corpo nos brilhantes (pinta_corpo), o lampejo enquanto ele gira e o toque no
// feltro quando deita (mesa.inc), o jeito de rolar (fisica) e os sons: o dado
// rolando, a batida na borda, em outro dado e o pouso (tools/sons/gera_acab.py).
// Os dados comuns vao no acabamento do material de quem joga com eles; cada
// especial tem o seu (ACAB_TIPO) e, quando o tema pede, uma assinatura: um
// jeito proprio de rolar (o Bumerangue faz a curva, a Ancora cai pesada). As
// moedas (d2) giram e soam como sempre.
// ===========================================================================
enum { AC_MARFIM, AC_LACA, AC_VIDRO, AC_GELO, AC_PEDRA, AC_METAL, AC_MADEIRA, AC_MACIO,
       AC_SECO, AC_LOUCA, AC_BORRACHA, AC_NOITE, AC_PAPEL, AC_GRAO, AC_N };
enum { LP_NADA, LP_BRILHO, LP_ESTRELA };                       // o lampejo enquanto gira
enum { PO_POEIRA, PO_POEIRA_FRACA, PO_CINTILA, PO_GELO, PO_AFUNDA, PO_FIAPO, PO_MIGALHA,
       PO_BRILHO, PO_NADA, PO_ESTRELAS, PO_FAISCA, PO_TREMOR, PO_PAPEL, PO_GRAO, PO_TRILHO };   // o toque no feltro quando deita
enum { AS_NADA, AS_PESADO, AS_DESLIZA, AS_FLUTUA, AS_PIAO, AS_FURIA, AS_RAJADA, AS_VOLTA,
       AS_ONDA, AS_TREMOR, AS_TOMBA, AS_N };                    // as assinaturas

typedef struct {
    const char *nome;
    // A luz: o lampejo e se o reflexo tambem sobe o tom do desenho (os brilhantes).
    uint8_t lampejo;
    uint8_t por_cima;
    // O rolar (fisica): quanto devolve na borda (o marfim: 0,62), quanto perde
    // por segundo (2,4), o giro e a forca do lancamento (1: os do marfim), a
    // velocidade em que para (36 px/s) e a altura do pulinho no ar (1: o do marfim).
    float quica, atrito, giro, forca, para, pulo;
    uint8_t pouso;            // PO_*: o que entra por cima do anel de poeira
    float anel;               // a forca do anel de poeira de sempre (1: o do marfim)
    uint8_t dureza;           // dois dados batendo: soa o mais duro
    int16_t som_pousa, som_bate, som_bate_dado;
    uint8_t rola;             // o laco do dado rolando (SOM_ROLA_*)
} acabamento_t;

static const acabamento_t ACAB[AC_N] = {
    // O reflexo proprio so nos brilhantes (laca, vidro, metal, borracha e a
    // noite), por cima do reflexo de sempre; todo pouso guarda o anel de
    // poeira de sempre (70% a 100%) e o toque da classe entra por cima; o gelo
    // e a noite rolam menos. O seco e o biscoito; o papel e os graos tem os seus.
    //               nome        lampejo    cima quica  atrito giro   forca  para pulo   pouso        anel   dur  sons
    [AC_MARFIM]   = { "Marfim",   LP_NADA,    0, 0.62f, 2.4f, 1.0f,  1.0f,  36, 1.0f, PO_POEIRA,   1.00f, 3, SOM_POUSA, SOM_BATE, SOM_BATE_DADO, SOM_ROLA_FELTRO },
    [AC_LACA]     = { "Laca",     LP_BRILHO,  1, 0.68f, 2.3f, 1.15f, 1.0f,  36, 1.1f, PO_POEIRA,   0.85f, 4, SOM_POUSA_LACA, SOM_BATE_LACA, SOM_BATE_DADO_LACA, SOM_ROLA_FELTRO },
    [AC_VIDRO]    = { "Vidro",    LP_ESTRELA, 1, 0.55f, 2.0f, 0.75f, 0.95f, 34, 0.8f, PO_CINTILA,  0.75f, 6, SOM_POUSA_VIDRO, SOM_BATE_VIDRO, SOM_BATE_DADO_VIDRO, SOM_ROLA_VIDRO },
    [AC_GELO]     = { "Gelo",     LP_BRILHO,  0, 0.50f, 1.9f, 0.35f, 0.85f, 40, 0.3f, PO_GELO,     0.75f, 5, SOM_POUSA_GELO, SOM_BATE_GELO, SOM_BATE_DADO_GELO, SOM_ROLA_GELO },
    [AC_PEDRA]    = { "Pedra",    LP_NADA,    0, 0.40f, 2.9f, 0.7f,  0.92f, 42, 0.6f, PO_AFUNDA,   1.00f, 7, SOM_POUSA_PEDRA, SOM_BATE_PEDRA, SOM_BATE_DADO_PEDRA, SOM_ROLA_PEDRA },
    [AC_METAL]    = { "Metal",    LP_ESTRELA, 1, 0.48f, 2.7f, 0.85f, 0.95f, 40, 0.7f, PO_POEIRA,   0.80f, 8, SOM_POUSA_METAL, SOM_BATE_METAL, SOM_BATE_DADO_METAL, SOM_ROLA_METAL },
    [AC_MADEIRA]  = { "Madeira",  LP_NADA,    0, 0.70f, 2.2f, 1.2f,  1.0f,  34, 1.2f, PO_POEIRA,   1.00f, 2, SOM_POUSA_MADEIRA, SOM_BATE_MADEIRA, SOM_BATE_DADO_MADEIRA, SOM_ROLA_MADEIRA },
    [AC_MACIO]    = { "Macio",    LP_NADA,    0, 0.32f, 2.8f, 0.6f,  0.95f, 38, 0.5f, PO_FIAPO,    0.70f, 0, SOM_POUSA_MACIO, SOM_BATE_MACIO, SOM_BATE_DADO_MACIO, SOM_ROLA_MACIO },
    [AC_SECO]     = { "Biscoito", LP_NADA,    0, 0.45f, 2.9f, 0.8f,  0.90f, 34, 0.9f, PO_MIGALHA,  0.70f, 1, SOM_POUSA_SECO, SOM_BATE_SECO, SOM_BATE_DADO_SECO, SOM_ROLA_SECO },
    [AC_LOUCA]    = { "Louça",    LP_BRILHO,  0, 0.58f, 2.5f, 0.95f, 1.0f,  36, 1.0f, PO_BRILHO,   0.85f, 5, SOM_POUSA_LOUCA, SOM_BATE_LOUCA, SOM_BATE_DADO_LOUCA, SOM_ROLA_LOUCA },
    [AC_BORRACHA] = { "Borracha", LP_BRILHO,  1, 0.85f, 2.5f, 1.1f,  1.0f,  38, 1.6f, PO_POEIRA,   0.70f, 1, SOM_POUSA_BORRACHA, SOM_BATE_BORRACHA, SOM_BATE_DADO_BORRACHA, SOM_ROLA_BORRACHA },
    [AC_NOITE]    = { "Noite",    LP_ESTRELA, 0, 0.60f, 2.5f, 0.7f,  0.88f, 30, 1.2f, PO_ESTRELAS, 0.70f, 3, SOM_POUSA_NOITE, SOM_BATE_NOITE, SOM_BATE_DADO_NOITE, SOM_ROLA_FELTRO },
    [AC_PAPEL]    = { "Papel",    LP_NADA,    0, 0.50f, 2.6f, 1.0f,  0.95f, 36, 1.0f, PO_PAPEL,    0.70f, 1, SOM_POUSA_PAPEL, SOM_BATE_PAPEL, SOM_BATE_DADO_PAPEL, SOM_ROLA_PAPEL },
    [AC_GRAO]     = { "Grãos",    LP_NADA,    0, 0.45f, 2.8f, 0.9f,  0.92f, 36, 0.9f, PO_GRAO,     0.75f, 2, SOM_POUSA_GRAO, SOM_BATE_GRAO, SOM_BATE_DADO_GRAO, SOM_ROLA_GRAO },
};
// O reflexo proprio: so os brilhantes; os outros ficam com o de sempre.
static bool acab_tem_luz(int ac)
{
    return ac == AC_LACA || ac == AC_VIDRO || ac == AC_METAL || ac == AC_BORRACHA || ac == AC_NOITE;
}

// Os comuns: o acabamento do material.
static const uint8_t ACAB_MAT[MAT_N] = {
    [MAT_MARFIM] = AC_MARFIM, [MAT_OSSO] = AC_MARFIM, [MAT_JADE] = AC_PEDRA, [MAT_OBSIDIANA] = AC_VIDRO,
    [MAT_EBANO] = AC_MADEIRA, [MAT_MADEIRA] = AC_MADEIRA, [MAT_PEROLA] = AC_LOUCA, [MAT_MARMORE] = AC_PEDRA,
    [MAT_LAPIS] = AC_PEDRA, [MAT_PEROLA_NEGRA] = AC_LOUCA, [MAT_BRONZE] = AC_METAL, [MAT_BOLA] = AC_VIDRO,
    [MAT_BISCOITO] = AC_SECO, [MAT_GELO] = AC_GELO, [MAT_VELUDO] = AC_MACIO,
};

// Os especiais (o que nao esta aqui e AC_MARFIM) e as assinaturas.
static const uint8_t ACAB_TIPO[D_N] = {
    [D_DOBRO] = AC_METAL, [D_VICIADO] = AC_LACA, [D_LASTRO] = AC_METAL,
    [D_ESCUDO] = AC_METAL, [D_PIRATA] = AC_MADEIRA, [D_ESPELHO] = AC_VIDRO, [D_FICHAS] = AC_LOUCA,
    [D_MALDITO] = AC_NOITE, [D_GATUNO] = AC_MACIO, [D_QUEBRADO] = AC_LOUCA,
    [D_INVEJOSO] = AC_PEDRA, [D_FARTURA] = AC_GRAO,
    [D_JACKPOT] = AC_METAL, [D_EGOISTA] = AC_MACIO, [D_LASER] = AC_METAL,
    [D_SOLITARIO] = AC_NOITE, [D_SENTINELA] = AC_PEDRA, [D_FERREIRO] = AC_METAL, [D_COBRADOR] = AC_PAPEL,
    [D_VIDRO] = AC_VIDRO, [D_ACUMULADOR] = AC_METAL, [D_BUMERANGUE] = AC_MADEIRA, [D_PRISMA] = AC_VIDRO,
    [D_TRONO] = AC_MACIO, [D_CAVALO] = AC_MADEIRA, [D_BERSERKER] = AC_MADEIRA, [D_VIUVA] = AC_NOITE,
    [D_DOMINO] = AC_LOUCA, [D_COVEIRO] = AC_PEDRA, [D_HEXA] = AC_LACA, [D_OURICO] = AC_MACIO,
    [D_CALIBRE] = AC_METAL, [D_AMPULHETA] = AC_MADEIRA, [D_MEDUSA] = AC_PEDRA, [D_MISTERIOSO] = AC_NOITE,
    [D_ESPERANCA] = AC_NOITE, [D_GARIMPEIRO] = AC_PEDRA, [D_PREVISIVEL] = AC_PAPEL, [D_OCULTO] = AC_NOITE,
    [D_SACRIFICIO] = AC_MACIO, [D_VAZIO] = AC_NOITE, [D_VAMPIRO] = AC_MACIO, [D_PACTO] = AC_PAPEL,
    [D_SANGUESSUGA] = AC_BORRACHA, [D_TRENO] = AC_LACA, [D_BONECO] = AC_GELO,
    [D_JINGLE] = AC_METAL, [D_TSUNAMI] = AC_LACA, [D_PRESENTE] = AC_PAPEL, [D_PANETONE] = AC_SECO,
    [D_MEIA] = AC_MACIO, [D_ESTRELA] = AC_NOITE, [D_RENA] = AC_MACIO, [D_REVOLVER] = AC_METAL,
    [D_ASTRO] = AC_NOITE, [D_CONSTELACAO] = AC_NOITE, [D_GALAXIA] = AC_NOITE, [D_BURACO] = AC_NOITE,
    [D_UNIVERSO] = AC_NOITE, [D_TERREMOTO] = AC_PEDRA, [D_DIGITAL] = AC_LACA,
    [D_MAIS_ADESIVADO] = AC_LACA, [D_AGNUS] = AC_MACIO, [D_TELA] = AC_PAPEL, [D_MOSAICO] = AC_LOUCA,
    [D_PIAO] = AC_MADEIRA, [D_CAMELO] = AC_MACIO, [D_LEQUE] = AC_PAPEL, [D_ESTILINGUE] = AC_MACIO,
    [D_CAMALEAO] = AC_MACIO, [D_BALAO] = AC_BORRACHA, [D_PAPAGAIO] = AC_MACIO, [D_MATRIOSKA] = AC_LACA,
    [D_COLCHA] = AC_MACIO, [D_GELATINA] = AC_BORRACHA, [D_CARROSSEL] = AC_LACA,
};
static const uint8_t ASSINA_TIPO[D_N] = {
    [D_VICIADO] = AS_TOMBA, [D_LASTRO] = AS_PESADO, [D_VENTANIA] = AS_RAJADA, [D_BUMERANGUE] = AS_VOLTA,
    [D_BERSERKER] = AS_FURIA, [D_ANCORA] = AS_PESADO, [D_FANTASMA] = AS_FLUTUA, [D_VAZIO] = AS_FLUTUA,
    [D_ASSOMBRACAO] = AS_FLUTUA, [D_TRENO] = AS_DESLIZA, [D_FURIA] = AS_FURIA, [D_ONDA] = AS_ONDA,
    [D_TSUNAMI] = AS_ONDA, [D_TERREMOTO] = AS_TREMOR, [D_PIAO] = AS_PIAO,
    [D_ESTILINGUE] = AS_FURIA, [D_BALAO] = AS_FLUTUA, [D_CARROSSEL] = AS_PIAO,
};

static bool acab_comum(int tipo)
{
    return TIPO[tipo].efeito == EF_NADA && TIPO[tipo].lados != 2 && tipo != D_MISTERIOSO;
}
// O acabamento de um dado de dono (o material dos comuns e o de quem joga com eles).
static int acab_de(int tipo, int dono)
{
    if (tipo < 0 || tipo >= D_N || TIPO[tipo].lados == 2) return AC_MARFIM;   // as moedas e as fichas: como sempre
    return acab_comum(tipo) ? ACAB_MAT[material_j[dono & 1] % MAT_N] : ACAB_TIPO[tipo];
}
static int assina_de(int tipo)
{
    return tipo >= 0 && tipo < D_N && TIPO[tipo].lados != 2 ? ASSINA_TIPO[tipo] : AS_NADA;
}
// O laco do dado rolando (o som_rufo_de): a Furia rola pesada, a Onda com a
// agua e o Piao zunindo; os outros, com o do acabamento.
static int acab_rola(int tipo, int dono)
{
    switch (assina_de(tipo)) {
    case AS_FURIA: return SOM_ROLA_FURIA;
    case AS_ONDA:  return SOM_ROLA_ONDA;
    case AS_PIAO:  return SOM_ROLA_PIAO;
    default:       return ACAB[acab_de(tipo, dono)].rola;
    }
}
