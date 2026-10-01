// ===========================================================================
// MODO DESAFIANTE: uma run pelo cassino, de salao em salao.
//
// Incluido pelo dado.c (usa as mesmas regras, dados e desenho); nao se compila
// sozinho. Nada daqui muda a partida rapida, as 2 pessoas ou o online.
//
// A run: 3 saloes, 4 passos cada. Em cada passo o jogador escolhe um caminho
// entre 2 ou 3 mesas (oponente facil, medio ou dificil, as vezes uma loja); o
// quarto passo e o chefe do salao. Cada partida tem 8 rodadas e 50 fichas de
// cada lado; vencer converte as fichas que sobraram em moedas (1 por 1) e da
// um premio de dado. Perder encerra a run. Nada passa de uma run para outra.
// ===========================================================================
#include "hal/salva.h"

#define DES_ANDARES   3
#define DES_PASSOS    4                  // o ultimo e o chefe
#define DES_FICHAS    50


#define DES_MOEDAS    50                 // moedas no comeco da run

enum { NO_FACIL, NO_MEDIO, NO_DIFICIL, NO_CHEFE, NO_LOJA, NO_VAZIO = 0xFF };
enum { R_LOJA_INI, R_MAPA, R_PARTIDA, R_PREMIO, R_LOJA, R_AMULETO };

// Os oponentes do cassino. Cada um tem um jeito de apostar e dados que prefere.
typedef struct {
    const char *nome, *frase;
    float ousadia;                       // -1 cauteloso ... +1 aposta alto
    uint8_t fav[5];                      // dados preferidos (0xFF = fim)
} perso_t;

static const perso_t PERSO[] = {
    { "O NOVATO",        "Chegou hoje ao cassino.",           -0.3f,
      { D_D8, D_D12, D_TEIMOSO, D_QUEBRADO, D_SENTINELA } },
    { "A CAUTELOSA",     "Aposta pouco e não arrisca.",       -1.0f,
      { D_ESCUDO, D_TEIMOSO, D_LASTRO, D_SENTINELA, D_ACUMULADOR } },
    { "O APOSTADOR",     "Dobra tudo, sempre.",                1.0f,
      { D_TUDO_NADA, D_EXPLOSIVO, D_D20, D_FICHAS, D_VENTANIA } },
    { "O LADRÃO",        "O que é seu pode ser dele.",         0.3f,
      { D_GATUNO, D_CARIDOSO, D_PIRATA, D_ESPELHO, D_COBRADOR } },
    { "A MATEMÁTICA",    "Conta cada multiplicador.",          0.0f,
      { D_DOBRO, D_FARTURA, D_COPIADOR, D_FERREIRO, D_ACUMULADOR } },
    { "O ESPINHENTO",    "Cada ponto seu custa caro.",         0.4f,
      { D_MARTELO, D_MALDICAO, D_CARRASCO, D_INVEJOSO, D_LASER } },
    { "A SUPERSTICIOSA", "Confia no trevo e na moeda.",        0.2f,
      { D_TREVO, D_SORTE, D_SEMENTE, D_JACKPOT, D_MISERICORDIOSO } },
    { "O BRUTAMONTES",   "Dados grandes, pouca conversa.",     0.5f,
      { D_D20, D_LASTRO, D_EGOISTA, D_EXPLOSIVO, D_DESAFIANTE } },
    // Chefes, um por salao.
    { "O CRUPIÊ",        "Conhece cada dado desta mesa.",      0.0f,
      { D_VICIADO, D_LASTRO, D_DOBRO, D_ESCUDO, D_CARRASCO } },
    { "A DONA DO SALÃO", "Nunca perdeu no próprio salão.",     0.4f,
      { D_ESPELHO, D_INVEJOSO, D_FOGO, D_COPIADOR, D_EGOISTA } },
    { "O BARÃO",         "O maior apostador do cassino.",      0.8f,
      { D_TUDO_NADA, D_MALDITO, D_EXPLOSIVO, D_FOGO, D_JACKPOT } },
};
#define N_PERSO_COMUM 8

static const char *const SALAO[DES_ANDARES] = { "Salão Verde", "Salão Vermelho", "Salão Dourado" };
static const uint16_t COR_SALAO[DES_ANDARES] = { C_VERDE, C_VINHO_CLR, C_OURO };
static const char *const NOME_NO[] = { "fácil", "médio", "difícil", "chefe", "loja" };
static uint16_t cor_no(int t)
{
    switch (t) {
    case NO_FACIL:   return RGB(104, 186, 110);
    case NO_MEDIO:   return C_LATAO;
    case NO_DIFICIL: return C_VINHO_CLR;
    case NO_CHEFE:   return C_OURO;
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
    // Daqui para baixo, o que veio depois do primeiro salvamento: um arquivo
    // antigo (RUN_TAM_V1 bytes) carrega com tudo isto zerado.
    uint16_t amuletos;                   // bit a: tem o amuleto a
    uint8_t  am_oferta[3];               // amuletos a escolher (estado R_AMULETO)
    uint8_t  am_volta;                   // depois da escolha: 0 loja da entrada, 1 segue no mapa
    uint8_t  am_loja, am_levou;          // amuleto a venda na loja (0xFF: nenhum)
    uint8_t  juros;                      // moedas que o Cofre rendeu na ultima vitoria
} run_t;
#define RUN_TAM_V1 244                   // o run_t antes dos amuletos

static run_t run;
static int   des_cur;                    // linha do cursor no mapa

static bool  des_venceu_run;             // tela final: venceu o Barao?
static int   des_perdeu_para = -1;       // quem encerrou a run (indice em PERSO)
static int   des_perdeu_andar;

// Sorteio proprio da run: montar mapa e oponentes nao mexe no sorteio da
// partida.
static uint32_t rs;
static uint32_t rnd_r(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static int sorte_r(int n) { return n > 0 ? (int)(rnd_r() % (uint32_t)n) : 0; }

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
    [A_CONTRATO] = { "Contrato Sujo", "+50% de moedas por vitória, mas a sequência leva no máximo 3 dados.", C_VINHO_CLR },
    [A_IMA]      = { "Ímã",           "Nenhum dado seu pode ser roubado.", RGB(214, 84, 84) },
    [A_COURACA]  = { "Couraça",       "Os ataques do oponente não anulam os seus dados.", C_ACO },
    [A_COFRE]    = { "Cofre",         "Ao vencer uma mesa, rende 1 moeda a cada 5 que você tem (até 15).", C_LATAO },
    [A_RESERVA]  = { "Reserva",       "Você começa cada partida com 15 fichas a mais.", RGB(104, 186, 110) },
    [A_POTE]     = { "Pote Gordo",    "Vencendo uma rodada, você leva 3 fichas a mais.", RGB(222, 154, 50) },
    [A_FURADO]   = { "Saco Furado",   "O oponente compra só 6 dados por rodada.", RGB(170, 160, 150) },
};
#define AM_PRECO 70

static int am_conta(uint16_t m) { int n = 0; for (; m; m >>= 1) n += m & 1; return n; }

// Sorteia n amuletos diferentes que a run ainda nao tem (0xFF se faltar).
static void sorteia_amuletos(uint8_t *out, int n)
{
    int livres[A_N], nl = 0;
    for (int a = 0; a < A_N; a++) if (!(run.amuletos >> a & 1)) livres[nl++] = a;
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
    }
#undef PX
#undef PY
}

