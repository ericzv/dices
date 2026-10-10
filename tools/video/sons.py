# Os efeitos e a trilha do jogo (desktop/sons/*.ogg), decodificados para 48 kHz estereo.
import os, subprocess, numpy as np, glob
SONS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "desktop", "sons")
SR = 48000
NOMES = ["tique", "bate", "pousa", "valido", "anula", "ficha", "efeito", "vitoria", "correu", "abertura",
         "jackpot", "laser", "tira", "rouba", "fogo", "zera", "vento", "partida", "fim", "derrota", "turno",
         "bolsa", "empate", "slot", "premio", "vidro", "bigorna", "valido2", "valido3", "valido4", "bate_dado",
         "passa", "moeda_bate", "moeda_pousa"]
_cache = {}

def le(nome):
    if nome in _cache: return _cache[nome]
    p = os.path.join(SONS, nome + ".ogg")
    raw = subprocess.run(["ffmpeg", "-v", "quiet", "-i", p, "-f", "f32le", "-ac", "2", "-ar", str(SR), "-"],
                         capture_output=True, check=True).stdout
    a = np.frombuffer(raw, dtype=np.float32).reshape(-1, 2).copy()
    _cache[nome] = a
    return a

def variantes(som_id):
    n = NOMES[som_id]
    if os.path.exists(os.path.join(SONS, n + ".ogg")): return [le(n)]
    import re
    fs = sorted(x for x in glob.glob(os.path.join(SONS, n + "_*.ogg")) if re.fullmatch(re.escape(n) + r"_\d+\.ogg", os.path.basename(x)))
    return [le(os.path.basename(x)[:-4]) for x in fs]
