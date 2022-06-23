#include <gtest/gtest.h>

extern "C" {
#include "malloc_internal.h"
}

#define TEST_ZONE_SIZE 4096

static BYTE* test_zone = (BYTE*)mmap(0, TEST_ZONE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
                                     VM_MAKE_TAG(VM_MEMORY_MALLOC), 0);
static BYTE* test_empty_zone = (BYTE*)mmap(0, sizeof(t_zone), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
                                           VM_MAKE_TAG(VM_MEMORY_MALLOC), 0);

TEST(Split_Node, CheckCorrect) {
    bzero(test_zone, TEST_ZONE_SIZE);
    t_memory_node* node = (t_memory_node*)test_zone;
    node->next = NULL;
    node->available = true;
    node->usable_size = 128;
    size_t left_part_size = 40;
    t_memory_node* splitted_node = SplitNode(node, left_part_size);
    ASSERT_EQ((BYTE*)splitted_node, (BYTE*)node + SIZE_WITH_NODE_HEADER(left_part_size));
    ASSERT_EQ(splitted_node->usable_size, 128 - SIZE_WITH_NODE_HEADER(left_part_size));
    ASSERT_EQ(splitted_node->next, nullptr);
}

TEST(Take_Mem_From_Free_Nodes, Empty_List) {
    t_zone* zone = (t_zone*)test_zone;
    bzero(test_zone, TEST_ZONE_SIZE);
    zone->next = nullptr;
    zone->total_size = TEST_ZONE_SIZE;
    zone->first_free_node = nullptr;
    zone->last_free_node = nullptr;
    void* mem = TakeMemoryFromZoneList(zone, 16, 64);
    ASSERT_EQ(mem, nullptr);
}

