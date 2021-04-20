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

    m = new Module("Program", context);

    auto TargetTriple = sys::getDefaultTargetTriple();

    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
//    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    m->setDataLayout(TargetMachine->createDataLayout());
    m->setTargetTriple(TargetTriple);


    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        errs() << Error;
        cout << "ERROR";
    }

    IRBuilder<> temp = IRBuilder<>(context);
    builder = &(temp);

    if (this->tree->type == T_PROGRAM_ROOT)
        codegen_program_root(this->tree);
    else
        std::cout << "Not program root" << endl;

    if (true){
        llvm::raw_ostream& output = llvm::outs();
        this->m->print(output, nullptr);
    }

    auto Filename = "output.o";
    std::error_code EC;
    raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

    if (EC) {
        errs() << "Could not open file: " << EC.message();;
    }

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        errs() << "TargetMachine can't emit a file of this type";
    }

    pass.run(*m);
    dest.flush();
}

Module* code_generation::codegen_program_root(node *n) {

    n->children.pop_front(); // Popping program
    const string program_name = string("program_") + n->children.front()->val.stringValue;
    n->children.pop_front(); // Popping program name
    n->children.pop_front(); // Popping is


    Function* f = m->getFunction("mul");
    if (!f)
        f = codegen_function(n,m);

    BasicBlock *BB = BasicBlock::Create(context, "entry", f);
    builder->SetInsertPoint(BB);
    block = BB;

    namedValues.clear();
    for (auto &Arg : f->args())
        namedValues.insert_or_assign(Arg.getName().str(), &Arg);


    if (true){
        codegen_print_prototype(m); // Move this inside
        codegen_scan_prototype();
        codegen_declaration_block(n->children.front(), builder);


        n->children.pop_front(); // ignoring declaration block for now
        n->children.pop_front(); // Popping begin

        codegen_statement_block(n->children.front(), builder);
        n->children.pop_front();

//        identifiers.insert_or_assign("words", alloca);
        builder->CreateRet(ConstantInt::get(context, APInt(32,0)));
//        verifyFunction(*f);
    }
    return m;
}

void code_generation::codegen_declaration_block(node *n, IRBuilder<>* b) {
    for (node* x : n->children){
        if (x->type == T_VARIABLE_DECLARATION) codegen_variable_declaration(x, b);
        if (x->type == T_PROCEDURE_DECLARATION) codegen_function_body(x);
    }
}
void code_generation::codegen_statement_block(node *n, IRBuilder<>* b) {
    for (node* x : n->children){
        if (x->type == T_VARIABLE_ASSIGNMENT) codegen_variable_assignment(x, b);
        else if (x->type == T_IF_BLOCK) codegen_if_statement(x);
        else if (x->type == T_FOR_LOOP) codegen_for_statement(x);
        else if (x->type == T_RETURN_BLOCK) codegen_return_statement(x);
    }
}

void code_generation::codegen_if_statement(node* n) {
    n->children.pop_front(); // Popping if
    Value* condition = codegen_expression(n->children.front());
    n->children.pop_front(); // Popping expression
    n->children.pop_front(); // Popping then

    Function* function = builder->GetInsertBlock()->getParent();
    BasicBlock* thenBB = BasicBlock::Create(context, "then", function);
    BasicBlock* elseBB = BasicBlock::Create(context, "else");
    BasicBlock* mergeBB = BasicBlock::Create(context, "ifcont");

    builder->CreateCondBr(condition, thenBB, elseBB);
    builder->SetInsertPoint(thenBB);

    codegen_statement_block(n->children.front(), builder);
    n->children.pop_front();
    n->children.pop_front(); // Popping else if there

    builder->CreateBr(mergeBB);
    thenBB = builder->GetInsertBlock();


    function->getBasicBlockList().push_back(elseBB);
    builder->SetInsertPoint(elseBB);

    codegen_statement_block(n->children.front(), builder);
    n->children.pop_front();

    builder->CreateBr(mergeBB);
    elseBB = builder->GetInsertBlock();

    function->getBasicBlockList().push_back(mergeBB);
    builder->SetInsertPoint(mergeBB);
//    PHINode* pn = builder.CreatePHI(Type::getDoubleTy(context), 2, "iftmp");

//    pn->addIncoming(thenB)
}
void code_generation::codegen_for_statement(node *n) {
    n->children.pop_front(); // Popping for
    codegen_variable_assignment(n->children.front(), builder); // processing loop var
    n->children.pop_front(); // pop loop var assignment

    node* conditional_copy = new node(n->children.front()); // copying conditional
    Value* start_conditional = codegen_expression(n->children.front());
    n->children.pop_front(); // Popping expression

    Function* function = builder->GetInsertBlock()->getParent();
    BasicBlock* loopBB = BasicBlock::Create(context, "loop", function);
    BasicBlock* continueBB = BasicBlock::Create(context, "forcont");

    builder->CreateCondBr(start_conditional, loopBB, continueBB);
    builder->SetInsertPoint(loopBB);

    codegen_statement_block(n->children.front(), builder);
    n->children.pop_front(); // pop statement

    Value *endVal = codegen_expression(conditional_copy); // codegen for end expression
    builder->CreateCondBr(endVal, loopBB, continueBB);
    n->children.pop_front(); // Popping end
    n->children.pop_front(); // Popping for

    function->getBasicBlockList().push_back(continueBB);
    builder->SetInsertPoint(continueBB);
}
void code_generation::codegen_return_statement(node *n) {
    n->children.pop_front(); // Popping return
    builder->CreateRet(codegen_expression(n->children.front()));
    n->children.pop_front(); // Popping return value
}

