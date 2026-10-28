// Entry point: `mylang script.rene` runs a file, `mylang` alone starts a REPL.
// `--vm` runs programs on the bytecode VM instead of the tree-walking interpreter,
// and `--dump` (which implies --vm) also prints the compiled bytecode before running it.
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "errors.h"
#include "interpreter.h"
#include "value.h"
#include "vm/vm.h"

#define MYLANG_VERSION "1.0"

static const char* const USAGE =
    "Usage: mylang [--vm] [--dump] [script.rene]\n"
    "  script.rene  run this file (with no file, start the interactive REPL)\n"
    "  --vm         run on the bytecode virtual machine instead of the tree-walking interpreter\n"
    "  --dump       print the compiled bytecode, then run it on the VM (implies --vm)\n"
    "  --help       show this message\n";

static const char* const REPL_HELP =
    "Statements end with ';' (the REPL adds it for you on a single line).\n"
    "  print 2 + 3 * 4;            arithmetic: + - * / % and parentheses, prints 14\n"
    "  let x = 10;                 declare a variable\n"
    "  x = x - 1;                  assign to it\n"
    "  fn sq(n) { return n * n; }  define a function, then call it: print sq(x);\n"
    "  if (x > 5) { print \"big\"; } else { print \"small\"; }\n"
    "  while (x > 0) { x = x - 1; }\n"
    "Values: numbers (7, -3, 2.5), strings (\"hi\"), true, false, nil, functions.\n"
    "Commands: help, exit (or quit, or end of input: Ctrl+D, or Ctrl+Z then Enter on Windows).\n";

// The selected backend: runs a piece of source and returns an exit code.
typedef struct {
    bool useVm;               // true: run on the bytecode VM; false: on the tree-walking interpreter
    Interpreter interpreter;  // state of the tree-walking backend (globals persist across REPL lines)
    VM vm;                    // state of the bytecode backend (globals persist across REPL lines)
} Runner;

static int run_source(Runner* r, const char* source, size_t length) {
    return r->useVm ? vm_run(&r->vm, source, length) : interpreter_run(&r->interpreter, source, length);
}

static int run_file(Runner* r, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", path);
        return EXIT_NO_FILE;
    }
    size_t cap = 4096, len = 0;
    char* buffer = malloc(cap);
    size_t n;
    while (buffer && (n = fread(buffer + len, 1, cap - len, f)) > 0) {
        len += n;
        if (len == cap) buffer = realloc(buffer, cap *= 2);
    }
    fclose(f);
    if (!buffer) {
        fputs("Out of memory.\n", stderr);
        return EXIT_FAILURE;
    }
    error_set_source(buffer, len, true);
    int result = run_source(r, buffer, len);
    error_set_source(NULL, 0, false);
    free(buffer);
    return result;
}

// Reads one line of any length from `in` into *line, growing it as needed (*cap is its size).
// Returns the number of characters read, including the '\n', or -1 at end of input.
// This is a portable stand-in for POSIX getline(), which Windows does not provide.
static long read_line(char** line, size_t* cap, FILE* in) {
    size_t len = 0;
    int c = EOF;
    while ((c = fgetc(in)) != EOF) {
        if (len + 2 > *cap) {
            size_t newCap = *cap < 128 ? 128 : *cap * 2;
            char* grown = realloc(*line, newCap);
            if (!grown) return -1;
            *line = grown;
            *cap = newCap;
        }
        (*line)[len++] = (char)c;
        if (c == '\n') break;
    }
    if (len == 0 && c == EOF) return -1;
    (*line)[len] = '\0';
    return (long)len;
}

static void repl(Runner* r) {
    printf("mylang %s - interactive mode (%s)\n", MYLANG_VERSION, r->useVm ? "bytecode VM" : "tree-walking interpreter");
    printf("Type 'help' for examples, 'exit' to quit.\n\n");
    char* line = NULL;
    size_t cap = 0;
    for (;;) {
        printf("mylang> ");
        fflush(stdout);
        long got = read_line(&line, &cap, stdin);
        if (got < 0) {
            printf("\n");
            break;
        }
        size_t len = (size_t)got;
        while (len > 0 && strchr(" \t\r\n", line[len - 1])) len--;
        if (len == 0) continue;
        line[len] = '\0';
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;
        if (strcmp(line, "help") == 0) {
            fputs(REPL_HELP, stdout);
            continue;
        }
        // Convenience: allow typing `print 1 + 2` without the final semicolon.
        if (line[len - 1] != ';' && line[len - 1] != '}') {
            if (len + 2 > cap) line = realloc(line, cap = len + 2);
            line[len++] = ';';
            line[len] = '\0';
        }
        error_set_source(line, len, false);
        run_source(r, line, len);  // errors are printed; the session keeps going
    }
    free(line);
}

int main(int argc, char** argv) {
    bool useVm = false;
    bool dump = false;
    const char* path = NULL;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            fputs(USAGE, stdout);
            return EXIT_OK;
        } else if (strcmp(arg, "--vm") == 0) {
            useVm = true;
        } else if (strcmp(arg, "--dump") == 0) {
            useVm = dump = true;
        } else if (!path && strncmp(arg, "--", 2) != 0) {
            path = arg;
        } else {
            fprintf(stderr, "Unknown option or extra argument: %s\n", arg);
            fputs(USAGE, stderr);
            return EXIT_USAGE;
        }
    }

    Runner runner = {.useVm = useVm};
    interpreter_init(&runner.interpreter);
    vm_init(&runner.vm, dump);

    int result = EXIT_OK;
    if (path) {
        result = run_file(&runner, path);
    } else {
        repl(&runner);
    }

    interpreter_free(&runner.interpreter);
    vm_free(&runner.vm);
    objects_free_all();
    arena_free_all();
    return result;
}
