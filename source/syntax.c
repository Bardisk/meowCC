#include <lexer.h>
#include <syntax.h>

extern token_t **token_list;
extern int token_cnt;
int current_token_cnt = 0;
// static token_t* current_token;

#define current_token (token_list[current_token_cnt])
#define next_token (token_list[current_token_cnt + 1])
#define line_number (current_token->lineno)
#define istyp(TYPE) (strcmp(current_token->name, #TYPE) == 0)
#define istoktyp(TOKEN, TYPE) (strcmp(token_list[current_token_cnt + TOKEN]->name, #TYPE) == 0)
#define isnxttyp(TYPE) (strcmp(next_token->name, #TYPE) == 0)

#define SAVE_CONT int cont = current_token_cnt
#define RESTORE_CONT current_token_cnt = cont
#define NONLAST_FAIL do {\
  assert(!last);\
  current_token_cnt = cont;\
  return NULL;\
}while(0)
#define TOKEN_UNMATCH(token) do {\
  if (last) {\
    syn_error(line_number, "expected " #token, __func__);\
  }\
  NONLAST_FAIL;\
}while(0)
#define MALFORM do {\
  if (last) {\
    syn_error(line_number, "malformed", __func__);\
  }\
  return NULL;\
}while(0)

void syn_error(int lineno, const char *cause, const char *sym) {
  fprintf(stderr, "Syntax error at line %d (%s in %s)\n", lineno, cause, sym);
  exit(-1);
}

syntax_t* advance() {
  // if (current_token_cnt == token_cnt - 1)
  //   return false;
  syntax_t *res = (syntax_t *) malloc(sizeof(syntax_t));
  res->type = TOKEN;
  res->token = *current_token;
  ++current_token_cnt;
  assert(current_token_cnt <= token_cnt);
  // current_token = token_list[++current_token_cnt];
  return res;
}

syntax_t *new_symbol(const char *name, int lineno, int size, ...) {
  syntax_t *ret = (syntax_t *) malloc(sizeof(syntax_t));
  ret->type = SYMBOL;  // 设置为符号类型

  strcpy(ret->symbol.name, name);  // 设置符号名称
  ret->symbol.lineno = lineno;  // 设置符号所在行

  ret->symbol.size = size;  // 子节点数量
  if (!size) {
    return ret;
  }
  ret->symbol.child = (syntax_t **) malloc(size * sizeof(syntax_t*));  // 分配内存存储子节点

  va_list args;
  va_start(args, size);
  for (int i = 0; i < size; i++) {
    syntax_t* child = va_arg(args, syntax_t*);
    ret->symbol.child[i] = child;  // 存储子节点
  }
  va_end(args);

  return ret;
}

void print_syntax_tree(syntax_t* node, int indent) {
  if (node == NULL) return;
  for (int i = 0; i < indent; i++) printf("  ");  // 打印缩进

  if (node->type == TOKEN) {
    printf("%s: %s\n", node->token.name, node->token.value);  // 打印词法单元信息
    return;
  }

  assert(node->type == SYMBOL);
  printf("%s (%d)\n", node->symbol.name, node->symbol.lineno);  // 打印符号信息

  for (int i = 0; i < node->symbol.size; i++)
  print_syntax_tree(node->symbol.child[i], indent + 1);  // 递归打印子节点
}

syntax_t *relop(bool last) {
  if (strcmp(current_token->name, "NEQ") == 0
    || strcmp(current_token->name, "EQUAL") == 0
    || strcmp(current_token->name, "LESS") == 0
    || strcmp(current_token->name, "GREAT") == 0
    || strcmp(current_token->name, "GEQ") == 0
    || strcmp(current_token->name, "LEQ") == 0) {
    syntax_t *token = advance();
    return new_symbol("relop", token->token.lineno, 1, token);
  }
  else {
    MALFORM;
  }
}

syntax_t *addop(bool last) {
  if (istyp(PLUS) || istyp(MINUS)) {
    syntax_t *token = advance();
    return new_symbol("addop", token->token.lineno, 1, token);
  }
  else {
    MALFORM;
  }
}

