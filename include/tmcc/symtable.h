#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <tmcc/symbol.h>

#include <stdbool.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 256

typedef struct
{
    symbol_t *symbols[MAX_SYMBOL_TABLE_SIZE];
    size_t count;
} symbol_table_t;

void symtable_init(symbol_table_t *symtable);

symbol_table_t *symtable_new();

bool symtable_add(symbol_table_t *symtable, symbol_t *symbol);

symbol_t *symtable_lookup(const symbol_table_t *symtable, const char *name);

void symtable_free(symbol_table_t *symtable);

#endif