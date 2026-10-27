// ===========================================================================
// Os dados
// ===========================================================================
enum { EF_NADA, EF_DOBRO, EF_VICIADO, EF_LASTRO, EF_PAR, EF_ESCUDO,
       EF_CARRASCO, EF_EXPLODE, EF_MARTELO, EF_PIRATA, EF_ESPELHO, EF_FICHAS,
       EF_TUDO_NADA, EF_MALDITO, EF_GATUNO, EF_QUEBRADO, EF_INVEJOSO, EF_CARIDOSO,
       EF_FARTURA, EF_SORTE, EF_TEIMOSO, EF_SEMENTE, EF_FOGO, EF_MALDICAO, EF_TREVO,
       EF_JACKPOT, EF_COPIADOR, EF_EGOISTA, EF_LASER, EF_VENTANIA,
       EF_MISERICORDIA, EF_SOLITARIO, EF_SENTINELA, EF_FERREIRO, EF_COBRADOR,
       EF_DESAFIANTE, EF_VIDRO, EF_ACUMULADOR, EF_SEGURANCA,
       EF_BUMERANGUE, EF_GEMEO, EF_PRISMA, EF_FENIX, EF_TRONO,
       EF_CAVALO, EF_BERSERKER, EF_VIUVA, EF_CAPITAO, EF_CHECKPOINT, EF_SORTE_REVES,
       EF_AUTOFAGIA, EF_ECO, EF_DOMINO, EF_ANCORA, EF_MARTIR, EF_PUXA, EF_COVEIRO, EF_HEXA,
       EF_OURICO, EF_CALIBRE, EF_SABOTADOR, EF_AZARAO, EF_AMPULHETA, EF_AGIOTA, EF_MEDUSA,
       EF_RANCOR, EF_ESPERANCA, EF_PONTE, EF_GARIMPEIRO, EF_POTE, EF_FANTASMA,
       EF_PREVISIVEL, EF_OCULTO, EF_SACRIFICIO, EF_VAZIO, EF_MEDIA, EF_VAMPIRO, EF_COLECIONADOR,
       EF_PACTO, EF_ASSOMBRA, EF_FICHA_VELHA, EF_SANGUESSUGA,
       EF_TRENO, EF_BONECO, EF_JINGLE, EF_FURIA, EF_ONDA, EF_TSUNAMI, EF_PATINHOS,
       EF_PRESENTE, EF_PANETONE, EF_PISCA, EF_MEIA, EF_ESTRELA, EF_RENA, EF_REVOLVER,
       EF_ASTRO, EF_CONSTELACAO, EF_GALAXIA, EF_BURACO, EF_UNIVERSO };

enum { RAR_COMUM, RAR_INCOMUM, RAR_RARO, RAR_LENDA };

typedef struct {
    const char *nome;
    uint8_t lados, efeito, raridade;
    uint8_t preco;                   // fichas na loja (0: sem preco)
    const char *desc;
} tipo_t;

enum { D_D2, D_D4, D_D6, D_D8, D_D12, D_D20,
       D_DOBRO, D_VICIADO, D_LASTRO, D_PAR, D_ESCUDO,
       D_CARRASCO, D_EXPLOSIVO, D_MARTELO, D_PIRATA, D_ESPELHO, D_FICHAS,
       D_TUDO_NADA, D_MALDITO, D_GATUNO, D_QUEBRADO, D_INVEJOSO, D_CARIDOSO,
       D_FARTURA, D_SORTE,
       // D_MALDITO e o d20 que se chama Agouro na tela; o "Maldito" que a tela
       // mostra e o d4 D_MALDICAO, que tira pontos do rival.
       D_TEIMOSO, D_SEMENTE, D_FOGO, D_MALDICAO, D_TREVO,
       D_JACKPOT, D_COPIADOR, D_EGOISTA, D_LASER, D_VENTANIA,
       // Sempre no fim da lista: os numeros dos tipos vao no salvamento.
       D_MISERICORDIOSO, D_SOLITARIO, D_SENTINELA, D_FERREIRO, D_COBRADOR,
       D_DESAFIANTE, D_VIDRO, D_ACUMULADOR, D_SEGURANCA,
       // Vindos do Modo Desafiante e os mais novos: so_desafio diz quais
       // aparecem so la.
       D_BUMERANGUE, D_GEMEO, D_PRISMA, D_FENIX, D_TRONO,
       D_CAVALO, D_BERSERKER, D_VIUVA, D_CAPITAO,
       D_CHECKPOINT, D_SORTE_REVES, D_AUTOFAGIA,
       D_ECO, D_DOMINO, D_ANCORA, D_MARTIR, D_PUXA_FILA, D_COVEIRO, D_HEXA,
       D_OURICO, D_CALIBRE, D_SABOTADOR, D_AZARAO, D_AMPULHETA, D_AGIOTA, D_MEDUSA,
       D_MISTERIOSO, D_RANCOR, D_ESPERANCA, D_PONTE, D_GARIMPEIRO, D_POTE_QUENTE, D_FANTASMA,
       D_PREVISIVEL, D_OCULTO, D_SACRIFICIO, D_VAZIO, D_MEDIA, D_VAMPIRO, D_COLECIONADOR,
       D_PACTO, D_ASSOMBRACAO, D_FICHA_VELHA, D_SANGUESSUGA,
       // Os de Natal (so de 20 de outubro a 6 de janeiro) e os mais novos.
       D_TRENO, D_BONECO, D_JINGLE, D_FURIA, D_ONDA, D_TSUNAMI, D_PATINHOS,
       D_PRESENTE, D_PANETONE, D_PISCA, D_MEIA, D_ESTRELA, D_RENA, D_REVOLVER,
       // O ciclo cosmico: a Estrela (D_ASTRO; a de Natal e a D_ESTRELA) e o que ela vira.
       D_ASTRO, D_CONSTELACAO, D_GALAXIA, D_BURACO, D_UNIVERSO, D_N };
// Os que so aparecem no Modo Desafiante; o resto vale tambem na partida
// rapida, com 2 jogadores e online. (A ordem acima nao diz nada: os tipos
// novos entram sempre no fim, por causa do salvamento.)
// Os lados do d8 do Reves, no Sorte/Reves (a Sorte e o d12 do proprio tipo).
#define SR_REVES 8
// O teto do bonus que o Acumulador guarda, e o da Autofagia.
#define ACUM_MAX 6
#define AUTO_MAX 3
// O que a Ficha Velha anulada rende no fim.
#define FICHA_VELHA_PTS 4
// A Autofagia so come com pelo menos tantos dados em jogo na bolsa.
#define AUTO_BOLSA 8
// A Esperanca dobra quem comecou a rodada com tantas fichas ou menos.
#define ESPERANCA_FICHAS 15
// O que cada Jingle Bell valendo rende no fim, com outro valendo junto.
#define JINGLE_PTS 4
// A Furia: o que guarda tirando 4, o que perde tirando 1, e o teto (dos dois lados).
#define FURIA_GANHA 2
#define FURIA_PERDE 1
#define FURIA_MAX   99
// A Onda vira Tsunami quando o que ela guardou chega a tanto.
#define ONDA_VIRA 9
// Os Tres Patinhos: x3 a cada tantas vezes que terminam valendo.
#define PATINHOS_CADA 3
// O ciclo cosmico: tirando a faixa de cima, a Estrela (d6) vira Constelacao
// (d8), Galaxia (d12), Buraco Negro (d20) e Universo (d100); o Universo,
// lancado (sempre sozinho), faz o Big Bang e volta a ser Estrela. O bonus de
// quem evolui, e o Buraco Negro anula os vizinhos.
#define COSMO_ESTRELA 1
#define COSMO_CONSTELACAO 3
#define COSMO_GALAXIA 5
// O Revolver: as balas do tambor (uma por lance) e o que o tiro vale, na ultima.
#define REVOLVER_BALAS 6
#define REVOLVER_TIRO  12
// A Meia na Lareira: o que guarda a cada vez que termina anulada, e o teto.
#define MEIA_GUARDA 2
#define MEIA_MAX    99
// A Estrela de Belem, por ultimo: o que rende por dado valendo abaixo dela.
#define ESTRELA_PTS 2
// O Pisca-pisca: o fixo diz como ele vem no proximo lance (0 aceso, 1 apagado).
static bool pisca_aceso(int fixo) { return !(fixo & 1); }
// O Revolver: o fixo sao as balas no tambor antes do lance; este lance poe
// mais uma, e com a sexta ele dispara.
static bool revolver_dispara(int fixo) { return fixo + 1 >= REVOLVER_BALAS; }
static bool cosmico(int t) { return t >= D_ASTRO && t <= D_UNIVERSO; }
// O que ele vira, e a partir de que numero (o Universo: sempre, no Big Bang).
static int cosmo_proximo(int t) { return t == D_UNIVERSO ? D_ASTRO : t + 1; }
static int cosmo_faixa(int t)
{
    switch (t) {
    case D_ASTRO:       return 5;
    case D_CONSTELACAO: return 6;
    case D_GALAXIA:     return 9;
    case D_BURACO:      return 14;
    default:            return 1;
    }
}
static bool cosmo_evolui(int t, int v) { return cosmico(t) && v >= cosmo_faixa(t); }
static int cosmo_bonus(int t) { return t == D_ASTRO ? COSMO_ESTRELA : t == D_CONSTELACAO ? COSMO_CONSTELACAO : t == D_GALAXIA ? COSMO_GALAXIA : 0; }
// A epoca do Natal (20 de outubro a 6 de janeiro): o feltro Natalino abre e os dados
// de Natal entram no jogo (util.inc le o relogio no comeco de cada partida).
// Em dezembro o Papai Noel e os oponentes de Natal chegam ao cassino; do dia
// 26 de dezembro a 6 de janeiro, o feltro do Reveillon.
static bool natal, dezembro, reveillon;
static bool de_natal(int t)
{
    switch (t) {
    case D_TRENO: case D_BONECO: case D_JINGLE: case D_PRESENTE: case D_PANETONE:
    case D_PISCA: case D_MEIA: case D_ESTRELA: case D_RENA:
        return true;
    default:
        return false;
    }
}
static bool fora_de_epoca(int t) { return de_natal(t) && !natal; }
// O Dado Misterioso nao aparece em lugar nenhum: nem na colecao, nem no
// premio, nem na loja. So sai de uma pedra negativa que se parte.
static bool oculto(int t) { return t == D_MISTERIOSO; }
// Dados que nao saem em sorteio nenhum (premio, loja, liberacao): o oculto, o
// Fantasma, que so aparece quando alguem o poe na bolsa, o Tsunami, que so
// nasce de uma Onda, e os de Natal fora da epoca.
// O ciclo cosmico depois da Estrela so nasce dela: nao sai em sorteio nem
// aparece na colecao (e surpresa).
static bool cosmo_oculto(int t) { return t >= D_CONSTELACAO && t <= D_UNIVERSO; }
static bool sem_sorteio(int t) { return oculto(t) || t == D_FANTASMA || t == D_TSUNAMI || fora_de_epoca(t) || cosmo_oculto(t); }
static bool so_desafio(int t)
{
    switch (t) {
    case D_TRONO: case D_BERSERKER: case D_AUTOFAGIA: case D_GARIMPEIRO:
        return true;
    default:
        return false;
    }
}
// Quantos tipos a partida rapida tem, e o i-esimo deles.
static int n_basicos(void)
{
    int n = 0;
    for (int t = 0; t < D_N; t++) n += !so_desafio(t) && !sem_sorteio(t);
    return n;
}
static int basico(int i)
{
    for (int t = 0; t < D_N; t++)
        if (!so_desafio(t) && !sem_sorteio(t) && i-- == 0) return t;
    return D_D6;
}

