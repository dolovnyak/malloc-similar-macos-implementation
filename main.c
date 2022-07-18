#include "malloc.h"
#include <sys/time.h>
#include <stdio.h>

static inline size_t timeval_to_size_t(struct timeval timeval)
{
    return (timeval.tv_sec * 1000 + (size_t)(timeval.tv_usec * 0.001));
}

static inline size_t get_current_time(void)
{
    struct timeval	timeval;

    gettimeofday(&timeval, NULL);
    return (timeval_to_size_t(timeval));
}

int main() {
    size_t start_time = get_current_time();

    char* a = (char*)malloc(16);
    free(a);

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

    for (int i = 0; i < 20; i+=2) {
        free(arr_tiny_ptr[i]);
    }

    for (int i = 0; i < 20; i+=2) {
        free(arr_small_ptr[i]);
    }

    for (int i = 0; i < 20; i+=2) {
        free(arr_large_ptr[i]);
    }

    for (int i = 3; i < 20; i+=2) {
        free(arr_tiny_ptr[i]);
    }

    for (int i = 3; i < 20; i+=2) {
        free(arr_small_ptr[i]);
    }

    for (int i = 3; i < 20; i+=2) {
        free(arr_large_ptr[i]);
    }

    free(arr_tiny_ptr[1]);
    free(arr_small_ptr[1]);
    free(arr_large_ptr[1]);

    a = malloc(32);
    char* b = malloc(64);

    char*c = malloc(129);

    char* d = malloc(1024);

    char* g = malloc(1024);

    void* arr[10000];
    for (int i = 0; i < 10000; ++i) {
        arr[i] = malloc(64);
    }
    for (int i = 0; i < 10000; ++i) {
        arr[i] = realloc(arr[i],82);
    }
    for (int i = 0; i < 10000; ++i) {
        free(arr[i]);
    }
    printf("%lu\n", get_current_time() - start_time);
}