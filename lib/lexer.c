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

typedef struct {
    char *name;
    void (*tokenize)(LexerState *, Span kw);
} Tokenizer;

static void insert_token(TokenType, Span);
static Span read_until(LexerState *st, int (*cmp) (int));
static Span read_spaces(LexerState *st);
static Span read_until_after_inc(LexerState *st, char c);
static Span read_until_char_inc(LexerState *st, char c);
static int isid(int c);
static int notid(int c);
static int notdigit(int c);
static int binsearch_lex_span(Span target, Tokenizer *arr, size_t size);

static void tokenize_nothing(LexerState *lex, Span kw) {}

static void tokenize_include(LexerState *lex, Span kw) {
    insert_token(INCLUDE_TOKEN, (Span) {kw.ptr-1, kw.end});
    insert_token(WHITESPACE_TOKEN, read_spaces(lex));

    char c = read(lex);

    if (c == '<') {
        lex->pos--;
        insert_token(HEADER_NAME_TOKEN, read_until_char_inc(lex, '>'));
    } else if (c == '"') {
        lex->pos--;
        insert_token(INCLUDE_PATH_TOKEN, read_until_after_inc(lex, '"'));
    }
}

static void tokenize_define(LexerState *lex, Span kw) {
    insert_token(DEFINE_TOKEN, (Span) {kw.ptr-1, kw.end});
    insert_token(WHITESPACE_TOKEN, read_spaces(lex));
    insert_token(IDENTIFIER_TOKEN, read_until(lex, isspace));
}

static Tokenizer prep_directives[] = {
    (Tokenizer) {"define", tokenize_define},
    (Tokenizer) {"elif", tokenize_nothing},
    (Tokenizer) {"else", tokenize_nothing},
    (Tokenizer) {"endif", tokenize_nothing},
    (Tokenizer) {"error", tokenize_nothing},
    (Tokenizer) {"if", tokenize_nothing},
    (Tokenizer) {"ifdef", tokenize_nothing},
    (Tokenizer) {"ifndef", tokenize_nothing},
    (Tokenizer) {"include", tokenize_include},
    (Tokenizer) {"line", tokenize_nothing},
    (Tokenizer) {"pragma", tokenize_nothing},
    (Tokenizer) {"undef", tokenize_nothing},
};

#define PREP_DIRECTIVE_SIZE (sizeof(prep_directives) / sizeof(prep_directives[0]))

static char *lang_keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
    "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long",
    "register", "restrict", "return", "short", "signed", "sizeof", "static", "struct",
    "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary"
};

#define LANG_KEYWORD_SIZE (sizeof(lang_keywords) / sizeof(lang_keywords[0]))

static Token *first_token = NULL, *token = NULL;
static int current_line, current_column;

typedef struct {
    char *token_str;
    TokenType token_type;
} SimpleTokenDef;

static SimpleTokenDef simple_token_defs[] = {
    (SimpleTokenDef){"(", OPEN_PAREN_TOKEN},
    (SimpleTokenDef){")", CLOSE_PAREN_TOKEN},
    (SimpleTokenDef){",", COMMA_TOKEN},
    (SimpleTokenDef){"&", AMPERSAND_TOKEN},
    (SimpleTokenDef){"(", OPEN_PAREN_TOKEN},
    (SimpleTokenDef){")", CLOSE_PAREN_TOKEN},
    (SimpleTokenDef){"{", OPEN_CURLY_TOKEN},
    (SimpleTokenDef){"}", CLOSE_CURLY_TOKEN},
    (SimpleTokenDef){"[", OPEN_BRACKET_TOKEN},
    (SimpleTokenDef){"]", CLOSE_BRACKET_TOKEN},
    (SimpleTokenDef){":", COLON_TOKEN},
    (SimpleTokenDef){";", SEMICOLON_TOKEN},
    (SimpleTokenDef){"-", MINUS_TOKEN},
    (SimpleTokenDef){"-=", MINUS_EQUAL_TOKEN},
    (SimpleTokenDef){"->", ARROW_TOKEN},
    (SimpleTokenDef){"--", DECREMENT_TOKEN},
    (SimpleTokenDef){"/", DIVISION_TOKEN},
    (SimpleTokenDef){"//", LINE_COMMENT_TOKEN},
    (SimpleTokenDef){"/*", OPEN_MULTI_COMMENT_TOKEN},
    (SimpleTokenDef){"!", NOT_TOKEN},
    (SimpleTokenDef){"!=", NOT_EQUAL_TOKEN},
    (SimpleTokenDef){"=", EQUAL_TOKEN},
    (SimpleTokenDef){"==", DOUBLE_EQUAL_TOKEN},
    (SimpleTokenDef){"<", LESSER_TOKEN},
    (SimpleTokenDef){"<=", LESSER_OR_EQUAL_TOKEN},
    (SimpleTokenDef){">", GREATER_TOKEN},
    (SimpleTokenDef){">=", GREATER_OR_EQUAL_TOKEN},
    (SimpleTokenDef){"*", STAR_TOKEN},
    (SimpleTokenDef){"*/", CLOSE_MULTI_COMMENT_TOKEN},
    (SimpleTokenDef){".", DOT_TOKEN},
    (SimpleTokenDef){"...", ELLIPSIS_TOKEN},
};