// "Fica" = termina a rodada valendo, sem ter sido anulado.
static const tipo_t TIPO[D_N] = {
    { "Moeda",            2, EF_NADA,         RAR_COMUM,    4, "Sai 1 ou 2" },
    { "D4",               4, EF_NADA,         RAR_COMUM,    5, "Quatro lados, de 1 a 4" },
    { "D6",               6, EF_NADA,         RAR_COMUM,    0, "Seis lados, de 1 a 6" },
    { "D8",               8, EF_NADA,         RAR_INCOMUM,  9, "Oito lados, de 1 a 8" },
    { "D12",             12, EF_NADA,         RAR_RARO,    20, "Doze lados, de 1 a 12" },
    { "D20",             20, EF_NADA,         RAR_LENDA,   26, "Vinte lados, de 1 a 20" },
    { "Dobro",            6, EF_DOBRO,        RAR_INCOMUM, 16, "Se tirar de 1 a 3: o próximo dado pontua x2" },
    { "Viciado",          6, EF_VICIADO,      RAR_INCOMUM, 16, "Rola 2 vezes e fica com o maior" },
    { "Lastro",          12, EF_LASTRO,       RAR_LENDA,   26, "Sempre de 4 a 12" },
    { "Par",              4, EF_PAR,          RAR_INCOMUM, 11, "Igual ao último dado válido: +6" },
    { "Escudo",           6, EF_ESCUDO,       RAR_RARO,    15, "Nunca é anulado nem roubado" },
    { "Carrasco",         4, EF_CARRASCO,     RAR_INCOMUM, 17, "Se tirar 4: anula o maior dado do oponente. Se tirar 1: anula o seu menor dado." },
    { "Explosivo",        8, EF_EXPLODE,      RAR_RARO,    16, "Se tirar 8, rola novamente somando a pontuação" },
    { "Espinhoso",        4, EF_MARTELO,      RAR_RARO,    15, "Com 3 ou 4: tira 4 pontos do oponente" },
    { "Pirata",           8, EF_PIRATA,       RAR_RARO,     0, "Com 7 ou 8: tira 4 pontos do oponente" },
    { "Espelho",          6, EF_ESPELHO,      RAR_RARO,    20, "1 ou 2: vira o maior do oponente" },
    { "Fichas",           4, EF_FICHAS,       RAR_COMUM,    6, "Com mais fichas que o oponente: +4" },
    { "Tudo ou Nada",     2, EF_TUDO_NADA,    RAR_INCOMUM, 14, "2: seus dados pontuam +5 cada · 1: seus dados pontuam -5 cada" },
    { "Agouro",          20, EF_MALDITO,      RAR_LENDA,   24, "7 ou menos: anula a sua sequência" },
    { "Gatuno",           6, EF_GATUNO,       RAR_RARO,    20, "Rouba 1 dado do oponente por 1 nos seus outros válidos" },
    { "Quebrado",         8, EF_QUEBRADO,     RAR_COMUM,    9, "Número fixo; quebra ao ser lançado" },
    { "Invejoso",         6, EF_INVEJOSO,     RAR_RARO,     0, "Se tirar 5 ou 6: anula o maior do oponente. Se tirar 1 ou 2: anula os seus 2 menores." },
    { "Caridoso",         4, EF_CARIDOSO,     RAR_RARO,     0, "Com 3 ou 4: rouba o menor do oponente" },
    { "Fartura",          4, EF_FARTURA,      RAR_INCOMUM, 14, "Seus dados com 1 ou 2 pontuam x2" },
    { "Moeda sorte",      2, EF_SORTE,        RAR_INCOMUM, 14, "1: seus dados ímpares +1 · 2: seus dados pares +1" },
    { "Teimoso",          6, EF_TEIMOSO,      RAR_INCOMUM, 11, "Nunca é anulado" },
    { "Semente",          2, EF_SEMENTE,      RAR_INCOMUM,  9, "Seus dados somam 14 ou mais: +7" },
    { "Ficha Fogo",       2, EF_FOGO,         RAR_RARO,    22, "Válido: queima o seu maior dado · 2: +10" },
    { "Maldito",          4, EF_MALDICAO,     RAR_RARO,     0, "Pontua e tira o mesmo número do oponente" },
    { "Trevo",            4, EF_TREVO,        RAR_INCOMUM,  8, "4 pontua 10 · 1 a 3 pontua 0" },
    { "Jackpot",          4, EF_JACKPOT,      RAR_INCOMUM,  9, "Dois iguais: +4 · três iguais: +10" },
    { "Copiador",         6, EF_COPIADOR,     RAR_INCOMUM, 11, "Cada dado seu igual a ele: +2" },
    { "Egoísta",         12, EF_EGOISTA,      RAR_INCOMUM, 14, "Anula os seus outros dados" },
    { "Laser",            6, EF_LASER,        RAR_RARO,    16, "Com 5 ou 6: anula um dado do oponente" },
    { "Ventania",         8, EF_VENTANIA,     RAR_RARO,    10, "Rola de novo o seu primeiro dado" },
    { "Misericordioso",   4, EF_MISERICORDIA, RAR_INCOMUM, 13, "Válido: +2 por dado seu anulado" },
    { "Solitário",        4, EF_SOLITARIO,    RAR_COMUM,    9, "Sem vizinho válido: +6" },
    { "Sentinela",        4, EF_SENTINELA,    RAR_COMUM,    6, "Anulado pela sequência: só ele é anulado" },
    { "Ferreiro",         4, EF_FERREIRO,     RAR_INCOMUM, 10, "Não pontua · 3 ou 4: o próximo dado +6" },
    { "Cobrador",         4, EF_COBRADOR,     RAR_COMUM,    6, "Oponente com mais fichas: +4" },
    { "Desafiante",       8, EF_DESAFIANTE,   RAR_RARO,    13, "Maior dado da rodada: +4" },
    { "Vidro",            8, EF_VIDRO,        RAR_COMUM,    7, "Tirou 1: quebra e anula o dado anterior" },
    { "Acumulador",       8, EF_ACUMULADOR,   RAR_RARO,    16, "Guarda +1 cada vez que entra válido" },
    { "Segurança",        4, EF_SEGURANCA,    RAR_INCOMUM,  7, "Recebe os ataques do oponente · válido: +2" },
    { "Bumerangue",       6, EF_BUMERANGUE,   RAR_COMUM,    0, "1 ou 2: volta e rola de novo" },
    { "Gêmeo",            6, EF_GEMEO,        RAR_RARO,     0, "Tira o mesmo do último dado válido (até 6)" },
    { "Prisma",           8, EF_PRISMA,       RAR_RARO,     0, "+1 por tipo de dado válido (d2, d4, d6...)" },
    { "Fênix",            6, EF_FENIX,        RAR_RARO,     0, "Se for anulado: renasce no fim" },
    { "Trono",           12, EF_TRONO,        RAR_LENDA,    0, "4º da sequência, os quatro válidos: +4" },
    { "Cavalo de Troia",  8, EF_CAVALO,       RAR_RARO,     0, "Anulado ou roubado pelo oponente: anula os dados dele" },
    { "Berserker",       20, EF_BERSERKER,    RAR_LENDA,    0, "Válido: anula um outro dado seu ao acaso" },
    { "Viúva Negra",      8, EF_VIUVA,        RAR_RARO,     0, "Único dado seu válido: x2" },
    { "Capitão",          8, EF_CAPITAO,      RAR_RARO,     0, "Válido: +1 por dado seu válido" },
    { "Checkpoint",       8, EF_CHECKPOINT,   RAR_RARO,     0, "Válido: +3 em cada dado válido depois dele" },
    { "Sorte/Revés",     12, EF_SORTE_REVES,  RAR_INCOMUM,  0, "Um d12 soma, um d8 subtrai" },
    { "Autofagia",       12, EF_AUTOFAGIA,    RAR_LENDA,    0, "Come o seu menor dado: +1 (até +3)" },
    { "Eco",              6, EF_ECO,          RAR_RARO,     0, "Tira o mesmo número do 1º dado válido" },
    { "Dominó",           6, EF_DOMINO,       RAR_COMUM,    0, "O próximo dado tira o mesmo: +3" },
    { "Âncora",           4, EF_ANCORA,       RAR_INCOMUM,  0, "O próximo dado não é anulado nem roubado" },
    { "Mártir",           4, EF_MARTIR,       RAR_RARO,     0, "Anulado: +2 em cada dado válido seu" },
    { "Puxa-fila",        8, EF_PUXA,         RAR_INCOMUM,  0, "1º da sequência, os quatro válidos: +5" },
    { "Coveiro",          6, EF_COVEIRO,      RAR_INCOMUM,  0, "+1 por dado anulado na mesa" },
    { "Hexa",             6, EF_HEXA,         RAR_INCOMUM,  0, "+1 por dado de seis lados válido seu" },
    { "Ouriço",           6, EF_OURICO,       RAR_INCOMUM,  0, "Cada dado seu atacado: -4 para o oponente" },
    { "Calibre",          4, EF_CALIBRE,      RAR_INCOMUM,  0, "O próximo dado tira +1" },
    { "Sabotador",        6, EF_SABOTADOR,    RAR_INCOMUM,  0, "5 ou 6: o maior do oponente pontua metade" },
    { "Azarão",           4, EF_AZARAO,       RAR_INCOMUM,  0, "Começou a rodada com menos fichas: x2" },
    { "Ampulheta",        6, EF_AMPULHETA,    RAR_INCOMUM,  0, "+1 a cada 3 rodadas (até +4)" },
    { "Agiota",           4, EF_AGIOTA,       RAR_COMUM,    0, "Válido: 2 fichas do oponente para você" },
    { "Medusa",          12, EF_MEDUSA,       RAR_LENDA,     0, "Válido: os dados da sequência viram Quebrados" },
    { "Misterioso",      20, EF_NADA,         RAR_LENDA,    0, "Um d20 todo preto" },
    { "Rancor",           4, EF_RANCOR,       RAR_RARO,     0, "+1 por rodada que você perdeu" },
    { "Esperança",        8, EF_ESPERANCA,    RAR_LENDA,    0, "Com 15 fichas ou menos: x2" },
    { "Ponte",            4, EF_PONTE,        RAR_RARO,     0, "Não é anulado pela sequência, que o ignora" },
    { "Garimpeiro",       6, EF_GARIMPEIRO,   RAR_RARO,     0, "Válido: os outros dados válidos voltam à bolsa" },
    { "Pote Quente",      8, EF_POTE,         RAR_RARO,     0, "+1 a cada 25 fichas no pote" },
    { "Fantasma",         6, EF_FANTASMA,     RAR_COMUM,    0, "Pontua sempre 1; some no fim da partida" },
    { "Previsível",      12, EF_PREVISIVEL,   RAR_INCOMUM,  0, "Tira 1, depois 2, 3... (+1 a cada lance)" },
    { "Oculto",           8, EF_OCULTO,       RAR_INCOMUM,  0, "O número fica escondido até o fim da rodada" },
    { "Sacrifício",       4, EF_SACRIFICIO,   RAR_INCOMUM,  0, "Não pontua · válido: x2 no seu menor dado" },
    { "Vazio",            6, EF_VAZIO,        RAR_COMUM,    0, "Pontua 0 e nunca é anulado" },
    { "Média",            4, EF_MEDIA,        RAR_INCOMUM,  0, "Válido: + o total da sequência ÷ 4" },
    { "Vampiro",          4, EF_VAMPIRO,      RAR_RARO,     0, "Válido: -1 em cada dado do oponente" },
    { "Colecionador",     6, EF_COLECIONADOR, RAR_RARO,     0, "Tirou 6: + metade dos seus dados" },
    { "Pacto",           12, EF_PACTO,        RAR_INCOMUM,  0, "Lançado: um Fantasma na sua bolsa" },
    { "Assombração",      6, EF_ASSOMBRA,     RAR_INCOMUM,  0, "Válido: um Fantasma na bolsa do oponente" },
    { "Ficha Velha",      2, EF_FICHA_VELHA,  RAR_INCOMUM,  0, "Anulado: +4 no fim" },
    { "Sanguessuga",      8, EF_SANGUESSUGA,  RAR_INCOMUM,  0, "Anula o dado anterior; nada o anula" },
    { "Trenó",            8, EF_TRENO,        RAR_RARO,     0, "Válido: +1, +2, +3 nos seus dados antes dele" },
    { "Boneco de Neve",   8, EF_BONECO,       RAR_RARO,     0, "Tirou 8: congela e anula o 4º do oponente" },
    { "Jingle Bell",      6, EF_JINGLE,       RAR_RARO,     0, "Lançado: cópia na bolsa · 2 ou mais válidos: +4 cada" },
    { "Fúria",            4, EF_FURIA,        RAR_RARO,     0, "Tirou 4: guarda +2 · tirou 1: perde 1" },
    { "Onda",             4, EF_ONDA,         RAR_INCOMUM,  0, "Não pontua · válido: guarda o que tirou; 9 vira Tsunami" },
    { "Tsunami",         12, EF_TSUNAMI,      RAR_RARO,     0, "9 a 12: anula um dado do oponente" },
    { "Três Patinhos",    4, EF_PATINHOS,     RAR_INCOMUM,  0, "A cada 3 vezes que termina válido: x3" },
    { "Presente",         6, EF_PRESENTE,     RAR_INCOMUM,  0, "Lançado: vira de vez um dado raro ao acaso" },
    { "Panetone",         8, EF_PANETONE,     RAR_INCOMUM,  0, "Cada fatia: +0 a +3 de frutas" },
    { "Pisca-pisca",      4, EF_PISCA,        RAR_COMUM,    0, "Uma vez aceso (x2), outra apagado (0)" },
    { "Meia na Lareira",  4, EF_MEIA,         RAR_INCOMUM,  0, "Anulada: guarda +2 · válida: paga tudo" },
    { "Estrela de Belém", 8, EF_ESTRELA,      RAR_RARO,     0, "Por último: +2 por dado válido abaixo" },
    { "Rena",             6, EF_RENA,         RAR_COMUM,    0, "+1 nos dados válidos ao lado dela" },
    { "Revólver",         6, EF_REVOLVER,     RAR_INCOMUM,  0, "Carrega 1 bala por lance · na 6ª: +12" },
    { "Estrela",          6, EF_ASTRO,        RAR_INCOMUM,  0, "5 ou 6: +1, e evolui" },
    { "Constelação",      8, EF_CONSTELACAO,  RAR_INCOMUM,  0, "6 a 8: +3, e evolui" },
    { "Galáxia",         12, EF_GALAXIA,      RAR_RARO,     0, "9 a 12: +5, e evolui" },
    { "Buraco Negro",    20, EF_BURACO,       RAR_RARO,     0, "14 a 20: anula os vizinhos, e evolui" },
    { "Universo",       100, EF_UNIVERSO,     RAR_LENDA,    0, "Só vai sozinho · não cai nem é roubado" },
};

