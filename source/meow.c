#include <basics.h>
#include <lexer.h>
#include <syntax.h>
#include <getopt.h>

FILE *source_fp;
int indent = 0;
bool lexer_only = false, exp_only = false;
bool debug_lexicon = false;
token_t **token_list;
int token_cnt;
extern int current_token_cnt;

int main(int argc, char *argv[]){
  int opt;
  while ((opt = getopt(argc, argv, "dhlei:")) != -1) {
    switch (opt)
    {
      case 'h': {
        printf("Usage: %s [OPTIONS] SOURCE\nOptions: hlei:" , argv[0]);
        break;
      }
      case 'l': {
        lexer_only = true;
        break;
      }
      case 'e': {
        exp_only = true;
        break;
      }
      case 'd': {
        debug_lexicon = true;
        break;
      }
      case 'i': {
        indent = atoi(optarg);
        break;
      }
      default: {
        fprintf(stderr, "Usage: %s [OPTIONS] SOURCE\nOptions: hlei:" , argv[0]);
        exit(-1);
      }
    }
  }
  if (optind >= argc) {
    fprintf(stderr, "missing source file\n");
    exit(-1);
  }
  source_fp = fopen(argv[optind], "r");
  if (source_fp == NULL) {
    fprintf(stderr, "open source file failed\n");
    exit(-1);
  }
  
  int line_number = 1, max_token_cnt = 1048576;
  token_list = (token_t **) malloc(max_token_cnt * sizeof(token_t *));

  token_t *now_token;
  while ((now_token = getToken(source_fp, &line_number)) != NULL) {
    if (strcmp(now_token->name, "EXCEPTION") == 0) {
      fprintf(stderr, "lexical error at line %d, type %s\n", now_token->lineno, now_token->value);
      exit(-1);
    }
    if (token_cnt >= max_token_cnt) {
      fprintf(stderr, "LEXER_PANIC: too many tokens");
      exit(-1);
    }
    token_list[token_cnt++] = now_token;
    // printf("Token {name: %s, line: %d, value: %s}\n", now_token->name, now_token->lineno, now_token->value);
  }
  
  if (lexer_only || debug_lexicon) {
    for (int i = 0; i < token_cnt; i++) {
      token_t *now_token = token_list[i];
      printf("Token {name: %s, line: %d, value: %s}\n", now_token->name, now_token->lineno, now_token->value);
    }
  }

  if (!lexer_only) {
    token_list[token_cnt] = (token_t *) malloc(sizeof(token_t));
    *token_list[token_cnt] = (token_t) {
      .lineno = line_number,
      .name = "EOT",
      .value = "EOT"
    };
    if (exp_only) {
      syntax_t *expr = expression(true);
      if (token_cnt != current_token_cnt) {
        fprintf(stderr, "SYNTATIC PANIC: EXTRA TOKENS\n");
        exit(-1);
      }
      print_syntax_tree(expr, indent);
    } else {
      syntax_t *prog = program(true);
      if (token_cnt != current_token_cnt) {
        fprintf(stderr, "SYNTATIC PANIC: EXTRA TOKENS\n");
        exit(-1);
      }
      print_syntax_tree(prog, indent);
    }
  }
  return 0;
}