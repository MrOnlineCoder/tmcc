#include <tmcc/parser.h>
#include <tmcc/types.h>
#include <tmcc/tokens.h>

#include <stdarg.h>
#include <stdlib.h>

typedef struct
{
    ctype_t *type;
    bool is_static;
    bool is_const;
} declspec_t;

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
static ast_node_t *parse_compound_statement(parser_state_t *parser);
/*End of forward declarations*/

static ast_node_t *parse_integer_literal(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_INTEGER_LITERAL);

    const token_t *token = parser_expect(parser, TOKEN_INTEGER);
    if (!token)
        return NULL;

    const char *buffer = extract_token_as_new_string(token);

    node->meta.integer_literal.value = atoi(buffer);

    free((char *)buffer);

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
    else if (parser_test(parser, TOKEN_IDENTIFIER))
    {
        ast_node_t *var = parser_make_node(parser, AST_VARIABLE);

        var->meta.variable.sym = NULL;

        parser_expect(parser, TOKEN_IDENTIFIER);

        return var;
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

static ast_node_t *parse_relational_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_additive_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    while (parser_test(parser, TOKEN_LT) || parser_test(parser, TOKEN_GT))
    {
        const token_t *op_token = parser_next(parser);

        ast_bin_op_type_t op_type;

        if (op_token->type == TOKEN_LT)
        {
            op_type = AST_BIN_OP_LT;

            if (parser_test(parser, TOKEN_EQUAL))
            {
                parser_next(parser);
                op_type = AST_BIN_OP_LTE;
            }
        }
        else
        {
            op_type = AST_BIN_OP_GT;

            if (parser_test(parser, TOKEN_EQUAL))
            {
                parser_next(parser);
                op_type = AST_BIN_OP_GTE;
            }
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

static ast_node_t *parse_equality_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_relational_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    while (parser_test(parser, TOKEN_EQUAL) || parser_test(parser, TOKEN_BANG))
    {
        const token_t *op_token = parser_next(parser);

        ast_bin_op_type_t op_type;

        if (op_token->type == TOKEN_EQUAL)
        {
            op_type = AST_BIN_OP_EQ;
        }
        else
        {
            if (!parser_expect(parser, TOKEN_EQUAL))
            {
                return NULL;
            }

            op_type = AST_BIN_OP_NEQ;
        }

        node->meta.binary_op.op_type = op_type;

        ast_node_t *right = parse_relational_expression(parser);
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
    return parse_equality_expression(parser);
}

static ast_node_t *parse_return_statement(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_RETURN_STATEMENT);

    parser_expect(parser, TOKEN_KW_RETURN);

    ast_node_t *expr = parse_expression(parser);

    if (!expr)
        return NULL;

    ast_add_child(node, expr);

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ast_node_t *parse_assign_statement(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_ASSIGN_STATEMENT);

    ast_add_child(node, parse_primary_expression(parser));

    parser_expect(parser, TOKEN_EQUAL);

    ast_add_child(node, parse_expression(parser));

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static bool is_declspec_token(token_type_t type)
{
    return type == TOKEN_KW_INT || type == TOKEN_KW_VOID || type == TOKEN_KW_STATIC || type == TOKEN_KW_CONST;
}

static declspec_t parse_declspec(parser_state_t *parser)
{
    declspec_t res = {0};

    res.type = NULL;

    while (is_declspec_token(parser_peek(parser)->type))
    {
        if (parser_test(parser, TOKEN_KW_INT))
        {
            res.type = &CTYPE_BUILTIN_INT;
        }
        else if (parser_test(parser, TOKEN_KW_VOID))
        {
            res.type = &CTYPE_BUILTIN_VOID;
        }
        else if (parser_test(parser, TOKEN_KW_STATIC))
        {
            res.is_static = true;
        }
        else if (parser_test(parser, TOKEN_KW_CONST))
        {
            res.is_const = true;
        }

        parser_next(parser);
    }

    return res;
}

static ast_node_t *parse_declaration(parser_state_t *parser)
{
    declspec_t declspec = parse_declspec(parser);

    if (declspec.type == NULL)
    {
        return parse_assign_statement(parser);
    }

    ast_node_t *node = parser_make_node(parser, AST_DECLARATION);

    node->meta.declaration.type = declspec.type;
    node->meta.declaration.id = parser_expect(parser, TOKEN_IDENTIFIER);

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ast_node_t *parse_if_statement(parser_state_t *parser)
{
    if (!parser_expect(parser, TOKEN_KW_IF))
        return NULL;

    if (!parser_expect(parser, TOKEN_LPAREN))
        return NULL;

    ast_node_t *node = parser_make_node(parser, AST_IF_STATEMENT);

    ast_node_t *condition = parse_expression(parser);

    if (!condition)
        return NULL;

    ast_add_child(node, condition);

    if (!parser_expect(parser, TOKEN_RPAREN))
        return NULL;

    ast_node_t *then_branch = parse_compound_statement(parser);

    if (!then_branch)
        return NULL;

    ast_add_child(node, then_branch);

    if (parser_test(parser, TOKEN_KW_ELSE))
    {
        parser_next(parser);

        ast_node_t *else_branch = parse_compound_statement(parser);

        if (!else_branch)
            return NULL;

        ast_add_child(node, else_branch);
    }

    return node;
}

static ast_node_t *parse_while_statement(parser_state_t *parser)
{
    if (!parser_expect(parser, TOKEN_KW_WHILE))
        return NULL;

    if (!parser_expect(parser, TOKEN_LPAREN))
        return NULL;

    ast_node_t *node = parser_make_node(parser, AST_WHILE_STATEMENT);

    ast_node_t *condition = parse_expression(parser);

    if (!condition)
        return NULL;

    ast_add_child(node, condition);

    if (!parser_expect(parser, TOKEN_RPAREN))
        return NULL;

    ast_node_t *body = parse_compound_statement(parser);

    if (!body)
        return NULL;

    ast_add_child(node, body);

    return node;
}

static ast_node_t *parse_compound_statement(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_COMPOUND_STATEMENT);

    parser_expect(parser, TOKEN_LBRACE);

    while (!parser_is_eof(parser) && !parser_test(parser, TOKEN_RBRACE))
    {
        ast_node_t *stmt = NULL;

        if (parser_test(parser, TOKEN_KW_RETURN))
        {
            stmt = parse_return_statement(parser);
        }
        else if (parser_test(parser, TOKEN_KW_IF))
        {
            stmt = parse_if_statement(parser);
        }
        else if (parser_test(parser, TOKEN_KW_WHILE))
        {
            stmt = parse_while_statement(parser);
        }
        else
        {
            stmt = parse_declaration(parser);
        }

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

    declspec_t dtype = parse_declspec(parser);

    node->meta.function.return_type = dtype.type;

    const token_t *id_token = parser_expect(parser, TOKEN_IDENTIFIER);

    node->meta.function.name = extract_token_as_new_string(id_token);
    node->meta.function.is_inline = false;
    node->meta.function.is_static = dtype.is_static;

    parser_expect(parser, TOKEN_LPAREN);
    // TODO: parse parameters
    parser_expect(parser, TOKEN_RPAREN);

    return node;
}

static ast_node_t *parse_function_definition(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_FUNCTION_DEFINITION);

    /*TODO: implement type parsing*/

    declspec_t dtype = parse_declspec(parser);

    node->meta.function.return_type = dtype.type;

    const token_t *id_token = parser_expect(parser, TOKEN_IDENTIFIER);

    node->meta.function.name = extract_token_as_new_string(id_token);
    node->meta.function.is_inline = false;
    node->meta.function.is_static = dtype.is_static;

    parser_expect(parser, TOKEN_LPAREN);
    // TODO: parse parameters
    parser_expect(parser, TOKEN_RPAREN);

    // ast_node_t *signature = parse_function_signature(parser);
    ast_node_t *body = parse_compound_statement(parser);

    // ast_add_child(node, signature);
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
