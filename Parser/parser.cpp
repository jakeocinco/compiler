//
// Created by Jacob Carlson on 2/10/21.
//

#include "parser.h"
#include "../tokenCodes.h"

#include <iostream>
#include <regex>

using namespace std;
using namespace std::placeholders;

parser::parser(string file_text) {

    scan = new scanner(file_text + ' ');
    current_table = new symbol_table();

    current = scan->get_next_token();
    next = scan->get_next_token();

    head = parse_program();

    print_node_to_json(head);
}

void parser::consume_token() {
    current = next;
    if (next.type != T_END_OF_FILE)
        next = scan->get_next_token();
}

/** Program **/
node* parser::parse_program() {
    node* n = new node(T_PROGRAM_ROOT);

    n->newChild(expecting_reserved_word(T_PROGRAM, "program"));
    n->newChild(expecting_identifier());
    n->newChild(expecting_reserved_word(T_IS, "is"));

    n->newChild(parse_program_declaration_block());

    n->newChild(expecting_reserved_word(T_BEGIN, "begin"));
    std::cout << "\nPROGRAM LEVEL" << endl;
    this->current_table->print(true);
    n->newChild(parse_program_statement_block());

    n->newChild(expecting_reserved_word(T_END, "end"));
    n->newChild(expecting_reserved_word(T_PROGRAM, "program"));

    return n;
}
node* parser::parse_program_declaration_block() {

    node* n = new node(T_PROGRAM_DECLARATION_BLOCK);

    while (current.type != T_BEGIN && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_declaration(n); });
        functionList.emplace_back([this, n] { return process_procedure_declaration(n); });

        run_processes_until_true(n, functionList);
    }

    if (n->children.empty())
        return nullptr;
    return n;
}
node* parser::parse_program_statement_block(){

    node* n = new node(T_PROGRAM_STATEMENT_BLOCK);

    while (current.type != T_END && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_assignment(n); });
        functionList.emplace_back([this, n] { return process_if_block(n); });
        functionList.emplace_back([this, n] { return process_for_block(n); });

        run_processes_until_true(n, functionList);
    }

    if(n->children.empty())
        return nullptr;
    return n;
}

/** Procedures **/
node* parser::parse_procedure() {

    node* n = new node(T_PROCEDURE_DECLARATION);

    if (current.type == T_GLOBAL)
        n->newChild(expecting_reserved_word(T_GLOBAL, "global"));

    n->newChild(expecting_reserved_word(T_PROCEDURE, "procedure"));

    // TODO
    const string proc_name = current.val.stringValue;
    n->newChild(expecting_identifier());

    n->newChild(expecting_reserved_word(T_COLON, ":"));

    const unsigned type_val = current.type;
    n->newChild(parse_type_mark());

    push_new_identifier_to_symbol_table(proc_name, type_to_literal(type_val));
    push_current_symbol_table();
    push_new_identifier_to_symbol_table(proc_name, type_to_literal(type_val));

    n->newChild(parse_procedure_parameter_list());

    n->newChild(parse_procedure_declaration_block());

    n->newChild(expecting_reserved_word(T_BEGIN, "begin"));

    n->newChild(parse_procedure_statement_block());

    n->newChild(expecting_reserved_word(T_END, "end"));
    n->newChild(expecting_reserved_word(T_PROCEDURE, "procedure"));
    std::cout << "\nPROCEDURE LEVEL | " << proc_name << "()" << endl;
    this->current_table->print(true);
    pop_current_symbol_table();
    return n;
}
node* parser::parse_procedure_declaration_block() {

    node *n = new node(T_PROCEDURE_DECLARATION_BLOCK);

    while (current.type != T_BEGIN && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_declaration(n); });
        functionList.emplace_back([this, n] { return process_procedure_declaration(n); });
