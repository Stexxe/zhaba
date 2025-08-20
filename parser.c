#include "parser.h"
#include <assert.h>

static char *primitive_types[] = {
    "double", "char", "float", "int", "long", "short"
};

#define PRIMITIVE_COUNT (sizeof(primitive_types) / sizeof(primitive_types[0]))

static Grammar astout[1024];

#define MAX_ELEMENTS 128

static DataType data_types[MAX_ELEMENTS];
static DataType *data_type_p = data_types;

static FuncSignature func_signatures[MAX_ELEMENTS];
static FuncSignature *func_signature_p = func_signatures;

static FuncDef func_defs[MAX_ELEMENTS];
static FuncDef *func_def_p = func_defs;

static Statement statements[MAX_ELEMENTS];
static Statement *statement_p = statements;

static FuncArgument arguments[MAX_ELEMENTS];
static FuncArgument *argument_p = arguments;

void parse(Token *tokens) {
    Token *tokenp = tokens;
    Grammar *astp = astout;
    Token *start_token, *end_token;

    while (tokenp->type != EOF_TOKEN) {
        switch (tokenp->type) {
            case INCLUDE_HEADER: {
                start_token = tokenp;
                if ((++tokenp)->type == WHITESPACE_TOKEN) {
                    tokenp++;
                }

                if (tokenp->type == HEADER_NAME_TOKEN) {
                    Token *header = tokenp;
                    *astp++ = (Grammar){.type = INCLUDE_HEADER, .start_token = start_token, .end_token = ++tokenp, .data = header};
                }
            } break;
            case WHITESPACE_TOKEN: {
                tokenp++;
            } break;
            case KEYWORD_TOKEN: {
                FuncSignature *sign = parse_func_signature(&tokenp);
                skip_white(&tokenp);

                if (tokenp->type == SEMICOLON_TOKEN) { // Func declaration

                } else if (tokenp->type == OPEN_CURLY_TOKEN) { // Func definition
                    Statement *first_statement = parse_func_body(&tokenp);
                    *func_def_p++ = (FuncDef){.signature = sign, .first_statement = first_statement};
                }


                // if (binsearch(tokenp->span, primitive_types, PRIMITIVE_COUNT) >= 0) {
                //     start_token = tokenp;
                //     DataType *return_type = parse_data_type(&tokenp);
                //     skip_white(&tokenp);
                //
                //     Token *name = tokenp;
                //     if (tokenp->type == IDENTIFIER_TOKEN) {
                //         tokenp++;
                //     }
                //
                //     skip_white(&tokenp);
                //
                //     if (tokenp->type == OPEN_PAREN_TOKEN) { // Function declaration
                //         skip_token(&tokenp, OPEN_PAREN_TOKEN);
                //         skip_white(&tokenp);
                //         skip_token(&tokenp, CLOSE_PAREN_TOKEN);
                //         skip_white(&tokenp);
                //
                //         if (tokenp->type == OPEN_CURLY_TOKEN) { // Function definition
                //             FuncDef def = {.name = name, .data_type = return_type};
                //             parse_func_body(&tokenp, astp);
                //             *astp = (Grammar){.type = FUNC_DEF, .start_token = start_token, .end_token = tokenp, .data = header}
                //         }
                //     }
                // }
            } break;
            default: {
                assert(false);
            } break;
        }
    }
}

void skip_white(Token **tokenp) {
    for (; (*tokenp)->type == WHITESPACE_TOKEN; *tokenp += 1)
        ;
}

void skip_token(Token **tokenp, TokenType token_type) {
    assert((*tokenp)->type == token_type);
    *tokenp += 1;
}

DataType *parse_data_type(Token **tokenp) {
    if (spanstrcmp((*tokenp)->span, "int") >= 0) { // TODO: Lookup
        *data_type_p = (DataType) {.primitive = INT_TYPE};
    }

    *tokenp += 1;
    return data_type_p++;
}

FuncSignature *parse_func_signature(Token **tokenp) {
    DataType *type = parse_data_type(tokenp);
    skip_white(tokenp);
    Token *name = *tokenp;
    *tokenp += 1;

    skip_white(tokenp);
    skip_token(tokenp, OPEN_PAREN_TOKEN);
    skip_white(tokenp);
    skip_token(tokenp, CLOSE_PAREN_TOKEN);
    skip_white(tokenp);

    *func_signature_p = (FuncSignature) {.name = name, .data_type = type};
    return func_signature_p++;
}

Statement *parse_func_body(Token **tokenp) {
    skip_token(tokenp, OPEN_CURLY_TOKEN);
    skip_white(tokenp);
    Statement *prev, *st, *first;

    first = prev = parse_statement(tokenp);
    skip_token(tokenp, SEMICOLON_TOKEN);
    skip_white(tokenp);

    while ((*tokenp)->type != CLOSE_CURLY_TOKEN) {
        st = parse_statement(tokenp);
        prev->next = st;
        skip_token(tokenp, SEMICOLON_TOKEN);
        skip_white(tokenp);
        prev = st;
    }

    return first;
}

Statement *parse_statement(Token **tokenp) {
    if ((*tokenp)->type == IDENTIFIER_TOKEN) {
        Expression *expr = parse_expression(tokenp);

        // *tokenp += 1;
        //
        // skip_white(tokenp);
        //
        // if ((*tokenp)->type == OPEN_PAREN_TOKEN) {
        //
        // }
    }
}

Expression *parse_expression(Token **tokenp) {
    if ((*tokenp)->type == IDENTIFIER_TOKEN) {
        *tokenp += 1;
        skip_white(tokenp);

        if ((*tokenp)->type == OPEN_PAREN_TOKEN) {
            // TODO: Parse func arguments
        }
    }
}
