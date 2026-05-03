#include <tmcc/parser.h>
#include <tmcc/types.h>
#include <tmcc/tokens.h>

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

static bool parser_test_nth(parser_state_t *parser, token_type_t type, size_t n)
{
    if (parser->token_index + n >= parser->lexer->token_count)
        return false;

    const token_t *token = &parser->lexer->tokens[parser->token_index + n];
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
static ast_node_t *parse_shift_expression(parser_state_t *parser);
static ctype_t *parse_declarator(parser_state_t *parser, ctype_t *tt);
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

    ast_node_t *left = parse_shift_expression(parser);

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

        ast_node_t *right = parse_shift_expression(parser);
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

static ast_node_t *parse_shift_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_additive_expression(parser);

    ast_add_child(node, left);

    while (parser_test(parser, TOKEN_LT) || parser_test(parser, TOKEN_GT))
    {
        const token_t *f1_token = parser_peek(parser);

        ast_node_t *right = NULL;

        if (f1_token->type == TOKEN_LT && parser_test_nth(parser, TOKEN_LT, 1))
        {
            parser_next(parser);
            parser_next(parser);
            node->meta.binary_op.op_type = AST_BIN_OP_SHL;
            right = parse_additive_expression(parser);
        }
        else if (f1_token->type == TOKEN_GT && parser_test_nth(parser, TOKEN_GT, 1))
        {
            parser_next(parser);
            parser_next(parser);
            node->meta.binary_op.op_type = AST_BIN_OP_SHR;
            right = parse_additive_expression(parser);
        }
        else
        {
            break;
        }

        if (right)
        {
            ast_add_child(node, right);
        }
    }

    if (node->children_count == 1)
    {
        ast_free(node);
        return left;
    }

    return node;
}

