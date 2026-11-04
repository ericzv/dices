// ---------------------------------------------------------------------------
// A escolha da bolsa, no comeco de cada run: as bolsas sao fichas no
// feltro, cada uma da sua cor, com o icone no centro, numa vitrine: a do
// cursor fica no meio, no tamanho cheio, e duas de cada lado, menores, mais
// cinzentas e mais apagadas quanto mais longe; o resto fica escondido. As
// setas giram a vitrine (a escolhida sempre no meio), e um pontinho por bolsa,
// embaixo, mostra onde se esta. As abertas vem primeiro, as fechadas no fim.
// As fichas caem uma depois da outra; a do meio sobe, vira e gira, e embaixo
// vem o nome (na letra do titulo), a regra e os dados com que ela comeca. A
// que ainda nao abriu fica escura, com o cadeado, e diz o que a abre; a que
// acabou de abrir chega fechada e o cadeado estoura. Escolhida, a ficha
// assenta e os dados saltam dela; a run comeca. Enquanto so o Saco de Pano
// esta aberto, nem pergunta.
// ---------------------------------------------------------------------------
#define BS_Y      110                    // o centro das fichas
#define BS_R      32                     // a do meio, em foco
#define BS_LADO   2                      // as que aparecem de cada lado
#define BS_CAI(s) (0.12f + 0.07f * (s))  // quando a ficha do lugar s (0 a 4, da esquerda) comeca a cair
#define BS_QUEDA  0.28f
#define BS_VIRA   0.26f                  // a ficha em foco vira (de lado e de volta)
#define BS_REVELA 0.95f                  // o cadeado da nova: treme e estoura
#define BS_CLIQUE_T 1.15f                // escolhida: ate a run comecar
#define TECLA_BS_VAI 0x210               // o clique numa ficha do lado ou num pontinho: vai ate ela (+ o lugar)
static int   bs_cur;                     // a bolsa do cursor
static int   bs_ant = -1;
static float t_bs_troca = 9;             // desde que o foco mudou
static float bs_giro[B_N];               // o angulo das listras de cada ficha
static uint8_t bs_ordem[B_N];            // a fila da vitrine: as abertas, depois as fechadas
static float bs_rolo;                    // o lugar da fila no meio agora (vai ate o do cursor)
static int   bs_pos0;                    // o lugar no meio quando a tela abriu (a ordem da queda)
static float t_bs_clique = -1;
static float t_bs_nega = -1;             // fechada: treme e o "para abrir" pisca
static uint32_t bs_novas;                // as que chegam abertas agora (a festa)
static uint32_t bs_chamou;               // as novas que ja trouxeram a vitrine ate elas
static uint32_t bs_caiu, bs_estourou;    // os sons que ja tocaram (caiu: por lugar da queda)

// A vitrine pela distancia ate o meio (0, 1, 2 e 3 lugares; entre eles, a
// reta): o x a partir do meio, o raio, a cor (1 cheia, 0 cinza) e o quanto
// a ficha se mistura no feltro. Depois de 2 lugares, ela some pelo pontilhado.
static const float BS_DX[4]    = { 0, 80, 144, 190 };
static const float BS_RR[4]    = { BS_R, 24, 16, 10 };
static const float BS_VIVA[4]  = { 1, 0.3f, 0, 0 };
static const float BS_APAGA[4] = { 0, 10, 34, 50 };
static float bs_curva(const float *k, float d)
{
    d = fabsf(d);
    if (d >= 3) return k[3];
    int i = (int)d;
    return k[i] + (k[i + 1] - k[i]) * (d - i);
}
static float bs_dx(float d) { return (d < 0 ? -1 : 1) * bs_curva(BS_DX, d); }

// O lugar da bolsa b na fila.
static int bs_pos(int b)
{
    for (int i = 0; i < B_N; i++) if (bs_ordem[i] == b) return i;
    return 0;
}
// Quando a ficha b comeca a cair: pelo lugar dela na vitrine, da esquerda
// para a direita (as escondidas, junto com as das pontas).
static float bs_cai(int b)
{
    int s = bs_pos(b) - bs_pos0 + BS_LADO;
    return BS_CAI(s < 0 ? 0 : s > 2 * BS_LADO ? 2 * BS_LADO : s);
}

