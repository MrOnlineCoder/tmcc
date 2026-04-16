#include <tmcc/lexer.h>

#include <stdlib.h>

static void lexer_error(lexer_state_t *lexer, const char *msg)
{
    snprintf(lexer->err, sizeof(lexer->err), "%s at line %zu:%zu", msg, lexer->line, lexer->column);
}

static void lexer_add_token(lexer_state_t *lexer, token_type_t type, size_t length)
{
    if (lexer->token_count >= LEXER_MAX_TOKENS)
    {
        lexer_error(lexer, "too many tokens");
        return;
    }

    token_t *token = &lexer->tokens[lexer->token_count];
    token->type = type;
    token->start = lexer->src + lexer->pos;
    token->length = length;
    token->line = lexer->line;
    token->column = lexer->column;

    lexer->token_count++;
}

static void lexer_add_token_at(lexer_state_t *lexer, token_type_t type, size_t start_pos, size_t length)
{
    if (lexer->token_count >= LEXER_MAX_TOKENS)
    {
        lexer_error(lexer, "too many tokens");
        return;
    }

    token_t *token = &lexer->tokens[lexer->token_count];
    token->type = type;
    token->start = lexer->src + start_pos;
    token->length = length;
    token->line = lexer->line;
    token->column = lexer->column;

    lexer->token_count++;
}

static void lexer_advance(lexer_state_t *lexer)
{
    if (lexer->pos < lexer->src_length)
    {
        lexer->pos++;
    }
}

static void lexer_advance_many(lexer_state_t *lexer, size_t count)
{
    if (lexer->pos + count <= lexer->src_length)
    {
        lexer->pos += count;
    }
    else
    {
        lexer->pos = lexer->src_length;
    }
}

static bool lexer_is_eof(lexer_state_t *lexer)
{
    return lexer->pos >= lexer->src_length;
}

static bool lexer_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool lexer_is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool lexer_is_alphanumeric(char c)
{
    return lexer_is_alpha(c) || lexer_is_digit(c);
}

static token_type_t lexer_detect_keyword_or_id(const char *start, size_t length)
{
    if (length == 2 && strncmp(start, "if", 2) == 0)
        return TOKEN_IF;
    if (length == 4 && strncmp(start, "else", 4) == 0)
        return TOKEN_ELSE;
    if (length == 6 && strncmp(start, "return", 6) == 0)
        return TOKEN_RETURN;

    return TOKEN_IDENTIFIER;
}

bool lexer_init(lexer_state_t *lexer)
{
    lexer->pos = 0;
    lexer->src = NULL;
    lexer->src_length = 0;
    lexer->token_count = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->err[0] = '\0';
    memset(lexer->tokens, 0, sizeof(lexer->tokens));
    return true;
}

bool lexer_loadfile(lexer_state_t *lexer, FILE *file)
{
    if (!lexer)
        return false;

    if (lexer->src)
    {
        lexer_error(lexer, "lexer already has source loaded");
        return false;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0)
    {
        lexer_error(lexer, "failed to determine file size");
        return false;
    }

    lexer->src = malloc(file_size + 1);
    if (!lexer->src)
    {
        lexer_error(lexer, "failed to allocate memory for source");
        return false;
    }

    size_t read_size = fread(lexer->src, 1, file_size, file);
    if (read_size != file_size)
    {
        lexer_error(lexer, "failed to read entire file into memory");
        lexer_reset(lexer);
        return false;
    }

    lexer->src_length = read_size;
    lexer->src[read_size] = '\0';

    return true;
}

