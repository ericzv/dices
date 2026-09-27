#!/usr/bin/env python3
"""Gera o icone do Dado em Casa para o PC.

Saidas, todas em desktop/:
  icone.h     RGBA 64x64 para o icone da janela (embutido no executavel)
  icone.ico   icone do .exe no Windows
  icone.icns  icone do .app no macOS

O desenho imita a mesa do jogo: feltro verde com aro de latao, o dado de
marfim de ERIC na frente e o de ebano de LILI atras. Cores tiradas do dado.c.

Uso:  python3 tools/gen_icone.py      (precisa de Pillow)
"""
import math
import os
from PIL import Image, ImageDraw

FELTRO = (18, 51, 38)
FELTRO2 = (24, 64, 48)
LATAO = (190, 156, 86)
LATAO_ESC = (110, 90, 46)
MARFIM_D = (232, 220, 194)
MARFIM = (238, 231, 214)
EBANO = (34, 30, 29)
SOMBRA = (8, 26, 19)

N = 1024          # desenha grande e reduz: bordas suaves em qualquer tamanho
AQUI = os.path.dirname(os.path.abspath(__file__))
DESTINO = os.path.join(AQUI, "..", "desktop")


def gira(pts, cx, cy, ang):
    c, s = math.cos(ang), math.sin(ang)
    return [(cx + x * c - y * s, cy + x * s + y * c) for x, y in pts]


def dado(d, cx, cy, lado, ang, corpo, aro, tinta, pips):
    """d6 de canto aparado, como o dado.c desenha."""
    h, corte = lado / 2, lado * 0.16
    contorno = [(-h + corte, -h), (h - corte, -h), (h, -h + corte), (h, h - corte),
                (h - corte, h), (-h + corte, h), (-h, h - corte), (-h, -h + corte)]
    d.polygon(gira(contorno, cx + lado * 0.06, cy + lado * 0.09, ang), fill=SOMBRA)
    d.polygon(gira(contorno, cx, cy, ang), fill=aro)
    miolo = [(x * 0.88, y * 0.88) for x, y in contorno]
    d.polygon(gira(miolo, cx, cy, ang), fill=corpo)
    o, r = lado * 0.25, lado * 0.085
    for px, py in pips:
        (x, y), = gira([(px * o, py * o)], cx, cy, ang)
        d.ellipse((x - r, y - r, x + r, y + r), fill=tinta)


def desenha():
    img = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    m, raio = N * 0.03, N * 0.18
    d.rounded_rectangle((m, m, N - m, N - m), raio, fill=LATAO_ESC)
    b = m + N * 0.025
    d.rounded_rectangle((b, b, N - b, N - b), raio * 0.88, fill=LATAO)
    f = b + N * 0.03
    d.rounded_rectangle((f, f, N - f, N - f), raio * 0.8, fill=FELTRO)
    # trama do feltro, como na mesa
    for y in range(int(f + 40), int(N - f - 30), 34):
        for x in range(int(f + 40 + (y // 34 % 2) * 17), int(N - f - 30), 50):
            d.rectangle((x, y, x + 9, y + 9), fill=FELTRO2)

    tres = [(-1, -1), (0, 0), (1, 1)]
    cinco = [(-1, -1), (1, -1), (0, 0), (-1, 1), (1, 1)]
    dado(d, N * 0.63, N * 0.37, N * 0.38, math.radians(14), EBANO, LATAO, MARFIM, tres)
    dado(d, N * 0.40, N * 0.62, N * 0.44, math.radians(-10), MARFIM_D, LATAO, EBANO, cinco)
    return img


def main():
    img = desenha()
    tamanhos = [16, 24, 32, 48, 64, 128, 256]
    img.resize((256, 256), Image.LANCZOS).save(
        os.path.join(DESTINO, "icone.ico"), sizes=[(t, t) for t in tamanhos])
    img.save(os.path.join(DESTINO, "icone.icns"))

    p = img.resize((64, 64), Image.LANCZOS)
    dados = p.tobytes()
    with open(os.path.join(DESTINO, "icone.h"), "w") as f:
        f.write("// GERADO POR tools/gen_icone.py - NAO EDITAR A MAO\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        f.write("#define ICONE_LADO 64\n\n")
        f.write("static const uint8_t ICONE_RGBA[ICONE_LADO * ICONE_LADO * 4] = {\n")
        for i in range(0, len(dados), 16):
            f.write("    " + ",".join(str(v) for v in dados[i:i + 16]) + ",\n")
        f.write("};\n")
    img.resize((256, 256), Image.LANCZOS).save(os.path.join(DESTINO, "icone.png"))
    print("icone gerado em", os.path.normpath(DESTINO))


if __name__ == "__main__":
    main()
