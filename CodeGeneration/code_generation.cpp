//
// Created by Jacob Carlson on 4/6/21.
//

#include "code_generation.h"
#include "../Parser/parser.h"
#include "../tokenCodes.h"

//Value *NumberExprAST::codegen() {
//    return ConstantFP::get(TheContext, APFloat(Val));
//}
code_generation::code_generation(std::string file_text) {
    auto p = parser(file_text);
    this->tree = p.get_head();

    Module* m;

    if (this->tree->type == T_PROGRAM_ROOT)
        codegen_program_root(this->tree);
    else
        std::cout << "Not program root" << endl;

}

Value *code_generation::codegen(node* n) {
    if (n->type == T_INTEGER_LITERAL) return codegen_literal_integer(n);
    if (n->type == T_FLOAT_LITERAL) return codegen_literal_float(n);
//    if (n->type == T)
    return nullptr;
}

Function* code_generation::codegen_program_root(node *n) {

//    Module* m = new Module("Hello", context);

    Function* f = module->getFunction("mul_add");
    if (!f)
        f = codegen_function(n);

    BasicBlock *BB = BasicBlock::Create(context, "entry", f);
    builder.SetInsertPoint(BB);

    namedValues.clear();
    for (auto &Arg : f->args())
        namedValues.insert_or_assign(Arg.getName().str(), &Arg);

    if (Value* returnVal = codegen_function_body(nullptr)){
        builder.CreateRet(returnVal);
        verifyFunction(*f);
        return f;
    }
    return nullptr;
}

Function *code_generation::codegen_function(node *n) {

    std::vector<std::string> args = {"x","y","z"};
    std::string name = "Jake";

    std::vector<Type*> Doubles(args.size(),
                               Type::getDoubleTy(context));
    FunctionType *FT =
            FunctionType::get(Type::getDoubleTy(context), Doubles, false);

    Function *F =
            Function::Create(FT, Function::ExternalLinkage, name, module.get());

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(args[Idx++]);

    return F;
}

Value *code_generation::codegen_function_body(node *n) {
    return nullptr;
}

Value *code_generation::codegen_literal_integer(node *n) {
//    return ConstantInt::get(context, APInt(4 * 8,n->val.intValue));
    return nullptr;
}

Value *code_generation::codegen_literal_float(node *n) {
//    return ConstantFP::get(context, APFloat(n->val.doubleValue));
    return nullptr;
}

Value *code_generation::codegen_literal_boolean(node *n) {

    return nullptr;
}

Value *code_generation::codegen_addition(node *n) {
//    Value* L = codegen(n->children.at(0));
//    Value* R = codegen(n->children.at(2));

//    if (!L || !R)
//        return nullptr;

//    return Builder.CreateFAdd(L,R);
    return nullptr;
}







