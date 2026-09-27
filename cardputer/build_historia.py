#!/usr/bin/env python3
"""Compila historia/travessia.yaml em build/historia.pack.

O trabalho interessante nao e serializar: e resolver o grafo.

Para o padre ser infalivel, ele nao pode estar lendo falas que eu escrevi -
uma frase escrita a mao envelhece na primeira vez que a historia muda. Entao
o compilador percorre o grafo a partir de CADA opcao e grava:

  - quais finais continuam alcancaveis por ali (mascara de bits);
  - se todos os caminhos terminam em final ruim ("nao tem volta");
  - a menor distancia ate um final.

O padre no aparelho so le essa tabela. Ele nao opina, informa - e por isso
nao pode errar. Se eu mudar uma cena, a fala dele muda junto, sozinha.

A travessia do grafo ignora condicoes de estado: ela responde "por onde da
para chegar", nao "com o seu inventario atual". O padre fala de estrutura,
que e o que ele teria como saber.
"""
import struct
import sys
import zlib

try:
    import yaml
except ImportError:
    sys.exit("falta pyyaml:  pip3 install pyyaml --break-system-packages")

MAGIC = b"HST1"
MAX_FINAIS = 32
ENTRADA = "historia/travessia.yaml"
SAIDA = "build/historia.pack"

# tipos de condicao
C_TEM, C_NAO_TEM, C_FLAG, C_NAO_FLAG, C_STAT_MIN, C_STAT_MAX, C_PERSONAGEM, \
    C_TEM_ARMA, C_NAO_PERSONAGEM = range(9)
# tipos de efeito
E_GANHA, E_PERDE, E_FLAG, E_LIMPA_FLAG, E_STAT = range(5)

STATS = ["confianca", "suspeita", "fe", "moeda"]


class Erro(Exception):
    pass


class Pool:
    """Textos deduplicados; offsets em 16 bits."""

    def __init__(self):
        self.buf = bytearray(b"\x00")
        self.idx = {"": 0}

    def add(self, s):
        s = (s or "").rstrip("\n")
        if s in self.idx:
            return self.idx[s]
        off = len(self.buf)
        if off > 0xFFFF:
            raise Erro("textos passaram de 64 KB")
        self.buf += s.encode("latin-1", "replace") + b"\x00"
        self.idx[s] = off
        return off


def parse_cond(expr, itens, personagens, cena):
    """'tem:faca' / 'nao_tem:x' / 'flag:y' / 'nao_flag:y' / 'fe>=2' / ..."""
    if expr == "tem_arma":
        return (C_TEM_ARMA, 0, 0)
    for op, tipo in (("nao_tem:", C_NAO_TEM), ("tem:", C_TEM)):
        if expr.startswith(op):
            nome = expr[len(op):]
            if nome not in itens:
                raise Erro(f"{cena}: item desconhecido '{nome}'")
            return (tipo, itens[nome], 0)
    for op, tipo in (("nao_flag:", C_NAO_FLAG), ("flag:", C_FLAG)):
        if expr.startswith(op):
            return (tipo, -1, expr[len(op):])
    if expr.startswith("nao_personagem:"):
        nome = expr[len("nao_personagem:"):]
        if nome not in personagens:
            raise Erro(f"{cena}: personagem desconhecido '{nome}'")
        return (C_NAO_PERSONAGEM, personagens[nome], 0)
    if expr.startswith("personagem:"):
        nome = expr[len("personagem:"):]
        if nome not in personagens:
            raise Erro(f"{cena}: personagem desconhecido '{nome}'")
        return (C_PERSONAGEM, personagens[nome], 0)
    for op, tipo in ((">=", C_STAT_MIN), ("<=", C_STAT_MAX)):
        if op in expr:
            chave, valor = expr.split(op)
            chave = chave.strip()
            if chave not in STATS:
                raise Erro(f"{cena}: stat desconhecido '{chave}'")
            return (tipo, STATS.index(chave), int(valor))
    raise Erro(f"{cena}: condicao ilegivel '{expr}'")


