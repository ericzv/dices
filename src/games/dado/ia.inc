// ===========================================================================
// O aparelho como adversario
// ===========================================================================
// A IA joga pelas mesmas teclas que uma pessoa: poe o cursor onde quer e
// aperta ENTER ou TAB, com uma pausa antes de cada gesto para dar tempo de
// ver o que ela fez. So decide nas fases de escolha; o resto e animacao.
static void trata_tecla(int k);

static bool fase_de_escolha(void)
{
    return fase == F_LOJA || fase == F_ORDEM || fase == F_JOGA
        || fase == F_APOSTA || fase == F_PREMIO;
}

// No modo 2 jogadores, com dois controles, cada um so escolhe na sua vez.
int dado_vez_do_controle(void) { return modo == M_DOIS && fase_de_escolha() ? vez : -1; }

static bool ia_na_vez(void)
{
    return (cpu >> vez & 1) && fase_de_escolha();
}

// Valor medio que o dado mostra ao cair: e o que decide a queda da fila.
static float media(peca_t p)
{
    const tipo_t *t = &TIPO[p.tipo];
    switch (t->efeito) {
    case EF_QUEBRADO: return (int8_t)p.fixo;
    case EF_FANTASMA: return p.fixo ? (int8_t)p.fixo : 1;
    case EF_LASTRO:   return 8.0f;
    case EF_VICIADO:  return 4.47f;
    case EF_EXPLODE:  return 5.1f;
    case EF_MATRIOSKA: return 6.2f;                      // o d8 e as bonecas que abrem (6 a 8: d6; 5 ou 6: d4; 4: d2)
    case EF_COLCHA:   return 4.5f;                       // o 1, o 2 e o 3 remendados
    case EF_SORTE_REVES: return 6.5f - (SR_REVES + 1) / 2.0f;   // a Sorte (6,5) menos o Reves (2,5)
    case EF_PREVISIVEL: return previsivel_valor(p.fixo);   // o proximo da escada
    case EF_RELOGIO:  return relogio_valor();            // o numero da rodada
    case EF_DIGITAL:  return 21.0f;                      // o de antes +1: vai por ultimo
    case EF_FICHA_CORINGA: return 6.0f;                  // o ultimo do oponente, em geral um dos maiores
    case EF_VAZIO: case EF_SACRIFICIO: return 0;   // nao pontuam
    case EF_PRESENTE: return 4.5f;                       // vira um raro: em media, um d8
    case EF_PISCA:    return pisca_aceso(p.fixo) ? 5.0f : 0;     // aceso, x2; apagado, nada
    case EF_PANETONE: return 6.0f;                       // o d8 e as frutas (1,5 em media)
    case EF_MEIA:     return 2.5f + p.fixo;              // e o que guardou, se ficar
    case EF_BUMERANGUE: return 25.0f / 6;                // o 1 e o 2 voltam e rolam de novo: 4 x 4,5 + 2 x 3,5, em 6
    case EF_TELA: {                                      // so os lados com adesivo pontuam: o vazio da 0
        float s = 0;
        for (int l = 1; l <= t->lados; l++) if (ad_no_lado(&p, l)) s += remenda(&p, l);
        return s / t->lados;
    }
    default:          return (t->lados + 1) / 2.0f;
    }
}

// Dados que so atrapalham a fila de quem nao sabe o que esta fazendo.
static bool arriscado(int tipo)
{
    return tipo == D_MALDITO || tipo == D_TUDO_NADA || tipo == D_EGOISTA || tipo == D_SORTE_REVES;
}

// Escolha simples, sem imaginar jogadas: os de maior media, do menor para o
// maior (assim cada um tem boa chance de passar a barra do anterior), sem os
// arriscados quando da. E como a IA imagina a fila do rival, e a sua propria
// quando mede uma compra. Na mesma media, o dado com poder vai antes do liso:
// as mesmas faces e mais alguma coisa (o Espinhoso no lugar de um d4).
static bool com_poder(peca_t p) { return TIPO[p.tipo].efeito != EF_NADA; }
static int fila_simples(const peca_t *pc, int n, int *ordem)
{
    for (int i = 0; i < n; i++)                  // o Universo so vai sozinho, e vale mais que a fila
        if (pc[i].tipo == D_UNIVERSO) { ordem[0] = i; return 1; }
    int quer = n < FILA_NORMAL ? n : FILA_NORMAL, ok = 0;
    for (int i = 0; i < n; i++) if (!arriscado(pc[i].tipo)) ok++;
    bool sem_risco = ok >= quer;
    bool esc[MAX_COL + 1] = { false };
    int m = 0;
    for (int q = 0; q < quer; q++) {
        int b = -1;
        for (int i = 0; i < n; i++) {
            if (esc[i] || (sem_risco && arriscado(pc[i].tipo))) continue;
            float mi = media(pc[i]), mb = b < 0 ? 0 : media(pc[b]);
            if (b < 0 || mi > mb + 0.001f || (mi > mb - 0.001f && com_poder(pc[i]) && !com_poder(pc[b]))) b = i;
        }
        if (b < 0) break;
        esc[b] = true;
        ordem[m++] = b;
    }
    for (int a = 1; a < m; a++)
        for (int b = a; b > 0 && media(pc[ordem[b]]) < media(pc[ordem[b - 1]]); b--) {
            int t = ordem[b]; ordem[b] = ordem[b - 1]; ordem[b - 1] = t;
        }
    // O Dobro dobra o dado seguinte: por ultimo nao serve. Vai uma casa antes.
    for (int b = m - 1; b > 0; b--)
        if ((pc[ordem[b]].tipo == D_DOBRO || pc[ordem[b]].tipo == D_FERREIRO) && b == m - 1) {
            int t = ordem[b]; ordem[b] = ordem[b - 1]; ordem[b - 1] = t;
        }
    return m;
}

