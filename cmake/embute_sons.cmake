# Embute os sons do jogo (desktop/sons/*.ogg) no executavel: gera um .c com
# cada arquivo como um vetor de bytes e uma tabela nome -> dados.
# Roda no build (cmake -P), so quando algum .ogg muda:
#   cmake -DSAIDA=<arquivo.c> -DPASTA=<desktop/sons> [-DFORA=<regex>] -P embute_sons.cmake
# FORA: os arquivos cujo nome casa ficam de fora (as trilhas, na versao do
# navegador, que as busca ao lado da pagina).
# Recebe a pasta, e nao a lista: com centenas de sons, a lista na linha de
# comando passava do limite do Windows.
file(GLOB ARQUIVOS "${PASTA}/*.ogg")
list(SORT ARQUIVOS)
set(c "// GERADO PELO CMAKE (cmake/embute_sons.cmake) - NAO EDITAR\n")
string(APPEND c "// Os sons do jogo, de desktop/sons/*.ogg. Creditos em desktop/sons/CREDITOS.md.\n")
string(APPEND c "#include \"sons_embutidos.h\"\n\n")
set(tabela "")
set(i 0)
foreach(arq IN LISTS ARQUIVOS)
    if (arq STREQUAL "")
        continue()
    endif()
    get_filename_component(nome ${arq} NAME_WE)
    if (FORA AND nome MATCHES "${FORA}")
        continue()
    endif()
    file(READ ${arq} hex HEX)
    string(LENGTH "${hex}" n)
    math(EXPR n "${n} / 2")
    # 0xNN, 0xNN, ... com uma quebra de linha a cada 24 bytes
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," hex "${hex}")
    string(REGEX REPLACE "((0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,)(0x..,))" "\\1 " hex "${hex}")
    string(REGEX REPLACE "([^ ]+ [^ ]+ [^ ]+ )" "\\1\n" hex "${hex}")   # o regex do CMake nao tem {n}
    string(REPLACE " \n" "\n" hex "${hex}")
    string(APPEND c "static const unsigned char s${i}[${n}] = {\n${hex}\n};\n")
    string(APPEND tabela "    { \"${nome}\", s${i}, ${n} },\n")
    math(EXPR i "${i} + 1")
endforeach()
string(APPEND c "\nconst som_arquivo_t SONS_ARQ[] = {\n${tabela}};\nconst int SONS_N = ${i};\n")
file(WRITE ${SAIDA} "${c}")