void lexer_run(lexer_state_t *lexer)
{
    if (!lexer->src)
    {
        lexer_error(lexer, "no source loaded in lexer");
        return;
    }

    while (!lexer_is_eof(lexer))
    {
        char c = lexer->src[lexer->pos];

        if (c == '\n')
        {
            lexer->line++;
            lexer->column = 1;
        }
        else
        {
            lexer->column++;
        }

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            lexer_advance(lexer);
            continue;
        }

        if (c == '(')
        {
            lexer_add_token(lexer, TOKEN_LPAREN, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == ')')
        {
            lexer_add_token(lexer, TOKEN_RPAREN, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '{')
        {
            lexer_add_token(lexer, TOKEN_LBRACE, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '}')
        {
            lexer_add_token(lexer, TOKEN_RBRACE, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == ';')
        {
            lexer_add_token(lexer, TOKEN_SEMICOLON, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == ',')
        {
            lexer_add_token(lexer, TOKEN_COMMA, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == ':')
        {
            lexer_add_token(lexer, TOKEN_COLON, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '+')
        {
            lexer_add_token(lexer, TOKEN_PLUS, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '-')
        {
            lexer_add_token(lexer, TOKEN_MINUS, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '*')
        {
            lexer_add_token(lexer, TOKEN_ASTERISK, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '/')
        {
            if (lexer->pos + 1 < lexer->src_length && lexer->src[lexer->pos + 1] == '*')
            {
                lexer_advance_many(lexer, 2);
                while (!lexer_is_eof(lexer))
                {
                    char comment_char = lexer->src[lexer->pos];

                    if (comment_char == '*')
                    {
                        if (lexer->pos + 1 < lexer->src_length && lexer->src[lexer->pos + 1] == '/')
                        {
                            lexer_advance_many(lexer, 2);
                            break;
                        }
                    }

                    lexer_advance(lexer);
                }
                continue;
            }

            lexer_add_token(lexer, TOKEN_SLASH, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '=')
        {
            lexer_add_token(lexer, TOKEN_EQUAL, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '<')
        {
            lexer_add_token(lexer, TOKEN_LT, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '>')
        {
            lexer_add_token(lexer, TOKEN_GT, 1);
            lexer_advance(lexer);
            continue;
        }

        if (c == '"')
        {
            lexer_advance(lexer);

            size_t start_pos = lexer->pos;

            while (lexer->pos < lexer->src_length && lexer->src[lexer->pos] != '"')
            {
                lexer_advance(lexer);
            }

            if (lexer_is_eof(lexer))
            {
                lexer_error(lexer, "unterminated string literal");
                return;
            }

            size_t length = lexer->pos - start_pos;

            lexer_add_token_at(lexer, TOKEN_STRING, start_pos, length);
            continue;
        }

        if (lexer_is_alpha(c))
        {
            size_t start_pos = lexer->pos;

            while (lexer->pos < lexer->src_length && lexer_is_alphanumeric(lexer->src[lexer->pos]))
            {
                lexer_advance(lexer);
            }

            size_t length = lexer->pos - start_pos;

            const char *token_content = lexer->src + start_pos;

            token_type_t type = lexer_detect_keyword_or_id(token_content, length);

            lexer_add_token_at(lexer, type, start_pos, length);
            continue;
        }

        if (lexer_is_digit(c))
        {
            size_t start_pos = lexer->pos;

            while (lexer->pos < lexer->src_length && lexer_is_digit(lexer->src[lexer->pos]))
            {
                lexer_advance(lexer);
            }

            size_t length = lexer->pos - start_pos;

            lexer_add_token_at(lexer, TOKEN_INTEGER, start_pos, length);
            continue;
        }

        lexer_advance(lexer);
    }
}

void lexer_reset(lexer_state_t *lexer)
{
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->token_count = 0;
    memset(lexer->tokens, 0, sizeof(lexer->tokens));

    if (lexer->src)
    {
        free(lexer->src);
        lexer->src = NULL;
        lexer->src_length = 0;
    }
}

bool lexer_has_error(lexer_state_t *lexer)
{
    return lexer->err[0] != '\0';
}

const char *lexer_get_error(lexer_state_t *lexer)
{
    return lexer->err;
}

const char *token_type_to_string(token_type_t type)
{
    switch (type)
    {
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_INTEGER:
        return "INTEGER";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_IF:
        return "IF";
    case TOKEN_ELSE:
        return "ELSE";
    case TOKEN_RETURN:
        return "RETURN";
    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_ASTERISK:
        return "ASTERISK";
    case TOKEN_SLASH:
        return "SLASH";
    case TOKEN_EQUAL:
        return "EQUAL";
    case TOKEN_LT:
        return "LT";
    case TOKEN_GT:
        return "GT";
    case TOKEN_LPAREN:
        return "LPAREN";
    case TOKEN_RPAREN:
        return "RPAREN";
    case TOKEN_LBRACE:
        return "LBRACE";
    case TOKEN_RBRACE:
        return "RBRACE";
    case TOKEN_SEMICOLON:
        return "SEMICOLON";
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_COLON:
        return "COLON";
    case TOKEN_EOF:
        return "EOF";
    default:
        return "UNKNOWN";
    }
}

void lexer_dump_tokens(lexer_state_t *lexer)
{
    char content[256];
    for (size_t i = 0; i < lexer->token_count; i++)
    {
        token_t *token = &lexer->tokens[i];

        memcpy(content, token->start, token->length);
        content[token->length] = '\0';

        printf("Token %zu: type=%s, line=%zu, column=%zu, text='%s'\n",
               i, token_type_to_string(token->type), token->line, token->column,
               content);
    }
}