#!/usr/bin/env python3
"""Converte exportacoes do TabNet no pacote binario lido pelo Cardputer ADV.

Uso:
    python3 tools/build_pack.py [--extracoes DIR] [--estado F] [--out F] [--force]

O script mantem um estado cumulativo em JSON. Cada execucao le todos os CSV
que estiverem em DIR, mescla sobre o estado (extracao nova sobrescreve as
competencias que ela cobre, porque o TabNet revisa os ultimos meses) e so
entao emite o pacote. Isso permite extrair apenas os ultimos meses depois da
carga inicial.

Formato do pacote: ver FORMAT.md.
"""
from __future__ import annotations

import argparse
import csv
import io
import json
import os
import re
import struct
import sys
import zlib
from collections import defaultdict

try:
    import yaml
except ImportError:
    sys.exit("falta pyyaml:  pip3 install pyyaml --break-system-packages")

MAGIC = b"TBN2"
FORMAT_VERSION = 2

MESES = {
    "janeiro": 1, "fevereiro": 2, "marco": 3, "março": 3, "abril": 4,
    "maio": 5, "junho": 6, "julho": 7, "agosto": 8, "setembro": 9,
    "outubro": 10, "novembro": 11, "dezembro": 12,
    "jan": 1, "fev": 2, "mar": 3, "abr": 4, "mai": 5, "jun": 6,
    "jul": 7, "ago": 8, "set": 9, "out": 10, "nov": 11, "dez": 12,
}


def per(ano: int, mes: int) -> int:
    return ano * 12 + (mes - 1)


def per_str(p: int) -> str:
    return f"{p % 12 + 1:02d}/{p // 12}"


def parse_num(s: str):
    """Celula do TabNet -> int ou None. '-' e vazio sao zero ausente."""
    s = s.strip().strip('"')
    if s in ("-", "", "..."):
        return None
    s = s.replace(".", "").replace(",", ".")
    try:
        return int(round(float(s)))
    except ValueError:
        return None


def le_csv(path: str) -> list[str]:
    with open(path, encoding="latin-1") as f:
        return f.read().replace("\r\n", "\n").split("\n")


def linha_csv(l: str) -> list[str]:
    return next(csv.reader(io.StringIO(l), delimiter=";"))


def rotulo_periodo(rot: str):
    """'Janeiro/2025' ou '2025/Jan' -> (ano, mes). Subtotais retornam None."""
    rot = rot.strip().strip('"')
    if "/" not in rot:
        return None                      # linha de subtotal de ano ou 'Total'
    a, b = [x.strip() for x in rot.split("/", 1)]
    if a.isdigit() and len(a) == 4:      # 2025/Jan
        mes = MESES.get(b.lower())
        return (int(a), mes) if mes else None
    if b.isdigit() and len(b) == 4:      # Janeiro/2025
        mes = MESES.get(a.lower())
        return (int(b), mes) if mes else None
    return None


# ---------------------------------------------------------------------------
# Parsers
# ---------------------------------------------------------------------------

def parse_sia(linhas: list[str], origem: str, av: "Aviso"):
    """SIA: linhas = periodos, colunas = regiao de saude.

    Devolve (base, codigo_proc, {(cod_cir, periodo): qtd}).
    """
    base = "atendimento" if "local de atendimento" in linhas[0] else "residencia"
    m = re.search(r"Procedimento:\s*(\d{10})", linhas[2])
    if not m:
        av.erro(origem, "linha 3 nao traz o codigo do procedimento")
        return None
    codigo = m.group(1)

    i = next((i for i, l in enumerate(linhas) if l.lstrip().startswith('"Ano/m')), None)
    if i is None:
        av.erro(origem, "cabecalho da tabela nao encontrado")
        return None
    cols = linha_csv(linhas[i])[1:-1]              # tira rotulo e coluna Total

    dados: dict[tuple[str, int], int] = {}
    for l in linhas[i + 1:]:
        if not l.startswith('"'):
            break
        row = linha_csv(l)
        am = rotulo_periodo(row[0])
        if am is None:
            continue                               # subtotal de ano / Total
        p = per(*am)
        for col, val in zip(cols, row[1:-1]):
            n = parse_num(val)
            if n is None:
                continue
            dados[(col, p)] = n
    return base, codigo, dados


