#include "dpdk/write.h"


struct rte_eth_dev_tx_buffer* dpdk_write_tx_buffer[RTE_MAX_ETHPORTS];
struct rte_ring* dpdk_write_write_queue;

int64_t dpdk_write_write(struct out_buffer_t* out_buffer) {
	rte_ring_enqueue(dpdk_write_write_queue, out_buffer);
}