// Tres niveis de texto por dado, sempre com a mesma regra: o curto (TIPO.desc)
// vai no painel da fila e do jogo; o medio, numa linha, no premio, na loja e na
// escolha do que sai; o completo, na colecao e na tecla I.
static const char *TEXTO_MEDIO[D_N] = {
    [D_D2]              = "Uma moeda: sai 1 ou 2.",
    [D_D4]              = "Quatro lados: de 1 a 4.",
    [D_D6]              = "Seis lados: de 1 a 6.",
    [D_D8]              = "Oito lados: de 1 a 8.",
    [D_D12]             = "Doze lados: de 1 a 12.",
    [D_D20]             = "Vinte lados: de 1 a 20.",
    [D_DOBRO]           = "Se tirar de 1 a 3, o próximo dado da sequência pontua x2 no fim.",
    [D_VICIADO]         = "Rola duas vezes e fica com o maior dos dois resultados.",
    [D_LASTRO]          = "Doze lados, mas só sai de 4 a 12, todos com a mesma chance.",
    [D_PAR]             = "Se tirar o mesmo número do último dado válido da sequência: +6.",
    [D_ESCUDO]          = "Nunca é anulado. Nunca é roubado.",
    [D_CARRASCO]        = "Se tirar 4: anula o maior dado do oponente. Se tirar 1: anula o seu menor dado.",
    [D_EXPLOSIVO]       = "Se tirar 8, rola de novo, somando a pontuação (até 3 vezes seguidas).",
    [D_MARTELO]         = "Se terminar válido com 3 ou 4, tira 4 pontos do oponente.",
    [D_PIRATA]          = "Se terminar válido com 7 ou 8, tira 4 pontos do oponente.",
    [D_ESPELHO]         = "Se tirar 1 ou 2, vira o número do maior dado do oponente.",
    [D_FICHAS]          = "Se terminar válido e você começou a rodada com mais fichas que o oponente: +4.",
    [D_TUDO_NADA]       = "Tirando 2: cada dado válido da sequência pontua +5. Tirando 1: cada dado válido da sequência pontua "
                          "-5.",
    [D_MALDITO]         = "Se tirar 7 ou menos, anula a sua sequência inteira, este dado incluído.",
    [D_GATUNO]          = "Se terminar válido, rouba um dado do oponente (o menor) para cada 1 tirado por outro dado seu válido.",
    [D_QUEBRADO]        = "Não é sorteado: pontua o número gravado nele. Quebra ao ser lançado.",
    [D_INVEJOSO]        = "Se terminar válido com 5 ou 6, anula o maior dado do oponente. Com 1 ou 2, anula os seus 2 menores "
                          "dados.",
    [D_CARIDOSO]        = "Se terminar válido com 3 ou 4, rouba o menor dado do oponente.",
    [D_FARTURA]         = "Se terminar válido, seus dados válidos que tiraram 1 ou 2 pontuam x2 no fim.",
    [D_SORTE]           = "Se terminar válido, tirando 1: cada dado ímpar seu pontua +1. Tirando 2: cada dado par seu pontua "
                          "+1.",
    [D_TEIMOSO]         = "Nunca é anulado. Pode ser roubado.",
    [D_SEMENTE]         = "Se terminar válido e seus dados válidos somarem 14 ou mais no fim: +7.",
    [D_FOGO]            = "Se terminar válido, o seu maior dado queima e sai da bolsa. Tirando 2, ainda +10.",
    [D_MALDICAO]        = "Se terminar válido, pontua para você e tira o mesmo número de pontos do oponente.",
    [D_TREVO]           = "Tirando 4: SORTE, pontua 10. Tirando 1, 2 ou 3: pontua 0.",
    [D_JACKPOT]         = "Se terminar válido: dois dados válidos seus com o mesmo número, +4; três, +10.",
    [D_COPIADOR]        = "Se terminar válido, cada outro dado válido seu com o mesmo número pontua +2.",
    [D_EGOISTA]         = "Anula todos os seus outros dados, menos a si mesmo. Pode ser anulado.",
    [D_LASER]           = "Se terminar válido com 5 ou 6, anula um dado do oponente, ao acaso.",
    [D_VENTANIA]        = "Ao ser lançado, rola de novo o primeiro dado da sua sequência.",
    [D_MISERICORDIOSO]  = "Se terminar válido, pontua +2 por dado seu anulado na rodada.",
    [D_SOLITARIO]       = "Se terminar válido, sem dado válido logo antes nem logo depois dele: +6.",
    [D_SENTINELA]       = "Se for anulado pela sequência, só ele é anulado: o dado anterior continua válido.",
    [D_FERREIRO]        = "Não pontua. Tirando 3 ou 4, o próximo dado pontua +6, se os dois terminarem válidos.",
    [D_COBRADOR]        = "Se o oponente começou a rodada com mais fichas que você: +4.",
    [D_DESAFIANTE]      = "Se for o maior dado válido da rodada, seu ou do oponente: +4 no fim.",
    [D_VIDRO]           = "Se tirar 1, quebra: é anulado, anula o dado anterior e sai da bolsa.",
    [D_ACUMULADOR]      = "Cada vez que entra válido na sequência, guarda +1 de bônus para as próximas vezes (até +6).",
    [D_SEGURANCA]       = "Se estiver válido, é anulado ou roubado no lugar dos seus outros dados. Válido no fim: +2.",
    [D_BUMERANGUE]      = "Se tirar 1 ou 2, volta e rola de novo, uma vez.",
    [D_GEMEO]           = "Tira o mesmo número do último dado válido da sequência, até 6 (senão, rola normalmente).",
    [D_PRISMA]          = "Se terminar válido, +1 no fim para cada tipo de dado entre os seus válidos: d2, d4, d6, d8, d12, d20.",
    [D_FENIX]           = "Se terminar anulado, renasce no fim e pontua o número que tirou.",
    [D_TRONO]           = "Se for o 4º dado da sequência e os quatro terminarem válidos: +4.",
    [D_CAVALO]          = "Se o oponente anular ou roubar este dado, todos os dados do oponente são anulados.",
    [D_BERSERKER]       = "Se terminar válido, anula no fim um outro dado seu, ao acaso.",
    [D_VIUVA]           = "Se terminar como o único dado válido seu, pontua x2 no fim.",
    [D_CAPITAO]         = "Se terminar válido, +1 no fim por dado válido seu, este dado incluído.",
    [D_CHECKPOINT]      = "Se terminar válido: +3 no fim para cada dado seu lançado depois dele que terminar válido.",
    [D_SORTE_REVES]     = "Rola um d12 que soma e um d8 que subtrai. Pontua a diferença, até negativa.",
    [D_AUTOFAGIA]       = "Se terminar válido, come o seu menor dado (sai da bolsa até a próxima partida) e ganha +1 (até +3).",
    [D_ECO]             = "Tira o mesmo número do primeiro dado válido da sequência (até 6). Pode ser anulado pela sequência.",
    [D_DOMINO]          = "Se terminar válido e o próximo dado seu tirar o mesmo número: +3.",
    [D_ANCORA]          = "Se estiver válida, o próximo dado seu não pode ser anulado nem roubado.",
    [D_MARTIR]          = "Se terminar anulado, no fim cada dado válido seu pontua +2.",
    [D_PUXA_FILA]       = "Se for o 1º da sequência e os quatro terminarem válidos: +5.",
    [D_COVEIRO]         = "Se terminar válido: +1 no fim por dado anulado, seu ou do oponente.",
    [D_HEXA]            = "Se terminar válido, +1 no fim por dado de seis lados válido seu, este dado incluído.",
    [D_OURICO]          = "Se terminar válido: cada dado seu que o oponente anulou ou roubou custa 4 pontos a ele.",
    [D_CALIBRE]         = "Se estiver válido, o próximo dado seu tira +1, sem passar do máximo dele.",
    [D_SABOTADOR]       = "Se terminar válido com 5 ou 6: o maior dado do oponente pontua metade no fim.",
    [D_AZARAO]          = "Se você começou a rodada com menos fichas e ele terminar válido: x2.",
    [D_AMPULHETA]       = "Se terminar válido: +1 a cada 3 rodadas já jogadas, até +4.",
    [D_AGIOTA]          = "Se terminar válido, no fim 2 fichas do oponente passam para você, fora do pote.",
    [D_MEDUSA]          = "Se terminar válido, no fim os dados que você lançou, este incluído, viram Quebrados com o número que "
                          "tiraram.",
    [D_MISTERIOSO]      = "Um d20, de 1 a 20. Saiu de dentro de uma pedra.",
    [D_RANCOR]          = "Se terminar válido: +1 por rodada que você já perdeu nesta partida.",
    [D_ESPERANCA]       = "Se você começou a rodada com 15 fichas ou menos e este dado terminar válido: x2.",
    [D_PONTE]           = "Não pode ser anulado pela sequência, e o dado seguinte compara com o dado anterior a ele.",
    [D_GARIMPEIRO]      = "Se terminar válido, no fim os outros dados válidos seus voltam direto para a bolsa.",
    [D_POTE_QUENTE]     = "Se terminar válido: +1 a cada 25 fichas no pote, no fim da rodada.",
    [D_FANTASMA]        = "Pontua sempre o número marcado (1). Não quebra ao ser lançado: só some no fim da partida.",
    [D_PREVISIVEL]      = "Não é sorteado: tira 1 na primeira vez, 2 na segunda, e assim até 12. Recomeça a cada partida.",
    [D_OCULTO]          = "Ninguém vê o que ele tirou até o fim da rodada: só então entra na sequência e pode ser anulado ou "
                          "anular.",
    [D_SACRIFICIO]      = "Não pontua. Se terminar válido, no fim o seu menor dado válido (entre os que pontuam) pontua x2.",
    [D_VAZIO]           = "Pontua sempre 0 e nunca é anulado. Não anula o dado anterior: depois dele, qualquer número é válido.",
    [D_MEDIA]           = "Se terminar válido, ganha no fim o total dos seus dados dividido por 4 (para baixo).",
    [D_VAMPIRO]         = "Se terminar válido, no fim cada dado válido do oponente perde 1 ponto.",
    [D_COLECIONADOR]    = "Tirando 6, ganha um bônus da metade dos dados que você tem (Fantasmas contam).",
    [D_PACTO]           = "Doze lados. Cada vez que é lançado, um Fantasma (pontua 1) entra na sua bolsa.",
    [D_ASSOMBRACAO]     = "Se terminar válido, no fim um Fantasma (pontua 1) entra na bolsa do oponente.",
    [D_FICHA_VELHA]     = "Se terminar a rodada anulado (pela sequência ou por outro dado), pontua +4 no fim.",
    [D_SANGUESSUGA]     = "Ao ser lançado, anula o dado anterior na sequência. Este dado não pode ser anulado.",
    [D_TRENO]           = "Se terminar válido, os seus dados válidos antes dele pontuam +1, +2 e +3, do primeiro ao último.",
    [D_BONECO]          = "Se terminar válido tirando 8, congela e anula o 4º dado da sequência do oponente.",
    [D_JINGLE]          = "Lançado, uma cópia dele entra na sua bolsa. Com 2 ou mais válidos na sequência: +4 em cada um.",
    [D_FURIA]           = "Tirando 4, guarda +2 de bônus; tirando 1, perde 1. O bônus soma no que ele tira, até negativo.",
    [D_ONDA]            = "Não pontua. Cada vez que termina válido, guarda o que tirou; ao juntar 9, vira um Tsunami.",
    [D_TSUNAMI]         = "Se terminar válido tirando de 9 a 12, anula um dado do oponente, ao acaso.",
    [D_PATINHOS]        = "Na 3ª, 6ª, 9ª... vez que termina válido, pontua x3. Os patinhos marcam a contagem.",
    [D_PRESENTE]        = "Ao ser lançado, desembrulha e vira um dado raro ao acaso, que fica no lugar dele na bolsa.",
    [D_PANETONE]        = "Cada vez que é lançado, a fatia vem com 0 a 3 frutas: +0 a +3 no que tirou.",
    [D_PISCA]           = "Lançado aceso, pontua x2; na vez seguinte, apagado, pontua 0. O dado mostra como vem.",
    [D_MEIA]            = "Cada vez que termina anulada, guarda +2. Válida, paga tudo o que guardou e esvazia.",
    [D_ESTRELA]         = "Se terminar válida como o último dado lançado, +2 por dado válido abaixo dela.",
    [D_RENA]            = "Se terminar válida, os dados válidos logo antes e logo depois dela pontuam +1.",
    [D_REVOLVER]        = "Cada vez que é lançado, valendo ou não, carrega uma bala. Na 6ª vez, o tambor cheio dispara: +12, e esvazia.",
    [D_ASTRO]           = "Tirando 5 ou 6, evolui, valendo ou não, e volta para a sua mão na rodada seguinte. Valendo, +1.",
    [D_CONSTELACAO]     = "Tirando 6, 7 ou 8, evolui, valendo ou não, e volta para a sua mão na rodada seguinte. Valendo, +3.",
    [D_GALAXIA]         = "Tirando de 9 a 12, evolui, valendo ou não, e volta para a sua mão na rodada seguinte. Valendo, +5.",
    [D_BURACO]          = "Tirando de 14 a 20, evolui, valendo ou não, e volta para a sua mão na rodada seguinte. Valendo, anula os seus dados ao lado dele.",
    [D_UNIVERSO]        = "Só pode ser lançado sozinho. Não pode ser anulado nem roubado.",
};