//        functionList.emplace_back([this, n] { return process_type_declaration(n); });

        run_processes_until_true(n, functionList);
    }

    if (n->children.empty())
        return nullptr;
    return n;
}
node* parser::parse_procedure_statement_block(){

    node* n = new node(T_PROCEDURE_STATEMENT_BLOCK);

    while (current.type != T_END && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_assignment(n); });
        functionList.emplace_back([this, n] { return process_return_block(n); });
        functionList.emplace_back([this, n] { return process_if_block(n); });
        functionList.emplace_back([this, n] { return process_for_block(n); });

        run_processes_until_true(n, functionList);
    }

    if (n->children.empty())
        return nullptr;
    return n;
}
node* parser::parse_procedure_parameter_list(){

    bool isFirstRun = true;
    node* n = new node(T_PARAMETER_LIST);


    expecting_reserved_word(T_LPAREN, "(");
    while (current.type != T_RPAREN && current.type != T_END_OF_FILE) {
        if (isFirstRun){
            isFirstRun = false;
        } else {
            expecting_reserved_word(T_COMMA, ",");
        }
        n->newChild(parse_variable_declaration());
    }

    expecting_reserved_word(T_RPAREN, ")");

    if (n->children.empty())
        return nullptr;
    return n;
}
node* parser::parse_procedure_return_statement(){

    node* n = new node(T_RETURN_BLOCK);

    n->newChild(expecting_reserved_word(T_RETURN, "return"));
    unsigned variable_val = 0;
    n->newChild(parse_expression(variable_val));
    std::cout << "Variable Val: " << variable_val << " | line: " << current.line_number << std::endl;
    expecting_reserved_word(T_SEMICOLON, ";");

    return n;
}
node* parser::parse_procedure_call() {
    bool isFirstRun = true;
    node* n = new node(T_PROCEDURE_CALL);

    verify_identifier_is_declared(current.val.stringValue);
    n->newChild(expecting_identifier());
    expecting_reserved_word(T_LPAREN, "(");
    while (current.type != T_RPAREN && current.type != T_END_OF_FILE) {
        if (isFirstRun){
            isFirstRun = false;
        } else {
            expecting_reserved_word(T_COMMA, ",");
        }
        unsigned variable_val = 0;
        n->newChild(parse_expression(variable_val));
        std::cout << "Variable Val: " << variable_val << " | line: " << current.line_number << std::endl;
    }

    expecting_reserved_word(T_RPAREN, ")");

    if (n->children.empty())
        return nullptr;
    return n;
}

/** Built in Functionality **/
node* parser::parse_if_block() {
    node* n = new node(T_IF_BLOCK);

    n->newChild(expecting_reserved_word(T_IF, "if"));
    expecting_reserved_word(T_LPAREN, "(");
    unsigned variable_val = 0;
    n->newChild(parse_expression(variable_val));
    std::cout << "Variable Val: " << variable_val << " | line: " << current.line_number << std::endl;
    expecting_reserved_word(T_RPAREN, ")");
    n->newChild(expecting_reserved_word(T_THEN, "then"));
    n->newChild(parse_if_statement_block());

    if (current.type == T_ELSE){
        n->newChild(expecting_reserved_word(T_ELSE, "else"));
        n->newChild(parse_if_statement_block());
    }

    n->newChild(expecting_reserved_word(T_END, "end"));
    n->newChild(expecting_reserved_word(T_IF, "if"));

    return n;
}
node* parser::parse_if_statement_block(){
    node* n = new node(T_IF_STATEMENT_BLOCK);

    while (current.type != T_END && current.type != T_ELSE && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_assignment(n); });
        functionList.emplace_back([this, n] { return process_return_block(n); });
        functionList.emplace_back([this, n] { return process_if_block(n); });
        functionList.emplace_back([this, n] { return process_for_block(n); });

        run_processes_until_true(n, functionList);
    }

    if (n->children.empty())
        return nullptr;
    return n;
}

