// ===========================================================================
// Utilidades
// ===========================================================================
static uint32_t rnd(void)
{
    semente ^= semente << 13;
    semente ^= semente >> 17;
    semente ^= semente << 5;
    return semente;
}
static int sorte(int n) { return n > 0 ? (int)(rnd() % (uint32_t)n) : 0; }
// Sorteio so do que se ve (faces piscando, trajetoria do dado): separado do
// sorteio das regras, que no modo online precisa andar igual nos dois lados.
static uint32_t semente_v = 88172645u;
static uint32_t rnd_v(void)
{
    semente_v ^= semente_v << 13; semente_v ^= semente_v >> 17; semente_v ^= semente_v << 5;
    return semente_v;
}
static float frnd(float a, float b) { return a + (b - a) * (float)(rnd_v() % 10000) / 10000.0f; }
static int rola_v(int lados) { return 1 + (int)(rnd_v() % (uint32_t)lados); }
// Face que pisca enquanto o dado rola: so as que ele pode mostrar.
static int face_v(int tipo, int valor)
{
    if (tipo == D_QUEBRADO || tipo == D_FANTASMA || tipo == D_PREVISIVEL || tipo == D_RELOGIO) return valor;
    if (tipo == D_VAZIO) return 0;
    if (tipo == D_LASTRO)   return 3 + rola_v(9);
    return rola_v(TIPO[tipo].lados);
}
// A epoca do Natal, pelo relogio do computador: de 20 de outubro a 6 de
// janeiro (o Dia de Reis). Dentro dela, dezembro (o Papai Noel e os
// oponentes de Natal) e o Reveillon (de 26 de dezembro a 6 de janeiro). Online,
// pelo de Greenwich, para os dois lados sortearem dos mesmos dados. Le-se no
// comeco de cada partida (e na volta ao menu). natal_forcado: os testes e as
// cenas de amostra fixam a epoca (0 fora dela, 1 novembro, 2 dezembro, 3 do
// dia 26 em diante; -1, o relogio).
#define NATAL_DESDE 20                   // o dia de outubro em que comeca
#define NATAL_ATE   6                    // e o de janeiro em que acaba
static int natal_forcado = -1;
static void poe_epoca(int mes, int dia)  // mes de 0 (janeiro) a 11
{
    bool jan = mes == 0 && dia <= NATAL_ATE;
    natal = (mes == 9 && dia >= NATAL_DESDE) || mes == 10 || mes == 11 || jan;
    dezembro = mes == 11;
    reveillon = (dezembro && dia >= 26) || jan;
}
static void le_epoca(void)
{
    if (natal_forcado >= 0) {
        static const int8_t MES[4] = { 3, 10, 11, 11 }, DIA[4] = { 1, 1, 1, 26 };
        int f = natal_forcado < 4 ? natal_forcado : 0;
        poe_epoca(MES[f], DIA[f]);
        return;
    }
    time_t agora = time(NULL);
    const struct tm *m = modo == M_ONLINE ? gmtime(&agora) : localtime(&agora);
    if (m) poe_epoca(m->tm_mon, m->tm_mday);
    else natal = dezembro = reveillon = false;
}

static int outro(int j) { return j ^ 1; }
static int rola(int lados) { return 1 + sorte(lados); }
static int baixo;                        // quem aparece embaixo: o jogador local
static int zona(int j) { return j == baixo ? LINHA_BAIXO : LINHA_CIMA; }
// So para a conta da causa decisiva (partida.inc): o dado calado nao age -
// vale como um dado comum do mesmo tamanho.
static int8_t calado_j = -1, calado_k = -1;
static int tipo_comum(int lados)
{
    switch (lados) {
    case 2: return D_D2; case 4: return D_D4; case 6: return D_D6;
    case 8: return D_D8; case 12: return D_D12; default: return D_D20;
    }
}
static int tipo_de(int j, int k)
{
    if (j == calado_j && k == calado_k) return tipo_comum(TIPO[F[j].tipo[k]].lados);
    // O Camaleao tem o poder do dado logo antes dele, como se fosse ele (rola o
    // proprio d8: o lance usa F.tipo). Atras de um dado sem poder, e um d8.
    if (F[j].tipo[k] == D_CAMALEAO && k > 0 && k < N_FILA) {
        int a = tipo_de(j, k - 1);
        if (TIPO[a].efeito != EF_NADA) return a;
    }
    return F[j].tipo[k];
}

