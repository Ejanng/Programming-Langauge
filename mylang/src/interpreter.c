// Stage 3: the tree-walking interpreter. It runs the lexer and parser, then
// evaluates the AST node by node, keeping variables in a chain of scopes (ObjEnv).
#include "interpreter.h"

#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "errors.h"
#include "lexer.h"
#include "parser.h"

#define MAX_CALL_DEPTH 1000

static void* heapReserveArray(void* array, int count, int* capacity, size_t elementSize) {
    if (count < *capacity) return array;
    *capacity = *capacity < 16 ? 16 : *capacity * 2;
    array = realloc(array, (size_t)*capacity * elementSize);
    if (!array) {
        fputs("Out of memory.\n", stderr);
        exit(EXIT_FAILURE);
    }
    return array;
}

static Value evaluate(Interpreter* interpreter, const Expr* expression);
static bool execute(Interpreter* interpreter, const Stmt* statement);

// ---- statements ----

// Runs `count` statements in `scope`, then frees the scope unless a function captured it.
// Returns true if one of the statements executed a `report`.
// The previous scope is restored when the block ends (after a runtime error, interpreterRun resets it).
static bool executeBlock(Interpreter* interpreter, Stmt* const* statements, int count, ObjEnv* scope) {
    ObjEnv* saved = interpreter->environment;
    interpreter->environment = scope;
    interpreter->live = heapReserveArray(interpreter->live, interpreter->liveCount, &interpreter->liveCapacity, sizeof(ObjEnv*));
    interpreter->live[interpreter->liveCount++] = scope;
    bool returned = false;
    for (int i = 0; i < count && !returned; i++) returned = execute(interpreter, statements[i]);
    interpreter->liveCount--;
    interpreter->environment = saved;
    environmentRelease(scope);
    return returned;
}

// Returns true if a `report` statement ran, so the enclosing loops and blocks stop and the call unwinds.
static bool execute(Interpreter* interpreter, const Stmt* statement) {
    switch (statement->kind) {
        case STMT_EXPRESSION: evaluate(interpreter, statement->expression); break;
        case STMT_PRINT: {
            ObjString* text = valueToString(evaluate(interpreter, statement->expression));
            fwrite(text->characters, 1, text->length, stdout);
            fputc('\n', stdout);
            break;
        }
        case STMT_LET: environmentDefine(interpreter->environment, statement->name, statement->expression ? evaluate(interpreter, statement->expression) : nilValue()); break;
        case STMT_BLOCK: return executeBlock(interpreter, statement->body, statement->bodyCount, environmentNew(interpreter->environment));
        case STMT_IF:
            if (isTruthy(evaluate(interpreter, statement->expression))) return execute(interpreter, statement->thenBranch);
            if (statement->elseBranch) return execute(interpreter, statement->elseBranch);
            break;
        case STMT_WHILE:
            while (isTruthy(evaluate(interpreter, statement->expression))) {
                if (execute(interpreter, statement->thenBranch)) return true;
            }
            break;
        case STMT_FUNCTION: environmentDefine(interpreter->environment, statement->name, objectValue(functionNew(statement, interpreter->environment))); break;
        case STMT_RETURN:
            if (interpreter->callDepth == 0) languageError(ERR_RUNTIME, statement->line, "Can't report from top-level code.");
            interpreter->returnValue = statement->expression ? evaluate(interpreter, statement->expression) : nilValue();
            return true;
    }
    return false;
}

// ---- expressions ----

static Value evaluateUnary(Interpreter* interpreter, const Expr* expression) {
    Value operand = evaluate(interpreter, expression->left);
    if (expression->operatorType == TOK_BANG) return booleanValue(!isTruthy(operand));
    if (operand.type != VAL_NUMBER) languageError(ERR_RUNTIME, expression->line, "Operand of '-' must be a number.");
    return numberValue(-operand.as.number);
}

static Value evaluateBinary(Interpreter* interpreter, const Expr* expression) {
    Value left = evaluate(interpreter, expression->left);
    Value right = evaluate(interpreter, expression->right);

    if (expression->operatorType == TOK_EQUAL_EQUAL) return booleanValue(valuesEqual(left, right));
    if (expression->operatorType == TOK_BANG_EQUAL) return booleanValue(!valuesEqual(left, right));

    if (expression->operatorType == TOK_PLUS) {
        if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) return numberValue(left.as.number + right.as.number);
        if (isObjectType(left, OBJ_STRING) || isObjectType(right, OBJ_STRING)) {
            // string + anything joins them as text
            return objectValue(stringConcatenate(valueToString(left), valueToString(right)));
        }
        languageError(ERR_RUNTIME, expression->line, "Operands of '+' must be two numbers or include a string.");
    }

    if (left.type != VAL_NUMBER || right.type != VAL_NUMBER) languageError(ERR_RUNTIME, expression->line, "Operands must be numbers.");
    double leftNumber = left.as.number, rightNumber = right.as.number;

    switch (expression->operatorType) {
        case TOK_MINUS: return numberValue(leftNumber - rightNumber);
        case TOK_STAR: return numberValue(leftNumber * rightNumber);
        case TOK_SLASH:
            if (rightNumber == 0) languageError(ERR_RUNTIME, expression->line, "Division by zero.");
            return numberValue(leftNumber / rightNumber);
        case TOK_PERCENT:
            if (rightNumber == 0) languageError(ERR_RUNTIME, expression->line, "Division by zero.");
            return numberValue(fmod(leftNumber, rightNumber));
        case TOK_GREATER: return booleanValue(leftNumber > rightNumber);
        case TOK_GREATER_EQUAL: return booleanValue(leftNumber >= rightNumber);
        case TOK_LESS: return booleanValue(leftNumber < rightNumber);
        case TOK_LESS_EQUAL: return booleanValue(leftNumber <= rightNumber);
        default: break;
    }
    languageError(ERR_RUNTIME, expression->line, "Unknown operator.");
}

