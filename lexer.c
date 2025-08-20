#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"
#include "lexer.h"

char read(LexerState *st) {
    if (st->pos >= st->srcspan.end) {
        st->eof = true;
        return 0;
    }

    return (char) *st->pos++;
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


void lexer_free(LexerState *st) {
    free(st);
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

static Token tokenout[1024];

Token *tokenize(byte *buf, size_t bufsize) {
    int i;
    Token *tokenp = tokenout;

    LexerState *lex = lexer_new(buf, bufsize);

    while (!lex->eof) {
        char c = read(lex);

        if (c == '#') {
            Span kw = read_until(lex, isspace);
            i = binsearch(kw, prep_directives, PREP_DIRECTIVE_SIZE);

            if (i >= 0) {
                *tokenp++ = (Token) {.type = INCLUDE_TOKEN, .span = kw}; // TODO: Match directives with type
            }
        } if (c == '<') {
            lex->pos--;
            *tokenp++ = (Token) {.type = HEADER_NAME_TOKEN, .span = read_until_char_inc(lex, '>')};
        } else if (c == '"') {
            lex->pos--;
            *tokenp++ = (Token) {.type = S_CHAR_SEQ_TOKEN, .span = read_until_after_inc(lex, '"')};
        } else if (c == '(') {
            *tokenp++ = (Token) {.type = OPEN_PAREN_TOKEN, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (c == ')') {
            *tokenp++ = (Token) {.type = CLOSE_PAREN_TOKEN, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (c == '{') {
            *tokenp++ = (Token) {.type = OPEN_CURLY_TOKEN, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (c == ';') {
            *tokenp++ = (Token) {.type = SEMICOLON_TOKEN, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (c == '}') {
            *tokenp++ = (Token) {.type = CLOSE_CURLY_TOKEN, .span = (Span){lex->pos - 1, lex->pos}};
        } else if (isspace(c)) {
            lex->pos--;
            *tokenp++ = (Token) {.type = WHITESPACE_TOKEN, .span = read_spaces(lex)};
        } if (isdigit(c)) {
            lex->pos--;
            *tokenp++ = (Token) {.type = NUM_LITERAL_TOKEN, .span = read_until(lex, notdigit)};
        } else if (isalpha(c) || c == '_') {
            lex->pos--;

            Span word = read_until(lex, notid);
            if (binsearch(word, lang_keywords, LANG_KEYWORD_SIZE) >= 0) {
                *tokenp++ = (Token) {.type = KEYWORD_TOKEN, .span = word};
            } else {
                *tokenp++ = (Token) {.type = IDENTIFIER_TOKEN, .span = word};
            }
        }
    }

    *tokenp = (Token) {.type = EOF_TOKEN};

    lexer_free(lex);
    return tokenout;
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
