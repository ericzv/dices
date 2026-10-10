# Roteiro do video: "Dado em Casa - como jogar, passo a passo", para quem
# nunca mexeu num computador. Tudo com o mouse, ate o Modo Desafiante.
#
#   python3 roteiro.py saida.mp4            grava o video
#   python3 roteiro.py --seco               so confere o caminho (rapido)
#   python3 roteiro.py --previa DIR N       salva 1 quadro a cada N, sem gravar
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gravador import *
import cartoes
from plano import F, FN, ordem_mao, decide_jogar, decide_aposta

SEMENTE = 388872                 # a run da demonstracao (ensaiada: vence a Cautelosa)
NOME = "PAI"

g = None
j = None


# Centros das fichas do mapa (MAPA_X e MAPA_Y em desafio/desenho.inc).
MAPA_X = [136, 224, 312, 400, 488, 580]
MAPA_Y = [92, 142, 192]
def no_pos(p, l): return MAPA_X[p], MAPA_Y[l]


def fase(): return j.mv_fase()
def fala_e_espera(t, extra=0.4): g.diz(t); g.espera_fala(extra)


# ---------------------------------------------------------------- partes
def abertura():
    g.capitulo("Abertura")
    g.cartao(cartoes.abertura())
    g.vol_mus = 0.8
    g.pausa(1.0)
    fala_e_espera("Olá! Este vídeo ensina, bem devagar, a jogar o Dado em Casa.")
    g.vol_mus = 1.0
    fala_e_espera("Vamos aprender a usar o mouse, conhecer o menu do jogo, entender as regras e, no fim, "
                  "jogar juntos o Modo Desafiante, do começo ao fim.")
    fala_e_espera("Se alguma parte passar rápido demais, pode pausar ou voltar o vídeo quantas vezes quiser. "
                  "Não tem pressa nenhuma.", 0.8)


def parte_mouse():
    g.capitulo("1. O mouse")
    g.cartao(cartoes.titulo("Parte 1", "O MOUSE", "Tudo no jogo se faz com o mouse."))
    fala_e_espera("Primeira parte: o mouse.")
    g.cartao(cartoes.mouse_cartao(0))
    fala_e_espera("Para jogar, você só precisa do mouse. O mouse é esse aparelhinho que fica do lado do teclado.")
    fala_e_espera("Quando você mexe o mouse em cima da mesa, uma setinha se mexe na tela, do mesmo jeito.")
    g.cartao(cartoes.mouse_cartao(1))
    fala_e_espera("Em cima do mouse tem dois botões. O que importa é o da esquerda, que fica embaixo do dedo indicador.")
    g.cartao(cartoes.mouse_cartao(2))
    fala_e_espera("Para escolher alguma coisa na tela, leve a setinha até ela e aperte o botão da esquerda uma vez: "
                  "aperta e solta, rapidinho. Isso se chama clicar.")
    fala_e_espera("Não precisa clicar duas vezes. Um clique só já basta.")
    fala_e_espera("Se o computador for um notebook sem mouse, você arrasta o dedo no quadradinho que fica abaixo do teclado, "
                  "e aperta o botão da esquerda dele.", 0.6)
    # de volta ao jogo: o menu principal
    g.tira_cartao()
    g.mostra_cursor(470, 300)
    g.diz("Quando o jogo abre, aparece esta tela. Ela se chama menu principal.")
    g.pausa(1.2)
    g.espera_fala()
    g.diz("Repare na setinha. Quando ela passa por cima de um botão, o botão fica amarelo.")
    g.vai(2, 2, dur=1.4)
    g.pausa(0.6)
    g.vai(2, 3, dur=0.9)
    g.pausa(0.6)
    g.vai(2, 1, dur=1.0)
    g.espera_fala()
    fala_e_espera("O botão amarelo é o que vai ser escolhido se você clicar agora.")
    fala_e_espera("No vídeo, toda vez que eu clicar, vai aparecer um círculo amarelo e um mouse com o botão da esquerda aceso, "
                  "para você ver o momento do clique.", 0.6)


def parte_menu():
    g.capitulo("2. As formas de jogar")
    g.esconde_cursor()
    g.cartao(cartoes.titulo("Parte 2", "AS FORMAS DE JOGAR", "O que tem em cada botão do menu."))
    fala_e_espera("Segunda parte: as formas de jogar.")
    g.tira_cartao()
    g.mostra_cursor()
    fala_e_espera("O menu principal tem cinco botões. Vamos ver um por um.")
    explica = [
        (0, "Um jogador: você joga sozinho, contra o computador. É aqui que fica o Modo Desafiante."),
        (1, "Dois jogadores: duas pessoas jogam no mesmo computador, uma de cada vez."),
        (2, "Online: cada pessoa joga no seu próprio computador, pela internet. "
            "Um cria uma sala e passa um código de quatro letras para o outro entrar."),
        (3, "Dados da casa: um catálogo com todos os dados do jogo e o que cada um faz."),
        (4, "Como jogar: as regras do jogo, escritas, com desenhos."),
    ]
    for val, txt in explica:
        g.vai(2, val, dur=0.9)
        d = g.destaca_zona(2, val, margem=3)
        fala_e_espera(txt, 0.3)
        g.apaga(d)
    # 1 jogador
    fala_e_espera("Vamos entrar em um jogador. Leve a setinha até o botão e clique.")
    g.clica_em(2, 0, dur=1.2)
    g.pausa(0.8)
    fala_e_espera("Aqui tem duas opções.")
    g.vai(2, 0, dur=0.8)
    d = g.destaca_zona(2, 0, margem=3)
    fala_e_espera("Partida rápida: uma partida só, de quinze rodadas, contra o computador. "
                  "Você escolhe se ele joga no fácil, no médio ou no difícil. É bom para treinar.")
    g.apaga(d)
    g.vai(2, 1, dur=0.8)
    d = g.destaca_zona(2, 1, margem=3)
    fala_e_espera("E o Modo Desafiante: uma aventura por um cassino, com vários oponentes. "
                  "É o modo que vamos aprender hoje.")
    g.apaga(d)
    # voltar
    fala_e_espera("Antes, uma coisa muito importante: se você entrar num lugar errado, olhe para o canto de cima, "
                  "à esquerda da tela.")
    g.vai(tecla=KEY_ESC, dur=1.4)
    d = g.destaca_zona(tecla=KEY_ESC, rotulo="voltar", margem=2)
    fala_e_espera("Este botão com a setinha para a esquerda sempre volta para a tela anterior. Vou clicar nele.")
    g.apaga(d)
    g.clica()
    g.pausa(0.8)
    fala_e_espera("Pronto, voltamos ao menu principal.", 0.5)


