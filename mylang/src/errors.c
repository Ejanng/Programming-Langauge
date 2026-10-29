// Error reporting: languageError() records an error and jumps back to the phase that is running;
// reportError() prints it as "[line N] Kind error: message" plus the source line. See errors.h.
#include "errors.h"

#include <stdarg.h>
#include <stdio.h>

ErrorState errorState;

// The source text currently running, set by setErrorSource().
static const char* currentSource = NULL;
static size_t currentSourceLength = 0;
static bool sourceIsWholeProgram = false;  // false in the REPL: runtime line numbers may refer to earlier input

// Records where the error is (the message is already in errorState) and jumps back to the setjmp.
static _Noreturn void raiseError(ErrorKind kind, int line, long offset) {
    errorState.kind = kind;
    errorState.line = line;
    errorState.offset = offset;
    longjmp(errorState.jumpBuffer, 1);
}

void languageError(ErrorKind kind, int line, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(errorState.message, sizeof errorState.message, format, arguments);
    va_end(arguments);
    raiseError(kind, line, -1);
}

void languageErrorAt(ErrorKind kind, int line, long offset, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(errorState.message, sizeof errorState.message, format, arguments);
    va_end(arguments);
    raiseError(kind, line, offset);
}

void setErrorSource(const char* source, size_t length, bool wholeProgram) {
    currentSource = source;
    currentSourceLength = length;
    sourceIsWholeProgram = wholeProgram;
}

// Prints source line `line` with a "  N | " gutter, and a ^ under byte `offset` if it is on that line.
static void printSourceLine(int line, long offset) {
    // Find where the line starts by counting newlines.
    size_t start = 0;
    for (int current = 1; current < line; current++) {
        while (start < currentSourceLength && currentSource[start] != '\n') start++;
        if (start >= currentSourceLength) return;  // line is past the end: nothing to show
        start++;
    }
    size_t end = start;
    while (end < currentSourceLength && currentSource[end] != '\n' && currentSource[end] != '\r') end++;

    int width = snprintf(NULL, 0, "%d", line);
    fprintf(stderr, "    %d | %.*s\n", line, (int)(end - start), currentSource + start);
    if (offset < (long)start || offset > (long)end) return;
    // Copy tabs from the source line so the ^ lines up however wide the terminal shows a tab.
    fprintf(stderr, "    %*s | ", width, "");
    for (size_t i = start; i < (size_t)offset; i++) fputc(currentSource[i] == '\t' ? '\t' : ' ', stderr);
    fputs("^\n", stderr);
}

void reportError(void) {
    const char* kind = errorState.kind == ERR_SYNTAX ? "Syntax error" : "Runtime error";
    fflush(stdout);
    fprintf(stderr, "[line %d] %s: %s\n", errorState.line, kind, errorState.message);
    bool sourceMatches = errorState.kind != ERR_RUNTIME || sourceIsWholeProgram;
    if (currentSource && sourceMatches && errorState.line > 0) printSourceLine(errorState.line, errorState.offset);
}
