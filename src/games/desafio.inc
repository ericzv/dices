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

// Os oponentes do cassino. Cada um tem um jeito de apostar, dados que
// prefere, um trunfo (regra so dele, que vale a partida toda) e o que diz.
typedef struct {
    const char *nome, *frase;
    float ousadia;                       // -1 cauteloso ... +1 aposta alto
    uint8_t fav[5];                      // dados preferidos (0xFF = fim)
    const char *trunfo, *regra;          // nome do trunfo e o que ele faz
    uint16_t amul;                       // trunfo feito de amuletos
    uint8_t reg;                         // ou regra propria (REG_*)
    const char *fala[FA_N][2];
} perso_t;

#define AM(a) (uint16_t)(1u << (a))
static const perso_t PERSO[] = {
    { "O NOVATO",        "Chegou hoje ao cassino.",           -0.3f,
      { D_D8, D_D12, D_TEIMOSO, D_QUEBRADO, D_SENTINELA },
      "Nervoso", "Depois de perder uma rodada, compra só 6 dados na seguinte.", 0, REG_NERVOSO,
      { { "Primeira vez aqui... pega leve?", "Eu li as regras ontem!" },
        { "Ih, essa doeu.", "Ufa, não fui eu." },
        { "Sério? Ganhei?", "Sorte de principiante!" },
        { "Tá, tá, eu aprendo.", "Calma, calma..." },
        { "Eu ganhei de verdade?", "Vou contar pra todo mundo!" },
        { "Foi justo. Boa partida.", "Volto amanhã, mais esperto." } } },
    { "A CAUTELOSA",     "Aposta pouco e não arrisca.",       -1.0f,
      { D_ESCUDO, D_TEIMOSO, D_LASTRO, D_SENTINELA, D_ACUMULADOR },
      "Pé atrás", "O primeiro dado da sequência dela não cai.", AM(A_LUVA), REG_NADA,
      { { "Devagar se vai longe.", "Sem pressa, querido." },
        { "Pressa demais dá nisso.", "Eu avisei." },
        { "Passo a passo.", "Sem sustos." },
        { "Hm. Anotado.", "Um passo atrás, só isso." },
        { "Paciência sempre paga.", "Nada como a prudência." },
        { "Você arriscou e deu certo.", "Desta vez, a sorte foi sua." } } },
    { "O APOSTADOR",     "Dobra tudo, sempre.",                1.0f,
      { D_TUDO_NADA, D_EXPLOSIVO, D_D20, D_FICHAS, D_VENTANIA },
      "Pote quente", "Vencendo uma rodada, leva 3 fichas a mais.", AM(A_POTE), REG_NADA,
      { { "Dobra ou nada, parceiro!", "Hoje eu tô quente!" },
        { "Hahaha! Desabou!", "Faltou coragem!" },
        { "Pote na mão!", "Mais uma! Mais uma!" },
        { "Tô só esquentando.", "Ainda tenho ficha!" },
        { "A mesa é minha!", "Quem não arrisca, não petisca!" },
        { "Amanhã eu recupero tudo.", "Foi só um azar." } } },
    { "O LADRÃO",        "O que é seu pode ser dele.",         0.3f,
      { D_GATUNO, D_CARIDOSO, D_PIRATA, D_ESPELHO, D_COBRADOR },
      "Batedor de carteira", "Vencendo uma rodada, ainda leva 2 fichas suas.", 0, REG_BATEDOR,
      { { "Bela bolsa. Seria uma pena...", "Relaxa, eu não mordo." },
        { "Ops. Escorregou?", "Essa eu nem precisei roubar." },
        { "Obrigado pelas fichas.", "Ninguém viu nada." },
        { "Hm, você é atento.", "Da próxima vez..." },
        { "Leve, como sempre.", "Foi um prazer fazer negócio." },
        { "Você vigiou bem a bolsa.", "Essa escapou." } } },
    { "A MATEMÁTICA",    "Conta cada multiplicador.",          0.0f,
      { D_DOBRO, D_FARTURA, D_COPIADOR, D_FERREIRO, D_ACUMULADOR },
      "Conta certa", "Dado dela que tirar o número máximo ganha +2.", AM(A_OURO), REG_NADA,
      { { "As chances estão comigo.", "Já fiz as contas." },
        { "Era 62% de chance de cair.", "Previsível." },
        { "Exatamente como calculei.", "Estatística pura." },
        { "Um desvio aceitável.", "Margem de erro." },
        { "Os números não mentem.", "Como queríamos demonstrar." },
        { "Preciso refazer o modelo.", "Variância. Só isso." } } },
    { "O ESPINHENTO",    "Cada ponto seu custa caro.",         0.4f,
      { D_MARTELO, D_MALDICAO, D_CARRASCO, D_INVEJOSO, D_LASER },
      "Casca grossa", "Os seus ataques não anulam os dados dele.", AM(A_COURACA), REG_NADA,
      { { "Vai doer.", "Cada ponto vai custar caro." },
        { "Isso! Desmorona!", "Gostei de ver." },
        { "Sentiu o espinho?", "Mais um arranhão." },
        { "Grrr.", "Você vai pagar por essa." },
        { "Sai daqui sangrando.", "Ninguém passa por mim." },
        { "...Respeito.", "Da próxima, eu te pego." } } },
    { "A SUPERSTICIOSA", "Confia no trevo e na moeda.",        0.2f,
      { D_TREVO, D_SORTE, D_SEMENTE, D_JACKPOT, D_MISERICORDIOSO },
      "Mão de sorte", "Os dados dela de 2 e 4 lados valem +1.", AM(A_PENA), REG_NADA,
      { { "Toquei no trevo três vezes.", "As cartas disseram: hoje é meu dia." },
        { "Viu? Mau agouro.", "A lua não está a seu favor." },
        { "O trevo nunca falha.", "Bati na madeira!" },
        { "Gato preto, certeza.", "Esqueci de cruzar os dedos." },
        { "Os astros me guiaram.", "Sorte é tudo, meu bem." },
        { "Deixei meu amuleto em casa.", "Mercúrio retrógrado..." } } },
    { "O BRUTAMONTES",   "Dados grandes, pouca conversa.",     0.5f,
      { D_D20, D_LASTRO, D_EGOISTA, D_EXPLOSIVO, D_DESAFIANTE },
      "Mão grande", "Compra 8 dados por rodada.", AM(A_BOLSA), REG_NADA,
      { { "Dado grande. Ponto grande.", "Hmpf." },
        { "Fraco.", "Ha." },
        { "Esmaguei.", "Mais." },
        { "Hmpf.", "Isso não conta." },
        { "Fácil.", "Próximo." },
        { "...Forte, você.", "Vou treinar mais." } } },
    // Chefes, um por salao.
    { "O CRUPIÊ",        "Conhece cada dado desta mesa.",      0.0f,
      { D_VICIADO, D_LASTRO, D_DOBRO, D_ESCUDO, D_CARRASCO },
      "A casa sempre ganha", "Rodada empatada: o pote fica com ele.", 0, REG_CASA,
      { { "Façam suas apostas.", "A casa lhe dá as boas-vindas." },
        { "A casa agradece.", "Que pena, senhor." },
        { "A casa recolhe.", "Como de costume." },
        { "A casa paga.", "Bem jogado." },
        { "A casa sempre ganha.", "Volte sempre." },
        { "A banca se rende. Por hoje.", "Impressionante. Siga em frente." } } },
    { "A DONA DO SALÃO", "Nunca perdeu no próprio salão.",     0.4f,
      { D_ESPELHO, D_INVEJOSO, D_FOGO, D_COPIADOR, D_EGOISTA },
      "Dona da casa", "Começa com 15 fichas a mais, e você compra só 6 dados por rodada.",
      AM(A_RESERVA) | AM(A_FURADO), REG_NADA,
      { { "Você está na minha casa.", "Que gracinha, um desafiante." },
        { "Que deselegante.", "Meu salão não perdoa." },
        { "Obrigada pela visita.", "Tudo aqui é meu." },
        { "Ora, ora.", "Não se acostume." },
        { "Os seguranças levam você até a porta.", "Ninguém me vence aqui." },
        { "Você tem talento. Suba.", "O Barão vai gostar de você." } } },
    { "O BARÃO",         "O maior apostador do cassino.",      0.8f,
      { D_TUDO_NADA, D_MALDITO, D_EXPLOSIVO, D_FOGO, D_JACKPOT },
      "Intocável", "Os dados dele não são roubados nem anulados por ataques, e o último da sequência cai sozinho.",
      AM(A_IMA) | AM(A_COURACA) | AM(A_REDE), REG_NADA,
      { { "Então você chegou até mim.", "Sente-se. Isso será breve." },
        { "Lamentável.", "Esperava mais." },
        { "Naturalmente.", "Como previsto." },
        { "Interessante...", "Hm. Sorte." },
        { "O cassino continua meu.", "Volte quando for digno." },
        { "Impossível... o cassino é seu.", "Bravo. Você mereceu." } } },
};
#define N_PERSO_COMUM 8

