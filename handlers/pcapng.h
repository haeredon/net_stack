#ifndef KAGE_H
#define KAGE_H

#include "handler.h"

struct kage_t {
    struct rte_pcapng* pcap_fd;
    struct rte_mempool* mem_pool;
};

struct handler_t* pcapng_create_handler(struct handler_config_t *handler_config);






#endif // KAGE_H