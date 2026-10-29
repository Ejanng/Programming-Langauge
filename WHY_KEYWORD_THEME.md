# Why mylang uses the "Team Communication & Signal" keywords

mylang's keywords are not the usual `let`, `print`, `if` and `while`. They follow one theme: how a team talks over a radio or signal channel. This page explains why we made that choice, how it fits the programming language evaluation criteria, and what it costs. The full list of words is in [KEYWORD_THEME.md](KEYWORD_THEME.md).

## The keywords

| Keyword | Replaces | What it does | Why this word |
|---|---|---|---|
| `assign` | `let` | Creates a variable | Says exactly what happens: a value is assigned to a name. |
| `broadcast` | `print` | Shows a value on the screen | Output goes out to everyone watching, like a radio broadcast. |
| `protocol` | `fn` | Defines a function | A protocol is a fixed procedure a team follows whenever it is called for, which is what a function is. |
| `report` | `return` | Sends a result back from a function | The function reports its result back to whoever called it. |
| `when` | `if` | Makes a decision | "When the condition holds, do this" reads like an instruction to a team. |
| `otherwise` | `else` | The other branch of a decision | The fallback plan, in plain English. |
| `repeat` | `while` | Loops while a condition holds | Says directly that the code runs again and again. |
| `affirm` | `true` | The "yes" value | Radio speech: "affirmative". |
| `negative` | `false` | The "no" value | Radio speech: "negative". |
| `silence` | `nil` | "No value" | No signal on the channel means nothing was sent. |
| `and`, `or` | (unchanged) | Combine conditions | Kept, because they are already plain English (see below). |

## Why a theme at all

### 1. It gives the language an identity
Most small teaching languages copy C or JavaScript word for word. With a consistent theme, mylang is recognizably *our* language. Every program reads like a set of instructions passed around a team:

```
protocol checkIn(name) {
  report "Unit " + name + " checking in";
}

assign ready = affirm;
when (ready) {
  broadcast checkIn("Alpha");
} otherwise {
  broadcast "Standing by";
}
```

### 2. The words describe what happens
A beginner can guess most keywords without a manual. `broadcast` sends something out, `report` hands a result back, `repeat` does something again, `otherwise` is the other choice. Compare that with the traditional words: `fn` is an abbreviation and `nil` is jargon.

### 3. It shows that keywords are a design choice
A point of the course is that a language's syntax is designed, not given. Renaming the keywords while keeping the grammar and behavior the same shows the difference between **syntax** (the words you type) and **semantics** (what the program does). Inside the implementation the token types still have their original names (`TOK_LET`, `TOK_PRINT`, ...). Only the spelling the programmer sees changed, in one table in `src/lexer.c`.

### 4. It is consistent everywhere
The theme is not only in the keywords. Error messages use it (`Expect variable name after 'assign'.`, `Can't report from top-level code.`), and so do printed values: `broadcast 1 < 2;` prints `affirm`, `broadcast silence;` prints `silence`, and a function prints as `<protocol name>`. A program never shows a word that is not part of the language.

## How the choice fits the PL evaluation criteria

### Readability
- **Helps:** every keyword is a whole English word with one clear meaning. There are no abbreviations (`fn`) or jargon (`nil`).
- **Helps:** all the words belong to one theme, so they feel like a set rather than a random mix.
- **Kept on purpose:** `and` and `or` were not themed. Alternatives like `also` and `either` would read worse (`ready also armed` versus `ready and armed`). Readability decided against the theme here.

### Writability
- **Cost:** some keywords are longer: `broadcast` is 9 letters against `print`'s 5, and `protocol` is 8 against `fn`'s 2. Programs take a little more typing.
- **Not affected:** the grammar is unchanged, so everything that was easy to write before (expressions anywhere, functions as values, `+` joining text) is still easy.

### Reliability
- **Helps:** no two keywords sound like the same action. For example, we avoided using both `report` and `reply`, so a reader can't mix up "send a result back" with "show output".
- **Checked:** every keyword is reserved, so it can't be used as a variable name by accident. We checked the sample programs before choosing: `copy` was already used as a variable name, which is why it was not used for `true`.
- **Same behavior:** the change did not alter how any program runs. All 8 sample programs give the same results as before, apart from the new spelling of `affirm`/`negative`/`silence` in the output, and the test script checks this.

### Cost
- **Learning:** someone who knows C, Java or Python has to learn 10 new words. That is a small cost, and each word explains itself.
- **Changing it:** all keywords are in one table, so the theme costs almost nothing to maintain or change again (see [CUSTOMIZING.md](CUSTOMIZING.md)).

## Why `assign` instead of `signal`

The proposal recommended `signal` for creating a variable, because it fits the radio theme best. We chose `assign` (one of the listed alternatives) because it describes the action more precisely: the line `assign x = 10;` stores the value 10 under the name `x`. `signal x = 10;` sounds like sending a message, which is closer to what `broadcast` does. Readability won over a closer fit to the theme.

## Trade-offs we accept

- **Longer words** mean a bit more typing (writability).
- **Unfamiliar at first** to programmers who expect `if`/`while`/`print`, so the REPL's `help` command lists every keyword with an example.
- **More reserved words:** names like `report`, `repeat` and `silence` can no longer be used for variables.

We judged these small next to the gain in readability and identity.

## One-line summary for the presentation

> "We gave mylang a 'Team Communication & Signal' theme: you `assign` values, `broadcast` output, define a `protocol` and `report` results. The words describe what they do, which helps readability, and the theme is used consistently, even in error messages and printed values. Underneath, the grammar and the behavior are exactly the same, which shows the difference between syntax and semantics."
