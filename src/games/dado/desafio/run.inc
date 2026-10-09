// ===========================================================================
// A run: andares, nos do mapa, eventos e os oponentes
// ===========================================================================
#define DES_ANDARES   3
#define DES_PASSOS    6                  // 5 mesas e o chefe
#define DES_FICHAS    50


#define DES_MOEDAS    50                 // moedas no comeco da run, no terreo
#define DES_MOEDAS_ANDAR 25              // e mais estas a cada andar acima

enum { NO_FACIL, NO_MEDIO, NO_DIFICIL, NO_CHEFE, NO_LOJA, NO_EVENTO, NO_VAZIO = 0xFF };
enum { R_LOJA_INI, R_MAPA, R_PARTIDA, R_PREMIO, R_LOJA, R_AMULETO, R_EVENTO };
// Eventos do mapa (no no, o campo perso guarda qual).
enum { EVT_LAPIDADOR, EVT_CAPELA, EVT_NIQUEL, EVT_TROCADOR, EVT_FONTE, EVT_GARCOM, EVT_ESQUECIDO,
       EVT_COPISTA, EVT_N };
typedef struct { const char *nome, *cena, *resumo; } evt_t;
static const evt_t EVT[EVT_N] = {
    [EVT_LAPIDADOR] = { "O Lapidador", "Bancada de lapidação, lupa e lixas.",
                        "Um dado simples sobe um degrau: moeda vira D4, D4 vira D6, D6 vira D8, D8 vira D12. Só uma vez por run." },
    [EVT_CAPELA]    = { "A Capela", "Uma capela pequena no corredor do cassino.",
                        "Tira até 2 dados da sua bolsa, de graça." },
    [EVT_NIQUEL]    = { "Caça-níquel", "Uma máquina velha num canto, ainda ligada.",
                        "15 moedas por puxada. Três iguais: dado raro, 200 moedas ou nada." },
    [EVT_TROCADOR]  = { "O Trocador", "Um homem de chapéu com o paletó cheio de dados.",
                        "Troca um dado seu por um sorteado da raridade de cima." },
    [EVT_FONTE]     = { "Fonte dos Desejos", "Moedas brilham no fundo da água.",
                        "Jogue moedas: quanto mais, maior a chance de sair um amuleto." },
    [EVT_GARCOM]    = { "O Garçom", "Ele serve a próxima mesa e aceita um agrado.",
                        "Por 50 moedas, o próximo oponente começa com 10 fichas a menos." },
    [EVT_ESQUECIDO] = { "Dado Esquecido", "Alguém deixou um dado na mesa vazia.",
                        "Pegue um dado comum, ou arrisque: raro ou Quebrado." },
    [EVT_COPISTA]   = { "O Copista", "Ele copia qualquer dado, menos os lendários.",
                        "Por 70 moedas, faz uma cópia de um dado da sua bolsa." },
};
static void abre_evento(void);


// Os oponentes do cassino. Cada um tem um jeito de apostar, dados que
// prefere, um trunfo (regra so dele, que vale a partida toda) e o que diz.
typedef struct {
    const char *nome, *frase;
    float ousadia;                       // -1 cauteloso ... +1 aposta alto
    uint8_t fav[5];                      // dados preferidos (0xFF = fim)
    const char *trunfo, *regra;          // nome do trunfo e o que ele faz
    uint32_t amul;                       // trunfo feito de amuletos
    uint8_t reg;                         // ou regra propria (REG_*)
    const char *fala[FA_N][2];
} perso_t;

