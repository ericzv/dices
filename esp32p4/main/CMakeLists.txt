# Tudo num componente so: o aparelho (este diretorio), o console
# (esp32p4/console) e os jogos (src/, os mesmos arquivos do PC).
set(RAIZ ${CMAKE_CURRENT_LIST_DIR}/../..)
set(CONSOLE ${CMAKE_CURRENT_LIST_DIR}/../console)

idf_component_register(
    SRCS
        "main.c" "tela.c" "toque.c" "audio.c" "som_p4.c" "relogio.c" "salva_nvs.c"
        "${CONSOLE}/console.c" "${CONSOLE}/inicio.c" "${CONSOLE}/ajustes.c" "${CONSOLE}/partida.c"
        "${CONSOLE}/primeira.c" "${CONSOLE}/ui.c" "${CONSOLE}/desenho.c" "${CONSOLE}/texto.c"
        "${CONSOLE}/fontes_ttf.c" "${CONSOLE}/catalogo.c" "${CONSOLE}/jogo_dado.c"
        "${RAIZ}/src/games/dado.c" "${RAIZ}/src/ui/gfx.c" "${RAIZ}/src/ui/fonte.c"
        "${RAIZ}/desktop/rede.c"
    INCLUDE_DIRS "." "${CONSOLE}" "${RAIZ}/src"
    EMBED_FILES "sons.bin"
    LDFRAGMENTS "psram.lf"
    REQUIRES
        esp_lcd esp_driver_ppa esp_driver_ledc esp_driver_gpio esp_driver_i2c esp_driver_i2s
        esp_hw_support esp_timer nvs_flash esp_app_format
)

# A versao (o commit) aparece miuda na tela inicial do jogo, como no PC.
idf_build_get_property(versao PROJECT_VER)
target_compile_definitions(${COMPONENT_LIB} PRIVATE DADO_VERSAO="${versao}")
# O jogo e compilado no PC sem -Werror; aqui tambem, so ele.
# (No RISC-V, int32_t e long: os printf do jogo com %d dao aviso de formato.)
set_source_files_properties("${RAIZ}/src/games/dado.c" PROPERTIES COMPILE_OPTIONS
    "-Wno-error=all;-Wno-format;-Wno-format-truncation;-Wno-missing-field-initializers")

# O sons.bin e feito das gravacoes do PC (desktop/sons/*.ogg). Se elas
# mudaram desde que ele foi gerado, avisa (o firmware compila assim mesmo,
# com os sons antigos).
file(GLOB sons_ogg "${RAIZ}/desktop/sons/*.ogg")
if (NOT CMAKE_BUILD_EARLY_EXPANSION AND sons_ogg AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/sons.lista")
    list(SORT sons_ogg)
    set(lista "")
    foreach(f ${sons_ogg})
        get_filename_component(nome "${f}" NAME_WE)
        file(SIZE "${f}" tam)
        string(APPEND lista "${nome} ${tam}\n")
    endforeach()
    file(READ "${CMAKE_CURRENT_LIST_DIR}/sons.lista" gerada)
    string(REPLACE "\r" "" gerada "${gerada}")            # git no Windows pode por CRLF
    if (NOT lista STREQUAL gerada)
        message(WARNING "Os sons do PC (desktop/sons) mudaram depois do main/sons.bin. "
                        "Gere de novo: cmake --build build-console --target sons_p4 (README).")
    endif()
endif()
