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
    src/ui/font6x12.c)
target_include_directories(jogo PUBLIC src desktop)
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

set(FONTES desktop/main.c desktop/som.c)
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
if (NOT MSVC AND NOT APPLE)
    target_link_options(dado PRIVATE $<$<CONFIG:Release>:-s>)
endif()

# Teste de host: joga partidas inteiras com teclas ao acaso, sem janela.
option(DADO_TESTES "Compila o teste de host" ON)
if (DADO_TESTES AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_executable(simula tests/simula.c)
    target_link_libraries(simula PRIVATE jogo)
    add_test(NAME simula COMMAND simula 200)
endif()
