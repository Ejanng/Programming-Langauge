# Code Guide: every file, one by one

This guide walks through every file in the project, explains what it does, and explains why it is written that way. It assumes you know basic C but have never built a language before.

---

## 1. The big picture

A programming language implementation turns text into behavior. Your program goes through these stages:

```
 hello.bc (text)
      │
      ▼
 ┌─────────┐   tokens    ┌─────────┐    AST     ┌─────────────────────────────┐
 │  Lexer  │ ──────────▶ │ Parser  │ ─────────▶ │ Tree-walking interpreter    │
 │lexer.c  │             │parser.c │            │ interpreter.c               │
 └─────────┘             └─────────┘            └─────────────────────────────┘
```

1. **Lexer** (scanner): turns raw characters into **tokens**, meaning words like `assign`, `42`, `+` and `"hi"`.
2. **Parser**: turns the flat token list into an **AST** (abstract syntax tree), a tree that shows the structure of the program, such as "this `+` has these two operands".
3. **Interpreter**: actually runs the program. It is a **tree-walking interpreter**: it walks the AST directly and does what each node says. This is simple and easy to understand.

### The language itself

```
assign x = 10;
protocol add(a, b) { report a + b; }
broadcast add(x, 5);          // 15
when (x > 5) { broadcast "big"; } otherwise { broadcast "small"; }
repeat (x > 0) { x = x - 1; }
```

- **Types:** numbers (all `double`), strings, booleans (`affirm`/`negative`), `silence`, and functions.
- **Statements:** `assign`, `broadcast`, `when`/`otherwise`, `repeat`, `report`, `protocol`, blocks `{ }`, and expression statements. Every simple statement ends with `;`.
- **Operators:** `+ - * / %`, `== != < <= > >=`, `and`, `or`, `!`, unary `-`, and `=` for assignment.
- **Functions** are first-class values and **closures** (they remember the variables around where they were defined).
- Only `silence` and `negative` are falsy. `0` and `""` count as true.
- `"a" + 1` produces `"a1"`. If either side of `+` is a string, the other side is converted to text.

### File map

| File | Stage | One-line job |
|---|---|---|
| `src/token.h` | front end | Defines the token types |
| `src/lexer.c/.h` | front end | Text → tokens |
| `src/ast.h` | front end | Defines the tree node shapes |
| `src/parser.c/.h` | front end | Tokens → AST |
| `src/arena.c/.h` | support | Fast memory allocator for the front end |
| `src/errors.c/.h` | support | Error reporting with `setjmp`/`longjmp` |
| `src/value.c/.h` | runtime | Values, strings, hash table, scopes, functions |
| `src/interpreter.c/.h` | runtime | Tree-walking interpreter |
| `src/main.c` | driver | Command line + REPL |
| `CMakeLists.txt` | build | Build config |
| `tests/run_tests.sh` | tests | Runs the examples and checks their output |
| `examples/*.bc` | tests | Sample programs + `.expected` output |

---

## 2. The front end

### `src/token.h`: the vocabulary

A **token** is the smallest meaningful piece of source code. `assign x = 5;` becomes: `LET`, `IDENTIFIER(x)`, `EQUAL`, `NUMBER(5)`, `SEMICOLON`.

`token.h` has two things:
- `TokenType`, an enum of every kind of token (`TOK_PLUS`, `TOK_IF`, `TOK_NUMBER`, ...), grouped into single-character, one-or-two-character (`!` vs `!=`), literals/names, and keywords.
- `Token`, a struct holding the type, the text (`lexeme`), its length, the numeric value (for numbers), and the **line number**. The line number is carried through every stage so error messages can say where the problem is.

### `src/lexer.c` / `lexer.h`: text → tokens

The public API is one function: `Token* lex(source, length, &count)`.

**How it works:** a `Lexer` struct tracks `start` (where the current token began) and `cur` (where we are now). The main loop in `lex()`:

```
while not at end:
    start = cur
    scanToken()      // reads one token, pushes it onto the list
push an EOF token
```

