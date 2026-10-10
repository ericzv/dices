# Console no ESP32-P4 (Guition JC8012P4A1, 10,1")

Um console de jogos para a placa **Guition JC8012P4A1** (ESP32-P4 + ESP32-C6,
tela IPS de 10,1" 1280×800 com toque capacitivo). O primeiro jogo é o
**Dado em Casa**, o mesmo `src/games/dado.c` do PC e do navegador.

Este diretório é um projeto paralelo: não mexe no jogo nem no build do PC.

## O que tem

- **Tela de início no jeito do PS5.** A fileira de jogos no alto (o jogo em
  foco maior, com borda branca), a arte do jogo no fundo e, embaixo, o nome,
  a descrição e o botão **Jogar**. Se o jogo ficou em pausa, o botão vira
  **Continuar** e aparece o **✕** para fechar a partida (pede um segundo toque).
  Ao lado, um lugar "Em breve" para os próximos jogos.
- **Ajustes** (a engrenagem no canto): volume, música do jogo, brilho, girar a
  imagem 180°, calibrar o toque, as opções do jogo (no Dado em Casa,
  animações rápidas), o relógio e um cartão "Sobre" com o chip, a memória, a
  versão do painel e do firmware.
- **Durante o jogo**, a mesa ocupa os 1280×720 de cima: os 640×360 do jogo
  em 2×, pixel por pixel, sem borrar (o texto vem no próprio quadro, no mesmo
  grid de pixels da mesa, como no PC). Os 80 px de baixo são a barra do
  sistema: **Início** à esquerda e o volume à direita.
- **Central de controle**: o botão Início da barra (ou o botão **BOOT** da
  placa) pausa o jogo e sobe um painel com Continuar, Ir para o início, Fechar
  o jogo, volume e brilho.
- **Teclado na tela** quando o jogo pede texto (o nome do perfil no Modo
  Desafiante).
- **Som**: as mesmas gravações do PC (trilha e efeitos, com as variantes de
  cada batida), pelo codec ES8311 e o alto-falante da placa.

### Jogar com o dedo

| Gesto | O que faz no jogo |
|---|---|
| Tocar | Clica (escolhe, joga o dado, compra...) |
| Arrastar | Passa o "mouse" por cima (o cursor segue o dedo) |
| Segurar ~0,5 s | Mostra tudo sobre o dado embaixo do dedo (a tecla I do PC) |
| `<` no alto | Volta / sai da partida, como no celular |

As dicas do jogo mostram as teclas do PC, como no site aberto no celular: o
jogo as desenha dentro do próprio quadro e o `dado.c` é o mesmo do PC. Tudo
o que elas pedem também se faz tocando (os botões, o `<` do alto, os dados).

## A placa e as revisões dela

A Guition já vendeu o JC8012P4A1 com duas telas diferentes (o mesmo chip
JD9365, sequência de início diferente) e com duas gerações do ESP32-P4:

| Revisão | Como saber | Tela | Firmware |
|---|---|---|---|
| V3 | o esptool diz **ESP32-P4 (revision v3.x)** | nova (V2) | padrão |
| V2 | revisão abaixo de v3.0, traseira **2628** ou mais | nova (V2) | `sdkconfig.p4_antigo` |
| V1 | revisão abaixo de v3.0, traseira até **2627** | primeira (V1) | `sdkconfig.p4_antigo` |

O esptool mostra a revisão ao gravar (`Chip is ESP32-P4 (revision v3.1)`).
Placas compradas de meados de 2026 para cá costumam ser V3.

A **versão da tela** não precisa de recompilar: o firmware escolhe sozinho (V2
nos chips v3, V1 nos anteriores) e guarda. Se a tela ficar **preta ou
listrada**, ligue a placa e **aperte e solte o BOOT nos primeiros 8 segundos**
(não segure ao ligar): ela troca para a outra tela e reinicia. Dá para trocar
também em **Ajustes → Sobre**.

## Compilar e gravar

Precisa do [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) **5.5** ou
**6.x** (testado com 5.5.4, 5.5.5 e 6.1). Nenhum componente externo: só o IDF.

```sh
cd esp32p4
idf.py set-target esp32p4
idf.py build
idf.py -p PORTA flash monitor
```

Para placas com **ESP32-P4 anterior a v3.0** (V1 e V2 da tabela), compile com
a configuração extra, a partir de um `sdkconfig` apagado:

```sh
rm -f sdkconfig
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.p4_antigo" build
```

Se gravar o firmware da geração errada, o esptool avisa que a revisão do chip
não confere — é só trocar a configuração.

A placa tem três USB-C. Para gravar e ver o log use a do conversor serial
(CH340, "USB from UART0"): é a que aparece como porta serial no computador.

### Primeira vez

1. Abertura, e depois **"Vamos ajustar o toque"**: toque no centro de cada um
   dos quatro alvos. O console descobre sozinho como o painel de toque está
   montado (eixos, sentido e escala). Se um toque sair do alvo, ele pede de novo.
2. **"A seta aponta para cima?"**: se a imagem estiver de cabeça para baixo,
   toque em **Girar a imagem**.
3. Pronto: a tela de início. Os ajustes ficam guardados.

Para recalibrar depois: **Ajustes → Calibrar o toque**.

### O que ver no log serial

`idf.py monitor` mostra a revisão do chip, a versão da tela escolhida, o ID que
o JD9365 respondeu, se o toque (GSL3680) e o codec (ES8311) responderam e
quanta PSRAM sobrou.

## Simulador no PC

O mesmo console e o mesmo jogo, numa janela de 1280×800, com o mouse no lugar
do dedo e o som do PC (`desktop/som.c` e as gravações):

```sh
cmake -S esp32p4/simulador -B build-console
cmake --build build-console
./build-console/console_sim             # Esc faz o botão de início, F12 tira foto
./build-console/console_fotos /tmp/f    # sem janela: liga, joga e fotografa cada tela
```

No Linux, instale antes os mesmos pacotes do jogo de PC
(`libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`).

## Como funciona

```
esp32p4/
  console/        o console, portátil (o firmware e o simulador usam os mesmos arquivos)
    console.c       estado, ajustes guardados, toque, troca de telas
    inicio.c        tela de início
    ajustes.c       ajustes
    partida.c       jogo, barra do sistema, central de controle, teclado da tela
    primeira.c      abertura, calibração do toque, "a imagem está de pé?"
    ui.c            ícones, botões, chaves, barras de deslizar
    desenho.c       formas suavizadas e degradês em RGB565 (1280x800)
    texto.c         texto TrueType nítido do console (stb_truetype), com cache de letras
    catalogo.c      a lista de jogos
    jogo_dado.c     o Dado em Casa no console: capa, ícone, logotipo, adaptador
  main/           o firmware: tela, toque, som, relógio, salvamento
    tela.c          JD9365 por MIPI-DSI, dois quadros, giro com a PPA, luz
    painel_jd9365.h as sequências de início das duas telas
    toque.c         GSL3680 (e toque_fw.h, o firmware que vai para o chip)
    audio.c         I2S, codec ES8311 e amplificador
    som_p4.c        o som do jogo: as gravações do sons.bin e o misturador
    relogio.c       RX8025T
    salva_nvs.c     o salvamento dos jogos na flash (NVS), um espaço por jogo
    placa.h         os pinos
  ferramentas/    geradores (sons.bin e fontes)
  simulador/      o console no PC
```

- O console desenha numa tela deitada de 1280×800; a cada quadro a **PPA** do
  ESP32-P4 gira essa imagem um quarto de volta para o painel (que é em pé,
  800×1280) e o quadro troca no fim da varredura, sem rasgar.
- A ampliação 2× do jogo é feita pelo processador, pixel por pixel (a PPA
  amplia com filtro e borraria a arte em pixels).
- O `dado.c` guarda quase 1 MB em variáveis estáticas; o fragmento
  `main/psram.lf` as põe na PSRAM.
- **Sons:** o PC toca as gravações de `desktop/sons/*.ogg`. Abrir OGG no
  ESP32-P4 custaria processador demais, então elas vão convertidas para
  `main/sons.bin` (2,2 MB em IMA ADPCM, um trecho por arquivo, com o mesmo
  nome); `main/som_p4.c` lê cada voz direto dali, aos poucos, sem abrir nada
  na memória, e segue o `desktop/som.c` (variantes sorteadas, tom variando
  nas batidas, rufo e giro com rampa abaixando a trilha). Se os sons do PC
  mudarem, o build avisa; gere de novo com o simulador (da raiz):

  ```sh
  cmake -S esp32p4/simulador -B build-console
  cmake --build build-console --target sons_p4
  ```

  Um som novo no jogo (um `SOM_...` a mais em `hal/som.h`) também precisa do
  nome dele na tabela `NOME` de `main/som_p4.c`, como no `desktop/som.c`.

### Adicionar um jogo

1. O jogo desenha no próprio quadro (por exemplo 640×360 RGB565, como o
   Dado em Casa) e recebe toque como mouse.
2. Escreva um adaptador como `console/jogo_dado.c`: nome, descrição,
   etiquetas, a capa (desenhada em 1280×800), o ícone, o logotipo e as funções
   de abrir, fechar, passo, desenho e toque.
3. Ponha o jogo em `console/catalogo.c` e os arquivos dele em
   `main/CMakeLists.txt` e `simulador/CMakeLists.txt`.

O que o jogo guarda com `salva_grava` fica no espaço dele (o `id` do
catálogo), separado dos outros jogos e dos ajustes do console.

## Ainda não tem

- **Online**: o Wi-Fi da placa está no ESP32-C6 (ESP-Hosted); o menu Online do
  jogo diz que esta versão não tem rede.
- Bateria, cartão de memória, câmera e microfone não são usados.

## Créditos e licenças

- Fonte do console: [Inter](https://rsms.me/inter/) (SIL OFL 1.1,
  `ferramentas/fontes/OFL-Inter.txt`), recortada para Latin-1. O nome do jogo
  e a capa usam a Jersey 10 do jogo (`tools/fontes/OFL-Jersey10.txt`).
- Sons: as gravações do jogo de PC (`desktop/sons/CREDITOS.md`).
- [stb_truetype](https://github.com/nothings/stb) e, no gerador de sons,
  stb_vorbis (domínio público / MIT).
- Sequências de início do JD9365: do fabricante (BSP da Guition), as mesmas do
  modelo `JC8012P4A1` / `JC8012P4A1-V2` do ESPHome.
- Firmware do toque (`main/toque_fw.h`): o mesmo que o ESPHome usa nesta placa
  (`esphome-libs/gsl3670-firmware`).
- Registradores do ES8311 conforme o `esp_codec_dev` da Espressif (Apache 2.0).
