#!/usr/bin/env python3
"""Gera build/lexico.pack: dicionario portugues como DAWG + solucao do jogo.

Tres etapas:

1. Normaliza a lista (tira acento, caixa, hifen) e filtra por tamanho.
2. Minimiza num DAWG (automato aciclico deterministico minimo) pelo algoritmo
   incremental de Daciuk: estados equivalentes sao compartilhados, o que
   derruba o numero de nos em uma ordem de grandeza frente a uma trie.
3. Resolve o Fantasma por inteiro. Para cada no e cada comprimento de prefixo
   0..15, calcula se quem esta para jogar ganha. Sao 16 bits por no, gravados
   junto - assim a maquina joga perfeito sem pensar em tempo de execucao.

DOIS dicionarios no mesmo automato, porque o Fantasma tem duas exigencias
contrarias:

  LEGALIDADE  a lista inteira, 256 mil verbetes. Serve para decidir se a
              letra que voce escreveu continua alguma palavra, e para julgar
              um desafio. Com ela voce quase nunca leva um "nao continua"
              injusto.
  DERROTA     so o vocabulario comum, cruzado com frequencia de uso. Voce so
              PERDE ao fechar uma palavra que as pessoas conhecem. Fechar
              "bafabaf" sem querer nao conta.

Cada aresta carrega dois bits de fim: um para cada lista. A solucao do jogo e
calculada sobre a derrota (comum) percorrendo os lances da legalidade (todas).

Uso:  python3 tools/build_lexico.py [--top N]
"""
import sys
import struct
import unicodedata
import zlib

MIN_LEN, MAX_LEN = 3, 15
MIN_PALAVRA = 4          # so conta como palavra completa a partir daqui
MAX_PREFIXO = 16         # comprimentos resolvidos: 0..15
MAGIC = b"LEX2"


def normaliza(linha):
    p = unicodedata.normalize("NFD", linha.strip().lower())
    p = "".join(c for c in p if unicodedata.category(c) != "Mn")
    if not p.isalpha() or not p.isascii():
        return None
    if not (MIN_LEN <= len(p) <= MAX_LEN):
        return None
    return p


class No:
    __slots__ = ("final", "comum", "filhos", "id")
    _seq = 0

    def __init__(self):
        self.final = False      # existe na lista grande
        self.comum = False      # existe no vocabulario comum
        self.filhos = {}
        No._seq += 1
        self.id = No._seq

    def chave(self):
        return (self.final, self.comum,
                tuple((c, f.id) for c, f in sorted(self.filhos.items())))


class DAWG:
    def __init__(self):
        self.raiz = No()
        self.registro = {}
        self.anterior = ""
        self.pilha = []          # (pai, letra, filho) nao minimizados ainda

    def insere(self, palavra, comum=False):
        if palavra <= self.anterior:
            raise ValueError("a lista precisa estar ordenada")
        # prefixo comum com a palavra anterior
        comum = 0
        while (comum < len(palavra) and comum < len(self.anterior)
               and palavra[comum] == self.anterior[comum]):
            comum += 1
        self._minimiza(comum)

        no = self.pilha[-1][2] if self.pilha else self.raiz
        for c in palavra[comum:]:
            novo = No()
            no.filhos[c] = novo
            self.pilha.append((no, c, novo))
            no = novo
        no.final = True
        if comum:
            no.comum = True
        self.anterior = palavra

    def _minimiza(self, ate):
        while len(self.pilha) > ate:
            pai, letra, filho = self.pilha.pop()
            k = filho.chave()
            existente = self.registro.get(k)
            if existente is not None:
                pai.filhos[letra] = existente
            else:
                self.registro[k] = filho

    def termina(self):
        self._minimiza(0)


def ordem_topologica(raiz):
    """Filhos antes dos pais. Evita recursao profunda em 120 mil nos."""
    visto, saida, pilha = set(), [], [(raiz, False)]
    while pilha:
        no, processado = pilha.pop()
        if processado:
            saida.append(no)
            continue
        if id(no) in visto:
            continue
        visto.add(id(no))
        pilha.append((no, True))
        for f in no.filhos.values():
            if id(f) not in visto:
                pilha.append((f, False))
    return saida


def resolve(raiz, nos):
    """Para cada no e comprimento de prefixo, quem joga agora ganha?

    Perde quem escreve a letra que fecha uma palavra COMUM de 4+, e quem nao
    tem nenhuma letra legal para jogar. Os lances possiveis sao os da lista
    grande - e por isso que voce nunca fica sem saida injustamente.
    """
    tabela = {}
    for no in ordem_topologica(raiz):     # filhos ja resolvidos
        m = 0
        for tam in range(MAX_PREFIXO):
            if tam >= MAX_PREFIXO - 1:
                continue                  # prefixo no limite: quem jogar perde
            ganha = False
            for filho in no.filhos.values():
                if filho.comum and tam + 1 >= MIN_PALAVRA:
                    continue              # fecharia palavra conhecida
                if not (tabela.get(id(filho), 0) >> (tam + 1)) & 1:
                    ganha = True
                    break
            if ganha:
                m |= 1 << tam
        tabela[id(no)] = m
    return tabela


