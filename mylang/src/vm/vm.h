#pragma once
// Bytecode backend, part 3: the virtual machine that runs compiled bytecode.
//
// One value stack is shared by every call. A call frame records which closure is running,
// its instruction pointer, and `base`, the stack index of its slot 0 (the callee itself).
// Arguments are already on the stack when a call starts, so they become locals 1..arity for free.
#include <stdbool.h>
#include <stddef.h>

#include "../value.h"
#include "chunk.h"

typedef struct {
    ObjClosure* closure;  // the function being run
    size_t ip;            // index of the next instruction to execute in closure->proto->code
    size_t base;          // stack index of this call's slot 0 (the callee; arguments and locals follow)
} Frame;

typedef struct {
    bool dumpBytecode;            // print the disassembled bytecode before running (--dump)
    Value* stack;                 // the value stack shared by all calls
    size_t stackCount, stackCap;  // values on the stack, and slots allocated
    Frame* frames;                // one frame per call in progress; the last is the running one
    size_t frameCount, frameCap;  // frames in use, and slots allocated
    ObjUpvalue** openUpvalues;  // captured variables that are still on the stack
    size_t openCount, openCap;    // open upvalues, and slots allocated
    Table globals;                // global variables and functions, by name
} VM;

// Sets up an empty VM. With dumpBytecode, each program's bytecode is printed before it runs.
void vm_init(VM* vm, bool dumpBytecode);
// Releases the VM's stacks and global table. Heap objects are freed by objects_free_all().
void vm_free(VM* vm);

// Runs a piece of source. Returns 0 on success, 65 for a syntax or compile error, 70 for a runtime error.
// Globals persist between calls, which is what the REPL relies on.
int vm_run(VM* vm, const char* source, size_t length);
