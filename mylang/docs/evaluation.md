# mylang and the language evaluation criteria

This document checks mylang against the standard criteria for evaluating programming languages: **readability**, **writability**, **reliability** and **cost**, plus portability and generality. Each point is backed by real mylang code and the output it produces, so every claim can be checked by running the interpreter. Weak spots are listed too, because a fair evaluation covers both sides.

Every example below runs as written. Results are the same on both backends: the tree-walking interpreter (`mylang file.rene`) and the bytecode VM (`mylang --vm file.rene`).

## Summary

| Criterion | Characteristic | How mylang meets it |
|---|---|---|
| Readability | Simplicity | 12 keywords, 1 number type, one way to do each thing |
| | Orthogonality | Every expression works in every expression position; functions are ordinary values |
| | Data types | Real `true`/`false` and `nil` instead of 1/0 or magic values |
| | Syntax design | Keywords say what they do (`let`, `fn`, `print`); `{ }` closes every block |
| Writability | Simplicity and orthogonality | A few constructs combine freely (see the `power` function in `examples/arithmetic.rene`) |
| | Support for abstraction | Functions, recursion and closures |
| | Expressivity | Operators with conventional precedence, `+` that joins text, REPL for quick experiments |
| Reliability | Type checking | Every operator checks its operand types and stops with a clear message |
| | Exception handling | Every error is caught and reported with its line number; the REPL keeps running |
| | Restricted aliasing | No pointers or references; variables hold values |
| | Readability and writability | The points above make correct programs easier to write and check |
| Cost | Training, writing, compiling, running, maintenance | Small language, fast startup, two backends, documented source |

## 1. Readability

### Simplicity

The whole language fits on one screen. It has 12 keywords (`let fn return if else while print and or true false nil`), five arithmetic operators (`+ - * / %`), six comparisons and three logical operators. There is one number type, so `7`, `-3` and `2.5` are all just numbers, and there is no `int`/`float`/`long` choice to make.

The language avoids **feature multiplicity** (several ways to do the same thing). There is no `x++` next to `x += 1` next to `x = x + 1`; only the last exists. It also avoids **operator overloading surprises**: `+` has exactly two meanings, adding numbers and joining text, and both are listed in the README.

### Orthogonality

A small set of constructs combine in every way that makes sense:

- Any expression can go anywhere an expression is allowed: `print`, `let`, conditions, arguments and `return`.
- Functions are values. They can be stored in variables, passed as arguments and returned from other functions (`examples/closures.rene`).
- Blocks `{ }` work the same after `if`, `else`, `while` and `fn`, and alone to make a new scope.

```
let width = 12;
let height = 7;
print "perimeter = " + 2 * (width + height);   // perimeter = 38
```

One trade-off is that comparisons such as `<` work only on numbers. This keeps their meaning simple, but it means strings cannot be ordered.

### Data types

mylang has a real boolean type, so a condition reads as what it means:

```
let done = false;
if (!done) { print "still working"; }
```

Only `false` and `nil` count as false, so `0` and `""` are true. That avoids the classic mistake of a valid zero being treated as "no value".

### Syntax design

- Keywords describe their purpose: `let` introduces a variable, `fn` a function, `print` prints.
- Every block uses `{ }`, so there is no dangling-`else` ambiguity about which `if` an `else` belongs to.
- Every statement ends with `;`, so line breaks never change meaning.
- Operator precedence follows school arithmetic: `2 + 3 * 4` is `14`, and `(2 + 3) * 4` is `20`. Operators of the same precedence group left to right: `100 - 10 - 1` is `89`.

## 2. Writability

### Simplicity and orthogonality

Because the constructs combine freely, short programs stay short. Raising a number to a power uses only `fn`, `let`, `while` and arithmetic:

```
fn power(base, exp) {
  let result = 1;
  while (exp > 0) { result = result * base; exp = exp - 1; }
  return result;
}
print "2^10 = " + power(2, 10);   // 2^10 = 1024
```

### Support for abstraction

- **Process abstraction:** functions with parameters and return values, including recursion (`examples/fib.rene`).
- **Closures:** a function remembers the variables around it, so state can be hidden inside it (`examples/closures.rene` builds a counter this way).

### Expressivity

- `+` joins a string with any value, so output needs no conversion calls: `print "area = " + area;`.
- `and` / `or` short-circuit, so `x != 0 and 10 / x > 1` never divides by zero.
- The REPL runs one line at a time, adds the final `;` for you and has a `help` command, so trying an idea takes seconds.

## 3. Reliability

### Type checking

mylang is dynamically typed, so types are checked while the program runs, at the operator that uses the value. A wrong type stops the program with a message that names the problem, instead of producing a meaningless result:

