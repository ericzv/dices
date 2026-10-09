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
       EF_CAVALO, EF_BERSERKER, EF_VIUVA };

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
       // Daqui em diante, so no Modo Desafiante, liberados pelas dificuldades.
       D_BUMERANGUE, D_GEMEO, D_PRISMA, D_FENIX, D_TRONO,
       D_CAVALO, D_BERSERKER, D_VIUVA, D_N };
#define D_BASICOS D_BUMERANGUE           // tipos da partida rapida, 2 jogadores e online
static bool so_desafio(int t) { return t >= D_BASICOS; }

// "Fica" = termina a rodada valendo, sem ter sido anulado.
static const tipo_t TIPO[D_N] = {
    { "Moeda",            2, EF_NADA,         RAR_COMUM,    4, "1 ou 2: ótima para abrir a sequência" },
    { "D4",               4, EF_NADA,         RAR_COMUM,    5, "quatro lados, de 1 a 4" },
    { "D6",               6, EF_NADA,         RAR_COMUM,    0, "seis lados, de 1 a 6" },
    { "D8",               8, EF_NADA,         RAR_COMUM,    9, "oito lados, de 1 a 8" },
    { "D12",             12, EF_NADA,         RAR_INCOMUM, 20, "doze lados, de 1 a 12" },
    { "D20",             20, EF_NADA,         RAR_RARO,    26, "vinte lados, de 1 a 20" },
    { "Dobro",            6, EF_DOBRO,        RAR_INCOMUM, 16, "1 a 3: o dado seguinte vale x2" },
    { "Viciado",          6, EF_VICIADO,      RAR_INCOMUM, 16, "rola 2 vezes e fica com o maior" },
    { "Lastro",          12, EF_LASTRO,       RAR_RARO,    26, "sempre de 4 a 12" },
    { "Par",              4, EF_PAR,          RAR_INCOMUM, 11, "igual ao anterior: +8" },
    { "Escudo",           6, EF_ESCUDO,       RAR_INCOMUM, 15, "não pode ser anulado nem roubado" },
    { "Carrasco",         4, EF_CARRASCO,     RAR_RARO,    17, "4: anula o maior do rival · 1: anula o seu menor" },
    { "Explosivo",        8, EF_EXPLODE,      RAR_RARO,    16, "no 8, rola de novo e soma" },
    { "Espinhoso",        4, EF_MARTELO,      RAR_RARO,    15, "com 3 ou 4: tira 4 do oponente" },
    { "Pirata",           8, EF_PIRATA,       RAR_RARO,     0, "com 6 ou mais: tira 4 do oponente" },
    { "Espelho",          6, EF_ESPELHO,      RAR_RARO,    20, "1 ou 2: vira o maior do oponente" },
    { "Fichas",           4, EF_FICHAS,       RAR_COMUM,    6, "com mais fichas que o oponente: +2" },
    { "Tudo ou Nada",     2, EF_TUDO_NADA,    RAR_LENDA,   14, "2: tudo vale x2 · 1: rodada vale 0" },
    { "Agouro",          20, EF_MALDITO,      RAR_LENDA,   24, "7 ou menos: anula a sua sequência" },
    { "Gatuno",           6, EF_GATUNO,       RAR_RARO,    20, "com um 1 na sua sequência: rouba" },
    { "Quebrado",         8, EF_QUEBRADO,     RAR_COMUM,    9, "número fixo; quebra ao ser jogado" },
    { "Invejoso",         6, EF_INVEJOSO,     RAR_RARO,     0, "1, 3 ou 5: anula o maior do oponente e o seu" },
    { "Caridoso",         4, EF_CARIDOSO,     RAR_RARO,     0, "com 3 ou 4: rouba o menor do oponente" },
    { "Fartura",          4, EF_FARTURA,      RAR_INCOMUM, 14, "seus dados com 1 ou 2 valem x2" },
    { "Moeda sorte",      2, EF_SORTE,        RAR_INCOMUM, 14, "1: ímpares +1 · 2: pares +1" },
    { "Teimoso",          6, EF_TEIMOSO,      RAR_INCOMUM, 11, "não pode ser anulado" },
    { "Semente",          2, EF_SEMENTE,      RAR_INCOMUM,  9, "seus dados somam 20+: +10" },
    { "Ficha Fogo",       2, EF_FOGO,         RAR_RARO,    22, "valendo: +10, e o seu maior queima" },
    { "Maldito",          4, EF_MALDICAO,     RAR_RARO,     0, "vale e tira o mesmo do oponente" },
    { "Trevo",            4, EF_TREVO,        RAR_INCOMUM,  8, "4 vale 8 · 1 a 3 vale 0" },
    { "Jackpot",          4, EF_JACKPOT,      RAR_INCOMUM,  9, "três dados iguais: +10" },
    { "Copiador",         6, EF_COPIADOR,     RAR_INCOMUM, 11, "cada dado igual a ele: +2" },
    { "Egoísta",         12, EF_EGOISTA,      RAR_RARO,    14, "anula os seus outros dados" },
    { "Laser",            6, EF_LASER,        RAR_RARO,    16, "com 5 ou 6: anula um dado do oponente" },
    { "Ventania",         8, EF_VENTANIA,     RAR_RARO,    10, "rola de novo o dado de antes dele" },
    { "Misericordioso",   4, EF_MISERICORDIA, RAR_INCOMUM, 13, "valendo: +2 por dado seu anulado" },
    { "Solitário",        4, EF_SOLITARIO,    RAR_COMUM,    9, "sem vizinho valendo: +4" },
    { "Sentinela",        4, EF_SENTINELA,    RAR_COMUM,    6, "se cair, cai sozinho" },
    { "Ferreiro",         4, EF_FERREIRO,     RAR_INCOMUM, 10, "não pontua; 3 ou 4: +6 no seguinte" },
    { "Cobrador",         4, EF_COBRADOR,     RAR_COMUM,    6, "oponente com mais fichas: +2" },
    { "Desafiante",       8, EF_DESAFIANTE,   RAR_INCOMUM, 13, "maior dado da rodada: +4" },
    { "Vidro",            8, EF_VIDRO,        RAR_COMUM,    7, "tirou 1: quebra e anula o anterior" },
    { "Acumulador",       8, EF_ACUMULADOR,   RAR_INCOMUM, 16, "+1 a cada lance, até +6" },
    { "Segurança",        4, EF_SEGURANCA,    RAR_INCOMUM,  7, "anulação ou roubo do oponente cai nele" },
    { "Bumerangue",       6, EF_BUMERANGUE,   RAR_COMUM,    0, "1 ou 2: volta e rola de novo" },
    { "Gêmeo",            6, EF_GEMEO,        RAR_INCOMUM,  0, "tira o mesmo do anterior que vale" },
    { "Prisma",           8, EF_PRISMA,       RAR_INCOMUM,  0, "+1 por tipo de dado seu valendo" },
    { "Fênix",            6, EF_FENIX,        RAR_RARO,     0, "anulado: renasce no fim" },
    { "Trono",           12, EF_TRONO,        RAR_LENDA,    0, "último que você jogou, valendo: x2" },
    { "Cavalo de Troia",  8, EF_CAVALO,       RAR_RARO,     0, "valendo: +1 por dado seu valendo" },
    { "Berserker",       20, EF_BERSERKER,    RAR_LENDA,    0, "valendo: anula um dado seu ao acaso" },
    { "Viúva Negra",      8, EF_VIUVA,        RAR_RARO,     0, "único dado seu valendo: x2" },
};

