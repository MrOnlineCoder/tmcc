#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <tmcc/lexer.h>
#include <tmcc/error.h>

static lexer_state_t lexer;

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

    lexer_dump_tokens(&lexer);

    return 0;
}