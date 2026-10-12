// ===========================================================================
// Estilo: o feltro, a mesa e o material dos dados comuns
// ===========================================================================
// O perfil escolhe o feltro, a mesa (madeira e moldura) e o material dos seus
// dados comuns; o que ainda nao abriu fica no padrao. Os oponentes tem o
// material do nivel deles. Os dados especiais tem um desenho so, o mesmo
// para os dois lados.
enum { FT_VERDE, FT_VINHO, FT_AZUL, FT_GRAFITE, FT_N };
static const struct { const char *nome; uint16_t tons[7]; } FELTROS[FT_N] = {
    // do mais escuro (a sombra) ao realce, e o texto discreto
    [FT_VERDE]   = { "Verde", { RGB(8, 24, 17), RGB(11, 33, 24), RGB(15, 41, 30), RGB(18, 49, 36),
                                RGB(24, 62, 46), RGB(40, 84, 62), RGB(170, 190, 178) } },
    [FT_VINHO]   = { "Vinho", { RGB(26, 8, 12), RGB(38, 12, 18), RGB(48, 16, 24), RGB(58, 20, 30),
                                RGB(74, 28, 38), RGB(104, 46, 58), RGB(210, 176, 180) } },
    [FT_AZUL]    = { "Azul-noite", { RGB(8, 14, 28), RGB(11, 20, 40), RGB(15, 27, 50), RGB(19, 34, 60),
                                     RGB(26, 46, 78), RGB(44, 70, 108), RGB(170, 184, 206) } },
    [FT_GRAFITE] = { "Grafite", { RGB(14, 14, 16), RGB(20, 20, 23), RGB(26, 26, 30), RGB(32, 32, 36),
                                  RGB(42, 42, 48), RGB(64, 64, 72), RGB(182, 182, 188) } },
};

enum { MAT_MARFIM, MAT_OSSO, MAT_JADE, MAT_OBSIDIANA, MAT_EBANO, MAT_N };
typedef struct {
    const char *nome;
    uint16_t tons[5];            // o corpo: sombra, meia-sombra, corpo, luz, reflexo
    uint16_t brilho;             // a aresta de cima (0: o corpo clareado brilho_pct)
    uint8_t  brilho_pct;
    uint16_t tinta;              // os pontos e o numero
    uint16_t apagado, tinta_apagada;
} material_t;
static const material_t MATERIAL[MAT_N] = {
    [MAT_MARFIM]    = { "Marfim", { RGB(218, 208, 186), RGB(229, 220, 199), RGB(238, 230, 212), RGB(245, 239, 223),
                                    RGB(252, 248, 236) }, 0, 35,
                        RGB(36, 32, 29), RGB(204, 194, 172), RGB(62, 56, 50) },
    [MAT_OSSO]      = { "Osso", { RGB(208, 194, 156), RGB(220, 206, 170), RGB(230, 218, 184), RGB(240, 230, 198),
                                  RGB(250, 242, 214) }, RGB(250, 242, 214), 0,
                        RGB(64, 42, 28), RGB(186, 170, 132), RGB(90, 64, 44) },
    [MAT_JADE]      = { "Jade", { RGB(80, 148, 112), RGB(96, 166, 128), RGB(120, 186, 148), RGB(150, 206, 172),
                                  RGB(186, 226, 200) }, RGB(186, 226, 200), 0,
                        RGB(18, 48, 36), RGB(62, 122, 92), RGB(30, 70, 52) },
    [MAT_OBSIDIANA] = { "Obsidiana", { RGB(38, 33, 52), RGB(48, 42, 64), RGB(62, 54, 80), RGB(84, 74, 106),
                                       RGB(110, 98, 134) }, RGB(110, 98, 134), 0,
                        RGB(228, 194, 106), RGB(28, 24, 40), RGB(150, 122, 62) },
    [MAT_EBANO]     = { "Ébano", { RGB(24, 22, 20), RGB(30, 27, 24), RGB(36, 32, 29), RGB(46, 42, 40),
                                   RGB(53, 48, 43) }, 0, 18,
                        RGB(238, 230, 212), RGB(62, 56, 50), RGB(204, 194, 172) },
};

// A mesa: a madeira (veio, corpo, filete de luz) e o metal da moldura. A de
// nogueira e ouro e a de sempre: o metal acompanha os andares do Desafiante.
enum { MS_NOGUEIRA, MS_MOGNO, MS_EBANO, MS_CARVALHO, MS_N };
static const struct { const char *nome; uint16_t madeira[3], metal; } MESAS[MS_N] = {
    [MS_NOGUEIRA] = { "Nogueira e ouro", { 0, 0, 0 }, 0 },
    [MS_MOGNO]    = { "Mogno e cobre", { RGB(44, 16, 14), RGB(66, 24, 20), RGB(104, 40, 32) }, RGB(184, 110, 70) },
    [MS_EBANO]    = { "Ébano e prata", { RGB(18, 16, 16), RGB(30, 27, 26), RGB(84, 76, 70) }, RGB(184, 194, 208) },
    [MS_CARVALHO] = { "Carvalho e bronze", { RGB(128, 90, 54), RGB(150, 108, 66), RGB(204, 164, 110) }, RGB(176, 124, 72) },
};

