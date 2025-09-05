#include <assert.h>
#include <stdio.h>
#include "parser.h"
#include "html_writer.h"

static void write_serial(HtmlHandle *html, NodeHeader *last);
static void write_params(HtmlHandle *html, Declaration *last_param);
static void write_decl(HtmlHandle *html, Declaration *decl, bool is_struct_member);
static void write_statement(HtmlHandle *html, NodeHeader *st);
static void write_member_decls(HtmlHandle *html, Declaration *last_decl);

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

static void write_tokenc(HtmlHandle *html, Token *token, char *class) {
    html_open_tag(html, "span");
        html_add_attr(html, "class", class);
        html_write_token(html, token);
    html_close_tag(html);
}

static void write_token_spanc(HtmlHandle *html, Token *tokenp, Token *end_token, char *class) {
    html_open_tag(html, "span");
    for (; tokenp != end_token; tokenp = tokenp->next) {
        html_add_attr(html, "class", class);
        html_write_token(html, tokenp);
    }
    html_close_tag(html);
}

static void write_params(HtmlHandle *html, Declaration *last_param) {
    NodeHeader *head = last_param->header.next;
    NodeHeader *param = head;

    for (param = param->next; param != head; param = param->next) {
        write_decl(html, (Declaration *)param, false);
        if (param->next != head) write_token_span(html, param->end_token, param->next->start_token);
    }
}

static void write_decl(HtmlHandle *html, Declaration *decl, bool is_struct_member) {
    if (decl->var_arg) {
        write_token_span(html, decl->header.start_token, decl->header.end_token);
        return;
    }

    for (Token *t = decl->data_type->start_token; t != decl->data_type->end_token; t = t->next) {
        if (t->type == KEYWORD_TOKEN) {
            write_tokenc(html, t, "keyword");
        } else if (t == decl->data_type->struct_id || t == decl->data_type->typedef_id) {
            write_tokenc(html, t, "typename");
        } else {
            html_write_token(html, t);
        }
    }

    write_ws(html, decl->data_type->end_token);

    if (is_struct_member) {
        write_tokenc(html, decl->id, "member");
    } else {
        html_write_token(html, decl->id);
    }


    write_ws_after(html, decl->id);

    if (decl->assign != NULL) {
        html_write_token(html, decl->assign->equal_sign);
        write_ws_after(html, decl->assign->equal_sign);
        write_statement(html, decl->assign->expr);
    }
}

static void write_member_decls(HtmlHandle *html, Declaration *last_decl) {
    NodeHeader *head = last_decl->header.next;
    NodeHeader *param = head;

    for (param = param->next; param != head; param = param->next) {
        write_decl(html, (Declaration *) param, true);
        if (param->next != head) write_token_span(html, param->end_token, param->next->start_token);
    }
}

static void write_serial(HtmlHandle *html, NodeHeader *last) {
    NodeHeader *head = last->next;
    NodeHeader *st = head;

    for (st = st->next; st != head; st = st->next) {
        write_statement(html, st);
        if (st->next != head) write_token_span(html, st->end_token, st->next->start_token);
    }
}

