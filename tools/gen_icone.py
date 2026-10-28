#!/usr/bin/env python3
"""Gera o icone do Dado em Casa para o PC e para o site.

Saidas, todas em desktop/:
  icone.h     RGBA 64x64 para o icone da janela (embutido no executavel)
  icone.ico   icone do .exe no Windows
  icone.icns  icone do .app no macOS
  icone.png   256x256, o do site (favicon e a pagina)

O desenho e do proprio jogo, no estilo da mesa (tools/icone_pixel.c): o
feltro pontilhado, a moldura de latao em rampa, o dado de ebano atras e o de
marfim na frente, com o contorno escuro. Os tamanhos pequenos (16 a 64) sao
desenhados cada um no seu tamanho; os grandes sao o de 64 ampliado em pixels
inteiros, sem borrar.

Uso:  python3 tools/gen_icone.py      (precisa de Pillow e de um compilador C)
"""
import os
import subprocess
import tempfile
from PIL import Image

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.normpath(os.path.join(AQUI, ".."))
DESTINO = os.path.join(RAIZ, "desktop")
NATIVOS = [16, 24, 32, 48, 64]


def desenha(pasta):
    """Compila e roda o icone_pixel.c: um .rgba por tamanho nativo."""
    exe = os.path.join(pasta, "icone_pixel")
    cc = os.environ.get("CC", "cc")
    subprocess.run([cc, "-O2", "-w", "-I" + os.path.join(RAIZ, "src"), "-I" + os.path.join(RAIZ, "desktop"),
                    "-DDADO_VERSAO=\"icone\"", "-o", exe, os.path.join(AQUI, "icone_pixel.c"),
                    os.path.join(RAIZ, "src", "ui", "gfx.c"), os.path.join(RAIZ, "src", "ui", "fonte.c"),
                    os.path.join(RAIZ, "desktop", "rede.c"), "-lm", "-lpthread"], check=True)
    subprocess.run([exe, pasta] + [str(n) for n in NATIVOS], check=True)
    img = {}
    for n in NATIVOS:
        with open(os.path.join(pasta, "icone_%d.rgba" % n), "rb") as f:
            img[n] = Image.frombytes("RGBA", (n, n), f.read())
    return img


def amplia(img, n):
    return img.resize((n, n), Image.NEAREST)


def main():
    with tempfile.TemporaryDirectory() as pasta:
        img = desenha(pasta)
    base = img[64]
    for n in (128, 256, 512, 1024):                      # os grandes: o de 64 em pixels inteiros
        img[n] = amplia(base, n)

    tamanhos = [16, 24, 32, 48, 64, 128, 256]
    img[256].save(os.path.join(DESTINO, "icone.ico"), sizes=[(t, t) for t in tamanhos],
                  append_images=[img[t] for t in tamanhos if t != 256])
    img[1024].save(os.path.join(DESTINO, "icone.icns"),
                   append_images=[img[t] for t in (16, 32, 64, 128, 256, 512)])

    dados = base.tobytes()
    with open(os.path.join(DESTINO, "icone.h"), "w") as f:
        f.write("// GERADO POR tools/gen_icone.py - NAO EDITAR A MAO\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        f.write("#define ICONE_LADO 64\n\n")
        f.write("static const uint8_t ICONE_RGBA[ICONE_LADO * ICONE_LADO * 4] = {\n")
        for i in range(0, len(dados), 16):
            f.write("    " + ",".join(str(v) for v in dados[i:i + 16]) + ",\n")
        f.write("};\n")
    img[256].save(os.path.join(DESTINO, "icone.png"))
    print("icone gerado em", os.path.normpath(DESTINO))


if __name__ == "__main__":
    main()
