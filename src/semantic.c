#include <tmcc/semantic.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void semantic_error(semantic_state_t *semantic, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(semantic->err, sizeof(semantic->err), format, args);
    va_end(args);
}

static void semantic_debug(semantic_state_t *semantic, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

static semantic_scope_t *semantic_enter_scope(semantic_state_t *semantic)
{
    semantic_scope_t *new_scope = malloc(sizeof(semantic_scope_t));
    if (!new_scope)
    {
        semantic_error(semantic, "failed to allocate memory for new scope");
        return NULL;
    }

    symtable_init(&new_scope->symtable);
    new_scope->depth = semantic->current_scope ? semantic->current_scope->depth + 1 : 0;
    new_scope->parent = semantic->current_scope;

    semantic->current_scope = new_scope;

    semantic_debug(semantic, "entered new scope at depth %zu", new_scope->depth);

    return new_scope;
}

static semantic_scope_t *semantic_exit_scope(semantic_state_t *semantic)
{
    if (!semantic->current_scope)
    {
        semantic_error(semantic, "cannot exit scope: no current scope");
        return NULL;
    }

    if (!semantic->current_scope->parent)
    {
        semantic_error(semantic, "cannot exit global scope");
        return NULL;
    }

    semantic_scope_t *exiting_scope = semantic->current_scope;
    semantic->current_scope = exiting_scope->parent;

    semantic_debug(semantic, "exited scope at depth %zu, now at depth %zu",
                   exiting_scope->depth, semantic->current_scope->depth);

    return exiting_scope;
}

static symbol_t *semantic_lookup(semantic_state_t *semantic, const char *name)
{
    semantic_scope_t *scope = semantic->current_scope;

    while (scope)
    {
        symbol_t *sym = symtable_lookup(&scope->symtable, name);
        if (sym)
        {
            return sym;
        }
        scope = scope->parent;
    }

    return NULL;
}

static char *extract_token_as_new_string(const token_t *token)
{
    return strndup(token->start, token->length);
}

/*------*/
static void analyze_binary_operator(semantic_state_t *semantic, ast_node_t *node);
static void analyze_expression(semantic_state_t *semantic, ast_node_t *node);
static void analyze_variable(semantic_state_t *semantic, ast_node_t *node);
/*------*/

static bool is_valid_lvalue(ast_node_t *node)
{
    return node->type == AST_VARIABLE;
}

static void analyze_declaration(semantic_state_t *semantic, ast_node_t *node)
{
    symbol_t *sym = symbol_new(
        extract_token_as_new_string(node->meta.declaration.id),
        node->meta.declaration.type,
        SCOPE_LOCAL,
        0);

    symtable_add(&semantic->current_scope->symtable, sym);

    semantic_debug(semantic, "declared variable '%s' of type %d in scope depth %zu",
                   sym->name, sym->type->kind, semantic->current_scope->depth);
}

static void analyze_binary_operator(semantic_state_t *semantic, ast_node_t *node)
{
    ast_node_t *left = node->children[0];
    ast_node_t *right = node->children[1];

    analyze_expression(semantic, left);
    analyze_expression(semantic, right);
}

static void analyze_expression(semantic_state_t *semantic, ast_node_t *node)
{
    if (node->type == AST_BINARY_OPERATOR)
    {
        analyze_binary_operator(semantic, node);
    }
    else if (node->type == AST_VARIABLE)
    {
        analyze_variable(semantic, node);
        node->expr_type = node->meta.variable.sym ? node->meta.variable.sym->type : NULL;
    }
    else if (node->type == AST_INTEGER_LITERAL)
    {
        node->expr_type = &CTYPE_BUILTIN_INT;
    }
}

static void analyze_return_statement(semantic_state_t *semantic, ast_node_t *node)
{
    if (node->children_count > 0)
    {
        analyze_expression(semantic, node->children[0]);
    }
}

static void analyze_assign_statement(semantic_state_t *semantic, ast_node_t *node)
{
    if (node->children_count != 2)
    {
        semantic_error(semantic, "invalid assign statement: expected 2 children, got %d", node->children_count);
        return;
    }

    ast_node_t *lhs = node->children[0];
    ast_node_t *rhs = node->children[1];

    if (!is_valid_lvalue(lhs))
    {
        semantic_error(semantic, "invalid lvalue in assignment at line %zu:%zu", lhs->start_token->line, lhs->start_token->column);
        return;
    }

    analyze_expression(semantic, rhs);
}

static void analyze_variable(semantic_state_t *semantic, ast_node_t *node)
{
    static char var_name[MAX_SYMBOL_NAME_LENGTH];

    strncpy(var_name, node->start_token->start, node->start_token->length);
    var_name[node->start_token->length] = '\0';

    symbol_t *sym = semantic_lookup(semantic, var_name);

    if (!sym)
    {
        semantic_error(semantic, "undefined variable '%s' at line %zu:%zu", var_name, node->start_token->line, node->start_token->column);
        return;
    }

    node->meta.variable.sym = sym;

    semantic_debug(semantic, "got variable '%s' of type %d from scope depth %zu at line %zu:%zu",
                   sym->name, sym->type->kind, semantic->current_scope->depth,
                   node->start_token->line, node->start_token->column);
}

static void analyze_compound_statement(semantic_state_t *semantic, ast_node_t *node)
{
    for (int i = 0; i < node->children_count; i++)
    {
        ast_node_t *child = node->children[i];

        if (child->type == AST_DECLARATION)
        {
            analyze_declaration(semantic, child);
        }
        else if (child->type == AST_RETURN_STATEMENT)
        {
            analyze_return_statement(semantic, child);
        }
        else if (child->type == AST_COMPOUND_STATEMENT)
        {
            semantic_enter_scope(semantic);
            analyze_compound_statement(semantic, child);
            semantic_exit_scope(semantic);
        }
    }
}

static void analyze_function_definition(semantic_state_t *semantic, ast_node_t *node)
{
    ast_node_t *signature = node->children[0];
    ast_node_t *body = node->children[1];

    semantic_enter_scope(semantic);

    if (body->type == AST_COMPOUND_STATEMENT)
    {
        analyze_compound_statement(semantic, body);
    }

    semantic_exit_scope(semantic);
}

static void analyze_program(semantic_state_t *semantic, ast_node_t *node)
{
    analyze_function_definition(semantic, node->children[0]);
}

bool semantic_init(semantic_state_t *semantic)
{
    semantic->root = NULL;
    semantic->err[0] = '\0';
    semantic->current_scope = &semantic->global_scope;
    semantic->global_scope = (semantic_scope_t){0};
    semantic->global_scope.depth = 0;
    semantic->global_scope.parent = NULL;
    symtable_init(&semantic->global_scope.symtable);
    return true;
}

bool semantic_run(semantic_state_t *semantic, ast_node_t *ast)
{
    if (!ast)
        return false;

    semantic->root = ast;
    semantic->err[0] = '\0';

    if (!semantic->root || semantic->root->type != AST_PROGRAM)
    {
        semantic_error(semantic, "expected AST_PROGRAM as root node");
        return false;
    }

    analyze_program(semantic, semantic->root);

    return semantic->err[0] == '\0';
}

void semantic_free(semantic_state_t *semantic)
{
    if (semantic->root)
    {
        semantic->root = NULL;
    }
}