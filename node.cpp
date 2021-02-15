//
// Created by Jacob Carlson on 2/10/21.
//

#include <string>
#include "node.h"
#include "tokenCodes.h"

node::node(int type) {
    this->type = type;
}

node::node(std::string s, int type) {
    this->type = type;
    std::strcpy( this->val.stringValue , s.c_str());
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

node* node::create_double_literal_node(double i) {
    node* n = new node(T_INTEGER_LITERAL);
    n->val.intValue = i;
    return n;
}

node *node::create_identifier_literal_node(std::string s, int type) {
    node* n = new node(type);
    std::strcpy( n->val.stringValue , s.c_str());
    return n;
}



