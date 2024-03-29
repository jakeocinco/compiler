//
// Created by Jacob Carlson on 2/10/21.
//

#ifndef COMPILER_5183_NODE_H
#define COMPILER_5183_NODE_H


#include <list>
#include <deque>
#include <string>

class node {
public:
    node(int type);
    node(const node *n1);
    node(std::string s, int type);
    static node* create_string_literal_node(std::string s);
    static node* create_double_literal_node(double d);
    static node* create_integer_literal_node(int i);
    static node* create_identifier_literal_node(std::string s, int type);
    void newChild(node* n);


    int type;
    std::deque<node*> children;
    union {
        char stringValue[256]; // holds lexeme value if string/identifier
        int intValue; // holds lexeme value if integer
        double doubleValue; // holds lexeme value if double
    } val;
private:

};


#endif //COMPILER_5183_NODE_H
