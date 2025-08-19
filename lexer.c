#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"

typedef enum {
    UNKNOWN_TOKEN,
    PREP_INCLUDE, HEADER_NAME,
    WHITESPACE,
    S_CHAR_SEQ,
    KEYWORD,
    SEMICOLON,
    NUM_LITERAL,
    OPEN_PAREN, CLOSE_PAREN,
    OPEN_CURLY, CLOSE_CURLY,
    IDENTIFIER,
    COUNT_TOKEN,
} TokenType;

typedef struct {
    TokenType type;
    Span span;

    union {

    };
} Token;

typedef struct {
    Token *data;
    size_t size;
} TokenMem;

#define TOKEN_MEM_MAX 64 * 1024
TokenMem *token_mem_new() {
    TokenMem *mem = (TokenMem *) malloc(sizeof(TokenMem));
    if (mem == NULL) {
        return NULL;
    }

    size_t size = sizeof(Token) * TOKEN_MEM_MAX;
    mem->data = (Token *) malloc(sizeof(Token) * TOKEN_MEM_MAX);

    if (mem->data == NULL) {
        free(mem);
        return NULL;
    }

    mem->size = size;
    return mem;
}

typedef struct {
    Span srcspan;
    byte *pos;
    boolean eof;
} LexerState;

char read(LexerState *st) {
    if (st->pos >= st->srcspan.end) {
        st->eof = true;
        return 0;
    }

    return *st->pos++;
}

LexerState *lexer_new(byte *src, size_t srcsize) {
    LexerState *st = (LexerState *) malloc(sizeof(LexerState));

    if (st == NULL) {
        return NULL;
    }

    st->srcspan.ptr = src;
    st->srcspan.end = src + srcsize;
    st->pos = src;
    st->eof = false;

    return st;
}

static char *prep_directives[] = {
    "define", "elif", "else", "endif", "error", "if", "ifdef", "ifndef", "include", "line", "pragma", "undef"
};

#define PREP_DIRECTIVE_SIZE (sizeof(prep_directives) / sizeof(prep_directives[0]))

static char *lang_keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long",
    "register", "restrict", "return", "short", "signed", "sizeof", "static", "struct",
    "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary"
};

#define LANG_KEYWORD_SIZE (sizeof(lang_keywords) / sizeof(lang_keywords[0]))


void lexer_free(LexerState *st) {
    free(st);
}

static Token tokenout[1024];

Span read_until(LexerState *st, int (*cmp) (int));
Span read_spaces(LexerState *st);
Span read_until_after_inc(LexerState *st, char c);
Span read_until_char_inc(LexerState *st, char c);
int isid(int c);
int notid(int c);
int notdigit(int c);

int tokenize(byte *buf, size_t bufsize) {
    int i;
    Token *tokenp = tokenout;

    LexerState *lex = lexer_new(buf, bufsize);

    while (!lex->eof) {
        char c = read(lex);

        if (c == '#') {
            Span kw = read_until(lex, isspace);
            i = binsearch(kw, prep_directives, PREP_DIRECTIVE_SIZE);

            if (i >= 0) {
                *tokenp++ = (Token) {.type = PREP_INCLUDE, .span = kw}; // TODO: Match directives with type
            }
        } if (c == '<') {
            lex->pos--;
            *tokenp++ = (Token) {.type = HEADER_NAME, .span = read_until_char_inc(lex, '>')};
        } else if (c == '"') {
            lex->pos--;
            *tokenp++ = (Token) {.type = S_CHAR_SEQ, .span = read_until_after_inc(lex, '"')};
        } else if (c == '(') {
            *tokenp++ = (Token) {.type = OPEN_PAREN, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (c == ')') {
            *tokenp++ = (Token) {.type = CLOSE_PAREN, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (c == '{') {
            *tokenp++ = (Token) {.type = OPEN_CURLY, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (c == ';') {
            *tokenp++ = (Token) {.type = SEMICOLON, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (c == '}') {
            *tokenp++ = (Token) {.type = CLOSE_CURLY, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (isspace(c)) {
            lex->pos--;
            *tokenp++ = (Token) {.type = WHITESPACE, .span = read_spaces(lex)};
        } if (isdigit(c)) {
            lex->pos--;
            *tokenp++ = (Token) {.type = NUM_LITERAL, .span = read_until(lex, notdigit)};
        } else if (isalpha(c) || c == '_') {
            lex->pos--;

            Span word = read_until(lex, notid);
            if (binsearch(word, lang_keywords, LANG_KEYWORD_SIZE) >= 0) {
                *tokenp++ = (Token) {.type = KEYWORD, .span = word};
            } else {
                *tokenp++ = (Token) {.type = IDENTIFIER, .span = word};
            }
        }
    }

    lexer_free(lex);
}


Span read_until_char_inc(LexerState *st, char c) {
    Span span = {st->pos};
    byte *current = st->pos;
    while (read(st) != c && !st->eof) {
        current++;
    }

    if (!st->eof) {
        current = st->pos;
    }

    span.end = current;
    return span;
}

Span read_spaces(LexerState *st) {
    Span span = {st->pos};
    byte *current = st->pos;

    while (isspace(read(st)) && !st->eof) {
        current++;
    }

    if (!st->eof) {
        st->pos--;
    }

    span.end = current;
    return span;
}

Span read_until_after_inc(LexerState *st, char c) {
    Span span = {st->pos++};
    byte *current = st->pos;

    while (read(st) != c && !st->eof) {
        current++;
    }

    if (!st->eof) {
        current = st->pos;
    }

    span.end = current;
    return span;
}

Span read_until(LexerState *st, int (*cmp) (int)) {
    Span span = {st->pos};
    byte *current = st->pos;
    while (!(cmp)(read(st)) && !st->eof) {
        current++;
    }

    if (!st->eof) {
        st->pos--;
    }

    span.end = current;
    return span;
}

int notid(int c) {
    return !isid(c);
}

int isid(int c) {
    return isalnum(c) || c == '_';
}

int notdigit(int c) {
    return !isdigit(c);
}
