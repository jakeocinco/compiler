//
// Created by Jacob Carlson on 2/10/21.
//

#include <string>
#include "node.h"
#include "tokenCodes.h"

node::node(int type) {
    this->type = type;
}

void node::newChild(node* n) {
    this->children.push_back(n);
}

node* node::create_string_literal_node(std::string s) {
    node* n = new node(T_INTEGER_LITERAL);
    std::strcpy( n->val.stringValue , s.c_str());
    return n;
}

node* node::create_integer_literal_node(int i) {
    node* n = new node(T_INTEGER_LITERAL);
    n->val.intValue = i;
    return n;
}


