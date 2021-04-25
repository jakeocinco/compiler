## Structure
The project is structured into 3 main parts: the scanner, parser, and code generator. 

The scanner works by getting each character from an input file/string. Each time that it encounters a known symbol or delimiter (such as white space), it returns the current string or symbol. The scanner only advances forward when the get token method is called.

The parser operates by calling the scanner to receive the next token. The parser then checks against the known grammar to determine whether or not the token is applicable to the language. If the token is applicable, it adds it to an abstract syntax tree, and if the token is not applicable it throws an error. The parser is also responsible for tracking identifiers, anything that is not a reserved word or literal, and their scopes as well as managing type checking for variables, parameters, and return statements.

The code generator consumes the abstract syntax tree and generates code for each pattern it encounters. This code is then emitted to an object file and linked with the run time functionality, creating an executable.

## Build Instructions
Necessary Packages: `git, cmake, clang, llvm, zlib1g-dev, make`

### Build Steps:
1.	From project directory, execute `cmake .` in terminal.
2.	From project directory, execute `make` in terminal.
3.	From project directory, execute `./Compiler_5183 ./path/to/source.src` in terminal to compile your source code.
If the source compiled successfully, you will be greeted with a message that says: “Program compiled to executable: <name>”
4.	From project directory, execute `./<name>` in terminal to run the compiled source code.

Note: I performed these steps on a clean Linux build to ensure that the compiler to be compiled successfully. So everything should work!

## Notes
If you want to see the generated IR code. Nativate to line 24 of code_generation.cpp and change the parameter on the `print_module_ll` function to true and recompile the compiler via `make`. This will output the IR code to the system out at source compile time.