static const char *TEXTO_CAT[D_N] = {
    [D_D2]              = "Uma moeda: sai 1 ou 2.",
    [D_D4]              = "Quatro lados, de 1 a 4.",
    [D_D6]              = "Seis lados, de 1 a 6.",
    [D_D8]              = "Oito lados, de 1 a 8.",
    [D_D12]             = "Doze lados, de 1 a 12.",
    [D_D20]             = "Vinte lados, de 1 a 20.",
    [D_DOBRO]           = "Seis lados. Se terminar válido, tirando de 1 a 3: o próximo dado da sequência pontua x2 no fim da "
                          "rodada, se também terminar válido. Tirando de 4 a 6, pontua normalmente.",
    [D_VICIADO]         = "Seis lados. Rola duas vezes e fica com o maior resultado. O menor aparece na mesa e é descartado.",
    [D_LASTRO]          = "Doze lados, mas só sai de 4 a 12, cada número com a mesma chance (1 em 9).",
    [D_PAR]             = "Quatro lados. Se tirar o mesmo número do último dado válido da sequência, pontua +6.",
    [D_ESCUDO]          = "Seis lados. Nunca é anulado. Nunca é roubado.",
    [D_CARRASCO]        = "Quatro lados. Se terminar válido, tirando 4: anula o maior dado do oponente. Tirando 1, mesmo se "
                          "terminar anulado: anula o seu menor dado.",
    [D_EXPLOSIVO]       = "Oito lados. Tirando 8, rola de novo e soma a pontuação da nova rolagem ao 8 que já tinha. Pode rolar "
                          "de novo, a cada 8, até 3 vezes seguidas (pontuação máxima: 32).",
    [D_MARTELO]         = "Quatro lados. Se terminar válido, tirando 3 ou 4: tira 4 pontos do oponente. Tirando 1 ou 2, pontua "
                          "normalmente.",
    [D_PIRATA]          = "Oito lados. Se terminar válido, tirando 7 ou 8: tira 4 pontos do oponente. Os pontos não passam para "
                          "você.",
    [D_ESPELHO]         = "Seis lados. Se tirar 1 ou 2, vira o número do maior dado do oponente. Se o oponente ainda não jogou, "
                          "o Espelho permanece válido até o fim do turno do oponente, vira no fim da rodada e pode ser anulado "
                          "pela sequência.",
    [D_FICHAS]          = "Quatro lados. Se terminar válido e você começou a rodada com mais fichas que o oponente, pontua +4.",
    [D_TUDO_NADA]       = "Uma moeda de risco. Tirando 2 e terminando válido: cada dado válido da sequência pontua +5. Tirando "
                          "1, mesmo se terminar anulado: cada dado válido da sequência pontua -5.",
    [D_MALDITO]         = "Vinte lados. Se tirar 7 ou menos, anula a sua sequência inteira, este dado incluído. Tirando 8 ou "
                          "mais, pontua normalmente.",
    [D_GATUNO]          = "Seis lados. Se terminar válido, rouba um dado do oponente para cada 1 tirado por outro dado válido "
                          "da sua sequência (o anulado não conta): o menor, depois o menor que sobrou, e assim por diante. Os "
                          "pontos dos roubados passam para você.",
    [D_QUEBRADO]        = "Oito lados, mas não é sorteado: pontua sempre o número gravado nele. Depois de lançado, quebra e sai "
                          "da sua bolsa.",
    [D_INVEJOSO]        = "Seis lados. Se terminar válido, tirando 5 ou 6: anula o maior dado do oponente. Se terminar válido, "
                          "tirando 1 ou 2: anula os seus 2 menores dados. Tirando 3 ou 4, pontua normalmente.",
    [D_CARIDOSO]        = "Quatro lados. Se terminar válido, tirando 3 ou 4, rouba o menor dado do oponente: os pontos dele "
                          "passam para você. Se tirar 1 ou 2, pontua normalmente, mas não rouba dado do oponente.",
    [D_FARTURA]         = "Quatro lados. Se terminar válido, no fim da rodada cada dado válido seu que tirou 1 ou 2 pontua x2, "
                          "este dado incluído.",
    [D_SORTE]           = "Uma moeda. Se terminar válido, tirando 1: cada dado válido seu com número ímpar pontua +1. Tirando "
                          "2: cada dado válido seu com número par pontua +1. Conta a si mesma.",
    [D_TEIMOSO]         = "Seis lados. Nunca é anulado. Pode ser roubado.",
    [D_SEMENTE]         = "Uma moeda. Se terminar válido e, no fim da rodada, os seus dados válidos somarem 14 ou mais (com "
                          "bônus e roubos), pontua +7.",
    [D_FOGO]            = "Uma moeda. Se terminar válido, o seu maior dado vira cinza no fim da rodada e sai da sua bolsa para "
                          "sempre. Tirando 2, ainda pontua +10; tirando 1, nada.",
    [D_MALDICAO]        = "Quatro lados. Se terminar válido, pontua para você e tira do oponente o mesmo número de pontos que "
                          "tirou.",
    [D_TREVO]           = "Quatro lados. Tirando 4: SORTE, pontua 10. Tirando 1, 2 ou 3, pontua 0. Para a sequência, conta o "
                          "número que saiu.",
    [D_JACKPOT]         = "Quatro lados. Se terminar válido e dois dados válidos seus mostrarem o mesmo número (este dado pode "
                          "ser um deles), pontua +4 no fim da rodada; se forem três: JACKPOT, +10.",
    [D_COPIADOR]        = "Seis lados. Se terminar válido, no fim da rodada cada outro dado válido seu com o mesmo número "
                          "pontua +2.",
    [D_EGOISTA]         = "Doze lados. Ao ser lançado, anula os outros dados da sequência, antes e depois dele, menos a si "
                          "mesmo. Este dado pode ser anulado.",
    [D_LASER]           = "Seis lados. Se terminar válido com 5 ou 6, um raio verde anula um dado do oponente escolhido ao "
                          "acaso.",
    [D_VENTANIA]        = "Oito lados. Ao ser lançado, rola de novo o primeiro dado da sua sequência, e a sequência é refeita: "
                          "dados podem ser anulados ou voltar a ser válidos. Se for o primeiro da sequência, não faz nada.",
    [D_MISERICORDIOSO]  = "Quatro lados. Se terminar válido, pontua +2 no fim da rodada para cada dado seu anulado.",
    [D_SOLITARIO]       = "Quatro lados. Se terminar válido e os dados vizinhos dele na sequência (o anterior e o seguinte) não "
                          "forem válidos, pontua +6 no fim da rodada. Na ponta da sequência, só conta o vizinho que existe.",
    [D_SENTINELA]       = "Quatro lados. Se tirar menos que o último dado válido, só ele é anulado: o dado anterior continua "
                          "válido.",
    [D_FERREIRO]        = "Quatro lados. Não pontua, mas conta na sequência com o número que tirou. Tirando 3 ou 4, o próximo "
                          "dado da sequência pontua +6 no fim da rodada, se os dois terminarem válidos.",
    [D_COBRADOR]        = "Quatro lados. Pontua +4 se o oponente começou a rodada com mais fichas que você.",
    [D_DESAFIANTE]      = "Oito lados. Se terminar válido e for maior que todos os outros dados válidos da rodada, seus e do "
                          "oponente, pontua +4 no fim da rodada. Empate não conta.",
    [D_VIDRO]           = "Oito lados. Tirando 1, quebra: é anulado, anula também o último dado válido seu e sai da sua bolsa "
                          "para sempre.",
    [D_ACUMULADOR]      = "Oito lados. Cada vez que entra válido na sequência, guarda +1 de bônus para as próximas vezes, até "
                          "+6. O bônus fica com ele de rodada em rodada e soma ao número que tirou, se ele terminar válido.",
    [D_SEGURANCA]       = "Quatro lados. Se o oponente anular ou roubar um dado seu e o Segurança estiver válido, quem é "
                          "anulado ou roubado é o Segurança, e o outro dado fica. Se terminar válido, pontua +2 no fim da "
                          "rodada.",
    [D_BUMERANGUE]      = "Seis lados. Se tirar 1 ou 2, volta e rola de novo uma vez; fica o segundo número.",
    [D_GEMEO]           = "Seis lados. Tira o mesmo número do último dado válido da sequência. Se o último dado válido da "
                          "sequência passar de 6, ou se não houver um dado válido antes do Gêmeo, ele rola normalmente.",
    [D_PRISMA]          = "Oito lados. Se terminar válido, pontua +1 no fim da rodada para cada tipo de dado diferente entre os "
                          "seus válidos, pelo número de lados (d2, d4, d6, d8, d12 e d20), este dado incluído: com um d4, um d6 "
                          "e um d20 válidos, +4.",
    [D_FENIX]           = "Seis lados. Se terminar anulado, renasce no fim da rodada e pontua o número que tirou. Se for "
                          "roubado, não volta.",
    [D_TRONO]           = "Doze lados. Se for o 4º dado da sua sequência e os quatro terminarem válidos, este dado incluído, "
                          "pontua +4 no fim da rodada.",
    [D_CAVALO]          = "Oito lados. Se um dado do oponente anular ou roubar este dado, todos os dados válidos do oponente "
                          "são anulados, e o que ele roubou na rodada deixa de contar.",
    [D_BERSERKER]       = "Vinte lados. Se terminar válido, no fim da rodada anula um outro dado válido seu, escolhido ao "
                          "acaso.",
    [D_VIUVA]           = "Oito lados. Se terminar a rodada como o único dado válido seu, pontua x2 no fim.",
    [D_CAPITAO]         = "Oito lados. Se terminar válido, pontua +1 no fim da rodada para cada dado válido seu, este dado "
                          "incluído.",
    [D_CHECKPOINT]      = "Oito lados. Se terminar válido, no fim da rodada cada dado seu lançado depois dele que também "
                          "terminar válido pontua +3.",
    [D_SORTE_REVES]     = "Dois dados em um: rola o d12 da Sorte e o d8 do Revés, e pontua a Sorte menos o Revés (de -7 a 11). "
                          "Negativo, tira pontos da sequência, e o dado seguinte só precisa ser maior que ele.",
    [D_AUTOFAGIA]       = "Doze lados. Se terminar válido, no fim da rodada anula o outro dado válido seu de menor número, que "
                          "sai da bolsa até o fim da partida, e a Autofagia ganha +1 nas rodadas seguintes, até +3. Se você "
                          "tiver menos de 8 dados, ou ela já tiver +3, só anula.",
    [D_ECO]             = "Seis lados. Tira o mesmo número do primeiro dado válido da sequência. Se esse número passar de 6, ou "
                          "se ele for o primeiro, rola normalmente. Se o último dado válido for maior, o Eco é anulado pela "
                          "sequência.",
    [D_DOMINO]          = "Seis lados. Se terminar válido e o dado seu lançado logo depois dele tirar o mesmo número, pontua +3 "
                          "no fim da rodada.",
    [D_ANCORA]          = "Quatro lados. O dado seu lançado logo depois da Âncora, com ela válida, não pode ser anulado (nem "
                          "pela sequência, nem por outro dado) nem roubado. Se ele for menor, quem é anulada pela sequência é a "
                          "Âncora.",
    [D_MARTIR]          = "Quatro lados. Se terminar a rodada anulado, cada dado válido seu pontua +2 no fim.",
    [D_PUXA_FILA]       = "Oito lados. Se for o primeiro dado da sua sequência e os quatro terminarem válidos, este dado "
                          "incluído, pontua +5 no fim da rodada.",
    [D_COVEIRO]         = "Seis lados. Se terminar válido, pontua +1 no fim da rodada para cada dado anulado na mesa, seus e "
                          "do oponente.",
    [D_HEXA]            = "Seis lados. Se terminar válido, pontua +1 no fim da rodada para cada dado de seis lados válido seu, "
                          "este dado incluído.",
    [D_OURICO]          = "Seis lados. Se terminar válido, cada dado seu que um dado do oponente anulou ou roubou na rodada "
                          "custa 4 pontos ao oponente no fim.",
    [D_CALIBRE]         = "Quatro lados. O dado seu lançado logo depois dele, com o Calibre válido, tira 1 a mais, sem passar "
                          "do máximo: o Laser com 4 vira 5, o Trevo com 3 vira 4.",
    [D_SABOTADOR]       = "Seis lados. Se terminar válido com 5 ou 6, o maior dado válido do oponente passa a pontuar metade no "
                          "fim da rodada (arredondado para baixo).",
    [D_AZARAO]          = "Quatro lados. Se você começou a rodada com menos fichas que o oponente e ele terminar válido, pontua "
                          "x2.",
    [D_AMPULHETA]       = "Seis lados. Se terminar válido, pontua +1 no fim da rodada a cada 3 rodadas já jogadas na partida, "
                          "até +4.",
    [D_AGIOTA]          = "Quatro lados. Se terminar válido, no fim da rodada 2 fichas do oponente passam para você, fora do "
                          "pote, ganhe quem ganhar.",
    [D_MEDUSA]          = "Doze lados. Se terminar válido, no fim da rodada todos os dados da sua sequência (os anulados "
                          "também; os roubados, não), este dado incluído, viram pedra: cada um vira um Quebrado com o número "
                          "que tirou, que sai uma vez com esse número e quebra.",
    [D_MISTERIOSO]      = "Vinte lados, de 1 a 20, todo preto. Estava dentro de uma pedra que tinha um número negativo; não faz "
                          "nada além de rolar.",
    [D_RANCOR]          = "Quatro lados. Se terminar válido, pontua +1 no fim da rodada para cada rodada que você já perdeu na "
                          "partida (correr conta como perder; empate, não).",
    [D_ESPERANCA]       = "Oito lados. Se você começou a rodada com 15 fichas ou menos e este dado terminar válido, pontua x2 "
                          "no fim.",
    [D_PONTE]           = "Quatro lados. Este dado não pode ser anulado pela sequência, mas pode ser anulado por outro dado. A "
                          "sequência o ignora: o dado seguinte só precisa igualar ou passar o último dado válido antes dele.",
    [D_GARIMPEIRO]      = "Seis lados. Se terminar válido, no fim da rodada os outros dados válidos da sua sequência voltam "
                          "direto para a bolsa e podem sair de novo já na próxima rodada.",
    [D_POTE_QUENTE]     = "Oito lados. Se terminar válido, pontua +1 no fim da rodada a cada 25 fichas no pote.",
    [D_FANTASMA]        = "Seis lados, mas não é sorteado: pontua sempre o número marcado nele (1). Não quebra ao ser lançado: "
                          "volta para a bolsa como os outros e só some no fim da partida.",
    [D_PREVISIVEL]      = "Doze lados, mas não é sorteado: na primeira vez que é lançado na partida tira 1, na segunda 2, e "
                          "assim por diante, até 12. Sobe a cada lance, válido ou não. Recomeça do 1 a cada partida.",
    [D_OCULTO]          = "Oito lados. O que ele tirou fica escondido (um \"?\") para os dois até o fim da rodada. Enquanto isso "
                          "não é anulado, não anula nenhum dado e não conta na sequência: o dado seguinte compara com o dado "
                          "anterior a ele. No fim ele aparece e a sequência é refeita: pode ser válido, ser anulado ou anular o "
                          "dado anterior.",
    [D_SACRIFICIO]      = "Quatro lados, mas não pontua. Se terminar válido, no fim da rodada o seu dado válido de menor "
                          "número, entre os que pontuam, pontua x2.",
    [D_VAZIO]           = "Seis lados, mas pontua sempre 0. Nunca é anulado e nunca anula o dado anterior, por maior que ele "
                          "seja: depois dele, qualquer número é válido. Pode ser roubado.",
    [D_MEDIA]           = "Quatro lados. Se terminar válido, ganha no fim da rodada um bônus igual ao total dos seus dados "
                          "válidos (com bônus e multiplicadores, este dado incluído) dividido por 4, arredondado para baixo.",
    [D_VAMPIRO]         = "Quatro lados. Se terminar válido, com qualquer número, no fim da rodada cada dado válido do oponente "
                          "perde 1 ponto.",
    [D_COLECIONADOR]    = "Seis lados. Tirando 6, ganha um bônus da metade dos dados que você tem (arredondado para baixo). Os "
                          "Fantasmas que entraram na sua bolsa contam.",
    [D_PACTO]           = "Doze lados. Cada vez que é lançado, válido ou não, um Fantasma (pontua sempre 1) entra na sua bolsa "
                          "até o fim da partida.",
    [D_ASSOMBRACAO]     = "Seis lados. Se terminar válido, no fim da rodada um Fantasma (pontua sempre 1) entra na bolsa do "
                          "oponente até o fim da partida.",
    [D_FICHA_VELHA]     = "Uma ficha gasta, de 1 ou 2. Se terminar válido, pontua o que tirou, como uma moeda. Se terminar a "
                          "rodada anulado, pela sequência ou por um dado do oponente, pontua +4 no fim, fora dos "
                          "multiplicadores.",
    [D_SANGUESSUGA]     = "Oito lados. Ao ser lançado, anula o dado anterior na sequência (se ele puder ser anulado), e a "
                          "sequência segue a partir dela. Este dado não pode ser anulado.",
    [D_TRENO]           = "Oito lados. Se terminar válido, puxa os seus dados válidos que vieram antes dele na sequência: o "
                          "primeiro pontua +1, o segundo +2 e o terceiro +3 no fim da rodada. Só aparece de 20 de outubro "
                          "a 6 de janeiro.",
    [D_BONECO]          = "Oito lados. Se terminar válido tirando 8, no fim da rodada congela e anula o 4º dado da sequência "
                          "do oponente, se ele lançou um 4º e esse dado puder ser anulado. Só aparece de 20 de outubro a 6 de janeiro.",
    [D_JINGLE]          = "Seis lados. Cada vez que é lançado, válido ou não, uma cópia dele (com os mesmos adesivos) entra na "
                          "sua bolsa até o fim da partida. Com 2 ou mais Jingle Bells válidos na sequência, os sinos tocam: "
                          "cada um pontua +4 no fim, fora dos multiplicadores. Só aparece de 20 de outubro a 6 de janeiro.",
    [D_FURIA]           = "Quatro lados. Cada vez que tira 4, guarda +2 de bônus; cada vez que tira 1, perde 1. O bônus "
                          "guardado, já com o lance de agora, soma no que ele tira, de rodada em rodada, sem limite, e pode "
                          "ficar negativo. Recomeça a cada partida.",
    [D_ONDA]            = "Quatro lados, mas não pontua: conta na sequência com o número que tirou. Cada vez que termina "
                          "válido, guarda esse número. Quando o que guardou chega a 9, no fim da rodada vira um Tsunami, de "
                          "vez.",
    [D_TSUNAMI]         = "Doze lados. Não é vendido nem sorteado: nasce de uma Onda que juntou 9. Se terminar válido tirando "
                          "de 9 a 12, uma onda anula um dado do oponente, escolhido ao acaso.",
    [D_PATINHOS]        = "Quatro lados. Conta as vezes que termina válido: na 3ª, na 6ª, na 9ª... pontua x3 no fim da "
                          "rodada. Os três patinhos no dado marcam a contagem; com os três acesos, ele está ativo. A "
                          "contagem fica com ele.",
    [D_PRESENTE]        = "Seis lados, embrulhado. Ao ser lançado, desembrulha e vira um dado raro sorteado (no Desafiante, "
                          "só entre os que você já abriu): o Presente sai e o dado novo fica no lugar dele na bolsa - no "
                          "Desafiante, pela run inteira; nas outras partidas, até o fim da partida. Só aparece de 20 de "
                          "outubro a 6 de janeiro.",
    [D_PANETONE]        = "Oito lados. Cada vez que é lançado, a fatia vem com 0, 1, 2 ou 3 frutas cristalizadas, ao acaso: "
                          "esse bônus soma no que ele tirou, se terminar válido. Só aparece de 20 de outubro a 6 de janeiro.",
    [D_PISCA]           = "Quatro lados. Alterna a cada vez que é lançado: aceso, pontua x2; apagado, pontua 0 (mas conta "
                          "na sequência com o número que tirou). As luzinhas no dado mostram como ele vem na próxima vez. "
                          "Só aparece de 20 de outubro a 6 de janeiro.",
    [D_MEIA]            = "Quatro lados. Cada vez que termina a rodada anulada, guarda +2 na meia. Quando termina válida, "
                          "paga tudo o que guardou no fim da rodada, fora dos multiplicadores, e esvazia. Só aparece em "
                          "20 de outubro a 6 de janeiro.",
    [D_ESTRELA]         = "Oito lados. Se terminar válida e for o último dado que você lançou na rodada (a estrela no "
                          "topo da árvore), pontua +2 no fim para cada dado seu válido abaixo dela. Só aparece em "
                          "20 de outubro a 6 de janeiro.",
    [D_RENA]            = "Seis lados. Se terminar válida, o dado seu lançado logo antes e o lançado logo depois dela, "
                          "se terminarem válidos, pontuam +1 cada. Só aparece de 20 de outubro a 6 de janeiro.",
    [D_REVOLVER]        = "Seis lados. Cada vez que é lançado, termine válido ou anulado, carrega uma bala no tambor (as "
                          "balas aparecem no dado). Na sexta vez, com o tambor cheio, dispara: +12 - somado no que tirou, "
                          "se ficar válido; anulado, o tiro já saiu e os +12 vêm no fim da rodada. Depois o tambor esvazia "
                          "para a próxima volta. As balas ficam com ele.",
    [D_ASTRO]           = "Seis lados. Tirando 5 ou 6, evolui, valendo ou não: na rodada seguinte, volta para a sua mão "
                          "(e os adesivos dela caem). Se ficar válida, ainda pontua +1 no que tirou. A cada partida, "
                          "recomeça como Estrela.",
    [D_CONSTELACAO]     = "Oito lados. Tirando 6, 7 ou 8, evolui, valendo ou não: na rodada seguinte, volta para a sua mão. "
                          "Se ficar válida, ainda pontua +3 no que tirou. A cada partida, recomeça como Estrela.",
    [D_GALAXIA]         = "Doze lados. Tirando de 9 a 12, evolui, valendo ou não: na rodada seguinte, volta para a sua mão. "
                          "Se ficar válida, ainda pontua +5 no que tirou. A cada partida, recomeça como Estrela.",
    [D_BURACO]          = "Vinte lados. Tirando de 14 a 20, evolui, valendo ou não: na rodada seguinte, volta para a sua mão. "
                          "Se ficar válido, no fim da rodada ainda anula os seus dados lançados logo antes e logo depois "
                          "dele (os que não caem, ficam). A cada partida, recomeça como Estrela.",
    [D_UNIVERSO]        = "Cem lados. Só pode ser lançado sozinho, sem nenhum outro dado na sequência. Não pode ser anulado "
                          "nem roubado. A cada partida, recomeça como Estrela.",
};

