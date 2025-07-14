#ifndef WORKER_H
#define WORKER_H

#include "handlers/handler.h"

#include <stdint.h>
#include <rte_mbuf.h>

#define MAX_PKT_BURST 32



struct lcore_setup_t {
    struct interface_t interface;
    struct handler_t** handlers;
    uint8_t num_handlers;
};

int worker_start_lcore_worker(void* setups);

extern volatile bool force_quit; 


/************************************************************************************ */

// FIFO queue
struct work_queue_t {
    void (*enqueue)();
    void (*dequeue)();
    void (*notify)();    
};

struct thread_t {
    struct work_queue_t work_queue;        

    void (*run)();
};

struct thread_t create_netstack_thread();


#endif // WORKER_H