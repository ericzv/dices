#!/usr/bin/env python3
"""Gera a fonte do texto nitido (Jersey 10, licenca OFL, em tools/fontes).

O texto do PC e desenhado na resolucao real da tela, por cima da mesa de
pixels. O jogo precisa saber a largura de cada letra para centralizar e
quebrar linhas; a plataforma precisa do arquivo da fonte para desenhar.

Saidas:
  src/ui/fonte_tela.c   avanco de cada letra Latin-1, em 1/64 de pixel do jogo
  desktop/fonte_ttf.h   o .ttf inteiro, embutido no executavel

Uso:  python3 tools/gen_fonte_tela.py      (precisa de fonttools)
"""
import os
from fontTools.ttLib import TTFont

AQUI = os.path.dirname(os.path.abspath(__file__))
TTF = os.path.join(AQUI, "fontes", "Jersey10-Regular.ttf")
EM = 20            # tamanho da fonte, em pixels do jogo (linha de 20 px)


def main():
    t = TTFont(TTF)
    upem = t["head"].unitsPerEm
    cmap = t.getBestCmap()
    hmtx = t["hmtx"]
    subst = cmap[ord("?")]
    avancos = []
    for cp in range(0x20, 0x100):
        g = cmap.get(cp, subst)
        avancos.append(round(hmtx[g][0] * EM * 64 / upem))
    asc = round(t["hhea"].ascent * EM * 64 / upem)

    with open(os.path.join(AQUI, "..", "src", "ui", "fonte_tela.c"), "w") as f:
        f.write("// GERADO POR tools/gen_fonte_tela.py - NAO EDITAR A MAO\n")
        f.write("// Jersey 10, (c) 2023 The Soft Type Project Authors, SIL OFL 1.1\n")
        f.write('#include "fonte_tela.h"\n\n')
        f.write("const uint16_t FT_AVANCO[FONTE_N] = {\n")
        for i in range(0, len(avancos), 16):
            f.write("    " + ", ".join(str(a) for a in avancos[i:i + 16]) + ",\n")
        f.write("};\n")
    with open(os.path.join(AQUI, "..", "src", "ui", "fonte_tela.h"), "w") as f:
        f.write("// GERADO POR tools/gen_fonte_tela.py - NAO EDITAR A MAO\n")
        f.write("// Metricas da fonte do texto nitido (Jersey 10), em pixels do jogo.\n")
        f.write("#pragma once\n#include <stdint.h>\n#include \"fonte.h\"\n\n")
        f.write("#define FT_EM   %d       // tamanho da fonte em pixels do jogo\n" % EM)
        f.write("#define FT_ASC  %d      // ascendente, em 1/64 de pixel\n\n" % asc)
        f.write("extern const uint16_t FT_AVANCO[FONTE_N];   // 1/64 de pixel do jogo\n")

    dados = open(TTF, "rb").read()
    with open(os.path.join(AQUI, "..", "desktop", "fonte_ttf.h"), "w") as f:
        f.write("// GERADO POR tools/gen_fonte_tela.py - NAO EDITAR A MAO\n")
        f.write("// Jersey 10, (c) 2023 The Soft Type Project Authors, SIL OFL 1.1\n")
        f.write("#pragma once\n\n")
        f.write("static const unsigned char FONTE_TTF[%d] = {\n" % len(dados))
        for i in range(0, len(dados), 20):
            f.write("    " + ",".join(str(b) for b in dados[i:i + 20]) + ",\n")
        f.write("};\n")
    print("fonte do texto gerada: %d glifos, ttf de %d bytes" % (len(avancos), len(dados)))


if __name__ == "__main__":
    main()
