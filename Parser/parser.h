//
// Created by Jacob Carlson on 2/10/21.
//

#ifndef COMPILER_5183_PARSER_H
#define COMPILER_5183_PARSER_H

#include "node.h"
#include "symbol_table.h"
#include "../Scanner/scanner.h"
#include <string>

class parser {
public:
    parser(std::string file_text);
private:

    scanner* scan;
    scanner::_token current{};
    scanner::_token next{};

    node* head;
    symbol_table* current_table;

    void consume_token();

    /** Program Level **/
    node* parse_program();
    node* parse_program_declaration_block();
    node* parse_program_statement_block();

    /** Procedures **/
    node* parse_procedure();
    node* parse_procedure_declaration_block();
    node* parse_procedure_statement_block();
    node* parse_procedure_parameter_list();
    node* parse_procedure_return_statement();
    node* parse_procedure_call();

    /** Built in Functionality **/
    node* parse_if_block();
    node* parse_if_statement_block();
    node* parse_for_loop();
    node* parse_for_loop_statement_block();

    /** Expressions **/
    node* parse_expression(unsigned& code, node* n = nullptr);
    node* parse_arith(unsigned& code, node* n = nullptr);
    node* parse_relation(unsigned& code, node* n = nullptr);
    node* parse_term(unsigned& code, node* n = nullptr);
    node* parse_factor(unsigned& code);

    /** Variables **/
    node* parse_variable_declaration();
    node* parse_variable_assignment();

    /** Types **/
    node* parse_type_mark();

    /** Block Comments **/
    node* parse_block_comments();

    /** Helpers and Code Reuse **/
    void run_processes_until_true(node* n, std::list<std::function<bool()>> ll);
    bool is_current_relational_operator() const;
    bool are_types_valid_to_combine(unsigned& code, unsigned c1, unsigned c2, bool first_run = true) const;
    unsigned type_to_literal(unsigned type);

    node* expecting_reserved_word(int expected_type, const string& expected_value);
    node* expecting_identifier();
    node* expecting_predefined_type();
    node* expecting_literal(int expected_type = -1);

    /** Processors **/
    bool process_block_comments(node* n);
    bool process_variable_declaration(node* n);
    bool process_variable_assignment(node* n);
    bool process_procedure_declaration(node* n);
    bool process_if_block(node* n);
    bool process_for_block(node* n);
    bool process_return_block(node* n);

    /** Error Handling **/
    void throw_runtime_template(const string& message) const;
    void throw_unexpected_token(const string& expected_token, const string& received_token, const string& extra_message = "");
    void throw_unexpected_token_wanted_type(const string& received_token, const string& extra_message = "");
    void throw_unexpected_token_wanted_literal(const string& received_token, const string& extra_message = "");
    void throw_unexpected_reserved_word(const string& received_token, const string& extra_message = "");

    /** Symbol Table **/
    void initialize_symbol_table();
    void push_new_identifier_to_symbol_table(string identifier, int n);
    unsigned verify_identifier_is_declared(string identifier);
    void push_current_symbol_table();
    void pop_current_symbol_table();

    /** VISUALIZERS **/
    void print_nodes(node* n, unsigned depth = 0);
    void print_node_leaves(node* n);
    void print_node_to_json(node* n, std::ofstream* file_id = nullptr);
};


#endif //COMPILER_5183_PARSER_H
