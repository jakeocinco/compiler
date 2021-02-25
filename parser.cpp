//
// Created by Jacob Carlson on 2/10/21.
//

#include "parser.h"
#include "tokenCodes.h"

#include <iostream>
#include <regex>

using namespace std;
using namespace std::placeholders;

parser::parser(string file_text) {

    scan = new scanner(file_text + ' ');

    std::list<scanner::_token> tokens;
    current = scan->get_next_token();
    next = scan->get_next_token();

    head = new node(T_PROGRAM_ROOT);

    parse_program(head);
    print_node_to_json(head);
}

void parser::consume_token() {
    current = next;
    if (next.type != T_END_OF_FILE)
        next = scan->get_next_token();
}

/** Program **/
void parser::parse_program(node *n) {
    node* program_identifier = NULL;
    node* program_name = NULL;
    node* is_identifier = NULL;

    if (current.type == T_PROGRAM){
        program_identifier = new node("program", T_PROGRAM);
        consume_token();
    }
    if (current.type == T_IDENTIFIER){
        program_name = new node(current.val.stringValue, T_IDENTIFIER);
        consume_token();
    }
    if (current.type == T_IS){
        is_identifier = new node("is", T_IS);
        consume_token();
    }

    if (program_identifier != NULL && program_name != NULL && is_identifier != NULL){
        n->newChild(program_identifier);
        n->newChild(program_name);
        n->newChild(is_identifier);


    }

    /** Process declaration block **/
    node* declarations = new node(T_PROGRAM_DECLARATION_BLOCK);
    parse_program_declaration_block(declarations);
    n->newChild(declarations);

    node* begin_identifier = NULL;
    if (current.type == T_BEGIN){
        begin_identifier = new node("begin", T_BEGIN);
        consume_token();
    }
    if (begin_identifier != NULL){
        n->newChild(begin_identifier);
    }

    /** Process statement block **/
    node* statement = new node(T_PROGRAM_STATEMENT_BLOCK);
    parse_program_statement_block(statement);
    n->newChild(statement);

    node* end_identifier = NULL;
    node* end_program_identifier = NULL;
    node* end_period_identifier = NULL;
    if (current.type == T_END){
        end_identifier = new node("end", T_END);
        consume_token();
    }
    if (current.type == T_PROGRAM){
        end_program_identifier = new node("program", T_PROGRAM);
        consume_token();
    }
    if (current.type == T_PERIOD){
        end_period_identifier = new node(".", T_PERIOD);
        current = next;
    }
    if (end_identifier != NULL && end_program_identifier != NULL && end_period_identifier != NULL){
        n->newChild(end_identifier);
        n->newChild(end_program_identifier);
        n->newChild(end_period_identifier);
    }

}
void parser::parse_program_declaration_block(node *n) {
    while (current.type != T_BEGIN && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_declaration(n); });
        functionList.emplace_back([this, n] { return process_procedure_declaration(n); });

        run_processes_until_true(n, functionList);
    }
}
void parser::parse_program_statement_block(node *n){
    while (current.type != T_END && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_assignment(n); });
        functionList.emplace_back([this, n] { return process_if_block(n); });

        run_processes_until_true(n, functionList);
    }
}

