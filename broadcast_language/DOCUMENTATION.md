# broadcast_language: Project Documentation

**Course:** CMPSC 132: Programming Languages
**Term:** 1st semester, A.Y. 2025-2026
**Project:** Final Project: A Simple Programming Language
**Submitted:** October 2026

**Group members:**

| Name | Role |
|---|---|
| _Name 1_ | _e.g. lexer and parser_ |
| _Name 2_ | _e.g. interpreter and testing_ |
| _Name 3_ | _e.g. documentation and presentation_ |

---

## Contents

1. [What is broadcast_language?](#1-what-is-broadcast_language)
2. [How we met the project requirements](#2-how-we-met-the-project-requirements)
3. [Getting started](#3-getting-started)
4. [Learning the language](#4-learning-the-language)
5. [Arithmetic in detail](#5-arithmetic-in-detail)
6. [How broadcast_language handles mistakes](#6-how-broadcast_language-handles-mistakes)
7. [Sample programs and their output](#7-sample-programs-and-their-output)
8. [How broadcast_language works inside](#8-how-broadcast_language-works-inside)
9. [The source code, file by file](#9-the-source-code-file-by-file)
10. [Every data structure and its variables](#10-every-data-structure-and-its-variables)
11. [How broadcast_language meets the language evaluation criteria](#11-how-broadcast_language-meets-the-language-evaluation-criteria)
12. [Testing](#12-testing)
13. [Libraries we used](#13-libraries-we-used)
14. [Limits and future plans](#14-limits-and-future-plans)

---

## 1. What is broadcast_language?

broadcast_language is a small programming language that we built from scratch in the C programming language. You type in instructions, and broadcast_language reads them, checks them for mistakes, and carries them out.

Its main job is arithmetic: adding, subtracting, multiplying, dividing, and finding remainders, with whole numbers and decimals. On top of that it has the basic building blocks most languages share:

- **variables**, to store values and use them later
- **when / otherwise**, to make decisions
- **repeat loops**, to repeat things
- **functions**, to name a piece of work and reuse it
- **text (strings)**, to print readable messages

Here is a small broadcast_language program:

```
assign price = 250;
assign quantity = 4;
broadcast "Total: " + price * quantity;
```

It prints:

```
Total: 1000
```

You can use broadcast_language two ways: run a whole file of code, or type one line at a time in **interactive mode** and see each answer right away.

---

## 2. How we met the project requirements

The project guidelines list five general requirements. This table shows each one and where you can see it working.

| Requirement | How we met it | Where to see it |
|---|---|---|
| Process simple arithmetic, at least with integers | broadcast_language supports `+`, `-`, `*`, `/` and `%`, parentheses, negative numbers, and the normal order of operations. Whole numbers stay exact up to 9,007,199,254,740,992. | Section 5, and `examples/arithmetic.bc` |
| Handle errors like incorrect format | Every mistake is caught and explained in plain words, with the line number, the line of code, and a `^` pointing at the problem. The program never crashes. | Section 6, and `examples/error_syntax.bc` |
| Present correct output | We wrote down the correct output for every sample program, and an automatic test checks that broadcast_language gives exactly that output. All 8 checks pass. | Sections 7 and 12 |
| Follow the PL evaluation criteria | We checked broadcast_language against readability, writability, reliability and cost. | Section 11, and `docs/evaluation.md` |
| Proper documentation of the source code | Every source file starts with a note explaining it. Every function and every variable inside every data structure has a comment. This document explains them all in simple words. | Sections 9 and 10 |

---

## 3. Getting started

### What is in the folder

```
broadcast_language/
├── DOCUMENTATION.md    this document
├── README.md           a short overview
├── CMakeLists.txt      build instructions for CMake
├── src/                the source code of the language (written in C)
├── examples/           sample programs (.bc) and their correct output (.expected)
├── tests/              the automatic test script
└── docs/               extra guides
```

broadcast_language programs are saved in files ending in `.bc`.

### Step 1: Build the program

The source code has to be turned into a program the computer can run. This step is called **building** or **compiling**. Do it once.

**On Windows with Visual Studio installed**, open a terminal in the `broadcast_language` folder and type:

```
cmake -B build
cmake --build build --config Release
```

This makes the program `build\Release\broadcast_language.exe`.

**On a computer with gcc** (Linux, Mac, or MinGW on Windows), type:

```
gcc -std=c11 -O2 src/*.c -lm -o broadcast_language
```

This makes the program `broadcast_language` in the same folder.

### Step 2: Run it

The examples below use the Windows name, `.\build\Release\broadcast_language.exe`. With gcc, use `./broadcast_language` instead.

**To start interactive mode**, run the program by itself:

```
.\build\Release\broadcast_language.exe
```

You will see:

```
broadcast_language 1.0 - interactive mode
Type 'help' for examples, 'exit' to quit.

broadcast_language>
```

Now type any line and press Enter. In interactive mode you can leave out the `;` at the end of a line, because broadcast_language adds it for you. Type `help` to see examples of everything the language can do, and `exit` to leave.

**To run a file**, put its name after the program:

```
.\build\Release\broadcast_language.exe examples\arithmetic.bc
```

**Other option:**

| Option | What it does |
|---|---|
| `--help` | Lists the options. |

---

## 4. Learning the language

### Comments

Anything after `//` on a line is a note for people. broadcast_language ignores it.

```
// This is a comment.
broadcast 5;  // This is also a comment.
```

### Statements end with a semicolon

Each instruction ends with `;`, the same way a sentence ends with a period.

```
broadcast 1 + 1;
broadcast 2 + 2;
```

### Printing

`broadcast` shows a value on the screen.

```
broadcast 42;
broadcast "Hello!";
broadcast 3 * 7;
```

### Values

broadcast_language has five kinds of values:

| Kind | Examples | Notes |
|---|---|---|
| Numbers | `7`, `-3`, `2.5` | Whole numbers and decimals are the same kind of value. |
| Text (strings) | `"hello"`, `"Total: "` | Written inside double quotes. `\n` makes a new line and `\t` a tab. |
| True or false | `affirm`, `negative` | The answer to a yes-or-no question. |
| Nothing | `silence` | Means "no value". |
| Functions | `protocol square(n) { ... }` | A named piece of work you can use again. |

### Variables

A variable is a named box that holds a value. Make one with `assign`, and change it later with `=`.

```
assign score = 10;
broadcast score;        // 10
score = score + 5;
broadcast score;        // 15
```

You must create a variable with `assign` before you use it. This catches spelling mistakes (see Section 6).

### Joining text

When one side of `+` is text, broadcast_language joins the two sides into longer text:

```
assign name = "Ana";
assign age = 20;
broadcast "Name: " + name + ", age: " + age;   // Name: Ana, age: 20
```

### Comparing

These give `affirm` or `negative`:

| Symbol | Meaning | Example | Result |
|---|---|---|---|
| `==` | is equal to | `5 == 5` | `affirm` |
| `!=` | is not equal to | `5 != 3` | `affirm` |
| `<` | is less than | `2 < 8` | `affirm` |
| `<=` | is less than or equal to | `8 <= 8` | `affirm` |
| `>` | is greater than | `2 > 8` | `negative` |
| `>=` | is greater than or equal to | `9 >= 10` | `negative` |

### Combining true and false

| Word | Meaning | Example | Result |
|---|---|---|---|
| `and` | both must be true | `affirm and negative` | `negative` |
| `or` | at least one must be true | `affirm or negative` | `affirm` |
| `!` | flips it | `!affirm` | `negative` |

Only `negative` and `silence` count as "false". Everything else, including `0` and empty text `""`, counts as "true".

### Making decisions: when / otherwise

```
assign temperature = 31;
when (temperature > 30) {
  broadcast "It's hot.";
} otherwise {
  broadcast "It's fine.";
}
```

The condition goes in `( )`, and the code to run goes in `{ }`. The `otherwise` part is optional.

### Repeating: repeat

`repeat` repeats its code for as long as the condition is true.

```
assign count = 1;
repeat (count <= 3) {
  broadcast count;
  count = count + 1;
}
```

Output:

```
1
2
3
```

### Functions

A function is a named piece of work. Make one with `protocol`, give it inputs (called **parameters**), and send back a result with `report`.

```
protocol area(width, height) {
  report width * height;
}

broadcast area(5, 3);    // 15
broadcast area(10, 10);  // 100
```

A function can call itself. This is called **recursion**:

```
protocol factorial(n) {
  when (n <= 1) { report 1; }
  report n * factorial(n - 1);
}
broadcast factorial(5);  // 120
```

### Scope: where a variable can be seen

A variable made inside `{ }` only exists inside those braces. Once the block ends, it is gone.

```
assign x = "outside";
{
  assign x = "inside";
  broadcast x;   // inside
}
broadcast x;     // outside
```

### Every keyword at a glance

broadcast_language has only 12 keywords:

| Keyword | Use |
|---|---|
| `assign` | create a variable |
| `broadcast` | show a value |
| `when`, `otherwise` | make a decision |
| `repeat` | repeat |
| `protocol` | create a function |
| `report` | send a result back from a function |
| `and`, `or` | combine true/false values |
| `affirm`, `negative` | the two true/false values |
| `silence` | "no value" |

---

## 5. Arithmetic in detail

Arithmetic is the heart of this project, so this section covers exactly how it works.

### The five operators

| Operator | Meaning | Example | Result |
|---|---|---|---|
| `+` | add | `7 + 5` | `12` |
| `-` | subtract | `7 - 5` | `2` |
| `*` | multiply | `7 * 5` | `35` |
| `/` | divide | `20 / 4` | `5` |
| `%` | remainder after dividing | `17 % 5` | `2` |

A minus sign in front of a number or expression makes it negative: `-8`, `-(4 * 3)`.

### Order of operations

broadcast_language follows the order you learned in school:

1. Parentheses `( )` first
2. Then `*`, `/` and `%`
3. Then `+` and `-`
4. Operators at the same level go from **left to right**

| Expression | How broadcast_language reads it | Result |
|---|---|---|
| `2 + 3 * 4` | `2 + (3 * 4)` | `14` |
| `(2 + 3) * 4` | parentheses first | `20` |
| `100 - 10 - 1` | `(100 - 10) - 1` | `89` |
| `10 / 4 * 4` | `(10 / 4) * 4` | `10` |
| `2 * (3 + 4) * 5` | `2 * 7 * 5` | `70` |

### Whole numbers and decimals

broadcast_language stores every number as a 64-bit decimal number (called a "double" in C). Whole numbers are stored exactly, with no rounding, all the way up to **9,007,199,254,740,992**, and printed with every digit:

```
broadcast 2147483647 + 1;     // 2147483648
broadcast 1000000 * 1000000;  // 1000000000000
```

When a division does not come out even, you get the decimal answer, just like a calculator:

```
broadcast 7 / 2;   // 3.5
broadcast 1 / 3;   // 0.333333333333333
```

### The remainder operator with negative numbers

`%` gives what is left over after dividing. If the left number is negative, the answer is negative too, the same as in C and JavaScript:

```
broadcast 17 % 5;    // 2
broadcast -17 % 5;   // -2
```

### Arithmetic mistakes

Dividing by zero, or finding a remainder after dividing by zero, stops the program with a clear message instead of giving a wrong answer:

```
[line 1] Runtime error: Division by zero.
    1 | broadcast 10 / 0;
```

---

## 6. How broadcast_language handles mistakes

Everyone makes mistakes when typing code. A good language should catch them early and explain them clearly. broadcast_language does both, and it never crashes, whatever you type.

### What an error message looks like

Every message has the same shape:

```
[line 4] Syntax error: Expect expression. (found '*')
    4 | broadcast (3 + * 4;
      |                ^
```

1. **`[line 4]`** tells you which line has the problem.
2. **`Syntax error`** tells you what kind of mistake it is.
3. **The message** explains what went wrong, in words.
4. **`4 | broadcast (3 + * 4;`** shows the line itself.
5. **The `^`** points at the exact spot.

### The two kinds of mistakes

**Syntax errors (wrong format).** The code is not written in a way broadcast_language understands, like a sentence with bad grammar. broadcast_language checks the **whole file before running any of it**, so if there is a format mistake anywhere, nothing runs at all. That way a program never stops halfway because of a typo.

**Runtime errors.** The code is written correctly, but something goes wrong while it runs, like dividing by zero. The lines before the mistake have already run; broadcast_language stops at the mistake.

### Every mistake broadcast_language catches

**Format mistakes (syntax errors):**

| You typed | What broadcast_language says |
|---|---|
| `broadcast 3 +;` | `Expect expression. (found ';')`: something is missing after `+` |
| `broadcast 3 3;` | `Expect ';' after value. (found '3')`: two numbers with nothing between them |
| `broadcast (1 + 2;` | `Expect ')' after expression. (found ';')`: a bracket was never closed |
| `broadcast 1 + 2` (in a file) | `Expect ';' after value. (reached end of file)`: the `;` is missing |
| `broadcast 2 ** 3;` | `Expect expression. (found '*')`: `**` is not an operator in broadcast_language |
| `broadcast 5 $ 2;` | `Unexpected character '$'.`: a symbol broadcast_language does not know |
| `broadcast 1..2;` | `Unexpected character '.'.`: a badly written number |
| `broadcast "abc;` | `Unterminated string.`: text with no closing quote |
| `assign = 5;` | `Expect variable name after 'assign'. (found '=')`: the name is missing |
| `1 = 2;` | `Invalid assignment target.`: you can only store a value in a variable |

**Mistakes while running (runtime errors):**

| You typed | What broadcast_language says |
|---|---|
| `broadcast 10 / 0;` | `Division by zero.` |
| `broadcast 5 % 0;` | `Division by zero.` |
| `broadcast "a" - 1;` | `Operands must be numbers.`: you cannot subtract from text |
| `broadcast affirm + 1;` | `Operands of '+' must be two numbers or include a string.` |
| `broadcast totl;` | `Undefined variable 'totl'.`: probably a spelling mistake |
| `broadcast 5(3);` | `Can only call functions.`: `5` is not a function |
| `area(1, 2, 3);` when `area` takes 2 | `Expected 2 arguments but got 3.` |
| A function that calls itself forever | `Stack overflow: too many nested calls.` |

### Mistakes in interactive mode

In interactive mode, a mistake prints its message and then you can keep typing. Every variable you made before the mistake is still there.

### Exit codes

When a file finishes, broadcast_language reports a number to the system, so other programs and scripts can tell what happened:

| Code | Meaning |
|---|---|
| `0` | Everything ran fine |
| `64` | The command was typed wrong (for example, an unknown option) |
| `65` | The program has a format (syntax) mistake |
| `70` | The program stopped because of a runtime mistake |
| `74` | The file could not be found or opened |

---

## 7. Sample programs and their output

The `examples` folder has 8 sample programs. Next to each one is a `.expected` file holding its correct output. Here are the main ones.

### `arithmetic.bc`: integer arithmetic

```
broadcast 7 + 5;          // 12
broadcast 2 + 3 * 4;      // 14
broadcast (2 + 3) * 4;    // 20
broadcast -8 + 3;         // -5
broadcast 2147483647 + 1;
broadcast 7 / 2;          // 3.5

assign width = 12;
assign height = 7;
broadcast "area = " + width * height;

protocol power(base, exp) {
  assign result = 1;
  repeat (exp > 0) {
    result = result * base;
    exp = exp - 1;
  }
  report result;
}
broadcast "2^10 = " + power(2, 10);
```

(This is a shortened version. The full file has more lines.) Output of the full file:

```
-- basic operators --
12
2
35
5
2
-- precedence and grouping --
14
20
89
70
-- negative numbers --
-5
-7
-12
-2
-- large whole numbers stay exact --
2147483648
1000000000000
9007199254740992
-- division that does not come out even --
3.5
0.333333333333333
-- variables and expressions --
area = 84
perimeter = 38
2^10 = 1024
3^5 = 243
```

### `hello.bc`: the basics

```
// The first program in the language.
broadcast "Hello, world!";
assign name = "broadcast_language";
broadcast "Welcome to " + name + "!";
broadcast 1 + 2 * 3;
```

Output:

```
Hello, world!
Welcome to broadcast_language!
7
```

### `fib.bc`: loops and recursion

This prints the Fibonacci numbers, where each number is the sum of the two before it.

```
protocol fib(n) {
  when (n < 2) { report n; }
  report fib(n - 1) + fib(n - 2);
}

assign i = 0;
repeat (i <= 10) {
  broadcast "fib(" + i + ") = " + fib(i);
  i = i + 1;
}
```

Output:

```
fib(0) = 0
fib(1) = 1
fib(2) = 1
fib(3) = 2
fib(4) = 3
fib(5) = 5
fib(6) = 8
fib(7) = 13
fib(8) = 21
fib(9) = 34
fib(10) = 55
```

### `error_syntax.bc`: a format mistake

```
broadcast 1 + 2;
broadcast (3 + * 4;
broadcast "never printed";
```

(The real file has two comment lines on top, so the mistake is on line 4.) Output:

```
[line 4] Syntax error: Expect expression. (found '*')
    4 | broadcast (3 + * 4;
      |                ^
```

Notice that even `broadcast 1 + 2;` did not run. broadcast_language found the mistake first and refused to start.

### `error_runtime.bc`: a mistake while running

Output:

```
before
[line 3] Runtime error: Undefined variable 'missing'.
    3 | broadcast missing;
```

Here the first line ran and printed `before`, then broadcast_language stopped at the unknown variable.

### The other samples

| File | What it shows |
|---|---|
| `logic.bc` | `and`, `or`, `!`, and which values count as true or false |
| `scopes.bc` | Variables inside `{ }` blocks, and the same name used at different levels |
| `closures.bc` | Functions that remember their own private variables |

---

## 8. How broadcast_language works inside

When you run a broadcast_language program, your code passes through a few stages, like a product on an assembly line. Each stage has one job and hands its result to the next.

```
 Your code (text)
      │
      ▼
 ┌─────────┐
 │  Lexer  │  splits the text into "words" called tokens
 └─────────┘
      │
      ▼
 ┌─────────┐
 │ Parser  │  checks the grammar and builds a tree
 └─────────┘
      │
      │
      ▼
 ┌─────────────┐
 │ Interpreter │  walks the tree and does the work
 └─────────────┘
```

### Stage 1: The lexer (splitting into words)

Before you can understand a sentence, you have to see the separate words. The lexer reads your code one character at a time and groups the characters into **tokens**, the "words" of the language.

`broadcast 12 + x;` becomes five tokens:

| Token | Kind |
|---|---|
| `broadcast` | keyword |
| `12` | number |
| `+` | plus sign |
| `x` | name |
| `;` | semicolon |

The lexer also skips spaces and comments. If it meets a character it does not know, like `$`, it reports a syntax error.

### Stage 2: The parser (checking the grammar)

The parser checks that the tokens are in an order that makes sense, the way a grammar teacher checks a sentence. It follows a set of grammar rules (written out in full at the top of `src/parser.h`).

As it checks, it builds a **syntax tree**: a diagram of what the code means. For `2 + 3 * 4`:

```
      +
     / \
    2   *
       / \
      3   4
```

The multiplication sits lower in the tree, so it is done first. This is how broadcast_language gets the order of operations right. The parser has one rule for each level of precedence: the rule for `+` and `-` asks the rule for `*` and `/` to read each side first, so `*` and `/` always end up deeper in the tree.

### Stage 3: The interpreter (doing the work)

The interpreter walks through the tree and does the work. To work out `+`, it first works out its left side (`2`) and its right side (`3 * 4` = `12`), then adds them to get `14`. Because it works directly on the tree, this kind of interpreter is called a **tree-walking interpreter**.

### How errors travel back

C does not have a built-in way to "throw" an error from deep inside the program back to the top. We used two standard C tools, `setjmp` and `longjmp`. Before a stage starts, `setjmp` marks a safe return point. When something goes wrong, `longjmp` jumps straight back to that point, however deep inside the program the mistake happened. There, broadcast_language prints the message and stops cleanly, or carries on with the next line in interactive mode.

### How memory is handled

- The tokens and the tree are stored in one big memory area called an **arena**. When the program ends, the whole arena is freed in one go. This is simple and fast, and nothing gets lost.
- Values made while running, such as text and functions, are kept on one list and freed together when the program ends.

---

## 9. The source code, file by file

All source code is in the `src` folder. Each `.h` file (header) describes what a part offers; the `.c` file with the same name contains the actual code. Every file starts with a comment explaining its job.

| File | Its job, in simple words |
|---|---|
| `main.c` | The starting point. Reads the command, then runs a file or starts interactive mode. Holds the help text and the `broadcast_language>` prompt. |
| `token.h` | The list of every kind of token (word) the language has. |
| `lexer.h`, `lexer.c` | Stage 1: splits the code text into tokens. |
| `ast.h` | Describes the pieces of the syntax tree. ("AST" means abstract syntax tree.) |
| `parser.h`, `parser.c` | Stage 2: checks the grammar and builds the tree. The full grammar is written at the top of `parser.h`. |
| `value.h`, `value.c` | Describes every kind of value (numbers, text, true/false, nil, functions) and how to compare and print them. Also has the lookup table used to find variables by name. |
| `interpreter.h`, `interpreter.c` | Stage 3: walks the tree and runs the program. |
| `errors.h`, `errors.c` | Catches errors and prints them with the line number and the `^` marker. |
| `arena.h`, `arena.c` | The memory area for tokens and the tree, freed all at once at the end. |

Other files:

| File | Its job |
|---|---|
| `CMakeLists.txt` | Instructions for building the program with CMake, on Windows, Mac or Linux. |
| `tests/run_tests.sh` | Runs every sample program and checks the output. |
| `docs/code-guide.md` | A longer, more technical walk-through of every file. |
| `docs/evaluation.md` | The full evaluation against the language criteria. |

---

## 10. Every data structure and its variables

C does not have classes. Instead it has **structures** (`struct`): a group of variables kept together under one name, which is what a class's variables are. This section describes every structure in broadcast_language and every variable inside it. The same descriptions are written as comments next to each variable in the source code.

A few words that come up often:

- **slot**: one space in a list that can hold one item
- **allocated slots** (`capacity`): how many spaces have been made ready; when they fill up, the list is made bigger
- **pointer**: a variable that holds the location of something else, so it can be found

### Tokens and the syntax tree

**`Token`** (`token.h`): one "word" of the code, made by the lexer.

| Variable | Type | What it holds |
|---|---|---|
| `type` | `TokenType` | What kind of token it is: a number, a name, `+`, `repeat`, and so on. |
| `lexeme` | `const char*` | The token's actual text from the code. For text in quotes, the text without the quotes. |
| `length` | `size_t` | How many characters `lexeme` has. |
| `number` | `double` | The value, if the token is a number. |
| `line` | `int` | The line the token is on, used in error messages. |
| `offset` | `size_t` | How far into the code the token starts, so an error's `^` can point at it. |

**`Lexer`** (`lexer.c`): what the lexer keeps track of while splitting the code into tokens.

| Variable | Type | What it holds |
|---|---|---|
| `source` | `const char*` | The code being read. |
| `length` | `size_t` | How long the code is. |
| `start` | `size_t` | Where the token being read now begins. |
| `current` | `size_t` | The position of the next character to read. |
| `line` | `int` | The line number it is on now. |
| `tokens` | `Token*` | The tokens made so far. |
| `count` | `int` | How many tokens have been made. |
| `capacity` | `int` | How many allocated slots `tokens` has. |

**`Parser`** (`parser.c`): what the parser keeps track of while checking the grammar.

| Variable | Type | What it holds |
|---|---|---|
| `tokens` | `const Token*` | The list of tokens from the lexer. The last one marks the end of the file. |
| `count` | `int` | How many tokens there are. |
| `current` | `int` | The position of the next token to look at. |

**`Expr`** (`ast.h`): one piece of the syntax tree that produces a value, such as `2 + 3`, `x`, or `area(5, 3)`. Different kinds of pieces use different variables.

| Variable | Type | What it holds |
|---|---|---|
| `kind` | `ExprKind` | What kind of piece this is: a number, text, a variable, an operation, a function call, and so on. It decides which of the variables below are used. |
| `line` | `int` | The line it came from, for error messages. |
| `number` | `double` | The value, for a number. |
| `boolean` | `bool` | The value, for `affirm` or `negative`. |
| `text` | `const char*` | The text, for text in quotes; or the variable's name, for a variable. |
| `textLength` | `size_t` | How long `text` is. |
| `operatorType` | `TokenType` | Which operator, for an operation such as `+`, `<` or `and`. |
| `left` | `Expr*` | The left side of an operation, the value being stored, or the function being called. |
| `right` | `Expr*` | The right side of an operation. |
| `arguments` | `Expr**` | The inputs given to a function call. |
| `argumentCount` | `int` | How many inputs were given. |

**`Stmt`** (`ast.h`): one statement (instruction) in the syntax tree, such as `broadcast`, `assign`, `when`, `repeat` or a function definition.

| Variable | Type | What it holds |
|---|---|---|
| `kind` | `StmtKind` | What kind of statement this is. It decides which of the variables below are used. |
| `line` | `int` | The line it came from, for error messages. |
| `name` | `const char*` | The name being created, for `assign` and `protocol`. |
| `expression` | `Expr*` | The value used by the statement: what to print, the starting value for `assign`, the condition for `when` and `repeat`, or what to `report`. Can be empty. |
| `body` | `Stmt**` | The statements inside a `{ }` block or a function. |
| `bodyCount` | `int` | How many statements are in `body`. |
| `thenBranch` | `Stmt*` | What to do when an `when` is true, or the code a `repeat` repeats. |
| `elseBranch` | `Stmt*` | What to do when an `when` is false. Empty if there is no `otherwise`. |
| `parameters` | `const char**` | The names of a function's inputs (parameters). |
| `parameterCount` | `int` | How many inputs the function takes. |

**`Program`** (`ast.h`): a whole parsed program.

| Variable | Type | What it holds |
|---|---|---|
| `statements` | `Stmt**` | All the top-level statements, in order. |
| `count` | `int` | How many statements there are. |

### Values

**`Value`** (`value.h`): any single value in broadcast_language: a number, true/false, nil, text or a function.

| Variable | Type | What it holds |
|---|---|---|
| `type` | `ValueType` | Which kind of value it is: nil, number, true/false, or an object (text or a function). |
| `as.number` | `double` | The number, if it is a number. |
| `as.boolean` | `bool` | `affirm` or `negative`, if it is a true/false value. |
| `as.object` | `Obj*` | Where to find the object, if it is text or a function. |

(`as` holds only one of its three variables at a time, the one that matches `type`. This is called a `union` in C, and it saves memory.)

**`Obj`** (`value.h`): the shared header at the start of every object (text, scope or function).

| Variable | Type | What it holds |
|---|---|---|
| `type` | `ObjType` | Which kind of object it is. |
| `next` | `Obj*` | The next object on the list of all objects, which is used to free them all at the end. |

**`ObjString`** (`value.h`): a piece of text.

| Variable | Type | What it holds |
|---|---|---|
| `object` | `Obj` | The shared object header. |
| `length` | `size_t` | How many characters the text has. |
| `characters` | `char[]` | The characters themselves. |

**`TableEntry`** (`value.h`): one name-and-value pair in a lookup table.

| Variable | Type | What it holds |
|---|---|---|
| `key` | `const char*` | The name, such as a variable name. Empty means this slot is free. |
| `hash` | `uint32_t` | A number worked out from the name, used to find it quickly. It is saved so it never has to be worked out again. |
| `value` | `Value` | The value stored under that name. |

**`Table`** (`value.h`): a lookup table that finds a value by its name quickly. Used to store variables.

| Variable | Type | What it holds |
|---|---|---|
| `entries` | `TableEntry*` | The slots holding the name-and-value pairs. |
| `count` | `int` | How many slots are in use. |
| `capacity` | `int` | How many slots there are in total. |

### Values used by the interpreter

**`ObjEnv`** (`value.h`): a **scope**, meaning one level of variables, such as the variables inside one `{ }` block or one function call.

| Variable | Type | What it holds |
|---|---|---|
| `object` | `Obj` | The shared object header. |
| `parent` | `ObjEnv*` | The scope around this one. If a name is not found here, broadcast_language looks there next. Empty for the outermost (global) scope. |
| `variables` | `Table` | The variables made in this scope, by name. |
| `captured` | `bool` | True once a function needs to remember this scope. Then it is kept until the program ends. |

**`ObjFunction`** (`value.h`): a function, as the interpreter stores it.

| Variable | Type | What it holds |
|---|---|---|
| `object` | `Obj` | The shared object header. |
| `declaration` | `const Stmt*` | The function's definition in the tree: its name, inputs and body. |
| `closure` | `ObjEnv*` | The scope where the function was made. This is how a function remembers the variables around it. |

**`Interpreter`** (`interpreter.h`): everything the interpreter keeps track of while running.

| Variable | Type | What it holds |
|---|---|---|
| `globals` | `ObjEnv*` | The outermost scope, which lasts for the whole session. |
| `environment` | `ObjEnv*` | The scope being used right now. |
| `live` | `ObjEnv**` | The scopes of every block and function call still running, so they can be cleaned up if an error happens. |
| `liveCount` | `int` | How many scopes are in `live`. |
| `liveCapacity` | `int` | How many allocated slots `live` has. |
| `callDepth` | `int` | How many function calls are running inside each other, to catch a function that calls itself forever. |
| `returnValue` | `Value` | The value a `report` statement is sending back. |

### Errors, memory and the program itself

**`ErrorState`** (`errors.h`): the details of the most recent error.

| Variable | Type | What it holds |
|---|---|---|
| `jumpBuffer` | `jmp_buf` | The safe return point that the error jumps back to (see Section 8). |
| `kind` | `ErrorKind` | What kind of error it is: syntax or runtime. |
| `line` | `int` | The line the error is on. |
| `offset` | `long` | The exact position of the mistake, for the `^` marker. `-1` when only the line is known. |
| `message` | `char[512]` | The error message in words. |

**`Block`** (`arena.c`): one large piece of memory in the arena.

| Variable | Type | What it holds |
|---|---|---|
| `next` | `Block*` | The block made before this one. All blocks are linked in a chain so they can be freed together. |
| `used` | `size_t` | How much of this block has been handed out. |
| `size` | `size_t` | How much space this block has in total. |
| `align` | `union` | Not used to store anything. It makes sure the memory after it is lined up correctly for any kind of value. |

**`PtrVec`** (`arena.h`): a list that grows as items are added. The parser uses it to collect statements, inputs and parameters.

| Variable | Type | What it holds |
|---|---|---|
| `items` | `void**` | The items, in the order they were added. |
| `count` | `int` | How many items there are. |
| `capacity` | `int` | How many allocated slots `items` has. |

---

## 11. How broadcast_language meets the language evaluation criteria

Programming languages are usually judged on four main things. Here is how broadcast_language does on each, in short. The full version, with more examples, is in `docs/evaluation.md`.

### Readability: is the code easy to read?

- **It's small.** There are only 12 keywords and one kind of number, so there is little to learn and nothing confusing.
- **One way to do each thing.** For example, to add 1 to a variable there is only `x = x + 1`, not three different ways.
- **The words say what they do.** `assign` makes a variable, `broadcast` prints, `protocol` makes a function.
- **Real true and false values.** Conditions use `affirm` and `negative`, not `1` and `0`.
- **Blocks are always in `{ }`,** so it is always clear which code belongs to a `when` or a loop.

### Writability: is the code easy to write?

- **The pieces fit together freely.** Any value can be printed, stored, compared, passed to a function, or returned from one.
- **Functions** let you name a piece of work once and reuse it, including recursion.
- **Text joins easily:** `broadcast "Total: " + total;` needs no extra conversion step.
- **Interactive mode** lets you try ideas one line at a time.

### Reliability: does the code do what you expect?

- **Types are checked.** You cannot subtract a number from text, and broadcast_language says so instead of giving a strange answer.
- **Variables must be created first** with `assign`, which catches spelling mistakes.
- **Function inputs are counted.** Giving a function the wrong number of inputs is an error.
- **Every mistake is reported clearly** with the line and the exact spot, and the program never crashes.
- **Format mistakes are caught before running,** so a program never stops halfway because of a typo.

### Cost: how much does it take to learn, build and run?

- **Easy to learn:** it looks like C, Java and JavaScript, and `help` lists everything.
- **Easy to build:** standard C only, with no extra libraries to install. It builds on Windows, Mac and Linux.
- **Fast to start:** a program starts running in a fraction of a second.
- **Easy to maintain:** every file, function and variable in the source code is explained.

---

## 12. Testing

We tested broadcast_language in three ways.

**1. Automatic tests.** The script `tests/run_tests.sh` runs all 8 sample programs. It compares each result with the correct output saved in the matching `.expected` file. Result:

```
8 passed, 0 failed
```

This proves that every sample gives the correct output. To run it (on Windows, use Git Bash):

```
tests/run_tests.sh build/Release/broadcast_language.exe
```

**2. Mistake testing.** We typed in every kind of wrong input we could think of (the tables in Section 6) and checked that each one gives a clear message and never crashes.

**3. Edge cases.** We also checked:

| Test | Result |
|---|---|
| Very large whole numbers (`9007199254740992`) | Printed exactly, every digit |
| Dividing by zero, and remainder by zero | Clear error message |
| A function that calls itself 999 times | Works correctly |
| A function that calls itself forever | Stops cleanly with "Stack overflow" |
| A code file saved by Windows Notepad (which adds a hidden mark at the start) | Runs correctly |
| A file that does not exist | `Could not open file` message |
| An unknown option, like `--bogus` | Says which option is wrong, then shows the correct usage |

---

## 13. Libraries we used

The guidelines allow libraries, as long as we understand them. **We did not use any outside libraries.** Everything (the lexer, parser, interpreter, memory arena and lookup table) was written by us. We only used parts of the C standard library, which comes with every C compiler:

| Library | What we used it for |
|---|---|
| `stdio.h` | Reading files, printing output and error messages |
| `stdlib.h` | Getting and freeing memory (`malloc`, `calloc`, `realloc`, `free`), and turning text into numbers (`strtod`) |
| `string.h` | Copying and comparing text (`memcpy`, `strcmp`, `strlen`) |
| `math.h` | The remainder of decimal numbers (`fmod`), and rounding checks for printing numbers (`floor`, `fabs`) |
| `ctype.h` | Telling whether a character is a letter or a digit |
| `setjmp.h` | Jumping back to a safe point when an error happens (Section 8) |
| `stdarg.h` | Building error messages that include details, such as the variable's name |
| `stdbool.h`, `stddef.h`, `stdint.h` | Basic types such as `bool`, `size_t` and `uint32_t` |

---

## 14. Limits and future plans

No language is perfect, and here are broadcast_language's limits:

- **No separate integer type.** Numbers are exact up to 9,007,199,254,740,992. Past that, whole numbers may be rounded.
- **Type mistakes are found only while running.** A mistake in a part of the code that never runs is not reported.
- **No `for` loop or `otherwise when`.** A `repeat` loop, and a `when` inside an `otherwise`, do the same jobs with a little more typing.
- **No lists.** A variable holds one value at a time.
- **One mistake at a time.** broadcast_language stops at the first format mistake, so fixing several takes several runs.
- **Memory is freed only at the end.** A program that runs for a very long time and makes a lot of text keeps using more memory.

What we would add next:

1. `for` loops, `otherwise when` and `break`
2. Built-in functions such as `len(text)` and `clock()`
3. Lists, such as `[1, 2, 3]`
4. A separate whole-number type with overflow checks
5. A garbage collector that frees unused memory while the program runs
