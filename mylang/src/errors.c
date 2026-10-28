// Error reporting: lang_error() records an error and jumps back to the phase that is running;
// report_error() prints it as "[line N] Kind error: message" plus the source line. See errors.h.
#include "errors.h"

#include <stdarg.h>
#include <stdio.h>

ErrorState g_err;

// The source text currently running, set by error_set_source().
static const char* g_source = NULL;
static size_t g_sourceLength = 0;
static bool g_wholeProgram = false;  // false in the REPL: runtime line numbers may refer to earlier input

// Records where the error is (the message is already in g_err) and jumps back to the setjmp.
static _Noreturn void raise_error(ErrorKind kind, int line, long offset) {
    g_err.kind = kind;
    g_err.line = line;
    g_err.offset = offset;
    longjmp(g_err.env, 1);
}

void lang_error(ErrorKind kind, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_err.message, sizeof g_err.message, fmt, args);
    va_end(args);
    raise_error(kind, line, -1);
}

void lang_error_at(ErrorKind kind, int line, long offset, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_err.message, sizeof g_err.message, fmt, args);
    va_end(args);
    raise_error(kind, line, offset);
}

void error_set_source(const char* source, size_t length, bool wholeProgram) {
    g_source = source;
    g_sourceLength = length;
    g_wholeProgram = wholeProgram;
}

// Prints source line `line` with a "  N | " gutter, and a ^ under byte `offset` if it is on that line.
static void print_source_line(int line, long offset) {
    // Find where the line starts by counting newlines.
    size_t start = 0;
    for (int current = 1; current < line; current++) {
        while (start < g_sourceLength && g_source[start] != '\n') start++;
        if (start >= g_sourceLength) return;  // line is past the end: nothing to show
        start++;
    }
    size_t end = start;
    while (end < g_sourceLength && g_source[end] != '\n' && g_source[end] != '\r') end++;

    int width = snprintf(NULL, 0, "%d", line);
    fprintf(stderr, "    %d | %.*s\n", line, (int)(end - start), g_source + start);
    if (offset < (long)start || offset > (long)end) return;
    // Copy tabs from the source line so the ^ lines up however wide the terminal shows a tab.
    fprintf(stderr, "    %*s | ", width, "");
    for (size_t i = start; i < (size_t)offset; i++) fputc(g_source[i] == '\t' ? '\t' : ' ', stderr);
    fputs("^\n", stderr);
}

void report_error(void) {
    const char* kind = g_err.kind == ERR_SYNTAX ? "Syntax error" : g_err.kind == ERR_COMPILE ? "Compile error" : "Runtime error";
    fflush(stdout);
    fprintf(stderr, "[line %d] %s: %s\n", g_err.line, kind, g_err.message);
    bool sourceMatches = g_err.kind != ERR_RUNTIME || g_wholeProgram;
    if (g_source && sourceMatches && g_err.line > 0) print_source_line(g_err.line, g_err.offset);
}