// Os amuletos da run, miudos, numa linha (na faixa da partida): os que
// acabaram de agir acendem.
static void amuletos_faixa(int x, int y)
{
    int n = 0;
    for (int a = 0; a < A_N && n < 9; a++) {
        if (!(amul[0] >> a & 1)) continue;
        int cx = x + n * 19;
        if (am_brilho[a] > 0) {
            int rr = 10 + (int)((1.2f - am_brilho[a]) * 6);
            gfx_disco(cx, y, rr, gfx_mistura(C_OURO, C_BARRA, (int)((1.2f - am_brilho[a]) * 70)));
        }
        desenha_amuleto(cx, y, 8, a, false);
        n++;
    }
}


// ---------------------------------------------------------------------------
// Dados sorteados por raridade
// ---------------------------------------------------------------------------
// Pesos por dado de cada raridade [comum, incomum, raro, lenda]. Os dados
// iniciais (moeda, D4, D6) nunca saem, como no premio da partida normal.
static int sorteia_raridade(const float w[4])
{
    float total = 0;
    for (int t = 0; t < D_N; t++) if (!so_inicial(t)) total += w[TIPO[t].raridade];
    float x = (float)(rnd_r() % 1000000u) / 1000000.0f * total;
    for (int t = 0; t < D_N; t++) {
        if (so_inicial(t)) continue;
        x -= w[TIPO[t].raridade];
        if (x < 0) return t;
    }
    return D_D8;
}

static peca_t peca_r(int t)
{
    peca_t p = { (uint8_t)t, 0 };
    if (t == D_QUEBRADO) p.fixo = (uint8_t)(1 + sorte_r(8));
    return p;
}

// Tres (ou n) dados diferentes entre si.
static void sorteia_distintos(peca_t *out, int n, const float w[4])
{
    for (int i = 0; i < n; i++) {
        bool rep;
        int tent = 0;
        do {
            out[i] = peca_r(sorteia_raridade(w));
            rep = false;
            for (int k = 0; k < i; k++) if (out[k].tipo == out[i].tipo) rep = true;
        } while (rep && ++tent < 50);
    }
}

// Premio depois de vencer: mesas faceis dao mais dados comuns; dificeis e
// chefes, mais raros. [tier] = facil, medio, dificil, chefe.
static const float PESO_PREMIO[4][4] = {
    { 6, 2.0f, 0.4f, 0.0f },
    { 3, 3.0f, 1.2f, 0.3f },
    { 1, 2.0f, 2.5f, 1.5f },
    { 0, 1.0f, 3.0f, 4.0f },
};

// Loja: caro de proposito, para a bolsa nao encher. Com as 50 moedas do
// comeco, da para um dado incomum ou um comum.
static int preco_run(int t)
{
    static const int P[4] = { 25, 40, 60, 90 };
    return P[TIPO[t].raridade];
}

static void monta_loja_run(void)
{
    rs = run.semente ^ (0x51ED27u * (uint32_t)(run.andar * 8 + run.passo + 1));
    rnd_r();
    float w[4] = { 3, 4, 2.0f * (1 + 0.3f * run.andar), 0.4f * (1 + 0.5f * run.andar) };
    sorteia_distintos(run.loja, N_LOJA, w);
    memset(run.levou, 0, sizeof run.levou);
    sorteia_amuletos(&run.am_loja, 1);
    run.am_levou = 0;
}

// ---------------------------------------------------------------------------
// O mapa de um salao
// ---------------------------------------------------------------------------
static bool existe(int p, int l) { return p >= 0 && p < DES_PASSOS && l >= 0 && l < 3 && run.no[p][l].tipo != NO_VAZIO; }

static void gera_andar(void)
{
    rs = run.semente ^ (0x9E3779B9u * (uint32_t)(run.andar + 1));
    rnd_r(); rnd_r();
    for (int p = 0; p < DES_PASSOS; p++)
        for (int l = 0; l < 3; l++) run.no[p][l].tipo = NO_VAZIO;
    uint32_t usados = 0;
    bool loja_feita = false;
    for (int p = 0; p < DES_PASSOS - 1; p++) {
        int ls[3], n;
        if (sorte_r(100) < 45) { n = 3; ls[0] = 0; ls[1] = 1; ls[2] = 2; }
        else {
            static const int PAR[3][2] = { { 0, 1 }, { 1, 2 }, { 0, 2 } };
            int q = sorte_r(3);
            n = 2; ls[0] = PAR[q][0]; ls[1] = PAR[q][1];
        }
        for (int i = 0; i < n; i++) {
            no_t *no = &run.no[p][ls[i]];
            int x = sorte_r(100);
            no->tipo = x < 34 ? NO_FACIL : (x < 70 ? NO_MEDIO : NO_DIFICIL);
        }
        // Uma loja por salao, no maximo, e nunca na primeira mesa.
        if (!loja_feita && p >= 1 && sorte_r(100) < (p == 1 ? 40 : 55)) {
            run.no[p][ls[sorte_r(n)]].tipo = NO_LOJA;
            loja_feita = true;
        }
        for (int i = 0; i < n; i++) {
            no_t *no = &run.no[p][ls[i]];
            no->semente = rnd_r() | 1;
            if (no->tipo == NO_LOJA) continue;
            int pe, tent = 0;
            // O Novato nao senta nas mesas dificeis.
            do pe = sorte_r(N_PERSO_COMUM);
            while (((usados >> pe & 1) || (pe == 0 && no->tipo == NO_DIFICIL)) && ++tent < 20);
            usados |= 1u << pe;
            no->perso = (uint8_t)pe;
            no->nivel = NIVEL_NO[run.andar][no->tipo];
        }
    }
    no_t *chefe = &run.no[DES_PASSOS - 1][1];
    chefe->tipo = NO_CHEFE;
    chefe->perso = (uint8_t)(N_PERSO_COMUM + run.andar);
    chefe->nivel = NIVEL_NO[run.andar][3];
    chefe->semente = rnd_r() | 1;

    // Ligacoes: cada mesa leva as do passo seguinte na mesma linha ou ao lado.
    // Toda mesa tem saida, e toda mesa do passo seguinte tem entrada.
    for (int p = 0; p < DES_PASSOS - 1; p++) {
        for (int l = 0; l < 3; l++) {
            if (!existe(p, l)) continue;
            uint8_t liga = 0;
            for (int m = 0; m < 3; m++)
                if (existe(p + 1, m) && abs(l - m) <= 1) liga |= (uint8_t)(1 << m);
            if (!liga) {                     // nenhuma ao lado: a mais proxima
                int melhor = -1;
                for (int m = 0; m < 3; m++)
                    if (existe(p + 1, m) && (melhor < 0 || abs(l - m) < abs(l - melhor))) melhor = m;
                if (melhor >= 0) liga = (uint8_t)(1 << melhor);
            }
            run.no[p][l].liga = liga;
        }
        for (int m = 0; m < 3; m++) {
            if (!existe(p + 1, m)) continue;
            bool entra = false;
            for (int l = 0; l < 3; l++) if (existe(p, l) && (run.no[p][l].liga >> m & 1)) entra = true;
            if (entra) continue;
            int melhor = -1;
            for (int l = 0; l < 3; l++)
                if (existe(p, l) && (melhor < 0 || abs(l - m) < abs(melhor - m))) melhor = l;
            if (melhor >= 0) run.no[p][melhor].liga |= (uint8_t)(1 << m);
        }
    }
    for (int p = 0; p < DES_PASSOS; p++) run.caminho[p] = -1;
}