```
print "a" - 1;      // [line 1] Runtime error: Operands must be numbers.
print true + 1;     // [line 1] Runtime error: Operands of '+' must be two numbers or include a string.
print 5(3);         // [line 1] Runtime error: Can only call functions.
```

Function calls are checked too. A wrong number of arguments is an error, never silently ignored:

```
fn f(a) { return a; }
print f(1, 2);      // [line 2] Runtime error: Expected 1 arguments but got 2.
```

Variables must be declared with `let` before use, which catches typos in names:

```
let total = 5;
print totl;         // [line 2] Runtime error: Undefined variable 'totl'.
```

A variable declared inside a block is not visible after the block ends, which keeps names from leaking (`examples/scopes.rene`).

### Exception handling

The language does not have `try`/`catch`. Instead, the implementation catches every error, at every stage, and reports it in one consistent format with the source line. The first line of each message is shown below; under it the program prints the source line, and for syntax errors a `^` under the exact token:

```
[line 4] Syntax error: Expect expression. (found '*')
    4 | print (3 + * 4;
      |            ^
```


| Stage | Example input | Message |
|---|---|---|
| Lexer | `print 5 $ 2;` | `[line 1] Syntax error: Unexpected character '$'.` |
| Parser | `print (3 + * 4;` | `[line 1] Syntax error: Expect expression. (found '*')` |
| Parser | `print 1 + 2` | `[line 1] Syntax error: Expect ';' after value. (reached end of file)` |
| Runtime | `print 1 / 0;` | `[line 1] Runtime error: Division by zero.` |
| Runtime | `print 5 % 0;` | `[line 1] Runtime error: Division by zero.` |
| Runtime | endless recursion | `[line 1] Runtime error: Stack overflow: too many nested calls.` |

Badly formatted programs are rejected **before anything runs**: a syntax error anywhere in a file means no line of it executes (`examples/error_syntax.rene`). The process exit code tells scripts what happened: `0` success, `65` syntax error, `70` runtime error, `74` file not found, `64` bad command-line usage.

In the REPL, an error is printed and the session continues with every earlier variable still defined.

### Restricted aliasing

There are no pointers, references or arrays, so two names cannot secretly refer to the same storage and change each other. The one deliberate exception is a closure: a function that captures a variable shares it with the scope it came from, which is exactly what makes the counter in `examples/closures.rene` work.

### Correct arithmetic output

Numbers are stored as 64-bit floating point (IEEE 754 doubles). Every whole number up to 2^53 = 9,007,199,254,740,992 is stored exactly and printed with all its digits (`2147483647 + 1` prints `2147483648`). Division that does not come out even gives a fraction (`7 / 2` is `3.5`), as in Python 3 and JavaScript. Larger whole numbers and long fractions are rounded to 15 significant digits.

## 4. Cost

- **Training:** the language is small and its syntax resembles C, Java and JavaScript, so it can be learned in an afternoon. The REPL's `help` command lists every construct.
- **Writing programs:** see Writability above.
- **Compiling:** there is no separate compile step for the user. The interpreter starts and runs a file in milliseconds.
- **Executing:** two backends trade off differently. The tree-walking interpreter is the simplest to read; the bytecode VM resolves variable names before running, so variable access is a stack index instead of a name lookup.
- **Implementation:** the interpreter is about 3,000 lines of standard C11 with no external libraries, so it builds with any C compiler.
- **Reliability cost:** errors stop at the first problem with a line number, so a mistake does not run on and corrupt later results.
- **Maintenance:** every source file has a header comment, every struct field is documented, and [code-guide.md](code-guide.md) walks through each file.

## 5. Other criteria

- **Portability:** standard C11 only, with no compiler-specific features. It builds with GCC or Clang (Linux, macOS, MinGW) and with Microsoft Visual C++ (Windows), and `tests/run_tests.sh` checks that every example gives the same output on each.
- **Generality:** variables, conditionals, loops and recursive functions make the language Turing complete, so it can express any computation, not only arithmetic.
- **Well-definedness:** the grammar is written out in full at the top of `src/parser.h`, and both backends are tested against the same expected output, so the language means the same thing whichever backend runs it.

## Known limitations

These are honest gaps, with the criterion each one affects:

- **No separate integer type** (reliability): whole numbers above 2^53 lose precision. A future version could add 64-bit integers with overflow checks.
- **No compile-time type checking** (reliability): a type error in a branch that never runs is not reported.
- **No `for`, `break` or `else if` keyword** (writability): `else { if ... }` and `while` cover these cases, but take more typing.
- **No lists or user-defined types** (writability, generality): there are no data structures beyond single values.
- **Only the first error is reported** (cost of writing programs): fixing several syntax errors takes several runs.
