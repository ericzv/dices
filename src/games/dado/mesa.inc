// ===========================================================================
// Desenho da mesa: o lance, as faixas dos jogadores, as filas, os
// ataques e o pote
// ===========================================================================
// O nome do jogador na faixa.
static int txt_nome(int x, int y, int j)
{
    return gfx_texto(x, y, J[j].nome, J[j].cor, 1, true);
}

static void desenha_lance(void)
{
    for (int i = n_voo - 1; i >= 0; i--) {
        voo_t *v = &voo[i];
        float r = R_DADO + (v->pouso > 0 ? v->pouso * 16 : 0);
        bool descartado = false;
        // O lance menor do Viciado: riscado e encolhendo enquanto o maior
        // vai para a fila.
        if (v->fantasma && fase == F_ARRUMA) {
            float q = t_fase / 0.45f;
            q = q > 1 ? 1 : q;
            r = R_DADO * (1 - q * 0.4f);
            descartado = true;
        }
        // Poeira do feltro quando o dado assenta.
        if (v->pouso > 0 && !v->fantasma) {
            float q = 1 - v->pouso / 0.18f;
            for (int k = 0; k < 10; k++) {
                float a = k * 0.628f + v->x * 0.01f;
                int px = (int)(v->x + cosf(a) * (R_DADO + q * 20));
                int py = (int)(v->y + sinf(a) * (R_DADO + q * 20) * 0.6f + R_DADO * 0.3f);
                int t = q < 0.5f ? 3 : 2;
                gfx_rect(px - 1, py - 1, t, t, gfx_mistura(C_FELTRO2, C_MARFIM_S, 30 - (int)(q * 20)));
            }
        }
        // Rolando: quica, e a sombra se afasta quando ele esta no alto.
        sombra_alt = 0;
        if (!v->parado) {
            float vel = sqrtf(v->vx * v->vx + v->vy * v->vy);
            float h = fabsf(sinf(v->giro * 0.9f)) * fminf(1, vel / 420);
            r *= 1 + 0.16f * h;
            sombra_alt = h * 7;
        }
        if (TIPO[v->tipo].lados == 2 && !v->parado && !descartado) {
            float c = cosf(v->giro * 1.6f);            // moeda gira no ar
            moeda_girando_arte(v->x, v->y, r, v->tipo, v->dono, c, c >= 0 ? v->face : 3 - v->face);
            sombra_alt = 0;
            continue;
        }
        // Na tela inicial, depois de assentar, os dados flutuam de leve (o
        // balanco entra devagar, sem pulo), e a sombra acompanha.
        // Quando o cursor do menu anda, chacoalham: pulinhos, um tremor de
        // lado e um giro curto, que se acalmam em menos de um segundo.
        float bob = 0, dx = 0, ang = v->giro;
        if (fase == F_ABERTURA && abertura_pronta && v->parado) {
            float entra = fminf(1, t_fase / 0.8f);
            float alto = (1 + sinf(t_fase * 2.2f + i * 1.9f)) * 3 * entra;   // 0..6 px
            float s = sacode * sacode;
            alto += s * 11 * fabsf(sinf(t_fase * 15 + i * 1.3f));
            dx = s * 3 * sinf(t_fase * 29 + i * 2.1f);
            ang += s * 0.32f * sinf(t_fase * 18 + i * 1.7f);
            bob = -alto;
            sombra_alt = alto * 0.6f;
        }
        desenha_dado(v->x + dx, v->y + bob, r, v->tipo, v->dono, ang,
                     v->parado ? v->valor : v->face, 2, descartado);
        sombra_alt = 0;
    }
    sombra_alt = 0;
}

static void peca_parada(float x, float y, float r, peca_t p, int dono)
{
    marca_nivel = p.fixo;
    desenha_dado(x, y, r, p.tipo, dono, 0, p.fixo, 0, false);
    marca_nivel = -1;
}

// Moldura de destaque em volta de um dado da fila.
static void aro(int x, int y, uint16_t c)
{
    int m = R_DADO + 7;
    ui_contorno(x - m, y - m, 2 * m, 2 * m, c);
    ui_contorno(x - m + 1, y - m + 1, 2 * m - 2, 2 * m - 2, c);
}

// Etiqueta no canto do dado (x2, +8, =9), com fundo para ler sobre o feltro.
static void etiqueta(int x, int y, const char *s)
{
    int w = gfx_largura(s, 1) + 8;
    int ex = x + R_DADO - 8, ey = y - R_DADO - 14;
    uint16_t c = s[0] == '-' ? C_VINHO_CLR : C_OURO;
    gfx_rect(ex + 1, ey + 1, w - 2, TXT_H, C_PLACA);
    ui_contorno(ex, ey, w, TXT_H + 2, c);
    txt(ex + 4, ey + 1, s, c, false);
}

// Painel escuro e translucido atras de um bloco de texto no meio da mesa.
static void painel(int y, int h, int w)
{
    ui_painel(GFX_W / 2 - w / 2, y, w, h);
}

// Largura de um painel: o que ele mostra, com folga dos lados.
static int larg_painel(int conteudo, int minimo)
{
    int w = conteudo + 36;
    return w < minimo ? minimo : w;
}