void code_generation::codegen_variable_declaration(node *n, IRBuilder<> *b) {
    n->children.pop_front(); // variable
    string s = n->children.front()->val.stringValue;
    n->children.pop_front(); // variable name
    n->children.pop_front(); // colon


    Type* type = get_type(n->children.front());
    n->children.pop_front();
    if (!n->children.empty()) {
        n->children.pop_front(); // [
        int size = cast<ConstantInt>(codegen_expression(n->children.front()))->getZExtValue();
        n->children.pop_front(); // size
        n->children.pop_front(); // ]
        type = ArrayType::get(type, size);
    }


    AllocaInst* alloca = builder->CreateAlloca(type, 0, s.c_str());
    identifiers.insert_or_assign(s, alloca);


}
void code_generation::codegen_variable_assignment(node *n, IRBuilder<>* b) {
    string s = n->children.front()->val.stringValue;
    Value* varInst = identifiers.at(s);
    n->children.pop_front();
    if (n->children.front()->type == T_LBRACKET){

        n->children.pop_front();
        Value* index = codegen_expression(n->children.front());
        n->children.pop_front();
        n->children.pop_front();

        Value *i32zero = ConstantInt::get(context, APInt(8, 0));
        Value *indices[2] = {i32zero, index};
        varInst = b->CreateGEP(varInst, ArrayRef<Value *>(indices, 2));
    }
    n->children.pop_front();
    Value* v =  codegen_expression(n->children.front());

    b->CreateStore(v, varInst);
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

    n->children.pop_front(); // Popping procedure
    std::string name = n->children.front()->val.stringValue;
    n->children.pop_front(); // Popping name
    n->children.pop_front(); // Popping :
    n->children.pop_front(); // Popping return type

    std::vector<std::string> arg_names;
    std::vector<Type *> arg_types;
    for (auto arg : n->children.front()->children){
        arg->children.pop_front(); // pop variable
        arg_names.emplace_back(arg->children.front()->val.stringValue);
        arg->children.pop_front(); // pop var name
        arg->children.pop_front(); // pop :
        arg_types.push_back(get_type(arg->children.front()));
        arg->children.pop_front(); // pop type
    }

    FunctionType *FT = FunctionType::get(Type::getDoubleTy(context), arg_types, false);

    Function* f = Function::Create(FT, Function::ExternalLinkage, name, m);

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : f->args())
        Arg.setName(arg_names[Idx++]);


    functions.insert_or_assign(name, f);

    BasicBlock *BB = BasicBlock::Create(context, "entry", f);
    BasicBlock* currentBlock = builder->GetInsertBlock();

    builder->SetInsertPoint(BB);

    namedValues.clear();
    for (auto &Arg : f->args()){
//        Value* temp = &Arg;
//        auto
//        identifiers.insert_or_assign(std::string(Arg.getName()), )
        namedValues[std::string(Arg.getName())] = &Arg;
    }

    n->children.pop_front(); // Popping params
    codegen_declaration_block(n->children.front(), builder);
    n->children.pop_front(); // Popping declaration block
    n->children.pop_front(); // Popping begin

    codegen_statement_block(n->children.front(), builder);
    n->children.pop_front();

    builder->SetInsertPoint(currentBlock);

    return codegen_literal_integer(0);
}

