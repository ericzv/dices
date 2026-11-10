// Varredura das regras: sorteia mesas inteiras (dados de todos os tipos, com
// adesivos nos lados, amuletos, trunfos e bolsas do Desafiante, quem abre,
// filas de 1 a 6) e joga cada uma pelo caminho de verdade do jogo - a compra,
// o lance (lanca_proximo), o pouso (pousa), o fim do turno (termina_turno), o
// desfecho, os eventos um a um, o pagamento e a rodada seguinte -, conferindo
// depois de cada pouso e no fim o que tem que valer sempre:
//
//   * a previa (calcula_desfecho sem gerar) nao muda nada na mesa nem no
//     estado das regras, e duas previas seguidas dao o mesmo; o mesmo vale
//     para as consultas da mesa (risco, miras) e para a IA pensando;
//   * sem nada ao acaso no fim (fim_incerto), a previa e o desfecho de
//     verdade dao o mesmo, campo por campo;
//   * depois do desfecho nenhum dado fica esperando, nem em V_ESPERA;
//   * valores, lados, estados e indices dentro dos limites; nenhum evento
//     aponta para dado que nao existe;
//   * os eventos, aplicados um a um, chegam no placar e nos estados que o
//     motor pagou; as fichas nao somem nem aparecem;
//   * o Carrossel para no numero que a mesa mostrava nele;
//   * refazer uma sequencia sem nada ter mudado da a mesma sequencia;
//   * a mesma semente da o mesmo resultado, com ou sem as previas e as
//     consultas no meio (o online anda igual dos dois lados por isso).
//
//   varredura [mesas] [semente]        mesas ao acaso (padrao: 3000)
//   varredura --uma SEMENTE            uma mesa so, escrita rodada a rodada
//
// Cada mesa joga de uma a quatro rodadas seguidas. Cada falha escreve a
// semente da mesa: --uma repete so ela. Compile com
// -fsanitize=address,undefined para pegar erro de memoria.
#include <stdio.h>
#include "../src/games/dado.c"

static uint16_t fb[GFX_W * GFX_H];
uint16_t *display_fb(void) { return fb; }
int64_t esp_timer_get_time(void) { return 1; }
void som_toca(int s) { (void)s; }
void som_toca_tom(int s, int t) { (void)s; (void)t; }
bool som_liga(bool on) { (void)on; return false; }
bool som_ligado(void) { return false; }
bool som_falhou(void) { return false; }
void som_rufo(bool on) { (void)on; }
void som_rufo_moeda(void) {}
void som_giro(bool on) { (void)on; }
void som_suspense(bool on) { (void)on; }
void som_cadeia(bool on) { (void)on; }
void som_chefe(int q, bool t) { (void)q; (void)t; }
void som_volume(int c, int n) { (void)c; (void)n; }
int som_volume_de(int c) { (void)c; return SOM_NIVEIS; }

#include "salva_mem.h"

// ---------------------------------------------------------------------------
// O sorteio da varredura (o do jogo, 'semente', e semeado por ele)
// ---------------------------------------------------------------------------
static uint64_t vs;
static uint32_t vr(void)
{
    vs += 0x9E3779B97F4A7C15ull;
    uint64_t z = vs;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return (uint32_t)((z ^ (z >> 31)) >> 16);
}
static int vn(int n) { return n > 0 ? (int)(vr() % (uint32_t)n) : 0; }
static bool vp(int pct) { return vn(100) < pct; }

// Cada mesa e jogada de dois jeitos, e o desfecho tem que ser o mesmo: com as
// previas, as consultas e a tela dos efeitos no meio (a cheia), e so com as
// regras (a seca).
enum { VAR_CHEIA, VAR_SECA };

