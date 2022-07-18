#pragma once

#include "malloc_internal.h"

#define MASK_NODE_SIZE 0xFFFFFF0000000000
#define SHIFT_NODE_SIZE 40

#define MASK_PREV_NODE_SIZE 0x000000FFFFFF0000
#define SHIFT_PREV_NODE_SIZE 16

#define MASK_NODE_ZONE_START_OFFSET_LEFT_PART 0x000000000000FFFF
#define MASK_NODE_ZONE_START_OFFSET_RIGHT_PART 0xFF00000000000000
#define SHIFT_NODE_ZONE_START_OFFSET_LEFT_PART 0
#define SHIFT_NODE_ZONE_START_OFFSET_RIGHT_PART 56
#define ZONE_VALUE_DELIMITER_SHIFT 8

#define MASK_PREV_FREE_NODE_OFFSET 0x00FFFFFF00000000
#define SHIFT_PREV_FREE_NODE_OFFSET 32

#define MASK_NEXT_FREE_NODE_OFFSET 0x00000000FFFFFF00
#define SHIFT_NEXT_FREE_NODE_OFFSET 8

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

static inline uint64_t get_node_size(const BYTE* node_header, t_allocation_type type) {
    return (type == Large) ?
           (*(uint64_t*)node_header) :
           (get(*(uint64_t*)node_header, MASK_NODE_SIZE, SHIFT_NODE_SIZE));
}

static inline void set_node_size(BYTE* node_header, uint64_t size, t_allocation_type type) {
    if (type == Large) {
        *(uint64_t*)node_header = size;
    }
    else {
        set((uint64_t*)node_header, MASK_NODE_SIZE, SHIFT_NODE_SIZE, size);
    }
}

static inline uint64_t get_prev_node_size(const BYTE* node_header) {
    return get(*(uint64_t*)node_header, MASK_PREV_NODE_SIZE, SHIFT_PREV_NODE_SIZE);
}

static inline void set_prev_node_size(BYTE* node_header, uint64_t size) {
    set((uint64_t*)node_header, MASK_PREV_NODE_SIZE, SHIFT_PREV_NODE_SIZE, size);
}

static inline uint64_t get_node_zone_start_offset(const BYTE* node_header) {
    uint64_t left_part = get(*(uint64_t*)node_header, MASK_NODE_ZONE_START_OFFSET_LEFT_PART,
                             SHIFT_NODE_ZONE_START_OFFSET_LEFT_PART);
    uint64_t right_part = get(*((uint64_t*)node_header + 1), MASK_NODE_ZONE_START_OFFSET_RIGHT_PART,
                              SHIFT_NODE_ZONE_START_OFFSET_RIGHT_PART);
    return (left_part << ZONE_VALUE_DELIMITER_SHIFT) | right_part;
}

static inline void set_node_zone_start_offset(BYTE* node_header, uint64_t size) {
    uint64_t left_part = size >> ZONE_VALUE_DELIMITER_SHIFT;
    uint64_t right_part =
            (size << (sizeof(uint64_t) - 8)) >> (sizeof(uint64_t) - 8); /// set all bits except last 8 bit to 0
    set((uint64_t*)node_header, MASK_NODE_ZONE_START_OFFSET_LEFT_PART, SHIFT_NODE_ZONE_START_OFFSET_LEFT_PART,
        left_part);
    set((uint64_t*)node_header + 1, MASK_NODE_ZONE_START_OFFSET_RIGHT_PART, SHIFT_NODE_ZONE_START_OFFSET_RIGHT_PART,
        right_part);
}

static inline uint64_t get_prev_free_node_zone_start_offset(const BYTE* node_header) {
    return get(*((uint64_t*)node_header + 1), MASK_PREV_FREE_NODE_OFFSET, SHIFT_PREV_FREE_NODE_OFFSET);
}

