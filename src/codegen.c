#include <tmcc/codegen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <tmcc/symtable.h>
#include <tmcc/symbol.h>
#include <tmcc/types.h>

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

static int next_label(codegen_state_t *cg)
{
    return cg->counter++;
}

/* Forward declarations */
static void codegen_expression(codegen_state_t *cg, ast_node_t *node);
static void codegen_address(codegen_state_t *cg, ast_node_t *node);
static void codegen_compound_statement(codegen_state_t *cg, ast_node_t *node);
/* End of forward declarations*/

bool codegen_init(codegen_state_t *cg)
{
    cg->output_length = 0;
    memset(cg->output, 0, sizeof(cg->output));
    cg->ast = NULL;
    cg->current_function = NULL;
    cg->counter = 1;
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

static void codegen_unary_op(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_UNARY_OPERATOR)
        return;

    ast_node_t *operand = node->children[0];

    switch (node->meta.unary_op.op_type)
    {
    case AST_UNARY_OP_MINUS:
        codegen_expression(cg, operand); // rax == operand
        emit_code(cg, "neg rax");
        break;
    case AST_UNARY_OP_BITNOT:
        codegen_expression(cg, operand); // rax == operand
        emit_code(cg, "not rax");
        break;
    case AST_UNARY_OP_LOGNOT:
        codegen_expression(cg, operand); // rax == operand
        emit_code(cg, "cmp rax, 0");
        emit_code(cg, "sete al");
        emit_code(cg, "movzx rax, al");
        break;
    case AST_UNARY_OP_PREDEC:
    case AST_UNARY_OP_PREINC:
    {
        codegen_address(cg, operand);    // rax == &operand
        emit_code(cg, "mov rbx, [rax]"); // rbx == operand
        if (node->meta.unary_op.op_type == AST_UNARY_OP_PREDEC)
        {
            emit_code(cg, "dec rbx");
        }
        else
        {
            emit_code(cg, "inc rbx");
        }
        emit_code(cg, "mov [rax], rbx");
    }
    break;
    case AST_UNARY_OP_ADDR:
        codegen_address(cg, operand); // rax == &operand
        break;
    case AST_UNARY_OP_DEREF:
        // TODO: implement deref
        break;
    default:
        break;
    }
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

    emit_code(cg, "; binary operator '%s'", bin_op_to_string(node->meta.binary_op.op_type));

    codegen_expression(cg, lhs);
    emit_code(cg, "push rax");

    codegen_expression(cg, rhs); // rax == rhs

    emit_code(cg, "pop rbx"); // rbx == lhs

    ast_bin_op_type_t op_type = node->meta.binary_op.op_type;

    switch (op_type)
    {
    case AST_BIN_OP_ADD:
        emit_code(cg, "add rax, rbx");
        break;
    case AST_BIN_OP_SUB:
        emit_code(cg, "sub rax, rbx");
        break;
    case AST_BIN_OP_MUL:
        emit_code(cg, "imul rax, rbx");
        break;
    case AST_BIN_OP_DIV:
        emit_code(cg, "xor rdx, rdx");
        emit_code(cg, "div rbx");
        break;
    case AST_BIN_OP_EQ:
    case AST_BIN_OP_NEQ:
    case AST_BIN_OP_LT:
    case AST_BIN_OP_GT:
    case AST_BIN_OP_LTE:
    case AST_BIN_OP_GTE:
    {
        emit_code(cg, "cmp rbx, rax");
        if (op_type == AST_BIN_OP_EQ)
        {
            emit_code(cg, "sete al");
        }
        else if (op_type == AST_BIN_OP_NEQ)
        {
            emit_code(cg, "setne al");
        }
        else if (op_type == AST_BIN_OP_LT)
        {
            emit_code(cg, "setl al");
        }
        else if (op_type == AST_BIN_OP_GT)
        {
            emit_code(cg, "setg al");
        }
        else if (op_type == AST_BIN_OP_LTE)
        {
            emit_code(cg, "setle al");
        }
        else if (op_type == AST_BIN_OP_GTE)
        {
            emit_code(cg, "setge al");
        }

        emit_code(cg, "movzx rax, al");
    }
    break;

    case AST_BIN_OP_BITAND:
        emit_code(cg, "and rax, rbx");
        break;
    case AST_BIN_OP_BITOR:
        emit_code(cg, "or rax, rbx");
        break;
    case AST_BIN_OP_BITXOR:
        emit_code(cg, "xor rax, rbx");
        break;
    case AST_BIN_OP_SHR:
        emit_code(cg, "shr rbx, rax");
        emit_code(cg, "mov rax, rbx");
        break;
    case AST_BIN_OP_SHL:
        emit_code(cg, "shl rbx, rax");
        emit_code(cg, "mov rax, rbx");
        break;
    default:
        break;
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
    else if (node->type == AST_UNARY_OPERATOR)
    {
        codegen_unary_op(cg, node);
    }
}

static void codegen_address(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_VARIABLE)
        return;

    symbol_t *sym = node->meta.variable.sym;

    if (!sym)
        return;

    if (sym->scope == SCOPE_GLOBAL)
    {
        emit_code(cg, "; address of global variable '%s'", sym->name);
        emit_code(cg, "lea rax, [rel %s]", sym->name);
    }
    else
    {
        emit_code(cg, "; address of local variable '%s' at stack offset %d", sym->name, sym->stack_offset);
        emit_code(cg, "lea rax, [rbp - %d]", sym->stack_offset);
    }
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

static void codegen_if_statement(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_IF_STATEMENT)
        return;

    ast_node_t *condition = node->children[0];
    ast_node_t *then_branch = node->children[1];
    ast_node_t *else_branch = node->children_count > 2 ? node->children[2] : NULL;

    int id = next_label(cg);

    emit_code(cg, "; if statement");
    codegen_expression(cg, condition);

    emit_code(cg, "cmp rax, 0"); // rax == 0 means condition is false
    emit_code(cg, "je .L.else.%d", id);

    emit_label(cg, ".L.then.%d:", id);
    codegen_compound_statement(cg, then_branch);

    emit_label(cg, ".L.else.%d:", id);
    if (else_branch)
    {
        codegen_compound_statement(cg, else_branch);
    }

    emit_label(cg, ".L.end.%d:", id);
}

