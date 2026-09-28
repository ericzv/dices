# Dado em Casa

Duelo de apostas com dados para dois: contra o computador, com duas pessoas
no mesmo teclado ou **online**, cada um no seu computador. Criado por ERICZ.
Nasceu no
[Cardputer ADV](https://docs.m5stack.com/en/core/Cardputer-Adv) e agora roda
também no PC (Windows, macOS e Linux), numa mesa de 640×360 pixels ampliada
sem borrar, com trilha sonora e efeitos sintetizados pelo próprio jogo.

Cada rodada, os dois montam uma fila de até quatro dados e jogam um por um.
Depois de cada dado: parar ou arriscar o próximo. O primeiro sempre vale; os
seguintes precisam ser iguais ou maiores que o último que vale — se sair menor,
os dois se anulam. Entre os turnos tem aposta, e 33 dados mudam as
regras (veja o menu **Dados** no jogo).

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
| Tab ou Ctrl | Confirma a fila / para |
| Backspace | Tira o último dado da fila |
| 1–7 | Escolhe direto o dado da mão, a opção da aposta ou o item da loja |
| X | Recusa o prêmio / sai da loja |
| M | Liga e desliga todo o som |
| N | Liga e desliga só a música |
| F11 ou Alt+Enter | Tela cheia |
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
```

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
  games/dado.c    regras, IA, animação e desenho (framebuffer 640x360 RGB565)
  ui/             primitivas de desenho e a fonte de pixels (DotGothic16)
  hal/            contrato com a plataforma: teclado, som, tela
desktop/      a plataforma PC: janela, teclado, trilha e efeitos sintetizados, ícone
web/          a página do navegador e a rede do modo online (MQTT)
tests/        simulador de partidas e exportador de sons (WAV)
tools/        geradores da fonte e do ícone
cardputer/    o firmware do Cardputer (ESP-IDF), fora do build do PC
```

A versão do Cardputer (tela 240x135) ficou em `cardputer/dado.c`: tem as
mesmas regras e a IA, com o desenho do aparelho. Uma regra nova precisa ir
para os dois arquivos.

Para ouvir a trilha, o rufar e os efeitos fora do jogo:
`./build/exporta_som pasta/` grava um `.wav` de cada.

A fonte DotGothic16 é distribuída sob a SIL Open Font License
(`tools/fontes/OFL-DotGothic16.txt`); a Jersey 10, também OFL
(`tools/fontes/OFL-Jersey10.txt`).
