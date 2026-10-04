# Console de jogos para o Guition JC8012P4A1 (ESP32-P4, tela de 10,1").
#
#   cd esp32p4
#   idf.py set-target esp32p4
#   idf.py build flash monitor
#
# Placas com ESP32-P4 anterior a v3.0 precisam da outra configuracao (o
# README explica): idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.p4_antigo" build
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(console_p4)
