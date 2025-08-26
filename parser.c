#include "parser.h"
#include <assert.h>

static char *primitive_types[] = {
    "double", "char", "float", "int", "long", "short"
};

#define PRIMITIVE_COUNT (sizeof(primitive_types) / sizeof(primitive_types[0]))

static void skip_token(TokenType token_type);
static void skip_white();
static FuncSignature *parse_func_signature();
static DataType *parse_data_type();
static NodeHeader *parse_block();
static NodeHeader *parse_func_body();
static NodeHeader *parse_statement();
static FuncInvoke *parse_func_invoke();
static NodeHeader *parse_expr();
// static NodeHeader *parse_expr_span(Token *tokenp, Token *end_token);
static ReturnStatement *parse_return();
static Declaration *parse_decl();
static Assignment *parse_assign();
static IfStatement *parse_if();

static Token *token;
static NodeHeader *first_element = NULL, *element = NULL;

static void insert(NodeHeader *el) {
    NodeHeader *prev = element;

    element = el;

    if (first_element == NULL) first_element = el;
    else prev->next = element;
}

NodeHeader *parse(Token *first_token) {
    Token *start_token;

    for (token = first_token; token != NULL; ) {
        switch (token->type) {
            case INCLUDE_HEADER: {
                start_token = token;
                token = token->next;
                skip_white();

                if (token->type == HEADER_NAME_TOKEN) {
                    Token *header = token;
                    token = token->next;

                    IncludeHeaderName *Include = pool_alloc_struct(IncludeHeaderName);
                    Include->header = (NodeHeader) {INCLUDE_HEADER, start_token, token};
                    Include->name = header;
                    insert((NodeHeader *)Include);
                }
            } break;
            case STUB_TOKEN: {
                token = token->next;
            } break;
            case WHITESPACE_TOKEN: {
                token = token->next;
            } break;
            case KEYWORD_TOKEN: {
                start_token = token;
                FuncSignature *sign = parse_func_signature();
                skip_white();

                if (token->type == SEMICOLON_TOKEN) { // Func declaration

                } else if (token->type == OPEN_CURLY_TOKEN) { // Func definition
                    NodeHeader *first_statement = parse_func_body();
                    FuncDef *def = pool_alloc_struct(FuncDef);
                    def->header = (NodeHeader) {FUNC_DEF, start_token, token};
                    def->signature = sign;
                    def->last_stmt = first_statement;
                    insert((NodeHeader *)def);
                }
            } break;
            default: {
                assert(0);
            } break;
        }
    }

    return first_element;
}

static void skip_white() {
    for (; token != NULL && token->type == WHITESPACE_TOKEN; token = token->next)
        ;
}

static void skip_token(TokenType token_type) {
    assert(token->type == token_type);
    token = token->next;
}

static DataType *parse_data_type() {
    DataType *data_type = pool_alloc(sizeof(DataType), DataType);
    data_type->start_token = token;
    if (spanstrcmp(token->span, "int") >= 0) { // TODO: Lookup
        data_type->primitive = INT_TYPE;
    }

    token = token->next;
    data_type->end_token = token;

    return data_type;
}

static FuncSignature *parse_func_signature() {
    DataType *type = parse_data_type();
    skip_white();
    Token *name = token;
    token = token->next;

    skip_white();
    skip_token(OPEN_PAREN_TOKEN);
    skip_white();
    skip_token(CLOSE_PAREN_TOKEN);
    skip_white();

    FuncSignature *signature = pool_alloc(sizeof(FuncSignature), FuncSignature);
    signature->name = name;
    signature->return_type = type;
    return signature;
}

static NodeHeader *parse_func_body() {
    return parse_block();
}

static NodeHeader *parse_statement() {
    if (token->type == IDENTIFIER_TOKEN) {
        if (token->next->type == WHITESPACE_TOKEN ? token->next->next->type == OPEN_PAREN_TOKEN : token->next->type == OPEN_PAREN_TOKEN) {
            NodeHeader *invoke_expr = (NodeHeader *) parse_func_invoke();
            return invoke_expr;
        }
    } else if (token->type == KEYWORD_TOKEN) {
        // TODO: Table lookup
        if (spanstrcmp(token->span, "return") >= 0) {
            NodeHeader *ret = (NodeHeader *) parse_return();
            return ret;
        } else if (binsearch_span(token->span, primitive_types, PRIMITIVE_COUNT) >= 0) {
            Declaration *decl = parse_decl();
            skip_white();

            if (token->type == EQUAL_SIGN_TOKEN) {
                token = decl->varname;
                decl->assign = parse_assign();
            }

            decl->header.end_token = token;

            return (NodeHeader *) decl;
        } else if (spanstrcmp(token->span, "if") >= 0) {
            NodeHeader *ifcond = (NodeHeader *) parse_if();
            return ifcond;
        }
    }

    assert(0);
}

static FuncInvoke *parse_func_invoke() {
    Token *name = token;
    skip_token(IDENTIFIER_TOKEN);
    skip_white();
    skip_token(OPEN_PAREN_TOKEN);
    skip_white();

    NodeHeader *first_expr = NULL, *prev, *expr;

    do {
        if (token->type == CLOSE_PAREN_TOKEN) break;

        expr = parse_expr();
        skip_white();

        if (token->type == COMMA_TOKEN) skip_token(COMMA_TOKEN);
        skip_white();

        if (first_expr == NULL) first_expr = expr;
        else prev->next = expr;

        prev = expr;
    } while (token->type != CLOSE_PAREN_TOKEN);

    skip_token(CLOSE_PAREN_TOKEN);

    FuncInvoke *invoke = pool_alloc_struct(FuncInvoke);
    invoke->header = (NodeHeader) {FUNC_INVOKE, name, token};
    invoke->name = name;
    invoke->arg = first_expr;
    return invoke;
}

