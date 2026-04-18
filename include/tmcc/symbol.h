#ifndef SYMBOL_H
#define SYMBOL_H

#include <tmcc/types.h>

#define MAX_SYMBOL_NAME_LENGTH 64

typedef enum
{
    SCOPE_LOCAL,
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
} symbol_scope_t;

typedef struct
{
    symbol_scope_t scope;
    const ctype_t *type;

    const char *name;

    unsigned int stack_offset;
} symbol_t;

symbol_t *symbol_new(const char *name, const ctype_t *type, symbol_scope_t scope, int stack_offset);

#endif