// ---------------------------------------------------------------------------
// As falhas
// ---------------------------------------------------------------------------
static int falhas, rodadas_ruins;
static uint32_t semente_rodada;
static int variante_agora;
static bool falhou_rodada, fala;                 // fala: --uma (escreve a mesa)
static void escreve_mesa(void);
static void falha(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void falha(const char *fmt, ...)
{
    falhas++;
    if (!falhou_rodada) rodadas_ruins++;
    if (rodadas_ruins > 12 && !fala) { falhou_rodada = true; return; }   // ja deu para ver
    printf("FALHOU [semente %u, variante %d] ", semente_rodada, variante_agora);
    va_list a;
    va_start(a, fmt);
    vprintf(fmt, a);
    va_end(a);
    putchar('\n');
    if (!falhou_rodada) escreve_mesa();
    falhou_rodada = true;
}
#define CONFERE(cond, ...) do { if (!(cond)) falha(__VA_ARGS__); } while (0)

static const char *nome_est(int e)
{
    static const char *N[4] = { "espera", "valido", "anulado", "roubado" };
    return e >= 0 && e < 4 ? N[e] : "???";
}
static void escreve_mesa(void)
{
    printf("  modo %d  rodada %d  primeiro %d  vez %d  pote %d  fase %d  reg %d  bolsa %d  nivel %d  perso %d  natal %d\n",
           modo, rodada, primeiro, vez, pote, fase, reg_op, modo == M_DESAFIO ? run.bolsa : -1,
           modo == M_DESAFIO ? run.dificuldade : -1, fala_perso, natal);
    for (int j = 0; j < 2; j++) {
        const fila_t *f = &F[j];
        printf("  jogador %d: fichas %d (ini %d)  n %d  lancados %d  pontos %d  carta %d  grudado %d  amuletos",
               j, J[j].fichas, fichas_ini[j], f->n, f->lancados, f->pontos, f->carta, f->grudado);
        for (int a = 0; a < A_N; a++) if (tem_am(j, a)) printf(" %s", AMULETO[a].nome);
        putchar('\n');
        for (int k = 0; k < f->n && k < N_FILA; k++) {
            int t = f->tipo[k];
            printf("    %d: %-14s", k, t < D_N ? TIPO[t].nome : "???");
            if (k < f->lancados) {
                printf(" valor %d cru %d lado %d %s bonus %d fixo %d", f->valor[k], f->cru[k], f->lado[k],
                       nome_est(f->est[k]), f->bonus[k], f->fixo[k]);
                int a = ad_fila(j, k);
                if (a) printf(" [%s]", ADESIVO[a].nome);
                if (f->escondido[k]) printf(" escondido");
                if (f->curinga[k]) printf(" curinga");
                if (f->curinga_nada[k]) printf(" curinga_nada");
                if (f->gira[k]) printf(" gira");
                if (f->espera[k]) printf(" espera");
                if (f->ancora[k]) printf(" ancora");
                if (f->calibre[k]) printf(" calibre");
                if (f->copiou[k]) printf(" copiou");
                if (f->bomba[k]) printf(" bomba");
                if (tipo_de(j, k) != t) printf(" (como %s)", TIPO[tipo_de(j, k)].nome);
            } else printf(" (por lancar) fixo %d", f->fixo[k]);
            const peca_t *p = peca_fila(j, k);
            if (p && ad_quantos(p)) {
                printf("  adesivos:");
                for (int l = 1; l <= AD_LADOS; l++) if (ad_no_lado(p, l)) printf(" %d=%s", l, ADESIVO[ad_no_lado(p, l)].nome);
            }
            if (p && p->capa) printf("  capa");
            putchar('\n');
        }
    }
}

// ---------------------------------------------------------------------------
// O retrato do estado das regras, para comparar antes e depois
// ---------------------------------------------------------------------------
typedef struct {
    fila_t f[2];
    jogador_t j[2];
    uint32_t semente, hash;
    int pote, primeiro, vez, rodada, zerou, vence_j, n_ev, ver_tipo, anulou_a, anulou_b, resistiu;
    int fichas_ini[2], bis_fila[2], bis_vem[2], chic_valor[2], ad_moedas[2];
    uint8_t chic_lado[2], anulou_masc;
    int8_t ferr_k[2];
    amul_t amul[2];
    int16_t embrulho;
    bool ver_comum, carta_virando;
    uint8_t pavio_mais[2][N_FILA];
    char veredito[sizeof veredito];
    evento_t ev[MAX_EV];
    desfecho_t resolvido;
} retrato_t;

static void retrata(retrato_t *r)
{
    memset(r, 0, sizeof *r);
    memcpy(r->f, F, sizeof F);
    memcpy(r->j, J, sizeof J);
    r->semente = semente; r->hash = hash_partida();
    r->pote = pote; r->primeiro = primeiro; r->vez = vez; r->rodada = rodada; r->zerou = zerou; r->vence_j = vence_j;
    r->n_ev = n_ev; r->ver_tipo = ver_tipo; r->anulou_a = anulou_a; r->anulou_b = anulou_b; r->resistiu = resistiu;
    r->anulou_masc = anulou_masc; r->ver_comum = ver_comum; r->carta_virando = carta_virando;
    r->embrulho = embrulho;
    for (int j = 0; j < 2; j++) {
        r->fichas_ini[j] = fichas_ini[j]; r->bis_fila[j] = bis_fila[j]; r->bis_vem[j] = bis_vem[j];
        r->chic_valor[j] = chic_valor[j]; r->chic_lado[j] = chic_lado[j]; r->ad_moedas[j] = ad_moedas[j];
        r->ferr_k[j] = ferr_k[j]; r->amul[j] = amul[j];
    }
    memcpy(r->pavio_mais, pavio_mais, sizeof pavio_mais);
    memcpy(r->veredito, veredito, sizeof veredito);
    memcpy(r->ev, ev, sizeof ev);
    memcpy(&r->resolvido, &resolvido, sizeof resolvido);
}

// O campo da fila em que o byte 'o' cai (para dizer o que a previa mexeu).
#define CAMPO(c) { #c, offsetof(fila_t, c), sizeof ((fila_t *)0)->c }
static const struct { const char *nome; size_t de, tam; } CAMPOS_FILA[] = {
    CAMPO(idx), CAMPO(tipo), CAMPO(fixo), CAMPO(n), CAMPO(lancados), CAMPO(cru), CAMPO(valor), CAMPO(lado), CAMPO(est),
    CAMPO(dobra), CAMPO(bonus), CAMPO(quebra), CAMPO(cinza), CAMPO(descarte), CAMPO(espera), CAMPO(ancora),
    CAMPO(grudado), CAMPO(chiclete), CAMPO(calibre), CAMPO(bomba), CAMPO(bomba1), CAMPO(copiou), CAMPO(carta),
    CAMPO(escondido), CAMPO(fruta), CAMPO(presente), CAMPO(faixa), CAMPO(curinga), CAMPO(curinga_nada), CAMPO(gira),
    CAMPO(abalo), CAMPO(guarda), CAMPO(bonecas), CAMPO(tag), CAMPO(tag_num), CAMPO(tag_mais), CAMPO(pontos), CAMPO(total),
};
static const char *campo_fila(size_t o)
{
    for (size_t i = 0; i < sizeof CAMPOS_FILA / sizeof CAMPOS_FILA[0]; i++)
        if (o >= CAMPOS_FILA[i].de && o < CAMPOS_FILA[i].de + CAMPOS_FILA[i].tam) return CAMPOS_FILA[i].nome;
    return "(enchimento)";
}
#define CAMPO_D(c) { #c, offsetof(desfecho_t, c), sizeof ((desfecho_t *)0)->c }
static const struct { const char *nome; size_t de, tam; } CAMPOS_DESFECHO[] = {
    CAMPO_D(est), CAMPO_D(bonus), CAMPO_D(mult), CAMPO_D(queima), CAMPO_D(semente), CAMPO_D(roubo_val), CAMPO_D(delta),
    CAMPO_D(total), CAMPO_D(bruto), CAMPO_D(atacados), CAMPO_D(agiota), CAMPO_D(medusa), CAMPO_D(come), CAMPO_D(come_de),
    CAMPO_D(garimpo), CAMPO_D(onda), CAMPO_D(meia), CAMPO_D(moedas), CAMPO_D(bis), CAMPO_D(gorjeta), CAMPO_D(fermento),
    CAMPO_D(raspa), CAMPO_D(raspa_lados), CAMPO_D(selo_rompe), CAMPO_D(selo_lacra), CAMPO_D(velcro), CAMPO_D(fragil),
    CAMPO_D(capa), CAMPO_D(ratoeira), CAMPO_D(rato_bits), CAMPO_D(coleira_bits), CAMPO_D(fig_cresce), CAMPO_D(fermento2),
    CAMPO_D(fig_cresce2), CAMPO_D(bilhete), CAMPO_D(dinamite), CAMPO_D(balao), CAMPO_D(bil_carimbo), CAMPO_D(bil_holo),
};
static const char *campo_desfecho(size_t o)
{
    for (size_t i = 0; i < sizeof CAMPOS_DESFECHO / sizeof CAMPOS_DESFECHO[0]; i++)
        if (o >= CAMPOS_DESFECHO[i].de && o < CAMPOS_DESFECHO[i].de + CAMPOS_DESFECHO[i].tam) return CAMPOS_DESFECHO[i].nome;
    return "(enchimento)";
}
// Diz a primeira diferenca entre dois retratos (ou devolve false).
static bool difere(const retrato_t *a, const retrato_t *b, char *onde, size_t n)
{
    if (!memcmp(a, b, sizeof *a)) return false;
    for (int j = 0; j < 2; j++) {
        const uint8_t *x = (const uint8_t *)&a->f[j], *y = (const uint8_t *)&b->f[j];
        for (size_t o = 0; o < sizeof(fila_t); o++)
            if (x[o] != y[o]) { snprintf(onde, n, "F[%d].%s (byte %d: %d vira %d)", j, campo_fila(o), (int)o, x[o], y[o]); return true; }
    }
    for (int j = 0; j < 2; j++) {
        const uint8_t *x = (const uint8_t *)&a->j[j], *y = (const uint8_t *)&b->j[j];
        for (size_t o = 0; o < sizeof(jogador_t); o++)
            if (x[o] != y[o]) { snprintf(onde, n, "J[%d] (byte %d: %d vira %d)", j, (int)o, x[o], y[o]); return true; }
    }
    if (a->semente != b->semente) snprintf(onde, n, "o sorteio da partida (semente)");
    else if (memcmp(a->ev, b->ev, sizeof a->ev) || a->n_ev != b->n_ev) snprintf(onde, n, "os eventos");
    else if (memcmp(&a->resolvido, &b->resolvido, sizeof a->resolvido)) snprintf(onde, n, "o desfecho pago");
    else if (memcmp(a->veredito, b->veredito, sizeof a->veredito) || a->ver_tipo != b->ver_tipo || a->ver_comum != b->ver_comum
             || a->anulou_a != b->anulou_a || a->anulou_b != b->anulou_b || a->resistiu != b->resistiu
             || a->anulou_masc != b->anulou_masc) snprintf(onde, n, "o veredito do ultimo lance");
    else snprintf(onde, n, "o resto do estado (pote, vez, amuletos, Ferradura, Chiclete...)");
    return true;
}

static uint32_t mistura(uint32_t h, const void *p, size_t n)
{
    const uint8_t *b = p;
    for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}

// ---------------------------------------------------------------------------
// A mesa sorteada
// ---------------------------------------------------------------------------
static void zera_jogo(void)
{
    memset(F, 0, sizeof F);
    memset(J, 0, sizeof J);
    memset(voo, 0, sizeof voo);
    memset(ev, 0, sizeof ev);
    memset(&resolvido, 0, sizeof resolvido);
    memset(&run, 0, sizeof run);
    memset(perfis, 0, sizeof perfis);
    snprintf(perfis[0].nome, sizeof perfis[0].nome, "VARRE");
    perfis[0].nivel = N_NIVEIS;                  // tudo aberto: o Presente desembrulha qualquer raro
    perfis[0].marcos = 0xFF;
    perfil_atual = 0;
    perfis_lidos = true;
    J[0].nome = "VOCE"; J[1].nome = "RIVAL";
    J[0].cor = C_LATAO; J[1].cor = C_ROSA;
    n_voo = n_ev = ev_i = 0;
    fase = F_ORDEM;
    vez = primeiro = 0; rodada = 1; pote = 0; cursor = 0; partidas = 0;
    a_pagar = passes = 0; aumentou = false;
    zerou = vence_j = -1; correu_venc = -1; venc_mao = venc_partida = 0;
    ferr_k[0] = ferr_k[1] = -1;
    amul[0] = amul[1] = 0; op_amul_extra = -1; am_disparou = 0;
    reg_op = REG_NADA; estilo_op = 0; maluco_lado = 0; juros_rodada = 0;
    modo = M_DOIS; cpu = 0; remoto = 0; baixo = 0;
    embrulho = -1; embrulho_tipo = 0;
    carta_virando = false; op_nervoso = false;
    fala_perso = -1; perso_virado = -1;
    des_rod_vencidas = des_rod_cheias = 0; des_atras7 = false; des_saiu = false;
    memset(bis_fila, 0, sizeof bis_fila); memset(bis_vem, 0, sizeof bis_vem);
    memset(ad_moedas, 0, sizeof ad_moedas);
    memset(chic_valor, 0, sizeof chic_valor); memset(chic_lado, 0, sizeof chic_lado);
    memset(fichas_ini, 0, sizeof fichas_ini);
    memset(pavio_mais, 0, sizeof pavio_mais);
    memset(cosmo_recem, 0, sizeof cosmo_recem);
    memset(jingle_copias, 0, sizeof jingle_copias);
    n_pouso = 0; n_devolvidos = 0; t_devolve = -1;
    hora = 0; t_hora = 0; calado_j = calado_k = -1;
    lance_no_maximo = false; bonecas_ult = 0;
    ev_decisivo = -1; ev_espera = false; t_respiro = -1;
    t_misterio = 0; t_fantasma = 0; t_virada = 0; t_congela = 0; t_onda = -1; t_tremor = 0;
    n_confete = 0;
    anulou_a = anulou_b = resistiu = -1; anulou_masc = 0; ver_tipo = 0; ver_comum = false;
    aviso[0] = veredito[0] = nota[0] = anuncio[0] = fala_txt[0] = 0;
    memset(vis_est, 0, sizeof vis_est); memset(vis_roubo, 0, sizeof vis_roubo);
    memset(vis_nroubo, 0, sizeof vis_nroubo); memset(vis_delta, 0, sizeof vis_delta);
    memset(vis_bonus, 0, sizeof vis_bonus); memset(vis_mult, 0, sizeof vis_mult);
    memset(vis_queima, 0, sizeof vis_queima); memset(vis_corte, 0, sizeof vis_corte);
    memset(vis_pedra, 0, sizeof vis_pedra); memset(vis_capa_t, 0, sizeof vis_capa_t);
    memset(vis_gelo, 0, sizeof vis_gelo); memset(vis_golpe_cor, 0, sizeof vis_golpe_cor);
    memset(vis_golpe_t, 0, sizeof vis_golpe_t); memset(vis_selo, 0, sizeof vis_selo);
    memset(vis_fragil, 0, sizeof vis_fragil); memset(vis_tsunami, 0, sizeof vis_tsunami);
    memset(vis_espelho, 0, sizeof vis_espelho); memset(vis_oculto, 0, sizeof vis_oculto);
    memset(vis_curinga, 0, sizeof vis_curinga); memset(vis_carrossel, 0, sizeof vis_carrossel);
    memset(vis_car_n, 0, sizeof vis_car_n); memset(vis_car_de, 0, sizeof vis_car_de);
    memset(vis_car_ate, 0, sizeof vis_car_ate); memset(vis_car_t, 0, sizeof vis_car_t);
    memset(vis_tremor, 0, sizeof vis_tremor); memset(vis_tremor_de, 0, sizeof vis_tremor_de);
    memset(est_pre_espelho, 0, sizeof est_pre_espelho); memset(vis_semente, 0, sizeof vis_semente);
    semente_v = 88172645u;
    t_relogio = 10; t_fase = 0;
}

// Um tipo de dado que pode estar numa bolsa: os que saem em sorteio e, mais
// raro, os que so nascem dentro da partida (o Fantasma, o Tsunami, o ciclo
// cosmico, o Dado Misterioso, o Mais Adesivado).
static int tipo_ao_acaso(void)
{
    for (;;) {
        int t = vn(D_N);
        if (aposentado(t) || fora_de_epoca(t)) continue;
        if (sem_sorteio(t) && !vp(15)) continue;
        if (modo != M_DESAFIO && so_desafio(t)) continue;
        return t;
    }
}

// O que cada dado guarda de uma rodada para a outra (o fixo), dentro do que
// o jogo deixa la.
static void guarda_ao_acaso(peca_t *p)
{
    switch (p->tipo) {
    case D_QUEBRADO:   p->fixo = vp(85) ? (uint8_t)(1 + vn(8)) : (uint8_t)(int8_t)(vp(50) ? -(1 + vn(12)) : 9 + vn(30)); break;
    case D_FANTASMA:   p->fixo = 1; break;
    case D_ACUMULADOR: p->fixo = (uint8_t)vn(ACUM_MAX + 1); break;
    case D_AUTOFAGIA:  p->fixo = (uint8_t)vn(AUTO_MAX + 1); break;
    case D_PREVISIVEL: p->fixo = (uint8_t)vn(12); break;
    case D_BALAO:      p->fixo = (uint8_t)(vp(80) ? vn(20) : vn(BALAO_GUARDA + 1)); break;
    case D_FURIA:      p->fixo = (uint8_t)(int8_t)(vn(21) - 6); break;
    case D_ONDA:       p->fixo = (uint8_t)vn(ONDA_VIRA); break;
    case D_PATINHOS:   p->fixo = (uint8_t)vn(9); break;
    case D_REVOLVER:   p->fixo = (uint8_t)vn(REVOLVER_BALAS); break;
    case D_PISCA:      p->fixo = (uint8_t)vn(2); break;
    case D_MEIA:       p->fixo = (uint8_t)(vp(50) ? 0 : MEIA_GUARDA * (1 + vn(6))); break;
    case D_JINGLE:     p->fixo = (uint8_t)vp(20); break;
    default: break;
    }
}

// Adesivos nos lados que o dado mostra (no maximo dois Chumbos, como na loja).
static void cola_ao_acaso(peca_t *p)
{
    if (cosmo_oculto(p->tipo) || p->tipo == D_MAIS_ADESIVADO) return;   // o ciclo cosmico evoluido vem liso
    static const int8_t CHANCE[6] = { 0, 0, 12, 30, 60, 100 };
    int chance = CHANCE[vn(6)];
    if (!chance) return;
    for (int l = ad_lado_min(p->tipo); l <= ad_lado_max(p->tipo) && l <= AD_LADOS; l++) {
        if (!ad_lado_serve(p, l) || ad_no_lado(p, l) || !vp(chance)) continue;
        int a = 1 + vn(AD_N - 1);
        if (ad_aposentado(a) || (a == AD_CHUMBO && ad_conta(p, AD_CHUMBO) >= CHUMBO_MAX)) continue;
        if (a == AD_DOURADO && modo != M_DESAFIO) continue;
        ad_cola(p, l, a);
    }
    if (vp(10)) p->fermento = (uint8_t)vn(FERMENTO_MAX + 1);
    if (vp(10)) p->figurinha = (uint8_t)vn(FIGURINHA_MAX + 1);
    if (vp(8)) p->selo = 1;
    if (vp(6)) p->capa = 1;
    if (vp(4)) p->velcro = 1;
}

static int rola_v_(int lados) { return 1 + vn(lados); }
static bool mesa_ia;                             // nesta mesa quem joga e a IA do jogo (confere_ia)

static void bolsa_ao_acaso(int j, const int *paleta, int n_paleta)
{
    jogador_t *p = &J[j];
    p->n_col = vp(4) ? 4 + vn(4) : 8 + vn(9);
    for (int i = 0; i < p->n_col; i++) {
        int t = n_paleta && vp(70) ? paleta[vn(n_paleta)] : tipo_ao_acaso();
        peca_t c = { .tipo = (uint8_t)t };
        if (t == D_QUEBRADO) c.fixo = 1;
        if (t == D_ADESIVADO || t == D_MAIS_ADESIVADO) adesiva(&c, rola_v_, modo == M_DESAFIO);
        if (t == D_COLCHA) colcha_remenda(&c);
        guarda_ao_acaso(&c);
        cola_ao_acaso(&c);
        p->col[i] = c;
        p->onde[i] = NA_BOLSA;
        if (ad_conta(&c, AD_DINAMITE) && vp(50)) p->pavio[i] = (uint8_t)(DINAMITE_CADA * vn(DINAMITE_MAX / DINAMITE_CADA + 1));
    }
}

// Uma mesa, do zero: o modo, os amuletos e trunfos, as bolsas, a compra de
// cada um e quem abre.
static void monta_mesa(void)
{
    zera_jogo();
    natal = vp(50); dezembro = natal && vp(30); reveillon = false;
    modo = vp(60) ? M_DESAFIO : vp(50) ? M_UM : M_DOIS;
    rodada = 1 + vn(modo == M_DESAFIO ? DES_RODADAS + DES_EXTRAS : MAX_RODADAS);
    if (modo == M_DESAFIO) {
        int n = vn(AM_LUGARES + 1);
        for (int i = 0; i < n; i++) amul[0] |= AM_BIT(vn(A_N));
        if (vp(35)) {
            amul[1] |= AM_BIT(vn(A_N));
            if (vp(40)) { int a = vn(A_N); amul[1] |= AM_BIT(a); op_amul_extra = a; }
        }
        if (vp(35)) reg_op = (uint8_t)(1 + vn(REG_DUENDE));
        run.bolsa = (uint8_t)(vp(50) ? B_PANO : vn(B_N));
        run.dificuldade = (uint8_t)vn(N_NIVEIS + 1);
        fala_perso = vn(N_PERSO);
        if (reg_op == REG_MALUCO) maluco_lado = (int8_t)(vp(50) ? 2 : -1);
        op_nervoso = vp(30);
    }
    // Metade das mesas tira os dados de uma paleta curta: os mesmos tipos se
    // repetem e se encontram (dois Carrosseis, o Camaleao atras de um deles).
    int paleta[6], n_paleta = vp(50) ? 2 + vn(5) : 0;
    for (int i = 0; i < n_paleta; i++) paleta[i] = tipo_ao_acaso();
    for (int j = 0; j < 2; j++) {
        bolsa_ao_acaso(j, paleta, n_paleta);
        J[j].fichas = vp(10) ? vn(12) : 20 + vn(180);
        J[j].perdidas = vn(rodada);
        J[j].perdeu_ultima = vp(40);
        fichas_ini[j] = J[j].fichas + ante_da(rodada);
        if (vp(6)) bis_vem[j] = 1;
    }
    pote = 2 * ante_da(rodada) + (vp(40) ? vn(60) : 0);
    primeiro = vn(2);
    mesa_ia = vn(2000) == 0;                    // (ela pensa devagar: poucas mesas)
    semente = vr() | 1;
    for (int j = 0; j < 2; j++) compra(&J[j]);
    embrulha_um();
    // O Chiclete da rodada de antes: um dado da mao ja vem grudado.
    for (int j = 0; j < 2; j++) {
        if (!J[j].n_mao || !vp(5)) continue;
        int i = J[j].mao[vn(J[j].n_mao)], t = J[j].col[i].tipo;
        if (t == D_UNIVERSO || t == D_PRESENTE) continue;
        J[j].col[i].chiclete = 1;
        chic_lado[j] = (uint8_t)(ad_lado_min(t) + vn(ad_n_lados(t)));
        chic_valor[j] = t == D_VAZIO ? 0 : chic_lado[j];
    }
}

static retrato_t ret_a, ret_b;

// Algumas mesas quem joga e a IA do jogo (a fila que ela planeja, parar ou
// seguir): pensando, ela joga rodadas de mentira na propria mesa, e tem que
// deixar tudo como estava.
static void confere_ia(const char *o_que, int variante)
{
    char onde[160];
    if (variante != VAR_CHEIA) return;
    retrata(&ret_b);
    if (difere(&ret_a, &ret_b, onde, sizeof onde)) falha("a IA, %s, mexeu em %s", o_que, onde);
}

// Monta a sequencia de j com dados da mao, como a tela da ordem (o Universo
// so vai sozinho; o grudado pelo Chiclete ja esta la).
static void monta_fila(int j, int variante)
{
    jogador_t *p = &J[j];
    fila_t *f = &F[j];
    if (mesa_ia) {
        retrata(&ret_a);
        plano_rodada = -1;
        ia_planeja();
        confere_ia("planejando a sequencia", variante);
        for (int k = f->n; k < plano_n && k < max_fila(j); k++) {
            int c = p->mao[plano[k]];
            f->idx[k] = (int8_t)c;
            f->tipo[k] = p->col[c].tipo;
            f->fixo[k] = p->col[c].fixo;
            f->n = k + 1;
        }
        coringa_completa(j);
        return;
    }
    int quer = 1 + vn(max_fila(j));
    if (vp(55)) quer = max_fila(j);
    bool usada[MAO] = { false };
    for (int tenta = 0; tenta < 40 && f->n < quer && f->n < max_fila(j); tenta++) {
        if (!p->n_mao) break;
        int i = vn(p->n_mao), c = p->mao[i];
        if (usada[i]) continue;
        bool ja = false;
        for (int k = 0; k < f->n; k++) ja |= f->idx[k] == c;
        if (ja) continue;
        bool uni = p->col[c].tipo == D_UNIVERSO;
        for (int k = 0; k < f->n; k++) uni |= f->tipo[k] == D_UNIVERSO;
        if (uni && f->n > 0) continue;
        usada[i] = true;
        f->idx[f->n] = (int8_t)c;
        f->tipo[f->n] = p->col[c].tipo;
        f->fixo[f->n] = p->col[c].fixo;
        f->n++;
    }
    coringa_completa(j);
}

// ---------------------------------------------------------------------------
// As conferencias
// ---------------------------------------------------------------------------
static void confere_limites(const char *quando, bool no_fim)
{
    CONFERE(pote >= 0, "%s: pote %d", quando, pote);
    for (int j = 0; j < 2; j++) {
        const fila_t *f = &F[j];
        const jogador_t *p = &J[j];
        CONFERE(p->fichas >= 0, "%s: fichas de %d: %d", quando, j, p->fichas);
        CONFERE(p->n_col >= 0 && p->n_col <= MAX_COL, "%s: bolsa de %d com %d dados", quando, j, p->n_col);
        CONFERE(p->n_mao >= 0 && p->n_mao <= MAO, "%s: mao de %d com %d dados", quando, j, p->n_mao);
        if (p->n_col < 0 || p->n_col > MAX_COL || p->n_mao < 0 || p->n_mao > MAO) return;
        for (int i = 0; i < p->n_mao; i++)
            CONFERE(p->mao[i] >= 0 && p->mao[i] < p->n_col, "%s: mao de %d aponta para %d (bolsa de %d)", quando, j, p->mao[i], p->n_col);
        for (int i = 0; i < p->n_col; i++) {
            const peca_t *c = &p->col[i];
            CONFERE(c->tipo < D_N, "%s: dado %d da bolsa de %d com tipo %d", quando, i, j, c->tipo);
            CONFERE(p->onde[i] <= COMIDO, "%s: dado %d da bolsa de %d em lugar %d", quando, i, j, p->onde[i]);
            CONFERE(c->fermento <= FERMENTO_MAX, "%s: Fermento %d no dado %d de %d", quando, c->fermento, i, j);
            CONFERE(p->pavio[i] <= DINAMITE_MAX, "%s: pavio %d no dado %d de %d", quando, p->pavio[i], i, j);
            for (int l = 0; l < AD_LADOS; l++)
                CONFERE((c->ad[l] & ~AD_BIT_VELHO) < AD_N, "%s: adesivo %d no lado %d do dado %d de %d", quando, c->ad[l], l + 1, i, j);
        }
        CONFERE(f->n >= 0 && f->n <= N_FILA && f->lancados >= 0 && f->lancados <= f->n,
                "%s: fila de %d com n %d e lancados %d", quando, j, f->n, f->lancados);
        if (f->n < 0 || f->n > N_FILA || f->lancados < 0 || f->lancados > f->n) return;
        CONFERE(f->n <= max_fila(j) + (coringa_joga(j) ? 1 : 0), "%s: fila de %d com %d dados, cabem %d", quando, j, f->n, max_fila(j));
        for (int k = 0; k < f->n; k++) {
            CONFERE(f->tipo[k] < D_N, "%s: dado %d de %d com tipo %d", quando, k, j, f->tipo[k]);
            CONFERE(f->idx[k] >= IDX_SAIU && f->idx[k] < p->n_col, "%s: dado %d de %d aponta para %d (bolsa de %d)",
                    quando, k, j, f->idx[k], p->n_col);
            if (f->tipo[k] >= D_N) return;
        }
        for (int k = 0; k < f->lancados; k++) {
            int t = f->tipo[k];
            CONFERE(f->est[k] <= V_ROUBADO, "%s: dado %d de %d com estado %d", quando, k, j, f->est[k]);
            CONFERE(f->lado[k] == 0 || (f->lado[k] >= ad_lado_min(t) && f->lado[k] <= ad_lado_max(t)),
                    "%s: dado %d de %d (%s) com o lado %d para cima", quando, k, j, TIPO[t].nome, f->lado[k]);
            CONFERE(f->valor[k] >= -110 && f->valor[k] <= 250 && f->cru[k] >= -110 && f->cru[k] <= 250,
                    "%s: dado %d de %d com valor %d (cru %d)", quando, k, j, f->valor[k], f->cru[k]);
            CONFERE(memchr(f->tag[k], 0, sizeof f->tag[k]) && memchr(f->tag_num[k], 0, sizeof f->tag_num[k])
                    && memchr(f->tag_mais[k], 0, sizeof f->tag_mais[k]), "%s: etiqueta sem fim no dado %d de %d", quando, k, j);
            CONFERE(f->guarda[k] <= N_FILA, "%s: guarda %d no dado %d de %d", quando, f->guarda[k], k, j);
            bool espera = f->escondido[k] || f->curinga[k] || f->gira[k] || (f->carta && k == 3);
            if (no_fim) {
                CONFERE(!f->escondido[k] && !f->curinga[k] && !f->gira[k] && !f->espera[k],
                        "%s: dado %d de %d (%s) ainda espera (escondido %d, curinga %d, gira %d, espera %d)", quando, k, j,
                        TIPO[t].nome, f->escondido[k], f->curinga[k], f->gira[k], f->espera[k]);
                CONFERE(f->est[k] != V_ESPERA, "%s: dado %d de %d (%s) terminou em V_ESPERA", quando, k, j, TIPO[t].nome);
            } else
                CONFERE((f->est[k] == V_ESPERA) == espera, "%s: dado %d de %d (%s) %s, mas %s fora da sequencia", quando, k, j,
                        TIPO[t].nome, nome_est(f->est[k]), espera ? "espera" : "nao espera");
        }
        if (!no_fim && f->lancados) CONFERE(f->pontos == soma(j, f->est), "%s: pontos de %d: %d, a soma da %d", quando, j, f->pontos, soma(j, f->est));
    }
    CONFERE(memchr(veredito, 0, sizeof veredito) != NULL, "%s: veredito sem fim", quando);
}

// A mao no comeco da rodada: dados diferentes da bolsa, todos marcados como
// na mao, e ninguem mais marcado.
static void confere_mao(const char *quando)
{
    for (int j = 0; j < 2; j++) {
        const jogador_t *p = &J[j];
        int na_mao = 0;
        for (int i = 0; i < p->n_col && i < MAX_COL; i++) na_mao += p->onde[i] == NA_MAO;
        CONFERE(na_mao == p->n_mao, "%s: %d dados de %d marcados na mao, e a mao tem %d", quando, na_mao, j, p->n_mao);
        for (int a = 0; a < p->n_mao && a < MAO; a++) {
            int c = p->mao[a];
            if (c < 0 || c >= p->n_col) continue;            // (confere_limites ja disse)
            CONFERE(p->onde[c] == NA_MAO, "%s: o dado %d da mao de %d esta em %d", quando, c, j, p->onde[c]);
            for (int b2 = 0; b2 < a; b2++) CONFERE(p->mao[b2] != c, "%s: o dado %d aparece duas vezes na mao de %d", quando, c, j);
        }
    }
}

static void confere_desfecho(const desfecho_t *d, const char *quando)
{
    for (int j = 0; j < 2; j++) {
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
            CONFERE(d->est[j][k] <= V_ROUBADO, "%s: estado %d no dado %d de %d", quando, d->est[j][k], k, j);
        CONFERE(d->total[j] >= 0 && d->total[j] == (d->bruto[j] < 0 ? 0 : d->bruto[j]),
                "%s: total %d com bruto %d (jogador %d)", quando, d->total[j], d->bruto[j], j);
        CONFERE(d->bruto[j] > -2000 && d->bruto[j] < 20000, "%s: bruto %d (jogador %d)", quando, d->bruto[j], j);
        CONFERE(d->agiota[j] >= 0 && d->gorjeta[j] >= 0, "%s: agiota %d, gorjeta %d (jogador %d)", quando, d->agiota[j], d->gorjeta[j], j);
    }
}

// A previa: nao mexe em nada, e duas seguidas dao o mesmo.
static void confere_previa(const char *quando, desfecho_t *sai)
{
    static desfecho_t d1, d2;
    char onde[160];
    retrata(&ret_a);
    calcula_desfecho(&d1, false);
    retrata(&ret_b);
    if (difere(&ret_a, &ret_b, onde, sizeof onde)) falha("%s: a previa mexeu em %s", quando, onde);
    calcula_desfecho(&d2, false);
    CONFERE(!memcmp(&d1, &d2, sizeof d1), "%s: duas previas seguidas deram resultados diferentes (%d x %d e %d x %d)", quando,
            d1.total[0], d1.total[1], d2.total[0], d2.total[1]);
    confere_desfecho(&d1, quando);
    if (sai) *sai = d1;
}

// As consultas que a mesa e a IA fazem enquanto a rodada anda: nenhuma pode
// mexer no estado das regras.
static void confere_consultas(const char *quando)
{
    char onde[160];
    retrata(&ret_a);
    for (int j = 0; j < 2; j++) {
        int r = risco(j);
        CONFERE(r >= 0 && r <= 100, "%s: risco %d (jogador %d)", quando, r, j);
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) (void)poder_armado(j, k);
        (void)nota_mesa(j, true);
    }
    uint8_t mira[2] = { 0, 0 };
    miras_agora(mira);
    (void)fim_incerto(); (void)fim_em_aberto(); (void)carrossel_conta();
    (void)ja_vence(0); (void)ja_vence(1);
    retrata(&ret_b);
    if (difere(&ret_a, &ret_b, onde, sizeof onde)) falha("%s: uma consulta (risco, IA, miras) mexeu em %s", quando, onde);
}

