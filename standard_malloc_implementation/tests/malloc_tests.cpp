#include <gtest/gtest.h>

extern "C" {
#include "malloc_internal.h"
#include "utilities.h"
}

#define TEST_ZONE_SIZE 4096

static BYTE* test_zone = (BYTE*)mmap(0, TEST_ZONE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
                                     VM_MAKE_TAG(VM_MEMORY_MALLOC), 0);
static BYTE* test_fully_occupied_zone = (BYTE*)mmap(0, TEST_ZONE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
                                                    VM_MAKE_TAG(VM_MEMORY_MALLOC), 0);

TEST(Split_Node, Check_Correct) {
    bzero(test_zone, TEST_ZONE_SIZE);
    t_zone* zone = (t_zone*)test_zone;
    zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;

    BYTE* node = ((BYTE*)zone + ZONE_HEADER_SIZE + 248);
    construct_node_header(zone, node, 128, 52, TRUE, Tiny);
    add_node_to_free_list(zone, node);

    BYTE* last_free_node = ((BYTE*)zone + ZONE_HEADER_SIZE);
    construct_node_header(zone, last_free_node, 16, 0, TRUE, Tiny);
    add_node_to_free_list(zone, last_free_node);

    size_t first_node_new_size = 48;
    separate_node_on_new_free_node(node, first_node_new_size, zone, Tiny);
    BYTE* new_node = node + NODE_HEADER_SIZE + get_node_size(node, Tiny);

    ASSERT_EQ(get_node_size(node, Tiny), 48);
    ASSERT_EQ(get_prev_node_size(node), 52);
    ASSERT_EQ(get_node_zone_start_offset(node), 248 + ZONE_HEADER_SIZE);
    ASSERT_EQ(zone->last_free_node, new_node);
    ASSERT_EQ(get_node_available(node), TRUE);
    ASSERT_EQ(get_node_allocation_type(node), Tiny);

    ASSERT_EQ(get_node_size(new_node, Tiny), 128 - NODE_HEADER_SIZE - first_node_new_size);
    ASSERT_EQ(get_prev_node_size(new_node), 48);
    ASSERT_EQ(get_node_zone_start_offset(new_node), 248 + ZONE_HEADER_SIZE + NODE_HEADER_SIZE + 48);
    ASSERT_EQ((BYTE*)zone + get_node_zone_start_offset(new_node), new_node);
    ASSERT_EQ(get_prev_free_node_zone_start_offset(new_node), ZONE_HEADER_SIZE);
    ASSERT_EQ(get_prev_free_node(zone, new_node), last_free_node);
    ASSERT_EQ(get_node_available(new_node), TRUE);
    ASSERT_EQ(get_node_allocation_type(new_node), Tiny);
}

TEST(Take_Mem_From_Free_Nodes, Empty_List) {
    t_zone* zone = (t_zone*)test_zone;
    bzero(test_zone, TEST_ZONE_SIZE);
    zone->next = nullptr;
    zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;
    zone->first_free_node = nullptr;
    zone->last_free_node = nullptr;

    BYTE* last_allocated_node = (BYTE*)zone + (zone->total_size + ZONE_HEADER_SIZE) - NODE_HEADER_SIZE - 16;
    set_node_size(last_allocated_node, 16, Tiny);
    set_prev_node_size(last_allocated_node, 16);
    set_node_zone_start_offset(last_allocated_node, zone->total_size - NODE_HEADER_SIZE - 16);
    set_next_free_node(last_allocated_node, 0);
    set_node_available(last_allocated_node, FALSE);
    set_node_allocation_type(last_allocated_node, Tiny);
    zone->last_allocated_node = last_allocated_node; // there is no space for any other node

    void* mem = take_memory_from_zone_list(zone, 16, 64, Small);
    ASSERT_EQ(mem, nullptr);
}