static void codegen_while_statement(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_WHILE_STATEMENT)
        return;

    ast_node_t *condition = node->children[0];
    ast_node_t *body = node->children[1];

    int id = next_label(cg);

    emit_label(cg, ".L.while.%d:", id);
    codegen_expression(cg, condition);
    emit_code(cg, "cmp rax, 0");       // rax == 0 means condition is false
    emit_code(cg, "je .L.end.%d", id); // jump to end if condition is false

    codegen_compound_statement(cg, body);
    emit_code(cg, "jmp .L.while.%d", id); // repeat the loop

    emit_label(cg, ".L.end.%d:", id);
}

static void codegen_declaration(codegen_state_t *cg, ast_node_t *node)
{
    // Declaration is not a statement, but declarations may have initializer expressions, so we need to handle them in codegen as well.

    if (node->type != AST_DECLARATION)
        return;

    if (node->children_count > 0)
    {
        ast_node_t *initializer_expr = node->children[0];
        codegen_expression(cg, initializer_expr); // value in rax

        symbol_t *sym = symtable_lookup(
            cg->current_function->meta.function.locals,
            node->meta.declaration.id);

        if (sym)
        {
            emit_code(cg, "mov [rbp - %d], rax", sym->stack_offset);
        }
    }
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
        else if (stmt->type == AST_IF_STATEMENT)
        {
            codegen_if_statement(cg, stmt);
        }
        else if (stmt->type == AST_WHILE_STATEMENT)
        {
            codegen_while_statement(cg, stmt);
        }
        else if (stmt->type == AST_COMPOUND_STATEMENT)
        {
            codegen_compound_statement(cg, stmt);
        }
        else if (stmt->type == AST_DECLARATION)
        {
            codegen_declaration(cg, stmt);
        }
    }
}

static void codegen_function_definition(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_FUNCTION_DEFINITION)
        return;

    int total_locals_offset = 8;

    if (node->meta.function.locals)
    {
        for (size_t i = 0; i < node->meta.function.locals->count; i++)
        {
            symbol_t *sym = node->meta.function.locals->symbols[i];

            if (sym)
            {
                int local_offset = calculate_alignment(total_locals_offset, sym->type->alignment);
                sym->stack_offset = local_offset;

                total_locals_offset = local_offset + sym->type->size;

                printf("symbol '%s' of type %d at stack offset %d\n", sym->name, sym->type->kind, sym->stack_offset);
            }
        }
    }

    int stack_size = calculate_alignment(total_locals_offset, 16); // align stack to 16 bytes

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

static void codegen_globals(codegen_state_t *cg, ast_node_t *node)
{
    for (int i = 0; i < node->meta.program.globals->count; i++)
    {
        symbol_t *sym = node->meta.program.globals->symbols[i];

        if (sym)
        {
            static char type_desc[64];
            ctype_explain(sym->type, type_desc, sizeof(type_desc));
            emit_label(cg, "; global variable %s", type_desc);
            emit_label(cg, "global %s", sym->name);
            emit_label(cg, "section .data");
            emit_label(cg, "%s:", sym->name);

            if (sym->type->size == 1)
            {
                emit_code(cg, "db 0");
            }
            else if (sym->type->size == 2)
            {
                emit_code(cg, "dw 0");
            }
            else if (sym->type->size == 4)
            {
                emit_code(cg, "dd 0");
            }
            else if (sym->type->size == 8)
            {
                emit_code(cg, "dq 0");
            }

            emit_label(cg, "");
        }
    }
}

static void codegen_program(codegen_state_t *cg, ast_node_t *node)
{
    if (node->type != AST_PROGRAM)
        return;

    emit_label(cg, "global _main");

    codegen_globals(cg, node);

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