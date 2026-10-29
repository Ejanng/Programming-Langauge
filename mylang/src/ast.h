#pragma once
// The abstract syntax tree (AST): what the parser produces and the interpreter walks.
// Each node is one struct with a `kind` tag. It is compact and easy to read.
// A good later experiment is replacing the one-struct-fits-all node with a tagged union.
// All nodes live in the arena (see arena.h), so there is nothing to free node by node.
#include <stdbool.h>
#include <stddef.h>

#include "token.h"

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef enum { EXPR_NUMBER, EXPR_STRING, EXPR_BOOL, EXPR_NIL, EXPR_VARIABLE, EXPR_ASSIGN, EXPR_UNARY, EXPR_BINARY, EXPR_LOGICAL, EXPR_CALL } ExprKind;

struct Expr {
    ExprKind kind;           // which kind of expression this node is; decides which fields below are used
    int line;                // source line, for error messages
    double number;           // EXPR_NUMBER
    bool boolean;            // EXPR_BOOL
    const char* text;        // string literal, or the name for EXPR_VARIABLE and EXPR_ASSIGN
    size_t textLength;       // length of text
    TokenType operatorType;  // EXPR_UNARY, EXPR_BINARY, EXPR_LOGICAL
    Expr* left;              // unary operand, binary/logical left, assign value, call callee
    Expr* right;             // binary/logical right
    Expr** arguments;        // call arguments
    int argumentCount;       // number of entries in arguments
};

typedef enum { STMT_EXPRESSION, STMT_PRINT, STMT_LET, STMT_BLOCK, STMT_IF, STMT_WHILE, STMT_FUNCTION, STMT_RETURN } StmtKind;

struct Stmt {
    StmtKind kind;            // which kind of statement this node is; decides which fields below are used
    int line;                 // source line, for error messages
    const char* name;         // STMT_LET, STMT_FUNCTION
    Expr* expression;         // expression, print, let initializer, if/while condition, return value (may be NULL)
    Stmt** body;              // block statements, function body
    int bodyCount;            // number of entries in body
    Stmt* thenBranch;         // when "then" branch, repeat body
    Stmt* elseBranch;         // when "otherwise" branch (may be NULL)
    const char** parameters;  // function parameters
    int parameterCount;       // number of entries in parameters
};

// A parsed program: the top-level statements, in order.
typedef struct {
    Stmt** statements;  // the statements, in source order
    int count;          // number of entries in statements
} Program;
