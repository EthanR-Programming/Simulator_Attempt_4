#include "mem.h"

ulong total_malloc;

void *mem_malloc(ulong size){
    void *ret_ptr = malloc(size);

    total_malloc += size;
    
    if (!ret_ptr){
        printf("MALLOC FAILED\n");
        return -1;
    }

    memset(ret_ptr , 0 , size);

    return ret_ptr;
}
