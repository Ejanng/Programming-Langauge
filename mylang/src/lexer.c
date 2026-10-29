// Stage 1: the lexer. Scans the source character by character and produces tokens:
// punctuation, operators, numbers, strings, identifiers and keywords. Comments and whitespace are skipped.
#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "errors.h"

typedef struct {
    const char* source;  // the source text being scanned
    size_t length;       // length of source
    size_t start;        // index where the current token begins
    size_t current;      // index of the next character to read
    int line;            // current line number, for tokens and errors
    Token* tokens;       // tokens produced so far
    int count;           // number of entries in tokens
    int capacity;        // slots allocated in tokens
} Lexer;

static bool isAtEnd(const Lexer* lexer) { return lexer->current >= lexer->length; }
static char peek(const Lexer* lexer) { return isAtEnd(lexer) ? '\0' : lexer->source[lexer->current]; }
static char peekNext(const Lexer* lexer) { return lexer->current + 1 >= lexer->length ? '\0' : lexer->source[lexer->current + 1]; }
static char advance(Lexer* lexer) { return lexer->source[lexer->current++]; }

static bool match(Lexer* lexer, char expected) {
    if (isAtEnd(lexer) || lexer->source[lexer->current] != expected) return false;
    lexer->current++;
    return true;
}

static bool isDigit(char character) { return isdigit((unsigned char)character) != 0; }
static bool isAlpha(char character) { return isalpha((unsigned char)character) != 0 || character == '_'; }

static void pushToken(Lexer* lexer, Token token) {
    token.offset = lexer->start;
    lexer->tokens = arenaReserveArray(lexer->tokens, lexer->count, &lexer->capacity, sizeof(Token));
    lexer->tokens[lexer->count++] = token;
}

static void addToken(Lexer* lexer, TokenType type) {
    size_t length = lexer->current - lexer->start;
    pushToken(lexer, (Token){type, arenaDuplicateString(lexer->source + lexer->start, length), length, 0, lexer->line});
}

static void scanString(Lexer* lexer) {
    int startLine = lexer->line;
    size_t capacity = 16, length = 0;
    char* value = malloc(capacity);
    while (!isAtEnd(lexer) && peek(lexer) != '"') {
        char character = advance(lexer);
        if (character == '\n') lexer->line++;
        if (character == '\\' && !isAtEnd(lexer)) {  // escape sequences: \n \t \" \\ .
            char escaped = advance(lexer);
            switch (escaped) {
                case 'n': character = '\n'; break;
                case 't': character = '\t'; break;
                default: character = escaped;  // \" and \\ map to themselves, like any other escaped character
            }
        }
        if (length + 1 >= capacity) value = realloc(value, capacity *= 2);
        value[length++] = character;
    }
    if (isAtEnd(lexer)) {
        free(value);
        languageErrorAt(ERR_SYNTAX, startLine, (long)lexer->start, "Unterminated string.");
    }
    advance(lexer);  // closing quote
    pushToken(lexer, (Token){TOK_STRING, arenaDuplicateString(value, length), length, 0, startLine});
    free(value);
}

static void scanNumber(Lexer* lexer) {
    while (isDigit(peek(lexer))) advance(lexer);
    if (peek(lexer) == '.' && isDigit(peekNext(lexer))) {
        advance(lexer);
        while (isDigit(peek(lexer))) advance(lexer);
    }
    size_t length = lexer->current - lexer->start;
    const char* text = arenaDuplicateString(lexer->source + lexer->start, length);
    pushToken(lexer, (Token){TOK_NUMBER, text, length, strtod(text, NULL), lexer->line});
}

static void scanIdentifier(Lexer* lexer) {
    static const struct {
        const char* word;
        TokenType type;
    } keywords[] = {
        {"and", TOK_AND},             {"otherwise", TOK_ELSE}, {"negative", TOK_FALSE}, {"protocol", TOK_FN},
        {"when", TOK_IF},             {"assign", TOK_LET},     {"silence", TOK_NIL},    {"or", TOK_OR},
        {"broadcast", TOK_PRINT},     {"report", TOK_RETURN},  {"affirm", TOK_TRUE},    {"repeat", TOK_WHILE},
    };
    while (isAlpha(peek(lexer)) || isDigit(peek(lexer))) advance(lexer);
    size_t length = lexer->current - lexer->start;
    TokenType type = TOK_IDENTIFIER;
    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strlen(keywords[i].word) == length && memcmp(keywords[i].word, lexer->source + lexer->start, length) == 0) {
            type = keywords[i].type;
            break;
        }
    }
    addToken(lexer, type);
}

static void scanToken(Lexer* lexer) {
    char character = advance(lexer);
    switch (character) {
        case '(': addToken(lexer, TOK_LEFT_PAREN); break;
        case ')': addToken(lexer, TOK_RIGHT_PAREN); break;
        case '{': addToken(lexer, TOK_LEFT_BRACE); break;
        case '}': addToken(lexer, TOK_RIGHT_BRACE); break;
        case ',': addToken(lexer, TOK_COMMA); break;
        case ';': addToken(lexer, TOK_SEMICOLON); break;
        case '+': addToken(lexer, TOK_PLUS); break;
        case '-': addToken(lexer, TOK_MINUS); break;
        case '*': addToken(lexer, TOK_STAR); break;
        case '%': addToken(lexer, TOK_PERCENT); break;
        case '!': addToken(lexer, match(lexer, '=') ? TOK_BANG_EQUAL : TOK_BANG); break;
        case '=': addToken(lexer, match(lexer, '=') ? TOK_EQUAL_EQUAL : TOK_EQUAL); break;
        case '<': addToken(lexer, match(lexer, '=') ? TOK_LESS_EQUAL : TOK_LESS); break;
        case '>': addToken(lexer, match(lexer, '=') ? TOK_GREATER_EQUAL : TOK_GREATER); break;
        case '/':
            if (match(lexer, '/')) {
                while (peek(lexer) != '\n' && !isAtEnd(lexer)) advance(lexer);  // comment runs to end of line
            } else {
                addToken(lexer, TOK_SLASH);
            }
            break;
        case ' ':
        case '\r':
        case '\t': break;
        case '\n': lexer->line++; break;
        case '"': scanString(lexer); break;
        default:
            if (isDigit(character)) {
                scanNumber(lexer);
            } else if (isAlpha(character)) {
                scanIdentifier(lexer);
            } else {
                languageErrorAt(ERR_SYNTAX, lexer->line, (long)lexer->start, "Unexpected character '%c'.", character);
            }
    }
}

Token* lex(const char* source, size_t length, int* count) {
    Lexer lexer = {.source = source, .length = length, .line = 1};
    // Skip a UTF-8 byte order mark. Windows editors such as Notepad (and PowerShell pipes) put
    // one at the start of text, and it is not part of the program.
    if (length >= 3 && memcmp(source, "\xEF\xBB\xBF", 3) == 0) lexer.current = 3;
    while (!isAtEnd(&lexer)) {
        lexer.start = lexer.current;
        scanToken(&lexer);
    }
    lexer.start = lexer.current;  // the end-of-file token sits just past the last character
    pushToken(&lexer, (Token){TOK_EOF, "", 0, 0, lexer.line});
    *count = lexer.count;
    return lexer.tokens;
}
