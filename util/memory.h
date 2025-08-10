#ifndef UTIL_MEMORY_H
#define UTIL_MEMORY_H

#include <rte_malloc.h>

#define NET_STACK_MALLOC(TYPE, SIZE) malloc(SIZE)

#define NET_STACK_FREE(POINTER) free(void *POINTER)          

#endif // UTIL_MEMORY_H 