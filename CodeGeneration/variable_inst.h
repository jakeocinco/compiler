//
// Created by Jacob Carlson on 4/21/21.
//

#ifndef COMPILER_5183_VARIABLE_INST_H
#define COMPILER_5183_VARIABLE_INST_H

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

class variable_inst {
public:
    enum VARIABLE_CLASS {INSTANCE, VALUE, FUNCTION, ARRAY_INSTANCE};

private:
    VARIABLE_CLASS clazz;
    llvm::Value* val;
    llvm::Type* type;
    int size;

    llvm::IRBuilder<>* b;
    llvm::Module* m;
public:

    variable_inst();
//    ~variable_inst();
    variable_inst(variable_inst const &v);
    variable_inst(llvm::IRBuilder<>* builder, llvm::Module* module, llvm::Type* type, VARIABLE_CLASS clazz, int size);
    variable_inst(llvm::IRBuilder<>* builder,  llvm::Module* module, llvm::Value *value, llvm::Type* type, VARIABLE_CLASS clazz, int size);

    llvm::Value* get(llvm::Value* index = nullptr);
    void set(llvm::Value* val, llvm::Value *index, int new_val_size = 1);
    void realloca(llvm::Value* val);

    int get_size();
    variable_inst operator=(const variable_inst& v);
};


#endif //COMPILER_5183_VARIABLE_INST_H
