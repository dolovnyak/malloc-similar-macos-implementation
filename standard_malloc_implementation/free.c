#include "malloc_internal.h"

static inline void clear_zone_list(t_zone* current_zone) {
    while (current_zone != NULL) {
        t_zone* next_zone = current_zone->next;
        if (munmap((void*)current_zone, SIZE_WITH_ZONE_HEADER(current_zone->total_size)) == -1) {
            exit(-1);
        }
        current_zone = next_zone;
    }
}

void free_all() {
    if (!gInit) { return; }
    gInit = false;

    clear_zone_list(gMemoryZones.first_tiny_zone);
    clear_zone_list(gMemoryZones.first_small_zone);
    clear_zone_list(gMemoryZones.first_large_allocation);

    bzero(&gMemoryZones, sizeof(t_memory_zones));
}