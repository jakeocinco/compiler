//
// Created by Jacob Carlson on 4/21/21.
//

#include "variable_inst.h"

variable_inst::variable_inst(llvm::IRBuilder<>* builder,  llvm::LLVMContext* context, llvm::Type *type, VARIABLE_CLASS clazz) {
    this->clazz = clazz;
    this->type = type;
    this->val = nullptr;
    this->b = builder;
    this->c = context;
}

variable_inst::variable_inst(llvm::IRBuilder<> *builder,  llvm::LLVMContext* context, llvm::Value *value, llvm::Type *type,  VARIABLE_CLASS clazz) {
    this->clazz = clazz;
    this->type = type;
    this->val = value;
    this->b = builder;
    this->c = context;
}

llvm::Value* variable_inst::get(llvm::Value* index) {
    if (clazz == VARIABLE_CLASS::INSTANCE)
        return b->CreateLoad(val);
    else if (clazz == VARIABLE_CLASS::ARRAY_INSTANCE) {
        llvm::Value *array_inst = val;
        llvm::Value *i32zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*c), llvm::APInt(8, 0));
        llvm::Value *indices[2] = {i32zero, index};
        auto varInst = b->CreateGEP(array_inst, llvm::ArrayRef<llvm::Value *>(indices, 2));
        return b->CreateLoad(varInst);
    }
    else
        return val;
}

void variable_inst::set(llvm::Value *val, llvm::Value *index) {
    if (clazz == VARIABLE_CLASS::INSTANCE){
        b->CreateStore(val, this->val);
    } else if (clazz == VARIABLE_CLASS::ARRAY_INSTANCE){
        llvm::Value *i32zero = llvm::ConstantInt::get(*c, llvm::APInt(8, 0));
        llvm::Value *indices[2] = {i32zero, index};

        llvm::Value* varInst = b->CreateGEP(this->val, llvm::ArrayRef<llvm::Value *>(indices, 2));
        b->CreateStore(val, varInst);
    }  else {
        this->val = val;
    }
}

void variable_inst::realloca(llvm::Value *val) {
    this->val = val;
}

variable_inst::variable_inst() {
    this->clazz = VARIABLE_CLASS::VALUE;
    this->type = nullptr;
    this->val = nullptr;
    this->b = nullptr;
}


variable_inst::variable_inst(variable_inst const &v) {
    this->clazz = v.clazz;
    this->type = v.type;
    this->val = v.val;
    this->b = v.b;
}






