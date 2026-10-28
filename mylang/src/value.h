#pragma once
// Runtime values shared by both backends: nil, number, boolean, or a pointer to a heap object
// (string, function, closure, ...).
//
// Heap objects are never freed while the program runs: each one is linked into a global list and
// everything is released together by objects_free_all() at exit. This stands in for the reference
// counting the language used before, which leaked on cycles anyway. A real garbage collector
// (mark-and-sweep, rooted in the stack, globals and open upvalues) would walk this same list.
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Stmt;
struct Proto;

typedef struct Obj Obj;

typedef enum { VAL_NIL, VAL_NUMBER, VAL_BOOL, VAL_OBJ } ValueType;

typedef struct {
    ValueType type;  // which member of `as` holds the value
    union {
        double number;  // VAL_NUMBER: all numbers, whole or fractional, are doubles
        bool boolean;   // VAL_BOOL
        Obj* obj;       // VAL_OBJ: points at a heap object; obj->type says which kind
    } as;
} Value;

// Constructors: wrap a C value in a Value with the matching type tag.
static inline Value nil_val(void) { return (Value){VAL_NIL, {.number = 0}}; }
static inline Value number_val(double n) { return (Value){VAL_NUMBER, {.number = n}}; }
static inline Value bool_val(bool b) { return (Value){VAL_BOOL, {.boolean = b}}; }
static inline Value obj_val(void* o) { return (Value){VAL_OBJ, {.obj = (Obj*)o}}; }

typedef enum {
    OBJ_STRING,
    OBJ_ENV,       // tree-walker: a scope
    OBJ_FUNCTION,  // tree-walker: a user-defined function plus the scope it was defined in
    OBJ_CLOSURE,   // VM: compiled code plus the variables it captured
    OBJ_UPVALUE,   // VM: a variable captured by a closure
} ObjType;

struct Obj {
    ObjType type;  // which kind of object this is, so it can be cast to the right struct
    Obj* next;  // the global list of every object
};

// True if `v` is a heap object of type `t`. Check this before casting v.as.obj.
static inline bool is_obj_type(Value v, ObjType t) { return v.type == VAL_OBJ && v.as.obj->type == t; }

// ---- strings ----

typedef struct {
    Obj obj;        // common object header (first, so an ObjString* is also an Obj*)
    size_t length;  // number of characters, not counting the terminator
    char chars[];  // always '\0'-terminated as well
} ObjString;

// Makes a new string object holding a copy of `length` bytes of `chars`.
ObjString* string_copy(const char* chars, size_t length);
// Makes a new string object holding `a` followed by `b`. Neither input is changed.
ObjString* string_concat(const ObjString* a, const ObjString* b);
#define AS_STRING(v) ((ObjString*)(v).as.obj)

// ---- hash table (string keys) ----
// Keys are borrowed '\0'-terminated strings that must outlive the table (AST names, string constants).

typedef struct {
    const char* key;  // NULL marks an empty slot
    uint32_t hash;    // hash of key, stored so lookups and resizing need not rehash
    Value value;      // the value stored under key
} TableEntry;

typedef struct {
    TableEntry* entries;  // open-addressing slots (linear probing)
    int count;            // number of occupied slots
    int cap;              // number of slots in entries
} Table;

// Frees the slot array and leaves `t` empty. The keys are borrowed, so they are not freed.
void table_free(Table* t);
void table_set(Table* t, const char* key, Value value);  // adds the key or replaces its value
Value* table_get(const Table* t, const char* key);       // NULL if absent

// ---- tree-walker objects ----

// A scope. Scopes start out private to the call or block that made them and are freed with
// env_release() when it ends. Creating a function over a scope "captures" it (and its parents),
// which hands it over to the global object list so it outlives the call.
typedef struct ObjEnv {
    Obj obj;                // common object header
    struct ObjEnv* parent;  // the enclosing scope, searched when a name is not found here; NULL for globals
    Table vars;             // the variables declared in this scope, by name
    bool captured;          // true once a function closes over it; it then lives until exit
} ObjEnv;

typedef struct {
    Obj obj;                  // common object header
    const struct Stmt* decl;  // the STMT_FUNCTION node: name, parameters and body
    ObjEnv* closure;  // the scope the function was defined in, which is what makes closures work
} ObjFunction;

ObjEnv* env_new(ObjEnv* parent);  // private until captured; the caller must env_release() it
void env_release(ObjEnv* env);    // frees a scope that was not captured; does nothing otherwise
void env_capture(ObjEnv* env);    // keeps `env` and its parents alive until exit
// Declares `name` in this scope (replacing it if this scope already declared it), as `let` does.
void env_define(ObjEnv* env, const char* name, Value value);
bool env_assign(ObjEnv* env, const char* name, Value value);  // false if no scope declares `name`
Value* env_lookup(const ObjEnv* env, const char* name);       // NULL if undefined
// Makes a function object for `decl` that remembers the scope it was defined in. Captures that scope.
ObjFunction* function_new(const struct Stmt* decl, ObjEnv* closure);

// ---- VM objects ----

// A variable captured by a closure. While the variable's scope is still running it is "open"
// and lives on the stack at `slot`; when the scope ends it is "closed": the value moves into `closed`.
// Using a stack index instead of a pointer keeps it valid when the stack array grows.
typedef struct {
    Obj obj;       // common object header
    size_t slot;   // while open: index of the variable on the VM stack
    bool open;     // true while the variable still lives on the stack
    Value closed;  // once closed: the variable's value
} ObjUpvalue;

typedef struct {
    Obj obj;                // common object header
    struct Proto* proto;    // the compiled function this closure runs
    ObjUpvalue** upvalues;  // the captured variables, in the order proto->upvalues lists them
    int upvalueCount;       // number of entries in upvalues
} ObjClosure;

// Makes a closure for `proto` with room for its upvalues; the VM fills them in right after.
ObjClosure* closure_new(struct Proto* proto);
// Makes an open upvalue for the variable in VM stack slot `slot`.
ObjUpvalue* upvalue_new(size_t slot);

// ---- operations on values ----

bool is_truthy(Value v);  // only nil and false are falsy; 0 and "" are truthy
// The == operator: numbers and booleans by value, strings by content, functions by identity.
// Values of different types are never equal ("1" == 1 is false).
bool values_equal(Value a, Value b);
ObjString* value_to_string(Value v);  // how print and string + show a value
// Writes `d` the way print shows it (7, -2.5, 9007199254740992, nan, inf). Returns what snprintf returns.
int format_number(char* buf, size_t cap, double d);

// Frees every heap object at once; called at exit.
void objects_free_all(void);
