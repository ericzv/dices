# Dado em Casa

Duelo de apostas com dados, ERIC contra LILI: contra o computador (que joga
como LILI) ou com duas pessoas no mesmo teclado. Nasceu no
[Cardputer ADV](https://docs.m5stack.com/en/core/Cardputer-Adv) e agora roda
também no PC (Windows, macOS e Linux) — com o **mesmo código de jogo**.

Cada rodada, os dois montam uma fila de até quatro dados e jogam um por um.
Depois de cada dado: parar ou arriscar o próximo. O primeiro sempre vale; os
seguintes precisam ser iguais ou maiores que o último que vale — se sair menor,
os dois se anulam. Entre os turnos tem aposta, e 30 dados especiais mudam as
regras (veja o menu **Dados** no jogo).

## Jogar

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
| Tab | Confirma a fila / para |
| Backspace | Tira o último dado da fila |
| 1–7 | Escolhe direto o dado da mão, a opção da aposta ou o item da loja |
| X | Recusa o prêmio / sai da loja |
| M | Liga e desliga o som |
| F11 ou Alt+Enter | Tela cheia |
| F12 | Salva uma foto da tela (`dado_*.png`) |

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

### Publicar uma versão

Cada push compila os três sistemas no GitHub Actions. Para gerar uma Release
com link de download:

```sh
git tag v0.1.0
git push origin v0.1.0
```

## Organização

```
src/          o jogo, igual ao do Cardputer
  games/dado.c    regras, animação e desenho (framebuffer 240x135 RGB565)
  ui/             gfx e fonte 6x12
  hal/            contrato com a plataforma: teclado, som, tela
desktop/      a plataforma PC: janela, teclado, som sintetizado, ícone
tests/        simulador de partidas
tools/        geradores da fonte e do ícone
cardputer/    arquivos do firmware do Cardputer (ESP-IDF), fora do build do PC
```

`src/games/dado.c`, `src/ui/gfx.c` e `src/ui/font6x12.c` são os mesmos
arquivos do firmware: uma correção de regra feita aqui vale nos dois lugares.
Os cabeçalhos de `src/hal/` são a versão PC; no Cardputer valem os do
firmware.