def parte_regras():
    g.capitulo("3. As regras")
    g.esconde_cursor()
    g.cartao(cartoes.titulo("Parte 3", "AS REGRAS", "Como funciona uma rodada de dados."))
    fala_e_espera("Terceira parte: as regras.")
    g.tira_cartao()
    g.mostra_cursor()
    fala_e_espera("Para ver as regras, clique em Como jogar.")
    g.clica_em(2, 4, dur=1.2)
    g.pausa(0.6)
    # pagina 1: o jogo
    fala_e_espera("Cada jogador tem fichas. A cada rodada, os dois colocam uma entrada no pote, "
                  "que é o montinho de fichas do meio da mesa.")
    fala_e_espera("Quem fizer mais pontos na rodada leva o pote inteiro. No fim, ganha quem tiver mais fichas.")
    fala_e_espera("Para passar de página, clique no lado direito da tela.")
    g.move(520, 250, 1.0)
    g.clica()
    g.pausa(0.5)
    # pagina 2: a bolsa e a sequencia
    fala_e_espera("Cada jogador tem uma bolsa de dados. A cada rodada, você tira sete dados da bolsa, "
                  "e escolhe até quatro deles para jogar, na ordem que quiser.")
    fala_e_espera("Essa fila de dados escolhidos se chama sequência. Os dados da sequência são jogados um por um, "
                  "do primeiro ao último.")
    g.clica()
    g.pausa(0.5)
    # pagina 3: a regra da queda (com animacao)
    fala_e_espera("Agora, a regra mais importante do jogo. Preste atenção no desenho.")
    fala_e_espera("O primeiro dado sempre vale. Cada dado seguinte precisa tirar um número igual ou maior "
                  "que o último dado que valeu.")
    fala_e_espera("Se sair um número menor, o dado cai. E quando ele cai, leva junto o último que valia: "
                  "os dois ficam com um xis vermelho e não contam mais.")
    fala_e_espera("No desenho: saiu dois, valeu. Saiu quatro, valeu. Depois saiu três, que é menor que quatro: "
                  "o três e o quatro foram anulados.")
    fala_e_espera("Por isso a ordem importa: coloque os dados pequenos na frente, e os grandes no fim.", 0.6)
    g.clica()
    g.pausa(0.5)
    # pagina 4: parar ou arriscar
    fala_e_espera("Depois de cada dado, você decide: jogar o próximo, ou parar e ficar com os pontos que já tem.")
    fala_e_espera("O botão de jogar mostra a chance de o próximo dado cair. "
                  "E quem joga por último já sabe quantos pontos precisa fazer.")
    g.clica()
    g.pausa(0.5)
    # pagina 5: apostas
    fala_e_espera("Entre a vez de um jogador e a vez do outro, tem a aposta. "
                  "Dá para passar, que quer dizer não apostar, ou apostar dez ou vinte e cinco fichas.")
    fala_e_espera("Quem recebe uma aposta escolhe: pagar para ver, dobrar, ou correr. "
                  "Correr é desistir da rodada e entregar o pote para o outro.")
    fala_e_espera("Não precisa decorar nada disso agora. Já, já, vamos ver tudo acontecendo numa partida de verdade.")
    fala_e_espera("Para sair das regras, clique na setinha de voltar, lá no canto de cima.")
    g.clica_em(tecla=KEY_ESC, dur=1.4)
    g.pausa(0.8)


