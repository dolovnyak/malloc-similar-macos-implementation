#include "malloc_internal.h"
#include "utilities.h"

#ifdef GTEST
void* __realloc(void* ptr, size_t new_size) {
#else
void* realloc(void* ptr, size_t new_size) {
#endif
    if (ptr == NULL) {
        return malloc(new_size);
    }
    if (new_size == 0) {
        free(ptr);
        return malloc(MINIMUM_SIZE_TO_ALLOCATE);
    }

    new_size = new_size + 15 & ~15;
    BYTE* node = (BYTE*)ptr - NODE_HEADER_SIZE;
    t_allocation_type allocation_type_from_node = get_node_allocation_type(node);
    t_allocation_type allocation_type_from_size = to_allocation_type(new_size);

    if (allocation_type_from_node < allocation_type_from_size) {
        /// skip to the end because we need to change zone
    }
    else if (allocation_type_from_node == Large) {
        t_large_node_representation node_representation = get_large_node_representation(node);
        if (node_representation.zone->total_size - NODE_HEADER_SIZE >= new_size) {
            set_node_size(node, new_size, Large);
            return ptr;
        }
    }
    else {
        uint64_t separate_size = (allocation_type_from_node == Tiny) ? (TINY_SEPARATE_SIZE) : (SMALL_SEPARATE_SIZE);
        if (reallocate_memory_in_zone(node, new_size, separate_size)) {
            return ptr;
        }
    }

    void* mem = malloc(new_size);
    if (!mem) {
        return NULL;
    }
    memcpy(mem, (void*)(node + NODE_HEADER_SIZE), get_node_size(node, allocation_type_from_node));
    free((void*)(node + NODE_HEADER_SIZE));
    return mem;
}