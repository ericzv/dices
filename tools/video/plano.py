# As decisoes da demonstracao, iguais no ensaio (ensaio.py) e na gravacao
# (roteiro.py): o jogo e deterministico, entao as mesmas decisoes dao a mesma
# partida. As jogadas da pessoa sao as que a IA dificil faria no lugar dela.
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from motor import *

F = {n: i for i, n in enumerate(
    "F_ORDEM F_APOSTA F_ROLANDO F_ARRUMA F_RESULTADO F_PREMIO F_FIM F_LOJA F_ABERTURA F_JOGA F_VEREDITO "
    "F_EFEITOS F_CATALOGO F_ONLINE F_TUTORIAL F_MAPA F_RLOJA F_RBOLSA F_RFIM F_RAMULETO F_PERFIS F_PCRIA "
    "F_RDIFIC F_REVENTO F_CREDITOS".split())}
FN = {v: k for k, v in F.items()}
# Amuletos e dados faceis de explicar, na ordem de preferencia.
AM_PREF = ["Reserva", "Bolsa Funda", "Luva de Couro", "Rede", "Peso Pena", "Pote Gordo", "Saco Furado", "Dado de Ouro"]
LOJA_PREF = ["Teimoso", "D8", "Sentinela", "D12", "Acumulador", "Desafiante", "Misericordioso", "Solitário"]


def escolhe_amuleto(j):
    nomes = [j.mv_am_nome(j.mv_am_oferta(i)).decode() if j.mv_am_oferta(i) < 255 else "" for i in range(3)]
    for p in AM_PREF:
        if p in nomes: return nomes.index(p), nomes
    return 0, nomes


def escolhe_loja(j):
    nomes = [j.mv_tipo_nome(j.mv_loja_tipo(i)).decode() for i in range(5)]
    for p in LOJA_PREF:
        if p in nomes:
            i = nomes.index(p)
            if j.mv_preco_run(j.mv_loja_tipo(i)) <= j.mv_run_moedas(): return i, nomes
    return -1, nomes


def escolhe_mesa(j):
    """A mesa mais facil entre as que se pode escolher."""
    ops = [(j.mv_no_tipo(j.mv_run_passo(), l), l) for l in range(3) if j.mv_escolhivel(l)]
    ops = [o for o in ops if o[0] <= 2] or ops
    return min(ops)[1]


def ordem_mao(j):
    return j.conselho_fila()


def decide_jogar(j):
    """True: jogar o proximo dado; False: parar."""
    return bool(j.mv_conselho_continua())


def decide_aposta(j):
    """Indice do botao da aposta (passar/apostar ou pagar/dobrar/correr)."""
    return j.mv_conselho_aposta()
