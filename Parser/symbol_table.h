//
// Created by Jacob Carlson on 3/7/21.
//

#ifndef COMPILER_5183_SYMBOL_TABLE_H
#define COMPILER_5183_SYMBOL_TABLE_H
#include <string>
#include <map>
#include <vector>
#include "node.h"

using namespace std;

class symbol_table {

public:
    symbol_table(symbol_table* parent = nullptr);

    void add_symbol(std::string identifier, int n);
    bool check_symbol_status(const std::string& identifier);
    unsigned get_symbol_value(std::string identifier);
    vector<int> get_symbol_params(std::string identifier);
    void add_symbol_params(std::string identifier, vector<int> param_types);

    symbol_table *getParent() const;

    void print(bool current = false);
private:
    std::map<string,int> table;
    std::map<string,vector<int>> params_types;
    symbol_table* parent;
};


#endif //COMPILER_5183_SYMBOL_TABLE_H
