# Confere a narracao: transcreve cada frase gerada (Whisper) e mostra as que
# diferem do texto, para achar pronuncias estranhas.
import sys, os, re, hashlib, difflib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np, soundfile as sf, sherpa_onnx
import voz
from gravador import para_voz
W = voz.TTS + "/sherpa-onnx-whisper-small"
rec = sherpa_onnx.OfflineRecognizer.from_whisper(encoder=f"{W}/small-encoder.int8.onnx", decoder=f"{W}/small-decoder.int8.onnx",
        tokens=f"{W}/small-tokens.txt", language="pt", task="transcribe", num_threads=4)
def norm(s):
    s = s.lower()
    s = re.sub(r"[^\wáàâãéêíóôõúç ]", " ", s)
    return " ".join(s.split())
linhas = [l.split("  ", 1)[1].strip() for l in open(sys.argv[1]) if "  " in l]
ruins = 0
for t in linhas:
    tv = para_voz(t)
    h = hashlib.sha1(f"{voz.VOZ}|{voz.VEL}|{tv}".encode()).hexdigest()[:16]
    s, sr = sf.read(os.path.join(voz.CACHE, h + ".wav"), dtype="float32")
    s = np.concatenate([np.zeros(sr // 4, np.float32), s, np.zeros(sr // 2, np.float32)])
    st = rec.create_stream(); st.accept_waveform(sr, s); rec.decode_stream(st)
    a, b = norm(t), norm(st.result.text)
    r = difflib.SequenceMatcher(None, a, b).ratio()
    if r < 0.93:
        ruins += 1
        print(f"{r:.2f} | {t}\n     -> {st.result.text}")
print(len(linhas), "frases,", ruins, "para olhar")
