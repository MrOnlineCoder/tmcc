#ifndef TOKENS_H
#define TOKENS_H

#include <string.h>

typedef enum
{
    TOKEN_INVALID,

    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_STRING,
    TOKEN_CHAR,

    TOKEN_KW_IF,
    TOKEN_KW_ELSE,
    TOKEN_KW_RETURN,
    TOKEN_KW_INT,
    TOKEN_KW_VOID,
    TOKEN_KW_STATIC,
    TOKEN_KW_CONST,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_ASTERISK,
    TOKEN_SLASH,

    TOKEN_EQUAL,
    TOKEN_LT,
    TOKEN_GT,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,

    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_COLON,

    TOKEN_EOF,
} token_type_t;

typedef struct
{
    token_type_t type;
    char *start;
    size_t length;

    size_t line;
    size_t column;
} token_t;

const char *token_type_to_string(token_type_t type);

#endif