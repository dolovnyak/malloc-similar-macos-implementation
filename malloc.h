#pragma once

#include <string.h>

void *malloc(size_t size);

void *realloc(void *ptr, size_t size);

void* calloc(size_t count, size_t size);

void* valloc(size_t size);

void free(void *ptr);

void free_all();

void print_alloc_mem();

void print_alloc_mem_hex_dump();
