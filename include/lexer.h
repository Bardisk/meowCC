#ifndef MEOW_LEXER
#define MEOW_LEXER

#include <basics.h>

typedef struct token_t {
    char name[64];				// 词法单元名，即词法单元的类型
    int lineno;						// 行号，这是为了输出而保存的
    char value[64];	      // 词素
}token_t;

token_t* new_token(const char *name, int lineno, const char* value);

token_t* getToken(FILE *fp, int *line);

#endif
