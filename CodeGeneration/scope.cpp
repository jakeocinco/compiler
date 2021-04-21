//
// Created by Jacob Carlson on 4/21/21.
//

#include "scope.h"

scope::scope(scope* parent) {
    this->parent = parent;
}

variable_inst *scope::get_temp(std::string s) {
    if (this->table.contains(s))
        return this->table.find(s)->second;
    // TODO - Change this to only check global
    if (this->parent != nullptr)
        return this->parent->get_temp(s);
    return nullptr;
}

void scope::add(std::string s, llvm::Type* type, llvm::IRBuilder<>* builder, bool is_allocated) {
    variable_inst* temp = new variable_inst(builder, type, is_allocated);
    this->table.insert_or_assign(s, temp);
}

void scope::add(std::string s, llvm::Value *value, llvm::Type *type, llvm::IRBuilder<> *builder, bool is_allocated) {
    variable_inst* vi = new variable_inst(builder, value, type, is_allocated);
//    vi->realloca(value);
    this->table.insert_or_assign(s, vi);
}

void scope::set(std::string s, llvm::Value *v) {
    variable_inst* vi = get_temp(s);
    vi->set(v);
}

scope* scope::get_parent(){
    return parent;
}