// Alto-falante miudo na faixa de ERIC quando o som esta ligado.
static void icone_som(int x, int y)
{
    gfx_rect(x, y + 4, 4, 6, C_OURO);
    for (int i = 0; i < 4; i++) gfx_rect(x + 4 + i, y + 4 - i, 1, 6 + 2 * i, C_OURO);
    for (int k = 0; k < 2; k++) {
        int d = 11 + 4 * k;
        gfx_rect(x + d, y + 3 - 2 * k, 1, 8 + 4 * k, C_OURO);
    }
}

static void icone_bolsa(int cx, int cy);
static void amuletos_faixa(int j, int x, int y);   // Modo Desafiante (desafio.inc)
static int  retrato_faixa(int j, int x, int y0);
static int  fichas_vis(int j);   // devolve onde o nome comeca

static void faixa_jogador(int j)
{
    jogador_t *p = &J[j];
    bool cima = j != baixo;
    int y0 = cima ? 0 : GFX_H - FAIXA_H;
    gfx_rect(0, y0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, cima ? FAIXA_H - 2 : y0, GFX_W, 2, C_LATAO_ESC);
    int ty = y0 + (FAIXA_H - TXT_H) / 2 - 2;

    bool da_vez = fase != F_RESULTADO && fase != F_EFEITOS && fase != F_FIM
                  && fase != F_ROLANDO && fase != F_ARRUMA && vez == j;
    if (da_vez) gfx_rect(0, y0 + 2, 5, FAIXA_H - 4, p->cor);
    // Na faixa de cima, o "<" sai da partida (pede confirmacao, como o ESC):
    // no celular nao ha tecla.
    int x0 = cima ? botao_voltar(KEY_ESC) - 4 : 6;
    int xn = modo == M_DESAFIO ? retrato_faixa(j, x0, y0) : (cima ? x0 + 4 : 18);
    int x = xn + txt_nome(xn, ty, j);
    ficha(x + 22, y0 + FAIXA_H / 2, j == 0 ? C_LATAO : C_ROSA);
    ficha_px[j] = x + 22; ficha_py[j] = y0 + FAIXA_H / 2;
    char s[48];
    snprintf(s, sizeof s, "%d", fichas_vis(j));
    txt(x + 36, ty, s, C_MARFIM, true);
    int fim_fichas = x + 36 + gfx_largura(s, 1);

    // A rodada mora na faixa de cima; o som, na de baixo.
    if (cima && rodada > 0 && fase != F_LOJA) {
        int max_r = modo == M_DESAFIO ? DES_RODADAS : MAX_RODADAS;
        if (rodada > max_r) snprintf(s, sizeof s, "rodada extra  ·  entrada %d", ante_da(rodada));
        else snprintf(s, sizeof s, "rodada %d de %d  ·  entrada %d", rodada, max_r, ante_da(rodada));
        int cx = GFX_W / 2, meio = gfx_largura(s, 1) / 2;
        if (cx - meio < fim_fichas + 20) cx = fim_fichas + 20 + meio;   // nome comprido: anda para a direita
        txt_c(cx, ty, s, t_aviso_entrada > 0 ? C_OURO : C_TEXTO_M, t_aviso_entrada > 0);
    }
    if (!cima && som_ligado()) icone_som(GFX_W / 2 - 8, y0 + FAIXA_H / 2 - 7);
    if (!cima && modo != M_ONLINE) {        // a velocidade, que o toque tambem troca
        const char *v = rapido ? "» rápido" : "» normal";
        int vx = GFX_W / 2 + 22;
        gfx_texto(vx, y0 + FAIXA_H / 2 - 10, v, rapido ? C_OURO : C_TEXTO_M, 1, false);
        zona_clique(vx - 4, y0 + 6, gfx_largura(v, 1) + 8, FAIXA_H - 12, NULL, 0, 'v');
    }
    if (modo == M_DESAFIO)                  // os do oponente, junto da bolsa (o meio e da rodada)
        amuletos_faixa(j, j == 0 ? x + 60 + gfx_largura(s, 1) : GFX_W - 142, y0 + FAIXA_H / 2);

    // A bolsa: quantos ainda estao nela, de quantos o jogador tem.
    int bx = GFX_W - 118, by = y0 + FAIXA_H / 2;
    icone_bolsa(bx + 8, by);
    snprintf(s, sizeof s, "bolsa %d/%d", na_bolsa(p), p->n_col);
    txt(bx + 22, ty, s, C_MARFIM_S, false);
}

// Saquinho de couro fechado por um cordao dourado; (cx, cy) e o meio dele.
static void icone_bolsa(int cx, int cy)
{
    const uint16_t borda = RGB(58, 36, 20), couro = RGB(150, 98, 54);
    const uint16_t claro = RGB(186, 132, 80), escuro = RGB(116, 72, 38);
    gfx_disco(cx, cy + 3, 7, borda);                        // o bojo
    gfx_disco(cx, cy + 3, 6, couro);
    gfx_disco(cx + 1, cy + 5, 4, escuro);                   // sombra embaixo, a direita
    gfx_disco(cx, cy + 4, 4, couro);
    gfx_rect(cx - 3, cy, 2, 2, claro);                      // brilho do couro
    gfx_rect(cx - 4, cy + 2, 1, 2, claro);
    gfx_rect(cx - 3, cy - 6, 7, 3, borda);                  // o pescoco franzido
    gfx_rect(cx - 2, cy - 5, 5, 2, escuro);
    gfx_rect(cx - 5, cy - 9, 11, 3, borda);                 // a boca, abrindo em leque
    gfx_rect(cx - 4, cy - 8, 9, 1, claro);
    gfx_rect(cx - 2, cy - 8, 1, 1, escuro);
    gfx_rect(cx + 1, cy - 8, 1, 1, escuro);
    gfx_rect(cx - 4, cy - 4, 9, 2, C_LATAO_ESC);            // o cordao
    gfx_rect(cx - 3, cy - 4, 7, 1, C_OURO);
    gfx_pixel(cx + 4, cy - 2, C_OURO);                      // a ponta solta
    gfx_pixel(cx + 5, cy - 1, C_OURO);
    gfx_pixel(cx + 5, cy, C_LATAO);
}