def parse_efeito(expr, itens, cena):
    for op, tipo in (("ganha:", E_GANHA), ("perde:", E_PERDE)):
        if expr.startswith(op):
            nome = expr[len(op):]
            if nome not in itens:
                raise Erro(f"{cena}: item desconhecido '{nome}'")
            return (tipo, itens[nome], 0)
    if expr.startswith("flag:"):
        return (E_FLAG, -1, expr[len("flag:"):])
    if expr.startswith("limpa_flag:"):
        return (E_LIMPA_FLAG, -1, expr[len("limpa_flag:"):])
    for sinal in ("+", "-"):
        if sinal in expr:
            chave, valor = expr.split(sinal)
            chave = chave.strip()
            if chave not in STATS:
                raise Erro(f"{cena}: stat desconhecido '{chave}'")
            v = int(valor)
            return (E_STAT, STATS.index(chave), v if sinal == "+" else -v)
    raise Erro(f"{cena}: efeito ilegivel '{expr}'")


def resolve_oraculo(cenas, ordem, finais):
    """Para cada cena: mascara de finais alcancaveis e menor distancia."""
    idx = {c["id"]: i for i, c in enumerate(ordem)}
    alcanca = [0] * len(ordem)
    dist = [0xFFFF] * len(ordem)

    for i, c in enumerate(ordem):
        if c.get("final"):
            alcanca[i] = 1 << finais[c["final"]]
            dist[i] = 0

    # Ponto fixo: o grafo e pequeno, entao iterar ate estabilizar e suficiente
    # e dispensa tratar ciclos como caso especial.
    mudou = True
    voltas = 0
    while mudou:
        mudou = False
        voltas += 1
        if voltas > 200:
            raise Erro("o grafo nao estabilizou; ha ciclo sem saida?")
        for i, c in enumerate(ordem):
            if c.get("final"):
                continue
            m, d = alcanca[i], dist[i]
            for o in c.get("opcoes", []):
                j = idx[o["vai"]]
                m |= alcanca[j]
                if dist[j] != 0xFFFF and dist[j] + 1 < d:
                    d = dist[j] + 1
            if m != alcanca[i] or d != dist[i]:
                alcanca[i], dist[i] = m, d
                mudou = True
    return alcanca, dist


