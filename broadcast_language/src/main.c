// Entry point: `broadcast_language script.bc` runs a file, `broadcast_language` alone starts a REPL.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "errors.h"
#include "interpreter.h"
#include "value.h"

#define BROADCAST_LANGUAGE_VERSION "1.0"

static const char* const USAGE =
    "Usage: broadcast_language [script.bc]\n"
    "  script.bc  run this file (with no file, start the interactive REPL)\n"
    "  --help     show this message\n";

static const char* const REPL_HELP =
    "Statements end with ';' (the REPL adds it for you on a single line).\n"
    "  broadcast 2 + 3 * 4;                 arithmetic: + - * / % and parentheses, prints 14\n"
    "  assign x = 10;                       declare a variable\n"
    "  x = x - 1;                           change its value\n"
    "  protocol sq(n) { report n * n; }     define a function, then call it: broadcast sq(x);\n"
    "  when (x > 5) { broadcast \"big\"; } otherwise { broadcast \"small\"; }\n"
    "  repeat (x > 0) { x = x - 1; }\n"
    "Values: numbers (7, -3, 2.5), strings (\"hi\"), affirm, negative, silence, protocols.\n"
    "Commands: help, exit (or quit, or end of input: Ctrl+D, or Ctrl+Z then Enter on Windows).\n";

static int runFile(Interpreter* interpreter, const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file: %s\n", path);
        return EXIT_NO_FILE;
    }
    size_t capacity = 4096, length = 0;
    char* buffer = malloc(capacity);
    size_t bytesRead;
    while (buffer && (bytesRead = fread(buffer + length, 1, capacity - length, file)) > 0) {
        length += bytesRead;
        if (length == capacity) buffer = realloc(buffer, capacity *= 2);
    }
    fclose(file);
    if (!buffer) {
        fputs("Out of memory.\n", stderr);
        return EXIT_FAILURE;
    }
    setErrorSource(buffer, length, true);
    int result = interpreterRun(interpreter, buffer, length);
    setErrorSource(NULL, 0, false);
    free(buffer);
    return result;
}

// Reads one line of any length from `input` into *line, growing it as needed (*capacity is its size).
// Returns the number of characters read, including the '\n', or -1 at end of input.
// This is a portable stand-in for POSIX getline(), which Windows does not provide.
static long readLine(char** line, size_t* capacity, FILE* input) {
    size_t length = 0;
    int character = EOF;
    while ((character = fgetc(input)) != EOF) {
        if (length + 2 > *capacity) {
            size_t newCapacity = *capacity < 128 ? 128 : *capacity * 2;
            char* grown = realloc(*line, newCapacity);
            if (!grown) return -1;
            *line = grown;
            *capacity = newCapacity;
        }
        (*line)[length++] = (char)character;
        if (character == '\n') break;
    }
    if (length == 0 && character == EOF) return -1;
    (*line)[length] = '\0';
    return (long)length;
}

static void repl(Interpreter* interpreter) {
    printf("broadcast_language %s - interactive mode\n", BROADCAST_LANGUAGE_VERSION);
    printf("Type 'help' for examples, 'exit' to quit.\n\n");
    char* line = NULL;
    size_t capacity = 0;
    for (;;) {
        printf("broadcast_language> ");
        fflush(stdout);
        long charactersRead = readLine(&line, &capacity, stdin);
        if (charactersRead < 0) {
            printf("\n");
            break;
        }
        size_t length = (size_t)charactersRead;
        while (length > 0 && strchr(" \t\r\n", line[length - 1])) length--;
        if (length == 0) continue;
        line[length] = '\0';
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;
        if (strcmp(line, "help") == 0) {
            fputs(REPL_HELP, stdout);
            continue;
        }
        // Convenience: allow typing `print 1 + 2` without the final semicolon.
        if (line[length - 1] != ';' && line[length - 1] != '}') {
            if (length + 2 > capacity) line = realloc(line, capacity = length + 2);
            line[length++] = ';';
            line[length] = '\0';
        }
        setErrorSource(line, length, false);
        interpreterRun(interpreter, line, length);  // errors are printed; the session keeps going
    }
    free(line);
}

int main(int argc, char** argv) {
    const char* path = NULL;
    for (int i = 1; i < argc; i++) {
        const char* argument = argv[i];
        if (strcmp(argument, "--help") == 0 || strcmp(argument, "-h") == 0) {
            fputs(USAGE, stdout);
            return EXIT_OK;
        } else if (!path && strncmp(argument, "--", 2) != 0) {
            path = argument;
        } else {
            fprintf(stderr, "Unknown option or extra argument: %s\n", argument);
            fputs(USAGE, stderr);
            return EXIT_USAGE;
        }
    }

    Interpreter interpreter;
    interpreterInit(&interpreter);

    int result = EXIT_OK;
    if (path) {
        result = runFile(&interpreter, path);
    } else {
        repl(&interpreter);
    }

    interpreterFree(&interpreter);
    objectsFreeAll();
    arenaFreeAll();
    return result;
}
