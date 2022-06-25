#include "malloc_internal.h"
#include "utilities.h"

BYTE* separate_node(BYTE* node, size_t left_part_size) {
    BYTE* new_node = node + NODE_HEADER_SIZE + left_part_size;
    set_node_available(new_node, TRUE);

    int64_t new_node_size = get_node_size(node) - (left_part_size + NODE_HEADER_SIZE);
    set_node_size(new_node, new_node_size, FALSE);

    int64_t prev_node_size = 1; // TOOO
    set_p

    new_node->next_free_node = NULL;
    new_node->usable_size = old_node->usable_size - SIZE_WITH_NODE_HEADER(left_part_size);

    return new_node;
}

void* take_memory_from_free_nodes(t_zone* zone, size_t required_size, size_t required_size_to_separate) {
    t_memory_node* prev_node = NULL;
    t_memory_node* splitted_node = NULL;

    for (t_memory_node* current_node = zone->first_free_node; current_node != NULL; current_node = current_node->next_free_node) {

        if (current_node->usable_size >= required_size) {

            /// split node if it has enough free space
            if (current_node->usable_size - required_size >= required_size_to_separate) {
                splitted_node = separate_node(current_node, required_size);
                splitted_node->next_free_node = current_node->next_free_node;
                current_node->next_free_node = splitted_node;
                current_node->usable_size = required_size;
            }

            if (prev_node == NULL) { // it also equals to (current_node == zone->first_free_node)
                zone->first_free_node = current_node->next_free_node;
            }
            else {
                prev_node->next_free_node = current_node->next_free_node;
            }

            if (zone->last_free_node == current_node) {
                if (splitted_node) {
                    zone->last_free_node = splitted_node;
                }
                else {
                    zone->last_free_node = prev_node;
                }
            }

            current_node->next_free_node = NULL;
            current_node->available = false;
            return CAST_TO_BYTE_APPLY_NODE_SHIFT(current_node);
        }
        prev_node = current_node;
    }

    return NULL;
}

void* take_memory_from_zone(t_zone* zone, size_t required_size, size_t required_size_to_separate) {
    void* mem = take_memory_from_free_nodes(zone, required_size, required_size_to_separate);
    if (mem) {
        return mem;
    }

    if (zone->available_size >= SIZE_WITH_NODE_HEADER(required_size)) {
        t_memory_node* node =
                (t_memory_node*)((BYTE*)zone + zone->total_size - zone->available_size + sizeof(t_zone));
        node->next_free_node = NULL;
        node->usable_size = required_size;
        node->available = false;

        zone->available_size -= SIZE_WITH_NODE_HEADER(required_size);

        return (void*)CAST_TO_BYTE_APPLY_NODE_SHIFT(node);
    }
    return NULL;
}

void* take_memory_from_zone_list(t_zone* first_zone, size_t required_size, size_t required_size_to_separate) {
    for (t_zone* current_zone = first_zone; current_zone != NULL; current_zone = current_zone->next) {
        void* memory = take_memory_from_zone(current_zone, required_size, required_size_to_separate);
        if (memory != NULL) {
            return memory;
        }
    }
    return NULL;
}

bool free_memory_in_zone_list(t_zone* first_zone, t_allocation_type zone_type, void* mem) {
    t_zone* prev_zone = NULL;
    for (t_zone* current_zone = first_zone; current_zone != NULL; current_zone = current_zone->next) {
        if ((BYTE*)mem > CAST_TO_BYTE_APPLY_ZONE_SHIFT(current_zone) &&
            (BYTE*)mem < CAST_TO_BYTE_APPLY_ZONE_SHIFT(current_zone) + current_zone->total_size) {

            if (zone_type == Large) {
                /// completely delete large zone
                if (prev_zone == NULL) {
                    gMemoryZones.first_large_allocation = current_zone->next;
                }
                else {
                    prev_zone->next = current_zone->next;
                }

                if (gMemoryZones.last_large_allocation == current_zone) {
                    gMemoryZones.last_large_allocation = prev_zone;
                }

                munmap(current_zone, SIZE_WITH_ZONE_HEADER(current_zone->total_size));
            }
            else {
                t_memory_node* node =
                /// * взять ноду где лежит эта память
                /// * пометить ее как освобожденную
                /// * идти по нодам после нее и пока они свободные сливать их в одну (удаляя их из списка свободных нод и увеличивая размер этой ноды)
                /// * проверить не идет ли после этой (возможно слитой) ноды свободное пространство, если идет
                /// то увеличить свободное место в зоне на размер ноды, с самой нодой ничего делать не нужно (не нужно добавлять ее в список free нод)
                /// * иначе (если после ноды идет другая, которая точно будет занаятой так как мы смерджили свободные на прошлом шаге)
                /// то добавить эту ноду в конец списка free нод.
            }
            return true;
        }
        prev_zone = current_zone;
    }
    return false;
}