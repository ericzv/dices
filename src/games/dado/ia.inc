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
    case EF_PISCA:    return pisca_aceso(p.fixo) ? 5.0f : 2.5f;  // aceso, x2; apagado, um d4
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

// O que um dado de poder vale de verdade na mao, em decimos de "ponto de
// dado liso": a chance de vencer com ele na fila (jogando bem, contra a mao
// do comeco: um d6 a mais e dois d4), convertida na media do dado liso que
// daria a mesma chance (o d4 vale 2,5, o d8 4,5, o d12 6,5, o d20 10,5).
// So os que valem mais do que a media das faces diz; o resto fica pela
// media. Medido com tests de fora do repo (a mao {X, d6 x4, d4 x2}, 600
// jogadas por fila); refazer quando um poder mudar.
static const uint8_t VALOR_IA[D_N] = {
    [D_DOBRO] = 38,           // Dobro: +0.084 (a media diz 3.5)
    [D_LASTRO] = 104,          // Lastro: +0.395 (a media diz 8.0)
    [D_PAR] = 29,             // Par: +0.023 (a media diz 2.5)
    [D_ESCUDO] = 33,          // Escudo: +0.055 (a media diz 2.5)
    [D_MARTELO] = 32,         // Espinhoso: +0.047 (a media diz 2.5)
    [D_PIRATA] = 49,          // Pirata: +0.153 (a media diz 4.5)
    [D_TUDO_NADA] = 34,       // Tudo ou Nada: +0.056 (a media diz 1.5)
    [D_GATUNO] = 38,          // Gatuno: +0.082 (a media diz 3.5)
    [D_INVEJOSO] = 40,        // Invejoso: +0.096 (a media diz 3.5)
    [D_CARIDOSO] = 35,        // Caridoso: +0.065 (a media diz 2.5)
    [D_FARTURA] = 36,         // Fartura: +0.072 (a media diz 2.5)
    [D_SORTE] = 42,           // Moeda sorte: +0.111 (a media diz 1.5)
    [D_TEIMOSO] = 43,         // Teimoso: +0.120 (a media diz 3.5)
    [D_FOGO] = 58,            // Ficha Fogo: +0.209 (a media diz 1.5)
    [D_MALDICAO] = 40,        // Maldito: +0.098 (a media diz 2.5)
    [D_TREVO] = 30,           // Trevo: +0.031 (a media diz 2.5)
    [D_JACKPOT] = 33,         // Jackpot: +0.052 (a media diz 2.5)
    [D_LASER] = 42,           // Laser: +0.111 (a media diz 3.5)
    [D_MISERICORDIOSO] = 46,  // Misericordioso: +0.137 (a media diz 2.5)
    [D_SOLITARIO] = 40,       // Solitário: +0.101 (a media diz 2.5)
    [D_SENTINELA] = 31,       // Sentinela: +0.041 (a media diz 2.5)
    [D_FERREIRO] = 28,        // Ferreiro: +0.018 (a media diz 2.5)
    [D_DESAFIANTE] = 52,      // Desafiante: +0.172 (a media diz 4.5)
    [D_SEGURANCA] = 35,       // Segurança: +0.067 (a media diz 2.5)
    [D_GEMEO] = 56,           // Gêmeo: +0.199 (a media diz 3.5)
    [D_PRISMA] = 57,          // Prisma: +0.204 (a media diz 4.5)
    [D_FENIX] = 45,           // Fênix: +0.131 (a media diz 3.5)
    [D_BERSERKER] = 126,       // Berserker: +0.472 (a media diz 10.5)
    [D_VIUVA] = 59,           // Viúva Negra: +0.214 (a media diz 4.5)
    [D_CAPITAO] = 57,         // Capitão: +0.205 (a media diz 4.5)
    [D_ECO] = 56,             // Eco: +0.199 (a media diz 3.5)
    [D_ANCORA] = 42,          // Âncora: +0.113 (a media diz 2.5)
    [D_MARTIR] = 44,          // Mártir: +0.125 (a media diz 2.5)
    [D_COVEIRO] = 43,         // Coveiro: +0.117 (a media diz 3.5)
    [D_HEXA] = 42,            // Hexa: +0.114 (a media diz 3.5)
    [D_CALIBRE] = 33,         // Calibre: +0.054 (a media diz 2.5)
    [D_PONTE] = 42,           // Ponte: +0.114 (a media diz 2.5)
    [D_GARIMPEIRO] = 43,      // Garimpeiro: +0.115 (a media diz 3.5)
    [D_OCULTO] = 57,          // Oculto: +0.205 (a media diz 4.5)
    [D_SACRIFICIO] = 42,      // Sacrifício: +0.112 (a media diz 0.0)
    [D_VAZIO] = 40,           // Vazio: +0.101 (a media diz 0.0)
    [D_MEDIA] = 36,           // Média: +0.073 (a media diz 2.5)
    [D_VAMPIRO] = 46,         // Vampiro: +0.138 (a media diz 2.5)
    [D_ASSOMBRACAO] = 42,     // Assombração: +0.109 (a media diz 3.5)
    [D_FICHA_VELHA] = 27,     // Ficha Velha: +0.016 (a media diz 1.5)
    [D_TRENO] = 54,           // Trenó: +0.184 (a media diz 4.5)
    [D_TSUNAMI] = 71,         // Tsunami: +0.273 (a media diz 6.5)
    [D_ESTRELA] = 58,         // Estrela de Belém: +0.208 (a media diz 4.5)
    [D_CONSTELACAO] = 54,     // Constelação: +0.187 (a media diz 4.5)
    [D_GALAXIA] = 74,         // Galáxia: +0.286 (a media diz 6.5)
    [D_AGNUS] = 51,           // Agnus: +0.167 (a media diz 4.5)
};
// O valor do dado para escolher a fila simples: o medido ou a media.
static float valor_ia(peca_t p)
{
    float m = media(p), v = VALOR_IA[p.tipo] / 10.0f;
    return v > m ? v : m;
}

