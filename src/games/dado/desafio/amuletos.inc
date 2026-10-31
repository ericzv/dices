// ---------------------------------------------------------------------------
// Amuletos: efeitos que valem a run inteira. Um no comeco e um por chefe
// vencido (a loja nao vende). Cabem 5: o sexto pede troca.
// ---------------------------------------------------------------------------
typedef struct { const char *nome, *desc; uint16_t cor; } amuleto_t;
static const amuleto_t AMULETO[A_N] = {
    [A_LUVA]     = { "Luva de Couro", "O primeiro dado da sequência não é anulado quando o dado seguinte é menor.", RGB(196, 140, 90) },
    [A_REDE]     = { "Rede",          "Se o último dado da sequência for anulado pela sequência, o dado anterior continua válido.", RGB(150, 186, 214) },
    [A_BOLSA]    = { "Bolsa Funda",   "Você compra 8 dados por rodada, em vez de 7.", RGB(204, 150, 96) },
    [A_OURO]     = { "Dado de Ouro",  "Dado seu que tirar o número máximo pontua +2.", C_OURO },
    [A_PENA]     = { "Peso Pena",     "Seus dados de 2 e 4 lados pontuam +1.", RGB(196, 176, 230) },
    [A_CONTRATO] = { "Contrato Sujo", "Cada dado válido seu pontua +2, mas a sequência leva 1 dado a menos.", C_VINHO_CLR },
    [A_IMA]      = { "Ímã",           "Nenhum dado seu pode ser roubado.", RGB(214, 84, 84) },
    [A_COURACA]  = { "Couraça",       "Os dados do oponente não podem anular os seus.", C_ACO },
    [A_COFRE]    = { "Cofre",         "Ao vencer uma mesa, rende 1 moeda a cada 5 que você tem (até 3).", C_LATAO },
    [A_RESERVA]  = { "Reserva",       "Você começa cada partida com 10 fichas a mais.", RGB(104, 186, 110) },
    [A_POTE]     = { "Pote Gordo",    "Vencendo uma rodada, você leva 2 fichas a mais.", RGB(222, 154, 50) },
    [A_FURADO]   = { "Saco Furado",   "O oponente compra só 6 dados por rodada.", RGB(170, 160, 150) },
    [A_FERRADURA]= { "Ferradura",     "Uma vez por rodada, na primeira vez que a sequência anularia o dado anterior, ele continua válido.", C_PRATA },
    [A_PESADO]   = { "Dado Pesado",   "Seus dados de 6 lados pontuam +1.", RGB(120, 128, 140) },
    [A_RELOGIO]  = { "Relógio de Bolso", "Na primeira rodada e quando você está atrás em fichas, o oponente abre; você joga depois.", RGB(214, 186, 110) },
    [A_CARTOLA]  = { "Cartola",       "A vitrine das lojas mostra 3 dados, em vez de 2.", RGB(150, 110, 170) },
    [A_COROA]    = { "Coroa do Azarão", "Começando a rodada com menos fichas que o oponente, seus dados pontuam +1.", RGB(226, 176, 70) },
    [A_CUPOM]    = { "Cupom",         "Tudo na loja custa 1 moeda a menos (nunca menos de 1).", RGB(120, 190, 150) },
    [A_PRENSA]   = { "Prensa",        "Seu dado que tirar 1 vira um 3.", RGB(150, 150, 160) },
    [A_TACA]     = { "Taça",          "Ao vencer uma mesa, rende 1 moeda a cada 3 rodadas que você venceu nela.", C_OURO },
    [A_CORINGA]  = { "Coringa",       "O primeiro dado da sua sequência pontua +2.", RGB(206, 80, 110) },
    [A_DUPLA]    = { "Dupla",         "Seu dado que repetir o número de outro dado válido seu pontua +2.", RGB(110, 160, 220) },
    [A_SELO]     = { "Selo de Dívida", "Chefe vencido deixa um segundo amuleto, sorteado entre os comuns e os incomuns.", RGB(196, 60, 60) },
    [A_PEDAGIO]  = { "Pedágio",       "Os chefes começam a partida com 10 fichas a menos.", RGB(226, 186, 70) },
    [A_DESFORRA] = { "Desforra",      "Se você perdeu a rodada anterior, cada dado válido seu pontua +1.", RGB(222, 112, 72) },
    [A_MAU_OLHADO] = { "Mau-Olhado",  "Cada rodada que você vence põe um Fantasma (pontua sempre 1) na bolsa do oponente, até o fim da partida.", RGB(84, 136, 222) },
    [A_CRISANTEMO] = { "Crisântemo",  "Cada dado seu anulado na rodada pontua +1 para você no fim.", RGB(232, 206, 128) },
    [A_TRINCA]   = { "Trinca",        "Dado seu que tirar 3 pontua +3.", RGB(72, 150, 138) },
    [A_PIRAMIDE] = { "Pirâmide",      "Seus dados de 4 lados pontuam x2 no fim.", RGB(238, 162, 74) },
    [A_ABRE_ALAS]= { "Abre-Alas",     "O primeiro dado da sua sequência pontua x2 no fim.", RGB(96, 134, 196) },
    [A_SAIDEIRA] = { "Saideira",      "Na última rodada da partida, os poderes de fim de rodada dos seus dados agem duas vezes.", RGB(150, 112, 160) },
    [A_COLAR]    = { "Colar de Pérolas", "Ao vencer uma mesa, rende 1 moeda.", RGB(226, 220, 236) },
    [A_CORRIMAO] = { "Corrimão",      "Se o seu dado for só 1 menor que o anterior, a sequência não anula nenhum dos dois.", RGB(96, 182, 198) },
};
// Raridade de cada um, que diz em que momento da run ele aparece: comuns no
// comeco, incomuns depois do primeiro chefe, raros depois do segundo e, na
// Cobertura, do Barao. Fora: nao sai para o jogador
// (nenhum hoje). Os de nivel (NIVEL_AMULETO) so entram depois de abertos.
enum { AMR_FORA, AMR_COMUM, AMR_INCOMUM, AMR_RARO };
static const uint8_t RAR_AMULETO[A_N] = {
    [A_BOLSA] = AMR_COMUM, [A_OURO] = AMR_COMUM, [A_PENA] = AMR_COMUM, [A_IMA] = AMR_COMUM,
    [A_COURACA] = AMR_COMUM, [A_COFRE] = AMR_COMUM, [A_POTE] = AMR_COMUM, [A_FURADO] = AMR_COMUM,
    [A_SELO] = AMR_COMUM, [A_DESFORRA] = AMR_COMUM, [A_RELOGIO] = AMR_COMUM, [A_CUPOM] = AMR_COMUM,
    [A_TACA] = AMR_COMUM, [A_CORINGA] = AMR_COMUM, [A_DUPLA] = AMR_COMUM, [A_COLAR] = AMR_COMUM,
    [A_CONTRATO] = AMR_INCOMUM, [A_RESERVA] = AMR_INCOMUM, [A_PEDAGIO] = AMR_INCOMUM, [A_MAU_OLHADO] = AMR_INCOMUM,
    [A_PESADO] = AMR_INCOMUM, [A_CARTOLA] = AMR_INCOMUM, [A_CRISANTEMO] = AMR_INCOMUM, [A_TRINCA] = AMR_INCOMUM,
    [A_PIRAMIDE] = AMR_INCOMUM, [A_COROA] = AMR_INCOMUM, [A_PRENSA] = AMR_INCOMUM,
    [A_LUVA] = AMR_RARO, [A_FERRADURA] = AMR_RARO, [A_REDE] = AMR_RARO, [A_ABRE_ALAS] = AMR_RARO, [A_SAIDEIRA] = AMR_RARO, [A_CORRIMAO] = AMR_RARO,
};
#define AMR(r) (1u << (r))
static bool amuleto_no_jogo(int a) { return RAR_AMULETO[a] != AMR_FORA; }
static const char *nome_rar_am(int a) { return RAR_AMULETO[a] ? RARIDADE[RAR_AMULETO[a] - 1] : ""; }
static uint16_t cor_rar_am(int a) { return RAR_AMULETO[a] ? COR_RAR[RAR_AMULETO[a] - 1] : C_TEXTO_M; }