// A mesa l do passo atual pode ser escolhida?
static bool escolhivel(int l)
{
    if (!existe(run.passo, l)) return false;
    if (run.passo == 0) return true;
    int de = run.caminho[run.passo - 1];
    return de >= 0 && (run.no[run.passo - 1][de].liga >> l & 1);
}

// ---------------------------------------------------------------------------
// Os oponentes: dados melhores e menos dados fracos a cada salao e dificuldade
// ---------------------------------------------------------------------------
static int monta_oponente(const no_t *no, int andar, peca_t *col)
{
    rs = no->semente;
    rnd_r();
    int tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
    // Os oito de saida, do mais forte ao mais fraco; os fracos saem primeiro.
    static const uint8_t INICIO[] = { D_D6, D_D6, D_D6, D_D6, D_D4, D_D4, D_D2, D_D2 };
    int tira = andar * 2 + (tier >= 2 ? 1 : 0);
    if (tira > 6) tira = 6;
    int n = 0;
    for (int i = 0; i < 8 - tira; i++) col[n++] = peca_r(INICIO[i]);
    int extras = 1 + andar * 2 + (tier == 3 ? 1 : tier);
    static const float BASE[4][4] = {
        { 6, 3, 1.0f, 0.0f }, { 3, 4, 2.0f, 0.2f }, { 1, 3, 4.0f, 0.6f }, { 1, 3, 3.5f, 1.0f } };
    float w[4] = { BASE[tier][0] / (1 + andar), BASE[tier][1],
                   BASE[tier][2] * (1 + 0.4f * andar), BASE[tier][3] * (1 + 0.8f * andar) };
    const perso_t *pe = &PERSO[no->perso];
    int n_fav = 0;
    while (n_fav < 5 && pe->fav[n_fav] != 0xFF) n_fav++;
    for (int i = 0; i < extras && n < MAX_COL; i++) {
        int t = (n_fav && sorte_r(100) < 45) ? pe->fav[sorte_r(n_fav)] : sorteia_raridade(w);
        col[n++] = peca_r(t);
    }
    // Recheio: dados simples (ou de efeito fraco) ate um tamanho minimo de
    // bolsa. Sem isso, um oponente de poucos dados poe os seus dois ou tres
    // dados bons na mao quase toda rodada.
    static const uint8_t RECHEIO[] = { D_D6, D_D8, D_D4, D_D8, D_QUEBRADO, D_TEIMOSO, D_PAR,
                                       D_TREVO, D_SORTE, D_SEMENTE, D_JACKPOT, D_COPIADOR,
                                       D_SENTINELA, D_COBRADOR, D_VIDRO, D_SOLITARIO };
    int alvo = 13 + andar * 2;
    while (n < alvo && n < MAX_COL) col[n++] = peca_r(RECHEIO[sorte_r((int)sizeof RECHEIO)]);
    return n;
}

// ---------------------------------------------------------------------------
// Salvamento: a estrutura da run em hexadecimal
// ---------------------------------------------------------------------------
static void des_salva(void)
{
    static char buf[2 * sizeof(run_t) + 1];
    const uint8_t *b = (const uint8_t *)&run;
    for (size_t i = 0; i < sizeof run; i++) {
        buf[2 * i]     = "0123456789abcdef"[b[i] >> 4];
        buf[2 * i + 1] = "0123456789abcdef"[b[i] & 15];
    }
    buf[2 * sizeof run] = 0;
    salva_grava(buf);
}

static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1); }

static bool des_carrega(run_t *r)
{
    static char buf[2 * sizeof(run_t) + 8];
    int n = salva_le(buf, (int)sizeof buf);
    // O arquivo de antes dos amuletos (mais curto) carrega com eles zerados.
    if (n != (int)(2 * sizeof(run_t)) && n != 2 * RUN_TAM_V1) return false;
    size_t tam = (size_t)n / 2;
    memset(r, 0, sizeof *r);
    uint8_t *b = (uint8_t *)r;
    for (size_t i = 0; i < tam; i++) {
        int a = hexv(buf[2 * i]), c = hexv(buf[2 * i + 1]);
        if (a < 0 || c < 0) return false;
        b[i] = (uint8_t)(a << 4 | c);
    }
    if (memcmp(r->marca, "DES1", 4) || r->andar >= DES_ANDARES || r->passo >= DES_PASSOS
        || r->estado > R_AMULETO || r->n_col < N_FILA || r->n_col > MAX_COL) return false;
    if (tam == RUN_TAM_V1) r->am_loja = 0xFF;            // loja de antes: sem amuleto
    for (int i = 0; i < r->n_col; i++) if (r->col[i].tipo >= D_N) return false;
    return true;
}

static bool des_tem_run(void)
{
    run_t r;
    return des_carrega(&r);
}

// "Salão Vermelho · mesa 2 · 14 dados · 120 moedas"
static void des_resumo(char *s, int n)
{
    run_t r;
    if (!des_carrega(&r)) { s[0] = 0; return; }
    if (r.estado == R_AMULETO && r.vitorias == 0) snprintf(s, (size_t)n, "escolhendo o primeiro amuleto");
    else if (r.estado == R_LOJA_INI) snprintf(s, (size_t)n, "na loja da entrada · %d moedas", r.moedas);
    else snprintf(s, (size_t)n, "%s · mesa %d de 4 · %d dados · %d moedas",
                  SALAO[r.andar], r.passo + 1, r.n_col, r.moedas);
}

// ---------------------------------------------------------------------------
// Andamento da run
// ---------------------------------------------------------------------------
static void abre_mapa(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    run.estado = R_MAPA;
    des_cur = -1;
    for (int l = 0; l < 3 && des_cur < 0; l++) if (escolhivel(l)) des_cur = l;
    if (des_cur < 0) des_cur = 1;
    t_fase = 0;
    fase = F_MAPA;
}

static void abre_loja_run(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    cursor = 0;
    t_fase = 0;
    fase = F_RLOJA;
}

static void encerra_run(bool venceu)
{
    des_venceu_run = venceu;
    salva_apaga();
    t_fase = 0;
    fase = F_RFIM;
}

