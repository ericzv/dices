# Os sons do jogo

Os sons que o jogo toca são os arquivos `desktop/sons/*.ogg`. No build, o
CMake os transforma num `.c` e eles vão dentro do executável
(`cmake/embute_sons.cmake`); o `desktop/som.c` toca cada um.

Eles são montados por `gera.py`, a partir de gravações que `baixa.py` baixa
para `tools/sons/fontes/` (fora do git, cerca de 28 MB):

```sh
pip install numpy scipy imageio-ffmpeg
python3 tools/sons/baixa.py
python3 tools/sons/gera.py vidro fogo      # refaz só esses
python3 tools/sons/gera.py --saida /tmp/x  # refaz todos em outra pasta, para comparar
```

Cada som tem a sua semente: refazer um não muda os outros. Os arquivos do
repositório são os que foram aprovados ouvindo; refazer dá sons equivalentes
(a mesma receita, com detalhes ao acaso diferentes, como a ordem das fichas
caindo).

## O que é cada som

| Grupo | Sons | Como é feito |
|---|---|---|
| Mesa e dados | `bate_*`, `bate_dado_*`, `pousa_*`, `ficha_*`, `rufo`, `bolsa`, `turno` | gravações de dados em madeira, feltro e plástico |
| Moedas | `moeda_bate_*`, `moeda_pousa_*`, `moeda_rola` | uma ficha leve, meio madeira: cliques de plástico mais agudos, um toque de madeira e o bambear ao deitar; mais baixas que os dados |
| Menu | `tique` (cursor e escolher), `passa` (o mouse passando por cima) | madeira gravada, encurtada; o `passa` é mais curto e bem mais baixo |
| Jogadas | `valido`…`valido4`, `anula`, `efeito`, `empate`, `vitoria`, `correu` | as notas do som sintetizado antigo, em teclado (Rhodes) e baixo elétrico; o `valido4` tem um sax-alto |
| Momentos | `suspense` (a hora da verdade), `forte` (a vitória grande do crescendo) | o coração em tímpano abafado com o baixo por baixo; a vitória com acorde cheio, sax-alto, prato de vassourinha e mais fichas |
| Partida | `abertura`, `partida`, `fim`, `derrota` | as notas antigas, em teclado e baixo |
| Música | `trilha` | a bossa antiga nota por nota: acordes e melodia no teclado, linha no baixo, vassourinha e shaker sintetizados |
| Camadas da música | `trilha_chefe` (a mesa do chefe), `trilha_tensao` (a hora da verdade) | do tamanho exato da trilha e na mesma harmonia, para tocar junto dela: a batucada (surdo sintetizado, agogô, o aro num dado batendo na madeira, cordas e o rolo de tímpano no fim de cada frase); as cordas em trêmolo no Dó grave com a terça e a sétima de cada acorde |
| Ambiente | `ambiente_0`…`ambiente_3` (um por salão) | loops de 24 s: gente conversando ao longe (vozes sintetizadas: sílabas de vogais na altura de cada pessoa), fichas, dados e o detalhe de cada salão: taças no Verde e no Dourado, a roleta no Vermelho, cartas e risadas no do Coringa |
| Troca de tela | `transicao` | um sopro de pano que anda da esquerda para a direita junto com a varredura |
| Poderes | `laser`, `vento`, `vidro`, `fogo`, `tira`, `rouba`, `zera`, `bigorna`, `jackpot` | gravações, instrumentos do FluidR3 e sínteses |
| Roleta | `giro`, `slot`, `premio` | celesta, cliques reais, metal e moedas |
| Poderes com som próprio | `vampiro`, `sabotador`, `coveiro`, `autofagia`, `ampulheta` | asas e presas, catraca e assobio murchando, pazadas e sino, dentada e marimba, areia grão a grão e celesta |

Quem tem variantes (`bate_0`…`bate_3` etc.) é sorteado no jogo, sem repetir
a anterior, e com um pouco de variação de tom.

As camadas da música são medidas na `trilha.ogg` do repositório (o volume e
o tamanho): o `som.c` as mistura amostra a amostra com ela, e o teste `sons`
confere que as três têm o mesmo número de quadros. Uma camada de outro
tamanho fica fora do jogo. O ambiente não anda junto da trilha, então não
leva nada com altura definida que dure (brigaria com o acorde).

O volume de cada som foi igualado ao do som sintetizado que ele substituiu
(`VOLUME` no `gera.py`); `GANHO_DB` guarda os ajustes pedidos depois
(vidro mais forte, ventania mais baixa, o 4º valendo mais intenso). Os sons
que não substituíram nenhum (moedas e `passa`) ficam abaixo dos parentes.

Créditos e licenças das gravações: `desktop/sons/CREDITOS.md`.