static ast_node_t *parse_binary_and_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_equality_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    while (parser_test(parser, TOKEN_AMPERSAND) && !parser_test_nth(parser, TOKEN_AMPERSAND, 1))
    {
        parser_expect(parser, TOKEN_AMPERSAND);

        node->meta.binary_op.op_type = AST_BIN_OP_BITAND;

        ast_node_t *right = parse_equality_expression(parser);

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

static ast_node_t *parse_binary_or_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_binary_and_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    while (parser_test(parser, TOKEN_PIPE) && !parser_test_nth(parser, TOKEN_PIPE, 1))
    {
        parser_expect(parser, TOKEN_PIPE);

        node->meta.binary_op.op_type = AST_BIN_OP_BITOR;

        ast_node_t *right = parse_binary_and_expression(parser);

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

static ast_node_t *parse_binary_xor_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_binary_or_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    while (parser_test(parser, TOKEN_CARET))
    {
        parser_expect(parser, TOKEN_CARET);

        node->meta.binary_op.op_type = AST_BIN_OP_BITXOR;

        ast_node_t *right = parse_binary_or_expression(parser);

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

static ast_node_t *parse_logical_and_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_binary_xor_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    while (parser_test(parser, TOKEN_AMPERSAND) && parser_test_nth(parser, TOKEN_AMPERSAND, 1))
    {
        parser_expect(parser, TOKEN_AMPERSAND);
        parser_expect(parser, TOKEN_AMPERSAND);

        node->meta.binary_op.op_type = AST_BIN_OP_AND;

        ast_node_t *right = parse_binary_xor_expression(parser);

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

static ast_node_t *parse_logical_or_expression(parser_state_t *parser)
{
    ast_node_t *node = parser_make_node(parser, AST_BINARY_OPERATOR);

    ast_node_t *left = parse_logical_and_expression(parser);

    if (!left)
    {
        return NULL;
    }

    ast_add_child(node, left);

    while (parser_test(parser, TOKEN_PIPE) && parser_test_nth(parser, TOKEN_PIPE, 1))
    {
        parser_expect(parser, TOKEN_PIPE);
        parser_expect(parser, TOKEN_PIPE);

        node->meta.binary_op.op_type = AST_BIN_OP_OR;

        ast_node_t *right = parse_logical_and_expression(parser);

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
    return parse_logical_or_expression(parser);
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

static bool is_declspec_type_specifier_token(token_type_t type)
{
    return type == TOKEN_KW_INT || type == TOKEN_KW_VOID || type == TOKEN_KW_CHAR || type == TOKEN_KW_SHORT || type == TOKEN_KW_LONG || type == TOKEN_KW_FLOAT || type == TOKEN_KW_DOUBLE;
}

static bool is_declspec_type_qualifier_token(token_type_t type)
{
    return type == TOKEN_KW_CONST || type == TOKEN_KW_VOLATILE;
}

static bool is_declspec_storage_class_token(token_type_t type)
{
    return type == TOKEN_KW_AUTO || type == TOKEN_KW_REGISTER || type == TOKEN_KW_STATIC || type == TOKEN_KW_EXTERN || type == TOKEN_KW_TYPEDEF;
}

static ctype_t *parse_declarator_pointer(parser_state_t *parser, ctype_t *base_type)
{
    parser_expect(parser, TOKEN_ASTERISK);

    while (is_declspec_type_qualifier_token(parser_peek(parser)->type))
    {
        // TODO: account for type qualifiers in pointer types
        parser_next(parser);
    }

    ctype_t *new_type = ctype_make_pointer(base_type);

    if (parser_test(parser, TOKEN_ASTERISK))
    {
        return parse_declarator_pointer(parser, new_type);
    }

    return new_type;
}

static ctype_t *parse_direct_declarator(parser_state_t *parser, ctype_t *base_type)
{
    if (parser_test(parser, TOKEN_IDENTIFIER))
    {
        const token_t *id_token = parser_expect(parser, TOKEN_IDENTIFIER);

        base_type->name = extract_token_as_new_string(id_token);
        return base_type;
    }
    else if (parser_test(parser, TOKEN_LPAREN))
    {
        parser_expect(parser, TOKEN_LPAREN);
        base_type = parse_declarator(parser, base_type);
        parser_expect(parser, TOKEN_RPAREN);
        return base_type;
    }
    else if (parser_test(parser, TOKEN_LBRACKET))
    {
        // TODO: parse array constant expression
        return base_type;
    }

    return base_type;
}

static ctype_t *parse_declarator(parser_state_t *parser, ctype_t *base_type)
{
    ctype_t *decl_type = base_type;

    if (parser_test(parser, TOKEN_ASTERISK))
    {
        decl_type = parse_declarator_pointer(parser, decl_type);
    }

    decl_type = parse_direct_declarator(parser, decl_type);

    return decl_type;
}

static declspec_t parse_declspec(parser_state_t *parser)
{
    declspec_t res = {0};

    res.type = NULL;

    int storage_classes_assigned = 0;

    while (
        is_declspec_storage_class_token(parser_peek(parser)->type) ||
        is_declspec_type_specifier_token(parser_peek(parser)->type) ||
        is_declspec_type_qualifier_token(parser_peek(parser)->type))
    {
        const token_t *spec_token = parser_peek(parser);

        // storage class specifier
        if (is_declspec_storage_class_token(parser_peek(parser)->type))
        {
            storage_classes_assigned++;

            if (parser_test(parser, TOKEN_KW_STATIC))
            {
                res.is_static = true;
            }
            else if (parser_test(parser, TOKEN_KW_EXTERN))
            {
                res.is_extern = true;
            }
            else if (parser_test(parser, TOKEN_KW_TYPEDEF))
            {
                res.is_typedef = true;
            }

            parser_next(parser);
            continue;
        }

        // type specifier
        if (parser_test(parser, TOKEN_KW_INT))
        {
            res.type = &CTYPE_BUILTIN_INT;
        }
        else if (parser_test(parser, TOKEN_KW_VOID))
        {
            res.type = &CTYPE_BUILTIN_VOID;
        }
        else if (parser_test(parser, TOKEN_KW_CHAR))
        {
            res.type = &CTYPE_BUILTIN_CHAR;
        }
        else if (parser_test(parser, TOKEN_KW_SHORT))
        {
            res.type = &CTYPE_BUILTIN_SHORT;
        }
        else if (parser_test(parser, TOKEN_KW_LONG))
        {
            res.type = &CTYPE_BUILTIN_LONG;
        }
        else if (parser_test(parser, TOKEN_KW_FLOAT))
        {
            res.type = &CTYPE_BUILTIN_FLOAT;
        }
        else if (parser_test(parser, TOKEN_KW_DOUBLE))
        {
            res.type = &CTYPE_BUILTIN_DOUBLE;
        }

        // type qualifier
        if (parser_test(parser, TOKEN_KW_CONST))
        {
            res.is_const = true;
        }
        else if (parser_test(parser, TOKEN_KW_VOLATILE))
        {
            res.is_volatile = true;
        }

        parser_next(parser);
    }

    if (storage_classes_assigned > 1)
    {
        parser_error(parser, "multiple storage class specifiers in declaration");
    }

    return res;
}

static ast_node_t *parse_declaration(parser_state_t *parser)
{
    declspec_t declspec = parse_declspec(parser);

    ast_node_t *node = NULL;

    if (declspec.type == NULL)
    {
        node = parse_assign_statement(parser);
    }
    else
    {
        node = parser_make_node(parser, AST_DECLARATION);

        ctype_t *decl_type = ctype_clone(declspec.type); // do we need this?

        declspec.type = parse_declarator(parser, decl_type);

        node->meta.declaration.ds = declspec;

        node->meta.declaration.id = node->meta.declaration.ds.type->name;
        // node->meta.declaration.id = parser_expect(parser, TOKEN_IDENTIFIER);
    }

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
