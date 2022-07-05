#pragma once

#include <string.h>

void* malloc(size_t required_size);
void* realloc(void* ptr, size_t new_size);
void free_all();
void free(void* ptr);
