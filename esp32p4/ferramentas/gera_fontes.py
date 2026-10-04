#!/usr/bin/env python3
"""Embute as fontes do console (Inter, licenca OFL) num .c.

As fontes em ferramentas/fontes ja sao recortes do Inter (instancias fixas
do Inter variavel, so com Latin-1 e alguns sinais), feitos assim:

    python3 -m fontTools.varLib.instancer Inter.ttf wght=700 opsz=24 -o x.ttf
    pyftsubset x.ttf --unicodes="U+0020-007E,U+00A0-00FF,U+2013,U+2014,..." \
        --no-hinting --layout-features='' --drop-tables+=GPOS,GSUB,GDEF,DSIG,STAT

Saida: console/fontes_ttf.c (os tres pesos). O texto do jogo usa a Jersey 10
de desktop/fonte_ttf.h, a mesma do PC.

Uso:  python3 esp32p4/ferramentas/gera_fontes.py
"""
import os

AQUI = os.path.dirname(os.path.abspath(__file__))
FONTES = [("INTER", "Inter-Regular.ttf"), ("INTER_NEGRITO", "Inter-Bold.ttf"),
          ("INTER_PRETO", "Inter-Black.ttf")]


def main():
    saida = os.path.join(AQUI, "..", "console", "fontes_ttf.c")
    with open(saida, "w") as f:
        f.write("// GERADO POR esp32p4/ferramentas/gera_fontes.py - NAO EDITAR A MAO\n")
        f.write("// Inter, (c) 2020 The Inter Project Authors, SIL OFL 1.1\n")
        f.write("// (ferramentas/fontes/OFL-Inter.txt)\n")
        f.write('#include "fontes_ttf.h"\n')
        for nome, arq in FONTES:
            dados = open(os.path.join(AQUI, "fontes", arq), "rb").read()
            f.write("\nconst unsigned char FONTE_%s[%d] = {\n" % (nome, len(dados)))
            for i in range(0, len(dados), 20):
                f.write("    " + ",".join(str(b) for b in dados[i:i + 20]) + ",\n")
            f.write("};\n")
            f.write("const unsigned int FONTE_%s_TAM = %d;\n" % (nome, len(dados)))
    print("fontes do console geradas em", os.path.relpath(saida))


if __name__ == "__main__":
    main()
