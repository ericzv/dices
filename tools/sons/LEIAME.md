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
python3 tools/sons/gera_acab.py metal rola_pedra   # os dos acabamentos (todos, sem nomes)
```

Cada som tem a sua semente: refazer um não muda os outros. Os arquivos do
repositório são os que foram aprovados ouvindo; refazer dá sons equivalentes
(a mesma receita, com detalhes ao acaso diferentes, como a ordem das fichas
caindo).

## O que é cada som

| Grupo | Sons | Como é feito |
|---|---|---|
| Mesa e dados | `bate_*`, `bate_dado_*`, `pousa_*`, `ficha_*`, `rufo`, `bolsa`, `turno` | gravações de dados em madeira, feltro e plástico |
| Moedas | `moeda_bate_*`, `moeda_pousa_*`, `moeda_rola` | uma ficha leve, meio madeira: cliques de plástico mais agudos, um toque de madeira e o bambear ao deitar; mais baixas que os dados. São as das fichas (os d2 de laca) |
| Moedas de metal (`gera_acab.py`) | `moeda_bate_metal_*`, `moeda_pousa_metal_*`, `rola_moeda_metal` | a Moeda e o Tudo ou Nada: o tinido abafado da beirada no feltro a cada volta, o baque macio ao deitar com o tilintar bambeando cada vez mais rápido, e na borda o tinido curto com um pouco da madeira. A Semente, um d2 de grãos, usa os sons dos grãos |
| Menu | `tique` (cursor e escolher), `passa` (o mouse passando por cima) | madeira gravada, encurtada; o `passa` é mais curto e bem mais baixo |
| Jogadas | `valido`…`valido4`, `anula`, `efeito`, `empate`, `vitoria`, `correu` | as notas do som sintetizado antigo, em teclado (Rhodes) e baixo elétrico; o `valido4` tem um sax-alto |
| Momentos | `suspense` (a hora da verdade), `forte` (a vitória grande do crescendo) | o coração em tímpano abafado com o baixo por baixo; a vitória com acorde cheio, sax-alto, prato de vassourinha e mais fichas |
| Partida | `abertura`, `partida`, `fim`, `derrota` | as notas antigas, em teclado e baixo |
| Música | `trilha` | a bossa antiga nota por nota: acordes e melodia no teclado, linha no baixo, vassourinha e shaker sintetizados |
| Camada da música | `trilha_chefe` (a mesa do chefe) | do tamanho exato da trilha e na mesma harmonia, para tocar junto dela: a batucada (surdo sintetizado, agogô, o aro num dado batendo na madeira, cordas e o rolo de tímpano no fim de cada frase) |
| A voz de cada chefe | `trilha_dona`, `trilha_barao`, `trilha_coringa`, `trilha_noel` | por cima da batucada, também do tamanho da trilha (o Crupiê fica só com a batucada): o sax-alto da Dona segurando as notas-guia e respondendo nos respiros da melodia; duas trompas do Barão e o chamado delas no fim de cada frase; a caixinha de música do Coringa imitando a melodia uma batida atrás; os guizos do Papai Noel e o "jin-gle bells" no glockenspiel |
| Troca de tela | `transicao` | um sopro de pano que anda da esquerda para a direita junto com a varredura |
| Poderes | `laser`, `vento`, `vidro`, `fogo`, `tira`, `rouba`, `zera`, `bigorna`, `jackpot` | gravações, instrumentos do FluidR3 e sínteses |
| Roleta | `giro`, `slot`, `premio` | celesta, cliques reais, metal e moedas |
| Elevador | `elevador_motor`, `elevador_sino` | o motor subindo (o tchac do relé, o zumbido que sobe de tom com a velocidade, a engrenagem, a polia e o cabo; o jogo o liga enquanto o ponteiro sobe e o apaga na chegada) e a chegada: o baque do freio, o clec da trava e o sino de cúpula em Mi com o vibrafone (um toque só, de quem sobe) |
| Poderes com som próprio | `vampiro`, `sabotador`, `coveiro`, `autofagia`, `ampulheta`, `agnus` | asas e presas, catraca e assobio murchando, pazadas e sino, dentada e marimba, areia grão a grão e celesta, e o sino de capela com o coro de cordas e a celesta subindo (o Agnus ressuscitando) |
| Acabamentos (`gera_acab.py`) | `pousa_<classe>_*`, `bate_<classe>_*`, `bate_dado_<classe>_*` e `rola_<classe>`, das classes laca, vidro, gelo, pedra, metal, madeira, macio, seco (o biscoito), papel, grao, louca, borracha e noite | as mesmas gravações, com o caráter de cada material: a pedra pesada e áspera, o metal grave no feltro e o tinido abafado rolando, a madeira com madeira e o toc oco, o macio abafado, a borracha grave, a louça pousando como duas bolinhas de gude se tocando (o tique breve, sem tinido) e rolando como bolinha de gude (o toque abafado no feltro e o tique curto), o gelo deslizando (um chiado discreto, abaixo do rufo), o papel amassando, os grãos chacoalhando. O `rola_*` é o laço do dado rolando, como o `rufo` |
| Assinaturas (`gera_acab.py`) | `rola_furia`, `rola_onda`, `rola_arrasta`, `pousa_furia_*`, `pousa_tremor_*`, `sopro`, `treno_sino` | a Âncora arrastando no fim do lance (o raspado áspero e abafado do feltro, que prende e solta, no lugar do rolar; o jogo baixa o laço conforme ela perde a força), a Fúria rolando e caindo pesada (a pancada seca e grave na mesa, um corpo grave que assenta logo, o chão roncando e as fichas pulando com o tranco), a Onda rolando com água, o Terremoto pousando mais longo, o fluuush da Ventania e o guizo do Trenó (baixinho, duas ou três vezes no caminho). O Pião rola com o som de dado do material dele |
| Assinaturas novas (`gera_acab.py`) | `as_oco`, `as_eco`, `as_bigorna`, `as_enterra`, `as_reta`, `as_atrai`, `as_casco`, `as_asas`, `as_espinho_*`, `as_tambor_*`, `as_trava`, `as_passo_*` | o Cavalo de Troia (o pouso de madeira e duas batidas abafadas por dentro), o Eco (o pouso e dois ecos cada vez mais fracos e abafados), o Ferreiro (três marteladas na bigorna, cada uma mais baixa), o Coveiro (a pedra abafada, um tímpano fundo curto e baixo e um pouco de terra assentando: discreto), o Laser (o pouso de metal e uma nota de vibrafone), o Buraco Negro (um baque grave e um assobio que cai, como sugado), a Rena (o casco no feltro, a cada salto), o Vampiro (três batidas de asa, curtas), os espinhos do Ouriço (um tique seco e miúdo enquanto rola), o tambor do Revólver (o clique do metal girando) e a trava dele no pouso (o pouso de metal e o estalo duplo do cão), e as patinhas da Viúva Negra (dois tiquinhos secos quase juntos, bem baixinho, uns 13 por segundo enquanto ela anda) |

Quem tem variantes (`bate_0`…`bate_3` etc.) é sorteado no jogo, sem repetir
a anterior, e com um pouco de variação de tom.

As camadas da música são medidas na `trilha.ogg` do repositório (o volume e
o tamanho): o `som.c` as mistura amostra a amostra com ela, e o teste `sons`
confere que todas têm o mesmo número de quadros. Uma camada de outro
tamanho fica fora do jogo. A batucada fica na memória desde o começo; a voz
é só a do chefe do salão, decodificada aos poucos quando o salão muda.
Por cima do volume medido aqui, o `som.c` tem o ajuste de ouvido de cada
chefe (`AJUSTE_CHEFE`): o Crupiê, que não tem voz, +15% na batucada; a Dona
+5%, o Barão e o Coringa +10% na voz; o Papai Noel como está.

O volume de cada som foi igualado ao do som sintetizado que ele substituiu
(`VOLUME` no `gera.py`); `GANHO_DB` guarda os ajustes pedidos depois
(vidro mais forte, ventania mais baixa, o 4º valendo mais intenso). Os sons
que não substituíram nenhum (moedas e `passa`) ficam abaixo dos parentes.

Créditos e licenças das gravações: `desktop/sons/CREDITOS.md`.
