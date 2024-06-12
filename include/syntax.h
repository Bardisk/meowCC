#ifndef MEOW_SYNTAX
#define MEOW_SYNTAX

#include <basics.h>
#include <lexer.h>

struct syntax_t;

typedef struct symbol_t {
  char name[64];
  int lineno;
  int size;
  struct syntax_t** child; 
} symbol_t;

typedef struct syntax_t {
  union {
    symbol_t symbol;  // 符号节点
    token_t token;    // 终结符节点，直接用 token_t
  };
  int type;
} syntax_t;

// type 的取值
#define SYMBOL 1
#define TOKEN 2

syntax_t *new_symbol(const char *name, int lineno, int size, ...);

void print_syntax_tree(syntax_t* node, int indent);

syntax_t* advance();
bool stepback();

syntax_t* program(bool last);
syntax_t* declaration_list(bool last);
syntax_t* declaration(bool last);
syntax_t* var_declaration(bool last);
syntax_t* fun_declaration(bool last);
syntax_t* params(bool last);
syntax_t* param_list(bool last);
syntax_t* param(bool last);
syntax_t* compound_stmt(bool last);
syntax_t* local_declarations(bool last);
syntax_t* statement_list(bool last);
syntax_t* statement(bool last);
syntax_t* expression_stmt(bool last);
syntax_t* selection_stmt(bool last);
syntax_t* iteration_stmt(bool last);
syntax_t* return_stmt(bool last);
syntax_t* expression(bool last);
syntax_t* var(bool last);
syntax_t* simple_expression(bool last);
syntax_t* additive_expression(bool last);
syntax_t* relop(bool last);
syntax_t* addop(bool last);
syntax_t* mulop(bool last);
syntax_t* term(bool last);
syntax_t* factor(bool last);
syntax_t* call(bool last);
syntax_t* args(bool last);
syntax_t* arg_list(bool last);

#endif