syntax_t *mulop(bool last) {
  if (istyp(DIV) || istyp(STAR)) {
    syntax_t *token = advance();
    return new_symbol("mulop", token->token.lineno, 1, token);
  }
  else {
    MALFORM;
  }
}

syntax_t* factor(bool last) {
  SAVE_CONT;
  if (istyp(INT)) {
    // factor -> NUM
    syntax_t *token = advance();
    return new_symbol("factor", token->token.lineno, 1, token);
  } else if (istyp(LP)) {
    // factor -> ( expression )
    syntax_t *lp, *expr, *rp;
    lp = advance();
    expr = expression(last);
    // expression fail
    if (expr == NULL) {
      NONLAST_FAIL;
    }
    if (istyp(RP)) {
      rp = advance();
    } else {
      TOKEN_UNMATCH(RP);
    }
    return new_symbol("factor", lp->token.lineno, 3, lp, expr, rp);
  } else if (istyp(ID) && isnxttyp(LP)) {
    // factor -> call
    syntax_t *call_0 = call(last);
    if (call_0 == NULL) {
      assert(cont == current_token_cnt);
      NONLAST_FAIL;
    }
    return new_symbol("factor", call_0->symbol.lineno, 1, call_0);
  } else if (istyp(ID)) {
    // factor -> var
    syntax_t *var_0 = var(last);
    if (var_0 == NULL) {
      assert(cont == current_token_cnt);
      NONLAST_FAIL;
    }
    return new_symbol("factor", var_0->symbol.lineno, 1, var_0);
  } else {
    // no rule available
    MALFORM;
  }
}

syntax_t *var(bool last) {
  SAVE_CONT;
  if (!istyp(ID)) {
    TOKEN_UNMATCH(ID);
  }
  syntax_t *id = advance();
  if (istyp(LB)) {
    // var -> ID [ expression ]
    syntax_t *lb, *expr, *rb;
    lb = advance();
    expr = expression(last);
    if (expr == NULL) {
      NONLAST_FAIL;
    }
    if (istyp(RB)) {
      rb = advance();
    } else {
      TOKEN_UNMATCH(RB);
    }
    return new_symbol("var", id->token.lineno, 4, id, lb, expr, rb);
  } else {
    // var -> ID
    return new_symbol("var", id->token.lineno, 1, id);
  }
}

// term -> factor termlist
// termlist -> mulop factor termlist | empty
syntax_t *term(bool last) {
  SAVE_CONT;
  syntax_t *left = factor(last);
  if (left == NULL) {
    NONLAST_FAIL;
  }
  // not necessary
  syntax_t *mul = mulop(false);
  syntax_t *fac;
  if (mul == NULL) fac = NULL;
  else fac = factor(false);
  while (mul != NULL && fac != NULL) {
    left = new_symbol("term", left->symbol.lineno, 3, left, mul, fac);
    mul = mulop(false);
    if (mul == NULL) fac = NULL;
    else fac = factor(false);
  } 
  // we may get a redundant mulop finally
  if (mul != NULL) {
    // we dont care the memory leak
    // free(mul);
    --current_token_cnt;
  }
  if (strcmp(left->symbol.name, "factor") == 0) {
    // term -> factor
    return new_symbol("term", left->symbol.lineno, 1, left);
  } else {
    // term -> factor termlist
    return left;
  }
}

syntax_t *call(bool last) {
  // call -> ID ( args )
  syntax_t *id, *lp, *args_0, *rp;
  SAVE_CONT;
  if (!istyp(ID)) {
    TOKEN_UNMATCH(ID);
  }
  id = advance();
  if (!istyp(LP)) {
    TOKEN_UNMATCH(LP);
  }
  lp = advance();
  args_0 = args(last);
  if (args_0 == NULL) {
    NONLAST_FAIL;
  }
  if (!istyp(RP)) {
    TOKEN_UNMATCH(RP);
  }
  rp = advance();
  return new_symbol("call", id->token.lineno, 4, id, lp, args_0, rp);
}

syntax_t *args(bool last) {
  SAVE_CONT;
  if (istyp(RP)) {
    // args -> empty
    return new_symbol("args", line_number, 0);
  } else {
    // args -> arg_list
    syntax_t *al = arg_list(last);
    if (al == NULL) {
      NONLAST_FAIL;
    }
    return new_symbol("args", al->symbol.lineno, 1, al);
  }
}