/** For Loops **/
node* parser::parse_for_loop() {
    node* n = new node(T_FOR_LOOP);

    n->newChild(expecting_reserved_word(T_FOR, "for"));
    expecting_reserved_word(T_LPAREN, "(");
    n->newChild(parse_variable_assignment());
    expecting_reserved_word(T_SEMICOLON, ";");

    unsigned variable_val = 0;
    n->newChild(parse_expression(variable_val));
    std::cout << "Variable Val: " << variable_val << " | line: " << current.line_number << std::endl;
    expecting_reserved_word(T_RPAREN, ")");

    n->newChild(parse_for_loop_statement_block());

    n->newChild(expecting_reserved_word(T_END, "end"));
    n->newChild(expecting_reserved_word(T_FOR, "for"));

    return n;
}
node* parser::parse_for_loop_statement_block(){
    node* n = new node(T_FOR_LOOP_STATEMENT_BLOCK);

    while (current.type != T_END && current.type != T_ELSE && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_assignment(n); });
        functionList.emplace_back([this, n] { return process_return_block(n); });
        functionList.emplace_back([this, n] { return process_if_block(n); });
        functionList.emplace_back([this, n] { return process_for_block(n); });

        run_processes_until_true(n, functionList);
    }

    if (n->children.empty())
        return nullptr;
    return n;
}

/** Expressions **/
node* parser::parse_expression(unsigned& code, node* n ){

    if (n == nullptr)
        n = new node(T_EXPRESSION);

    if (current.type == T_NOT){
        n->newChild(expecting_reserved_word(T_NOT, "not"));
        n->newChild(parse_arith(code));
        code = T_RELATION;
    } else {
        n->newChild(parse_arith(code));
        if (current.type == T_AND || current.type == T_OR){
            n->newChild(expecting_reserved_word(current.type, current.val.stringValue));
            parse_expression(code, n);
            code = T_RELATION;
        }
    }

    return n;
}
node* parser::parse_arith(unsigned& code, node* n){

    if (n == nullptr)
        n = new node(T_ARITH_OP);

    unsigned c1 = 0;
    n->newChild(parse_relation(c1));
    if (current.type == T_ADD || current.type == T_MINUS){
        n->newChild(expecting_reserved_word(current.type, current.val.stringValue));
        unsigned c2 = 0;
        parse_arith(c2, n);
        if (!are_types_valid_to_combine(code, c1, c2)){
            throw_runtime_template("Invalid types: " + std::to_string(c1) + " and " + std::to_string(c2) + ".");
        }
    } else {
        code = c1;
    }
    return n;
}
node* parser::parse_relation(unsigned& code, node *n) {

    if (n == nullptr)
        n = new node(T_RELATION);

    unsigned c1 = 0;
    n->newChild(parse_term(c1));
    if (is_current_relational_operator()){
        n->newChild(expecting_reserved_word(current.type, current.val.stringValue));
        unsigned c2 = 0;
        parse_relation(c2, n);
        if (!are_types_valid_to_combine(code, c1, c2)){
            throw_runtime_template("Invalid types: " + std::to_string(c1) + " and " + std::to_string(c2) + ".");
        } else {
            code = T_RELATION;
        }
    } else {
        code = c1;
    }
    return n;
}
node* parser::parse_term(unsigned& code, node* n){

    if (n == nullptr)
        n = new node(T_TERM);

    unsigned c1 = 0;
    n->newChild(parse_factor(c1));
    if (current.type == T_MULTIPLY || current.type == T_DIVIDE){
        n->newChild(expecting_reserved_word(current.type, current.val.stringValue));

        unsigned c2 = 0;
        parse_term(c2, n);
        if (!are_types_valid_to_combine(code, c1, c2)){
            throw_runtime_template("Invalid types: " + std::to_string(c1) + " and " + std::to_string(c2) + ".");
        }
    } else {
        code = c1;
    }
    return n;
}
node* parser::parse_factor(unsigned& code){

    node* n = new node(T_FACTOR);
    bool isNegative = false;

    if (current.type == T_LPAREN){
        expecting_reserved_word(T_LPAREN,"(");
        n->newChild(parse_expression(code));
        expecting_reserved_word(T_RPAREN,")");

        return n;
    }

    if (current.type == T_MINUS){
        n->newChild(expecting_reserved_word(T_MINUS, "-"));
        isNegative = true;
    }

    if (current.type == T_STRING_LITERAL || current.type == T_FALSE || current.type == T_TRUE){
        code = current.type;
        if (!isNegative){
            n->newChild(expecting_literal(current.type));
        } else {
            throw_unexpected_token("variable or number", current.val.stringValue);
        }
        return n;
    }

    if (current.type == T_IDENTIFIER){
        code = verify_identifier_is_declared(current.val.stringValue);
        if (next.type == T_LPAREN && !isNegative){
            n->newChild(parse_procedure_call());
        } else {
            n->newChild(expecting_identifier());
        }
        return n;
    }

    if (current.type == T_INTEGER_LITERAL || current.type == T_FLOAT_LITERAL){
        code = current.type;
        n->newChild(expecting_literal(current.type));
        return n;
    }

    throw_runtime_template("Never got valid factor");
    return nullptr;
}

