#include <stdio.h>
#include "malloc_internal.h"

typedef struct s_a {
    void* zone_ptr;
    BYTE size;
    BYTE prev_node_size;
    uint16_t next_free_node_ptr;
    BYTE type;
} t_a;

typedef struct s_b {
    void* zone_ptr;
         ;
    BYTE prev_node_size;
    uint16_t next_free_node_ptr;
    BYTE type;
} t_b;

void print_bits(uint64_t value) {
    for (int i = 63; i >= 0; --i) {
        if ((i + 1) % 8 == 0) {
            printf(" ");
        }
        printf("%d ", (value & (1ULL << i)) ? 1 : 0);
    }
}

int main() {
    uint64_t val = 0x0000000000FFFFFF;
    print_bits(val << 40);
//    printf("size: %d\n", sizeof(t_a));
//    bool flag = true;
//    __uint128_t value = 0;
//    printf("value: %ld\n", value);
//    value = (value & ~(1UL << 2)) | (flag << 2);
//    printf("value: %ld\n", value);
//    flag = false;
//    value = (value & ~(1UL << 2)) | (flag << 2);
//    printf("value: %ld\n", value);
//    printf("size: %ld\n", sizeof(_Bool));
}