// No empate de valor: o dado de poder antes do liso, o com adesivo antes do
// sem, e o com Sujeira (o barro do oponente, -2 no placar) por ultimo. O
// Chumbo puxa o dado para o lado dele: so conta a favor num lado acima da
// media (num lado baixo, conta contra).
static int preferencia_ia(peca_t p)
{
    int s = TIPO[p.tipo].efeito != EF_NADA ? 2 : 0;
    for (int l = 1; l <= AD_LADOS; l++) {
        int a = ad_no_lado(&p, l);
        if (a == AD_SUJEIRA) s -= 2;
        else if (a == AD_CHUMBO) s += (float)l > media(p) ? 1 : -1;
        else if (a) s++;
    }
    return s;
}

// Dados cujo poder depende de um dado antes deles na fila: abrindo a fila, o
// poder nao tem como agir (o Par nao tem com quem empatar, a Sentinela nao
// tem quem proteger, o Treno nao tem quem puxar). Nunca abrem a fila quando
// ha outro dado para abrir.
static bool precisa_anterior(int tipo)
{
    switch (tipo) {
    case D_PAR: case D_SENTINELA: case D_GEMEO: case D_ECO: case D_DIGITAL:
    case D_VENTANIA: case D_TRENO: case D_CAMALEAO:
        return true;
    default:
        return false;
    }
}
// E os que so agem abrindo a fila: o Puxa-fila (o primeiro, com os quatro
// valendo) e o Checkpoint (pontua por quem vem depois dele).
static bool quer_abrir(int tipo) { return tipo == D_PUXA_FILA || tipo == D_CHECKPOINT; }

// Dados que so atrapalham a fila de quem nao sabe o que esta fazendo.
static bool arriscado(int tipo)
{
    return tipo == D_MALDITO || tipo == D_TUDO_NADA || tipo == D_EGOISTA || tipo == D_SORTE_REVES;
}

