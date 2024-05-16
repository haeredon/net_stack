#ifndef KAGE_H
#define KAGE_H

#include "worker.h"

struct kage_t {
    struct rte_pcapng* pcap_fd;
    struct rte_mempool* mem_pool;
};

struct handler_t* pcapng_create_handler();






#endif // KAGE_H