#pragma once
// Bytecode backend, part 1: the instruction set and the compiled form of a function.
//
// Each instruction is an (opcode, operand) pair plus the source line it came from.
// Operands are plain ints: a constant index, a local slot, an upvalue index, a jump
// target (an absolute instruction index), or an argument count, depending on the opcode.
#include <stdbool.h>
#include <stdio.h>

#include "../value.h"

typedef enum {
    OP_CONSTANT,       // push constants[operand]
    OP_NIL, OP_TRUE, OP_FALSE,
    OP_POP,            // discard the top of the stack
    OP_GET_LOCAL,      // push the local in slot `operand` of the current frame
    OP_SET_LOCAL,      // store the top of the stack into local `operand` (value stays on the stack)
    OP_GET_GLOBAL,     // push the global named by constants[operand]
    OP_DEFINE_GLOBAL,  // pop a value and bind it to the global named by constants[operand]
    OP_SET_GLOBAL,     // store the top of the stack into an existing global (value stays on the stack)
    OP_GET_UPVALUE,    // push the captured variable `operand` of the current closure
    OP_SET_UPVALUE,    // store the top of the stack into captured variable `operand`
    OP_EQUAL, OP_NOT_EQUAL, OP_GREATER, OP_GREATER_EQUAL, OP_LESS, OP_LESS_EQUAL,
    OP_ADD, OP_SUBTRACT, OP_MULTIPLY, OP_DIVIDE, OP_MODULO,
    OP_NOT, OP_NEGATE,
    OP_PRINT,          // pop and print
    OP_JUMP,           // ip = operand
    OP_JUMP_IF_FALSE,  // if the top of the stack is falsy, ip = operand (does not pop)
    OP_JUMP_IF_TRUE,   // if the top of the stack is truthy, ip = operand (does not pop)
    OP_CALL,           // call the value below `operand` arguments
    OP_CLOSURE,        // wrap functions[operand] in a closure, capturing its upvalues, and push it
    OP_CLOSE_UPVALUE,  // move the local on top of the stack into its upvalue, then pop it
    OP_RETURN,         // pop the result, drop the frame, push the result for the caller
} Op;

// The readable name of an opcode, as the --dump listing prints it (OP_GET_LOCAL -> "GetLocal").
const char* op_name(Op op);

typedef struct {
    Op op;        // what to do
    int operand;  // its argument; the meaning depends on op (see the Op list above)
    int line;     // source line it was compiled from, for runtime error messages
} Instr;

// Where a closure finds each variable it captures when it is created:
// a local slot of the enclosing function, or one of the enclosing closure's own upvalues.
typedef struct {
    bool isLocal;  // true: a local slot of the enclosing function; false: one of its upvalues
    int index;     // that slot number or upvalue index
} UpvalueRef;

// A compiled function ("prototype"). Closures created at run time share one Proto.
// Protos live in the arena, like the AST they were compiled from.
typedef struct Proto {
    const char* name;        // function name, or "<script>" for the top level
    int arity;               // number of parameters
    Instr* code;             // the instructions
    int codeCount, codeCap;  // instructions used, and slots allocated
    Value* constants;  // numbers, strings, and global names
    int constCount, constCap;        // constants used, and slots allocated
    struct Proto** functions;  // functions declared directly inside this one
    int functionCount, functionCap;  // nested functions, and slots allocated
    UpvalueRef* upvalues;            // where each captured variable comes from when a closure is made
    int upvalueCount, upvalueCap;    // captured variables, and slots allocated
} Proto;

// Prints a readable listing of a function and every function nested inside it.
void disassemble(const Proto* p, FILE* out);