`scanToken()` reads one character and decides what to do:
- **Single characters** like `(`, `+`, `;` → emit that token immediately.
- **Two-character operators**: for `!`, `=`, `<`, `>` it calls `match(lexer, '=')`. That checks whether the next character is `=` and consumes it if so. This is how `=` vs `==` is told apart.
- **`/`**: if the next character is also `/`, it is a comment, so skip to the end of the line. Otherwise it is division.
- **Whitespace**: skipped. A newline bumps the line counter.
- **`"`**: calls `scanString()`, which reads until the closing quote and handles escapes (`\n`, `\t`, `\"`, `\\`). It tracks newlines inside strings and reports "Unterminated string" if the file ends first.
- **Digits**: `scanNumber()` reads digits, then an optional `.` followed by more digits, and converts with `strtod`. It requires a digit after the `.`, so `5.` is a number followed by a dot.
- **Letters or `_`**: `scanIdentifier()` reads the whole word, then checks it against a keyword table. Match → keyword token, no match → `TOK_IDENTIFIER`.

**Design notes**
- The lexer works on the whole file at once and returns an array. The parser can then look ahead freely with no re-scanning.
- Token text is copied into the arena (see below) so it stays valid after the source buffer is freed.

### `src/ast.h`: the tree shape

The AST has two node families:

- **`Expr`**: things that produce a value: numbers, strings, variables, `a + b`, calls.
- **`Stmt`**: things that do something: `broadcast`, `assign`, `when`, `repeat`, blocks, function declarations.

Each node is a struct with a `kind` tag (an enum). The "kind" tells you which fields are meaningful. For example, `EXPR_BINARY` uses `operatorType`, `left` and `right`, while `EXPR_NUMBER` only uses `number`.

The header comment is honest about the trade-off. One struct that fits all node kinds wastes some memory but is very easy to read. A tagged union would be tighter.

`Program` is just the list of top-level statements.

**Example:** `broadcast 1 + 2 * 3;` becomes

```
STMT_PRINT
  └─ EXPR_BINARY (+)
       ├─ EXPR_NUMBER 1
       └─ EXPR_BINARY (*)
            ├─ EXPR_NUMBER 2
            └─ EXPR_NUMBER 3
```

The tree shape is what encodes "multiply first".

### `src/parser.c` / `parser.h`: tokens → AST

The public API is `Program* parse(tokens, count)`.

This is a **recursive descent parser**. The grammar is written as a comment at the top of `parser.h`, and each grammar rule is one C function. The functions call each other in the same order as the grammar.

**Operator precedence** is handled by the call chain. Lowest precedence is at the top and highest at the bottom:

```
expression → assignment → parseLogicOr → parseLogicAnd → equality
           → comparison → term (+ -) → factor (* / %) → unary (! -) → call → primary
```

To parse `1 + 2 * 3`: `parseTerm()` calls `parseFactor()` for its left side. `parseFactor()` parses `1` and sees no `*`, so it returns. `parseTerm()` sees `+`, then calls `parseFactor()` again for the right side. That call parses `2 * 3` as one unit. So `*` binds tighter, because it is parsed deeper in the chain.

**`parseLeftAssociative()`** is a helper that does the same loop for all the binary levels (`or`, `and`, `==`, `<`, `+`, `*`):

```
expr = next()
while we see one of our operators:
    op = that operator
    right = next()
    expr = Binary(expr, op, right)    // the old expr becomes the left child
```

Building left-to-right makes `10 - 3 - 2` parse as `(10 - 3) - 2`, which is **left-associative** (the correct, expected result).

**Assignment** is the exception. It is **right-associative** (`a = b = 3` means `a = (b = 3)`), so `parseAssignment()` calls itself recursively. It parses the left side as a normal expression first, then, if it sees `=`, checks that the left side is a plain variable. If it is not (like `1 = 2`), it throws "Invalid assignment target".

**Statements:** `parseDeclaration()` checks for `protocol` or `assign`, otherwise falls to `parseStatement()`, which checks for `broadcast`/`when`/`repeat`/`report`/`{`, otherwise it is an expression statement.

