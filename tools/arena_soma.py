#!/usr/bin/env python3
"""Soma as linhas PAR de varias execucoes de tests/arena.c (em paralelo, com
sementes diferentes) e da as medias com o intervalo de confianca de 95%.

    python3 tools/arena_soma.py saida1.txt saida2.txt ...

Cada linha PAR e um par de partidas (as mesmas bolsas e a mesma semente, com
as IAs trocando de lugar): partidas vencidas por A (de 2), rodadas vencidas
por A, rodadas jogadas e o saldo de fichas de A nas duas. O intervalo e por
pares, que e a unidade independente.
"""
import math
import sys


def ic(xs):
    n = len(xs)
    m = sum(xs) / n
    v = sum((x - m) ** 2 for x in xs) / (n - 1) if n > 1 else 0.0
    e = 1.96 * math.sqrt(v / n)
    return m, m - e, m + e


def main():
    part, rod, fich, rodadas = [], [], [], 0
    tempos = {}
    for nome in sys.argv[1:]:
        for linha in open(nome):
            t = linha.split()
            if len(t) == 5 and t[0] == "PAR" and linha.endswith("\n"):
                v, ra, rt, f = map(float, t[1:])
                part.append(v / 2)
                rod.append(ra / rt)
                fich.append(f / 2)
                rodadas += int(rt)
            elif len(t) >= 12 and t[0] == "TEMPO":
                ch = (t[1], t[3], t[4])
                n, med, pior = int(t[6]), float(t[8]), float(t[11])
                a = tempos.setdefault(ch, [0, 0.0, 0.0])
                a[0] += n
                a[1] += n * med
                a[2] = max(a[2], pior)
    if not part:
        print("nenhuma linha PAR")
        return
    print("%d pares, %d partidas, %d rodadas" % (len(part), 2 * len(part), rodadas))
    for nome, xs, esc, un in (("partidas vencidas por A", part, 100, "%"), ("rodadas vencidas por A", rod, 100, "%"),
                              ("saldo de fichas de A por partida", fich, 1, "")):
        m, a, b = ic(xs)
        print("%-34s %7.2f%s  IC95 [%7.2f, %7.2f]" % (nome, m * esc, un, a * esc, b * esc))
    for (que, niv, fase), (n, s, pior) in sorted(tempos.items()):
        print("TEMPO %-7s nivel %s %-7s n %8d  medio %7.2f ms  pior %7.2f ms" % (que, niv, fase, n, s / n, pior))


if __name__ == "__main__":
    main()