static int am_conta(amul_t m) { int n = 0; for (; m; m >>= 1) n += m & 1; return n; }
static amul_t run_amuletos(void)
{
    return (amul_t)run.amuletos | (amul_t)run.amuletos_hi << 16 | (amul_t)run.amuletos_x << 32;
}
static void run_poe_amuletos(amul_t m)
{
    run.amuletos = (uint16_t)m;
    run.amuletos_hi = (uint16_t)(m >> 16);
    run.amuletos_x = (uint8_t)(m >> 32);
}
static void run_poe_amuleto(int a) { run_poe_amuletos(run_amuletos() | AM_BIT(a)); }
static void run_tira_amuleto(int a) { run_poe_amuletos(run_amuletos() & ~AM_BIT(a)); }
static bool am_lugar(void) { return am_conta(run_amuletos()) < am_lugares(); }
static bool amuleto_liberado(int a);
static bool dado_liberado(int t);

// Sorteia n amuletos diferentes que a run ainda nao tem, das raridades em
// rar (AMR(...)); 0xFF se faltar.
static void sorteia_amuletos(uint8_t *out, int n, unsigned rar)
{
    int livres[A_N], nl = 0;
    for (int a = 0; a < A_N; a++)
        if (!(run_amuletos() >> a & 1) && amuleto_liberado(a) && (rar >> RAR_AMULETO[a] & 1) && amuleto_no_jogo(a))
            livres[nl++] = a;
    for (int i = 0; i < n; i++) {
        if (!nl) { out[i] = 0xFF; continue; }
        int q = sorte_r(nl);
        out[i] = (uint8_t)livres[q];
        livres[q] = livres[--nl];
    }
}

