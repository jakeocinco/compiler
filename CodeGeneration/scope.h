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
public:
    scope(scope* parent = nullptr);

    void add(std::string s, llvm::Type* type, llvm::IRBuilder<>* builder, bool is_allocated = false);
    void add(std::string s, llvm::Value* value, llvm::Type* type, llvm::IRBuilder<>* builder, bool is_allocated = false);

    variable_inst* get_temp(std::string s);
    void set(std::string s, llvm::Value* v);
};


#endif //COMPILER_5183_SCOPE_H
