# broadcast_language

A small programming language written in C11, with integer and decimal arithmetic, variables, functions, closures, `when`/`repeat`, and strings. Programs are run by a tree-walking interpreter: the source is turned into tokens, then a syntax tree, and the interpreter walks that tree to run it.

Documentation:

- **[DOCUMENTATION.md](DOCUMENTATION.md): the main project documentation. Start here.** It covers how to use the language, error handling, sample outputs, how it works inside, and every data structure and its variables, in simple terms.
- [docs/code-guide.md](docs/code-guide.md) walks through every source file.
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
| Simple arithmetic on (at least) integer values | `+ - * / %`, parentheses and unary `-` with the usual precedence; whole numbers up to 2^53 are exact. See `examples/arithmetic.bc`. |
| Handles errors such as incorrect format | Lexer, parser and runtime errors all print `[line N] Kind error: message` and never crash. See `examples/error_syntax.bc`, `examples/error_runtime.bc`, and the error table in [docs/evaluation.md](docs/evaluation.md#exception-handling). |
| Presents correct output | `tests/run_tests.sh` checks every example's output against its `.expected` file. |
| Follows the PL evaluation criteria | [docs/evaluation.md](docs/evaluation.md) |
| Proper documentation of the source code | Every source file starts with a comment saying what it does, every struct field is described, and [docs/code-guide.md](docs/code-guide.md) explains each file. |
| Runtime: executes without errors, clear prompts | `broadcast_language --help` explains the options; the REPL shows a banner, a `broadcast_language>` prompt and a `help` command. Errors show the source line with a `^` under the problem. |

## Broadcast Studio (the IDE)

`ide/` holds Broadcast Studio, a desktop editor for broadcast_language written in Python with tkinter (no extra packages needed). It has syntax colors, line numbers and auto-indent. It runs programs with the real interpreter, explains errors with beginner tips you can click to jump to the line, and has a live console. It also has 11 guided lessons that check your answers.

```
python ide/broadcast_ide.py      # or double-click ide\start_studio.bat on Windows
python ide/test_lessons.py       # checks that every lesson's solution passes
```

See [DOCUMENTATION.md, section 3](DOCUMENTATION.md#3-getting-started) for a tour.

## Build and run

With GCC or Clang (Linux, macOS, or MinGW/MSYS2 on Windows):

```
gcc -std=c11 -O2 -Wall -Wextra src/*.c -lm -o broadcast_language
./broadcast_language examples/hello.bc   # run a file
./broadcast_language                     # start the REPL
./broadcast_language --help              # list the options
tests/run_tests.sh                       # run every example and compare with its .expected file
```

With CMake, on any platform including Visual Studio on Windows:

```
cmake -B build
cmake --build build --config Release
build/Release/broadcast_language.exe examples/arithmetic.bc   # Visual Studio puts the program here
build/broadcast_language examples/arithmetic.bc               # other generators put it here
ctest --test-dir build -C Release                             # runs tests/run_tests.sh (needs bash, e.g. Git Bash)
```

Directly with Microsoft Visual C++, from a "Developer Command Prompt for VS":

```
cl /std:c11 /O2 /D_CRT_SECURE_NO_WARNINGS src\*.c /Fe:broadcast_language.exe
broadcast_language.exe examples\arithmetic.bc
```

## A short session

```
$ ./broadcast_language
broadcast_language 1.0 - interactive mode
Type 'help' for examples, 'exit' to quit.

broadcast_language> broadcast 2 + 3 * 4
14
broadcast_language> assign n = 7
broadcast_language> broadcast n * n - 1
48
broadcast_language> broadcast (3 + * 4
[line 1] Syntax error: Expect expression. (found '*')
    1 | broadcast (3 + * 4;
      |                ^
broadcast_language> broadcast 10 / 0
[line 1] Runtime error: Division by zero.
broadcast_language> exit
```

## The language

```
// comments run to the end of the line
assign x = 10;                         // variables
x = x + 1;
broadcast "x is " + x;                 // string + anything joins as text

protocol square(n) { report n * n; }   // functions
broadcast square(7);

when (x > 5) { broadcast "big"; } otherwise { broadcast "small"; }

assign i = 0;
repeat (i < 3) { broadcast i; i = i + 1; }
```

Values: numbers, strings, `affirm`/`negative`, `silence`, functions. Operators: `+ - * / %`, `< <= > >= == !=`, `and or !`. Only `silence` and `negative` are falsy. Functions are closures and can be passed around and returned.

## Where each pipeline stage lives

| Stage | File | What it does |
|---|---|---|
| Memory and errors | `src/arena.*`, `src/errors.*` | an arena for tokens and the AST; `setjmp`/`longjmp` error handling |
| Tokens | `src/token.h` | the list of token types |
| Lexer | `src/lexer.*` | source text to tokens |
| AST | `src/ast.h` | the tree node types |
| Parser | `src/parser.*` | tokens to AST (recursive descent, grammar in the header comment) |
| Values | `src/value.*` | tagged `Value`, heap objects (strings, scopes, functions), string hash table |
| Interpreter | `src/interpreter.*` | walks the AST and runs it |
| Entry point | `src/main.c` | file runner and REPL |

Errors carry a line number and show the source line. Syntax errors also put a `^` under the exact token:

```
[line 4] Syntax error: Expect expression. (found '*')
    4 | broadcast (3 + * 4;
      |                ^
```

## Memory model

There is no garbage collector yet. The lexer and parser allocate from an arena that is released at exit. At run time, strings, scopes and functions are heap objects linked into one global list and freed at exit, so a long-running program that builds many strings keeps growing. The interpreter frees each call's and block's scope when it ends unless a function captured it. Adding a mark-and-sweep collector means walking that object list (see the first lines of `src/value.h`).

## Good first experiments

1. Add `for` loops and `otherwise when`, then add `break`.
2. Add built-in functions such as `clock()` and `len(s)`.
3. Add lists and indexing, which means a new `Value` type, new AST nodes and parser rules.
4. Print the AST as text and diff it against expected output in your tests.
5. Report several syntax errors at once instead of stopping at the first.
6. Resolve variables to scope depths before running, which also fixes edge cases with closures.
7. Add a garbage collector: objects are only freed at exit today. Write a mark-and-sweep collector rooted in the global scope and the scopes still running.
8. Compile the AST to bytecode and run it on a stack-based virtual machine (a Compiler Design topic).
