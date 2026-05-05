#include <tmcc/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ctype_t CTYPE_BUILTIN_VOID = {
    .kind = CTYPE_KIND_VOID,
    .size = 0,
    .alignment = 1,
    .name = "void"};

ctype_t CTYPE_BUILTIN_INT = {
    .kind = CTYPE_KIND_INT,
    .size = 4,
    .alignment = 8,
    .name = "int"};

ctype_t CTYPE_BUILTIN_CHAR = {
    .kind = CTYPE_KIND_CHAR,
    .size = 1,
    .alignment = 1,
    .name = "char"};

ctype_t CTYPE_BUILTIN_SHORT = {
    .kind = CTYPE_KIND_SHORT,
    .size = 2,
    .alignment = 2,
    .name = "short"};

ctype_t CTYPE_BUILTIN_LONG = {
    .kind = CTYPE_KIND_LONG,
    .size = 8,
    .alignment = 8,
    .name = "long"};

ctype_t CTYPE_BUILTIN_FLOAT = {
    .kind = CTYPE_KIND_FLOAT,
    .size = 4,
    .alignment = 4,
    .name = "float"};

ctype_t CTYPE_BUILTIN_DOUBLE = {
    .kind = CTYPE_KIND_DOUBLE,
    .size = 8,
    .alignment = 8,
    .name = "double"};

static ctype_t *get_builtin_type(ctype_kind_t kind)
{
    switch (kind)
    {
    case CTYPE_KIND_VOID:
        return &CTYPE_BUILTIN_VOID;
    case CTYPE_KIND_INT:
        return &CTYPE_BUILTIN_INT;
    case CTYPE_KIND_CHAR:
        return &CTYPE_BUILTIN_CHAR;
    case CTYPE_KIND_SHORT:
        return &CTYPE_BUILTIN_SHORT;
    case CTYPE_KIND_LONG:
        return &CTYPE_BUILTIN_LONG;
    case CTYPE_KIND_FLOAT:
        return &CTYPE_BUILTIN_FLOAT;
    case CTYPE_KIND_DOUBLE:
        return &CTYPE_BUILTIN_DOUBLE;
    default:
        return NULL;
    }
}

const ctype_t *ctype_binary_result_type(const ctype_t *left, const ctype_t *right)
{
    if (left->kind == right->kind)
    {
        return get_builtin_type(left->kind);
    }

    return NULL;
}

ctype_t *ctype_new(ctype_kind_t kind)
{
    ctype_t *tt = malloc(sizeof(ctype_t));
    if (!tt)
    {
        return NULL;
    }
    tt->kind = kind;
    tt->size = 0;
    tt->alignment = 1;
    tt->name = NULL;

    return tt;
}

ctype_t *ctype_make_pointer(const ctype_t *base)
{
    ctype_t *tt = ctype_new(CTYPE_KIND_POINTER);

    if (!tt)
    {
        return NULL;
    }

    tt->size = sizeof(void *);
    tt->alignment = sizeof(void *);
    tt->meta.pointer.base = (ctype_t *)base;
    tt->original = base;

    return tt;
}

ctype_t *ctype_make_array(const ctype_t *element_type, int size)
{
    ctype_t *tt = ctype_new(CTYPE_KIND_ARRAY);

    if (!tt)
    {
        return NULL;
    }

    tt->size = element_type->size * size;
    tt->alignment = element_type->alignment;
    tt->meta.array.element_type = (ctype_t *)element_type;
    tt->meta.array.size = size;

    return tt;
}

void ctype_explain(const ctype_t *tt, char *output, int output_size)
{
    const ctype_t *ptr = tt;

    static char temp[128] = {0};

    strncat(output, " => ", output_size - strlen(output) - 1);

    while (ptr)
    {
        if (ptr->kind == CTYPE_KIND_POINTER)
        {
            snprintf(temp, sizeof(temp), "pointer to ");
            ptr = ptr->meta.pointer.base;
        }
        else if (ptr->kind == CTYPE_KIND_ARRAY)
        {
            snprintf(temp, sizeof(temp), "array of %d ", ptr->meta.array.size);
            ptr = ptr->meta.array.element_type;
        }
        else
        {
            snprintf(temp, sizeof(temp), "%s ", ptr->name ? ptr->name : "unknown");
            ptr = NULL;
        }

        strncat(output, temp, output_size - strlen(output) - 1);
    }

    snprintf(temp, sizeof(temp), "(%u bytes, originally %s)", tt->size, tt->original->name);
    strncat(output, temp, output_size - strlen(output) - 1);
}

ctype_t *ctype_clone(const ctype_t *tt)
{
    ctype_t *clone = ctype_new(tt->kind);

    if (!clone)
    {
        return NULL;
    }

    *clone = *tt;
    clone->original = (ctype_t *)tt;

    return clone;
}

const ctype_t *ctype_unary_result_type(ast_unary_op_type_t op_type, const ctype_t *operand)
{
    if (op_type == AST_UNARY_OP_SIZEOF)
    {
        return &CTYPE_BUILTIN_INT; // TODO: size_t?
    }

    if (op_type == AST_UNARY_OP_ADDR)
    {
        return ctype_make_pointer(operand);
    }

    // TODO: pointers

    return operand;
}