// ---- Rodadas imaginadas -----------------------------------------------------
// A IA decide jogando de mentira com as mesmas regras (aplica_regra e
// calcula_desfecho) sobre a mesa de verdade, que depois volta como estava.
// O sorteio e dela: pensar nao mexe nos dados da partida.
static uint32_t semente_ia = 0x2545F491u;
static uint32_t rnd_ia(void)
{
    semente_ia ^= semente_ia << 13; semente_ia ^= semente_ia >> 17; semente_ia ^= semente_ia << 5;
    return semente_ia;
}
static int rola_ia(int lados) { return 1 + (int)(rnd_ia() % (uint32_t)lados); }
static void semeia_ia(uint32_t base, int amostra)
{
    semente_ia = base ^ (2654435761u * (uint32_t)(amostra + 1));
    if (!semente_ia) semente_ia = 1;
}

// Tudo o que as regras escrevem durante uma jogada.
typedef struct {
    fila_t f[2];
    char ver[sizeof veredito];
    int vt, aa, ab, res, prim;
    uint8_t am;
    bool vc;
} mesa_t;

static void guarda_mesa(mesa_t *m)
{
    memcpy(m->f, F, sizeof F);
    memcpy(m->ver, veredito, sizeof veredito);
    m->vt = ver_tipo; m->aa = anulou_a; m->ab = anulou_b; m->res = resistiu;
    m->am = anulou_masc; m->prim = primeiro; m->vc = ver_comum;
}
static void volta_mesa(const mesa_t *m)
{
    memcpy(F, m->f, sizeof F);
    memcpy(veredito, m->ver, sizeof veredito);
    ver_tipo = m->vt; anulou_a = m->aa; anulou_b = m->ab; resistiu = m->res;
    anulou_masc = m->am; primeiro = m->prim; ver_comum = m->vc;
}

static void sim_lanca(int j)
{
    fila_t *f = &F[j];
    int k = f->lancados, menor;
    if (f->carta && k == 3) { f->lancados = 4; f->est[3] = V_ESPERA; return; }   // a carta: so posta
    // Com os adesivos do dado, como no lance de verdade (o Reflexivo rola de novo).
    const peca_t *pc = peca_fila(j, k);
    int fora;
    lance_t l = lanca_do_dado(j, k, f->tipo[k], f->fixo[k], pc, rola_ia, &fora);
    bool postit = k > 0 && ad_fila(j, k - 1) == AD_POSTIT;
    int casca = 0;                                   // a Casca do oponente, como no lance de verdade
    if (casca_pega(j, k)) {
        casca = (pc && pc->capa) || f->tipo[k] == D_UNIVERSO ? 2 : postit ? 3 : 1;
        if (casca == 3) postit = false;
    }
    // A Bussola no dado de antes: caindo sem adesivo (e tendo algum), rola de
    // novo; fica o segundo se cair num adesivo ou for maior.
    bool bussola = k > 0 && ad_fila(j, k - 1) == AD_BUSSOLA && pc && ad_quantos(pc) > 0 && !ad_no_lado(pc, l.lado);
    if (postit) {                                    // o Post-it: duas vezes, o maior
        lance_t l2 = lanca_do_dado(j, k, f->tipo[k], f->fixo[k], pc, rola_ia, &fora);
        if (l2.x > l.x) l = l2;
    } else if (bussola && casca != 1) {
        lance_t l2 = lanca_do_dado(j, k, f->tipo[k], f->fixo[k], pc, rola_ia, &fora);
        if (ad_no_lado(pc, l2.lado) || l2.x > l.x) l = l2;
    } else if (casca == 1) {                         // escorregou: duas vezes, o menor
        lance_t l2 = lanca_do_dado(j, k, f->tipo[k], f->fixo[k], pc, rola_ia, &fora);
        if (l2.x < l.x) l = l2;
    }
    f->bonecas[k] = l.bonecas;
    int x = l.x;
    menor = l.menor;
    f->cru[k] = f->valor[k] = x;
    f->lado[k] = (uint8_t)l.lado;
    f->bomba[k] = l.bomba;
    f->bomba1[k] = (int8_t)l.x1;
    f->descarte[k] = (uint8_t)menor;
    fatia(j, k, rola_ia);
    f->lancados = k + 1;
    pousa(j, k, rola_ia);
}

// Quem abre nao sabe o alvo: olha o risco do proximo dado. Pouco na mesa
// (um dado so, ou menos de 10 pontos) nao segura a rodada: o rival passa
// facil, entao so para diante de uma queda quase certa. O Egoista trocaria
// a fila inteira por um d12: so compensa com pouco na mesa.
static bool abre_continua(int j)
{
    const fila_t *f = &F[j];
    // A carta do Coringa vira, no fim, o dado seu que mais tirou - em geral
    // um dado grande. Vale arriscar se o ultimo dele que vale nao passa de 10.
    if (f->carta && f->lancados == 3) {
        int l = ultimo_da_regra(j, f->est, 3);
        return l < 0 || f->valor[l] <= 10;
    }
    if (tipo_de(j, f->lancados) == D_EGOISTA) return f->pontos < 6;
    if (agnus_armado(j) >= 0) return true;      // o Agnus com 7: o que cair agora volta no fim
    int r = risco(j);
    if (f->lancados == 1 || f->pontos < 10) return r < 85;
    return r < 40 || (r < 60 && f->pontos < 14);
}

