#include "malloc_internal.h"
#include "utilities.h"
#include <stdio.h>

static inline void print_node_mem(t_node_representation *node_representation, uint32_t j) {
    node_representation->available ?
    printf("MEM NODE %d : %p - %p : %llu : FREE\n", j,
           node_representation->raw_node + NODE_HEADER_SIZE,
           node_representation->raw_node + NODE_HEADER_SIZE + node_representation->size,
           node_representation->size) :
    printf("MEM NODE %d : %p - %p : %llu : OCCUPIED\n", j,
           node_representation->raw_node + NODE_HEADER_SIZE,
           node_representation->raw_node + NODE_HEADER_SIZE + node_representation->size,
           node_representation->size);
}

static inline void print_zone_mem(t_zone *zone, uint64_t *total_by_user) {
    uint32_t j = 0;
    t_node_representation node_representation;
    for (node_representation = get_node_representation((BYTE *) zone + ZONE_HEADER_SIZE);
         node_representation.next_node != NULL;
         node_representation = get_node_representation(node_representation.next_node)) {
        print_node_mem(&node_representation, j);
        ++j;

        *total_by_user += node_representation.size;
    }
    print_node_mem(&node_representation, j);
    *total_by_user += node_representation.size;
}

void __print_alloc_mem() {
    uint32_t i;
    uint64_t total_for_user = 0;
    uint64_t total_by_fact = 0;

    printf("--------------------------------------------\n");
    i = 0;
    for (t_zone *zone = gMemoryZones.first_tiny_zone; zone != NULL; zone = zone->next) {
        total_by_fact += zone->total_size + ZONE_HEADER_SIZE;

        printf("TINY ZONE %d : %p : %llu\n", i, zone, zone->total_size);
        ++i;
        if (zone->last_allocated_node == NULL) {
            printf("\n");
            continue;
        }
        print_zone_mem(zone, &total_for_user);
        printf("\n");
    }

    i = 0;
    for (t_zone *zone = gMemoryZones.first_small_zone; zone != NULL; zone = zone->next) {
        total_by_fact += zone->total_size + ZONE_HEADER_SIZE;

        printf("SMALL ZONE %d : %p : %llu\n", i, zone, zone->total_size);
        ++i;
        if (zone->last_allocated_node == NULL) {
            printf("\n");
            continue;
        }
        print_zone_mem(zone, &total_for_user);
        printf("\n");
    }

    i = 0;
    for (t_zone *zone = gMemoryZones.first_large_allocation; zone != NULL; zone = zone->next) {
        printf("LARGE ZONE %d : %p : %llu\n", i, zone, zone->total_size);
        ++i;
        t_large_node_representation large_node = get_large_node_representation((BYTE *) zone + ZONE_HEADER_SIZE);
        printf("MEM NODE : %p : %llu\n", large_node.raw_node + NODE_HEADER_SIZE, large_node.size);
        printf("\n");

        total_for_user += large_node.size;
        total_by_fact += zone->total_size + ZONE_HEADER_SIZE;
    }

    printf("TOTAL FOR USER : %llu\n", total_for_user);
    printf("TOTAL BY FACT  : %llu\n", total_by_fact);
    printf("--------------------------------------------\n");
}

static inline void print_hex_dump(const BYTE *data, size_t size) {
    uint8_t ascii[17];
    size_t i, j;
    ascii[16] = '\0';

    for (i = 0; i < size; ++i) {
        if (i % 16 == 0) {
            printf("%p | ", &data[i]);
        }
        printf("%02X ", data[i]);
        if (data[i] >= ' ' && data[i] <= '~') {
            ascii[i % 16] = data[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size) {
            printf(" ");
            if ((i + 1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i + 1 == size) {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}

static inline void print_hex_dump_zone_nodes(t_zone *zone) {
    uint32_t j = 0;
    t_node_representation node_representation;
    for (node_representation = get_node_representation((BYTE *) zone + ZONE_HEADER_SIZE);
         node_representation.next_node != NULL;
         node_representation = get_node_representation(node_representation.next_node)) {
        printf("MEM NODE %d:\n", j);
        print_hex_dump(node_representation.raw_node + NODE_HEADER_SIZE, node_representation.size);
        ++j;
    }
    printf("MEM NODE %d:\n", j);
    print_hex_dump(node_representation.raw_node + NODE_HEADER_SIZE, node_representation.size);
}

void __print_alloc_mem_hex_dump() {
    uint32_t i;

    printf("--------------------------------------------\n");
    i = 0;
    for (t_zone *zone = gMemoryZones.first_tiny_zone; zone != NULL; zone = zone->next) {
        printf("TINY ZONE %d:\n", i);
        ++i;
        if (zone->last_allocated_node == NULL) {
            continue;
        }
        print_hex_dump_zone_nodes(zone);
        printf("\n");
    }

    i = 0;
    for (t_zone *zone = gMemoryZones.first_small_zone; zone != NULL; zone = zone->next) {
        printf("SMALL ZONE %d:\n", i);
        ++i;
        if (zone->last_allocated_node == NULL) {
            continue;
        }
        print_hex_dump_zone_nodes(zone);
        printf("\n");
    }

    i = 0;
    for (t_zone *zone = gMemoryZones.first_large_allocation; zone != NULL; zone = zone->next) {
        printf("LARGE ALLOCATION %d:\n", i);
        ++i;
        t_large_node_representation large_node = get_large_node_representation((BYTE *) zone + ZONE_HEADER_SIZE);
        print_hex_dump(large_node.raw_node + NODE_HEADER_SIZE, large_node.size);
        printf("\n");
    }
    printf("--------------------------------------------\n");
}