def coleta(raiz):
    vistos, ordem, pilha = set(), [], [raiz]
    while pilha:
        n = pilha.pop()
        if id(n) in vistos:
            continue
        vistos.add(id(n))
        ordem.append(n)
        pilha.extend(n.filhos.values())
    return ordem


def conta_palavras(raiz):
    """Quantas palavras COMUNS saem de cada no - e o medidor de tensao da tela.

    Contar as 256 mil daria um numero sem significado: o que aperta o jogo e
    quantas palavras de verdade ainda cabem. Satura em 16 bits.
    """
    memo = {}
    for no in ordem_topologica(raiz):
        t = 1 if no.comum else 0
        for f in no.filhos.values():
            t += memo[id(f)]
        memo[id(no)] = t if t < 0xFFFF else 0xFFFF
    return memo


def serializa(raiz, nos, vitoria, quantos):
    """Cada no vira um bloco contiguo de arestas.

    Aresta (4 bytes): letra 5 | fim comum 1 | ultimo irmao 1 | fim grande 1
                      | destino 24
    O destino e o indice da PRIMEIRA aresta do no filho, ou 0 se for folha.
    O indice 0 e reservado para esse sentinela, entao as arestas de verdade
    comecam em 1 e uma aresta morta ocupa a posicao zero.
    Um vetor paralelo de 16 bits por aresta guarda a solucao do filho.
    """
    bloco = {}          # id(no) -> indice da primeira aresta
    arestas = []
    solucao = []

    ordem = [raiz] + [n for n in nos if n is not raiz and n.filhos]
    arestas.append(None)            # sentinela na posicao 0
    solucao.append(0)
    for no in ordem:
        if not no.filhos:
            continue
        bloco[id(no)] = len(arestas)
        arestas.extend([None] * len(no.filhos))
        solucao.extend([0] * len(no.filhos))

    for no in ordem:
        if not no.filhos:
            continue
        base = bloco[id(no)]
        itens = sorted(no.filhos.items())
        for i, (c, filho) in enumerate(itens):
            arestas[base + i] = (c, filho, i == len(itens) - 1)

    dados = bytearray(4)            # a aresta morta do indice 0
    contagem = [0]
    for i, aresta in enumerate(arestas):
        if aresta is None:
            continue
        c, filho, ultimo = aresta
        destino = bloco.get(id(filho), 0)
        if destino > 0xFFFFFF:
            raise RuntimeError("DAWG grande demais para 24 bits")
        v = ((ord(c) - 97)
             | (0x20 if filho.comum else 0)
             | (0x40 if ultimo else 0)
             | (0x80 if filho.final else 0))
        dados += struct.pack("<I", v | (destino << 8))
        solucao[i] = vitoria[id(filho)]
        contagem.append(quantos[id(filho)])

    sol = b"".join(struct.pack("<H", s) for s in solucao)
    cnt = b"".join(struct.pack("<H", c) for c in contagem)
    return bytes(dados), sol, cnt, bloco[id(raiz)], len(arestas)


def carrega(top):
    # Lista grande: decide o que e lance legal e o que vale num desafio.
    grande = set()
    for l in open("build/palavras.txt", encoding="utf-8", errors="ignore"):
        w = normaliza(l)
        if w:
            grande.add(w)

    # Vocabulario comum: so este faz voce perder ao fechar palavra.
    comuns = []
    for l in open("build/freq.txt", encoding="utf-8", errors="ignore"):
        parte = l.split()
        if len(parte) != 2:
            continue
        w = normaliza(parte[0])
        if w and w in grande:
            comuns.append(w)
        if len(comuns) >= top:
            break
    comuns = set(comuns)
    print(f"  legalidade {len(grande)} palavras | derrota {len(comuns)}")
    return sorted(grande), comuns


def main():
    top = 16000
    if "--top" in sys.argv:
        top = int(sys.argv[sys.argv.index("--top") + 1])
    palavras, comuns = carrega(top)

    d = DAWG()
    for p in palavras:
        d.insere(p, p in comuns)
    d.termina()

    nos = coleta(d.raiz)
    print(f"  DAWG: {len(nos)} nos")

    vitoria = resolve(d.raiz, nos)
    quantos = conta_palavras(d.raiz)
    arestas, sol, cnt, raiz_idx, n_arestas = serializa(d.raiz, nos, vitoria, quantos)
    print(f"  {n_arestas} arestas: {len(arestas)} B + solucao {len(sol)} B"
          f" + contagem {len(cnt)} B")

    corpo = arestas + sol + cnt
    hdr = bytearray(32)
    struct.pack_into("<4sHHIIIII", hdr, 0, MAGIC, 2, MIN_PALAVRA,
                     zlib.crc32(corpo) & 0xFFFFFFFF, len(corpo),
                     raiz_idx, n_arestas, len(comuns))
    with open("build/lexico.pack", "wb") as f:
        f.write(hdr)
        f.write(corpo)
    total = len(hdr) + len(corpo)
    print(f"  build/lexico.pack: {total} B ({total/1024:.0f} KB)")


if __name__ == "__main__":
    main()