// Escolha simples, sem imaginar jogadas: os de maior valor (valor_ia: o
// medido dos dados de poder, ou a media), do menor para o maior pela media
// (assim cada um tem boa chance de passar a barra do anterior), sem os
// arriscados quando da (evita_risco: o nivel facil e a fila imaginada do
// rival; a compra mede a mao de quem joga bem, e nao evita). E como a IA
// imagina a fila do rival, e a sua propria quando mede uma compra. No mesmo
// valor, preferencia_ia: o dado com poder, depois o com adesivo, antes do
// liso (o Espinhoso no lugar de um d4); o com Sujeira por ultimo.
static int fila_simples_r(const peca_t *pc, int n, int *ordem, bool evita_risco)
{
    for (int i = 0; i < n; i++)                  // o Universo so vai sozinho, e vale mais que a fila
        if (pc[i].tipo == D_UNIVERSO) { ordem[0] = i; return 1; }
    int quer = n < FILA_NORMAL ? n : FILA_NORMAL, ok = 0;
    for (int i = 0; i < n; i++) if (!arriscado(pc[i].tipo)) ok++;
    bool sem_risco = evita_risco && ok >= quer;
    bool esc[MAX_COL + 1] = { false };
    int m = 0;
    for (int q = 0; q < quer; q++) {
        int b = -1;
        for (int i = 0; i < n; i++) {
            if (esc[i] || (sem_risco && arriscado(pc[i].tipo))) continue;
            float mi = valor_ia(pc[i]), mb = b < 0 ? 0 : valor_ia(pc[b]);
            if (b < 0 || mi > mb + 0.001f || (mi > mb - 0.001f && preferencia_ia(pc[i]) > preferencia_ia(pc[b]))) b = i;
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
    // O que so age abrindo a fila vai na frente; o que precisa de um dado
    // antes (o Par, a Sentinela...) nao abre: o primeiro que nao precisa
    // passa para a frente dele.
    for (int b = 1; b < m; b++)
        if (quer_abrir(pc[ordem[b]].tipo) && !quer_abrir(pc[ordem[0]].tipo)) {
            int t = ordem[b];
            memmove(&ordem[1], &ordem[0], (size_t)b * sizeof ordem[0]);
            ordem[0] = t;
            break;
        }
    if (m >= 2 && precisa_anterior(pc[ordem[0]].tipo)) {
        int b = 1;
        while (b < m && precisa_anterior(pc[ordem[b]].tipo)) b++;
        if (b < m) {
            int t = ordem[b];
            memmove(&ordem[1], &ordem[0], (size_t)b * sizeof ordem[0]);
            ordem[0] = t;
        }
    }
    return m;
}
static int fila_simples(const peca_t *pc, int n, int *ordem) { return fila_simples_r(pc, n, ordem, true); }

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
static bool abre_continua_de(int j, bool facil)
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
    if (f->lancados == 1) return r < 85;
    // O nivel facil, com dois dados ou mais e menos de 10 pontos: o risco que
    // compensa cai com o que ja esta na mesa (medido: com 7, ate uns 75%; com
    // 9, ate uns 45%). As rodadas imaginadas seguem a regra de antes.
    if (f->pontos < 10 && facil) { int lim = 75 - 15 * (f->pontos - 7); return r < (lim > 85 ? 85 : lim); }
    if (f->pontos < 10) return r < 85;
    return r < 40 || (r < 60 && f->pontos < 14);
}
static bool abre_continua(int j) { return abre_continua_de(j, false); }

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
        // O que o lance muda na bolsa, para as proximas rodadas (IA-5): o
        // Presente vira um raro; a Medusa valendo quebra os seus lancados;
        // o Pacto poe um Fantasma na sua bolsa e a Assombracao valendo, dois
        // na do rival; o Acumulador e a Furia guardam bonus; o Previsivel
        // sobe a escada; o Quebrado sai da bolsa; o ciclo cosmico valendo
        // evolui e volta para a mao.
        case EF_PRESENTE: b += 1.0f; break;
        case EF_MEDUSA:   if (vale) b -= 1.0f * (float)(f->lancados - 1); break;
        case EF_PACTO:    b -= 0.5f; break;
        case EF_ASSOMBRA: if (vale) b += 1.0f; break;
        case EF_ACUMULADOR: if (vale) b += 0.5f; break;
        case EF_FURIA:    b += f->cru[k] >= 4 ? 1.0f : f->cru[k] <= 2 ? -0.5f : 0; break;
        case EF_PREVISIVEL: b += 0.5f; break;
        case EF_QUEBRADO: b -= 0.5f; break;
        default:
            if (cosmico(f->tipo[k]) && vale) b += 1.5f;   // valendo no fim, evolui
            break;
        }
        // E os adesivos do lado que ficou para cima.
        const peca_t *pc = peca_fila(j, k);
        int ad = pc && f->lado[k] ? ad_no_lado(pc, f->lado[k]) : AD_NADA;
        switch (ad) {
        case AD_RASPADINHA: if (vale) b -= 1.0f; break;      // pagou e sai do dado
        case AD_FRAGIL:     if (!vale) b -= 1.5f; break;     // anulado, o dado quebra
        case AD_FERMENTO:   if (vale) b += 0.5f; break;      // cresce
        case AD_FIGURINHA:  if (vale) b += 0.3f; break;      // junta mais uma
        case AD_SELO:       b += 2.0f; break;                // lacrado: +5 no proximo lance valido
        case AD_VELCRO:     b += 0.5f; break;                // volta para a mao
        case AD_BIS:        b += 2.0f; break;                // um dado a mais na proxima fila
        case AD_MASSINHA:   b += 0.5f; break;                // um Remendo para sempre
        case AD_XEROX:      b += 0.5f; break;                // uma copia de adesivo para sempre
        default: break;
        }
        // A Dinamite que nao saiu: o pavio cresce (+2).
        if (pc && ad != AD_DINAMITE)
            for (int l = 1; l <= AD_LADOS; l++) if (ad_no_lado(pc, l) == AD_DINAMITE) { b += 0.5f; break; }
    }
    return b;
}
static float ia_futuro_nota(int j) { return ia_sem_margem[j & 1] ? 0 : IA_MARGEM * ia_futuro(j); }

