#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "token.hpp"
#include "value.hpp"

namespace rene {

// Syntax tree (AST) nodes. The parser builds a tree of these from the tokens,
// and the interpreter walks the tree to run the program.

// ---------------------------------------------------------------- Expressions

// Base class of every expression (code that produces a value).
struct Expr {
    // Source line of the expression, used in error messages.
    int line = 0;
    explicit Expr(int l) : line(l) {}
    virtual ~Expr() = default;
};
using ExprPtr = std::shared_ptr<Expr>;

// Base class of every statement (code that does something, like a loop).
struct Stmt {
    // Source line of the statement, used in error messages.
    int line = 0;
    explicit Stmt(int l) : line(l) {}
    virtual ~Stmt() = default;
};
using StmtPtr = std::shared_ptr<Stmt>;

// A fixed value written in the code: 42, "hi", true, nil.
struct LiteralExpr : Expr {
    // The value itself.
    Value value;
    LiteralExpr(Value v, int l) : Expr(l), value(std::move(v)) {}
};

// Reading a variable: x
struct VariableExpr : Expr {
    // Name of the variable.
    std::string name;
    VariableExpr(std::string n, int l) : Expr(l), name(std::move(n)) {}
};

// Assigning to a variable: x = value
struct AssignExpr : Expr {
    // Name of the variable being assigned.
    std::string name;
    // Expression whose result is stored.
    ExprPtr value;
    AssignExpr(std::string n, ExprPtr v, int l) : Expr(l), name(std::move(n)), value(std::move(v)) {}
};

// An operator with one operand: -x or not x
struct UnaryExpr : Expr {
    // The operator (Minus or Not).
    TokenType op;
    // The operand.
    ExprPtr right;
    UnaryExpr(TokenType o, ExprPtr r, int l) : Expr(l), op(o), right(std::move(r)) {}
};

// An operator with two operands: a + b, a < b, a == b, ...
struct BinaryExpr : Expr {
    // Left operand.
    ExprPtr left;
    // The operator (Plus, Star, Less, ...).
    TokenType op;
    // Right operand.
    ExprPtr right;
    BinaryExpr(ExprPtr a, TokenType o, ExprPtr b, int l)
        : Expr(l), left(std::move(a)), op(o), right(std::move(b)) {}
};

// `and` / `or` short-circuit, so they get their own node.
struct LogicalExpr : Expr {
    // Left operand, always evaluated.
    ExprPtr left;
    // And or Or.
    TokenType op;
    // Right operand, evaluated only when the left side doesn't decide the result.
    ExprPtr right;
    LogicalExpr(ExprPtr a, TokenType o, ExprPtr b, int l)
        : Expr(l), left(std::move(a)), op(o), right(std::move(b)) {}
};

// A function call: f(a, b)
struct CallExpr : Expr {
    // Expression that gives the function (usually just its name).
    ExprPtr callee;
    // Argument expressions, in order.
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr c, std::vector<ExprPtr> a, int l) : Expr(l), callee(std::move(c)), args(std::move(a)) {}
};

// A list literal: [1, 2, 3]
struct ListExpr : Expr {
    // Expressions for each element.
    std::vector<ExprPtr> elements;
    ListExpr(std::vector<ExprPtr> e, int l) : Expr(l), elements(std::move(e)) {}
};

// Reading an element: list[i] or string[i]
struct IndexExpr : Expr {
    // The list or string being indexed.
    ExprPtr object;
    // Position to read (negative counts from the end).
    ExprPtr index;
    IndexExpr(ExprPtr o, ExprPtr i, int l) : Expr(l), object(std::move(o)), index(std::move(i)) {}
};

// Assigning to an element: list[i] = value
struct IndexAssignExpr : Expr {
    // The list being changed.
    ExprPtr object;
    // Position to change.
    ExprPtr index;
    // New value for the element.
    ExprPtr value;
    IndexAssignExpr(ExprPtr o, ExprPtr i, ExprPtr v, int l)
        : Expr(l), object(std::move(o)), index(std::move(i)), value(std::move(v)) {}
};

