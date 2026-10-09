// ===========================================================================
// Teclado
// ===========================================================================
static int eixo(int k)
{
    if (k == 'a' || k == ',' || k == 'w' || k == ';' || k == KEY_LEFT || k == KEY_UP) return -1;
    if (k == 'd' || k == '/' || k == 's' || k == '.' || k == KEY_RIGHT || k == KEY_DOWN) return 1;
    return 0;
}

// A partida acaba ao fim desta rodada? (ultima rodada, ou alguem sem
// fichas para a proxima entrada). Entao nao ha premio: o jogo encerra.
static bool ultima_rodada(void)
{
    int prox = ante_da(rodada + 1);
    return rodada >= MAX_RODADAS || J[0].fichas < prox || J[1].fichas < prox;
}

static void segue_do_resultado(void)
{
    // Quem venceu a rodada, ou quem ficou com o pote porque o outro correu,
    // escolhe um premio - menos quando a partida ja acabou.
    int w = (venc_mao == 0 || venc_mao == 1) ? venc_mao : correu_venc;
    correu_venc = -1;
    // No Desafiante nao ha premio por rodada: so ao fim da partida.
    if (w >= 0 && !ultima_rodada() && modo != M_DESAFIO) {
        primeiro = w;
        abre_premio(w);
    } else {
        if (w >= 0) primeiro = w;
        if (venc_mao == 2) primeiro ^= 1;
        nova_rodada();
    }
}

static void poe_na_fila(int i)
{
    fila_t *f = &F[vez];
    jogador_t *p = &J[vez];
    int c = p->mao[i];
    int ja = -1;
    for (int k = 0; k < f->n; k++) if (f->idx[k] == c) ja = k;
    if (ja >= 0) {
        for (int k = ja; k + 1 < f->n; k++) {
            f->idx[k] = f->idx[k + 1];
            f->tipo[k] = f->tipo[k + 1];
            f->fixo[k] = f->fixo[k + 1];
        }
        f->n--;
    } else if (f->n < max_fila(vez)) {
        f->idx[f->n] = (int8_t)c;
        f->tipo[f->n] = p->col[c].tipo;
        f->fixo[f->n] = p->col[c].fixo;
        f->n++;
    }
    if (!anuncia_maquina()) som_toca(SOM_TIQUE);
}

void dado_tecla(const key_event_t *ev_)
{
    int k = ev_->key;
    if (k >= 'A' && k <= 'Z') k += 32;

    if (t_elev > 0) { t_elev = 0; return; }        // qualquer tecla pula o elevador
    if (t_virada > 0) { t_virada = 0; return; }    // e o anuncio da virada
    if (t_anuncio > 0 && k != KEY_ESC) { t_anuncio = 0; return; }   // e o que a maquina fez
    if (k == 'v' && !dado_digitando()) {
        rapido = !rapido;
        salva_grava("vel", rapido ? "1" : "0");
        t_aviso_vel = 1.6f;
        som_toca(SOM_TIQUE);
        return;
    }
    if (k == 'm' && !dado_digitando()) {
        bool on = som_liga(!som_ligado());
        if (!on && som_falhou()) snprintf(nota, sizeof nota, "sem saída de som neste computador");
        if (on) som_toca(SOM_FICHA);
        return;
    }
    // Detalhes do dado (tecla I): so da tela local, nunca vai para o rival.
    if (detalhe >= 0 && fase == detalhe_fase) { detalhe = -1; return; }   // qualquer tecla fecha
    detalhe = -1;
    // Roleta girando: a tecla so trava os rolos, aqui mesmo (nao vai para o
    // rival, que ve a propria roleta).
    if (slot_girando() && k != KEY_ESC) { slot_pula(); return; }
    if (k == 'i' && !dado_digitando() && tipo_em_foco() >= 0) {
        detalhe = tipo_em_foco();
        detalhe_fase = fase;
        som_toca(SOM_TIQUE);
        return;
    }
    if (k == KEY_ESC) { trata_esc(); return; }
    t_sair = -1;
    if (ia_na_vez()) return;             // a vez e do aparelho: so ele escolhe
    if (modo == M_ONLINE) {
        if (rede_na_vez()) return;       // a vez e do rival: as teclas vem dele
        if (aceita_escolha()) rede_envia_tecla(k);
    }
    trata_tecla(k);
}

static void manda_tecla(int k)
{
    key_event_t ev = { 0 };
    ev.key = k;
    dado_tecla(&ev);
}

