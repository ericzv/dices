// ---------------------------------------------------------------------------
// O estilo do perfil: o feltro, a mesa e o material dos seus dados comuns.
// Abre andar a andar no Desafiante (vencer a run naquela dificuldade) e
// fica na chave do perfil ("EST1" + feltro, material e mesa, um caractere
// cada: '0' + o numero, que passa do 9).
// ---------------------------------------------------------------------------
static int estilo_mat = MAT_MARFIM;          // o material dos seus dados comuns

static const char *chave_estilo(int i)
{
    static const char *const K[N_PERFIS] = { "estilo1", "estilo2", "estilo3" };
    return K[i >= 0 && i < N_PERFIS ? i : 0];
}

// Aberto para o perfil i? (andar -1: sempre aberto; ANDAR_NATAL: de 20 de
// outubro a 6 de janeiro, para todos; ANDAR_REVEILLON: de 26 de dezembro a 6
// de janeiro) Quem
// fecha a run do andar o abre.
static bool estilo_aberto(int i, int andar)
{
    if (andar == ANDAR_NATAL) return natal;
    if (andar == ANDAR_REVEILLON) return reveillon;
    if (andar < 0) return true;
    return i >= 0 && i < N_PERFIS && perfis[i].nome[0] && chefe_abriu_de(&perfis[i], andar, chefe_final(andar));
}

// A escolha guardada do perfil i; o que nao estiver aberto volta ao padrao.
static void estilo_le(int i, int *feltro, int *mat, int *mesa_id)
{
    *feltro = FT_VERDE; *mat = MAT_MARFIM; *mesa_id = MS_NOGUEIRA;
    char b[16];
    if (i < 0 || salva_le(chave_estilo(i), b, (int)sizeof b) != 7 || memcmp(b, "EST1", 4)) return;
    int f = b[4] - '0', m = b[5] - '0', s = b[6] - '0';
    if (f >= 0 && f < FT_N && estilo_aberto(i, ABRE_FELTRO[f])) *feltro = f;
    if (m >= 0 && m < MAT_N && estilo_aberto(i, ABRE_MATERIAL[m])) *mat = m;
    if (s >= 0 && s < MS_N && estilo_aberto(i, ABRE_MESA[s])) *mesa_id = s;
}

static void estilo_grava(int i, int feltro, int mat, int mesa_id)
{
    char b[8] = { 'E', 'S', 'T', '1', (char)('0' + feltro), (char)('0' + mat), (char)('0' + mesa_id), 0 };
    salva_grava(chave_estilo(i), b);
}

static int estilo_material_seu(void) { return estilo_mat; }

// O perfil escolhido entra na mesa com o estilo dele (sem perfil, o padrao).
static void estilo_do_perfil(void)
{
    perfis_le();
    int f, m, s;
    estilo_le(perfil_atual >= 0 && perfis[perfil_atual].nome[0] ? perfil_atual : -1, &f, &m, &s);
    estilo_mat = m;
    material_j[baixo] = (uint8_t)m;
    if (f != estilo_feltro || s != estilo_mesa || !estilo_versao) aplica_estilo(f, s);
}

// ---- A tela ---------------------------------------------------------------
// Tres linhas (feltro, seus dados, mesa); setas escolhem. O que esta aberto
// vai para a mesa na hora - a mesa atras e a previa; o fechado diz o andar
// que o abre. es_foco: linha * ES_LIN + opcao.
#define ES_LIN 16
#define ES_VE  11                                // quantas opcoes cabem numa linha
static int es_foco;

static void estilo_abre(int i)
{
    perfil_atual = i;                            // ajeitar um perfil e entrar nele
    perfis_salva();
    estilo_do_perfil();
    es_foco = estilo_feltro;
    t_fase = 0;
    fase = F_ESTILO;
}

static const int ES_N[3] = { FT_N, MAT_N, MS_N };
static int es_andar(int lin, int k)
{
    return lin == 0 ? ABRE_FELTRO[k] : lin == 1 ? ABRE_MATERIAL[k] : ABRE_MESA[k];
}