TEST(Take_Mem_From_Free_Nodes, Check_Without_Separated) {
    t_zone* occupied_zone = (t_zone*)test_fully_occupied_zone;
    occupied_zone->first_free_node = nullptr;
    occupied_zone->last_free_node = nullptr;
    occupied_zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;

    BYTE* last_allocated_node = (BYTE*)occupied_zone + (occupied_zone->total_size + ZONE_HEADER_SIZE) - NODE_HEADER_SIZE - 16;
    set_node_size(last_allocated_node, 16, Tiny);
    set_prev_node_size(last_allocated_node, 16);
    set_node_zone_start_offset(last_allocated_node, occupied_zone->total_size - NODE_HEADER_SIZE - 16);
    set_next_free_node(last_allocated_node, 0);
    set_node_available(last_allocated_node, FALSE);
    set_node_allocation_type(last_allocated_node, Tiny);
    occupied_zone->last_allocated_node = last_allocated_node; // there is no space for any other node

    {
        /// check first node
        bzero(test_zone, TEST_ZONE_SIZE);
        t_zone* zone = (t_zone*)test_zone;
        zone->last_allocated_node = (BYTE*)zone + zone->total_size - NODE_HEADER_SIZE + 16; // there is no space for any other node
        zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;
        zone->next = nullptr;
        occupied_zone->next = zone;

        BYTE* node1 = test_zone + ZONE_HEADER_SIZE;
        construct_node_header(zone, node1, 38, 0, TRUE, Tiny);
        add_node_to_free_list(zone, node1);

        BYTE* node2 = node1 + NODE_HEADER_SIZE + get_node_size(node1, Tiny);
        construct_node_header(zone, node2, 26, get_node_size(node1, Tiny),  TRUE, Tiny);
        add_node_to_free_list(zone, node2);

        BYTE* node3 = node2 + NODE_HEADER_SIZE + get_node_size(node2, Tiny);
        construct_node_header(zone, node3, 64, get_node_size(node2, Tiny), TRUE, Tiny);
        add_node_to_free_list(zone, node3);

        void* mem = take_memory_from_zone_list(occupied_zone, 16, 64, Tiny);
        ASSERT_TRUE(mem != nullptr);
        ASSERT_EQ(zone->first_free_node, node2);
        ASSERT_EQ(zone->last_free_node, node3);
        ASSERT_EQ(get_next_free_node(zone, node2), node3);
        ASSERT_EQ(get_next_free_node(zone, node3), nullptr);

        BYTE* node = (BYTE*)zone + ZONE_HEADER_SIZE;
        ASSERT_EQ(node + NODE_HEADER_SIZE, (BYTE*)mem);
        ASSERT_EQ(node, node1);
        ASSERT_EQ(get_node_size(node, Tiny), 38);
        ASSERT_EQ(get_prev_node_size(node), 0);
        ASSERT_EQ(get_node_zone_start_offset(node), ZONE_HEADER_SIZE);
        ASSERT_EQ(get_next_free_node(zone, node), nullptr);
        ASSERT_EQ(get_node_available(node), FALSE);
        ASSERT_EQ(get_node_allocation_type(node), Tiny);
    }

    {
        /// check last node
        bzero(test_zone, TEST_ZONE_SIZE);
        t_zone* zone = (t_zone*)test_zone;
        zone->last_allocated_node = (BYTE*)zone + zone->total_size - NODE_HEADER_SIZE + 16; // there is no space for any other node
        zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;
        zone->next = nullptr;
        occupied_zone->next = zone;

        BYTE* node1 = test_zone + ZONE_HEADER_SIZE;
        construct_node_header(zone, node1, 38, 0, TRUE, Tiny);
        add_node_to_free_list(zone, node1);

        BYTE* node2 = node1 + NODE_HEADER_SIZE + get_node_size(node1, Tiny);
        construct_node_header(zone, node2, 26, get_node_size(node1, Tiny),  TRUE, Tiny);
        add_node_to_free_list(zone, node2);

        BYTE* node3 = node2 + NODE_HEADER_SIZE + get_node_size(node2, Tiny);
        construct_node_header(zone, node3, 111, get_node_size(node2, Tiny), TRUE, Tiny); // in 112 it will be separate
        add_node_to_free_list(zone, node3);

        void* mem = take_memory_from_zone_list(occupied_zone, 48, 64, Tiny);
        ASSERT_TRUE(mem != nullptr);
        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(zone->last_free_node, node2);
        ASSERT_EQ(get_next_free_node(zone, node1), node2);
        ASSERT_EQ(get_next_free_node(zone, node2), nullptr);

        uint64_t node_zone_start_offset = ZONE_HEADER_SIZE + NODE_HEADER_SIZE * 2 + 38 + 26;
        BYTE* node = (BYTE*)zone + node_zone_start_offset;
        ASSERT_EQ(node + NODE_HEADER_SIZE, (BYTE*)mem);
        ASSERT_EQ(node, node3);
        ASSERT_EQ(get_node_size(node, Tiny), 111);
        ASSERT_EQ(get_prev_node_size(node), 26);
        ASSERT_EQ(get_node_zone_start_offset(node), node_zone_start_offset);
        ASSERT_EQ(get_next_free_node(zone, node), nullptr);
        ASSERT_EQ(get_node_available(node), FALSE);
        ASSERT_EQ(get_node_allocation_type(node), Tiny);
    }

    {
        /// check middle node
        bzero(test_zone, TEST_ZONE_SIZE);
        t_zone* zone = (t_zone*)test_zone;
        zone->last_allocated_node = (BYTE*)zone + zone->total_size - NODE_HEADER_SIZE + 16; // there is no space for any other node
        zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;
        zone->next = nullptr;
        occupied_zone->next = zone;

        BYTE* node1 = test_zone + ZONE_HEADER_SIZE;
        construct_node_header(zone, node1, 38, 0, TRUE, Tiny);
        add_node_to_free_list(zone, node1);

        BYTE* node2 = node1 + NODE_HEADER_SIZE + get_node_size(node1, Tiny);
        construct_node_header(zone, node2, 56, get_node_size(node1, Tiny),  TRUE, Tiny);
        add_node_to_free_list(zone, node2);

        BYTE* node3 = node2 + NODE_HEADER_SIZE + get_node_size(node2, Tiny);
        construct_node_header(zone, node3, 64, get_node_size(node2, Tiny), TRUE, Tiny);
        add_node_to_free_list(zone, node3);


        void* mem = take_memory_from_zone_list(occupied_zone, 48, 64, Tiny);
        ASSERT_TRUE(mem != nullptr);
        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(zone->last_free_node, node3);
        ASSERT_EQ(get_next_free_node(zone, node1), node3);
        ASSERT_EQ(get_next_free_node(zone, node3), nullptr);

        uint64_t node_zone_start_offset = ZONE_HEADER_SIZE + NODE_HEADER_SIZE + 38;
        BYTE* node = (BYTE*)zone + node_zone_start_offset;
        ASSERT_EQ(node + NODE_HEADER_SIZE, (BYTE*)mem);
        ASSERT_EQ(node, node2);
        ASSERT_EQ(get_node_size(node, Tiny), 56);
        ASSERT_EQ(get_prev_node_size(node), 38);
        ASSERT_EQ(get_node_zone_start_offset(node), node_zone_start_offset);
        ASSERT_EQ(get_next_free_node(zone, node), nullptr);
        ASSERT_EQ(get_node_available(node), FALSE);
        ASSERT_EQ(get_node_allocation_type(node), Tiny);
    }
}