// Passo vencido (ou loja deixada): segue no mapa; depois do chefe, sobe de
// salao; depois do terceiro chefe, a run esta ganha.
static void avanca_passo(void)
{
    run.passo++;
    if (run.passo >= DES_PASSOS) {
        run.andar++;
        if (run.andar >= DES_ANDARES) { run.andar = DES_ANDARES - 1; encerra_run(true); return; }
        run.passo = 0;
        gera_andar();
    }
    run.estado = R_MAPA;
    des_salva();
    abre_mapa();
}

static void abre_amuleto(void)
{
    modo = M_DESAFIO;
    cpu = remoto = 0;
    cursor = 0;
    t_fase = 0;
    fase = F_RAMULETO;
}

// Escolha de amuleto: volta = 0 vai para a loja da entrada; 1 segue no mapa.
static void oferece_amuleto(int volta)
{
    rs = run.semente ^ (0xA3E1u * (uint32_t)(run.andar * 8 + run.passo + 1 + volta * 64));
    rnd_r();
    sorteia_amuletos(run.am_oferta, 3);
    if (run.am_oferta[0] == 0xFF) {              // ja tem todos
        if (volta) avanca_passo();
        return;
    }
    run.am_volta = (uint8_t)volta;
    run.estado = R_AMULETO;
    des_salva();
    abre_amuleto();
}

static void amuleto_escolhido(int i)
{
    if (i >= 0 && i < 3 && run.am_oferta[i] != 0xFF) run.amuletos |= (uint16_t)(1u << run.am_oferta[i]);
    som_toca(SOM_PREMIO);
    solta_confete(60);
    if (run.am_volta == 0) {
        run.estado = R_LOJA_INI;
        monta_loja_run();
        run.am_loja = 0xFF;                      // a loja da entrada so vende dados
        des_salva();
        abre_loja_run();
    } else {
        avanca_passo();
    }
}

static void des_nova_run(void)
{
    memset(&run, 0, sizeof run);
    memcpy(run.marca, "DES1", 4);
    run.semente = ((uint32_t)esp_timer_get_time() * 2654435761u) ^ 0x5bd1e995u;
    if (!run.semente) run.semente = 1;
    run.moedas = DES_MOEDAS;
    static const uint8_t INICIO[] = { D_D6, D_D6, D_D6, D_D6, D_D4, D_D4, D_D2, D_D2 };
    for (unsigned i = 0; i < sizeof INICIO; i++) run.col[run.n_col++] = (peca_t){ INICIO[i], 0 };
    gera_andar();
    oferece_amuleto(0);                          // a run comeca com um amuleto
}

// Monta a mesa de uma partida da run e comeca a primeira rodada. A mesma mesa,
// recomecada (quem saiu no meio e voltou), sorteia do mesmo jeito.
static void des_comeca_partida(void)
{
    const no_t *no = &run.no[run.passo][run.caminho[run.passo]];
    run.estado = R_PARTIDA;
    des_salva();
    modo = M_DESAFIO;
    cpu = 2;
    remoto = 0;
    baixo = 0;
    semente = (no->semente * 2654435761u) | 1;
    for (int i = 0; i < 8; i++) rnd();
    som_rufo(false);
    memset(J, 0, sizeof J);
    memset(F, 0, sizeof F);
    J[0].nome = "VOCÊ";
    J[0].cor = C_LATAO;
    amul[0] = run.amuletos;
    amul[1] = 0;
    memset(am_brilho, 0, sizeof am_brilho);
    J[0].fichas = DES_FICHAS + (tem_am(0, A_RESERVA) ? 15 : 0);
    J[0].n_col = run.n_col;
    memcpy(J[0].col, run.col, sizeof run.col);
    J[1].nome = PERSO[no->perso].nome;
    J[1].cor = C_ROSA;
    J[1].fichas = DES_FICHAS;
    J[1].n_col = monta_oponente(no, run.andar, J[1].col);
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < J[j].n_col; i++) J[j].onde[i] = NA_BOLSA;
    nivel_jog[1] = no->nivel;
    ousadia_jog[1] = PERSO[no->perso].ousadia;
    rodada = 0;
    pote = 0;
    primeiro = (int)(rnd() & 1);
    venc_partida = -1;
    correu_venc = -1;
    n_voo = 0;
    t_sair = -1;
    detalhe = -1;
    nova_rodada();
}

// Fim de uma partida da run (chamado assim que ela acaba, para que sair na
// tela final nao deixe jogar de novo): vitoria vira moedas e premio; derrota
// ou empate encerram a run.
static void des_resultado(void)
{
    const no_t *no = &run.no[run.passo][run.caminho[run.passo]];
    // A bolsa volta para a run como terminou: o Quebrado e o que o Fogo
    // queimou nao voltam.
    run.n_col = (uint8_t)J[0].n_col;
    memcpy(run.col, J[0].col, sizeof run.col);
    if (venc_partida != 0) {
        des_perdeu_para = no->perso;
        des_perdeu_andar = run.andar;
        salva_apaga();
        return;
    }
    run.vitorias++;
    run.ganho = J[0].fichas;
    if (run.amuletos >> A_CONTRATO & 1) run.ganho += J[0].fichas / 2;     // Contrato Sujo: +50%
    run.juros = 0;
    if (run.amuletos >> A_COFRE & 1) {                                    // Cofre: 1 a cada 5
        int j = (run.moedas + run.ganho) / 5;
        run.juros = (uint8_t)(j > 15 ? 15 : j);
    }
    run.moedas += run.ganho + run.juros;
    run.tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
    rs = no->semente ^ 0xA11CEu;
    rnd_r();
    sorteia_distintos(run.oferta, 3, PESO_PREMIO[run.tier]);
    run.estado = R_PREMIO;
    des_salva();
}

static void abre_premio_run(void)
{
    modo = M_DESAFIO;
    cpu = 0;
    baixo = 0;
    J[0].nome = "VOCÊ";
    J[0].cor = C_LATAO;
    J[0].n_col = run.n_col;
    memcpy(J[0].col, run.col, sizeof run.col);
    memcpy(oferta, run.oferta, sizeof run.oferta);
    vez = 0;
    cursor = 0;
    removendo = false;
    t_tira = 0;
    t_fase = 0;
    fase = F_PREMIO;
    slot_comeca();
}

// ENTER na tela do fim da partida.
static void des_apos_fim(void)
{
    if (venc_partida == 0) abre_premio_run();
    else encerra_run(false);
}

// Premio escolhido, recusado ou trocado por tirar um dado.
static void des_premio_feito(void)
{
    run.n_col = (uint8_t)J[0].n_col;
    memcpy(run.col, J[0].col, sizeof run.col);
    // Chefe vencido (menos o ultimo, que fecha a run): mais um amuleto.
    if (run.tier == 3 && run.andar < DES_ANDARES - 1) oferece_amuleto(1);
    else avanca_passo();
}

