// ---------------------------------------------------------------------------
// Perfis e dificuldades
// ---------------------------------------------------------------------------
static const char *const NOME_DIF[N_NIVEIS + 1] = { "Normal", "Desafiante - Nível 1", "Desafiante - Nível 2",
    "Desafiante - Nível 3", "Desafiante - Nível 4", "Desafiante - Nível 5", "Desafiante - Nível 6",
    "Desafiante - Nível 7", "Desafiante - Nível 8", "Desafiante - Nível 9", "Desafiante - Nível 10" };
static const char *const NOME_DIF_CURTO[N_NIVEIS + 1] = { "Normal", "Nível 1", "Nível 2", "Nível 3", "Nível 4",
    "Nível 5", "Nível 6", "Nível 7", "Nível 8", "Nível 9", "Nível 10" };
// Cada dificuldade e um andar do cassino, mais chique a cada um: a mesa
// mostra o andar nos acabamentos (veja pinta_mesa).
static const char *const ANDAR_DIF[N_NIVEIS + 1] = { "Térreo", "1º Andar", "2º Andar", "3º Andar", "4º Andar",
    "5º Andar", "6º Andar", "7º Andar", "8º Andar", "9º Andar", "Cobertura" };
static const uint16_t COR_DIF[N_NIVEIS + 1] = { C_MARFIM, C_BRONZE, RGB(210, 120, 80), C_PRATA, RGB(110, 190, 150),
    C_OURO, RGB(232, 150, 60), RGB(222, 70, 92), RGB(176, 110, 210), RGB(90, 140, 230), RGB(150, 220, 240) };
// O que cada dificuldade acrescenta (vale junto com as de baixo): uma coisa
// por nivel, para a subida ser aos poucos.
static const char *const PENA_DIF[N_NIVEIS + 1] = {
    "Os salões como sempre; só os chefes têm trunfo.",
    "O Crupiê passa a virar na 5ª rodada.",
    "A Dona também vira na 5ª rodada.",
    "A loja fica 25% mais cara.",
    "O Barão também vira na 5ª rodada.",
    "As mesas difíceis pensam mais.",
    "Oponentes trazem 1 dado raro a mais.",
    "Viradas do Crupiê e da Dona mais fortes.",
    "Oponentes trazem 2 dados raros a mais.",
    "Barão vira mais forte; chefes pensam mais.",
    "Os oponentes ganham um amuleto.",
};

static const char *nome_dif(int d) { return NOME_DIF_CURTO[d <= N_NIVEIS ? d : 0]; }

// O Coringa da dificuldade d, numa linha (0 se ele ainda nao aparece): do
// Nivel 5, depois do Barao, e mais forte a cada nivel.
static int coringa_txt(int d, char *s, int n)
{
    if (d < CORINGA_DESDE) { s[0] = 0; return 0; }
    if (d == CORINGA_DESDE) return snprintf(s, (size_t)n, "O Coringa espera depois do Barão, com %d dados.", coringa_bolsa(d));
    return snprintf(s, (size_t)n, "Coringa mais forte: %d dados%s.", coringa_bolsa(d),
                    d >= N_NIVEIS ? ", um amuleto e seu dado anulado some"
                    : d == CORINGA_SOME ? ", IA afiada; da 5ª rodada, seu dado anulado some"
                    : d > CORINGA_SOME ? "; da 5ª rodada, seu dado anulado some" : "");
}

// O nome do oponente sem o artigo e so com a inicial maiuscula ("Caubói").
static void nome_curto(const char *nome, char *s, int n)
{
    if ((nome[0] == 'O' || nome[0] == 'A') && nome[1] == ' ') nome += 2;
    int k = 0;
    for (int i = 0; nome[i] && k < n - 1; i++) {
        unsigned char c = (unsigned char)nome[i];
        if (i > 0 && c >= 'A' && c <= 'Z') c += 32;
        else if (i > 0 && c >= 0x80 && c <= 0x9E && (unsigned char)nome[i - 1] == 0xC3) c += 0x20;   // Á, Ã, Ó...
        s[k++] = (char)c;
    }
    s[k] = 0;
}

