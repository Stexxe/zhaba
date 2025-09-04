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

// TODO: Sort once
static TokenType binary_operations[] = {
    NOT_EQUAL_TOKEN, DOUBLE_EQUAL_TOKEN,
    GREATER_TOKEN, GREATER_OR_EQUAL_TOKEN,
    LESSER_TOKEN, LESSER_OR_EQUAL_TOKEN, MINUS_TOKEN,
};

#define BINARY_OP_COUNT (sizeof(binary_operations) / sizeof(binary_operations[0]))

// TODO: Sort once
static TokenType unary_operations[] = {
    AMPERSAND_TOKEN, STAR_TOKEN
};

#define UNARY_OP_COUNT (sizeof(unary_operations) / sizeof(unary_operations[0]))

typedef NodeHeader *(*ParseFunc)(void);

typedef struct {
    char *keyword;
    ParseFunc parse;
} KeywordParser;

static void skip_token(TokenType token_type);
static void skip_white();
static void next_token();
static bool is_next_skipws(TokenType token_type);
static Token *nonws_token();
static FuncSignature *parse_func_signature();
static DataType *parse_data_type();
static NodeHeader *parse_block();
static NodeHeader *parse_func_body();
static NodeHeader *parse_statement();
static FuncInvoke *parse_func_invoke();
static NodeHeader *parse_expr();
static StructDecl *parse_struct_decl();
static Declaration *parse_decl_block();
static ReturnStatement *parse_return();
static GotoStatement *parse_goto();
static LabelDecl *parse_label();
static NodeHeader *parse_comment();
static Declaration *parse_decl();
static NodeHeader *parse_expr_list(TokenType open_token_type, TokenType close_token_type);
static Assignment *parse_assign();
static IfStatement *parse_if();
static int binsearch_primitive(Span, Primitive *, size_t);
static int binsearch_parser(Span target, KeywordParser *arr, size_t size);

// TODO: Sort once
static KeywordParser keyword_parsers[] = {
    (KeywordParser) {"goto", (ParseFunc) parse_goto},
    (KeywordParser) {"if", (ParseFunc) parse_if},
    (KeywordParser) {"return", (ParseFunc) parse_return},
};

