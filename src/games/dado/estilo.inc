// ===========================================================================
// Estilo: o feltro, a mesa e o material dos dados comuns
// ===========================================================================
// O perfil escolhe o feltro, a mesa (madeira e moldura) e o material dos seus
// dados comuns; o que ainda nao abriu fica no padrao. Os oponentes tem o
// material do nivel deles. Os dados especiais tem um desenho so, o mesmo
// para os dois lados.
enum { FT_VERDE, FT_VINHO, FT_AZUL, FT_GRAFITE, FT_GALAXIA, FT_NATAL, FT_MAR, FT_DECO, FT_COURO, FT_LOSANGO,
       FT_NOITE_FELIZ, FT_XADREZ, FT_REVEILLON, FT_N };
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
    // Os de enfeite: o ceu de estrelas e o Natal (pinta_mesa desenha o resto).
    [FT_GALAXIA] = { "Galáxias", { RGB(3, 4, 12), RGB(6, 8, 22), RGB(9, 12, 32), RGB(13, 17, 44),
                                   RGB(24, 30, 68), RGB(44, 52, 100), RGB(178, 186, 216) } },
    [FT_NATAL]   = { "Natalino", { RGB(5, 20, 12), RGB(8, 30, 18), RGB(11, 40, 24), RGB(14, 50, 30),
                                   RGB(22, 66, 40), RGB(40, 92, 58), RGB(192, 212, 196) } },
    // O mar a noite (ondas e o reflexo da lua), o art deco (leques de ouro
    // nos cantos), o couro (com costura) e os losangos (a estampa um tom acima).
    [FT_MAR]     = { "Mar à noite", { RGB(3, 7, 16), RGB(5, 11, 26), RGB(8, 16, 36), RGB(11, 21, 46),
                                      RGB(20, 34, 66), RGB(38, 58, 98), RGB(172, 188, 214) } },
    [FT_DECO]    = { "Art déco", { RGB(5, 12, 11), RGB(8, 19, 17), RGB(11, 26, 23), RGB(14, 33, 29),
                                   RGB(22, 47, 41), RGB(40, 72, 62), RGB(180, 198, 190) } },
    [FT_COURO]   = { "Couro", { RGB(20, 11, 7), RGB(31, 18, 11), RGB(42, 25, 16), RGB(54, 33, 21),
                                RGB(70, 45, 29), RGB(98, 66, 44), RGB(214, 194, 172) } },
    [FT_LOSANGO] = { "Losangos", { RGB(6, 19, 18), RGB(9, 28, 26), RGB(12, 37, 34), RGB(15, 46, 42),
                                   RGB(23, 61, 55), RGB(42, 86, 76), RGB(176, 196, 190) } },
    // Os do fim do ano: a noite azul com neve caindo devagar, o xadrez
    // escoces vermelho e verde (o vermelho e o feltro; o verde, as faixas) e
    // o Reveillon, preto com ouro e fogos nos cantos.
    [FT_NOITE_FELIZ] = { "Noite Feliz", { RGB(5, 9, 22), RGB(8, 14, 32), RGB(11, 19, 42), RGB(14, 24, 52),
                                          RGB(24, 38, 74), RGB(46, 64, 106), RGB(178, 190, 214) } },
    [FT_XADREZ]  = { "Xadrez escocês", { RGB(30, 6, 8), RGB(46, 10, 12), RGB(60, 14, 16), RGB(74, 18, 20),
                                         RGB(94, 28, 28), RGB(124, 48, 44), RGB(214, 182, 176) } },
    [FT_REVEILLON] = { "Réveillon", { RGB(6, 6, 6), RGB(10, 10, 10), RGB(14, 14, 13), RGB(18, 18, 17),
                                      RGB(28, 27, 24), RGB(52, 48, 40), RGB(196, 186, 160) } },
};