def parse_sih(linhas: list[str], origem: str, av: "Aviso"):
    """SIH: linhas = estabelecimento, colunas = competencia, CIR fixa na linha 3.

    Devolve (cod_cir, {(cnes, periodo): qtd}).
    """
    m = re.search(r"\(CIR\):\s*(\d{5})", linhas[2])
    if not m:
        av.erro(origem, "linha 3 nao traz a regiao de saude (CIR)")
        return None
    cir = m.group(1)

    i = next((i for i, l in enumerate(linhas)
              if l.lstrip().startswith('"Estabelecimento"')), None)
    if i is None:
        av.erro(origem, "cabecalho da tabela nao encontrado")
        return None

    cols = []
    for rot in linha_csv(linhas[i])[1:-1]:
        am = rotulo_periodo(rot)
        if am is None:
            av.erro(origem, f"coluna de competencia ilegivel: {rot!r}")
            cols.append(None)
        else:
            cols.append(per(*am))

    dados: dict[tuple[str, int], int] = {}
    for l in linhas[i + 1:]:
        if not l.startswith('"'):
            break
        row = linha_csv(l)
        rot = row[0].strip()
        if rot.lower() == "total":
            continue
        cm = re.match(r"(\d{7})\s+(.*)", rot)
        if not cm:
            av.erro(origem, f"estabelecimento sem CNES: {rot!r}")
            continue
        cnes = cm.group(1)
        av.nome_cnes.setdefault(cnes, cm.group(2).strip())
        for p, val in zip(cols, row[1:-1]):
            n = parse_num(val)
            if p is None or n is None:
                continue
            dados[(cnes, p)] = n
    return cir, dados


# ---------------------------------------------------------------------------
# Relatorio
# ---------------------------------------------------------------------------

class Aviso:
    def __init__(self):
        self.erros: list[str] = []
        self.alertas: list[str] = []
        self.nome_cnes: dict[str, str] = {}

    def erro(self, origem, msg):
        self.erros.append(f"{os.path.basename(origem)}: {msg}")

    def alerta(self, msg):
        self.alertas.append(msg)


# ---------------------------------------------------------------------------
# Estado cumulativo
# ---------------------------------------------------------------------------

def carrega_estado(path):
    if os.path.exists(path):
        with open(path) as f:
            e = json.load(f)
        return {"amb": {k: v for k, v in e.get("amb", {}).items()},
                "vvpp": {k: v for k, v in e.get("vvpp", {}).items()}}
    return {"amb": {}, "vvpp": {}}


def salva_estado(path, estado):
    with open(path, "w") as f:
        json.dump(estado, f, separators=(",", ":"), sort_keys=True)


# ---------------------------------------------------------------------------
# Serializacao
# ---------------------------------------------------------------------------

class Strings:
    """Pool de strings com deduplicacao; offsets cabem em u16."""

    def __init__(self):
        self.buf = bytearray(b"\x00")   # offset 0 = string vazia
        self.idx = {"": 0}

    def add(self, s: str) -> int:
        s = s or ""
        if s in self.idx:
            return self.idx[s]
        off = len(self.buf)
        self.buf += s.encode("latin-1", "replace") + b"\x00"
        if off > 0xFFFF:
            raise RuntimeError("pool de strings passou de 64 KB")
        self.idx[s] = off
        return off


