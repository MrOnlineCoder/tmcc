#ifndef LEXER_H
#define LEXER_H

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <tmcc/tokens.h>

#define LEXER_MAX_TOKENS 4096

typedef struct
{
    char *src;
    size_t src_length;

    size_t pos;

    size_t line;
    size_t column;

    token_t tokens[LEXER_MAX_TOKENS];
    size_t token_count;

    char err[256];
} lexer_state_t;

bool lexer_init(lexer_state_t *lexer);

bool lexer_loadfile(lexer_state_t *lexer, FILE *file);

void lexer_run(lexer_state_t *lexer);

void lexer_reset(lexer_state_t *lexer);

bool lexer_has_error(lexer_state_t *lexer);

const char *lexer_get_error(lexer_state_t *lexer);

void lexer_dump_tokens(lexer_state_t *lexer);

#endif