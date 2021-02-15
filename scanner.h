//
// Created by Jacob Carlson on 1/27/21.
//

#ifndef COMPILER_5183_SCANNER_H
#define COMPILER_5183_SCANNER_H

#include <iostream>
#include <fstream>
#include <map>
#include <list>

using namespace std;

class scanner {
public:
    struct _token {
        int type; // one of the token codes from above
        unsigned line_number;
        union {
            char stringValue[256]; // holds lexeme value if string/identifier
            int intValue; // holds lexeme value if integer
            double doubleValue; // holds lexeme value if double
        } val;
    };

    scanner(string file_text);
    _token get_next_token();
private:

    unsigned cursor;
    unsigned line_num;
    string raw_text;
    string next_text;

    std::map<string, int> reserved_words;
    const char* ws = " \t\n\r\f\v";

    static bool is_delimiter(string s);
    static bool is_white_space(string s);
    static string lower(string s);
    static bool is_symbol(char c);
    static std::map<std::string, int> get_reserved_words();

    string get_next_text();



    /** Token Builders **/
    static _token generate_reserve_word_token(int type, const string& s, unsigned line_number);
    static _token generate_string_literal_token(const string& s, unsigned line_number);
    static _token generate_integer_literal_token(int s, unsigned line_number);
    static _token generate_float_literal_token(double s, unsigned line_number);
    static _token generate_identifier_literal_token(const string& s, unsigned line_number);
    /** String modifiers **/
    static string trim(basic_string<char> s, const char* t);

};


#endif //COMPILER_5183_SCANNER_H
