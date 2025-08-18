#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "utf8.c"
#include "common.h"

typedef enum {
    PREP_INCLUDE
} TokenType;

typedef struct {
    TokenType type;
    union {

    };
} Token;

typedef struct {
    Token *data;
    size_t size;
} TokenMem;

typedef struct {
    rune *ptr;
    size_t size;
} String;

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
    unsigned char *src;
    size_t srcsize;
    size_t pos;
    TokenMem *outmem;
    boolean eof;
} LexerState;

// rune read(LexerState *st) {
//     int rsz;
//     rune r = read_rune(st->src + st->pos, &rsz);
//
//     if ((st->pos += rsz) >= st->srcsize) {
//         st->eof = true;
//     }
//
//     return r;
// }

char read(LexerState *st) {
    char c = *(st->src + st->pos);

    if (++st->pos >= st->srcsize) {
        st->eof = true;
    }

    return c;
}

LexerState *lexer_new(unsigned char *src, size_t srcsize, TokenMem *outmem) {
    LexerState *st = (LexerState *) malloc(sizeof(LexerState));

    if (st == NULL) {
        return NULL;
    }

    st->src = src;
    st->srcsize = srcsize;
    st->outmem = outmem;
    st->eof = false;
    st->pos = 0;

    return st;
}

static char *prep_directives[] = {
    "define", "elif", "else", "endif", "error", "if", "ifdef", "ifndef", "include", "line", "pragma", "undef"
};

#define PREP_DIRECTIVE_SIZE (sizeof(prep_directives) / sizeof(prep_directives[0]))

void lexer_free(LexerState *st) {
    free(st);
}


#define WH_BUF_SIZE 512
static char tempbuf[WH_BUF_SIZE];

int tokenize(unsigned char *buf, size_t bufsize, TokenMem *outmem) {
    char c;
    size_t i;
    int ti;

    LexerState *lex = lexer_new(buf, bufsize, outmem);

    while (!lex->eof) {
        c = read(lex);

        if (c == '#') {
            i = 0;
            while (!lex->eof && !isspace(c = read(lex))) { // TODO: Unread the space and save
                tempbuf[i++] = c;
            }

            tempbuf[i] = '\0';

            ti = binsearch(tempbuf, prep_directives, PREP_DIRECTIVE_SIZE);

            if (ti >= 0) {
                outmem->data[0] = (Token) {.type = PREP_INCLUDE}; // TODO: Write token func
            }

            // outmem->data[0] = PREP_INCLUDE
        }
    }

    // while (sz < bufsize) {
    //     r = read_rune(buf + sz, &rsz);
    //
    //     if (r == '#') {
    //         sz += rsz;
    //         for (r = read_rune(buf + sz, &rsz); sz < bufsize && !isspace(r); r = read_rune(buf + sz, &rsz), sz += rsz) {
    //
    //         }
    //
    //         while () {
    //             r = read_rune(buf + sz, &rsz);
    //         }
    //     }
    //
    //     // if (isspace(r) && whi < WH_BUF_SIZE) {
    //     //     whbuf[whi++] = r;
    //     // }
    //     //
    //     // if (!isspace(r)) {
    //     //
    //     // }
    //
    //     printf("%c", r);
    //
    //     sz += rsz;
    // }

    lexer_free(lex);
}