// Refazer a sequencia sem nada ter mudado da a mesma sequencia: o fim da
// rodada a refaz por causa de um dado (o Oculto que aparece, o Carrossel que
// para), e os outros nao podem mudar por isso.
static void confere_refaz(const char *quando)
{
    static fila_t antes[2];
    static retrato_t r0;
    retrata(&r0);
    memcpy(antes, F, sizeof F);
    for (int j = 0; j < 2; j++) {
        // Ficam de fora (veja o relatorio das regras em aberto): a Ferradura e
        // a Rede, que olham onde a queda aconteceu da primeira vez; e o dado
        // que o proprio poder (ou a Prensa, o Calibre) virou para um lado com
        // adesivo da sequencia.
        bool fora = !F[j].lancados || tem_am(j, A_FERRADURA) || tem_am(j, A_REDE);
        for (int k = 0; k < F[j].lancados && k < N_FILA; k++) {
            int a = ad_fila(j, k), ef = TIPO[tipo_de(j, k)].efeito;
            bool virou = F[j].calibre[k] || tem_am(j, A_PRENSA) || ef == EF_GEMEO || ef == EF_ECO || ef == EF_DIGITAL
                      || ef == EF_ESPELHO || ef == EF_FICHA_CORINGA || ef == EF_CARROSSEL;
            fora |= a == AD_CARBONO && !F[j].copiou[k];
            fora |= (a == AD_TRANSPARENTE || a == AD_CORRETIVO) && virou;
        }
        if (fora) continue;
        refaz_sequencia(j, F[j].lancados);
        const fila_t *a = &antes[j], *f = &F[j];
        for (int k = 0; k < f->lancados && k < N_FILA; k++)
            CONFERE(a->est[k] == f->est[k] && a->valor[k] == f->valor[k] && a->bonus[k] == f->bonus[k] && a->dobra[k] == f->dobra[k]
                    && a->lado[k] == f->lado[k],
                    "%s: refeita sem nada mudar, a sequencia de %d muda no dado %d (%s): %s %d (bonus %d) vira %s %d (bonus %d)", quando,
                    j, k, TIPO[f->tipo[k]].nome, nome_est(a->est[k]), a->valor[k], a->bonus[k], nome_est(f->est[k]), f->valor[k], f->bonus[k]);
    }
    memcpy(F, antes, sizeof F);
    memcpy(veredito, r0.veredito, sizeof veredito);
    ver_tipo = r0.ver_tipo; anulou_a = r0.anulou_a; anulou_b = r0.anulou_b; resistiu = r0.resistiu;
    anulou_masc = r0.anulou_masc; ver_comum = r0.ver_comum;
}

