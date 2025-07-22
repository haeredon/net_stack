#include "execution_pool.h"
#include "log.h"


bool open(struct execution_pool_t* execution_pool) {
    bool allStarted = true;
    for(uint8_t i = 0 ; i < execution_pool->num_executions ; i++) {
        struct execution_context_t* execution_context = execution_pool->execution_contexts[i];
        
        if(execution_context->start(execution_context)) {
            NETSTACK_LOG(NETSTACK_WARNING, "Failed to start execution context\n"); 
            allStarted  = false;
        }
    }    

    return allStarted;
}

bool close(struct execution_pool_t* execution_pool) {
    for(uint8_t i = 0 ; i < execution_pool->num_executions ; i++) {
        struct execution_context_t* execution_context = execution_pool->execution_contexts[i];
        
        execution_context->stop(execution_context);
    }    

    return true;
}

struct execution_context_t* get_execution_context(struct execution_pool_t* execution_pool, uint64_t uuid) {

}

struct execution_context_t* get_any_execution_context(struct execution_pool_t* execution_pool) {
    
}

struct execution_pool_t create_execution_pool(void* (*mem_allocate)(const char *type, size_t size), uint8_t num_executions) {
    struct execution_context_t** execution_contexts = (struct execution_context_t**) mem_allocate("Execution pool: execution context array", 
        sizeof(struct execution_contexts*) * num_executions);

    for(uint8_t i = 0 ; i < execution_contexts ; i++) {
        execution_contexts[i] = create_netstack_thread(mem_allocate);
    }

    struct execution_pool_t execution_pool = {
        .execution_contexts = execution_contexts,
        .num_executions = num_executions,
        
        .open = open,
        .close = close,
        .get_execution_context = get_execution_context,
        .get_any_execution_context = get_any_execution_context
	};

	return execution_pool;
}