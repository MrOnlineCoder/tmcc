#include <tmcc/semantic.h>

bool semantic_init(semantic_state_t *semantic)
{
    semantic->root = NULL;
    return true;
}

bool semantic_run(semantic_state_t *semantic, ast_node_t *ast)
{
    if (!ast)
        return false;

    semantic->root = ast;

    return true;
}

void semantic_free(semantic_state_t *semantic)
{
    if (semantic->root)
    {
        semantic->root = NULL;
    }
}