#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <mach/vm_statistics.h>
#include <string.h>

#include "malloc.h"

static bool gInit = false;
static t_memory_zones gMemoryZones;
static size_t gPageSize;

static size_t gTinyZoneSize;
static size_t gTinyNodeSize;

static size_t gSmallZoneSize;
static size_t gSmallNodeSize;

#define APPLY_NODE_HEADER_SHIFT(node) ((node) + sizeof(t_memory_node))
#define APPLY_NODE_HEADER_SIZE(size) ((size) + sizeof(t_memory_node))

static inline size_t CalculateZoneSize(t_allocation_type type, size_t size) {
    switch (type) {
        case Tiny:
            return gTinyZoneSize;
        case Small:
            return gSmallZoneSize;
        case Large:
            size += sizeof(t_zone) + sizeof(t_memory_node);
            return size + gPageSize - size % gPageSize;
    }
}

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
    gTinyNodeSize = gTinyZoneSize / 256;
    t_zone* tiny_default_zone = CreateNewZone(CalculateZoneSize(Tiny, 0));
    gMemoryZones.first_tiny_zone = tiny_default_zone;
    gMemoryZones.last_tiny_zone = tiny_default_zone;

    gSmallZoneSize = gPageSize * 16;
    gSmallNodeSize = gSmallZoneSize / 256;
    t_zone* small_default_zone = CreateNewZone(CalculateZoneSize(Small, 0));
    gMemoryZones.first_small_zone = small_default_zone;
    gMemoryZones.last_small_zone = small_default_zone;

    if (!tiny_default_zone || !small_default_zone) {
        return false;
    }
    return true;
}

static inline t_allocation_type ToType(size_t size) {
    if (size <= gTinyNodeSize * 8) {
        return Tiny;
    }
    if (size <= gSmallNodeSize * 8) {
        return Small;
    }
    return Large;
}

static inline t_memory_node* SplitNode(t_memory_node* old_node, size_t first_part_raw_size) {
    t_memory_node* new_node = old_node + first_part_raw_size;
    new_node->available = true;
    new_node->next = NULL;
    new_node->usable_size = old_node->usable_size - first_part_raw_size - sizeof(t_memory_node);

    return new_node;
}

static inline void* TakeMemoryFromFreeNodes(t_zone* zone, size_t required_size, size_t required_size_to_separate) {
    t_memory_node* prev_node = NULL;
    t_memory_node* splitted_node = NULL;

    for (t_memory_node* current_node = zone->first_free_node; current_node != NULL; current_node = current_node->next) {

        if (current_node->usable_size >= required_size) {

            /// split node if it has enough free space
            if (current_node->usable_size - required_size >= APPLY_NODE_HEADER_SIZE(required_size_to_separate)) {
                splitted_node = SplitNode(current_node, APPLY_NODE_HEADER_SIZE(required_size));
                splitted_node->next = current_node->next;
                current_node->next = splitted_node;
            }

            if (prev_node == NULL) { // it also equals to (current_node == zone->first_free_node)
                zone->first_free_node = current_node->next;
            } else {
                prev_node->next = current_node->next;
            }

            if (zone->last_free_node == current_node) {
                if (splitted_node) {
                    zone->last_free_node = splitted_node;
                } else {
                    zone->last_free_node = prev_node;
                }
            }

            current_node->next = NULL;
            current_node->available = false;
            return APPLY_NODE_HEADER_SHIFT(current_node);
        }
        prev_node = current_node;
    }

    return NULL;
}

static inline void* TakeMemoryFromZone(t_zone* zone, size_t required_size, size_t required_size_to_separate) {
    void* mem = TakeMemoryFromFreeNodes(zone, required_size, required_size_to_separate);
    if (mem) {
        return mem;
    }

    if (zone->available_size >= APPLY_NODE_HEADER_SIZE(required_size)) {
        t_memory_node* node =
                (t_memory_node*)zone + zone->total_size - zone->available_size + sizeof(t_zone) + sizeof(t_memory_node);
        node->next = NULL;
        node->usable_size = required_size;
        node->available = false;

        zone->available_size -= APPLY_NODE_HEADER_SIZE(required_size);

        return APPLY_NODE_HEADER_SHIFT(node);
    }
    return NULL;
}

static inline void* TakeMemoryFromZoneList(t_zone* first_zone, size_t required_size, size_t required_size_to_separate) {
    for (t_zone* current_zone = first_zone; current_zone != NULL; current_zone = current_zone->next) {
        void* memory = TakeMemoryFromZone(current_zone, required_size, required_size_to_separate);
        if (memory != NULL) {
            return memory;
        }
    }
    return NULL;
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

    /// 16 byte align (MacOS align). Equal to "usable_size + 16 - usable_size % 16".
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
        return (void*)large_allocation + sizeof(t_zone);
    }

    t_zone* first_zone;
    t_zone** last_zone;
    size_t required_size_to_separate;
    switch (allocation_type) {
        case Tiny:
            first_zone = gMemoryZones.first_tiny_zone;
            last_zone = &gMemoryZones.last_tiny_zone;
            required_size_to_separate = gTinyNodeSize;
            break;
        case Small:
            first_zone = gMemoryZones.first_small_zone;
            last_zone = &gMemoryZones.last_small_zone;
            required_size_to_separate = gSmallZoneSize;
            break;
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

int main() {
    char* a = malloc(5);
    a[0] = '0';
    a[1] = '1';
    a[2] = '2';
    a[3] = '3';
    a[4] = '\0';
    printf("%s\n", a);
    return 0;
}