// Estilhacos do Quebrado: o dado se abre em lascas que se afastam e somem.
static void estilhaca(int x, int y, float t, int dono, int tipo)
{
    static const int8_t DIR[9][2] = { {-3,-4},{3,-4},{5,0},{3,4},{-3,4},{-5,0},{0,-5},{2,5},{-4,2} };
    uint16_t c = t < 0.5f ? (dono == 0 ? C_MARFIM : C_EBANO_B) : C_TERRA;
    if (tipo == D_VIDRO) c = t < 0.5f ? C_VIDRO : C_ACO;
    for (int i = 0; i < 9; i++) {
        int px = x + (int)(DIR[i][0] * t * 16), py = y + (int)(DIR[i][1] * t * 16);
        int tam = t < 0.6f ? 6 : 4;
        gfx_rect(px - tam / 2, py - tam / 2, tam, tam, c);
    }
}

// Projetil em arco de (ax, ay) a (bx, by); q vai de 0 (sai) a 1 (chega) e
// segue ate 1.5 com o anel do impacto se abrindo no alvo.
static void projetil(int ax, int ay, int bx, int by, float q, uint16_t c)
{
    if (q < 0) return;
    float arco = -fminf(90, fabsf((float)(by - ay)) * 0.25f + 40);
    for (int i = 5; i >= 0; i--) {                     // rastro
        float t = q - i * 0.05f;
        if (t < 0 || t > 1) continue;
        float x = ax + (bx - ax) * t, y = ay + (by - ay) * t + arco * 4 * t * (1 - t);
        int rr = i == 0 ? 5 : 4 - i / 2;
        gfx_disco((int)x, (int)y, rr, i == 0 ? C_BRANCO : gfx_mistura(c, C_FELTRO, i * 15));
        if (i == 0) gfx_disco((int)x, (int)y, 3, c);
    }
    if (q > 1 && q < 1.5f) {                           // impacto
        float t = (q - 1) / 0.5f;
        int rr = (int)(R_DADO * (0.6f + t * 1.2f));
        for (int a = 0; a < 32; a++) {
            float an = a * 0.19635f;
            gfx_rect(bx + (int)(cosf(an) * rr) - 1, by + (int)(sinf(an) * rr) - 1, 3, 3,
                     gfx_mistura(c, C_FELTRO, (int)(t * 80)));
        }
    }
}

// Faiscas douradas subindo de um dado que ganhou alguma coisa.
static void faiscas(int x, int y, float t, uint16_t c)
{
    for (int i = 0; i < 6; i++) {
        float f = fmodf(t * 1.3f + i * 0.17f, 1.0f);
        int px = x - R_DADO + (i * 37) % (2 * R_DADO) + (int)(sinf(t * 5 + i) * 3);
        int py = y + R_DADO - (int)(f * (2 * R_DADO + 16));
        int tam = f < 0.7f ? 2 : 1;
        gfx_rect(px - tam, py, 2 * tam + 1, 1, c);
        gfx_rect(px, py - tam, 1, 2 * tam + 1, c);
    }
}

// O Ventania de j acabou de cair e esta rolando o dado de antes?
static bool ventania_agora(int j)
{
    const fila_t *f = &F[j];
    return fase == F_VEREDITO && j == vez && f->lancados > 1
        && f->tipo[f->lancados - 1] == D_VENTANIA && t_fase < 0.7f;
}

// Tira a cor do que foi desenhado por cima da mesa numa caixa: fica cinza e
// com menos contraste. O feltro, a sombra e o lugar marcado nao mudam.
static void acinzenta(int x0, int y0, int w, int h, int pct)
{
    uint16_t *fb = display_fb();
    for (int y = y0 < 0 ? 0 : y0; y < y0 + h && y < GFX_H; y++)
        for (int x = x0 < 0 ? 0 : x0; x < x0 + w && x < GFX_W; x++) {
            int i = y * GFX_W + x;
            uint16_t c = fb[i];
            if (c == mesa_pronta[i] || c == C_FELTRO_ESC || c == C_SOMBRA) continue;
            int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
            int l = 70 + ((r * 77 + g * 150 + b * 29) >> 8) * 45 / 100;   // cinza, puxado para o meio
            r += (l - r) * pct / 100;
            g += (l - g) * pct / 100;
            b += (l - b) * pct / 100;
            fb[i] = RGB(r, g, b);
        }
}

