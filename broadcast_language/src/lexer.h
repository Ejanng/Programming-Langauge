#pragma once
// Stage 1: the lexer turns source text into a flat list of tokens.
// The list is stored in the arena and ends with a TOK_EOF token. Throws ERR_SYNTAX on bad input.
#include <stddef.h>

#include "token.h"

// Scans `length` bytes of `source` and returns the tokens; stores how many (including TOK_EOF) in *count.
Token* lex(const char* source, size_t length, int* count);
