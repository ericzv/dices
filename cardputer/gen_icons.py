#!/usr/bin/env python3
"""Gera main/ui/icones.c: tres icones 28x28 com 4 niveis de alfa.

Desenha em 8x a resolucao e reduz, o que produz bordas suaves apesar do
tamanho. Cada pixel ocupa 2 bits; 28x28 = 196 bytes por icone.
"""
from PIL import Image, ImageDraw, ImageChops

S = 28          # tamanho final em px
K = 8           # fator de supersampling
B = S * K       # canvas de trabalho


def novo():
    return Image.new("L", (B, B), 0)


def circulo(img, cx, cy, r, fill=255):
    ImageDraw.Draw(img).ellipse([cx - r, cy - r, cx + r, cy + r], fill=fill)


def anel(cx, cy, r, esp):
    """Anel como diferenca de dois discos."""
    fora, dentro = novo(), novo()
    circulo(fora, cx, cy, r)
    circulo(dentro, cx, cy, r - esp)
    return ImageChops.subtract(fora, dentro)


def olho():
    """Amendoa = intersecao de dois discos grandes, com iris e pupila."""
    c = B / 2
    R = B * 0.78
    a, b = novo(), novo()
    circulo(a, c, c + R - B * 0.33, R)
    circulo(b, c, c - R + B * 0.33, R)
    amendoa = ImageChops.darker(a, b)

    ai, bi = novo(), novo()
    esp = B * 0.055
    circulo(ai, c, c + R - B * 0.33, R - esp)
    circulo(bi, c, c - R + B * 0.33, R - esp)
    interior = ImageChops.darker(ai, bi)

    img = ImageChops.subtract(amendoa, interior)     # contorno
    iris = anel(c, c, B * 0.20, B * 0.05)
    img = ImageChops.lighter(img, iris)
    pupila = novo()
    circulo(pupila, c, c, B * 0.085)
    return ImageChops.lighter(img, pupila)


def cruz():
    """Cruz medica de bracos iguais, levemente arredondada."""
    img = novo()
    d = ImageDraw.Draw(img)
    c, br, cp = B / 2, B * 0.155, B * 0.40
    r = B * 0.03
    d.rounded_rectangle([c - br, c - cp, c + br, c + cp], radius=r, fill=255)
    d.rounded_rectangle([c - cp, c - br, c + cp, c + br], radius=r, fill=255)
    return img


def engrenagem():
    """Coroa com oito dentes, aro e furo central."""
    img = novo()
    d = ImageDraw.Draw(img)
    c = B / 2
    rd = B * 0.46          # ponta dos dentes
    for i in range(8):
        ang = i * 45
        d.pieslice([c - rd, c - rd, c + rd, c + rd], ang - 13, ang + 13, fill=255)
    circulo(img, c, c, B * 0.345)
    img = ImageChops.subtract(img, aro_interno(c))
    return img


def aro_interno(c):
    """Furo central da engrenagem."""
    img = novo()
    circulo(img, c, c, B * 0.145)
    return img


def controle():
    """Controle de video game: corpo com dois punhos, cruz e dois botoes."""
    img = novo()
    d = ImageDraw.Draw(img)
    c = B / 2
    corpo_w, corpo_h = B * 0.76, B * 0.40
    d.rounded_rectangle([c - corpo_w / 2, c - corpo_h / 2,
                         c + corpo_w / 2, c + corpo_h / 2],
                        radius=B * 0.14, fill=255)
    circulo(img, c - corpo_w / 2 + B * 0.09, c, B * 0.205)
    circulo(img, c + corpo_w / 2 - B * 0.09, c, B * 0.205)

    # Recorta a cruz direcional e os dois botoes.
    furo = novo()
    fd = ImageDraw.Draw(furo)
    cx = c - B * 0.20
    br, cp = B * 0.048, B * 0.135
    fd.rectangle([cx - br, c - cp, cx + br, c + cp], fill=255)
    fd.rectangle([cx - cp, c - br, cx + cp, c + br], fill=255)
    circulo(furo, c + B * 0.145, c - B * 0.075, B * 0.068)
    circulo(furo, c + B * 0.275, c + B * 0.075, B * 0.068)
    return ImageChops.subtract(img, furo)