/** Variables **/
node* parser::parse_variable_declaration() {
    /** TODO: ADD TO TABLE **/
    node* n = new node(T_VARIABLE_DECLARATION);

    if (current.type == T_GLOBAL)
        n->newChild(expecting_reserved_word(T_GLOBAL, "global"));

    n->newChild(expecting_reserved_word(T_VARIABLE, "variable"));
    const string identifier_name = current.val.stringValue;
    n->newChild(expecting_identifier());
    n->newChild(expecting_reserved_word(T_COLON, ":"));
    const unsigned type_val = current.type;
    n->newChild(parse_type_mark());

    push_new_identifier_to_symbol_table(identifier_name, type_to_literal(type_val));
    if (current.type == T_LBRACKET){
        n->newChild(expecting_reserved_word(T_LBRACKET, "["));
        n->newChild(expecting_literal(T_INTEGER_LITERAL));
        n->newChild(expecting_reserved_word(T_RBRACKET, "]"));
    }


    return n;
}
node* parser::parse_variable_assignment() {
    node* n = new node(T_VARIABLE_ASSIGNMENT);

    verify_identifier_is_declared(current.val.stringValue);
    n->newChild(expecting_identifier());

    if (T_LBRACKET == current.type){
        n->newChild(expecting_reserved_word(T_LBRACKET, "["));
        n->newChild(expecting_literal(T_INTEGER_LITERAL));
        n->newChild(expecting_reserved_word(T_RBRACKET, "]"));
    }
    // Can stuff like this be stripped yet -- kinda like semi colons
    n->newChild(expecting_reserved_word(T_COLON_EQUALS, ":="));
    unsigned variable_val = 0;
    n->newChild(parse_expression(variable_val));
    std::cout << "Variable Val: " << variable_val << " | line: " << current.line_number << std::endl;

    return n;
}

/** Types **/
node* parser::parse_type_mark(){

    node* n = new node(T_TYPE_DEF);

    if (current.type == T_IDENTIFIER){
        // TODO - check to make sure type exists
        n->newChild(expecting_identifier());
    } else {
        n->newChild(expecting_predefined_type());
    }

    return n;
}

/** Block Comments **/
node* parser::parse_block_comments() {
    if (current.type == T_BLOCK_COMMENT_OPEN){
        unsigned line_number = current.line_number;
        while( current.type != T_BLOCK_COMMENT_CLOSE) {
            consume_token();
            if (current.type == T_BLOCK_COMMENT_OPEN){
                parse_block_comments();
            }
            if (current.type == T_END_OF_FILE){
                throw_runtime_template("Block comment not closed. Opening comment block (/*) of of unclosed block at " + std::to_string(line_number) + ".");
            }
        }
        expecting_reserved_word(T_BLOCK_COMMENT_CLOSE, "/*");
    }
    return nullptr;
}

/** Helpers and Code Reuse **/
void parser::run_processes_until_true(node* n, std::list<std::function<bool()>> ll) {
    while (!ll.empty()){
        if (ll.front()()){
            return;
        }
        ll.pop_front();
    }
    throw_runtime_template("No valid processes were found.");
}
bool parser::is_current_relational_operator() const {
    return current.type >= T_L_THAN && current.type <= T_N_EQUALS;
}
bool parser::are_types_valid_to_combine(unsigned& code, unsigned c1, unsigned c2, bool first_run)  const {
    if (c1 == c2){
        code = c1;
        return true;
    } else if ((c1 == T_INTEGER_LITERAL && c2 == T_TRUE) ||
            (c1 == T_INTEGER_LITERAL && c2 == T_FALSE)){
        code = T_INTEGER_LITERAL;
        return true;
    } else if(c1 == T_INTEGER_LITERAL && c2 == T_FLOAT_LITERAL){
        code = T_FLOAT_LITERAL;
        return true;
    } else if (first_run){
        return are_types_valid_to_combine(code, c2, c1, false);
    }
    return false;
}
unsigned parser::type_to_literal(unsigned type){
    if (type == T_INTEGER_TYPE){
        return T_INTEGER_LITERAL;
    } else if (type == T_FLOAT_TYPE){
        return T_FLOAT_LITERAL;
    } else if (type == T_STRING_TYPE){
        return T_STRING_LITERAL;
    } else if (type == T_BOOL_TYPE){
        return T_TRUE;
    }
    return 0;
}