static void desenha_fila(int j)
{
    fila_t *f = &F[j];
    int y = zona(j);
    bool no_desfecho = fase == F_EFEITOS || fase == F_RESULTADO;
    bool pisca = fase == F_VEREDITO && j == vez && ((int)(t_fase * 7) % 2 == 0);
    const evento_t *e = (fase == F_EFEITOS && ev_i < n_ev) ? &ev[ev_i] : NULL;
    float p = e ? t_fase / duracao_evento(ev_i) : 0;
    const int R = R_DADO;

    if (f->n == 0 && j != vez) return;                   // fila alheia ainda vazia

    // Previa do fim da rodada, calculada pelas regras de verdade: o que cada
    // dado ja ganhou de outro (Moeda da sorte, Copiador, multiplicadores) e
    // o total com isso e com o que o rival ja tirou. So durante o jogo; no
    // desfecho sao os eventos que mostram.
    bool previa = !no_desfecho && fase != F_FIM && (F[0].lancados || F[1].lancados);
    desfecho_t pv;
    if (previa) calcula_desfecho(&pv, false);

    for (int k = 0; k < f->n; k++) {
        int x = SLOT_X(k);
        bool voando = n_voo && (fase == F_ROLANDO || fase == F_ARRUMA)
                      && voo[0].dono == j && voo[0].slot == k;
        if (voando) continue;
        int t = tipo_de(j, k);
        dica_zona(x - R, y - R, 2 * R, 2 * R, DICA_DADO, t);
        if (k >= f->lancados) {
            gfx_disco(x, y, R - 2, C_FELTRO_ESC);                    // lugar marcado
            marca_nivel = f->fixo[k];
            desenha_dado((float)x, (float)y, R - 6, t, j, 0, f->fixo[k], 0, false);
            marca_nivel = -1;
            // Na hora de lancar, o que ainda nao foi jogado fica cinza;
            // montando a sequencia, cada dado na sua cor.
            if (fase == F_JOGA || fase == F_ROLANDO || fase == F_ARRUMA || fase == F_VEREDITO)
                acinzenta(x - R, y - R, 2 * R + 6, 2 * R + 6, 80);
            continue;
        }
        uint8_t est = no_desfecho ? vis_est[j][k] : f->est[k];

        // Dado sendo roubado agora: sai do lugar e atravessa a mesa.
        if (e && e->tipo == EV_ROUBA && e->alvo_j == j && e->alvo_k == k && p > 0.25f) {
            float q = (p - 0.25f) / 0.6f;
            if (q > 1) q = 1;
            float s = q * q * (3 - 2 * q);
            int tx = ROUBO_X(vis_nroubo[e->de_j]), ty = zona(e->de_j);
            desenha_dado(x + (tx - x) * s, y + (ty - y) * s, R + (R_ROUBO - R) * s, t, j, s * 6.28f,
                         f->valor[k], 2, false);
            continue;
        }
        if (est == V_ROUBADO) {                           // lugar vazio: foi levado
            gfx_disco(x, y, R - 2, C_FELTRO_ESC);
            continue;
        }
        if (fase == F_RESULTADO && f->quebra[k] && t_fase > 0.35f) {
            if (t_fase < 1.3f) estilhaca(x, y, (t_fase - 0.35f) / 0.95f, j, t);
            continue;
        }
        // Queimado pela Ficha de Fogo: se desfaz de cima para baixo em cinza.
        if (fase == F_RESULTADO && f->cinza[k] && t_fase > 0.35f) {
            float q = (t_fase - 0.35f) / 1.25f;
            if (q < 1) {
                desenha_dado((float)x, (float)y, R, t, j, 0, f->valor[k], 2, false);
                int topo = y - R - 6, corte = topo + (int)(q * (2 * R + 12));
                for (int yy = topo; yy < corte; yy++)            // o feltro volta
                    memcpy(display_fb() + yy * GFX_W + x - R - 6,
                           mesa_pronta + yy * GFX_W + x - R - 6, (size_t)(2 * R + 12) * 2);
                for (int i = 0; i < 9; i++) {
                    int ex = x - R + (i * 11) % (2 * R);
                    int ey = corte - ((int)(t_fase * 90) + i * 13) % 40;
                    gfx_rect(ex, ey, 4, 4, i % 2 ? C_FOGO : C_CINZA);
                }
            }
            continue;
        }
        bool anul = est == V_ANULADO;
        bool recente = fase == F_VEREDITO && j == vez
                       && (k == anulou_a || k == anulou_b || (anulou_masc >> k & 1));
        // O que segurou a queda treme no lugar, como quem balanca a cabeca.
        if (fase == F_VEREDITO && j == vez && k == resistiu && t_fase < 0.7f)
            x += ((int)(t_fase * 28) % 2) ? 4 : -4;
        if (recente && t_fase < 0.5f) x += (int)(sinf(t_fase * 60) * 3);   // treme antes de apagar
        if (recente && pisca) continue;
        // Alvo de anulacao agora: pisca antes de cair.
        bool alvo = e && e->tipo == EV_ANULA && e->alvo_j == j && e->alvo_k == k;
        if (alvo && p > 0.4f && ((int)(t_fase * 8) % 2 == 0)) continue;
        // O dado que acabou de valer da um pulinho.
        float pulo = 0;
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && ver_tipo == 0 && t_fase < 0.25f)
            pulo = sinf(t_fase / 0.25f * 3.1416f) * 8;
        // Espelho que so vira no fim: mostra o que tirou ate o evento dele
        // chegar na metade, e entao o numero do rival.
        bool ainda = no_desfecho && vis_espelho[j][k]
                     && !(e && e->tipo == EV_ESPELHO && e->de_j == j && e->de_k == k && p > 0.5f);
        // Ventania: o dado de antes rodopia mostrando faces ao acaso e
        // assenta no numero novo.
        int face = ainda ? f->cru[k] : f->valor[k];
        float giro = 0;
        if (ventania_agora(j) && k == f->lancados - 2) {
            float q = t_fase / 0.7f;
            giro = (1 - q) * (1 - q) * 9;
            x += (int)(sinf(t_fase * 40 + k) * (1 - q) * 3);
            if (q < 0.7f) face = face_v(t, f->valor[k]);
        }
        marca_nivel = f->fixo[k];
        desenha_dado((float)x, y - pulo, R, t, j, giro, face, 2, anul && giro < 0.5f);
        marca_nivel = -1;
        // Etiqueta: no desfecho, o multiplicador somado (x4) ou o bonus da
        // Moeda (+2); antes, o aviso do que vem (x2 do Dobro, +8, =9).
        char tg[16] = "";
        if (no_desfecho && vis_mult[j][k] > 1) snprintf(tg, sizeof tg, "x%d", vis_mult[j][k]);
        else if (no_desfecho && vis_bonus[j][k]) snprintf(tg, sizeof tg, "+%d", vis_bonus[j][k]);
        else if (ainda) snprintf(tg, sizeof tg, "=?");
        else if (f->tag[k][0]) snprintf(tg, sizeof tg, "%s", f->tag[k]);
        // Bonus que este dado ja recebeu de outros, pela previa. O x2 do
        // Dobro e o +2 do Copiador ficam por conta dela.
        int extra = 0;
        if (previa && f->est[k] == V_VALIDO) {
            int base = valor_pontos(j, k) + f->bonus[k];
            int m = pv.mult[j][k] > 0 ? pv.mult[j][k] : 1;
            int fim = pv.est[j][k] == V_VALIDO ? (base + pv.bonus[j][k]) * m : 0;
            extra = fim - base;
        }
        bool do_fim = (!strcmp(tg, "x2") && f->dobra[k]) || (!strcmp(tg, "+2") && copia_antes(j, k))
                      || (!strcmp(tg, "+6") && k > 0 && tipo_de(j, k - 1) == D_FERREIRO);
        if (previa && do_fim) tg[0] = 0;
        // Egoista chegando: uma onda purpura se abre dele e derruba os outros.
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && t == D_EGOISTA
            && anulou_masc && t_fase < 0.9f) {
            for (int o = 0; o < 2; o++) {
                int rr = (int)(t_fase * 260) - o * 26;
                if (rr <= R_DADO) continue;
                for (int a = 0; a < 48; a++) {
                    float an = a * 0.1309f;
                    gfx_rect(x + (int)(cosf(an) * rr), y + (int)(sinf(an) * rr * 0.45f), 3, 3, C_EGO);
                }
            }
        }
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && ver_tipo == 0) {
            aro(x, y, C_OURO);
            // Trevo com 4: SORTE em dourado, com faiscas em volta.
            if (t == D_TREVO && f->valor[k] == 4) {
                int ty = j == baixo ? y - R - 34 : y + R + 12;
                gfx_rect(x - 28, ty - 2, 56, TXT_H + 4, C_BARRA);
                txt_c(x, ty, "SORTE", C_OURO, true);
                for (int i = 0; i < 6; i++) {
                    float a = t_fase * 5 + i * 1.0472f;
                    int sx = x + (int)(cosf(a) * 36), sy = y + (int)(sinf(a) * 32);
                    gfx_rect(sx - 3, sy, 7, 1, C_OURO);
                    gfx_rect(sx, sy - 3, 1, 7, C_OURO);
                }
            }
        }
        if (e && e->de_j == j && e->de_k == k) aro(x, y, C_VINHO_CLR);   // quem age
        if (alvo) aro(x, y, C_VINHO_CLR);
        // Bonus e multiplicadores: aro dourado nos dados que recebem.
        if (e && (e->tipo == EV_BONUS || e->tipo == EV_MULT || e->tipo == EV_JACKPOT) && e->de_j == j
            && (e->mascara >> k & 1) && p > 0.2f) {
            aro(x, y, C_OURO);
            faiscas(x, y, t_fase + k * 0.3f, C_AMARELO);
        }
        if (e && e->tipo == EV_BONUS && e->de_j == j && e->de_k == k) aro(x, y, C_OURO);
        // Marcado pelo fogo: aro em brasa ate o fim da rodada.
        if (no_desfecho && (vis_queima[j][k] ||
            (e && e->tipo == EV_FOGO && e->alvo_j == j && e->alvo_k == k && p > 0.35f)))
            aro(x, y, ((int)(t_fase * 10) % 2) ? C_FOGO : C_BRASA);
        if (e && e->tipo == EV_ZERA && e->de_j == j && p > 0.15f && ((int)(t_fase * 9) % 2 == 0))
            aro(x, y, C_VIOLETA);
        if (e && (e->tipo == EV_FOGO || e->tipo == EV_SEMENTE) && e->de_j == j && e->de_k == k)
            aro(x, y, e->tipo == EV_FOGO ? C_FOGO : C_VERDE);
        if (e && e->tipo == EV_EXTRA && e->de_j == j && e->de_k == k) aro(x, y, cor_dado(t));
        if (tg[0] && !anul) etiqueta(x, y, tg);        // por cima dos aros
        // O bonus, miudo e apagado, no canto de cima a direita (depois da
        // etiqueta, se houver uma).
        if (extra && !anul) {
            char b[8];
            snprintf(b, sizeof b, "%+d", extra);
            int bx = tg[0] ? x + R + 3 + gfx_largura(tg, 1) : x + R - 4, by = y - R - 10;
            uint16_t cb = extra > 0 ? gfx_mistura(C_MARFIM_S, C_FELTRO, 35)
                                    : gfx_mistura(C_VINHO_CLR, C_FELTRO, 30);
            gfx_texto(bx, by, b, cb, GFX_MIUDO, false);
        }
    }
    // Rajadas da Ventania saindo dela e passando pelo dado de antes.
    if (ventania_agora(j))
        for (int i = 0; i < 7; i++) {
            float q = t_fase / 0.7f;
            int xs = SLOT_X(f->lancados - 1) + 10 - (int)fmodf(t_fase * 500 + i * 61, 160.0f);
            int ys = y - 24 + i * 8;
            gfx_rect(xs, ys, 30 + (i % 3) * 10, 1, gfx_mistura(C_VENTO, C_FELTRO, 30 + (int)(q * 60)));
        }
    // O par que se anulou fica unido por um traco.
    if (fase == F_VEREDITO && j == vez && anulou_a >= 0 && anulou_b >= 0)
        gfx_rect(SLOT_X(anulou_a) + R + 4, y - 2, SLOT_X(anulou_b) - SLOT_X(anulou_a) - 2 * R - 8, 5,
                 C_VINHO_CLR);

    // Dados roubados por este jogador, a esquerda da fila.
    if (no_desfecho)
        for (int i = 0; i < vis_nroubo[j]; i++) {
            roubado_t *r = &vis_roubo[j][i];
            desenha_dado((float)ROUBO_X(i), (float)y, R_ROUBO, r->tipo, r->de_j, 0, r->valor, 2, false);
        }

    if (f->lancados || vis_nroubo[j] || (previa && pv.bruto[j] != 0)) {
        char s[16];
        int pts = fase == F_RESULTADO && venc_mao >= 0 ? f->total
                : no_desfecho ? total_visivel(j) : f->pontos;
        // Rodada zerando: o placar desce ate 0 durante o evento.
        if (e && e->tipo == EV_ZERA && e->de_j == j) {
            float q = p / 0.7f;
            pts = (int)(pts * (q > 1 ? 0 : 1 - q));
        }
        snprintf(s, sizeof s, "%d", pts);
        uint16_t c = (fase == F_RESULTADO && venc_mao == j) ? J[j].cor
                   : f->zerada ? C_VIOLETA
                   : (j == vez && !no_desfecho ? C_MARFIM : C_TEXTO_M);
        ui_painel(PLACAR_X - 34, y - 26, 68, 50);
        txt_c(PLACAR_X, y - 42 + (j == baixo ? 72 : 0), "pontos", C_TEXTO_M, false);
        txt_c2(PLACAR_X, y - 20, s, c);
        // Ao lado, apagado: quanto fica depois dos bonus e dos ataques que ja
        // estao na mesa. Pode ser negativo (Espinhoso, Pirata, Maldito do rival).
        if (previa && pv.bruto[j] != pts) {
            snprintf(s, sizeof s, "(%d)", pv.bruto[j]);
            txt(PLACAR_X + 38, y - 10, s, gfx_mistura(C_TEXTO_M, C_FELTRO, 25), false);
        }
    }
}

