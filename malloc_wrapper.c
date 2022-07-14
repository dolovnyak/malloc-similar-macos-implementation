#include "malloc.h"
#include "macos_similar_malloc_implementation/malloc_internal.h"

void* malloc(size_t size) {
    return __malloc(size);
}

void* realloc(void* ptr, size_t size) {
    return __realloc(ptr, size);
}

void free(void* ptr) {
    __free(ptr);
}

void* calloc(size_t count, size_t size) {
    void* ptr = __malloc(count * size);
    bzero(ptr, count * size);
    return ptr;
}

void* valloc(size_t size) {
    if (!gInit) {
        __malloc(0);
    }
    return __malloc(size + gPageSize - size % gPageSize);
}

void free_all() {
    __free_all();
}

void print_alloc_mem() {
    __print_alloc_mem();
}

void print_alloc_mem_hex_dump() {
    __print_alloc_mem_hex_dump();
}
