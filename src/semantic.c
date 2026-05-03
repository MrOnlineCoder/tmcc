#include <tmcc/semantic.h>
#include <tmcc/tokens.h>
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
    free(exiting_scope);

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

/*------*/
static void analyze_binary_operator(semantic_state_t *semantic, ast_node_t *node);
static void analyze_expression(semantic_state_t *semantic, ast_node_t *node);
static void analyze_variable(semantic_state_t *semantic, ast_node_t *node);
static void analyze_compound_statement(semantic_state_t *semantic, ast_node_t *node, bool needs_new_scope);
/*------*/

static bool is_valid_lvalue(ast_node_t *node)
{
    return node->type == AST_VARIABLE;
}

static void analyze_declaration(semantic_state_t *semantic, ast_node_t *node)
{
    // const char *name = extract_token_as_new_string(node->meta.declaration.id);
    const char *name = node->meta.declaration.id;

    symbol_t *existing_sym = symtable_lookup(&semantic->current_scope->symtable, name);

    if (existing_sym)
    {
        semantic_error(semantic, "redeclaration of variable '%s' at line %zu:%zu", name, node->start_token->line, node->start_token->column);
        free((char *)name);
        return;
    }

    if (node->meta.declaration.ds.type->kind == CTYPE_KIND_VOID)
    {
        semantic_error(semantic, "variable '%s' cannot be of type void at line %zu:%zu", name, node->start_token->line, node->start_token->column);
        free((char *)name);
        return;
    }

    symbol_scope_t ss = semantic->current_scope == &semantic->global_scope ? SCOPE_GLOBAL : SCOPE_LOCAL;

    symbol_t *sym = symbol_new(
        name,
        node->meta.declaration.ds.type,
        ss,
        0);

    symtable_add(&semantic->current_scope->symtable, sym);

    if (ss == SCOPE_LOCAL && semantic->locals)
    {
        symtable_add(semantic->locals, sym);
    }

    semantic_debug(semantic, "declared variable '%s' of type %d in scope depth %zu",
                   sym->name, sym->type->kind, semantic->current_scope->depth);
}

static void analyze_binary_operator(semantic_state_t *semantic, ast_node_t *node)
{
    ast_node_t *left = node->children[0];
    ast_node_t *right = node->children[1];

    analyze_expression(semantic, left);
    analyze_expression(semantic, right);

    node->expr_type = ctype_binary_result_type(
        left->expr_type, right->expr_type);
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
        ast_node_t *expr = node->children[0];
        analyze_expression(semantic, expr);

        node->expr_type = expr->expr_type;
    }
    else
    {
        node->expr_type = &CTYPE_BUILTIN_VOID;
    }
}

static void analyze_if_statement(semantic_state_t *semantic, ast_node_t *node)
{
    ast_node_t *condition = node->children[0];
    ast_node_t *then_branch = node->children[1];
    ast_node_t *else_branch = node->children_count > 2 ? node->children[2] : NULL;

    analyze_expression(semantic, condition);

    // TODO: check that condition is of integer type or boolean-like

    analyze_compound_statement(semantic, then_branch, true);

    if (else_branch)
    {
        analyze_compound_statement(semantic, else_branch, true);
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

    analyze_variable(semantic, lhs);

    analyze_expression(semantic, rhs);

    node->expr_type = rhs->expr_type;
}

static void analyze_while_statement(semantic_state_t *semantic, ast_node_t *node)
{
    ast_node_t *condition = node->children[0];
    ast_node_t *body = node->children[1];

    analyze_expression(semantic, condition);

    analyze_compound_statement(semantic, body, true);
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

static void analyze_compound_statement(semantic_state_t *semantic, ast_node_t *node, bool needs_new_scope)
{
    if (needs_new_scope)
    {
        semantic_enter_scope(semantic);
    }

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
            analyze_compound_statement(semantic, child, true);
        }
        else if (child->type == AST_ASSIGN_STATEMENT)
        {
            analyze_assign_statement(semantic, child);
        }
        else if (child->type == AST_IF_STATEMENT)
        {
            analyze_if_statement(semantic, child);
        }
        else if (child->type == AST_WHILE_STATEMENT)
        {
            analyze_while_statement(semantic, child);
        }
        else
        {
            semantic_error(semantic, "unexpected node type %d in compound statement at line %zu:%zu", child->type, child->start_token->line, child->start_token->column);
        }
    }

    if (needs_new_scope)
    {
        semantic_exit_scope(semantic);
    }
}

static void analyze_function_definition(semantic_state_t *semantic, ast_node_t *node)
{
    ast_node_t *body = node->children[0];

    semantic_enter_scope(semantic);

    semantic->locals = symtable_new();

    if (body->type == AST_COMPOUND_STATEMENT)
    {
        analyze_compound_statement(semantic, body, false);
    }

    node->meta.function.locals = semantic->locals;

    semantic->locals = NULL;

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
    semantic->locals = NULL;
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