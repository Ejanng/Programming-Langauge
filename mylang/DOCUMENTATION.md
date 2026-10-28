# mylang: Project Documentation

**Course:** CMPSC 132: Programming Languages
**Term:** 1st semester, A.Y. 2025-2026
**Project:** Final Project: A Simple Programming Language
**Submitted:** October 2026

**Group members:**

| Name | Role |
|---|---|
| _Name 1_ | _e.g. lexer and parser_ |
| _Name 2_ | _e.g. interpreter and testing_ |
| _Name 3_ | _e.g. bytecode VM and documentation_ |

---

## Contents

1. [What is mylang?](#1-what-is-mylang)
2. [How we met the project requirements](#2-how-we-met-the-project-requirements)
3. [Getting started](#3-getting-started)
4. [Learning the language](#4-learning-the-language)
5. [Arithmetic in detail](#5-arithmetic-in-detail)
6. [How mylang handles mistakes](#6-how-mylang-handles-mistakes)
7. [Sample programs and their output](#7-sample-programs-and-their-output)
8. [How mylang works inside](#8-how-mylang-works-inside)
9. [The source code, file by file](#9-the-source-code-file-by-file)
10. [Every data structure and its variables](#10-every-data-structure-and-its-variables)
11. [How mylang meets the language evaluation criteria](#11-how-mylang-meets-the-language-evaluation-criteria)
12. [Testing](#12-testing)
13. [Libraries we used](#13-libraries-we-used)
14. [Limits and future plans](#14-limits-and-future-plans)

---

## 1. What is mylang?

mylang is a small programming language that we built from scratch in the C programming language. You type in instructions, and mylang reads them, checks them for mistakes, and carries them out.

Its main job is arithmetic: adding, subtracting, multiplying, dividing, and finding remainders, with whole numbers and decimals. On top of that it has the basic building blocks most languages share:

- **variables**, to store values and use them later
- **if / else**, to make decisions
- **while loops**, to repeat things
- **functions**, to name a piece of work and reuse it
- **text (strings)**, to print readable messages

Here is a small mylang program:

```
let price = 250;
let quantity = 4;
print "Total: " + price * quantity;
```

It prints:

```
Total: 1000
```

You can use mylang two ways: run a whole file of code, or type one line at a time in **interactive mode** and see each answer right away.

---

## 2. How we met the project requirements

The project guidelines list five general requirements. This table shows each one and where you can see it working.

| Requirement | How we met it | Where to see it |
|---|---|---|
| Process simple arithmetic, at least with integers | mylang supports `+`, `-`, `*`, `/` and `%`, parentheses, negative numbers, and the normal order of operations. Whole numbers stay exact up to 9,007,199,254,740,992. | Section 5, and `examples/arithmetic.rene` |
| Handle errors like incorrect format | Every mistake is caught and explained in plain words, with the line number, the line of code, and a `^` pointing at the problem. The program never crashes. | Section 6, and `examples/error_syntax.rene` |
| Present correct output | We wrote down the correct output for every sample program, and an automatic test checks that mylang gives exactly that output. All 16 checks pass. | Sections 7 and 12 |
| Follow the PL evaluation criteria | We checked mylang against readability, writability, reliability and cost. | Section 11, and `docs/evaluation.md` |
| Proper documentation of the source code | Every source file starts with a note explaining it. Every function and every variable inside every data structure has a comment. This document explains them all in simple words. | Sections 9 and 10 |

---

## 3. Getting started

### What is in the folder

```
mylang/
├── DOCUMENTATION.md    this document
├── README.md           a short overview
├── CMakeLists.txt      build instructions for CMake
├── src/                the source code of the language (written in C)
├── examples/           sample programs (.rene) and their correct output (.expected)
├── tests/              the automatic test script
└── docs/               extra guides
```

mylang programs are saved in files ending in `.rene`.

### Step 1: Build the program

The source code has to be turned into a program the computer can run. This step is called **building** or **compiling**. Do it once.

**On Windows with Visual Studio installed**, open a terminal in the `mylang` folder and type:

```
cmake -B build
cmake --build build --config Release
```

This makes the program `build\Release\mylang.exe`.

**On a computer with gcc** (Linux, Mac, or MinGW on Windows), type:

```
gcc -std=c11 -O2 src/*.c src/vm/*.c -lm -o mylang
```

This makes the program `mylang` in the same folder.

### Step 2: Run it

The examples below use the Windows name, `.\build\Release\mylang.exe`. With gcc, use `./mylang` instead.

**To start interactive mode**, run the program by itself:

```
.\build\Release\mylang.exe
```

You will see:

```
mylang 1.0 - interactive mode (tree-walking interpreter)
Type 'help' for examples, 'exit' to quit.

mylang>
```

Now type any line and press Enter. In interactive mode you can leave out the `;` at the end of a line, because mylang adds it for you. Type `help` to see examples of everything the language can do, and `exit` to leave.

**To run a file**, put its name after the program:

```
.\build\Release\mylang.exe examples\arithmetic.rene
```

**Other options:**

| Option | What it does |
|---|---|
| `--vm` | Runs the program on our bytecode virtual machine instead of the default interpreter. The output is the same; only the method changes (see Section 8). |
| `--dump` | Shows the bytecode (the step-by-step instructions) the program was turned into, then runs it. |
| `--help` | Lists these options. |

---

## 4. Learning the language

### Comments

Anything after `//` on a line is a note for people. mylang ignores it.

```
// This is a comment.
print 5;  // This is also a comment.
```

### Statements end with a semicolon

Each instruction ends with `;`, the same way a sentence ends with a period.

```
print 1 + 1;
print 2 + 2;
```

### Printing

`print` shows a value on the screen.

```
print 42;
print "Hello!";
print 3 * 7;
```

### Values

mylang has five kinds of values:

| Kind | Examples | Notes |
|---|---|---|
| Numbers | `7`, `-3`, `2.5` | Whole numbers and decimals are the same kind of value. |
| Text (strings) | `"hello"`, `"Total: "` | Written inside double quotes. `\n` makes a new line and `\t` a tab. |
| True or false | `true`, `false` | The answer to a yes-or-no question. |
| Nothing | `nil` | Means "no value". |
| Functions | `fn square(n) { ... }` | A named piece of work you can use again. |

### Variables

A variable is a named box that holds a value. Make one with `let`, and change it later with `=`.

```
let score = 10;
print score;        // 10
score = score + 5;
print score;        // 15
```

You must create a variable with `let` before you use it. This catches spelling mistakes (see Section 6).

### Joining text

When one side of `+` is text, mylang joins the two sides into longer text:

```
let name = "Ana";
let age = 20;
print "Name: " + name + ", age: " + age;   // Name: Ana, age: 20
```

### Comparing

These give `true` or `false`:

| Symbol | Meaning | Example | Result |
|---|---|---|---|
| `==` | is equal to | `5 == 5` | `true` |
| `!=` | is not equal to | `5 != 3` | `true` |
| `<` | is less than | `2 < 8` | `true` |
| `<=` | is less than or equal to | `8 <= 8` | `true` |
| `>` | is greater than | `2 > 8` | `false` |
| `>=` | is greater than or equal to | `9 >= 10` | `false` |

### Combining true and false

| Word | Meaning | Example | Result |
|---|---|---|---|
| `and` | both must be true | `true and false` | `false` |
| `or` | at least one must be true | `true or false` | `true` |
| `!` | flips it | `!true` | `false` |

Only `false` and `nil` count as "false". Everything else, including `0` and empty text `""`, counts as "true".

### Making decisions: if / else

```
let temperature = 31;
if (temperature > 30) {
  print "It's hot.";
} else {
  print "It's fine.";
}
```

The condition goes in `( )`, and the code to run goes in `{ }`. The `else` part is optional.

### Repeating: while

`while` repeats its code for as long as the condition is true.

```
let count = 1;
while (count <= 3) {
  print count;
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

A function is a named piece of work. Make one with `fn`, give it inputs (called **parameters**), and send back a result with `return`.

```
fn area(width, height) {
  return width * height;
}

print area(5, 3);    // 15
print area(10, 10);  // 100
```

A function can call itself. This is called **recursion**:

```
fn factorial(n) {
  if (n <= 1) { return 1; }
  return n * factorial(n - 1);
}
print factorial(5);  // 120
```

### Scope: where a variable can be seen

A variable made inside `{ }` only exists inside those braces. Once the block ends, it is gone.

```
let x = "outside";
{
  let x = "inside";
  print x;   // inside
}
print x;     // outside
```

### Every keyword at a glance

mylang has only 12 keywords:

| Keyword | Use |
|---|---|
| `let` | create a variable |
| `print` | show a value |
| `if`, `else` | make a decision |
| `while` | repeat |
| `fn` | create a function |
| `return` | send a result back from a function |
| `and`, `or` | combine true/false values |
| `true`, `false` | the two true/false values |
| `nil` | "no value" |

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

mylang follows the order you learned in school:

1. Parentheses `( )` first
2. Then `*`, `/` and `%`
3. Then `+` and `-`
4. Operators at the same level go from **left to right**

| Expression | How mylang reads it | Result |
|---|---|---|
| `2 + 3 * 4` | `2 + (3 * 4)` | `14` |
| `(2 + 3) * 4` | parentheses first | `20` |
| `100 - 10 - 1` | `(100 - 10) - 1` | `89` |
| `10 / 4 * 4` | `(10 / 4) * 4` | `10` |
| `2 * (3 + 4) * 5` | `2 * 7 * 5` | `70` |

### Whole numbers and decimals

mylang stores every number as a 64-bit decimal number (called a "double" in C). Whole numbers are stored exactly, with no rounding, all the way up to **9,007,199,254,740,992**, and printed with every digit:

```
print 2147483647 + 1;     // 2147483648
print 1000000 * 1000000;  // 1000000000000
```

When a division does not come out even, you get the decimal answer, just like a calculator:

```
print 7 / 2;   // 3.5
print 1 / 3;   // 0.333333333333333
```

### The remainder operator with negative numbers

`%` gives what is left over after dividing. If the left number is negative, the answer is negative too, the same as in C and JavaScript:

```
print 17 % 5;    // 2
print -17 % 5;   // -2
```

### Arithmetic mistakes

Dividing by zero, or finding a remainder after dividing by zero, stops the program with a clear message instead of giving a wrong answer:

```
[line 1] Runtime error: Division by zero.
    1 | print 10 / 0;
```

---

## 6. How mylang handles mistakes

Everyone makes mistakes when typing code. A good language should catch them early and explain them clearly. mylang does both, and it never crashes, whatever you type.

### What an error message looks like

Every message has the same shape:

```
[line 4] Syntax error: Expect expression. (found '*')
    4 | print (3 + * 4;
      |            ^
```

1. **`[line 4]`** tells you which line has the problem.
2. **`Syntax error`** tells you what kind of mistake it is.
3. **The message** explains what went wrong, in words.
4. **`4 | print (3 + * 4;`** shows the line itself.
5. **The `^`** points at the exact spot.

### The two kinds of mistakes

**Syntax errors (wrong format).** The code is not written in a way mylang understands, like a sentence with bad grammar. mylang checks the **whole file before running any of it**, so if there is a format mistake anywhere, nothing runs at all. That way a program never stops halfway because of a typo.

**Runtime errors.** The code is written correctly, but something goes wrong while it runs, like dividing by zero. The lines before the mistake have already run; mylang stops at the mistake.

### Every mistake mylang catches

**Format mistakes (syntax errors):**

| You typed | What mylang says |
|---|---|
| `print 3 +;` | `Expect expression. (found ';')`: something is missing after `+` |
| `print 3 3;` | `Expect ';' after value. (found '3')`: two numbers with nothing between them |
| `print (1 + 2;` | `Expect ')' after expression. (found ';')`: a bracket was never closed |
| `print 1 + 2` (in a file) | `Expect ';' after value. (reached end of file)`: the `;` is missing |
| `print 2 ** 3;` | `Expect expression. (found '*')`: `**` is not an operator in mylang |
| `print 5 $ 2;` | `Unexpected character '$'.`: a symbol mylang does not know |
| `print 1..2;` | `Unexpected character '.'.`: a badly written number |
| `print "abc;` | `Unterminated string.`: text with no closing quote |
| `let = 5;` | `Expect variable name after 'let'. (found '=')`: the name is missing |
| `1 = 2;` | `Invalid assignment target.`: you can only store a value in a variable |

**Mistakes while running (runtime errors):**

| You typed | What mylang says |
|---|---|
| `print 10 / 0;` | `Division by zero.` |
| `print 5 % 0;` | `Division by zero.` |
| `print "a" - 1;` | `Operands must be numbers.`: you cannot subtract from text |
| `print true + 1;` | `Operands of '+' must be two numbers or include a string.` |
| `print totl;` | `Undefined variable 'totl'.`: probably a spelling mistake |
| `print 5(3);` | `Can only call functions.`: `5` is not a function |
| `area(1, 2, 3);` when `area` takes 2 | `Expected 2 arguments but got 3.` |
| A function that calls itself forever | `Stack overflow: too many nested calls.` |

### Mistakes in interactive mode

In interactive mode, a mistake prints its message and then you can keep typing. Every variable you made before the mistake is still there.

### Exit codes

When a file finishes, mylang reports a number to the system, so other programs and scripts can tell what happened:

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

### `arithmetic.rene`: integer arithmetic

```
print 7 + 5;          // 12
print 2 + 3 * 4;      // 14
print (2 + 3) * 4;    // 20
print -8 + 3;         // -5
print 2147483647 + 1;
print 7 / 2;          // 3.5

let width = 12;
let height = 7;
print "area = " + width * height;

fn power(base, exp) {
  let result = 1;
  while (exp > 0) {
    result = result * base;
    exp = exp - 1;
  }
  return result;
}
print "2^10 = " + power(2, 10);
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

### `hello.rene`: the basics

```
// The first program in the language.
print "Hello, world!";
let name = "mylang";
print "Welcome to " + name + "!";
print 1 + 2 * 3;
```

Output:

```
Hello, world!
Welcome to mylang!
7
```

### `fib.rene`: loops and recursion

This prints the Fibonacci numbers, where each number is the sum of the two before it.

```
fn fib(n) {
  if (n < 2) { return n; }
  return fib(n - 1) + fib(n - 2);
}

let i = 0;
while (i <= 10) {
  print "fib(" + i + ") = " + fib(i);
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

### `error_syntax.rene`: a format mistake

```
print 1 + 2;
print (3 + * 4;
print "never printed";
```

(The real file has two comment lines on top, so the mistake is on line 4.) Output:

```
[line 4] Syntax error: Expect expression. (found '*')
    4 | print (3 + * 4;
      |            ^
```

Notice that even `print 1 + 2;` did not run. mylang found the mistake first and refused to start.

### `error_runtime.rene`: a mistake while running

Output:

```
before
[line 3] Runtime error: Undefined variable 'missing'.
    3 | print missing;
```

Here the first line ran and printed `before`, then mylang stopped at the unknown variable.

### The other samples

| File | What it shows |
|---|---|
| `logic.rene` | `and`, `or`, `!`, and which values count as true or false |
| `scopes.rene` | Variables inside `{ }` blocks, and the same name used at different levels |
| `closures.rene` | Functions that remember their own private variables |

---

## 8. How mylang works inside

When you run a mylang program, your code passes through a few stages, like a product on an assembly line. Each stage has one job and hands its result to the next.

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
      ├───────────────────────────┐
      ▼                           ▼
 ┌─────────────┐          ┌──────────────┐
 │ Interpreter │          │   Compiler   │  turns the tree into
 │ (default)   │          │              │  simple step-by-step
 │ walks the   │          └──────────────┘  instructions (bytecode)
 │ tree and    │                  │
 │ does the    │                  ▼
 │ work        │          ┌──────────────┐
 └─────────────┘          │ Virtual      │  follows the steps
                          │ machine (VM) │  one by one (--vm)
                          └──────────────┘
```

### Stage 1: The lexer (splitting into words)

Before you can understand a sentence, you have to see the separate words. The lexer reads your code one character at a time and groups the characters into **tokens**, the "words" of the language.

`print 12 + x;` becomes five tokens:

| Token | Kind |
|---|---|
| `print` | keyword |
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

The multiplication sits lower in the tree, so it is done first. This is how mylang gets the order of operations right. The parser has one rule for each level of precedence: the rule for `+` and `-` asks the rule for `*` and `/` to read each side first, so `*` and `/` always end up deeper in the tree.

### Stage 3, option A: The interpreter (the default)

The interpreter walks through the tree and does the work. To work out `+`, it first works out its left side (`2`) and its right side (`3 * 4` = `12`), then adds them to get `14`. This is the easiest method to understand.

### Stage 3, option B: The compiler and the virtual machine (`--vm`)

This method has two steps, and it is how real languages such as Python and Java work.

First, the **compiler** turns the tree into **bytecode**: a list of very simple instructions. You can see it with `--dump`. For `print 1 + 2 * 3;` it makes:

```
Constant  1     put 1 on the pile
Constant  2     put 2 on the pile
Constant  3     put 3 on the pile
Multiply        take the top two (2 and 3), multiply them, put 6 back
Add             take the top two (1 and 6), add them, put 7 back
Print           take the top value (7) and print it
```

Then the **virtual machine (VM)** follows the instructions one by one. It keeps its numbers on a **stack**, which works like a pile of plates: you can only add to the top or take from the top.

```
Constant 1  →  [1]
Constant 2  →  [1, 2]
Constant 3  →  [1, 2, 3]
Multiply    →  [1, 6]
Add         →  [7]
Print       →  prints 7
```

**Why have both?** The interpreter is simpler to read. The VM is faster: it ran about 30% faster in our test. That is because the compiler does some work ahead of time, such as working out where each variable lives, so the VM does not have to look up names while running. Both methods always give the same output, and our tests check this.

### How errors travel back

C does not have a built-in way to "throw" an error from deep inside the program back to the top. We used two standard C tools, `setjmp` and `longjmp`. Before a stage starts, `setjmp` marks a safe return point. When something goes wrong, `longjmp` jumps straight back to that point, however deep inside the program the mistake happened. There, mylang prints the message and stops cleanly, or carries on with the next line in interactive mode.

### How memory is handled

- The tokens, the tree and the bytecode are stored in one big memory area called an **arena**. When the program ends, the whole arena is freed in one go. This is simple and fast, and nothing gets lost.
- Values made while running, such as text and functions, are kept on one list and freed together when the program ends.

---

## 9. The source code, file by file

All source code is in the `src` folder. Each `.h` file (header) describes what a part offers; the `.c` file with the same name contains the actual code. Every file starts with a comment explaining its job.

| File | Its job, in simple words |
|---|---|
| `main.c` | The starting point. Reads the command, then runs a file or starts interactive mode. Holds the help text and the `mylang>` prompt. |
| `token.h` | The list of every kind of token (word) the language has. |
| `lexer.h`, `lexer.c` | Stage 1: splits the code text into tokens. |
| `ast.h` | Describes the pieces of the syntax tree. ("AST" means abstract syntax tree.) |
| `parser.h`, `parser.c` | Stage 2: checks the grammar and builds the tree. The full grammar is written at the top of `parser.h`. |
| `value.h`, `value.c` | Describes every kind of value (numbers, text, true/false, nil, functions) and how to compare and print them. Also has the lookup table used to find variables by name. |
| `interpreter.h`, `interpreter.c` | Stage 3A: walks the tree and runs the program. |
| `vm/chunk.h`, `vm/chunk.c` | The list of bytecode instructions, and the code that prints them for `--dump`. |
| `vm/compiler.h`, `vm/compiler.c` | Stage 3B, part 1: turns the tree into bytecode. |
| `vm/vm.h`, `vm/vm.c` | Stage 3B, part 2: the virtual machine that runs the bytecode. |
| `errors.h`, `errors.c` | Catches errors and prints them with the line number and the `^` marker. |
| `arena.h`, `arena.c` | The memory area for tokens, the tree and bytecode, freed all at once at the end. |

Other files:

| File | Its job |
|---|---|
| `CMakeLists.txt` | Instructions for building the program with CMake, on Windows, Mac or Linux. |
| `tests/run_tests.sh` | Runs every sample program on both methods and checks the output. |
| `docs/code-guide.md` | A longer, more technical walk-through of every file. |
| `docs/backend.md` | A deeper explanation of the compiler and virtual machine. |
| `docs/evaluation.md` | The full evaluation against the language criteria. |

---

## 10. Every data structure and its variables

C does not have classes. Instead it has **structures** (`struct`): a group of variables kept together under one name, which is what a class's variables are. This section describes every structure in mylang and every variable inside it. The same descriptions are written as comments next to each variable in the source code.

A few words that come up often:

- **slot**: one space in a list that can hold one item
- **allocated slots** (`cap`): how many spaces have been made ready; when they fill up, the list is made bigger
- **pointer**: a variable that holds the location of something else, so it can be found

### Tokens and the syntax tree

**`Token`** (`token.h`): one "word" of the code, made by the lexer.

| Variable | Type | What it holds |
|---|---|---|
| `type` | `TokenType` | What kind of token it is: a number, a name, `+`, `while`, and so on. |
| `lexeme` | `const char*` | The token's actual text from the code. For text in quotes, the text without the quotes. |
| `length` | `size_t` | How many characters `lexeme` has. |
| `number` | `double` | The value, if the token is a number. |
| `line` | `int` | The line the token is on, used in error messages. |
| `offset` | `size_t` | How far into the code the token starts, so an error's `^` can point at it. |

**`Lexer`** (`lexer.c`): what the lexer keeps track of while splitting the code into tokens.

| Variable | Type | What it holds |
|---|---|---|
| `src` | `const char*` | The code being read. |
| `len` | `size_t` | How long the code is. |
| `start` | `size_t` | Where the token being read now begins. |
| `cur` | `size_t` | The position of the next character to read. |
| `line` | `int` | The line number it is on now. |
| `tokens` | `Token*` | The tokens made so far. |
| `count` | `int` | How many tokens have been made. |
| `cap` | `int` | How many allocated slots `tokens` has. |

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
| `boolean` | `bool` | The value, for `true` or `false`. |
| `text` | `const char*` | The text, for text in quotes; or the variable's name, for a variable. |
| `textLength` | `size_t` | How long `text` is. |
| `op` | `TokenType` | Which operator, for an operation such as `+`, `<` or `and`. |
| `left` | `Expr*` | The left side of an operation, the value being stored, or the function being called. |
| `right` | `Expr*` | The right side of an operation. |
| `args` | `Expr**` | The inputs given to a function call. |
| `argCount` | `int` | How many inputs were given. |

**`Stmt`** (`ast.h`): one statement (instruction) in the syntax tree, such as `print`, `let`, `if`, `while` or a function definition.

| Variable | Type | What it holds |
|---|---|---|
| `kind` | `StmtKind` | What kind of statement this is. It decides which of the variables below are used. |
| `line` | `int` | The line it came from, for error messages. |
| `name` | `const char*` | The name being created, for `let` and `fn`. |
| `expr` | `Expr*` | The value used by the statement: what to print, the starting value for `let`, the condition for `if` and `while`, or what to `return`. Can be empty. |
| `body` | `Stmt**` | The statements inside a `{ }` block or a function. |
| `bodyCount` | `int` | How many statements are in `body`. |
| `thenBranch` | `Stmt*` | What to do when an `if` is true, or the code a `while` repeats. |
| `elseBranch` | `Stmt*` | What to do when an `if` is false. Empty if there is no `else`. |
| `params` | `const char**` | The names of a function's inputs (parameters). |
| `paramCount` | `int` | How many inputs the function takes. |

**`Program`** (`ast.h`): a whole parsed program.

| Variable | Type | What it holds |
|---|---|---|
| `stmts` | `Stmt**` | All the top-level statements, in order. |
| `count` | `int` | How many statements there are. |

### Values

**`Value`** (`value.h`): any single value in mylang: a number, true/false, nil, text or a function.

| Variable | Type | What it holds |
|---|---|---|
| `type` | `ValueType` | Which kind of value it is: nil, number, true/false, or an object (text or a function). |
| `as.number` | `double` | The number, if it is a number. |
| `as.boolean` | `bool` | `true` or `false`, if it is a true/false value. |
| `as.obj` | `Obj*` | Where to find the object, if it is text or a function. |

(`as` holds only one of its three variables at a time, the one that matches `type`. This is called a `union` in C, and it saves memory.)

**`Obj`** (`value.h`): the shared header at the start of every object (text, scope, function, closure or upvalue).

| Variable | Type | What it holds |
|---|---|---|
| `type` | `ObjType` | Which kind of object it is. |
| `next` | `Obj*` | The next object on the list of all objects, which is used to free them all at the end. |

**`ObjString`** (`value.h`): a piece of text.

| Variable | Type | What it holds |
|---|---|---|
| `obj` | `Obj` | The shared object header. |
| `length` | `size_t` | How many characters the text has. |
| `chars` | `char[]` | The characters themselves. |

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
| `cap` | `int` | How many slots there are in total. |

### Values used by the interpreter

**`ObjEnv`** (`value.h`): a **scope**, meaning one level of variables, such as the variables inside one `{ }` block or one function call.

| Variable | Type | What it holds |
|---|---|---|
| `obj` | `Obj` | The shared object header. |
| `parent` | `ObjEnv*` | The scope around this one. If a name is not found here, mylang looks there next. Empty for the outermost (global) scope. |
| `vars` | `Table` | The variables made in this scope, by name. |
| `captured` | `bool` | True once a function needs to remember this scope. Then it is kept until the program ends. |

**`ObjFunction`** (`value.h`): a function, as the interpreter stores it.

| Variable | Type | What it holds |
|---|---|---|
| `obj` | `Obj` | The shared object header. |
| `decl` | `const Stmt*` | The function's definition in the tree: its name, inputs and body. |
| `closure` | `ObjEnv*` | The scope where the function was made. This is how a function remembers the variables around it. |

**`Interpreter`** (`interpreter.h`): everything the interpreter keeps track of while running.

| Variable | Type | What it holds |
|---|---|---|
| `globals` | `ObjEnv*` | The outermost scope, which lasts for the whole session. |
| `env` | `ObjEnv*` | The scope being used right now. |
| `live` | `ObjEnv**` | The scopes of every block and function call still running, so they can be cleaned up if an error happens. |
| `liveCount` | `int` | How many scopes are in `live`. |
| `liveCap` | `int` | How many allocated slots `live` has. |
| `callDepth` | `int` | How many function calls are running inside each other, to catch a function that calls itself forever. |
| `returnValue` | `Value` | The value a `return` statement is sending back. |

### Bytecode and the virtual machine

**`Instr`** (`vm/chunk.h`): one bytecode instruction.

| Variable | Type | What it holds |
|---|---|---|
| `op` | `Op` | What to do, such as `Add` or `Print`. |
| `operand` | `int` | Extra information for the instruction, such as which value to load or where to jump. |
| `line` | `int` | The line of code it came from, for error messages. |

**`UpvalueRef`** (`vm/chunk.h`): tells a function where to find one outside variable it needs to remember.

| Variable | Type | What it holds |
|---|---|---|
| `isLocal` | `bool` | True if the variable belongs to the function right around this one; false if it is further out. |
| `index` | `int` | Which variable it is, by position. |

**`Proto`** (`vm/chunk.h`): one compiled function: its instructions and everything they need.

| Variable | Type | What it holds |
|---|---|---|
| `name` | `const char*` | The function's name, or `<script>` for the main program. |
| `arity` | `int` | How many inputs the function takes. |
| `code` | `Instr*` | The function's instructions. |
| `codeCount` | `int` | How many instructions there are. |
| `codeCap` | `int` | How many allocated slots `code` has. |
| `constants` | `Value*` | The fixed values the instructions use: numbers, text and variable names. |
| `constCount` | `int` | How many constants there are. |
| `constCap` | `int` | How many allocated slots `constants` has. |
| `functions` | `Proto**` | The functions defined inside this one. |
| `functionCount` | `int` | How many functions are defined inside. |
| `functionCap` | `int` | How many allocated slots `functions` has. |
| `upvalues` | `UpvalueRef*` | Where to find each outside variable this function remembers. |
| `upvalueCount` | `int` | How many outside variables it remembers. |
| `upvalueCap` | `int` | How many allocated slots `upvalues` has. |

**`ObjUpvalue`** (`value.h`): one outside variable that a function remembers (called an **upvalue**).

| Variable | Type | What it holds |
|---|---|---|
| `obj` | `Obj` | The shared object header. |
| `slot` | `size_t` | While the variable is still on the VM's stack, its position there. |
| `open` | `bool` | True while the variable still lives on the stack. |
| `closed` | `Value` | After the variable's block ends, its value is moved and kept here. |

**`ObjClosure`** (`value.h`): a function as the VM runs it, together with the outside variables it remembers.

| Variable | Type | What it holds |
|---|---|---|
| `obj` | `Obj` | The shared object header. |
| `proto` | `Proto*` | The compiled function. |
| `upvalues` | `ObjUpvalue**` | The outside variables this function remembers. |
| `upvalueCount` | `int` | How many there are. |

**`Frame`** (`vm/vm.h`): one function call that is running in the VM.

| Variable | Type | What it holds |
|---|---|---|
| `closure` | `ObjClosure*` | The function being run. |
| `ip` | `size_t` | The position of the next instruction to run. ("ip" means instruction pointer.) |
| `base` | `size_t` | Where this call's values start on the stack. The function's inputs and variables come right after it. |

**`VM`** (`vm/vm.h`): everything the virtual machine keeps track of while running.

| Variable | Type | What it holds |
|---|---|---|
| `dumpBytecode` | `bool` | True when `--dump` was given, so the instructions are shown before running. |
| `stack` | `Value*` | The stack: the "pile of plates" where values are kept while working. |
| `stackCount` | `size_t` | How many values are on the stack. |
| `stackCap` | `size_t` | How many allocated slots the stack has. |
| `frames` | `Frame*` | One entry for each function call that is running. The last one is the call running right now. |
| `frameCount` | `size_t` | How many calls are running. |
| `frameCap` | `size_t` | How many allocated slots `frames` has. |
| `openUpvalues` | `ObjUpvalue**` | Remembered outside variables that still live on the stack. |
| `openCount` | `size_t` | How many there are. |
| `openCap` | `size_t` | How many allocated slots `openUpvalues` has. |
| `globals` | `Table` | All global variables and functions, by name. |

### The compiler

**`Local`** (`vm/compiler.c`): one variable inside a block or function, as the compiler tracks it.

| Variable | Type | What it holds |
|---|---|---|
| `name` | `const char*` | The variable's name. |
| `depth` | `int` | How many blocks deep it was made, so it can be removed when its block ends. |
| `captured` | `bool` | True if a function inside needs to remember it, so it must be saved before its block ends. |

**`FunctionState`** (`vm/compiler.c`): what the compiler keeps track of for each function it is compiling.

| Variable | Type | What it holds |
|---|---|---|
| `enclosing` | `FunctionState*` | The function this one is written inside. Empty for the main program. |
| `proto` | `Proto*` | The compiled function being built. |
| `locals` | `Local*` | The variables that can be seen right now, in the order they sit on the stack. |
| `localCount` | `int` | How many variables are in `locals`. |
| `localCap` | `int` | How many allocated slots `locals` has. |
| `scopeDepth` | `int` | How many blocks deep the compiler is right now. `0` means the top level of the function. |

### Errors, memory and the program itself

**`ErrorState`** (`errors.h`): the details of the most recent error.

| Variable | Type | What it holds |
|---|---|---|
| `env` | `jmp_buf` | The safe return point that the error jumps back to (see Section 8). |
| `kind` | `ErrorKind` | What kind of error it is: syntax, compile or runtime. |
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
| `cap` | `int` | How many allocated slots `items` has. |

**`Runner`** (`main.c`): which method is running the program, and the state of each method.

| Variable | Type | What it holds |
|---|---|---|
| `useVm` | `bool` | True to run on the virtual machine (`--vm`); false for the default interpreter. |
| `interpreter` | `Interpreter` | The interpreter and its variables. Variables are kept between lines in interactive mode. |
| `vm` | `VM` | The virtual machine and its variables. Variables are kept between lines in interactive mode. |

---

## 11. How mylang meets the language evaluation criteria

Programming languages are usually judged on four main things. Here is how mylang does on each, in short. The full version, with more examples, is in `docs/evaluation.md`.

### Readability: is the code easy to read?

- **It's small.** There are only 12 keywords and one kind of number, so there is little to learn and nothing confusing.
- **One way to do each thing.** For example, to add 1 to a variable there is only `x = x + 1`, not three different ways.
- **The words say what they do.** `let` makes a variable, `print` prints, `fn` makes a function.
- **Real true and false values.** Conditions use `true` and `false`, not `1` and `0`.
- **Blocks are always in `{ }`,** so it is always clear which code belongs to an `if` or a loop.

### Writability: is the code easy to write?

- **The pieces fit together freely.** Any value can be printed, stored, compared, passed to a function, or returned from one.
- **Functions** let you name a piece of work once and reuse it, including recursion.
- **Text joins easily:** `print "Total: " + total;` needs no extra conversion step.
- **Interactive mode** lets you try ideas one line at a time.

### Reliability: does the code do what you expect?

- **Types are checked.** You cannot subtract a number from text, and mylang says so instead of giving a strange answer.
- **Variables must be created first** with `let`, which catches spelling mistakes.
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

We tested mylang in three ways.

**1. Automatic tests.** The script `tests/run_tests.sh` runs all 8 sample programs, first with the interpreter and then with the virtual machine. It compares each result with the correct output saved in the matching `.expected` file. Result:

```
16 passed, 0 failed
```

This proves that both methods give the correct output, and the same output as each other. To run it (on Windows, use Git Bash):

```
tests/run_tests.sh build/Release/mylang.exe
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

The guidelines allow libraries, as long as we understand them. **We did not use any outside libraries.** Everything (the lexer, parser, interpreter, compiler, virtual machine, memory arena and lookup table) was written by us. We only used parts of the C standard library, which comes with every C compiler:

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

No language is perfect, and here are mylang's limits:

- **No separate integer type.** Numbers are exact up to 9,007,199,254,740,992. Past that, whole numbers may be rounded.
- **Type mistakes are found only while running.** A mistake in a part of the code that never runs is not reported.
- **No `for` loop or `else if`.** A `while` loop, and an `if` inside an `else`, do the same jobs with a little more typing.
- **No lists.** A variable holds one value at a time.
- **One mistake at a time.** mylang stops at the first format mistake, so fixing several takes several runs.
- **Memory is freed only at the end.** A program that runs for a very long time and makes a lot of text keeps using more memory.

What we would add next:

1. `for` loops, `else if` and `break`
2. Built-in functions such as `len(text)` and `clock()`
3. Lists, such as `[1, 2, 3]`
4. A separate whole-number type with overflow checks
5. A garbage collector that frees unused memory while the program runs
