#ifndef WORKER_H
#define WORKER_H

#include <stdint.h>

#define MAX_PKT_BURST 32

struct interface_t {
     uint16_t port;
     uint32_t queue;
};

struct operations_t {
    uint16_t (*read)(struct rte_mbuf** packets, uint16_t num_packets, struct interface_t* interface, void* priv);    
};

struct handler_t {
    void (*init)(struct handler_t*); 
    void (*close)(struct handler_t*);
    struct operations_t operations;

    void* priv;
};

struct lcore_setup_t {
    struct interface_t interface;
    struct handler_t* handlers;
};

int worker_start_lcore_worker(void* setups);

extern volatile bool force_quit; 

#endif // WORKER_H