%option c++ prefix="p6_scanner_" outfile="p6_scanner.cpp"
%option stack noyywrap yylineno 8bit debug
%{ 
// make relevant includes before including the parser's tab file
#include <string>
#include <sstream>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include <cdk/types/balanced3_type.h>
#include <cdk/types/takum3_type.h>
#include "p6_parser.tab.h"

/* output stream for building string literals */
static std::ostringstream strlit;
static bool string_terminated = false;

#define BALANCED3_MAX 6078832729528464400LL
#define BALANCED3_MIN (-6078832729528464400LL)

/* don't change this */
#define yyerror LexerError
%}

SPACE       [ \t\r\n]
DIGIT       [[:digit:]]
LETTER      [A-Za-z]
IDENTIFIER  {LETTER}({LETTER}|{DIGIT})*

%x X_COMMENT
%x X_STRING X_SPECIAL

%%

                  { set_debug(false); yydebug = 0; }

  /* ====================================================================== */
  /* ====[                      3.2 Comentários                       ]==== */
  /* ====================================================================== */

"//".*             ; /* ignore line comments */

"/*"               yy_push_state(X_COMMENT);
<X_COMMENT>"/*"    yy_push_state(X_COMMENT);
<X_COMMENT>"*/"    yy_pop_state();
<X_COMMENT>.|\n    ; /* ignore comment content */
<X_COMMENT><<EOF>> yyerror("unterminated comment");

  /* ====================================================================== */
  /* ====[                  1   Tipos de dados                        ]==== */
  /* ====[                  4.1 Inferência de tipos                   ]==== */
  /* ====[                  4.5 Símbolos globais                      ]==== */
  /* ====================================================================== */

"int"              return tTYPE_INT;
"real"             return tTYPE_REAL;
"string"           return tTYPE_STRING;
"void"             return tTYPE_VOID;

"auto"             return tAUTO;

"extern"           return tEXTERN;
"forward"          return tFORWARD;
"public"           return tPUBLIC;

  /* ====================================================================== */
  /* ====[                  5.4 Função principal                      ]==== */
  /* ====================================================================== */

"begin"            return tBEGIN;
"end"              return tEND;

  /* ====================================================================== */
  /* ====[                  6.2 Instrução condicional                 ]==== */
  /* ====[                  6.3 Instrução de iteração                 ]==== */
  /* ====[                  6.4 Instrução de terminação               ]==== */
  /* ====[                  6.5 Instrução de continuação              ]==== */
  /* ====[                  6.6 Instrução de retorno                  ]==== */
  /* ====[                  6.8 Instrução de impressão                ]==== */
  /* ====================================================================== */

"if"               return tIF;
"elif"             return tELIF;
"else"             return tELSE;
"while"            return tWHILE;
"stop"             return tSTOP;
"next"             return tNEXT;
"return"           return tRETURN;
"!!"               return tPRINTLN;
"!"                return tPRINT;

// Teste prático
"unless"           return tUNLESS;
"iterate"          return tITERATE;
"for"              return tFOR;
"using"            return tUSING;

  /* ====================================================================== */
  /* ====[                  3.8.4 Ponteiros                           ]==== */
  /* ====[                  7.1.2 Leitura                             ]==== */
  /* ====[                  7.2.5 Expressão de dimensão               ]==== */
  /* ====================================================================== */

"null"             return tNULL;
"input"            return tINPUT;
"sizeof"           return tSIZEOF;

  /* ====================================================================== */
  /* ====[                    7 Expressões                            ]==== */
  /* ====================================================================== */

"->"               return tARROW;

">="               return tGE;
"<="               return tLE;
"=="               return tEQ;
"!="               return tNE;

"&&"               return tAND;
"||"               return tOR;

[+\-*/%?~]         return *yytext;
[<>=]              return *yytext;

  /* ====================================================================== */
  /* ====[                    3 Delimitadores e separadores           ]==== */
  /* ====================================================================== */

[\[\](){},;]       return *yytext;

  /* ====================================================================== */
  /* ====[                  3.7.1 Literais inteiros                   ]==== */
  /* ====================================================================== */

0t[0]+             yyerror("invalid zero integer literal");

0t[-0+]+           {
                     long long value = 0;

                     for (size_t i = 2; yytext[i] != '\0'; i++) {
                       int digit = 0;

                       if (yytext[i] == '+') {
                         digit = 1;
                       } else if (yytext[i] == '0') {
                         digit = 0;
                       } else if (yytext[i] == '-') {
                         digit = -1;
                       } else {
                         yyerror("invalid balanced ternary integer literal");
                       }

                       if (value > BALANCED3_MAX / 3 || value < BALANCED3_MIN / 3) {
                         yyerror("integer literal overflow");
                       }

                       long long next = value * 3;

                       if (digit > 0 && next > BALANCED3_MAX - digit) {
                         yyerror("integer literal overflow");
                       }

                       if (digit < 0 && next < BALANCED3_MIN - digit) {
                         yyerror("integer literal overflow");
                       }

                       value = next + digit;
                     }

                     yylval.b = new cdk::balanced3_type::value_type(value);
                     return tINTEGER;
                   }