// Quando o cadeado da nova estoura: depois que todas cairam, uma de cada vez.
static float bs_revela(int b)
{
    int k = 0;
    for (int i = 0; i < b; i++) k += bs_novas >> i & 1;
    return BS_CAI(2 * BS_LADO + 1) + BS_QUEDA + 0.25f + k * (BS_REVELA + 0.15f);
}
static bool bs_fechada(int b)
{
    if (!bolsa_aberta(b)) return true;
    return (bs_novas >> b & 1) && t_fase < bs_revela(b) + BS_REVELA * 0.55f;
}

static void abre_bolsas(void)
{
    int n = 0;
    for (int b = 0; b < B_N; b++) if (bolsa_aberta(b)) bs_ordem[n++] = (uint8_t)b;
    if (n <= 1) {                                // so o Saco de Pano: nem pergunta
        des_bolsa = B_PANO;
        des_nova_run();
        return;
    }
    for (int b = 0; b < B_N; b++) if (!bolsa_aberta(b)) bs_ordem[n++] = (uint8_t)b;
    const perfil_t *p = perfil();
    bs_novas = 0;
    for (int b = 1; b < B_N; b++)
        if (bolsa_aberta(b) && !(p->bolsas_vistas >> b & 1)) bs_novas |= (uint32_t)1 << b;
    bs_cur = p->bolsa_ult < B_N && bolsa_aberta(p->bolsa_ult) ? p->bolsa_ult : B_PANO;
    for (int b = 0; b < B_N; b++) if (bs_novas >> b & 1) { bs_cur = b; break; }   // a nova chama
    bs_pos0 = bs_pos(bs_cur);
    bs_rolo = (float)bs_pos0;
    bs_ant = -1;
    t_bs_troca = 9;
    t_bs_clique = t_bs_nega = -1;
    bs_chamou = bs_caiu = bs_estourou = 0;
    for (int b = 0; b < B_N; b++) bs_giro[b] = b * 0.5f;
    cursor = 0;
    t_fase = 0;
    fase = F_RBOLSAS;
}

// As novas ja foram vistas: daqui em diante chegam abertas, sem festa.
static void bs_marca_vistas(void)
{
    if (perfil_atual < 0 || !bs_novas) return;
    perfis[perfil_atual].bolsas_vistas |= bs_novas;
    perfis_salva();
}

static void bs_escolhe(void)
{
    if (t_bs_clique >= 0) return;
    if (t_fase < bs_cai(bs_cur) + BS_QUEDA) {             // ENTER adianta a queda
        t_fase = BS_CAI(2 * BS_LADO + 1) + BS_QUEDA;
        if (bs_caiu != (1u << (2 * BS_LADO + 1)) - 1) som_toca(SOM_FICHA);
        bs_caiu = (1u << (2 * BS_LADO + 1)) - 1;          // as que faltavam batem juntas, num som so
        return;
    }
    if (bs_fechada(bs_cur)) {
        t_bs_nega = 0;
        som_toca_tom(SOM_TIQUE, -7);
        return;
    }
    t_bs_clique = 0;
    som_toca(SOM_MENU_ESCOLHE);
}

static bool bs_tecla(int k, int e)
{
    if (t_bs_clique >= 0) {                       // escolhida: ENTER pula a festa
        if (k == KEY_ENTER) t_bs_clique = BS_CLIQUE_T;
        return true;
    }
    int p = bs_pos(bs_cur);
    if (e) {
        if (p + e < 0 || p + e >= B_N) bs_rolo += e * 0.14f;   // na ponta: a vitrine so balanca
        else bs_cur = bs_ordem[p + e];
        return true;
    }
    if (k >= TECLA_BS_VAI && k < TECLA_BS_VAI + B_N) { bs_cur = bs_ordem[k - TECLA_BS_VAI]; return true; }
    if (k >= '1' && k <= '9' && k - '1' < B_N) { bs_cur = bs_ordem[k - '1']; return true; }
    if (k == KEY_ENTER) bs_escolhe();
    return true;
}