// Tres niveis de texto por dado, sempre com a mesma regra: o curto (TIPO.desc)
// vai no painel da fila e do jogo; o medio, numa linha, no premio, na loja e na
// escolha do que sai; o completo, na colecao e na tecla I.
static const char *TEXTO_MEDIO[D_N] = {
    [D_D2]        = "Uma moeda: 1 ou 2. Deixa a barra baixa para os próximos.",
    [D_D4]        = "De 1 a 4. Vale pouco, mas é bom para abrir a sequência.",
    [D_D6]        = "O dado comum da casa: de 1 a 6.",
    [D_D8]        = "De 1 a 8. Vale mais, mas levanta a barra da sequência.",
    [D_D12]       = "De 1 a 12. Forte no fim da sequência, arriscado no começo.",
    [D_D20]       = "De 1 a 20. O maior da mesa: guarde para o fim da sequência.",
    [D_DOBRO]     = "Tirando 1 a 3, o dado logo depois dele vale x2 no fim.",
    [D_VICIADO]   = "Rola duas vezes e fica com o maior dos dois resultados.",
    [D_LASTRO]    = "Doze lados, mas só cai de 4 a 12, todos com a mesma chance.",
    [D_PAR]       = "Se tirar o mesmo número do último dado que vale: +8.",
    [D_ESCUDO]    = "Nada o anula e nenhum ataque o alcança, nem roubo.",
    [D_CARRASCO]  = "Valendo com 4, anula o maior dado do oponente. Tirando 1, anula o seu menor.",
    [D_EXPLOSIVO] = "Se tirar 8, rola de novo e soma (até 3 vezes seguidas).",
    [D_MARTELO]   = "Se ficar valendo com 3 ou 4, tira 4 pontos do oponente.",
    [D_PIRATA]    = "Se ficar valendo com 6 ou mais, tira 4 pontos do oponente.",
    [D_ESPELHO]   = "Se tirar 1 ou 2, vira o número do maior dado do oponente.",
    [D_FICHAS]    = "Se ficar valendo e você começou a rodada com mais fichas que o oponente: +2.",
    [D_TUDO_NADA] = "Tirou 2: todos os seus dados valem x2. Tirou 1: sua rodada vale 0.",
    [D_MALDITO]   = "Se tirar 7 ou menos, anula a sua sequência inteira, e ele junto.",
    [D_GATUNO]    = "Se ficar valendo e houver um 1 na sua sequência, rouba o menor dado do oponente.",
    [D_QUEBRADO]  = "Sem sorteio: vale o número gravado nele. Quebra ao ser jogado.",
    [D_INVEJOSO]  = "Valendo com 1, 3 ou 5, anula o maior do oponente e depois o seu maior.",
    [D_CARIDOSO]  = "Se ficar valendo com 3 ou 4, rouba o menor dado do oponente.",
    [D_FARTURA]   = "Se ficar valendo, seus dados que tiraram 1 ou 2 valem x2 no fim.",
    [D_SORTE]     = "Tirou 1: +1 em cada dado ímpar seu. Tirou 2: +1 em cada dado par.",
    [D_TEIMOSO]   = "Nada o anula, nem a queda da sequência. Mas pode ser roubado.",
    [D_SEMENTE]   = "Se ficar valendo e seus dados somarem 20 ou mais no fim: +10.",
    [D_FOGO]      = "Se ficar valendo: +10 pontos, e o seu maior dado queima e sai.",
    [D_MALDICAO]  = "Se ficar valendo, soma para você e tira o mesmo do oponente.",
    [D_TREVO]     = "Tirou 4: SORTE, vale 8. Tirou 1, 2 ou 3: vale 0.",
    [D_JACKPOT]   = "Se ficar valendo e três dados seus mostrarem o mesmo número: +10.",
    [D_COPIADOR]  = "Se ficar valendo, cada outro dado seu com o mesmo número ganha +2.",
    [D_EGOISTA]   = "Anula todos os seus outros dados, menos a si mesmo. Pode ser anulado.",
    [D_LASER]     = "Se ficar valendo com 5 ou 6, anula um dado do oponente, ao acaso.",
    [D_VENTANIA]  = "Ao cair, rola de novo o seu dado lançado logo antes dele.",
    [D_MISERICORDIOSO] = "Se ficar valendo, ganha +2 por dado seu anulado na rodada.",
    [D_SOLITARIO] = "Valendo, sem dado valendo logo antes nem logo depois dele: +4.",
    [D_SENTINELA] = "Se cair na sequência, cai sozinho: o dado de antes continua valendo.",
    [D_FERREIRO]  = "Não pontua. Tirando 3 ou 4, o dado logo depois ganha +6 se os dois valerem.",
    [D_COBRADOR]  = "Se o oponente começou a rodada com mais fichas: +2.",
    [D_DESAFIANTE] = "Se for o maior dado da rodada, dos dois lados: +4 no fim.",
    [D_VIDRO]     = "Se tirar 1, se estilhaça: anula ele e o anterior, e some da bolsa.",
    [D_ACUMULADOR] = "Cada lance soma +1 de bônus a ele, para sempre (até +6).",
    [D_SEGURANCA]  = "Valendo, leva no lugar dos outros a anulação ou o roubo do oponente.",
    [D_BUMERANGUE] = "Se tirar 1 ou 2, volta e rola de novo, uma vez.",
    [D_GEMEO]     = "Tira o mesmo número do último dado seu que vale (sem um antes, rola normal).",
    [D_PRISMA]    = "Se ficar valendo, +1 no fim para cada tipo diferente de dado seu valendo.",
    [D_FENIX]     = "Se terminar anulado, renasce no fim valendo o número que tirou.",
    [D_TRONO]     = "Se for o último dado que você jogou na rodada e ficar valendo: x2.",
    [D_CAVALO]    = "Se ficar valendo, +1 no fim por dado seu valendo, ele incluído.",
    [D_BERSERKER] = "Se ficar valendo, anula no fim um outro dado seu, ao acaso.",
    [D_VIUVA]     = "Se terminar como o único dado seu valendo, vale x2 no fim.",
};

