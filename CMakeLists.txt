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
set(HEADER_FILES Scanner/scanner.h compiler.h Parser/node.h Parser/parser.h Parser/symbol_table.h CodeGeneration/code_generation.h CodeGeneration/ExprAst.h GeneratorLLVM.cpp GeneratorLLVM.h)

add_executable(Compiler_5183 main.cpp tokenCodes.h ${SOURCE_FILES} ${HEADER_FILES} )


#set(llvm_libs support core irreader target)
#llvm_map_components_to_libnames(llvm_libs aarch64asmparser aarch64codegen aarch64desc aarch64disassembler aarch64info aarch64utils aggressiveinstcombine amdgpuasmparser amdgpucodegen amdgpudesc amdgpudisassembler amdgpuinfo amdgpuutils analysis armasmparser armcodegen armdesc armdisassembler arminfo armutils asmparser asmprinter avrasmparser avrcodegen avrdesc avrdisassembler avrinfo binaryformat bitreader bitstreamreader bitwriter bpfasmparser bpfcodegen bpfdesc bpfdisassembler bpfinfo cfguard codegen core coroutines coverage debuginfocodeview debuginfodwarf debuginfogsym debuginfomsf debuginfopdb demangle dlltooldriver dwarflinker engine executionengine extensions frontendopenmp fuzzmutate globalisel hexagonasmparser hexagoncodegen hexagondesc hexagondisassembler hexagoninfo instcombine instrumentation interpreter ipo irreader lanaiasmparser lanaicodegen lanaidesc lanaidisassembler lanaiinfo libdriver lineeditor linker lto mc mca mcdisassembler mcparser mipsasmparser mipscodegen mipsdesc mipsdisassembler mipsinfo mirparser msp430asmparser msp430codegen msp430desc msp430disassembler msp430info native nativecodegen nvptxcodegen nvptxdesc nvptxinfo objcarcopts object objectyaml option orcerror passes powerpcasmparser powerpccodegen powerpcdesc powerpcdisassembler powerpcinfo profiledata remarks riscvasmparser riscvcodegen riscvdesc riscvdisassembler riscvinfo riscvutils runtimedyld scalaropts selectiondag sparcasmparser sparccodegen sparcdesc sparcdisassembler sparcinfo support symbolize systemzasmparser systemzcodegen systemzdesc systemzdisassembler systemzinfo tablegen target textapi transformutils vectorize webassemblyasmparser webassemblycodegen webassemblydesc webassemblydisassembler webassemblyinfo windowsmanifest x86asmparser x86codegen x86desc x86disassembler x86info xcorecodegen xcoredesc xcoredisassembler xcoreinfo xray)
llvm_map_components_to_libnames(llvm_libs irreader target core executionengine interpreter mc support nativecodegen)
target_link_libraries(Compiler_5183 ${llvm_libs})