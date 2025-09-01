#include "parser.h"
#include <assert.h>
#include <string.h>

#include "prep.h"

typedef struct {
    char *name;
    PrimitiveDataType type;
} Primitive;

static Primitive primitive_types[] = {
    (Primitive){"double", DOUBLE_TYPE},
    (Primitive){"char", CHAR_TYPE},
    (Primitive){"float", FLOAT_TYPE},
    (Primitive){"int", INT_TYPE},
    (Primitive){"long", LONG_TYPE},
    (Primitive){"short", SHORT_TYPE},
    (Primitive){"void", VOID_TYPE},
};

#define PRIMITIVE_COUNT (sizeof(primitive_types) / sizeof(primitive_types[0]))

static TokenType binary_operations[] = {
    NOT_EQUAL_TOKEN, DOUBLE_EQUAL_TOKEN,
    GREATER_TOKEN, GREATER_OR_EQUAL_TOKEN,
    LESSER_TOKEN, LESSER_OR_EQUAL_TOKEN,
};

#define BINARY_OP_COUNT (sizeof(binary_operations) / sizeof(binary_operations[0]))

static void skip_token(TokenType token_type);
static void skip_white();
static void next_token();
static Token *nonws_token();
static FuncSignature *parse_func_signature();
static DataType *parse_data_type();
static NodeHeader *parse_block();
static NodeHeader *parse_func_body();
static NodeHeader *parse_statement();
static FuncInvoke *parse_func_invoke();
static NodeHeader *parse_expr();
static ReturnStatement *parse_return();
static Declaration *parse_decl();
static Assignment *parse_assign();
static IfStatement *parse_if();
static int binsearch_primitive(Span, Primitive *, size_t);

static Token *token;
static NodeHeader *first_element = NULL, *element = NULL;

static DefineTable *define_table;

static void insert(NodeHeader *el) {
    NodeHeader *prev = element;

    element = el;

    if (first_element == NULL) first_element = el;
    else prev->next = element;
}