// O mouse esta sobre algo clicavel? (a plataforma troca a seta pela maozinha)
bool dado_mouse_clicavel(void)
{
    if (zonas_fase != fase) return false;
    bool alheia = ia_na_vez() || (modo == M_ONLINE && rede_na_vez());
    for (int i = 0; i < n_zonas; i++)
        if (mouse_em(zonas[i].x, zonas[i].y, zonas[i].w, zonas[i].h) && (!alheia || zonas[i].tecla == KEY_ESC))
            return true;
    return false;
}

// Mouse, ja nas coordenadas da tela do jogo. acao: 0 moveu, 1 clique
// esquerdo, 2 clique direito (o mesmo que BACKSPACE: volta ou desfaz).
void dado_mouse(int x, int y, int acao)
{
    mouse_x = x;
    mouse_y = y;
    if (acao == 2) { manda_tecla(KEY_BKSP); return; }
    if (detalhe >= 0 && fase == detalhe_fase) {         // um clique fecha os detalhes
        if (acao == 1) manda_tecla(KEY_ENTER);
        return;
    }
    const zona_clique_t *z = NULL;
    for (int i = zonas_fase == fase ? n_zonas - 1 : -1; i >= 0 && !z; i--)
        if (mouse_em(zonas[i].x, zonas[i].y, zonas[i].w, zonas[i].h)) z = &zonas[i];
    // Na vez da IA ou do rival online, o mouse nao mexe em nada; so o "<" de
    // sair vale sempre.
    if (ia_na_vez() || (modo == M_ONLINE && rede_na_vez())) {
        if (acao == 1 && z && z->tecla == KEY_ESC) manda_tecla(KEY_ESC);
        return;
    }

    if (acao == 0) {
        // Passar por cima escolhe. No online o cursor so anda pelas teclas,
        // que vao para o rival: la so o clique vale.
        if (z && z->var && *z->var != z->val && modo != M_ONLINE) {
            *z->var = z->val;
            som_toca(SOM_PASSA);                  // mais curto e baixo que o de escolher
        }
        return;
    }
    if (z) {
        if (z->var && modo == M_ONLINE) {
            for (int i = 0; i < 64 && *z->var != z->val; i++) manda_tecla(KEY_RIGHT);
            if (*z->var != z->val) return;
        } else if (z->var) {
            *z->var = z->val;
        }
        manda_tecla(z->tecla);
        return;
    }
    // Fora dos botoes: nas telas de "aperte para seguir", o clique segue.
    switch (fase) {
    case F_CATALOGO:
        manda_tecla(x < GFX_W / 2 ? KEY_LEFT : KEY_RIGHT);
        break;
    case F_TUTORIAL:
        manda_tecla(x < GFX_W / 3 ? KEY_LEFT : KEY_ENTER);
        break;
    case F_ABERTURA:
        if (!abertura_pronta || t_fase < 0.75f) manda_tecla(KEY_ENTER);   // pula a entrada
        break;
    case F_ONLINE:
        if (!rede_disponivel() || rede_estado() == REDE_ERRO) manda_tecla(KEY_ENTER);
        break;
    case F_RESULTADO: case F_FIM: case F_RFIM: case F_CREDITOS: case F_VEREDITO: case F_EFEITOS:
        manda_tecla(KEY_ENTER);
        break;
    case F_PREMIO:
        if (slot_girando()) manda_tecla(KEY_ENTER);       // trava a roleta
        break;
    default:
        break;
    }
}

