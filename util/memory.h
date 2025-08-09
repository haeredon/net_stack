#ifndef UTIL_MEMORY_H
#define UTIL_MEMORY_H

#include <rte_malloc.h>

#define NET_STACK_MALLOC(TYPE, SIZE) rte_malloc(TYPE, SIZE, 0)

#define NET_STACK_free(POINTER) rte_free(void *POINTER)          

#endif // UTIL_MEMORY_H