#define KEYWORD_PARSER_COUNT (sizeof(keyword_parsers) / sizeof(keyword_parsers[0]))

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
            case LINE_COMMENT_TOKEN:
            case MULTI_COMMENT_TOKEN: {
                insert(parse_comment());
            } break;
            case KEYWORD_TOKEN: {
                start_token = nonws_token();

                if (spanstrcmp(token->span, "struct") == 0) {
                    insert((NodeHeader *) parse_struct_decl());
                    nonws_token();
                    skip_token(SEMICOLON_TOKEN);
                } else {
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

static bool is_next_skipws(TokenType token_type) {
    Token *next = token->next;
    return token_type == (next->type == WHITESPACE_TOKEN ? next->next : next)->type;
}

static void skip_white() {
    if (token && token->type == WHITESPACE_TOKEN) token = token->next;
}

static DataType *parse_data_type() {
    DataType *data_type = pool_alloc(sizeof(DataType), DataType);
    data_type->start_token = nonws_token();

    Token *end_token;

    if (token->type == IDENTIFIER_TOKEN) { // typedef
        data_type->kind = DATA_TYPE_TYPEDEF;
        data_type->typedef_id = token;
        next_token();
        end_token = token;
    } else if (token->type == KEYWORD_TOKEN) {
        if (spanstrcmp(token->span, "struct") == 0) {
            skip_token(KEYWORD_TOKEN);
            data_type->kind = DATA_TYPE_STRUCT;
            data_type->struct_id = nonws_token();
            skip_token(IDENTIFIER_TOKEN);
            end_token = token;
        } else {
            data_type->primitive = UNKNOWN_PRIMITIVE_TYPE;

            int i;
            if ((i = binsearch_primitive(data_type->start_token->span, primitive_types, PRIMITIVE_COUNT)) >= 0) {
                data_type->kind = DATA_TYPE_PRIMITIVE;
                data_type->primitive = primitive_types[i].type;
            }

            next_token();
            end_token = token;
        }
    } else {
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
    if (nonws_token()->type == LINE_COMMENT_TOKEN || nonws_token()->type == MULTI_COMMENT_TOKEN) {
        return parse_comment();
    } else if (nonws_token()->type == IDENTIFIER_TOKEN) {
        if (is_next_skipws(OPEN_PAREN_TOKEN)) {
            return (NodeHeader *) parse_func_invoke();
        } else if (is_next_skipws(COLON_TOKEN)) {
            return (NodeHeader *) parse_label();
        } else {
            return (NodeHeader *) parse_decl();
        }
    } else if (nonws_token()->type == KEYWORD_TOKEN) {
        int i;

        if ((i = binsearch_parser(nonws_token()->span, keyword_parsers, KEYWORD_PARSER_COUNT)) >= 0) {
            return keyword_parsers[i].parse();
        } else {
            Declaration *decl = parse_decl();
            return (NodeHeader *) decl;
        }
    } else {
        return parse_expr();
    }

    assert(0);
}

static FuncInvoke *parse_func_invoke() {
    FuncInvoke *invoke = pool_alloc_struct(FuncInvoke);
    invoke->name = nonws_token();
    skip_token(IDENTIFIER_TOKEN);
    nonws_token();
    invoke->last_arg = parse_expr_list(OPEN_PAREN_TOKEN, CLOSE_PAREN_TOKEN);
    invoke->header = (NodeHeader) {FUNC_INVOKE, invoke->name, token};
    return invoke;
}

static NodeHeader *parse_comment() {
    assert(token->type == LINE_COMMENT_TOKEN || token->type == MULTI_COMMENT_TOKEN);
    NodeHeader *comment = pool_alloc_struct(NodeHeader);
    comment->type = token->type == LINE_COMMENT_TOKEN ? LINE_COMMENT : MULTI_COMMENT;
    comment->start_token = token;
    comment->end_token = token->next;

    next_token();
    return comment;
}

static LabelDecl *parse_label() {
    Token *start = token;

    LabelDecl *label = pool_alloc_struct(LabelDecl);
    label->label = token;
    skip_token(IDENTIFIER_TOKEN);
    nonws_token();
    skip_token(COLON_TOKEN);
    label->header = (NodeHeader) {LABEL_DECL, start, token};
    return label;
}

static GotoStatement *parse_goto() {
    Token *start = token;
    skip_token(KEYWORD_TOKEN);
    GotoStatement *got = pool_alloc_struct(GotoStatement);
    got->label = nonws_token();
    skip_token(IDENTIFIER_TOKEN);
    got->header = (NodeHeader) {GOTO_STATEMENT, start, token};
    return got;
}

static StructDecl *parse_struct_decl() {
    Token *start = token;
    skip_token(KEYWORD_TOKEN);

    StructDecl *decl = pool_alloc_struct(StructDecl);
    decl->id = nonws_token();
    skip_token(IDENTIFIER_TOKEN);
    nonws_token();

    decl->last_decl = parse_decl_block();
    decl->header = (NodeHeader) {STRUCT_DECL, start, token};
    return decl;
}

static ReturnStatement *parse_return() {
    Token *start = nonws_token();
    skip_token(KEYWORD_TOKEN);

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

        if (is_next_skipws(EQUAL_TOKEN)) {
            decl->assign = parse_assign();
        } else {
            skip_token(IDENTIFIER_TOKEN);
        }
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
        literal->header = (NodeHeader) {STRING_LITERAL, token, token->next};
        literal->str = nonws_token();

        next_token();

        return (NodeHeader *) literal;
    } else if (nonws_token()->type == NUM_LITERAL_TOKEN) {
        IntLiteral *literal = pool_alloc_struct(IntLiteral);
        literal->header = (NodeHeader) {INT_LITERAL, token, token->next};
        literal->num = nonws_token();

        next_token();

        return (NodeHeader *) literal;
    } else if (token->type == OPEN_CURLY_TOKEN) {
        Token *start = token;
        StructInit *init = pool_alloc_struct(StructInit);
        init->last_expr = parse_expr_list(OPEN_CURLY_TOKEN, CLOSE_CURLY_TOKEN);
        init->header = (NodeHeader){STRUCT_INIT, start, token};

        return (NodeHeader *) init;
    } else if (nonws_token()->type == IDENTIFIER_TOKEN) {
        NodeHeader *def_expr;

        NodeHeader *node;
        if ((def_expr = prep_define_get(define_table, token->span)) != NULL) {
            DefineReference *ref = pool_alloc_struct(DefineReference);
            ref->header = (NodeHeader) {DEFINE_REFERENCE, token, token->next};
            ref->expr = def_expr;
            node = (NodeHeader *) ref;
            next_token();
        } else if (is_next_skipws(OPEN_BRACKET_TOKEN)) {
            ArrAccess *access = pool_alloc_struct(ArrAccess);
            access->id = token;
            next_token();
            skip_token(OPEN_BRACKET_TOKEN);
            nonws_token();
            access->index_expr = parse_expr();
            nonws_token();
            skip_token(CLOSE_BRACKET_TOKEN);
            access->header = (NodeHeader) {ARRAY_ACCESS, access->id, token};
            node = (NodeHeader *) access;
        } else {
            VarReference *ref = pool_alloc_struct(VarReference);
            ref->header = (NodeHeader) {VAR_REFERENCE, token, token->next};
            ref->id = nonws_token();
            node = (NodeHeader *) ref;
            next_token();
        }

        return node;
    }

    assert(0);
}

static NodeHeader *parse_expr() {
    Token *start = nonws_token();

    if (binsearchi(nonws_token()->type, (int *) unary_operations, UNARY_OP_COUNT) >= 0) {
        UnaryOp *op = pool_alloc_struct(UnaryOp);
        start = nonws_token();
        next_token();
        nonws_token();
        op->expr = parse_expr();
        op->header = (NodeHeader) {UNARY_OP, start, token};
        return (NodeHeader *) op;
    }

    NodeHeader *lhs = parse_expr_lazy();
    Token *after_expr = token;

    if (nonws_token()->type == ARROW_TOKEN) {
        ArrowOp *op = pool_alloc_struct(ArrowOp);
        op->lhs = lhs;
        nonws_token();
        skip_token(ARROW_TOKEN);
        op->member = nonws_token();
        next_token();
        op->header = (NodeHeader) {ARROW_OP, start, token};
        return (NodeHeader *) op;
    } else if (binsearchi(nonws_token()->type, (int *) binary_operations, BINARY_OP_COUNT) >= 0) {
        next_token();
        BinaryOp *op = pool_alloc_struct(BinaryOp);
        op->lhs = lhs;
        op->rhs = parse_expr_lazy();
        op->header = (NodeHeader) {BINARY_OP, start, token};
        return (NodeHeader *) op;
    } else {
        token = after_expr;
        return lhs;
    }
}

static NodeHeader *parse_expr_list(TokenType open_token_type, TokenType close_token_type) {
    skip_token(open_token_type);

    NodeHeader *stub = pool_alloc_struct(NodeHeader);
    stub->type = STUB;
    stub->start_token = nonws_token();
    stub->end_token = nonws_token();

    NodeHeader *expr, *prev;
    expr = prev = stub;

    while (nonws_token()->type != close_token_type) {
        expr = parse_expr();
        prev->next = expr;
        if (nonws_token()->type == COMMA_TOKEN) skip_token(COMMA_TOKEN);

        prev = expr;
    }

    skip_token(close_token_type);
    expr->next = stub;

    return expr;
}

static Declaration *parse_decl_block() {
    skip_token(OPEN_CURLY_TOKEN);

    Declaration *stub = pool_alloc_struct(Declaration);
    stub->header = (NodeHeader) {STUB, nonws_token(), nonws_token()};

    Declaration *decl, *prev;
    decl = prev = stub;

    while (nonws_token()->type != CLOSE_CURLY_TOKEN) {
        decl = parse_decl();
        prev->header.next = (NodeHeader *) decl;
        if (nonws_token()->type == SEMICOLON_TOKEN) skip_token(SEMICOLON_TOKEN);

        prev = decl;
    }

    skip_token(CLOSE_CURLY_TOKEN);
    decl->header.next = (NodeHeader *) stub;

    return decl;
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

static int binsearch_parser(Span target, KeywordParser *arr, size_t size) {
    int low = 0;
    int high = size - 1;
    int comp;

    while (low <= high) {
        int mid = (low + high) / 2;

        if ((comp = spanstrcmp(target, arr[mid].keyword)) < 0) {
            high = mid - 1;
        } else if (comp > 0) {
            low = mid + 1;
        } else {
            return mid;
        }
    }

    return -1;
}