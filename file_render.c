#include <assert.h>
#include <stdio.h>
#include "parser.h"

typedef enum {
    NO_COLOR = 0,
    STR_LIT_COLOR = 32,
    KEYWORD_COLOR = 33,
    NUM_LIT_COLOR = 35,
    DEFAULT_COLOR = 97,
    PREP_INST_COLOR = 93,
    FUNC_NAME_DEF_COLOR = 94,
} SyntaxColor;


static void colored(Span sp, SyntaxColor color, FILE *file) {
    fprintf(file, "\x1b[%dm", color);

    for (; sp.ptr < sp.end; sp.ptr++) {
        putc((char) *sp.ptr, file);
    }

    fprintf(file, "\x1b[0m");
}

static void colored_token_span(Token *tokenp, Token *end_token, SyntaxColor color, FILE *file) {
    for (; tokenp < end_token; tokenp++) {
        colored(tokenp->span, color, file);
    }
}

static void print_ws(Token *token, FILE *file) {
    for (; token != NULL && token->type == WHITESPACE_TOKEN; token = token->next) {
        colored(token->span, NO_COLOR, file);
    }
}

static void print_ws_after(Token *token, FILE *file) {
    for (token = token->next; token != NULL && token->type == WHITESPACE_TOKEN; token = token->next) {
        colored(token->span, NO_COLOR, file);
    }
}

static void render_statement_expr(NodeHeader *st, FILE *file) {
    switch (st->type) {
        case FUNC_INVOKE: {
            FuncInvoke *invoke = (FuncInvoke*) st;
            colored(invoke->name->span, DEFAULT_COLOR, file);
            colored_token_span(invoke->name->next, invoke->arg->start_token, NO_COLOR, file);

            NodeHeader *last_arg = NULL;
            for (NodeHeader *arg = invoke->arg; arg != NULL; arg = arg->next) {
                render_statement_expr(arg, file);
                if (arg->next != NULL) {
                    colored_token_span(arg->end_token, arg->next->start_token, NO_COLOR, file);
                }
                last_arg = arg;
            }

            if (last_arg != NULL) { // TODO: Handle no args
                colored_token_span(last_arg->end_token, invoke->header.end_token, NO_COLOR, file);
            }
        } break;
        case RETURN_STATEMENT: {
            ReturnStatement *ret = (ReturnStatement*) st;
            colored(ret->header.start_token->span, KEYWORD_COLOR, file);
            print_ws_after(ret->header.start_token, file);
            render_statement_expr(ret->expr, file);
        } break;
        case STRING_LITERAL: {
            StringLiteral *literal = (StringLiteral*) st;
            colored(literal->str->span, STR_LIT_COLOR, file);
        } break;
        case INT_LITERAL: {
            IntLiteral *literal = (IntLiteral*) st;
            colored(literal->num->span, NUM_LIT_COLOR, file);
        } break;
        default: {
            assert(0);
        } break;
    }

    if (st->next != NULL) {
        colored_token_span(st->end_token, st->next->start_token, NO_COLOR, file);
    }
}

void render_file(NodeHeader *node, FILE *file) {
    NodeHeader *last_node = node;

    while (node != NULL) {
        if (node->type == INCLUDE_HEADER) {
            IncludeHeaderName *inc = (IncludeHeaderName *) node;
            colored(inc->header.start_token->span, PREP_INST_COLOR, file);
            print_ws_after(inc->header.start_token, file);
            colored(inc->name->span, STR_LIT_COLOR, file);
            print_ws_after(inc->name, file);
        } else if (node->type == FUNC_DEF) {
            FuncDef *def = (FuncDef *) node;
            DataType *return_type = def->signature->return_type;
            colored_token_span(return_type->start_token, return_type->end_token, KEYWORD_COLOR, file);
            print_ws(return_type->end_token, file);
            colored(def->signature->name->span, FUNC_NAME_DEF_COLOR, file);
            colored_token_span(def->signature->name->next, def->last_stmt->start_token, NO_COLOR, file);

            NodeHeader *st, *last = NULL;

            for (st = def->last_stmt; st != NULL; st = st->next) {
                render_statement_expr(st, file);
                last = st;
            }

            if (last != NULL) {
                colored_token_span(last->end_token, def->header.end_token, NO_COLOR, file);
            }
        }

        last_node = node;
        node = node->next;
    }

    colored_token_span(last_node->end_token, last_node->end_token->next, NO_COLOR, file);
}