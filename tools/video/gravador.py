# Gravador do video tutorial: roda o jogo de verdade (libmotor.so), move um
# cursor de mouse desenhado, clica, destaca partes da tela, narra com legendas
# e grava tudo num MP4 de 1920x1080 com os sons do proprio jogo.
import os, re, math, random, subprocess, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from motor import *
import voz, sons

AQUI = os.path.dirname(os.path.abspath(__file__))
OUT_W, OUT_H = 1920, 1080
GS = 2.5                                  # escala da mesa: 640x360 -> 1600x900
GX, GY = 160, 0
CAP_Y, CAP_H = 900, 180
FPS = 30
SR = 48000

RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
# Inter (pacote fonts-inter) nas legendas; a Jersey 10 do jogo nos titulos.
INTER = os.environ.get("DADO_INTER", "/usr/share/fonts/opentype/inter/")
JERSEY = os.path.join(RAIZ, "tools", "fontes", "Jersey10-Regular.ttf")
def fonte(nome, tam): return ImageFont.truetype(nome, tam)
F_CAP = fonte(INTER + "Inter-SemiBold.otf", 52)
F_ROT = fonte(INTER + "Inter-Bold.otf", 40)
F_SUB = fonte(INTER + "Inter-Medium.otf", 52)
F_SUB2 = fonte(INTER + "Inter-Medium.otf", 44)
F_TIT = fonte(JERSEY, 170)
F_TIT2 = fonte(JERSEY, 120)
F_TECLA = fonte(INTER + "Inter-Bold.otf", 64)

AMARELO = (255, 214, 64)
BRANCO = (250, 248, 240)
FUNDO = (12, 14, 16)
OURO = (214, 178, 92)

