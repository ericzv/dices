// Confere os sons do jogo sem abrir a placa de som: todo efeito tem ao menos
// uma gravacao embutida, todas decodificam e nenhuma esta muda. Exporta tudo
// em WAV, para ouvir fora do jogo e conferir niveis.
//   exporta_som DIR
#include "../desktop/som.c"

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    SetTraceLogLevel(LOG_WARNING);
    int erros = 0;
    for (int s = 0; s < SOM_N; s++) {
        char nome[32];
        snprintf(nome, sizeof nome, "%s_0", NOME[s]);
        if (!NOME[s] || (!acha(NOME[s]) && !acha(nome))) {
            printf("falta a gravacao do som %d (%s)\n", s, NOME[s] ? NOME[s] : "?");
            erros++;
        }
    }
    static const char *const LOOPS[] = { "rufo", "moeda_rola", "giro", "elevador_motor", "trilha", "trilha_chefe" };
    for (int i = 0; i < (int)(sizeof LOOPS / sizeof *LOOPS); i++)
        if (!acha(LOOPS[i])) { printf("falta %s\n", LOOPS[i]); erros++; }
    // As camadas (a batucada e a voz de cada chefe) tocam junto com a trilha,
    // amostra a amostra: do mesmo tamanho dela (senao o som.c as deixa de fora).
    const char *camadas[CAMADAS + SOM_CHEFES];
    int nc = 0;
    for (int c = 0; c < CAMADAS; c++) if (CAM_ARQ[c]) camadas[nc++] = CAM_ARQ[c];
    for (int q = 0; q < SOM_CHEFES; q++) if (VOZ_ARQ[q]) camadas[nc++] = VOZ_ARQ[q];
    unsigned n_trilha = 0;
    for (int c = 0; c < nc; c++) {
        const som_arquivo_t *a = acha(camadas[c]);
        if (!a) { printf("falta %s\n", camadas[c]); erros++; continue; }
        Wave w = LoadWaveFromMemory(".ogg", a->dados, a->tam);
        if (c == 0) n_trilha = w.frameCount;
        else if (w.frameCount != n_trilha) {
            printf("%s: %u quadros, a trilha tem %u\n", camadas[c], w.frameCount, n_trilha);
            erros++;
        }
        if (w.channels != 2 || w.sampleRate != 44100) { printf("%s: nao e estereo a 44100 Hz\n", camadas[c]); erros++; }
        UnloadWave(w);
        // A voz e decodificada aos poucos no jogo (o stb_vorbis do raylib): tem
        // que abrir e dizer o mesmo tamanho.
        if (c >= 2) {
            int e = 0;
            stb_vorbis *v = stb_vorbis_open_memory(a->dados, (int)a->tam, &e, NULL);
            if (!v || stb_vorbis_stream_length_in_samples(v) != n_trilha || stb_vorbis_get_info(v).sample_rate != 44100) {
                printf("%s: o stb_vorbis nao abre do tamanho da trilha\n", camadas[c]);
                erros++;
            }
            if (v) stb_vorbis_close(v);
        }
    }

    for (int i = 0; i < SONS_N; i++) {
        const som_arquivo_t *a = &SONS_ARQ[i];
        Wave w = LoadWaveFromMemory(".ogg", a->dados, a->tam);
        if (!w.frameCount || !w.data) { printf("%s: nao decodificou\n", a->nome); erros++; continue; }
        WaveFormat(&w, w.sampleRate, 16, w.channels);
        const short *x = w.data;
        int pico = 0;
        for (unsigned k = 0; k < w.frameCount * w.channels; k++) {
            int v = x[k] < 0 ? -x[k] : x[k];
            if (v > pico) pico = v;
        }
        if (pico < 300) { printf("%s: muda\n", a->nome); erros++; }
        char path[512];
        snprintf(path, sizeof path, "%s/%s.wav", dir, a->nome);
        if (!ExportWave(w, path)) erros++;
        printf("%-12s %6.2f s  %d canal(is)  pico %.2f\n", a->nome,
               (double)w.frameCount / w.sampleRate, (int)w.channels, pico / 32768.0);
        UnloadWave(w);
    }
    if (erros) printf("%d erro(s)\n", erros);
    return erros ? 1 : 0;
}
