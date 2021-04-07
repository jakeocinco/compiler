#include <string>

#include "compiler.h"

#include <iostream>
#include <regex>
#include <sstream>

using namespace std;

//static LLVMContext TheContext;
//static IRBuilder<> Builder(TheContext);
//static std::unique_ptr<Module> TheModule;
//static std::map<std::string, Value *> NamedValues;

int main(int argc, char** argv) {

    if (argc > 1){
        for (int i = 1; i < argc; i++){
            new compiler(argv[i]);
        }
    }
    return 0;
}
