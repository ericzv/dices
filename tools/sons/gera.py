#!/usr/bin/env python3
"""Gera os sons do jogo (desktop/sons/*.ogg) a partir das gravacoes de
tools/sons/fontes/, que o tools/sons/baixa.py baixa.

  python3 tools/sons/gera.py                # todos
  python3 tools/sons/gera.py vidro fogo     # so esses (os outros nao mudam)
  python3 tools/sons/gera.py --saida DIR    # grava em outro lugar, para comparar

Precisa de numpy, scipy e imageio-ffmpeg (ou um ffmpeg no PATH).

Cada som tem a sua semente: gerar um nao muda os outros, e gerar de novo da
o mesmo resultado. Os arquivos que estao no repositorio sao os que foram
aprovados ouvindo; gerar de novo da sons equivalentes (a mesma receita, com
outros detalhes ao acaso, como a ordem das fichas caindo).

O volume de cada som fica igual ao do som sintetizado que ele substituiu
(VOLUME, medido do som.c antigo), com os ajustes pedidos em GANHO_DB.

As receitas:
  - mesa e dados: gravacoes de dados em madeira, feltro, metal e moedas
    (dice-box-threejs, MIT);
  - jogadas, partida e trilha: as notas do som.c antigo, tocadas por teclado
    (Rhodes Mark I 1977, jRhodes3d) e baixo eletrico (Karoryfer, CC BY 3.0);
    o 4o "valendo" tem um sax-alto (Karoryfer, CC BY 3.0);
  - poderes e roleta: gravacoes e instrumentos do FluidR3 GM (CC BY 3.0);
  - laser, ventania e o chiado do fogo: sintetizados aqui mesmo.
"""
import os, sys, random, shutil, subprocess, tempfile, wave, zlib
import numpy as np
from scipy.signal import butter, sosfilt, fftconvolve, lfilter
from scipy.ndimage import minimum_filter1d

AQUI = os.path.dirname(os.path.abspath(__file__))
FONTES = os.path.join(AQUI, 'fontes')
SAIDA = os.path.join(AQUI, '..', '..', 'desktop', 'sons')
SR = 44100

# ---------------------------------------------------------------------------
# Ler e gravar audio (com o ffmpeg)
# ---------------------------------------------------------------------------
def ffmpeg():
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError:
        ff = shutil.which('ffmpeg')
        if not ff: sys.exit('preciso do ffmpeg: pip install imageio-ffmpeg')
        return ff
FF = ffmpeg()

def le(caminho):
    p = subprocess.run([FF, '-v', 'quiet', '-i', caminho, '-f', 's16le', '-ac', '1', '-ar', str(SR), '-'],
                       capture_output=True, check=True)
    return np.frombuffer(p.stdout, dtype=np.int16).astype(np.float64) / 32768

def grava_ogg(caminho, x, qualidade=4):
    """x: mono (n,) ou estereo (n, 2), em -1..1."""
    x = np.clip(x, -1, 1)
    ch = 1 if x.ndim == 1 else x.shape[1]
    with tempfile.TemporaryDirectory() as tmp:
        w = os.path.join(tmp, 'x.wav')
        with wave.open(w, 'wb') as f:
            f.setnchannels(ch); f.setsampwidth(2); f.setframerate(SR)
            f.writeframes((x * 32767).astype(np.int16).tobytes())
        subprocess.run([FF, '-v', 'error', '-y', '-i', w, '-c:a', 'libvorbis', '-q:a', str(qualidade), caminho],
                       check=True)

_cache = {}
def carrega(caminho):
    """Le uma gravacao e corta o silencio do comeco (o mp3 poe um pouco)."""
    if caminho not in _cache:
        if not os.path.exists(caminho):
            sys.exit('falta %s: rode tools/sons/baixa.py' % os.path.relpath(caminho))
        x = le(caminho)
        a = np.abs(x)
        i = int(np.argmax(a > a.max() * 0.02)) if a.max() > 0 else 0
        _cache[caminho] = x[max(0, i - 22):].copy()
    return _cache[caminho]

# ---------------------------------------------------------------------------
# Ferramentas de mixagem
# ---------------------------------------------------------------------------
rnd = random.Random(0)                  # trocados a cada som por semeia()
rng = np.random.default_rng(0)

def semeia(nome):
    global rnd, rng
    s = zlib.crc32(nome.encode())
    rnd = random.Random(s)
    rng = np.random.default_rng(s)

def tom(x, semitons):
    """Muda a altura reamostrando (como uma fita mais rapida ou mais lenta)."""
    if semitons == 0: return x
    t = np.arange(0, len(x) - 1, 2 ** (semitons / 12))
    return np.interp(t, np.arange(len(x)), x)

def reamostra(x, semitons, n_saida, curva=None):
    """Como tom(), mas le so o pedaco que precisa; 'curva' (semitons por
    amostra de saida) faz a altura deslizar."""
    passo = np.full(n_saida, 2 ** (semitons / 12))
    if curva is not None:
        passo *= 2 ** (curva[:n_saida] / 12)
    pos = np.cumsum(passo) - passo[0]
    pos = pos[pos < len(x) - 1]
    return np.interp(pos, np.arange(len(x)), x)

def pb(x, fc, ordem=2): return sosfilt(butter(ordem, fc, 'low', fs=SR, output='sos'), x)
def pa(x, fc, ordem=2): return sosfilt(butter(ordem, fc, 'high', fs=SR, output='sos'), x)
def banda(x, lo, hi): return sosfilt(butter(2, [lo, hi], 'band', fs=SR, output='sos'), x)

def ruido(dur, grave=200, agudo=4000):
    return banda(rng.standard_normal(int(dur * SR)), grave, agudo)

def env(n, ataque, queda):
    t = np.arange(n) / SR
    return np.minimum(1, t / max(ataque, 1e-4)) * np.exp(-t / queda)

class Mix:
    def __init__(self, dur):
        self.x = np.zeros(int(dur * SR))
    def poe(self, t, y, g=1.0):
        i = int(t * SR)
        if i >= len(self.x): return
        n = min(len(y), len(self.x) - i)
        self.x[i:i + n] += g * y[:n]

_ir = None
def sala(x, mistura=0.18):
    """A mesma sala para todos os efeitos: ruido que cai, um pouco escuro."""
    global _ir
    if _ir is None:
        g = np.random.default_rng(7)
        n = int(0.9 * SR)
        r = g.standard_normal(n) * np.exp(-np.arange(n) / (0.16 * SR))
        r = pb(r, 3500)
        r[:int(0.012 * SR)] = 0                       # pre-atraso de 12 ms
        _ir = r / np.sqrt((r ** 2).sum())
    w = fftconvolve(x, _ir)[:len(x) + len(_ir)]
    y = np.zeros(len(w)); y[:len(x)] = x
    return y + mistura * w