def parte_entrada():
    g.capitulo("4. Modo Desafiante: perfil")
    g.esconde_cursor()
    g.cartao(cartoes.titulo("Parte 4", "MODO DESAFIANTE", "Criando o seu perfil e começando a viagem."))
    fala_e_espera("Quarta parte: o Modo Desafiante.")
    g.tira_cartao()
    g.mostra_cursor()
    fala_e_espera("Para entrar, clique em um jogador.")
    g.clica_em(2, 0, dur=1.2)
    g.pausa(0.5)
    fala_e_espera("E depois em Modo Desafiante.")
    g.clica_em(2, 1, dur=0.9)
    g.pausa(0.6)
    fala_e_espera("O Modo Desafiante é uma viagem por um cassino. São três salões, e cada salão tem cinco mesas "
                  "e um chefe no final.")
    fala_e_espera("Você escolhe o caminho, joga contra vários oponentes, e vai ganhando moedas e dados novos.")
    fala_e_espera("Mas atenção: se perder uma partida, a viagem acaba, e você começa outra do zero. "
                  "No jogo, cada viagem dessas se chama run.")
    # perfis
    d = g.destaca(49, 66, 542, 190, margem=3)
    fala_e_espera("Primeiro, o jogo pergunta quem vai jogar. Cada pessoa da casa pode ter o seu perfil, "
                  "com o seu nome e o seu progresso guardado. Cabem três pessoas.")
    g.apaga(d)
    fala_e_espera("Clique em novo perfil.")
    g.clica_em(3, 0, dur=1.2)
    g.pausa(0.6)
    # criar perfil
    fala_e_espera("Agora, escolha um retrato. As setinhas dos lados trocam o desenho.")
    g.clica_em(tecla=KEY_RIGHT, dur=1.2, espera=0.7)
    fala_e_espera("Cada clique mostra outro retrato. Escolha o que achar mais bonito.")
    g.diz("Depois, escreva o seu nome usando o teclado. Aqui eu vou escrever PAI.")
    g.espera_fala(0.2)
    g.move(320, 236, 1.0)
    for ch in NOME:
        g.digita(ch)
    g.pausa(0.4)
    fala_e_espera("Se errar uma letra, a tecla de apagar, que fica no canto de cima do teclado, "
                  "do lado direito, com uma seta para a esquerda, apaga a última letra.")
    fala_e_espera("Com o nome escrito, clique em criar.")
    x, y, w, h = g.zona(tecla=KEY_ENTER)
    g.move(x + w / 2, y + h / 2, 1.0)
    g.pausa(0.25)
    # o clique que cria a run: o relogio fixo da a mesma run do ensaio
    g.pausa(0.15)
    cx, cy = g2o(*g.cur)
    g.cliques.append((g.f, cx, cy))
    m = (int(round(g.cur[0])), int(round(g.cur[1])))
    j.mouse(m[0], m[1], 0)
    j.mv_relogio(SEMENTE)
    j.mouse(m[0], m[1], 1)
    g.mouse_jogo = m
    g.pausa(0.4)
    g.esconde_cursor()
    fala_e_espera("O perfil foi criado. Agora, o elevador do cassino sobe até o primeiro salão.")
    g.ate(lambda: fase() == F["F_RAMULETO"] and not j.mv_elev() and j.mv_t_fase() > 0.6, 20, "amuleto")


def parte_amuleto_loja():
    g.capitulo("4. Amuleto e loja")
    g.mostra_cursor(320, 300)
    fala_e_espera("Toda run começa com a escolha de um amuleto. O amuleto é um ajudante que vale para a viagem inteira.")
    fala_e_espera("Para saber o que cada um faz, pare a setinha em cima dele, e leia o texto que aparece embaixo.")
    nomes = [j.mv_am_nome(j.mv_am_oferta(i)).decode() for i in range(3)]
    assert nomes[1] == "Reserva", nomes
    g.vai(1, 0, dur=1.0)
    g.pausa(2.2)
    g.vai(1, 2, dur=1.2)
    g.pausa(2.2)
    g.vai(1, 1, dur=1.0)
    d = g.destaca_zona(1, 1, margem=3)
    fala_e_espera("Este aqui é a Reserva: você começa cada partida com dez fichas a mais. "
                  "É simples, e ajuda sempre. Vou escolher a Reserva com um clique.")
    g.apaga(d)
    g.clica()
    g.pausa(1.2)
    # loja da entrada
    fala_e_espera("Agora estamos na loja da entrada. Aqui você troca moedas de ouro por dados especiais.")
    d = g.destaca(574, 8, 56, 28, rotulo="suas moedas", pos="esq", margem=2)
    fala_e_espera("Você começa com cinquenta moedas. O número de moedas que você tem fica sempre aqui, "
                  "no canto de cima, do lado da moedinha.")
    g.apaga(d)
    nomes = [j.mv_tipo_nome(j.mv_loja_tipo(i)).decode() for i in range(5)]
    assert nomes[2] == "Teimoso", nomes
    fala_e_espera("O preço de cada dado está escrito embaixo dele. Parando a setinha em cima, "
                  "aparece o nome e o que ele faz.")
    g.vai(1, 0, dur=1.0)
    g.pausa(1.6)
    g.vai(1, 1, dur=0.8)
    g.pausa(1.6)
    g.vai(1, 2, dur=0.8)
    d = g.destaca_zona(1, 2, margem=3)
    fala_e_espera("Este é o Teimoso: um dado de seis lados que nunca é anulado. Nem quando o dado seguinte cai. "
                  "Ele custa quarenta moedas.")
    fala_e_espera("Para comprar, é só clicar nele.")
    g.apaga(d)
    g.clica(0.8)
    fala_e_espera("Pronto, comprado! Ele foi para a sua bolsa, e sobraram dez moedas.")
    g.vai(tecla=9, dur=1.2)
    d = g.destaca_zona(tecla=9, margem=3)
    fala_e_espera("Quando terminar de comprar, clique em sair da loja.")
    g.apaga(d)
    g.clica()
    g.pausa(1.0)




