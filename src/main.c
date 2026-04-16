#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <tmcc/lexer.h>
#include <tmcc/error.h>
#include <tmcc/parser.h>
#include <tmcc/semantic.h>

static lexer_state_t lexer;
static parser_state_t parser;
static semantic_state_t semantic;

int main(int argc, char *argv[])
{
    const char *source_filename = argv[1];

    if (argc < 2)
    {
        printf("Usage: tmcc <source_file>\n");
        return 0;
    }

    FILE *file = fopen(source_filename, "r");

    if (!file)
    {
        crash("cannot open source file %s: %s", source_filename, strerror(errno));
        return 1;
    }

    if (!lexer_init(&lexer))
    {
        crash("failed to initialize lexer");
        return 1;
    }

    if (!lexer_loadfile(&lexer, file))
    {
        crash("lexer file loading error: %s", lexer_get_error(&lexer));
        return 1;
    }

    fclose(file);

    lexer_run(&lexer);

    if (lexer_has_error(&lexer))
    {
        crash("lexer error: %s", lexer_get_error(&lexer));
        return 1;
    }

    printf("Tokens:\n");
    lexer_dump_tokens(&lexer);

    if (!parser_init(&parser))
    {
        crash("failed to initialize parser");
        return 1;
    }

    if (!parser_run(&parser, &lexer))
    {
        crash("%s", parser.err);
        return 1;
    }

    printf("\nAST:\n");

    ast_dump(parser.root, 0);

    if (!semantic_init(&semantic))
    {
        crash("failed to initialize semantic analyzer");
        return 1;
    }

    if (!semantic_run(&semantic, parser.root))
    {
        crash("semantic analysis failed");
        return 1;
    }

    semantic_free(&semantic);

    parser_free(&parser);

    return 0;
}