static inline void set_prev_free_node_zone_start_offset(BYTE* node_header, uint64_t size) {
    set((uint64_t*)node_header + 1, MASK_PREV_FREE_NODE_OFFSET, SHIFT_PREV_FREE_NODE_OFFSET, size);
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


static inline BYTE* get_next_free_node(t_zone* zone, BYTE* current_node) {
    uint64_t next_free_node_offset = get_next_free_node_zone_start_offset(current_node);
    if (next_free_node_offset == 0) {
        return NULL;
    }
    return (BYTE*)zone + next_free_node_offset;
}

static inline void set_next_free_node(BYTE* current_node, BYTE* node_to_set) {
    if (!node_to_set) {
        set_next_free_node_zone_start_offset(current_node, 0);
        return;
    }
    set_next_free_node_zone_start_offset(current_node, get_node_zone_start_offset(node_to_set));
}

static inline BYTE* get_next_node(t_zone* zone, BYTE* current_node) {
    if (current_node == zone->last_allocated_node) {
        return NULL;
    }
    return current_node + NODE_HEADER_SIZE + get_node_size(current_node, get_node_allocation_type(current_node));
}

static inline BYTE* get_prev_node(t_zone* zone, BYTE* current_node) {
    if (current_node == (BYTE*)zone + ZONE_HEADER_SIZE) {
        return NULL;
    }
    return current_node - get_prev_node_size(current_node) - NODE_HEADER_SIZE;
}

static inline BYTE* get_prev_free_node(t_zone* zone, BYTE* current_node) {
    uint64_t prev_free_node_offset = get_prev_free_node_zone_start_offset(current_node);
    if (prev_free_node_offset == 0) {
        return NULL;
    }
    return (BYTE*)zone + prev_free_node_offset;
}

static inline void set_prev_free_node(BYTE* current_node, BYTE* node_to_set) {
    if (!node_to_set) {
        set_prev_free_node_zone_start_offset(current_node, 0);
        return;
    }
    set_prev_free_node_zone_start_offset(current_node, get_node_zone_start_offset(node_to_set));
}

static inline void construct_node_header(t_zone* zone,
                                         BYTE* node,
                                         uint64_t size,
                                         uint64_t prev_node_size,
                                         t_allocation_type type) {
    set_node_size(node, size, type);
    set_prev_node_size(node, prev_node_size);
    set_node_zone_start_offset(node, node - (BYTE*)zone);
    set_node_available(node, FALSE);
    set_node_allocation_type(node, type);
}

static inline void construct_large_node_header(BYTE* node,
                                               uint64_t size) {
    set_node_size(node, size, Large);
    set_node_allocation_type(node, Large);
}

static inline t_node_representation get_node_representation(BYTE* node) {
    t_node_representation node_representation;
    node_representation.type = get_node_allocation_type(node);
    node_representation.raw_node = node;
    node_representation.zone = (t_zone*)(node - get_node_zone_start_offset(node));
    node_representation.size = get_node_size(node, node_representation.type);
    node_representation.prev_node = get_prev_node(node_representation.zone, node);
    node_representation.next_node = get_next_node(node_representation.zone, node);
    node_representation.prev_free_node = get_prev_free_node(node_representation.zone, node);
    node_representation.next_free_node = get_next_free_node(node_representation.zone, node);
    node_representation.available = get_node_available(node);
    return node_representation;
}

static inline t_large_node_representation get_large_node_representation(BYTE* node) {
    t_large_node_representation node_representation;
    node_representation.raw_node = node;
    node_representation.zone = (t_zone*)(node - ZONE_HEADER_SIZE);
    node_representation.size = get_node_size(node, Large);
    node_representation.type = Large;
    return node_representation;
}

static inline void add_node_to_available_list(t_zone* zone, BYTE* node_to_add) {
    if (zone->first_free_node == NULL) {
        set_prev_free_node(node_to_add, NULL);
        set_next_free_node(node_to_add, NULL);
        zone->first_free_node = node_to_add;
        zone->last_free_node = node_to_add;
    }
    else {
        set_prev_free_node(node_to_add, zone->last_free_node);
        set_next_free_node(node_to_add, NULL);
        set_next_free_node(zone->last_free_node, node_to_add);
        zone->last_free_node = node_to_add;
    }
    set_node_available(node_to_add, TRUE);
}

static inline void delete_node_from_available_list(t_zone* zone, BYTE* node_to_delete) {
    if (zone->first_free_node == zone->last_free_node) {
        zone->first_free_node = NULL;
        zone->last_free_node = NULL;
    }
    else {
        BYTE* prev_node = get_prev_free_node(zone, node_to_delete);
        BYTE* next_node = get_next_free_node(zone, node_to_delete);
        if (zone->first_free_node == node_to_delete) {
            zone->first_free_node = next_node;
            set_prev_free_node(zone->first_free_node, NULL);
        }
        else {
            set_next_free_node(prev_node, next_node);
        }

        if (zone->last_free_node == node_to_delete) {
            zone->last_free_node = prev_node;
            set_next_free_node(zone->last_free_node, NULL);
        }
        else {
            set_prev_free_node(next_node, prev_node);
        }
    }

    set_prev_free_node(node_to_delete, NULL);
    set_next_free_node(node_to_delete, NULL);
    set_node_available(node_to_delete, FALSE);
}

static inline void add_zone_to_list(t_zone** first_zone, t_zone** last_zone, t_zone* zone_to_add) {
    if (*first_zone == NULL) {
        zone_to_add->prev = NULL;
        zone_to_add->next = NULL;
        *first_zone = zone_to_add;
        *last_zone = zone_to_add;
    }
    else {
        zone_to_add->prev = (*last_zone);
        zone_to_add->next = NULL;
        (*last_zone)->next = zone_to_add;
        *last_zone = zone_to_add;
    }
}

static inline void delete_zone_from_list(t_zone** first_zone, t_zone** last_zone, t_zone* zone_to_delete) {
    if (*first_zone == *last_zone) {
        *first_zone = NULL;
        *last_zone = NULL;
        return;
    }

    if (*first_zone == zone_to_delete) {
        (*first_zone)->next->prev = NULL;
        *first_zone = (*first_zone)->next;
    }
    else {
        zone_to_delete->prev->next = zone_to_delete->next;
    }

    if (*last_zone == zone_to_delete) {
        (*last_zone)->prev->next = NULL;
        *last_zone = (*last_zone)->prev;
    }
    else {
        zone_to_delete->next->prev = zone_to_delete->prev;
    }
    zone_to_delete->next = NULL;
    zone_to_delete->prev = NULL;
}

static inline uint64_t get_zone_not_used_mem_size(t_zone* zone) {
    BYTE* last_node = zone->last_allocated_node;
    if (last_node == NULL) {
        return zone->total_size;
    }
    uint64_t node_size = get_node_size(last_node, get_node_allocation_type(last_node));
    uint64_t zone_occupied_memory_size = (uint64_t)((last_node + NODE_HEADER_SIZE + node_size) -
                                                    ((BYTE*)zone + ZONE_HEADER_SIZE));
    return zone->total_size - zone_occupied_memory_size;
}

static inline t_allocation_type to_allocation_type(uint64_t size) {
    if (size <= TINE_ALLOCATION_MAX_SIZE) {
        return Tiny;
    }
    if (size <= SMALL_ALLOCATION_MAX_SIZE) {
        return Small;
    }
    return Large;
}

static inline uint64_t calculate_zone_size(t_allocation_type type, uint64_t size) {
    switch (type) {
        case Tiny:
            return TINY_ZONE_SIZE;
        case Small:
            return SMALL_ZONE_SIZE;
        case Large:
            size += ZONE_HEADER_SIZE + NODE_HEADER_SIZE;
            return size + gPageSize - size % gPageSize;
    }
}
