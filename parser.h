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
    scanner::_token current{};
    scanner::_token next{};

    node* head;

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

    /** Built in Functionality **/
    node* parse_if_block();
    node* parse_if_statement_block();
    node* parse_for_loop();
    node* parse_for_loop_statement_block();

    /** Expressions **/
    node* parse_expression(node* n = nullptr);
    node* parse_arith(node* n = nullptr);
    node* parse_relation(node* n = nullptr);
    node* parse_term(node* n = nullptr);
    node* parse_factor();

    /** Variables **/
    node* parse_variable_declaration();
    node* parse_variable_assignment();

    /** Block Comments **/
    node* parse_block_comments();

    /** Helpers and Code Reuse **/
    static void run_processes_until_true(node* n, std::list<std::function<bool()>> ll);
    bool is_current_relational_operator() const;

    node* expecting_reserved_word(int expected_type, const string& expected_value);
    node* expecting_identifier();
    node* expecting_type();
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

    /** VISUALIZERS **/
    void printer_tokens();
    void print_nodes(node* n, unsigned depth = 0);
    void print_node_leaves(node* n);
    void print_node_to_json(node* n, std::ofstream* file_id = nullptr);
    std::list<scanner::_token> get_tokens(std::string file_text);
};


#endif //COMPILER_5183_PARSER_H
