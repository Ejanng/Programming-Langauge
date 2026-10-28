#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace rene {

// Every error the language reports (syntax or runtime) carries a kind and a line.
class ReneError : public std::runtime_error {
public:
    // kind: "SyntaxError" or "RuntimeError", line: where it happened,
    // message: what went wrong.
    ReneError(std::string kind, int line, const std::string& message)
        : std::runtime_error(message), kind_(std::move(kind)), line_(line) {}

    // Getters for the error's kind and line.
    const std::string& kind() const { return kind_; }
    int line() const { return line_; }

    // Builds the text shown to the user, e.g. "[line 4] RuntimeError: division by zero".
    std::string format() const {
        return "[line " + std::to_string(line_) + "] " + kind_ + ": " + what();
    }

    // True if the error was found before running (by the lexer or parser).
    bool isSyntaxError() const { return kind_ == "SyntaxError"; }

private:
    // Error category: "SyntaxError" or "RuntimeError".
    std::string kind_;
    // Source line the error points to.
    int line_;
};

}  // namespace rene
