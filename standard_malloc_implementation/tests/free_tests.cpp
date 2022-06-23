#include <gtest/gtest.h>

extern "C" {
#include "malloc_internal.h"
}

TEST(Free_All, Check_Correct) {
    void* mem = malloc(10);
    ASSERT_EQ(gInit, true);

    free_all();

    ASSERT_EQ(gInit, false);
    ASSERT_EQ(gMemoryZones.first_tiny_zone, nullptr);
    ASSERT_EQ(gMemoryZones.first_small_zone, nullptr);
    ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
}