/** Procedures **/
void parser::parse_procedure(node *n) {

    try {
        n->newChild(expecting_reserved_word(T_PROCEDURE, "procedure"));
        n->newChild(expecting_identifier());
        n->newChild(expecting_reserved_word(T_COLON, ":"));
        n->newChild(expecting_type());

        if (current.type == T_LPAREN) {
            node *param_list = new node(T_PARAMETER_LIST);
            while (current.type != T_RPAREN && current.type != T_END_OF_FILE) {
                consume_token();

                node *param = new node(T_PARAMETER);

                /* TODO can this use variable declaration block?? */
                param->newChild(expecting_reserved_word(T_VARIABLE, "variable"));
                param->newChild(expecting_identifier());
                param->newChild(expecting_reserved_word(T_COLON, ":"));
                param->newChild(expecting_type());

                param_list->newChild(param);
            }
            if (current.type != T_RPAREN) {
                throw_unexpected_token("(", current.val.stringValue,
                                       " All procedure headers must end with (<parameter_list>).");
            }
            consume_token();

            if (!param_list->children.empty()) {
                n->newChild(param_list);
            }
        } else {
            throw_unexpected_token("(", current.val.stringValue,
                                   " All procedure headers must end with (<parameter_list>).");
        }

        /** Process declaration block **/
        // TODO - can this be made into a function
        node *declarations = new node(T_PROCEDURE_DECLARATION_BLOCK);
        parse_procedure_declaration_block(declarations);
        if (!declarations->children.empty())
            n->newChild(declarations);

        n->newChild(expecting_reserved_word(T_BEGIN, "begin"));

        /** Process statement block **/
        // TODO - can this be made into a function
        node *statement = new node(T_PROCEDURE_STATEMENT_BLOCK);
        parse_procedure_statement_block(statement);
        if (!declarations->children.empty())
            n->newChild(statement);

        n->newChild(expecting_reserved_word(T_END, "end"));
        n->newChild(expecting_reserved_word(T_PROCEDURE, "procedure"));
    } catch (runtime_error& error){
        cout << error.what() << endl;
    }

}
void parser::parse_procedure_declaration_block(node *n) {
    while (current.type != T_BEGIN && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_declaration(n); });
        functionList.emplace_back([this, n] { return process_procedure_declaration(n); });

        run_processes_until_true(n, functionList);
    }
}
void parser::parse_procedure_statement_block(node *n){
    while (current.type != T_END && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_assignment(n); });
        functionList.emplace_back([this, n] { return process_return_block(n); });
        functionList.emplace_back([this, n] { return process_if_block(n); });

        run_processes_until_true(n, functionList);
    }
}
void parser::parse_procedure_return_statement(node *n){

    n->newChild(expecting_reserved_word(T_RETURN, "return"));

    node* value = nullptr;
    get_value_node(value);

    if (current.type == T_SEMICOLON && value != nullptr){
        n->newChild(value);
        consume_token();
    } else {
        if (value == nullptr)
            throw_runtime_template("Could not calculate valid return value.");
        else
            throw_unexpected_token(";", current.val.stringValue);
    }
}

/** Built in Functionality **/
void parser::parse_if_block(node *n) {
    node* if_identifier = nullptr;
    node* logical_expression = nullptr;
    node* then_identifier = nullptr;
    node* if_statement_block = new node(T_LOGICAL_OP_STATEMENT_BLOCK);

    node* else_identifier = nullptr;
    node* else_statement_block = new node(T_LOGICAL_OP_STATEMENT_BLOCK);

    node* end_identifier = nullptr;
    node* end_if_statement_block = nullptr;

    if (current.type == T_IF){
        if_identifier = new node("if", T_IDENTIFIER);
        consume_token();
    }
    if (current.type == T_LPAREN){
        consume_token();

        logical_expression = new node(T_LOGICAL_OP);
        parse_logical_op(logical_expression);

        if (current.type == T_RPAREN){
            consume_token();
        }
    }
    if (current.type == T_THEN){
        then_identifier = new node("then", T_THEN);
        consume_token();
    }
    parse_if_statement_block(if_statement_block);

    if (current.type == T_ELSE){
        else_identifier = new node("else", T_ELSE);
        consume_token();

        parse_if_statement_block(else_statement_block);
    }

    if (current.type == T_END){
        end_identifier = new node("end", T_END);
        consume_token();
    }
    if (current.type == T_IF){
        end_if_statement_block = new node("if", T_IF);
        consume_token();
    }

    if (if_identifier != nullptr && logical_expression != nullptr && then_identifier != nullptr
            && end_identifier != nullptr && end_if_statement_block != nullptr){

        n->newChild(if_identifier);
        n->newChild(logical_expression);
        n->newChild(then_identifier);
        n->newChild(if_statement_block);

        if (else_identifier != nullptr){
            n->newChild(else_identifier);
            n->newChild(else_statement_block);
        }
        n->newChild(end_identifier);
        n->newChild(end_if_statement_block);
    }
}
void parser::parse_if_statement_block(node *n){
    while (current.type != T_END && current.type != T_ELSE && current.type != T_END_OF_FILE){
        std::list<std::function<bool()>> functionList;

        functionList.emplace_back([this, n] { return process_block_comments(n); });
        functionList.emplace_back([this, n] { return process_variable_assignment(n); });
        functionList.emplace_back([this, n] { return process_return_block(n); });
        functionList.emplace_back([this, n] { return process_if_block(n); });

        run_processes_until_true(n, functionList);
    }
}

