// ===========================================================================
// A run: saloes, as mesas de cada um e os oponentes
// ===========================================================================
#define DES_ANDARES   3
#define DES_ANDAR_FINAL 3               // o salao do Coringa: depois do Barao, so no ultimo nivel
#define DES_PASSOS    4                  // 3 oponentes e o chefe, sempre nessa ordem
#define DES_FICHAS    50


#define N_NIVEIS      10                 // dificuldades acima da normal (andares do cassino)
#define N_NIVEIS_V    5                  // os perfis de antes tinham 5: o nivel n de la e o 2n daqui
#define DES_MOEDAS    4                  // moedas no comeco da run: a loja da entrada ja vende
#define DES_MOEDAS_NIVEL 2               // e mais 1 a cada 2 niveis (os oponentes la de cima sao mais duros)

// Moedas por mesa vencida: [salao][1o, 2o, 3o oponente, chefe]. O Barao so
// paga na Cobertura (nos outros niveis a run acaba nele); o Coringa fecha a run.
static const uint8_t MOEDAS_MESA[DES_ANDARES][DES_PASSOS] = { { 2, 3, 4, 5 }, { 4, 5, 6, 10 }, { 7, 8, 9, 20 } };

enum { NO_FACIL, NO_MEDIO, NO_DIFICIL, NO_CHEFE, NO_VAZIO = 0xFF };
// Onde a run parou: R_VENCEU e a mesa vencida (falta o amuleto do chefe, a
// loja ou o fim).
enum { R_LOJA_INI, R_MAPA, R_PARTIDA, R_VENCEU, R_LOJA, R_AMULETO };


// Os oponentes do cassino. Cada um tem um jeito de apostar, dados que
// prefere, um trunfo (regra so dele, que vale a partida toda) e o que diz.
// Os comuns trazem o trunfo aos poucos, andar a andar do cassino: nenhum na
// dificuldade Normal (o terreo), um ou dois a mais a cada nivel, todos no 10
// (os chefes, sempre). E alguns tem um nucleo: dados que sempre estao na bolsa
// deles (um no primeiro salao, dois no segundo, tres no terceiro), alem dos
// do tema, sorteados.
enum { ES_NORMAL, ES_BOLA };             // ES_BOLA: aposta alto na frente em fichas, corre atras
typedef struct {
    const char *nome, *frase;
    float ousadia;                       // -1 cauteloso ... +1 aposta alto
    uint8_t fav[8];                      // o tema: dados preferidos (0xFF = fim)
    const char *trunfo, *regra;          // nome do trunfo e o que ele faz
    amul_t   amul;                       // trunfo feito de amuletos
    uint8_t reg;                         // ou regra propria (REG_*)
    const char *fala[FA_N][2];
    uint8_t andar_trunfo;                // a dificuldade a partir da qual o trunfo vale
    uint8_t nucleo[3];                   // dados que ele sempre traz (0: nenhum; a moeda nunca e nucleo)
    uint8_t estilo;                      // ES_*
} perso_t;

