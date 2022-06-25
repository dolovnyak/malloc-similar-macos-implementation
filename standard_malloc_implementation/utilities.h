#pragma once

#include "malloc_internal.h"

#define MASK_NODE_SIZE 0xFFFFFF0000000000
#define MASK_NODE_SIZE_SHIFT 40

#define MASK_PREV_NODE_SIZE 0x000000FFFFFF0000
#define MASK_PREV_NODE_SIZE_SHIFT 16

#define MASK_NODE_ZONE_START_OFFSET 0xFFFFFF0000000000
#define MASK_NODE_ZONE_START_OFFSET_SHIFT 40

#define MASK_NODE_NEXT_FREE_NODE_OFFSET 0x000000FFFFFF0000
#define MASK_NODE_NEXT_FREE_NODE_OFFSET_SHIFT 16

#define MASK_NODE_AVAILABLE 0x0000000000000004
#define MASK_NODE_AVAILABLE_SHIFT 2

#define MASK_NODE_TYPE 0x0000000000000003
#define MASK_NODE_TYPE_SHIFT 0

static inline uint64_t get_node_size(const BYTE* node_header) {
}

static inline void set_node_size(BYTE* node_header, uint64_t size) {
}

static inline uint64_t get_large_node_size(const BYTE* node_header) {
}

static inline void set_large_node_size(BYTE* node_header, uint64_t size) {
}

static inline uint64_t get_previous_node_size(const BYTE* node_header) {
}

static inline void set_previous_node_size(BYTE* node_header, uint64_t size) {
}

static inline uint64_t get_node_zone_start_offset(const BYTE* node_header) {
}

static inline void set_node_zone_start_offset(BYTE* node_header, uint64_t size) {
}

static inline uint64_t get_next_free_node_zone_start_offset(const BYTE* node_header) {
}

static inline void set_next_free_node_zone_start_offset(BYTE* node_header, uint64_t size) {
}

static inline BOOL get_node_available(const BYTE* node_header) {
    return *((uint64_t*)node_header + 1) & MASK_NODE_AVAILABLE;
}

static inline void set_node_available(BYTE* node_header, BOOL available) {
    uint64_t tmp = *((uint64_t*)node_header + 1);
    tmp = tmp & ~MASK_NODE_AVAILABLE;
    tmp = tmp | available << MASK_NODE_AVAILABLE_SHIFT;
}

static inline t_allocation_type get_node_allocation_type(BYTE* node_header) {

}

static inline void set_node_allocation_type(const BYTE* node_header, t_allocation_type type) {

}