// O passo de tudo das bolsas: o aviso de nova, o Porquinho quebrando, a luz
// do fosforo e, na escolha, as fichas.
static void bolsas_passo(float dt)
{
    if (bolsa_nova >= 0) t_bolsa_nova += dt;
    if (t_porco > 0 && (t_porco += dt) > 1.2f) t_porco = 0;
    t_fosf += dt;
    if (fase != F_RBOLSAS) return;
    for (int b = 0; b < B_N && t_bs_clique < 0; b++)      // a nova chama: a vitrine vai ate ela antes do cadeado estourar
        if ((bs_novas >> b & 1) && !(bs_chamou >> b & 1) && t_fase >= bs_revela(b) - 0.45f) {
            bs_chamou |= (uint32_t)1 << b;
            bs_cur = b;
        }
    if (bs_cur != bs_ant) {
        if (bs_ant >= 0) som_toca_tom(SOM_MENU_PASSA, bs_pos(bs_cur) * 8 / B_N);
        bs_ant = bs_cur;
        t_bs_troca = 0;
    }
    t_bs_troca += dt;
    if (t_bs_nega >= 0 && (t_bs_nega += dt) > 0.6f) t_bs_nega = -1;
    float alvo = (float)bs_pos(bs_cur);                   // a vitrine gira ate o cursor
    bs_rolo += (alvo - bs_rolo) * fminf(1, dt * 11);
    if (fabsf(alvo - bs_rolo) < 0.002f) bs_rolo = alvo;
    bs_giro[bs_cur] += dt * 0.8f;
    for (int s = 0; s <= 2 * BS_LADO; s++) {              // a ficha bate no feltro
        int i = bs_pos0 - BS_LADO + s;
        if (i < 0 || i >= B_N || (bs_caiu >> s & 1) || t_fase < BS_CAI(s) + BS_QUEDA * 0.8f) continue;
        bs_caiu |= 1u << s;
        som_toca_tom(SOM_FICHA, s * 8 / (2 * BS_LADO + 1) - 3);
    }
    for (int b = 0; b < B_N; b++)
        if ((bs_novas >> b & 1) && !(bs_estourou >> b & 1) && t_fase >= bs_revela(b) + BS_REVELA * 0.55f) {
            bs_estourou |= (uint32_t)1 << b;     // o cadeado estoura
            som_toca(SOM_NOVIDADE_LENDA);
            solta_confete(40);
            if (bs_estourou == bs_novas) bs_marca_vistas();
        }
    if (t_bs_clique < 0) return;
    float antes = t_bs_clique;
    t_bs_clique += dt;
    bs_giro[bs_cur] += dt * 16 * fmaxf(0, 1 - t_bs_clique / 0.5f);   // gira rapido e deita
    if (antes < 0.3f && t_bs_clique >= 0.3f) som_toca(SOM_BOLSA_ESPIA);   // abre: os dados saltam
    if (t_bs_clique >= BS_CLIQUE_T) {
        t_bs_clique = -1;
        bs_marca_vistas();
        des_bolsa = bs_cur;
        des_nova_run();
    }
}

// ---- O desenho -------------------------------------------------------------
// Uma linha da regra, centrada, com a setinha antes: na letra fina, ou na
// miuda se for comprida.
static void bs_seta(int x, int y, bool sobe, uint16_t c);
static void bs_linha(int y, const char *s, uint16_t c, bool sobe, uint16_t cs)
{
    int f = gfx_largura(s, GFX_FINO) <= 540 ? GFX_FINO : GFX_MIUDO;
    int w = gfx_largura(s, f), x = GFX_W / 2 - (w + 12) / 2;
    bs_seta(x + 3, y + 8, sobe, cs);
    gfx_texto(x + 12, y + (f == GFX_MIUDO ? 4 : 0), s, c, f, false);
}

// O nome, na letra do titulo: as letras caem uma a uma quando o foco chega.
static void bs_nome(int cx, int y, const char *nome, uint16_t c, float t)
{
    int w = gfx_largura(nome, 3), x = cx - w / 2;
    const char *p = nome;
    char pre[48];
    for (int i = 0; *p; i++) {
        const char *ini = p;
        gfx_proximo_car(&p);
        size_t nb = (size_t)(p - ini), np = (size_t)(ini - nome);
        if (np + nb >= sizeof pre) break;
        memcpy(pre, nome, np);
        pre[np] = 0;
        int lx = x + gfx_largura(pre, 3);
        char um[8];
        memcpy(um, ini, nb);
        um[nb] = 0;
        float q = (t - i * 0.028f) / 0.16f;
        if (q <= 0) continue;
        int dy = q < 1 ? (int)((1 - sai_mola(q)) * -14) : 0;
        gfx_texto(lx + 3, y + 3 + dy, um, C_SOMBRA, 3, true);
        gfx_texto(lx, y + dy, um, q < 0.5f ? C_BRANCO : c, 3, true);
    }
}

