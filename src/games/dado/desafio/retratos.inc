// ---------------------------------------------------------------------------
// Retratos: busto em 20x22 pixels, com contorno escuro e duas tonalidades,
// ampliado sem filtro. Movimento lento: respira, pisca e fala.
// ---------------------------------------------------------------------------
#define RT_W 22
#define RETRATO_AVATAR 20                // os avatares vem depois dos oponentes no desenho
#define RETRATO_EVENTO 24                // e os eventos, depois dos 4 avatares
#define RT_H 27
#define RT_TOPO 3                        // folga em cima para chapeus e coques
static uint16_t rt[RT_H][RT_W];          // 0 = vazio (1 px de margem em volta)

static void rp(int x, int y, uint16_t c)
{
    x += 1; y += 1 + RT_TOPO;
    if (x >= 0 && y >= 0 && x < RT_W && y < RT_H) rt[y][x] = c;
}
static void rr(int x, int y, int w, int h, uint16_t c) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) rp(x + i, y + j, c); }

static uint16_t sombra(uint16_t c) { return gfx_mistura(c, C_BARRA, 28); }
static uint16_t luz(uint16_t c)    { return gfx_mistura(c, C_BRANCO, 22); }

// Rosto: oval da linha hy em diante, orelhas, olhos, sobrancelhas, nariz e
// boca. expr: 0 calma, 1 brava, 2 alegre, 3 preocupada.
static void rosto(int hy, uint16_t pele, uint16_t cabelo, int expr, bool pisca, int boca)
{
    static const int8_t X0[12] = { 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 6, 7 };
    for (int i = 0; i < 12; i++) {
        int x0 = X0[i], x1 = 19 - X0[i];
        rr(x0, hy + i, x1 - x0, 1, pele);
        rp(x1 - 1, hy + i, sombra(pele));                       // lado da sombra
    }
    rr(4, hy + 5, 1, 2, sombra(pele));                          // orelhas
    rr(15, hy + 5, 1, 2, sombra(pele));
    int ey = hy + 6;
    uint16_t olho = RGB(30, 26, 30);
    if (pisca) { rr(7, ey + 1, 2, 1, sombra(pele)); rr(11, ey + 1, 2, 1, sombra(pele)); }
    else {
        rp(7, ey, C_BRANCO); rp(8, ey, olho); rp(8, ey + 1, olho); rp(7, ey + 1, luz(pele));
        rp(12, ey, C_BRANCO); rp(11, ey, olho); rp(11, ey + 1, olho); rp(12, ey + 1, luz(pele));
    }
    uint16_t sob = sombra(cabelo);
    switch (expr) {                                             // sobrancelhas
    case 1:  rp(7, ey - 2, sob); rp(8, ey - 1, sob); rp(12, ey - 2, sob); rp(11, ey - 1, sob); break;
    case 3:  rp(7, ey - 1, sob); rp(8, ey - 2, sob); rp(12, ey - 1, sob); rp(11, ey - 2, sob); break;
    default: rr(7, ey - 2, 2, 1, sob); rr(11, ey - 2, 2, 1, sob); break;
    }
    rp(10, ey + 2, sombra(pele));                               // nariz
    uint16_t lab = RGB(120, 52, 52);
    int my = hy + 10;
    if (boca) { rr(9, my - 1, 2, 2, RGB(60, 22, 26)); rp(8, my - 1, lab); rp(11, my - 1, lab); }
    else if (expr == 2) { rr(9, my - 1, 2, 1, lab); rp(8, my - 2, lab); rp(11, my - 2, lab); }
    else if (expr == 1) { rr(8, my - 1, 4, 1, lab); }
    else rr(9, my - 1, 2, 1, lab);
}

// Tronco: ombros, pescoco e a gola.
static void tronco(uint16_t roupa, uint16_t pele, uint16_t camisa, uint16_t gravata, int estilo)
{
    rr(8, 14, 4, 3, sombra(pele));                              // pescoco
    rr(3, 17, 14, 1, roupa);
    rr(1, 18, 18, 4, roupa);
    rr(1, 18, 1, 4, luz(roupa));
    rr(18, 18, 1, 4, sombra(roupa));
    if (estilo == 0) {                                          // terno: camisa em V e gravata
        rr(8, 17, 4, 1, camisa); rr(9, 18, 2, 2, camisa); rp(9, 20, camisa);
        if (gravata) { rp(9, 17, gravata); rp(10, 17, gravata); rp(9, 18, gravata); rp(10, 19, gravata); }
    } else if (estilo == 1) {                                   // gravata borboleta
        rr(8, 17, 4, 2, camisa);
        rr(7, 17, 2, 1, gravata); rr(11, 17, 2, 1, gravata); rr(9, 17, 2, 1, sombra(gravata));
    } else if (estilo == 2) {                                   // decote: colar
        rr(8, 17, 4, 1, pele); rr(9, 18, 2, 1, pele);
    }
}