node*  parser::expecting_reserved_word(int expected_type, const string& expected_value) {
    if (current.type == expected_type){
        node* n = new node(expected_value, expected_type);
        consume_token();
        return n;
    }
    throw_unexpected_token(expected_value, current.val.stringValue);
    return nullptr;
}
node*  parser::expecting_identifier() {
    if (current.type == T_IDENTIFIER){
        node* n = new node(current.val.stringValue, T_IDENTIFIER);
        consume_token();
        return n;
    }
    throw_unexpected_reserved_word(current.val.stringValue);
    return nullptr;
}
node*  parser::expecting_predefined_type() {
    if (current.type == T_INTEGER_TYPE || current.type == T_FLOAT_TYPE ||
            current.type == T_STRING_TYPE || current.type == T_BOOL_TYPE){
        node* n = new node(current.val.stringValue, current.type);
        consume_token();
        return n;
    }
    throw_unexpected_token_wanted_type(current.val.stringValue);
    return nullptr;
}
node*  parser::expecting_literal(int expected_type) {
    if ((expected_type == T_INTEGER_LITERAL) || (expected_type < 0 && current.type == T_INTEGER_LITERAL)){
        node* n = node::create_integer_literal_node(current.val.intValue);
        consume_token();
        return n;
    } else if ((expected_type == T_FLOAT_LITERAL) || (expected_type < 0 && current.type == T_FLOAT_LITERAL)){
        node* n = node::create_double_literal_node(current.val.doubleValue);
        consume_token();
        return n;
    } else if ((expected_type == T_STRING_LITERAL) || (expected_type < 0 && current.type == T_STRING_LITERAL)){
        node* n = node::create_string_literal_node(current.val.stringValue);
        consume_token();
        return n;
    } else if ((expected_type == T_FALSE) || (expected_type < 0 && current.type == T_FALSE)
                || (expected_type == T_TRUE) || (expected_type < 0 && current.type == T_TRUE)){
        node* n = new node((current.type == T_FALSE ? "false" : "true"), current.type);
        consume_token();
        return n;
    }

    // TODO - enums and custom types

    throw_unexpected_token_wanted_literal(current.val.stringValue);
    return nullptr;
}

/** Processors **/
bool parser::process_block_comments(node* n) {
    if (current.type == T_BLOCK_COMMENT_OPEN){
        parse_block_comments();
        return true;
    }
    return false;
}
bool parser::process_variable_declaration(node* n) {
    if (current.type == T_VARIABLE || (current.type == T_GLOBAL && next.type == T_VARIABLE)){
        n->newChild(parse_variable_declaration());
        expecting_reserved_word(T_SEMICOLON, ";");
        return true;
    }
    return false;
}
bool parser::process_variable_assignment(node* n) {
    if (current.type == T_IDENTIFIER){
        // TODO -  This is not gonna work lol
        n->newChild(parse_variable_assignment());
        expecting_reserved_word(T_SEMICOLON, ";");
        return true;
    }
    return false;
}
bool parser::process_procedure_declaration(node* n) {
    if (current.type == T_PROCEDURE || (current.type == T_GLOBAL && next.type == T_PROCEDURE)){
        n->newChild(parse_procedure());
        return true;
    }
    return false;
}
bool parser::process_if_block(node* n) {
    if (current.type == T_IF){
        n->newChild(parse_if_block());
        return true;
    }
    return false;
}
bool parser::process_for_block(node* n) {
    if (current.type == T_FOR){
        n->newChild(parse_for_loop());
        return true;
    }
    return false;
}
bool parser::process_return_block(node* n) {
    if (current.type == T_RETURN){
        n->newChild(parse_procedure_return_statement());
        return true;
    }
    return false;
}

