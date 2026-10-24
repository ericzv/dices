// ---------------------------------------------------------------------------
// Falas: um balao curto saindo do retrato do oponente
// ---------------------------------------------------------------------------

// Os chefes reparam no andar: a fala de abertura muda a cada dificuldade
// (do 1o andar a Cobertura), e o Barao se despede de outro jeito la em cima.
static const char *const FALA_ANDAR[3][N_NIVEIS] = {     // [chefe][dificuldade - 1]
    { "1º andar, senhor? A casa reparou na sua subida.", "2º andar. Poucos chegam a esta mesa.",
      "No 3º andar, a casa não dá desconto.", "4º andar. Aqui a casa conta as fichas duas vezes.",
      "5º andar. Metade do caminho, e a casa ainda sorri.", "No 6º andar, os dados são mais pesados.",
      "7º andar. A casa abriu o cofre para você.", "No 8º andar, até a casa joga sério.",
      "9º andar. A casa já não dorme quando você sobe.", "Cobertura. A casa nunca serviu ninguém aqui." },
    { "O térreo ficou pequeno para você?", "2º andar. Você está subindo rápido.",
      "3º andar... alguém andou se esforçando.", "4º andar. Já ouvi falar de você nos corredores.",
      "5º andar. Meus clientes começam a apostar em você.", "6º andar. Trouxe o meu melhor colar.",
      "7º andar. Aqui o aluguel é outro.", "No 8º andar, eu uso minhas melhores joias.",
      "9º andar. Nunca recebi ninguém tão alto.", "Aqui na cobertura, o salão continua meu." },
    { "1º andar. É um começo.", "2º andar. Começo a lembrar do seu nome.",
      "3º andar. Já sei como você aposta.", "4º andar. Agora você me interessa.",
      "5º andar. Mandei preparar uma mesa melhor.", "6º andar. Os meus sócios querem conhecer você.",
      "7º andar. Isso já não é sorte.", "8º andar... ninguém sobe tanto por sorte.",
      "9º andar. Só falta um, e ele é meu.", "Bem-vindo à cobertura. Daqui, só se desce." },
};

// As viradas (veja virada_forca no dado.c): o anuncio, a frase curta do
// mapa e o que o chefe diz ao virar e depois.
static const char *const VIRADA_TITULO[3] = { "A CASA TOMA VANTAGEM", "A DONA COBRA O ALUGUEL",
                                              "O BARÃO PERDE A PACIÊNCIA" };
static const char *const VIRADA_FRASE[3] = { "A casa sempre tem vantagem.", "Agora você paga o aluguel.",
                                             "Chega de brincadeira." };
// O Papai Noel vira como o Barao (o lugar e o mesmo), com as palavras dele.
static const char *const NOEL_TITULO = "O PAPAI NOEL PERDE O BOM HUMOR";
static const char *const NOEL_FRASE = "Ho. Ho. Ho. Agora é sério.";
static const char *const NOEL_VIROU[2][2] = { { "Ho-ho. Como deveria ser.", "Carvão. Muito carvão." },
                                              { "Isso não estava na lista!", "Nem as renas acreditam!" } };
