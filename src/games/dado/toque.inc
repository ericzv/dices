// ===========================================================================
// Aparelho so de toque (DADO_TOQUE): o console do ESP32-P4, branch paralela.
// O PC e o navegador nao compilam isto.
// ===========================================================================
//
// Na mesa da partida, um toque e um clique. Fora dela (menus, mapa, lojas,
// bolsa, premios, perfis), o primeiro toque escolhe, como o mouse passando
// por cima, e o segundo no mesmo lugar confirma. Tocar onde nao ha nada para
// escolher (as telas de "toque para seguir") vale na hora.
//
// As dicas de teclado viram dicas de toque (DICA, chamada em txt, txt_c,
// teclas_c e quebra_linhas).

static const char *const DICAS_TOQUE[][2] = {
    // menus
    { "M som  ·  N música  ·  F11 tela cheia  ·  - e + tamanho da mesa",
      "toque para escolher  ·  toque de novo para confirmar" },
    { "setas escolhem  ·  ENTER confirma  ·  ESC volta",
      "toque para escolher  ·  toque de novo para confirmar  ·  o < volta" },
    { "< anterior  ·  ENTER ou > próxima  ·  ESC menu",
      "toque à esquerda: anterior  ·  à direita: próxima  ·  o < volta" },
    { "< anterior  ·  ENTER volta ao menu",
      "toque à esquerda: anterior  ·  à direita: volta ao menu" },
    { "ESC volta ao menu", "o < volta ao menu" },
    { "ENTER volta", "toque para voltar" },
    { "ENTER tenta de novo  ·  ESC volta", "toque para tentar de novo  ·  o < volta" },
    { "ENTER entra na sala", "OK entra na sala" },
    { "BACKSPACE apaga  ·  ESC volta", "a tecla de apagar apaga  ·  o < volta" },
    { "setas trocam o dado  ·  TAB troca a seção  ·  ENTER volta",
      "toque à esquerda ou à direita para trocar o dado  ·  o < volta" },
    { "qualquer tecla fecha", "toque para fechar" },
    // partida
    { "ENTER segue", "toque para seguir" },
    { "sequência cheia: TAB ou CTRL confirma", "sequência cheia: toque em pronto" },
    { "ENTER tira para sempre  ·  BACKSPACE volta",
      "toque num dado para escolher  ·  de novo para tirar para sempre" },
    { "setas escolhem  ·  ENTER leva  ·  X recusa",
      "toque para escolher  ·  toque de novo para levar" },
    { "setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB termina",
      "toque para escolher  ·  de novo para comprar  ·  segure para ver o dado" },
    { "ENTER nova partida", "toque para uma nova partida" },
    { "Aperte ESC (ou toque no <) de novo para sair da partida.",
      "Toque no < de novo para sair da partida." },
    { "A conexão com o oponente caiu.  ESC volta ao menu.",
      "A conexão com o oponente caiu.  O < volta ao menu." },
    { "Velocidade: rápida  (V troca)", "Velocidade: rápida" },
    { "Velocidade: normal  (V troca)", "Velocidade: normal" },
    // Modo Desafiante
    { "ENTER ou X apaga  ·  ESC deixa como está",
      "toque no perfil de novo para apagar  ·  o < deixa como está" },
    { "setas escolhem  ·  ENTER entra  ·  X apaga  ·  ESC volta",
      "toque num perfil para escolher  ·  de novo para entrar  ·  o < volta" },
    { "setas trocam o retrato  ·  digite o nome  ·  ENTER cria  ·  ESC volta",
      "as setas trocam o retrato  ·  digite o nome  ·  OK cria" },
    { "setas escolhem  ·  ENTER começa a run  ·  ESC volta",
      "toque para escolher  ·  de novo para começar  ·  o < volta" },
    { "setas escolhem  ·  ENTER confirma",
      "toque para escolher  ·  toque de novo para confirmar" },
    { "setas escolhem  ·  ENTER confirma  ·  I detalhes  ·  BACKSPACE volta",
      "toque para escolher  ·  de novo para confirmar  ·  segure para ver o dado" },
    { "ENTER segue para o mapa", "toque para seguir para o mapa" },
    { "setas escolhem  ·  ENTER leva o amuleto", "toque para escolher  ·  de novo para levar" },
    { "setas escolhem  ·  ENTER senta à mesa  ·  TAB bolsa  ·  ESC menu",
      "toque numa mesa para escolher  ·  de novo para sentar  ·  a bolsa fica no alto" },
    { "setas escolhem  ·  ENTER compra  ·  I detalhes  ·  TAB sai da loja",
      "toque para escolher  ·  de novo para comprar  ·  segure para ver o dado" },
    { "setas escolhem  ·  I detalhes  ·  TAB volta ao mapa",
      "toque num dado para escolher  ·  segure para ver tudo sobre ele" },
    { "ENTER volta ao menu", "toque para voltar ao menu" },
    { "ENTER escolhe o prêmio", "toque para escolher o prêmio" },
    // como jogar: a pagina das teclas vira a do toque
    { "Teclas", "Toque" },
    { "Setas movem  ·  ENTER escolhe e joga  ·  TAB/CTRL confirma ou para  ·  BACKSPACE tira  ·  I mostra o dado escolhido",
      "Na mesa, um toque joga: escolhe o dado, monta a sequência, para ou arrisca. Fora da mesa (menus, "
      "mapa, lojas e prêmios), o primeiro toque escolhe e o segundo confirma." },
    { "Mouse e toque também jogam: clique no que quiser. O < no alto volta ou sai da partida.",
      "Segure o dedo num dado para ver tudo sobre ele. O < no alto volta ou sai da partida." },
    { "X recusa  ·  ESC duas vezes: menu  ·  M som  ·  N música  ·  V velocidade  ·  F11 tela cheia  ·  - e + tamanho da mesa.  Boa sorte!",
      "O botão Início, embaixo da mesa, pausa o jogo e abre o volume e o brilho.  Boa sorte!" },
};

