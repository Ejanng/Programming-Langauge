# Proposal: "Love" Keyword Theme

Status: **proposal only.** No changes have been made to the language yet.

This proposal swaps broadcast_language's current keywords for words on a "Love" theme. Each new word was chosen so that it still hints at what the keyword does.

## Current keywords

The language has 12 keywords, defined in `src/lexer.c`:

`assign`, `broadcast`, `when`, `otherwise`, `repeat`, `protocol`, `report`, `and`, `or`, `affirm`, `negative`, `silence`

## Recommended mapping

| Current | What it does | Recommended | Example | Other ideas |
|---|---|---|---|---|
| `assign` | declares a variable | **`cherish`** | `cherish age = 18;` | `adore`, `keep` |
| `broadcast` | prints output | **`confess`** | `confess "Hello!";` | `whisper`, `serenade` |
| `when` | if | **`wish`** | `wish (x > 3) { … }` | `crush`, `dream` |
| `otherwise` | else | **`heartbreak`** | `} heartbreak { … }` | `instead` |
| `repeat` | while loop | **`yearn`** | `yearn (i < 10) { … }` | `longing`, `forever` |
| `protocol` | defines a function | **`vow`** | `vow greet(name) { … }` | `ritual`, `promise` |
| `report` | returns a value | **`gift`** | `gift a + b;` | `give` |
| `and` | logical and | keep **`and`** | `x > 1 and y < 5` | `together` |
| `or` | logical or | keep **`or`** | `x > 1 or y < 5` | `either` |
| `affirm` | true | **`love`** | `cherish happy = love;` | `requited` |
| `negative` | false | **`loveless`** | `cherish sad = loveless;` | `unrequited` |
| `silence` | null / no value | **`single`** | `cherish partner = single;` | `ghosted`, `alone` |

## Why these words

- **`love` / `loveless`:** the theme word itself becomes **true**, so its opposite is the natural word for **false**.
- **`cherish`:** you hold on to a value, the way you cherish something.
- **`confess`:** you say something out loud, the way a program prints output.
- **`wish` / `heartbreak`:** you make a wish (if). When it doesn't come true, you get heartbreak (else).
- **`yearn`:** you keep going for as long as you still want something, the way a while loop repeats as long as its condition holds.
- **`vow`:** a promise you make once and can keep calling on, just as a function is defined once and called many times.
- **`gift`:** something a function hands back (return).
- **`single`:** no partner means no value (null). `ghosted` is a funnier alternative.
- **`and` / `or` stay the same:** themed versions make conditions harder to read (`x > 1 together y < 5` is awkward), and readability matters more here.

## Sample program

```
// Current keywords
protocol double(n) {
    report n * 2;
}
assign count = 0;
repeat (count < 3) {
    when (count == 1) {
        broadcast double(count);
    } otherwise {
        broadcast count;
    }
    count = count + 1;
}
```

```
// Proposed "Love" keywords
vow double(n) {
    gift n * 2;
}
cherish count = 0;
yearn (count < 3) {
    wish (count == 1) {
        confess double(count);
    } heartbreak {
        confess count;
    }
    count = count + 1;
}
```

## Things to consider before applying

- **New reserved words:** every new keyword becomes reserved. For example, `love` and `single` could no longer be used as variable names.
- **Files that would change:**
  - `src/lexer.c`: the keyword table
  - `ide/broadcast_ide.py`: keyword colours, the Reference tab and the welcome program
  - every `.bc` file in `examples/` and `tests/`
  - `DOCUMENTATION.md` and `README.md`, including the section that explains the keyword theme
- **Language name:** the language is still called *broadcast_language*, and `broadcast` is its signature keyword. With a love theme, the language could be renamed too (for example *love_language* with a `.love` file extension) so the name matches the keywords.
