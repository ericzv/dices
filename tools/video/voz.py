# Narracao: Kokoro (voz pf_dora, portugues do Brasil), com cache em disco.
import os, hashlib, numpy as np, soundfile as sf
# Os modelos (veja o LEIAME): kokoro-v1.0.onnx, voices-v1.0.bin e, para o
# confere_voz.py, a pasta sherpa-onnx-whisper-small.
TTS = os.environ.get("DADO_TTS", os.path.join(os.path.dirname(os.path.abspath(__file__)), "modelos"))
CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cache_voz")
os.makedirs(CACHE, exist_ok=True)
SR = 48000
VOZ, VEL = "pf_dora", 0.85
_k = None

def _kokoro():
    global _k
    if _k is None:
        from kokoro_onnx import Kokoro
        _k = Kokoro(f"{TTS}/kokoro-v1.0.onnx", f"{TTS}/voices-v1.0.bin")
    return _k

def fala(texto):
    """Devolve (amostras float32 a 48 kHz, duracao em segundos)."""
    h = hashlib.sha1(f"{VOZ}|{VEL}|{texto}".encode()).hexdigest()[:16]
    p = os.path.join(CACHE, h + ".wav")
    if not os.path.exists(p):
        s, sr = _kokoro().create(texto, voice=VOZ, speed=VEL, lang="pt-br")
        s = np.asarray(s, dtype=np.float32)
        # corta silencio das pontas e reamostra para 48 kHz
        lim = 0.01 * np.abs(s).max()
        nz = np.nonzero(np.abs(s) > lim)[0]
        if len(nz): s = s[max(0, nz[0] - int(0.03 * sr)): nz[-1] + int(0.08 * sr)]
        n = int(len(s) * SR / sr)
        s = np.interp(np.linspace(0, len(s) - 1, n), np.arange(len(s)), s).astype(np.float32)
        s = s / max(1e-6, np.abs(s).max()) * 0.9
        sf.write(p, s, SR)
    s, _ = sf.read(p, dtype="float32")
    return s, len(s) / SR