// Placar de j com a mesa como esta: 1 vence, 0.5 empata, 0 perde. Com a
// margem, soma um pouco por ponto de diferenca (ate 20): entre jogadas que
// vencem igual, a IA prefere a que vence com folga, em vez de uma fila de
// moedas que so ganha porque o rival fez pouco.
#define IA_MARGEM 0.005f
#define IA_FICHAS 0.01f                 // cada ficha do Agiota e da Gorjeta, na nota da mao
static bool ia_sem_margem[2];                // so para medir a IA antiga nos testes
// O que ainda nao se ve na mesa, a IA imagina: o Oculto de cada lado tira um
// numero ao acaso (nem o dono sabe o dele) e entra na sequencia.
static void imagina_ocultos(void)
{
    for (int j = 0; j < 2; j++) {
        if (!tem_escondido(j)) continue;
        fila_t *f = &F[j];
        for (int k = 0; k < f->lancados && k < N_FILA; k++) {
            if (!f->escondido[k]) continue;
            int menor;
            f->cru[k] = f->valor[k] = sorteia_valor(f->tipo[k], f->fixo[k], rola_ia, &menor);
            f->lado[k] = (uint8_t)lado_do_lance(f->tipo[k], f->valor[k], menor);
            f->escondido[k] = false;
        }
        refaz_sequencia(j, f->lancados);
    }
}

static float nota_mesa(int j, bool margem)
{
    desfecho_t d;
    int c = carta_posta(0) ? 0 : carta_posta(1) ? 1 : -1;
    bool carta = c >= 0 && F[outro(c)].lancados, ocultos = tem_escondido(0) || tem_escondido(1);
    if (carta || ocultos) {                      // a carta do Coringa e o Oculto: imagina eles virando agora
        mesa_t m;
        guarda_mesa(&m);
        if (ocultos) imagina_ocultos();
        if (carta && vira_carta(c)) sim_lanca(c);
        calcula_desfecho(&d, false);
        volta_mesa(&m);
    } else calcula_desfecho(&d, false);
    int a = d.total[j], b = d.total[outro(j)];
    float r = a > b ? 1.0f : (a == b ? 0.5f : 0.0f);
    if (margem && !ia_sem_margem[j & 1]) {
        int m = a - b < -20 ? -20 : a - b > 20 ? 20 : a - b;
        r += IA_MARGEM * (float)m;
        // E as fichas que a rodada paga por fora (o Agiota e a Gorjeta), com
        // peso pequeno: so desempatam maos de margem parecida.
        int o = outro(j);
        r += IA_FICHAS * (float)(d.agiota[j] + d.gorjeta[j] - d.agiota[o] - d.gorjeta[o]);
    }
    return r;
}
static float placar(int j) { return nota_mesa(j, false); }

// Joga o turno de j ate o fim da fila ou ate parar. Quem joga depois para
// assim que passa na frente; quem abre segue o risco.
static void sim_turno(int j)
{
    fila_t *f = &F[j];
    while (f->lancados < f->n) {
        if (f->lancados > 0) {
            if (j != primeiro) { if (placar(j) == 1.0f) break; }
            else if (!abre_continua(j)) break;
        }
        sim_lanca(j);
    }
}

// Uma fila provavel para r. A mao de verdade a IA nao olha: sorteia uma entre
// os dados que ele ainda pode ter (ou a colecao toda, para medir compras) e
// escolhe pela regra simples.
static void fila_imaginada(int r, bool toda)
{
    jogador_t *p = &J[r];
    peca_t pool[MAX_COL];
    int8_t de[MAX_COL];                          // o lugar de cada um na bolsa (os adesivos dele)
    int n = 0, gi = -1;
    for (int i = 0; i < p->n_col; i++) if (p->col[i].chiclete && p->onde[i] != COMIDO) gi = i;   // o grudado abre a fila dele
    for (int i = 0; i < p->n_col; i++)
        if (i != gi && p->onde[i] != COMIDO && (toda || p->onde[i] != FORA)) { de[n] = (int8_t)i; pool[n++] = p->col[i]; }
    if (n < FILA_NORMAL) { n = 0; for (int i = 0; i < p->n_col; i++) if (i != gi) { de[n] = (int8_t)i; pool[n++] = p->col[i]; } }
    int m = n < tam_mao(r) ? n : tam_mao(r);
    for (int i = 0; i < m; i++) {
        int k = i + (int)(rnd_ia() % (uint32_t)(n - i));
        peca_t t = pool[i]; pool[i] = pool[k]; pool[k] = t;
        int8_t d = de[i]; de[i] = de[k]; de[k] = d;
    }
    int ordem[N_FILA], q = fila_simples(pool, m, ordem);
    fila_t *f = &F[r];
    memset(f, 0, sizeof *f);
    int o = 0;
    if (gi >= 0) {                               // o Chiclete: ja na mesa, com o numero dele
        f->idx[0] = (int8_t)gi; f->tipo[0] = p->col[gi].tipo; f->fixo[0] = p->col[gi].fixo;
        f->cru[0] = f->valor[0] = chic_valor[r];
        f->lado[0] = chic_lado[r];
        f->grudado = true;
        f->lancados = 1;
        aplica_regra(r, 0);
        o = 1;
        if (q > max_fila(r) - 1) q = max_fila(r) - 1;
    }
    for (int k = 0; k < q; k++) {
        f->idx[k + o] = de[ordem[k]];
        f->tipo[k + o] = pool[ordem[k]].tipo;
        f->fixo[k + o] = pool[ordem[k]].fixo;
    }
    f->n = q + o;
}

