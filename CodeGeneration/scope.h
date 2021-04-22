//
// Created by Jacob Carlson on 4/21/21.
//

#ifndef COMPILER_5183_SCOPE_H
#define COMPILER_5183_SCOPE_H


//#include <cstdio>
#include <string>
#include <map>
#include "variable_inst.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

class scope {
private:
    scope* parent;
    std::string s;
    std::map<std::string,variable_inst*> table;

    llvm::IRBuilder<>* builder;
    llvm::Module* module;

public:
    scope(scope* parent);
    scope(llvm::IRBuilder<>* builder, llvm::Module* module);

    void add(std::string s, llvm::Type* type, variable_inst::VARIABLE_CLASS clazz, int size = 1);
    void add(std::string s, llvm::Value* value, llvm::Type* type, variable_inst::VARIABLE_CLASS clazz, int size = 1);

    variable_inst* get_temp(std::string s);
    void set(std::string s, llvm::Value* v, int size, llvm::Value *index = nullptr);

    scope* get_parent();
};


#endif //COMPILER_5183_SCOPE_H