// Os oponentes comuns que passam a trazer o trunfo na dificuldade d:
// "Cautelosa, Apostador e Caubói". Devolve quantos.
static int trunfos_do_nivel(int d, char *s, int n)
{
    int q = 0, tot = 0;
    for (int p = 0; p < N_PERSO; p++) tot += !perso_chefe(p) && !perso_natal(p) && PERSO[p].andar_trunfo == d;
    s[0] = 0;
    for (int p = 0; p < N_PERSO; p++) {
        if (perso_chefe(p) || perso_natal(p) || PERSO[p].andar_trunfo != d) continue;
        char nm[24];
        nome_curto(PERSO[p].nome, nm, sizeof nm);
        int l = (int)strlen(s);
        snprintf(s + l, (size_t)(n - l), "%s%s", q == 0 ? "" : q == tot - 1 ? " e " : ", ", nm);
        q++;
    }
    return q;
}
static uint16_t cor_dif(int d) { return COR_DIF[d <= N_NIVEIS ? d : 0]; }

static int  pf_cur;                      // card em foco na tela de perfis
static bool pf_apagando;                 // pedindo confirmacao para apagar
static int  pf_avatar;                   // tela de criar perfil
static char pf_nome[11];
static int  dif_cur;

// Uma parte do estilo, pequena: o feltro e a mesa num retalho, o material
// num D6. lin: 0 feltro, 1 material, 2 mesa.
static void estilo_mini(int lin, int k, int cx, int cy, int r)
{
    if (lin == 1) {
        uint8_t antes = material_j[0];
        material_j[0] = (uint8_t)k;
        desenha_dado((float)cx, (float)cy, (float)r, D_D6, 0, 0.18f, 5, 2, false);
        material_j[0] = antes;
        return;
    }
    uint16_t c = lin == 0 ? FELTROS[k].tons[k == FT_GALAXIA ? 3 : 4] : k == MS_NOGUEIRA ? C_MADEIRA : MESAS[k].madeira[1];
    uint16_t aro = lin == 0 ? C_LATAO_ESC : k == MS_NOGUEIRA ? C_OURO : k == MS_LED ? LED_TOM[5] : MESAS[k].metal;
    int x = cx - r, y = cy - r * 2 / 3, w = r * 2, h = r * 4 / 3;
    gfx_rect(x, y, w, h, c);
    if (lin == 0) retalho_enfeite(k, x, y, w, h);
    if (lin != 0 || k != FT_NATAL) ui_contorno(x, y, w, h, aro);
}

// As partes do estilo de um andar (linha da tela de estilo e a opcao).
static int estilo_partes(int andar, int *lin, int *k)
{
    int n = 0;
    for (int i = 0; i < FT_N; i++)  if (ABRE_FELTRO[i] == andar)   { lin[n] = 0; k[n++] = i; }
    for (int i = 0; i < MAT_N; i++) if (ABRE_MATERIAL[i] == andar) { lin[n] = 1; k[n++] = i; }
    for (int i = 0; i < MS_N; i++)  if (ABRE_MESA[i] == andar)     { lin[n] = 2; k[n++] = i; }
    return n;
}
static void estilo_parte_nome(int lin, int k, char *s, int n)
{
    if (lin == 0) snprintf(s, (size_t)n, "feltro %s", FELTROS[k].nome);
    else if (lin == 1) snprintf(s, (size_t)n, "dados de %s", MATERIAL[k].nome);
    else snprintf(s, (size_t)n, "mesa %s", MESAS[k].nome);
}

static void des_abre_perfis(void)
{
    perfis_le();
    pf_cur = perfil_atual >= 0 ? perfil_atual : 0;
    pf_apagando = false;
    t_fase = 0;
    fase = F_PERFIS;
}

