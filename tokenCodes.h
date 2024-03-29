//
// Created by Jacob Carlson on 1/28/21.
//

#ifndef COMPILER_5183_TOKENCODES_H
#define COMPILER_5183_TOKENCODES_H

// ------- Reserved Words: start -------
#define T_PROGRAM 150
#define T_IS 151
#define T_BEGIN 152
#define T_END 153
#define T_GLOBAL 154
#define T_PROCEDURE 155
#define T_RETURN 167

// -- VARIABLES --
#define T_VARIABLE 156
#define T_TYPE 157
#define T_INTEGER_TYPE 158
#define T_FLOAT_TYPE 159
#define T_STRING_TYPE 160
#define T_BOOL_TYPE 161
#define T_ENUM_TYPE 162

// -- VALUES --
#define T_INTEGER_LITERAL 178
#define T_FLOAT_LITERAL 179
#define T_STRING_LITERAL 180

#define T_IDENTIFIER 181
#define T_ENUM_LITERAL 182

// -- CONDITIONALS --
#define T_IF 163
#define T_THEN 164
#define T_ELSE 165
#define T_FOR 166

// -- LOGICAL SYMBOLS --
#define T_AND '&'      // ascii value - 38
#define T_OR '|'      // ascii value - 124
#define T_NOT 169
#define T_L_THAN 170   // <
#define T_G_THAN 171   // >
#define T_LE_THAN 172  // <=
#define T_GE_THAN 173  // >=
#define T_D_EQUALS 174 // ==
#define T_N_EQUALS 175 // !=

// -- MATH --
#define T_MULTIPLY '*' // ascii value - 42
#define T_DIVIDE '/'   // ascii value - 59
#define T_ADD '+'      // ascii value - 43
#define T_MINUS '-'    // ascii value - 45

// -- BOOLEAN --
#define T_TRUE 176
#define T_FALSE 177

// -- SYMBOLS
#define T_LPAREN '('    // ascii value - 40
#define T_RPAREN ')'    // ascii value - 41
#define T_LBRACE '{'    // ascii value - 123
#define T_RBRACE '}'    // ascii value - 125
#define T_LBRACKET '['  // ascii value - 91
#define T_RBRACKET ']'  // ascii value - 93
#define T_COMMA ','     // ascii value - 44
#define T_PERIOD '.'    // ascii value - 46
#define T_QUOTATION '"' // ascii value - 34
#define T_COLON ':' // ascii value - 58
#define T_SEMICOLON ';' // ascii value - 59
#define T_ASSIGN '='    // ascii value - 61


#define T_BLOCK_COMMENT_OPEN 185
#define T_BLOCK_COMMENT_CLOSE 186
#define T_END_OF_FILE 187
#define T_COLON_EQUALS 188

#define T_GET_BOOL 190
#define T_GET_INTEGER 191
#define T_GET_FLOAT 192
#define T_GET_STRING 193
#define T_PUT_BOOL 194
#define T_PUT_INTEGER 195
#define T_PUT_FLOAT 196
#define T_PUT_STRING 197
#define T_SQRT 198

// ------- Reserved Words: end -------


// ------- Parser Codes: Start -------

#define T_EMPTY_STRING 298

#define T_PROGRAM_ROOT 300
#define T_PROGRAM_DECLARATION_BLOCK 301
#define T_PROGRAM_STATEMENT_BLOCK 302

#define T_EXPRESSION 310
#define T_ARITH_OP 311
#define T_TERM 312
#define T_RELATION 313
#define T_FACTOR 315

#define T_VARIABLE_DECLARATION 320
#define T_VARIABLE_ASSIGNMENT 321

#define T_TYPE_DECLARATION 325
#define T_TYPE_DEF 326
#define T_TYPE_MARK 326

#define T_PROCEDURE_DECLARATION 330
#define T_PARAMETER_LIST 331
#define T_PARAMETER 332
#define T_RETURN_BLOCK 332
#define T_PROCEDURE_DECLARATION_BLOCK 333
#define T_PROCEDURE_STATEMENT_BLOCK 334
#define T_PROCEDURE_CALL 335

#define T_IF_BLOCK 340
#define T_IF_STATEMENT_BLOCK 341
#define T_FOR_LOOP 345
#define T_FOR_LOOP_STATEMENT_BLOCK 345

#endif //COMPILER_5183_TOKENCODES_H
