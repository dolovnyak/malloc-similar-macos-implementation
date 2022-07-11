#include <gtest/gtest.h>
#include <array>

extern "C" {
#include "malloc_internal.h"
#include "utilities.h"
}

TEST(Free_All, Check_Correct) {
    void* mem = __malloc(10);
    ASSERT_EQ(gInit, true);

    __free_all();

    ASSERT_EQ(gInit, false);
    ASSERT_EQ(gMemoryZones.first_tiny_zone, nullptr);
    ASSERT_EQ(gMemoryZones.first_small_zone, nullptr);
    ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
}

TEST(Free, Large) {
    __free_all();

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
    __free_all();
    std::array<void*, 60000> ptr_arr{};

    {
        /// free both zones from the end
        for (uint64_t i = 0; i < 60000; ++i) {
            ptr_arr[i] = __malloc(16);
        }
        ASSERT_FALSE(gMemoryZones.first_tiny_zone == gMemoryZones.last_tiny_zone);
        ASSERT_EQ(gMemoryZones.first_tiny_zone->next, gMemoryZones.last_tiny_zone);
        for (uint64_t i = 60000; i > 0; --i) {
            __free(ptr_arr[i - 1]);
        }
        ASSERT_EQ(gMemoryZones.first_tiny_zone, gMemoryZones.last_tiny_zone);
        ASSERT_EQ((BYTE*)gMemoryZones.first_tiny_zone->last_allocated_node, nullptr);
    }

    {
        /// free both zones from the begin
        for (uint64_t i = 0; i < 60000; ++i) {
            ptr_arr[i] = __malloc(16);
        }
        ASSERT_FALSE(gMemoryZones.first_tiny_zone == gMemoryZones.last_tiny_zone);
        ASSERT_EQ(gMemoryZones.first_tiny_zone->next, gMemoryZones.last_tiny_zone);
        for (uint64_t i = 0; i < 60000; ++i) {
            __free(ptr_arr[i]);
        }
        ASSERT_EQ(gMemoryZones.first_tiny_zone, gMemoryZones.last_tiny_zone);
        ASSERT_EQ((BYTE*)gMemoryZones.first_tiny_zone->last_allocated_node, nullptr);
    }

    {
        /// free through one
        t_zone* zone = gMemoryZones.first_tiny_zone;

        for (uint64_t i = 0; i < 1000; ++i) {
            ptr_arr[i] = __malloc(16);
        }
        for (uint64_t i = 0; i < 1000; i+=2) {
                __free(ptr_arr[i]);
        }
        uint64_t idx = 0;
        for (BYTE* node = (BYTE*)zone + ZONE_HEADER_SIZE; node != zone->last_allocated_node; node += NODE_HEADER_SIZE + 16) {
            if (idx % 2 == 0) {
                ASSERT_EQ(get_node_available(node), TRUE);
            }
            else {
                ASSERT_EQ(get_node_available(node), FALSE);
            }
            ++idx;
        }
        for (uint64_t i = 1; i < 1000; i+=2) {
            __free(ptr_arr[i]);
        }
        ASSERT_EQ((BYTE*)gMemoryZones.first_tiny_zone->last_allocated_node, nullptr);
    }
}