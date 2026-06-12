%{
//-- don't change *any* of these: if you do, you'll break the compiler.
#include <algorithm>
#include <memory>
#include <cstring>
#include <vector>
#include <cdk/compiler.h>
#include <cdk/types/types.h>
#include ".auto/all_nodes.h"
#define LINE                         compiler->scanner()->lineno()
#define yylex()                      compiler->scanner()->scan()
#define yyerror(compiler, s)         compiler->scanner()->error(s)
//-- don't change *any* of these --- END!

#define EMPTY (new cdk::sequence_node(LINE))
%}

%parse-param {std::shared_ptr<cdk::compiler> compiler}

%union {
  //--- don't change *any* of these: if you do, you'll break the compiler.
  YYSTYPE() : type(cdk::primitive_type::create(0, cdk::TYPE_VOID)) {}
  ~YYSTYPE() {}
  YYSTYPE(const YYSTYPE &other) { *this = other; }
  YYSTYPE& operator=(const YYSTYPE &other) { type = other.type; return *this; }

  std::shared_ptr<cdk::basic_type> type;
  //-- don't change *any* of these --- END!

  int                   i;          /* integer value */
  cdk::balanced3_type::value_type *b; /* ternary integer value */
  cdk::takum3_type::value_type *t;  /* ternary real value */
  std::string          *s;          /* symbol name or string literal */
  cdk::basic_node      *node;       /* node pointer */
  cdk::sequence_node   *sequence;
  cdk::expression_node *expression; /* expression nodes */
  cdk::lvalue_node     *lvalue;
  p6::block_node       *block;

  std::vector<std::shared_ptr<cdk::basic_type>> *types;
};

%token <b> tINTEGER
%token <t> tREAL
%token <s> tIDENTIFIER tSTRING

%token tTYPE_INT tTYPE_REAL tTYPE_STRING tTYPE_VOID
%token tAUTO
%token tPUBLIC tFORWARD tEXTERN
%token tBEGIN tEND
%token tIF tELIF tELSE tWHILE
%token tSTOP tNEXT tRETURN
%token tPRINT tPRINTLN
%token tINPUT tNULL tSIZEOF
%token tARROW
%token tGE tLE tEQ tNE
%token tAND tOR

// Teste prático
%token tUNLESS tITERATE tFOR tUSING

%nonassoc tIFX
%nonassoc tELIF
%nonassoc tELSE

%right '='
%left tOR
%left tAND
%left tEQ tNE
%left '<' '>' tGE tLE
%left '+' '-'
%left '*' '/' '%'
%nonassoc tUNARY
%nonassoc '['

%type <sequence> file decls local_decls stmts exprs opt_exprs args opt_args
%type <node> program decl local_decl global_var_decl local_var_decl stmt if_tail function_definition
%type <expression> expr opt_initializer
%type <lvalue> lval
%type <block> block
%type <type> data_type variable_type primitive_type void_type function_type non_void_type
%type <types> types
%type <i> qualifier
%type <s> string_literal

%{
//-- The rules below will be included in yyparse, the main parsing function.
%}
%%

file : /* empty */   { compiler->ast($$ = EMPTY); }
     | decls         { compiler->ast($$ = $1); }
     | decls program { compiler->ast($$ = new cdk::sequence_node(LINE, $2, $1)); }
     | program       { compiler->ast($$ = new cdk::sequence_node(LINE, $1)); }
     ;

program : tBEGIN tEND                   { $$ = new p6::program_node(LINE, new p6::block_node(LINE, EMPTY, EMPTY)); }
        | tBEGIN local_decls tEND       { $$ = new p6::program_node(LINE, new p6::block_node(LINE, $2, EMPTY)); }
        | tBEGIN stmts tEND             { $$ = new p6::program_node(LINE, new p6::block_node(LINE, EMPTY, $2)); }
        | tBEGIN local_decls stmts tEND { $$ = new p6::program_node(LINE, new p6::block_node(LINE, $2, $3)); }
        ;

decls : decl       { $$ = new cdk::sequence_node(LINE, $1); }
      | decls decl { $$ = new cdk::sequence_node(LINE, $2, $1); }
      ;

decl : function_definition                         { $$ = $1; }
     | qualifier function_type tIDENTIFIER ';'     { $$ = new p6::function_declaration_node(LINE, $1, $2, *$3); delete $3; }
     | function_type tIDENTIFIER ';'               { $$ = new p6::function_declaration_node(LINE, 0, $1, *$2); delete $2; }
     | global_var_decl                             { $$ = $1; }
     ;

