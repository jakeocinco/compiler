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

    llvm::raw_ostream& output = llvm::outs();
    m->print(output, nullptr);
}

Module* code_generation::codegen_program_root(node *n) {

    n->children.pop_front(); // Popping program
    const string program_name = string("program_") + n->children.front()->val.stringValue;
    n->children.pop_front(); // Popping program name
    n->children.pop_front(); // Popping is

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
        codegen_declaration_block(n->children.front(), &builder);
        n->children.pop_front(); // ignoring declaration block for now
        n->children.pop_front(); // Popping begin

        //    codegen_statement_block(n->children.front());
//        node* t = node::create_integer_literal_node(5);
//        Value* v = codegen_literal_integer(t);
//        Value* temp = builder.CreateMul(v, v, "multemp");
//        builder.CreateAlloca(Type::getInt32Ty(context), temp, "a");

        codegen_statement_block(n->children.front(), &builder);
        n->children.pop_front();

//        codegen_print_prototype(context, m, ConstantInt::get(context, APInt(32, 69))
        Value* v = builder.CreateGlobalString("jake");
        codegen_print_prototype(context, m, codegen_literal_integer(69));
        codegen_print_prototype(context, m, codegen_literal_float(69.6));
        codegen_print_prototype(context, m, builder.CreateLoad(identifiers.at("var1")));
        codegen_print_prototype(context, m,  v);

        builder.CreateRet(ConstantInt::get(context, APInt(32,0)));
//        verifyFunction(*f);
    }
    return m;
}

void code_generation::codegen_declaration_block(node *n, IRBuilder<>* b) {
    for (node* x : n->children){
        if (x->type == T_VARIABLE_DECLARATION) codegen_variable_declaration(x, b);
    }
}
void code_generation::codegen_statement_block(node *n, IRBuilder<>* b) {
    for (node* x : n->children){
        if (x->type == T_VARIABLE_ASSIGNMENT) codegen_variable_assignment(x, b);
        if (x->type == T_IF_BLOCK) codegen_if_statement(x);
    }
}

void code_generation::codegen_if_statement(node* n) {
    n->children.pop_front(); // Popping if
    Value* condition = codegen_expression(n->children.front());
    n->children.pop_front(); // Popping expression
    n->children.pop_front(); // Popping then

    Function* function = builder.GetInsertBlock()->getParent();
    BasicBlock* thenBB = BasicBlock::Create(context, "then", function);
    BasicBlock* elseBB = BasicBlock::Create(context, "else");
    BasicBlock* mergeBB = BasicBlock::Create(context, "ifcont");

    builder.CreateCondBr(condition, thenBB, elseBB);
    builder.SetInsertPoint(thenBB);

    codegen_statement_block(n->children.front(), &builder);
    n->children.pop_front();
    n->children.pop_front(); // Popping else if there

    builder.CreateBr(mergeBB);
    thenBB = builder.GetInsertBlock();


    function->getBasicBlockList().push_back(elseBB);
    builder.SetInsertPoint(elseBB);

    codegen_statement_block(n->children.front(), &builder);
    n->children.pop_front();

    builder.CreateBr(mergeBB);
    elseBB = builder.GetInsertBlock();

    function->getBasicBlockList().push_back(mergeBB);
    builder.SetInsertPoint(mergeBB);
//    PHINode* pn = builder.CreatePHI(Type::getDoubleTy(context), 2, "iftmp");

//    pn->addIncoming(thenB)
}

void code_generation::codegen_variable_declaration(node *n, IRBuilder<> *b) {
    n->children.pop_front(); // variable
    string s = n->children.front()->val.stringValue;
    n->children.pop_front(); // variable name
    n->children.pop_front(); // colon
    n->children.pop_front(); // type -- WILL be important
    AllocaInst* alloca = builder.CreateAlloca(Type::getDoubleTy(context), 0, s.c_str());
    identifiers.insert_or_assign(s, alloca);
    //    builder.CreateStore()
}
void code_generation::codegen_variable_assignment(node *n, IRBuilder<>* b) {
    string s = n->children.front()->val.stringValue;
    n->children.pop_front();
    n->children.pop_front();
    Value* v =  codegen_expression(n->children.front());
    b->CreateStore(v, identifiers.at(s));
//    builder.CreateAlloca(Type::getInt32Ty(context), v, s);
}


Function *code_generation::codegen_function(node *n, Module* m) {

    std::vector<std::string> args = {};
    std::string name = "main";

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
//    node* x = node::create_integer_literal_node(42);
    return codegen_literal_integer(42);
}

Value *code_generation::codegen_literal_integer(int n) {
    return ConstantFP::get(context, APFloat(static_cast<double>(n)));
}

Value *code_generation::codegen_literal_float(double n) {
    return ConstantFP::get(context, APFloat(n));
}

