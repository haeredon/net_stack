#ifndef UTIL_MEMORY_H
#define UTIL_MEMORY_H

#include <rte_malloc.h>

#ifdef RTE_MEM_MANAGEMENT
    #define NET_STACK_MALLOC(TYPE, SIZE) rte_malloc(TYPE, SIZE, 0);
    #define NET_STACK_FREE(POINTER) rte_free(void *POINTER); 
#else
    #define NET_STACK_MALLOC(TYPE, SIZE) malloc(SIZE)
    #define NET_STACK_FREE(POINTER) free(void *POINTER)          
#endif

#endif // UTIL_MEMORY_H 