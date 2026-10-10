#!/usr/bin/env python3
"""Baixa as gravacoes de onde os sons do jogo sao feitos para
tools/sons/fontes/ (fora do git). Depois: python3 tools/sons/gera.py

  dados/    dados batendo em madeira, feltro, metal e moedas
            (pacote npm @3d-dice/dice-box-threejs 0.0.12, licenca MIT)
  fluidr3/  notas do banco FluidR3 GM, de Frank Wen (midi-js-soundfonts),
            licenca CC BY 3.0
  rhodes/   Rhodes Mark I 1977 do jRhodes3d, de J. Learman (sfzinstruments);
            musica feita com ele e CC0, as amostras em si nao vao no jogo
  baixo/    baixo eletrico da Karoryfer (tonejs-instruments), CC BY 3.0
  sax/      sax-alto da Karoryfer (tonejs-instruments), CC BY 3.0
"""
import io, json, os, sys, tarfile, urllib.request
import concurrent.futures as cf

FONTES = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'fontes')
FL = 'https://raw.githubusercontent.com/gleitz/midi-js-soundfonts/gh-pages/FluidR3_GM/'
RH = 'https://raw.githubusercontent.com/sfzinstruments/jlearman.jRhodes3d/master/jRhodes3d-mono/'
TJ = 'https://raw.githubusercontent.com/nbrosowsky/tonejs-instruments/master/samples/'
NOTA_FL = ['C', 'Db', 'D', 'Eb', 'E', 'F', 'Gb', 'G', 'Ab', 'A', 'Bb', 'B']
NOTA_S = ['C', 'Cs', 'D', 'Ds', 'E', 'F', 'Fs', 'G', 'Gs', 'A', 'As', 'B']

def busca(url):
    return urllib.request.urlopen(url, timeout=60).read()

def dados():
    """As gravacoes de dados vem dentro do pacote npm."""
    if os.path.isdir(os.path.join(FONTES, 'dados', 'surfaces')):
        return
    meta = json.loads(busca('https://registry.npmjs.org/@3d-dice/dice-box-threejs/0.0.12'))
    tgz = busca(meta['dist']['tarball'])
    with tarfile.open(fileobj=io.BytesIO(tgz)) as t:
        for m in t.getmembers():
            pre = 'package/public/sounds/'
            if m.isfile() and m.name.startswith(pre) and m.name.endswith('.mp3'):
                destino = os.path.join(FONTES, 'dados', m.name[len(pre):])
                os.makedirs(os.path.dirname(destino), exist_ok=True)
                with open(destino, 'wb') as f:
                    f.write(t.extractfile(m).read())
    print('dados: ok')

def tarefas():
    # FluidR3: so os instrumentos e as faixas que as receitas usam
    for inst, lo, hi in (('glockenspiel', 84, 108), ('vibraphone', 72, 90), ('celesta', 72, 94),
                         ('acoustic_bass', 36, 48), ('timpani', 36, 41), ('marimba', 48, 52),
                         ('tubular_bells', 82, 86)):
        for m in range(lo, hi + 1):
            n = NOTA_FL[m % 12] + str(m // 12 - 1) + '.mp3'
            yield FL + inst + '-mp3/' + n, os.path.join('fluidr3', inst, n)
    # Rhodes: as quatro camadas de forca mais leves, das 15 regioes
    for c in (29, 35, 40, 45, 50, 55, 59, 62, 65, 71, 76, 81, 86, 91, 96):
        for camada in (2, 3, 4, 5):
            if camada == 3 and c >= 71:
                continue                              # o banco nao tem a camada 3 nos agudos
            n = 'A_%03d__%s%d_%d.flac' % (c, NOTA_FL[c % 12], c // 12 - 1, camada)
            yield RH + n, os.path.join('rhodes', n)
    for m in range(25, 71):                           # baixo: C#, E, G e A#
        if m % 12 in (1, 4, 7, 10):
            n = NOTA_S[m % 12] + str(m // 12 - 1) + '.mp3'
            yield TJ + 'bass-electric/' + n, os.path.join('baixo', n)
    for m in range(70, 79):                           # sax: em volta do Re5
        n = NOTA_S[m % 12] + str(m // 12 - 1) + '.mp3'
        yield TJ + 'saxophone/' + n, os.path.join('sax', n)

def baixa(t):
    url, rel = t
    destino = os.path.join(FONTES, rel)
    if os.path.exists(destino) and os.path.getsize(destino) > 0:
        return True
    try:
        d = busca(url)
    except Exception as e:
        print('falhou %s: %s' % (rel, e))
        return False
    os.makedirs(os.path.dirname(destino), exist_ok=True)
    with open(destino, 'wb') as f:
        f.write(d)
    return True

def main():
    dados()
    lista = list(tarefas())
    with cf.ThreadPoolExecutor(8) as ex:
        ok = sum(ex.map(baixa, lista))
    print('%d de %d arquivos em %s' % (ok, len(lista), os.path.relpath(FONTES)))
    if ok < len(lista):
        sys.exit(1)

if __name__ == '__main__':
    main()
