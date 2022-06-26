#pragma once

#include "malloc_internal.h"

#define MASK_NODE_SIZE 0xFFFFFF0000000000
#define SHIFT_NODE_SIZE 40

#define MASK_PREV_NODE_SIZE 0x000000FFFFFF0000
#define SHIFT_PREV_NODE_SIZE 16

#define MASK_NODE_ZONE_START_OFFSET 0xFFFFFF0000000000
#define SHIFT_NODE_ZONE_START_OFFSET 40

#define MASK_NEXT_FREE_NODE_OFFSET 0x000000FFFFFF0000
#define SHIFT_NEXT_FREE_NODE_OFFSET 16

#define MASK_NODE_AVAILABLE 0x0000000000000004
#define SHIFT_NODE_AVAILABLE 2

#define MASK_NODE_TYPE 0x0000000000000003
#define SHIFT_NODE_TYPE 0

static inline uint64_t get(uint64_t variable_to_get, uint64_t mask, uint8_t shift) {
    return (variable_to_get & mask) >> shift;
}

static inline void set(uint64_t* variable_to_set, uint64_t mask, uint8_t shift, uint64_t value) {
    *variable_to_set &= ~mask; // set bits in value region to 0
    *variable_to_set |= (value << shift); // set value in value region
}

static inline uint64_t get_node_size(const BYTE* node_header) {
    return get(*(uint64_t*)node_header, MASK_NODE_SIZE, SHIFT_NODE_SIZE);
}

static inline void set_node_size(BYTE* node_header, uint64_t size) {
    set((uint64_t*)node_header, MASK_NODE_SIZE, SHIFT_NODE_SIZE, size);
}

static inline uint64_t get_large_node_size(const BYTE* node_header) {
    return *(uint64_t*)node_header;
}

static inline void set_large_node_size(BYTE* node_header, uint64_t size) {
    *(uint64_t*)node_header = size;
}

static inline uint64_t get_previous_node_size(const BYTE* node_header) {
    return get(*(uint64_t*)node_header, MASK_PREV_NODE_SIZE, SHIFT_PREV_NODE_SIZE);
}

static inline void set_previous_node_size(BYTE* node_header, uint64_t size) {
    set((uint64_t*)node_header, MASK_PREV_NODE_SIZE, SHIFT_PREV_NODE_SIZE, size);
}

static inline uint64_t get_node_zone_start_offset(const BYTE* node_header) {
    return get(*((uint64_t*)node_header + 1), MASK_NODE_ZONE_START_OFFSET, SHIFT_NODE_ZONE_START_OFFSET);
}

static inline void set_node_zone_start_offset(BYTE* node_header, uint64_t size) {
    set((uint64_t*)node_header + 1, MASK_NODE_ZONE_START_OFFSET, SHIFT_NODE_ZONE_START_OFFSET, size);
}

static inline uint64_t get_next_free_node_zone_start_offset(const BYTE* node_header) {
    return get(*((uint64_t*)node_header + 1), MASK_NEXT_FREE_NODE_OFFSET, SHIFT_NEXT_FREE_NODE_OFFSET);
}

static inline void set_next_free_node_zone_start_offset(BYTE* node_header, uint64_t size) {
    set((uint64_t*)node_header + 1, MASK_NEXT_FREE_NODE_OFFSET, SHIFT_NEXT_FREE_NODE_OFFSET, size);
}

static inline BOOL get_node_available(const BYTE* node_header) {
    return (BOOL)get(*((uint64_t*)node_header + 1), MASK_NODE_AVAILABLE, SHIFT_NODE_AVAILABLE);
}

static inline void set_node_available(BYTE* node_header, BOOL available) {
    set((uint64_t*)node_header + 1, MASK_NODE_AVAILABLE, SHIFT_NODE_AVAILABLE, (uint64_t)available);
}

static inline t_allocation_type get_node_allocation_type(const BYTE* node_header) {
    return (t_allocation_type)get(*((uint64_t*)node_header + 1), MASK_NODE_TYPE, SHIFT_NODE_TYPE);
}

static inline void set_node_allocation_type(BYTE* node_header, t_allocation_type type) {
    set((uint64_t*)node_header + 1, MASK_NODE_TYPE, SHIFT_NODE_TYPE, (uint64_t)type);
}
