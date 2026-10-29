// Runtime values: heap objects (strings, scopes, functions), the string
// hash table, truthiness, equality, and how values are printed.
#include "value.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

static Obj* allObjects = NULL;

static void* allocateObject(size_t size, ObjType type) {
    Obj* object = calloc(1, size);
    if (!object) {
        fputs("Out of memory.\n", stderr);
        exit(EXIT_FAILURE);
    }
    object->type = type;
    object->next = allObjects;
    allObjects = object;
    return object;
}

void objectsFreeAll(void) {
    while (allObjects) {
        Obj* next = allObjects->next;
        if (allObjects->type == OBJ_ENV) tableFree(&((ObjEnv*)allObjects)->variables);
        free(allObjects);
        allObjects = next;
    }
}

// ---- strings ----

static ObjString* stringAllocate(size_t length) {
    ObjString* string = allocateObject(sizeof(ObjString) + length + 1, OBJ_STRING);
    string->length = length;
    string->characters[length] = '\0';
    return string;
}

ObjString* stringCopy(const char* characters, size_t length) {
    ObjString* string = stringAllocate(length);
    memcpy(string->characters, characters, length);
    return string;
}

ObjString* stringConcatenate(const ObjString* left, const ObjString* right) {
    ObjString* string = stringAllocate(left->length + right->length);
    memcpy(string->characters, left->characters, left->length);
    memcpy(string->characters + left->length, right->characters, right->length);
    return string;
}

// ---- table ----

static uint32_t hashString(const char* key) {  // FNV-1a
    uint32_t hash = 2166136261u;
    for (; *key; key++) {
        hash ^= (uint8_t)*key;
        hash *= 16777619u;
    }
    return hash;
}

static TableEntry* findEntry(TableEntry* entries, int capacity, const char* key, uint32_t hash) {
    uint32_t i = hash & (uint32_t)(capacity - 1);  // capacity is a power of two
    for (;;) {
        TableEntry* entry = &entries[i];
        if (!entry->key || (entry->hash == hash && strcmp(entry->key, key) == 0)) return entry;
        i = (i + 1) & (uint32_t)(capacity - 1);
    }
}

void tableFree(Table* table) {
    free(table->entries);
    table->entries = NULL;
    table->count = table->capacity = 0;
}

static void tableGrow(Table* table) {
    int newCapacity = table->capacity < 8 ? 8 : table->capacity * 2;
    TableEntry* entries = calloc((size_t)newCapacity, sizeof(TableEntry));
    if (!entries) {
        fputs("Out of memory.\n", stderr);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < table->capacity; i++) {
        if (!table->entries[i].key) continue;
        *findEntry(entries, newCapacity, table->entries[i].key, table->entries[i].hash) = table->entries[i];
    }
    free(table->entries);
    table->entries = entries;
    table->capacity = newCapacity;
}

void tableSet(Table* table, const char* key, Value value) {
    if (table->count + 1 > table->capacity * 3 / 4) tableGrow(table);
    uint32_t hash = hashString(key);
    TableEntry* entry = findEntry(table->entries, table->capacity, key, hash);
    if (!entry->key) table->count++;
    entry->key = key;
    entry->hash = hash;
    entry->value = value;
}

Value* tableGet(const Table* table, const char* key) {
    if (table->count == 0) return NULL;
    TableEntry* entry = findEntry(table->entries, table->capacity, key, hashString(key));
    return entry->key ? &entry->value : NULL;
}

// ---- environments and functions (tree-walker) ----

ObjEnv* environmentNew(ObjEnv* parent) {
    ObjEnv* environment = calloc(1, sizeof(ObjEnv));
    if (!environment) {
        fputs("Out of memory.\n", stderr);
        exit(EXIT_FAILURE);
    }
    environment->object.type = OBJ_ENV;
    environment->parent = parent;
    return environment;
}

void environmentRelease(ObjEnv* environment) {
    if (environment->captured) return;
    tableFree(&environment->variables);
    free(environment);
}

// Captured scopes join the global object list. A captured scope's parents are always captured too.
void environmentCapture(ObjEnv* environment) {
    for (; environment && !environment->captured; environment = environment->parent) {
        environment->captured = true;
        environment->object.next = allObjects;
        allObjects = &environment->object;
    }
}

void environmentDefine(ObjEnv* environment, const char* name, Value value) { tableSet(&environment->variables, name, value); }

bool environmentAssign(ObjEnv* environment, const char* name, Value value) {
    Value* slot = environmentLookup(environment, name);
    if (!slot) return false;
    *slot = value;
    return true;
}

Value* environmentLookup(const ObjEnv* environment, const char* name) {
    for (; environment; environment = environment->parent) {
        Value* value = tableGet(&environment->variables, name);
        if (value) return value;
    }
    return NULL;
}

ObjFunction* functionNew(const Stmt* declaration, ObjEnv* closure) {
    ObjFunction* function = allocateObject(sizeof(ObjFunction), OBJ_FUNCTION);
    function->declaration = declaration;
    function->closure = closure;
    environmentCapture(closure);
    return function;
}

// ---- operations on values ----

bool isTruthy(Value value) {
    if (value.type == VAL_NIL) return false;
    if (value.type == VAL_BOOL) return value.as.boolean;
    return true;  // 0 and "" are truthy
}

bool valuesEqual(Value left, Value right) {
    if (left.type != right.type) return false;
    switch (left.type) {
        case VAL_NIL: return true;
        case VAL_NUMBER: return left.as.number == right.as.number;
        case VAL_BOOL: return left.as.boolean == right.as.boolean;
        case VAL_OBJ:
            if (left.as.object == right.as.object) return true;
            if (left.as.object->type == OBJ_STRING && right.as.object->type == OBJ_STRING) {
                const ObjString* leftString = (const ObjString*)left.as.object;
                const ObjString* rightString = (const ObjString*)right.as.object;
                return leftString->length == rightString->length && memcmp(leftString->characters, rightString->characters, leftString->length) == 0;
            }
            return false;  // functions are equal only if they are the same function
    }
    return false;
}

int formatNumber(char* buffer, size_t capacity, double number) {
    if (isnan(number)) return snprintf(buffer, capacity, "nan");
    if (isinf(number)) return snprintf(buffer, capacity, "%s", number < 0 ? "-inf" : "inf");
    // Whole numbers up to 2^53 are stored exactly in a double, so print every digit
    // (9007199254740992, not 9.00719925474099e+15). "%.15g" covers everything else.
    if (number == floor(number) && fabs(number) <= 9007199254740992.0) return snprintf(buffer, capacity, "%.0f", number == 0 ? 0.0 : number);
    return snprintf(buffer, capacity, "%.15g", number);
}

static ObjString* stringFromCString(const char* string) { return stringCopy(string, strlen(string)); }

ObjString* valueToString(Value value) {
    switch (value.type) {
        case VAL_NIL: return stringFromCString("silence");
        case VAL_BOOL: return stringFromCString(value.as.boolean ? "affirm" : "negative");
        case VAL_NUMBER: {
            char buffer[64];
            formatNumber(buffer, sizeof buffer, value.as.number);
            return stringFromCString(buffer);
        }
        case VAL_OBJ: break;
    }
    switch (value.as.object->type) {
        case OBJ_STRING: return AS_STRING(value);
        case OBJ_FUNCTION: {
            const char* name = ((ObjFunction*)value.as.object)->declaration->name;
            ObjString* string = stringAllocate(strlen(name) + 11);  // "<protocol " + name + ">"
            snprintf(string->characters, string->length + 1, "<protocol %s>", name);
            return string;
        }
        default: return stringFromCString("<object>");
    }
}
