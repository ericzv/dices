// ===========================================================================
// ARMADO: a mesa ao vivo, antes do fim - se o poder de um dado esta armado
// (poder_armado, o brilho dele) e quais dados os poderes armados miram
// (miras_agora). E os numeros do Agnus.
// Le: F como esta agora (F[].est, nao d), J[].perdidas, as fichas do comeco
// da rodada, o pote e os amuletos. Nao le d nem ev[].
// Grava: nada; so a mascara mira[] que recebe.
// Quem chama: a mesa (mesa.inc), a cada quadro.
// ===========================================================================

#define AGNUS_NUMERO 7                 // o Agnus: o numero que ressuscita
#define AGNUS_RAIO   8                 // e o que anula, com a luz, o maior do oponente (valendo)
// O evento dele na mesa: a gloria acende sobre o Agnus e, a partir de
// AGNUS_LUZ_0 segundos, uma luz sai dela para cada anulado, um a cada
// AGNUS_LUZ_PASSO; ela leva AGNUS_LEVANTA para chegar, e o dado se levanta.
#define AGNUS_LUZ_0     0.45f
#define AGNUS_LUZ_PASSO 0.32f
#define AGNUS_LEVANTA   0.22f

// Os poderes que atacam o oponente pelo numero que o dado tirou (valendo): o
// que o golpe faz e quando ele arma. E a fonte unica dos limiares para o
// brilho (poder_armado), o contra-ataque (ataca_oponente), a Couraca e o
// proprio golpe (poder_de). Os que dependem da mesa (o Gatuno, pelos 1 dos
// outros; o Berserker, pelo vizinho) ficam de fora. miras_agora, que refaz a
// escolha dos alvos, segue com as suas condicoes.
enum { GOLPE_NENHUM, GOLPE_ANULA, GOLPE_CORTA, GOLPE_MENOS1, GOLPE_TIRA, GOLPE_ROUBA };
enum { ARMA_SEMPRE, ARMA_IGUAL, ARMA_DE };        // sempre; com o limiar exato; do limiar para cima
typedef struct { uint8_t golpe, arma, limiar; } poder_ataque_t;
static const poder_ataque_t PODER_ATAQUE[EF_N] = {
    [EF_CARRASCO]  = { GOLPE_ANULA,  ARMA_IGUAL,  CARRASCO_ANULA },   // o maior do oponente
    [EF_LASER]     = { GOLPE_ANULA,  ARMA_DE,     LASER_DE },         // o ultimo
    [EF_TSUNAMI]   = { GOLPE_ANULA,  ARMA_DE,     TSUNAMI_DE },       // o primeiro
    [EF_BONECO]    = { GOLPE_ANULA,  ARMA_DE,     BONECO_DE },        // o 4o
    [EF_INVEJOSO]  = { GOLPE_ANULA,  ARMA_DE,     INVEJOSO_DE },      // o maior (com 1 ou 2, os do dono)
    [EF_AGNUS]     = { GOLPE_ANULA,  ARMA_IGUAL,  AGNUS_RAIO },       // a luz no maior
    [EF_SABOTADOR] = { GOLPE_CORTA,  ARMA_DE,     SABOTADOR_DE },     // metade do maior
    [EF_VAMPIRO]   = { GOLPE_MENOS1, ARMA_SEMPRE, 0 },                // -1 em cada um
    [EF_MARTELO]   = { GOLPE_TIRA,   ARMA_DE,     MARTELO_DE },       // o Espinhoso: -4 no placar
    [EF_MALDICAO]  = { GOLPE_TIRA,   ARMA_SEMPRE, 0 },                // o Maldito: -o que tirou
    [EF_PIRATA]    = { GOLPE_TIRA,   ARMA_DE,     PIRATA_DE },        // -4 no placar
    [EF_CARIDOSO]  = { GOLPE_ROUBA,  ARMA_DE,     CARIDOSO_DE },      // o menor
};
static bool ataque_armado(int ef, int v)
{
    const poder_ataque_t *p = &PODER_ATAQUE[ef];
    if (p->golpe == GOLPE_NENHUM) return false;
    return p->arma == ARMA_SEMPRE || (p->arma == ARMA_IGUAL ? v == p->limiar : v >= p->limiar);
}

// A Autofagia (posicao a da fila de j) pode comer o dado b? So com os dois
// vindos da bolsa e com pelo menos 8 dados em jogo.
static bool autofagia_come_pode(int j, int a, int b)
{
    const jogador_t *p = &J[j];
    int ia = F[j].idx[a], ib = F[j].idx[b];
    if (ia < 0 || ia >= p->n_col || ib < 0 || ib >= p->n_col || ia == ib) return false;
    if (p->col[ia].tipo != D_AUTOFAGIA) return false;
    int vivos = 0;
    for (int i = 0; i < p->n_col; i++) vivos += p->onde[i] != COMIDO;
    return vivos >= AUTO_BOLSA;
}

