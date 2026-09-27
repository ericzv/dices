idf_component_register(
    SRCS
        "main.c"
        "hal/display.c"
        "hal/keyboard.c"
        "hal/bateria.c"
        "hal/som.c"
        "ui/gfx.c"
        "ui/lista.c"
        "ui/icones.c"
        "ui/sprites.c"
        "ui/tabela.c"
        "ui/font6x12.c"
        "data/pack.c"
        "data/query.c"
        "data/lexico.c"
        "data/historia.c"
        "apps/menu.c"
        "apps/amb.c"
        "apps/vvpp.c"
        "apps/diag.c"
        "apps/arcade.c"
        "games/garimpo.c"
        "games/dado.c"
        "games/travessia.c"
        "games/fantasma.c"
        "apps/estado.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_lcd driver esp_partition nvs_flash esp_hw_support
        esp_adc esp_timer esp_driver_i2s esp_driver_i2c
)
