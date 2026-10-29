# Data structures and their variables

This is the reference for every data structure in broadcast_language and every variable inside it. It belongs with the main project documentation, [DOCUMENTATION.md](../DOCUMENTATION.md); see its [Section 7](../DOCUMENTATION.md#7-how-broadcast_language-works-inside) for how the stages that use these structures fit together.

C does not have classes. Instead it has **structures** (`struct`): a group of variables kept together under one name, which is what a class's variables are. This page describes every structure in broadcast_language and every variable inside it. The same descriptions are written as comments next to each variable in the source code.

A few words that come up often:

- **slot**: one space in a list that can hold one item
- **allocated slots** (`capacity`): how many spaces have been made ready; when they fill up, the list is made bigger
- **pointer**: a variable that holds the location of something else, so it can be found

## Tokens and the syntax tree

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
| `thenBranch` | `Stmt*` | What to do when a `when` is true, or the code a `repeat` repeats. |
| `elseBranch` | `Stmt*` | What to do when a `when` is false. Empty if there is no `otherwise`. |
| `parameters` | `const char**` | The names of a function's inputs (parameters). |
| `parameterCount` | `int` | How many inputs the function takes. |

**`Program`** (`ast.h`): a whole parsed program.

| Variable | Type | What it holds |
|---|---|---|
| `statements` | `Stmt**` | All the top-level statements, in order. |
| `count` | `int` | How many statements there are. |

## Values

**`Value`** (`value.h`): any single value in broadcast_language: a number, `affirm`/`negative`, `silence`, text or a function.

| Variable | Type | What it holds |
|---|---|---|
| `type` | `ValueType` | Which kind of value it is: `silence`, number, `affirm`/`negative`, or an object (text or a function). |
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

## Values used by the interpreter

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

## Errors, memory and the program itself

**`ErrorState`** (`errors.h`): the details of the most recent error.

| Variable | Type | What it holds |
|---|---|---|
| `jumpBuffer` | `jmp_buf` | The safe return point that the error jumps back to (see [How errors travel back](../DOCUMENTATION.md#how-errors-travel-back)). |
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
