//
// Created by Jacob Carlson on 1/27/21.
//
#include <regex>
#include <list>
#include <utility>
#include "scanner.h"
#include "../tokenCodes.h"

#include <cstring>

scanner::scanner(string file_text) {
    cursor = 0;
    line_num = 1;
    raw_text = std::move(file_text);

    reserved_words = get_reserved_words();
    next_text = get_next_text();
}

scanner::_token scanner::get_next_token() {
    while (true){
        string current = next_text;
        next_text = get_next_text();
        // Whitespace
        if (current == "//"){
            while(current != "\n"){
                current = next_text;
                next_text = get_next_text();
            }
        }
        if (current == "\n"){
            line_num++;
        }
        if (!is_white_space(current)){

            /* Check if word is in reserves*/
            if ( reserved_words.find(lower(current)) != reserved_words.end() ) {
                return generate_reserve_word_token(reserved_words.at(lower(current)), lower(current), line_num);
            }
            /* String literal */
            if (regex_match (current, regex("(\")(.*)(\")") )){
                return generate_string_literal_token(current, line_num);
            }
            /* Integer literal */
            if (regex_match (current, regex("([0-9]*)") )){
                return generate_integer_literal_token(std::stoi(current), line_num);
            }
            /* float literal */
            if (regex_match (current, regex("([0-9]*[.])?[0-9]*") )){
                return generate_float_literal_token(std::stod(current), line_num);
            }
            return generate_identifier_literal_token(lower(current), line_num);
        }

    }

}

std::map<string, int> scanner::get_reserved_words() {
    std::map<string,int> r;

    // Reserve Words
    r.insert_or_assign("program",T_PROGRAM);
    r.insert_or_assign("is",T_IS);
    r.insert_or_assign("begin",T_BEGIN);
    r.insert_or_assign("end",T_END);
    r.insert_or_assign("global",T_GLOBAL);
    r.insert_or_assign("procedure",T_PROCEDURE);
    r.insert_or_assign("return",T_RETURN);
    // VARIABLES
    r.insert_or_assign("variable",T_VARIABLE);
    r.insert_or_assign("type",T_TYPE);
    r.insert_or_assign("integer",T_INTEGER_TYPE);
    r.insert_or_assign("float",T_FLOAT_TYPE);
    r.insert_or_assign("string",T_STRING_TYPE);
    r.insert_or_assign("bool",T_BOOL_TYPE);
    r.insert_or_assign("enum",T_ENUM_TYPE);
    // CONDITIONALS
    r.insert_or_assign("if",T_IF);
    r.insert_or_assign("then",T_THEN);
    r.insert_or_assign("else",T_ELSE);
    r.insert_or_assign("for",T_FOR);
    // LOGICAL SYMBOLS
    r.insert_or_assign("&",T_AND);
    r.insert_or_assign("|",T_OR);
    r.insert_or_assign("not",T_NOT);
    r.insert_or_assign("<",T_L_THAN);
    r.insert_or_assign(">",T_G_THAN);
    r.insert_or_assign("<=",T_LE_THAN);
    r.insert_or_assign(">=",T_GE_THAN);
    r.insert_or_assign("==",T_D_EQUALS);
    r.insert_or_assign("!=",T_N_EQUALS);
    // MATH
    r.insert_or_assign("*",T_MULTIPLY);
    r.insert_or_assign("/",T_DIVIDE);
    r.insert_or_assign("+",T_ADD);
    r.insert_or_assign("-",T_MINUS);
    // BOOLEAN
    r.insert_or_assign("true",T_TRUE);
    r.insert_or_assign("false",T_FALSE);
    // SYMBOLS
    r.insert_or_assign("(",T_LPAREN);
    r.insert_or_assign(")",T_RPAREN);
    r.insert_or_assign("{",T_LBRACE);
    r.insert_or_assign("}",T_RBRACE);
    r.insert_or_assign("[",T_LBRACKET);
    r.insert_or_assign("]",T_RBRACKET);
    r.insert_or_assign(",",T_COMMA);
    r.insert_or_assign(".",T_PERIOD);
    r.insert_or_assign("\"",T_QUOTATION);
    r.insert_or_assign(":",T_COLON);
    r.insert_or_assign(";",T_SEMICOLON);
    r.insert_or_assign("=",T_ASSIGN);
    r.insert_or_assign("eof", T_END_OF_FILE);
    r.insert_or_assign(":=",T_COLON_EQUALS);
    r.insert_or_assign("/*", T_BLOCK_COMMENT_OPEN);
    r.insert_or_assign("*/", T_BLOCK_COMMENT_CLOSE);
    // BUILT IN FUNCTIONS
    r.insert_or_assign("getbool",T_GET_BOOL);
    r.insert_or_assign("getinteger",T_GET_INTEGER);
    r.insert_or_assign("getfloat",T_GET_FLOAT);
    r.insert_or_assign("getstring",T_GET_STRING);
    r.insert_or_assign("putbool",T_PUT_BOOL);
    r.insert_or_assign("putinteger",T_PUT_INTEGER);
    r.insert_or_assign("putfloat",T_PUT_FLOAT);
    r.insert_or_assign("putstring",T_PUT_STRING);
    r.insert_or_assign("sqrt",T_SQRT);


    return r;
}

