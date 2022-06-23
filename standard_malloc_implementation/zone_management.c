#include "malloc.h"

t_memory_node* SplitNode(t_memory_node* old_node, size_t left_part_size) {
    t_memory_node* new_node = (t_memory_node*)((BYTE*)old_node + SIZE_WITH_NODE_HEADER(left_part_size));
    new_node->available = true;
    new_node->next = NULL;
    new_node->usable_size = old_node->usable_size - SIZE_WITH_NODE_HEADER(left_part_size);

    return new_node;
}

void* TakeMemoryFromFreeNodes(t_zone* zone, size_t required_size, size_t required_size_to_separate) {
    t_memory_node* prev_node = NULL;
    t_memory_node* splitted_node = NULL;

    for (t_memory_node* current_node = zone->first_free_node; current_node != NULL; current_node = current_node->next) {

        if (current_node->usable_size >= required_size) {

            /// split node if it has enough free space
            if (current_node->usable_size - required_size >= required_size_to_separate) {
                splitted_node = SplitNode(current_node, required_size);
                splitted_node->next = current_node->next;
                current_node->next = splitted_node;
                current_node->usable_size = required_size;
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
            return CAST_TO_BYTE_APPLY_NODE_SHIFT(current_node);
        }
        prev_node = current_node;
    }

    return NULL;
}

void* TakeMemoryFromZone(t_zone* zone, size_t required_size, size_t required_size_to_separate) {
    void* mem = TakeMemoryFromFreeNodes(zone, required_size, required_size_to_separate);
    if (mem) {
        return mem;
    }

    if (zone->available_size >= SIZE_WITH_NODE_HEADER(required_size)) {
        t_memory_node* node =
                (t_memory_node*)((BYTE*)zone + zone->total_size - zone->available_size + sizeof(t_zone));
        node->next = NULL;
        node->usable_size = required_size;
        node->available = false;

        zone->available_size -= SIZE_WITH_NODE_HEADER(required_size);

        return (void*)CAST_TO_BYTE_APPLY_NODE_SHIFT(node);
    }
    return NULL;
}

void* TakeMemoryFromZoneList(t_zone* first_zone, size_t required_size, size_t required_size_to_separate) {
    for (t_zone* current_zone = first_zone; current_zone != NULL; current_zone = current_zone->next) {
        void* memory = TakeMemoryFromZone(current_zone, required_size, required_size_to_separate);
        if (memory != NULL) {
            return memory;
        }
    }
    return NULL;
}
