cmake_minimum_required(VERSION 3.17)
project(Compiler_5183)

#set(LLVM_PATH /usr/local/opt/llvm/)

find_package(LLVM REQUIRED CONFIG)

#link_directories(${LLVM_PATH}/lib)
#include_directories(${LLVM_PATH}/include)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
#add_definitions(
#        -D__STDC_LIMIT_MACROS
#        -D__STDC_CONSTANT_MACROS
#)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES Scanner/scanner.cpp compiler.cpp Parser/node.cpp Parser/parser.cpp Parser/symbol_table.cpp  CodeGeneration/code_generation.cpp )
set(HEADER_FILES Scanner/scanner.h compiler.h Parser/node.h Parser/parser.h Parser/symbol_table.h CodeGeneration/code_generation.h CodeGeneration/ExprAst.h)

add_executable(Compiler_5183 main.cpp tokenCodes.h ${SOURCE_FILES} ${HEADER_FILES} )


#set(llvm_libs support core irreader)
llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(Compiler_5183 ${llvm_libs})