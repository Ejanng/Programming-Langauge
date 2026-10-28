#pragma once

#include <string>

namespace rene {

// Every kind of token the lexer can produce.
enum class TokenType {
    // Single-character tokens
    LeftParen, RightParen, LeftBrace, RightBrace, LeftBracket, RightBracket,
    Comma, Semicolon, Plus, Minus, Star, Slash, Percent,

    // One- or two-character tokens
    Equal, EqualEqual, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,

    // Literals
    Identifier, String, Number,

    // Keywords
    Let, Fn, Return, If, Else, While, For, In, Break, Continue,
    Print, True, False, Nil, And, Or, Not,

    EndOfFile
};

// One token: the smallest meaningful piece of source code (a word, number or symbol).
struct Token {
    // What kind of token this is (keyword, operator, literal, ...).
    TokenType type;
    std::string lexeme;  // raw text, or the decoded value for string literals
    // Numeric value, filled in only for Number tokens.
    double number = 0.0;
    // Source line the token starts on, used in error messages.
    int line = 0;
};

}  // namespace rene
