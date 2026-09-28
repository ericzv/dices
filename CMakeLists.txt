# DADO EM CASA - versao PC (Windows, macOS, Linux).
#
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#
# O raylib (janela, teclado, audio) e baixado e compilado junto, estatico: o
# executavel final nao depende de nenhuma DLL fora do sistema.
# O firmware do Cardputer usa cardputer/CMakeLists.txt (ESP-IDF), nao este.
cmake_minimum_required(VERSION 3.16)
project(DadoEmCasa VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()
# MSVC: runtime estatico, para o .exe nao pedir o Visual C++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
if (EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "" FORCE)      # raylib para o navegador
endif()
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(raylib)

# O jogo em si: os mesmos arquivos do firmware.
add_library(jogo STATIC
    src/games/dado.c
    src/ui/gfx.c
    src/ui/fonte.c
    src/ui/fonte_tela.c)
target_include_directories(jogo PUBLIC src desktop)
# A versao (o commit) aparece miuda na tela inicial: da para saber qual jogo
# esta aberto, no PC ou no site.
find_package(Git QUIET)
if (GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE DADO_VERSAO OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()
if (NOT DADO_VERSAO)
    set(DADO_VERSAO "dev")
endif()
target_compile_definitions(jogo PRIVATE DADO_VERSAO="${DADO_VERSAO}")
if (MSVC)
    # __attribute__((format)) e coisa de GCC/Clang.
    target_compile_options(jogo PUBLIC /FI${CMAKE_CURRENT_SOURCE_DIR}/desktop/msvc.h
                                       /wd4244 /wd4267 /wd4305)
else()
    target_compile_options(jogo PRIVATE -Wall -Wno-unused-parameter)
endif()
if (UNIX)
    target_link_libraries(jogo PUBLIC m)
endif()

if (EMSCRIPTEN)
    set(FONTES desktop/main.c desktop/som.c web/rede.c)
else()
    set(FONTES desktop/main.c desktop/som.c desktop/rede.c)
endif()
if (WIN32)
    list(APPEND FONTES desktop/dado.rc)
endif()
if (APPLE)
    set(ICNS ${CMAKE_CURRENT_SOURCE_DIR}/desktop/icone.icns)
    set_source_files_properties(${ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    list(APPEND FONTES ${ICNS})
endif()

add_executable(dado WIN32 MACOSX_BUNDLE ${FONTES})
target_link_libraries(dado PRIVATE jogo raylib)
set_target_properties(dado PROPERTIES
    OUTPUT_NAME "DadoEmCasa"
    MACOSX_BUNDLE_BUNDLE_NAME "Dado em Casa"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.ericzv.dadoemcasa"
    MACOSX_BUNDLE_ICON_FILE "icone.icns"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
if (MSVC)
    target_link_options(dado PRIVATE /ENTRY:mainCRTStartup)
elseif (MINGW)
    target_link_options(dado PRIVATE -static)
endif()
if (EMSCRIPTEN)
    # Versao do navegador: index.html + index.js + index.wasm, mais a
    # biblioteca MQTT do modo online e o icone ao lado.
    set_target_properties(dado PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
    target_link_options(dado PRIVATE
        -sUSE_GLFW=3 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=64MB
        "-sEXPORTED_FUNCTIONS=_main,_dado_online_comeca,_dado_rede_tecla,_dado_dbg_fichas,_dado_dbg_rodada,_dado_dbg_fase,_dado_dbg_vez,_dado_dbg_pote"
        "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$stringToUTF8,$UTF8ToString"
        "-sEXPORTED_RUNTIME_METHODS=HEAPF32,HEAP8,HEAPU8,HEAP16,HEAP32"   # o miniaudio le Module.HEAPF32
        --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
    set_property(TARGET dado APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
    configure_file(web/mqtt.min.js mqtt.min.js COPYONLY)
    configure_file(desktop/icone.png icone.png COPYONLY)
elseif (NOT MSVC AND NOT APPLE)
    target_link_options(dado PRIVATE $<$<CONFIG:Release>:-s>)
endif()

# Teste de host: joga partidas inteiras com teclas ao acaso, sem janela.
option(DADO_TESTES "Compila o teste de host" ON)
if (DADO_TESTES AND NOT CMAKE_CROSSCOMPILING AND NOT EMSCRIPTEN)
    enable_testing()
    add_executable(simula tests/simula.c desktop/rede.c)
    target_link_libraries(simula PRIVATE jogo)
    add_test(NAME simula COMMAND simula 120)
    # Regras dos dados com jogadas montadas a mao.
    add_executable(regras tests/regras.c src/ui/gfx.c src/ui/fonte.c src/ui/fonte_tela.c desktop/rede.c)
    target_include_directories(regras PRIVATE src desktop)
    if (MSVC)
        target_compile_options(regras PRIVATE /FI${CMAKE_CURRENT_SOURCE_DIR}/desktop/msvc.h)
    endif()
    if (UNIX)
        target_link_libraries(regras PRIVATE m)
    endif()
    add_test(NAME regras COMMAND regras)
    # Exporta a trilha, o rufar e os efeitos em WAV (e confere que a sintese roda).
    add_executable(exporta_som tests/exporta_som.c)
    target_include_directories(exporta_som PRIVATE src desktop)
    target_link_libraries(exporta_som PRIVATE raylib)
    add_test(NAME sons COMMAND exporta_som ${CMAKE_CURRENT_BINARY_DIR})
endif()
