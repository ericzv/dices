# O vídeo tutorial

`roteiro.py` grava um vídeo de uns 21 minutos que ensina a jogar, passo a
passo, para quem nunca mexeu num computador: o mouse, o menu, as regras e uma
run do Modo Desafiante do começo ao fim (perfil, amuleto, loja, mapa, uma
partida inteira, o prêmio e como continuar outro dia). Tudo é feito com o
mouse, com narração em português e legendas grandes.

Nada é filmado: o jogo de verdade (`src/games/dado.c`) roda sem janela dentro
de uma biblioteca (`motor.c`), e o roteiro dirige um cursor desenhado,
clica nas mesmas áreas que o mouse clicaria, destaca partes da tela e narra.
Os sons são os do próprio jogo (`desktop/sons/`).

## O que precisa

- `gcc`, `ffmpeg` e a fonte Inter (`fonts-inter`; outra pasta em `DADO_INTER`)
- Python 3 com `numpy pillow soundfile kokoro-onnx` (e `sherpa-onnx` para
  conferir a pronúncia)
- A voz, em `tools/video/modelos/` (fora do git, cerca de 350 MB; outra pasta
  em `DADO_TTS`):

```sh
mkdir -p tools/video/modelos && cd tools/video/modelos
curl -LO https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx
curl -LO https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin
# so para o confere_voz.py:
curl -L https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/sherpa-onnx-whisper-small.tar.bz2 | tar xj
```

A `libmotor.so` é compilada sozinha na primeira vez (apague-a depois de mudar
o jogo).

## Gravar

```sh
cd tools/video
python3 roteiro.py --seco                 # confere o caminho todo, sem imagem (gera a voz)
python3 roteiro.py --previa previa 30     # 1 quadro por segundo em previa/, sem gravar
python3 folha.py previa 600 760           # miniaturas de um trecho, para revisar
python3 roteiro.py saida/tutorial.mp4     # o vídeo (1920x1080, ~80 MB, com capítulos)
python3 confere_voz.py saida/tutorial.mp4.roteiro.txt   # frases que a voz falou estranho
```

Junto do MP4 saem `*.capitulos.txt` (os minutos de cada parte) e
`*.roteiro.txt` (cada frase com o seu tempo). A voz fica em `cache_voz/`:
mudar uma frase só gera ela de novo. O que a legenda mostra e o que a voz
fala podem ser diferentes (`PRONUNCIA` no `gravador.py`: "run" vira "ran").

## Por que a partida sai sempre igual

O jogo é determinístico: a run nasce da hora do relógio no clique em
**criar**, e o roteiro fixa essa hora (`SEMENTE` no `roteiro.py`). As
jogadas da pessoa são as que a IA difícil faria no lugar dela (`plano.py`),
iguais no ensaio e na gravação. Assim a narração pode dizer "saiu três, que é
menor que quatro" e a tela concorda; os `assert` do roteiro param a gravação
se algo mudou.

Se o jogo mudar e a partida sair diferente, procure outra semente:

```sh
python3 busca.py 0 200        # resume cada run: oponente, amuleto, loja, vitória, 1ª rodada
python3 ensaio.py 388872      # o detalhe de uma, rodada a rodada
```

e ajuste `SEMENTE` e as falas da partida (`parte_partida` no `roteiro.py`).
