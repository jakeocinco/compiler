//
// Created by Jacob Carlson on 4/6/21.
//

#ifndef COMPILER_5183_CODE_GENERATION_H
#define COMPILER_5183_CODE_GENERATION_H

//#include "llvm/Pass.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
//#include "llvm/IR/PassManager.h"
//#include "llvm/IR/CallingConv.h"
//#include "llvm/IR/Verifier.h"
//#include "llvm/IR/IRPrintingPasses.h"

//#include "llvm/Bitcode/BitcodeWriter.h"
//#include "llvm/ADT/APFloat.h"
//#include "llvm/ADT/Optional.h"
//#include "llvm/ADT/STLExtras.h"
//#include "llvm/IR/BasicBlock.h"
//#include "llvm/IR/Constants.h"
//#include "llvm/IR/DerivedTypes.h"
//#include "llvm/IR/Function.h"
//#include "llvm/IR/Instructions.h"
//#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/LegacyPassManager.h"
//#include "llvm/IR/Module.h"
//#include "llvm/IR/Type.h"
//#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "../Parser/node.h"
#include "scope.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace llvm;
using namespace llvm::sys;

static LLVMContext context;
static IRBuilder<> builder(context);
static std::unique_ptr<Module> module;
static std::map<std::string, Value *> namedValues;

class code_generation {
public:
    code_generation(std::string file_text);
private:

    Module* m;
    LLVMContext context;
    IRBuilder<>* builder;
    BasicBlock* block;

    TargetMachine* targetMachine;

    std::map<std::string, Function*> functions;
//    ()(llvm::Value *, llvm::Value *, const llvm::Twine &, bool, bool
    typedef  Value* (llvm::IRBuilderBase::*memFn)(llvm::Value *, llvm::Value *, const llvm::Twine &);

    struct _operator_block {
        int type; // one of the token codes from above
        std::function<bool()> standard_op;
        std::function<bool()> floating_op;
        Value* lhs;
        Value* rhs;
    };


    node* tree;
    scope* variable_scope;

    std::map<std::string,Value*> identifiers;
    std::map<std::string,Type*> identifier_types;

    IRBuilder<>* b2;
    Value* codegen(node* n);

    /** Program **/
    Module* codegen_program_root(node* n);
    Function* codegen_program_root_function();
    void codegen_declaration_block(node* n);
    bool codegen_statement_block(node* n);

    /** Control Blocks **/
    void codegen_if_statement(node* n);
    void codegen_for_statement(node* n);
    void codegen_return_statement(node* n);
    void codegen_function_prototype(node* n);

    /** Variables **/
    void codegen_variable_declaration(node* n);
    void codegen_variable_assignment(node* n);

    /** Literals **/
    Value* codegen_literal_integer(int n);
    Value* codegen_literal_float(double n);
    Value* codegen_literal_boolean(bool n);
    Value* codegen_literal_string(const std::string& n, int& size);

    /** Expressions **/
    Value* codegen_expression(node *n, int& size, Value* lhs = nullptr);
    Value* codegen_arith_op(node *n, int& size, Value* lhs = nullptr);
    Value* codegen_relation(node *n, int& size, Value* lhs = nullptr);
    Value* codegen_term(node *n, int& size, Value* lhs = nullptr);
    Value* codegen_factor(node *n, int& size);

    /** Run Time Functions **/
    void codegen_run_time_prototypes();
    Value* codegen_print_base(Value* v, Value* formatStr);
    Value* codegen_scan_base(Type* t, Value* formatStr);
    Value* codegen_print_integer(Value* v);
    Value* codegen_scan_integer();
    Value* codegen_print_double(Value* v);
    Value* codegen_scan_double();
    Value* codegen_print_boolean(Value* v);
    Value* codegen_scan_bool();
    Value* codegen_print_string(Value* v);
    Value* codegen_scan_string();
    Value* codegen_sqrt(Value* v);

    /** Helpers **/
    Value* operation_block(const std::function<Value*(Value* lhs, Value* rhs)>& floating_op,
                           Value* lhs, int &lhs_size,
                           Value* rhs,  int rhs_size,
                           bool is_comparison = false, bool neg = false);
    Type* get_type(node *n);
    node* get_reserve_node(node* n, int type);

    /** system Code **/
    void initialize_for_target();
    void print_module_ll(bool should_print);
    void write_module_to_file(std::string file_name);

    /** Errors **/
    void throw_runtime_template(const std::string& message) const;
};


#endif //COMPILER_5183_CODE_GENERATION_H
