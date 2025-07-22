#ifndef EXECUTION_POOL_H
#define EXECUTION_POOL_H

#include "handlers/handler.h"
#include "util/queue.h"
#include "worker.h"

#include<stdint.h>
#include<stdbool.h>

#define MAX_PKT_BURST 32


struct execution_pool_t {
    struct execution_context_t** execution_contexts;
    
    uint8_t num_executions;    

    bool (*open)(struct execution_pool_t* execution_pool);
    bool (*close)(struct execution_pool_t* execution_pool);

    struct execution_context_t* (*get_execution_context)(struct execution_pool_t* execution_pool, uint64_t uuid);
    struct execution_context_t* (*get_any_execution_context)(struct execution_pool_t* execution_pool);
};

struct execution_pool_t create_execution_pool(void* (*mem_allocate)(const char *type, size_t size), uint8_t num_executions);


#endif // EXECUTION_POOL_H