#include "lexer.h"

#ifndef ZHABA_PARSER_H
#define ZHABA_PARSER_H

typedef enum {
    UNKNOWN_ELEMENT,
    INCLUDE_HEADER,
    FUNC_DEF, FUNC_DECL,
    STATEMENT,
    ELEMENT_TYPE_COUNT
} ElementType;

struct ElementHeader {
    ElementType type;
    Token *start_token;
    Token *end_token;
    struct ElementHeader *next;
};

typedef struct ElementHeader ElementHeader;

typedef struct {
    ElementHeader header;
    Token *name;
} IncludeHeaderName;

// struct Element {
//     ElementType type;
//     Token *start_token;
//     Token *end_token;
//     void *data;
//     void *next;
// };
//
// typedef struct Element Element;
//
// typedef enum {
//     UNKNOWN_STATEMENT,
//     STATEMENT_COUNT,
// } StatementType;
//
//
// typedef struct {
//     StatementType type;
//     Token *start_token;
//     Token *end_token;
//     void *data;
//     void *next;
// } Statement;
//
// typedef enum {
//     UNKNOWN_EXPRESSION,
//     FUNC_INVOKE,
//     EXPRESSION_COUNT,
// } ExpressionType;
//
// typedef struct {
//     ExpressionType type;
//     Token *start_token;
//     Token *end_token;
//     void *data;
// } Expression;
//
typedef enum {
    UNKNOWN_DATA_TYPE,
    INT_TYPE,
    DATA_TYPE_COUNT
} PrimitiveDataType;

typedef struct {
    PrimitiveDataType primitive;
} DataType;

typedef struct {
    Token *name;
    DataType *data_type;
} FuncSignature;
//
// typedef struct {
//     FuncSignature *signature;
//     Statement *first_statement;
// } FuncDef;
//
// struct FuncArgument {
//     Expression *expr;
//     struct FuncArgument *next;
// };

typedef struct FuncArgument FuncArgument;

ElementHeader *parse(Token *);

ElementHeader *parse_statement(Token **);
ElementHeader *parse_expression(Token **);

#endif //ZHABA_PARSER_H