Value *code_generation::codegen_literal_integer(int n) {
    return ConstantInt::get(Type::getInt32Ty(context), APInt(32, n));
}
Value *code_generation::codegen_literal_float(double n) {
    return ConstantFP::get(context, APFloat(n));
}
Value *code_generation::codegen_literal_boolean(bool n) {
    return ConstantInt::get(Type::getInt1Ty(context), APInt(1, n ? 1 : 0));
}
Value *code_generation::codegen_literal_string(const std::string&  n) {

    auto ss = StringRef(n);
    auto string_val = ConstantDataArray::getString(context, ss, true);

    AllocaInst* alloca = builder->CreateAlloca(string_val->getType(), 0, "temp_string_alloca");
    builder->CreateStore(string_val, alloca);
    Value *i32zero = ConstantInt::get(context, APInt(8, 0));
    Value *i32one = ConstantInt::get(context, APInt(8, 0));
    Value *indices[2] = {i32zero, i32one};
    Value* ptr = builder->CreateGEP(alloca, ArrayRef<Value *>(indices, 2));
    return ptr;
}

Value *code_generation::codegen_literal_array(std::vector<Value*> values) {
    auto arr = ConstantDataArray::get(context, values);
    return arr;
}

Value *code_generation::codegen_expression(node *n,  Value* lhs) {

    bool is_not_l = false;
    if (!n->children.empty() && n->children.front()->type == T_NOT){
        n->children.pop_front();  // Pop not
        is_not_l = true;
    }

    if (lhs == nullptr){
        lhs = is_not_l ? builder->CreateNot(codegen_arith_op(n->children.front())) : codegen_arith_op(n->children.front());
        n->children.pop_front();
    }

    if (n->children.empty())
        return lhs;

    int operation = n->children.front()->type;
    n->children.pop_front(); // Pop sign

    bool is_not_r = false;
    if (n->children.front()->type == T_NOT){
        n->children.pop_front(); // Pop not
        is_not_r = true;
    }
    Value* rhs = is_not_r ? builder->CreateNot(codegen_arith_op(n->children.front())) : codegen_arith_op(n->children.front());
    n->children.pop_front(); // Pop RHS

    // check flips
    switch (operation) {
        case T_AND:
            return codegen_expression(n, builder->CreateAnd(lhs, rhs, "andtmp"));
        case T_OR:
            return codegen_expression(n, builder->CreateOr(lhs, rhs, "ortmp"));
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
            return codegen_arith_op(n,operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFAdd(lhs, rhs,"addtmp");},
                                lhs, rhs));
        case T_MINUS:
            return codegen_arith_op(n,operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFSub(lhs, rhs,"subtmp");},
                                lhs, rhs));
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
            return codegen_term(n, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOLT(lhs, rhs,"lthan");},
                    lhs, rhs, true));
        case T_LE_THAN:
            return codegen_term(n, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOLE(lhs, rhs,"lethan");},
                    lhs, rhs, true));
        case T_G_THAN:
            return codegen_term(n, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOGT(lhs, rhs,"gthan");},
                    lhs, rhs, true));
        case T_GE_THAN:
            return codegen_term(n, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOGE(lhs, rhs,"gethan");},
                    lhs, rhs, true));
        case T_D_EQUALS:
            return codegen_term(n, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOEQ(lhs, rhs,"equals");},
                    lhs, rhs, true));
        case T_N_EQUALS:
            return codegen_term(n, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpONE(lhs, rhs,"nequals");},
                    lhs, rhs, true));
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

    std::function<Value*(Value* lhs, Value* rhs)> standard_op = [this](Value* lhs, Value* rhs){return builder->CreateMul(lhs, rhs);};
    std::function<Value*(Value* lhs, Value* rhs)> floating_op = [this](Value* lhs, Value* rhs){return builder->CreateFMul(lhs, rhs);};

    switch (operation) {
        case T_MULTIPLY:
            return codegen_term(n, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFMul(lhs, rhs,"multmp");},
                    lhs, rhs));
        case T_DIVIDE:
            return codegen_term(n, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFDiv(lhs, rhs,"divtmp");},
                    lhs, rhs));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_factor(node *n) {

    node* x = n->children.front();
    n->children.pop_front();

    bool is_neg = false;
    Value* return_val = nullptr;

    if (x->type == T_MINUS) {
        is_neg = true;
        x = n->children.front();
        n->children.pop_front();
    }

    if (x->type == T_INTEGER_LITERAL) return_val = codegen_literal_integer(x->val.intValue);
    if (x->type == T_FLOAT_LITERAL) return_val = codegen_literal_float(x->val.doubleValue);
    if (x->type == T_IDENTIFIER) {
        if (n->children.empty()){
            if (namedValues.contains(x->val.stringValue)){
                return_val = namedValues.at(x->val.stringValue);
            } else if (identifiers.contains(x->val.stringValue)){
                return_val = builder->CreateLoad(identifiers.at(x->val.stringValue));
            }
        } else {
            Value* index = codegen_expression(n->children.front());
            AllocaInst* array_inst = identifiers.at(x->val.stringValue);
            Value *i32zero = ConstantInt::get(context, APInt(8, 0));
            Value *indices[2] = {i32zero, index};
            auto varInst = builder->CreateGEP(array_inst, ArrayRef<Value *>(indices, 2));
            return_val = builder->CreateLoad(varInst);
        }

    }
    if (x->type == T_PROCEDURE_CALL && x->children.front()->type == T_IDENTIFIER){
        const string functionName = x->children.front()->val.stringValue;
        x->children.pop_front();
        if (string("putinteger") == functionName) {
            return_val = codegen_print_integer(m,codegen_expression(x->children.front()));
        }  else if (string("putfloat") == functionName){
            return_val = codegen_print_double(m,codegen_expression(x->children.front()));
        }   else if (string("putstring") == functionName) {
            return_val = codegen_print_string(m, codegen_expression(x->children.front()));
        }   else if (string("putbool") == functionName) {
            Value *v = codegen_expression(x->children.front());
            return_val = codegen_print_boolean(m, v);
        } else if (string("getinteger") == functionName){
            return_val = codegen_scan_integer();
        } else if (string("getfloat") == functionName){
            return_val = codegen_scan_double();
        } else if (string("getstring") == functionName){
            Value* v = codegen_scan_string();
            return_val =  v;
        } else if (string("getbool") == functionName){
            return_val = codegen_scan_bool();
        } else if (m->getFunction(functionName) != nullptr){
            std::vector<Value *> args;

            for (auto child : x->children){
                args.push_back(codegen_expression(child));
            }
            return_val = builder->CreateCall(m->getFunction(functionName), args);
        }
    }
    if (x->type == T_FALSE) return_val = codegen_literal_boolean(false);
    if (x->type == T_TRUE) return_val = codegen_literal_boolean(true);
    if (x->type == T_STRING_LITERAL) return_val = codegen_literal_string(x->val.stringValue);

    return is_neg ? builder->CreateNeg(return_val) : return_val;
}