// ---- A fila ----------------------------------------------------------------
// Testa toda ordem de ate quatro dados da mao, jogando cada uma varias vezes
// contra filas imaginadas do rival (ou contra a fila que ele ja jogou). Assim
// as sinergias (Dobro antes do maior, Fartura com dados baixos, Copiador,
// Jackpot...) aparecem sozinhas, sem regra escrita para cada uma.

// *chance (se pedida): so a chance de vencer, sem a margem, as fichas e o
// futuro que a nota soma - e o que a aposta usa (plano_p).
static float avalia_fila_c(int j, const int8_t *seq, int L, int amostras, uint32_t base,
                           const mesa_t *m, float *chance)
{
    float s = 0, c = 0;
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
        if (chance) c += placar(j);
        s += ia_futuro_nota(j);
    }
    if (chance) *chance = c / (float)amostras;
    return s / (float)amostras;
}
static float avalia_fila(int j, const int8_t *seq, int L, int amostras, uint32_t base, const mesa_t *m)
{
    return avalia_fila_c(j, seq, L, amostras, base, m, NULL);
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
    // O Par, a Sentinela e os outros que precisam de um dado antes nao abrem a
    // fila (o poder deles ficaria sem efeito), quando ha outro dado para abrir.
    bool tem_abre = false;
    for (int m = 0; m < n; m++) tem_abre |= !precisa_anterior(p->col[p->mao[m]].tipo);
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
        if (L >= 2 && gm < 0 && tem_abre && precisa_anterior(p->col[p->mao[s[0]]].tipo)) continue;
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
    int melhor = -1, segundo = -1;
    float nota_m = -1, nota_s = -1, chance_m = 0.5f;
    for (int f = 0; f < finalistas && f < nc; f++) {
        int b = -1;
        for (int c = 0; c < nc; c++) if (nota_c[c] > -1 && (b < 0 || nota_c[c] > nota_c[b])) b = c;
        if (b < 0) break;
        nota_c[b] = -2;                      // ja foi finalista
        float v = avalia_fila(j, cand[b], L, a2, base ^ 0x5bd1e995u, &m);
        if (v > nota_m) { segundo = melhor; nota_s = nota_m; nota_m = v; melhor = b; }
        else if (v > nota_s) { segundo = b; nota_s = v; }
    }
    // A sorte do vencedor: a melhor das finalistas pode estar na frente so
    // porque os sorteios dela sairam bons (filas com dados diferentes gastam
    // o sorteio de jeitos diferentes). A primeira e a segunda jogam de novo,
    // com sementes novas, e vale essa medida.
    uint32_t sb = base ^ 0x5bd1e995u;
    if (melhor >= 0) {
        sb = base ^ 0x27d4eb2fu;
        float c1, c2 = 0;
        nota_m = avalia_fila_c(j, cand[melhor], L, a2, sb, &m, &c1);
        chance_m = c1;
        if (segundo >= 0) {
            float v2 = avalia_fila_c(j, cand[segundo], L, a2, sb, &m, &c2);
            if (v2 > nota_m) { nota_m = v2; chance_m = c2; melhor = segundo; }
        }
    }
    // A fila pode ser mais curta: se so o comeco dela ja e melhor (o
    // Solitario sozinho, um dado grande e parar), a IA monta so esse comeco,
    // em vez de por quatro dados na mesa e parar no primeiro.
    int curta = L;
    for (int q = 1; q < L && melhor >= 0; q++) {
        float c;
        float v = avalia_fila_c(j, cand[melhor], q, a2, sb, &m, &c);
        if (v > nota_m + 0.01f) { nota_m = v; chance_m = c; curta = q; }
    }
    if (melhor >= 0) { memcpy(plano, cand[melhor], N_FILA); plano_n = curta; }
    // O Universo, sozinho, contra a melhor fila sem ele (com um grudado na
    // mesa, nao da: ele so vai sozinho).
    for (int i = 0; i < n && L >= 2 && gm < 0; i++) {
        if (p->col[p->mao[i]].tipo != D_UNIVERSO) continue;
        int8_t so[N_FILA] = { (int8_t)i };
        float c;
        float v = avalia_fila_c(j, so, 1, a2, sb, &m, &c);
        if (melhor < 0 || v > nota_m) { nota_m = v; chance_m = c; melhor = i; memcpy(plano, so, N_FILA); plano_n = 1; }
        break;
    }
    volta_mesa(&m);
    if (melhor < 0) return;
    // A aposta usa a chance de vencer, nao a nota (que soma margem, fichas e
    // o que fica guardado para depois).
    plano_p = chance_m < 0 ? 0 : chance_m > 1 ? 1 : chance_m;
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
        int ordem[N_FILA], L = fila_simples_r(mao, q, ordem, false);   // a mao de quem joga bem (IA-1)
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
    return abre_continua_de(vez, true);
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
