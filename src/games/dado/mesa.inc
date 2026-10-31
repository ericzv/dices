// ===========================================================================
// Desenho da mesa: o lance, as faixas dos jogadores, as filas, os
// ataques e o pote
// ===========================================================================
// O lance de aposta de j (passou, apostou 10...), junto do placar dele: do
// lado de dentro da mesa, sobe um pouco ao entrar e some no fim.
static void desenha_lances(void)
{
    for (int j = 0; j < 2; j++) {
        if (t_lance[j] <= 0 || !lance_txt[j][0]) continue;
        float e = LANCE_T - t_lance[j], q = e < 0.18f ? e / 0.18f : 1;
        if (t_lance[j] < 0.12f && (int)(t_lance[j] * 40) % 2) continue;      // pisca ao sair
        bool cima = j != baixo;
        int y = cima ? zona(j) + 30 + (int)((1 - q) * -6) : zona(j) - 46 + (int)((1 - q) * 6);
        int f = fonte_de(true), w = gfx_largura(lance_txt[j], f);
        gfx_texto(PLACAR_X - w / 2, y, lance_txt[j], J[j].cor, f, true);
    }
}

// A cor de um numero que conta: no pulinho de cada passo, acende dourado
// subindo e vinho descendo (o que ja e dourado acende para o branco).
static uint16_t conta_cor(const contador_t *c, uint16_t base)
{
    if (c->pulo <= 0) return base;
    uint16_t acende = c->sinal > 0 ? (base == C_OURO ? C_BRANCO : C_OURO) : C_VINHO_CLR;
    return gfx_mistura(base, acende, (int)(c->pulo * 75));
}

// O nome do jogador na faixa.
static int txt_nome(int x, int y, int j)
{
    return gfx_texto(x, y, J[j].nome, J[j].cor, 1, true);
}

// O laco do Presente, com o no em (x, y): marca o dado que saiu de um.
static void lacinho(int x, int y)
{
    gfx_rect(x - 6, y - 3, 5, 5, C_LATAO_ESC);                // as alcas, com a borda escura
    gfx_rect(x + 2, y - 3, 5, 5, C_LATAO_ESC);
    gfx_rect(x - 5, y - 2, 3, 3, C_OURO);
    gfx_rect(x + 3, y - 2, 3, 3, C_OURO);
    gfx_rect(x - 3, y + 2, 2, 3, C_OURO);                     // as pontas
    gfx_rect(x + 2, y + 2, 2, 3, C_OURO);
    gfx_rect(x - 1, y - 2, 3, 4, C_AMARELO);                  // o no
}
// O Presente abrindo no lance (q de 0 a 1): no comeco ainda e a caixa;
// depois o papel rasga em pedacos que voam e caem, e o laco sobe.
#define PRESENTE_ABRE 0.62f
#define PRESENTE_RASGA 0.24f                 // ate aqui, a caixa fechada
static void presente_abre(float x, float y, float q)
{
    if (q < PRESENTE_RASGA || q >= 1) return;
    float s = (q - PRESENTE_RASGA) / (1 - PRESENTE_RASGA);
    if (s < 0.25f)                                           // o estalo da caixa abrindo
        for (int a = 0; a < 24; a++) {
            float an = a * 0.2618f, rr = R_DADO + 2 + s * 40;
            gfx_rect((int)(x + cosf(an) * rr), (int)(y + sinf(an) * rr), 2, 2, gfx_mistura(C_AMARELO, C_FELTRO, (int)(s * 280)));
        }
    for (int i = 0; i < 16; i++) {
        float an = i * 0.3927f + 0.2f, vel = 34 + (i * 37) % 30, d = R_DADO * 0.8f + vel * s * 1.2f;
        int px = (int)(x + cosf(an) * d), py = (int)(y + sinf(an) * d * 0.85f - 8 * s + 40 * s * s);
        uint16_t c = i % 5 == 4 ? C_OURO : (i % 2 ? C_VERMELHO : C_VINHO_CLR);
        int w = 3 + i % 2, h = 2 + (i + 1) % 2;
        if ((int)(s * 20 + i) % 4 == 0) { int t = w; w = h; h = t; }        // o papel girando
        gfx_rect(px, py, w, h, gfx_mistura(c, C_FELTRO, (int)(s * 75)));
    }
    if (s < 0.8f) lacinho((int)x, (int)(y - R_DADO - 4 - s * 34));
}
// ---- O pouso -------------------------------------------------------------
// Quando o dado deita, o feltro levanta poeira: um anel que clareia o feltro
// um tom (pontilhado), abre em volta de onde ele bateu e some. Fica no lugar
// mesmo quando o dado ja foi para a fila.
#define POEIRA_T 0.42f
#define POEIRA_N 4
static struct { float x, y, t; } poeira[POEIRA_N];
static int feltro_idx(uint16_t c);           // catalogo.inc
static void poeira_solta(float x, float y)
{
    int k = 0;
    for (int i = 1; i < POEIRA_N; i++) if (poeira[i].t <= 0 || poeira[i].t > poeira[k].t) k = i;   // a mais velha
    poeira[k].x = x; poeira[k].y = y; poeira[k].t = 0.001f;
}
// O estouro do Papel Bomba: um clarao, o anel de fogo que abre, a fumaca
// subindo e os pedacos do papel queimado, onde o dado estava quando o papel
// explodiu (ele sai voando dali). A mesa treme e o feltro levanta poeira.
#define ESTOURO_T 0.6f
static struct { float x, y, t; } estouro_fx;
static void treme(float px, float dur);      // hora.inc
static void estoura(float x, float y)
{
    estouro_fx.x = x; estouro_fx.y = y; estouro_fx.t = 0.001f;
    poeira_solta(x, y);
    treme(3, 0.25f);
}
// Um disco pontilhado (fumaca, brilho): so os pixels onde o Bayer passa da
// densidade, e so dentro do feltro (nao sobe na borda de madeira).
static void disco_pontilhado(int cx, int cy, int r, uint16_t c, float dens)
{
    for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++)
            if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r && bayer(x, y) < dens
                && x >= MESA_X0 && x < MESA_X1 && y >= MESA_Y0 && y < MESA_Y1) gfx_pixel(x, y, c);
}
static void desenha_estouro(void)
{
    if (estouro_fx.t <= 0) return;
    float q = estouro_fx.t / ESTOURO_T;
    int cx = (int)estouro_fx.x, cy = (int)estouro_fx.y;
    if (q < 0.3f) {                                        // o clarao: abre e apaga de fora para dentro
        float s = q / 0.3f;
        disco_pontilhado(cx, cy, (int)(R_DADO + 14 + s * 6), C_BRASA, 0.9f - s * 0.8f);
        disco_pontilhado(cx, cy, (int)(R_DADO + 8 + s * 4), C_FOGO, 1 - s * 0.7f);
        disco_pontilhado(cx, cy, (int)(R_DADO - s * 10), C_AMARELO, 1 - s * 0.6f);
        if (s < 0.5f) disco_pontilhado(cx, cy, (int)(R_DADO - 8 - s * 12), C_BRANCO, 1);
    }
    for (int i = 0; i < 6 && q > 0.12f; i++) {             // a fumaca: bolas que sobem, abrem e somem
        float s = (q - 0.12f) / 0.88f;
        int px = cx - 18 + i * 7 + (int)(sinf(s * 5 + i * 1.7f) * 4), py = cy - 6 - (int)(s * 34) - (i % 3) * 4;
        disco_pontilhado(px, py, 5 + (int)(s * 6), i % 2 ? C_EBANO_B : RGB(96, 88, 80), (1 - s) * 0.85f);
    }
    for (int i = 0; i < 26; i++) {                         // as brasas e os pedacos do papel, voando e caindo
        float an = i * 0.2417f + 0.17f, vel = 38 + (i * 37) % 26;
        float d = R_DADO * 0.5f + q * vel;
        int px = cx + (int)(cosf(an) * d), py = cy + (int)(sinf(an) * d * 0.8f) + (int)(q * q * 30);
        if ((q > 0.65f && i % 2) || px < MESA_X0 || px > MESA_X1 - 3 || py < MESA_Y0 || py > MESA_Y1 - 3) continue;
        if (i % 3 == 2) {                                  // papel queimando: claro, depois cinza
            gfx_rect(px, py, 3, 2, C_BARRA);
            gfx_rect(px, py, 2, 1, q < 0.45f ? C_MARFIM_S : C_CINZA);
        } else {
            int b = q < 0.4f ? 3 : 2;
            gfx_rect(px, py, b, b, q < 0.25f ? C_AMARELO : q < 0.55f ? C_FOGO : C_BRASA);
        }
    }
}
static void poeira_passo(float dt)
{
    for (int i = 0; i < POEIRA_N; i++)
        if (poeira[i].t > 0 && (poeira[i].t += dt) > POEIRA_T) poeira[i].t = 0;
    if (estouro_fx.t > 0 && (estouro_fx.t += dt) > ESTOURO_T) estouro_fx.t = 0;
    for (int i = 0; i < n_voo; i++) if (voo[i].t_assenta > 0 && voo[i].t_assenta < 9) voo[i].t_assenta += dt;
}
static void desenha_poeira(void)
{
    uint16_t *fb = display_fb();
    for (int i = 0; i < POEIRA_N; i++) {
        if (poeira[i].t <= 0) continue;
        float q = poeira[i].t / POEIRA_T, e = 1 - (1 - q) * (1 - q) * (1 - q);   // abre rapido e devagara
        float rx = R_DADO + 2 + e * 28, ry = rx * 0.55f, larg = 7 - 5 * q;   // o anel, achatado como o chao
        float forte = (1 - q) * 1.5f;
        int cx = (int)poeira[i].x, cy = (int)(poeira[i].y + R_DADO * 0.45f);
        for (int y = (int)(cy - ry - larg); y <= (int)(cy + ry + larg); y++) {
            if (y < MESA_Y0 || y >= MESA_Y1) continue;
            float dy = (y - cy) / ry;
            for (int x = (int)(cx - rx - larg); x <= (int)(cx + rx + larg); x++) {
                if (x < MESA_X0 || x >= MESA_X1) continue;
                float dx = (x - cx) / rx, d = sqrtf(dx * dx + dy * dy) * rx;   // ~ a distancia, em px no eixo x
                float fora = fabsf(d - rx) / larg;                             // 0 no meio do anel
                if (fora >= 1) continue;
                float dens = (1 - fora) * forte;
                if (bayer(x, y) >= dens) continue;
                uint16_t *p = &fb[y * GFX_W + x];
                int k = feltro_idx(*p);
                int k2 = k + (dens > 0.6f ? 2 : 1);              // forte: dois tons acima
                if (k >= 0 && k < 5) *p = feltro_tons[k2 > 5 ? 5 : k2];
            }
        }
        for (int g = 0; g < 7; g++) {                // uns graos que espirram para fora
            float a = g * 0.898f + poeira[i].x * 0.03f, d = rx + 4 + e * 10;
            int px = (int)(cx + cosf(a) * d), py = (int)(cy + sinf(a) * d * 0.55f - sinf(q * 3.1416f) * 5);
            if (q < 0.7f && px > MESA_X0 && px < MESA_X1 - 2 && py > MESA_Y0 && py < MESA_Y1 - 2)
                gfx_rect(px, py, q < 0.35f ? 2 : 1, q < 0.35f ? 2 : 1, feltro_tons[5]);
        }
    }
}
// O que os adesivos do pouso desenham em volta do dado parado: a Nota Dez
// virando, com estrelinhas de ouro em volta; o Papel Bomba, o pavio chiando
// antes de estourar; o Papel Carbono, a folha azul que passa por cima com o
// numero copiado escrito nela e deixa o dado com ele.
static void ad_pouso_efeito(const voo_t *v, float x, float y, float r)
{
    if (!v->etapa || !v->parado) return;
    float te = v->t_etapa;
    if (v->etapa == 1 && v->ad_vira == AD_NOTA_DEZ && te > AD_MOSTRA) {
        float q = fminf(1, (te - AD_MOSTRA) / AD_VIRA);
        for (int i = 0; i < 8; i++) {
            float an = i * 0.785f + q * 2.4f, d = r + 4 + q * 9;
            int px = (int)(x + cosf(an) * d), py = (int)(y + sinf(an) * d);
            int b = (int)(t_relogio * 14 + i) % 3 ? 1 : 2;         // a estrelinha pisca maior
            uint16_t c = i % 2 ? C_AMARELO : C_OURO;
            gfx_rect(px - b, py, 2 * b + 1, 1, c);
            gfx_rect(px, py - b, 1, 2 * b + 1, c);
        }
    }
    if (v->etapa == 2 && v->ad_rola == AD_BOMBA && te < AD_MOSTRA) {    // o pavio chiando
        for (int i = 0; i < 9; i++) {
            float an = te * 9 + i * 0.7f, d = r + 1 + (int)(t_relogio * 30 + i * 3) % 6;
            int px = (int)(x + cosf(an) * d), py = (int)(y + sinf(an) * d), b = i % 3 ? 1 : 2;
            gfx_rect(px, py, b, b, i % 2 ? C_AMARELO : C_FOGO);
        }
    }
    if (v->etapa == 3 && te > AD_MOSTRA) {                 // a folha de carbono passa por cima
        float q = fminf(1, (te - AD_MOSTRA) / AD_COPIA), s = q * q * (3 - 2 * q);
        int w = (int)(r * 1.6f), h = (int)(r * 2.3f);
        int fx = (int)(x - 2.4f * r + s * 4.8f * r) - w / 2, fy = (int)y - h / 2 - 2;
        uint16_t folha = ADESIVO[AD_CARBONO].fundo, beira = ADESIVO[AD_CARBONO].luz;
        disco_pontilhado(fx + w / 2 + 3, fy + h / 2 + 4, w / 2 + 2, C_SOMBRA, 0.5f);
        gfx_rect(fx - 1, fy - 1, w + 2, h + 2, C_BARRA);
        gfx_rect(fx, fy, w, h, folha);
        gfx_rect(fx, fy, w, 1, beira);
        gfx_rect(fx, fy, 1, h, beira);
        for (int l = 0; l < 4; l++) gfx_rect(fx + 3, fy + 4 + l * 5, w - 7 - (l % 2) * 4, 1, C_AZUL);   // as linhas da letra
        for (int c = 0; c < 5; c++) gfx_rect(fx + w - 1 - c, fy + h - 5 + c, c + 1, 1, beira);       // a ponta dobrada
        char n[8];
        snprintf(n, sizeof n, "%d", v->valor_c);
        txt_c(fx + w / 2, fy + h / 2 - 7, n, C_VENTO, true);                                   // o numero que ela copia
    }
}

