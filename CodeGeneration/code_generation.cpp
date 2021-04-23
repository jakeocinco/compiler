//
// Created by Jacob Carlson on 4/6/21.
//

#include <sstream>
#include "code_generation.h"
#include "../Parser/parser.h"
#include "../tokenCodes.h"


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

    variable_scope = new scope(builder, m);

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

//        Value* temp = variable_scope->get_temp("arr")->get();
//        builder->CreateInBoundsGEP()
//        ConstantArray* ca = cast<ConstantArray>(temp);
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
bool code_generation::codegen_statement_block(node *n, IRBuilder<>* b) {
    bool parse_return = false;
    for (node* x : n->children){
        if (x->type == T_VARIABLE_ASSIGNMENT) codegen_variable_assignment(x, b);
        else if (x->type == T_IF_BLOCK) codegen_if_statement(x);
        else if (x->type == T_FOR_LOOP) codegen_for_statement(x);
        else if (x->type == T_RETURN_BLOCK) {
            codegen_return_statement(x);
            parse_return = true;
            break;
        }
    }
    return parse_return;
}

void code_generation::codegen_if_statement(node* n) {
    n->children.pop_front(); // Popping if
    int bool_size;
    Value* condition = codegen_expression(n->children.front(), bool_size);
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
    int cond_size;
    Value* start_conditional = codegen_expression(n->children.front(),cond_size);
    n->children.pop_front(); // Popping expression

    Function* function = builder->GetInsertBlock()->getParent();
    BasicBlock* loopBB = BasicBlock::Create(context, "loop", function);
    BasicBlock* continueBB = BasicBlock::Create(context, "forcont");

    builder->CreateCondBr(start_conditional, loopBB, continueBB);
    builder->SetInsertPoint(loopBB);

    codegen_statement_block(n->children.front(), builder);
    n->children.pop_front(); // pop statement

    Value *endVal = codegen_expression(conditional_copy, cond_size); // codegen for end expression
    builder->CreateCondBr(endVal, loopBB, continueBB);
    n->children.pop_front(); // Popping end
    n->children.pop_front(); // Popping for

    function->getBasicBlockList().push_back(continueBB);
    builder->SetInsertPoint(continueBB);
}
void code_generation::codegen_return_statement(node *n) {
    n->children.pop_front(); // Popping return
    int cond_size;
    builder->CreateRet(codegen_expression(n->children.front(), cond_size));
    n->children.pop_front(); // Popping return value
}

