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

    print_module_ll(true);
    write_module_to_file("output.o");

}

/** Program **/
Module* code_generation::codegen_program_root(node *n) {

    get_reserve_node(n,T_PROGRAM); // Popping program
    const string program_name = string("program_") + get_reserve_node(n,T_IDENTIFIER)->val.stringValue;
    get_reserve_node(n,T_IS); // Popping is

    m = new Module(program_name, context);

    initialize_for_target();
    variable_scope = new scope(builder, m);

    Function* f = codegen_program_root_function();

    BasicBlock* program_entry_bb = BasicBlock::Create(context, "entry", f);
    builder->SetInsertPoint(program_entry_bb);
    block = program_entry_bb;

    // Runtime/Built-in prototypes
    codegen_run_time_prototypes();

    codegen_declaration_block(n->children.front());
    n->children.pop_front(); // ignoring declaration block for now

    get_reserve_node(n,T_BEGIN); // Popping program

    codegen_statement_block(n->children.front());
    n->children.pop_front();

    builder->CreateRet(ConstantInt::get(context, APInt(32,0)));

    return m;
}
Function* code_generation::codegen_program_root_function(){
    std::vector<std::string> args = {};
    std::string name = "main";

    std::vector<Type*> Integers(args.size(),
                                Type::getDoubleTy(context));
    FunctionType *FT =
            FunctionType::get(Type::getInt32Ty(context), Integers, false);

    Function *f =
            Function::Create(FT, Function::ExternalLinkage, name, m);

    unsigned Idx = 0;
    for (auto &Arg : f->args())
        Arg.setName(args[Idx++]);

    return f;
}
void code_generation::codegen_declaration_block(node *n) {
    for (node* x : n->children){
        if (x->type == T_VARIABLE_DECLARATION) codegen_variable_declaration(x);
        else if (x->type == T_PROCEDURE_DECLARATION) codegen_function_prototype(x);
    }
}
bool code_generation::codegen_statement_block(node *n) {
    bool parse_return = false;
    for (node* x : n->children){
        if (x->type == T_VARIABLE_ASSIGNMENT) codegen_variable_assignment(x);
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

/** Control Blocks **/
void code_generation::codegen_if_statement(node* n) {
    get_reserve_node(n,T_IF);
    int bool_size;
    Value* condition = codegen_expression(get_reserve_node(n,T_EXPRESSION), bool_size);
    get_reserve_node(n,T_THEN);

    Function* parent_function = builder->GetInsertBlock()->getParent();
    BasicBlock* then_block = BasicBlock::Create(context, "then", parent_function);
    BasicBlock* else_block = BasicBlock::Create(context, "else");
    BasicBlock* cont_block = BasicBlock::Create(context, "ifcont");

    builder->CreateCondBr(condition, then_block, else_block);
    builder->SetInsertPoint(then_block);

    codegen_statement_block(n->children.front());
    n->children.pop_front();
    builder->CreateBr(cont_block);
    then_block = builder->GetInsertBlock();

    parent_function->getBasicBlockList().push_back(else_block);
    builder->SetInsertPoint(else_block);

    if (n->children.front()->type == T_ELSE){
        get_reserve_node(n,T_ELSE);

        codegen_statement_block(n->children.front());
        n->children.pop_front();
    }

    builder->CreateBr(cont_block);
    else_block = builder->GetInsertBlock();

    parent_function->getBasicBlockList().push_back(cont_block);
    builder->SetInsertPoint(cont_block);

    get_reserve_node(n,T_END);
    get_reserve_node(n,T_IF);
}
void code_generation::codegen_for_statement(node *n) {
    get_reserve_node(n,T_FOR);
    codegen_variable_assignment(get_reserve_node(n,T_VARIABLE_ASSIGNMENT));

    node* conditional = get_reserve_node(n,T_EXPRESSION);
    node* conditional_copy = new node(conditional); // copying conditional
    int cond_size;
    Value* start_conditional = codegen_expression(conditional,cond_size);

    Function* parent_function = builder->GetInsertBlock()->getParent();
    BasicBlock* loop_block = BasicBlock::Create(context, "loop", parent_function);
    BasicBlock* cont_block = BasicBlock::Create(context, "forcont");

    builder->CreateCondBr(start_conditional, loop_block, cont_block);
    builder->SetInsertPoint(loop_block);

    codegen_statement_block(get_reserve_node(n,T_FOR_LOOP_STATEMENT_BLOCK));

    Value *endVal = codegen_expression(conditional_copy, cond_size); // codegen for end expression
    builder->CreateCondBr(endVal, loop_block, cont_block);
    get_reserve_node(n,T_END);
    get_reserve_node(n,T_FOR);

    parent_function->getBasicBlockList().push_back(cont_block);
    builder->SetInsertPoint(cont_block);
}
void code_generation::codegen_return_statement(node *n) {
    // TODO - type checking?
    get_reserve_node(n,T_RETURN);// Popping return
    int cond_size;
    builder->CreateRet(codegen_expression(get_reserve_node(n,T_EXPRESSION), cond_size));
}
void code_generation::codegen_function_prototype(node *n) {

    get_reserve_node(n,T_PROCEDURE);
    std::string name = get_reserve_node(n,T_IDENTIFIER)->val.stringValue;
    Type* return_type = get_type(get_reserve_node(n,T_TYPE_MARK));

    std::vector<std::string> arg_names;
    std::vector<Type *> arg_types;
    for (auto arg : get_reserve_node(n,T_PARAMETER_LIST)->children){
        get_reserve_node(arg,T_VARIABLE);
        arg_names.emplace_back(get_reserve_node(arg,T_IDENTIFIER)->val.stringValue);
        get_reserve_node(arg,T_COLON);
        arg_types.push_back(get_type(get_reserve_node(arg,T_TYPE_MARK)));
    }

    FunctionType* f_type = FunctionType::get(return_type, arg_types, false);
    // Have to add the function to scope as Value*
    Value* f_temp = Function::Create(f_type, Function::ExternalLinkage, name, m);

    variable_scope->add(name, f_temp, f_temp->getType(), variable_inst::VARIABLE_CLASS::FUNCTION);
    variable_scope = new scope(variable_scope);
    variable_scope->add(name, f_temp, f_temp->getType(), variable_inst::VARIABLE_CLASS::FUNCTION);

    // Have to cast function back to Function*
    auto* f = cast<Function>(f_temp);

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : f->args())
        Arg.setName(arg_names[Idx++]);

    BasicBlock* func_block = BasicBlock::Create(context, "entry", f);
    BasicBlock* current_block = builder->GetInsertBlock();

    builder->SetInsertPoint(func_block);

    for (auto &Arg : f->args()){
        variable_scope->add(std::string(Arg.getName()), &Arg, (&Arg)->getType(), variable_inst::VARIABLE_CLASS::VALUE);
    }


    codegen_declaration_block(get_reserve_node(n,T_PROCEDURE_DECLARATION_BLOCK));

    get_reserve_node(n,T_BEGIN);

    if (!codegen_statement_block(get_reserve_node(n,T_PROCEDURE_STATEMENT_BLOCK))){
        throw runtime_error(string("Procedure '") + name + "' must contain a return statement.");
    }

    builder->SetInsertPoint(current_block);

    variable_scope = variable_scope->get_parent();
    get_reserve_node(n,T_END);
    get_reserve_node(n,T_PROCEDURE);
}

/** Variables **/
void code_generation::codegen_variable_declaration(node *n) {
    bool is_global = false;
    if (n->children.front()->type == T_GLOBAL) {
        is_global = true;
        get_reserve_node(n,T_GLOBAL);
    }

    get_reserve_node(n,T_VARIABLE);
    string s = get_reserve_node(n,T_IDENTIFIER)->val.stringValue;
    get_reserve_node(n,T_COLON);

    Type* type = get_type(get_reserve_node(n,T_TYPE_MARK));
    Type* element_type = type;

    int size = 1;
    variable_inst::VARIABLE_CLASS clazz = variable_inst::VARIABLE_CLASS::INSTANCE;

    if (!n->children.empty()) {
        get_reserve_node(n,T_LBRACKET);

        int size_size;
        size = cast<ConstantInt>(codegen_expression(get_reserve_node(n,T_EXPRESSION), size_size))->getZExtValue();
        get_reserve_node(n,T_RBRACKET);

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
    }
    else {
        variable_ptr = builder->CreateAlloca(type, nullptr, s);
    }

    variable_scope->add(s, variable_ptr, element_type, clazz, size);
}
void code_generation::codegen_variable_assignment(node *n) {

    string s = get_reserve_node(n,T_IDENTIFIER)->val.stringValue;

    Value* index = nullptr;

    bool is_array_index = false;

    // This stuff only matters if is_array_index = true;
    Function* parent_function = nullptr;
    BasicBlock* valid_index_block = nullptr;
    BasicBlock* invalid_index_block = nullptr;
    BasicBlock* cont_block = nullptr;

    if (n->children.front()->type == T_LBRACKET){
        is_array_index = true;
        get_reserve_node(n,T_LBRACKET);

        int temp_size;
        index = codegen_expression(get_reserve_node(n,T_EXPRESSION), temp_size);
        get_reserve_node(n,T_RBRACKET);

        Value* lhs = variable_scope->get_temp(s + "_size")->get();
        Value* cond = builder->CreateICmpSGT(lhs, index);

        parent_function = builder->GetInsertBlock()->getParent();
        valid_index_block = BasicBlock::Create(context, "validIndex", parent_function);
        invalid_index_block = BasicBlock::Create(context, "invalidIndex");
        cont_block = BasicBlock::Create(context, "ifcont");

        builder->CreateCondBr(cond, valid_index_block, invalid_index_block);
        builder->SetInsertPoint(valid_index_block);

    }

    get_reserve_node(n,T_COLON_EQUALS);

    int exp_size;
    Value* v =  codegen_expression(get_reserve_node(n,T_EXPRESSION),exp_size);
    variable_scope->set(s, v,exp_size, index);

    if (is_array_index){
        builder->CreateBr(cont_block);
        valid_index_block = builder->GetInsertBlock();

        parent_function->getBasicBlockList().push_back(invalid_index_block);
        builder->SetInsertPoint(invalid_index_block);

        // TODO - throw runtime in llvm
        int temp = 1;
        codegen_print_string(codegen_literal_string("Invalid array index", temp));

        builder->CreateBr(cont_block);
        invalid_index_block = builder->GetInsertBlock();

        parent_function->getBasicBlockList().push_back(cont_block);
        builder->SetInsertPoint(cont_block);
    }

}

/** Literals **/
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

/** Expressions **/
Value *code_generation::codegen_expression(node *n, int& size,  Value* lhs) {

    bool is_not_l = false;
    if (!n->children.empty() && n->children.front()->type == T_NOT){
        get_reserve_node(n,T_NOT);
        is_not_l = true;
    }

    if (lhs == nullptr){
        lhs = is_not_l ?
                builder->CreateNot(codegen_arith_op(get_reserve_node(n,T_ARITH_OP), size)) :
                codegen_arith_op(get_reserve_node(n,T_ARITH_OP),size);
    }

    if (n->children.empty())
        return lhs;

    int operation = n->children.front()->type;
    n->children.pop_front(); // Pop sign

    bool is_not_r = false;
    if (n->children.front()->type == T_NOT){
        get_reserve_node(n,T_NOT);
        is_not_r = true;
    }
    int rhs_size;
    Value* rhs = is_not_r ?
            builder->CreateNot(codegen_arith_op(get_reserve_node(n,T_ARITH_OP),rhs_size)) :
            codegen_arith_op(get_reserve_node(n,T_ARITH_OP),rhs_size);

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
        return codegen_relation(get_reserve_node(n,T_RELATION), size);

    if (lhs == nullptr){
        lhs = codegen_relation(get_reserve_node(n,T_RELATION), size);
    }

    int operation = n->children.front()->type;
    n->children.pop_front();

    int rhs_size;
    Value* rhs = codegen_relation(get_reserve_node(n,T_RELATION), rhs_size);

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
        return codegen_term(get_reserve_node(n,T_TERM), size);

    if (lhs == nullptr){
        lhs = codegen_term(get_reserve_node(n,T_TERM), size);
    }
    int operation = n->children.front()->type;
    n->children.pop_front(); // Pop operation

    int rhs_size;
    Value* rhs = codegen_term(get_reserve_node(n,T_TERM), rhs_size);

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
        return codegen_factor(get_reserve_node(n,T_FACTOR), size);

    if (lhs == nullptr){
        lhs = codegen_factor(get_reserve_node(n,T_FACTOR), size);
    }

    int operation = n->children.front()->type;
    n->children.pop_front();

    int rhs_size;
    Value* rhs = codegen_factor(get_reserve_node(n,T_FACTOR),rhs_size);

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
        }
        else {
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
            codegen_print_string(codegen_literal_string("Invalid array index", temp_size));

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
            return_val = codegen_print_integer(codegen_expression(x->children.front(), temp_size));
        }
        else if (string("putfloat") == functionName){
            return_val = codegen_print_double(codegen_expression(x->children.front(), temp_size));
        }
        else if (string("putstring") == functionName) {
            return_val = codegen_print_string(codegen_expression(x->children.front(), temp_size));
        }
        else if (string("putbool") == functionName) {
            return_val = codegen_print_boolean(codegen_expression(x->children.front(), temp_size));
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

/** Run Time Functions **/
void code_generation::codegen_run_time_prototypes() {

    Function* printer = m->getFunction("printf");
    if (printer == nullptr){
        std::vector<Type *> args;
        args.push_back(Type::getInt8PtrTy(context));

        FunctionType *printfType = FunctionType::get(builder->getInt32Ty(), args, true);
        Function::Create(printfType, Function::ExternalLinkage, "printf", m);
    }

    Function* scan = m->getFunction("scanf");
    if (scan == nullptr){
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
}
Value* code_generation::codegen_print_base(Value* v, Value* formatStr) {
    std::vector<Value *> printArgs;
    printArgs.push_back(formatStr);
    printArgs.push_back(v);

    builder->CreateCall(m->getFunction("printf"), printArgs);
    return codegen_literal_boolean(true);
}
Value *code_generation::codegen_scan_base(Type* t, Value *formatStr) {
    AllocaInst* tempInst = builder->CreateAlloca(t, 0, "temp");
    std::vector<Value *> printArgs;

    printArgs.push_back(formatStr);
    printArgs.push_back(tempInst);

    builder->CreateCall(m->getFunction("scanf"), printArgs);
    return builder->CreateLoad(tempInst);
}
Value* code_generation::codegen_print_integer(Value *v) {
    if (!namedValues.contains(".int")){
        Value *formatStr = builder->CreateGlobalStringPtr("%d\n", ".int");
        namedValues.insert_or_assign(".int", formatStr);
    }
    return codegen_print_base(v, namedValues.at(".int"));
}
Value *code_generation::codegen_scan_integer() {
    if (!namedValues.contains(".integer_sc")){
        Value *formatStr = builder->CreateGlobalStringPtr("%u", ".integer_sc");
        namedValues.insert_or_assign(".integer_sc", formatStr);
    }
    return codegen_scan_base(Type::getInt32Ty(context),namedValues.at(".integer_sc"));
}
Value* code_generation::codegen_print_double(Value *v) {
    if (!namedValues.contains(".double")){
        Value *formatStr = builder->CreateGlobalStringPtr("%g\n", ".double");
        namedValues.insert_or_assign(".double", formatStr);
    }
    return codegen_print_base(v, namedValues.at(".double"));
}
Value *code_generation::codegen_scan_double() {
    if (!namedValues.contains(".double_sc")){
        Value *formatStr = builder->CreateGlobalStringPtr("%lf", ".double_sc");
        namedValues.insert_or_assign(".double_sc", formatStr);
    }
    return codegen_scan_base(Type::getDoubleTy(context),namedValues.at(".double_sc"));
}
Value* code_generation::codegen_print_boolean(Value *v) {
    if (!namedValues.contains(".int")){
        Value *formatStr = builder->CreateGlobalStringPtr("%d\n", ".int");
        namedValues.insert_or_assign(".int", formatStr);
    }
    // change integers to actually be ints ... maybe
    return codegen_print_base(v, namedValues.at(".int"));
}
Value *code_generation::codegen_scan_bool() {
    if (!namedValues.contains(".integer_sc")){
        Value *formatStr = builder->CreateGlobalStringPtr("%u", ".integer_sc");
        namedValues.insert_or_assign(".integer_sc", formatStr);
    }
    return codegen_scan_base(Type::getInt1Ty(context),namedValues.at(".integer_sc"));
}
Value* code_generation::codegen_print_string(Value *v) {
    if (!namedValues.contains(".str")){
        Value *formatStr = builder->CreateGlobalStringPtr("%s\n", ".str");
        namedValues.insert_or_assign(".str", formatStr);
    }
    return codegen_print_base(v, namedValues.at(".str"));
}
Value *code_generation::codegen_scan_string() {
    std::vector<Value *> args;
    return builder->CreateCall(m->getFunction("scanString"), args);
}
Value *code_generation::codegen_sqrt(Value* v){
    std::vector<Value *> args;
    args.push_back(v);

    return builder->CreateCall(m->getFunction("sqrt"), args);
}

/** HELPERS **/
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
node *code_generation::get_reserve_node(node* n, int type) {
    if (n->children.front()->type == type){
        node* temp = n->children.front();
        n->children.pop_front();
        return temp;
    }
    throw_runtime_template("Expected type " + std::to_string(type) + ", but received " +  std::to_string(n->children.front()->type) + " instead.");
    return nullptr;
}

/** System Code **/
void code_generation::initialize_for_target() {
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
    targetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    m->setDataLayout(targetMachine->createDataLayout());
    m->setTargetTriple(TargetTriple);


    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        errs() << Error;
        cout << "ERROR";
    }
}
void code_generation::print_module_ll(bool should_print) {
    if (should_print){
        llvm::raw_ostream& output = llvm::outs();
        this->m->print(output, nullptr);
    }
}
void code_generation::write_module_to_file(string file_name) {

    std::error_code EC;
    raw_fd_ostream dest(file_name, EC, sys::fs::OF_None);

    if (EC) {
        errs() << "Could not open file: " << EC.message();;
    }

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        errs() << "TargetMachine can't emit a file of this type";
    }

    pass.run(*m);
    dest.flush();
}

/** Errors **/
void code_generation::throw_runtime_template(const string &message) const {
        throw runtime_error("Codegegen Error: " + message);
}