/** Expressions **/
// Arithmetic
void parser::parse_arith_op(node* n) {
    node* term = new node(T_TERM);
    node* arith_op_prime = new node(T_ARITH_OP_PRIME);

    parse_term(term);
    parse_arith_op_prime(arith_op_prime);

    if (!term->children.empty())
        n->newChild(term);
    if (!arith_op_prime->children.empty())
        n->newChild(arith_op_prime);
}
void parser::parse_term(node* n) {
    node* factor = new node(T_FACTOR);
    node* term_prime = new node(T_TERM_PRIME);

    parse_factor(factor);
    parse_term_prime(term_prime);

    if (!factor->children.empty())
        n->newChild(factor);
    if (!term_prime->children.empty())
        n->newChild(term_prime);
}
void parser::parse_arith_op_prime(node* n) {
    if (current.type == T_ADD || current.type == T_MINUS){
        node* symbol = node::create_identifier_literal_node(current.val.stringValue, current.type);
        node* term = new node(T_TERM);
        node* arith_op_prime = new node(T_ARITH_OP_PRIME);

        consume_token();

        parse_term(term);
        parse_arith_op(arith_op_prime);

        n->newChild(symbol);
        if (!term->children.empty())
            n->newChild(term);
        if (!arith_op_prime->children.empty())
            n->newChild(arith_op_prime);
    }
}
void parser::parse_term_prime(node* n) {
    if (current.type == T_MULTIPLY || current.type == T_DIVIDE){
        node* symbol = node::create_identifier_literal_node(current.val.stringValue, current.type);
        node* factor = new node(T_FACTOR);
        node* term_prime = new node(T_TERM_PRIME);

        consume_token();

        parse_factor(factor);
        parse_term_prime(term_prime);

        n->newChild(symbol);
        if (!factor->children.empty())
            n->newChild(factor);
        if (!term_prime->children.empty())
            n->newChild(term_prime);
    }
}
void parser::parse_factor(node* n) {
    if (current.type == T_INTEGER_LITERAL){
        n->newChild(node::create_integer_literal_node(current.val.intValue));
        consume_token();
    } else if (current.type == T_FLOAT_LITERAL){
        n->newChild(node::create_double_literal_node(current.val.doubleValue));
        consume_token();
    } else if (current.type != T_SEMICOLON){
        n->newChild(node::create_identifier_literal_node(current.val.stringValue, current.type));
        consume_token();
    }
//    consume_token();
//    if (current.type == T_SEMICOLON){
//        consume_token();
//    }
}
// Logic
void parser::parse_logical_op(node *n) {
    bool single_op = false;
    node* left_hand_value = nullptr;
    node* operator_identifier = nullptr;
    node* right_hand_value = nullptr;

    get_boolean_node(left_hand_value);

    if (is_current_relational_operator()){
        operator_identifier = new node(current.val.stringValue, current.type);
        consume_token();
        get_boolean_node(right_hand_value);
        single_op = true;
    }

    if (left_hand_value != nullptr ){
        if (operator_identifier != nullptr && right_hand_value != nullptr) {
            n->newChild(left_hand_value);
            n->newChild(operator_identifier);
            n->newChild(right_hand_value);
        } else if(single_op){
            if (operator_identifier != nullptr){
                new runtime_error("Was expecting operator (<, >, <=, >=, !=, ==), but none was received.");
            } else if (right_hand_value != nullptr){
                string s1 = "Was expecting expression on right side after ";
                new runtime_error(s1 + operator_identifier->val.stringValue + ", but a valid expression was not recieved.");

            }
        }
    }
}