void code_generation::codegen_variable_declaration(node *n, IRBuilder<> *b) {
    bool is_global = false;
    if (n->children.front()->type == T_GLOBAL) {
        is_global = true;
        n->children.pop_front(); // global
    }

    n->children.pop_front(); // variable
    string s = n->children.front()->val.stringValue;
    n->children.pop_front(); // variable name
    n->children.pop_front(); // colon


    Type* type = get_type(n->children.front());
    Type* element_type = type;
    n->children.pop_front();
    int size = 1;
    variable_inst::VARIABLE_CLASS clazz = variable_inst::VARIABLE_CLASS::INSTANCE;
    if (!n->children.empty()) {
        n->children.pop_front(); // [
        int size_size;
        size = cast<ConstantInt>(codegen_expression(n->children.front(), size_size))->getZExtValue();
        n->children.pop_front(); // size
        n->children.pop_front(); // ]
        type = ArrayType::get(type, size);
        element_type = type->getArrayElementType();
        clazz = variable_inst::VARIABLE_CLASS::ARRAY_INSTANCE;
    }

    Value* variable_ptr;
    if (is_global) {

        variable_ptr = new GlobalVariable(*m,
                                 type,
                                 false,
                                 llvm::GlobalValue::CommonLinkage,
                                 Constant::getNullValue(type),
                                 s);
    } else {
        variable_ptr = builder->CreateAlloca(type, 0, s.c_str());
    }

    variable_scope->add(s, variable_ptr, element_type, clazz, size);
}
void code_generation::codegen_variable_assignment(node *n, IRBuilder<>* b) {
    string s = n->children.front()->val.stringValue;

    n->children.pop_front();
    Value* index = nullptr;

    bool is_array_index = false;

    Function* function = nullptr;
    BasicBlock* thenBB = nullptr;
    BasicBlock* elseBB = nullptr;
    BasicBlock* mergeBB = nullptr;

    if (n->children.front()->type == T_LBRACKET){
        is_array_index = true;

        n->children.pop_front();
        int temp_size;
        index = codegen_expression(n->children.front(), temp_size);
        n->children.pop_front();
        n->children.pop_front();

        Value* lhs = variable_scope->get_temp(s + "_size")->get();
        Value* cond = b->CreateICmpSGT(lhs, index);

        function = b->GetInsertBlock()->getParent();
        thenBB = BasicBlock::Create(context, "validIndex", function);
        elseBB = BasicBlock::Create(context, "invalidIndex");
        mergeBB = BasicBlock::Create(context, "ifcont");

        b->CreateCondBr(cond, thenBB, elseBB);
        b->SetInsertPoint(thenBB);

    }

        n->children.pop_front();

        int exp_size;
        Value* v =  codegen_expression(n->children.front(),exp_size);
        variable_scope->set(s, v,exp_size, index);


    if (is_array_index){
        builder->CreateBr(mergeBB);
        thenBB = builder->GetInsertBlock();

        function->getBasicBlockList().push_back(elseBB);
        builder->SetInsertPoint(elseBB);

        int temp = 1;
        codegen_print_string(m, codegen_literal_string("Invalid array index", temp));


        builder->CreateBr(mergeBB);
        elseBB = builder->GetInsertBlock();

        function->getBasicBlockList().push_back(mergeBB);
        builder->SetInsertPoint(mergeBB);

    }

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
    Type* return_type = get_type(n->children.front());
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

    FunctionType *FT = FunctionType::get(return_type, arg_types, false);

    Value* f_temp = Function::Create(FT, Function::ExternalLinkage, name, m);

    variable_scope->add(name, f_temp, f_temp->getType(), variable_inst::VARIABLE_CLASS::FUNCTION);
    variable_scope = new scope(variable_scope);
    variable_scope->add(name, f_temp, f_temp->getType(), variable_inst::VARIABLE_CLASS::FUNCTION);

    Function* f = cast<Function>(f_temp);

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
        variable_scope->add(std::string(Arg.getName()), &Arg, (&Arg)->getType(), variable_inst::VARIABLE_CLASS::VALUE);
    }

    n->children.pop_front(); // Popping params
    codegen_declaration_block(n->children.front(), builder);
    n->children.pop_front(); // Popping declaration block

    n->children.pop_front(); // Popping begin

    if (!codegen_statement_block(n->children.front(), builder)){
        throw runtime_error(string("Procedure '") + name + "' must contain a return statement.");
    }
    n->children.pop_front();

    builder->SetInsertPoint(currentBlock);

    variable_scope = variable_scope->get_parent();
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
Value *code_generation::codegen_literal_string(const std::string& n, int& size) {

    auto ss = StringRef(n);
    size = n.length();
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

Value *code_generation::codegen_expression(node *n, int& size,  Value* lhs) {

    bool is_not_l = false;
    if (!n->children.empty() && n->children.front()->type == T_NOT){
        n->children.pop_front();  // Pop not
        is_not_l = true;
    }

    if (lhs == nullptr){
        lhs = is_not_l ? builder->CreateNot(codegen_arith_op(n->children.front(), size)) : codegen_arith_op(n->children.front(),size);
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
    int rhs_size;
    Value* rhs = is_not_r ? builder->CreateNot(codegen_arith_op(n->children.front(),rhs_size)) : codegen_arith_op(n->children.front(),rhs_size);
    n->children.pop_front(); // Pop RHS

    if (size != rhs_size){
        // Do something
    }
    // check flips
    switch (operation) {
        case T_AND:
            return codegen_expression(n, size, builder->CreateAnd(lhs, rhs, "andtmp"));
        case T_OR:
            return codegen_expression(n, size, builder->CreateOr(lhs, rhs, "ortmp"));
        default:
            cout << "Error" << endl;
    }


    return nullptr;
}
Value *code_generation::codegen_arith_op(node *n, int& size,  Value* lhs) {

    if (n->children.empty())
        return lhs;
    if (n->children.size() == 1)
        return codegen_relation(n->children.front(), size);

    if (lhs == nullptr){
        lhs = codegen_relation(n->children.front(), size);
        n->children.pop_front();
    }

    int operation = n->children.front()->type;
    n->children.pop_front();

    int rhs_size;
    Value* rhs = codegen_relation(n->children.front(), rhs_size);
    n->children.pop_front();

    if (size != rhs_size){
        // Do something
    }

    switch (operation) {
        case T_ADD:
            return codegen_arith_op(n, size,operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFAdd(lhs, rhs,"addtmp");},
                                lhs, size, rhs, rhs_size));
        case T_MINUS:
            return codegen_arith_op(n, size,operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFSub(lhs, rhs,"subtmp");},
                                lhs, size, rhs, rhs_size));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_relation(node *n, int& size, Value* lhs) {

    if (n->children.empty())
        return lhs;
    if (n->children.size() == 1)
        return codegen_term(n->children.front(), size);

    if (lhs == nullptr){
        lhs = codegen_term(n->children.front(), size);
        n->children.pop_front(); // Pop lhs
    }
    int operation = n->children.front()->type;
    n->children.pop_front(); // Pop operation
    int rhs_size;
    Value* rhs = codegen_term(n->children.front(), rhs_size);
    if (size != rhs_size){
        // Do something
    }
    n->children.pop_front(); // Pop rhs
    switch (operation) {
        case T_L_THAN:
            return codegen_term(n,size, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOLT(lhs, rhs,"lthan");},
                    lhs, size, rhs, rhs_size, true));
        case T_LE_THAN:
            return codegen_term(n,size, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOLE(lhs, rhs,"lethan");},
                    lhs, size, rhs, rhs_size, true));
        case T_G_THAN:
            return codegen_term(n,size, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOGT(lhs, rhs,"gthan");},
                    lhs, size, rhs, rhs_size, true));
        case T_GE_THAN:
            return codegen_term(n,size, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOGE(lhs, rhs,"gethan");},
                    lhs, size, rhs, rhs_size, true));
        case T_D_EQUALS:
            return codegen_term(n,size, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpOEQ(lhs, rhs,"equals");},
                    lhs, size, rhs, rhs_size, true));
        case T_N_EQUALS:
            return codegen_term(n,size, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFCmpONE(lhs, rhs,"nequals");},
                    lhs, size, rhs, rhs_size, true));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_term(node *n, int& size, Value* lhs) {

    if (n->children.empty())
        return lhs;
    if (n->children.size() == 1)
        return codegen_factor(n->children.front(), size);

    if (lhs == nullptr){
        lhs = codegen_factor(n->children.front(), size);
        n->children.pop_front();
    }

    int operation = n->children.front()->type;
    n->children.pop_front();
    int rhs_size;
    Value* rhs = codegen_factor(n->children.front(),rhs_size);
    if (size != rhs_size){
        // Do something
    }
    n->children.pop_front();

    switch (operation) {
        case T_MULTIPLY:
            return codegen_term(n, size, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFMul(lhs, rhs,"multmp");},
                    lhs, size, rhs, rhs_size));
        case T_DIVIDE:
            return codegen_term(n, size, operation_block(
                    [this](Value* lhs, Value* rhs){return builder->CreateFDiv(lhs, rhs,"divtmp");},
                    lhs, size, rhs, rhs_size));
        default:
            cout << "Error" << endl;
    }
    return nullptr;
}
Value *code_generation::codegen_factor(node *n, int& size) {

    node* x = n->children.front();
    n->children.pop_front();

    bool is_neg = false;
    Value* return_val = nullptr;

    if (x->type == T_MINUS) {
        is_neg = true;
        x = n->children.front();
        n->children.pop_front();
    }

    if (x->type == T_EXPRESSION){
        return_val = codegen_expression(x, size);
    }
    else if (x->type == T_INTEGER_LITERAL) {
        return_val = codegen_literal_integer(x->val.intValue);
        size = 1;
    }
    else if (x->type == T_FLOAT_LITERAL) {
        return_val = codegen_literal_float(x->val.doubleValue);
        size = 1;
    }
    else if (x->type == T_IDENTIFIER) {
        if (n->children.empty()){
            auto vi = variable_scope->get_temp(x->val.stringValue);
            return_val = vi->get();
            size = vi->get_size();
        } else {
            int index_size;
            Value* index = codegen_expression(n->children.front(),index_size);

            string name = x->val.stringValue;
            Value* lhs = variable_scope->get_temp(name + "_size")->get();
            Value* cond = builder->CreateICmpSGT(lhs, index);

            Function* function = builder->GetInsertBlock()->getParent();
            BasicBlock* thenBB = BasicBlock::Create(context, "validIndex", function);
            BasicBlock* elseBB = BasicBlock::Create(context, "invalidIndex");
            BasicBlock* mergeBB = BasicBlock::Create(context, "ifcont");

            builder->CreateCondBr(cond, thenBB, elseBB);
            builder->SetInsertPoint(thenBB);


            return_val = variable_scope->get_temp(x->val.stringValue)->get(index);
            size = 1;

            builder->CreateBr(mergeBB);
            thenBB = builder->GetInsertBlock();

            function->getBasicBlockList().push_back(elseBB);
            builder->SetInsertPoint(elseBB);

            int temp_size = 0;
            codegen_print_string(m, codegen_literal_string("Invalid array index", temp_size));


            builder->CreateBr(mergeBB);
            elseBB = builder->GetInsertBlock();

            function->getBasicBlockList().push_back(mergeBB);
            builder->SetInsertPoint(mergeBB);
        }
    }
    else if (x->type == T_PROCEDURE_CALL && x->children.front()->type == T_IDENTIFIER){
        const string functionName = x->children.front()->val.stringValue;
        x->children.pop_front();
        size = 1;
        int temp_size = 1;
        if (string("putinteger") == functionName) {
            return_val = codegen_print_integer(m,codegen_expression(x->children.front(), temp_size));
        }
        else if (string("putfloat") == functionName){
            return_val = codegen_print_double(m,codegen_expression(x->children.front(), temp_size));
        }
        else if (string("putstring") == functionName) {
            return_val = codegen_print_string(m, codegen_expression(x->children.front(), temp_size));
        }
        else if (string("putbool") == functionName) {
            Value *v = codegen_expression(x->children.front(), temp_size);
            return_val = codegen_print_boolean(m, v);
        }
        else if (string("getinteger") == functionName){
            return_val = codegen_scan_integer();
        }
        else if (string("getfloat") == functionName){
            return_val = codegen_scan_double();
        }
        else if (string("getstring") == functionName){
            Value* v = codegen_scan_string();
            return_val =  v;
        }
        else if (string("getbool") == functionName){
            return_val = codegen_scan_bool();
        }
        else if (string("sqrt") == functionName){
            return_val = codegen_sqrt(codegen_expression(x->children.front(),temp_size));
        }
        else if (m->getFunction(functionName) != nullptr){
            std::vector<Value *> args;

            for (auto child : x->children){
                args.push_back(codegen_expression(child, size));
            }
            // Casting the value back to Function -- this is safe because its created as function, just stored as a value
            return_val = builder->CreateCall(cast<Function>(variable_scope->get_temp(functionName)->get()), args);
        }
    }
    else if (x->type == T_FALSE) {
        return_val = codegen_literal_boolean(false);
        size = 1;
    }
    else if (x->type == T_TRUE) {
        return_val = codegen_literal_boolean(true);
        size = 1;
    }
    else if (x->type == T_STRING_LITERAL) {
        return_val = codegen_literal_string(x->val.stringValue, size);
    }

    int temp = size;
    return is_neg ? operation_block(
            [this](Value* lhs,Value* rhs){return builder->CreateFNeg(lhs,"multmp");},
            return_val, temp, nullptr, 0)
            : return_val;
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

    Function* scanString = m->getFunction("scanString");
    if (scanString == nullptr){
        std::vector<Type *> args;

        FunctionType *printfType = FunctionType::get(builder->getInt8PtrTy(), args, true);
        Function::Create(printfType, Function::ExternalLinkage, "scanString", m);
    }

    Function* stringCompare = m->getFunction("stringCompare");
    if (stringCompare == nullptr){
        std::vector<Type *> args;
        args.push_back(Type::getInt8PtrTy(context));
        args.push_back(Type::getInt8PtrTy(context));

        FunctionType *printfType = FunctionType::get(builder->getInt8PtrTy(), args, true);
        Function::Create(printfType, Function::ExternalLinkage, "stringCompare", m);
    }

    Function* sqrt = m->getFunction("sqrt");
    if (sqrt == nullptr){
        std::vector<Type *> args;
        args.push_back(Type::getDoubleTy(context));

        FunctionType *printfType = FunctionType::get(builder->getDoubleTy(), args, true);
        Function::Create(printfType, Function::ExternalLinkage, "sqrt", m);
    }
//    Function* stringCompareLLVM = m->getFunction("stringCompareLLVM");
//    if (scanString == nullptr){
//        std::vector<Type *> args;
//        args.push_back(Type::getInt8PtrTy(context));
//        args.push_back(Type::getInt8PtrTy(context));
//
//        FunctionType *printfType = FunctionType::get(builder->getInt1Ty(), args, true);
//        Function* f = Function::Create(printfType, Function::ExternalLinkage, "stringCompareLLVM", m);
//
//        BasicBlock *BB = BasicBlock::Create(context, "entry", f);
//        BasicBlock* currentBlock = builder->GetInsertBlock();
//
//        builder->SetInsertPoint(BB);
//
//        vector<Value*> v;
//        for (auto &Arg : f->args()){
//            v.push_back(&Arg);
////            variable_scope->add(std::string(Arg.getName()), &Arg, (&Arg)->getType(), variable_inst::VARIABLE_CLASS::VALUE);
//        }
//
//        Function* function = builder->GetInsertBlock()->getParent();
//        BasicBlock* mainBlock = BasicBlock::Create(context, "startBl");
//        BasicBlock* retFalse = BasicBlock::Create(context, "retFalse");
//        BasicBlock* retTrue = BasicBlock::Create(context, "retTrue");
//        BasicBlock* callAgain = BasicBlock::Create(context, "callAgain");
//        BasicBlock* secondTier = BasicBlock::Create(context, "secondTier");
//
//        /** Main **/
//        builder->SetInsertPoint(mainBlock);
//        Value* vv = v.at(0);
//        Value* lc = builder->CreateLoad(vv);
//        Value* rc = builder->CreateLoad(v.at(1));
//
//        Value* l1 = builder->CreateICmpEQ(lc, ConstantInt::get(builder->getInt8Ty(), APInt(8,0)));
//        Value* r1 = builder->CreateICmpEQ(rc, ConstantInt::get(builder->getInt8Ty(), APInt(8,0)));
//
//        Value* cond = builder->CreateAnd(l1,r1);
//        builder->CreateCondBr(cond, retTrue, secondTier);
//
//        /** Return True **/
//        mainBlock = builder->GetInsertBlock();
//        function->getBasicBlockList().push_back(retTrue);
//        builder->SetInsertPoint(retTrue);
//
//        builder->CreateRet(codegen_literal_boolean(true));
//
//        /** Second Level **/
//        retTrue = builder->GetInsertBlock();
//        function->getBasicBlockList().push_back(secondTier);
//        builder->SetInsertPoint(secondTier);
//
//        cond = builder->CreateNot(builder->CreateOr(l1,r1));
//        builder->CreateCondBr(cond, retFalse, callAgain);
//
//        /** Return False **/
//        secondTier = builder->GetInsertBlock();
//        function->getBasicBlockList().push_back(retFalse);
//        builder->SetInsertPoint(retFalse);
//
//        /** Call Again **/
//        retFalse = builder->GetInsertBlock();
//        function->getBasicBlockList().push_back(callAgain);
//        builder->SetInsertPoint(callAgain);
//
////        builder->getInt8Ty()->get
//        Value* newL = builder->CreateAdd(v.at(0), ConstantInt::get(builder->getInt8Ty(), APInt(8,8)));
//        Value* newR = builder->CreateAdd(v.at(1), ConstantInt::get(builder->getInt8Ty(), APInt(8,8)));
//        vector<Value*> newArgs = {newL,newR};
//        builder->CreateRet(builder->CreateCall(f, newArgs));
//
//
//
//        cond = builder->CreateICmpEQ(builder->CreateLoad(v.at(0)), builder->CreateLoad(v.at(1)));
//    }
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

    std::vector<Value *> printArgs;
    return builder->CreateCall(m->getFunction("scanString"), printArgs);
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

Value *code_generation::codegen_sqrt(Value* v){
//    AllocaInst* tempInst = builder->CreateAlloca(t, 0, "temp");
    std::vector<Value *> printArgs;

    printArgs.push_back(v);


    return builder->CreateCall(m->getFunction("sqrt"), printArgs);
}

Value *code_generation::operation_block(const std::function<Value*(Value* lhs, Value* rhs)>& floating_op,
                                        Value* lhs, int &lhs_size,
                                        Value* rhs, int rhs_size,
                                        bool is_comparison) {

    if (lhs_size == 1 && rhs_size == 0){
        Type* lhs_type = nullptr;

        if (lhs->getType() != Type::getDoubleTy(context)){
            lhs_type = lhs->getType();
            lhs = builder->CreateSIToFP(lhs, Type::getDoubleTy(context));
        }

        Value *v = floating_op(lhs,nullptr);

        if (lhs_type != nullptr && !is_comparison)
            v = builder->CreateFPToSI(v, lhs_type);

        return v;
    }
    else if (lhs_size == 1 && rhs_size == 1){
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

        if (lhs_type != nullptr && rhs_type != nullptr && lhs_type == rhs_type && !is_comparison)
            v = builder->CreateFPToSI(v, rhs_type);

        return v;
    }
    else if ((lhs_size == 1 || rhs_size == 1) && !is_comparison) {
        Value* temp_lhs = nullptr;
        Value* temp_rhs = nullptr;

        if (lhs_size == 1){
            temp_rhs = lhs;
            Value* load = builder->CreateLoad(rhs);
            temp_lhs = builder->CreateAlloca(load->getType(), nullptr, "temp_lhs");
            builder->CreateStore(load, temp_lhs);
            lhs_size = rhs_size;
            rhs_size = 1;
        } else {
            temp_rhs = rhs;
            Value* load = builder->CreateLoad(lhs);
            temp_lhs = builder->CreateAlloca(load->getType(), nullptr, "temp_lhs");
            builder->CreateStore(load, temp_lhs);
        }

        for (int i = 0; i < lhs_size; i++){
            Value* temp_index = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, i));

            llvm::Value *i32zero = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, 0));
            llvm::Value *indices[2] = {i32zero, temp_index};

            llvm::Value* varInst = builder->CreateInBoundsGEP(temp_lhs, llvm::ArrayRef<llvm::Value *>(indices, 2));
            int temp = 1;
            Value* op = operation_block(floating_op, builder->CreateLoad(varInst), temp, temp_rhs, 1);
            builder->CreateStore(op, varInst);
        }

        return temp_lhs;
    }
    else if (lhs_size == rhs_size && !is_comparison){


        Value *temp_rhs = rhs;
        Value *load = builder->CreateLoad(lhs);
        Value *temp_lhs = builder->CreateAlloca(load->getType(), nullptr, "temp_lhs");
        builder->CreateStore(load, temp_lhs);

        for (int i = 0; i < lhs_size; i++) {
            Value *temp_index = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, i));

            llvm::Value *i32zero = llvm::ConstantInt::get(m->getContext(), llvm::APInt(8, 0));
            llvm::Value *indices[2] = {i32zero, temp_index};

            llvm::Value *varInst_l = builder->CreateInBoundsGEP(temp_lhs,
                                                                llvm::ArrayRef<llvm::Value *>(indices, 2));
            llvm::Value *varInst_r = builder->CreateInBoundsGEP(temp_rhs,
                                                                llvm::ArrayRef<llvm::Value *>(indices, 2));

            int temp = 1;
            Value *op = operation_block(floating_op, builder->CreateLoad(varInst_l), temp,
                                        builder->CreateLoad(varInst_r), 1);
            builder->CreateStore(op, varInst_l);
        }

        return temp_lhs;

    }
    else if (is_comparison){
        std::vector<Value *> printArgs;

        printArgs.push_back(lhs);
        printArgs.push_back(rhs);
        return builder->CreateCall(m->getFunction("stringCompare"), {lhs, rhs});
//        vector<Value*> newArgs = {lhs,rhs};
//        return builder->CreateRet(builder->CreateCall(m->getFunction("stringCompareLLVM"), newArgs));
//        return codegen_literal_boolean(false);
    }

    return nullptr;

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
//    globalDeclaration->setInitializer(llvm::ConstantArray::get_temp(stringType, chars));
//    globalDeclaration->setConstant(true);
//    globalDeclaration->setLinkage(llvm::GlobalValue::LinkageTypes::PrivateLinkage);
//    globalDeclaration->setUnnamedAddr (llvm::GlobalValue::UnnamedAddr::Global);



    //4. Return a cast to an i8*
//    return llvm::ConstantExpr::getBitCast(chars, charType->getPointerTo());
    return ConstantArray::get(arrayType, chars);
}