// As setinhas da regra: para cima (o que melhora) e para baixo (o que piora).
static void bs_seta(int x, int y, bool sobe, uint16_t c)
{
    for (int i = 0; i < 4; i++) {
        int w = sobe ? i : 3 - i;
        gfx_rect(x - w, y + i, 2 * w + 1, 1, c);
    }
}
// As setas dos lados da vitrine: botoes de ebano com a ponta da seta, como
// o "<" de voltar. O mouse por cima so acende; so o clique gira a vitrine.
// Na ponta da fila, a seta daquele lado fica apagada.
#define BS_SETA_DX 214                           // do meio da mesa ao meio da seta
#define BS_SETA_W  24
#define BS_SETA_H  30
static void bs_setas(void)
{
    int p = bs_pos(bs_cur);
    for (int lado = -1; lado <= 1; lado += 2) {
        int x = GFX_W / 2 + lado * BS_SETA_DX - BS_SETA_W / 2, y = BS_Y - BS_SETA_H / 2;
        bool pode = lado < 0 ? p > 0 : p < B_N - 1, livre = pode && t_bs_clique < 0;
        bool em = livre && mouse_em(x - 4, y - 6, BS_SETA_W + 8, BS_SETA_H + 12);
        int d;
        uint16_t c = ui_botao_face(x, y, BS_SETA_W, BS_SETA_H, 0, pode ? (em ? UI_FOCO : 0) : UI_DESLIGADO, &d);
        int cx = x + BS_SETA_W / 2 - lado, cy = y + d + (BS_SETA_H - 4) / 2;
        gfx_linha_grossa(cx - 3 * lado, cy - 6, cx + 3 * lado, cy, 3, c);   // so a ponta: < ou >
        gfx_linha_grossa(cx + 3 * lado, cy, cx - 3 * lado, cy + 6, 3, c);
        if (livre) zona_clique(x - 4, y - 6, BS_SETA_W + 8, BS_SETA_H + 12, NULL, lado, lado < 0 ? KEY_LEFT : KEY_RIGHT);
    }
}

// As do fundo somem no feltro pelo pontilhado: guarda o que esta embaixo da
// ficha antes de desenha-la e, depois, devolve os pontos que ja sumiram.
#define BS_GUARDA 72
static uint16_t bs_guarda[BS_GUARDA * BS_GUARDA];
static int bs_gx, bs_gy, bs_gw, bs_gh;
static void bs_guarda_fundo(int x, int y, int w, int h)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > GFX_W) w = GFX_W - x;
    if (y + h > GFX_H) h = GFX_H - y;
    if (w > BS_GUARDA) w = BS_GUARDA;
    if (h > BS_GUARDA) h = BS_GUARDA;
    bs_gx = x; bs_gy = y; bs_gw = w > 0 ? w : 0; bs_gh = h > 0 ? h : 0;
    const uint16_t *fb = gfx_alvo() ? gfx_alvo() : display_fb();
    for (int j = 0; j < bs_gh; j++) memcpy(bs_guarda + j * BS_GUARDA, fb + (y + j) * GFX_W + x, (size_t)bs_gw * sizeof(uint16_t));
}
static void bs_some(float q)
{
    uint16_t *fb = gfx_alvo() ? gfx_alvo() : display_fb();
    for (int j = 0; j < bs_gh; j++)
        for (int i = 0; i < bs_gw; i++)
            if (bayer(bs_gx + i, bs_gy + j) < q) fb[(bs_gy + j) * GFX_W + bs_gx + i] = bs_guarda[j * BS_GUARDA + i];
}

