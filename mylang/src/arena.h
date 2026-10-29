#pragma once
// Memory for everything the front end builds (tokens and AST nodes).
// Nothing is freed one piece at a time: the whole arena is released at exit by arenaFreeAll().
// That also means a syntax error can abandon a half-built tree without leaking anything.
#include <stddef.h>

// Returns zeroed memory, aligned for any type.
void* arenaAllocate(size_t size);

// Copies `length` bytes of `text` and adds a terminating '\0'.
char* arenaDuplicateString(const char* text, size_t length);

// Growable arrays: makes sure `array` (which currently holds `count` elements and has room for *capacity)
// has space for one more element, and returns the (possibly moved) array.
void* arenaReserveArray(void* array, int count, int* capacity, size_t elementSize);

// Releases every block at once. Everything the arena handed out becomes invalid.
void arenaFreeAll(void);

// A growable list of pointers, used while the parser collects statements, arguments and parameters.
typedef struct {
    void** items;  // the pointers, in insertion order
    int count;     // number of pointers stored
    int capacity;  // number of slots allocated in items
} PtrVec;

// Appends `item`, growing the list (in the arena) when it is full.
void pointerVectorPush(PtrVec* vector, void* item);
