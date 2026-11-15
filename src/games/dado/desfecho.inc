// ===========================================================================
// DESFECHO: o fim da rodada - os poderes que mexem no rival, os adesivos, os
// bonus e o placar, em sequencia. calcula_desfecho(d, gera) e a entrada: com
// gera, e o desfecho de verdade (partida.inc); sem, e a previa, que nao muda
// nada (a mesa, a IA e as dicas).
//
// Incluido pelo dado.c, como os outros arquivos de dado/; nao se compila
// sozinho. As partes, na ordem em que uma usa a outra:
// ===========================================================================

#include "desfecho/tipos.inc"          // o que o desfecho calcula (desfecho_t)
#include "desfecho/alvos.inc"          // quem cada poder atinge
#include "desfecho/eventos.inc"        // a lista de eventos e a capa do Impermeavel
#include "desfecho/reacoes.inc"        // Ratoeira, Coleira e Cavalo de Troia
#include "desfecho/tremor.inc"         // o Terremoto e o abalo
#include "desfecho/efeitos_fim.inc"    // os bonus, multiplicadores e fixos do fim
#include "desfecho/armado.inc"         // o poder armado e as miras da mesa ao vivo
#include "desfecho/poderes.inc"        // os poderes dos dados que valem no fim
#include "desfecho/adesivos_fim.inc"   // os adesivos que agem no fim da rodada
#include "desfecho/resolve_mesa.inc"   // as fases que mexem na mesa antes dos poderes
#include "desfecho/orquestra.inc"      // calcula_desfecho: as fases, em ordem
#include "desfecho/placar_visivel.inc" // o placar que a mesa mostra
