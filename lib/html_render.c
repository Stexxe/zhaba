#include <assert.h>
#include <stdio.h>
#include "parser.h"
#include "html_writer.h"

static void write_statements(HtmlHandle *html, NodeHeader *last);
static void write_statement(HtmlHandle *html, NodeHeader *st);

static void write_head(HtmlHandle *html, char *filename) {
    html_open_tag(html, "head");
        html_open_tag(html, "meta");
            html_add_attr(html, "charset", "UTF-8");
        html_close_tag(html);
        html_open_tag(html, "title");
            html_write_text_raw(html, filename);
        html_close_tag(html);
        html_open_tag(html,  "link");
            html_add_attr(html, "rel", "preconnect");
            html_add_attr(html, "href", "https://fonts.googleapis.com");
        html_close_tag(html);
        html_open_tag(html,  "link");
            html_add_attr(html, "rel", "preconnect");
            html_add_attr(html, "href", "https://fonts.gstatic.com");
            html_add_flag(html, "crossorigin");
        html_close_tag(html);
        html_open_tag(html,  "link");
            html_add_attr(html, "rel", "stylesheet");
            html_add_attr(html, "href", "https://fonts.googleapis.com/css2?family=JetBrains+Mono:ital,wght@0,100..800;1,100..800&display=swap");
        html_close_tag(html);
        html_open_tag(html,  "link");
            html_add_attr(html, "rel", "stylesheet");
            html_add_attr(html, "href", "css/reset.css");
        html_close_tag(html);
        html_open_tag(html,  "link");
            html_add_attr(html, "rel", "stylesheet");
            html_add_attr(html, "href", "css/style.css");
        html_close_tag(html);
    html_close_tag(html);
}

static void write_ws_after(HtmlHandle *html, Token *token) {
    for (token = token->next; token != NULL && token->type == WHITESPACE_TOKEN; token = token->next) {
        html_write_token(html, token);
    }
}

static void write_ws(HtmlHandle *html, Token *token) {
    for (; token != NULL && token->type == WHITESPACE_TOKEN; token = token->next) {
        html_write_token(html, token);
    }
}

static void write_token_span(HtmlHandle *html, Token *tokenp, Token *end_token) {
    for (; tokenp != end_token; tokenp = tokenp->next) {
        html_write_token(html, tokenp);
    }
}

static void write_token_spanc(HtmlHandle *html, Token *tokenp, Token *end_token, char *class) {
    html_open_tag(html, "span");
    for (; tokenp != end_token; tokenp = tokenp->next) {
        html_add_attr(html, "class", class);
        html_write_token(html, tokenp);
    }
    html_close_tag(html);
}

static void write_statements(HtmlHandle *html, NodeHeader *last) {
    NodeHeader *head = last->next;
    NodeHeader *st = head;

    for (st = st->next; st != head; st = st->next) {
        write_statement(html, st);
        if (st->next != head) write_token_span(html, st->end_token, st->next->start_token);
    }

    // if (last != NULL) {
    //     write_token_span(html, last->end_token, def->header.end_token);
    // }
}

