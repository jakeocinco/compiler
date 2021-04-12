//
// Created by Jacob Carlson on 4/6/21.
//

#ifndef COMPILER_5183_CODE_GENERATION_H
#define COMPILER_5183_CODE_GENERATION_H

//#include "llvm/Pass.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
//#include "llvm/IR/PassManager.h"
//#include "llvm/IR/CallingConv.h"
//#include "llvm/IR/Verifier.h"
//#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
//#include "llvm/Bitcode/BitcodeWriter.h"
//#include "llvm/ADT/APFloat.h"
//#include "llvm/ADT/Optional.h"
//#include "llvm/ADT/STLExtras.h"
//#include "llvm/IR/BasicBlock.h"
//#include "llvm/IR/Constants.h"
//#include "llvm/IR/DerivedTypes.h"
//#include "llvm/IR/Function.h"
//#include "llvm/IR/Instructions.h"
//#include "llvm/IR/IRBuilder.h"
//#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/LegacyPassManager.h"
//#include "llvm/IR/Module.h"
//#include "llvm/IR/Type.h"
//#include "llvm/IR/Verifier.h"
//#include "llvm/Support/FileSystem.h"
//#include "llvm/Support/Host.h"
//#include "llvm/Support/raw_ostream.h"
//#include "llvm/Support/TargetRegistry.h"
//#include "llvm/Support/TargetSelect.h"
//#include "llvm/Target/TargetMachine.h"
//#include "llvm/Target/TargetOptions.h"
#include "../Parser/node.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace llvm;
using namespace llvm::sys;

static LLVMContext context;
static IRBuilder<> builder(context);
static std::unique_ptr<Module> module;
static std::map<std::string, Value *> namedValues;

class code_generation {
public:
    code_generation(std::string file_text);
private:

    void write_to_file(Module* m);

    node* tree;
    std::map<std::string, AllocaInst*> identifiers;

    IRBuilder<>* b2;
    Value* codegen(node* n);

    Module* codegen_program_root(node* n);
    void codegen_declaration_block(node* n, IRBuilder<>* b);
    void codegen_statement_block(node* n, IRBuilder<>* b);

    Function* codegen_function(node* n, Module* m);
    Value* codegen_function_body(node* n);

    void codegen_variable_declaration(node* n, IRBuilder<>* b);
    void codegen_variable_assignment(node* n, IRBuilder<>* b);

    void codegen_if_statement(node* n);

    Value* codegen_literal_integer(int n);
    Value* codegen_literal_float(double n);
    Value* codegen_multiply(node* n);
    Value* codegen_literal_boolean(node* n);

    Value* codegen_expression(node *n, Value* lhs = nullptr);
    Value* codegen_arith_op(node *n, Value* lhs = nullptr);
    Value* codegen_relation(node *n, Value* lhs = nullptr);
    Value* codegen_term(node *n, Value* lhs = nullptr);
    Value* codegen_factor(node *n);

    Function* codegen_print_prototype(LLVMContext &ctx, Module *mod, Value *v);
};


#endif //COMPILER_5183_CODE_GENERATION_H