enum { MAT_MARFIM, MAT_OSSO, MAT_JADE, MAT_OBSIDIANA, MAT_EBANO, MAT_MADEIRA, MAT_PEROLA,
       MAT_MARMORE, MAT_LAPIS, MAT_PEROLA_NEGRA, MAT_BRONZE,
       MAT_BOLA, MAT_BISCOITO, MAT_GELO, MAT_VELUDO, MAT_N };
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
    // Madeira polida, com veio (pinta_corpo), e os pontos marchetados em
    // marfim dourado; perola, com o furta-cor por cima.
    [MAT_MADEIRA]   = { "Madeira", { RGB(72, 36, 20), RGB(98, 52, 28), RGB(126, 70, 38), RGB(158, 94, 52),
                                     RGB(204, 140, 86) }, RGB(214, 158, 102), 0,
                        RGB(242, 218, 160), RGB(88, 70, 58), RGB(146, 128, 110) },
    [MAT_PEROLA]    = { "Pérola", { RGB(192, 184, 198), RGB(216, 210, 218), RGB(234, 230, 230), RGB(246, 243, 240),
                                    RGB(255, 253, 250) }, RGB(255, 253, 250), 0,
                        RGB(70, 58, 82), RGB(176, 172, 178), RGB(116, 110, 122) },
    // Marmore branco com veios cinza; lapis-lazuli azul com pintas de ouro;
    // perola negra, com o furta-cor verde e roxo; bronze fosco, de metal.
    [MAT_MARMORE]   = { "Mármore", { RGB(194, 196, 202), RGB(214, 216, 220), RGB(232, 232, 234), RGB(243, 243, 245),
                                     RGB(252, 252, 252) }, RGB(252, 252, 252), 0,
                        RGB(50, 52, 60), RGB(174, 174, 178), RGB(108, 110, 116) },
    [MAT_LAPIS]     = { "Lápis-lazúli", { RGB(20, 30, 84), RGB(28, 42, 110), RGB(36, 54, 134), RGB(50, 72, 160),
                                          RGB(82, 106, 190) }, RGB(96, 120, 200), 0,
                        RGB(232, 198, 110), RGB(50, 56, 82), RGB(118, 108, 80) },
    [MAT_PEROLA_NEGRA] = { "Pérola negra", { RGB(36, 36, 44), RGB(52, 52, 62), RGB(70, 70, 82), RGB(94, 94, 108),
                                             RGB(130, 130, 146) }, RGB(130, 130, 146), 0,
                        RGB(236, 228, 210), RGB(62, 62, 68), RGB(128, 124, 118) },
    [MAT_BRONZE]    = { "Bronze", { RGB(94, 58, 24), RGB(130, 86, 36), RGB(166, 116, 52), RGB(202, 154, 78),
                                    RGB(238, 206, 140) }, RGB(238, 206, 140), 0,
                        RGB(58, 32, 14), RGB(112, 92, 74), RGB(70, 54, 40) },
    // Os de Natal: a bola vermelha espelhada, com os pontos de ouro; o
    // biscoito de gengibre, com os pontos de glace; o gelo azul, com um floco
    // gravado; e o veludo vinho, com os pontos de ouro - o mais sobrio.
    [MAT_BOLA]      = { "Bola de Natal", { RGB(70, 8, 14), RGB(112, 16, 24), RGB(152, 26, 34), RGB(196, 50, 54),
                                           RGB(250, 196, 184) }, RGB(250, 196, 184), 0,
                        RGB(240, 206, 120), RGB(96, 52, 54), RGB(146, 120, 80) },
    [MAT_BISCOITO]  = { "Biscoito", { RGB(96, 52, 24), RGB(118, 66, 32), RGB(142, 84, 42), RGB(164, 104, 56),
                                      RGB(186, 128, 74) }, RGB(192, 136, 82), 0,
                        RGB(250, 246, 236), RGB(110, 84, 64), RGB(176, 162, 146) },
    [MAT_GELO]      = { "Gelo", { RGB(112, 156, 196), RGB(140, 184, 218), RGB(170, 208, 234), RGB(200, 228, 246),
                                  RGB(236, 248, 254) }, RGB(236, 248, 254), 0,
                        RGB(28, 56, 104), RGB(132, 150, 168), RGB(72, 88, 112) },
    [MAT_VELUDO]    = { "Veludo vinho", { RGB(48, 10, 20), RGB(64, 16, 28), RGB(80, 22, 36), RGB(98, 32, 46),
                                          RGB(120, 46, 60) }, RGB(124, 52, 64), 0,
                        RGB(226, 190, 104), RGB(62, 40, 46), RGB(132, 112, 80) },
};
// O furta-cor da perola: rosado e azulado, em faixas largas no corpo; o da
// perola negra, esverdeado e arroxeado.
static const uint16_t PEROLA_FURTA[2] = { RGB(246, 224, 230), RGB(222, 236, 242) };
static const uint16_t PEROLA_NEGRA_FURTA[2] = { RGB(70, 86, 84), RGB(86, 74, 98) };
// Os veios do marmore (o forte e o fraco) e as pintas de ouro do lapis-lazuli.
static const uint16_t MARMORE_VEIO[2] = { RGB(146, 150, 160), RGB(180, 184, 192) };
static const uint16_t LAPIS_OURO = RGB(222, 188, 100);
// O floco gravado no gelo (o sulco e a luz dele) e as migalhas do biscoito.
static const uint16_t GELO_FLOCO[2] = { RGB(120, 166, 206), RGB(246, 252, 255) };
static const uint16_t BISCOITO_MIGALHA[2] = { RGB(80, 42, 18), RGB(206, 152, 98) };

