//
// Created by Jacob Carlson on 4/6/21.
//

#ifndef COMPILER_5183_CODE_GENERATION_H
#define COMPILER_5183_CODE_GENERATION_H

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "ExprAst.h"
#include "../Parser/node.h"
#include <map>
#include <string>
#include <vector>

using namespace llvm;

static LLVMContext context;
static IRBuilder<> builder(context);
static std::unique_ptr<Module> module;
static std::map<std::string, Value *> namedValues;

class code_generation {
public:
    code_generation(std::string file_text);
private:

    node* tree;

    Value* codegen(node* n);

    Function* codegen_program_root(node* n);

    Function* codegen_function(node* n);
    Value* codegen_function_body(node* n);

    Value* codegen_literal_integer(node* n);
    Value* codegen_literal_float(node* n);
    Value* codegen_literal_boolean(node* n);

    Value* codegen_addition(node* n);
};



//class ExprAST {
//public:
//    virtual ~ExprAST() {}
////    virtual Value *codegen() = 0;
//};
//
///// NumberExprAST - Expression class for numeric literals like "1.0".
//class NumberExprAST : public ExprAST {
//    double Val;
//
//public:
//    NumberExprAST(double Val) : Val(Val) {}
//    virtual Value *codegen();
//};
#endif //COMPILER_5183_CODE_GENERATION_H
