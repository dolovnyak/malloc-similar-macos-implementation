#include "malloc_internal.h"
#include "utilities.h"

BOOL gInit = FALSE;
t_memory_zones gMemoryZones;
uint64_t gPageSize;

uint64_t gTinyZoneSize;
uint64_t gTinyAllocationMaxSize;

uint64_t gSmallZoneSize;
uint64_t gSmallAllocationMaxSize;

static inline uint64_t calculate_zone_size(t_allocation_type type, uint64_t size) {
    switch (type) {
        case Tiny:
            return gTinyZoneSize;
        case Small:
            return gSmallZoneSize;
        case Large:
            size += ZONE_HEADER_SIZE + NODE_HEADER_SIZE;
            return size + gPageSize - size % gPageSize;
    }
}

static inline t_zone* create_new_zone(size_t size) {
    t_zone* new_zone = (t_zone*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
                                     VM_MAKE_TAG(VM_MEMORY_MALLOC), 0);
    if ((void*)new_zone == MAP_FAILED) {
        return NULL;
    }
    new_zone->last_allocated_node = NULL;
    new_zone->total_size = size - ZONE_HEADER_SIZE;
    new_zone->first_free_node = NULL;
    new_zone->last_free_node = NULL;
    new_zone->prev = NULL;
    new_zone->next = NULL;

    return new_zone;
}

BOOL init() {
    gInit = TRUE;
    gPageSize = getpagesize();

    gTinyZoneSize = gPageSize * 4;
    gTinyAllocationMaxSize = gTinyZoneSize / 64;
    t_zone* tiny_default_zone = create_new_zone(calculate_zone_size(Tiny, 0));
    gMemoryZones.first_tiny_zone = tiny_default_zone;
    gMemoryZones.last_tiny_zone = tiny_default_zone;

    gSmallZoneSize = gPageSize * 16;
    gSmallAllocationMaxSize = gSmallZoneSize / 64;
    t_zone* small_default_zone = create_new_zone(calculate_zone_size(Small, 0));
    gMemoryZones.first_small_zone = small_default_zone;
    gMemoryZones.last_small_zone = small_default_zone;

    if (!tiny_default_zone || !small_default_zone) {
        return FALSE;
    }
    return TRUE;
}

void* malloc(size_t required_size) {
    if (required_size == 0) {
        return NULL;
    }

    if (!gInit) {
        if (!init()) {
            return NULL;
        }
    }

    /// 16 byte align (MacOS align).
    required_size = required_size + 15 & ~15;
    t_allocation_type allocation_type = to_type(required_size);

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
    size_t required_size_to_separate;
    switch (allocation_type) {
        case Tiny:
            first_zone = &gMemoryZones.first_tiny_zone;
            last_zone = &gMemoryZones.last_tiny_zone;
            required_size_to_separate = gTinyAllocationMaxSize / 2 + NODE_HEADER_SIZE;
            break;
        case Small:
            first_zone = &gMemoryZones.first_small_zone;
            last_zone = &gMemoryZones.last_small_zone;
            required_size_to_separate = gSmallAllocationMaxSize / 2 + NODE_HEADER_SIZE;
            break;
        case Large:
            exit(-1);
    }

    void* memory = take_memory_from_zone_list(*first_zone, required_size, required_size_to_separate, allocation_type);
    if (!memory) {
        t_zone* new_zone = create_new_zone(calculate_zone_size(allocation_type, required_size));
        if (!new_zone) {
            return NULL;
        }
        memory = take_memory_from_zone_list(new_zone, required_size, required_size_to_separate, allocation_type);
        add_zone_to_list(first_zone, last_zone, new_zone);
    }
    return memory;
}
