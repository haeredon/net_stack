#include "dpdk/write.h"
#include "util/log.h"

struct rte_eth_dev_tx_buffer* dpdk_write_tx_buffer[RTE_MAX_ETHPORTS];
struct rte_ring* dpdk_write_write_queue;

int64_t dpdk_write_write(struct out_buffer_t* out_buffer) {
	int res = rte_ring_enqueue(dpdk_write_write_queue, out_buffer);

	if(res) {
		NETSTACK_LOG(NETSTACK_ERROR, "Failed to enqueue packet for write");            
	}
}