static const char *virada_titulo(int perso) { return perso == PERSO_NOEL ? NOEL_TITULO : VIRADA_TITULO[chefe_idx(perso)]; }
static const char *virada_frase(int perso) { return perso == PERSO_NOEL ? NOEL_FRASE : VIRADA_FRASE[chefe_idx(perso)]; }
static const char *const FALA_VIROU[3][2][2] = {        // [chefe][ganhou, perdeu a rodada][2]
    { { "A casa recolhe. Sempre.", "Como manda a casa." }, { "A noite ainda não acabou.", "Anotado." } },
    { { "Mais um mês de aluguel.", "Meu salão, minhas regras." }, { "Isso vai para a conta.", "Não se anime." } },
    { { "Hmpf. Como deveria ser.", "Eu avisei." }, { "Isso não vai se repetir!", "Inaceitável!" } },
};
static void virada_txt(int c, int forca, bool curto, char *s, int n)
{
    if (c == 0) snprintf(s, (size_t)n, "%s", curto ? (forca == 2 ? "+1 em cada dado dele; você compra 1 a menos" : "+1 em cada dado dele")
                         : (forca == 2 ? "Cada dado dele pontua +1, e você compra 1 dado a menos." : "Cada dado dele pontua +1."));
    else if (c == 1) snprintf(s, (size_t)n, curto ? "aluguel: %d fichas suas por rodada"
                              : "No fim de cada rodada, ela leva %d fichas suas.", forca == 2 ? 5 : 3);
    else snprintf(s, (size_t)n, "%s", curto ? (forca == 2 ? "+3 por dado seu anulado; 1º e 2º dado dele x2" : "+3 por dado seu anulado; 1º dado dele x2")
                  : (forca == 2 ? "Cada dado seu que for anulado dá +3 a ele; os dois primeiros dele pontuam x2."
                                : "Cada dado seu que for anulado dá +3 a ele, e o primeiro dele pontua x2."));
}

static int des_dificuldade(void);

static void fala_evento(int cat)
{
    if (modo != M_DESAFIO || fala_perso < 0 || cat < 0 || cat >= FA_N) return;
    // Nem tudo tem resposta: o comeco e o fim sempre; o resto, as vezes.
    bool sempre = cat == FA_INICIO || cat == FA_GANHA_P || cat == FA_PERDE_P;
    if (!sempre && (rnd_v() % 100) >= 55) return;
    if (!sempre && t_fala > 0.6f && !fala_e_curta) return;   // nao atropela a fala anterior
    const char *s = PERSO[fala_perso].fala[cat][rnd_v() & 1];
    int dif = des_dificuldade();
    bool noel = fala_perso == PERSO_NOEL;
    if (perso_chefe(fala_perso) && !noel && dif > 0 && cat == FA_INICIO)
        s = FALA_ANDAR[chefe_idx(fala_perso)][dif - 1];
    if (fala_perso == perso_virado && (cat == FA_GANHA_R || cat == FA_PERDE_R))
        s = noel ? NOEL_VIROU[cat == FA_GANHA_R ? 0 : 1][rnd_v() & 1]
                 : FALA_VIROU[chefe_idx(fala_perso)][cat == FA_GANHA_R ? 0 : 1][rnd_v() & 1];
    if (chefe_idx(fala_perso) == 2 && dif >= CORINGA_DESDE && cat == FA_PERDE_P)   // o Coringa vem depois
        s = noel ? "Ho-ho... ouço alguém rindo lá em cima. Não é rena."
                 : dif == N_NIVEIS ? "A cobertura é sua... mas lá em cima alguém está rindo."
                                   : "Você me venceu... mas ouço alguém rindo lá em cima.";
    snprintf(fala_txt, sizeof fala_txt, "%s", s);
    fala_comeca(3.2f);
    fala_e_curta = false;
}