def main():
    doc = yaml.safe_load(open(ENTRADA, encoding="utf-8"))
    P = Pool()

    itens_ord = list(doc["itens"].keys())
    itens = {n: i for i, n in enumerate(itens_ord)}
    pers_ord = [p["id"] for p in doc["personagens"]]
    personagens = {n: i for i, n in enumerate(pers_ord)}
    finais_ord = [f["id"] for f in doc["finais"]]
    finais = {n: i for i, n in enumerate(finais_ord)}
    if len(finais_ord) > MAX_FINAIS:
        raise Erro("mais finais do que cabem na mascara de 32 bits")

    ordem = doc["cenas"]
    idx_cena = {c["id"]: i for i, c in enumerate(ordem)}

    # --- validacao: destino inexistente e o erro que mais acontece ---------
    flags = {}
    problemas = []
    for c in ordem:
        if c.get("final") and c["final"] not in finais:
            problemas.append(f"{c['id']}: final desconhecido '{c['final']}'")
        for o in c.get("opcoes", []):
            if o["vai"] not in idx_cena:
                problemas.append(f"{c['id']}: '{o['txt']}' vai para "
                                 f"'{o['vai']}', que nao existe")
    # Toda cena precisa de pelo menos uma saida sem condicao, senao um estado
    # infeliz tranca o jogador sem nenhuma escolha na tela.
    for c in ordem:
        if c.get("final"):
            continue
        if not c.get("opcoes"):
            problemas.append(f"{c['id']}: cena sem saida e sem final")
        elif all(o.get("se") for o in c["opcoes"]):
            problemas.append(f"{c['id']}: todas as saidas tem condicao; "
                             "da para ficar preso")

    if problemas:
        for p in problemas:
            print("  [ERRO]", p)
        sys.exit("historia inconsistente; nada foi gravado")

    def flag_id(nome):
        if nome not in flags:
            flags[nome] = len(flags)
        return flags[nome]

    # --- monta condicoes, efeitos e opcoes --------------------------------
    conds, efeitos, opcoes = [], [], []
    for c in ordem:
        c["_op_ini"] = len(opcoes)
        for o in c.get("opcoes", []):
            ci = len(conds)
            for e in o.get("se", []):
                t, k, v = parse_cond(str(e), itens, personagens, c["id"])
                conds.append((t, flag_id(v) if k == -1 else k,
                              0 if k == -1 else v))
            ei = len(efeitos)
            for e in o.get("efeito", []):
                t, k, v = parse_efeito(str(e), itens, c["id"])
                efeitos.append((t, flag_id(v) if k == -1 else k,
                                0 if k == -1 else v))
            opcoes.append({
                "txt": P.add(o["txt"]), "vai": idx_cena[o["vai"]],
                "ci": ci, "cn": len(conds) - ci,
                "ei": ei, "en": len(efeitos) - ei,
            })
        c["_op_n"] = len(opcoes) - c["_op_ini"]

    alcanca, dist = resolve_oraculo(doc["cenas"], ordem, finais)

    # --- alcancabilidade a partir do inicio, para achar cena orfa ----------
    vistos, pilha = set(), [idx_cena["inicio"]]
    while pilha:
        i = pilha.pop()
        if i in vistos:
            continue
        vistos.add(i)
        for o in ordem[i].get("opcoes", []):
            pilha.append(idx_cena[o["vai"]])
    orfas = [c["id"] for i, c in enumerate(ordem) if i not in vistos]
    if orfas:
        print("  [alerta] cenas inalcancaveis:", ", ".join(orfas))
    nunca = [f for f in finais_ord
             if not (alcanca[idx_cena["inicio"]] >> finais[f]) & 1]
    if nunca:
        print("  [alerta] finais inalcancaveis:", ", ".join(nunca))

    # --- serializacao ------------------------------------------------------
    b_cena = bytearray()
    for i, c in enumerate(ordem):
        b_cena += struct.pack("<HHHBBIH", P.add(c["titulo"]), P.add(c["texto"]),
                              c["_op_ini"], c["_op_n"],
                              finais[c["final"]] + 1 if c.get("final") else 0,
                              alcanca[i], dist[i])

    b_op = bytearray()
    b_orac = bytearray()
    for o in opcoes:
        b_op += struct.pack("<HHHBHBH", o["txt"], o["vai"], o["ci"], o["cn"],
                            o["ei"], o["en"], 0)
        j = o["vai"]
        m = alcanca[j]
        bons = sum(1 for f in doc["finais"]
                   if f["qualidade"] > 0 and (m >> finais[f["id"]]) & 1)
        b_orac += struct.pack("<IHBB", m, dist[j], bons, 0)

    b_cond = b"".join(struct.pack("<BBh", t, k, v) for t, k, v in conds)
    b_ef = b"".join(struct.pack("<BBh", t, k, v) for t, k, v in efeitos)
    b_item = b"".join(struct.pack("<HBB", P.add(doc["itens"][n]["nome"]),
                                  1 if doc["itens"][n].get("arma") else 0, 0)
                      for n in itens_ord)
    b_fim = b"".join(struct.pack("<HBB", P.add(f["nome"]), f["qualidade"], 0)
                     for f in doc["finais"])

    b_pers = bytearray()
    for p in doc["personagens"]:
        mask = 0
        for it in p["itens"]:
            mask |= 1 << itens[it]
        b_pers += struct.pack("<HHI4B", P.add(p["nome"]), P.add(p["desc"]),
                              mask, *[p["stats"].get(s, 0) for s in STATS])

    secoes = [bytes(b_cena), bytes(b_op), bytes(b_orac), b_cond, b_ef,
              b_item, b_fim, bytes(b_pers), bytes(P.buf)]
    HDR = 128   # 28 B de campos + 9 secoes x 8 B, com folga
    corpo = bytearray()
    tabela = []
    for s in secoes:
        while len(corpo) % 4:
            corpo += b"\x00"
        tabela.append((HDR + len(corpo), len(s)))
        corpo += s

    hdr = bytearray(HDR)
    struct.pack_into("<4sHHIIHHHHHH", hdr, 0, MAGIC, 1, len(secoes),
                     zlib.crc32(corpo) & 0xFFFFFFFF, len(corpo),
                     len(ordem), len(opcoes), len(itens_ord),
                     len(finais_ord), len(pers_ord), idx_cena["inicio"])
    off = 28
    for o, n in tabela:
        struct.pack_into("<II", hdr, off, o, n)
        off += 8

    with open(SAIDA, "wb") as f:
        f.write(hdr)
        f.write(corpo)

    print(f"  {len(ordem)} cenas, {len(opcoes)} escolhas, "
          f"{len(finais_ord)} finais, {len(flags)} marcadores")
    print(f"  textos {len(P.buf)} B   pacote {HDR + len(corpo)} B")
    alc = bin(alcanca[idx_cena["inicio"]]).count("1")
    print(f"  finais alcancaveis a partir do inicio: {alc} de {len(finais_ord)}")
    print(f"  {SAIDA} gravado")


if __name__ == "__main__":
    main()
