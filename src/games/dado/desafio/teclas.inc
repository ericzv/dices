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

    case F_RAVISO:                               // o aviso: ENTER mostra a frase toda; de novo, segue
        if (k != KEY_ENTER) return true;
        if (t_fase < aviso_pronto()) { t_fase = aviso_pronto(); return true; }
        som_toca(SOM_FICHA);
        des_pos_aviso();
        return true;

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
                salva_apaga(chave_vistos(pf_cur));
                vistos_de = -2;                  // relidos na proxima vez
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
        // H e E; no controle, o Y (TAB) e o X (I), que aqui nao fazem outra coisa.
        if ((k == 'h' || k == KEY_TAB) && cheio) { som_toca(SOM_TIQUE); hist_abre(pf_cur); return true; }
        if ((k == 'e' || k == 'i') && cheio) { som_toca(SOM_TIQUE); estilo_abre(pf_cur); return true; }
        if (k != KEY_ENTER) return true;
        som_toca(SOM_FICHA);
        if (cheio) { des_entra_perfil(pf_cur); return true; }
        pf_avatar = pf_cur % AV_N;
        pf_nome[0] = 0;
        tt_zera();                               // o teclado na tela, do controle: na primeira letra
        t_fase = 0;
        fase = F_PCRIA;
        return true;
    }

    case F_PCRIA: {
        int n = (int)strlen(pf_nome);
        if (tt_ativo()) {                        // com o controle: o teclado na tela
            if ((k == KEY_LEFT || k == KEY_RIGHT) && tt_lin < 0) {
                pf_avatar = (pf_avatar + (k == KEY_LEFT ? AV_N - 1 : 1)) % AV_N;
                som_toca(SOM_TIQUE);
                return true;
            }
            if (tt_anda(k, -1, true)) return true;
            if (k == KEY_TAB) k = KEY_ENTER;     // o Y cria
            else if (k == KEY_ENTER && tt_lin < 0) { tt_lin = 0; som_toca(SOM_TIQUE); return true; }
            else if (k == KEY_ENTER && tt_lin < TT_LINHAS) {
                char c = tt_aperta(true);
                if (!c) return true;
                k = c >= 'A' && c <= 'Z' ? c + 32 : c;   // escreve como o teclado
            }
        }
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
        // A tela e um menu de tres botoes: "Iniciar a run" embaixo (em foco
        // ao abrir) e, em cima, "Desbloqueios" e "Trocar de andar". As setas
        // passam o foco de um a outro e ENTER (o A do controle) aperta o que
        // esta em foco; o mouse em cima poe o foco e o clique aperta.
        // "Trocar de andar" acende o predio: as setas trocam o andar (passar
        // do ultimo aberto aperta o trancado de cima, que nega; do Terreo
        // para baixo nao tem nada) e ENTER confirma, com o foco de volta no
        // "Iniciar a run"; ESC (o B) volta ao andar de antes. O clique num
        // andar (no predio ou na botoeira) so vale com ele aceso, e escolhe.
        // No pop-up dos desbloqueios as setas nao fazem nada e o resto
        // fecha. I (o X do controle) abre o pop-up direto. Fora disso, o B
        // volta ao menu, como o ESC.
        int nv = perfil()->nivel < N_NIVEIS ? perfil()->nivel : N_NIVEIS;   // so as abertas
        if (dif_foco != DIF_INICIAR) dif_foco_cima = dif_foco;   // o de cima onde o foco estava (seta, mouse ou ENTER)
        if (k >= TECLA_TRANCADO && k <= TECLA_TRANCADO + N_NIVEIS) { predio_nega(k - TECLA_TRANCADO); return true; }
        if (k >= TECLA_ANDAR && k <= TECLA_ANDAR + N_NIVEIS) {   // o clique num andar: so com "Trocar de andar" aceso
            int nd = k - TECLA_ANDAR;
            if (dif_escolhe && nd <= nv) {
                if (nd != dif_cur) som_toca(SOM_TIQUE);
                dif_cur = nd;
                dif_foco = DIF_INICIAR;
            }
            dif_escolhe = false;
            return true;
        }
        bool cima = k == KEY_UP || k == 'w' || k == ';', baixo = k == KEY_DOWN || k == 's' || k == '.';
        bool esq = k == KEY_LEFT || k == 'a' || k == ',', dir = k == KEY_RIGHT || k == 'd' || k == '/';
        if (dif_escolhe) {                       // o andar: para cima (ou a direita) sobe
            int sobe = cima || dir ? 1 : baixo || esq ? -1 : 0;
            if (sobe) {
                int nd = (dif_cur < 0 ? 0 : dif_cur > nv ? nv : dif_cur) + sobe;
                if (nd > nv && nd <= N_NIVEIS) predio_nega(nd);
                else if (nd >= 0 && nd <= nv) { dif_cur = nd; som_toca(SOM_TIQUE); }
                return true;
            }
        }
        if (dif_pop) {                           // o pop-up: as setas nao fazem nada; o resto fecha
            if (k == KEY_ENTER || k == KEY_BKSP || k == 'i' || k == 'x') { dif_pop = false; som_toca(SOM_TIQUE); }
            return true;
        }
        int aperta = k == KEY_ENTER ? dif_foco : k == 'i' ? DIF_DESBLOQ : -1;
        if (dif_escolhe) {
            if (k == KEY_BKSP) { dif_cur = dif_antes; dif_escolhe = false; som_toca(SOM_TIQUE); return true; }
            if (aperta < 0) return true;
            dif_escolhe = false;                 // ENTER confirma o andar; o "Iniciar a run" (o mouse em cima) ja comeca
            if (aperta == DIF_TROCA) { dif_foco = DIF_INICIAR; som_toca(SOM_TIQUE); return true; }
        } else if (cima || baixo || esq || dir) {   // o foco: os dois de cima lado a lado, o "Iniciar a run" embaixo
            int f = dif_foco;
            if (cima && f == DIF_INICIAR) f = dif_foco_cima;
            else if (baixo && f != DIF_INICIAR) f = DIF_INICIAR;
            else if (esq && f == DIF_TROCA) f = DIF_DESBLOQ;
            else if (dir && f == DIF_DESBLOQ) f = DIF_TROCA;
            if (f != dif_foco) { dif_foco = f; som_toca(SOM_TIQUE); }
            return true;
        } else if (k == KEY_BKSP) {             // o B volta ao menu, como o ESC
            des_esc();
            return true;
        }
        if (aperta == DIF_DESBLOQ) { dif_pop = true; dif_pop_t = t_relogio; som_toca(SOM_TIQUE); return true; }
        if (aperta == DIF_TROCA) { dif_antes = dif_cur; dif_escolhe = true; som_toca(SOM_TIQUE); return true; }
        if (aperta == DIF_INICIAR && dif_cur >= 0 && dif_cur <= nv) {
            som_toca(SOM_FICHA);
            des_dif = dif_cur;
            abre_bolsas();                       // e a bolsa da run
        }
        return true;
    }

    case F_RBOLSAS:
        return bs_tecla(k, e);

    default:
        return false;
    }
}