// ---------------------------------------------------------------------------
// A rodada
// ---------------------------------------------------------------------------
static uint32_t marca_calculo;     // o desfecho calculado: tem que ser o mesmo nas duas variantes
static int fichas_mesa;            // as fichas dos dois e o pote, antes de a rodada pagar
static bool previa_vale;           // a previa de antes do fim, se o fim nao dependia da sorte
static desfecho_t previa_fim;

// O dado que lanca_proximo jogou chega na fila: o mesmo que o passo de tempo
// faz quando ele termina de arrumar (passo.inc, F_ARRUMA).
static void chega_na_fila(void)
{
    voo_t *v = &voo[0];
    int j = v->dono, k = v->slot;
    F[j].lancados = k + 1;
    n_voo = 0;
    am_disparou = 0;
    pousa(j, k, rola);
    adesivos_pousou(j, k);
    if (F[j].tipo[k] == D_ACUMULADOR && F[j].est[k] == V_VALIDO && F[j].idx[k] >= 0
        && F[j].idx[k] < J[j].n_col && J[j].col[F[j].idx[k]].fixo < ACUM_MAX)
        J[j].col[F[j].idx[k]].fixo++;
    for (int a = 0; a < A_N; a++) if (am_disparou >> a & 1) am_acende(a);
    if ((am_disparou >> A_FERRADURA & 1) && ferr_k[j] < 0) ferr_k[j] = (int8_t)k;
    if (F[j].tipo[k] == D_REVOLVER && revolver_dispara(F[j].fixo[k])) dispara_revolver(j, k);
    cosmo_pousa(j, k);
    flutua_pouso(j, k);
    reage_lance(j, k);
    rival_lance(j, k);
    t_fase = 0;
    fase = F_VEREDITO;
    if (ja_vence(j)) { vence_j = j; t_vence = 0; }
    n_pouso = 0;                                 // (o veredito espera os adesivos do pouso acabarem)
    hora = 0;
}

