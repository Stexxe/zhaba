#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"
#include "lexer.h"

void prev(LexerState *st) {
    if (st->pos > st->srcspan.ptr) {
        st->pos--;
    }
}

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

static Span read_until(LexerState *st, int (*cmp) (int));
static  Span read_spaces(LexerState *st);
static Span read_until_after_inc(LexerState *st, char c);
static Span read_until_char_inc(LexerState *st, char c);
static int isid(int c);
static int notid(int c);
static int notdigit(int c);
static void insert_token(TokenType, Span);

static Token *first_token = NULL, *token = NULL;
static int current_line, current_column;

Token *tokenize(byte *buf, size_t bufsize, int *nlines, LexerError *err) {
    int i;
    LexerState *lex = lexer_new(buf, bufsize);
    current_column = current_line = 1;
    first_token = token = NULL;

    while (!lex->eof) {
        char c = read(lex);

        if (lex->eof) break;

        if (c == '#') {
            Span kw = read_until(lex, isspace);
            i = binsearch_span(kw, prep_directives, PREP_DIRECTIVE_SIZE);

            if (i >= 0) {
                insert_token(INCLUDE_TOKEN, (Span) {kw.ptr-1, kw.end}); // TODO: Match directives with type
                // *tokenp++ = (Token) {.type = INCLUDE_TOKEN, .span = kw};
            }
        } else if (c == '<') {
            lex->pos--;
            insert_token(HEADER_NAME_TOKEN, read_until_char_inc(lex, '>'));
        } else if (c == '"') {
            lex->pos--;
            insert_token(S_CHAR_SEQ_TOKEN, read_until_after_inc(lex, '"'));
        } else if (c == '(') {
            insert_token(OPEN_PAREN_TOKEN, (Span){lex->pos - 1, lex->pos});
        } else if (c == ')') {
            insert_token(CLOSE_PAREN_TOKEN, (Span){lex->pos - 1, lex->pos});
        } else if (c == ',') {
            insert_token(COMMA_TOKEN, (Span){lex->pos - 1, lex->pos});
        } else if (c == '{') {
            insert_token(OPEN_CURLY_TOKEN, (Span){lex->pos - 1, lex->pos});
        } else if (c == ';') {
            insert_token(SEMICOLON_TOKEN, (Span){lex->pos - 1, lex->pos});
        } else if (c == '}') {
            insert_token(CLOSE_CURLY_TOKEN, (Span){lex->pos - 1, lex->pos});
        } else if (c == '=') {
            insert_token(EQUAL_SIGN_TOKEN, (Span){lex->pos - 1, lex->pos});
        }  else if (c == '>') {
            insert_token(GREATER_TOKEN, (Span){lex->pos - 1, lex->pos});
        } else if (isspace(c)) {
            lex->pos--;
            insert_token(WHITESPACE_TOKEN, read_spaces(lex));
        } else if (isdigit(c)) {
            lex->pos--;
            insert_token(NUM_LITERAL_TOKEN, read_until(lex, notdigit));
        } else if (isalpha(c) || c == '_') {
            lex->pos--;

            Span word = read_until(lex, notid);
            if (binsearch_span(word, lang_keywords, LANG_KEYWORD_SIZE) >= 0) {
                insert_token(KEYWORD_TOKEN, word);
            } else {
                insert_token(IDENTIFIER_TOKEN, word);
            }
        } else {
            err->token = c;
            err->column = current_column;
            err->line = current_line;
            return NULL;
        }
    }

    for (i = 0; i < 4; i++) {
        insert_token(STUB_TOKEN, (Span){});
    }

    lexer_free(lex);
    *nlines = current_line;

    return first_token;
}

static void insert_token(TokenType type, Span sp) {
    Token *prev = token;

    token = pool_alloc(sizeof(Token), Token);
    token->type = type;
    token->column = current_column;
    token->line = current_line;
    token->span = sp;
    token->next = NULL;

    for (byte *cp = sp.ptr; cp < sp.end; cp++) {
        current_column++;
        if (*cp == '\n') {
            current_column = 1;
            current_line++;
        }
    }

    if (first_token == NULL) first_token = token;
    else prev->next = token;
}

static Span read_until_char_inc(LexerState *st, char c) {
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

static Span read_spaces(LexerState *st) {
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

static Span read_until_after_inc(LexerState *st, char c) {
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

static Span read_until(LexerState *st, int (*cmp) (int)) {
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

static int notid(int c) {
    return !isid(c);
}

static int isid(int c) {
    return isalnum(c) || c == '_';
}

static int notdigit(int c) {
    return !isdigit(c);
}