static const char *RARIDADE[] = { "comum", "incomum", "raro", "lendário" };
// Cor de cada raridade nas etiquetas, a de um metal: ferro, bronze, prata
// e ouro.
static const uint16_t COR_RAR[] = { RGB(128, 120, 112), RGB(156, 106, 64),
                                    RGB(184, 194, 208), RGB(228, 194, 106) };
static uint16_t cor_rar(int tipo) { return COR_RAR[TIPO[tipo].raridade]; }

// "moeda", "D4" ... "D20" (e o "D100" do Universo): o tipo, sempre escrito do mesmo jeito.
static const char *nome_tipo(int tipo)
{
    switch (TIPO[tipo].lados) {
    case 2:   return "moeda";
    case 4:   return "D4";
    case 6:   return "D6";
    case 8:   return "D8";
    case 12:  return "D12";
    case 100: return "D100";
    default:  return "D20";
    }
}

static bool igual_sem_caixa(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if ((*a | 32) != (*b | 32)) return false;
    return *a == *b;
}

// "D8 · raro", na cor da raridade.
static void classe(int tipo, char *s, int n)
{
    // O dado comum ja se chama pelo tipo ("D6"): a etiqueta diz so a raridade.
    if (igual_sem_caixa(TIPO[tipo].nome, nome_tipo(tipo))) snprintf(s, (size_t)n, "%s", RARIDADE[TIPO[tipo].raridade]);
    else snprintf(s, (size_t)n, "%s · %s", nome_tipo(tipo), RARIDADE[TIPO[tipo].raridade]);
}

