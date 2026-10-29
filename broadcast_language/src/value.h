#pragma once
// Runtime values used by the interpreter: nil, number, boolean, or a pointer to a heap object
// (string, scope or function).
//
// Heap objects are never freed while the program runs: each one is linked into a global list and
// everything is released together by objectsFreeAll() at exit. This stands in for the reference
// counting the language used before, which leaked on cycles anyway. A real garbage collector
// (mark-and-sweep, rooted in the global scope and the scopes still running) would walk this same list.
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Stmt;

typedef struct Obj Obj;

typedef enum { VAL_NIL, VAL_NUMBER, VAL_BOOL, VAL_OBJ } ValueType;

typedef struct {
    ValueType type;     // which member of `as` holds the value
    union {
        double number;  // VAL_NUMBER: all numbers, whole or fractional, are doubles
        bool boolean;   // VAL_BOOL
        Obj* object;    // VAL_OBJ: points at a heap object; obj->type says which kind
    } as;
} Value;

// Constructors: wrap a C value in a Value with the matching type tag.
static inline Value nilValue(void) { return (Value){VAL_NIL, {.number = 0}}; }
static inline Value numberValue(double number) { return (Value){VAL_NUMBER, {.number = number}}; }
static inline Value booleanValue(bool boolean) { return (Value){VAL_BOOL, {.boolean = boolean}}; }
static inline Value objectValue(void* object) { return (Value){VAL_OBJ, {.object = (Obj*)object}}; }

typedef enum {
    OBJ_STRING,
    OBJ_ENV,       // a scope
    OBJ_FUNCTION,  // a user-defined function plus the scope it was defined in
} ObjType;

struct Obj {
    ObjType type;  // which kind of object this is, so it can be cast to the right struct
    Obj* next;     // the global list of every object
};

// True if `value` is a heap object of type `table`. Check this before casting v.as.object.
static inline bool isObjectType(Value value, ObjType type) { return value.type == VAL_OBJ && value.as.object->type == type; }

// ---- strings ----

typedef struct {
    Obj object;         // common object header (first, so an ObjString* is also an Obj*)
    size_t length;      // number of characters, not counting the terminator
    char characters[];  // always '\0'-terminated as well
} ObjString;

// Makes a new string object holding a copy of `length` bytes of `characters`.
ObjString* stringCopy(const char* characters, size_t length);
// Makes a new string object holding `left` followed by `right`. Neither input is changed.
ObjString* stringConcatenate(const ObjString* left, const ObjString* right);
#define AS_STRING(v) ((ObjString*)(v).as.object)

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
    int capacity;         // number of slots in entries
} Table;

// Frees the slot array and leaves `table` empty. The keys are borrowed, so they are not freed.
void tableFree(Table* table);
void tableSet(Table* table, const char* key, Value value);  // adds the key or replaces its value
Value* tableGet(const Table* table, const char* key);       // NULL if absent

// ---- scopes and functions ----

// A scope. Scopes start out private to the call or block that made them and are freed with
// environmentRelease() when it ends. Creating a function over a scope "captures" it (and its parents),
// which hands it over to the global object list so it outlives the call.
typedef struct ObjEnv {
    Obj object;             // common object header
    struct ObjEnv* parent;  // the enclosing scope, searched when a name is not found here; NULL for globals
    Table variables;        // the variables declared in this scope, by name
    bool captured;          // true once a function closes over it; it then lives until exit
} ObjEnv;

typedef struct {
    Obj object;                      // common object header
    const struct Stmt* declaration;  // the STMT_FUNCTION node: name, parameters and body
    ObjEnv* closure;                 // the scope the function was defined in, which is what makes closures work
} ObjFunction;

ObjEnv* environmentNew(ObjEnv* parent);        // private until captured; the caller must environmentRelease() it
void environmentRelease(ObjEnv* environment);  // frees a scope that was not captured; does nothing otherwise
void environmentCapture(ObjEnv* environment);  // keeps `environment` and its parents alive until exit
// Declares `name` in this scope (replacing it if this scope already declared it), as `assign` does.
void environmentDefine(ObjEnv* environment, const char* name, Value value);
bool environmentAssign(ObjEnv* environment, const char* name, Value value);  // false if no scope declares `name`
Value* environmentLookup(const ObjEnv* environment, const char* name);       // NULL if undefined
// Makes a function object for `declaration` that remembers the scope it was defined in. Captures that scope.
ObjFunction* functionNew(const struct Stmt* declaration, ObjEnv* closure);

// ---- operations on values ----

bool isTruthy(Value value);  // only silence and negative are falsy; 0 and "" are truthy
// The == operator: numbers and booleans by value, strings by content, functions by identity.
// Values of different types are never equal ("1" == 1 is false).
bool valuesEqual(Value left, Value right);
ObjString* valueToString(Value value);  // how broadcast and string + show a value
// Writes `number` the way print shows it (7, -2.5, 9007199254740992, nan, inf). Returns what snprintf returns.
int formatNumber(char* buffer, size_t capacity, double number);

// Frees every heap object at once; called at exit.
void objectsFreeAll(void);