**Helpers:** `peek` (look at the current token), `advance` (consume it), `check` (is it this type?), `match` (if it is this type, consume it and say yes), `consume` (must be this type, or throw a syntax error with a message). Almost the whole parser is built out of these five.

**Errors:** `errorAt()` reports the problem and the offending token ("Expect ';' after value. (found 'broadcast')"). It does not try to recover, so the **first** syntax error stops everything.

---

## 3. Support code

### `src/arena.c` / `arena.h`: the memory arena

**Problem:** the lexer and parser create thousands of small objects (tokens, AST nodes, strings). Freeing each one individually is tedious and easy to get wrong, especially when a syntax error aborts parsing halfway.

**Solution:** an **arena allocator**. You ask it for memory (`arenaAllocate`), and you **never free individual pieces**. At exit, `arenaFreeAll()` releases everything at once.

**How it works:** memory comes in 64 KB blocks. Each allocation just "bumps" a pointer forward within the current block (rounded to 16 bytes for alignment). When the block is full, it allocates a new one and links it into a list. A request bigger than 16 KB gets its own dedicated block. Blocks are `calloc`'d, so arena memory always starts zeroed. That is why new AST nodes have all fields at `0`/`NULL` by default.

Other pieces:
- `arenaDuplicateString`: copies a string into the arena.
- `arenaReserveArray`: a growable array. When full, it allocates a bigger array (double the size) and copies the old data. The old array is not freed, because the arena just keeps it until exit. That is a small waste, but a simple design.
- `PtrVec` / `pointerVectorPush`: a growable list of pointers, which the parser uses for "list of statements / arguments / parameters".

**Why it matters:** it makes the front end leak-free by construction, and fast, because allocation is a pointer bump.

### `src/errors.c` / `errors.h`: errors without exceptions

C has no `try/catch`. This project uses **`setjmp`/`longjmp`** to get the same effect.

- A phase that can fail calls `setjmp(errorState.jumpBuffer)`. This marks "if something goes wrong, jump back here". It returns 0 normally.
- Anywhere deep in the code, `languageError(kind, line, "message...")` fills in the global `errorState` (kind, line, message) and calls `longjmp`. Execution teleports back to the `setjmp` call, which now returns non-zero.
- The lexer and parser use `languageErrorAt(kind, line, offset, ...)` instead, which also records the byte offset of the bad character or token (every `Token` stores its `offset`).
- The code at the `setjmp` site then calls `reportError()` and returns an exit code. `reportError()` prints `[line N] Kind error: message`, then the source line from the text `main.c` registered with `setErrorSource()`, then a `^` under the offset if there is one. In the REPL, runtime errors skip the source line, because the error may be inside a function typed on an earlier line.

There are two kinds of error:
- `ERR_SYNTAX`: from the lexer/parser
- `ERR_RUNTIME`: while the program runs (e.g. division by zero, undefined variable, `report` at the top level)

`languageError` is `_Noreturn`, which tells the compiler it never comes back. That is why functions like `errorAt` do not need a return value afterwards.

`reportError()` prints `[line N] Syntax error: message` to stderr. It flushes stdout first so that, when both go to the same terminal, output appears in the right order.

Exit codes are defined here in an enum: 0 OK, 64 usage, 65 syntax error, 70 runtime error, 74 file not found.

**Caveat of `longjmp`:** it skips any cleanup in between. That is fine here because the front end uses the arena (nothing to clean up), but the interpreter needs to manually clean up scopes after a runtime error (see below).

---

## 4. The runtime

### `src/value.h` / `value.c`: what values look like at run time

**`Value`** is a small tagged union:

```c
typedef struct {
    ValueType type;           // NIL, NUMBER, BOOL, or OBJ
    union { double number; bool boolean; Obj* obj; } as;
} Value;
```

Numbers, booleans and nil live directly inside the `Value`. Anything bigger (strings, functions, scopes) is a **heap object**, and the `Value` just holds a pointer to it.