// Nova run: com dificuldades abertas, pergunta qual antes; depois, a bolsa.
static void abre_bolsas(void);           // bolsa_escolha.inc
static void des_escolhe_nova(void)
{
    if (perfil()->nivel > 0) {
        dif_cur = perfil()->nivel;
        t_fase = 0;
        fase = F_RDIFIC;
        return;
    }
    des_dif = 0;
    abre_bolsas();
}

static void des_entra_perfil(int i)
{
    perfil_atual = i;
    perfis_salva();
    estilo_do_perfil();                          // o feltro, a mesa e os dados dele
    if (des_tem_run()) {                         // continuar ou comecar outra
        menu_tela = MT_DESAFIO;
        menu_cur = 0;
        t_fase = 1.0f;
        fase = F_ABERTURA;
        return;
    }
    des_escolhe_nova();
}

static int avatar_retrato(void) { return perfil_atual >= 0 && perfis[perfil_atual].nome[0] ? RETRATO_AVATAR + perfil()->avatar : -1; }

static void moldura_retrato(int x, int y, int esc, int av, float t, bool cur)
{
    uint16_t r[4];                               // a moldura de latao em rampa, como a dos retratos do mapa
    ui_rampa(C_LATAO, !cur, r);
    ui_contorno(x - 4, y - 4, RT_W * esc + 8, RT_H * esc + 8, C_BARRA);
    ui_aro(x - 3, y - 3, RT_W * esc + 6, RT_H * esc + 6, 2, r, UI_VIVO);
    gfx_rect(x - 1, y - 1, RT_W * esc + 2, RT_H * esc + 2, C_BARRA);
    gfx_rect(x, y, RT_W * esc, RT_H * esc, gfx_mistura(C_FELTRO_ESC, C_BARRA, 30));
    retrato(x, y, esc, RETRATO_AVATAR + av, t, false, 0, RT_H);
}

static void faixa_titulo(const char *s)
{
    faixa_madeira(0);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    txt(botao_voltar(KEY_ESC), ty, "MODO DESAFIANTE", C_LATAO, true);
    txt_c(GFX_W / 2, ty, s, C_MARFIM, true);
}

static void desenha_perfis(void)
{
    ui_escurece(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, 110);
    faixa_titulo("Quem vai jogar?");
    for (int i = 0; i < N_PERFIS; i++) {
        const perfil_t *p = &perfis[i];
        int w = 170, h = 190, x = GFX_W / 2 + (i - 1) * 186 - w / 2, y = 66;
        bool cur = i == pf_cur;
        ui_cartao(x, y, w, h, cur, 0);
        zona_clique(x, y, w, h, &pf_cur, i, KEY_ENTER);
        if (!p->nome[0]) {
            int cx = x + w / 2, cy = y + 84;
            uint16_t c = cur ? C_OURO : C_TEXTO_M;
            gfx_rect(cx - 14, cy - 2, 28, 4, c);
            gfx_rect(cx - 2, cy - 14, 4, 28, c);
            txt_c(cx, y + 120, "novo perfil", c, true);
            continue;
        }
        int esc = 3, rx = x + (w - RT_W * esc) / 2, ry = y + 14;
        moldura_retrato(rx, ry, esc, p->avatar, t_fase + i * 2.1f, cur);
        txt_c(x + w / 2, ry + RT_H * esc + 8, p->nome, cur ? C_OURO : C_MARFIM, true);
        char s[48];
        int nv = p->nivel;
        txt_c(x + w / 2, ry + RT_H * esc + 26, NOME_DIF[nv], nv ? COR_DIF[nv] : C_MARFIM_S, false);
        snprintf(s, sizeof s, "%d %s", p->partidas, p->partidas == 1 ? "partida" : "partidas");
        txt_c(x + w / 2, ry + RT_H * esc + 42, s, C_TEXTO_M, false);
        int ganhou = 0;
        for (int d = 0; d <= N_NIVEIS; d++) ganhou += p->venceu[d];
        if (ganhou) {
            snprintf(s, sizeof s, "Runs vencidas: %d", ganhou);
            txt_c(x + w / 2, ry + RT_H * esc + 58, s, C_OURO, false);
        }
    }
    if (pf_apagando && perfis[pf_cur].nome[0]) {
        char s[80];
        snprintf(s, sizeof s, "Apagar %s e a run guardada?", perfis[pf_cur].nome);
        txt_c(GFX_W / 2, 266, s, C_VINHO_CLR, true);
        rodape_run("ENTER ou X apaga  ·  ESC deixa como está");
    } else {
        if (perfis[pf_cur].nome[0]) {
            int bx = GFX_W / 2 + (pf_cur - 1) * 186 - 104;
            ui_botao(bx, 264, 64, 22, "estilo", 0, 0);
            zona_clique(bx, 264, 64, 22, NULL, 0, 'e');
            ui_botao(bx + 70, 264, 80, 22, "histórico", 0, 0);
            zona_clique(bx + 70, 264, 80, 22, NULL, 0, 'h');
            ui_botao(bx + 156, 264, 60, 22, "apagar", C_VINHO, 0);
            zona_clique(bx + 156, 264, 60, 22, NULL, 0, 'x');
        }
        rodape_run("setas escolhem  ·  ENTER entra  ·  E estilo  ·  H histórico  ·  X apaga  ·  ESC volta");
    }
}