void code_generation::codegen_print_prototype(Module *mod) {


    Function* printer = mod->getFunction("printf");
    if (printer == nullptr){
        std::vector<Type *> args;
        args.push_back(Type::getInt8PtrTy(context));

        FunctionType *printfType = FunctionType::get(builder->getInt32Ty(), args, true);
        Function::Create(printfType, Function::ExternalLinkage, "printf",
                         mod);
    }
}
Value* code_generation::codegen_print_base(Module* mod, Value* v, Value* formatStr) {
    std::vector<Value *> printArgs;

    printArgs.push_back(formatStr);
    printArgs.push_back(v);

    builder->CreateCall(mod->getFunction("printf"), printArgs);
    return codegen_literal_boolean(true);
}
Value* code_generation::codegen_print_string(Module *mod, Value *v) {
    if (!namedValues.contains(".str")){
        Value *formatStr = builder->CreateGlobalStringPtr("%s\n", ".str");
        namedValues.insert_or_assign(".str", formatStr);
    }
    return codegen_print_base(mod,v, namedValues.at(".str"));
}
Value* code_generation::codegen_print_double(Module *mod, Value *v) {
    if (!namedValues.contains(".double")){
        Value *formatStr = builder->CreateGlobalStringPtr("%g\n", ".double");
        namedValues.insert_or_assign(".double", formatStr);
    }
    return codegen_print_base(mod, v, namedValues.at(".double"));
}
Value* code_generation::codegen_print_integer(Module *mod, Value *v) {
    if (!namedValues.contains(".int")){
        Value *formatStr = builder->CreateGlobalStringPtr("%d\n", ".int");
        namedValues.insert_or_assign(".int", formatStr);
    }
    // change integers to actually be ints ... maybe
    return codegen_print_base(mod, v, namedValues.at(".int"));
}
Value* code_generation::codegen_print_boolean(Module *mod, Value *v) {
    if (!namedValues.contains(".int")){
        Value *formatStr = builder->CreateGlobalStringPtr("%d\n", ".int");
        namedValues.insert_or_assign(".int", formatStr);
    }
    // change integers to actually be ints ... maybe
    return codegen_print_base(mod, v, namedValues.at(".int"));
}

