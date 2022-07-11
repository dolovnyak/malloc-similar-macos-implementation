#include "malloc.h"
#include "macos_similar_malloc_implementation//malloc_internal.h"

void* malloc(size_t size) {
    return __malloc(size);
}

void* realloc(void* ptr, size_t size) {
    return __realloc(ptr, size);
}

void free(void* ptr) {
    __free(ptr);
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