// Termina a rodada de mentira do ponto de vista de j e devolve o placar dele
// (com a margem, para comparar jogadas). Se o rival ainda nao jogou, ele joga
// uma fila imaginada.
static float sim_resto(int j, bool margem)
{
    int r = outro(j);
    if (j == primeiro) fila_imaginada(r, false);
    sim_turno(j);
    if (j == primeiro) sim_turno(r);
    return nota_mesa(j, margem);
}

#define IA_AMOSTRAS 240
static int ia_esforco = 1;                   // o simulador pede menos para andar rapido
// Nivel da IA: 0 facil (a IA antiga, de regras simples), 1 medio (a que
// imagina jogadas, mas pensando pouco: erra de vez em quando), 2 dificil.
static int ia_nivel = 2;
static int nivel_jog[2] = { 2, 2 };          // o nivel de cada lado (so conta onde ha IA; declarado em estado.inc)
// No Desafiante, o oponente do comeco da run pensa um nivel acima numa parte
// das decisoes (pensa_mesa): pensa_jog, de 0 a 99 (%). O sorteio de cada
// decisao sai de um contador da partida e da mesa, sem gastar o sorteio do
// jogo (os lances ficam os mesmos).
static int pensa_jog[2];
static uint32_t ia_conta;                    // as decisoes da partida
static bool ia_sobe;                         // esta decisao, um nivel acima
static float ousadia_jog[2];
// Ousadia nas apostas, de -1 (cautela) a +1 (aposta alto): o jeito de cada
// oponente do Modo Desafiante. 0 na partida rapida.
static float ia_ousadia;
// Quantas rodadas de mentira cabem numa decisao, pelo nivel.
static int amostras_ia(int cheio, int pouco)
{
    if (ia_esforco == 0) return pouco;
    if (ia_nivel == 1) return cheio / 6 > pouco ? cheio / 6 : pouco;
    return cheio;
}

// O que os dados lancados por j guardam para as proximas rodadas, em pontos:
// a bala do Revolver (o tiro dividido pelo tambor), a Meia anulada que
// guarda, os Tres Patinhos andando para o x3, a Onda valendo que enche para
// virar Tsunami, a copia do Jingle Bell valendo que entra na bolsa. Nada disso
// aparece no placar da rodada, entao entra so na margem: entre jogadas que
// vencem igual, a IA prefere a que deixa alguma coisa guardada (e usa esses
// dados, em vez de um d6 comum que vale o mesmo agora).
static float ia_futuro(int j)
{
    const fila_t *f = &F[j];
    float b = 0;
    for (int k = 0; k < f->lancados && k < N_FILA; k++) {
        bool vale = f->est[k] == V_VALIDO;
        switch (TIPO[f->tipo[k]].efeito) {
        case EF_REVOLVER: if (!revolver_dispara(f->fixo[k])) b += (float)REVOLVER_TIRO / REVOLVER_BALAS; break;
        case EF_MEIA:     if (!vale) b += MEIA_GUARDA; break;
        case EF_PATINHOS: if (!patinhos_ativo(f->fixo[k])) b += 1.5f; break;
        case EF_ONDA:     if (vale) b += 0.5f * f->valor[k]; break;
        case EF_BALAO:    if (!vale && f->tipo[k] == D_BALAO) b += BALAO_CADA; break;   // anulado, enche mais
        case EF_JINGLE:   if (vale) b += 1.0f; break;
        default: break;
        }
    }
    return b;
}
static float ia_futuro_nota(int j) { return ia_sem_margem[j & 1] ? 0 : IA_MARGEM * ia_futuro(j); }

// ---- A fila ----------------------------------------------------------------
// Testa toda ordem de ate quatro dados da mao, jogando cada uma varias vezes
// contra filas imaginadas do rival (ou contra a fila que ele ja jogou). Assim
// as sinergias (Dobro antes do maior, Fartura com dados baixos, Copiador,
// Jackpot...) aparecem sozinhas, sem regra escrita para cada uma.

static float avalia_fila(int j, const int8_t *seq, int L, int amostras, uint32_t base,
                         const mesa_t *m)
{
    float s = 0;
    for (int a = 0; a < amostras; a++) {
        volta_mesa(m);
        semeia_ia(base, a);
        fila_t *f = &F[j];
        int k0 = f->grudado ? 1 : 0;             // o grudado ja esta pousado no primeiro lugar
        if (!k0) memset(f, 0, sizeof *f);
        for (int k = k0; k < L; k++) {
            peca_t pc = J[j].col[J[j].mao[seq[k]]];
            f->idx[k] = J[j].mao[seq[k]];
            f->tipo[k] = pc.tipo;
            f->fixo[k] = pc.fixo;
        }
        f->n = L;
        if (L == 3) coringa_completa(j);       // o quarto do Coringa: a copia, ou a carta
        s += sim_resto(j, true);
        s += ia_futuro_nota(j);
    }
    return s / (float)amostras;
}

