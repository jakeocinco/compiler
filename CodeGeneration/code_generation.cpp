//
// Created by Jacob Carlson on 4/6/21.
//

#include <sstream>
#include "code_generation.h"
#include "../Parser/parser.h"
#include "../tokenCodes.h"

//Value *NumberExprAST::codegen() {
//    return ConstantFP::get(TheContext, APFloat(Val));
//}
code_generation::code_generation(std::string file_text) {
    auto p = parser(file_text);
    this->tree = p.get_head();


    Module* m = nullptr;
    if (this->tree->type == T_PROGRAM_ROOT)
        m = codegen_program_root(this->tree);
    else
        std::cout << "Not program root" << endl;

    m->print(llvm::outs(), nullptr);
}


Value *code_generation::codegen(node* n) {
    if (n->type == T_INTEGER_LITERAL) return codegen_literal_integer(n);
    if (n->type == T_FLOAT_LITERAL) return codegen_literal_float(n);
//    if (n->type == T_MULTIPLY) return codegen_multiply(n);
//    if (n->type == T_EXPRESSION) return codegen_multiply(n);
    return nullptr;
}

Module* code_generation::codegen_program_root(node *n) {

    n->children.pop_front(); // Popping program
    const string program_name = string("program_") + n->children.front()->val.stringValue;
    n->children.pop_front(); // Popping program name
    n->children.pop_front(); // Popping is

    n->children.pop_front(); // ignoring declaration block for now
    n->children.pop_front(); // Popping begin

    auto* m = new Module(program_name, context);

    Function* f = m->getFunction("mul");
    if (!f)
        f = codegen_function(n,m);

    BasicBlock *BB = BasicBlock::Create(context, "entry", f);
    builder.SetInsertPoint(BB);

    namedValues.clear();
    for (auto &Arg : f->args())
        namedValues.insert_or_assign(Arg.getName().str(), &Arg);

    if (Value* returnVal = codegen_function_body(nullptr)){
        //    codegen_statement_block(n->children.front());
//        node* t = node::create_integer_literal_node(5);
//        Value* v = codegen_literal_integer(t);
//        Value* temp = builder.CreateMul(v, v, "multemp");
//        builder.CreateAlloca(Type::getInt32Ty(context), temp, "a");
            codegen_statement_block(n->children.front(), &builder);
//        n->children.pop_front();
        builder.CreateRet(returnVal);
//        verifyFunction(*f);
    }
    return m;
}

void code_generation::codegen_statement_block(node *n, IRBuilder<>* b) {
    for (node* x : n->children){
        if (x->type == T_VARIABLE_ASSIGNMENT) codegen_variable_assignment(x, b);
    }
}

void code_generation::codegen_variable_assignment(node *n, IRBuilder<>* b) {
    string s = n->children.front()->val.stringValue;
    n->children.pop_front();
    n->children.pop_front();
    Value* v =  codegen_expression(n->children.front());
    builder.CreateAlloca(Type::getInt32Ty(context), v, s);
}


Function *code_generation::codegen_function(node *n, Module* m) {

    std::vector<std::string> args = {"x","y","z"};
    std::string name = "Jake";

    std::vector<Type*> Integers(args.size(),
                               Type::getDoubleTy(context));
    FunctionType *FT =
            FunctionType::get(Type::getInt32Ty(context), Integers, false);

    Function *F =
            Function::Create(FT, Function::ExternalLinkage, name, m);

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(args[Idx++]);

    return F;
}

Value *code_generation::codegen_function_body(node *n) {
    node* x = node::create_integer_literal_node(42);
    return codegen_literal_integer(x);
}

Value *code_generation::codegen_literal_integer(node *n) {
    return ConstantInt::get(context, APInt(4 * 8,n->val.intValue));
}

Value *code_generation::codegen_literal_float(node *n) {
    return ConstantFP::get(context, APFloat(n->val.doubleValue));
}

Value *code_generation::codegen_literal_boolean(node *n) {
    return nullptr;
}

