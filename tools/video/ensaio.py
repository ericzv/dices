# Ensaio rapido de uma run de demonstracao, sem video: cria o perfil, escolhe
# amuleto, loja e mesa, e joga a primeira partida com o plano. Serve para
# achar uma semente boa (busca.py) e conferir o que acontece em cada rodada.
#
#   python3 ensaio.py 388872
import os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from plano import *


class Ensaio:
    def __init__(self, j):
        self.j = j
        self.log = []                    # (rodada, o que a maquina anunciou)
        self.ult = b""

    def passo(self, n=1):
        for _ in range(n):
            self.j.passo()
            a = self.j.mv_anuncio_txt()
            if a != self.ult and self.j.mv_anuncio():
                self.ult = a
                self.log.append((self.j.mv_rodada(), a.decode()))

    def clica(self, var=None, val=None, tecla=None, relogio=None):
        j = self.j
        j.desenha_so()
        for z in j.zonas():
            if (var is None or z[6] == var) and (val is None or z[4] == val) and (tecla is None or z[5] == tecla):
                x, y = z[0] + z[2] // 2, z[1] + z[3] // 2
                j.mouse(x, y, 0)
                j.passo()
                if relogio is not None: j.mv_relogio(relogio)    # a semente da run
                j.mouse(x, y, 1)
                return
        raise RuntimeError(f"zona nao achada {var} {val} {tecla} fase {FN[j.mv_fase()]} {j.zonas()}")

    def ate(self, pred, maxs=120):
        for _ in range(int(maxs * 60)):
            self.passo()
            if pred(): return
        raise RuntimeError(f"esperei demais (fase {FN[self.j.mv_fase()]})")


def joga_partida(c, j, rodadas):
    """Joga a partida inteira com o plano. Devolve o vencedor."""
    ultima = -1
    while True:
        f = j.mv_fase()
        if f == F["F_FIM"]:
            return j.mv_venc_partida()
        if j.mv_ia_na_vez() or j.mv_anuncio() or j.mv_virada():
            c.passo()
            continue
        if f == F["F_ORDEM"] and j.mv_vez() == 0 and j.mv_n_mao(0) and j.mv_t_fase() > 2.5:
            if j.mv_rodada() != ultima:
                ultima = j.mv_rodada()
                rodadas.append(dict(r=ultima, primeiro=j.mv_primeiro(), fichas=(j.mv_fichas(0), j.mv_fichas(1)),
                                    mao=[j.mv_mao_nome(0, i).decode() for i in range(j.mv_n_mao(0))], eu=[]))
            if j.mv_fila_n(0) == 0:
                for i in ordem_mao(j): c.clica(1, i)
            c.clica(tecla=KEY_TAB)
        elif f == F["F_JOGA"] and j.mv_vez() == 0 and j.mv_t_fase() > 0.8:
            if decide_jogar(j):
                c.clica(1, 0)
                c.ate(lambda: j.mv_fase() not in (F["F_ROLANDO"], F["F_ARRUMA"], F["F_VEREDITO"]), 30)
                n = j.mv_fila_lancados(0)
                rodadas[-1]["eu"].append(([j.mv_fila_valor(0, k) for k in range(n)],
                                          [j.mv_fila_est(0, k) for k in range(n)], j.mv_fila_pontos(0)))
            else:
                c.clica(1, 1)
        elif f == F["F_APOSTA"] and j.mv_vez() == 0 and j.mv_t_fase() > 0.8:
            c.clica(1, decide_aposta(j))
        elif f == F["F_RESULTADO"] and j.mv_t_fase() > 2.0:
            rodadas[-1].update(venc=j.mv_venc_mao(), pts=(j.mv_fila_total(0), j.mv_fila_total(1)),
                               depois=(j.mv_fichas(0), j.mv_fichas(1)))
            j.mouse(320, 100, 1)
        else:
            c.passo()


def demo(semente):
    j = Jogo()
    j.mv_salva_limpa()
    j.inicia(0, 987654321)
    c = Ensaio(j)
    c.ate(lambda: j.mv_pronta() and j.mv_t_fase() > 1.0)
    c.clica(2, 0); c.passo(20)                     # 1 jogador
    c.clica(2, 1); c.passo(20)                     # Modo Desafiante
    c.clica(3, 0); c.passo(20)                     # novo perfil
    c.clica(tecla=KEY_RIGHT); c.passo(10)
    for ch in "PAI": j.tecla(ch); c.passo(5)
    c.clica(tecla=KEY_ENTER, relogio=semente)      # criar: a run nasce aqui
    c.ate(lambda: j.mv_fase() == F["F_RAMULETO"] and not j.mv_elev() and j.mv_t_fase() > 0.5)
    ia, nomes_am = escolhe_amuleto(j)
    c.clica(1, ia); c.passo(30)
    il, nomes_loja = escolhe_loja(j)
    if il >= 0: c.clica(1, il); c.passo(20)
    c.clica(tecla=KEY_TAB); c.passo(30)            # sai da loja
    l = escolhe_mesa(j)
    oponente = (j.mv_no_tipo(0, l), j.mv_perso_nome(j.mv_no_perso(0, l)).decode())
    c.clica(4, l); c.passo(5)
    rodadas = []
    venc = joga_partida(c, j, rodadas)
    return dict(semente=semente, amuleto=nomes_am, loja=nomes_loja, comprou=il, oponente=oponente, venc=venc,
                fichas=(j.mv_fichas(0), j.mv_fichas(1)), rod=rodadas, anuncios=c.log, j=j, c=c)


if __name__ == "__main__":
    t = time.time()
    r = demo(int(sys.argv[1]) if len(sys.argv) > 1 else 388872)
    print(f"{time.time() - t:.1f} s")
    for k, v in r.items():
        if k not in ("rod", "j", "c", "anuncios"): print(k, v)
    for x in r["rod"]: print(x)
    for a in r["anuncios"]: print("   ", a)
