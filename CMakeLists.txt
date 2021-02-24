cmake_minimum_required(VERSION 3.17)
project(Compiler_5183)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES scanner.cpp compiler.cpp node.cpp parser.cpp)
set(HEADER_FILES scanner.h   compiler.h   node.h   parser.h)

add_executable(Compiler_5183 main.cpp tokenCodes.h ${SOURCE_FILES} ${HEADER_FILES})