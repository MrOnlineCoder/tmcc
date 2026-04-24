#include <tmcc/codegen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <tmcc/symtable.h>
#include <tmcc/symbol.h>

static void emit_code(codegen_state_t *cg, const char *code, ...)
{
    if (cg->output_length >= CODEGEN_MAX_OUTPUT)
    {
        return;
    }

    char buffer[256];

    buffer[0] = '\t';

    va_list args;
    va_start(args, code);
    int written = vsnprintf(buffer + 1, sizeof(buffer) - 1, code, args);
    va_end(args);

    buffer[written + 1] = '\n';

    memcpy(cg->output + cg->output_length, buffer, written + 2);
    cg->output_length += written + 2;
}

static void emit_label(codegen_state_t *cg, const char *code, ...)
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

/* Forward declarations */
static void codegen_expression(codegen_state_t *cg, ast_node_t *node);
static void codegen_address(codegen_state_t *cg, ast_node_t *node);
/* End of forward declarations*/

bool codegen_init(codegen_state_t *cg)
{
    cg->output_length = 0;
    memset(cg->output, 0, sizeof(cg->output));
    cg->ast = NULL;
    cg->current_function = NULL;
    return true;
}

static int calculate_alignment(int n, int align)
{
    return (n + align - 1) / align * align;
}

static void codegen_integer_literal(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_INTEGER_LITERAL)
        return;

    emit_code(cg, "mov rax, %d", node->meta.integer_literal.value);
}

/*
    Every expression leaves its result in rax.
    Temporary intermediate values are saved with push/pop.
    All named local variables live on the stack.
*/
static void codegen_binary_op(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_BINARY_OPERATOR)
        return;

    ast_node_t *lhs = node->children[0];
    ast_node_t *rhs = node->children[1];

    emit_code(cg, "; binary operator '%s'", node->meta.binary_op.op_type == AST_BIN_OP_ADD ? "+" : "-");

    codegen_expression(cg, lhs);
    emit_code(cg, "push rax");

    codegen_expression(cg, rhs); // rax == rhs

    emit_code(cg, "pop rbx"); // rbx == lhs

    if (node->meta.binary_op.op_type == AST_BIN_OP_ADD)
    {
        emit_code(cg, "add rax, rbx");
    }
    else
    {
        emit_code(cg, "sub rax, rbx");
    }
}

static void codegen_expression(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type == AST_INTEGER_LITERAL)
    {
        codegen_integer_literal(cg, node);
    }
    else if (node->type == AST_BINARY_OPERATOR)
    {
        codegen_binary_op(cg, node);
    }
    else if (node->type == AST_VARIABLE)
    {
        codegen_address(cg, node);
        emit_code(cg, "mov rax, [rax]");
    }
}

static void codegen_address(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_VARIABLE)
        return;

    symbol_t *sym = node->meta.variable.sym;

    if (!sym)
        return;

    emit_code(cg, "; address of variable '%s' at stack offset %d", sym->name, sym->stack_offset);
    emit_code(cg, "lea rax, [rbp - %d]", sym->stack_offset);
}

static void codegen_return_statement(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_RETURN_STATEMENT)
        return;

    if (node->children_count > 0)
    {
        ast_node_t *expr = node->children[0];
        codegen_expression(cg, expr);
    }

    emit_code(cg, "jmp .L.return.%s", cg->current_function->meta.function.name);
}

static void codegen_assign_statement(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_ASSIGN_STATEMENT)
        return;

    ast_node_t *lhs = node->children[0];
    ast_node_t *rhs = node->children[1];

    codegen_address(cg, lhs);
    emit_code(cg, "push rax"); // rax == &lhs

    codegen_expression(cg, rhs); // rax == rhs

    emit_code(cg, "pop rbx"); // rbx == &lhs

    emit_code(cg, "mov [rbx], rax");
}

static void codegen_compound_statement(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_COMPOUND_STATEMENT)
        return;

    for (int i = 0; i < node->children_count; i++)
    {
        ast_node_t *stmt = node->children[i];

        if (stmt->type == AST_RETURN_STATEMENT)
        {
            codegen_return_statement(cg, stmt);
        }
        else if (stmt->type == AST_ASSIGN_STATEMENT)
        {
            codegen_assign_statement(cg, stmt);
        }
        else if (stmt->type == AST_COMPOUND_STATEMENT)
        {
            codegen_compound_statement(cg, stmt);
        }
    }
}

static void codegen_function_definition(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_FUNCTION_DEFINITION)
        return;

    int stack_size = 0;
    int locals_offset = 8;

    if (node->meta.function.locals)
    {
        for (size_t i = 0; i < node->meta.function.locals->count; i++)
        {
            symbol_t *sym = node->meta.function.locals->symbols[i];

            if (sym)
            {
                int stack_offset = calculate_alignment(
                    stack_size, sym->type->alignment);

                sym->stack_offset = locals_offset + stack_offset;

                stack_size += stack_offset + sym->type->size;

                printf("symbol '%s' of type %d at stack offset %d\n", sym->name, sym->type->kind, sym->stack_offset);
            }
        }
    }

    cg->current_function = node;

    emit_label(cg, "; function '%s', %d params, %d local stack size",
               node->meta.function.name, node->meta.function.params ? node->meta.function.params->count : 0, stack_size);

    emit_label(cg, "%s:", node->meta.function.name);

    if (stack_size > 0)
    {
        emit_code(cg, "; local stack size: %d bytes", stack_size);
        emit_code(cg, "push rbp");
        emit_code(cg, "mov rbp, rsp");
        emit_code(cg, "sub rsp, %d", stack_size);
        emit_label(cg, "");
    }

    ast_node_t *body = node->children[0];

    if (body->type == AST_COMPOUND_STATEMENT)
    {
        codegen_compound_statement(cg, body);
    }

    emit_label(cg, "; function '%s' epilogue", node->meta.function.name);

    emit_label(cg, ".L.return.%s:", node->meta.function.name);
    emit_code(cg, "mov rsp, rbp");
    emit_code(cg, "pop rbp");
    emit_code(cg, "ret");
    emit_label(cg, "");

    cg->current_function = NULL;
}

static void codegen_program(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_PROGRAM)
        return;

    emit_label(cg, "global _main");

    emit_label(cg, "section .text");

    emit_label(cg, "");

    emit_label(cg, "_main:");
    emit_code(cg, "call main");
    // emit_code(cg, "mov rax, 0");
    emit_code(cg, "ret");
    emit_label(cg, "");

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