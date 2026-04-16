#include <tmcc/parser.h>
#include <stdarg.h>
#include <stdlib.h>

static void parser_fatal_error(parser_state_t *parser, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(parser->err, sizeof(parser->err), format, args);
    va_end(args);
}

static void parser_error(parser_state_t *parser, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    snprintf(parser->err, sizeof(parser->err), "at line %zu:%zu: ", parser->cur_token->line, parser->cur_token->column);
    vsnprintf(parser->err + strlen(parser->err), sizeof(parser->err) - strlen(parser->err), format, args);
    va_end(args);
}

static const token_t *parser_peek(parser_state_t *parser)
{
    if (parser->token_index < parser->lexer->token_count)
    {
        return &parser->lexer->tokens[parser->token_index];
    }

    return NULL;
}

static const token_t *parser_next(parser_state_t *parser)
{
    if (parser->token_index < parser->lexer->token_count)
    {
        return &parser->lexer->tokens[parser->token_index++];
    }

    return NULL;
}

static bool parser_is_eof(parser_state_t *parser)
{
    return parser->token_index >= parser->lexer->token_count || parser_peek(parser)->type == TOKEN_EOF;
}

static ast_node_t *parser_make_node(parser_state_t *parser, ast_node_type_t type)
{
    return ast_make_node(type, parser_peek(parser));
}

static const token_t *parser_expect(parser_state_t *parser, token_type_t type)
{
    const token_t *token = parser_peek(parser);

    if (token && token->type == type)
    {
        parser->cur_token = token;
        parser->token_index++;
        return token;
    }

    parser_error(parser, "expected token %s, got %s instead",
                 token_type_to_string(type), token_type_to_string(token->type));

    return NULL;
}

static bool parser_test(parser_state_t *parser, token_type_t type)
{
    const token_t *token = parser_peek(parser);
    return token && token->type == type;
}

static bool extract_token_string(const token_t *token, char *buffer, size_t buffer_size)
{
    if (token->length >= buffer_size)
    {
        return false;
    }

    memcpy(buffer, token->start, token->length);
    buffer[token->length] = '\0';
    return true;
}

bool parser_init(parser_state_t *parser)
{
    parser->root = NULL;
    parser->err[0] = '\0';
    parser->token_index = 0;
    parser->cur_token = NULL;

    return true;
}

/* ------- */

/*Forward declarations*/
static ast_node_t *parse_expression(parser_state_t *parser);
/*End of forward declarations*/

static ast_node_t *parse_integer_literal(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_INTEGER_LITERAL);

    const token_t *token = parser_expect(parser, TOKEN_INTEGER);
    if (!token)
        return NULL;

    char buffer[64];

    if (!extract_token_string(token, buffer, sizeof(buffer)))
    {
        parser_error(parser, "integer literal too long");
        return NULL;
    }

    node->meta.integer_literal.value = atoi(buffer);

    return node;
}

static ast_node_t *parse_primary_expression(parser_state_t *parser)
{
    if (parser_test(parser, TOKEN_INTEGER))
    {
        return parse_integer_literal(parser);
    }
    else if (parser_test(parser, TOKEN_LPAREN))
    {
        parser_expect(parser, TOKEN_LPAREN);
        ast_node_t *expr = parse_expression(parser);
        if (!expr)
            return NULL;
        parser_expect(parser, TOKEN_RPAREN);
        return expr;
    }

    parser_error(parser, "unexpected token");
    return NULL;
}

static ast_node_t *parse_multiplicative_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_primary_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    if (parser_test(parser, TOKEN_ASTERISK) || parser_test(parser, TOKEN_SLASH))
    {
        const token_t *op_token = parser_next(parser);

        ast_bin_op_type_t op_type;

        if (op_token->type == TOKEN_ASTERISK)
        {
            op_type = AST_BIN_OP_MUL;
        }
        else
        {
            op_type = AST_BIN_OP_DIV;
        }

        node->meta.binary_op.op_type = op_type;

        ast_node_t *right = parse_multiplicative_expression(parser);
        if (!right)
        {
            return NULL;
        }

        ast_add_child(node, right);
    }

    if (node->children_count == 1)
    {
        ast_free(node);
        return left;
    }

    return node;
}

static ast_node_t *parse_additive_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_multiplicative_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    if (parser_test(parser, TOKEN_PLUS) || parser_test(parser, TOKEN_MINUS))
    {
        const token_t *op_token = parser_next(parser);

        ast_bin_op_type_t op_type;

        if (op_token->type == TOKEN_PLUS)
        {
            op_type = AST_BIN_OP_ADD;
        }
        else
        {
            op_type = AST_BIN_OP_SUB;
        }

        node->meta.binary_op.op_type = op_type;

        ast_node_t *right = parse_additive_expression(parser);
        if (!right)
        {
            return NULL;
        }

        ast_add_child(node, right);
    }

    if (node->children_count == 1)
    {
        ast_free(node);
        return left;
    }

    return node;
}

static ast_node_t *parse_expression(parser_state_t *parser)
{
    // ast_node_t *node = parser_make_node(parser, AST_EXPRESSION);

    // ast_node_t *v = parse_additive_expression(parser);

    // if (!v)
    //     return NULL;

    // ast_add_child(node, v);

    // return node;
    return parse_additive_expression(parser);
}

static ast_node_t *parse_return_statement(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_RETURN_STATEMENT);

    parser_expect(parser, TOKEN_RETURN);

    ast_node_t *expr = parse_expression(parser);

    if (!expr)
        return NULL;

    ast_add_child(node, expr);

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ast_node_t *parse_compound_statement(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_COMPOUND_STATEMENT);

    parser_expect(parser, TOKEN_LBRACE);

    while (!parser_is_eof(parser) && !parser_test(parser, TOKEN_RBRACE))
    {
        ast_node_t *stmt = parse_return_statement(parser);

        if (stmt)
        {
            ast_add_child(node, stmt);
        }
        else
        {
            break;
        }
    }

    parser_expect(parser, TOKEN_RBRACE);

    return node;
}

static ast_node_t *parse_function_signature(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_FUNCTION_SIGNATURE);

    parser_expect(parser, TOKEN_IDENTIFIER); /* return type */

    node->meta.function_signature.name = parser_expect(parser, TOKEN_IDENTIFIER); /* function name */
    node->meta.function_signature.is_inline = false;
    node->meta.function_signature.is_static = false;

    parser_expect(parser, TOKEN_LPAREN);
    // TODO: parse parameters
    parser_expect(parser, TOKEN_RPAREN);

    return node;
}

static ast_node_t *parse_function_definition(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_FUNCTION_DEFINITION);

    /*TODO: implement type parsing*/

    ast_node_t *signature = parse_function_signature(parser);
    ast_node_t *body = parse_compound_statement(parser);

    ast_add_child(node, signature);
    ast_add_child(node, body);

    return node;
}

static ast_node_t *parse_program(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_PROGRAM);

    ast_add_child(
        node, parse_function_definition(parser));

    return node;
}

/* ------- */

bool parser_run(parser_state_t *parser, lexer_state_t *lexer)
{
    if (!lexer || !lexer->token_count)
    {
        parser_fatal_error(parser, "lexer has no tokens to parse");
        return false;
    }

    parser->lexer = lexer;

    parser->root = parse_program(parser);

    return parser->err[0] == '\0';
}

void parser_free(parser_state_t *parser)
{
    if (parser->root)
    {
        ast_free_deep(parser->root);
        parser->root = NULL;
    }
}
