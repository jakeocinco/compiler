//
// Created by Jacob Carlson on 2/10/21.
//

#include "parser.h"
#include "tokenCodes.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <fstream>
parser::parser(string file_text) {

    head = new node(T_PROGRAM_ROOT);
//    head = new node(T_ARITH_OP);

    scan = new scanner(file_text + ' ');
    std::list<scanner::_token> tokens;
    current = scan->get_next_token();
    next = scan->get_next_token();

    parse_program(head);
//    parse_arith_op(head);
    print_node_to_json(head);
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
    node* declarations = new node(68);
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
    node* statement = new node(70);
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
        if (current.type == T_BLOCK_COMMENT_OPEN){
            parse_block_comments();
        } else if (current.type == T_VARIABLE){
            node* nn = new node(69);
            parse_variable_declaration(nn);
            n->newChild(nn);
        }
    }
}
void parser::parse_program_statement_block(node *n){
    while (current.type != T_END && current.type != T_END_OF_FILE){

        if (current.type == T_BLOCK_COMMENT_OPEN){
            parse_block_comments();
        } else if (current.type == T_IDENTIFIER){
            // This is not gonna work lol
            node* nn = new node(69);
            parse_variable_assignment(nn);
            n->newChild(nn);
        }
    }
}

/** Procedures **/
void parser::parse_procedure(node *n) {
    node* program_identifier = NULL;
    node* program_name = NULL;
    node* is_identifier = NULL;

    if (current.type == T_PROCEDURE){
        program_identifier = new node("procedure", T_PROGRAM);
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
    node* declarations = new node(68);
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
    node* statement = new node(70);
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

/** Expressions **/
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


    if (current.type == T_STRING_LITERAL){
        value = node::create_string_literal_node(current.val.stringValue);
        consume_token();
    } else if (current.type == T_FALSE){
        value = new node("false", T_FALSE);
        consume_token();
    } else if (current.type == T_TRUE){
        value = new node("true", T_TRUE);
        consume_token();
    } else {
        value = new node("", T_ARITH_OP);
        parse_arith_op(value);
    }


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

void parser::printer_tokens(std::list<scanner::_token> tokens) {
    for (auto tt : tokens){
        cout << tt.type << " | ";
        if (tt.type == T_INTEGER_LITERAL)
            cout << tt.val.intValue << " | " ;
        else if (tt.type == T_FLOAT_LITERAL)
            cout << tt.val.doubleValue << " | " ;
        else
            cout << tt.val.stringValue << " | ";
        cout << tt.line_number << endl;
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


void parser::consume_token() {
    current = next;
    if (next.type != T_END_OF_FILE)
        next = scan->get_next_token();
}










