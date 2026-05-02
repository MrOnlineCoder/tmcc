#include <tmcc/types.h>

#include <stdlib.h>

ctype_t CTYPE_BUILTIN_VOID = {
    .kind = CTYPE_KIND_VOID,
    .size = 0,
    .alignment = 1};

ctype_t CTYPE_BUILTIN_INT = {
    .kind = CTYPE_KIND_INT,
    .size = 4,
    .alignment = 8};

ctype_t CTYPE_BUILTIN_CHAR = {
    .kind = CTYPE_KIND_CHAR,
    .size = 1,
    .alignment = 1};

ctype_t CTYPE_BUILTIN_SHORT = {
    .kind = CTYPE_KIND_SHORT,
    .size = 2,
    .alignment = 2};

ctype_t CTYPE_BUILTIN_LONG = {
    .kind = CTYPE_KIND_LONG,
    .size = 8,
    .alignment = 8};

ctype_t CTYPE_BUILTIN_FLOAT = {
    .kind = CTYPE_KIND_FLOAT,
    .size = 4,
    .alignment = 4};

ctype_t CTYPE_BUILTIN_DOUBLE = {
    .kind = CTYPE_KIND_DOUBLE,
    .size = 8,
    .alignment = 8};

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