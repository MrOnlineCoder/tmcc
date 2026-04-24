#ifndef CODEGEN_H
#define CODEGEN_H

#include <tmcc/ast.h>
#include <string.h>

#define CODEGEN_MAX_OUTPUT 8192

typedef struct
{
    char output[CODEGEN_MAX_OUTPUT];
    size_t output_length;

    ast_node_t *ast;

    ast_node_t *current_function;
} codegen_state_t;

bool codegen_init(codegen_state_t *cg);

bool codegen_run(codegen_state_t *cg, ast_node_t *ast);

#endif