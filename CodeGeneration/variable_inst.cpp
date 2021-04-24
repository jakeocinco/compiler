//
// Created by Jacob Carlson on 4/21/21.
//

#include "variable_inst.h"

variable_inst::variable_inst(llvm::IRBuilder<>* builder,  llvm::Module* module, llvm::Type *type, VARIABLE_CLASS clazz, int size) {

    variable_inst(builder, module, nullptr, type, clazz, size);
}

variable_inst::variable_inst( llvm::IRBuilder<> *builder,  llvm::Module* module, llvm::Value *value, llvm::Type *type,  VARIABLE_CLASS clazz, int size) {
    this->clazz = clazz;
    this->type = type;
    this->val = value;
    this->b = builder;
    this->m = module;
    this->size = size;

}

llvm::Value* variable_inst::get(llvm::Value* index) {
    if (clazz == VARIABLE_CLASS::INSTANCE)
        return b->CreateLoad(val);
    else if (clazz == VARIABLE_CLASS::ARRAY_INSTANCE && index != nullptr) {


        llvm::Value *array_inst = val;
        llvm::Value *i32zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m->getContext()), llvm::APInt(8, 0));
        llvm::Value *indices[2] = {i32zero, index};
        auto varInst = b->CreateInBoundsGEP(array_inst, llvm::ArrayRef<llvm::Value *>(indices, 2));
        return b->CreateLoad(varInst);
    } else if (clazz == VARIABLE_CLASS::ARRAY_INSTANCE && index == nullptr) {
            return val;
    } else
        return val;
}

void variable_inst::set(llvm::Value *val, llvm::Value *index, int new_val_size) {

    if (type != val->getType()){
        if (type == llvm::Type::getDoubleTy(m->getContext()) && val->getType() == llvm::Type::getInt32Ty(m->getContext()))
            val = b->CreateSIToFP(val, type);
        if (type == llvm::Type::getInt1Ty(m->getContext()) && val->getType() == llvm::Type::getInt32Ty(m->getContext()))
            val = b->CreateCall(m->getFunction("castIntToBool"), {val});
    }

    if (clazz == VARIABLE_CLASS::INSTANCE){
        b->CreateStore(val, this->val);
    } else if (clazz == VARIABLE_CLASS::ARRAY_INSTANCE){
        int temp_size = size;
        llvm::Value* temp_index = nullptr;
        if (index != nullptr){
            temp_size = 1;
            temp_index = index;
        }
        if (new_val_size == 1){
            for (int i = 0; i < temp_size; i++){
                if (index == nullptr){
                    temp_index = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, i));
                }
                llvm::Value *i32zero = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, 0));
                llvm::Value *indices[2] = {i32zero, temp_index};

                llvm::Value* varInst = b->CreateInBoundsGEP(this->val, llvm::ArrayRef<llvm::Value *>(indices, 2));
                b->CreateStore(val, varInst);
            }
        } else {
            for (int i = 0; i < temp_size; i++){
                if (index == nullptr){
                    temp_index = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, i));
                }
                llvm::Value *i32zero = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, 0));
                llvm::Value *indices[2] = {i32zero, temp_index};

                llvm::Value* varInst = b->CreateInBoundsGEP(this->val, llvm::ArrayRef<llvm::Value *>(indices, 2));

//                llvm::Value *i32zero = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, 0));
//                llvm::Value *indices[2] = {i32zero, temp_index};

                llvm::Value* newVarInst = b->CreateInBoundsGEP(val, llvm::ArrayRef<llvm::Value *>(indices, 2));

                b->CreateStore(b->CreateLoad(newVarInst), varInst);
            }
        }

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

int variable_inst::get_size() {
    return size;
}






