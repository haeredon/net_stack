#ifndef DPDK_WRITE_H
#define DPDK_WRITE_H

#include <stdint.h>
#include <rte_ethdev.h>
#include<rte_ring.h>

#include "handlers/handler.h"

struct rte_eth_dev_tx_buffer* dpdk_write_tx_buffer[RTE_MAX_ETHPORTS];
struct rte_ring* dpdk_write_write_queue;

int64_t dpdk_write_write(struct out_buffer_t* out_buffer);

#endif // DPDK_WRITE_H