// A mesa: a madeira (veio, corpo, filete de luz) e o metal da moldura. A de
// nogueira e ouro e a de sempre: o metal acompanha os andares do Desafiante.
enum { MS_NOGUEIRA, MS_MOGNO, MS_EBANO, MS_CARVALHO, MS_LED, MS_LAREIRA, MS_PINHO, MS_N };
static const struct { const char *nome; uint16_t madeira[3], metal; } MESAS[MS_N] = {
    [MS_NOGUEIRA] = { "Nogueira e ouro", { 0, 0, 0 }, 0 },
    [MS_MOGNO]    = { "Mogno e cobre", { RGB(44, 16, 14), RGB(66, 24, 20), RGB(104, 40, 32) }, RGB(184, 110, 70) },
    [MS_EBANO]    = { "Ébano e prata", { RGB(18, 16, 16), RGB(30, 27, 26), RGB(84, 76, 70) }, RGB(184, 194, 208) },
    [MS_CARVALHO] = { "Carvalho e bronze", { RGB(128, 90, 54), RGB(150, 108, 66), RGB(204, 164, 110) }, RGB(176, 124, 72) },
    // Laca preta, e a moldura e uma fita de led vermelha que respira (pinta_mesa).
    [MS_LED]      = { "Led", { RGB(10, 10, 12), RGB(18, 18, 22), RGB(70, 68, 76) }, RGB(204, 42, 46) },
    // A lareira: tijolinho em volta (madeira: o rejunte, o tijolo, a luz) e,
    // no lugar da moldura, a brasa que respira com o led. O pinho claro, com
    // uma fita vermelha no lugar do metal.
    [MS_LAREIRA]  = { "Lareira", { RGB(54, 30, 24), RGB(132, 58, 40), RGB(170, 86, 60) }, RGB(232, 108, 40) },
    [MS_PINHO]    = { "Pinho com fita", { RGB(170, 128, 78), RGB(206, 166, 110), RGB(232, 202, 150) }, RGB(178, 30, 40) },
};

// Os enfeites dos estilos novos (pinta_mesa): cores exatas, so deles.
// O ceu: quatro nevoas da Via Lactea; as estrelas paradas, apagadinhas no
// escuro (a fraca, a media, a quente, a fria e a clara); e as que cintilam,
// mais fortes.
static const uint16_t GAL_NEVOA[4]   = { RGB(20, 22, 56), RGB(34, 28, 74), RGB(20, 36, 76), RGB(52, 44, 102) };
static const uint16_t GAL_ESTRELA[5] = { RGB(36, 42, 78), RGB(66, 74, 116), RGB(118, 108, 94), RGB(86, 100, 148),
                                         RGB(132, 138, 172) };