TEST(Take_Mem_From_Free_Nodes, Main_Check) {
    t_zone* empty_zone = (t_zone*)test_empty_zone;
    empty_zone->first_free_node = nullptr;
    empty_zone->last_free_node = nullptr;
    empty_zone->total_size = 1337;
    empty_zone->available_size = 0;

    {
        /// check first node without separated
        bzero(test_zone, TEST_ZONE_SIZE);
        t_memory_node* node1 = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        node1->usable_size = 38;
        node1->available = true;

        t_memory_node* node2 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node1) + node1->usable_size);
        node2->usable_size = 26;
        node2->available = true;
        node1->next = node2;

        t_memory_node* node3 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node2) + node2->usable_size);
        node3->usable_size = 64;
        node3->available = true;
        node2->next = node3;
        node3->next = nullptr;

        t_zone* zone = (t_zone*)test_zone;
        zone->available_size = 0;
        zone->total_size = TEST_ZONE_SIZE;
        zone->first_free_node = node1;
        zone->last_free_node = node3;
        zone->next = nullptr;
        empty_zone->next = zone;

        void* mem = TakeMemoryFromZoneList(empty_zone, 16, 64);
        ASSERT_TRUE(mem != nullptr);
        ASSERT_EQ(zone->first_free_node, node2);
        ASSERT_EQ(zone->last_free_node, node3);
        ASSERT_EQ(node2->next, node3);
        ASSERT_EQ(node3->next, nullptr);
        t_memory_node* node = (t_memory_node*)CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone);
        ASSERT_EQ(CAST_TO_BYTE_APPLY_NODE_SHIFT(node), (BYTE*)mem);
        ASSERT_EQ(node, node1);
        ASSERT_EQ(node->available, false);
        ASSERT_EQ(node->usable_size, 38);
        ASSERT_EQ(node->next, nullptr);
    }

    {
        /// check last node without separated
        bzero(test_zone, TEST_ZONE_SIZE);
        t_memory_node* node1 = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        node1->usable_size = 38;
        node1->available = true;

        t_memory_node* node2 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node1) + node1->usable_size);
        node2->usable_size = 26;
        node2->available = true;
        node1->next = node2;

        t_memory_node* node3 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node2) + node2->usable_size);
        node3->usable_size = 111; // in 112 it will be separate
        node3->available = true;
        node2->next = node3;
        node3->next = nullptr;

        t_zone* zone = (t_zone*)test_zone;
        zone->available_size = 0;
        zone->total_size = TEST_ZONE_SIZE;
        zone->first_free_node = node1;
        zone->last_free_node = node3;
        zone->next = empty_zone;
        empty_zone->next = nullptr;

        void* mem = TakeMemoryFromZoneList(zone, 48, 64);
        ASSERT_TRUE(mem != nullptr);
        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(zone->last_free_node, node2);
        ASSERT_EQ(node1->next, node2);
        ASSERT_EQ(node2->next, nullptr);
        t_memory_node* node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone) + 38 + 26 +
                                               sizeof(t_memory_node) * 2);
        ASSERT_EQ(CAST_TO_BYTE_APPLY_NODE_SHIFT(node), (BYTE*)mem);
        ASSERT_EQ(node, node3);
        ASSERT_EQ(node->available, false);
        ASSERT_EQ(node->usable_size, 111);
        ASSERT_EQ(node->next, nullptr);
    }

    {
        /// check middle node without separated
        bzero(test_zone, TEST_ZONE_SIZE);
        t_memory_node* node1 = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        node1->usable_size = 38;
        node1->available = true;

        t_memory_node* node2 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node1) + node1->usable_size);
        node2->usable_size = 56;
        node2->available = true;
        node1->next = node2;

        t_memory_node* node3 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node2) + node2->usable_size);
        node3->usable_size = 64;
        node3->available = true;
        node2->next = node3;
        node3->next = nullptr;

        t_zone* zone = (t_zone*)test_zone;
        zone->available_size = 0;
        zone->total_size = TEST_ZONE_SIZE;
        zone->first_free_node = node1;
        zone->last_free_node = node3;
        zone->next = empty_zone;
        empty_zone->next = nullptr;

        void* mem = TakeMemoryFromZoneList(zone, 48, 64);
        ASSERT_TRUE(mem != nullptr);
        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(zone->last_free_node, node3);
        ASSERT_EQ(node1->next, node3);
        ASSERT_EQ(node3->next, nullptr);
        t_memory_node* node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone) + 38 + sizeof(t_memory_node));
        ASSERT_EQ(CAST_TO_BYTE_APPLY_NODE_SHIFT(node), (BYTE*)mem);
        ASSERT_EQ(node, node2);
        ASSERT_EQ(node->available, false);
        ASSERT_EQ(node->usable_size, 56);
        ASSERT_EQ(node->next, nullptr);
    }

    {
        /// check first node with separated
        bzero(test_zone, TEST_ZONE_SIZE);
        t_memory_node* node1 = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        node1->usable_size = 136;
        node1->available = true;

        t_memory_node* node2 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node1) + node1->usable_size);
        node2->usable_size = 162;
        node2->available = true;
        node1->next = node2;

        t_memory_node* node3 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node2) + node2->usable_size);
        node3->usable_size = 181;
        node3->available = true;
        node2->next = node3;
        node3->next = nullptr;

        t_zone* zone = (t_zone*)test_zone;
        zone->available_size = 0;
        zone->total_size = TEST_ZONE_SIZE;
        zone->first_free_node = node1;
        zone->last_free_node = node3;
        zone->next = empty_zone;
        empty_zone->next = nullptr;

        void* mem = TakeMemoryFromZoneList(zone, 48, 64);
        ASSERT_TRUE(mem != nullptr);

        t_memory_node* new_separated_node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone) + 48 +
                                                             sizeof(t_memory_node));
        ASSERT_EQ(new_separated_node->usable_size, 136 - 48 - sizeof(t_memory_node));
        ASSERT_EQ(new_separated_node->available, true);
        ASSERT_EQ(new_separated_node->next, node2);
        ASSERT_EQ(node2->next, node3);
        ASSERT_EQ(node3->next, nullptr);
        ASSERT_EQ(zone->first_free_node, new_separated_node);
        ASSERT_EQ(zone->last_free_node, node3);
        t_memory_node* node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        ASSERT_EQ(CAST_TO_BYTE_APPLY_NODE_SHIFT(node), (BYTE*)mem);
        ASSERT_EQ(node, node1);
        ASSERT_EQ(node->available, false);
        ASSERT_EQ(node->usable_size, 48);
        ASSERT_EQ(node->next, nullptr);
    }

    {
        /// check middle node with separated
        bzero(test_zone, TEST_ZONE_SIZE);
        t_memory_node* node1 = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        node1->usable_size = 47;
        node1->available = true;

        t_memory_node* node2 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node1) + node1->usable_size);
        node2->usable_size = 162;
        node2->available = true;
        node1->next = node2;

        t_memory_node* node3 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node2) + node2->usable_size);
        node3->usable_size = 181;
        node3->available = true;
        node2->next = node3;
        node3->next = nullptr;

        t_zone* zone = (t_zone*)test_zone;
        zone->available_size = 0;
        zone->total_size = TEST_ZONE_SIZE;
        zone->first_free_node = node1;
        zone->last_free_node = node3;
        zone->next = empty_zone;
        empty_zone->next = nullptr;

        void* mem = TakeMemoryFromZoneList(zone, 48, 64);
        ASSERT_TRUE(mem != nullptr);

        t_memory_node* new_separated_node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone) + 47 + 48 +
                                                             sizeof(t_memory_node) * 2);
        ASSERT_EQ(new_separated_node->usable_size, 162 - 48 - sizeof(t_memory_node));
        ASSERT_EQ(new_separated_node->available, true);
        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(node1->next, new_separated_node);
        ASSERT_EQ(new_separated_node->next, node3);
        ASSERT_EQ(node3->next, nullptr);
        ASSERT_EQ(zone->last_free_node, node3);
        t_memory_node* node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone) + SIZE_WITH_NODE_HEADER(47));
        ASSERT_EQ(CAST_TO_BYTE_APPLY_NODE_SHIFT(node), (BYTE*)mem);
        ASSERT_EQ(node, node2);
        ASSERT_EQ(node->available, false);
        ASSERT_EQ(node->usable_size, 48);
        ASSERT_EQ(node->next, nullptr);
    }

    {
        /// check last node with separated
        bzero(test_zone, TEST_ZONE_SIZE);
        t_memory_node* node1 = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        node1->usable_size = 47;
        node1->available = true;

        t_memory_node* node2 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node1) + node1->usable_size);
        node2->usable_size = 22;
        node2->available = true;
        node1->next = node2;

        t_memory_node* node3 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node2) + node2->usable_size);
        node3->usable_size = 181;
        node3->available = true;
        node2->next = node3;
        node3->next = nullptr;

        t_zone* zone = (t_zone*)test_zone;
        zone->available_size = 0;
        zone->total_size = TEST_ZONE_SIZE;
        zone->first_free_node = node1;
        zone->last_free_node = node3;
        zone->next = empty_zone;
        empty_zone->next = nullptr;

        void* mem = TakeMemoryFromZoneList(zone, 48, 64);
        ASSERT_TRUE(mem != nullptr);

        t_memory_node* new_separated_node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone) + 47 + 22 + 48 +
                                                             sizeof(t_memory_node) * 3);
        ASSERT_EQ(new_separated_node->usable_size, 181 - 48 - sizeof(t_memory_node));
        ASSERT_EQ(new_separated_node->available, true);
        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(node1->next, node2);
        ASSERT_EQ(node2->next, new_separated_node);
        ASSERT_EQ(new_separated_node->next, nullptr);
        ASSERT_EQ(zone->last_free_node, new_separated_node);
        t_memory_node* node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone) + SIZE_WITH_NODE_HEADER(47) +
                                               SIZE_WITH_NODE_HEADER(22));
        ASSERT_EQ(CAST_TO_BYTE_APPLY_NODE_SHIFT(node), (BYTE*)mem);
        ASSERT_EQ(node, node3);
        ASSERT_EQ(node->available, false);
        ASSERT_EQ(node->usable_size, 48);
        ASSERT_EQ(node->next, nullptr);
    }
}

