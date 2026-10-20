# Dado em Casa

Duelo de apostas com dados para dois: contra o computador, com duas pessoas
no mesmo teclado ou **online**, cada um no seu computador. Criado por ERICZ.
Nasceu no
[Cardputer ADV](https://docs.m5stack.com/en/core/Cardputer-Adv) e agora roda
também no PC (Windows, macOS e Linux), numa mesa de 640×360 pixels ampliada
sem borrar, com trilha sonora e efeitos sintetizados pelo próprio jogo.

Cada rodada, os dois montam uma sequência de até quatro dados e jogam um por um.
Depois de cada dado: parar ou arriscar o próximo. O primeiro sempre vale; os
seguintes precisam ser iguais ou maiores que o último que vale — se sair menor,
os dois se anulam. Entre os turnos tem aposta, e 82 dados mudam as
regras (veja o menu **Dados** no jogo; o Modo Desafiante abre mais 4).

## 1 jogador: partida rápida ou Modo Desafiante

- **Partida rápida**: 15 rodadas contra o computador (a entrada de cada rodada
  sobe: 5 fichas, 10 da 6ª e 15 da 11ª), em três níveis —
  **Fácil** (regras simples), **Médio** (pensa, mas erra às vezes) e
  **Difícil** (simula as jogadas antes de decidir). Quem vence a rodada
  escolhe um dado de prêmio entre três: até a 5ª rodada saem comuns e
  incomuns, da 6ª à 10ª incomuns e raros, e depois raros e lendários.
- **Modo Desafiante**: uma run pelo cassino. São 3 salões, cada um com três
  oponentes (do mais fácil ao mais difícil) e o chefe, sempre nessa ordem e
  todos à vista desde o começo. A dificuldade sobe mesa a mesa pelos dados dos
  oponentes: no primeiro salão eles só têm dados de número, quase todos D4, e a
  bolsa cresce a cada mesa e a cada salão. São 21 oponentes, cada um com seu
  jeito de jogar, dados que são a cara dele e um trunfo: no Normal, só os
  chefes trazem o seu; a cada nível, mais oponentes, até todos no nível 10. Os
  chefes também viram na 5ª rodada a partir de um nível: a Dona no 2, o
  Crupiê no 3 e o Barão no 4 (mais fortes no 7, no 8 e no 10). No nível 10,
  depois do Barão ainda há o salão do Coringa, o oponente mais difícil do
  jogo. Você começa com 12 dados simples (6 D6, 4 D4 e 2 moedas) e 4 moedas
  de ouro (mais 1 a cada dois níveis). Cada partida tem 8 rodadas e 50 fichas
  de cada lado; vencer paga poucas moedas, que sobem a cada mesa e a cada
  salão (1, 2, 3 e 5 no primeiro; 4, 5, 6 e 10 no segundo; 7, 8, 9 e 20 no
  terceiro, este só no nível 10), e abre uma loja pequena: a vitrine com 2
  dados (3, 4, 6 ou 9 moedas, pela raridade), um pacote (4 moedas: dados
  embrulhados de um grupo, Números, Ataque, Defesa ou Combo; abre na hora,
  mostra 3 do grupo e você leva 1 ou pula), trocar a vitrine (1, depois 2,
  3...) e tirar um dado da bolsa (3, depois 5, 7... na run inteira).
  Amuletos: um comum no começo, um incomum depois do primeiro chefe e um raro
  depois do segundo (e do Barão, no nível 10). Cabem 5: para levar o sexto,
  um dos seus sai.
  Perdeu, a run acaba. Até 3 perfis, cada um com seu avatar e sua run guardada
  (no PC, numa pasta do usuário; no navegador, no próprio navegador), e o
  **histórico** (H na tela de perfis): partidas e rodadas vencidas, recordes,
  os números de cada dado e a run campeã com a bolsa que venceu. A partida
  rápida conta no último perfil escolhido.
- **Estilo** (E na tela de perfis): o feltro (verde, vinho, azul-noite,
  grafite), o material dos seus dados comuns (marfim, osso, jade, obsidiana,
  ébano) e a mesa (nogueira e ouro, mogno e cobre, ébano e prata, carvalho e
  bronze). Abrem andar a andar: vencer o Barão em cada dificuldade abre mais.
  Os oponentes têm o material do nível deles (os chefes, obsidiana); os dados
  especiais têm um desenho só. Vencer o
  Barão abre a dificuldade seguinte (são 11: a Normal e 10 níveis, uma coisa
  a mais por nível), e os chefes e as dificuldades abrem dados novos: no
  Normal só os de bônus simples; os ataques, os que mexem na sequência e os
  mais estranhos entram aos poucos, alguns por nível. Os oponentes só usam o
  que você já abriu, e as mesas fáceis levam poucos tipos de dado especial. A
  coleção mostra o que está aberto e o que falta.

## Jogar no navegador (e online)

**https://ericzv.github.io/dices/** — abre direto no navegador, sem instalar.

Para jogar online com um amigo: os dois abrem o endereço, escolhem **Online**;
um clica em **Criar sala** e passa o código de 4 letras; o outro clica em
**Entrar com código** e digita. Cada um se vê embaixo da mesa.

As jogadas passam por um servidor MQTT público e gratuito (HiveMQ, com EMQX
e Mosquitto de reserva), que só repassa as mensagens da sala. Os dois
navegadores rodam a mesma partida com a mesma semente; só viajam as teclas
de quem está na vez. Serve bem para jogar entre amigos — não é um servidor
com garantia nem privacidade (quem souber o código vê as jogadas).

## Jogar no computador

Baixe o arquivo do seu sistema na página de
[Releases](https://github.com/ericzv/dices/releases) (ou, antes da primeira
release, em **Actions → última execução → Artifacts**):

| Sistema | Arquivo | Como abrir |
|---|---|---|
| Windows | `DadoEmCasa-windows.zip` | Descompacte e abra `DadoEmCasa.exe`. Se aparecer "O Windows protegeu o computador", clique em **Mais informações → Executar assim mesmo** (o executável não tem assinatura paga). |
| macOS | `DadoEmCasa-macos.zip` | Descompacte, clique com o botão direito em **Dado em Casa → Abrir**. Se o macOS bloquear, vá em **Ajustes → Privacidade e Segurança → Abrir mesmo assim**. |
| Linux | `DadoEmCasa-linux.tar.gz` | `tar xzf DadoEmCasa-linux.tar.gz && ./DadoEmCasa` |

Não precisa instalar nada: é um executável só.

### Teclas

| Tecla | O que faz |
|---|---|
| Setas (ou A/D, W/S) | Move o cursor |
| Enter | Escolhe / joga o próximo dado |
| Tab ou Ctrl | Confirma a sequência / para |
| Backspace | Tira o último dado da sequência |
| I | Mostra tudo sobre o dado escolhido (qualquer tecla fecha) |
| 1–7 | Escolhe direto o dado da mão, a opção da aposta ou o item da loja |
| X | Recusa o prêmio / sai da loja |
| M | Liga e desliga todo o som |
| N | Liga e desliga só a música |
| V | Velocidade normal, média (1,5x) ou rápida (2x): animações e vez do computador (fica guardada) |
| T | Liga e desliga o tremor da tela e o clarão das jogadas fortes (fica guardado) |
| F11 ou Alt+Enter | Tela cheia |
| – e + | Diminuem e aumentam a mesa na tela (de 50% a 100%; fica guardado) |
| F12 | Salva uma foto da tela (`dado_*.png`) |
| ESC (duas vezes) | Sai da partida e volta ao menu |

O menu **Como jogar** explica as regras.

## Compilar

Precisa de um compilador C e do [CMake](https://cmake.org/) 3.16+. O
[raylib](https://www.raylib.com/) (janela, teclado e som) é baixado sozinho na
primeira compilação.

```sh
cmake -B build
cmake --build build --config Release
```

O executável sai em `build/` (`DadoEmCasa`, `DadoEmCasa.exe` ou
`DadoEmCasa.app`).

- **Linux:** instale antes `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`.
- **Windows:** Visual Studio 2022 (com "Desenvolvimento para desktop com C++") já serve. Também dá para gerar o `.exe` a partir do Linux: `cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake`.

Teste sem janela, que joga partidas inteiras com teclas ao acaso:

```sh
ctest --test-dir build --output-on-failure
./build/simula 500 --fotos /tmp/fotos   # salva uma imagem de cada tela
./build/balanco curva 2000 0            # o Desafiante no Normal: a chance de vencer cada mesa
```

`balanco` também mede cada dado e cada amuleto numa mesa só e o recomeço de
partida (veja o começo de `tests/balanco.c`).

### Versão do navegador

Precisa do [Emscripten](https://emscripten.org/):

```sh
emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
cd build-web && python3 -m http.server   # abra http://localhost:8000
```

Para testar o online sem internet, rode um Mosquitto local com websockets e
abra `index.html?servidor=ws://localhost:9001` em duas abas.

O GitHub Actions compila a versão web a cada push e publica na branch
`gh-pages`. Para ela aparecer em `ericzv.github.io/dices`, ligue uma vez:
**Settings → Pages → Build and deployment → Deploy from a branch → `gh-pages` / `(root)`**.
O zip `DadoEmCasa-web` (em Actions → Artifacts) também pode ser enviado ao
[itch.io](https://itch.io) como jogo HTML.

### Publicar uma versão

Cada push compila os três sistemas no GitHub Actions. Para gerar uma Release
com link de download:

```sh
git tag v0.1.0
git push origin v0.1.0
```

## Organização

```
src/          o jogo
  games/dado.c        o jogo inteiro (framebuffer 640x360 RGB565), em partes:
  games/dado/         um arquivo por assunto: regras da fila, poderes, IA, loja e
                      prêmio, desenho dos dados, telas, Modo Desafiante (desafio.inc)
  ui/             primitivas de desenho e as fontes de pixels (Jersey 10, Bitcount Prop Single e Tiny5)
  hal/            contrato com a plataforma: teclado, som, tela
desktop/      a plataforma PC: janela, teclado, som e ícone
  sons/           a trilha e os efeitos (OGG), embutidos no executável no build
web/          a página do navegador e a rede do modo online (MQTT)
tests/        simulador de partidas, regras, Desafiante e conferência dos sons
tools/        geradores da fonte, do ícone e dos sons (tools/sons/)
cardputer/    o firmware do Cardputer (ESP-IDF), fora do build do PC
```

A versão do Cardputer (tela 240x135) ficou em `cardputer/dado.c`: tem as
mesmas regras e a IA, com o desenho do aparelho. Uma regra nova precisa ir
para os dois arquivos.

Para ouvir a trilha e os efeitos fora do jogo:
`./build/exporta_som pasta/` grava um `.wav` de cada. Os sons são feitos por
`tools/sons/gera.py`, a partir de gravações que `tools/sons/baixa.py` baixa
(veja `tools/sons/LEIAME.md`).

As fontes Jersey 10, Bitcount Prop Single e Tiny5 são distribuídas sob a SIL
Open Font License (`tools/fontes/OFL-Jersey10.txt`, `OFL-BitcountPropSingle.txt`
e `OFL-Tiny5.txt`).

Os sons usam gravações de terceiros; os créditos e as licenças estão em
`desktop/sons/CREDITOS.md`.