// O medalhao: o aro de latao em rampa (a luz de cima a esquerda), o campo
// afundado no escuro da cor do amuleto e o simbolo em pixels, desenhado a
// mao numa grade de 11 x 11 so com cores da paleta, com o contorno escuro
// em volta. O simbolo cresce em degraus inteiros (1x na faixa, 2x e 3x na
// escolha, 4x na vitrine); pequeno demais para o aro (r < 8), vai so o
// simbolo. Apagado: tudo nos tons do feltro, pelo claro e escuro de cada pixel.
#define AM_PX 11
// As cores do desenho: cada letra e um tom da paleta (docs/estilo.md).
static const char AM_CHAVE[] = "KeEzZcCsSWwmngGhHjJlLoOyqQprRPfFbBuUaAiIvVtT";
static const uint16_t AM_COR[] = { RGB(14, 14, 12), RGB(36, 32, 29), RGB(62, 56, 50), RGB(96, 88, 80), RGB(128, 120, 112), RGB(70, 76, 88), RGB(104, 112, 124), RGB(140, 150, 164), RGB(184, 194, 208), RGB(252, 248, 236), RGB(238, 230, 212), RGB(204, 194, 172), RGB(166, 154, 132), RGB(88, 152, 84), RGB(130, 204, 112), RGB(56, 32, 20), RGB(78, 46, 30), RGB(120, 78, 50), RGB(156, 106, 64), RGB(106, 86, 44), RGB(150, 122, 62), RGB(190, 156, 86), RGB(228, 194, 106), RGB(246, 222, 132), RGB(66, 18, 24), RGB(118, 30, 38), RGB(150, 62, 80), RGB(190, 56, 60), RGB(208, 98, 108), RGB(226, 164, 168), RGB(214, 112, 44), RGB(238, 162, 74), RGB(26, 34, 62), RGB(56, 86, 150), RGB(96, 134, 196), RGB(168, 204, 230), RGB(72, 150, 138), RGB(96, 182, 198), RGB(54, 36, 74), RGB(102, 74, 138), RGB(150, 112, 160), RGB(206, 180, 218), RGB(240, 200, 164), RGB(200, 142, 104) };
static const char *const AM_DESENHO[A_N][AM_PX] = {
    [A_LUVA] = {
        "..Jj.Jj.Jj.", "..Jj.Jj.Jj.", "..Jj.Jj.Jj.", "..JjjJjjJjj",
        "J.Jjjjjjjjj", "JJjjjjjjjjH", ".JjjjjjjjjH", "..jjjjjjjjH",
        "..HHHHHHHHh", "..hohohohoh", "..hhhhhhhhh",
    },
    [A_REDE] = {
        "y.........O", "oU.......UL", "o.UUUUUUU.L", "oU.U.U.U.UL",
        "o.U.U.U.U.L", "ou.u.u.u.uL", "o.u.u.u.u.L", "o..u.u.u..L",
        "o...uuu...L", "o.........L", "o.........L",
    },
    [A_BOLSA] = {
        "..J.J.J.J..", "..JjJjJjJ..", "...jjjjj...", "..yOOOOOL..",
        "..JjjjjjH..", ".JJjjjjjjH.", ".Jjjjjjjjjh", "JJjjjjjjjjh",
        "Jjjjjjjjjhh", ".jjjjjjjhh.", "..hhhhhhh..",
    },
    [A_OURO] = {
        ".yyyyyyyyy.", "yyOOOOOOOOo", "yOOOOOOllOo", "yOOOOOOllOo",
        "yOOOOOOOOOo", "yOOOOllOOOo", "yOOOOllOOOo", "yOOOOOOOOOo",
        "yOllOOOOOOo", "yOllOOOOOOo", ".ooooooooo.",
    },
    [A_PENA] = {
        "..........w", ".......VVw.", "......Vvwv.", ".....VvwvI.",
        "....VvwvI..", "...VvwvI...", "...vwvI....", "..vwvI.....",
        "..wv.......", ".w.........", "w..........",
    },
    [A_CONTRATO] = {
        "wwwwwwww...", "wmmmmmmmn..", "wmEEEEEmn..", "wmmmmmmmn..",
        "wmEEEEmmn..", "wmmmmJjmn..", "wmEEJjjjn..", "wmmmmjjmn..",
        "wmmmmmmmnhJ", "wmzEzzEmhJ.", "nnnnnnnnj..",
    },
    [A_IMA] = {
        ".SS.....SS.", ".Ss.....Ss.", ".Rr.....rQ.", ".Rr.....rQ.",
        ".Rr.....rQ.", ".Rr.....rQ.", ".Rrr...rrQ.", "..Rrrrrrrq.",
        "...rrrrrq..", "....QQQq...", "...........",
    },
    [A_COURACA] = {
        "SSSSSSsssss", "SWWWWWsssCc", "SWSSSSsssCc", "SWSSSSsssCc",
        "SWSSSSsssCc", ".SSSSSsssc.", ".SSSSSsssc.", "..SSSSssc..",
        "...SSSsc...", "....SSc....", ".....c.....",
    },
    [A_COFRE] = {
        "SSSSSSSSSSc", "SCCCCCCCCCc", "SC.......Cc", "SC.SSSs..Cc",
        "SC.SKsc.oCc", "SC.ssCc.oCc", "SC..cc...Cc", "SC.......Cc",
        "SCCCCCCCCCc", "Scccccccccc", ".c.......c.",
    },
    [A_RESERVA] = {
        "...........", "...RRRRR...", ".RRPRwRRRr.", ".RRRRRRRRr.",
        "..rrrrrrr..", ".QwQQwQQwQq", ".gGwGGwGGgg", ".ggggggggg.",
        ".BuBBuBBuBb", ".BBBBBBBBB.", "...........",
    },
    [A_POTE] = {
        "...yO.yO...", "..yOoyOoO..", ".yOoOyOoOo.", "JJJJJJJJJJj",
        ".Jjjjjjjjh.", "Jjjjjjjjjjh", "Jjjjjjjjjjh", "Jjjjjjjjjjh",
        ".jjjjjjjjh.", "..hhhhhhh..", "..h.....h..",
    },
    [A_FURADO] = {
        "..Z.Z.Z.Z..", "..ZzZzZzZ..", "...zzzzz...", "..yOOOOOL..",
        "..ZzzzzzE..", ".ZZzzzzzzE.", ".ZzzzzzzzzE", "ZZzzzzzzzzE",
        "ZzzzzzKKzEE", ".zzzzzKKE..", "..EEEE...Ww",
    },
    [A_FERRADURA] = {
        ".SS.....Ss.", "SSs.....sCC", "SKs.....sKC", "SSs.....sCC",
        "SKs.....sKC", "SSs.....sCc", "SSSs...sCCc", ".SKSsssKCc.",
        "..SSssCCc..", "...sCCCc...", "...........",
    },
    [A_PESADO] = {
        "ssssssssC..", "sCCCCCCCCc.", "sCwCCCwCCcc", "sCCCCCCCCcc",
        "sCwCCCwCCcc", "sCCCCCCCCcc", "sCwCCCwCCcc", "sCCCCCCCCcc",
        "Ccccccccccc", ".cccccccccc", "...........",
    },
    [A_RELOGIO] = {
        "....OyL....", ".....o.....", "...yyyyO...", "..yywnwOO..",
        ".yywwKwwOL.", ".ywwwKwwmL.", ".ynwwrKKnL.", ".ywwwwmmmL.",
        ".OOwwmmmLL.", "..OOmnmLL..", "...LLLLL...",
    },
    [A_CARTOLA] = {
        "...EEEEEe..", "...Eeeeee..", "...Eeeeee..", "...Eeeeee..",
        "...Eeeeee..", "...Eeeeee..", "...VvvvvI..", "...IIIIIi..",
        "EEEEEEEEEEe", ".eeeeeeeee.", "...........",
    },
    [A_COROA] = {
        "...........", ".y...y.....", ".O..yOy.yy.", ".OO.OoOOo..",
        ".OOoOoOoO..", ".OOOOOOOOo.", ".OrOOuOOrL.", ".OOOOOOOOL.",
        ".LLLLLLLLl.", "...........", "...........",
    },
    [A_CUPOM] = {
        "...........", "AAAAAAAAAAa", "AWWWWWWWWWa", "AWaaWWWaWWa",
        ".WaaWWaWWW.", ".WWWWaWWWW.", ".WWWaWWaaW.", "AWWaWWWaaWa",
        "AWWWWWWWWWa", "Aaaaaaaaaaa", "...........",
    },
    [A_PRENSA] = {
        "SSSSSSSSSs.", "....Ss.....", "CCCCCCCCCCc", "Scccccccccc",
        "..wwwwwwm..", "..wKwwwwm..", "..wwwKwwm..", "..wwwwwKm..",
        "..mmmmmmm..", "SSSSSSSSSSc", "cc.......cc",
    },
    [A_TACA] = {
        "..yOOOOOL..", "o.yOOOOOL.L", "o.yOOOOOL.L", ".oyOOOOOLL.",
        "...yOOOL...", "....OOL....", ".....o.....", ".....o.....",
        "...yOOOL...", "..LLLLLLL..", "...........",
    },
    [A_CORINGA] = {
        ".wwwwwwwwm.", ".wRwwwwwwm.", ".wwwwrwwwm.", ".wwwwrwwwm.",
        ".wrrrrrrrm.", ".wwrrrrrwm.", ".wwwrrrwwm.", ".wwrrwrrwm.",
        ".wrwwwwwrm.", ".wwwwwwwRm.", ".mmmmmmmmm.",
    },
    [A_DUPLA] = {
        "WWWWWw.....", "WKwwwm.....", "Wwwwwm.....", "WwwwKm.....",
        "Wwwwwm.....", "wmmmKWWWWWw", ".....WKwwwm", ".....Wwwwwm",
        ".....WwwwKm", ".....Wwwwwm", ".....wmmmmm",
    },
    [A_SELO] = {
        "...........", "wwwwwwwwwn.", "wmmmmmmmmn.", "wmzzzzzmmn.",
        "wmmmmmmmmn.", "wmzzzzRRrn.", "wmmmmRPRrQ.", "wmzzzRRrrQ.",
        "nnnnnrrrQQ.", ".....QQQQ..", "...........",
    },
    [A_PEDAGIO] = {
        "Fy.........", "Ff.........", "Zz.........", "ZzwwRRwwRRw",
        "ZzwwrrwwrrW", "ZzQQQQQQQQq", "Zz.........", "Zz.........",
        "Zz.........", "Zz.........", "zEE........",
    },
    [A_DESFORRA] = {
        "........FFF", ".........FF", "F.......FfF", "FF.....Ff.f",
        ".FF...Ff...", "..FF.Ff....", "...FFf.....", "....f......",
        "...........", "...........", "...........",
    },
    [A_MAU_OLHADO] = {
        "....uuu....", "..uuuuuuB..", ".uuuWWWBBB.", ".uuWUUUwBB.",
        "uuWUWbUAwBB", "uuWUbbbAwBB", "uuWUUbAAwBB", ".uBwAAAwBB.",
        ".BBBwwwBBB.", "..BBBBBBB..", "....BBB....",
    },
    [A_CRISANTEMO] = {
        "....O.y....", "..yyOyyOO..", ".O.yOyyO.y.", ".OOyyyOyyO.",
        "yyyOyyoyooo", ".yyyyLoOOO.", "OOOyooooOOO", ".yyyOOOOoo.",
        ".y.OOOoO.o.", "..OoOOoOO..", "....O.o....",
    },
    [A_TRINCA] = {
        ".....A.....", "....UAa....", "...AAaaa...", "....aaa....",
        "....aaa....", "...aa.aa...", "..Aa...aA..", ".UAa...UAa.",
        "AAaaaaAAaaa", ".aaa...aaa.", "..a.....a..",
    },
    [A_PIRAMIDE] = {
        ".....y.....", "....FyF....", "....FyFf...", "...FFyFf...",
        "...FFyfff..", "..FFFyfff..", "..FFFyffff.", ".FFFFyffff.",
        ".FFFFyfffjj", "FFFFFyfjjj.", "jjjjjjjj...",
    },
    [A_ABRE_ALAS] = {
        "..BBBBBB...", "..BbbbbbB..", "UUUUUUbbB..", "UuuuuBbbB..",
        "UuWuuBbbB..", "UuuuuBbB...", "UuuWuB.....", "UuuuuB.UuB.",
        "BBBBBB.uWB.", ".......BBB.", "...........",
    },
    [A_SAIDEIRA] = {
        ".WwWwWwW...", "WWwWwWwWw..", "SwwwwwwmS..", "SyOOOOOoSSS",
        "SyOOyOOoS.S", "SyOOOOOoS.S", "SyOyOOOoS.S", "SyOOOOyoSSS",
        "SyOOOOOoS..", "SoooooooS..", "SSSSSSSSS..",
    },
    [A_CORRIMAO] = {
        "y..........", "OOy........", "L.OOy......", "L...OOy....",
        "AAA.L.OOy..", "aaa.L...OOy", "aaaAAA.L..O", "aaaaaa.L...",
        "aaaaaaAAA..", "aaaaaaaaa..", "aaaaaaaaaAA",
    },
    [A_COLAR] = {
        "W.........W", "m.........m", "W.........W", ".m.......m.",
        ".W.......W.", "..m.....m..", "..WmWmWmW..", ".....m.....",
        "....WWw....", "....Wwm....", ".....m.....",
    },
};