void code_generation::codegen_scan_prototype() {


    Function* printer = m->getFunction("scanf");
    if (printer == nullptr){
        std::vector<Type *> args;
        args.push_back(Type::getInt8PtrTy(context));

        FunctionType *printfType = FunctionType::get(builder->getInt32Ty(), args, true);
        Function::Create(printfType, Function::ExternalLinkage, "scanf", m);
    }

    Function* malloc = m->getFunction("malloc");
    if (malloc == nullptr){
        std::vector<Type *> args;
        args.push_back(Type::getInt32Ty(context));

        FunctionType *printfType = FunctionType::get(builder->getInt8PtrTy(), args, true);
        Function::Create(printfType, Function::ExternalLinkage, "malloc", m);
    }
}
void code_generation::codegen_scan_string_prototype() {

    std::vector<std::string> arg_names = {"full_str", "size"};
    std::string name = "shorten_string";

    std::vector<Type *> arg_types = {
            ArrayType::get(Type::getInt8Ty(context), 256),
            Type::getInt32Ty(context)
    };

    FunctionType *FT = FunctionType::get(Type::getDoubleTy(context), arg_types, false);

    Function* f = Function::Create(FT, Function::ExternalLinkage, name, m);

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : f->args())
        Arg.setName(arg_names[Idx++]);

    functions.insert_or_assign(name, f);

    BasicBlock *BB = BasicBlock::Create(context, "entry", f);
    BasicBlock* currentBlock = builder->GetInsertBlock();

    builder->SetInsertPoint(BB);

    namedValues.clear();
    for (auto &Arg : f->args()){
//        Value* temp = &Arg;
//        auto
//        identifiers.insert_or_assign(std::string(Arg.getName()), )
        namedValues[std::string(Arg.getName())] = &Arg;
    }

//    CallInst::CreateMa

}
Value *code_generation::codegen_scan_base(Type* t, Value *formatStr) {
    AllocaInst* tempInst = builder->CreateAlloca(t, 0, "temp");
    std::vector<Value *> printArgs;

    printArgs.push_back(formatStr);
    printArgs.push_back(tempInst);

    builder->CreateCall(m->getFunction("scanf"), printArgs);
    return builder->CreateLoad(tempInst);
}
Value *code_generation::codegen_scan_string() {
    if (!namedValues.contains(".string_sc")){
        Value *formatStr = builder->CreateGlobalStringPtr("%[^\n]%n", ".string_sc");
//        Value *formatStr = builder->CreateGlobalStringPtr("%n", ".string_sc");
        namedValues.insert_or_assign(".string_sc", formatStr);
    }
    ArrayType* arrayType = ArrayType::get(Type::getInt8Ty(context), 256);
    AllocaInst* tempInst = builder->CreateAlloca(arrayType, 0, "string_buffer");
    AllocaInst* lengthInst = builder->CreateAlloca(Type::getInt32Ty(context), 0, "temp_string_size");

    std::vector<Value *> printArgs;

    printArgs.push_back(namedValues.at(".string_sc"));
    printArgs.push_back(tempInst);
    printArgs.push_back(lengthInst);

    builder->CreateCall(m->getFunction("scanf"), printArgs);

    std::vector<Value *> mallocArgs;
    mallocArgs.push_back(builder->CreateMul(builder->CreateLoad(lengthInst), codegen_literal_integer(8)));
    Value* string_malloc = builder->CreateCall(m->getFunction("malloc"), mallocArgs);


//    string_len_value->print(llvm::outs());
//    string_len_value->
////    CastInst::
//    int string_len = llvm::cast<llvm::ConstantInt>(codegen_literal_integer(6))->getZExtValue();
//    ArrayType* rec_string_type = ArrayType::get(Type::getInt8Ty(context), string_len);
//    AllocaInst* string_inst = builder->CreateAlloca(rec_string_type, 0, "temp_put_string");

//    LoadInst* string_value = builder->(tempInst);
//    builder->CreateLoad(Type::getInt32Ty(context), lengthInst);

//    builder->CreateAllo

    builder->CreateStore(tempInst, string_malloc);
    Value *i32zero = ConstantInt::get(context, APInt(8, 0));
    Value *i32one = ConstantInt::get(context, APInt(8, 0));
    Value *indices[2] = {i32zero, i32one};
    Value* ptr = builder->CreateGEP(tempInst, ArrayRef<Value *>(indices, 2));

//    builder->CreateStore(ptr, string_malloc);

    codegen_print_integer(m,builder->CreateLoad(lengthInst));
    return ptr;
//    return codegen_scan_base(Type::getDoubleTy(context),namedValues.at(".string_sc"));
}
Value *code_generation::codegen_scan_double() {
    if (!namedValues.contains(".double_sc")){
        Value *formatStr = builder->CreateGlobalStringPtr("%lf", ".double_sc");
        namedValues.insert_or_assign(".double_sc", formatStr);
    }
    return codegen_scan_base(Type::getDoubleTy(context),namedValues.at(".double_sc"));
}
Value *code_generation::codegen_scan_integer() {
    if (!namedValues.contains(".integer_sc")){
        Value *formatStr = builder->CreateGlobalStringPtr("%u", ".integer_sc");
        namedValues.insert_or_assign(".integer_sc", formatStr);
    }
    return codegen_scan_base(Type::getInt32Ty(context),namedValues.at(".integer_sc"));
}
Value *code_generation::codegen_scan_bool() {
    if (!namedValues.contains(".integer_sc")){
        Value *formatStr = builder->CreateGlobalStringPtr("%u", ".integer_sc");
        namedValues.insert_or_assign(".integer_sc", formatStr);
    }
    return codegen_scan_base(Type::getInt1Ty(context),namedValues.at(".integer_sc"));
}

