// ---------------------------------------------------------------------------
// Falas: um balao curto saindo do retrato do oponente
// ---------------------------------------------------------------------------

// Os chefes reparam no andar: a fala de abertura muda a cada dificuldade
// (do 1o andar a Cobertura), e o Barao se despede de outro jeito la em cima.
static const char *const FALA_ANDAR[3][5] = {            // [chefe][dificuldade - 1]
    { "1º andar, senhor? A casa reparou na sua subida.", "2º andar. Poucos chegam a esta mesa.",
      "No 3º andar, a casa não dá desconto.", "No 4º andar, até a casa joga sério.",
      "Cobertura. A casa nunca serviu ninguém aqui." },
    { "O térreo ficou pequeno para você?", "2º andar. Você está subindo rápido.",
      "3º andar... alguém andou se esforçando.", "No 4º andar, eu uso minhas melhores joias.",
      "Aqui na cobertura, o salão continua meu." },
    { "1º andar. É um começo.", "2º andar. Começo a lembrar do seu nome.",
      "3º andar. Agora você me interessa.", "4º andar... ninguém sobe tanto por sorte.",
      "Bem-vindo à cobertura. Daqui, só se desce." },
};

// As viradas (veja virada_forca no dado.c): o anuncio, a frase curta do
// mapa e o que o chefe diz ao virar e depois.
static const char *const VIRADA_TITULO[3] = { "A CASA TOMA VANTAGEM", "A DONA COBRA O ALUGUEL",
                                              "O BARÃO PERDE A PACIÊNCIA" };
static const char *const VIRADA_FRASE[3] = { "A casa sempre tem vantagem.", "Agora você paga o aluguel.",
                                             "Chega de brincadeira." };
static const char *const FALA_VIROU[3][2][2] = {        // [chefe][ganhou, perdeu a rodada][2]
    { { "A casa recolhe. Sempre.", "Como manda a casa." }, { "A noite ainda não acabou.", "Anotado." } },
    { { "Mais um mês de aluguel.", "Meu salão, minhas regras." }, { "Isso vai para a conta.", "Não se anime." } },
    { { "Hmpf. Como deveria ser.", "Eu avisei." }, { "Isso não vai se repetir!", "Inaceitável!" } },
};
static void virada_txt(int c, int forca, bool curto, char *s, int n)
{
    if (c == 0) snprintf(s, (size_t)n, "%s", curto ? (forca == 2 ? "+1 em cada dado dele; você compra 1 a menos" : "+1 em cada dado dele")
                         : (forca == 2 ? "Cada dado dele vale +1, e você compra 1 dado a menos." : "Cada dado dele vale +1."));
    else if (c == 1) snprintf(s, (size_t)n, curto ? "aluguel: %d fichas suas por rodada"
                              : "No fim de cada rodada, ela leva %d fichas suas.", forca == 2 ? 5 : 3);
    else snprintf(s, (size_t)n, "%s", curto ? (forca == 2 ? "+3 por queda sua; 1º e 2º dado dele x2" : "+3 por queda sua; 1º dado dele x2")
                  : (forca == 2 ? "Cada dado seu que cair dá +3 a ele; os dois primeiros dele valem x2."
                                : "Cada dado seu que cair dá +3 a ele, e o primeiro dele vale x2."));
}

static int des_dificuldade(void);

static void fala_evento(int cat)
{
    if (modo != M_DESAFIO || fala_perso < 0 || cat < 0 || cat >= FA_N) return;
    // Nem tudo tem resposta: o comeco e o fim sempre; o resto, as vezes.
    bool sempre = cat == FA_INICIO || cat == FA_GANHA_P || cat == FA_PERDE_P;
    if (!sempre && (rnd_v() % 100) >= 55) return;
    if (!sempre && t_fala > 0.6f) return;               // nao atropela a fala anterior
    const char *s = PERSO[fala_perso].fala[cat][rnd_v() & 1];
    int dif = des_dificuldade();
    if (perso_chefe(fala_perso) && dif > 0 && cat == FA_INICIO)
        s = FALA_ANDAR[fala_perso - PERSO_CHEFE0][dif - 1];
    if (fala_perso == perso_virado && (cat == FA_GANHA_R || cat == FA_PERDE_R))
        s = FALA_VIROU[fala_perso - PERSO_CHEFE0][cat == FA_GANHA_R ? 0 : 1][rnd_v() & 1];
    if (fala_perso == PERSO_CHEFE0 + 2 && dif == 5 && cat == FA_PERDE_P)
        s = "A cobertura é sua. Eu desço pela escada.";
    snprintf(fala_txt, sizeof fala_txt, "%s", s);
    t_fala = 3.2f;
}