static void lanca(int variante)
{
    int j = vez, k = F[j].lancados;
    char quando[48];
    snprintf(quando, sizeof quando, "lance %d de %d", k, j);
    lanca_proximo();
    if (fase == F_ROLANDO) chega_na_fila();
    confere_limites(quando, false);
    if (variante == VAR_CHEIA) {
        confere_previa(quando, NULL);
        confere_consultas(quando);
        confere_refaz(quando);
    }
}

// O fim do turno: o do jogo (termina_turno) ou, na variante seca, os mesmos
// passos com o desfecho calculado direto, sem a tela dos efeitos.
static void termina(int variante)
{
    if (variante == VAR_CHEIA) {
        // A previa de quem para agora (o botao "parar" e o ja_vence usam
        // esta): se o fim nao depende da sorte, e o que o desfecho vai dar.
        bool vai_virar = !carta_virando && vez != primeiro && carta_posta(primeiro);
        previa_vale = !fim_incerto() && !vai_virar;
        confere_previa("antes de terminar o turno", &previa_fim);
        fichas_mesa = J[0].fichas + J[1].fichas + pote;
        termina_turno();
        return;
    }
    if (carta_virando) { carta_virando = false; calcula_desfecho(&resolvido, true); fase = F_EFEITOS; return; }
    descarta_sobras(vez);
    if (vez != primeiro && carta_posta(primeiro)) {
        int c = primeiro;
        if (vira_carta(c)) { vez = c; carta_virando = true; lanca_proximo(); return; }
    }
    if (vez == primeiro && zerou_de_vez(vez)) {
        zerou = vez;
        devolve_mao(outro(vez));
        calcula_desfecho(&resolvido, true);
        fase = F_EFEITOS;
        return;
    }
    if (vez == primeiro) fase = F_APOSTA;
    else { calcula_desfecho(&resolvido, true); fase = F_EFEITOS; }
}

