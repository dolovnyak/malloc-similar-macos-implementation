#include <gtest/gtest.h>
#include <array>

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

    constexpr uint64_t max_nodes_number_in_tiny_zone = (TINY_ZONE_SIZE - ZONE_HEADER_SIZE) / (16 + NODE_HEADER_SIZE);
    std::array<void*, max_nodes_number_in_tiny_zone> ptr_arr1{};
    std::array<void*, max_nodes_number_in_tiny_zone / 2> ptr_arr2{};

    /// fill full zone
    for (uint64_t i = 0; i < max_nodes_number_in_tiny_zone; ++i) {
        ptr_arr1[i] = __malloc(16);
    }
    ASSERT_EQ(gMemoryZones.first_tiny_zone, gMemoryZones.last_tiny_zone);
    ASSERT_TRUE(get_zone_not_used_mem_size(gMemoryZones.first_tiny_zone) < 32);

    /// free trough one from the begin
    for (uint64_t i = 1; i < max_nodes_number_in_tiny_zone; i+=2) {
        __free(ptr_arr1[i]);
    }

    /// last node goes to not used space
    ASSERT_TRUE(get_zone_not_used_mem_size(gMemoryZones.first_tiny_zone) > 32);

    uint64_t j = 0;
    for (uint64_t i = 0; i < max_nodes_number_in_tiny_zone; i+=2) {
        ptr_arr2[j] = __realloc(ptr_arr1[i], 48);
        ++j;
    }
    ASSERT_EQ(gMemoryZones.first_tiny_zone, gMemoryZones.last_tiny_zone);
    ASSERT_TRUE(get_zone_not_used_mem_size(gMemoryZones.first_tiny_zone) < 32);

    for (uint64_t i = 1; i < ptr_arr2.size(); ++i) {
        __free(ptr_arr2[i]);
    }
    ASSERT_EQ(get_zone_not_used_mem_size(gMemoryZones.first_tiny_zone), TINY_ZONE_SIZE - ZONE_HEADER_SIZE - NODE_HEADER_SIZE - 48);

    ptr_arr2[0] = __realloc(ptr_arr2[0], 112);
    ASSERT_EQ(get_zone_not_used_mem_size(gMemoryZones.first_tiny_zone), TINY_ZONE_SIZE - ZONE_HEADER_SIZE - NODE_HEADER_SIZE - 112);

    ptr_arr2[0] = __realloc(ptr_arr2[0], 16);
    ASSERT_EQ(get_zone_not_used_mem_size(gMemoryZones.first_tiny_zone), TINY_ZONE_SIZE - ZONE_HEADER_SIZE - NODE_HEADER_SIZE - 112);
}