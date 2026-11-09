# Guia de estilo do Dado em Casa

Este guia diz como o jogo é desenhado, para que dados, adesivos, telas e
ícones novos já nasçam com a mesma cara do resto. Os nomes entre crases são
funções e constantes de `src/games/dado/`.

## 1. A tela e a paleta

- O quadro tem 640 x 360 pixels em RGB565, ampliado sem filtro.
- Tudo sai de uma **paleta fechada de 52 cores** (`PALETA`, em `cores.inc`),
  arrumada em famílias de tons vizinhos: escuros quentes, cinzas frios,
  marfim, feltro, verdes, madeira, latão e ouro, vinho e vermelho, fogo,
  azuis, água, roxos e pele.
- No fim de cada quadro, `gfx_quantiza` troca cada pixel pela cor mais
  perto da paleta. Por isso **nenhuma cor fora da paleta chega à tela**.
- Uma cor nova só entra se couber numa família. Prefira sempre as cores com
  nome (`C_OURO`, `C_LATAO`, `C_VINHO_CLR`, `C_MARFIM`...).
- O feltro troca com o estilo do perfil (`estilo.inc`). Para o feltro, use
  `feltro_tons[0..5]` (do mais escuro ao realce) ou `C_SOMBRA`, `C_FELTRO`,
  `C_FELTRO2`, nunca um verde fixo.

## 2. Não misture: escolha o tom

A regra que mais pesa: **um fundo, um véu ou um brilho não pode depender
do arredondamento**. Misturar duas cores (`gfx_mistura`, `gfx_rect_alfa`)
e deixar a paleta decidir dá errado de dois jeitos:

- a mistura volta para o tom de partida, e o efeito some (o véu de sombra
  que não escurecia o feltro, a seleção sem fundo);
- ou cai em outra família, e a cor muda (o brilho da pele que virava
  dourado, a etiqueta vermelha com fundo marrom).

O certo é escolher o tom na rampa e deixar o pontilhado fazer a passagem:

| Para                         | Use                                                   |
|------------------------------|-------------------------------------------------------|
| os 4 tons de uma cor         | `rampa_de(cor, r)`: r[0] e r[1] abaixo, r[2] a cor, r[3] acima |
| um tom pela luz              | `tom_rampa(r, luz, x, y)`, luz de -1 (costas) a 1 (frente) |
| fundo escuro tingido         | `tinta_escura(cor)`, o escuro da família dela         |
| escurecer o que está atrás   | `ui_escurece(x, y, w, h, alfa)`, no lugar do `gfx_rect_alfa` com `C_SOMBRA` |
| luz sobre o feltro           | `ui_clareia(x, y, w, h, tons)` ou `luz_no_feltro`     |
| brilho e sombra de um pixel  | `r[3]` e `r[1]` da rampa da cor como ela sai na paleta (`gfx_na_paleta`) |

`gfx_mistura` continua valendo para o que muda a cada quadro (um número
piscando, um rastro que some), onde um degrau a mais ou a menos não
aparece.

## 3. A luz e o pontilhado

- A luz vem **de cima, à esquerda**: o lado de cima e o da esquerda ficam um
  tom acima; o de baixo e o da direita, um ou dois abaixo.
- Todo degradê escolhe, pixel a pixel, entre dois tons vizinhos da rampa
  pelo limiar de uma **matriz de Bayer 4 x 4 presa à tela** (`bayer(x, y)`).
  As faixas ficam cheias, e o pontilhado aparece só na passagem.
- A sombra no feltro é pontilhada na beira (`ficha_sombra`, `ui_sombra`),
  deslocada 1 px para a direita e 2 px para baixo.

## 4. O kit de interface (`interface.inc`)

As mesmas peças em todo o jogo, desenhadas como os dados: contorno quase
preto de 1 px (`C_BARRA`), cantos aparados, metal em rampa e sombra
pontilhada.

| Peça                         | Função                                     | Quando usar |
|------------------------------|--------------------------------------------|-------------|
| Placa de ébano com latão     | `ui_painel`, `painel(y, h, w)`             | avisos e blocos de texto sobre a mesa |
| Cartão de feltro             | `ui_cartao`, `ui_cartao_aceso`             | escolhas lado a lado: loja, perfis, listas |
| Botão com espessura          | `ui_botao`, `botao_clique`, `botao_voltar` | ações; o apertado afunda (`ui_aperta`) |
| Seleção (o cursor)           | `ui_cantos`, `ui_selecao`                  | quatro cantoneiras que respiram; a seleção numa grade acende o fundo com `feltro_tons[5]` |
| Peça em uso                  | `ui_marca`                                 | o que está escolhido, sem ser o cursor: fundo `C_FELTRO2` e contorno parado |
| Etiqueta                     | `ui_etiqueta`, `ui_selo`                   | raridade, tipo, proteção: fundo `tinta_escura` e filete na cor |
| Plaquinha de metal           | `ui_plaqueta`                              | rótulo curto preso numa placa (o "VENCE") |
| Faixas de cima e de baixo    | `faixa_madeira`, `teclas_c`                | a borda de madeira da mesa, com as teclas |