**Heap objects** all begin with an `Obj` header (a type tag and a `next` pointer). Because the header is first, a pointer to any object can be treated as an `Obj*` (this is C's version of inheritance). Every object is added to one global linked list, `allObjects`.

**Memory management:** objects are **never freed while the program runs**. At exit, `objectsFreeAll()` walks the list and frees everything. This is a deliberate simplification. A real garbage collector (mark-and-sweep) would walk that same list and free the unreachable ones. The header comment points this out as the natural next step. The trade-off is that a long-running program that creates many strings uses more and more memory.

**What's in value.c**
- **Strings** (`ObjString`): the length plus the characters, stored inline after the header (a "flexible array member"). `stringCopy` and `stringConcatenate` create them.
- **Hash table** (`Table`): string keys → Values. Uses **open addressing** with **linear probing** (if a slot is taken, try the next). The hash function is FNV-1a. Capacity is always a power of two, so `hash & (capacity - 1)` replaces a slow modulo. Used for variable scopes.
- **Scopes** (`ObjEnv`): a table of variables plus a `parent` pointer. Looking up a variable checks this scope, then the parent, and so on up the chain.
- **Functions** (`ObjFunction`): a pointer to the function's AST node, plus the scope it was defined in. Remembering that scope is exactly what makes a **closure** work.
- **Operations:** `isTruthy` (only `silence` and `negative` are falsy), `valuesEqual`, and `valueToString` (how `broadcast` shows a value). `formatNumber` prints whole numbers without a decimal point (`5`, not `5.000000`), and prints every digit of whole numbers up to 2^53, the largest range a double stores exactly (`9007199254740992`, not `9.00719925474099e+15`).

**Scope lifetime (`environmentNew` / `environmentRelease` / `environmentCapture`)**
This is the trickiest part of `value.c`. Each function call or block creates a scope. Most scopes die when the call ends, so freeing them right away keeps memory use low (this matters for loops and recursion). But if a closure was created inside, the scope must **stay alive**.

- `environmentNew` makes a scope that is *private*: not yet on the global list.
- `environmentRelease` frees a private scope when its block ends.
- `environmentCapture` is called when a function is created. It marks the scope, and all its parents, as *captured* and moves them onto the global list, so they are no longer freed early. `environmentRelease` then does nothing for captured scopes.

---

## 5. The tree-walking interpreter

### `src/interpreter.c` / `interpreter.h`

The idea: **walk the AST and do what each node says.** It has two main functions that call each other:

- `execute(stmt)`: runs a statement.
- `evaluate(expr)`: computes an expression and returns a `Value`.

Each is a big `switch` on the node's `kind`.

**Expressions (`evaluate`)**
- Literals produce a Value directly.
- `EXPR_VARIABLE`: look the name up through the scope chain. If it is not found, it is a runtime error "Undefined variable".
- `EXPR_ASSIGN`: evaluate the right side, then update the existing variable.
- `EXPR_LOGICAL` (`and`/`or`): **short-circuits**. For `a or b`, if `a` is truthy it returns `a` without ever evaluating `b`. It returns the actual value, not a boolean, so `silence or "default"` gives `"default"`.
- `evaluateBinary`: evaluates both sides, then does the operation. `==` works on any values. `+` handles numbers or string joining. Everything else requires numbers. Division and modulo by zero are errors.
- `evaluateCall`: the most involved. Steps: evaluate the callee, evaluate the arguments (into an 8-slot buffer on the C stack, or a heap array for longer argument lists, because MSVC has no variable-length arrays), check it is a function and the argument count matches, check call depth (max 1000, to give a clean "Stack overflow" error instead of crashing C), create a new scope whose **parent is the function's saved closure scope** (not the caller's scope, which gives lexical scoping), bind parameters, run the body, and pick up the return value.

**Statements (`execute`)**
- `broadcast`: converts to string, writes it, adds a newline.
- `assign`: defines the variable in the current scope (nil if there is no initializer).
- `block`: makes a new child scope.
- `when`/`repeat`: evaluate the condition with `isTruthy`, then run the right branch.
- `protocol`: creates a function object and captures the current scope.
- `report`: stores the value in `interpreter->returnValue` and returns `true`.

