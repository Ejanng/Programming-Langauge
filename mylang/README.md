# mylang

A small programming language written in C11, with integer and decimal arithmetic, variables, functions, closures, `if`/`while`, and strings. It has two backends that share one front end: a tree-walking interpreter (the default) and a bytecode compiler plus stack-based virtual machine (`--vm`).

Documentation:

- [docs/code-guide.md](docs/code-guide.md) walks through every source file.
- [docs/backend.md](docs/backend.md) explains the bytecode compiler and VM.
- [docs/evaluation.md](docs/evaluation.md) evaluates the language against the readability, writability, reliability and cost criteria.
- [docs/rubric.md](docs/rubric.md) goes through the grading rubric row by row, with a presentation demo script and likely questions.

## Course project

CMPSC 132: Programming Languages, 1st semester, A.Y. 2025-2026, Final Project.

**Group members:**

- _Name 1_
- _Name 2_
- _Name 3_

How the project meets each requirement in the guidelines:

| Requirement | Where it is met |
|---|---|
| Simple arithmetic on (at least) integer values | `+ - * / %`, parentheses and unary `-` with the usual precedence; whole numbers up to 2^53 are exact. See `examples/arithmetic.rene`. |
| Handles errors such as incorrect format | Lexer, parser and runtime errors all print `[line N] Kind error: message` and never crash. See `examples/error_syntax.rene`, `examples/error_runtime.rene`, and the error table in [docs/evaluation.md](docs/evaluation.md#exception-handling). |
| Presents correct output | `tests/run_tests.sh` checks every example's output against its `.expected` file on both backends. |
| Follows the PL evaluation criteria | [docs/evaluation.md](docs/evaluation.md) |
| Proper documentation of the source code | Every source file starts with a comment saying what it does, every struct field is described, and [docs/code-guide.md](docs/code-guide.md) explains each file. |
| Runtime: executes without errors, clear prompts | `mylang --help` explains the options; the REPL shows a banner, a `mylang>` prompt and a `help` command. Errors show the source line with a `^` under the problem. |

## Build and run

With GCC or Clang (Linux, macOS, or MinGW/MSYS2 on Windows):

```
gcc -std=c11 -O2 -Wall -Wextra src/*.c src/vm/*.c -lm -o mylang
./mylang examples/hello.rene         # run a file on the tree-walking interpreter
./mylang --vm examples/hello.rene    # run it on the bytecode VM
./mylang --dump examples/hello.rene  # show the compiled bytecode, then run it on the VM
./mylang                             # start the REPL (add --vm to use the VM)
./mylang --help                      # list the options
tests/run_tests.sh                   # run every example on both backends and compare with its .expected file
```

With CMake, on any platform including Visual Studio on Windows:

```
cmake -B build
cmake --build build --config Release
build/Release/mylang.exe examples/arithmetic.rene   # Visual Studio puts the program here
build/mylang examples/arithmetic.rene               # other generators put it here
ctest --test-dir build -C Release                    # runs tests/run_tests.sh (needs bash, e.g. Git Bash)
```

Directly with Microsoft Visual C++, from a "Developer Command Prompt for VS":

```
cl /std:c11 /O2 /D_CRT_SECURE_NO_WARNINGS src\*.c src\vm\*.c /Fe:mylang.exe
mylang.exe examples\arithmetic.rene
```

## A short session

```
$ ./mylang
mylang 1.0 - interactive mode (tree-walking interpreter)
Type 'help' for examples, 'exit' to quit.

mylang> print 2 + 3 * 4
14
mylang> let n = 7
mylang> print n * n - 1
48
mylang> print (3 + * 4
[line 1] Syntax error: Expect expression. (found '*')
    1 | print (3 + * 4;
      |            ^
mylang> print 10 / 0
[line 1] Runtime error: Division by zero.
mylang> exit
```

## The language

```
// comments run to the end of the line
let x = 10;                      // variables
x = x + 1;
print "x is " + x;               // string + anything joins as text

fn square(n) { return n * n; }   // functions
print square(7);

if (x > 5) { print "big"; } else { print "small"; }

let i = 0;
while (i < 3) { print i; i = i + 1; }
```

Values: numbers, strings, `true`/`false`, `nil`, functions. Operators: `+ - * / %`, `< <= > >= == !=`, `and or !`. Only `nil` and `false` are falsy. Functions are closures and can be passed around and returned.

## Where each pipeline stage lives

| Stage | File | What it does |
|---|---|---|
| Memory and errors | `src/arena.*`, `src/errors.*` | an arena for tokens, AST and bytecode; `setjmp`/`longjmp` error handling |
| Tokens | `src/token.h` | the list of token types |
| Lexer | `src/lexer.*` | source text to tokens |
| AST | `src/ast.h` | the tree node types |
| Parser | `src/parser.*` | tokens to AST (recursive descent, grammar in the header comment) |
| Values | `src/value.*` | tagged `Value`, heap objects (strings, scopes, functions, closures), string hash table |
| Interpreter | `src/interpreter.*` | walks the AST and runs it (default backend) |
| Bytecode | `src/vm/chunk.*` | instruction set, compiled functions, disassembler |
| Compiler | `src/vm/compiler.*` | AST to bytecode, resolves names to locals, upvalues and globals |
| VM | `src/vm/vm.*` | runs bytecode on a value stack with call frames (`--vm` backend) |
| Entry point | `src/main.c` | file runner and REPL |

Errors carry a line number and show the source line. Syntax errors also put a `^` under the exact token:

```
[line 4] Syntax error: Expect expression. (found '*')
    4 | print (3 + * 4;
      |            ^
```

## Memory model

There is no garbage collector yet. The lexer, parser and compiler allocate from an arena that is released at exit. At run time, strings, closures and upvalues are heap objects linked into one global list and freed at exit, so a long-running program that builds many strings keeps growing. The tree-walker frees each call's and block's scope when it ends unless a function captured it. Adding a mark-and-sweep collector means walking that object list (see the first lines of `src/value.h`).

## Good first experiments

1. Add `for` loops and `else if`, then add `break`.
2. Add built-in functions such as `clock()` and `len(s)`.
3. Add lists and indexing, which means a new `Value` type, new AST nodes and parser rules.
4. Print the AST as text and diff it against expected output in your tests.
5. Report several syntax errors at once instead of stopping at the first.
6. Resolve variables to scope depths before running, which also fixes edge cases with closures.
7. Add a garbage collector: objects are only freed at exit today. Write a mark-and-sweep collector rooted in the stack, the globals and the open upvalues.
8. ~~Compile the AST to bytecode and run it on a stack-based VM.~~ Done: see [docs/backend.md](docs/backend.md), which also lists next steps for the VM.