As peças entram descendo e assentam com a mola (`ui_entra`, `sai_mola`).
Cartões lado a lado entram em cascata (`UI_CASCATA`). A troca de tela é a
varredura pontilhada do centro.

## 5. Ícones pequenos

Todos os ícones pequenos seguem o mesmo material:

- contorno de 1 px em `C_BARRA`;
- corpo nos tons da paleta, com a luz de cima à esquerda;
- sombra pontilhada embaixo, quando estão sobre a mesa.

A moeda e a ficha usam o disco das fichas (`ficha_disco`).

| Tamanho                 | Onde                                                  | Ícones |
|-------------------------|-------------------------------------------------------|--------|
| **13 px** (`ICONE_M`)   | junto dos números grandes: faixas, preços, contadores | `moeda_ic`, `ficha` |
| **9 px** (`ICONE_P`)    | junto do texto fino: "vale 3", rótulos do mapa        | `moeda_r(x, y, ICONE_P)`, `icone_cadeado(.., 1, ..)` |
| **7 x 8 px**            | no canto dos dados e nas etiquetas de proteção        | `icone_protecao`: escudo de aço (não anulável) e cadeado de ouro (não roubável) |
| **16 px**               | o desenho de cada bolsa                               | `icone_bolsa_em`, `icone_da_run`, `icone_bolsa` |

- O cadeado de tudo o que ainda está fechado (bolsa, dado ou estilo) é um
  só: `icone_cadeado(cx, cy, e, cor)`, com 9 x 11 pixels, a alça de aço e o
  corpo na cor.
- Moedas maiores (a fileira do saque) usam `moeda_r` com o raio que
  precisarem, no mesmo material.

### Amuletos

- Cada amuleto é um medalhão (`desenha_amuleto`) com três partes:
  - o aro de latão em rampa;
  - o campo afundado no escuro da cor dele (`tinta_escura`);
  - o símbolo em pixels.
- O símbolo é desenhado à mão numa grade de 11 x 11 (`AM_DESENHO`, em
  `desafio/amuletos.inc`). Cada letra é uma cor da paleta (`AM_CHAVE`): por
  exemplo `j`/`J` madeira, `o`/`O`/`y` latão e ouro, `r`/`R` vermelho,
  `u`/`U` azul, `w`/`W` marfim e `K` o quase preto. O `.` é vazio.
- A luz vem de cima, à esquerda: a beira de cima e a da esquerda num tom
  acima, a de baixo e a da direita num tom abaixo.
- O contorno não se desenha: ele sai sozinho, com 1 px na tela e os cantos
  aparados.
- O símbolo cresce em degraus inteiros (1x na faixa, 2x e 3x na escolha, 4x
  na vitrine).
- Abaixo de r = 8 (no canto dos dados), vai só o símbolo, sem medalhão.
- Apagado (o que ainda está fechado), tudo passa para os tons do feltro.

## 6. Texto

| Fonte                   | Constante              | Uso |
|-------------------------|------------------------|-----|
| Jersey (grossa)         | `1`, `txt(.., true)`   | títulos, nomes, números |
| Fina                    | `GFX_FINO`             | texto corrido, legendas, rótulos |
| Miúda                   | `GFX_MIUDO`            | só quando a fina não cabe |
| Fina de largura fixa    | `GFX_FINO_FIXO`        | números que mudam sem dançar |

- A fonte fina desenha uns 6 px abaixo do `y` pedido: alinhe pela letra,
  não pelo `y`.
- Texto sobre fundo com desenho (o piso do salão, um cartão claro) leva
  contorno escuro (`texto_no_chao`) ou sombra (`txt_sombra_c`).
- Texto que pode não caber quebra em linhas (`quebra_linhas`) ou troca de
  fonte (`nome_no_cartao`). Nunca deixe texto cortado.

## 7. Dados e adesivos

- O corpo do dado usa os tons exatos de `LUZ_CORPO`, fora da paleta e só
  dele: nada mais pode cair neles.
- O aro vai em rampa pela direção em que a beira olha (`aro_rampa`). O
  número usa a fonte própria dos algarismos (`numero_em`), centrado no
  pixel do meio da forma.
- Um dado novo precisa de:
  - entrada em `TIPO`, `TEXTO_MEDIO`, `TEXTO_CAT` e `VISUAL` (`dados.inc`);
  - novo `D_*` sempre no fim do enum, por causa dos salvamentos;
  - o desenho em `desenho_dado.inc`, com cores da paleta e a luz de cima à
    esquerda;
  - a animação do poder armado em `anima_poder.inc`, se tiver.
- Adesivos (`adesivos.inc`): ícone em pixels com contorno escuro, no mesmo
  tamanho dos outros.