#define AM(a) (1u << (a))
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
      "Pote quente", "Vencendo uma rodada, leva 2 fichas a mais.", AM(A_POTE), REG_NADA,
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
      "Dona da casa", "Começa com 10 fichas a mais, e você compra só 6 dados por rodada.",
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
    // Mais oponentes comuns (vem depois dos chefes para nao mudar os mapas ja salvos).
    { "O ILUSIONISTA",   "Nada é o que parece.",               0.2f,
      { D_ESPELHO, D_COPIADOR, D_VENTANIA, D_PAR, D_TUDO_NADA },
      "Truque duplo", "Dado dele que repete o número de outro dele: +2.", AM(A_DUPLA), REG_NADA,
      { { "Nada na manga. Nada na mão.", "Escolha um dado... qualquer dado." },
        { "Puf! Desapareceu.", "E o seu ponto... sumiu." },
        { "Tcharam!", "A mão é mais rápida que o olho." },
        { "Faz parte do número.", "Um truque falhou. Tenho outros." },
        { "E para o grand finale...", "Aplausos, por favor." },
        { "Bravo! Você viu o truque.", "Um mágico nunca explica a derrota." } } },
    { "O CAUBÓI",        "Atira primeiro, pergunta depois.",   0.6f,
      { D_LASER, D_EXPLOSIVO, D_DESAFIANTE, D_D12, D_VICIADO },
      "Saque rápido", "O primeiro dado da sequência dele vale +1.", 0, REG_SAQUE,
      { { "Esta mesa é pequena pra nós dois.", "Saque quando estiver pronto." },
        { "Caiu do cavalo, parceiro.", "Poeira. Só sobrou poeira." },
        { "Na mosca.", "O mais rápido do oeste." },
        { "Errei o tiro.", "Hm. Vento contrário." },
        { "Volte pro rancho, forasteiro.", "Pode guardar o revólver." },
        { "Você saca rápido, forasteiro.", "Tiro limpo. Respeito." } } },
    { "O MONGE",         "Não aposta contra o destino.",      -0.6f,
      { D_SENTINELA, D_SOLITARIO, D_MISERICORDIOSO, D_TEIMOSO, D_D8 },
      "Equilíbrio", "Uma vez por rodada, na primeira queda que nada mais segurou, o dado de antes fica.", AM(A_FERRADURA), REG_NADA,
      { { "Respire. Os dados também respiram.", "Que a mesa nos ensine algo." },
        { "A pressa derruba o que ergue.", "Tudo que sobe, cai." },
        { "Assim é.", "O rio segue o seu curso." },
        { "Também isso passará.", "Uma lição, não uma perda." },
        { "Não há vencedor. Só o caminho.", "Medite sobre isso." },
        { "Você está em paz com a sorte.", "O aluno superou o mestre." } } },
    { "A DETETIVE",      "Já sabe o que você vai jogar.",      0.0f,
      { D_INVEJOSO, D_COBRADOR, D_MARTELO, D_D12, D_D8 },
      "Última palavra", "Na 1ª rodada e quando está atrás em fichas, ela joga por último.", 0, REG_ULTIMA,
      { { "Pode começar. Eu observo.", "Cada jogada deixa uma pista." },
        { "Previsível. Anotei.", "Elementar." },
        { "Caso encerrado.", "As pistas não mentem." },
        { "Uma pista falsa. Curioso.", "Isso não estava no relatório." },
        { "Seu padrão te entregou.", "Investigação concluída." },
        { "Você me despistou. Bom trabalho.", "Vou reabrir o caso." } } },
    { "A MARATONISTA",   "Começa devagar e termina voando.",   0.3f,
      { D_ACUMULADOR, D_SEMENTE, D_TEIMOSO, D_FARTURA, D_D8 },
      "Segundo fôlego", "Da 5ª rodada em diante, os dados dela valem +1.", 0, REG_FOLEGO,
      { { "É maratona, não corrida.", "Vou no meu ritmo." },
        { "Tropeçou na largada?", "Fôlego curto, hein." },
        { "Mantendo o ritmo.", "Um passo de cada vez." },
        { "Só aquecendo.", "Ainda tenho fôlego." },
        { "Linha de chegada!", "Treino é tudo." },
        { "Você correu bem. Vou treinar.", "Cãibra. Só pode ser." } } },
};
#define N_PERSO_COMUM 8                  // os oito primeiros...
#define PERSO_NOVOS   11                 // ...e os comuns que vieram depois
#define N_PERSO       ((int)(sizeof PERSO / sizeof PERSO[0]))
static bool perso_chefe(int p) { return p >= PERSO_CHEFE0 && p < PERSO_CHEFE0 + DES_ANDARES; }