// ---------------------------------------------------------------------------
// Falas: um balao curto saindo do retrato do oponente
// ---------------------------------------------------------------------------

static void fala_evento(int cat)
{
    if (modo != M_DESAFIO || fala_perso < 0 || cat < 0 || cat >= FA_N) return;
    // Nem tudo tem resposta: o comeco e o fim sempre; o resto, as vezes.
    bool sempre = cat == FA_INICIO || cat == FA_GANHA_P || cat == FA_PERDE_P;
    if (!sempre && (rnd_v() % 100) >= 55) return;
    if (!sempre && t_fala > 0.6f) return;               // nao atropela a fala anterior
    const char *s = PERSO[fala_perso].fala[cat][rnd_v() & 1];
    snprintf(fala_txt, sizeof fala_txt, "%s", s);
    t_fala = 3.2f;
}

// ---------------------------------------------------------------------------
// Retratos: busto em 20x22 pixels, com contorno escuro e duas tonalidades,
// ampliado sem filtro. Movimento lento: respira, pisca e fala.
// ---------------------------------------------------------------------------
#define RT_W 22
#define RETRATO_AVATAR 11                // os avatares vem depois dos 11 oponentes no desenho
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
        rosto(hy, P_CLARA, PRETO, 0, pisca, boca);
        rr(5, hy, 10, 2, PRETO); rr(5, hy + 2, 1, 2, PRETO); rr(14, hy + 2, 1, 2, PRETO);
        rr(4, hy + 1, 12, 1, RGB(60, 150, 100));                // viseira
        rr(5, hy + 2, 10, 1, gfx_mistura(RGB(60, 150, 100), C_FELTRO, 30));
        if (!boca) rr(8, hy + 9, 4, 1, PRETO);                  // bigode fino
        break;
    }
    case 9: {                                                   // DONA DO SALAO: coque com grampo, vestido, colar
        tronco(RGB(150, 30, 46), P_CLARA, 0, 0, 2);
        rr(7, 17, 6, 1, C_OURO); rp(10, 18, C_OURO);
        rosto(hy, P_CLARA, PRETO, 2, pisca, boca);
        rr(5, hy - 1, 10, 3, PRETO); rr(4, hy + 1, 2, 5, PRETO); rr(14, hy + 1, 2, 5, PRETO);
        rr(7, hy - 4, 6, 3, PRETO); rp(12, hy - 4, C_OURO); rp(13, hy - 5, C_OURO);   // coque e grampo
        if (!boca) rr(9, hy + 9, 2, 1, RGB(200, 40, 60));       // batom
        rp(4, hy + 7, C_OURO); rp(15, hy + 7, C_OURO);
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
    default: {                                                  // BARAO: cartola, monoculo, bigodao branco
        tronco(RGB(30, 30, 34), P_CLARA, RGB(236, 232, 222), RGB(30, 30, 34), 1);
        rr(13, 19, 3, 1, C_OURO);                               // corrente do relogio
        rosto(hy, P_CLARA, GRISALHO, 0, pisca, boca);
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
    tapa_textos(bx, by, w, h);
    gfx_rect(bx + 2, by + 2, w, h, C_SOMBRA);
    gfx_rect(bx, by, w, h, C_MARFIM);
    gfx_moldura(bx, by, w, h, 1, C_LATAO_ESC);
    for (int i = 0; i < 5; i++) gfx_rect(x + i, y - (5 - i) / 2, 7 - i, 5 - i, C_MARFIM);   // bico
    txt(bx + 8, by + 3, s, C_EBANO, false);
}

// Na faixa do oponente: o retrato (cortado no peito) e, se ele falou, o
// balao logo abaixo. Devolve onde o nome comeca.
static int avatar_retrato(void);
static int retrato_faixa(int j, int y0)
{
    int perso = j == 1 ? fala_perso : avatar_retrato();
    if (perso < 0) return 18;
    int x = 6, esc = 2, corte = 3;              // em cima so tem a ponta da cartola
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
    // Desde as dificuldades (um arquivo antigo carrega com tudo zerado):
    uint8_t  dificuldade;                // 0 normal .. 5 diamante
    uint16_t amuletos_hi;                // amuletos 16 em diante
    uint8_t  taca;                       // moedas que a Taca rendeu na ultima vitoria
} run_t;
#define RUN_TAM_V2 252                   // o run_t antes das dificuldades
#define RUN_TAM_V1 244                   // o run_t antes dos amuletos

static run_t run;
static int   des_cur;                    // linha do cursor no mapa

static bool  des_venceu_run;             // tela final: venceu o Barao?
static int   des_liberou = -1;           // tela final: dificuldade que acabou de abrir
static int   des_dif;                    // dificuldade escolhida para a proxima run
static int   op_amul_extra = -1;         // Diamante: o amuleto que o oponente ganhou
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
    [A_FERRADURA]= { "Ferradura",     "Na primeira queda de cada rodada, nenhum dos dois dados cai.", C_PRATA },
    [A_PESADO]   = { "Dado Pesado",   "Seus dados de 6 lados valem +1.", RGB(120, 128, 140) },
    [A_RELOGIO]  = { "Relógio de Bolso", "O oponente sempre abre a rodada; você joga depois.", RGB(214, 186, 110) },
    [A_CARTOLA]  = { "Cartola",       "O prêmio de cada mesa vencida sobe um nível.", RGB(150, 110, 170) },
    [A_COROA]    = { "Coroa do Azarão", "Começando a rodada com menos fichas que o oponente, seus dados valem +1.", RGB(226, 176, 70) },
    [A_CUPOM]    = { "Cupom",         "Tudo na loja custa 25% menos.", RGB(120, 190, 150) },
    [A_PRENSA]   = { "Prensa",        "Seu dado que tirar 1 passa a valer 3.", RGB(150, 150, 160) },
    [A_TACA]     = { "Taça",          "Ao vencer uma mesa, rende 2 moedas por rodada que você venceu nela.", C_OURO },
    [A_CORINGA]  = { "Coringa",       "O primeiro dado da sua sequência vale x2.", RGB(206, 80, 110) },
    [A_DUPLA]    = { "Dupla",         "Seu dado que repete o número de outro seu valendo ganha +2.", RGB(110, 160, 220) },
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

// ---------------------------------------------------------------------------
// Perfis: ate 3 por aparelho. Cada um tem nome, avatar e o que liberou no
// Modo Desafiante (a dificuldade mais alta e o que vem com ela); cada um
// guarda a sua run.
// ---------------------------------------------------------------------------
#define N_PERFIS 3
#define N_NIVEIS 5                       // dificuldades acima da normal
enum { AV_ET, AV_BERRO, AV_NOMALISA, AV_INTERRUPTOR, AV_N };
static const char *const NOME_AVATAR[AV_N] = { "ET Vilu", "O Berro", "Nomalisa", "Interruptor do Futuro" };

typedef struct {
    char     nome[12];                   // "" = lugar vazio
    uint8_t  avatar, nivel;              // nivel: a dificuldade mais alta liberada
    uint8_t  venceu[N_NIVEIS + 1];       // Barao vencido em cada dificuldade
    uint16_t runs;
} perfil_t;
static perfil_t perfis[N_PERFIS];
static int perfil_atual = -1;
static bool perfis_lidos;

static const char *chave_run(void)
{
    static const char *const K[N_PERFIS] = { "desafio", "desafio2", "desafio3" };
    return K[perfil_atual >= 0 && perfil_atual < N_PERFIS ? perfil_atual : 0];
}

static const perfil_t *perfil(void)
{
    static const perfil_t nenhum = { "VOCÊ", 0, 0, { 0 }, 0 };
    return perfil_atual >= 0 && perfis[perfil_atual].nome[0] ? &perfis[perfil_atual] : &nenhum;
}

static void hex_de(const void *p, size_t n, char *out)
{
    const uint8_t *b = (const uint8_t *)p;
    for (size_t i = 0; i < n; i++) {
        out[2 * i]     = "0123456789abcdef"[b[i] >> 4];
        out[2 * i + 1] = "0123456789abcdef"[b[i] & 15];
    }
    out[2 * n] = 0;
}
static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1); }
static bool de_hex(const char *s, void *p, size_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (size_t i = 0; i < n; i++) {
        int a = hexv(s[2 * i]), c = hexv(s[2 * i + 1]);
        if (a < 0 || c < 0) return false;
        b[i] = (uint8_t)(a << 4 | c);
    }
    return true;
}