TEST(Take_Mem_From_Free_Nodes, Check_With_Separated) {
    t_zone* occupied_zone = (t_zone*)test_fully_occupied_zone;
    occupied_zone->first_free_node = nullptr;
    occupied_zone->last_free_node = nullptr;
    occupied_zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;

    BYTE* last_allocated_node =
    (BYTE*)occupied_zone + (occupied_zone->total_size + ZONE_HEADER_SIZE) - NODE_HEADER_SIZE - 16;
    set_node_size(last_allocated_node, 16, Tiny);
    set_prev_node_size(last_allocated_node, 16);
    set_node_zone_start_offset(last_allocated_node, occupied_zone->total_size - NODE_HEADER_SIZE - 16);
    set_next_free_node(last_allocated_node, nullptr);
    set_node_available(last_allocated_node, FALSE);
    set_node_allocation_type(last_allocated_node, Tiny);
    occupied_zone->last_allocated_node = last_allocated_node; // there is no space for any other node

    {
        /// check first node
        bzero(test_zone, TEST_ZONE_SIZE);
        t_zone* zone = (t_zone*)test_zone;
        zone->last_allocated_node = (BYTE*)zone + zone->total_size - NODE_HEADER_SIZE + 16; // there is no space for any other node
        zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;
        zone->first_free_node = NULL;
        zone->last_free_node = NULL;
        zone->next = nullptr;
        occupied_zone->next = zone;

        BYTE* node1 = test_zone + ZONE_HEADER_SIZE;
        construct_node_header(zone, node1, 136, 0, TRUE, Tiny);
        add_node_to_free_list(zone, node1);

        BYTE* node2 = node1 + NODE_HEADER_SIZE + get_node_size(node1, Tiny);
        construct_node_header(zone, node2, 162, get_node_size(node1, Tiny), TRUE, Tiny);
        add_node_to_free_list(zone, node2);

        BYTE* node3 = node2 + NODE_HEADER_SIZE + get_node_size(node2, Tiny);
        construct_node_header(zone, node3, 181, get_node_size(node2, Tiny), TRUE, Tiny);
        add_node_to_free_list(zone, node3);

        void* mem = take_memory_from_zone_list(occupied_zone, 48, 64, Tiny);
        ASSERT_TRUE(mem != nullptr);

        BYTE* new_separated_node = (BYTE*)zone + ZONE_HEADER_SIZE + NODE_HEADER_SIZE + 48;
        ASSERT_EQ(get_node_size(new_separated_node, Tiny), 136 - 48 - NODE_HEADER_SIZE);
        ASSERT_EQ(get_prev_node_size(new_separated_node), 48);
        ASSERT_EQ(get_node_zone_start_offset(new_separated_node), ZONE_HEADER_SIZE + NODE_HEADER_SIZE + 48);
        ASSERT_EQ(get_node_available(new_separated_node), TRUE);
        ASSERT_EQ(get_node_allocation_type(new_separated_node), Tiny);

        ASSERT_EQ(zone->first_free_node, node2);
        ASSERT_EQ(get_next_free_node(zone, node2), node3);
        ASSERT_EQ(get_next_free_node(zone, node3), new_separated_node);
        ASSERT_EQ(zone->last_free_node, new_separated_node);

        BYTE* node = (BYTE*)zone + ZONE_HEADER_SIZE;
        ASSERT_EQ(node + NODE_HEADER_SIZE, (BYTE*)mem);
        ASSERT_EQ(node, node1);
        ASSERT_EQ(get_node_size(node, Tiny), 48);
        ASSERT_EQ(get_prev_node_size(node), 0);
        ASSERT_EQ(get_node_zone_start_offset(node), ZONE_HEADER_SIZE);
        ASSERT_EQ(get_node_available(node), FALSE);
        ASSERT_EQ(get_node_allocation_type(node), Tiny);
    }

    {
        t_zone* zone = (t_zone*)test_zone;
        zone->last_allocated_node = (BYTE*)zone + zone->total_size - NODE_HEADER_SIZE + 16; // there is no space for any other node
        zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;
        zone->first_free_node = NULL;
        zone->last_free_node = NULL;
        zone->next = nullptr;
        occupied_zone->next = zone;

        /// check middle node
        BYTE* node1 = test_zone + ZONE_HEADER_SIZE;
        construct_node_header(zone, node1, 47, 0, TRUE, Tiny);
        add_node_to_free_list(zone, node1);

        BYTE* node2 = node1 + NODE_HEADER_SIZE + get_node_size(node1, Tiny);
        construct_node_header(zone, node2, 162, get_node_size(node1, Tiny), TRUE, Tiny);
        add_node_to_free_list(zone, node2);

        BYTE* node3 = node2 + NODE_HEADER_SIZE + get_node_size(node2, Tiny);
        construct_node_header(zone, node3, 181, get_node_size(node2, Tiny), TRUE, Tiny);
        add_node_to_free_list(zone, node3);

        void* mem = take_memory_from_zone_list(occupied_zone, 48, 64, Tiny);
        ASSERT_TRUE(mem != nullptr);

        uint64_t new_separated_node_zone_offset = ZONE_HEADER_SIZE + NODE_HEADER_SIZE * 2 + 47 + 48;
        BYTE* new_separated_node = (BYTE*)zone + new_separated_node_zone_offset;
        ASSERT_EQ(get_node_size(new_separated_node, Tiny), 162 - 48 - NODE_HEADER_SIZE);
        ASSERT_EQ(get_prev_node_size(new_separated_node), 48);
        ASSERT_EQ(get_node_zone_start_offset(new_separated_node), new_separated_node_zone_offset);
        ASSERT_EQ(get_node_available(new_separated_node), TRUE);
        ASSERT_EQ(get_node_allocation_type(new_separated_node), Tiny);

        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(zone->last_free_node, new_separated_node);

        ASSERT_EQ(get_next_free_node(zone, node1), node3);
        ASSERT_EQ(get_next_free_node(zone, node3), new_separated_node);
        ASSERT_EQ(get_next_free_node(zone, new_separated_node), nullptr);

        ASSERT_EQ(get_prev_free_node(zone, new_separated_node), node3);
        ASSERT_EQ(get_prev_free_node(zone, node3), node1);
        ASSERT_EQ(get_prev_free_node(zone, node1), nullptr);

        uint64_t node_zone_offset = ZONE_HEADER_SIZE + NODE_HEADER_SIZE + 47;
        BYTE* node = (BYTE*)zone + node_zone_offset;
        ASSERT_EQ(node + NODE_HEADER_SIZE, (BYTE*)mem);
        ASSERT_EQ(node, node2);
        ASSERT_EQ(get_node_size(node, Tiny), 48);
        ASSERT_EQ(get_prev_node_size(node), 47);
        ASSERT_EQ(get_node_zone_start_offset(node), node_zone_offset);
        ASSERT_EQ(get_next_free_node(zone, node), nullptr);
        ASSERT_EQ(get_node_available(node), FALSE);
        ASSERT_EQ(get_node_allocation_type(node), Tiny);
    }

    {
        /// check last node
        t_zone* zone = (t_zone*)test_zone;
        zone->last_allocated_node = (BYTE*)zone + zone->total_size - NODE_HEADER_SIZE + 16; // there is no space for any other node
        zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;
        zone->first_free_node = nullptr;
        zone->last_free_node = nullptr;
        zone->next = nullptr;
        occupied_zone->next = zone;

        BYTE* node1 = test_zone + ZONE_HEADER_SIZE;
        construct_node_header(zone, node1, 47, 0, TRUE, Tiny);
        add_node_to_free_list(zone, node1);

        BYTE* node2 = node1 + NODE_HEADER_SIZE + get_node_size(node1, Tiny);
        construct_node_header(zone, node2, 22, get_node_size(node1, Tiny), TRUE, Tiny);
        add_node_to_free_list(zone, node2);

        BYTE* node3 = node2 + NODE_HEADER_SIZE + get_node_size(node2, Tiny);
        construct_node_header(zone, node3, 181, get_node_size(node2, Tiny), TRUE, Tiny);
        add_node_to_free_list(zone, node3);

        void* mem = take_memory_from_zone_list(occupied_zone, 48, 64, Tiny);
        ASSERT_TRUE(mem != nullptr);

        uint64_t new_separated_node_zone_offset = ZONE_HEADER_SIZE + NODE_HEADER_SIZE * 3 + 47 + 22 + 48;
        BYTE* new_separated_node = (BYTE*)zone + new_separated_node_zone_offset;
        ASSERT_EQ(get_node_size(new_separated_node, Tiny), 181 - 48 - NODE_HEADER_SIZE);
        ASSERT_EQ(get_prev_node_size(new_separated_node), 48);
        ASSERT_EQ(get_node_zone_start_offset(new_separated_node), new_separated_node_zone_offset);
        ASSERT_EQ(get_node_available(new_separated_node), TRUE);
        ASSERT_EQ(get_node_allocation_type(new_separated_node), Tiny);

        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(zone->last_free_node, new_separated_node);

        ASSERT_EQ(get_next_free_node(zone, node1), node2);
        ASSERT_EQ(get_next_free_node(zone, node2), new_separated_node);
        ASSERT_EQ(get_next_free_node(zone, new_separated_node), nullptr);

        uint64_t node_zone_offset = ZONE_HEADER_SIZE + NODE_HEADER_SIZE * 2 + 47 + 22;
        BYTE* node = (BYTE*)zone + node_zone_offset;
        ASSERT_EQ(node + NODE_HEADER_SIZE, (BYTE*)mem);
        ASSERT_EQ(node, node3);
        ASSERT_EQ(get_node_size(node, Tiny), 48);
        ASSERT_EQ(get_prev_node_size(node), 22);
        ASSERT_EQ(get_node_zone_start_offset(node), node_zone_offset);
        ASSERT_EQ(get_next_free_node(zone, node), nullptr);
        ASSERT_EQ(get_node_available(node), FALSE);
        ASSERT_EQ(get_node_allocation_type(node), Tiny);
    }
}

