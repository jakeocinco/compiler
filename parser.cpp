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
    list<scanner::_token> tokens = get_tokens(file_text);

//    node* n = new node(T_ASSIGN);
//    node* n1 = new node(3);
//    node* n2 = new node(5);
//    n->newChild(n1);
//    n->newChild(n2);
//    parser_expr(n, tokens);
//    print_nodes(n);

    printer_tokens(tokens);
}

void parser::parser_expr(node* n, std::list<scanner::_token> tokens) {
    scanner::_token op{};
    std::list<scanner::_token> left_hand_side;
    std::list<scanner::_token> rights_hand_side;
    bool foundOperator = false;

    while (!foundOperator && !tokens.empty()){
        if (tokens.front().type == T_ADD || tokens.front().type == T_MINUS ){
            foundOperator = true;
            op = tokens.front();
            tokens.pop_front();
        } else {
            left_hand_side.push_back(tokens.front());
            tokens.pop_front();
        }
    }

    //DO MATH

    rights_hand_side.assign(tokens.begin(), tokens.end());
    if (!foundOperator){

        tokens.assign(left_hand_side.begin(),left_hand_side.end());

        left_hand_side = {};
        rights_hand_side = {};

        while (!foundOperator && !tokens.empty()){
            cout << tokens.front().type << endl;
            if ((tokens.front().type == T_MULTIPLY) || (tokens.front().type == T_DIVIDE )){
                foundOperator = true;
                op = tokens.front();
                tokens.pop_front();
            } else {
                left_hand_side.push_back(tokens.front());
                tokens.pop_front();
            }
        }
        rights_hand_side.assign(tokens.begin(), tokens.end());
    }

//    std::cout << "--------------------------------" << std::endl;
//    std::cout << "---- LEFT ----" << std::endl;
//    printer_tokens(left_hand_side);
//    std::cout << "---- OP ----" << std::endl;
//    cout << op.type << " | " << op.val.stringValue << " | " << op.line_number << endl;
//    std::cout << "---- RIGHT ----" << std::endl;
//    printer_tokens(rights_hand_side);


    if (left_hand_side.size() > 1){
        node *left = new node(T_ASSIGN);
        parser_expr(left, left_hand_side);
        n->newChild(left);
    }else{
        node *left = new node(left_hand_side.front().type);
        if (left_hand_side.front().type == T_INTEGER_LITERAL)
            left->val.intValue = left_hand_side.front().val.intValue;
        n->newChild(left);
    }
    n->newChild(new node(op.type));
    if (rights_hand_side.size() > 1){
        node *right = new node(T_ASSIGN);
        parser_expr(right, rights_hand_side);
        n->newChild(right);
    } else {
        node *right = new node(rights_hand_side.front().type);
        if (rights_hand_side.front().type == T_INTEGER_LITERAL)
            right->val.intValue = rights_hand_side.front().val.intValue;
        //node::create_integer_literal_node(rights_hand_side.front().val.intValue);
        n->newChild(right);
    }
//    return n;
}
//return pointer to potential child, if null try parse mutl
node parser::parser_add_subtract(node *n, std::list<scanner::_token> tokens) {

    return node(1);
}

node parser::parser_multiply_divide(node *n, std::list<scanner::_token> tokens) {

    return node(1);
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




