// ---------------------------------------------------------------------------
// Tutorial
// ---------------------------------------------------------------------------
typedef struct { const char *titulo; const char *par[3]; } pagina_t;
static const pagina_t TUTORIAL[] = {
    { "O jogo", {
        "Cada jogador começa com 100 fichas. São 15 rodadas; a entrada de cada uma vai para o pote: 5 fichas, 10 a partir da 6ª rodada e 15 a partir da 11ª.",
        "Quem faz mais pontos na rodada leva o pote. No fim, ganha quem tiver mais fichas.",
        NULL } },
    { "A bolsa e a sequência", {
        "Cada jogador tem uma bolsa de dados. A cada rodada, você tira 7 dados dela e escolhe até 4 para a sua sequência, na ordem que quiser.",
        "Depois joga os dados da sequência um por um, do primeiro ao último.",
        NULL } },
    { "A regra da queda", {
        "O primeiro dado sempre vale. Cada dado seguinte precisa ser IGUAL OU MAIOR que o último que vale.",
        "Se sair menor, ele e o último que valia são anulados juntos. Por isso a ordem importa: dados pequenos na frente, grandes no fim.",
        NULL } },
    { "Parar ou arriscar", {
        "Depois de cada dado: jogar o próximo ou parar com os pontos que tem. O botão jogar mostra a chance de cair (some nas dificuldades altas).",
        "Quem venceu a rodada anterior joga primeiro, no escuro. Quem joga depois já sabe o alvo: o botão parar mostra se vence, perde ou empata.",
        NULL } },
    { "Apostas", {
        "Entre os dois turnos, com os pontos de quem jogou primeiro na mesa, dá para apostar 10 ou 25 fichas.",
        "O outro escolhe: pagar para ver, dobrar a aposta ou correr. Quem corre entrega o pote na hora.",
        NULL } },
    { "Loja, prêmios e dados especiais", {
        "A loja do começo vende dados especiais. Quem vence uma rodada escolhe um prêmio: um dado novo ou tirar um fraco da bolsa.",
        "São %d dados, cada um com um poder: veja todos em Dados da casa.",
        NULL } },
    { "Modo Desafiante", {
        "Em 1 jogador: uma run por 3 salões do cassino, cada um com 5 mesas e o chefe. No mapa você escolhe o caminho: oponentes, lojas e eventos.",
        "Cada partida tem 8 rodadas e 50 fichas de cada lado. Venceu: as fichas viram moedas e você ganha um prêmio. Perdeu, a run acaba.",
        "Vencer o Barão abre a dificuldade seguinte, e os chefes abrem dados novos. Cada perfil guarda o seu progresso e a sua run." } },
    { "Teclas", {
        "Setas movem  ·  ENTER escolhe e joga  ·  TAB/CTRL confirma ou para  ·  BACKSPACE tira  ·  I mostra o dado escolhido",
        "Mouse e toque também jogam: clique no que quiser. O < no alto volta ou sai da partida.",
        "X recusa  ·  ESC duas vezes: menu  ·  M som  ·  N música  ·  V velocidade  ·  F11 tela cheia  ·  - e + tamanho da mesa.  Boa sorte!" } },
};
#define N_TUTORIAL ((int)(sizeof TUTORIAL / sizeof TUTORIAL[0]))

// Um botao de mentira nas ilustracoes, aceso ou apagado.
static void rotulo_tut(int cx, int y, const char *s, bool aceso)
{
    int w = gfx_largura(s, 1) + 24;
    ui_botao(cx - w / 2, y, w, 26, s, aceso ? C_VINHO : 0, aceso ? UI_FOCO : 0);
}

