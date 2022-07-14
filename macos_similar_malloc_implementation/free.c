#include "malloc_internal.h"
#include "utilities.h"

void __free_all() {
    if (!gInit) { return; }
    gInit = FALSE;

    clear_zone_list(gMemoryZones.first_tiny_zone);
    clear_zone_list(gMemoryZones.first_small_zone);
    clear_zone_list(gMemoryZones.first_large_allocation);

    bzero(&gMemoryZones, sizeof(t_memory_zones));
}

static BOOL zone_list_contains_user_memory(t_zone* zone, void* ptr) {
    while (zone != NULL) {
        if ((BYTE*)ptr > (BYTE*)zone &&
            (BYTE*)ptr < (BYTE*)zone + ZONE_HEADER_SIZE + zone->total_size) {
            return TRUE;
        }
        zone = zone->next;
    }
    return FALSE;
}

static BOOL zones_contains_user_memory(void* ptr) {
    if (zone_list_contains_user_memory(gMemoryZones.first_tiny_zone, ptr)) {
        return TRUE;
    }
    if (zone_list_contains_user_memory(gMemoryZones.first_small_zone, ptr)) {
        return TRUE;
    }
    return zone_list_contains_user_memory(gMemoryZones.first_large_allocation, ptr);
}

void __free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
#ifdef SAFE_FREE
    if (!zones_contains_user_memory(ptr)) {
        return;
    }
#endif

    BYTE* node = (BYTE*)ptr - NODE_HEADER_SIZE;
    t_allocation_type allocation_type = get_node_allocation_type(node);
    if (allocation_type == Large) {
        /// deallocate full large_allocation
        t_zone* large_allocation = (t_zone*)(node - ZONE_HEADER_SIZE);
        delete_zone_from_list(&gMemoryZones.first_large_allocation, &gMemoryZones.last_large_allocation,
                              large_allocation);
        munmap((void*)large_allocation, large_allocation->total_size + ZONE_HEADER_SIZE);
        return;
    }

    t_zone** first_zone;
    t_zone** last_zone;
    if (allocation_type == Tiny) {
        first_zone = &gMemoryZones.first_tiny_zone;
        last_zone = &gMemoryZones.last_tiny_zone;
    }
    else {
        first_zone = &gMemoryZones.first_small_zone;
        last_zone = &gMemoryZones.last_small_zone;
    }
    free_memory_in_zone_list(first_zone, last_zone, node);
}