// Desenha as linhas l0..l0+nl-1 do retrato em (x, y).
static void retrato(int x, int y, int esc, int perso, float t, bool falando, int l0, int nl)
{
    memset(rt, 0, sizeof rt);
    int fundo = 0;                               // os quadros tem paisagem atras
    // Respira devagar (1 px a cada 1,6 s); pisca de tempos em tempos.
    int dy = (int)(t / 1.6f) % 2;
    float ciclo = fmodf(t + perso * 0.77f, 3.9f);
    bool pisca = ciclo > 3.72f;
    int boca = falando ? ((int)(t / 0.22f) % 2) : 0;
    int hy = 3 + dy;
    bool bravo = modo == M_DESAFIO && perso == perso_virado;   // o chefe virou
    const uint16_t PRETO = RGB(34, 30, 32), MARROM = RGB(104, 66, 40),
                   GRISALHO = RGB(178, 176, 172), RUIVO = RGB(196, 84, 46);
    const uint16_t P_CLARA = RGB(234, 196, 160), P_MEDIA = RGB(204, 150, 106), P_ESCURA = RGB(132, 88, 60),
                   P_PALIDA = RGB(236, 210, 188);
    switch (perso) {
    case 0: {                                                   // NOVATO: bone verde, sardas, moletom
        tronco(RGB(86, 140, 92), P_CLARA, 0, 0, 3);
        rr(8, 17, 4, 1, sombra(RGB(86, 140, 92)));
        rosto(hy, P_CLARA, MARROM, 3, pisca, boca);
        rp(6, hy + 8, RGB(196, 128, 96)); rp(13, hy + 8, RGB(196, 128, 96));     // sardas
        rr(5, hy - 1, 10, 3, RGB(70, 132, 80));                 // bone
        rr(6, hy - 2, 8, 1, RGB(70, 132, 80));
        rr(13, hy + 1, 4, 1, sombra(RGB(70, 132, 80)));         // aba
        rr(5, hy + 2, 2, 2, MARROM); rr(14, hy + 2, 1, 2, MARROM);
        break;
    }
    case 1: {                                                   // CAUTELOSA: coque grisalho, colar de perolas
        tronco(RGB(196, 178, 146), P_PALIDA, 0, 0, 2);
        for (int i = 0; i < 4; i++) rp(7 + i * 2, 17 + (i == 1 || i == 2), C_BRANCO);
        rosto(hy, P_PALIDA, GRISALHO, 0, pisca, boca);
        rr(5, hy - 1, 10, 3, GRISALHO); rr(4, hy + 1, 2, 4, GRISALHO); rr(14, hy + 1, 2, 4, GRISALHO);
        rr(8, hy - 4, 4, 3, GRISALHO); rr(9, hy - 5, 2, 1, luz(GRISALHO));       // coque
        rr(6, hy, 8, 1, luz(GRISALHO));
        break;
    }
    case 2: {                                                   // APOSTADOR: chapeu, charuto, terno vermelho
        tronco(RGB(168, 46, 46), P_MEDIA, RGB(236, 220, 190), C_OURO, 0);
        rosto(hy, P_MEDIA, PRETO, 2, pisca, boca);
        rr(4, hy - 1, 12, 2, RGB(92, 64, 44));                   // aba
        rr(6, hy - 4, 8, 3, RGB(110, 78, 52));                  // copa
        rr(6, hy - 2, 8, 1, RGB(176, 52, 52));                  // fita
        if (!boca) {                                            // charuto e a fumaca subindo
            rr(12, hy + 9, 4, 1, RGB(120, 80, 50)); rp(16, hy + 9, C_FOGO);
            int f = (int)(t / 0.9f) % 3;
            rp(16 + (f == 1), hy + 7 - f, GRISALHO); rp(17 - (f == 2), hy + 5 - f, gfx_mistura(GRISALHO, C_FELTRO, 40));
        }
        break;
    }
    case 3: {                                                   // LADRAO: gorro, mascara, listras
        uint16_t lis = RGB(36, 36, 40);
        tronco(RGB(220, 216, 206), P_MEDIA, 0, 0, 3);
        for (int i = 18; i < 22; i += 2) rr(1, i, 18, 1, lis);
        rosto(hy, P_MEDIA, PRETO, 0, pisca, boca);
        rr(5, hy + 5, 10, 3, lis);                              // mascara
        if (!pisca) { rp(7, hy + 6, C_BRANCO); rp(8, hy + 6, PRETO); rp(11, hy + 6, PRETO); rp(12, hy + 6, C_BRANCO); }
        rr(5, hy - 2, 10, 4, RGB(54, 58, 72));                  // gorro
        rr(5, hy + 1, 10, 1, RGB(74, 80, 98));
        for (int i = 0; i < 5; i++) rp(6 + i, hy + 9 + (i & 1), sombra(P_MEDIA));  // barba por fazer
        break;
    }
    case 4: {                                                   // MATEMATICA: cabelo longo, oculos redondos
        tronco(RGB(52, 64, 112), P_ESCURA, RGB(230, 226, 214), 0, 0);
        rr(3, hy + 2, 3, 13, PRETO); rr(14, hy + 2, 3, 13, PRETO);                  // cabelo atras
        rosto(hy, P_ESCURA, PRETO, 0, pisca, boca);
        rr(5, hy - 1, 10, 3, PRETO); rr(5, hy + 2, 1, 3, PRETO); rr(14, hy + 2, 1, 3, PRETO);
        rr(9, hy - 1, 1, 2, luz(PRETO));
        uint16_t aro = RGB(196, 160, 90);                       // oculos redondos: so a armacao
        rr(7, hy + 5, 2, 1, aro); rr(7, hy + 8, 2, 1, aro); rr(6, hy + 6, 1, 2, aro); rr(9, hy + 6, 1, 2, aro);
        rr(11, hy + 5, 2, 1, aro); rr(11, hy + 8, 2, 1, aro); rr(10, hy + 6, 1, 2, aro); rr(13, hy + 6, 1, 2, aro);
        if ((int)(t / 2.7f) % 3 == 0) rp(12, hy + 6, C_BRANCO); // brilho na lente
        break;
    }
    case 5: {                                                   // ESPINHENTO: cabelo espetado, jaqueta com rebites
        uint16_t jaq = RGB(50, 46, 48);
        tronco(jaq, P_CLARA, RGB(150, 40, 50), 0, 0);
        for (int i = 0; i < 4; i++) { rp(3 + i, 18 + (i & 1), GRISALHO); rp(16 - i, 18 + (i & 1), GRISALHO); }
        rosto(hy, P_CLARA, RUIVO, 1, pisca, boca);
        rr(5, hy - 1, 10, 2, RUIVO);
        for (int i = 0; i < 5; i++) rr(5 + i * 2, hy - 3 + (i & 1), 1, 2, RUIVO);  // espetos
        rp(13, hy + 7, RGB(200, 120, 120)); rp(13, hy + 8, RGB(200, 120, 120));    // cicatriz
        break;
    }
    case 6: {                                                   // SUPERSTICIOSA: lenco de bolinhas, argolas, trevo
        uint16_t lenco = RGB(132, 72, 150);
        tronco(RGB(64, 112, 98), P_MEDIA, 0, 0, 2);
        rp(13, 18, RGB(96, 196, 112)); rp(14, 19, RGB(96, 196, 112)); rp(13, 19, RGB(60, 150, 80));  // trevo
        rosto(hy, P_MEDIA, PRETO, 2, pisca, boca);
        rr(4, hy - 1, 12, 4, lenco); rr(3, hy + 2, 2, 3, lenco); rr(15, hy + 2, 2, 3, lenco);
        for (int i = 0; i < 4; i++) rp(6 + i * 3, hy + (i & 1), C_AMARELO);         // bolinhas
        rp(4, hy + 8, C_OURO); rp(4, hy + 9, C_OURO); rp(15, hy + 8, C_OURO); rp(15, hy + 9, C_OURO);  // argolas
        break;
    }
    case 7: {                                                   // BRUTAMONTES: careca, bigode, regata
        tronco(RGB(206, 200, 190), P_MEDIA, 0, 0, 3);
        rr(1, 17, 4, 5, P_MEDIA); rr(15, 17, 4, 5, P_MEDIA);     // ombros de fora
        rr(1, 17, 1, 5, luz(P_MEDIA));
        rosto(hy, P_MEDIA, MARROM, 1, pisca, boca);
        rr(6, hy, 8, 1, luz(P_MEDIA));                          // brilho da careca
        if (!boca) { rr(7, hy + 9, 6, 1, MARROM); rp(7, hy + 10, MARROM); rp(12, hy + 10, MARROM); }
        break;
    }
    case 8: {                                                   // CRUPIE: viseira verde, colete, gravata borboleta
        tronco(RGB(40, 40, 44), P_CLARA, RGB(236, 232, 222), RGB(176, 40, 52), 1);
        rr(2, 18, 3, 4, RGB(236, 232, 222)); rr(15, 18, 3, 4, RGB(236, 232, 222));  // mangas da camisa
        rosto(hy, P_CLARA, PRETO, bravo ? 1 : 0, pisca, boca);
        rr(5, hy, 10, 2, PRETO); rr(5, hy + 2, 1, 2, PRETO); rr(14, hy + 2, 1, 2, PRETO);
        rr(4, hy + 1, 12, 1, RGB(60, 150, 100));                // viseira
        rr(5, hy + 2, 10, 1, gfx_mistura(RGB(60, 150, 100), C_FELTRO, 30));
        if (bravo) rr(6, hy + 4, 8, 1, sombra(P_CLARA));       // a viseira sombreia o olhar
        if (!boca) rr(8, hy + 9, 4, 1, PRETO);                  // bigode fino
        break;
    }
    case 9: {                                                   // DONA DO SALAO: coque com grampo, vestido, colar
        tronco(RGB(150, 30, 46), P_CLARA, 0, 0, 2);
        rr(7, 17, 6, 1, C_OURO); rp(10, 18, C_OURO);
        rosto(hy, P_CLARA, PRETO, bravo ? 1 : 2, pisca, boca);
        rr(5, hy - 1, 10, 3, PRETO); rr(4, hy + 1, 2, 5, PRETO); rr(14, hy + 1, 2, 5, PRETO);
        rr(7, hy - 4, 6, 3, PRETO); rp(12, hy - 4, C_OURO); rp(13, hy - 5, C_OURO);   // coque e grampo
        if (!boca) rr(9, hy + 9, 2, 1, bravo ? RGB(140, 20, 40) : RGB(200, 40, 60));   // batom
        if (bravo) {                                           // uma sobrancelha erguida; o colar brilha
            rr(11, hy + 3, 2, 1, sombra(PRETO)); rp(13, hy + 4, sombra(PRETO));
            if (fmodf(t, 1.8f) < 0.3f) rp(8 + (int)(t / 1.8f) % 4, 17, C_BRANCO);
        }
        rp(4, hy + 7, C_OURO); rp(15, hy + 7, C_OURO);
        break;
    }
    case 11: {                                                  // ILUSIONISTA: capa de gola alta, cavanhaque, uma carta
        const uint16_t CAPA = RGB(92, 46, 120);
        tronco(RGB(30, 30, 36), P_CLARA, RGB(236, 232, 222), RGB(150, 30, 50), 1);
        rr(1, 15, 3, 3, CAPA); rr(16, 15, 3, 3, CAPA); rr(0, 13, 2, 3, CAPA); rr(18, 13, 2, 3, CAPA);   // gola alta
        rr(1, 18, 2, 4, CAPA); rr(17, 18, 2, 4, CAPA);
        rosto(hy, P_CLARA, PRETO, 2, pisca, boca);
        rr(5, hy - 1, 10, 2, PRETO); rr(9, hy + 1, 2, 1, PRETO);  // cabelo com bico na testa
        rr(5, hy + 1, 1, 3, PRETO); rr(14, hy + 1, 1, 3, PRETO);
        rp(12, hy + 3, sombra(PRETO)); rp(13, hy + 2, sombra(PRETO));   // a sobrancelha arqueada
        if (!boca) { rr(9, hy + 10, 2, 2, PRETO); rp(8, hy + 9, PRETO); rp(11, hy + 9, PRETO); }   // cavanhaque e bigode
        float c = fmodf(t, 4.4f);                               // a carta aparece e some no ombro
        if (c > 2.6f) {
            rr(15, 9, 3, 4, C_BRANCO); rp(16, 10, C_VERMELHO); rp(16, 11, C_VERMELHO);
            if (c > 3.7f) rr(15, 9, 3, 4, gfx_mistura(C_BRANCO, CAPA, 50));
        }
        break;
    }
    case 12: {                                                  // CAUBOI: chapeu de aba virada, lenco, palito
        const uint16_t CHAPEU = RGB(132, 88, 52);
        tronco(RGB(110, 74, 44), P_MEDIA, RGB(196, 180, 150), 0, 0);
        rr(6, 17, 8, 2, RGB(176, 44, 44)); rp(9, 19, RGB(176, 44, 44)); rp(10, 19, RGB(176, 44, 44));   // lenco
        rosto(hy, P_MEDIA, MARROM, 1, pisca, boca);
        if (!pisca) { rr(7, hy + 6, 2, 1, sombra(P_MEDIA)); rr(11, hy + 6, 2, 1, sombra(P_MEDIA)); }   // olhos apertados
        rr(8, hy + 11, 4, 1, sombra(P_MEDIA)); rp(7, hy + 10, sombra(P_MEDIA)); rp(12, hy + 10, sombra(P_MEDIA));   // barba por fazer
        rr(5, hy, 10, 1, MARROM); rr(5, hy + 1, 1, 4, MARROM); rr(14, hy + 1, 1, 4, MARROM);   // cabelo e costeletas
        rr(4, hy - 1, 12, 1, CHAPEU); rp(3, hy - 2, CHAPEU); rp(16, hy - 2, CHAPEU);   // aba virada nas pontas
        rr(6, hy - 4, 8, 3, CHAPEU); rr(8, hy - 5, 4, 1, CHAPEU); rr(9, hy - 4, 2, 1, sombra(CHAPEU));
        rr(6, hy - 2, 8, 1, sombra(sombra(CHAPEU)));
        if (!boca) {                                            // o palito no canto da boca, mexendo
            int p = (int)(t / 1.3f) % 2;
            rr(8, hy + 9, 3, 1, RGB(120, 52, 52));             // boca fechada, de canto
            rp(11, hy + 9, RGB(226, 206, 150)); rp(12, hy + 9 - p, RGB(226, 206, 150)); rp(13, hy + 9 - p, RGB(226, 206, 150));
        }
        break;
    }
    case 13: {                                                  // MONGE: cabeca raspada, manto laranja, contas
        const uint16_t MANTO = RGB(214, 120, 40);
        tronco(MANTO, P_MEDIA, 0, 0, 3);
        rr(1, 18, 6, 4, P_MEDIA); rr(1, 18, 1, 4, luz(P_MEDIA));  // ombro de fora
        for (int i = 0; i < 6; i++) rr(6 + i, 17 + i / 2, 2, 1, sombra(MANTO));   // a dobra do manto na diagonal
        for (int i = 0; i < 5; i++) rp(7 + i * 2, 17 + (i == 0 || i == 4 ? 0 : 1), RGB(110, 60, 30));   // as contas
        bool abre = fmodf(t, 5.5f) > 4.6f;                      // quase sempre de olhos fechados
        rosto(hy, P_MEDIA, P_MEDIA, 2, !abre || pisca, boca);
        rr(6, hy, 8, 1, luz(P_MEDIA)); rr(7, hy - 1, 6, 1, P_MEDIA); rr(8, hy - 1, 2, 1, luz(P_MEDIA));
        break;
    }
    case 14: {                                                  // DETETIVE: chapeu cinza, gola levantada, batom
        const uint16_t CAPOTE = RGB(196, 172, 128), CHAPEU = RGB(96, 96, 104), RUIVA = RGB(150, 70, 40);
        tronco(CAPOTE, P_CLARA, RGB(60, 60, 66), 0, 0);
        rr(3, 15, 3, 3, CAPOTE); rr(14, 15, 3, 3, CAPOTE);     // gola levantada
        rr(4, 16, 1, 2, sombra(CAPOTE)); rr(15, 16, 1, 2, sombra(CAPOTE));
        rr(4, hy + 2, 2, 8, RUIVA); rr(14, hy + 2, 2, 8, RUIVA);   // cabelo chanel
        rosto(hy, P_CLARA, RUIVA, 0, pisca, boca);
        bool desconfia = fmodf(t, 4.8f) > 3.4f;                 // aperta os olhos e ergue a sobrancelha
        if (desconfia && !pisca) { rr(7, hy + 6, 2, 1, sombra(P_CLARA)); rp(7, hy + 7, PRETO); rp(8, hy + 7, PRETO); rp(11, hy + 3, sombra(RUIVA)); }
        if (!boca) rr(9, hy + 9, 2, 1, RGB(190, 40, 56));       // batom
        rr(3, hy, 14, 1, CHAPEU); rr(5, hy - 3, 10, 3, CHAPEU);  // chapeu de feltro
        rr(5, hy - 1, 10, 1, RGB(40, 40, 46)); rr(8, hy - 3, 4, 1, sombra(CHAPEU));
        break;
    }
    case 15: {                                                  // MARATONISTA: faixa na testa, agasalho, medalha, suor
        const uint16_t AGASALHO = RGB(40, 120, 90), FAIXA = RGB(80, 200, 210);
        tronco(AGASALHO, P_ESCURA, 0, 0, 3);
        rr(2, 18, 1, 4, C_BRANCO); rr(17, 18, 1, 4, C_BRANCO);   // listras do agasalho
        rr(8, 17, 4, 1, C_BRANCO);                              // ziper/gola
        rp(9, 19, C_OURO); rp(10, 19, C_OURO); rp(9, 20, C_OURO); rp(10, 20, gfx_mistura(C_OURO, C_BARRA, 30));   // medalha
        rp(8, 18, RGB(200, 40, 50)); rp(11, 18, RGB(200, 40, 50));
        rosto(hy, P_ESCURA, PRETO, 2, pisca, boca);
        rr(5, hy - 1, 10, 2, PRETO); rr(5, hy - 2, 2, 1, PRETO); rr(9, hy - 2, 3, 1, PRETO); rr(13, hy - 2, 2, 1, PRETO);   // cabelo crespo curto
        rr(5, hy + 1, 10, 1, FAIXA); rr(15, hy + 1, 1, 2, FAIXA); // faixa na testa, com a ponta solta
        int g = (int)(t / 0.5f) % 6;                            // gota de suor descendo
        if (g < 4) rp(14, hy + 3 + g, RGB(170, 220, 240));
        break;
    }
    case RETRATO_AVATAR + 0: {                                  // ET VILU: cabecao, olhos negros, antena
        const uint16_t PELE = RGB(128, 184, 112), ROUPA = RGB(150, 160, 178);
        tronco(ROUPA, PELE, 0, 0, 3);
        rr(7, 17, 6, 1, RGB(84, 196, 200)); rp(9, 19, RGB(84, 196, 200)); rp(10, 19, RGB(84, 196, 200));
        static const int8_t X0[13] = { 6, 5, 4, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8 };
        for (int i = 0; i < 13; i++) {
            rr(X0[i], hy - 2 + i, 20 - 2 * X0[i], 1, PELE);
            rp(19 - X0[i], hy - 2 + i, sombra(PELE));
        }
        rr(6, hy - 1, 3, 1, luz(PELE));                         // brilho do cocuruto
        uint16_t olho = RGB(18, 20, 26);
        if (pisca) { rr(5, hy + 5, 4, 1, sombra(PELE)); rr(11, hy + 5, 4, 1, sombra(PELE)); }
        else {                                                  // olhos amendoados, grandes, puxados para fora
            rr(4, hy + 2, 3, 1, olho); rr(4, hy + 3, 5, 1, olho); rr(5, hy + 4, 4, 1, olho); rr(6, hy + 5, 3, 1, olho);
            rr(13, hy + 2, 3, 1, olho); rr(11, hy + 3, 5, 1, olho); rr(11, hy + 4, 4, 1, olho); rr(11, hy + 5, 3, 1, olho);
            int g = (int)(t / 2.6f) % 2;                        // o brilho corre devagar
            rp(5 + g, hy + 3, RGB(96, 120, 132)); rp(13 + g, hy + 3, RGB(96, 120, 132));
        }
        rp(9, hy + 7, sombra(PELE)); rp(10, hy + 7, sombra(PELE));
        if (boca) rr(9, hy + 8, 2, 2, RGB(40, 30, 36));
        else rr(9, hy + 9, 2, 1, sombra(sombra(PELE)));
        int bal = (int)(t / 1.1f) % 4;                          // a antena balanca
        int ax = 10 + (bal == 1) - (bal == 3);
        rp(10, hy - 3, sombra(PELE)); rp(ax, hy - 4, sombra(PELE));
        bool acesa = fmodf(t, 2.2f) < 1.5f;
        rp(ax, hy - 5, acesa ? RGB(236, 246, 140) : RGB(130, 150, 80));
        break;
    }
    case RETRATO_AVATAR + 1: {                                  // O BERRO: caveira palida, maos no rosto
        fundo = 1;
        const uint16_t PELE = RGB(212, 198, 150), CASACO = RGB(38, 44, 66);
        rr(5, 16, 10, 6, CASACO); rr(4, 18, 12, 4, CASACO);     // casaco escuro, estreito
        rr(5, 16, 1, 6, luz(CASACO));
        rr(9, 14, 2, 2, sombra(PELE));
        // Cranio comprido, de cabeca de pera: largo na testa, fino no queixo.
        static const int8_t X0[14] = { 8, 7, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9 };
        for (int i = 0; i < 14; i++) {
            rr(X0[i], hy - 2 + i, 20 - 2 * X0[i], 1, PELE);
            rp(19 - X0[i], hy - 2 + i, sombra(PELE));
        }
        rr(8, hy - 1, 3, 1, luz(PELE));
        uint16_t oco = RGB(70, 66, 58);
        rr(7, hy + 3, 2, 2, oco); rr(11, hy + 3, 2, 2, oco);   // olhos fundos
        if (!pisca) { rp(8, hy + 3, luz(PELE)); rp(11, hy + 3, luz(PELE)); }
        rp(9, hy + 5, sombra(PELE)); rp(10, hy + 5, sombra(PELE));
        int ab = boca ? 1 : (int)(t / 1.8f) % 2 == 0;             // a boca em O, abrindo e fechando
        rr(9, hy + 7, 2, 2 + ab, RGB(48, 36, 40));
        rp(8, hy + 7 + ab, sombra(PELE)); rp(11, hy + 7 + ab, sombra(PELE));
        // As maos coladas nas bochechas: a borda de dentro, mais escura, separa do rosto.
        uint16_t mao = gfx_mistura(PELE, RGB(150, 160, 120), 25);
        rr(5, hy + 3, 2, 7, mao); rr(13, hy + 3, 2, 7, mao);
        rr(6, hy + 4, 1, 6, sombra(mao)); rr(13, hy + 4, 1, 6, sombra(mao));
        rp(5, hy + 3, luz(mao)); rp(14, hy + 3, luz(mao));
        rr(4, hy + 9, 3, 7, CASACO); rr(13, hy + 9, 3, 7, CASACO);   // bracos subindo
        break;
    }
    case RETRATO_AVATAR + 2: {                                  // NOMALISA: cabelo repartido, veu, meio sorriso
        fundo = 2;
        const uint16_t PELE = RGB(222, 184, 126), CAB = RGB(58, 40, 28), VEST = RGB(56, 50, 34);
        tronco(VEST, PELE, 0, 0, 2);
        rr(7, 17, 6, 1, RGB(146, 116, 60));                     // bordado da gola
        rr(3, hy + 2, 3, 13, CAB); rr(14, hy + 2, 3, 13, CAB);  // cabelo comprido, atras
        rosto(hy, PELE, PELE, 2, pisca, boca);
        if (!pisca) {                                           // os olhos que seguem quem olha
            bool esq = fmodf(t, 7.0f) > 4.6f;
            uint16_t olho = RGB(44, 30, 24);
            rp(7, hy + 6, esq ? olho : C_MARFIM); rp(8, hy + 6, esq ? C_MARFIM : olho);
            rp(11, hy + 6, esq ? olho : C_MARFIM); rp(12, hy + 6, esq ? C_MARFIM : olho);
        }
        rr(5, hy - 1, 4, 2, CAB); rr(11, hy - 1, 4, 2, CAB);    // repartido no meio
        rr(9, hy - 1, 2, 1, sombra(PELE));
        rr(5, hy + 1, 1, 4, CAB); rr(14, hy + 1, 1, 4, CAB);
        rr(5, hy - 2, 10, 1, RGB(36, 30, 26));                  // o veu fino
        rr(9, hy + 9, 2, 1, PELE);                              // o meio sorriso, sem dente
        rp(8, hy + 8, RGB(160, 100, 76)); rp(11, hy + 8, RGB(160, 100, 76));
        if (!boca) { rr(9, hy + 9, 2, 1, RGB(176, 120, 90)); }
        else rr(9, hy + 9, 2, 1, RGB(110, 60, 50));
        rr(10, 20, 6, 2, PELE); rr(9, 21, 2, 1, sombra(PELE));  // as maos cruzadas
        break;
    }
    case RETRATO_AVATAR + 3: {                                  // INTERRUPTOR DO FUTURO: meio rosto de metal
        const uint16_t PELE = P_MEDIA, MET = RGB(150, 156, 164), JAQ = RGB(34, 32, 36);
        tronco(JAQ, PELE, RGB(56, 54, 60), 0, 0);
        rr(3, 17, 3, 2, RGB(62, 60, 66)); rr(14, 17, 3, 2, RGB(62, 60, 66));     // gola de couro
        bool liga = fmodf(t, 5.0f) < 4.2f;                      // o interruptor do peito
        rr(13, 19, 3, 3, C_MARFIM); rp(14, liga ? 19 : 21, RGB(90, 90, 96)); rp(14, 20, RGB(120, 120, 126));
        rosto(hy, PELE, PRETO, 1, pisca, boca);
        for (int i = 0; i < 12; i++) {                          // metade direita: o cranio de metal
            int x1 = i < 1 || i > 10 ? 12 : 14;
            if (i == 11) x1 = 12;
            rr(10, hy + i, x1 - 10 + 1, 1, i & 1 ? MET : luz(MET));
        }
        rr(10, hy + 1, 1, 10, sombra(MET));                     // a costura entre os dois lados
        rp(13, hy + 3, sombra(MET)); rp(13, hy + 9, sombra(MET));
        float pul = fmodf(t, 1.6f);                             // o olho vermelho pulsando
        uint16_t olho = !liga ? RGB(70, 20, 20) : (pul < 0.5f ? RGB(255, 70, 50) : (pul < 1.1f ? RGB(214, 44, 36) : RGB(160, 30, 30)));
        rr(11, hy + 6, 2, 2, olho);
        if (liga && pul < 0.5f) rp(13, hy + 6, gfx_mistura(olho, MET, 50));
        for (int i = 0; i < 3; i++) rp(11 + i, hy + 9, i & 1 ? sombra(MET) : C_MARFIM);   // dentes
        rr(5, hy - 1, 6, 2, PRETO); rr(5, hy + 1, 1, 2, PRETO);   // cabelo curto, so do lado humano
        rr(10, hy - 1, 4, 2, sombra(MET));
        break;
    }
    case RETRATO_EVENTO + EVT_LAPIDADOR: {                      // LAPIDADOR: careca, lupa de joalheiro, avental
        const uint16_t COURO = RGB(124, 84, 52);
        tronco(COURO, P_CLARA, RGB(222, 216, 202), 0, 0);
        rr(5, 17, 1, 5, luz(COURO)); rr(14, 17, 1, 5, luz(COURO));   // alcas do avental
        rosto(hy, P_CLARA, GRISALHO, 0, pisca, boca);
        rr(6, hy - 1, 8, 1, P_CLARA); rr(7, hy - 1, 3, 1, luz(P_CLARA));   // careca com brilho
        rr(4, hy + 2, 2, 4, GRISALHO); rr(14, hy + 2, 2, 4, GRISALHO);
        rr(5, hy + 3, 10, 1, RGB(60, 44, 34));                  // a tira da lupa
        rr(10, hy + 5, 4, 4, RGB(54, 48, 46));                  // a lupa no olho direito
        rr(11, hy + 6, 2, 2, RGB(150, 190, 200));
        if ((int)(t / 2.2f) % 3 == 0) rp(11, hy + 6, C_BRANCO);
        if (!boca) rr(7, hy + 9, 6, 1, GRISALHO);               // bigode branco
        break;
    }
    case RETRATO_EVENTO + EVT_CAPELA: {                         // PADRE: batina, colarinho, cruz pequena
        const uint16_t BATINA = RGB(36, 36, 42);
        tronco(BATINA, P_MEDIA, 0, 0, 3);
        rr(8, 17, 4, 1, BATINA); rr(9, 17, 2, 1, C_BRANCO);     // colarinho
        rr(9, 19, 1, 3, C_OURO); rr(8, 20, 3, 1, C_OURO);       // a cruz
        bool reza = fmodf(t, 6.0f) > 4.2f;                      // fecha os olhos de vez em quando
        rosto(hy, P_MEDIA, GRISALHO, 0, pisca || reza, boca);
        rr(5, hy - 1, 10, 2, GRISALHO); rr(5, hy + 1, 1, 2, GRISALHO); rr(14, hy + 1, 1, 2, GRISALHO);
        rr(8, hy - 1, 4, 1, luz(GRISALHO));
        break;
    }
    case RETRATO_EVENTO + EVT_TROCADOR: {                       // TROCADOR: chapelao, bigode fino, dente de ouro
        const uint16_t CHAPEU = RGB(68, 42, 88);
        tronco(RGB(112, 32, 52), P_MEDIA, RGB(230, 220, 200), C_OURO, 0);
        rp(3, 19, C_MARFIM); rp(4, 20, C_MARFIM); rp(16, 19, C_MARFIM); rp(15, 20, C_MARFIM);   // dados na lapela
        rosto(hy, P_MEDIA, PRETO, 2, pisca, boca);
        rr(3, hy, 14, 1, CHAPEU);                               // aba larga
        rr(6, hy - 4, 8, 4, CHAPEU);
        rr(6, hy - 1, 8, 1, C_OURO);                            // fita
        rr(7, hy - 4, 1, 3, luz(CHAPEU));
        if ((int)(t / 4.0f) % 2) rp(12, hy + 3, sombra(PRETO)); // levanta uma sobrancelha
        if (!boca) {
            rr(7, hy + 8, 2, 1, PRETO); rr(11, hy + 8, 2, 1, PRETO);   // bigode fino
            if (fmodf(t, 3.0f) < 0.5f) rp(10, hy + 9, C_OURO);          // o dente de ouro brilha
            else rp(10, hy + 9, RGB(200, 160, 60));
        }
        break;
    }
    case RETRATO_EVENTO + EVT_GARCOM: {                         // GARCOM: cabelo lambido, gravata borboleta, pano no ombro
        tronco(RGB(236, 232, 222), P_CLARA, RGB(250, 250, 250), RGB(30, 30, 34), 1);
        rr(2, 17, 3, 5, C_BRANCO); rr(2, 19, 3, 1, RGB(150, 40, 50));   // o pano
        rosto(hy, P_CLARA, PRETO, 0, pisca, boca);
        rr(5, hy - 1, 10, 3, PRETO); rr(5, hy + 2, 1, 2, PRETO); rr(14, hy + 2, 1, 2, PRETO);
        rr(7, hy - 1, 4, 1, luz(PRETO));                        // brilho da brilhantina
        if (!pisca) {                                           // olha de lado, discreto
            bool lado = fmodf(t, 5.0f) > 3.2f;
            uint16_t olho = RGB(30, 26, 30);
            rp(7, hy + 6, lado ? olho : C_BRANCO); rp(8, hy + 6, lado ? C_BRANCO : olho);
            rp(11, hy + 6, lado ? olho : C_BRANCO); rp(12, hy + 6, lado ? C_BRANCO : olho);
        }
        if (!boca) rr(8, hy + 9, 4, 1, PRETO);                  // bigodinho
        break;
    }
    case RETRATO_EVENTO + EVT_COPISTA: {                        // COPISTA: boina, oculos redondos, pena atras da orelha
        tronco(RGB(92, 72, 52), P_PALIDA, RGB(222, 212, 192), 0, 0);
        rosto(hy, P_PALIDA, MARROM, 0, pisca, boca);
        rr(5, hy + 1, 1, 3, MARROM); rr(14, hy + 1, 1, 3, MARROM);
        rr(4, hy - 1, 11, 2, RGB(140, 40, 48)); rr(6, hy - 2, 8, 1, RGB(140, 40, 48));   // boina caida
        rr(13, hy - 1, 3, 1, sombra(RGB(140, 40, 48)));
        uint16_t aro = RGB(70, 60, 50);                         // oculos redondos
        rr(7, hy + 5, 2, 1, aro); rr(7, hy + 8, 2, 1, aro); rr(6, hy + 6, 1, 2, aro); rr(9, hy + 6, 1, 2, aro);
        rr(11, hy + 5, 2, 1, aro); rr(11, hy + 8, 2, 1, aro); rr(10, hy + 6, 1, 2, aro); rr(13, hy + 6, 1, 2, aro);
        if ((int)(t / 2.5f) % 3 == 1) rp(8, hy + 6, C_BRANCO);
        rp(16, hy + 1, C_BRANCO); rp(16, hy + 2, C_BRANCO); rp(17, hy, C_BRANCO); rp(17, hy - 1, C_MARFIM_S);   // a pena
        rp(15, hy + 3, PRETO);
        rp(14, 20, RGB(40, 40, 90)); rp(15, 21, RGB(40, 40, 90));   // mancha de tinta
        break;
    }
    case RETRATO_EVENTO + EVT_FONTE: {                          // FONTE: taca de pedra, agua caindo, moedas no fundo
        fundo = 3;
        const uint16_t PEDRA = RGB(150, 150, 140), AGUA = RGB(90, 160, 210);
        rr(0, 16, 20, 6, PEDRA); rr(0, 16, 20, 1, luz(PEDRA));  // a bacia
        rr(1, 17, 18, 2, AGUA);
        static const int8_t MOEDA[4] = { 3, 7, 12, 16 };
        for (int i = 0; i < 4; i++) rp(MOEDA[i], 18, ((int)(t / 0.7f) + i) % 3 ? C_OURO : C_AMARELO);
        rr(8, 6, 4, 10, PEDRA); rr(8, 6, 1, 10, luz(PEDRA));    // o pe
        rr(4, 4, 12, 2, PEDRA); rr(4, 4, 12, 1, luz(PEDRA));    // a taca de cima
        rr(9, 1, 2, 3, AGUA);                                   // o jato
        int f = (int)(t / 0.25f) % 3;
        rp(9 + (f == 1), -1 + (f == 2), luz(AGUA));
        for (int yy = 6; yy < 16; yy++)                          // agua caindo dos dois lados
            if ((yy + f) % 3) { rp(3, yy, AGUA); rp(16, yy, AGUA); }
        break;
    }
    case RETRATO_EVENTO + EVT_ESQUECIDO: {                      // DADO ESQUECIDO: luminaria e um dado sozinho no feltro
        fundo = 4;
        const uint16_t LUMI = RGB(46, 84, 64);
        rr(6, -3, 8, 2, LUMI); rr(5, -1, 10, 1, LUMI); rr(9, -4, 2, 1, RGB(60, 60, 60));
        bool fraca = fmodf(t, 3.7f) > 3.45f;                    // a lampada pisca de leve
        rr(8, 0, 4, 1, fraca ? RGB(200, 180, 110) : RGB(250, 230, 150));
        rr(8, 19, 7, 2, sombra(sombra(RGB(40, 110, 72))));       // sombra do dado
        rr(7, 13, 6, 6, C_MARFIM); rr(7, 13, 6, 1, C_BRANCO); rr(12, 14, 1, 5, C_MARFIM_S);
        rp(8, 14, C_EBANO_B); rp(10, 16, C_EBANO_B); rp(11, 17, C_EBANO_B);   // tres pontos
        break;
    }
    default: {                                                  // BARAO: cartola, monoculo, bigodao branco
        tronco(RGB(30, 30, 34), P_CLARA, RGB(236, 232, 222), RGB(30, 30, 34), 1);
        rr(13, 19, 3, 1, C_OURO);                               // corrente do relogio
        // Virado, o Barao fica vermelho de raiva, de cara fechada.
        uint16_t pele_b = bravo ? gfx_mistura(P_CLARA, RGB(222, 84, 72), 42) : P_CLARA;
        rosto(hy, pele_b, GRISALHO, bravo ? 1 : 0, pisca, boca);
        rr(5, hy + 2, 1, 3, GRISALHO); rr(14, hy + 2, 1, 3, GRISALHO);
        rr(4, hy - 1, 12, 1, RGB(28, 28, 30));                  // aba da cartola
        rr(6, hy - 6, 8, 5, RGB(40, 40, 44));
        rr(6, hy - 2, 8, 1, RGB(120, 40, 50));                  // fita
        rr(7, hy - 6, 1, 4, RGB(70, 70, 76));
        if (!boca) {                                            // bigodao branco, com sombra
            rr(7, hy + 9, 6, 1, RGB(238, 236, 230)); rp(6, hy + 10, RGB(238, 236, 230)); rp(13, hy + 10, RGB(238, 236, 230));
            rr(8, hy + 10, 4, 1, RGB(190, 186, 180));
        }
        rr(11, hy + 5, 2, 1, C_OURO); rr(11, hy + 8, 2, 1, C_OURO);   // monoculo: aro so no olho direito
        rr(10, hy + 6, 1, 2, C_OURO); rr(13, hy + 6, 1, 2, C_OURO);
        rp(14, hy + 8, C_OURO); rp(14, hy + 9, C_OURO);          // cordao
        if ((int)(t / 3.3f) % 3 == 0 && fmodf(t, 3.3f) < 0.4f) rp(12, hy + 6, C_BRANCO);
        if (bravo && !boca) {                                  // e bufa, de vez em quando
            float b = fmodf(t, 2.6f);
            if (b < 0.6f) {
                // Dois sopros saem do nariz e se afastam, sumindo.
                uint16_t vapor = b < 0.2f ? RGB(226, 230, 234) : b < 0.4f ? RGB(196, 202, 210) : RGB(150, 158, 168);
                if (b < 0.2f)      { rp(8, hy + 8, vapor); rp(11, hy + 8, vapor); }
                else if (b < 0.4f) { rr(5, hy + 8, 2, 1, vapor); rr(13, hy + 8, 2, 1, vapor); }
                else               { rp(3, hy + 7, vapor); rp(16, hy + 7, vapor); }
            }
        }
        break;
    }
    }
    // Contorno: tudo o que encosta num pixel cheio e esta vazio vira escuro.
    uint16_t contorno = RGB(14, 14, 13);
    for (int j = 0; j < RT_H; j++)
        for (int i = 0; i < RT_W; i++) {
            if (rt[j][i]) continue;
            bool perto = (i > 0 && rt[j][i - 1] > 1) || (i + 1 < RT_W && rt[j][i + 1] > 1)
                      || (j > 0 && rt[j - 1][i] > 1) || (j + 1 < RT_H && rt[j + 1][i] > 1);
            if (perto) rt[j][i] = 1;                           // 1 = contorno
        }
    if (fundo) {
        int fase_c = (int)(t / 1.4f) % 4;                      // o ceu ondula devagar
        for (int j = 0; j < RT_H; j++)
            for (int i = 0; i < RT_W; i++) {
                if (rt[j][i]) continue;
                uint16_t c;
                if (fundo == 1) {                              // o ceu em fogo, o fiorde e a ponte
                    static const uint16_t CEU[4] = { RGB(226, 120, 52), RGB(196, 72, 44), RGB(232, 168, 70), RGB(208, 98, 48) };
                    static const uint16_t MAR[3] = { RGB(42, 66, 108), RGB(60, 92, 132), RGB(36, 54, 86) };
                    int ponte = RT_H - 2 - i * 9 / RT_W;
                    if (j == ponte || j == ponte + 2) c = RGB(122, 78, 44);
                    else if (j > ponte) c = RGB(150, 106, 62);
                    else if (j < 12) c = CEU[((j + i / 4 + fase_c) & 7) >> 1];
                    else c = MAR[((j + (i + fase_c) / 3) % 6) >> 1];
                } else if (fundo == 3) {                       // jardim a noite, com estrelas
                    c = j < 14 ? RGB(22, 40, 52) : RGB(30, 52, 44);
                    if (j < 10 && (i * 7 + j * 13) % 23 == 0 && ((int)(t / 0.9f) + i) % 4) c = RGB(220, 220, 180);
                } else if (fundo == 4) {                       // feltro, com o cone de luz da luminaria
                    int d = abs(2 * i - 19), largura = 2 + j * 2 / 3;
                    c = (j >= 1 && d <= largura) ? RGB(58, 134, 88) : RGB(28, 78, 52);
                } else {                                       // paisagem enevoada, caminho sinuoso
                    int h1 = 13 + (i * 3 % 7) / 3, h2 = 17 + ((i + 3) % 9) / 4;
                    if (j < h1) c = j < 6 ? RGB(178, 182, 132) : RGB(156, 170, 128);
                    else if (j < h2) c = RGB(102, 124, 102);
                    else c = (i + j / 2) % 7 == 0 ? RGB(150, 112, 66) : RGB(74, 82, 56);
                }
                rt[j][i] = c;
            }
    }
    for (int j = l0; j < l0 + nl && j < RT_H; j++)
        for (int i = 0; i < RT_W; i++)
            if (rt[j][i]) gfx_rect(x + i * esc, y + (j - l0) * esc, esc, esc, rt[j][i] == 1 ? contorno : rt[j][i]);
}