global_var_decl : variable_type tIDENTIFIER opt_initializer ';'           { $$ = new p6::variable_declaration_node(LINE, 0, $1, *$2, $3); delete $2; }
                | qualifier variable_type tIDENTIFIER opt_initializer ';' { $$ = new p6::variable_declaration_node(LINE, $1, $2, *$3, $4); delete $3; }
                | tAUTO tIDENTIFIER '=' expr ';'                          { $$ = new p6::variable_declaration_node(LINE, 0, nullptr, *$2, $4); delete $2; }
                | qualifier tAUTO tIDENTIFIER '=' expr ';'                { $$ = new p6::variable_declaration_node(LINE, $1, nullptr, *$3, $5); delete $3; }
                | qualifier tIDENTIFIER '=' expr ';'                      { $$ = new p6::variable_declaration_node(LINE, $1, nullptr, *$2, $4); delete $2; }
                ;

local_var_decl : variable_type tIDENTIFIER opt_initializer ';' { $$ = new p6::variable_declaration_node(LINE, 0, $1, *$2, $3); delete $2; }
               | tAUTO tIDENTIFIER '=' expr ';'                { $$ = new p6::variable_declaration_node(LINE, 0, nullptr, *$2, $4); delete $2; }
               ;

local_decls : local_decl             { $$ = new cdk::sequence_node(LINE, $1); }
            | local_decls local_decl { $$ = new cdk::sequence_node(LINE, $2, $1); }
            ;

local_decl : local_var_decl                { $$ = $1; }
           | function_type tIDENTIFIER ';' { $$ = new p6::variable_declaration_node(LINE, 0, $1, *$2, nullptr); delete $2; }
           ;

qualifier : tPUBLIC  { $$ = tPUBLIC; }
          | tFORWARD { $$ = tFORWARD; }
          | tEXTERN  { $$ = tEXTERN; }
          ;

opt_initializer : /* empty */ { $$ = nullptr; }
                | '=' expr    { $$ = $2; }
                ;

primitive_type : tTYPE_INT    { $$ = cdk::primitive_type::create(8, cdk::TYPE_BALANCED3); }
               | tTYPE_REAL   { $$ = cdk::primitive_type::create(16, cdk::TYPE_TAKUM3); }
               | tTYPE_STRING { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING); }
               ;

void_type : tTYPE_VOID { $$ = cdk::primitive_type::create(0, cdk::TYPE_VOID); }
          ;

variable_type : primitive_type    { $$ = $1; }
              | '[' data_type ']' { $$ = cdk::reference_type::create(4, $2); }
              ;

non_void_type : variable_type { $$ = $1; }
              | function_type { $$ = $1; }
              ;

data_type : non_void_type { $$ = $1; }
          | void_type     { $$ = $1; }
          ;

function_type : data_type '<' '>'       { $$ = cdk::functional_type::create($1); }
              | data_type '<' types '>' { $$ = cdk::functional_type::create(*$3, $1); $3->clear(); delete $3; }
              ;

types : non_void_type           { $$ = new std::vector<std::shared_ptr<cdk::basic_type>>(); $$->push_back($1); }
      | types ',' non_void_type { $$ = $1; $$->push_back($3); }
      ;

function_definition : tIDENTIFIER '(' opt_args ')' tARROW data_type block           { $$ = new p6::function_definition_node(LINE, 0, $6, *$1, $3, $7); delete $1; }
                    | qualifier tIDENTIFIER '(' opt_args ')' tARROW data_type block { $$ = new p6::function_definition_node(LINE, $1, $7, *$2, $4, $8); delete $2; }
                    ;

opt_args : /* empty */ { $$ = EMPTY; }
         | args        { $$ = $1; }
         ;

args : variable_type tIDENTIFIER          { $$ = new cdk::sequence_node(LINE, new p6::variable_declaration_node(LINE, 0, $1, *$2, nullptr)); delete $2; }
     | args ',' variable_type tIDENTIFIER { $$ = new cdk::sequence_node(LINE, new p6::variable_declaration_node(LINE, 0, $3, *$4, nullptr), $1); delete $4; }
     ;

stmts : stmt       { $$ = new cdk::sequence_node(LINE, $1); }
      | stmts stmt { $$ = new cdk::sequence_node(LINE, $2, $1); }
      ;

stmt : expr ';'                         { $$ = new p6::evaluation_node(LINE, $1); }
     | exprs tPRINT                     { $$ = new p6::print_node(LINE, $1, false); }
     | exprs tPRINTLN                   { $$ = new p6::print_node(LINE, $1, true); }
     | tWHILE '(' expr ')' stmt         { $$ = new p6::while_node(LINE, $3, $5); }
     | tIF '(' expr ')' stmt if_tail    { if ($6) { $$ = new p6::if_else_node(LINE, $3, $5, $6); } else { $$ = new p6::if_node(LINE, $3, $5); } }
     | tSTOP ';'                        { $$ = new p6::stop_node(LINE); }
     | tSTOP tINTEGER ';'               { $$ = new p6::stop_node(LINE, static_cast<size_t>($2->to_int())); delete $2; }
     | tNEXT ';'                        { $$ = new p6::next_node(LINE); }
     | tNEXT tINTEGER ';'               { $$ = new p6::next_node(LINE, static_cast<size_t>($2->to_int())); delete $2; }
     | tRETURN ';'                      { $$ = new p6::return_node(LINE, nullptr); }
     | tRETURN expr ';'                 { $$ = new p6::return_node(LINE, $2); }
     | block                            { $$ = $1; }
     // Teste prático
     | tUNLESS expr tITERATE expr tFOR expr tUSING tIDENTIFIER { $$ = new p6::conditional_iterate_node(LINE, new cdk::not_node(LINE, $2), $4, $6, *$8); delete $8; }
     ;