- As marcas de um dado na mesa (`marcas_dado`, em `mesa.inc`) vão sem caixa,
  na fonte fina com contorno escuro, e cada tipo tem o seu lugar:

  | Lugar                 | O que                                         | Cor |
  |-----------------------|-----------------------------------------------|-----|
  | em cima, à esquerda   | o que fez o número: `=5`, `=?`, `4+5`, `12-5` | marfim |
  | em cima, à direita    | os pontos a mais: `+6`, `-1`, `$+4`; o do dado e o que vem de outro no fim, um em cima do outro | ouro (menos: vinho claro) |
  | embaixo               | o multiplicador: `x2`, `x3`                   | fogo claro |

  O que só vem no fim da rodada (a prévia) sai no tom mais escuro da mesma
  cor; no desfecho, acende quando acontece. As etiquetas do próprio dado
  passam por `poe_tag` (`fila.inc`), que guarda a última de cada tipo.

### Os grupos do desenho dos dados

Todo dado especial entra num destes grupos. O desenho diz do que o dado é
feito ou onde mora a identidade dele; nunca é uma figura solta.

| Grupo | O que é | Regras |
|-------|---------|--------|
| **Feito de…** | O dado inteiro é de um material: madeira, latão, veludo, lã, pedra, papel. | O material cobre 60% ou mais da face. Dois ou três tons vizinhos da mesma rampa, pouco contraste. Luz de cima, à esquerda, convexa. O número tem contorno e nada passa por cima dele. |
| **Acabamento de borda** | O centro fica marfim e calmo; a identidade mora na borda, num canto ou num padrão que se repete. | O desenho ocupa a faixa junto da borda (30% dela ou mais). Padrão repetido em vez de figura. O centro em volta do número fica liso. |
| **Noite** | Corpo escuro com brilho miúdo: o cósmico, o misterioso, o oculto. | Corpo escuro em duas camadas. Número claro. Pontos de brilho esparsos, nunca encostando no número. |
| **Ficha e moeda** | Os d2 são discos. | Ficha de cassino com listras na borda, ou metal com aro (moeda), com o mesmo cuidado de material do "Feito de…". |
| **Clássico** | Só os dados comuns. | Marfim liso e aro de latão, sem desenho. É o que faz os especiais se destacarem. |

O que evitar, com os limites medidos na simulação das pranchas:

- **Quase liso**: um traço pequeno num dado marfim. Some na mesa e fica igual
  a outros na bolsa. Limite: menos de 29% da face com desenho (na faixa da
  borda e na face toda). A exceção são os pontos do d6, quando a identidade
  está neles (o Copiador).
- **Desenho solto no centro**: uma figura atrás do número, no meio do marfim.
  Limite: o desenho no meio 1,15 vez ou mais do que na borda, cobrindo menos
  da metade da face.
- **Número disputado**: o desenho encostando em 44% ou mais do contorno do
  número. Abra um campo calmo em volta dele.
- **Luz invertida**: a metade de baixo, à direita, mais clara que a de cima,
  à esquerda.

Ficam como estão, por escolha: o Desafiante, o Dominó e o Panetone.

Para desenhar (`desenho_dado.inc`): `desenho_interno` devolve 0 (o corpo),
1 a 3 (as cores `c1`..`c3` do `VISUAL`) ou `LUZ(c, l)`, a cor `c` na rampa
com a luz `l` de -1 a 1. As ajudas comuns:

- `face_g(lados, u, v, &t)`: 0 no centro da face, 1 na beira de dentro do
  aro; em `t`, onde o ponto cai ao longo da beira. É a régua das faixas de
  borda e do campo calmo do número.
- `celulas` e `celula_vet`: o mosaico torto (rachaduras, grãos, covinhas de
  martelo). `celula_vet` dá o caminho até o centro da célula, para o volume.
- `grao`: o grão de um pixel preso ao desenho (escovado, veludo).
- Os números claros (`tinta = C_MARFIM`) vão nos corpos escuros. A lista fica
  em `desenha_dado_base`.

## 8. Retratos

- Busto de 22 x 27 pixels (`RT_W`, `RT_H`), ampliado 2x (a mesa, o mapa) ou
  3x (o chefe, o fim da partida). Contorno escuro automático.
- Brilho e sombra com `luz()` e `sombra()`. O brilho sobe um tom na rampa da
  cor, então a pele clareia em pele, e não em dourado.
- O retrato fica sempre inteiro na moldura (`ui_aro` no latão, ou no ouro
  para o chefe). Nada se desenha por cima do rosto.

## 9. Antes de juntar uma tela nova

1. Olhe a tela sobre o feltro verde e sobre outro estilo de feltro.
2. Procure fundos e véus feitos com `gfx_mistura` ou `gfx_rect_alfa` e
   troque pelo tom da rampa (seção 2).
3. Confira os textos: nada cortado, contraste bom, a fonte certa.
4. Meça o custo por quadro. Desenho por pixel deve calcular só onde
   precisa (como a luz do mapa, linha a linha) e não guardar buffers à toa.