#define AM(a) AM_BIT(a)
static const perso_t PERSO[] = {
    { "O NOVATO",        "Chegou hoje ao cassino.",           -0.3f,
      { D_D8, D_D12, D_TEIMOSO, D_QUEBRADO, D_SENTINELA, 0xFF },
      "Nervoso", "Depois de perder uma rodada, compra só 6 dados na seguinte.", 0, REG_NERVOSO,
      { { "Primeira vez aqui... pega leve?", "Eu li as regras ontem!" },
        { "Ih, essa doeu.", "Ufa, não fui eu." },
        { "Sério? Ganhei?", "Sorte de principiante!" },
        { "Tá, tá, eu aprendo.", "Calma, calma..." },
        { "Eu ganhei de verdade?", "Vou contar pra todo mundo!" },
        { "Foi justo. Boa partida.", "Volto amanhã, mais esperto." } },
      3, { 0 }, ES_NORMAL },
    { "A CAUTELOSA",     "Aposta pouco e não arrisca.",       -1.0f,
      { D_ESCUDO, D_TEIMOSO, D_LASTRO, D_SENTINELA, D_ACUMULADOR, 0xFF },
      "Pé atrás", "O primeiro dado da sequência dela não é anulado pela sequência.", AM(A_LUVA), REG_NADA,
      { { "Devagar se vai longe.", "Sem pressa, querido." },
        { "Pressa demais dá nisso.", "Eu avisei." },
        { "Passo a passo.", "Sem sustos." },
        { "Hm. Anotado.", "Um passo atrás, só isso." },
        { "Paciência sempre paga.", "Nada como a prudência." },
        { "Você arriscou e deu certo.", "Desta vez, a sorte foi sua." } },
      1, { D_TEIMOSO, D_ESCUDO }, ES_NORMAL },
    { "O APOSTADOR",     "Dobra tudo, sempre.",                1.0f,
      { D_TUDO_NADA, D_EXPLOSIVO, D_D20, D_FICHAS, D_VENTANIA, 0xFF },
      "Pote quente", "Vencendo uma rodada, leva 2 fichas a mais.", AM(A_POTE), REG_NADA,
      { { "Dobra ou nada, parceiro!", "Hoje eu tô quente!" },
        { "Hahaha! Desabou!", "Faltou coragem!" },
        { "Pote na mão!", "Mais uma! Mais uma!" },
        { "Tô só esquentando.", "Ainda tenho ficha!" },
        { "A mesa é minha!", "Quem não arrisca, não petisca!" },
        { "Amanhã eu recupero tudo.", "Foi só um azar." } },
      1, { D_TUDO_NADA, D_EXPLOSIVO }, ES_NORMAL },
    { "O LADRÃO",        "O que é seu pode ser dele.",         0.3f,
      { D_GATUNO, D_CARIDOSO, D_PIRATA, D_ESPELHO, D_COBRADOR, 0xFF },
      "Batedor de carteira", "Vencendo uma rodada, ainda leva 2 fichas suas.", 0, REG_BATEDOR,
      { { "Bela bolsa. Seria uma pena...", "Relaxa, eu não mordo." },
        { "Ops. Escorregou?", "Essa eu nem precisei roubar." },
        { "Obrigado pelas fichas.", "Ninguém viu nada." },
        { "Hm, você é atento.", "Da próxima vez..." },
        { "Leve, como sempre.", "Foi um prazer fazer negócio." },
        { "Você vigiou bem a bolsa.", "Essa escapou." } },
      5, { D_GATUNO, D_CARIDOSO }, ES_NORMAL },
    { "A MATEMÁTICA",    "Conta cada multiplicador.",          0.0f,
      { D_DOBRO, D_FARTURA, D_COPIADOR, D_FERREIRO, D_ACUMULADOR, 0xFF },
      "Conta certa", "Dado dela que tirar o número máximo pontua +2.", AM(A_OURO), REG_NADA,
      { { "As chances estão comigo.", "Já fiz as contas." },
        { "A chance de anular era de 62%.", "Previsível." },
        { "Exatamente como calculei.", "Estatística pura." },
        { "Um desvio aceitável.", "Margem de erro." },
        { "Os números não mentem.", "Como queríamos demonstrar." },
        { "Preciso refazer o modelo.", "Variância. Só isso." } },
      3, { D_DOBRO, D_ACUMULADOR }, ES_NORMAL },
    { "O ESPINHENTO",    "Cada ponto seu custa caro.",         0.4f,
      { D_MARTELO, D_MALDICAO, D_CARRASCO, D_INVEJOSO, D_LASER, 0xFF },
      "Casca grossa", "Os seus dados não podem anular os dele.", AM(A_COURACA), REG_NADA,
      { { "Vai doer.", "Cada ponto vai custar caro." },
        { "Isso! Desmorona!", "Gostei de ver." },
        { "Sentiu o espinho?", "Mais um arranhão." },
        { "Grrr.", "Você vai pagar por essa." },
        { "Sai daqui sangrando.", "Ninguém passa por mim." },
        { "...Respeito.", "Da próxima, eu te pego." } },
      7, { D_MARTELO, D_CARRASCO }, ES_NORMAL },
    { "A SUPERSTICIOSA", "Confia no trevo e na moeda.",        0.2f,
      { D_TREVO, D_SORTE, D_SEMENTE, D_JACKPOT, D_MISERICORDIOSO, 0xFF },
      "Mão de sorte", "Os dados dela de 2 e 4 lados pontuam +1.", AM(A_PENA), REG_NADA,
      { { "Toquei no trevo três vezes.", "As cartas disseram: hoje é meu dia." },
        { "Viu? Mau agouro.", "A lua não está a seu favor." },
        { "O trevo nunca falha.", "Bati na madeira!" },
        { "Gato preto, certeza.", "Esqueci de cruzar os dedos." },
        { "Os astros me guiaram.", "Sorte é tudo, meu bem." },
        { "Deixei meu amuleto em casa.", "Mercúrio retrógrado..." } },
      4, { D_TREVO, D_SORTE }, ES_NORMAL },
    { "O BRUTAMONTES",   "Dados grandes, pouca conversa.",     0.5f,
      { D_D12, D_LASTRO, D_EGOISTA, D_EXPLOSIVO, D_DESAFIANTE, 0xFF },
      "Mão grande", "Compra 8 dados por rodada.", AM(A_BOLSA), REG_NADA,
      { { "Dado grande. Ponto grande.", "Hmpf." },
        { "Fraco.", "Ha." },
        { "Esmaguei.", "Mais." },
        { "Hmpf.", "Isso não conta." },
        { "Fácil.", "Próximo." },
        { "...Forte, você.", "Vou treinar mais." } },
      2, { D_D8, D_D12, D_LASTRO }, ES_NORMAL },
    // Chefes, um por salao.
    { "O CRUPIÊ",        "Conhece cada dado desta mesa.",      0.0f,
      { D_VICIADO, D_LASTRO, D_DOBRO, D_ESCUDO, D_CARRASCO, 0xFF },
      "A casa sempre ganha", "Rodada empatada: o pote fica com ele.", 0, REG_CASA,
      { { "Façam suas apostas.", "A casa lhe dá as boas-vindas." },
        { "A casa agradece.", "Que pena, senhor." },
        { "A casa recolhe.", "Como de costume." },
        { "A casa paga.", "Bem jogado." },
        { "A casa sempre ganha.", "Volte sempre." },
        { "A banca se rende. Por hoje.", "Impressionante. Siga em frente." } },
      0, { 0 }, ES_NORMAL },
    { "A DONA DO SALÃO", "Nunca perdeu no próprio salão.",     0.4f,
      { D_ESPELHO, D_INVEJOSO, D_FOGO, D_COPIADOR, D_EGOISTA, 0xFF },
      "Dona da casa", "Você compra só 6 dados por rodada.",
      AM(A_FURADO), REG_NADA,
      { { "Você está na minha casa.", "Que gracinha, um desafiante." },
        { "Que deselegante.", "Meu salão não perdoa." },
        { "Obrigada pela visita.", "Tudo aqui é meu." },
        { "Ora, ora.", "Não se acostume." },
        { "Os seguranças levam você até a porta.", "Ninguém me vence aqui." },
        { "Você tem talento. Suba.", "O Barão vai gostar de você." } },
      0, { 0 }, ES_NORMAL },
    { "O BARÃO",         "O maior apostador do cassino.",      0.8f,
      { D_TUDO_NADA, D_MALDITO, D_EXPLOSIVO, D_FOGO, D_JACKPOT, 0xFF },
      "Intocável", "Seus dados não roubam nem anulam os dele, e na sequência dele o último não anula o anterior.",
      AM(A_IMA) | AM(A_COURACA) | AM(A_REDE), REG_NADA,
      { { "Então você chegou até mim.", "Sente-se. Isso será breve." },
        { "Lamentável.", "Esperava mais." },
        { "Naturalmente.", "Como previsto." },
        { "Interessante...", "Hm. Sorte." },
        { "O cassino continua meu.", "Volte quando for digno." },
        { "Impossível... o cassino é seu.", "Bravo. Você mereceu." } },
      0, { 0 }, ES_NORMAL },
    // Mais oponentes comuns (vem depois dos chefes para nao mudar os mapas ja salvos).
    { "O ILUSIONISTA",   "Nada é o que parece.",               0.2f,
      { D_ESPELHO, D_COPIADOR, D_VENTANIA, D_PAR, D_TUDO_NADA, 0xFF },
      "Truque duplo", "Dado dele que repete o número de outro dele: +2.", AM(A_DUPLA), REG_NADA,
      { { "Nada na manga. Nada na mão.", "Escolha um dado... qualquer dado." },
        { "Puf! Desapareceu.", "E o seu ponto... sumiu." },
        { "Tcharam!", "A mão é mais rápida que o olho." },
        { "Faz parte do número.", "Um truque falhou. Tenho outros." },
        { "E para o grand finale...", "Aplausos, por favor." },
        { "Bravo! Você viu o truque.", "Um mágico nunca explica a derrota." } },
      7, { D_ESPELHO, D_COPIADOR }, ES_NORMAL },
    { "O CAUBÓI",        "Atira primeiro, pergunta depois.",   0.6f,
      { D_LASER, D_EXPLOSIVO, D_DESAFIANTE, D_D12, D_VICIADO, 0xFF },
      "Saque rápido", "O primeiro dado da sequência dele pontua +1.", 0, REG_SAQUE,
      { { "Esta mesa é pequena pra nós dois.", "Saque quando estiver pronto." },
        { "Caiu do cavalo, parceiro.", "Poeira. Só sobrou poeira." },
        { "Na mosca.", "O mais rápido do oeste." },
        { "Errei o tiro.", "Hm. Vento contrário." },
        { "Volte pro rancho, forasteiro.", "Pode guardar o revólver." },
        { "Você saca rápido, forasteiro.", "Tiro limpo. Respeito." } },
      2, { D_LASER, D_VICIADO }, ES_NORMAL },
    { "O MONGE",         "Não aposta contra o destino.",      -0.6f,
      { D_SENTINELA, D_SOLITARIO, D_MISERICORDIOSO, D_TEIMOSO, D_D8, 0xFF },
      "Equilíbrio", "Uma vez por rodada, na primeira vez que a sequência anularia o dado anterior, ele continua válido.", AM(A_FERRADURA), REG_NADA,
      { { "Respire. Os dados também respiram.", "Que a mesa nos ensine algo." },
        { "A pressa derruba o que ergue.", "Tudo que sobe, cai." },
        { "Assim é.", "O rio segue o seu curso." },
        { "Também isso passará.", "Uma lição, não uma perda." },
        { "Não há vencedor. Só o caminho.", "Medite sobre isso." },
        { "Você está em paz com a sorte.", "O aluno superou o mestre." } },
      5, { D_SENTINELA, D_SOLITARIO }, ES_NORMAL },
    { "A DETETIVE",      "Já sabe o que você vai jogar.",      0.0f,
      { D_INVEJOSO, D_COBRADOR, D_MARTELO, D_D12, D_D8, 0xFF },
      "Última palavra", "Na 1ª rodada e quando está atrás em fichas, ela joga por último.", 0, REG_ULTIMA,
      { { "Pode começar. Eu observo.", "Cada jogada deixa uma pista." },
        { "Previsível. Anotei.", "Elementar." },
        { "Caso encerrado.", "As pistas não mentem." },
        { "Uma pista falsa. Curioso.", "Isso não estava no relatório." },
        { "Seu padrão te entregou.", "Investigação concluída." },
        { "Você me despistou. Bom trabalho.", "Vou reabrir o caso." } },
      8, { D_INVEJOSO }, ES_NORMAL },
    { "A MARATONISTA",   "Começa devagar e termina voando.",   0.3f,
      { D_ACUMULADOR, D_SEMENTE, D_TEIMOSO, D_FARTURA, D_D8, 0xFF },
      "Segundo fôlego", "Da 6ª rodada em diante, os dados dela pontuam +1.", 0, REG_FOLEGO,
      { { "É maratona, não corrida.", "Vou no meu ritmo." },
        { "Tropeçou na largada?", "Fôlego curto, hein." },
        { "Mantendo o ritmo.", "Um passo de cada vez." },
        { "Só aquecendo.", "Ainda tenho fôlego." },
        { "Linha de chegada!", "Treino é tudo." },
        { "Você correu bem. Vou treinar.", "Cãibra. Só pode ser." } },
      4, { D_ACUMULADOR, D_SEMENTE }, ES_NORMAL },
    // Mais cinco comuns, com nucleo de dados e trunfo proprio...
    { "O RIVAL",         "Entrou no cassino no mesmo dia que você.", 0.4f,
      { D_COPIADOR, D_ESPELHO, D_PAR, D_DOMINO, D_JACKPOT, D_RANCOR, 0xFF },
      "Revanche", "Se perdeu a rodada anterior, cada dado válido dele pontua +1.", AM(A_DESFORRA), REG_NADA,
      { { "De novo você? Desta vez eu ganho.", "Eu copio tudo que você faz. E melhor." },
        { "Anulado! Igualzinho ao seu.", "Até no erro eu te imito." },
        { "Viu? Eu aprendo rápido.", "Um a zero pra mim." },
        { "Anotei. Revanche já.", "Agora é pessoal." },
        { "Finalmente! Fui melhor que você.", "Sempre um passo à frente." },
        { "Isso não acabou.", "A gente se vê no próximo salão." } },
      6, { D_ECO, D_GEMEO }, ES_NORMAL },
    { "A BRUXA",         "Lê a sua sorte. E muda.",            0.1f,
      { D_MALDITO, D_MALDICAO, D_QUEBRADO, D_TREVO, D_SORTE, 0xFF },
      "Bruxaria", "Cada rodada que ela vence põe um dado Fantasma (pontua 1) na sua bolsa, até o fim da partida.",
      AM(A_MAU_OLHADO), REG_NADA,
      { { "Embaralhei os seus dados ontem à noite.", "Já vi como isso termina." },
        { "A pedra não mente.", "Eu disse. Eu sempre digo." },
        { "Mais um para a sua coleção de fantasmas.", "Hihihi..." },
        { "Os astros erraram. Por enquanto.", "Hm. Uma nuvem passou." },
        { "Seus dados agora são meus pesadelos.", "Volte quando estiver menos assombrado." },
        { "Você quebrou o feitiço.", "Os fantasmas vão embora com você." } },
      9, { D_SORTE_REVES, D_MEDUSA }, ES_NORMAL },
    { "O MALUCO",        "Ninguém sabe o que ele vai fazer. Nem ele.", 1.0f,
      { D_MALDITO, D_SORTE_REVES, D_EXPLOSIVO, D_VENTANIA, D_BUMERANGUE, D_FOGO, 0xFF },
      "Cara ou coroa", "A cada rodada ele joga uma moeda: cara, os dados dele pontuam +2; coroa, pontuam -1.",
      0, REG_MALUCO,
      { { "Hahaha! Vamos ver o que sai!", "Eu aposto tudo. Sempre. Em tudo." },
        { "Anulado! Que lindo!", "Hahaha, de novo!" },
        { "Eu nem sei como!", "Hahaha! Tudo!" },
        { "Ótimo! Agora fica interessante!", "Perdi? Que divertido!" },
        { "Que loucura! Mais uma?", "Hahaha! Ninguém entende!" },
        { "Perdi tudo! Que dia lindo!", "Hahaha... espera, perdi mesmo?" } },
      10, { D_TUDO_NADA, D_EGOISTA }, ES_NORMAL },
    { "A BANQUEIRA",     "Empresta, cobra e nunca esquece.",   0.0f,
      { D_COBRADOR, D_FICHAS, D_CAPITAO, D_ESPERANCA, 0xFF },
      "Juros", "Toda rodada que você começa atrás em fichas, a sua entrada custa 2 a mais.", 0, REG_JUROS,
      { { "Sente-se. Os juros já começaram.", "Tudo aqui tem um preço." },
        { "Prejuízo. Seu.", "Isso entra na sua conta." },
        { "Lucro.", "Depositado." },
        { "Um investimento de risco.", "Vou cobrar depois." },
        { "A dívida está quitada. Por você.", "O banco agradece." },
        { "Você saldou a conta. Por hoje.", "Bom pagador. Raro." } },
      6, { D_AGIOTA, D_POTE_QUENTE }, ES_BOLA },
    { "O COVEIRO",       "Gosta de ver dado anulado.",      -0.2f,
      { D_MISERICORDIOSO, D_FENIX, D_VIDRO, D_SENTINELA, 0xFF },
      "Velório", "Cada dado dele anulado na rodada pontua +1 para ele no fim.", AM(A_CRISANTEMO), REG_NADA,
      { { "Já medi a sua cova.", "Calma. Ninguém tem pressa aqui." },
        { "Mais um para o meu jardim.", "Descanse em paz." },
        { "A terra agradece.", "Silêncio, por favor." },
        { "Tudo volta para o pó.", "Hm. Cedo demais." },
        { "Encomendo as flores.", "Que descanse em paz." },
        { "Você escapou. Desta vez.", "Guardo o seu lugar." } },
      8, { D_MARTIR, D_COVEIRO }, ES_NORMAL },
    // ...e o chefe do salao final, depois do Barao, so no ultimo nivel.
    { "O CORINGA",       "Ri de tudo. Principalmente de você.", 0.7f,
      { D_D20, D_ESCUDO, D_LASER, D_CAPITAO, D_TEIMOSO, D_FOGO, D_D20, D_CHECKPOINT },
      "Carta na manga", "Seu dado que tirou mais vira o 4º dele (\"?\" se ele abre). Da 5ª rodada, seu dado anulado some.",
      0, REG_CORINGA,
      { { "Hahaha! Achou que o Barão era o último?", "Vamos brincar com os SEUS dados." },
        { "Ops! Hahaha!", "Anulado, anulado, anulado!" },
        { "Ganhei com o seu próprio dado! Hahaha!", "Obrigado pelo empréstimo!" },
        { "Hahaha! Boa piada!", "Essa foi engraçada. Para você." },
        { "A piada era você. Hahaha!", "Fim de jogo! Hahaha!" },
        { "Hahaha... ha. Você venceu o Coringa.", "A última risada é sua. Por hoje." } },
      0, { D_D20, D_ESCUDO, D_LASER }, ES_NORMAL },
};
#define N_PERSO_COMUM 8                  // os oito primeiros...
#define PERSO_NOVOS   11                 // ...e os comuns que vieram depois
#define PERSO_CORINGA 21                 // o chefe do salao final
#define N_PERSO       ((int)(sizeof PERSO / sizeof PERSO[0]))
static bool perso_chefe(int p) { return p >= PERSO_CHEFE0 && p < PERSO_CHEFE0 + DES_ANDARES; }