static void des_continua(void)
{
    if (!des_carrega(&run)) { des_nova_run(); return; }
    switch (run.estado) {
    case R_LOJA_INI: case R_LOJA: abre_loja_run(); break;
    case R_PARTIDA:               des_comeca_partida(); break;
    case R_PREMIO:                abre_premio_run(); break;
    case R_AMULETO:               abre_amuleto(); break;
    default:                      abre_mapa(); break;
    }
}

// ---------------------------------------------------------------------------
// Desenho
// ---------------------------------------------------------------------------
// Moeda de ouro: nao e ficha. Disco dourado com borda escura e um brilho.
static void moeda_ic(int x, int y)
{
    gfx_disco(x + 1, y + 2, 7, C_SOMBRA);
    gfx_disco(x, y, 7, C_LATAO_ESC);
    gfx_disco(x, y, 6, C_OURO);
    gfx_disco(x, y, 4, gfx_mistura(C_OURO, C_LATAO_ESC, 30));
    gfx_disco(x - 2, y - 2, 1, gfx_mistura(C_OURO, C_BRANCO, 60));
}

static void moedas_c(int cx, int y, int valor, const char *sufixo, uint16_t c)
{
    char t[48];
    snprintf(t, sizeof t, "%d%s", valor, sufixo);
    int w = gfx_largura(t, 1) + 20, x = cx - w / 2;
    moeda_ic(x + 7, y + 11);
    txt(x + 20, y, t, c, true);
}

// Aro de pontinhos em volta de (x, y).
static void aro_circulo(int x, int y, int r, uint16_t c, int n)
{
    for (int i = 0; i < n; i++) {
        float a = i * 6.2832f / n;
        gfx_rect(x + (int)(cosf(a) * r) - 1, y + (int)(sinf(a) * r) - 1, 2, 2, c);
    }
}

static void pontilhado(int x0, int y0, int x1, int y1, uint16_t c, int esp, int passo)
{
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0);
    int n = (int)(sqrtf(dx * dx + dy * dy) / passo);
    for (int i = 0; i <= n; i++) {
        int x = x0 + (int)(dx * i / (n ? n : 1)), y = y0 + (int)(dy * i / (n ? n : 1));
        gfx_rect(x - esp / 2, y - esp / 2, esp, esp, c);
    }
}

static void faixa_run(void)
{
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(18, ty, "MODO DESAFIANTE", C_LATAO, true);
    char s[48];
    snprintf(s, sizeof s, "%s  ·  %d de %d", SALAO[run.andar], run.andar + 1, DES_ANDARES);
    txt_c(GFX_W / 2, ty, s, COR_SALAO[run.andar], true);
    snprintf(s, sizeof s, "%d", run.moedas);
    int x = GFX_W - 18 - gfx_largura(s, 1);
    txt(x, ty, s, C_OURO, true);
    moeda_ic(x - 12, FAIXA_H / 2);
    snprintf(s, sizeof s, "%d dados", run.n_col);
    txt(x - 34 - gfx_largura(s, 1), ty, s, C_MARFIM_S, false);
}

static void rodape_run(const char *s)
{
    int yb = GFX_H - FAIXA_H, ty = (FAIXA_H - TXT_H) / 2 - 2;
    gfx_rect(0, yb, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, yb, GFX_W, 2, C_LATAO_ESC);
    txt_c(GFX_W / 2, yb + ty + 2, s, C_TEXTO_M, false);
}

// ---- O mapa -------------------------------------------------------------
static const int MAPA_X[DES_PASSOS] = { 168, 290, 412, 540 };
static const int MAPA_Y[3] = { 96, 150, 204 };
#define MAPA_X0 70
enum { V_FEITO, V_ABERTO, V_LONGE, V_PERDIDO };

// Uma mesa do mapa como ficha de cassino: a cor diz a dificuldade, os pontos
// no meio tambem (1, 2 ou 3); a loja e uma moeda; o chefe, uma ficha maior
// com aro dourado.
static void mesa_no(int x, int y, int tipo, int vis, bool cur)
{
    int r = tipo == NO_CHEFE ? 20 : 15;
    uint16_t c = cor_no(tipo);
    int apaga = vis == V_PERDIDO ? 72 : (vis == V_LONGE ? 38 : (vis == V_FEITO ? 50 : 0));
    uint16_t base = gfx_mistura(c, C_FELTRO, apaga);
    uint16_t marca = gfx_mistura(C_MARFIM, C_FELTRO, apaga);
    if (cur) {
        int rr = r + 6 + (int)(sinf(t_fase * 6) * 1.5f);
        gfx_disco(x, y, rr, C_BRANCO);
        gfx_disco(x, y, rr - 2, C_FELTRO_ESC);
    } else if (vis == V_ABERTO) {
        aro_circulo(x, y, r + 5, gfx_mistura(c, C_FELTRO, 30), 32);
    }
    gfx_disco(x + 1, y + 3, r, C_SOMBRA);
    if (tipo == NO_LOJA) {                       // moeda grande
        gfx_disco(x, y, r, gfx_mistura(C_LATAO_ESC, C_FELTRO, apaga));
        gfx_disco(x, y, r - 2, base);
        gfx_disco(x, y, r - 6, gfx_mistura(base, C_LATAO_ESC, 30));
        gfx_disco(x - 4, y - 4, 2, gfx_mistura(base, C_BRANCO, 55));
        txt_c(x, y - 10, "$", gfx_mistura(C_LATAO_ESC, C_FELTRO, apaga), true);
    } else {
        gfx_disco(x, y, r, base);
        for (int i = 0; i < 8; i++) {                // listras da borda
            float a = i * 0.7854f + 0.39f;
            gfx_rect(x + (int)(cosf(a) * (r - 3)) - 1, y + (int)(sinf(a) * (r - 3)) - 1, 3, 3, marca);
        }
        gfx_disco(x, y, r - 6, gfx_mistura(base, C_BARRA, 30));
        int n = tipo == NO_FACIL ? 1 : (tipo == NO_MEDIO ? 2 : 3);
        for (int k = 0; k < n; k++)
            gfx_disco(x + (int)((k - (n - 1) / 2.0f) * 6), y, tipo == NO_CHEFE ? 2 : 1, marca);
        if (tipo == NO_CHEFE) aro_circulo(x, y, r + 3, gfx_mistura(C_OURO, C_FELTRO, apaga), 40);
    }
    if (vis == V_FEITO) aro_circulo(x, y, r + 3, C_OURO, 28);
}

static int vis_de(int p, int l)
{
    if (p < run.passo) return run.caminho[p] == l ? V_FEITO : V_PERDIDO;
    if (p == run.passo) return escolhivel(l) ? V_ABERTO : V_PERDIDO;
    return V_LONGE;
}

static const char *const NIVEL_IA[] = { "IA simples", "IA atenta", "IA afiada" };
static const char *const PREMIO_TIER[] = { "prêmio: dados comuns", "prêmio: dados variados",
                                           "prêmio: dados raros", "prêmio: os melhores dados" };

