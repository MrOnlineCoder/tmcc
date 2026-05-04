#include <tmcc/ast.h>

#include <stdlib.h>
#include <stdio.h>

ast_node_t *ast_make_node(ast_node_type_t type, const token_t *start_token)
{
    ast_node_t *node = calloc(1, sizeof(ast_node_t));
    node->type = type;
    node->start_token = start_token;
    node->children_count = 0;

    return node;
}

void ast_add_child(ast_node_t *parent, ast_node_t *child)
{
    if (parent->children_count < AST_MAX_CHILDREN)
    {
        parent->children[parent->children_count++] = child;
    }
}

static const char *ast_type_to_string(ast_node_type_t type)
{
    switch (type)
    {
    case AST_PROGRAM:
        return "PROGRAM";
    case AST_COMPOUND_STATEMENT:
        return "COMPOUND_STATEMENT";
    case AST_FUNCTION_DEFINITION:
        return "FUNCTION_DEFINITION";
    case AST_FUNCTION_SIGNATURE:
        return "FUNCTION_SIGNATURE";
    case AST_RETURN_STATEMENT:
        return "RETURN_STATEMENT";
    case AST_EXPRESSION:
        return "EXPRESSION";
    case AST_INTEGER_LITERAL:
        return "INTEGER_LITERAL";
    case AST_BINARY_OPERATOR:
        return "BINARY_OPERATOR";
    case AST_ASSIGN_STATEMENT:
        return "ASSIGN_STATEMENT";
    case AST_DECLARATION:
        return "DECLARATION";
    case AST_VARIABLE:
        return "VARIABLE";
    case AST_IF_STATEMENT:
        return "IF_STATEMENT";
    case AST_WHILE_STATEMENT:
        return "WHILE_STATEMENT";
    default:
        return "UNKNOWN";
    }
}

void ast_dump(ast_node_t *node, int indent)
{
    if (!node)
        return;

    char indent_str[64];

    char meta_str[256] = {0};

    for (int i = 0; i < indent; i++)
    {
        indent_str[i] = ' ';
    }
    indent_str[indent] = '\0';

    if (node->type == AST_BINARY_OPERATOR)
    {
        snprintf(meta_str, sizeof(meta_str), " (%s)", bin_op_to_string(node->meta.binary_op.op_type));
    }
    else if (node->type == AST_VARIABLE)
    {
        snprintf(meta_str, sizeof(meta_str), " (%s)", node->meta.variable.sym ? node->meta.variable.sym->name : "unknown");
    }
    else if (node->type == AST_INTEGER_LITERAL)
    {
        snprintf(meta_str, sizeof(meta_str), " (%d)", node->meta.integer_literal.value);
    }
    else if (node->type == AST_DECLARATION)
    {
        ctype_explain(node->meta.declaration.ds.type, meta_str, sizeof(meta_str));

        if (node->meta.declaration.ds.is_static)
        {
            strncat(meta_str, ", static", sizeof(meta_str) - strlen(meta_str) - 1);
        }
        else if (node->meta.declaration.ds.is_extern)
        {
            strncat(meta_str, ", extern", sizeof(meta_str) - strlen(meta_str) - 1);
        }
        else if (node->meta.declaration.ds.is_typedef)
        {
            strncat(meta_str, ", typedef", sizeof(meta_str) - strlen(meta_str) - 1);
        }

        if (node->meta.declaration.ds.is_const)
        {
            strncat(meta_str, ", const", sizeof(meta_str) - strlen(meta_str) - 1);
        }

        if (node->meta.declaration.ds.is_volatile)
        {
            strncat(meta_str, ", volatile", sizeof(meta_str) - strlen(meta_str) - 1);
        }
    }

    printf("%s%s%s\n", indent_str, ast_type_to_string(node->type), meta_str);
    for (int i = 0; i < node->children_count; i++)
    {
        if (!node->children[i])
        {
            printf("%s  (null-child)\n", indent_str);
            continue;
        }
        ast_dump(node->children[i], indent + 2);
    }
}

void ast_free_deep(ast_node_t *node)
{
    for (int i = 0; i < node->children_count; i++)
    {
        if (node->children[i])
        {
            ast_free_deep(node->children[i]);
        }
    }
    free(node);
}

void ast_free(ast_node_t *node)
{
    free(node);
}

const char *bin_op_to_string(ast_bin_op_type_t op_type)
{
    switch (op_type)
    {
    case AST_BIN_OP_ADD:
        return "+";
    case AST_BIN_OP_SUB:
        return "-";
    case AST_BIN_OP_MUL:
        return "*";
    case AST_BIN_OP_DIV:
        return "/";
    case AST_BIN_OP_EQ:
        return "==";
    case AST_BIN_OP_NEQ:
        return "!=";
    case AST_BIN_OP_LT:
        return "<";
    case AST_BIN_OP_GT:
        return ">";
    case AST_BIN_OP_LTE:
        return "<=";
    case AST_BIN_OP_GTE:
        return ">=";
    case AST_BIN_OP_AND:
        return "&&";
    case AST_BIN_OP_OR:
        return "||";
    case AST_BIN_OP_SHL:
        return "<<";
    case AST_BIN_OP_SHR:
        return ">>";
    case AST_BIN_OP_MOD:
        return "%%";
    case AST_BIN_OP_BITAND:
        return "&";
    case AST_BIN_OP_BITOR:
        return "|";
    case AST_BIN_OP_BITXOR:
        return "^";
    default:
        return "unknown";
    }
}