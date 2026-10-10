# Folha de miniaturas da previa (roteiro.py --previa DIR 30), para revisar o
# video sem assistir: ate 16 quadros entre os segundos A e B.
#
#   python3 folha.py previa 600 760 [passo] [saida.jpg]
import sys, glob, os
from PIL import Image, ImageDraw
d = sys.argv[1]
a, b = int(sys.argv[2]), int(sys.argv[3])
passo = int(sys.argv[4]) if len(sys.argv) > 4 else 1
fs = sorted(glob.glob(d + "/q*.jpg"))
sel = [f for f in fs if a <= int(os.path.basename(f)[1:7]) // 30 < b][::passo][:16]
W, H = 480, 270
out = Image.new("RGB", (W * 4, H * ((len(sel) + 3) // 4)))
for i, f in enumerate(sel):
    im = Image.open(f).resize((W, H))
    ImageDraw.Draw(im).text((6, 4), f"{int(os.path.basename(f)[1:7]) // 30}s", fill=(255, 0, 255))
    out.paste(im, ((i % 4) * W, (i // 4) * H))
out.save(sys.argv[5] if len(sys.argv) > 5 else "folha.jpg", quality=85)