def acaba(x, limiar_db=-50):
    """Corta o fim que ja nao se ouve, com 20 ms de saida suave."""
    a = np.abs(x)
    idx = np.where(a > a.max() * 10 ** (limiar_db / 20))[0]
    x = x[:idx[-1] + 1 if len(idx) else len(x)].copy()
    r = min(int(0.02 * SR), len(x))
    x[len(x) - r:] *= np.linspace(1, 0, r)
    return x

def volume(x):
    """Volume percebido: o maior RMS numa janela de 50 ms."""
    w = int(0.05 * SR); h = int(0.005 * SR)
    if len(x) <= w: return np.sqrt(np.mean(x ** 2))
    return max(np.sqrt(np.mean(x[i:i + w] ** 2)) for i in range(0, len(x) - w, h))

def limita(x, teto):
    """Limitador simples: segura os picos acima do teto (3 ms de antecipacao,
    solta em 60 ms), sem mexer no resto."""
    g = np.minimum(1, teto / np.maximum(np.abs(x), 1e-9))
    g = minimum_filter1d(g, 2 * int(0.003 * SR) + 1)
    k = 1 - np.exp(-1 / (0.06 * SR))
    out = np.empty_like(g); y = 1.0
    for i in range(len(g)):
        y = g[i] if g[i] < y else y + (g[i] - y) * k
        out[i] = y
    return x * out

def iguala(x, alvo, teto=0.85):
    for _ in range(3):
        x = x * (alvo / max(volume(x), 1e-9))
        if np.abs(x).max() <= teto: break
        x = limita(x, teto)
    p = np.abs(x).max()
    return x * (0.97 / p) if p > 0.97 else x

# ---------------------------------------------------------------------------
# As fontes
# ---------------------------------------------------------------------------
NOTA_FL = ['C', 'Db', 'D', 'Eb', 'E', 'F', 'Gb', 'G', 'Ab', 'A', 'Bb', 'B']
NOTA_S = ['C', 'Cs', 'D', 'Ds', 'E', 'F', 'Fs', 'G', 'Gs', 'A', 'As', 'B']

def batida(nome):            # dado com dado (dicehit_*)
    return carrega(os.path.join(FONTES, 'dados', 'dicehit', nome + '.mp3'))

def superficie(nome):        # dado na superficie (surface_*)
    return carrega(os.path.join(FONTES, 'dados', 'surfaces', nome + '.mp3'))