Value *code_generation::operation_block(const std::function<Value*(Value* lhs, Value* rhs)>& floating_op,
                                        Value* lhs, Value* rhs, bool is_comparison) {
    Type* lhs_type = nullptr;
    Type* rhs_type = nullptr;

    if (lhs->getType() != Type::getDoubleTy(context)){
        lhs_type = lhs->getType();
        lhs = builder->CreateSIToFP(lhs, Type::getDoubleTy(context));
    }
    if (rhs != nullptr && rhs->getType() != Type::getDoubleTy(context)){
        rhs_type = rhs->getType();
        rhs = builder->CreateSIToFP(rhs, Type::getDoubleTy(context));
    }

    Value *v = floating_op(lhs,rhs);

//    if (lhs_type != nullptr)
//        lhs = builder->CreateFPToSI(lhs, lhs_type);
//    if (rhs_type != nullptr)
//        rhs = builder->CreateFPToSI(rhs, rhs_type);
    if (lhs_type != nullptr && rhs_type != nullptr && lhs_type == rhs_type && !is_comparison)
        v = builder->CreateFPToSI(v, rhs_type);

    return v;
}

Type *code_generation::get_type(node *n) {
    switch (n->children.front()->type) {
        case T_INTEGER_TYPE: return Type::getInt32Ty(context);
        case T_FLOAT_TYPE: return Type::getDoubleTy(context);
        case T_BOOL_TYPE: return Type::getInt1Ty(context);
        case T_STRING_TYPE: return Type::getInt8PtrTy(context);
    }
    return nullptr;
}
Type *code_generation::get_array_type(node *n) {
    return nullptr;
}
Value *code_generation::generateValue(Module *m) {
    //0. Defs
    auto str = string("Butt");
    auto charType = llvm::IntegerType::get(context, 8);
    auto arrayType = ArrayType::get(charType, str.length() + 1);

    //1. Initialize chars vector
    std::vector<llvm::Constant *> chars(str.length());
    for(unsigned int i = 0; i < str.size(); i++) {
        chars[i] = llvm::ConstantInt::get(charType, str[i]);
    }

    //1b. add a zero terminator too
    chars.push_back(llvm::ConstantInt::get(charType, 0));


    //2. Initialize the string from the characters
    auto stringType = llvm::ArrayType::get(charType, chars.size());

    //3. Create the declaration statement
    auto globalDeclaration = (llvm::GlobalVariable*) m->getOrInsertGlobal(".str", stringType);
//    globalDeclaration->setInitializer(llvm::ConstantArray::get(stringType, chars));
//    globalDeclaration->setConstant(true);
//    globalDeclaration->setLinkage(llvm::GlobalValue::LinkageTypes::PrivateLinkage);
//    globalDeclaration->setUnnamedAddr (llvm::GlobalValue::UnnamedAddr::Global);



    //4. Return a cast to an i8*
//    return llvm::ConstantExpr::getBitCast(chars, charType->getPointerTo());
    return ConstantArray::get(arrayType, chars);
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









