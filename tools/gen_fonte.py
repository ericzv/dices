#!/usr/bin/env python3
"""Gera src/ui/fonte.c: as duas fontes de pixels do jogo (licenca OFL, em tools/fontes).

  Jersey 10 (texto normal): desenhada numa grade em que cada pixel dela vale
      75 unidades; no tamanho nativo cada pixel da fonte cai exatamente num
      pixel do jogo (maiusculas de 10 px, linha de 20 px).
  Tiny5 (texto miudo): mesma ideia, pixel de 128 unidades, maiusculas de 5 px.
  DotGothic16 (texto corrido, fino): os pontos dela tem passo de 76,3
      unidades (cerca de 13 px por em); amostrada nesse passo cai um ponto por
      pixel, traco de 1 px. Fica proporcional: cada glifo e cortado nas
      colunas que tem tinta, com 1 px de espaco.

O texto e desenhado no mesmo framebuffer de 640x360 da mesa, em escala
inteira: assim letra e desenho ficam no mesmo grid de pixels.

Cada glifo vira linhas de bits (bit 0 = coluna 0) e um avanco em pixels.
Guardamos Latin-1 (0x20..0xFF) para ter os acentos; o que a fonte nao tem
vira '?'.

Uso:  python3 tools/gen_fonte.py      (precisa de fonttools)
"""
import os
from fontTools.ttLib import TTFont
from fontTools.pens.recordingPen import DecomposingRecordingPen

AQUI = os.path.dirname(os.path.abspath(__file__))
SAIDA_C = os.path.join(AQUI, "..", "src", "ui", "fonte.c")
SAIDA_H = os.path.join(AQUI, "..", "src", "ui", "fonte.h")


def contornos(gs, nome):
    p = DecomposingRecordingPen(gs)
    gs[nome].draw(p)
    cs, cur = [], []
    for op, args in p.value:
        if op == "moveTo":
            cur = [args[0]]
        elif op in ("lineTo", "qCurveTo", "curveTo"):
            cur.extend(args)
        elif op in ("closePath", "endPath"):
            if cur:
                cs.append(cur)
            cur = []
    return cs


def dentro(cs, x, y):
    w = 0
    for c in cs:
        n = len(c)
        for i in range(n):
            (x0, y0), (x1, y1) = c[i], c[(i + 1) % n]
            if y0 <= y < y1 or y1 <= y < y0:
                xi = x0 + (y - y0) * (x1 - x0) / (y1 - y0)
                if xi > x:
                    w += 1 if y1 > y0 else -1
    return w != 0


def gera(ttf, pix, base, alt):
    """Glifos de 0x20 a 0xFF: (avanco, [linhas]) com a linha de base em 'base'."""
    t = TTFont(ttf)
    cmap, gs, hmtx = t.getBestCmap(), t.getGlyphSet(), t["hmtx"]
    saida = []
    for cp in range(0x20, 0x100):
        nome = cmap.get(cp) or cmap[ord("?")]
        if 0x7F <= cp < 0xA0:
            nome = cmap[ord("?")]
        adv = round(hmtx[nome][0] / pix)
        cs = contornos(gs, nome)
        linhas = []
        for r in range(alt):
            y = (base - r - 0.5) * pix
            b = 0
            for x in range(16):
                if dentro(cs, (x + 0.5) * pix, y):
                    b |= 1 << x
            linhas.append(b)
        saida.append((adv, linhas))
    return saida


def gera_fino(ttf, passo, ox, oy, base, alt):
    """A DotGothic16 no passo dos pontos, proporcional."""
    t = TTFont(ttf)
    cmap, gs = t.getBestCmap(), t.getGlyphSet()
    saida = []
    for cp in range(0x20, 0x100):
        nome = cmap.get(cp) or cmap[ord("?")]
        if 0x7F <= cp < 0xA0:
            nome = cmap[ord("?")]
        cs = contornos(gs, nome)
        linhas = []
        for r in range(alt):
            y = oy + (base - r - 0.5) * passo
            b = 0
            for x in range(16):
                if dentro(cs, ox + (x + 0.5) * passo, y):
                    b |= 1 << x
            linhas.append(b)
        cols = [c for c in range(16) if any(l >> c & 1 for l in linhas)]
        if not cols:
            saida.append((4 if cp == 0x20 else 3, [0] * alt))
            continue
        c0, c1 = min(cols), max(cols)
        saida.append((c1 - c0 + 2, [l >> c0 for l in linhas]))
    return saida


