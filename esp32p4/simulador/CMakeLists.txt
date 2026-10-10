# Simulador do console no PC: o mesmo console e o mesmo jogo do firmware, com
# o mouse no lugar do dedo.
#
#   cmake -S esp32p4/simulador -B build-console
#   cmake --build build-console
#   ./build-console/console_sim            # janela (precisa do raylib, baixado sozinho)
#   ./build-console/console_fotos /tmp/f   # sem janela: fotos de cada tela
cmake_minimum_required(VERSION 3.16)
project(ConsoleSimulador C)

set(CMAKE_C_STANDARD 11)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(RAIZ ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(CONSOLE ${CMAKE_CURRENT_SOURCE_DIR}/../console)

add_library(console STATIC
    ${CONSOLE}/console.c ${CONSOLE}/inicio.c ${CONSOLE}/ajustes.c ${CONSOLE}/partida.c
    ${CONSOLE}/primeira.c ${CONSOLE}/ui.c ${CONSOLE}/desenho.c ${CONSOLE}/texto.c
    ${CONSOLE}/fontes_ttf.c ${CONSOLE}/catalogo.c ${CONSOLE}/jogo_dado.c
    ${RAIZ}/src/games/dado.c ${RAIZ}/src/ui/gfx.c ${RAIZ}/src/ui/fonte.c
    ${RAIZ}/desktop/rede.c)
# desktop/ da o esp_timer.h de mentira que o dado.c inclui.
target_include_directories(console PUBLIC ${CONSOLE} ${RAIZ}/src ${RAIZ}/desktop ${CMAKE_CURRENT_SOURCE_DIR})
if (MSVC)
    target_compile_options(console PUBLIC /FI${RAIZ}/desktop/msvc.h /wd4244 /wd4267 /wd4305)
else()
    target_compile_options(console PRIVATE -Wall -Wno-unused-parameter -Wno-unused-function)
endif()
if (UNIX)
    target_link_libraries(console PUBLIC m)
endif()

add_executable(console_fotos sim_fotos.c sistema_pc.c)
target_link_libraries(console_fotos PRIVATE console)

option(CONSOLE_JANELA "Compila o simulador com janela (raylib)" ON)
if (CONSOLE_JANELA)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(raylib)
    # Os sons do PC (desktop/sons/*.ogg), embutidos como no jogo de PC.
    file(GLOB SONS_OGG CONFIGURE_DEPENDS ${RAIZ}/desktop/sons/*.ogg)
    list(SORT SONS_OGG)
    string(REPLACE ";" "|" SONS_LISTA "${SONS_OGG}")
    set(SONS_C ${CMAKE_CURRENT_BINARY_DIR}/sons_embutidos.c)
    add_custom_command(OUTPUT ${SONS_C}
        COMMAND ${CMAKE_COMMAND} -DSAIDA=${SONS_C} -DARQUIVOS=${SONS_LISTA}
                -P ${RAIZ}/cmake/embute_sons.cmake
        DEPENDS ${SONS_OGG} ${RAIZ}/cmake/embute_sons.cmake
        COMMENT "Embutindo os sons do jogo"
        VERBATIM)
    # A janela usa o som de verdade do PC (desktop/som.c, com o raylib).
    add_executable(console_sim sim_janela.c sistema_pc.c ${RAIZ}/desktop/som.c ${SONS_C})
    target_link_libraries(console_sim PRIVATE console raylib)

    # O sons.bin do firmware: as mesmas gravacoes em ADPCM, abertas pelo
    # stb_vorbis que vem com o raylib.
    #   cmake --build build-console --target sons_p4
    add_executable(gera_sons ../ferramentas/gera_sons.c)
    target_include_directories(gera_sons PRIVATE ${raylib_SOURCE_DIR}/src/external)
    if (UNIX)
        target_link_libraries(gera_sons PRIVATE m)
    endif()
    add_custom_target(sons_p4
        COMMAND gera_sons ${CMAKE_CURRENT_SOURCE_DIR}/../main/sons.bin ${SONS_OGG}
        DEPENDS gera_sons
        COMMENT "Gerando esp32p4/main/sons.bin"
        VERBATIM)
endif()
