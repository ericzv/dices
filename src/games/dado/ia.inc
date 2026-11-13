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
    case EF_CARROSSEL: return 5.0f;                      // os validos da mesa no fim: em geral uns 5 dos 8
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
    case D_VENTANIA: case D_TRENO: case D_CAMALEAO: case D_GELATINA:   // a Gelatina quica abaixo do de antes
        return true;
    default:
        return false;
    }
}
// O lugar na fila em que o poder de cada dado age melhor, medido jogando as
// 24 ordens de maos de quatro (o dado de poder com d4, d6, d8, d12), abrindo
// e jogando depois, contra a bolsa inicial (fora do repo, 1500 rodadas por
// ordem). So os que a ordem pela media punha num lugar pior:
//   abre:       o Checkpoint (pontua por quem vem depois), a Viuva Negra
//               (sozinha valendo, x2: abrindo, da para parar nela), a
//               Sanguessuga (anula o seu dado de antes: abrindo, nao ha), o
//               Dobro (o seguinte dobra, e ele vale sempre) e o Garimpeiro
//               (abrindo, vale sempre);
//   segundo:    o Digital (sempre vale: cedo, ele joga) e o Martir (quer
//               cair: logo depois de um maior);
//   penultimo:  o Berserker (na ponta da fila, o vizinho do oponente pode nao
//               existir, e ai ele anula um dos seus) e o Estilingue (com 1, o
//               seguinte tira o maximo: antes do maior);
//   ultimo:     o Tudo ou Nada, o Egoista e o Sorte/Reves (os arriscados,
//               quando entram: por ultimo, da para parar antes deles), o
//               Oculto (fora da sequencia ate o fim) e a Estrela de Belem
//               (so age por ultima).
enum { LUGAR_LIVRE, LUGAR_ABRE, LUGAR_SEGUNDO, LUGAR_PENULTIMO, LUGAR_ULTIMO };
static int lugar_ia(int tipo)
{
    switch (tipo) {
    case D_CHECKPOINT: case D_VIUVA: case D_SANGUESSUGA: case D_DOBRO: case D_GARIMPEIRO:
        return LUGAR_ABRE;
    case D_DIGITAL: case D_MARTIR:
        return LUGAR_SEGUNDO;
    case D_BERSERKER: case D_ESTILINGUE:
        return LUGAR_PENULTIMO;
    case D_TUDO_NADA: case D_EGOISTA: case D_SORTE_REVES: case D_OCULTO: case D_ESTRELA:
        return LUGAR_ULTIMO;
    default:
        return LUGAR_LIVRE;
    }
}
// A barra: a media do numero que cai, que e o que a sequencia compara. O
// Pisca aceso dobra so no fim da rodada; na sequencia, vale o d4 que caiu.
static float barra_ia(peca_t p)
{
    if (TIPO[p.tipo].efeito == EF_PISCA) return (TIPO[p.tipo].lados + 1) / 2.0f;
    return media(p);
}

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
        for (int b = a; b > 0 && barra_ia(pc[ordem[b]]) < barra_ia(pc[ordem[b - 1]]); b--) {
            int t = ordem[b]; ordem[b] = ordem[b - 1]; ordem[b - 1] = t;
        }
    // Cada poder no lugar em que age (lugar_ia): o que abre na frente, o
    // segundo logo depois do primeiro, o penultimo e o ultimo no fim (um de
    // cada; os outros iguais ficam como livres); os livres no meio, pela barra.
    int ab = -1, sg = -1, pe = -1, ul = -1, livre[N_FILA], nl = 0;
    for (int i = 0; i < m; i++) {
        int l = lugar_ia(pc[ordem[i]].tipo), o = ordem[i];
        if (l == LUGAR_ABRE && ab < 0) ab = o;
        else if (l == LUGAR_SEGUNDO && sg < 0) sg = o;
        else if (l == LUGAR_PENULTIMO && pe < 0 && m >= 3) pe = o;
        else if (l == LUGAR_ULTIMO && ul < 0) ul = o;
        else livre[nl++] = o;
    }
    int k = 0, f = 0, fim = ul >= 0 ? nl : nl - 1;   // sem um ultimo, o maior dos livres fecha a fila
    if (ab >= 0) ordem[k++] = ab;
    else if (sg >= 0 && f < nl) ordem[k++] = livre[f++];
    if (sg >= 0) ordem[k++] = sg;
    while (f < fim) ordem[k++] = livre[f++];
    if (pe >= 0) ordem[k++] = pe;
    while (f < nl) ordem[k++] = livre[f++];
    if (ul >= 0) ordem[k++] = ul;
    // O Ferreiro (e o Dobro, se nao abriu) mexe no dado seguinte: por ultimo
    // nao serve. Vai uma casa antes.
    if (m >= 2 && (pc[ordem[m - 1]].tipo == D_DOBRO || pc[ordem[m - 1]].tipo == D_FERREIRO)) {
        int t = ordem[m - 1]; ordem[m - 1] = ordem[m - 2]; ordem[m - 2] = t;
    }
    // O que precisa de um dado antes (o Par, a Sentinela...) nao abre: o
    // primeiro que nao precisa passa para a frente dele.
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
static float ia_folga = 1.0f;                // quanto a margem pesa (a mestre, que conta fichas, quase nao olha)
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
        r += IA_MARGEM * ia_folga * (float)m;
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
// imagina jogadas, mas pensando pouco: erra de vez em quando), 2 dificil,
// 3 a mestre (no fim do arquivo; nenhuma mesa do jogo usa ainda).
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

// ---- Pensar aos pedacos -------------------------------------------------------
// Uma decisao custa dezenas de milissegundos (com a mesa cheia de especiais,
// centenas): feita de uma vez, a tela para. Quando a plataforma da um tempo
// por quadro (dado_ia_orcamento: o PC e o navegador), a decisao e feita aos
// poucos, ja na pausa de "pensar": cada conta de rodadas imaginadas guarda
// onde parou (as somas e a proxima amostra). No quadro seguinte a decisao e
// refeita do comeco, e cada conta pronta so devolve o resultado guardado, com
// a semente da IA onde a conta a deixou. Cada amostra tem a semente dela, e
// as somas seguem na mesma ordem: a decisao sai igual a feita de uma vez,
// so mais tarde quando nao cabe na pausa. Sem tempo dado (os testes, o
// simulador, o Cardputer), tudo e de uma vez, como sempre.
static int ia_orcamento_us;                  // 0: sem limite
void dado_ia_orcamento(int us) { ia_orcamento_us = us > 0 ? us : 0; }

typedef struct {
    uint32_t chave;                          // de quem e a conta (conferencia)
    int feitas;                              // amostras ja somadas
    float s, c;                              // as somas ate ali
    uint32_t semente;                        // a semente da IA no fim da conta
    bool pronta;
} ia_memo_t;
#define IA_MEMOS (2520 + 64)                 // as filas candidatas (MAX_CAND) e as outras contas
static ia_memo_t ia_memos[IA_MEMOS];
static int ia_n_memos, ia_i_memo;            // as guardadas; a proxima desta passada
static bool ia_aos_pedacos, ia_faltou;       // numa passada aos pedacos; o tempo dela acabou
static int64_t ia_prazo;                     // quando acaba o tempo desta passada
static int ia_feitas;                        // amostras contadas nesta passada (a primeira sempre anda)
static uint32_t ia_marca;                    // a mesa a que as contas guardadas valem
static int ia_trocas;                        // quantas vezes a mesa mudou nesta decisao
static bool ia_de_uma_vez;                   // a mesa nao para quieta: esta decisao vai de uma vez

static uint32_t ia_fnv(uint32_t h, const void *p, size_t n)
{
    const uint8_t *b = p;
    for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}
