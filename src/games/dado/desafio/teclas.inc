// ---------------------------------------------------------------------------
// Teclas
// ---------------------------------------------------------------------------
static bool des_tecla(int k, int e)
{
    switch (fase) {
    case F_MAPA:
        // As setas passam pelas mesas do salao (para ver quem vem); ENTER
        // senta a mesa da vez.
        if (e) {
            int n = mesas_do_salao();
            des_cur = (des_cur + e + n) % n;
            som_toca(SOM_TIQUE);
            return true;
        }
        if (k == KEY_ENTER) {
            som_toca(SOM_FICHA);
            des_comeca_partida();
            return true;
        }
        if (k == KEY_TAB) { cursor = 0; fase = F_RBOLSA; }
        return true;

    case F_RLOJA:
        return loja_tecla(k, e);

    case F_RAMULETO: {
        if (run.am_novo) {                       // a troca: os seus e, no fim, deixar o novo
            if (e) { cursor = (cursor + e + TROCA_N) % TROCA_N; som_toca(SOM_TIQUE); return true; }
            if (k == KEY_BKSP || k == 'x') { amuleto_desiste(); return true; }
            if (k == KEY_ENTER) {
                int meus[A_N], n = meus_amuletos(meus);
                amuleto_troca(cursor >= 0 && cursor < n && cursor < TROCA_N - 1 ? meus[cursor] : -1);
            }
            return true;
        }
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

    case F_RBOLSA: {
        int por = grade_bolsa(run.n_col).por;
        if (k == 'w' || k == ';' || k == KEY_UP)   { if (cursor >= por) cursor -= por; return true; }
        if (k == 's' || k == '.' || k == KEY_DOWN) { if (cursor + por < run.n_col) cursor += por; return true; }
        if (e && run.n_col) {
            int tot = run.n_col + am_conta(run_amuletos());
            cursor = (cursor + e + tot) % tot;
            som_toca(SOM_TIQUE);
            return true;
        }
        if (k == KEY_TAB || k == KEY_ENTER || k == KEY_BKSP || k == 'x') { t_fase = 0; fase = F_MAPA; }
        return true;
    }

    case F_RFIM:
        if (k != KEY_ENTER) return true;
        if (des_venceu_run && n_nov) nov_abre();                 // venceu: o que abriu, card a card
        else if (des_venceu_run) { t_fase = 0; fase = F_CREDITOS; }   // e os creditos antes do menu
        else volta_ao_menu();
        return true;

    case F_NOVIDADES:
        return nov_tecla(k);

    case F_CREDITOS:
        if (k == KEY_ENTER) volta_ao_menu();
        return true;

    case F_PERFIS: {
        bool cheio = perfis[pf_cur].nome[0] != 0;
        if (pf_apagando) {
            if ((k == KEY_ENTER || k == 'x') && cheio) {
                int antes = perfil_atual;
                perfil_atual = pf_cur;
                salva_apaga(chave_run());
                hist_apaga(pf_cur);
                salva_apaga(chave_estilo(pf_cur));
                memset(&perfis[pf_cur], 0, sizeof perfis[pf_cur]);
                perfil_atual = antes == pf_cur ? -1 : antes;
                perfis_salva();
                estilo_do_perfil();
                som_toca(SOM_FICHA);
            }
            pf_apagando = false;
            return true;
        }
        if (e) { pf_cur = (pf_cur + e + N_PERFIS) % N_PERFIS; som_toca(SOM_TIQUE); return true; }
        if (k >= '1' && k <= '3') { pf_cur = k - '1'; k = KEY_ENTER; cheio = perfis[pf_cur].nome[0] != 0; }
        if ((k == 'x' || k == KEY_BKSP) && cheio) { pf_apagando = true; return true; }
        if (k == 'h' && cheio) { som_toca(SOM_TIQUE); hist_abre(pf_cur); return true; }
        if (k == 'e' && cheio) { som_toca(SOM_TIQUE); estilo_abre(pf_cur); return true; }
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

    case F_HIST:
        return hist_tecla(k, e);

    case F_ESTILO:
        return estilo_tecla(k);

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