static const uint16_t GAL_CINTILA[4] = { RGB(78, 86, 128), RGB(146, 156, 196), RGB(255, 234, 176), RGB(252, 248, 236) };
// O Natal: a fita de veludo vinho, as lampadas (acesas e apagadas), a neve e
// o azevinho.
static const uint16_t NATAL_FITA[3]    = { RGB(64, 14, 22), RGB(98, 22, 32), RGB(136, 40, 50) };
static const uint16_t NATAL_ACESA[4]   = { RGB(255, 86, 74), RGB(255, 208, 94), RGB(114, 232, 126), RGB(118, 174, 255) };
static const uint16_t NATAL_APAGADA[4] = { RGB(110, 36, 36), RGB(122, 96, 42), RGB(42, 96, 56), RGB(44, 66, 122) };
static const uint16_t NATAL_BRILHO[4]  = { RGB(150, 52, 48), RGB(160, 128, 56), RGB(58, 132, 74), RGB(60, 92, 160) };
static const uint16_t NATAL_NEVE[2]    = { RGB(238, 244, 252), RGB(170, 186, 206) };
static const uint16_t NATAL_FOLHA[3]   = { RGB(16, 70, 36), RGB(30, 104, 56), RGB(64, 146, 88) };
static const uint16_t NATAL_FIO        = RGB(26, 34, 26);
// O led vermelho: sete forcas da fita, da mais fraca a mais forte (a ultima
// so no pico da respiracao), e o halo de cada uma.
// O mar a noite: a onda fraca e a mais clara, a lua, e o reflexo dela do
// mais perto (o mais claro) ao mais longe.
#define MAR_N 7
static const uint16_t MAR_TOM[MAR_N] = { RGB(24, 42, 76), RGB(36, 56, 94), RGB(214, 218, 196), RGB(132, 142, 146),
                                         RGB(92, 106, 128), RGB(62, 78, 108), RGB(42, 58, 92) };
// A linha da costura do couro e o furo dela.
static const uint16_t COURO_LINHA[2] = { RGB(196, 172, 130), RGB(26, 14, 8) };
#define LED_N 7
static const uint16_t LED_TOM[LED_N]  = { RGB(110, 18, 22), RGB(140, 24, 30), RGB(172, 32, 38), RGB(204, 42, 46),
                                          RGB(232, 58, 60), RGB(252, 92, 88), RGB(255, 124, 114) };
static const uint16_t LED_HALO[LED_N] = { RGB(30, 8, 10), RGB(42, 10, 12), RGB(56, 14, 16), RGB(72, 18, 20),
                                          RGB(88, 22, 24), RGB(108, 28, 30), RGB(120, 32, 34) };
// A brasa da Lareira: do carvao ao amarelo, e o tijolo esquentando perto dela.
#define BRASA_N 7
static const uint16_t BRASA_TOM[BRASA_N] = { RGB(60, 16, 10), RGB(98, 24, 12), RGB(140, 38, 14), RGB(184, 60, 18),
                                            RGB(224, 96, 26), RGB(246, 142, 44), RGB(255, 196, 92) };
static const uint16_t BRASA_TIJOLO[2] = { RGB(150, 70, 46), RGB(184, 96, 62) };
// A fita do Pinho: a sombra, o cetim e o brilho dele.
static const uint16_t FITA_VERM[3] = { RGB(110, 14, 22), RGB(178, 30, 40), RGB(226, 82, 84) };
// O xadrez: o verde das faixas, o cruzamento com o vermelho (do escuro ao
// claro, um por tom do feltro) e o fio claro que corta.
static const uint16_t XADREZ_VERDE[3] = { RGB(10, 30, 18), RGB(14, 42, 26), RGB(18, 54, 32) };
static const uint16_t XADREZ_MISTO[3] = { RGB(22, 20, 14), RGB(30, 28, 18), RGB(38, 36, 22) };
static const uint16_t XADREZ_FIO[2]   = { RGB(92, 72, 40), RGB(120, 96, 52) };
// O Reveillon: o ouro dos fogos, do mais forte ao que se apaga.
static const uint16_t FOGOS_TOM[4] = { RGB(255, 236, 170), RGB(236, 196, 102), RGB(170, 132, 62), RGB(92, 72, 36) };
// A neve da Noite Feliz: a fraca, ao longe, e a do floco que cai.
static const uint16_t NOITE_NEVE[2] = { RGB(64, 80, 112), RGB(206, 216, 236) };