#define MAX_CAND 2520                        // 7 x 6 x 5 x 4 x 3 (a fila de 5 do Estojo de Lapis)
_Static_assert(MAO <= 16 && N_FILA * 4 <= 64, "a assinatura da fila: um lugar da mao em 4 bits");
static void ia_planeja(void)
{
    int j = vez;
    if (plano_rodada == rodada && plano_vez == j) return;
    jogador_t *p = &J[j];
    int n = p->n_mao, L = n < max_fila(j) ? n : max_fila(j);
    plano_rodada = rodada; plano_vez = j; plano_n = 0; plano_p = 0.5f;
    if (L <= 0) return;
    int gm = -1;                                 // o grudado pelo Chiclete: o primeiro de toda fila
    for (int m = 0; m < n && F[j].grudado; m++) if (p->mao[m] == F[j].idx[0]) gm = m;

    static int8_t cand[MAX_CAND][N_FILA];
    static float nota_c[MAX_CAND];
    static uint64_t assin[MAX_CAND];
    int nc = 0;
    // Todas as sequencias de L posicoes diferentes da mao. Sequencias com os
    // mesmos dados na mesma ordem (dois d6 trocados) contam uma vez so. Igual
    // e igual de tudo: o tipo, o numero guardado, os adesivos e as marcas
    // (um d6 com adesivo nao e um d6 liso). Cada lugar da mao fica com o
    // primeiro igual a ele, e a fila e a lista desses lugares.
    int8_t igual[MAO];
    for (int m = 0; m < n; m++) {
        igual[m] = (int8_t)m;
        for (int q = 0; q < m; q++)
            if (!memcmp(&p->col[p->mao[q]], &p->col[p->mao[m]], sizeof(peca_t))) { igual[m] = (int8_t)q; break; }
    }
    int8_t s[N_FILA] = { 0 };
    int nivel = 0;
    s[0] = -1;
    while (nivel >= 0) {
        s[nivel]++;
        if (s[nivel] >= n) { nivel--; continue; }
        bool rep = false;
        for (int i = 0; i < nivel; i++) if (s[i] == s[nivel]) rep = true;
        if (rep || (gm >= 0 && (nivel == 0) != (s[nivel] == gm))) continue;   // o grudado so no primeiro lugar
        if (nivel + 1 < L) { nivel++; s[nivel] = -1; continue; }
        // Dobro por ultimo nao dobra nada: a IA nem considera. O Universo so
        // vai sozinho: entra no fim, como fila de um dado so.
        if (L >= 2 && p->col[p->mao[s[L - 1]]].tipo == D_DOBRO) continue;   // (o Ferreiro pontua: por ultimo ainda vale)
        bool uni = false;
        for (int k = 0; k < L && L >= 2; k++) uni |= p->col[p->mao[s[k]]].tipo == D_UNIVERSO;
        if (uni) continue;
        uint64_t a = 0;
        for (int k = 0; k < L; k++) a = a * 16 + (uint64_t)igual[s[k]];   // MAO < 16
        bool visto = false;
        for (int c = 0; c < nc && !visto; c++) visto = assin[c] == a;
        if (visto || nc >= MAX_CAND) continue;
        assin[nc] = a;
        memcpy(cand[nc++], s, N_FILA);
    }

    mesa_t m;
    guarda_mesa(&m);
    uint32_t base = 0xA5A5u + (uint32_t)rodada * 7919u + (uint32_t)j * 104729u;
    // Primeira peneira com poucas jogadas; as melhores jogam de novo, muito mais.
    int a1 = nc > 60 ? 24 : 64;
    if (ia_esforco == 0) a1 = 4;
    else if (ia_nivel == 1) a1 = nc > 60 ? 24 : 32;
    for (int c = 0; c < nc; c++) nota_c[c] = avalia_fila(j, cand[c], L, a1, base, &m);
    int finalistas = ia_esforco == 0 ? 2 : (ia_nivel == 1 ? 4 : 8);
    int a2 = amostras_ia(IA_AMOSTRAS, 24);
    int melhor = -1;
    float nota_m = -1;
    for (int f = 0; f < finalistas && f < nc; f++) {
        int b = -1;
        for (int c = 0; c < nc; c++) if (nota_c[c] > -1 && (b < 0 || nota_c[c] > nota_c[b])) b = c;
        if (b < 0) break;
        nota_c[b] = -2;                      // ja foi finalista
        float v = avalia_fila(j, cand[b], L, a2, base ^ 0x5bd1e995u, &m);
        if (v > nota_m) { nota_m = v; melhor = b; }
    }
    // A fila pode ser mais curta: se so o comeco dela ja e melhor (o
    // Solitario sozinho, um dado grande e parar), a IA monta so esse comeco,
    // em vez de por quatro dados na mesa e parar no primeiro.
    int curta = L;
    for (int q = 1; q < L && melhor >= 0; q++) {
        float v = avalia_fila(j, cand[melhor], q, a2, base ^ 0x5bd1e995u, &m);
        if (v > nota_m + 0.01f) { nota_m = v; curta = q; }
    }
    if (melhor >= 0) { memcpy(plano, cand[melhor], N_FILA); plano_n = curta; }
    // O Universo, sozinho, contra a melhor fila sem ele (com um grudado na
    // mesa, nao da: ele so vai sozinho).
    for (int i = 0; i < n && L >= 2 && gm < 0; i++) {
        if (p->col[p->mao[i]].tipo != D_UNIVERSO) continue;
        int8_t so[N_FILA] = { (int8_t)i };
        float v = avalia_fila(j, so, 1, a2, base ^ 0x5bd1e995u, &m);
        if (melhor < 0 || v > nota_m) { nota_m = v; melhor = i; memcpy(plano, so, N_FILA); plano_n = 1; }
        break;
    }
    volta_mesa(&m);
    if (melhor < 0) return;
    plano_p = nota_m < 0 ? 0 : nota_m > 1 ? 1 : nota_m;   // a chance, sem a margem passar de 0 a 1
}