def parte_mapa():
    g.capitulo("5. O mapa do cassino")
    g.esconde_cursor()
    g.cartao(cartoes.titulo("Parte 5", "O MAPA DO CASSINO", "Escolhendo por onde ir."))
    fala_e_espera("Quinta parte: o mapa do cassino.")
    g.tira_cartao()
    fala_e_espera("Este é o mapa do Salão Verde, o primeiro salão. Cada ficha redonda é uma mesa.")
    d = g.destaca(110, 52, 500, 18, margem=4)
    fala_e_espera("Você anda da esquerda para a direita, uma coluna por vez: mesa um, mesa dois, e assim por diante, "
                  "até o chefe, lá no fim.")
    g.apaga(d)
    tipos = [[j.mv_no_tipo(p, l) for l in range(3)] for p in range(6)]
    def acha(t, pmin=0):
        for p in range(pmin, 6):
            for l in range(3):
                if tipos[p][l] == t: return p, l
    fala_e_espera("A cor da ficha e os pontinhos no meio dizem se a mesa é fácil ou difícil.")
    for t, rot, txt in [(0, "fácil", "Verde, com um pontinho, é fácil."),
                        (1, "média", "Amarela, com dois pontinhos, é média."),
                        (2, "difícil", "Vermelha, com três pontinhos, é difícil."),
                        (4, "loja", "A ficha com o cifrão é uma loja, para gastar moedas."),
                        (5, "evento", "A ficha com o ponto de interrogação é um evento: uma surpresa, "
                                      "que quase sempre ajuda."),
                        (3, "chefe", "E a ficha grande, no fim, é o chefe do salão.")]:
        pl = acha(t, 1 if t in (0, 1) else 0) or acha(t)
        if pl is None:
            if t == 4: txt = "Em alguns salões aparece também uma ficha com um cifrão: é uma loja, para gastar moedas."
            fala_e_espera(txt, 0.3); continue
        x, y = no_pos(*pl)
        r = 20 if t == 3 else 15
        d = g.destaca(x - r, y - r, 2 * r, 2 * r, rotulo=rot, margem=6)
        fala_e_espera(txt, 0.3)
        g.apaga(d)
    fala_e_espera("As linhas de pontinhos mostram o caminho. Você só pode ir para uma mesa que esteja "
                  "ligada à mesa de antes.")
    g.mostra_cursor(330, 270)
    fala_e_espera("Parando a setinha numa mesa, o quadro de baixo mostra quem é o oponente.")
    escolhe = [l for l in range(3) if j.mv_escolhivel(l)]
    facil = [l for l in escolhe if tipos[0][l] == 0][0]
    outra = [l for l in escolhe if l != facil][0]
    g.vai(4, outra, dur=1.2)
    g.pausa(1.0)
    g.vai(4, facil, dur=1.0)
    g.pausa(0.5)
    d = g.destaca(28, 220, 584, 86, margem=3)
    fala_e_espera("Aqui está A Cautelosa, numa mesa fácil. No quadro aparece o nome dela, o prêmio, "
                  "o trunfo, que é uma regra especial só dela, e os dados que ela tem na bolsa.")
    fala_e_espera("O trunfo da Cautelosa é: o primeiro dado da sequência dela nunca cai.")
    g.apaga(d)
    fala_e_espera("Uma dica: no começo, prefira as mesas fáceis, as verdes. Vou clicar na mesa da Cautelosa para sentar e jogar.")
    g.vai(4, facil, dur=0.5)
    g.pausa(0.15)
    cx, cy = g2o(*g.cur)
    g.cliques.append((g.f, cx, cy))
    j.mouse(int(round(g.cur[0])), int(round(g.cur[1])), 1)
    g.congelado = True                   # a mesa espera a explicacao
    g.pausa(0.5)
    return facil


def espera_minha_vez(nome="minha vez", maxs=60):
    g.ate(lambda: fase() in (F["F_ORDEM"], F["F_JOGA"], F["F_APOSTA"]) and j.mv_vez() == 0
          and not j.mv_ia_na_vez() and not j.mv_anuncio()
          and (fase() != F["F_ORDEM"] or (j.mv_n_mao(0) and j.mv_t_fase() > 2.5))
          and (fase() != F["F_JOGA"] or j.mv_t_fase() > 0.6), maxs, nome)


def espera_lance():
    """Depois de clicar em jogar: espera o dado parar e o resultado aparecer."""
    g.ate(lambda: fase() not in (F["F_ROLANDO"], F["F_ARRUMA"]), 20, "rolando")
    g.ate(lambda: fase() != F["F_VEREDITO"], 20, "veredito")
    g.pausa(0.5)


def monta_sequencia(narra=True):
    o = ordem_mao(j)
    for k, i in enumerate(o):
        g.clica_em(1, i, dur=0.9 if narra else 0.5, espera=0.5 if narra else 0.25)
    return o