TEST(Take_Memory_From_Zone, Main_Check) {
    t_zone* empty_zone = (t_zone*)test_empty_zone;
    empty_zone->first_free_node = nullptr;
    empty_zone->last_free_node = nullptr;
    empty_zone->total_size = 1337;
    empty_zone->available_size = 0;

    {
        /// check first occurrence
        bzero(test_zone, TEST_ZONE_SIZE);

        t_zone* zone = (t_zone*)test_zone;
        zone->available_size = TEST_ZONE_SIZE - sizeof(t_zone);
        zone->total_size = TEST_ZONE_SIZE - sizeof(t_zone);
        zone->first_free_node = nullptr;
        zone->last_free_node = nullptr;
        empty_zone->next = zone;
        zone->next = nullptr;

        void* mem = TakeMemoryFromZoneList(zone, 48, 64);
        ASSERT_TRUE(mem != nullptr);

        t_memory_node* node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        ASSERT_EQ(CAST_TO_BYTE_APPLY_NODE_SHIFT(node), (BYTE*)mem);
        ASSERT_EQ(node->available, false);
        ASSERT_EQ(node->usable_size, 48);
        ASSERT_EQ(node->next, nullptr);
        ASSERT_EQ(zone->available_size, TEST_ZONE_SIZE - sizeof(t_zone) - 48 - sizeof(t_memory_node));
        ASSERT_EQ(zone->total_size, TEST_ZONE_SIZE - sizeof(t_zone));
    }

    {
        /// check with not suitable free nodes
        bzero(test_zone, TEST_ZONE_SIZE);
        t_memory_node* node1 = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone));
        node1->usable_size = 47;
        node1->available = true;

        t_memory_node* node2 = (t_memory_node*)(CAST_TO_BYTE_APPLY_NODE_SHIFT(node1) + node1->usable_size);
        node2->usable_size = 22;
        node2->available = true;
        node1->next = node2;

        t_zone* zone = (t_zone*)test_zone;
        size_t start_zone_size =
                TEST_ZONE_SIZE - sizeof(t_zone) - SIZE_WITH_NODE_HEADER(47) - SIZE_WITH_NODE_HEADER(22);
        zone->available_size = start_zone_size;
        zone->total_size = TEST_ZONE_SIZE - sizeof(t_zone);
        zone->first_free_node = node1;
        zone->last_free_node = node2;
        empty_zone->next = zone;
        zone->next = nullptr;

        void* mem = TakeMemoryFromZoneList(zone, 48, 64);
        ASSERT_TRUE(mem != nullptr);

        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(zone->last_free_node, node2);

        t_memory_node* node = (t_memory_node*)(CAST_TO_BYTE_APPLY_ZONE_SHIFT(test_zone) + SIZE_WITH_NODE_HEADER(47) +
                                               SIZE_WITH_NODE_HEADER(22));
        ASSERT_EQ(CAST_TO_BYTE_APPLY_NODE_SHIFT(node), (BYTE*)mem);
        ASSERT_EQ(node->available, false);
        ASSERT_EQ(node->usable_size, 48);
        ASSERT_EQ(node->next, nullptr);
        ASSERT_EQ(zone->available_size, start_zone_size - SIZE_WITH_NODE_HEADER(48));
        ASSERT_EQ(zone->total_size, TEST_ZONE_SIZE - sizeof(t_zone));
    }
}

