//
// Created by Jacob Carlson on 2/9/21.
//

#ifndef COMPILER_5183_COMPILER_H
#define COMPILER_5183_COMPILER_H

#include <string>

using namespace std;

class compiler {
public:
    explicit compiler(const string& file_name);
private:

    static string get_file_text_as_string(const string& file_name);
    static string get_file_name_root(const string& file_name);

    static void link_to_runtime(const string& file_name);
};


#endif //COMPILER_5183_COMPILER_H