/** Error Handling **/
void parser::throw_runtime_template(const string& message) const {
    throw runtime_error("Line Number: " + std::to_string(current.line_number) + " | " + message);
}
void parser::throw_unexpected_token(const string& expected_token, const string& received_token, const string& extra_message) {
    throw_runtime_template("Was expecting next token to be '" + expected_token + "', but received '"
            + received_token + "' instead." + extra_message);
}
void parser::throw_unexpected_token_wanted_type(const string& received_token, const string& extra_message) {
    throw_runtime_template("Was expecting next token to be a type identifier, but received '"
            + received_token + "' instead." + extra_message);
}
void parser::throw_unexpected_token_wanted_literal(const string& received_token, const string& extra_message) {
    throw_runtime_template("Was expecting next token to be a value, but received '"
            + received_token + "' instead." + extra_message);
}
void parser::throw_unexpected_reserved_word(const string& received_token, const string& extra_message) {
    throw_runtime_template("Was expecting variable identifier but received '"
            + received_token + "' instead." + extra_message);
}

/** Symbol Table **/
void parser::push_new_identifier_to_symbol_table(string identifier, int n) {
    current_table->add_symbol(identifier, n);
}
unsigned parser::verify_identifier_is_declared(string identifier) {
    if (current_table->check_symbol_status(identifier)){
        return current_table->get_symbol_value(identifier);
    }
    throw_runtime_template("Variable " + identifier + " is not declared in the current scope.");
    return 0;
}
void parser::push_current_symbol_table() {
    current_table = new symbol_table(current_table);
}
void parser::pop_current_symbol_table() {
    current_table = current_table->getParent();
}

/** VISUALIZERS **/
void parser::print_nodes(node *n, unsigned depth) {
    for (int i = 0; i <= depth; i++){
        std::cout << "---";
    }
    std::cout << std::endl;
    for (int i = 0; i < depth; i++){
        std::cout << "   ";
    }
    std::cout << " " << n->type;
    if (n->type == T_INTEGER_LITERAL){
        cout << " | " << n->val.intValue;
    }
    std::cout << std::endl;
    for(auto nn : n->children){
        print_nodes(nn, depth + 1);
    }
}
void parser::print_node_leaves(node *n) {
    if (n->children.empty()){
        if (n->type == T_INTEGER_LITERAL){
            cout << n->type << " | " << n->val.intValue << endl;
        } else if (n->type == T_EMPTY_STRING) {
        } else {
            cout << n->type << " | " << n->val.stringValue << endl;
        }
    } else {
        for (auto c: n->children){
            print_node_leaves(c);
        }
    }


}
void parser::print_node_to_json(node *n, std::ofstream* file_id) {
    bool root = false;
    if( file_id == nullptr){
        root = true;
        file_id = new std::ofstream ("file2.json");

        *(file_id) << "{\n";
    }


    if (n->children.empty()){
        *(file_id) << "\"" << n->type << "\":";
        if (n->type == T_INTEGER_LITERAL){
            *(file_id) << "\"" << n->val.intValue << "\"";
        } else if (n->type == T_FLOAT_LITERAL){
            *(file_id) << "\"" << n->val.doubleValue << "\"";
        } else if (n->type == T_STRING_LITERAL){
            *(file_id) << "" << n->val.stringValue << "";
        }else if (n->type == T_EMPTY_STRING){
        }else if (n->type < 300){
            *(file_id) << "\"" << n->val.stringValue << "\"";
        }
    } else {
        *(file_id) << "\"" << n->type << "\":";
        *(file_id) << "{\n";

        for (int c = 0; c < n->children.size(); c++){
            print_node_to_json(n->children.at(c), file_id);
            if (c != n->children.size() - 1)
                *(file_id) << ",";
            *(file_id) << "\n";
        }

        *(file_id) << "}\n";
    }

    if( root ){
        *(file_id) << "}\n";
        file_id->close();
    }
}


