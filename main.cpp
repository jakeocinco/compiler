#include <string>

#include "compiler.h"

#include <iostream>
#include <regex>
#include <sstream>

using namespace std;

int main(int argc, char** argv) {

    bool test = false;
    if (test){
        if (argc > 1){
            for (int i = 1; i < argc; i++){
                if (regex_match (argv[i], regex("(\")(.*)(\")") ))
                    cout << "string:" << argv[i] << " => matched" << endl;
                else
                    cout << "string:" << argv[i] << " => NOT matched" << endl;
            }
        }
        return 0;
    }

    if (argc > 1){
        for (int i = 1; i < argc; i++){
            new compiler(argv[i]);
        }
    }
    return 0;
}
