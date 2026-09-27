#!/usr/bin/env python3
"""Gera main/ui/font6x12.c a partir do DejaVu Sans Mono.

Célula 6x12, faces regular e negrito, codepage Latin-1 (0x20..0xFF).
Cada glifo: 12 bytes, um por linha, bits 5..0 = colunas 0..5 (bit 5 = x=0).
"""
import sys
from PIL import Image, ImageDraw, ImageFont

W, H = 6, 12
FIRST, LAST = 0x20, 0xFF
TTF = "/usr/local/lib/python3.12/dist-packages/matplotlib/mpl-data/fonts/ttf/"
FACES = [("regular", TTF + "DejaVuSansMono.ttf"), ("bold", TTF + "DejaVuSansMono-Bold.ttf")]
SIZE = 11        # tamanho em px que melhor preenche a célula 6x12
OX, OY = 0, -1   # ajuste fino da posição do glifo dentro da célula
THRESH = 110     # limiar de binarização


def render(path):
    font = ImageFont.truetype(path, SIZE)
    rows = []
    for cp in range(FIRST, LAST + 1):
        ch = chr(cp)
        img = Image.new("L", (W, H), 0)
        d = ImageDraw.Draw(img)
        try:
            d.text((OX, OY), ch, font=font, fill=255)
        except Exception:
            pass
        px = img.load()
        glyph = []
        for y in range(H):
            bits = 0
            for x in range(W):
                if px[x, y] >= THRESH:
                    bits |= 1 << (5 - x)
            glyph.append(bits)
        rows.append(glyph)
    return rows


def preview(rows, text):
    """Desenha uma linha de texto em ASCII art, para conferência visual."""
    out = [""] * H
    for ch in text:
        g = rows[ord(ch) - FIRST]
        for y in range(H):
            out[y] += "".join("#" if g[y] & (1 << (5 - x)) else "." for x in range(W))
    return "\n".join(out)


def main():
    faces = {name: render(path) for name, path in FACES}
    if "--preview" in sys.argv:
        print(preview(faces["regular"], "Laguna 1.234 (58%)"))
        print()
        print(preview(faces["bold"], "VVPP Acao 42% ->"))
        return
    with open("main/ui/font6x12.c", "w") as f:
        f.write("// GERADO POR tools/gen_font.py - NAO EDITAR A MAO\n")
        f.write('#include "font6x12.h"\n\n')
        for name, rows in faces.items():
            f.write(f"const uint8_t font6x12_{name}[FONT_GLYPHS][FONT_H] = {{\n")
            for cp, g in zip(range(FIRST, LAST + 1), rows):
                vis = f"'{chr(cp)}'" if 0x20 < cp < 0x7F else ""
                f.write("    {" + ",".join(f"0x{b:02X}" for b in g) + f"}}, // {cp:#04x} {vis}\n")
            f.write("};\n\n")
    print("main/ui/font6x12.c escrito")


if __name__ == "__main__":
    main()
