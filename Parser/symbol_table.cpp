//
// Created by Jacob Carlson on 3/7/21.
//

#include <iostream>
#include "symbol_table.h"

symbol_table::symbol_table(symbol_table *parent) {
    this->parent = parent;
}

void symbol_table::add_symbol(std::string identifier, int n) {
    this->table.insert_or_assign(identifier, n);
}
bool symbol_table::check_symbol_status(const std::string& identifier) {
    if (this->table.contains(identifier))
        return true;
    if (this->parent != nullptr)
        return this->parent->check_symbol_status(identifier);
    return false;
}
unsigned symbol_table::get_symbol_value(std::string identifier) {
    if (this->table.contains(identifier))
        return this->table.find(identifier)->second;
    if (this->parent != nullptr)
        return this->parent->get_symbol_value(identifier);
    return 0;
}

symbol_table *symbol_table::getParent() const {
    return parent;
}

void symbol_table::print(bool current) {
    if (current){
        for (auto const& pair: this->table) {
            std::cout << "{" << pair.first << ": " << pair.second << "}\n";
        }
    } else {
        std::list<symbol_table*> tables;
        symbol_table* current_table = this;
        while (current_table != nullptr){
            tables.push_front(current_table);
            current_table = current_table->getParent();
        }
        for (auto const& i : tables) {
            std::cout << "__________________" << std::endl;
            for (auto const& pair: i->table) {
                std::cout << "{" << pair.first << ": " << pair.second << "}\n";
            }
        }

    }
}

vector<int> symbol_table::get_symbol_params(std::string identifier) {
    if (this->params_types.contains(identifier))
        return this->params_types.find(identifier)->second;
    if (this->parent != nullptr)
        return this->parent->get_symbol_params(identifier);
    return vector<int>();
}

void symbol_table::add_symbol_params(std::string identifier, vector<int> param_types) {
    this->params_types.insert_or_assign(identifier, param_types);
}


