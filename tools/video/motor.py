# O jogo de verdade (libmotor.so, feita de motor.c + src/games/dado.c) visto
# do Python: passo de tempo, teclas, mouse, o quadro desenhado e espiadas no
# estado de dentro, para o roteiro do video dirigir.
import ctypes as C, numpy as np, os
AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
KEY_BKSP, KEY_TAB, KEY_ENTER, KEY_ESC = 0x08, 0x09, 0x0D, 0x1B
KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT = 0x100, 0x101, 0x102, 0x103
W, H = 640, 360


class KeyEv(C.Structure):
    _fields_ = [("key", C.c_uint16)]


def compila():
    """libmotor.so: o jogo inteiro numa biblioteca, mais as espiadas de motor.c."""
    import subprocess
    versao = subprocess.run(["git", "-C", RAIZ, "rev-parse", "--short=7", "HEAD"],
                            capture_output=True, text=True).stdout.strip() or "dev"
    subprocess.run(["gcc", "-O2", "-fPIC", "-shared", "-o", os.path.join(AQUI, "libmotor.so"),
                    os.path.join(AQUI, "motor.c"), os.path.join(RAIZ, "src/ui/gfx.c"),
                    os.path.join(RAIZ, "src/ui/fonte.c"), os.path.join(RAIZ, "desktop/rede.c"),
                    "-I" + os.path.join(RAIZ, "src"), "-I" + os.path.join(RAIZ, "desktop"),
                    "-I" + os.path.join(RAIZ, "tests"), "-lm", f'-DDADO_VERSAO="{versao}"',
                    "-Wall", "-Wno-unused-parameter", "-Wno-unused-function"], check=True)


class Jogo:
    def __init__(self):
        if not os.path.exists(os.path.join(AQUI, "libmotor.so")): compila()
        L = C.CDLL(os.path.join(AQUI, "libmotor.so"))
        self.L = L
        for n in ["mv_anuncio_txt", "mv_aviso_txt", "mv_mao_nome", "mv_perso_nome", "mv_tipo_nome",
                  "mv_am_nome", "dado_dbg_veredito"]:
            getattr(L, n).restype = C.c_char_p
        L.mv_fb.restype = C.POINTER(C.c_uint16)
        L.mv_t_fase.restype = C.c_float
        L.mv_conselho_chance.restype = C.c_float
        L.dado_passo.argtypes = [C.c_float]
        L.mv_relogio.argtypes = [C.c_int64]
        L.mv_avanca.argtypes = [C.c_int64]
        self.fb = np.ctypeslib.as_array(L.mv_fb(), shape=(H * W,))
        self._sons = (C.c_int * 256)()
        self._z = (C.c_int * 7)()
        self._ord = (C.c_int * 8)()

    def __getattr__(self, n):          # jogo.mv_fase() etc.
        return getattr(self.L, n)

    def inicia(self, n=0, relogio=1):
        self.L.mv_relogio(relogio)
        self.L.dado_inicia(n)

    def passo(self, dt=1 / 60):
        self.L.mv_avanca(int(dt * 1e6))
        self.L.dado_passo(C.c_float(dt))

    def tecla(self, k):
        if isinstance(k, str): k = ord(k)
        self.L.dado_tecla(C.byref(KeyEv(k)))

    def mouse(self, x, y, acao=0):
        self.L.dado_mouse(int(x), int(y), int(acao))

    def desenha_so(self):
        """Desenha o quadro (e marca as zonas de clique) sem converter."""
        self.L.dado_desenha()

    def desenha(self):
        self.L.dado_desenha()
        c = self.fb.astype(np.uint32)
        r = ((c >> 11) & 0x1F) * 255 // 31
        g = ((c >> 5) & 0x3F) * 255 // 63
        b = (c & 0x1F) * 255 // 31
        return np.stack([r, g, b], axis=-1).astype(np.uint8).reshape(H, W, 3)

    def sons(self):
        n = self.L.mv_sons(self._sons)
        return list(self._sons[:n])

    def zonas(self):
        """(x, y, w, h, valor, tecla, variavel) de cada area clicavel da tela."""
        out = []
        for i in range(self.L.mv_n_zonas()):
            self.L.mv_zona(i, self._z)
            out.append(tuple(self._z))
        return out

    def conselho_fila(self):
        """A sequencia que a IA dificil montaria com a mao do jogador 0."""
        n = self.L.mv_conselho_fila(self._ord)
        return list(self._ord[:n])
