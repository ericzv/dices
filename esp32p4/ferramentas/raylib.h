// "raylib" de mentira para compilar desktop/som.c fora do PC: so o pedaco de
// audio que o som.c usa, e cada som que ele monta fica guardado para o
// gera_sons.c gravar. Nada toca aqui.
#pragma once
#include <stdbool.h>

typedef struct { unsigned int frameCount, sampleRate, sampleSize, channels; void *data; } Wave;
typedef struct { int id; unsigned int frameCount; } Sound;
typedef struct { int id; unsigned int frameCount; bool looping; } Music;

bool  IsAudioDeviceReady(void);
Sound LoadSoundFromWave(Wave w);
Sound LoadSoundAlias(Sound s);
void  UnloadSoundAlias(Sound s);
void  UnloadSound(Sound s);
Music LoadMusicStreamFromMemory(const char *tipo, const unsigned char *dados, int n);
void  UnloadMusicStream(Music m);
void  PlayMusicStream(Music m);
void  UpdateMusicStream(Music m);
void  SetMusicVolume(Music m, float v);
bool  IsSoundPlaying(Sound s);
void  SetSoundVolume(Sound s, float v);
void  SetSoundPitch(Sound s, float p);
void  PlaySound(Sound s);
void  StopSound(Sound s);
