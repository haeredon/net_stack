#ifndef WORKER_H
#define WORKER_H

#include "handlers/handler.h"
#include "util/queue.h"

#include<stdint.h>
#include<rte_mbuf.h>
#include<pthread.h>

#define MAX_PKT_BURST 32



struct lcore_setup_t {
    struct interface_t interface;
    struct handler_t** handlers;
    uint8_t num_handlers;
};

int worker_start_lcore_worker(void* setups);

extern volatile bool force_quit; 


/************************************************************************************ */


struct execution_context_t {
    struct queue_t* work_queue;        

    pthread_t thread;

    int (*start)(struct execution_context_t* thread);
};

struct execution_context_t create_netstack_thread(void* (*mem_allocate)(const char *type, size_t size));


#endif // WORKER_H