**How `report` unwinds:** `execute` returns a bool, "did a return happen?". Every loop and block checks it and stops early if true. By the time control gets back to `evaluateCall`, the value is waiting in `returnValue`. This avoids needing exceptions for `report`.

**Error recovery:** `interpreterRun` sets up the `setjmp`. On a runtime error, `longjmp` skips the normal `environmentRelease` calls. To avoid leaking, the interpreter keeps a `live` stack of every active scope. After an error it releases them all and resets to the global scope. This also lets the REPL continue after an error.

**`interpreterInit`** creates the global scope and captures it so it lives until exit.

---

## 6. The driver and build

### `src/main.c`: the entry point

- Parses the command line: a file path, or `--help`. Anything else prints the usage message and exits with code 64.
- Creates one `Interpreter`, and passes each piece of source to `interpreterRun()`.
- `runFile()` reads the entire file into a buffer and runs it. **The file extension is never checked**, so `.bc` is just a convention.
- `repl()` is an interactive loop: show the `mylang>` prompt, read a line, trim it, run it. It auto-adds a `;` if you forget one, so `broadcast 1 + 2` works. Errors are printed but the session continues. `help` prints a short language reference; `exit`, `quit` or end of input ends it.
- `readLine()` reads one line of any length. It replaces POSIX `getline()`, which Windows does not have.
- `--help` prints the usage text; an unknown option prints which argument was wrong, then the usage.
- Cleanup at the end: `objectsFreeAll()` and `arenaFreeAll()`.

### `CMakeLists.txt`

Defines the executable from all the `.c` files (C11, with warnings). GCC and Clang get `-Wall -Wextra -Wpedantic` and the math library; MSVC gets `/W3`, since it rejects the GCC flags. Build with `cmake -B build && cmake --build build`, or directly with:

```bash
gcc -std=c11 -O2 -Wall -Wextra src/*.c -lm -o mylang
```

`-lm` links the math library (needed for `fmod`).

### `tests/run_tests.sh`

It runs every `examples/*.bc`, captures stdout+stderr, and compares it to the matching `.expected` file, ignoring carriage returns so Windows output matches too. It prints PASS/FAIL per test and a total. With 8 examples, you see `8 passed`.

### `examples/`

| File | What it tests |
|---|---|
| `hello.bc` | Basic printing |
| `arithmetic.bc` | Integer arithmetic: operators, precedence, negatives, large numbers |
| `fib.bc` | Recursion |
| `logic.bc` | `and` / `or` / truthiness |
| `closures.bc` | Functions capturing variables |
| `scopes.bc` | Nested scopes, shadowing, deep closures |
| `error_runtime.bc` | A runtime error and its message |
| `error_syntax.bc` | A badly formatted expression, rejected before anything runs |

---

## 7. Tracing one program end to end

```
assign x = 2;
broadcast x * 3;
```

1. **Lexer** → `LET IDENT(x) EQUAL NUMBER(2) SEMICOLON PRINT IDENT(x) STAR NUMBER(3) SEMICOLON EOF`
2. **Parser** → `Program[ Let(x, Number 2), Print(Binary(*, Var x, Number 3)) ]`
3. **Interpreter:** `execute(Let)` stores `x = 2` in the global scope. `execute(Print)` evaluates the Binary: looks up `x` (2), evaluates 3, multiplies → 6, prints `6`.

---

## 8. Where to go next

Ideas, roughly in order of difficulty:

1. **New operators or keywords**: add a token in `token.h`, lex it in `lexer.c`, parse it in `parser.c`, then handle it in `interpreter.c`. Add an example with an `.expected` file.
2. **`for` loops**: the parser can translate them into `repeat`, so the interpreter does not change.
3. **More built-in functions** (e.g. `clock()`, `len()`).
4. **Arrays or maps**: new object types in `value.h`.
5. **A garbage collector**: walk `allObjects`, mark everything reachable from the global scope and the scopes still running, free the rest.
6. **A bytecode compiler and virtual machine**: a faster way to run programs (a Compiler Design topic).
7. **Better error recovery** in the parser, so it can report several syntax errors at once.

**The rule for any new feature:** add an example with an `.expected` file, so the test script checks it from then on.