/** Variables **/
void parser::parse_variable_declaration(node *n) {
    /** TODO: ADD TO TABLE **/
    node* variable_identifier = NULL;
    node* variable_name = NULL;
    node* colon_identifier = NULL;
    node* type_identifier = NULL;

    node* l_bracket_identifier = NULL;
    node* array_size = NULL;
    node* r_bracket_identifier = NULL;
    bool isArray = false;

    if (current.type == T_VARIABLE){
        variable_identifier = new node("variable", T_VARIABLE);
        consume_token();
    }
    if (current.type == T_IDENTIFIER){
        variable_name = new node(current.val.stringValue, T_IDENTIFIER);
        consume_token();
    }
    if (current.type == T_COLON){
        colon_identifier = new node(":", T_COLON);
        consume_token();
    }

    if (current.type == T_INTEGER_TYPE){
        type_identifier = new node(current.val.stringValue, T_INTEGER_TYPE);
    } else if (current.type == T_FLOAT_TYPE){
        type_identifier = new node(current.val.stringValue, T_FLOAT_TYPE);
    } else if (current.type == T_STRING_TYPE){
        type_identifier = new node(current.val.stringValue, T_STRING_TYPE);
    }
    consume_token();

    if (current.type == T_LBRACKET){
        l_bracket_identifier = new node(current.val.stringValue, T_LBRACKET);
        isArray = true;
        consume_token();
    }
    if (current.type == T_INTEGER_LITERAL){
        array_size = node::create_integer_literal_node(current.val.intValue);
        isArray = true;
        consume_token();
    }
    if (current.type == T_RBRACKET){
        r_bracket_identifier = new node(current.val.stringValue, T_RBRACKET);
        isArray = true;
        consume_token();
    }

    if (current.type == T_SEMICOLON){
        consume_token();
        if ( variable_identifier != NULL && variable_name != NULL &&
                colon_identifier != NULL && type_identifier != NULL ){
            if ((l_bracket_identifier != NULL && array_size != NULL &&
                    r_bracket_identifier != NULL) || !isArray){
                n->newChild(variable_identifier);
                n->newChild(variable_name);
                n->newChild(colon_identifier);
                n->newChild(type_identifier);
                if (isArray){
                    n->newChild(l_bracket_identifier);
                    n->newChild(array_size);
                    n->newChild(r_bracket_identifier);
                }
            }
        } else {
            cout << "ERROR" << endl;
        }
    }
}
void parser::parse_variable_assignment(node *n) {

    node* variable = NULL;

    node* l_bracket_identifier = NULL;
    node* array_size = NULL;
    node* r_bracket_identifier = NULL;
    bool isArray = false;

    node* assignment_sign = NULL;
    node* value = NULL;

    if (current.type == T_IDENTIFIER){
        /** TODO: Check scope **/

        variable = new node(current.val.stringValue, T_IDENTIFIER);
        consume_token();
    }
    if (current.type == T_LBRACKET){
        l_bracket_identifier = new node(current.val.stringValue, T_LBRACKET);
        isArray = true;
        consume_token();
    }
    if (current.type == T_INTEGER_LITERAL){
        array_size = node::create_integer_literal_node(current.val.intValue);
        isArray = true;
        consume_token();
    }
    if (current.type == T_RBRACKET){
        r_bracket_identifier = new node(current.val.stringValue, T_RBRACKET);
        isArray = true;
        consume_token();
    }

    // Can stuff like this be stripped yet -- kinda like semi colons
    if (current.type == T_COLON_EQUALS){
        assignment_sign = new node(current.val.stringValue, T_COLON_EQUALS);
        consume_token();
    }

    get_value_node(value);

    if (current.type == T_SEMICOLON){
        consume_token();
        if ( variable != NULL && assignment_sign != NULL &&
             value != NULL){
            if ((l_bracket_identifier != NULL && array_size != NULL &&
                 r_bracket_identifier != NULL) || !isArray) {
                n->newChild(variable);
                if (isArray) {
                    n->newChild(l_bracket_identifier);
                    n->newChild(array_size);
                    n->newChild(r_bracket_identifier);
                }
                n->newChild(assignment_sign);
                n->newChild(value);
            } else {
                cout << "ERROR 1";
            }
        } else {
            cout << "ERROR 2, size: " << assignment_sign << variable << value->children.size();
        }
    } else{
        cout << "No semicolon" << endl;
    }
}

/** Block Comments **/
void parser::parse_block_comments() {
    if (current.type == T_BLOCK_COMMENT_OPEN){
        while( current.type != T_BLOCK_COMMENT_CLOSE) {
            consume_token();
            if (current.type == T_BLOCK_COMMENT_OPEN){
                parse_block_comments();
            }
            if (current.type == T_END_OF_FILE){
                cout << "NO CLOSURE" << endl;
                break;
            }
        }
        consume_token();
    }
}