static ReturnStatement *parse_return() {
    Token *start = token;
    skip_token(KEYWORD_TOKEN);
    skip_white();

    ReturnStatement *ret = pool_alloc_struct(ReturnStatement);
    NodeHeader *expr = parse_expr();
    ret->header = (NodeHeader) {RETURN_STATEMENT, start, token};
    ret->expr = expr;
    return ret;
}

static Declaration *parse_decl() {
    Token *start = token;
    Declaration *decl = pool_alloc_struct(Declaration);
    DataType *type = parse_data_type();
    skip_white();
    Token *varname = token;

    skip_token(IDENTIFIER_TOKEN);

    decl->header = (NodeHeader) {DECLARATION, start};
    decl->data_type = type;
    decl->varname = varname;
    decl->assign = NULL;

    return decl;
}

static Assignment *parse_assign() {
    Token *start = token;
    Assignment *assign = pool_alloc_struct(Assignment);
    Token *varname = token;
    skip_token(IDENTIFIER_TOKEN);
    skip_white();
    Token *sign = token;
    skip_token(EQUAL_SIGN_TOKEN);
    skip_white();

    assign->varname = varname;
    assign->equal_sign = sign;
    assign->expr = parse_expr();
    assign->header = (NodeHeader) {ASSIGNMENT, start, token};

    return assign;
}

static IfStatement *parse_if() {
    Token *start = token;
    skip_token(KEYWORD_TOKEN);
    skip_white();
    skip_token(OPEN_PAREN_TOKEN);
    skip_white();

    NodeHeader *cond = parse_expr();

    skip_white();
    skip_token(CLOSE_PAREN_TOKEN);
    skip_white();

    IfStatement *ifstat = pool_alloc_struct(IfStatement);
    ifstat->cond = cond;
    ifstat->then_statement = token->type == OPEN_CURLY_TOKEN ? parse_block() : parse_statement();

    NodeHeader *elsest = pool_alloc_struct(NodeHeader);
    elsest->type = STUB;
    elsest->start_token = token;
    elsest->end_token = token;
    elsest->next = elsest;

    ifstat->else_statement = elsest;

    ifstat->header = (NodeHeader) {IF_STATEMENT, start, token};

    return ifstat;
}

static NodeHeader *parse_expr_lazy() {
    if (token->type == S_CHAR_SEQ_TOKEN) {
        StringLiteral *literal = pool_alloc_struct(StringLiteral);
        literal->header = (NodeHeader) {STRING_LITERAL, token, token->next};
        literal->str = token;

        token = token->next;

        return (NodeHeader *) literal;
    } else if (token->type == NUM_LITERAL_TOKEN) {
        IntLiteral *literal = pool_alloc_struct(IntLiteral);
        literal->header = (NodeHeader) {INT_LITERAL, token, token->next};
        literal->num = token;

        token = token->next;

        return (NodeHeader *) literal;
    } else if (token->type == IDENTIFIER_TOKEN) {
        VarReference *ref = pool_alloc_struct(VarReference);
        ref->header = (NodeHeader) {VAR_REFERENCE, token, token->next};
        ref->varname = token;

        token = token->next;

        return (NodeHeader *) ref;
    }

    assert(0);
}

static NodeHeader *parse_expr() {
    Token *start = token;
    NodeHeader *lhs = parse_expr_lazy();
    skip_white();
    if (token->type == GREATER_TOKEN) { // TODO: Check for other binary and unary operators tokens
        token = token->next;
        skip_white();
        GreaterComp *comp = pool_alloc_struct(GreaterComp);
        comp->lhs = lhs;
        comp->rhs = parse_expr_lazy();
        comp->header = (NodeHeader) {GREATER_COMP, start, token};
        return (NodeHeader *) comp;
    } else {
        return lhs;
    }
}

static NodeHeader *parse_block() {
    skip_token(OPEN_CURLY_TOKEN);
    skip_white();

    NodeHeader *stub = pool_alloc_struct(NodeHeader);
    stub->type = STUB;
    stub->start_token = token;
    stub->end_token = token;

    NodeHeader *st, *prev;
    st = prev = stub;

    while (token->type != CLOSE_CURLY_TOKEN) {
        st = parse_statement();
        prev->next = st;
        skip_white();
        if (token->type == SEMICOLON_TOKEN) skip_token(SEMICOLON_TOKEN);
        skip_white();

        prev = st;
    }

    skip_token(CLOSE_CURLY_TOKEN);
    st->next = stub;

    return st;

    // NodeHeader *first_statement = NULL, *prev, *st;
    // if (token->type == CLOSE_CURLY_TOKEN) {
    //     skip_token(CLOSE_CURLY_TOKEN);
    //     return NULL;
    // }

    // do {
    //     st = parse_statement();
    //     skip_white();
    //     if (st->type != IF_STATEMENT) { // TODO: Add more statements
    //         skip_token(SEMICOLON_TOKEN);
    //     }
    //
    //     skip_white();
    //
    //     if (first_statement == NULL) first_statement = st;
    //     else prev->next = st;
    //
    //     prev = st;
    // } while (token->type != CLOSE_CURLY_TOKEN);
    //
    // skip_token(CLOSE_CURLY_TOKEN);
    // return first_statement;
}

