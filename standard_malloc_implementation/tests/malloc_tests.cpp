#include <gtest/gtest.h>

extern "C" {
#include "malloc.h"
}

#define TEST_ZONE_SIZE 4096

static t_zone *test_zone = (t_zone*)mmap(0, TEST_ZONE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
                                          VM_MAKE_TAG(VM_MEMORY_MALLOC), 0);

TEST(Split_Node, CheckCorrect) {
    bzero(test_zone, TEST_ZONE_SIZE);
    t_memory_node* node = (t_memory_node*)test_zone;
    node->next = NULL;
    node->available = true;
    node->usable_size = 128;
    t_memory_node* splitted_node = SplitNode(node, 64);
    ASSERT_EQ(splitted_node, node + 64);
    ASSERT_EQ(splitted_node->usable_size, 64 - sizeof(t_memory_node));
    ASSERT_EQ(splitted_node->next, nullptr);
}

TEST(Take_Mem_From_Free_Nodes, Empty_List) {
    bzero(test_zone, TEST_ZONE_SIZE);
    test_zone->next = NULL;
    test_zone->available_size = 0;
    test_zone->total_size = TEST_ZONE_SIZE;
    test_zone->first_free_node = NULL;
    test_zone->last_free_node = NULL;
    void* mem = TakeMemoryFromZoneList(test_zone, 16, 64);
    ASSERT_EQ(mem, nullptr);
}

TEST(Take_Mem_From_Free_Nodes, Main_Check) {
    bzero(test_zone, TEST_ZONE_SIZE);

    t_zone *test_zone_test = (t_zone*)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
                                             VM_MAKE_TAG(VM_MEMORY_MALLOC), 0);
    if (test_zone_test == MAP_FAILED) {
        std::cout << "AAA" << std::endl;
    }

    bzero(test_zone_test, TEST_ZONE_SIZE);
    for (int i = 0; i <= TEST_ZONE_SIZE; i += 1) {
        t_memory_node* node = (t_memory_node*)((char*)test_zone_test + i);
        node->usable_size = 15431;
        node->available = true;
        node->next = (t_memory_node*)0x12351;
    }

    t_memory_node* node1 = (t_memory_node*)APPLY_ZONE_HEADER_SHIFT(test_zone);
    node1->usable_size = 38;
    node1->available = true;

    t_memory_node* node2 = APPLY_NODE_HEADER_SHIFT(node1) + node1->usable_size;
    node2->usable_size = 26;
    node2->available = true;
    node1->next = node2;

    t_memory_node* node3 = APPLY_NODE_HEADER_SHIFT(node2) + node2->usable_size;
    node3->usable_size = 64;
    node3->available = true;
    node2->next = node3;
    node3->next = NULL;

    /// first node will use without separated
    test_zone->next = NULL;
    test_zone->available_size = 0;
    test_zone->total_size = TEST_ZONE_SIZE;
    test_zone->first_free_node = node1;
    test_zone->last_free_node = node3;
    void* mem = TakeMemoryFromZoneList(test_zone, 16, 64);
    ASSERT_TRUE(mem != NULL);
    ASSERT_EQ(test_zone->first_free_node, node2);
    ASSERT_EQ(test_zone->last_free_node, node3);
    t_memory_node* node = (t_memory_node*)APPLY_ZONE_HEADER_SHIFT(test_zone);
    ASSERT_EQ(node->available, false);
    ASSERT_EQ(node->usable_size, 38);
    ASSERT_EQ(node->next, nullptr);
}