static bool acabou(void) { return fase == F_EFEITOS || fase == F_RESULTADO; }

static void joga_turno(int j, int variante)
{
    vez = j;
    gruda_chiclete(j);
    n_pouso = 0;
    monta_fila(j, variante);
    fase = F_JOGA;
    int para = vp(60) ? N_FILA : 1 + vn(F[j].n > 0 ? F[j].n : 1);
    while (F[j].lancados < F[j].n) {
        if (mesa_ia) {
            retrata(&ret_a);
            bool segue = ia_continua();
            confere_ia("decidindo se lanca mais um", variante);
            if (!segue) break;
        } else if (F[j].lancados > 0 && F[j].lancados >= para) break;
        lanca(variante);
        if (vence_j == j) break;
    }
    if (!F[j].lancados) { fase = F_APOSTA; return; }   // mao vazia: nada a lancar (nao acontece no jogo)
    termina(variante);
    if (fase == F_ROLANDO) {                           // a carta do Coringa virou e foi lancada
        chega_na_fila();
        confere_limites("a carta do Coringa", false);
        if (variante == VAR_CHEIA) confere_previa("a carta do Coringa", NULL);
        termina(variante);
    }
}

// O que o desfecho calculou, para comparar as variantes.
static uint32_t marca_do_calculo(void)
{
    uint32_t h = 2166136261u;
    h = mistura(h, &resolvido, sizeof resolvido);
    h = mistura(h, &n_ev, sizeof n_ev);
    for (int i = 0; i < n_ev; i++) h = mistura(h, &ev[i], sizeof ev[i]);
    for (int j = 0; j < 2; j++) {
        const fila_t *f = &F[j];
        h = mistura(h, &f->lancados, sizeof f->lancados);
        h = mistura(h, f->tipo, sizeof f->tipo);
        h = mistura(h, f->valor, sizeof f->valor);
        h = mistura(h, f->cru, sizeof f->cru);
        h = mistura(h, f->lado, sizeof f->lado);
    }
    h = mistura(h, &semente, sizeof semente);
    return h;
}

