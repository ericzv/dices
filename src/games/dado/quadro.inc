// ===========================================================================
// O quadro: as dicas do mouse e cada tela desenhada por fase
// ===========================================================================
static void desenha_tudo(void);

// ---------------------------------------------------------------------------
// Dica do mouse: uma caixa com o nome e o que a coisa faz, por cima de tudo.
// ---------------------------------------------------------------------------

static void dica_textos(int tipo, int id, const char **nome, uint16_t *cor, const char **desc)
{
    if (tipo == DICA_DADO && id == DICA_EMBRULHO) {
        *nome = "Embrulho"; *cor = cor_dado(D_PRESENTE);
        *desc = "Um dos seus dados, embrulhado pelo Papai Noel: só se sabe qual é quando você o lança. "
                "No fim da rodada, volta a ser o que era.";
    } else if (tipo == DICA_DADO)  { *nome = TIPO[id].nome; *cor = cor_dado(id); *desc = TEXTO_MEDIO[id]; }
    else if (tipo == DICA_AMULETO) { *nome = AMULETO[id].nome; *cor = AMULETO[id].cor; *desc = AMULETO[id].desc; }
    else if (tipo == DICA_ADESIVO) { *nome = ADESIVO[id].nome; *cor = ADESIVO[id].luz; *desc = AD_TEXTO[id]; }
    else {
        *nome = PERSO[id].trunfo; *cor = C_ROSA; *desc = regra_perso(id);
        if (id == PERSO_CORINGA)                         // o Coringa: os detalhes da copia
            *desc = des_dificuldade() >= CORINGA_SOME
                  ? "O 4º dado dele é a cópia do seu dado que tirou o maior valor no seu turno (empate: o último). "
                    "Se ele abre a rodada, é um \"?\" que só vira e é lançado depois que você joga. "
                    "Da 5ª rodada em diante, cada dado seu que terminar anulado é destruído."
                  : "O 4º dado dele é a cópia do seu dado que tirou o maior valor no seu turno (empate: o último). "
                    "Se ele abre a rodada, é um \"?\" que só vira e é lançado depois que você joga.";
        int v = virada_de(id);
        if (v) {                                         // o chefe tambem conta a virada
            static char t[200];
            char vt[96];
            virada_txt(chefe_idx(id), v, false, vt, sizeof vt);
            snprintf(t, sizeof t, "%s Virada na 5ª rodada: %s", regra_perso(id), vt);
            *desc = t;
        }
    }
}

// Os adesivos de um dado com a peca: os lados que tem adesivo. Com ate dois,
// cada um vem com a regra; com mais, uma linha so por adesivo.
#define DICA_AD_LIN 16                   // a linha do nome do adesivo
#define DICA_AD_W   (260 - 16 - 16)      // a largura da regra, ao lado do simbolo
static int dica_adesivos(const dica_t *d, int *lados)
{
    if (d->tipo != DICA_DADO || !d->com_peca) return 0;
    int n = 0;
    for (int l = 1; l <= AD_LADOS; l++) if (ad_no_lado(&d->pc, l)) lados[n++] = l;
    return n;
}
static int dica_ad_altura(const dica_t *d)
{
    int lados[AD_LADOS], n = dica_adesivos(d, lados), h = 0;
    if (!n) return 0;
    quebra_so_conta = true;
    for (int i = 0; i < n; i++)
        h += DICA_AD_LIN + (n <= 2 ? quebra_linhas(0, 0, DICA_AD_W, 3, AD_TEXTO[ad_no_lado(&d->pc, lados[i])], 0) * LINHA_FINA + 2 : 0);
    quebra_so_conta = false;
    return h + 8;                                        // e o filete que separa do dado
}

// A caixa da dica sob o mouse, se houver. Devolve o retangulo em r.
static bool dica_rect(int *r, const dica_t **qual)
{
    if (mouse_x < 0 || (detalhe >= 0 && fase == detalhe_fase)) return false;
    const dica_t *d = NULL;
    for (int i = n_dicas - 1; i >= 0 && !d; i--) {
        if (dicas[i].tipo == DICA_TRUNFO && modo == M_DESAFIO && !trunfo_ativo(dicas[i].id)) continue;   // sem trunfo ainda: nada
        if (mouse_em(dicas[i].x, dicas[i].y, dicas[i].w, dicas[i].h)) d = &dicas[i];
    }
    if (!d) return false;
    const char *nome, *desc;
    uint16_t cor;
    dica_textos(d->tipo, d->id, &nome, &cor, &desc);
    int larg = 260;
    quebra_so_conta = true;                              // so conta as linhas
    int linhas = quebra_linhas(0, 0, larg - 16, 7, desc, 0);
    quebra_so_conta = false;
    int w = larg, h = 10 + (TXT_H + 2) + linhas * LINHA_FINA + 6 + dica_ad_altura(d);
    // Sempre para baixo, centrada embaixo da coisa: nao cobre o que esta em
    // cima dela na mesa. Se nao couber, encosta no pe da tela; e se assim
    // fosse cobrir a propria coisa, vai para o lado dela.
    int x = d->x + d->w / 2 - w / 2, y = d->y + d->h + 4;
    if (y + h > GFX_H - 4) {
        y = GFX_H - 4 - h;
        if (y < d->y + d->h) {
            x = d->x + d->w + 6;
            if (x + w > GFX_W - 4) x = d->x - 6 - w;
        }
    }
    if (x + w > GFX_W - 4) x = GFX_W - 4 - w;
    if (x < 4) x = 4;
    if (y < 4) y = 4;
    r[0] = x; r[1] = y; r[2] = w; r[3] = h;
    *qual = d;
    return true;
}

