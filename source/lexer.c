#include <lexer.h>

typedef enum {
    START,
    SLASH,
    RELOP,
    SHARP,
    ASSIGN,
    NUM,
    WORD,
    INCOMMENT,
    OUT,
    PANIC
} State;

token_t* new_token(const char *name, int lineno, const char *value) {
  token_t* ret = malloc(sizeof(token_t));

  strcpy(ret->name, name);
  strcpy(ret->value, value);
  ret->lineno = lineno;

//   printf("Token: {value: %s, line: %d, type: TOKEN_%s}\n", value, lineno, name);

//   puts("");
  return ret;
}

// @param fp: the input stream
// @param line: the line number
// @returns the next token
token_t* getToken(FILE *fp, int *line) {
    char c;
    State state = START;
    char value[64];
    int i = 0;

    while ((c = fgetc(fp)) != EOF) {
        switch (state) {
            case START:
                if (isspace(c)) {
                    // 处理空白字符，增加行号计数
                    if (c == '\n') (*line)++;
                } else if (c == '/') {
                    // 处理可能的注释开始
                    state = SLASH;
                } else if (c == '<' || c == '>') {
                    // 处理关系运算符
                    state = RELOP;
                    value[i++] = c;
                } else if (c == '=') {
                    // 处理赋值操作符
                    state = ASSIGN;
                    value[i++] = c;
                } else if (c == '!') {
                    // 处理叹号
                    state = SHARP;
                    value[i++] = c;
                } else if (isdigit(c)) {
                    // 处理数字
                    state = NUM;
                    value[i++] = c;
                } else if (isalpha(c)) {
                    // 处理标识符或关键字
                    state = WORD;
                    value[i++] = c;
                } else if (c == ';') {
                    // 处理分号
                    return new_token("SEMI", *line, ";");
                } else if (c == ',') {
                    // 处理逗号
                    return new_token("COMMA", *line, ",");
                } else if (c == '(') {
                    // 处理左圆括号
                    return new_token("LP", *line, "(");
                } else if (c == ')') {
                    // 处理右圆括号
                    return new_token("RP", *line, ")");
                } else if (c == '[') {
                    // 处理左方括号
                    return new_token("LB", *line, "[");
                } else if (c == ']') {
                    // 处理右方括号
                    return new_token("RB", *line, "]");
                } else if (c == '{') {
                    // 处理左大括号
                    return new_token("LC", *line, "{");
                } else if (c == '}') {
                    // 处理右大括号
                    return new_token("RC", *line, "}");
                } else if (c == '+') {
                    // 处理加号
                    return new_token("PLUS", *line, "+");
                } else if (c == '-') {
                    // 处理减号
                    return new_token("MINUS", *line, "-");
                } else if (c == '*') {
                    // 处理星号
                    return new_token("STAR", *line, "*");
                } else {
                    // 进入PANIC状态
                    return new_token("EXCEPTION", *line, "BAD_CHAR");
                }
                break;
            case SHARP:
                if (c == '=') {
                    return new_token("NEQ", *line, "!=");
                } else {
                    return new_token("EXCEPTION", *line, "BAD_SHARP");
                }
                break;
            case SLASH:
                if (c == '*') {
                    // 处理块注释的开始
                    state = INCOMMENT;
                } else {
                    // 处理除号
                    ungetc(c, fp);
                    return new_token("DIV", *line, "/");
                }
                break;
            case RELOP:
                if (c == '=') {
                    // 处理关系运算符的组合，如 <= 或 >=
                    if (value[0] == '<')
                      return new_token("LEQ", *line, "<=");
                    else
                      return new_token("GEQ", *line, ">=");
                } else {
                    // 处理单个关系运算符，如 < 或 >
                    ungetc(c, fp);
                    if (value[0] == '<')
                      return new_token("LESS", *line, "<");
                    else
                      return new_token("GREAT", *line, ">");
                }
                break;
            case ASSIGN:
                if (c == '=') {
                    return new_token("EQUAL", *line, "==");
                } else {
                    // 处理赋值运算符 =
                    ungetc(c, fp);
                    return new_token("ASSIGN", *line, "=");
                }
                break;
            case NUM:
                if (isdigit(c)) {
                    // 继续读取数字
                    value[i++] = c;
                } else if (isalpha(c)) {
                    // 数字后面有字母，进入PANIC状态
                    value[i] = '\0';
                    return new_token("EXCEPTION", *line, "INVALID_TOKEN");
                } else {
                    // 结束读取数字，返回INT token
                    value[i] = '\0';
                    ungetc(c, fp);
                    return new_token("INT", *line, value);
                }
                break;
            case WORD:
                if (isalpha(c)) {
                    // 继续读取标识符或关键字
                    value[i++] = c;
                } else if (isdigit(c)) {
                    // 标识符或关键字后面有数字，进入PANIC状态
                    return new_token("EXCEPTION", *line, "INVALID_TOKEN");
                } else {
                    // 结束读取标识符或关键字，判断是否为关键字
                    value[i] = '\0';
                    ungetc(c, fp);
                    if (strcmp(value, "int") == 0 || strcmp(value, "void") == 0) {
                        return new_token("TYPE", *line, value);
                    } else if (strcmp(value, "return") == 0) {
                        return new_token("RETURN", *line, "return");
                    } else if (strcmp(value, "if") == 0) {
                        return new_token("IF", *line, "if");
                    } else if (strcmp(value, "else") == 0) {
                        return new_token("ELSE", *line, "else");
                    } else if (strcmp(value, "while") == 0) {
                        return new_token("WHILE", *line, "while");
                    } else {
                        return new_token("ID", *line, value);
                    }
                }
                break;
            case INCOMMENT:
                if (c == '*') {
                    // 检测注释结束标记
                    state = OUT;
                } else if (c == '\n') {
                    // 处理换行符，增加行号计数
                    (*line)++;
                }
                break;
            case OUT:
                if (c == '/') {
                    // 注释结束，回到START状态
                    state = START;
                } else if (c == '*') {
                    // 仍然在注释中，保持在OUT状态
                } else {
                    state = INCOMMENT;
                }
                break;
            default:
                fprintf(stderr, "LEXER_PANIC: INVALID STATE");
                exit(-1);
        }
    }

    switch (state) {
        case OUT:
        case INCOMMENT:
            return new_token("EXCEPTION", *line, "UNTERMINATED_COMMENT");
        case START:
            return NULL;
        default:
            return new_token("EXCEPTION", *line, "UNEXPECTED_EOF");
    }

    // 如果读取到文件末尾，返回 NULL
    return NULL;
}