def parte_partida():
    g.capitulo("6. Uma partida")
    g.esconde_cursor()
    g.cartao(cartoes.titulo("Parte 6", "UMA PARTIDA", "Jogando contra A Cautelosa."))
    fala_e_espera("Sexta parte: uma partida de verdade.")
    g.tira_cartao()
    fala_e_espera("Esta é a mesa de jogo. Antes de jogar, vamos olhar com calma.")
    d = g.destaca(0, 0, 250, 44, rotulo="o oponente", pos="baixo", margem=0)
    fala_e_espera("Lá em cima fica o oponente: o retrato, o nome, e as fichas dele.")
    g.apaga(d)
    d = g.destaca(0, 316, 170, 44, rotulo="você", margem=0)
    fala_e_espera("Aqui embaixo fica você: o seu retrato, o seu nome, e as suas fichas.")
    g.apaga(d)
    d = g.destaca(562, 156, 48, 68, rotulo="o pote", margem=4)
    fala_e_espera("No meio da mesa, à direita, fica o pote, com as fichas que estão em jogo nesta rodada.")
    g.apaga(d)
    d = g.destaca(270, 10, 140, 22, rotulo="a rodada", pos="baixo", margem=3)
    fala_e_espera("E lá em cima, no meio, está escrito qual é a rodada, e quanto custa a entrada.")
    g.apaga(d)
    fala_e_espera("No Modo Desafiante, cada partida tem oito rodadas, e cada um começa com cinquenta fichas. "
                  "Como eu escolhi a Reserva, comecei com dez a mais.")
    fala_e_espera("Ganha a partida quem terminar com mais fichas, ou quem deixar o outro sem fichas.")
    g.congelado = False
    # ---- rodada 1: a Cautelosa abre
    assert j.mv_primeiro() == 1
    g.diz("Nesta primeira rodada, quem começa é a Cautelosa. Vamos olhar ela jogar: "
          "os dados dela aparecem na parte de cima da mesa.")
    g.ate(lambda: j.mv_anuncio() and b"parou" in j.mv_anuncio_txt(), 40, "ela parou")
    g.congelado = True
    fala_e_espera("Ela tirou dois e depois seis, e parou com oito pontos. Agora eu sei que preciso fazer mais de oito.")
    g.congelado = False
    g.ate(lambda: j.mv_anuncio() and b"passou" in j.mv_anuncio_txt(), 20, "ela passou")
    g.congelado = True
    fala_e_espera("Na aposta, ela passou. Quer dizer que ela não quis apostar.")
    g.congelado = False
    espera_minha_vez("aposta r1")
    assert fase() == F["F_APOSTA"] and decide_aposta(j) == 1
    g.mostra_cursor(330, 260)
    d = g.destaca(170, 160, 300, 46, margem=3)
    fala_e_espera("Agora é a minha vez de decidir a aposta. Tenho três botões: passar, apostar dez, ou apostar vinte e cinco.")
    g.apaga(d)
    fala_e_espera("Vou apostar dez fichas, para você ver como funciona.")
    g.clica_em(1, 1, dur=1.0)
    g.ate(lambda: j.mv_anuncio() and b"pagou" in j.mv_anuncio_txt(), 20, "ela pagou")
    g.congelado = True
    fala_e_espera("Ela pagou para ver. Agora o pote tem trinta fichas: quem ganhar a rodada leva tudo.")
    g.congelado = False
    espera_minha_vez("ordem r1")
    mao = [j.mv_mao_nome(0, i).decode() for i in range(j.mv_n_mao(0))]
    assert mao == ['Moeda', 'D4', 'Teimoso', 'Moeda', 'D6', 'D6', 'D4'], mao
    d = g.destaca(133, 237, 374, 50, rotulo="a sua mão: 7 dados", pos="baixo", margem=4)
    fala_e_espera("Agora eu monto a minha sequência. Estes sete dados saíram da minha bolsa.")
    fala_e_espera("O número desenhado em cada dado mostra quantos lados ele tem. A moedinha amarela tem dois lados, "
                  "o triângulo tem quatro, e os de seis lados são o dado comum.")
    g.apaga(d)
    fala_e_espera("Para montar a sequência, clique nos dados na ordem em que quer jogar. Lembre da dica: "
                  "os pequenos primeiro.")
    o = ordem_mao(j)
    assert o == [1, 2, 4, 5], o
    g.clica_em(1, 1, dur=1.1, espera=0.6)
    fala_e_espera("Primeiro, o de quatro lados. Repare que ele ganhou o número um.")
    g.clica_em(1, 2, dur=0.9, espera=0.6)
    fala_e_espera("Depois, o Teimoso, que eu comprei na loja.")
    g.clica_em(1, 4, dur=0.9, espera=0.5)
    g.clica_em(1, 5, dur=0.8, espera=0.6)
    fala_e_espera("E dois dados comuns, de seis lados. Os números em cima de cada dado mostram a ordem.")
    fala_e_espera("Se mudar de ideia, é só clicar de novo no dado, e ele sai da sequência.")
    g.vai(tecla=9, dur=1.0)
    d = g.destaca_zona(tecla=9, margem=3)
    fala_e_espera("Com a sequência pronta, clique em pronto.")
    g.apaga(d)
    g.clica(0.8)
    espera_minha_vez("joga r1")
    d = g.destaca_zona(1, 0, margem=3)
    g.vai(1, 0, dur=1.0)
    fala_e_espera("Agora aparece o botão jogar. Clique nele para lançar o primeiro dado.")
    g.apaga(d)
    assert decide_jogar(j)
    g.clica(0.3)
    espera_lance()
    d = g.destaca(508, 236, 66, 52, rotulo="meus pontos", margem=3)
    fala_e_espera("Saiu dois. O primeiro dado sempre vale: tenho dois pontos. O total fica nesse quadrado, à direita.")
    g.apaga(d)
    espera_minha_vez("joga r1 2")
    d = g.destaca_zona(1, 0, margem=3)
    g.vai(1, 0, dur=0.8)
    fala_e_espera("Repare no botão: a chance de o próximo dado cair é de dezesseis por cento. É pouco. Vou jogar.")
    g.apaga(d)
    assert decide_jogar(j)
    g.clica(0.3)
    espera_lance()
    fala_e_espera("O Teimoso tirou quatro. Quatro é maior que dois: valeu! Agora tenho seis pontos.")
    espera_minha_vez("joga r1 3")
    g.vai(1, 1, dur=0.9)
    d = g.destaca_zona(1, 1, margem=3)
    fala_e_espera("Agora repare no botão parar: ele mostra o que aconteceria se eu parasse agora. "
                  "Aqui diz: perde por dois.")
    g.apaga(d)
    g.vai(1, 0, dur=0.6)
    fala_e_espera("A chance de cair agora é de cinquenta por cento. Mas seis ainda é menos que os oito dela, "
                  "então eu preciso arriscar.")
    assert decide_jogar(j)
    g.clica(0.3)
    espera_lance()
    assert [j.mv_fila_est(0, k) for k in range(3)] == [1, 1, 2]
    fala_e_espera("Saiu três, que é menor que quatro. Esse dado caiu, e ficou com o xis vermelho.")
    fala_e_espera("Normalmente ele levaria junto o quatro. Mas o quatro é do Teimoso, que nunca é anulado. "
                  "Por isso eu continuo com seis pontos.")
    espera_minha_vez("joga r1 4")
    g.vai(1, 0, dur=0.6)
    fala_e_espera("Ainda tenho um dado. Vamos lá.")
    assert decide_jogar(j)
    g.clica(0.3)
    espera_lance()
    fala_e_espera("Saiu cinco! Cinco é maior que quatro, então valeu. Fiquei com onze pontos, contra oito dela.")
    g.ate(lambda: fase() == F["F_RESULTADO"], 30, "resultado r1")
    g.pausa(1.5)
    fala_e_espera("Ganhei a rodada, e levei o pote de trinta fichas.")
    fala_e_espera("Para seguir, clique em qualquer lugar da mesa.")
    g.move(320, 120, 1.0)
    g.clica(0.8)
    # ---- rodada 2: eu abro, no escuro
    fala_e_espera("Quem ganha a rodada começa a próxima. Então agora eu jogo primeiro, sem saber quantos pontos ela vai fazer.")
    espera_minha_vez("ordem r2")
    fala_e_espera("De novo, monto a sequência: a moedinha, o Teimoso, e dois dados comuns.")
    o = ordem_mao(j)
    for i in o: g.clica_em(1, i, dur=0.8, espera=0.35)
    g.clica_em(tecla=9, dur=0.9, espera=0.6)
    espera_minha_vez("joga r2")
    g.vai(1, 0, dur=0.6)
    assert decide_jogar(j); g.clica(0.3); espera_lance()
    espera_minha_vez("joga r2 2")
    g.vai(1, 0, dur=0.4)
    assert decide_jogar(j); g.clica(0.3); espera_lance()
    fala_e_espera("A moedinha deu dois, e o Teimoso deu seis. Já tenho oito pontos.")
    espera_minha_vez("joga r2 3")
    d = g.destaca(212, 181, 217, 24, margem=3)
    fala_e_espera("Agora tenho dois botões: jogar o próximo dado, ou parar e ficar com os oito pontos.")
    g.apaga(d)
    fala_e_espera("A chance de cair é alta. Mas, com o Teimoso valendo, cair não me custa nada: só o dado novo se perde. "
                  "Então vale a pena arriscar.")
    g.vai(1, 0, dur=0.6)
    assert decide_jogar(j); g.clica(0.3); espera_lance()
    espera_minha_vez("joga r2 4")
    g.vai(1, 0, dur=0.4)
    assert decide_jogar(j); g.clica(0.3); espera_lance()
    fala_e_espera("Os dois últimos caíram, mas o Teimoso segurou. Continuo com oito pontos.")
    espera_minha_vez("aposta r2")
    assert fase() == F["F_APOSTA"] and decide_aposta(j) == 0
    g.vai(1, 0, dur=1.0)
    fala_e_espera("Na aposta, desta vez eu vou passar. Quando estiver em dúvida, passar é sempre seguro.")
    g.clica(0.5)
    g.diz("Agora é a vez dela. Ela joga, e eu só olho.")
    g.esconde_cursor()
    g.ate(lambda: j.mv_fila_lancados(1) >= 3 and fase() == F["F_JOGA"], 40, "ela 3 dados")
    g.congelado = True
    fala_e_espera("Olha a regra da queda acontecendo com ela: saiu dois, depois do cinco. "
                  "O dois e o cinco foram anulados juntos.")
    g.congelado = False
    g.ate(lambda: fase() == F["F_RESULTADO"], 40, "resultado r2")
    g.pausa(1.2)
    fala_e_espera("Ela terminou com cinco pontos, e eu ganhei a rodada com oito.")
    g.mostra_cursor(320, 120)
    g.clica(0.6)


