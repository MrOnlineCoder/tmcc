#include <tmcc/types.h>

#include <stdlib.h>

ctype_t CTYPE_BUILTIN_VOID = {
    .kind = CTYPE_KIND_VOID,
    .size = 0,
    .alignment = 1};

ctype_t CTYPE_BUILTIN_INT = {
    .kind = CTYPE_KIND_INT,
    .size = 4,
    .alignment = 4};

ctype_t CTYPE_BUILTIN_CHAR = {
    .kind = CTYPE_KIND_CHAR,
    .size = 1,
    .alignment = 1};

const ctype_t *ctype_binary_result_type(const ctype_t *left, const ctype_t *right)
{
    if (left->kind == CTYPE_KIND_INT && right->kind == CTYPE_KIND_INT)
    {
        return &CTYPE_BUILTIN_INT;
    }

    return NULL;
}