// O que o evento em curso desenha por cima da mesa: o traco do ataque, os
// pontos que saem de um placar e entram no outro.
static void desenha_evento(void)
{
    if (fase != F_EFEITOS || ev_i >= n_ev) return;
    const evento_t *e = &ev[ev_i];
    float p = t_fase / duracao_evento(ev_i);
    if (e->tipo == EV_CHEFE) {                   // a virada do Barao: o +N sobe no placar dele
        char t[8];
        snprintf(t, sizeof t, "+%d", e->valor);
        txt_sombra_c(PLACAR_X, zona(e->de_j) - 10 - (int)(p * 30), t, C_VINHO_CLR, 1);
        return;
    }
    int ax = SLOT_X(e->de_k), ay = zona(e->de_j);
    // Ataques: um projetil em arco do dado que age ate o alvo (o dado, ou o
    // placar do rival), com rastro, e um anel de impacto quando chega.
    if (e->tipo == EV_ANULA || e->tipo == EV_TIRA) {
        bool praga = TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_MALDICAO;
        uint16_t c = praga ? C_VIOLETA : C_VINHO_CLR;
        int bx = e->tipo == EV_ANULA ? SLOT_X(e->alvo_k) : PLACAR_X, by = zona(e->alvo_j);
        if (TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_LASER) {
            // Laser: um raio verde reto sai do emissor e chega ao alvo,
            // tremulando, e fica aceso um instante antes do impacto.
            float q = p / 0.3f;
            if (q > 1) q = 1;
            int ex = ax + (int)((bx - ax) * q), ey = ay - R_DADO + (int)((by - (ay - R_DADO)) * q);
            if (p < 0.6f) {
                int tr = ((int)(t_fase * 40) % 2) ? 5 : 4;
                gfx_linha_grossa(ax, ay - R_DADO, ex, ey, tr + 2, gfx_mistura(C_LASER, C_FELTRO, 55));
                gfx_linha_grossa(ax, ay - R_DADO, ex, ey, tr, C_LASER);
                gfx_linha_grossa(ax, ay - R_DADO, ex, ey, 1, C_BRANCO);
                gfx_disco(ex, ey, tr, C_BRANCO);
            }
            float tt = (p - 0.3f) / 0.5f;           // anel do impacto no alvo
            if (tt > 0 && tt < 1) {
                int rr = (int)(R_DADO * (0.6f + tt * 1.2f));
                for (int a = 0; a < 32; a++) {
                    float an = a * 0.19635f;
                    gfx_rect(bx + (int)(cosf(an) * rr) - 1, by + (int)(sinf(an) * rr) - 1, 3, 3,
                             gfx_mistura(C_LASER, C_FELTRO, (int)(tt * 80)));
                }
            }
        } else {
            projetil(ax, ay, bx, by, p / 0.42f, c);
        }
    }
    // Espelho: um reflexo sai do maior dado do rival e chega nele.
    if (e->tipo == EV_ESPELHO) {
        const fila_t *fr = &F[e->alvo_j];
        int a = 0;
        for (int i = 0; i < fr->lancados; i++) if (fr->valor[i] == e->valor) { a = i; break; }
        projetil(SLOT_X(a), zona(e->alvo_j), ax, ay, p / 0.5f, C_ACO);
    }
    // Ficha de Fogo: linha de fogo tremulando ate o alvo, brasas subindo dele.
    if (e->tipo == EV_FOGO) {
        float q = p / 0.35f;
        if (q > 1) q = 1;
        int bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        int ex = ax + (int)((bx - ax) * q), ey = ay + (int)((by - ay) * q);
        uint16_t c = ((int)(t_fase * 16) % 2) ? C_FOGO : C_AMARELO;
        gfx_linha_grossa(ax, ay, ex, ey + ((int)(t_fase * 20) % 3) - 1, 5, c);
        if (p > 0.35f)
            for (int i = 0; i < 10; i++) {
                int px = bx - 18 + (i * 7) % 36;
                int py = by - 24 - ((int)(t_fase * 70) + i * 9) % 40;
                gfx_rect(px, py, 4, 4, i % 2 ? C_AMARELO : C_FOGO);
            }
    }
    // Jackpot: letreiro piscando sobre a maquininha e moedas caindo no placar.
    if (e->tipo == EV_JACKPOT) {
        for (int i = 0; i < 14; i++) {
            float q = fmodf(p * 1.6f + i * 0.137f, 1.0f);
            int mx = ax - 60 + (i * 53) % 120 + (int)(sinf(t_fase * 6 + i) * 4);
            int my = ay - 70 + (int)(q * 90);
            gfx_disco(mx, my, 4, i % 3 ? C_OURO : C_AMARELO);
            gfx_rect(mx - 1, my - 2, 2, 2, C_BRANCO);
        }
        // A placa fica a esquerda da fila, longe do texto do meio da mesa.
        int lx = 96, ly = ay - 24;
        uint16_t c = ((int)(t_fase * 8) % 2) ? C_AMARELO : C_VERMELHO;
        int w = gfx_largura("JACKPOT!", 2) + 16;
        gfx_rect(lx - w / 2, ly - 4, w, 48, C_BARRA);
        gfx_moldura(lx - w / 2, ly - 4, w, 48, 2, c);
        for (int i = 0; i < 8; i++)                        // lampadas da moldura
            gfx_disco(lx - w / 2 + 6 + i * (w - 12) / 7, ly - 4, 2,
                      ((i + (int)(t_fase * 10)) % 2) ? C_AMARELO : C_VERMELHO);
        txt_c2(lx, ly, "JACKPOT!", c);
        if (p > 0.5f) {
            int ty = zona(e->de_j) - 10 - (int)((p - 0.5f) * 30);
            txt_sombra_c(PLACAR_X - 64, ty, "+10", C_OURO, 1);
        }
    }
    // Semente: o broto cresce da moeda, e o +10 sobe em verde.
    if (e->tipo == EV_SEMENTE) {
        float q = p / 0.6f;
        if (q > 1) q = 1;
        int alto = (int)(q * 34);
        int sx = ax, sy = ay - R_DADO;
        gfx_rect(sx - 1, sy - alto, 4, alto, C_VERDE);
        if (q > 0.5f) {
            gfx_rect(sx - 12, sy - alto + 8, 11, 6, C_VERDE);
            gfx_rect(sx + 3, sy - alto + 3, 11, 6, C_VERDE);
        }
        if (p > 0.5f) {
            int ty = zona(e->de_j) - 10 - (int)((p - 0.5f) * 30);
            txt_sombra_c(PLACAR_X - 64, ty, "+10", C_VERDE, 1);
        }
    }
    // Bonus fixos do fim, cada um com o seu gesto, e o +N subindo no placar.
    if (e->tipo == EV_EXTRA) {
        int t = F[e->de_j].tipo[e->de_k];
        uint16_t c = cor_dado(t);
        if (t == D_MISERICORDIOSO) {
            // Uma luz sai de cada dado anulado e vem pousar nele.
            const fila_t *f = &F[e->de_j];
            for (int i = 0; i < f->lancados; i++) {
                if (vis_est[e->de_j][i] != V_ANULADO) continue;
                float q = p / 0.55f - i * 0.08f;
                if (q < 0 || q > 1) continue;
                int sx = SLOT_X(i), bx = ax;
                int lx = sx + (int)((bx - sx) * q), ly = ay - (int)(sinf(q * 3.1416f) * 34);
                gfx_disco(lx, ly, 4, C_HALO);
                gfx_disco(lx, ly, 2, C_BRANCO);
            }
        } else if (t == D_SOLITARIO) {
            // Um facho de luz de cima, so nele.
            float q = p < 0.3f ? p / 0.3f : (p > 0.8f ? (1 - p) / 0.2f : 1);
            int topo = ay - 70, base = ay + R_DADO;
            for (int yy = topo; yy < base; yy += 2) {
                int meia = 6 + (yy - topo) * 22 / (base - topo);
                gfx_rect_alfa(ax - meia, yy, 2 * meia, 2, C_HALO, (int)(40 * q));
            }
        } else if (t == D_DESAFIANTE) {
            // A flamula sobe do dado e se abre, tremulando: ele e o maior da mesa.
            float q = p / 0.4f;
            if (q > 1) q = 1;
            int topo = ay - R_DADO - 4 - (int)(q * 40);
            gfx_linha_grossa(ax, ay - R_DADO - 4, ax, topo, 2, C_LATAO);
            int w = (int)(q * 30), onda = (int)(sinf(t_fase * 12) * 3);
            if (w > 2) {
                float vx[3] = { (float)ax, (float)(ax + w), (float)ax };
                float vy[3] = { (float)topo, (float)(topo + 7 + onda), (float)(topo + 15) };
                poligono(vx, vy, 3, C_CARMIM);
            }
            gfx_disco(ax, topo, 2, C_OURO);
            if (q >= 1 && p < 0.85f) faiscas(ax + 12, topo + 10, t_fase, C_OURO);
        }
        // Anel que se abre do dado.
        float tt = p / 0.6f;
        if (tt < 1) {
            int rr = (int)(R_DADO * (0.8f + tt * 1.3f));
            for (int a = 0; a < 40; a++) {
                float an = a * 0.15708f;
                gfx_rect(ax + (int)(cosf(an) * rr) - 1, ay + (int)(sinf(an) * rr) - 1, 2, 2,
                         gfx_mistura(c, C_FELTRO, (int)(tt * 85)));
            }
        }
        if (p > 0.45f) {
            char mais[8];
            snprintf(mais, sizeof mais, "+%d", e->valor);
            int ty = zona(e->de_j) - 10 - (int)((p - 0.45f) * 30);
            txt_sombra_c(PLACAR_X - 64, ty, mais, C_OURO, 1);
        }
    }
    // Ferreiro: a marreta desce no dado seguinte e solta faiscas.
    if (e->tipo == EV_BONUS && e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_FERREIRO) {
        int bx = SLOT_X(e->de_k + 1), by = zona(e->de_j) - R_DADO - 6;
        float q = p / 0.3f;
        if (q <= 1) {
            float a = -1.1f + q * 1.1f;                    // o cabo girando ate bater
            int hx = bx + (int)(cosf(a - 1.5708f) * 24), hy = by + (int)(sinf(a - 1.5708f) * 24) + 24;
            gfx_linha_grossa(bx + 18, by + 16, hx, hy, 3, C_MADEIRA_CLR);
            gfx_rect(hx - 6, hy - 4, 12, 8, C_FERRO);
        } else if (p < 0.9f) {
            for (int i = 0; i < 10; i++) {
                float an = i * 0.628f + p * 3;
                int rr = (int)((p - 0.3f) * 70);
                gfx_rect(bx + (int)(cosf(an) * rr), by + 8 + (int)(sinf(an) * rr * 0.6f), 3, 2,
                         i % 2 ? C_FOGO : C_AMARELO);
            }
        }
    }
    if ((e->tipo == EV_TIRA || e->tipo == EV_PIRATA) && p > 0.2f) {
        float q = (p - 0.2f) / 0.6f;
        if (q > 1) q = 1;
        int sy = zona(e->alvo_j) - 10 - (int)(q * 18);
        char menos[8];
        snprintf(menos, sizeof menos, "-%d", e->valor);
        txt_sombra_c(PLACAR_X - 64, sy, menos, C_VINHO_CLR, 1);
        if (e->tipo == EV_PIRATA) {
            int y0 = zona(e->alvo_j), y1 = zona(e->de_j);
            int fy = y0 + (int)((y1 - y0) * q);
            ficha(PLACAR_X + 46, fy, C_OURO);
            if (q >= 1) txt_sombra_c(PLACAR_X - 64, zona(e->de_j) - 28, "+4", C_OURO, 1);
        }
    }
}

static void pote_mesa(void)
{
    int n = pote / 10 + 1;
    if (n > 9) n = 9;
    int y = CENTRO + 10;
    for (int i = 0; i < n; i++)
        ficha(POTE_X, y - i * 4, (i % 3 == 0) ? C_LATAO : (i % 3 == 1 ? C_VINHO : C_MARFIM_S));
    char s[16];
    snprintf(s, sizeof s, "%d", pote);
    txt_c(POTE_X, y + 12, s, C_OURO, true);
    txt_c(POTE_X, y - n * 4 - 26, "pote", C_TEXTO_M, false);
}
