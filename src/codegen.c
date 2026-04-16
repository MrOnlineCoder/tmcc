#include <tmcc/codegen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

static void emit_code(codegen_state_t *cg, const char *code, ...)
{
    if (cg->output_length >= CODEGEN_MAX_OUTPUT)
    {
        return;
    }

    char buffer[256];

    va_list args;
    va_start(args, code);
    int written = vsnprintf(buffer, sizeof(buffer), code, args);
    va_end(args);

    buffer[written] = '\n';

    memcpy(cg->output + cg->output_length, buffer, written + 1);
    cg->output_length += written + 1;
}

bool codegen_init(codegen_state_t *cg)
{
    cg->output_length = 0;
    memset(cg->output, 0, sizeof(cg->output));
    cg->ast = NULL;
    return true;
}

static void codegen_integer_literal(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_INTEGER_LITERAL)
        return;

    emit_code(cg, "mov rax, %d", node->meta.integer_literal.value);
}

static void codegen_compound_statement(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_COMPOUND_STATEMENT)
        return;

    for (int i = 0; i < node->children_count; i++)
    {
        ast_node_t *child = node->children[i];

        if (child->type == AST_RETURN_STATEMENT)
        {
            codegen_integer_literal(cg, child->children[0]);
            emit_code(cg, "ret");
        }
    }
}

static void codegen_function_definition(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_FUNCTION_DEFINITION)
        return;

    ast_node_t *body = node->children[1];

    if (body->type == AST_COMPOUND_STATEMENT)
    {
        codegen_compound_statement(cg, body);
    }
}

static void codegen_program(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_PROGRAM)
        return;

    emit_code(cg, "global _main");

    emit_code(cg, "section .text");

    emit_code(cg, "");

    emit_code(cg, "_main:");

    for (int i = 0; i < node->children_count; i++)
    {
        ast_node_t *child = node->children[i];

        if (child->type == AST_FUNCTION_DEFINITION)
        {
            codegen_function_definition(cg, child);
        }
    }
}

bool codegen_run(codegen_state_t *cg, ast_node_t *ast)
{
    if (!ast)
    {
        return false;
    }

    cg->ast = ast;

    codegen_program(cg, ast);

    return true;
}