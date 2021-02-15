cmake_minimum_required(VERSION 3.17)
project(Compiler_5183)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES scanner.cpp compiler.cpp compiler.h node.cpp node.h parser.cpp parser.h)
set(HEADER_FILES scanner.h tokenCodes.h compiler.cpp compiler.h node.cpp node.h parser.cpp parser.h)

add_executable(Compiler_5183 main.cpp ${SOURCE_FILES} ${HEADER_FILES})