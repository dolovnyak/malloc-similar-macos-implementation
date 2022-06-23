#include "malloc_internal.h"

bool gInit = false;
t_memory_zones gMemoryZones;
size_t gPageSize;

size_t gTinyZoneSize;
size_t gTinyAllocationMaxSize;

size_t gSmallZoneSize;
size_t gSmallAllocationMaxSize;

static inline t_zone* CreateNewZone(size_t size) {
    t_zone* new_zone = (t_zone*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
                                     VM_MAKE_TAG(VM_MEMORY_MALLOC), 0);
    if ((void*)new_zone == MAP_FAILED) {
        return NULL;
    }
    new_zone->available_size = size - sizeof(t_zone);
    new_zone->total_size = size - sizeof(t_zone);
    new_zone->first_free_node = NULL;
    new_zone->last_free_node = NULL;
    new_zone->next = NULL;

    return new_zone;
}

static inline bool Init() {
    gInit = true;
    gPageSize = getpagesize();

    gTinyZoneSize = gPageSize * 4;
    gTinyAllocationMaxSize = gTinyZoneSize / 64;
    t_zone* tiny_default_zone = CreateNewZone(CalculateZoneSize(Tiny, 0));
    gMemoryZones.first_tiny_zone = tiny_default_zone;
    gMemoryZones.last_tiny_zone = tiny_default_zone;

    gSmallZoneSize = gPageSize * 16;
    gSmallAllocationMaxSize = gSmallZoneSize / 64;
    t_zone* small_default_zone = CreateNewZone(CalculateZoneSize(Small, 0));
    gMemoryZones.first_small_zone = small_default_zone;
    gMemoryZones.last_small_zone = small_default_zone;

    if (!tiny_default_zone || !small_default_zone) {
        return false;
    }
    return true;
}

void* malloc(size_t required_size) {
    if (required_size == 0) {
        return NULL;
    }

    if (!gInit) {
        if (!Init()) {
            return NULL;
        }
    }

    /// 16 byte align (MacOS align).
    required_size = required_size + 15 & ~15;
    t_allocation_type allocation_type = ToType(required_size);

    if (allocation_type == Large) {
        t_zone* large_allocation = CreateNewZone(CalculateZoneSize(Large, required_size));
        if (!large_allocation) {
            return NULL;
        }
        if (!gMemoryZones.first_large_allocation) {
            gMemoryZones.first_large_allocation = large_allocation;
            gMemoryZones.last_large_allocation = large_allocation;
        } else {
            gMemoryZones.last_large_allocation->next = large_allocation;
            gMemoryZones.last_large_allocation = large_allocation;
        }
        return (void*)CAST_TO_BYTE_APPLY_ZONE_SHIFT(large_allocation);
    }

    t_zone* first_zone;
    t_zone** last_zone;
    size_t required_size_to_separate;
    switch (allocation_type) {
        case Tiny:
            first_zone = gMemoryZones.first_tiny_zone;
            last_zone = &gMemoryZones.last_tiny_zone;
            required_size_to_separate = SIZE_WITH_NODE_HEADER(gTinyAllocationMaxSize / 2);
            break;
        case Small:
            first_zone = gMemoryZones.first_small_zone;
            last_zone = &gMemoryZones.last_small_zone;
            required_size_to_separate = SIZE_WITH_NODE_HEADER(gSmallAllocationMaxSize / 2);
            break;
        case Large:
            exit(-1);
    }

    void* memory = TakeMemoryFromZoneList(first_zone, required_size, required_size_to_separate);
    if (!memory) {
        t_zone* new_zone = CreateNewZone(CalculateZoneSize(allocation_type, required_size));
        if (!new_zone) {
            return NULL;
        }
        (*last_zone)->next = new_zone;
        *last_zone = new_zone;
        memory = TakeMemoryFromZoneList(new_zone, required_size, required_size_to_separate);
    }
    return memory;
}
