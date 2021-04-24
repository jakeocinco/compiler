//
// Created by Jacob Carlson on 2/9/21.
//


#include "compiler.h"
#include "Parser/parser.h"
#include "CodeGeneration/code_generation.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <fstream>

using namespace std;

compiler::compiler(const string& file_name) {

    const string file_text = get_file_text_as_string(file_name);
    new code_generation(file_text);

    link_to_runtime(get_file_name_root(file_name));
}

string compiler::get_file_text_as_string(const string& file_name) {
    std::ifstream inFile;
    inFile.open(file_name); //open the input file

    std::stringstream strStream;
    strStream << inFile.rdbuf(); //read the file
    string str = strStream.str(); //str holds the content of the file
    return str;
}
string compiler::get_file_name_root(const string& file_name) {
    string is = file_name;
    int index = 0;
    while (true){
        index = is.find('/');
        if (index > 0){
            string temp = is.substr(index + 1);
            is = temp;
        } else {
            break;
        }
    }
    index = is.find('.');
    return (is.substr(0, index));
}

void compiler::link_to_runtime(const string &file_name) {
    cout << "Executable - " << file_name << endl;
    string temp = "clang output.o runtime.c -lm -o " + file_name;
    system(temp.c_str());
}