// A tecla ja traduzida, venha de quem vier: pessoa ou IA.
static void trata_tecla(int k)
{
    int e = eixo(k);
    if (e && (fase == F_ORDEM || fase == F_APOSTA || fase == F_JOGA ||
              fase == F_PREMIO || fase == F_LOJA))
        som_toca(SOM_TIQUE);

    switch (fase) {
    case F_ABERTURA:
        if (!abertura_pronta || t_fase < 0.75f) {             // pula para o menu
            for (int i = 0; i < n_voo; i++) {
                voo[i].parado = true; voo[i].face = voo[i].valor;
                voo[i].vx = voo[i].vy = 0;
            }
            abertura_pronta = true;
            t_fase = 1.0f;
            return;
        }
        {
        const char *op[MN_N];
        int n_op = menu_opcoes(op);
        if (e) {
            menu_cur = (menu_cur + e + n_op) % n_op;
            sacode = 1;                              // como se alguem mexesse na mesa
            som_toca(SOM_TIQUE);
            return;
        }
        if (k == KEY_BKSP && menu_tela) { menu_volta(); return; }
        if (k >= '1' && k < '1' + n_op) { menu_cur = k - '1'; sacode = 1; }
        if (k != KEY_ENTER && !(k >= '1' && k < '1' + n_op)) return;
        }
        som_toca(SOM_FICHA);
        sacode = 1;
        if (menu_tela == MT_UM) {
            if (menu_cur == 0) { menu_tela = MT_NIVEL; menu_cur = 1; }
            else des_abre_perfis();
            return;
        }
        if (menu_tela == MT_NIVEL) {                // partida rapida no nivel escolhido
            modo = M_UM;
            cpu = 2;
            remoto = 0;
            baixo = 0;
            nivel_jog[1] = menu_cur;
            ousadia_jog[1] = 0;
            poe_nomes();
            abre_loja();
            return;
        }
        if (menu_tela == MT_DESAFIO) {
            if (menu_cur == 0) des_continua(); else des_escolhe_nova();
            return;
        }
        switch (menu_cur) {
        case MN_UM:
            menu_tela = MT_UM;
            menu_cur = 0;
            break;
        case MN_DOIS:
            modo = M_DOIS;
            cpu = 0;
            remoto = 0;
            baixo = 0;
            poe_nomes();
            abre_loja();
            break;
        case MN_ONLINE:
            online_cur = 0;
            digitando = false;
            t_fase = 0;
            fase = F_ONLINE;
            break;
        case MN_DADOS:    t_fase = 0; fase = F_CATALOGO; break;
        default:          tut_pag = 0; t_fase = 0; fase = F_TUTORIAL; break;
        }
        return;

    case F_TUTORIAL:
        if (e) {
            int n = tut_pag + e;
            if (n >= 0 && n < N_TUTORIAL) { tut_pag = n; t_fase = 0; som_toca(SOM_TIQUE); }
            return;
        }
        if (k == KEY_ENTER) {
            if (tut_pag + 1 < N_TUTORIAL) { tut_pag++; t_fase = 0; som_toca(SOM_TIQUE); }
            else ao_menu_sem_partida();
        }
        if (k == KEY_BKSP) ao_menu_sem_partida();
        return;

    case F_ONLINE: {
        if (!rede_disponivel()) {
            if (k == KEY_ENTER || k == KEY_BKSP) ao_menu_sem_partida();
            return;
        }
        int st = rede_estado();
        if (st == REDE_ERRO) { if (k == KEY_ENTER) rede_sai(); return; }
        if (st != REDE_PARADA) return;
        if (digitando) {
            int n = (int)strlen(codigo_dig);
            if (k == KEY_BKSP) { if (n) codigo_dig[n - 1] = 0; else digitando = false; return; }
            if (k == KEY_ENTER && n == 4) { rede_entra(codigo_dig); return; }
            bool letra = k >= 'a' && k <= 'z', num = k >= '0' && k <= '9';
            if (n < 4 && (letra || num)) {
                codigo_dig[n] = (char)(letra ? k - 32 : k);
                codigo_dig[n + 1] = 0;
                som_toca(SOM_TIQUE);
            }
            return;
        }
        if (e) { online_cur ^= 1; som_toca(SOM_TIQUE); return; }
        if (k == KEY_ENTER) {
            if (online_cur == 0) rede_cria();
            else { digitando = true; codigo_dig[0] = 0; }
        }
        return;
    }

    case F_CATALOGO:
        // Cima e baixo (e TAB) trocam a secao; os lados passam as paginas.
        if (k == KEY_UP || k == KEY_DOWN || k == 'w' || k == 's' || k == KEY_TAB) {
            int sec = k == KEY_TAB ? !cat_sec : (k == KEY_DOWN || k == 's');
            if (sec != cat_sec) { cat_sec = sec; t_fase = 0; som_toca(SOM_TIQUE); }
            return;
        }
        if (e) {
            if (cat_sec == 0) cat_i = (cat_i + e + D_N) % D_N;
            else cat_a = (cat_a + e + A_N) % A_N;
            t_fase = 0;
            som_toca(SOM_TIQUE);
            return;
        }
        if (k == KEY_ENTER || k == KEY_BKSP) ao_menu_sem_partida();
        return;

    case F_LOJA: {
        jogador_t *p = &J[vez];
        if (e) { cursor = (cursor + e + N_LOJA) % N_LOJA; return; }
        if (k >= '1' && k <= '5') cursor = k - '1';
        if (k == KEY_ENTER || (k >= '1' && k <= '5')) {
            int tp = loja[cursor].tipo;
            if (levou[vez][cursor]) return;
            if (p->fichas < preco_loja(tp) || p->n_col >= lim_col()) return;
            p->fichas -= preco_loja(tp);
            p->col[p->n_col] = loja[cursor];
            p->onde[p->n_col++] = NA_BOLSA;             // a bolsa comeca cheia
            levou[vez][cursor] = true;
            som_toca(SOM_FICHA);
            return;
        }
        if (k == KEY_TAB || k == 'x') {
            if (vez == loja_primeiro) { vez = outro(vez); cursor = 0; }
            else nova_rodada();
        }
        return;
    }

    case F_ORDEM: {
        jogador_t *p = &J[vez];
        fila_t *f = &F[vez];
        if (!p->n_mao) return;
        if (e) { cursor = (cursor + e + p->n_mao) % p->n_mao; return; }
        if (k >= '1' && k <= '8' && k - '1' < p->n_mao) { cursor = k - '1'; poe_na_fila(cursor); return; }
        if (k == KEY_ENTER) { poe_na_fila(cursor); return; }
        if (k == KEY_BKSP && f->n) { f->n--; return; }
        if ((k == KEY_TAB || k == 'x') && f->n >= 1) { cursor = 0; fase = F_JOGA; }
        return;
    }

    case F_JOGA: {
        fila_t *f = &F[vez];
        bool pode_jogar = f->lancados < f->n, pode_parar = f->lancados > 0;
        // Antes do primeiro lance, o segundo botao e "voltar": a sequencia
        // ainda pode ser mudada.
        bool pode_voltar = f->lancados == 0;
        if (e) { if (pode_parar || pode_voltar) cursor ^= 1; return; }
        if (k == KEY_BKSP && pode_voltar) { cursor = 0; t_fase = 1; fase = F_ORDEM; return; }
        if (k == KEY_TAB || k == 'x') { if (pode_parar) termina_turno(); return; }
        if (k == KEY_ENTER) {
            if (cursor == 1 && pode_voltar) { cursor = 0; t_fase = 1; fase = F_ORDEM; }
            else if (cursor == 1 && pode_parar) termina_turno();
            else if (pode_jogar) lanca_proximo();
        }
        return;
    }

    case F_VEREDITO:
        t_fase = 10.0f;
        return;

    case F_EFEITOS:
        t_fase = 10.0f;                      // qualquer tecla adianta o evento
        return;

    case F_APOSTA: {
        int op[4], n = opcoes(op);
        if (e) { cursor = (cursor + e + n) % n; return; }
        if (k >= '1' && k <= '0' + n) { executa(op[k - '1']); return; }
        if (k == KEY_ENTER) executa(op[cursor]);
        return;
    }

    case F_RESULTADO:
        if (k == KEY_ENTER) segue_do_resultado();
        return;

    case F_PREMIO: {
        jogador_t *p = &J[vez];
        slot_pula();                                // tecla do rival: a roleta daqui trava
        if (t_tira > 0) return;                     // dado saindo: espera
        if (removendo) {
            if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= GRADE) cursor -= GRADE; return; }
            if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + GRADE < p->n_col) cursor += GRADE; return; }
            if (e) { cursor = (cursor + e + p->n_col) % p->n_col; return; }
            if (k == KEY_BKSP || k == 'x') { removendo = false; cursor = 3; return; }
            if (k == KEY_ENTER) { tira_i = cursor; t_tira = 0.001f; som_toca(SOM_ANULA); }
            return;
        }
        if (e) { cursor = (cursor + e + 4) % 4; return; }
        if (k == 'x') { if (modo == M_DESAFIO) des_premio_feito(); else nova_rodada(); return; }
        if (k == KEY_ENTER) {
            if (cursor == 3) {
                if (p->n_col > N_FILA) { removendo = true; cursor = 0; }
                return;
            }
            // O dado ganho fica fora da bolsa ate o proximo embaralho: o
            // premio chega, mas nao na rodada seguinte.
            if (p->n_col < lim_col()) {
                p->col[p->n_col] = oferta[cursor];
                p->onde[p->n_col++] = FORA;
            }
            if (modo == M_DESAFIO) des_premio_feito();
            else nova_rodada();
        }
        return;
    }

    case F_FIM:
        if (k != KEY_ENTER) return;
        if (modo == M_DESAFIO) des_apos_fim();
        else dado_inicia(partidas + 1);
        return;

    default:
        des_tecla(k, e);                     // telas do Modo Desafiante
        return;
    }
}