static int ia_proximo_da_fila(void)
{
    ia_planeja();
    int k = F[vez].n;
    return k < plano_n ? plano[k] : -1;
}

// ---- Lancar ou parar -----------------------------------------------------
// Compara as duas coisas jogando de mentira: parar agora (e o rival
// responder, se ainda nao jogou) contra lancar mais um e seguir. Um empate
// certo nao vira derrota certa, como lancar um Quebrado que anula o seu 6.
static bool ia_continua(void)
{
    int j = vez;
    fila_t *f = &F[j];
    if (f->lancados >= f->n) return false;
    if (f->lancados == 0) return true;
    mesa_t m;
    guarda_mesa(&m);
    int amostras = amostras_ia(IA_AMOSTRAS, 24);
    uint32_t base = 0xC0FFEEu + (uint32_t)rodada * 31u + (uint32_t)f->lancados;
    float parar = 0, seguir = 0;
    for (int a = 0; a < amostras; a++) {
        volta_mesa(&m);
        semeia_ia(base, a);
        F[j].n = F[j].lancados;              // parar: a fila acaba aqui
        parar += sim_resto(j, true) + ia_futuro_nota(j);
        volta_mesa(&m);
        semeia_ia(base, a);
        sim_lanca(j);
        seguir += sim_resto(j, true) + ia_futuro_nota(j);
    }
    volta_mesa(&m);
    parar /= (float)amostras;
    seguir /= (float)amostras;
    // O Quebrado some da colecao quando e jogado: so vale a pena se ajudar.
    if (tipo_de(j, f->lancados) == D_QUEBRADO) seguir -= 0.02f;
    return seguir > parar + 0.005f;
}

// ---- Aposta ----------------------------------------------------------------
// Chance de vencer a rodada, vista por quem esta na vez.
static float ia_chance(void)
{
    int j = vez;
    if (F[j].lancados == 0) {                    // ainda vai jogar
        // O Chiclete dele so gruda quando a vez chega (a aposta vem antes):
        // pesa a fila ja com ele no primeiro lugar, numa copia da mesa, e
        // na vez de verdade a fila e planejada de novo.
        int g = F[j].n ? -1 : chiclete_na_mao(j);
        if (g < 0) { ia_planeja(); return plano_p; }
        mesa_t m;
        guarda_mesa(&m);
        poe_grudado(j, g);
        plano_rodada = -1;
        ia_planeja();
        float p = plano_p;
        volta_mesa(&m);
        plano_rodada = -1;
        return p;
    }
    mesa_t m;
    guarda_mesa(&m);
    int amostras = amostras_ia(IA_AMOSTRAS, 24);
    float s = 0;
    for (int a = 0; a < amostras; a++) {
        volta_mesa(&m);
        semeia_ia(0xBE77u + (uint32_t)rodada, a);
        F[j].n = F[j].lancados;
        s += sim_resto(j, false);
    }
    volta_mesa(&m);
    return s / (float)amostras;
}

static int ia_aposta(void)
{
    int op[4], n = opcoes(op);
    float p = ia_chance();
    // A ousadia desloca as barras: quem e ousado aposta e dobra com menos
    // chance, e blefa mais; o cauteloso so aposta quando esta bem.
    float o = ia_ousadia * 0.12f;
    int quer;
    if (!a_pagar) {
        quer = p >= 0.72f - o ? OP_APOSTAR25 : (p >= 0.58f - o ? OP_APOSTAR10 : OP_PASSAR);
        int blefe = ia_ousadia > 0.5f ? 4 : (ia_ousadia < -0.5f ? 20 : 8);
        if (quer == OP_PASSAR && p < 0.35f && rnd_ia() % (uint32_t)blefe == 0) quer = OP_APOSTAR10;
    } else {
        // Pagar 'a_pagar' para disputar o pote, que ja tem a aposta do oponente.
        float precisa = (float)a_pagar / (float)(pote + a_pagar);
        if (p >= 0.75f - o) quer = OP_AUMENTAR;
        else quer = p >= precisa - o * 0.5f ? OP_PAGAR : OP_CORRER;
    }
    for (int i = 0; i < n; i++) if (op[i] == quer) return i;
    for (int i = 0; i < n; i++) if (op[i] == OP_PAGAR || op[i] == OP_PASSAR) return i;
    return 0;
}

