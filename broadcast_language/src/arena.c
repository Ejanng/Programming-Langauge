// Arena allocator: hands out memory from large blocks and frees all of it at once at exit.
// The interface and the reasoning behind it are in arena.h.
#include "arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE (64 * 1024)

// One chunk of arena memory. The usable bytes follow the header directly.
typedef struct Block {
    struct Block* next;  // the previously allocated block (blocks form a singly linked list)
    size_t used;         // bytes of this block already handed out
    size_t size;         // bytes of usable space in this block
    // Forces the data that follows to be aligned for any type. A union is used instead of C11's
    // max_align_t because MSVC does not define max_align_t when compiling C.
    union { long double longDouble; long long longLong; void* pointer; void (*functionPointer)(void); } align;
} Block;

static Block* blocks = NULL;

static Block* newBlock(size_t size) {
    Block* block = calloc(1, sizeof(Block) + size);  // calloc: arena memory starts out zeroed
    if (!block) {
        fputs("Out of memory.\n", stderr);
        exit(70);
    }
    block->size = size;
    return block;
}

void* arenaAllocate(size_t size) {
    size = (size + 15) & ~(size_t)15;
    if (size > BLOCK_SIZE / 4) {  // big requests get a block of their own
        Block* block = newBlock(size);
        if (blocks) {
            block->next = blocks->next;  // keep the current bump block at the head
            blocks->next = block;
        } else {
            blocks = block;
        }
        block->used = size;
        return (char*)(block + 1);
    }
    if (!blocks || blocks->used + size > blocks->size) {
        Block* block = newBlock(BLOCK_SIZE);
        block->next = blocks;
        blocks = block;
    }
    void* pointer = (char*)(blocks + 1) + blocks->used;
    blocks->used += size;
    return pointer;
}

char* arenaDuplicateString(const char* text, size_t length) {
    char* copy = arenaAllocate(length + 1);
    memcpy(copy, text, length);
    return copy;
}

void* arenaReserveArray(void* array, int count, int* capacity, size_t elementSize) {
    if (count < *capacity) return array;
    int newCapacity = *capacity < 8 ? 8 : *capacity * 2;
    void* grown = arenaAllocate((size_t)newCapacity * elementSize);
    if (count > 0) memcpy(grown, array, (size_t)count * elementSize);
    *capacity = newCapacity;
    return grown;
}

void arenaFreeAll(void) {
    while (blocks) {
        Block* next = blocks->next;
        free(blocks);
        blocks = next;
    }
}

void pointerVectorPush(PtrVec* vector, void* item) {
    vector->items = arenaReserveArray(vector->items, vector->count, &vector->capacity, sizeof(void*));
    vector->items[vector->count++] = item;
}