// Balao de fala saindo do retrato (x, y: a ponta do bico).
static void desenha_balao(int x, int y, const char *s)
{
    int w = gfx_largura(s, 1) + 16, h = TXT_H + 8;
    int bx = x + 6, by = y - h / 2;
    if (bx + w > GFX_W - 6) bx = GFX_W - 6 - w;
    float a = t_fala < 0.25f ? t_fala / 0.25f : 1;
    if (a < 0.3f) return;
    gfx_rect(bx + 2, by + 2, w, h, C_SOMBRA);
    gfx_rect(bx + 1, by + 1, w - 2, h - 2, C_MARFIM);
    ui_contorno(bx, by, w, h, C_EBANO_B);
    for (int i = 0; i < 5; i++) gfx_rect(x + i, y - (5 - i) / 2, 7 - i, 5 - i, C_MARFIM);   // bico
    txt(bx + 8, by + 3, s, C_EBANO, false);
}

// Na faixa do oponente: o retrato (cortado no peito) e, se ele falou, o
// balao logo abaixo. Devolve onde o nome comeca.
static int avatar_retrato(void);
static int retrato_faixa(int j, int x, int y0)
{
    int perso = j == 1 ? fala_perso : avatar_retrato();
    if (perso < 0) return x + 12;
    int esc = 2, corte = 3;                     // em cima so tem a ponta da cartola
    int y = j == 1 ? y0 : y0 + 2;
    gfx_rect(x - 2, y, RT_W * esc + 4, FAIXA_H - 2, gfx_mistura(C_FELTRO_ESC, C_BARRA, 40));
    retrato(x, y, esc, perso, t_fase + perso * 1.3f, j == 1 && t_fala > 0.3f, corte, (FAIXA_H - 2) / esc);
    if (j == 1) {
        gfx_rect(x - 2, y0 + FAIXA_H - 2, RT_W * esc + 4, 2, C_LATAO_ESC);
        dica_zona(x, y0, RT_W * esc, FAIXA_H, DICA_TRUNFO, fala_perso);
    }
    return x + RT_W * esc + 8;
}

