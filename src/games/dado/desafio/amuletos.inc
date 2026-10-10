// ---------------------------------------------------------------------------
// Amuletos: efeitos que valem a run inteira. Um no comeco, um por chefe
// vencido e um a venda em cada loja do caminho.
// ---------------------------------------------------------------------------
typedef struct { const char *nome, *desc; uint16_t cor; } amuleto_t;
static const amuleto_t AMULETO[A_N] = {
    [A_LUVA]     = { "Luva de Couro", "O primeiro dado da sequência não cai quando o seguinte é menor.", RGB(196, 140, 90) },
    [A_REDE]     = { "Rede",          "Se o último dado da sequência cair, cai sozinho: o de antes fica.", RGB(150, 186, 214) },
    [A_BOLSA]    = { "Bolsa Funda",   "Você compra 8 dados por rodada, em vez de 7.", RGB(204, 150, 96) },
    [A_OURO]     = { "Dado de Ouro",  "Dado seu que tirar o número máximo ganha +2.", C_OURO },
    [A_PENA]     = { "Peso Pena",     "Seus dados de 2 e 4 lados valem +1.", RGB(196, 176, 230) },
    [A_CONTRATO] = { "Contrato Sujo", "Todo dado seu que vale ganha +2, mas a sequência leva no máximo 3 dados.", C_VINHO_CLR },
    [A_IMA]      = { "Ímã",           "Nenhum dado seu pode ser roubado.", RGB(214, 84, 84) },
    [A_COURACA]  = { "Couraça",       "Os ataques do oponente não anulam os seus dados.", C_ACO },
    [A_COFRE]    = { "Cofre",         "Ao vencer uma mesa, rende 1 moeda a cada 5 que você tem.", C_LATAO },
    [A_RESERVA]  = { "Reserva",       "Você começa cada partida com 10 fichas a mais.", RGB(104, 186, 110) },
    [A_POTE]     = { "Pote Gordo",    "Vencendo uma rodada, você leva 2 fichas a mais.", RGB(222, 154, 50) },
    [A_FURADO]   = { "Saco Furado",   "O oponente compra só 6 dados por rodada.", RGB(170, 160, 150) },
    [A_FERRADURA]= { "Ferradura",     "Uma vez por rodada, na primeira queda que nada mais segurou, o dado de antes fica.", C_PRATA },
    [A_PESADO]   = { "Dado Pesado",   "Seus dados de 6 lados valem +1.", RGB(120, 128, 140) },
    [A_RELOGIO]  = { "Relógio de Bolso", "Na primeira rodada e quando você está atrás em fichas, o oponente abre; você joga depois.", RGB(214, 186, 110) },
    [A_CARTOLA]  = { "Cartola",       "O prêmio de cada mesa vencida sobe um nível.", RGB(150, 110, 170) },
    [A_COROA]    = { "Coroa do Azarão", "Começando a rodada com menos fichas que o oponente, seus dados valem +1.", RGB(226, 176, 70) },
    [A_CUPOM]    = { "Cupom",         "Tudo na loja custa 25% menos.", RGB(120, 190, 150) },
    [A_PRENSA]   = { "Prensa",        "Seu dado que tirar 1 passa a valer 3.", RGB(150, 150, 160) },
    [A_TACA]     = { "Taça",          "Ao vencer uma mesa, rende 2 moedas por rodada que você venceu nela.", C_OURO },
    [A_CORINGA]  = { "Coringa",       "O primeiro dado da sua sequência vale +2.", RGB(206, 80, 110) },
    [A_DUPLA]    = { "Dupla",         "Seu dado que repete o número de outro seu valendo ganha +2.", RGB(110, 160, 220) },
    [A_SELO]     = { "Selo de Dívida", "Chefe vencido dá dois prêmios de dados.", RGB(196, 60, 60) },
    [A_PEDAGIO]  = { "Pedágio",       "Os chefes começam a partida com 10 fichas a menos.", RGB(226, 186, 70) },
};
#define AM_PRECO 70