syntax_t *arg_list(bool last) {
  SAVE_CONT;
  syntax_t *left = expression(last);
  if (left == NULL) {
    NONLAST_FAIL;
  }
  syntax_t *com, *exp;
  if (istyp(COMMA)) {
    com = advance();
    exp = expression(true);
  } else {
    com = exp = NULL;
  }
  while (com && exp) {
    left = new_symbol("arg_list", left->symbol.lineno, 3, left, com, exp);
    if (istyp(COMMA)) {
      com = advance();
      exp = expression(true);
    } else {
      com = exp = NULL;
    }
  }
  // redundant comma
  if (com) {
    // free(com)
    current_token_cnt--;
  }
  if (strcmp(left->symbol.name, "expression") == 0) {
    // arg_list -> expression
    return new_symbol("arg_list", left->symbol.lineno, 1, left);
  } else {
    // arg_list -> arg_list comma expression
    return left;
  }
}

syntax_t* param_list(bool last) {
  SAVE_CONT;
  syntax_t *left = param(last);
  if (left == NULL) {
    NONLAST_FAIL;
  }
  syntax_t *com, *par;
  if (istyp(COMMA)) {
    com = advance();
    // must...
    par = param(last);
  } else {
    com = par = NULL;
  }
  while (com && par) {
    left = new_symbol("param_list", left->symbol.lineno, 3, left, com, par);
    if (istyp(COMMA)) {
      com = advance();
      par = param(last);
    } else {
      com = par = NULL;
    }
  }
  // redundant comma
  if (com) {
    // free(com)
    current_token_cnt--;
  }
  if (strcmp(left->symbol.name, "param") == 0) {
    // param_list -> param
    return new_symbol("param_list", left->symbol.lineno, 1, left);
  } else {
    // param_list -> param_list comma param
    return left;
  }
}

syntax_t* expression(bool last) {
  SAVE_CONT;
  if (istyp(ID) && !isnxttyp(LP)) {
    // expression -> var = expression
    // expression -> simple_expression -> var ....
    syntax_t *var_0, *assign, *expr;
    var_0 = var(last);
    if (var_0 == NULL) {
      NONLAST_FAIL;
    }
    if (!istyp(ASSIGN)) {
      // expression -> simple_expression -> var ....
      // TOKEN_UNMATCH(ASSIGN);
      RESTORE_CONT;
      syntax_t *sexpr = simple_expression(last);
      if (sexpr == NULL) {
        NONLAST_FAIL;
      }
      return new_symbol("expression", sexpr->symbol.lineno, 1, sexpr);
    }
    assign = advance();
    // recursive...
    expr = expression(last);
    if (expr == NULL) {
      NONLAST_FAIL;
    }
    return new_symbol("expression", var_0->symbol.lineno, 3, var_0, assign, expr);
  } else {
    // expression -> simple expression
    syntax_t *sexpr = simple_expression(last);
    if (sexpr == NULL) {
      NONLAST_FAIL;
    }
    return new_symbol("expression", sexpr->symbol.lineno, 1, sexpr);
  }
}

syntax_t* simple_expression(bool last) {
  SAVE_CONT;
  syntax_t *ae = additive_expression(last);
  if (ae == NULL) {
    NONLAST_FAIL;
  }
  syntax_t *rel = relop(false);
  if (rel == NULL) {
    // simple_expression -> additive_expression
    return new_symbol("simple_expression", ae->symbol.lineno, 1, ae);
  } else {
    syntax_t *ae2 = additive_expression(false);
    if (ae2 == NULL) {
      // simple_expression -> additive_expression
      // give back the relop
      // actually this will result in a failure afterwards...
      --current_token_cnt;
      return new_symbol("simple_expression", ae->symbol.lineno, 1, ae);
    } else {
      // simple_expression -> additive_expression relop additive_expression
      return new_symbol("simple_expression", ae->symbol.lineno, 3, ae, rel, ae2);
    } 
  }
}

