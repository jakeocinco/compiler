//
// Created by Jacob Carlson on 2/10/21.
//

#ifndef COMPILER_5183_PARSER_H
#define COMPILER_5183_PARSER_H

#include "node.h"
#include "scanner.h"
#include <string>

class parser {
public:
    parser(std::string file_text);
private:

    scanner* scan;
    scanner::_token current;
    scanner::_token next;

    node* head;

    void consume_token();

    void parse_program(node* n);
    void parse_program_declaration_block(node* n);
    void parse_program_statement_block(node* n);

    void parse_arith_op(node* n);
    void parse_term(node* n);
    void parse_arith_op_prime(node* n);
    void parse_term_prime(node* n);
    void parse_factor(node* n);

    void parse_variable_declaration(node* n);
    void parse_variable_assignment(node* n);

    /** VISUALIZERS **/
    void printer_tokens(std::list<scanner::_token> tokens);
    void print_nodes(node* n, unsigned depth = 0);
    void print_node_leaves(node* n);
    std::list<scanner::_token> get_tokens(std::string file_text);
};


#endif //COMPILER_5183_PARSER_H
