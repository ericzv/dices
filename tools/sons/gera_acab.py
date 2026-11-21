#!/usr/bin/env python3
"""Os sons dos acabamentos (acabamento.inc): o pouso, a batida na borda de
madeira, a batida em outro dado e o dado rolando, de cada acabamento (o
MARFIM fica com os de sempre, do gera.py), e os sons das assinaturas. Mesmas
gravacoes do gera.py: dado no feltro, na bandeja e na mesa de madeira e no
metal; dado batendo em dado de plastico, de madeira e de metal; e notas do
FluidR3.

  python3 tools/sons/gera_acab.py [--saida DIR] [classe|som ...]

Grava, de cada classe, pousa_<classe>_<v>.ogg, bate_<classe>_<v>.ogg e
bate_dado_<classe>_<v>.ogg (5 variantes de cada, no volume do pousa e do bate
de sempre, com o ajuste da classe em AJUSTE) e rola_<classe>.ogg (o laco do
dado rolando, como o rufo, no volume dele no primeiro 1,1 s - o que se ouve
num lance -, com o ajuste em AJUSTE_ROLA; a laca e a noite rolam com o rufo
de sempre); e os das assinaturas: rola_furia, rola_onda, rola_arrasta,
pousa_furia_<v>, pousa_tremor_<v>, sopro, treno_sino e as_* (as assinaturas
novas; as_espinho_<v>, as_tambor_<v> e as_passo_<v> com variantes); e as
moedas de metal: moeda_pousa_metal_<v>, moeda_bate_metal_<v> e
rola_moeda_metal.

O que cada um quer parecer: a pedra pesada e aspera; o metal grave no feltro
(sem o chiado de metal com metal) e, rolando, metal mesmo; a madeira
pousando como madeira com madeira e rolando com o toc oco; o macio abafado; a
borracha grave; a louca pousando como duas bolinhas de gude se tocando (o
tique breve, sem o tinido longo) e rolando como bolinha de gude; o gelo deslizando; o biscoito seco, com migalhas; o papel amassando; os
graos chacoalhando. A Furia cai pesada (o estrondo, a bigorna e o chao
roncando; o grave tem harmonicos para aparecer nas caixinhas pequenas) e rola
pesada; o
Terremoto pousa com um impacto mais longo; a Ventania sopra um fluuush no
meio do caminho; o Treno toca o guizo no caminho; a Onda rola com agua; a
Ancora arrasta no feltro; o Ourico estala os espinhos; o Revolver gira o
tambor e trava; a Viuva Negra anda nas patinhas, baixinho.
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gera as G
import numpy as np

SR = G.SR
def db(x): return 10 ** (x / 20)
def rnd(): return G.rnd                      # o sorteio do som da vez (semeia)
FELTRO = ['surface_felt%d' % i for i in (1, 2, 3, 4, 5)]
MADEIRA = ['surface_wood_tray%d' % i for i in (1, 2, 4, 5)]
MESA = ['surface_wood_table%d' % i for i in (1, 2, 3, 4, 5, 6, 7)]
PLASTICO = ['dicehit_plastic%d' % i for i in (1, 2, 4, 5, 6, 13, 14, 15)]
DMADEIRA = ['dicehit_wood%d' % i for i in (1, 2, 3, 4, 5, 6, 7, 8)]
DMETAL = ['dicehit_metal%d' % i for i in (1, 2, 3, 4, 6, 8, 10, 11)]
SMETAL = ['surface_metal%d' % i for i in (1, 2, 3, 4, 5, 6)]
def f(v): return G.superficie(FELTRO[v % len(FELTRO)])
def m(v): return G.superficie(MADEIRA[v % len(MADEIRA)])
def mesa(v): return G.superficie(MESA[v % len(MESA)])
def pl(v): return G.batida(PLASTICO[v % len(PLASTICO)])
def dm(v): return G.batida(DMADEIRA[v % len(DMADEIRA)])
def mt(v): return G.batida(DMETAL[v % len(DMETAL)])
def sm(v): return G.superficie(SMETAL[v % len(SMETAL)])
def corta(x, dur):
    n = int(dur * SR)
    x = x[:n].copy()
    r = min(int(0.012 * SR), len(x))
    x[len(x) - r:] *= np.linspace(1, 0, r)
    return x
NOTA = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
def nota(inst, nome, dur):
    midi = 12 * (int(nome[-1]) + 1) + NOTA[nome[0]]
    x = G.nota(inst, midi, dur, solta=0.05)
    return x * G.env(len(x), 0.001, dur * 0.6)
AFINA = [-2, -1, 0, 1, 2]           # a afinacao das notas, uma por variante (em semitons)
def nota_v(inst, nome, dur, v):
    return G.tom(nota(inst, nome, dur), AFINA[v % 5])
def parcial(fr, dur, queda):
    """Um parcial puro (o tinido do vidro), com ataque curto."""
    n = int(dur * SR); t = np.arange(n) / SR
    return np.sin(2 * np.pi * fr * t) * G.env(n, 0.0008, queda)
def mix(*camadas, dur=0.6):
    M = G.Mix(dur)
    for t, x, g in camadas: M.poe(t, x, g)
    return M.x
def unit(x):
    """Normaliza pelo desvio (para somar ruidos filtrados numa escala conhecida)."""
    s = np.std(x)
    return x / s if s > 0 else x

# --- Pedacos ---------------------------------------------------------------
def grao_de(dur, corte, ganho=1.0):
    """Um estalo miudo (graozinho de areia, migalha, semente batendo)."""
    return G.estalinho(dur, corte) * ganho
def amassa(dur):
    """O papel amassando: um chiado claro com estalinhos rapidos dentro."""
    n = int(dur * SR)
    e = np.zeros(n)
    for _ in range(rnd().randint(4, 9)):
        p = rnd().randrange(max(1, n - 40)); L = rnd().randint(40, 260)
        k = min(L, n - p)
        e[p:p + k] += np.exp(-np.arange(k) / (L / 3)) * rnd().uniform(0.4, 1.0)
    return G.banda(G.rng.standard_normal(n), 1200, 7000) * e
def semente():
    """Uma semente batendo: a batida de plastico bem aguda e curtinha."""
    x = G.pb(G.pa(G.tom(G.batida(rnd().choice(PLASTICO)), rnd().uniform(8, 13)), 1500), 9000)
    return corta(x, 0.012)
def chocalho(M, t0, n, g0, larg=0.05):
    """Varias sementes batendo juntas (o chocalho), a partir de t0."""
    for k in range(n):
        M.poe(t0 + rnd().uniform(0, larg) * (1 + k / n), semente(), g0 * rnd().uniform(0.4, 1.0) * (1 - 0.5 * k / n))
def boom(f0, f1, dur, queda):
    """O estrondo de algo pesado: um seno que despenca de f0 a f1, saturado
    (os harmonicos fazem o grave aparecer nas caixinhas pequenas)."""
    n = int(dur * SR); t = np.arange(n) / SR
    fr = f1 + (f0 - f1) * np.exp(-t / (queda * 0.5))
    x = np.sin(2 * np.pi * np.cumsum(fr) / SR) * G.env(n, 0.003, queda)
    return G.pb(np.tanh(3 * x) / np.tanh(3), 1200)
def ronco(dur, corte, ataque, queda):
    """O chao roncando: ruido bem grave que sobe e se vai."""
    n = int(dur * SR)
    return unit(G.pb(G.rng.standard_normal(n), corte, 4)) * G.env(n, ataque, queda)
def cascalho(M, t0, t1, n, g0):
    """Pedrinhas assentando depois do impacto, cada vez mais fracas."""
    for k in range(n):
        t = t0 + (t1 - t0) * (k / max(1, n - 1)) ** 1.3 + rnd().uniform(0, 0.02)
        M.poe(t, grao_de(rnd().uniform(0.0008, 0.0025), 1500), g0 * rnd().uniform(0.5, 1.0) * (1 - 0.6 * k / n))

# --- O pouso, a batida na borda e em outro dado ----------------------------
# classe -> (pousa, bate, bate_dado), cada um uma funcao da variante v
def pousa_grao(v):
    M = G.Mix(0.5)
    M.poe(0, f(v), 1)
    chocalho(M, 0.004, 9, db(-12), 0.12)
    return M.x
def bate_grao(v):
    M = G.Mix(0.4)
    M.poe(0, G.tom(m(v), 2), 1)
    chocalho(M, 0.003, 5, db(-14), 0.06)
    return M.x
def bate_dado_grao(v):
    M = G.Mix(0.4)
    M.poe(0, G.tom(pl(v), 3), 1)
    chocalho(M, 0.003, 4, db(-14), 0.05)
    return M.x
def pousa_louca(v):
    """A louca (e a perola) pousando: duas bolinhas de gude se tocando -- o
    tique breve e abafado, vidro com vidro, e um tiquinho de volta -- com o
    feltro bem baixo por baixo (sem o tinido longo nem o agudo)."""
    def clique(k, st):
        return corta(G.pb(G.pa(G.tom(pl(k), st), 1800), 6000, 4), 0.022)
    M = G.Mix(0.3)
    M.poe(0, clique(v, 6 + v % 3), 1.0)
    M.poe(0.026 + 0.007 * (v % 3), clique(v + 3, 8), 0.32)
    M.poe(0, G.pb(f(v), 900), db(-14))
    return M.x
R = {
 'laca': (lambda v: mix((0, G.tom(f(v), 2), 1), (0, G.pa(pl(v), 2000), db(-14))),
          lambda v: G.pa(G.tom(m(v), 2), 300),
          lambda v: corta(G.tom(pl(v), 3), 0.06)),
 'vidro': (lambda v: mix((0, G.pb(f(v), 3000), 1), (0.002, parcial(3200 * 2 ** (AFINA[v % 5] / 12), 0.1, 0.03), db(-14))),
           lambda v: mix((0, G.pa(m(v), 500), 1), (0, G.tom(mt(v), 7), db(-10))),
           lambda v: G.pb(mix((0, G.pa(G.tom(mt(v), 9), 1500), 1), (0.003, nota_v('celesta', 'G6', 0.05, v), db(-36))), 9000)),
 'gelo': (lambda v: G.pa(f(v), 1000),
          lambda v: mix((0, G.pa(G.tom(m(v), 5), 800), 1), (0, G.tom(pl(v), 7), db(-8))),
          lambda v: mix((0, G.tom(pl(v), 8), 1), (0.003, nota_v('glockenspiel', 'E7', 0.04, v), db(-36)))),
 'pedra': (lambda v: mix((0, G.pb(G.tom(f(v), -4), 1800), 1), (0, G.pb(G.tom(m(v), -7), 900), db(-10))),
           lambda v: G.tom(m(v), -5),
           lambda v: G.pb(mix((0, G.tom(dm(v), -8), 1), (0, G.pb(G.tom(dm(v), -16), 400), 1)), 1200, 4)),
 # o metal no feltro: o baque grave e cheio (o feltro abaixado) e o corpo do
 # metal bem abaixo, abafado (sem o chiado de metal com metal)
 'metal': (lambda v: mix((0, G.pb(G.tom(f(v), -4), 2800), 1), (0.002, corta(G.banda(G.tom(mt(v), -7), 500, 2200), 0.09), db(-5))),
           lambda v: mix((0, G.tom(m(v), -2), 1), (0, mt(v), db(-8))),
           lambda v: G.pb(G.tom(mt(v), [-1.5, 0, 1.5, -0.7, 0.7][v % 5]), 7000)),
 # madeira com madeira: o toc de dois dados de madeira, o corpo grave e o feltro baixinho
 'madeira': (lambda v: G.pb(mix((0, dm(v), 1), (0.001, G.pb(G.tom(dm(v + 3), -7), 1400), db(-5)),
                                 (0.001, G.banda(G.tom(dm(v + 1), -5), 400, 1200), 1), (0, f(v), db(-14))), 6000),
             lambda v: G.tom(m(v), 3),
             lambda v: dm(v)),
 'macio': (lambda v: G.pb(G.tom(f(v), -5), 900),
           lambda v: G.pb(G.tom(m(v), -4), 700),
           lambda v: G.pb(G.tom(dm(v), -6), 800)),
 # o biscoito: seco e curto, com as migalhas
 'seco': (lambda v: mix((0, corta(G.pa(G.tom(f(v), 4), 600), 0.05), 1), (0.004, grao_de(0.0012, 2500), db(-16)), (0.019, grao_de(0.0008, 2500), db(-20))),
          lambda v: corta(G.pa(G.tom(m(v), 6), 700), 0.045),
          lambda v: corta(G.pb(G.tom(pl(v), 5), 3000), 0.04)),
 # a louca (e a perola): pousa como bolinha de gude; bate firme, sem o tinido
 'louca': (pousa_louca,
           lambda v: mix((0, G.tom(m(v), 4), 1), (0, corta(G.pa(G.tom(pl(v), 7), 1500), 0.04), db(-12))),
           lambda v: G.pb(mix((0, G.tom(pl(v), 7), 1), (0, G.tom(mt(v), 5), db(-18))), 7000)),
 'borracha': (lambda v: mix((0, G.pb(G.tom(f(v), -3), 1200), 1), (0.005, G.pb(nota('marimba', 'C3', 0.09), 800), db(-8))),
              lambda v: mix((0, G.pb(nota('marimba', 'E3', 0.09), 1000), 1), (0, G.pb(m(v), 600), db(-12))),
              lambda v: mix((0, nota('marimba', 'D3', 0.08), db(-10)), (0, G.pb(G.tom(pl(v), -5), 1500), db(-10)))),
 'noite': (lambda v: mix((0, G.pb(f(v), 2500), db(-6)), (0.006, nota('celesta', ['C6', 'D6', 'E6', 'G6', 'A6'][v % 5], 0.25), db(-8)), dur=0.9),
           lambda v: mix((0, G.pb(m(v), 2000), db(-6)), (0.004, nota('music_box', 'G6', 0.2), db(-16)), dur=0.8),
           lambda v: mix((0, G.pb(pl(v), 3000), 1), (0.004, nota_v('vibraphone', 'C6', 0.08, v), db(-32)), dur=0.8)),
 # o papel: o feltro leve e o papel amassando
 'papel': (lambda v: mix((0, G.pb(G.tom(f(v), 2), 2500), db(-2)), (0.001, amassa(0.07), db(-9))),
           lambda v: mix((0, G.pa(G.tom(m(v), 4), 600), db(-2)), (0.001, amassa(0.045), db(-11))),
           lambda v: mix((0, corta(G.pb(G.tom(pl(v), 3), 2500), 0.05), db(-2)), (0.001, amassa(0.045), db(-11)))),
 # os graos: o feltro e as sementes chacoalhando
 'grao': (pousa_grao, bate_grao, bate_dado_grao),
}
# o volume de cada classe em relacao ao pousa/bate de sempre
AJUSTE = {'macio': -3, 'seco': -4, 'noite': -2, 'pedra': 1, 'borracha': -1, 'metal': -2, 'papel': -4, 'grao': -3}
AJUSTE_POUSA = {'louca': -3}      # so o pouso: o tique claro soa mais alto que um baque no mesmo volume medido

# --- O dado rolando (os lacos, como o rufo) --------------------------------
ROLA_T = 4.0              # o laco (o rufo de sempre tem 8 s; um lance dura de 1 a 2)
def rolagem(grao, taxa=(0.045, 0.085), cresce=0.5, piso=0.6, cama=None, sala=0.10):
    """O dado rolando: um impacto (grao(M, t, c)) a cada 'taxa' segundos,
    crescendo de 'piso' ate o cheio em 'cresce' s, como o rufo; 'cama' e um
    som continuo por baixo (o gelo deslizando, a bolinha de gude, a agua)."""
    M = G.Mix(ROLA_T + 1.0)
    t = 0.0
    while t < ROLA_T:
        grao(M, t, min(1.0, piso + (1 - piso) * t / cresce))
        t += rnd().uniform(*taxa)
    x = G.loop(M, ROLA_T)
    if cama is not None:
        n = len(x); tt = np.arange(n) / SR
        c = cama(n)
        r = int(0.05 * SR)
        c[:r] *= np.linspace(0, 1, r); c[-r:] *= np.linspace(1, 0, r)
        x = x + c * np.minimum(1.0, piso + (1 - piso) * tt / cresce)
    return G.sala(x, sala)[:int(ROLA_T * SR)]
def feltro_r(semi=(-3, 3)):
    return G.tom(G.superficie(rnd().choice(FELTRO)), rnd().uniform(*semi))
def oscila(n, f_lo, f_hi, prof):
    """Um balanco lento e irregular (0..1), para as camas."""
    k = max(2, int(n / SR * rnd().uniform(f_lo, f_hi)) + 2)
    pts = np.array([rnd().uniform(1 - prof, 1) for _ in range(k)])
    return np.interp(np.arange(n), np.linspace(0, n, k), pts)

def rola_vidro():
    def g(M, t, c):
        M.poe(t, G.pb(feltro_r((-2, 3)), 4500), c * rnd().uniform(0.35, 0.8))
        if rnd().random() < 0.3:                       # a quina do vidro: um tique curtinho
            M.poe(t + rnd().uniform(0.004, 0.02), corta(G.pa(G.tom(G.batida(rnd().choice(DMETAL)), rnd().uniform(6, 9)), 3000), 0.025),
                  c * rnd().uniform(0.06, 0.12) * db(8))
    return rolagem(g)
def rola_gelo():
    def g(M, t, c):                                    # o gelo quase nao vira: poucos toques, leves
        M.poe(t, G.pa(feltro_r((1, 3)), 400), c * rnd().uniform(0.25, 0.5))
        if rnd().random() < 0.12:
            M.poe(t + rnd().uniform(0.003, 0.02), corta(G.pa(G.tom(G.batida(rnd().choice(PLASTICO)), rnd().uniform(9, 12)), 2500), 0.02),
                  c * rnd().uniform(0.04, 0.08))
    def cama(n):                                       # e desliza: o chiado do gelo no feltro, discreto
        return unit(G.banda(G.rng.standard_normal(n), 1400, 3200)) * oscila(n, 1.5, 4, 0.6) * 0.0035
    return rolagem(g, taxa=(0.08, 0.15), cama=cama)
def rola_pedra():
    def g(M, t, c):                                    # baque grave, o corpo da pedra e a aspereza
        M.poe(t, G.pb(feltro_r((-9, -6)), 1000), c * rnd().uniform(0.5, 0.9))
        if rnd().random() < 0.45:
            M.poe(t + 0.002, G.pb(G.tom(G.batida(rnd().choice(DMADEIRA)), rnd().uniform(-11, -8)), 900), c * 0.6)
        for _ in range(rnd().randint(2, 5)):
            M.poe(t + rnd().uniform(0.002, 0.03), G.pb(grao_de(rnd().uniform(0.0008, 0.002), 1200), 4000), c * rnd().uniform(0.05, 0.13))
    return rolagem(g, taxa=(0.08, 0.14))
def rola_metal():
    def g(M, t, c):                                    # o baque pesado e, as vezes, o metal abafado
        M.poe(t, G.pb(feltro_r((-4, -1)), 3000), c * rnd().uniform(0.45, 0.85) * 0.38)
        if rnd().random() < 0.8:
            M.poe(t + 0.003, G.banda(G.tom(G.batida(rnd().choice(DMETAL)), rnd().uniform(-2, 1)), 700, 3500), c * rnd().uniform(0.9, 1.4))
    return rolagem(g)
def rola_madeira():
    def g(M, t, c):                                    # o feltro e o toc oco da madeira
        M.poe(t, feltro_r((-1, 2)), c * rnd().uniform(0.15, 0.3))
        if rnd().random() < 0.9:
            M.poe(t + 0.002, G.banda(G.tom(G.batida(rnd().choice(DMADEIRA)), rnd().uniform(-2.5, 2.5)), 300, 2500), c * rnd().uniform(0.6, 0.9))
    return rolagem(g)
def rola_macio():
    def g(M, t, c):                                    # abafado: o feltro baixo e fofo, sem estalo
        M.poe(t, G.pb(feltro_r((-6, -3)), 700), c * rnd().uniform(0.35, 0.7))
        if rnd().random() < 0.3:
            n = int(0.03 * SR)
            M.poe(t, G.banda(G.rng.standard_normal(n), 200, 900) * G.env(n, 0.004, 0.01), c * 0.05)
    return rolagem(g, taxa=(0.07, 0.12))
def rola_seco():
    def g(M, t, c):                                    # o biscoito: toques secos e as migalhas
        M.poe(t, corta(G.pa(feltro_r((3, 6)), 600), 0.05), c * rnd().uniform(0.3, 0.6))
        if rnd().random() < 0.45:
            for _ in range(rnd().randint(2, 4)):
                M.poe(t + rnd().uniform(0.005, 0.04), G.pb(grao_de(rnd().uniform(0.0006, 0.0015), 2500), 7000), c * rnd().uniform(0.04, 0.09))
    return rolagem(g, taxa=(0.05, 0.09))
def rola_papel():
    def g(M, t, c):                                    # o papel: toque leve e o amassado
        M.poe(t, G.pb(feltro_r((1, 3)), 3000), c * rnd().uniform(0.25, 0.45))
        M.poe(t + 0.002, G.pb(amassa(rnd().uniform(0.02, 0.05)), 6000), c * rnd().uniform(0.07, 0.14))
    return rolagem(g, taxa=(0.06, 0.10))
def rola_grao():
    def g(M, t, c):                                    # as sementes chacoalhando a cada virada
        M.poe(t, feltro_r((-1, 2)), c * rnd().uniform(0.3, 0.55))
        chocalho(M, t + 0.002, rnd().randint(3, 7), c * 0.1 * db(8), 0.045)
    return rolagem(g, taxa=(0.06, 0.10))
def rola_louca():
    def g(M, t, c):                                    # a bolinha de gude no feltro: o toque abafado e o tique curto, de vidro grosso
        M.poe(t, feltro_r((0, 3)), c * rnd().uniform(0.3, 0.55))
        if rnd().random() < 0.55:
            tq = corta(G.banda(G.tom(G.batida(rnd().choice(PLASTICO)), rnd().uniform(2, 5)), 900, 5000), 0.035)
            M.poe(t + 0.002, tq, c * rnd().uniform(0.3, 0.5))
            if rnd().random() < 0.2:                   # as vezes o tique duplo: as bolinhas se batendo
                M.poe(t + rnd().uniform(0.018, 0.035), tq, c * rnd().uniform(0.12, 0.2))
    return rolagem(g, taxa=(0.05, 0.09))
def rola_borracha():
    def g(M, t, c):                                    # grave e quicando, como o pouso dela
        M.poe(t, G.pb(feltro_r((-7, -4)), 1000), c * rnd().uniform(0.45, 0.8))
        if rnd().random() < 0.5:
            b = G.pb(G.tom(nota('marimba', rnd().choice(['C3', 'D3', 'E3']), 0.07), rnd().uniform(-5, -2)), 700)
            M.poe(t + 0.004, b, c * rnd().uniform(0.10, 0.18))
    return rolagem(g, taxa=(0.06, 0.14))
# as assinaturas
def rola_furia():
    def g(M, t, c):                                    # pesado e rapido: baques fundos e trancos na mesa
        M.poe(t, G.pb(feltro_r((-6, -3)), 1500), c * rnd().uniform(0.6, 1.0))
        if rnd().random() < 0.3:
            M.poe(t + 0.003, G.pb(G.tom(G.superficie(rnd().choice(MESA)), rnd().uniform(-9, -6)), 900), c * rnd().uniform(0.3, 0.45))
    def cama(n):                                       # e o ronco grave por baixo
        r = np.tanh(2 * unit(G.banda(G.rng.standard_normal(n), 90, 350))) / np.tanh(2)
        return r * oscila(n, 3, 7, 0.5) * 0.07
    return rolagem(g, taxa=(0.035, 0.06), cama=cama)
def rola_onda():
    def g(M, t, c):
        M.poe(t, feltro_r(), c * rnd().uniform(0.3, 0.6))
    def cama(n):                                       # a agua: o marulho que vai e vem e umas bolhinhas
        tt = np.arange(n) / SR
        vai = np.sin(np.pi * ((tt / rnd().uniform(1.1, 1.3)) % 1)) ** 2
        x = unit(G.banda(G.rng.standard_normal(n), 300, 2000)) * (0.25 + 0.75 * vai) * 0.03
        t = 0.2
        while t < n / SR - 0.1:
            k = int(t * SR); b = G.bolha(rnd().uniform(700, 1300), rnd().uniform(350, 600), 0.05) * 0.03
            x[k:k + len(b)] += b[:max(0, n - k)]
            t += rnd().uniform(0.3, 0.6)
        return x
    return rolagem(g, cama=cama)
# a moeda de metal (a Moeda e o Tudo ou Nada) girando ate deitar
def rola_arrasta():
    """A Ancora arrastando no feltro (no lugar do dado rolando; o jogo baixa o
    laco conforme ela perde a forca): o raspado aspero e abafado do feltro,
    que prende e solta sem ritmo, e os fios prendendo, miudos e graves."""
    n = int((ROLA_T + 1.0) * SR)
    w = G.rng.standard_normal(n)
    leito = G.pb(0.8 * unit(G.banda(w, 120, 700)) + 0.2 * unit(G.banda(w, 700, 1800)), 1800, 4)
    m = G.pb(G.rng.standard_normal(n), 14)
    leito = leito * (0.62 + 0.38 * m / (np.abs(m).max() + 1e-9))   # prende e solta: a aspereza variando, sem ritmo de motor
    M = G.Mix(ROLA_T + 1.0)
    M.poe(0, leito * 0.05, 1.0)
    t = 0.0
    while t < ROLA_T:                                  # os fios do feltro prendendo: toques curtos, baixos e graves
        M.poe(t, corta(G.pb(G.tom(f(int(t * 97)), rnd().uniform(-9, -5)), 900), 0.05), rnd().uniform(0.08, 0.2))
        t += rnd().uniform(0.03, 0.07)
    M.x = G.sala(M.x, 0.10)[:len(M.x)]                # a sala antes de fechar o laco: a cauda volta para o comeco
    return G.loop(M, ROLA_T)
DMOEDA = ['dicehit_coin%d' % i for i in range(1, 7)]
def cm(v): return G.batida(DMOEDA[v % len(DMOEDA)])
def rola_moeda_metal():
    def g(M, t, c):                                    # a beirada tocando o feltro a cada volta: o tinido abafado
        M.poe(t, G.pb(G.pa(G.tom(cm(rnd().randrange(6)), rnd().uniform(2, 4)), 1800), 6000), c * rnd().uniform(0.10, 0.18))
        if rnd().random() < 0.5:
            M.poe(t, G.pb(feltro_r((2, 5)), 2500), c * 0.12)
    def cama(n):                                       # e o sopro leve que pulsa no giro, como a moeda de sempre
        tt = np.arange(n) / SR
        giro = np.abs(np.sin(2 * np.pi * 7.0 * tt + 0.6 * np.sin(2 * np.pi * 0.4 * tt))) ** 3
        return G.banda(G.rng.standard_normal(n), 600, 2500) * (0.2 + 0.8 * giro) * 0.012
    return rolagem(g, taxa=(0.07, 0.16), cama=cama)
ROLA = {'arrasta': rola_arrasta, 'vidro': rola_vidro, 'gelo': rola_gelo, 'pedra': rola_pedra, 'metal': rola_metal,
        'madeira': rola_madeira, 'macio': rola_macio, 'seco': rola_seco, 'papel': rola_papel, 'grao': rola_grao,
        'louca': rola_louca, 'borracha': rola_borracha,
        'furia': rola_furia, 'onda': rola_onda, 'moeda_metal': rola_moeda_metal}
AJUSTE_ROLA = {'macio': -4, 'seco': -3, 'papel': -4, 'gelo': -6, 'grao': -2, 'furia': 2, 'onda': -1, 'moeda_metal': -2,
               'borracha': -1, 'louca': -2}

# --- Os sons das assinaturas -----------------------------------------------
def pousa_furia(v):
    """A Furia caindo: a pancada seca e grave na mesa, o tampo respondendo, um
    corpo bem grave que assenta logo (sem o seno descendo), o chao roncando e
    as fichas da mesa pulando com o tranco."""
    M = G.Mix(1.4)
    M.poe(0, G.pb(G.tom(mesa(v), -12), 900), 1.0)            # a pancada na madeira da mesa
    M.poe(0, G.pb(G.tom(f(v), -9), 700), 0.8)                # o feltro abafando
    M.poe(0.002, G.pb(G.tom(m(v + 1), -14), 450), 0.6)       # o tampo inteiro respondendo
    n = int(0.55 * SR); t = np.arange(n) / SR
    fr = 64 + 20 * np.exp(-t / 0.03)                         # o corpo: 84 Hz a 64 Hz num instante, e fica
    x = np.sin(2 * np.pi * np.cumsum(fr) / SR) * G.env(n, 0.004, 0.16)
    M.poe(0.002, G.pb(np.tanh(3 * x) / np.tanh(3), 520), 0.22)
    M.poe(0.006, ronco(0.9, 140, 0.01, 0.24), 0.13)
    for k in range(6):                                       # as fichas da mesa pulando e assentando
        tk = 0.06 + 0.045 * k + rnd().uniform(0, 0.02)
        M.poe(tk, G.pa(G.tom(G.batida(rnd().choice(PLASTICO)), rnd().uniform(-6, -3)), 300), 0.16 * (1 - 0.12 * k))
    return G.pa(M.x, 40)
def pousa_tremor(v):
    """O Terremoto pousando: a pedra caindo pesada e o chao tremendo mais um pouco."""
    M = G.Mix(1.1)
    M.poe(0, G.pb(G.tom(f(v), -5), 1600), 1.0)
    M.poe(0, G.pb(G.tom(m(v), -8), 800), db(-6))
    M.poe(0.004, boom(180, 60, 0.6, 0.16), 0.24)
    n = int(0.9 * SR)
    tr = unit(G.banda(G.rng.standard_normal(n), 70, 300)) * G.env(n, 0.02, 0.32)
    tt = np.arange(len(tr)) / SR
    M.poe(0.01, tr * (0.7 + 0.3 * np.sin(2 * np.pi * 9 * tt)), 0.17)
    cascalho(M, 0.06, 0.45, 7, 0.08)
    return M.x
def sopro():
    """A Ventania mudando o caminho: um fluuush leve e curto (nao a ventania)."""
    n = int(0.34 * SR); q = np.linspace(0, 1, n)
    forma = np.sin(np.pi * np.minimum(1, q / 0.4) / 2) ** 2 * np.where(q < 0.4, 1, (1 - (q - 0.4) / 0.6) ** 1.6)
    w = G.rng.standard_normal(n)
    abre = np.clip((q - 0.15) / 0.5, 0, 1)
    M = G.Mix(0.5)
    M.poe(0, forma * (G.banda(w, 400, 1600) * (1 - 0.5 * abre) + G.banda(w, 1400, 4500) * abre * 0.6))
    return G.sala(M.x, 0.12)
def treno_sino():
    """O Treno no meio do caminho: o guizo de treno, uma vez."""
    M = G.Mix(0.6)
    G.guizos(M, 0.0, 9, 0.5, 0.14)
    M.poe(0.002, parcial(2800, 0.3, 0.08), 0.12)              # o corpo do guizo
    M.poe(0.002, parcial(4200, 0.25, 0.07), 0.12 * db(-3))    # e as parciais de cima (guizo, nao bip)
    M.poe(0.002, parcial(5600, 0.2, 0.06), 0.12 * db(-5))
    x = G.pb(M.x, 9000)
    x = x * np.exp(-np.arange(len(x)) / (0.25 * SR) * 2)     # e some em ~0,25 s
    return G.sala(x, 0.15)
# As assinaturas com pouso proprio (o pouso de sempre e o que ela faz depois,
# num arquivo so), o casco da Rena e as asas do Vampiro.
def poe(n, x, t, g=1.0):
    """x somado em t num vetor de n amostras."""
    out = np.zeros(n); k = int(t * SR); m = min(len(x), n - k)
    if m > 0: out[k:k + m] = x[:m] * g
    return out
def as_oco():
    """O Cavalo de Troia: o pouso de madeira e, logo depois, duas batidas abafadas por dentro."""
    M = G.Mix(1.0)
    M.poe(0, R['madeira'][0](1), 1.0)
    for t, g, st in ((0.30, 0.55, -6), (0.45, 0.42, -8)):
        M.poe(t, G.pb(G.tom(dm(5), st), 900), g)
        n = int(0.09 * SR)
        M.poe(t + 0.001, unit(G.banda(G.rng.standard_normal(n), 150, 420)) * G.env(n, 0.001, 0.035) * 0.08, g)
    return G.sala(M.x, 0.22)
def as_eco():
    """O Eco: o pouso de sempre e dois ecos, cada vez mais fracos e mais abafados."""
    x = G.superficie(G.FELTRO[0])
    n = len(x) + int(0.45 * SR)
    y = poe(n, x, 0) + poe(n, G.pb(x, 3000), 0.12, 0.45) + poe(n, G.pb(x, 1800), 0.24, 0.22)
    return G.sala(y, 0.25)
def as_bigorna():
    """O Ferreiro: tres marteladas na bigorna, cada uma mais baixa."""
    M = G.Mix(1.2)
    for k, (st, g) in enumerate(((0, 1.0), (-1, 0.62), (-2, 0.38))):
        t = 0.14 * k
        M.poe(t, G.tom(G.superficie('surface_metal4'), st), 0.8 * g)
        M.poe(t, G.nota('tubular_bells', 84 + st, 0.35, 0.25), 0.4 * g)
        M.poe(t, G.pb(G.tom(f(k), -4), 1800), 0.5 * g)
    return G.sala(M.x, 0.18)
def as_enterra():
    """O Coveiro: a pedra abafada, o timpano fundo (curto e baixo) e um pouco
    de terra assentando -- discreto."""
    M = G.Mix(0.8)
    M.poe(0, G.pb(R['pedra'][0](0), 1200), 0.9)
    M.poe(0.008, G.pb(G.nota('timpani', 36, 0.32, 0.18), 300), 0.45)
    cascalho(M, 0.03, 0.22, 6, 0.05)
    return G.sala(M.x, 0.12)
def as_reta():
    """O Laser: o pouso de metal e uma nota de vibrafone."""
    M = G.Mix(1.2)
    M.poe(0, R['metal'][0](2), 1.0)
    M.poe(0.004, G.nota('vibraphone', 76, 0.7, 0.5), 0.45)
    return G.sala(M.x, 0.2)
def as_atrai():
    """O Buraco Negro: um baque grave e um assobio que cai, como sugado."""
    M = G.Mix(1.0)
    M.poe(0, boom(110, 40, 0.5, 0.14), 0.5)
    n = int(0.4 * SR); t = np.arange(n) / SR
    fr = 200 + 1000 * np.exp(-t / 0.12)
    M.poe(0, np.sin(2 * np.pi * np.cumsum(fr) / SR) * G.env(n, 0.002, 0.15) * 0.12, 1.0)
    M.poe(0, G.pb(f(3), 1500), 0.6)
    return G.sala(M.x, 0.25)
def as_casco():
    """A Rena: o casco no feltro (tloc-tloc)."""
    M = G.Mix(0.3)
    M.poe(0, corta(G.banda(G.tom(dm(2), 5), 500, 3500), 0.05), 1.0)
    M.poe(0.075, corta(G.banda(G.tom(dm(6), 3), 500, 3000), 0.05), 0.7)
    return G.sala(M.x, 0.12)
def as_asas():
    """O Vampiro: tres batidas de asa, curtas."""
    M = G.Mix(0.5)
    for k in range(3):
        n = int(0.06 * SR)
        M.poe(0.075 * k, unit(G.banda(G.rng.standard_normal(n), 250, 1300)) * G.env(n, 0.006, 0.02) * 0.08, 1 - 0.2 * k)
    return G.sala(M.x, 0.1)
def as_espinho(v):
    """Um espinho do Ourico estalando no feltro: um tique seco, miudo."""
    x = G.pb(G.pa(G.tom(G.batida(PLASTICO[v % len(PLASTICO)]), 9 + v % 3), 2200), 9000)
    return corta(x, 0.015)
def as_tambor(v):
    """O tambor do Revolver girando: o clique do metal, curto."""
    x = G.pb(G.pa(G.tom(mt(v + 2), 6 + v), 1500), 9000)
    return G.sala(corta(x, 0.03), 0.05)
def as_trava():
    """O Revolver pousando e travando: o pouso de metal e o estalo duplo do cao."""
    M = G.Mix(0.8)
    M.poe(0, R['metal'][0](1), 0.8)
    M.poe(0.05, G.pb(G.pa(G.tom(mt(4), 4), 1200), 8000), 0.9)
    M.poe(0.085, G.pb(G.pa(G.tom(mt(6), 7), 1500), 8000), 0.7)
    return G.sala(M.x, 0.12)
def as_passo(v):
    """A Viuva Negra andando: as patinhas no feltro -- dois tiquinhos secos,
    quase juntos (tic) -- bem baixinho; o jogo toca um a cada ~0,06 s."""
    M = G.Mix(0.06)
    for k, (t, g) in enumerate(((0, 1.0), (0.006 + 0.002 * (v % 3), 0.55))):
        x = G.pb(G.pa(G.tom(pl(v * 2 + k), 7 + (v + k) % 4), 1800), 6000, 4)
        M.poe(t, corta(x, 0.008), g)
    return M.x
# os de varias variantes: (receita, quantas, volume)
VARIAS = {'as_espinho': (as_espinho, 5, G.VOLUME['bate'] * db(-12)), 'as_tambor': (as_tambor, 3, G.VOLUME['bate'] * db(-9)),
          'as_passo': (as_passo, 5, G.VOLUME['bate'] * db(-16))}
UM = {'sopro': (sopro, G.VOLUME['vento'] * db(0)), 'treno_sino': (treno_sino, G.VOLUME['efeito'] * db(-9)),
      'as_oco': (as_oco, G.VOLUME['pousa'] * db(1)), 'as_eco': (as_eco, G.VOLUME['pousa']),
      'as_bigorna': (as_bigorna, G.VOLUME['pousa'] * db(1)), 'as_enterra': (as_enterra, G.VOLUME['pousa'] * db(-4)),
      'as_reta': (as_reta, G.VOLUME['pousa']), 'as_atrai': (as_atrai, G.VOLUME['pousa']),
      'as_casco': (as_casco, G.VOLUME['bate'] * db(-2)), 'as_asas': (as_asas, G.VOLUME['vento'] * db(-6)),
      'as_trava': (as_trava, G.VOLUME['pousa'] * db(1))}

# A moeda de metal pousando e batendo (como a moeda de sempre, que e ficha, mas de metal).
def moeda_pousa_metal(v):
    """Cai deitada no feltro: o baque macio e o tilintar bambeando, cada vez
    mais rapido e mais fraco, ate parar."""
    M = G.Mix(0.8)
    M.poe(0, G.pb(f(v), 2500), 0.6)
    t, passo, g, k = 0.03, 0.075, 0.22, 0
    while passo > 0.018:                               # o feltro abafa: o tilintar sem o agudo todo
        M.poe(t, G.pb(G.pa(G.tom(cm(v + k), rnd().uniform(-0.5, 0.5)), 700), 5000), g)
        t += passo; passo *= 0.78; g *= 0.82; k += 1
    return M.x
def moeda_bate_metal(v):
    """Bate na borda ou num dado: o tinido curto e um pouco da madeira."""
    M = G.Mix(0.3)
    M.poe(0, G.pb(G.tom(cm(v + 2), rnd().uniform(-1, 1)), 7000), 0.8)
    M.poe(0, G.pb(m(v), 3000), 0.5)                   # a borda de madeira
    return M.x
MOEDA_M = {'moeda_pousa_metal': (moeda_pousa_metal, 'moeda_pousa', 0.10), 'moeda_bate_metal': (moeda_bate_metal, 'moeda_bate', 0.08)}

def rms(x): return float(np.sqrt(np.mean(x ** 2)))
_RUFO = []
def rufo_ref():
    """O volume do rufo de sempre no primeiro 1,1 s (o que se ouve num lance),
    gravado como o gera.py grava: a referencia dos lacos."""
    if not _RUFO:
        G.semeia('rufo')
        _RUFO.append(rms(G.iguala(G.rufo(), G.VOLUME['rufo'])[:int(1.1 * SR)]))
    return _RUFO[0]

def grava(saida, arq, x, alvo, laco=False, acima=0):
    x = x if laco else G.acaba(x, -40)              # a cauda abaixo de -40 dB nao se ouve na mesa
    if laco:                                         # o laco: o volume do primeiro 1,1 s, como o rufo
        x = x * (alvo / max(rms(x[:int(1.1 * SR)]), 1e-9))
        if np.abs(x).max() > 0.85: x = G.limita(x, 0.85)
        p = np.abs(x).max()
        if p > 0.97: x *= 0.97 / p
    elif acima:                                      # o volume medido so acima de 'acima' Hz
        x = x * (alvo / max(G.volume(G.pa(x, acima)), 1e-9))
        if np.abs(x).max() > 0.85: x = G.limita(x, 0.85)
        p = np.abs(x).max()
        if p > 0.97: x *= 0.97 / p
    else:
        x = G.iguala(x, alvo)
    G.grava_ogg(os.path.join(saida, arq + '.ogg'), x, 3 if laco else 4)

def main():
    args = sys.argv[1:]
    saida = os.path.join(G.AQUI, '..', '..', 'desktop', 'sons')
    if '--saida' in args:
        i = args.index('--saida'); saida = args[i + 1]; del args[i:i + 2]
    os.makedirs(saida, exist_ok=True)
    quais = set(args)
    def quer(n): return not quais or n in quais
    for cl, (pousa, bate, bate_dado) in R.items():
        if not quer(cl): continue
        for nome, receita, ref, sala in (('pousa', pousa, 'pousa', 0.10), ('bate', bate, 'bate', 0.08),
                                         ('bate_dado', bate_dado, 'bate', 0.08)):
            for v in range(5):
                arq = '%s_%s_%d' % (nome, cl, v)
                G.semeia(arq)
                aj = AJUSTE.get(cl, 0) + (AJUSTE_POUSA.get(cl, 0) if nome == 'pousa' else 0)
                grava(saida, arq, G.sala(receita(v), sala), G.VOLUME[ref] * db(aj))
        print('%-9s ok' % cl)
    for cl, receita in ROLA.items():
        if not (quer(cl) or quer('rola_' + cl)): continue
        arq = 'rola_' + cl
        G.semeia(arq)
        grava(saida, arq, receita(), rufo_ref() * db(AJUSTE_ROLA.get(cl, 0)), laco=True)
        print('%-12s ok' % arq)
    # A Furia e grave (o corpo fica abaixo de 150 Hz): o volume dela e medido na
    # faixa toda, no nivel da Furia antiga (-12,4 dB na janela de 50 ms); o
    # Tremor, acima de 150 Hz, como antes.
    for nome, receita, ganho, acima in (('pousa_furia', pousa_furia, 4.8, 0), ('pousa_tremor', pousa_tremor, 1.0, 150)):
        if not quer(nome): continue
        for v in range(3):
            arq = '%s_%d' % (nome, v)
            G.semeia(arq)
            grava(saida, arq, G.sala(receita(v), 0.14), G.VOLUME['pousa'] * db(ganho), acima=acima)
        print('%-12s ok' % nome)
    for nome, (receita, ref, sala) in MOEDA_M.items():
        if not quer(nome): continue
        for v in range(5):
            arq = '%s_%d' % (nome, v)
            G.semeia(arq)
            grava(saida, arq, G.sala(receita(v), sala), G.VOLUME[ref])
        print('%-12s ok' % nome)
    for nome, (receita, alvo) in UM.items():
        if not quer(nome): continue
        G.semeia(nome)
        grava(saida, nome, receita(), alvo)
        print('%-12s ok' % nome)
    for nome, (receita, n, alvo) in VARIAS.items():
        if not quer(nome): continue
        for v in range(n):
            arq = '%s_%d' % (nome, v)
            G.semeia(arq)
            grava(saida, arq, receita(v), alvo)
        print('%-12s ok' % nome)

if __name__ == '__main__':
    main()
