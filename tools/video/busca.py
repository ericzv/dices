# Procura sementes de run boas para a demonstracao: roda o ensaio em paralelo
# (um processo novo por semente, porque o estado do jogo e global) e resume
# cada uma. A escolhida vai para SEMENTE no roteiro.py.
#
#   python3 busca.py 0 200
import os, sys, json, multiprocessing as mp
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def semente_de(s): return s * 7919 + 13


def um(s):
    from ensaio import demo
    try:
        r = demo(semente_de(s))
    except Exception as e:
        return dict(s=s, erro=str(e))
    r.pop("j"); r.pop("c")
    r["s"] = s
    return r


if __name__ == "__main__":
    from plano import AM_PREF
    a, b = int(sys.argv[1]), int(sys.argv[2])
    with mp.get_context("fork").Pool(os.cpu_count(), maxtasksperchild=1) as pool:
        res = pool.map(um, range(a, b), chunksize=1)
    json.dump(res, open(f"busca_{a}.json", "w"), ensure_ascii=False)
    for r in res:
        if "erro" in r: print(r["s"], "ERRO", r["erro"]); continue
        am = [x for x in r["amuleto"] if x in AM_PREF[:5]]
        r1 = r["rod"][0] if r["rod"] else {}
        queda1 = any(2 in e[1] for e in r1.get("eu", []))
        print(f"semente {r['semente']}: venceu {r['venc'] == 0}, oponente {r['oponente']}, amuletos {am}, "
              f"loja {r['loja'][r['comprou']] if r['comprou'] >= 0 else '-'}, {len(r['rod'])} rodadas, "
              f"1a rodada: abre {r1.get('primeiro')}, vence {r1.get('venc')}, queda {queda1}")