TEST(Take_Memory_From_Zone, Main_Check) {
    t_zone* occupied_zone = (t_zone*)test_fully_occupied_zone;
    occupied_zone->first_free_node = nullptr;
    occupied_zone->last_free_node = nullptr;
    occupied_zone->total_size = TEST_ZONE_SIZE - ZONE_HEADER_SIZE;

    BYTE* last_allocated_node =
    (BYTE*)occupied_zone + (occupied_zone->total_size + ZONE_HEADER_SIZE) - NODE_HEADER_SIZE - 16;
    construct_node_header(occupied_zone, last_allocated_node, 16, 16, FALSE, Tiny);
    occupied_zone->last_allocated_node = last_allocated_node; // there is no space for any other node

    {
        /// check first occurrence
        bzero(test_zone, TEST_ZONE_SIZE);

        t_zone* zone = (t_zone*)test_zone;
        zone->last_allocated_node = nullptr; // zone is completely free
        zone->total_size = TEST_ZONE_SIZE - sizeof(t_zone);
        zone->first_free_node = nullptr;
        zone->last_free_node = nullptr;
        occupied_zone->next = zone;
        zone->next = nullptr;

        void* mem = take_memory_from_zone_list(occupied_zone, 48, 64, Tiny);
        ASSERT_TRUE(mem != nullptr);

        BYTE* node = (BYTE*)zone + ZONE_HEADER_SIZE;
        ASSERT_EQ(node + NODE_HEADER_SIZE, (BYTE*)mem);

        ASSERT_EQ(get_node_size(node, Tiny), 48);
        ASSERT_EQ(get_prev_node_size(node), 0);
        ASSERT_EQ(get_node_zone_start_offset(node), ZONE_HEADER_SIZE);
        ASSERT_EQ(get_next_free_node(zone, node), nullptr);
        ASSERT_EQ(get_node_available(node), FALSE);
        ASSERT_EQ(get_node_allocation_type(node), Tiny);

        ASSERT_EQ(zone->last_allocated_node, node);
        ASSERT_EQ(zone->total_size, TEST_ZONE_SIZE - sizeof(t_zone));
    }

    {
        /// check with not suitable free nodes
        bzero(test_zone, TEST_ZONE_SIZE);

        BYTE* node1 = test_zone + ZONE_HEADER_SIZE;
        set_node_size(node1, 16, Tiny);
        set_prev_node_size(node1, 0);
        set_node_zone_start_offset(node1, ZONE_HEADER_SIZE);
        set_node_available(node1, TRUE);
        set_node_allocation_type(node1, Tiny);

        BYTE* node2 = node1 + NODE_HEADER_SIZE + get_node_size(node1, Tiny);
        set_node_size(node2, 32, Tiny);
        set_prev_node_size(node2, get_node_size(node1, Tiny));
        set_node_zone_start_offset(node2, get_node_zone_start_offset(node1) + NODE_HEADER_SIZE + get_node_size(node1, Tiny));
        set_node_available(node2, TRUE);
        set_node_allocation_type(node2, Tiny);
        set_next_free_node(node1, node2);

        t_zone* zone = (t_zone*)test_zone;
        zone->last_allocated_node = node2; // zone is completely free
        zone->total_size = TEST_ZONE_SIZE - sizeof(t_zone);
        zone->first_free_node = node1;
        zone->last_free_node = node2;
        occupied_zone->next = zone;
        zone->next = nullptr;

        void* mem = take_memory_from_zone_list(occupied_zone, 48, 64, Tiny);
        ASSERT_TRUE(mem != nullptr);

        uint64_t node_zone_start_offset = ZONE_HEADER_SIZE + NODE_HEADER_SIZE * 2 + 16 + 32;
        BYTE* node = (BYTE*)zone + node_zone_start_offset;
        ASSERT_EQ(node + NODE_HEADER_SIZE, (BYTE*)mem);

        ASSERT_EQ(zone->first_free_node, node1);
        ASSERT_EQ(zone->last_free_node, node2);

        ASSERT_EQ(get_node_size(node, Tiny), 48);
        ASSERT_EQ(get_prev_node_size(node), 32);
        ASSERT_EQ(get_node_zone_start_offset(node), node_zone_start_offset);
        ASSERT_EQ(get_next_free_node(zone, node), nullptr);
        ASSERT_EQ(get_node_available(node), FALSE);
        ASSERT_EQ(get_node_allocation_type(node), Tiny);

        ASSERT_EQ(zone->last_allocated_node, node);
        ASSERT_EQ(zone->total_size, TEST_ZONE_SIZE - sizeof(t_zone));
    }
}