// Tudo o que as contas leem e pode mudar entre um quadro e outro.
static uint32_t ia_estado(void)
{
    uint32_t h = 2166136261u;
    h = ia_fnv(h, F, sizeof F);
    h = ia_fnv(h, J, sizeof J);
    int v[] = { fase, vez, primeiro, rodada, pote, a_pagar, ia_nivel, ia_esforco, ver_tipo,
                anulou_a, anulou_b, resistiu, anulou_masc, ver_comum, ia_sem_margem[0], ia_sem_margem[1] };
    h = ia_fnv(h, v, sizeof v);
    float fl[] = { ia_ousadia, ia_folga };
    h = ia_fnv(h, fl, sizeof fl);
    h = ia_fnv(h, chic_valor, sizeof chic_valor);
    h = ia_fnv(h, chic_lado, sizeof chic_lado);
    h = ia_fnv(h, loja, sizeof loja);
    h = ia_fnv(h, levou, sizeof levou);
    h = ia_fnv(h, oferta, sizeof oferta);
    h = ia_fnv(h, oferta_ad, sizeof oferta_ad);
    return h;
}
// Esquece as contas guardadas (outra decisao).
static void ia_memo_zera(void) { ia_n_memos = 0; ia_marca = 0; ia_trocas = 0; ia_de_uma_vez = false; }
// Comeca uma passada aos pedacos: se a mesa mudou, as contas guardadas nao
// valem mais (e se ela mudar toda hora, esta decisao vai de uma vez).
static void ia_passada_comeca(void)
{
    uint32_t e = ia_estado();
    if (e != ia_marca) {
        if (ia_marca && ++ia_trocas > 4) ia_de_uma_vez = true;
        ia_n_memos = 0;
        ia_marca = e;
    }
    ia_aos_pedacos = !ia_de_uma_vez;
    ia_faltou = false;
    ia_i_memo = 0;
    ia_feitas = 0;
    ia_prazo = esp_timer_get_time() + ia_orcamento_us;
}
static void ia_passada_fim(void) { ia_aos_pedacos = false; }
// A proxima conta desta passada: a guardada no mesmo lugar (se for a mesma
// conta) ou uma nova. NULL: fora de uma passada aos pedacos, de uma vez.
static ia_memo_t *ia_memo(uint32_t chave)
{
    if (!ia_aos_pedacos || ia_i_memo >= IA_MEMOS) return NULL;
    int i = ia_i_memo++;
    ia_memo_t *e = &ia_memos[i];
    if (i >= ia_n_memos || e->chave != chave) {
        memset(e, 0, sizeof *e);
        e->chave = chave;
        ia_n_memos = i + 1;
    }
    return e;
}
// O tempo da passada acabou? (Depois de acabar, toda conta devolve 0 sem
// contar nada: a passada so termina para a mesa voltar como estava.)
static bool ia_sem_tempo(void)
{
    if (ia_faltou) return true;
    if (ia_feitas++ == 0 || esp_timer_get_time() < ia_prazo) return false;
    ia_faltou = true;
    return true;
}
// Uma conta guardada pronta: a semente fica onde a conta a deixou.
static bool ia_memo_pronta(const ia_memo_t *e)
{
    if (!e || !e->pronta) return false;
    semente_ia = e->semente;
    return true;
}
static void ia_memo_guarda(ia_memo_t *e, int feitas, float s, float c, bool pronta)
{
    if (!e) return;
    e->feitas = feitas; e->s = s; e->c = c; e->pronta = pronta;
    if (pronta) e->semente = semente_ia;
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
    int a0 = 0;
    ia_memo_t *e = NULL;
    if (ia_aos_pedacos) {
        if (ia_faltou) { if (chance) *chance = 0; return 0; }
        uint32_t k = ia_fnv(2166136261u ^ 1u, seq, (size_t)L);
        int v[] = { j, L, amostras, chance != NULL };
        k = ia_fnv(ia_fnv(k, v, sizeof v), &base, sizeof base);
        e = ia_memo(k);
        if (ia_memo_pronta(e)) { if (chance) *chance = e->c / (float)amostras; return e->s / (float)amostras; }
        if (e) { a0 = e->feitas; s = e->s; c = e->c; }
    }
    for (int a = a0; a < amostras; a++) {
        if (e && ia_sem_tempo()) { ia_memo_guarda(e, a, s, c, false); if (chance) *chance = 0; return 0; }
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
    ia_memo_guarda(e, amostras, s, c, true);
    if (chance) *chance = c / (float)amostras;
    return s / (float)amostras;
}
static float avalia_fila(int j, const int8_t *seq, int L, int amostras, uint32_t base, const mesa_t *m)
{
    return avalia_fila_c(j, seq, L, amostras, base, m, NULL);
}

#define MAX_CAND 2520                        // 7 x 6 x 5 x 4 x 3 (a fila de 5 do Estojo de Lapis)
_Static_assert(MAO <= 16 && N_FILA * 4 <= 64, "a assinatura da fila: um lugar da mao em 4 bits");
static int8_t cand[MAX_CAND][N_FILA];
static float nota_c[MAX_CAND];
static uint64_t assin[MAX_CAND];
// Todas as filas de L dados da mao de j que vale a pena pesar (em cand[]);
// gm: o lugar na mao do dado grudado pelo Chiclete, ou -1. Devolve quantas.
static uint32_t cand_versao;                 // muda a cada montagem de cand[]
static int monta_candidatas(int j, int L, int gm)
{
    cand_versao++;
    jogador_t *p = &J[j];
    int n = p->n_mao, nc = 0;
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
    return nc;
}

static void mestre_planeja(void);            // a do nivel 3, mais abaixo
// As candidatas de j: aos pedacos, a mesma mesa nao monta de novo a cada
// passada (com a mao grande, montar ja pesa).
static int ia_candidatas(int j, int L, int gm)
{
    static uint32_t marca, versao;
    static int cj = -1, cl, cg, cn;
    if (ia_aos_pedacos && marca == ia_marca && versao == cand_versao && cj == j && cl == L && cg == gm) return cn;
    cn = monta_candidatas(j, L, gm);
    marca = ia_marca; versao = cand_versao; cj = j; cl = L; cg = gm;
    return cn;
}
static void ia_planeja_fila(int j);
static void ia_planeja(void)
{
    int j = vez;
    if (plano_rodada == rodada && plano_vez == j) return;
    if (ia_nivel >= 3) { mestre_planeja(); return; }
    ia_planeja_fila(j);
    if (ia_faltou) plano_rodada = -1;            // a conta nao acabou: o plano fica para a proxima passada
}
static void ia_planeja_fila(int j)
{
    jogador_t *p = &J[j];
    int n = p->n_mao, L = n < max_fila(j) ? n : max_fila(j);
    plano_rodada = rodada; plano_vez = j; plano_n = 0; plano_p = 0.5f;
    if (L <= 0) return;
    int gm = -1;                                 // o grudado pelo Chiclete: o primeiro de toda fila
    for (int m = 0; m < n && F[j].grudado; m++) if (p->mao[m] == F[j].idx[0]) gm = m;
    int nc = ia_candidatas(j, L, gm);

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
    int a0 = 0;
    ia_memo_t *e = NULL;
    if (ia_aos_pedacos) {                    // aos pedacos: de onde a conta parou
        if (ia_faltou) { volta_mesa(&m); return false; }
        int v[] = { 2, j, f->lancados, amostras };
        e = ia_memo(ia_fnv(ia_fnv(2166136261u, v, sizeof v), &base, sizeof base));
        if (ia_memo_pronta(e)) a0 = amostras;
        if (e) { if (!e->pronta) a0 = e->feitas; parar = e->s; seguir = e->c; }
    }
    for (int a = a0; a < amostras; a++) {
        if (e && ia_sem_tempo()) { ia_memo_guarda(e, a, parar, seguir, false); volta_mesa(&m); return false; }
        volta_mesa(&m);
        semeia_ia(base, a);
        F[j].n = F[j].lancados;              // parar: a fila acaba aqui
        parar += sim_resto(j, true) + ia_futuro_nota(j);
        volta_mesa(&m);
        semeia_ia(base, a);
        sim_lanca(j);
        seguir += sim_resto(j, true) + ia_futuro_nota(j);
    }
    if (e && !e->pronta) ia_memo_guarda(e, amostras, parar, seguir, true);
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
    int a0 = 0;
    ia_memo_t *e = NULL;
    if (ia_aos_pedacos) {                        // aos pedacos: de onde a conta parou
        if (ia_faltou) { volta_mesa(&m); return 0; }
        int v[] = { 3, j, rodada, amostras, F[j].lancados };
        e = ia_memo(ia_fnv(2166136261u, v, sizeof v));
        if (ia_memo_pronta(e)) a0 = amostras;
        if (e) { if (!e->pronta) a0 = e->feitas; s = e->s; }
    }
    for (int a = a0; a < amostras; a++) {
        if (e && ia_sem_tempo()) { ia_memo_guarda(e, a, s, 0, false); volta_mesa(&m); return 0; }
        volta_mesa(&m);
        semeia_ia(0xBE77u + (uint32_t)rodada, a);
        F[j].n = F[j].lancados;
        s += sim_resto(j, false);
    }
    if (e && !e->pronta) ia_memo_guarda(e, amostras, s, 0, true);
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
    int a0 = 0;
    ia_memo_t *e = NULL;
    if (ia_aos_pedacos) {                        // aos pedacos: de onde a conta parou
        if (ia_faltou) return 0;
        int v[] = { 4, j, extra, fixo_extra, amostras, n };
        e = ia_memo(ia_fnv(ia_fnv(2166136261u, v, sizeof v), &base, sizeof base));
        if (ia_memo_pronta(e)) a0 = amostras;
        if (e) { if (!e->pronta) a0 = e->feitas; s = e->s; }
    }
    for (int a = a0; a < amostras; a++) {
        if (e && ia_sem_tempo()) { ia_memo_guarda(e, a, s, 0, false); volta_mesa(&m); return 0; }
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
    if (e && !e->pronta) ia_memo_guarda(e, amostras, s, 0, true);
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

// ===========================================================================
// A IA mestre (nivel 3)
// ===========================================================================
// Um nivel acima da afiada. Nenhuma mesa do jogo usa ainda: e so pedir o
// nivel 3 (nivel_jog) que ela joga. Monta a fila e lanca como a afiada, com
// mais cuidado na escolha, e muda de verdade na aposta, que ela pensa pela
// partida inteira: o que conta nao e vencer a rodada, e a chance de vencer a
// partida com as fichas que sobram depois dela.
//
// A aposta e um jogo pequeno em que so um lado tem segredo: a mesa de quem
// abriu esta a vista, e so quem joga depois sabe a mao que tem. A mestre
// sorteia as maos que ele pode ter, mede a chance de cada uma contra a mesa
// (com as regras de verdade) e resolve esse jogo ate o equilibrio: quanto
// apostar com mao boa, quanto blefar, quando pagar e quando correr. Medido
// na arena (tests/arena.c), contra a afiada. (O jeito de apostar de cada
// oponente, ia_ousadia, ela nao usa: aposta sempre pela conta.)

// A curva da partida: a chance (em 10000) de vencer a partida para quem ABRE
// a rodada r (linha r - 1), entrando nela com d fichas a mais que o rival
// (coluna: d de -100 a 100, de 4 em 4; as fichas dos dois somam 100). Quem
// nao paga a entrada ja perdeu. Sai de tras para a frente (tests/arena.c,
// "curva"): na ultima rodada so importa quem fica na frente; em cada rodada
// antes, o valor e o da aposta resolvida, em mesas de partidas de verdade,
// com a curva da rodada seguinte no fim. Quem joga depois ve a mesa, e por
// isso abrir vale menos: levar a rodada (e abrir a proxima) pesa menos que as
// fichas dizem.
#define MS_CURVA_N 51
static const uint16_t MS_CURVA[DES_RODADAS][MS_CURVA_N] = {
    { 0, 0, 0, 417, 558, 733, 941, 1111, 1323, 1511, 1699, 1880, 2087, 2348, 2556, 2724, 2873, 3102, 3412,
      3650, 3792, 3937, 4194, 4530, 4789, 4922, 5054, 5315, 5657, 5917, 6057, 6196, 6441, 6753, 6995, 7151,
      7312, 7529, 7793, 8005, 8195, 8392, 8589, 8813, 8992, 9210, 9398, 9550, 10000, 10000, 10000, },
    { 0, 0, 0, 404, 541, 701, 894, 1093, 1253, 1455, 1669, 1897, 2109, 2240, 2437, 2714, 2987, 3160, 3275,
      3457, 3789, 4124, 4297, 4383, 4560, 4922, 5287, 5463, 5543, 5717, 6061, 6405, 6584, 6688, 6861, 7164,
      7462, 7633, 7762, 7987, 8226, 8450, 8663, 8829, 9041, 9243, 9415, 9564, 10000, 10000, 10000, },
    { 0, 0, 0, 330, 481, 663, 871, 1017, 1240, 1426, 1619, 1772, 1982, 2320, 2544, 2683, 2777, 3024, 3419,
      3654, 3746, 3825, 4103, 4571, 4855, 4919, 4980, 5265, 5746, 6028, 6098, 6184, 6433, 6839, 7104, 7196,
      7319, 7560, 7898, 8113, 8274, 8477, 8671, 8912, 9066, 9285, 9480, 9644, 10000, 10000, 10000, },
    { 0, 0, 0, 391, 487, 610, 696, 935, 1131, 1283, 1545, 1828, 2093, 2125, 2330, 2683, 3024, 3147, 3205,
      3298, 3768, 4198, 4277, 4325, 4385, 4919, 5459, 5520, 5552, 5630, 6106, 6577, 6660, 6726, 6835, 7206,
      7596, 7740, 7774, 8060, 8357, 8638, 8795, 8985, 9248, 9341, 9475, 9578, 10000, 10000, 10000, },
    { 0, 0, 0, 0, 0, 667, 836, 1041, 1041, 1403, 1424, 1438, 1584, 2341, 2622, 2680, 2719, 3084, 3488, 3574,
      3594, 3630, 3674, 4764, 4867, 4887, 4913, 4957, 6040, 6147, 6151, 6183, 6277, 6864, 7087, 7129, 7191,
      7497, 8275, 8441, 8456, 8479, 8878, 8878, 9095, 9283, 10000, 10000, 10000, 10000, 10000, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1120, 1384, 1384, 1384, 2024, 2024, 2213, 2213, 2213, 2308, 3634, 3634, 3780,
      3885, 3885, 4852, 4852, 4852, 4852, 4852, 6132, 6132, 6132, 6132, 6132, 7595, 7595, 7595, 7595, 7807,
      7807, 8503, 8503, 8503, 8797, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2323, 2323, 2323, 2323, 3296, 3296, 3296, 3296, 3296, 3296,
      5005, 5005, 5005, 5005, 5005, 6412, 6412, 6412, 6412, 6412, 7503, 7503, 7503, 7503, 7503, 10000, 10000,
      10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4820, 4820, 4820, 4820, 4820, 4820, 4820, 4820,
      4820, 4820, 4820, 4820, 4820, 4820, 4820, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
      10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, },
};
static const uint16_t (*ms_curva)[MS_CURVA_N] = MS_CURVA;   // (a arena troca pela que esta medindo)
static float ms_curva_le(int r, float d)
{
    const uint16_t *c = ms_curva[r - 1];
    float x = (d + 100.0f) / 4.0f;
    if (x <= 0) return (float)c[0] / 10000.0f;
    if (x >= MS_CURVA_N - 1) return (float)c[MS_CURVA_N - 1] / 10000.0f;
    int i = (int)x;
    float t = x - (float)i;
    return ((1.0f - t) * (float)c[i] + t * (float)c[i + 1]) / 10000.0f;
}
// Chance de vencer a partida saindo desta rodada com fj fichas contra fr;
// abre: j abre a proxima (levou esta). Depois da ultima rodada, so importa
// quem fica na frente; antes dela, quem nao paga a proxima entrada perde, e o
// resto vem da curva. Fora do Desafiante (mais rodadas, mais fichas), a curva
// vale pelas rodadas que faltam e pela parte das fichas.
static float ms_util(int fj, int fr, bool abre)
{
    int max_r = modo == M_DESAFIO ? DES_RODADAS : MAX_RODADAS;
    int d = fj - fr, faltam = max_r - rodada;
    float lado = d > 0 ? 1.0f : d < 0 ? 0.0f : 0.5f;
    if (!ms_curva) return 0.5f + (float)d / 400.0f;   // (sem curva: so as fichas, para a arena medir a aposta em fichas)
    if (faltam <= 0) return lado;
    int ante = ante_da(rodada + 1);
    if (fj < ante || fr < ante) return fj < ante && fr < ante ? lado : fj < ante ? 0.0f : 1.0f;
    int r = faltam >= DES_RODADAS ? 1 : DES_RODADAS + 1 - faltam;
    float dn = (float)d * 100.0f / (float)(fj + fr);
    return abre ? ms_curva_le(r, dn) : 1.0f - ms_curva_le(r, -dn);
}

// As maos que r pode ter (ele ainda nao jogou): MS_TIPOS maos sorteadas do
// que ele tem na bolsa e na mao (a mao de verdade a IA nao olha), cada uma
// jogada MS_LANCES vezes contra a mesa de quem abriu. p[k]: a chance de a mao
// k vencer a rodada.
#define MS_TIPOS  40
#define MS_LANCES 20
// A mao imaginada joga pela regra simples (fila_imaginada); quem joga de
// verdade acha filas melhores. Medido (LAB=3 na arena): 9 pontos de chance a
// mais nas maos do meio, nada nas que ja vencem ou perdem sempre.
#define MS_ACHA   0.09f
static uint32_t ms_marca(int j);
// As maos ja medidas desta aposta (a conta se reparte por quadros, e serve
// para as duas decisoes de quem abriu). crua: a chance de quem abriu sem o
// acerto de MS_ACHA - o que o rival franco ve.
static struct { uint32_t marca; int k; float p[MS_TIPOS], crua; } ms_maos;
// Mede mais n maos de r; true quando estao todas. A mesa volta como estava.
static bool ms_maos_fatia(int r, int n)
{
    uint32_t base = 0x3E57A1u + (uint32_t)rodada * 7919u + (uint32_t)primeiro * 104729u;
    uint32_t marca = (ms_marca(r) ^ base) | 1;
    if (ms_maos.marca != marca) { ms_maos.marca = marca; ms_maos.k = 0; ms_maos.crua = 0; }
    if (ms_maos.k >= MS_TIPOS) return true;
    mesa_t m;
    guarda_mesa(&m);
    for (; n > 0 && ms_maos.k < MS_TIPOS; n--, ms_maos.k++) {
        int k = ms_maos.k;
        float s = 0;
        for (int a = 0; a < MS_LANCES; a++) {
            volta_mesa(&m);
            semeia_ia(base, k);                  // a mesma mao em todos os lances dela
            fila_imaginada(r, false);
            semeia_ia(base ^ (0x9E3779B9u * (uint32_t)(k + 1)), a);
            sim_turno(r);
            s += placar(r);
        }
        s /= (float)MS_LANCES;
        ms_maos.crua += (1.0f - s) / (float)MS_TIPOS;
        ms_maos.p[k] = s + MS_ACHA * 4.0f * s * (1.0f - s);
    }
    volta_mesa(&m);
    return ms_maos.k >= MS_TIPOS;
}
// Todas de uma vez, em p[] (e devolve a chance crua de quem abriu).
static float ms_tipos(int r, float *p)
{
    while (!ms_maos_fatia(r, MS_TIPOS)) {}
    memcpy(p, ms_maos.p, sizeof ms_maos.p);
    return ms_maos.crua;
}

// A aposta, numa copia: f[0] e de quem abriu a rodada, f[1] de quem joga
// depois; vez, 0 ou 1 do mesmo jeito. As contas sao as de aposta.inc.
typedef struct { int f[2], pote, a_pagar, passes, vez; bool aumentou; } ms_ap_t;
static int ms_lim(const ms_ap_t *s, int quer)
{
    int m = s->f[0] < s->f[1] ? s->f[0] : s->f[1];
    return quer < m ? quer : m;
}
static int ms_sobe(const ms_ap_t *s)
{
    int sobe = s->a_pagar, sobra = s->f[s->vez] - s->a_pagar;
    if (sobe > sobra) sobe = sobra;
    if (sobe > s->f[s->vez ^ 1]) sobe = s->f[s->vez ^ 1];
    return sobe < 0 ? 0 : sobe;
}
// O que da para fazer (so o que muda alguma coisa: sem ficha, apostar e passar).
static int ms_ops(const ms_ap_t *s, int *op)
{
    int n = 0;
    if (!s->a_pagar) {
        int b10 = ms_lim(s, 10), b25 = ms_lim(s, 25);
        op[n++] = OP_PASSAR;
        if (b10 > 0) op[n++] = OP_APOSTAR10;
        if (b25 > b10) op[n++] = OP_APOSTAR25;
    } else {
        op[n++] = OP_CORRER;
        op[n++] = OP_PAGAR;
        if (!s->aumentou && ms_sobe(s) > 0) op[n++] = OP_AUMENTAR;
    }
    return n;
}
// Faz o lance na copia: 0, a aposta segue; 1, fechou (o segundo joga); 2,
// quem estava na vez correu.
static int ms_faz(ms_ap_t *s, int op)
{
    int v = s->vez;
    switch (op) {
    case OP_PASSAR:
        if (++s->passes >= 2) return 1;
        break;
    case OP_APOSTAR10:
    case OP_APOSTAR25: {
        int b = ms_lim(s, op == OP_APOSTAR10 ? 10 : 25);
        if (b <= 0) return 1;
        s->f[v] -= b; s->pote += b; s->a_pagar = b;
        break;
    }
    case OP_PAGAR: {
        int b = s->a_pagar < s->f[v] ? s->a_pagar : s->f[v];
        s->f[v] -= b; s->pote += b;
        return 1;
    }
    case OP_AUMENTAR: {
        int sobe = ms_sobe(s);
        s->f[v] -= s->a_pagar + sobe; s->pote += s->a_pagar + sobe;
        s->a_pagar = sobe;
        s->aumentou = true;
        break;
    }
    default:
        return 2;
    }
    s->vez = v ^ 1;
    return 0;
}
// O que quem abriu espera da partida (ms_util) se a aposta acabar assim,
// contra cada mao k do outro: correu alguem, ou os dados decidem o pote.
static void ms_fim(const ms_ap_t *s, int fim, const float *p, int K, float *u)
{
    if (fim == 2) {
        float v = s->vez == 0 ? ms_util(s->f[0], s->f[1] + s->pote, false) : ms_util(s->f[0] + s->pote, s->f[1], true);
        for (int k = 0; k < K; k++) u[k] = v;
        return;
    }
    float ganha = ms_util(s->f[0] + s->pote, s->f[1], true), perde = ms_util(s->f[0], s->f[1] + s->pote, false);
    for (int k = 0; k < K; k++) u[k] = (1.0f - p[k]) * ganha + p[k] * perde;
}

// O rival franco: aposta pelo valor, como a IA afiada e muita gente - aposta
// quando esta bem, dobra quando esta muito bem, paga quando a chance cobre o
// preco, quase nao blefa (os cortes sao os de ia_aposta). x: a chance que ele
// ve (a da mao, para quem joga depois; a da mesa, para quem abriu), com uma
// folga (larg) no lugar do corte seco. Devolve a chance de cada lance de op[].
static float ms_mole(float v, float larg) { return 1.0f / (1.0f + expf(-v / larg)); }
static void ms_franco(const ms_ap_t *s, float x, float larg, const int *op, int n, float *sg)
{
    float q[OP_CORRER + 1] = { 0 };
    if (!s->a_pagar) {
        float f25 = ms_mole(x - 0.72f, larg), f10 = ms_mole(x - 0.58f, larg);
        if (f10 < f25) f10 = f25;
        float blefe = 0.125f * ms_mole(0.35f - x, larg);
        q[OP_APOSTAR25] = f25;
        q[OP_APOSTAR10] = f10 - f25 + blefe * (1 - f10);
        q[OP_PASSAR] = 1 - q[OP_APOSTAR25] - q[OP_APOSTAR10];
        bool tem10 = false, tem25 = false;
        for (int a = 0; a < n; a++) { tem10 |= op[a] == OP_APOSTAR10; tem25 |= op[a] == OP_APOSTAR25; }
        if (!tem25) { q[OP_APOSTAR10] += q[OP_APOSTAR25]; q[OP_APOSTAR25] = 0; }
        if (!tem10) { q[OP_PASSAR] += q[OP_APOSTAR10]; q[OP_APOSTAR10] = 0; }
    } else {
        float precisa = (float)s->a_pagar / (float)(s->pote + s->a_pagar);
        float dobra = ms_mole(x - 0.75f, larg), paga = ms_mole(x - precisa, larg);
        q[OP_AUMENTAR] = dobra;
        q[OP_PAGAR] = (1 - dobra) * paga;
        q[OP_CORRER] = 1 - q[OP_AUMENTAR] - q[OP_PAGAR];
        bool tem = false;
        for (int a = 0; a < n; a++) tem |= op[a] == OP_AUMENTAR;
        if (!tem) { q[OP_PAGAR] += q[OP_AUMENTAR]; q[OP_AUMENTAR] = 0; }
    }
    for (int a = 0; a < n; a++) sg[a] = q[op[a]];
}

// O equilibrio, pelo arrependimento: a cada volta, cada lado joga mais o que
// teria dado mais certo ate ali (quem abriu, um plano so; o outro, um por
// mao), e a media das voltas vai para o equilibrio. O jogo e pequeno: cabe
// em poucos milissegundos.
// O rival da conta e uma mistura: numa parte (MS_FRANCA) ele e o franco, de
// plano fixo; no resto, joga o melhor que da contra a mestre. Assim ela ganha
// do jeito franco de apostar sem abrir a guarda para quem a estuda: com a
// parte franca em 0, e o equilibrio puro, que ninguem explora (e que ganha
// menos de quem aposta franco).
#define MS_NOS 64                            // ate 3 lances antes de cada decisao: 4^3
static float ms_arrep[MS_NOS][MS_TIPOS + 1][3], ms_media[MS_NOS][MS_TIPOS + 1][3], ms_fixo[MS_NOS][MS_TIPOS + 1][3];
typedef struct {
    const float *p, *w;                      // a chance e o peso de cada mao de quem joga depois
    int K, eu;                               // eu: o lado da mestre (0 abriu, 1 joga depois)
    bool franco, primeira;                   // nesta volta, o rival e o franco; e a primeira volta
    float peso, mesa;                        // o peso da volta na media; mesa: a chance que quem abriu ve
} ms_conta_t;
static void ms_plano(const float *arrep, int n, float *sg)
{
    float t = 0;
    for (int a = 0; a < n; a++) t += arrep[a] > 0 ? arrep[a] : 0;
    for (int a = 0; a < n; a++) {
        sg[a] = t > 0 ? (arrep[a] > 0 ? arrep[a] : 0) / t : 1.0f / (float)n;
        if (sg[a] < 1e-4f) sg[a] = 0;            // (chances minusculas multiplicadas viram numeros que travam a conta)
    }
}
#define MS_LARG_MESA 0.03f
#define MS_LARG_MAO  0.06f
// Uma volta, do no h em diante. ra e rb[k]: a chance de cada lado ter jogado
// ate aqui (rb, por mao). Devolve em u[k] o que quem abriu espera contra a mao k.
static void ms_volta(const ms_conta_t *c, ms_ap_t s, int h, float ra, const float *rb, float *u)
{
    int op[3], n = ms_ops(&s, op), K = c->K;
    float ua[3][MS_TIPOS + 1], rb2[MS_TIPOS + 1], sg[3];
    bool fixo = c->franco && s.vez != c->eu;     // o rival franco: plano fixo, sem aprender
    if (s.vez == 0) {
        if (fixo) {                              // (o plano do franco nao muda de uma volta para a outra)
            if (c->primeira) ms_franco(&s, c->mesa, MS_LARG_MESA, op, n, ms_fixo[h][0]);
            memcpy(sg, ms_fixo[h][0], sizeof sg);
        } else ms_plano(ms_arrep[h][0], n, sg);
        for (int a = 0; a < n; a++) {
            ms_ap_t s2 = s;
            int fim = ms_faz(&s2, op[a]);
            if (fim) ms_fim(&s2, fim, c->p, K, ua[a]);
            else ms_volta(c, s2, h * 4 + a + 1, ra * sg[a], rb, ua[a]);
            if (!c->franco) ms_media[h][0][a] += c->peso * ra * sg[a];
        }
        float arr[3] = { 0 };
        for (int k = 0; k < K; k++) {
            u[k] = 0;
            for (int a = 0; a < n; a++) u[k] += sg[a] * ua[a][k];
            for (int a = 0; a < n; a++) arr[a] += c->w[k] * rb[k] * (ua[a][k] - u[k]);
        }
        for (int a = 0; a < n && !fixo; a++) {
            float v = ms_arrep[h][0][a] + arr[a];
            ms_arrep[h][0][a] = v > 0 ? v : 0;
        }
        return;
    }
    float sk[MS_TIPOS + 1][3];
    for (int k = 0; k < K; k++) {
        if (fixo) {
            if (c->primeira) ms_franco(&s, c->p[k], MS_LARG_MAO, op, n, ms_fixo[h][k]);
            memcpy(sk[k], ms_fixo[h][k], sizeof sk[k]);
        } else ms_plano(ms_arrep[h][k], n, sk[k]);
    }
    for (int a = 0; a < n; a++) {
        for (int k = 0; k < K; k++) rb2[k] = rb[k] * sk[k][a];
        ms_ap_t s2 = s;
        int fim = ms_faz(&s2, op[a]);
        if (fim) ms_fim(&s2, fim, c->p, K, ua[a]);
        else ms_volta(c, s2, h * 4 + a + 1, ra, rb2, ua[a]);
    }
    for (int k = 0; k < K; k++) {
        u[k] = 0;
        for (int a = 0; a < n; a++) u[k] += sk[k][a] * ua[a][k];
        for (int a = 0; a < n && !fixo; a++) {
            float v = ms_arrep[h][k][a] + ra * (u[k] - ua[a][k]);   // o que e bom para quem abriu e ruim para ele
            ms_arrep[h][k][a] = v > 0 ? v : 0;
            if (!c->franco) ms_media[h][k][a] += c->peso * rb[k] * sk[k][a];
        }
    }
}
#define MS_VOLTAS 300
#define MS_FRANCA 0.8f
static float ms_franca_jog[2] = { MS_FRANCA, MS_FRANCA };   // a parte franca de cada lugar (a arena mede uma contra a outra)
// Resolve a aposta a partir de s. eu: o lado da mestre; fr: a parte franca do
// rival; mesa: a chance que quem abriu ve. Devolve o que quem abriu espera da
// partida (a media das voltas).
static float ms_resolve(const ms_ap_t *s, const float *p, const float *w, int K, int eu, float fr, float mesa)
{
    memset(ms_arrep, 0, sizeof ms_arrep);
    memset(ms_media, 0, sizeof ms_media);
    float rb[MS_TIPOS + 1], u[MS_TIPOS + 1];
    ms_conta_t c = { .p = p, .w = w, .K = K, .eu = eu, .mesa = mesa };
    if (fr > 0.95f) fr = 0.95f;                  // (a media dos planos vem da parte livre)
    float valor = 0, pesos = 0;
    for (int t = 1; t <= MS_VOLTAS; t++) {
        c.peso = (float)t;
        c.primeira = t == 1;
        for (int mundo = 0; mundo < 2; mundo++) {
            float parte = mundo ? fr : 1.0f - fr;
            if (parte <= 0) continue;
            c.franco = mundo == 1;
            // A parte do rival entra no alcance dele: no de quem joga depois, por mao; no de quem abriu, um so.
            for (int k = 0; k < K; k++) rb[k] = eu == 0 ? parte : 1.0f;
            ms_volta(&c, *s, 0, eu == 0 ? 1.0f : parte, rb, u);
            for (int k = 0; k < K; k++) valor += c.peso * parte * w[k] * u[k];
        }
        pesos += c.peso;
    }
    return valor / pesos;
}
// O plano medio de um no (k: a mao de quem joga depois; 0 para quem abriu).
static void ms_le(int h, int k, int n, float *q)
{
    float t = 0;
    for (int a = 0; a < n; a++) t += ms_media[h][k][a];
    for (int a = 0; a < n; a++) q[a] = t > 0 ? ms_media[h][k][a] / t : 1.0f / (float)n;
}

// Um numero de 0 a 1 para sortear o lance desta decisao: sai do contador das
// decisoes e do que esta na mesa, sem gastar sorteio nenhum (nem o da IA).
static float ms_sorte(void)
{
    uint32_t h = ia_conta * 2654435761u ^ (uint32_t)rodada * 40503u ^ (uint32_t)pote * 9973u
               ^ (uint32_t)a_pagar * 131u ^ (uint32_t)J[vez].fichas * 7919u ^ (uint32_t)vez * 0x51ED27u;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
    return (float)(h >> 8) / 16777216.0f;
}
// Sorteia um lance do plano q; o que quase nunca se joga (menos de 4%) e
// ruido da conta, e fica de fora.
static int ms_escolhe(const float *q, int n)
{
    float t = 0, c[3];
    for (int a = 0; a < n; a++) { c[a] = q[a] < 0.04f ? 0 : q[a]; t += c[a]; }
    if (t <= 0) return 0;
    float x = ms_sorte() * t;
    for (int a = 0; a < n; a++) { if (x < c[a]) return a; x -= c[a]; }
    return n - 1;
}

static int ms_apostou[2] = { -1, -1 };       // rodada * 32 + quanto: a aposta que a mestre abriu (para refazer a conta se dobrarem)
// O plano da ultima decisao de aposta: os lances e a chance de cada um (os
// testes e a arena olham).
static int ms_ult_ops[3], ms_ult_n;
static float ms_ult_q[3];

// Ele apostou (ou dobrou) e a mestre, que abriu, responde: acha o comeco da
// aposta (s0) e o caminho ate aqui. Devolve o no de agora (ou -1); *hb, o no
// em que ele decidiu; *ib, o lance dele la; *sb, a aposta naquela hora.
static int ms_refaz(const ms_ap_t *s, int j, ms_ap_t *s0, ms_ap_t *sb, int *hb, int *ib)
{
    for (int t = 0; t < 3; t++) {
        int x = 0;
        if (s->aumentou) {                       // a mestre apostou x e ele dobrou
            x = t == 0 ? (ms_apostou[j & 1] / 32 == rodada ? ms_apostou[j & 1] % 32 : 0) : t == 1 ? 10 : 25;
            if (x <= 0) continue;
            *s0 = (ms_ap_t){ .f = { s->f[0] + x, s->f[1] + x + s->a_pagar }, .pote = s->pote - 2 * x - s->a_pagar };
        } else {                                 // a mestre passou e ele apostou
            if (t) break;
            *s0 = (ms_ap_t){ .f = { s->f[0], s->f[1] + s->a_pagar }, .pote = s->pote - s->a_pagar, .passes = 0 };
        }
        if (s0->pote < 0) continue;
        int o0[3], n0 = ms_ops(s0, o0);
        for (int a = 0; a < n0; a++) {
            int o1 = o0[a];
            if (s->aumentou ? (o1 == OP_PASSAR || ms_lim(s0, o1 == OP_APOSTAR10 ? 10 : 25) != x) : o1 != OP_PASSAR) continue;
            *sb = *s0;
            if (ms_faz(sb, o1)) continue;
            int o2[3], nb = ms_ops(sb, o2);
            for (int c = 0; c < nb; c++) {
                ms_ap_t s2 = *sb;
                if (ms_faz(&s2, o2[c])) continue;
                if (s2.f[0] == s->f[0] && s2.f[1] == s->f[1] && s2.pote == s->pote && s2.a_pagar == s->a_pagar
                    && s2.aumentou == s->aumentou) { *hb = a + 1; *ib = c; return *hb * 4 + c + 1; }
            }
        }
    }
    return -1;
}

// O lance da mestre na aposta: o lugar dele em opcoes().
static int mestre_aposta(void)
{
    int j = vez, r = outro(j), pr = primeiro;
    bool abre = j == pr;
    ms_ap_t s = { .f = { J[pr].fichas, J[outro(pr)].fichas }, .pote = pote, .a_pagar = a_pagar,
                  .passes = passes, .vez = abre ? 0 : 1, .aumentou = aumentou };
    int ops[3], n = ms_ops(&s, ops);
    float p[MS_TIPOS + 1], w[MS_TIPOS + 1], q[3] = { 0 }, fr = ms_franca_jog[j & 1];
    int K = MS_TIPOS, certo = -1;                // certo: o lance, quando a conta da um so
    float crua;
    if (!abre) {
        // Quem joga depois sabe tudo: a chance da mao dele e a conta.
        float eu = ia_chance();
        if (a_pagar && aumentou) {               // dobraram a aposta dele: paga se vale
            float corre = ms_util(s.f[1], s.f[0] + s.pote, false);
            int b = s.a_pagar < s.f[1] ? s.a_pagar : s.f[1];
            float paga = eu * ms_util(s.f[1] + s.pote, s.f[0], true) + (1 - eu) * ms_util(s.f[1] - b, s.f[0] + s.pote + b, false);
            certo = paga >= corre ? OP_PAGAR : OP_CORRER;
        } else {
            // O que quem abriu responde depende do que ele imagina da mao: as
            // maos que esta poderia ser, e a de verdade no meio delas.
            crua = ms_tipos(j, p);
            p[K] = eu;
            for (int k = 0; k <= K; k++) w[k] = 1.0f / (float)(K + 1);
            ms_resolve(&s, p, w, K + 1, 1, fr, crua);
            ms_le(0, K, n, q);
        }
    } else {
        crua = ms_tipos(r, p);
        for (int k = 0; k < K; k++) w[k] = 1.0f / (float)K;
        ms_ap_t s0, sb;
        int hb = -1, ib = -1, h = 0;
        if (!a_pagar) {
            ms_resolve(&s, p, w, K, 0, fr, crua);
            ms_le(0, 0, n, q);
        } else if ((h = ms_refaz(&s, j, &s0, &sb, &hb, &ib)) >= 0) {
            // Ele apostou (ou dobrou): a aposta diz alguma coisa da mao dele.
            // A conta, refeita do comeco da aposta, diz com que maos ele faria
            // isso, e o plano dela para este no ja responde a essas maos.
            ms_resolve(&s0, p, w, K, 0, fr, crua);
            float pos[MS_TIPOS], massa = 0;
            int ob[3], nb = ms_ops(&sb, ob);
            for (int t = 0; t < 2 && massa < 0.03f; t++) {
                massa = 0;
                for (int k = 0; k < K; k++) {
                    float qb[3], qf[3];
                    ms_le(hb, k, nb, qb);
                    ms_franco(&sb, p[k], MS_LARG_MAO, ob, nb, qf);
                    for (int c = 0; c < nb; c++) qb[c] = (1 - fr) * qb[c] + fr * qf[c];
                    float v = t == 0 ? qb[ib] : 0;
                    if (t == 1) for (int c = 1; c < nb; c++) v += qb[c];
                    pos[k] = w[k] * v;
                    massa += pos[k];
                }
            }
            if (massa >= 0.03f) ms_le(h, 0, n, q);
            else {
                // Um lance que a conta nao espera de mao nenhuma (nem a outra
                // aposta dele): quanto mais forte a mao, mais ele aposta - e a
                // mestre responde o melhor contra essas maos.
                massa = 0;
                for (int k = 0; k < K; k++) { pos[k] = w[k] * p[k]; massa += pos[k]; }
                float melhor = -1;
                for (int a = 0; a < n; a++) {
                    ms_ap_t s2 = s;
                    int fim = ms_faz(&s2, ops[a]);
                    float u[MS_TIPOS], v = 0;
                    if (fim) ms_fim(&s2, fim, p, K, u);
                    else {                       // dobrou: ele paga so com o que vale a pena
                        ms_ap_t sc = s2, sp = s2;
                        float uc[MS_TIPOS], up[MS_TIPOS];
                        ms_fim(&sc, ms_faz(&sc, OP_CORRER), p, K, uc);
                        ms_fim(&sp, ms_faz(&sp, OP_PAGAR), p, K, up);
                        for (int k = 0; k < K; k++) u[k] = up[k] < uc[k] ? up[k] : uc[k];
                    }
                    for (int k = 0; k < K; k++) v += pos[k] * u[k];
                    if (v > melhor) { melhor = v; certo = ops[a]; }
                }
            }
        } else {
            // Nao deu para refazer a conta (nao devia acontecer): paga pelo preco.
            float eu = 0;
            for (int k = 0; k < K; k++) eu += w[k] * (1 - p[k]);
            certo = eu >= (float)a_pagar / (float)(pote + a_pagar) ? OP_PAGAR : OP_CORRER;
        }
    }
    if (certo >= 0) for (int a = 0; a < n; a++) q[a] = ops[a] == certo ? 1.0f : 0.0f;
    ms_ult_n = n;
    for (int a = 0; a < n; a++) { ms_ult_ops[a] = ops[a]; ms_ult_q[a] = q[a]; }
    int quer = ops[ms_escolhe(q, n)];
    if (abre && !a_pagar && (quer == OP_APOSTAR10 || quer == OP_APOSTAR25))
        ms_apostou[j & 1] = rodada * 32 + ms_lim(&s, quer == OP_APOSTAR10 ? 10 : 25);
    int op[4], no = opcoes(op);
    for (int i = 0; i < no; i++) if (op[i] == quer) return i;
    for (int i = 0; i < no; i++) if (op[i] == OP_PAGAR || op[i] == OP_PASSAR) return i;
    return 0;
}

// ---- A fila -----------------------------------------------------------------
// Como a afiada, testa as ordens da mao jogando de mentira (as mesmas rodadas
// imaginadas), mas:
//  - peneira aos poucos: todas as filas jogam um pouco, as melhores jogam
//    mais, e as melhores dessas mais ainda. A afiada escolhe 8 finalistas com
//    24 jogadas cada, e a sorte decide muito: medido (LAB=4 na arena), a fila
//    da mestre vence 0,8 ponto de chance a mais, abrindo ou jogando depois;
//  - quem joga depois conta so a chance de vencer: a folga no placar nao vale ficha;
//  - pensa em pedacos (ms_fatia): um pouco a cada quadro, sem travar a tela.
static void ms_poe_fila(int j, const int8_t *seq, int L)
{
    fila_t *f = &F[j];
    int k0 = f->grudado ? 1 : 0;                 // o grudado ja esta pousado no primeiro lugar
    if (!k0) memset(f, 0, sizeof *f);
    for (int k = k0; k < L; k++) {
        peca_t pc = J[j].col[J[j].mao[seq[k]]];
        f->idx[k] = J[j].mao[seq[k]];
        f->tipo[k] = pc.tipo;
        f->fixo[k] = pc.fixo;
    }
    f->n = L;
    if (L == 3) coringa_completa(j);             // o quarto do Coringa: a copia, ou a carta
}
#define MS_FOLGA 0.1f                        // a margem, para quem joga depois: so desempata
// Joga as amostras a0..a1-1 da fila e soma a nota em *s e a chance de vencer
// a rodada em *c (como avalia_fila_c, aos pedacos).
static void ms_avalia(int j, const int8_t *seq, int L, int a0, int a1, uint32_t base, const mesa_t *m, float *s, float *c)
{
    if (j != primeiro) ia_folga = MS_FOLGA;
    for (int a = a0; a < a1; a++) {
        volta_mesa(m);
        semeia_ia(base, a);
        ms_poe_fila(j, seq, L);
        *s += sim_resto(j, true);
        *c += placar(j);
        *s += ia_futuro_nota(j);
    }
    ia_folga = 1.0f;
}

enum { MS_E_PENEIRA, MS_E_TEIMA, MS_E_CURTA, MS_E_UNIVERSO, MS_E_PRONTO };
#define MS_LOTE (N_FILA - 1)                 // as do tira-teima, ou as filas curtas
typedef struct {
    uint32_t marca;                          // a situacao que este plano pensa (ms_marca)
    int etapa, j, L, gm, nc;
    uint32_t base;
    int volta, vivos, feitas, i, a;          // a peneira: a volta, quantas seguem, as jogadas de cada uma, a da vez
    int8_t seq[MS_LOTE][N_FILA];             // o lote da vez: o tira-teima, as filas curtas, o Universo
    int8_t Lq[MS_LOTE];
    float s[MS_LOTE], c[MS_LOTE];
    int n_lote, N;                           // quantas no lote e as jogadas de cada
    uint32_t base_lote;
    bool tem;                                // ja ha uma fila escolhida
    int8_t melhor[N_FILA];
    int melhor_n;
    float nota_m, chance_m;
} ms_pl_t;
static ms_pl_t ms_pl;
static int8_t ms_cand[MAX_CAND][N_FILA];
static int16_t ms_ord[MAX_CAND];
static float ms_soma[MAX_CAND];

// A situacao em que o plano vale: a rodada, a mao e o que esta na mesa.
static uint32_t ms_marca(int j)
{
    uint32_t h = 2166136261u;
    #define MS_MX(v) (h = (h ^ (uint32_t)(v)) * 16777619u)
    MS_MX(rodada); MS_MX(j); MS_MX(primeiro); MS_MX(J[j].n_mao); MS_MX(ia_esforco); MS_MX(max_fila(j));
    for (int m = 0; m < J[j].n_mao; m++) {
        const peca_t *pc = &J[j].col[J[j].mao[m]];
        MS_MX(J[j].mao[m]); MS_MX(pc->tipo); MS_MX(pc->fixo);
    }
    for (int q = 0; q < 2; q++) {
        MS_MX(F[q].lancados); MS_MX(F[q].n);
        for (int k = 0; k < F[q].lancados && k < N_FILA; k++) { MS_MX(F[q].tipo[k]); MS_MX(F[q].valor[k]); MS_MX(F[q].est[k]); }
    }
    #undef MS_MX
    return h | 1;
}

// As jogadas de cada volta da peneira e quantas filas seguem depois dela.
// (Medido: com metade das jogadas a fila piora; com uma vez e meia, nao melhora.)
#define MS_VOLTAS_PEN 4
static const int16_t MS_PENEIRA[MS_VOLTAS_PEN][2] = { { 16, 64 }, { 48, 16 }, { 128, 6 }, { 256, 2 } };
static const int16_t MS_PEN_POUCO[MS_VOLTAS_PEN][2] = { { 4, 8 }, { 8, 2 }, { 0, 2 }, { 0, 2 } };   // ia_esforco 0: os testes andam rapido
static int ms_n_teima(void) { return ia_esforco == 0 ? 24 : IA_AMOSTRAS; }

static void ms_abre_lote(int etapa, int n, int N, uint32_t base)
{
    ms_pl.etapa = etapa; ms_pl.n_lote = n; ms_pl.N = N; ms_pl.base_lote = base;
    ms_pl.i = ms_pl.a = 0;
    memset(ms_pl.s, 0, sizeof ms_pl.s);
    memset(ms_pl.c, 0, sizeof ms_pl.c);
}
// O Universo sozinho, contra a melhor fila sem ele (com um grudado na mesa,
// nao da: ele so vai sozinho).
static void ms_abre_universo(void)
{
    const jogador_t *p = &J[ms_pl.j];
    for (int i = 0; i < p->n_mao && ms_pl.L >= 2 && ms_pl.gm < 0; i++) {
        if (p->col[p->mao[i]].tipo != D_UNIVERSO) continue;
        ms_abre_lote(MS_E_UNIVERSO, 1, ms_n_teima(), ms_pl.base ^ 0x27d4eb2fu);
        memset(ms_pl.seq[0], 0, N_FILA);
        ms_pl.seq[0][0] = (int8_t)i;
        ms_pl.Lq[0] = 1;
        return;
    }
    ms_pl.etapa = MS_E_PRONTO;
}

static void ms_comeca(int j, uint32_t marca)
{
    const jogador_t *p = &J[j];
    memset(&ms_pl, 0, sizeof ms_pl);
    ms_pl.marca = marca;
    ms_pl.j = j;
    int n = p->n_mao;
    ms_pl.L = n < max_fila(j) ? n : max_fila(j);
    ms_pl.gm = -1;
    ms_pl.chance_m = 0.5f;
    ms_pl.base = 0xA5A5u + (uint32_t)rodada * 7919u + (uint32_t)j * 104729u;
    if (ms_pl.L <= 0) { ms_pl.etapa = MS_E_PRONTO; return; }
    for (int m = 0; m < n && F[j].grudado; m++) if (p->mao[m] == F[j].idx[0]) ms_pl.gm = m;
    ms_pl.nc = monta_candidatas(j, ms_pl.L, ms_pl.gm);
    memcpy(ms_cand, cand, sizeof(cand[0]) * (size_t)ms_pl.nc);   // (cand[] e tambem da afiada, que pode pensar entre dois pedacos)
    for (int c = 0; c < ms_pl.nc; c++) { ms_ord[c] = (int16_t)c; ms_soma[c] = 0; }
    ms_pl.vivos = ms_pl.nc;
    ms_pl.etapa = MS_E_PENEIRA;
    if (!ms_pl.nc) ms_abre_universo();
}

// Um pedaco do plano; devolve quanto trabalho fez (em jogadas de mentira).
static int ms_passo(const mesa_t *m)
{
    int j = ms_pl.j;
    if (ms_pl.etapa == MS_E_PENEIRA) {
        const int16_t (*pen)[2] = ia_esforco == 0 ? MS_PEN_POUCO : MS_PENEIRA;
        int N = pen[ms_pl.volta][0];
        if (ms_pl.i < ms_pl.vivos) {                 // a fila da vez joga as dela, em pedacos de ate 64
            int c = ms_ord[ms_pl.i], a1 = ms_pl.a + 64 < N ? ms_pl.a + 64 : N, feito = a1 - ms_pl.a;
            float chance = 0;
            ms_avalia(j, ms_cand[c], ms_pl.L, ms_pl.feitas + ms_pl.a, ms_pl.feitas + a1, ms_pl.base, m, &ms_soma[c], &chance);
            ms_pl.a = a1;
            if (a1 >= N) { ms_pl.i++; ms_pl.a = 0; }
            return feito;
        }
        // A volta acabou: as melhores na frente, e so elas seguem.
        for (int a = 1; a < ms_pl.vivos; a++) {
            int16_t c = ms_ord[a];
            int b = a;
            while (b > 0 && ms_soma[ms_ord[b - 1]] < ms_soma[c]) { ms_ord[b] = ms_ord[b - 1]; b--; }
            ms_ord[b] = c;
        }
        ms_pl.feitas += N;
        if (ms_pl.vivos > pen[ms_pl.volta][1]) ms_pl.vivos = pen[ms_pl.volta][1];
        ms_pl.volta++;
        ms_pl.i = 0;
        if (ms_pl.volta < MS_VOLTAS_PEN && ms_pl.vivos > 2 && pen[ms_pl.volta][0] > 0) return 1;
        // A sorte do vencedor: a primeira pode estar na frente so porque os
        // sorteios dela sairam bons. As duas melhores jogam de novo, com
        // sorteios novos, e vale essa medida.
        int n = ms_pl.vivos < 2 ? ms_pl.vivos : 2;
        ms_abre_lote(MS_E_TEIMA, n, ms_n_teima(), ms_pl.base ^ 0x27d4eb2fu);
        for (int i = 0; i < n; i++) { memcpy(ms_pl.seq[i], ms_cand[ms_ord[i]], N_FILA); ms_pl.Lq[i] = (int8_t)ms_pl.L; }
        return 1;
    }
    if (ms_pl.i < ms_pl.n_lote) {                // o lote: uma fila de cada vez, em pedacos
        int i = ms_pl.i, a1 = ms_pl.a + 64 < ms_pl.N ? ms_pl.a + 64 : ms_pl.N, feito = a1 - ms_pl.a;
        ms_avalia(j, ms_pl.seq[i], ms_pl.Lq[i], ms_pl.a, a1, ms_pl.base_lote, m, &ms_pl.s[i], &ms_pl.c[i]);
        ms_pl.a = a1;
        if (a1 >= ms_pl.N) { ms_pl.i++; ms_pl.a = 0; }
        return feito;
    }
    float N = (float)ms_pl.N;
    switch (ms_pl.etapa) {
    case MS_E_TEIMA: {
        int a = ms_pl.n_lote > 1 && ms_pl.s[1] > ms_pl.s[0] ? 1 : 0, n = 0;
        ms_pl.tem = true;
        memcpy(ms_pl.melhor, ms_pl.seq[a], N_FILA);
        ms_pl.melhor_n = ms_pl.L;
        ms_pl.nota_m = ms_pl.s[a] / N;
        ms_pl.chance_m = ms_pl.c[a] / N;
        // A fila pode ser mais curta: se so o comeco dela ja e melhor (o
        // Solitario sozinho, um dado grande e parar), a mestre monta so esse
        // comeco, em vez de por quatro dados na mesa e parar no primeiro.
        ms_abre_lote(MS_E_CURTA, ms_pl.L - 1, ms_pl.N, ms_pl.base_lote);
        for (int q = 1; q < ms_pl.L; q++) {
            memcpy(ms_pl.seq[n], ms_pl.melhor, N_FILA);
            ms_pl.Lq[n++] = (int8_t)q;
        }
        break;
    }
    case MS_E_CURTA:
        for (int i = 0; i < ms_pl.n_lote; i++)
            if (ms_pl.s[i] / N > ms_pl.nota_m + 0.01f) {
                ms_pl.nota_m = ms_pl.s[i] / N;
                ms_pl.chance_m = ms_pl.c[i] / N;
                ms_pl.melhor_n = ms_pl.Lq[i];
            }
        ms_abre_universo();
        break;
    case MS_E_UNIVERSO:
        if (!ms_pl.tem || ms_pl.s[0] / N > ms_pl.nota_m) {
            ms_pl.tem = true;
            memset(ms_pl.melhor, 0, N_FILA);
            ms_pl.melhor[0] = ms_pl.seq[0][0];
            ms_pl.melhor_n = 1;
            ms_pl.nota_m = ms_pl.s[0] / N;
            ms_pl.chance_m = ms_pl.c[0] / N;
        }
        ms_pl.etapa = MS_E_PRONTO;
        break;
    }
    return 1;
}

// Pensa a fila de quem esta na vez por mais um pedaco (trabalho: quantas
// jogadas de mentira cabem agora). true: o plano esta pronto (em plano[]).
// A mesa volta como estava a cada pedaco: entre um e outro, o jogo desenha.
// O plano e o mesmo, em quantos pedacos for: cada jogada tem o seu sorteio.
static bool ms_fatia(int trabalho)
{
    int j = vez;
    if (plano_rodada == rodada && plano_vez == j) return true;
    uint32_t marca = ms_marca(j);
    if (ms_pl.marca != marca) ms_comeca(j, marca);
    if (ms_pl.etapa != MS_E_PRONTO) {
        mesa_t m;
        guarda_mesa(&m);
        while (trabalho > 0 && ms_pl.etapa != MS_E_PRONTO) trabalho -= ms_passo(&m);
        volta_mesa(&m);
        if (ms_pl.etapa != MS_E_PRONTO) return false;
    }
    plano_rodada = rodada; plano_vez = j;
    plano_n = ms_pl.tem ? ms_pl.melhor_n : 0;
    if (ms_pl.tem) memcpy(plano, ms_pl.melhor, N_FILA);
    // A aposta usa a chance de vencer, nao a nota (que soma margem, fichas e
    // o que fica guardado para depois).
    plano_p = ms_pl.chance_m < 0 ? 0 : ms_pl.chance_m > 1 ? 1 : ms_pl.chance_m;
    return true;
}
static void mestre_planeja(void) { while (!ms_fatia(1 << 20)) {} }
// Enquanto a maquina "pensa" (a pausa antes de cada gesto), a mestre pensa de
// verdade, um pedaco por quadro: a fila e as maos da aposta. O resto (resolver
// a aposta, lancar ou parar) cabe num quadro. Devolve se ja pode decidir.
#define MS_FATIA 300                         // jogadas de mentira por quadro: uns 3 ms no PC
#define MS_FATIA_MAOS 10                     // maos da aposta por quadro: uns 2 ms
static bool mestre_pensa(void)
{
    if (fase == F_ORDEM) return ms_fatia(MS_FATIA);
    if (fase != F_APOSTA) return true;
    // Na aposta, quem joga depois ja pesa a fila. Com um Chiclete na mao, pesa
    // com ele grudado numa copia da mesa, como ia_chance (que depois acha o
    // plano pronto); na vez de verdade a situacao e a mesma, e o plano tambem.
    if (vez != primeiro && !F[vez].lancados) {
        int g = F[vez].n ? -1 : chiclete_na_mao(vez);
        if (g < 0) { if (!ms_fatia(MS_FATIA)) return false; }
        else {
            mesa_t m;
            guarda_mesa(&m);
            poe_grudado(vez, g);
            plano_rodada = -1;
            bool pronto = ms_fatia(MS_FATIA);
            volta_mesa(&m);
            plano_rodada = -1;
            if (!pronto) return false;
        }
    }
    if (vez != primeiro && a_pagar && aumentou) return true;   // dobraram a aposta dele: so a conta de pagar
    return ms_maos_fatia(vez == primeiro ? outro(vez) : vez, MS_FATIA_MAOS);
}

// ---- Lancar ou parar -----------------------------------------------------
// Como a afiada (parar agora contra lancar mais um, jogando de mentira);
// quem joga depois, sem a folga no placar.
static bool mestre_continua(void)
{
    if (vez != primeiro) ia_folga = MS_FOLGA;
    bool segue = ia_continua();
    ia_folga = 1.0f;
    return segue;
}

// A escolha desta fase pelo nivel (a mesma de sempre).
static int ia_escolha(void)
{
    switch (fase) {
    case F_LOJA:   return ia_nivel == 0 ? facil_compra() : ia_compra();
    case F_ORDEM:  return ia_nivel == 0 ? facil_proximo_da_fila() : ia_proximo_da_fila();
    case F_JOGA:   return ia_nivel == 0 ? facil_continua() : ia_nivel >= 3 ? mestre_continua() : ia_continua();
    case F_APOSTA: return ia_nivel == 0 ? facil_aposta() : ia_nivel >= 3 ? mestre_aposta() : ia_aposta();
    case F_PREMIO: return ia_nivel == 0 ? facil_premio() : ia_premio();
    }
    return -1;
}

// Uma passada aos pedacos: devolve se a escolha ficou pronta (em *r). Sem
// ficar pronta (ou so pensando, na pausa: 'so_pensa'), a semente da IA e o
// plano voltam como estavam: a passada nao deixa rastro alem das contas
// guardadas, e a passada que decide de fato faz o mesmo caminho de antes.
static bool ia_tenta(int *r, bool so_pensa)
{
    uint32_t sem = semente_ia;
    int8_t pl[N_FILA];
    memcpy(pl, plano, sizeof pl);
    int pn = plano_n, pr = plano_rodada, pv = plano_vez;
    float pp = plano_p;
    ia_passada_comeca();
    *r = ia_escolha();
    bool ok = !ia_faltou;
    ia_passada_fim();
    if (!ok || so_pensa) {
        semente_ia = sem;
        memcpy(plano, pl, sizeof pl);
        plano_n = pn; plano_rodada = pr; plano_vez = pv; plano_p = pp;
    }
    return ok;
}

// A loja da maquina contra uma pessoa (sem tela): as compras em sequencia,
// cada uma podendo levar alguns quadros aos pedacos.
static int ia_loja_k = -1, ia_loja_n;        // a compra da vez (-1: nenhuma comecou); quantas levou
static peca_t ia_loja_lev[N_LOJA];

static void ia_passo(float dt)
{
    ia_nivel = nivel_jog[vez & 1];
    ia_ousadia = ousadia_jog[vez & 1];
    // A Banqueira: aposta alto quando esta na frente em fichas e corre cedo
    // quando esta atras.
    if (modo == M_DESAFIO && (vez & 1) == 1 && estilo_op == 1) ia_ousadia = J[1].fichas > J[0].fichas ? 1.0f : -1.0f;
    if (!ia_na_vez() || (fase == F_PREMIO && (t_tira > 0 || slot_girando()))) { ia_fase = -1; ia_loja_k = -1; return; }
    // Contra uma pessoa, a loja e o premio da maquina nao tem tela: ela
    // decide na hora, e o anuncio mostra o que levou.
    bool direto = anuncia_maquina() && (fase == F_LOJA || fase == F_PREMIO);
    bool comprando = direto && fase == F_LOJA && ia_loja_k >= 0 && fase == ia_fase && vez == ia_vez;
    if (t_anuncio > 0 && !comprando) return;  // a pessoa ainda le o que ela fez
    if (fase != ia_fase || vez != ia_vez) {   // nova decisao: pensa um pouco
        ia_fase = fase; ia_vez = vez;
        t_ia = fase == F_ORDEM && !anuncia_maquina() ? -0.4f : 0;
        int p = modo == M_DESAFIO ? pensa_jog[vez & 1] : 0;
        uint32_t h = ++ia_conta * 2654435761u ^ (uint32_t)rodada * 40503u ^ (uint32_t)fase * 9973u
                   ^ (uint32_t)(F[vez].lancados * 16 + F[vez].n) * 131u ^ (uint32_t)J[vez].fichas * 7919u;
        h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
        ia_sobe = p > 0 && (int)(h % 100u) < p;
        ia_memo_zera();
        ia_loja_k = -1;
    }
    if (ia_sobe && ia_nivel < 2) ia_nivel++;
    t_ia += dt;
    // Aos pedacos (a plataforma deu um tempo por quadro), as contas andam ja
    // na pausa. A mestre pensa a fila e os lances do jeito dela (mestre_pensa).
    bool pedacos = ia_orcamento_us > 0 && ia_nivel >= 1
                && !(ia_nivel >= 3 && (fase == F_ORDEM || fase == F_JOGA || fase == F_APOSTA));
    // A mestre pensa a fila durante a pausa, um pedaco por quadro; se a pausa
    // acabar antes, espera o plano ficar pronto.
    bool pronta = ia_nivel < 3 || direto || mestre_pensa();
    if (!direto && t_ia < (fase == F_JOGA ? 0.3f : 0.55f)) {
        int r;
        if (pedacos) ia_tenta(&r, true);
        return;
    }
    if (!pronta) return;

    if (direto && fase == F_LOJA) {
        if (ia_loja_k < 0) { ia_loja_k = 0; ia_loja_n = 0; }
        while (ia_loja_k < N_LOJA) {
            int i;
            if (pedacos) { if (!ia_tenta(&i, false)) return; }   // o resto no proximo quadro
            else i = ia_escolha();
            ia_loja_k++;
            if (i < 0) break;
            int antes = J[vez].n_col;
            cursor = i;
            trata_tecla(KEY_ENTER);
            if (J[vez].n_col == antes) break;
            ia_loja_lev[ia_loja_n++] = loja[i];
        }
        t_ia = 0;
        ia_loja_k = -1;
        anuncia_pecas(ia_loja_n ? "%s comprou" : "%s não comprou nada", ia_loja_lev, ia_loja_n);
        trata_tecla(KEY_TAB);
        return;
    }
    int r;
    if (pedacos) { if (!ia_tenta(&r, false)) return; }   // ainda pensando: o resto no proximo quadro
    else r = ia_escolha();
    t_ia = 0;

    if (direto && fase == F_PREMIO) {
        int i = r;
        peca_t p = oferta[i];
        bool cabe = J[vez].n_col < lim_col();
        cursor = i;
        anuncia_pecas(cabe ? "%s levou o prêmio" : "%s ficou sem prêmio", &p, cabe ? 1 : 0);
        trata_tecla(KEY_ENTER);
        return;
    }

    switch (fase) {
    case F_LOJA:
        if (r >= 0) { cursor = r; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    case F_ORDEM: {
        // Contra uma pessoa, monta a sequencia inteira de uma vez: o menu
        // dela nao aparece, so os dados indo para a fila. (O primeiro dado e
        // o que pesa: os outros saem do plano ja feito.)
        int vezes = anuncia_maquina() ? N_FILA + 1 : 1;
        for (int n = 0; n < vezes && fase == F_ORDEM; n++) {
            int i = n == 0 ? r : ia_escolha();
            if (i >= 0) { cursor = i; trata_tecla(KEY_ENTER); }
            else { trata_tecla(KEY_TAB); break; }
        }
        break;
    }
    case F_JOGA:
        if (r) { cursor = 0; trata_tecla(KEY_ENTER); }
        else trata_tecla(KEY_TAB);
        break;
    case F_APOSTA:
        cursor = r;
        trata_tecla(KEY_ENTER);
        break;
    case F_PREMIO:
        cursor = r;
        trata_tecla(KEY_ENTER);
        break;
    }
}
