#pragma once
// C has no exceptions, so errors use setjmp/longjmp. A phase that can fail calls setjmp(errorState.jumpBuffer)
// first; languageError() fills in errorState and jumps back there. Every error carries the source line
// so messages can point at the problem.
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ERR_SYNTAX,   // raised by the lexer and parser
    ERR_RUNTIME,  // raised while a program is running
} ErrorKind;

typedef struct {
    jmp_buf jumpBuffer;  // where languageError() jumps back to (set by setjmp in the running phase)
    ErrorKind kind;      // which phase raised the error
    int line;            // source line the error points at
    long offset;         // byte offset in the source of the exact spot, or -1 when only the line is known
    char message[512];   // the formatted error message, without the "[line N]" prefix
} ErrorState;

extern ErrorState errorState;

#if defined(__GNUC__)
#define PRINTF_LIKE(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define PRINTF_LIKE(fmt, args)
#endif

// Raises an error that points at a whole line (used for runtime errors).
_Noreturn void languageError(ErrorKind kind, int line, const char* format, ...) PRINTF_LIKE(3, 4);

// Raises an error that points at one spot: `offset` is a byte offset into the source (used by the
// lexer and parser, which know exactly which character or token is wrong).
_Noreturn void languageErrorAt(ErrorKind kind, int line, long offset, const char* format, ...) PRINTF_LIKE(4, 5);

// Tells the error reporter which source text is running, so it can show the line an error is on.
// `wholeProgram` is true for a file. It is false in the REPL, where a runtime error can come from a
// function typed on an earlier line, so only syntax errors (always in the current line) show source.
void setErrorSource(const char* source, size_t length, bool wholeProgram);

// Prints the error in errorState to stderr, followed by the source line and a ^ under the exact spot
// when they are known:
//
//   [line 4] Syntax error: Expect expression. (found '*')
//       4 | print (3 + * 4;
//         |            ^
//
// Flushes stdout first so program output stays ahead of the error when both go to one stream.
void reportError(void);

// Process exit codes.
enum { EXIT_OK = 0, EXIT_USAGE = 64, EXIT_SYNTAX_ERROR = 65, EXIT_RUNTIME_ERROR = 70, EXIT_NO_FILE = 74 };
