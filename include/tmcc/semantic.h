#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <tmcc/ast.h>

typedef struct
{
    ast_node_t *root;
} semantic_state_t;

bool semantic_init(semantic_state_t *semantic);

bool semantic_run(semantic_state_t *semantic, ast_node_t *ast);

void semantic_free(semantic_state_t *semantic);

#endif