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
    void parser_expr(node* n, std::list<scanner::_token> tokens);
    node parser_add_subtract(node* n, std::list<scanner::_token> tokens);
    node parser_multiply_divide(node* n, std::list<scanner::_token> tokens);

    /** VISUALIZERS **/
    void printer_tokens(std::list<scanner::_token> tokens);
    void print_nodes(node* n, unsigned depth = 0);
    std::list<scanner::_token> get_tokens(std::string file_text);
};


#endif //COMPILER_5183_PARSER_H
