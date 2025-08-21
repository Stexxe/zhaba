#include "parser.h"
#include <assert.h>

static char *primitive_types[] = {
    "double", "char", "float", "int", "long", "short"
};

#define PRIMITIVE_COUNT (sizeof(primitive_types) / sizeof(primitive_types[0]))

// static Element astout[1024];
//
// #define MAX_ELEMENTS 128
//
// static DataType data_types[MAX_ELEMENTS];
// static DataType *data_type_p = data_types;
//
// static FuncSignature func_signatures[MAX_ELEMENTS];
// static FuncSignature *func_signature_p = func_signatures;
//
// static FuncDef func_defs[MAX_ELEMENTS];
// static FuncDef *func_def_p = func_defs;
//
// static Statement statements[MAX_ELEMENTS];
// static Statement *statement_p = statements;
//
// static FuncArgument arguments[MAX_ELEMENTS];
// static FuncArgument *argument_p = arguments;

static void skip_white();
static FuncSignature *parse_func_signature();
static DataType *parse_data_type();
static void skip_token(TokenType token_type);
static ElementHeader *parse_func_body();

static Token *token;
static ElementHeader *first_element = NULL, *element = NULL;

// static void next_token() {
//     token = token->next;
//     assert(token != NULL);
// }

// static void insert_element(ElementType type, Token *start, Token *end, void *data) {
//     ElementHeader *prev = element;
//
//     element = pool_alloc(sizeof(Element), Element);
//     element->type = type;
//     element->start_token = start;
//     element->end_token = end;
//
//     if (start_element == NULL) start_element = element;
//     else prev->next = element;
// }

static void insert(ElementHeader *el) {
    ElementHeader *prev = element;

    element = el;

    if (first_element == NULL) first_element = el;
    else prev->next = element;
}

ElementHeader *parse(Token *first_token) {
    // Element *start_element = prev_element = pool_alloc(sizeof(Element), Element);
    // prev_element->type = UNKNOWN_ELEMENT;
    // prev_element->next = prev_element;

    Token *start_token, *end_token;

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
                    Include->header = (ElementHeader) {INCLUDE_HEADER, start_token, token};
                    Include->name = header;
                    insert((ElementHeader *)Include);

                    // insert_element(INCLUDE_HEADER, start_token, token, header);
                }
            } break;
            case WHITESPACE_TOKEN: {
                token = token->next;
            } break;
            case KEYWORD_TOKEN: {
                FuncSignature *sign = parse_func_signature();
                skip_white();

                if (token->type == SEMICOLON_TOKEN) { // Func declaration

                } else if (token->type == OPEN_CURLY_TOKEN) { // Func definition
                    ElementHeader *first_statement = parse_func_body();
                    // FuncDef *def = pool_alloc(sizeof(FuncDef), FuncDef);
                    // *func_def_p++ = (FuncDef){.signature = sign, .first_statement = first_statement};
                }
            }
            default: {
                assert(0);
            } break;
        }
    }

    // Token *tokenp = tokens;
    // Element *astp = astout;


    // while (tokenp->type != EOF_TOKEN) {
    //     switch (tokenp->type) {
    //         case INCLUDE_HEADER: {
    //             start_token = tokenp;
    //             if ((++tokenp)->type == WHITESPACE_TOKEN) {
    //                 tokenp++;
    //             }
    //
    //             if (tokenp->type == HEADER_NAME_TOKEN) {
    //                 Token *header = tokenp;
    //                 *astp++ = (Element){.type = INCLUDE_HEADER, .start_token = start_token, .end_token = ++tokenp, .data = header};
    //             }
    //         } break;
    //         case WHITESPACE_TOKEN: {
    //             tokenp++;
    //         } break;
    //         case KEYWORD_TOKEN: {
    //             FuncSignature *sign = parse_func_signature(&tokenp);
    //             skip_white(&tokenp);
    //
    //             if (tokenp->type == SEMICOLON_TOKEN) { // Func declaration
    //
    //             } else if (tokenp->type == OPEN_CURLY_TOKEN) { // Func definition
    //                 Statement *first_statement = parse_func_body(&tokenp);
    //                 *func_def_p++ = (FuncDef){.signature = sign, .first_statement = first_statement};
    //             }
    //
    //
    //             // if (binsearch(tokenp->span, primitive_types, PRIMITIVE_COUNT) >= 0) {
    //             //     start_token = tokenp;
    //             //     DataType *return_type = parse_data_type(&tokenp);
    //             //     skip_white(&tokenp);
    //             //
    //             //     Token *name = tokenp;
    //             //     if (tokenp->type == IDENTIFIER_TOKEN) {
    //             //         tokenp++;
    //             //     }
    //             //
    //             //     skip_white(&tokenp);
    //             //
    //             //     if (tokenp->type == OPEN_PAREN_TOKEN) { // Function declaration
    //             //         skip_token(&tokenp, OPEN_PAREN_TOKEN);
    //             //         skip_white(&tokenp);
    //             //         skip_token(&tokenp, CLOSE_PAREN_TOKEN);
    //             //         skip_white(&tokenp);
    //             //
    //             //         if (tokenp->type == OPEN_CURLY_TOKEN) { // Function definition
    //             //             FuncDef def = {.name = name, .data_type = return_type};
    //             //             parse_func_body(&tokenp, astp);
    //             //             *astp = (Grammar){.type = FUNC_DEF, .start_token = start_token, .end_token = tokenp, .data = header}
    //             //         }
    //             //     }
    //             // }
    //         } break;
    //         default: {
    //             assert(false);
    //         } break;
    //     }
    // }

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
    if (spanstrcmp(token->span, "int") >= 0) { // TODO: Lookup
        data_type->primitive = INT_TYPE;
        // *data_type_p = (DataType) {.primitive = INT_TYPE};
    }

    token = token->next;
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
    signature->data_type = type;
    return signature;
}

static ElementHeader *parse_func_body() {
    skip_token(OPEN_CURLY_TOKEN);
    skip_white();

    // TODO: Parse
}

// Statement *parse_func_body() {
//     skip_token(tokenp, OPEN_CURLY_TOKEN);
//     skip_white(tokenp);
//     Statement *prev, *st, *first;
//
//     first = prev = parse_statement(tokenp);
//     skip_token(tokenp, SEMICOLON_TOKEN);
//     skip_white(tokenp);
//
//     while ((*tokenp)->type != CLOSE_CURLY_TOKEN) {
//         st = parse_statement(tokenp);
//         prev->next = st;
//         skip_token(tokenp, SEMICOLON_TOKEN);
//         skip_white(tokenp);
//         prev = st;
//     }
//
//     return first;
// }
//
// Statement *parse_statement(Token **tokenp) {
//     if ((*tokenp)->type == IDENTIFIER_TOKEN) {
//         Expression *expr = parse_expression(tokenp);
//
//         // *tokenp += 1;
//         //
//         // skip_white(tokenp);
//         //
//         // if ((*tokenp)->type == OPEN_PAREN_TOKEN) {
//         //
//         // }
//     }
// }
//
// Expression *parse_expression(Token **tokenp) {
//     if ((*tokenp)->type == IDENTIFIER_TOKEN) {
//         *tokenp += 1;
//         skip_white(tokenp);
//
//         if ((*tokenp)->type == OPEN_PAREN_TOKEN) {
//             // TODO: Parse func arguments
//         }
//     }
// }