/** Helpers and Code Reuse **/
void parser::get_value_node(node *&n) {
    if (current.type == T_STRING_LITERAL){
        n = node::create_string_literal_node(current.val.stringValue);
        consume_token();
    } else if (current.type == T_FALSE){
        n = new node("false", T_FALSE);
        consume_token();
    } else if (current.type == T_TRUE){
        n = new node("true", T_TRUE);
        consume_token();
    } else {
        n = new node("", T_ARITH_OP);
        parse_arith_op(n);
    }
}
void parser::get_boolean_node(node *&n){
    if (current.type == T_FALSE){
        n = new node("false", T_FALSE);
        consume_token();
    } else if (current.type == T_TRUE){
        n = new node("true", T_TRUE);
        consume_token(); 
    } else {
        n = new node("", T_ARITH_OP);
        parse_arith_op(n);
    }
}
void parser::run_processes_until_true(node* n, std::list<std::function<bool()>> ll) {
    while (!ll.empty()){
        if (ll.front()()){
            return;
        }
        ll.pop_front();
    }
    throw runtime_error("No valid processes were found.");
}
bool parser::is_current_relational_operator() const {
    return current.type >= T_L_THAN && current.type <= T_N_EQUALS;
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
node*  parser::expecting_type() {
    if (current.type == T_INTEGER_TYPE || current.type == T_FLOAT_TYPE ||
            current.type == T_STRING_TYPE || current.type == T_BOOL_TYPE){
        node* n = new node(current.val.stringValue, current.type);
        consume_token();
        return n;
    }
    throw_unexpected_token_wanted_type(current.val.stringValue);
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
    if (current.type == T_VARIABLE){
        node* nn = new node(T_VARIABLE_DECLARATION);
        parse_variable_declaration(nn);
        n->newChild(nn);
        return true;
    }
    return false;
}
bool parser::process_variable_assignment(node* n) {
    if (current.type == T_IDENTIFIER){
        // TODO -  This is not gonna work lol
        node* nn = new node(T_VARIABLE_ASSIGNMENT);
        parse_variable_assignment(nn);
        n->newChild(nn);
        return true;
    }
    return false;
}
bool parser::process_procedure_declaration(node* n) {
    if(current.type == T_PROCEDURE){
        node* nn = new node(T_PROCEDURE_DECLARATION);
        parse_procedure(nn);
        n->newChild(nn);
        return true;
    }
    return false;
}
bool parser::process_if_block(node* n) {
    if (current.type == T_IF){
        node* nn = new node(T_IF_BLOCK);
        parse_if_block(nn);
        n->newChild(nn);
        return true;
    }
    return false;
}
bool parser::process_return_block(node* n) {
    if (current.type == T_RETURN){
        node* nn = new node(T_RETURN_BLOCK);
        parse_procedure_return_statement(nn);
        n->newChild(nn);
        return true;
    }
    return false;
}

/** Error Handling **/
void parser::throw_runtime_template(const string& message) {
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
void parser::throw_unexpected_reserved_word(const string& received_token, const string& extra_message) {
    throw_runtime_template("Was expecting variable identifier but received '"
            + received_token + "' instead." + extra_message);
}

/** VISUALIZERS **/
void parser::printer_tokens() {
    while(current.type != T_END_OF_FILE){
        cout << current.type << " | ";
        if (current.type == T_INTEGER_LITERAL)
            cout << current.val.intValue << " | " ;
        else if (current.type == T_FLOAT_LITERAL)
            cout << current.val.doubleValue << " | " ;
        else
            cout << current.val.stringValue << " | ";
        cout << current.line_number << endl;

        consume_token();
    }
}
std::list<scanner::_token> parser::get_tokens(std::string file_text) {
    auto *s = new scanner(file_text + ' ');
    std::list<scanner::_token> tokens;
    scanner::_token tt = s->get_next_token();
    while (tt.type != T_END_OF_FILE){
        tokens.push_back(tt);

        tt = s->get_next_token();

    }

    return tokens;
}
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