static int am_conta(uint32_t m) { int n = 0; for (; m; m >>= 1) n += m & 1; return n; }
static uint32_t run_amuletos(void) { return (uint32_t)run.amuletos | (uint32_t)run.amuletos_hi << 16; }
static void run_poe_amuleto(int a)
{
    uint32_t m = run_amuletos() | 1u << a;
    run.amuletos = (uint16_t)m;
    run.amuletos_hi = (uint16_t)(m >> 16);
}
static bool amuleto_liberado(int a);
static bool dado_liberado(int t);

// Sorteia n amuletos diferentes que a run ainda nao tem (0xFF se faltar).
static void sorteia_amuletos(uint8_t *out, int n)
{
    int livres[A_N], nl = 0;
    for (int a = 0; a < A_N; a++) if (!(run_amuletos() >> a & 1) && amuleto_liberado(a)) livres[nl++] = a;
    for (int i = 0; i < n; i++) {
        if (!nl) { out[i] = 0xFF; continue; }
        int q = sorte_r(nl);
        out[i] = (uint8_t)livres[q];
        livres[q] = livres[--nl];
    }
}

// O medalhao: aro de latao, fundo escuro na cor do amuleto e o simbolo.
static void desenha_amuleto(int x, int y, int r, int a, bool apagado)
{
    uint16_t cor = AMULETO[a].cor;
    if (apagado) cor = gfx_mistura(cor, C_FELTRO, 60);
    float u = r / 10.0f;
    int U = (int)(u + 0.5f) > 0 ? (int)(u + 0.5f) : 1;
    gfx_disco(x + 1, y + 2, r, C_SOMBRA);
    gfx_disco(x, y, r, apagado ? C_FELTRO2 : C_LATAO_ESC);
    gfx_disco(x, y, r - 1, apagado ? C_FELTRO2 : C_LATAO);
    gfx_disco(x, y, r - (r > 10 ? 3 : 2), gfx_mistura(cor, C_BARRA, 78));
    uint16_t claro = apagado ? C_FELTRO2 : C_MARFIM, esc = gfx_mistura(cor, C_BARRA, 45);
#define PX(v) (x + (int)lroundf((v) * u))
#define PY(v) (y + (int)lroundf((v) * u))
    switch (a) {
    case A_LUVA:                                       // luva de couro: palma, dedos e polegar
        gfx_rect(PX(-3), PY(-1), 6 * U, 5 * U, cor);
        for (int i = 0; i < 4; i++) gfx_rect(PX(-3 + i * 1.6f), PY(-5 + (i == 0 || i == 3 ? 1 : 0)), U + 1, 4 * U, cor);
        gfx_linha_grossa(PX(-3), PY(2), PX(-5.2f), PY(-0.5f), U + 1, cor);
        gfx_rect(PX(-3), PY(3), 6 * U, U, esc);                           // punho
        break;
    case A_REDE:                                       // malha de rede num aro
        for (int i = -1; i <= 1; i++) {
            int h = (int)(sqrtf(25 - i * 3 * i * 3) * u);
            gfx_rect(PX(i * 3), y - h, 1, 2 * h, claro);
            gfx_rect(x - h, PY(i * 3), 2 * h, 1, claro);
        }
        for (int i = 0; i < 24; i++) {
            float an = i * 0.2618f;
            gfx_rect(x + (int)(cosf(an) * 5 * u), y + (int)(sinf(an) * 5 * u), U, U, cor);
        }
        break;
    case A_BOLSA: case A_FURADO: {                     // saco de couro amarrado (furado: perdendo um dado)
        uint16_t c = a == A_BOLSA ? cor : gfx_mistura(cor, C_EBANO_B, 25);
        uint16_t dobra = gfx_mistura(c, C_BARRA, 35), luz = gfx_mistura(c, C_BRANCO, 30);
        gfx_disco(x, PY(1.6f), (int)(4.0f * u + 0.5f), c);                     // fundo cheio
        float vx[4] = { (float)PX(-1.0f), (float)PX(1.0f), (float)PX(3.7f), (float)PX(-3.7f) };
        float vy[4] = { (float)PY(-2.8f), (float)PY(-2.8f), (float)PY(0.8f), (float)PY(0.8f) };
        poligono(vx, vy, 4, c);                                                  // ombros
        // boca franzida, abrindo para fora acima do cordao
        float bx[7] = { (float)PX(-1.0f), (float)PX(1.0f), (float)PX(3.0f), (float)PX(1.5f), (float)x, (float)PX(-1.5f), (float)PX(-3.0f) };
        float by[7] = { (float)PY(-2.9f), (float)PY(-2.9f), (float)PY(-5.0f), (float)PY(-4.3f), (float)PY(-5.2f), (float)PY(-4.3f), (float)PY(-5.0f) };
        poligono(bx, by, 7, luz);
        gfx_rect(PX(-1.5f), PY(-3.2f), (int)(3.0f * u) + 1, U + 1, C_OURO);    // cordao
        gfx_disco(PX(1.9f), PY(-2.4f), U, C_OURO);                             // ponta do laco
        gfx_linha_grossa(PX(-0.6f), PY(-1.2f), PX(-1.4f), PY(3.6f), 1, dobra);  // dobras do couro
        gfx_linha_grossa(PX(1.0f), PY(-0.8f), PX(1.6f), PY(3.2f), 1, dobra);
        gfx_disco(PX(-1.8f), PY(0.4f), U, luz);
        if (a == A_FURADO) {
            gfx_disco(PX(1.4f), PY(4.4f), U, C_BARRA);                         // o furo
            gfx_rect(PX(2.6f), PY(5.6f), U + 1, U + 1, C_MARFIM);              // um dado escapando
        }
        break;
    }
    case A_OURO:                                       // um d6 de ouro mostrando 3
        gfx_rect(PX(-3.6f), PY(-3.6f), (int)(7.2f * u) + 1, (int)(7.2f * u) + 1, C_LATAO_ESC);
        gfx_rect(PX(-3.2f), PY(-3.2f), (int)(6.4f * u) + 1, (int)(6.4f * u) + 1, C_OURO);
        for (int i = -1; i <= 1; i++) gfx_disco(PX(i * 1.8f), PY(-i * 1.8f), (int)(0.7f * u + 0.5f), C_LATAO_ESC);
        break;
    case A_PENA: {                                     // pena na diagonal
        float vx[6] = { (float)PX(-4), (float)PX(-1), (float)PX(3), (float)PX(4.2f), (float)PX(1.5f), (float)PX(-2.5f) };
        float vy[6] = { (float)PY(4), (float)PY(-1.5f), (float)PY(-4.5f), (float)PY(-4.2f), (float)PY(0), (float)PY(3.2f) };
        poligono(vx, vy, 6, cor);
        gfx_linha_grossa(PX(-4.6f), PY(4.6f), PX(3.6f), PY(-3.6f), 1, claro);
        break;
    }
    case A_CONTRATO:                                   // papel escrito e o lacre
        gfx_rect(PX(-3.4f), PY(-4.4f), (int)(6.8f * u) + 1, (int)(8.8f * u) + 1, claro);
        for (int i = 0; i < 3; i++) gfx_rect(PX(-2.4f), PY(-3 + i * 1.8f), (int)((i == 2 ? 3 : 4.8f) * u), 1, C_EBANO_B);
        gfx_disco(PX(1.8f), PY(2.8f), (int)(1.6f * u + 0.5f), cor);
        break;
    case A_IMA:                                        // ima de ferradura
        gfx_disco(x, PY(1), (int)(4.2f * u), cor);
        gfx_disco(x, PY(1), (int)(1.8f * u), gfx_mistura(cor, C_BARRA, 78));
        gfx_rect(PX(-1.8f), PY(-4.4f), (int)(3.6f * u) + 1, (int)(5.4f * u), gfx_mistura(cor, C_BARRA, 78));
        gfx_rect(PX(-4.2f), PY(-4.4f), (int)(2.4f * u) + 1, (int)(5.4f * u), cor);
        gfx_rect(PX(1.8f), PY(-4.4f), (int)(2.4f * u) + 1, (int)(5.4f * u), cor);
        gfx_rect(PX(-4.2f), PY(-4.4f), (int)(2.4f * u) + 1, (int)(1.4f * u) + 1, C_PRATA);
        gfx_rect(PX(1.8f), PY(-4.4f), (int)(2.4f * u) + 1, (int)(1.4f * u) + 1, C_PRATA);
        break;
    case A_COURACA: {                                  // escudo de aco
        float vx[5] = { (float)PX(-4), (float)PX(4), (float)PX(4), (float)x, (float)PX(-4) };
        float vy[5] = { (float)PY(-4.4f), (float)PY(-4.4f), (float)PY(0.5f), (float)PY(5), (float)PY(0.5f) };
        poligono(vx, vy, 5, cor);
        gfx_rect(x, PY(-3.6f), U, (int)(7.6f * u), gfx_mistura(cor, C_BRANCO, 40));
        gfx_rect(PX(-3.2f), PY(-1.2f), (int)(6.4f * u) + 1, U, gfx_mistura(cor, C_BRANCO, 40));
        break;
    }
    case A_COFRE:                                      // cofre com segredo
        gfx_rect(PX(-4.2f), PY(-4.2f), (int)(8.4f * u) + 1, (int)(8.4f * u) + 1, gfx_mistura(cor, C_BARRA, 30));
        gfx_rect(PX(-3.6f), PY(-3.6f), (int)(7.2f * u) + 1, (int)(7.2f * u) + 1, C_CHUMBO);
        gfx_disco(x, y, (int)(2.4f * u + 0.5f), C_PRATA);
        gfx_disco(x, y, (int)(1.2f * u + 0.5f), C_CHUMBO);
        gfx_rect(x, PY(-2.4f), 1, U + 1, C_BARRA);
        break;
    case A_RESERVA:                                    // tres fichas empilhadas de lado
        for (int i = 0; i < 3; i++) {
            int fx = PX(-2.2f + i * 2.2f), fy = PY(2.2f - i * 2.2f), fr = (int)(2.8f * u + 0.5f);
            uint16_t c = i == 0 ? C_VERMELHO : (i == 1 ? cor : C_AZUL);
            gfx_disco(fx, fy, fr + 1, C_BARRA);
            gfx_disco(fx, fy, fr, c);
            gfx_disco(fx, fy, fr > 3 ? fr - 2 : 1, gfx_mistura(c, C_BRANCO, 35));
            gfx_disco(fx, fy, fr > 4 ? fr - 3 : 1, c);
        }
        break;
    case A_POTE: {                                     // panela de bronze cheia de moedas
        uint16_t bronze = gfx_mistura(cor, C_BARRA, 30), borda = gfx_mistura(cor, C_BRANCO, 20);
        gfx_disco(x, PY(1.4f), (int)(3.8f * u + 0.5f), bronze);                // bojo
        gfx_rect(PX(-3.8f), PY(-1.4f), (int)(7.6f * u) + 1, (int)(2.8f * u), bronze);
        static const float M[5][2] = { {-2.2f,-2.0f}, {0,-2.6f}, {2.2f,-2.0f}, {-1.1f,-3.6f}, {1.1f,-3.6f} };
        for (int i = 0; i < 5; i++) {                                          // moedas por cima
            int mr = (int)(1.3f * u + 0.5f);
            gfx_disco(PX(M[i][0]), PY(M[i][1]), mr, C_LATAO_ESC);
            gfx_disco(PX(M[i][0]), PY(M[i][1]), mr > 1 ? mr - 1 : 1, C_OURO);
        }
        gfx_rect(PX(-4.4f), PY(-1.6f), (int)(8.8f * u) + 1, U + 1, borda);       // aba da panela
        gfx_rect(PX(-3.0f), PY(4.8f), U + 1, U, bronze);                       // pezinhos
        gfx_rect(PX(2.0f), PY(4.8f), U + 1, U, bronze);
        break;
    }
    case A_FERRADURA: {                                // ferradura aberta para cima, com os cravos
        uint16_t fundo = gfx_mistura(cor, C_BARRA, 78), som = gfx_mistura(cor, C_BARRA, 35);
        gfx_disco(x, PY(0.8f), (int)(4.6f * u + 0.5f), cor);
        gfx_disco(x, PY(0.8f), (int)(2.2f * u + 0.5f), fundo);
        gfx_rect(PX(-2.2f), PY(-5.0f), (int)(4.4f * u) + 1, (int)(5.8f * u), fundo);
        gfx_rect(PX(-4.6f), PY(-3.6f), (int)(2.4f * u) + 1, (int)(4.4f * u), cor);
        gfx_rect(PX(2.2f), PY(-3.6f), (int)(2.4f * u) + 1, (int)(4.4f * u), cor);
        gfx_rect(PX(-5.0f), PY(-4.4f), (int)(3.0f * u) + 1, (int)(1.2f * u) + 1, som);    // as pontas viradas
        gfx_rect(PX(2.0f), PY(-4.4f), (int)(3.0f * u) + 1, (int)(1.2f * u) + 1, som);
        static const float C[6][2] = { {-3.4f,-1.8f}, {-3.4f,1.2f}, {-1.4f,3.8f}, {1.4f,3.8f}, {3.4f,1.2f}, {3.4f,-1.8f} };
        for (int i = 0; i < 6; i++) gfx_rect(PX(C[i][0]) - (U > 1), PY(C[i][1]) - (U > 1), U, U, C_EBANO_B);
        break;
    }
    case A_PESADO: {                                   // d6 de ferro, grosso, mostrando 6
        int l = (int)(7.0f * u) + 1, d = U + 1;
        int x0 = PX(-4.0f), y0 = PY(-3.4f);
        gfx_rect(x0 + d, y0 + d, l, l, gfx_mistura(cor, C_BARRA, 55));        // a espessura
        gfx_rect(x0, y0, l, l, gfx_mistura(cor, C_BRANCO, 30));
        gfx_rect(x0, y0, l, U, gfx_mistura(cor, C_BRANCO, 60));
        for (int i = 0; i < 3; i++) {
            gfx_rect(PX(-2.4f), PY(-2.0f + i * 2.0f), U, U, C_EBANO_B);
            gfx_rect(PX(0.8f), PY(-2.0f + i * 2.0f), U, U, C_EBANO_B);
        }
        break;
    }
    case A_RELOGIO:                                    // relogio de bolso com a coroa em cima
        gfx_rect(PX(-0.8f), PY(-5.4f), (int)(1.6f * u) + 1, (int)(1.4f * u) + 1, cor);
        gfx_disco(x, PY(0.6f), (int)(4.4f * u + 0.5f), cor);
        gfx_disco(x, PY(0.6f), (int)(3.4f * u + 0.5f), claro);
        for (int i = 0; i < 4; i++) {
            float an = i * 1.5708f;
            gfx_rect(x + (int)(cosf(an) * 2.7f * u), PY(0.6f) + (int)(sinf(an) * 2.7f * u), 1, 1, esc);
        }
        gfx_linha_grossa(x, PY(0.6f), x, PY(-1.8f), 1, C_EBANO_B);
        gfx_linha_grossa(x, PY(0.6f), PX(1.8f), PY(1.4f), 1, C_EBANO_B);
        break;
    case A_CARTOLA:                                    // cartola preta com fita
        gfx_rect(PX(-4.8f), PY(2.6f), (int)(9.6f * u) + 1, (int)(1.4f * u) + 1, C_EBANO_B);
        gfx_rect(PX(-2.8f), PY(-4.4f), (int)(5.6f * u) + 1, (int)(7.2f * u), C_EBANO_B);
        gfx_rect(PX(-2.8f), PY(0.6f), (int)(5.6f * u) + 1, (int)(1.4f * u) + 1, cor);
        gfx_rect(PX(-2.2f), PY(-3.8f), U, (int)(3.6f * u), gfx_mistura(C_EBANO_B, C_BRANCO, 25));
        break;
    case A_COROA: {                                    // coroa amassada: uma ponta torta
        float vx[9] = { (float)PX(-4.2f), (float)PX(-4.2f), (float)PX(-2.4f), (float)PX(-1.2f), (float)x,
                        (float)PX(1.4f), (float)PX(4.0f), (float)PX(4.2f), (float)PX(4.2f) };
        float vy[9] = { (float)PY(3.2f), (float)PY(-3.8f), (float)PY(-0.6f), (float)PY(-4.4f), (float)PY(-0.6f),
                        (float)PY(-0.4f), (float)PY(-2.6f), (float)PY(0), (float)PY(3.2f) };
        poligono(vx, vy, 9, cor);
        gfx_rect(PX(-4.2f), PY(1.6f), (int)(8.4f * u) + 1, U, esc);
        gfx_rect(PX(-2.6f), PY(2.4f), U, U, C_VERMELHO);
        gfx_rect(PX(1.8f), PY(2.4f), U, U, C_AZUL);
        break;
    }
    case A_CUPOM:                                      // tiquete com recortes e o sinal de %
        gfx_rect(PX(-4.6f), PY(-3.0f), (int)(9.2f * u) + 1, (int)(6.0f * u) + 1, claro);
        gfx_disco(PX(-4.6f), y, (int)(1.2f * u + 0.5f), gfx_mistura(cor, C_BARRA, 78));
        gfx_disco(PX(4.6f), y, (int)(1.2f * u + 0.5f), gfx_mistura(cor, C_BARRA, 78));
        gfx_linha_grossa(PX(1.6f), PY(-1.8f), PX(-1.6f), PY(1.8f), 1, cor);
        gfx_rect(PX(-1.8f), PY(-1.8f), U + 1, U + 1, cor);
        gfx_rect(PX(1.0f), PY(1.0f), U + 1, U + 1, cor);
        break;
    case A_PRENSA: {                                   // prensa apertando um dado que mostra 3
        uint16_t ferro = gfx_mistura(cor, C_BARRA, 25);
        gfx_rect(PX(-3.0f), PY(-5.4f), (int)(6.0f * u) + 1, U, esc);              // manivela
        gfx_rect(PX(-0.5f), PY(-5.0f), U, (int)(2.0f * u), esc);                  // fuso
        gfx_rect(PX(-4.2f), PY(-3.2f), (int)(8.4f * u) + 1, (int)(1.2f * u) + 1, ferro);
        gfx_rect(PX(-4.2f), PY(3.4f), (int)(8.4f * u) + 1, (int)(1.2f * u) + 1, ferro);
        int l = (int)(5.0f * u) + 1;
        gfx_rect(PX(-2.5f), PY(-1.8f), l, l, C_MARFIM);
        for (int i = -1; i <= 1; i++) gfx_rect(PX(i * 1.4f) - U / 2, PY(0.7f - i * 1.4f) - U / 2, U, U, C_EBANO_B);
        break;
    }
    case A_TACA:                                       // taca com alcas, haste e base
        gfx_disco(x, PY(-2.4f), (int)(3.4f * u + 0.5f), cor);
        gfx_rect(PX(-3.4f), PY(-4.6f), (int)(6.8f * u) + 1, (int)(2.2f * u), cor);
        gfx_disco(PX(-3.8f), PY(-2.6f), (int)(1.4f * u + 0.5f), esc);
        gfx_disco(PX(3.8f), PY(-2.6f), (int)(1.4f * u + 0.5f), esc);
        gfx_disco(PX(-3.8f), PY(-2.6f), (int)(0.6f * u + 0.5f), gfx_mistura(cor, C_BARRA, 78));
        gfx_disco(PX(3.8f), PY(-2.6f), (int)(0.6f * u + 0.5f), gfx_mistura(cor, C_BARRA, 78));
        gfx_disco(x, PY(-2.4f), (int)(3.4f * u + 0.5f), cor);
        gfx_rect(PX(-0.6f), PY(0.8f), (int)(1.2f * u) + 1, (int)(2.4f * u), esc);
        gfx_rect(PX(-2.6f), PY(3.2f), (int)(5.2f * u) + 1, (int)(1.4f * u) + 1, cor);
        gfx_rect(PX(-1.8f), PY(-4.0f), U, (int)(2.6f * u), gfx_mistura(cor, C_BRANCO, 45));
        break;
    case A_CORINGA: {                                  // carta com a estrela do coringa
        gfx_rect(PX(-3.2f), PY(-4.6f), (int)(6.4f * u) + 1, (int)(9.2f * u) + 1, claro);
        float vx[10], vy[10];
        for (int i = 0; i < 10; i++) {
            float an = -1.5708f + i * 0.6283f, rr = (i & 1 ? 1.1f : 2.6f) * u;
            vx[i] = x + cosf(an) * rr; vy[i] = y + sinf(an) * rr;
        }
        poligono(vx, vy, 10, cor);
        gfx_rect(PX(-2.6f), PY(-4.0f), U, U, cor);
        gfx_rect(PX(1.8f), PY(3.2f), U, U, cor);
        break;
    }
    case A_SELO: {                                     // nota promissoria com o lacre de cera
        gfx_rect(PX(-4.2f), PY(-4.0f), (int)(7.4f * u) + 1, (int)(7.0f * u) + 1, claro);
        for (int i = 0; i < 3; i++) gfx_rect(PX(-3.2f), PY(-2.8f + i * 1.6f), (int)((i == 2 ? 2.6f : 5.2f) * u), 1, C_EBANO_B);
        int sr = (int)(2.2f * u + 0.5f);
        gfx_disco(PX(2.0f), PY(2.2f), sr + 1, gfx_mistura(cor, C_BARRA, 40));
        gfx_disco(PX(2.0f), PY(2.2f), sr, cor);
        gfx_disco(PX(2.0f), PY(2.2f), sr > 2 ? sr - 2 : 1, gfx_mistura(cor, C_BARRA, 25));
        gfx_disco(PX(1.5f), PY(1.6f), U > 1 ? U - 1 : 1, gfx_mistura(cor, C_BRANCO, 45));
        break;
    }
    case A_PEDAGIO: {                                  // cancela listrada, baixada, presa no poste
        gfx_rect(PX(-4.4f), PY(-2.0f), (int)(1.6f * u) + 1, (int)(6.6f * u), gfx_mistura(cor, C_BARRA, 35));   // poste
        gfx_rect(PX(-4.8f), PY(4.2f), (int)(2.4f * u) + 1, U, gfx_mistura(cor, C_BARRA, 50));
        int y0 = PY(-2.0f), h = (int)(2.4f * u) + 1;
        for (int i = 0; i < 4; i++)                                        // braco: vermelho e branco
            gfx_rect(PX(-2.8f + i * 1.9f), y0, (int)(1.9f * u) + 1, h, i & 1 ? C_MARFIM : C_VERMELHO);
        gfx_disco(PX(-3.6f), PY(-0.8f), U + 1, C_EBANO_B);                // o eixo
        gfx_disco(PX(-3.6f), PY(-3.4f), U, cor);                          // a lampada
        break;
    }
    case A_DUPLA:                                      // dois dados iguais, mostrando 2
        for (int i = 0; i < 2; i++) {
            int lado = (int)(4.6f * u) + 1;
            int dx = i ? PX(0.2f) : PX(-4.8f), dy = i ? PY(-0.6f) : PY(-3.8f);
            gfx_rect(dx, dy, lado, lado, C_EBANO_B);
            gfx_rect(dx + 1, dy + 1, lado - 2, lado - 2, C_MARFIM);
            int m = lado / 4 + (U > 1);
            gfx_rect(dx + m, dy + m, U, U, C_EBANO_B);
            gfx_rect(dx + lado - m - U, dy + lado - m - U, U, U, C_EBANO_B);
        }
        break;
    }
#undef PX
#undef PY
}

// Os amuletos da run, miudos, numa linha (na faixa da partida): os que
// acabaram de agir acendem. Do oponente, so o que o Diamante deu (o trunfo
// dele ja aparece no retrato).
static void amuletos_faixa(int j, int x, int y)
{
    int n = 0;
    for (int a = 0; a < A_N && n < 9; a++) {
        if (!(amul[j] >> a & 1) || (j == 1 && a != op_amul_extra)) continue;
        int cx = x + n * 19;
        if (am_brilho[a] > 0) {
            int rr = 10 + (int)((1.2f - am_brilho[a]) * 6);
            gfx_disco(cx, y, rr, gfx_mistura(C_OURO, C_BARRA, (int)((1.2f - am_brilho[a]) * 70)));
        }
        desenha_amuleto(cx, y, 8, a, false);
        dica_zona(cx - 9, y - 9, 18, 18, DICA_AMULETO, a);
        n++;
    }
}