def tabela(f, tipo, nome, glifos, alt):
    f.write("const uint8_t %s_AV[FONTE_N] = {\n" % nome)
    av = [g[0] for g in glifos]
    for i in range(0, len(av), 16):
        f.write("    " + ", ".join(str(a) for a in av[i:i + 16]) + ",\n")
    f.write("};\n\n")
    f.write("const %s %s[FONTE_N][%s_ALT] = {\n" % (tipo, nome, nome))
    for cp, (_, ls) in zip(range(0x20, 0x100), glifos):
        f.write("    {" + ",".join("0x%x" % b for b in ls) + "},   // %02X\n" % cp)
    f.write("};\n\n")


def main():
    jersey = gera(os.path.join(AQUI, "fontes", "Jersey10-Regular.ttf"), 75, 15, 20)
    tiny = gera(os.path.join(AQUI, "fontes", "Tiny5-Regular.ttf"), 128, 9, 12)
    # Na Jersey, a e o ordinais (5ª, 3º) sao iguais as letras normais. Fazemos
    # os de verdade: a letra miuda da Tiny5 la no alto, com um traco embaixo.
    for cp, letra in ((0xAA, "a"), (0xBA, "o")):
        adv, ls = tiny[ord(letra) - 0x20]
        linhas = [0] * 20
        for r in range(12):
            if 0 <= r - 1 < 20:
                linhas[r - 1] |= ls[r]          # a x-altura da Tiny5 cai no alto das maiusculas
        larg = max(b.bit_length() for b in ls)
        linhas[9] = (1 << larg) - 1
        jersey[cp - 0x20] = (larg + 1, linhas)
    fino = gera_fino(os.path.join(AQUI, "fontes", "DotGothic16-Regular.ttf"), 76.3, 28, 0, 15, 20)
    with open(SAIDA_H, "w") as f:
        f.write("""// GERADO POR tools/gen_fonte.py - NAO EDITAR A MAO
// As fontes de pixels do jogo, Latin-1, no grid de 640x360:
//   FONTE  Jersey 10 (texto normal): linha de 20 px, base na linha 15.
//   MIUDA  Tiny5 (texto miudo): linha de 12 px, base na linha 9.
//   FINO   DotGothic16 (texto corrido): linha de 20 px, base na linha 15.
// Cada linha de um glifo e um mapa de bits (bit 0 = coluna 0).
#pragma once
#include <stdint.h>

#define FONTE_ALT    20
#define FONTE_BASE   15
#define MIUDA_ALT    12
#define MIUDA_BASE   9
#define FINO_ALT     20
#define FONTE_PRIM   0x20
#define FONTE_N      (0x100 - FONTE_PRIM)

extern const uint8_t  FONTE_AV[FONTE_N];
extern const uint16_t FONTE[FONTE_N][FONTE_ALT];
extern const uint8_t  MIUDA_AV[FONTE_N];
extern const uint16_t MIUDA[FONTE_N][MIUDA_ALT];
extern const uint8_t  FINO_AV[FONTE_N];
extern const uint16_t FINO[FONTE_N][FINO_ALT];
""")
    with open(SAIDA_C, "w") as f:
        f.write("// GERADO POR tools/gen_fonte.py - NAO EDITAR A MAO\n")
        f.write("// Jersey 10, (c) 2023 The Soft Type Project Authors, SIL OFL 1.1\n")
        f.write("// Tiny5, (c) 2022-2024 The Tiny5 Project Authors, SIL OFL 1.1\n")
        f.write("// DotGothic16, (c) 2020 The DotGothic16 Project Authors, SIL OFL 1.1\n")
        f.write('#include "fonte.h"\n\n')
        tabela(f, "uint16_t", "FONTE", jersey, 20)
        tabela(f, "uint16_t", "MIUDA", tiny, 12)
        tabela(f, "uint16_t", "FINO", fino, 20)
    print("fontes geradas: %d glifos em cada" % len(jersey))


if __name__ == "__main__":
    main()
