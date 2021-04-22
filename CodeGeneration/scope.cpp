//
// Created by Jacob Carlson on 4/21/21.
//

#include "scope.h"

scope::scope(scope* parent) {
    this->parent = parent;
    this->builder = parent->builder;
    this->context = parent->context;
}
scope::scope(llvm::IRBuilder<>* builder, llvm::LLVMContext* context) {
    this->builder = builder;
    this->context = context;
}

variable_inst *scope::get_temp(std::string s) {
    if (this->table.contains(s))
        return this->table.find(s)->second;
    // TODO - Change this to only check global
    if (this->parent != nullptr)
        return this->parent->get_temp(s);
    return nullptr;
}

void scope::add(std::string s, llvm::Type* type, variable_inst::VARIABLE_CLASS clazz) {
    variable_inst* temp = new variable_inst(builder,context, type, clazz);
    this->table.insert_or_assign(s, temp);
}

void scope::add(std::string s, llvm::Value *value, llvm::Type *type, variable_inst::VARIABLE_CLASS clazz) {
    variable_inst* vi = new variable_inst(builder,context, value, type, clazz);
//    vi->realloca(value);
    this->table.insert_or_assign(s, vi);
}

void scope::set(std::string s, llvm::Value *v, llvm::Value *index) {
    variable_inst* vi = get_temp(s);
    vi->set(v,index);
}

scope* scope::get_parent(){
    return parent;
}