static void info_mesa(int l)
{
    if (!existe(run.passo, l)) return;
    const no_t *no = &run.no[run.passo][l];
    int y0 = 242, x = 40;
    painel(y0, 62, GFX_W - 56);
    if (no->tipo == NO_LOJA) {
        txt(x, y0 + 3, "LOJA", C_AMARELO, true);
        txt(x, y0 + 22, "Troque moedas de ouro por dados para a sua bolsa. Não é uma partida.", C_MARFIM_S, false);
        moedas_c(GFX_W / 2, y0 + 40, run.moedas, " moedas para gastar", C_OURO);
        return;
    }
    const perso_t *pe = &PERSO[no->perso];
    int tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
    int w = gfx_texto(x, y0 + 3, pe->nome, cor_no(no->tipo), 1, true);
    int xt = etiqueta_caixa(x + w + 10, y0 + 3, NOME_NO[no->tipo], cor_no(no->tipo));
    etiqueta_caixa(xt, y0 + 3, NIVEL_IA[no->nivel], C_ACO);
    const char *pr = PREMIO_TIER[tier];
    txt(GFX_W - 40 - gfx_largura(pr, 1), y0 + 3, pr, C_TEXTO_M, false);
    txt(x, y0 + 24, pe->frase, C_MARFIM_S, false);
    // A bolsa dele, miuda: da para ver contra o que vai jogar.
    peca_t col[MAX_COL];
    int n = monta_oponente(no, run.andar, col);
    for (int i = 0; i < n; i++)
        peca_parada((float)(x + 8 + i * 19), (float)(y0 + 52), 8, col[i], 1);
    char s[24];
    snprintf(s, sizeof s, "%d dados", n);
    gfx_texto(x + 8 + n * 19 + 2, y0 + 46, s, C_TEXTO_M, GFX_MIUDO, false);
}

static void desenha_mapa(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 50);
    faixa_run();
    rodape_run("setas escolhem  ·  ENTER senta à mesa  ·  TAB sua bolsa  ·  ESC menu (a run fica guardada)");

    for (int p = 0; p < DES_PASSOS; p++) {
        char s[24];
        if (p < DES_PASSOS - 1) snprintf(s, sizeof s, "mesa %d", p + 1);
        else snprintf(s, sizeof s, "chefe");
        uint16_t c = p == run.passo ? COR_SALAO[run.andar] : (p < run.passo ? C_FELTRO2 : C_TEXTO_M);
        txt_c(MAPA_X[p], 60, s, c, p == run.passo);
    }
    // Entrada do salao e as ligacoes.
    int ey = MAPA_Y[1];
    for (int l = 0; l < 3; l++) {
        if (!existe(0, l)) continue;
        bool feito = run.passo > 0 && run.caminho[0] == l;
        uint16_t c = feito ? C_OURO : (run.passo == 0 ? C_TEXTO_M : C_FELTRO2);
        pontilhado(MAPA_X0, ey, MAPA_X[0], MAPA_Y[l], c, feito ? 3 : 2, 6);
    }
    for (int p = 0; p < DES_PASSOS - 1; p++)
        for (int l = 0; l < 3; l++) {
            if (!existe(p, l)) continue;
            for (int m = 0; m < 3; m++) {
                if (!(run.no[p][l].liga >> m & 1) || !existe(p + 1, m)) continue;
                bool feito = p + 1 < run.passo && run.caminho[p] == l && run.caminho[p + 1] == m;
                bool aberto = p + 1 == run.passo && run.caminho[p] == l;
                uint16_t c = feito ? C_OURO : (aberto ? C_TEXTO_M : C_FELTRO2);
                pontilhado(MAPA_X[p], MAPA_Y[l], MAPA_X[p + 1], MAPA_Y[m], c, feito ? 3 : 2, 6);
            }
        }
    gfx_disco(MAPA_X0, ey, 6, C_LATAO_ESC);
    gfx_disco(MAPA_X0, ey, 4, C_LATAO);
    gfx_texto(MAPA_X0 - 16, ey + 10, "entrada", C_TEXTO_M, GFX_MIUDO, false);

    for (int p = 0; p < DES_PASSOS; p++)
        for (int l = 0; l < 3; l++) {
            if (!existe(p, l)) continue;
            const no_t *no = &run.no[p][l];
            int vis = vis_de(p, l);
            bool cur = p == run.passo && l == des_cur;
            mesa_no(MAPA_X[p], MAPA_Y[l], no->tipo, vis, cur);
            int r = no->tipo == NO_CHEFE ? 20 : 15;
            uint16_t c = vis == V_ABERTO ? cor_no(no->tipo) : gfx_mistura(cor_no(no->tipo), C_FELTRO, 55);
            const char *rot = NOME_NO[no->tipo];
            gfx_texto(MAPA_X[p] - gfx_largura(rot, GFX_MIUDO) / 2, MAPA_Y[l] + r + 5, rot, c, GFX_MIUDO, false);
            if (vis == V_ABERTO) zona_clique(MAPA_X[p] - r - 6, MAPA_Y[l] - r - 6, 2 * r + 12, 2 * r + 12,
                                             &des_cur, l, KEY_ENTER);
        }
    info_mesa(des_cur);
    botao_clique(GFX_W - 40, 242 + 38, "sua bolsa", KEY_TAB);
    // Os amuletos da run, no canto de cima.
    for (int a = 0, n = 0; a < A_N; a++)
        if (run.amuletos >> a & 1) { desenha_amuleto(30 + (n % 4) * 19, 66 + (n / 4) * 19, 8, a, false); n++; }
}

