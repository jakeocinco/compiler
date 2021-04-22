//
// Created by Jacob Carlson on 4/21/21.
//

#include "scope.h"

scope::scope(scope* parent) {
    this->parent = parent;
    this->builder = parent->builder;
    this->module = parent->module;
}
scope::scope(llvm::IRBuilder<>* builder, llvm::Module* module) {
    this->builder = builder;
    this->module = module;
}

variable_inst *scope::get_temp(std::string s) {
    if (this->table.contains(s))
        return this->table.find(s)->second;
    // TODO - Change this to only check global
    if (this->parent != nullptr)
        return this->parent->get_temp(s);
    return nullptr;
}

void scope::add(std::string s, llvm::Type* type, variable_inst::VARIABLE_CLASS clazz, int size) {
    variable_inst* temp = new variable_inst(builder,module, type, clazz, size);
    this->table.insert_or_assign(s, temp);
    if (size > 1){
//        llvm::AllocaInst* a = builder->CreateAlloca(llvm::Type::getInt32Ty(module->getContext()), nullptr, s + "_size");
        llvm::Value* tempSize = llvm::ConstantInt::get(llvm::Type::getInt32Ty(module->getContext()), llvm::APInt(32, size));
//        builder->CreateStore(tempSize, a);
        auto size = new variable_inst(builder,module, tempSize,llvm::Type::getInt32Ty(module->getContext()), variable_inst::VALUE, 1);
        this->table.insert_or_assign(s + "_size", size);
    }
}

void scope::add(std::string s, llvm::Value *value, llvm::Type *type, variable_inst::VARIABLE_CLASS clazz, int size) {
    variable_inst* vi = new variable_inst(builder,module, value, type, clazz, size);
    this->table.insert_or_assign(s, vi);
    if (size > 1){
//        llvm::AllocaInst* a = builder->CreateAlloca(llvm::Type::getInt32Ty(module->getContext()), nullptr, s + "_size");
        llvm::Value* tempSize = llvm::ConstantInt::get(llvm::Type::getInt32Ty(module->getContext()), llvm::APInt(32, size));
//        builder->CreateStore(tempSize, a);
        auto size = new variable_inst(builder,module, tempSize,llvm::Type::getInt32Ty(module->getContext()), variable_inst::VALUE, 1);
        this->table.insert_or_assign(s + "_size", size);
    }
}

void scope::set(std::string s, llvm::Value *v, int size, llvm::Value *index) {
    variable_inst* vi = get_temp(s);
    vi->set(v,index, size);
}

scope* scope::get_parent(){
    return parent;
}