// O catalogo mostra os dados do mais comum ao mais raro, menos o oculto e,
// fora da epoca, os de Natal.
static bool no_catalogo(int t) { return !oculto(t) && !fora_de_epoca(t) && !cosmo_oculto(t); }
static int n_catalogo(void)
{
    int n = 0;
    for (int t = 0; t < D_N; t++) n += no_catalogo(t);
    return n;
}
static int cat_tipo(int i)
{
    for (int r = RAR_COMUM; r <= RAR_LENDA; r++)
        for (int t = 0; t < D_N; t++)
            if (TIPO[t].raridade == r && no_catalogo(t) && i-- == 0) return t;
    return 0;
}

// O que a loja da partida rapida vende: N_LOJA destes, embaralhados nesta
// ordem - so comuns e incomuns, como o premio das primeiras rodadas. Precos
// (TIPO.preco) medidos com partidas IA x IA, 2000 por dado, com duas copias
// na bolsa: quem rende mais custa mais.
static const uint8_t VITRINE[] = { D_D8, D_PAR,
                                   D_DOBRO, D_VICIADO, D_QUEBRADO,
                                   D_FARTURA, D_SORTE, D_TEIMOSO, D_SEMENTE, D_TREVO,
                                   D_JACKPOT, D_COPIADOR, D_MISERICORDIOSO, D_SOLITARIO,
                                   D_SENTINELA, D_FERREIRO, D_COBRADOR,
                                   D_VIDRO, D_SEGURANCA };

