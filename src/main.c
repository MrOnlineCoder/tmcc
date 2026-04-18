#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <tmcc/lexer.h>
#include <tmcc/error.h>
#include <tmcc/parser.h>
#include <tmcc/semantic.h>
#include <tmcc/codegen.h>
#include <tmcc/linker.h>

static lexer_state_t lexer;
static parser_state_t parser;
static semantic_state_t semantic;
static codegen_state_t codegen;

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
        ast_dump(parser.root, 0);
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
        crash("semantic analysis failed: %s", semantic.err);
        return 1;
    }

    if (!codegen_init(&codegen))
    {
        crash("failed to initialize code generator");
        return 1;
    }

    if (!codegen_run(&codegen, parser.root))
    {
        crash("code generation failed");
        return 1;
    }

    printf("%s\n", codegen.output);

    // if (!linker_assemble(&codegen, "output"))
    // {
    //     crash("linking failed");
    //     return 1;
    // }

    semantic_free(&semantic);
    parser_free(&parser);

    return 0;
}