// ---- Loja e premio -----------------------------------------------------------
// Forca da colecao de j, com um dado a mais (ou nenhum, extra < 0): chance
// media de vencer rodadas imaginadas, com maos sorteadas da colecao toda,
// contra a colecao do rival, metade abrindo e metade respondendo. Pega as
// sinergias com o que ja tem e tambem a diluicao: dado fraco atrapalha.
static float forca_colecao(int j, int extra, int fixo_extra, uint32_t base)
{
    jogador_t *p = &J[j];
    peca_t pool[MAX_COL + 1];
    int n = 0;
    for (int i = 0; i < p->n_col; i++) pool[n++] = p->col[i];
    if (extra >= 0) pool[n++] = (peca_t){ .tipo = (uint8_t)extra, .fixo = (uint8_t)fixo_extra };
    mesa_t m;
    guarda_mesa(&m);
    int amostras = amostras_ia(600, 40), r = outro(j);
    float s = 0;
    for (int a = 0; a < amostras; a++) {
        volta_mesa(&m);
        semeia_ia(base, a);
        primeiro = a & 1 ? j : r;
        fila_imaginada(r, true);
        peca_t mao[MAX_COL + 1];
        memcpy(mao, pool, sizeof pool);
        int q = n < tam_mao(j) ? n : tam_mao(j);
        for (int i = 0; i < q; i++) {
            int k = i + (int)(rnd_ia() % (uint32_t)(n - i));
            peca_t t = mao[i]; mao[i] = mao[k]; mao[k] = t;
        }
        int ordem[N_FILA], L = fila_simples(mao, q, ordem);
        fila_t *f = &F[j];
        memset(f, 0, sizeof *f);
        for (int k = 0; k < L; k++) { f->tipo[k] = mao[ordem[k]].tipo; f->fixo[k] = mao[ordem[k]].fixo; }
        f->n = L;
        sim_turno(primeiro);
        sim_turno(outro(primeiro));
        s += placar(j);
    }
    volta_mesa(&m);
    return s / (float)amostras;
}

// Loja: compra o que mais sobe a chance de vencer, pesando o preco: um ponto
// de chance a mais em cada rodada que falta vale mais ou menos um quarto do
// pote medio. Guarda sempre fichas para as apostas.
static int ia_compra(void)
{
    int j = vez;
    if (J[j].n_col >= lim_col()) return -1;
    uint32_t base = 0x51ED27u + (uint32_t)rodada * 977u + (uint32_t)J[j].n_col;
    float sem = forca_colecao(j, -1, 0, base);
    int faltam = MAX_RODADAS - rodada + 1;
    if (faltam < 1) faltam = 1;
    int b = -1;
    float melhor = 0;
    for (int i = 0; i < N_LOJA; i++) {
        int pr = TIPO[loja[i].tipo].preco;
        if (levou[j][i] || J[j].fichas - pr < 60) continue;
        float ganho = forca_colecao(j, loja[i].tipo, loja[i].fixo, base) - sem;
        float valor = ganho * (float)faltam * 25.0f - (float)pr;
        if (valor > melhor) { melhor = valor; b = i; }
    }
    return b;
}

// Premio: o que mais fortalece a colecao.
static int ia_premio(void)
{
    int j = vez, b = 0;
    uint32_t base = 0x9E37u + (uint32_t)rodada * 131u;
    float melhor = -1;
    for (int i = 0; i < 3; i++) {
        if (oferta_ad[i]) continue;              // (a maquina nao ganha adesivo: so dados)
        float v = forca_colecao(j, oferta[i].tipo, oferta[i].fixo, base);
        if (v > melhor) { melhor = v; b = i; }
    }
    return b;
}

// ---- A IA antiga, do nivel facil -----------------------------------------------
// Regras fixas, sem imaginar jogadas: a fila pelas medias, parar pelo risco,
// apostar pelos pontos na mesa, comprar e escolher premio pela raridade.
static int facil_proximo_da_fila(void)
{
    jogador_t *p = &J[vez];
    peca_t mao[MAO];
    int de[MAO], n = 0, o = F[vez].grudado ? 1 : 0;   // sem o grudado, que ja abriu a fila
    for (int i = 0; i < p->n_mao; i++)
        if (!o || p->mao[i] != F[vez].idx[0]) { de[n] = i; mao[n++] = p->col[p->mao[i]]; }
    int ordem[N_FILA], q = fila_simples(mao, n, ordem);
    if (q > max_fila(vez) - o) q = max_fila(vez) - o;
    int k = F[vez].n - o;
    return k >= 0 && k < q ? de[ordem[k]] : -1;
}

static bool facil_continua(void)
{
    fila_t *f = &F[vez];
    if (f->lancados >= f->n) return false;
    if (f->lancados == 0) return true;
    if (vez != primeiro) {                     // parar agora ganha?
        desfecho_t d;
        calcula_desfecho(&d, false);
        return d.total[vez] <= d.total[outro(vez)];
    }
    return abre_continua(vez);
}

static int facil_aposta(void)
{
    int op[4], n = opcoes(op);
    int pts = F[primeiro].pontos;
    bool jogou = vez == primeiro;          // a IA ja tem pontos na mesa?
    int quer;
    if (!a_pagar) {
        if (jogou) quer = pts >= 20 ? OP_APOSTAR25 : (pts >= 13 ? OP_APOSTAR10 : OP_PASSAR);
        else       quer = pts <= 5 ? OP_APOSTAR10 : OP_PASSAR;
    } else if (jogou) {
        quer = pts <= 4 ? OP_CORRER : OP_PAGAR;
    } else {
        quer = (pts >= 22 && a_pagar >= 20) ? OP_CORRER
             : (pts <= 5 ? OP_AUMENTAR : OP_PAGAR);
    }
    for (int i = 0; i < n; i++) if (op[i] == quer) return i;
    for (int i = 0; i < n; i++) if (op[i] == OP_PAGAR || op[i] == OP_PASSAR) return i;
    return 0;
}