// O estilo de cada andar: abre com quem fecha a run naquela dificuldade (o
// Barao, ou do Nivel 5 o Coringa; -1: sempre aberto). O Natalino nao e de
// andar nenhum: fica aberto em novembro e dezembro, e so (ANDAR_NATAL); os
// outros de Natal tambem. O Reveillon, so do dia 26 de dezembro ao fim do ano.
#define ANDAR_NATAL (-2)
#define ANDAR_REVEILLON (-3)
static const int8_t ABRE_FELTRO[FT_N]   = { -1, 0, 4, 8, 7, ANDAR_NATAL, 10, 9, 5, 1,
                                            ANDAR_NATAL, ANDAR_NATAL, ANDAR_REVEILLON };
static const int8_t ABRE_MATERIAL[MAT_N] = { -1, 0, 4, 10, 2, 5, 1, 6, 7, 8, 3,
                                             ANDAR_NATAL, ANDAR_NATAL, ANDAR_NATAL, ANDAR_NATAL };
static const int8_t ABRE_MESA[MS_N]      = { -1, 2, 8, 6, 9, ANDAR_NATAL, ANDAR_NATAL };

static int estilo_feltro, estilo_mesa;      // o que esta na mesa agora
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
        uint16_t outras[4] = { t->tinta, t->apagado, t->tinta_apagada, t->brilho ? t->brilho : t->tons[4] };
        gfx_paleta_exata(outras, 4);
    }
    gfx_paleta_exata(PEROLA_FURTA, 2);
    gfx_paleta_exata(PEROLA_NEGRA_FURTA, 2);
    gfx_paleta_exata(MARMORE_VEIO, 2);
    gfx_paleta_exata(&LAPIS_OURO, 1);
    gfx_paleta_exata(MAR_TOM, MAR_N);
    gfx_paleta_exata(COURO_LINHA, 2);
    for (int s = 1; s < MS_N; s++) {
        gfx_paleta_exata(MESAS[s].madeira, 3);
        gfx_paleta_exata(&MESAS[s].metal, 1);
    }
    gfx_paleta_exata(GAL_NEVOA, 4);
    gfx_paleta_exata(GAL_ESTRELA, 5);
    gfx_paleta_exata(GAL_CINTILA, 4);
    gfx_paleta_exata(NATAL_FITA, 3);
    gfx_paleta_exata(NATAL_ACESA, 4);
    gfx_paleta_exata(NATAL_APAGADA, 4);
    gfx_paleta_exata(NATAL_BRILHO, 4);
    gfx_paleta_exata(NATAL_NEVE, 2);
    gfx_paleta_exata(NATAL_FOLHA, 3);
    gfx_paleta_exata(&NATAL_FIO, 1);
    gfx_paleta_exata(LED_TOM, LED_N);
    gfx_paleta_exata(LED_HALO, LED_N);
    gfx_paleta_exata(GELO_FLOCO, 2);
    gfx_paleta_exata(BISCOITO_MIGALHA, 2);
    gfx_paleta_exata(BRASA_TOM, BRASA_N);
    gfx_paleta_exata(BRASA_TIJOLO, 2);
    gfx_paleta_exata(FITA_VERM, 3);
    gfx_paleta_exata(XADREZ_VERDE, 3);
    gfx_paleta_exata(XADREZ_MISTO, 3);
    gfx_paleta_exata(XADREZ_FIO, 2);
    gfx_paleta_exata(FOGOS_TOM, 4);
    gfx_paleta_exata(NOITE_NEVE, 2);
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
