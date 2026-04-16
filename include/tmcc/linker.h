#ifndef LINKER_H
#define LINKER_H

#include <tmcc/codegen.h>

bool linker_assemble(
    codegen_state_t *cg,
    const char *output_filename);

#endif