#include <gtest/gtest.h>
#include <array>

extern "C" {
#include "malloc_internal.h"
}

TEST(Free_All, Check_Correct) {
    void* mem = __malloc(10);
    ASSERT_EQ(gInit, true);

    free_all();

    ASSERT_EQ(gInit, false);
    ASSERT_EQ(gMemoryZones.first_tiny_zone, nullptr);
    ASSERT_EQ(gMemoryZones.first_small_zone, nullptr);
    ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
}

TEST(Free, Large) {
    free_all();

    void* mem1 = __malloc(SMALL_ALLOCATION_MAX_SIZE + 1);
    ASSERT_EQ((BYTE*)gMemoryZones.first_large_allocation, (BYTE*)mem1 - NODE_HEADER_SIZE - ZONE_HEADER_SIZE);

    void* mem2 = __malloc(SMALL_ALLOCATION_MAX_SIZE + 1);
    ASSERT_FALSE(gMemoryZones.first_large_allocation == gMemoryZones.last_large_allocation);
    ASSERT_EQ((BYTE*)gMemoryZones.last_large_allocation, (BYTE*)mem2 - NODE_HEADER_SIZE - ZONE_HEADER_SIZE);

    __free(mem1);
    ASSERT_TRUE(gMemoryZones.first_large_allocation == gMemoryZones.last_large_allocation);
    ASSERT_EQ((BYTE*)gMemoryZones.first_large_allocation, (BYTE*)mem2 - NODE_HEADER_SIZE - ZONE_HEADER_SIZE);

    __free(mem2);
    ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
    ASSERT_EQ(gMemoryZones.last_large_allocation, nullptr);
}

TEST(Free, Tiny_Small) {
    free_all();
    std::array<void*, 60000> ptr_arr{};

    for (uint64_t i = 0; i < 60000; ++i) {
        void* mem = __malloc(16);
        ptr_arr[i] = mem;
    }
    ASSERT_FALSE(gMemoryZones.first_tiny_zone == gMemoryZones.last_tiny_zone);
    ASSERT_EQ(gMemoryZones.first_tiny_zone->next, gMemoryZones.last_tiny_zone);
    for (uint64_t i = 60000; i > 0; --i) {
        __free(ptr_arr[i - 1]);
    }
    ASSERT_EQ(gMemoryZones.first_tiny_zone, gMemoryZones.last_tiny_zone);
    ASSERT_EQ((BYTE*)gMemoryZones.first_tiny_zone->last_allocated_node, nullptr);
}