#include <tmcc/types.h>

ctype_t CTYPE_BUILTIN_VOID = {
    .kind = CTYPE_KIND_VOID,
    .size = 0,
    .alignment = 1};

ctype_t CTYPE_BUILTIN_INT = {
    .kind = CTYPE_KIND_INT,
    .size = 4,
    .alignment = 4};

ctype_t CTYPE_BUILTIN_CHAR = {
    .kind = CTYPE_KIND_CHAR,
    .size = 1,
    .alignment = 1};