syntax_t* additive_expression(bool last) {
  SAVE_CONT;
  // leftmost term (necessary)
  syntax_t *left = term(last);
  if (left == NULL) {
    NONLAST_FAIL;
  }

  // not necessary
  syntax_t *add = addop(false);
  syntax_t *ter;
  if (add == NULL) ter = NULL;
  else ter = term(false);
  while (ter != NULL && add != NULL) {
    left = new_symbol("additive_expression", left->symbol.lineno, 3, left, add, ter);
    add = addop(false);
    if (add == NULL) ter = NULL;
    else ter = term(false);
  } 
  // we may get a redundant addop finally
  if (add != NULL) {
    // we dont care the memory leak
    // free(add);
    --current_token_cnt;
  }
  if (strcmp(left->symbol.name, "term") == 0) {
    // additive_expression -> term
    return new_symbol("additive_expression", left->symbol.lineno, 1, left);
  } else {
    // additive_expression -> term additive_expression_list
    return left;
  }
}

// caution: it returns a token node!!!
// syntax_t* type_specifier(bool last) {
//   if (istyp(TYPE)) {
//     return advance();
//   } else {
//     MALFORM;
//   }
// }

syntax_t *param(bool last) {
  // param -> type ID | type ID []
  SAVE_CONT;
  syntax_t *type, *id, *lb, *rb;
  if (!istyp(TYPE)) {
    TOKEN_UNMATCH(TYPE);
  }
  type = advance();
  if (!istyp(ID)) {
    TOKEN_UNMATCH(ID);
  }
  id = advance();
  if (istyp(LB) && isnxttyp(RB)) {
    // param -> type ID []
    lb = advance();
    rb = advance();
    return new_symbol("param", type->token.lineno, 4, type, id, lb, rb);
  } else {
    // param -> type ID
    return new_symbol("param", type->token.lineno, 2, type, id);
  }
}

syntax_t *return_stmt(bool last) {
  // return_stmt -> return ; | return expression ;
  SAVE_CONT;
  if (!istyp(RETURN)) {
    TOKEN_UNMATCH(RETURN);
  }
  syntax_t *ret, *exp, *semi;
  ret = advance();
  if (istyp(SEMI)) {
    // return_stmt -> return ;
    semi = advance();
    return new_symbol("return_stmt", ret->token.lineno, 2, ret, semi);
  } else {
    // return_stmt -> return expression ;
    exp = expression(last);
    if (exp == NULL) {
      NONLAST_FAIL;
    }
    if (!istyp(SEMI)) {
      TOKEN_UNMATCH(SEMI);
    }
    semi = advance();
    return new_symbol("return_stmt", ret->token.lineno, 3, ret, exp, semi);
  }
}

syntax_t* expression_stmt(bool last) {
  SAVE_CONT;
  if (istyp(SEMI)) {
    // expression_stmt -> ;
    syntax_t *semi = advance();
    return new_symbol("expression_stmt", semi->token.lineno, 1, semi);
  } else {
    // expression_stmt -> expression ;
    syntax_t *exp, *semi;
    exp = expression(last);
    if (exp == NULL) {
      NONLAST_FAIL;
    }
    if (!istyp(SEMI)) {
      TOKEN_UNMATCH(SEMI);
    }
    semi = advance();
    return new_symbol("expression_stmt", exp->symbol.lineno, 2, exp, semi);
  }
}

syntax_t* iteration_stmt(bool last) {
  // iteration_stmt -> while ( expression ) statement
  SAVE_CONT;
  syntax_t *w, *lp, *exp, *rp, *stmt;

  if (!istyp(WHILE)) {
    TOKEN_UNMATCH(WHILE);
  }
  w = advance();

  if (!istyp(LP)) {
    TOKEN_UNMATCH(LP);
  }
  lp = advance();

  exp = expression(last);
  if (exp == NULL) {
    NONLAST_FAIL;
  }

  if (!istyp(RP)) {
    TOKEN_UNMATCH(RP);
  }
  rp = advance();

  stmt = statement(last);
  if (stmt == NULL) {
    NONLAST_FAIL;
  }

  return new_symbol("iteration_stmt", w->token.lineno, 5, w, lp, exp, rp, stmt);
}

