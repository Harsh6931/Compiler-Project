cmake_minimum_required(VERSION 3.16)
project(lumen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lumen
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/ast/ast_printer.cpp
    src/interpreter/environment.cpp
    src/interpreter/interpreter.cpp
    src/compiler/chunk.cpp
    src/compiler/compiler.cpp
    src/compiler/disassembler.cpp
    src/vm/vm.cpp
)

target_include_directories(lumen PRIVATE src)
