// Stage 2: the recursive descent parser. Each grammar rule in parser.h is one function here,
// and operator precedence comes from which rule calls which.
#include "parser.h"

#include <stdbool.h>
#include <string.h>

#include "arena.h"
#include "errors.h"

typedef struct {
    const Token* tokens;  // the token list from the lexer, ending in TOK_EOF
    int count;            // number of entries in tokens
    int current;          // index of the next token to consume
} Parser;

static Stmt* parseDeclaration(Parser* parser);
static Stmt* parseStatement(Parser* parser);
static Expr* parseExpression(Parser* parser);
static Expr* parseUnary(Parser* parser);

// ---- token helpers ----

static const Token* peek(const Parser* parser) { return &parser->tokens[parser->current]; }
static const Token* previous(const Parser* parser) { return &parser->tokens[parser->current - 1]; }
static bool isAtEnd(const Parser* parser) { return peek(parser)->type == TOK_EOF; }
static bool check(const Parser* parser, TokenType type) { return !isAtEnd(parser) && peek(parser)->type == type; }

static const Token* advance(Parser* parser) {
    if (!isAtEnd(parser)) parser->current++;
    return previous(parser);
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

static bool matchAny(Parser* parser, const TokenType* types, int count) {
    for (int i = 0; i < count; i++) {
        if (match(parser, types[i])) return true;
    }
    return false;
}

_Noreturn static void errorAt(const Token* token, const char* message) {
    if (token->type == TOK_EOF) {
        languageErrorAt(ERR_SYNTAX, token->line, (long)token->offset, "%s (reached end of file)", message);
    }
    languageErrorAt(ERR_SYNTAX, token->line, (long)token->offset, "%s (found '%s')", message, token->lexeme);
}

static const Token* consume(Parser* parser, TokenType type, const char* message) {
    if (check(parser, type)) return advance(parser);
    errorAt(peek(parser), message);
}

// ---- nodes ----

static Expr* newExpression(ExprKind kind, int line) {
    Expr* node = arenaAllocate(sizeof(Expr));
    node->kind = kind;
    node->line = line;
    node->operatorType = TOK_EOF;
    return node;
}

static Stmt* newStatement(StmtKind kind, int line) {
    Stmt* statement = arenaAllocate(sizeof(Stmt));
    statement->kind = kind;
    statement->line = line;
    return statement;
}

// ---- statements ----

// Parses statements up to and including the closing '}' of a block.
static void parseBlockStatements(Parser* parser, Stmt*** body, int* bodyCount) {
    PtrVec statements = {0};
    while (!check(parser, TOK_RIGHT_BRACE) && !isAtEnd(parser)) pointerVectorPush(&statements, parseDeclaration(parser));
    consume(parser, TOK_RIGHT_BRACE, "Expect '}' after block.");
    *body = (Stmt**)statements.items;
    *bodyCount = statements.count;
}

static Stmt* parseFunctionDeclaration(Parser* parser) {
    const Token* name = consume(parser, TOK_IDENTIFIER, "Expect function name after 'protocol'.");
    Stmt* statement = newStatement(STMT_FUNCTION, name->line);
    statement->name = name->lexeme;
    consume(parser, TOK_LEFT_PAREN, "Expect '(' after function name.");
    PtrVec parameters = {0};
    if (!check(parser, TOK_RIGHT_PAREN)) {
        do {
            if (parameters.count >= 255) errorAt(peek(parser), "Can't have more than 255 parameters.");
            pointerVectorPush(&parameters, (void*)consume(parser, TOK_IDENTIFIER, "Expect parameter name.")->lexeme);
        } while (match(parser, TOK_COMMA));
    }
    statement->parameters = (const char**)parameters.items;
    statement->parameterCount = parameters.count;
    consume(parser, TOK_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(parser, TOK_LEFT_BRACE, "Expect '{' before function body.");
    parseBlockStatements(parser, &statement->body, &statement->bodyCount);
    return statement;
}

static Stmt* parseLetDeclaration(Parser* parser) {
    const Token* name = consume(parser, TOK_IDENTIFIER, "Expect variable name after 'assign'.");
    Stmt* statement = newStatement(STMT_LET, name->line);
    statement->name = name->lexeme;
    if (match(parser, TOK_EQUAL)) statement->expression = parseExpression(parser);
    consume(parser, TOK_SEMICOLON, "Expect ';' after variable declaration.");
    return statement;
}

static Stmt* parsePrintStatement(Parser* parser) {
    Stmt* statement = newStatement(STMT_PRINT, previous(parser)->line);
    statement->expression = parseExpression(parser);
    consume(parser, TOK_SEMICOLON, "Expect ';' after value.");
    return statement;
}

static Stmt* parseIfStatement(Parser* parser) {
    Stmt* statement = newStatement(STMT_IF, previous(parser)->line);
    consume(parser, TOK_LEFT_PAREN, "Expect '(' after 'when'.");
    statement->expression = parseExpression(parser);
    consume(parser, TOK_RIGHT_PAREN, "Expect ')' after condition.");
    statement->thenBranch = parseStatement(parser);
    if (match(parser, TOK_ELSE)) statement->elseBranch = parseStatement(parser);
    return statement;
}

static Stmt* parseWhileStatement(Parser* parser) {
    Stmt* statement = newStatement(STMT_WHILE, previous(parser)->line);
    consume(parser, TOK_LEFT_PAREN, "Expect '(' after 'repeat'.");
    statement->expression = parseExpression(parser);
    consume(parser, TOK_RIGHT_PAREN, "Expect ')' after condition.");
    statement->thenBranch = parseStatement(parser);
    return statement;
}

static Stmt* parseReturnStatement(Parser* parser) {
    Stmt* statement = newStatement(STMT_RETURN, previous(parser)->line);
    if (!check(parser, TOK_SEMICOLON)) statement->expression = parseExpression(parser);
    consume(parser, TOK_SEMICOLON, "Expect ';' after report value.");
    return statement;
}

static Stmt* parseExpressionStatement(Parser* parser) {
    Stmt* statement = newStatement(STMT_EXPRESSION, peek(parser)->line);
    statement->expression = parseExpression(parser);
    consume(parser, TOK_SEMICOLON, "Expect ';' after expression.");
    return statement;
}

static Stmt* parseStatement(Parser* parser) {
    if (match(parser, TOK_PRINT)) return parsePrintStatement(parser);
    if (match(parser, TOK_IF)) return parseIfStatement(parser);
    if (match(parser, TOK_WHILE)) return parseWhileStatement(parser);
    if (match(parser, TOK_RETURN)) return parseReturnStatement(parser);
    if (match(parser, TOK_LEFT_BRACE)) {
        Stmt* statement = newStatement(STMT_BLOCK, previous(parser)->line);
        parseBlockStatements(parser, &statement->body, &statement->bodyCount);
        return statement;
    }
    return parseExpressionStatement(parser);
}

static Stmt* parseDeclaration(Parser* parser) {
    if (match(parser, TOK_FN)) return parseFunctionDeclaration(parser);
    if (match(parser, TOK_LET)) return parseLetDeclaration(parser);
    return parseStatement(parser);
}

// ---- expressions ----

// Parses a left-associative chain such as a + b - c using the next-higher precedence level.
static Expr* parseLeftAssociative(Parser* parser, Expr* (*nextRule)(Parser*), const TokenType* operators, int operatorCount, ExprKind kind) {
    Expr* expression = nextRule(parser);
    while (matchAny(parser, operators, operatorCount)) {
        const Token* operatorToken = previous(parser);
        Expr* right = nextRule(parser);
        Expr* node = newExpression(kind, operatorToken->line);
        node->operatorType = operatorToken->type;
        node->left = expression;
        node->right = right;
        expression = node;
    }
    return expression;
}

static Expr* parseFactor(Parser* parser) {
    static const TokenType operators[] = {TOK_SLASH, TOK_STAR, TOK_PERCENT};
    return parseLeftAssociative(parser, parseUnary, operators, 3, EXPR_BINARY);
}

static Expr* parseTerm(Parser* parser) {
    static const TokenType operators[] = {TOK_MINUS, TOK_PLUS};
    return parseLeftAssociative(parser, parseFactor, operators, 2, EXPR_BINARY);
}

static Expr* parseComparison(Parser* parser) {
    static const TokenType operators[] = {TOK_GREATER, TOK_GREATER_EQUAL, TOK_LESS, TOK_LESS_EQUAL};
    return parseLeftAssociative(parser, parseTerm, operators, 4, EXPR_BINARY);
}

static Expr* parseEquality(Parser* parser) {
    static const TokenType operators[] = {TOK_BANG_EQUAL, TOK_EQUAL_EQUAL};
    return parseLeftAssociative(parser, parseComparison, operators, 2, EXPR_BINARY);
}

static Expr* parseLogicAnd(Parser* parser) {
    static const TokenType operators[] = {TOK_AND};
    return parseLeftAssociative(parser, parseEquality, operators, 1, EXPR_LOGICAL);
}

static Expr* parseLogicOr(Parser* parser) {
    static const TokenType operators[] = {TOK_OR};
    return parseLeftAssociative(parser, parseLogicAnd, operators, 1, EXPR_LOGICAL);
}

static Expr* parsePrimary(Parser* parser) {
    if (match(parser, TOK_NUMBER)) {
        Expr* node = newExpression(EXPR_NUMBER, previous(parser)->line);
        node->number = previous(parser)->number;
        return node;
    }
    if (match(parser, TOK_STRING)) {
        Expr* node = newExpression(EXPR_STRING, previous(parser)->line);
        node->text = previous(parser)->lexeme;
        node->textLength = previous(parser)->length;
        return node;
    }
    if (match(parser, TOK_TRUE) || match(parser, TOK_FALSE)) {
        Expr* node = newExpression(EXPR_BOOL, previous(parser)->line);
        node->boolean = previous(parser)->type == TOK_TRUE;
        return node;
    }
    if (match(parser, TOK_NIL)) return newExpression(EXPR_NIL, previous(parser)->line);
    if (match(parser, TOK_IDENTIFIER)) {
        Expr* node = newExpression(EXPR_VARIABLE, previous(parser)->line);
        node->text = previous(parser)->lexeme;
        node->textLength = previous(parser)->length;
        return node;
    }
    if (match(parser, TOK_LEFT_PAREN)) {
        Expr* node = parseExpression(parser);
        consume(parser, TOK_RIGHT_PAREN, "Expect ')' after expression.");
        return node;
    }
    errorAt(peek(parser), "Expect expression.");
}

static Expr* parseCall(Parser* parser) {
    Expr* expression = parsePrimary(parser);
    while (match(parser, TOK_LEFT_PAREN)) {
        Expr* callNode = newExpression(EXPR_CALL, previous(parser)->line);
        callNode->left = expression;
        PtrVec arguments = {0};
        if (!check(parser, TOK_RIGHT_PAREN)) {
            do {
                if (arguments.count >= 255) errorAt(peek(parser), "Can't have more than 255 arguments.");
                pointerVectorPush(&arguments, parseExpression(parser));
            } while (match(parser, TOK_COMMA));
        }
        callNode->arguments = (Expr**)arguments.items;
        callNode->argumentCount = arguments.count;
        consume(parser, TOK_RIGHT_PAREN, "Expect ')' after arguments.");
        expression = callNode;
    }
    return expression;
}

static Expr* parseUnary(Parser* parser) {
    static const TokenType operators[] = {TOK_BANG, TOK_MINUS};
    if (matchAny(parser, operators, 2)) {
        const Token* operatorToken = previous(parser);
        Expr* node = newExpression(EXPR_UNARY, operatorToken->line);
        node->operatorType = operatorToken->type;
        node->left = parseUnary(parser);
        return node;
    }
    return parseCall(parser);
}

static Expr* parseAssignment(Parser* parser) {
    Expr* expression = parseLogicOr(parser);
    if (match(parser, TOK_EQUAL)) {
        const Token* equals = previous(parser);
        Expr* value = parseAssignment(parser);  // right-associative: a = b = 3
        if (expression->kind == EXPR_VARIABLE) {
            Expr* assignNode = newExpression(EXPR_ASSIGN, equals->line);
            assignNode->text = expression->text;
            assignNode->textLength = expression->textLength;
            assignNode->left = value;
            return assignNode;
        }
        languageErrorAt(ERR_SYNTAX, equals->line, (long)equals->offset, "Invalid assignment target.");
    }
    return expression;
}

static Expr* parseExpression(Parser* parser) { return parseAssignment(parser); }

Program* parse(const Token* tokens, int count) {
    Parser parser = {tokens, count, 0};
    PtrVec statements = {0};
    while (!isAtEnd(&parser)) pointerVectorPush(&statements, parseDeclaration(&parser));
    Program* program = arenaAllocate(sizeof(Program));
    program->statements = (Stmt**)statements.items;
    program->count = statements.count;
    return program;
}