TEST(Malloc_Internal_State, Check_Init_Correct) {
    char* mem = (char*)__malloc(5);

    ASSERT_EQ(gInit, true);
    ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
    ASSERT_EQ(gMemoryZones.last_large_allocation, nullptr);

    BYTE* last_allocated_node = (BYTE*)mem - NODE_HEADER_SIZE;
    ASSERT_EQ(gMemoryZones.first_tiny_zone->total_size, TINY_ZONE_SIZE - ZONE_HEADER_SIZE);
    ASSERT_EQ(last_allocated_node, gMemoryZones.first_tiny_zone->last_allocated_node);
    ASSERT_EQ(get_node_size(last_allocated_node, Tiny), 16);
    ASSERT_EQ(gMemoryZones.first_tiny_zone->first_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_tiny_zone->last_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_tiny_zone->next, nullptr);

    ASSERT_EQ(gMemoryZones.first_tiny_zone, gMemoryZones.last_tiny_zone);

    ASSERT_EQ(gMemoryZones.first_small_zone->total_size, SMALL_ZONE_SIZE - ZONE_HEADER_SIZE);
    ASSERT_EQ(gMemoryZones.first_small_zone->last_allocated_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_small_zone->first_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_small_zone->last_free_node, nullptr);
    ASSERT_EQ(gMemoryZones.first_small_zone->next, nullptr);

    ASSERT_EQ(gMemoryZones.first_small_zone, gMemoryZones.last_small_zone);

}