TEST(Malloc, Check_Init_Correct) {
    char* mem = (char*)malloc(5);

    ASSERT_EQ(gInit, true);
    ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
    ASSERT_EQ(gMemoryZones.last_large_allocation, nullptr);

    ASSERT_EQ(gMemoryZones.first_tiny_zone->total_size, gPageSize * 4 - sizeof(t_zone));
    ASSERT_EQ(gMemoryZones.first_tiny_zone->available_size, gPageSize * 4 - sizeof(t_zone) - SIZE_WITH_NODE_HEADER(16));
    ASSERT_EQ(gMemoryZones.first_tiny_zone->first_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_tiny_zone->last_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_tiny_zone->next, nullptr);

    ASSERT_EQ(gMemoryZones.last_tiny_zone->total_size, gPageSize * 4 - sizeof(t_zone));
    ASSERT_EQ(gMemoryZones.last_tiny_zone->available_size, gPageSize * 4 - sizeof(t_zone) - SIZE_WITH_NODE_HEADER(16));
    ASSERT_EQ(gMemoryZones.last_tiny_zone->first_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.last_tiny_zone->last_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.last_tiny_zone->next, nullptr);

    ASSERT_EQ(gMemoryZones.first_small_zone->total_size, gPageSize * 16 - sizeof(t_zone));
    ASSERT_EQ(gMemoryZones.first_small_zone->available_size, gPageSize * 16 - sizeof(t_zone));
    ASSERT_EQ(gMemoryZones.first_small_zone->first_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_small_zone->last_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_small_zone->next, nullptr);

    ASSERT_EQ(gMemoryZones.last_small_zone->total_size, gPageSize * 16 - sizeof(t_zone));
    ASSERT_EQ(gMemoryZones.last_small_zone->available_size, gPageSize * 16 - sizeof(t_zone));
    ASSERT_EQ(gMemoryZones.last_small_zone->first_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.last_small_zone->last_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.last_small_zone->next, nullptr);
}