// Ilustracoes: dados de verdade, desenhados pelo mesmo codigo da mesa.
static void ilustra_tutorial(int pag, int y)
{
    int cx = GFX_W / 2;
    switch (pag) {
    case 0:                                          // o pote
        for (int i = 0; i < 6; i++) ficha(cx - 60, y + 30 - i * 4, i % 2 ? C_VINHO : C_LATAO);
        for (int i = 0; i < 4; i++) ficha(cx + 60, y + 30 - i * 4, i % 2 ? C_MARFIM_S : C_ROSA);
        txt_c(cx - 60, y + 44, "suas fichas", C_TEXTO_M, false);
        txt_c(cx + 60, y + 44, "pote", C_TEXTO_M, false);
        break;
    case 1: {                                        // a mao e a ordem
        static const uint8_t M[7] = { D_D6, D_D2, D_D6, D_D4, D_D8, D_D6, D_D2 };
        static const int8_t O[7] = { 3, 1, -1, 2, 4, -1, -1 };
        for (int i = 0; i < 7; i++) {
            int x = cx + (i - 3) * 58;
            peca_parada((float)x, (float)y + 26, 19, nova_peca(M[i]), 0);
            if (O[i] > 0) {
                char o[2] = { (char)('0' + O[i]), 0 };
                gfx_disco(x + 17, y + 6, 10, C_OURO);
                gfx_disco(x + 17, y + 6, 8, C_AMARELO);
                txt_c(x + 17, y - 5, o, C_BARRA, true);
            }
        }
        break;
    }
    case 2: {                                        // 2 vale, 4 vale, sai 3: anula o 3 e o 4
        static const int V[3] = { 2, 4, 3 };
        int passo = (int)(t_fase / 1.1f) % 4;        // 0: so o 2 ... 3: a queda
        for (int i = 0; i < 3 && i <= passo; i++) {
            int x = cx + (i - 1) * 110;
            bool anul = passo == 3 && i > 0;
            desenha_dado((float)x, (float)y + 22, 22, D_D6, 0, 0, V[i], 2, anul);
            const char *r = anul ? (i == 2 ? "menor: anula" : "anulado") : "vale";
            txt_c(x, y + 52, r, anul ? C_VINHO_CLR : C_OURO, false);
        }
        break;
    }
    case 3: {                                        // dois dados valendo; jogar ou parar
        desenha_dado((float)cx - 214, (float)y + 22, 20, D_D6, 0, 0, 2, 2, false);
        desenha_dado((float)cx - 162, (float)y + 22, 20, D_D6, 0, 0, 4, 2, false);
        txt_c(cx - 188, y + 46, "6 pontos", C_OURO, false);
        bool parar = (int)(t_fase / 1.4f) % 2;
        rotulo_tut(cx - 6, y + 10, "jogar: 50% de cair", !parar);
        rotulo_tut(cx + 170, y + 10, "parar: vence por 1", parar);
        break;
    }
    case 4: {                                        // aposta na mesa; pagar, dobrar ou correr
        for (int i = 0; i < 5; i++) ficha(cx - 190, y + 30 - i * 4, i % 2 ? C_VINHO : C_LATAO);
        txt_c(cx - 190, y + 44, "aposta 25", C_TEXTO_M, false);
        static const char *const R[3] = { "pagar", "dobrar", "correr" };
        int k = (int)(t_fase / 1.2f) % 3;
        for (int i = 0; i < 3; i++) rotulo_tut(cx - 50 + i * 110, y + 10, R[i], i == k);
        break;
    }
    case 5: {                                        // alguns dados especiais
        static const uint8_t E[5] = { D_ESCUDO, D_JACKPOT, D_EGOISTA, D_COPIADOR, D_FOGO };
        for (int i = 0; i < 5; i++) {
            int x = cx + (i - 2) * 100;
            float bob = sinf(t_fase * 3 + i) * 2;
            peca_parada((float)x, y + 24 + bob, 19, nova_peca(E[i]), 0);
            txt_c(x, y + 46, TIPO[E[i]].nome, cor_dado(E[i]), false);
        }
        break;
    }
    default: break;
    }
}

static void desenha_tutorial(void)
{
    char s[32];
    snprintf(s, sizeof s, "%d de %d", tut_pag + 1, N_TUTORIAL);
    faixas_menu("COMO JOGAR", s, tut_pag + 1 < N_TUTORIAL
                ? "< anterior  ·  ENTER ou > próxima  ·  ESC menu"
                : "< anterior  ·  ENTER volta ao menu");
    gfx_rect_alfa(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, C_SOMBRA, 110);
    const pagina_t *pg = &TUTORIAL[tut_pag];
    txt_sombra_c(GFX_W / 2, MESA_Y0 + 8, pg->titulo, C_LATAO, 2);
    int y = MESA_Y0 + 52;
    for (int i = 0; i < 3 && pg->par[i]; i++) {
        char par[256];                               // "%d": quantos dados ha
        snprintf(par, sizeof par, pg->par[i], D_BASICOS);
        int n = quebra_linhas(44, y, 552, 4, par, i == 0 ? C_MARFIM : C_MARFIM_S);
        y += n * LINHA_FINA + 8;
    }
    // A ilustracao vai abaixo do texto: na faixa de sempre ou, se o texto
    // desceu mais, logo depois dele.
    int yi = y + 10 > 226 ? y + 10 : 226;
    if (yi > 234) yi = 234;                          // sem encostar nas bolinhas
    ilustra_tutorial(tut_pag, yi);
    // Bolinhas das paginas.
    for (int i = 0; i < N_TUTORIAL; i++)
        gfx_disco(GFX_W / 2 + (i - N_TUTORIAL / 2) * 16, MESA_Y1 - 6, 3,
                  i == tut_pag ? C_OURO : C_FELTRO2);
}