static const char *TEXTO_CAT[D_N] = {
    [D_D2]        = "Uma moeda: sai 1 ou 2. Como primeiro dado da sequência, deixa a barra baixa para os que vêm depois.",
    [D_D4]        = "Quatro lados, de 1 a 4. Vale pouco, mas é bom para abrir a sequência.",
    [D_D6]        = "Seis lados, de 1 a 6. O dado comum da casa.",
    [D_D8]        = "Oito lados, de 1 a 8. Vale mais que o D6, mas levanta a barra para os próximos dados.",
    [D_D12]       = "Doze lados, de 1 a 12. Forte no fim da sequência, arriscado no começo: depois dele, é difícil não cair.",
    [D_D20]       = "Vinte lados, de 1 a 20. O maior da mesa: melhor como último dado da sequência.",
    [D_DOBRO]     = "Seis lados. Se tirar de 1 a 3, no fim da rodada o dado logo depois dele na sequência vale x2, se os dois estiverem valendo. Tirando 4 a 6, não dobra nada. Se esse dado for anulado, o x2 se perde: não passa para o próximo. Multiplicadores se somam: dois x2 no mesmo dado dão x4; três, x6.",
    [D_VICIADO]   = "Seis lados. Rola duas vezes e fica com o maior resultado. O menor aparece na mesa e é descartado.",
    [D_LASTRO]    = "Doze lados, mas só cai de 4 a 12, cada número com a mesma chance (1 em 9).",
    [D_PAR]       = "Quatro lados. Se tirar o mesmo número do último dado que vale na sequência, ganha +8. Igual não cai; menor cai, como qualquer dado.",
    [D_ESCUDO]    = "Seis lados. Nada anula este dado: nem a queda da sequência, nem o Agouro, nem o Egoísta, nem ataques. E ninguém pode roubá-lo.",
    [D_CARRASCO]  = "Quatro lados. Se terminar valendo com 4, anula o maior dado do oponente. Se tirar 1, anula o menor dado da sua própria sequência (fora ele). Escudo e Teimoso resistem à anulação.",
    [D_EXPLOSIVO] = "Oito lados. Tirando 8, rola de novo e soma ao que já tinha. Pode explodir até 3 vezes seguidas.",
    [D_MARTELO]   = "Quatro lados. Se terminar valendo com 3 ou 4, tira 4 pontos do oponente. Com 1 ou 2, é um D4 comum.",
    [D_PIRATA]    = "Oito lados. Se terminar valendo com 6 ou mais, tira 4 pontos do oponente. Os pontos não passam para você.",
    [D_ESPELHO]   = "Seis lados. Se tirar 1 ou 2, vira o número do maior dado do oponente. Se o oponente ainda não jogou, espera valendo e vira no fim da rodada.",
    [D_FICHAS]    = "Quatro lados. Se terminar valendo e você começou a rodada com mais fichas que o oponente, vale +2. É o contrário do Cobrador.",
    [D_TUDO_NADA] = "Uma moeda de risco. Tirando 2, no fim todos os seus dados que valem ganham x2. Tirando 1, a sua rodada inteira vale zero, sem volta.",
    [D_MALDITO]   = "Vinte lados. Tirando 8 ou mais, é um D20 forte. Tirando 7 ou menos, anula a sua sequência inteira, e ele junto. Escudo e Teimoso resistem.",
    [D_GATUNO]    = "Seis lados. Se terminar valendo e algum dado da sua sequência tiver tirado 1, rouba o menor dado do oponente: os pontos dele passam para você. Se for anulado, não rouba.",
    [D_QUEBRADO]  = "Oito lados, mas sem sorteio: vale sempre o número gravado nele. Depois de jogado, quebra e sai da sua bolsa.",
    [D_INVEJOSO]  = "Seis lados. Se terminar valendo com 1, 3 ou 5, anula o maior dado do oponente e depois o maior da sua própria sequência (ele mesmo, se for o maior). Escudo e Teimoso resistem à anulação.",
    [D_CARIDOSO]  = "Quatro lados. Se terminar valendo com 3 ou 4, rouba o menor dado do oponente: os pontos dele passam para você. Só o Escudo não pode ser roubado.",
    [D_FARTURA]   = "Quatro lados. Se terminar valendo, no fim da rodada todos os seus dados que tiraram 1 ou 2 valem x2, ele também.",
    [D_SORTE]     = "Uma moeda. Se terminar valendo: tirando 1, cada dado ímpar seu ganha +1; tirando 2, cada dado par ganha +1. Ela conta a si mesma.",
    [D_TEIMOSO]   = "Seis lados. Nada o anula: nem a queda da sequência, nem o Agouro, nem o Egoísta, nem ataques. Mas pode ser roubado.",
    [D_SEMENTE]   = "Uma moeda. Se terminar valendo e, no fim, os seus dados somarem 20 ou mais (com bônus e roubos), ganha +10.",
    [D_FOGO]      = "Uma moeda. Se terminar valendo, dá +10 pontos no fim, e o seu maior dado vira cinza e sai da sua bolsa para sempre.",
    [D_MALDICAO]  = "Quatro lados. Se terminar valendo, soma para você e tira do placar do oponente o mesmo número que tirou.",
    [D_TREVO]     = "Quatro lados. Tirando 4: SORTE, vale 8. Tirando 1, 2 ou 3, vale zero. Na queda da sequência conta o número que saiu.",
    [D_JACKPOT]   = "Quatro lados. Se terminar valendo e três dados seus que valem mostrarem o mesmo número (ele pode ser um deles): JACKPOT, +10 no fim.",
    [D_COPIADOR]  = "Seis lados. Se terminar valendo, no fim cada outro dado seu que mostrar o mesmo número ganha +2. Iguais nunca caem na sequência.",
    [D_EGOISTA]   = "Doze lados. Ao cair, anula todos os seus outros dados da rodada, antes e depois dele, menos a si mesmo. Ele pode ser anulado: pela queda da sequência, por ataques, pelo Agouro. Escudo e Teimoso resistem.",
    [D_LASER]     = "Seis lados. Se terminar valendo com 5 ou 6, um raio verde anula um dado do oponente escolhido ao acaso. Escudo e Teimoso resistem à anulação.",
    [D_VENTANIA]  = "Oito lados. Ao cair, rola de novo o seu dado lançado logo antes dele (nunca a si mesmo nem os outros). A sequência é refeita: dados podem cair ou voltar a valer.",
    [D_MISERICORDIOSO] = "Quatro lados. Se terminar valendo, ganha +2 no fim para cada dado seu anulado na rodada.",
    [D_SOLITARIO] = "Quatro lados. Se terminar valendo e os dados vizinhos dele na sequência (o de antes e o de depois) não estiverem valendo, ganha +4 no fim. Na ponta da sequência, só conta o vizinho que existe.",
    [D_SENTINELA] = "Quatro lados. Se tirar menos que o último dado que vale, só ele cai: o dado de antes continua valendo.",
    [D_FERREIRO] = "Quatro lados. Não pontua, mas conta na queda da sequência com o número que tirou. Se tirar 3 ou 4, o dado logo depois dele ganha +6 no fim, se os dois terminarem valendo.",
    [D_COBRADOR] = "Quatro lados. Vale +2 se o oponente começou a rodada com mais fichas que você.",
    [D_DESAFIANTE] = "Oito lados. Se terminar valendo e for maior que todos os outros dados que valem na rodada, dos dois jogadores, ganha +4 no fim. Empate não conta.",
    [D_VIDRO] = "Oito lados. Tirando 1, se estilhaça: é anulado, anula também o último dado seu que valia (como na queda) e sai da sua bolsa para sempre.",
    [D_ACUMULADOR] = "Oito lados. Cada vez que é lançado, guarda +1 de bônus, até +6. O bônus fica com ele de rodada em rodada e soma ao número que tirou, se ele terminar valendo.",
    [D_SEGURANCA]  = "Quatro lados. Se o oponente anular ou roubar um dado seu (Carrasco, Laser, Invejoso, Caridoso, Gatuno) e o Segurança estiver valendo, quem é anulado ou roubado é o Segurança, e o outro dado fica.",
    [D_BUMERANGUE] = "Seis lados. Se tirar 1 ou 2, volta e rola de novo uma vez; fica o segundo número.",
    [D_GEMEO]     = "Seis lados. Tira sempre o mesmo número do último dado seu que vale, e por isso nunca cai. Sem um dado valendo antes dele, rola normal.",
    [D_PRISMA]    = "Oito lados. Se terminar valendo, ganha +1 no fim para cada tipo diferente de dado seu que vale na rodada, ele incluído.",
    [D_FENIX]     = "Seis lados. Se terminar anulado (pela queda, por ataque, pelo Agouro ou pelo Egoísta), renasce no fim e vale o número que tirou. Roubado, não volta.",
    [D_TRONO]     = "Doze lados. Se for o último dado que você jogou na rodada e terminar valendo, vale x2 no fim.",
    [D_CAVALO]    = "Oito lados. Se terminar valendo, ganha +1 no fim para cada dado seu que vale na rodada, ele incluído.",
    [D_BERSERKER] = "Vinte lados. Se terminar valendo, no fim anula um outro dado seu que vale, escolhido ao acaso. Escudo e Teimoso resistem.",
    [D_VIUVA]     = "Oito lados. Se terminar a rodada como o único dado seu que vale, vale x2 no fim. Soma com outros multiplicadores.",
};