static int facil_compra(void)
{
    int b = -1;
    for (int i = 0; i < N_LOJA; i++) {
        int pr = TIPO[loja[i].tipo].preco;
        if (levou[vez][i] || J[vez].fichas - pr < 75 || J[vez].n_col >= lim_col()) continue;
        if (b < 0 || TIPO[loja[i].tipo].raridade > TIPO[loja[b].tipo].raridade
            || (TIPO[loja[i].tipo].raridade == TIPO[loja[b].tipo].raridade
                && pr > TIPO[loja[b].tipo].preco))
            b = i;
    }
    return b;
}

static int facil_premio(void)
{
    int b = 0;
    while (b < 2 && oferta_ad[b]) b++;
    for (int i = b + 1; i < 3; i++)
        if (!oferta_ad[i] && TIPO[oferta[i].tipo].raridade > TIPO[oferta[b].tipo].raridade) b = i;
    return b;
}

static void ia_passo(float dt)
{
    ia_nivel = nivel_jog[vez & 1];
    ia_ousadia = ousadia_jog[vez & 1];
    // A Banqueira: aposta alto quando esta na frente em fichas e corre cedo
    // quando esta atras.
    if (modo == M_DESAFIO && (vez & 1) == 1 && estilo_op == 1) ia_ousadia = J[1].fichas > J[0].fichas ? 1.0f : -1.0f;
    if (!ia_na_vez() || (fase == F_PREMIO && (t_tira > 0 || slot_girando()))) { ia_fase = -1; return; }
    if (t_anuncio > 0) return;                // a pessoa ainda le o que ela fez
    if (fase != ia_fase || vez != ia_vez) {   // nova decisao: pensa um pouco
        ia_fase = fase; ia_vez = vez;
        t_ia = fase == F_ORDEM && !anuncia_maquina() ? -0.4f : 0;
        int p = modo == M_DESAFIO ? pensa_jog[vez & 1] : 0;
        uint32_t h = ++ia_conta * 2654435761u ^ (uint32_t)rodada * 40503u ^ (uint32_t)fase * 9973u
                   ^ (uint32_t)(F[vez].lancados * 16 + F[vez].n) * 131u ^ (uint32_t)J[vez].fichas * 7919u;
        h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
        ia_sobe = p > 0 && (int)(h % 100u) < p;
    }
    if (ia_sobe && ia_nivel < 2) ia_nivel++;
    t_ia += dt;
    // Contra uma pessoa, a loja e o premio da maquina nao tem tela: ela
    // decide na hora, e o anuncio mostra o que levou.
    bool direto = anuncia_maquina() && (fase == F_LOJA || fase == F_PREMIO);
    if (!direto && t_ia < (fase == F_JOGA ? 0.3f : 0.55f)) return;
    t_ia = 0;

    if (direto && fase == F_LOJA) {
        peca_t levadas[N_LOJA];
        int n = 0;
        for (int k = 0; k < N_LOJA; k++) {
            int i = ia_nivel == 0 ? facil_compra() : ia_compra();
            if (i < 0) break;
            int antes = J[vez].n_col;
            cursor = i;
            trata_tecla(KEY_ENTER);
            if (J[vez].n_col == antes) break;
            levadas[n++] = loja[i];
        }
        anuncia_pecas(n ? "%s comprou" : "%s não comprou nada", levadas, n);
        trata_tecla(KEY_TAB);
        return;
    }
    if (direto && fase == F_PREMIO) {
        int i = ia_nivel == 0 ? facil_premio() : ia_premio();
        peca_t p = oferta[i];
        bool cabe = J[vez].n_col < lim_col();
        cursor = i;
        anuncia_pecas(cabe ? "%s levou o prêmio" : "%s ficou sem prêmio", &p, cabe ? 1 : 0);
        trata_tecla(KEY_ENTER);
        return;
    }

    switch (fase) {
    case F_LOJA: {
        int i = ia_nivel == 0 ? facil_compra() : ia_compra();
        if (i >= 0) { cursor = i; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    }
    case F_ORDEM: {
        // Contra uma pessoa, monta a sequencia inteira de uma vez: o menu
        // dela nao aparece, so os dados indo para a fila.
        int vezes = anuncia_maquina() ? N_FILA + 1 : 1;
        for (int n = 0; n < vezes && fase == F_ORDEM; n++) {
            int i = ia_nivel == 0 ? facil_proximo_da_fila() : ia_proximo_da_fila();
            if (i >= 0) { cursor = i; trata_tecla(KEY_ENTER); }
            else { trata_tecla(KEY_TAB); break; }
        }
        break;
    }
    case F_JOGA:
        if (ia_nivel == 0 ? facil_continua() : ia_continua()) { cursor = 0; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    case F_APOSTA:
        cursor = ia_nivel == 0 ? facil_aposta() : ia_aposta();
        trata_tecla(KEY_ENTER);
        break;
    case F_PREMIO:
        cursor = ia_nivel == 0 ? facil_premio() : ia_premio();
        trata_tecla(KEY_ENTER);
        break;
    }
}