static void write_statement(HtmlHandle *html, NodeHeader *st) {
    switch (st->type) {
        case FUNC_INVOKE: {
            FuncInvoke *invoke = (FuncInvoke*) st;
            NodeHeader *head_arg = invoke->last_arg->next;
            write_token_span(html, invoke->name, head_arg->start_token);
            write_serial(html, invoke->last_arg);
            write_token_span(html, invoke->last_arg->end_token, invoke->header.end_token);
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
            write_decl(html, (Declaration *) st, false);
        } break;
        case IF_STATEMENT: {
            IfStatement *ifst = (IfStatement *) st;
            write_token_spanc(html, ifst->header.start_token, ifst->header.start_token->next, "keyword");
            write_token_span(html, ifst->header.start_token->next, ifst->cond->start_token);
            write_statement(html, ifst->cond);

            write_token_span(html, ifst->cond->end_token, ifst->then_statement->next->start_token);
            write_serial(html, ifst->then_statement);

            write_token_span(html, ifst->then_statement->end_token, ifst->else_token ? ifst->else_token : ifst->header.end_token);

            if (ifst->else_token) {
                write_token_spanc(html, ifst->else_token, ifst->else_token->next, "keyword");
                write_token_span(html, ifst->else_token->next, ifst->else_statement->start_token);
                write_serial(html, ifst->else_statement);
                write_token_span(html, ifst->else_statement->end_token, ifst->header.end_token);
            }
        } break;
        case UNARY_OP: {
            UnaryOp *op = (UnaryOp *) st;
            write_token_span(html, op->header.start_token, op->expr->start_token);
            write_statement(html, op->expr);
        } break;
        case BINARY_OP: {
            BinaryOp *op = (BinaryOp *) st;
            write_statement(html, op->lhs);
            write_token_span(html, op->lhs->end_token, op->rhs->start_token);
            write_statement(html, op->rhs);
        } break;
        case VAR_REFERENCE: {
            VarReference *ref = (VarReference *) st;
            write_token_span(html, ref->id, ref->id->next);
        } break;
        case DEFINE_REFERENCE: {
            DefineReference *ref = (DefineReference *) st;
            write_token_spanc(html, ref->header.start_token, ref->header.end_token, "prepid");
        } break;
        case ARRAY_ACCESS: {
            ArrAccess *access = (ArrAccess *) st;
            write_token_span(html, access->id, access->id->next);
            write_token_span(html, access->id->next, access->index_expr->start_token);
            write_statement(html, access->index_expr);
            write_token_span(html, access->index_expr->end_token, access->header.end_token);
        } break;
        case GOTO_STATEMENT: {
            GotoStatement *got = (GotoStatement *) st;
            write_tokenc(html, got->header.start_token, "keyword");
            write_token_span(html, got->header.start_token->next, got->header.end_token);
        } break;
        case LABEL_DECL: {
            write_token_span(html, st->start_token, st->end_token);
        } break;
        case STRUCT_INIT: {
            StructInit *init = (StructInit *) st;
            NodeHeader *head = init->last_expr->next;

            for (Token *t = st->start_token; t != head->start_token; t = t->next) {
                if (t->type == OPEN_CURLY_TOKEN) {
                    write_tokenc(html, t, "init");
                } else {
                    html_write_token(html, t);
                }
            }

            write_serial(html, init->last_expr);
            for (Token *t = init->last_expr->end_token; t != st->end_token; t = t->next) {
                if (t->type == CLOSE_CURLY_TOKEN) {
                    write_tokenc(html, t, "init");
                } else {
                    html_write_token(html, t);
                }
            }
        } break;
        case ARROW_OP: {
            ArrowOp *op = (ArrowOp *) st;
            write_token_span(html, op->lhs->start_token, op->member);
            write_tokenc(html, op->member, "member");
        } break;
        case LINE_COMMENT:
        case MULTI_COMMENT: {
            write_token_spanc(html, st->start_token, st->end_token, "comment");
        } break;
        case ASSIGNMENT: {
            Assignment *assign = (Assignment *) st;
            write_token_span(html, st->start_token, assign->expr->start_token);
            write_statement(html, assign->expr);
        } break;
        default: {
            assert(0);
        } break;
    }
}

static void write_code(HtmlHandle *html, NodeHeader *node) {
    NodeHeader *prev_node = node;

    while (node != NULL) {
        switch (node->type) {
            case INCLUDE_DIRECTIVE: {
                Include *inc = (Include *) node;
                write_token_spanc(html, inc->header.start_token, inc->header.start_token->next, "prep");
                write_ws_after(html, inc->header.start_token);
                write_token_spanc(html, inc->pathOrHeader, inc->pathOrHeader->next, "str");
            } break;
            case DEFINE_DIRECTIVE: {
                Define *def = (Define *) node;
                write_token_spanc(html, def->header.start_token, def->header.start_token->next, "prep");
                write_ws_after(html, def->header.start_token);
                write_token_spanc(html, def->id, def->id->next, "prepid");
                write_ws_after(html, def->id);
                write_statement(html, def->expr);
            } break;
            case FUNC_DEF: {
                FuncDef *def = (FuncDef *) node;
                DataType *return_type = def->signature->return_type;
                write_token_spanc(html, return_type->start_token, return_type->end_token, "keyword");
                write_ws(html, return_type->end_token);
                write_token_spanc(html, def->signature->name, def->signature->name->next, "func-name");
                Declaration *last_param = def->signature->last_param;
                Declaration *head_param = (Declaration *) last_param->header.next;
                write_token_span(html, def->signature->name->next, head_param->header.next->start_token);

                write_params(html, last_param);
                write_token_span(html, last_param->header.end_token, def->last_stmt->next->start_token);

                write_serial(html, def->last_stmt);
                write_token_span(html, def->last_stmt->end_token, def->header.end_token);
            } break;
            case STRUCT_DECL: {
                StructDecl *decl = (StructDecl *) node;
                write_tokenc(html, node->start_token, "keyword");
                write_ws_after(html, node->start_token);
                NodeHeader *head_decl = decl->last_decl->header.next;
                write_tokenc(html, decl->id, "typename");
                write_token_span(html, decl->id->next, head_decl->start_token);
                write_member_decls(html, decl->last_decl);
                write_token_span(html, decl->last_decl->header.end_token, node->end_token);
            } break;
            case LINE_COMMENT:
            case MULTI_COMMENT: {
                write_token_spanc(html, node->start_token, node->end_token, "comment");
            } break;
            default: {
                assert(0);
            } break;
        }

        prev_node = node;
        node = node->next;

        if (node != NULL) write_token_span(html, prev_node->end_token, node->start_token);
    }

    write_token_span(html, prev_node->end_token, prev_node->end_token->next);
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