// Embaixo da vitrine, um pontinho por bolsa, na ordem da fila: as abertas
// cheias, na cor delas; as fechadas, so o aro; a do cursor, maior; a nova
// (a da etiqueta NOVA), com um aro dourado e um brilho dando a volta nele.
// Passar o mouse por eles tambem escolhe.
#define BS_PT_Y   150
#define BS_PT_VAO 11
static void bs_ponto(int x, int y, int r, uint16_t c, bool cheio)
{
    for (int j = -r; j <= r; j++)
        for (int i = -r; i <= r; i++) {
            int d = i * i + j * j;
            if (d > r * r + r - 1) continue;                  // o redondo de pixel: sem as quinas
            if (!cheio && d <= (r - 1) * (r - 1) + (r - 1) - 1 && r > 1) continue;
            gfx_pixel(x + i, y + j, c);
        }
}
static void bs_aro_novo(int x, int y, int R, int b)
{
    float brilho = fmodf(t_relogio * 4.2f + b * 1.3f, 6.2832f);      // o brilho dando a volta
    for (int j = -R; j <= R; j++)
        for (int i = -R; i <= R; i++) {
            int d = i * i + j * j;
            if (d < R * R - R + 1 || d > R * R + R) continue;        // so o aro, de um pixel
            float a = atan2f((float)j, (float)i) + 3.1416f, dist = fabsf(a - brilho);
            if (dist > 3.1416f) dist = 6.2832f - dist;
            gfx_pixel(x + i, y + j, dist < 0.45f ? C_BRANCO : dist < 1.3f ? C_OURO : C_LATAO);
        }
}
static void bs_pontinhos(void)
{
    int x0 = GFX_W / 2 - (B_N - 1) * BS_PT_VAO / 2;
    for (int i = 0; i < B_N; i++) {
        int b = bs_ordem[i], x = x0 + i * BS_PT_VAO;
        bool foco = b == bs_cur, fechada = bs_fechada(b);
        uint16_t c = fechada ? C_TEXTO_M : b == B_PANO ? C_LATAO : BOLSA[b].cor;
        if (foco) bs_ponto(x, BS_PT_Y, 3, fechada ? C_MARFIM_S : c, !fechada);
        else bs_ponto(x, BS_PT_Y, 2, c, !fechada);
        if (foco && !fechada) gfx_pixel(x - 1, BS_PT_Y - 1, C_MARFIM);   // o brilho da escolhida
        if ((bs_novas >> b & 1) && !fechada) bs_aro_novo(x, BS_PT_Y, (foco ? 3 : 2) + 2, b);
        if (t_bs_clique < 0) zona_clique(x - BS_PT_VAO / 2, BS_PT_Y - 6, BS_PT_VAO, 12, &bs_cur, b, TECLA_BS_VAI + i);
    }
}

