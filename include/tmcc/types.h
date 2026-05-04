#ifndef TYPES_H
#define TYPES_H

#define CTYPE_MAX_FUNCTION_PARAMS 16

#include <tmcc/ast.h>

typedef enum
{
    CTYPE_KIND_VOID,
    CTYPE_KIND_INT,
    CTYPE_KIND_CHAR,
    CTYPE_KIND_SHORT,
    CTYPE_KIND_LONG,
    CTYPE_KIND_FLOAT,
    CTYPE_KIND_DOUBLE,
    CTYPE_KIND_POINTER,
    CTYPE_KIND_FUNCTION,
    CTYPE_KIND_ARRAY,
} ctype_kind_t;

struct ctype_s
{
    ctype_kind_t kind;
    struct ctype_s *original;

    const char *name;

    unsigned int size;
    unsigned int alignment;

    union
    {
        struct
        {
            struct ctype_s *base;
        } pointer;

        struct
        {
            struct ctype_s *return_type;
            unsigned int param_count;
            struct ctype_s *params[CTYPE_MAX_FUNCTION_PARAMS];
        } function;

        struct
        {
            struct ctype_s *element_type;
            int size;
        } array;
    } meta;
};

typedef struct ctype_s ctype_t;

extern ctype_t CTYPE_BUILTIN_VOID;
extern ctype_t CTYPE_BUILTIN_INT;
extern ctype_t CTYPE_BUILTIN_CHAR;
extern ctype_t CTYPE_BUILTIN_SHORT;
extern ctype_t CTYPE_BUILTIN_LONG;
extern ctype_t CTYPE_BUILTIN_FLOAT;
extern ctype_t CTYPE_BUILTIN_DOUBLE;

const ctype_t *ctype_binary_result_type(const ctype_t *left, const ctype_t *right);
const ctype_t *ctype_unary_result_type(ast_unary_op_type_t op_type, const ctype_t *operand);

ctype_t *ctype_clone(const ctype_t *tt);

ctype_t *ctype_new(ctype_kind_t kind);
ctype_t *ctype_make_pointer(const ctype_t *base);
ctype_t *ctype_make_array(const ctype_t *element_type, int size);

void ctype_explain(const ctype_t *tt, char *output, int output_size);

#endif