// A amostra de cada opcao, centrada em (cx, cy).
static void es_amostra(int lin, int k, int cx, int cy, bool aberto)
{
    if (lin == 0) {                              // o feltro, com o pontilhado da mesa
        const uint16_t *t = FELTROS[k].tons;
        gfx_rect(cx - 25, cy - 15, 50, 30, C_BARRA);
        for (int y = 0; y < 28; y++)
            for (int x = 0; x < 48; x++) {
                float dx = (x - 23.5f) / 24, dy = (y - 13.5f) / 14, v = (dx * dx * 0.6f + dy * dy) * 1.5f;
                int i = v > 1.1f ? 1 : v > 0.55f ? 2 : 3;
                if (ruido_xy(x + k * 50, y) % 23 == 0) i = 4;
                gfx_pixel(cx - 24 + x, cy - 14 + y, t[i]);
            }
        retalho_enfeite(k, cx - 24, cy - 14, 48, 28);
    } else if (lin == 1) {                       // um D6 no material
        uint8_t antes = material_j[0];
        material_j[0] = (uint8_t)k;
        desenha_dado((float)cx, (float)cy, 15, D_D6, 0, 0.18f, 5, 2, false);
        material_j[0] = antes;
    } else {                                     // a madeira com a moldura
        bool n0 = k == MS_NOGUEIRA;
        uint16_t mad = n0 ? C_MADEIRA : MESAS[k].madeira[1], veio = n0 ? C_MADEIRA_ESC : MESAS[k].madeira[0];
        uint16_t metal = n0 ? C_OURO : MESAS[k].metal;
        gfx_rect(cx - 25, cy - 15, 50, 30, mad);
        for (int y = 0; y < 30; y += 2)
            for (int x = 0; x < 50; x++) if (ruido_xy(x / 7 + k * 9, y) % 5 == 0) gfx_pixel(cx - 25 + x, cy - 15 + y, veio);
        if (k == MS_LAREIRA) {                   // o tijolinho e a brasa
            for (int y = 0; y < 30; y++)
                for (int x = 0; x < 50; x++) {
                    int fiada = y / 5, bx = x + (fiada & 1) * 6;
                    uint16_t c = y % 5 == 4 || bx % 12 == 11 ? MESAS[k].madeira[0]
                               : ruido_xy(bx / 12 + 2, fiada) % 4 == 0 ? MESAS[k].madeira[2] : MESAS[k].madeira[1];
                    gfx_pixel(cx - 25 + x, cy - 15 + y, c);
                }
            gfx_moldura(cx - 16, cy - 8, 32, 24, 1, BRASA_TOM[1]);
            gfx_moldura(cx - 15, cy - 7, 30, 22, 2, BRASA_TOM[3]);
            for (int i = 0; i < 30; i += 3) { gfx_pixel(cx - 15 + i, cy - 7, BRASA_TOM[5]); gfx_pixel(cx - 14 + i, cy + 14, BRASA_TOM[5]); }
            gfx_rect(cx - 13, cy - 5, 26, 18, feltro_tons[3]);
        } else if (k == MS_PINHO) {              // a fita vermelha, com o laco
            gfx_moldura(cx - 15, cy - 7, 30, 22, 1, FITA_VERM[2]);
            gfx_moldura(cx - 14, cy - 6, 28, 20, 1, FITA_VERM[1]);
            gfx_rect(cx - 13, cy - 5, 26, 18, feltro_tons[3]);
            gfx_rect(cx - 5, cy - 10, 4, 4, FITA_VERM[1]); gfx_rect(cx + 2, cy - 10, 4, 4, FITA_VERM[1]);
            gfx_rect(cx - 1, cy - 9, 3, 3, FITA_VERM[2]);
        } else if (k == MS_LED) {                // a fita acesa no lugar da moldura
            gfx_moldura(cx - 16, cy - 8, 32, 24, 1, LED_HALO[4]);
            gfx_moldura(cx - 15, cy - 7, 30, 22, 1, LED_TOM[3]);
            gfx_moldura(cx - 14, cy - 6, 28, 20, 1, LED_TOM[5]);
            gfx_rect(cx - 13, cy - 5, 26, 18, feltro_tons[3]);
            gfx_moldura(cx - 13, cy - 5, 26, 18, 1, LED_HALO[3]);
        } else {
            gfx_moldura(cx - 15, cy - 7, 30, 22, 2, metal);
            gfx_rect(cx - 13, cy - 5, 26, 18, feltro_tons[3]);
            tacha(cx - 15, cy - 7, metal); tacha(cx + 14, cy - 7, metal);
        }
    }
    if (!aberto) {                               // fechado: apagado, com o cadeado
        acinzenta(cx - 26, cy - 17, 52, 34, 70);
        cadeado(cx, cy - 4, C_MARFIM_S, 1);
    }
}

