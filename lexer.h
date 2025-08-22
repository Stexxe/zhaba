#ifndef ZHABA_LEXER_H
#define ZHABA_LEXER_H

#include <stdbool.h>
#include "common.h"

typedef enum {
    UNKNOWN_TOKEN,
    INCLUDE_TOKEN, HEADER_NAME_TOKEN,
    WHITESPACE_TOKEN,
    S_CHAR_SEQ_TOKEN,
    KEYWORD_TOKEN,
    COMMA_TOKEN,
    SEMICOLON_TOKEN,
    NUM_LITERAL_TOKEN,
    OPEN_PAREN_TOKEN, CLOSE_PAREN_TOKEN,
    OPEN_CURLY_TOKEN, CLOSE_CURLY_TOKEN,
    IDENTIFIER_TOKEN,
    EOF_TOKEN,
    COUNT_TOKEN,
} TokenType;

struct Token {
    TokenType type;
    Span span;
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
Token *tokenize(byte *buf, size_t bufsize);

#endif //ZHABA_LEXER_H