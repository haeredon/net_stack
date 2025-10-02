#include "dpdk/write.h"


int64_t dpdk_write_write(struct out_buffer_t* out_buffer) {
	rte_ring_enqueue(dpdk_write_write_queue, out_buffer);
}

