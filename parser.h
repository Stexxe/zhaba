#include "lexer.h"

#ifndef ZHABA_PARSER_H
#define ZHABA_PARSER_H

typedef enum {
    UNKNOWN_NODE,
    INCLUDE_HEADER,
    FUNC_DEF, FUNC_DECL,
    FUNC_INVOKE,
    STRING_LITERAL, INT_LITERAL,
    RETURN_STATEMENT,
    STATEMENT,
    NODE_TYPE_COUNT
} NodeType;

struct NodeHeader {
    NodeType type;
    Token *start_token;
    Token *end_token;
    struct NodeHeader *next;
};

typedef struct NodeHeader NodeHeader;

typedef enum {
    UNKNOWN_DATA_TYPE,
    INT_TYPE,
    DATA_TYPE_COUNT
} PrimitiveDataType;

typedef struct {
    PrimitiveDataType primitive;
    Token *start_token;
    Token *end_token;
} DataType;

typedef struct {
    Token *name;
    DataType *return_type;
} FuncSignature;

typedef struct {
    NodeHeader header;
    FuncSignature *signature;
    NodeHeader *first_statement;
} FuncDef;

typedef struct {
    NodeHeader header;
    Token *name;
} IncludeHeaderName;

typedef struct {
    NodeHeader header;
    Token *name;
    NodeHeader *first_arg;
} FuncInvoke;

typedef struct {
    NodeHeader header;
    Token *str;
} StringLiteral;

typedef struct {
    NodeHeader header;
    Token *num;
} IntLiteral;

typedef struct {
    NodeHeader header;
    NodeHeader *expr;
} ReturnStatement;

typedef struct FuncArgument FuncArgument;

NodeHeader *parse(Token *);



#endif //ZHABA_PARSER_H