// "PER1" + perfil escolhido por ultimo + os tres perfis em hexadecimal.
static void perfis_salva(void)
{
    static char buf[8 + 2 * sizeof perfis + 1];
    memcpy(buf, "PER1", 4);
    buf[4] = (char)('0' + (perfil_atual < 0 ? 0 : perfil_atual));
    hex_de(perfis, sizeof perfis, buf + 5);
    salva_grava("perfis", buf);
}

static void perfis_le(void)
{
    if (perfis_lidos) return;
    perfis_lidos = true;
    memset(perfis, 0, sizeof perfis);
    perfil_atual = -1;
    static char buf[16 + 2 * sizeof perfis];
    int n = salva_le("perfis", buf, (int)sizeof buf);
    if (n != 5 + (int)(2 * sizeof perfis) || memcmp(buf, "PER1", 4)) return;
    perfil_t lido[N_PERFIS];
    if (!de_hex(buf + 5, lido, sizeof lido)) return;
    for (int i = 0; i < N_PERFIS; i++) {
        lido[i].nome[sizeof lido[i].nome - 1] = 0;
        if (lido[i].avatar >= AV_N) lido[i].avatar = 0;
        if (lido[i].nivel > N_NIVEIS) lido[i].nivel = N_NIVEIS;
    }
    memcpy(perfis, lido, sizeof perfis);
    int a = buf[4] - '0';
    if (a >= 0 && a < N_PERFIS && perfis[a].nome[0]) perfil_atual = a;
}

// Conteudo liberado: o que vem com cada dificuldade.
static const uint8_t NIVEL_AMULETO[A_N] = {
    [A_FERRADURA] = 1, [A_PESADO] = 1, [A_RELOGIO] = 2, [A_CARTOLA] = 2, [A_COROA] = 3,
    [A_CUPOM] = 3, [A_PRENSA] = 4, [A_TACA] = 4, [A_CORINGA] = 5, [A_DUPLA] = 5 };
static int nivel_do_dado(int t)
{
    switch (t) {
    case D_BUMERANGUE: return 1;
    case D_GEMEO:      return 2;
    case D_PRISMA:     return 3;
    case D_FENIX:      return 4;
    case D_TRONO:      return 5;
    default:           return 0;
    }
}
static bool dado_liberado(int t)  { return nivel_do_dado(t) <= perfil()->nivel; }
static bool amuleto_liberado(int a) { return NIVEL_AMULETO[a] <= perfil()->nivel; }