static void confere_eventos(void)
{
    CONFERE(n_ev >= 0 && n_ev < MAX_EV, "%d eventos: a lista encheu (cabem %d) e o resto se perdeu", n_ev, MAX_EV);
    for (int i = 0; i < n_ev; i++) {
        const evento_t *e = &ev[i];
        CONFERE(e->tipo <= EV_CARROSSEL, "evento %d com tipo %d", i, e->tipo);
        CONFERE(e->de_j <= 1 && e->alvo_j <= 1, "evento %d (tipo %d): jogador %d ou %d", i, e->tipo, e->de_j, e->alvo_j);
        if (e->de_j > 1 || e->alvo_j > 1) continue;
        CONFERE(e->de_k == 0xFF || e->de_k < F[e->de_j].lancados, "evento %d (tipo %d, '%s'): sai do dado %d de %d, que nao existe",
                i, e->tipo, e->txt, e->de_k, e->de_j);
        // Os que a mesa usa para achar o dado atingido.
        bool usa_alvo = e->tipo == EV_ANULA || e->tipo == EV_ROUBA || e->tipo == EV_FOGO || e->tipo == EV_CORTA
                     || (e->tipo == EV_TREMOR)
                     || (e->tipo == EV_ADESIVO && (e->ad == AD_MIRA || (e->ad == AD_RATOEIRA && e->alvo_j != e->de_j)));
        if (usa_alvo)
            CONFERE(e->alvo_k < F[e->alvo_j].lancados, "evento %d (tipo %d, '%s'): atinge o dado %d de %d, que nao existe",
                    i, e->tipo, e->txt, e->alvo_k, e->alvo_j);
        bool usa_de = e->tipo == EV_ANULA || e->tipo == EV_ESPELHO || e->tipo == EV_REVELA || e->tipo == EV_CORINGA
                   || e->tipo == EV_CARROSSEL || e->tipo == EV_RENASCE || e->tipo == EV_ONDA || e->tipo == EV_CAVALO
                   || e->tipo == EV_ROUBA;
        if (usa_de) CONFERE(e->de_k < F[e->de_j].lancados, "evento %d (tipo %d, '%s'): o dado de origem e %d", i, e->tipo, e->txt, e->de_k);
        CONFERE(e->txt[0] && memchr(e->txt, 0, sizeof e->txt), "evento %d (tipo %d) sem texto ou sem fim", i, e->tipo);
        CONFERE(e->ad < AD_N, "evento %d com adesivo %d", i, e->ad);
        CONFERE(!(e->mascara >> N_FILA) && !(e->bloqueado >> N_FILA), "evento %d (tipo %d): mascara %d, bloqueado %d", i, e->tipo,
                e->mascara, e->bloqueado);
    }
}