static void desenha_lance(void)
{
    desenha_poeira();
    desenha_estouro();
    for (int i = n_voo - 1; i >= 0; i--) {
        voo_t *v = &voo[i];
        float r = R_DADO + (v->pouso > 0 ? v->pouso * 16 : 0);
        if (hora && !v->fantasma) {              // a hora da verdade: a camera chega perto
            float z = t_hora;
            if (fase == F_ARRUMA) z *= 1 - fminf(1, fmaxf(0, t_fase - junta_t()) / 0.45f);
            r *= 1 + 0.22f * z;
        }
        bool descartado = false;
        float ex = v->x, ey = v->y;              // onde ele e desenhado
        int mostra = v->parado ? v->valor : v->face;
        face_escondida = v->tipo == D_OCULTO;     // o Oculto rola e pousa virado
        // O lance menor do Viciado: riscado e encolhendo enquanto o maior
        // vai para a fila. O Reves do Sorte/Reves desliza para baixo da
        // Sorte e os dois viram um dado so, com o valor final.
        if (v->tipo == D_SORTE_REVES) sr_lado = v->fantasma ? 2 : 1;
        if (!v->fantasma && v->tipo == D_SORTE_REVES && n_voo > 1 && fase == F_ARRUMA && t_fase >= SR_JUNTA) {
            sr_lado = 0;
            mostra = v->valor - voo[1].valor;
            float pop = 1 - (t_fase - SR_JUNTA) / 0.15f;
            if (pop > 0) r *= 1 + 0.15f * pop;
        }
        if (v->fantasma && fase == F_ARRUMA) {
            float q = t_fase / (v->tipo == D_SORTE_REVES ? SR_JUNTA : 0.45f);
            q = q > 1 ? 1 : q;
            if (v->tipo == D_SORTE_REVES) {
                float s = q * q * (3 - 2 * q);
                ex = v->ox + (voo[0].x - v->ox) * s;
                ey = v->oy + (voo[0].y - v->oy) * s;
                r = R_DADO * (1 - q * 0.3f);
                if (q >= 1) { sr_lado = 0; continue; }
            } else {
                r = R_DADO * (1 - q * 0.4f);
                descartado = true;
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
        // O Presente no comeco do lance: ainda a caixa embrulhada, que rasga.
        bool abre = !v->fantasma && fase == F_ROLANDO && v->slot < N_FILA && F[v->dono].presente[v->slot]
                    && t_fase < PRESENTE_ABRE;
        bool caixa = abre && t_fase < PRESENTE_ABRE * PRESENTE_RASGA;
        if (TIPO[v->tipo].lados == 2 && !v->parado && !descartado && !caixa) {
            float c = cosf(v->giro * 1.6f);            // moeda gira no ar
            int lado = c >= 0 ? v->face : 3 - v->face;
            if (!v->fantasma) {                        // o adesivo do lado que esta a vista, aceso, como nos dados
                int a = ad_no_lado(peca_fila(v->dono, v->slot), lado);
                if (a) { ad_face = a; ad_face_aceso = true; }
            }
            face_curinga = v->tipo == D_FICHA_CORINGA;   // a Ficha Coringa gira com o curinga dos dois lados
            moeda_girando_arte(v->x, v->y, r, v->tipo, v->dono, c, lado);
            face_curinga = false;
            if (abre) presente_abre(v->x, v->y, t_fase / PRESENTE_ABRE);
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
        // Os adesivos: rolando, a etiqueta aparece (acesa) cada vez que passa
        // pelo lado dela; parado, a do lado de cima, acesa enquanto age (o
        // Avesso pula e vira; o Reflexivo pisca e sai rolando).
        if (!v->fantasma && !descartado) {
            const peca_t *pc = peca_fila(v->dono, v->slot);
            int t = v->tipo, lv = v->parado ? v->lado : (mostra >= ad_lado_min(t) && mostra <= ad_lado_max(t) ? mostra : 0);
            int a = ad_no_lado(pc, lv);
            if (a) {
                ad_face = a;
                ad_face_aceso = !v->parado || (v->etapa && (int)(v->t_etapa * 12) % 2 == 0);
                if (!v->parado && a == AD_REMENDO) mostra += 2;
            }
            if (v->etapa == 1 && v->t_etapa > AD_MOSTRA) {    // o Avesso: pula girando meia volta
                float q = fminf(1, (v->t_etapa - AD_MOSTRA) / AD_VIRA), s = sinf(q * 3.1416f);
                bob -= s * 14;
                ang += q * 3.1416f;
                r *= 1 + 0.15f * s;
                sombra_alt = s * 8;
            }
        }
        int tv = caixa ? D_PRESENTE : v->tipo;
        if (caixa && mostra > 6) mostra = (mostra - 1) % 6 + 1;   // a caixa e um d6
        // No ar, o dado mostra o que guarda (a carga, os patinhos, as balas -
        // a do Revolver ja com a bala deste lance), como na mao e na mesa.
        marca_nivel = v->slot < N_FILA ? F[v->dono].fixo[v->slot] : -1;
        bala_lancada = v->tipo == D_REVOLVER;
        // No pouso, o numero aparece com um pulo.
        num_cresce = !v->fantasma && v->t_assenta > 0.03f && v->t_assenta < 0.11f && mostra < 10
                     && TIPO[tv].lados != 4;                   // no triangulo o numero grande nao cabe
        face_curinga = tv == D_FICHA_CORINGA;                  // o curinga ate o fim da rodada
        if (tv == D_DIGITAL) digital_q = -1;                   // o visor so acende na sequencia
        desenha_dado(ex + dx, ey + bob, r, tv, v->dono, ang, mostra, 2, descartado);
        if (!v->fantasma && !descartado) ad_pouso_efeito(v, ex + dx, ey + bob, r);
        face_curinga = false;
        digital_q = 1;
        num_cresce = 0;
        marca_nivel = -1;
        bala_lancada = false;
        if (abre) presente_abre(ex + dx, ey + bob, t_fase / PRESENTE_ABRE);
        face_escondida = false;
        sr_lado = 0;
        sombra_alt = 0;
    }
    sombra_alt = 0;
}

static void peca_parada(float x, float y, float r, peca_t p, int dono)
{
    marca_nivel = p.fixo;
    desenha_dado(x, y, r, p.tipo, dono, 0, p.fixo, 0, false);
    marca_nivel = -1;
    ad_selos(x, y, r, &p, 0);                    // os adesivos: um selinho por lado, na beira de baixo
    if (p.selo) lacre_cera((int)(x + r * 0.72f), (int)(y - r * 0.72f), r >= 16 ? 4 : 3, 0);   // lacrado pelo Selo
    if (p.capa) gota_capa((int)(x - r * 0.74f), (int)(y - r * 0.5f), r >= 16 ? 3 : 2);        // com a capa do Impermeavel
    marca_protecao(x, y, r, nao_anulavel_t(p.tipo), nao_roubavel_t(p.tipo));
}

// O clarao do golpe (hora.inc): enquanto a mesa esta parada, o dado que
// acertou vira uma silhueta branca (o desenho dele, feito no rascunho, so
// para saber quais pixels sao dele), com um halo de um pixel; o placar
// acende a moldura e os numeros. Tudo apaga conforme o golpe solta.
static void clareia_dado(int j, int k, int pct)
{
    int cx = SLOT_X(k), cy = zona(j), caixa = R_DADO + 6;
    int x0 = cx - caixa, y0 = cy - caixa, lado = 2 * caixa + 1;
    for (int y = y0; y < y0 + lado; y++)
        for (int x = x0; x < x0 + lado; x++)
            if (x >= 0 && y >= 0 && x < GFX_W && y < GFX_H) rascunho[y * GFX_W + x] = CHAVE;
    gfx_desvia(rascunho);
    desenha_dado((float)cx, (float)cy, R_DADO, F[j].tipo[k], j, 0, F[j].valor[k], 2, false);
    gfx_desvia(NULL);
    for (int y = y0; y < y0 + lado; y++)                // a sombra no feltro nao acende
        for (int x = x0; x < x0 + lado; x++)
            if (x >= 0 && y >= 0 && x < GFX_W && y < GFX_H && rascunho[y * GFX_W + x] == C_SOMBRA)
                rascunho[y * GFX_W + x] = CHAVE;
    uint16_t *fb = display_fb();
    for (int y = y0 + 1; y < y0 + lado - 1; y++) {
        if (y < 1 || y >= GFX_H - 1) continue;
        for (int x = x0 + 1; x < x0 + lado - 1; x++) {
            if (x < 1 || x >= GFX_W - 1) continue;
            const uint16_t *r = &rascunho[y * GFX_W + x];
            if (*r != CHAVE) fb[y * GFX_W + x] = gfx_mistura(fb[y * GFX_W + x], C_BRANCO, pct);
            else if (r[-1] != CHAVE || r[1] != CHAVE || r[-GFX_W] != CHAVE || r[GFX_W] != CHAVE)
                fb[y * GFX_W + x] = gfx_mistura(fb[y * GFX_W + x], C_BRANCO, pct * 3 / 5);   // o halo
        }
    }
}

static void desenha_clarao(void)
{
    if (t_congela <= 0 || congela_j < 0) return;
    float q = t_congela / congela_dur;                 // 1 no golpe, 0 quando solta
    int pct = q > 0.5f ? 92 : 40 + (int)(104 * q), j = congela_j, y = zona(j);
    for (int k = 0; k < N_FILA; k++)
        if (congela_masc >> k & 1) clareia_dado(j, k, pct);
    if (congela_masc >> N_FILA & 1) {
        uint16_t c = gfx_mistura(C_OURO, C_BRANCO, pct);
        gfx_moldura(PLACAR_X - 36, y - 28, 72, 54, 2, c);
        char s[16];
        snprintf(s, sizeof s, "%d", placar_c[j].mostra);
        txt_c2(PLACAR_X, y - 20, s, c);
    }
}

// O lugar de um dado que ainda vai sair (ou que foi levado): o feltro escuro
// pontilhado, a meio tom (cheio, parecia um buraco na mesa).
static void lugar_marcado(int cx, int cy, int R)
{
    for (int y = -R; y <= R; y++)
        for (int x = -R; x <= R; x++)
            if (x * x + y * y <= R * R + R && bayer(cx + x, cy + y) < 0.5f) gfx_pixel(cx + x, cy + y, C_FELTRO_ESC);
}

// Destaque em volta de um dado da fila: as cantoneiras da selecao, na cor.
static void aro(int x, int y, uint16_t c)
{
    int m = R_DADO + 6;
    ui_cantos_em(x - m, y - m, 2 * m, 2 * m, c, ui_respira());
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
static void icone_da_run(int cx, int cy);         // desafio/bolsas.inc
static void amuletos_faixa(int j, int x, int y);   // Modo Desafiante (desafio.inc)
static void trunfo_faixa(int x, int y, int x_max);
static bool trunfo_ativo(int p);              // desafio/mapa.inc
static void desenha_amuleto(int x, int y, int r, int a, bool apagado);
static int  retrato_faixa(int j, int x, int y0);
static int  fichas_vis(int j);   // devolve onde o nome comeca

static void acinzenta(int x0, int y0, int w, int h, int pct);

// Onde fica a bolsa de j, na faixa dele.
static int bolsa_x(void) { return GFX_W - 118 + 8; }
static int bolsa_y(int j) { return (j == baixo ? GFX_H - FAIXA_H : 0) + FAIXA_H / 2; }

// Os dados nao lancados saindo da mesa: escorregam para o lado de quem
// jogou, diminuindo e apagando (nao voltam para a bolsa: ficam fora, como a
// mao).
static void desenha_devolvidos(void)
{
    for (int i = 0; t_devolve >= 0 && i < n_devolvidos; i++) {
        float q = (t_devolve - 0.07f * i) / DEVOLVE_T;
        if (q >= 1) continue;
        if (q < 0) q = 0;                              // ainda esperando a vez de sair
        float s = q * q * (3 - 2 * q);
        const devolvido_t *v = &devolvidos[i];
        float lado = v->dono == baixo ? 1.0f : -1.0f;
        float x = v->x, y = v->y + lado * 22 * s;
        float r = (R_DADO - 6) * (1 - 0.8f * s);
        marca_nivel = v->fixo;
        desenha_dado(x, y, r, v->tipo, v->dono, s * 1.2f, 0, 0, false);
        marca_nivel = -1;
        int R = (int)r + 4;                            // cinza como estava, enquanto sobre o feltro
        if (y - R > MESA_Y0 && y + R + 6 < MESA_Y1) acinzenta((int)x - R, (int)y - R, 2 * R + 6, 2 * R + 6, 80);
    }
}

static void faixa_jogador(int j)
{
    jogador_t *p = &J[j];
    bool cima = j != baixo;
    int y0 = cima ? 0 : GFX_H - FAIXA_H;
    faixa_madeira(y0);
    int ty = y0 + (FAIXA_H - TXT_H) / 2 - 2;
    // O oponente do Desafiante: o trunfo dele fica numa linha fina embaixo
    // do nome, sempre a vista; o resto da faixa sobe um pouco.
    bool com_trunfo = cima && modo == M_DESAFIO && j == 1 && fala_perso >= 0 && trunfo_ativo(fala_perso);
    if (com_trunfo) ty -= 5;

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
    snprintf(s, sizeof s, "%d", faixa_c[j].mostra);        // contando: dourado subindo, vinho descendo
    txt(x + 36, ty - (faixa_c[j].pulo > 0.4f ? 1 : 0), s, conta_cor(&faixa_c[j], C_MARFIM), true);
    int fim_fichas = x + 36 + gfx_largura(s, 1);

    // A rodada mora na faixa de cima; o som, na de baixo.
    if (cima && rodada > 0 && fase != F_LOJA) {
        int max_r = modo == M_DESAFIO ? DES_RODADAS : MAX_RODADAS;
        if (rodada > max_r) snprintf(s, sizeof s, "rodada extra  ·  entrada %d", ante_da(rodada));
        else snprintf(s, sizeof s, "rodada %d de %d  ·  entrada %d", rodada, max_r, ante_da(rodada));
        if (juros_rodada > 0) {                  // a Banqueira cobrou juros da sua entrada
            size_t n = strlen(s);
            snprintf(s + n, sizeof s - n, " (+%d juros)", juros_rodada);
        }
        int cx = GFX_W / 2, meio = gfx_largura(s, 1) / 2;
        if (cx - meio < fim_fichas + 20) cx = fim_fichas + 20 + meio;   // nome comprido: anda para a direita
        txt_c(cx, ty, s, t_aviso_entrada > 0 ? C_OURO : C_TEXTO_M, t_aviso_entrada > 0);
    }
    if (!cima && som_ligado()) icone_som(GFX_W / 2 - 8, y0 + FAIXA_H / 2 - 7);
    if (!cima && modo != M_ONLINE) {        // a velocidade, que o toque tambem troca
        static const char *const VEL_NOME[VEL_N] = { "» normal", "» médio", "» rápido" };
        const char *v = VEL_NOME[velocidade];
        int vx = GFX_W / 2 + 22;
        gfx_texto(vx, y0 + FAIXA_H / 2 - 10, v, velocidade == VEL_RAPIDA ? C_OURO : velocidade == VEL_MEDIA ? C_MARFIM_S : C_TEXTO_M, 1, false);
        zona_clique(vx - 4, y0 + 6, gfx_largura(v, 1) + 8, FAIXA_H - 12, NULL, 0, 'v');
    }
    if (modo == M_DESAFIO)                  // os do oponente, junto da bolsa (o meio e da rodada)
        amuletos_faixa(j, j == 0 ? x + 60 + gfx_largura(s, 1) : GFX_W - 142, y0 + FAIXA_H / 2);

    // A bolsa: quantos ainda estao nela, de quantos o jogador tem. Clicar
    // abre o pop-up com os dados dela (bolsa.inc), a sua ou a do oponente.
    int bx = GFX_W - 118, by = y0 + FAIXA_H / 2;
    if (com_trunfo) trunfo_faixa(xn, y0 + FAIXA_H - 22, bx - 12);
    bool em = bolsa_pop < 0 && mouse_em(bx - 2, y0 + 4, GFX_W - bx, FAIXA_H - 8);
    if (modo == M_DESAFIO && p == &J[0]) icone_da_run(bx + 8, by - em);   // a bolsa que voce escolheu
    else icone_bolsa(bx + 8, by - em);
    snprintf(s, sizeof s, "bolsa %d/%d", na_bolsa(p), em_jogo(p));
    txt(bx + 22, ty, s, em ? C_OURO : C_MARFIM_S, false);
    zona_clique(bx - 2, y0 + 4, GFX_W - bx, FAIXA_H - 8, NULL, j, TECLA_BOLSA);
    bolsas_vistas |= (uint8_t)(1 << j);
}

// Saquinho de couro fechado por um cordao dourado; (cx, cy) e o meio dele.
// Como os outros icones: o contorno escuro e o couro em tons da paleta.
static void icone_bolsa(int cx, int cy)
{
    const uint16_t borda = C_BARRA, couro = C_TERRA, claro = RGB(200, 142, 104), escuro = C_MADEIRA_CLR;
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

// Um presente do Treno: caixa na cor c, fita de ouro em cruz e o laco em cima.
static void presente(int x, int y, uint16_t c)
{
    gfx_rect(x - 4, y - 3, 9, 8, C_BARRA);
    gfx_rect(x - 3, y - 2, 7, 6, c);
    gfx_rect(x, y - 2, 1, 6, C_OURO);
    gfx_rect(x - 3, y, 7, 1, C_OURO);
    gfx_rect(x - 2, y - 5, 2, 2, C_OURO);
    gfx_rect(x + 1, y - 5, 2, 2, C_OURO);
}
// Os presentes do Treno saem um atras do outro: quando o do n-esimo dado sai
// (p do evento), e o tempo do voo.
#define TRENO_VOO 0.30f
static float treno_sai(int n) { return 0.06f + n * 0.16f; }
static bool treno_agora(const evento_t *e)
{
    return e && e->tipo == EV_BONUS && e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_TRENO;
}
// Quando o presente chega ao dado k (a ordem dele entre os que recebem).
static float treno_chega(const evento_t *e, int k)
{
    int n = 0;
    for (int i = 0; i < k; i++) n += e->mascara >> i & 1;
    return treno_sai(n) + TRENO_VOO;
}

// Um patinho de perfil, olhando para a direita: (x, y) e o meio do corpo.
static void patinho(int x, int y)
{
    gfx_rect(x - 4, y - 1, 9, 4, C_AMARELO);         // o corpo
    gfx_rect(x - 6, y - 2, 3, 2, C_AMARELO);         // o rabinho
    gfx_rect(x + 1, y - 5, 4, 4, C_AMARELO);         // a cabeca
    gfx_rect(x + 5, y - 3, 2, 1, C_FOGO);            // o bico
    gfx_pixel(x + 3, y - 4, C_BARRA);                // o olho
    gfx_rect(x - 3, y + 3, 7, 1, C_LATAO);           // a sombra do corpo
}

// Uma moedinha de ouro (a Meia na Lareira pagando).
static void moedinha(int x, int y)
{
    gfx_disco(x, y, 4, C_LATAO_ESC);
    gfx_disco(x, y - 1, 3, C_OURO);
    gfx_rect(x - 1, y - 3, 2, 1, C_AMARELO);
}
// Uma estrelinha de quatro pontas em (x, y), de raio r.
static void estrelinha(int x, int y, int r, uint16_t c)
{
    gfx_rect(x - r, y, 2 * r + 1, 1, c);
    gfx_rect(x, y - r, 1, 2 * r + 1, c);
    if (r >= 3) { gfx_rect(x - 1, y - 1, 3, 3, c); gfx_pixel(x, y, C_BRANCO); }
}

// O gelo do Boneco de Neve sobre o dado em (x, y), q de 0 a 1: a camada
// palida que se espalha do meio, os cristais e os pingentes embaixo.
static void gelo_sobre(int x, int y, float q)
{
    if (q <= 0) return;
    if (q > 1) q = 1;
    uint16_t *fb = display_fb();
    int rr = (int)((R_DADO + 4) * (0.35f + 0.65f * q));
    for (int yy = y - rr; yy <= y + rr; yy++)
        for (int xx = x - rr; xx <= x + rr; xx++) {
            if (xx < 0 || yy < 0 || xx >= GFX_W || yy >= GFX_H) continue;
            if ((xx - x) * (xx - x) + (yy - y) * (yy - y) > rr * rr) continue;
            fb[yy * GFX_W + xx] = gfx_mistura(fb[yy * GFX_W + xx], C_VENTO, 42);
        }
    for (int i = 0; i < 6; i++) {                    // os cristais, com dois ramos cada
        float an = i * 1.0472f + 0.26f, L = R_DADO * 0.95f * q;
        float cs = cosf(an), sn = sinf(an);
        gfx_linha(x, y, x + (int)(cs * L), y + (int)(sn * L), C_BRANCO);
        int mx = x + (int)(cs * L * 0.6f), my = y + (int)(sn * L * 0.6f);
        for (int k = -1; k <= 1; k += 2)
            gfx_linha(mx, my, mx + (int)(cosf(an + k * 0.8f) * L * 0.28f), my + (int)(sinf(an + k * 0.8f) * L * 0.28f), C_BRANCO);
    }
    if (q > 0.5f)                                    // os pingentes, crescendo embaixo
        for (int i = -1; i <= 1; i++) {
            int h = (int)((q - 0.5f) * 2 * (i ? 5 : 8));
            gfx_rect(x + i * 8, y + R_DADO - 2, 2, h, C_BRANCO);
            gfx_rect(x + i * 8, y + R_DADO - 2, 1, h + 1, C_VENTO);
        }
}

// Os Jingle Bells tocando (o bonus deles no fim): os sinos sobem um pouco da
// fila, para o lado do meio da mesa (assim passam por cima dos outros dados),
// se juntam no ar e batem tres vezes um no outro, e voltam.
static bool sinos_agora(const evento_t *e)
{
    return e && e->tipo == EV_EXTRA && e->mascara && e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_JINGLE;
}
static float suave(float a, float b, float p)    // 0 ate a, 1 depois de b, suave no meio
{
    float q = (p - a) / (b - a);
    q = q < 0 ? 0 : q > 1 ? 1 : q;
    return q * q * (3 - 2 * q);
}
static float sinos_no_ar(float p) { return suave(0.0f, 0.10f, p) * (1 - suave(0.84f, 0.94f, p)); }   // sobem e descem
static float sinos_perto(float p) { return suave(0.08f, 0.18f, p) * (1 - suave(0.76f, 0.86f, p)); }  // e chegam perto la em cima
static float sinos_juntos(float p)               // la em cima: 0 afastados, 1 encostados
{
    if (p < 0.16f || p > 0.78f) return 0;
    return 1 - fabsf(cosf((p - 0.16f) / 0.62f * 3 * 3.1416f));
}
// Onde o sino k esta agora: todos em volta do meio deles, cada vez mais perto.
static void sino_pos(const evento_t *e, float p, int k, int *x, int *y)
{
    int n = 0, ik = 0;
    float soma = 0;
    for (int i = 0; i < N_FILA; i++)
        if (e->mascara >> i & 1) { if (i == k) ik = n; soma += (float)SLOT_X(i); n++; }
    float vao = 2 * R_DADO + 14 - 12 * sinos_juntos(p);
    float alvo = soma / (float)n + (ik - (n - 1) / 2.0f) * vao;
    *x = SLOT_X(k) + (int)((alvo - SLOT_X(k)) * sinos_perto(p));
    *y = zona(e->de_j) + (int)((e->de_j == baixo ? -40 : 40) * sinos_no_ar(p));
}

// Uma notinha musical subindo (cabeca, haste e bandeira).
static void notinha(int x, int y, uint16_t c)
{
    gfx_rect(x - 2, y, 3, 2, c);
    gfx_rect(x, y - 6, 1, 7, c);
    gfx_rect(x + 1, y - 6, 2, 1, c);
    gfx_rect(x + 2, y - 5, 1, 1, c);
}

// A crista de uma onda andando para a frente (dir: +1 descendo, -1 subindo
// na tela), larga e curva, com espuma na beira.
static void crista(int x, int y, int dir, int larg)
{
    for (int i = -larg; i <= larg; i++) {
        float t = i / (float)larg;
        int yy = y - dir * (int)(t * t * 7);
        gfx_disco(x + i * 3, yy - dir * 3, 4, C_AZUL);
        gfx_disco(x + i * 3, yy, 3, RGB(96, 182, 198));
        if ((i + larg) % 2 == 0) gfx_rect(x + i * 3 - 1, yy + dir * 3, 3, 2, C_BRANCO);
    }
}

// O Ventania de j acabou de cair e esta rolando o primeiro dado?
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
            if (c == mesa_acesa[i] || c == C_FELTRO_ESC || c == C_SOMBRA || c == C_BARRA) continue;   // o feltro (com a luz) e o contorno ficam
            int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
            int l = 70 + ((r * 77 + g * 150 + b * 29) >> 8) * 45 / 100;   // cinza, puxado para o meio
            r += (l - r) * pct / 100;
            g += (l - g) * pct / 100;
            b += (l - b) * pct / 100;
            fb[i] = RGB(r, g, b);
        }
}

// Capitao: um galao dourado sai dele e pousa em cada dado seu que vale, um
// depois do outro (ele mesmo incluido). Quando, no evento (p de 0 a 1), o
// galao do dado k sai; ele chega CAPITAO_VOO depois.
#define CAPITAO_VOO 0.26f
static float capitao_sai(const evento_t *e, int k)
{
    int n = 0;
    for (int i = 0; i < k; i++) n += e->mascara >> i & 1;
    return 0.08f + n * 0.12f;
}
static bool capitao_agora(const evento_t *e)
{
    return e && e->tipo == EV_EXTRA && e->mascara && F[e->de_j].tipo[e->de_k] == D_CAPITAO;
}

// Poder armado: uma borda de luz em volta do dado, rente a forma dele,
// pulsando devagar, com um ponto claro correndo por ela. bom: dourada;
// senao (o poder age contra quem o jogou), vinho.
static void brilho_poder(float cx, float cy, float r, int tipo, float ang, bool bom)
{
    float pulso = 0.5f + 0.5f * sinf(t_relogio * 4.5f);
    uint16_t c = bom ? C_OURO : C_VINHO_CLR, claro = bom ? C_AMARELO : C_ROSA;
    uint16_t forte = gfx_mistura(c, C_FELTRO, (int)(45 - 40 * pulso));
    uint16_t fraca = gfx_mistura(c, C_FELTRO, (int)(85 - 30 * pulso));   // o halo de fora
    int lados = TIPO[tipo].lados;
    float fase_luz = fmodf(t_relogio * 0.55f, 1.0f);
    float ea = r < 12 ? 1 : r / 10;                    // a borda de fora do aro do dado (desenho_dado.inc)
    if (lados == 2 || tipo == D_FOGO || tipo == D_TUDO_NADA) {   // moeda e fichas: anel
        for (int o = 1; o >= 0; o--) {
            float rr = r + ea + 3 + o * 2;
            for (int a = 0; a < 96; a++) {
                float an = a * 0.06545f;
                gfx_rect((int)lroundf(cx + cosf(an) * rr) - !o / 2, (int)lroundf(cy + sinf(an) * rr) - !o / 2, 1 + !o, 1 + !o,
                         o ? fraca : forte);
            }
        }
        float an = fase_luz * 6.2832f;
        gfx_rect((int)lroundf(cx + cosf(an) * (r + ea + 3)) - 1, (int)lroundf(cy + sinf(an) * (r + ea + 3)) - 1, 3, 3, claro);
        return;
    }
    // A mesma forma do dado, com as bordas afastadas por igual (forma_d): a luz
    // corre paralela ao aro, no mesmo centro.
    float vx[12], vy[12];
    int n = 0;
    for (int o = 1; o >= 0; o--) {
        n = forma_d(lados, cx, cy, r, ang, ea * 0.85f + 2.5f + o * 2, vx, vy);
        for (int i = 0; i < n; i++) {
            int k = (i + 1) % n;
            gfx_linha_grossa((int)lroundf(vx[i]), (int)lroundf(vy[i]), (int)lroundf(vx[k]), (int)lroundf(vy[k]),
                             o ? 1 : 2, o ? fraca : forte);
        }
    }
    if (n < 2) return;
    float q = fase_luz * n;                            // o ponto claro, de lado em lado
    int i = (int)q % n, k = (i + 1) % n;
    float u = q - (int)q;
    int px = (int)lroundf(vx[i] + (vx[k] - vx[i]) * u), py = (int)lroundf(vy[i] + (vy[k] - vy[i]) * u);
    gfx_rect(px - 1, py - 1, 3, 3, claro);
}

// Um galao: a tira dourada com a borda escura.
static void galao(int cx, int cy)
{
    gfx_rect(cx - 7, cy - 2, 14, 5, C_LATAO_ESC);
    gfx_rect(cx - 6, cy - 1, 12, 3, C_OURO);
    gfx_rect(cx - 6, cy - 1, 12, 1, C_AMARELO);
}

// A carta do Coringa: so um "?" no lugar do dado. Na fila, esperando a vez,
// e menor e parado, como os dados que ainda nao foram lancados; posta, em
// suspensao ate o seu turno acabar, fica do tamanho de um dado, flutuando.
static void desenha_carta(int x, int y, bool posta)
{
    if (!posta) {
        lugar_marcado(x, y, R_DADO - 2);
        txt_sombra_c(x, y - 20, "?", C_OURO, 2);                  // o glifo: 10 a 30 da linha
        return;
    }
    int b = (int)lroundf(sinf(t_relogio * 3.0f) * 3);
    txt_sombra_c(x, y - 30 + b, "?", C_OURO, 3);                  // 30 px de altura, centrado no lugar
}

// O placar esquenta com os pontos da rodada: de PLACAR_MORNO para cima o
// filete, depois uma luz em volta da placa e por fim os numeros sobem pela
// rampa de dourados (em degraus da paleta), e pulsam - mais forte e mais
// depressa quanto mais pontos; passando de PLACAR_QUENTE, soltam faiscas
// douradas, mais quanto mais alto.
#define PLACAR_MORNO  12
#define PLACAR_QUENTE 30
static const uint16_t RAMPA_OURO[5] = { C_LATAO_ESC, C_LATAO, C_OURO, C_AMARELO, C_BRANCO };
static uint16_t rampa_ouro(float v) { int i = (int)v; return RAMPA_OURO[i < 0 ? 0 : i > 4 ? 4 : i]; }

static float placar_calor(int pts)       // 0 frio, 1 em PLACAR_QUENTE; passa de 1 acima
{
    return pts <= PLACAR_MORNO ? 0 : (float)(pts - PLACAR_MORNO) / (PLACAR_QUENTE - PLACAR_MORNO);
}

static float placar_pulso(float calor)   // 0..1, no tempo real
{
    float q = calor > 1 ? 1 : calor, alem = calor > 1 ? fminf(calor - 1, 1) : 0;
    float hz = 0.7f + 1.5f * q + 1.3f * alem;
    return 0.5f + 0.5f * sinf(t_relogio * hz * 6.2832f);
}

// A luz em volta da placa (x, y, w, h): aneis de um pixel, cada um um degrau
// abaixo do de dentro; mais aneis quanto mais quente.
static void placar_halo(int x, int y, int w, int h, float calor, float pulso)
{
    float q = calor > 1 ? 1 : calor;
    int aneis = q < 0.3f ? 0 : q < 0.65f ? 1 : calor <= 1.3f ? 2 : 3;
    float topo = q * 2 + pulso * q;                       // o degrau do anel de dentro
    for (int i = 1; i <= aneis; i++) {
        uint16_t c = rampa_ouro(topo - (i - 1));
        gfx_rect(x - i, y - i, w + 2 * i, 1, c);
        gfx_rect(x - i, y + h - 1 + i, w + 2 * i, 1, c);
        gfx_rect(x - i, y - i + 1, 1, h + 2 * i - 2, c);
        gfx_rect(x + w - 1 + i, y - i + 1, 1, h + 2 * i - 2, c);
    }
}

// As faiscas de cima de PLACAR_QUENTE: cruzinhas douradas de miolo branco
// saindo da beira da placa e subindo, cada uma no seu tempo.
static void placar_faiscas(int x, int y, int w, int h, float alem)
{
    int n = 3 + (int)(fminf(alem, 1.5f) * 8);
    for (int i = 0; i < n; i++) {
        float f = fmodf(t_relogio * (0.8f + 0.12f * (i % 5)) + i * 0.37f, 1.0f);
        int lado = i % 3;                                   // de cima, ou dos lados
        int sx = lado == 0 ? x + 4 + (i * 23) % (w - 8) : lado == 1 ? x - 2 : x + w + 1;
        int sy = lado == 0 ? y - 1 : y + 6 + (i * 17) % (h - 12);
        int px = sx + (int)(sinf(f * 6 + i) * 3) + (lado == 1 ? -(int)(f * 10) : lado == 2 ? (int)(f * 10) : 0);
        int py = sy - (int)(f * 30);
        uint16_t c = i % 2 ? C_AMARELO : C_OURO;
        if (f < 0.45f) {
            gfx_rect(px - 2, py, 5, 1, c);
            gfx_rect(px, py - 2, 1, 5, c);
            gfx_rect(px, py, 1, 1, C_BRANCO);
        } else if (f < 0.8f) {
            gfx_rect(px - 1, py, 3, 1, c);
            gfx_rect(px, py - 1, 1, 3, c);
        } else gfx_rect(px, py, 1, 1, C_OURO);
    }
}

// A Sanguessuga que acabou de pousar sugando o dado de antes (em xa): gotas
// saem dele, uma atras da outra, e correm em arco ate ela (em xs). Devolve
// quanto ela incha agora (cada gota que chega e um gole).
#define SANGUE_GOTAS 5
static float sangue_parte(int i) { return 0.10f + i * 0.08f; }
#define SANGUE_VOO 0.28f
static float sanguessuga_gole(float t)
{
    float g = 0;
    for (int i = 0; i < SANGUE_GOTAS; i++) {
        float d = t - sangue_parte(i) - SANGUE_VOO;
        if (d > 0 && d < 0.3f) g += 2.2f * expf(-d / 0.07f);
    }
    return g > 3 ? 3 : g;
}
static void sanguessuga_gotas(int xa, int xs, int y, float t)
{
    for (int i = 0; i < SANGUE_GOTAS; i++) {
        float q = (t - sangue_parte(i)) / SANGUE_VOO;
        if (q < 0 || q >= 1) continue;
        float sq = q * q * (3 - 2 * q);
        int gx = xa + (int)((xs - xa) * sq), gy = y - 6 - (int)(sinf(sq * 3.1416f) * 20);
        gfx_disco(gx, gy, 4, C_VINHO);                       // a gota: borda vinho, miolo vermelho, brilho
        gfx_disco(gx, gy, 3, C_VERMELHO);
        gfx_rect(gx - 2, gy - 2, 2, 1, C_ROSA);
        if (q > 0.15f) gfx_rect(gx - (xs > xa ? 7 : -6), gy + 1, 2, 2, C_VINHO);   // um respingo atras
    }
}

// O fim da rodada: de cada Jingle Bell lancado sai a copia, que voa em arco
// para a bolsa de quem o jogou; a bolsa acende quando ela chega.
static void desenha_jingle_copias(void)
{
    if (fase != F_RESULTADO) return;
    for (int j = 0; j < 2; j++) {
        int n = 0, bx = bolsa_x(), by = bolsa_y(j);
        for (int k = 0; k < N_FILA; k++) {
            if (!(jingle_copias[j] >> k & 1)) continue;
            float q = (t_fase - 0.25f - n++ * 0.12f) / 0.6f;
            if (q < 0 || q > 1.4f) continue;
            if (q < 1) {
                float sq = q * q * (3 - 2 * q);
                int sx = SLOT_X(k), sy = zona(j), arco = j == baixo ? 40 : -30;
                int x = sx + (int)((bx - sx) * sq), y = sy + (int)((by - sy) * sq) - (int)(sinf(sq * 3.1416f) * arco);
                desenha_dado((float)x, (float)y, 14 - sq * 2, D_JINGLE, j, sinf(q * 12) * 0.3f, 6, 0, false);
            } else {
                float r2 = (q - 1) / 0.4f;
                int rr = 9 + (int)(r2 * 10);
                for (int a = 0; a < 28; a++) {
                    float an = a * 0.2244f;
                    gfx_rect(bx + (int)(cosf(an) * rr), by + (int)(sinf(an) * rr), 1, 1, gfx_mistura(C_OURO, C_BARRA, (int)(r2 * 85)));
                }
                int ty = j == baixo ? by - 26 - (int)(r2 * 6) : by + 16 + (int)(r2 * 6);
                txt_sombra_c(bx, ty, "+1 Jingle Bell", C_OURO, 1);
            }
        }
    }
}

static void rachadura(int x, int y, int r, float q);
static void rachadura_em(int x, int y, int r, float q, uint16_t clara, uint16_t escura);
static void escudo_capa(int x, int y, float abre, float q, uint16_t c);   // o guarda-chuva do Impermeavel (abaixo)
static void desenha_fila(int j)
{
    fila_t *f = &F[j];
    int y = zona(j);
    bool no_desfecho = fase == F_EFEITOS || fase == F_RESULTADO;
    const evento_t *e = (fase == F_EFEITOS && ev_i < n_ev && t_respiro < 0) ? &ev[ev_i] : NULL;
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
    uint8_t mira[2] = { 0, 0 };                          // os dados que um poder armado vai pegar
    if (previa) miras_agora(mira);

    for (int k = 0; k < f->n; k++) {
        int x = SLOT_X(k);
        bool voando = n_voo && (fase == F_ROLANDO || fase == F_ARRUMA)
                      && voo[0].dono == j && voo[0].slot == k;
        if (voando) continue;
        int t = tipo_de(j, k);
        if (f->carta && k == 3) {                // a carta do Coringa: na fila ou posta
            if (fala_perso >= 0) dica_zona(x - R, y - R, 2 * R, 2 * R, DICA_TRUNFO, fala_perso);
            desenha_carta(x, y, k < f->lancados);
            if (k >= f->lancados && (fase == F_JOGA || fase == F_ROLANDO || fase == F_ARRUMA || fase == F_VEREDITO))
                acinzenta(x - R, y - R, 2 * R + 6, 2 * R + 6, 80);       // como os dados que ainda nao foram
            if (k < f->lancados && fase == F_VEREDITO && j == vez && t_fase < 0.6f) aro(x, y, C_OURO);
            continue;
        }
        if (j == 0 && k >= f->lancados && f->idx[k] == embrulho) dica_zona(x - R, y - R, 2 * R, 2 * R, DICA_DADO, DICA_EMBRULHO);
        else if (peca_fila(j, k)) dica_peca(x - R, y - R, 2 * R, 2 * R, peca_fila(j, k));
        else dica_zona(x - R, y - R, 2 * R, 2 * R, DICA_DADO, t);
        if (k >= f->lancados) {                  // ainda vai sair: o dado menor, sem nada atras
            // O proximo a sair, na vez de jogar: quase do tamanho cheio, na
            // cor dele, respirando devagar.
            bool prox = fase == F_JOGA && j == vez && k == f->lancados;
            float rp = prox ? R - 3 + sinf(t_relogio * 4.2f) * 1.2f : R - 6;
            marca_nivel = f->fixo[k];
            caixa_fechada = j == 0 && f->idx[k] == embrulho;
            desenha_dado((float)x, (float)y, rp, t, j, 0, f->fixo[k], 0, false);
            marca_nivel = -1;
            const peca_t *pc = peca_fila(j, k);
            if (pc) ad_selos((float)x, (float)y, rp, pc, 0);
            // Na hora de lancar, o que ainda nao foi jogado fica cinza;
            // montando a sequencia, cada dado na sua cor.
            if (!prox && (fase == F_JOGA || fase == F_ROLANDO || fase == F_ARRUMA || fase == F_VEREDITO))
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
            lugar_marcado(x, y, R - 2);
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
                           mesa_acesa + yy * GFX_W + x - R - 6, (size_t)(2 * R + 12) * 2);
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
        if (recente && t_fase < 0.5f) x += (int)(sinf(t_fase * 60) * 3);   // treme enquanto o X entra
        if (e && e->tipo == EV_CAVALO && e->de_j == j && e->de_k == k && p < 0.3f)
            x += (int)(sinf(t_fase * 70) * 3);                      // o Cavalo treme e se abre
        // Anulado: pela sequencia, apaga e o X vinho entra em dois tracos; por
        // um ataque no fim da rodada, sem o X: racha na cor de quem atacou.
        uint16_t golpe = anul && no_desfecho ? vis_golpe_cor[j][k] : 0;
        // Alvo de anulacao agora: pisca antes de cair.
        bool alvo = e && (e->tipo == EV_ANULA || (e->tipo == EV_ADESIVO && (e->ad == AD_MIRA || e->ad == AD_RATOEIRA))) && e->alvo_j == j && e->alvo_k == k;
        bool congela = alvo && e->tipo == EV_ANULA && F[e->de_j].tipo[e->de_k] == D_BONECO;   // o gelo prende: nao pisca
        bool engolido = alvo && e->tipo == EV_ANULA && F[e->de_j].tipo[e->de_k] == D_BURACO;  // o Buraco Negro puxa: gira
        if (alvo && !congela && !engolido && p > 0.4f && ((int)(t_fase * 8) % 2 == 0)) continue;
        bool cavalo = e && e->tipo == EV_CAVALO && e->alvo_j == j && (e->mascara >> k & 1);
        if (cavalo && p > 0.6f && ((int)(t_fase * 8) % 2 == 0)) continue;
        // O dado que acabou de valer da um pulinho.
        float pulo = 0;
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && ver_tipo == 0 && t_fase < 0.25f)
            pulo = sinf(t_fase / 0.25f * 3.1416f) * 8;
        if (!anul) pulo += onda_pulo(j, k);
        // Os Jingle Bells tocando: sobem, batem um no outro e voltam.
        if (sinos_agora(e) && e->de_j == j && (e->mascara >> k & 1)) {
            int sx, sy;
            sino_pos(e, p, k, &sx, &sy);
            x += sx - SLOT_X(k);
            pulo += (float)(y - sy);
        }
        // Espelho que so vira no fim: mostra o que tirou ate o evento dele
        // chegar na metade, e entao o numero do rival.
        bool ainda = no_desfecho && vis_espelho[j][k]
                     && !(e && e->tipo == EV_ESPELHO && e->de_j == j && e->de_k == k && p > 0.5f);
        // Ventania: o primeiro dado rodopia mostrando faces ao acaso e
        // assenta no numero novo.
        int face = ainda ? f->cru[k] : f->valor[k];
        float giro = 0;
        // O dado que o Terremoto sacode: o numero de antes ate pousar de novo;
        // no evento, treme, salta rodopiando e cai no numero novo.
        bool tremendo = e && e->tipo == EV_TREMOR && e->alvo_j == j && e->alvo_k == k;
        if (no_desfecho && vis_tremor[j][k] && !(tremendo && p >= TREMOR_POUSA)) face = vis_tremor_de[j][k];
        if (tremendo && p < TREMOR_SALTA) x += (int)(sinf(t_fase * 70) * 3 * p / TREMOR_SALTA);
        else if (tremendo && p < TREMOR_POUSA) {
            float q = (p - TREMOR_SALTA) / (TREMOR_POUSA - TREMOR_SALTA);
            pulo += sinf(q * 3.1416f) * 26;
            giro = (1 - q) * 11 + 0.6f;
            if (q < 0.85f) face = face_v(t, f->valor[k]);
        }
        // O Terremoto que age: treme no lugar enquanto a falha corre.
        if (e && e->tipo == EV_TREMOR && e->de_j == j && e->de_k == k && p < TREMOR_SALTA)
            x += (int)(sinf(t_fase * 90) * 3);
        // A Ficha Coringa: o curinga ate o evento dela; no evento, sobe girando
        // e cai com o numero novo para cima.
        bool virando = e && e->tipo == EV_CORINGA && e->de_j == j && e->de_k == k;
        bool vira_vazia = virando && e->alvo_k >= N_FILA;   // sem nada valido para copiar: treme e e anulada
        face_curinga = f->curinga[k] || f->curinga_nada[k] || (no_desfecho && vis_curinga[j][k] && !(virando && p > CORINGA_VIRA));
        if (vira_vazia && p > 0.2f) x += (int)(sinf(t_fase * 60) * 2 * (1 - p));
        float giro_ficha = -1;                   // a largura aparente da ficha girando (-1: deitada)
        if (virando && !vira_vazia && p > CORINGA_SOBE && p < CORINGA_POUSA) {
            float q = (p - CORINGA_SOBE) / (CORINGA_POUSA - CORINGA_SOBE);
            pulo += sinf(q * 3.1416f) * 22;
            giro_ficha = cosf(q * 5 * 3.1416f);   // duas voltas e meia: cai do outro lado
        }
        // O Digital que acabou de entrar: o visor acende.
        if (t == D_DIGITAL && fase == F_VEREDITO && j == vez && k == f->lancados - 1 && t_fase < DIGITAL_ACENDE)
            digital_q = t_fase / DIGITAL_ACENDE;
        if (engolido) {                          // puxado pelo Buraco Negro: gira e chega mais perto
            giro = p * p * 9;
            x += (int)((SLOT_X(e->de_k) - x) * 0.25f * (p < 0.75f ? p / 0.75f : 1));
        }
        if (ventania_agora(j) && k == 0) {
            float q = t_fase / 0.7f;
            giro = (1 - q) * (1 - q) * 9;
            x += (int)(sinf(t_fase * 40 + k) * (1 - q) * 3);
            if (q < 0.7f) face = face_v(t, f->valor[k]);
        }
        marca_nivel = f->fixo[k];
        // O Oculto: "?" ate o evento dele chegar na metade.
        face_escondida = f->escondido[k] || (no_desfecho && vis_oculto[j][k]
                         && !(e && e->tipo == EV_REVELA && e->de_j == j && e->de_k == k && p > 0.5f));
        // Acabou de pousar valendo: com o pulinho, um brilho atravessa o dado.
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && ver_tipo == 0 && t_fase >= 0.05f && t_fase < 0.45f)
            brilho_passa = (t_fase - 0.05f) / 0.4f;
        // A Sanguessuga sugando o de antes incha a cada gole.
        bool suga = fase == F_VEREDITO && j == vez && k == f->lancados - 1 && t == D_SANGUESSUGA
                    && anulou_a >= 0 && t_fase < 0.9f;
        float r_suga = suga ? R + sanguessuga_gole(t_fase) : R;
        // A Ficha Velha pagando o seu +4 no fim: a cor volta enquanto ela paga.
        bool paga = e && e->tipo == EV_EXTRA && e->de_j == j && e->de_k == k && t == D_FICHA_VELHA && p > 0.12f && p < 0.92f;
        // O adesivo do lado de cima: a etiqueta, acesa enquanto ele age no fim.
        if (!ainda && giro < 0.5f) {
            ad_face = ad_fila(j, k);
            ad_face_aceso = e && e->tipo == EV_ADESIVO && e->de_j == j && e->de_k == k && (p > 0.3f || (int)(t_fase * 12) % 2 == 0);
            // O Post-it aceso enquanto o dado seguinte rola as duas vezes.
            if (ad_face == AD_POSTIT && (fase == F_ROLANDO || fase == F_ARRUMA) && n_voo && voo[0].dono == j && voo[0].slot == k + 1)
                ad_face_aceso = true;
        }
        // A Onda que juntou 9 vira Tsunami no meio do evento dela (e fica).
        int td = t;
        if (t == D_ONDA && no_desfecho && (vis_tsunami[j][k] || (e && e->tipo == EV_ONDA && e->de_j == j && e->de_k == k && p > 0.48f)))
            td = D_TSUNAMI;
        patos_pousou = t == D_PATINHOS && !anul;   // valendo na mesa: esta vez ja conta
        bala_lancada = t == D_REVOLVER;            // na mesa, valendo ou nao: a bala desta vez ja entrou
        frutas_desta = t == D_PANETONE ? f->fruta[k] : -1;
        if (recente) risca_anima(t_fase);
        sem_risca = golpe != 0;                              // a trinca ja diz
        face_capa = impermeavel(j, k);
        if (giro_ficha > -1) moeda_girando_arte((float)x, y - pulo, r_suga, td, j, giro_ficha, face);
        else desenha_dado((float)x, y - pulo, r_suga, td, j, giro, face, 2, anul && giro < 0.5f && !paga);
        if (face_capa && giro < 0.5f) gota_capa((int)(x - R * 0.78f), (int)(y - pulo - R * 0.55f), 3);   // com a capa
        face_capa = false;
        float tc = t_relogio - vis_capa_t[j][k];                // acabou de segurar um efeito: o guarda-chuva fecha
        if (no_desfecho && vis_capa_t[j][k] > 0 && tc >= 0 && tc < 0.45f && giro < 0.5f)
            escudo_capa(x, (int)(y - pulo), 1 - tc / 0.45f, 0, C_VENTO);
        sem_risca = false;
        face_curinga = false;
        digital_q = 1;
        risca_anima(9);
        if (golpe && giro < 0.5f) {                           // as trincas crescem do meio
            float qg = (t_relogio - vis_golpe_t[j][k]) / 0.22f;
            rachadura_em(x, (int)(y - pulo), R, qg < 0 || qg > 1 ? 1 : qg, golpe, C_BARRA);
        }
        patos_pousou = bala_lancada = false;
        frutas_desta = -1;
        if (f->presente[k] && giro < 0.5f) lacinho(x + R - 3, (int)(y - pulo) - R + 2);   // saiu de um Presente
        const peca_t *pl = peca_fila(j, k);                // lacrado pelo Selo: ate o lacre romper (ou desde que fechou)
        bool lacrado = pl && pl->selo;
        if (no_desfecho) lacrado = (lacrado && !(vis_selo[j][0] >> k & 1)) || (vis_selo[j][1] >> k & 1);
        if (lacrado && giro < 0.5f && !f->presente[k]) lacre_cera(x + R - 4, (int)(y - pulo) - R + 4, 4, 0);
        if (suga) sanguessuga_gotas(SLOT_X(anulou_a), x, y, t_fase);
        if (anul && no_desfecho && vis_gelo[j][k]) gelo_sobre(x, (int)(y - pulo), 0.8f);   // congelado pelo Boneco
        if (no_desfecho && (vis_fragil[j] >> k & 1) && giro < 0.5f) rachadura(x, (int)(y - pulo), R, 1);   // o Fragil rachou
        face_escondida = false;
        marca_nivel = -1;
        // A copia do Coringa: uma carta de baralho no canto - o dado e seu, nao dele.
        if (coringa_joga(j) && k == 3 && f->idx[k] < 0) {
            int cx2 = x - R - 2, cy2 = (int)(y - pulo) - R - 4;
            gfx_rect(cx2 - 1, cy2 - 1, 9, 12, C_BARRA);
            gfx_rect(cx2, cy2, 7, 10, C_BRANCO);
            gfx_rect(cx2 + 3, cy2 + 2, 1, 6, C_REAL); gfx_rect(cx2 + 2, cy2 + 3, 3, 4, C_REAL); gfx_rect(cx2 + 1, cy2 + 4, 5, 2, C_REAL);
        }
        if (!anul && t_onda >= 0 && j == onda_j && k < onda_n && onda_nivel >= 2 && t_onda < 0.9f)
            faiscas(x, (int)(y - pulo), t_onda + k * 0.3f, C_AMARELO);
        // Durante a sequencia: o dado que tirou o que o poder dele pede brilha.
        if (!no_desfecho && fase != F_FIM && giro < 0.5f) {
            int arm = poder_armado(j, k);
            if (arm) {
                brilho_poder((float)x, y - pulo, R, t, giro, arm == 1);
                anima_poder(x, (int)(y - pulo), R, t, k * 0.37f + j * 0.19f);   // e o desenho do que ele vai fazer
            }
        }
        // O ciclo cosmico que vai evoluir: uma estrelinha gira em volta dele.
        if (f->evolui[k] && t != D_UNIVERSO && f->idx[k] >= 0 && k < f->lancados && fase != F_FIM && giro < 0.5f
            && !(fase == F_VEREDITO && j == vez && k == f->lancados - 1 && t_evolui >= 0 && t_evolui < EVOLUI_CLARAO))
            cosmo_orbita(x, (int)(y - pulo), R, t);
        if ((mira[j] >> k & 1) && !anul && giro < 0.5f) mira_sobre(x, (int)(y - pulo), R);
        // As protecoes de agora: lancado, conta a Ancora e o Transparente.
        if (!anul && giro < 0.5f && vis_est[j][k] != V_ROUBADO) {
            bool la = k < f->lancados;
            marca_protecao((float)x, y - pulo, R, la ? firme(j, k) : nao_anulavel_t(t), la ? nao_roubavel(j, k) : nao_roubavel_t(t));
        }
        // Preso pela Ancora: dois elos de corrente ligam o dado a ela.
        if (f->ancora[k] && !anul && vis_est[j][k] != V_ROUBADO) {
            int mx = (SLOT_X(k - 1) + x) / 2, my = (int)(y - pulo);
            for (int i = -1; i <= 1; i++) {      // tres elos, deitado, em pe, deitado
                int ex = mx + i * 7;
                if (i == 0) { gfx_rect(ex - 2, my - 4, 5, 9, C_PRATA); gfx_rect(ex - 1, my - 2, 3, 5, C_FELTRO_ESC); }
                else        { gfx_rect(ex - 4, my - 2, 9, 5, C_PRATA); gfx_rect(ex - 2, my - 1, 5, 3, C_FELTRO_ESC); }
            }
        }
        // Petrificado pela Medusa (ou alcancado agora pela onda dela): a borda
        // de pedra e duas rachaduras.
        bool pedra = no_desfecho && (vis_pedra[j][k] || (e && e->tipo == EV_MEDUSA && e->de_j == j
                     && (e->mascara >> k & 1) && t_fase / duracao_evento(ev_i) > 0.1f + 0.45f * abs(k - e->de_k) / 3.0f));
        if (pedra) {
            int yy = (int)(y - pulo);
            aro(x, yy, C_CINZA);
            gfx_linha_grossa(x - R / 2, yy - R / 2, x - 2, yy + 1, 1, C_EBANO);
            gfx_linha_grossa(x - 2, yy + 1, x + 1, yy + R / 2, 1, C_EBANO);
            gfx_linha_grossa(x + R / 3, yy - R / 2, x + R / 5, yy - 2, 1, C_EBANO);
        }
        // Cortado pelo Sabotador: uma linha tracejada na diagonal; vale metade.
        if (no_desfecho && vis_corte[j][k] && !anul)
            for (int i = 0; i < 2 * R; i += 5)
                gfx_linha_grossa(x - R + i, (int)(y - pulo) - R + i, x - R + i + 3, (int)(y - pulo) - R + i + 3, 2, C_VINHO_CLR);
        // Acabou de pousar: o Eco faz ondas do primeiro dado ate ele; o Calibre
        // mostra a mira sobre o dado que ele subiu.
        if (fase == F_VEREDITO && j == vez && k == f->lancados - 1 && t_fase < 0.7f) {
            if (t == D_ECO && !strncmp(f->tag[k], "=", 1)) {
                int p0 = 0;
                while (p0 < k && f->est[p0] != V_VALIDO) p0++;
                for (int o = 0; o < 3; o++) {
                    float q = t_fase / 0.5f - o * 0.15f;
                    if (q < 0 || q > 1) continue;
                    int cx2 = SLOT_X(p0) + (int)((x - SLOT_X(p0)) * q);
                    for (int a = -4; a <= 4; a++) {
                        float an = a * 0.15f;
                        gfx_rect(cx2 + (int)(cosf(an) * 10), (int)(y - pulo) + (int)(sinf(an) * 10), 2, 2,
                                 gfx_mistura(C_AZUL, C_FELTRO, (int)(q * 60)));
                    }
                }
            }
            // Depois de uma Ponte (ou de um Corretivo): o arco tracejado sai do
            // dado com que este se compara e passa por cima dela.
            if (k > 0 && t != D_PONTE) {
                int a = anulou_a >= 0 && anulou_a < k ? anulou_a : ultimo_da_regra(j, f->est, k);
                bool ponte = false, corretivo = false;
                for (int i = a + 1; a >= 0 && i < k; i++) {
                    ponte |= f->tipo[i] == D_PONTE;
                    corretivo |= f->est[i] == V_VALIDO && ad_fila(j, i) == AD_CORRETIVO;
                }
                if (ponte || corretivo) {
                    uint16_t c_arco = ponte ? cor_dado(D_PONTE) : ADESIVO[AD_CORRETIVO].fundo;
                    int x0 = SLOT_X(a), yy = (int)(y - pulo) - R - 4;
                    float q = t_fase / 0.35f;
                    for (int d = 0; d <= 24; d++) {
                        float s = d / 24.0f;
                        if (s > q || d % 3 == 2) continue;            // tracejado, nascendo de a para k
                        int px = x0 + (int)((x - x0) * s), py = yy - (int)(sinf(s * 3.1416f) * 18);
                        gfx_rect(px - 1, py - 1, 2, 2, gfx_mistura(c_arco, C_FELTRO, (int)(t_fase / 0.7f * 60)));
                    }
                }
            }
            // A Furia: tirou 4, as chamas sobem dela; tirou 1, solta fumaca.
            if (t == D_FURIA && (f->valor[k] == 4 || f->valor[k] == 1)) {
                bool brava = f->valor[k] == 4;
                for (int i = 0; i < 14; i++) {
                    float q = fmodf(t_fase * 1.8f + i * 0.09f, 1.0f);
                    int px = x - R + 2 + (i * 29) % (2 * R - 4) + (int)(sinf(t_fase * 9 + i) * 2);
                    int py = (int)(y - pulo) + R / 3 - (int)(q * (2 * R + 14));
                    int tam = q < 0.4f ? 4 : q < 0.75f ? 3 : 2;
                    uint16_t c = brava ? (i % 3 ? C_FOGO : C_AMARELO) : (i % 2 ? C_CINZA : C_MARFIM_S);
                    gfx_rect(px, py, tam, tam, gfx_mistura(c, C_FELTRO, (int)(q * 40)));
                }
            }
            if (f->calibre[k]) {
                int rr = R + 6 - (int)(t_fase * 8), yy = (int)(y - pulo);
                for (int a = 0; a < 32; a++) {
                    float an = a * 0.19635f;
                    gfx_rect(x + (int)(cosf(an) * rr), yy + (int)(sinf(an) * rr), 1, 1, C_VERMELHO);
                }
                gfx_rect(x - rr - 4, yy, 6, 1, C_VERMELHO); gfx_rect(x + rr - 1, yy, 6, 1, C_VERMELHO);
                gfx_rect(x, yy - rr - 4, 1, 6, C_VERMELHO); gfx_rect(x, yy + rr - 1, 1, 6, C_VERMELHO);
            }
        }
        // Protegido contra ataques pelo dono (Couraca: nao e anulado; Ima: nao
        // e roubado): o medalhao do amuleto no pe do dado.
        if (!anul && modo == M_DESAFIO) {
            int n = 0;
            if (tem_am(j, A_COURACA)) desenha_amuleto(x + R - 1 - 13 * n++, (int)(y - pulo) + R - 2, 6, A_COURACA, false);
            if (tem_am(j, A_IMA))     desenha_amuleto(x + R - 1 - 13 * n++, (int)(y - pulo) + R - 2, 6, A_IMA, false);
        }
        // Etiqueta: no desfecho, o multiplicador somado (x4) ou o bonus da
        // Moeda (+2); antes, o aviso do que vem (x2 do Dobro, +8, =9).
        char tg[16] = "";
        if (no_desfecho && vis_mult[j][k] > 1) snprintf(tg, sizeof tg, "x%d", vis_mult[j][k]);
        else if (no_desfecho && vis_bonus[j][k]) snprintf(tg, sizeof tg, "%+d", vis_bonus[j][k]);
        else if (ainda) snprintf(tg, sizeof tg, "=?");
        else if (f->tag[k][0] && td == t) snprintf(tg, sizeof tg, "%s", f->tag[k]);   // a Onda que virou Tsunami ja nao guarda
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
        bool sino = sinos_agora(e) && e->de_j == j;    // os sinos voando: o aro de quem age nao segue
        if (e && e->de_j == j && e->de_k == k && !sino) aro(x, y, C_VINHO_CLR);   // quem age
        if (e && e->tipo == EV_VAMPIRO && e->alvo_j == j && (e->mascara >> k & 1) && p > 0.15f + k * 0.08f)
            aro(x, y, C_VINHO_CLR);              // o Vampiro bebe deste
        if (alvo || (cavalo && p > 0.3f)) aro(x, y, C_VINHO_CLR);
        // Capitao: o galao pousou neste dado.
        if (capitao_agora(e) && e->de_j == j && (e->mascara >> k & 1)
            && p > capitao_sai(e, k) + CAPITAO_VOO) {
            aro(x, y, C_OURO);
            galao(x, y - R - 7);
        }
        // Checkpoint: depois que a bandeira sobe, os dados depois dele acendem.
        if (e && e->tipo == EV_EXTRA && e->de_j == j && (e->mascara >> k & 1)
            && F[j].tipo[e->de_k] == D_CHECKPOINT && p > 0.3f + 0.08f * (k - e->de_k)) {
            aro(x, y, C_OURO);
            faiscas(x, y, t_fase + k * 0.3f, C_AMARELO);
        }
        // Martir, Hexa e Domino: os dados que recebem acendem quando a luz chega.
        if (e && e->tipo == EV_EXTRA && e->de_j == j && (e->mascara >> k & 1)) {
            int td = F[j].tipo[e->de_k];
            float chega = td == D_MARTIR ? 0.6f + k * 0.06f : td == D_DOMINO ? 0.45f : td == D_HEXA ? 0.38f : 2;
            if (p > chega) { aro(x, y, C_OURO); faiscas(x, y, t_fase + k * 0.3f, C_AMARELO); }
        }
        if (e && e->tipo == EV_EXTRA && e->de_j == j && F[j].tipo[e->de_k] == D_PUXA_FILA
            && p > 0.1f + 0.12f * k && p < 0.95f)
            aro(x, y, C_OURO);
        // Bonus e multiplicadores: aro dourado nos dados que recebem.
        if (e && (e->tipo == EV_BONUS || e->tipo == EV_MULT || e->tipo == EV_JACKPOT) && e->de_j == j
            && (e->mascara >> k & 1) && p > (treno_agora(e) ? treno_chega(e, k) : 0.2f)) {
            bool perde = e->tipo == EV_BONUS && e->por_dado[k] < 0;     // Tudo ou Nada com 1
            aro(x, y, perde ? C_VINHO_CLR : C_OURO);
            if (!perde) faiscas(x, y, t_fase + k * 0.3f, C_AMARELO);
        }
        if (e && e->tipo == EV_BONUS && e->de_j == j && e->de_k == k) aro(x, y, C_OURO);
        // Esperanca dobrando: um leque de luz se abre por tras dela, de baixo.
        if (e && e->tipo == EV_MULT && e->de_j == j && (e->mascara >> k & 1) && t == D_ESPERANCA && p < 0.95f) {
            float q = p < 0.3f ? p / 0.3f : 1, some = p > 0.75f ? (0.95f - p) / 0.2f : 1;
            for (int i = 0; i < 9; i++) {
                float an = 3.1416f + (i + 0.5f) * 3.1416f / 9;
                int r0 = R + 4, r1 = R + 4 + (int)(q * (i % 2 ? 14 : 22));
                gfx_linha_grossa(x + (int)(cosf(an) * r0), y + 6 + (int)(sinf(an) * r0),
                                 x + (int)(cosf(an) * r1), y + 6 + (int)(sinf(an) * r1), 1,
                                 gfx_mistura(i % 2 ? C_VENTO : C_AMARELO, C_FELTRO, (int)((1 - some) * 90)));
            }
        }
        // Marcado pelo fogo: aro em brasa ate o fim da rodada.
        if (no_desfecho && (vis_queima[j][k] ||
            (e && e->tipo == EV_FOGO && e->alvo_j == j && e->alvo_k == k && p > 0.35f && !(e->bloqueado >> k & 1))))
            aro(x, y, ((int)(t_fase * 10) % 2) ? C_FOGO : C_BRASA);
        if (e && (e->tipo == EV_FOGO || e->tipo == EV_SEMENTE) && e->de_j == j && e->de_k == k)
            aro(x, y, e->tipo == EV_FOGO ? C_FOGO : C_VERDE);
        if (e && e->tipo == EV_EXTRA && e->de_j == j && e->de_k == k && !sino) aro(x, y, cor_dado(t));
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
    // Rajadas da Ventania saindo dela e passando ate o primeiro dado.
    if (ventania_agora(j))
        for (int i = 0; i < 7; i++) {
            float q = t_fase / 0.7f;
            float alcance = (float)(SLOT_X(f->lancados - 1) - SLOT_X(0) + 90);
            int xs = SLOT_X(f->lancados - 1) + 10 - (int)fmodf(t_fase * 500 + i * 61, alcance);
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
            int rx = ROUBO_X(i);
            if (e && e->tipo == EV_CAVALO && e->alvo_j == j && r->tipo == D_CAVALO && p < 0.3f)
                rx += (int)(sinf(t_fase * 70) * 3);               // o Cavalo roubado treme e se abre
            desenha_dado((float)rx, (float)y, R_ROUBO, r->tipo, r->de_j, 0, r->valor, 2, r->anulado);
        }

    if (f->lancados || vis_nroubo[j] || (previa && pv.bruto[j] != 0)) {
        char s[16];
        // O numero conta ate o valor de verdade (hora.inc); a cada passo da
        // um pulinho e acende: dourado subindo, vinho descendo.
        const contador_t *pc = &placar_c[j];
        snprintf(s, sizeof s, "%d", pc->mostra);
        // No resultado o placar fala sozinho: o de quem levou fecha as
        // cantoneiras e ganha a plaquinha; o zero treme e pisca em vinho; no
        // empate os dois acendem iguais.
        bool res = fase == F_RESULTADO, zero = res && zerou == j, empate = res && venc_mao == 2;
        int sx = zero && t_fase < 0.7f ? (int)(sinf(t_fase * 48) * 3 * (1 - t_fase / 0.7f)) : 0;
        uint16_t c = (res && venc_mao == j) ? J[j].cor
                   : (j == vez && !no_desfecho ? C_MARFIM : C_TEXTO_M);
        if (zero) c = t_fase < 1.2f && (int)(t_fase * 6) % 2 ? C_BRANCO : C_VINHO_CLR;
        if (empate) c = C_MARFIM;
        float calor = placar_calor(pc->mostra), pulso = calor > 0 ? placar_pulso(calor) : 0;
        float q = calor > 1 ? 1 : calor;
        if (q >= 0.45f) c = rampa_ouro(3 + pulso * (q - 0.45f) * 2.2f);   // os numeros: amarelo, pulsando ao branco
        if (pc->pulo > 0) c = gfx_mistura(c, pc->sinal > 0 ? C_OURO : C_VINHO_CLR, (int)(pc->pulo * 80));
        ui_painel(PLACAR_X - 34 + sx, y - 26, 68, 50);
        if (zero) ui_contorno(PLACAR_X - 32 + sx, y - 24, 64, 46, t_fase < 1.2f && (int)(t_fase * 6) % 2 ? C_VINHO : C_VINHO_CLR);
        if (empate) ui_cantos(PLACAR_X - 34, y - 26, 68, 50, C_MARFIM_S);
        if (calor > 0) {                          // esquentando: o filete e a luz em volta
            ui_contorno(PLACAR_X - 32, y - 24, 64, 46, rampa_ouro(1 + q * 2 + pulso * q));
            placar_halo(PLACAR_X - 34, y - 26, 68, 50, calor, pulso);
        }
        bool vence = res ? venc_mao == j : vence_j == j;   // passou do alvo (ou levou): a rodada e dele
        if (vence) {                              // as cantoneiras chegam de longe, fecham e respiram
            float tv = res ? t_fase : t_vence;
            int a = tv < 0.45f ? (int)((1 - tv / 0.45f) * 14) : 0;
            ui_cantos_em(PLACAR_X - 34, y - 26, 68, 50, C_OURO, 3 + a + ui_respira());
            c = C_OURO;
        }
        if (vence)                                // a plaquinha de latao, presa na beira do placar
            ui_plaqueta(PLACAR_X, (j == baixo ? y + 24 : y - 26) - UI_PLAQUETA_H / 2, "VENCE", C_OURO);
        else txt_c(PLACAR_X, y - 42 + (j == baixo ? 72 : 0), "pontos", C_TEXTO_M, false);
        txt_c2(PLACAR_X + sx, y - 20 - (pc->pulo > 0.4f ? 2 : 0), s, c);
        if (calor > 1) placar_faiscas(PLACAR_X - 34, y - 26, 68, 50, calor - 1);
        // Ao lado, apagado: quanto fica depois dos bonus e dos ataques que ja
        // estao na mesa. Pode ser negativo (Espinhoso, Pirata, Maldito do rival).
        if (previa && pv.bruto[j] != placar_real(j)) {
            snprintf(s, sizeof s, "(%d)", pv.bruto[j]);
            txt(PLACAR_X + 38, y - 10, s, gfx_mistura(C_TEXTO_M, C_FELTRO, 25), false);
        }
        if (carta_posta(j) || tem_escondido(j))   // a carta do Coringa ou um Oculto ainda vai virar
            txt(PLACAR_X + 38, y - 26, "+?", C_OURO, false);
    }
}

// Um adesivo agindo no fim da rodada: o adesivo sai do dado e faz o gesto
// dele. p vai de 0 a 1 no tempo do evento; (ax, ay) e o dado.
// A rachadura do Fragil anulado, crescendo do meio do dado (q: 0..1). Com
// as cores: a do dado que levou um ataque (a clara, a de quem atacou).
static void rachadura(int x, int y, int r, float q) { rachadura_em(x, y, r, q, C_BRANCO, C_SOMBRA); }
static void rachadura_em(int x, int y, int r, float q, uint16_t clara, uint16_t escura)
{
    static const int8_t R[4][4][2] = {        // quatro trincas em zigue-zague, do meio para a beira
        { { -2, -3 }, { -5, -4 }, { -7, -8 }, { -11, -10 } }, { { 3, -1 }, { 6, -4 }, { 9, -3 }, { 12, -6 } },
        { { 1, 3 }, { 0, 7 }, { 3, 9 }, { 2, 13 } },          { { -3, 2 }, { -7, 1 }, { -9, 5 }, { -13, 6 } },
    };
    float esc = r / 13.0f;
    for (int t = 0; t < 4; t++) {
        int x0 = x, y0 = y, n = (int)(q * 4 + 0.999f);
        for (int s = 0; s < n && s < 4; s++) {
            int x1 = x + (int)(R[t][s][0] * esc), y1 = y + (int)(R[t][s][1] * esc);
            gfx_linha(x0 + 1, y0 + 1, x1 + 1, y1 + 1, clara);
            gfx_linha(x0, y0, x1, y1, escura);
            x0 = x1; y0 = y1;
        }
    }
}

// O guarda-chuva do Impermeavel: a copa (meia elipse de raio rw x rh, com o
// meio da beira em cx, cy) em gomos amarelos, a beira azul, o brilho do
// plastico e o cabo descendo 'cabo' pixels.
static void guarda_chuva(int cx, int cy, int rw, int rh, int cabo)
{
    if (rw < 2 || rh < 2) return;
    uint16_t capa = ADESIVO[AD_IMPERMEAVEL].fundo;
    gfx_linha_grossa(cx, cy, cx, cy + cabo, 2, C_EBANO);       // o cabo
    for (int y = -rh - 1; y <= 1; y++) {                      // o contorno da copa
        int w = (int)((rw + 1) * sqrtf(fmaxf(0, 1 - (float)(y * y) / ((rh + 1) * (rh + 1)))));
        gfx_rect(cx - w, cy + y, 2 * w + 1, 1, C_BARRA);
    }
    for (int y = -rh; y <= 0; y++) {                          // os gomos, em duas cores
        int w = (int)(rw * sqrtf(1 - (float)(y * y) / (rh * rh)));
        for (int x = -w; x <= w; x++)
            gfx_pixel(cx + x, cy + y, y == 0 ? C_AZUL : ((x + 64) / 5) & 1 ? capa : RGB(228, 194, 106));
    }
    if (rw > 8) gfx_rect(cx - rw / 2, cy - rh + 2, 2, 1, C_BRANCO);   // o brilho
}
// O Impermeavel segurando um efeito no dado em (x, y): o guarda-chuva abre
// por cima dele (abre: 0 fechado, 1 aberto) e o que chega (fogo ou agua, na
// cor c) espirra para os lados da copa. q: o tempo do espirro (0..1).
static void escudo_capa(int x, int y, float abre, float q, uint16_t c)
{
    if (abre <= 0) return;
    int rw = (int)(4 + abre * (R_DADO + 3)), rh = (int)(3 + abre * 9), cy = y - R_DADO - 3;
    guarda_chuva(x, cy, rw, rh, R_DADO - 2);
    for (int i = 0; i < 8 && q > 0 && q < 1; i++) {           // o espirro, dos dois lados
        int lado = i % 2 ? 1 : -1;
        float t = fmodf(q * 1.6f + i * 0.13f, 1.0f);
        int px = x + lado * (rw + 1 + (int)(t * 12)), py = cy - 2 + (int)(t * t * 22) - (int)(sinf(t * 3.1416f) * 6);
        gfx_rect(px, py, 2, 2, i % 3 ? c : C_BRANCO);
    }
}

static void desenha_adesivo_agindo(const evento_t *e, float p, int ax, int ay)
{
    int a = e->ad, j = e->de_j, dir = j == baixo ? -1 : 1;     // para o meio da mesa
    const adesivo_t *d = &ADESIVO[a];
    int salta = e->copia ? AD_HOLOGRAFICO : a;                 // o Holografico copiando: e ele que pula
    uint16_t fundo = d->fundo == AD_SEM_FUNDO ? d->luz : d->fundo;
    // O adesivo pula do dado e fica no ar, girando um nada, enquanto age
    // (a Raspadinha se solta e vai embora).
    float sobe = fminf(1, p / 0.15f);
    int sx = ax + R_DADO - 2, sy = ay + dir * (R_DADO + (int)((2 - sobe) * sobe * 10));
    if (a == AD_RASPADINHA && p > 0.55f) {                     // descola e voa para fora
        float q = (p - 0.55f) / 0.4f;
        sx += (int)(q * 60);
        sy += dir * (int)(q * 30) - (int)(sinf(q * 3.1416f) * 20);
    }
    bool rompe = a == AD_SELO && e->valor;                     // o lacre de antes: so ele age, sem o adesivo
    if (p < 0.9f && !rompe) ad_solto_d(sx, sy, salta, 1, a == AD_RASPADINHA && !e->copia ? fminf(1, p * 2) : 0);
    if (e->copia && p < 0.85f) {                                // o brilho que muda de cor em volta dele
        static const uint16_t ARCO[6] = { RGB(208, 98, 108), RGB(246, 222, 132), RGB(88, 152, 84),
                                          RGB(96, 182, 198), RGB(96, 134, 196), RGB(150, 112, 160) };
        for (int i = 0; i < 10; i++) {
            float an = i * 0.628f + t_fase * 3;
            int rr = 9 + (int)(fminf(1, p / 0.2f) * 4);
            gfx_rect(sx + (int)(cosf(an) * rr), sy + (int)(sinf(an) * rr), 2, 2, ARCO[(i + (int)(t_fase * 10)) % 6]);
        }
    }
    switch (a) {
    case AD_FRAGIL:
        if (e->valor) {                                         // valendo: um brilho de vidro atravessa o dado
            float q = fminf(1, p / 0.5f);
            for (int i = -2; i <= 2 && p < 0.6f; i++) {
                int bx = ax - R_DADO + (int)(q * 2 * R_DADO) + i;
                gfx_linha(bx - 6, ay + 8, bx + 6, ay - 8, i ? gfx_mistura(C_BRANCO, fundo, 50) : C_BRANCO);
            }
        } else if (!e->bloqueado) {                             // anulado: racha e solta lascas (com a capa, nao)
            rachadura(ax, ay, R_DADO, fminf(1, p / 0.45f));
            for (int i = 0; i < 7 && p > 0.4f && p < 0.95f; i++) {
                float q = (p - 0.4f) / 0.55f;
                int x = ax - 9 + (i * 13) % 19 + (int)((i - 3) * q * 6), y = ay + (int)(q * q * 40) - (int)(sinf(q * 3.1416f) * 8);
                gfx_rect(x, y, i % 2 ? 2 : 3, 2, i % 3 ? C_BRANCO : fundo);
            }
        }
        break;
    case AD_LENTICULAR: {                                       // o cromo 3D vira de um lado e do outro e para no x2 (ou no -4)
        int cx = ax - R_DADO - 18, cy = ay + dir * 4;
        float q = fminf(1, p / 0.55f);
        float vira = cosf(q * 3.1416f * 5);                     // vira cinco vezes, cada vez mais devagar ate parar
        int w = (int)(fabsf(vira) * 9) + 1, h = 12;
        bool frente = q >= 1 ? e->valor > 0 : (vira > 0) == (e->valor > 0);
        if (p < 0.92f) {
            gfx_rect(cx - w + 1, cy - h + 2, 2 * w, 2 * h, C_SOMBRA);
            gfx_rect(cx - w - 1, cy - h - 1, 2 * w + 2, 2 * h + 2, C_BARRA);
            gfx_rect(cx - w, cy - h, 2 * w, 2 * h, frente ? d->fundo : C_VINHO);
            for (int x = cx - w + 1; x < cx + w; x += 3) gfx_rect(x, cy - h, 1, 2 * h, frente ? RGB(102, 74, 138) : C_BRASA);   // as lentes
            if (w >= 6) txt_c(cx, cy - 8, frente ? "x2" : "-4", frente ? C_AMARELO : C_ROSA, true);
        }
        if (q >= 1 && p < 0.92f) {                              // parou: brilha (x2) ou pinga (-4)
            for (int i = 0; i < 6; i++) {
                float an = i * 1.047f + p * 5;
                if (e->valor > 0 && (int)(t_fase * 12 + i) % 2) gfx_rect(cx + (int)(cosf(an) * 15), cy + (int)(sinf(an) * 17), 2, 2, C_AMARELO);
            }
            if (e->valor < 0) {
                float g = fmodf(p * 2.5f, 1.0f);
                gfx_rect(cx - 1, cy + h + 2 + (int)(g * 14), 2, 3, C_ROSA);
            }
        }
        break;
    }
    case AD_NEON: {                                             // o luminoso acende em volta do dado, um pedaco por adesivo,
        int n = e->valor / NEON_CADA;                           // piscando como luminoso; aceso, pulsa e uma luz corre nele
        if (n < 1) n = 1;
        if (n > 20) n = 20;
        int rr = R_DADO + 7;
        float pulso = 0.5f + 0.5f * sinf(t_fase * 13), corre = t_fase * 7;
        for (int i = 0; i < n && p < 0.95f; i++) {
            float liga = p - 0.06f - i * (0.4f / n);
            if (liga < 0) continue;
            bool pisca = liga < 0.12f && (int)(liga * 60) % 3 == 0;   // o luminoso pisca ao ligar
            if (pisca) continue;
            float a0 = -1.5708f + i * 6.2832f / n, a1 = a0 + 6.2832f / n - 0.18f;
            int g = pulso > 0.5f ? 2 : 1;                       // o brilho respira
            for (float an = a0; an < a1; an += 0.05f) {
                int px = ax + (int)(cosf(an) * rr), py = ay + (int)(sinf(an) * rr);
                gfx_rect(px - g, py - g, 2 * g + 1, 2 * g + 1, pulso > 0.75f ? C_VINHO_CLR : C_VINHO);
            }
            if (pulso > 0.6f)                                   // no pico, um anel de brilho mais largo, pontilhado
                for (float an = a0; an < a1; an += 0.09f) {
                    int px = ax + (int)(cosf(an) * (rr + 5)), py = ay + (int)(sinf(an) * (rr + 5));
                    if (((px + py) & 1) == 0) gfx_pixel(px, py, C_VINHO);
                }
            for (float an = a0; an < a1; an += 0.04f) {
                int px = ax + (int)(cosf(an) * rr), py = ay + (int)(sinf(an) * rr);
                float d = fabsf(fmodf(an - corre + 62.832f, 6.2832f) - 3.1416f);   // a luz que corre
                gfx_pixel(px, py, d > 2.9f ? C_BRANCO : pulso > 0.7f && (int)(an * 20) % 3 == 0 ? C_BRANCO : C_ROSA);
                if (d > 2.9f) gfx_rect(px - 1, py - 1, 2, 2, C_BRANCO);
            }
        }
        break;
    }
    case AD_IMPERMEAVEL: {                                      // o guarda-chuva abre em cima do dado e a chuva escorre pelos lados
        float q = fminf(1, p / 0.3f), s = q * q * (3 - 2 * q);
        int cx = ax - 7, cy = ay - R_DADO - 10, rw = (int)(4 + s * 15), rh = (int)(3 + s * 8);
        if (p < 0.92f) guarda_chuva(cx, cy, rw, rh, ay - 4 - cy);
        for (int i = 0; i < 14 && p > 0.25f && p < 0.95f; i++) {         // a chuva: cai, bate na copa e escorre para fora
            float g = fmodf((p - 0.25f) * 2.2f + i * 0.071f, 1.0f);
            int x0 = cx - rw - 8 + (i * 29) % (2 * rw + 16);
            int topo = cy - rh - 34 + (int)(g * 60);
            bool em_cima = abs(x0 - cx) <= rw && topo >= cy - rh;
            if (em_cima) {                                         // bateu: escorre para o lado de fora da copa
                int lado = x0 < cx ? -1 : 1, sx = cx + lado * (rw + 2 + (int)((topo - (cy - rh)) * 0.5f));
                if (topo - (cy - rh) < 14) gfx_rect(sx, cy + (topo - (cy - rh)) / 2, 1, 2, C_VENTO);
            } else gfx_rect(x0, topo, 1, 3, RGB(96, 134, 196));
        }
        break;
    }
    case AD_ESTRELINHA:                                         // uma estrelinha pula em cima de cada dado
        for (int k = 0; k < N_FILA && p < 0.95f; k++) {
            float q = (p - 0.1f - k * 0.1f) / 0.35f;
            if (q <= 0) continue;
            if (q > 1) q = 1;
            int x = SLOT_X(k), y = zona(j) + dir * (R_DADO + 6) - (int)(sinf(q * 3.1416f) * 10) * (dir < 0 ? 1 : -1);
            ad_icone(x - 3, y - 3, AD_ESTRELINHA, 1, d->tinta);
            if (q >= 1 && (int)(t_fase * 12 + k) % 3) gfx_pixel(x + 4, y - 4, C_BRANCO);
        }
        break;
    case AD_IMANTADO: {                                         // as linhas do ima entre os dois dados
        int bx = SLOT_X(e->alvo_k);
        for (int l = 0; l < 3 && p < 0.9f; l++) {
            int alto = 8 + l * 7;
            for (int i = 0; i <= 16; i++) {
                float s2 = i / 16.0f;
                if ((i + (int)(t_fase * 20)) % 4 == 0) continue;   // a corrente andando
                int x = ax + (int)((bx - ax) * s2), y = ay + dir * (R_DADO - 4 + (int)(sinf(s2 * 3.1416f) * alto));
                gfx_rect(x, y, 2, 2, l == 1 ? d->tinta : d->luz);
            }
        }
        break;
    }
    case AD_PURPURINA:                                          // um punhado de brilho sobe do dado e cai piscando
        for (int i = 0; i < 40 && p < 0.95f; i++) {
            static const uint16_t K[3] = { RGB(246, 222, 132), C_BRANCO, RGB(226, 164, 168) };
            float q = fminf(1, p / 0.9f);
            float vx = ((i * 37) % 21 - 10) * 3.2f, vy = 60 + (i * 53) % 40;
            int x = ax + (int)(vx * q), y = ay - (int)(vy * q) + (int)(150 * q * q) * (dir < 0 ? 1 : 1);
            if ((i + (int)(t_fase * 15)) % 4 == 0) continue;     // piscando
            uint16_t k = K[i % 3];
            if (i % 4 == 0) {                                   // os maiores: uma cruzinha
                gfx_pixel(x - 1, y, k); gfx_pixel(x + 1, y, k); gfx_pixel(x, y - 1, k); gfx_pixel(x, y + 1, k);
            }
            gfx_rect(x, y, i % 3 ? 1 : 2, i % 3 ? 1 : 2, k);
        }
        break;
    case AD_VELCRO: {                                           // as duas fitas se soltam e grudam de volta
        float q = p < 0.35f ? p / 0.35f : p < 0.6f ? 1 : fmaxf(0, 1 - (p - 0.6f) / 0.15f);
        int abre = (int)(q * 8), y0 = ay + dir * (R_DADO + 6);
        for (int f = 0; f < 2 && p < 0.92f; f++) {
            int y = y0 + (f ? abre : -abre) / 2 + (f ? 2 : -3);
            gfx_rect(ax - 13, y + 1, 26, 4, C_SOMBRA);
            gfx_rect(ax - 14, y, 26, 4, f ? d->tinta : gfx_mistura(d->tinta, C_BRANCO, 40));
            for (int i = 0; i < 7; i++) gfx_pixel(ax - 13 + i * 4, f ? y - 1 : y + 4, f ? d->tinta : C_BRANCO);   // os ganchinhos
        }
        break;
    }
    case AD_FIGURINHA: {                                        // as figurinhas juntadas abrem em leque
        int n = e->valor > 0 ? e->valor : 1;
        if (n > 9) n = 9;
        for (int c = 0; c < n && p < 0.95f; c++) {
            float q = (p - 0.1f - c * 0.06f) / 0.25f;
            if (q <= 0) continue;
            if (q > 1) q = 1;
            float an = (c - (n - 1) / 2.0f) * 0.42f;              // o leque, de um lado ao outro
            int rr = (int)(q * (R_DADO + 26));
            int x = ax + (int)(sinf(an) * rr), y = ay + dir * (int)(cosf(an) * rr);
            bool nova = c == n - 1;                             // a ultima (a deste lance) pisca
            gfx_rect(x - 3, y - 4 + 1, 7, 9, C_SOMBRA);
            gfx_rect(x - 4, y - 5, 8, 10, nova && (int)(t_fase * 12) % 2 ? C_OURO : d->tinta);
            gfx_rect(x - 3, y - 4, 6, 8, C_BRANCO);
            gfx_rect(x - 1, y - 2, 2, 2, RGB(190, 56, 60));      // o jogador do cromo
            gfx_rect(x - 2, y, 4, 2, RGB(190, 56, 60));
        }
        break;
    }
    case AD_PATINHO: {                                          // o patinho pula do dado: feliz (+2) ou com a nuvem (-2)
        float q = fminf(1, p / 0.5f);
        int x = ax - R_DADO - 14, y = ay + dir * (R_DADO + 6) - (int)(sinf(q * 3.1416f * 2) * 6) * (dir < 0 ? 1 : -1);
        if (p < 0.92f) {
            gfx_disco(x, y, 14, C_SOMBRA);
            gfx_disco(x, y - 1, 13, d->fundo);
            ad_icone(x - 10, y - 11, AD_PATINHO, 3, d->tinta);
        }
        if (e->valor < 0) {                                     // a nuvem de chuva em cima dele
            if (p > 0.15f && p < 0.92f) {
                gfx_disco(x - 4, y - 20, 5, C_ACO); gfx_disco(x + 4, y - 21, 6, C_ACO);
                for (int i = 0; i < 3; i++) {
                    float g = fmodf(p * 3 + i * 0.33f, 1.0f);
                    gfx_rect(x - 5 + i * 5, y - 14 + (int)(g * 12), 1, 3, RGB(96, 134, 196));
                }
            }
        } else if (p > 0.3f && p < 0.92f)                       // brilhinhos
            for (int i = 0; i < 4; i++) {
                float an = i * 1.57f + p * 6;
                if ((int)(t_fase * 12 + i) % 2) gfx_rect(x + (int)(cosf(an) * 17), y + (int)(sinf(an) * 15), 2, 2, C_BRANCO);
            }
        break;
    }
    case AD_ETIQUETA: {                                         // as etiquetas da bolsa caem uma sobre a outra
        int n = e->valor > 0 ? e->valor : 1;
        if (n > 9) n = 9;
        for (int c = 0; c < n && p < 0.95f; c++) {
            float q = (p - 0.08f - c * 0.07f) / 0.22f;
            if (q <= 0) continue;
            if (q > 1) q = 1;
            int x = ax - 16 + (c % 3) * 16, y0 = ay + dir * (R_DADO + 10 + (c / 3) * 9);
            int y = y0 - (int)((1 - q) * 30) * (dir < 0 ? 1 : -1);
            gfx_rect(x - 5, y - 2, 11, 6, C_SOMBRA);
            gfx_rect(x - 6, y - 3, 11, 6, d->tinta);
            gfx_rect(x - 5, y - 1, 1, 2, d->fundo);              // o furo
            gfx_linha(x - 5, y, x - 9, y - 4, d->luz);           // o barbante
        }
        break;
    }
    case AD_BILHETE: {                                          // o dourado passa por cada dado com bonus e pelo placar
        for (int k = 0; k < N_FILA; k++) {
            if (!(e->mascara >> k & 1)) continue;
            float q = (p - 0.2f - k * 0.08f) / 0.3f;
            if (q <= 0 || q >= 1) continue;
            int rr = R_DADO + 2 + (int)(q * 10);
            for (int i = 0; i < 16; i++) {
                float an = i * 0.3927f + q * 2;
                gfx_rect(SLOT_X(k) + (int)(cosf(an) * rr), zona(j) + (int)(sinf(an) * rr), 2, 2,
                         i % 2 ? d->tinta : C_AMARELO);
            }
        }
        if (e->valor > 0 && p > 0.45f && p < 0.95f) {           // a parte de fora: o placar
            float q = (p - 0.45f) / 0.5f;
            for (int i = 0; i < 10; i++) {
                float an = i * 0.628f + q * 3;
                int rr = 12 + (int)(q * 16);
                gfx_rect(PLACAR_X + (int)(cosf(an) * rr), zona(j) + (int)(sinf(an) * rr * 0.6f), 2, 2, C_AMARELO);
            }
        }
        break;
    }
    case AD_RATOEIRA: {                                         // a mola estala no dado e a barra vai ate o maior dele
        int bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        float q = fminf(1, p / 0.15f);
        float an = 3.1416f * (1 - q);                           // a barra gira por cima do dado
        if (p < 0.5f) gfx_linha_grossa(ax, ay, ax + (int)(cosf(an) * (R_DADO + 4)), ay - (int)(sinf(an) * (R_DADO + 4)), 3, C_PRATA);
        float m = (p - 0.15f) / 0.3f;
        if (m > 0 && p < 0.9f) {                                // a mola esticando ate o alvo
            if (m > 1) m = 1;
            int n = 14;
            for (int i = 0; i < n; i++) {
                float s1 = i / (float)n * m, s2 = (i + 1) / (float)n * m;
                int x1 = ax + (int)((bx - ax) * s1) + (i % 2 ? 3 : -3), y1 = ay + (int)((by - ay) * s1);
                int x2 = ax + (int)((bx - ax) * s2) + (i % 2 ? -3 : 3), y2 = ay + (int)((by - ay) * s2);
                gfx_linha(x1 + 1, y1 + 1, x2 + 1, y2 + 1, C_SOMBRA);
                gfx_linha(x1, y1, x2, y2, d->tinta);
            }
            if (m >= 1) for (int i = 0; i < 12; i++) {          // o impacto
                float a2 = i * 0.5236f;
                int rr = R_DADO + 2 + (int)((p - 0.45f) * 30);
                gfx_rect(bx + (int)(cosf(a2) * rr), by + (int)(sinf(a2) * rr), 2, 2, d->tinta);
            }
        }
        break;
    }
    case AD_SELO: {
        int lx = ax + R_DADO - 4, ly = ay - R_DADO + 4;
        if (rompe) {                                            // o lacre racha e as metades caem
            float q = fminf(1, p / 0.7f);
            for (int h = -1; h <= 1 && p < 0.85f; h += 2)
                lacre_cera(lx + h * (1 + (int)(q * 12)), ly - (int)(sinf(q * 3.1416f) * 12) + (int)(q * q * 22), 4, h);
        } else {                                                // o carimbo desce e lacra: amassa ao bater
            float q = fminf(1, p / 0.3f);
            int bate = p > 0.3f && p < 0.4f;
            lacre_cera(lx, ly - (int)((1 - q * q) * 34), 4 + bate, 0);
            if (bate) for (int i = 0; i < 6; i++) gfx_pixel(lx - 7 + i * 3, ly + 6 - (i % 2), CERA[2]);
        }
        break;
    }
    case AD_CONFETE:                                            // a chuva de papel picado
        for (int i = 0; i < 24 && p < 0.95f; i++) {
            static const uint16_t K[6] = { RGB(228, 194, 106), RGB(96, 134, 196), RGB(226, 164, 168),
                                           RGB(88, 152, 84), RGB(190, 56, 60), RGB(150, 112, 160) };
            float an = i * 0.2618f + 0.4f, v = 50 + (i * 37) % 60, q = fminf(1, p / 0.8f);
            int x = ax + (int)(cosf(an) * v * q), y = ay + (int)(sinf(an) * v * q * 0.7f - 40 * q + 90 * q * q);
            gfx_rect(x, y, (i + (int)(t_fase * 12)) % 2 ? 2 : 3, (i + (int)(t_fase * 12)) % 2 ? 3 : 2, K[i % 6]);
        }
        break;
    case AD_ELETRONICO:                                         // raios em volta do dado
        if (p < 0.75f && (int)(t_fase * 24) % 3) {
            for (int b = 0; b < 4; b++) {
                float an = b * 1.5708f + 0.78f + p * 2;
                int x0 = ax + (int)(cosf(an) * (R_DADO + 2)), y0 = ay + (int)(sinf(an) * (R_DADO + 2));
                int x1 = x0, y1 = y0;
                for (int s = 1; s <= 3; s++) {
                    int x2 = ax + (int)(cosf(an + (s % 2 ? 0.25f : -0.25f)) * (R_DADO + 2 + s * 7));
                    int y2 = ay + (int)(sinf(an + (s % 2 ? 0.25f : -0.25f)) * (R_DADO + 2 + s * 7));
                    gfx_linha_grossa(x1, y1, x2, y2, 2, d->fundo);
                    gfx_linha(x1, y1, x2, y2, C_BRANCO);
                    x1 = x2; y1 = y2;
                }
            }
        }
        break;
    case AD_RASPADINHA:                                         // a raspa prateada saindo
        for (int i = 0; i < 16 && p < 0.7f; i++) {
            float q = fmodf(p * 2.2f + i * 0.13f, 1.0f);
            int x = ax - 8 + (i * 5) % 16 + (int)(q * ((i % 3) - 1) * 14), y = ay - (int)(q * 26) * (dir < 0 ? 1 : -1);
            gfx_rect(x, y, 2, 1, i % 2 ? C_PRATA : C_BRANCO);
        }
        break;
    case AD_FERMENTO:                                           // bolhas subindo e crescendo
        for (int i = 0; i < 8 && p < 0.95f; i++) {
            float q = fmodf(p * 1.5f + i * 0.125f, 1.0f);
            int x = ax - 12 + (i * 7) % 24 + (int)(sinf(q * 6 + i) * 3), y = ay + dir * (int)(q * 40);
            int r = 1 + (int)(q * 4);
            gfx_disco(x, y, r, gfx_mistura(fundo, C_FELTRO, (int)(q * 60)));
            if (r > 2) gfx_pixel(x - 1, y - 1, C_BRANCO);
        }
        break;
    case AD_DOURADO: {                                          // a moeda salta do dado para a sua faixa
        int bx = ficha_px[j] - 46, by = ficha_py[j];
        float q = (p - 0.12f) / 0.55f;
        for (int r = 3; r >= 0 && q > 0; r--) {
            float t = q - r * 0.05f;
            if (t < 0 || t > 1) continue;
            float s = t * t * (3 - 2 * t);
            int x = ax + (int)((bx - ax) * s), y = ay + (int)((by - ay) * s) - (int)(sinf(s * 3.1416f) * 60);
            if (r) { gfx_rect(x - 1, y - 1, 2, 2, C_AMARELO); continue; }
            gfx_disco(x, y, 6, C_LATAO_ESC);
            gfx_disco(x, y, 5, C_OURO);
            gfx_rect(x - 2, y - 3, 2, 2, C_AMARELO);
        }
        if (q > 1 && p < 0.95f)
            for (int i = 0; i < 6; i++) {                       // chegou: brilho
                float an = i * 1.047f;
                int rr = 6 + (int)((q - 1) * 30);
                gfx_rect(bx + (int)(cosf(an) * rr), by + (int)(sinf(an) * rr), 2, 2, C_AMARELO);
            }
        break;
    }
    case AD_LOBO: {                                             // tres garras no placar do rival
        int px = PLACAR_X, py = zona(e->alvo_j);
        for (int g = 0; g < 3; g++) {
            float q = (p - 0.2f - g * 0.08f) / 0.18f;
            if (q <= 0 || p > 0.95f) continue;
            if (q > 1) q = 1;
            int x0 = px - 22 + g * 12, y0 = py - 20, x1 = x0 + (int)(16 * q), y1 = y0 + (int)(38 * q);
            gfx_linha_grossa(x0 + 1, y0 + 1, x1 + 1, y1 + 1, 3, C_SOMBRA);
            gfx_linha_grossa(x0, y0, x1, y1, 3, d->fundo);
            gfx_linha(x0, y0, x1, y1, C_ROSA);
        }
        break;
    }
    case AD_MIRA: {                                             // a mira vai ate o alvo e trava
        int bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        float q = fminf(1, p / 0.4f), s = q * q * (3 - 2 * q);
        int x = ax + (int)((bx - ax) * s), y = ay + (int)((by - ay) * s);
        int r = q < 1 ? 14 : 14 + (int)(fmaxf(0, 0.6f - p) * 20);
        if (p < 0.9f) {
            for (int i = 0; i < 24; i++) {
                float an = i * 0.2618f;
                gfx_rect(x + (int)(cosf(an) * r), y + (int)(sinf(an) * r), 2, 2, d->luz);
            }
            gfx_rect(x - r - 5, y, 8, 2, d->luz); gfx_rect(x + r - 2, y, 8, 2, d->luz);
            gfx_rect(x, y - r - 5, 2, 8, d->luz); gfx_rect(x, y + r - 2, 2, 8, d->luz);
            if (q >= 1 && (int)(t_fase * 16) % 2) gfx_disco(x, y, 3, C_BRANCO);
        }
        break;
    }
    case AD_GORJETA: {                                          // duas fichas do rival para voce
        int r = e->alvo_j;
        for (int i = 0; i < 2; i++) {
            float q = p / 0.55f - i * 0.14f;
            if (q < 0 || q > 1) continue;
            float s = q * q * (3 - 2 * q);
            int x = ficha_px[r] + (int)((ficha_px[j] - ficha_px[r]) * s) + 30;
            int y = ficha_py[r] + (int)((ficha_py[j] - ficha_py[r]) * s) - (int)(sinf(s * 3.1416f) * 40);
            ficha(x, y, d->luz);
        }
        if (p > 0.15f && p < 0.9f) txt_sombra_c(ficha_px[r] + 46, ficha_py[r] - 4, "-2", C_VINHO_CLR, 1);
        if (p > 0.6f) txt_sombra_c(ficha_px[j] + 46, ficha_py[j] - 4 - (int)((p - 0.6f) * 16), "+2", C_OURO, 1);
        break;
    }
    case AD_BIS: {                                              // dois dados fantasmas vao para a sua bolsa
        int bx = GFX_W - 110, by = (j != baixo ? 0 : GFX_H - FAIXA_H) + FAIXA_H / 2;
        float q = 0;
        for (int f = 0; f < BIS_DADOS; f++) {
            q = (p - 0.15f - f * 0.12f) / 0.5f;
            if (q <= 0 || q >= 1) continue;
            float s = q * q * (3 - 2 * q);
            int x = ax + (int)((bx - ax) * s), y = ay + (int)((by - ay) * s) - (int)(sinf(s * 3.1416f) * (40 + f * 14));
            int w = 14 - (int)(s * 6);
            ad_caixa_borda(x - w / 2, y - w / 2, w, w, d->luz);
            gfx_rect(x - 1, y - 1, 2, 2, C_BRANCO);
        }
        if (q >= 1 && p < 0.95f) {
            char t[8];
            snprintf(t, sizeof t, "+%d", BIS_DADOS);
            txt_sombra_c(bx, by - 26 - (int)((p - 0.77f) * 20), t, d->luz, 1);
        }
        break;
    }
    default: break;
    }
}

// Terremoto: a falha corre pelo feltro, do Terremoto ate o dado do
// oponente, em zigue-zague, com a lava acesa dentro e pedrinhas saltando na
// frente; o dado salta dela, e no fim ela se fecha. Por baixo do letreiro.
static void desenha_falha(void)
{
    if (fase != F_EFEITOS || ev_i >= n_ev || t_respiro >= 0) return;
    const evento_t *e = &ev[ev_i];
    float p = t_fase / duracao_evento(ev_i);
    if (e->tipo == EV_TREMOR && e->alvo_k < N_FILA) {
        int ax = SLOT_X(e->de_k), ay = zona(e->de_j), bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        int y0 = ay + (by > ay ? R_DADO + 4 : -R_DADO - 4), y1 = by + (by > ay ? -R_DADO - 2 : R_DADO + 2);
        float q = (p - TREMOR_RACHA) / (TREMOR_SALTA - TREMOR_RACHA);
        float fecha = p > TREMOR_POUSA ? (p - TREMOR_POUSA) / (1 - TREMOR_POUSA) : 0;
        if (q > 0 && fecha < 1) {
            if (q > 1) q = 1;
            enum { N = 14 };
            float px[N + 1], py[N + 1];
            float dx = (float)(bx - ax), dy = (float)(y1 - y0), len = sqrtf(dx * dx + dy * dy);
            float nx = len > 0 ? -dy / len : 0, ny = len > 0 ? dx / len : 0;
            for (int i = 0; i <= N; i++) {
                float t = i / (float)N, off = (i == 0 || i == N) ? 0 : (float)((int)(hash2(i, e->de_k * 7 + e->alvo_k) % 15) - 7) * 1.4f;
                px[i] = ax + dx * t + nx * off;
                py[i] = y0 + dy * t + ny * off;
            }
            float frente = q * N;
            int w = (int)lroundf(4 * (1 - fecha));
            for (int pass = 0; pass < 3; pass++) {           // a beira escura, a lava e o miolo aceso
                int ww = pass == 0 ? w + 2 : pass == 1 ? w : w - 2;
                if (ww < 1) continue;
                uint16_t c = pass == 0 ? C_BARRA : pass == 1 ? C_FOGO : ((int)(t_fase * 14) % 2 ? C_AMARELO : RGB(238, 162, 74));
                for (int i = 0; i < N && i < frente; i++) {
                    float f = frente - i < 1 ? frente - i : 1;
                    gfx_linha_grossa((int)px[i], (int)py[i], (int)(px[i] + (px[i + 1] - px[i]) * f),
                                     (int)(py[i] + (py[i + 1] - py[i]) * f), ww, c);
                }
            }
            for (int i = 3; i < N && i < frente && fecha < 0.5f; i += 4) {   // os galhos, secos
                int s2 = (i / 4) % 2 ? 1 : -1;
                gfx_linha_grossa((int)px[i], (int)py[i], (int)(px[i] + nx * 9 * s2 + dx / len * 4),
                                 (int)(py[i] + ny * 9 * s2 + dy / len * 4), 1, C_BARRA);
            }
            if (q < 1) {                                      // pedrinhas na frente da falha
                int fi = (int)frente < N ? (int)frente : N - 1;
                float fx = px[fi] + (px[fi + 1] - px[fi]) * (frente - fi), fy = py[fi] + (py[fi + 1] - py[fi]) * (frente - fi);
                for (int k2 = 0; k2 < 5; k2++) {
                    float tt = fmodf(t_fase * 4 + k2 * 0.21f, 1.0f);
                    int rx = (int)(fx + ((int)(hash2(k2, 3) % 13) - 6) * tt * 1.6f);
                    int ry = (int)(fy - sinf(tt * 3.1416f) * (8 + k2 * 2));
                    gfx_rect(rx - 1, ry - 1, 3, 3, C_BARRA);
                    gfx_rect(rx, ry, 2, 2, k2 % 2 ? C_CINZA : C_TERRA);
                }
            }
        }
    }
}

// O que o evento em curso desenha por cima da mesa: o traco do ataque, os
// pontos que saem de um placar e entram no outro.
static void desenha_evento_dentro(const evento_t *e, float p)
{
    if (e->tipo == EV_AVISO) return;             // so o texto (e o amuleto acende na faixa)
    if (e->tipo == EV_VAMPIRO) {                 // gotas saem dos dados do oponente e vao ate o Vampiro
        int bx = SLOT_X(e->de_k), by = zona(e->de_j);
        for (int k = 0; k < N_FILA; k++) {
            if (!(e->mascara >> k & 1)) continue;
            float q = (p - 0.15f - k * 0.08f) / 0.5f;
            if (q < 0 || q > 1) continue;
            float s = q * q * (3 - 2 * q);
            int ax = SLOT_X(k), ay = zona(e->alvo_j);
            for (int t = 0; t < 4; t++) {
                float s2 = s - t * 0.05f;
                if (s2 < 0) continue;
                int gx = ax + (int)((bx - ax) * s2), gy = ay + (int)((by - ay) * s2);
                gfx_disco(gx, gy, t < 2 ? 3 : 2, gfx_mistura(C_VINHO_CLR, C_FELTRO, t * 22));
            }
        }
        return;
    }
    if (e->tipo == EV_CHEFE) {                   // pontos direto no placar: o +N sobe nele
        char t[8];
        snprintf(t, sizeof t, "+%d", e->valor);
        txt_sombra_c(PLACAR_X, zona(e->de_j) - 10 - (int)(p * 30), t, e->de_j == 0 ? C_OURO : C_VINHO_CLR, 1);
        return;
    }
    int ax = SLOT_X(e->de_k), ay = zona(e->de_j);
    if (e->tipo == EV_ADESIVO) { desenha_adesivo_agindo(e, p, ax, ay); return; }
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
        } else if (TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_AUTOFAGIA) {
            // Autofagia: a cobra sai do dado, desliza rente a fila ate o mais
            // fraco e se fecha em volta dele, mordendo o rabo.
            float q = p / 0.4f;
            int lado = by < GFX_H / 2 ? 1 : -1;           // pelo lado de fora da mesa
            int yb = by + lado * (R_DADO + 7);
            if (q < 1) {
                for (int i = 9; i >= 0; i--) {
                    float t = q - i * 0.035f;
                    if (t < 0) continue;
                    int x = ax + (int)((bx - ax) * t), y = yb + (int)(sinf(t * 14 - i * 0.6f) * 3);
                    gfx_disco(x, y, i == 0 ? 4 : 3 - i / 4, i == 0 ? C_VERDE : gfx_mistura(C_VERDE, C_FELTRO, i * 6));
                    if (i == 0) gfx_pixel(x + (bx > ax ? 1 : -1), y - 1, C_BARRA);
                }
            } else if (p < 0.9f) {
                float a = fminf(1, (q - 1) / 0.6f) * 6.2832f;   // o anel se fecha em volta do alvo
                int rr = R_DADO + 6;
                for (float s2 = 0; s2 < a; s2 += 0.12f) {
                    float an = 1.5708f * lado + s2;
                    int w = 1 + (int)(s2 / 2.5f);
                    gfx_disco(bx + (int)(cosf(an) * rr), by + (int)(sinf(an) * rr), w, C_VERDE);
                }
                float an = 1.5708f * lado + a;            // a cabeca, na ponta
                int hx = bx + (int)(cosf(an) * rr), hy = by + (int)(sinf(an) * rr);
                gfx_disco(hx, hy, 4, C_VERDE);
                gfx_pixel(hx, hy - 1, C_BARRA);
            }
        } else if (TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_BONECO) {
            // Boneco de Neve: um sopro de flocos sai dele em arco ate o 4o do
            // oponente, que congela - o gelo se espalha e prende o dado.
            for (int i = 0; i < 9; i++) {
                float q = (p - i * 0.03f) / 0.42f;
                if (q < 0 || q > 1) continue;
                float sq = q * q * (3 - 2 * q);
                int x = ax + (int)((bx - ax) * sq + sinf(q * 9 + i * 1.7f) * 7);
                int y = ay - R_DADO + (int)((by - (ay - R_DADO)) * sq - sinf(sq * 3.1416f) * 40 + cosf(q * 7 + i) * 4);
                floco(x, y, i % 2 ? C_BRANCO : C_VENTO);
            }
            if (p > 0.35f) gelo_sobre(bx, by, (p - 0.35f) / 0.4f);
        } else if (TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_TSUNAMI) {
            // Tsunami: a crista sai dele, atravessa a mesa e quebra em cima do
            // dado do oponente, que some na espuma.
            float q = p / 0.5f;
            int dir = by > ay ? 1 : -1;
            if (q < 1) {                                     // tres cristas rolando, a da frente maior
                float sq = q * q * (3 - 2 * q);
                for (int c2 = 2; c2 >= 0; c2--) {
                    float s2 = sq - c2 * 0.13f;
                    if (s2 < 0) continue;
                    int x = ax + (int)((bx - ax) * s2), y = ay + (int)((by - ay) * s2);
                    crista(x, y, dir, 4 + (int)(q * 4) - c2);
                }
            } else if (p < 0.95f) {
                float t = (p - 0.5f) / 0.45f;                // a espuma espirrando em volta do alvo
                for (int i = 0; i < 12; i++) {
                    float an = i * 0.5236f + 0.2f, rr = R_DADO * (0.5f + t * 1.4f);
                    int x = bx + (int)(cosf(an) * rr), y = by + (int)(sinf(an) * rr * 0.8f) + (int)(t * t * 10);
                    gfx_rect(x - 1, y - 1, 3, 3, gfx_mistura(i % 3 ? C_BRANCO : RGB(96, 182, 198), C_FELTRO, (int)(t * 75)));
                }
                if (t < 0.4f) crista(bx, by, dir, 10);
            }
        } else if (TIPO[F[e->de_j].tipo[e->de_k]].efeito == EF_OURICO) {
            // Ourico: um leque de espinhos voa dele ate o placar do rival.
            for (int i = 0; i < 5; i++) {
                float q = p / 0.5f - i * 0.06f;
                if (q < 0 || q > 1) continue;
                float x = ax + (bx - ax) * q + (i - 2) * 6 * sinf(q * 3.1416f);
                float y = ay + (by - ay) * q - sinf(q * 3.1416f) * (30 + i * 4);
                float an = atan2f((float)(by - ay), (float)(bx - ax));
                float vx[3] = { x + cosf(an) * 6, x + cosf(an + 2.6f) * 3, x + cosf(an - 2.6f) * 3 };
                float vy[3] = { y + sinf(an) * 6, y + sinf(an + 2.6f) * 3, y + sinf(an - 2.6f) * 3 };
                poligono(vx, vy, 3, i % 2 ? C_TERRA : C_MADEIRA_CLR);
            }
        } else {
            projetil(ax, ay, bx, by, p / 0.42f, c);
        }
    }
    // A Onda junta 9: a agua sobe e cobre o dado; quando baixa, ele e um
    // Tsunami.
    if (e->tipo == EV_ONDA) {
        float h = p < 0.48f ? p / 0.48f : p < 0.85f ? 1 - (p - 0.48f) / 0.37f : 0;
        h = h * h * (3 - 2 * h);
        int base = ay + R_DADO + 6, topo = base - (int)(h * (2 * R_DADO + 22));
        if (h > 0) {
            for (int xx = ax - R_DADO - 10; xx <= ax + R_DADO + 10; xx++) {
                int ty = topo + (int)(sinf(xx * 0.35f + t_fase * 14) * 2);
                for (int yy = ty; yy < base; yy++)
                    gfx_pixel(xx, yy, ((yy - ty) / 4) % 3 == 1 ? RGB(96, 182, 198) : C_AZUL);
                gfx_pixel(xx, ty, C_BRANCO);
                if ((xx + (int)(t_fase * 40)) % 5 == 0) gfx_rect(xx, ty - 2, 2, 2, C_BRANCO);
            }
        }
        if (p > 0.5f && p < 0.97f) {
            int ty = zona(e->de_j) < CENTRO ? ay + R_DADO + 12 : ay - R_DADO - 34;
            int w = gfx_largura("TSUNAMI!", 1) + 12;
            gfx_rect(ax - w / 2, ty - 2, w, TXT_H + 4, C_BARRA);
            txt_c(ax, ty, "TSUNAMI!", RGB(96, 182, 198), true);
            for (int i = 0; i < 8; i++) {                    // gotas que sobram da onda
                float q = fmodf((p - 0.5f) * 2.2f + i * 0.125f, 1.0f);
                int gx = ax - R_DADO + (i * 11) % (2 * R_DADO), gy = ay - R_DADO - 6 + (int)(q * q * 30);
                gfx_rect(gx, gy, 2, 3, gfx_mistura(RGB(96, 182, 198), C_FELTRO, (int)(q * 70)));
            }
        }
    }
    // Treno: do treno saem os presentes, um para cada dado valendo antes dele,
    // do primeiro (+1) ao ultimo; cada um cai em arco em cima do dado, que acende.
    if (treno_agora(e)) {
        static const uint16_t COR[3] = { C_VERMELHO, C_VERDE, C_AZUL };
        int n = 0;
        for (int k = 0; k < N_FILA; k++) {
            if (!(e->mascara >> k & 1)) continue;
            float q = (p - treno_sai(n)) / TRENO_VOO;
            int bx = SLOT_X(k), by = ay - R_DADO - 8;
            if (q >= 0 && q < 2.2f) {
                float sq = q > 1 ? 1 : q * q * (3 - 2 * q);
                int x = ax + (int)((bx - ax) * sq), y = ay - R_DADO + (int)((by - (ay - R_DADO)) * sq) - (int)(sinf(sq * 3.1416f) * 30);
                if (q > 1) y -= (int)(sinf((q - 1) * 6) * 2);     // assenta com um pulinho
                presente(x, y, COR[n % 3]);
                if (q < 1)                                        // o rastro de neve
                    for (int r = 1; r <= 3; r++) {
                        float s2 = sq - r * 0.06f;
                        if (s2 < 0) continue;
                        int rx = ax + (int)((bx - ax) * s2), ry = ay - R_DADO + (int)((by - (ay - R_DADO)) * s2) - (int)(sinf(s2 * 3.1416f) * 30);
                        gfx_rect(rx, ry, 2, 2, gfx_mistura(C_BRANCO, C_FELTRO, r * 25));
                    }
            }
            n++;
        }
    }
    // Rena: o nariz acende e uma luz vermelha corre ate cada vizinho valendo
    // (o nariz guia o caminho); o vizinho acende quando ela chega.
    if (e->tipo == EV_BONUS && e->de_k < N_FILA && F[e->de_j].tipo[e->de_k] == D_RENA) {
        int nx = ax, ny = ay + (int)(R_DADO * 0.34f);
        float pulso = 0.5f + 0.5f * sinf(t_fase * 16);
        if (p < 0.9f) {
            int hr = 4 + (int)(pulso * 3);
            for (int a = 0; a < 16; a++)
                gfx_pixel(nx + (int)(cosf(a * 0.3927f) * hr), ny + (int)(sinf(a * 0.3927f) * hr), gfx_mistura(C_VERMELHO, C_ROSA, (int)(pulso * 60)));
        }
        for (int k = 0; k < N_FILA; k++) {
            if (!(e->mascara >> k & 1)) continue;
            float q = (p - 0.18f) / 0.42f;
            if (q < 0 || q >= 1.35f) continue;
            float s = q > 1 ? 1 : q * q * (3 - 2 * q);
            int bx = SLOT_X(k), by = ay - R_DADO - 2;
            for (int r = 3; r >= 0; r--) {                  // a luz e o rastro
                float s2 = s - r * 0.07f;
                if (s2 < 0 || (q > 1 && r)) continue;
                int lx = nx + (int)((bx - nx) * s2), ly = ny + (int)((by - ny) * s2) - (int)(sinf(s2 * 3.1416f) * 22);
                gfx_disco(lx, ly, r ? 2 : 3, gfx_mistura(C_VERMELHO, C_FELTRO, r * 22));
                if (!r) gfx_pixel(lx - 1, ly - 1, C_ROSA);
            }
        }
    }
    // Tres Patinhos: na vez que conta, os tres saltam do dado, um atras do
    // outro, e ficam em fila em cima dele.
    if (e->tipo == EV_MULT)
        for (int k = 0; k < N_FILA; k++) {
            if (!(e->mascara >> k & 1) || F[e->de_j].tipo[k] != D_PATINHOS || !patinhos_ativo(F[e->de_j].fixo[k])) continue;
            int bx = SLOT_X(k), by = e->de_j == baixo ? zona(e->de_j) - R_DADO - 16 : zona(e->de_j) + R_DADO + 20;
            for (int i = 0; i < 3; i++) {
                float q = (p - 0.10f - i * 0.12f) / 0.22f;
                if (q < 0) continue;
                int hop = q < 1 ? (int)(sinf(q * 3.1416f) * 12) : (int)(fabsf(sinf((p + i * 0.3f) * 14)) * 2);
                patinho(bx - 16 + i * 15, by - hop);
            }
        }
    // Sabotador: uma serra gira da mao dele ate o maior do rival e o corta
    // na diagonal; dali em diante ele vale metade.
    if (e->tipo == EV_CORTA) {
        int bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        int sx = e->de_k < N_FILA ? ax : 60, sy = e->de_k < N_FILA ? ay : zona(e->de_j);   // o Ranzinza: da mao dele
        float q = p / 0.4f;
        if (q < 1) {
            float s = q * q * (3 - 2 * q);
            int x = sx + (int)((bx - sx) * s), y = sy + (int)((by - sy) * s) - (int)(sinf(s * 3.1416f) * 30);
            gfx_disco(x, y, 6, C_PRATA);
            for (int i = 0; i < 8; i++) {                // os dentes, girando
                float an = t_fase * 18 + i * 0.785f;
                gfx_rect(x + (int)(cosf(an) * 7) - 1, y + (int)(sinf(an) * 7) - 1, 2, 2, C_FERRO);
            }
            gfx_disco(x, y, 2, C_FERRO);
        } else if (p < 0.85f) {
            float c2 = (p - 0.4f) / 0.25f;               // o corte desce na diagonal
            if (c2 > 1) c2 = 1;
            int x0 = bx - R_DADO, y0 = by - R_DADO;
            gfx_linha_grossa(x0, y0, x0 + (int)(2 * R_DADO * c2), y0 + (int)(2 * R_DADO * c2), 2, C_VINHO_CLR);
            for (int i = 0; i < 6; i++)                 // lascas
                gfx_rect(x0 + (int)(2 * R_DADO * c2) + (i % 3) * 3 - 3, y0 + (int)(2 * R_DADO * c2) - (i / 3) * 4,
                         2, 2, i % 2 ? C_PRATA : C_MARFIM_S);
        }
    }
    // Medusa: o olhar dela sai como uma onda cinza e passa pela fila; cada
    // dado que ela alcanca vira pedra (racha e acinzenta a borda).
    if (e->tipo == EV_MEDUSA) {
        float q = p / 0.55f;
        if (q < 1) {
            int rr = (int)(q * 150);
            for (int a = 0; a < 40; a++) {
                float an = a * 0.15708f;
                gfx_rect(ax + (int)(cosf(an) * rr) - 1, ay + (int)(sinf(an) * rr * 0.4f) - 1, 3, 2,
                         gfx_mistura(C_CINZA, C_FELTRO, (int)(q * 70)));
            }
        }
    }
    // Agiota: as fichas do rival saem da faixa dele e pousam na sua.
    // Garimpeiro: de cada outro dado que vale sai uma pepita que volta, em arco,
    // para a bolsa de quem joga; a bolsa acende quando elas chegam.
    if (e->tipo == EV_GARIMPO) {
        int j = e->de_j, bx = GFX_W - 110, by = (j != baixo ? 0 : GFX_H - FAIXA_H) + FAIXA_H / 2;
        int n = 0;
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            if (!(e->mascara >> k & 1)) continue;
            float q0 = (p - 0.08f - n++ * 0.09f) / 0.45f;
            for (int r = 3; r >= 0; r--) {                   // a pepita e um rastro que se apaga
                float q = q0 - r * 0.05f;
                if (q < 0 || q >= 1) continue;
                float s = q * q * (3 - 2 * q);
                int sx = SLOT_X(k), sy = zona(j), m = r ? 3 - r / 2 : 5;
                int x = sx + (int)((bx - sx) * s), y = sy + (int)((by - sy) * s) - (int)(sinf(s * 3.1416f) * 36);
                float vx[4] = { (float)(x - m), (float)x, (float)(x + m), (float)x };
                float vy[4] = { (float)y, (float)(y - m * 3 / 4), (float)y, (float)(y + m * 3 / 4) };
                poligono(vx, vy, 4, r ? gfx_mistura(C_OURO, C_FELTRO, 25 * r) : C_OURO);
                if (!r) gfx_rect(x - 1, y - 1, 2, 1, C_AMARELO);
            }
        }
        if (p > 0.5f) {                                      // a bolsa recebe
            float q = (p - 0.5f) / 0.5f;
            int rr = 10 + (int)(q * 10);
            for (int a = 0; a < 28; a++) {
                float an = a * 0.2244f;
                gfx_rect(bx + (int)(cosf(an) * rr), by + (int)(sinf(an) * rr), 1, 1, gfx_mistura(C_OURO, C_BARRA, (int)(q * 85)));
            }
        }
    }
    if (e->tipo == EV_AGIOTA) {
        int r = e->alvo_j, d = e->de_j;
        for (int i = 0; i < 2; i++) {
            float q = p / 0.55f - i * 0.14f;
            if (q < 0 || q > 1) continue;
            float s = q * q * (3 - 2 * q);
            int x = ficha_px[r] + (int)((ficha_px[d] - ficha_px[r]) * s) + 30;
            int y = ficha_py[r] + (int)((ficha_py[d] - ficha_py[r]) * s) - (int)(sinf(s * 3.1416f) * 40);
            ficha(x, y, C_OURO);
        }
        if (p > 0.15f && p < 0.9f) txt_sombra_c(ficha_px[r] + 46, ficha_py[r] - 4, "-2", C_VINHO_CLR, 1);
        if (p > 0.6f) txt_sombra_c(ficha_px[d] + 46, ficha_py[d] - 4 - (int)((p - 0.6f) * 16), "+2", C_OURO, 1);
    }
    // Cavalo de Troia: atingido, se abre onde estiver (no lugar dele ou, se foi
    // roubado, entre os roubados do outro) e de dentro sai um soldado para
    // cada dado de quem o atingiu; os dados piscam e caem.
    if (e->tipo == EV_CAVALO) {
        int ox = ax, oy = ay, eu = -1;
        if (vis_est[e->de_j][e->de_k] == V_ROUBADO)
            for (int i = vis_nroubo[e->alvo_j] - 1; i >= 0 && eu < 0; i--)
                if (vis_roubo[e->alvo_j][i].tipo == D_CAVALO) { eu = i; ox = ROUBO_X(i); oy = zona(e->alvo_j); }
        // As tabuas se soltam: lascas de madeira saltando do cavalo.
        float q = (p - 0.1f) / 0.35f;
        if (q > 0 && q < 1)
            for (int i = 0; i < 10; i++) {
                float an = i * 0.6283f + 0.3f;
                int r0 = 14 + (int)(q * 26), r1 = r0 + 7;
                gfx_linha_grossa(ox + (int)(cosf(an) * r0), oy + (int)(sinf(an) * r0),
                                 ox + (int)(cosf(an) * r1), oy + (int)(sinf(an) * r1), 3,
                                 gfx_mistura(i % 2 ? C_TERRA : C_MADEIRA_CLR, C_FELTRO, (int)(q * 70)));
            }
        // E de dentro sai um soldado para cada dado de quem o atingiu.
        int n = 0, by = zona(e->alvo_j);
        for (int k = 0; k < N_FILA; k++)
            if (e->mascara >> k & 1) projetil(ox, oy, SLOT_X(k), by, (p - 0.22f - 0.07f * n++) / 0.4f, C_TERRA);
        for (int i = 0; i < vis_nroubo[e->alvo_j]; i++)         // e nos que ele tinha roubado
            if (i != eu) projetil(ox, oy, ROUBO_X(i), by, (p - 0.22f - 0.07f * n++) / 0.4f, C_TERRA);
    }
    // Terremoto: a falha corre por baixo do letreiro (desenha_falha); aqui,
    // o dado arranca do chao com estilhacos em volta.
    if (e->tipo == EV_TREMOR && e->alvo_k < N_FILA) {
        int bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        float sq = (p - TREMOR_SALTA) / 0.16f;                // o dado arranca do chao: estilhacos em volta
        if (sq > 0 && sq < 1)
            for (int k2 = 0; k2 < 10; k2++) {
                float an = k2 * 0.628f + 0.3f, rr = R_DADO * 0.6f + sq * 26;
                int rx = bx + (int)(cosf(an) * rr), ry = by + (int)(sinf(an) * rr * 0.6f) - (int)(sinf(sq * 3.1416f) * 10);
                gfx_rect(rx - 1, ry - 1, 3, 3, C_BARRA);
                gfx_rect(rx, ry, 2, 2, k2 % 3 ? C_TERRA : C_FOGO);
            }
    }
    // Ficha Coringa: os naipes saem do dado do oponente que ela copia e
    // chegam nela em arco; ela brilha, sobe girando e cai virada (desenha_fila).
    if (e->tipo == EV_CORINGA && e->alvo_k < N_FILA) {
        static const char *const NAIPE[4][5] = {
            { "..#..", ".###.", "#####", "..#..", ".###." },  // espadas
            { ".#.#.", "#####", "#####", ".###.", "..#.." },  // copas
            { "..#..", ".###.", "#####", ".###.", "..#.." },  // ouros
            { "..#..", ".###.", "#.#.#", "#####", "..#.." } };  // paus
        int bx = SLOT_X(e->alvo_k), by = zona(e->alvo_j);
        if (p < 0.34f) aro(bx, by, ((int)(t_fase * 12) % 2) ? C_OURO : C_REAL);   // o que ela vai copiar acende
        for (int i = 0; i < 4; i++) {
            float q = (p - i * 0.05f) / 0.24f;
            if (q < 0 || q > 1) continue;
            float s2 = q * q * (3 - 2 * q), arco = -46.0f - i * 9;
            int x = bx + (int)((ax - bx) * s2 + sinf(s2 * 6.28f + i) * 6 * (1 - s2));
            int y = by + (int)((ay - by) * s2 + arco * 4 * s2 * (1 - s2));
            sprite_em(x, y, NAIPE[i], 5, i % 2 ? C_VERMELHO : C_MARFIM, C_BRANCO, C_EBANO, false, q > 0.85f ? (q - 0.85f) / 0.15f : 0);
        }
        float c = (p - 0.26f) / 0.14f;                       // chegaram: um anel de brilho na ficha
        if (c > 0 && c < 1)
            for (int a2 = 0; a2 < 24; a2++) {
                float an = a2 * 0.2618f, rr = R_DADO + 2 + c * 14;
                if (bayer(a2 * 3, (int)(c * 9)) < c) continue;
                gfx_rect(ax + (int)(cosf(an) * rr), ay + (int)(sinf(an) * rr), 2, 2, a2 % 2 ? C_OURO : C_AMARELO);
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
        if (p > 0.35f && !(e->bloqueado >> e->alvo_k & 1))
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
            // Um facho de luz de cima, so nele: o feltro sobe na rampa.
            float q = p < 0.3f ? p / 0.3f : (p > 0.8f ? (1 - p) / 0.2f : 1);
            int topo = ay - 70, base = ay + R_DADO;
            for (int yy = topo; yy < base; yy += 2) {
                int meia = 6 + (yy - topo) * 22 / (base - topo);
                ui_clareia(ax - meia, yy, 2 * meia, 2, 1.6f * q);
            }
        } else if (t == D_CAPITAO && e->mascara) {
            // Um galao dourado sai dele em arco e vai pousar em cada dado
            // seu que vale; o dele sobe primeiro.
            for (int k = 0; k < N_FILA; k++) {
                if (!(e->mascara >> k & 1)) continue;
                float q = (p - capitao_sai(e, k)) / CAPITAO_VOO;
                if (q < 0 || q >= 1) continue;
                float s = q * q * (3 - 2 * q);
                int bx = SLOT_X(k), by = ay - R_DADO - 7;
                int gx = ax + (int)((bx - ax) * s);
                int gy = ay - R_DADO + (int)((by - (ay - R_DADO)) * s) - (int)(sinf(s * 3.1416f) * 26);
                galao(gx, gy);
            }
        } else if (t == D_CHECKPOINT) {
            // A bandeira quadriculada sobe do dado e tremula: daqui em diante, +3.
            float q = p / 0.3f;
            if (q > 1) q = 1;
            int topo = ay - R_DADO - 4 - (int)(q * 38);
            gfx_linha_grossa(ax, ay - R_DADO - 4, ax, topo, 2, C_PRATA);
            int w = (int)(q * 24);
            for (int cy2 = 0; cy2 < 3; cy2++)
                for (int cx2 = 0; cx2 * 4 < w; cx2++) {
                    int onda = (int)(sinf(t_fase * 10 + cx2 * 0.9f) * 2);
                    gfx_rect(ax + 1 + cx2 * 4, topo + cy2 * 4 + onda, 4, 4, (cx2 + cy2) & 1 ? C_BARRA : C_BRANCO);
                }
            gfx_disco(ax, topo, 2, C_PRATA);
        } else if (t == D_DOMINO && e->mascara) {
            // Domino: uma pedra sobe dele e tomba em cima do seguinte.
            float q = p / 0.45f;
            if (q < 1) {
                int bx = SLOT_X(e->de_k + 1);
                float s = q * q * (3 - 2 * q), an = s * 1.5708f;
                float x = ax + (bx - ax) * s, y = ay - R_DADO - 12 - sinf(s * 3.1416f) * 24;
                float hw = 5, hh = 9, cs = cosf(an), sn = sinf(an);
                float vx[4], vy[4];
                static const float CX[4] = { -1, 1, 1, -1 }, CY[4] = { -1, -1, 1, 1 };
                for (int i = 0; i < 4; i++) {
                    vx[i] = x + CX[i] * hw * cs - CY[i] * hh * sn;
                    vy[i] = y + CX[i] * hw * sn + CY[i] * hh * cs;
                }
                poligono(vx, vy, 4, C_BARRA);
                for (int i = 0; i < 4; i++) { vx[i] = x + (vx[i] - x) * 0.8f; vy[i] = y + (vy[i] - y) * 0.8f; }
                poligono(vx, vy, 4, C_MARFIM);
                gfx_linha_grossa((int)(x - 4 * cs), (int)(y - 4 * sn), (int)(x + 4 * cs), (int)(y + 4 * sn), 1, C_BARRA);
            }
        } else if (t == D_FICHA_VELHA) {
            // Ficha Velha: a rachadura acende e ela paga - seis moedas saem
            // dela, uma atras da outra, e voam em arco ate o placar.
            static const float CR[5][2] = { { 0.50f, -0.42f }, { 0.36f, -0.24f }, { 0.47f, -0.08f }, { 0.37f, 0.10f }, { 0.44f, 0.24f } };
            if (p < 0.55f) {
                uint16_t cr = ((int)(p * 14) % 2) ? C_AMARELO : C_OURO;
                for (int i = 0; i < 4; i++)
                    gfx_linha_grossa(ax + (int)(CR[i][0] * R_DADO), ay + (int)(CR[i][1] * R_DADO),
                                     ax + (int)(CR[i + 1][0] * R_DADO), ay + (int)(CR[i + 1][1] * R_DADO), 1, cr);
            }
            int px = PLACAR_X, py = zona(e->de_j);
            for (int i = 0; i < FICHA_VELHA_PTS; i++) {
                float q = (p - 0.22f - i * 0.06f) / 0.32f;
                if (q < 0 || q >= 1) continue;
                float sq = q * q * (3 - 2 * q);
                int mx = ax + (int)((px - ax) * sq), my = ay - (int)(sinf(sq * 3.1416f) * (30 + i * 3)) + (int)((py - ay) * sq);
                gfx_disco(mx, my, 4, C_LATAO_ESC);
                gfx_disco(mx, my - 1, 3, C_OURO);
                gfx_rect(mx - 1, my - 3, 2, 1, C_AMARELO);
            }
        } else if (t == D_MEIA && e->valor > 0) {
            // Meia na Lareira pagando: as moedas saltam da boca da meia, uma
            // atras da outra, e voam em arco ate o placar.
            int px = PLACAR_X, py = zona(e->de_j), n = e->valor < 10 ? (e->valor + 1) / 2 : 5;
            for (int i = 0; i < n; i++) {
                float q = (p - 0.08f - i * 0.07f) / 0.40f;
                if (q < 0 || q >= 1) continue;
                float sq = q * q * (3 - 2 * q);
                moedinha(ax + (int)((px - ax) * sq), ay - R_DADO / 2 + (int)((py - ay) * sq) - (int)(sinf(sq * 3.1416f) * (34 + i * 4)));
            }
        } else if (t == D_MEIA) {
            // Meia anulada: uma moeda cai do alto dentro dela e some, com um brilho.
            float q = p / 0.45f;
            if (q < 1) moedinha(ax - 2, ay - R_DADO - 34 + (int)(q * q * 30));
            else if (p < 0.85f) estrelinha(ax - 2, ay - R_DADO / 2, (int)(2 + (p - 0.45f) * 8), C_AMARELO);
        } else if (t == D_ESTRELA && e->mascara) {
            // Estrela de Belem: a estrela sobe do dado e brilha; dela desce um
            // facho de luz em cada dado valendo abaixo, do mais perto ao mais longe.
            int sx = ax, sy = ay - R_DADO - 14 - (int)(suave(0, 0.15f, p) * 6);
            float pulso = 0.5f + 0.5f * sinf(t_fase * 14);
            estrelinha(sx, sy, 5 + (int)(pulso * 2), C_AMARELO);
            estrelinha(sx, sy, 2, C_BRANCO);
            int n = 0;
            for (int k = N_FILA - 1; k >= 0; k--) {
                if (!(e->mascara >> k & 1)) continue;
                float q = (p - 0.15f - n++ * 0.12f) / 0.22f;
                if (q < 0 || p > 0.92f) continue;
                float s = q > 1 ? 1 : q * q * (3 - 2 * q);
                int bx = SLOT_X(k), by = ay - R_DADO + 2;
                int fx = sx + (int)((bx - sx) * s), fy = sy + (int)((by - sy) * s);
                gfx_linha_grossa(sx, sy, fx, fy, 1, gfx_mistura(C_AMARELO, C_FELTRO, q > 1 ? (int)((q - 1) * 30) : 0));
                if (q >= 1) estrelinha(bx, by - 3, 3 + (int)(pulso * 2), C_AMARELO);
            }
        } else if (t == D_JINGLE && e->mascara) {
            // Jingle Bells: os sinos batem um no outro no ar (desenha_fila os
            // move); em cada batida, um estalo de luz onde encostam, e
            // notinhas sobem deles.
            float b = sinos_juntos(p);
            int n = 0, xs[N_FILA], ys[N_FILA];
            for (int k = 0; k < N_FILA; k++)
                if (e->mascara >> k & 1) { sino_pos(e, p, k, &xs[n], &ys[n]); n++; }
            if (b > 0.80f)
                for (int i = 0; i + 1 < n; i++) {
                    int cx = (xs[i] + xs[i + 1]) / 2, cy = ys[i] - 4, m = (int)((b - 0.80f) / 0.20f * 9) + 3;
                    for (int a = 0; a < 8; a++) {
                        float an = a * 0.7854f;
                        gfx_linha(cx + (int)(cosf(an) * 3), cy + (int)(sinf(an) * 3),
                                  cx + (int)(cosf(an) * m), cy + (int)(sinf(an) * m), a % 2 ? C_AMARELO : C_BRANCO);
                    }
                }
            for (int i = 0; i < n * 2 && p > 0.12f; i++) {
                float q = fmodf(p * 1.4f + i * 0.23f, 1.0f);
                if (p > 0.86f && q > 0.5f) continue;
                int nx = xs[i / 2] + (i % 2 ? 14 : -16) + (int)(sinf(q * 6 + i) * 3);
                int ny = ys[i / 2] - 4 - (int)(q * 12);
                notinha(nx, ny, gfx_mistura(i % 3 ? C_OURO : C_AMARELO, C_FELTRO, (int)(q * 70)));
            }
        } else if (t == D_MARTIR && e->mascara) {
            // Martir: a aureola sobe do dado caido e se reparte em luzes, uma
            // para cada dado seu que vale.
            float q = p / 0.25f;
            int hy = ay - R_DADO - 6 - (int)((q < 1 ? q : 1) * 16);
            if (p < 0.4f) {
                for (int a = 0; a < 24; a++) {
                    float an = a * 0.2618f;
                    gfx_rect(ax + (int)(cosf(an) * 10) - 1, hy + (int)(sinf(an) * 3) - 1, 2, 2, C_OURO);
                }
            }
            for (int k = 0; k < N_FILA; k++) {
                if (!(e->mascara >> k & 1)) continue;
                float s = (p - 0.3f - k * 0.06f) / 0.3f;
                if (s < 0 || s > 1) continue;
                int lx = ax + (int)((SLOT_X(k) - ax) * s), ly = hy + (int)((ay - R_DADO - hy) * s) - (int)(sinf(s * 3.1416f) * 14);
                gfx_disco(lx, ly, 4, C_HALO);
                gfx_disco(lx, ly, 2, C_OURO);
            }
        } else if (t == D_PUXA_FILA) {
            // Puxa-fila: um rastro dourado corre a fila inteira, da frente ao fim.
            float q = p / 0.5f;
            if (q < 1) {
                int x0 = SLOT_X(0), x1 = SLOT_X(F[e->de_j].n - 1), y = ay + (ay < CENTRO ? R_DADO + 8 : -R_DADO - 8);
                int x = x0 + (int)((x1 - x0 + 30) * q) - 15;
                gfx_linha_grossa(x0 - 15, y, x, y, 2, gfx_mistura(C_OURO, C_FELTRO, 40));
                for (int i = 0; i < 3; i++) {
                    int cx = x - i * 7;
                    gfx_linha_grossa(cx - 4, y - 4, cx, y, 2, C_OURO);
                    gfx_linha_grossa(cx - 4, y + 4, cx, y, 2, C_OURO);
                }
            }
        } else if (t == D_COVEIRO) {
            // Coveiro: de cada dado anulado na mesa sobe uma cruzinha que vem
            // para a mao dele.
            int n = 0;
            for (int q2 = 0; q2 < 2; q2++)
                for (int i = 0; i < F[q2].lancados; i++) {
                    if (vis_est[q2][i] != V_ANULADO) continue;
                    float s = (p - n++ * 0.07f) / 0.5f;
                    if (s < 0 || s > 1) continue;
                    int sx = SLOT_X(i), sy = zona(q2);
                    int x = sx + (int)((ax - sx) * s), y = sy + (int)((ay - sy) * s) - (int)(sinf(s * 3.1416f) * 30);
                    gfx_rect(x - 1, y - 5, 3, 10, C_CINZA);
                    gfx_rect(x - 4, y - 2, 9, 3, C_CINZA);
                }
        } else if (t == D_HEXA && e->mascara) {
            // Hexa: um hexagono se acende em volta de cada dado de seis lados.
            int n = 0;
            for (int k = 0; k < N_FILA; k++) {
                if (!(e->mascara >> k & 1)) continue;
                float s = (p - 0.08f - n++ * 0.12f) / 0.3f;
                if (s < 0) continue;
                float rr = R_DADO + 4 + (s < 1 ? (1 - s) * 8 : 0);
                float vx[6], vy[6];
                for (int i = 0; i < 6; i++) { vx[i] = SLOT_X(k) + cosf(i * 1.0472f) * rr; vy[i] = ay + sinf(i * 1.0472f) * rr; }
                for (int i = 0; i < 6; i++)
                    gfx_linha_grossa((int)vx[i], (int)vy[i], (int)vx[(i + 1) % 6], (int)vy[(i + 1) % 6], 2,
                                     s < 1 ? C_FOGO : gfx_mistura(C_FOGO, C_FELTRO, 40));
            }
        } else if (t == D_AMPULHETA) {
            // Ampulheta: ela sobe, vira e a areia cai.
            float q = p / 0.35f;
            if (q > 1) q = 1;
            int cx = ax, cy = ay - R_DADO - 26;
            float an = q * 3.1416f, cs = cosf(an), sn = sinf(an);
            // o vidro (dois triangulos), as tampas e a areia: embaixo antes de
            // virar, caindo depois
            static const float HX[6] = { -9, 9, 0, 9, -9, 0 }, HY[6] = { -13, -13, 0, 13, 13, 0 };
            float vx[4], vy[4];
            for (int m = 0; m < 2; m++) {
                for (int i = 0; i < 3; i++) {
                    float x = HX[m * 3 + i], y = HY[m * 3 + i];
                    vx[i] = cx + x * cs - y * sn; vy[i] = cy + x * sn + y * cs;
                }
                poligono(vx, vy, 3, C_MARFIM_S);
                for (int i = 0; i < 3; i++) { vx[i] = cx + (vx[i] - cx) * 0.7f; vy[i] = cy + (vy[i] - cy) * 0.7f; }
                if ((m == 1) == (q < 1)) poligono(vx, vy, 3, C_AMARELO);       // a areia
            }
            for (int m = -1; m <= 1; m += 2) {                                  // as tampas
                static const float TX[4] = { -11, 11, 11, -11 }, TY[4] = { -2, -2, 1, 1 };
                for (int i = 0; i < 4; i++) {
                    float x = TX[i], y = TY[i] + m * 14;
                    vx[i] = cx + x * cs - y * sn; vy[i] = cy + x * sn + y * cs;
                }
                poligono(vx, vy, 4, C_MADEIRA_CLR);
            }
            if (q >= 1 && p < 0.9f)
                for (int i = 0; i < 4; i++) {
                    int yy = cy + (int)(fmodf(t_fase * 30 + i * 3, 11));
                    gfx_rect(cx, yy, 1, 2, C_AMARELO);
                }
        } else if (t == D_RANCOR) {
            // Rancor: uma camada escura para cada rodada perdida desce do alto e
            // se assenta no dado, uma sobre a outra.
            int n = e->valor < 8 ? e->valor : 8;
            for (int i = 0; i < n; i++) {
                float q = (p - i * 0.07f) / 0.28f;
                if (q < 0 || q >= 1) continue;
                float s = q * q;                                   // cai pesado
                int cy = ay - R_DADO - 44 + (int)(s * 40), w = 7 - (q > 0.85f ? 2 : 0);
                float vx[3] = { (float)(ax - w), (float)(ax + w), (float)ax };
                float vy[3] = { (float)(cy - 4), (float)(cy - 4), (float)(cy + 5) };
                poligono(vx, vy, 3, i % 2 ? C_VINHO : C_BRASA);
            }
            if (n && p > 0.2f && p < 0.2f + n * 0.07f + 0.3f) aro(ax, ay, C_VINHO);
        } else if (t == D_POTE_QUENTE) {
            // Pote Quente: losangos de calor se abrem do dado, um atras do
            // outro, e brasas sobem tremendo.
            for (int o = 0; o < 3; o++) {
                float q = (p - o * 0.14f) / 0.5f;
                if (q < 0 || q > 1) continue;
                int rr = R_DADO + 2 + (int)(q * 22);
                uint16_t cq = gfx_mistura(o == 0 ? C_FOGO : C_AMARELO, C_FELTRO, (int)(q * 80));
                for (int i = 0; i < 4; i++) {
                    float a0 = i * 1.5708f, a1 = a0 + 1.5708f;
                    gfx_linha_grossa(ax + (int)(cosf(a0) * rr), ay + (int)(sinf(a0) * rr),
                                     ax + (int)(cosf(a1) * rr), ay + (int)(sinf(a1) * rr), 1, cq);
                }
            }
            for (int i = 0; i < 6 && p < 0.9f; i++) {
                float q = fmodf(p * 1.6f + i * 0.17f, 1);
                int bx = ax - 12 + i * 5 + (int)(sinf(t_fase * 9 + i) * 2), by = ay - R_DADO - (int)(q * 30);
                gfx_rect(bx, by, 2, 2, gfx_mistura(i % 2 ? C_FOGO : C_AMARELO, C_FELTRO, (int)(q * 70)));
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
        // Anel que se abre do dado (os sinos nao: eles saem do lugar).
        float tt = t == D_JINGLE ? 1 : p / 0.6f;
        if (tt < 1) {
            int rr = (int)(R_DADO * (0.8f + tt * 1.3f));
            for (int a = 0; a < 40; a++) {
                float an = a * 0.15708f;
                gfx_rect(ax + (int)(cosf(an) * rr) - 1, ay + (int)(sinf(an) * rr) - 1, 2, 2,
                         gfx_mistura(c, C_FELTRO, (int)(tt * 85)));
            }
        }
        float pm = capitao_agora(e) ? 0.7f : 0.45f;   // o Capitao: depois do ultimo galao
        if (p > pm && e->valor) {             // (a Meia anulada so guarda: nada sobe)
            char mais[8];
            snprintf(mais, sizeof mais, "+%d", e->valor);
            int ty = zona(e->de_j) - 10 - (int)((p - pm) * 30);
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

// O evento da vez; e, nos dados com a capa do Impermeavel que ele atingiu,
// o guarda-chuva abrindo quando o efeito chega (a onda da Medusa, em cada um
// quando ela passa). O efeito vai ate eles e espirra para os lados.
static void desenha_evento(void)
{
    if (fase != F_EFEITOS || ev_i >= n_ev || t_respiro >= 0) return;
    const evento_t *e = &ev[ev_i];
    float p = t_fase / duracao_evento(ev_i);
    desenha_evento_dentro(e, p);
    if (!e->bloqueado || e->alvo_j > 1) return;
    uint16_t c = e->tipo == EV_FOGO ? C_FOGO : e->tipo == EV_MEDUSA ? C_CINZA : C_VENTO;
    for (int k = 0; k < N_FILA; k++) {
        if (!(e->bloqueado >> k & 1)) continue;
        float chega = e->tipo == EV_MEDUSA ? 0.1f + 0.45f * abs(k - e->de_k) / 3.0f : e->tipo == EV_FOGO ? 0.35f : 0.38f;
        if (p < chega) continue;
        float q = (p - chega) / (1 - chega);
        escudo_capa(SLOT_X(k), zona(e->alvo_j), fminf(1, q / 0.2f), q, c);
    }
}

static void pote_mesa(void)
{
    int n = pote_c.mostra / 10 + 1;
    if (n > 9) n = 9;
    int y = CENTRO + 10;
    for (int i = 0; i < n; i++) {
        int sobe = 0;                            // na onda do crescendo, as fichas pulam
        if (t_onda >= 0) {
            float q = (t_onda - 0.3f - i * 0.03f) / 0.24f;
            if (q > 0 && q < 1) sobe = (int)(sinf(q * 3.1416f) * (onda_nivel + 1));
        }
        ficha(POTE_X, y - i * 4 - sobe, (i % 3 == 0) ? C_LATAO : (i % 3 == 1 ? C_VINHO : C_MARFIM_S));
    }
    char s[16];
    snprintf(s, sizeof s, "%d", pote_c.mostra);
    txt_c(POTE_X, y + 12 - (pote_c.pulo > 0.4f ? 1 : 0), s, conta_cor(&pote_c, C_OURO), true);
    txt_c(POTE_X, y - n * 4 - 26, "pote", C_TEXTO_M, false);
}