TEST(Malloc_Internal_State, Correct_Zone_Select) {
    free_all();
    init(); // needed for global vars which use getpagesize();
    {
        void* mem = nullptr;
        /// tiny zone choosing and adding correct
        for (size_t i = 0; i < (TINY_ZONE_SIZE - ZONE_HEADER_SIZE) / (TINE_ALLOCATION_MAX_SIZE + NODE_HEADER_SIZE); ++i) {
            mem = __malloc(TINE_ALLOCATION_MAX_SIZE);
        }
        BYTE* last_allocated_node = (BYTE*)mem - NODE_HEADER_SIZE;
        t_zone* first_tiny_zone = gMemoryZones.first_tiny_zone;

        ASSERT_EQ(first_tiny_zone->last_allocated_node, last_allocated_node);
        uint64_t first_zone_occupied_size = (last_allocated_node + NODE_HEADER_SIZE + get_node_size(last_allocated_node, Tiny)) - (BYTE*)first_tiny_zone - ZONE_HEADER_SIZE;
        uint64_t first_zone_available_size = first_tiny_zone->total_size - first_zone_occupied_size;
        ASSERT_EQ(first_zone_available_size,
                  (TINY_ZONE_SIZE - sizeof(t_zone)) % (TINE_ALLOCATION_MAX_SIZE + NODE_HEADER_SIZE));
        ASSERT_EQ(first_tiny_zone->next, nullptr);
        ASSERT_EQ(gMemoryZones.first_small_zone->last_allocated_node, nullptr);
        ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);

        mem = __malloc(TINE_ALLOCATION_MAX_SIZE);
        t_zone* second_tiny_zone = gMemoryZones.last_tiny_zone;

        last_allocated_node = (BYTE*)mem - NODE_HEADER_SIZE;

        uint64_t second_zone_occupied_size = (last_allocated_node + NODE_HEADER_SIZE + get_node_size(last_allocated_node, Tiny)) - (BYTE*)second_tiny_zone - ZONE_HEADER_SIZE;
        uint64_t second_zone_available_size = second_tiny_zone->total_size - second_zone_occupied_size;

        ASSERT_FALSE(first_tiny_zone == second_tiny_zone);
        ASSERT_EQ(second_tiny_zone->last_allocated_node, last_allocated_node);
        ASSERT_EQ(first_tiny_zone->next, second_tiny_zone);
        ASSERT_EQ(second_zone_available_size,
                  TINY_ZONE_SIZE - ZONE_HEADER_SIZE - NODE_HEADER_SIZE - TINE_ALLOCATION_MAX_SIZE);
        ASSERT_EQ(gMemoryZones.first_small_zone->last_allocated_node, nullptr);
        ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
    }

    {
        /// small zone choosing and adding correct
        void *mem = nullptr;
        for (size_t i = 0; i < (SMALL_ZONE_SIZE - sizeof(t_zone)) / (SMALL_ALLOCATION_MAX_SIZE + NODE_HEADER_SIZE); ++i) {
            mem = __malloc(SMALL_ALLOCATION_MAX_SIZE);
        }
        BYTE* last_allocated_node = (BYTE*)mem - NODE_HEADER_SIZE;
        t_zone* first_small_zone = gMemoryZones.first_small_zone;

        ASSERT_EQ(first_small_zone->last_allocated_node, last_allocated_node);
        uint64_t first_zone_occupied_size = (last_allocated_node + NODE_HEADER_SIZE + get_node_size(last_allocated_node, Small)) - (BYTE*)first_small_zone - ZONE_HEADER_SIZE;
        uint64_t first_zone_available_size = first_small_zone->total_size - first_zone_occupied_size;
        ASSERT_EQ(first_zone_available_size,
                  (SMALL_ZONE_SIZE - ZONE_HEADER_SIZE) % (SMALL_ALLOCATION_MAX_SIZE + NODE_HEADER_SIZE));
        ASSERT_EQ(first_small_zone->next, nullptr);
        ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);

        mem = __malloc(SMALL_ALLOCATION_MAX_SIZE);
        last_allocated_node = (BYTE*)mem - NODE_HEADER_SIZE;
        t_zone* second_small_zone = gMemoryZones.last_small_zone;
        uint64_t second_zone_occupied_size = (last_allocated_node + NODE_HEADER_SIZE + get_node_size(last_allocated_node, Small)) - (BYTE*)second_small_zone - ZONE_HEADER_SIZE;
        uint64_t second_zone_available_size = second_small_zone->total_size - second_zone_occupied_size;

        ASSERT_FALSE(first_small_zone == second_small_zone);
        ASSERT_EQ(first_small_zone->next, second_small_zone);
        ASSERT_EQ(second_zone_available_size,
                  SMALL_ZONE_SIZE - ZONE_HEADER_SIZE - SMALL_ALLOCATION_MAX_SIZE - NODE_HEADER_SIZE);
        ASSERT_EQ(gMemoryZones.first_large_allocation, nullptr);
    }

    {
        /// large allocation choosing and adding correct
        size_t mem_size = 2048 + 15 & ~15;
        void* mem = __malloc(mem_size);

        t_zone* first_large_allocation = gMemoryZones.first_large_allocation;
        BYTE* allocated_node = (BYTE*)mem - NODE_HEADER_SIZE;

        ASSERT_EQ(first_large_allocation->total_size, mem_size + gPageSize - mem_size % gPageSize - ZONE_HEADER_SIZE);
        ASSERT_EQ(allocated_node, first_large_allocation->last_allocated_node);
        ASSERT_EQ(get_node_size(allocated_node, Large), 2048);
        ASSERT_EQ(get_node_allocation_type(allocated_node), Large);

        mem_size = SMALL_ALLOCATION_MAX_SIZE + gPageSize;
        mem = __malloc(mem_size);

        t_zone* second_large_allocation = gMemoryZones.last_large_allocation;
        allocated_node = (BYTE*)mem - NODE_HEADER_SIZE;

        ASSERT_FALSE(first_large_allocation == second_large_allocation);
        ASSERT_EQ(first_large_allocation->next, second_large_allocation);
        ASSERT_EQ(allocated_node, second_large_allocation->last_allocated_node);
        ASSERT_EQ(second_large_allocation->total_size, mem_size + gPageSize - mem_size % gPageSize - ZONE_HEADER_SIZE);
        ASSERT_EQ(get_node_size(allocated_node, Large), mem_size);
        ASSERT_EQ(get_node_allocation_type(allocated_node), Large);
    }
}