static void desenha_bolsas(void)
{
    ui_escurece(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, 110);
    faixa_titulo("Escolha a bolsa da run");
    {                                                     // a dificuldade, a direita da faixa
        int ty = (FAIXA_H - TXT_H) / 2 - 2;
        const char *dn = NOME_DIF_CURTO[des_dif];
        txt(MESA_X1 - 8 - gfx_largura(dn, 1), ty, dn, COR_DIF[des_dif], true);
    }
    // A luz no feltro, no meio da vitrine.
    if (t_fase > BS_CAI(BS_LADO)) {
        ui_clareia(GFX_W / 2 - 40, BS_Y - 46, 80, 92, 0.6f);
        ui_clareia(GFX_W / 2 - 30, BS_Y - 40, 60, 80, 0.6f);
    }
    // As fichas a vista, das do fundo para a do meio (a de cima por ultimo).
    int vis[B_N], nv = 0;
    for (int i = 0; i < B_N; i++)
        if (fabsf(i - bs_rolo) < BS_LADO + 0.85f) vis[nv++] = i;
    for (int a = 1; a < nv; a++)
        for (int k = a; k > 0 && fabsf(vis[k] - bs_rolo) > fabsf(vis[k - 1] - bs_rolo); k--) { int t = vis[k]; vis[k] = vis[k - 1]; vis[k - 1] = t; }
    int foco_x = GFX_W / 2;                               // onde esta a do cursor (os dados saltam dela)
    for (int k = 0; k < nv; k++) {
        int i = vis[k], b = bs_ordem[i];
        float d = i - bs_rolo, ad = fabsf(d);
        float q = (t_fase - bs_cai(b)) / BS_QUEDA;
        if (q <= 0) continue;
        bool foco = b == bs_cur, fechada = bs_fechada(b);
        int r = (int)lroundf(bs_curva(BS_RR, d));
        int cx = GFX_W / 2 + (int)lroundf(bs_dx(d)), cy = BS_Y - (q < 1 ? (int)((1 - sai_mola(q)) * 70) : 0);
        float sobe = fmaxf(0, 1 - ad), viva = bs_curva(BS_VIVA, d);   // perto do meio, a ficha sobe e ganha a cor
        if (foco) foco_x = cx;
        // As do lado: o clique vai ate ela; o mouse por cima so acende um pouco.
        int zx = cx - r - 3, zy = BS_Y - r - 8, zw = 2 * r + 6, zh = 2 * r + 12;
        if (!foco && ad < BS_LADO + 0.4f && t_bs_clique < 0 && mouse_em(zx, zy, zw, zh)) { viva = fminf(1, viva + 0.45f); sobe += 0.3f; }
        float sx = 1, t_cad = -1;
        if (foco && t_bs_troca < BS_VIRA && q >= 1)           // virando: de lado e de volta
            sx = fabsf(cosf(t_bs_troca / BS_VIRA * 3.1416f));
        if (bs_novas >> b & 1) {                              // a nova: o cadeado treme e estoura
            float rv = t_fase - bs_revela(b);
            if (rv >= 0 && rv < BS_REVELA * 0.55f) t_cad = rv;
            float v = (rv - BS_REVELA * 0.45f) / 0.2f;        // e ela vira, aberta
            if (v > 0 && v < 1) sx = fabsf(cosf(v * 3.1416f));
        }
        if (foco && t_bs_nega >= 0) { cx += (int)(sinf(t_bs_nega * 60) * 3 * (1 - t_bs_nega / 0.6f)); t_cad = t_bs_nega; }
        float aperta = foco && t_bs_clique >= 0 ? sinf(fminf(t_bs_clique / 0.35f, 1) * 3.1416f) : 0;
        cy += (int)lroundf(aperta * 3);
        if (foco && t_bs_clique >= 0 && t_bs_clique < 0.6f) {   // o anel da que assenta
            float c = t_bs_clique / 0.6f;
            int ra = r + 4 + (int)(c * 26);
            uint16_t ca = gfx_mistura(C_OURO, C_FELTRO, (int)(c * 100));
            for (int n = 0; n < 48; n++) {
                float an = n * 0.1309f;
                gfx_rect(cx + (int)(cosf(an) * ra), cy + (int)(sinf(an) * ra), 1, 1, ca);
            }
        }
        float some = ad > BS_LADO ? (ad - BS_LADO) / 0.8f : 0;   // indo para o fundo: some no feltro
        if (some > 0) bs_guarda_fundo(cx - r - 3, cy - r - 9, 2 * r + 9, 2 * r + 17);
        fb_apaga = (int)lroundf(bs_curva(BS_APAGA, d));
        ficha_bolsa_viva(cx, cy, r, b, sobe, bs_giro[b], foco, fechada, sx, foco ? t_relogio : -1, t_cad, viva);
        int lev = (int)lroundf(sobe * 6);
        const perfil_t *pf = perfil();
        if (!fechada && pf->bolsa_rec[b]) {                    // o recorde: o nivel mais alto vencido com ela
            int dn = pf->bolsa_rec[b] - 1;
            if (sobe > 0.6f) {                                // em foco: a medalha com o nivel
                char nv2[4];
                int mx = cx + r - 6, my = cy + r - 6 - lev;
                snprintf(nv2, sizeof nv2, "%d", dn);
                gfx_disco(mx, my, 7, C_LATAO_ESC);
                gfx_disco(mx, my, 6, COR_DIF[dn]);
                gfx_texto(mx - gfx_largura(nv2, GFX_MIUDO) / 2, my - 6, nv2, C_BARRA, GFX_MIUDO, false);
            } else {                                          // pequena: so o pontinho na cor do nivel
                gfx_disco(cx + r - 3, cy + r - 3, 3, fb_ap(C_LATAO_ESC));
                gfx_disco(cx + r - 3, cy + r - 3, 2, fb_ap(COR_DIF[dn]));
            }
        }
        fb_apaga = 0;
        if (some > 0) bs_some(some);
        if ((bs_novas >> b & 1) && t_fase - bs_revela(b) >= 0 && t_fase - bs_revela(b) < BS_REVELA) {
            float rv = (t_fase - bs_revela(b) - BS_REVELA * 0.55f) / 0.4f;   // os pedacos do cadeado
            for (int n = 0; rv > 0 && rv < 1 && n < 10; n++) {
                float an = n * 0.628f, dd = (r + 6) * rv;
                int px = cx + (int)(cosf(an) * dd), py = cy + (int)(-sinf(an) * dd + 40 * rv * rv);
                gfx_rect(px, py, 2, 2, n % 2 ? C_LATAO : C_ACO);
            }
        }
        if ((bs_novas >> b & 1) && !fechada && ad < BS_LADO + 0.2f) {   // a etiqueta de nova, em cima
            int ny = cy - r - 13 - lev + (int)(sinf(t_relogio * 4 + b) * 1.5f);
            int w = gfx_largura("NOVA", GFX_MIUDO) + 6;
            gfx_rect(cx - w / 2, ny, w, 10, C_VINHO);
            gfx_texto(cx - w / 2 + 3, ny - 2, "NOVA", C_BRANCO, GFX_MIUDO, false);
        }
        // Escolhida, nada mais e clicavel: o mouse passando nao troca a bolsa
        // antes da run comecar.
        if (t_bs_clique >= 0) continue;
        if (foco) zona_clique(cx - r - 4, BS_Y - BS_R - 6, 2 * r + 8, 2 * BS_R + 8, &bs_cur, b, KEY_ENTER);
        else if (ad < BS_LADO + 0.4f) zona_clique(zx, zy, zw, zh, NULL, i, TECLA_BS_VAI + i);
    }
    if (t_fase > BS_CAI(BS_LADO)) { bs_pontinhos(); bs_setas(); }

    // A escolhida: os dados saltam da ficha, giram e caem fora da mesa.
    if (t_bs_clique >= 0.3f) {
        float t = t_bs_clique - 0.3f;
        const bolsa_t *bo = &BOLSA[bs_cur];
        int n = bolsa_n_dados(bs_cur), cx = foco_x;
        for (int i = 0; i < n; i++) {
            float an = 3.1416f * (0.12f + 0.76f * (n > 1 ? (float)i / (n - 1) : 0.5f));
            float v = 150 + (i * 37 % 60), tt = t - i * 0.012f;
            if (tt <= 0) continue;
            float x = cx + cosf(an) * v * tt * 0.9f, y = BS_Y - sinf(an) * v * tt + 260 * tt * tt;
            desenha_dado(x, y, 9, bo->dados[i], 0, tt * (i % 2 ? 9 : -9), 0, 0, false);
        }
        float esc = (t_bs_clique - 0.75f) / (BS_CLIQUE_T - 0.75f);    // e a mesa escurece
        if (esc > 0) gfx_rect_alfa(0, 0, GFX_W, GFX_H, C_BARRA, (int)(fminf(esc, 1) * 230));
    }

    // Embaixo, a da vez: o nome, a regra e os dados do comeco.
    int b = bs_cur;
    const bolsa_t *bo = &BOLSA[b];
    bool fechada = bs_fechada(b);
    if (t_fase < bs_cai(b) + BS_QUEDA * 0.5f) { rodape_run("setas escolhem  ·  ENTER começa a run  ·  ESC volta"); return; }
    float t = t_bs_troca;
    uint16_t cn = fechada ? C_TEXTO_M : (b == B_PANO ? C_LATAO : bo->cor);
    bs_nome(GFX_W / 2, 148, bo->nome, cn, t);
    float qa = fminf(1, (t - 0.12f) / 0.2f);                  // a regra acende depois do nome
    if (qa > 0) {
        int y = 194;
        uint16_t cg = gfx_mistura(fechada ? C_TEXTO_M : C_MARFIM, C_FELTRO, (int)((1 - qa) * 100));
        uint16_t cc = gfx_mistura(fechada ? C_TEXTO_M : C_MARFIM_S, C_FELTRO, (int)((1 - qa) * 100));
        if (b == B_PANO) {
            txt_c(GFX_W / 2, y + 6, "Os dados de sempre, sem regra nenhuma.", cg, false);
        } else {
            bs_linha(y, bo->ganho, cg, true, gfx_mistura(C_LASER, C_FELTRO, (int)((1 - qa) * 100)));
            bs_linha(y + 18, bo->custo, cc, false, gfx_mistura(C_VINHO_CLR, C_FELTRO, (int)((1 - qa) * 100)));
        }
    }
    // Os dados do comeco, depois do rotulo: os de base juntos (D6 x6), e os
    // que vem a mais (o D8 e os de poder), depois do "+", com o nome
    // embaixo (repetidos, com o x3 ao lado). Entram um de cada vez.
    // Os Quebrados, cada um com o numero gravado, ficam separados.
    int tipos[BOLSA_DADOS], fixos[BOLSA_DADOS], conta[BOLSA_DADOS], ng = 0, n = bolsa_n_dados(b), dy = 252;
    for (int i = 0; i < n; i++) {
        int g = 0, fx = bolsa_fixo(b, i);
        while (g < ng && (tipos[g] != bo->dados[i] || fixos[g] != fx)) g++;
        if (g == ng) { tipos[ng] = bo->dados[i]; fixos[ng] = fx; conta[ng++] = 0; }
        conta[g]++;
    }
    int larg[BOLSA_DADOS], w = 0;
    bool mais_antes = false;
    for (int g = 0; g < ng; g++) {
        bool poder = bolsa_extra(tipos[g]);
        char xn[8];
        snprintf(xn, sizeof xn, "x%d", conta[g]);
        int wn = gfx_largura(TIPO[tipos[g]].nome, GFX_FINO) + 4, wx = conta[g] > 1 ? gfx_largura(xn, GFX_FINO) + 2 : 0;
        larg[g] = poder ? (wn > 26 + wx ? wn : 26 + wx) : 24 + wx;
        if (poder && !mais_antes) { w += 14; mais_antes = true; }   // o "+"
        w += larg[g] + 6;
    }
    if (bo->mais[0]) w += gfx_largura(bo->mais, GFX_FINO) + 8;
    int wr = gfx_largura("começa com", GFX_FINO) + 10;   // o rotulo, na frente da fila
    int x = GFX_W / 2 - (w + wr) / 2;
    gfx_texto(x, dy - 11, "começa com", C_TEXTO_M, GFX_FINO, false);
    x += wr;
    mais_antes = false;
    for (int g = 0; g < ng; g++) {
        float q = (t - 0.2f - g * 0.05f) / 0.16f;
        bool poder = bolsa_extra(tipos[g]);
        if (poder && !mais_antes) {
            mais_antes = true;
            if (q > 0) txt(x + 2, dy - 10, "+", fechada ? C_TEXTO_M : C_OURO, true);
            x += 14;
        }
        if (q > 0) {
            int wx = conta[g] > 1 ? gfx_largura("x0", GFX_FINO) + 2 : 0;
            int cx = poder ? x + (larg[g] - wx) / 2 : x + 12;
            peca_t p = tipos[g] == D_ADESIVADO ? adesivado_amostra() : (peca_t){ .tipo = (uint8_t)tipos[g], .fixo = (uint8_t)fixos[g] };
            float r = (poder ? 11 : 10) * (q < 1 ? sai_mola(q) : 1);
            peca_parada((float)cx, (float)dy, r, p, 0);
            if (fechada) ui_escurece(cx - 12, dy - 12, 25, 25, 120);
            dica_zona(cx - 11, dy - 11, 22, 22, DICA_DADO, tipos[g]);
            char xn[8];
            snprintf(xn, sizeof xn, "x%d", conta[g]);
            if (poder) {
                const char *nm = TIPO[tipos[g]].nome;
                gfx_texto(x + larg[g] / 2 - gfx_largura(nm, GFX_FINO) / 2, dy + 8, nm, fechada ? C_TEXTO_M : cor_dado(tipos[g]),
                          GFX_FINO, false);
                if (conta[g] > 1) gfx_texto(cx + 12, dy - 8, xn, fechada ? C_TEXTO_M : C_MARFIM_S, GFX_FINO, false);
            } else if (conta[g] > 1) gfx_texto(x + 24, dy - 8, xn, fechada ? C_TEXTO_M : C_MARFIM_S, GFX_FINO, false);
        }
        x += larg[g] + 6;
    }
    if (bo->mais[0] && t > 0.2f + ng * 0.05f) txt(x + 2, dy - 10, bo->mais, fechada ? C_TEXTO_M : C_MARFIM_S, false);
    // Embaixo de tudo: o que a abre, ou o recorde com ela.
    char s[96];
    if (fechada && !bolsa_aberta(b)) {
        bool pisca = t_bs_nega >= 0 && (int)(t_bs_nega * 10) % 2;
        snprintf(s, sizeof s, "Para abrir: %s", bo->abre);
        txt_c(GFX_W / 2, 276, s, pisca ? C_BRANCO : C_OURO, true);
    } else if (perfil()->bolsa_rec[b]) {
        int d = perfil()->bolsa_rec[b] - 1;
        snprintf(s, sizeof s, "Já venceu uma run com ela no %s", NOME_DIF_CURTO[d]);
        txt_c(GFX_W / 2, 278, s, COR_DIF[d], false);
    }
    rodape_run(fechada && !bolsa_aberta(b) ? "setas escolhem  ·  esta ainda não abriu  ·  ESC volta"
                                           : "setas escolhem  ·  ENTER começa a run  ·  ESC volta");
}
