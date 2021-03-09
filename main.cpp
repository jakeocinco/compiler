#include <string>

#include "compiler.h"

#include <iostream>
#include <regex>
#include <sstream>

using namespace std;

int main(int argc, char** argv) {

    if (argc > 1){
        for (int i = 1; i < argc; i++){
            new compiler(argv[i]);
        }
    }
    return 0;
}
