// ===========================================================================
// Cores da mesa e dos dados
// ===========================================================================
#define C_FELTRO     RGB(18, 49, 36)
#define C_FELTRO2    RGB(24, 62, 46)
#define C_FELTRO_ESC RGB(11, 33, 24)
#define C_FELTRO_MEIO RGB(15, 41, 30)
#define C_LATAO      RGB(190, 156, 86)
#define C_LATAO_ESC  RGB(106, 86, 44)
#define C_BARRA      RGB(14, 14, 12)
#define C_MARFIM     RGB(238, 230, 212)
#define C_MARFIM_S   RGB(204, 194, 172)
#define C_EBANO      RGB(36, 32, 29)
#define C_EBANO_B    RGB(62, 56, 50)
#define C_ROSA       RGB(226, 164, 168)
#define C_VINHO      RGB(150, 62, 80)
#define C_VINHO_CLR  RGB(208, 98, 108)
#define C_VIOLETA    RGB(102, 74, 138)
#define C_ACO        RGB(140, 150, 164)
#define C_OURO       RGB(228, 194, 106)
#define C_TERRA      RGB(156, 106, 64)
#define C_SOMBRA     RGB(8, 24, 17)
#define C_VERDE      RGB(88, 152, 84)        // a Semente brotando
#define C_FOGO       RGB(214, 112, 44)       // a Ficha de Fogo queimando
#define C_BRASA      RGB(190, 56, 60)
#define C_AMARELO    RGB(246, 222, 132)
#define C_CINZA      RGB(128, 120, 112)      // cinza do que queimou
#define C_PRATA      RGB(184, 194, 208)
#define C_MARFIM_D   RGB(238, 230, 212)      // corpo dos dados de quem joga embaixo
#define C_AZUL       RGB(56, 86, 150)
#define C_VERMELHO   RGB(190, 56, 60)
#define C_TEXTO_M    RGB(170, 190, 178)
#define C_BRANCO     RGB(252, 248, 236)
#define C_CHUMBO     RGB(70, 76, 88)
#define C_REAL       RGB(150, 112, 160)       // purpura real
#define C_EGO        RGB(150, 112, 160)      // o Egoista anulando os outros
#define C_LASER      RGB(130, 204, 112)       // raio do Laser
#define C_VENTO      RGB(168, 204, 230)      // Ventania
#define C_HALO       RGB(184, 194, 208)      // a luz do Misericordioso
#define C_BRONZE     RGB(156, 106, 64)
#define C_FERRO      RGB(104, 112, 124)       // o martelo do Ferreiro
#define C_CARMIM     RGB(190, 56, 60)        // a flamula do Desafiante
#define C_VIDRO      RGB(168, 204, 230)      // o Vidro se partindo
#define C_MADEIRA    RGB(78, 46, 30)
#define C_MADEIRA_ESC RGB(56, 32, 20)
#define C_MADEIRA_CLR RGB(120, 78, 50)

// ===========================================================================
// A paleta: 52 cores, em familias de tons vizinhos. O quadro inteiro sai so
// nelas (gfx_quantiza no fim de dado_desenha): misturas, sombras e
// transparencias caem sempre numa cor daqui. Cor nova so entra se couber numa
// familia.
// ===========================================================================
static const uint16_t PALETA[] = {
    // escuros quentes: barras, ebano, pardo
    RGB(14, 14, 12), RGB(24, 22, 20), RGB(36, 32, 29), RGB(62, 56, 50), RGB(96, 88, 80), RGB(128, 120, 112),
    // cinzas frios: chumbo, aco, prata
    RGB(70, 76, 88), RGB(104, 112, 124), RGB(140, 150, 164), RGB(184, 194, 208),
    // marfim
    RGB(252, 248, 236), RGB(238, 230, 212), RGB(204, 194, 172), RGB(166, 154, 132),
    // feltro, do mais escuro ao realce, e o texto discreto (PAL_FELTRO)
    RGB(8, 24, 17), RGB(11, 33, 24), RGB(15, 41, 30), RGB(18, 49, 36), RGB(24, 62, 46), RGB(40, 84, 62),
    RGB(170, 190, 178),
    // verdes
    RGB(88, 152, 84), RGB(130, 204, 112),
    // madeira
    RGB(56, 32, 20), RGB(78, 46, 30), RGB(120, 78, 50), RGB(156, 106, 64),
    // latao e ouro
    RGB(106, 86, 44), RGB(150, 122, 62), RGB(190, 156, 86), RGB(228, 194, 106), RGB(246, 222, 132),
    // vinho e vermelho
    RGB(66, 18, 24), RGB(118, 30, 38), RGB(150, 62, 80), RGB(190, 56, 60), RGB(208, 98, 108), RGB(226, 164, 168),
    // fogo
    RGB(214, 112, 44), RGB(238, 162, 74),
    // azuis
    RGB(26, 34, 62), RGB(56, 86, 150), RGB(96, 134, 196), RGB(168, 204, 230),
    // agua
    RGB(72, 150, 138), RGB(96, 182, 198),
    // roxos
    RGB(54, 36, 74), RGB(102, 74, 138), RGB(150, 112, 160), RGB(206, 180, 218),
    // pele (PAL_PELE): so os retratos
    RGB(240, 200, 164), RGB(200, 142, 104),
};
#define PAL_FELTRO 14                    // onde comeca cada familia em PALETA
#define PAL_PELE   50

// A luz e a sombra do corpo dos dados: tons do meio entre os do marfim e
// entre os do ebano. Nao entram na paleta: so o corpo de um dado e pintado
// exatamente neles (tons_do_corpo), e nada mais cai neles.
static const uint16_t LUZ_CORPO[] = {
    RGB(245, 239, 223), RGB(229, 220, 199), RGB(218, 208, 186),
    RGB(30, 27, 24), RGB(46, 42, 40), RGB(53, 48, 43),
};
