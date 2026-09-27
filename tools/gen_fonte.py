#!/usr/bin/env python3
"""Gera src/ui/fonte.c a partir da DotGothic16 (tools/fontes, licenca OFL).

A DotGothic16 e uma fonte de pixels desenhada numa grade de 16 px: no corpo
16 cada pixel dela cai exatamente num pixel da tela. Latim e monoespacado,
8 px de avanco. Guardamos Latin-1 (0x20..0xFF) para ter os acentos.
Cada glifo: ALTURA linhas de 8 bits (bit 7 = coluna 0).

Uso:  python3 tools/gen_fonte.py      (precisa de Pillow)
"""
import os
from PIL import Image, ImageDraw, ImageFont

AQUI = os.path.dirname(os.path.abspath(__file__))
TTF = os.path.join(AQUI, "fontes", "DotGothic16-Regular.ttf")
SAIDA = os.path.join(AQUI, "..", "src", "ui", "fonte.c")
CORPO, TOPO, ALTURA, AVANCO = 16, 3, 20, 8
LIMIAR = 110


def glifo(font, cp):
    img = Image.new("L", (AVANCO + 4, 32), 0)
    ImageDraw.Draw(img).text((0, 0), chr(cp), font=font, fill=255)
    linhas = []
    for y in range(TOPO, TOPO + ALTURA):
        b = 0
        for x in range(AVANCO):
            if img.getpixel((x, y)) >= LIMIAR:
                b |= 0x80 >> x
        linhas.append(b)
    return linhas


def main():
    font = ImageFont.truetype(TTF, CORPO)
    with open(SAIDA, "w") as f:
        f.write("// GERADO POR tools/gen_fonte.py - NAO EDITAR A MAO\n")
        f.write("// DotGothic16, (c) 2020 The DotGothic16 Project Authors, SIL OFL 1.1\n")
        f.write('#include "fonte.h"\n\n')
        f.write("const uint8_t FONTE[FONTE_N][FONTE_ALT] = {\n")
        for cp in range(0x20, 0x100):
            ls = glifo(font, cp) if cp < 0x7F or cp >= 0xA0 else [0] * ALTURA
            nome = chr(cp) if cp >= 0xA0 or cp < 0x7F else "?"
            nome = nome.replace("\\", "barra")
            f.write("    {" + ",".join("0x%02X" % v for v in ls) + "}, // %02X %s\n" % (cp, nome))
        f.write("};\n")
    print("fonte gerada em", os.path.normpath(SAIDA))


if __name__ == "__main__":
    main()
