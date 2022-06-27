#include "malloc_internal.h"
#include "utilities.h"

void separate_free_node(BYTE* first_node, size_t first_node_new_size, t_zone* zone) {
    BYTE* second_node = first_node + NODE_HEADER_SIZE + first_node_new_size;

    /// set second node size
    uint64_t second_node_size = get_node_size(first_node) - (NODE_HEADER_SIZE + first_node_new_size);
    set_node_size(second_node, second_node_size);

    /// set second node previous size
    set_previous_node_size(second_node, first_node_new_size);

    /// set second node zone start offset
    uint64_t second_node_zone_start_offset = get_node_zone_start_offset(first_node) + NODE_HEADER_SIZE + first_node_new_size;
    set_node_zone_start_offset(second_node, second_node_zone_start_offset);

    /// set second node next free node offset from first node
    uint64_t first_node_next_free_node_offset = get_next_free_node_zone_start_offset(first_node);
    set_next_free_node_zone_start_offset(second_node, first_node_next_free_node_offset);

    /// set second node available
    set_node_available(second_node, TRUE);

    /// set second node type
    set_node_allocation_type(second_node, get_node_allocation_type(first_node));


    /// set first node size (new reduced size)
    set_node_size(first_node, first_node_new_size);

    /// set first node next free node in second node
    set_next_free_node_zone_start_offset(first_node, second_node_zone_start_offset);

    /// if first node was last free node in zone we need update it
    if (zone->last_free_node == first_node) {
        zone->last_free_node = second_node;
    }
}

void* take_memory_from_free_nodes(t_zone* zone, size_t required_size, size_t required_size_to_separate) {
    BYTE* prev_node = NULL;

    for (BYTE* current_node = zone->first_free_node; current_node != NULL; current_node = get_next_free_node((BYTE*)zone, current_node)) {

        if (get_node_size(current_node) >= required_size) {

            /// split node if it has enough free space
            if (get_node_size(current_node) - required_size >= required_size_to_separate) {
                separate_free_node(current_node, required_size, zone);
            }

            if (prev_node == NULL) { // it also equals to (current_node == zone->first_free_node)
                zone->first_free_node = get_next_free_node((BYTE*)zone, current_node);
            }
            else {
                /// set previous node free_node_next to current_node->next_free_node
                set_next_free_node_zone_start_offset(prev_node, get_next_free_node_zone_start_offset(current_node));
            }

            if (zone->last_free_node == current_node) {
                zone->last_free_node = prev_node;
            }

            set_next_free_node_zone_start_offset(current_node, 0);
            set_node_available(current_node, FALSE);

            return current_node - NODE_HEADER_SIZE;
        }
        prev_node = current_node;
    }

    return NULL;
}

void* take_not_marked_memory_from_zone(t_zone* zone, size_t required_size, t_allocation_type type) {
    BYTE* last_allocated_node = zone->last_allocated_node;
    uint64_t zone_occupied_memory_size = 0;
    uint64_t zone_available_size = zone->total_size;
    uint64_t last_allocated_node_size = 0;

    if (last_allocated_node != NULL) {
        last_allocated_node_size = get_node_size(last_allocated_node);
        zone_occupied_memory_size = (uint64_t)((last_allocated_node + NODE_HEADER_SIZE + last_allocated_node_size) - (BYTE*)zone);
        zone_available_size = (zone->total_size + ZONE_HEADER_SIZE) - zone_occupied_memory_size;
    }

    if (zone_available_size >= NODE_HEADER_SIZE + required_size) {
        BYTE* node = (BYTE*)zone + ZONE_HEADER_SIZE + zone_occupied_memory_size;

        set_node_size(node, required_size);
        set_previous_node_size(node, last_allocated_node_size);
        set_node_zone_start_offset(node, zone_occupied_memory_size);
        set_next_free_node_zone_start_offset(node, 0);
        set_node_available(node, FALSE);
        set_node_allocation_type(node, type);

        zone->last_allocated_node = node;
    }
    return NULL;
}

void* take_memory_from_zone(t_zone* zone, size_t required_size, size_t required_size_to_separate, t_allocation_type type) {
    void* mem = take_memory_from_free_nodes(zone, required_size, required_size_to_separate);
    if (mem) {
        return mem;
    }
    return take_not_marked_memory_from_zone(zone, required_size, type);
}

void* take_memory_from_zone_list(t_zone* first_zone, size_t required_size, size_t required_size_to_separate, t_allocation_type type) {
    for (t_zone* current_zone = first_zone; current_zone != NULL; current_zone = current_zone->next) {
        void* memory = take_memory_from_zone(current_zone, required_size, required_size_to_separate, type);
        if (memory != NULL) {
            return memory;
        }
    }
    return NULL;
}

//BOOL free_memory_in_zone_list(t_zone* first_zone, t_allocation_type zone_type, void* mem) {
//    t_zone* prev_zone = NULL;
//    for (t_zone* current_zone = first_zone; current_zone != NULL; current_zone = current_zone->next) {
//        if ((BYTE*)mem > CAST_TO_BYTE_APPLY_ZONE_SHIFT(current_zone) &&
//            (BYTE*)mem < CAST_TO_BYTE_APPLY_ZONE_SHIFT(current_zone) + current_zone->total_size) {
//
//            if (zone_type == Large) {
//                /// completely delete large zone
//                if (prev_zone == NULL) {
//                    gMemoryZones.first_large_allocation = current_zone->next;
//                }
//                else {
//                    prev_zone->next = current_zone->next;
//                }
//
//                if (gMemoryZones.last_large_allocation == current_zone) {
//                    gMemoryZones.last_large_allocation = prev_zone;
//                }
//
//                munmap(current_zone, SIZE_WITH_ZONE_HEADER(current_zone->total_size));
//            }
//            else {
//                t_memory_node* node =
//                /// * взять ноду где лежит эта память
//                /// * пометить ее как освобожденную
//                /// * идти по нодам после нее и пока они свободные сливать их в одну (удаляя их из списка свободных нод и увеличивая размер этой ноды)
//                /// * проверить не идет ли после этой (возможно слитой) ноды свободное пространство, если идет
//                /// то увеличить свободное место в зоне на размер ноды, с самой нодой ничего делать не нужно (не нужно добавлять ее в список free нод)
//                /// * иначе (если после ноды идет другая, которая точно будет занаятой так как мы смерджили свободные на прошлом шаге)
//                /// то добавить эту ноду в конец списка free нод.
//            }
//            return true;
//        }
//        prev_zone = current_zone;
//    }
//    return false;
//}
