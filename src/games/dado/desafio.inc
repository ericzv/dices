// ===========================================================================
// MODO DESAFIANTE: uma run pelo cassino, de salao em salao.
//
// Incluido pelo dado.c, como os outros arquivos de dado/ (usa as mesmas regras,
// dados e desenho); nao se compila sozinho. Nada daqui muda a partida rapida,
// as 2 pessoas ou o online.
//
// A run: 3 saloes, 6 passos cada. Em cada passo o jogador escolhe um caminho
// entre 2 ou 3 nos (oponente facil, medio ou dificil, loja ou evento); o
// sexto passo e o chefe do salao. Cada partida tem 8 rodadas e 50 fichas de
// cada lado; vencer converte as fichas que sobraram em moedas (1 por 1) e da
// um premio de dado. Perder encerra a run. Nada passa de uma run para outra.
// ===========================================================================

// As partes, na ordem em que uma usa a outra.
#include "desafio/run.inc"        // andares, nos, eventos e os oponentes
#include "desafio/falas.inc"      // o balao do oponente
#include "desafio/retratos.inc"   // os bustos em pixels
#include "desafio/amuletos.inc"   // os amuletos da run
#include "desafio/perfis.inc"     // ate 3 perfis por aparelho
#include "desafio/mapa.inc"       // o sorteio, o mapa de cada salao e os oponentes
#include "desafio/andamento.inc"  // salvamento e andamento da run
#include "desafio/desenho.inc"    // mapa, loja, bolsa e fim de run
#include "desafio/eventos.inc"    // as paradas do mapa que nao sao partida
#include "desafio/telas.inc"      // perfis e dificuldades
#include "desafio/colecao.inc"    // o que esta aberto e o que esta fechado
#include "desafio/teclas.inc"     // as teclas das telas do Desafiante