static void desenha_cria_perfil(void)
{
    ui_escurece(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, 110);
    faixa_titulo("Novo perfil");
    int esc = 4, rx = GFX_W / 2 - RT_W * esc / 2, ry = 58;
    moldura_retrato(rx, ry, esc, pf_avatar, t_fase, true);
    // Setas para trocar o retrato.
    for (int lado = -1; lado <= 1; lado += 2) {
        int cx = GFX_W / 2 + lado * (RT_W * esc / 2 + 34), cy = ry + RT_H * esc / 2;
        float o[6] = { (float)(cx + lado * 8), (float)cy, (float)(cx - lado * 6), (float)(cy - 10),
                       (float)(cx - lado * 6), (float)(cy + 10) };
        float vx[3] = { o[0], o[2], o[4] }, vy[3] = { o[1], o[3], o[5] };
        poligono(vx, vy, 3, C_LATAO);
        zona_clique(cx - 16, cy - 18, 32, 36, NULL, 0, lado < 0 ? KEY_LEFT : KEY_RIGHT);
    }
    int y = ry + RT_H * esc + 8;
    txt_c(GFX_W / 2, y, NOME_AVATAR[pf_avatar], C_OURO, true);
    // O nome, digitado.
    int bw = 180, bx = GFX_W / 2 - bw / 2, by = y + 22;
    gfx_rect(bx + 1, by + 1, bw - 2, 18, C_PLACA);
    ui_contorno(bx, by, bw, 20, C_LATAO);
    char s[16];
    bool pisca = fmodf(t_fase, 1.0f) < 0.55f;
    snprintf(s, sizeof s, "%s%s", pf_nome, pisca && strlen(pf_nome) < 10 ? "_" : " ");
    if (pf_nome[0] || pisca) txt_c(GFX_W / 2, by + 2, s, C_MARFIM, true);
    if (!pf_nome[0] && !pisca) txt_c(GFX_W / 2, by + 2, "seu nome", C_TEXTO_M, false);
    // Botao de criar.
    int ok = pf_nome[0] != 0;
    ui_botao(GFX_W / 2 - 50, by + 28, 100, 24, "criar", C_LATAO, ok ? 0 : UI_DESLIGADO);
    zona_clique(GFX_W / 2 - 50, by + 30, 100, 20, NULL, 0, KEY_ENTER);
    rodape_run("setas trocam o retrato  ·  digite o nome  ·  ENTER cria  ·  ESC volta");
}