// O que cada andar abre: vencer o Barao naquela dificuldade (-1: sempre aberto).
static const int8_t ABRE_FELTRO[FT_N]   = { -1, 0, 2, 4 };
static const int8_t ABRE_MATERIAL[MAT_N] = { -1, 0, 2, 5, 1 };
static const int8_t ABRE_MESA[MS_N]      = { -1, 1, 4, 3 };

static int estilo_feltro, estilo_mesa;      // o que esta na mesa agora
static int des_estilo_abriu = -1;           // fim da run: o andar cujo estilo acabou de abrir
static uint8_t material_j[2];               // o material dos dados comuns de cada lado
static int estilo_versao;                   // muda a cada troca: a mesa pronta se refaz

// Monta a paleta com os sete tons do feltro escolhido e marca como exatas as
// cores que so os dados e a mesa usam, inteiras (nada mais cai nelas).
static void aplica_estilo(int feltro, int mesa_id)
{
    if (feltro < 0 || feltro >= FT_N) feltro = FT_VERDE;
    if (mesa_id < 0 || mesa_id >= MS_N) mesa_id = MS_NOGUEIRA;
    estilo_feltro = feltro;
    estilo_mesa = mesa_id;
    memcpy(feltro_tons, FELTROS[feltro].tons, sizeof feltro_tons);
    uint16_t viva[sizeof PALETA / sizeof PALETA[0]];
    memcpy(viva, PALETA, sizeof viva);
    memcpy(viva + PAL_FELTRO, feltro_tons, sizeof feltro_tons);
    gfx_paleta(viva, (int)(sizeof viva / sizeof viva[0]));
    gfx_paleta_exata(LUZ_CORPO, (int)(sizeof LUZ_CORPO / sizeof LUZ_CORPO[0]));
    for (int f = 0; f < FT_N; f++) gfx_paleta_exata(FELTROS[f].tons, 7);   // as amostras da tela de estilo
    for (int m = 0; m < MAT_N; m++) {
        if (m == MAT_MARFIM || m == MAT_EBANO) continue;          // ja estao na paleta
        const material_t *t = &MATERIAL[m];
        gfx_paleta_exata(t->tons, 5);
        uint16_t outras[3] = { t->tinta, t->apagado, t->tinta_apagada };
        gfx_paleta_exata(outras, 3);
    }
    for (int s = 1; s < MS_N; s++) {
        gfx_paleta_exata(MESAS[s].madeira, 3);
        gfx_paleta_exata(&MESAS[s].metal, 1);
    }
    estilo_versao++;
}

// Os dados comuns de cada lado na partida que comeca: os seus no material
// escolhido; os do oponente, o que o nivel dele pede (op).
static int estilo_material_seu(void);
static void materiais_da_partida(int op)
{
    material_j[baixo] = (uint8_t)estilo_material_seu();
    material_j[outro(baixo)] = (uint8_t)op;
}

// O oponente da partida rapida, pelo nivel: facil marfim, medio osso,
// dificil jade. Com duas pessoas ou online, o outro lado fica no marfim.
static int material_rapida(void)
{
    if (modo != M_UM) return MAT_MARFIM;
    return nivel_jog[1] >= 2 ? MAT_JADE : nivel_jog[1] == 1 ? MAT_OSSO : MAT_MARFIM;
}

// O que o andar d abre, numa linha ("feltro Vinho  ·  dados de osso").
static int estilo_do_andar(int d, char *s, int n)
{
    int k = 0, p = 0;
    s[0] = 0;
    for (int i = 0; i < FT_N; i++)
        if (ABRE_FELTRO[i] == d) p += snprintf(s + p, (size_t)(n - p), "%sfeltro %s", k++ ? "  ·  " : "", FELTROS[i].nome);
    for (int i = 0; i < MAT_N && p < n; i++)
        if (ABRE_MATERIAL[i] == d)
            p += snprintf(s + p, (size_t)(n - p), "%sdados de %s", k++ ? "  ·  " : "", MATERIAL[i].nome);
    for (int i = 0; i < MS_N && p < n; i++)
        if (ABRE_MESA[i] == d) p += snprintf(s + p, (size_t)(n - p), "%smesa %s", k++ ? "  ·  " : "", MESAS[i].nome);
    return k;
}
