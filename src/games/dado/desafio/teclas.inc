// ---------------------------------------------------------------------------
// Teclas
// ---------------------------------------------------------------------------
static bool des_tecla(int k, int e)
{
    switch (fase) {
    case F_MAPA:
        if (e) {
            for (int i = 1; i <= 3; i++) {
                int l = ((des_cur + e * i) % 3 + 3) % 3;
                if (escolhivel(l)) { des_cur = l; som_toca(SOM_TIQUE); break; }
            }
            return true;
        }
        if (k == KEY_ENTER && escolhivel(des_cur)) {
            som_toca(SOM_FICHA);
            run.caminho[run.passo] = (int8_t)des_cur;
            if (run.no[run.passo][des_cur].tipo == NO_EVENTO) {
                entra_evento();
            } else if (run.no[run.passo][des_cur].tipo == NO_LOJA) {
                run.estado = R_LOJA;
                monta_loja_run();
                des_salva();
                abre_loja_run();
            } else {
                des_comeca_partida();
            }
            return true;
        }
        if (k == KEY_TAB) { cursor = 0; fase = F_RBOLSA; }
        return true;

    case F_RLOJA: {
        int nc = N_LOJA + (run.am_loja < A_N ? 1 : 0);
        if (e) { cursor = (cursor + e + nc) % nc; som_toca(SOM_TIQUE); return true; }
        if (k >= '1' && k < '1' + nc) cursor = k - '1';
        if ((k == KEY_ENTER || (k >= '1' && k < '1' + nc)) && cursor == N_LOJA) {
            if (run.am_levou || run.am_loja >= A_N || run.moedas < preco_am()) return true;
            run.moedas -= preco_am();
            run_poe_amuleto(run.am_loja);
            run.am_levou = 1;
            som_toca(SOM_FICHA);
            des_salva();
            return true;
        }
        if (k == KEY_ENTER || (k >= '1' && k <= '5')) {
            int tp = run.loja[cursor].tipo, pr = preco_run(tp);
            if (run.levou[cursor] || run.moedas < pr || run.n_col >= MAX_COL) return true;
            run.moedas -= pr;
            run.col[run.n_col++] = run.loja[cursor];
            run.levou[cursor] = 1;
            som_toca(SOM_FICHA);
            des_salva();
            return true;
        }
        if (k == KEY_TAB || k == 'x') {
            if (run.estado == R_LOJA_INI) { run.estado = R_MAPA; des_salva(); abre_mapa(); }
            else avanca_passo();
        }
        return true;
    }

    case F_RAMULETO: {
        if (e) {
            for (int i = 1; i <= 3; i++) {
                int c = ((cursor + e * i) % 3 + 3) % 3;
                if (run.am_oferta[c] < A_N) { cursor = c; som_toca(SOM_TIQUE); break; }
            }
            return true;
        }
        if (k >= '1' && k <= '3' && run.am_oferta[k - '1'] < A_N) cursor = k - '1';
        if (k == KEY_ENTER || (k >= '1' && k <= '3')) amuleto_escolhido(cursor);
        return true;
    }

    case F_RBOLSA:
        if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= GRADE_B) cursor -= GRADE_B; return true; }
        if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + GRADE_B < run.n_col) cursor += GRADE_B; return true; }
        if (e && run.n_col) {
            int tot = run.n_col + am_conta(run_amuletos());
            cursor = (cursor + e + tot) % tot;
            som_toca(SOM_TIQUE);
            return true;
        }
        if (k == KEY_TAB || k == KEY_ENTER || k == KEY_BKSP || k == 'x') { t_fase = 0; fase = F_MAPA; }
        return true;

    case F_RFIM:
        if (k != KEY_ENTER) return true;
        if (des_venceu_run) { t_fase = 0; fase = F_CREDITOS; }   // venceu: os creditos antes do menu
        else volta_ao_menu();
        return true;

    case F_CREDITOS:
        if (k == KEY_ENTER) volta_ao_menu();
        return true;

    case F_REVENTO:
        return ev_tecla(k, e);

    case F_PERFIS: {
        bool cheio = perfis[pf_cur].nome[0] != 0;
        if (pf_apagando) {
            if ((k == KEY_ENTER || k == 'x') && cheio) {
                int antes = perfil_atual;
                perfil_atual = pf_cur;
                salva_apaga(chave_run());
                memset(&perfis[pf_cur], 0, sizeof perfis[pf_cur]);
                perfil_atual = antes == pf_cur ? -1 : antes;
                perfis_salva();
                som_toca(SOM_FICHA);
            }
            pf_apagando = false;
            return true;
        }
        if (e) { pf_cur = (pf_cur + e + N_PERFIS) % N_PERFIS; som_toca(SOM_TIQUE); return true; }
        if (k >= '1' && k <= '3') { pf_cur = k - '1'; k = KEY_ENTER; cheio = perfis[pf_cur].nome[0] != 0; }
        if ((k == 'x' || k == KEY_BKSP) && cheio) { pf_apagando = true; return true; }
        if (k != KEY_ENTER) return true;
        som_toca(SOM_FICHA);
        if (cheio) { des_entra_perfil(pf_cur); return true; }
        pf_avatar = pf_cur % AV_N;
        pf_nome[0] = 0;
        t_fase = 0;
        fase = F_PCRIA;
        return true;
    }

    case F_PCRIA: {
        int n = (int)strlen(pf_nome);
        if (k == KEY_LEFT || k == KEY_RIGHT) {
            pf_avatar = (pf_avatar + (k == KEY_LEFT ? AV_N - 1 : 1)) % AV_N;
            som_toca(SOM_TIQUE);
            return true;
        }
        if (k == KEY_BKSP) { if (n) pf_nome[n - 1] = 0; return true; }
        if (k == KEY_ENTER) {
            if (!n) return true;
            while (n && pf_nome[n - 1] == ' ') pf_nome[--n] = 0;
            perfil_t *p = &perfis[pf_cur];
            memset(p, 0, sizeof *p);
            snprintf(p->nome, sizeof p->nome, "%s", pf_nome);
            p->avatar = (uint8_t)pf_avatar;
            som_toca(SOM_PREMIO);
            des_entra_perfil(pf_cur);
            return true;
        }
        bool letra = k >= 'a' && k <= 'z', num = k >= '0' && k <= '9';
        if (n < 10 && (letra || num || (k == ' ' && n > 0 && pf_nome[n - 1] != ' '))) {
            pf_nome[n] = (char)(letra ? k - 32 : k);
            pf_nome[n + 1] = 0;
            som_toca(SOM_TIQUE);
        }
        return true;
    }

    case F_RDIFIC: {
        int nv = perfil()->nivel, ult = nv < N_NIVEIS ? nv + 1 : N_NIVEIS;
        if (e) { dif_cur = (dif_cur + e + ult + 1) % (ult + 1); som_toca(SOM_TIQUE); return true; }
        if (k == KEY_ENTER && dif_cur <= nv) {
            som_toca(SOM_FICHA);
            des_dif = dif_cur;
            des_nova_run();
        }
        return true;
    }

    default:
        return false;
    }
}