// Defined further down; lambdas reuse it to hold their parameters and body.
struct FunctionStmt;

// Anonymous function: fn (a, b) { ... }
struct LambdaExpr : Expr {
    // Parameters and body of the function.
    std::shared_ptr<FunctionStmt> function;
    LambdaExpr(std::shared_ptr<FunctionStmt> f, int l) : Expr(l), function(std::move(f)) {}
};

// ----------------------------------------------------------------- Statements

// An expression used as a statement, e.g. a call: greet("Ada");
struct ExpressionStmt : Stmt {
    // The expression to evaluate (its result is discarded).
    ExprPtr expression;
    ExpressionStmt(ExprPtr e, int l) : Stmt(l), expression(std::move(e)) {}
};

// print a, b, c;  prints the values separated by spaces.
struct PrintStmt : Stmt {
    // Expressions to print; may be empty (prints a blank line).
    std::vector<ExprPtr> values;
    PrintStmt(std::vector<ExprPtr> v, int l) : Stmt(l), values(std::move(v)) {}
};

// Variable declaration: let x = value;
struct LetStmt : Stmt {
    // Name of the new variable.
    std::string name;
    ExprPtr initializer;  // may be null -> nil
    LetStmt(std::string n, ExprPtr i, int l) : Stmt(l), name(std::move(n)), initializer(std::move(i)) {}
};

// A { ... } block, which runs its statements in a new scope.
struct BlockStmt : Stmt {
    // Statements inside the braces, in order.
    std::vector<StmtPtr> statements;
    BlockStmt(std::vector<StmtPtr> s, int l) : Stmt(l), statements(std::move(s)) {}
};

// if condition { ... } else { ... }
struct IfStmt : Stmt {
    // Decides which branch runs.
    ExprPtr condition;
    // Runs when the condition is truthy.
    StmtPtr thenBranch;
    StmtPtr elseBranch;  // may be null
    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e, int l)
        : Stmt(l), condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
};

// while condition { ... }
struct WhileStmt : Stmt {
    // Checked before every pass through the loop.
    ExprPtr condition;
    // The loop body.
    StmtPtr body;
    WhileStmt(ExprPtr c, StmtPtr b, int l) : Stmt(l), condition(std::move(c)), body(std::move(b)) {}
};

// for item in iterable { ... }
struct ForInStmt : Stmt {
    // Name that holds the current item on each pass.
    std::string variable;
    // The list or string to loop over.
    ExprPtr iterable;
    // The loop body.
    StmtPtr body;
    ForInStmt(std::string v, ExprPtr i, StmtPtr b, int l)
        : Stmt(l), variable(std::move(v)), iterable(std::move(i)), body(std::move(b)) {}
};

// break;  leaves the innermost loop.
struct BreakStmt : Stmt {
    explicit BreakStmt(int l) : Stmt(l) {}
};

// continue;  skips to the next pass of the innermost loop.
struct ContinueStmt : Stmt {
    explicit ContinueStmt(int l) : Stmt(l) {}
};

// Named function declaration: fn name(a, b) { ... }
struct FunctionStmt : Stmt {
    // Function name ("<anonymous>" for lambdas).
    std::string name;
    // Parameter names, in order.
    std::vector<std::string> params;
    // Statements in the function body.
    std::vector<StmtPtr> body;
    FunctionStmt(std::string n, std::vector<std::string> p, std::vector<StmtPtr> b, int l)
        : Stmt(l), name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
};

// return value;  ends the current function.
struct ReturnStmt : Stmt {
    ExprPtr value;  // may be null -> nil
    ReturnStmt(ExprPtr v, int l) : Stmt(l), value(std::move(v)) {}
};

}  // namespace rene