static uint16_t am_tom(char c, bool apagado)
{
    const char *p = strchr(AM_CHAVE, c);
    uint16_t k = p ? AM_COR[p - AM_CHAVE] : C_BARRA;
    if (!apagado) return k;
    int b = brilho_de(k), i = 1 + b * 4 / 256;           // o claro e o escuro, no feltro
    return feltro_tons[i > 4 ? 4 : i];
}

// O simbolo do amuleto a com o centro em (cx, cy), cada pixel do desenho
// em e x e. O contorno e sempre de 1 px na tela, do lado de fora de cada
// beira do desenho; nos cantos de fora ele nao se fecha (o canto aparado).
static void am_simbolo(int cx, int cy, int e, int a, bool apagado)
{
    const char *const *d = AM_DESENHO[a];
    int x0 = cx - AM_PX * e / 2, y0 = cy - AM_PX * e / 2;
#define AM_CHEIO(x, y) ((x) >= 0 && (y) >= 0 && (x) < AM_PX && (y) < AM_PX && d[y][x] != '.')
    for (int y = 0; y < AM_PX; y++)
        for (int x = 0; x < AM_PX; x++) {
            if (!AM_CHEIO(x, y)) continue;
            int px = x0 + x * e, py = y0 + y * e;
            gfx_rect(px, py, e, e, am_tom(d[y][x], apagado));
            if (!AM_CHEIO(x - 1, y)) gfx_rect(px - 1, py, 1, e, C_BARRA);
            if (!AM_CHEIO(x + 1, y)) gfx_rect(px + e, py, 1, e, C_BARRA);
            if (!AM_CHEIO(x, y - 1)) gfx_rect(px, py - 1, e, 1, C_BARRA);
            if (!AM_CHEIO(x, y + 1)) gfx_rect(px, py + e, e, 1, C_BARRA);
        }
#undef AM_CHEIO
}