# RGB565 -> RGB, tabela pronta
_c = np.arange(65536, dtype=np.uint32)
LUT = np.stack([((_c >> 11) & 31) * 255 // 31, ((_c >> 5) & 63) * 255 // 63, (_c & 31) * 255 // 31], -1).astype(np.uint8)

# Pronuncia: o que a legenda mostra e o que a voz fala.
PRONUNCIA = [
    (r"\bruns\b", "rans"), (r"\brun\b", "ran"), (r"\bENTER\b", "Énter"), (r"\bEnter\b", "Énter"),
    (r"\bTAB\b", "tab"), (r"\bESC\b", "esc"), (r"\bX\b", "xis"), (r"\bok\b", "ókei"),
    (r"\bD4\b", "dê quatro"), (r"\bD6\b", "dê seis"), (r"\bD8\b", "dê oito"),
]
def para_voz(t):
    for a, b in PRONUNCIA: t = re.sub(a, b, t)
    return t


def g2o(x, y):
    """Coordenada da mesa (640x360) -> coordenada do video."""
    return GX + x * GS, GY + y * GS


def _sprite_cursor(h=56):
    # seta classica: branca com contorno preto e sombra
    pts = [(0, 0), (0, 17), (4.5, 13.2), (7.6, 20.2), (10.4, 19), (7.5, 12.3), (13, 12.3)]
    s = h / 20.2
    pad = 8
    W, H = int(13 * s) + 2 * pad, int(20.6 * s) + 2 * pad
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    P = [(pad + x * s, pad + y * s) for x, y in pts]
    sh = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(sh).polygon([(x + 3, y + 4) for x, y in P], fill=(0, 0, 0, 130))
    sh = sh.filter(ImageFilter.GaussianBlur(2.5))
    im.alpha_composite(sh)
    d.polygon(P, fill=(255, 255, 255, 255), outline=(0, 0, 0, 255), width=3)
    return im, (pad, pad)


def _sprite_mouse(w=110, botao=True):
    """Desenho de um mouse visto de cima, com o botao esquerdo aceso."""
    h = int(w * 1.55)
    im = Image.new("RGBA", (w + 12, h + 12), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    r = w // 2
    d.rounded_rectangle([6, 6, 6 + w, 6 + h], radius=r, fill=(235, 235, 232, 255), outline=(30, 30, 30, 255), width=5)
    meio = 6 + int(h * 0.42)
    if botao:  # botao esquerdo em amarelo
        esq = Image.new("RGBA", im.size, (0, 0, 0, 0))
        de = ImageDraw.Draw(esq)
        de.rounded_rectangle([6, 6, 6 + w, 6 + h], radius=r, fill=AMARELO + (255,))
        mask = Image.new("L", im.size, 0)
        ImageDraw.Draw(mask).rectangle([0, 0, 6 + w // 2, meio], fill=255)
        im.paste(esq, (0, 0), Image.composite(esq, Image.new("RGBA", im.size), mask).split()[3])
        d.rounded_rectangle([6, 6, 6 + w, 6 + h], radius=r, outline=(30, 30, 30, 255), width=5)
    d.line([6 + w // 2, 8, 6 + w // 2, meio], fill=(30, 30, 30, 255), width=4)
    d.line([8, meio, 4 + w, meio], fill=(30, 30, 30, 255), width=4)
    d.rounded_rectangle([6 + w // 2 - 7, 6 + int(h * 0.12), 6 + w // 2 + 7, 6 + int(h * 0.27)], radius=6,
                        fill=(120, 120, 120, 255), outline=(30, 30, 30, 255), width=3)
    return im


def _pilula(texto, f=F_ROT, cor=AMARELO, fundo=(20, 20, 20, 235), pad=(22, 10)):
    l, t, r, b = f.getbbox(texto)
    w, h = r - l + 2 * pad[0], b - t + 2 * pad[1]
    im = Image.new("RGBA", (w + 8, h + 8), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([4, 4, 4 + w, 4 + h], radius=h // 2, fill=fundo, outline=cor + (255,), width=4)
    d.text((4 + pad[0] - l, 4 + pad[1] - t), texto, font=f, fill=cor + (255,))
    return im


def _sprite_tecla(rot):
    l, t, r, b = F_TECLA.getbbox(rot)
    w = max(110, r - l + 50)
    h = 110
    im = Image.new("RGBA", (w + 10, h + 14), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([5, 9, 5 + w, 9 + h], radius=16, fill=(40, 40, 44, 255))
    d.rounded_rectangle([5, 3, 5 + w, 3 + h - 10], radius=16, fill=(240, 238, 230, 255), outline=(40, 40, 44, 255), width=4)
    d.text((5 + (w - (r - l)) / 2 - l, 3 + (h - 10 - (b - t)) / 2 - t), rot, font=F_TECLA, fill=(30, 30, 30, 255))
    return im


def quebra(texto, f, larg):
    linhas, atual = [], ""
    for p in texto.split():
        t = (atual + " " + p).strip()
        if f.getlength(t) <= larg: atual = t
        else:
            if atual: linhas.append(atual)
            atual = p
    if atual: linhas.append(atual)
    return linhas


class Gravador:
    def __init__(self, saida, dry=False, preview=None, so_audio=False):
        self.saida = saida
        self.dry = dry
        self.preview = preview          # (pasta, a cada N quadros) para revisar sem gravar
        self.j = Jogo()
        self.f = 0
        self.cur = [320.0, 200.0]
        self.cur_vis = False
        self.mouse_jogo = None
        self.cliques = []                # (f0, x, y) em coordenadas do video
        self.destaques = {}
        self.n_dest = 0
        self.legendas = []               # (f0, f1, texto)
        self.falas = []                  # (f0, amostras)
        self.fala_fim = 0
        self.sfx = []
        self.rufo = []
        self.giro = []
        self.mus = []                    # volume da trilha por quadro (0..1)
        self.vol_mus = 1.0
        self.cartao_img = None
        self.congelado = False
        self.vel = 1
        self.mudo = False
        self.teclas = []                 # (f0, sprite)
        self.selos = {}                  # rotulos fixos na tela (acelerado etc.)
        self.log = []
        self.capitulos = []              # (quadro, titulo) para os capitulos do MP4
        self.cursor_sprite, self.cursor_hot = _sprite_cursor()
        self.mouse_sprite = _sprite_mouse(70)
        self._leg_cache = {}
        self.fundo = Image.new("RGB", (OUT_W, OUT_H), FUNDO)
        self.ff = None
        if not dry and not preview:
            self.ff = subprocess.Popen(
                ["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{OUT_W}x{OUT_H}",
                 "-r", str(FPS), "-i", "-", "-c:v", "libx264", "-preset", "medium", "-crf", "20", "-tune", "animation",
                 "-pix_fmt", "yuv420p", "-g", "300", "-keyint_min", "30", saida + ".video.mp4"], stdin=subprocess.PIPE)

    # ------------------------------------------------------------------ tempo
    def quadro(self):
        j = self.j
        if not self.congelado:
            for _ in range(2 * self.vel): j.passo(1 / 60)
        if self.cur_vis:
            m = (int(round(self.cur[0])), int(round(self.cur[1])))
            if m != self.mouse_jogo:
                j.mouse(m[0], m[1], 0)
                self.mouse_jogo = m
        for s in j.sons():
            if not self.mudo and self.vel == 1: self.sfx.append((self.f, s))
        mudo = self.mudo or self.vel > 1 or self.congelado
        self.rufo.append(0 if mudo else j.mv_rufo())
        self.giro.append(0 if mudo else j.mv_giro())
        self.mus.append(self.vol_mus)
        j.desenha_so()
        if not self.dry and (self.ff or (self.preview and self.f % self.preview[1] == 0)):
            quadro = self.compoe()
            if self.ff: self.ff.stdin.write(quadro.tobytes())
            else: quadro.save(os.path.join(self.preview[0], f"q{self.f:06d}.jpg"), quality=80)
        self.f += 1

    def pausa(self, seg):
        for _ in range(int(round(seg * FPS))): self.quadro()

    def ate(self, pred, maxs=60, nome=""):
        for _ in range(int(maxs * FPS)):
            if pred(): return
            self.quadro()
        raise RuntimeError(f"esperei demais: {nome} (fase {self.j.mv_fase()})")

    # --------------------------------------------------------------- narracao
    def diz(self, texto, espera=False, gap=0.45):
        """Enfileira a narracao (frase por frase, cada uma com sua legenda)."""
        frases = [f.strip() for f in re.split(r"(?<=[.!?…])\s+", texto.strip()) if f.strip()]
        # frase muito curta soa mal sozinha na voz: vai junto com a seguinte
        juntas = []
        for fr in frases:
            if juntas and len(juntas[-1]) < 24: juntas[-1] += " " + fr
            else: juntas.append(fr)
        frases = juntas
        ini = max(self.f, self.fala_fim)
        for fr in frases:
            amostras, dur = voz.fala(para_voz(fr))
            f0 = ini
            f1 = f0 + int(math.ceil(dur * FPS))
            self.falas.append((f0, amostras))
            # legenda: no maximo 2 linhas por vez; frase longa vira pedacos
            partes = self._pedacos(fr)
            tot = sum(len(p) for p in partes)
            fa = f0
            for i, p in enumerate(partes):
                fb = f1 if i == len(partes) - 1 else fa + int((f1 - f0) * len(p) / tot)
                self.legendas.append((fa, fb, p))
                fa = fb
            ini = f1 + int(gap * FPS)
            self.log.append((f0 / FPS, fr))
        self.fala_fim = ini
        if espera: self.espera_fala()

    def _pedacos(self, fr):
        larg = OUT_W - 160
        cabe = lambda t: len(quebra(t, F_CAP, larg)) <= 2
        if cabe(fr): return [fr]
        # corta nas virgulas e dois-pontos; junta o que couber em 2 linhas
        trechos = [t for t in re.split(r"(?<=[,:;])\s+", fr) if t]
        partes = []
        for t in trechos:
            if partes and cabe(partes[-1] + " " + t): partes[-1] += " " + t
            elif cabe(t): partes.append(t)
            else:
                linhas = quebra(t, F_CAP, larg)
                for i in range(0, len(linhas), 2): partes.append(" ".join(linhas[i:i + 2]))
        return partes

    def espera_fala(self, extra=0.4):
        alvo = self.fala_fim - int(0.45 * FPS) + int(extra * FPS)
        while self.f < alvo: self.quadro()

    # ------------------------------------------------------------------ mouse
    def mostra_cursor(self, x=None, y=None):
        if x is not None: self.cur = [float(x), float(y)]
        self.cur_vis = True

    def esconde_cursor(self):
        self.cur_vis = False

    def move(self, x, y, dur=None):
        x0, y0 = self.cur
        dist = math.hypot(x - x0, y - y0)
        if dur is None: dur = min(1.5, max(0.6, 0.45 + dist / 380))
        n = max(1, int(dur * FPS))
        self.cur_vis = True
        for i in range(1, n + 1):
            t = i / n
            e = t * t * (3 - 2 * t)
            # um arco leve, como a mao de verdade
            arco = math.sin(t * math.pi) * min(18, dist * 0.06)
            self.cur = [x0 + (x - x0) * e, y0 + (y - y0) * e - arco]
            self.quadro()
        self.cur = [float(x), float(y)]

    def zona(self, var=None, val=None, tecla=None):
        self.j.desenha()
        for z in self.j.zonas():
            if (var is None or z[6] == var) and (val is None or z[4] == val) and (tecla is None or z[5] == tecla):
                return z[:4]
        raise RuntimeError(f"zona nao achada var={var} val={val} tecla={tecla} fase={self.j.mv_fase()} {self.j.zonas()}")

    def vai(self, var=None, val=None, tecla=None, dur=None, dx=0, dy=0):
        x, y, w, h = self.zona(var, val, tecla)
        self.move(x + w / 2 + dx, y + h / 2 + dy, dur)
        return (x, y, w, h)

    def clica(self, espera=0.35):
        self.pausa(0.15)
        x, y = g2o(*self.cur)
        self.cliques.append((self.f, x, y))
        m = (int(round(self.cur[0])), int(round(self.cur[1])))
        self.j.mouse(m[0], m[1], 0)
        self.j.mouse(m[0], m[1], 1)
        self.mouse_jogo = m
        self.pausa(espera)

    def clica_em(self, var=None, val=None, tecla=None, dur=None, espera=0.35, antes=0.25):
        r = self.vai(var, val, tecla, dur)
        self.pausa(antes)
        self.clica(espera)
        return r

    def digita(self, k, rot=None):
        if rot is None: rot = k if isinstance(k, str) else "?"
        self.teclas.append((self.f, _sprite_tecla(rot)))
        self.j.tecla(k)
        self.pausa(0.5)

    # ------------------------------------------------------------- destaques
    def destaca(self, x, y, w, h, rotulo=None, dur=None, pos="cima", margem=4, forma="ret"):
        self.n_dest += 1
        self.destaques[self.n_dest] = dict(r=(x - margem, y - margem, w + 2 * margem, h + 2 * margem), f0=self.f,
                                           f1=self.f + int(dur * FPS) if dur else None,
                                           rot=_pilula(rotulo) if rotulo else None, pos=pos, forma=forma)
        return self.n_dest

    def destaca_zona(self, var=None, val=None, tecla=None, rotulo=None, dur=None, pos="cima", margem=4):
        x, y, w, h = self.zona(var, val, tecla)
        return self.destaca(x, y, w, h, rotulo, dur, pos, margem)

    def apaga(self, *ids):
        if not ids: self.destaques.clear()
        for i in ids: self.destaques.pop(i, None)

    def selo(self, chave, texto=None, pos=(0, 0)):
        if texto is None: self.selos.pop(chave, None)
        else: self.selos[chave] = (_pilula(texto), pos)

    # ---------------------------------------------------------------- cartoes
    def cartao(self, img):
        """Mostra uma imagem de tela cheia (titulo, desenho) no lugar do jogo."""
        self.cartao_img = img

    def capitulo(self, titulo):
        self.capitulos.append((self.f, titulo))

    def tira_cartao(self):
        self.cartao_img = None

    # --------------------------------------------------------------- composicao
    def _legenda_img(self, texto):
        if texto in self._leg_cache: return self._leg_cache[texto]
        im = Image.new("RGB", (OUT_W, CAP_H), FUNDO)
        d = ImageDraw.Draw(im)
        linhas = quebra(texto, F_CAP, OUT_W - 160)[:2]
        alt = 68
        y0 = (CAP_H - alt * len(linhas)) // 2 - 4
        for i, l in enumerate(linhas):
            w = F_CAP.getlength(l)
            d.text(((OUT_W - w) / 2, y0 + i * alt), l, font=F_CAP, fill=BRANCO)
        self._leg_cache[texto] = im
        return im

    def compoe(self):
        if self.cartao_img is not None:
            tela = self.cartao_img.copy()
        else:
            tela = self.fundo.copy()
            jogo = Image.fromarray(LUT[self.j.fb].reshape(360, 640, 3)).resize((1600, 900), Image.BOX)
            tela.paste(jogo, (GX, GY))
            d = ImageDraw.Draw(tela)
            # destaques
            for k, h in list(self.destaques.items()):
                if h["f1"] is not None and self.f >= h["f1"]:
                    del self.destaques[k]; continue
                x, y, w, hh = h["r"]
                x0, y0 = g2o(x, y)
                x1, y1 = g2o(x + w, y + hh)
                t = (self.f - h["f0"]) / FPS
                pul = 0.5 + 0.5 * math.sin(t * 5)
                larg = int(5 + 3 * pul)
                cor = tuple(int(c * (0.75 + 0.25 * pul)) for c in AMARELO)
                if h["forma"] == "circ":
                    d.ellipse([x0, y0, x1, y1], outline=cor, width=larg)
                else:
                    d.rounded_rectangle([x0, y0, x1, y1], radius=14, outline=(0, 0, 0), width=larg + 4)
                    d.rounded_rectangle([x0, y0, x1, y1], radius=14, outline=cor, width=larg)
                if h["rot"] is not None:
                    r = h["rot"]
                    rx = int((x0 + x1) / 2 - r.width / 2)
                    rx = max(GX + 6, min(GX + 1600 - r.width - 6, rx))
                    ry = int(y0 - r.height - 8) if h["pos"] == "cima" else int(y1 + 8)
                    if ry < 4: ry = int(y1 + 8)
                    if ry + r.height > CAP_Y - 4: ry = int(y0 - r.height - 8)
                    if h["pos"] == "esq":
                        rx, ry = int(x0 - r.width - 10), int((y0 + y1) / 2 - r.height / 2)
                    tela.paste(r, (rx, ry), r)
            # selos fixos
            for k, (s, pos) in self.selos.items():
                tela.paste(s, pos, s)
            # cliques: anel que cresce e some
            for c in list(self.cliques):
                t = (self.f - c[0]) / FPS
                if t > 0.55: continue
                q = t / 0.55
                r = 14 + 46 * q
                w = max(1, int(9 * (1 - q)))
                d.ellipse([c[1] - r, c[2] - r, c[1] + r, c[2] + r], outline=AMARELO, width=w)
            # cursor
            if self.cur_vis:
                x, y = g2o(*self.cur)
                tela.paste(self.cursor_sprite, (int(x - self.cursor_hot[0]), int(y - self.cursor_hot[1])), self.cursor_sprite)
                # no clique, um mouse com o botao esquerdo aceso ao lado da seta
                for c in self.cliques:
                    t = (self.f - c[0]) / FPS
                    if 0 <= t < 0.7:
                        mx = int(x + 46) if x < GX + 1500 else int(x - 46 - self.mouse_sprite.width)
                        tela.paste(self.mouse_sprite, (mx, int(y + 10)), self.mouse_sprite)
                        break
            # teclas apertadas
            for f0, s in self.teclas:
                t = (self.f - f0) / FPS
                if 0 <= t < 0.9:
                    tela.paste(s, (GX + 1600 - s.width - 30, CAP_Y - s.height - 30), s)
        # legenda
        leg = None
        for f0, f1, tx in reversed(self.legendas):
            if f0 <= self.f < f1 + int(0.6 * FPS):
                leg = tx; break
            if f1 + int(0.6 * FPS) < self.f - 5000: break
        if leg: tela.paste(self._legenda_img(leg), (0, CAP_Y))
        elif self.cartao_img is None:
            ImageDraw.Draw(tela).rectangle([0, CAP_Y, OUT_W, OUT_H], fill=FUNDO)
        return tela

    # ------------------------------------------------------------------ audio
    def mixa_audio(self):
        n = int(self.f / FPS * SR) + SR
        mix = np.zeros((n, 2), np.float32)
        voz_mask = np.zeros(n, np.float32)
        for f0, a in self.falas:
            i = int(f0 / FPS * SR)
            m = min(len(a), n - i)
            mix[i:i + m] += a[:m, None] * 1.0
            voz_mask[i:i + m] = 1
        rng = random.Random(7)
        cache_var = {}
        for f, s in self.sfx:
            if s not in cache_var: cache_var[s] = sons.variantes(s)
            vs = cache_var[s]
            if not vs: continue
            a = vs[rng.randrange(len(vs))]
            vol = 0.45
            if s == 0 or s == 31: vol = 0.25      # tique e passa: baixinhos
            i = int(f / FPS * SR)
            m = min(len(a), n - i)
            if m > 0: mix[i:i + m] += a[:m] * vol
        # loops: dado rolando e roleta
        def env(estados, ataque, solta):
            e = np.zeros(len(estados) + 1, np.float32)
            v = 0.0
            for k, st in enumerate(estados):
                alvo = 1.0 if st else 0.0
                v += (alvo - v) * min(1, (1 / FPS) / (ataque if alvo > v else solta))
                e[k] = v
            return np.interp(np.arange(n) / SR * FPS, np.arange(len(e)), e).astype(np.float32)
        for estados, nome, vol in [(self.rufo, "rufo", 0.42), (self.giro, "giro", 0.45)]:
            if not any(estados): continue
            e = env(estados, 0.06, 0.12)
            loop = sons.le(nome)
            reps = n // len(loop) + 1
            mix += np.tile(loop, (reps, 1))[:n] * (e * vol)[:, None]
        # trilha: baixa sob a voz, mais baixa com o dado rolando
        tr = sons.le("trilha")
        reps = n // len(tr) + 1
        trilha = np.tile(tr, (reps, 1))[:n]
        k = int(0.3 * SR)
        voz_s = np.convolve(voz_mask, np.ones(k) / k, mode="same")
        mus = np.interp(np.arange(n) / SR * FPS, np.arange(len(self.mus)), np.array(self.mus, np.float32))
        rol = env([a or b for a, b in zip(self.rufo, self.giro)], 0.25, 0.25)
        vol = 0.22 * mus * (1 - 0.65 * np.clip(voz_s, 0, 1)) * (1 - 0.5 * rol)
        mix += trilha * vol[:, None]
        pico = np.abs(mix).max()
        if pico > 0.98: mix *= 0.98 / pico
        import soundfile as sf
        sf.write(self.saida + ".wav", mix, SR)

    def fecha(self):
        if self.ff:
            self.ff.stdin.close()
            self.ff.wait()
        if not self.dry:
            self.mixa_audio()
        if self.ff:
            meta = self.saida + ".meta.txt"
            fim = int(self.f / FPS * 1000)
            with open(meta, "w") as m:
                m.write(";FFMETADATA1\ntitle=Dado em Casa - como jogar, passo a passo\n")
                caps = self.capitulos
                for i, (f0, t) in enumerate(caps):
                    f1 = caps[i + 1][0] if i + 1 < len(caps) else self.f
                    m.write(f"[CHAPTER]\nTIMEBASE=1/1000\nSTART={int(f0 / FPS * 1000)}\nEND={int(f1 / FPS * 1000)}\ntitle={t}\n")
            subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", self.saida + ".video.mp4", "-i", self.saida + ".wav",
                            "-i", meta, "-map", "0:v", "-map", "1:a", "-map_metadata", "2", "-map_chapters", "2",
                            "-c:v", "copy", "-af", "loudnorm=I=-16:TP=-1.5:LRA=11", "-c:a", "aac", "-b:a", "160k",
                            "-ar", "48000", "-movflags", "+faststart", self.saida], check=True)
        with open(self.saida + ".capitulos.txt", "w") as f:
            for f0, t in self.capitulos:
                s = f0 / FPS
                f.write(f"{int(s // 60):02d}:{int(s % 60):02d}  {t}\n")
        with open(self.saida + ".roteiro.txt", "w") as f:
            for t, fr in self.log:
                f.write(f"{int(t // 60):02d}:{t % 60:05.2f}  {fr}\n")