static void desenha_dica(const int *r, const dica_t *d)
{
    const char *nome, *desc;
    uint16_t cor;
    dica_textos(d->tipo, d->id, &nome, &cor, &desc);
    ui_painel(r[0], r[1], r[2], r[3]);
    gfx_rect(r[0] + 3, r[1] + 3, 3, r[3] - 6, cor);             // a cor da coisa, na beira
    int x = r[0] + 8, y = r[1] + 5;
    int w = gfx_texto(x, y, nome, cor, 1, true);
    if (d->tipo == DICA_DADO && d->id != DICA_EMBRULHO) {   // "D4 · raro", miudo, ao lado
        char c2[32];
        classe(d->id, c2, sizeof c2);
        int wc = ui_selo(x + w + 9, y + 3, c2, cor_rar(d->id), GFX_MIUDO);
        icone_protecao(x + w + 8 + wc + 6, y + 3, nao_anulavel_t(d->id), nao_roubavel_t(d->id));   // as protecoes dele
    }
    int lin = quebra_linhas(x, y + TXT_H + 2, r[2] - 16, 7, desc, C_MARFIM_S);
    // Os adesivos colados nele: o simbolo no vinil, o nome na cor dele, o lado
    // e (com ate dois) a regra.
    int lados[AD_LADOS], n = dica_adesivos(d, lados);
    if (!n) return;
    int yy = y + TXT_H + 2 + lin * LINHA_FINA + 4;
    gfx_rect(x, yy, r[2] - 16, 1, C_LATAO_ESC);
    yy += 5;
    for (int i = 0; i < n; i++) {
        int a = ad_no_lado(&d->pc, lados[i]);
        const adesivo_t *ad = &ADESIVO[a];
        ad_solto_d(x + 6, yy + 7, a, 1, 0);
        int wn = gfx_texto(x + 18, yy - 3, ad->nome, ad->luz, 1, true);
        char s[24];
        snprintf(s, sizeof s, "· lado %d", lados[i]);
        gfx_texto(x + 18 + wn + 6, yy - 2, s, C_MARFIM_S, GFX_FINO, false);
        yy += DICA_AD_LIN;
        if (n <= 2) yy += quebra_linhas(x + 18, yy - 2, DICA_AD_W, 3, AD_TEXTO[a], C_TEXTO_M) * LINHA_FINA + 2;
    }
}

// ---------------------------------------------------------------------------
// Transicao entre telas: ao trocar de tela, o ultimo quadro da velha fica
// guardado e a nova entra por cima dele numa varredura em diagonal, da
// esquerda para a direita, com a beira pontilhada (o Bayer em blocos de 2x2,
// o mesmo grao dos dados), em TRANS_T segundos. A partida inteira conta como
// uma tela so: as fases dela se sucedem na mesma mesa.
// ---------------------------------------------------------------------------
#define TRANS_T    0.3f
#define TRANS_BEIRA 0.16f                    // a largura da beira pontilhada (0..1 da tela)
static uint16_t trans_buf[GFX_W * GFX_H];
static int trans_tela = -1;                  // a tela do ultimo quadro

static int tela_atual(void)
{
    bool partida = fase == F_ORDEM || fase == F_JOGA || fase == F_ROLANDO || fase == F_ARRUMA
                || fase == F_VEREDITO || fase == F_APOSTA || fase == F_EFEITOS || fase == F_RESULTADO;
    int t = (partida ? F_JOGA : fase) * 32;
    if (fase == F_ABERTURA) t += menu_tela;
    if (fase == F_TUTORIAL) t += tut_pag;
    if (fase == F_RLOJA) t += lj_tirando ? 1 : lj_colando ? 1 + lj_colando : lj_pac ? 4 : 0;
    if (fase == F_PREMIO) t += removendo ? 1 : premio_colando() ? 2 : 0;
    return t;
}