// O poder do dado k de j esta armado, pelo que ele tirou e pela mesa de
// agora? E o brilho dele enquanto a sequencia e lancada. 0: nao; 1: sim;
// 2: sim, mas contra quem o jogou (Carrasco ou Tudo ou Nada com 1, Berserker, Autofagia).
static int poder_armado(int j, int k)
{
    const fila_t *f = &F[j];
    if (k >= f->lancados) return 0;
    int ef = TIPO[tipo_de(j, k)].efeito, v = f->valor[k];
    if (ef == EF_CARRASCO && f->cru[k] == 1) return 2;          // valendo ou nao
    if (ef == EF_TUDO_NADA && f->cru[k] == 1) return 2;
    if (ef == EF_FENIX) return f->est[k] == V_ANULADO;         // anulada: renasce no fim
    if (ef == EF_AGNUS)                                         // com 7, valendo ou nao: ressuscita os anulados no fim;
        return v == AGNUS_NUMERO || (ataque_armado(ef, v) && f->est[k] == V_VALIDO);   // com 8 valendo: a luz no oponente
    if (ef == EF_FICHA_VELHA) return f->est[k] == V_ANULADO;   // anulada: +4 no fim
    if (ef == EF_ESTILINGUE) return v == 1 && k + 1 < f->n && f->lancados == k + 1;   // com 1, valendo ou nao: o proximo sai no maximo
    if (ef == EF_REVOLVER) return f->est[k] == V_ANULADO && revolver_dispara(fixo_de(j, k));   // o tiro anulado: +12 no fim
    if (ef == EF_MEIA && f->est[k] == V_ANULADO && f->tipo[k] == D_MEIA) return 1;   // anulada: guarda +MEIA_GUARDA (4) (o Camaleao, nao)
    if (ef == EF_FICHA_CORINGA) return f->curinga[k];          // com o curinga para cima: vira no fim
    if (ef == EF_CARROSSEL) return f->gira[k];                 // girando: para no fim
    if (ef == EF_MARTIR) {                                      // caido, com algum seu valendo
        if (f->est[k] != V_ANULADO) return 0;
        for (int i = 0; i < f->lancados && i < N_FILA; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    }
    if (f->est[k] != V_VALIDO) return 0;
    // Os ataques pelo numero (PODER_ATAQUE). Divergencia mantida: o brilho
    // nao olha a Couraca do oponente nem se ha alvo (a mira, sim).
    if (PODER_ATAQUE[ef].golpe != GOLPE_NENHUM) {
        if (ataque_armado(ef, v)) return 1;
        return ef == EF_INVEJOSO && v <= 2 ? 2 : 0;          // o Invejoso com 1 ou 2: contra o dono
    }
    int outros = 0, baixos = 0, iguais = 0, uns = 0;           // os outros dados seus
    for (int i = 0; i < f->lancados && i < N_FILA; i++) {
        if (i == k || f->est[i] != V_VALIDO) continue;
        uns += f->cru[i] == 1;                                 // os 1 dos outros valendo (o Gatuno)
        outros++;
        baixos += f->valor[i] <= 2;
        iguais += f->valor[i] == v;
    }
    switch (ef) {
    case EF_DOBRO:        return v <= 3;
    case EF_PAR:          return f->bonus[k] >= 6;
    case EF_ESPELHO:      return f->cru[k] <= 2;
    case EF_FICHAS:       return fichas_ini[j] > fichas_ini[outro(j)];
    case EF_COBRADOR:     return fichas_ini[outro(j)] > fichas_ini[j];
    case EF_TUDO_NADA:    return v == 2;
    case EF_GATUNO:       return uns > 0;
    case EF_FARTURA:      return baixos > 0 || v <= 2;
    case EF_SEMENTE:      return soma(j, f->est) >= SEMENTE_BROTA;
    case EF_FOGO:         return outros > 0 ? (f->cru[k] == 2 ? 1 : 2) : 0;   // com 1, so queima
    case EF_TREVO:        return v == 4;
    case EF_JACKPOT:      return iguais > 0;                    // outro seu valendo com o numero dele
    case EF_COPIADOR:     return iguais > 0;
    case EF_TERREMOTO:    return v >= TERREMOTO_DE;
    case EF_FERREIRO:     return v >= FERREIRO_ARMA;
    case EF_TRONO:        return k == FILA_NORMAL - 1 && outros == f->lancados - 1;
    case EF_VIUVA:        return outros == 0;
    case EF_DOMINO:       return k + 1 < f->lancados && f->est[k + 1] == V_VALIDO && f->valor[k + 1] >= v && f->valor[k + 1] <= v + 2;
    case EF_ANCORA:       return k == f->lancados - 1 || f->ancora[k + 1];
    case EF_CALIBRE:      return k == f->lancados - 1 || f->calibre[k + 1];
    case EF_PUXA:         return k == 0 && f->lancados >= FILA_NORMAL && outros == f->lancados - 1;
    case EF_COVEIRO:
        for (int q = 0; q < 2; q++)
            for (int i = 0; i < F[q].lancados && i < N_FILA; i++) if (F[q].est[i] == V_ANULADO) return 1;
        return 0;
    case EF_HEXA:         return 1;
    case EF_OURICO:       return 1;
    case EF_AZARAO:       return J[j].perdeu_ultima;
    case EF_AMPULHETA:    return ampulheta() > 0;
    case EF_AGIOTA:       return 1;
    case EF_MEDUSA:       return 1;
    case EF_MOSAICO:      return 1;
    case EF_LEQUE:                                           // um outro seu valendo no lado com adesivo
        for (int i = 0; i < f->lancados && i < N_FILA; i++) if (i != k && f->est[i] == V_VALIDO && ad_fila(j, i)) return 1;
        return 0;
    case EF_BALAO:        return f->fixo[k] > 0 && v >= BALAO_ESTOURA && f->tipo[k] == D_BALAO;
    case EF_PAPAGAIO:     return 1;
    case EF_RETRATO:      return k > 0 && ad_fila(j, k) && f->est[k - 1] == V_VALIDO;
    case EF_RANCOR:       return J[j].perdidas > 0;
    case EF_ESPERANCA:    return fichas_ini[j] <= ESPERANCA_FICHAS;
    case EF_POTE:         return pote >= 25;
    case EF_GARIMPEIRO:                                      // um outro dado seu valendo
        for (int i = 0; i < f->lancados && i < N_FILA; i++)
            if (i != k && f->est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito != EF_GARIMPEIRO) return 1;
        return 0;
    case EF_CHECKPOINT:                                      // um dado seu valendo depois dele
        for (int i = k + 1; i < f->lancados && i < N_FILA; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    case EF_SOLITARIO:
        return !(k > 0 && f->est[k - 1] == V_VALIDO) && !(k + 1 < f->lancados && f->est[k + 1] == V_VALIDO);
    case EF_DESAFIANTE:                                      // o maior da mesa, sozinho
        for (int q = 0; q < 2; q++)
            for (int i = 0; i < F[q].lancados && i < N_FILA; i++)
                if (!(q == j && i == k) && F[q].est[i] == V_VALIDO && F[q].valor[i] >= v) return 0;
        return 1;
    case EF_BERSERKER:                                       // o vizinho do oponente; sem ele, o seu (contra o dono)
        if (!tem_am(outro(j), A_COURACA) && alvo_vizinho(F[outro(j)].est, outro(j), k) >= 0) return 1;
        return alvo_vizinho(f->est, j, k) >= 0 ? 2 : 0;
    case EF_AUTOFAGIA: {                                     // vai comer o seu menor (se puder: bolsa de 8)
        int b = menor_valido(f->est, j, (uint8_t)(1 << k));
        return b >= 0 && autofagia_come_pode(j, k, b) ? 2 : 0;
    }
    case EF_COLECIONADOR: return v >= COLECIONADOR_DE;
    case EF_BURACO:                                          // vai engolir um vizinho seu
        for (int s = -1; s <= 1 && f->faixa[k]; s += 2)
            if (k + s >= 0 && k + s < f->lancados && f->est[k + s] == V_VALIDO && !firme_sem_capa(j, k + s)) return 2;
        return 0;
    case EF_PATINHOS:     return patinhos_ativo(fixo_de(j, k));
    case EF_PISCA:        return pisca_aceso(fixo_de(j, k));
    case EF_MEIA:         return f->fixo[k] > 0;
    case EF_ESTRELA:                                         // por ultimo, com um seu valendo abaixo
        if (k != f->lancados - 1) return 0;
        for (int i = 0; i < k; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    case EF_RENA:                                            // um vizinho valendo
        return (k > 0 && f->est[k - 1] == V_VALIDO) || (k + 1 < f->lancados && f->est[k + 1] == V_VALIDO);
    case EF_ONDA:         return onda_da_bolsa(j, k) && onda_depois(f->fixo[k], v) >= ONDA_VIRA;
    case EF_TRENO:                                           // um dado seu valendo antes dele
        for (int i = 0; i < k; i++) if (f->est[i] == V_VALIDO) return 1;
        return 0;
    case EF_JINGLE:                                          // um outro sino valendo
        for (int i = 0; i < f->lancados && i < N_FILA; i++)
            if (i != k && f->est[i] == V_VALIDO && TIPO[tipo_de(j, i)].efeito == EF_JINGLE) return 1;
        return 0;
    case EF_SACRIFICIO: {                                    // um dado seu que pontua e que outro Sacrificio nao pegou
        int8_t alvo[N_FILA];
        sacrificio_alvos(j, f->est, alvo);
        return alvo[k] >= 0;
    }
    case EF_SEGURANCA:    return outros == f->lancados - 1;   // o +2: nenhum seu caido (o desvio vale sempre)
    case EF_SORTE: case EF_MISERICORDIA: case EF_PRISMA: case EF_CAPITAO:
    case EF_MEDIA: case EF_ASSOMBRA:
        return 1;                                            // basta terminar valendo
    default:              return 0;
    }
}

// A mira: os dados que os poderes armados agora vao pegar no fim da rodada
// (anular, roubar ou cortar), pelas mesmas regras de alvo do fim, com a mesa
// como esta. Uma mascara por jogador (de quem e o dado mirado).
static void miras_agora(uint8_t mira[2])
{
    mira[0] = mira[1] = 0;
    for (int j = 0; j < 2; j++) {
        int r = outro(j);
        const fila_t *f = &F[j];
        bool couraca = tem_am(r, A_COURACA);
        for (int k = 0; k < f->lancados && k < N_FILA; k++) {
            if (!poder_armado(j, k)) continue;
            int ef = TIPO[tipo_de(j, k)].efeito, v = f->valor[k], a = -1;
            bool desvia = true;                  // o Seguranca do oponente toma o ataque no lugar
            uint8_t est[N_FILA];
            memcpy(est, f->est, N_FILA);
            switch (ef) {
            case EF_CARRASCO:
                if (f->cru[k] == 1) mira_menores(est, j, 1, -1, &mira[j]);   // o seu menor (ele mesmo, se for)
                else if (v == CARRASCO_ANULA && !couraca) a = alvo_de(F[r].est, r, true, -1);
                break;
            case EF_INVEJOSO:
                if (invejoso_age(v)) { if (!couraca) a = alvo_de(F[r].est, r, true, -1); }
                else mira_menores(est, j, 2, -1, &mira[j]);                 // os seus dois menores
                break;
            case EF_LASER:   if (!couraca) a = alvo_ultimo(F[r].est, r); break;
            case EF_TSUNAMI: if (!couraca) a = alvo_primeiro(F[r].est, r); break;
            case EF_TERREMOTO:                   // sacode o ultimo valido do oponente (sem desvio)
                if (!couraca) a = alvo_tremor(F[r].est, r);
                desvia = false;
                break;
            case EF_BONECO:
                if (!couraca && F[r].lancados > 3 && F[r].est[3] == V_VALIDO && !firme_sem_capa(r, 3)) a = 3;
                break;
            case EF_SABOTADOR:                   // corta o maior do oponente que pontua (sem desvio)
                a = alvo_corte(F[r].est, NULL, r);
                desvia = false;
                break;
            case EF_CARIDOSO: a = alvo_de(F[r].est, r, false, -1); break;
            case EF_GATUNO: {                    // um roubo por 1 dos seus outros dados valendo
                int n = 0;
                uint8_t er[N_FILA];
                memcpy(er, F[r].est, N_FILA);
                for (int i = 0; i < f->lancados && i < N_FILA; i++) n += i != k && f->cru[i] == 1 && f->est[i] == V_VALIDO;
                for (int q = 0; q < n; q++) {
                    int b = desvia_seguranca(er, r, alvo_de(er, r, false, -1));
                    if (b < 0) break;
                    er[b] = V_ROUBADO;
                    mira[r] |= (uint8_t)(1 << b);
                }
                break;
            }
            case EF_BERSERKER: {                 // o vizinho do oponente; sem ele (ou com a Couraca), o seu
                a = couraca ? -1 : alvo_vizinho(F[r].est, r, k);
                int b = a < 0 ? alvo_vizinho(est, j, k) : -1;
                if (b >= 0) mira[j] |= (uint8_t)(1 << b);
                break;
            }
            case EF_AGNUS:                       // com 8 valendo: a luz no maior do oponente
                if (v == AGNUS_RAIO && f->est[k] == V_VALIDO && !couraca) a = alvo_de(F[r].est, r, true, -1);
                break;
            case EF_AUTOFAGIA: mira_menores(est, j, 1, k, &mira[j]); break;   // armada: ela pode comer
            case EF_BURACO:
                for (int s = -1; s <= 1 && f->faixa[k]; s += 2)
                    if (k + s >= 0 && k + s < f->lancados && f->est[k + s] == V_VALIDO && !firme_sem_capa(j, k + s))
                        mira[j] |= (uint8_t)(1 << (k + s));
                break;
            default: break;
            }
            if (a >= 0 && desvia) a = desvia_seguranca(F[r].est, r, a);
            if (a >= 0) mira[r] |= (uint8_t)(1 << a);
        }
    }
}