static void desenha_amuleto(int x, int y, int r, int a, bool apagado)
{
    if (r < 8) { am_simbolo(x, y, 1, a, apagado); return; }
    int aro = r >= 30 ? 5 : r >= 16 ? 3 : 2, ri = r - aro;
    ficha_sombra(x + 1, y + 2, r);
    ficha_disco(x, y, r, r, apagado ? C_FELTRO2 : C_LATAO);     // o aro, com o contorno e a luz
    // O campo, afundado: a beira de cima a esquerda na sombra do aro.
    uint16_t campo = apagado ? C_FELTRO_ESC : tinta_escura(AMULETO[a].cor);
    for (int j = -ri; j <= ri; j++)
        for (int i = -ri; i <= ri; i++) {
            float d = sqrtf((float)(i * i + j * j));
            if (d > ri + 0.5f) continue;
            bool labio = d > ri - (aro > 2 ? 1.5f : 0.7f) && i + j < -ri / 3;
            gfx_pixel(x + i, y + j, labio ? C_BARRA : campo);
        }
    int e = (int)((1.5f * (ri + 1) - 2) / AM_PX);           // o maior degrau que nao invade muito o aro
    am_simbolo(x, y, e < 1 ? 1 : e, a, apagado);
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
        if (am_brilho[a] > 0) {                // o halo de quem agiu: abre e apaga, descendo na rampa do ouro
            static const uint16_t HALO[4] = { C_AMARELO, C_OURO, C_LATAO, C_LATAO_ESC };
            float q = 1.2f - am_brilho[a];
            int rr = 10 + (int)(q * 6), k = (int)(q * 3.3f);
            gfx_disco(cx, y, rr, HALO[k > 3 ? 3 : k]);
        }
        desenha_amuleto(cx, y + respira(n + 2 * j), 8, a, false);   // respira, cada um no seu tempo
        dica_zona(cx - 9, y - 9, 18, 18, DICA_AMULETO, a);
        n++;
    }
}