0[0-8]*9[0-9]*     yyerror("invalid base-9 integer literal");

0[0-8]+            {
                     long long value = 0;

                     for (size_t i = 0; yytext[i] != '\0'; i++) {
                       int digit = yytext[i] - '0';

                       if (value > (BALANCED3_MAX - digit) / 9) {
                         yyerror("integer literal overflow");
                       }

                       value = value * 9 + digit;
                     }

                     if (value == 0) {
                       yyerror("invalid zero integer literal");
                     }

                     yylval.b = new cdk::balanced3_type::value_type(value);
                     return tINTEGER;
                   }

0|[1-9][0-9]*      {
                     long long value = 0;

                     for (size_t i = 0; yytext[i] != '\0'; i++) {
                       int digit = yytext[i] - '0';

                       if (value > (BALANCED3_MAX - digit) / 10) {
                         yyerror("integer literal overflow");
                       }

                       value = value * 10 + digit;
                     }

                     yylval.b = new cdk::balanced3_type::value_type(value);
                     return tINTEGER;
                   }

  /* ====================================================================== */
  /* ====[                  3.7.2 Literais reais                     ]==== */
  /* ====================================================================== */

([0-9]+\.[0-9]*|\.[0-9]+)([Ee][+-]?[0-9]+)? {
                     yylval.t = new cdk::takum3_type::value_type(yytext);
                     return tREAL;
                   }

[0-9]+[Ee][+-]?[0-9]+ {
                     yylval.t = new cdk::takum3_type::value_type(yytext);
                     return tREAL;
                   }

  /* ====================================================================== */
  /* ====[                  3.6 Identificadores                      ]==== */
  /* ====================================================================== */

{IDENTIFIER}       {
                     yylval.s = new std::string(yytext);
                     return tIDENTIFIER;
                   }

  /* ====================================================================== */
  /* ====[                  3.7.3 Cadeias de caracteres              ]==== */
  /* ====================================================================== */

"\""               {
                     strlit.str("");
                     strlit.clear();
                     string_terminated = false;
                     yy_push_state(X_STRING);
                   }

<X_STRING>"\\"     yy_push_state(X_SPECIAL);

<X_STRING>"\""     {
                     yylval.s = new std::string(strlit.str());
                     strlit.str("");
                     strlit.clear();
                     string_terminated = false;
                     yy_pop_state();
                     return tSTRING;
                   }

<X_STRING>\n       yyerror("newline in string");
<X_STRING><<EOF>>  yyerror("unterminated string");

<X_STRING>.        {
                     if (!string_terminated) {
                       strlit << *yytext;
                     }
                   }

<X_SPECIAL>n       {
                     if (!string_terminated) strlit << '\n';
                     yy_pop_state();
                   }

<X_SPECIAL>r       {
                     if (!string_terminated) strlit << '\r';
                     yy_pop_state();
                   }

<X_SPECIAL>t       {
                     if (!string_terminated) strlit << '\t';
                     yy_pop_state();
                   }

<X_SPECIAL>"\""    {
                     if (!string_terminated) strlit << '"';
                     yy_pop_state();
                   }

<X_SPECIAL>"\\"    {
                     if (!string_terminated) strlit << '\\';
                     yy_pop_state();
                   }

<X_SPECIAL>[0-7]{1,3} {
                     unsigned long value = std::stoul(yytext, nullptr, 8);

                     if (value > 255) {
                       yyerror("octal escape out of range");
                     }

                     if (value == 0) {
                       string_terminated = true;
                     } else if (!string_terminated) {
                       strlit << (char)value;
                     }

                     yy_pop_state();
                   }

<X_SPECIAL>.       yyerror("invalid escape sequence");
<X_SPECIAL><<EOF>> yyerror("unterminated string escape");

  /* ====================================================================== */
  /* ====[                  3.1 Caracteres brancos                    ]==== */
  /* ====================================================================== */

{SPACE}+           ; /* ignore whitespace */

  /* ====================================================================== */
  /* ====[                  Outros caracteres                         ]==== */
  /* ====================================================================== */

.                  yyerror(yytext);

  /* ====================================================================== */
  /* ====[                  THE END                                   ]==== */
  /* ====================================================================== */

%%
