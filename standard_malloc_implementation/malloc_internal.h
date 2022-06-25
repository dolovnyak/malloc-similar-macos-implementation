#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mach/vm_statistics.h>

#include "malloc.h"

typedef struct s_memory_zones t_memory_zones;
typedef struct s_zone t_zone;

#define BYTE uint8_t

#define BOOL BYTE
#define TRUE 1
#define FALSE 0

#define NODE_HEADER_SIZE 16
#define ZONE_HEADER_SIZE sizeof(t_zone)

//#define NODE_AVAILABLE_BIT_MASK 0x00000

#define CAST_TO_BYTE_APPLY_ZONE_SHIFT(zone) ((BYTE*)(zone) + sizeof(t_zone))
//#define CAST_TO_BYTE_APPLY_NODE_SHIFT(node) ((BYTE*)(node) + NODE_HEADER_SIZE)
//#define SIZE_WITH_NODE_HEADER(size) ((size) + NODE_HEADER_SIZE)
//#define SIZE_WITH_ZONE_HEADER(size) ((size) + sizeof(t_zone))

extern BOOL gInit;
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

/// zone memory structure looking like this:
/// [[zone_header]free_zone_space] <- zone after creating
/// [[zone_header][[node_header]node_space]free_zone_space] <- zone with one allocated node
typedef struct s_zone {
    struct s_zone* next;
    BYTE* first_free_node;
    BYTE* last_free_node;  /// using for fast inserting
    size_t available_size;
    size_t total_size;
} t_zone;

/// for memory optimization memory_node header doesn't have structure and we work with it using bit operations.
/// memory_node header size is 16 byte for all types (tiny, small, large).

/// memory_node header for tiny and small objects looks like this:
/**
 * memory_node {
 * 24_bit size;
 * 24_bit previous_node_size;
 * 16_bit not_used_memory; // needed for easy bit operations with two int64 numbers.
 * ---- end of 64 byte
 * 24_bit offset_from_zone_start;
 * 24_bit next_free_node_offset_from_zone_start;
 * 13_bit not_used_memory;
 * 1_bit  available;
 * 2_bit  node_type; (tiny/small/large)
 * ---- end of 128 byte
}
 */

/// memory_node header for large objects looks like this:
/**
 * large_memory_node {
 * 64_bit size
 * 62_bit not_used_memory;
 * 2_bit  node_type; (tiny/small/large)
 * }
 */

typedef enum s_allocation_type {
    Tiny = 0,
    Small,
    Large
} t_allocation_type;

void* take_memory_from_zone_list(t_zone* first_zone, size_t required_size, size_t required_size_to_separate);
void* take_memory_from_zone(t_zone* zone, size_t required_size, size_t required_size_to_separate);
void* take_memory_from_free_nodes(t_zone* zone, size_t required_size, size_t required_size_to_separate);
BYTE* separate_node(BYTE* node, size_t left_part_size);

BOOL free_memory_in_zone_list(t_zone* first_zone, t_allocation_type zone_type, void* mem);

static inline t_allocation_type to_type(size_t size) {
    if (size <= gTinyAllocationMaxSize) {
        return Tiny;
    }
    if (size <= gSmallAllocationMaxSize) {
        return Small;
    }
    return Large;
}

static inline size_t calculate_zone_size(t_allocation_type type, size_t size) {
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
