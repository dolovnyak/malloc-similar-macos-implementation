#include <gtest/gtest.h>

extern "C" {
#include "malloc_internal.h"
#include "utilities.h"
}

TEST(Realloc, Large) {
    __free_all();

    void* mem = __malloc(SMALL_ALLOCATION_MAX_SIZE + 1);

    void* mem1 = __realloc(mem, SMALL_ALLOCATION_MAX_SIZE);
    ASSERT_EQ(mem, mem1);

    void* mem2 = __realloc(mem1, SMALL_ALLOCATION_MAX_SIZE + 2);
    ASSERT_EQ(mem1, mem2);

    void* mem3 = __realloc(mem2, SMALL_ALLOCATION_MAX_SIZE * 16);
    ASSERT_FALSE(mem2 == mem3);

    void* mem4 = __realloc(mem3, 0);
    ASSERT_FALSE(mem3 == mem4);
}

TEST(Realloc, Tiny_Small) {
    __free_all();


}