// A cara de cada dado especial: a cor dele (o aro e o nome nas telas), o tom
// que tinge o corpo e as cores do desenho de dentro (desenho_interno). Tudo
// entra misturado ao corpo do dono, em %: claro no marfim, escuro no ebano,
// sempre nas cores da paleta. O dado comum fica em branco: aro de latao.
typedef struct {
    uint16_t cor;                // aro e nome
    uint16_t tom;  uint8_t pct;  // o corpo, tingido
    uint16_t c1;   uint8_t p1;   // o desenho
    uint16_t c2;   uint8_t p2;   // um segundo e um terceiro tom do desenho
    uint16_t c3;   uint8_t p3;
} visual_t;

#define V_LATAO   RGB(190, 156, 86)
#define V_OURO    RGB(228, 194, 106)
#define V_LATAO_E RGB(150, 122, 62)
#define V_VINHO   RGB(150, 62, 80)
#define V_SANGUE  RGB(118, 30, 38)
#define V_VERM    RGB(190, 56, 60)
#define V_ROSA    RGB(208, 98, 108)
#define V_FOGO    RGB(214, 112, 44)
#define V_AMBAR   RGB(238, 162, 74)
#define V_TERRA   RGB(156, 106, 64)
#define V_COURO   RGB(120, 78, 50)
#define V_MADEIRA RGB(78, 46, 30)
#define V_PEDRA   RGB(166, 154, 132)
#define V_BARRO   RGB(96, 88, 80)
#define V_CHUMBO  RGB(70, 76, 88)
#define V_FERRO   RGB(104, 112, 124)
#define V_ACO     RGB(140, 150, 164)
#define V_PRATA   RGB(184, 194, 208)
#define V_CEU     RGB(168, 204, 230)
#define V_AZUL    RGB(56, 86, 150)
#define V_NOITE   RGB(96, 134, 196)
#define V_AGUA    RGB(72, 150, 138)
#define V_CIANO   RGB(96, 182, 198)
#define V_VERDE   RGB(88, 152, 84)
#define V_LIMA    RGB(130, 204, 112)
#define V_ROXO    RGB(102, 74, 138)
#define V_AMEIXA  RGB(150, 112, 160)
#define V_LILAS   RGB(206, 180, 218)
#define V_BREU    RGB(14, 14, 12)
#define V_BRANCO  RGB(252, 248, 236)
#define V_TRIGO   RGB(246, 222, 132)

