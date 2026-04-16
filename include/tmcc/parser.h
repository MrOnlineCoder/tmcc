#ifndef PARSER_H
#define PARSER_H

#include <tmcc/ast.h>

#include <tmcc/lexer.h>

typedef struct
{
    ast_node_t *root;
    lexer_state_t *lexer;

    char err[256];

    size_t token_index;

    const token_t *cur_token;
} parser_state_t;

bool parser_init(parser_state_t *parser);

bool parser_run(parser_state_t *parser, lexer_state_t *lexer);

void parser_free(parser_state_t *parser);

#endif