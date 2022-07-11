#include "malloc.h"
#include <stdio.h>

int main() {
    print_alloc_mem();

    char* a = (char*)malloc(16);
    print_alloc_mem();
    free(a);
    print_alloc_mem();

    void* arr_tiny_ptr[20];
    for (int i = 0; i < 20; ++i) {
        arr_tiny_ptr[i] = malloc(16);
    }

    void* arr_small_ptr[20];
    for (int i = 0; i < 20; ++i) {
        arr_small_ptr[i] = malloc(129);
    }

    void* arr_large_ptr[20];
    for (int i = 0; i < 20; ++i) {
        arr_large_ptr[i] = malloc(1023);
    }
    print_alloc_mem();

    for (int i = 0; i < 20; i+=2) {
        free(arr_tiny_ptr[i]);
    }

    for (int i = 0; i < 20; i+=2) {
        free(arr_small_ptr[i]);
    }

    for (int i = 0; i < 20; i+=2) {
        free(arr_large_ptr[i]);
    }
    print_alloc_mem();

    for (int i = 3; i < 20; i+=2) {
        free(arr_tiny_ptr[i]);
    }

    for (int i = 3; i < 20; i+=2) {
        free(arr_small_ptr[i]);
    }

    for (int i = 3; i < 20; i+=2) {
        free(arr_large_ptr[i]);
    }
    print_alloc_mem();

    free(arr_tiny_ptr[1]);
    free(arr_small_ptr[1]);
    free(arr_large_ptr[1]);
    print_alloc_mem();

    a = malloc(32);
    a[0] = 'a';
    a[1] = 'b';
    a[17] = 'k';
    char* b = malloc(64);
    b[0] = 'O';
    b[38] = 'k';
    b[63] = '2';

    char*c = malloc(129);
    c[0] = 'l';
    c[128] = 'k';

    char* d = malloc(1024);
    d[0] = 'x';
    d[1023] = 'O';

    char* g = malloc(1024);
    g[0] = 'X';
    g[1023] = 'o';
    print_alloc_mem_hex_dump();
}