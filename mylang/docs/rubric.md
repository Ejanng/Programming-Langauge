# Grading rubric: self-assessment and presentation guide

This page goes through the final project rubric one row at a time. For each row it shows what earns **Excellent (20 pts)** and where the project shows it. The second half is a demo script and a list of likely questions for the presentation.

All commands assume the program was built as in the README. On Windows with Visual Studio, use `build\Release\mylang.exe` in place of `./mylang`.

## Rubric, row by row

### Requirements: completed 90-100% of the requirements

| Requirement (from the guidelines) | Done | Evidence |
|---|---|---|
| Simple arithmetic with (at least) integer values | Yes | `./mylang examples/arithmetic.rene`: `+ - * / %`, precedence, parentheses, negatives, whole numbers exact up to 9,007,199,254,740,992 |
| Handles errors like incorrect format | Yes | `./mylang examples/error_syntax.rene`, plus the error table in [evaluation.md](evaluation.md#exception-handling) |
| Presents correct output | Yes | `tests/run_tests.sh` checks 8 example programs against their expected output: `8 passed, 0 failed` |
| Follows the PL evaluation criteria | Yes | [evaluation.md](evaluation.md) |
| Proper documentation of the source code | Yes | See the Documentation row below |
| Source code in a ZIP on Google Drive, link posted in MVLE | **Group to do** | The ZIP contains the `mylang/` folder; build output is not included |

### Delivery: delivered on time

**Group to do.** Deadline: **October 30, 2026, 17:00.** Upload the ZIP to Google Drive, set sharing to "Anyone with the link can view", then post the link in the MVLE Course Final Project Submission Thread. Each hour late costs 5 points in this row.

### Documentation: "clearly and effectively documented including descriptions of all class variables"

C has no classes. The equivalent of a class is a `struct`, and its "class variables" are the struct's fields.

- **Every struct field** in every `.h` and `.c` file has a comment describing it. Example from `src/interpreter.h`:

  ```c
  typedef struct {
      ObjEnv* globals;              // top-level scope; lives for the whole session
      ObjEnv* environment;          // the scope currently executing
      ObjEnv** live;                // scopes of the calls and blocks in progress, so an error can free them
      int liveCount, liveCapacity;  // number of entries in live, and slots allocated
      int callDepth;                // how many function calls are in progress, to catch runaway recursion
      Value returnValue;            // set by a `report` statement while the call unwinds
  } Interpreter;
  ```

- **Every source file** starts with a comment saying what it does and where it sits in the pipeline.
- **Every public function** in the headers has a comment saying what it does, and what it returns where that isn't obvious.
- **The grammar** is written out in full at the top of `src/parser.h`.
- **Guides:** [DOCUMENTATION.md](../DOCUMENTATION.md) explains the whole project in plain language, [code-guide.md](code-guide.md) walks through every file in pipeline order, and [evaluation.md](evaluation.md) covers the language evaluation criteria.

### Runtime: "executes without errors, excellent user prompts, good use of symbols, spacing in output"

- **Executes without errors:** builds with no warnings under MSVC (`/W3`), and the CMake build turns on `-Wall -Wextra -Wpedantic` for GCC and Clang. No input crashes it: bad syntax, wrong types, division by zero and runaway recursion all produce a message, never a crash.
- **User prompts:** the REPL opens with a banner naming the version, shows a `mylang>` prompt, and has a `help` command that lists every construct with an example. `./mylang --help` lists the command-line options. An unknown option says which argument was wrong.
- **Symbols and spacing in errors:** every error names its line and kind, shows the source line, and puts a `^` under the exact token for syntax errors:

  ```
  [line 4] Syntax error: Expect expression. (found '*')
      4 | broadcast (3 + * 4;
        |                ^
  ```

- **Exit codes** for scripts: `0` OK, `64` bad usage, `65` syntax error, `70` runtime error, `74` file not found.

### Presentation: "deep understanding; all requirements implemented correctly; includes advanced features or optimizations"

Advanced features to point out, all beyond simple arithmetic:

1. **A themed keyword set** ("Team Communication & Signal": `assign`, `broadcast`, `protocol`, `report`, ...), used consistently in the code, the error messages and the printed values (`affirm`, `negative`, `silence`).
2. **Closures and first-class functions.** A function remembers the scope it was created in, so it can keep private state (`examples/closures.rene`).
3. **Block scopes and shadowing,** with each scope freed as soon as it ends unless a function still needs it.
4. **Precise error reporting** with the source line and a `^` under the exact token, without crashing.
5. **An interactive mode (REPL)** with a `help` command, and variables that persist between lines.
6. **No external libraries:** about 1,500 lines of standard C11, including its own memory arena and hash table.

## Presentation demo script

About 10 minutes. Practice it once. Each step lists what to run and what to say.

1. **What it is (1 min).** "mylang is a small programming language we wrote in C. It reads source code, checks it, and runs it. It supports arithmetic, variables, `when`/`repeat`, functions and closures. Its keywords follow a team-communication theme: `assign`, `broadcast`, `report`."

2. **The pipeline (2 min).** Show the table in the README. "Source text goes through the lexer, which makes tokens; the parser, which builds a syntax tree using our grammar; then the interpreter walks the tree and runs each node."

3. **Arithmetic (1 min).** Run `./mylang examples/arithmetic.rene`. Point out `2 + 3 * 4` giving `14` and `(2 + 3) * 4` giving `20`. "Precedence comes from the grammar: `term` handles + and -, and calls `factor`, which handles * and /, so * binds tighter."

4. **Interactive mode (1 min).** Run `./mylang`, type `help`, then:

   ```
   assign price = 250
   assign qty = 4
   broadcast "total: " + price * qty
   ```

5. **Error handling (2 min).** In the REPL:

   ```
   broadcast (3 + * 4
   broadcast 10 / 0
   broadcast totl
   ```

   "Badly formatted input is caught by the parser before anything runs, and the caret shows the exact token. Runtime errors like division by zero stop cleanly with the line number, and the session continues."

6. **Functions and closures (2 min).** Run `./mylang examples/closures.rene`. "Each counter keeps its own `count`, because a function remembers the scope it was made in." Then run `tests/run_tests.sh`: "every example's output is checked against the expected output."

7. **Evaluation criteria (1 min).** Open [evaluation.md](evaluation.md) and name one point for each criterion. Readability: 12 keywords. Writability: functions and closures. Reliability: type checks with clear errors. Cost: no dependencies, builds anywhere.

## Questions to expect

**Is mylang compiled or interpreted?**
Interpreted. It is a tree-walking interpreter: the parser builds a syntax tree and the interpreter runs it directly, node by node. Compiling to bytecode is a possible next step, which belongs to Compiler Design.

**How does the parser handle operator precedence?**
With one grammar rule per precedence level (see `src/parser.h`). Each rule parses its operands by calling the next-tighter rule, so `2 + 3 * 4` parses as `2 + (3 * 4)`. The rules share one helper, `parseLeftAssociative` in `src/parser.c`, whose `while` loop keeps folding the tree to the left, so operators are left-associative: `10 - 4 - 3` is `(10 - 4) - 3`.

**How are errors handled in C, which has no exceptions?**
With `setjmp`/`longjmp` (`src/errors.h`). Before a phase runs, `setjmp` marks a return point. On an error, `languageError` saves the message and jumps straight back there, out of any depth of nested calls. The REPL then prints the error and carries on.

**Are numbers integers?**
All numbers are 64-bit floating point, as in JavaScript. Every whole number up to 2^53 is stored and printed exactly, which covers the integer requirement. Division can give a fraction: `7 / 2` is `3.5`.

**What is a closure?**
A function together with the variables it captured from where it was defined. `examples/closures.rene` makes two counters that each keep their own `count`. The function object stores a pointer to the scope it was created in, and that scope is kept alive as long as the function needs it.

**How is memory managed?**
The tokens and syntax tree come from an arena that is freed all at once at exit. Runtime objects (strings, functions) go on one list and are also freed at exit. Scopes are freed as soon as their block ends, unless a function captured them. There is no garbage collector yet.

**What would you add next?**
`for` loops and `otherwise when`, built-in functions such as `len()`, lists, and a garbage collector. See the "Good first experiments" list in the README.
