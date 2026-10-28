// ===========================================================================
// MODO DESAFIANTE: uma run pelo cassino, de salao em salao.
//
// Incluido pelo dado.c, como os outros arquivos de dado/ (usa as mesmas regras,
// dados e desenho); nao se compila sozinho. Nada daqui muda a partida rapida,
// as 2 pessoas ou o online.
//
// A run: 3 saloes, cada um com tres oponentes (do mais facil ao mais
// dificil) e o chefe, sempre nessa ordem, todos a vista desde o comeco. Cada
// partida tem 8 rodadas e 50 fichas de cada lado; vencer paga umas poucas
// moedas (mais a cada mesa e a cada salao) e abre uma loja pequena: a
// vitrine, trocar a vitrine e tirar um dado da bolsa. Perder encerra a run.
// Nada passa de uma run para outra.
// ===========================================================================

// As partes, na ordem em que uma usa a outra.
#include "desafio/run.inc"        // saloes, mesas e os oponentes
#include "desafio/falas.inc"      // o balao do oponente
#include "desafio/retratos.inc"   // os bustos em pixels
#include "desafio/amuletos.inc"   // os amuletos da run
#include "desafio/perfis.inc"     // ate 3 perfis por aparelho
#include "desafio/mapa.inc"       // o sorteio, a loja, as mesas de cada salao e os oponentes
#include "desafio/bolsas.inc"     // as bolsas do comeco da run: regra, icone e o que abre cada uma
#include "desafio/andamento.inc"  // salvamento e andamento da run
#include "desafio/desenho.inc"    // o salao, a bolsa, a vitoria e o fim de run
#include "desafio/loja.inc"       // a loja depois de cada mesa
#include "desafio/amuleto_escolha.inc" // a escolha e a troca de amuleto
#include "desafio/telas.inc"      // perfis e dificuldades
#include "desafio/bolsa_escolha.inc" // a escolha da bolsa, em fichas, no comeco da run
#include "desafio/historico.inc"  // o que cada perfil ja jogou
#include "desafio/estilo_perfil.inc" // o feltro, a mesa e os dados do perfil
#include "desafio/novidades.inc"  // o que abriu, um card de cada vez
#include "desafio/colecao.inc"    // o que esta aberto e o que esta fechado
#include "desafio/teclas.inc"     // as teclas das telas do Desafiante