bool scanner::is_delimiter(string s) {
    std::list<string> l = { "&","|","*","/","+","-","(",")","[","]",",",":",";","=",">","<","!"};
    return (std::find(std::begin(l), std::end(l), s) != std::end(l));
}

bool scanner::is_white_space(string s) {
    std::list<string> l = { " " ,"\n","\t",""};
    return (std::find(std::begin(l), std::end(l), s) != std::end(l));
//    return (std::find(l.begin(), l.end(), c) != l.end());
}
string scanner::lower(string s) {

    if (!is_delimiter(s) && !is_white_space(s))
    {
        for (int i = 0; i < s.length(); i++){
            s[i] = tolower(s[i]);
        }
    }
    return s;
}

string scanner::trim(basic_string<char> s, const char *t) {
    s.erase(s.find_last_not_of(t) + 1);
    s.erase(0, s.find_first_not_of(t));
    return s;
}

string scanner::get_next_text() {
    while (cursor < raw_text.length()){
        if (is_delimiter(string(1,raw_text.at(cursor))) || is_white_space(string(1,raw_text.at(cursor)))){
            string s = trim(raw_text.substr(0,cursor), ws);
            string symbol = string(1, raw_text.at(cursor));

            // Special cases for symbols, split out to separate functions
            if (symbol == "/"){
                if (raw_text.at(cursor + 1) == '/' || raw_text.at(cursor + 1) == '*'){
                    s = trim(raw_text.substr(0,cursor+2), ws);
                    raw_text = raw_text.substr(cursor+2, raw_text.length());
                    cursor = 0;
                    return s;
                }
            }
            if (symbol == "*"){
                if (raw_text.at(cursor + 1) == '/'){
                    if (cursor == 0){
                        s = trim(raw_text.substr(0,cursor+2), ws);
                        raw_text = raw_text.substr(cursor+2, raw_text.length());
                        cursor = 0;
                        return s;
                    } else {
                        s = trim(raw_text.substr(0,cursor), ws);
                        raw_text = raw_text.substr(cursor, raw_text.length());
                        cursor = 0;
                        return s;
                    }
                }
            }
            if (symbol == ":"){
                if (raw_text.at(cursor + 1) == '='){
                    if (cursor == 0){
                        s = trim(raw_text.substr(0,cursor+2), ws);
                        raw_text = raw_text.substr(cursor+2, raw_text.length());
                        cursor = 0;
                        return s;
                    } else {
                        s = trim(raw_text.substr(0,cursor), ws);
                        raw_text = raw_text.substr(cursor, raw_text.length());
                        cursor = 0;
                        return s;
                    }
                }
            }
            if (symbol == ">" || symbol == "<" || symbol == "!" || symbol == "="){
                if (raw_text.at(cursor + 1) == '='){
                    if (cursor == 0){
                        s = trim(raw_text.substr(0,cursor+2), ws);
                        raw_text = raw_text.substr(cursor+2, raw_text.length());
                        cursor = 0;
                        return s;
                    } else {
                        s = trim(raw_text.substr(0,cursor), ws);
                        raw_text = raw_text.substr(cursor, raw_text.length());
                        cursor = 0;
                        return s;
                    }
                }
            }

            // THIS CAN BE BETTER - check only symbol probably
            if (0 < s.length()){
                raw_text = raw_text.substr(cursor, raw_text.length());
                cursor = 0;
                return s;
            }
            if (is_delimiter(symbol) || is_white_space(symbol)){
                raw_text = raw_text.substr(cursor + 1, raw_text.length());
                cursor = 0;
                return symbol;
            }

        }
        cursor++;
    }
    // THROW EXCEPTION
    return "eof";
}


scanner::_token scanner::generate_reserve_word_token(int type, const string &s, unsigned line_number) {
    _token t{};
    t.type = type;
    t.line_number = line_number;
    std::strcpy(t.val.stringValue, s.c_str());
    return t;
}
scanner::_token scanner::generate_string_literal_token(const string& s, unsigned line_number) {
    _token t{};
    t.type = T_STRING_LITERAL;
    t.line_number = line_number;
    std::strcpy(t.val.stringValue, s.c_str());
    return t;
}
scanner::_token scanner::generate_integer_literal_token(int s, unsigned line_number) {
    _token t{};
    t.type = T_INTEGER_LITERAL;
    t.line_number = line_number;
    t.val.intValue = s;
    return t;
}
scanner::_token scanner::generate_float_literal_token(double s, unsigned line_number) {
    _token t{};
    t.type = T_FLOAT_LITERAL;
    t.line_number = line_number;
    t.val.doubleValue = s;
    return t;
}
scanner::_token scanner::generate_identifier_literal_token(const string &s, unsigned line_number) {
    _token t{};
    t.type = T_IDENTIFIER;
    t.line_number = line_number;
    std::strcpy(t.val.stringValue, s.c_str());
    return t;
}



