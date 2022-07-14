#include "malloc_internal.h"
#include "utilities.h"

BOOL gInit = FALSE;
t_memory_zones gMemoryZones;
int gPageSize;

BOOL init() {
    gInit = TRUE;
    gPageSize = getpagesize();

    t_zone* tiny_default_zone = create_new_zone(calculate_zone_size(Tiny, 0));
    gMemoryZones.first_tiny_zone = tiny_default_zone;
    gMemoryZones.last_tiny_zone = tiny_default_zone;

    t_zone* small_default_zone = create_new_zone(calculate_zone_size(Small, 0));
    gMemoryZones.first_small_zone = small_default_zone;
    gMemoryZones.last_small_zone = small_default_zone;

    if (!tiny_default_zone || !small_default_zone) {
        return FALSE;
    }
    return TRUE;
}

void* __malloc(size_t required_size) {
    if (!gInit) {
        if (!init()) {
            return NULL;
        }
    }

    if (required_size == 0) {
        return NULL;
    }

    /// 16 byte align (MacOS align).
    required_size = required_size + 15 & ~15;
    t_allocation_type allocation_type = to_allocation_type(required_size);

    if (allocation_type == Large) {
        t_zone* large_allocation = create_new_zone(calculate_zone_size(Large, required_size));
        if (!large_allocation) {
            return NULL;
        }

        BYTE* mem_node = (BYTE*)large_allocation + ZONE_HEADER_SIZE;
        construct_large_node_header(mem_node, required_size);
        large_allocation->last_allocated_node = mem_node;

        add_zone_to_list(&gMemoryZones.first_large_allocation, &gMemoryZones.last_large_allocation, large_allocation);
        return (void*)(mem_node + NODE_HEADER_SIZE);
    }

    t_zone** first_zone;
    t_zone** last_zone;
    uint64_t separate_size;
    switch (allocation_type) {
        case Tiny:
            first_zone = &gMemoryZones.first_tiny_zone;
            last_zone = &gMemoryZones.last_tiny_zone;
            separate_size = TINY_SEPARATE_SIZE;
            break;
        case Small:
            first_zone = &gMemoryZones.first_small_zone;
            last_zone = &gMemoryZones.last_small_zone;
            separate_size = SMALL_SEPARATE_SIZE;
            break;
        case Large:
            exit(-1);
    }

    void* memory = take_memory_from_zone_list(*first_zone, required_size, separate_size, allocation_type);
    if (!memory) {
        t_zone* new_zone = create_new_zone(calculate_zone_size(allocation_type, required_size));
        if (!new_zone) {
            return NULL;
        }
        memory = take_memory_from_zone_list(new_zone, required_size, separate_size, allocation_type);
        add_zone_to_list(first_zone, last_zone, new_zone);
    }
    return memory;
}
