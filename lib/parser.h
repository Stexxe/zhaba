#include "lexer.h"

#ifndef ZHABA_PARSER_H
#define ZHABA_PARSER_H

typedef enum {
    UNKNOWN_NODE,
    INCLUDE_DIRECTIVE,
    DEFINE_DIRECTIVE,
    FUNC_DEF, FUNC_DECL,
    FUNC_INVOKE,
    STRING_LITERAL, INT_LITERAL,
    LINE_COMMENT, MULTI_COMMENT,
    DECLARATION, ASSIGNMENT,
    VAR_REFERENCE, DEFINE_REFERENCE,
    ARRAY_ACCESS,
    STRUCT_INIT,
    UNARY_OP,
    BINARY_OP,
    ARROW_OP,
    LABEL_DECL,
    STRUCT_DECL,
    IF_STATEMENT, GOTO_STATEMENT,
    STUB,
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
    UNKNOWN_DATA_TYPE_KIND,
    DATA_TYPE_STRUCT,
    DATA_TYPE_PRIMITIVE,
    DATA_TYPE_TYPEDEF
} DataTypeKind;

typedef enum {
    UNKNOWN_PRIMITIVE_TYPE,
    CHAR_TYPE, INT_TYPE, SHORT_TYPE, LONG_TYPE, FLOAT_TYPE, DOUBLE_TYPE, VOID_TYPE,
    PRIMITIVE_TYPE_COUNT
} PrimitiveDataType;

typedef enum {
    NO_POINTER_TYPE,
    POINTER_TYPE, POINTER_TO_POINTER_TYPE,
} PointerType;

typedef struct {
    DataTypeKind kind;
    PrimitiveDataType primitive;
    PointerType pointer;
    Token *typedef_id;
    Token *struct_id;
    Token *start_token;
    Token *end_token;
} DataType;

typedef struct {
    NodeHeader header;
    Token *varname;
    NodeHeader *expr;
    Token *equal_sign;
} Assignment;

typedef struct {
    NodeHeader header;
    bool var_arg;
    DataType *data_type;
    Token *id;
    Assignment *assign;
} Declaration;

typedef struct {
    NodeHeader header;
    NodeHeader *last_expr;
} StructInit;

typedef struct {
    Token *name;
    DataType *return_type;
    Declaration *last_param;
} FuncSignature;

typedef struct {
    NodeHeader header;
    FuncSignature *signature;
    NodeHeader *last_stmt;
} FuncDef;

typedef enum {
    IncludeHeaderType, IncludePathType
} IncludeType;

typedef struct {
    NodeHeader header;
    IncludeType include_type;
    Token *pathOrHeader;
} Include;

typedef struct {
    NodeHeader header;
    Token *id;
    NodeHeader *expr;
} Define;

typedef struct {
    NodeHeader header;
    Token *name;
    NodeHeader *last_arg;
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
    Token *id;
    Declaration *last_decl;
} StructDecl;

typedef struct {
    NodeHeader header;
    NodeHeader *expr;
} ReturnStatement;

typedef struct {
    NodeHeader header;
    Token *label;
} LabelDecl;

typedef struct {
    NodeHeader header;
    Token *label;
} GotoStatement;

typedef struct {
    NodeHeader header;
    Token *id;
} VarReference;

typedef struct {
    NodeHeader header;
    Token *id;
    NodeHeader *index_expr;
} ArrAccess;

typedef struct {
    NodeHeader header;
    NodeHeader *expr;
} DefineReference;

typedef struct {
    NodeHeader header;
    NodeHeader *expr;
} UnaryOp;

typedef struct {
    NodeHeader header;
    NodeHeader *lhs;
    NodeHeader *rhs;
} BinaryOp;

typedef struct {
    NodeHeader header;
    NodeHeader *lhs;
    Token *member;
} ArrowOp;

typedef struct {
    NodeHeader header;
    NodeHeader *cond;
    NodeHeader *then_statement;
    NodeHeader *else_statement;
    Token *else_token;
} IfStatement;

typedef struct FuncArgument FuncArgument;

NodeHeader *parse(Token *);

#endif //ZHABA_PARSER_H