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



//    if (this->tree->type == T_PROGRAM_ROOT)
    Module* m = codegen_program_root(this->tree);
//    else
//        std::cout << "Not program root" << endl;
     m->print(llvm::errs(), nullptr);
//    write_to_file(m);
}


Value *code_generation::codegen(node* n) {
    if (n->type == T_INTEGER_LITERAL) return codegen_literal_integer(n);
    if (n->type == T_FLOAT_LITERAL) return codegen_literal_float(n);
//    if (n->type == T)
    return nullptr;
}

Module* code_generation::codegen_program_root(node *n) {

//
    Module* m = new Module("hello", context);
    Function* f = m->getFunction("mul_add");
    if (!f)
        f = codegen_function(n,m);

    BasicBlock *BB = BasicBlock::Create(context, "entry", f);
    builder.SetInsertPoint(BB);

    namedValues.clear();
    for (auto &Arg : f->args())
        namedValues.insert_or_assign(Arg.getName().str(), &Arg);

    if (Value* returnVal = codegen_function_body(nullptr)){
        builder.CreateRet(returnVal);
        verifyFunction(*f);
        return m;
    }
    return nullptr;
}

Function *code_generation::codegen_function(node *n, Module* m) {

    std::vector<std::string> args = {"x","y","z"};
    std::string name = "Jake";

    std::vector<Type*> Doubles(args.size(),
                               Type::getDoubleTy(context));
    FunctionType *FT =
            FunctionType::get(Type::getDoubleTy(context), Doubles, false);

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
//    return nullptr;
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

void code_generation::write_to_file(Module* m) {
////    InitializeModuleAndPassManager();
    // Initialize the target registry etc.
//    InitializeAllTargetInfos();
//    InitializeAllTargets();
//    InitializeAllTargetMCs();
//    InitializeAllAsmParsers();
//    InitializeAllAsmPrinters();

    auto TargetTriple = sys::getDefaultTargetTriple();
    m->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        errs() << Error;
        return;
    }

    auto CPU = "generic";
    auto Features = "";

//    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
//    auto TheTargetMachine =Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

//    m->setDataLayout(TheTargetMachine->createDataLayout());

    auto Filename = "output.o";
    std::error_code EC;
    raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

    if (EC) {
        errs() << "Could not open file: " << EC.message();
        return;
    }

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

//    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
//        errs() << "TargetMachine can't emit a file of this type";
//        return;
//    }

    pass.run(*m);
    dest.flush();
}