syntax_t* params(bool last) {
  SAVE_CONT;
  if (istyp(TYPE) && strcmp(current_token->value, "void") == 0 && isnxttyp(RP)) {
    // params -> void
    syntax_t *v = advance();
    return new_symbol("params", v->token.lineno, 1, v);
  } else {
    syntax_t *pl = param_list(last);
    if (pl == NULL) {
      NONLAST_FAIL;
    } else {
      return new_symbol("params", pl->symbol.lineno, 1, pl);
    }
  }
}

// selection_stmt -> if ( expression ) statement else statement
// selection_stmt -> if ( expression ) statement
syntax_t* selection_stmt(bool last) {
  SAVE_CONT;
  syntax_t *i, *e, *stmt1, *stmt2, *lp, *exp, *rp;
  if (!istyp(IF)) {
    TOKEN_UNMATCH(IF);
  }
  i = advance();
  if (!istyp(LP)) {
    TOKEN_UNMATCH(LP);
  }
  lp = advance();
  exp = expression(last);
  if (exp == NULL) {
    NONLAST_FAIL;
  }
  if (!istyp(RP)) {
    TOKEN_UNMATCH(RP);
  }
  rp = advance();
  
  stmt1 = statement(last);
  if (stmt1 == NULL) {
    NONLAST_FAIL;
  }

  if (!istyp(ELSE)) {
    // TOKEN_UNMATCH(ELSE);
    return new_symbol("selection-statement", i->token.lineno, 5, i, lp, exp, rp, stmt1);
  } else {
    e = advance();
    stmt2 = statement(last);
    if (stmt2 == NULL) {
      NONLAST_FAIL;
    }
    return new_symbol("selection-statement", i->token.lineno, 7, i, lp, exp, rp, stmt1, e, stmt2);
  }

}

syntax_t* compound_stmt(bool last) {
  SAVE_CONT;
  syntax_t *lc, *rc, *ld, *stmtl;
  if (!istyp(LC)) {
    TOKEN_UNMATCH(LC);
  }
  lc = advance();
  ld = local_declarations(last);
  if (ld == NULL) {
    NONLAST_FAIL;
  }
  stmtl = statement_list(last);
  if (stmtl == NULL) {
    NONLAST_FAIL;
  }
  if (!istyp(RC)) {
    TOKEN_UNMATCH(RC);
  }
  rc = advance();
  return new_symbol("compound_stmt", lc->token.lineno, 4, lc, ld, stmtl, rc);
}

syntax_t* local_declarations(bool last) {
  syntax_t *vd = var_declaration(false);
  if (vd == NULL) {
    // empty
    return new_symbol("local_declarations", line_number, 0);
  } else {
    syntax_t *ld = local_declarations(last);
    // emptiable
    assert(ld != NULL);
    // ld is empty
    if (ld->symbol.size == 0) {
      free(ld);
      return new_symbol("local_declarations", vd->symbol.lineno, 1, vd);
    }
    return new_symbol("local_declarations", vd->symbol.lineno, 2, vd, ld);
  }
}


syntax_t* statement(bool last) {
  syntax_t *stmt;
  if (istyp(RETURN)) {
    // return_stmt
    stmt = return_stmt(last);
  } else if (istyp(WHILE)) {
    // while_stmt
    stmt = iteration_stmt(last);
  } else if (istyp(LC)) {
    // compound_stmt
    stmt = compound_stmt(last);
  } else if (istyp(IF)) {
    // selection_stmt
    stmt = selection_stmt(last);
  } else {
    // expression_stmt
    stmt = expression_stmt(last);
  }
  if (stmt == NULL) {
    assert(!last);
    return NULL;
  }
  return new_symbol("statement", stmt->symbol.lineno, 1, stmt);
}

syntax_t* program(bool last) {
  SAVE_CONT;
  syntax_t *dl = declaration_list(last);
  // actually, this wont happen
  if (dl == NULL) {
    NONLAST_FAIL;
  }
  return new_symbol("program", dl->symbol.lineno, 1, dl);
}