// No fim do quadro, com as cores ja na paleta (as duas telas misturam so
// pixels prontos).
static void transicao(void)
{
    uint16_t *fb = display_fb();
    int tela = tela_atual();
    if (trans_tela >= 0 && tela != trans_tela) t_trans = 0;   // trocou: a velha esta em trans_buf
    trans_tela = tela;
    if (t_trans >= 0 && t_trans >= TRANS_T) t_trans = -1;
    if (t_trans >= 0) {
        // A beira e uma faixa escura pontilhada, como a sombra de uma cortina
        // passando: do lado novo ela vai abrindo em pontos, do velho fecha.
        float e = t_trans / TRANS_T * (1 + TRANS_BEIRA), tot = GFX_W + GFX_H * 0.6f;
        for (int y = 0; y < GFX_H; y++)
            for (int x = 0; x < GFX_W; x++) {
                float s = (x + y * 0.6f) / tot;           // 0 no canto de cima a esquerda, 1 no de baixo a direita
                float k = (e - s) / TRANS_BEIRA;          // 1: ja na tela nova; 0: ainda na velha
                if (k >= 1) continue;
                int i = y * GFX_W + x;
                float b = bayer(x >> 1, y >> 1);
                if (k <= 0) fb[i] = trans_buf[i];
                else if (b >= k) fb[i] = b < k + 0.35f ? C_BARRA : trans_buf[i];
            }
        return;                                           // a velha fica guardada ate a varredura acabar
    }
    memcpy(trans_buf, fb, sizeof trans_buf);
}

void dado_desenha(void)
{
    n_zonas = 0;
    n_dicas = 0;
    foco_bx = -1;
    zonas_fase = fase;
    bolsas_vistas = 0;
    desenha_tudo();
    desenha_tiro();
    desenha_evolui();
    desenha_buraco_suga();
    desenha_flutua();
    desenha_tiro_fogo();
    desenha_big_bang();
    // O que a maquina fez fica por cima dos numeros que ainda sobem dos dados.
    if (t_anuncio > 0 && (fase == F_ORDEM || fase == F_APOSTA || fase == F_JOGA || fase == F_LOJA))
        desenha_anuncio();
    balao_oponente();
    desenha_confete(true);
    bool menu = fase == F_CATALOGO || fase == F_TUTORIAL || fase == F_ONLINE || fase == F_ABERTURA;
    if (!menu) {
        desenha_detalhe();
        desenha_avisos();
        desenha_aviso_entrada();
        des_elevador();
        des_virada_anuncio();
        des_bolsa_nova();
        desenha_bolsa_pop();
    } else bolsa_pop = -1;
    int r[4];
    const dica_t *d = NULL;
    bool tem_dica = !menu && dica_rect(r, &d);
    if (tem_dica) desenha_dica(r, d);
    desenha_fantasma_voo();
    desenha_misterio();
    desenha_clarao();                                    // o golpe: o clarao de quem acertou
    aplica_tremor();
    gfx_quantiza();                                      // o quadro so com cores da paleta
    transicao();
}

static void desenha_tudo(void)
{
    mesa();
    desenha_confete(false);                  // o papel cai por baixo dos paineis
    if (des_desenha()) return;
    if (fase == F_CATALOGO) { desenha_catalogo(); return; }
    if (fase == F_TUTORIAL) { desenha_tutorial(); return; }
    if (fase == F_ONLINE)   { desenha_online(); return; }
    if (fase == F_ABERTURA) {
        desenha_abertura();
        return;
    }
    faixa_jogador(1);
    faixa_jogador(0);
    if (modo == M_DESAFIO) desenha_fosforo();    // a Caixa de Fosforos: a mao dele na luz

    switch (fase) {
    // A loja e o premio da maquina contra uma pessoa nao aparecem: so a
    // mesa, e o anuncio do que ela levou por cima.
    case F_LOJA:   if (!anuncia_maquina() && t_anuncio <= 0) desenha_loja();   break;
    case F_PREMIO: if (!anuncia_maquina()) desenha_premio(); break;
    case F_FIM:    if (modo == M_DESAFIO) desenha_fim_partida_run(); else desenha_fim(); break;
    case F_ORDEM:
        desenha_fila(outro(vez));
        if (t_anuncio > 0) break;                // o anuncio vem por cima, no fim
        if (anuncia_maquina()) {                 // a maquina monta sem menu
            desenha_fila(vez);
            desenha_maquina_monta();
            break;
        }
        desenha_ordem();
        break;
    default:
        desenha_fila(outro(vez));
        desenha_fila(vez);
        desenha_devolvidos();
        desenha_jingle_copias();                 // as copias dos Jingle Bells voando para a bolsa
        desenha_hora();                          // a hora da verdade: a mesa escurece em volta do dado
        if (t_anuncio > 0 && (fase == F_APOSTA || fase == F_JOGA)) pote_mesa();   // o anuncio vem no fim
        else if (fase == F_APOSTA)    desenha_aposta();
        else if (fase == F_RESULTADO) { desenha_log(); desenha_resultado(); }
        else if (fase == F_EFEITOS)   { desenha_log(); desenha_efeitos(); }
        else                          desenha_joga();
        if (fase == F_ROLANDO || fase == F_ARRUMA) desenha_lance();
        break;
    }
}