static const char *es_nome(int lin, int k)
{
    return lin == 0 ? FELTROS[k].nome : lin == 1 ? MATERIAL[k].nome : MESAS[k].nome;
}

static void desenha_estilo(void)
{
    gfx_rect(0, 0, GFX_W, FAIXA_H, C_BARRA);
    gfx_rect(0, FAIXA_H - 2, GFX_W, 2, C_LATAO_ESC);
    int ty = (FAIXA_H - TXT_H) / 2 - 2;
    int tx = botao_voltar(KEY_ESC);
    txt(tx, ty, "ESTILO", C_LATAO, true);
    txt(tx + gfx_largura("ESTILO", 1) + 12, ty, perfil()->nome, C_MARFIM, true);
    txt(GFX_W - 18 - gfx_largura("a mesa atrás mostra a escolha", GFX_FINO), ty + 3,
        "a mesa atrás mostra a escolha", C_TEXTO_M, false);
    static const char *const TIT[3] = { "Feltro", "Seus dados", "Mesa" };
    int em_uso[3] = { estilo_feltro, estilo_mat, estilo_mesa };
    for (int lin = 0; lin < 3; lin++) {
        // O titulo em cima e as opcoes em fila, na largura toda (cabem 11).
        int y = 52 + lin * 80, n = ES_N[lin];
        ui_painel(26, y, GFX_W - 52, 76);
        int wt = gfx_texto(40, y + 2, TIT[lin], lin == es_foco / ES_LIN ? C_OURO : C_LATAO, 1, true);
        if (lin == 1) gfx_texto(40 + wt + 8, y + 3, "os comuns", C_TEXTO_M, GFX_FINO, false);
        // Mais de ES_VE opcoes: uma janela de ES_VE que so anda quando o foco
        // (ou a que esta em uso) sai dela; as setas mostram (e passam) o resto.
        static int es_ini[3];
        int ve = n < ES_VE ? n : ES_VE, foco = lin == es_foco / ES_LIN ? es_foco % ES_LIN : em_uso[lin];
        int ini = es_ini[lin];
        if (foco < ini) ini = foco;
        if (foco >= ini + ve) ini = foco - ve + 1;
        ini = ini < 0 ? 0 : ini > n - ve ? n - ve : ini;
        es_ini[lin] = ini;
        int vao = (GFX_W - 72) / ve > 92 ? 92 : (GFX_W - 72) / ve;
        int x0 = GFX_W / 2 - (ve * vao) / 2 + vao / 2, sw = vao - 2 < 64 ? vao - 2 : 64;
        if (ini > 0) {
            txt(31, y + 38, "<", C_OURO, true);
            zona_clique(28, y + 26, 14, 40, &es_foco, lin * ES_LIN + ini - 1, 'v');
        }
        if (ini + ve < n) {
            txt(GFX_W - 31 - gfx_largura(">", 1), y + 38, ">", C_OURO, true);
            zona_clique(GFX_W - 42, y + 26, 14, 40, &es_foco, lin * ES_LIN + ini + ve, 'v');
        }
        for (int k = ini; k < ini + ve; k++) {
            int cx = x0 + (k - ini) * vao, cy = y + 44;
            bool aberto = estilo_aberto(perfil_atual, es_andar(lin, k));
            if (k == em_uso[lin]) ui_selecao(cx - sw / 2, y + 25, sw, 50, C_OURO);
            else if (lin * ES_LIN + k == es_foco) ui_selecao(cx - sw / 2, y + 25, sw, 50, C_LATAO_ESC);
            es_amostra(lin, k, cx, cy, aberto);
            const char *nm = es_nome(lin, k);
            char curto[20];
            snprintf(curto, sizeof curto, "%s", nm);
            char *e = strstr(curto, " e ");                      // "Mogno e cobre" cabe so "Mogno"
            if (e) *e = 0;
            gfx_texto(cx - gfx_largura(curto, GFX_MIUDO) / 2, y + 62, curto,
                      k == em_uso[lin] ? C_OURO : aberto ? C_MARFIM_S : C_TEXTO_M, GFX_MIUDO, false);
            zona_clique(cx - sw / 2, y + 25, sw, 50, &es_foco, lin * ES_LIN + k, 'u');
        }
    }
    // O que esta em foco: o nome inteiro e, se fechado, o andar que abre.
    int fl = es_foco / ES_LIN, k = es_foco % ES_LIN, andar = es_andar(fl, k);
    char s[96];
    if (estilo_aberto(perfil_atual, andar)) snprintf(s, sizeof s, "%s", es_nome(fl, k));
    else if (andar == ANDAR_NATAL) snprintf(s, sizeof s, "%s  ·  só de 20 de outubro a 6 de janeiro", es_nome(fl, k));
    else if (andar == ANDAR_REVEILLON) snprintf(s, sizeof s, "%s  ·  só de 26 de dezembro a 6 de janeiro", es_nome(fl, k));
    else if (andar == 0) snprintf(s, sizeof s, "%s  ·  abre ao vencer %s", es_nome(fl, k), nome_chefe(chefe_final(andar)));
    else snprintf(s, sizeof s, "%s  ·  abre ao vencer %s no %s", es_nome(fl, k), nome_chefe(chefe_final(andar)),
                  NOME_DIF_CURTO[andar]);
    txt_c(GFX_W / 2, 292, s, estilo_aberto(perfil_atual, andar) ? C_MARFIM : C_TEXTO_M, true);
    rodape_run("setas escolhem  ·  ENTER ou ESC volta");
}