def joga_rodadas_aceleradas():
    """O resto da partida, em velocidade alta, com o mesmo plano."""
    g.vel = 4
    g.selo("rapido", "vídeo acelerado", (GX + 1600 - 330, 70))
    g.diz("Agora eu vou acelerar o vídeo, para as próximas rodadas passarem mais rápido. "
          "Repare nas fichas da Cautelosa, lá em cima, acabando.")
    while True:
        f = fase()
        if f == F["F_FIM"]: break
        if j.mv_ia_na_vez() or j.mv_anuncio() or j.mv_virada():
            g.quadro(); continue
        if f == F["F_ORDEM"] and j.mv_vez() == 0 and j.mv_n_mao(0) and j.mv_t_fase() > 2.5:
            if j.mv_fila_n(0) == 0:
                for i in ordem_mao(j): g.clica_em(1, i, dur=0.25, espera=0.05, antes=0.05)
            g.clica_em(tecla=9, dur=0.25, espera=0.05, antes=0.05)
        elif f == F["F_JOGA"] and j.mv_vez() == 0 and j.mv_t_fase() > 0.8:
            g.clica_em(1, 0 if decide_jogar(j) else 1, dur=0.25, espera=0.05, antes=0.05)
        elif f == F["F_APOSTA"] and j.mv_vez() == 0 and j.mv_t_fase() > 0.8:
            g.clica_em(1, decide_aposta(j), dur=0.25, espera=0.05, antes=0.05)
        elif f == F["F_RESULTADO"] and j.mv_t_fase() > 2.0:
            g.move(320, 120, 0.2); g.clica(0.05)
        else:
            g.quadro()
    g.vel = 1
    g.selo("rapido")
    g.pausa(0.8)


