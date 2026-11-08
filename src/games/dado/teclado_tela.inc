// ---------------------------------------------------------------------------
// O teclado na tela: com o controle, o nome do perfil e o codigo da sala se
// escrevem escolhendo as letras com o direcional e o A (ENTER). So aparece
// com o controle em uso (controle_estilo): no teclado de verdade e no toque,
// fica o de sempre. O B (BACKSPACE) apaga e o Y (TAB) confirma, como sempre.
//
// A grade: duas linhas de letras (13 cada), a dos numeros e, quando o nome
// aceita, o espaco no fim dela (largo, ocupa as colunas 10 a 12). Embaixo
// do teclado, o botao de confirmar (a linha TT_LINHAS); em cima, a tela pode
// ter mais uma linha (o retrato do perfil: a linha -1).
// ---------------------------------------------------------------------------
#define TT_COLS   13
#define TT_LINHAS 3
#define TT_CW     22                     // a tecla
#define TT_CH     20
#define TT_PASSO  24                     // a tecla e o vao
#define TT_ALTO   (TT_LINHAS * TT_PASSO - 4)
static const char *const TT_LETRAS[2] = { "ABCDEFGHIJKLM", "NOPQRSTUVWXYZ" };
static int tt_lin, tt_col;               // a tecla em foco (na linha dos numeros, a coluna 10 ou mais e o espaco)

static bool tt_ativo(void) { return controle_estilo != CONTROLE_NENHUM; }
static void tt_zera(void) { tt_lin = 0; tt_col = 0; }

// O caractere da tecla (lin, col); 0: nenhuma.
static char tt_char(int lin, int col, bool espaco)
{
    if (lin < 0 || lin >= TT_LINHAS || col < 0 || col >= TT_COLS) return 0;
    if (lin < 2) return TT_LETRAS[lin][col];
    if (col < 10) return (char)('0' + col);
    return espaco ? ' ' : 0;
}

// A ultima coluna que a linha tem.
static int tt_ultima(int lin, bool espaco) { return lin == 2 ? (espaco ? 10 : 9) : TT_COLS - 1; }

// O direcional dentro do teclado. lin_min: -1 se a tela tem a linha de cima.
// Devolve se a tecla era do direcional.
static bool tt_anda(int k, int lin_min, bool espaco)
{
    if (k == KEY_UP) { if (tt_lin > lin_min) tt_lin--; }
    else if (k == KEY_DOWN) { if (tt_lin < TT_LINHAS) tt_lin++; }
    else if ((k == KEY_LEFT || k == KEY_RIGHT) && tt_lin >= 0 && tt_lin < TT_LINHAS) {
        int u = tt_ultima(tt_lin, espaco), c = tt_col > u ? u : tt_col;
        c = k == KEY_LEFT ? (c > 0 ? c - 1 : u) : (c < u ? c + 1 : 0);
        tt_col = tt_lin == 2 && c == 10 ? 11 : c;   // o espaco: a coluna do meio dele
    } else return false;
    if (tt_lin == 2 && tt_col > tt_ultima(2, espaco)) tt_col = espaco ? 11 : 9;
    som_toca(SOM_TIQUE);
    return true;
}

// O caractere da tecla em foco (0: o foco nao esta numa tecla).
static char tt_aperta(bool espaco)
{
    int c = tt_lin == 2 && tt_col >= 10 ? 10 : tt_col;
    return tt_char(tt_lin, c, espaco);
}

// A grade, com o topo em y.
static void tt_desenha(int y, bool espaco)
{
    int x0 = GFX_W / 2 - (TT_COLS * TT_PASSO - 4) / 2;
    for (int lin = 0; lin < TT_LINHAS; lin++)
        for (int c = 0; c <= tt_ultima(lin, espaco); c++) {
            bool esp = lin == 2 && c == 10;
            int x = x0 + c * TT_PASSO, w = esp ? 3 * TT_PASSO - 4 : TT_CW;
            bool foco = tt_lin == lin && (esp ? tt_col >= 10 : tt_col == c);
            char s[2] = { tt_char(lin, c, espaco), 0 };
            ui_botao(x, y + lin * TT_PASSO, w, TT_CH, esp ? "espaço" : s, 0, foco ? UI_FOCO : 0);
        }
}