def nota(inst, midi, dur=None, solta=0.12):
    """Uma nota do FluidR3 GM."""
    x = carrega(os.path.join(FONTES, 'fluidr3', inst, NOTA_FL[midi % 12] + str(midi // 12 - 1) + '.mp3')).copy()
    if dur is not None:
        n = int(dur * SR); r = int(solta * SR)
        x = x[:n + r].copy()
        if len(x) > n:
            x[n:] *= np.linspace(1, 0, len(x) - n) ** 2
    return x

# Teclado: o Rhodes do jRhodes3d, com as regioes e camadas de forca do .sfz.
REGIOES = [(21, 32, 29), (33, 37, 35), (38, 42, 40), (43, 47, 45), (48, 52, 50), (53, 57, 55),
           (58, 60, 59), (61, 63, 62), (64, 68, 65), (69, 73, 71), (74, 78, 76), (79, 83, 81),
           (84, 88, 86), (89, 93, 91), (94, 108, 96)]

def tecla(m, dur, vel=0.6, solta=0.3):
    """Uma nota de Rhodes. vel 0..1 escolhe a camada (mais forte, mais brilho
    e 'latido') e o volume."""
    centro = next(c for lo, hi, c in REGIOES if lo <= m <= hi)
    camada = 5 if vel < 0.4 else 4 if vel < 0.62 else 3 if vel < 0.82 else 2
    p = os.path.join(FONTES, 'rhodes', 'A_%03d__%s%d_%d.flac' % (centro, NOTA_FL[centro % 12], centro // 12 - 1, camada))
    if not os.path.exists(p) and camada == 3:         # nos agudos o banco nao tem a camada 3
        p = p.replace('_3.flac', '_4.flac')
    n, r = int(dur * SR), int(solta * SR)
    y = reamostra(carrega(p), m - centro, n + r)
    if len(y) > n:
        y[n:] *= np.exp(-np.linspace(0, 6, len(y) - n))
    return y * (0.5 + 0.5 * vel)

def baixo(m, dur, vel=0.8, solta=0.07, desliza=None):
    """Baixo eletrico (uma amostra a cada terca menor). desliza: (de, ate) em
    semitons relativos a m, ao longo da nota. Redondo e escuro, por baixo."""
    base = min((b for b in range(25, 71) if b % 12 in (1, 4, 7, 10)), key=lambda b: abs(b - m))
    x = carrega(os.path.join(FONTES, 'baixo', NOTA_S[base % 12] + str(base // 12 - 1) + '.mp3'))
    n, r = int(dur * SR), int(solta * SR)
    curva = None
    if desliza:
        q = np.linspace(0, 1, n + r)
        curva = desliza[0] + (desliza[1] - desliza[0]) * np.minimum(1, q * (n + r) / n)
    y = reamostra(x, m - base, n + r, curva)
    if len(y) > n:
        y[n:] *= np.linspace(1, 0, len(y) - n) ** 2
    return pb(y, 1100) * vel

def sax(m, dur, vel=0.6, ataque=0.05, solta=0.25, chega=1.0):
    """Sax-alto. 'chega': entra esse tanto de semitom abaixo e sobe ate a
    nota em 70 ms, como o saxofonista faz."""
    pasta = os.path.join(FONTES, 'sax')
    disp = {f[:-4] for f in os.listdir(pasta)}
    base = min((b for b in range(40, 90) if NOTA_S[b % 12] + str(b // 12 - 1) in disp), key=lambda b: abs(b - m))
    x = carrega(os.path.join(pasta, NOTA_S[base % 12] + str(base // 12 - 1) + '.mp3'))
    n, r = int(dur * SR), int(solta * SR)
    curva = np.zeros(n + r)
    k = int(0.07 * SR)
    curva[:k] = -chega * (1 - np.linspace(0, 1, k)) ** 2
    y = reamostra(x, m - base, n + r, curva)
    a = int(ataque * SR)
    y[:a] *= np.linspace(0, 1, a) ** 1.5
    if len(y) > n:
        y[n:] *= np.linspace(1, 0, len(y) - n) ** 2
    return pb(y, 4500) * vel * 0.8

PLASTICO = ['dicehit_plastic%d' % i for i in (1, 2, 4, 5, 6, 13, 14, 15)]
FELTRO = ['surface_felt%d' % i for i in (1, 2, 3, 4, 5)]
MADEIRA = ['surface_wood_tray%d' % i for i in (1, 2, 4, 5)]
MOEDA = ['dicehit_coin%d' % i for i in range(1, 7)]
METAL = ['dicehit_metal%d' % i for i in (1, 2, 3, 4, 6, 8, 10, 11)]

def ficha_clique(semi=-4):
    """Uma ficha batendo na outra: o clique de plastico, um pouco mais grave."""
    return tom(batida(rnd.choice(PLASTICO)), semi + rnd.uniform(-1, 1))

def monte_de_fichas(m, t0, t1, n, g0):
    for k in range(n):
        t = t0 + (t1 - t0) * k / n + rnd.uniform(-0.012, 0.012)
        m.poe(max(0, t), ficha_clique(rnd.uniform(-5, -2)), g0 * (1 - 0.7 * k / n) * rnd.uniform(0.7, 1))

def moedas(m, t0, t1, n, g0):
    for k in range(n):
        t = t0 + (t1 - t0) * k / n + rnd.uniform(-0.015, 0.015)
        m.poe(max(0, t), tom(batida(rnd.choice(MOEDA)), rnd.uniform(-3, 1)), g0 * (1 - 0.6 * k / n) * rnd.uniform(0.6, 1))

# ---------------------------------------------------------------------------
# Mesa e dados
# ---------------------------------------------------------------------------
def tique():
    x = pb(superficie('surface_wood_table3'), 5000)[:int(0.06 * SR)]
    return sala(tom(x * env(len(x), 0.0005, 0.012), 7), 0.06)

def passa():
    """O mouse passando por cima: um tic bem curto, agudo e baixinho."""
    x = pb(superficie('surface_wood_table3'), 6000)[:int(0.02 * SR)]
    return sala(tom(x * env(len(x), 0.0003, 0.004), 12), 0.03)

def bate(v):         return sala(superficie(MADEIRA[v % len(MADEIRA)]), 0.08)
def bate_dado(v):    return sala(batida(PLASTICO[v % len(PLASTICO)]), 0.08)
def pousa(v):        return sala(superficie(FELTRO[v % len(FELTRO)]), 0.10)

def ficha(v):
    m = Mix(0.4)
    r = random.Random(v)
    for k, (t, g) in enumerate(((0, 1), (0.045, 0.75), (0.075, 0.5))):
        m.poe(t, tom(batida(PLASTICO[(v * 3 + k) % len(PLASTICO)]), -4 + r.uniform(-1, 1)), g)
    m.poe(0, superficie('surface_felt2'), 0.25)
    return sala(m.x, 0.12)

# Moeda: nem metal nem dado. Uma ficha leve, meio madeira: o clique de
# plastico mais agudo com um toque de madeira por baixo.
def moeda_bate(v):
    """A moeda bate na borda ou num dado."""
    m = Mix(0.3)
    m.poe(0, tom(batida(PLASTICO[(v * 3 + 1) % len(PLASTICO)]), rnd.uniform(-1, 1)), 1.0)
    w = pa(superficie(['surface_wood_table2', 'surface_wood_table3', 'surface_wood_table4'][v % 3]), 1500)
    w = w[:int(0.04 * SR)] * env(int(0.04 * SR), 0.0005, 0.008)
    m.poe(0, tom(w, 3), 0.45)
    return sala(m.x, 0.08)

def moeda_pousa(v):
    """A moeda cai deitada no feltro e bambeia ate parar: um baque macio e
    tiques cada vez mais rapidos e fracos."""
    m = Mix(0.7)
    m.poe(0, pb(superficie(FELTRO[v % len(FELTRO)]), 2500), 0.5)
    t, passo, g = 0.03, 0.075, 0.38
    k = 0
    while passo > 0.018:
        e = pa(tom(batida(PLASTICO[(v + k) % len(PLASTICO)]), 2 + rnd.uniform(-0.5, 0.5)), 2000)
        m.poe(t, e, g)
        t += passo
        passo *= 0.78
        g *= 0.82
        k += 1
    return sala(m.x, 0.1)

def turno():
    m = Mix(0.5)
    m.poe(0, superficie('surface_wood_table3'), 1.0)
    m.poe(0.11, tom(superficie('surface_wood_table4'), -1), 0.7)
    return sala(m.x, 0.10)

def bolsa():
    m = Mix(0.6)
    w = ruido(0.32, 400, 3500)
    t = np.arange(len(w)) / SR
    m.poe(0, w * np.sin(np.pi * np.minimum(1, t / 0.32)) * 0.25, 0.4)
    for k in range(6):
        m.poe(0.02 + k * 0.04 + rnd.uniform(0, 0.015), tom(batida(rnd.choice(PLASTICO)), rnd.uniform(-2, 2)), rnd.uniform(0.5, 1))
    m.poe(0.3, superficie('surface_felt2'), 0.5)
    return sala(m.x, 0.12)

def loop(m, dur):
    """Fecha um loop: a cauda que passou do fim volta para o comeco."""
    x = m.x; n = int(dur * SR)
    x[:len(x) - n] += x[n:]
    return x[:n]

def rufo():
    """O dado rolando no feltro: baques miudos, cada vez mais firmes (8 s)."""
    m = Mix(9.0)
    t = 0.0
    while t < 8.0:
        cresce = min(1.0, 0.3 + 0.7 * t / 1.5)
        m.poe(t, tom(superficie(rnd.choice(FELTRO)), rnd.uniform(-3, 3)), cresce * rnd.uniform(0.35, 0.8))
        if rnd.random() < 0.12:
            m.poe(t + 0.01, batida(rnd.choice(PLASTICO)), cresce * 0.25)
        t += rnd.uniform(0.045, 0.085)
    return sala(loop(m, 8.0), 0.10)[:int(8.0 * SR)]

def moeda_rola():
    """A moeda girando enquanto corre a mesa: a beirada tocando o feltro a
    cada volta, com um sopro bem leve que pulsa no giro (8 s)."""
    m = Mix(9.0)
    n = int(8.0 * SR); t = np.arange(n) / SR
    giro = np.abs(np.sin(2 * np.pi * 7.0 * t + 0.6 * np.sin(2 * np.pi * 0.4 * t))) ** 3
    m.poe(0, banda(rng.standard_normal(n), 600, 2500) * (0.2 + 0.8 * giro), 0.015)
    tt = 0.05
    while tt < 8.0:
        e = pb(pa(tom(batida(rnd.choice(PLASTICO)), 4 + rnd.uniform(-1, 1)), 1800), 5000)
        m.poe(tt, e, rnd.uniform(0.08, 0.16))
        tt += rnd.uniform(0.07, 0.16)
    return sala(loop(m, 8.0), 0.08)[:n]

# ---------------------------------------------------------------------------
# Jogadas e partida: as notas e os tempos do som.c antigo, em teclado e baixo
# ---------------------------------------------------------------------------
G_BX = 0.3            # o baixo acompanha, bem abaixo do teclado

def valido(pos):
    """G5 B5 D6 em arpejo, subindo +0, +2, +4 e +7 semitons a cada posicao.
    O 4o fecha a fila: mais forte, o arpejo chega ao Re de cima, o acorde
    soa mais longo e um sax-alto entra no Re."""
    sobe = (0, 2, 4, 7)[pos]
    m = Mix(2.6)
    if pos < 3:
        for i, n in enumerate((79, 83, 86)):
            m.poe(i * 0.055, tecla(n + sobe, 0.55, 0.6, 0.35))
        m.poe(0, baixo(43 + sobe, 0.3, 0.8), G_BX)
        return sala(m.x, 0.22)
    for i, n in enumerate((86, 90, 93, 98)):                      # D6 F#6 A6 e o D7 por cima
        m.poe(i * 0.055, tecla(n, 1.3, 0.82, 0.6))
    for i, n in enumerate((74, 78, 81)):                          # o acorde por baixo, mais cheio
        m.poe(0.01 + i * 0.012, tecla(n, 1.3, 0.6, 0.6), 0.7)
    m.poe(0, baixo(38, 1.3, 0.85), G_BX)
    m.poe(0.11, sala(sax(74, 0.75, 0.7), 0.35), 1.0)
    return sala(m.x, 0.24)

def anula():
    """O baque e as duas descidas: La3 escorregando ate La2 e, logo depois,
    Lab3 ate Sol2, no baixo."""
    m = Mix(1.2)
    m.poe(0, superficie('surface_felt3'), 0.5)
    m.poe(0.0, baixo(57, 0.40, 0.9, 0.12, desliza=(0, -12)), 1.0)
    m.poe(0.09, baixo(56, 0.45, 0.8, 0.14, desliza=(0, -13)), 0.9)
    m.poe(0.0, tecla(57, 0.12, 0.45, 0.15), 0.5)
    return sala(m.x, 0.2)

def efeito():
    """A5 C#6 E6 A6 em arpejo, com o La grave no baixo."""
    m = Mix(1.6)
    for i, n in enumerate((81, 85, 88, 93)):
        m.poe(i * 0.045, tecla(n, 0.6, 0.55 + 0.05 * i, 0.4))
    m.poe(0, baixo(45, 0.5, 0.7), G_BX)
    return sala(m.x, 0.24)

def vitoria():
    """C5 E5 G5 C6 E6 em arpejo, Do no baixo e as fichas caindo no monte."""
    m = Mix(2.4)
    for i, n in enumerate((72, 76, 79, 84, 88)):
        m.poe(i * 0.07, tecla(n, 1.1 if i < 4 else 1.4, 0.62 if i < 4 else 0.72, 0.5))
    m.poe(0, baixo(36, 1.2, 0.85), G_BX)
    monte_de_fichas(m, 0.30, 0.65, 7, 0.35)
    return sala(m.x, 0.22)

def correu():
    """Sol4 e depois Re4 caindo para Do#4; o baixo faz a caida."""
    m = Mix(1.6)
    m.poe(0.0, tecla(67, 0.3, 0.55, 0.2))
    m.poe(0.20, tecla(62, 0.6, 0.5, 0.35))
    m.poe(0.0, baixo(43, 0.3, 0.8), G_BX)
    m.poe(0.20, baixo(38, 0.65, 0.8, 0.1, desliza=(0, -1)), G_BX)
    return sala(m.x, 0.22)

def empate():
    """Mi5 duas vezes, com o mesmo Mi no baixo."""
    m = Mix(1.6)
    m.poe(0, tecla(76, 0.25, 0.6, 0.3))
    m.poe(0.16, tecla(76, 0.75, 0.55, 0.45))
    m.poe(0, baixo(40, 0.14, 0.75), G_BX)
    m.poe(0.16, baixo(40, 0.6, 0.7), G_BX)
    return sala(m.x, 0.22)

def partida():
    """Do maior e, aos 0,22 s, Sol maior; Do e Sol no baixo."""
    m = Mix(2.2)
    for i, n in enumerate((72, 76, 79)):
        m.poe(i * 0.02, tecla(n, 0.25, 0.55, 0.3))
    for i, n in enumerate((74, 79, 83)):
        m.poe(0.22 + i * 0.02, tecla(n, 1.1, 0.62, 0.5))
    m.poe(0, baixo(36, 0.22, 0.8), G_BX)
    m.poe(0.22, baixo(43, 1.1, 0.8), G_BX)
    return sala(m.x, 0.24)

def fim():
    """G4 B4 D5 subindo e depois C5 E5 G5 C6; Sol e Do no baixo."""
    m = Mix(2.6)
    for i, n in enumerate((67, 71, 74, 72, 76, 79, 84)):
        t = i * 0.06 if i < 3 else 0.30 + (i - 3) * 0.05
        m.poe(t, tecla(n, 0.3 if i < 3 else (1.4 if i < 6 else 1.7), 0.58 if i < 6 else 0.7, 0.3 if i < 3 else 0.6))
    m.poe(0, baixo(43, 0.3, 0.8), G_BX)
    m.poe(0.30, baixo(36, 1.5, 0.85), G_BX)
    return sala(m.x, 0.24)

def derrota():
    """La4, Fa4, Re4 descendo; o baixo dobra a frase duas oitavas abaixo."""
    m = Mix(2.0)
    for i, (n, d) in enumerate(((69, 0.25), (65, 0.25), (62, 0.9))):
        m.poe(i * 0.22, tecla(n, d, 0.52, 0.25 if i < 2 else 0.5))
        m.poe(i * 0.22, baixo(n - 24, d, 0.75), G_BX)
    return sala(m.x, 0.24)

def abertura():
    """F4 A4 C5 E5 A5 em arpejo, o ultimo mais longo; Fa no baixo."""
    m = Mix(2.8)
    for i, n in enumerate((65, 69, 72, 76, 81)):
        m.poe(i * 0.09, tecla(n, 1.5 - i * 0.09 if i < 4 else 1.8, 0.58 if i < 4 else 0.72, 0.6))
    m.poe(0, baixo(41, 1.6, 0.85), G_BX)
    return sala(m.x, 0.25)

# ---------------------------------------------------------------------------
# Poderes dos dados
# ---------------------------------------------------------------------------
def laser():
    """O laser do som.c antigo, sintetizado do mesmo jeito: seno despencando,
    um triangulo por cima e um chiado curto, numa sala de Schroeder."""
    n_ef = SR * 3
    ef = np.zeros(n_ef)
    def tom_c(t0, dur, f0, f1, tri, vol, queda):
        n = int(dur * SR); t = np.arange(n) / SR; q = np.arange(n) / n
        fase = np.cumsum(f0 * (f1 / f0) ** q / SR)
        e = np.minimum(1, t / 0.003) * np.exp(-t / queda) * np.where(q > 0.9, (1 - q) * 10, 1)
        x = 4 * np.abs(fase % 1 - 0.5) - 1 if tri else np.sin(2 * np.pi * fase)
        i0 = int(t0 * SR); ef[i0:i0 + n] += vol * e * x
    def ruido_c(t0, dur, vol, queda, grave, agudo):
        n = int(dur * SR); t = np.arange(n) / SR
        x = lfilter([grave], [1, -(1 - grave)], rng.uniform(-1, 1, n))
        x = x - lfilter([agudo], [1, -(1 - agudo)], x)
        i0 = int(t0 * SR); ef[i0:i0 + n] += vol * np.minimum(1, t / 0.002) * np.exp(-t / queda) * x
    tom_c(0, 0.28, 2600, 260, False, 0.20, 0.10)
    tom_c(0, 0.22, 3900, 520, True, 0.05, 0.06)
    ruido_c(0, 0.08, 0.10, 0.02, 0.9, 0.5)
    # reverb de Schroeder do som.c: quatro pentes e dois passa-tudo
    pentes = [int(p * 0.5) for p in (1557, 1617, 1491, 1422)]
    buf = [np.zeros(p) for p in pentes]; ip = [0] * 4; amort = [0.0] * 4
    pt = [np.zeros(225), np.zeros(556)]; it = [0, 0]
    out = np.empty(n_ef)
    for i in range(n_ef):
        s = 0.0
        for k in range(4):
            y = buf[k][ip[k]]
            amort[k] += 0.3 * (y - amort[k])
            buf[k][ip[k]] = ef[i] + amort[k] * 0.70
            ip[k] = (ip[k] + 1) % pentes[k]
            s += y
        s *= 0.25
        for k in range(2):
            y = pt[k][it[k]]
            pt[k][it[k]] = s + y * 0.5
            s = y - s * 0.5
            it[k] = (it[k] + 1) % len(pt[k])
        out[i] = ef[i] + 0.35 * s
    return out

def vento():
    """Chiado leve e rapido: um sopro de ar que abre e passa em 0,4 s."""
    n = int(0.42 * SR); t = np.arange(n) / SR
    w = rng.standard_normal(n)
    q = t / t[-1]
    forma = np.sin(np.pi * np.minimum(1, q / 0.3) / 2) ** 2 * np.where(q < 0.3, 1, (1 - (q - 0.3) / 0.7) ** 2)
    abre = np.clip(q / 0.4, 0, 1)                     # fica mais claro no meio
    m = Mix(0.6)
    m.poe(0, forma * (banda(w, 700, 3000) * (1 - 0.6 * abre) + banda(w, 2500, 9000) * abre))
    return sala(m.x, 0.15)

def vidro():
    """Estalo forte e cacos caindo por mais de um segundo, cada vez mais raros."""
    m = Mix(1.8)
    m.poe(0, tom(batida('dicehit_metal2'), 5), 1.0)
    m.poe(0.004, tom(batida('dicehit_metal8'), 9), 0.8)
    m.poe(0, pa(rng.standard_normal(int(0.03 * SR)), 2500) * env(int(0.03 * SR), 0.0005, 0.006), 0.7)
    m.poe(0, pb(superficie('surface_wood_table3'), 900), 0.35)       # o corpo do impacto
    t, k = 0.015, 0
    while t < 1.15:
        e = tom(batida(METAL[k % len(METAL)] if k % 3 else MOEDA[k % len(MOEDA)]), rng.uniform(5, 14))
        m.poe(t, e, 0.95 * max(0.12, 1 - t / 1.2) * rng.uniform(0.5, 1))
        t += 0.012 + 0.09 * (t / 1.15) ** 1.5 * rng.uniform(0.5, 1.5)   # cada vez mais espacado
        k += 1
    for j in range(9):
        tt = 0.05 + j * 0.11 + rng.uniform(0, 0.04)
        m.poe(tt, nota('glockenspiel', int(rng.choice((98, 100, 103, 105, 107))), 0.05, 0.15), 0.16 * (1 - j / 11))
    return sala(m.x, 0.18)

def fogo():
    """Brasas estalando e, no fim, um fogo crepitando que vai sumindo."""
    m = Mix(2.4)
    b = pb(rng.standard_normal(int(0.55 * SR)), 900) * 3
    m.poe(0, b * env(len(b), 0.02, 0.2), 0.15)
    for k in range(10):
        e = tom(batida(['dicehit_wood10', 'dicehit_wood11', 'dicehit_wood5', 'dicehit_wood8'][k % 4]), rng.uniform(-9, -3))
        m.poe(0.02 + k * 0.045 + rng.uniform(0, 0.02), e, rng.uniform(0.4, 1.0))
    t0, dur = 0.30, 1.6                               # o chiado, tremendo como chama
    n = int(dur * SR); t = np.arange(n) / SR
    chama = np.clip(pb(rng.standard_normal(n), 9) * 6 + 1, 0.3, 1.8)
    forma = np.minimum(1, t / 0.2) * np.where(t > dur - 0.8, (dur - t) / 0.8, 1) ** 1.5
    m.poe(t0, banda(rng.standard_normal(n), 500, 3800) * chama * forma, 0.04)
    tt = t0 + 0.05                                    # estalinhos, cada vez mais raros
    while tt < t0 + dur - 0.1:
        q = (tt - t0) / dur
        nn = int(rng.uniform(0.0008, 0.0035) * SR)
        m.poe(tt, pa(rng.standard_normal(nn + 64), 1800)[:nn] * env(nn, 0.0002, nn / SR / 3), rng.uniform(0.15, 1.0) * (1 - 0.6 * q))
        if rng.random() < 0.08:
            m.poe(tt, tom(batida('dicehit_wood11'), rng.uniform(-10, -6)), 0.4 * (1 - q))
        tt += rng.exponential(1 / (40 - 25 * q))
    return sala(m.x, 0.14)

def tira():
    m = Mix(0.8)
    m.poe(0, nota('acoustic_bass', 45, 0.12, 0.06), 1.0)
    m.poe(0.11, nota('acoustic_bass', 40, 0.22, 0.10), 0.95)
    return sala(m.x, 0.12)

def rouba():
    m = Mix(0.6)
    for k, t in enumerate((0, 0.05, 0.09, 0.12, 0.14)):
        m.poe(t, superficie(FELTRO[k % len(FELTRO)]), 0.9 * (1 - k / 6))
    w = ruido(0.28, 900, 5000)
    t = np.arange(len(w)) / SR
    m.poe(0, w * np.sin(np.pi * np.minimum(1, t / 0.28)) ** 2, 0.12)
    m.poe(0.2, ficha_clique(-2), 0.35)
    return sala(m.x, 0.12)

def zera():
    m = Mix(1.6)
    m.poe(0, nota('timpani', 38, 0.9, 0.5), 1.0)
    m.poe(0, nota('acoustic_bass', 38, 0.6, 0.4), 0.5)
    m.poe(0.04, pb(nota('marimba', 50, 0.4, 0.2), 1200), 0.3)
    return sala(m.x, 0.2)

def bigorna():
    m = Mix(1.4)
    m.poe(0, superficie('surface_metal4'), 0.8)
    m.poe(0, nota('tubular_bells', 84, 0.8, 0.4), 0.55)
    m.poe(0.18, tom(superficie('surface_metal6'), 2), 0.25)
    m.poe(0.18, nota('tubular_bells', 84, 0.3, 0.3), 0.12)
    return sala(m.x, 0.18)

def jackpot():
    m = Mix(2.0)
    for k, n in enumerate((84, 89, 93, 96, 93, 96)):
        m.poe(k * 0.075, nota('glockenspiel', n, 0.3, 0.3), 0.6)
    for n in (77, 81, 84):
        m.poe(0.45, nota('vibraphone', n, 1.0, 0.5), 0.5)
    m.poe(0.45, nota('glockenspiel', 101, 0.8, 0.5), 0.45)
    moedas(m, 0.40, 1.30, 16, 0.45)
    return sala(m.x, 0.2)

# ---------------------------------------------------------------------------
# Roleta do premio
# ---------------------------------------------------------------------------
def giro():
    """Arpejo de celesta a 12 notas por segundo, com o tique-taque dos rolos (4 s)."""
    m = Mix(5.0)
    passo = 1 / 12
    A = (77, 81, 84, 89, 93, 89, 84, 81)
    B = (72, 76, 79, 84, 88, 84, 79, 76)
    for k in range(int(4.0 / passo)):
        m.poe(k * passo, nota('celesta', (A if (k // 8) % 2 == 0 else B)[k % 8], 0.12, 0.1), 0.5)
        for h in range(2):
            m.poe(k * passo + h * passo / 2, tom(batida(PLASTICO[(k * 2 + h) % len(PLASTICO)]), 3), 0.35)
    return sala(loop(m, 4.0), 0.12)[:int(4.0 * SR)]

def slot():
    m = Mix(0.8)
    m.poe(0, tom(batida('dicehit_metal2'), -7), 0.7)
    m.poe(0, superficie('surface_wood_table2'), 0.5)
    m.poe(0.02, nota('glockenspiel', 96, 0.25, 0.25), 0.35)
    return sala(m.x, 0.15)

def premio():
    m = Mix(2.0)
    for k, n in enumerate((72, 77, 81, 84)):
        m.poe(k * 0.06, nota('vibraphone', n, 0.25, 0.3), 0.7)
    m.poe(0.24, nota('vibraphone', 89, 1.1, 0.5), 0.8)
    m.poe(0.24, nota('glockenspiel', 96, 0.9, 0.5), 0.4)
    moedas(m, 0.30, 0.90, 12, 0.4)
    return sala(m.x, 0.22)

# ---------------------------------------------------------------------------
# Trilha: a bossa de 16 compassos do som.c antigo, nota por nota. Os acordes
# e a melodia no teclado, a linha de baixo no baixo eletrico; a vassourinha
# e o shaker sintetizados como antes. Cada parte nova fica no volume da
# parte sintetizada que substitui; o baixo, 7,5 dB abaixo, e a melodia, 1 dB
# acima: o teclado manda.
# ---------------------------------------------------------------------------
BAT = 60 / 100.0                        # 100 batidas por minuto
COMP = 16
NT = int(COMP * 4 * BAT * SR)
MAJ7, M7, DOM7, M6 = range(4)
INTERV = [(0, 4, 7, 11), (0, 3, 7, 10), (0, 4, 7, 10), (0, 3, 7, 9)]
# Fa maior: I vi ii V, com um desvio para Bb e Bbm6 no meio.
HARMONIA = [((41, MAJ7), (41, MAJ7)), ((38, M7), (38, M7)), ((43, M7), (43, M7)), ((36, DOM7), (36, DOM7)),
            ((41, MAJ7), (41, MAJ7)), ((38, M7), (38, M7)), ((43, M7), (36, DOM7)), ((41, MAJ7), (41, MAJ7)),
            ((46, MAJ7), (46, MAJ7)), ((46, M6), (46, M6)), ((45, M7), (45, M7)), ((38, DOM7), (38, DOM7)),
            ((43, M7), (43, M7)), ((36, DOM7), (36, DOM7)), ((41, MAJ7), (41, MAJ7)), ((43, M7), (36, DOM7))]
# { compasso, batida, duracao, nota MIDI }
MELODIA = [(0,0,1.5,69),(0,1.5,.5,72),(0,2,2,76),(1,0,1,74),(1,1,1,72),(1,2,2,69),
    (2,0,1.5,70),(2,1.5,.5,74),(2,2,1.5,77),(2,3.5,.5,76),(3,0,1,76),(3,1,.5,74),(3,1.5,.5,72),(3,2,2,70),
    (4,0,1,69),(4,1,1,72),(4,2,1,77),(4,3,1,76),(5,0,1.5,77),(5,1.5,.5,76),(5,2,2,74),
    (6,0,1,74),(6,1,1,70),(6,2,1,76),(6,3,1,79),(7,0,3,77),
    (8,0,1.5,74),(8,1.5,.5,77),(8,2,2,81),(9,0,1,79),(9,1,.5,77),(9,1.5,.5,73),(9,2,2,77),
    (10,0,1.5,76),(10,1.5,.5,72),(10,2,1,79),(10,3,1,76),(11,0,1,78),(11,1,1,81),(11,2,1,84),(11,3,1,81),
    (12,0,1.5,82),(12,1.5,.5,81),(12,2,1,79),(12,3,1,74),(13,0,1,76),(13,1,1,79),(13,2,1,82),(13,3,.5,76),(13,3.5,.5,79),
    (14,0,1.5,81),(14,1.5,.5,79),(14,2,2,77),(15,0,1,76),(15,1,1,72)]

def trilha():
    pistas = {k: (np.zeros(NT), np.zeros(NT)) for k in
              ('ep', 'baixo', 'vibra', 'bateria', 'teclado', 'baixo_novo', 'melodia')}
    def poe(pista, bat, y, pan, g=1.0):
        L, R = pistas[pista]
        idx = (np.arange(len(y)) + int(bat * BAT * SR)) % NT           # da a volta: o loop emenda
        np.add.at(L, idx, g * y * (1 - pan) * 0.5)
        np.add.at(R, idx, g * y * (1 + pan) * 0.5)

    def sint(inst, dur, midi):
        """Os instrumentos sintetizados do som.c antigo, so para medir o volume
        de cada parte."""
        f = 440.0 * 2 ** ((midi - 69) / 12); soa = dur * BAT
        cauda = {'baixo': 0.06, 'ep': 0.25, 'vibra': 0.9}[inst]
        n = int((soa + cauda) * SR); t = np.arange(n) / SR
        solta = np.where(t > soa, np.exp(-(t - soa) / (cauda * 0.3)), 1)
        if inst == 'ep':
            ind = 1.6 * np.exp(-t / 0.25) + 0.35
            x = np.sin(2 * np.pi * f * 1.003 * t + ind * np.sin(2 * np.pi * f * t)) * np.exp(-t / 1.4)
            x *= np.minimum(1, t / 0.004)
        elif inst == 'vibra':
            trem = 1 - 0.22 * (0.5 + 0.5 * np.sin(2 * np.pi * 5.2 * t))
            x = (np.sin(2 * np.pi * f * t) + 0.22 * np.sin(2 * np.pi * 4 * f * t) * np.exp(-t / 0.15)) * np.exp(-t / 1.3) * trem
            x *= np.minimum(1, t / 0.002)
        else:
            x = (np.sin(2 * np.pi * f * t) + 0.35 * np.sin(4 * np.pi * f * t) * np.exp(-t / 0.15)
                 + 0.12 * np.sin(6 * np.pi * f * t) * np.exp(-t / 0.08)) * np.exp(-t / 0.9)
            x *= np.minimum(1, t / 0.008)
        return x * solta

    def acorde(bat, dur, ac, vel):
        raiz, tipo = ac
        notas = []
        for k in range(1, 4):                         # sem a raiz: o baixo cuida dela
            mm = raiz + INTERV[tipo][k]
            while mm < 55: mm += 12
            while mm >= 67: mm -= 12
            notas.append((mm, vel))
        nona = raiz + 14                              # a nona da o tempero de bossa
        while nona < 60: nona += 12
        while nona >= 72: nona -= 12
        notas.append((nona, vel * 0.6))
        for mm, v in notas:
            poe('ep', bat, sint('ep', dur, mm), -0.35, v)
            poe('teclado', bat + rng.uniform(0, 0.006), tecla(mm, dur * BAT, 0.45, 0.25), -0.35, v)

    def nota_baixo(bat, dur, mm, vel):
        poe('baixo', bat, sint('baixo', dur, mm), 0, vel)
        poe('baixo_novo', bat, baixo(mm, dur * BAT, 1.0, 0.06), 0, vel)

    def bumbo(bat, vel):
        n = int(SR / 5); t = np.arange(n) / SR
        fase = np.cumsum((48 + 50 * np.exp(-t / 0.03)) / SR)
        poe('bateria', bat, np.sin(2 * np.pi * fase) * np.exp(-t / 0.10), 0, vel)

    def chiado(bat, dur, vel, grave, agudo, pan):     # vassourinha (grave) ou shaker (agudo)
        n = int(dur * SR * 4); t = np.arange(n) / SR
        r = lfilter([grave], [1, -(1 - grave)], rng.uniform(-1, 1, n))
        r = r - lfilter([agudo], [1, -(1 - agudo)], r)
        poe('bateria', bat, r * np.minimum(1, t / 0.004) * np.exp(-t / dur), pan, vel)

    for c in range(COMP):
        b0 = c * 4.0
        h = HARMONIA[c]
        troca = h[0] != h[1]
        prox = HARMONIA[(c + 1) % COMP][0]
        acorde(b0 + 0.0, 1.25, h[0], 0.09)            # batida de bossa, sincopada
        acorde(b0 + 1.5, 0.5, h[0], 0.07)
        acorde(b0 + 2.5, 1.25, h[1], 0.08)
        r0, r1 = h[0][0], h[1][0]
        while r0 > 45: r0 -= 12
        while r1 > 45: r1 -= 12
        nota_baixo(b0 + 0.0, 1.4, r0, 0.32)           # raiz, quinta e uma aproximacao
        nota_baixo(b0 + 1.5, 0.45, r0 + 7, 0.24)
        nota_baixo(b0 + 2.0, 1.4, r1 if troca else r0 + 7, 0.28)
        alvo = prox[0]
        while alvo > 45: alvo -= 12
        nota_baixo(b0 + 3.5, 0.45, alvo - 1, 0.22)
        bumbo(b0 + 0.0, 0.30)
        bumbo(b0 + 2.5, 0.18)
        chiado(b0 + 1.0, 0.07, 0.045, 0.35, 0.05, -0.3)
        chiado(b0 + 3.0, 0.07, 0.045, 0.35, 0.05, -0.3)
        for e in range(8):
            chiado(b0 + e * 0.5, 0.018, 0.028 if e % 2 else 0.016, 0.95, 0.5, 0.45)
    for (c, b, d, mm) in MELODIA:
        poe('vibra', c * 4 + b, sint('vibra', d, mm), 0.3, 0.12)
        poe('melodia', c * 4 + b, tecla(mm, d * BAT, 0.7, 0.6), 0.3, 0.12)

    def rms(p): return np.sqrt(np.mean(pistas[p][0] ** 2 + pistas[p][1] ** 2))
    for novo, velho, extra in (('teclado', 'ep', 1.0), ('baixo_novo', 'baixo', 0.42), ('melodia', 'vibra', 1.12)):
        g = rms(velho) / max(rms(novo), 1e-12) * extra
        pistas[novo] = (pistas[novo][0] * g, pistas[novo][1] * g)
    L = sum(pistas[p][0] for p in ('teclado', 'baixo_novo', 'melodia', 'bateria'))
    R = sum(pistas[p][1] for p in ('teclado', 'baixo_novo', 'melodia', 'bateria'))

    def ir(semente):
        g = np.random.default_rng(semente)
        n = int(1.6 * SR)
        r = g.standard_normal(n) * np.exp(-np.arange(n) / (0.35 * SR))
        r = pb(r, 4000); r[:int(0.02 * SR)] = 0
        return r / np.sqrt((r ** 2).sum())
    def circ(x, h):                                   # convolucao circular: a sala da a volta no loop
        H = np.fft.rfft(np.pad(h, (0, NT - len(h))))
        return np.fft.irfft(np.fft.rfft(x) * H, NT)
    M = (L + R) * 0.5
    x = np.stack([L + 0.30 * circ(M, ir(1)), R + 0.30 * circ(M, ir(2))], 1)
    for _ in range(3):                                # o volume da trilha antiga, segurando os picos
        x = x * (VOLUME['trilha'] / max(volume(x.mean(1)), 1e-9))
        a = np.abs(x).max(1)
        if a.max() <= 0.9: break
        x = x * (limita(a, 0.9) / np.maximum(a, 1e-9))[:, None]
    p = np.abs(x).max()
    return x * (0.97 / p) if p > 0.97 else x

# ---------------------------------------------------------------------------
# O volume de cada som do som.c antigo (o maior RMS em 50 ms); os novos ficam
# iguais, com os ajustes pedidos em GANHO_DB.
VOLUME = {
    'abertura': 0.1433, 'anula': 0.1359, 'bate': 0.0467, 'bigorna': 0.0845, 'bolsa': 0.0532,
    'correu': 0.1693, 'derrota': 0.1323, 'efeito': 0.1058, 'empate': 0.1109, 'ficha': 0.0648,
    'fim': 0.1222, 'fogo': 0.0738, 'giro': 0.0497, 'jackpot': 0.1646, 'laser': 0.1215,
    'partida': 0.1022, 'pousa': 0.1383, 'premio': 0.1159, 'rouba': 0.0658, 'rufo': 0.0847,
    'slot': 0.0782, 'tique': 0.0408, 'tira': 0.1262, 'trilha': 0.3594, 'turno': 0.0517,
    'valido': 0.1302, 'valido2': 0.1333, 'valido3': 0.1583, 'valido4': 0.144, 'vento': 0.0788,
    'vidro': 0.0806, 'vitoria': 0.1194, 'zera': 0.1765,
}
# Os sons novos, que nao substituem nenhum, ficam abaixo dos parentes:
VOLUME.update({'passa': VOLUME['tique'] * 0.32, 'moeda_bate': VOLUME['bate'] * 0.6,
               'moeda_pousa': VOLUME['pousa'] * 0.45, 'moeda_rola': VOLUME['rufo'] * 0.5})
GANHO_DB = {'vidro': 5.0, 'vento': -3.0, 'valido4': 3.0}

# nome do arquivo -> (receita, som de referencia do volume, e loop?)
SONS = {
    'tique': (tique, 'tique'), 'turno': (turno, 'turno'), 'bolsa': (bolsa, 'bolsa'),
    'rufo': (rufo, 'rufo'), 'giro': (giro, 'giro'), 'passa': (passa, 'passa'),
    'moeda_rola': (moeda_rola, 'moeda_rola'),
    'valido': (lambda: valido(0), 'valido'), 'valido2': (lambda: valido(1), 'valido2'),
    'valido3': (lambda: valido(2), 'valido3'), 'valido4': (lambda: valido(3), 'valido4'),
    'anula': (anula, 'anula'), 'efeito': (efeito, 'efeito'), 'vitoria': (vitoria, 'vitoria'),
    'correu': (correu, 'correu'), 'empate': (empate, 'empate'), 'partida': (partida, 'partida'),
    'fim': (fim, 'fim'), 'derrota': (derrota, 'derrota'), 'abertura': (abertura, 'abertura'),
    'laser': (laser, 'laser'), 'vento': (vento, 'vento'), 'vidro': (vidro, 'vidro'), 'fogo': (fogo, 'fogo'),
    'tira': (tira, 'tira'), 'rouba': (rouba, 'rouba'), 'zera': (zera, 'zera'),
    'bigorna': (bigorna, 'bigorna'), 'jackpot': (jackpot, 'jackpot'),
    'slot': (slot, 'slot'), 'premio': (premio, 'premio'),
}
for v in range(4):
    SONS['bate_%d' % v] = ((lambda v=v: bate(v)), 'bate')
    SONS['bate_dado_%d' % v] = ((lambda v=v: bate_dado(v)), 'bate')
    SONS['ficha_%d' % v] = ((lambda v=v: ficha(v)), 'ficha')
for v in range(5):
    SONS['pousa_%d' % v] = ((lambda v=v: pousa(v)), 'pousa')
for v in range(3):
    SONS['moeda_bate_%d' % v] = ((lambda v=v: moeda_bate(v)), 'moeda_bate')
    SONS['moeda_pousa_%d' % v] = ((lambda v=v: moeda_pousa(v)), 'moeda_pousa')

def gera(nome, saida):
    semeia(nome)
    if nome == 'trilha':
        x = trilha()
        grava_ogg(os.path.join(saida, 'trilha.ogg'), x, 5)
    else:
        receita, ref = SONS[nome]
        x = receita()
        if nome not in ('rufo', 'giro', 'moeda_rola'): x = acaba(x)
        alvo = VOLUME[ref] * 10 ** (GANHO_DB.get(nome, 0.0) / 20)
        x = iguala(x, alvo)
        grava_ogg(os.path.join(saida, nome + '.ogg'), x)
    print('%-12s %5.2f s  pico %.2f' % (nome, len(x) / SR, np.abs(x).max()))

def main():
    args = sys.argv[1:]
    saida = SAIDA
    if '--saida' in args:
        i = args.index('--saida'); saida = args[i + 1]; del args[i:i + 2]
    os.makedirs(saida, exist_ok=True)
    nomes = args or (list(SONS) + ['trilha'])
    for n in nomes:
        if n != 'trilha' and n not in SONS: sys.exit('nao conheco o som "%s"' % n)
    for n in nomes:
        gera(n, saida)

if __name__ == '__main__':
    main()