Value *code_generation::codegen_expression(node *n) {
    Value* v =  codegen_arith_op(n->children.front());
    return v;
}

Value *code_generation::codegen_arith_op(node *n,  Value* n2) {

    if (n->children.size() == 1)
        return codegen_relation(n->children.front());

    Value* lhs = nullptr;
    if (n2 != nullptr){
        lhs = n2;
    } else {
        lhs = codegen_relation(n->children.front());
        n->children.pop_front();
    }

    int operation = n->children.front()->type;
    n->children.pop_front();
    Value* rhs = codegen_relation(n->children.front());
    n->children.pop_front();

    switch (operation) {
        case T_ADD:
            if (n->children.empty())
                return builder.CreateFAdd(lhs, rhs, "addtmp");
            else
                return codegen_arith_op(n, builder.CreateFAdd(lhs, rhs, "addtmp"));
        case T_MINUS:
            if (n->children.empty())
                return builder.CreateFSub(lhs, rhs, "addtmp");
            else
                return codegen_arith_op(n, builder.CreateFSub(lhs, rhs, "addtmp"));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_relation(node *n) {
    return codegen_term(n->children.front());
}
Value *code_generation::codegen_term(node *n, Value* n2) {
    if (n->children.size() == 1)
        return codegen_factor(n->children.front());

    Value* lhs = nullptr;
    if (n2 != nullptr){
        lhs = n2;
    } else {
        lhs = codegen_factor(n->children.front());
        n->children.pop_front();
    }

    int operation = n->children.front()->type;
    n->children.pop_front();


    Value* rhs = codegen_factor(n->children.front());
    n->children.pop_front();

    switch (operation) {
        case T_MULTIPLY:
            if (n->children.empty())
                return builder.CreateFMul(lhs, rhs, "multmp");
            else
                return codegen_term(n, builder.CreateFMul(lhs, rhs, "multmp"));
        case T_DIVIDE:
            if (n->children.empty())
                return builder.CreateFDiv(lhs, rhs, "divtmp");
            else
                return codegen_term(n, builder.CreateFDiv(lhs, rhs, "divtmp"));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_factor(node *n) {
    node* x = n->children.front();
    if (x->type == T_INTEGER_LITERAL)
        return codegen_literal_integer(x);
    if (x->type == T_FLOAT_LITERAL)
        return codegen_literal_float(x);
    if (x->type == T_IDENTIFIER)
        return codegen_literal_integer(node::create_integer_literal_node(2));
    return nullptr;
}




//void code_generation::write_to_file(Module* m) {
//////    InitializeModuleAndPassManager();
//    // Initialize the target registry etc.
////    InitializeAllTargetInfos();
////    InitializeAllTargets();
////    InitializeAllTargetMCs();
////    InitializeAllAsmParsers();
////    InitializeAllAsmPrinters();
//
//    auto TargetTriple = sys::getDefaultTargetTriple();
////    m->setTargetTriple(TargetTriple);
//
////    std::string Error;
////    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
//
//    // Print an error and exit if we couldn't find the requested target.
//    // This generally occurs if we've forgotten to initialise the
//    // TargetRegistry or we have a bogus target triple.
////    if (!Target) {
////        errs() << Error;
////        return;
////    }
//
//    auto CPU = "generic";
//    auto Features = "";
//
////    TargetOptions opt;
//    auto RM = Optional<Reloc::Model>();
////    auto TheTargetMachine =Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
//
////    m->setDataLayout(TheTargetMachine->createDataLayout());
//
//    auto Filename = "output.o";
//    std::error_code EC;
//    raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);
//
//    if (EC) {
//        errs() << "Could not open file: " << EC.message();
//        return;
//    }
//
//    legacy::PassManager pass;
//    auto FileType = CGFT_ObjectFile;
//
////    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
////        errs() << "TargetMachine can't emit a file of this type";
////        return;
////    }
//
//    pass.run(*m);
//    dest.flush();
//}