// declaration_list -> declaration declaration_list | declaration
syntax_t* declaration_list(bool last) {
  SAVE_CONT;
  // the first declartion will be necessary
  syntax_t *dec = declaration(last);
  if (dec != NULL) {
    // end
    if (istyp(EOT)) {
      // declaration
      return new_symbol("declaration_list", dec->symbol.lineno, 1, dec);
    }
    // not end
    // declaration_list -> declaration declaration_list
    syntax_t *dl = declaration_list(last);
    if (dl == NULL) {
      NONLAST_FAIL;
    }
    // declaration_list -> declaration declaration_list
    return new_symbol("declaration_list", dec->symbol.lineno, 2, dec, dl);
  } else {
    // error
    NONLAST_FAIL;
  }
}

syntax_t* statement_list(bool last) {
  SAVE_CONT;
  // whether emptiable
  syntax_t *stmt = statement(istyp(RC) ? false : last);
  if (stmt != NULL) {
    // not necessary
    syntax_t *stmtl = statement_list(false);
    if (stmtl == NULL || stmtl->symbol.size == 0) {
      if (stmtl) free(stmtl);
      return new_symbol("statement_list", stmt->symbol.lineno, 1, stmt);
    } else {
      return new_symbol("statement_list", stmt->symbol.lineno, 2, stmt, stmtl);
    }
  } else {
    // empty
    if (istyp(RC)) {
      return new_symbol("statement_list", line_number, 0);
    }
    else {
      NONLAST_FAIL;
    }
  }
}

syntax_t* fun_declaration(bool last) {
  SAVE_CONT;
  syntax_t *type, *id, *lp, *par, *rp, *cstmt;
  if (!istyp(TYPE)) {
    TOKEN_UNMATCH(TYPE);
  }
  type = advance();
  if (!istyp(ID)) {
    TOKEN_UNMATCH(ID);
  }
  id = advance();
  if (!istyp(LP)) {
    TOKEN_UNMATCH(LP);
  }
  lp = advance();
  par = params(last);
  if (par == NULL) {
    NONLAST_FAIL;
  }
  if (!istyp(RP)) {
    TOKEN_UNMATCH(RP);
  }
  rp = advance();
  cstmt = compound_stmt(last);
  if (cstmt == NULL) {
    NONLAST_FAIL;
  }
  return new_symbol("fun_declaration", type->token.lineno, 6, type, id, lp, par, rp, cstmt);
}

syntax_t* var_declaration(bool last) {
  // param -> type ID ; | type ID [ NUM ] ;
  SAVE_CONT;
  syntax_t *type, *id, *lb, *rb, *num, *semi;
  if (!istyp(TYPE)) {
    TOKEN_UNMATCH(TYPE);
  }
  type = advance();
  if (!istyp(ID)) {
    TOKEN_UNMATCH(ID);
  }
  id = advance();
  if ((istyp(LB) && isnxttyp(INT)) && istoktyp(2, RB) && istoktyp(3, SEMI)) {
    // param -> type ID [ NUM ] ;
    lb = advance();
    num = advance();
    rb = advance();
    semi = advance();
    return new_symbol("param", type->token.lineno, 6, type, id, lb, num, rb, semi);
  } else if (istyp(SEMI)){
    // param -> type ID ;
    semi = advance();
    return new_symbol("param", type->token.lineno, 3, type, id, semi);
  } else {
    MALFORM;
  }
}

syntax_t* declaration(bool last) {
  SAVE_CONT;
  
  if (!istyp(TYPE)) {
    TOKEN_UNMATCH(TYPE);
  }
  if (!isnxttyp(ID)) {
    TOKEN_UNMATCH(ID);
  }
  
  if (istoktyp(2, LP)) {
    // declaration -> fun_declaration
    syntax_t *fd = fun_declaration(last);
    if (fd == NULL) {
      NONLAST_FAIL;
    }
    return new_symbol("declaration", fd->symbol.lineno, 1, fd);
  } else if (istoktyp(2, LB) || istoktyp(2, SEMI)) {
    // declaration -> var_declaration
    syntax_t *vd = var_declaration(last);
    if (vd == NULL) {
      NONLAST_FAIL;
    }
    return new_symbol("declaration", vd->symbol.lineno, 1, vd);
  } else {
    MALFORM;
  }
}