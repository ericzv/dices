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
    ${RAIZ}/src/games/dado.c ${RAIZ}/src/ui/gfx.c ${RAIZ}/src/ui/fonte.c ${RAIZ}/src/ui/fonte_tela.c
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
    # A janela usa o som de verdade do PC (desktop/som.c, sintetizado com o raylib).
    add_executable(console_sim sim_janela.c sistema_pc.c ${RAIZ}/desktop/som.c)
    target_link_libraries(console_sim PRIVATE console raylib)
endif()
