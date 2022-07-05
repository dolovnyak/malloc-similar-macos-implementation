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

#define MINIMUM_SIZE_TO_ALLOCATE 16

extern BOOL gInit;
extern t_memory_zones gMemoryZones;
extern int gPageSize;

#define TINY_ZONE_SIZE 0x100000 /// 1 mb
#define TINE_ALLOCATION_MAX_SIZE 128
#define TINY_SEPARATE_SIZE TINE_ALLOCATION_MAX_SIZE / 2 + NODE_HEADER_SIZE

#define SMALL_ZONE_SIZE 0x800000 /// 8 mb
#define SMALL_ALLOCATION_MAX_SIZE 512
#define SMALL_SEPARATE_SIZE SMALL_ALLOCATION_MAX_SIZE / 2 + NODE_HEADER_SIZE

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
    struct s_zone* prev;
    BYTE* first_free_node;
    BYTE* last_free_node;  /// using for fast inserting
    BYTE* last_allocated_node;
    uint64_t total_size;
} t_zone;

/// for memory optimization memory_node header doesn't have structure and we work with it using bit operations.
/// memory_node header size is 16 byte for all types (tiny, small, large).

/// memory_node header for tiny and small objects looks like this:
/**
 * memory_node {
 * 24_bit size;
 * 24_bit previous_node_size;
 * 24_bit offset_from_zone_start;
 * 24_bit prev_free_node_offset_from_zone_start;
 * 24_bit next_free_node_offset_from_zone_start;
 * 1_bit  not_used_memory;
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
    Small = 1,
    Large = 2
} t_allocation_type;

typedef struct s_node_representation {
    BYTE* raw_node;
    t_zone* zone;
    uint64_t size;
    BYTE* prev_node;
    BYTE* next_node;
    BYTE* prev_free_node;
    BYTE* next_free_node;
    BOOL available;
    t_allocation_type type;
} t_node_representation;

typedef struct s_large_node_representation {
    BYTE* raw_node;
    t_zone* zone;
    uint64_t size;
    t_allocation_type type;
} t_large_node_representation;

BOOL init();

void* take_memory_from_zone_list(t_zone* first_zone, uint64_t required_size, uint64_t separate_size, t_allocation_type type);
void* take_memory_from_zone(t_zone* zone, uint64_t required_size, uint64_t separate_size, t_allocation_type type);
void* take_memory_from_free_nodes(t_zone* zone, uint64_t required_size, uint64_t separate_size, t_allocation_type type);
void separate_node_on_new_free_node(BYTE* first_node, uint64_t first_node_new_size, t_zone* zone, t_allocation_type type);
BOOL reallocate_memory_in_zone(BYTE* node, uint64_t new_size, uint64_t separate_size);

t_zone* create_new_zone(size_t size);

void free_memory_in_zone_list(t_zone** first_zone, t_zone**last_zone, BYTE* node);
void clear_zone_list(t_zone* current_zone);

/// for gtest
void __free(void* ptr);
void* __malloc(size_t required_size);
void* __realloc(void* ptr, size_t new_size);