static Value evaluateCall(Interpreter* interpreter, const Expr* expression) {
    Value callee = evaluate(interpreter, expression->left);
    // Most calls pass a few arguments, so they fit in a small buffer on the C stack. Longer argument
    // lists (the parser allows up to 255) go to the heap. A variable-length array would be simpler,
    // but MSVC does not support them.
    Value small[8];
    Value* arguments = expression->argumentCount <= 8 ? small : malloc(sizeof(Value) * (size_t)expression->argumentCount);
    if (!arguments) languageError(ERR_RUNTIME, expression->line, "Out of memory.");
    for (int i = 0; i < expression->argumentCount; i++) arguments[i] = evaluate(interpreter, expression->arguments[i]);

    if (!isObjectType(callee, OBJ_FUNCTION)) languageError(ERR_RUNTIME, expression->line, "Can only call functions.");
    const ObjFunction* function = (const ObjFunction*)callee.as.object;
    const Stmt* declaration = function->declaration;

    if (expression->argumentCount != declaration->parameterCount) {
        languageError(ERR_RUNTIME, expression->line, "Expected %d argument%s but got %d.", declaration->parameterCount, declaration->parameterCount == 1 ? "" : "s", expression->argumentCount);
    }
    if (interpreter->callDepth >= MAX_CALL_DEPTH) languageError(ERR_RUNTIME, expression->line, "Stack overflow: too many nested calls.");

    ObjEnv* scope = environmentNew(function->closure);
    for (int i = 0; i < expression->argumentCount; i++) environmentDefine(scope, declaration->parameters[i], arguments[i]);
    if (arguments != small) free(arguments);

    interpreter->callDepth++;
    interpreter->returnValue = nilValue();  // functions without a return give nil
    executeBlock(interpreter, declaration->body, declaration->bodyCount, scope);
    interpreter->callDepth--;
    Value result = interpreter->returnValue;
    interpreter->returnValue = nilValue();
    return result;
}

static Value evaluate(Interpreter* interpreter, const Expr* expression) {
    switch (expression->kind) {
        case EXPR_NUMBER: return numberValue(expression->number);
        case EXPR_STRING: return objectValue(stringCopy(expression->text, expression->textLength));
        case EXPR_BOOL: return booleanValue(expression->boolean);
        case EXPR_NIL: return nilValue();
        case EXPR_VARIABLE: {
            const Value* value = environmentLookup(interpreter->environment, expression->text);
            if (!value) languageError(ERR_RUNTIME, expression->line, "Undefined variable '%s'.", expression->text);
            return *value;
        }
        case EXPR_ASSIGN: {
            Value value = evaluate(interpreter, expression->left);
            if (!environmentAssign(interpreter->environment, expression->text, value)) languageError(ERR_RUNTIME, expression->line, "Undefined variable '%s'.", expression->text);
            return value;
        }
        case EXPR_UNARY: return evaluateUnary(interpreter, expression);
        case EXPR_BINARY: return evaluateBinary(interpreter, expression);
        case EXPR_LOGICAL: {
            Value left = evaluate(interpreter, expression->left);
            if (expression->operatorType == TOK_OR) {
                if (isTruthy(left)) return left;
            } else {
                if (!isTruthy(left)) return left;
            }
            return evaluate(interpreter, expression->right);
        }
        case EXPR_CALL: return evaluateCall(interpreter, expression);
    }
    return nilValue();
}

// ---- running source ----

void interpreterInit(Interpreter* interpreter) {
    *interpreter = (Interpreter){.globals = environmentNew(NULL), .returnValue = nilValue()};
    environmentCapture(interpreter->globals);  // globals live until exit
    interpreter->environment = interpreter->globals;
}

void interpreterFree(Interpreter* interpreter) { free(interpreter->live); }

// Lexes and parses. Returns the program, or NULL after reporting a syntax error.
static Program* parseSource(const char* source, size_t length) {
    if (setjmp(errorState.jumpBuffer) != 0) {
        reportError();
        return NULL;
    }
    int count;
    Token* tokens = lex(source, length, &count);
    return parse(tokens, count);  // the AST stays alive in the arena: functions point into it
}

int interpreterRun(Interpreter* interpreter, const char* source, size_t length) {
    Program* program = parseSource(source, length);
    if (!program) return EXIT_SYNTAX_ERROR;

    if (setjmp(errorState.jumpBuffer) != 0) {
        reportError();
        for (int i = 0; i < interpreter->liveCount; i++) environmentRelease(interpreter->live[i]);
        interpreter->liveCount = 0;
        interpreter->environment = interpreter->globals;
        interpreter->callDepth = 0;
        return EXIT_RUNTIME_ERROR;
    }
    for (int i = 0; i < program->count; i++) execute(interpreter, program->statements[i]);
    return EXIT_OK;
}