static const visual_t VISUAL[D_N] = {
    //                 cor        tom           desenho        segundo e terceiro tons
    [D_DOBRO]      = { V_OURO,    0, 0,         V_LATAO_E, 100 },
    [D_VICIADO]    = { V_VERM,    0, 0,         V_CHUMBO, 40 },
    [D_LASTRO]     = { V_FERRO,   V_ACO, 34,    V_CHUMBO, 24 },
    [D_PAR]        = { V_AGUA,    0, 0,         V_AGUA, 60 },
    [D_ESCUDO]     = { V_ACO,     0, 0,         V_ACO, 34 },
    [D_CARRASCO]   = { V_SANGUE,  0, 0,         V_BREU, 75 },
    [D_EXPLOSIVO]  = { V_FOGO,    0, 0,         V_FOGO, 65 },
    [D_MARTELO]    = { V_VINHO,   0, 0,         V_ROSA, 100 },
    [D_PIRATA]     = { V_VINHO,   V_TERRA, 26,  V_MADEIRA, 30,  V_BREU, 85 },
    [D_ESPELHO]    = { V_PRATA,   V_PRATA, 55,  V_ACO, 28,      V_BRANCO, 55 },
    [D_FICHAS]     = { V_OURO,    0, 0,         V_LATAO, 75,    V_BRANCO, 90,  V_LATAO_E, 85 },
    [D_TUDO_NADA]  = { V_OURO },                // a moeda de ouro, desenhada a parte
    [D_MALDITO]    = { V_ROXO,    V_ROXO, 30,   RGB(54, 36, 74), 45, V_LILAS, 40 },
    [D_GATUNO]     = { V_CHUMBO,  0, 0,         V_BREU, 18 },
    [D_QUEBRADO]   = { V_TERRA },               // a rachadura vem por cima
    [D_INVEJOSO]   = { V_LIMA,    V_LIMA, 28,   V_VERDE, 65 },
    [D_CARIDOSO]   = { V_ROSA,    0, 0,         V_ROSA, 45 },
    [D_FARTURA]    = { V_AMBAR,   0, 0,         V_LATAO, 65 },
    [D_SORTE]      = { V_AZUL,    0, 0,         V_AZUL, 65,     V_VERM, 65 },
    [D_TEIMOSO]    = { V_ACO,     0, 0,         V_ACO, 34 },
    [D_SEMENTE]    = { V_VERDE,   0, 0,         V_VERDE, 75 },
    [D_FOGO]       = { V_FOGO },                // a ficha em chamas, desenhada a parte
    [D_MALDICAO]   = { V_ROXO,    V_ROXO, 20,   V_ROXO, 65 },
    [D_TREVO]      = { V_VERDE,   0, 0,         V_LIMA, 70 },
    [D_JACKPOT]    = { V_OURO,    V_OURO, 16 },  // os tres setes vem por baixo
    [D_COPIADOR]   = { V_CIANO,   V_CIANO, 18,  V_CIANO, 75 },
    [D_EGOISTA]    = { V_AMEIXA,  V_AMEIXA, 18, V_OURO, 55 },
    [D_LASER]      = { V_LIMA,    V_CHUMBO, 34, V_LIMA, 60 },
    [D_VENTANIA]   = { V_CEU,     V_CEU, 35,    V_NOITE, 45 },
    [D_MISERICORDIOSO] = { V_PRATA, V_PRATA, 20, V_LATAO, 100 },
    [D_SOLITARIO]  = { V_NOITE,   V_AZUL, 25,   RGB(246, 222, 132), 90 },
    [D_SENTINELA]  = { V_TERRA,   V_PEDRA, 46,  V_BARRO, 35,    V_BRANCO, 18 },
    [D_FERREIRO]   = { V_FERRO,   V_FERRO, 30,  V_BREU, 22 },
    [D_COBRADOR]   = { V_TERRA,   V_TERRA, 16,  V_TERRA, 35,    V_VINHO, 45 },
    [D_DESAFIANTE] = { V_VERM,    V_VERM, 35,   V_VERM, 65 },
    [D_VIDRO]      = { V_CEU,     V_CEU, 52,    V_BRANCO, 85 },
    [D_ACUMULADOR] = { V_AMBAR,   V_AMBAR, 20 }, // as seis casas vem por cima
    [D_SEGURANCA]  = { V_NOITE,   0, 0,         V_AZUL, 50 },
    [D_BUMERANGUE] = { V_TERRA,   V_TERRA, 18,  V_COURO, 55 },
    [D_GEMEO]      = { V_AGUA,    0, 0,         V_AGUA, 50 },
    [D_PRISMA]     = { V_LILAS,   0, 0,         V_ROSA, 45,     V_OURO, 45,    V_NOITE, 45 },
    [D_FENIX]      = { V_FOGO,    V_FOGO, 14,   V_AMBAR, 60,    V_FOGO, 60 },
    [D_TRONO]      = { V_OURO,    V_AMEIXA, 18, V_OURO, 65 },
    [D_CAVALO]     = { V_TERRA,   V_TERRA, 30,  V_MADEIRA, 40 },
    [D_BERSERKER]  = { V_VINHO,   0, 0,         V_SANGUE, 50 },
    [D_VIUVA]      = { V_ACO,     0, 0,         V_ACO, 45 },
    [D_CAPITAO]    = { V_AZUL,    V_AZUL, 14,   V_LATAO_E, 100 },
    [D_CHECKPOINT] = { V_VERDE,   V_VERDE, 10,  V_BREU, 85,     V_BRANCO, 100 },
    [D_SORTE_REVES] = { V_AMEIXA, 0, 0,         V_BREU, 84,     V_OURO, 100 },
    [D_AUTOFAGIA]  = { V_AGUA,    V_AGUA, 10,   V_VERDE, 100,   V_BREU, 100,   RGB(34, 76, 44), 100 },
    [D_ECO]        = { V_LILAS,   V_LILAS, 12,  V_ROXO, 90,     V_ROXO, 62,    V_ROXO, 38 },
    [D_DOMINO]     = { V_BREU,    0, 0,         V_BREU, 70,     V_LATAO, 100 },
    [D_ANCORA]     = { V_AZUL,    V_AZUL, 12,   V_AZUL, 85,     V_FERRO, 80 },
    [D_MARTIR]     = { V_SANGUE,  V_PEDRA, 12,  V_SANGUE, 100,  V_BREU, 75,    V_VERM, 100 },
    [D_PUXA_FILA]  = { V_VERM,    V_VERM, 10,   V_VERM, 75,     V_FOGO, 100 },
    [D_COVEIRO]    = { V_BARRO,   V_PEDRA, 12,  V_BARRO, 62,    V_BREU, 70,    V_PEDRA, 100 },
    [D_HEXA]       = { V_AMBAR,   V_TRIGO, 30,  V_AMBAR, 70 },
    [D_OURICO]     = { V_COURO,   V_TERRA, 22,  V_COURO, 85 },
    [D_CALIBRE]    = { V_LIMA,    V_LIMA, 10,   V_VERM, 85 },
    [D_SABOTADOR]  = { V_ROXO,    V_ROXO, 12,   V_FERRO, 100,   V_BREU, 85,    V_PRATA, 100 },
    [D_AZARAO]     = { V_VERDE,   V_VERDE, 12,  V_VERDE, 85 },
    [D_AMPULHETA]  = { V_MADEIRA, V_TRIGO, 18,  V_AMBAR, 55,    V_AMBAR, 95,   V_MADEIRA, 90 },
    [D_AGIOTA]     = { V_OURO,    V_OURO, 18,   V_LATAO_E, 85,  V_OURO, 100 },
    [D_MEDUSA]     = { V_VERDE,   V_PEDRA, 34,  V_VERDE, 100,   V_BREU, 80,    V_LIMA, 100 },
    [D_MISTERIOSO] = { RGB(40, 36, 34) },          // o corpo preto e o "?" vem do desenho
    [D_RANCOR]     = { V_SANGUE,  V_VINHO, 8,   V_VINHO, 100,   V_SANGUE, 100, V_BREU, 85 },
    [D_ESPERANCA]  = { V_CEU,     V_NOITE, 72,  V_CEU, 95,      V_TRIGO, 100,  V_TRIGO, 75 },
    [D_PONTE]      = { V_AGUA,    V_CEU, 14,    V_AGUA, 100,    V_CIANO, 100,  V_CEU, 40 },
    [D_GARIMPEIRO] = { V_TERRA,   V_TRIGO, 10,  V_TERRA, 100,   V_COURO, 100,  V_OURO, 100 },
    [D_POTE_QUENTE] = { V_FOGO,   V_AMBAR, 12,  V_AMBAR, 80,    V_FOGO, 95,    V_TRIGO, 100 },
    [D_FANTASMA]   = { V_CIANO,   V_CEU, 62,    V_CIANO, 38,    V_BRANCO, 45,  V_AZUL, 70 },
    [D_PREVISIVEL] = { V_CIANO,   V_CEU, 16,    V_AZUL, 70,     V_AGUA, 100,   V_BRANCO, 85 },
    [D_OCULTO]     = { V_FERRO,   0, 0,         V_FERRO, 45,    V_BREU, 60,    V_PRATA, 60 },
    [D_SACRIFICIO] = { V_VINHO,   V_SANGUE, 78, V_BREU, 45,     V_AMBAR, 95,   V_OURO, 100 },
    [D_VAZIO]      = { V_PEDRA,   V_BRANCO, 40, V_BARRO, 45,    V_PEDRA, 60,   V_BRANCO, 60 },
    [D_MEDIA]      = { V_AGUA,    V_CEU, 20,    V_AGUA, 75,     V_SANGUE, 85,  V_AGUA, 100 },
    [D_VAMPIRO]    = { V_BREU,    V_VINHO, 30,  V_SANGUE, 100,  V_VINHO, 90,   V_BRANCO, 70 },
    [D_COLECIONADOR] = { V_NOITE, V_NOITE, 30,  V_AZUL, 70,     V_OURO, 100,   V_CEU, 85 },
    [D_PACTO]      = { V_ROXO,    V_SANGUE, 22, V_SANGUE, 90,   V_AMEIXA, 100, V_OURO, 100 },
    [D_ASSOMBRACAO] = { V_AMEIXA, V_LILAS, 24,  V_LILAS, 100,   V_ROXO, 80,    V_BRANCO, 80 },
    [D_FICHA_VELHA] = { V_COURO,  V_TERRA, 46,  V_VINHO, 80,    V_ROSA, 55,    V_MADEIRA, 95 },
    [D_SANGUESSUGA] = { V_VERDE,  V_VERDE, 16,  V_BREU, 80,     V_AMBAR, 90,   V_SANGUE, 100 },
    [D_TRENO]      = { V_VERM,    V_VERM, 16,   V_VERM, 100,    V_OURO, 100,   V_VERDE, 100 },
    [D_BONECO]     = { V_CEU,     V_BRANCO, 100 }, // a neve, a cara e o cachecol vem do corpo (neve_boneco)
    [D_JINGLE]     = { V_OURO,    V_OURO, 12,   V_OURO, 100,    V_LATAO_E, 100, V_VERM, 80 },
    [D_FURIA]      = { V_FOGO,    V_SANGUE, 16, V_FOGO, 95,     V_AMBAR, 100,  V_BREU, 85 },
    [D_ONDA]       = { V_CIANO,   V_CEU, 14,    V_AZUL, 75,     V_CIANO, 100,  V_BRANCO, 90 },
    [D_TSUNAMI]    = { V_AZUL,    V_NOITE, 44,  V_AZUL, 100,    V_CIANO, 100,  V_BRANCO, 100 },
    [D_PATINHOS]   = { V_TRIGO,   V_CEU, 80,    V_TRIGO, 100,   V_FOGO, 100,   V_AGUA, 45 },
    [D_PRESENTE]   = { V_VERM,    0, 0,         V_ROSA, 40,     V_LATAO, 100 },
    [D_PANETONE]   = { V_TERRA }, // o miolo e as frutas vem do corpo (miolo_panetone)
    [D_PISCA]      = { V_VERDE,   V_VERDE, 16,  V_VERM, 100,    V_AMBAR, 100,  V_CHUMBO, 100 },
    [D_MEIA]       = { V_VERM,    0, 0,         V_VERM, 40,     V_LATAO_E, 100, V_OURO, 100 },
    [D_ESTRELA]    = { V_OURO,    V_NOITE, 60,  V_OURO, 100,    V_TRIGO, 100,  V_BRANCO, 100 },
    [D_RENA]       = { V_TERRA,   V_TERRA, 30,  V_COURO, 35,    V_MADEIRA, 100, V_VERM, 100 },
    [D_REVOLVER]   = { V_CHUMBO,  V_COURO, 16,  V_ACO, 100,     V_LATAO, 100,  V_BREU, 100 },
    // O ciclo cosmico: o corpo de noite vem de desenha_dado; aqui, o aro e as luzes.
    [D_ASTRO]       = { V_OURO,   0, 0,         V_BRANCO, 100,  V_TRIGO, 100,  V_NOITE, 100 },
    [D_CONSTELACAO] = { V_NOITE,  0, 0,         V_BRANCO, 100,  V_TRIGO, 100,  V_NOITE, 100 },
    [D_GALAXIA]     = { V_AMEIXA, 0, 0,         V_LILAS, 100,   V_AMEIXA, 100, V_ROXO, 100 },
    [D_BURACO]      = { V_FOGO,   0, 0,         V_TRIGO, 100,   V_AMBAR, 100,  V_FOGO, 100 },
    [D_UNIVERSO]    = { V_AMEIXA, 0, 0,         V_BRANCO, 100,  V_ROXO, 100,   V_VINHO, 100 },
};

// A cor de um dado nas telas (o nome dele): a cor propria, clareada se for
// escura demais para ler sobre o painel.
static uint16_t cor_dado(int tipo)
{
    uint16_t c = VISUAL[tipo].cor;
    if (!c) return C_LATAO;
    int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
    return r * 3 + g * 6 + b < 1200 ? gfx_mistura(c, C_MARFIM, 40) : c;
}

// O aro: a cor do dado; no dado comum, latao.
static uint16_t aro_de(int tipo)
{
    return VISUAL[tipo].cor ? VISUAL[tipo].cor : C_LATAO;
}
