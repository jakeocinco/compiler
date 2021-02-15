//
// Created by Jacob Carlson on 2/9/21.
//


#include "compiler.h"
#include "parser.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <fstream>

using namespace std;

compiler::compiler(const string& file_name) {

    const string file_text = get_file_text_as_string(file_name);
    new parser(file_text);
}

string compiler::get_file_text_as_string(const string& file_name) {
    std::ifstream inFile;
    inFile.open(file_name); //open the input file

    std::stringstream strStream;
    strStream << inFile.rdbuf(); //read the file
    string str = strStream.str(); //str holds the content of the file
    return str;
}