// ---- Loja ---------------------------------------------------------------
static void desenha_loja_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 90);
    faixa_run();
    bool ini = run.estado == R_LOJA_INI;
    txt_c2(GFX_W / 2, MESA_Y0 + 2, ini ? "LOJA DA ENTRADA" : "LOJA DO SALÃO", C_OURO);
    txt_c(GFX_W / 2, MESA_Y0 + 44, ini ? "A run começa: gaste bem, a próxima loja pode demorar."
                                        : "Troque moedas de ouro por dados para a sua bolsa.",
          C_MARFIM_S, false);
    moedas_c(GFX_W / 2, MESA_Y0 + 66, run.moedas, " moedas", C_OURO);
    bool tem_am_loja = run.am_loja < A_N;
    int nc = N_LOJA + (tem_am_loja ? 1 : 0);
    for (int i = 0; i < nc; i++) {
        int x = GFX_W / 2 + (int)((i - (nc - 1) / 2.0f) * (nc > N_LOJA ? 96 : 108)), yb = 142;
        int wc = nc > N_LOJA ? 88 : 96;
        bool am = i == N_LOJA;
        int pr = am ? AM_PRECO : preco_run(run.loja[i].tipo);
        bool ja = am ? run.am_levou : run.levou[i], cur = i == cursor, da = run.moedas >= pr;
        uint16_t cr = am ? C_OURO : cor_rar(run.loja[i].tipo);
        gfx_rect(x - wc / 2, yb, wc, 92, cur ? C_FELTRO_ESC : C_FELTRO);
        gfx_moldura(x - wc / 2, yb, wc, 92, cur ? 3 : 1, cur ? cr : gfx_mistura(C_FELTRO, cr, 55));
        float bob = cur && !ja ? sinf(t_fase * 4) * 3 : 0;
        if (am) desenha_amuleto(x, yb + 32 + (int)bob, 20, run.am_loja, false);
        else peca_parada((float)x, yb + 32 + bob, 22, run.loja[i], 0);
        if (ja) {
            txt_c(x, yb + 64, "comprado", C_TEXTO_M, false);
            gfx_rect_alfa(x - wc / 2 + 1, yb + 1, wc - 2, 90, C_SOMBRA, 120);
        } else {
            moedas_c(x, yb + 64, pr, "", da ? C_OURO : C_VINHO_CLR);
        }
        zona_clique(x - wc / 2, yb, wc, 92, &cursor, i, KEY_ENTER);
    }
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "sair da loja", KEY_TAB);
    if (cursor == N_LOJA && tem_am_loja) {
        const amuleto_t *a = &AMULETO[run.am_loja];
        txt_c(GFX_W / 2, 240, a->nome, a->cor, true);
        txt_c(GFX_W / 2, 240 + TXT_H + 2, a->desc, C_MARFIM_S, false);
        if (!run.am_levou && run.moedas < AM_PRECO) txt_c(GFX_W / 2, 284, "moedas insuficientes", C_VINHO_CLR, true);
    } else if (cursor < N_LOJA) {
        dado_e_poder(240, run.loja[cursor].tipo, true);
        if (!run.levou[cursor] && run.moedas < preco_run(run.loja[cursor].tipo))
            txt_c(GFX_W / 2, 284, "moedas insuficientes", C_VINHO_CLR, true);
    }
    rodape_run("setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB sai da loja");
}

// ---- A bolsa ------------------------------------------------------------
#define GRADE_B 12
static void desenha_bolsa_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run();
    txt_c2(GFX_W / 2, MESA_Y0 + 2, "SUA BOLSA", C_LATAO);
    char s[64];
    snprintf(s, sizeof s, "%d dados: eles vão com você de mesa em mesa", run.n_col);
    txt_c(GFX_W / 2, MESA_Y0 + 44, s, C_MARFIM_S, false);
    for (int i = 0; i < run.n_col; i++) {
        int lin = i / GRADE_B, col = i % GRADE_B;
        int na_linha = (run.n_col - lin * GRADE_B) < GRADE_B ? run.n_col - lin * GRADE_B : GRADE_B;
        int x = GFX_W / 2 + (int)((col - (na_linha - 1) / 2.0f) * 44);
        int y = 138 + lin * 40;
        if (i == cursor) gfx_rect_alfa(x - 20, y - 20, 40, 40, C_LATAO, 70);
        peca_parada((float)x, (float)y, 15, run.col[i], 0);
        if (i != cursor) marca_rar(x, y + 20, run.col[i].tipo);
        zona_clique(x - 20, y - 20, 40, 40, &cursor, i, 'i');   // clique abre os detalhes
    }
    botao_clique(MESA_X1 - 10, MESA_Y0 + 8, "voltar ao mapa", KEY_TAB);
    // Os amuletos da run, nas laterais: seis de cada lado.
    int na = 0;
    for (int a = 0; a < A_N; a++) {
        if (!(run.amuletos >> a & 1)) continue;
        int x = na < 6 ? 34 : GFX_W - 34, y = 128 + (na % 6) * 26;
        int idx = run.n_col + na;
        if (idx == cursor) gfx_disco(x, y, 14, C_LATAO);
        desenha_amuleto(x, y, 11, a, false);
        zona_clique(x - 12, y - 12, 24, 24, &cursor, idx, 0);
        if (idx == cursor) {
            txt_c(GFX_W / 2, 262, AMULETO[a].nome, AMULETO[a].cor, true);
            txt_c(GFX_W / 2, 262 + TXT_H + 2, AMULETO[a].desc, C_MARFIM_S, false);
        }
        na++;
    }
    if (run.n_col && cursor < run.n_col) dado_e_poder(262, run.col[cursor].tipo, true);
    rodape_run("setas escolhem  ·  I detalhes  ·  TAB volta ao mapa");
}

// ---- Fim da run ---------------------------------------------------------
static void desenha_fim_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run();
    painel(CENTRO - 84, 168, 500);
    char s[96];
    if (des_venceu_run) {
        txt_sombra_c(GFX_W / 2, CENTRO - 74, "VITÓRIA", C_OURO, 3);
        txt_c(GFX_W / 2, CENTRO - 6, "Você venceu O BARÃO e fechou o cassino.", C_MARFIM, true);
    } else {
        txt_sombra_c(GFX_W / 2, CENTRO - 74, "FIM DA RUN", C_VINHO_CLR, 3);
        if (des_perdeu_para >= 0)
            snprintf(s, sizeof s, "%s venceu você no %s.", PERSO[des_perdeu_para].nome, SALAO[des_perdeu_andar]);
        else snprintf(s, sizeof s, "A casa ficou com a mesa.");
        txt_c(GFX_W / 2, CENTRO - 6, s, C_MARFIM, true);
    }
    snprintf(s, sizeof s, "%d %s  ·  %d dados na bolsa  ·  %d moedas", run.vitorias,
             run.vitorias == 1 ? "partida vencida" : "partidas vencidas", run.n_col, run.moedas);
    txt_c(GFX_W / 2, CENTRO + 22, s, C_TEXTO_M, false);
    txt_c(GFX_W / 2, CENTRO + 56, "ENTER volta ao menu", C_MARFIM_S, false);
}

// Tela do fim de uma partida da run (por cima da mesa, como a normal).
static void desenha_fim_partida_run(void)
{
    painel(CENTRO - 70, 150, 460);
    char s[80];
    if (venc_partida == 0) {
        txt_sombra_c(GFX_W / 2, CENTRO - 58, "VOCÊ VENCEU", C_LATAO, 3);
        if (run.ganho != J[0].fichas)
            snprintf(s, sizeof s, "%d fichas viram %d moedas (Contrato +50%%)", J[0].fichas, run.ganho);
        else snprintf(s, sizeof s, "%d fichas viram %d moedas de ouro", run.ganho, run.ganho);
        if (run.juros) {
            char j2[24];
            snprintf(j2, sizeof j2, "  ·  Cofre +%d", run.juros);
            strncat(s, j2, sizeof s - strlen(s) - 1);
        }
        moedas_c(GFX_W / 2, CENTRO + 6, run.ganho + run.juros, " moedas", C_OURO);
        txt_c(GFX_W / 2, CENTRO + 32, s, C_TEXTO_M, false);
        txt_c(GFX_W / 2, CENTRO + 56, "ENTER escolhe o prêmio", C_MARFIM_S, false);
    } else {
        txt_sombra_c(GFX_W / 2, CENTRO - 58, venc_partida == 2 ? "EMPATE" : "DERROTA", C_VINHO_CLR, 3);
        txt_c(GFX_W / 2, CENTRO + 6, venc_partida == 2 ? "Empate não passa de mesa: a run termina aqui."
                                                       : "A run termina aqui.", C_MARFIM, true);
        snprintf(s, sizeof s, "%s %d  x  %d %s", J[0].nome, J[0].fichas, J[1].fichas, J[1].nome);
        txt_c(GFX_W / 2, CENTRO + 32, s, C_TEXTO_M, false);
        txt_c(GFX_W / 2, CENTRO + 56, "ENTER segue", C_MARFIM_S, false);
    }
}

