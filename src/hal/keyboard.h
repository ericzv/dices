// Teclado, versao PC. Mesmo contrato do hal/keyboard.h do Cardputer: cada
// tecla chega como um evento; letras e numeros chegam como o proprio ASCII,
// as teclas de controle com os codigos abaixo.
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define KEY_BKSP   0x08
#define KEY_TAB    0x09
#define KEY_ENTER  0x0D
#define KEY_ESC    0x1B
#define KEY_UP     0x100
#define KEY_DOWN   0x101
#define KEY_LEFT   0x102
#define KEY_RIGHT  0x103

typedef struct {
    uint16_t key;
} key_event_t;