def picareta():
    """Cabeca de duas pontas na horizontal e cabo na vertical."""
    img = novo()
    d = ImageDraw.Draw(img)
    c = B / 2
    topo = c - B * 0.22
    # cabeca: dois bicos que afinam para fora
    d.polygon([(c - B*0.44, topo - B*0.02), (c - B*0.06, topo - B*0.10),
               (c - B*0.06, topo + B*0.10), (c - B*0.44, topo + B*0.06)],
              fill=255)
    d.polygon([(c + B*0.44, topo - B*0.02), (c + B*0.06, topo - B*0.10),
               (c + B*0.06, topo + B*0.10), (c + B*0.44, topo + B*0.06)],
              fill=255)
    # olhal e cabo
    d.rectangle([c - B*0.09, topo - B*0.13, c + B*0.09, topo + B*0.13], fill=255)
    d.rectangle([c - B*0.055, topo, c + B*0.055, c + B*0.44], fill=255)
    return img


def encruzilhada():
    """Caminho que se abre em dois: a historia de escolhas."""
    img = novo()
    d = ImageDraw.Draw(img)
    c = B / 2
    w = int(B * 0.085)
    d.line([(c, c + B*0.42), (c, c - B*0.02)], fill=255, width=w)
    d.line([(c, c - B*0.02), (c - B*0.32, c - B*0.36)], fill=255, width=w)
    d.line([(c, c - B*0.02), (c + B*0.32, c - B*0.36)], fill=255, width=w)
    circulo(img, c - B*0.32, c - B*0.36, B*0.085)
    circulo(img, c + B*0.32, c - B*0.36, B*0.085)
    return img


def letra_a():
    """Um A: nao ha simbolo mais direto para jogo de palavras."""
    img = novo()
    d = ImageDraw.Draw(img)
    c = B / 2
    w = int(B * 0.115)
    d.line([(c - B*0.30, c + B*0.38), (c, c - B*0.38)], fill=255, width=w)
    d.line([(c + B*0.30, c + B*0.38), (c, c - B*0.38)], fill=255, width=w)
    d.line([(c - B*0.16, c + B*0.08), (c + B*0.16, c + B*0.08)],
           fill=255, width=w)
    return img


def dado():
    """Dado de cinco pontos, cantos arredondados, com as pips vazadas."""
    img = novo()
    d = ImageDraw.Draw(img)
    c = B / 2
    m = B * 0.40
    d.rounded_rectangle([c - m, c - m, c + m, c + m], radius=B * 0.13, fill=255)
    furo = novo()
    off = B * 0.20
    for dx, dy in ((-1, -1), (1, -1), (0, 0), (-1, 1), (1, 1)):
        circulo(furo, c + dx * off, c + dy * off, B * 0.075)
    return ImageChops.subtract(img, furo)


def para_2bits(img):
    p = img.resize((S, S), Image.LANCZOS).load()
    dados = []
    for y in range(S):
        linha = []
        acc = bits = 0
        for x in range(S):
            a = min(3, p[x, y] * 4 // 256)
            acc = (acc << 2) | a
            bits += 2
            if bits == 8:
                linha.append(acc)
                acc = bits = 0
        if bits:
            linha.append(acc << (8 - bits))
        dados.append(linha)
    return dados


def main():
    icones = [("olho", olho()), ("cruz", cruz()), ("controle", controle()),
              ("engrenagem", engrenagem()), ("picareta", picareta()),
              ("encruzilhada", encruzilhada()), ("letra_a", letra_a()),
              ("dado", dado())]
    with open("main/ui/icones.c", "w") as f:
        f.write("// GERADO POR tools/gen_icons.py - NAO EDITAR A MAO\n")
        f.write('#include "icones.h"\n\n')
        for nome, img in icones:
            d = para_2bits(img)
            f.write(f"const uint8_t icone_{nome}[ICONE_BYTES] = {{\n")
            for linha in d:
                f.write("    " + ",".join(f"0x{b:02X}" for b in linha) + ",\n")
            f.write("};\n\n")
    print("main/ui/icones.c escrito")

    # conferencia em ASCII
    for nome, img in icones:
        print(f"\n{nome}")
        p = img.resize((S, S), Image.LANCZOS).load()
        for y in range(S):
            print("".join(" .:#"[min(3, p[x, y] * 4 // 256)] for x in range(S)))


if __name__ == "__main__":
    main()