def parte_vitoria():
    g.capitulo("7. Vitória e prêmio")
    g.ate(lambda: fase() == F["F_FIM"] and j.mv_t_fase() > 1.5, 20, "fim")
    assert j.mv_venc_partida() == 0
    fala_e_espera("Pronto! A Cautelosa ficou sem fichas, e eu venci a partida.")
    fala_e_espera("As fichas que sobraram viram moedas de ouro: cem fichas, cem moedas. "
                  "Com as dez que eu tinha, fiquei com cento e dez.")
    g.mostra_cursor(320, 200)
    fala_e_espera("Clique em qualquer lugar para receber o prêmio.")
    g.clica(0.5)
    g.esconde_cursor()
    g.cartao(cartoes.titulo("Parte 7", "O PRÊMIO", "E depois, de volta ao mapa."))
    fala_e_espera("Sétima parte: o prêmio.")
    g.tira_cartao()
    g.diz("Cada vitória dá direito a um prêmio. A roleta gira, e sorteia três dados.")
    g.ate(lambda: fase() == F["F_PREMIO"] and not j.mv_slot_girando(), 20, "roleta")
    g.espera_fala()
    g.mostra_cursor(320, 230)
    fala_e_espera("Você escolhe um deles para levar para a sua bolsa. Pare a setinha em cima de cada um para ler o que faz.")
    for i in range(3):
        g.vai(1, i, dur=0.9)
        g.pausa(2.0)
    pr = j.mv_conselho_premio()
    nomes = [j.mv_tipo_nome(j.mv_oferta_tipo(i)).decode() for i in range(3)]
    assert nomes[pr] == "Espelho", (nomes, pr)
    g.vai(1, 3, dur=1.0)
    d = g.destaca_zona(1, 3, margem=3)
    fala_e_espera("Em vez de levar um dado novo, você também pode clicar em remover um dado, "
                  "para tirar um dado fraco da sua bolsa.")
    g.apaga(d)
    g.vai(1, pr, dur=1.0)
    fala_e_espera("Eu vou levar o Espelho: se ele tirar um ou dois, vira o número do maior dado do oponente. Clique nele para levar.")
    g.clica(1.0)
    g.ate(lambda: fase() == F["F_MAPA"], 20, "mapa 2")
    g.pausa(0.8)
    fala_e_espera("E voltamos ao mapa. A primeira mesa ficou marcada como feita, e agora a escolha é a próxima coluna.")
    fala_e_espera("É sempre assim: escolher uma mesa ligada, jogar, ganhar o prêmio, e seguir em frente.")
    x, y = MAPA_X[5], MAPA_Y[1]
    d = g.destaca(x - 22, y - 22, 44, 44, rotulo="chefe", margem=6)
    fala_e_espera("No fim de cada salão tem um chefe, mais forte que os outros oponentes. No Salão Verde é o Crupiê, "
                  "no Vermelho é a Dona do Salão, e no Dourado é o Barão.")
    g.apaga(d)
    fala_e_espera("Vencendo um chefe, você ganha mais um amuleto e sobe para o próximo salão. Vencendo o Barão, "
                  "você vence o cassino inteiro, e abre uma dificuldade nova.")
    fala_e_espera("Nas lojas do caminho, use as moedas para comprar dados. E nos eventos, leia o texto com calma, "
                  "e clique na opção que quiser.")