if_tail : /* empty */ %prec tIFX                  { $$ = nullptr; }
        | tELSE stmt                              { $$ = $2; }
        | tELIF '(' expr ')' stmt if_tail         { if ($6) { $$ = new p6::if_else_node(LINE, $3, $5, $6); } else { $$ = new p6::if_node(LINE, $3, $5); } }
        ;

block : '{' '}'                   { $$ = new p6::block_node(LINE, EMPTY, EMPTY); }
      | '{' local_decls '}'       { $$ = new p6::block_node(LINE, $2, EMPTY); }
      | '{' stmts '}'             { $$ = new p6::block_node(LINE, EMPTY, $2); }
      | '{' local_decls stmts '}' { $$ = new p6::block_node(LINE, $2, $3); }
      ;

exprs : expr           { $$ = new cdk::sequence_node(LINE, $1); }
      | exprs ',' expr { $$ = new cdk::sequence_node(LINE, $3, $1); }
      ;

opt_exprs : /* empty */ { $$ = EMPTY; }
          | exprs       { $$ = $1; }
          ;

expr : tINTEGER                      { $$ = new cdk::balanced3_node(LINE, *$1); delete $1; }
     | tREAL                         { $$ = new cdk::takum3_node(LINE, *$1); delete $1; }
     | string_literal                { $$ = new cdk::string_node(LINE, *$1); delete $1; }
     | tNULL                         { $$ = new p6::null_node(LINE); }
     /* LEFT VALUES */
     | lval                          { $$ = new cdk::rvalue_node(LINE, $1); }
     /* ASSIGNMENTS */
     | lval '=' expr                 { $$ = new cdk::assignment_node(LINE, $1, $3); }
     /* ARITHMETIC EXPRESSIONS */
     | expr '+' expr                 { $$ = new cdk::add_node(LINE, $1, $3); }
     | expr '-' expr                 { $$ = new cdk::sub_node(LINE, $1, $3); }
     | expr '*' expr                 { $$ = new cdk::mul_node(LINE, $1, $3); }
     | expr '/' expr                 { $$ = new cdk::div_node(LINE, $1, $3); }
     | expr '%' expr                 { $$ = new cdk::mod_node(LINE, $1, $3); }
     /* LOGICAL EXPRESSIONS */
     | expr '<' expr                 { $$ = new cdk::lt_node(LINE, $1, $3); }
     | expr '>' expr                 { $$ = new cdk::gt_node(LINE, $1, $3); }
     | expr tGE expr                 { $$ = new cdk::ge_node(LINE, $1, $3); }
     | expr tLE expr                 { $$ = new cdk::le_node(LINE, $1, $3); }
     | expr tNE expr                 { $$ = new cdk::ne_node(LINE, $1, $3); }
     | expr tEQ expr                 { $$ = new cdk::eq_node(LINE, $1, $3); }
     | expr tAND expr                { $$ = new cdk::and_node(LINE, $1, $3); }
     | expr tOR expr                 { $$ = new cdk::or_node(LINE, $1, $3); }
     /* UNARY EXPRESSIONS */
     | '-' expr %prec tUNARY         { $$ = new cdk::unary_minus_node(LINE, $2); }
     | '+' expr %prec tUNARY         { $$ = new cdk::unary_plus_node(LINE, $2); }
     | '~' expr %prec tUNARY         { $$ = new cdk::not_node(LINE, $2); }
     /* OTHER EXPRESSIONS */
     | tINPUT                        { $$ = new p6::input_node(LINE); }
     | tSIZEOF '(' expr ')'          { $$ = new p6::sizeof_node(LINE, $3); }
     | '[' expr ']'                  { $$ = new p6::stack_alloc_node(LINE, $2); }
     | lval '?' %prec tUNARY         { $$ = new p6::address_of_node(LINE, $1); }
     /* FUNCTION CALLS */
     | tIDENTIFIER '(' opt_exprs ')' { $$ = new p6::function_call_node(LINE, *$1, $3); delete $1; }
     /* PARENTHESIZED EXPRESSIONS */
     | '(' expr ')'                  { $$ = $2; }
     ;

lval : tIDENTIFIER                  { $$ = new cdk::variable_node(LINE, *$1); delete $1; }
     | expr '[' expr ']' %prec '['  { $$ = new p6::index_node(LINE, $1, $3); }
     ;

string_literal : tSTRING                { $$ = $1; }
               | string_literal tSTRING { *$1 += *$2; delete $2; $$ = $1; }
               ;

%%
