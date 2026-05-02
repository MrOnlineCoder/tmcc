#include <tmcc/codegen.h>

#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>

bool linker_assemble(
    codegen_state_t *cg,
    const char *output_filename)
{
    char asm_filename[256] = {0};
    char object_filename[256] = {0};
    char listing_filename[256] = {0};

    snprintf(asm_filename, sizeof(asm_filename), "%s.asm", output_filename);
    snprintf(object_filename, sizeof(object_filename), "%s.o", output_filename);
    snprintf(listing_filename, sizeof(listing_filename), "%s.lst", output_filename);

    FILE *asm_file = fopen(asm_filename, "w");

    if (!asm_file)
    {
        return false;
    }

    fwrite(cg->output, cg->output_length, 1, asm_file);

    fclose(asm_file);

    return true;

    int nasm_pid = fork();

    if (nasm_pid == -1)
    {
        return false;
    }

    if (nasm_pid == 0)
    {
        execlp("nasm", "nasm", "-f", "macho64", "-o", object_filename, "-l", listing_filename, asm_filename, (char *)NULL);
        return false; // never called
    }

    int nasm_status;

    waitpid(nasm_pid, &nasm_status, 0);

    if (nasm_status != 0)
    {
        return false;
    }

    int gcc_pid = fork();

    if (gcc_pid == -1)
    {
        return false;
    }

    if (gcc_pid == 0)
    {
        execlp("clang", "clang", "-arch", "x86_64", "-o", output_filename, object_filename, (char *)NULL);
        return false; // never called
    }

    int clang_status;

    waitpid(gcc_pid, &clang_status, 0);

    if (clang_status != 0)
    {
        return false;
    }

    return true;
}