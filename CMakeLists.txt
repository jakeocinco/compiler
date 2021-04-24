cmake_minimum_required(VERSION 3.10)
project(Compiler_5183)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES Scanner/scanner.cpp compiler.cpp Parser/node.cpp Parser/parser.cpp Parser/symbol_table.cpp  CodeGeneration/code_generation.cpp CodeGeneration/variable_inst.cpp CodeGeneration/scope.cpp)
set(HEADER_FILES Scanner/scanner.h compiler.h Parser/node.h Parser/parser.h Parser/symbol_table.h CodeGeneration/code_generation.h CodeGeneration/variable_inst.h CodeGeneration/scope.h)

add_executable(Compiler_5183 main.cpp tokenCodes.h ${SOURCE_FILES} ${HEADER_FILES} )

llvm_map_components_to_libnames(llvm_libs irreader target core executionengine interpreter mc support nativecodegen)

foreach(target ${LLVM_TARGETS_TO_BUILD})
    list(APPEND targets "LLVM${target}CodeGen")
endforeach()
target_link_libraries(Compiler_5183 ${llvm_libs} ${targets})