static void write_statement(HtmlHandle *html, NodeHeader *st) {
    switch (st->type) {
        case FUNC_INVOKE: {
            FuncInvoke *invoke = (FuncInvoke*) st;
            write_token_span(html, invoke->name, invoke->name->next);
            write_token_span(html, invoke->name->next, invoke->arg->start_token);

            NodeHeader *last_arg = NULL;
            for (NodeHeader *arg = invoke->arg; arg != NULL; arg = arg->next) {
                write_statement(html, arg);
                if (arg->next != NULL) {
                    write_token_span(html, arg->end_token, arg->next->start_token);
                }
                last_arg = arg;
            }

            if (last_arg != NULL) { // TODO: Handle no args
                write_token_span(html, last_arg->end_token, invoke->header.end_token);
            }
        } break;
        case RETURN_STATEMENT: {
            ReturnStatement *ret = (ReturnStatement*) st;
            write_token_spanc(html, ret->header.start_token, ret->header.start_token->next, "keyword");
            write_ws_after(html, ret->header.start_token);
            write_statement(html, ret->expr);
        } break;
        case STRING_LITERAL: {
            StringLiteral *literal = (StringLiteral*) st;
            write_token_spanc(html, literal->str, literal->str->next, "str");
        } break;
        case INT_LITERAL: {
            IntLiteral *literal = (IntLiteral*) st;
            write_token_spanc(html, literal->num, literal->num->next, "num");
        } break;
        case DECLARATION: {
            Declaration *decl = (Declaration*) st;
            write_token_spanc(html, decl->data_type->start_token, decl->data_type->end_token, "keyword");
            write_ws(html, decl->data_type->end_token);
            write_token_span(html, decl->varname, decl->varname->next);
            write_ws_after(html, decl->varname);

            if (decl->assign != NULL) {
                write_token_span(html, decl->assign->equal_sign, decl->assign->equal_sign->next);
                write_ws_after(html, decl->assign->equal_sign);
                write_statement(html, decl->assign->expr);
            }
        } break;
        case IF_STATEMENT: {
            IfStatement *ifst = (IfStatement *) st;
            write_token_spanc(html, ifst->header.start_token, ifst->header.start_token->next, "keyword");
            write_token_span(html, ifst->header.start_token->next, ifst->cond->start_token);
            write_statement(html, ifst->cond);
            write_token_span(html, ifst->cond->end_token, ifst->then_statement->start_token);

            write_statements(html, ifst->then_statement);
            write_token_span(html, ifst->then_statement->end_token, ifst->else_statement->start_token);
        } break;
        case GREATER_COMP: {
            GreaterComp *comp = (GreaterComp *) st;
            write_statement(html, comp->lhs);
            write_token_span(html, comp->lhs->end_token, comp->rhs->start_token);
            write_statement(html, comp->rhs);
        } break;
        case VAR_REFERENCE: {
            VarReference *ref = (VarReference *) st;
            write_token_span(html, ref->varname, ref->varname->next);
        } break;
        default: {
            assert(0);
        } break;
    }
}

static void write_code(HtmlHandle *html, NodeHeader *node) {
    NodeHeader *last_node = node;

    while (node != NULL) {
        if (node->type == INCLUDE_HEADER) {
            IncludeHeaderName *inc = (IncludeHeaderName *) node;
            write_token_spanc(html, inc->header.start_token, inc->header.start_token->next, "prep");
            write_ws_after(html, inc->header.start_token);
            write_token_spanc(html, inc->name, inc->name->next, "str");
            write_ws_after(html, inc->name);
        } else if (node->type == FUNC_DEF) {
            FuncDef *def = (FuncDef *) node;
            DataType *return_type = def->signature->return_type;
            write_token_spanc(html, return_type->start_token, return_type->end_token, "keyword");
            write_ws(html, return_type->end_token);
            write_token_spanc(html, def->signature->name, def->signature->name->next, "func-name");
            write_token_span(html, def->signature->name->next, def->last_stmt->next->start_token);

            write_statements(html, def->last_stmt);
            write_token_span(html, def->last_stmt->end_token, def->header.end_token);
        }

        last_node = node;
        node = node->next;
    }

    write_token_span(html, last_node->end_token, last_node->end_token->next);
}

void gen_html(NodeHeader *node, char *filename, int nlines, FILE *filep) {
    HtmlHandle *html = html_new(filep);

    html_add_doctype(html);
    html_open_tag(html, "html");
        html_add_attr(html, "lang", "en");
        write_head(html, filename);

        html_open_tag(html, "body");
            html_open_tag(html, "div");
                html_add_attr(html, "class", "wrap");
                html_open_tag(html, "div");
                    html_add_attr(html, "class", "panel");
                    for (int i = 1; i <= nlines; i++) {
                        html_write_textf(html, "%d", i);
                        html_open_tag(html, "br");
                        html_close_tag(html);
                    }
                html_close_tag(html);
                html_open_tag(html, "div");
                    html_add_attr(html, "class", "source");
                    write_code(html, node);
                html_close_tag(html);
            html_close_tag(html);
        html_close_tag(html);
    html_close_tag(html);

    html_close(html);
}