static long n_rodadas, n_eventos;   // o que a varredura jogou (so para o resumo)
static int max_eventos;
// Uma rodada da mesa, do primeiro lance ao pagamento. Devolve se houve rodada.
static bool joga_rodada(int variante)
{
    previa_vale = false;
    joga_turno(primeiro, variante);
    if (!acabou()) {
        // (a aposta fica de fora: o pote ja esta na mesa)
        joga_turno(outro(primeiro), variante);
    }
    if (!acabou()) return false;                 // sem nada lancado: nao ha rodada
    if (variante == VAR_CHEIA) { n_rodadas++; n_eventos += n_ev; if (n_ev > max_eventos) max_eventos = n_ev; }
    if (fala && variante == VAR_CHEIA && !falhou_rodada) {
        printf("rodada %d, depois do desfecho:\n", rodada);
        escreve_mesa();
        for (int i = 0; i < n_ev; i++) printf("  evento %d (tipo %d): %s\n", i, ev[i].tipo, ev[i].txt);
        printf("  desfecho: %d x %d\n", resolvido.total[0], resolvido.total[1]);
    }
    uint32_t marca = marca_do_calculo();
    marca_calculo = mistura(marca_calculo, &marca, sizeof marca);
    if (variante == VAR_SECA) { conclui_rodada(); return true; }

    // O desfecho de verdade ja rodou (desfecho): os eventos, um a um.
    confere_eventos();
    confere_desfecho(&resolvido, "desfecho");
    if (previa_vale) {
        CONFERE(previa_fim.total[0] == resolvido.total[0] && previa_fim.total[1] == resolvido.total[1],
                "sem nada ao acaso, a previa dava %d x %d e o desfecho deu %d x %d", previa_fim.total[0], previa_fim.total[1],
                resolvido.total[0], resolvido.total[1]);
        for (int j = 0; j < 2; j++)
            CONFERE(!memcmp(previa_fim.est[j], resolvido.est[j], N_FILA),
                    "sem nada ao acaso, a previa e o desfecho deixam os dados de %d em estados diferentes", j);
        const uint8_t *x = (const uint8_t *)&previa_fim, *y = (const uint8_t *)&resolvido;
        for (size_t o = 0; o < sizeof resolvido; o++)
            if (x[o] != y[o]) {
                falha("sem nada ao acaso, a previa e o desfecho diferem no campo %s (byte %d: %d e %d; totais %d x %d)",
                      campo_desfecho(o), (int)o, x[o], y[o], resolvido.total[0], resolvido.total[1]);
                break;
            }
    }
    int fichas_antes = fichas_mesa;
    if (fase == F_EFEITOS) {
        for (int i = 0; i < n_ev; i++) {
            if (i) { toca_evento(i); reage_evento(&ev[i]); }
            const evento_t *e = &ev[i];
            if (e->tipo == EV_CARROSSEL && e->de_j <= 1 && e->de_k < N_FILA) {
                // O numero que a mesa mostra nele girando e o numero em que ele para.
                int mostra = carrossel_vivo(true), para = e->valor, validos = 0;
                if (sscanf(e->txt, "Carrossel para no %*d: %d válidos e o Calibre", &validos) == 1) para = validos;
                CONFERE(mostra == para, "o Carrossel %d de %d girava mostrando %d e parou no %d ('%s')", e->de_k, e->de_j,
                        mostra, para, e->txt);
            }
            aplica_evento(i);
            golpe_evento(e);
        }
        for (int j = 0; j < 2; j++) {
            CONFERE(total_visivel(j) == resolvido.total[j], "os eventos mostram %d, o motor paga %d (jogador %d)",
                    total_visivel(j), resolvido.total[j], j);
            for (int k = 0; k < F[j].lancados && k < N_FILA; k++)
                CONFERE(vis_est[j][k] == resolvido.est[j][k], "depois dos eventos o dado %d de %d aparece %s, e o motor diz %s",
                        k, j, nome_est(vis_est[j][k]), nome_est(resolvido.est[j][k]));
        }
        conclui_rodada();
    }
    CONFERE(fase == F_RESULTADO, "a rodada nao chegou ao resultado (fase %d)", fase);
    for (int j = 0; j < 2; j++)
        CONFERE(F[j].total == resolvido.total[j], "pagou %d, o motor deu %d (jogador %d)", F[j].total, resolvido.total[j], j);
    int ganho = (venc_mao < 2 && tem_am(venc_mao, A_POTE)) ? 2 : 0;       // o Pote Gordo poe 2 fichas na mesa
    CONFERE(J[0].fichas + J[1].fichas + pote == fichas_antes + ganho, "as fichas nao fecham: %d antes, %d depois (+%d do Pote Gordo)",
            fichas_antes, J[0].fichas + J[1].fichas + pote, ganho);
    confere_limites("fim da rodada", true);
    return true;
}

// Joga a mesa da semente s: uma rodada, ou algumas seguidas com as mesmas
// bolsas (o que cada dado guarda, os que saem, os que voltam para a mao).
// Devolve a marca do estado final.
static uint32_t joga_mesa(uint32_t s, int variante)
{
    vs = 0x1234567ull * s + 99;
    semente_rodada = s;
    variante_agora = variante;
    marca_calculo = 2166136261u;
    monta_mesa();
    confere_limites("mesa montada", false);
    confere_mao("mesa montada");
    int quantas = vp(50) ? 1 : 2 + vn(3);
    for (int r = 0; r < quantas && !falhou_rodada; r++) {
        if (!joga_rodada(variante)) break;
        if (r + 1 == quantas) break;
        // Como o ENTER no resultado (segue_do_resultado), sem o premio.
        if (venc_mao == 0 || venc_mao == 1) primeiro = venc_mao;
        else primeiro ^= 1;
        nova_rodada();
        if (fase != F_ORDEM) break;              // a partida acabou
        confere_limites("rodada nova", false);
        confere_mao("rodada nova");
    }
    uint32_t h = hash_partida();
    h = mistura(h, F, sizeof F);
    for (int j = 0; j < 2; j++) {
        h = mistura(h, J[j].col, sizeof J[j].col[0] * (size_t)J[j].n_col);
        h = mistura(h, &J[j].fichas, sizeof J[j].fichas);
    }
    return h;
}

static bool varre(uint32_t s)
{
    falhou_rodada = false;
    uint32_t h1 = joga_mesa(s, VAR_CHEIA), m1 = marca_calculo;
    if (falhou_rodada) return false;
    joga_mesa(s, VAR_SECA);
    CONFERE(marca_calculo == m1, "a mesma rodada, com e sem as previas no meio, deu desfechos diferentes (%d x %d)",
            resolvido.total[0], resolvido.total[1]);
    if (!falhou_rodada && s % 4 == 0) {
        uint32_t h2 = joga_mesa(s, VAR_CHEIA);
        CONFERE(h1 == h2 && marca_calculo == m1, "a mesma semente, jogada duas vezes, deu resultados diferentes");
    }
    return !falhou_rodada;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IOLBF, 0);            // as falhas saem na hora, mesmo se a varredura for interrompida
    natal_forcado = 0;
    ia_esforco = 0;
    if (argc > 2 && !strcmp(argv[1], "--uma")) {
        fala = true;
        bool ok = varre((uint32_t)strtoul(argv[2], NULL, 10));
        printf("%s\n", ok ? "mesa sem falhas" : "mesa com falhas");
        return ok ? 0 : 1;
    }
    long mesas = argc > 1 ? atol(argv[1]) : 3000;
    uint32_t base = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 10) : 20240229u;
    long ruins = 0;
    for (long i = 0; i < mesas; i++) {
        uint32_t s = base + (uint32_t)i * 2654435761u;
        if (!varre(s)) ruins++;
        if (rodadas_ruins > 12) { printf("... muitas falhas: parei na mesa %ld\n", i + 1); break; }
    }
    if (falhas) { printf("%d falhas em %ld mesas (de %ld)\n", falhas, ruins, mesas); return 1; }
    printf("%ld mesas varridas (semente %u), %ld rodadas, %ld eventos (ate %d numa rodada): nenhuma falha\n", mesas, base,
           n_rodadas, n_eventos, max_eventos);
    return 0;
}