TEST(Malloc, Correct_Zone_Select) {
    free_all();
    {
        /// tiny zone choosing and adding correct
        for (size_t i = 0; i < (gTinyZoneSize - sizeof(t_zone)) / (SIZE_WITH_NODE_HEADER(gTinyAllocationMaxSize)); ++i) {
            void* mem = malloc(gTinyAllocationMaxSize);
        }
        ASSERT_EQ(gMemoryZones.first_tiny_zone->available_size,
                  (gTinyZoneSize - sizeof(t_zone)) % SIZE_WITH_NODE_HEADER(gTinyAllocationMaxSize));
        ASSERT_EQ(gMemoryZones.first_tiny_zone->next, nullptr);
        ASSERT_EQ(gMemoryZones.first_small_zone->available_size, gSmallZoneSize - sizeof(t_zone));
        ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);

        void* mem = malloc(gTinyAllocationMaxSize);
        t_zone* zone = gMemoryZones.last_tiny_zone;

        ASSERT_FALSE(zone == gMemoryZones.first_tiny_zone);
        ASSERT_EQ(gMemoryZones.first_tiny_zone->next, zone);
        ASSERT_EQ(zone->available_size,
                  gTinyZoneSize - sizeof(t_zone) - SIZE_WITH_NODE_HEADER(gTinyAllocationMaxSize));
        ASSERT_EQ(gMemoryZones.first_small_zone->available_size, gSmallZoneSize - sizeof(t_zone));
        ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
    }

    {
        /// small zone choosing and adding correct
        for (size_t i = 0; i < (gSmallZoneSize - sizeof(t_zone)) / (SIZE_WITH_NODE_HEADER(gSmallAllocationMaxSize)); ++i) {
            void* mem = malloc(gSmallAllocationMaxSize);
        }
        ASSERT_EQ(gMemoryZones.first_small_zone->available_size,
                  (gSmallZoneSize - sizeof(t_zone)) % SIZE_WITH_NODE_HEADER(gSmallAllocationMaxSize));
        ASSERT_EQ(gMemoryZones.first_small_zone->next, nullptr);
        ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);

        void* mem = malloc(gSmallAllocationMaxSize);
        t_zone* zone = gMemoryZones.last_small_zone;

        ASSERT_FALSE(zone == gMemoryZones.first_small_zone);
        ASSERT_EQ(gMemoryZones.first_small_zone->next, zone);
        ASSERT_EQ(zone->available_size,
                  gSmallZoneSize - sizeof(t_zone) - SIZE_WITH_NODE_HEADER(gSmallAllocationMaxSize));
        ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
    }

    {
        /// large allocation choosing and adding correct
        size_t mem_size = gSmallAllocationMaxSize + 16;
        void* mem = malloc(mem_size);
        ASSERT_EQ(gMemoryZones.first_large_allocation->available_size, mem_size + gPageSize - mem_size % gPageSize - sizeof(t_zone));
        ASSERT_EQ(gMemoryZones.first_large_allocation->last_free_node->usable_size, mem_size + gPageSize - mem_size % gPageSize - sizeof(t_zone) - sizeof(t_memory_node));

        mem_size = gSmallAllocationMaxSize + gPageSize;
        mem = malloc(mem_size);

        t_zone* zone = gMemoryZones.last_large_allocation;
        ASSERT_FALSE(zone == gMemoryZones.first_large_allocation);
        ASSERT_EQ(gMemoryZones.first_large_allocation->next, zone);
        ASSERT_EQ(zone->available_size, mem_size + gPageSize - mem_size % gPageSize - sizeof(t_zone));
        ASSERT_EQ(zone->first_free_node->usable_size, mem_size + gPageSize - mem_size % gPageSize - sizeof(t_zone) - sizeof(t_memory_node));
    }
}

