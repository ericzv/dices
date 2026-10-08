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
    static const char *const LOOPS[] = { "rufo", "moeda_rola", "giro", "trilha" };
    for (int i = 0; i < 4; i++)
        if (!acha(LOOPS[i])) { printf("falta %s\n", LOOPS[i]); erros++; }

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