// ---------------------------------------------------------------------------
// Reacoes: o rosto do oponente muda por um instante quando algo acontece na
// mesa - quase sempre so o rosto; as vezes, uma palavra. Cada um reage pelo
// temperamento; os tres chefes tem as palavras deles.
// ---------------------------------------------------------------------------
// As expressoes do rosto (retratos.inc): as quatro de sempre e as de reacao.
enum { EX_CALMA, EX_BRAVA, EX_ALEGRE, EX_PREOCUPADA, EX_SURPRESA, EX_DESDEM, EX_SERIA };
enum { TP_TIMIDO, TP_FRIO, TP_ESQUENTADO, TP_ALEGRE, TP_N };
static const uint8_t TEMPERAMENTO[] = {
    TP_TIMIDO, TP_TIMIDO, TP_ALEGRE, TP_ALEGRE,               // Novato, Cautelosa, Apostador, Ladrao
    TP_FRIO, TP_ESQUENTADO, TP_ALEGRE, TP_ESQUENTADO,         // Matematica, Espinhento, Supersticiosa, Brutamontes
    TP_FRIO, TP_FRIO, TP_FRIO, TP_FRIO,                       // Crupie, Dona do Salao, Barao, Ilusionista
    TP_ESQUENTADO, TP_TIMIDO, TP_FRIO, TP_ALEGRE,             // Caubói, Monge, Detetive, Maratonista
    TP_ESQUENTADO, TP_FRIO, TP_ALEGRE, TP_FRIO,               // Rival, Bruxa, Maluco, Banqueira
    TP_FRIO, TP_ALEGRE,                                       // Coveiro, Coringa
    TP_ALEGRE, TP_ESQUENTADO, TP_ALEGRE, TP_TIMIDO,           // Papai Noel, Ranzinza, Duende, Rena
};
_Static_assert(sizeof TEMPERAMENTO == PERSO_RENA + 1, "um temperamento por oponente");
// [temperamento][momento]: voce passou ele, ele passou voce, o dado dele
// caiu, o seu caiu, o seu combo, ele te atacou, a hora da verdade, voce
// apostou, voce correu, ele levou a rodada, ele perdeu.
static const uint8_t REACAO[TP_N][RM_N] = {
    { EX_PREOCUPADA, EX_ALEGRE, EX_PREOCUPADA, EX_ALEGRE, EX_SURPRESA, EX_ALEGRE, EX_PREOCUPADA,
      EX_PREOCUPADA, EX_ALEGRE, EX_ALEGRE, EX_PREOCUPADA },
    { EX_BRAVA, EX_DESDEM, EX_BRAVA, EX_DESDEM, EX_SURPRESA, EX_DESDEM, EX_SERIA,
      EX_DESDEM, EX_DESDEM, EX_DESDEM, EX_BRAVA },
    { EX_BRAVA, EX_ALEGRE, EX_BRAVA, EX_ALEGRE, EX_BRAVA, EX_ALEGRE, EX_BRAVA,
      EX_BRAVA, EX_ALEGRE, EX_ALEGRE, EX_BRAVA },
    { EX_SURPRESA, EX_ALEGRE, EX_SURPRESA, EX_ALEGRE, EX_SURPRESA, EX_ALEGRE, EX_PREOCUPADA,
      EX_SURPRESA, EX_ALEGRE, EX_ALEGRE, EX_PREOCUPADA },
};
// As palavras: so em alguns momentos (passou, caiu o dele, combo, ataque e
// aposta); NULL fica so no rosto.
static const char *const CURTA[TP_N][RM_N][2] = {
    [TP_TIMIDO] = { [RM_PASSOU] = { "Ah, não...", "Eita." }, [RM_CAI_DELE] = { "Ai.", "Puxa..." },
                    [RM_COMBO] = { "Nossa!", "Uau..." }, [RM_ATACOU] = { "Desculpa!", "Foi sem querer!" },
                    [RM_APOSTA] = { "Tudo isso?", "Hã..." } },
    [TP_FRIO] = { [RM_PASSOU] = { "Hm.", "Interessante." }, [RM_CAI_DELE] = { "Irrelevante.", "Hm." },
                  [RM_COMBO] = { "Ora, ora.", "Notável." }, [RM_ATACOU] = { "Previsível.", "Agradeço." },
                  [RM_APOSTA] = { "Que ousadia.", "Veremos." } },
    [TP_ESQUENTADO] = { [RM_PASSOU] = { "Grr!", "Ah, é?" }, [RM_CAI_DELE] = { "Argh!", "Maldição!" },
                        [RM_COMBO] = { "Sorte!", "Tá de brincadeira!" }, [RM_ATACOU] = { "Toma!", "É meu!" },
                        [RM_APOSTA] = { "Pode vir!", "Vem!" } },
    [TP_ALEGRE] = { [RM_PASSOU] = { "Opa!", "Boa!" }, [RM_CAI_DELE] = { "Ops!", "Ih, rapaz." },
                    [RM_COMBO] = { "Que lance!", "Bonito!" }, [RM_ATACOU] = { "Valeu!", "Hehe." },
                    [RM_APOSTA] = { "Gostei!", "Agora sim!" } },
};
static const char *const CURTA_CHEFE[3][3][2] = {     // [chefe][passou, caiu o dele, combo]
    { { "A casa anota.", "Hm." }, { "Acontece.", "Hm." }, { "A casa observa.", "Bem jogado." } },
    { { "Que ousadia.", "Hmpf." }, { "Hmpf.", "Que deselegante." }, { "No meu salão?", "Que ousadia!" } },
    { { "Hmpf.", "Insolente." }, { "Inadmissível.", "Hmpf!" }, { "Insolente!", "Impossível!" } },
};

