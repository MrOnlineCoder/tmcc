#include <tmcc/symtable.h>

#include <stdlib.h>

void symtable_init(symbol_table_t *symtable)
{
    memset(symtable, 0, sizeof(symbol_table_t));
    symtable->count = 0;
}

bool symtable_add(symbol_table_t *symtable, symbol_t *symbol)
{
    symbol_t *existing = symtable_lookup(symtable, symbol->name);
    if (existing)
        return false;

    if (symtable->count >= MAX_SYMBOL_TABLE_SIZE)
        return false;

    symtable->symbols[symtable->count++] = symbol;
    return true;
}

symbol_t *symtable_lookup(symbol_table_t *symtable, const char *name)
{
    for (size_t i = 0; i < symtable->count; i++)
    {
        if (strncmp(symtable->symbols[i]->name, name, MAX_SYMBOL_NAME_LENGTH) == 0)
        {
            return symtable->symbols[i];
        }
    }

    return NULL;
}

void symtable_free(symbol_table_t *symtable)
{
    for (size_t i = 0; i < symtable->count; i++)
    {
        free(symtable->symbols[i]);
    }
    symtable->count = 0;
}

symbol_t *symbol_new(const char *name, const ctype_t *type, symbol_scope_t scope, int stack_offset)
{
    symbol_t *sym = malloc(sizeof(symbol_t));
    if (!sym)
        return NULL;

    sym->name = name;
    sym->type = type;
    sym->scope = scope;
    sym->stack_offset = stack_offset;

    return sym;
}