static const char *RARIDADE[] = { "comum", "incomum", "raro", "lenda" };
// Cor de cada raridade nas etiquetas, a de um metal: ferro, bronze, prata
// e ouro.
static const uint16_t COR_RAR[] = { RGB(128, 120, 112), RGB(156, 106, 64),
                                    RGB(184, 194, 208), RGB(228, 194, 106) };
static uint16_t cor_rar(int tipo) { return COR_RAR[TIPO[tipo].raridade]; }

// "moeda", "D4" ... "D20": o tipo, sempre escrito do mesmo jeito.
static const char *nome_tipo(int tipo)
{
    switch (TIPO[tipo].lados) {
    case 2:  return "moeda";
    case 4:  return "D4";
    case 6:  return "D6";
    case 8:  return "D8";
    case 12: return "D12";
    default: return "D20";
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

// O catalogo mostra os dados do mais comum ao mais raro.
static int cat_tipo(int i)
{
    for (int r = RAR_COMUM; r <= RAR_LENDA; r++)
        for (int t = 0; t < D_N; t++)
            if (TIPO[t].raridade == r && i-- == 0) return t;
    return 0;
}

// O que a loja da partida rapida vende: N_LOJA destes, embaralhados nesta
// ordem. Precos (TIPO.preco) medidos com partidas IA x IA, 2000 por dado, com
// duas copias na bolsa: quem rende mais custa mais.
static const uint8_t VITRINE[] = { D_D8, D_D12, D_PAR, D_ESCUDO,
                                   D_DOBRO, D_VICIADO, D_LASTRO, D_QUEBRADO,
                                   D_FARTURA, D_SORTE, D_TEIMOSO, D_SEMENTE, D_TREVO,
                                   D_JACKPOT, D_COPIADOR, D_MISERICORDIOSO, D_SOLITARIO,
                                   D_SENTINELA, D_FERREIRO, D_COBRADOR, D_DESAFIANTE,
                                   D_VIDRO, D_ACUMULADOR, D_SEGURANCA };

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
    [D_DOBRO]      = { V_OURO,    0, 0,         V_OURO, 70 },
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
    [D_MISERICORDIOSO] = { V_PRATA, V_PRATA, 20, V_OURO, 60 },
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
