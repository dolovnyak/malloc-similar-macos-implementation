#include <pthread.h>
#include <stdio.h>
//#include "malloc.h"
#include <stdlib.h>
#include <sys/time.h>

void checker(int type, size_t size) {
    int arr_elems = 100;
    void* arr[arr_elems];

    for (int i = 0; i < arr_elems; ++i) {
        switch (type) {
        case 0:
                arr[i] = malloc(size);
            break;
        case 1:
                arr[i] = calloc(i + 1, size);
            break;
        case 2:
                arr[i] = valloc(size);
            break;
        default:
            printf("incorrect type\n");
        }
    }

    for (int i = 0; i < arr_elems; ++i) {
        arr[i] = realloc(arr[i], size * 2);
    }
    for(int i = 0; i < arr_elems; ++i) {
        free(arr[i]);
    }
}

void *thread(void *ptr)
{
    for (int i = 0; i < 1000; ++i) {
        int type = (unsigned)rand() % 2;
        size_t size = (size_t)rand() % 1013 + 1;
        checker(type, size);
    }
    return ptr;
}

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

int main()
{
    size_t start_time = get_current_time();

    size_t thread_num = 16;
    pthread_t threads[thread_num];

	for (size_t i = 0; i < thread_num; ++i) {
		pthread_create(&threads[i], NULL, thread, (void*)i);
	}
	for (size_t i = 0; i < thread_num; ++i) {
		pthread_join(threads[i], NULL);
	}

    printf("%lu\n", get_current_time() - start_time);

//    print_alloc_mem();
    return 0;
}
