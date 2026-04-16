#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include <tmcc/tokens.h>

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

    AST_BINARY_OPERATOR
} ast_node_type_t;

typedef enum
{
    AST_BIN_OP_ADD,
    AST_BIN_OP_SUB,
    AST_BIN_OP_MUL,
    AST_BIN_OP_DIV,
} ast_bin_op_type_t;

struct ast_node_s
{
    ast_node_type_t type;

    int children_count;
    struct ast_node_s *children[AST_MAX_CHILDREN];

    const token_t *start_token;

    union
    {
        struct
        {
            bool is_static;
            bool is_inline;
            const token_t *name;
        } function_signature;

        struct
        {
            int value;
        } integer_literal;

        struct
        {
            ast_bin_op_type_t op_type;
        } binary_op;
    } meta;
};

typedef struct ast_node_s ast_node_t;

ast_node_t *ast_make_node(ast_node_type_t type, const token_t *start_token);

void ast_add_child(ast_node_t *parent, ast_node_t *child);

void ast_dump(ast_node_t *node, int indent);

void ast_free_deep(ast_node_t *node);

void ast_free(ast_node_t *node);

#endif