// ---- Escolha de amuleto ---------------------------------------------------
static void desenha_amuleto_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run();
    txt_c2(GFX_W / 2, MESA_Y0 + 2, run.am_volta ? "O CHEFE DEIXOU UM AMULETO" : "ESCOLHA UM AMULETO", C_OURO);
    txt_c(GFX_W / 2, MESA_Y0 + 44, "Ele vale a run inteira.", C_MARFIM_S, false);
    for (int i = 0; i < 3; i++) {
        int a = run.am_oferta[i];
        if (a >= A_N) continue;
        int x = GFX_W / 2 + (i - 1) * 150;
        bool cur = i == cursor;
        gfx_rect(x - 64, 124, 128, 100, cur ? C_FELTRO_ESC : C_FELTRO);
        gfx_moldura(x - 64, 124, 128, 100, cur ? 3 : 1, cur ? C_OURO : gfx_mistura(C_FELTRO, C_OURO, 55));
        float bob = cur ? sinf(t_fase * 4) * 3 : 0;
        desenha_amuleto(x, 162 + (int)bob, 26, a, false);
        txt_c(x, 198, AMULETO[a].nome, cur ? AMULETO[a].cor : C_MARFIM_S, cur);
        zona_clique(x - 64, 124, 128, 100, &cursor, i, KEY_ENTER);
    }
    if (cursor >= 0 && cursor < 3 && run.am_oferta[cursor] < A_N)
        txt_c(GFX_W / 2, 234, AMULETO[run.am_oferta[cursor]].desc, C_MARFIM, false);
    // Os que ja tem, miudos, embaixo.
    int na = am_conta(run.amuletos);
    if (na) {
        int x0 = GFX_W / 2 - (na - 1) * 10, n = 0;
        txt_c(GFX_W / 2, 258, "seus amuletos", C_TEXTO_M, false);
        for (int a = 0; a < A_N; a++)
            if (run.amuletos >> a & 1) desenha_amuleto(x0 + 20 * n++, 288, 8, a, false);
    }
    rodape_run("setas escolhem  ·  ENTER leva o amuleto");
}

static bool des_desenha(void)
{
    switch (fase) {
    case F_MAPA:   desenha_mapa();      return true;
    case F_RLOJA:  desenha_loja_run();  return true;
    case F_RBOLSA: desenha_bolsa_run(); return true;
    case F_RFIM:   desenha_fim_run();   return true;
    case F_RAMULETO: desenha_amuleto_run(); return true;
    default:       return false;
    }
}

// O dado em foco nas telas da run (tecla I).
static int des_foco(void)
{
    if (fase == F_RLOJA && cursor >= 0 && cursor < N_LOJA) return run.loja[cursor].tipo;
    if (fase == F_RBOLSA && cursor >= 0 && cursor < run.n_col) return run.col[cursor].tipo;
    return -1;
}

// ---------------------------------------------------------------------------
// Teclas
// ---------------------------------------------------------------------------
static bool des_tecla(int k, int e)
{
    switch (fase) {
    case F_MAPA:
        if (e) {
            for (int i = 1; i <= 3; i++) {
                int l = ((des_cur + e * i) % 3 + 3) % 3;
                if (escolhivel(l)) { des_cur = l; som_toca(SOM_TIQUE); break; }
            }
            return true;
        }
        if (k == KEY_ENTER && escolhivel(des_cur)) {
            som_toca(SOM_FICHA);
            run.caminho[run.passo] = (int8_t)des_cur;
            if (run.no[run.passo][des_cur].tipo == NO_LOJA) {
                run.estado = R_LOJA;
                monta_loja_run();
                des_salva();
                abre_loja_run();
            } else {
                des_comeca_partida();
            }
            return true;
        }
        if (k == KEY_TAB) { cursor = 0; fase = F_RBOLSA; }
        return true;

    case F_RLOJA: {
        int nc = N_LOJA + (run.am_loja < A_N ? 1 : 0);
        if (e) { cursor = (cursor + e + nc) % nc; som_toca(SOM_TIQUE); return true; }
        if (k >= '1' && k < '1' + nc) cursor = k - '1';
        if ((k == KEY_ENTER || (k >= '1' && k < '1' + nc)) && cursor == N_LOJA) {
            if (run.am_levou || run.am_loja >= A_N || run.moedas < AM_PRECO) return true;
            run.moedas -= AM_PRECO;
            run.amuletos |= (uint16_t)(1u << run.am_loja);
            run.am_levou = 1;
            som_toca(SOM_FICHA);
            des_salva();
            return true;
        }
        if (k == KEY_ENTER || (k >= '1' && k <= '5')) {
            int tp = run.loja[cursor].tipo, pr = preco_run(tp);
            if (run.levou[cursor] || run.moedas < pr || run.n_col >= MAX_COL) return true;
            run.moedas -= pr;
            run.col[run.n_col++] = run.loja[cursor];
            run.levou[cursor] = 1;
            som_toca(SOM_FICHA);
            des_salva();
            return true;
        }
        if (k == KEY_TAB || k == 'x') {
            if (run.estado == R_LOJA_INI) { run.estado = R_MAPA; des_salva(); abre_mapa(); }
            else avanca_passo();
        }
        return true;
    }

    case F_RAMULETO: {
        if (e) {
            for (int i = 1; i <= 3; i++) {
                int c = ((cursor + e * i) % 3 + 3) % 3;
                if (run.am_oferta[c] < A_N) { cursor = c; som_toca(SOM_TIQUE); break; }
            }
            return true;
        }
        if (k >= '1' && k <= '3' && run.am_oferta[k - '1'] < A_N) cursor = k - '1';
        if (k == KEY_ENTER || (k >= '1' && k <= '3')) amuleto_escolhido(cursor);
        return true;
    }

    case F_RBOLSA:
        if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= GRADE_B) cursor -= GRADE_B; return true; }
        if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + GRADE_B < run.n_col) cursor += GRADE_B; return true; }
        if (e && run.n_col) {
            int tot = run.n_col + am_conta(run.amuletos);
            cursor = (cursor + e + tot) % tot;
            som_toca(SOM_TIQUE);
            return true;
        }
        if (k == KEY_TAB || k == KEY_ENTER || k == KEY_BKSP || k == 'x') { t_fase = 0; fase = F_MAPA; }
        return true;

    case F_RFIM:
        if (k == KEY_ENTER) volta_ao_menu();
        return true;

    default:
        return false;
    }
}
