#include "dpdk/packet.h"

#include <rte_mbuf.h>

void* dpdk_packet_get_packet_buffer(void* packet) {
	struct rte_mbuf* buffer = (struct rte_mbuf*) packet;
	
	void* buffer_start = rte_pktmbuf_mtod(buffer, void *);
	rte_prefetch0(buffer_start); // learn more about this statement!!!

	return buffer_start;
}
void dpdk_packet_free_packet(void* packet) {
	rte_pktmbuf_free((struct rte_mbuf*) packet);
}

struct interface_t* dpdk_packet_get_interface(void* packet) {
    struct rte_mbuf* buffer = (struct rte_mbuf*) packet;
    return interfaces[buffer->port];
}