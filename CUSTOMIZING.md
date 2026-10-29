# Customizing mylang

How to change the language's keywords, symbols, name, prompts, messages and code names to your own preferences. All paths below are inside the `mylang/` folder.

Most changes are a single edit in one place. After any change, rebuild and run the tests (see [After any change](#after-any-change)).

## 1. Language keywords (`broadcast`, `assign`, `protocol`, `when`, `repeat`, ...)

The keywords currently use the "Team Communication & Signal" theme (see [KEYWORD_THEME.md](KEYWORD_THEME.md)). All 12 are in one table in `src/lexer.c`, inside `scanIdentifier()`:

```c
{"and", TOK_AND},             {"otherwise", TOK_ELSE}, {"negative", TOK_FALSE}, {"protocol", TOK_FN},
{"when", TOK_IF},             {"assign", TOK_LET},     {"silence", TOK_NIL},    {"or", TOK_OR},
{"broadcast", TOK_PRINT},     {"report", TOK_RETURN},  {"affirm", TOK_TRUE},    {"repeat", TOK_WHILE},
```

Change only the word in quotes. Example: `{"assign", TOK_LET}` becomes `{"var", TOK_LET}`, and now `var x = 5;` declares a variable.

Leave the `TOK_...` names alone. They keep the original keyword names (`TOK_LET` is the variable keyword, `TOK_PRINT` the output keyword, and so on). The parser and the interpreter only see those, so they keep working without changes.

Then update the places that mention the old word:

- **Error messages** in `src/parser.c` that quote a keyword, for example `"Expect variable name after 'assign'."` and `"Expect function name after 'protocol'."`, plus `"Can't report from top-level code."` in `src/interpreter.c`
- **Printed values**: if you rename `affirm`, `negative` or `silence`, also change what `broadcast` shows for them in `valueToString()` in `src/value.c` (and `<protocol name>` for functions, in the same function)
- **REPL help text**: `REPL_HELP` in `src/main.c`
- **Example programs**: `examples/*.rene`, and any `.expected` file whose output changed
- **Docs**: the keyword table in `DOCUMENTATION.md` ("Every keyword at a glance") and `docs/evaluation.md`

## 2. Operator symbols (`+ - * / %`, `==`, `<`, ...)

Each symbol is one `case` in the `switch` inside `scanToken()` in `src/lexer.c`:

```c
case '+': addToken(lexer, TOK_PLUS); break;
case '%': addToken(lexer, TOK_PERCENT); break;
```

Example: changing `case '%':` to `case '#':` makes `#` the remainder operator. Two-character operators (`==`, `!=`, `<=`, `>=`) use `match(lexer, '=')` to check the second character.

## 3. Language name, prompt and version

| What | Where |
|---|---|
| REPL prompt `mylang>` | `printf("mylang> ")` in `repl()`, `src/main.c` |
| Startup banner | the first `printf` in `repl()`, `src/main.c` |
| Version number `1.0` | `#define MYLANG_VERSION` in `src/main.c` |
| `--help` text | `USAGE` in `src/main.c` |
| Program (executable) name | `CMakeLists.txt` (`project(...)`, `add_executable(...)` and every `mylang` after it) and the default `./mylang` in `tests/run_tests.sh` |

## 4. File extension (`.rene`)

The program never checks the extension, so any file name works. Only these depend on it:

- `tests/run_tests.sh`: the `examples/*.rene` loop and the `${src%.rene}` line
- the example file names in `examples/`
- the usage text in `src/main.c` and the docs

## 5. Error messages

| Kind | Where |
|---|---|
| Syntax errors | `src/parser.c` (and `"Unexpected character"` / `"Unterminated string"` in `src/lexer.c`) |
| Runtime errors | `src/interpreter.c` |
| Error prefix (`[line N] Syntax error:`) | `reportError()` in `src/errors.c` |

After changing a message, update the matching `examples/error_*.expected` file.

## 6. C function and variable names

The code uses camelCase with full words (`environmentLookup`, `argumentCount`, `instructionPointer`). Type names are PascalCase (`ObjString`) and constants are UPPER_CASE (`TOK_NUMBER`).

To rename one, use **Rename Symbol** in VS Code: put the cursor on the name and press **F2**. It updates every use across all files. It needs the C/C++ extension. Avoid a plain find-and-replace: the same short word can be a function in one place and a struct field in another (for example `number`).

Comments and the docs (`DOCUMENTATION.md`, `docs/*.md`) mention many names, so search them for the old name afterwards.

## After any change

Rebuild and run the tests:

```sh
cmake -S . -B build && cmake --build build
bash tests/run_tests.sh build/mylang
```

On Windows with Visual Studio the program is `build\Release\mylang.exe`.

The tests run every `examples/*.rene` and compare the output with its `.expected` file. If a test fails, the `diff` it prints shows which example or expected output still uses the old word.
