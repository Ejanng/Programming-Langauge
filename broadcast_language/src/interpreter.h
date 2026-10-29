#pragma once
// Stage 3: a tree-walking interpreter. It evaluates the AST directly.
#include <stddef.h>

#include "value.h"

typedef struct {
    ObjEnv* globals;              // top-level scope; lives for the whole session
    ObjEnv* environment;          // the scope currently executing
    ObjEnv** live;                // scopes of the calls and blocks in progress, so an error can free them
    int liveCount, liveCapacity;  // number of entries in live, and slots allocated
    int callDepth;                // how many function calls are in progress, to catch runaway recursion
    Value returnValue;            // set by a `report` statement while the call unwinds
} Interpreter;

// Sets up an interpreter with an empty global scope.
void interpreterInit(Interpreter* interpreter);
// Releases the interpreter's own buffers. Scopes and other objects are freed by objectsFreeAll().
void interpreterFree(Interpreter* interpreter);

// Runs a piece of source. Returns 0 on success, 65 for a syntax error, 70 for a runtime error.
// Global variables and functions persist between calls, which is what the REPL relies on.
int interpreterRun(Interpreter* interpreter, const char* source, size_t length);