NodeHeader *parse(Token *first_token) {
    first_element = element = NULL;
    Token *start_token;
    define_table = prep_define_newtable();

    for (token = first_token; nonws_token() != NULL; ) {
        switch (nonws_token()->type) {
            case INCLUDE_DIRECTIVE: {
                start_token = nonws_token();
                next_token();

                Include *inc = pool_alloc_struct(Include);

                assert(nonws_token()->type == HEADER_NAME_TOKEN || nonws_token()->type == INCLUDE_PATH_TOKEN);
                inc->pathOrHeader = nonws_token();
                inc->include_type = nonws_token()->type == HEADER_NAME_TOKEN ? IncludeHeaderType : IncludePathType;
                next_token();
                inc->header = (NodeHeader) {INCLUDE_DIRECTIVE, start_token, token};
                insert((NodeHeader *) inc);
            } break;
            case DEFINE_TOKEN: {
                start_token = nonws_token();
                next_token();

                Define *def = pool_alloc_struct(Define);
                def->id = nonws_token();
                next_token();
                def->expr = parse_expr();
                def->header = (NodeHeader) {DEFINE_DIRECTIVE, start_token, token};
                insert((NodeHeader *) def);

                prep_define_set(define_table, def->id->span, def->expr);
            } break;
            case STUB_TOKEN: {
                next_token();
            } break;
            case KEYWORD_TOKEN: {
                start_token = nonws_token();
                FuncSignature *sign = parse_func_signature();

                if (nonws_token()->type == SEMICOLON_TOKEN) { // Func declaration

                } else if (nonws_token()->type == OPEN_CURLY_TOKEN) { // Func definition
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

static Token *nonws_token() {
    if (token && token->type == WHITESPACE_TOKEN) token = token->next;
    return token;
}

static void next_token() {
    token = token->next;
}

static void skip_token(TokenType token_type) {
    assert(nonws_token()->type == token_type);
    next_token();
}

static void skip_white() {
    if (token && token->type == WHITESPACE_TOKEN) token = token->next;
}

static DataType *parse_data_type() {
    DataType *data_type = pool_alloc(sizeof(DataType), DataType);
    data_type->start_token = nonws_token();

    Token *end_token;

    if (token->type == IDENTIFIER_TOKEN) { // typedef
        data_type->typedef_token = token;
        next_token();
        end_token = token;
    } else {
        data_type->primitive = UNKNOWN_PRIMITIVE_TYPE;

        int i;
        if ((i = binsearch_primitive(data_type->start_token->span, primitive_types, PRIMITIVE_COUNT)) >= 0) {
            data_type->primitive = primitive_types[i].type;
        }

        next_token();
        end_token = token;
    }

    skip_white();

    data_type->pointer = NO_POINTER_TYPE;
    if (token->type == STAR_TOKEN) {
        data_type->pointer = POINTER_TYPE;
        next_token();
        end_token = token;
    }

    skip_white();
    if (nonws_token()->type == STAR_TOKEN) {
        data_type->pointer = POINTER_TO_POINTER_TYPE;
        next_token();
        end_token = token;
    }

    data_type->end_token = end_token;
    token = end_token;

    return data_type;
}

static FuncSignature *parse_func_signature() {
    DataType *type = parse_data_type();
    Token *name = nonws_token();
    skip_token(IDENTIFIER_TOKEN);

    skip_token(OPEN_PAREN_TOKEN);

    Declaration *stub = pool_alloc_struct(Declaration);
    stub->header = (NodeHeader) {STUB, nonws_token(), nonws_token()};

    Declaration *param, *prev;
    param = prev = stub;

    while (nonws_token()->type != CLOSE_PAREN_TOKEN) {
        param = parse_decl();
        prev->header.next = (NodeHeader *) param;
        if (nonws_token()->type == COMMA_TOKEN) skip_token(COMMA_TOKEN);

        prev = param;
    }

    param->header.next = (NodeHeader *) stub;

    skip_token(CLOSE_PAREN_TOKEN);

    FuncSignature *signature = pool_alloc(sizeof(FuncSignature), FuncSignature);
    signature->name = name;
    signature->return_type = type;
    signature->last_param = param;
    return signature;
}

static NodeHeader *parse_func_body() {
    return parse_block();
}

static NodeHeader *parse_statement() {
    if (nonws_token()->type == IDENTIFIER_TOKEN) {
        if (nonws_token()->next->type == OPEN_PAREN_TOKEN) {
            NodeHeader *invoke_expr = (NodeHeader *) parse_func_invoke();
            return invoke_expr;
        } else {
            Declaration *decl = parse_decl();
            return (NodeHeader *) decl;
        }
    } else if (nonws_token()->type == KEYWORD_TOKEN) {
        // TODO: Table lookup
        if (spanstrcmp(nonws_token()->span, "return") == 0) {
            NodeHeader *ret = (NodeHeader *) parse_return();
            return ret;
        } else if (spanstrcmp(nonws_token()->span, "if") == 0) {
            NodeHeader *ifcond = (NodeHeader *) parse_if();
            return ifcond;
        } else if (binsearch_span(nonws_token()->span, primitive_types, PRIMITIVE_COUNT) >= 0) {
            Declaration *decl = parse_decl();
            // skip_white();

            if (nonws_token()->type == EQUAL_TOKEN) {
                token = decl->id;
                decl->assign = parse_assign();
            }

            decl->header.end_token = nonws_token();

            return (NodeHeader *) decl;
        }
    } else {
        return parse_expr();
    }

    assert(0);
}

static FuncInvoke *parse_func_invoke() {
    Token *name = nonws_token();
    skip_token(IDENTIFIER_TOKEN);
    // skip_white();
    skip_token(OPEN_PAREN_TOKEN);
    // skip_white();

    NodeHeader *first_expr = NULL, *prev, *expr;

    do {
        if (nonws_token()->type == CLOSE_PAREN_TOKEN) break;

        expr = parse_expr();
        // skip_white();

        if (nonws_token()->type == COMMA_TOKEN) skip_token(COMMA_TOKEN);
        // skip_white();

        if (first_expr == NULL) first_expr = expr;
        else prev->next = expr;

        prev = expr;
    } while (nonws_token()->type != CLOSE_PAREN_TOKEN);

    skip_token(CLOSE_PAREN_TOKEN);

    FuncInvoke *invoke = pool_alloc_struct(FuncInvoke);
    invoke->header = (NodeHeader) {FUNC_INVOKE, name, token};
    invoke->name = name;
    invoke->arg = first_expr;
    return invoke;
}

static ReturnStatement *parse_return() {
    Token *start = nonws_token();
    skip_token(KEYWORD_TOKEN);
    // skip_white();

    ReturnStatement *ret = pool_alloc_struct(ReturnStatement);
    NodeHeader *expr = parse_expr();
    ret->header = (NodeHeader) {RETURN_STATEMENT, start, token};
    ret->expr = expr;
    return ret;
}

static Declaration *parse_decl() {
    Token *start = nonws_token();
    Declaration *decl = pool_alloc_struct(Declaration);
    decl->var_arg = false;

    if (nonws_token()->type == ELLIPSIS_TOKEN) {
        decl->var_arg = true;
        skip_token(ELLIPSIS_TOKEN);
    } else {
        DataType *type = parse_data_type();
        decl->id = nonws_token();
        decl->data_type = type;
        decl->assign = NULL;

        skip_token(IDENTIFIER_TOKEN);
    }

    decl->header = (NodeHeader) {DECLARATION, start, token};

    return decl;
}

static Assignment *parse_assign() {
    Token *start = nonws_token();
    Assignment *assign = pool_alloc_struct(Assignment);
    Token *varname = nonws_token();
    skip_token(IDENTIFIER_TOKEN);
    Token *sign = nonws_token();
    skip_token(EQUAL_TOKEN);

    assign->varname = varname;
    assign->equal_sign = sign;
    assign->expr = parse_expr();
    assign->header = (NodeHeader) {ASSIGNMENT, start, token};

    return assign;
}

static IfStatement *parse_if() {
    Token *start = nonws_token();
    skip_token(KEYWORD_TOKEN);
    skip_token(OPEN_PAREN_TOKEN);

    NodeHeader *cond = parse_expr();

    skip_token(CLOSE_PAREN_TOKEN);

    IfStatement *ifstat = pool_alloc_struct(IfStatement);
    ifstat->cond = cond;
    ifstat->then_statement = nonws_token()->type == OPEN_CURLY_TOKEN ? parse_block() : parse_statement();
    Token *endif = token;
    ifstat->else_token = NULL;

    if (spanstrcmp(nonws_token()->span, "else") == 0) {
        ifstat->else_token = token;
        skip_token(KEYWORD_TOKEN);
        ifstat->else_statement = nonws_token()->type == OPEN_CURLY_TOKEN ? parse_block() : parse_statement();
        endif = token;
    } else {
        NodeHeader *elsest = pool_alloc_struct(NodeHeader);
        elsest->type = STUB;
        elsest->start_token = nonws_token();
        elsest->end_token = nonws_token();
        elsest->next = elsest;

        ifstat->else_statement = elsest;
    }

    ifstat->header = (NodeHeader) {IF_STATEMENT, start, endif};

    return ifstat;
}

static NodeHeader *parse_expr_lazy() {
    if (nonws_token()->type == S_CHAR_SEQ_TOKEN) {
        StringLiteral *literal = pool_alloc_struct(StringLiteral);
        literal->header = (NodeHeader) {STRING_LITERAL, token, nonws_token()->next};
        literal->str = nonws_token();

        next_token();

        return (NodeHeader *) literal;
    } else if (nonws_token()->type == NUM_LITERAL_TOKEN) {
        IntLiteral *literal = pool_alloc_struct(IntLiteral);
        literal->header = (NodeHeader) {INT_LITERAL, token, nonws_token()->next};
        literal->num = nonws_token();

        next_token();

        return (NodeHeader *) literal;
    } else if (nonws_token()->type == IDENTIFIER_TOKEN) {
        NodeHeader *def_expr;

        NodeHeader *node;
        if ((def_expr = prep_define_get(define_table, token->span)) != NULL) {
            DefineReference *ref = pool_alloc_struct(DefineReference);
            ref->header = (NodeHeader) {DEFINE_REFERENCE, token, nonws_token()->next};
            ref->expr = def_expr;
            node = (NodeHeader *) ref;
        } else {
            VarReference *ref = pool_alloc_struct(VarReference);
            ref->header = (NodeHeader) {VAR_REFERENCE, token, nonws_token()->next};
            ref->varname = nonws_token();
            node = (NodeHeader *) ref;
        }

        next_token();

        return node;
    }

    assert(0);
}

static NodeHeader *parse_expr() {
    Token *start = nonws_token();
    NodeHeader *lhs = parse_expr_lazy();
    Token *after_expr = token;
    if (nonws_token()->type == GREATER_TOKEN) { // TODO: Check for other binary and unary operators tokens
        next_token();
        GreaterComp *comp = pool_alloc_struct(GreaterComp);
        comp->lhs = lhs;
        comp->rhs = parse_expr_lazy();
        comp->header = (NodeHeader) {BINARY_OP, start, token};
        return (NodeHeader *) comp;
    } else {
        token = after_expr;
        return lhs;
    }
}

static NodeHeader *parse_block() {
    skip_token(OPEN_CURLY_TOKEN);

    NodeHeader *stub = pool_alloc_struct(NodeHeader);
    stub->type = STUB;
    stub->start_token = nonws_token();
    stub->end_token = nonws_token();

    NodeHeader *st, *prev;
    st = prev = stub;

    while (nonws_token()->type != CLOSE_CURLY_TOKEN) {
        st = parse_statement();
        prev->next = st;
        if (nonws_token()->type == SEMICOLON_TOKEN) skip_token(SEMICOLON_TOKEN);

        prev = st;
    }

    skip_token(CLOSE_CURLY_TOKEN);
    st->next = stub;

    return st;
}

static int binsearch_primitive(Span target, Primitive *arr, size_t size) {
    int low = 0;
    int high = size - 1;
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