// O Relogio tira o numero da rodada (de 1 ate o maior lado dele).
static int relogio_valor(void) { return rodada < 1 ? 1 : rodada > RELOGIO_MAX ? RELOGIO_MAX : rodada; }

static void adesiva(peca_t *p, int (*r)(int), bool desafio);
static void colcha_remenda(peca_t *p);
static peca_t nova_peca(int tipo)
{
    peca_t p = { .tipo = (uint8_t)tipo, .fixo = 0 };
    if (tipo == D_QUEBRADO) p.fixo = (uint8_t)rola(8);
    if (tipo == D_ADESIVADO || tipo == D_MAIS_ADESIVADO) adesiva(&p, rola, modo == M_DESAFIO);   // ja vem com os adesivos
    if (tipo == D_COLCHA) colcha_remenda(&p);    // e a Colcha, com os tres Remendos
    return p;
}

// Texto centrado em cx; y e o topo da linha.
// Duas fontes com papeis fixos: 'bold' e a Jersey 10, encorpada, para
// titulos, nomes e o que importa; sem 'bold' e a fina (DotGothic16), para o
// texto corrido e as descricoes.
static int fonte_de(bool bold) { return bold ? 1 : GFX_FINO; }

static void txt_c(int cx, int y, const char *s, uint16_t c, bool bold)
{
    gfx_texto(cx - gfx_largura(s, fonte_de(bold)) / 2, y, s, c, fonte_de(bold), bold);
}

// O texto s passa de max_w (e o txt_c_duas o quebra em duas linhas)? E o
// passo de uma linha para a outra.
static bool txt_quebra(const char *s, int max_w) { return gfx_largura(s, fonte_de(false)) > max_w; }
static int txt_passo(void) { return gfx_letra_e_maior() ? 16 : 13; }

// Centrado em cx numa linha ou, se passar de max_w, em duas, quebrando onde
// as duas ficam mais parelhas; as duas ficam em volta de y (meia linha acima
// e meia abaixo), para nao invadir o que vem embaixo.
static void txt_c_duas(int cx, int y, const char *s, uint16_t c, int max_w)
{
    const int f = fonte_de(false);
    int n = (int)strlen(s), corte = -1, pior = 0;
    char a[200];
    if (gfx_largura(s, f) > max_w)
        for (int i = 1; i < n && i < (int)sizeof a; i++) {
            if (s[i] != ' ') continue;
            snprintf(a, sizeof a, "%.*s", i, s);
            int la = gfx_largura(a, f), lb = gfx_largura(s + i + 1, f), m = la > lb ? la : lb;
            if (corte < 0 || m < pior) { corte = i; pior = m; }
        }
    if (corte < 0) { gfx_texto(cx - gfx_largura(s, f) / 2, y, s, c, f, false); return; }
    int passo = txt_passo();
    snprintf(a, sizeof a, "%.*s", corte, s);
    gfx_texto(cx - gfx_largura(a, f) / 2, y - passo / 2, a, c, f, false);
    gfx_texto(cx - gfx_largura(s + corte + 1, f) / 2, y - passo / 2 + passo, s + corte + 1, c, f, false);
}
#define LARG_TEXTO (MESA_X1 - MESA_X0 - 24)    // a linha de texto mais larga que cabe na mesa

static void txt_c2(int cx, int y, const char *s, uint16_t c)
{
    gfx_texto(cx - gfx_largura(s, 2) / 2, y, s, c, 2, true);
}

static void txt(int x, int y, const char *s, uint16_t c, bool bold)
{
    gfx_texto(x, y, s, c, fonte_de(bold), bold);
}

// Texto com sombra de um pixel: legivel sobre o feltro trabalhado.
static void txt_sombra_c(int cx, int y, const char *s, uint16_t c, int esc)
{
    int x = cx - gfx_largura(s, esc) / 2;
    gfx_texto(x + esc, y + esc, s, C_SOMBRA, esc, true);
    gfx_texto(x, y, s, c, esc, true);
}