// O balao do oponente: depois de tudo, por cima da mesa.
static void balao_oponente(void)
{
    if (modo != M_DESAFIO || fala_perso < 0 || t_fala <= 0 || !fala_txt[0]) return;
    bool partida = fase == F_ORDEM || fase == F_JOGA || fase == F_ROLANDO || fase == F_ARRUMA
                || fase == F_VEREDITO || fase == F_APOSTA || fase == F_EFEITOS || fase == F_RESULTADO;
    if (!partida) return;
    int y = FAIXA_H + 14;
    desenha_balao(20, y, fala_txt);
}


static const char *const SALAO[DES_ANDARES] = { "Salão Verde", "Salão Vermelho", "Salão Dourado" };
static const uint16_t COR_SALAO[DES_ANDARES] = { C_VERDE, C_VINHO_CLR, C_OURO };
static const char *const NOME_NO[] = { "Fácil", "Médio", "Difícil", "Chefe", "Loja", "Evento" };
static uint16_t cor_no(int t)
{
    switch (t) {
    case NO_FACIL:   return RGB(104, 186, 110);
    case NO_MEDIO:   return C_LATAO;
    case NO_DIFICIL: return C_VINHO_CLR;
    case NO_CHEFE:   return C_OURO;
    case NO_EVENTO:  return RGB(164, 128, 214);
    default:         return C_AMARELO;
    }
}

