# Keyword theme proposal: "Team Communication & Signal"

> **Status: applied.** The chosen set (see [Final choice](#final-choice)) is now the language's keywords in the code, examples and docs.
> To change a word again, follow [CUSTOMIZING.md](CUSTOMIZING.md#1-language-keywords-broadcast-assign-protocol-when-repeat-).

broadcast_language has 12 keywords. This page suggests a replacement for each one, themed around how a team talks over radio: signals, broadcasts, reports, "copy" and "negative". Each row has a **recommended** word and some alternatives.

## Recommended set at a glance

| Current | Recommended | Meaning in the theme | Alternatives |
|---|---|---|---|
| `let` | **`signal`** | Send out a named signal (create a variable) | `tag`, `callsign`, `assign` |
| `fn` | **`protocol`** | A procedure the team follows (define a function) | `routine`, `drill`, `play` |
| `return` | **`report`** | Report the result back to whoever called | `reply`, `respond` |
| `print` | **`broadcast`** | Announce a value to everyone (output) | `announce`, `radio`, `say` |
| `if` | **`when`** | Act when a condition is met | `check`, `oncue` |
| `else` | **`otherwise`** | The fallback plan | `fallback`, `instead` |
| `while` | **`repeat`** | Keep repeating while the condition holds | `relay`, `standby`, `hold` |
| `true` | **`affirm`** | "Affirmative": yes | `roger`, `yes` |
| `false` | **`negative`** | "Negative": no | `deny`, `no` |
| `nil` | **`silence`** | No signal at all: no value | `static`, `void`, `none` |
| `and` | **`and`** (keep) | Both signals must be true | `also`, `with` |
| `or` | **`or`** (keep) | At least one signal is true | `either`, `alt` |

### Why keep `and` / `or`?
Themed words here hurt readability: `ready also armed` is less clear than `ready and armed`. The PL evaluation criteria (readability and writability, see `broadcast_language/docs/evaluation.md`) favor words whose meaning is obvious. If you want a fully themed set anyway, `also` / `either` are the best fit.

## Example: the same program, before and after

**Current broadcast_language**
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

**With the new keywords**
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

**Conditions, booleans and nil**
```
assign ready = affirm;
assign target = silence;

when (ready and target == silence) {
  broadcast "Standing by, no target.";
} otherwise {
  broadcast "Engaging.";
}
```

## Things to consider before choosing

1. **Keywords become reserved.** Once a word is a keyword it can no longer be used as a variable or function name. I checked the current `examples/*.bc` code: only **`copy`** is already used as a variable name, so it is *not* recommended for `true`. Every recommended word above is free.
2. **Length vs. typing.** `broadcast` and `protocol` are long. They read well, but they cost more typing than `print` and `fn` (writability). Shorter options from the alternatives column: `say`, `drill`, `tag`.
3. **No spaces or symbols.** A keyword must be one word of letters, digits or `_` (for example `on_cue`, not `on cue`).
4. **Case matters.** `Signal` and `signal` would be different words. Lowercase is recommended to match the rest of the language.
5. **Avoid confusing pairs.** For example, don't pick `report` for `return` *and* `reply` for `print`, because they sound like the same action.
6. **Error messages and docs change too.** Messages such as `Expect variable name after 'let'.` need the new word, and every example and doc page that uses the old keywords must be updated.

## What applying it would touch (for later)

When you decide, these are the places that change (details in [CUSTOMIZING.md](CUSTOMIZING.md)):

| Place | Change |
|---|---|
| `broadcast_language/src/lexer.c`, keyword table in `scanIdentifier()` | Swap the quoted words (the `TOK_...` names stay the same) |
| `broadcast_language/src/parser.c` | Error messages that quote a keyword |
| `broadcast_language/src/main.c` | `REPL_HELP` examples |
| `broadcast_language/examples/*.bc` (+ `.expected` if output changes) | Rewrite the programs with the new words |
| `broadcast_language/README.md`, `broadcast_language/DOCUMENTATION.md`, `broadcast_language/docs/*.md` | Keyword tables and code samples |

Then rebuild and run `bash tests/run_tests.sh build/broadcast_language`. All 8 tests should still pass.

## Final choice

The recommended set, except `let`, which became `assign` (one of its alternatives):

| Old | Now |
|---|---|
| `let` | **`assign`** |
| `fn` | **`protocol`** |
| `return` | **`report`** |
| `print` | **`broadcast`** |
| `if` | **`when`** |
| `else` | **`otherwise`** |
| `while` | **`repeat`** |
| `true` | **`affirm`** |
| `false` | **`negative`** |
| `nil` | **`silence`** |
| `and` | `and` (kept) |
| `or` | `or` (kept) |

Printed values follow the keywords too: `broadcast 1 < 2;` shows `affirm`, `broadcast silence;` shows `silence`, and a function shows as `<protocol name>`.
