#pragma once

#include <string.h>

void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void free_all();
void free(void* ptr);