def monta_pacote(cfg, estado, av: Aviso) -> bytes:
    S = Strings()
    pmin = cfg["competencia_minima"]
    pmin = per(pmin // 100, pmin % 100)

    # --- dimensoes AMB -----------------------------------------------------
    procs = cfg["amb_procedimentos"]
    proc_ix = {p["codigo"]: i for i, p in enumerate(procs)}

    regioes = cfg["regioes"]
    cir_ix = {c: i for i, c in enumerate(sorted(regioes))}

    # --- fatos AMB ---------------------------------------------------------
    amb = []
    for chave, qtd in estado["amb"].items():
        base, cod, cir, p = chave.split("|")
        p = int(p)
        amb.append((0 if base == "atendimento" else 1, proc_ix[cod], cir_ix[cir], p, qtd))
    amb.sort()

    # --- fatos VVPP --------------------------------------------------------
    estabs = cfg["vvpp_estabelecimentos"]
    est_ix = {c: i for i, c in enumerate(sorted(estabs))}
    vv = []
    vv_cirs = set()
    for chave, qtd in estado["vvpp"].items():
        cir, cnes, p = chave.split("|")
        p = int(p)
        vv_cirs.add(cir)
        vv.append((cir_ix[cir], est_ix[cnes], p, qtd))
    vv.sort()

    per_min_amb = min((f[3] for f in amb), default=pmin)
    per_max_amb = max((f[3] for f in amb), default=pmin)
    per_min_vv = min((f[2] for f in vv), default=pmin)
    per_max_vv = max((f[2] for f in vv), default=pmin)
    for lo, hi, nome in ((per_min_amb, per_max_amb, "AMB"), (per_min_vv, per_max_vv, "VVPP")):
        if hi - lo > 255:
            raise RuntimeError(f"{nome}: serie maior que 255 meses")

    # --- secoes ------------------------------------------------------------
    sec: list[bytes] = []

    # 1 AMB_PROC  (16 B)
    b = bytearray()
    for p in procs:
        b += struct.pack("<IHHII", int(p["codigo"]), S.add(p["nome"]), S.add(p["abrev"]),
                         int(round(p["valor"] * 100)), 0)
    sec.append(bytes(b))

    # 2 AMB_GRUPO (12 B)
    b = bytearray()
    for g in cfg.get("amb_grupos", []):
        mask = 0
        for c in g["codigos"]:
            mask |= 1 << proc_ix[c]
        b += struct.pack("<HHII", S.add(g["nome"]), S.add(g["abrev"]), mask, 0)
    sec.append(bytes(b))

    # 3 CIR (8 B)
    b = bytearray()
    for cod in sorted(regioes):
        r = regioes[cod]
        b += struct.pack("<HHHH", int(cod), S.add(r["nome"]), S.add(r["abrev"]), 0)
    sec.append(bytes(b))

    # 4 GRUPO_LOCAL (12 B)
    b = bytearray()
    for g in cfg["grupos_local"]:
        if g["regioes"] == "todas":
            mask = (1 << len(cir_ix)) - 1
        else:
            mask = 0
            for c in g["regioes"]:
                mask |= 1 << cir_ix[c]
        b += struct.pack("<HHIHH", S.add(g["nome"]), S.add(g["abrev"]), mask,
                         1 if g.get("destaque") else 0, 0)
    sec.append(bytes(b))

    # 5 AMB_FACT (8 B)
    b = bytearray()
    for base, pi, ci, p, q in amb:
        b += struct.pack("<BBBBI", base, pi, ci, p - per_min_amb, q)
    sec.append(bytes(b))

    # 6 VVPP_ESTAB (12 B)
    b = bytearray()
    for cnes in sorted(estabs):
        e = estabs[cnes]
        b += struct.pack("<IHHI", int(cnes), S.add(e["rotulo"]),
                         S.add(e.get("cidade") or ""), 0)
    sec.append(bytes(b))

    # 7 VVPP_FACT (8 B)
    b = bytearray()
    for ci, ei, p, q in vv:
        b += struct.pack("<BBBBI", ci, ei, p - per_min_vv, 0, q)
    sec.append(bytes(b))

    # 8 VVPP_META (12 B, registro unico)
    v = cfg["vvpp"]
    mask_cob = 0
    for c in vv_cirs:
        mask_cob |= 1 << cir_ix[c]
    sec.append(struct.pack("<HHIIBBH", S.add(v["nome"]), S.add(v["abrev"]),
                           mask_cob, int(round(v["valor_estimado"] * 100)),
                           1 if v.get("estimado") else 0, 0, 0))

    # 9 STR
    sec.append(bytes(S.buf))

    # --- corpo e cabecalho -------------------------------------------------
    HDR = 128
    corpo = bytearray()
    tabela = []
    for s in sec:
        while len(corpo) % 4:
            corpo += b"\x00"
        tabela.append((HDR + len(corpo), len(s)))
        corpo += s

    ext = cfg["extracao"].replace("-", "")
    hdr = bytearray(HDR)
    struct.pack_into("<4sHHIIHHHHHH", hdr, 0,
                     MAGIC, FORMAT_VERSION, len(sec),
                     zlib.crc32(corpo) & 0xFFFFFFFF, len(corpo),
                     int(ext[:4]), int(ext[4:6]), int(ext[6:8]),
                     per_min_amb, per_max_amb, per_min_vv)
    struct.pack_into("<H", hdr, 30, per_max_vv)
    off = 32
    for o, n in tabela:
        struct.pack_into("<II", hdr, off, o, n)
        off += 8

    av.resumo = {
        "fatos_amb": len(amb), "fatos_vvpp": len(vv),
        "amb": (per_min_amb, per_max_amb), "vvpp": (per_min_vv, per_max_vv),
        "strings": len(S.buf), "bytes": HDR + len(corpo),
        "cob_vvpp": sorted(vv_cirs),
    }
    return bytes(hdr) + bytes(corpo)


# ---------------------------------------------------------------------------
# Principal
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--extracoes", default="extracoes")
    ap.add_argument("--config", default="tools/config.yaml")
    ap.add_argument("--estado", default="tools/estado.json")
    ap.add_argument("--out", default="build/tabnet.pack")
    ap.add_argument("--force", action="store_true",
                    help="grava o pacote mesmo com erros de validacao")
    a = ap.parse_args()

    with open(a.config, encoding="utf-8") as f:
        cfg = yaml.safe_load(f)
    av = Aviso()

    proc_validos = {p["codigo"] for p in cfg["amb_procedimentos"]}
    regioes = set(cfg["regioes"])
    descartadas = set(cfg.get("regioes_descartadas", []))
    pmin_i = cfg["competencia_minima"]
    pmin = per(pmin_i // 100, pmin_i % 100)

    estado = carrega_estado(a.estado)
    antes = {k: dict(v) for k, v in estado.items()}

    arquivos = sorted(f for f in os.listdir(a.extracoes) if f.lower().endswith(".csv"))
    if not arquivos:
        sys.exit(f"nenhum CSV em {a.extracoes}/")

    lidos_amb = lidos_vv = 0
    cob_amb = defaultdict(set)     # (base, codigo) -> competencias vistas
    for nome in arquivos:
        path = os.path.join(a.extracoes, nome)
        linhas = le_csv(path)
        cab = linhas[0]
        if "Ambulatorial" in cab:
            r = parse_sia(linhas, path, av)
            if not r:
                continue
            base, cod, dados = r
            if cod not in proc_validos:
                av.alerta(f"{nome}: procedimento {cod} nao esta no config; ignorado")
                continue
            for (col, p), q in dados.items():
                if p < pmin:
                    continue
                if col in descartadas:
                    continue
                cm = re.match(r"(\d{5})\s", col)
                if not cm or cm.group(1) not in regioes:
                    av.alerta(f"{nome}: coluna desconhecida {col!r}; ignorada")
                    continue
                estado["amb"][f"{base}|{cod}|{cm.group(1)}|{p}"] = q
                cob_amb[(base, cod)].add(p)
            lidos_amb += 1
        elif "hospitalares" in cab:
            r = parse_sih(linhas, path, av)
            if not r:
                continue
            cir, dados = r
            if cir not in regioes:
                av.alerta(f"{nome}: CIR {cir} nao esta no config; ignorado")
                continue
            for (cnes, p), q in dados.items():
                if p < pmin:
                    continue
                if cnes not in cfg["vvpp_estabelecimentos"]:
                    av.alerta(f"{nome}: CNES {cnes} ({av.nome_cnes.get(cnes,'?')}) "
                              "sem rotulo no config")
                    continue
                estado["vvpp"][f"{cir}|{cnes}|{p}"] = q
                lidos_vv += 1
        else:
            av.alerta(f"{nome}: cabecalho nao reconhecido; ignorado")

    # ---- diferencas em relacao a execucao anterior -------------------------
    novos = revis = 0
    delta_rev = 0
    for sec in ("amb", "vvpp"):
        for k, v in estado[sec].items():
            old = antes[sec].get(k)
            if old is None:
                novos += 1
            elif old != v:
                revis += 1
                delta_rev += v - old

    # ---- lacunas de competencia -------------------------------------------
    for (base, cod), ps in sorted(cob_amb.items()):
        faltando = [p for p in range(min(ps), max(ps) + 1) if p not in ps]
        # meses sem nenhum registro em nenhuma regiao sao suspeitos, nao certos
        if len(faltando) > 1:
            av.alerta(f"AMB {cod} ({base}): sem dado em "
                      + ", ".join(per_str(p) for p in faltando))

    pacote = monta_pacote(cfg, estado, av)

    # ---- relatorio ---------------------------------------------------------
    r = av.resumo
    print("=" * 62)
    print(f"  {len(arquivos)} arquivos  ({lidos_amb} SIA, {lidos_vv} blocos SIH)")
    print(f"  fatos novos: {novos}   revisados: {revis}"
          + (f"   (delta {delta_rev:+d})" if revis else ""))
    print("-" * 62)
    print(f"  AMB   {r['fatos_amb']:5d} fatos   "
          f"{per_str(r['amb'][0])} a {per_str(r['amb'][1])}")
    print(f"  VVPP  {r['fatos_vvpp']:5d} fatos   "
          f"{per_str(r['vvpp'][0])} a {per_str(r['vvpp'][1])}")
    print(f"  cobertura VVPP: " + ", ".join(
        cfg["regioes"][c]["abrev"] for c in r["cob_vvpp"]))
    print(f"  strings {r['strings']} B   pacote {r['bytes']} B")
    print("-" * 62)

    # conferencia: total por procedimento e ano, para bater com a tela do TabNet
    tot = defaultdict(int)
    for k, q in estado["amb"].items():
        base, cod, _cir, p = k.split("|")
        tot[(cod, base, int(p) // 12)] += q
    print("  CONFERENCIA - total por procedimento/ano")
    abrev = {p["codigo"]: p["abrev"] for p in cfg["amb_procedimentos"]}
    for (cod, base, ano), q in sorted(tot.items()):
        print(f"    {abrev[cod]:<13} {base:<11} {ano}  {q:>8,}".replace(",", "."))
    tv = defaultdict(int)
    for k, q in estado["vvpp"].items():
        cir, _cnes, p = k.split("|")
        tv[(cir, int(p) // 12)] += q
    for (cir, ano), q in sorted(tv.items()):
        print(f"    {'VVPP':<13} {cfg['regioes'][cir]['abrev']:<11} {ano}  {q:>8}")
    print("=" * 62)

    for m in av.alertas:
        print("  [alerta]", m)
    for m in av.erros:
        print("  [ERRO]  ", m)

    if av.erros and not a.force:
        sys.exit("\nerros de validacao; pacote NAO gravado (use --force para ignorar)")

    os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
    with open(a.out, "wb") as f:
        f.write(pacote)
    salva_estado(a.estado, estado)
    print(f"\n  {a.out} gravado ({len(pacote)} bytes)")
    print(f"  estado cumulativo em {a.estado}")


if __name__ == "__main__":
    main()
