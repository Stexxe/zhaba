cmake_minimum_required(VERSION 3.31)
project(zhaba C)

set(CMAKE_C_STANDARD 11)

add_executable(zhaba main.c common.c lexer.c parser.c file_render.c)
