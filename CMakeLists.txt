cmake_minimum_required(VERSION 3.17)
project(Compiler_5183)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES Scanner/scanner.cpp compiler.cpp Parser/node.cpp Parser/parser.cpp Parser/symbol_table.cpp)
set(HEADER_FILES Scanner/scanner.h compiler.h Parser/node.h Parser/parser.h Parser/symbol_table.h)

add_executable(Compiler_5183 main.cpp tokenCodes.h ${SOURCE_FILES} ${HEADER_FILES} )