// Nivel da IA de cada mesa: sobe pouco, porque os oponentes fortes ja tem
// dados melhores. [salao][facil, medio, dificil, chefe]
static const uint8_t NIVEL_NO[DES_ANDARES][4] = { { 0, 0, 0, 0 }, { 0, 0, 1, 1 }, { 1, 1, 1, 2 } };

typedef struct {
    uint8_t tipo, perso, nivel, liga;    // liga: bit l = leva a linha l do proximo passo
    uint32_t semente;
} no_t;

// Tudo o que a run guarda. Vai para o salvamento como esta (em hexadecimal).
typedef struct {
    char     marca[4];                   // "DES1"
    uint8_t  andar, passo, estado, vitorias;
    int32_t  moedas, ganho;              // ganho: moedas da ultima vitoria
    uint32_t semente;
    uint8_t  n_col, tier;                // tier: dificuldade da ultima mesa vencida
    peca_t   col[MAX_COL];
    no_t     no[DES_PASSOS][3];          // [passo][linha]; tipo NO_VAZIO = sem mesa
    int8_t   caminho[DES_PASSOS];        // linha escolhida em cada passo do salao
    peca_t   loja[N_LOJA];
    uint8_t  levou[N_LOJA];
    peca_t   oferta[3];                  // premio a escolher (estado R_PREMIO)
    uint16_t amuletos;                   // bit a: tem o amuleto a (os de 16 em diante em amuletos_hi)
    uint8_t  am_oferta[3];               // amuletos a escolher (estado R_AMULETO)
    uint8_t  am_volta;                   // depois da escolha: 0 loja da entrada, 1 segue no mapa
    uint8_t  am_loja, am_levou;          // amuleto a venda na loja (0xFF: nenhum)
    uint8_t  juros;                      // moedas que o Cofre rendeu na ultima vitoria
    uint8_t  dificuldade;                // 0 normal .. 5
    uint16_t amuletos_hi;
    uint8_t  taca;                       // moedas que a Taca rendeu na ultima vitoria
    uint8_t  premio2;                    // Selo de Divida: no segundo premio do chefe
    // Eventos: lapidou (o Lapidador ja foi usado: nao aparece mais), garcom
    // (o proximo oponente comeca com 10 fichas a menos), puxadas no
    // caca-niquel e dados que a Capela ja levou.
    uint8_t  lapidou, garcom, puxadas, tirados;
} run_t;

static run_t run;
static int   des_cur;                    // linha do cursor no mapa

static bool  des_venceu_run;             // tela final: venceu o Barao?
static int   des_liberou = -1;           // tela final: dificuldade que acabou de abrir
static int   des_dif;                    // dificuldade escolhida para a proxima run
static int   des_marco = -1;             // dado que a ultima vitoria abriu (chefe vencido pela 1a vez)
static int   op_amul_extra = -1;         // Diamante: o amuleto que o oponente ganhou
static int   des_perdeu_para = -1;       // quem encerrou a run (indice em PERSO)
static int   des_perdeu_andar;

// Sorteio proprio da run: montar mapa e oponentes nao mexe no sorteio da
// partida.
static uint32_t rs;
static uint32_t rnd_r(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static int sorte_r(int n) { return n > 0 ? (int)(rnd_r() % (uint32_t)n) : 0; }
