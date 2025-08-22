#include "parser.h"
#include <assert.h>

static char *primitive_types[] = {
    "double", "char", "float", "int", "long", "short"
};

#define PRIMITIVE_COUNT (sizeof(primitive_types) / sizeof(primitive_types[0]))

static void skip_white();
static FuncSignature *parse_func_signature();
static DataType *parse_data_type();
static void skip_token(TokenType token_type);
static NodeHeader *parse_func_body();
static NodeHeader *parse_statement();
static FuncInvoke *parse_func_invoke();
static NodeHeader *parse_expr();
static ReturnStatement *parse_return();

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
                    def->first_statement = first_statement;
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
    skip_token(OPEN_CURLY_TOKEN);
    skip_white();

    NodeHeader *first_statement = NULL, *prev, *st;

    do {
        st = parse_statement();
        skip_white();
        skip_token(SEMICOLON_TOKEN);
        skip_white();

        if (first_statement == NULL) first_statement = st;
        else prev->next = st;

        prev = st;
    } while (token->type != CLOSE_CURLY_TOKEN);

    skip_token(CLOSE_CURLY_TOKEN);

    return first_statement;
}

static NodeHeader *parse_statement() {
    if (token->type == IDENTIFIER_TOKEN) {
        if (token->next->type == WHITESPACE_TOKEN ? token->next->next->type == OPEN_PAREN_TOKEN : token->next->type == OPEN_PAREN_TOKEN) {
            NodeHeader *invoke_expr = (NodeHeader *) parse_func_invoke();
            return invoke_expr;
        }
    } else if (token->type == KEYWORD_TOKEN) {
        if (spanstrcmp(token->span, "return") >= 0) {
            NodeHeader *ret = (NodeHeader *) parse_return();
            return ret;
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
    invoke->first_arg = first_expr;
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

static NodeHeader *parse_expr() {
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
    }

    assert(0);
}
