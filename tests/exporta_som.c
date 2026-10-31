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
    static const char *const LOOPS[] = { "rufo", "moeda_rola", "giro", "trilha", "trilha_chefe", "trilha_tensao" };
    for (int i = 0; i < (int)(sizeof LOOPS / sizeof *LOOPS); i++)
        if (!acha(LOOPS[i])) { printf("falta %s\n", LOOPS[i]); erros++; }
    // As camadas tocam junto com a trilha, amostra a amostra: do mesmo tamanho
    // dela (senao o som.c as deixa de fora).
    unsigned n_trilha = 0;
    for (int c = 0; c < CAMADAS; c++) {
        const som_arquivo_t *a = acha(CAM_ARQ[c]);
        if (!a) continue;
        Wave w = LoadWaveFromMemory(".ogg", a->dados, a->tam);
        if (c == CAM_TRILHA) n_trilha = w.frameCount;
        else if (w.frameCount != n_trilha) {
            printf("%s: %u quadros, a trilha tem %u\n", CAM_ARQ[c], w.frameCount, n_trilha);
            erros++;
        }
        if (w.channels != 2 || w.sampleRate != 44100) { printf("%s: nao e estereo a 44100 Hz\n", CAM_ARQ[c]); erros++; }
        UnloadWave(w);
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
