#include "malloc.h"
#include "macos_similar_malloc_implementation/malloc_internal.h"
#include <pthread.h>

static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;

static inline void mutex_lock(pthread_mutex_t* mutex) {
#ifdef THREAD_SAFE
    pthread_mutex_lock(mutex);
#endif
}

static inline void mutex_unlock(pthread_mutex_t* mutex) {
#ifdef THREAD_SAFE
    pthread_mutex_unlock(mutex);
#endif
}

void* malloc(size_t size) {
    mutex_lock(&gMutex);
    void* tmp = __malloc(size);
    mutex_unlock(&gMutex);
    return tmp;
}

void* realloc(void* ptr, size_t size) {
    mutex_lock(&gMutex);
    void* tmp = __realloc(ptr, size);
    mutex_unlock(&gMutex);
    return tmp;
}

void free(void* ptr) {
    mutex_lock(&gMutex);
    __free(ptr);
    mutex_unlock(&gMutex);
}

void* calloc(size_t count, size_t size) {
    mutex_lock(&gMutex);
    void* ptr = __malloc(count * size);
    bzero(ptr, count * size);
    mutex_unlock(&gMutex);
    return ptr;
}

void* valloc(size_t size) {
    mutex_lock(&gMutex);
    /// needed for gPageSize
    if (!gInit) {
        init();
    }
    void* tmp = __malloc(size + gPageSize - size % gPageSize);
    mutex_unlock(&gMutex);
    return tmp;
}

void free_all() {
    mutex_lock(&gMutex);
    __free_all();
    mutex_unlock(&gMutex);
}

void print_alloc_mem() {
    mutex_lock(&gMutex);
    __print_alloc_mem();
    mutex_unlock(&gMutex);
}

void print_alloc_mem_hex_dump() {
    mutex_lock(&gMutex);
    __print_alloc_mem_hex_dump();
    mutex_unlock(&gMutex);
}
