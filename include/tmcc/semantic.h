#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <tmcc/ast.h>

#include <tmcc/symtable.h>

typedef struct semantic_scope_s semantic_scope_t;
typedef struct semantic_scope_s
{
    symbol_table_t symtable;
    int depth;

    semantic_scope_t *parent;
} semantic_scope_t;

typedef struct
{
    ast_node_t *root;

    symbol_table_t *locals;

    semantic_scope_t *current_scope;
    semantic_scope_t global_scope;

    char err[256];
} semantic_state_t;

bool semantic_init(semantic_state_t *semantic);

bool semantic_run(semantic_state_t *semantic, ast_node_t *ast);

void semantic_free(semantic_state_t *semantic);

#endif