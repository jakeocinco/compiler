//
// Created by Jacob Carlson on 4/21/21.
//

#include "variable_inst.h"

variable_inst::variable_inst(llvm::IRBuilder<>* builder, llvm::Type *type, bool is_allocated) {
    this->is_allocated = is_allocated;
    this->type = type;
    this->val = nullptr;
    this->b = builder;
}

variable_inst::variable_inst(llvm::IRBuilder<> *builder, llvm::Value *value, llvm::Type *type, bool is_allocated) {
    this->is_allocated = is_allocated;
    this->type = type;
    this->val = value;
    this->b = builder;
}

llvm::Value* variable_inst::get() {
    if (is_allocated)
        return b->CreateLoad(val);
    else
        return val;
}

void variable_inst::set(llvm::Value *val) {
    if (is_allocated){
        b->CreateStore(val, this->val);
    } else {
        this->val = val;
    }
}

void variable_inst::realloca(llvm::Value *val) {
    this->val = val;
}

variable_inst::variable_inst() {
    this->is_allocated = false;
    this->type = nullptr;
    this->val = nullptr;
    this->b = nullptr;
}


variable_inst::variable_inst(variable_inst const &v) {
    this->is_allocated = v.is_allocated;
    this->type = v.type;
    this->val = v.val;
    this->b = v.b;
}






