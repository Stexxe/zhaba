#ifndef ZHABA_LEXER_H
#define ZHABA_LEXER_H

#include <stdbool.h>
#include "common.h"

typedef enum {
    UNKNOWN_TOKEN,
    INCLUDE_TOKEN, HEADER_NAME_TOKEN, INCLUDE_PATH_TOKEN,
    DEFINE_TOKEN,
    PREP_DIRECTIVE_TOKEN,
    WHITESPACE_TOKEN,
    S_CHAR_SEQ_TOKEN,
    KEYWORD_TOKEN,
    COMMA_TOKEN,
    COLON_TOKEN,
    SEMICOLON_TOKEN,
    NUM_LITERAL_TOKEN,
    DOT_TOKEN, ELLIPSIS_TOKEN,
    OPEN_PAREN_TOKEN, CLOSE_PAREN_TOKEN,
    OPEN_CURLY_TOKEN, CLOSE_CURLY_TOKEN,
    OPEN_BRACKET_TOKEN, CLOSE_BRACKET_TOKEN,
    NOT_TOKEN, EQUAL_TOKEN,

    // unary operations
    AMPERSAND_TOKEN, STAR_TOKEN,
    // binary operations
    NOT_EQUAL_TOKEN, DOUBLE_EQUAL_TOKEN,
    GREATER_TOKEN, GREATER_OR_EQUAL_TOKEN,
    LESSER_TOKEN, LESSER_OR_EQUAL_TOKEN,

    IDENTIFIER_TOKEN,
    STUB_TOKEN,
    EOF_TOKEN,
    COUNT_TOKEN,
} TokenType;

typedef enum {
    UNEXPECTED_TOKEN
} LexerErrorType;

typedef struct {
    LexerErrorType type;
    char token;
    int column;
    int line;
} LexerError;

struct Token {
    TokenType type;
    Span span;
    int line, column;
    struct Token *next;
};

typedef struct Token Token;

typedef struct {
    Span srcspan;
    byte *pos;
    bool eof;
} LexerState;

char read(LexerState *st);
LexerState *lexer_new(byte *src, size_t srcsize);
void lexer_free(LexerState *st);
Token *tokenize(byte *buf, size_t bufsize, int *nlines, LexerError *err);

#endif //ZHABA_LEXER_H