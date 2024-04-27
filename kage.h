#ifndef KAGE_H
#define KAGE_H

#include "worker.h"

struct kage_t {
    struct rte_pcapng* pcap_fd;
    struct rte_mempool* mem_pool;
};

void pcapng_close_handler(struct handler_t* handler);

void pcapng_init_handler(struct handler_t* handler);

uint16_t pcapng_read(struct rte_mbuf** packets, uint16_t num_packets, struct interface_t* interface, void* priv);

struct handler_t* pcapng_create_handler();






#endif // KAGE_H