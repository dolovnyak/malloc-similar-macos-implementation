#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mach/vm_statistics.h>

#include "malloc.h"

typedef struct s_memory_zones t_memory_zones;
typedef struct s_zone t_zone;
typedef struct s_memory_node t_memory_node;

#define BYTE uint8_t
#define CAST_TO_BYTE_APPLY_ZONE_SHIFT(zone) ((BYTE*)(zone) + sizeof(t_zone))
#define CAST_TO_BYTE_APPLY_NODE_SHIFT(node) ((BYTE*)(node) + sizeof(t_memory_node))
#define SIZE_WITH_NODE_HEADER(size) ((size) + sizeof(t_memory_node))
#define SIZE_WITH_ZONE_HEADER(size) ((size) + sizeof(t_zone))

extern bool gInit;
extern t_memory_zones gMemoryZones;
extern size_t gPageSize;

extern size_t gTinyZoneSize;
extern size_t gTinyAllocationMaxSize;

extern size_t gSmallZoneSize;
extern size_t gSmallAllocationMaxSize;

/// We have 3 memory zones to optimize malloc speed and decrease mmap using.
///
/// Tiny zone usable_size is 4 * getpagesize() (usually page usable_size is 4096 byte)
/// and it contains allocations with usable_size higher than 0 and lower or equal to 4 * getpagesize() / 256
///
/// Small zone usable_size is 16 * getpagesize()
/// and it conatains allocations with usable_size higher than tiny zone allocations and lower or equal
/// to 16 * getpagesize() / 256
///
/// Large allocations fully own their zones. And these zones aren't preallocated.
/// Using for allocations with usable_size higher than 16 * getpagesize() / 256
typedef struct s_memory_zones {
    t_zone* first_tiny_zone;
    t_zone* last_tiny_zone;  /// last ptr using for fast new_zone inserting
    t_zone* first_small_zone;
    t_zone* last_small_zone;
    t_zone* first_large_allocation;
    t_zone* last_large_allocation;
} t_memory_zones;

/// Each zone contains: * ptr to next zone with equal type.
///                     * available memory usable_size (not allocated memory usable_size on the end of zone,
/// if we don't have free blocks we mark new memory block, set it as occupied and change available memory usable_size).
///                     * first free node ptr: when malloc requested and we searching memory - we check
/// all free nodes (which been occupied and freed before) to usable_size
/// zone structure looking like this:
/// [[zone_header]free_zone_space] <- zone after creating
/// [[zone_header][[node_header]node_space]free_zone_space] <- zone with one allocated node
typedef struct s_zone {
    struct s_zone* next;
    t_memory_node* first_free_node;
    t_memory_node* last_free_node;  /// using for fast inserting
    size_t available_size;
    size_t total_size;
} t_zone;

typedef struct s_memory_node {
    size_t usable_size;
    struct s_memory_node* next;
    bool available;
} t_memory_node;

typedef enum s_allocation_type {
    Tiny = 0,
    Small,
    Large
} t_allocation_type;

void* TakeMemoryFromZoneList(t_zone* first_zone, size_t required_size, size_t required_size_to_separate);
void* TakeMemoryFromZone(t_zone* zone, size_t required_size, size_t required_size_to_separate);
void* TakeMemoryFromFreeNodes(t_zone* zone, size_t required_size, size_t required_size_to_separate);
t_memory_node* SplitNode(t_memory_node* old_node, size_t left_part_size);

static inline t_allocation_type ToType(size_t size) {
    if (size <= gTinyAllocationMaxSize) {
        return Tiny;
    }
    if (size <= gSmallAllocationMaxSize) {
        return Small;
    }
    return Large;
}

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