#ifndef ZHABA_LEXER_H
#define ZHABA_LEXER_H

#include "common.h"

typedef enum {
    UNKNOWN_TOKEN,
    INCLUDE_TOKEN, HEADER_NAME_TOKEN,
    WHITESPACE_TOKEN,
    S_CHAR_SEQ_TOKEN,
    KEYWORD_TOKEN,
    SEMICOLON_TOKEN,
    NUM_LITERAL_TOKEN,
    OPEN_PAREN_TOKEN, CLOSE_PAREN_TOKEN,
    OPEN_CURLY_TOKEN, CLOSE_CURLY_TOKEN,
    IDENTIFIER_TOKEN,
    EOF_TOKEN,
    COUNT_TOKEN,
} TokenType;

typedef struct {
    TokenType type;
    Span span;
} Token;

typedef struct {
    Span srcspan;
    byte *pos;
    boolean eof;
} LexerState;

char read(LexerState *st);
LexerState *lexer_new(byte *src, size_t srcsize);
void lexer_free(LexerState *st);



Span read_until(LexerState *st, int (*cmp) (int));
Span read_spaces(LexerState *st);
Span read_until_after_inc(LexerState *st, char c);
Span read_until_char_inc(LexerState *st, char c);
int isid(int c);
int notid(int c);
int notdigit(int c);
Token *tokenize(byte *buf, size_t bufsize);

#endif //ZHABA_LEXER_H