// ---------------------------------------------------------------------------
// Dados sorteados por raridade
// ---------------------------------------------------------------------------
// Pesos por dado de cada raridade [comum, incomum, raro, lenda]. Os dados
// iniciais (moeda, D4, D6) nunca saem, como no premio da partida normal.
static int sorteia_raridade(const float w[4])
{
    float total = 0;
    for (int t = 0; t < D_N; t++) if (!so_inicial(t) && dado_liberado(t)) total += w[TIPO[t].raridade];
    float x = (float)(rnd_r() % 1000000u) / 1000000.0f * total;
    for (int t = 0; t < D_N; t++) {
        if (so_inicial(t) || !dado_liberado(t)) continue;
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
// A dificuldade Prata encarece a loja; o Cupom barateia.
static int preco_ajusta(int p)
{
    if (run.dificuldade >= 2) p = p * 5 / 4;
    if (run_amuletos() >> A_CUPOM & 1) p = p * 3 / 4;
    return p;
}
static int preco_run(int t)
{
    static const int P[4] = { 25, 40, 60, 90 };
    return preco_ajusta(P[TIPO[t].raridade]);
}
static int preco_am(void) { return preco_ajusta(AM_PRECO); }

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
    if (run.dificuldade >= 3) {                  // Ouro: dois dados raros a mais
        static const float RARO[4] = { 0, 0.5f, 3, 1.5f };
        for (int i = 0; i < 2 && n < MAX_COL; i++) col[n++] = peca_r(sorteia_raridade(RARO));
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
// A IA da mesa: Prata faz as mesas dificeis pensarem mais; Diamante, os chefes.
static int nivel_mesa(const no_t *no)
{
    bool pensa = (run.dificuldade >= 2 && no->tipo == NO_DIFICIL) || (run.dificuldade >= 5 && no->tipo == NO_CHEFE);
    return no->nivel + (pensa && no->nivel < 2 ? 1 : 0);
}

static void des_salva(void)
{
    static char buf[2 * sizeof(run_t) + 1];
    hex_de(&run, sizeof run, buf);
    salva_grava(chave_run(), buf);
}

static bool des_carrega(run_t *r)
{
    static char buf[2 * sizeof(run_t) + 8];
    int n = salva_le(chave_run(), buf, (int)sizeof buf);
    // Os arquivos de antes (sem amuletos; sem dificuldade) carregam com o
    // que falta zerado.
    if (n != (int)(2 * sizeof(run_t)) && n != 2 * RUN_TAM_V1 && n != 2 * RUN_TAM_V2) return false;
    size_t tam = (size_t)n / 2;
    memset(r, 0, sizeof *r);
    if (!de_hex(buf, r, tam)) return false;
    if (memcmp(r->marca, "DES1", 4) || r->andar >= DES_ANDARES || r->passo >= DES_PASSOS
        || r->estado > R_AMULETO || r->n_col < N_FILA || r->n_col > MAX_COL) return false;
    if (tam == RUN_TAM_V1) r->am_loja = 0xFF;            // loja de antes: sem amuleto
    for (int i = 0; i < r->n_col; i++) if (r->col[i].tipo >= D_N) return false;
    if (r->dificuldade > N_NIVEIS) return false;
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
    des_liberou = -1;
    if (venceu && perfil_atual >= 0) {          // venceu o Barao: a proxima dificuldade abre
        perfil_t *p = &perfis[perfil_atual];
        int d = run.dificuldade;
        if (p->venceu[d] < 255) p->venceu[d]++;
        if (d == p->nivel && p->nivel < N_NIVEIS) { p->nivel++; des_liberou = p->nivel; }
        perfis_salva();
    }
    salva_apaga(chave_run());
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
    if (i >= 0 && i < 3 && run.am_oferta[i] != 0xFF) run_poe_amuleto(run.am_oferta[i]);
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
    run.dificuldade = (uint8_t)des_dif;
    run.moedas = run.dificuldade >= 4 ? 30 : DES_MOEDAS;
    if (perfil_atual >= 0) { if (perfis[perfil_atual].runs < 65535) perfis[perfil_atual].runs++; perfis_salva(); }
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
    J[0].nome = perfil()->nome;
    J[0].cor = C_LATAO;
    amul[0] = run_amuletos();
    memset(am_brilho, 0, sizeof am_brilho);
    amul[1] = PERSO[no->perso].amul;              // o trunfo do oponente
    op_amul_extra = -1;
    if (run.dificuldade >= 5) {                   // Diamante: mais um amuleto para ele
        static const uint8_t EXTRA[] = { A_LUVA, A_REDE, A_BOLSA, A_OURO, A_PENA, A_IMA,
                                         A_COURACA, A_RESERVA, A_POTE, A_FURADO };
        uint32_t h = no->semente * 2246822519u;
        for (int i = 0; i < 4; i++) {
            int a = EXTRA[(h >> (i * 8)) % sizeof EXTRA];
            if (!(amul[1] >> a & 1)) { amul[1] |= 1u << a; op_amul_extra = a; break; }
        }
    }
    des_rod_vencidas = 0;
    reg_op = PERSO[no->perso].reg;
    op_nervoso = false;
    fala_perso = no->perso;
    t_fala = 0;
    J[0].fichas = DES_FICHAS + (tem_am(0, A_RESERVA) ? 15 : 0);
    J[0].n_col = run.n_col;
    memcpy(J[0].col, run.col, sizeof run.col);
    J[1].nome = PERSO[no->perso].nome;
    J[1].cor = C_ROSA;
    J[1].fichas = DES_FICHAS + (tem_am(1, A_RESERVA) ? 15 : 0) + (run.dificuldade >= 1 ? 10 : 0);
    J[1].n_col = monta_oponente(no, run.andar, J[1].col);
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < J[j].n_col; i++) J[j].onde[i] = NA_BOLSA;
    nivel_jog[1] = nivel_mesa(no);
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
    fala_evento(FA_INICIO);
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
        salva_apaga(chave_run());
        return;
    }
    run.vitorias++;
    run.ganho = J[0].fichas;
    if (run_amuletos() >> A_CONTRATO & 1) run.ganho += J[0].fichas / 2;     // Contrato Sujo: +50%
    run.juros = 0;
    if (run_amuletos() >> A_COFRE & 1) {                                    // Cofre: 1 a cada 5
        int j = (run.moedas + run.ganho) / 5;
        run.juros = (uint8_t)(j > 15 ? 15 : j);
    }
    run.taca = 0;
    if (run_amuletos() >> A_TACA & 1) {                                     // Taca: 2 por rodada vencida
        int t = 2 * des_rod_vencidas;
        run.taca = (uint8_t)(t > 40 ? 40 : t);
    }
    run.moedas += run.ganho + run.juros + run.taca;
    run.tier = no->tipo == NO_CHEFE ? 3 : no->tipo;
    int tp = run.tier + ((run_amuletos() >> A_CARTOLA & 1) && run.tier < 3 ? 1 : 0);   // Cartola
    rs = no->semente ^ 0xA11CEu;
    rnd_r();
    sorteia_distintos(run.oferta, 3, PESO_PREMIO[tp]);
    run.estado = R_PREMIO;
    des_salva();
}

static void abre_premio_run(void)
{
    modo = M_DESAFIO;
    cpu = 0;
    baixo = 0;
    J[0].nome = perfil()->nome;
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

static const char *nome_dif(int d);
static uint16_t cor_dif(int d);
static void faixa_run(void)
{
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(18, ty, "MODO DESAFIANTE", C_LATAO, true);
    if (run.dificuldade) txt(18 + gfx_largura("MODO DESAFIANTE", 1) + 10, ty, nome_dif(run.dificuldade), cor_dif(run.dificuldade), true);
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
    // O retrato a esquerda; o resto do painel anda para a direita.
    int rx = 32, ry = y0 + 4, esc = 2, nl = 27;
    gfx_rect(rx - 2, ry - 2, RT_W * esc + 4, 58, no->tipo == NO_CHEFE ? C_OURO : C_LATAO_ESC);
    gfx_rect(rx, ry, RT_W * esc, 54, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(rx, ry, esc, no->perso, t_fase + no->perso * 1.3f, false, 0, nl);
    dica_zona(rx, ry, RT_W * esc, 54, DICA_TRUNFO, no->perso);
    x = rx + RT_W * esc + 10;
    int w = gfx_texto(x, y0 + 3, pe->nome, cor_no(no->tipo), 1, true);
    int xt = etiqueta_caixa(x + w + 10, y0 + 3, NOME_NO[no->tipo], cor_no(no->tipo));
    etiqueta_caixa(xt, y0 + 3, NIVEL_IA[nivel_mesa(no)], C_ACO);
    if ((run_amuletos() >> A_CARTOLA & 1) && tier < 3) tier++;             // Cartola: um premio acima
    const char *pr = PREMIO_TIER[tier];
    txt(GFX_W - 40 - gfx_largura(pr, 1), y0 + 3, pr, C_TEXTO_M, false);
    // O trunfo: nome em destaque e a regra; miuda se nao couber.
    int wt = gfx_texto(x, y0 + 22, pe->trunfo, C_ROSA, 1, true);
    int livre = GFX_W - 40 - (x + wt + 14);
    int esc_r = gfx_largura(pe->regra, 1) <= livre ? 1 : GFX_MIUDO;
    gfx_texto(x + wt + 14, y0 + 22 + (esc_r == 1 ? 0 : 3), pe->regra, C_MARFIM_S, esc_r, false);
    dica_zona(x, y0 + 20, GFX_W - 40 - x, TXT_H + 2, DICA_TRUNFO, no->perso);
    // A bolsa dele, miuda: da para ver contra o que vai jogar (o mouse em
    // cima de um dado diz o que ele faz).
    peca_t col[MAX_COL];
    int n = monta_oponente(no, run.andar, col);
    for (int i = 0; i < n; i++) {
        peca_parada((float)(x + 8 + i * 19), (float)(y0 + 52), 8, col[i], 1);
        dica_zona(x + i * 19 - 1, y0 + 43, 19, 19, DICA_DADO, col[i].tipo);
    }
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
        if (run_amuletos() >> a & 1) {
            int ax = 30 + (n % 4) * 19, ay = 66 + (n / 4) * 19;
            desenha_amuleto(ax, ay, 8, a, false);
            dica_zona(ax - 9, ay - 9, 18, 18, DICA_AMULETO, a);
            n++;
        }
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
        int pr = am ? preco_am() : preco_run(run.loja[i].tipo);
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
        if (!run.am_levou && run.moedas < preco_am()) txt_c(GFX_W / 2, 284, "moedas insuficientes", C_VINHO_CLR, true);
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
        if (!(run_amuletos() >> a & 1)) continue;
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
static void desenha_liberou(void);
static void desenha_fim_run(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_run();
    bool abriu = des_venceu_run && des_liberou > 0;
    painel(CENTRO - 84, abriu ? 208 : 168, 500);
    char s[96];
    if (des_venceu_run) {
        txt_sombra_c(GFX_W / 2, CENTRO - 74, "VITÓRIA", C_OURO, 3);
        txt_c(GFX_W / 2, CENTRO - 6, "Você venceu O BARÃO e fechou o cassino.", C_MARFIM, true);
        desenha_liberou();
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
    txt_c(GFX_W / 2, CENTRO + (abriu ? 96 : 56), "ENTER volta ao menu", C_MARFIM_S, false);
}

// Tela do fim de uma partida da run (por cima da mesa, como a normal).
static void desenha_fim_partida_run(void)
{
    // O oponente, grande, a esquerda, com o que disse no fim.
    if (fala_perso >= 0) {
        int esc = 3, rx = 22, ry = CENTRO - 70;
        bool chefe = fala_perso >= N_PERSO_COMUM;
        gfx_rect(rx - 2, ry - 2, RT_W * esc + 4, RT_H * esc + 4, chefe ? C_OURO : C_LATAO_ESC);
        gfx_rect(rx, ry, RT_W * esc, RT_H * esc, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
        retrato(rx, ry, esc, fala_perso, t_fase + fala_perso * 1.3f, t_fala > 0.3f, 0, RT_H);
        dica_zona(rx, ry, RT_W * esc, RT_H * esc, DICA_TRUNFO, fala_perso);
        if (fala_txt[0]) txt_c(rx + RT_W * esc / 2, ry + RT_H * esc + 6, PERSO[fala_perso].nome, C_ROSA, true);
    }
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
        if (run.taca) {
            char j2[24];
            snprintf(j2, sizeof j2, "  ·  Taça +%d", run.taca);
            strncat(s, j2, sizeof s - strlen(s) - 1);
        }
        moedas_c(GFX_W / 2, CENTRO + 6, run.ganho + run.juros + run.taca, " moedas", C_OURO);
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
    if (fala_perso >= 0 && fala_txt[0]) {                 // a ultima palavra dele
        char q[80];
        snprintf(q, sizeof q, "\"%s\"", fala_txt);
        txt_c(GFX_W / 2, CENTRO + 88, q, C_ROSA, false);
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
    int na = am_conta(run_amuletos());
    if (na) {
        int x0 = GFX_W / 2 - (na - 1) * 10, n = 0;
        txt_c(GFX_W / 2, 258, "seus amuletos", C_TEXTO_M, false);
        for (int a = 0; a < A_N; a++)
            if (run_amuletos() >> a & 1) desenha_amuleto(x0 + 20 * n++, 288, 8, a, false);
    }
    rodape_run("setas escolhem  ·  ENTER leva o amuleto");
}

// ---------------------------------------------------------------------------
// Perfis e dificuldades
// ---------------------------------------------------------------------------
static const char *const NOME_DIF[N_NIVEIS + 1] = { "Normal", "Bronze", "Prata", "Ouro", "Rubi", "Diamante" };
static const uint16_t COR_DIF[N_NIVEIS + 1] = { C_MARFIM, C_BRONZE, C_PRATA, C_OURO, RGB(222, 70, 92), RGB(150, 220, 240) };
// O que cada dificuldade acrescenta (vale junto com as de baixo).
static const char *const PENA_DIF[N_NIVEIS + 1] = {
    "Os salões como sempre.",
    "O oponente começa com 10 fichas a mais.",
    "A loja fica 25% mais cara, e as mesas difíceis pensam mais.",
    "Os oponentes trazem 2 dados raros a mais.",
    "Você começa a run com 30 moedas, não 50.",
    "Cada oponente ganha um amuleto, e os chefes pensam mais.",
};

static const char *nome_dif(int d) { return NOME_DIF[d <= N_NIVEIS ? d : 0]; }
static uint16_t cor_dif(int d) { return COR_DIF[d <= N_NIVEIS ? d : 0]; }

static int  pf_cur;                      // card em foco na tela de perfis
static bool pf_apagando;                 // pedindo confirmacao para apagar
static int  pf_avatar;                   // tela de criar perfil
static char pf_nome[11];
static int  dif_cur;

// O que abre com a dificuldade n: dois amuletos e um dado.
static void liberados(int n, int *am, int *dado)
{
    int k = 0;
    for (int a = 0; a < A_N && k < 2; a++) if (NIVEL_AMULETO[a] == n) am[k++] = a;
    for (; k < 2; k++) am[k] = -1;
    *dado = -1;
    for (int t = 0; t < D_N; t++) if (nivel_do_dado(t) == n) *dado = t;
}

static void des_abre_perfis(void)
{
    perfis_le();
    pf_cur = perfil_atual >= 0 ? perfil_atual : 0;
    pf_apagando = false;
    t_fase = 0;
    fase = F_PERFIS;
}

// Nova run: com dificuldades abertas, pergunta qual antes.
static void des_escolhe_nova(void)
{
    if (perfil()->nivel > 0) {
        dif_cur = perfil()->nivel;
        t_fase = 0;
        fase = F_RDIFIC;
        return;
    }
    des_dif = 0;
    des_nova_run();
}

static void des_entra_perfil(int i)
{
    perfil_atual = i;
    perfis_salva();
    if (des_tem_run()) {                         // continuar ou comecar outra
        menu_tela = MT_DESAFIO;
        menu_cur = 0;
        t_fase = 1.0f;
        fase = F_ABERTURA;
        return;
    }
    des_escolhe_nova();
}

static int avatar_retrato(void) { return perfil_atual >= 0 && perfis[perfil_atual].nome[0] ? RETRATO_AVATAR + perfil()->avatar : -1; }

static void moldura_retrato(int x, int y, int esc, int av, float t, bool cur)
{
    gfx_rect(x - 3, y - 3, RT_W * esc + 6, RT_H * esc + 6, cur ? C_OURO : C_LATAO_ESC);
    gfx_rect(x - 1, y - 1, RT_W * esc + 2, RT_H * esc + 2, C_BARRA);
    gfx_rect(x, y, RT_W * esc, RT_H * esc, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(x, y, esc, RETRATO_AVATAR + av, t, false, 0, RT_H);
}

static void faixa_titulo(const char *s)
{
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(18, ty, "MODO DESAFIANTE", C_LATAO, true);
    txt_c(GFX_W / 2, ty, s, C_MARFIM, true);
}

static void desenha_perfis(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_titulo("Quem vai jogar?");
    for (int i = 0; i < N_PERFIS; i++) {
        const perfil_t *p = &perfis[i];
        int w = 170, h = 184, x = GFX_W / 2 + (i - 1) * 186 - w / 2, y = 70;
        bool cur = i == pf_cur;
        gfx_rect(x, y, w, h, cur ? C_FELTRO_ESC : C_FELTRO);
        gfx_moldura(x, y, w, h, cur ? 3 : 1, cur ? C_OURO : gfx_mistura(C_FELTRO, C_OURO, 45));
        zona_clique(x, y, w, h, &pf_cur, i, KEY_ENTER);
        if (!p->nome[0]) {
            int cx = x + w / 2, cy = y + 84;
            uint16_t c = cur ? C_OURO : C_TEXTO_M;
            gfx_rect(cx - 14, cy - 2, 28, 4, c);
            gfx_rect(cx - 2, cy - 14, 4, 28, c);
            txt_c(cx, y + 120, "novo perfil", c, cur);
            continue;
        }
        int esc = 3, rx = x + (w - RT_W * esc) / 2, ry = y + 14;
        moldura_retrato(rx, ry, esc, p->avatar, t_fase + i * 2.1f, cur);
        txt_c(x + w / 2, ry + RT_H * esc + 8, p->nome, cur ? C_OURO : C_MARFIM, true);
        char s[48];
        int nv = p->nivel;
        snprintf(s, sizeof s, "dificuldade %s", NOME_DIF[nv]);
        txt_c(x + w / 2, ry + RT_H * esc + 26, s, nv ? COR_DIF[nv] : C_MARFIM_S, false);
        int ganhou = 0;
        for (int d = 0; d <= N_NIVEIS; d++) ganhou += p->venceu[d];
        if (ganhou) snprintf(s, sizeof s, "Barão vencido %d %s", ganhou, ganhou == 1 ? "vez" : "vezes");
        else snprintf(s, sizeof s, "%d %s", p->runs, p->runs == 1 ? "run" : "runs");
        txt_c(x + w / 2, ry + RT_H * esc + 42, s, C_TEXTO_M, false);
    }
    if (pf_apagando && perfis[pf_cur].nome[0]) {
        char s[80];
        snprintf(s, sizeof s, "Apagar %s e a run guardada?", perfis[pf_cur].nome);
        txt_c(GFX_W / 2, 266, s, C_VINHO_CLR, true);
        rodape_run("ENTER ou X apaga  ·  ESC deixa como está");
    } else {
        if (perfis[pf_cur].nome[0]) {
            int bx = GFX_W / 2 + (pf_cur - 1) * 186 - 30;
            gfx_rect(bx, 264, 60, 16, C_BARRA);
            gfx_moldura(bx, 264, 60, 16, 1, C_VINHO_CLR);
            txt_c(bx + 30, 264, "apagar", C_VINHO_CLR, false);
            zona_clique(bx, 264, 60, 16, NULL, 0, 'x');
        }
        rodape_run("setas escolhem  ·  ENTER entra  ·  X apaga  ·  ESC volta");
    }
}

static void desenha_cria_perfil(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_titulo("Novo perfil");
    int esc = 4, rx = GFX_W / 2 - RT_W * esc / 2, ry = 58;
    moldura_retrato(rx, ry, esc, pf_avatar, t_fase, true);
    // Setas para trocar o retrato.
    for (int lado = -1; lado <= 1; lado += 2) {
        int cx = GFX_W / 2 + lado * (RT_W * esc / 2 + 34), cy = ry + RT_H * esc / 2;
        float o[6] = { (float)(cx + lado * 8), (float)cy, (float)(cx - lado * 6), (float)(cy - 10),
                       (float)(cx - lado * 6), (float)(cy + 10) };
        float vx[3] = { o[0], o[2], o[4] }, vy[3] = { o[1], o[3], o[5] };
        poligono(vx, vy, 3, C_LATAO);
        zona_clique(cx - 16, cy - 18, 32, 36, NULL, 0, lado < 0 ? KEY_LEFT : KEY_RIGHT);
    }
    int y = ry + RT_H * esc + 8;
    txt_c(GFX_W / 2, y, NOME_AVATAR[pf_avatar], C_OURO, true);
    // O nome, digitado.
    int bw = 180, bx = GFX_W / 2 - bw / 2, by = y + 22;
    gfx_rect(bx, by, bw, 20, C_BARRA);
    gfx_moldura(bx, by, bw, 20, 1, C_LATAO);
    char s[16];
    bool pisca = fmodf(t_fase, 1.0f) < 0.55f;
    snprintf(s, sizeof s, "%s%s", pf_nome, pisca && strlen(pf_nome) < 10 ? "_" : " ");
    if (pf_nome[0] || pisca) txt_c(GFX_W / 2, by + 2, s, C_MARFIM, true);
    if (!pf_nome[0] && !pisca) txt_c(GFX_W / 2, by + 2, "seu nome", C_TEXTO_M, false);
    // Botao de criar.
    int ok = pf_nome[0] != 0;
    gfx_rect(GFX_W / 2 - 50, by + 30, 100, 20, ok ? C_LATAO : C_FELTRO2);
    txt_c(GFX_W / 2, by + 31, "criar", ok ? C_BARRA : C_TEXTO_M, true);
    zona_clique(GFX_W / 2 - 50, by + 30, 100, 20, NULL, 0, KEY_ENTER);
    rodape_run("setas trocam o retrato  ·  digite o nome  ·  ENTER cria  ·  ESC volta");
}

static void desenha_dificuldade(void)
{
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    faixa_titulo("Dificuldade da run");
    int nv = perfil()->nivel, ult = nv < N_NIVEIS ? nv + 1 : N_NIVEIS;
    int x0 = 30, y0 = 54;
    for (int d = 0; d <= ult; d++) {
        bool aberta = d <= nv, cur = d == dif_cur;
        int y = y0 + d * 26;
        gfx_rect(x0, y, 150, 22, cur ? C_FELTRO_ESC : C_FELTRO);
        gfx_moldura(x0, y, 150, 22, cur ? 2 : 1, cur ? C_OURO : gfx_mistura(C_FELTRO, C_OURO, 40));
        uint16_t c = aberta ? COR_DIF[d] : C_FELTRO2;
        gfx_disco(x0 + 14, y + 11, 6, gfx_mistura(c, C_BARRA, 30));
        gfx_disco(x0 + 14, y + 11, 4, c);
        txt(x0 + 28, y + 3, NOME_DIF[d], aberta ? c : C_TEXTO_M, cur);
        if (perfil()->venceu[d]) {                  // medalha: o Barao ja caiu nesta
            gfx_disco(x0 + 134, y + 11, 5, C_LATAO_ESC);
            gfx_disco(x0 + 134, y + 11, 4, C_OURO);
            gfx_disco(x0 + 133, y + 10, 1, C_BRANCO);
        }
        if (aberta) zona_clique(x0, y, 150, 22, &dif_cur, d, KEY_ENTER);
        else zona_clique(x0, y, 150, 22, &dif_cur, d, 0);
    }
    // O que vale na escolhida: tudo o que vem de baixo, mais o dela.
    int px = 194, py = 54;
    gfx_rect(px, py, 416, 232, C_FELTRO_ESC);
    gfx_moldura(px, py, 416, 232, 1, C_LATAO_ESC);
    int d = dif_cur;
    txt(px + 12, py + 8, NOME_DIF[d], COR_DIF[d], true);
    int ly = py + 30;
    if (d > nv) {
        char s[64];
        snprintf(s, sizeof s, "Vença o Barão no %s para abrir.", NOME_DIF[d - 1]);
        txt(px + 12, ly, s, C_VINHO_CLR, false);
        ly += 22;
    }
    for (int k = d == 0 ? 0 : 1; k <= d; k++) {
        txt(px + 12, ly, k == d ? "+" : "·", k == d ? COR_DIF[d] : C_TEXTO_M, true);
        txt(px + 24, ly, PENA_DIF[k], k == d ? C_MARFIM : C_TEXTO_M, false);
        ly += 18;
    }
    if (d > 0) {
        int am[2], dado;
        liberados(d, am, &dado);
        ly += 6;
        txt(px + 12, ly, d <= nv ? "Abriu para você:" : "Abre:", C_TEXTO_M, false);
        ly += 26;
        int ix = px + 24;
        for (int i = 0; i < 2; i++) {
            if (am[i] < 0) continue;
            desenha_amuleto(ix, ly, 9, am[i], d > nv);
            txt(ix + 14, ly - 6, AMULETO[am[i]].nome, d > nv ? C_TEXTO_M : AMULETO[am[i]].cor, false);
            if (d <= nv) dica_zona(ix - 10, ly - 10, 120, 20, DICA_AMULETO, am[i]);
            ly += 24;
        }
        if (dado >= 0) {
            desenha_dado((float)ix, (float)ly, 9, dado, 0, 0, 0, 0, d > nv);
            txt(ix + 14, ly - 6, TIPO[dado].nome, d > nv ? C_TEXTO_M : cor_aro(dado), false);
            if (d <= nv) dica_zona(ix - 10, ly - 10, 120, 20, DICA_DADO, dado);
        }
    }
    rodape_run("setas escolhem  ·  ENTER começa a run  ·  ESC volta");
}

static void desenha_liberou(void)
{
    if (des_liberou < 1) return;
    int am[2], dado;
    liberados(des_liberou, am, &dado);
    char s[64];
    snprintf(s, sizeof s, "Abriu a dificuldade %s", NOME_DIF[des_liberou]);
    txt_c(GFX_W / 2, CENTRO + 44, s, COR_DIF[des_liberou], true);
    int x = GFX_W / 2 - 150;
    for (int i = 0; i < 2; i++)
        if (am[i] >= 0) {
            desenha_amuleto(x + 8, CENTRO + 74, 8, am[i], false);
            txt(x + 20, CENTRO + 68, AMULETO[am[i]].nome, AMULETO[am[i]].cor, false);
            dica_zona(x, CENTRO + 64, 96, 20, DICA_AMULETO, am[i]);
            x += 104;
        }
    if (dado >= 0) {
        desenha_dado((float)(x + 8), (float)(CENTRO + 74), 8, dado, 0, 0, 0, 0, false);
        txt(x + 20, CENTRO + 68, TIPO[dado].nome, cor_aro(dado), false);
        dica_zona(x, CENTRO + 64, 96, 20, DICA_DADO, dado);
    }
}

static bool des_desenha(void)
{
    switch (fase) {
    case F_MAPA:   desenha_mapa();      return true;
    case F_RLOJA:  desenha_loja_run();  return true;
    case F_RBOLSA: desenha_bolsa_run(); return true;
    case F_RFIM:   desenha_fim_run();   return true;
    case F_RAMULETO: desenha_amuleto_run(); return true;
    case F_PERFIS: desenha_perfis();    return true;
    case F_PCRIA:  desenha_cria_perfil(); return true;
    case F_RDIFIC: desenha_dificuldade(); return true;
    default:       return false;
    }
}

// ESC nas telas de perfil e dificuldade: um passo para tras.
static void des_esc(void)
{
    if (fase == F_PERFIS && pf_apagando) { pf_apagando = false; return; }
    if (fase == F_PCRIA) { t_fase = 0; fase = F_PERFIS; return; }
    menu_tela = fase == F_RDIFIC && des_tem_run() ? MT_DESAFIO : MT_UM;
    menu_cur = 1;
    t_fase = 1.0f;
    fase = F_ABERTURA;
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
            if (run.am_levou || run.am_loja >= A_N || run.moedas < preco_am()) return true;
            run.moedas -= preco_am();
            run_poe_amuleto(run.am_loja);
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
            int tot = run.n_col + am_conta(run_amuletos());
            cursor = (cursor + e + tot) % tot;
            som_toca(SOM_TIQUE);
            return true;
        }
        if (k == KEY_TAB || k == KEY_ENTER || k == KEY_BKSP || k == 'x') { t_fase = 0; fase = F_MAPA; }
        return true;

    case F_RFIM:
        if (k == KEY_ENTER) volta_ao_menu();
        return true;

    case F_PERFIS: {
        bool cheio = perfis[pf_cur].nome[0] != 0;
        if (pf_apagando) {
            if ((k == KEY_ENTER || k == 'x') && cheio) {
                int antes = perfil_atual;
                perfil_atual = pf_cur;
                salva_apaga(chave_run());
                memset(&perfis[pf_cur], 0, sizeof perfis[pf_cur]);
                perfil_atual = antes == pf_cur ? -1 : antes;
                perfis_salva();
                som_toca(SOM_FICHA);
            }
            pf_apagando = false;
            return true;
        }
        if (e) { pf_cur = (pf_cur + e + N_PERFIS) % N_PERFIS; som_toca(SOM_TIQUE); return true; }
        if (k >= '1' && k <= '3') { pf_cur = k - '1'; k = KEY_ENTER; cheio = perfis[pf_cur].nome[0] != 0; }
        if ((k == 'x' || k == KEY_BKSP) && cheio) { pf_apagando = true; return true; }
        if (k != KEY_ENTER) return true;
        som_toca(SOM_FICHA);
        if (cheio) { des_entra_perfil(pf_cur); return true; }
        pf_avatar = pf_cur % AV_N;
        pf_nome[0] = 0;
        t_fase = 0;
        fase = F_PCRIA;
        return true;
    }

    case F_PCRIA: {
        int n = (int)strlen(pf_nome);
        if (k == KEY_LEFT || k == KEY_RIGHT) {
            pf_avatar = (pf_avatar + (k == KEY_LEFT ? AV_N - 1 : 1)) % AV_N;
            som_toca(SOM_TIQUE);
            return true;
        }
        if (k == KEY_BKSP) { if (n) pf_nome[n - 1] = 0; return true; }
        if (k == KEY_ENTER) {
            if (!n) return true;
            while (n && pf_nome[n - 1] == ' ') pf_nome[--n] = 0;
            perfil_t *p = &perfis[pf_cur];
            memset(p, 0, sizeof *p);
            snprintf(p->nome, sizeof p->nome, "%s", pf_nome);
            p->avatar = (uint8_t)pf_avatar;
            som_toca(SOM_PREMIO);
            des_entra_perfil(pf_cur);
            return true;
        }
        bool letra = k >= 'a' && k <= 'z', num = k >= '0' && k <= '9';
        if (n < 10 && (letra || num || (k == ' ' && n > 0 && pf_nome[n - 1] != ' '))) {
            pf_nome[n] = (char)(letra ? k - 32 : k);
            pf_nome[n + 1] = 0;
            som_toca(SOM_TIQUE);
        }
        return true;
    }

    case F_RDIFIC: {
        int nv = perfil()->nivel, ult = nv < N_NIVEIS ? nv + 1 : N_NIVEIS;
        if (e) { dif_cur = (dif_cur + e + ult + 1) % (ult + 1); som_toca(SOM_TIQUE); return true; }
        if (k == KEY_ENTER && dif_cur <= nv) {
            som_toca(SOM_FICHA);
            des_dif = dif_cur;
            des_nova_run();
        }
        return true;
    }

    default:
        return false;
    }
}