static void desenha_dificuldade(void)
{
    ui_escurece(MESA_X0, MESA_Y0, MESA_X1 - MESA_X0, MESA_Y1 - MESA_Y0, 110);
    faixa_titulo("Dificuldade da run");
    int nv = perfil()->nivel, ult = nv < N_NIVEIS ? nv + 1 : N_NIVEIS;
    int x0 = 28, y0 = 54;
    for (int d = 0; d <= ult; d++) {
        bool aberta = d <= nv, cur = d == dif_cur;
        int y = y0 + d * 22;
        ui_cartao(x0, y, 150, 20, cur, 0);
        uint16_t c = aberta ? COR_DIF[d] : C_FELTRO2;
        gfx_disco(x0 + 14, y + 10, 6, gfx_mistura(c, C_BARRA, 30));
        gfx_disco(x0 + 14, y + 10, 4, c);
        txt(x0 + 28, y, NOME_DIF_CURTO[d], aberta ? c : C_TEXTO_M, true);
        if (perfil()->venceu[d]) {                  // medalha: o Barao ja caiu nesta
            gfx_disco(x0 + 134, y + 10, 5, C_LATAO_ESC);
            gfx_disco(x0 + 134, y + 10, 4, C_OURO);
            gfx_disco(x0 + 133, y + 9, 1, C_BRANCO);
        }
        if (aberta) zona_clique(x0, y, 150, 20, &dif_cur, d, KEY_ENTER);
        else zona_clique(x0, y, 150, 20, &dif_cur, d, 0);
    }
    // O que vale na escolhida: o que ela acrescenta, em destaque, e o que vem
    // de baixo, miudo.
    int px = 194, py = 54;
    ui_painel(px, py, 416, 254);
    int d = dif_cur;
    int wt = gfx_texto(px + 12, py + 8, NOME_DIF[d], COR_DIF[d], 1, true);
    txt(px + 12 + wt + 12, py + 8, ANDAR_DIF[d], C_TEXTO_M, false);
    int ly = py + 30;
    if (d > nv) {
        char s[64];
        snprintf(s, sizeof s, "Vença %s no %s para abrir.", d - 1 >= CORINGA_DESDE ? "o Coringa" : nome_chefe(2),
                 NOME_DIF_CURTO[d - 1]);
        txt(px + 12, ly, s, C_VINHO_CLR, false);
        ly += 20;
    }
    txt(px + 12, ly, "+", COR_DIF[d], true);
    txt(px + 24, ly, PENA_DIF[d], C_MARFIM, false);
    ly += 19;
    for (int k = d - 1, i = 0; k >= 1; k--, i++)    // as de baixo, da mais recente, em duas colunas
        gfx_texto(px + 26 + (i % 2) * 196, ly + 2 + (i / 2) * 11, PENA_DIF[k], C_TEXTO_M, GFX_MIUDO, false);
    if (d > 1) ly += (d / 2) * 11 + 4;
    if (d > 0) {                                     // quem passa a trazer o trunfo
        char nomes[96], t[128];
        if (trunfos_do_nivel(d, nomes, sizeof nomes)) {
            snprintf(t, sizeof t, "Com trunfo agora: %s.", nomes);
            txt(px + 24, ly, t, C_ROSA, false);
            ly += 17;
        }
    }
    {
        char cj[96];                                 // o Coringa, do Nivel 5
        if (coringa_txt(d, cj, sizeof cj)) {
            txt(px + 24, ly, cj, C_REAL, false);
            ly += 17;
        }
    }
    {
        char m[64];                                  // o mesmo comeco em todo andar
        snprintf(m, sizeof m, "Você começa com %d moedas.", moedas_iniciais(d));
        txt(px + 24, ly, m, C_VERDE, false);
        ly += 17;
    }
    // O que os chefes desta dificuldade abrem, chefe a chefe: aceso o que ja
    // abriu, apagado (com o nome do chefe apagado) o que falta.
    ly += 4;
    txt(px + 12, ly, "Os chefes deste nível abrem:", C_TEXTO_M, false);
    ly += 18;
    static const char *const CURTO[CH_N] = { "Crupiê", "Dona", "Barão", "Coringa" };
    const char *curto_c[CH_N] = { CURTO[0], CURTO[1], dezembro ? "Noel" : CURTO[2], CURTO[3] };
    for (int c = 0; c < chefes_de(d); c++) {
        novidade_t it[PEDACO_MAX];
        int ni = pedaco(d, c, it);
        if (!ni) continue;
        bool ab = chefe_abriu(d, c);
        int ix = px + 84, iy = ly + 9;
        gfx_texto(px + 24, iy - 10, curto_c[c], ab ? C_OURO : C_TEXTO_M, GFX_FINO, false);
        for (int i = 0; i < ni; i++) {               // em fluxo: cada um com a largura do seu nome
            int v = it[i].v, lin[4], k[4], np = it[i].k == NV_ESTILO ? estilo_partes(v, lin, k) : 1;
            for (int q = 0; q < np; q++) {
                char nome[40];
                switch (it[i].k) {
                case NV_DIF:     snprintf(nome, sizeof nome, "%s", NOME_DIF_CURTO[v]); break;
                case NV_AMULETO: snprintf(nome, sizeof nome, "%s", AMULETO[v].nome); break;
                case NV_ADESIVO: snprintf(nome, sizeof nome, "%s", ADESIVO[v].nome); break;
                case NV_DADO:    snprintf(nome, sizeof nome, "%s", TIPO[v].nome); break;
                default:         estilo_parte_nome(lin[q], k[q], nome, sizeof nome); break;
                }
                int wi = 13 + gfx_largura(nome, GFX_FINO) + 12;
                if (ix + wi > px + 410) { ix = px + 84; iy += 19; }
                uint16_t cn = C_TEXTO_M;
                switch (it[i].k) {
                case NV_DIF: {
                    char nn[4];
                    snprintf(nn, sizeof nn, "%d", v);
                    gfx_disco(ix, iy, 8, ab ? COR_DIF[v] : C_FELTRO2);
                    gfx_disco(ix, iy, 6, C_FELTRO_ESC);
                    gfx_texto(ix - gfx_largura(nn, GFX_MIUDO) / 2, iy - 6, nn, ab ? COR_DIF[v] : C_TEXTO_M, GFX_MIUDO, false);
                    if (ab) cn = COR_DIF[v];
                    break;
                }
                case NV_AMULETO:
                    desenha_amuleto(ix, iy, 8, v, !ab);
                    if (ab) { cn = AMULETO[v].cor; dica_zona(ix - 10, iy - 9, wi, 18, DICA_AMULETO, v); }
                    break;
                case NV_ADESIVO:
                    ad_solto_d(ix, iy, v, 1, 0);
                    if (ab) { cn = ADESIVO[v].luz; dica_zona(ix - 10, iy - 9, wi, 18, DICA_ADESIVO, v); }
                    break;
                case NV_DADO:
                    desenha_dado((float)ix, (float)iy, 8, v, 0, 0, 0, 0, !ab);
                    if (ab) { cn = cor_dado(v); dica_zona(ix - 10, iy - 9, wi, 18, DICA_DADO, v); }
                    break;
                default:
                    estilo_mini(lin[q], k[q], ix, iy, 7);
                    if (!ab) acinzenta(ix - 8, iy - 8, 17, 17, 70);
                    if (ab) cn = C_MARFIM_S;
                    break;
                }
                gfx_texto(ix + 12, iy - 10, nome, cn, GFX_FINO, false);
                ix += wi;
            }
        }
        ly = iy + 10;
    }
    rodape_run("setas escolhem  ·  ENTER começa a run  ·  ESC volta");
}

