#ifndef TYPES_H
#define TYPES_H

#define CTYPE_MAX_FUNCTION_PARAMS 16

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
    CTYPE_KIND_FUNCTION
} ctype_kind_t;

struct ctype_s
{
    ctype_kind_t kind;

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

#endif