def parte_continuar():
    g.capitulo("8. Parar e continuar")
    g.esconde_cursor()
    g.cartao(cartoes.titulo("Parte 8", "PARAR E CONTINUAR", "O jogo guarda tudo sozinho."))
    fala_e_espera("Oitava parte: como parar e continuar outro dia.")
    g.tira_cartao()
    g.mostra_cursor(320, 200)
    fala_e_espera("Você pode parar de jogar quando quiser. O melhor momento é aqui, no mapa, entre uma mesa e outra. "
                  "O jogo guarda tudo sozinho.")
    g.vai(tecla=KEY_ESC, dur=1.4)
    d = g.destaca_zona(tecla=KEY_ESC, rotulo="voltar ao menu", margem=2)
    fala_e_espera("Para sair do mapa, clique na setinha de voltar.")
    g.apaga(d)
    g.clica(1.0)
    fala_e_espera("Só um cuidado: se você sair no meio de uma partida, aquela partida recomeça do zero quando você voltar.")
    fala_e_espera("Agora, imagine que é outro dia, e você quer continuar de onde parou.")
    fala_e_espera("Clique em um jogador, no alto da lista.")
    g.clica_em(2, 0, dur=1.2)
    g.pausa(0.4)
    fala_e_espera("Depois em Modo Desafiante.")
    g.clica_em(2, 1, dur=0.9)
    g.pausa(0.6)
    fala_e_espera("E depois clique no seu perfil, que agora tem o seu nome e o seu retrato.")
    g.clica_em(3, 0, dur=1.2)
    g.pausa(0.6)
    g.vai(2, 0, dur=0.9)
    d = g.destaca_zona(2, 0, margem=3)
    fala_e_espera("Aparecem duas opções. Continuar a run volta exatamente de onde você parou. "
                  "Repare que embaixo aparece onde você está e quantas moedas tem.")
    g.apaga(d)
    g.vai(2, 1, dur=0.8)
    d = g.destaca_zona(2, 1, margem=3)
    fala_e_espera("Já a Nova run começa uma viagem nova, do começo, e apaga a que estava guardada.")
    g.apaga(d)
    g.vai(2, 0, dur=0.8)
    fala_e_espera("Então, para continuar, clique sempre em Continuar a run.")
    g.clica(1.0)
    g.ate(lambda: fase() == F["F_MAPA"], 20, "mapa 3")
    fala_e_espera("Pronto: de volta ao mapa, com tudo do jeito que estava.")
    fala_e_espera("E se um dia você perder uma partida, vai aparecer escrito derrota, e a run termina. "
                  "Não tem problema nenhum: é só começar uma Nova run, e tentar de novo.")
    fala_e_espera("A cada tentativa você aprende um pouco mais, e vai mais longe.", 0.6)


def parte_resumo():
    g.capitulo("Resumo")
    g.esconde_cursor()
    linhas = [
        "Clique em 1 jogador, depois em Modo Desafiante, depois no seu perfil.",
        "Na primeira vez: crie o perfil, escolha um amuleto e compre um dado na loja.",
        "No mapa, clique numa mesa ligada. No começo, prefira as verdes.",
        "Monte a sequência com os dados pequenos primeiro, e clique em pronto.",
        "Clique em jogar ou em parar. Na aposta, na dúvida, passe ou pague.",
        "Venceu? Escolha o prêmio. Para continuar outro dia: Continuar a run.",
    ]
    g.cartao(cartoes.resumo(linhas))
    g.vol_mus = 0.9
    fala_e_espera("Para terminar, um resumo do que vimos.")
    cartoes_res = [cartoes.resumo(linhas, k) for k in range(len(linhas))]
    falas = [
        "Um: clique em um jogador, depois em Modo Desafiante, e depois no seu perfil.",
        "Dois: na primeira vez, crie o seu perfil, escolha um amuleto, e compre um dado na loja.",
        "Três: no mapa, clique numa mesa ligada. No começo, prefira as verdes.",
        "Quatro: monte a sequência com os dados pequenos primeiro, e clique em pronto.",
        "Cinco: clique em jogar para lançar, ou em parar para ficar com os pontos. Na aposta, na dúvida, passe ou pague.",
        "Seis: venceu, escolha o prêmio. E para continuar outro dia, clique em Continuar a run.",
    ]
    for k, f in enumerate(falas):
        g.cartao(cartoes_res[k])
        fala_e_espera(f, 0.5)
    g.cartao(cartoes.resumo(linhas))
    fala_e_espera("Lembre: dá para pausar e voltar este vídeo sempre que precisar.")
    fala_e_espera("Boa sorte, e bom jogo!", 1.5)
    g.vol_mus = 1.0
    g.pausa(2.5)


def main():
    global g, j
    seco = "--seco" in sys.argv
    previa = None
    if "--previa" in sys.argv:
        i = sys.argv.index("--previa")
        previa = (sys.argv[i + 1], int(sys.argv[i + 2]))
        os.makedirs(previa[0], exist_ok=True)
    saida = [a for a in sys.argv[1:] if a.endswith(".mp4")]
    saida = saida[0] if saida else os.path.join(AQUI, "saida", "tutorial.mp4")
    os.makedirs(os.path.dirname(saida), exist_ok=True)
    g = Gravador(saida, dry=seco, preview=previa)
    j = g.j
    j.mv_salva_limpa()
    j.inicia(0, 987654321)
    # a mesa de abertura assenta antes de o video comecar
    for _ in range(240): j.passo(1 / 60)
    partes = [abertura, parte_mouse, parte_menu, parte_regras, parte_entrada, parte_amuleto_loja, parte_mapa,
              parte_partida, joga_rodadas_aceleradas, parte_vitoria, parte_continuar, parte_resumo]
    so = [a for a in sys.argv[1:] if a.startswith("--ate=")]
    for p in partes:
        t0 = g.f / FPS
        p()
        print(f"{p.__name__:28s} {t0 / 60:5.2f} -> {g.f / FPS / 60:5.2f} min", flush=True)
        if so and p.__name__ == so[0][6:]: break
    g.fecha()
    print("total", g.f / FPS / 60, "min")


if __name__ == "__main__":
    main()
