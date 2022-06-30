#include "malloc_internal.h"

#include "utilities.h"

void free_all() {
    if (!gInit) { return; }
    gInit = FALSE;

    clear_zone_list(gMemoryZones.first_tiny_zone);
    clear_zone_list(gMemoryZones.first_small_zone);
    clear_zone_list(gMemoryZones.first_large_allocation);

    bzero(&gMemoryZones, sizeof(t_memory_zones));
}

/// needed because gtest using this free function instead external, but using function which using external malloc.
#ifdef GTEST
void __free(void* ptr) {
#else
void free(void* ptr) {
#endif
    if (ptr == NULL) {
        return;
    }

    BYTE* node = (BYTE*)ptr - NODE_HEADER_SIZE;
    t_allocation_type allocation_type = get_node_allocation_type(node);
    if (allocation_type == Large) {
        /// deallocate full large_allocation
        t_zone* large_allocation = (t_zone*)(node - ZONE_HEADER_SIZE);

        delete_zone_from_list(&gMemoryZones.first_large_allocation, &gMemoryZones.last_large_allocation, large_allocation);
        munmap((void*)large_allocation, large_allocation->total_size + ZONE_HEADER_SIZE);
    }
    else {
        t_zone** first_zone;
        t_zone** last_zone;
        switch (allocation_type) {
            case Tiny:
                first_zone = &gMemoryZones.first_tiny_zone;
                last_zone = &gMemoryZones.last_tiny_zone;
                break;
            case Small:
                first_zone = &gMemoryZones.first_small_zone;
                last_zone = &gMemoryZones.last_small_zone;
                break;
            case Large:
                exit(-1);
        }
        free_memory_in_zone_list(first_zone, last_zone, node);
    }
}