static bool estilo_tecla(int k)
{
    int lin = es_foco / ES_LIN, col = es_foco % ES_LIN;
    bool sobe = k == KEY_UP || k == 'w', desce = k == KEY_DOWN || k == 's';
    if (sobe || desce) {                         // trocar de linha so move o foco
        lin = (lin + (sobe ? 2 : 1)) % 3;
        if (col >= ES_N[lin]) col = ES_N[lin] - 1;
        es_foco = lin * ES_LIN + col;
        som_toca(SOM_TIQUE);
        return true;
    }
    if (k == KEY_LEFT || k == 'a') col = (col + ES_N[lin] - 1) % ES_N[lin];
    else if (k == KEY_RIGHT || k == 'd') col = (col + 1) % ES_N[lin];
    else if (k == KEY_ENTER) { t_fase = 0; fase = F_PERFIS; return true; }
    else if (k != 'u') return true;              // 'u': o clique numa opcao
    es_foco = lin * ES_LIN + col;
    som_toca(SOM_TIQUE);
    // O que esta aberto vai para a mesa na hora, e fica guardado.
    if (!estilo_aberto(perfil_atual, es_andar(lin, col))) return true;
    int f = estilo_feltro, m = estilo_mat, s = estilo_mesa;
    if (lin == 0) f = col; else if (lin == 1) m = col; else s = col;
    estilo_grava(perfil_atual, f, m, s);
    estilo_do_perfil();
    return true;
}
