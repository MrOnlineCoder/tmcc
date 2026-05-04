#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <tmcc/symbol.h>
#include <tmcc/tokens.h>
#include <tmcc/symtable.h>

#define AST_MAX_CHILDREN 64

typedef enum
{
    AST_INVALID,

    AST_PROGRAM,

    AST_FUNCTION_DEFINITION,

    AST_COMPOUND_STATEMENT,

    AST_FUNCTION_SIGNATURE,

    AST_RETURN_STATEMENT,

    AST_EXPRESSION,

    AST_INTEGER_LITERAL,

    AST_BINARY_OPERATOR,

    AST_UNARY_OPERATOR,

    AST_ASSIGN_STATEMENT,

    AST_IF_STATEMENT,

    AST_WHILE_STATEMENT,

    AST_DECLARATION,

    AST_VARIABLE
} ast_node_type_t;

typedef enum
{
    AST_BIN_OP_INVALID,

    AST_BIN_OP_ADD,
    AST_BIN_OP_SUB,
    AST_BIN_OP_MUL,
    AST_BIN_OP_DIV,

    AST_BIN_OP_EQ,
    AST_BIN_OP_NEQ,

    AST_BIN_OP_LT,
    AST_BIN_OP_GT,
    AST_BIN_OP_LTE,
    AST_BIN_OP_GTE,

    AST_BIN_OP_AND,
    AST_BIN_OP_OR,

    AST_BIN_OP_SHL,
    AST_BIN_OP_SHR,
    AST_BIN_OP_MOD,

    AST_BIN_OP_BITAND,
    AST_BIN_OP_BITOR,
    AST_BIN_OP_BITXOR,
} ast_bin_op_type_t;

typedef enum
{
    AST_UNARY_OP_INVALID,
    AST_UNARY_OP_BITNOT,
    AST_UNARY_OP_LOGNOT,
    AST_UNARY_OP_PLUS,
    AST_UNARY_OP_MINUS,
    AST_UNARY_OP_ADDR,
    AST_UNARY_OP_DEREF,
    AST_UNARY_OP_PREINC,
    AST_UNARY_OP_PREDEC,
    AST_UNARY_OP_SIZEOF,
} ast_unary_op_type_t;

typedef struct
{
    // storage class
    bool is_static;
    bool is_extern;
    bool is_typedef;

    // type specifier
    ctype_t *type;

    // type qualifiers
    bool is_volatile;
    bool is_const;
} declspec_t;

struct ast_node_s
{
    ast_node_type_t type;

    int children_count;
    struct ast_node_s *children[AST_MAX_CHILDREN];

    const token_t *start_token;

    const ctype_t *expr_type;

    union
    {
        struct
        {
            bool is_static;
            bool is_inline;
            const char *name;

            symbol_table_t *params;
            symbol_table_t *locals;

            const ctype_t *return_type;
        } function;

        struct
        {
            int value;
        } integer_literal;

        struct
        {
            ast_bin_op_type_t op_type;
        } binary_op;

        struct
        {
            ast_unary_op_type_t op_type;
        } unary_op;

        struct
        {
            symbol_t *sym;
        } variable;

        struct
        {
            declspec_t ds;
            const char *id;
        } declaration;
    } meta;
};

typedef struct ast_node_s ast_node_t;

ast_node_t *ast_make_node(ast_node_type_t type, const token_t *start_token);

void ast_add_child(ast_node_t *parent, ast_node_t *child);

void ast_dump(ast_node_t *node, int indent);

void ast_free_deep(ast_node_t *node);

void ast_free(ast_node_t *node);

const char *bin_op_to_string(ast_bin_op_type_t op_type);
const char *unary_op_to_string(ast_unary_op_type_t op_type);

#endif