static void rival_reage(int m)
{
    if (modo != M_DESAFIO || fala_perso < 0 || fala_perso >= (int)sizeof TEMPERAMENTO || m < 0 || m >= RM_N) return;
    int tp = TEMPERAMENTO[fala_perso];
    reacao_expr = REACAO[tp][m];
    t_reacao = m == RM_HORA ? 6.0f : 1.8f;               // na hora da verdade, ate o veredito
    // A palavra: as vezes, com folga entre uma e outra, nunca por cima de
    // uma fala de verdade.
    const char *const *op = CURTA[tp][m];
    int c = fala_perso == PERSO_NOEL ? -1 : chefe_idx(fala_perso);   // o Noel reage como os alegres
    if (c >= 0 && (m == RM_PASSOU || m == RM_CAI_DELE || m == RM_COMBO))
        op = CURTA_CHEFE[c][m == RM_PASSOU ? 0 : m == RM_CAI_DELE ? 1 : 2];
    if (!op[0] || t_curta > 0 || (t_fala > 0 && !fala_e_curta) || rnd_v() % 100 >= 40) return;
    snprintf(fala_txt, sizeof fala_txt, "%s", op[rnd_v() & 1]);
    fala_comeca(1.4f);
    fala_e_curta = true;
    t_curta = 6.0f;
}

// O dado k de j pousou: quem passou quem, quem caiu, a fila cheia.
static void rival_lance(int j, int k)
{
    if (modo != M_DESAFIO || fala_perso < 0) return;
    desfecho_t d;
    calcula_desfecho(&d, false);
    int a = j, b = outro(j);
    bool passou = (j != primeiro) && placar_antes[a] <= placar_antes[b] && d.total[a] > d.total[b];
    int seguidos = 0;
    while (seguidos <= k && F[j].est[seguidos] == V_VALIDO) seguidos++;
    if (j == 0 && seguidos == N_FILA) rival_reage(RM_COMBO);
    else if (passou) rival_reage(j == 0 ? RM_PASSOU : RM_ELE_PASSOU);
    else if (ver_tipo == 1 || ver_tipo >= 3) rival_reage(j == 0 ? RM_CAI_SEU : RM_CAI_DELE);   // caiu, zerou, o Egoista
}

// A fala sai letra por letra: o tanto de fala_txt ja dito (contado em letras,
// nao em bytes, por causa dos acentos), rapido o bastante para terminar com
// folga antes de o balao sumir.
static const char *fala_dita_txt(void)
{
    static char s[sizeof fala_txt];
    int n = 0;
    for (const char *p = fala_txt; *p; p++) n += (*p & 0xC0) != 0x80;
    float lps = 45, folga = fala_dur > 0 ? n / (0.4f * fala_dur) : lps;   // letras por segundo
    if (folga > lps) lps = folga;
    int ver = (int)(fala_dita * lps) + 1, i = 0, l = 0;
    for (; fala_txt[i]; i++) {
        if ((fala_txt[i] & 0xC0) != 0x80 && l++ == ver) break;
        s[i] = fala_txt[i];
    }
    s[i] = 0;
    return s;
}

// A boca do retrato mexe enquanto as letras saem.
static bool fala_digitando(void)
{
    return t_fala > 0.3f && strlen(fala_dita_txt()) < strlen(fala_txt);
}
