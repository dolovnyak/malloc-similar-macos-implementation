#include "malloc_internal.h"
#include "utilities.h"

/// construct new separated node
/// update first node size and connections
void separate_zone_free_node(BYTE* first_node, uint64_t first_node_new_size, t_zone* zone) {
    BYTE* second_node = first_node + NODE_HEADER_SIZE + first_node_new_size;

    uint64_t second_node_size = get_node_size(first_node) - (NODE_HEADER_SIZE + first_node_new_size);
    uint64_t second_node_zone_start_offset =
            get_node_zone_start_offset(first_node) + NODE_HEADER_SIZE + first_node_new_size;

    /// construct new second new
    construct_node_header(zone, second_node, second_node_size, first_node_new_size,
                          TRUE, get_node_allocation_type(first_node));
    add_node_to_free_list(zone, second_node);

    /// update first node size
    set_node_size(first_node, first_node_new_size);
}

void* take_memory_from_free_nodes(t_zone* zone, uint64_t required_size, uint64_t required_uint64_to_separate) {
    for (BYTE* current_node = zone->first_free_node;
         current_node != NULL; current_node = get_next_free_node(zone, current_node)) {
        t_node_representation current_node_representation = get_node_representation(current_node);

        if (current_node_representation.size >= required_size) {

            /// split node if it has enough free space
            if (current_node_representation.size - required_size >= required_uint64_to_separate) {
                separate_zone_free_node(current_node, required_size, zone);
            }

            delete_node_from_free_list(zone, current_node);
            set_node_available(current_node, FALSE);

            return current_node + NODE_HEADER_SIZE;
        }
    }

    return NULL;
}

void* take_not_marked_memory_from_zone(t_zone* zone, uint64_t required_size, t_allocation_type type) {
    BYTE* last_allocated_node = zone->last_allocated_node;
    uint64_t zone_occupied_memory_size = 0;
    uint64_t zone_available_size = zone->total_size;
    uint64_t last_allocated_node_size = 0;

    if (last_allocated_node != NULL) {
        last_allocated_node_size = get_node_size(last_allocated_node);
        zone_occupied_memory_size = (uint64_t)((last_allocated_node + NODE_HEADER_SIZE + last_allocated_node_size) -
                                               (BYTE*)zone - ZONE_HEADER_SIZE);
        zone_available_size = zone->total_size - zone_occupied_memory_size;
    }

    if (zone_available_size >= NODE_HEADER_SIZE + required_size) {
        BYTE* node = (BYTE*)zone + ZONE_HEADER_SIZE + zone_occupied_memory_size;

        construct_node_header(zone, node, required_size, last_allocated_node_size,
                              FALSE, type);

        zone->last_allocated_node = node;
        return (void*)(node + NODE_HEADER_SIZE);
    }
    return NULL;
}

void* take_memory_from_zone(t_zone* zone, uint64_t required_size, uint64_t required_uint64_to_separate,
                            t_allocation_type type) {
    void* mem = take_memory_from_free_nodes(zone, required_size, required_uint64_to_separate);
    if (mem) {
        return mem;
    }
    return take_not_marked_memory_from_zone(zone, required_size, type);
}

void* take_memory_from_zone_list(t_zone* first_zone, uint64_t required_size, uint64_t required_uint64_to_separate,
                                 t_allocation_type type) {
    for (t_zone* current_zone = first_zone; current_zone != NULL; current_zone = current_zone->next) {
        void* memory = take_memory_from_zone(current_zone, required_size, required_uint64_to_separate, type);
        if (memory != NULL) {
            return memory;
        }
    }
    return NULL;
}

void clear_zone_list(t_zone* current_zone) {
    while (current_zone != NULL) {
        t_zone* next_zone = current_zone->next;
        munmap((void*)current_zone, current_zone->total_size + ZONE_HEADER_SIZE);
        current_zone = next_zone;
    }
}

void free_memory_in_zone_list(t_zone** first_zone, t_zone** last_zone, BYTE* node) {
    t_node_representation current_node_representation = get_node_representation(node);
    t_zone* zone = current_node_representation.zone;

    /// merge with prev node if possible
    if (current_node_representation.prev_node != NULL && get_node_available(current_node_representation.prev_node)) {
        t_node_representation prev_node_representation = get_node_representation(current_node_representation.prev_node);

        uint64_t new_size = prev_node_representation.size + NODE_HEADER_SIZE + current_node_representation.size;
        set_node_size(prev_node_representation.raw_node, new_size);

        if (current_node_representation.raw_node == zone->last_allocated_node) {
            zone->last_allocated_node = prev_node_representation.raw_node;
        }
        delete_node_from_free_list(zone, current_node_representation.raw_node);
        current_node_representation = get_node_representation(prev_node_representation.raw_node);
    }

    /// merge with next node if possible
    if (current_node_representation.next_node != NULL && get_node_available(current_node_representation.next_node)) {
        t_node_representation next_node_representation = get_node_representation(current_node_representation.next_node);

        uint64_t new_size = current_node_representation.size + NODE_HEADER_SIZE + next_node_representation.size;
        set_node_size(current_node_representation.raw_node, new_size);

        if (next_node_representation.raw_node == zone->last_allocated_node) {
            zone->last_allocated_node = current_node_representation.raw_node;
        }
        delete_node_from_free_list(zone, next_node_representation.raw_node);
        current_node_representation = get_node_representation(current_node_representation.raw_node); // get changed node value
    }

    /// processing if released node is last last allocated node.
    if (current_node_representation.raw_node == zone->last_allocated_node) {

        /// if current node is last allocated node and we can delete zone (there is more than one zones)
        /// we delete zone from list and unmap it.
        if (current_node_representation.prev_node == NULL && *first_zone != *last_zone) {
            delete_zone_from_list(first_zone, last_zone, zone);
            munmap((void*)zone, ZONE_HEADER_SIZE + zone->total_size);
            return;
        }

        /// if current node is last zone node, but we can't delete zone (there is only one zone)
        /// we mark zone like totally free (without free node list)
        if (current_node_representation.prev_node == NULL) {
            zone->last_allocated_node = NULL;
            zone->first_free_node = NULL;
            zone->last_free_node = NULL;
            return;
        }

        /// we don't add node to free nodes list if it last, just unmark it
        zone->last_allocated_node = current_node_representation.prev_node;
        return;
    }

    set_node_available(node, TRUE);
    add_node_to_free_list(zone, node);
}