static const char *dica_toque(const char *s)
{
    if (!s) return s;
    for (unsigned i = 0; i < sizeof DICAS_TOQUE / sizeof DICAS_TOQUE[0]; i++)
        if (s[0] == DICAS_TOQUE[i][0][0] && !strcmp(s, DICAS_TOQUE[i][0])) return DICAS_TOQUE[i][1];
    return s;
}

// A mesa da partida: aqui o toque e clique direto.
static bool toque_na_mesa(void)
{
    switch (fase) {
    case F_ORDEM: case F_APOSTA: case F_ROLANDO: case F_ARRUMA: case F_RESULTADO:
    case F_JOGA: case F_VEREDITO: case F_EFEITOS: case F_FIM:
        return true;
    default:
        return false;
    }
}

// Algo por cima da tela que qualquer toque fecha ou pula.
static bool toque_por_cima(void)
{
    return (detalhe >= 0 && fase == detalhe_fase) || t_elev > 0 || t_virada > 0 || t_anuncio > 0 ||
           slot_girando();
}

static const zona_clique_t *zona_no(int x, int y)
{
    for (int i = zonas_fase == fase ? n_zonas - 1 : -1; i >= 0; i--) {
        const zona_clique_t *z = &zonas[i];
        if (x >= z->x && x < z->x + z->w && y >= z->y && y < z->y + z->h) return z;
    }
    return NULL;
}

// A mesma escolha do toque anterior? As zonas sao marcadas de novo a cada
// quadro: compara o que elas fazem, nao o endereco.
static bool mesma_zona(const zona_clique_t *a, const zona_clique_t *b)
{
    if (a->var != b->var || a->val != b->val || a->tecla != b->tecla) return false;
    return a->var || (a->x == b->x && a->y == b->y && a->w == b->w && a->h == b->h);
}

static zona_clique_t toque_armada;
static int toque_armada_fase = -1;

// Um toque curto (o dedo encostou e saiu), ja em pixels da mesa.
void dado_toque(int x, int y)
{
    const zona_clique_t *z = zona_no(x, y);
    bool armada = toque_armada_fase == fase && z && mesma_zona(z, &toque_armada);
    if (toque_na_mesa() || toque_por_cima() || !z || armada) {
        toque_armada_fase = -1;
        dado_mouse(x, y, 0);
        dado_mouse(x, y, 1);
        return;
    }
    bool muda = z->var && *z->var != z->val;
    dado_mouse(x, y, 0);                 // escolhe: o mesmo que o mouse por cima
    if (!muda) som_toca(SOM_PASSA);
    toque_armada = *z;
    toque_armada_fase = fase;
}
