// ===========================================================================
// Passo de tempo
// ===========================================================================
// Telas de menu, fora de qualquer partida: nelas a velocidade nao muda nada.
static bool menu_sem_partida(void)
{
    return fase == F_ABERTURA || fase == F_CATALOGO || fase == F_TUTORIAL || fase == F_ONLINE
        || fase == F_PERFIS || fase == F_PCRIA || fase == F_RDIFIC || fase == F_HIST || fase == F_ESTILO
        || fase == F_RBOLSAS;
}

static void menu_passo(float dt);        // as fichas do menu (menus.inc)
static void foto_clique(float antes, float agora);
static void poeira_passo(float dt);       // o pouso: a poeira e o achatado (mesa.inc)

void dado_passo(float dt)
{
    float dt_real = dt;                  // baloes e avisos: no tempo de quem le
    t_relogio = fmodf(t_relogio + dt_real, 1000.0f);
    if (t_trans >= 0) t_trans += dt_real;            // a transicao entre telas (quadro.inc)
    t_detalhe += dt_real;
    if (t_aperto > 0) t_aperto -= dt_real;
    if (t_devolve >= 0 && (t_devolve += dt) > DEVOLVE_T + 0.07f * N_FILA) t_devolve = -1;
    float vel = modo != M_ONLINE && !menu_sem_partida() ? VEL_FATOR[velocidade] : 1;
    dt *= vel;
    if (bolsa_pop >= 0) {                // o pop-up da bolsa: fora do online, a partida espera
        t_bolsa_pop += dt_real;
        if (modo != M_ONLINE) dt = 0;
    }
    if (fase == F_EFEITOS) dt *= ritmo_efeitos(vel);   // a cadeia do fim da rodada acelera
    if (t_aviso_vel > 0) t_aviso_vel -= dt_real;
    if (t_aviso_universo > 0) t_aviso_universo -= dt_real;
    if (t_aviso_tremor > 0) t_aviso_tremor -= dt_real;
    if (t_aviso_letra > 0) t_aviso_letra -= dt_real;
    if (t_px_pulso >= 0 && (t_px_pulso += dt_real) > PX_PULSO_T) t_px_pulso = -1;
    if (t_aviso_entrada > 0) t_aviso_entrada -= dt_real;
    if (t_sua_vez > 0) t_sua_vez -= dt_real;
    for (int j = 0; j < 2; j++) if (t_humor_pisca[j] > 0) t_humor_pisca[j] -= dt_real;
    if (t_fantasma > 0) t_fantasma -= dt_real;
    if (t_elev > 0) {
        float antes = ELEV_T - t_elev;
        t_elev -= dt_real;
        if (antes < ELEV_CHEGA && ELEV_T - t_elev >= ELEV_CHEGA) som_toca(SOM_VALIDO);   // o "dim" do elevador
    }
    if (t_virada > 0) { t_virada -= dt_real; return; }   // o anuncio da virada para a mesa
    if (t_misterio > 0) {                // o Dado Misterioso: a pedra se parte, depois a mesa para
        float antes = t_misterio;
        t_misterio += dt_real;
        if (antes <= MIST_ESPERA && t_misterio > MIST_ESPERA) som_toca(SOM_PREMIO);
        if (t_misterio > MIST_MAX) t_misterio = 0;
        else if (t_misterio > MIST_ESPERA) return;
    }
    if (golpe_passo(dt_real)) return;    // o golpe: a mesa parada por um instante
    if (ev_espera) {                     // e so depois dele entra o efeito seguinte
        ev_espera = false;
        if (fase == F_EFEITOS) proximo_evento();
    }
    float t_antes = t_fase;
    t_fase += dt;
    if (fase == F_RFIM) foto_clique(t_antes, t_fase);   // a foto da run campea (desafio/desenho.inc)
    mola_passo(dt_real);
    hora_passo(dt, dt_real);
    placar_passo(dt, dt_real);
    des_contadores_passo(dt, dt_real);
    bolsas_passo(dt_real);
    if (t_anuncio > 0) t_anuncio -= dt;
    flutua_passo(dt);
    poeira_passo(dt);
    tiro_passo(dt);
    cosmo_passo(dt);
    passo_slot(dt);
    for (int a = 0; a < A_N; a++) if (am_brilho[a] > 0) am_brilho[a] -= dt;
    if (t_fala > 0) t_fala -= dt_real;
    if (fala_dita < 60) fala_dita += dt_real;
    if (t_reacao > 0) t_reacao -= dt_real;
    if (t_curta > 0) t_curta -= dt_real;
    ia_passo(dt);
    rede_passo();
    if (t_sair >= 0 && (t_sair += dt_real) > 2.5f) t_sair = -1;
    if (vence_j >= 0) t_vence += dt_real;
    switch (fase) {
    case F_ABERTURA:
        if (!abertura_pronta) {
            if (fisica(dt) && t_fase > 0.4f) {
                abertura_pronta = true;
                t_fase = 0;
                som_toca(SOM_ABERTURA);
            }
        } else {
            // O crupie afasta os dados para os lados do titulo e abre a mesa as fichas.
            static const float ALVO[2][2] = { { 86, 88 }, { 554, 88 } };
            menu_passo(dt_real);
            float a = dt * 5 > 1 ? 1 : dt * 5;
            if (sacode > 0) sacode -= dt / 0.85f;
            if (sacode < 0) sacode = 0;
            for (int i = 0; i < n_voo; i++) {
                voo[i].x += (ALVO[i][0] - voo[i].x) * a;
                voo[i].y += (ALVO[i][1] - voo[i].y) * a;
                voo[i].giro *= 1 - a;
            }
        }
        break;
    case F_CATALOGO:
        break;
    case F_PREMIO:
        if (premio_colando()) cola_passo(dt_real);   // o adesivo ganho descendo no lado
        if (t_tira > 0) {
            t_tira += dt;
            if (t_tira > 0.7f) {
                jogador_t *p = &J[vez];
                for (int m = tira_i; m + 1 < p->n_col; m++) {
                    p->col[m] = p->col[m + 1];
                    p->onde[m] = p->onde[m + 1];
                }
                p->n_col--;
                p->n_mao = 0;
                t_tira = 0;
                removendo = false;
                nova_rodada();
            }
        }
        break;
    case F_ROLANDO: {
        // O ultimo dado da sequencia rola devagar: e nele que a rodada se
        // decide. Na vez da IA, os outros andam mais rapido. Na hora da
        // verdade, sai quase parado no ar e rola mais devagar ainda.
        bool ia = (cpu >> voo[0].dono & 1) != 0;
        bool ultimo = voo[0].slot == F[voo[0].dono].n - 1;
        float lento = ultimo ? 0.42f : (ia ? 1.0f : 0.62f);
        if (hora) lento = 0.40f * fminf(1, 0.5f + t_fase / 0.4f);
        bool parou = fisica(dt * lento);
        ad_pousou();                         // pousou no Chumbo (o baque) ou no Remendo
        if (parou && t_fase > (ia && !ultimo ? 0.25f : 0.5f)) {
            if (ad_etapa_passo(dt)) break;       // o adesivo age no pouso (Avesso, Reflexivo)
            if (hora) som_rufo(false);           // o som segura: silencio ate o veredito
            for (int i = 0; i < n_voo; i++) { voo[i].ox = voo[i].x; voo[i].oy = voo[i].y; }
            t_fase = 0;
            fase = F_ARRUMA;
        }
        break;
    }
    case F_ARRUMA: {
        bool ia_arr = (cpu >> voo[0].dono & 1) && voo[0].slot != F[voo[0].dono].n - 1;
        float jt = junta_t();                    // o Sorte/Reves se junta antes de ir
        if (jt && t_fase >= jt && t_fase - dt < jt) som_toca(SOM_BATE_DADO);
        float t = (t_fase - jt) / (ia_arr ? 0.28f : 0.45f);
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        float e = t * t * (3 - 2 * t);
        voo_t *v = &voo[0];
        v->x = v->ox + (SLOT_X(v->slot) - v->ox) * e;
        v->y = v->oy + (zona(v->dono) - v->oy) * e;
        v->giro *= (1 - e);
        if (t >= 1) {
            int j = v->dono, k = v->slot;
            F[j].lancados = k + 1;
            n_voo = 0;
            som_rufo(false);
            am_disparou = 0;
            pousa(j, k, rola);
            // O Acumulador que pousou valendo guarda +1 para as proximas vezes.
            if (F[j].tipo[k] == D_ACUMULADOR && F[j].est[k] == V_VALIDO && F[j].idx[k] >= 0
                && F[j].idx[k] < J[j].n_col && J[j].col[F[j].idx[k]].fixo < ACUM_MAX)
                J[j].col[F[j].idx[k]].fixo++;
            for (int a = 0; a < A_N; a++) if (am_disparou >> a & 1) am_acende(a);
            if ((am_disparou >> A_FERRADURA & 1) && ferr_k[j] < 0) ferr_k[j] = (int8_t)k;
            // A Ventania tem o som dela quando rola o primeiro dado.
            if (resistiu >= 0 && ad_fila(j, resistiu) == AD_TRANSPARENTE)   // o Transparente segurou
                som_toca(som_adesivo(AD_TRANSPARENTE));
            // O Revolver com a sexta bala dispara ao pousar, valendo ou nao.
            if (F[j].tipo[k] == D_REVOLVER && revolver_dispara(F[j].fixo[k])) dispara_revolver(j, k);
            cosmo_pousa(j, k);                    // o ciclo cosmico que tirou a faixa de cima: evolui (o Universo: Big Bang)
            if (k > 0 && F[j].tipo[k] == D_VENTANIA) som_toca(SOM_VENTO);
            else if (F[j].tipo[k] == D_VIDRO && F[j].cru[k] == 1) {
                som_toca(SOM_VIDRO);
                solta_cacos(SLOT_X(k), zona(j));
            } else if (ver_tipo == 0) {             // valendo: uma nota acima a cada posicao
                static const int SOM_POS[N_FILA] = { SOM_VALIDO, SOM_VALIDO2, SOM_VALIDO3, SOM_VALIDO4, SOM_VALIDO4 };
                som_toca(SOM_POS[k < N_FILA ? k : N_FILA - 1]);
                px_pulsa(SLOT_X(k), zona(j));          // o modo Pixel: a luz pulsa embaixo dele
            } else som_toca(SOM_ANULA);
            if (j == 0 && ver_tipo == 1) fala_evento(FA_CAI);
            flutua_pouso(j, k);
            reage_lance(j, k);
            rival_lance(j, k);
            t_fase = 0;
            fase = F_VEREDITO;
            if (ja_vence(j)) {                     // passou do alvo: o placar acende e a vez acaba
                vence_j = j;
                t_vence = 0;
                som_toca_tom(SOM_VALIDO4, 5);
            }
        }
        break;
    }
    case F_VEREDITO:
        // Na vez da IA o veredito passa mais rapido (menos no ultimo dado).
        bool ia_v = (cpu >> vez & 1) && F[vez].lancados < F[vez].n;
        if (t_fase > (ver_tipo == 0 ? (ia_v ? 0.35f : 0.6f) : (ia_v ? 0.8f : 1.3f)) + (hora ? 0.35f : 0)
                     + (vence_j == vez ? 0.5f : 0) && !cosmo_segura()) {
            if (F[vez].lancados >= F[vez].n || vence_j == vez) termina_turno();
            else { cursor = 0; fase = F_JOGA; }
        }
        break;
    case F_EFEITOS:
        if (t_respiro >= 0) {                      // o respiro: o resultado deste fica a vista, e so entao o seguinte
            if ((t_respiro += dt_real) > EV_RESPIRO) proximo_evento();
            break;
        }
        if (t_fase > duracao_evento(ev_i)) {
            aplica_evento(ev_i);
            golpe_evento(&ev[ev_i]);
            if (t_congela > 0) ev_espera = true;   // a mesa para mostrando este; o seguinte espera
            else t_respiro = 0;
        }
        break;
    default:
        break;
    }
}
