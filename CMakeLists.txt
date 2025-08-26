cmake_minimum_required(VERSION 3.31)
project(zhaba C)

set(CMAKE_C_STANDARD 11)

add_library(zhaba_lib STATIC lib/common.c lib/lexer.c lib/parser.c lib/file_render.c lib/html_render.c lib/html_writer.c lib/lib.c)

add_executable(zhaba main.c)
target_link_libraries(zhaba PRIVATE zhaba_lib)

add_subdirectory(test)