#define SIMPLE_TOKEN_DEFS_SIZE (sizeof(simple_token_defs) / sizeof(simple_token_defs[0]))

static int binsearch_tokendef(Span target, SimpleTokenDef *arr, size_t size);

int token_def_cmp(const void *p1, const void *p2) {
    return strcmp(((SimpleTokenDef *) p1)->token_str, ((SimpleTokenDef *) p2)->token_str);
}

int prep_directive_cmp(const void *p1, const void *p2) {
    return strcmp(((Tokenizer *) p1)->name, ((Tokenizer *) p2)->name);
}

void lexer_init() {
    qsort(lang_keywords, LANG_KEYWORD_SIZE, sizeof(lang_keywords[0]), qsort_strcmp);
    qsort(simple_token_defs, SIMPLE_TOKEN_DEFS_SIZE, sizeof(simple_token_defs[0]), token_def_cmp);
    qsort(prep_directives, PREP_DIRECTIVE_SIZE, sizeof(prep_directives[0]), token_def_cmp);
}

Token *tokenize(byte *buf, size_t bufsize, int *nlines, LexerError *err) {
    int i;
    LexerState *lex = lexer_new(buf, bufsize);
    current_column = current_line = 1;
    first_token = token = NULL;

    while (!lex->eof) {
        if (lex->eof) break;

        char c = read(lex);

        if (c == '#') {
            Span kw = read_until(lex, isspace);
            i = binsearch_lex_span(kw, prep_directives, PREP_DIRECTIVE_SIZE);

            if (i >= 0) {
                prep_directives[i].tokenize(lex, kw);
            }
        } else if (c == '"') {
            lex->pos--;
            insert_token(S_CHAR_SEQ_TOKEN, read_until_after_inc(lex, '"'));
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
            lex->pos--;
            int simple_ind = -1;
            Span simple_span = {lex->pos, lex->pos + 1};

            while (simple_span.end <= lex->srcspan.end && (i = binsearch_tokendef(simple_span, simple_token_defs, SIMPLE_TOKEN_DEFS_SIZE)) >= 0) {
                simple_span.end++;
                simple_ind = i;
            }

            simple_span.end--;
            lex->pos = simple_span.end;

            if (simple_ind >= 0) {
                insert_token(simple_token_defs[simple_ind].token_type, simple_span);
            } else {
                err->token = c;
                err->column = current_column;
                err->line = current_line;
                return NULL;
            }
        }
    }

    for (i = 0; i < 4; i++) {
        insert_token(STUB_TOKEN, (Span){});
    }

    lexer_free(lex);
    *nlines = current_line;

    return first_token;
}

static int binsearch_lex_span(Span target, Tokenizer *arr, size_t size) {
    int low = 0;
    int high = (int) size - 1;
    int comp;

    while (low <= high) {
        int mid = (low + high) / 2;

        if ((comp = spanstrcmp(target, arr[mid].name)) < 0) {
            high = mid - 1;
        } else if (comp > 0) {
            low = mid + 1;
        } else {
            return mid;
        }
    }

    return -1;
}

static int binsearch_tokendef(Span target, SimpleTokenDef *arr, size_t size) {
    int low = 0;
    int high = (int) size - 1;
    int comp;

    while (low <= high) {
        int mid = (low + high) / 2;

        if ((comp = spanstrcmp(target, arr[mid].token_str)) < 0) {
            high = mid - 1;
        } else if (comp > 0) {
            low = mid + 1;
        } else {
            return mid;
        }
    }

    return -1;
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
