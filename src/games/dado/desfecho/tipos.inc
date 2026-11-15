// ===========================================================================
// Desfecho: poderes que mexem no rival, em sequencia
// ===========================================================================
typedef struct {
    uint8_t est[2][N_FILA];
    int8_t  bonus[2][N_FILA];        // Moeda da sorte
    int8_t  mult[2][N_FILA];         // soma dos multiplicadores (0 = nenhum)
    uint8_t queima[2][N_FILA];       // alvos da Ficha de Fogo
    int semente[2];
    int roubo_val[2];
    int delta[2];
    int total[2];
    int bruto[2];                    // o total antes de travar em 0 (a previa mostra negativo)
    int atacados[2];                 // as tentativas do outro de anular, atacar ou tirar pontos de cada um (Ourico)
    int agiota[2];                   // fichas que o Agiota de cada um leva do outro, fora do pote
    uint8_t medusa[2];               // a Medusa valeu: a fila vira Quebrados no fim da rodada
    uint8_t come[2], come_de[2];     // a Autofagia comeu: o dado (posicao + 1) e ela (posicao + 1); 0: nada
    uint8_t garimpo[2];              // o Garimpeiro valeu: posicoes dos outros validos que voltam para a bolsa
    uint8_t onda[2];                 // posicoes das Ondas que juntaram 9: viram Tsunami
    uint8_t meia[2];                 // posicoes das Meias anuladas que ja avisaram que guardam
    // Os adesivos (adesivos_de): o que fica depois da rodada.
    uint8_t moedas[2];               // Dourado: moedas para a run
    uint8_t bis[2];                  // Bis: 1 lugar a mais na sequencia da proxima rodada
    int gorjeta[2];                  // Gorjeta: fichas do rival para o dono, fora do pote
    uint8_t fermento[2], raspa[2];   // posicoes na fila: o Fermento que cresce, a Raspadinha que sai
    uint32_t raspa_lados[2][N_FILA]; // e de que lado ela sai (pela Dupla Face ou pela Moldura, nao e o de cima)
    uint8_t selo_rompe[2], selo_lacra[2];   // e o lacre do Selo que se rompe, e o que se fecha
    uint8_t velcro[2], fragil[2];    // o Velcro que volta para a mao, o Fragil que quebra
    uint8_t capa[2];                 // o Impermeavel que agiu: a capa fica ate o fim da partida
    uint8_t ratoeira[2], rato_bits[2];   // Ratoeiras que desarmaram (quantas e quais posicoes)
    uint8_t coleira_bits[2];             // as Coleiras que ja cobraram (quais posicoes)
    uint8_t fig_cresce[2];           // as Figurinhas que ativaram: o dado junta mais uma (posicoes)
    uint8_t fermento2[2], fig_cresce2[2];   // e a segunda vez, carimbadas: crescem de novo
    uint8_t bilhete[2];              // os Bilhetes Premiados que valeram (agem no fim de tudo)
    uint8_t dinamite[2];             // as Dinamites que estouraram ou apagaram: o pavio zera
    uint8_t balao[2];                // os Baloes que estouraram: o ar zera
    uint8_t bil_carimbo[2];          // e os carimbados: o bonus vale x3 em vez de x2
    uint8_t bil_holo[2][N_FILA];     // e quantos cada Holografico copia (por posicao)
    uint8_t ad_agiram[2];            // quantos adesivos de cada um agiram na rodada (a conquista da Purpurina)
} desfecho_t;