Value *code_generation::codegen_expression(node *n,  Value* lhs) {

    if (n->children.empty())
        return lhs;

    bool is_not_l = false;
    if (n->children.front()->type == T_NOT){
        n->children.pop_front();  // Pop not
        is_not_l = true;
    }

    if (n->children.size() == 1)
        return codegen_arith_op(n->children.front()); // flip if is_not_l

    if (lhs == nullptr){
        lhs = codegen_relation(n->children.front());
        n->children.pop_front();
    }

    int operation = n->children.front()->type;
    n->children.pop_front(); // Pop sign

    bool is_not_r = false;
    if (n->children.front()->type == T_NOT){
        n->children.pop_front(); // Pop not
        is_not_r = true;
    }
    Value* rhs = codegen_relation(n->children.front());
    n->children.pop_front(); // Pop RHS

    // check flips
    switch (operation) {
        case T_ADD:
            return codegen_arith_op(n, builder.CreateFAdd(lhs, rhs, "addtmp"));
        case T_MINUS:
            return codegen_arith_op(n, builder.CreateFSub(lhs, rhs, "subtmp"));
        default:
            cout << "Error" << endl;
    }


    return nullptr;
}
Value *code_generation::codegen_arith_op(node *n,  Value* lhs) {

    if (n->children.empty())
        return lhs;
    if (n->children.size() == 1)
        return codegen_relation(n->children.front());

    if (lhs == nullptr){
        lhs = codegen_relation(n->children.front());
        n->children.pop_front();
    }

    int operation = n->children.front()->type;
    n->children.pop_front();
    Value* rhs = codegen_relation(n->children.front());
    n->children.pop_front();

    switch (operation) {
        case T_ADD:
            return codegen_arith_op(n, builder.CreateFAdd(lhs, rhs, "addtmp"));
        case T_MINUS:
            return codegen_arith_op(n, builder.CreateFSub(lhs, rhs, "subtmp"));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_relation(node *n, Value* lhs) {

    if (n->children.empty())
        return lhs;
    if (n->children.size() == 1)
        return codegen_term(n->children.front());

    if (lhs == nullptr){
        lhs = codegen_term(n->children.front());
        n->children.pop_front(); // Pop lhs
    }
    int operation = n->children.front()->type;
    n->children.pop_front(); // Pop operation
    Value* rhs = codegen_term(n->children.front());
    n->children.pop_front(); // Pop rhs
    switch (operation) {
        case T_L_THAN:
            return codegen_term(n, builder.CreateFCmpULT(lhs, rhs, "lthan"));
        case T_LE_THAN:
            return codegen_term(n, builder.CreateFCmpULE(lhs, rhs, "lehan"));
        case T_G_THAN:
            return codegen_term(n, builder.CreateFCmpUGT(lhs, rhs, "gthan"));
        case T_GE_THAN:
            return codegen_term(n, builder.CreateFCmpUGE(lhs, rhs, "gthan"));
        case T_D_EQUALS:
            return codegen_term(n, builder.CreateFCmpUEQ(lhs, rhs, "equals"));
        case T_N_EQUALS:
            return codegen_term(n, builder.CreateFCmpUNE(lhs, rhs, "equals"));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_term(node *n, Value* lhs) {

    if (n->children.empty())
        return lhs;
    if (n->children.size() == 1)
        return codegen_factor(n->children.front());

    if (lhs == nullptr){
        lhs = codegen_factor(n->children.front());
        n->children.pop_front();
    }

    int operation = n->children.front()->type;
    n->children.pop_front();
    Value* rhs = codegen_factor(n->children.front());
    n->children.pop_front();

    switch (operation) {
        case T_MULTIPLY:
            return codegen_term(n, builder.CreateFMul(lhs, rhs, "multmp"));
        case T_DIVIDE:
            return codegen_term(n, builder.CreateFDiv(lhs, rhs, "divtmp"));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_factor(node *n) {
    node* x = n->children.front();
    if (x->type == T_INTEGER_LITERAL)
        return codegen_literal_integer(x->val.intValue);
    if (x->type == T_FLOAT_LITERAL)
        return codegen_literal_float(x->val.doubleValue);
    if (x->type == T_IDENTIFIER)
        return builder.CreateLoad(identifiers.at(x->val.stringValue));
    if (x->type == T_FALSE)
        return builder.CreateFCmpOEQ(codegen_literal_integer(0), codegen_literal_integer(1), "false");
    if (x->type == T_TRUE){
        return builder.CreateFCmpOEQ(codegen_literal_integer(0), codegen_literal_integer(0), "true");
    }
    return nullptr;
}

Function* code_generation::codegen_print_prototype(LLVMContext &ctx, Module *mod, Value* v) {


    Function* printer = mod->getFunction("printf");
    if (printer == nullptr){
        std::vector<Type *> args;
        args.push_back(Type::getInt8PtrTy(context));

        FunctionType *printfType = FunctionType::get(builder.getInt32Ty(), args, true);
        Function::Create(printfType, Function::ExternalLinkage, "printf",
                         mod);
    }


    std::vector<Value *> printArgs;
    Value *formatStr = builder.CreateGlobalStringPtr("%f\n");
    printArgs.push_back(formatStr);

    /*We will be printing "20"*/
//
//    printArgs.push_back(v);
    printArgs.push_back(v);
    builder.CreateCall(mod->getFunction("printf"), printArgs);

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









