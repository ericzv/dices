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
    if (tipo == D_QUEBRADO || tipo == D_FANTASMA || tipo == D_PREVISIVEL) return valor;
    if (tipo == D_VAZIO) return 0;
    if (tipo == D_LASTRO)   return 3 + rola_v(9);
    return rola_v(TIPO[tipo].lados);
}
// A epoca do Natal, pelo relogio do computador: novembro e dezembro. Online,
// pelo de Greenwich, para os dois lados sortearem dos mesmos dados. Le-se no
// comeco de cada partida (e na volta ao menu). natal_forcado: os testes e as
// cenas de amostra fixam a epoca (0 fora dela, 1 novembro, 2 dezembro, 3 do
// dia 26 em diante; -1, o relogio).
static int natal_forcado = -1;
static void le_epoca(void)
{
    int mes = -1, dia = 0;
    if (natal_forcado >= 0) {
        mes = natal_forcado == 0 ? 0 : natal_forcado == 1 ? 10 : 11;
        dia = natal_forcado == 3 ? 26 : 1;
    } else {
        time_t agora = time(NULL);
        const struct tm *m = modo == M_ONLINE ? gmtime(&agora) : localtime(&agora);
        if (m) { mes = m->tm_mon; dia = m->tm_mday; }
    }
    natal = mes == 10 || mes == 11;
    dezembro = mes == 11;
    reveillon = dezembro && dia >= 26;
}

static int outro(int j) { return j ^ 1; }
static int rola(int lados) { return 1 + sorte(lados); }
static int baixo;                        // quem aparece embaixo: o jogador local
static int zona(int j) { return j == baixo ? LINHA_BAIXO : LINHA_CIMA; }
static int tipo_de(int j, int k) { return F[j].tipo[k]; }

static peca_t nova_peca(int tipo)
{
    peca_t p = { .tipo = (uint8_t)tipo, .fixo = 0 };
    if (tipo == D_QUEBRADO) p.fixo = (uint8_t)rola(8);
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
