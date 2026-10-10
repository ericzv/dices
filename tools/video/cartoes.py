# Cartoes de tela cheia: titulos das partes, o desenho do mouse e o resumo.
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from gravador import *
from gravador import _sprite_mouse

def feltro():
    rng = np.random.default_rng(3)
    h, w = CAP_Y, OUT_W
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.hypot((xx - w / 2) / w, (yy - h * 0.45) / h)
    base = np.array([22, 74, 52], np.float32)
    luz = (1.15 - 0.9 * d)[..., None]
    img = base * luz + rng.normal(0, 1.6, (h, w, 1))
    felt = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(felt)
    d.rectangle([22, 22, w - 23, h - 23], outline=(92, 62, 34), width=14)
    d.rectangle([36, 36, w - 37, h - 37], outline=OURO, width=4)
    im = Image.new("RGB", (OUT_W, OUT_H), FUNDO)
    im.paste(felt, (0, 0))
    return im

_FELTRO = None
def fundo_feltro():
    global _FELTRO
    if _FELTRO is None: _FELTRO = feltro()
    return _FELTRO.copy()

def texto_c(d, y, s, f, cor, sombra=True):
    w = f.getlength(s)
    if sombra: d.text(((OUT_W - w) / 2 + 5, y + 6), s, font=f, fill=(8, 20, 14))
    d.text(((OUT_W - w) / 2, y), s, font=f, fill=cor)

def titulo(parte, nome, sub=None):
    im = fundo_feltro()
    d = ImageDraw.Draw(im)
    y = 190 if sub else 250
    if parte: texto_c(d, y, parte.upper(), F_SUB, (190, 210, 196), sombra=False); y += 90
    texto_c(d, y, nome, F_TIT, OURO)
    if sub:
        y += 230
        for l in quebra(sub, F_SUB, 1500):
            texto_c(d, y, l, F_SUB, BRANCO, sombra=False); y += 70
    return im

def abertura():
    im = fundo_feltro()
    d = ImageDraw.Draw(im)
    texto_c(d, 170, "DADO EM CASA", fonte(JERSEY, 230), OURO)
    texto_c(d, 450, "Como jogar, passo a passo", F_SUB, BRANCO, sombra=False)
    texto_c(d, 540, "com o mouse  ·  sem pressa  ·  até o Modo Desafiante", F_SUB2, (190, 210, 196), sombra=False)
    return im

def mouse_cartao(passo):
    """passo 0: so o mouse; 1: o botao esquerdo aceso com a legenda; 2: + 'um clique'."""
    im = fundo_feltro()
    d = ImageDraw.Draw(im)
    texto_c(d, 70, "O MOUSE", F_TIT2, OURO)
    m = _sprite_mouse(280, botao=passo >= 1)
    mx, my = 620, 230
    im.paste(m, (mx, my), m)
    if passo >= 1:
        # seta do texto ate o botao esquerdo
        d.line([(470, my + 75), (mx + 90, my + 110)], fill=AMARELO, width=8)
        d.ellipse([mx + 78, my + 98, mx + 102, my + 122], fill=AMARELO)
        d.text((110, my + 10), "botão", font=F_SUB, fill=AMARELO)
        d.text((110, my + 70), "ESQUERDO", font=fonte(INTER + "Inter-Bold.otf", 62), fill=AMARELO)
        d.text((110, my + 160), "(é este que", font=F_SUB2, fill=BRANCO)
        d.text((110, my + 215), "você aperta)", font=F_SUB2, fill=BRANCO)
    if passo >= 2:
        x0 = 1110
        d.text((x0, 270), "CLICAR =", font=fonte(INTER + "Inter-Bold.otf", 70), fill=OURO)
        d.text((x0, 370), "apertar e soltar", font=F_SUB, fill=BRANCO)
        d.text((x0, 440), "uma vez, rapidinho.", font=F_SUB, fill=BRANCO)
        d.text((x0, 550), "Não precisa clicar", font=F_SUB2, fill=(190, 210, 196))
        d.text((x0, 605), "duas vezes.", font=F_SUB2, fill=(190, 210, 196))
    return im

def resumo(linhas, ativo=None, titulo_txt="RESUMO"):
    im = fundo_feltro()
    d = ImageDraw.Draw(im)
    texto_c(d, 55, titulo_txt, F_TIT2, OURO)
    y = 200
    fb = fonte(INTER + "Inter-SemiBold.otf", 42)
    for i, l in enumerate(linhas):
        aceso = ativo is None or i == ativo
        cor = BRANCO if aceso else (120, 150, 132)
        if ativo is not None and i == ativo:
            d.rounded_rectangle([140, y - 10, OUT_W - 140, y + 52 * len(quebra(l, fb, 1450)) + 6], radius=18,
                                fill=(14, 52, 36), outline=AMARELO, width=4)
        d.ellipse([170, y + 2, 226, y + 58], fill=OURO if aceso else (90, 110, 96))
        n = str(i + 1)
        w = F_ROT.getlength(n)
        d.text((198 - w / 2, y + 5), n, font=F_ROT, fill=(20, 30, 24))
        partes = quebra(l, fb, 1450)
        for k, p in enumerate(partes):
            d.text((255, y + 4 + k * 52), p, font=fb, fill=cor)
        y += 52 * len(partes) + 30
    return im
